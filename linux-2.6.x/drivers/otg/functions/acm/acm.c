/*
 * otg/functions/acm/acm.c
 * @(#) balden@belcarra.com|otg/functions/acm/acm.c|20060407231422|08277
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2006,2007 Motorola, Inc. 
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 07/25/2006         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language 
 * 12/11/2006         Motorola         Changes for Open src compliance. 
 * 03/12/2007         Motorola         Changes for np deref in acm_send_queued 
 * 05/15/2007         Motorola         Don't schedule hotplug in reset if there is one scheduled.
 * 06/05/2007         Motorola         Changes to support HSDPA
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
 * @file otg/functions/acm/acm.c
 * @brief ACM Function Driver private defines
 *
 * This is the ACM Protocol specific portion of the
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
 *    Descriptors     | tty-if   |
 *    Registration    +----------+
 *
 *
 *    Function        +----------+
 *    I/O             | acm      |      <-----
 *    Implementation  +----------+
 *
 *
 *    Module          +----------+
 *    Loading         | acm-l26  |
 *                    +----------+
 *
 * @ingroup ACMFunction
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
#include <linux/smp_lock.h>
#include <linux/wait.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>
#include <otg/usbp-mcpc.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#include <otg/usbp-bus.h>
#include "acm.h"
#include <public/otg-node.h>
#ifdef CONFIG_OTG_ACM_HOTPLUG
#include <otg/hotplug.h>
#endif

otg_tag_t acm_trace_tag;


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * @brief acm_urb_sent_ep0 - called to indicate ep0 URB transmit finished
 * @param urb pointer to struct usbd_urb
 * @param urb_rc result
 * @return non-zero for error
 */
int acm_urb_sent_ep0 (struct usbd_urb *urb, int urb_rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG4(TTY,"URB SENT acm: %x urb: %x urb_rc: %d length: %d", acm, urb, urb_rc, urb->actual_length);
        #endif
        RETURN_EINVAL_UNLESS(acm);

        urb->function_privdata = NULL;
        usbd_free_urb(urb);
        return 0;
}

/*! acm_line_coding_urb_received - handles line coding urb
 *
 * Handles the line coding urb and updates the value.
 *
 * @param urb
 * @param urb_rc
 * @return non-zero for failure.
 */
int acm_line_coding_urb_received (struct usbd_urb *urb, int urb_rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG5(TTY,"URB RECEIVED acm: %x urb: %x urb_rc: %d length: %d %d", acm, urb, urb_rc,
                        urb->actual_length, sizeof(struct cdc_acm_line_coding));
        #endif
        RETURN_EINVAL_UNLESS(acm);
        RETURN_EINVAL_IF (USBD_URB_OK != urb_rc);
        RETURN_EINVAL_IF (urb->actual_length < sizeof(struct cdc_acm_line_coding));

        memcpy(&acm->line_coding, urb->buffer, sizeof(struct cdc_acm_line_coding));

        // XXX notify application if baudrate has changed
        // we need an os layer function to call here.... it should decode line_coding
        // and if possible notify posix layer that things have changed.

        usbd_free_urb(urb);

        if (acm->ops->line_coding)
                acm->ops->line_coding(function_instance);

        return 0;
}


/*!
 * @brief  acm_set_link_urb_received - callback for sent URB
 * @param urb
 * @param urb_rc result
 * @return non-zero for error
 */
