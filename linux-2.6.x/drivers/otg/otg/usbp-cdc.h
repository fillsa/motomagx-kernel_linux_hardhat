/*
 * otg/otg/usbp-cdc.h
 * @(#) sl@whiskey.enposte.net|otg/otg/usbp-cdc.h|20051116230344|25113
 *
 *      Copyright (c) 2004-2005 Belcarra
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
/*!
 * @file otg/otg/usbp-cdc.h
 * @brief CDC class descriptor structure definitions.
 *
 * @ingroup USBP
 */

#ifdef PRAGMAPACK
#pragma pack(push,1)
#endif

/*!
 * @name Class-Specific Request Codes
 * C.f. CDC Table 46
 */
 /*! @{ */

#define CDC_CLASS_REQUEST_SEND_ENCAPSULATED     0x00
#define CDC_CLASS_REQUEST_GET_ENCAPSULATED      0x01

#define CDC_CLASS_REQUEST_SET_COMM_FEATURE      0x02
#define CDC_CLASS_REQUEST_GET_COMM_FEATURE      0x03
#define CDC_CLASS_REQUEST_CLEAR_COMM_FEATURE    0x04

#define CDC_CLASS_REQUEST_SET_LINE_CODING       0x20
#define CDC_CLASS_REQUEST_GET_LINE_CODING       0x21

#define CDC_CLASS_REQUEST_SET_CONTROL_LINE_STATE 0x22
#define CDC_CLASS_REQUEST_SEND_BREAK            0x23
/*! @} */

/*!
 * @name Notification codes
 * c.f. CDC Table 68
 */
 /*! @{ */

#define CDC_NOTIFICATION_NETWORK_CONNECTION         0x00
#define CDC_NOTIFICATION_RESPONSE_AVAILABLE         0x01
#define CDC_NOTIFICATION_AUX_JACK_HOOK_STATE        0x08
#define CDC_NOTIFICATION_RING_DETECT                0x09
#define CDC_NOTIFICATION_SERIAL_STATE               0x20
#define CDC_NOTIFICATION_CALL_STATE_CHANGE          0x28
#define CDC_NOTIFICATION_LINE_STATE_CHANGE          0x29
#define CDC_NOTIFICATION_CONNECTION_SPEED_CHANGE    0x2a
/*! @} */


/*!
 * @name ACM - Line Coding structure
 * C.f. CDC Table 50
 */
 /*! @{ */


PACKED0 struct PACKED1 cdc_acm_line_coding {
        u32 dwDTERate;
        u8 bCharFormat;
        u8 bParityType;
        u8 bDataBits;
};

 /*!@} */
 
/*!
 * @name ACM - Line State
 * C.f. CDC Table 51
 */
 /*! @{ */
typedef u16 cdc_acm_bmLineState;
#define CDC_LINESTATE_D1_RTS            (1 << 1)
#define CDC_LINESTATE_D0_DTR            (1 << 0)
/*! @} */

/*!
 * Serial State - C.f. 6.3.5
 */
PACKED0 struct PACKED1 acm_serial_state {
        u16     uart_state_bitmap;
};


/*! 
* @name ACM - UART State
 * C.f. Table 69 - UART State Bitmap Values
 */
/*! @{ */
typedef u16 cdc_acm_bmUARTState;
#define CDC_UARTSTATE_BOVERRUN        (1 << 6)
#define CDC_UARTSTATE_BPARITY         (1 << 5)
#define CDC_UARTSTATE_BFRAMING        (1 << 4)
#define CDC_UARTSTATE_BRINGSIGNAL     (1 << 3)
#define CDC_UARTSTATE_BBREAK          (1 << 2)
#define CDC_UARTSTATE_BTXCARRIER_DSR  (1 << 1)
#define CDC_UARTSTATE_BRXCARRIER_DCD  (1 << 0)
/*! @} */

/* !USB Notification
 *
 */

PACKED0 struct PACKED1 cdc_notification_descriptor {
        u8 bmRequestType;
        u8 bNotification;
        u16 wValue;
        u16 wIndex;
        u16 wLength;
        u8 data[2];
};




/*!
 * @name Communications Class Types
 *
 * c.f. CDC  USB Class Definitions for Communications Devices
 * c.f. WMCD USB CDC Subclass Specification for Wireless Mobile Communications Devices
 *
 */
/*! @{ */

#define CLASS_BCD_VERSION               0x0110
/*! @} */

/*! @name c.f. CDC 4.1 Table 14 */

/*! @{ */

#define AUDIO_CLASS                     0x01
#define COMMUNICATIONS_DEVICE_CLASS     0x02
/*! @} */

