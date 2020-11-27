/*
 * otg/otg/usbp-chap9.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/usbp-chap9.h|20051116205002|15860
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
 * @defgroup USBP USB Peripheral Support
 * @ingroup onthegogroup
 */

 
/*!
 * @file otg/otg/usbp-chap9.h
 * @brief Chapter 9 and other class descriptor structure definitions.
 *
 * USB Descriptors are used to build a configuration database for each USB
 * Function driver.
 *
 * @ingroup USBP
 */

/*!
 * @name Device and/or Interface Class Codes
 */

 /*! @{ */


#define USB_CLASS_PER_INTERFACE         0       /* for DeviceClass */
#define USB_CLASS_AUDIO                 1
#define USB_CLASS_COMM                  2
#define USB_CLASS_HID                   3
#define USB_CLASS_PHYSICAL              5
#define USB_CLASS_PRINTER               7
#define USB_CLASS_MASS_STORAGE          8
#define USB_CLASS_HUB                   9
#define USB_CLASS_DATA                  10
#define USB_CLASS_MISC                  0xef
#define USB_CLASS_APP_SPEC              0xfe
#define USB_CLASS_VENDOR_SPEC           0xff

/*! @} */


/*! @name USB Device Request Types (bmRequestType)
 *C.f. USB 2.0 Table 9-2
*/

/*! @{ */

#define USB_TYPE_STANDARD               (0x00 << 5)
#define USB_TYPE_CLASS                  (0x01 << 5)
#define USB_TYPE_VENDOR                 (0x02 << 5)
#define USB_TYPE_RESERVED               (0x03 << 5)

#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

#define USB_RECIP_HUB                   0x00
#define USB_RECIP_PORT                  0x03

#define USB_DIR_OUT                     0
#define USB_DIR_IN                      0x80
#define USB_ENDPOINT_OPT                0x40
/*! @} */

/*! @name Descriptor types
 * C.f. USB Table 9-5
 */

/*! @{ */

#define USB_DT_DEVICE                           1
#define USB_DT_CONFIGURATION                    2
#define USB_DT_STRING                           3
#define USB_DT_INTERFACE                        4
#define USB_DT_ENDPOINT                         5
#define USB_DT_DEVICE_QUALIFIER                 6
#define USB_DT_OTHER_SPEED_CONFIGURATION        7
#define USB_DT_INTERFACE_POWER                  8
#define USB_DT_OTG                              9
#define USB_DT_DEBUG                            10

#define USB_DT_INTERFACE_ASSOCIATION            11
#define USB_DT_CLASS_SPECIFIC                   0x24


#define USB_DT_HID                      (USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT                   (USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL                 (USB_TYPE_CLASS | 0x03)
#define USB_DT_HUB                      (USB_TYPE_CLASS | 0x09)
/*! @} */

#if 0
/*! @name Descriptor sizes per descriptor type
 */
 /*! @{ */

#define USB_DT_DEVICE_SIZE              18
#define USB_DT_CONFIG_SIZE              9
#define USB_DT_INTERFACE_SIZE           9
#define USB_DT_ENDPOINT_SIZE            7
#define USB_DT_ENDPOINT_AUDIO_SIZE      9               /* Audio extension */
#define USB_DT_HUB_NONVAR_SIZE          7
#define USB_DT_HID_SIZE                 9
/*! @} */
#endif

/*! @name Endpoints
 */
/*! @{ */
#define USB_ENDPOINT_NUMBER_MASK        0x0f            /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK           0x80

#define USB_ENDPOINT_MASK               0x03            /* in bmAttributes */
#define USB_ENDPOINT_CONTROL            0x00
#define USB_ENDPOINT_ISOCHRONOUS        0x01
#define USB_ENDPOINT_BULK               0x02
#define USB_ENDPOINT_INTERRUPT          0x03
/*! @} */

/*!
 * @name USB Packet IDs (PIDs)
 */
