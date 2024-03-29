/** @file wlan_wext.h
 * @brief This file contains definition for IOCTL call.
 *  
 * (c) Copyright � 2003-2007, Marvell International Ltd.  
 *
 * This software file (the "File") is distributed by Marvell International 
 * Ltd. under the terms of the GNU Lesser General Public License version 2.1 
 * (the "License") as published by the Free Software Foundation.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 * this warranty disclaimer.
 * 
 * A copy of the License is provided along with the File in the lgpl.txt file 
 * or may be requested by writing to the Free Software Foundation, Inc., 51 
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA or on the worldwide 
 * web at http://www.gnu.org/licenses/lgpl.txt
 *
 * For more information about the File or the License as it applies to the File, 
 * please contact Marvell International Ltd. via its affiliate, 
 * Marvell Semiconductor, Inc., 5488 Marvell Lane, Santa Clara, CA 95054.
 *
 *****************************************************************************/
/********************************************************
Change log:
	10/11/05: Add Doxygen format comments
	12/19/05: Correct a typo in structure _wlan_ioctl_wmm_tspec
	01/11/06: Conditionalize new scan/join ioctls
	04/10/06: Add hostcmd generic API
	04/18/06: Remove old Subscrive Event and add new Subscribe Event
	          implementation through generic hostcmd API
	06/08/06: Add definitions of custom events
********************************************************/

#ifndef	_WLAN_WEXT_H_
#define	_WLAN_WEXT_H_
#include "wlan_types.h"

#define SUBCMD_OFFSET			4
/** PRIVATE CMD ID */
#define	WLANIOCTL			0x8BE0

#define WLANSETWPAIE			(WLANIOCTL + 0)
#define WLANCISDUMP 			(WLANIOCTL + 1)
#ifdef MFG_CMD_SUPPORT
#define	WLANMANFCMD			(WLANIOCTL + 2)
#endif
#define	WLANREGRDWR			(WLANIOCTL + 3)
#define MAX_EEPROM_DATA     			256
#define	WLANHOSTCMD			(WLANIOCTL + 4)

#define WLANHOSTSLEEPCFG		(WLANIOCTL + 5)
#define WLANARPFILTER			(WLANIOCTL + 6)

#define WLAN_SETINT_GETINT		(WLANIOCTL + 7)
#define WLANNF					1
#define WLANRSSI				2
#define WLANBGSCAN				4
#define WLANENABLE11D				5
#define WLANADHOCGRATE				6
#define WLANSDIOCLOCK				7
#define WLANWMM_ENABLE				8
#define WLANNULLGEN				10
#define WLAN_SUBCMD_SET_PRESCAN			11
#define WLANADHOCCSET				12
#ifdef WPRM_DRV
#define WLANMOTOTM				20
#endif /* WPRM_DRV */

#define WLAN_SETNONE_GETNONE	        (WLANIOCTL + 8)
#define WLANDEAUTH                  		1
#define WLANRADIOON                 		2
#define WLANRADIOOFF                		3
#define WLANREMOVEADHOCAES          		4
#define WLANADHOCSTOP               		5
#ifdef REASSOCIATION
#define WLANREASSOCIATIONAUTO			8
#define WLANREASSOCIATIONUSER			9
#endif /* REASSOCIATION */
#define WLANWLANIDLEON				10
#define WLANWLANIDLEOFF				11

#define WLANGETLOG                  	(WLANIOCTL + 9)
#define WLAN_SETCONF_GETCONF		(WLANIOCTL + 10)

#define BG_SCAN_CONFIG				1
#define WMM_ACK_POLICY              2
#define WMM_PARA_IE                 3
#define WMM_ACK_POLICY_PRIO         4
#define CAL_DATA_EXT_CONFIG         5

#define WLANSCAN_TYPE			(WLANIOCTL + 11)

#define WLAN_SET_GET_2K         (WLANIOCTL + 13)
#define WLAN_SET_USER_SCAN              1
#define WLAN_GET_SCAN_TABLE             2
#define WLAN_SET_MRVL_TLV               3
#define WLAN_GET_ASSOC_RSP              4
#define WLAN_ADDTS_REQ                  5
#define WLAN_DELTS_REQ                  6
#define WLAN_QUEUE_CONFIG               7
#define WLAN_QUEUE_STATS                8
#define WLAN_GET_CFP_TABLE              9

