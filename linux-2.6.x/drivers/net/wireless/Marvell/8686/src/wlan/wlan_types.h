/** @file wlan_types.h
 *  @brief This header file contains definition for global types
 *
 *  (c) Copyright � 2003-2007, Marvell International Ltd.  
 *
 *  This software file (the "File") is distributed by Marvell International 
 *  Ltd. under the terms of the GNU Lesser General Public License version 2.1 
 *  (the "License") as published by the Free Software Foundation.
 *
 *  THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 *  this warranty disclaimer.
 * 
 *  A copy of the License is provided along with the File in the lgpl.txt file 
 *  or may be requested by writing to the Free Software Foundation, Inc., 51 
 *  Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA or on the worldwide 
 *  web at http://www.gnu.org/licenses/lgpl.txt
 *
 *  For more information about the File or the License as it applies to the File, 
 *  please contact Marvell International Ltd. via its affiliate, 
 *  Marvell Semiconductor, Inc., 5488 Marvell Lane, Santa Clara, CA 95054.
 *
 *****************************************************************************/
/*************************************************************
Change log:
	10/11/05: add Doxygen format comments 
	01/11/06: Add IEEE Association response type. Add TSF TLV information.
	01/31/06: Add support to selectively enabe the FW Scan channel filter
	04/10/06: Add power_adapt_cfg_ext command
	04/18/06: Remove old Subscrive Event and add new Subscribe Event
	          implementation through generic hostcmd API
	05/03/06: Add auto_tx hostcmd
************************************************************/

#ifndef _WLAN_TYPES_
#define _WLAN_TYPES_

#ifndef __KERNEL__
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed long s32;
typedef unsigned long u32;

typedef signed long long s64;
typedef unsigned long long u64;

typedef u32 dma_addr_t;
typedef u32 dma64_addr_t;

/* Dma addresses are 32-bits wide.  */
#ifndef __ATTRIB_ALIGN__
#define __ATTRIB_ALIGN__ __attribute__((aligned(4)))
#endif

#ifndef __ATTRIB_PACK__
#define __ATTRIB_PACK__ __attribute__ ((packed))
#endif
#endif

typedef long LONG;
typedef unsigned long long ULONGLONG;
typedef u32 WLAN_OID;

#define MRVDRV_MAX_MULTICAST_LIST_SIZE		32
#define MRVDRV_MAX_CHANNEL_SIZE			14
#define MRVDRV_ETH_ADDR_LEN                      6

#define MRVDRV_MAX_SSID_LENGTH			32
#define MRVDRV_MAX_BSS_DESCRIPTS		16

#define MRVL_KEY_BUFFER_SIZE_IN_BYTE  		16
#define MRVL_MAX_KEY_WPA_KEY_LENGTH     	32

#define HOSTCMD_SUPPORTED_RATES G_SUPPORTED_RATES

#define	BAND_B			(0x01)
#define	BAND_G			(0x02)
#define ALL_802_11_BANDS	(BAND_B | BAND_G)

#define B_SUPPORTED_RATES			8
#define G_SUPPORTED_RATES			14

#define	WLAN_SUPPORTED_RATES			14
#define WLAN_MAX_SSID_LENGTH			32

#define	MAX_POWER_ADAPT_GROUP		5

typedef u8 WLAN_802_11_RATES[WLAN_SUPPORTED_RATES];     // Set of 8 data rates
typedef u8 WLAN_802_11_MAC_ADDRESS[ETH_ALEN];

typedef enum _WLAN_802_11_NETWORK_TYPE
{
    Wlan802_11FH,
    Wlan802_11DS,
    Wlan802_11NetworkTypeMax    // not a real type, 
        //defined as an upper bound
} WLAN_802_11_NETWORK_TYPE, *PWLAN_802_11_NETWORK_TYPE;

typedef enum _WLAN_802_11_NETWORK_INFRASTRUCTURE
{
    Wlan802_11IBSS,
    Wlan802_11Infrastructure,
    Wlan802_11AutoUnknown,
    Wlan802_11InfrastructureMax // Not a real value, 
        // defined as upper bound
} WLAN_802_11_NETWORK_INFRASTRUCTURE, *PWLAN_802_11_NETWORK_INFRASTRUCTURE;

/** IEEE Type definitions  */
typedef enum _IEEEtypes_ElementId_e
{
    SSID = 0,
    SUPPORTED_RATES,
    FH_PARAM_SET,
    DS_PARAM_SET,
    CF_PARAM_SET,
    TIM,
    IBSS_PARAM_SET,
    COUNTRY_INFO = 7,

    CHALLENGE_TEXT = 16,

    EXTENDED_SUPPORTED_RATES = 50,

    VENDOR_SPECIFIC_221 = 221,
    WMM_IE = VENDOR_SPECIFIC_221,

    WPA_IE = VENDOR_SPECIFIC_221,
    WPA2_IE = 48,

    EXTRA_IE = 133,
} __ATTRIB_PACK__ IEEEtypes_ElementId_e;

#define WMM_OUI_TYPE  2

