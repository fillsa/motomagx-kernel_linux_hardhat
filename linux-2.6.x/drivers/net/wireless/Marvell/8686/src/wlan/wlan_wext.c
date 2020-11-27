/** @file  wlan_wext.c 
  * @brief This file contains ioctl functions
  * 
  * (c) Copyright � 2003-2007, Marvell International Ltd. 
  * (c) Copyright � 2008, Motorola.
  *
  * This software file (the "File") is distributed by Marvell International 
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
  * (the "License").  You may use, redistribute and/or modify this File in 
  * accordance with the terms and conditions of the License, a copy of which 
  * is available along with the File in the gpl.txt file or by writing to 
  * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
  * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
  * this warranty disclaimer.
  *
  */
/********************************************************
Change log:
	10/10/05: Add Doxygen format comments
	12/23/05: Modify FindBSSIDInList to search entire table for
	          duplicate BSSIDs when earlier matches are not compatible
	12/26/05: Remove errant memcpy in wlanidle_off; overwriting stack space
	01/05/06: Add kernel 2.6.x support	
	01/11/06: Conditionalize new scan/join functions.
	          Update statics/externs.  Move forward decl. from wlan_decl.h
	04/06/06: Add TSPEC, queue metrics, and MSDU expiry support
	04/10/06: Add hostcmd generic API
	04/18/06: Remove old Subscrive Event and add new Subscribe Event
	          implementation through generic hostcmd API
	05/04/06: Add IBSS coalescing related new iwpriv command
	10/23/06: Validate setbcnavg/setdataavg command parameters and
	          return error if out of range
********************************************************/
/*****************************************************
 Date         Author         Comment
 ==========   ===========    ==========================
 21-Mar-2008  Motorola       Integrate Marvell recovery mechanism in getSNR
 15-May-2008  Motorola       Allow sessiontype command in deepsleep mode.
 21-May-2008  Motorola       Add MPM support.
 04-Jun-2008  Motorola       WIFI driver :integrate Marvell 8686 release (81048p4_26340p77)
 17-Jun-2008  Motorola       WIFI driver :integrate Marvell 8686 release (81048p5_26340p78)
*******************************************************/

#include	"include.h"

#include	"wlan_version.h"

#define GETLOG_BUFSIZE  512

#define MAX_SCAN_CELL_SIZE      (IW_EV_ADDR_LEN + \
				MRVDRV_MAX_SSID_LENGTH + \
				IW_EV_UINT_LEN + IW_EV_FREQ_LEN + \
				IW_EV_QUAL_LEN + MRVDRV_MAX_SSID_LENGTH + \
				IW_EV_PARAM_LEN + 40)   /* 40 for WPAIE */

#define WAIT_FOR_SCAN_RRESULT_MAX_TIME (10 * HZ)

typedef struct _ioctl_cmd
{
    int cmd;
    int subcmd;
    BOOLEAN fixsize;
} ioctl_cmd;

static ioctl_cmd Commands_Allowed_In_DeepSleep[] = {
    {.cmd = WLANDEEPSLEEP,.subcmd = 0,.fixsize = FALSE},
    {.cmd = WLAN_SETNONE_GETWORDCHAR,.subcmd = WLANVERSION,.fixsize = FALSE},
    {.cmd = WLAN_SETINT_GETINT,.subcmd = WLANSDIOCLOCK,.fixsize = TRUE},
    {.cmd = WLAN_SET_GET_2K,.subcmd = WLAN_GET_CFP_TABLE,.fixsize = FALSE},
#ifdef WPRM_DRV    
    {.cmd = WLAN_SET_GET_SIXTEEN_INT,.subcmd = WLAN_SESSIONTYPE,.fixsize = FALSE},
#endif    
#ifdef DEBUG_LEVEL1
    {.cmd = WLAN_SET_GET_SIXTEEN_INT,.subcmd = WLAN_DRV_DBG,.fixsize = FALSE},
#endif
};

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/
#ifdef DEBUG_LEVEL1
#ifdef DEBUG_LEVEL2
#define	DEFAULT_DEBUG_MASK	(0xffffffff & ~DBG_EVENT)
#else
#ifdef MOTO_DBG
#define DEFAULT_DEBUG_MASK	(DBG_MSG | DBG_FATAL | DBG_ERROR | DBG_MOTO_MSG)
#else
#define DEFAULT_DEBUG_MASK	(DBG_MSG | DBG_FATAL)
#endif
#endif
u32 drvdbg = DEFAULT_DEBUG_MASK;
u32 ifdbg = 0;
#endif

/********************************************************
		Local Functions
********************************************************/
static int wlan_set_rate(struct net_device *dev, struct iw_request_info *info,
                         struct iw_param *vwrq, char *extra);
static int wlan_get_rate(struct net_device *dev, struct iw_request_info *info,
                         struct iw_param *vwrq, char *extra);

static int wlan_get_essid(struct net_device *dev,
                          struct iw_request_info *info, struct iw_point *dwrq,
                          char *extra);

static int wlan_set_freq(struct net_device *dev, struct iw_request_info *info,
                         struct iw_freq *fwrq, char *extra);
static int wlan_get_freq(struct net_device *dev, struct iw_request_info *info,
                         struct iw_freq *fwrq, char *extra);

static int wlan_set_mode(struct net_device *dev, struct iw_request_info *info,
                         u32 * uwrq, char *extra);
static int wlan_get_mode(struct net_device *dev, struct iw_request_info *info,
                         u32 * uwrq, char *extra);

static int wlan_set_encode(struct net_device *dev,
                           struct iw_request_info *info,
                           struct iw_point *dwrq, char *extra);
static int wlan_get_encode(struct net_device *dev,
                           struct iw_request_info *info,
                           struct iw_point *dwrq, u8 * extra);

static int wlan_set_txpow(struct net_device *dev,
                          struct iw_request_info *info, struct iw_param *vwrq,
                          char *extra);
static int wlan_get_txpow(struct net_device *dev,
                          struct iw_request_info *info, struct iw_param *vwrq,
                          char *extra);

static int wlan_set_coalescing_ioctl(wlan_private * priv, struct iwreq *wrq);

extern CHANNEL_FREQ_POWER *wlan_get_region_cfp_table(u8 region, u8 band,
                                                     int *cfp_no);

/** 
 *  @brief This function checks if the commans is allowed
 *  in deepsleep/hostsleep mode or not.
 * 
 *  @param req	       A pointer to ifreq structure 
 *  @param cmd         the command ID
 *  @return 	   TRUE or FALSE
 */
static BOOLEAN
Is_Command_Allowed_In_Sleep(struct ifreq *req, int cmd,
                            ioctl_cmd * allowed_cmds, int count)
{
    int subcmd = 0;
    struct iwreq *wrq = (struct iwreq *) req;
    int i;

    for (i = 0; i < count; i++) {
        if (cmd == allowed_cmds[i].cmd) {
            if (allowed_cmds[i].subcmd == 0)
                return TRUE;
            if (allowed_cmds[i].fixsize == TRUE)
                subcmd = (int) req->ifr_data;
            else
                subcmd = wrq->u.data.flags;
            if (allowed_cmds[i].subcmd == subcmd)
                return TRUE;
        }
    }
    return FALSE;
}

/** 
 *  @brief This function checks if the commans is allowed.
 * 
 *  @param priv		A pointer to wlan_private structure
 *  @return		TRUE or FALSE
 */
BOOLEAN
Is_Command_Allowed(wlan_private * priv)
{
    BOOLEAN ret = TRUE;

    if ((priv->adapter->IsDeepSleep == TRUE)) {
        PRINTM(INFO, "IOCTLS called when station is in DeepSleep\n");
        ret = FALSE;
    }

    return ret;
}

/** 
 *  @brief Find a character in a string.
 *   
 *  @param s	   A pointer to string
 *  @param c	   Character to be located 
 *  @param dlen    the length of string
 *  @return 	   A pointer to the first occurrence of c in string, or NULL if c is not found.
 */
static void *
wlan_memchr(void *s, int c, int n)
{
    const u8 *p = s;

    while (n-- != 0) {
        if ((u8) c == *p++) {
            return (void *) (p - 1);
        }
    }
    return NULL;
}

#if WIRELESS_EXT > 14
/** 
 *  @brief Convert mw value to dbm value
 *   
 *  @param mw	   the value of mw
 *  @return 	   the value of dbm
 */
static int
mw_to_dbm(int mw)
{
    if (mw < 2)
        return 0;
    else if (mw < 3)
        return 3;
    else if (mw < 4)
        return 5;
    else if (mw < 6)
        return 7;
    else if (mw < 7)
        return 8;
    else if (mw < 8)
        return 9;
    else if (mw < 10)
        return 10;
    else if (mw < 13)
        return 11;
    else if (mw < 16)
        return 12;
    else if (mw < 20)
        return 13;
    else if (mw < 25)
        return 14;
    else if (mw < 32)
        return 15;
    else if (mw < 40)
        return 16;
    else if (mw < 50)
        return 17;
    else if (mw < 63)
        return 18;
    else if (mw < 79)
        return 19;
    else if (mw < 100)
        return 20;
    else
        return 21;
}
#endif

#define FIRST_VALID_CHANNEL	0xff
/** 
 *  @brief Find the channel frequency power info with specific channel
 *   
 *  @param adapter 	A pointer to wlan_adapter structure
 *  @param band		it can be BAND_A, BAND_G or BAND_B
 *  @param channel      the channel for looking	
 *  @return 	   	A pointer to CHANNEL_FREQ_POWER structure or NULL if not find.
 */
CHANNEL_FREQ_POWER *
find_cfp_by_band_and_channel(wlan_adapter * adapter, u8 band, u16 channel)
{
    CHANNEL_FREQ_POWER *cfp = NULL;
    REGION_CHANNEL *rc;
    int count = sizeof(adapter->region_channel) /
        sizeof(adapter->region_channel[0]);
    int i, j;

    for (j = 0; !cfp && (j < count); j++) {
        rc = &adapter->region_channel[j];

        if (adapter->State11D.Enable11D == ENABLE_11D) {
            rc = &adapter->universal_channel[j];
        }
        if (!rc->Valid || !rc->CFP)
            continue;
        if (rc->Band != band)
            continue;
        if (channel == FIRST_VALID_CHANNEL)
            cfp = &rc->CFP[0];
        else {
            for (i = 0; i < rc->NrCFP; i++) {
                if (rc->CFP[i].Channel == channel) {
                    cfp = &rc->CFP[i];
                    break;
                }
            }
        }
    }

    if (!cfp && channel)
        PRINTM(INFO, "find_cfp_by_band_and_channel(): cannot find "
               "cfp by band %d & channel %d\n", band, channel);

    return cfp;
}

/** 
 *  @brief Find the channel frequency power info with specific frequency
 *   
 *  @param adapter 	A pointer to wlan_adapter structure
 *  @param band		it can be BAND_A, BAND_G or BAND_B
 *  @param freq	        the frequency for looking	
 *  @return 	   	A pointer to CHANNEL_FREQ_POWER structure or NULL if not find.
 */
static CHANNEL_FREQ_POWER *
find_cfp_by_band_and_freq(wlan_adapter * adapter, u8 band, u32 freq)
{
    CHANNEL_FREQ_POWER *cfp = NULL;
    REGION_CHANNEL *rc;
    int count = sizeof(adapter->region_channel) /
        sizeof(adapter->region_channel[0]);
    int i, j;

    for (j = 0; !cfp && (j < count); j++) {
        rc = &adapter->region_channel[j];

        if (adapter->State11D.Enable11D == ENABLE_11D) {
            rc = &adapter->universal_channel[j];
        }

        if (!rc->Valid || !rc->CFP)
            continue;
        if (rc->Band != band)
            continue;
        for (i = 0; i < rc->NrCFP; i++) {
            if (rc->CFP[i].Freq == freq) {
                cfp = &rc->CFP[i];
                break;
            }
        }
    }

    if (!cfp && freq)
        PRINTM(INFO, "find_cfp_by_band_and_freql(): cannot find cfp by "
               "band %d & freq %d\n", band, freq);

    return cfp;
}

#ifdef MFG_CMD_SUPPORT
/** 
 *  @brief Manufacturing command ioctl function
 *   
 *  @param priv 	A pointer to wlan_private structure
 *  @param userdata	A pointer to user buf
 *  @return 	   	WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
static int
wlan_mfg_command(wlan_private * priv, void *userdata)
{
    PkHeader *pkHdr;
    int len, ret;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    // creating the cmdbuf
    if (Adapter->mfg_cmd == NULL) {
        PRINTM(INFO, "Creating cmdbuf\n");
        if (!
            (Adapter->mfg_cmd =
             kmalloc(MRVDRV_SIZE_OF_CMD_BUFFER, GFP_KERNEL))) {
            PRINTM(INFO, "kmalloc failed!\n");
            return WLAN_STATUS_FAILURE;
        }
        Adapter->mfg_cmd_len = MRVDRV_SIZE_OF_CMD_BUFFER;
    }
    // get PktHdr from userdata
    if (copy_from_user(Adapter->mfg_cmd, userdata, sizeof(PkHeader))) {
        PRINTM(INFO, "copy from user failed :PktHdr\n");
        ret = WLAN_STATUS_FAILURE;
        goto mfg_exit;
    }
    // get the size
    pkHdr = (PkHeader *) Adapter->mfg_cmd;
    len = pkHdr->len;

    PRINTM(INFO, "cmdlen = %d\n", (u32) len);

    while (len >= Adapter->mfg_cmd_len) {
        kfree(Adapter->mfg_cmd);

        if (!(Adapter->mfg_cmd = kmalloc(len + 256, GFP_KERNEL))) {
            PRINTM(INFO, "kmalloc failed!\n");
            return WLAN_STATUS_FAILURE;
        }

        Adapter->mfg_cmd_len = len + 256;
    }

    // get the whole command from user
    if (copy_from_user(Adapter->mfg_cmd, userdata, len)) {
        PRINTM(INFO, "copy from user failed :PktHdr\n");
        ret = WLAN_STATUS_FAILURE;
        goto mfg_exit;
    }

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_MFG_COMMAND,
                                0, HostCmd_OPTION_WAITFORRSP, 0, NULL);

    if (ret) {
        goto mfg_exit;
    }
    // copy it back to user
    if (Adapter->mfg_cmd_resp_len > 0) {
        if (copy_to_user(userdata, Adapter->mfg_cmd, len)) {
            PRINTM(INFO, "copy to user failed \n");
            ret = WLAN_STATUS_FAILURE;
        }
    }

  mfg_exit:
    kfree(Adapter->mfg_cmd);
    Adapter->mfg_cmd = NULL;
    Adapter->mfg_cmd_len = 0;
    LEAVE();
    return ret;
}
#endif

/** 
 *  @brief Check if Rate Auto
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @return 	   		TRUE/FALSE
 */
BOOLEAN
Is_Rate_Auto(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int i;
    int ratenum = 0;
    int bitsize = 0;
    bitsize = sizeof(Adapter->RateBitmap) * 8;
    for (i = 0; i < bitsize; i++) {
        if (Adapter->RateBitmap & (1 << i))
            ratenum++;
        if (ratenum > 1)
            break;
    }
    if (ratenum > 1)
        return TRUE;
    else
        return FALSE;
}

/** 
 *  @brief Covert Rate Bitmap to Rate index
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @return 	   		TRUE/FALSE
 */
int
GetRateIndex(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int bitsize = sizeof(Adapter->RateBitmap) * 8;
    int i;
    for (i = 0; i < bitsize; i++) {
        if (Adapter->RateBitmap & (1 << i))
            return i;
    }
    return 0;
}

/** 
 *  @brief Update Current Channel 
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @return 	   		WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
static int
UpdateCurrentChannel(wlan_private * priv)
{
    int ret;

    /*
     ** the channel in f/w could be out of sync, get the current channel
     */
    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_RF_CHANNEL,
                                HostCmd_OPT_802_11_RF_CHANNEL_GET,
                                HostCmd_OPTION_WAITFORRSP, 0, NULL);

    PRINTM(INFO, "Current Channel = %d\n",
           priv->adapter->CurBssParams.channel);

    return ret;
}

/** 
 *  @brief Set Current Channel 
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @param channel		The channel to be set. 
 *  @return 	   		WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
static int
SetCurrentChannel(wlan_private * priv, int channel)
{
    PRINTM(INFO, "Set Channel = %d\n", channel);

    /* 
     **  Current channel is not set to AdhocChannel requested, set channel
     */
    return (PrepareAndSendCommand(priv, HostCmd_CMD_802_11_RF_CHANNEL,
                                  HostCmd_OPT_802_11_RF_CHANNEL_SET,
                                  HostCmd_OPTION_WAITFORRSP, 0, &channel));
}

/** 
 *  @brief Change Adhoc Channel
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @param channel		The channel to be set. 
 *  @return 	   		WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
static int
ChangeAdhocChannel(wlan_private * priv, int channel)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    Adapter->AdhocChannel = channel;

    UpdateCurrentChannel(priv);

    if (Adapter->CurBssParams.channel == Adapter->AdhocChannel) {
        /* AdhocChannel is set to the current Channel already */
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    PRINTM(INFO, "Updating Channel from %d to %d\n",
           Adapter->CurBssParams.channel, Adapter->AdhocChannel);

    SetCurrentChannel(priv, Adapter->AdhocChannel);

    UpdateCurrentChannel(priv);

    if (Adapter->CurBssParams.channel != Adapter->AdhocChannel) {
        PRINTM(INFO, "Failed to updated Channel to %d, channel = %d\n",
               Adapter->AdhocChannel, Adapter->CurBssParams.channel);
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        int i;
        WLAN_802_11_SSID curAdhocSsid;

        PRINTM(INFO, "Channel Changed while in an IBSS\n");

        /* Copy the current ssid */
        memcpy(&curAdhocSsid, &Adapter->CurBssParams.ssid,
               sizeof(WLAN_802_11_SSID));

        /* Exit Adhoc mode */
        PRINTM(INFO, "In ChangeAdhocChannel(): Sending Adhoc Stop\n");
        ret = StopAdhocNetwork(priv);

        if (ret) {
            LEAVE();
            return ret;
        }

        /* Scan for the network */
        SendSpecificSSIDScan(priv, &curAdhocSsid);

        // find out the BSSID that matches the current SSID 
        i = FindSSIDInList(Adapter, &curAdhocSsid, NULL, Wlan802_11IBSS);

        if (i >= 0) {
            PRINTM(INFO, "SSID found at %d in List," "so join\n", i);
            JoinAdhocNetwork(priv, &Adapter->ScanTable[i]);
        } else {
            // else send START command
            PRINTM(INFO, "SSID not found in list, "
                   "so creating adhoc with ssid = %s\n", curAdhocSsid.Ssid);
            StartAdhocNetwork(priv, &curAdhocSsid);
        }                       // end of else (START command)
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set WPA key
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_encode_wpa(struct net_device *dev,
                    struct iw_request_info *info,
                    struct iw_point *dwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    PWLAN_802_11_KEY pKey;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    pKey = (PWLAN_802_11_KEY) extra;

    HEXDUMP("Key buffer: ", extra, dwrq->length);

    // current driver only supports key length of up to 32 bytes
    if (pKey->KeyLength > MRVL_MAX_WPA_KEY_LENGTH) {
        PRINTM(INFO, " Error in key length \n");
        return WLAN_STATUS_FAILURE;
    }

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_KEY_MATERIAL,
                                HostCmd_ACT_SET,
                                HostCmd_OPTION_WAITFORRSP,
                                KEY_INFO_ENABLED, pKey);
    if (ret) {
        LEAVE();
        return ret;
    }

    LEAVE();
    return ret;
}

/*
 *  iwconfig ethX key on:	WEPEnabled;
 *  iwconfig ethX key off:	WEPDisabled;
 *  iwconfig ethX key [x]:	CurrentWepKeyIndex = x; WEPEnabled;
 *  iwconfig ethX key [x] kstr:	WepKey[x] = kstr;
 *  iwconfig ethX key kstr:	WepKey[CurrentWepKeyIndex] = kstr;
 *
 *  all:			Send command SET_WEP;
 				SetMacPacketFilter;
 */

/** 
 *  @brief Set WEP key
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_encode_nonwpa(struct net_device *dev,
                       struct iw_request_info *info,
                       struct iw_point *dwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    MRVL_WEP_KEY *pWep;
    WLAN_802_11_SSID ssid;
    int index, PrevAuthMode;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    if (Adapter->CurrentWepKeyIndex >= MRVL_NUM_WEP_KEY)
        Adapter->CurrentWepKeyIndex = 0;
    pWep = &Adapter->WepKey[Adapter->CurrentWepKeyIndex];
    PrevAuthMode = Adapter->SecInfo.AuthenticationMode;

    index = (dwrq->flags & IW_ENCODE_INDEX) - 1;

    if (index >= 4) {
        PRINTM(INFO, "Key index #%d out of range.\n", index + 1);
        return -EINVAL;
    }

    PRINTM(INFO, "Flags=0x%x, Length=%d Index=%d CurrentWepKeyIndex=%d\n",
           dwrq->flags, dwrq->length, index, Adapter->CurrentWepKeyIndex);

    if (dwrq->length > 0) {
        /* iwconfig ethX key [n] xxxxxxxxxxx 
         * Key has been provided by the user 
         */

        /*
         * Check the size of the key 
         */

        if (dwrq->length > MAX_KEY_SIZE) {
            return -EINVAL;
        }

        /*
         * Check the index (none -> use current) 
         */

        if (index < 0 || index > 3)     //invalid index or no index
            index = Adapter->CurrentWepKeyIndex;
        else                    //index is given & valid
            pWep = &Adapter->WepKey[index];

        /*
         * Check if the key is not marked as invalid 
         */
        if (!(dwrq->flags & IW_ENCODE_NOKEY)) {
            /* Cleanup */
            memset(pWep, 0, sizeof(MRVL_WEP_KEY));

            /* Copy the key in the driver */
            memcpy(pWep->KeyMaterial, extra, dwrq->length);

            /* Set the length */
            if (dwrq->length > MIN_KEY_SIZE) {
                pWep->KeyLength = MAX_KEY_SIZE;
            } else {
                if (dwrq->length > 0) {
                    pWep->KeyLength = MIN_KEY_SIZE;
                } else {
                    /* Disable the key */
                    pWep->KeyLength = 0;
                }
            }
            pWep->KeyIndex = index;

            if (Adapter->SecInfo.WEPStatus != Wlan802_11WEPEnabled) {
                /*
                 * The status is set as Key Absent 
                 * so as to make sure we display the 
                 * keys when iwlist ethX key is 
                 * used - MPS 
                 */

                Adapter->SecInfo.WEPStatus = Wlan802_11WEPKeyAbsent;
            }

            PRINTM(INFO, "KeyIndex=%u KeyLength=%u\n",
                   pWep->KeyIndex, pWep->KeyLength);
            HEXDUMP("WepKey", (u8 *) pWep->KeyMaterial, pWep->KeyLength);
        }
    } else {
        /*
         * No key provided so it is either enable key, 
         * on or off */
        if (dwrq->flags & IW_ENCODE_DISABLED) {
            PRINTM(INFO, "*** iwconfig ethX key off ***\n");

            Adapter->SecInfo.WEPStatus = Wlan802_11WEPDisabled;
            if (Adapter->SecInfo.AuthenticationMode ==
                Wlan802_11AuthModeShared)
                Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;
        } else {
            /* iwconfig ethX key [n]
             * iwconfig ethX key on 
             * Do we want to just set the transmit key index ? 
             */

            if (index < 0 || index > 3) {
                PRINTM(INFO, "*** iwconfig ethX key on ***\n");
                index = Adapter->CurrentWepKeyIndex;
            } else {
                PRINTM(INFO, "*** iwconfig ethX key [x=%d] ***\n", index);
                Adapter->CurrentWepKeyIndex = index;
            }

            /* Copy the required key as the current key */
            pWep = &Adapter->WepKey[index];

            if (!pWep->KeyLength) {
                PRINTM(INFO, "Key not set,so cannot enable it\n");
                return -EPERM;
            }

            Adapter->SecInfo.WEPStatus = Wlan802_11WEPEnabled;

            HEXDUMP("KeyMaterial", (u8 *) pWep->KeyMaterial, pWep->KeyLength);
        }
    }

    if (pWep->KeyLength) {
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_SET_WEP,
                                    0, HostCmd_OPTION_WAITFORRSP,
                                    OID_802_11_ADD_WEP, NULL);

        if (ret) {
            LEAVE();
            return ret;
        }
    }

    if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled) {
        Adapter->CurrentPacketFilter |= HostCmd_ACT_MAC_WEP_ENABLE;
    } else {
        Adapter->CurrentPacketFilter &= ~HostCmd_ACT_MAC_WEP_ENABLE;
    }

    SetMacPacketFilter(priv);

    if (dwrq->flags & IW_ENCODE_RESTRICTED) {
        /* iwconfig ethX restricted key [1] */
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeShared;
        PRINTM(INFO, "Auth mode restricted!\n");
    } else if (dwrq->flags & IW_ENCODE_OPEN) {
        /* iwconfig ethX key [2] open */
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;
        PRINTM(INFO, "Auth mode open!\n");
    }

    /*
     * If authentication mode changed - de-authenticate, set authentication
     * method and re-associate if we were previously associated.
     */
    if (Adapter->SecInfo.AuthenticationMode != PrevAuthMode) {
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected &&
            Adapter->InfrastructureMode == Wlan802_11Infrastructure) {

            /* keep a copy of the ssid associated with */
            memcpy(&ssid, &Adapter->CurBssParams.ssid, sizeof(ssid));

            /*
             * De-authenticate from AP 
             */

            ret = SendDeauthentication(priv);

            if (ret) {
                LEAVE();
                return ret;
            }

        } else {
            /* reset ssid */
            memset(&ssid, 0, sizeof(ssid));
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set RX Antenna
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param Mode			RF antenna mode
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
SetRxAntenna(wlan_private * priv, int Mode)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    if (Mode != RF_ANTENNA_1 && Mode != RF_ANTENNA_2
        && Mode != RF_ANTENNA_AUTO) {
        return -EINVAL;
    }

    Adapter->RxAntennaMode = Mode;

    PRINTM(INFO, "SET RX Antenna Mode to 0x%04x\n", Adapter->RxAntennaMode);

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_RF_ANTENNA,
                                HostCmd_ACT_SET_RX, HostCmd_OPTION_WAITFORRSP,
                                0, &Adapter->RxAntennaMode);
    return ret;
}