#define WLAN_SETNONE_GETONEINT		(WLANIOCTL + 15)
#define WLANGETREGION				1
#define WLAN_GET_LISTEN_INTERVAL		2
#define WLAN_GET_MULTIPLE_DTIM			3
#define WLAN_GET_TX_RATE			4
#define	WLANGETBCNAVG				5
#define WLANGETDATAAVG				6

#define WLAN_SETNONE_GETTWELVE_CHAR (WLANIOCTL + 19)
#define WLAN_SUBCMD_GETRXANTENNA    1
#define WLAN_SUBCMD_GETTXANTENNA    2
#define WLAN_GET_TSF                3

#define WLAN_SETWORDCHAR_GETNONE	(WLANIOCTL + 20)
#define WLANSETADHOCAES				1

#define WLAN_SETNONE_GETWORDCHAR	(WLANIOCTL + 21)
#define WLANGETADHOCAES				1
#define WLANVERSION				2

#define WLAN_SETONEINT_GETONEINT	(WLANIOCTL + 23)
#define WLAN_WMM_QOSINFO			2
#define	WLAN_LISTENINTRVL			3
#define WLAN_FW_WAKEUP_METHOD			4
#define WAKEUP_FW_UNCHANGED			0
#define WAKEUP_FW_THRU_INTERFACE		1
#define WAKEUP_FW_THRU_GPIO			2

#define WLAN_TXCONTROL				5
#define WLAN_NULLPKTINTERVAL			6
#define WLAN_BCN_MISS_TIMEOUT			7
#define WLAN_ADHOC_AWAKE_PERIOD			8
#define WLAN_LDO				9
#define	WLAN_SDIO_MODE				10

#define WLAN_SETONEINT_GETNONE		(WLANIOCTL + 24)
#define WLAN_SUBCMD_SETRXANTENNA		1
#define WLAN_SUBCMD_SETTXANTENNA		2
#define WLANSETAUTHALG				5
#define WLANSETENCRYPTIONMODE			6
#define WLANSETREGION				7
#define WLAN_SET_LISTEN_INTERVAL		8

#define WLAN_SET_MULTIPLE_DTIM			9

#define WLANSETBCNAVG				10
#define WLANSETDATAAVG				11

#define WLAN_SET64CHAR_GET64CHAR	(WLANIOCTL + 25)
#define WLANSLEEPPARAMS 			2
#define	WLAN_BCA_TIMESHARE			3
#define WLANSCAN_MODE				6

#define WLAN_GET_ADHOC_STATUS			9

#define WLAN_SET_GEN_IE                 	10
#define WLAN_GET_GEN_IE                 	11
#define WLAN_REASSOCIATE                	12
#define WLAN_WMM_QUEUE_STATUS               	14
#define SET_WILDCARD_SSID			15
#define GET_WILDCARD_SSID			16

#define WLANEXTSCAN			(WLANIOCTL + 26)
#define WLANDEEPSLEEP			(WLANIOCTL + 27)
#define DEEP_SLEEP_ENABLE			1
#define DEEP_SLEEP_DISABLE  			0

#define WLAN_SET_GET_SIXTEEN_INT       (WLANIOCTL + 29)
#define WLAN_TPCCFG                             1
#define WLAN_AUTO_FREQ_SET			3
#define WLAN_AUTO_FREQ_GET			4
#define WLAN_LED_GPIO_CTRL			5
#define WLAN_SCANPROBES 			6
#define WLAN_SLEEP_PERIOD			7
#define	WLAN_ADAPT_RATESET			8
#define	WLAN_INACTIVITY_TIMEOUT			9
#define WLANSNR					10
#define WLAN_GET_RATE				11
#define	WLAN_GET_RXINFO				12
#define	WLAN_SET_ATIM_WINDOW			13
#define WLAN_BEACON_INTERVAL			14
#define WLAN_SCAN_TIME				16
#define WLAN_DATA_SUBSCRIBE_EVENT		18
#define WLANHSCFG				20
#define	WLAN_INACTIVITY_TIMEOUT_EXT	 	22
#ifdef DEBUG_LEVEL1
#define WLAN_DRV_DBG				24
#endif
#ifdef WPRM_DRV
#define WLAN_SESSIONTYPE                        25
#endif

#define WLANCMD52RDWR			(WLANIOCTL + 30)
#define WLANCMD53RDWR			(WLANIOCTL + 31)
#define CMD53BUFLEN				32