#define CAPINFO_MASK    (~( W_BIT_15 | W_BIT_14 |               \
                            W_BIT_12 | W_BIT_11 | W_BIT_9) )

typedef struct _IEEEtypes_CapInfo_t
{
    u8 Ess:1;
    u8 Ibss:1;
    u8 CfPollable:1;
    u8 CfPollRqst:1;
    u8 Privacy:1;
    u8 ShortPreamble:1;
    u8 Pbcc:1;
    u8 ChanAgility:1;
    u8 SpectrumMgmt:1;
    u8 Rsrvd3:1;
    u8 ShortSlotTime:1;
    u8 Apsd:1;
    u8 Rsvrd2:1;
    u8 DSSSOFDM:1;
    u8 Rsrvd1:2;
} __ATTRIB_PACK__ IEEEtypes_CapInfo_t;

/** IEEEtypes_CfParamSet_t */
typedef struct _IEEEtypes_CfParamSet_t
{
    u8 ElementId;
    u8 Len;
    u8 CfpCnt;
    u8 CfpPeriod;
    u16 CfpMaxDuration;
    u16 CfpDurationRemaining;
} __ATTRIB_PACK__ IEEEtypes_CfParamSet_t;

typedef struct IEEEtypes_IbssParamSet_t
{
    u8 ElementId;
    u8 Len;
    u16 AtimWindow;
} __ATTRIB_PACK__ IEEEtypes_IbssParamSet_t;

/** IEEEtypes_SsParamSet_t */
typedef union _IEEEtypes_SsParamSet_t
{
    IEEEtypes_CfParamSet_t CfParamSet;
    IEEEtypes_IbssParamSet_t IbssParamSet;
} __ATTRIB_PACK__ IEEEtypes_SsParamSet_t;

/** IEEEtypes_FhParamSet_t */
typedef struct _IEEEtypes_FhParamSet_t
{
    u8 ElementId;
    u8 Len;
    u16 DwellTime;
    u8 HopSet;
    u8 HopPattern;
    u8 HopIndex;
} __ATTRIB_PACK__ IEEEtypes_FhParamSet_t;

typedef struct _IEEEtypes_DsParamSet_t
{
    u8 ElementId;
    u8 Len;
    u8 CurrentChan;
} __ATTRIB_PACK__ IEEEtypes_DsParamSet_t;

/** IEEEtypes_DsParamSet_t */
typedef union IEEEtypes_PhyParamSet_t
{
    IEEEtypes_FhParamSet_t FhParamSet;
    IEEEtypes_DsParamSet_t DsParamSet;
} __ATTRIB_PACK__ IEEEtypes_PhyParamSet_t;

typedef u16 IEEEtypes_AId_t;
typedef u16 IEEEtypes_StatusCode_t;

typedef struct
{
    IEEEtypes_CapInfo_t Capability;
    IEEEtypes_StatusCode_t StatusCode;
    IEEEtypes_AId_t AId;
    u8 IEBuffer[1];
} __ATTRIB_PACK__ IEEEtypes_AssocRsp_t;

/** TLV  type ID definition */
#define PROPRIETARY_TLV_BASE_ID		0x0100

/* Terminating TLV Type */
#define MRVL_TERMINATE_TLV_ID		0xffff

#define TLV_TYPE_SSID				0x0000
#define TLV_TYPE_RATES				0x0001
#define TLV_TYPE_PHY_FH				0x0002
#define TLV_TYPE_PHY_DS				0x0003
#define TLV_TYPE_CF				    0x0004
#define TLV_TYPE_IBSS				0x0006

#define TLV_TYPE_DOMAIN				0x0007

#define TLV_TYPE_POWER_CAPABILITY	0x0021

#define TLV_TYPE_KEY_MATERIAL       (PROPRIETARY_TLV_BASE_ID + 0)
#define TLV_TYPE_CHANLIST           (PROPRIETARY_TLV_BASE_ID + 1)
#define TLV_TYPE_NUMPROBES          (PROPRIETARY_TLV_BASE_ID + 2)
#define TLV_TYPE_RSSI_LOW           (PROPRIETARY_TLV_BASE_ID + 4)
#define TLV_TYPE_SNR_LOW            (PROPRIETARY_TLV_BASE_ID + 5)
#define TLV_TYPE_FAILCOUNT          (PROPRIETARY_TLV_BASE_ID + 6)
#define TLV_TYPE_BCNMISS            (PROPRIETARY_TLV_BASE_ID + 7)
#define TLV_TYPE_LED_GPIO           (PROPRIETARY_TLV_BASE_ID + 8)
#define TLV_TYPE_LEDBEHAVIOR        (PROPRIETARY_TLV_BASE_ID + 9)
#define TLV_TYPE_PASSTHROUGH        (PROPRIETARY_TLV_BASE_ID + 10)
#define TLV_TYPE_REASSOCAP          (PROPRIETARY_TLV_BASE_ID + 11)
#define TLV_TYPE_POWER_TBL_2_4GHZ   (PROPRIETARY_TLV_BASE_ID + 12)
#define TLV_TYPE_POWER_TBL_5GHZ     (PROPRIETARY_TLV_BASE_ID + 13)
#define TLV_TYPE_BCASTPROBE	    (PROPRIETARY_TLV_BASE_ID + 14)
#define TLV_TYPE_NUMSSID_PROBE	    (PROPRIETARY_TLV_BASE_ID + 15)
#define TLV_TYPE_WMMQSTATUS   	    (PROPRIETARY_TLV_BASE_ID + 16)
#define TLV_TYPE_WILDCARDSSID	    (PROPRIETARY_TLV_BASE_ID + 18)
#define TLV_TYPE_TSFTIMESTAMP	    (PROPRIETARY_TLV_BASE_ID + 19)
#define TLV_TYPE_POWERADAPTCFGEXT   (PROPRIETARY_TLV_BASE_ID + 20)
#define TLV_TYPE_RSSI_HIGH          (PROPRIETARY_TLV_BASE_ID + 22)
#define TLV_TYPE_SNR_HIGH           (PROPRIETARY_TLV_BASE_ID + 23)
#define TLV_TYPE_AUTO_TX	    (PROPRIETARY_TLV_BASE_ID + 24)

