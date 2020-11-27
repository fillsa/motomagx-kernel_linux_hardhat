/*
 * otg/ocd/otg-pcd/pcd.c - OTG Peripheral Controller Driver
 * @(#) balden@seth2.belcarratech.com|otg/ocd/otglib/pcd.c|20051117004939|15419
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2007 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 02/23/2006         Motorola         Enable HS USB 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 * 11/23/2007         Motorola         Changes for OSS compliance.
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
 */
/*!
 * @file otg/ocd/otglib/pcd.c
 * @brief PCD only driver init.
 * Notes
 *
 * 1. The pcd file abstracts much of the UDC complexity as possible and
 * provides a common implementation that is shared to various extents by the
 * various pcd drivers.
 *
 * @ingroup OTGPCD
 */

#include <otg/pcd-include.h>
//#include <asm/au1000.h>


//extern char *serial_number_str;

/* ******************************************************************************************* */
/*! bus_request_endpoints
 */
int bus_request_endpoints(struct usbd_bus_instance *bus, struct usbd_endpoint_map *endpoint_map_array, int endpointsRequested, 
                struct usbd_endpoint_request *requestedEndpoints)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int i;
        TRACE_MSG0(PCD, "BUS_REQUEST ENDPOINTS:");
        if (usbd_pcd_ops.request_endpoints(pcd, endpoint_map_array, endpointsRequested, requestedEndpoints)) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
                return -EINVAL;
        }
        for (i = 0; i < endpointsRequested; i++) {
                struct usbd_endpoint_map *endpoint_map = endpoint_map_array + i;
                 TRACE_MSG8(PCD, "address: %02x:%02x physical: %02x:%02x request: %02x:%02x size: %04x:%04x", 
                                 endpoint_map->bEndpointAddress[0], endpoint_map->bEndpointAddress[1], 
                                 endpoint_map->physicalEndpoint[0], endpoint_map->physicalEndpoint[1], 
                                 endpoint_map->bmAttributes[0], endpoint_map->bmAttributes[1], 
                                 endpoint_map->wMaxPacketSize[0], endpoint_map->wMaxPacketSize[1]
                                 );
        }
        return 0;
}

/*! bus_set_endpoints
 */
int bus_set_endpoints(struct usbd_bus_instance *bus, int endpointsRequested, struct usbd_endpoint_map *endpoint_map_array)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        TRACE_MSG0(PCD, "SET ENDPOINTS");
        // XXX do we need to set bEndpointAddress
        return usbd_pcd_ops.set_endpoints ? usbd_pcd_ops.set_endpoints(pcd, endpointsRequested, endpoint_map_array) : 0;
}

/*! bus_set_address
 */
int bus_set_address(struct usbd_bus_instance *bus, int address)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        if (usbd_pcd_ops.set_address) usbd_pcd_ops.set_address (pcd, address);
        return 0;
}

/*! bus_set_speed
 */
void bus_set_speed(struct usbd_bus_instance *bus, int hs)
{
        bus->high_speed = hs;
}

/* ******************************************************************************************* */

/*! pcd_search_endpoint_index - find endpoint map index given endpoint address
 */
int pcd_search_endpoint_index(struct usbd_bus_instance *bus, int bEndpointAddress)
{
        int i;
        for (i = 0; i < bus->endpoints; i++)
                BREAK_IF (bus->endpoint_array[i].new_bEndpointAddress[bus->high_speed] == bEndpointAddress);
        return i;
}

/*! pcd_search_endpoint - find endpoint given endpoint address
 */
struct usbd_endpoint_instance * pcd_search_endpoint(struct usbd_bus_instance *bus, int bEndpointAddress)
{
        int i = pcd_search_endpoint_index(bus, bEndpointAddress);
        TRACE_MSG2(PCD, "BUS_SEARCH ENDPOINT:  addr: %02x index: %d", bEndpointAddress, i);
        RETURN_NULL_UNLESS(i < bus->endpoints);
        return &(bus->endpoint_array[i]);
}

/*! bus_endpoint_halted - check if endpoint halted
 * Used by the USB Device Core to check endpoint halt status.
 *
 * Return actual halted status if available or'd with endpoint->feature_setting set value.
 * Assume not halted if udc driver does not provide an endpoint halted function.
 */