/** 
 *  @brief Set TX Antenna
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param Mode			RF antenna mode
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
SetTxAntenna(wlan_private * priv, int Mode)
{
    int ret = 0;
    wlan_adapter *Adapter = priv->adapter;

    if ((Mode != RF_ANTENNA_1) && (Mode != RF_ANTENNA_2)
        && (Mode != RF_ANTENNA_AUTO)) {
        return -EINVAL;
    }

    Adapter->TxAntennaMode = Mode;

    PRINTM(INFO, "SET TX Antenna Mode to 0x%04x\n", Adapter->TxAntennaMode);

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_RF_ANTENNA,
                                HostCmd_ACT_SET_TX, HostCmd_OPTION_WAITFORRSP,
                                0, &Adapter->TxAntennaMode);

    return ret;
}

/** 
 *  @brief Get RX Antenna
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param buf			A pointer to recieve antenna mode
 *  @return 	   		length of buf 
 */
static int
GetRxAntenna(wlan_private * priv, char *buf)
{
    int ret = 0;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    // clear it, so we will know if the value 
    // returned below is correct or not.
    Adapter->RxAntennaMode = 0;

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_RF_ANTENNA,
                                HostCmd_ACT_GET_RX, HostCmd_OPTION_WAITFORRSP,
                                0, NULL);

    if (ret) {
        LEAVE();
        return ret;
    }

    PRINTM(INFO, "Get Rx Antenna Mode:0x%04x\n", Adapter->RxAntennaMode);

    LEAVE();

    return sprintf(buf, "0x%04x", Adapter->RxAntennaMode) + 1;
}

/** 
 *  @brief Get TX Antenna
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param buf			A pointer to recieve antenna mode
 *  @return 	   		length of buf 
 */
static int
GetTxAntenna(wlan_private * priv, char *buf)
{
    int ret = 0;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    // clear it, so we will know if the value 
    // returned below is correct or not.
    Adapter->TxAntennaMode = 0;

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_RF_ANTENNA,
                                HostCmd_ACT_GET_TX, HostCmd_OPTION_WAITFORRSP,
                                0, NULL);

    if (ret) {
        LEAVE();
        return ret;
    }

    PRINTM(INFO, "Get Tx Antenna Mode:0x%04x\n", Adapter->TxAntennaMode);

    LEAVE();

    return sprintf(buf, "0x%04x", Adapter->TxAntennaMode) + 1;
}

/** 
 *  @brief Set Radio On/OFF
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @option 			Radio Option
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail 
 */
static int
wlan_radio_ioctl(wlan_private * priv, u8 option)
{
    int ret = 0;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->RadioOn != option) {
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            PRINTM(MSG, "Cannot turn radio off in connected state.\n");
            LEAVE();
            return -EINVAL;
        }

        PRINTM(INFO, "Switching %s the Radio\n", option ? "On" : "Off");
        Adapter->RadioOn = option;

        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_RADIO_CONTROL,
                                    HostCmd_ACT_GEN_SET,
                                    HostCmd_OPTION_WAITFORRSP, 0, NULL);
    }

    LEAVE();
    return ret;
}

#ifdef REASSOCIATION
/** 
 *  @brief Set Auto Reassociation On
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail 
 */
static int
reassociation_on(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    Adapter->Reassoc_on = TRUE;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Auto Reassociation Off
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail 
 */
static int
reassociation_off(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->TimerIsSet == TRUE) {
        CancelTimer(&Adapter->MrvDrvTimer);
        Adapter->TimerIsSet = FALSE;
    }

    Adapter->Reassoc_on = FALSE;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}
#endif /* REASSOCIATION */

/** 
 *  @brief Set Region
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param region_code		region code
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail 
 */
static int
wlan_set_region(wlan_private * priv, u16 region_code)
{
    int i;

    ENTER();

    for (i = 0; i < MRVDRV_MAX_REGION_CODE; i++) {
        // use the region code to search for the index
        if (region_code == RegionCodeToIndex[i]) {
            priv->adapter->RegionTableIndex = (u16) i;
            priv->adapter->RegionCode = region_code;
            break;
        }
    }

    // if it's unidentified region code
    if (i >= MRVDRV_MAX_REGION_CODE) {
        PRINTM(INFO, "Region Code not identified\n");
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    if (wlan_set_regiontable(priv, priv->adapter->RegionCode, 0)) {
        LEAVE();
        return -EINVAL;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Copy Rates
 *   
 *  @param dest                 A pointer to Dest Buf
 *  @param src		        A pointer to Src Buf
 *  @param len                  The len of Src Buf
 *  @return 	   	        Number of Rates copyed 
 */
static inline int
CopyRates(u8 * dest, int pos, u8 * src, int len)
{
    int i;

    for (i = 0; i < len && src[i]; i++, pos++) {
        if (pos >= sizeof(WLAN_802_11_RATES))
            break;
        dest[pos] = src[i];
    }

    return pos;
}

/** 
 *  @brief Get active data rates
 *   
 *  @param Adapter              A pointer to wlan_adapter structure
 *  @param rate		        The buf to return the active rates
 *  @return 	   	        The number of Rates
 */
static int
get_active_data_rates(wlan_adapter * Adapter, WLAN_802_11_RATES rates)
{
    int k = 0;

    ENTER();

    if (Adapter->MediaConnectStatus != WlanMediaStateConnected) {
        if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
            //Infra. mode
            PRINTM(INFO, "Infra\n");
            k = CopyRates(rates, k, SupportedRates, sizeof(SupportedRates));
        } else {
            //ad-hoc mode
            PRINTM(INFO, "Adhoc G\n");
            k = CopyRates(rates, k, AdhocRates_G, sizeof(AdhocRates_G));
        }
    } else {
        k = CopyRates(rates, 0, Adapter->CurBssParams.DataRates,
                      Adapter->CurBssParams.NumOfRates);
    }

    LEAVE();

    return k;
}

/** 
 *  @brief Get/Set Firmware wakeup method
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_txcontrol(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data;
    ENTER();

    if ((int) wrq->u.data.length == 0) {
        if (copy_to_user
            (wrq->u.data.pointer, &Adapter->PktTxCtrl, sizeof(u32))) {
            PRINTM(MSG, "copy_to_user failed!\n");
            return -EFAULT;
        }
    } else {
        if ((int) wrq->u.data.length > 1) {
            PRINTM(MSG, "ioctl too many args!\n");
            return -EFAULT;
        }
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        Adapter->PktTxCtrl = (u32) data;
    }

    wrq->u.data.length = 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Enable/Disable atim uapsd null package generation
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_null_pkg_gen(wlan_private * priv, struct iwreq *wrq)
{
    int data;
    wlan_adapter *Adapter = priv->adapter;
    int *val;

    ENTER();

    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    PRINTM(INFO, "Enable UAPSD NULL PKG: %s\n",
           (data == CMD_ENABLED) ? "Enable" : "Disable");
    switch (data) {
    case CMD_ENABLED:
        Adapter->gen_null_pkg = TRUE;
        break;
    case CMD_DISABLED:
        Adapter->gen_null_pkg = FALSE;
        break;
    default:
        break;
    }

    data = (Adapter->gen_null_pkg == TRUE) ? CMD_ENABLED : CMD_DISABLED;
    val = (int *) wrq->u.name;
    *val = data;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set NULL Package generation interval
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_null_pkt_interval(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data;
    ENTER();

    if ((int) wrq->u.data.length == 0) {
        data = Adapter->NullPktInterval;

        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(MSG, "copy_to_user failed!\n");
            return -EFAULT;
        }
    } else {
        if ((int) wrq->u.data.length > 1) {
            PRINTM(MSG, "ioctl too many args!\n");
            return -EFAULT;
        }
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        Adapter->NullPktInterval = data;
    }

    wrq->u.data.length = 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Adhoc awake period 
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_adhoc_awake_period(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data;
    ENTER();

    if ((int) wrq->u.data.length == 0) {
        data = Adapter->AdhocAwakePeriod;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(MSG, "copy_to_user failed!\n");
            return -EFAULT;
        }
    } else {
        if ((int) wrq->u.data.length > 1) {
            PRINTM(MSG, "ioctl too many args!\n");
            return -EFAULT;
        }
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
#define AWAKE_PERIOD_MIN 1
#define AWAKE_PERIOD_MAX 31
#define DISABLE_AWAKE_PERIOD 0xff
        if ((((data & 0xff) >= AWAKE_PERIOD_MIN) &&
             ((data & 0xff) <= AWAKE_PERIOD_MAX)) ||
            ((data & 0xff) == DISABLE_AWAKE_PERIOD))
            Adapter->AdhocAwakePeriod = (u16) data;
        else {
            PRINTM(INFO,
                   "Invalid parameter, AdhocAwakePeriod not changed.\n");
            return -EINVAL;

        }
    }
    wrq->u.data.length = 1;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set bcn missing timeout 
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_bcn_miss_timeout(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data;
    ENTER();

    if ((int) wrq->u.data.length == 0) {
        data = Adapter->BCNMissTimeOut;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(MSG, "copy_to_user failed!\n");
            return -EFAULT;
        }
    } else {
        if ((int) wrq->u.data.length > 1) {
            PRINTM(MSG, "ioctl too many args!\n");
            return -EFAULT;
        }
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
        if (((data >= 0) && (data <= 50)) || (data == 0xffff))
            Adapter->BCNMissTimeOut = (u16) data;
        else {
            PRINTM(INFO,
                   "Invalid parameter, BCN Missing timeout not changed.\n");
            return -EINVAL;

        }
    }

    wrq->u.data.length = 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set sdio mode
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_sdio_mode(wlan_private * priv, struct iwreq *wrq)
{
    int data;
    int bus_width;
    wlan_adapter *Adapter = priv->adapter;
    ENTER();

    if ((int) wrq->u.data.length == 0) {
        data = ((mmc_card_t) ((priv->wlan_dev).card))->ctrlr->bus_width;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(MSG, "copy_to_user failed!\n");
            return -EFAULT;
        }
    } else {
        if ((int) wrq->u.data.length > 1) {
            PRINTM(MSG, "ioctl too many args!\n");
            return -EFAULT;
        }
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
        if ((data != SDIO_BUSWIDTH_1_BIT) && (data != SDIO_BUSWIDTH_4_BIT))
            return -EFAULT;
        bus_width = ((mmc_card_t) ((priv->wlan_dev).card))->ctrlr->bus_width;
        if (bus_width != data)
            Adapter->sdiomode = (u8) data;
    }
    wrq->u.data.length = 1;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set LDO config
 *  @param priv			A pointer to wlan_private structure
 *  @param wrq			A pointer to wrq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_ldo_config(wlan_private * priv, struct iwreq *wrq)
{
    HostCmd_DS_802_11_LDO_CONFIG ldocfg;
    int data = 0;
    u16 action;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length == 0) {
        action = HostCmd_ACT_GEN_GET;
    } else if (wrq->u.data.length > 1) {
        PRINTM(MSG, "ioctl too many args!\n");
        ret = -EFAULT;
        goto ldoexit;
    } else {
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            ret = -EFAULT;
            goto ldoexit;
        }
        if (data != LDO_INTERNAL && data != LDO_EXTERNAL) {
            PRINTM(MSG, "Invalid parameter, LDO config not changed.\n");
            ret = -EFAULT;
            goto ldoexit;
        }
        action = HostCmd_ACT_GEN_SET;
    }
    ldocfg.Action = action;
    ldocfg.PMSource = (u16) data;

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_LDO_CONFIG,
                                action, HostCmd_OPTION_WAITFORRSP,
                                0, (void *) &ldocfg);

    if (!ret && action == HostCmd_ACT_GEN_GET) {
        data = (int) ldocfg.PMSource;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
            goto ldoexit;
        }
        wrq->u.data.length = 1;
    }

  ldoexit:
    LEAVE();
    return ret;
}

#ifdef DEBUG_LEVEL1
/** 
 *  @brief Get/Set the bit mask of driver debug message control
 *  @param priv			A pointer to wlan_private structure
 *  @param wrq			A pointer to wrq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_drv_dbg(wlan_private * priv, struct iwreq *wrq)
{
    int data[4];
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length == 0) {
        data[0] = drvdbg;
        data[1] = ifdbg;
        /* Return the current driver debug bit masks */
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 2)) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
            goto drvdbgexit;
        }
        wrq->u.data.length = 2;
    } else if (wrq->u.data.length < 3) {
        /* Get the driver debug bit masks */
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            ret = -EFAULT;
            goto drvdbgexit;
        }
        drvdbg = data[0];
        if (wrq->u.data.length == 2)
            ifdbg = data[1];
    } else {
        PRINTM(INFO, "Invalid parameter number\n");
        goto drvdbgexit;
    }

    printk(KERN_ALERT "drvdbg = 0x%x\n", drvdbg);
#ifdef DEBUG_LEVEL2
    printk(KERN_ALERT "INFO  (%08x) %s\n", DBG_INFO,
           (drvdbg & DBG_INFO) ? "X" : "");
    printk(KERN_ALERT "WARN  (%08x) %s\n", DBG_WARN,
           (drvdbg & DBG_WARN) ? "X" : "");
    printk(KERN_ALERT "ENTRY (%08x) %s\n", DBG_ENTRY,
           (drvdbg & DBG_ENTRY) ? "X" : "");
#endif
    printk(KERN_ALERT "FW_D  (%08x) %s\n", DBG_FW_D,
           (drvdbg & DBG_FW_D) ? "X" : "");
    printk(KERN_ALERT "CMD_D (%08x) %s\n", DBG_CMD_D,
           (drvdbg & DBG_CMD_D) ? "X" : "");
    printk(KERN_ALERT "DAT_D (%08x) %s\n", DBG_DAT_D,
           (drvdbg & DBG_DAT_D) ? "X" : "");
#ifdef MOTO_DBG
    printk(KERN_ALERT "MOTO_DEBUG (%08x) %s\n", DBG_MOTO_DEBUG,
           (drvdbg & DBG_MOTO_DEBUG) ? "X" : "");
    printk(KERN_ALERT "MOTO_MSG   (%08x) %s\n", DBG_MOTO_MSG,
           (drvdbg & DBG_MOTO_MSG) ? "X" : "");
#endif
    printk(KERN_ALERT "INTR  (%08x) %s\n", DBG_INTR,
           (drvdbg & DBG_INTR) ? "X" : "");
    printk(KERN_ALERT "EVENT (%08x) %s\n", DBG_EVENT,
           (drvdbg & DBG_EVENT) ? "X" : "");
    printk(KERN_ALERT "CMND  (%08x) %s\n", DBG_CMND,
           (drvdbg & DBG_CMND) ? "X" : "");
    printk(KERN_ALERT "DATA  (%08x) %s\n", DBG_DATA,
           (drvdbg & DBG_DATA) ? "X" : "");
    printk(KERN_ALERT "ERROR (%08x) %s\n", DBG_ERROR,
           (drvdbg & DBG_ERROR) ? "X" : "");
    printk(KERN_ALERT "FATAL (%08x) %s\n", DBG_FATAL,
           (drvdbg & DBG_FATAL) ? "X" : "");
    printk(KERN_ALERT "MSG   (%08x) %s\n", DBG_MSG,
           (drvdbg & DBG_MSG) ? "X" : "");
    printk(KERN_ALERT "ifdbg = 0x%x\n", ifdbg);
    printk(KERN_ALERT "IF_D  (%08x) %s\n", DBG_IF_D,
           (ifdbg & DBG_IF_D) ? "X" : "");

  drvdbgexit:
    LEAVE();
    return ret;
}
#endif

/** 
 *  @brief Commit handler: called after a bunch of SET operations
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_config_commit(struct net_device *dev,
                   struct iw_request_info *info, char *cwrq, char *extra)
{
    ENTER();

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get protocol name 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_name(struct net_device *dev, struct iw_request_info *info,
              char *cwrq, char *extra)
{
    const char *cp;
    char comm[6] = { "COMM-" };
    char mrvl[6] = { "MRVL-" };
    int cnt;

    ENTER();

#ifdef MOTO_DBG
    PRINTM(MOTO_DEBUG,
           "SIOCGIWNAME (wlan_get_name) ioctl called by upper layer.\n");
#endif

    strcpy(cwrq, mrvl);

    cp = strstr(driver_version, comm);
    if (cp == driver_version)   //skip leading "COMM-"
        cp = driver_version + strlen(comm);
    else
        cp = driver_version;

    cnt = strlen(mrvl);
    cwrq += cnt;
    while (cnt < 16 && (*cp != '-')) {
        *cwrq++ = toupper(*cp++);
        cnt++;
    }
    *cwrq = '\0';

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get frequency
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param fwrq 		A pointer to iw_freq structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_freq(struct net_device *dev, struct iw_request_info *info,
              struct iw_freq *fwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    CHANNEL_FREQ_POWER *cfp;

    ENTER();

#ifdef MOTO_DBG
    PRINTM(MOTO_DEBUG,
           "SIOCGIWFREQ (wlan_get_freq) ioctl called by upper layer.\n");
#endif

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    cfp = find_cfp_by_band_and_channel(Adapter, 0,
                                       (u16) Adapter->CurBssParams.channel);

    if (!cfp) {
        if (Adapter->CurBssParams.channel)
            PRINTM(INFO, "Invalid channel=%d\n",
                   Adapter->CurBssParams.channel);
        return -EINVAL;
    }

    fwrq->m = (long) cfp->Freq * 100000;
    fwrq->e = 1;

    PRINTM(INFO, "freq=%u\n", fwrq->m);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get current BSSID
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param awrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_wap(struct net_device *dev, struct iw_request_info *info,
             struct sockaddr *awrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        memcpy(awrq->sa_data, Adapter->CurBssParams.bssid, ETH_ALEN);
    } else {
        memset(awrq->sa_data, 0, ETH_ALEN);
    }
    awrq->sa_family = ARPHRD_ETHER;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Adapter Node Name
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_nick(struct net_device *dev, struct iw_request_info *info,
              struct iw_point *dwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    /*
     * Check the size of the string 
     */

    if (dwrq->length > 16) {
        return -E2BIG;
    }

    memset(Adapter->nodeName, 0, sizeof(Adapter->nodeName));
    memcpy(Adapter->nodeName, extra, dwrq->length);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Adapter Node Name
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_nick(struct net_device *dev, struct iw_request_info *info,
              struct iw_point *dwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    /*
     * Get the Nick Name saved 
     */

    strncpy(extra, Adapter->nodeName, 16);

    extra[16] = '\0';

    /*
     * If none, we may want to get the one that was set 
     */

    /*
     * Push it out ! 
     */
    dwrq->length = strlen(extra) + 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set RTS threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_rts(struct net_device *dev, struct iw_request_info *info,
             struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    int rthr = vwrq->value;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    if (vwrq->disabled) {
        Adapter->RTSThsd = rthr = MRVDRV_RTS_MAX_VALUE;
    } else {
        if (rthr < MRVDRV_RTS_MIN_VALUE || rthr > MRVDRV_RTS_MAX_VALUE)
            return -EINVAL;
        Adapter->RTSThsd = rthr;
    }

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SNMP_MIB,
                                HostCmd_ACT_SET, HostCmd_OPTION_WAITFORRSP,
                                OID_802_11_RTS_THRESHOLD, &rthr);

    LEAVE();
    return ret;
}