int acm_set_link_urb_received (struct usbd_urb *urb, int urb_rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG4(TTY,"URB RECEIVED acm: %x urb: %x urb_rc: %d length: %d", acm, urb, urb_rc, urb->actual_length);
        #endif
        usbd_free_urb(urb);
        return 0;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*!
 * acm_start_out_urb - called to queue out urb on ep0 to receive device request OUT data
 * @param function
 * @param len
 * @param callback
 * @return non-zero if error
 */
int acm_start_out_urb_ep0(struct usbd_function_instance *function, int len, int (*callback) (struct usbd_urb *, int))
{
        struct acm_private *acm = function->privdata;
        struct usbd_urb *urb;
        RETURN_ZERO_UNLESS(len);
        RETURN_ENOMEM_UNLESS((urb = usbd_alloc_urb_ep0(function, len, callback)));
        urb->function_privdata = acm;
        urb->function_instance = function;
        RETURN_ZERO_UNLESS (usbd_start_out_urb(urb));

        usbd_free_urb(urb);          
        return -EINVAL;
}

/*!
 * acm_device_request - called to process a request to endpoint or interface
 * @param function
 * @param request
 * @return non-zero if error
 */
int acm_device_request (struct usbd_function_instance *function, struct usbd_device_request *request)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct acm_private *acm = interface_instance->function.privdata;

        TRACE_SETUP(TTY, request);

        // determine the request direction and process accordingly
        switch (request->bmRequestType & (USB_REQ_DIRECTION_MASK | USB_REQ_TYPE_MASK)) {

        case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_CLASS:
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG1(TTY, "H2D CLASS bRequest: %02x", request->bRequest);
                #endif
                switch (request->bRequest) {
                        /* CDC */
                case CDC_CLASS_REQUEST_SEND_ENCAPSULATED: break;
                case CDC_CLASS_REQUEST_SET_COMM_FEATURE: break;
                case CDC_CLASS_REQUEST_CLEAR_COMM_FEATURE: break;

                case CDC_CLASS_REQUEST_SET_LINE_CODING:                 /* 0x20 */
                           return acm_start_out_urb_ep0(function, le16_to_cpu(request->wLength), acm_line_coding_urb_received);

                case CDC_CLASS_REQUEST_SET_CONTROL_LINE_STATE: {        /* 0x22 */
                        unsigned int prev_bmLineState = acm->bmLineState;
                        acm->bmLineState = le16_to_cpu(request->wValue);

                        // schedule writers or hangup IFF open
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG5(TTY, "acm: %x interface: %x set control state, bmLineState: %04x previous: %04x changed: %04x",
                                        acm, interface_instance,
                                        acm->bmLineState, prev_bmLineState, acm->bmLineState ^ prev_bmLineState);
                        #endif
                        /* make sure there really is a state change in D0 */
                        if ((acm->bmLineState ^ prev_bmLineState) & CDC_LINESTATE_D0_DTR) {

                                #ifdef CONFIG_OTG_TRACE
                                TRACE_MSG1(TTY, "DTR state changed -> %x",
                                                (acm->bmLineState & CDC_LINESTATE_D0_DTR));
                                #endif
                                /* update line state interrupts */
                                acm->ops->tiocgicount(function); 

                                if (acm->bmLineState & CDC_LINESTATE_D0_DTR) {
                                        acm->ops->recv_start_bh(function);
                                        /* wake up blocked opens */
                                        acm->ops->wakeup_opens(function);

                                } else {
                                        if (atomic_and_read(&acm->flags, ACM_OPENED)) {
                                                acm->ops->schedule_hangup(function);
                                        }
                                        acm_flush(function);
                                }

                                /* wake up blocked ioctls */
                                acm->ops->wakeup_state(function);
                        }

                        /* make sure there really is a state change in D1 */
                        if ((acm->bmLineState ^ prev_bmLineState) & CDC_LINESTATE_D1_RTS) {

                                #ifdef CONFIG_OTG_TRACE
                                TRACE_MSG1(TTY, "DCD state changed -> %x",
                                                (acm->bmLineState & CDC_LINESTATE_D1_RTS));
                                #endif
                                /* update line state interrupts */
                                acm->ops->tiocgicount(function); 

                                /* wake up the writers if any blocked for RTS */
                                if (atomic_and_read(&acm->flags, ACM_OPENED)) {
                                        atomic_set(&acm->wakeup_writers,0);
                                        acm->ops->schedule_wakeup_writers(function);
                                }
                                /* wake up blocked ioctls */
                                acm->ops->wakeup_state(function);
                        }

                        /* send notification if we have DCD */
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "bmUARTState: %04x sending (DCD|DSR) notification", acm->bmUARTState);
                        #endif
                        acm_send_int_notification(function, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
                        break;
                }

                case CDC_CLASS_REQUEST_SEND_BREAK: break;

                default:
                        return -EINVAL;
                }
                return 0;

        case USB_REQ_DEVICE2HOST | USB_REQ_TYPE_CLASS:
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG1(TTY, "D2H CLASS bRequest: %02x", request->bRequest);
                #endif
                switch (request->bRequest) {
                                                /* CDC */
                case CDC_CLASS_REQUEST_GET_ENCAPSULATED: break;
                case CDC_CLASS_REQUEST_GET_COMM_FEATURE: break;
                case CDC_CLASS_REQUEST_GET_LINE_CODING: {
                        struct usbd_urb *urb;

                        // XXX should check wLength
                        RETURN_ENOMEM_IF (!(urb = usbd_alloc_urb_ep0(function, sizeof(struct cdc_acm_line_coding),
                                                        acm_urb_sent_ep0)));
                        urb->function_instance = function;
                        urb->function_privdata = acm;

                        memcpy(urb->buffer, &acm->line_coding, sizeof(struct cdc_acm_line_coding));

                        urb->actual_length = sizeof(struct cdc_acm_line_coding);

                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "sending line coding urb: %p",(u32)(void*)urb);
                        #endif
                        RETURN_ZERO_UNLESS(usbd_start_in_urb(urb));
                        usbd_free_urb(urb);
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG0(TTY, "(send failed)--> -EINVAL");
                        #endif
                        return -EINVAL;
                }
                default:
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "UNKNOWN D2H CLASS bRequest: %02x", request->bRequest);
                        #endif
                        return -EINVAL;
                }
                return 0;

                                                   /* MCPC */
        case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_VENDOR:
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG1(TTY, "H2D Vendor bRequest: %02x", request->bRequest);
                #endif
                switch (request->bRequest) {
                                                   /* MCPC */
                case MCPC_REQ_ACTIVATE_MODE:
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "H2D Vendor bRequest: %02x MCPC_REQ_ACTIVATE_MODE", request->bRequest);
                        #endif
                        return 0;

                case MCPC_REQ_SET_LINK:
                        return acm_start_out_urb_ep0(function, le16_to_cpu(request->wLength), acm_set_link_urb_received);

                case MCPC_REQ_CLEAR_LINK:
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "H2D Vendor bRequest: %02x MCPC_REQ_CLEAR_LINK", request->bRequest);
                        #endif
                        return 0;

                case MCPC_REQ_MODE_SPECIFIC_REQUEST:
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "H2D Vendor bRequest: %02x MCPC_REQ_SPECIFIC_REQUEST", request->bRequest);
                        #endif
                        return 0;
                default:
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "UNKNOWN H2D CLASS bRequest: %02x XXXX", request->bRequest);
                        #endif
                        return -EINVAL;
                }
                break;

        case USB_REQ_DEVICE2HOST | USB_REQ_TYPE_VENDOR:
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG1(TTY, "D2H Vendor bRequest: %02x", request->bRequest);
                #endif
                switch (request->bRequest) {
                case MCPC_REQ_GET_MODETABLE:
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "D2H Vendor bRequest: %02x MCPC_REQ_GET_MODETABLE", request->bRequest);
                        #endif
                        return 0;
                case MCPC_REQ_MODE_SPECIFIC_REQUEST:
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "D2H Vendor bRequest: %02x MCPC_REQ_SPECIFIC_REQUEST", request->bRequest);
                        #endif
                        return 0;
                default:
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "UNKNOWN D2H Vendor bRequest: %02x", request->bRequest);
                        #endif
                        return -EINVAL;
                }
                break;

        default:
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG2(TTY, "unknown bmRequestType: %02x bRequest: %02x", request->bmRequestType, request->bRequest);
                #endif
                return -EINVAL;
        }
        return 0;
}

