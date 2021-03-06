/** @file wlan_dev.h
 *  @brief This file contains definitions and data structures specific
 *          to Marvell 802.11 NIC. It contains the Device Information
 *          structure wlan_adapter.  
 *  
 *  (c) Copyright ? 2003-2007, Marvell International Ltd. 
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
Change log:
	09/26/05: add Doxygen format comments 
	01/11/06: Conditionalize new scan/join structures.
	04/18/06: Remove old Subscrive Event and add new Subscribe Event
		  implementation through generic hostcmd API
	05/08/06: Remove PermanentAddr from Adapter

 ************************************************************/

#ifndef _WLAN_DEV_H_
#define _WLAN_DEV_H_

#define	MAX_BSSID_PER_CHANNEL		16

#define MAX_NUM_IN_TX_Q			3

/* For the extended Scan */
#define MAX_EXTENDED_SCAN_BSSID_LIST    MAX_BSSID_PER_CHANNEL * \
						MRVDRV_MAX_CHANNEL_SIZE + 1

typedef struct _PER_CHANNEL_BSSID_LIST_DATA
{
    u8 ucStart;
    u8 ucNumEntry;
} PER_CHANNEL_BSSID_LIST_DATA, *PPER_CHANNEL_BSSID_LIST_DATA;

typedef struct _MRV_BSSID_IE_LIST
{
    WLAN_802_11_FIXED_IEs FixedIE;
    u8 VariableIE[MRVDRV_SCAN_LIST_VAR_IE_SPACE];
} MRV_BSSID_IE_LIST, *PMRV_BSSID_IE_LIST;

#define	MAX_REGION_CHANNEL_NUM	2

/** Chan-Freq-TxPower mapping table*/
typedef struct _CHANNEL_FREQ_POWER
{
        /** Channel Number		*/
    u16 Channel;
        /** Frequency of this Channel	*/
    u32 Freq;
        /** Max allowed Tx power level	*/
    u16 MaxTxPower;
        /** TRUE:channel unsupported;  FLASE:supported*/
    BOOLEAN Unsupported;
} CHANNEL_FREQ_POWER;

/** region-band mapping table*/
typedef struct _REGION_CHANNEL
{
        /** TRUE if this entry is valid		     */
    BOOLEAN Valid;
        /** Region code for US, Japan ...	     */
    u8 Region;
        /** Band B/G/A, used for BAND_CONFIG cmd	     */
    u8 Band;
        /** Actual No. of elements in the array below */
    u8 NrCFP;
        /** chan-freq-txpower mapping table*/
    CHANNEL_FREQ_POWER *CFP;
} REGION_CHANNEL;

typedef struct _wlan_802_11_security_t
{
    BOOLEAN WPAEnabled;
    BOOLEAN WPA2Enabled;
    WLAN_802_11_WEP_STATUS WEPStatus;
    WLAN_802_11_AUTHENTICATION_MODE AuthenticationMode;
    WLAN_802_11_ENCRYPTION_MODE EncryptionMode;
} wlan_802_11_security_t;

/** Current Basic Service Set State Structure */
typedef struct CurrentBSSParams
{

    BSSDescriptor_t BSSDescriptor;
        /** bssid */
    u8 bssid[MRVDRV_ETH_ADDR_LEN];
        /** ssid */
    WLAN_802_11_SSID ssid;

        /** band */
    u8 band;
        /** channel */
    u8 channel;
        /** number of rates supported */
    int NumOfRates;
        /** supported rates*/
    u8 DataRates[WLAN_SUPPORTED_RATES];
        /** wmm enable? */
    u8 wmm_enabled;
        /** wmm queue priority table*/
    u8 wmm_queue_prio[MAX_AC_QUEUES];
        /** uapse enable?*/
    u8 wmm_uapsd_enabled;
} CurrentBSSParams;

/** sleep_params */
typedef struct SleepParams
{
    u16 sp_error;
    u16 sp_offset;
    u16 sp_stabletime;
    u8 sp_calcontrol;
    u8 sp_extsleepclk;
    u16 sp_reserved;
} SleepParams;

