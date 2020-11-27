/** @file wlan_fw.c
  * @brief This file contains the initialization for FW
  * and HW
  *
  * (c) Copyright © 2003-2007, Marvell International Ltd. 
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
	09/28/05: Add Doxygen format comments
	01/05/06: Add kernel 2.6.x support	
	01/11/06: Conditionalize new scan/join functions.
	          Cleanup association response handler initialization.
	01/06/05: Add FW file read
	05/08/06: Remove the 2nd GET_HW_SPEC command and TempAddr/PermanentAddr
	06/30/06: replaced MODULE_PARM(name, type) with module_param(name, type, perm)

********************************************************/

#include	"include.h"
#include <linux/vmalloc.h>

/********************************************************
		Local Variables
********************************************************/

#ifdef MFG_CMD_SUPPORT
int mfgmode = 0;
module_param(mfgmode, int, 0);
#endif

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief This function downloads firmware image, gets
 *  HW spec from firmware and set basic parameters to
 *  firmware.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_setup_station_hw(wlan_private * priv)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *adapter = priv->adapter;

    struct net_device *dev = priv->wlan_dev.netdev;

    ENTER();
    if (netif_running(dev))
        netif_device_detach(dev);

    sbi_disable_host_int(priv);

    /* Download the helper */
    ret = sbi_prog_helper(priv);

    if (ret) {
        PRINTM(INFO, "Bootloader in invalid state!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
    /* Download the main firmware via the helper firmware */
    if (sbi_prog_firmware_w_helper(priv)) {
        PRINTM(INFO, "Wlan FW download failed!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* check if the fimware is downloaded successfully or not */
    if (sbi_verify_fw_download(priv)) {
        PRINTM(INFO, "FW failed to be active in time!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    priv->adapter->fwstate = FW_STATE_READY;
    if (netif_running(dev))
        netif_device_attach(dev);
#define RF_REG_OFFSET 0x07
#define RF_REG_VALUE  0xc8

    sbi_enable_host_int(priv);

#ifdef MFG_CMD_SUPPORT
    if (mfgmode == 0) {
#endif

        /*
         * Read MAC address from HW
         */
        memset(adapter->CurrentAddr, 0xff, MRVDRV_ETH_ADDR_LEN);

        ret = PrepareAndSendCommand(priv, HostCmd_CMD_GET_HW_SPEC,
                                    0, HostCmd_OPTION_WAITFORRSP, 0, NULL);

        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }

        SetMacPacketFilter(priv);

        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_FW_WAKEUP_METHOD,
                                    HostCmd_ACT_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0,
                                    &priv->adapter->fwWakeupMethod);

        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
#ifdef MFG_CMD_SUPPORT
    }
#endif

#ifdef MFG_CMD_SUPPORT
    if (mfgmode == 0) {
#endif
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
                                    HostCmd_ACT_GEN_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0, NULL);
        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
        priv->adapter->DataRate = 0;
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_RF_TX_POWER,
                                    HostCmd_ACT_GEN_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0, NULL);

        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
#ifdef MFG_CMD_SUPPORT
    }
#endif

    ret = WLAN_STATUS_SUCCESS;
  done:

    LEAVE();

    return (ret);
}

/** 
 *  @brief This function initializes timers.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   n/a
 */
static void
init_sync_objects(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    InitializeTimer(&Adapter->MrvDrvCommandTimer,
                    MrvDrvCommandTimerFunction, priv);
    Adapter->CommandTimerIsSet = FALSE;

#ifdef REASSOCIATION
    /* Initialize the timer for the reassociation */
    InitializeTimer(&Adapter->MrvDrvTimer, MrvDrvTimerFunction, priv);
    Adapter->TimerIsSet = FALSE;
#endif /* REASSOCIATION */

    return;
}

/** 
 *  @brief This function allocates buffer for the member of adapter
 *  structure like command buffer and BSSID list.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_allocate_adapter(wlan_private * priv)
{
    u32 ulBufSize;
    wlan_adapter *Adapter = priv->adapter;

    BSSDescriptor_t *pTempScanTable;

    /* Allocate buffer to store the BSSID list */
    ulBufSize = sizeof(BSSDescriptor_t) * MRVDRV_MAX_BSSID_LIST;
    if (!(pTempScanTable = kmalloc(ulBufSize, GFP_KERNEL))) {
        return WLAN_STATUS_FAILURE;
    }

    Adapter->ScanTable = pTempScanTable;
    memset(Adapter->ScanTable, 0, ulBufSize);

    if (!(Adapter->bgScanConfig =
          kmalloc(sizeof(HostCmd_DS_802_11_BG_SCAN_CONFIG), GFP_KERNEL))) {
        return WLAN_STATUS_FAILURE;
    }
    Adapter->bgScanConfigSize = sizeof(HostCmd_DS_802_11_BG_SCAN_CONFIG);
    memset(Adapter->bgScanConfig, 0, Adapter->bgScanConfigSize);

    spin_lock_init(&Adapter->QueueSpinLock);

    /* Allocate the command buffers */
    if (AllocateCmdBuffer(priv) != WLAN_STATUS_SUCCESS) {
        return WLAN_STATUS_FAILURE;
    }

    memset(&Adapter->PSConfirmSleep, 0, sizeof(PS_CMD_ConfirmSleep));
    Adapter->PSConfirmSleep.SeqNum = wlan_cpu_to_le16(++Adapter->SeqNum);
    Adapter->PSConfirmSleep.Command =
        wlan_cpu_to_le16(HostCmd_CMD_802_11_PS_MODE);
    Adapter->PSConfirmSleep.Size =
        wlan_cpu_to_le16(sizeof(PS_CMD_ConfirmSleep));
    Adapter->PSConfirmSleep.Result = 0;
    Adapter->PSConfirmSleep.Action =
        wlan_cpu_to_le16(HostCmd_SubCmd_Sleep_Confirmed);

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function initializes the adapter structure
 *  and set default value to the member of adapter.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   n/a
 */
static void
wlan_init_adapter(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int i;

    Adapter->ScanProbes = 0;

    Adapter->bcn_avg_factor = DEFAULT_BCN_AVG_FACTOR;
    Adapter->data_avg_factor = DEFAULT_DATA_AVG_FACTOR;

    /* ATIM params */
    Adapter->AtimWindow = 0;
    Adapter->ATIMEnabled = FALSE;

    Adapter->MediaConnectStatus = WlanMediaStateDisconnected;
#ifdef WPRM_DRV
    WPRM_DRV_TRACING_PRINT();
    /* Stop Traffic meter */
    wprm_traffic_meter_exit(priv);
#endif
    Adapter->LinkSpeed = MRVDRV_LINK_SPEED_1mbps;
    memset(Adapter->CurrentAddr, 0xff, MRVDRV_ETH_ADDR_LEN);

    /* Status variables */
    Adapter->HardwareStatus = WlanHardwareStatusInitializing;

    /* scan type */
    Adapter->ScanType = HostCmd_SCAN_TYPE_ACTIVE;

    /* scan mode */
    Adapter->ScanMode = HostCmd_BSS_TYPE_ANY;

    /* scan time */
    Adapter->SpecificScanTime = MRVDRV_SPECIFIC_SCAN_CHAN_TIME;
    Adapter->ActiveScanTime = MRVDRV_ACTIVE_SCAN_CHAN_TIME;
    Adapter->PassiveScanTime = MRVDRV_PASSIVE_SCAN_CHAN_TIME;

    /* 802.11 specific */
    Adapter->SecInfo.WEPStatus = Wlan802_11WEPDisabled;
    for (i = 0; i < sizeof(Adapter->WepKey) / sizeof(Adapter->WepKey[0]); i++)
        memset(&Adapter->WepKey[i], 0, sizeof(MRVL_WEP_KEY));
    Adapter->CurrentWepKeyIndex = 0;
    Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;
    Adapter->SecInfo.EncryptionMode = CIPHER_NONE;
    Adapter->AdhocAESEnabled = FALSE;
    Adapter->InfrastructureMode = Wlan802_11Infrastructure;

    Adapter->NumInScanTable = 0;
    Adapter->pAttemptedBSSDesc = NULL;
#ifdef REASSOCIATION
    OS_INIT_SEMAPHORE(&Adapter->ReassocSem);
#endif
    Adapter->pBeaconBufEnd = Adapter->beaconBuffer;

    Adapter->Prescan = CMD_ENABLED;
    Adapter->HisRegCpy |= HIS_TxDnLdRdy;

    memset(&Adapter->CurBssParams, 0, sizeof(Adapter->CurBssParams));

    /* PnP and power profile */
    Adapter->SurpriseRemoved = FALSE;

    Adapter->CurrentPacketFilter =
        HostCmd_ACT_MAC_RX_ON | HostCmd_ACT_MAC_TX_ON;

    Adapter->RadioOn = RADIO_ON;
#ifdef REASSOCIATION
    Adapter->Reassoc_on = TRUE;
#endif /* REASSOCIATION */
    Adapter->TxAntenna = RF_ANTENNA_2;
    Adapter->RxAntenna = RF_ANTENNA_AUTO;

    Adapter->HWRateDropMode = HW_TABLE_RATE_DROP;
    Adapter->Is_DataRate_Auto = TRUE;
    Adapter->BeaconPeriod = MRVDRV_BEACON_INTERVAL;

    // set default value of capInfo.
#define SHORT_PREAMBLE_ALLOWED		1
    memset(&Adapter->capInfo, 0, sizeof(Adapter->capInfo));
    Adapter->capInfo.ShortPreamble = SHORT_PREAMBLE_ALLOWED;

    Adapter->AdhocChannel = DEFAULT_AD_HOC_CHANNEL;

    Adapter->PSMode = Wlan802_11PowerModeCAM;
    Adapter->MultipleDtim = MRVDRV_DEFAULT_MULTIPLE_DTIM;

    Adapter->ListenInterval = MRVDRV_DEFAULT_LISTEN_INTERVAL;

    Adapter->PSState = PS_STATE_FULL_POWER;
#ifdef MOTO_DBG
    PRINTM(INFO, "PSState set to PS_STATE_AWAKE - KO for DSM\n");
#endif
    Adapter->NeedToWakeup = FALSE;
    Adapter->LocalListenInterval = 0;   /* default value in firmware will be used */
    Adapter->fwWakeupMethod = WAKEUP_FW_UNCHANGED;

    Adapter->IsDeepSleep = FALSE;

    Adapter->bWakeupDevRequired = FALSE;
#ifdef MOTO_DBG
    PRINTM(INFO, "bWakeupDevRequired set to FALSE - KO for DSM\n");
#endif
    Adapter->bHostSleepConfigured = FALSE;
    Adapter->WakeupTries = 0;
#ifdef MOTO_DBG
    PRINTM(INFO, "WakeupTries set to 0 - OK for DSM\n");
#endif
    Adapter->HSCfg.conditions = HOST_SLEEP_CFG_CANCEL;
    Adapter->HSCfg.gpio = 0;
    Adapter->HSCfg.gap = 0;

    Adapter->DataRate = 0;      // Initially indicate the rate as auto 

    Adapter->adhoc_grate_enabled = FALSE;

    Adapter->IntCounter = Adapter->IntCounterSaved = 0;
    memset(&Adapter->wmm, 0, sizeof(WMM_DESC));
    for (i = 0; i < MAX_AC_QUEUES; i++)
        INIT_LIST_HEAD((struct list_head *) &Adapter->wmm.TxSkbQ[i]);
    INIT_LIST_HEAD((struct list_head *) &Adapter->RxSkbQ);
    Adapter->gen_null_pkg = TRUE;       /*Enable NULL Pkg generation */

    INIT_LIST_HEAD((struct list_head *) &Adapter->TxSkbQ);
    Adapter->TxSkbNum = 0;

    Adapter->fwstate = FW_STATE_NREADY;
    init_waitqueue_head(&Adapter->cmd_EncKey);

    Adapter->EncryptionStatus = Wlan802_11WEPDisabled;

    spin_lock_init(&Adapter->CurrentTxLock);

    Adapter->CurrentTxSkb = NULL;
    Adapter->PktTxCtrl = 0;

    return;
}

/********************************************************
		Global Functions
********************************************************/

/** 
 *  @brief This function initializes firmware
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_init_fw(wlan_private * priv)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    /* Allocate adapter structure */
    if ((ret = wlan_allocate_adapter(priv)) != WLAN_STATUS_SUCCESS) {
        goto done;
    }

    /* init adapter structure */
    wlan_init_adapter(priv);

    /* init timer etc. */
    init_sync_objects(priv);

    /* init 802.11d */
    wlan_init_11d(priv);

    Adapter->HardwareStatus = WlanHardwareStatusReady;
    ret = WLAN_STATUS_SUCCESS;
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function frees the structure of adapter
 *    
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   n/a
 */
void
wlan_free_adapter(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!Adapter) {
        PRINTM(INFO, "Why double free adapter?:)\n");
        return;
    }

    PRINTM(INFO, "Free Command buffer\n");
    FreeCmdBuffer(priv);

    PRINTM(INFO, "Free CommandTimer\n");
    if (Adapter->CommandTimerIsSet) {
        CancelTimer(&Adapter->MrvDrvCommandTimer);
        Adapter->CommandTimerIsSet = FALSE;
    }
    FreeTimer(&Adapter->MrvDrvCommandTimer);
#ifdef REASSOCIATION
    PRINTM(INFO, "Free MrvDrvTimer\n");
    if (Adapter->TimerIsSet) {
        CancelTimer(&Adapter->MrvDrvTimer);
        Adapter->TimerIsSet = FALSE;
    }
    FreeTimer(&Adapter->MrvDrvTimer);
#endif /* REASSOCIATION */

    if (Adapter->bgScanConfig) {
        kfree(Adapter->bgScanConfig);
        Adapter->bgScanConfig = NULL;
    }

    OS_FREE_LOCK(&Adapter->CurrentTxLock);
    OS_FREE_LOCK(&Adapter->QueueSpinLock);

    PRINTM(INFO, "Free ScanTable\n");
    if (Adapter->ScanTable) {
        kfree(Adapter->ScanTable);
        Adapter->ScanTable = NULL;
    }

    PRINTM(INFO, "Free Adapter\n");

    /* Free the adapter object itself */
    kfree(Adapter);
    priv->adapter = NULL;
    LEAVE();
}

