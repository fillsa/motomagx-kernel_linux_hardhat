/** @file wlan_join.c
 *
 *  @brief Functions implementing wlan infrastructure and adhoc join routines
 *
 *  IOCTL handlers as well as command preperation and response routines
 *   for sending adhoc start, adhoc join, and association commands
 *   to the firmware.
 *  
 *  (c) Copyright © 2003-2006, Marvell International Ltd. 
 *   
 *  This software file (the "File") is distributed by Marvell International 
 *  Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 *  (the "License").  You may use, redistribute and/or modify this File in 
 *  accordance with the terms and conditions of the License, a copy of which 
 *  is available along with the File in the gpl.txt file or by writing to 
 *  the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 *  02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 *  THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 *  this warranty disclaimer.
 *
 */
/*************************************************************
Change Log:
    01/11/06: Initial revision. Match new scan code, relocate related functions
    01/19/06: Fix failure to save adhoc ssid as current after adhoc start
    03/16/06: Add a semaphore to protect reassociation thread
    10/16/06: Add comments for association response status code

************************************************************/

#include    "include.h"

/**
 *  @brief This function finds out the common rates between rate1 and rate2.
 *
 * It will fill common rates in rate1 as output if found.
 *
 * NOTE: Setting the MSB of the basic rates need to be taken
 *   care, either before or after calling this function
 *
 *  @param Adapter     A pointer to wlan_adapter structure
 *  @param rate1       the buffer which keeps input and output
 *  @param rate1_size  the size of rate1 buffer
 *  @param rate2       the buffer which keeps rate2
 *  @param rate2_size  the size of rate2 buffer.
 *
 *  @return            WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
get_common_rates(wlan_adapter * Adapter, u8 * rate1,
                 int rate1_size, u8 * rate2, int rate2_size)
{
    u8 *ptr = rate1;
    int ret = WLAN_STATUS_SUCCESS;
    u8 *tmp = NULL;
    int i, j;

    if (!(tmp = kmalloc(rate1_size, GFP_KERNEL))) {
        PRINTM(WARN, "Allocate buffer for common rates failed\n");
        return -ENOMEM;
    }

    memcpy(tmp, rate1, rate1_size);
    memset(rate1, 0, rate1_size);

    for (i = 0; rate2[i] && i < rate2_size; i++) {
        for (j = 0; tmp[j] && j < rate1_size; j++) {
            /* Check common rate, excluding the bit for basic rate */
            if ((rate2[i] & 0x7F) == (tmp[j] & 0x7F)) {
                *rate1++ = tmp[j];
                break;
            }
        }
    }

    HEXDUMP("rate1 (AP) Rates", tmp, rate1_size);
    HEXDUMP("rate2 (Card) Rates", rate2, rate2_size);
    HEXDUMP("Common Rates", ptr, rate1 - ptr);
    PRINTM(INFO, "Tx DataRate is set to 0x%X\n", Adapter->DataRate);

    if (!Adapter->Is_DataRate_Auto) {
        while (*ptr) {
            if ((*ptr & 0x7f) == Adapter->DataRate) {
                ret = WLAN_STATUS_SUCCESS;
                goto done;
            }
            ptr++;
        }
        PRINTM(MSG, "Previously set fixed data rate %#x isn't "
               "compatible with the network.\n", Adapter->DataRate);

        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    ret = WLAN_STATUS_SUCCESS;
  done:
    kfree(tmp);
    return ret;
}

/**
 *  @brief Send Deauth Request
 *
 *  @param priv      A pointer to wlan_private structure
 *  @return          WLAN_STATUS_SUCCESS--success, otherwise fail
 */
int
wlan_send_deauth(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (Adapter->InfrastructureMode == Wlan802_11Infrastructure &&
        Adapter->MediaConnectStatus == WlanMediaStateConnected) {

        ret = SendDeauthentication(priv);
    } else {
        LEAVE();
        return -ENOTSUPP;
    }

    LEAVE();
    return ret;
}

/**
 *  @brief Retrieve the association response
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_get_assoc_rsp_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int copySize;

    /*
     * Set the amount to copy back to the application as the minimum of the
     *   available assoc resp data or the buffer provided by the application
     */
    copySize = MIN(Adapter->assocRspSize, wrq->u.data.length);

    /* Copy the (re)association response back to the application */
    if (copy_to_user(wrq->u.data.pointer, Adapter->assocRspBuffer, copySize)) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }

    /* Returned copy length */
    wrq->u.data.length = copySize;

    /* Reset assoc buffer */
    Adapter->assocRspSize = 0;

    /* No error on return */
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Set an opaque block of Marvell TLVs for insertion into the
 *         association command
 *
 *  Pass an opaque block of data, expected to be Marvell TLVs, to the driver
 *    for eventual passthrough to the firmware in an associate/join
 *    (and potentially start) command.
 *
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_mrvl_tlv_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    /* If the passed length is zero, reset the buffer */
    if (wrq->u.data.length == 0) {
        Adapter->mrvlTlvBufferLen = 0;
    } else {
        /*
         * Verify that the passed length is not larger than the available
         *   space remaining in the buffer
         */
        if (wrq->u.data.length <
            (sizeof(Adapter->mrvlTlvBuffer) - Adapter->mrvlTlvBufferLen)) {
            /* Append the passed data to the end of the mrvlTlvBuffer */
            if (copy_from_user
                (Adapter->mrvlTlvBuffer + Adapter->mrvlTlvBufferLen,
                 wrq->u.data.pointer, wrq->u.data.length)) {
                PRINTM(INFO, "Copy from user failed\n");
                return -EFAULT;
            }

            /* Increment the stored buffer length by the size passed */
            Adapter->mrvlTlvBufferLen += wrq->u.data.length;
        } else {
            /* Passed data does not fit in the remaining buffer space */
            ret = WLAN_STATUS_FAILURE;
        }
    }

    /* Return WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE */
    return ret;
}

/**
 *  @brief Stop Adhoc Network
 *
 *  @param priv         A pointer to wlan_private structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_do_adhocstop_ioctl(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (Adapter->InfrastructureMode == Wlan802_11IBSS &&
        Adapter->MediaConnectStatus == WlanMediaStateConnected) {

        ret = StopAdhocNetwork(priv);

    } else {
        LEAVE();
        return -ENOTSUPP;
    }

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Set essid
 *
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param dwrq         A pointer to iw_point structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
int
wlan_set_essid(struct net_device *dev, struct iw_request_info *info,
               struct iw_point *dwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    WLAN_802_11_SSID reqSSID;
    int i;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    /* Clear any past association response stored for application retrieval */
    Adapter->assocRspSize = 0;

#ifdef REASSOCIATION
    // cancel re-association timer if there's one
    if (Adapter->TimerIsSet == TRUE) {
        CancelTimer(&Adapter->MrvDrvTimer);
        Adapter->TimerIsSet = FALSE;
    }

    if (OS_ACQ_SEMAPHORE_BLOCK(&Adapter->ReassocSem)) {
        PRINTM(ERROR, "Acquire semaphore error, wlan_set_essid\n");
        return -EBUSY;
    }
#endif /* REASSOCIATION */

    /* Check the size of the string */
    if (dwrq->length > IW_ESSID_MAX_SIZE + 1) {
        PRINTM(ERROR, "Set essid ioctl: wrong length\n");
        ret = -E2BIG;
        goto setessid_ret;
    }

    memset(&reqSSID, 0, sizeof(WLAN_802_11_SSID));

    /*
     * Check if we asked for `any' or 'particular'
     */
    if (!dwrq->flags) {
        if (FindBestNetworkSsid(priv, &reqSSID)) {
#ifdef MOTO_DBG
            PRINTM(MOTO_DEBUG,
                   "set ssid ioctl: Could not find best network\n");
#else
            PRINTM(INFO, "Could not find best network\n");
#endif
            ret = WLAN_STATUS_SUCCESS;
            goto setessid_ret;
        }
    } else {
        /* Set the SSID */
        reqSSID.SsidLength = dwrq->length - 1;
        memcpy(reqSSID.Ssid, extra, reqSSID.SsidLength);

    }

#ifdef MOTO_DBG
    PRINTM(MOTO_MSG,
           "set ssid ioctl called by upper layer: Requested new SSID = %s (%s)\n",
           (reqSSID.SsidLength > 0) ? (char *) reqSSID.Ssid : "NULL",
           (dwrq->flags ==
            0) ? "Best Network found" : "Provided in set ssid command");
#else
    PRINTM(INFO, "Requested new SSID = %s\n",
           (reqSSID.SsidLength > 0) ? (char *) reqSSID.Ssid : "NULL");