int bus_endpoint_halted (struct usbd_bus_instance *bus, int bEndpointAddress)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int endpoint_index = pcd_search_endpoint_index(bus, bEndpointAddress);
        struct usbd_endpoint_instance *endpoint = pcd_search_endpoint(bus, bEndpointAddress);
        int halted;

        TRACE_MSG1(PCD, "BUS_ENDPOINT HALTED:  endpoint: %p", (int) endpoint);
        RETURN_EINVAL_UNLESS(endpoint);

        //halted = usbd_pcd_ops.endpoint_halted ? usbd_pcd_ops.endpoint_halted(pcd, endpoint_index) : 0;
        halted = usbd_pcd_ops.endpoint_halted ? usbd_pcd_ops.endpoint_halted(pcd, endpoint) : 0;
        TRACE_MSG1(PCD, "BUS_ENDPOINT HALTED: %08x", bus->endpoint_array[endpoint_index].feature_setting);

        return halted || (endpoint->feature_setting & FEATURE(USB_ENDPOINT_HALT)? 1 : 0);
}

/*! bus_halt_endpoint - handle set/clear feature requests
 * Used by the USB Device Core to set/clear endpoint halt status.
 *
 * We assume that if the udc driver does not implement anything then 
 * we should just return zero for ok.
 *
 * XXX should do endpoint instance as arg
 */
int bus_halt_endpoint (struct usbd_bus_instance *bus, int bEndpointAddress,  int flag)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int endpoint_index = pcd_search_endpoint_index(bus, bEndpointAddress);
        struct usbd_endpoint_instance *endpoint = pcd_search_endpoint(bus, bEndpointAddress);

        TRACE_MSG1(PCD, "BUS_ENDPOINT HALT:  endpoint: %p", (int) endpoint);
        RETURN_EINVAL_UNLESS(endpoint);
        endpoint->feature_setting = flag;
        return usbd_pcd_ops.halt_endpoint ? usbd_pcd_ops.halt_endpoint(pcd, endpoint, flag) : 0;
}

/*! pcd_disable_endpoints - disable udc and all endpoints
 */
static void pcd_disable_endpoints (struct usbd_bus_instance *bus)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int i;
        RETURN_IF (!bus || !bus->endpoint_array);
        for (i = 1; i < usbd_pcd_ops.max_endpoints; i++) {
                struct usbd_endpoint_instance *endpoint = (bus->endpoint_array + i);
                CONTINUE_IF (!endpoint);
                usbd_flush_endpoint (endpoint);
        }
}


/*! bus_event_handler - handle generic bus event
 * Called by usb core layer to inform bus of an event.
 */
int bus_event_handler (struct usbd_bus_instance *bus, usbd_device_event_t event, int data)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        int epn;
        //struct usbd_endpoint_map *endpoint_map_array = bus->function_instance->endpoint_map_array;

        //TRACE_MSG1(PCD, "BUS_EVENT %x", event);
        switch (event) {
        case DEVICE_UNKNOWN:
                break;

        case DEVICE_INIT:
                TRACE_MSG0(PCD, "EVENT INIT");
                break;

        case DEVICE_CREATE:
                TRACE_MSG0(PCD, "EVENT CREATE");
                pcd_disable_endpoints (bus);
                if (usbd_pcd_ops.start) usbd_pcd_ops.start (pcd);
                break;

        case DEVICE_HUB_CONFIGURED:
                TRACE_MSG0(PCD, "EVENT HUB_CONFIGURED");
                //bi_connect (bus, NULL);
                break;

        case DEVICE_RESET:
                TRACE_MSG0(PCD, "EVENT RESET");

                if (usbd_pcd_ops.set_address) usbd_pcd_ops.set_address (pcd, 0);

                if (usbd_pcd_ops.reset_ep) usbd_pcd_ops.reset_ep (pcd, 0);

                pcd_disable_endpoints (bus);
                otg_event_irq(pcd->otg, (u64)(BUS_RESET | BUS_SUSPENDED_), PCD, "DEVICE_RESET");

                break;

        case DEVICE_ADDRESS_ASSIGNED:
                TRACE_MSG1(PCD, "EVENT ADDRESSED: %d", data);

                otg_event_irq(pcd->otg, ADDRESSED, PCD, "ADDRESSED");

                break;

        case DEVICE_CONFIGURED:
                TRACE_MSG0(PCD, "EVENT CONFIGURED");
                otg_event_irq(pcd->otg, CONFIGURED, PCD, "CONFIGURED");
                // iterate across the physical endpoint instance array to enable the endpoints
                if (usbd_pcd_ops.setup_ep)
                        for (epn = 1; epn < bus->endpoints; epn++) 
                                usbd_pcd_ops.setup_ep (pcd, epn, bus->endpoint_array + epn);
                break;

        case DEVICE_DE_CONFIGURED:
                TRACE_MSG0(PCD, "EVENT DE-CONFIGURED");
                otg_event_irq(pcd->otg, ADDRESSED, PCD, "DE-CONFIGURED");
                break;

        case DEVICE_SET_INTERFACE:
                TRACE_MSG0(PCD, "EVENT SET INTERFACE");
                break;

        case DEVICE_SET_FEATURE:
                TRACE_MSG0(PCD, "EVENT SET FEATURE");
                break;

        case DEVICE_CLEAR_FEATURE:
                TRACE_MSG0(PCD, "EVENT CLEAR FEATURE");
                break;

        case DEVICE_BUS_INACTIVE:
                TRACE_MSG0(PCD, "EVENT INACTIVE");
                TRACE_MSG1(PCD, "EVENT INACTIVE: state: %x", bus->device_state);
                otg_event_irq(pcd->otg, BUS_SUSPENDED, PCD, "DEVICE_BUS_INACTIVE");
                break;

        case DEVICE_BUS_ACTIVITY:
                TRACE_MSG0(PCD, "EVENT ACTIVITY");
                otg_event_irq(pcd->otg, BUS_SUSPENDED_, PCD, "DEVICE_BUS_ACTIVITY");
                break;

        case DEVICE_POWER_INTERRUPTION:
                TRACE_MSG0(PCD, "POWER INTERRUPTION");
                break;

        case DEVICE_HUB_RESET:
                TRACE_MSG0(PCD, "HUB RESET");
                break;

        case DEVICE_DESTROY:
                TRACE_MSG0(PCD, "DEVICE DESTROY");
                //bi_disconnect (bus, NULL);
                pcd_disable_endpoints (bus);
                if (usbd_pcd_ops.stop) usbd_pcd_ops.stop (pcd);
                break;

        case DEVICE_CLOSE:
                TRACE_MSG0(PCD, "DEVICE CLOSE");
                break;
        default:
                TRACE_MSG1(PCD, "DEVICE UNKNOWN: %d", event);
                break;
        }
        //TRACE_MSG2(PCD, "EVENT bNumInterfaces: %x alternates: %x", bus->bNumInterfaces, bus->alternates);
        return 0;
}