/** 
 *  @brief Get RTS threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_rts(struct net_device *dev, struct iw_request_info *info,
             struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    Adapter->RTSThsd = 0;
    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SNMP_MIB,
                                HostCmd_ACT_GET, HostCmd_OPTION_WAITFORRSP,
                                OID_802_11_RTS_THRESHOLD, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }

    vwrq->value = Adapter->RTSThsd;
    vwrq->disabled = ((vwrq->value < MRVDRV_RTS_MIN_VALUE)
                      || (vwrq->value > MRVDRV_RTS_MAX_VALUE));
    vwrq->fixed = 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Fragment threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_frag(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    int fthr = vwrq->value;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    if (vwrq->disabled) {
        Adapter->FragThsd = fthr = MRVDRV_FRAG_MAX_VALUE;
    } else {
        if (fthr < MRVDRV_FRAG_MIN_VALUE || fthr > MRVDRV_FRAG_MAX_VALUE)
            return -EINVAL;
        Adapter->FragThsd = fthr;
    }

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SNMP_MIB,
                                HostCmd_ACT_SET, HostCmd_OPTION_WAITFORRSP,
                                OID_802_11_FRAGMENTATION_THRESHOLD, &fthr);
    LEAVE();
    return ret;
}

/** 
 *  @brief Get Fragment threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_frag(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    Adapter->FragThsd = 0;
    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_SNMP_MIB, HostCmd_ACT_GET,
                                HostCmd_OPTION_WAITFORRSP,
                                OID_802_11_FRAGMENTATION_THRESHOLD, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }

    vwrq->value = Adapter->FragThsd;
    vwrq->disabled = ((vwrq->value < MRVDRV_FRAG_MIN_VALUE)
                      || (vwrq->value > MRVDRV_FRAG_MAX_VALUE));
    vwrq->fixed = 1;

    LEAVE();
    return ret;
}

/** 
 *  @brief Get Wlan Mode
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_mode(struct net_device *dev,
              struct iw_request_info *info, u32 * uwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *adapter = priv->adapter;

    ENTER();

    switch (adapter->InfrastructureMode) {
    case Wlan802_11IBSS:
        *uwrq = IW_MODE_ADHOC;
        break;

    case Wlan802_11Infrastructure:
        *uwrq = IW_MODE_INFRA;
        break;

    default:
    case Wlan802_11AutoUnknown:
        *uwrq = IW_MODE_AUTO;
        break;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Encryption key
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_encode(struct net_device *dev,
                struct iw_request_info *info,
                struct iw_point *dwrq, u8 * extra)
{

    wlan_private *priv = dev->priv;
    wlan_adapter *adapter = priv->adapter;
    int index = (dwrq->flags & IW_ENCODE_INDEX);

    ENTER();

    PRINTM(INFO, "flags=0x%x index=%d length=%d CurrentWepKeyIndex=%d\n",
           dwrq->flags, index, dwrq->length, adapter->CurrentWepKeyIndex);
    if (index < 0 || index > 4) {
        PRINTM(INFO, "Key index #%d out of range.\n", index);
        LEAVE();
        return -EINVAL;
    }
    if (adapter->CurrentWepKeyIndex >= MRVL_NUM_WEP_KEY)
        adapter->CurrentWepKeyIndex = 0;
    dwrq->flags = 0;

    /*
     * Check encryption mode 
     */

    switch (adapter->SecInfo.AuthenticationMode) {
    case Wlan802_11AuthModeOpen:
        dwrq->flags = IW_ENCODE_OPEN;
        break;

    case Wlan802_11AuthModeShared:
    case Wlan802_11AuthModeNetworkEAP:
        dwrq->flags = IW_ENCODE_RESTRICTED;
        break;
    default:
        dwrq->flags = IW_ENCODE_DISABLED | IW_ENCODE_OPEN;
        break;
    }

    if ((adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled)
        || (adapter->SecInfo.WEPStatus == Wlan802_11WEPKeyAbsent)
        || adapter->SecInfo.WPAEnabled || adapter->SecInfo.WPA2Enabled) {
        dwrq->flags &= ~IW_ENCODE_DISABLED;
    } else {
        dwrq->flags |= IW_ENCODE_DISABLED;
    }

    memset(extra, 0, 16);

    if (!index) {
        /* Handle current key request   */
        if ((adapter->WepKey[adapter->CurrentWepKeyIndex].KeyLength) &&
            (adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled)) {
            index = adapter->WepKey[adapter->CurrentWepKeyIndex].KeyIndex;
            memcpy(extra, adapter->WepKey[index].KeyMaterial,
                   adapter->WepKey[index].KeyLength);
            dwrq->length = adapter->WepKey[index].KeyLength;
            /* return current key */
            dwrq->flags |= (index + 1);
            /* return WEP enabled */
            dwrq->flags &= ~IW_ENCODE_DISABLED;
        } else if ((adapter->SecInfo.WPAEnabled)
                   || (adapter->SecInfo.WPA2Enabled)
            ) {
            /* return WPA enabled */
            dwrq->flags &= ~IW_ENCODE_DISABLED;
        } else {
            dwrq->flags |= IW_ENCODE_DISABLED;
        }
    } else {
        /* Handle specific key requests */
        index--;
        if (adapter->WepKey[index].KeyLength) {
            memcpy(extra, adapter->WepKey[index].KeyMaterial,
                   adapter->WepKey[index].KeyLength);
            dwrq->length = adapter->WepKey[index].KeyLength;
            /* return current key */
            dwrq->flags |= (index + 1);
            /* return WEP enabled */
            dwrq->flags &= ~IW_ENCODE_DISABLED;
        } else if ((adapter->SecInfo.WPAEnabled)
                   || (adapter->SecInfo.WPA2Enabled)
            ) {
            /* return WPA enabled */
            dwrq->flags &= ~IW_ENCODE_DISABLED;
        } else {
            dwrq->flags |= IW_ENCODE_DISABLED;
        }
    }

    dwrq->flags |= IW_ENCODE_NOKEY;

    PRINTM(INFO, "Key:%02x:%02x:%02x:%02x:%02x:%02x KeyLen=%d\n",
           extra[0], extra[1], extra[2],
           extra[3], extra[4], extra[5], dwrq->length);

    if (adapter->EncryptionStatus == Wlan802_11Encryption2Enabled
        && !dwrq->length) {
        dwrq->length = MAX_KEY_SIZE;
    }

    PRINTM(INFO, "Return flags=0x%x\n", dwrq->flags);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get TX Power
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_txpow(struct net_device *dev,
               struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_RF_TX_POWER,
                                HostCmd_ACT_GEN_GET,
                                HostCmd_OPTION_WAITFORRSP, 0, NULL);

    if (ret) {
        LEAVE();
        return ret;
    }

    PRINTM(INFO, "TXPOWER GET %d dbm.\n", Adapter->TxPowerLevel);
    vwrq->value = Adapter->TxPowerLevel;
    vwrq->fixed = 1;
    if (Adapter->RadioOn) {
        vwrq->disabled = 0;
        vwrq->flags = IW_TXPOW_DBM;
    } else {
        vwrq->disabled = 1;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set TX Retry Count
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_retry(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *adapter = priv->adapter;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    if (vwrq->flags == IW_RETRY_LIMIT) {
        /* The MAC has a 4-bit Total_Tx_Count register
           Total_Tx_Count = 1 + Tx_Retry_Count */
#define TX_RETRY_MIN 0
#define TX_RETRY_MAX 14
        if (vwrq->value < TX_RETRY_MIN || vwrq->value > TX_RETRY_MAX)
            return -EINVAL;

        /* Set Tx retry count */
        adapter->TxRetryCount = vwrq->value;

        ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SNMP_MIB,
                                    HostCmd_ACT_SET,
                                    HostCmd_OPTION_WAITFORRSP,
                                    OID_802_11_TX_RETRYCOUNT, NULL);

        if (ret) {
            LEAVE();
            return ret;
        }
    } else {
        return -EOPNOTSUPP;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get TX Retry Count
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_retry(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    Adapter->TxRetryCount = 0;
    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_SNMP_MIB, HostCmd_ACT_GET,
                                HostCmd_OPTION_WAITFORRSP,
                                OID_802_11_TX_RETRYCOUNT, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }
    vwrq->disabled = 0;
    if (!vwrq->flags) {
        vwrq->flags = IW_RETRY_LIMIT;
        /* Get Tx retry count */
        vwrq->value = Adapter->TxRetryCount;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Sort Channels
 *   
 *  @param freq 		A pointer to iw_freq structure
 *  @param num		        number of Channels
 *  @return 	   		NA
 */
static inline void
sort_channels(struct iw_freq *freq, int num)
{
    int i, j;
    struct iw_freq temp;

    for (i = 0; i < num; i++)
        for (j = i + 1; j < num; j++)
            if (freq[i].i > freq[j].i) {
                temp.i = freq[i].i;
                temp.m = freq[i].m;

                freq[i].i = freq[j].i;
                freq[i].m = freq[j].m;

                freq[j].i = temp.i;
                freq[j].m = temp.m;
            }
}

/* data rate listing
	MULTI_BANDS:
		abg		a	b	b/g
   Infra 	G(12)		A(8)	B(4)	G(12)
   Adhoc 	A+B(12)		A(8)	B(4)	B(4)

	non-MULTI_BANDS:
		   		 	b	b/g
   Infra 	     		    	B(4)	G(12)
   Adhoc 	      		    	B(4)	B(4)
 */
/** 
 *  @brief Get Range Info
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_range(struct net_device *dev, struct iw_request_info *info,
               struct iw_point *dwrq, char *extra)
{
    int i, j;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    struct iw_range *range = (struct iw_range *) extra;
    CHANNEL_FREQ_POWER *cfp;
    WLAN_802_11_RATES rates;

    ENTER();

#ifdef MOTO_DBG
    PRINTM(MOTO_DEBUG,
           "SIOCGIWRANGE (wlan_get_range) ioctl called by upper layer.\n");
#endif

    dwrq->length = sizeof(struct iw_range);
    memset(range, 0, sizeof(struct iw_range));

    range->min_nwid = 0;
    range->max_nwid = 0;

    memset(rates, 0, sizeof(rates));
    range->num_bitrates = get_active_data_rates(Adapter, rates);
    if (range->num_bitrates > sizeof(rates))
        range->num_bitrates = sizeof(rates);

    for (i = 0; i < MIN(range->num_bitrates, IW_MAX_BITRATES) && rates[i];
         i++) {
        range->bitrate[i] = (rates[i] & 0x7f) * 500000;
    }
    range->num_bitrates = i;
    PRINTM(INFO, "IW_MAX_BITRATES=%d num_bitrates=%d\n", IW_MAX_BITRATES,
           range->num_bitrates);

    range->num_frequency = 0;
    if (wlan_get_state_11d(priv) == ENABLE_11D &&
        Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        u8 chan_no;
        u8 band;

        parsed_region_chan_11d_t *parsed_region_chan =
            &Adapter->parsed_region_chan;

        band = parsed_region_chan->band;
        PRINTM(INFO, "band=%d NoOfChan=%d\n", band,
               parsed_region_chan->NoOfChan);

        for (i = 0; (range->num_frequency < IW_MAX_FREQUENCIES)
             && (i < parsed_region_chan->NoOfChan); i++) {
            chan_no = parsed_region_chan->chanPwr[i].chan;
            PRINTM(INFO, "chan_no=%d\n", chan_no);
            range->freq[range->num_frequency].i = (long) chan_no;
            range->freq[range->num_frequency].m =
                (long) chan_2_freq(chan_no, band) * 100000;
            range->freq[range->num_frequency].e = 1;
            range->num_frequency++;
        }
    } else {
        for (j = 0; (range->num_frequency < IW_MAX_FREQUENCIES)
             && (j < sizeof(Adapter->region_channel)
                 / sizeof(Adapter->region_channel[0])); j++) {
            cfp = Adapter->region_channel[j].CFP;
            for (i = 0; (range->num_frequency < IW_MAX_FREQUENCIES)
                 && Adapter->region_channel[j].Valid
                 && cfp && (i < Adapter->region_channel[j].NrCFP); i++) {
                range->freq[range->num_frequency].i = (long) cfp->Channel;
                range->freq[range->num_frequency].m =
                    (long) cfp->Freq * 100000;
                range->freq[range->num_frequency].e = 1;
                cfp++;
                range->num_frequency++;
            }
        }
    }

    PRINTM(INFO, "IW_MAX_FREQUENCIES=%d num_frequency=%d\n",
           IW_MAX_FREQUENCIES, range->num_frequency);

    range->num_channels = range->num_frequency;

    sort_channels(&range->freq[0], range->num_frequency);

    /*
     * Set an indication of the max TCP throughput in bit/s that we can
     * expect using this interface 
     */
    if (i > 2)
        range->throughput = 5000 * 1000;
    else
        range->throughput = 1500 * 1000;

    range->min_rts = MRVDRV_RTS_MIN_VALUE;
    range->max_rts = MRVDRV_RTS_MAX_VALUE;
    range->min_frag = MRVDRV_FRAG_MIN_VALUE;
    range->max_frag = MRVDRV_FRAG_MAX_VALUE;

    range->encoding_size[0] = 5;
    range->encoding_size[1] = 13;
    range->num_encoding_sizes = 2;
    range->max_encoding_tokens = 4;

    range->min_pmp = 1000000;
    range->max_pmp = 120000000;
    range->min_pmt = 1000;
    range->max_pmt = 1000000;
    range->pmp_flags = IW_POWER_PERIOD;
    range->pmt_flags = IW_POWER_TIMEOUT;
    range->pm_capa = IW_POWER_PERIOD | IW_POWER_TIMEOUT | IW_POWER_ALL_R;

    /*
     * Minimum version we recommend 
     */
    range->we_version_source = 15;

    /*
     * Version we are compiled with 
     */
    range->we_version_compiled = WIRELESS_EXT;

    range->retry_capa = IW_RETRY_LIMIT;
    range->retry_flags = IW_RETRY_LIMIT | IW_RETRY_MAX;

    range->min_retry = TX_RETRY_MIN;
    range->max_retry = TX_RETRY_MAX;

    /*
     * Set the qual, level and noise range values 
     */
    /*
     * need to put the right values here 
     */
    range->max_qual.qual = 10;
    range->max_qual.level = 0;
    range->max_qual.noise = 0;
    range->sensitivity = 0;

    /*
     * Setup the supported power level ranges 
     */
    memset(range->txpower, 0, sizeof(range->txpower));
    range->txpower[0] = Adapter->MinTxPowerLevel;
    range->txpower[1] = Adapter->MaxTxPowerLevel;
    range->num_txpower = 2;
    range->txpower_capa = IW_TXPOW_DBM | IW_TXPOW_RANGE;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Set power management 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_power(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    /* PS is currently supported only in Infrastructure Mode 
     * Remove this check if it is to be supported in IBSS mode also 
     */

    if (vwrq->disabled) {
        Adapter->PSMode = Wlan802_11PowerModeCAM;
        if (Adapter->PSState != PS_STATE_FULL_POWER) {
            PSWakeup(priv, HostCmd_OPTION_WAITFORRSP);
        }

        return 0;
    }

    if ((vwrq->flags & IW_POWER_TYPE) == IW_POWER_TIMEOUT) {
        PRINTM(INFO, "Setting power timeout command is not supported\n");
        return -EINVAL;
    } else if ((vwrq->flags & IW_POWER_TYPE) == IW_POWER_PERIOD) {
        PRINTM(INFO, "Setting power period command is not supported\n");
        return -EINVAL;
    }

    if (Adapter->PSMode != Wlan802_11PowerModeCAM) {
        return WLAN_STATUS_SUCCESS;
    }

    Adapter->PSMode = Wlan802_11PowerModeMAX_PSP;

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        PSSleep(priv, HostCmd_OPTION_WAITFORRSP);
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Get power management 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_power(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    int mode;

    ENTER();

    mode = Adapter->PSMode;

    if ((vwrq->disabled = (mode == Wlan802_11PowerModeCAM))
        || Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    vwrq->value = 0;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Set sensitivity threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_sens(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    ENTER();
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Get sensitivity threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_FAILURE
 */
static int
wlan_get_sens(struct net_device *dev,
              struct iw_request_info *info, struct iw_param *vwrq,
              char *extra)
{
    ENTER();
    LEAVE();
    return WLAN_STATUS_FAILURE;
}

#if (WIRELESS_EXT >= 18)
/** 
 *  @brief  Set IE 
 *   
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param dwrq         A pointer to iw_param structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_gen_ie(struct net_device *dev,
                struct iw_request_info *info,
                struct iw_param *dwrq, char *extra)
{
    /*
     ** Functionality to be added later
     */
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Get IE 
 *   
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param dwrq         A pointer to iw_param structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_gen_ie(struct net_device *dev,
                struct iw_request_info *info,
                struct iw_point *dwrq, char *extra)
{
    /*
     ** Functionality to be added later
     */
    return WLAN_STATUS_SUCCESS;
}
#endif

/** 
 *  @brief  Append/Reset IE buffer. 
 *   
 *  Pass an opaque block of data, expected to be IEEE IEs, to the driver 
 *    for eventual passthrough to the firmware in an associate/join 
 *    (and potentially start) command.
 *
 *  Data is appended to an existing buffer and then wrapped in a passthrough
 *    TLV in the command API to the firmware.  The firmware treats the data
 *    as a transparent passthrough to the transmitted management frame.
 *
 *  @param dev          A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure    
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_gen_ie_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    /* If the passed length is zero, reset the buffer */
    if (wrq->u.data.length == 0) {
        Adapter->genIeBufferLen = 0;
    } else {
        /* 
         * Verify that the passed length is not larger than the available 
         *   space remaining in the buffer
         */
        if (wrq->u.data.length <
            (sizeof(Adapter->genIeBuffer) - Adapter->genIeBufferLen)) {
            /* Append the passed data to the end of the genIeBuffer */
            if (copy_from_user(Adapter->genIeBuffer + Adapter->genIeBufferLen,
                               wrq->u.data.pointer, wrq->u.data.length)) {
                PRINTM(INFO, "Copy from user failed\n");
                return -EFAULT;
            }

            /* Increment the stored buffer length by the size passed */
            Adapter->genIeBufferLen += wrq->u.data.length;
        } else {
            /* Passed data does not fit in the remaining buffer space */
            ret = WLAN_STATUS_FAILURE;
        }
    }

    /* Return 0 for success, -1 for error case */
    return ret;
}

/** 
 *  @brief  Get IE buffer from driver 
 *   
 *  Used to pass an opaque block of data, expected to be IEEE IEs,
 *    back to the application.  Currently the data block passed
 *    back to the application is the saved association response retrieved 
 *    from the firmware.
 *
 *  @param priv          A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure    
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_gen_ie_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int copySize = 0;
    wlan_adapter *Adapter = priv->adapter;
    IEEEtypes_AssocRsp_t *pAssocRsp;

    pAssocRsp = (IEEEtypes_AssocRsp_t *) Adapter->assocRspBuffer;

    /*
     * Set the amount to copy back to the application as the minimum of the 
     *       available IE data or the buffer provided by the application
     */
    copySize = (Adapter->assocRspSize - sizeof(pAssocRsp->Capability) -
                -sizeof(pAssocRsp->StatusCode) - sizeof(pAssocRsp->AId));
    copySize = MIN(copySize, wrq->u.data.length);

    /* Copy the IEEE TLVs in the assoc response back to the application */
    if (copy_to_user
        (wrq->u.data.pointer, (u8 *) pAssocRsp->IEBuffer, copySize)) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }

    /* Returned copy length */
    wrq->u.data.length = copySize;

    /* No error on return */
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Set Reassociate Info
 *   
 *  @param dev          A pointer to net_device structure
 *  @param wrq          A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
wlan_reassociate_ioctl(struct net_device *dev, struct iwreq *wrq)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    wlan_ioctl_reassociation_info reassocInfo;
    u8 zeroMac[] = { 0, 0, 0, 0, 0, 0 };
    struct sockaddr bssidSockAddr;
    struct iw_point dwrq;

    int attemptBSSID = 0;
    int attemptSSID = 0;
    int retcode = 0;

    ENTER();

    Adapter->reassocAttempt = TRUE;

    /* Fail if the data passed is not what we expect */
    if (wrq->u.data.length != sizeof(reassocInfo)) {
        retcode = -EINVAL;
    } else if (copy_from_user(&reassocInfo, wrq->u.data.pointer,
                              sizeof(reassocInfo)) != 0) {
        /* copy_from_user failed  */
        PRINTM(INFO, "Reassoc: copy from user failed\n");
        retcode = -EFAULT;
    } else {
        /*
         **  Copy the passed CurrentBSSID into the driver adapter structure.
         **    This is later used in a TLV in the associate command to 
         **    trigger the reassociation in firmware.
         */
        memcpy(Adapter->reassocCurrentAp, reassocInfo.CurrentBSSID,
               sizeof(Adapter->reassocCurrentAp));

        /* Check if the desired BSSID is non-zero */
        if (memcmp(reassocInfo.DesiredBSSID, zeroMac, sizeof(zeroMac)) != 0) {
            /* Set a flag for later processing */
            attemptBSSID = TRUE;
        }

        /* Check if the desired SSID is set */
        if (reassocInfo.DesiredSSID[0]) {
            /* Make sure there is a null at the end of the SSID string */
            reassocInfo.DesiredSSID[sizeof(reassocInfo.DesiredSSID) - 1] = 0;

            /* Set a flag for later processing */
            attemptSSID = TRUE;
        }

        /* 
         **  Debug prints
         */
        PRINTM(INFO, "Reassoc: BSSID(%d), SSID(%d)\n",
               attemptBSSID, attemptSSID);
        PRINTM(INFO, "Reassoc: CBSSID(%02x:%02x:%02x:%02x:%02x:%02x)\n",
               reassocInfo.CurrentBSSID[0],
               reassocInfo.CurrentBSSID[1],
               reassocInfo.CurrentBSSID[2],
               reassocInfo.CurrentBSSID[3],
               reassocInfo.CurrentBSSID[4], reassocInfo.CurrentBSSID[5]);
        PRINTM(INFO, "Reassoc: DBSSID(%02x:%02x:%02x:%02x:%02x:%02x)\n",
               reassocInfo.DesiredBSSID[0],
               reassocInfo.DesiredBSSID[1],
               reassocInfo.DesiredBSSID[2],
               reassocInfo.DesiredBSSID[3],
               reassocInfo.DesiredBSSID[4], reassocInfo.DesiredBSSID[5]);
        PRINTM(INFO, "Reassoc: DSSID = %s\n", reassocInfo.DesiredSSID);

        /*
         **  If a BSSID was provided in the reassociation request, attempt
         **    to reassociate to it first 
         */
        if (attemptBSSID) {
            memset(bssidSockAddr.sa_data, 0x00,
                   sizeof(bssidSockAddr.sa_data));
            memcpy(bssidSockAddr.sa_data, reassocInfo.DesiredBSSID,
                   sizeof(reassocInfo.DesiredBSSID));
            bssidSockAddr.sa_family = ARPHRD_ETHER;

            /* Attempt to reassociate to the BSSID passed */
            retcode = wlan_set_wap(dev, NULL, &bssidSockAddr, NULL);
        }

        /*
         **  If a SSID was provided, and either the BSSID failed or a BSSID
         **    was not provided, attemtp to reassociate to the SSID
         */
        if (attemptSSID && (retcode != 0 || !attemptBSSID)) {
            reassocInfo.DesiredSSID[sizeof(reassocInfo.DesiredSSID) - 1] = 0;
            dwrq.length = strlen(reassocInfo.DesiredSSID) + 1;
            dwrq.flags = 1;     /* set for a specific association */

            /* Attempt to reassociate to the SSID passed */
            retcode = wlan_set_essid(dev, NULL, &dwrq,
                                     reassocInfo.DesiredSSID);
        }

        /* Reset the adapter control flag and current AP to zero */
        memset(Adapter->reassocCurrentAp, 0x00,
               sizeof(Adapter->reassocCurrentAp));
        Adapter->reassocAttempt = FALSE;
    }

    return retcode;
}

#define WILDCARD_CHAR		'*'
/** 
 *  @brief  Set wildcard ssid buffer to driver 
 *   
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure    
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_setwildcardssid(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    char buf[64];
    char ssid[32];
    int ssidlen = 0;

    memset(buf, 0, sizeof(buf));
    memset(ssid, 0, sizeof(ssid));
    if (wrq->u.data.length > 0) {
        if (copy_from_user(buf, wrq->u.data.pointer,
                           MIN(sizeof(buf) - 1, wrq->u.data.length))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
        sscanf(buf, "%32s %d", ssid, &ssidlen);
        if ((strlen(ssid) > WLAN_MAX_SSID_LENGTH) ||
            (ssidlen && (ssidlen < strlen(ssid))))
            return -EINVAL;
        //pass max length of wildcard ssid (<=32) or wildcard char to firmware
        if (ssidlen == 0)
            ssidlen = (int) WILDCARD_CHAR;
        memset(&Adapter->wildcardssid, 0, sizeof(Adapter->wildcardssid));
        memcpy(Adapter->wildcardssid.Ssid, ssid, strlen(ssid));
        Adapter->wildcardssid.SsidLength = strlen(ssid);
        Adapter->wildcardssidlen = ssidlen;
    } else {
        memset(&Adapter->wildcardssid, 0, sizeof(Adapter->wildcardssid));
        Adapter->wildcardssidlen = 0;
    }
    PRINTM(INFO, "ssid=%s, len=%d, wildcardlen=%d\n",
           Adapter->wildcardssid.Ssid, Adapter->wildcardssid.SsidLength,
           Adapter->wildcardssidlen);
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Get wildcard ssid buffer from driver 
 *   
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure    
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_getwildcardssid(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    char buf[64];

    memset(buf, 0, sizeof(buf));
    if (Adapter->wildcardssidlen == (u8) WILDCARD_CHAR)
        sprintf(buf, "%s", Adapter->wildcardssid.Ssid);
    else
        sprintf(buf, "%s %d", Adapter->wildcardssid.Ssid,
                Adapter->wildcardssidlen);
    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &buf, strlen(buf)))
            return -EFAULT;
        wrq->u.data.length = strlen(buf);
    }
    return WLAN_STATUS_SUCCESS;
}

/*
 * iwconfig settable callbacks 
 */
static const iw_handler wlan_handler[] = {
    (iw_handler) wlan_config_commit,    /* SIOCSIWCOMMIT */
    (iw_handler) wlan_get_name, /* SIOCGIWNAME */
    (iw_handler) NULL,          /* SIOCSIWNWID */
    (iw_handler) NULL,          /* SIOCGIWNWID */
    (iw_handler) wlan_set_freq, /* SIOCSIWFREQ */
    (iw_handler) wlan_get_freq, /* SIOCGIWFREQ */
    (iw_handler) wlan_set_mode, /* SIOCSIWMODE */
    (iw_handler) wlan_get_mode, /* SIOCGIWMODE */
    (iw_handler) wlan_set_sens, /* SIOCSIWSENS */
    (iw_handler) wlan_get_sens, /* SIOCGIWSENS */
    (iw_handler) NULL,          /* SIOCSIWRANGE */
    (iw_handler) wlan_get_range,        /* SIOCGIWRANGE */
    (iw_handler) NULL,          /* SIOCSIWPRIV */
    (iw_handler) NULL,          /* SIOCGIWPRIV */
    (iw_handler) NULL,          /* SIOCSIWSTATS */
    (iw_handler) NULL,          /* SIOCGIWSTATS */
#if WIRELESS_EXT > 15
    iw_handler_set_spy,         /* SIOCSIWSPY */
    iw_handler_get_spy,         /* SIOCGIWSPY */
    iw_handler_set_thrspy,      /* SIOCSIWTHRSPY */
    iw_handler_get_thrspy,      /* SIOCGIWTHRSPY */
#else /* WIRELESS_EXT > 15 */
#ifdef WIRELESS_SPY
    (iw_handler) wlan_set_spy,  /* SIOCSIWSPY */
    (iw_handler) wlan_get_spy,  /* SIOCGIWSPY */
#else /* WIRELESS_SPY */
    (iw_handler) NULL,          /* SIOCSIWSPY */
    (iw_handler) NULL,          /* SIOCGIWSPY */
#endif /* WIRELESS_SPY */
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) NULL,          /* -- hole -- */
#endif /* WIRELESS_EXT > 15 */
    (iw_handler) wlan_set_wap,  /* SIOCSIWAP */
    (iw_handler) wlan_get_wap,  /* SIOCGIWAP */
    (iw_handler) NULL,          /* -- hole -- */
    //(iw_handler) wlan_get_aplist,         /* SIOCGIWAPLIST */
    NULL,                       /* SIOCGIWAPLIST */
#if WIRELESS_EXT > 13
    (iw_handler) wlan_set_scan, /* SIOCSIWSCAN */
    (iw_handler) wlan_get_scan, /* SIOCGIWSCAN */
#else /* WIRELESS_EXT > 13 */
    (iw_handler) NULL,          /* SIOCSIWSCAN */
    (iw_handler) NULL,          /* SIOCGIWSCAN */
#endif /* WIRELESS_EXT > 13 */
    (iw_handler) wlan_set_essid,        /* SIOCSIWESSID */
    (iw_handler) wlan_get_essid,        /* SIOCGIWESSID */
    (iw_handler) wlan_set_nick, /* SIOCSIWNICKN */
    (iw_handler) wlan_get_nick, /* SIOCGIWNICKN */
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) wlan_set_rate, /* SIOCSIWRATE */
    (iw_handler) wlan_get_rate, /* SIOCGIWRATE */
    (iw_handler) wlan_set_rts,  /* SIOCSIWRTS */
    (iw_handler) wlan_get_rts,  /* SIOCGIWRTS */
    (iw_handler) wlan_set_frag, /* SIOCSIWFRAG */
    (iw_handler) wlan_get_frag, /* SIOCGIWFRAG */
    (iw_handler) wlan_set_txpow,        /* SIOCSIWTXPOW */
    (iw_handler) wlan_get_txpow,        /* SIOCGIWTXPOW */
    (iw_handler) wlan_set_retry,        /* SIOCSIWRETRY */
    (iw_handler) wlan_get_retry,        /* SIOCGIWRETRY */
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) wlan_get_encode,       /* SIOCGIWENCODE */
    (iw_handler) wlan_set_power,        /* SIOCSIWPOWER */
    (iw_handler) wlan_get_power,        /* SIOCGIWPOWER */
#if (WIRELESS_EXT >= 18)
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) wlan_set_gen_ie,       /* SIOCSIWGENIE */
    (iw_handler) wlan_get_gen_ie,       /* SIOCGIWGENIE */
#endif /* WIRELESSS_EXT >= 18 */
};