/*! @brief acm_endpoint_cleared - called by the USB Device Core when endpoint cleared
 * @param function_instance  The function instance for this driver
 * @param bEndpointAddress
 * @return none
 */
void acm_endpoint_cleared (struct usbd_function_instance *function_instance, int bEndpointAddress)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);

        TRACE_MSG1(TTY, "CLEARED bEndpointAddress: %02x", bEndpointAddress);
        #endif
}


/* ******************************************************************************************* */
/*! @brief  acm_open() - called with int disabled
 * @param function_instance
 * @return int
 */
int acm_open(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif

        RETURN_EINVAL_UNLESS(acm);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY,"OPEN");
        #endif

        atomic_or_set(&acm->flags, ACM_OPENED);
        /* set DSR */
        acm_set_bTxCarrier(function_instance, 1);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"bmUARTState: %04x", acm->bmUARTState);
        #endif
        return(acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState));
}

/*! acm_flush()
 * @param function_instance
 * @return number of urbs in queue
 */
void acm_flush(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        RETURN_UNLESS(acm);

        acm->line_coding.dwDTERate = cpu_to_le32(0x1c200); // 115200
        acm->line_coding.bDataBits = 0x08;

        RETURN_IF(usbd_get_device_status(function_instance) == USBD_CLOSING);
        RETURN_IF(usbd_get_device_status(function_instance) != USBD_OK);
        RETURN_IF(usbd_get_device_state(function_instance) != STATE_CONFIGURED);
        usbd_flush_endpoint_index(function_instance, BULK_IN);
        usbd_flush_endpoint_index(function_instance, BULK_OUT);
}

/*! @brief acm_close()
 * @param function_instance
 * @return number of urbs in queue
 */
int acm_close(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_EINVAL_UNLESS(acm);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"queued: %d", acm->queued_urbs);
        #endif

        atomic_and_set(&acm->flags, ~ACM_OPENED);
        /* set flag and return if there is more data to be send */
        if (atomic_read(&acm->queued_urbs)) {
                atomic_or_set(&acm->flags, ACM_CLOSING);
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG1(TTY, "setting ACM_CLOSING flags: %04x", atomic_and_read(&acm->flags,1));
                #endif
                return 0;
        }
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY, "normal close flags: %04x", atomic_and_read(&acm->flags,1));
        #endif

        /* Normal Close - no data pending
         */
        acm_flush(function_instance);
        /* flush the urbs from the out urb queue */
        acm_free_out_queued_urbs(acm);
        acm->bmUARTState = 0;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"bmUARTState: %04x", acm->bmUARTState);
        #endif
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
        return 0;
}


/*! acm_ready()
 * @param function_instance
 */
int acm_ready(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_ZERO_UNLESS(acm);
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG6(TTY,"FLAGS: %08x CONFIGURED: %x OPENED: %x LOCAL: %x d0_dtr: %x d1_dcd: %x",
                        atomic_and_read(&acm->flags,1),
                        BOOLEAN(atomic_and_read(&acm->flags, ACM_CONFIGURED)),
                        BOOLEAN(atomic_and_read(&acm->flags, ACM_OPENED)),
                        BOOLEAN(atomic_and_read(&acm->flags, ACM_LOCAL)),
                        acm->bmLineState & CDC_LINESTATE_D0_DTR,
                        acm->bmLineState & CDC_LINESTATE_D1_RTS);
        #endif
 
        return((atomic_and_read(&acm->flags, ACM_CONFIGURED)) &&
                 (atomic_and_read(&acm->flags, ACM_OPENED)) &&
                 (atomic_and_read(&acm->flags, ACM_LOCAL)) ? 1 : (acm->bmLineState & CDC_LINESTATE_D1_RTS));

}

/*! acm_start_recv_urbs()
 * @param function_instance
 * @return number of urbs in queue
 */
int acm_start_recv_urbs(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int num_in_queue = 0;

        RETURN_EINVAL_UNLESS(acm);
        RETURN_ZERO_IF(atomic_and_read(&acm->flags, ACM_THROTTLED));

        UNLESS (acm_ready(function_instance)) {
                TRACE_MSG0(TTY, "START RECV: NOT READY");
                return -1;
        }

        /* if configured, opened, not throttled and we have carrier or local then
         * we can queue recv urbs.
         */
        while (atomic_read(&acm->recv_urbs)  < MAX_ACM_OUT_URBS) {
                struct usbd_urb *urb;

                BREAK_IF(!(urb = usbd_alloc_urb((struct usbd_function_instance *) function_instance,
                                                BULK_OUT, acm->readsize, acm_recv_urb)));
                atomic_inc(&acm->recv_urbs);
                urb->function_privdata = acm;
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG3(TTY, "START RECV: %d urb: %p privdata: %p", atomic_read(&acm->recv_urbs), urb,acm);
                #endif
                CONTINUE_UNLESS (usbd_start_out_urb(urb));
                atomic_dec(&acm->recv_urbs);
                TRACE_MSG1(TTY, "FAILED START RECV: %d", atomic_read(&acm->recv_urbs));
                usbd_free_urb(urb);
                break;
        }

        /* There needs to be at least one recv urb queued in order
         * to keep driving the push to the OS layer, so return how
         * many there are in case the OS layer must try again later.
         */
        num_in_queue = atomic_read(&acm->recv_urbs);
        return num_in_queue;
}

/*! acm_reuse_last_urb() - this function sends 'urb' back to the core. 
 * @param usbd_urb
 * @param acm_private
 * @return void
 */
