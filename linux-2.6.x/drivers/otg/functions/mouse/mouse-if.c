/*
 * otg/functions/mouse/mouse-if.c
 * @(#) balden@seth2.belcarratech.com|otg/functions/mouse/mouse-if.c|20051116204958|08117
 *
 *      Copyright (c) 2003-2005 Belcarra
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         dn Initial distribution 
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA
 *
 */
/*!
 * @file otg/functions/mouse/mouse-if.c
 * @brief Mouse Interface Function Driver protocol implementation.
 *
 * This file implements the Mouse HID protocols and will generate
 * random mouse data on the INTERRUPT endpoint.
 *
 * The primary purpose of this driver is to demonstrate how to
 * implement a simple function driver and to provide a simple
 * uni-directional driver for testing USB Device peripheral drivers.
 *
 * The mouse driver has several other characteristics to allow testing of
 * other features of USB Device peripheral drivers:
 *
 *      - ep0 delayed CONTROL READ
 *
 * The delayed CONTROL READ test verifies that that USB Device peripheral
 * drivers will correctly NAK until data is provide for device requests.
 * This is done by (optionally) delaying the HID report via a bottom half
 * handler. The device request returns normally and the peripheral driver
 * must properly recognize that while the device request had a non-zero
 * wLength field, there is currently no queued urb. 
 *
 * When the bottom half handler is scheduled the HID report urb will be
 * queued on endpoint zero and then returned to the host.
 *
 * The CONFIG_OTG_MOUSE_PACKETS option is used to set the number of mouse
 * data reports to send. Set to zero for a continous stream of data.
 *
 *
 * @ingroup MouseFunction
 */

#include <linux/config.h>
#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-hid.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#define MOUSE mouse_if_trace_tag
extern otg_tag_t MOUSE;

int mouse_if_bh_active;

/*!
 * The mouse_if_private structure is used to collect all of the mouse driver 
 * global variables into one place.
 */
struct mouse_if_private {

#ifdef CONFIG_OTG_MOUSE_BH
        struct WORK_STRUCT notification_bh;
#endif /* CONFIG_OTG_MOUSE_BH */

        u8 interface_index;
        u8 bEndpointAddress;

        unsigned int writesize;       /*!< packetsize * 4 */

        int x;
        int y;
        int last_x;
        int last_y;
        int n;
        int mouse_count;

        int wLength;                            
        u16 pending_report;
        u16 idle;
        u16 protocol;
};

/*! 
 * @name Mouse Descriptors
 *
 * MouseHIDReport
 * MOUSE Configuration
 *
 * Endpoint, Class, Interface, Configuration and Device descriptors/descriptions
 */

/*@{*/ 

/*! This is the HID report returned for the HID Device Request.
 *  There is no pre-defined descriptor type for this.
 */
static char MouseHIDReport[52] = {
    0x05, 0x01,                    /*!< USAGE_PAGE (Generic Desktop)            */
    0x09, 0x02,                    /*!< USAGE (Mouse)                           */
    0xa1, 0x01,                    /*!< COLLECTION (Application)                */
    0x09, 0x01,                    /*!<   USAGE (Pointer)                       */
    0xa1, 0x00,                    /*!<   COLLECTION (Physical)                 */
    0x05, 0x09,                    /*!<     USAGE_PAGE (Button)                 */
    0x19, 0x01,                    /*!<     USAGE_MINIMUM (Button 1)            */
    0x29, 0x03,                    /*!<     USAGE_MAXIMUM (Button 3)            */
    0x15, 0x00,                    /*!<     LOGICAL_MINIMUM (0)                 */
    0x25, 0x01,                    /*!<     LOGICAL_MAXIMUM (1)                 */
    0x95, 0x03,                    /*!<     REPORT_COUNT (3)                    */
    0x75, 0x01,                    /*!<     REPORT_SIZE (1)                     */
    0x81, 0x02,                    /*!<     INPUT (Data,Var,Abs)                */
    0x95, 0x01,                    /*!<     REPORT_COUNT (1)                    */
    0x75, 0x05,                    /*!<     REPORT_SIZE (5)                     */
    0x81, 0x03,                    /*!<     INPUT (Cnst,Var,Abs)                */
    0x05, 0x01,                    /*!<     USAGE_PAGE (Generic Desktop)        */
    0x09, 0x30,                    /*!<     USAGE (X)                           */
    0x09, 0x31,                    /*!<     USAGE (Y)                           */
    0x09, 0x38,                    /*!<     USAGE (WHEEL)                       */
    0x15, 0x81,                    /*!<     LOGICAL_MINIMUM (-127)              */
    0x25, 0x7f,                    /*!<     LOGICAL_MAXIMUM (127)               */
    0x75, 0x08,                    /*!<     REPORT_SIZE (8)                     */
    0x95, 0x03,                    /*!<     REPORT_COUNT (3)                    */
    0x81, 0x06,                    /*!<     INPUT (Data,Var,Rel)                */
    0xc0,                          /*!<   END_COLLECTION                        */
    0xc0                           /*!< END_COLLECTION                        */
};