/*! @name c.f. CDC 4.2 Table 15
 */
 /*! @{ */
#define COMMUNICATIONS_INTERFACE_CLASS  0x02
/*! @} */

/*! @name c.f. CDC 4.3 Table 16
 */
 /*! @{ */
#define COMMUNICATIONS_NO_SUBCLASS      0x00
#define COMMUNICATIONS_DLCM_SUBCLASS    0x01
#define COMMUNICATIONS_ACM_SUBCLASS     0x02
#define COMMUNICATIONS_TCM_SUBCLASS     0x03
#define COMMUNICATIONS_MCCM_SUBCLASS    0x04
#define COMMUNICATIONS_CCM_SUBCLASS     0x05
#define COMMUNICATIONS_ENCM_SUBCLASS    0x06
#define COMMUNICATIONS_ANCM_SUBCLASS    0x07

#define AUDIO_CONTROL_SUBCLASS          0x01
#define AUDIO_STREAMING_SUBCLASS        0x02
/*! @} */

/*! @name c.f. WMCD 5.1
 */
 /*! @{ */
#define COMMUNICATIONS_WHCM_SUBCLASS    0x08
#define COMMUNICATIONS_DMM_SUBCLASS     0x09
#define COMMUNICATIONS_MDLM_SUBCLASS    0x0a
#define COMMUNICATIONS_OBEX_SUBCLASS    0x0b
#define COMMUNICATIONS_EEM_SUBCLASS     0x0c
/*! @} */

/*! @name c.f. CDC 4.6 Table 18
 */
 /*! @{ */
#define DATA_INTERFACE_CLASS            0x0a
/*! @} */

/*! @name c.f. CDC 4.7 Table 19
 */
 /*! @{ */
#define COMMUNICATIONS_NO_PROTOCOL      0x00
#define COMMUNICATIONS_EEM_PROTOCOL     0x07
/*! @} */

/*!
 * @name bDescriptorSubtypes
 *
 * c.f. CDC 5.2.3 Table 25
 * c.f. WMCD 5.3 Table 5.3
 */
 /*! @{ */

#define USB_ST_HEADER                   0x00
#define USB_ST_CMF                      0x01
#define USB_ST_ACMF                     0x02
#define USB_ST_DLMF                     0x03
#define USB_ST_TRF                      0x04
#define USB_ST_TCLF                     0x05
#define USB_ST_UF                       0x06
#define USB_ST_CSF                      0x07
#define USB_ST_TOMF                     0x08
#define USB_ST_USBTF                    0x09
#define USB_ST_NCT                      0x0a
#define USB_ST_PUF                      0x0b
#define USB_ST_EUF                      0x0c
#define USB_ST_MCMF                     0x0d
#define USB_ST_CCMF                     0x0e
#define USB_ST_ENF                      0x0f
#define USB_ST_ATMNF                    0x10

#define USB_ST_WHCM                     0x11
#define USB_ST_MDLM                     0x12
#define USB_ST_MDLMD                    0x13
#define USB_ST_DMM                      0x14
#define USB_ST_OBEX                     0x15
#define USB_ST_CS                       0x16
#define USB_ST_CSD                      0x17
#define USB_ST_TCM                      0x18
/*! @} */

/*!
 * @name Call Management - bmCapabilities
 *
 * c.f. CDC 5.2.3.2 Table 27
 * c.f. WMCD 
 */
#define USB_CMFD_DATA_CLASS              0x02
#define USB_CMFD_CALL_MANAGEMENT         0x01
/*! @} */

/*!
 * @name Abstract Control Management - bmCapabilities
 *
 * c.f. CDC 5.2.3.2 Table 28
 * c.f. WMCD 
 */
#define USB_ACMFD_NETWORK                 0x08
#define USB_ACMFD_SEND_BREAK              0x04
#define USB_ACMFD_CONFIG                  0x02
#define USB_ACMFD_COMM_FEATURE            0x01
/*! @} */

/*!
 * @name Class Descriptor Description Structures...
 * c.f. CDC 5.1
 * c.f. WCMC 6.7.2
 *
 * XXX add the other dozen class descriptor description structures....
 *
 */
 /*! @{ */
PACKED0 struct PACKED1 usbd_header_functional_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u16 bcdCDC;
};

PACKED0 struct PACKED1 usbd_call_management_functional_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bmCapabilities;
        u8 bDataInterface;
};

PACKED0 struct PACKED1 usbd_abstract_control_functional_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bmCapabilities;
};

PACKED0 struct PACKED1 usbd_union_functional_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bMasterInterface;
        u8 bSlaveInterface[4];
};