inline static void acm_reuse_last_urb(struct usbd_urb *urb, struct acm_private *acm)
{
        if((atomic_read(&acm->recv_urbs) <= MAX_ACM_OUT_URBS) && atomic_and_read(&acm->flags, ACM_CONFIGURED)) {
                #ifdef CONFIG_OTG_TRACE
                atomic_inc(&acm->recv_urbs);
               	TRACE_MSG1(TTY, "REUSE URB: allocated : %d urbs", atomic_read(&acm->recv_urbs));
                #endif
               	if (usbd_start_out_urb(urb)) {
               	        atomic_dec(&acm->recv_urbs);
                        #ifdef CONFIG_OTG_TRACE
               	        TRACE_MSG1(TTY, "FAILED REUSING THE URB: %d", atomic_read(&acm->recv_urbs));
                        #endif
               	        usbd_free_urb(urb);
                }
        } else { // Free this URB since MAX URB allocation is already done
                #ifdef CONFIG_OTG_TRACE
               	TRACE_MSG1(TTY, "NO REUSE: already allocated %d urbs", atomic_read(&acm->recv_urbs));
                #endif
                atomic_dec(&acm->recv_urbs);
               	usbd_free_urb(urb);
        }
}


/* Transmit INTERRUPT ************************************************************************** */

/*!
 *@brief acm_send_int_notfication()
 *
 * Generates a response urb on the notification (INTERRUPT) endpoint.
 * CALLED from interrupt context.
 * @param function_instance
 * @param bnotification
 * @param data
 * @return non-zero if error
 */
int acm_send_int_notification(struct usbd_function_instance *function_instance, int bnotification, int data)
{
        struct usbd_urb *urb = NULL;
        struct cdc_notification_descriptor *notification;
        int rc = 0;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        RETURN_EINVAL_UNLESS(acm && atomic_and_read(&acm->flags, ACM_CONFIGURED));

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        do {
                BREAK_IF(!(urb = usbd_alloc_urb((struct usbd_function_instance *)function_instance,
                                                INT_IN,
                                                sizeof(struct cdc_notification_descriptor),
                                                acm_urb_sent_int)));

                urb->function_privdata = acm;
                urb->function_instance = function_instance;

                memset(urb->buffer, 0, urb->buffer_length);
                urb->actual_length = sizeof(struct cdc_notification_descriptor);

                /* fill in notification structure */
                notification = (struct cdc_notification_descriptor *) urb->buffer;

                notification->bmRequestType = USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
                notification->bNotification = bnotification;

                switch (bnotification) {
                case CDC_NOTIFICATION_NETWORK_CONNECTION:
                        notification->wValue = data;
                        break;
                case CDC_NOTIFICATION_SERIAL_STATE:
                        notification->wLength = cpu_to_le16(2);
                        *((unsigned short *)notification->data) = cpu_to_le16(data);
                        break;
                }

                BREAK_IF(!(rc = usbd_start_in_urb (urb)));

                urb->function_privdata = NULL;
                usbd_free_urb (urb);

                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG1(TTY,"urb: %x", urb);
                #endif
        } while(0);

        return rc;
}


/*! acm_closing
 */
int acm_closing(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        unsigned long flags;


        RETURN_ZERO_UNLESS(atomic_and_read(&acm->flags, ACM_CLOSING));

        atomic_and_set(&acm->flags, ~ACM_CLOSING);
        /* No more data to send - complete close
         */
        acm_flush(function_instance);
        /* free the queued urbs */ 
        acm_free_out_queued_urbs(acm); 

        acm->bmUARTState = 0;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"bmUARTState: %04x", acm->bmUARTState);
        #endif
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);

        return 0;
}

/*! acm_urb_sent_zle - callback for completed BULK ZLE URB
 *
 * Handles notification that an ZLE urb has been sent (successfully or otherwise).
 *
 * @param urb     Pointer to the urb that has been sent.
 * @param urb_rc  Result code from the send operation.
 *
 * @return non-zero for failure.
 */
int acm_urb_sent_zle (struct usbd_urb *urb, int urb_rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        unsigned long flags;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG4(TTY,"urb: %p urb_rc: %d length: %d queue: %d",
                        urb, urb_rc, urb->actual_length, acm->queued_urbs);
        #endif
        usbd_free_urb (urb);
        local_irq_save(flags);
        acm_closing(function_instance);
        local_irq_restore(flags);
        return 0;
}

/*! acm_urb_sent_bulk - called to indicate bulk URB transmit finished
 * @param urb pointer to struct usbd_urb
 * @param rc result
 * @return non-zero for error
 */
static int acm_urb_sent_bulk (struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, BULK_IN, usbd_high_speed(function_instance));
        unsigned long flags;

        RETURN_ZERO_UNLESS(acm);

        #if CONFIG_OTG_TRACE
        TRACE_MSG5(TTY,"acm: %x urb: %x length: %d flags: %x queued_bytes: %d",
                        acm, urb, urb->actual_length,
                        acm ? atomic_and_read(&acm->flags,1) : 0,
                        acm ? atomic_read(&acm->queued_bytes) : 0
                        );
        #endif


        atomic_sub(urb->actual_length, &acm->queued_bytes);
        atomic_dec(&acm->queued_urbs);

        #if CONFIG_OTG_TRACE
        TRACE_MSG3(TTY, "queued_bytes: %d queude_urbs: %d opened: %d",
                        atomic_read(&acm->queued_bytes), atomic_read(&acm->queued_urbs), BOOLEAN(atomic_and_read(&acm->flags, ACM_OPENED)));
        #endif
        urb->function_privdata = NULL;
        usbd_free_urb (urb);

        local_irq_save(flags);
        /* wakeup upper layers to send more data */
        if (atomic_read(&acm->wakeup_writers)) {
                acm->ops->schedule_wakeup_writers(function_instance);
                atomic_set(&acm->wakeup_writers,0);
        }

        /* Force an xmit if queued < low_th */
        if(atomic_read(&acm->queued_urbs) < acm->low_th) {
                acm_force_xmit_chars(function_instance);
        }
        local_irq_restore(flags);
        /* check if there are still pending send urbs */
        RETURN_ZERO_IF(atomic_read(&acm->queued_urbs));

        /* check if we should send a zlp, send ZLP */
        UNLESS (urb->actual_length % in_pkt_sz) {
                if ((urb = usbd_alloc_urb (function_instance, BULK_IN, 0, acm_urb_sent_zle ))) {
                        urb->actual_length = 0;
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG1(TTY, "Sending ZLP: length: %d", urb->actual_length);
                        #endif
                        urb->flags |= USBD_URB_SENDZLP;

                        if ((rc = usbd_start_in_urb (urb))) {
                                TRACE_MSG1(TTY,"FAILED: %d", rc);
                                urb->function_privdata = NULL;
                                usbd_free_urb (urb);
                        }
                }
                return 0;
        }
        local_irq_save(flags);
        acm_closing(function_instance);
        local_irq_restore(flags);
        return 0;
}