/*! @{ */
#define USB_PID_UNDEF_0                 0xf0
#define USB_PID_OUT                     0xe1
#define USB_PID_ACK                     0xd2
#define USB_PID_DATA0                   0xc3
#define USB_PID_PING                    0xb4             /* USB 2.0 */
#define USB_PID_SOF                     0xa5
#define USB_PID_NYET                    0x96             /* USB 2.0 */
#define USB_PID_DATA2                   0x87             /* USB 2.0 */
#define USB_PID_SPLIT                   0x78             /* USB 2.0 */
#define USB_PID_IN                      0x69
#define USB_PID_NAK                     0x5a
#define USB_PID_DATA1                   0x4b
#define USB_PID_PREAMBLE                0x3c             /* Token mode */
#define USB_PID_ERR                     0x3c             /* USB 2.0: handshake mode */
#define USB_PID_SETUP                   0x2d
#define USB_PID_STALL                   0x1e
#define USB_PID_MDATA                   0x0f             /* USB 2.0 */
/*! @} */

/*! * @name Standard requests
 */

  /*! @{ */

#define USB_REQ_GET_STATUS              0x00
#define USB_REQ_CLEAR_FEATURE           0x01
#define USB_REQ_SET_FEATURE             0x03
#define USB_REQ_SET_ADDRESS             0x05
#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_REQ_SET_DESCRIPTOR          0x07
#define USB_REQ_GET_CONFIGURATION       0x08
#define USB_REQ_SET_CONFIGURATION       0x09
#define USB_REQ_GET_INTERFACE           0x0A
#define USB_REQ_SET_INTERFACE           0x0B
#define USB_REQ_SYNCH_FRAME             0x0C
/*! @} */

/*!
 * @name USB Spec Release number
 */
 /*! @{ */

#define USB_BCD_VERSION_11              0x0110
#define USB_BCD_VERSION_20              0x0200

#if defined(CONFIG_OTG_HIGH_SPEED) || defined(CONFIG_OTG_BDEVICE_WITH_SRP) || defined(CONFIG_OTG_DEVICE)
#define USB_BCD_VERSION                 USB_BCD_VERSION_20
#else /* CONFIG_OTG_HIGH_SPEED */
#define USB_BCD_VERSION                 USB_BCD_VERSION_11
#endif /* CONFIG_OTG_HIGH_SPEED */

/*! @} */


/*!
 * @name Device Requests      (c.f Table 9-2)
 */
 /*! @{ */

#define USB_REQ_DIRECTION_MASK          0x80
#define USB_REQ_TYPE_MASK               0x60
#define USB_REQ_RECIPIENT_MASK          0x1f

#define USB_REQ_DEVICE2HOST             0x80
#define USB_REQ_HOST2DEVICE             0x00

#define USB_REQ_TYPE_STANDARD           0x00
#define USB_REQ_TYPE_CLASS              0x20
#define USB_REQ_TYPE_VENDOR             0x40

#define USB_REQ_RECIPIENT_DEVICE        0x00
#define USB_REQ_RECIPIENT_INTERFACE     0x01
#define USB_REQ_RECIPIENT_ENDPOINT      0x02
#define USB_REQ_RECIPIENT_OTHER         0x03
/*! @} */


/*!
 * @name get status bits
 */
 /*! @{ */

#define USB_STATUS_SELFPOWERED          0x01
#define USB_STATUS_REMOTEWAKEUP         0x02

#define USB_STATUS_HALT                 0x01
/*! @} */

/*!
 * @name Convert Feature Selector to a Status Flag Setting
 */
 /*! @{ */
#define FEATURE(f)                      (1 << f)
/*! @} */

/*!
 * @name standard feature selectors
 */
 /*! @{ */
#define USB_ENDPOINT_HALT               0x00
#define USB_DEVICE_REMOTE_WAKEUP        0x01
#define USB_TEST_MODE                   0x02
/*! @} */

#ifdef PRAGMAPACK
#pragma pack(push,1)
#endif

/* Note:
The various structs declared here need 1-byte 
alignment. These is achieved in various ways
with assorted C compilers.
First of all, some compilers need the pack()
#pragma. This is enabled by defining the PRAGMAPACK
macro

Secondly the structure definitions themselves
may require specialized additions to their declarations.
The following model seems to accommodate all compilers
known so far:


PACKED0 struct PACKED1 {
    various items
};

Define appropriate values for PACKED0, PACKED1, PACKED2
suited to your compiler. In most cases at least two of 
these values will be empty strings.

*/


