/*
 * otg/functions/acm/dun-fd.c
 * @(#) sl@belcarra.com|otg/functions/acm/dun-if.c|20060403224909|44530
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 */
/*!
 * @file otg/functions/acm/dun-if.c
 * @brief ACM-TTY Descriptor Set
 *
 * This is the generic portion of the TTY version of the
 * ACM driver.
 *
 *                    TTY
 *                    Interface
 *    Upper           +----------+
 *    Edge            | tty-l26  |
 *    Implementation  +----------+
 *
 *
 *    Function        +----------+
 *    Descriptors     | dun-if   |        <-----
 *    Registration    +----------+
 *
 *
 *    Function        +----------+
 *    I/O             | acm      |
 *    Implementation  +----------+
 *
 *
 *    Module          +----------+
 *    Loading         | acm-l26  |
 *                    +----------+
 *
 *
 *
 * @ingroup TTYFunction
 */


#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/smp_lock.h>
#include <linux/slab.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>
#include <otg/usbp-mcpc.h>


#include <otg/otg-trace.h>
#include "acm.h"

/*
 * CDC ACM Configuration
 *
 * Endpoint, Class, Interface, Configuration and Device descriptors/descriptions
 */
/*! Endpoint Index Lists
 */
static u8 tty_alt_endpoint_index[] = { BULK_IN, BULK_OUT, };
static u8 tty_comm_endpoint_index[] = { INT_IN, };

/** @name Class Descriptors

*/

/*@{*/



static struct usbd_class_header_function_descriptor dun_class_1 = {
    bFunctionLength:    0x05,
    bDescriptorType:    USB_DT_CLASS_SPECIFIC,
    bDescriptorSubtype: USB_ST_HEADER,
    bcdCDC:             __constant_cpu_to_le16(0x0110),
};

static struct usbd_call_management_functional_descriptor dun_class_2 = {
    bFunctionLength:    0x05,
    bDescriptorType:    USB_DT_CLASS_SPECIFIC,
    bDescriptorSubtype: USB_ST_CMF,
    bmCapabilities:     USB_CMFD_CALL_MANAGEMENT | USB_CMFD_DATA_CLASS,
    bDataInterface:     1,              /* offset to data interface within pipe group */
};

static struct usbd_abstract_control_functional_descriptor dun_class_3 = {
     bFunctionLength:    0x04,
     bDescriptorType:    USB_DT_CLASS_SPECIFIC,
     bDescriptorSubtype: USB_ST_ACMF,
     bmCapabilities:     USB_ACMFD_CONFIG | USB_ACMFD_SEND_BREAK ,
};

static struct usbd_class_union_function_descriptor dun_class_4 = {
     bFunctionLength:    0x05,
     bDescriptorType:    USB_DT_CLASS_SPECIFIC,
     bDescriptorSubtype: USB_ST_UF,
     bMasterInterface:   0x00,
     bSlaveInterface:    { 1 },         /* offset to slave interface within pipe group */
};

static struct usbd_mobile_abstract_control_model_specific_desciptor dun_class_5 = {
     bFunctionLength:    0x06,
     bDescriptorType:    MCPC_VENDOR_REQUEST,
     bDescriptorSubtype: MCPC_VENDOR_SUBREQUEST_MACM_AB,
     bType:              MCPC_PIPE_GROUP_AB_2,
     bMode_N:            { 0x02, 0xc0, },
};

/*@}*/

/*! ACMF Class Descriptor
 * ACMF - c.f. Table 28
 * currenty set to 0x2 - Support Set_Line_Coding etc,
 *
 * XXX Should we also set 0x4 - Supports Network_Notification?
 */
static struct usbd_generic_class_descriptor *dun_comm_class_descriptors[] = {
        (struct usbd_generic_class_descriptor *) &dun_class_1,
        (struct usbd_generic_class_descriptor *) &dun_class_2,
        (struct usbd_generic_class_descriptor *) &dun_class_3,
        (struct usbd_generic_class_descriptor *) &dun_class_4,
        (struct usbd_generic_class_descriptor *) &dun_class_5,
};


/*! Comm alternate descriptions
 */