/*! bus_start_endpoint_in
 */
int bus_start_endpoint_in (struct usbd_bus_instance *bus, struct usbd_endpoint_instance *endpoint)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        unsigned long flags;

        RETURN_ZERO_IF (!endpoint);
        pcd->otg->interrupts++;

        local_irq_save (flags);

        // call pcd_start_endpoint_in IFF we didn't previously have a tx urb 
        if (!endpoint->tx_urb && pcd_tx_next_irq (endpoint)) 
                usbd_pcd_ops.start_endpoint_in (pcd, endpoint);
        
        local_irq_restore (flags);
        return 0;
}

/*! bus_start_endpoint_out
 */
int bus_start_endpoint_out (struct usbd_bus_instance *bus, struct usbd_endpoint_instance *endpoint)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        unsigned long flags;

        RETURN_ZERO_IF (!endpoint);
        pcd->otg->interrupts++;
        local_irq_save (flags);
        // call pcd_start_endpoint_OUT IFF we didn't previously have a rcv urb 
        if (!endpoint->rcv_urb && pcd_rcv_next_irq (endpoint)) 
                usbd_pcd_ops.start_endpoint_out (pcd, endpoint);
        
        local_irq_restore (flags);
        return 0;
}


/*! bus_cancel_urb_irq - cancel sending an urb
 * Used by the USB Device Core to cancel an urb.
 */
int bus_cancel_urb_irq (struct usbd_urb *urb)
{
        struct usbd_bus_instance *bus = urb->bus;
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        struct usbd_endpoint_instance *endpoint;
        RETURN_EINVAL_IF (!urb);
        //TRACE_MSG1(PCD, "BUS_CANCEL URB: %x", (int)urb);
        endpoint = urb->endpoint;
        switch (urb->endpoint->new_bEndpointAddress[urb->bus->high_speed] & USB_ENDPOINT_DIR_MASK) {
        case USB_DIR_IN:
                // is this the active urb?
                if (urb->endpoint->tx_urb == urb) {
                        urb->endpoint->tx_urb = NULL;
                        //TRACE_MSG1(PCD, "CANCEL IN URB: %02x", urb->endpoint->new_bEndpointAddress[urb->bus->high_speed]);
                        if (usbd_pcd_ops.cancel_in_irq) usbd_pcd_ops.cancel_in_irq (pcd, urb);
                }
                usbd_urb_finished_irq (urb, USBD_URB_CANCELLED);
                break;

        case USB_DIR_OUT:
                // is this the active urb?
                if (urb->endpoint->rcv_urb == urb) {
                        urb->endpoint->rcv_urb = NULL;
                        //TRACE_MSG1(PCD, "CANCEL OUT URB: %02x", urb->endpoint->new_bEndpointAddress[urb->bus->high_speed]);
                        if (usbd_pcd_ops.cancel_out_irq) usbd_pcd_ops.cancel_out_irq (pcd, urb);
                }
                //TRACE_MSG0(PCD, "CANCEL RECV URB");
                usbd_urb_finished_irq (urb, USBD_URB_CANCELLED);
                break;
        }
        endpoint->last = endpoint->sent = 0;
        return 0;
}