PACKED0 struct PACKED1 usbd_device_request {
        u8 bmRequestType;
        u8 bRequest;
        u16 wValue;
        u16 wIndex;
        u16 wLength;
};



PACKED0 struct PACKED1 usbd_otg_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u8 bmAttributes;
};

/*!
 * @name OTG
 */
 /*! @{ */
#define USB_OTG_HNP_SUPPORTED   0x02
#define USB_OTG_SRP_SUPPORTED   0x01
/*! @} */

/*!
 * @name OTG Feature selectors
 */
 /*! @{ */
#define USB_OTG_B_HNP_ENABLE     0x03
#define USB_OTG_A_HNP_SUPPORT    0x04
#define USB_OTG_A_ALT_HNP_ENABLE 0x05
/*! @} */


/*!
 * @name Endpoint Modifiers
 * static struct usbd_endpoint_description function_default_A_1[] = {
 *
 *     {this_endpoint: 0, attributes: CONTROL,   max_size: 8,  polling_interval: 0 },
 *     {this_endpoint: 1, attributes: BULK,      max_size: 64, polling_interval: 0, direction: IN},
 *     {this_endpoint: 2, attributes: BULK,      max_size: 64, polling_interval: 0, direction: OUT},
 *     {this_endpoint: 3, attributes: INTERRUPT, max_size: 8,  polling_interval: 0},
 *
 *
 */
 /*! @{ */

#define CONTROL         0x00
#define ISOCHRONOUS     0x01
#define BULK            0x02
#define INTERRUPT       0x03
/*! @} */


/*! @name configuration modifiers
 */

/* @{ */
#define BMATTRIBUTE_RESERVED            0x80
#define BMATTRIBUTE_SELF_POWERED        0x40
#define BMATTRIBUTE_REMOTE_WAKEUP       0x20

#if 0
/*
 * The UUT tester specifically tests for MaxPower to be non-zero (> 0).
 */
#if !defined(CONFIG_OTG_MAXPOWER) || (CONFIG_OTG_MAXPOWER == 0)
        #define BMATTRIBUTE BMATTRIBUTE_RESERVED | BMATTRIBUTE_SELF_POWERED
        #define BMAXPOWER 1
#else
        #define BMATTRIBUTE BMATTRIBUTE_RESERVED
        #define BMAXPOWER CONFIG_USBD_MAXPOWER
#endif
#endif
/*! @} */



/*!
 * @name Standard Usb Descriptor Structures
 */
 /*! @{ */

PACKED0 struct PACKED1 usbd_endpoint_descriptor {
        u8 bLength;
        u8 bDescriptorType;   // 0x5
        u8 bEndpointAddress;
        u8 bmAttributes;
        u16 wMaxPacketSize;
        u8 bInterval;
};

PACKED0 struct PACKED1 usbd_interface_descriptor {
        u8 bLength;
        u8 bDescriptorType;   // 0x04
        u8 bInterfaceNumber;
        u8 bAlternateSetting;
        u8 bNumEndpoints;
        u8 bInterfaceClass;
        u8 bInterfaceSubClass;
        u8 bInterfaceProtocol;
        u8 iInterface;
};

PACKED0 struct PACKED1 usbd_configuration_descriptor {
        u8 bLength;
        u8 bDescriptorType;   // 0x2
        u16 wTotalLength;
        u8 bNumInterfaces;
        u8 bConfigurationValue;
        u8 iConfiguration;
        u8 bmAttributes;
        u8 bMaxPower;
};

PACKED0 struct PACKED1 usbd_device_descriptor {
        u8 bLength;
        u8 bDescriptorType;   // 0x01
        u16 bcdUSB;
        u8 bDeviceClass;
        u8 bDeviceSubClass;
        u8 bDeviceProtocol;
        u8 bMaxPacketSize0;
        u16 idVendor;
        u16 idProduct;
        u16 bcdDevice;
        u8 iManufacturer;
        u8 iProduct;
        u8 iSerialNumber;
        u8 bNumConfigurations;
};

/* Define according to the whims of the compiler */
#define USBD_WDATA_MIN 1    /* Some compilers will allow 0 */
PACKED0 struct PACKED1 usbd_string_descriptor {
        u8 bLength;
        u8 bDescriptorType;   // 0x03
        u16 wData[USBD_WDATA_MIN];
};