/** 
 *  @brief This function handles the timeout of command sending.
 *  It will re-send the same command again.
 *  
 *  @param FunctionContext    A pointer to FunctionContext
 *  @return 	   n/a
 */
void
MrvDrvCommandTimerFunction(void *FunctionContext)
{
    wlan_private *priv = (wlan_private *) FunctionContext;
    wlan_adapter *Adapter = priv->adapter;
    CmdCtrlNode *pTempNode;
    HostCmd_DS_COMMAND *CmdPtr;

    ENTER();

    PRINTM(CMND, "Command timeout.\n");

    Adapter->CommandTimerIsSet = FALSE;

    if (!Adapter->num_cmd_timeout)
        Adapter->dbg.num_cmd_timeout++;

    pTempNode = Adapter->CurCmd;

    if (pTempNode == NULL) {
        PRINTM(INFO, "CurCmd Empty\n");
        goto exit;
    }

    CmdPtr = (HostCmd_DS_COMMAND *) pTempNode->BufVirtualAddr;
    if (CmdPtr == NULL) {
        goto exit;
    }

    if (CmdPtr->Size) {
        Adapter->dbg.TimeoutCmdId = wlan_cpu_to_le16(CmdPtr->Command);
        Adapter->dbg.TimeoutCmdAct =
            wlan_cpu_to_le16(*(u16 *) ((u8 *) CmdPtr + S_DS_GEN));
        PRINTM(CMND, "Timeout cmd = 0x%x, act = 0x%x\n",
               Adapter->dbg.TimeoutCmdId, Adapter->dbg.TimeoutCmdAct);
    }
#define MAX_CMD_TIMEOUT_COUNT	5
    Adapter->num_cmd_timeout++;
    if (Adapter->num_cmd_timeout > MAX_CMD_TIMEOUT_COUNT) {
        PRINTM(FATAL, "num_cmd_timeout=%d\n", Adapter->num_cmd_timeout);
        wlan_fatal_error_handle(priv);
        goto exit;
    }

    /* Restart the timer to trace command response again */
    ModTimer(&Adapter->MrvDrvCommandTimer, MRVDRV_TIMER_1S);
    Adapter->CommandTimerIsSet = TRUE;

    /* Wake up main thread to read int status register */
    Adapter->IntCounter++;
    wake_up_interruptible(&priv->MainThread.waitQ);

  exit:
    LEAVE();
    return;
}