#define BULK_INT        0x00    /*!< Interrupt endpoint number */
#define ENDPOINTS       0x01    /*!< Number of endpoints required */

/*! List of required endpoint numbers
 */
static u8 mouse_if_index[] = { BULK_INT, };

/*! List of requested endpoints
 */
static struct usbd_endpoint_request mouse_if_endpoint_requests[ENDPOINTS+1] = {
        { BULK_INT, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT, 16, 64,  2, },
        { 0, },
};

#if !defined(OTG_C99)
/*! This is the HID desccriptor.
 */
static struct hid_descriptor mouse_if_hid;

/*! List of class descriptors 
 */
static struct usbd_generic_class_descriptor *mouse_if_hid_descriptors[] = { 
        (struct usbd_generic_class_descriptor *)&mouse_if_hid, };


/*! Interface Descriptions
 */
static struct usbd_alternate_description mouse_if_data_alternate_descriptions[1];

/*! List of Interface description(s)
 */
static struct usbd_interface_description mouse_interfaces[1];


#ifndef CONFIG_OTG_MOUSE_INTERVAL
#define CONFIG_OTG_MOUSE_INTERVAL 1
#endif /*CONFIG_OTG_MOUSE_INTERVAL*/

/*! mouse_if_global_init -
 */
void mouse_if_global_init(void)
{
        /*! This is the HID desccriptor.
         */
        ZERO(mouse_if_hid);
        mouse_if_hid.bLength = 0x09; 
        mouse_if_hid.bDescriptorType = 0x21; 
        mouse_if_hid.bcdHID = __constant_cpu_to_le16(0x110); 
        mouse_if_hid.bCountryCode = 0x00; 
        mouse_if_hid.bNumDescriptors = 0x01; 
        mouse_if_hid.bReportType = 0x22; 
        mouse_if_hid.wItemLength = __constant_cpu_to_le16(0x34);  // XXX magic length?

        /*! Interface Descriptions
         */
        ZERO(mouse_if_data_alternate_descriptions);
             
        mouse_if_data_alternate_descriptions[0].iInterface = "Random Mouse Interface - Interrupt";
        //mouse_if_data_alternate_descriptions[0].interface_descriptor =  &mouse_if_data_alternate_descriptor;
        mouse_if_data_alternate_descriptions[0].classes = 
                sizeof (mouse_if_hid_descriptors) / sizeof (struct usbd_generic_class_descriptor *);
        mouse_if_data_alternate_descriptions[0].class_list = mouse_if_hid_descriptors;
        mouse_if_data_alternate_descriptions[0].endpoints = 0x1;
        mouse_if_data_alternate_descriptions[0].endpoint_index = mouse_if_index;

        /*! List of Interface description(s)
         */
        ZERO(mouse_interfaces);
        mouse_interfaces[0].alternates = sizeof (mouse_if_data_alternate_descriptions) / sizeof (struct usbd_alternate_description);
        mouse_interfaces[0].alternate_list = mouse_if_data_alternate_descriptions;
}
#else /* defined(OTG_C99) */
/*! This is the HID desccriptor.
 */