PACKED0 struct PACKED1 usbd_langid_descriptor {
        u8 bLength;
        u8 bDescriptorType;   // 0x03
        u8 bData[USBD_WDATA_MIN];
};

#if (USBD_WDATA_MIN > 0)
#define USBD_STRING_SIZEOF(x) (sizeof(x) - 2*(USBD_WDATA_MIN))
#else
#define USBD_STRING_SIZEOF(x)  (sizeof(x))
#endif


struct usbd_device_qualifier_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u16 bcdUSB;
        u8 bDeviceClass;
        u8 bDeviceSubClass;
        u8 bDeviceProtocol;
        u8 bMaxPacketSize0;
        u8 bNumConfigurations;
        u8 bReserved;
};


struct usbd_generic_descriptor {
        u8 bLength;
        u8 bDescriptorType;
};

struct usbd_generic_class_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
};
/*! @} */

/*!
 * @name Interface Association Descriptor
 */
 /*! @{ */
PACKED0 struct PACKED1 usbd_interface_association_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u8 bFirstInterface;
        u8 bInterfaceCount;
        u8 bFunctionClass;
        u8 bFunctionSubClass;
        u8 bFunctionProtocol;
        u8 iFunction;
};

/*! @} */

#if 0
/*!
 * @name Descriptor Union Structures
 */
 /*! @{ */

struct usbd_descriptor {
        union {
                struct usbd_generic_descriptor generic_descriptor;
                struct usbd_generic_class_descriptor generic_class_descriptor;
                struct usbd_endpoint_descriptor endpoint_descriptor;
                struct usbd_interface_descriptor interface_descriptor;
                struct usbd_configuration_descriptor configuration_descriptor;
                struct usbd_device_descriptor device_descriptor;
                struct usbd_string_descriptor string_descriptor;
        } descriptor;

};

struct usbd_class_descriptor {
        union {
                struct usbd_class_function_descriptor function;
                struct usbd_class_function_descriptor_generic generic;
                struct usbd_class_header_function_descriptor header_function;
                struct usbd_class_call_management_descriptor call_management;
                struct usbd_class_abstract_control_descriptor abstract_control;
                struct usbd_class_direct_line_descriptor direct_line;
                struct usbd_class_telephone_ringer_descriptor telephone_ringer;
                struct usbd_class_telephone_operational_descriptor telephone_operational;
                struct usbd_class_telephone_call_descriptor telephone_call;
                struct usbd_class_union_function_descriptor union_function;
                struct usbd_class_country_selection_descriptor country_selection;
                struct usbd_class_usbd_terminal_descriptor usb_terminal;
                struct usbd_class_network_channel_descriptor network_channel;
                struct usbd_class_extension_unit_descriptor extension_unit;
                struct usbd_class_multi_channel_descriptor multi_channel;
                struct usbd_class_capi_control_descriptor capi_control;
                struct usbd_class_ethernet_networking_descriptor ethernet_networking;
                struct usbd_class_atm_networking_descriptor atm_networking;
                struct usbd_class_mdlm_descriptor mobile_direct;
                //struct usbd_class_mdlmd_descriptor mobile_direct_detail;
                struct usbd_class_blan_descriptor mobile_direct_blan_detail;
                struct usbd_class_safe_descriptor mobile_direct_safe_detail;
        } descriptor;

};
/*! @} */
#endif


/*! 
 * Device Events 
 *
 * These are defined in the USB Spec (c.f USB Spec 2.0 Figure 9-1).
 *
 * There are additional events defined to handle some extra actions we need to have handled.
 *
 */