/*! acm_urb_sent_int - called to indicate int URB transmit finished
 * @param urb pointer to struct usbd_urb
 * @param rc result
 * @return non-zero for error
 */
int acm_urb_sent_int (struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif

        RETURN_ZERO_UNLESS(acm);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"urb: %x, INT length=%d",urb, urb->actual_length);
        #endif

        urb->function_privdata = NULL;
        usbd_free_urb(urb);
        return 0;
}

/* USB Device Functions ************************************************************************ */


/*!
 * @brief  acm_write_room
 * @param function_instance
 * @return non-zero if error
 */
int acm_write_room(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"acm: %x queued_bytes: %d", acm, acm ? atomic_read(&acm->queued_bytes) : 0);
        #endif

        RETURN_ZERO_UNLESS(acm);
        
        if(atomic_read(&acm->queued_urbs) < acm->high_th) {
                return ((acm->high_th - atomic_read(&acm->queued_urbs))* acm->writesize - (acm->in_urb? acm->in_urb->actual_length : 0));
        }
        return 0;
}

/*! @brief acm_chars_in_buffer
 * @param function_instance
 * @return non-zero if error
 */
int acm_chars_in_buffer(struct usbd_function_instance *function_instance)
{
        int rc;
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"acm: %x queued_bytes: %d", (int)acm, acm ? atomic_read(&acm->queued_bytes) : 0);
        #endif

        RETURN_ZERO_UNLESS(acm);

        return atomic_read(&acm->queued_bytes) + (acm->in_urb? acm->in_urb->actual_length: 0);
}

/*! @brief acm_try_xmit_chars - xmit data if necessary
 * Called with interrupts disabled
 * @param function_instance
 * @param count number of bytes to send
 * @buf buffer to copy the data from
 * @return number of bytes send
 */

int acm_try_xmit_chars(struct usbd_function_instance *function_instance, int count,
                   const unsigned char *buf)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int total_bytes_sent=0, data_to_send=count;
        int urb_size = acm->writesize;
        RETURN_ZERO_UNLESS(acm && buf);
        RETURN_ZERO_UNLESS(atomic_and_read(&acm->flags, ACM_CONFIGURED) &&
                        (atomic_and_read(&acm->flags, ACM_LOCAL) ? 1 : (acm->bmLineState & CDC_LINESTATE_D1_RTS)));
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY,"connect OK");
        #endif
        do {
            /* Max URB reached */
            if(unlikely(atomic_read(&acm->queued_urbs) >= acm->high_th)) {
                    TRACE_MSG1(TTY,"Max urbs reached: queued urbs = %d", atomic_read(&acm->queued_urbs));
                    break;
            } else {
                TRACE_MSG0(TTY,"allowed to use urbs - filling it up");
            	/* No outstanding allocated URB */
            	if(!acm->in_urb) {
                    data_to_send=MIN(data_to_send, acm->writesize);
                    /*if low_th, create only an urb of needed size */
                    urb_size = (atomic_read(&acm->queued_urbs) < acm->low_th) ? data_to_send : acm->writesize;

                    RETURN_ZERO_UNLESS ((acm->in_urb = usbd_alloc_urb ((struct usbd_function_instance *) function_instance,
                                        BULK_IN, urb_size, acm_urb_sent_bulk)));
            	}else { 
                    data_to_send=MIN(data_to_send,(acm->in_urb->alloc_length - acm->in_urb->actual_length));
            	}  
            	memcpy ((void *)(acm->in_urb->buffer + acm->in_urb->actual_length), (void *)(buf+total_bytes_sent), data_to_send);
            	acm->in_urb->actual_length += data_to_send;
            	total_bytes_sent += data_to_send;

            	if(acm->in_urb->actual_length == acm->in_urb->alloc_length) {
                    /* this is always supposed to succeed - in normal case 
                     * might fail in cable detach, mode switch etc */
                    acm_force_xmit_chars(function_instance);
            	}

            	data_to_send=count-total_bytes_sent;
            }
        } while(data_to_send);

        return total_bytes_sent;
}

/*! @brief acm_force_xmit_chars - force data left in in_urb
 * Called with interrupts disabled
 * @param function_instance
 */