#ifdef REASSOCIATION
/** 
 *  @brief This function triggers re-association by waking up
 *  re-assoc thread.
 *  
 *  @param FunctionContext    A pointer to FunctionContext
 *  @return 	   n/a
 */
void
MrvDrvTimerFunction(void *FunctionContext)
{
    wlan_private *priv = (wlan_private *) FunctionContext;
    wlan_adapter *Adapter = priv->adapter;
    OS_INTERRUPT_SAVE_AREA;

    ENTER();

    PRINTM(INFO, "MrvDrvTimer fired.\n");
    Adapter->TimerIsSet = FALSE;
    if (Adapter->PSState != PS_STATE_FULL_POWER) {
        /* wait until Exit_PS command returns */
        Adapter->TimerIsSet = TRUE;
        ModTimer(&Adapter->MrvDrvTimer, MRVDRV_TIMER_1S);
        PRINTM(INFO, "MrvDrvTimerFunction(PSState=%d) waiting"
               "for Exit_PS done\n", Adapter->PSState);
        LEAVE();
        return;
    }

    PRINTM(INFO, "Waking Up the Reassoc Thread\n");

    wake_up_interruptible(&priv->ReassocThread.waitQ);

    LEAVE();
    return;
}
#endif /* REASSOCIATION */

#define CUS_EVT_FATAL_ERR	"EVENT=FATAL_ERROR"
void
wlan_fatal_error_handle(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    ulong flags;
    CmdCtrlNode *CmdNode = NULL;

    ENTER();

    PRINTM(FATAL, "Fatal error handling...\n");

    //Stop reassociation 
    priv->adapter->Reassoc_on = FALSE;
    //stop access firmware
    priv->adapter->fwstate = FW_STATE_NREADY;
    if (priv->adapter->MediaConnectStatus == WlanMediaStateConnected) {
        union iwreq_data wrqu;

        //send disconnect event to app          
        memset(wrqu.ap_addr.sa_data, 0x00, ETH_ALEN);
        wrqu.ap_addr.sa_family = ARPHRD_ETHER;
        wireless_send_event(priv->wlan_dev.netdev, SIOCGIWAP, &wrqu, NULL);
#ifdef MOTO_DBG
        PRINTM(MOTO_MSG,
               "wlan_fatal_error_handle: Disconnected event sent to upper layer.\n");
#endif

        /* Free Tx and Rx packets */
        /* report disconnect to upper layer */
        wlan_clean_txrx(priv);
        Adapter->MediaConnectStatus = WlanMediaStateDisconnected;
    }
    //release ds_awake_q
    if (Adapter->IsDeepSleep == TRUE) {
        Adapter->IsDeepSleep = FALSE;
        wake_up_interruptible(&Adapter->ds_awake_q);
        sbi_reset_deepsleep_wakeup(priv);
    }
    //clean up host sleep flags
    Adapter->bWakeupDevRequired = FALSE;
#ifdef MOTO_DBG
    PRINTM(INFO, "bWakeupDevRequired set to FALSE - KO for DSM\n");
#endif
    Adapter->WakeupTries = 0;
#ifdef MOTO_DBG
    PRINTM(INFO, "WakeupTries set to 0 - OK for DSM\n");
#endif
    //Release all the pending command               
    if (priv->adapter->CurCmd) {
        /* set error code that will be transferred back to PrepareAndSendCommand() */
        Adapter->CurCmdRetCode = WLAN_STATUS_FAILURE;
        CleanupAndInsertCmd(priv, Adapter->CurCmd);
        spin_lock_irqsave(&Adapter->QueueSpinLock, flags);
        Adapter->CurCmd = NULL;
        spin_unlock_irqrestore(&Adapter->QueueSpinLock, flags);
    }

    while (TRUE) {
        spin_lock_irqsave(&Adapter->QueueSpinLock, flags);
        if (list_empty(&Adapter->CmdPendingQ)) {
            spin_unlock_irqrestore(&Adapter->QueueSpinLock, flags);
            break;
        }
        CmdNode = (CmdCtrlNode *) Adapter->CmdPendingQ.next;
        spin_unlock_irqrestore(&Adapter->QueueSpinLock, flags);
        if (!CmdNode)
            break;
        list_del((struct list_head *) CmdNode);
        CleanupAndInsertCmd(priv, CmdNode);
    }
    send_iwevcustom_event(priv, CUS_EVT_FATAL_ERR);
#ifdef MOTO_DBG
    PRINTM(MOTO_MSG,
           "wlan_fatal_error_handle: Fatal error event sent to upper layer.\n");
#endif

    LEAVE();
    return;
}