struct hid_descriptor mouse_if_hid = {
        .bLength = 0x09, 
        .bDescriptorType = 0x21, 
        .bcdHID = __constant_cpu_to_le16(0x110), 
        .bCountryCode = 0x00, 
        .bNumDescriptors = 0x01, 
        .bReportType = 0x22, 
        .wItemLength = __constant_cpu_to_le16(0x34), 
};

/*! List of class descriptors 
 */
static struct usbd_generic_class_descriptor *mouse_if_hid_descriptors[] = { 
        (struct usbd_generic_class_descriptor *)&mouse_if_hid, };

/*! Interface Descriptions
 */
static struct usbd_alternate_description mouse_if_data_alternate_descriptions[] = {
        { 
                .iInterface = "Belcarra Random Mouse - HID",
                .bInterfaceClass = 0x03, 
                .bInterfaceSubClass = 0x01, 
                .bInterfaceProtocol = 0x02, 
                .classes = sizeof (mouse_if_hid_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
                .class_list =  mouse_if_hid_descriptors,
                .endpoints = sizeof (mouse_if_index) / sizeof(u8),
                .endpoint_index =  mouse_if_index,
        },
};

/*! List of Interface description(s)
 */
static struct usbd_interface_description mouse_interfaces[] = {
      { .alternates = sizeof (mouse_if_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = mouse_if_data_alternate_descriptions,},
};

#endif /* defined(OTG_C99) */

/*@}*/ 

/* MOUSE ***************************************************************************************** */
/* Transmit Function *************************************************************************** */

static int get_xy[8] = {
         0,  0,  1,  1, 
         0,  0, -1, -1, 
};

static int mouse_urb_sent (struct usbd_urb *urb, int rc);

/*!
 * mouse_if_send() - send a mouse data urb with random data
 * @param function
 * @return void
 */
void mouse_if_send(struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        int new_x = 0;
        int new_y = 0;
        u8 random;

        struct usbd_urb *tx_urb;

        tx_urb = usbd_alloc_urb (function, BULK_INT, 4, mouse_urb_sent);
        RETURN_UNLESS(tx_urb);

        memset(tx_urb->buffer, 0, 4);
        if (!mouse->n) {
                get_random_bytes(&random, 1);

                mouse->last_x = MAX(-4, MIN(4, mouse->last_x + get_xy[random & 0x7]));
                mouse->last_y = MAX(-4, MIN(4, mouse->last_y + get_xy[(random >> 3) & 0x7]));
                mouse->n = (random>>6) & 0x3;

                new_x = mouse->x + mouse->last_x;
                new_y = mouse->y + mouse->last_y;

                tx_urb->buffer[1] = mouse->last_x;
                mouse->x = new_x;
                tx_urb->buffer[2] = mouse->last_y;
                mouse->y = new_y;
        }
        else if (mouse->n & 0x1) {
                mouse->n--;
        }
        else {
                mouse->n--;
                tx_urb->buffer[1] = mouse->last_x;
                mouse->x = new_x;
                tx_urb->buffer[2] = mouse->last_y;
                mouse->y = new_y;
        }
        tx_urb->actual_length = 4;
        RETURN_UNLESS(usbd_start_in_urb(tx_urb));
        usbd_free_urb(tx_urb);
}

/*!
 * @brief mouse_urb_sent() - called to indicate URB transmit finished
 * This function is the callback function for sent urbs.
 *
 * Continues until either there is an error or the maximum
 * count is exceeded.
 *
 * @param tx_urb The urb to be sent.
 * @param rc result
 * @return int Return non-zero for failure.
 */
static int mouse_urb_sent (struct usbd_urb *tx_urb, int rc)
{
        struct usbd_function_instance *function = tx_urb->function_instance;
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;

        // return non-zero to get tx_urb deallocated
        RETURN_EINVAL_IF(usbd_get_device_status(function) == USBD_CLOSING);
        RETURN_EINVAL_IF(usbd_get_device_status(function) != USBD_OK);
        RETURN_EINVAL_IF(usbd_get_device_state(function) != STATE_CONFIGURED);

        TRACE_MSG2(MOUSE, "mouse_count[%d]: %d", interface_instance->wIndex, mouse->mouse_count);

        #ifdef CONFIG_OTG_MOUSE_PACKETS
        RETURN_EINVAL_IF (CONFIG_OTG_MOUSE_PACKETS && (mouse->mouse_count++ > CONFIG_OTG_MOUSE_PACKETS));
        #endif /* CONFIG_OTG_MOUSE_PACKETS */
        usbd_free_urb(tx_urb);
        mouse_if_send(function);                // re-start
        return 0;
}

/* USB Device Functions ************************************************************************ */


/*! copy_report()
 * This function copies the Mouse HID report into the provided URB.
 * @param urb Destination
 * @param data Source
 * @param size Amount of data to copy.
 * @param max_buf Size of buffer
 * @return Non-zero if error.
 *
 */
static int copy_report (struct usbd_urb *urb, void *data, int size, int max_buf)
{
        int available;
        int length;

        RETURN_EINVAL_IF (!urb);
        RETURN_EINVAL_IF (!data);
        RETURN_EINVAL_IF (!(length = size));
        RETURN_EINVAL_IF ((available = max_buf - urb->actual_length) <= 0);

        length = (length < available) ? length : available;
        memcpy (urb->buffer + urb->actual_length, data, length);
        urb->actual_length += length;
        return 0;
}

/*! 
 * mouse_if_send_hid() - send an EP0 urb containing HID report
 * This is called to send the Mouse HID report.
 */
static int mouse_if_send_hid (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        struct usbd_urb *urb = usbd_alloc_urb_ep0(function, mouse->wLength, NULL);
        int wMaxPacketSize = usbd_endpoint_zero_wMaxPacketSize(function, 0);

        TRACE_MSG1(MOUSE, "Send Hid wLength: %d", mouse->wLength);
        RETURN_EINVAL_IF (copy_report(urb, MouseHIDReport, sizeof(MouseHIDReport), mouse->wLength));
        RETURN_EINVAL_UNLESS (wMaxPacketSize);

        if (!(urb->actual_length % wMaxPacketSize) && (urb->actual_length < mouse->wLength)) 
                urb->flags |= USBD_URB_SENDZLP;

        RETURN_ZERO_IF(!usbd_start_in_urb(urb));
        usbd_free_urb(urb);
        return -EINVAL;
}

#ifdef CONFIG_OTG_MOUSE_BH
/*! 
 * @brief mouse_if_hid_bh() - Bottom half handler to send a HID report
 * @param data
 * @return none
 */
static void mouse_if_hid_bh (void *data)
{
        struct usbd_function_instance *function = (struct usbd_function_instance *)data;
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        unsigned long flags;
        mouse_if_send_hid(function);
        local_irq_save (flags);
        mouse_if_bh_active--;
        local_irq_restore (flags);
}

/*! mouse_schedule_bh - schedule a call for mouse_if_hid_bh
 * @param void
 */
static int mouse_schedule_bh (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        unsigned long flags;

        TRACE_MSG0(MOUSE, "Scheduling");
        PREPARE_WORK_ITEM(mouse->notification_bh, mouse_if_hid_bh, (void *)function);
        local_irq_save (flags);
        mouse_if_bh_active = 1;
        local_irq_restore (flags);
        SCHEDULE_WORK(mouse->notfication_bh);
        return 0;
}
#endif /* CONFIG_OTG_MOUSE_BH */

/*! 
 * @brief mouse_report_received() - called to indicate URB transmit finished
 * This function is the callback function for sent urbs.
 * It simply queues up another urb until the packets to be sent
 * configuration parameter is reached (or forever if zero.)
 * @param rx_urb The urb to be sent.
 * @param rc result
 * @return int Return non-zero for failure.
 */
int mouse_report_received (struct usbd_urb *rx_urb, int rc)
{
        struct usbd_function_instance *function;
        struct usbd_interface_instance *interface_instance;
        struct mouse_if_private *mouse;
        function = rx_urb->function_instance;
        interface_instance = (struct usbd_interface_instance *)function;
        mouse = interface_instance->function.privdata;
        TRACE_MSG3(MOUSE, "REPORT[%02x]: len: %d buf[0]: %x", 
                        mouse->pending_report, rx_urb->actual_length, rx_urb->buffer[0]);
        usbd_free_urb(rx_urb);
        mouse->pending_report = 0;
        return 0;
}


/*! 
 * mouse_if_device_request - called to process a request to endpoint or interface
 * @param function
 * @param request
 * @return non-zero for failure, will cause endpoint zero stall
 */
static int mouse_if_device_request (struct usbd_function_instance *function, struct usbd_device_request *request)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        struct usbd_urb *urb = NULL;