/*! pcd_rcv_finished_irq 
 */
struct usbd_urb * pcd_rcv_finished_irq (struct usbd_endpoint_instance *endpoint, int len, int urb_bad)
{
        struct usbd_urb *rcv_urb;

        // if we had an urb then update actual_length, dispatch if neccessary
        if (likely ( (int) (rcv_urb = endpoint->rcv_urb))) {
                struct usbd_bus_instance *bus = rcv_urb->bus;
                struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;

                //TRACE_MSG5(PCD, "BUS_RCV COMPLETE: finished buffer: %x actual: %d len: %d bad: %d: status: %d",
                //                rcv_urb->buffer, rcv_urb->actual_length, len, urb_bad, rcv_urb->status);

                if (!urb_bad && !endpoint->rcv_error && (rcv_urb->bus->status == USBD_OK)) {

#if 0
                        int i;
                        u8 *cp = rcv_urb->buffer;
                        for (i = 0; i < rcv_urb->actual_length + len; i+= 8) {

                                TRACE_MSG8(PCD, "RECV  %02x %02x %02x %02x %02x %02x %02x %02x",
                                                cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                                cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                          );
                        }
#endif
                        // increment the received data size
                        rcv_urb->actual_length += len;

                        if (rcv_urb->actual_length && !len)
                                rcv_urb->flags |= USBD_URB_SENDZLP;

                        endpoint->rcv_urb = NULL;
                        rcv_urb->framenum = pcd_ops.framenum ? pcd_ops.framenum () : 0;
                        //TRACE_MSG1(PCD, "BUS_RCV COMPLETE: framenum: %x", (int) rcv_urb->framenum);
                        usbd_urb_finished_irq (rcv_urb, USBD_URB_OK);
                        rcv_urb = NULL;
                }
                else {
                        rcv_urb->actual_length = 0;
                        //endpoint->rcv_error = 1;
                }
        }

        // if we don't have an urb see if we can get one
        return pcd_rcv_next_irq (endpoint);
}

/*! pcd_rcv_complete_irq 
 */
struct usbd_urb * pcd_rcv_complete_irq (struct usbd_endpoint_instance *endpoint, int len, int urb_bad)
{
        struct usbd_urb *rcv_urb;

        // if we had an urb then update actual_length, dispatch if neccessary
        if (likely ( (int) (rcv_urb = endpoint->rcv_urb))) {
                struct usbd_bus_instance *bus = rcv_urb->bus;
                struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;

                TRACE_MSG4(PCD, "BUS_RCV COMPLETE: actual: %d len: %d bad: %d: status: %d",
                                rcv_urb->actual_length, len, urb_bad, rcv_urb->status);

                TRACE_MSG4(PCD, "BUS_RCV COMPLETE: buffer: %d alloc: %d packet: %d transfer: %d",
                                rcv_urb->buffer_length, rcv_urb->alloc_length,
                                endpoint->new_wMaxPacketSize[bus->high_speed], endpoint->rcv_transferSize);

                // check the urb is ok, are we adding data less than the packetsize
                if (!urb_bad && !endpoint->rcv_error && (rcv_urb->bus->status == USBD_OK) 
                                /*&& (len <= endpoint->wMaxPacketSize)*/) 
                {

                        // increment the received data size
                        rcv_urb->actual_length += len;

                        // if the current received data is short (less than full packetsize) which
                        // indicates the end of the bulk transfer, we have received the maximum
                        // transfersize, or if we do not have enough room to receive another packet 
                        // then pass this data up to the function driver

                        // XXX this needs to be fixed, for example the MSC driver 
                        // has varying maximum sizes


                        if (
                                        ( 
                                          //(len < endpoint->wMaxPacketSize) ||
                                          ((rcv_urb->actual_length % endpoint->new_wMaxPacketSize[bus->high_speed])) ||
                                          (rcv_urb->actual_length >= endpoint->rcv_transferSize) ||
                                          (rcv_urb->actual_length >= rcv_urb->buffer_length) ||
                                          (rcv_urb->actual_length + endpoint->new_wMaxPacketSize[bus->high_speed] > 
                                           rcv_urb->alloc_length) || !len))
                        {
#if 0
                                int i;
                                u8 *cp = rcv_urb->buffer;
                                for (i = 0; i < rcv_urb->actual_length; i+= 8)
                                        TRACE_MSG8(PCD, "RECV  %02x %02x %02x %02x %02x %02x %02x %02x",
                                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                                  );
#endif
                                endpoint->rcv_urb = NULL;
                                rcv_urb->framenum = pcd_ops.framenum ? pcd_ops.framenum () : 0;
                                TRACE_MSG2(PCD, "BUS_RCV COMPLETE: finished length: %d framenum: %x", 
                                                rcv_urb->actual_length, (int) rcv_urb->framenum);
                                usbd_urb_finished_irq (rcv_urb, USBD_URB_OK);
                                endpoint->last = 0;
                                rcv_urb = NULL;
                        }
                }
                else {
                        rcv_urb->actual_length = 0;
                        //endpoint->rcv_error = 1;
                }
        }