void acm_force_xmit_chars(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_UNLESS(acm);
        /* nothing to send */
        if(acm->in_urb == NULL) {
                return;
        }
        acm->in_urb->function_privdata = acm;
        #if 0 /* This is already printed in tty-l26 */
        {
                u8 *cp = urb->buffer;
                int len;
                for (len = 0; len < count; len += 8, cp += 8) {
                        TRACE_MSG8(TTY, "SEND [%02x %02x %02x %02x %02x %02x %02x %02x]",
                                  cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
                }
        }
        #endif
        if(!(usbd_start_in_urb (acm->in_urb))) {
                atomic_add(acm->in_urb->actual_length, &acm->queued_bytes);
                atomic_inc(&acm->queued_urbs);
                acm->in_urb=NULL;
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG1(TTY,"start_in_urb success %d", atomic_read(&acm->queued_urbs));
                #endif
        } else { 
                TRACE_MSG0(TTY,"start_in_failed");
                usbd_free_urb (acm->in_urb);
                acm->in_urb=NULL;
        }
}


/*! acm_push_queued_urbs - push the queued urbs in to LDISC as space allows.
 * @param urb
 * @param rc
 * @return non-zero if error
 */
void acm_push_queued_urbs(struct acm_private *acm)
{
        int data_to_write=0;
        int wrote_to_ldisc=0;

        if(!acm) {
                TRACE_MSG0(TTY,"acm is null!!!");
        }
        while (true) {
                if(acm->out_queue_urb == NULL) { /* No URB pending */
                        if(!(acm->out_queue_urb = usbd_first_urb_detached(&(acm->out_queue_head)))) {
                               TRACE_MSG0(TTY,"No URB in ACM QUEUE");
                               return ;
                        }
                        #ifdef CONFIG_OTG_TRACE
              	        TRACE_MSG1(TTY,"URB retrieved from Queue: recv_urbs queued = %d",atomic_read(&acm->recv_urbs));
                        #endif
                        acm->out_queue_buf_offset=0;
                }

                data_to_write =  acm->out_queue_urb->actual_length - acm->out_queue_buf_offset;

                /* recv_chars returns the number of bytes written to ldisc */  
                wrote_to_ldisc = acm->ops->recv_chars(
                                 acm->out_queue_urb->function_instance, 
                                 acm->out_queue_urb->buffer+acm->out_queue_buf_offset, 
                                 data_to_write);  
                if(wrote_to_ldisc <= 0) {
                        // LD THORTTLED or NO TTY
                        TRACE_MSG0(TTY,"LD THROTTLED!! or NO TTY");
                        return ;
                }
                acm->out_queue_buf_offset += wrote_to_ldisc;
                /* If the urb is read completely */
                if(acm->out_queue_buf_offset == acm->out_queue_urb->actual_length) {	
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG0(TTY,"BUF NUM equals Actual Length");
                        #endif
                        /* Re use the last urb; send it back to the core */
                        acm_reuse_last_urb(acm->out_queue_urb, acm);
                        acm->out_queue_urb = NULL;
                }
        }
}

/*! acm_free_out_queued_urbs - free the urbs queued to be send to LDISC
 * @param urb
 * @param rc
 * @return non-zero if error
 */
void acm_free_out_queued_urbs(struct acm_private *acm) 
{
         #ifdef CONFIG_OTG_TRACE
         TRACE_MSG0(TTY, "freeing the URBs from queue");
         #endif
         /* if there is pending URB, free it first */
         if(acm->out_queue_urb) {
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG1(TTY, "freeing the current URB - %d urbs left", atomic_read(&acm->recv_urbs));
                #endif
                atomic_dec(&acm->recv_urbs);
                usbd_free_urb(acm->out_queue_urb);
         }        
         while((acm->out_queue_urb = usbd_first_urb_detached(&(acm->out_queue_head)))) {
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG1(TTY, "freeing URBS %d urbs", atomic_read(&acm->recv_urbs));
                #endif
                atomic_dec(&acm->recv_urbs);
                usbd_free_urb(acm->out_queue_urb);
         } 
}

/*! acm_recv_urb - call back from core when URB is received from HOST
 * @param urb
 * @param rc
 * @return non-zero if error
 */
int acm_recv_urb (struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct acm_private *acm = function_instance->privdata;
        unsigned long flags;
        /* Return 0 if urb has been accepted,
         * return 1 and expect caller to deal with urb release otherwise.
         */

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY, "acm: %x len: %d", acm, urb->actual_length);
        #endif

        if (USBD_URB_CANCELLED == rc) {
                atomic_dec(&acm->recv_urbs);              
                TRACE_MSG1(TTY,"cancelled URB=%p",urb);
                return -EINVAL;
        }
        if (USBD_URB_OK != rc) {
                atomic_dec(&acm->recv_urbs);              
                TRACE_MSG2(TTY,"rejected URB=%p rc=%d",urb,rc);
                acm_reuse_last_urb(urb, acm);
                return 0;
        }

        /* loopback mode
         */
        if (unlikely(atomic_and_read(&acm->flags, ACM_LOOPBACK))) {
                local_irq_save(flags);
                acm_try_xmit_chars(function_instance, urb->actual_length,  urb->buffer);
                local_irq_restore(flags);
                acm_reuse_last_urb(urb, acm);
                
        /**
         * Append the URB to the ACM queue and then try to push it to the LDISC
         */
        } else {
                urb_append(&(acm->out_queue_head), urb);
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG0(TTY,"appended URB");
                atomic_dec(&acm->recv_urbs);
                #endif
                acm_push_queued_urbs(acm);
        }
        return 0;
}

/* ********************************************************************************************* */
/*!
 * @brief acm_get_d0_dtr
 * Get DTR status.
 * @param function_instance
 * @return int
 */
int acm_get_d0_dtr(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_ZERO_UNLESS(acm);
  
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"bmLineState: %04x d0_dtr: %x", acm->bmLineState, (acm->bmLineState & CDC_LINESTATE_D0_DTR) ? 1 : 0);
        #endif
        return (acm->bmLineState & CDC_LINESTATE_D0_DTR) ? 1 : 0;
}

/*!
 * @brief acm_get_d1_rts
 * Get DSR status
 * @param function_instance
 * @return int
 */
int acm_get_d1_rts(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_ZERO_UNLESS(acm);
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"bmLineState: %04x d1_rts: %x", acm->bmLineState, (acm->bmLineState & CDC_LINESTATE_D1_RTS) ? 1 : 0);
        #endif
        return (acm->bmLineState & CDC_LINESTATE_D1_RTS) ? 1 : 0;
}


/* ********************************************************************************************* */
/*!
 * @brief acm_get_bRxCarrier
 * Get bRxCarrier (DCD) status
 * @param function_instance
 @ @return int
 */
