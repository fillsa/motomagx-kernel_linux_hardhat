/*
 * otg/otg/usbp-pcd.h - OTG Peripheral Controller Driver
 * @(#) balden@seth2.belcarratech.com|otg/otg/usbp-pcd.h|20051116205002|24811
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
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
 * @file otg/otg/usbp-pcd.h
 * @brief Peripheral Controller Driver Related Structures and Definitions.
 *
 * @ingroup USBP
 */

/*!
 * The pcd_ops structure contains pointers to all of the functions implemented for the
 * linked in driver. Having them in this structure allows us to easily determine what
 * functions are available without resorting to ugly compile time macros or ifdefs
 *
 * There is only one instance of this, defined in the device specific lower layer.
 */
struct usbd_pcd_ops {

        /* mandatory */
        u8 bmAttributes;
        int max_endpoints;
        int ep0_packetsize;
        u8 high_speed_capable;
        u32 capabilities;                               /* UDC Capabilities - see usbd-bus.h for details */
        u8 bMaxPower;
        char *name;
        u8 ports;


        /* 3. called by ot_init() if defined */
        int (* serial_init) (struct pcd_instance *);    /* get device serial number if available */

        /* 4. called from usbd_enable_function_irq() - REQUIRED */
        int (*request_endpoints)                        /* process endpoint_request list and return endpoint_map */
                (struct pcd_instance *,
                 struct usbd_endpoint_map *,            /* showing allocated endpoints with attributes, addresses */
                 int, struct usbd_endpoint_request*);   /* and other associated information */

        /* 5. called from usbd_enable_function_irq() - REQUIRED */
        int (*set_endpoints)                            /* setup required endpoints in hardware */
                (struct pcd_instance *, int , struct usbd_endpoint_map *);      

        /* 6. called by xxx() if defined */
        void (*enable) (struct pcd_instance *);         /* enable the UDC */

        /* 7. called by xxx() if defined */
        void (*disable) (struct pcd_instance *);        /* disable the UDC */

        /* 8. called by xxx() if defined */
        void (*start) (struct pcd_instance *);          /* called for DEVICE_CREATE to start the UDC */
        void (*stop) (struct pcd_instance *);           /* called for DEVICE_DESTROY to stop the UDC */
        void (*disable_ep) 
                (struct pcd_instance *, 
                 unsigned int ep);                      /* called for DEVICE_DESTROTY to disable endpoint zero */

        /* 9. called by xxx() if defined */
        void (*set_address) 
                (struct pcd_instance *,
                unsigned char address);                 /* called for DEVICE_RESET and DEVICE_ADDRESS_ASSIGNED to */


        /* 10. called by xxx() if defined */
        void (*setup_ep) 
                (struct pcd_instance *,
                unsigned int ep,                        /* called for DEVICE_CONFIGURED to setup specified endpoint for use */
                struct usbd_endpoint_instance *endpoint); 

        /* 11. called by xxx() if defined */
        void (*reset_ep) 
                (struct pcd_instance *,
                unsigned int ep);                       /* called for DEVICE_RESET to reset endpoint zero */


        /* 12. called by usbd_send_urb() - REQUIRED */
        void (*start_endpoint_in)                       /* start an IN urb */
                (struct pcd_instance *,
                struct usbd_endpoint_instance *);       

        /* 13. called by usbd_start_recv() - REQUIRED */
        void (*start_endpoint_out)                      /* start an OUT urb */
                (struct pcd_instance *,
                struct usbd_endpoint_instance *);

        /* 14. called by usbd_cancel_urb_irq() */
        void (*cancel_in_irq) 
                (struct pcd_instance *,
                struct usbd_urb *urb);                  /* cancel active urb for IN endpoint */
        void (*cancel_out_irq) 
                (struct pcd_instance *,
                struct usbd_urb *urb);                  /* cancel active urb for OUT endpoint */


        /* 15. called from ep0_recv_setup() for GET_STATUS request */
        int (*endpoint_halted) 
                (struct pcd_instance *, 
                 struct usbd_endpoint_instance *);      /* Return non-zero if requested endpoint is halted */
        int (*halt_endpoint) 
                (struct pcd_instance *, 
                 struct usbd_endpoint_instance *,
                 int);                                  /* Return non-zero if requested endpoint clear fails */