#endif

    if (!reqSSID.SsidLength || reqSSID.Ssid[0] < 0x20) {
        PRINTM(INFO, "Invalid SSID - aborting set_essid\n");
        ret = -EINVAL;
        goto setessid_ret;
    }

    /* If the requested SSID is not a NULL string, join */

    if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
        /* infrastructure mode */
        PRINTM(INFO, "SSID requested = %s\n", reqSSID.Ssid);

        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            PRINTM(INFO, "Already Connected ..\n");
            ret = SendDeauthentication(priv);

            if (ret) {
                goto setessid_ret;
            }
        }

        if (Adapter->Prescan) {
            SendSpecificSSIDScan(priv, &reqSSID);
        }

        i = FindSSIDInList(Adapter, &reqSSID, NULL, Wlan802_11Infrastructure);
        if (i >= 0) {
            PRINTM(INFO, "SSID found in scan list ... associating...\n");

            ret = wlan_associate(priv, &Adapter->ScanTable[i]);

            if (ret) {
                goto setessid_ret;
            }
        } else {                /* i >= 0 */
            ret = i;            /* return -ENETUNREACH, passed from FindSSIDInList */
            goto setessid_ret;
        }
    } else {
        /* ad hoc mode */
        /* If the requested SSID matches current SSID return */
        if (!SSIDcmp(&Adapter->CurBssParams.ssid, &reqSSID)) {
            ret = WLAN_STATUS_SUCCESS;
            goto setessid_ret;
        }

        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            /*
             * Exit Adhoc mode
             */
            PRINTM(INFO, "Sending Adhoc Stop\n");
            ret = StopAdhocNetwork(priv);

            if (ret) {
                goto setessid_ret;
            }
        }
        Adapter->AdhocLinkSensed = FALSE;

        /* Scan for the network */
        SendSpecificSSIDScan(priv, &reqSSID);

        /* Search for the requested SSID in the scan table */
        i = FindSSIDInList(Adapter, &reqSSID, NULL, Wlan802_11IBSS);

        if (i >= 0) {
            PRINTM(INFO, "SSID found at %d in List, so join\n", i);
            JoinAdhocNetwork(priv, &Adapter->ScanTable[i]);
        } else {
            /* else send START command */
            PRINTM(INFO, "SSID not found in list, "
                   "so creating adhoc with ssid = %s\n", reqSSID.Ssid);

            StartAdhocNetwork(priv, &reqSSID);
        }                       /* end of else (START command) */
    }                           /* end of else (Ad hoc mode) */

    /*
     * The MediaConnectStatus change can be removed later when
     *   the ret code is being properly returned.
     */
    /* Check to see if we successfully connected */
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        ret = WLAN_STATUS_SUCCESS;
    } else {
        ret = -ENETDOWN;
    }

  setessid_ret:
#ifdef REASSOCIATION
    OS_REL_SEMAPHORE(&Adapter->ReassocSem);
#endif

    LEAVE();
    return ret;
}

/**
 *  @brief Connect to the AP or Ad-hoc Network with specific bssid
 *
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param awrq         A pointer to iw_param structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_wap(struct net_device *dev, struct iw_request_info *info,
             struct sockaddr *awrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    const u8 bcast[ETH_ALEN] = { 255, 255, 255, 255, 255, 255 };
    u8 reqBSSID[ETH_ALEN];
    int i;

    ENTER();

#ifdef MOTO_DBG
    PRINTM(MOTO_DEBUG,
           "SIOCSIWAP (wlan_set_wap) ioctl called by upper layer\n");
#endif

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    /* Clear any past association response stored for application retrieval */
    Adapter->assocRspSize = 0;

//Application should call scan before call this function.    

    if (awrq->sa_family != ARPHRD_ETHER)
        return -EINVAL;

#ifdef MOTO_DBG
    PRINTM(MOTO_MSG, "SIOCSIWAP: sa_data: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (u8) awrq->sa_data[0], (u8) awrq->sa_data[1],
           (u8) awrq->sa_data[2], (u8) awrq->sa_data[3],
           (u8) awrq->sa_data[4], (u8) awrq->sa_data[5]);
#else
    PRINTM(INFO, "ASSOC: WAP: sa_data: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (u8) awrq->sa_data[0], (u8) awrq->sa_data[1],
           (u8) awrq->sa_data[2], (u8) awrq->sa_data[3],
           (u8) awrq->sa_data[4], (u8) awrq->sa_data[5]);
#endif

#ifdef REASSOCIATION
    // cancel re-association timer if there's one
    if (Adapter->TimerIsSet == TRUE) {
        CancelTimer(&Adapter->MrvDrvTimer);
        Adapter->TimerIsSet = FALSE;
    }
#endif /* REASSOCIATION */

    if (!memcmp(bcast, awrq->sa_data, ETH_ALEN)) {
        i = FindBestSSIDInList(Adapter);
    } else {
        memcpy(reqBSSID, awrq->sa_data, ETH_ALEN);

        PRINTM(INFO, "ASSOC: WAP: Bssid = %02x:%02x:%02x:%02x:%02x:%02x\n",
               reqBSSID[0], reqBSSID[1], reqBSSID[2],
               reqBSSID[3], reqBSSID[4], reqBSSID[5]);

        /* Search for index position in list for requested MAC */
        i = FindBSSIDInList(Adapter, reqBSSID, Adapter->InfrastructureMode);
    }

    if (i < 0) {
#ifdef MOTO_DBG
        PRINTM(ERROR, "ASSOC: WAP: MAC address not found in BSSID List\n");
#else
        PRINTM(INFO, "ASSOC: WAP: MAC address not found in BSSID List\n");
#endif
        return -ENETUNREACH;
    }

    if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            ret = SendDeauthentication(priv);

            if (ret) {
                LEAVE();
                return ret;
            }
        }
        ret = wlan_associate(priv, &Adapter->ScanTable[i]);

        if (ret) {
            LEAVE();
            return ret;
        }
    } else {
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            /* Exit Adhoc mode */
            ret = StopAdhocNetwork(priv);

            if (ret) {
                LEAVE();
                return ret;
            }
        }
        Adapter->AdhocLinkSensed = FALSE;

        JoinAdhocNetwork(priv, &Adapter->ScanTable[i]);
    }

    /* Check to see if we successfully connected */
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        ret = WLAN_STATUS_SUCCESS;
    } else {
        ret = -ENETDOWN;
    }

#ifdef MOTO_DBG
    PRINTM(MOTO_DEBUG, "SIOCSIWAP (wlan_set_wap) ioctl end\n");
#endif

    LEAVE();
    return ret;
}

/**
 *  @brief Associated to a specific BSS discovered in a scan
 *
 *  @param priv      A pointer to wlan_private structure
 *  @param pBSSDesc  Pointer to the BSS descriptor to associate with.
 *
 *  @return          WLAN_STATUS_SUCCESS-success, otherwise fail
 */
int
wlan_associate(wlan_private * priv, BSSDescriptor_t * pBSSDesc)
{
    int ret;

    /* Clear any past association response stored for application retrieval */
    priv->adapter->assocRspSize = 0;

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_AUTHENTICATE,
                                0, HostCmd_OPTION_WAITFORRSP,
                                0, pBSSDesc->MacAddress);

    if (ret) {
        LEAVE();
        return ret;
    }

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_ASSOCIATE,
                                0, HostCmd_OPTION_WAITFORRSP, 0, pBSSDesc);

    LEAVE();
    return ret;
}