int acm_get_bRxCarrier(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_ZERO_UNLESS(acm);
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"bmLineState: %04x bRxCarrier: %x",
                        acm->bmLineState, (acm->bmUARTState & CDC_UARTSTATE_BRXCARRIER_DCD) ? 1 : 0);
        #endif
        return (acm->bmUARTState & CDC_UARTSTATE_BRXCARRIER_DCD) ? 1 : 0;
}

/*!
 * @brief acm_get_bTxCarrier
 * Get bTxCarrier (DSR) status
 * @param function_instance
 @ @return int
 */
int acm_get_bTxCarrier(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        RETURN_ZERO_UNLESS(acm);
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"bmLineState: %04x bTxCarrier: %x",
                        acm->bmLineState, (acm->bmUARTState & CDC_UARTSTATE_BTXCARRIER_DSR) ? 1 : 0);
        #endif
        return (acm->bmUARTState & CDC_UARTSTATE_BTXCARRIER_DSR) ? 1 : 0;
}


/*! @brief acm_set_bRxCarrier
 * Get bRxCarrier (DCD) status
 * @param function_instance
 * @param value
 * @return none
 */
void acm_set_bRxCarrier(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"acm: %x value: %x", (int)acm, value);
        #endif
        RETURN_UNLESS(acm);
        acm->bmUARTState &= ~CDC_UARTSTATE_BRXCARRIER_DCD;
        acm->bmUARTState |= value ? CDC_UARTSTATE_BRXCARRIER_DCD : 0;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
}

/*! @brief acm_set_bTxCarrier
 * Set bTxCarrier (DSR) status
 * @param function_instance
 * @param value
 * @return none
 */
void acm_set_bTxCarrier(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"acm: %x value: %x", (int)acm, value);
        #endif
        RETURN_UNLESS(acm);
        acm->bmUARTState &= ~CDC_UARTSTATE_BTXCARRIER_DSR;
        acm->bmUARTState |= value ? CDC_UARTSTATE_BTXCARRIER_DSR : 0;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
}


/* ********************************************************************************************* */

/*! acm_bRingSignal
 * Indicate Ring signal to host.
 */
void acm_bRingSignal(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        RETURN_UNLESS(acm);
        acm->bmUARTState |= CDC_UARTSTATE_BRINGSIGNAL;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
        acm->bmUARTState &= ~CDC_UARTSTATE_BRINGSIGNAL;
}

/*! acm_bBreak
 * Indicate Break signal to host.
 */
void acm_bBreak(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        RETURN_UNLESS(acm);
        acm->bmUARTState |= CDC_UARTSTATE_BBREAK;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
        acm->bmUARTState &= ~CDC_UARTSTATE_BBREAK;
}

/*! acm_bOverrun
 * Indicate Overrun signal to host.
 */
void acm_bOverrun(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        RETURN_UNLESS(acm);
        acm->bmUARTState |= CDC_UARTSTATE_BOVERRUN;
        acm_send_int_notification(function_instance, CDC_NOTIFICATION_SERIAL_STATE, acm->bmUARTState);
        acm->bmUARTState &= ~CDC_UARTSTATE_BOVERRUN;
}

/*! acm_set_local
 * Set LOCAL status
 * @param function_instance
 * @param value
 */
void acm_set_local(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"acm: %x value: %x", acm, value);
        #endif

        RETURN_UNLESS(acm);
        if(value) {
                atomic_or_set(&acm->flags, ACM_LOCAL);
        } else {
                atomic_and_set(&acm->flags, ~ACM_LOCAL);
        }
        acm_start_recv_urbs(function_instance);
}

/*! @ brief acm_set_loopback
 * Set LOOPBACK status
 * @param function_instance
 * @param value
 */
void acm_set_loopback(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"acm: %x value: %d", (int)acm, value);
        #endif
        RETURN_UNLESS(acm);
        if(value) {
                atomic_or_set(&acm->flags, ACM_LOOPBACK);
        } else {
                atomic_and_set(&acm->flags, ~ACM_LOOPBACK);
        }
        acm_start_recv_urbs(function_instance);
}

#ifdef CONFIG_OTG_ACM_HOTPLUG
struct usb_hotplug_private gen_priv_acm;
#endif

/*! @ brief acm_set_throttle
 * Set LOOPBACK status
 * @param function_instance
 * @param value
 */
void acm_set_throttle(struct usbd_function_instance *function_instance, int value)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"acm: %x value: %d", (int)acm, value);
        #endif
        RETURN_UNLESS(acm);
        if(value) {
                atomic_or_set(&acm->flags, ACM_THROTTLED);
        } else {
                atomic_and_set(&acm->flags, ~ACM_THROTTLED);
        }
        acm_start_recv_urbs(function_instance);
}

/*! @ brief acm_get_throttled
 * Set LOOPBACK status
 * @param function_instance
 * @param value
 */
int acm_get_throttled(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
 
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        RETURN_ZERO_UNLESS(acm);

        return atomic_and_read(&acm->flags, ACM_THROTTLED) ? 1 : 0;
}


/*! @brief acm_function_enable - called by USB Device Core to enable the driver
 * @param function_instance The function instance for this driver to use.
 * @param tty_type
 * @param os_ops
 * @param max_urbs
 * @param max_bytes
 * @return non-zero if error.
 */
int acm_function_enable (struct usbd_function_instance *function_instance, tty_type_t tty_type,
                struct acm_os_ops *os_ops, int max_urbs, int max_bytes)
{
        struct acm_private *acm = NULL;

        RETURN_EINVAL_UNLESS((acm = CKMALLOC(sizeof(struct acm_private), GFP_KERNEL)));

        /* Initialize the queue */	
        acm->out_queue_head.prev = acm->out_queue_head.next = &acm->out_queue_head;
        acm->out_queue_buf_offset = 0;
        acm->out_queue_urb = NULL;
        acm->in_urb=NULL;

	
        function_instance->privdata = acm;