PACKED0 struct PACKED1 usbd_ethernet_networking_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        char *iMACAddress;
        u8 bmEthernetStatistics;
        u16 wMaxSegmentSize;
        u16 wNumberMCFilters;
        u8 bNumberPowerFilters;
};

PACKED0 struct PACKED1 usbd_mobile_direct_line_model_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u16 bcdVersion;
        u8 bGUID[16];
};

PACKED0 struct PACKED1 usbd_mobile_direct_line_model_detail_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bGuidDescriptorType;
        u8 bDetailData[4];
};
/*! @} */




/*!
 * @name Communications Class Dscriptor Structures
 * c.f. CDC 5.2 Table 25c
  */

/*! @{*/

PACKED0 struct PACKED1 usbd_class_function_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
};

PACKED0 struct PACKED1 usbd_class_function_descriptor_generic {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bmCapabilities;
};

PACKED0 struct PACKED1 usbd_class_header_function_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x00
        u16 bcdCDC;
};

PACKED0 struct PACKED1 usbd_class_call_management_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x01
        u8 bmCapabilities;
        u8 bDataInterface;
};

PACKED0 struct PACKED1  usbd_class_abstract_control_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x02
        u8 bmCapabilities;
};

PACKED0 struct PACKED1  usbd_class_direct_line_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x03
};

PACKED0 struct PACKED1  usbd_class_telephone_ringer_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x04
        u8 bRingerVolSeps;
        u8 bNumRingerPatterns;
};

PACKED0 struct PACKED1  usbd_class_telephone_call_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x05
        u8 bmCapabilities;
};

PACKED0 struct PACKED1 usbd_class_union_function_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x06
        u8 bMasterInterface;
        u8 bSlaveInterface[4];
};

PACKED0 struct PACKED1  usbd_class_country_selection_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x07
        u8 iCountryCodeRelDate;
        u16 wCountryCode0[4];
};


PACKED0 struct PACKED1 usbd_class_telephone_operational_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x08
        u8 bmCapabilities;
};


PACKED0 struct PACKED1 usbd_class_usbd_terminal_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x09
        u8 bEntityId;
        u8 bInterfaceNo;
        u8 bOutInterfaceNo;
        u8 bmOptions;
        u8 bChild0[4];
};

PACKED0 struct PACKED1 usbd_class_network_channel_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0a
        u8 bEntityId;
        u8 iName;
        u8 bChannelIndex;
        u8 bPhysicalInterface;
};

PACKED0 struct PACKED1 usbd_class_protocol_unit_function_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0b
        u8 bEntityId;
        u8 bProtocol;
        u8 bChild0[4];
};

PACKED0 struct PACKED1 usbd_class_extension_unit_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0c
        u8 bEntityId;
        u8 bExtensionCode;
        u8 iName;
        u8 bChild0[4];
};

PACKED0 struct PACKED1 usbd_class_multi_channel_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0d
        u8 bmCapabilities;
};

PACKED0 struct PACKED1 usbd_class_capi_control_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0e
        u8 bmCapabilities;
};

PACKED0 struct PACKED1 usbd_class_ethernet_networking_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x0f
        u8 iMACAddress;
        u32 bmEthernetStatistics;
        u16 wMaxSegmentSize;
        u16 wNumberMCFilters;
        u8 bNumberPowerFilters;
};

PACKED0 struct PACKED1 usbd_class_atm_networking_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x10
        u8 iEndSystermIdentifier;
        u8 bmDataCapabilities;
        u8 bmATMDeviceStatistics;
        u16 wType2MaxSegmentSize;
        u16 wType3MaxSegmentSize;
        u16 wMaxVC;
};


PACKED0 struct PACKED1 usbd_class_mdlm_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x12
        u16 bcdVersion;
        u8 bGUID[16];
};

/*
PACKED0 struct PACKED1 usbd_class_mdlmd_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x13
        u8 bGuidDescriptorType;
        u8 bDetailData[4];
};
*/

PACKED0 struct PACKED1 usbd_class_blan_descriptor {
        u8 bFunctionLength;           // 0x7
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x13
        u8 bGuidDescriptorType;
        u8 bmNetworkCapabilities;
        u8 bmDataCapabilities;
        u8 bPad;
};

PACKED0 struct PACKED1 usbd_class_safe_descriptor {
        u8 bFunctionLength;           // 0x6
        u8 bDescriptorType;
        u8 bDescriptorSubtype;        // 0x13
        u8 bGuidDescriptorType;
        u8 bmNetworkCapabilities;
        u8 bmDataCapabilities;
};

/*@}*/


#ifdef PRAGMAPACK
#pragma pack(pop)
#endif