/*
 * iwpriv settable callbacks 
 */

static const iw_handler wlan_private_handler[] = {
    NULL,                       /* SIOCIWFIRSTPRIV */
};

static const struct iw_priv_args wlan_private_args[] = {
    /*
     * { cmd, set_args, get_args, name } 
     */
    {
     WLANEXTSCAN,
     IW_PRIV_TYPE_INT,
     IW_PRIV_TYPE_CHAR | 2,
     "extscan"},
    {
     WLANHOSTCMD,
     IW_PRIV_TYPE_BYTE | 2047,
     IW_PRIV_TYPE_BYTE | 2047,
     "hostcmd"},
    {
     WLANARPFILTER,
     IW_PRIV_TYPE_BYTE | 2047,
     IW_PRIV_TYPE_BYTE | 2047,
     "arpfilter"},
    {
     WLANREGRDWR,
     IW_PRIV_TYPE_CHAR | 256,
     IW_PRIV_TYPE_CHAR | 256,
     "regrdwr"},
    {
     WLANCMD52RDWR,
     IW_PRIV_TYPE_BYTE | 7,
     IW_PRIV_TYPE_BYTE | 7,
     "sdcmd52rw"},
    {
     WLANCMD53RDWR,
     IW_PRIV_TYPE_CHAR | 32,
     IW_PRIV_TYPE_CHAR | 32,
     "sdcmd53rw"},
    {
     WLAN_SETCONF_GETCONF,
     IW_PRIV_TYPE_BYTE | 1280,
     IW_PRIV_TYPE_BYTE | 1280,
     "setgetconf"},
    {
     WLANCISDUMP,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_BYTE | 512,
     "getcis"},

    {
     WLANSCAN_TYPE,
     IW_PRIV_TYPE_CHAR | 8,
     IW_PRIV_TYPE_CHAR | 8,
     "scantype"},

    {
     WLAN_SETINT_GETINT,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     ""},
    {
     WLANNF,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getNF"},
    {
     WLANRSSI,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getRSSI"},
    {
     WLANBGSCAN,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "bgscan"},
    {
     WLANENABLE11D,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "enable11d"},
    {
     WLANADHOCGRATE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "adhocgrate"},
    {
     WLANSDIOCLOCK,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "sdioclock"},
    {
     WLANWMM_ENABLE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "wmm"},
    {
     WLANNULLGEN,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "uapsdnullgen"},

    {
     WLAN_SUBCMD_SET_PRESCAN,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "prescan"},
    {
     WLANADHOCCSET,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "setcoalescing"},
#ifdef WPRM_DRV
    {
     WLANMOTOTM,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "mototm"},
#endif /* WPRM_DRV */
    {
     WLAN_SETONEINT_GETONEINT,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     ""},
    {
     WLAN_WMM_QOSINFO,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "wmm_qosinfo"},
    {
     WLAN_LISTENINTRVL,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "lolisteninter"},
    {
     WLAN_FW_WAKEUP_METHOD,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "fwwakeupmethod"},
    {
     WLAN_TXCONTROL,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "txcontrol"},
    {
     WLAN_NULLPKTINTERVAL,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "psnullinterval"},
    {
     WLAN_BCN_MISS_TIMEOUT,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "bcnmisto"},
    {
     WLAN_ADHOC_AWAKE_PERIOD,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "adhocawakepd"},
    {
     WLAN_LDO,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "ldocfg"},
    {
     WLAN_SDIO_MODE,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "sdiomode"},
    /* Using iwpriv sub-command feature */
    {
     WLAN_SETONEINT_GETNONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     ""},

    {
     WLAN_SUBCMD_SETRXANTENNA,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setrxant"},
    {
     WLAN_SUBCMD_SETTXANTENNA,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "settxant"},
    {
     WLANSETAUTHALG,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "authalgs",
     },
    {
     WLANSETENCRYPTIONMODE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "encryptionmode",
     },
    {
     WLANSETREGION,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setregioncode"},
    {
     WLAN_SET_LISTEN_INTERVAL,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setlisteninter"},
    {
     WLAN_SET_MULTIPLE_DTIM,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setmultipledtim"},
    {
     WLANSETBCNAVG,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setbcnavg"},
    {
     WLANSETDATAAVG,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setdataavg"},
    {
     WLAN_SETNONE_GETONEINT,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     ""},
    {
     WLANGETREGION,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getregioncode"},
    {
     WLAN_GET_LISTEN_INTERVAL,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getlisteninter"},
    {
     WLAN_GET_MULTIPLE_DTIM,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getmultipledtim"},
    {
     WLAN_GET_TX_RATE,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "gettxrate"},
    {
     WLANGETBCNAVG,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getbcnavg"},
    {
     WLANGETDATAAVG,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getdataavg"},
    {
     WLAN_SETNONE_GETTWELVE_CHAR,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 12,
     ""},
    {
     WLAN_SUBCMD_GETRXANTENNA,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 12,
     "getrxant"},
    {
     WLAN_SUBCMD_GETTXANTENNA,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 12,
     "gettxant"},
    {
     WLAN_GET_TSF,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 12,
     "gettsf"},
    {
     WLANDEEPSLEEP,
     IW_PRIV_TYPE_CHAR | 1,
     IW_PRIV_TYPE_CHAR | 6,
     "deepsleep"},
    {
     WLANHOSTSLEEPCFG,
     IW_PRIV_TYPE_CHAR | 31,
     IW_PRIV_TYPE_NONE,
     "hostsleepcfg"},
    {
     WLAN_SETNONE_GETNONE,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     ""},
    {
     WLANDEAUTH,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "deauth"},
    {
     WLANADHOCSTOP,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "adhocstop"},
    {
     WLANRADIOON,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "radioon"},
    {
     WLANRADIOOFF,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "radiooff"},
    {
     WLANREMOVEADHOCAES,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "rmaeskey"},
#ifdef REASSOCIATION
    {
     WLANREASSOCIATIONAUTO,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "reasso-on"},
    {
     WLANREASSOCIATIONUSER,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "reasso-off"},
#endif /* REASSOCIATION */
    {
     WLANWLANIDLEON,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "wlanidle-on"},
    {
     WLANWLANIDLEOFF,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "wlanidle-off"},
    {
     WLAN_SET64CHAR_GET64CHAR,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     ""},
    {
     WLANSLEEPPARAMS,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "sleepparams"},

    {
     WLAN_BCA_TIMESHARE,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "bca-ts"},
    {
     WLANSCAN_MODE,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "scanmode"},
    {
     WLAN_GET_ADHOC_STATUS,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "getadhocstatus"},
    {
     SET_WILDCARD_SSID,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "setwcssid"},
    {
     GET_WILDCARD_SSID,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "getwcssid"},
    {
     WLAN_SET_GEN_IE,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "setgenie"},
    {
     WLAN_GET_GEN_IE,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "getgenie"},
    {
     WLAN_REASSOCIATE,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "reassociate"},
    {
     WLAN_WMM_QUEUE_STATUS,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "qstatus"},
    {
     WLAN_SETWORDCHAR_GETNONE,
     IW_PRIV_TYPE_CHAR | 32,
     IW_PRIV_TYPE_NONE,
     ""},
    {
     WLANSETADHOCAES,
     IW_PRIV_TYPE_CHAR | 32,
     IW_PRIV_TYPE_NONE,
     "setaeskey"},
    {
     WLAN_SETNONE_GETWORDCHAR,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 128,
     ""},
    {
     WLANGETADHOCAES,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 128,
     "getaeskey"},
    {
     WLANVERSION,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 128,
     "version"},
    {
     WLANSETWPAIE,
     IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | 24,
     IW_PRIV_TYPE_NONE,
     "setwpaie"},
    {
     WLANGETLOG,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | GETLOG_BUFSIZE,
     "getlog"},
    {
     WLAN_SET_GET_SIXTEEN_INT,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     ""},
    {
     WLAN_TPCCFG,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "tpccfg"},
    {
     WLAN_AUTO_FREQ_SET,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "setafc"},
    {
     WLAN_AUTO_FREQ_GET,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "getafc"},
    {
     WLAN_SCANPROBES,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "scanprobes"},
    {
     WLAN_LED_GPIO_CTRL,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "ledgpio"},
    {
     WLAN_SLEEP_PERIOD,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "sleeppd"},
    {
     WLAN_ADAPT_RATESET,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "rateadapt"},

    {
     WLAN_INACTIVITY_TIMEOUT,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "inactivityto"},
    {
     WLANSNR,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "getSNR"},
    {
     WLAN_GET_RATE,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "getrate"},
    {
     WLAN_GET_RXINFO,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "getrxinfo"},
    {
     WLAN_SET_ATIM_WINDOW,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "atimwindow"},
    {
     WLAN_BEACON_INTERVAL,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "bcninterval"},
    {
     WLAN_SCAN_TIME,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "scantime"},
    {
     WLAN_DATA_SUBSCRIBE_EVENT,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "dataevtcfg"},
    {
     WLANHSCFG,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "hscfg"},
    {
     WLAN_INACTIVITY_TIMEOUT_EXT,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "inactoext"},
#ifdef DEBUG_LEVEL1
    {
     WLAN_DRV_DBG,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "drvdbg"},
#endif
#ifdef WPRM_DRV
    {
     WLAN_SESSIONTYPE,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "sessiontype"},
#endif /* WPRM_DRV */
    {
     WLAN_SET_GET_2K,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     ""},

    {
     WLAN_SET_USER_SCAN,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "setuserscan"},
    {
     WLAN_GET_SCAN_TABLE,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "getscantable"},
    {
     WLAN_SET_MRVL_TLV,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "setmrvltlv"},
    {
     WLAN_GET_ASSOC_RSP,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "getassocrsp"},
    {
     WLAN_ADDTS_REQ,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "addts"},
    {
     WLAN_DELTS_REQ,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "delts"},
    {
     WLAN_QUEUE_CONFIG,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "qconfig"},
    {
     WLAN_QUEUE_STATS,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "qstats"},
    {
     WLAN_GET_CFP_TABLE,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "getcfptable"},
};

struct iw_handler_def wlan_handler_def = {
  num_standard:sizeof(wlan_handler) / sizeof(iw_handler),
  num_private:sizeof(wlan_private_handler) / sizeof(iw_handler),
  num_private_args:sizeof(wlan_private_args) /
        sizeof(struct iw_priv_args),
  standard:(iw_handler *) wlan_handler,
  private:(iw_handler *) wlan_private_handler,
  private_args:(struct iw_priv_args *) wlan_private_args,
};

/** 
 *  @brief wlan hostcmd ioctl handler
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param req		        A pointer to ifreq structure
 *  @param cmd			command 
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_hostcmd_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    u8 *tempResponseBuffer;
    CmdCtrlNode *pCmdNode;
    HostCmd_DS_GEN *pCmdPtr;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    u16 wait_option = 0;
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    if ((wrq->u.data.pointer == NULL) || (wrq->u.data.length < S_DS_GEN)) {
        PRINTM(INFO,
               "wlan_hostcmd_ioctl() corrupt data: pointer=%p, length=%d\n",
               wrq->u.data.pointer, wrq->u.data.length);
        return -EFAULT;
    }

    /*
     * Get a free command control node 
     */
    if (!(pCmdNode = GetFreeCmdCtrlNode(priv))) {
        PRINTM(INFO, "Failed GetFreeCmdCtrlNode\n");
        return -ENOMEM;
    }

    if (!
        (tempResponseBuffer =
         kmalloc(MRVDRV_SIZE_OF_CMD_BUFFER, GFP_KERNEL))) {
        PRINTM(INFO, "ERROR: Failed to allocate response buffer!\n");
        CleanupAndInsertCmd(priv, pCmdNode);
        return -ENOMEM;
    }

    wait_option |= HostCmd_OPTION_WAITFORRSP;

    SetCmdCtrlNode(priv, pCmdNode, 0, wait_option, NULL);
    init_waitqueue_head(&pCmdNode->cmdwait_q);

    pCmdPtr = (HostCmd_DS_GEN *) pCmdNode->BufVirtualAddr;

    /*
     * Copy the whole command into the command buffer 
     */
    if (copy_from_user(pCmdPtr, wrq->u.data.pointer, wrq->u.data.length)) {
        PRINTM(INFO, "Copy from user failed\n");
        kfree(tempResponseBuffer);
        CleanupAndInsertCmd(priv, pCmdNode);
        return -EFAULT;
    }

    if (pCmdPtr->Size < S_DS_GEN) {
        PRINTM(INFO, "wlan_hostcmd_ioctl() invalid cmd size: Size=%d\n",
               pCmdPtr->Size);
        kfree(tempResponseBuffer);
        CleanupAndInsertCmd(priv, pCmdNode);
        return -EFAULT;
    }

    pCmdNode->pdata_buf = tempResponseBuffer;
    pCmdNode->CmdFlags |= CMD_F_HOSTCMD;

    pCmdPtr->SeqNum = ++priv->adapter->SeqNum;
    pCmdPtr->Result = 0;

    PRINTM(INFO, "HOSTCMD Command: 0x%04x Size: %d SeqNum: %d\n",
           pCmdPtr->Command, pCmdPtr->Size, pCmdPtr->SeqNum);
    HEXDUMP("Command Data", (u8 *) (pCmdPtr), MIN(32, pCmdPtr->Size));
    PRINTM(INFO, "Copying data from : (user)0x%p -> 0x%p(driver)\n",
           req->ifr_data, pCmdPtr);

    pCmdNode->CmdWaitQWoken = FALSE;
    QueueCmd(Adapter, pCmdNode, TRUE);
    wake_up_interruptible(&priv->MainThread.waitQ);

    if (wait_option & HostCmd_OPTION_WAITFORRSP) {
        /* Sleep until response is generated by FW */
        wait_event_interruptible(pCmdNode->cmdwait_q,
                                 pCmdNode->CmdWaitQWoken);
    }

    /* Copy the response back to user space */
    pCmdPtr = (HostCmd_DS_GEN *) tempResponseBuffer;

    if (copy_to_user(wrq->u.data.pointer, tempResponseBuffer, pCmdPtr->Size))
        PRINTM(INFO, "ERROR: copy_to_user failed!\n");
    wrq->u.data.length = pCmdPtr->Size;
    kfree(tempResponseBuffer);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief wlan arpfilter ioctl handler
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param req		        A pointer to ifreq structure
 *  @param cmd			command 
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_arpfilter_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    wlan_private *priv = dev->priv;
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;
    MrvlIEtypesHeader_t hdr;

    ENTER();

    if ((wrq->u.data.pointer == NULL)
        || (wrq->u.data.length < sizeof(MrvlIEtypesHeader_t))
        || (wrq->u.data.length > sizeof(Adapter->ArpFilter))) {
        PRINTM(INFO,
               "wlan_arpfilter_ioctl() corrupt data: pointer=%p, length=%d\n",
               wrq->u.data.pointer, wrq->u.data.length);
        return -EFAULT;
    }

    if (copy_from_user(&hdr, wrq->u.data.pointer, sizeof(hdr))) {
        PRINTM(INFO, "Copy from user failed\n");
        return -EFAULT;
    }

    if (hdr.Len == 0) {
        Adapter->ArpFilterSize = 0;
        memset(Adapter->ArpFilter, 0, sizeof(Adapter->ArpFilter));
    } else {
        Adapter->ArpFilterSize = wrq->u.data.length;

        PRINTM(INFO, "Copying data from : (user)0x%p -> 0x%p(driver)\n",
               wrq->u.data.pointer, Adapter->ArpFilter);
        if (copy_from_user(Adapter->ArpFilter, wrq->u.data.pointer,
                           Adapter->ArpFilterSize)) {
            Adapter->ArpFilterSize = 0;
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        HEXDUMP("ArpFilter", Adapter->ArpFilter, Adapter->ArpFilterSize);
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Rx Info 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success
 */
static int
wlan_get_rxinfo(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data[2];
    ENTER();
    data[0] = Adapter->SNR[TYPE_RXPD][TYPE_NOAVG];
    data[1] = Adapter->RxPDRate;
    if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 2)) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }
    wrq->u.data.length = 2;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get SNR 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_snr(wlan_private * priv, struct iwreq *wrq)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;
    int data[4];

    ENTER();

#ifdef MOTO_DBG
    PRINTM(MOTO_DEBUG, "getSNR ioctl called by upper layer\n");
#endif

    memset(data, 0, sizeof(data));
    if (wrq->u.data.length) {
        if (copy_from_user
            (data, wrq->u.data.pointer,
             MIN(wrq->u.data.length, 4) * sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
    }
    if ((wrq->u.data.length == 0) || (data[0] == 0) || (data[0] == 1)) {
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            ret = PrepareAndSendCommand(priv,
                                        HostCmd_CMD_802_11_RSSI,
                                        0,
                                        HostCmd_OPTION_WAITFORRSP |
                                        HostCmd_OPTION_TIMEOUT, 0, NULL);
            if (ret) {
                if (ret == WLAN_STATUS_CMD_TIME_OUT) {
                    wlan_fatal_error_handle(priv);
                }
                LEAVE();
                return ret;
            }
        }
    }

    if (wrq->u.data.length == 0) {
        data[0] = Adapter->SNR[TYPE_BEACON][TYPE_NOAVG];
        data[1] = Adapter->SNR[TYPE_BEACON][TYPE_AVG];
        if ((jiffies - Adapter->RxPDAge) > HZ)  //data expired after 1 second
            data[2] = 0;
        else
            data[2] = Adapter->SNR[TYPE_RXPD][TYPE_NOAVG];
        data[3] = Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE;
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 4)) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = 4;
    } else if (data[0] == 0) {
        data[0] = Adapter->SNR[TYPE_BEACON][TYPE_NOAVG];
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    } else if (data[0] == 1) {
        data[0] = Adapter->SNR[TYPE_BEACON][TYPE_AVG];
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    } else if (data[0] == 2) {
        if ((jiffies - Adapter->RxPDAge) > HZ)  //data expired after 1 second
            data[0] = 0;
        else
            data[0] = Adapter->SNR[TYPE_RXPD][TYPE_NOAVG];
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    } else if (data[0] == 3) {
        data[0] = Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE;
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    } else {
        return -ENOTSUPP;
    }

#ifdef MOTO_DBG
    PRINTM(MOTO_DEBUG, "getSNR ioctl end\n");
#endif

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set scan time
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wreq		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_scan_time(wlan_private * priv, struct iwreq *wrq)
{
    int data[3] = { 0, 0, 0 };
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wrq->u.data.length > 0 && wrq->u.data.length <= 3) {
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        PRINTM(INFO,
               "WLAN SET Scan Time: Specific %d, Active %d, Passive %d\n",
               data[0], data[1], data[2]);
        if (data[0]) {
            if (data[0] > MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME) {
                PRINTM(MSG,
                       "Invalid parameter, max specific scan time is %d ms\n",
                       MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME);
                return -EINVAL;
            }
            Adapter->SpecificScanTime = data[0];
        }
        if (data[1]) {
            if (data[1] > MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME) {
                PRINTM(MSG,
                       "Invalid parameter, max active scan time is %d ms\n",
                       MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME);
                return -EINVAL;
            }
            Adapter->ActiveScanTime = data[1];
        }
        if (data[2]) {
            if (data[2] > MRVDRV_MAX_PASSIVE_SCAN_CHAN_TIME) {
                PRINTM(MSG,
                       "Invalid parameter, max passive scan time is %d ms\n",
                       MRVDRV_MAX_PASSIVE_SCAN_CHAN_TIME);
                return -EINVAL;
            }
            Adapter->PassiveScanTime = data[2];
        }
    }

    data[0] = Adapter->SpecificScanTime;
    data[1] = Adapter->ActiveScanTime;
    data[2] = Adapter->PassiveScanTime;
    wrq->u.data.length = 3;
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Adhoc beacon Interval 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_beacon_interval(wlan_private * priv, struct iwreq *wrq)
{
    int data[2];
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wrq->u.data.length > 0) {
        if (copy_from_user(data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        PRINTM(INFO, "WLAN SET BEACON INTERVAL: %d\n", data[0]);
        if ((data[0] > MRVDRV_MAX_BEACON_INTERVAL) ||
            (data[0] < MRVDRV_MIN_BEACON_INTERVAL))
            return -ENOTSUPP;
        Adapter->BeaconPeriod = data[0];
    }
    data[0] = Adapter->BeaconPeriod;
    wrq->u.data.length = 1;
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        data[1] = Adapter->CurBssParams.BSSDescriptor.BeaconPeriod;
        wrq->u.data.length = 2;
    }
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Adhoc ATIM Window 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_ATIM_Window(wlan_private * priv, struct iwreq *wrq)
{
    int data[2];
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wrq->u.data.length > 0) {
        if (copy_from_user(data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        PRINTM(INFO, "WLAN SET ATIM WINDOW: %d\n", data[0]);
        Adapter->AtimWindow = data[0];
        Adapter->AtimWindow = MIN(Adapter->AtimWindow, 50);
    }
    data[0] = Adapter->AtimWindow;
    wrq->u.data.length = 1;
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        data[1] = Adapter->CurBssParams.BSSDescriptor.ATIMWindow;
        wrq->u.data.length = 2;
    }
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set data subscribe event
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wreq		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_data_subscribe_event(wlan_private * priv, struct iwreq *wrq)
{
    int data[9];
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    memset(data, 0, sizeof(data));
    if (wrq->u.data.length > 9)
        return -EFAULT;
    if (wrq->u.data.length > 0) {
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
        Adapter->subevent.EventsBitmap = (u16) data[0];
        Adapter->subevent.Rssi_low.value = (u8) data[1];
        Adapter->subevent.Rssi_low.Freq = (u8) data[2];
        Adapter->subevent.Snr_low.value = (u8) data[3];
        Adapter->subevent.Snr_low.Freq = (u8) data[4];
        Adapter->subevent.Rssi_high.value = (u8) data[5];
        Adapter->subevent.Rssi_high.Freq = (u8) data[6];
        Adapter->subevent.Snr_high.value = (u8) data[7];
        Adapter->subevent.Snr_high.Freq = (u8) data[8];
    } else {
        data[0] = (int) Adapter->subevent.EventsBitmap;
        data[1] = (int) Adapter->subevent.Rssi_low.value;
        data[2] = (int) Adapter->subevent.Rssi_low.Freq;
        data[3] = (int) Adapter->subevent.Snr_low.value;
        data[4] = (int) Adapter->subevent.Snr_low.Freq;
        data[5] = (int) Adapter->subevent.Rssi_high.value;
        data[6] = (int) Adapter->subevent.Rssi_high.Freq;
        data[7] = (int) Adapter->subevent.Snr_high.value;
        data[8] = (int) Adapter->subevent.Snr_high.Freq;
    }
    wrq->u.data.length = 9;
    if (copy_to_user(wrq->u.data.pointer, data, sizeof(data))) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get RSSI 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_rssi(wlan_private * priv, struct iwreq *wrq)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;
    int temp;
    int data = 0;
    int *val;

    ENTER();
    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    if ((data == 0) || (data == 1)) {
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_RSSI,
                                    0, HostCmd_OPTION_WAITFORRSP, 0, NULL);
        if (ret) {
            LEAVE();
            return ret;
        }
    }

    switch (data) {
    case 0:

        temp = CAL_RSSI(Adapter->SNR[TYPE_BEACON][TYPE_NOAVG],
                        Adapter->NF[TYPE_BEACON][TYPE_NOAVG]);
        break;
    case 1:
        temp = CAL_RSSI(Adapter->SNR[TYPE_BEACON][TYPE_AVG],
                        Adapter->NF[TYPE_BEACON][TYPE_AVG]);
        break;
    case 2:
        if ((jiffies - Adapter->RxPDAge) > HZ)  //data expired after 1 second
            temp = 0;
        else
            temp = CAL_RSSI(Adapter->SNR[TYPE_RXPD][TYPE_NOAVG],
                            Adapter->NF[TYPE_RXPD][TYPE_NOAVG]);
        break;
    case 3:
        temp = CAL_RSSI(Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE,
                        Adapter->NF[TYPE_RXPD][TYPE_AVG] / AVG_SCALE);
        break;
    default:
        return -ENOTSUPP;
    }
    val = (int *) wrq->u.name;
    *val = temp;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get NF
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure 
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_nf(wlan_private * priv, struct iwreq *wrq)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;
    int temp;
    int data = 0;
    int *val;

    ENTER();
    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    if ((data == 0) || (data == 1)) {
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_RSSI,
                                    0, HostCmd_OPTION_WAITFORRSP, 0, NULL);

        if (ret) {
            LEAVE();
            return ret;
        }
    }

    switch (data) {
    case 0:
        temp = Adapter->NF[TYPE_BEACON][TYPE_NOAVG];
        break;
    case 1:
        temp = Adapter->NF[TYPE_BEACON][TYPE_AVG];
        break;
    case 2:
        if ((jiffies - Adapter->RxPDAge) > HZ)  //data expired after 1 second
            temp = 0;
        else
            temp = Adapter->NF[TYPE_RXPD][TYPE_NOAVG];
        break;
    case 3:
        temp = Adapter->NF[TYPE_RXPD][TYPE_AVG] / AVG_SCALE;
        break;
    default:
        return -ENOTSUPP;
    }

    temp = CAL_NF(temp);

    PRINTM(INFO, "***temp = %d\n", temp);
    val = (int *) wrq->u.name;
    *val = temp;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Remove AES key
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_remove_aes(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    WLAN_802_11_KEY key;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (Adapter->InfrastructureMode != Wlan802_11IBSS ||
        Adapter->MediaConnectStatus == WlanMediaStateConnected)
        return -EOPNOTSUPP;

    Adapter->AdhocAESEnabled = FALSE;

    memset(&key, 0, sizeof(WLAN_802_11_KEY));
    PRINTM(INFO, "WPA2: DISABLE AES_KEY\n");
    key.KeyLength = WPA_AES_KEY_LEN;
    key.KeyIndex = 0x40000000;

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_KEY_MATERIAL,
                                HostCmd_ACT_SET,
                                HostCmd_OPTION_WAITFORRSP,
                                !(KEY_INFO_ENABLED), &key);

    LEAVE();

    return ret;
}

/** 
 *  @brief Get Support Rates
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_getrate_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    WLAN_802_11_RATES rates;
    int rate[sizeof(rates)];
    int i;

    ENTER();

    memset(rates, 0, sizeof(rates));
    memset(rate, 0, sizeof(rate));
    wrq->u.data.length = get_active_data_rates(Adapter, rates);
    if (wrq->u.data.length > sizeof(rates))
        wrq->u.data.length = sizeof(rates);

    for (i = 0; i < wrq->u.data.length; i++) {
        rates[i] &= ~0x80;
        rate[i] = rates[i];
    }

    if (copy_to_user
        (wrq->u.data.pointer, rate, wrq->u.data.length * sizeof(int))) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get TxRate
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_txrate_ioctl(wlan_private * priv, struct ifreq *req)
{
    wlan_adapter *Adapter = priv->adapter;
    int *pdata;
    struct iwreq *wrq = (struct iwreq *) req;
    int ret = WLAN_STATUS_SUCCESS;
    ENTER();
    Adapter->TxRate = 0;
    PRINTM(INFO, "wlan_get_txrate_ioctl\n");
    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_TX_RATE_QUERY,
                                HostCmd_ACT_GET, HostCmd_OPTION_WAITFORRSP,
                                0, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }
    pdata = (int *) wrq->u.name;
    *pdata = (int) Adapter->TxRate;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Adhoc Status
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_adhoc_status_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    char status[64];
    wlan_adapter *Adapter = priv->adapter;

    memset(status, 0, sizeof(status));

    switch (Adapter->InfrastructureMode) {
    case Wlan802_11IBSS:
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            if (Adapter->AdhocCreate == TRUE)
                strcpy(status, "AdhocStarted");
            else
                strcpy(status, "AdhocJoined");
        } else {
            strcpy(status, "AdhocIdle");
        }
        break;
    case Wlan802_11Infrastructure:
        strcpy(status, "InfraMode");
        break;
    default:
        strcpy(status, "AutoUnknownMode");
        break;
    }

    PRINTM(INFO, "Status = %s\n", status);
    wrq->u.data.length = strlen(status) + 1;

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &status, wrq->u.data.length))
            return -EFAULT;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Driver Version
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_version_ioctl(wlan_private * priv, struct ifreq *req)
{
    int len;
    char buf[128];
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    get_version(priv->adapter, buf, sizeof(buf) - 1);

    len = strlen(buf);
    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, buf, len)) {
            PRINTM(INFO, "CopyToUser failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = len;
    }

    PRINTM(INFO, "wlan version: %s\n", buf);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Read/Write adapter registers
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_regrdwr_ioctl(wlan_private * priv, struct ifreq *req)
{
    wlan_ioctl_regrdwr regrdwr;
    wlan_offset_value offval;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (copy_from_user(&regrdwr, req->ifr_data, sizeof(regrdwr))) {
        PRINTM(INFO,
               "copy of regrdwr for wlan_regrdwr_ioctl from user failed \n");
        LEAVE();
        return -EFAULT;
    }

    if (regrdwr.WhichReg == REG_EEPROM) {
        PRINTM(MSG, "Inside RDEEPROM\n");
        Adapter->pRdeeprom =
            (char *) kmalloc((regrdwr.NOB + sizeof(regrdwr)), GFP_KERNEL);
        if (!Adapter->pRdeeprom)
            return -ENOMEM;
        memcpy(Adapter->pRdeeprom, &regrdwr, sizeof(regrdwr));
        /* +14 is for Action, Offset, and NOB in 
         * response */
        PRINTM(INFO, "Action:%d Offset: %x NOB: %02x\n",
               regrdwr.Action, regrdwr.Offset, regrdwr.NOB);

        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_EEPROM_ACCESS,
                                    regrdwr.Action, HostCmd_OPTION_WAITFORRSP,
                                    0, &regrdwr);

        if (ret) {
            if (Adapter->pRdeeprom)
                kfree(Adapter->pRdeeprom);
            LEAVE();
            return ret;
        }

        mdelay(10);

        /*
         * Return the result back to the user 
         */

        if (regrdwr.Action == HostCmd_ACT_GEN_READ) {
            if (copy_to_user
                (req->ifr_data, Adapter->pRdeeprom,
                 sizeof(regrdwr) + regrdwr.NOB)) {
                if (Adapter->pRdeeprom)
                    kfree(Adapter->pRdeeprom);
                PRINTM(INFO,
                       "copy of regrdwr for wlan_regrdwr_ioctl to user failed \n");
                LEAVE();
                return -EFAULT;
            }
        }

        if (Adapter->pRdeeprom)
            kfree(Adapter->pRdeeprom);

        return WLAN_STATUS_SUCCESS;
    }

    offval.offset = regrdwr.Offset;
    offval.value = (regrdwr.Action) ? regrdwr.Value : 0x00;

    PRINTM(INFO, "RegAccess: %02x Action:%d "
           "Offset: %04x Value: %04x\n",
           regrdwr.WhichReg, regrdwr.Action, offval.offset, offval.value);

    /*
     * regrdwr.WhichReg should contain the command that
     * corresponds to which register access is to be 
     * performed HostCmd_CMD_MAC_REG_ACCESS 0x0019
     * HostCmd_CMD_BBP_REG_ACCESS 0x001a 
     * HostCmd_CMD_RF_REG_ACCESS 0x001b 
     */
    ret = PrepareAndSendCommand(priv, regrdwr.WhichReg,
                                regrdwr.Action, HostCmd_OPTION_WAITFORRSP,
                                0, &offval);

    if (ret) {
        LEAVE();
        return ret;
    }

    mdelay(10);

    /*
     * Return the result back to the user 
     */
    regrdwr.Value = Adapter->OffsetValue.value;
    if (regrdwr.Action == HostCmd_ACT_GEN_READ) {
        if (copy_to_user(req->ifr_data, &regrdwr, sizeof(regrdwr))) {
            PRINTM(INFO,
                   "copy of regrdwr for wlan_regrdwr_ioctl to user failed \n");
            LEAVE();
            return -EFAULT;
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Cmd52 read/write register
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_cmd52rdwr_ioctl(wlan_private * priv, struct ifreq *req)
{
    u8 buf[7];
    u8 rw, func, dat = 0xff;
    u32 reg;

    ENTER();

    if (copy_from_user(buf, req->ifr_data, sizeof(buf))) {
        PRINTM(INFO, "Copy from user failed\n");
        return -EFAULT;
    }

    rw = buf[0];
    func = buf[1];
    reg = buf[5];
    reg = (reg << 8) + buf[4];
    reg = (reg << 8) + buf[3];
    reg = (reg << 8) + buf[2];

    if (rw != 0)
        dat = buf[6];

    PRINTM(INFO, "rw=%d func=%d reg=0x%08X dat=0x%02X\n", rw, func, reg, dat);

    if (rw == 0) {
        if (sbi_read_ioreg(priv, func, reg, &dat) < 0) {
            PRINTM(INFO, "sdio_read_ioreg: reading register 0x%X failed\n",
                   reg);
            dat = 0xff;
        }
    } else {
        if (sbi_write_ioreg(priv, func, reg, dat) < 0) {
            PRINTM(INFO, "sdio_read_ioreg: writing register 0x%X failed\n",
                   reg);
            dat = 0xff;
        }
    }
    if (copy_to_user(req->ifr_data, &dat, sizeof(dat))) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Cmd53 read/write register
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_cmd53rdwr_ioctl(wlan_private * priv, struct ifreq *req)
{
    return -EINVAL;
}

/** 
 *  @brief Convert ascii string to Hex integer
 *     
 *  @param d                    A pointer to integer buf
 *  @param s			A pointer to ascii string 
 *  @param dlen			the length o fascii string
 *  @return 	   	        number of integer  
 */
static int
ascii2hex(u8 * d, char *s, u32 dlen)
{
    int i;
    u8 n;

    memset(d, 0x00, dlen);

    for (i = 0; i < dlen * 2; i++) {
        if ((s[i] >= 48) && (s[i] <= 57))
            n = s[i] - 48;
        else if ((s[i] >= 65) && (s[i] <= 70))
            n = s[i] - 55;
        else if ((s[i] >= 97) && (s[i] <= 102))
            n = s[i] - 87;
        else
            break;
        if ((i % 2) == 0)
            n = n * 16;
        d[i / 2] += n;
    }

    return i;
}

/** 
 *  @brief Set adhoc aes key
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_setadhocaes_ioctl(wlan_private * priv, struct ifreq *req)
{
    u8 key_ascii[32];
    u8 key_hex[16];
    int ret = 0;
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;

    WLAN_802_11_KEY key;

    ENTER();

    if (Adapter->InfrastructureMode != Wlan802_11IBSS)
        return -EOPNOTSUPP;

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected)
        return -EOPNOTSUPP;

    if (copy_from_user(key_ascii, wrq->u.data.pointer, sizeof(key_ascii))) {
        PRINTM(INFO, "wlan_setadhocaes_ioctl copy from user failed \n");
        LEAVE();
        return -EFAULT;
    }

    Adapter->AdhocAESEnabled = TRUE;
    ascii2hex(key_hex, key_ascii, sizeof(key_hex));

    HEXDUMP("wlan_setadhocaes_ioctl", key_hex, sizeof(key_hex));

    PRINTM(INFO, "WPA2: ENABLE AES_KEY\n");
    key.KeyLength = WPA_AES_KEY_LEN;
    key.KeyIndex = 0x40000000;
    memcpy(key.KeyMaterial, key_hex, key.KeyLength);

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_KEY_MATERIAL,
                                HostCmd_ACT_SET,
                                HostCmd_OPTION_WAITFORRSP,
                                KEY_INFO_ENABLED, &key);

    LEAVE();
    return ret;
}

/** 
 *  @brief Get adhoc aes key   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_getadhocaes_ioctl(wlan_private * priv, struct ifreq *req)
{
    u8 *tmp;
    u8 key_ascii[33];
    u8 key_hex[16];
    int i, ret = 0;
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;
    WLAN_802_11_KEY key;

    ENTER();

    memset(key_hex, 0x00, sizeof(key_hex));

    PRINTM(INFO, "WPA2: ENABLE AES_KEY\n");
    key.KeyLength = WPA_AES_KEY_LEN;
    key.KeyIndex = 0x40000000;
    memcpy(key.KeyMaterial, key_hex, key.KeyLength);

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_KEY_MATERIAL,
                                HostCmd_ACT_GET,
                                HostCmd_OPTION_WAITFORRSP,
                                KEY_INFO_ENABLED, &key);

    if (ret) {
        LEAVE();
        return ret;
    }

    memcpy(key_hex, Adapter->aeskey.KeyParamSet.Key, sizeof(key_hex));

    HEXDUMP("wlan_getadhocaes_ioctl", key_hex, sizeof(key_hex));

    wrq->u.data.length = sizeof(key_ascii) + 1;

    memset(key_ascii, 0x00, sizeof(key_ascii));
    tmp = key_ascii;

    for (i = 0; i < sizeof(key_hex); i++)
        tmp += sprintf(tmp, "%02x", key_hex[i]);

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &key_ascii, sizeof(key_ascii))) {
            PRINTM(INFO, "copy_to_user failed\n");
            return -EFAULT;
        }
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Set/Get WPA IE   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_setwpaie_ioctl(wlan_private * priv, struct ifreq *req)
{
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length) {
        if (wrq->u.data.length > sizeof(Adapter->Wpa_ie)) {
            PRINTM(INFO, "failed to copy WPA IE, too big \n");
            return -EFAULT;
        }
        if (copy_from_user(Adapter->Wpa_ie, wrq->u.data.pointer,
                           wrq->u.data.length)) {
            PRINTM(INFO, "failed to copy WPA IE \n");
            return -EFAULT;
        }
        Adapter->Wpa_ie_len = wrq->u.data.length;
        PRINTM(INFO, "Set Wpa_ie_len=%d IE=%#x\n", Adapter->Wpa_ie_len,
               Adapter->Wpa_ie[0]);
        HEXDUMP("Wpa_ie", Adapter->Wpa_ie, Adapter->Wpa_ie_len);
        if (Adapter->Wpa_ie[0] == WPA_IE)
            Adapter->SecInfo.WPAEnabled = TRUE;
        else if (Adapter->Wpa_ie[0] == WPA2_IE)
            Adapter->SecInfo.WPA2Enabled = TRUE;
        else {
            Adapter->SecInfo.WPAEnabled = FALSE;
            Adapter->SecInfo.WPA2Enabled = FALSE;
        }
    } else {
        memset(Adapter->Wpa_ie, 0, sizeof(Adapter->Wpa_ie));
        Adapter->Wpa_ie_len = wrq->u.data.length;
        PRINTM(INFO, "Reset Wpa_ie_len=%d IE=%#x\n", Adapter->Wpa_ie_len,
               Adapter->Wpa_ie[0]);
        Adapter->SecInfo.WPAEnabled = FALSE;
        Adapter->SecInfo.WPA2Enabled = FALSE;
    }

    // enable/disable RSN in firmware if WPA is enabled/disabled
    // depending on variable Adapter->SecInfo.WPAEnabled is set or not
    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_ENABLE_RSN,
                                HostCmd_ACT_SET, HostCmd_OPTION_WAITFORRSP,
                                0, NULL);

    LEAVE();
    return ret;
}

/** 
 *  @brief Set Auto Prescan
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_subcmd_setprescan_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int data;
    wlan_adapter *Adapter = priv->adapter;
    int *val;

    ENTER();

    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    PRINTM(INFO, "WLAN_SUBCMD_SET_PRESCAN %d\n", data);
    switch (data) {
    case CMD_ENABLED:
#ifdef MOTO_DBG
        PRINTM(MOTO_DEBUG, "WLAN_SUBCMD_SET_PRESCAN : enabled\n");
#endif
        Adapter->Prescan = TRUE;
        break;
    case CMD_DISABLED:
#ifdef MOTO_DBG
        PRINTM(MOTO_DEBUG, "WLAN_SUBCMD_SET_PRESCAN : disabled\n");
#endif
        Adapter->Prescan = FALSE;
        break;
    default:
#ifdef MOTO_DBG
        PRINTM(ERROR,
               "WLAN_SUBCMD_SET_PRESCAN : Not supported parameter %d.\n",
               data);
#endif
        break;
    }

    data = Adapter->Prescan;
    val = (int *) wrq->u.name;
    *val = data;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set multiple dtim
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_multiple_dtim_ioctl(wlan_private * priv, struct ifreq *req)
{
    struct iwreq *wrq = (struct iwreq *) req;
    u32 mdtim;
    int idata;
    int ret = -EINVAL;

    ENTER();

    idata = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    mdtim = (u32) idata;
    if (((mdtim >= MRVDRV_MIN_MULTIPLE_DTIM) &&
         (mdtim <= MRVDRV_MAX_MULTIPLE_DTIM))
        || (mdtim == MRVDRV_IGNORE_MULTIPLE_DTIM)) {
        priv->adapter->MultipleDtim = mdtim;
        ret = WLAN_STATUS_SUCCESS;
    }
    if (ret)
        PRINTM(INFO, "Invalid parameter, MultipleDtim not changed.\n");

    LEAVE();
    return ret;
}

/** 
 *  @brief Set authentication mode
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_setauthalg_ioctl(wlan_private * priv, struct ifreq *req)
{
    int alg;
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wrq->u.data.flags == 0) {
        //from iwpriv subcmd
        alg = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    } else {
        //from wpa_supplicant subcmd
        if (copy_from_user(&alg, wrq->u.data.pointer, sizeof(alg))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
    }

    PRINTM(INFO, "auth alg is %#x\n", alg);

    switch (alg) {
    case AUTH_ALG_SHARED_KEY:
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeShared;
        break;
    case AUTH_ALG_NETWORK_EAP:
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeNetworkEAP;
        break;
    case AUTH_ALG_OPEN_SYSTEM:
    default:
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;
        break;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Encryption mode
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_setencryptionmode_ioctl(wlan_private * priv, struct ifreq *req)
{
    int mode;
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    if (wrq->u.data.flags == 0) {
        //from iwpriv subcmd
        mode = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    } else {
        //from wpa_supplicant subcmd
        if (copy_from_user(&mode, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
    }
    PRINTM(INFO, "encryption mode is %#x\n", mode);
    priv->adapter->SecInfo.EncryptionMode = mode;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Rx antenna
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_subcmd_getrxantenna_ioctl(wlan_private * priv, struct ifreq *req)
{
    int len;
    char buf[8];
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    PRINTM(INFO, "WLAN_SUBCMD_GETRXANTENNA\n");
    len = GetRxAntenna(priv, buf);

    wrq->u.data.length = len;
    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &buf, len)) {
            PRINTM(INFO, "CopyToUser failed\n");
            return -EFAULT;
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Tx antenna
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_subcmd_gettxantenna_ioctl(wlan_private * priv, struct ifreq *req)
{
    int len;
    char buf[8];
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    PRINTM(INFO, "WLAN_SUBCMD_GETTXANTENNA\n");
    len = GetTxAntenna(priv, buf);

    wrq->u.data.length = len;
    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &buf, len)) {
            PRINTM(INFO, "CopyToUser failed\n");
            return -EFAULT;
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get the MAC TSF value from the firmware
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure containing buffer
 *                      space to store a TSF value retrieved from the firmware
 *
 *  @return             0 if successful; IOCTL error code otherwise
 */
static int
wlan_get_tsf_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    u64 tsfVal = 0;
    int ret;

    ENTER();

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_GET_TSF,
                                0, HostCmd_OPTION_WAITFORRSP, 0, &tsfVal);

    PRINTM(INFO, "IOCTL: Get TSF = 0x%016llx\n", tsfVal);

    if (ret != WLAN_STATUS_SUCCESS) {
        PRINTM(INFO, "IOCTL: Get TSF; Command exec failed\n");
        ret = -EFAULT;
    } else {
        if (copy_to_user(wrq->u.data.pointer,
                         &tsfVal,
                         MIN(wrq->u.data.length, sizeof(tsfVal))) != 0) {

            PRINTM(INFO, "IOCTL: Get TSF; Copy to user failed\n");
            ret = -EFAULT;
        } else {
            ret = 0;
        }
    }

    LEAVE();

    return ret;
}

/** 
 *  @brief Get/Set DeepSleep mode
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_deepsleep_ioctl(wlan_private * priv, struct ifreq *req)
{
    int ret = WLAN_STATUS_SUCCESS;
    char status[128];
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        PRINTM(MSG, "Cannot enter Deep Sleep mode in connected state.\n");
        return -EINVAL;
    }

    if (*(char *) req->ifr_data == '0') {
#ifdef MOTO_DBG
        PRINTM(MOTO_MSG,
               "Exit Deep Sleep Mode ioctl called by upper layer.\n");
#else
        PRINTM(INFO, "Exit Deep Sleep Mode.\n");
#endif
        sprintf(status, "setting to off ");
        SetDeepSleep(priv, FALSE);
    } else if (*(char *) req->ifr_data == '1') {
#ifdef MOTO_DBG
        PRINTM(MOTO_MSG,
               "Enter Deep Sleep Mode ioctl called by upper layer.\n");
#else
        PRINTM(INFO, "Enter Deep Sleep Mode.\n");
#endif
        sprintf(status, "setting to on ");
        SetDeepSleep(priv, TRUE);
    } else if (*(char *) req->ifr_data == '2') {
#ifdef MOTO_DBG
        PRINTM(MOTO_DEBUG,
               "Get Deep Sleep Mode ioctl called by upper layer.\n");
#else
        PRINTM(INFO, "Get Deep Sleep Mode.\n");
#endif
        if (Adapter->IsDeepSleep == TRUE) {
            sprintf(status, "on ");
        } else {
            sprintf(status, "off ");
        }
    } else {
#ifdef MOTO_DBG
        PRINTM(ERROR, "unknown deepsleep option = %d\n",
               *(u8 *) req->ifr_data);
#else
        PRINTM(INFO, "unknown option = %d\n", *(u8 *) req->ifr_data);
#endif
        return -EINVAL;
    }

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &status, strlen(status)))
            return -EFAULT;
        wrq->u.data.length = strlen(status);
    }
#ifdef MOTO_DBG
    PRINTM(MOTO_DEBUG, "Deep Sleep ioctl end.\n");
#endif

    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set inactivity timeout extend
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_inactivity_timeout_ext(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    int data[3];
    HostCmd_DS_INACTIVITY_TIMEOUT_EXT inactivity_ext;

    ENTER();
    if ((wrq->u.data.length != 3) && (wrq->u.data.length != 0))
        return -ENOTSUPP;
    memset(data, 0, sizeof(data));
    memset(&inactivity_ext, 0, sizeof(inactivity_ext));

    if (wrq->u.data.length == 0) {
        /* Get */
        inactivity_ext.Action = HostCmd_ACT_GET;
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_INACTIVITY_TIMEOUT_EXT,
                                    HostCmd_ACT_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0,
                                    &inactivity_ext);
        data[0] = (int) inactivity_ext.TimeoutUnit;
        data[1] = (int) inactivity_ext.UnicastTimeout;
        data[2] = (int) inactivity_ext.MulticastTimeout;
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 3)) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
    } else {
        /* Set */
        if (copy_from_user(data, wrq->u.data.pointer, sizeof(int) * 3)) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
        inactivity_ext.Action = HostCmd_ACT_SET;
        inactivity_ext.TimeoutUnit = (u16) data[0];
        inactivity_ext.UnicastTimeout = (u16) data[1];
        inactivity_ext.MulticastTimeout = (u16) data[2];
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_INACTIVITY_TIMEOUT_EXT,
                                    HostCmd_ACT_SET,
                                    HostCmd_OPTION_WAITFORRSP, 0,
                                    &inactivity_ext);
    }

    wrq->u.data.length = 3;

    LEAVE();
    return ret;
}

/** 
 *  @brief Config hostsleep parameter
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_hostsleepcfg_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    char buf[32];
    int ret = WLAN_STATUS_SUCCESS;
    int gpio, gap;

    memset(buf, 0, sizeof(buf));
    if (copy_from_user(buf, wrq->u.data.pointer,
                       MIN(sizeof(buf) - 1, wrq->u.data.length))) {
        PRINTM(INFO, "Copy from user failed\n");
        return -EFAULT;
    }
    buf[sizeof(buf) - 1] = 0;

    if (sscanf(buf, "%x %x %x", &Adapter->HSCfg.conditions, &gpio, &gap) != 3) {
        PRINTM(MSG, "Invalid parameters\n");
        return -EINVAL;
    }

    if (Adapter->HSCfg.conditions != HOST_SLEEP_CFG_CANCEL) {
        Adapter->HSCfg.gpio = (u8) gpio;
        Adapter->HSCfg.gap = (u8) gap;
    }
#ifdef MOTO_DBG
    PRINTM(MOTO_MSG,
           "hostsleepcfg ioctl called by upper layer: cond=%#x gpio=%#x gap=%#x\n",
#else
    PRINTM(INFO, "hostsleepcfg: cond=%#x gpio=%#x gap=%#x\n",
#endif
           Adapter->HSCfg.conditions, Adapter->HSCfg.gpio,
           Adapter->HSCfg.gap);

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_HOST_SLEEP_CFG,
                                0, HostCmd_OPTION_WAITFORRSP, 0,
                                &Adapter->HSCfg);

    return ret;
}

/** 
 *  @brief Config Host Sleep parameters
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wreq		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_hscfg_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    u32 data[3] = { -1, 0xff, 0xff };
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length >= 1 && wrq->u.data.length <= 3) {
        if (copy_from_user
            (data, wrq->u.data.pointer,
             sizeof(data[0]) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
        PRINTM(INFO,
               "wlan_hscfg_ioctl: data[0]=%#08x, data[1]=%#02x, data[2]=%#02x\n",
               data[0], data[1], data[2]);
    } else {
        PRINTM(MSG, "Invalid Argument\n");
        return -EINVAL;
    }

    Adapter->HSCfg.conditions = data[0];
    if (Adapter->HSCfg.conditions != HOST_SLEEP_CFG_CANCEL) {
        if (wrq->u.data.length == 2) {
            Adapter->HSCfg.gpio = (u8) data[1];
        } else if (wrq->u.data.length == 3) {
            Adapter->HSCfg.gpio = (u8) data[1];
            Adapter->HSCfg.gap = (u8) data[2];
        }
    }
#ifdef MOTO_DBG
    PRINTM(MOTO_MSG,
           "hscfg ioctl called by upper layer: nb_param=%d: cond=%#x gpio=%#x gap=%#x\n",
           wrq->u.data.length, Adapter->HSCfg.conditions, Adapter->HSCfg.gpio,
           Adapter->HSCfg.gap);
#else
    PRINTM(INFO, "hscfg: cond=%#x gpio=%#x gap=%#x\n",
           Adapter->HSCfg.conditions, Adapter->HSCfg.gpio,
           Adapter->HSCfg.gap);
#endif

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_HOST_SLEEP_CFG,
                                0, HostCmd_OPTION_WAITFORRSP, 0,
                                &Adapter->HSCfg);

    data[0] = Adapter->HSCfg.conditions;
    data[1] = Adapter->HSCfg.gpio;
    data[2] = Adapter->HSCfg.gap;
    wrq->u.data.length = 3;
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set Cal data ext
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_caldata_ext_ioctl(wlan_private * priv, struct ifreq *req)
{
    HostCmd_DS_802_11_CAL_DATA_EXT *pCalData = NULL;
    int ret = WLAN_STATUS_SUCCESS;
    u16 action;

    ENTER();

    if (!
        (pCalData =
         kmalloc(sizeof(HostCmd_DS_802_11_CAL_DATA_EXT), GFP_KERNEL))) {
        PRINTM(INFO, "Allocate memory failed\n");
        ret = -ENOMEM;
        goto calexit;
    }
    memset(pCalData, 0, sizeof(HostCmd_DS_802_11_CAL_DATA_EXT));

    if (copy_from_user(pCalData, req->ifr_data + SKIP_CMDNUM,
                       sizeof(HostCmd_DS_802_11_CAL_DATA_EXT))) {
        PRINTM(INFO, "Copy from user failed\n");
        kfree(pCalData);
        ret = -EFAULT;
        goto calexit;
    }

    action = (pCalData->Action == HostCmd_ACT_GEN_SET) ?
        HostCmd_ACT_GEN_SET : HostCmd_ACT_GEN_GET;

    HEXDUMP("Cal data ext", (u8 *) pCalData,
            sizeof(HostCmd_DS_802_11_CAL_DATA_EXT));

    PRINTM(INFO, "CalData Action = 0x%0X\n", action);

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_CAL_DATA_EXT,
                                action,
                                HostCmd_OPTION_WAITFORRSP, 0, pCalData);

    if (!ret && action == HostCmd_ACT_GEN_GET) {
        if (copy_to_user(req->ifr_data + SKIP_CMDNUM, pCalData,
                         sizeof(HostCmd_DS_802_11_CAL_DATA_EXT))) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
        }
    }

    kfree(pCalData);
  calexit:
    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set sleep period 
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_sleep_period(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    int data;
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_SLEEP_PERIOD sleeppd;

    ENTER();

    if (wrq->u.data.length > 1)
        return -ENOTSUPP;

    memset(&sleeppd, 0, sizeof(sleeppd));
    memset(&Adapter->sleep_period, 0, sizeof(SleepPeriod));

    if (wrq->u.data.length == 0) {
        sleeppd.Action = wlan_cpu_to_le16(HostCmd_ACT_GEN_GET);
    } else {
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        /* sleep period is 0 or 10~60 in milliseconds */
#define MIN_SLEEP_PERIOD		10
#define MAX_SLEEP_PERIOD		60
#define SLEEP_PERIOD_RESERVED_FF	0xFF
        if ((data <= MAX_SLEEP_PERIOD && data >= MIN_SLEEP_PERIOD) ||
            (data == 0)
            || (data == SLEEP_PERIOD_RESERVED_FF)       /* for UPSD certification tests */
            ) {
            sleeppd.Action = wlan_cpu_to_le16(HostCmd_ACT_GEN_SET);
            sleeppd.Period = data;
        } else
            return -EINVAL;
    }

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SLEEP_PERIOD,
                                0, HostCmd_OPTION_WAITFORRSP,
                                0, (void *) &sleeppd);

    data = (int) Adapter->sleep_period.period;
    if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }
    wrq->u.data.length = 1;

    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set adapt rate 
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_adapt_rateset(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    wlan_adapter *Adapter = priv->adapter;
    int data[4];
    int rateindex;

    ENTER();
    memset(data, 0, sizeof(data));
    if (!wrq->u.data.length) {
        PRINTM(INFO, "Get ADAPT RATE SET\n");
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
                                    HostCmd_ACT_GEN_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0, NULL);
        data[0] = Adapter->HWRateDropMode;
        data[2] = Adapter->Threshold;
        data[3] = Adapter->FinalRate;
        wrq->u.data.length = 4;
        data[1] = Adapter->RateBitmap;
        if (copy_to_user
            (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }

    } else {
        PRINTM(INFO, "Set ADAPT RATE SET\n");
        if (wrq->u.data.length > 4)
            return -EINVAL;
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        if (data[0] > HW_SINGLE_RATE_DROP)
            return -EINVAL;
        Adapter->HWRateDropMode = data[0];
        Adapter->Threshold = data[2];
        Adapter->FinalRate = data[3];
        Adapter->RateBitmap = data[1];
        Adapter->Is_DataRate_Auto = Is_Rate_Auto(priv);
        if (Adapter->Is_DataRate_Auto)
            Adapter->DataRate = 0;
        else {
            rateindex = GetRateIndex(priv);
            Adapter->DataRate = index_to_data_rate(rateindex);
        }
        PRINTM(INFO, "RateBitmap=%x,IsRateAuto=%d,DataRate=%d\n",
               Adapter->RateBitmap, Adapter->Is_DataRate_Auto,
               Adapter->DataRate);
        ret =
            PrepareAndSendCommand(priv, HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
                                  HostCmd_ACT_GEN_SET,
                                  HostCmd_OPTION_WAITFORRSP, 0, NULL);
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set inactivity timeout
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_inactivity_timeout(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    int data = 0;
    u16 timeout = 0;

    ENTER();
    if (wrq->u.data.length > 1)
        return -ENOTSUPP;

    if (wrq->u.data.length == 0) {
        /* Get */
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_INACTIVITY_TIMEOUT,
                                    HostCmd_ACT_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0, &timeout);
        data = timeout;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
    } else {
        /* Set */
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        timeout = data;
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_INACTIVITY_TIMEOUT,
                                    HostCmd_ACT_SET,
                                    HostCmd_OPTION_WAITFORRSP, 0, &timeout);
    }

    wrq->u.data.length = 1;

    LEAVE();
    return ret;
}

/** 
 *  @brief Get LOG
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_getlog_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    char *buf = NULL;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    PRINTM(INFO, " GET STATS\n");

    if (!(buf = kmalloc(GETLOG_BUFSIZE, GFP_KERNEL))) {
        PRINTM(INFO, "kmalloc failed!\n");
        return -ENOMEM;
    }

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_GET_LOG,
                                0, HostCmd_OPTION_WAITFORRSP, 0, NULL);

    if (!ret && wrq->u.data.pointer) {
        sprintf(buf, "\n"
                "mcasttxframe     %u\n"
                "failed           %u\n"
                "retry            %u\n"
                "multiretry       %u\n"
                "framedup         %u\n"
                "rtssuccess       %u\n"
                "rtsfailure       %u\n"
                "ackfailure       %u\n"
                "rxfrag           %u\n"
                "mcastrxframe     %u\n"
                "fcserror         %u\n"
                "txframe          %u\n"
                "wepundecryptable %u\n",
                Adapter->LogMsg.mcasttxframe,
                Adapter->LogMsg.failed,
                Adapter->LogMsg.retry,
                Adapter->LogMsg.multiretry,
                Adapter->LogMsg.framedup,
                Adapter->LogMsg.rtssuccess,
                Adapter->LogMsg.rtsfailure,
                Adapter->LogMsg.ackfailure,
                Adapter->LogMsg.rxfrag,
                Adapter->LogMsg.mcastrxframe,
                Adapter->LogMsg.fcserror,
                Adapter->LogMsg.txframe, Adapter->LogMsg.wepundecryptable);
        wrq->u.data.length = strlen(buf) + 1;
        if (copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
        }
    }

    kfree(buf);
    LEAVE();
    return ret;
}

/** 
 *  @brief config sleep parameters
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_sleep_params_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    wlan_adapter *Adapter = priv->adapter;
    wlan_ioctl_sleep_params_config sp;

    ENTER();

    memset(&sp, 0, sizeof(sp));

    if (!wrq->u.data.pointer)
        return -EFAULT;
    if (copy_from_user(&sp, wrq->u.data.pointer,
                       MIN(sizeof(sp), wrq->u.data.length))) {
        PRINTM(INFO, "Copy from user failed\n");
        return -EFAULT;
    }

    memcpy(&Adapter->sp, &sp.Error, sizeof(SleepParams));

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SLEEP_PARAMS,
                                sp.Action, HostCmd_OPTION_WAITFORRSP,
                                0, NULL);

    if (!ret && !sp.Action) {
        memcpy(&sp.Error, &Adapter->sp, sizeof(SleepParams));
        if (copy_to_user(wrq->u.data.pointer, &sp, sizeof(sp))) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = sizeof(sp);
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Read the CIS Table
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_getcis_ioctl(wlan_private * priv, struct ifreq *req)
{
    int ret = WLAN_STATUS_SUCCESS;
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, Adapter->CisInfoBuf,
                         Adapter->CisInfoLen)) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = Adapter->CisInfoLen;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Set BCA timeshare
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_bca_timeshare_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    wlan_adapter *Adapter = priv->adapter;
    wlan_ioctl_bca_timeshare_config bca_ts;

    ENTER();

    memset(&bca_ts, 0, sizeof(HostCmd_DS_802_11_BCA_TIMESHARE));

    if (!wrq->u.data.pointer)
        return -EFAULT;
    if (copy_from_user(&bca_ts, wrq->u.data.pointer,
                       MIN(sizeof(bca_ts), wrq->u.data.length))) {
        PRINTM(INFO, "Copy from user failed\n");
        return -EFAULT;
    }

    PRINTM(INFO, "TrafficType=%x TimeShareInterva=%x BTTime=%x\n",
           bca_ts.TrafficType, bca_ts.TimeShareInterval, bca_ts.BTTime);

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_BCA_CONFIG_TIMESHARE,
                                bca_ts.Action, HostCmd_OPTION_WAITFORRSP,
                                0, &bca_ts);

    if (!ret && !bca_ts.Action) {
        if (copy_to_user(wrq->u.data.pointer, &Adapter->bca_ts,
                         sizeof(bca_ts))) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = sizeof(HostCmd_DS_802_11_BCA_TIMESHARE);
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Set scan type
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_scan_type_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    u8 buf[12];
    u8 *option[] = { "active", "passive", "get", };
    int i, max_options = (sizeof(option) / sizeof(option[0]));
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wlan_get_state_11d(priv) == ENABLE_11D) {
#ifdef MOTO_DBG
        PRINTM(ERROR,
               "Scantype: 11D: Cannot set scantype when 11D enabled\n");
#else
        PRINTM(INFO, "11D: Cannot set scantype when 11D enabled\n");
#endif
        return -EFAULT;
    }

    memset(buf, 0, sizeof(buf));

    if (copy_from_user(buf, wrq->u.data.pointer, MIN(sizeof(buf),
                                                     wrq->u.data.length))) {
#ifdef MOTO_DBG
        PRINTM(ERROR, "Scantype: Copy from user failed\n");
#else
        PRINTM(INFO, "Copy from user failed\n");
#endif
        return -EFAULT;
    }

    PRINTM(INFO, "Scan Type Option = %s\n", buf);

    buf[sizeof(buf) - 1] = '\0';

    for (i = 0; i < max_options; i++) {
        if (!strcmp(buf, option[i]))
            break;
    }

    switch (i) {
    case 0:
        Adapter->ScanType = HostCmd_SCAN_TYPE_ACTIVE;
#ifdef MOTO_DBG
        PRINTM(MOTO_DEBUG,
               "scantype IOCTL called by upper layer. Set to active\n");
#endif
        break;
    case 1:
        Adapter->ScanType = HostCmd_SCAN_TYPE_PASSIVE;
#ifdef MOTO_DBG
        PRINTM(MOTO_DEBUG,
               "scantype IOCTL called by upper layer. Set to passive\n");
#endif
        break;
    case 2:
#ifdef MOTO_DBG
        PRINTM(MOTO_DEBUG, "Scantype get.\n")
#endif
            wrq->u.data.length = strlen(option[Adapter->ScanType]) + 1;

        if (copy_to_user(wrq->u.data.pointer,
                         option[Adapter->ScanType], wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
        }

        break;
    default:
#ifdef MOTO_DBG
        PRINTM(ERROR, "Scantype: Invalid Scan Type Ioctl Option %s\n", buf);
#else
        PRINTM(INFO, "Invalid Scan Type Ioctl Option\n");
#endif
        ret = -EINVAL;
        break;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Set scan mode
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_scan_mode_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    u8 buf[12];
    u8 *option[] = { "bss", "ibss", "any", "get" };
    int i, max_options = (sizeof(option) / sizeof(option[0]));
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    memset(buf, 0, sizeof(buf));

    if (copy_from_user(buf, wrq->u.data.pointer, MIN(sizeof(buf),
                                                     wrq->u.data.length))) {
        PRINTM(INFO, "Copy from user failed\n");
        return -EFAULT;
    }

    PRINTM(INFO, "Scan Mode Option = %s\n", buf);

    buf[sizeof(buf) - 1] = '\0';

    for (i = 0; i < max_options; i++) {
        if (!strcmp(buf, option[i]))
            break;
    }

    switch (i) {

    case 0:
        Adapter->ScanMode = HostCmd_BSS_TYPE_BSS;
        break;
    case 1:
        Adapter->ScanMode = HostCmd_BSS_TYPE_IBSS;
        break;
    case 2:
        Adapter->ScanMode = HostCmd_BSS_TYPE_ANY;
        break;
    case 3:

        wrq->u.data.length = strlen(option[Adapter->ScanMode - 1]) + 1;

        PRINTM(INFO, "Get Scan Mode Option = %s\n",
               option[Adapter->ScanMode - 1]);

        PRINTM(INFO, "Scan Mode Length %d\n", wrq->u.data.length);

        if (copy_to_user(wrq->u.data.pointer,
                         option[Adapter->ScanMode - 1], wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
        }
        PRINTM(INFO, "GET Scan Type Option after copy = %s\n",
               (char *) wrq->u.data.pointer);

        break;

    default:
        PRINTM(INFO, "Invalid Scan Mode Ioctl Option\n");
        ret = -EINVAL;
        break;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set Adhoc G Rate
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_do_set_grate_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data, data1;
    int *val;

    ENTER();

    data1 = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    switch (data1) {
    case 0:
        Adapter->adhoc_grate_enabled = FALSE;
        break;
    case 1:
        Adapter->adhoc_grate_enabled = TRUE;
        break;
    case 2:
        break;
    default:
        return -EINVAL;
    }
    data = Adapter->adhoc_grate_enabled;
    val = (int *) wrq->u.name;
    *val = data;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Firmware wakeup method
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_cmd_fw_wakeup_method(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    u16 action;
    u16 method;
    int ret;
    int data;

    ENTER();

    if (wrq->u.data.length == 0 || !wrq->u.data.pointer) {
        action = HostCmd_ACT_GET;
    } else {
        action = HostCmd_ACT_SET;
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }

        switch (data) {
        case 0:
            Adapter->fwWakeupMethod = WAKEUP_FW_UNCHANGED;
            break;
        case 1:
            Adapter->fwWakeupMethod = WAKEUP_FW_THRU_INTERFACE;
            break;
        case 2:
            Adapter->fwWakeupMethod = WAKEUP_FW_THRU_GPIO;
            break;
        default:
            return -EINVAL;
        }
    }

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_FW_WAKEUP_METHOD, action,
                                HostCmd_OPTION_WAITFORRSP, 0,
                                &Adapter->fwWakeupMethod);

    if (action == HostCmd_ACT_GET) {
        method = Adapter->fwWakeupMethod;
        if (copy_to_user(wrq->u.data.pointer, &method, sizeof(method))) {
            PRINTM(INFO, "Copy to user failed\n");
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    }

    LEAVE();
    return ret;
}

/**
 *  @brief Get the CFP table based on the region code
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param wrq      A pointer to iwreq structure
 *
 *  @return         WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_get_cfp_table_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    pwlan_ioctl_cfp_table ioctl_cfp;
    CHANNEL_FREQ_POWER *cfp;
    int cfp_no;
    int regioncode;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length == 0 || !wrq->u.data.pointer) {
        ret = -EINVAL;
        goto cfpexit;
    }

    ioctl_cfp = (pwlan_ioctl_cfp_table) wrq->u.data.pointer;

    if (copy_from_user(&regioncode, &ioctl_cfp->region, sizeof(int))) {
        PRINTM(INFO, "Get CFP table: copy from user failed\n");
        ret = -EFAULT;
        goto cfpexit;
    }

    if (!regioncode)
        regioncode = Adapter->RegionCode;

    cfp =
        wlan_get_region_cfp_table((u8) regioncode, BAND_G | BAND_B, &cfp_no);

    if (cfp == NULL) {
        PRINTM(MSG, "No related CFP table found, region code = 0x%x\n",
               regioncode);
        ret = -EFAULT;
        goto cfpexit;
    }

    if (copy_to_user(&ioctl_cfp->cfp_no, &cfp_no, sizeof(int))) {
        PRINTM(INFO, "Get CFP table: copy to user failed\n");
        ret = -EFAULT;
        goto cfpexit;
    }

    if (copy_to_user
        (ioctl_cfp->cfp, cfp, sizeof(CHANNEL_FREQ_POWER) * cfp_no)) {
        PRINTM(INFO, "Get CFP table: copy to user failed\n");
        ret = -EFAULT;
        goto cfpexit;
    }

    wrq->u.data.length =
        sizeof(int) * 2 + sizeof(CHANNEL_FREQ_POWER) * cfp_no;

  cfpexit:
    LEAVE();
    return ret;
}

/********************************************************
		Global Functions
********************************************************/
/** 
 *  @brief ioctl function - entry point
 *  
 *  @param dev		A pointer to net_device structure
 *  @param req	   	A pointer to ifreq structure
 *  @param cmd 		command
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
int
wlan_do_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    int subcmd = 0;
    int idata = 0;
    int *pdata;
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    if (Adapter->IsDeepSleep) {
        int count = sizeof(Commands_Allowed_In_DeepSleep)
            / sizeof(Commands_Allowed_In_DeepSleep[0]);

        if (!Is_Command_Allowed_In_Sleep
            (req, cmd, Commands_Allowed_In_DeepSleep, count)) {
            PRINTM(MSG,
                   "():%s IOCTLS called when station is" " in DeepSleep\n",
                   __FUNCTION__);
            return -EBUSY;
        }
    }

    PRINTM(INFO, "wlan_do_ioctl: ioctl cmd = 0x%x\n", cmd);
    switch (cmd) {
    case WLANEXTSCAN:
        ret = wlan_extscan_ioctl(priv, req);
        break;
    case WLANHOSTCMD:
        ret = wlan_hostcmd_ioctl(dev, req, cmd);
        break;
    case WLANARPFILTER:
        ret = wlan_arpfilter_ioctl(dev, req, cmd);
        break;

    case WLANCISDUMP:          /* Read CIS Table  */
        ret = wlan_do_getcis_ioctl(priv, req);
        break;

    case WLANSCAN_TYPE:
        PRINTM(INFO, "Scan Type Ioctl\n");
        ret = wlan_scan_type_ioctl(priv, wrq);
        break;

#ifdef MFG_CMD_SUPPORT
    case WLANMANFCMD:
        PRINTM(INFO, "Entering the Manufacturing ioctl SIOCCFMFG\n");
        ret = wlan_mfg_command(priv, (void *) req->ifr_data);

        PRINTM(INFO, "Manufacturing Ioctl %s\n",
               (ret) ? "failed" : "success");
        break;
#endif

    case WLANREGRDWR:          /* Register read write command */
        ret = wlan_regrdwr_ioctl(priv, req);
        break;

    case WLANCMD52RDWR:        /* CMD52 read/write command */
        ret = wlan_cmd52rdwr_ioctl(priv, req);
        break;

    case WLANCMD53RDWR:        /* CMD53 read/write command */
        ret = wlan_cmd53rdwr_ioctl(priv, req);
        break;

    case SIOCSIWENCODE:        /* set encoding token & mode for WPA */
        ret = wlan_set_encode(dev, NULL, &(wrq->u.data), wrq->u.data.pointer);
        break;
    case WLAN_SETNONE_GETNONE: /* set WPA mode on/off ioctl #20 */
        switch (wrq->u.data.flags) {
        case WLANDEAUTH:
#ifdef MOTO_DBG
            PRINTM(MOTO_MSG, "Deauth IOCTL called by upper layer.\n");
#else
            PRINTM(INFO, "Deauth\n");
#endif
            wlan_send_deauth(priv);
            break;

        case WLANADHOCSTOP:
            PRINTM(INFO, "Adhoc stop\n");
            ret = wlan_do_adhocstop_ioctl(priv);
            break;

        case WLANRADIOON:
            wlan_radio_ioctl(priv, RADIO_ON);
            break;

        case WLANRADIOOFF:
            ret = wlan_radio_ioctl(priv, RADIO_OFF);
            break;
        case WLANREMOVEADHOCAES:
            ret = wlan_remove_aes(priv);
            break;
#ifdef REASSOCIATION
        case WLANREASSOCIATIONAUTO:
            reassociation_on(priv);
            break;
        case WLANREASSOCIATIONUSER:
            reassociation_off(priv);
            break;
#endif /* REASSOCIATION */
        case WLANWLANIDLEON:
            wlanidle_on(priv);
            break;
        case WLANWLANIDLEOFF:
            wlanidle_off(priv);
            break;
        }                       /* End of switch */
        break;

    case WLAN_SETWORDCHAR_GETNONE:
        switch (wrq->u.data.flags) {
        case WLANSETADHOCAES:
            ret = wlan_setadhocaes_ioctl(priv, req);
            break;
        }
        break;

    case WLAN_SETNONE_GETWORDCHAR:
        switch (wrq->u.data.flags) {
        case WLANGETADHOCAES:
            ret = wlan_getadhocaes_ioctl(priv, req);
            break;
        case WLANVERSION:      /* Get driver version */
            ret = wlan_version_ioctl(priv, req);
            break;
        }
        break;

    case WLANSETWPAIE:
        ret = wlan_setwpaie_ioctl(priv, req);
        break;
    case WLAN_SETINT_GETINT:
        /* The first 4 bytes of req->ifr_data is sub-ioctl number
         * after 4 bytes sits the payload.
         */
        subcmd = (int) req->ifr_data;   //from iwpriv subcmd
        switch (subcmd) {
        case WLANNF:
            ret = wlan_get_nf(priv, wrq);
            break;
        case WLANRSSI:
            ret = wlan_get_rssi(priv, wrq);
            break;
        case WLANBGSCAN:
            {
                int data, data1;
                int *val;
                data1 = *((int *) (wrq->u.name + SUBCMD_OFFSET));
                switch (data1) {
                case CMD_DISABLED:
#ifdef MOTO_DBG
                    PRINTM(MOTO_MSG,
                           "Background scan: disabled by upper layer. \n");
#else
                    PRINTM(INFO, "Background scan is set to disable\n");
#endif
                    ret = wlan_bg_scan_enable(priv, FALSE);
                    val = (int *) wrq->u.name;
                    *val = data1;
                    break;
                case CMD_ENABLED:
#ifdef MOTO_DBG
                    PRINTM(MOTO_MSG,
                           "Background scan: enabled by upper layer. \n");
#else
                    PRINTM(INFO, "Background scan is set to enable\n");
#endif
                    ret = wlan_bg_scan_enable(priv, TRUE);
                    val = (int *) wrq->u.name;
                    *val = data1;
                    break;
                case CMD_GET:
#ifdef MOTO_DBG
                    PRINTM(MOTO_DEBUG,
                           "Background scan: get state called by upper layer. \n");
#endif
                    data = (Adapter->bgScanConfig->Enable == TRUE) ?
                        CMD_ENABLED : CMD_DISABLED;
                    val = (int *) wrq->u.name;
                    *val = data;
                    break;
                default:
                    ret = -EINVAL;
#ifdef MOTO_DBG
                    PRINTM(ERROR, "Background scan: wrong parameter %d\n",
                           data1);
#else
                    PRINTM(INFO, "Background scan: wrong parameter\n");
#endif
                    break;
                }
            }
            break;
        case WLANENABLE11D:
            ret = wlan_cmd_enable_11d(priv, wrq);
            break;
        case WLANADHOCGRATE:
            ret = wlan_do_set_grate_ioctl(priv, wrq);
            break;
        case WLANSDIOCLOCK:
            {
                int data;
                int *val;
                data = *((int *) (wrq->u.name + SUBCMD_OFFSET));
                switch (data) {
                case CMD_DISABLED:
                    PRINTM(INFO, "SDIO clock is turned off\n");
                    ret = sbi_set_bus_clock(priv, FALSE);
                    break;
                case CMD_ENABLED:
                    PRINTM(INFO, "SDIO clock is turned on\n");
                    ret = sbi_set_bus_clock(priv, TRUE);
                    break;
                case CMD_GET:  /* need an API in sdio.c to get STRPCL */
                default:
                    ret = -EINVAL;
                    PRINTM(INFO, "sdioclock: wrong parameter\n");
                    break;
                }
                val = (int *) wrq->u.name;
                *val = data;
            }
            break;
        case WLANWMM_ENABLE:
            ret = wlan_wmm_enable_ioctl(priv, wrq);
            break;
        case WLANNULLGEN:
            ret = wlan_null_pkg_gen(priv, wrq);
            /* enable/disable null pkg generation */
            break;
        case WLAN_SUBCMD_SET_PRESCAN:
            ret = wlan_subcmd_setprescan_ioctl(priv, wrq);
            break;
        case WLANADHOCCSET:
            ret = wlan_set_coalescing_ioctl(priv, wrq);
            break;
#ifdef WPRM_DRV
        case WLANMOTOTM:
            {
                int data1, data;
                int *val;
                data1 = *((int *) (wrq->u.name + SUBCMD_OFFSET));
                switch (data1) {
                case CMD_DISABLED:
                    PRINTM(INFO,
                           "Disable automatic traffic meter feature.\n");
                    ret = wlan_wprm_tm_enable(FALSE);
                    val = (int *) wrq->u.name;
                    *val = data1;
                    break;
                case CMD_ENABLED:
                    PRINTM(INFO, "Enable automatic traffic meter feature.\n");
                    ret = wlan_wprm_tm_enable(TRUE);
                    val = (int *) wrq->u.name;
                    *val = data1;
                    break;
                case CMD_GET:
                    data = wlan_wprm_tm_is_enabled();
                    PRINTM(INFO,
                           "Get automatic traffic meter feature state : %d.\n",
                           data);
                    val = (int *) wrq->u.name;
                    *val = data;
                    break;
                default:
                    ret = -EINVAL;
                    PRINTM(INFO, "mototm: wrong parameter.\n");
                    break;
                }
            }
            break;
#endif /* WPRM_DRV */

        }
        break;

    case WLAN_SETONEINT_GETONEINT:
        switch (wrq->u.data.flags) {

        case WLAN_WMM_QOSINFO:
            {
                int data;
                if (wrq->u.data.length == 1) {
                    if (copy_from_user
                        (&data, wrq->u.data.pointer, sizeof(int))) {
                        PRINTM(INFO, "Copy from user failed\n");
                        return -EFAULT;
                    }
                    Adapter->wmm.qosinfo = (u8) data;
                } else {
                    data = (int) Adapter->wmm.qosinfo;
                    if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
                        PRINTM(INFO, "Copy to user failed\n");
                        return -EFAULT;
                    }
                    wrq->u.data.length = 1;
                }
            }
            break;
        case WLAN_LISTENINTRVL:
            if (!wrq->u.data.length) {
                int data;
                PRINTM(INFO, "Get LocalListenInterval Value\n");
#define GET_ONE_INT	1
                data = Adapter->LocalListenInterval;
                if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
                    PRINTM(INFO, "Copy to user failed\n");
                    return -EFAULT;
                }

                wrq->u.data.length = GET_ONE_INT;
            } else {
                int data;
                if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
                    PRINTM(INFO, "Copy from user failed\n");
                    return -EFAULT;
                }

                PRINTM(INFO, "Set LocalListenInterval = %d\n", data);
#define MAX_U16_VAL	65535
                if (data > MAX_U16_VAL) {
                    PRINTM(INFO, "Exceeds U16 value\n");
                    return -EINVAL;
                }
                Adapter->LocalListenInterval = data;
            }
            break;
        case WLAN_FW_WAKEUP_METHOD:
            ret = wlan_cmd_fw_wakeup_method(priv, wrq);
            break;
        case WLAN_TXCONTROL:
            ret = wlan_txcontrol(priv, wrq);    //adds for txcontrol ioctl
            break;

        case WLAN_NULLPKTINTERVAL:
            ret = wlan_null_pkt_interval(priv, wrq);
            break;
        case WLAN_BCN_MISS_TIMEOUT:
            ret = wlan_bcn_miss_timeout(priv, wrq);
            break;
        case WLAN_ADHOC_AWAKE_PERIOD:
            ret = wlan_adhoc_awake_period(priv, wrq);
            break;
        case WLAN_LDO:
            ret = wlan_ldo_config(priv, wrq);
            break;
        case WLAN_SDIO_MODE:
            ret = wlan_sdio_mode(priv, wrq);
            break;
        default:
            ret = -EOPNOTSUPP;
            break;
        }
        break;

    case WLAN_SETONEINT_GETNONE:
        /* The first 4 bytes of req->ifr_data is sub-ioctl number
         * after 4 bytes sits the payload.
         */
        subcmd = wrq->u.data.flags;     //from wpa_supplicant subcmd

        if (!subcmd)
            subcmd = (int) req->ifr_data;       //from iwpriv subcmd

        switch (subcmd) {
        case WLAN_SUBCMD_SETRXANTENNA: /* SETRXANTENNA */
            idata = *((int *) (wrq->u.name + SUBCMD_OFFSET));
            ret = SetRxAntenna(priv, idata);
            break;
        case WLAN_SUBCMD_SETTXANTENNA: /* SETTXANTENNA */
            idata = *((int *) (wrq->u.name + SUBCMD_OFFSET));
            ret = SetTxAntenna(priv, idata);
            break;

        case WLANSETBCNAVG:
            {
                u16 bcn_avg = *((int *) (wrq->u.name + SUBCMD_OFFSET));

                if (bcn_avg == 0)
                    Adapter->bcn_avg_factor = DEFAULT_BCN_AVG_FACTOR;
                else if (bcn_avg > MAX_BCN_AVG_FACTOR
                         || bcn_avg < MIN_BCN_AVG_FACTOR) {
                    PRINTM(MSG,
                           "The value '%u' is out of the range (0-%u).\n",
                           bcn_avg, MAX_BCN_AVG_FACTOR);
                    return -EINVAL;
                } else
                    Adapter->bcn_avg_factor = bcn_avg;
                break;
            }
        case WLANSETDATAAVG:
            {
                u16 data_avg = *((int *) (wrq->u.name + SUBCMD_OFFSET));

                if (data_avg == 0)
                    Adapter->data_avg_factor = DEFAULT_DATA_AVG_FACTOR;
                else if (data_avg > MAX_DATA_AVG_FACTOR
                         || data_avg < MIN_DATA_AVG_FACTOR) {
                    PRINTM(MSG,
                           "The value '%u' is out of the range (0-%u).\n",
                           data_avg, MAX_DATA_AVG_FACTOR);
                    return -EINVAL;
                } else
                    Adapter->data_avg_factor = data_avg;
                memset(Adapter->rawSNR, 0x00, sizeof(Adapter->rawSNR));
                memset(Adapter->rawNF, 0x00, sizeof(Adapter->rawNF));
                Adapter->nextSNRNF = 0;
                Adapter->numSNRNF = 0;
                break;
            }
        case WLANSETREGION:
            idata = *((int *) (wrq->u.name + SUBCMD_OFFSET));
            ret = wlan_set_region(priv, (u16) idata);
            break;

        case WLAN_SET_LISTEN_INTERVAL:
            idata = *((int *) (wrq->u.name + SUBCMD_OFFSET));
            Adapter->ListenInterval = (u16) idata;
            break;

        case WLAN_SET_MULTIPLE_DTIM:
            ret = wlan_set_multiple_dtim_ioctl(priv, req);
            break;

        case WLANSETAUTHALG:
            ret = wlan_setauthalg_ioctl(priv, req);
            break;

        case WLANSETENCRYPTIONMODE:
            ret = wlan_setencryptionmode_ioctl(priv, req);
            break;
        default:
            ret = -EOPNOTSUPP;
            break;
        }

        break;

    case WLAN_SETNONE_GETTWELVE_CHAR:  /* Get Antenna settings */
        /* 
         * We've not used IW_PRIV_TYPE_FIXED so sub-ioctl number is
         * in flags of iwreq structure, otherwise it will be in
         * mode member of iwreq structure.
         */
        switch ((int) wrq->u.data.flags) {
        case WLAN_SUBCMD_GETRXANTENNA: /* Get Rx Antenna */
            ret = wlan_subcmd_getrxantenna_ioctl(priv, req);
            break;

        case WLAN_SUBCMD_GETTXANTENNA: /* Get Tx Antenna */
            ret = wlan_subcmd_gettxantenna_ioctl(priv, req);
            break;

        case WLAN_GET_TSF:
            ret = wlan_get_tsf_ioctl(priv, wrq);
            break;

        }
        break;

    case WLANDEEPSLEEP:
        ret = wlan_deepsleep_ioctl(priv, req);
        break;

    case WLANHOSTSLEEPCFG:
        ret = wlan_do_hostsleepcfg_ioctl(priv, wrq);
        break;

    case WLAN_SET64CHAR_GET64CHAR:
        switch ((int) wrq->u.data.flags) {

        case WLANSLEEPPARAMS:
            ret = wlan_sleep_params_ioctl(priv, wrq);
            break;

        case WLAN_BCA_TIMESHARE:
            ret = wlan_bca_timeshare_ioctl(priv, wrq);
            break;
        case WLANSCAN_MODE:
            PRINTM(INFO, "Scan Mode Ioctl\n");
            ret = wlan_scan_mode_ioctl(priv, wrq);
            break;

        case WLAN_GET_ADHOC_STATUS:
            ret = wlan_get_adhoc_status_ioctl(priv, wrq);
            break;
        case WLAN_SET_GEN_IE:
            ret = wlan_set_gen_ie_ioctl(priv, wrq);
            break;
        case WLAN_GET_GEN_IE:
            ret = wlan_get_gen_ie_ioctl(priv, wrq);
            break;
        case WLAN_REASSOCIATE:
            ret = wlan_reassociate_ioctl(dev, wrq);
            break;
        case WLAN_WMM_QUEUE_STATUS:
            ret = wlan_wmm_queue_status_ioctl(priv, wrq);
            break;
        case SET_WILDCARD_SSID:
            ret = wlan_setwildcardssid(priv, wrq);
            break;
        case GET_WILDCARD_SSID:
            ret = wlan_getwildcardssid(priv, wrq);
            break;
        }
        break;

    case WLAN_SETCONF_GETCONF:
        PRINTM(INFO, "The WLAN_SETCONF_GETCONF=0x%x is %d\n",
               WLAN_SETCONF_GETCONF, *(u8 *) req->ifr_data);
        switch (*(u8 *) req->ifr_data) {
        case CAL_DATA_EXT_CONFIG:
            ret = wlan_do_caldata_ext_ioctl(priv, req);
            break;
        case BG_SCAN_CONFIG:
            ret = wlan_do_bg_scan_config_ioctl(priv, req);
            break;

        case WMM_ACK_POLICY:
            ret = wlan_wmm_ack_policy_ioctl(priv, req);
            break;
        case WMM_PARA_IE:
            ret = wlan_wmm_para_ie_ioctl(priv, req);
            break;
        }
        break;

    case WLAN_SETNONE_GETONEINT:
        switch ((int) req->ifr_data) {
        case WLANGETBCNAVG:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->bcn_avg_factor;
            break;

        case WLANGETDATAAVG:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->data_avg_factor;
            break;

        case WLANGETREGION:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->RegionCode;
            break;

        case WLAN_GET_LISTEN_INTERVAL:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->ListenInterval;
            break;

        case WLAN_GET_MULTIPLE_DTIM:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->MultipleDtim;
            break;
        case WLAN_GET_TX_RATE:
            ret = wlan_get_txrate_ioctl(priv, req);
            break;
        default:
            ret = -EOPNOTSUPP;

        }

        break;

    case WLANGETLOG:
        ret = wlan_do_getlog_ioctl(priv, wrq);
        break;

    case WLAN_SET_GET_SIXTEEN_INT:
        switch ((int) wrq->u.data.flags) {
        case WLAN_TPCCFG:
            {
                int data[5];
                HostCmd_DS_802_11_TPC_CFG cfg;
                memset(&cfg, 0, sizeof(cfg));
                if ((wrq->u.data.length > 1) && (wrq->u.data.length != 5))
                    return WLAN_STATUS_FAILURE;

                if (wrq->u.data.length == 0) {
                    cfg.Action = wlan_cpu_to_le16(HostCmd_ACT_GEN_GET);
                } else {
                    if (copy_from_user
                        (data, wrq->u.data.pointer, sizeof(int) * 5)) {
                        PRINTM(INFO, "Copy from user failed\n");
                        return -EFAULT;
                    }

                    cfg.Action = wlan_cpu_to_le16(HostCmd_ACT_GEN_SET);
                    cfg.Enable = data[0];
                    cfg.UseSNR = data[1];
#define TPC_DATA_NO_CHANG	0x7f
                    if (wrq->u.data.length == 1) {
                        cfg.P0 = TPC_DATA_NO_CHANG;
                        cfg.P1 = TPC_DATA_NO_CHANG;
                        cfg.P2 = TPC_DATA_NO_CHANG;
                    } else {
                        cfg.P0 = data[2];
                        cfg.P1 = data[3];
                        cfg.P2 = data[4];
                    }
                }

                ret =
                    PrepareAndSendCommand(priv, HostCmd_CMD_802_11_TPC_CFG, 0,
                                          HostCmd_OPTION_WAITFORRSP, 0,
                                          (void *) &cfg);

                data[0] = cfg.Enable;
                data[1] = cfg.UseSNR;
                data[2] = cfg.P0;
                data[3] = cfg.P1;
                data[4] = cfg.P2;
                if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 5)) {
                    PRINTM(INFO, "Copy to user failed\n");
                    return -EFAULT;
                }

                wrq->u.data.length = 5;
            }
            break;
        case WLAN_AUTO_FREQ_SET:
            {
                int data[3];
                HostCmd_DS_802_11_AFC afc;
                memset(&afc, 0, sizeof(afc));
                if (wrq->u.data.length != 3)
                    return WLAN_STATUS_FAILURE;
                if (copy_from_user
                    (data, wrq->u.data.pointer, sizeof(int) * 3)) {
                    PRINTM(INFO, "Copy from user failed\n");
                    return -EFAULT;
                }
                afc.afc_auto = data[0];

                if (afc.afc_auto != 0) {
                    afc.afc_thre = data[1];
                    afc.afc_period = data[2];
                } else {
                    afc.afc_toff = data[1];
                    afc.afc_foff = data[2];
                }
                ret =
                    PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SET_AFC, 0,
                                          HostCmd_OPTION_WAITFORRSP, 0,
                                          (void *) &afc);
            }
            break;
        case WLAN_AUTO_FREQ_GET:
            {
                int data[3];
                HostCmd_DS_802_11_AFC afc;
                memset(&afc, 0, sizeof(afc));
                ret =
                    PrepareAndSendCommand(priv, HostCmd_CMD_802_11_GET_AFC, 0,
                                          HostCmd_OPTION_WAITFORRSP, 0,
                                          (void *) &afc);
                data[0] = afc.afc_auto;
                data[1] = afc.afc_toff;
                data[2] = afc.afc_foff;
                if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 3)) {
                    PRINTM(INFO, "Copy to user failed\n");
                    return -EFAULT;
                }

                wrq->u.data.length = 3;
            }
            break;
        case WLAN_SCANPROBES:
            {
                int data;
                if (wrq->u.data.length > 0) {
                    if (copy_from_user
                        (&data, wrq->u.data.pointer, sizeof(int))) {
                        PRINTM(INFO, "Copy from user failed\n");
                        return -EFAULT;
                    }

                    Adapter->ScanProbes = data;
                } else {
                    data = Adapter->ScanProbes;
                    if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
                        PRINTM(INFO, "Copy to user failed\n");
                        return -EFAULT;
                    }
                }
                wrq->u.data.length = 1;
            }
            break;
        case WLAN_LED_GPIO_CTRL:
            {
                int i;
                int data[16];

                HostCmd_DS_802_11_LED_CTRL ctrl;
                MrvlIEtypes_LedGpio_t *gpio =
                    (MrvlIEtypes_LedGpio_t *) ctrl.data;

                memset(&ctrl, 0, sizeof(ctrl));
                if (wrq->u.data.length > MAX_LEDS * 2)
                    return -ENOTSUPP;
                if ((wrq->u.data.length % 2) != 0)
                    return -ENOTSUPP;
                if (wrq->u.data.length == 0) {
                    ctrl.Action = wlan_cpu_to_le16(HostCmd_ACT_GEN_GET);
                } else {
                    if (copy_from_user
                        (data, wrq->u.data.pointer,
                         sizeof(int) * wrq->u.data.length)) {
                        PRINTM(INFO, "Copy from user failed\n");
                        return -EFAULT;
                    }

                    ctrl.Action = wlan_cpu_to_le16(HostCmd_ACT_GEN_SET);
                    ctrl.NumLed = wlan_cpu_to_le16(0);
                    gpio->Header.Type = wlan_cpu_to_le16(TLV_TYPE_LED_GPIO);
                    gpio->Header.Len = wrq->u.data.length;
                    for (i = 0; i < wrq->u.data.length; i += 2) {
                        gpio->LedPin[i / 2].Led = data[i];
                        gpio->LedPin[i / 2].Pin = data[i + 1];
                    }
                }
                ret =
                    PrepareAndSendCommand(priv,
                                          HostCmd_CMD_802_11_LED_GPIO_CTRL, 0,
                                          HostCmd_OPTION_WAITFORRSP, 0,
                                          (void *) &ctrl);
                for (i = 0; i < gpio->Header.Len; i += 2) {
                    data[i] = gpio->LedPin[i / 2].Led;
                    data[i + 1] = gpio->LedPin[i / 2].Pin;
                }
                if (copy_to_user(wrq->u.data.pointer, data,
                                 sizeof(int) * gpio->Header.Len)) {
                    PRINTM(INFO, "Copy to user failed\n");
                    return -EFAULT;
                }

                wrq->u.data.length = gpio->Header.Len;
            }
            break;
        case WLAN_SLEEP_PERIOD:
            ret = wlan_sleep_period(priv, wrq);
            break;
        case WLAN_ADAPT_RATESET:
            ret = wlan_adapt_rateset(priv, wrq);
            break;
        case WLAN_INACTIVITY_TIMEOUT:
            ret = wlan_inactivity_timeout(priv, wrq);
            break;
        case WLANSNR:
            ret = wlan_get_snr(priv, wrq);
            break;
        case WLAN_GET_RATE:
            ret = wlan_getrate_ioctl(priv, wrq);
            break;
        case WLAN_GET_RXINFO:
            ret = wlan_get_rxinfo(priv, wrq);
            break;
        case WLAN_SET_ATIM_WINDOW:
            ret = wlan_ATIM_Window(priv, wrq);
            break;
        case WLAN_BEACON_INTERVAL:
            ret = wlan_beacon_interval(priv, wrq);
            break;
        case WLAN_SCAN_TIME:
            ret = wlan_scan_time(priv, wrq);
            break;
        case WLAN_DATA_SUBSCRIBE_EVENT:
            ret = wlan_data_subscribe_event(priv, wrq);
            break;
        case WLANHSCFG:
            ret = wlan_hscfg_ioctl(priv, wrq);
            break;
        case WLAN_INACTIVITY_TIMEOUT_EXT:
            ret = wlan_inactivity_timeout_ext(priv, wrq);
            break;
