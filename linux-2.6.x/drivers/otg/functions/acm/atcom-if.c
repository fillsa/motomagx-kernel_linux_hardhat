/*
 * otg/functions/acm/atcom-fd.c
 * @(#) sl@belcarra.com|otg/functions/acm/atcom-if.c|20060403224909|21162
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 */
/*!
 * @file otg/functions/acm/atcom-if.c
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
 *    Descriptors     | atcom-if |        <-----
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
static u8 atcom_comm_endpoint_index[] = { INT_IN, };

/*! @name Class Descriptors
*/

/*@{*/
static struct usbd_class_header_function_descriptor atcom_class_1 = {
    bFunctionLength:    0x05,
    bDescriptorType:    USB_DT_CLASS_SPECIFIC,
    bDescriptorSubtype: USB_ST_HEADER,
    bcdCDC:             __constant_cpu_to_le16(0x0110),
};

static struct usbd_class_union_function_descriptor atcom_class_2 = {
     bFunctionLength:    0x04,
     bDescriptorType:    USB_DT_CLASS_SPECIFIC,
     bDescriptorSubtype: USB_ST_UF,
     bMasterInterface:   0x00,
};

static struct usbd_mobile_abstract_control_model_specific_desciptor atcom_class_3 = {
     bFunctionLength:    0x05,
     bDescriptorType:    MCPC_VENDOR_REQUEST,
     bDescriptorSubtype: MCPC_VENDOR_SUBREQUEST_MACM_AB,
     bType:              MCPC_PIPE_GROUP_AB_5,
     bMode_N:            { 0x02, },
};


/*@}*/

/*! ACMF Class Descriptor
 * ACMF - c.f. Table 28
 * currenty set to 0x2 - Support Set_Line_Coding etc,
 *
 * XXX Should we also set 0x4 - Supports Network_Notification?
 */
static struct usbd_generic_class_descriptor *atcom_comm_class_descriptors[] = {
        (struct usbd_generic_class_descriptor *) &atcom_class_1,
        (struct usbd_generic_class_descriptor *) &atcom_class_2,
        (struct usbd_generic_class_descriptor *) &atcom_class_3,
};


/*! Comm alternate descriptions
 */
static struct usbd_alternate_description atcom_comm_alternate_descriptions[] = {
      {
        .iInterface = CONFIG_OTG_ACM_COMM_INTF,
              .bInterfaceClass = COMMUNICATIONS_INTERFACE_CLASS,
              .bInterfaceSubClass = COMMUNICATIONS_ACM_SUBCLASS,
              .bInterfaceProtocol = 0x01,
              .classes = sizeof (atcom_comm_class_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
              .class_list =  atcom_comm_class_descriptors,
              .endpoints = sizeof (atcom_comm_endpoint_index) / sizeof(u8),
              .endpoint_index =  atcom_comm_endpoint_index,
      }, };


/*! Interface Descriptions List */
static struct usbd_interface_description atcom_interfaces[] = {
        {
                .alternates = sizeof (atcom_comm_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list =atcom_comm_alternate_descriptions,
        },
};

/*! Endpoint Request List
 * XXX this should be in acm.c
 */
static struct usbd_endpoint_request atcom_endpoint_requests[ENDPOINTS+1] = {
        { INT_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT, 16, 64, 4, },
        { 0, },
};


static struct usbd_function_operations atcom_function_ops;

/*! function_driver
 */
struct usbd_interface_driver atcom_function_driver = {
        .driver.name = "atcom-if",
        .driver.fops = &atcom_function_ops,
        .driver.function_type = function_interface,
        .interfaces = sizeof (atcom_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = atcom_interfaces,
        .endpointsRequested =  1,
        .requestedEndpoints =  atcom_endpoint_requests,
};

/*!
 *@brief atcom_if_endpoint_cleared - called by the USB Device Core when endpoint cleared
 * @param function_instance The function instance for this driver
 * @param bEndpointAddress
 * @return none

 */
static void atcom_if_endpoint_cleared (struct usbd_function_instance *function_instance, int bEndpointAddress)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        TRACE_MSG1(TTY, "CLEARED bEndpointAddress: %02x", bEndpointAddress);
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */
extern struct acm_os_ops tty_l26_os_ops;

/*! atcom_if_function_enable - called by USB Device Core to enable the driver
 * @param function_instance  The function instance for this driver to use.
 * @return non-zero if error.
 */
static int atcom_if_function_enable (struct usbd_function_instance *function_instance)
{
        TRACE_MSG0(TTY, "ENABLED");
        return acm_function_enable(function_instance, atcom_if, &tty_l26_os_ops, 1, 256);
}


/* ********************************************************************************************* */


/*!
 * @ brief atcom_if_term
 * @return none
 */
void atcom_if_exit(void)
{
        TRACE_MSG0(TTY, "EXIT");
        //acm_wait_task(acm->recv_tqueue);
        usbd_deregister_interface_function (&atcom_function_driver);
}

/*! atcom_function_ops
 */
static struct usbd_function_operations atcom_function_ops = {
        .function_enable = atcom_if_function_enable,
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
 * @brief atcom_if_init
 * @param usbd_module_info
 * @return non-zero if error
 */
int atcom_if_init(char *usbd_module_info)
{
        TRACE_MSG0(TTY, "INIT");
        return usbd_register_interface_function (&atcom_function_driver, "atcom-if", NULL);
}