        TRACE_MSG6(MOUSE, "Interface[%02d] bmRequestType: %02x bRequest: %02x wValue: %04x wIndex: %04x wLength: %04x",
                        interface_instance->wIndex, request->bmRequestType, request->bRequest, request->wValue,
                        request->wIndex, request->wLength);

        /* HID only sends requests to interface.
         */
        RETURN_EINVAL_UNLESS(USB_REQ_RECIPIENT_INTERFACE == (request->bmRequestType & USB_REQ_RECIPIENT_MASK));

        switch (request->bmRequestType & USB_REQ_DIRECTION_MASK) {
        case USB_REQ_DEVICE2HOST:
                switch (request->bRequest) {
                case USB_REQ_GET_DESCRIPTOR:
                        switch (le16_to_cpu(request->wValue)>>8) {
                        case HID_REPORT:
                                mouse->wLength = request->wLength;
                                #ifdef CONFIG_OTG_MOUSE_BH
                                return mouse_schedule_bh(function);
                                #else
                                return mouse_if_send_hid(function);
                                #endif
                        }
                        return -EINVAL;
                case USB_REQ_GET_REPORT:
                case USB_REQ_GET_IDLE:
                case USB_REQ_GET_PROTOCOL:
                        break;
                default:
                        return -EINVAL;
                }

                RETURN_EINVAL_UNLESS(request->wLength && 
                                ((urb = usbd_alloc_urb_ep0 (function, request->wLength, NULL))));

                switch (request->bRequest) {

                case USB_REQ_GET_REPORT:

                        switch (le16_to_cpu(request->wValue) >> 8) {
                        case HID_INPUT:
                        case HID_OUTPUT:
                        case HID_FEATURE:
                                break;
                        }
                        // XXX create urb and send?
                        urb->actual_length = 1;
                        break;

                case USB_REQ_GET_IDLE:
                case USB_REQ_GET_PROTOCOL:
                        urb->actual_length = 1;
                        switch (request->bRequest) {
                        case USB_REQ_GET_IDLE:
                                urb->buffer[0] = mouse->idle;
                                break;
                        case USB_REQ_GET_PROTOCOL:
                                urb->buffer[0] = mouse->protocol;
                                break;
                        }
                }

                RETURN_ZERO_UNLESS(usbd_start_in_urb(urb));

                /* only get here if error */
                usbd_free_urb(urb);
                TRACE_MSG0(MOUSE, "get failed");
                break;

        case USB_REQ_HOST2DEVICE:
                switch (request->bRequest) {
                case USB_REQ_SET_REPORT:
                        // Sample: 21 09 00 02 00 00 01 00
                        mouse->pending_report = request->wValue;
                        RETURN_EINVAL_UNLESS(request->wLength &&
                                        (urb = usbd_alloc_urb_ep0(function, request->wLength, mouse_report_received)));

                        RETURN_ZERO_UNLESS (usbd_start_out_urb (urb));
                        /* failure */
                        usbd_free_urb (urb);
                        mouse->pending_report = 0;
                        return -EINVAL;

                case USB_REQ_SET_IDLE:
                        mouse->idle = request->wValue;
                        TRACE_MSG1(MOUSE, "SET IDLE: %x", request->wValue);
                        return 0;

                case USB_REQ_SET_PROTOCOL:
                        mouse->protocol = request->wValue;
                        TRACE_MSG1(MOUSE, "SET PROTOCOL: %x", request->wValue);
                        return 0;
                }
                break;
        }
        return -EINVAL;
}

/*! 
 * @brief mouse_if_set_configuration - called to indicate urb has been received
 * @param function
 * @param configuration
 * @return int
 */
static int mouse_if_set_configuration (struct usbd_function_instance *function, int configuration)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;