#ifdef DEBUG_LEVEL1
        case WLAN_DRV_DBG:
            ret = wlan_drv_dbg(priv, wrq);
            break;
#endif
#ifdef WPRM_DRV
        case WLAN_SESSIONTYPE:
            {
                int *data = (int *) wrq->u.data.pointer;
                WPRM_SESSION_SPEC_S session;
                memset(&session, 0, sizeof(session));

                if ((wrq->u.data.length != 3))
                    return -1;

                session.pid = current->pid;
                session.sid = data[0];
                session.type = data[1];
                session.act = data[2];
                ret = wprm_session_type_action(&session);
            }
            break;
#endif

        }
        break;

    case WLAN_SET_GET_2K:
        switch ((int) wrq->u.data.flags) {
        case WLAN_SET_USER_SCAN:
            ret = wlan_set_user_scan_ioctl(priv, wrq);
            break;
        case WLAN_GET_SCAN_TABLE:
            ret = wlan_get_scan_table_ioctl(priv, wrq);
            break;

        case WLAN_SET_MRVL_TLV:
            ret = wlan_set_mrvl_tlv_ioctl(priv, wrq);
            break;
        case WLAN_GET_ASSOC_RSP:
            ret = wlan_get_assoc_rsp_ioctl(priv, wrq);
            break;
        case WLAN_ADDTS_REQ:
            ret = wlan_wmm_addts_req_ioctl(priv, wrq);
            break;
        case WLAN_DELTS_REQ:
            ret = wlan_wmm_delts_req_ioctl(priv, wrq);
            break;
        case WLAN_QUEUE_CONFIG:
            ret = wlan_wmm_queue_config_ioctl(priv, wrq);
            break;
        case WLAN_QUEUE_STATS:
            ret = wlan_wmm_queue_stats_ioctl(priv, wrq);
            break;
        case WLAN_GET_CFP_TABLE:
            ret = wlan_get_cfp_table_ioctl(priv, wrq);
            break;
        default:
            ret = -EOPNOTSUPP;
        }
        break;

    default:
        ret = -EINVAL;
        break;
    }
    LEAVE();
    return ret;
}

