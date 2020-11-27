/*
 * otg/functions/acm/acm.h
 * @(#) sl@belcarra.com|otg/functions/acm/acm.h|20060403224909|43346
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2006, 2007 Motorola, Inc. 
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 04/25/2006         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language 
 * 12/11/2006         Motorola         Changes for Open src compliance.
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
 * @defgroup ACMFunction ACM
 * @ingroup functiongroup
 */

/*!
 * @file otg/functions/acm/acm.h
 * @brief ACM Function Driver private defines
 *
 * This is an ACM Function Driver. The upper edge is exposed
 * to the hosting OS as a Posix type character device. The lower
 * edge implements the USB Device Stack API.
 *
 * This driver implements the CDC ACM driver model and uses the CDC ACM
 * protocols.
 *
 * Note that it appears to be impossible to determine the end of a receive
 * bulk transfer larger than wMaxPacketsize. The host is free to send
 * wMaxPacketsize chars in a single transfer. This means that we cannot
 * queue receive urbs larger than wMaxPacketsize (typically 64 bytes.)
 *
 * This does not however prevent queuing transmit urbs with larger amounts
 * of data. It is interpreted at the receiving (host) end as a series of
 * wMaxPacketsize transfers but because there is no interpretation to the
 * amounts of data sent it does affect anything if we treat the data as a
 * single larger transfer.
 *
 * @ingroup ACMFunction
 */


// Endpoint indexes in acm_endpoint_requests[] and the endpoint map.
#define INT_IN          0x00
#define BULK_IN         0x01
#define BULK_OUT        0x02
#define ENDPOINTS       0x03

#define COMM_INTF       0x00
#define DATA_INTF       0x01

/*! @name ACM Flags
 * @{
 */
#define ACM_OPENED      (1 << 0)        /*!< upper layer has opened the device port */
#define ACM_CONFIGURED  (1 << 1)        /*!< device is configured */
#define ACM_THROTTLED   (1 << 2)        /*!< upper layer doesn't want to receive data  */
#define ACM_LOOPBACK    (1 << 4)        /*!< upper layer wants local loopback */
#define ACM_LOCAL       (1 << 5)        /*!< upper layer specified LOCAL mode */

#define ACM_CLOSING     (1 << 6)        /*!< upper layer has asked us to close */

#define MAX_ACM_OUT_URBS       8        /*!< maximum OUT urbs ACM allocates */
#define MAX_ACM_IN_URBS        10       /*!< maximum IN urbs ACM allocates */
/*! @} */

/*! @name tty type
 * @{
 */
typedef enum tty_type {
        tty_if,
        dun_if,
        obex_if,
        atcom_if,
} tty_type_t;


/*! acm_os_ops
 * This structure lists the operations implemented in the upper os layer.
 */
struct acm_os_ops {
        /*! enable
         * Enable the function driver.
         */
        int (*enable)(struct usbd_function_instance *);

        /*! disable
         * Disable the function driver.
         */
        void (*disable)(struct usbd_function_instance *);

        /*! wakeup_opens
         * Wakeup processes waiting for DTR.
         */
        void (*wakeup_opens)(struct usbd_function_instance *);

        /*! wakeup_opens
         * Wakeup processes waiting for state change.
         */
        void (*wakeup_state)(struct usbd_function_instance *);

        /*! wakeup writers
         * Wakeup pending writes.
         */
        void (*schedule_wakeup_writers)(struct usbd_function_instance *);

        /*! recv_space_available
         * Check for amount of receive space that is available, controls
         * amount of receive urbs that will be queued.
         */
        int (*recv_space_available)(struct usbd_function_instance *);

        /*! recv_chars
         * Process chars received on specified interface.
         */
        int (*recv_chars)(struct usbd_function_instance *, u8 *cp, int n);


        /*! schedule_hangup
         * Schedule a work item that will perform a hangup.
         */
        void (*schedule_hangup)(struct usbd_function_instance *);

        /*! line_coding
         * Tell function that line coding has changed.
         */
        void (*line_coding)(struct usbd_function_instance *);

        /*! send_break
         * Tell function to send a break signal.
         */
        void (*send_break)(struct usbd_function_instance *);

        /*! schedule_recv_start_bh
         * Tell function to send a break signal.
         */
        void (*recv_start_bh)(struct usbd_function_instance *);

        /*! tiocgi count
         * Tell function to update line state interrupt count.
         */
        void (*tiocgicount)(struct usbd_function_instance *);


};