/**
 *  @brief Start an Adhoc Network
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param AdhocSSID    The ssid of the Adhoc Network
 *  @return             WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
int
StartAdhocNetwork(wlan_private * priv, WLAN_802_11_SSID * AdhocSSID)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    Adapter->AdhocCreate = TRUE;

    if (!Adapter->capInfo.ShortPreamble) {
        PRINTM(INFO, "AdhocStart: Long Preamble\n");
        Adapter->Preamble = HostCmd_TYPE_LONG_PREAMBLE;
    } else {
        PRINTM(INFO, "AdhocStart: Short Preamble\n");
        Adapter->Preamble = HostCmd_TYPE_SHORT_PREAMBLE;
    }

    SetRadioControl(priv);

    PRINTM(INFO, "Adhoc Channel = %d\n", Adapter->AdhocChannel);
    PRINTM(INFO, "CurBssParams.channel = %d\n",
           Adapter->CurBssParams.channel);
    PRINTM(INFO, "CurBssParams.band = %d\n", Adapter->CurBssParams.band);

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_AD_HOC_START,
                                0, HostCmd_OPTION_WAITFORRSP, 0, AdhocSSID);

    LEAVE();
    return ret;
}

/**
 *  @brief Join an adhoc network found in a previous scan
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param pBSSDesc     Pointer to a BSS descriptor found in a previous scan
 *                      to attempt to join
 *
 *  @return             WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
int
JoinAdhocNetwork(wlan_private * priv, BSSDescriptor_t * pBSSDesc)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    PRINTM(INFO, "JoinAdhocNetwork: CurBss.ssid =%s\n",
           Adapter->CurBssParams.ssid.Ssid);
    PRINTM(INFO, "JoinAdhocNetwork: CurBss.ssid_len =%u\n",
           Adapter->CurBssParams.ssid.SsidLength);
    PRINTM(INFO, "JoinAdhocNetwork: ssid =%s\n", pBSSDesc->Ssid.Ssid);
    PRINTM(INFO, "JoinAdhocNetwork: ssid len =%u\n",
           pBSSDesc->Ssid.SsidLength);

    /* check if the requested SSID is already joined */
    if (Adapter->CurBssParams.ssid.SsidLength
        && !SSIDcmp(&pBSSDesc->Ssid, &Adapter->CurBssParams.ssid)
        && (Adapter->CurBssParams.BSSDescriptor.InfrastructureMode ==
            Wlan802_11IBSS)) {

        PRINTM(INFO,
               "ADHOC_J_CMD: New ad-hoc SSID is the same as current, "
               "not attempting to re-join");

        return WLAN_STATUS_FAILURE;
    }

    /*Use ShortPreamble only when both creator and card supports
       short preamble */
    if (!pBSSDesc->Cap.ShortPreamble || !Adapter->capInfo.ShortPreamble) {
        PRINTM(INFO, "AdhocJoin: Long Preamble\n");
        Adapter->Preamble = HostCmd_TYPE_LONG_PREAMBLE;
    } else {
        PRINTM(INFO, "AdhocJoin: Short Preamble\n");
        Adapter->Preamble = HostCmd_TYPE_SHORT_PREAMBLE;
    }

    SetRadioControl(priv);

    PRINTM(INFO, "CurBssParams.channel = %d\n",
           Adapter->CurBssParams.channel);
    PRINTM(INFO, "CurBssParams.band = %c\n", Adapter->CurBssParams.band);

    Adapter->AdhocCreate = FALSE;

    // store the SSID info temporarily
    memset(&Adapter->AttemptedSSIDBeforeScan, 0, sizeof(WLAN_802_11_SSID));
    memcpy(&Adapter->AttemptedSSIDBeforeScan,
           &pBSSDesc->Ssid, sizeof(WLAN_802_11_SSID));

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_AD_HOC_JOIN,
                                0, HostCmd_OPTION_WAITFORRSP, 0, pBSSDesc);

    LEAVE();
    return ret;
}

/**
 *  @brief Stop the Adhoc Network
 *
 *  @param priv      A pointer to wlan_private structure
 *  @return          WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
int
StopAdhocNetwork(wlan_private * priv)
{
    return PrepareAndSendCommand(priv, HostCmd_CMD_802_11_AD_HOC_STOP,
                                 0, HostCmd_OPTION_WAITFORRSP, 0, NULL);
}

/**
 *  @brief Send Deauthentication Request
 *
 *  @param priv      A pointer to wlan_private structure
 *  @return          WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
int
SendDeauthentication(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    /* If a reassociation attempt is in progress, do not send the deauth */
    if (Adapter->reassocAttempt) {
        return WLAN_STATUS_SUCCESS;
    }
    return PrepareAndSendCommand(priv, HostCmd_CMD_802_11_DEAUTHENTICATE,
                                 0, HostCmd_OPTION_WAITFORRSP, 0, NULL);
}

/**
 *  @brief Set Idle Off
 *
 *  @param priv         A pointer to wlan_private structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlanidle_off(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    const u8 zeroMac[] = { 0, 0, 0, 0, 0, 0 };
    int i;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
            if (memcmp(Adapter->PreviousBSSID, zeroMac, sizeof(zeroMac)) != 0) {

                PRINTM(INFO, "Previous SSID = %s\n",
                       Adapter->PreviousSSID.Ssid);
                PRINTM(INFO, "Previous BSSID = "
                       "%02x:%02x:%02x:%02x:%02x:%02x:\n",
                       Adapter->PreviousBSSID[0], Adapter->PreviousBSSID[1],
                       Adapter->PreviousBSSID[2], Adapter->PreviousBSSID[3],
                       Adapter->PreviousBSSID[4], Adapter->PreviousBSSID[5]);

                i = FindSSIDInList(Adapter,
                                   &Adapter->PreviousSSID,
                                   Adapter->PreviousBSSID,
                                   Adapter->InfrastructureMode);

                if (i < 0) {
                    SendSpecificBSSIDScan(priv, Adapter->PreviousBSSID);
                    i = FindSSIDInList(Adapter,
                                       &Adapter->PreviousSSID,
                                       Adapter->PreviousBSSID,
                                       Adapter->InfrastructureMode);
                }

                if (i < 0) {
                    /* If the BSSID could not be found, try just the SSID */
                    i = FindSSIDInList(Adapter,
                                       &Adapter->PreviousSSID,
                                       NULL, Adapter->InfrastructureMode);
                }

                if (i < 0) {
                    SendSpecificSSIDScan(priv, &Adapter->PreviousSSID);
                    i = FindSSIDInList(Adapter,
                                       &Adapter->PreviousSSID,
                                       NULL, Adapter->InfrastructureMode);
                }

                if (i >= 0) {
                    ret = wlan_associate(priv, &Adapter->ScanTable[i]);
                }
            }
        } else if (Adapter->InfrastructureMode == Wlan802_11IBSS) {
            ret = PrepareAndSendCommand(priv,
                                        HostCmd_CMD_802_11_AD_HOC_START,
                                        0, HostCmd_OPTION_WAITFORRSP,
                                        0, &Adapter->PreviousSSID);
        }
    }
    /* else it is connected */

    PRINTM(INFO, "\nwlanidle is off");
    LEAVE();
    return ret;
}

/**
 *  @brief Set Idle On
 *
 *  @param priv         A pointer to wlan_private structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlanidle_on(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
            PRINTM(INFO, "Previous SSID = %s\n", Adapter->PreviousSSID.Ssid);
            memmove(&Adapter->PreviousSSID,
                    &Adapter->CurBssParams.ssid, sizeof(WLAN_802_11_SSID));
            wlan_send_deauth(priv);

        } else if (Adapter->InfrastructureMode == Wlan802_11IBSS) {
            ret = StopAdhocNetwork(priv);
        }

    }
#ifdef REASSOCIATION
    if (Adapter->TimerIsSet == TRUE) {
        CancelTimer(&Adapter->MrvDrvTimer);
        Adapter->TimerIsSet = FALSE;
    }
#endif /* REASSOCIATION */

    PRINTM(INFO, "\nwlanidle is on");

    LEAVE();
    return ret;
}

/**
 *  @brief Append a generic IE as a passthrough TLV to a TLV buffer.
 *
 *  It is called from the network join command prep. routine. If a generic
 *    IE buffer has been setup by the application/supplication, the routine
 *    appends the buffer as a passthrough TLV type to the request.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param ppBuffer pointer to command buffer pointer
 *  @return         bytes added to the buffer
 */
int
wlan_cmd_append_generic_ie(wlan_private * priv, u8 ** ppBuffer)
{
    wlan_adapter *Adapter = priv->adapter;
    int retLen = 0;
    MrvlIEtypesHeader_t ieHeader;

    /* Null Checks */
    if (ppBuffer == 0)
        return 0;
    if (*ppBuffer == 0)
        return 0;

    /*
     * If there is a generic ie buffer setup, append it to the return
     *   parameter buffer pointer.
     */
    if (Adapter->genIeBufferLen) {
        PRINTM(INFO, "append generic %d to %p\n", Adapter->genIeBufferLen,
               *ppBuffer);

        /* Wrap the generic IE buffer with a passthrough TLV type */
        ieHeader.Type = wlan_cpu_to_le16(TLV_TYPE_PASSTHROUGH);
        ieHeader.Len = wlan_cpu_to_le16(Adapter->genIeBufferLen);
        memcpy(*ppBuffer, &ieHeader, sizeof(ieHeader));

        /* Increment the return size and the return buffer pointer param */
        *ppBuffer += sizeof(ieHeader);
        retLen += sizeof(ieHeader);

        /* Copy the generic IE buffer to the output buffer, advance pointer */
        memcpy(*ppBuffer, Adapter->genIeBuffer, Adapter->genIeBufferLen);

        /* Increment the return size and the return buffer pointer param */
        *ppBuffer += Adapter->genIeBufferLen;
        retLen += Adapter->genIeBufferLen;

        /* Reset the generic IE buffer */
        Adapter->genIeBufferLen = 0;
    }

    /* return the length appended to the buffer */
    return retLen;
}

/**
 *  @brief Append any application provided Marvell TLVs to a TLV buffer.
 *
 *  It is called from the network join command prep. routine. If the Marvell
 *    TLV buffer has been setup by the application/supplication, the routine
 *    appends the buffer to the request.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param ppBuffer pointer to command buffer pointer
 *  @return         bytes added to the buffer
 */