/** 
 *  @brief Get wireless statistics
 *
 *  NOTE: If PrepareAndSendCommand() with wait option is issued 
 *    in this function, a kernel dump (scheduling while atomic) 
 *    issue may happen on some versions of kernels.
 *
 *  @param dev		A pointer to net_device structure
 *  @return 	   	A pointer to iw_statistics buf
 */
struct iw_statistics *
wlan_get_wireless_stats(struct net_device *dev)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return NULL;
    }

    priv->wstats.status = Adapter->InfrastructureMode;
    priv->wstats.discard.retries = priv->stats.tx_errors;

    /* send RSSI command to get beacon RSSI/NF, valid only if associated */
    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_RSSI, 0, 0, 0, NULL);

    priv->wstats.qual.level =
        CAL_RSSI(Adapter->SNR[TYPE_BEACON][TYPE_NOAVG],
                 Adapter->NF[TYPE_BEACON][TYPE_NOAVG]);
    priv->wstats.qual.noise = CAL_NF(Adapter->NF[TYPE_BEACON][TYPE_NOAVG]);
    if (Adapter->NF[TYPE_BEACON][TYPE_NOAVG] == 0
        && Adapter->MediaConnectStatus == WlanMediaStateConnected)
        priv->wstats.qual.noise = MRVDRV_NF_DEFAULT_SCAN_VALUE;
    else
        priv->wstats.qual.noise =
            CAL_NF(Adapter->NF[TYPE_BEACON][TYPE_NOAVG]);
    priv->wstats.qual.qual = 0;

    PRINTM(INFO, "Signal Level = %#x\n", priv->wstats.qual.level);
    PRINTM(INFO, "Noise = %#x\n", priv->wstats.qual.noise);

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_GET_LOG, 0,
                                0, 0, NULL);

    if (!ret) {
        priv->wstats.discard.code = Adapter->LogMsg.wepundecryptable;
        priv->wstats.discard.fragment = Adapter->LogMsg.fcserror;
        priv->wstats.discard.retries = Adapter->LogMsg.retry;
        priv->wstats.discard.misc = Adapter->LogMsg.ackfailure;
    }

    return &priv->wstats;
}