typedef enum usbd_device_event {

        DEVICE_UNKNOWN,         // bi - unknown event
        DEVICE_INIT,            // bi  - initialize
        DEVICE_CREATE,          // bi  - 
        DEVICE_HUB_CONFIGURED,  // bi  - bus has been plugged int
        DEVICE_RESET,           // bi  - hub has powered our port

        DEVICE_ADDRESS_ASSIGNED,// ep0 - set address setup received
        DEVICE_CONFIGURED,      // ep0 - set configure setup received
        DEVICE_SET_INTERFACE,   // ep0 - set interface setup received

        DEVICE_SET_FEATURE,     // ep0 - set feature setup received
        DEVICE_CLEAR_FEATURE,   // ep0 - clear feature setup received

        DEVICE_DE_CONFIGURED,   // ep0 - set configure setup received for ??

        DEVICE_BUS_INACTIVE,    // bi  - bus in inactive (no SOF packets)
        DEVICE_BUS_ACTIVITY,    // bi  - bus is active again

        DEVICE_POWER_INTERRUPTION,// bi  - hub has depowered our port
        DEVICE_HUB_RESET,       // bi  - bus has been unplugged
        DEVICE_DESTROY,         // bi  - device instance should be destroyed
        DEVICE_CLOSE,           // bi  - device instance should be destroyed

} usbd_device_event_t;

/*! USB Request Block structure 
 *
 * This is used for both sending and receiving data. 
 *
 * The callback function is used to let the function driver know when
 * transmitted data has been sent.
 *
 * The callback function is set by the alloc_recv function when an urb is
 * allocated for receiving data for an endpoint and used to call the
 * function driver to inform it that data has arrived.
 *
 * Note that for OUT urbs the buffer is always allocated to a multiple of
 * the packetsize that is 1 larger than the requested size. This prevents
 * overflow if the host unexpectedly sends a full sized packet when we are
 * expecting a short one (the host is always right..)
 *
 * USBD_URB_SENDZLP - set this flag if a ZLP (zero length packet) should be
 * sent after the data is sent. This is required if sending data that is a
 * multiple of the packetsize but less than the maximum transfer size for
 * the protocol in use.  A ZLP is required to ensure that the host
 * recognizes a short transfer.
 *
 */

typedef struct urb_link {
        struct urb_link *next;
        struct urb_link *prev;
} urb_link;


/*! URB Status
 *
 * This defines the current status of a pending or finshed URB.
 *
 */
typedef enum usbd_urb_status {
        USBD_URB_OK = 0,
        USBD_URB_IN_QUEUE,
        USBD_URB_ACTIVE,
        USBD_URB_CANCELLED,
        USBD_URB_ERROR,
        USBD_URB_STALLED,
        USBD_URB_RESET,
        USBD_URB_NOT_READY,
        USBD_URB_DISABLED,
        #if 0   /* Deprecated */
        //SEND_FINISHED_OK = USBD_URB_OK,
        //SEND_IN_QUEUE = USBD_URB_IN_QUEUE,
        //SEND_IN_PROGRESS = USBD_URB_ACTIVE,
        //SEND_FINISHED_ERROR = USBD_URB_ERROR,
        //SEND_CANCELLED = USBD_URB_CANCELLED,
        //SEND_STALLED = USBD_URB_STALLED,
        //SEND_DISABLED = USBD_URB_DISABLED,
        //SEND_RESET = USBD_URB_RESET,
        //SEND_NOT_READY = USBD_URB_NOT_READY,

        //RECV_OK = USBD_URB_OK,
        //RECV_IN_QUEUE = USBD_URB_IN_QUEUE,
        //RECV_IN_PROGRESS = USBD_URB_ACTIVE,
        //RECV_ERROR = USBD_URB_ERROR,
        //RECV_CANCELLED = USBD_URB_CANCELLED,
        //RECV_STALLED = USBD_URB_STALLED,
        //RECV_DISABLED = USBD_URB_DISABLED,
        //RECV_RESET = USBD_URB_RESET,
        //RECV_NOT_READY = USBD_URB_NOT_READY,
        #endif /* Deprecated */
} usbd_urb_status_t;

#define USBD_URB_SENDZLP        0x01    /* send a Zero Length Packet when urb is finished */
#define USBD_URB_SOF            0x02    /* terminate a receive transfer at SOF if any data received */
#define USBD_URB_SOF2           0x04    /* terminate a receive transfer at SOF if no data since previous SOF (some data was received) */

struct usbd_urb {

        struct usbd_bus_instance        *bus;
        struct usbd_function_instance   *function_instance;
        struct usbd_endpoint_instance   *endpoint;

        int                             (*callback) (struct usbd_urb *, int);