#define TLV_TYPE_STARTBGSCANLATER   (PROPRIETARY_TLV_BASE_ID + 30)

/** TLV related data structures*/
/** MrvlIEtypesHeader_t */
typedef struct _MrvlIEtypesHeader
{
    u16 Type;
    u16 Len;
} __ATTRIB_PACK__ MrvlIEtypesHeader_t;

/** MrvlIEtypes_Data_t */
typedef struct _MrvlIEtypes_Data_t
{
    MrvlIEtypesHeader_t Header;
    u8 Data[1];
} __ATTRIB_PACK__ MrvlIEtypes_Data_t;

/** MrvlIEtypes_RatesParamSet_t */
typedef struct _MrvlIEtypes_RatesParamSet_t
{
    MrvlIEtypesHeader_t Header;
    u8 Rates[1];
} __ATTRIB_PACK__ MrvlIEtypes_RatesParamSet_t;

/** MrvlIEtypes_SsIdParamSet_t */
typedef struct _MrvlIEtypes_SsIdParamSet_t
{
    MrvlIEtypesHeader_t Header;
    u8 SsId[1];
} __ATTRIB_PACK__ MrvlIEtypes_SsIdParamSet_t;

/** MrvlIEtypes_WildCardSsIdParamSet_t */
typedef struct _MrvlIEtypes_WildCardSsIdParamSet_t
{
    MrvlIEtypesHeader_t Header;
    u8 MaxSsidLength;
    u8 SsId[1];
} __ATTRIB_PACK__ MrvlIEtypes_WildCardSsIdParamSet_t;

/** ChanScanMode_t */
typedef struct
{
    u8 PassiveScan:1;
    u8 DisableChanFilt:1;
    u8 Reserved_2_7:6;
} __ATTRIB_PACK__ ChanScanMode_t;

/** ChanScanParamSet_t */
typedef struct _ChanScanParamSet_t
{
    u8 RadioType;
    u8 ChanNumber;
    ChanScanMode_t ChanScanMode;
    u16 MinScanTime;
    u16 MaxScanTime;
} __ATTRIB_PACK__ ChanScanParamSet_t;

/** MrvlIEtypes_ChanListParamSet_t */
typedef struct _MrvlIEtypes_ChanListParamSet_t
{
    MrvlIEtypesHeader_t Header;
    ChanScanParamSet_t ChanScanParam[1];
} __ATTRIB_PACK__ MrvlIEtypes_ChanListParamSet_t;

/** CfParamSet_t */
typedef struct _CfParamSet_t
{
    u8 CfpCnt;
    u8 CfpPeriod;
    u16 CfpMaxDuration;
    u16 CfpDurationRemaining;
} __ATTRIB_PACK__ CfParamSet_t;

/** IbssParamSet_t */
typedef struct _IbssParamSet_t
{
    u16 AtimWindow;
} __ATTRIB_PACK__ IbssParamSet_t;

/** MrvlIEtypes_SsParamSet_t */
typedef struct _MrvlIEtypes_SsParamSet_t
{
    MrvlIEtypesHeader_t Header;
    union
    {
        CfParamSet_t CfParamSet[1];
        IbssParamSet_t IbssParamSet[1];
    } cf_ibss;
} __ATTRIB_PACK__ MrvlIEtypes_SsParamSet_t;

/** FhParamSet_t */
typedef struct _FhParamSet_t
{
    u16 DwellTime;
    u8 HopSet;
    u8 HopPattern;
    u8 HopIndex;
} __ATTRIB_PACK__ FhParamSet_t;

/** DsParamSet_t */
typedef struct _DsParamSet_t
{
    u8 CurrentChan;
} __ATTRIB_PACK__ DsParamSet_t;