#define	REG_MAC					0x19
#define	REG_BBP					0x1a
#define	REG_RF					0x1b
#define	REG_EEPROM				0x59

#define	CMD_DISABLED				0
#define	CMD_ENABLED				1
#define	CMD_GET					2
#define SKIP_CMDNUM				4
#define SKIP_TYPE				1
#define SKIP_SIZE				2
#define SKIP_ACTION				2
#define SKIP_TYPE_SIZE			(SKIP_TYPE + SKIP_SIZE)
#define SKIP_TYPE_ACTION		(SKIP_TYPE + SKIP_ACTION)

/* define custom events */
#define CUS_EVT_HWM_CFG_DONE		"HWM_CFG_DONE.indication "
#define CUS_EVT_BEACON_RSSI_LOW		"EVENT=BEACON_RSSI_LOW"
#define CUS_EVT_BEACON_SNR_LOW		"EVENT=BEACON_SNR_LOW"
#define CUS_EVT_BEACON_RSSI_HIGH	"EVENT=BEACON_RSSI_HIGH"
#define CUS_EVT_BEACON_SNR_HIGH		"EVENT=BEACON_SNR_HIGH"
#define CUS_EVT_MAX_FAIL		"EVENT=MAX_FAIL"
#define CUS_EVT_MLME_MIC_ERR_UNI	"MLME-MICHAELMICFAILURE.indication unicast "
#define CUS_EVT_MLME_MIC_ERR_MUL	"MLME-MICHAELMICFAILURE.indication multicast "
#define CUS_EVT_DATA_RSSI_LOW		"EVENT=DATA_RSSI_LOW"
#define CUS_EVT_DATA_SNR_LOW		"EVENT=DATA_SNR_LOW"
#define CUS_EVT_DATA_RSSI_HIGH		"EVENT=DATA_RSSI_HIGH"
#define CUS_EVT_DATA_SNR_HIGH		"EVENT=DATA_SNR_HIGH"

#define CUS_EVT_DEEP_SLEEP_AWAKE	"EVENT=DS_AWAKE"

#define CUS_EVT_ADHOC_LINK_SENSED	"EVENT=ADHOC_LINK_SENSED"
#define CUS_EVT_ADHOC_BCN_LOST		"EVENT=ADHOC_BCN_LOST"

#define CUS_EVT_WATCH_DOG_TMOUT		"EVENT=WATCH_DOG_TMOUT"
/**
 *  @brief Maximum number of channels that can be sent in a setuserscan ioctl
 *
 *  @sa wlan_ioctl_user_scan_cfg
 */
#define WLAN_IOCTL_USER_SCAN_CHAN_MAX  50

/** wlan_ioctl */
typedef struct _wlan_ioctl
{
        /** Command ID */
    u16 command;
        /** data length */
    u16 len;
        /** data pointer */
    u8 *data;
} wlan_ioctl;

/** wlan_ioctl_rfantenna */
typedef struct _wlan_ioctl_rfantenna
{
    u16 Action;
    u16 AntennaMode;
} wlan_ioctl_rfantenna;

/** wlan_ioctl_regrdwr */
typedef struct _wlan_ioctl_regrdwr
{
        /** Which register to access */
    u16 WhichReg;
        /** Read or Write */
    u16 Action;
    u32 Offset;
    u16 NOB;
    u32 Value;
} wlan_ioctl_regrdwr;

/** wlan_ioctl_cfregrdwr */
typedef struct _wlan_ioctl_cfregrdwr
{
        /** Read or Write */
    u8 Action;
        /** register address */
    u16 Offset;
        /** register value */
    u16 Value;
} wlan_ioctl_cfregrdwr;

/** wlan_ioctl_rdeeprom */
typedef struct _wlan_ioctl_rdeeprom
{
    u16 WhichReg;
    u16 Action;
    u16 Offset;
    u16 NOB;
    u8 Value;
} wlan_ioctl_rdeeprom;

/** wlan_ioctl_adhoc_key_info */
typedef struct _wlan_ioctl_adhoc_key_info
{
    u16 action;
    u8 key[16];
    u8 tkiptxmickey[16];
    u8 tkiprxmickey[16];
} wlan_ioctl_adhoc_key_info;