        TRACE_MSG3(MOUSE, "MOUSE_IF[%d] %x cfg: %d ", interface_instance->wIndex, function, configuration);

        // XXX Need to differentiate between non-zero, zero and non-zero done twice

        function->privdata = mouse;;

        mouse->n = mouse->x = mouse->y = mouse->last_x = mouse->last_y = 0;

        mouse->bEndpointAddress = usbd_endpoint_bEndpointAddress(function, BULK_INT, 0);
        mouse->writesize = usbd_endpoint_wMaxPacketSize(function, BULK_INT, 0);

        mouse->mouse_count = 0;

        TRACE_MSG4(MOUSE, "MOUSE_IF[%2d] Configured: %d bEndpointAddress: %02x size: %02x", 
                        interface_instance->wIndex, configuration, mouse->bEndpointAddress, mouse->writesize);

        mouse_if_send(function);                                           // start sending

        return 0;
}


/*!
 8 @brief mouse_if_endpoint_cleared - called by the USB Device Core when endpoint cleared
 * @param function The function instance for this driver
 * @param bEndpointAddress
 * @return none
 */
static void mouse_if_endpoint_cleared (struct usbd_function_instance *function, int bEndpointAddress)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        TRACE_MSG1(MOUSE,"CLEARED bEndpointAddress: %02x", bEndpointAddress);
        mouse_if_send(function);                                           // re-start sending
}