/** sleep_period */
typedef struct SleepPeriod
{
    u16 period;
    u16 reserved;
} SleepPeriod;

/** info for debug purpose */
typedef struct _wlan_dbg
{
    u32 num_cmd_host_to_card_failure;
    u32 num_cmd_sleep_cfm_host_to_card_failure;
    u32 num_tx_host_to_card_failure;
    u32 num_event_deauth;
    u32 num_event_disassoc;
    u32 num_event_link_lost;
    u32 num_cmd_deauth;
    u32 num_cmd_assoc_success;
    u32 num_cmd_assoc_failure;
    u32 num_tx_timeout;
    u32 num_cmd_timeout;
    u16 TimeoutCmdId;
    u16 TimeoutCmdAct;
    u16 LastCmdId;
    u16 LastCmdRespId;
} wlan_dbg;

/** Data structure for the Marvell WLAN device */
typedef struct _wlan_dev
{
        /** device name */
    char name[DEV_NAME_LEN];
        /** card pointer */
    void *card;
        /** IO port */
    u32 ioport;
        /** Upload received */
    u32 upld_rcv;
        /** Upload type */
    u32 upld_typ;
        /** Upload length */
    u32 upld_len;
        /** netdev pointer */
    struct net_device *netdev;
    /* Upload buffer */
    u8 upld_buf[WLAN_UPLD_SIZE];
    /* Download sent: 
       bit0 1/0=data_sent/data_tx_done, 
       bit1 1/0=cmd_sent/cmd_tx_done, 
       all other bits reserved 0 */
    u8 dnld_sent;
} wlan_dev_t, *pwlan_dev_t;

/** Private structure for the MV device */
struct _wlan_private
{
    int open;

    wlan_adapter *adapter;
    wlan_dev_t wlan_dev;

    struct net_device_stats stats;

    struct iw_statistics wstats;
    struct proc_dir_entry *proc_entry;
    struct proc_dir_entry *proc_dev;
    struct proc_dir_entry *proc_firmware;

        /** thread to service interrupts */
    wlan_thread MainThread;

#ifdef REASSOCIATION
        /** thread to service mac events */
    wlan_thread ReassocThread;
#endif                          /* REASSOCIATION */
};

/** Wlan Adapter data structure*/
struct _wlan_adapter
{
    u8 TmpTxBuf[WLAN_UPLD_SIZE] __ATTRIB_ALIGN__;
        /** STATUS variables */
    WLAN_HARDWARE_STATUS HardwareStatus;
    u32 FWReleaseNumber;
    u32 fwCapInfo;
    u8 chip_rev;

        /** Command-related variables */
    u16 SeqNum;
    CmdCtrlNode *CmdArray;
        /** Current Command */
    CmdCtrlNode *CurCmd;
    int CurCmdRetCode;

        /** Command Queues */
        /** Free command buffers */
    struct list_head CmdFreeQ;
        /** Pending command buffers */
    struct list_head CmdPendingQ;

        /** Variables brought in from private structure */
    int irq;

        /** Async and Sync Event variables */
    u32 IntCounter;
    u32 IntCounterSaved;        /* save int for DS/PS */
    u32 EventCause;
    u8 nodeName[16];            /* nickname */

        /** spin locks */
    spinlock_t QueueSpinLock __ATTRIB_ALIGN__;

        /** Timers */
    WLAN_DRV_TIMER MrvDrvCommandTimer __ATTRIB_ALIGN__;
    BOOLEAN CommandTimerIsSet;

#ifdef REASSOCIATION
        /**Reassociation timer*/
    BOOLEAN TimerIsSet;
    WLAN_DRV_TIMER MrvDrvTimer __ATTRIB_ALIGN__;
#endif                          /* REASSOCIATION */

        /** Event Queues */
    wait_queue_head_t ds_awake_q __ATTRIB_ALIGN__;

    u8 HisRegCpy;

#ifdef MFG_CMD_SUPPORT
        /** manf command related cmd variable*/
    u32 mfg_cmd_len;
    int mfg_cmd_flag;
    u32 mfg_cmd_resp_len;
    u8 *mfg_cmd;
    wait_queue_head_t mfg_cmd_q;
#endif