/** MrvlIEtypes_PhyParamSet_t */
typedef struct _MrvlIEtypes_PhyParamSet_t
{
    MrvlIEtypesHeader_t Header;
    union
    {
        FhParamSet_t FhParamSet[1];
        DsParamSet_t DsParamSet[1];
    } fh_ds;
} __ATTRIB_PACK__ MrvlIEtypes_PhyParamSet_t;

/** MrvlIEtypes_ReassocAp_t */
typedef struct _MrvlIEtypes_ReassocAp_t
{
    MrvlIEtypesHeader_t Header;
    WLAN_802_11_MAC_ADDRESS currentAp;

} __ATTRIB_PACK__ MrvlIEtypes_ReassocAp_t;

/** MrvlIEtypes_RsnParamSet_t */
typedef struct _MrvlIEtypes_RsnParamSet_t
{
    MrvlIEtypesHeader_t Header;
    u8 RsnIE[1];
} __ATTRIB_PACK__ MrvlIEtypes_RsnParamSet_t;

/** MrvlIEtypes_WmmParamSet_t */
typedef struct _MrvlIEtypes_WmmParamSet_t
{
    MrvlIEtypesHeader_t Header;
    u8 WmmIE[1];
} __ATTRIB_PACK__ MrvlIEtypes_WmmParamSet_t;

typedef struct
{
    MrvlIEtypesHeader_t Header;
    u8 QueueIndex;
    u8 Disabled;
    u8 TriggeredPS;
    u8 FlowDirection;
    u8 FlowRequired;
    u8 FlowCreated;
    u32 MediumTime;
} __ATTRIB_PACK__ MrvlIEtypes_WmmQueueStatus_t;

typedef struct
{
    MrvlIEtypesHeader_t Header;
    u64 tsfTable[1];
} __ATTRIB_PACK__ MrvlIEtypes_TsfTimestamp_t;

/**  Local Power Capability */
typedef struct _MrvlIEtypes_PowerCapability_t
{
    MrvlIEtypesHeader_t Header;
    s8 MinPower;
    s8 MaxPower;
} __ATTRIB_PACK__ MrvlIEtypes_PowerCapability_t;

/** MrvlIEtypes_RssiParamSet_t */
typedef struct _MrvlIEtypes_RssiThreshold_t
{
    MrvlIEtypesHeader_t Header;
    u8 RSSIValue;
    u8 RSSIFreq;
} __ATTRIB_PACK__ MrvlIEtypes_RssiParamSet_t;

/** MrvlIEtypes_SnrThreshold_t */
typedef struct _MrvlIEtypes_SnrThreshold_t
{
    MrvlIEtypesHeader_t Header;
    u8 SNRValue;
    u8 SNRFreq;
} __ATTRIB_PACK__ MrvlIEtypes_SnrThreshold_t;

/** MrvlIEtypes_FailureCount_t */
typedef struct _MrvlIEtypes_FailureCount_t
{
    MrvlIEtypesHeader_t Header;
    u8 FailValue;
    u8 FailFreq;
} __ATTRIB_PACK__ MrvlIEtypes_FailureCount_t;

/** MrvlIEtypes_BeaconsMissed_t */
typedef struct _MrvlIEtypes_BeaconsMissed_t
{
    MrvlIEtypesHeader_t Header;
    u8 BeaconMissed;
    u8 Reserved;
} __ATTRIB_PACK__ MrvlIEtypes_BeaconsMissed_t;

/** MrvlIEtypes_NumProbes_t */
typedef struct _MrvlIEtypes_NumProbes_t
{
    MrvlIEtypesHeader_t Header;
    u16 NumProbes;
} __ATTRIB_PACK__ MrvlIEtypes_NumProbes_t;

/** MrvlIEtypes_BcastProbe_t */
typedef struct _MrvlIEtypes_BcastProbe_t
{
    MrvlIEtypesHeader_t Header;
    u16 BcastProbe;
} __ATTRIB_PACK__ MrvlIEtypes_BcastProbe_t;

/** MrvlIEtypes_NumSSIDProbe_t */
typedef struct _MrvlIEtypes_NumSSIDProbe_t
{
    MrvlIEtypesHeader_t Header;
    u16 NumSSIDProbe;
} __ATTRIB_PACK__ MrvlIEtypes_NumSSIDProbe_t;

/** MrvlIEtypes_StartBGScanLater_t */
typedef struct _MrvlIEtypes_StartBGScanLater_t
{
    MrvlIEtypesHeader_t Header;
    u16 StartLater;
} __ATTRIB_PACK__ MrvlIEtypes_StartBGScanLater_t;

typedef struct
{
    u8 Led;
    u8 Pin;
} __ATTRIB_PACK__ Led_Pin;

/** MrvlIEtypes_LedGpio_t */
typedef struct _MrvlIEtypes_LedGpio_t
{
    MrvlIEtypesHeader_t Header;
    Led_Pin LedPin[1];
} __ATTRIB_PACK__ MrvlIEtypes_LedGpio_t;