/* ********************************************************************************************* */
/*! mouse_if_function_enable - called by USB Device Core to enable the driver
 * @param function The function instance for this driver to use.
 * @return non-zero if error.
 */
static int mouse_if_function_enable (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = NULL;

        RETURN_EINVAL_UNLESS((mouse = CKMALLOC(sizeof(struct mouse_if_private), GFP_KERNEL)));
        // XXX MODULE LOCK HERE
        interface_instance->function.privdata = (void *)mouse;
        mouse->writesize = usbd_endpoint_wMaxPacketSize(function, BULK_INT, 0);
        return 0;
}

/*! mouse_if_function_disable - called by the USB Device Core to disable the driver
 * @param function The function instance for this driver
 */
static void mouse_if_function_disable (struct usbd_function_instance *function)
{               
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct mouse_if_private *mouse = interface_instance->function.privdata;
        interface_instance->function.privdata = NULL;
        LKFREE(mouse);
        // XXX MODULE UNLOCK HERE
}

/* ********************************************************************************************* */
#if !defined(OTG_C99)
/*! function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations mouse_function_ops;

/*! mouse_interface_driver - USB Device Core function driver definition
 */
struct usbd_interface_driver mouse_interface_driver;

void mouse_if_ops_init(void)
{
        /*! function_ops - operations table for the USB Device Core
         */

        ZERO(mouse_function_ops);
        mouse_function_ops.set_configuration = mouse_if_set_configuration;
        mouse_function_ops.device_request = mouse_if_device_request;          /*! called for each received device request */
        mouse_function_ops.endpoint_cleared = mouse_if_endpoint_cleared;

        mouse_function_ops.function_enable = mouse_if_function_enable;
        mouse_function_ops.function_disable = mouse_if_function_disable;

        /*! interface_driver - USB Device Core function driver definition
         */
        ZERO(mouse_interface_driver);
        mouse_interface_driver.driver.name = "mouse-random-if";                            /*! driver name */
        mouse_interface_driver.driver.fops = &mouse_function_ops;                             /*! operations table */

        mouse_interface_driver.interfaces = sizeof (mouse_interfaces) / sizeof (struct usbd_interface_description);
        mouse_interface_driver.interface_list = mouse_interfaces;
        mouse_interface_driver.endpointsRequested = ENDPOINTS;
        mouse_interface_driver.requestedEndpoints = mouse_if_endpoint_requests;
   
}