        // if we don't have an urb see if we can get one
        return pcd_rcv_next_irq (endpoint);
}

/*! pcd_tx_complete_irq 
 */
struct usbd_urb * pcd_tx_complete_irq (struct usbd_endpoint_instance *endpoint, int restart)
{
        struct usbd_urb *tx_urb;

        TRACE_MSG2(PCD, "tx_urb: %x restart: %d", endpoint->tx_urb, restart);

        // if we have a tx_urb advance or reset, finish if complete
        if ( (tx_urb = endpoint->tx_urb)) {

                struct usbd_bus_instance *bus = tx_urb->bus;
                struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;

                TRACE_MSG5(PCD, "BUS_TX CURRENT TX_URB: %p actual: %d sent: %d last: %d: flags: %d", 
                                (int)endpoint->tx_urb, tx_urb->actual_length, endpoint->sent,
                                endpoint->last, tx_urb->flags);

                if (likely (!restart)) {
                        int sent = endpoint->last;
                        endpoint->sent += sent;
                        endpoint->last -= sent;
                }
                else {
                        TRACE_MSG0(PCD, "RESTARTING");
                        endpoint->last = 0;
                }

                //if ( ( (tx_urb->actual_length - endpoint->sent) <= 0) && ! (tx_urb->flags & USBD_URB_SENDZLP) ) {
                
                if ( (endpoint->sent >= tx_urb->actual_length) && ! (tx_urb->flags & USBD_URB_SENDZLP) ) {
                        tx_urb->framenum = pcd_ops.framenum ? pcd_ops.framenum () : 0;
                        TRACE_MSG2(PCD, "BUS_TX COMPLETE: finished tx_urb: %p framenum: %x", 
                                        (int)tx_urb, (int)tx_urb->framenum);
                        usbd_urb_finished_irq (tx_urb, USBD_URB_OK);
                        endpoint->tx_urb = NULL;
                        endpoint->last = endpoint->sent = 0;
                }
        }
        return pcd_tx_next_irq (endpoint);
}

/* ******************************************************************************************* */

/*! pcd_disable - Stop using the USB Device Controller
 * Stop using the USB Device Controller.
 * Shutdown and free dma channels, de-register the interrupt handler.
 */
void pcd_disable (struct usbd_bus_instance *bus)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        TRACE_MSG0(PCD, "BUS_UDC DISABLE");
        if (usbd_pcd_ops.disable_ep) usbd_pcd_ops.disable_ep (pcd, 0);
        pcd_disable_endpoints (bus);

        if (usbd_pcd_ops.disable) usbd_pcd_ops.disable (pcd);
}

/* ************************************************************************************* */
/*! bus_framenum 
 */
int bus_framenum (void)
{
        return (pcd_ops.framenum) ? pcd_ops.framenum () : 0;
}

#if 0
/*! bus_interrupts 
 */
u64 bus_interrupts (void)
{
        return pcd_instance->otg->interrupts;
}
/*! bus_ticks 
 */
u64 bus_ticks (void)
{
        return (ocd_ops.ticks) ? ocd_ops.ticks () : 0;
}

/*! bus_elapsed 
 */
u64 bus_elapsed (u64 *t1, u64 *t2)
{
        return (ocd_ops.elapsed) ? (ocd_ops.elapsed (t1, t2)) : 0;
}
#endif

/*! pcd_recv_setup_emulate_irq - emulate a device request
 */
int pcd_recv_setup_emulate_irq(struct usbd_bus_instance *bus, u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        struct usbd_device_request request;

        TRACE_MSG4(PCD, "bmRequestType: %02x bRequest: %02x wValue: %04x wIndex: %04x", bmRequestType, bRequest, wValue, wIndex);
        request.bmRequestType = bmRequestType;
        request.bRequest = bRequest;
        request.wValue = cpu_to_le16(wValue);
        request.wIndex = cpu_to_le16(wIndex);
        request.wLength = cpu_to_le16(wLength);
        return pcd_recv_setup_irq(pcd, &request);
}