        acm->tty_type = tty_type;
        acm->ops = os_ops;
        acm->minors = 1;
        acm->low_th=2;
        acm->high_th=max_urbs;
        acm->line_coding.dwDTERate = __constant_cpu_to_le32(0x1c200);
        acm->line_coding.bDataBits = 0x08;
        /* Prepare the work item to enable ACM to queue urbs */
        PREPARE_WORK_ITEM(acm->out_work, acm_push_queued_urbs, acm);
        init_waitqueue_head(&acm->recv_wait);

#ifdef CONFIG_OTG_ACM_HOTPLUG
        gen_priv_acm.function_instance = function_instance;
        gen_priv_acm.dev_name = ACM_AGENT;
        hotplug_init(&gen_priv_acm);
#endif

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY, "ENABLED writesize: %d", acm->writesize);
        #endif

        return acm->ops->enable(function_instance);
}

/*! @brief acm_function_disable - called by the USB Device Core to disable the driver
 * @param function The function instance for this driver
 */
void acm_function_disable (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct acm_private *acm = function ? function->privdata : NULL;


        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY, "DISABLED");
        #endif
        acm->ops->disable(function);
        function->privdata = NULL;
#ifdef CONFIG_OTG_ACM_HOTPLUG
        while (PENDING_WORK_ITEM(gen_priv_acm.hotplug_bh)) {
                printk("Waiting for ACM hp!\n");
                TRACE_MSG0(TTY,"Waiting for ACM hp");
                schedule_timeout(10 * HZ);
        }
#endif
        while(PENDING_WORK_ITEM(acm->out_work)) {
                printk("Waiting for ACM queue!\n");
                TRACE_MSG0(TTY,"Waiting for ACM queue");
                schedule_timeout(10 * HZ);
        }

        /* free the queued urbs */ 
        acm_free_out_queued_urbs(acm); 
        if(acm->in_urb) usbd_free_urb(acm->in_urb);
        LKFREE(acm);
}

/*!
 * @brief acm_set_configuration - called to indicate urb has been received
 * @param function_instance
 * @param configuration
 * @return int
 */
int acm_set_configuration (struct usbd_function_instance *function_instance, int configuration)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;


        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG4(TTY, "CONFIGURED acm: %x wIndex:%d %x cfg: %d ", acm,
                        interface_instance->wIndex, function_instance, configuration);
        #endif

        // XXX Need to differentiate between non-zero, zero and non-zero done twice

        /* speead and size cannot be set until now as we don't know waht speed we are
         * enumerated at
         */
        acm->hs = usbd_high_speed((struct usbd_function_instance *) function_instance);
        acm->readsize = usbd_endpoint_transferSize(function_instance, BULK_OUT, acm->hs);
        acm->writesize = usbd_endpoint_transferSize(function_instance, BULK_IN, acm->hs);
        atomic_or_set(&acm->flags, ACM_CONFIGURED);


        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY, "configured readsize: %d writesize: %d", acm->readsize, acm->writesize);
        #endif

        /* scheduled to start receiving data */ 
        acm->ops->recv_start_bh(function_instance);
        atomic_set(&acm->wakeup_writers,0);
#ifdef CONFIG_OTG_ACM_HOTPLUG
	    gen_priv_acm.flags 	= HOTPLUG_FUNC_ENABLED;
        generic_os_hotplug(&gen_priv_acm);
#endif

        return 0;
}

/*!
 * acm_set_interface - called to indicate urb has been received
 * @param function_instance
 * @param wIndex
 * @param altsetting
 * @return int
 */
int acm_set_interface (struct usbd_function_instance *function_instance, int wIndex, int altsetting)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY, "SET INTERFACE");
        #endif
        return 0;
}

/*!
 * @brief acm_reset - called to indicate bus reset
 * @param function_instance
 * @return int
 */
int acm_reset (struct usbd_function_instance *function_instance)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        unsigned long flags;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY, "RESET");
        #endif
        local_irq_save(flags);

        if(!atomic_and_read(&acm->flags, ACM_CONFIGURED))
        {
           TRACE_MSG0(TTY,"ACM not yet configured: Ignore this reset");
           local_irq_restore(flags);
           return 0;

        }
        
        /* if configured and open then schedule hangup
         */
	if (atomic_and_read(&acm->flags, ACM_OPENED) && atomic_and_read(&acm->flags, ACM_CONFIGURED)) { 
                acm->ops->schedule_hangup(function_instance);
        }

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY, "RESET continue");
        #endif
#ifdef CONFIG_OTG_ACM_HOTPLUG
        gen_priv_acm.flags = ~HOTPLUG_FUNC_ENABLED;
        generic_os_hotplug(&gen_priv_acm);
#endif  
        // XXX flush
        // Release any queued urbs
        atomic_and_set(&acm->flags, ~ACM_CONFIGURED);
        local_irq_restore(flags);
        return 0;
}

/*!
 * @brief acm_suspended - called to indicate urb has been received
 * @param function_instance
 * @return int
 */
int acm_suspended (struct usbd_function_instance *function_instance)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        TRACE_MSG0(TTY, "SUSPENDED");
        return 0;
}

/*!
 * acm_resumed - called to indicate urb has been received
 * @param function_instance
 * @return int
 */
int acm_resumed (struct usbd_function_instance *function_instance)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct acm_private *acm = function_instance->privdata;
        TRACE_MSG0(TTY, "RESUMED");
        return 0;
}

/*! acm_send_queued - called by the USB Device Core when endpoint cleared
 * @param function_instance The function instance for this driver
 * @return int
 */
int acm_send_queued (struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance->privdata;

        if (acm == NULL)
        {
            TRACE_MSG0(TTY, "acm_send_queued: acm is NULL");
            return 0;
        }
        return atomic_read(&acm->queued_urbs);
}