static struct usbd_alternate_description dun_comm_alternate_descriptions[] = {
      {
        .iInterface = CONFIG_OTG_ACM_COMM_INTF,
              .bInterfaceClass = COMMUNICATIONS_INTERFACE_CLASS,
              .bInterfaceSubClass = 0x88,
              .bInterfaceProtocol = 0x01,
              .classes = sizeof (dun_comm_class_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
              .class_list =  dun_comm_class_descriptors,
              .endpoints = sizeof (tty_comm_endpoint_index) / sizeof(u8),
              .endpoint_index =  tty_comm_endpoint_index,
      }, };

/*! Data alternate descriptions
 */
static struct usbd_alternate_description dun_data_alternate_descriptions[] = {
      #if 0
      {
              .iInterface = CONFIG_OTG_ACM_NODATA_INTF,
              .bInterfaceClass = DATA_INTERFACE_CLASS,
              .bInterfaceSubClass = 0x0,
              .bInterfaceProtocol = 0x0,
      },
      #endif
      {
              .iInterface = CONFIG_OTG_ACM_DATA_INTF,
              .bInterfaceClass = DATA_INTERFACE_CLASS,
              .bInterfaceSubClass = COMMUNICATIONS_NO_SUBCLASS,
              .bInterfaceProtocol = COMMUNICATIONS_NO_PROTOCOL,
              .endpoints = sizeof (tty_alt_endpoint_index) / sizeof(u8),
              .endpoint_index =  tty_alt_endpoint_index,
      },
};


/*! Interface Descriptions List */
static struct usbd_interface_description dun_interfaces[] = {
        {
                .alternates = sizeof (dun_comm_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = dun_comm_alternate_descriptions,
        },
        {
                .alternates = sizeof (dun_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = dun_data_alternate_descriptions,
        },
};

/*! Endpoint Request List
 * XXX this should be in acm.c
 */
static struct usbd_endpoint_request tty_endpoint_requests[ENDPOINTS+1] = {
        { INT_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT, 16, 64, 4, },
        { BULK_IN, 1, 1, 0, USB_DIR_IN | USB_ENDPOINT_BULK, 64 , 512, 0, },
        { BULK_OUT, 1, 1, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, 64, 512, 0, },
        { 0, },
};


static struct usbd_function_operations dun_function_ops;

/*! function_driver
 */
struct usbd_interface_driver dun_function_driver = {
        .driver.name = "dun-if",
        .driver.fops = &dun_function_ops,
        .driver.function_type = function_interface,
        .interfaces = sizeof (dun_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = dun_interfaces,
        .endpointsRequested =  ENDPOINTS,
        .requestedEndpoints =  tty_endpoint_requests,
        .bFunctionClass = COMMUNICATIONS_INTERFACE_CLASS,
        .bFunctionSubClass = 0x88,
        .bFunctionProtocol = 0x01,
};

/*!
 * @brief dun_if_endpoint_cleared - called by the USB Device Core when endpoint cleared
 * @param function_instance The function instance for this driver
 * @param bEndpointAddress
 * @return none
 */
static void dun_if_endpoint_cleared (struct usbd_function_instance *function_instance, int bEndpointAddress)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        TRACE_MSG1(TTY, "CLEARED bEndpointAddress: %02x", bEndpointAddress);
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */
extern struct acm_os_ops tty_l26_os_ops;

/*!
 * @brief dun_if_function_enable - called by USB Device Core to enable the driver
 * @param function_instance The function instance for this driver to use.
 * @return non-zero if error.
 */
static int dun_if_function_enable (struct usbd_function_instance *function_instance)
{
        TRACE_MSG0(TTY, "ENABLED");
        return acm_function_enable(function_instance, dun_if, &tty_l26_os_ops, 4, 512);
}

/* ********************************************************************************************* */


/*!
 * @brief dun_if_term

 */
void dun_if_exit(void)
{
        TRACE_MSG0(TTY, "EXIT");
        //acm_wait_task(acm->recv_tqueue);
        usbd_deregister_interface_function (&dun_function_driver);
}

/*! dun_function_ops
 */
static struct usbd_function_operations dun_function_ops = {
        .function_enable = dun_if_function_enable,
        .function_disable = acm_function_disable,

        .device_request = acm_device_request,
        .endpoint_cleared = acm_endpoint_cleared,
        .set_configuration = acm_set_configuration,
        .set_interface = acm_set_interface,
        .reset = acm_reset,
        .suspended = acm_suspended,
        .resumed = acm_resumed,
};

/*!
 * @brief dun_if_init
  * @param usbd_module_info
 * @return non-zero if error
 */
int dun_if_init(char *usbd_module_info)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        TRACE_MSG0(TTY, "INIT");
        return usbd_register_interface_function (&dun_function_driver, "dun-if", NULL);
}