        u8                              *buffer;        // data received (OUT) or being sent (IN)
        u32                             buffer_length;  // maximum data expected for OUT
        u32                             alloc_length;   // allocated size of buffer
        u32                             actual_length;  // actual data received (OUT or being sent (IN)
        u32                             flags;
        
        void                            *function_privdata;
        void                            *bus_privdata;

        struct urb_link                 link;   
        usbd_urb_status_t               status;         // what is the current status of the urb
        u16                             framenum;       // SOF framenum when urb was finished
};

/*! Endpoint Request
 *
 * An array of these structures is initialized by each function driver to specify the endpoints
 * it requires. 
 *
 * The bus interface driver will attempt to fulfill these requests with the actual endpoints it
 * has available.
 *
 * Note that in most cases the bEndpointAddress should be left as zero except for specialized
 * function drivers that both require a specific value and know ahead of time that the specified
 * value is legal for the bus interface driver being used.
 */
struct usbd_endpoint_request {
        u8 index;                       /* function specific index */
        u8 configuration;               /* configuration endpoint will be in */
        u8 interface_num;               /* interface endpoint will be in */
        u8 alternate;                   /* altsetting for this request */
        u8 bmAttributes;                /* endpoint type AND direction */
        u16 fs_requestedTransferSize;   /* max full speed transfer size for this endpoint */
        u16 hs_requestedTransferSize;   /* max high speed transfer size for this endpoint */
        u8 bInterval;
        u8 bEndpointAddress;            /* specific bEndpointAddress function driver requires */
        u8 physical;                    /* physical endpoint used */
};

/*! Endpoint Map
 *
 * An array of these structures is created by the bus interface driver to
 * show what endpoints have been configured for the function driver.
 *
 * This is the logical array of endpoints that are indexed into by the 
 * function layer using the logical endpoint numbers defined by the function
 * drivers. It maps these numbers into a real physical endpoint.
 *
 * Fields that can be different for Full speed versus High speed are 
 * represented with an array both values are available.
 *
 * For interfaces with multiple alternate settings, each separate
 * combinantion of interface_num/altsetting/endpoint must be specified.
 * It is up to the lower layers to determine if / when overlapped 
 * endpoints can be re-used.
 *
 */
struct usbd_endpoint_map {
        u8 index;
        u8 configuration;
        u8 interface_num;
        u8 alternate;
        u8 bEndpointAddress[2];         // logical endpoint address
        u16 wMaxPacketSize[2];          // packetsSize for requested endpoint
        u8 bmAttributes[2];             // requested endpoint type
        u16 transferSize[2];            // transferSize for bulk transfers
        u8 physicalEndpoint[2];         // physical endpoint number
        u8 bInterval[2];
        struct usbd_endpoint_instance    *endpoint;
};


/*!
 * Device status
 *
 * Overall state, we use this to show when we are suspended.
 * This is required because when resumed we need to go back
 * to previous state.
 */
typedef enum usbd_device_status {
        USBD_OPENING,                   // 0. we are currently opening
        USBD_RESETING,                  // 1. we are currently opening
        USBD_OK,                        // 2. ok to use
        USBD_SUSPENDED,                 // 3. we are currently suspended
        USBD_CLOSING,                   // 4. we are currently closing
        USBD_CLOSED,                    // 5. we are currently closing
        USBD_UNKNOWN,
} usbd_device_status_t;


/*! 
 * Device State (c.f USB Spec 2.0 Figure 9-1)
 *
 * What state the usb device is in. 
 *
 * Note the state does not change if the device is suspended, we simply set a
 * flag to show that it is suspended.
 *
 */
typedef enum usbd_device_state {
        STATE_INIT,                     // 0. just initialized
        STATE_CREATED,                  // 1. just created
        STATE_ATTACHED,                 // 2. we are attached 
        STATE_POWERED,                  // 3. we have seen power indication (electrical bus signal)
        STATE_DEFAULT,                  // 4. we been reset
        STATE_ADDRESSED,                // 5. we have been addressed (in default configuration)
        STATE_CONFIGURED,               // 6. we have seen a set configuration device command
        STATE_SUSPENDED,                // 7. device has been suspended
        STATE_UNKNOWN,                  // 8. destroyed
} usbd_device_state_t;



#ifdef PRAGMAPACK
#pragma pack(pop)
#endif