/*! pcd_check_device_feature - verify that feature is set or clear
 * Check current feature setting and emulate SETUP Request to set or clear
 * if required.
 */
void pcd_check_device_feature(struct usbd_bus_instance *bus, int feature, int flag)
{
        int status = bus->device_feature_settings & (1 << feature);
        TRACE_MSG2(PCD, "BUS_CHECK FEATURE: feature: %x flag: %x", feature, flag);
        if (!status && flag)
                pcd_recv_setup_emulate_irq(bus, USB_REQ_HOST2DEVICE, USB_REQ_SET_FEATURE, feature, 0, 0);
        else if (status && !flag)
                pcd_recv_setup_emulate_irq(bus, USB_REQ_HOST2DEVICE, USB_REQ_CLEAR_FEATURE, feature, 0, 0);
        TRACE_MSG1(PCD, "BUS_CHECK FEATURE: features; %08x", bus->device_feature_settings);
}

/* ******************************************************************************************* */
#if !defined(OTG_C99)
struct usbd_bus_operations bus_ops;

void pcd_global_init(void)
{
        ZERO(bus_ops);
        bus_ops.start_endpoint_in = bus_start_endpoint_in;
        bus_ops.start_endpoint_out = bus_start_endpoint_out;
        bus_ops.cancel_urb_irq = bus_cancel_urb_irq;
        bus_ops.endpoint_halted = bus_endpoint_halted;
        bus_ops.halt_endpoint = bus_halt_endpoint;
        bus_ops.set_address = bus_set_address;
        bus_ops.event_handler = bus_event_handler;
        bus_ops.request_endpoints = bus_request_endpoints;
        bus_ops.set_endpoints = bus_set_endpoints;
        bus_ops.framenum = bus_framenum;
        //bus_ops.ticks = bus_ticks;
        //bus_ops.elapsed = bus_elapsed;
        //bus_ops.interrupts = bus_interrupts;
        //bus_ops.device_feature = bus_device_feature;
}

#else /* !defined(OTG_C99) */
struct usbd_bus_operations bus_ops = {
        .start_endpoint_in = bus_start_endpoint_in,
        .start_endpoint_out = bus_start_endpoint_out,
        .cancel_urb_irq = bus_cancel_urb_irq,
        .endpoint_halted = bus_endpoint_halted,
        .halt_endpoint = bus_halt_endpoint,
        .set_address = bus_set_address,
        .event_handler = bus_event_handler,
        .request_endpoints = bus_request_endpoints,
        .set_endpoints = bus_set_endpoints,
        .framenum = bus_framenum,
        //.ticks = bus_ticks,
        //.elapsed = bus_elapsed,
        //.interrupts = bus_interrupts,
        //.device_feature = bus_device_feature,
};
#endif /* !defined(OTG_C99) */


struct usbd_bus_driver bus_driver = {
        bops: &bus_ops,
};



/* ******************************************************************************************* */
/* Prevent overlapp of bi administrative functions mainly:
 *      bus_register_bh
 *      bus_deregister_bh
 */ 
//DECLARE_MUTEX (usbd_bi_sem);     
struct usbd_bus_instance *usbd_bus;

/*! pcd_startup_events
 */
void pcd_startup_events(struct usbd_bus_instance *bus)
{
        struct pcd_instance *pcd = (struct pcd_instance *)bus->privdata;
        TRACE_MSG0(PCD, "BI_STARTUP");
        if (usbd_pcd_ops.startup_events) {
                TRACE_MSG0(PCD, "BI_STARTUP_EVENTS - udc");
                usbd_pcd_ops.startup_events(pcd);
        }
        else {
                TRACE_MSG0(PCD, "BI_STARTUP_EVENTS - default");
                usbd_bus_event_handler_irq (bus, DEVICE_INIT, 0);
                usbd_bus_event_handler_irq (bus, DEVICE_CREATE, 0);
                usbd_bus_event_handler_irq (bus, DEVICE_HUB_CONFIGURED, 0);
                usbd_bus_event_handler_irq (bus, DEVICE_RESET, 0);
        }
        //TRACE_MSG0(PCD, "BI_STARTUP: FINISHED");
}

/*! bus_register_bh
 */