static int
wlan_set_coalescing_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int data;
    int *val;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));

    switch (data) {
    case CMD_DISABLED:
    case CMD_ENABLED:
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_IBSS_COALESCING_STATUS,
                                    HostCmd_ACT_SET,
                                    HostCmd_OPTION_WAITFORRSP, 0, &data);
        if (ret) {
            LEAVE();
            return ret;
        }
        break;

    case CMD_GET:
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_IBSS_COALESCING_STATUS,
                                    HostCmd_ACT_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0, &data);
        if (ret) {
            LEAVE();
            return ret;
        }
        break;

    default:
        return -EINVAL;
    }

    val = (int *) wrq->u.name;
    *val = data;

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set frequency
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @param info			A pointer to iw_request_info structure 
 *  @param fwrq			A pointer to iw_freq structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
int
wlan_set_freq(struct net_device *dev, struct iw_request_info *info,
              struct iw_freq *fwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    int rc = -EINPROGRESS;      /* Call commit handler */
    CHANNEL_FREQ_POWER *cfp;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }
    if (Adapter->InfrastructureMode != Wlan802_11IBSS)
        return -EOPNOTSUPP;

    /*
     * If setting by frequency, convert to a channel 
     */
    if (fwrq->e == 1) {

        long f = fwrq->m / 100000;
        int c = 0;

        cfp = find_cfp_by_band_and_freq(Adapter, 0, f);
        if (!cfp) {
            PRINTM(INFO, "Invalid freq=%ld\n", f);
            return -EINVAL;
        }

        c = (int) cfp->Channel;

        if (c < 0)
            return -EINVAL;

        fwrq->e = 0;
        fwrq->m = c;
    }

    /*
     * Setting by channel number 
     */
    if (fwrq->m > 1000 || fwrq->e > 0) {
        rc = -EOPNOTSUPP;
    } else {
        int channel = fwrq->m;

        cfp = find_cfp_by_band_and_channel(Adapter, 0, (u16) channel);
        if (!cfp) {
            rc = -EINVAL;
        } else {
            rc = ChangeAdhocChannel(priv, channel);
            /*  If station is WEP enabled, send the 
             *  command to set WEP in firmware
             */
            if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled) {
                PRINTM(INFO, "set_freq: WEP Enabled\n");
                ret = PrepareAndSendCommand(priv,
                                            HostCmd_CMD_802_11_SET_WEP,
                                            0, HostCmd_OPTION_WAITFORRSP,
                                            OID_802_11_ADD_WEP, NULL);

                if (ret) {
                    LEAVE();
                    return ret;
                }
                Adapter->CurrentPacketFilter |= HostCmd_ACT_MAC_WEP_ENABLE;
                SetMacPacketFilter(priv);
            }
        }
    }

    LEAVE();
    return rc;
}