        /** bg scan related variable */
    pHostCmd_DS_802_11_BG_SCAN_CONFIG bgScanConfig;
    u32 bgScanConfigSize;

        /** WMM related variable*/
    WMM_DESC wmm;

        /** current ssid/bssid related parameters*/
    CurrentBSSParams CurBssParams;

    WLAN_802_11_NETWORK_INFRASTRUCTURE InfrastructureMode;

    BSSDescriptor_t *pAttemptedBSSDesc;

    WLAN_802_11_SSID AttemptedSSIDBeforeScan;
    WLAN_802_11_SSID PreviousSSID;
    u8 PreviousBSSID[MRVDRV_ETH_ADDR_LEN];

    BSSDescriptor_t *ScanTable;
    u32 NumInScanTable;

    u8 ScanType;
    u32 ScanMode;
    u16 SpecificScanTime;
    u16 ActiveScanTime;
    u16 PassiveScanTime;

    u16 BeaconPeriod;
    u8 AdhocCreate;
    BOOLEAN AdhocLinkSensed;

        /** Capability Info used in Association, start, join */
    IEEEtypes_CapInfo_t capInfo;

#ifdef REASSOCIATION
        /** Reassociation on and off */
    BOOLEAN Reassoc_on;
    SEMAPHORE ReassocSem;
#endif                          /* REASSOCIATION */

    BOOLEAN ATIMEnabled;

        /** MAC address information */
    u8 CurrentAddr[MRVDRV_ETH_ADDR_LEN];
    u8 MulticastList[MRVDRV_MAX_MULTICAST_LIST_SIZE]
        [MRVDRV_ETH_ADDR_LEN];
    u32 NumOfMulticastMACAddr;

        /** 802.11 statistics */
    HostCmd_DS_802_11_GET_STAT wlan802_11Stat;

    u16 HWRateDropMode;
    u16 RateBitmap;
    u16 Threshold;
    u16 FinalRate;
        /** control G Rates */
    BOOLEAN adhoc_grate_enabled;

    WLAN_802_11_ANTENNA TxAntenna;
    WLAN_802_11_ANTENNA RxAntenna;

    u8 AdhocChannel;
    WLAN_802_11_FRAGMENTATION_THRESHOLD FragThsd;
    WLAN_802_11_RTS_THRESHOLD RTSThsd;

    u32 DataRate;
    BOOLEAN Is_DataRate_Auto;

        /** number of association attempts for the current SSID cmd */
    u32 m_NumAssociationAttemp;
    u16 ListenInterval;
    u16 Prescan;
    u8 TxRetryCount;

        /** Tx-related variables (for single packet tx) */
    spinlock_t TxSpinLock __ATTRIB_ALIGN__;
    struct sk_buff *CurrentTxSkb;
    struct sk_buff RxSkbQ;
    struct sk_buff TxSkbQ;
    u32 TxSkbNum;
    BOOLEAN TxLockFlag;
    u16 gen_null_pkg;
    spinlock_t CurrentTxLock __ATTRIB_ALIGN__;

        /** NIC Operation characteristics */
    u32 LinkSpeed;
    u16 CurrentPacketFilter;
    u32 MediaConnectStatus;
    u16 RegionCode;
    u16 RegionTableIndex;
    u16 TxPowerLevel;
    u8 MaxTxPowerLevel;
    u8 MinTxPowerLevel;

        /** POWER MANAGEMENT AND PnP SUPPORT */
    BOOLEAN SurpriseRemoved;
    u16 AtimWindow;

    u16 PSMode;                 /* Wlan802_11PowerModeCAM=disable
                                   Wlan802_11PowerModeMAX_PSP=enable */
    u16 MultipleDtim;
    u16 BCNMissTimeOut;
    u32 PSState;
    BOOLEAN NeedToWakeup;

    PS_CMD_ConfirmSleep PSConfirmSleep;
    u16 LocalListenInterval;
    u16 NullPktInterval;
    u16 AdhocAwakePeriod;
    u16 fwWakeupMethod;
    BOOLEAN IsDeepSleep;
        /** Host wakeup parameter */
    BOOLEAN bWakeupDevRequired;
    BOOLEAN bHostSleepConfigured;
    HostCmd_DS_802_11_HOST_SLEEP_CFG HSCfg;
        /** ARP filter related variable */
    u8 ArpFilter[ARP_FILTER_MAX_BUF_SIZE];
    u32 ArpFilterSize;
    u32 WakeupTries;