/*! struct acm_private
 */
struct acm_private {
        struct usbd_function_instance *function_instance;
        otg_tag_t trace_tag;

        tty_type_t tty_type;

        int index;
        int minors;

        otg_atomic_t flags;
        otg_atomic_t recv_urbs;
        otg_atomic_t queued_bytes;
        otg_atomic_t queued_urbs;
        otg_atomic_t wakeup_writers;

        BOOL hs;
        int writesize;
        int readsize;
        int low_th;
        int high_th;

        struct cdc_acm_line_coding line_coding; // C.f. Table 50 - [GS]ET_LINE_CODING Device Request (to/from host)
        cdc_acm_bmUARTState bmUARTState;        // C.f. Table 51 - SET_CONTROL_LINE_STATE Device Request (from host)
        cdc_acm_bmLineState bmLineState;        // C.f. Table 69 - SERIAL_STATE Notification (to host)

        WORK_STRUCT out_work;
        wait_queue_head_t recv_wait;            /*! wait queue for blocking open*/

        struct acm_os_ops *ops;
        struct urb_link out_queue_head;
        int out_queue_buf_offset;
        struct usbd_urb *out_queue_urb;
        struct usbd_urb *in_urb;
};


#define TTY acm_trace_tag
extern otg_tag_t acm_trace_tag;

/*! open - device open
 * This is called the device is opened (first open only if non-exclusive opens allowed).
 */
int acm_open (struct usbd_function_instance *);

/*! close - Device close
 * This is called when the device closed (last close only if non-exclusive opens allowed.)
 */
int acm_close (struct usbd_function_instance *);


/*! flush - flush data urbs.
 * Cancel outstanding data urbs.
 */
void acm_flush (struct usbd_function_instance *);

/*! schedule_recv - queue as many data receive urbs as possible
 * This will schedule a bottom half hander that will will start as
 * many receive data urbs as are allowed given the amount of room
 * available in the upper layer. If no urbs are queued by the
 * bottom half handler it will re-schedule itself.
 */
void acm_schedule_recv_restart (struct usbd_function_instance *);

/*! throttle - set throttle flag for specified interface
 * Receive urbs will not be queued when throttled.
 */
void acm_throttle (struct usbd_function_instance *);

/*! unthrottle - reset throttle flag for specified interface
 * Receive urbs are allowed to be queued. If no urbs are queued a
 * bottom half handler will be scheduled to queue them.
 */
void acm_unthrottle (struct usbd_function_instance *);


/*! try_xmit_chars - try to send data 
 * This will start a transmit urb to send the specified data. The
 * number of characters queued/ sent will be returned.
 */
int acm_try_xmit_chars(struct usbd_function_instance *function_instance, int count, const unsigned char *buf);

/*! force xmit_chars - send data via specified interface
 * This will transmit an urb to the bus layer.
 */
void acm_force_xmit_chars(struct usbd_function_instance *function_instance);


/*! write_room
 * Return amount of data that could be queued for sending.
 */
int acm_write_room (struct usbd_function_instance *);

/*! chars_in_buffer
 * Return number of chars in xmit buffer.
 */
int acm_chars_in_buffer (struct usbd_function_instance *);


/*! send_int_notification - send notification via interrupt endpoint
 *  This can be used to queue network, serial state change notifications.
 */
int acm_send_int_notification (struct usbd_function_instance *, int bnotification, int data);

/*! wait_task - wait for task to complete.
*/
void acm_wait_task (struct usbd_function_instance *, WORK_ITEM *queue);

/*! ready - return true if connected and carrier
*/
int acm_ready (struct usbd_function_instance *);

/*! acm_get_d1_rts
 * Get DSR status
 */
int acm_get_d1_rts (struct usbd_function_instance *);

/*! acm_get_d0_dtr
 * Get DTR status.
 */
int acm_get_d0_dtr (struct usbd_function_instance *);


/*! acm_get_bRxCarrier
 * Get bRxCarrier (DCD) status
 */
int acm_get_bRxCarrier(struct usbd_function_instance *function_instance);

/*! acm_set_bRxCarrier
 * set bRxCarrier (DCD) status
 */
void acm_set_bRxCarrier (struct usbd_function_instance *, int);

/*! acm_get_bTxCarrier
 * Get bTxCarrier (DSR) status
 */
int acm_get_bTxCarrier (struct usbd_function_instance *);