/** 
 *  @brief Set Deep Sleep 
 *   
 *  @param adapter 	A pointer to wlan_private structure
 *  @param bDeepSleep	TRUE--enalbe deepsleep, FALSE--disable deepsleep
 *  @return 	   	WLAN_STATUS_SUCCESS-success, otherwise fail
 */

int
SetDeepSleep(wlan_private * priv, BOOLEAN bDeepSleep)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (bDeepSleep == TRUE) {
        if (Adapter->IsDeepSleep != TRUE) {
            PRINTM(INFO, "Deep Sleep : sleep\n");

            // note: the command could be queued and executed later
            //       if there is command in prigressing.
            ret = PrepareAndSendCommand(priv,
                                        HostCmd_CMD_802_11_DEEP_SLEEP, 0,
                                        HostCmd_OPTION_WAITFORRSP, 0, NULL);

            if (ret) {
                LEAVE();
                return ret;
            }
            os_stop_queue(priv);
            os_carrier_off(priv);
	    
#ifdef ENABLE_PM	    
	    if (wlan_mpm_active == TRUE) 
            {
                if (wlan_mpm_advice_id > 0 ) {
                    mpm_driver_advise(wlan_mpm_advice_id, MPM_ADVICE_DRIVER_IS_NOT_BUSY);
                    PRINTM(MOTO_DEBUG, "EnterDeepSleep: driver is NOT busy sent to MPM\n");
		    wlan_mpm_active = FALSE;
                }
            }
#endif
        }
    } else {
        if (Adapter->IsDeepSleep == TRUE) {
            PRINTM(INFO, "Deep Sleep : wakeup\n");

            if (Adapter->IntCounterSaved)
                Adapter->IntCounter = Adapter->IntCounterSaved;

            if (sbi_exit_deep_sleep(priv))
                PRINTM(ERROR, "Deep Sleep : wakeup failed\n");

            if (Adapter->IsDeepSleep == TRUE) {

                if (os_wait_interruptible_timeout(Adapter->ds_awake_q,
                                                  !Adapter->IsDeepSleep,
                                                  WAIT_FOR_SCAN_RRESULT_MAX_TIME)
                    == 0) {
#ifdef MOTO_DBG
                    PRINTM(ERROR,
                           "SetDeepSleep: ds_awake_q: timer expired\n");
#else
                    PRINTM(MSG, "ds_awake_q: timer expired\n");
#endif
                    sbi_reset_deepsleep_wakeup(priv);
                    Adapter->IsDeepSleep = FALSE;
                    Adapter->bWakeupDevRequired = FALSE;
#ifdef MOTO_DBG
                    PRINTM(INFO,
                           "SetDeepSleep: bWakeupDevRequired set to FALSE - KO for DSM\n");
#endif
                    Adapter->WakeupTries = 0;
                    wlan_fatal_error_handle(priv);

                }
            }

            if (Adapter->IntCounter)
                wake_up_interruptible(&priv->MainThread.waitQ);
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief use index to get the data rate
 *   
 *  @param index                The index of data rate
 *  @return 	   		data rate or 0 
 */
u32
index_to_data_rate(u8 index)
{
    if (index >= sizeof(WlanDataRates))
        index = 0;

    return WlanDataRates[index];
}

/** 
 *  @brief use rate to get the index
 *   
 *  @param rate                 data rate
 *  @return 	   		index or 0 
 */
u8
data_rate_to_index(u32 rate)
{
    u8 *ptr;

    if (rate)
        if ((ptr = wlan_memchr(WlanDataRates, (u8) rate,
                               sizeof(WlanDataRates))))
            return (ptr - WlanDataRates);

    return 0;
}

/** 
 *  @brief set data rate
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */

int
wlan_set_rate(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    u32 data_rate;
    int ret = WLAN_STATUS_SUCCESS;
    WLAN_802_11_RATES rates;
    u8 *rate;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    PRINTM(INFO, "Vwrq->value = %d\n", vwrq->value);

    if (vwrq->value == -1) {
        Adapter->DataRate = 0;
        Adapter->RateBitmap = 0;
        memset(rates, 0, sizeof(rates));
        get_active_data_rates(Adapter, rates);
        rate = rates;
        while (*rate) {
            Adapter->RateBitmap |= 1 << (data_rate_to_index(*rate & 0x7f));
            rate++;
        }
        Adapter->Is_DataRate_Auto = TRUE;
    } else {
        if ((vwrq->value % 500000)) {
            return -EINVAL;
        }

        data_rate = vwrq->value / 500000;

        memset(rates, 0, sizeof(rates));
        get_active_data_rates(Adapter, rates);
        rate = rates;
        while (*rate) {
            PRINTM(INFO, "Rate=0x%X  Wanted=0x%X\n", *rate, data_rate);
            if ((*rate & 0x7f) == (data_rate & 0x7f))
                break;
            rate++;
        }
        if (!*rate) {
            PRINTM(MSG, "The fixed data rate 0x%X is out "
                   "of range.\n", data_rate);
            return -EINVAL;
        }

        Adapter->DataRate = data_rate;
        Adapter->RateBitmap = 1 << (data_rate_to_index(Adapter->DataRate));
        Adapter->Is_DataRate_Auto = FALSE;
    }

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
                                HostCmd_ACT_GEN_SET,
                                HostCmd_OPTION_WAITFORRSP, 0, NULL);

    LEAVE();
    return ret;
}

/** 
 *  @brief get data rate
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_get_rate(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    if (Adapter->Is_DataRate_Auto)
        vwrq->fixed = 0;
    else
        vwrq->fixed = 1;

    Adapter->TxRate = 0;

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_TX_RATE_QUERY,
                                HostCmd_ACT_GET, HostCmd_OPTION_WAITFORRSP,
                                0, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }
    vwrq->value = index_to_data_rate(Adapter->TxRate) * 500000;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief set wireless mode 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_mode(struct net_device *dev,
              struct iw_request_info *info, u32 * uwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    WLAN_802_11_NETWORK_INFRASTRUCTURE WantedMode;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    switch (*uwrq) {
    case IW_MODE_ADHOC:
        PRINTM(INFO, "Wanted Mode is ad-hoc: current DataRate=%#x\n",
               Adapter->DataRate);
        WantedMode = Wlan802_11IBSS;
        Adapter->AdhocChannel = DEFAULT_AD_HOC_CHANNEL;
        if (!find_cfp_by_band_and_channel
            (Adapter, 0, (u16) Adapter->AdhocChannel)) {
            CHANNEL_FREQ_POWER *cfp;
            cfp =
                find_cfp_by_band_and_channel(Adapter, 0, FIRST_VALID_CHANNEL);
            if (cfp)
                Adapter->AdhocChannel = cfp->Channel;
        }
        break;

    case IW_MODE_INFRA:
        PRINTM(INFO, "Wanted Mode is Infrastructure\n");
        WantedMode = Wlan802_11Infrastructure;
        break;

    case IW_MODE_AUTO:
        PRINTM(INFO, "Wanted Mode is Auto\n");
        WantedMode = Wlan802_11AutoUnknown;
        break;

    default:
        PRINTM(INFO, "Wanted Mode is Unknown: 0x%x\n", *uwrq);
        return -EINVAL;
    }

    if (Adapter->InfrastructureMode == WantedMode ||
        WantedMode == Wlan802_11AutoUnknown) {
        PRINTM(INFO, "Already set to required mode! No change!\n");

        Adapter->InfrastructureMode = WantedMode;

        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
        if (Adapter->PSState != PS_STATE_FULL_POWER) {
            PSWakeup(priv, HostCmd_OPTION_WAITFORRSP);
        }
        Adapter->PSMode = Wlan802_11PowerModeCAM;
    }

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
            ret = SendDeauthentication(priv);

            if (ret) {
                LEAVE();
                return ret;
            }
        } else if (Adapter->InfrastructureMode == Wlan802_11IBSS) {
            /* If current mode is Adhoc, clean stale information */
            ret = StopAdhocNetwork(priv);

            if (ret) {
                LEAVE();
                return ret;
            }
        }
    }

    if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled) {
        /* If there is a key with the specified SSID, 
         * send REMOVE WEP command, to make sure we clean up
         * the WEP keys in firmware
         */
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_SET_WEP,
                                    0, HostCmd_OPTION_WAITFORRSP,
                                    OID_802_11_REMOVE_WEP, NULL);

        if (ret) {
            LEAVE();
            return ret;
        }

        Adapter->CurrentPacketFilter &= ~HostCmd_ACT_MAC_WEP_ENABLE;

        SetMacPacketFilter(priv);
    }

    Adapter->SecInfo.WEPStatus = Wlan802_11WEPDisabled;
    Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;

    Adapter->InfrastructureMode = WantedMode;

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_SNMP_MIB,
                                0, HostCmd_OPTION_WAITFORRSP,
                                OID_802_11_INFRASTRUCTURE_MODE, NULL);

    if (ret) {
        LEAVE();
        return ret;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Encryption key
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_encode(struct net_device *dev,
                struct iw_request_info *info,
                struct iw_point *dwrq, char *extra)
{

    PWLAN_802_11_KEY pKey = NULL;

    ENTER();

    if (dwrq->length > MAX_KEY_SIZE) {
        pKey = (PWLAN_802_11_KEY) extra;

        if (pKey->KeyLength <= MAX_KEY_SIZE) {
            //dynamic WEP
            dwrq->length = pKey->KeyLength;
            dwrq->flags = pKey->KeyIndex + 1;
            return wlan_set_encode_nonwpa(dev, info, dwrq, pKey->KeyMaterial);
        } else {
            //WPA
            return wlan_set_encode_wpa(dev, info, dwrq, extra);
        }
    } else {
        //static WEP
        PRINTM(INFO, "Setting WEP\n");
        return wlan_set_encode_nonwpa(dev, info, dwrq, extra);
    }

    return -EINVAL;
}

/** 
 *  @brief set tx power 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_txpow(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    u16 dbm;

    ENTER();

    if (!Is_Command_Allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        return -EBUSY;
    }

    if (vwrq->disabled) {
        wlan_radio_ioctl(priv, RADIO_OFF);
        return WLAN_STATUS_SUCCESS;
    }

    Adapter->Preamble = HostCmd_TYPE_AUTO_PREAMBLE;

    wlan_radio_ioctl(priv, RADIO_ON);

#if WIRELESS_EXT > 14
    if ((vwrq->flags & IW_TXPOW_TYPE) == IW_TXPOW_MWATT) {
        dbm = (u16) mw_to_dbm(vwrq->value);
    } else
#endif
        dbm = (u16) vwrq->value;

    if ((dbm < Adapter->MinTxPowerLevel) || (dbm > Adapter->MaxTxPowerLevel)) {
        PRINTM(MSG,
               "The set txpower value %d dBm is out of range (%d dBm-%d dBm)!\n",
               dbm, Adapter->MinTxPowerLevel, Adapter->MaxTxPowerLevel);
        LEAVE();
        return -EINVAL;
    }

    /* auto tx power control */

    if (vwrq->fixed == 0)
        dbm = 0xffff;

    PRINTM(INFO, "<1>TXPOWER SET %d dbm.\n", dbm);

    ret = PrepareAndSendCommand(priv,
                                HostCmd_CMD_802_11_RF_TX_POWER,
                                HostCmd_ACT_GEN_SET,
                                HostCmd_OPTION_WAITFORRSP, 0, (void *) &dbm);

    LEAVE();
    return ret;
}

/** 
 *  @brief Get current essid 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_get_essid(struct net_device *dev, struct iw_request_info *info,
               struct iw_point *dwrq, char *extra)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();
    /*
     * Note : if dwrq->flags != 0, we should get the relevant SSID from
     * the SSID list... 
     */

    /*
     * Get the current SSID 
     */
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        memcpy(extra, Adapter->CurBssParams.ssid.Ssid,
               Adapter->CurBssParams.ssid.SsidLength);
        extra[Adapter->CurBssParams.ssid.SsidLength] = '\0';
    } else {
        memset(extra, 0, 32);
        extra[Adapter->CurBssParams.ssid.SsidLength] = '\0';
    }
    /*
     * If none, we may want to get the one that was set 
     */

    /* To make the driver backward compatible with WPA supplicant v0.2.4 */
    if (dwrq->length == 32)     /* check with WPA supplicant buffer size */
        dwrq->length = MIN(Adapter->CurBssParams.ssid.SsidLength,
                           IW_ESSID_MAX_SIZE);
    else
        dwrq->length = Adapter->CurBssParams.ssid.SsidLength + 1;

    dwrq->flags = 1;            /* active */

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get version 
 *   
 *  @param adapter              A pointer to wlan_adapter structure
 *  @param version		A pointer to version buffer
 *  @param maxlen		max length of version buffer
 *  @return 	   		NA
 */
void
get_version(wlan_adapter * adapter, char *version, int maxlen)
{
    union
    {
        u32 l;
        u8 c[4];
    } ver;
    char fwver[32];

    ver.l = adapter->FWReleaseNumber;
    if (ver.c[3] == 0)
        sprintf(fwver, "%u.%u.%u", ver.c[2], ver.c[1], ver.c[0]);
    else
        sprintf(fwver, "%u.%u.%u.p%u",
                ver.c[2], ver.c[1], ver.c[0], ver.c[3]);

    snprintf(version, maxlen, driver_version, fwver);
}