        /** Encryption parameter */
    wlan_802_11_security_t SecInfo;

    MRVL_WEP_KEY WepKey[MRVL_NUM_WEP_KEY];
    u16 CurrentWepKeyIndex;
    u8 mrvlTlvBuffer[256];
    u8 mrvlTlvBufferLen;

    u8 assocRspBuffer[MRVDRV_ASSOC_RSP_BUF_SIZE];
    int assocRspSize;
    u8 genIeBuffer[256];
    u8 genIeBufferLen;
    WLAN_802_11_ENCRYPTION_STATUS EncryptionStatus;

    BOOLEAN IsGTK_SET;

        /** Encryption Key*/
    u8 Wpa_ie[256];
    u8 Wpa_ie_len;

    MRVL_WPA_KEY WpaPwkKey, WpaGrpKey;

    HostCmd_DS_802_11_KEY_MATERIAL aeskey;

    /* Advanced Encryption Standard */
    BOOLEAN AdhocAESEnabled;
    wait_queue_head_t cmd_EncKey __ATTRIB_ALIGN__;

    u16 RxAntennaMode;
    u16 TxAntennaMode;

        /** Requested Signal Strength*/
    u16 bcn_avg_factor;
    u16 data_avg_factor;
    u16 SNR[MAX_TYPE_B][MAX_TYPE_AVG];
    u16 NF[MAX_TYPE_B][MAX_TYPE_AVG];
    u8 RSSI[MAX_TYPE_B][MAX_TYPE_AVG];
    u8 rawSNR[DEFAULT_DATA_AVG_FACTOR];
    u8 rawNF[DEFAULT_DATA_AVG_FACTOR];
    u16 nextSNRNF;
    u16 numSNRNF;
    u32 RxPDAge;
    u16 RxPDRate;

    BOOLEAN RadioOn;
    u32 Preamble;

        /** Blue Tooth Co-existence Arbitration */
    HostCmd_DS_802_11_BCA_TIMESHARE bca_ts;

        /** sleep_params */
    SleepParams sp;

        /** sleep_period (Enhanced Power Save) */
    SleepPeriod sleep_period;

#define	MAX_REGION_CHANNEL_NUM	2
        /** Region Channel data */
    REGION_CHANNEL region_channel[MAX_REGION_CHANNEL_NUM];

    REGION_CHANNEL universal_channel[MAX_REGION_CHANNEL_NUM];

        /** 11D and Domain Regulatory Data */
    wlan_802_11d_domain_reg_t DomainReg;
    parsed_region_chan_11d_t parsed_region_chan;

        /** FSM variable for 11d support */
    wlan_802_11d_state_t State11D;
    int reassocAttempt;
    WLAN_802_11_MAC_ADDRESS reassocCurrentAp;
    u8 beaconBuffer[MAX_SCAN_BEACON_BUFFER];
    u8 *pBeaconBufEnd;

        /**	MISCELLANEOUS */
    /* Card Information Structure */
    u8 CisInfoBuf[512];
    u16 CisInfoLen;
    u8 *pRdeeprom;
    wlan_offset_value OffsetValue;

    wait_queue_head_t cmd_get_log;

    HostCmd_DS_802_11_GET_LOG LogMsg;
    u16 ScanProbes;

    u32 PktTxCtrl;

    u8 *helper;
    u32 helper_len;
    u8 *fmimage;
    u32 fmimage_len;
    FW_STATE fwstate;
    u16 TxRate;

    WLAN_802_11_SSID wildcardssid;
    u8 wildcardssidlen;         //1-32 -- the max length of wildcardssid.
    // > 32 -- wildcard char

    wlan_dbg dbg;
    wlan_subscribe_event subevent;
    u8 sdiomode;
    u32 num_cmd_timeout;
};

#endif /* _WLAN_DEV_H_ */