typedef struct _PA_Group_t
{
    u16 PowerAdaptLevel;
    u16 RateBitmap;
    u32 Reserved;
} __ATTRIB_PACK__ PA_Group_t;

/** MrvlIEtypes_PA_Group_t */
typedef struct _MrvlIEtypes_PowerAdapt_Group_t
{
    MrvlIEtypesHeader_t Header;
    PA_Group_t PA_Group[MAX_POWER_ADAPT_GROUP];
} __ATTRIB_PACK__ MrvlIEtypes_PowerAdapt_Group_t;

typedef struct _AutoTx_MacFrame_t
{
    u16 Interval;               /* in seconds */
    u8 Priority;                /* User Priority: 0~7, ignored if non-WMM */
    u8 Reserved;                /* set to 0 */
    u16 FrameLen;               /* Length of MAC frame payload */
    u8 DestMacAddr[ETH_ALEN];
    u8 SrcMacAddr[ETH_ALEN];
    u8 Payload[];
} __ATTRIB_PACK__ AutoTx_MacFrame_t;

/** MrvlIEtypes_PA_Group_t */
typedef struct _MrvlIEtypes_AutoTx_t
{
    MrvlIEtypesHeader_t Header;
    AutoTx_MacFrame_t AutoTx_MacFrame;
} __ATTRIB_PACK__ MrvlIEtypes_AutoTx_t;

#define MRVDRV_MAX_SUBBAND_802_11D		83
#define COUNTRY_CODE_LEN			3
/** Data structure for Country IE*/
typedef struct _IEEEtypes_SubbandSet
{
    u8 FirstChan;
    u8 NoOfChan;
    u8 MaxTxPwr;
} __ATTRIB_PACK__ IEEEtypes_SubbandSet_t;

typedef struct _IEEEtypes_CountryInfoSet
{
    u8 ElementId;
    u8 Len;
    u8 CountryCode[COUNTRY_CODE_LEN];
    IEEEtypes_SubbandSet_t Subband[1];
} __ATTRIB_PACK__ IEEEtypes_CountryInfoSet_t;

typedef struct _IEEEtypes_CountryInfoFullSet
{
    u8 ElementId;
    u8 Len;
    u8 CountryCode[COUNTRY_CODE_LEN];
    IEEEtypes_SubbandSet_t Subband[MRVDRV_MAX_SUBBAND_802_11D];
} __ATTRIB_PACK__ IEEEtypes_CountryInfoFullSet_t;

typedef struct _MrvlIEtypes_DomainParamSet
{
    MrvlIEtypesHeader_t Header;
    u8 CountryCode[COUNTRY_CODE_LEN];
    IEEEtypes_SubbandSet_t Subband[1];
} __ATTRIB_PACK__ MrvlIEtypes_DomainParamSet_t;

typedef struct
{
    u8 value;
    u8 Freq;
} Threshold;

typedef struct
{
    u16 EventsBitmap;           //bit 0: RSSI low, bit 1:SNR low, bit2:RSSI high, bit 3 SNR high 
    Threshold Rssi_low;
    Threshold Snr_low;
    Threshold Rssi_high;
    Threshold Snr_high;
} wlan_subscribe_event;

/** enum of WMM AC_QUEUES */
#define  MAX_AC_QUEUES 4
typedef enum
{
    AC_PRIO_BK,
    AC_PRIO_BE,
    AC_PRIO_VI,
    AC_PRIO_VO
} __ATTRIB_PACK__ wlan_wmm_ac_e;

/** Size of a TSPEC.  Used to allocate necessary buffer space in commands */
#define WMM_TSPEC_SIZE              63

/** Extra IE bytes allocated in messages for appended IEs after a TSPEC */
#define WMM_ADDTS_EXTRA_IE_BYTES    256

/** Extra TLV bytes allocated in messages for configuring WMM Queues */
#define WMM_QUEUE_CONFIG_EXTRA_TLV_BYTES 64

/** wlan_ioctl_wmm_para_ie */
typedef struct
{

    /** type */
    u8 Type;

    /** action */
    u16 Action;

    /** WMM Parameter IE */
    u8 Para_IE[26];

} __ATTRIB_PACK__ wlan_ioctl_wmm_para_ie;

/** wlan_ioctl_wmm_ack_policy */
typedef struct
{
    u8 Type;

    /** 0-ACT_GET, 1-ACT_SET */
    u16 Action;

    /** 0-AC_BE, 1-AC_BK, 2-AC_VI, 3-AC_VO */
    u8 AC;

    /** 0-WMM_ACK_POLICY_IMM_ACK, 1-WMM_ACK_POLICY_NO_ACK */
    u8 AckPolicy;

} __ATTRIB_PACK__ wlan_ioctl_wmm_ack_policy;

/** data structure of WMM QoS information */
typedef struct
{
    u8 ParaSetCount:4;
    u8 Reserved:3;
    u8 QosUAPSD:1;
} __ATTRIB_PACK__ WMM_QoS_INFO;

typedef struct
{
    u8 AIFSN:4;
    u8 ACM:1;
    u8 ACI:2;
    u8 Reserved:1;
} __ATTRIB_PACK__ WMM_ACI_AIFSN;