        /* 16 */
        void (*startup_events) (struct pcd_instance *);   /* perform UDC specific USB events */

	/* 17. root hub operations
	 */
	int (*hub_status) (struct pcd_instance *);
	int (*port_status) (struct pcd_instance *, int);
	int (*hub_feature) (struct pcd_instance *, int, int);
	int (*port_feature) (struct pcd_instance *, int, int, int);
	int (*get_ports_status) (struct pcd_instance *);
	int (*wait_for_change) (struct pcd_instance *);
};

extern struct usbd_pcd_ops usbd_pcd_ops;

/*! bus_set_speed
 */     
void bus_set_speed(struct usbd_bus_instance *bus, int hs);


/*! pcd_rcv_next_irq - get the current or next urb to receive into
 * Called by UDC driver to get current receive urb or next available receive urb.
 */
static __inline__ struct usbd_urb * pcd_rcv_next_irq (struct usbd_endpoint_instance *endpoint)
{
        if (!endpoint->rcv_urb)
                if ( (endpoint->rcv_urb = usbd_first_urb_detached_irq (&endpoint->rdy)))
                        endpoint->rcv_urb->status = USBD_URB_ACTIVE;

        //TRACE_MSG1(PCD, "BUS_RCV_URB: %p", endpoint->rcv_urb);
        return endpoint->rcv_urb;
}


/*! pcd_rcv_complete_irq - complete a receive operation
 * Called by UDC driver to show completion of outstanding I/O, this will
 * update the urb length and if necessary will finish the urb passing it to the
 * bottom half which will in turn call the function driver callback.
 */
struct usbd_urb * pcd_rcv_complete_irq (struct usbd_endpoint_instance *endpoint, int len, int urb_bad);
struct usbd_urb * pcd_rcv_finished_irq (struct usbd_endpoint_instance *endpoint, int len, int urb_bad);


/*! pcd_rcv_cancelled_irq - cancel current receive urb
 * Called by UDC driver to cancel the current receive urb.
 */
static __inline__ void pcd_rcv_cancelled_irq (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *rcv_urb;

        TRACE_MSG1(PCD, "BUS_RCV CANCELLED: %p", (int) endpoint->rcv_urb);
        RETURN_IF (! (rcv_urb = endpoint->rcv_urb));
        //TRACE_MSG1(PCD, "rcv_urb: %p", (int)endpoint->rcv_urb);
        usbd_urb_finished_irq (rcv_urb, USBD_URB_CANCELLED);
        endpoint->sent = endpoint->last = 0;
        endpoint->rcv_urb = NULL;
}


/*! pcd_tx_next_irq - get the current or next urb to transmit
 * Called by UDC driver to get current transmit urb or next available transmit urb.
 */
static __inline__ struct usbd_urb * pcd_tx_next_irq (struct usbd_endpoint_instance *endpoint)
{
        if (!endpoint->tx_urb)
                if ( (endpoint->tx_urb = usbd_first_urb_detached_irq (&endpoint->tx))) {
#if 0
                        int i;
                        u8 *cp = endpoint->tx_urb->buffer;

                        TRACE_MSG2(PCD, "NEXT TX: length: %d flags: %x", 
                                        endpoint->tx_urb->actual_length, endpoint->tx_urb->flags);

                        for (i = 0; i < endpoint->tx_urb->actual_length;  i+= 8)

                                TRACE_MSG8(PCD, "SENT  %02x %02x %02x %02x %02x %02x %02x %02x",
                                                cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                                cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                          );
#endif
                        endpoint->tx_urb->status = USBD_URB_ACTIVE;
                }
        //TRACE_MSG1(PCD, "BUS_TX NEXT TX_URB: %p", (int)endpoint->tx_urb);
        return endpoint->tx_urb;
}


/*! pcd_tx_complete_irq - complete a transmit operation
 * Called by UDC driver to show completion of an outstanding I/O, this will update the urb sent
 * information and if necessary will finish the urb passing it to the bottom half which will in
 * turn call the function driver callback.
 */