int
wlan_cmd_append_marvell_tlv(wlan_private * priv, u8 ** ppBuffer)
{
    wlan_adapter *Adapter = priv->adapter;
    int retLen = 0;

    /* Null Checks */
    if (ppBuffer == 0)
        return 0;
    if (*ppBuffer == 0)
        return 0;

    /*
     * If there is a Marvell TLV buffer setup, append it to the return
     *   parameter buffer pointer.
     */
    if (Adapter->mrvlTlvBufferLen) {
        PRINTM(INFO, "append tlv %d to %p\n", Adapter->mrvlTlvBufferLen,
               *ppBuffer);

        /* Copy the TLV buffer to the output buffer, advance pointer */
        memcpy(*ppBuffer, Adapter->mrvlTlvBuffer, Adapter->mrvlTlvBufferLen);

        /* Increment the return size and the return buffer pointer param */
        *ppBuffer += Adapter->mrvlTlvBufferLen;
        retLen += Adapter->mrvlTlvBufferLen;

        /* Reset the Marvell TLV buffer */
        Adapter->mrvlTlvBufferLen = 0;
    }

    /* return the length appended to the buffer */
    return retLen;
}

/**
 *  @brief Append the reassociation TLV to the TLV buffer if appropriate.
 *
 *  It is called from the network join command prep. routine.
 *    If a reassociation attempt is in progress (determined from flag in
 *    the wlan_priv structure), a REASSOCAP TLV is added to the association
 *    request.
 *
 *  This causes the firmware to send a reassociation request instead of an
 *    association request.  The wlan_priv structure also contains the current
 *    AP BSSID to be passed in the TLV and eventually in the management
 *    frame to the new AP.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param ppBuffer pointer to command buffer pointer
 *  @return         bytes added to the buffer
 */
int
wlan_cmd_append_reassoc_tlv(wlan_private * priv, u8 ** ppBuffer)
{
    wlan_adapter *Adapter = priv->adapter;
    int retLen = 0;
    MrvlIEtypes_ReassocAp_t reassocIe;

    /* Null Checks */
    if (ppBuffer == 0)
        return 0;
    if (*ppBuffer == 0)
        return 0;

    /*
     * If the reassocAttempt flag is set in the adapter structure, include
     *   the appropriate TLV in the association buffer pointed to by ppBuffer
     */
    if (Adapter->reassocAttempt) {
        PRINTM(INFO, "Reassoc: append current AP: %#x:%#x:%#x:%#x:%#x:%#x\n",
               Adapter->reassocCurrentAp[0], Adapter->reassocCurrentAp[1],
               Adapter->reassocCurrentAp[2], Adapter->reassocCurrentAp[3],
               Adapter->reassocCurrentAp[4], Adapter->reassocCurrentAp[5]);

        /* Setup a copy of the reassocIe on the stack */
        reassocIe.Header.Type = wlan_cpu_to_le16(TLV_TYPE_REASSOCAP);
        reassocIe.Header.Len
            = wlan_cpu_to_le16(sizeof(MrvlIEtypes_ReassocAp_t)
                               - sizeof(MrvlIEtypesHeader_t));

        memcpy(&reassocIe.currentAp,
               &Adapter->reassocCurrentAp, sizeof(reassocIe.currentAp));

        /* Copy the stack reassocIe to the buffer pointer parameter */
        memcpy(*ppBuffer, &reassocIe, sizeof(reassocIe));

        /* Set the return length */
        retLen = sizeof(reassocIe);

        /* Advance passed buffer pointer */
        *ppBuffer += sizeof(reassocIe);

        /* Reset the reassocAttempt flag, only valid for a single attempt */
        Adapter->reassocAttempt = FALSE;

        /* Reset the reassociation AP address */
        memset(&Adapter->reassocCurrentAp,
               0x00, sizeof(Adapter->reassocCurrentAp));
    }

    /* return the length appended to the buffer */
    return retLen;
}

/**
 *  @brief This function prepares command of authenticate.
 *
 *  @param priv      A pointer to wlan_private structure
 *  @param cmd       A pointer to HostCmd_DS_COMMAND structure
 *  @param pdata_buf Void cast of pointer to a BSSID to authenticate with
 *
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_authenticate(wlan_private * priv,
                             HostCmd_DS_COMMAND * cmd, void *pdata_buf)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_AUTHENTICATE *pAuthenticate = &cmd->params.auth;
    u8 *bssid = (u8 *) pdata_buf;

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_AUTHENTICATE);
    cmd->Size = wlan_cpu_to_le16(sizeof(HostCmd_DS_802_11_AUTHENTICATE)
                                 + S_DS_GEN);

    pAuthenticate->AuthType = Adapter->SecInfo.AuthenticationMode;
    memcpy(pAuthenticate->MacAddr, bssid, MRVDRV_ETH_ADDR_LEN);

    PRINTM(INFO, "AUTH_CMD: Bssid is : %x:%x:%x:%x:%x:%x\n",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function prepares command of deauthenticat.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param cmd      A pointer to HostCmd_DS_COMMAND structure
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_deauthenticate(wlan_private * priv, HostCmd_DS_COMMAND * cmd)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_DEAUTHENTICATE *dauth = &cmd->params.deauth;

    ENTER();

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_DEAUTHENTICATE);
    cmd->Size =
        wlan_cpu_to_le16(sizeof(HostCmd_DS_802_11_DEAUTHENTICATE) + S_DS_GEN);

    /* set AP MAC address */
    memmove(dauth->MacAddr, Adapter->CurBssParams.bssid, MRVDRV_ETH_ADDR_LEN);

    /* Reason code 3 = Station is leaving */