/**  data structure of WMM ECW */
typedef struct
{
    u8 ECW_Min:4;
    u8 ECW_Max:4;
} __ATTRIB_PACK__ WMM_ECW;

/** data structure of WMM AC parameters  */
typedef struct
{
    WMM_ACI_AIFSN ACI_AIFSN;
    WMM_ECW ECW;
    u16 Txop_Limit;
} __ATTRIB_PACK__ WMM_AC_PARAS;

/** data structure of WMM Info IE  */
typedef struct
{
    /** 221 */
    u8 ElementId;
    /** 7 */
    u8 Length;
    /** 00:50:f2 (hex) */
    u8 Oui[3];
    /** 2 */
    u8 OuiType;
    /** 0 */
    u8 OuiSubtype;
    /** 1 */
    u8 Version;

    WMM_QoS_INFO QoSInfo;

} __ATTRIB_PACK__ WMM_INFO_IE;

/** data structure of WMM parameter IE  */
typedef struct
{
    /** 221 */
    u8 ElementId;
    /** 24 */
    u8 Length;
    /** 00:50:f2 (hex) */
    u8 Oui[3];
    /** 2 */
    u8 OuiType;
    /** 1 */
    u8 OuiSubtype;
    /** 1 */
    u8 Version;

    WMM_QoS_INFO QoSInfo;
    u8 Reserved;

    /** AC Parameters Record AC_BE */
    WMM_AC_PARAS AC_Paras_BE;
    /** AC Parameters Record AC_BK */
    WMM_AC_PARAS AC_Paras_BK;
    /** AC Parameters Record AC_VI */
    WMM_AC_PARAS AC_Paras_VI;
    /** AC Parameters Record AC_VO */
    WMM_AC_PARAS AC_Paras_VO;
} __ATTRIB_PACK__ WMM_PARAMETER_IE;

/** struct of command of WMM ack policy*/
typedef struct
{
    /** 0 - ACT_GET,
        1 - ACT_SET */
    u16 Action;

    /** 0 - AC_BE
        1 - AC_BK
        2 - AC_VI
        3 - AC_VO */
    u8 AC;

    /** 0 - WMM_ACK_POLICY_IMM_ACK
        1 - WMM_ACK_POLICY_NO_ACK */
    u8 AckPolicy;
} __ATTRIB_PACK__ HostCmd_DS_WMM_ACK_POLICY;

/**
 *  @brief Firmware command structure to retrieve the firmware WMM status.
 *
 *  Used to retrieve the status of each WMM AC Queue in TLV 
 *    format (MrvlIEtypes_WmmQueueStatus_t) as well as the current WMM
 *    parameter IE advertised by the AP.  
 *  
 *  Used in response to a MACREG_INT_CODE_WMM_STATUS_CHANGE event signalling
 *    a QOS change on one of the ACs or a change in the WMM Parameter in
 *    the Beacon.
 *
 *  TLV based command, byte arrays used for max sizing purpose. There are no 
 *    arguments sent in the command, the TLVs are returned by the firmware.
 */
typedef struct
{
    u8 queueStatusTlv[sizeof(MrvlIEtypes_WmmQueueStatus_t) * MAX_AC_QUEUES];
    u8 wmmParamTlv[sizeof(WMM_PARAMETER_IE) + 2];

}
__ATTRIB_PACK__ HostCmd_DS_WMM_GET_STATUS;

typedef struct
{
    u16 PacketAC;
} __ATTRIB_PACK__ HostCmd_DS_WMM_PRIO_PKT_AVAIL;

/**
 *  @brief Enumeration for the command result from an ADDTS or DELTS command 
 */
typedef enum
{
    TSPEC_RESULT_SUCCESS = 0,
    TSPEC_RESULT_EXEC_FAILURE = 1,
    TSPEC_RESULT_TIMEOUT = 2,
    TSPEC_RESULT_DATA_INVALID = 3,

} __ATTRIB_PACK__ wlan_wmm_tspec_result_e;