struct usbd_urb * pcd_tx_complete_irq (struct usbd_endpoint_instance *endpoint, int restart);


/*! pcd_tx_cancelled_irq - finish pending tx_urb with USBD_URB_CANCELLED
 * Called by UDC driver to cancel the current transmit urb.
 */
static __inline__ void pcd_tx_cancelled_irq (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *tx_urb;

        TRACE_MSG1(PCD, "BUS_TX CANCELLED: %p", (int) endpoint->tx_urb);
        RETURN_IF (! (tx_urb = endpoint->tx_urb));
        usbd_urb_finished_irq (tx_urb, USBD_URB_CANCELLED);
        endpoint->sent = endpoint->last = 0;
        endpoint->tx_urb = NULL;
}


/*! pcd_tx_sendzlp - test if we need to send a ZLP
 * This has a side-effect of reseting the USBD_URB_SENDZLP flag.
 */
static __inline__ int pcd_tx_sendzlp (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *tx_urb = endpoint->tx_urb;
        RETURN_FALSE_UNLESS(tx_urb);                                    // no urb
        RETURN_FALSE_IF(!endpoint->sent && tx_urb->actual_length);      // nothing sent yet and there is data to send
        RETURN_FALSE_IF(tx_urb->actual_length > endpoint->sent);        // still data to send
        RETURN_FALSE_UNLESS(tx_urb->flags & USBD_URB_SENDZLP);          // flag not set

        tx_urb->flags &= ~USBD_URB_SENDZLP;
        return TRUE;
}


/*! pcd_rcv_complete_irq - complete a receive
 * Called from rcv interrupt to complete.
 */
static __inline__ void pcd_rcv_fast_complete_irq (struct usbd_endpoint_instance *endpoint, struct usbd_urb *rcv_urb)
{
        //TRACE_MSG1(PCD, "BUS_RCV FAST COMPLETE: %d", rcv_urb->actual_length);
        usbd_urb_finished_irq (rcv_urb, USBD_URB_OK);
} 

/*! pcd_recv_setup - process a device request
 * Note that we verify if a receive urb has been queued for H2D with non-zero wLength
 * and return -EINVAL to stall if the upper layers have not properly tested for and 
 * setup a receive urb in this case.
 */
static __inline__ int pcd_recv_setup_irq (struct pcd_instance *pcd, struct usbd_device_request *request)
{
        struct usbd_bus_instance *bus = pcd->bus;
        struct usbd_endpoint_instance *endpoint = bus->endpoint_array + 0;
        TRACE_SETUP (PCD, request);
        RETURN_EINVAL_IF (usbd_device_request(bus, request));         // fail if already failed

        //TRACE_MSG2(PCD, "BUS_RECV SETUP: RCV URB: %x TX_URB: %x", (int)endpoint->rcv_urb, (int)endpoint->tx_urb);
        RETURN_ZERO_IF ( (request->bmRequestType & USB_REQ_DIRECTION_MASK) == USB_REQ_DEVICE2HOST);      
        RETURN_ZERO_IF (!le16_to_cpu (request->wLength));
        RETURN_EINVAL_IF (!endpoint->rcv_urb);
        return 0;
}

/*! pcd_recv_setup_emulate_irq - emulate a device request
 * Called by the UDC driver to inject a SETUP request. This is typically used
 * by drivers for UDC's that do not pass all SETUP requests to us but instead give us
 * a configuration change interrupt. 
 */
int pcd_recv_setup_emulate_irq(struct usbd_bus_instance *, u8, u8, u16, u16, u16);

/*! pcd_ep0_reset_irq - reset ep0 endpoint 
 */
static void __inline__ pcd_ep0_reset_endpoint_irq (struct usbd_endpoint_instance *endpoint)
{
        TRACE_MSG0(PCD, "BUS_EP0 RESET");
        pcd_tx_cancelled_irq (endpoint);
        pcd_rcv_cancelled_irq (endpoint);
        endpoint->sent = endpoint->last = 0;
}

/*! pcd_check_device_feature - verify that feature is set or clear
 * Check current feature setting and emulate SETUP Request to set or clear
 * if required.
 */
void pcd_check_device_feature(struct usbd_bus_instance *, int , int );