#define REASON_CODE_STA_LEAVING 3
    dauth->ReasonCode = wlan_cpu_to_le16(REASON_CODE_STA_LEAVING);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function prepares command of association.
 *
 *  @param priv      A pointer to wlan_private structure
 *  @param cmd       A pointer to HostCmd_DS_COMMAND structure
 *  @param pdata_buf Void cast of BSSDescriptor_t from the scan table to assoc
 *  @return          WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_associate(wlan_private * priv,
                          HostCmd_DS_COMMAND * cmd, void *pdata_buf)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_ASSOCIATE *pAsso = &cmd->params.associate;
    int ret = WLAN_STATUS_SUCCESS;
    BSSDescriptor_t *pBSSDesc;
    u8 *card_rates;
    u8 *pos;
    int card_rates_size;
    u16 TmpCap;
    MrvlIEtypes_SsIdParamSet_t *ssid;
    MrvlIEtypes_PhyParamSet_t *phy;
    MrvlIEtypes_SsParamSet_t *ss;
    MrvlIEtypes_RatesParamSet_t *rates;
    MrvlIEtypes_RsnParamSet_t *rsn;

    ENTER();

    pBSSDesc = (BSSDescriptor_t *) pdata_buf;
    pos = (u8 *) pAsso;

    if (!Adapter) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_ASSOCIATE);

    /* Save so we know which BSS Desc to use in the response handler */
    Adapter->pAttemptedBSSDesc = pBSSDesc;

    memcpy(pAsso->PeerStaAddr,
           pBSSDesc->MacAddress, sizeof(pAsso->PeerStaAddr));
    pos += sizeof(pAsso->PeerStaAddr);

    /* set preamble to firmware */
    if (Adapter->capInfo.ShortPreamble && pBSSDesc->Cap.ShortPreamble) {
        Adapter->Preamble = HostCmd_TYPE_SHORT_PREAMBLE;
    } else {
        Adapter->Preamble = HostCmd_TYPE_LONG_PREAMBLE;
    }

    SetRadioControl(priv);

    /* set the listen interval */
    pAsso->ListenInterval = wlan_cpu_to_le16(Adapter->ListenInterval);

    pos += sizeof(pAsso->CapInfo);
    pos += sizeof(pAsso->ListenInterval);
    pos += sizeof(pAsso->BcnPeriod);
    pos += sizeof(pAsso->DtimPeriod);

    ssid = (MrvlIEtypes_SsIdParamSet_t *) pos;
    ssid->Header.Type = wlan_cpu_to_le16(TLV_TYPE_SSID);
    ssid->Header.Len = pBSSDesc->Ssid.SsidLength;
    memcpy(ssid->SsId, pBSSDesc->Ssid.Ssid, ssid->Header.Len);
    pos += sizeof(ssid->Header) + ssid->Header.Len;
    ssid->Header.Len = wlan_cpu_to_le16(ssid->Header.Len);

    phy = (MrvlIEtypes_PhyParamSet_t *) pos;
    phy->Header.Type = wlan_cpu_to_le16(TLV_TYPE_PHY_DS);
    phy->Header.Len = sizeof(phy->fh_ds.DsParamSet);
    memcpy(&phy->fh_ds.DsParamSet,
           &pBSSDesc->PhyParamSet.DsParamSet.CurrentChan,
           sizeof(phy->fh_ds.DsParamSet));
    pos += sizeof(phy->Header) + phy->Header.Len;
    phy->Header.Len = wlan_cpu_to_le16(phy->Header.Len);

    ss = (MrvlIEtypes_SsParamSet_t *) pos;
    ss->Header.Type = wlan_cpu_to_le16(TLV_TYPE_CF);
    ss->Header.Len = sizeof(ss->cf_ibss.CfParamSet);
    pos += sizeof(ss->Header) + ss->Header.Len;
    ss->Header.Len = wlan_cpu_to_le16(ss->Header.Len);

    rates = (MrvlIEtypes_RatesParamSet_t *) pos;
    rates->Header.Type = wlan_cpu_to_le16(TLV_TYPE_RATES);

    memcpy(&rates->Rates, &pBSSDesc->SupportedRates, WLAN_SUPPORTED_RATES);

    card_rates = SupportedRates;
    card_rates_size = sizeof(SupportedRates);

    if (get_common_rates(Adapter, rates->Rates, WLAN_SUPPORTED_RATES,
                         card_rates, card_rates_size)) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    rates->Header.Len = MIN(strlen(rates->Rates), WLAN_SUPPORTED_RATES);
    Adapter->CurBssParams.NumOfRates = rates->Header.Len;

    pos += sizeof(rates->Header) + rates->Header.Len;
    rates->Header.Len = wlan_cpu_to_le16(rates->Header.Len);

    if (Adapter->SecInfo.WPAEnabled || Adapter->SecInfo.WPA2Enabled) {
        rsn = (MrvlIEtypes_RsnParamSet_t *) pos;
        rsn->Header.Type = (u16) Adapter->Wpa_ie[0];    /* WPA_IE or WPA2_IE */
        rsn->Header.Type = rsn->Header.Type & 0x00FF;
        rsn->Header.Type = wlan_cpu_to_le16(rsn->Header.Type);
        rsn->Header.Len = (u16) Adapter->Wpa_ie[1];
        rsn->Header.Len = rsn->Header.Len & 0x00FF;
        if (rsn->Header.Len <= (sizeof(Adapter->Wpa_ie) - 2)) {
            memcpy(rsn->RsnIE, &Adapter->Wpa_ie[2], rsn->Header.Len);
        } else {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
        HEXDUMP("ASSOC_CMD: RSN IE", (u8 *) rsn,
                sizeof(rsn->Header) + rsn->Header.Len);
        pos += sizeof(rsn->Header) + rsn->Header.Len;
        rsn->Header.Len = wlan_cpu_to_le16(rsn->Header.Len);
    }

    wlan_wmm_process_association_req(priv, &pos, &pBSSDesc->wmmIE);

    wlan_cmd_append_reassoc_tlv(priv, &pos);

    wlan_cmd_append_generic_ie(priv, &pos);

    wlan_cmd_append_marvell_tlv(priv, &pos);

    /* update CurBssParams */
    Adapter->CurBssParams.channel =
        (pBSSDesc->PhyParamSet.DsParamSet.CurrentChan);

    /* Copy the infra. association rates into Current BSS state structure */
    memcpy(&Adapter->CurBssParams.DataRates, &rates->Rates,
           MIN(sizeof(Adapter->CurBssParams.DataRates),
               wlan_le16_to_cpu(rates->Header.Len)));

    PRINTM(INFO, "ASSOC_CMD: rates->Header.Len = %d\n",
           wlan_le16_to_cpu(rates->Header.Len));

    /* set IBSS field */
    if (pBSSDesc->InfrastructureMode == Wlan802_11Infrastructure) {
#define CAPINFO_ESS_MODE 1
        pAsso->CapInfo.Ess = CAPINFO_ESS_MODE;
    }

    if (wlan_create_dnld_countryinfo_11d(priv, 0)) {
        PRINTM(INFO, "Dnld_countryinfo_11d failed\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    if (wlan_parse_dnld_countryinfo_11d(priv)) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    cmd->Size = wlan_cpu_to_le16((u16) (pos - (u8 *) pAsso) + S_DS_GEN);

    /* set the Capability info at last */
    memcpy(&TmpCap, &pBSSDesc->Cap, sizeof(pAsso->CapInfo));
    TmpCap &= CAPINFO_MASK;
    PRINTM(INFO, "ASSOC_CMD: TmpCap=%4X CAPINFO_MASK=%4X\n",
           TmpCap, CAPINFO_MASK);
    TmpCap = wlan_cpu_to_le16(TmpCap);
    memcpy(&pAsso->CapInfo, &TmpCap, sizeof(pAsso->CapInfo));

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function prepares command of ad_hoc_start.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param cmd      A pointer to HostCmd_DS_COMMAND structure
 *  @param pssid    A pointer to WLAN_802_11_SSID structure
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_ad_hoc_start(wlan_private * priv,
                             HostCmd_DS_COMMAND * cmd, void *pssid)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_AD_HOC_START *adhs = &cmd->params.ads;
    int ret = WLAN_STATUS_SUCCESS;
    int cmdAppendSize = 0;
    int i;
    u16 TmpCap;
    BSSDescriptor_t *pBSSDesc;

    ENTER();

    if (!Adapter) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_AD_HOC_START);

    pBSSDesc = &Adapter->CurBssParams.BSSDescriptor;
    Adapter->pAttemptedBSSDesc = pBSSDesc;

    /*
     * Fill in the parameters for 2 data structures:
     *   1. HostCmd_DS_802_11_AD_HOC_START Command
     *   2. Adapter->ScanTable[i]
     *
     * Driver will fill up SSID, BSSType,IBSS param, Physical Param,
     *   probe delay, and Cap info.
     *
     * Firmware will fill up beacon period, DTIM, Basic rates
     *   and operational rates.
     */

    memset(adhs->SSID, 0, MRVDRV_MAX_SSID_LENGTH);

    memcpy(adhs->SSID, ((PWLAN_802_11_SSID) pssid)->Ssid,
           ((PWLAN_802_11_SSID) pssid)->SsidLength);

    PRINTM(INFO, "ADHOC_S_CMD: SSID = %s\n", adhs->SSID);

    memset(pBSSDesc->Ssid.Ssid, 0, MRVDRV_MAX_SSID_LENGTH);
    memcpy(pBSSDesc->Ssid.Ssid,
           ((PWLAN_802_11_SSID) pssid)->Ssid,
           ((PWLAN_802_11_SSID) pssid)->SsidLength);

    pBSSDesc->Ssid.SsidLength = ((PWLAN_802_11_SSID) pssid)->SsidLength;

    /* set the BSS type */
    adhs->BSSType = HostCmd_BSS_TYPE_IBSS;
    pBSSDesc->InfrastructureMode = Wlan802_11IBSS;
    adhs->BeaconPeriod = Adapter->BeaconPeriod;
    pBSSDesc->BeaconPeriod = Adapter->BeaconPeriod;

    /* set Physical param set */
#define DS_PARA_IE_ID   3
#define DS_PARA_IE_LEN  1

    adhs->PhyParamSet.DsParamSet.ElementId = DS_PARA_IE_ID;
    adhs->PhyParamSet.DsParamSet.Len = DS_PARA_IE_LEN;

    ASSERT(Adapter->AdhocChannel);

    PRINTM(INFO, "ADHOC_S_CMD: Creating ADHOC on Channel %d\n",
           Adapter->AdhocChannel);

    Adapter->CurBssParams.channel = Adapter->AdhocChannel;

    pBSSDesc->Channel = Adapter->AdhocChannel;
    adhs->PhyParamSet.DsParamSet.CurrentChan = Adapter->AdhocChannel;

    memcpy(&pBSSDesc->PhyParamSet,
           &adhs->PhyParamSet, sizeof(IEEEtypes_PhyParamSet_t));

    pBSSDesc->NetworkTypeInUse = Wlan802_11DS;

    /* set IBSS param set */
#define IBSS_PARA_IE_ID   6
#define IBSS_PARA_IE_LEN  2

    adhs->SsParamSet.IbssParamSet.ElementId = IBSS_PARA_IE_ID;
    adhs->SsParamSet.IbssParamSet.Len = IBSS_PARA_IE_LEN;
    adhs->SsParamSet.IbssParamSet.AtimWindow = Adapter->AtimWindow;
    pBSSDesc->ATIMWindow = Adapter->AtimWindow;
    memcpy(&pBSSDesc->SsParamSet,
           &adhs->SsParamSet, sizeof(IEEEtypes_SsParamSet_t));

    /* set Capability info */
    adhs->Cap.Ess = 0;
    adhs->Cap.Ibss = 1;
    pBSSDesc->Cap.Ibss = 1;

    /* set up privacy in Adapter->ScanTable[i] */
    if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled
        || Adapter->AdhocAESEnabled) {

#define AD_HOC_CAP_PRIVACY_ON 1
        PRINTM(INFO, "ADHOC_S_CMD: WEPStatus set, Privacy to WEP\n");
        pBSSDesc->Privacy = Wlan802_11PrivFilter8021xWEP;
        adhs->Cap.Privacy = AD_HOC_CAP_PRIVACY_ON;
    } else {
        PRINTM(INFO, "ADHOC_S_CMD: WEPStatus NOT set, Setting "
               "Privacy to ACCEPT ALL\n");
        pBSSDesc->Privacy = Wlan802_11PrivFilterAcceptAll;
    }

    memset(adhs->DataRate, 0, sizeof(adhs->DataRate));

    if (Adapter->adhoc_grate_enabled == TRUE) {
        memcpy(adhs->DataRate, AdhocRates_G,
               MIN(sizeof(adhs->DataRate), sizeof(AdhocRates_G)));
    } else {
        memcpy(adhs->DataRate, AdhocRates_B,
               MIN(sizeof(adhs->DataRate), sizeof(AdhocRates_B)));
    }

    /* Find the last non zero */
    for (i = 0; i < sizeof(adhs->DataRate) && adhs->DataRate[i]; i++);

    Adapter->CurBssParams.NumOfRates = i;

    /* Copy the ad-hoc creating rates into Current BSS state structure */
    memcpy(&Adapter->CurBssParams.DataRates,
           &adhs->DataRate, Adapter->CurBssParams.NumOfRates);

    PRINTM(INFO, "ADHOC_S_CMD: Rates=%02x %02x %02x %02x \n",
           adhs->DataRate[0], adhs->DataRate[1],
           adhs->DataRate[2], adhs->DataRate[3]);

    PRINTM(INFO, "ADHOC_S_CMD: AD HOC Start command is ready\n");

    if (wlan_create_dnld_countryinfo_11d(priv, Adapter->CurBssParams.band)) {
        PRINTM(INFO, "ADHOC_S_CMD: dnld_countryinfo_11d failed\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    cmd->Size = wlan_cpu_to_le16(sizeof(HostCmd_DS_802_11_AD_HOC_START)
                                 + S_DS_GEN + cmdAppendSize);

    adhs->BeaconPeriod = wlan_cpu_to_le16(adhs->BeaconPeriod);
    adhs->SsParamSet.IbssParamSet.AtimWindow =
        wlan_cpu_to_le16(adhs->SsParamSet.IbssParamSet.AtimWindow);

    memcpy(&TmpCap, &adhs->Cap, sizeof(u16));
    TmpCap = wlan_cpu_to_le16(TmpCap);
    memcpy(&adhs->Cap, &TmpCap, sizeof(u16));

    ret = WLAN_STATUS_SUCCESS;
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function prepares command of ad_hoc_stop.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param cmd      A pointer to HostCmd_DS_COMMAND structure
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_ad_hoc_stop(wlan_private * priv, HostCmd_DS_COMMAND * cmd)
{
    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_AD_HOC_STOP);
    cmd->Size = wlan_cpu_to_le16(sizeof(HostCmd_DS_802_11_AD_HOC_STOP)
                                 + S_DS_GEN);

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function prepares command of ad_hoc_join.
 *
 *  @param priv      A pointer to wlan_private structure
 *  @param cmd       A pointer to HostCmd_DS_COMMAND structure
 *  @param pdata_buf Void cast of BSSDescriptor_t from the scan table to join
 *
 *  @return          WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_ad_hoc_join(wlan_private * priv,
                            HostCmd_DS_COMMAND * cmd, void *pdata_buf)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_AD_HOC_JOIN *pAdHocJoin = &cmd->params.adj;
    BSSDescriptor_t *pBSSDesc = (BSSDescriptor_t *) pdata_buf;
    int cmdAppendSize = 0;
    int ret = WLAN_STATUS_SUCCESS;
    u8 *card_rates;
    int card_rates_size;
    u16 TmpCap;
    int i;

    ENTER();

    Adapter->pAttemptedBSSDesc = pBSSDesc;

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_AD_HOC_JOIN);

    pAdHocJoin->BssDescriptor.BSSType = HostCmd_BSS_TYPE_IBSS;

    pAdHocJoin->BssDescriptor.BeaconPeriod = pBSSDesc->BeaconPeriod;

    memcpy(&pAdHocJoin->BssDescriptor.BSSID,
           &pBSSDesc->MacAddress, MRVDRV_ETH_ADDR_LEN);

    memcpy(&pAdHocJoin->BssDescriptor.SSID,
           &pBSSDesc->Ssid.Ssid, pBSSDesc->Ssid.SsidLength);

    memcpy(&pAdHocJoin->BssDescriptor.PhyParamSet,
           &pBSSDesc->PhyParamSet, sizeof(IEEEtypes_PhyParamSet_t));

    memcpy(&pAdHocJoin->BssDescriptor.SsParamSet,
           &pBSSDesc->SsParamSet, sizeof(IEEEtypes_SsParamSet_t));

    memcpy(&TmpCap, &pBSSDesc->Cap, sizeof(IEEEtypes_CapInfo_t));

    TmpCap &= CAPINFO_MASK;

    PRINTM(INFO, "ADHOC_J_CMD: TmpCap=%4X CAPINFO_MASK=%4X\n",
           TmpCap, CAPINFO_MASK);
    memcpy(&pAdHocJoin->BssDescriptor.Cap, &TmpCap,
           sizeof(IEEEtypes_CapInfo_t));

    /* information on BSSID descriptor passed to FW */
    PRINTM(INFO, "ADHOC_J_CMD: BSSID = %2x-%2x-%2x-%2x-%2x-%2x, SSID = %s\n",
           pAdHocJoin->BssDescriptor.BSSID[0],
           pAdHocJoin->BssDescriptor.BSSID[1],
           pAdHocJoin->BssDescriptor.BSSID[2],
           pAdHocJoin->BssDescriptor.BSSID[3],
           pAdHocJoin->BssDescriptor.BSSID[4],
           pAdHocJoin->BssDescriptor.BSSID[5],
           pAdHocJoin->BssDescriptor.SSID);

    PRINTM(INFO, "ADHOC_J_CMD: Data Rate = %x\n",
           (u32) pAdHocJoin->BssDescriptor.DataRates);

    /* Copy Data Rates from the Rates recorded in scan response */
    memset(pAdHocJoin->BssDescriptor.DataRates, 0,
           sizeof(pAdHocJoin->BssDescriptor.DataRates));
    memcpy(pAdHocJoin->BssDescriptor.DataRates, pBSSDesc->DataRates,
           MIN(sizeof(pAdHocJoin->BssDescriptor.DataRates),
               sizeof(pBSSDesc->DataRates)));

    card_rates = SupportedRates;
    card_rates_size = sizeof(SupportedRates);

    Adapter->CurBssParams.channel = pBSSDesc->Channel;

    if (get_common_rates(Adapter, pAdHocJoin->BssDescriptor.DataRates,
                         sizeof(pAdHocJoin->BssDescriptor.DataRates),
                         card_rates, card_rates_size)) {
        PRINTM(INFO, "ADHOC_J_CMD: get_common_rates returns error.\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* Find the last non zero */
    for (i = 0; i < sizeof(pAdHocJoin->BssDescriptor.DataRates)
         && pAdHocJoin->BssDescriptor.DataRates[i]; i++);

    Adapter->CurBssParams.NumOfRates = i;

    /*
     * Copy the adhoc joining rates to Current BSS State structure
     */
    memcpy(Adapter->CurBssParams.DataRates,
           pAdHocJoin->BssDescriptor.DataRates,
           Adapter->CurBssParams.NumOfRates);

    pAdHocJoin->BssDescriptor.SsParamSet.IbssParamSet.AtimWindow =
        wlan_cpu_to_le16(pBSSDesc->ATIMWindow);

    if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled
        || Adapter->AdhocAESEnabled) {
        pAdHocJoin->BssDescriptor.Cap.Privacy = AD_HOC_CAP_PRIVACY_ON;
    }

    if (Adapter->PSMode == Wlan802_11PowerModeMAX_PSP) {
        /* wake up first */
        WLAN_802_11_POWER_MODE LocalPSMode;

        LocalPSMode = Wlan802_11PowerModeCAM;
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_PS_MODE,
                                    HostCmd_ACT_GEN_SET, 0, 0, &LocalPSMode);

        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
    }

    if (wlan_create_dnld_countryinfo_11d(priv, 0)) {
        PRINTM(INFO, "Dnld_countryinfo_11d failed\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    if (wlan_parse_dnld_countryinfo_11d(priv)) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    cmd->Size = wlan_cpu_to_le16(sizeof(HostCmd_DS_802_11_AD_HOC_JOIN)
                                 + S_DS_GEN + cmdAppendSize);

    pAdHocJoin->BssDescriptor.BeaconPeriod =
        wlan_cpu_to_le16(pAdHocJoin->BssDescriptor.BeaconPeriod);
    pAdHocJoin->BssDescriptor.SsParamSet.IbssParamSet.AtimWindow =
        wlan_cpu_to_le16(pAdHocJoin->BssDescriptor.SsParamSet.IbssParamSet.
                         AtimWindow);

    memcpy(&TmpCap, &pAdHocJoin->BssDescriptor.Cap,
           sizeof(IEEEtypes_CapInfo_t));
    TmpCap = wlan_cpu_to_le16(TmpCap);

    memcpy(&pAdHocJoin->BssDescriptor.Cap,
           &TmpCap, sizeof(IEEEtypes_CapInfo_t));

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function handles the command response of authenticate
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param resp    A pointer to HostCmd_DS_COMMAND
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_ret_802_11_authenticate(wlan_private * priv, HostCmd_DS_COMMAND * resp)
{
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Association firmware command response handler
 *
 *   The response buffer for the association command has the following
 *      memory layout.
 *
 *   For cases where an association response was not received (indicated
 *      by the CapInfo and AId field):
 *
 *     .------------------------------------------------------------.
 *     |  Header(4 * sizeof(u16)):  Standard command response hdr   |
 *     .------------------------------------------------------------.
 *     |  CapInfo/Error Return(u16):                                |
 *     |           0xFFFF(-1): Internal error                       |
 *     |           0xFFFE(-2): Authentication unhandled message     |
 *     |           0xFFFD(-3): Authentication refused               |
 *     |           0xFFFC(-4): Timeout waiting for AP response      |
 *     .------------------------------------------------------------.
 *     |  StatusCode(u16):                                          |
 *     |        If CapInfo is -1:                                   |
 *     |           An internal firmware failure prevented the       |
 *     |           command from being processed.  The StatusCode    |
 *     |           will be set to 1.                                |
 *     |                                                            |
 *     |        If CapInfo is -2:                                   |
 *     |           An authentication frame was received but was     |
 *     |           not handled by the firmware.  IEEE Status        |
 *     |           code for the failure is returned.                |
 *     |                                                            |
 *     |        If CapInfo is -3:                                   |
 *     |           An authentication frame was received and the     |
 *     |           StatusCode is the IEEE Status reported in the    |
 *     |           response.                                        |
 *     |                                                            |
 *     |        If CapInfo is -4:                                   |
 *     |           (1) Association response timeout                 |
 *     |           (2) Authentication response timeout              |
 *     |           (3) Network join timeout (receive AP beacon)     |
 *     .------------------------------------------------------------.
 *     |  AId(u16): 0xFFFF                                          |
 *     .------------------------------------------------------------.
 *
 *
 *   For cases where an association response was received, the IEEE 
 *     standard association response frame is returned:
 *
 *     .------------------------------------------------------------.
 *     |  Header(4 * sizeof(u16)):  Standard command response hdr   |
 *     .------------------------------------------------------------.
 *     |  CapInfo(u16): IEEE Capability                             |
 *     .------------------------------------------------------------.
 *     |  StatusCode(u16): IEEE Status Code                         |
 *     .------------------------------------------------------------.
 *     |  AId(u16): IEEE Association ID                             |
 *     .------------------------------------------------------------.
 *     |  IEEE IEs(variable): Any received IEs comprising the       |
 *     |                      remaining portion of a received       |
 *     |                      association response frame.           |
 *     .------------------------------------------------------------.
 *
 *  For simplistic handling, the StatusCode field can be used to determine
 *    an association success (0) or failure (non-zero).
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param resp    A pointer to HostCmd_DS_COMMAND
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_ret_802_11_associate(wlan_private * priv, HostCmd_DS_COMMAND * resp)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    union iwreq_data wrqu;
    IEEEtypes_AssocRsp_t *pAssocRsp;
    BSSDescriptor_t *pBSSDesc;

    ENTER();

    pAssocRsp = (IEEEtypes_AssocRsp_t *) & resp->params;

    Adapter->assocRspSize = MIN(wlan_le16_to_cpu(resp->Size) - S_DS_GEN,
                                sizeof(Adapter->assocRspBuffer));

    memcpy(Adapter->assocRspBuffer, &resp->params, Adapter->assocRspSize);

    if (pAssocRsp->StatusCode) {

        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            MacEventDisconnected(priv);
        }

        priv->adapter->dbg.num_cmd_assoc_failure++;
#ifdef MOTO_DBG
        PRINTM(ERROR,
               "ASSOC_RESP: Association Failed, status code = %d, error = %d\n",
               pAssocRsp->StatusCode, *(short *) &pAssocRsp->Capability);
#else
        PRINTM(CMND, "ASSOC_RESP: Association Failed, "
               "status code = %d, error = %d\n",
               pAssocRsp->StatusCode, *(short *) &pAssocRsp->Capability);
#endif

        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* Send a Media Connected event, according to the Spec */
    Adapter->MediaConnectStatus = WlanMediaStateConnected;
#ifdef WPRM_DRV
    WPRM_DRV_TRACING_PRINT();
    /* Launch traffic meter */
    wprm_traffic_meter_init(priv);

    /* Reconfire host sleep config if needed */
    wprm_configure_hscfg();

#endif
    Adapter->LinkSpeed = MRVDRV_LINK_SPEED_11mbps;

    /* Set the attempted BSSID Index to current */
    pBSSDesc = Adapter->pAttemptedBSSDesc;

    PRINTM(INFO, "ASSOC_RESP: %s\n", pBSSDesc->Ssid.Ssid);

    /* Set the new SSID to current SSID */
    memcpy(&Adapter->CurBssParams.ssid,
           &pBSSDesc->Ssid, sizeof(WLAN_802_11_SSID));

    /* Set the new BSSID (AP's MAC address) to current BSSID */
    memcpy(Adapter->CurBssParams.bssid,
           pBSSDesc->MacAddress, MRVDRV_ETH_ADDR_LEN);

    /* Make a copy of current BSSID descriptor */
    memcpy(&Adapter->CurBssParams.BSSDescriptor,
           pBSSDesc, sizeof(BSSDescriptor_t));

    if (pBSSDesc->wmmIE.ElementId == WMM_IE) {
        Adapter->CurBssParams.wmm_enabled = TRUE;
    } else {
        Adapter->CurBssParams.wmm_enabled = FALSE;
    }

    if (Adapter->wmm.required && Adapter->CurBssParams.wmm_enabled)
        Adapter->wmm.enabled = TRUE;
    else
        Adapter->wmm.enabled = FALSE;

    Adapter->CurBssParams.wmm_uapsd_enabled = FALSE;

    if (Adapter->wmm.enabled == TRUE) {
        Adapter->CurBssParams.wmm_uapsd_enabled
            = pBSSDesc->wmmIE.QoSInfo.QosUAPSD;
    }

    PRINTM(INFO, "ASSOC_RESP: CurrentPacketFilter is %x\n",
           Adapter->CurrentPacketFilter);

    if (Adapter->SecInfo.WPAEnabled || Adapter->SecInfo.WPA2Enabled)
        Adapter->IsGTK_SET = FALSE;

    Adapter->SNR[TYPE_RXPD][TYPE_AVG] = 0;
    Adapter->NF[TYPE_RXPD][TYPE_AVG] = 0;

    memset(Adapter->rawSNR, 0x00, sizeof(Adapter->rawSNR));
    memset(Adapter->rawNF, 0x00, sizeof(Adapter->rawNF));
    Adapter->nextSNRNF = 0;
    Adapter->numSNRNF = 0;

    /* Don't enable carrier until we get the WMM_GET_STATUS event */
    if (Adapter->wmm.enabled == FALSE) {
        os_carrier_on(priv);
        os_start_queue(priv);
    }

    priv->adapter->dbg.num_cmd_assoc_success++;
#ifdef MOTO_DBG
    PRINTM(MOTO_MSG, "ASSOC_RESP: Associated %s wmm.enabled=%d "
           "wmm_uapsd_enabled=%d, num_cmd_assoc_success=%d\n",
           pBSSDesc->Ssid.Ssid, Adapter->wmm.enabled,
           Adapter->CurBssParams.wmm_uapsd_enabled,
           priv->adapter->dbg.num_cmd_assoc_success);
#else
    PRINTM(INFO, "ASSOC_RESP: Associated \n");
#endif

#ifdef MOTO_DBG
    PRINTM(MOTO_DEBUG,
           "Security: wep=%d, authmode=%d, encryption=%d, wpa=%d, wpa2=%d\n",
           Adapter->SecInfo.WEPStatus, Adapter->SecInfo.AuthenticationMode,
           Adapter->SecInfo.EncryptionMode, Adapter->SecInfo.WPAEnabled,
           Adapter->SecInfo.WPA2Enabled);
#endif

    memcpy(wrqu.ap_addr.sa_data, Adapter->CurBssParams.bssid, ETH_ALEN);
    wrqu.ap_addr.sa_family = ARPHRD_ETHER;
    wireless_send_event(priv->wlan_dev.netdev, SIOCGIWAP, &wrqu, NULL);
#ifdef MOTO_DBG
    PRINTM(MOTO_MSG, "ASSOC_RESP: Connected event sent to upper layer.\n");
#endif

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function handles the command response of disassociate
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param resp    A pointer to HostCmd_DS_COMMAND
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_ret_802_11_disassociate(wlan_private * priv, HostCmd_DS_COMMAND * resp)
{
    ENTER();

    priv->adapter->dbg.num_cmd_deauth++;
    MacEventDisconnected(priv);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function handles the command response of ad_hoc_start and
 *  ad_hoc_join
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param resp    A pointer to HostCmd_DS_COMMAND
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_ret_802_11_ad_hoc(wlan_private * priv, HostCmd_DS_COMMAND * resp)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    u16 Command = wlan_le16_to_cpu(resp->Command);
    u16 Result = wlan_le16_to_cpu(resp->Result);
    HostCmd_DS_802_11_AD_HOC_RESULT *pAdHocResult;
    union iwreq_data wrqu;
    BSSDescriptor_t *pBSSDesc;

    ENTER();

    pAdHocResult = &resp->params.result;

    pBSSDesc = Adapter->pAttemptedBSSDesc;

    /*
     * Join result code 0 --> SUCCESS
     */
    if (Result) {
#ifdef MOTO_DBG
        PRINTM(ERROR, "ADHOC_RESP Failed, result=%d\n", Result);
#else
        PRINTM(INFO, "ADHOC_RESP Failed\n");
#endif
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            MacEventDisconnected(priv);
        }

        memset(&Adapter->CurBssParams.BSSDescriptor,
               0x00, sizeof(Adapter->CurBssParams.BSSDescriptor));

        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    /* Send a Media Connected event, according to the Spec */
    Adapter->MediaConnectStatus = WlanMediaStateConnected;
#ifdef WPRM_DRV
    WPRM_DRV_TRACING_PRINT();
    /* Launch traffic meter */
    wprm_traffic_meter_init(priv);

    /* Reconfire host sleep config if needed */
    wprm_configure_hscfg();

#endif
    Adapter->LinkSpeed = MRVDRV_LINK_SPEED_11mbps;

    if (Command == HostCmd_RET_802_11_AD_HOC_START) {
#ifdef MOTO_DBG
        PRINTM(MOTO_DEBUG, "ADHOC_S_RESP  %s\n", pBSSDesc->Ssid.Ssid);
#else
        PRINTM(INFO, "ADHOC_S_RESP  %s\n", pBSSDesc->Ssid.Ssid);
#endif

        /* Update the created network descriptor with the new BSSID */
        memcpy(pBSSDesc->MacAddress,
               pAdHocResult->BSSID, MRVDRV_ETH_ADDR_LEN);
    } else {
        /*
         * Now the join cmd should be successful
         * If BSSID has changed use SSID to compare instead of BSSID
         */
#ifdef MOTO_DBG
        PRINTM(MOTO_DEBUG, "ADHOC_J_RESP  %s\n", pBSSDesc->Ssid.Ssid);
#else
        PRINTM(INFO, "ADHOC_J_RESP  %s\n", pBSSDesc->Ssid.Ssid);
#endif

        /* Make a copy of current BSSID descriptor, only needed for join since
         *   the current descriptor is already being used for adhoc start
         */
        memmove(&Adapter->CurBssParams.BSSDescriptor,
                pBSSDesc, sizeof(BSSDescriptor_t));
    }

    /* Set the BSSID from the joined/started descriptor */
    memcpy(&Adapter->CurBssParams.bssid,
           pBSSDesc->MacAddress, MRVDRV_ETH_ADDR_LEN);

    /* Set the new SSID to current SSID */
    memcpy(&Adapter->CurBssParams.ssid,
           &pBSSDesc->Ssid, sizeof(WLAN_802_11_SSID));

    memset(&wrqu, 0, sizeof(wrqu));
    memcpy(wrqu.ap_addr.sa_data, Adapter->CurBssParams.bssid, ETH_ALEN);
    wrqu.ap_addr.sa_family = ARPHRD_ETHER;
    wireless_send_event(priv->wlan_dev.netdev, SIOCGIWAP, &wrqu, NULL);

#ifdef MOTO_DBG
    PRINTM(MOTO_MSG, "ADHOC_RESP: Connected event sent to upper layer.\n");
    PRINTM(MOTO_MSG,
           "ADHOC_RESP: Associated Channel=%d, BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
           Adapter->AdhocChannel, pAdHocResult->BSSID[0],
           pAdHocResult->BSSID[1], pAdHocResult->BSSID[2],
           pAdHocResult->BSSID[3], pAdHocResult->BSSID[4],
           pAdHocResult->BSSID[5]);
#else
    PRINTM(INFO, "ADHOC_RESP: Channel = %d\n", Adapter->AdhocChannel);
    PRINTM(INFO, "ADHOC_RESP: BSSID = %02x:%02x:%02x:%02x:%02x:%02x\n",
           pAdHocResult->BSSID[0], pAdHocResult->BSSID[1],
           pAdHocResult->BSSID[2], pAdHocResult->BSSID[3],
           pAdHocResult->BSSID[4], pAdHocResult->BSSID[5]);
#endif

    LEAVE();
    return ret;
}

/**
 *  @brief This function handles the command response of ad_hoc_stop
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param resp    A pointer to HostCmd_DS_COMMAND
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_ret_802_11_ad_hoc_stop(wlan_private * priv, HostCmd_DS_COMMAND * resp)
{
    ENTER();

    MacEventDisconnected(priv);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

#ifdef REASSOCIATION
/**
 *  @brief This function handles re-association. it is triggered
 *  by re-assoc timer.
 *
 *  @param data    A pointer to wlan_thread structure
 *  @return        WLAN_STATUS_SUCCESS
 */
int
wlan_reassociation_thread(void *data)
{
    wlan_thread *thread = data;
    wlan_private *priv = thread->priv;
    wlan_adapter *Adapter = priv->adapter;
    wait_queue_t wait;
    int i;
    int ret = WLAN_STATUS_SUCCESS;

    OS_INTERRUPT_SAVE_AREA;

    ENTER();

    wlan_activate_thread(thread);
    init_waitqueue_entry(&wait, current);

    current->flags |= PF_NOFREEZE;

    for (;;) {
        add_wait_queue(&thread->waitQ, &wait);
        OS_SET_THREAD_STATE(TASK_INTERRUPTIBLE);

        PRINTM(INFO, "Reassoc: Thread sleeping...\n");

        schedule();

        OS_SET_THREAD_STATE(TASK_RUNNING);
        remove_wait_queue(&thread->waitQ, &wait);

        if (Adapter->SurpriseRemoved) {
            break;
        }

        if (kthread_should_stop()) {
            break;
        }

        PRINTM(INFO, "Reassoc: Thread waking up...\n");

        if (Adapter->InfrastructureMode != Wlan802_11Infrastructure ||
            Adapter->HardwareStatus != WlanHardwareStatusReady) {
            PRINTM(MSG, "Reassoc: mode or hardware status is not correct\n");
            continue;
        }

        /* The semaphore is used to avoid reassociation thread and 
           wlan_set_scan/wlan_set_essid interrupting each other.
           Reassociation should be disabled completely by application if 
           wlan_set_user_scan_ioctl/wlan_set_wap is used.
         */
        if (OS_ACQ_SEMAPHORE_BLOCK(&Adapter->ReassocSem)) {
            PRINTM(ERROR, "Acquire semaphore error, reassociation thread\n");
            goto settimer;
        }

        if (Adapter->MediaConnectStatus != WlanMediaStateDisconnected) {
            OS_REL_SEMAPHORE(&Adapter->ReassocSem);
            PRINTM(MSG, "Reassoc: Adapter->MediaConnectStatus is wrong\n");
            continue;
        }

        PRINTM(INFO, "Reassoc: Required ESSID: %s\n",
               Adapter->PreviousSSID.Ssid);

        PRINTM(INFO, "Reassoc: Performing Active Scan @ %lu\n",
               os_time_get());

        if (Adapter->Prescan) {
            SendSpecificSSIDScan(priv, &Adapter->PreviousSSID);
        }

        /* Try to find the specific SSID we were associated to first */
        i = FindSSIDInList(Adapter,
                           &Adapter->PreviousSSID,
                           Adapter->PreviousBSSID,
                           Adapter->InfrastructureMode);

        if (i < 0) {
            /* If the SSID could not be found, try just the SSID */
            i = FindSSIDInList(Adapter,
                               &Adapter->PreviousSSID,
                               NULL, Adapter->InfrastructureMode);
        }

        if (i >= 0) {
            if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled) {
                ret = PrepareAndSendCommand(priv,
                                            HostCmd_CMD_802_11_SET_WEP,
                                            0, HostCmd_OPTION_WAITFORRSP,
                                            OID_802_11_ADD_WEP, NULL);
                if (ret)
                    PRINTM(INFO, "Ressoc: Fail to set WEP KEY\n");
            }
            wlan_associate(priv, &Adapter->ScanTable[i]);
        }

        OS_REL_SEMAPHORE(&Adapter->ReassocSem);

      settimer:
        if (Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
            PRINTM(INFO, "Reassoc: No AP found or assoc failed."
                   "Restarting re-assoc Timer @ %lu\n", os_time_get());

            Adapter->TimerIsSet = TRUE;
            ModTimer(&Adapter->MrvDrvTimer, MRVDRV_TIMER_10S);
        }
    }

    wlan_deactivate_thread(thread);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}
#endif /* REASSOCIATION */

int
sendADHOCBSSIDQuery(wlan_private * priv)
{
    return PrepareAndSendCommand(priv,
                                 HostCmd_CMD_802_11_IBSS_COALESCING_STATUS,
                                 HostCmd_ACT_GET, 0, 0, NULL);
}