/*! acm_set_bTxCarrier
 * Set bTxCarrier (DSR) status
 */
void acm_set_bTxCarrier(struct usbd_function_instance *function_instance, int value);

/*! acm_bRingSignal
 * Indicate Ring signal to host.
 */
void acm_bRingSignal (struct usbd_function_instance *);

/*! acm_bBreak
 * Indicate Break signal to host.
 */
void acm_bBreak (struct usbd_function_instance *);

/*! acm_bOverrun
 * Indicate Overrun signal to host.
 */
void acm_bOverrun (struct usbd_function_instance *);

/*! acm_set_throttle
 * Set LOOPBACK status
 */
void acm_set_throttle (struct usbd_function_instance *, int value);

/*! acm_get_throttled
 * Set LOOPBACK status
 */
int acm_get_throttled(struct usbd_function_instance *function_instance);

/*! set_local
 * Set LOCAL status
 */
void acm_set_local (struct usbd_function_instance *, int value);

/*! set_loopback - set loopback mode
 * Sets LOOP flag, data received from the host will be immediately
 * returned without passing to the upper layer.
 */
void acm_set_loopback (struct usbd_function_instance *, int value);

/*! acm_start_recv_urbs
 */
int acm_start_recv_urbs(struct usbd_function_instance *);

/*! acm_line_coding_urb_received - callback for sent URB
 */
int acm_line_coding_urb_received (struct usbd_urb *urb, int urb_rc);

/*! acm_urb_sent_ep0 - called to indicate ep0 URB transmit finished
 */
int acm_urb_sent_ep0 (struct usbd_urb *urb, int rc);

/*! acm_push_queued_urbs
 */
void acm_push_queued_urbs(struct acm_private *acm);

/*! acm_free_out_queued_urbs
 */
void acm_free_out_queued_urbs(struct acm_private *acm);

/*! acm_reuse_last_urb
*/
inline static void acm_reuse_last_urb(struct usbd_urb *urb, struct acm_private *acm);

/*! acm_recv_urb
 */
int acm_recv_urb (struct usbd_urb *urb, int rc);

/*! acm_urb_sent_int - called to indicate int URB transmit finished
 */
int acm_urb_sent_int (struct usbd_urb *urb, int rc);

/*! acm_function_enable - called by USB Device Core to enable the driver
 * @param function The function instance for this driver to use.
 * @return non-zero if error.
 */
int acm_function_enable (struct usbd_function_instance *function_instance, tty_type_t tty_type,
                struct acm_os_ops *os_ops, int max_urbs, int max_bytes);

/*! acm_function_disable - called by the USB Device Core to disable the driver
 * @param function The function instance for this driver
 */
void acm_function_disable (struct usbd_function_instance *function);

/*! acm_set_configuration - called to indicate urb has been received
 * @param function
 * @param configuration
 * @param interface_index
 */
int acm_set_configuration (struct usbd_function_instance *function_instance, int configuration);

/*! acm_set_interface - called to indicate urb has been received
 * @param function
 * @param configuration
 * @param interface_index
 */
int acm_set_interface (struct usbd_function_instance *function, int wIndex, int altsetting);

/*! acm_reset - called to indicate urb has been received
 * @param function
 * @param configuration
 * @param interface_index
 */
int acm_reset (struct usbd_function_instance *function);

/*! acm_suspended - called to indicate urb has been received
 * @param function
 * @param configuration
 * @param interface_index
 */
int acm_suspended (struct usbd_function_instance *function);

/*! acm_resumed - called to indicate urb has been received
 * @param function
 * @param configuration
 * @param interface_index
 */
int acm_resumed (struct usbd_function_instance *function);


/*! acm_device_request - called to process a request to endpoint or interface
 * @param function
 * @param request
 * @return non-zero if error
 */
int acm_device_request (struct usbd_function_instance *function, struct usbd_device_request *request);


/*! acm_endpoint_cleared - called by the USB Device Core when endpoint cleared
 * @param function The function instance for this driver
 */
void acm_endpoint_cleared (struct usbd_function_instance *function_instance, int bEndpointAddress);

/*! acm_send_queued - called by the USB Device Core when endpoint cleared
 * @param function The function instance for this driver
 */
int acm_send_queued (struct usbd_function_instance *function_instance);


/*
 * otg-trace tag.
 */
extern otg_tag_t acm_fd_trace_tag;