/**
 *  @brief IOCTL structure to send an ADDTS request and retrieve the response.
 *
 *  IOCTL structure from the application layer relayed to firmware to 
 *    instigate an ADDTS management frame with an appropriate TSPEC IE as well
 *    as any additional IEs appended in the ADDTS Action frame.
 *
 *  @sa wlan_wmm_addts_req_ioctl
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;
    u32 timeout_ms;

    u8 ieeeStatusCode;

    u8 tspecData[WMM_TSPEC_SIZE];

    u8 addtsExtraIEBuf[WMM_ADDTS_EXTRA_IE_BYTES];

} __ATTRIB_PACK__ wlan_ioctl_wmm_addts_req_t;

/**
 *  @brief IOCTL structure to send a DELTS request.
 *
 *  IOCTL structure from the application layer relayed to firmware to 
 *    instigate an DELTS management frame with an appropriate TSPEC IE.
 *
 *  @sa wlan_wmm_delts_req_ioctl
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;      //!< Firmware execution result

    u8 ieeeReasonCode;          //!< IEEE reason code sent, unused for WMM 

    u8 tspecData[WMM_TSPEC_SIZE];       //!< TSPEC to send in the DELTS

} __ATTRIB_PACK__ wlan_ioctl_wmm_delts_req_t;

/**
 *  @brief Internal command structure used in executing an ADDTS command.
 *
 *  Relay information between the IOCTL layer and the firmware command and 
 *    command response procedures.
 *
 *  @sa wlan_wmm_addts_req_ioctl
 *  @sa wlan_cmd_wmm_addts_req
 *  @sa wlan_cmdresp_wmm_addts_req
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;
    u32 timeout_ms;

    u8 dialogToken;
    u8 ieeeStatusCode;

    int tspecDataLen;
    u8 tspecData[WMM_TSPEC_SIZE];
    u8 addtsExtraIEBuf[WMM_ADDTS_EXTRA_IE_BYTES];

} wlan_cmd_wmm_addts_req_t;

/**
 *  @brief Internal command structure used in executing an DELTS command.
 *
 *  Relay information between the IOCTL layer and the firmware command and 
 *    command response procedures.
 *
 *  @sa wlan_wmm_delts_req_ioctl
 *  @sa wlan_cmd_wmm_delts_req
 *  @sa wlan_cmdresp_wmm_delts_req
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;

    u8 dialogToken;

    u8 ieeeReasonCode;

    int tspecDataLen;
    u8 tspecData[WMM_TSPEC_SIZE];

} wlan_cmd_wmm_delts_req_t;

/**
 *  @brief Command structure for the HostCmd_CMD_WMM_ADDTS_REQ firmware command
 *
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;
    u32 timeout_ms;

    u8 dialogToken;
    u8 ieeeStatusCode;
    u8 tspecData[WMM_TSPEC_SIZE];
    u8 addtsExtraIEBuf[WMM_ADDTS_EXTRA_IE_BYTES];

} __ATTRIB_PACK__ HostCmd_DS_WMM_ADDTS_REQ;

/**
 *  @brief Command structure for the HostCmd_CMD_WMM_DELTS_REQ firmware command
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;
    u8 dialogToken;
    u8 ieeeReasonCode;
    u8 tspecData[WMM_TSPEC_SIZE];

} __ATTRIB_PACK__ HostCmd_DS_WMM_DELTS_REQ;

/**
 *  @brief Enumeration for the action field in the Queue configure command
 */
typedef enum
{
    WMM_QUEUE_CONFIG_ACTION_GET = 0,
    WMM_QUEUE_CONFIG_ACTION_SET = 1,
    WMM_QUEUE_CONFIG_ACTION_DEFAULT = 2,

    WMM_QUEUE_CONFIG_ACTION_MAX
} __ATTRIB_PACK__ wlan_wmm_queue_config_action_e;

/**
 *  @brief Command structure for the HostCmd_CMD_WMM_QUEUE_CONFIG firmware cmd
 *
 *  Set/Get/Default the Queue parameters for a specific AC in the firmware.
 *
 */
typedef struct
{
    wlan_wmm_queue_config_action_e action;      //!< Set, Get, or Default
    wlan_wmm_ac_e accessCategory;       //!< AC_BK(0) to AC_VO(3)

    /** @brief MSDU lifetime expiry per 802.11e
     *
     *   - Ignored if 0 on a set command 
     *   - Set to the 802.11e specified 500 TUs when defaulted
     */
    u16 msduLifetimeExpiry;

    u8 tlvBuffer[WMM_QUEUE_CONFIG_EXTRA_TLV_BYTES];     //!< Not supported yet

} __ATTRIB_PACK__ HostCmd_DS_WMM_QUEUE_CONFIG;

/**
 *  @brief Internal command structure used in executing a queue config command.
 *
 *  Relay information between the IOCTL layer and the firmware command and 
 *    command response procedures.
 *
 *  @sa wlan_wmm_queue_config_ioctl
 *  @sa wlan_cmd_wmm_queue_config
 *  @sa wlan_cmdresp_wmm_queue_config
 */
typedef struct
{
    wlan_wmm_queue_config_action_e action;      //!< Set, Get, or Default
    wlan_wmm_ac_e accessCategory;       //!< AC_BK(0) to AC_VO(3)
    u16 msduLifetimeExpiry;     //!< lifetime expiry in TUs

    int tlvBufLen;              //!< Not supported yet
    u8 tlvBuffer[WMM_QUEUE_CONFIG_EXTRA_TLV_BYTES];     //!< Not supported yet

} wlan_cmd_wmm_queue_config_t;

/**
 *  @brief IOCTL structure to configure a specific AC Queue's parameters
 *
 *  IOCTL structure from the application layer relayed to firmware to 
 *    get, set, or default the WMM AC queue parameters.
 *
 *  - msduLifetimeExpiry is ignored if set to 0 on a set command
 *
 *  @sa wlan_wmm_queue_config_ioctl
 */