/** sleep_params */
typedef struct _wlan_ioctl_sleep_params_config
{
    u16 Action;
    u16 Error;
    u16 Offset;
    u16 StableTime;
    u8 CalControl;
    u8 ExtSleepClk;
    u16 Reserved;
} __ATTRIB_PACK__ wlan_ioctl_sleep_params_config,
    *pwlan_ioctl_sleep_params_config;

/** BCA TIME SHARE */
typedef struct _wlan_ioctl_bca_timeshare_config
{
        /** ACT_GET/ACT_SET */
    u16 Action;
        /** Type: WLAN, BT */
    u16 TrafficType;
        /** Interval: 20msec - 60000msec */
    u32 TimeShareInterval;
        /** PTA arbiter time in msec */
    u32 BTTime;
} __ATTRIB_PACK__ wlan_ioctl_bca_timeshare_config,
    *pwlan_ioctl_bca_timeshare_config;

typedef struct _wlan_ioctl_reassociation_info
{
    u8 CurrentBSSID[6];
    u8 DesiredBSSID[6];
    char DesiredSSID[IW_ESSID_MAX_SIZE + 1];
} __ATTRIB_PACK__ wlan_ioctl_reassociation_info;

#define MAX_CFP_LIST_NUM	64

/** wlan_ioctl_cfp_table */
typedef struct _wlan_ioctl_cfp_table
{
    u32 region;
    u32 cfp_no;
    struct
    {
        u16 Channel;
        u32 Freq;
        u16 MaxTxPower;
        u8 Unsupported;
    } cfp[MAX_CFP_LIST_NUM];
} __ATTRIB_PACK__ wlan_ioctl_cfp_table, *pwlan_ioctl_cfp_table;

/**
 *  @brief IOCTL channel sub-structure sent in wlan_ioctl_user_scan_cfg
 *
 *  Multiple instances of this structure are included in the IOCTL command
 *   to configure a instance of a scan on the specific channel.
 */
typedef struct
{
    u8 chanNumber;              //!< Channel Number to scan
    u8 radioType;               //!< Radio type: 'B/G' Band = 0, 'A' Band = 1
    u8 scanType;                //!< Scan type: Active = 0, Passive = 1
    u8 reserved;
    u16 scanTime;               //!< Scan duration in milliseconds; if 0 default used
} __ATTRIB_PACK__ wlan_ioctl_user_scan_chan;

/**
 *  @brief IOCTL input structure to configure an immediate scan cmd to firmware
 *
 *  Used in the setuserscan (WLAN_SET_USER_SCAN) private ioctl.  Specifies
 *   a number of parameters to be used in general for the scan as well
 *   as a channel list (wlan_ioctl_user_scan_chan) for each scan period
 *   desired.
 *
 *  @sa wlan_set_user_scan_ioctl
 */
typedef struct
{

    /**
     *  @brief Flag set to keep the previous scan table intact
     *
     *  If set, the scan results will accumulate, replacing any previous
     *   matched entries for a BSS with the new scan data
     */
    u8 keepPreviousScan;        //!< Do not erase the existing scan results

    /**
     *  @brief BSS Type to be sent in the firmware command
     *
     *  Field can be used to restrict the types of networks returned in the
     *    scan.  Valid settings are:
     *
     *   - WLAN_SCAN_BSS_TYPE_BSS  (infrastructure)
     *   - WLAN_SCAN_BSS_TYPE_IBSS (adhoc)
     *   - WLAN_SCAN_BSS_TYPE_ANY  (unrestricted, adhoc and infrastructure)
     */
    u8 bssType;

    /**
     *  @brief Configure the number of probe requests for active chan scans
     */
    u8 numProbes;

    /**
     *  @brief BSSID filter sent in the firmware command to limit the results
     */
    u8 specificBSSID[MRVDRV_ETH_ADDR_LEN];

    /**
     *  @brief SSID filter sent in the firmware command to limit the results
     */
    char specificSSID[MRVDRV_MAX_SSID_LENGTH + 1];

    /**
     *  @brief Variable number (fixed maximum) of channels to scan up
     */
    wlan_ioctl_user_scan_chan chanList[WLAN_IOCTL_USER_SCAN_CHAN_MAX];
     /**
     *  @brief the max length of wildcard SSID
     */
    u8 wildcardssidlen;
} __ATTRIB_PACK__ wlan_ioctl_user_scan_cfg;

#endif /* _WLAN_WEXT_H_ */