#else /* defined(OTG_C99) */
/*! function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations mouse_function_ops = {
        .set_configuration = mouse_if_set_configuration,
        .device_request = mouse_if_device_request,           /*!< called for each received device request */
        .endpoint_cleared = mouse_if_endpoint_cleared,
        .function_enable = mouse_if_function_enable,
        .function_disable = mouse_if_function_disable,
};

/*! mouse_interface_driver - USB Device Core function driver definition
 */
struct usbd_interface_driver mouse_interface_driver = {
        .driver.name = "mouse-random-if",                            /*! driver name */
        .driver.fops = &mouse_function_ops,                             /*!< operations table */
        .interfaces = sizeof (mouse_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = mouse_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = mouse_if_endpoint_requests,

        .bFunctionClass = 0x03,
        .bFunctionSubClass = 0x01,
        .bFunctionProtocol = 0x02,
        .iFunction = "Belcarra Random Mouse - HID",
};
#endif /* defined(OTG_C99) */


/*! Module Parameters ******************************************************** */

/*! @name XXXXX MODULE Parameters
 */
/*@{*/

MOD_AUTHOR ("sl@belcarra.com"); 
EMBED_LICENSE();
MOD_DESCRIPTION ("Belcarra Random Walk MOUSE Function");

MOD_PARM_INT (vendor_id, "Device Vendor ID", 0);
MOD_PARM_INT (product_id, "Device Product ID", 0);

otg_tag_t MOUSE;

/*@}*/

/* USB Module init/exit ***************************************************** */

/*! 
 * mouse_if_modinit() - module init
 *
 * This is called by the Linux kernel; either when the module is loaded
 * if compiled as a module, or during the system intialization if the 
 * driver is linked into the kernel.
 *
 * This function will parse module parameters if required and then register
 * the mouse driver with the USB Device software.
 *
 * If the CONFIG_OTG_MOUSE_BH option is enabled it will also setup the mouse
 * bottom half handler.
 *
 */
static int mouse_if_modinit (void)
{
        int i;
        #if !defined(OTG_C99)
        mouse_if_global_init();
        mouse_if_ops_init();
        #endif /* defined(OTG_C99) */
        
        MOUSE = otg_trace_obtain_tag();
        TRACE_MSG2(MOUSE, "vendor_id: %04x product_id: %04x", MODPARM(vendor_id), MODPARM(product_id));

        mouse_if_hid.wItemLength = cpu_to_le16(0x34);      // XXX mips compiler bug.....

        // register as usb function driver
	TRACE_MSG0(MOUSE, "REGISTER INTERFACE");
        TRACE_MSG2(MOUSE, "%s %d", mouse_interface_driver.driver.name, mouse_interface_driver.endpointsRequested);

        THROW_IF (usbd_register_interface_function ( &mouse_interface_driver, "mouse-if", NULL), error);

        TRACE_MSG2(MOUSE, "%s %d", mouse_interface_driver.driver.name, mouse_interface_driver.endpointsRequested);

	TRACE_MSG0(MOUSE, "REGISTER FINISHED");
        #ifdef CONFIG_OTG_MOUSE_BH
        mouse_if_private.notification_bh.routine = mouse_if_hid_bh;
        mouse_if_private.notification_bh.data = NULL;
        #endif
        CATCH(error) {
                otg_trace_invalidate_tag(MOUSE);
                return -EINVAL;
        }
        return 0;
}

module_init (mouse_if_modinit);

#if OTG_EPILOGUE
/*! 
 * mouse_if_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded 
 * if compiled as a module. This function is never called if the 
 * driver is linked into the kernel.
 *
 * @param void
 * @return void
 */
static void mouse_if_modexit (void)
{
        int i;
        #ifdef CONFIG_OTG_MOUSE_BH
        while (mouse_if_bh_active) {
                printk(KERN_ERR"%s: waiting for bh\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }
        #endif
        usbd_deregister_interface_function (&mouse_interface_driver);
        otg_trace_invalidate_tag(MOUSE);
}

module_exit (mouse_if_modexit);
#endif