typedef struct
{
    wlan_wmm_queue_config_action_e action;      //!< Set, Get, or Default
    wlan_wmm_ac_e accessCategory;       //!< AC_BK(0) to AC_VO(3)
    u16 msduLifetimeExpiry;     //!< lifetime expiry in TUs

    u8 supportedRates[10];      //!< Not supported yet

} __ATTRIB_PACK__ wlan_ioctl_wmm_queue_config_t;

/**
 *   @brief Enumeration for the action field in the queue stats command
 */
typedef enum
{
    WMM_STATS_ACTION_START = 0,
    WMM_STATS_ACTION_STOP = 1,
    WMM_STATS_ACTION_GET_CLR = 2,

    WMM_STATS_ACTION_MAX
} __ATTRIB_PACK__ wlan_wmm_stats_action_e;

/** Number of bins in the histogram for the HostCmd_DS_WMM_QUEUE_STATS */
#define WMM_STATS_PKTS_HIST_BINS  7

/**
 *  @brief Command structure for the HostCmd_CMD_WMM_QUEUE_STATS firmware cmd
 *
 *  Turn statistical collection on/off for a given AC or retrieve the 
 *    accumulated stats for an AC and clear them in the firmware.
 */
typedef struct
{
    wlan_wmm_stats_action_e action;     //!< Start, Stop, or Get 
    wlan_wmm_ac_e accessCategory;       //!< AC_BK(0) to AC_VO(3)

    u16 pktCount;               //!< Number of successful packets transmitted
    u16 pktLoss;                //!< Packets lost; not included in pktCount
    u32 avgQueueDelay;          //!< Average Queue delay in microseconds
    u32 avgTxDelay;             //!< Average Transmission delay in microseconds
    u32 usedTime;               //!< Calculated medium time 

    /** @brief Queue Delay Histogram; number of packets per queue delay range
     * 
     *  [0] -  0ms <= delay < 5ms
     *  [1] -  5ms <= delay < 10ms
     *  [2] - 10ms <= delay < 20ms
     *  [3] - 20ms <= delay < 30ms
     *  [4] - 30ms <= delay < 40ms
     *  [5] - 40ms <= delay < 50ms
     *  [6] - 50ms <= delay < msduLifetime (TUs)
     */
    u16 delayHistogram[WMM_STATS_PKTS_HIST_BINS];

} __ATTRIB_PACK__ HostCmd_DS_WMM_QUEUE_STATS;

/**
 *  @brief IOCTL structure to start, stop, and get statistics for a WMM AC
 *
 *  IOCTL structure from the application layer relayed to firmware to 
 *    start or stop statistical collection for a given AC.  Also used to 
 *    retrieve and clear the collected stats on a given AC.
 *
 *  @sa wlan_wmm_queue_stats_ioctl
 */
typedef struct
{
    wlan_wmm_stats_action_e action;     //!< Start, Stop, or Get 
    wlan_wmm_ac_e accessCategory;       //!< AC_BK(0) to AC_VO(3)
    u16 pktCount;               //!< Number of successful packets transmitted  
    u16 pktLoss;                //!< Packets lost; not included in pktCount    
    u32 avgQueueDelay;          //!< Average Queue delay in microseconds
    u32 avgTxDelay;             //!< Average Transmission delay in microseconds
    u32 usedTime;               //!< Calculated medium time 

    /** @brief Queue Delay Histogram; number of packets per queue delay range
     * 
     *  [0] -  0ms <= delay < 5ms
     *  [1] -  5ms <= delay < 10ms
     *  [2] - 10ms <= delay < 20ms
     *  [3] - 20ms <= delay < 30ms
     *  [4] - 30ms <= delay < 40ms
     *  [5] - 40ms <= delay < 50ms
     *  [6] - 50ms <= delay < msduLifetime (TUs)
     */
    u16 delayHistogram[WMM_STATS_PKTS_HIST_BINS];
} __ATTRIB_PACK__ wlan_ioctl_wmm_queue_stats_t;

/** 
 *  @brief IOCTL sub structure for a specific WMM AC Status
 */
typedef struct
{
    u8 wmmACM;
    u8 flowRequired;
    u8 flowCreated;
    u8 disabled;
} __ATTRIB_PACK__ wlan_ioctl_wmm_queue_status_ac_t;

/**
 *  @brief IOCTL structure to retrieve the WMM AC Queue status
 *
 *  IOCTL structure from the application layer to retrieve:
 *     - ACM bit setting for the AC
 *     - Firmware status (flow required, flow created, flow disabled)
 *
 *  @sa wlan_wmm_queue_status_ioctl
 */
typedef struct
{
    wlan_ioctl_wmm_queue_status_ac_t acStatus[MAX_AC_QUEUES];
} __ATTRIB_PACK__ wlan_ioctl_wmm_queue_status_t;

/** Firmware status for a specific AC */
typedef struct
{
    u8 Disabled;
    u8 FlowRequired;
    u8 FlowCreated;
} WMM_AC_STATUS;

#endif /* _WLAN_TYPES_ */