void bus_register_bh(void *arg)
{
        struct pcd_instance *pcd = arg;
        struct otg_instance *otg = pcd->otg;
        struct usbd_bus_instance *bus;
        struct usbd_endpoint_instance *endpoint;
        unsigned long flags;

        TRACE_MSG1(PCD, "BUS_REGISTER_BH: pcd: %p", (int)arg);
        RETURN_UNLESS(pcd);

        TRACE_MSG0(PCD, "BUS_REGISTER_BH");

        // XXX MODULE LOCK HERE
        
        bus_driver.name = usbd_pcd_ops.name;
        bus_driver.max_endpoints = usbd_pcd_ops.max_endpoints;
        bus_driver.maxpacketsize = usbd_pcd_ops.ep0_packetsize;
        bus_driver.high_speed_capable = usbd_pcd_ops.high_speed_capable;
        bus_driver.capabilities = usbd_pcd_ops.capabilities;
        bus_driver.bMaxPower = usbd_pcd_ops.bMaxPower;
        bus_driver.ports = usbd_pcd_ops.ports;
        // XXX bus_driver.otg_bmAttributes = tcd_ops.bmAttributes;
        bus_driver.otg_bmAttributes = usbd_pcd_ops.bmAttributes;

        TRACE_MSG1(PCD, "BUS_REGISTER_BH UDC Capabilities: %x", bus_driver.capabilities);
        // XXX TRACE_MSG1(PCD, "BUS_REGISTER_BH UDC OTG Attributes: %x\n", tcd_ops.bmAttributes);
        TRACE_MSG1(PCD, "BUS_REGISTER_BH UDC OTG Attributes: %x\n", usbd_pcd_ops.bmAttributes);

        // register this bus interface driver and create the device driver instance
        if (! (bus = usbd_register_bus (&bus_driver, usbd_pcd_ops.ep0_packetsize))) {
                TRACE_MSG0(PCD, "BUS_REGISTER_BH: register failed");
                // XXX pcd_disable (NULL);
                otg_event(pcd->otg, enable_otg_ | PCD_OK, PCD, "BUS_REGISTER_BH");
                // XXX MODULE UNLOCK HERE
                return;
        }

        bus->privdata = (void *) pcd;
        pcd->bus = bus;

        //if (usbd_pcd_ops.serial_init ? usbd_pcd_ops.serial_init (pcd) : -EINVAL) {
        //
        //}
        
        #if 0
        if (serial_number_str && strlen(serial_number_str)) {
                char *sp, *dp;
                int i;
                //printk(KERN_INFO"%s:\n", __FUNCTION__);
                //printk(KERN_INFO"%s: serial_number_str: %s\n", __FUNCTION__, serial_number_str);
                TRACE_MSG1(PCD, "prm serial_number_str: %s", serial_number_str);
                for (sp = serial_number_str, dp = otg->serial_number, i = 0;
                                *sp && (i < (sizeof(otg->serial_number) - 1)); i++, sp++)
                        if (isxdigit(*sp)) *dp++ = toupper(*sp);
        }
        #endif

        TRACE_MSG1(PCD, "otg serial_number_str: %s", pcd->otg->serial_number);
        TRACE_MSG1(PCD, "BUS_REGISTER_BH: usbd_enable_function_irq function_name: %s", 
                        pcd->otg->function_name ? pcd->otg->function_name : "");

        if (usbd_enable_function_irq (bus, pcd->otg->function_name, pcd->otg->serial_number)) {
                TRACE_MSG0(PCD, "BUS_REGISTER_BH: enable function failed");
                printk (KERN_INFO "%s: failed\n", __FUNCTION__);
                // XXX pcd_disable (NULL);
                otg_event(pcd->otg, enable_otg_ | PCD_OK, PCD, "BUS_REGISTER_BH");
                // XXX MODULE UNLOCK HERE
                return;
        }

        // setup endpoint zero
        endpoint = bus->endpoint_array + 0;
        //endpoint->old_bEndpointAddress = 0;
        endpoint->new_bEndpointAddress[bus->high_speed] = endpoint->new_bEndpointAddress[1] = 0;

        //endpoint->tx_attributes = 0;
        //

        endpoint->new_wMaxPacketSize[bus->high_speed] = endpoint->new_wMaxPacketSize[1] = usbd_pcd_ops.ep0_packetsize;
        endpoint->rcv_transferSize = 4096;       // XXX should this be higher

        // XXX high_speed???
        //
        if (usbd_pcd_ops.setup_ep) usbd_pcd_ops.setup_ep (pcd, 0, endpoint);

        TRACE_MSG0(PCD, "BUS_REGISTER_BH: startup");

        // hopefully device enumeration will finish this process
        // XXX should this move to pcd_en?
        pcd_startup_events (bus);
        TRACE_MSG0(PCD, "BUS_REGISTER_BH: FINISHED - sending PCD_OK");
        //MOD_INC_USE_COUNT;
        otg_event(pcd->otg, PCD_OK, PCD, "BUS_REGISTER_BH PCD_OK");
}

/*! bus_deregister_bh
 */
void bus_deregister_bh(void *arg)
{
        struct pcd_instance *pcd = arg;
        struct usbd_bus_instance *bus;
        struct bus_data *data;
        unsigned long flags;

        TRACE_MSG1(PCD, "BUS_DEREGISTER_BH: pcd: %x", pcd);
        if (pcd && (bus = pcd->bus) && (usbd_bus_state_enabled == bus->bus_state)) {

                if (usbd_pcd_ops.disable) usbd_pcd_ops.disable (pcd);

                //if (bus->device_state != STATE_ATTACHED) 
                if (bus->device_state == STATE_ATTACHED) {
                        usbd_bus_event_handler_irq (bus, DEVICE_RESET, 0);
                        usbd_bus_event_handler_irq (bus, DEVICE_POWER_INTERRUPTION, 0);
                        usbd_bus_event_handler_irq (bus, DEVICE_HUB_RESET, 0);
                }
                usbd_bus_event_handler_irq (bus, DEVICE_DESTROY, 0);
                pcd_disable_endpoints (bus);
                pcd_disable (bus);


                usbd_disable_function (bus);
                bus->bus_state = usbd_bus_state_disabled;

                //TRACE_MSG1(PCD, "%s: BI_SEM UP", (int)__FUNCTION__);
                //otg_event(pcd->otg, exit_ok, "BUS_DISABLE EXIT_OK");

                //if (bus->serial_number_str)
                //        lkfree (pcd->bus->serial_number_str);

                usbd_deregister_bus (bus);
                pcd->bus = NULL;

                TRACE_MSG0(PCD, "BUS_DEREGISTER_BH: FINISHED - sending PCD_OK");
                //MOD_DEC_USE_COUNT;
        }
        otg_event(pcd->otg, PCD_OK, PCD, "BUS_DEREGISTER_BH PCD_OK");
        // XXX MODULE UNLOCK HERE
	g_usb_bus_disable_clr();
}

/* ************************************************************************************* */


/*! pcd_en_func - enable
 *
 * This is called to enable / disable the PCD and USBD stack.
 */
void pcd_en_func (struct otg_instance *otg, u8 flag)
{
        struct pcd_instance *pcd = otg->pcd;
        struct usbd_bus_instance *bus = pcd->bus;

        //TRACE_MSG0(PCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(PCD, "PCD_EN: SET");
#ifdef CONFIG_OTG_DB1550_J15
                int ret = au_readl(SYS_PINFUNC);
                au_writel((ret | 0x8000),SYS_PINFUNC);
                ret = au_readl(SYS_PINFUNC);
                TRACE_MSG1(PCD, "Set the OTG for device mode (sys_pfunc = %04x)", ret);
#endif
                // check if we can see the UDC and register, then enable the function
                if (usbd_pcd_ops.enable) usbd_pcd_ops.enable (pcd);
                //BREAK_IF (pcd_enable_irq (bus));
                //BREAK_IF (usbd_enable_function_irq (bus, otg->function_name));

                // XXX should this move to here
                // pcd_startup_events (bus);
                break;

        case RESET:
                TRACE_MSG0(PCD, "PCD_EN: RESET");
                usbd_bus_event_handler_irq (bus, DEVICE_RESET, 0);
                TRACE_MSG0(PCD, "PCD_EN: DESTROY");
                usbd_bus_event_handler_irq (bus, DEVICE_DESTROY, 0);
                TRACE_MSG0(PCD, "PCD_EN: FINISHED");
                // XXX need to set a flag here
                break;
        }
}

/*! pcd_init_func - per peripheral controller common initialization
 *
 * This is called to initialize / de-initialize the PCD and USBD stack.
 *
 * We start work items to do this.
 *
 */
void pcd_init_func (struct otg_instance *otg, u8 flag)
{
        struct pcd_instance *pcd = otg->pcd;
        struct usbd_bus_instance *bus = pcd->bus;
        //struct bus_data *data = NULL;

        //TRACE_MSG0(PCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(PCD, "PCD_INIT: SET");
                PREPARE_WORK_ITEM(pcd->bh, bus_register_bh, pcd);
		if(g_usb_bus_disable_ing()){printk("\nLYN BUSDISABLED Return \n"); return; }
                SCHEDULE_WORK(pcd->bh);
                TRACE_MSG0(PCD, "BUS_REGISTER_SCHEDULE: finished");
                break;

        case RESET:
                TRACE_MSG0(PCD, "PCD_INIT: RESET");
                PREPARE_WORK_ITEM(pcd->bh, bus_deregister_bh, pcd);
		if(!g_usb_bus_disable_ing()){
			printk("\nLYN SET BUS DISABLE \n"); g_usb_bus_disable_set();
		}else{
			printk("\nLYN BUSDISABLED Return \n");	return;
		}
                DEBUS_SCHEDULE_WORK(pcd->bh);
        }
}

