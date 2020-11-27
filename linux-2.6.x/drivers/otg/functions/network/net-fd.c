/*
 * otg/functions/network/net-fd.c - Network Function Driver
 * @(#) balden@seth2.belcarratech.com|otg/functions/network/net-fd.c|20051116204958|59094
 *
 *      Copyright (c) 2002-2004 Belcarra
 *
 * By: 
 *      Chris Lynne <cl@belcarra.com>
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 06/08/2005         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 *
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
 * @file otg/functions/network/net-fd.c
 * @brief The lower edge (USB Device Function) implementation of
 * the Network Function Driver. This performs the core protocol
 * handling and data encpasulation.
 *
 * This implements the lower edge (USB Device) layer of the Network Function
 * Driver. Specifically the data encapsulation, envent and protocol handlers.
 *
 *
 *
 * This network function driver intended to interoperate with 
 * Belcarra's USBLAN Class drivers. 
 *
 * These are available for Windows, Linux and Mac OSX. For more 
 * information and to download a copy for testing:
 *
 *      http://www.belcarra.com/usblan/
 *
 * When configured for CDC it can also work with any CDC Class driver.
 *
 *
 * @ingroup NetworkFunction
 */


#include <otg/otg-compat.h>
#include <otg/otg-module.h>
//#include <linux/config.h>
//#include <linux/module.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <net/arp.h>
#include <linux/rtnetlink.h>
#include <linux/smp_lock.h>
#include <linux/ctype.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/atmdev.h>
#include <linux/pkt_sched.h>
#include <linux/random.h>
#include <linux/utsname.h>

#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/inetdevice.h>

#include <linux/kmod.h>

#include <asm/uaccess.h>
#include <asm/system.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
//#include <otg/usbd-mem.h>
#include <otg/usbp-func.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#include "network.h"
#include "net-fd.h"
#include "net-os.h"
#ifdef CONFIG_OTG_NETWORK_BLAN_FERMAT
#include "fermat.h"
#endif


static char ip_addr_str[32];
u32 ip_addr;
u32 router_ip;
u32 network_mask;
u32 dns_server_ip;

char * local_dev_addr_str;
char * remote_dev_addr_str;


//_________________________________________________________________________________________________
#if !defined (CONFIG_OTG_NETWORK_BLAN_INTERVAL)
#define CONFIG_OTG_NETWORK_BLAN_INTERVAL 1
#endif /* !defined (CONFIG_OTG_NETWORK_BLAN_INTERVAL) */

/*! Endpoint Request List
 */
struct usbd_endpoint_request net_fd_endpoint_requests[ENDPOINTS+1] = {
        { BULK_OUT, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { INT_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT, 16, 64, CONFIG_OTG_NETWORK_BLAN_INTERVAL, },
        { 0, },
};

u8 net_fd_endpoint_index[ENDPOINTS] = { BULK_OUT, BULK_IN, INT_IN, };

/*! Endpoint Request List
 */
struct usbd_endpoint_request cdc_data_endpoint_requests[2+1] = {
        { BULK_OUT, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { BULK_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_BULK, MAXFRAMESIZE + 48, MAXFRAMESIZE + 512, 0, },
        { 0, },
};
u8 cdc_data_endpoint_index[2] = { BULK_OUT, BULK_IN, };

/*! Endpoint Request List
 */
struct usbd_endpoint_request cdc_int_endpoint_requests[1+1] = {
        { INT_IN, 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT, 16, 64, CONFIG_OTG_NETWORK_BLAN_INTERVAL, },
        { 0, },
};

u8 cdc_int_endpoint_index[1] = { INT_IN, };

//_________________________________________________________________________________________________

/*
 * If the following are defined we implement the crc32_copy routine using
 * Duff's device. This will unroll the copy loop by either 4 or 8. Do not
 * use these without profiling to test if it actually helps on any specific
 * device.
 */
#undef CONFIG_OTG_NETWORK_CRC_DUFF4
#undef CONFIG_OTG_NETWORK_CRC_DUFF8

static u32 *network_crc32_table;

#define CRC32_INIT   0xffffffff      // Initial FCS value
#define CRC32_GOOD   0xdebb20e3      // Good final FCS value

#define CRC32_POLY   0xedb88320      // Polynomial for table generation
        
#define COMPUTE_FCS(val, c) (((val) >> 8) ^ network_crc32_table[((val) ^ (c)) & 0xff])

//_________________________________________________________________________________________________
//                                      crc32_copy

/*! make_crc_table
 * Generate the crc32 table
 *
 * @return non-zero if malloc fails
 */
STATIC int make_crc_table(void)
{
        u32 n;
        RETURN_ZERO_IF(network_crc32_table);
        RETURN_ENOMEM_IF(!(network_crc32_table = (u32 *)ckmalloc(256*4, GFP_KERNEL)));
        for (n = 0; n < 256; n++) {
                int k;
                u32 c = n;
                for (k = 0; k < 8; k++) {
                        c =  (c & 1) ? (CRC32_POLY ^ (c >> 1)) : (c >> 1);
                }
                network_crc32_table[n] = c;
        }
        return 0;
}

#if !defined(CONFIG_OTG_NETWORK_CRC_DUFF4) && !defined(CONFIG_OTG_NETWORK_CRC_DUFF8)
/*! crc32_copy
 * Copies a specified number of bytes, computing the 32-bit CRC FCS as it does so.
 *
 * @param dst   Pointer to the destination memory area.
 * @param src   Pointer to the source memory area.
 * @param len   Number of bytes to copy.
 * @param val   Starting value for the CRC FCS.
 *
 * @return      Final value of the CRC FCS.
 *
 * @sa crc32_pad
 */
static u32 __inline__ crc32_copy (u8 *dst, u8 *src, int len, u32 val)
{
#if !defined( CONFIG_OTG_NETWORK_BLAN_CRC) && !defined ( CONFIG_OTG_NETWORK_SAFE_CRC )
	printk (KERN_INFO"Error - This is call from crc32_copy, which shouldn't be!\n");
#else
//	printk (KERN_INFO"Call from crc32_copy.\n");
#endif	
        for (; len-- > 0; val = COMPUTE_FCS (val, *dst++ = *src++));
        return val;
}

#else /* DUFFn */

/*! crc32_copy
 * Copies a specified number of bytes, computing the 32-bit CRC FCS as it does so.
 *
 * @param dst   Pointer to the destination memory area.
 * @param src   Pointer to the source memory area.
 * @param len   Number of bytes to copy.
 * @param val   Starting value for the CRC FCS.
 *
 * @return      Final value of the CRC FCS.
 *
 * @sa crc32_pad
 */
static u32 crc32_copy (u8 *dst, u8 *src, int len, u32 val)
{
#if defined(CONFIG_OTG_NETWORK_CRC_DUFF8)
        int n = (len + 7) / 8;
        switch (len % 8) 
#elif defined(CONFIG_OTG_NETWORK_CRC_DUFF4)
                int n = (len + 3) / 4;
        switch (len % 4) 
#endif
        {
        case 0: do {
                        val = COMPUTE_FCS (val, *dst++ = *src++);
#if defined(CONFIG_OTG_NETWORK_CRC_DUFF8)
                case 7:
                        val = COMPUTE_FCS (val, *dst++ = *src++);
                case 6:
                        val = COMPUTE_FCS (val, *dst++ = *src++);
                case 5:
                        val = COMPUTE_FCS (val, *dst++ = *src++);
                case 4:
                        val = COMPUTE_FCS (val, *dst++ = *src++);
#endif
                case 3:
                        val = COMPUTE_FCS (val, *dst++ = *src++);
                case 2:
                        val = COMPUTE_FCS (val, *dst++ = *src++);
                case 1:
                        val = COMPUTE_FCS (val, *dst++ = *src++);
                } while (--n > 0);
        }
        return val;
}
#endif /* DUFFn */


//_________________________________________________________________________________________________
//                                      crc32_pad

/*! crc32_pad - pad and calculate crc32
 *
 * @return CRC FCS
 */
static u32 __inline__ crc32_pad (u8 *dst, int len, u32 val)
{
#if !defined( CONFIG_OTG_NETWORK_BLAN_CRC ) && !defined ( CONFIG_OTG_NETWORK_SAFE_CRC  )	
	printk (KERN_INFO"Error - This call is from  crc32_pad, which shouldn't be.\n");
#else
//	printk (KERN_INFO"Call from crc32_pad.\n");
#endif	
        for (; len-- > 0; val = COMPUTE_FCS (val, *dst++ = '\0'));
        return val;
}

//_________________________________________________________________________________________________
//                                      net_fd_send_int
//

/*! net_fd_urb_sent_int - callback for completed INT URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @return non-zero for failure.
 */
STATIC int net_fd_urb_sent_int (struct usbd_urb *urb, int urb_rc)
{
        unsigned long flags;
        int rc = -EINVAL;
        struct usb_network_private *npd = urb->function_privdata;

        TRACE_MSG3(NTT,"urb: %p npd: %p urb_rc: %d", urb, npd, urb_rc);

        local_irq_save(flags);
        npd->int_urb = NULL;
        usbd_free_urb (urb);
        local_irq_restore(flags);
        return 0;
}

/*! net_fd_send_int_notification - send an interrupt notification response
 *
 * Generates a response urb on the notification (INTERRUPT) endpoint.
 *
 * This is called from either a scheduled task or from the process context
 * that calls network_open() or network_close().
 * This must be called with interrupts locked out as net_fd_event_handler can
 * change the NETWORK_CONFIGURED status
 *
 */
void net_fd_send_int_notification(struct usbd_function_instance *function_instance, int connected, int data)
{
        struct usb_network_private *npd = function_instance ? function_instance->privdata : NULL;
        struct usbd_urb *urb;
        struct cdc_notification_descriptor *cdc;
        int rc;

        TRACE_MSG3(NTT,"npd: %p function: %p flags: %04x", npd, function_instance, npd->flags);

        do {
                BREAK_IF(!function_instance);

                BREAK_IF(npd->network_type != network_blan);
                BREAK_IF(!npd->have_interrupt);

                BREAK_IF(!(npd->flags & NETWORK_CONFIGURED));

                TRACE_MSG3(NTT,"connected: %d network: %d %d", connected, 
                           npd->network_type, network_blan);

                BREAK_IF(usbd_get_device_status(function_instance) != USBD_OK);

                //if (npd->int_urb) {
                //        printk(KERN_INFO"%s: int_urb: %p\n", __FUNCTION__, npd->int_urb);
                //        usbd_cancel_urb_irq(npd->int_urb);
                //        npd->int_urb = NULL;
                //}

                BREAK_IF(!(urb = usbd_alloc_urb (function_instance, INT_IN, 
                                                sizeof(struct cdc_notification_descriptor), net_fd_urb_sent_int)));
                
                urb->actual_length = sizeof(struct cdc_notification_descriptor);
                memset(urb->buffer, 0, sizeof(struct cdc_notification_descriptor));
                urb->function_privdata = npd;

                cdc = (struct cdc_notification_descriptor *)urb->buffer;

                cdc->bmRequestType = 0xa1;

                if (data) {
                        cdc->bNotification = 0xf0;
                        cdc->wValue = 1;
                }
                else {
                        cdc->bNotification = 0x00;
                        cdc->wValue = connected ? 0x01 : 0x00;
                }
                cdc->wIndex = 0x00; // XXX interface - check that this is correct


                npd->int_urb = urb;
                TRACE_MSG1(NTT,"int_urb: %p", urb);
                BREAK_IF (!(rc = usbd_start_in_urb (urb)));

                TRACE_MSG1(NTT,"usbd_start_in_urb failed err: %x", rc);
                printk(KERN_ERR"%s: usbd_start_in_urb failed err: %x\n", __FUNCTION__, rc);
                urb->function_privdata = NULL;
                npd->int_urb = NULL;
                usbd_free_urb (urb);

        } while(0);
}

//_________________________________________________________________________________________________
//                                      net_fd_start_xmit

/*! net_fd_urb_sent_bulk - callback for completed BULK xmit URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @param urb     Pointer to the urb that has been sent.
 * @param urb_rc  Result code from the send operation.
 *
 * @return non-zero for failure.
 */
int net_fd_urb_sent_bulk (struct usbd_urb *urb, int urb_rc)
{
        unsigned long flags;
        void *buf;
        int rc = -EINVAL;

        TRACE_MSG3(NTT,"urb: %p urb_rc: %d length: %d", urb, urb_rc, urb->actual_length);

        local_irq_save(flags);
        do {

                BREAK_IF(!urb);
                buf = urb->function_privdata;
                TRACE_MSG2(NTT,"urb: %p buf: %p", urb, buf);
                urb->function_privdata = NULL;
                net_os_xmit_done(urb->function_instance, buf, urb_rc);
                usbd_free_urb (urb);
                rc = 0;

        } while (0);
        local_irq_restore(flags);
        return rc;
}

/*! net_fd_start_xmit - start sending a buffer
 *
 * Called with net_os_mutex_enter()d.
 *
 * @return: 0 if all OK
 *         -EINVAL, -EUNATCH, -ENOMEM
 *         rc from usbd_start_in_urb() if that fails (is != 0, may be one of err values above) 
 *         Note: -ECOMM is interpreted by calling routine as signal to leave IF stopped.
 */
int net_fd_start_xmit (struct usbd_function_instance *function_instance, u8 *buffer, int len, void *data)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct usbd_urb *urb = NULL;
        int rc;
        int in_pkt_sz;
        u8 *cp;
        u32 crc;
        TRACE_MSG4(NTT,"npd: %p function: %p flags: %04x len: %d", npd, function_instance, npd->flags, len);

#if 0
        printk(KERN_INFO"%s: %s len: %d encap: %d\n", __FUNCTION__, net_device->name, len, npd->encapsulation);
        printk(KERN_INFO"start_xmit: len: %x data: %p\n", len, buffer);
        {
                u8 *cp = buffer;
                int i;
                for (i = 0; i < len; i++) {
                        if ((i%32) == 0) {
                                printk("\ntx[%2x] ", i);
                        }
                        printk("%02x ", *cp++);
                }
                printk("\n");
        }
#endif

        if (!(npd->flags & NETWORK_CONFIGURED)) {
                return -EUNATCH;
        }
        if (usbd_get_device_status(function_instance) != USBD_OK) {
                return -EINVAL;
        }

#if defined(CONFIG_OTG_NETWORK_CDC)
        // verify interface is enabled - non-zero altsetting means data is enabled
        // XXX if (!usbd_interface_AltSetting(function_instance, DATA_INTF)) {
        // XXX         return -EINVAL;
        // XXX }
#endif
        in_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, BULK_IN, usbd_high_speed(function_instance));

        if (npd->encapsulation != simple_crc) {
                TRACE_MSG0(NTT,"unknown encapsulation");
                printk(KERN_ERR"%s: unknown encapsulation\n", __FUNCTION__);
                // Since we are now only really using one, just fix it.
                npd->encapsulation = simple_crc;
        }
        //TRACE_MSG0(NTT,"SIMPLE_CRC");
        // allocate urb 5 bytes larger than required
        if (!(urb = usbd_alloc_urb (function_instance, BULK_IN, len + 5 + 4 + in_pkt_sz, net_fd_urb_sent_bulk ))) {
                u8 epa = usbd_endpoint_bEndpointAddress(function_instance, BULK_IN, usbd_high_speed(function_instance));
                TRACE_MSG2(NTT,"urb alloc failed len: %d endpoint: %02x", len, epa);
                printk(KERN_ERR"%s: urb alloc failed len: %d endpoint: %02x\n", __FUNCTION__, len, epa);
                return -ENOMEM;
        }


#if !defined( CONFIG_OTG_NETWORK_BLAN_CRC ) && !defined( CONFIG_OTG_NETWORK_SAFE_CRC  )

        switch (npd->network_type) {
	        case network_eem:
                	urb->actual_length = len + 2;
                break;
	        default:
                	urb->actual_length = len;
                break;
        }
	
        memcpy (urb->buffer, buffer, len);
        urb->function_privdata = data;
        urb->actual_length = len;

		if( urb->actual_length % in_pkt_sz == 0)
		{
			
	       urb->flags |= USBD_URB_SENDZLP;
		   TRACE_MSG2(NTT,"Setting flag for ZLP : %p len: %d",urb,urb->actual_length);			  
		}
		
        if ((rc = usbd_start_in_urb (urb))) {
                TRACE_MSG1(NTT,"FAILED: %d", rc);
                printk(KERN_ERR"%s: FAILED: %d\n", __FUNCTION__, rc);
                urb->function_privdata = NULL;
                usbd_free_urb (urb);
                return rc;
        }
        return 0;
#endif
					
	
        switch (npd->network_type) {
        case network_eem:
                cp = urb->buffer + 2;
                urb->actual_length = len + 2;
                break;
        default:
                cp = urb->buffer;
                urb->actual_length = len;
                break;
        }

#if defined( CONFIG_OTG_NETWORK_BLAN_CRC ) || defined( CONFIG_OTG_NETWORK_SAFE_CRC  )	
        // copy and crc len bytes
        crc = crc32_copy(cp, buffer, len, CRC32_INIT);

        switch (npd->network_type) {
        case network_eem:
                break;
        default:
                if ((urb->actual_length % in_pkt_sz) == (in_pkt_sz - 4)) {

                        #undef CONFIG_OTG_NETWORK_PADBYTE
                        #ifdef CONFIG_OTG_NETWORK_PADBYTE
                        // add a pad byte if required to ensure a short packet, usbdnet driver
                        // will correctly handle pad byte before or after CRC, but the MCCI driver
                        // wants it before the CRC.
                        crc = crc32_pad(urb->buffer + urb->actual_length, 1, crc);
                        urb->actual_length++;
                        #else /* CONFIG_OTG_NETWORK_PADBYTE */
                        urb->flags |= USBD_URB_SENDZLP; 
                        TRACE_MSG2(NTT,"setting ZLP: urb: %p flags: %x", urb, urb->flags);
                        #endif /* CONFIG_OTG_NETWORK_PADBYTE */
                }
                break;
        }
        // munge and append crc
        crc = ~crc;
        urb->buffer[urb->actual_length++] = crc & 0xff;
        urb->buffer[urb->actual_length++] = (crc >> 8) & 0xff;
        urb->buffer[urb->actual_length++] = (crc >> 16) & 0xff;
        urb->buffer[urb->actual_length++] = (crc >> 24) & 0xff;
        // End of CRC processing
        //
#else
	memcpy (cp, buffer, len);
#endif
	
        switch (npd->network_type) {
        case network_eem:
                break;
        default:
                break;
        }

        TRACE_MSG3(NTT,"urb: %p buf: %p priv: %p", urb, data, urb->function_privdata);
        urb->function_privdata = data;
#if 0
        printk(KERN_INFO"start_xmit: len: %d : %d data: %p\n", skb->len, urb->actual_length, urb->buffer);
        {
                u8 *cp = urb->buffer;
                int i;
                for (i = 0; i < urb->actual_length; i++) {
                        if ((i%32) == 0) {
                                printk("\ntx[%2x] ", i);
                        }
                        printk("%02x ", *cp++);
                }
                printk("\n");
        }
#endif
#if defined(CONFIG_OTG_NETWORK_BLAN_FERMAT)
        if (npd->fermat) {
                fermat_encode(urb->buffer, urb->actual_length);
        }
#endif
        TRACE_MSG1(NTT,"sending urb: %p", urb);
        if ((rc = usbd_start_in_urb (urb))) {

                TRACE_MSG1(NTT,"FAILED: %d", rc);
                printk(KERN_ERR"%s: FAILED: %d\n", __FUNCTION__, rc);
                urb->function_privdata = NULL;
                usbd_free_urb (urb);

                return rc;
        }
        #if 0
        {
                static int xmit_count = 0;
                if (xmit_count++ == 100) {
                        TRACE_MSG1(NTT, "halt test: %02x", BULK_IN);
                        usbd_halt_endpoint(function_instance, BULK_IN);
                }
        }
        #endif

        TRACE_MSG0(NTT,"OK");
        return 0;
}


//_________________________________________________________________________________________________

/*! net_fd_recv_urb - callback to process a received URB
 *
 * @return non-zero for failure.
 */
STATIC int net_fd_recv_urb(struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct usb_network_private *npd = urb->function_privdata;
        void *data = NULL;
        u8 *buffer;
        int crc_bad = 0;
        int trim = 0;
        int len;
        int out_pkt_sz;
        u32 crc;

#if 0
        printk(KERN_INFO"%s: urb: %p len: %d maxtransfer: %d encap: %d\n", __FUNCTION__, 
                        urb, urb->actual_length, npd->maxtransfer, npd->encapsulation);

        {
                u8 *cp = urb->buffer;
                int i;
                for (i = 0; i < urb->actual_length; i++) {
                        if ((i%32) == 0) {
                                printk("\n[%2x] ", i);
                        }
                        printk("%02x ", *cp++);
                }
                printk("\n");
        }
#endif

        TRACE_MSG2(NTT, "status: %d actual_length: %d", urb->status, urb->actual_length);

        THROW_IF(urb->status != USBD_URB_OK, error);

        out_pkt_sz = usbd_endpoint_wMaxPacketSize(function_instance, BULK_OUT, usbd_high_speed(function_instance));
        // There is only one working encapsulation.
        if (npd->encapsulation != simple_crc) {
                 npd->encapsulation = simple_crc;
        }

        len = urb->actual_length;
        trim = 0;

        THROW_UNLESS((data = net_os_alloc_buffer(function_instance, &buffer, len)), error);
        //TRACE_MSG2(NTT, "data: %x buffer: %x", data, buffer);


#if defined(CONFIG_OTG_NETWORK_BLAN_PADAFTER)
        {
                /* This version simply checks for a correct CRC along the 
                 * entire packet. Some UDC's have trouble with some packet
                 * sizes, this allows us to add pad bytes after the CRC.
                 */

                u8 *src = urb->buffer;
                int copied;

                // XXX this should work, but the MIPS optimizer seems to get it wrong....
                //copied = (len < out_pkt_sz) ? 0 : ((len / out_pkt_sz) - 1) * out_pkt_sz;
                        
                if (len < out_pkt_sz*2) 
                        copied = 0;
                else {
                        int pkts = ((len - out_pkt_sz) / out_pkt_sz);
                        copied = (pkts - 1) * out_pkt_sz;
                }

                len -= copied;
                crc = CRC32_INIT;
                for (; copied-- > 0 ; crc = COMPUTE_FCS (crc, *buffer++ = *src++));

                for (; (len-- > 0) && (CRC32_GOOD != crc); crc = COMPUTE_FCS (crc, *buffer++ = *src++));

                trim = len + 4;

                if (CRC32_GOOD != crc) {
                        TRACE_MSG1(NTT,"AAA frame: %03x", urb->framenum);
                        THROW_IF(npd->crc, crc_error);
                }
                else 
                        npd->crc = 1;
        }
#else
        /* 
         * The CRC can be sent in two ways when the size of the transfer 
         * ends up being a multiple of the packetsize:
         *
         *                                           |
         *                <data> <CRC><CRC><CRC><CRC>|<???>     case 1
         *                <data> <NUL><CRC><CRC><CRC>|<CRC>     case 2
         *           <data> <NUL><CRC><CRC><CRC><CRC>|          case 3
         *     <data> <NUL><CRC><CRC><CRC>|<CRC>     |          case 4
         *                                           |
         *        
         * This complicates CRC checking, there are four scenarios:
         *
         *      1. length is 1 more than multiple of packetsize with a trailing byte
         *      2. length is 1 more than multiple of packetsize 
         *      3. length is multiple of packetsize
         *      4. none of the above
         *
         * Finally, even though we always compute CRC, we do not actually throw
         * things away until and unless we have previously seen a good CRC.
         * This allows backwards compatibility with hosts that do not support
         * adding a CRC to the frame.
         *
         */

        // test if 1 more than packetsize multiple
        if (1 == (len % out_pkt_sz)) {
#if defined( CONFIG_OTG_NETWORK_BLAN_CRC ) || defined( CONFIG_OTG_NETWORK_SAFE_CRC  )
                // copy and CRC up to the packetsize boundary
                crc = crc32_copy(buffer, urb->buffer, len - 1, CRC32_INIT);
                buffer += len - 1;

                // if the CRC is good then this is case 1
                if (CRC32_GOOD != crc) {

                        crc = crc32_copy(buffer, urb->buffer + len - 1, 1, crc);
                        buffer += 1;

                        if (CRC32_GOOD != crc) {
                                //crc_errors[len%64]++;
                                TRACE_MSG2(NTT,"A CRC error %08x %03x", crc, urb->framenum);
                                THROW_IF(npd->crc, crc_error);
                        }
                        else 
                                npd->crc = 1;
                }
                else 
                        npd->crc = 1;
#endif		
        }
        else {
		
#if defined( CONFIG_OTG_NETWORK_BLAN_CRC ) || defined( CONFIG_OTG_NETWORK_SAFE_CRC  )	
                crc = crc32_copy(buffer, urb->buffer, len, CRC32_INIT);
                buffer += len;

                if (CRC32_GOOD != crc) {
                        //crc_errors[len%64]++;
                        TRACE_MSG2(NTT,"B CRC error %08x %03x", crc, urb->framenum);
                        THROW_IF(npd->crc, crc_error);
                }
                else 
                        npd->crc = 1;
#else
		memcpy (buffer, urb->buffer, len);
#endif		
        }
	// trim IFF we are paying attention to crc
#if defined ( CONFIG_OTG_NETWORK_BLAN_CRC) || defined( CONFIG_OTG_NETWORK_SAFE_CRC  )
	if (npd->crc) 
                trim = 4;
#endif	
#endif
        if (net_os_recv_buffer(function_instance, data, crc_bad, trim)) {
                TRACE_MSG0(NTT, "FAILED");
                net_os_dealloc_buffer(function_instance, data);
        }

        // catch a simple error, just increment missed error and general error
        CATCH(error) {
                //TRACE_MSG4(NTT,"CATCH(error) urb: %p status: %d len: %d function: %p",
                //                urb, urb->status, urb->actual_length, function_instance);
                // catch a CRC error
                CATCH(crc_error) {
                        crc_bad = 1;
#if 0
                        printk(KERN_INFO"%s: urb: %p status: %d len: %d maxtransfer: %d encap: %d\n", __FUNCTION__, 
                                        urb, urb->status, urb->actual_length, npd->maxtransfer, 
                                        npd->encapsulation);

                        {
                                u8 *cp = urb->buffer;
                                int i;
                                for (i = 0; i < urb->actual_length; i++) {
                                        if ((i%32) == 0) {
                                                printk("\n[%2x] ", i);
                                        }
                                        printk("%02x ", *cp++);
                                }
                                printk("\n");
                        }
#endif
                }
        }

        //TRACE_MSG1(NTT,"restart: %p", urb);
        return usbd_start_out_urb (urb);
}

/*! net_fd_recv_urb_eem - callback to process a received URB
 *
 * @return non-zero for failure.
 */
STATIC int net_fd_recv_urb_eem(struct usbd_urb *urb, int rc)
{
        return 0;
}

//_________________________________________________________________________________________________
//                                      net_fd_device_request
//
/*! net_fd_urb_received_ep0 - callback for sent URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @return non-zero for failure.
 */
int net_fd_urb_received_ep0 (struct usbd_urb *urb, int urb_rc)
{
        TRACE_MSG2(NTT,"urb: %p status: %d", urb, urb->status);

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        RETURN_EINVAL_IF (USBD_URB_OK != urb->status);

        // TRACE_MSG1(NTT,"%s", urb->buffer); // QQSV  is this really a NUL-terminated string???

        //printk(KERN_INFO"%s: ok\n", __FUNCTION__);
        return -EINVAL;         // caller will de-allocate
}

/*! net_fd_device_request - process a received SETUP URB
 *
 * Processes a received setup packet and CONTROL WRITE data.
 * Results for a CONTROL READ are placed in urb->buffer.
 *
 * @return non-zero for failure.
 */
int net_fd_device_request (struct usbd_function_instance *function_instance, struct usbd_device_request *request)
{
        struct usb_network_private *npd = function_instance->privdata;
        struct usbd_urb *urb;
        int index;

        // Verify that this is a USB Class request per CDC specification or a vendor request.
        RETURN_ZERO_IF (!(request->bmRequestType & (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_VENDOR)));

        // Determine the request direction and process accordingly
        switch (request->bmRequestType & (USB_REQ_DIRECTION_MASK | USB_REQ_TYPE_MASK)) {

        case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_VENDOR:

                switch (request->bRequest) {
                case MCCI_ENABLE_CRC:
                        if (make_crc_table()) 
                                return -EINVAL;
                        npd->encapsulation = simple_crc;
                        return 0;

                case BELCARRA_PING:
                        TRACE_MSG1(NTT,"H2D VENDOR IP: %08x", ip_addr);
                        if ((npd->network_type == network_blan)) 
                                net_os_send_notification_later(function_instance);
                        break;

                case BELCARRA_SETIP:
                        #ifdef OTG_BIG_ENDIAN
                        ip_addr = ntohl( request->wValue | request->wIndex<< 16 );
                        #else /* OTG_BIG_ENDIAN */
                        ip_addr = ntohl( request->wValue << 16 | request->wIndex);
                        #endif /* OTG_BIG_ENDIAN */
                        TRACE_MSG1(NTT, "ip_addr: %08x", ip_addr);
                        // XXX need to get in correct order here
                        UNLESS(npd->local_dev_set) {
                                npd->local_dev_addr[0] = 0xfe | 0x02;           /* locally administered */
                                npd->local_dev_addr[1] = 0x00;
                                npd->local_dev_addr[2] = (ip_addr >> 24) & 0xff;
                                npd->local_dev_addr[3] = (ip_addr >> 16) & 0xff;
                                npd->local_dev_addr[4] = (ip_addr >> 8) & 0xff;
                                npd->local_dev_addr[5] = (ip_addr >> 0) & 0xff;
                        }
#if 1

//#ifdef CONFIG_OTG_NETWORK_BLAN_IPADDR
                        snprintf(ip_addr_str, sizeof(ip_addr_str), "IP=%d.%d.%d.%d", 
                                        (ip_addr >> 24) & 0xff, (ip_addr >> 16) & 0xff,
                                        (ip_addr >> 8) & 0xff, (ip_addr) & 0xff);
                        index = usbd_realloc_string(function_instance,STRINDEX_IPADDR,ip_addr_str);
//#endif /* CONFIG_OTG_NETWORK_IPADDR */

#endif 
                        break;

                case BELCARRA_SETMSK:
                        #ifdef OTG_BIG_ENDIAN
                        network_mask = ntohl( request->wValue | request->wIndex << 16);
                        #else /* OTG_BIG_ENDIAN */
                        network_mask = ntohl( request->wValue << 16 | request->wIndex);
                        #endif /* OTG_BIG_ENDIAN */
                        TRACE_MSG1(NTT, "network_mask: %08x", network_mask);
                        break;

                case BELCARRA_SETROUTER:
                        #ifdef OTG_BIG_ENDIAN
                        router_ip = ntohl( request->wValue | request->wIndex << 16);
                        #else /* OTG_BIG_ENDIAN */
                        router_ip = ntohl( request->wValue << 16 | request->wIndex);
                        #endif /* OTG_BIG_ENDIAN */
                        TRACE_MSG1(NTT, "router_ip: %08x", router_ip);
                        break;

                case BELCARRA_SETDNS:
                        #ifdef OTG_BIG_ENDIAN
                        dns_server_ip = ntohl( request->wValue | request->wIndex < 16);
                        #else /* OTG_BIG_ENDIAN */
                        dns_server_ip = ntohl( request->wValue << 16 | request->wIndex);
                        #endif /* OTG_BIG_ENDIAN */
                        break;
#ifdef CONFIG_OTG_NETWORK_BLAN_FERMAT
                case BELCARRA_SETFERMAT:
                        npd->fermat = 1;
                        break;
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_HOSTNAME
                case BELCARRA_HOSTNAME:
                        TRACE_MSG0(NTT,"HOSTNAME");
                        RETURN_EINVAL_IF(!(urb = usbd_alloc_urb_ep0(function_instance, le16_to_cpu(request->wLength), 
                                                        net_fd_urb_received_ep0) ));
                        RETURN_ZERO_IF(!usbd_start_out_urb(urb));                  // return if no error
                        usbd_free_urb(urb);                                  // de-alloc if error
                        return -EINVAL;
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_DATA_NOTIFY_OK
                case BELCARRA_DATA_NOTIFY:
                        TRACE_MSG0(NTT,"DATA NOTIFY");
                        npd->data_notify = 1;
                        return -EINVAL;

								    
#endif
				default:
                        return -EINVAL;					
									
                }
                switch (request->bRequest) {
                case BELCARRA_SETIP:
                case BELCARRA_SETMSK:
                case BELCARRA_SETROUTER:
                        TRACE_MSG5(NTT, "network_mask: %08x ip_addr: %08x router_ip: %08x flags: %08x %s",
                                        network_mask, ip_addr, router_ip, npd->flags, 
                                        (npd->flags & NETWORK_CONFIGURED) ? "CONFIGURED" : "");

                        BREAK_UNLESS (network_mask && ip_addr && router_ip && (npd->flags & NETWORK_CONFIGURED));
                        // let the os layer know, if it's interested.
                        net_os_config(function_instance);
                        net_os_hotplug(function_instance);
                        break;
                }
                return 0;
#if 0
        case usb_req_device2host | usb_req_type_vendor:
                urb->actual_length = 0;
                switch (request->brequest) {
                case belcarra_getmac:
                        {
                                // copy and free the original buffer
                                memcpy(urb->buffer, npd->local_dev_addr, eth_alen);
                                urb->actual_length = eth_alen;
                                return 0;
                        }
                }
		return 0;
#endif
        default:
                break;
        }
        return -EINVAL;
}
//_________________________________________________________________________________________________

#ifdef CONFIG_OTG_NETWORK_START_SINGLE
#define NETWORK_START_URBS 1
#else
#define NETWORK_START_URBS 2
#endif

typedef enum mesg {
        mesg_unknown,
        mesg_configured,
        mesg_reset,
} mesg_t;
mesg_t net_last_mesg;

char * net_messages[3] = {
        "",
        "Network Configured",
        "Network Reset",
};

/*! net_check_mesg
 */
void net_check_mesg(mesg_t curr_mesg)
{
        RETURN_UNLESS(net_last_mesg != curr_mesg);
        net_last_mesg = curr_mesg;
        otg_message(net_messages[curr_mesg]);
}



/*! net_fd_start_recv - start recv urb(s)
 */
STATIC void net_fd_start_recv(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        int i;
        int network_start_urbs = NETWORK_START_URBS;
        int hs = usbd_high_speed(function_instance);

        if (hs)
                network_start_urbs = NETWORK_START_URBS * 3;


        for (i = 0; i < network_start_urbs; i++) {
                struct usbd_urb *urb;
                BREAK_IF(!(urb = usbd_alloc_urb(function_instance, BULK_OUT, 
                                                usbd_endpoint_transferSize(function_instance, BULK_OUT, hs), 
                                                net_fd_recv_urb)));
                TRACE_MSG5(NTT,"i: %d urb: %p priv: %p npd: %p function: %p",
                           i, urb, urb->function_privdata, npd, function_instance);
                
                urb->function_privdata = npd;
                if (usbd_start_out_urb(urb)) {
                        urb->function_privdata = NULL;
                        usbd_free_urb(urb);
                }
        }
}

/*! net_fd_start_recv_eem - start recv urb(s)
 */
STATIC void net_fd_start_recv_eem(struct usbd_function_instance *function_instance)
{
}


/*! net_fd_endpoint_cleared - 
 */
void net_fd_endpoint_cleared (struct usbd_function_instance *function_instance, int bEndpointAddress)
{
        TRACE_MSG1(NTT, "bEndpointAddress: %02x", bEndpointAddress);
}


//_________________________________________________________________________________________________

/*! hexdigit -
 *
 * Converts characters in [0-9A-F] to 0..15, characters in [a-f] to 42..47, and all others to 0.
 */
static u8 hexdigit (char c)
{
        return isxdigit (c) ? (isdigit (c) ? (c - '0') : (c - 'A' + 10)) : 0;
}

/*! set_address -
 */
static void set_address(char *mac_address_str, u8 *dev_addr)
{
        int i;
        if (mac_address_str && strlen(mac_address_str)) {
                for (i = 0; i < ETH_ALEN; i++) {
                        dev_addr[i] = 
                                hexdigit (mac_address_str[i * 2]) << 4 | 
                                hexdigit (mac_address_str[i * 2 + 1]);
                }
        }
        else {
                get_random_bytes(dev_addr, ETH_ALEN);
                dev_addr[0] = (dev_addr[0] & 0xfe) | 0x02;
        }
}



/*! net_fd_function_enable - enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 */

int net_fd_function_enable (struct usbd_function_instance *function_instance, network_type_t network_type)
{
        struct usb_network_private *npd = NULL;

        // XXX MODULE LOCK HERE
        
        THROW_UNLESS((npd = CKMALLOC(sizeof(struct usb_network_private), GFP_KERNEL)), error);

        function_instance->privdata = npd;
        npd->network_type = network_type;
        
        TRACE_MSG0(NTT,"semaphore DOWN");
        net_os_mutex_enter(function_instance);

        net_os_enable(function_instance);

        if (local_dev_addr_str && strlen(local_dev_addr_str)) {
                set_address(local_dev_addr_str, npd->local_dev_addr);
                npd->local_dev_set = TRUE;
        }
        if (remote_dev_addr_str && strlen(remote_dev_addr_str)) {
                set_address(remote_dev_addr_str, npd->remote_dev_addr);
                npd->remote_dev_set = TRUE;
        }

        //_MOD_INC_USE_COUNT;  // QQQ Should this be _before_ the mutex_enter()?
        //TRACE_MSG1(NTT, "INC: %d", MOD_IN_USE);

        // set the network device address from the local device address
        // Now done in net_os_enable() above.
        // memcpy(npd->net_device->dev_addr, npd->local_dev_addr, ETH_ALEN);

        npd->have_interrupt = usbd_endpoint_bEndpointAddress(function_instance, INT_IN, 
                        usbd_high_speed(function_instance)) ? 1 : 0;

        npd->flags |= NETWORK_ENABLED;

        net_os_mutex_exit(function_instance);
        TRACE_MSG0(NTT,"semaphore UP");
        CATCH(error) {
                // XXX MODULE UNLOCK HERE
                return -EINVAL;
        }
        return 0;
}

/*! net_fd_function_disable - disable the function driver
 *
 */
void net_fd_function_disable (struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;
        TRACE_MSG0(NTT,"semaphore DOWN");
        net_os_mutex_enter(function_instance);
        function_instance->privdata = NULL;
        npd->flags &= ~NETWORK_ENABLED;
        net_os_mutex_exit(function_instance);
        TRACE_MSG0(NTT,"semaphore UP");

        while (PENDING_WORK_ITEM(npd->notification_bh)) {
                // TRACE_MSG pointless here, module will be gon before we can look at it.
                printk(KERN_ERR"%s: waiting for notificationhotplug bh\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }

#ifdef CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG
        while (PENDING_WORK_ITEM(npd->config_bh)) {
                printk(KERN_ERR"%s: waiting for config bh\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }
#endif /* CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG */

#ifdef CONFIG_OTG_NETWORK_HOTPLUG
        while (PENDING_WORK_ITEM(npd->hotplug_bh)) {
                printk(KERN_ERR"%s: waiting for hotplug bh\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }
#endif
        LKFREE(npd);
        // XXX MODULE UNLOCK HERE
}

//_________________________________________________________________________________________________

/*! 
 * net_fd_set_configuration - called to indicate urb has been received
 * @param function_instance
 * @param configuration
 * @return int
 */
int net_fd_set_configuration (struct usbd_function_instance *function_instance, int configuration)
{
        struct usb_network_private *npd = function_instance->privdata;

        //printk(KERN_INFO"%s: \n", __FUNCTION__);
        //struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;

	TRACE_MSG2(NTT, "CONFIGURED: %d ipaddr: %08x", configuration, ip_addr);

        net_check_mesg(mesg_configured);
        npd->flags |= NETWORK_CONFIGURED;

        if ((npd->network_type == network_blan) && (npd->flags & NETWORK_OPEN)) 
                net_os_send_notification_later(function_instance);
        net_os_carrier_on(function_instance);
        (npd->network_type == network_eem) ?  net_fd_start_recv_eem : net_fd_start_recv(function_instance);

        // Let the OS layer know, if it's interested.
        net_os_config(function_instance);
        net_os_hotplug(function_instance);

	TRACE_MSG1(NTT, "CONFIGURED npd->flags: %04x", npd->flags);
	return 0;
}



/*! 
 * @brief net_fd_set_interface - called to indicate urb has been received
 * @param function_instance
 * @param wIndex
 * @param altsetting
 * @return int
 */
int net_fd_set_interface (struct usbd_function_instance *function_instance, int wIndex, int altsetting)
{
        //printk(KERN_INFO"%s: \n", __FUNCTION__);
        //struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
	TRACE_MSG0(NTT, "SET INTERFACE");

	return 0;
}

/*! 
 * @brief  net_fd_reset - called to indicate urb has been received
 * @param function_instance
 * @return int
 */
int net_fd_reset (struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = function_instance->privdata;

        TRACE_MSG1(NTT,"RESET/DESTROY/BUS_INACTIVE/DE_CONFIGURED %08x",ip_addr);
        net_check_mesg(mesg_reset);
        {
                // Return if argument is null.

                // XXX flush

                npd->flags &= ~NETWORK_CONFIGURED;
                npd->int_urb = NULL;

                // Disable our net-device.
                // Apparently it doesn't matter if we should do this more than once.

                net_os_carrier_off(function_instance);

                // If we aren't already tearing things down, do it now.
                if (!(npd->flags & NETWORK_DESTROYING)) {
                        npd->flags |= NETWORK_DESTROYING;
                        //npd->device = NULL;
                }
        }
        npd->crc = 0;

        // Let the OS layer know, if it's interested.
        net_os_config(function_instance);
        net_os_hotplug(function_instance);


	// XXX flush
	// Release any queued urbs
	TRACE_MSG1(NTT, "RESET npd->flags: %04x", npd->flags);
	return 0;
}


/*! 
 * @brief net_fd_suspended - called to indicate urb has been received
 * @param function_instance
 * @return int
 */
int net_fd_suspended (struct usbd_function_instance *function_instance)
{
        //printk(KERN_INFO"%s: \n", __FUNCTION__);

	TRACE_MSG0(NTT, "SUSPENDED");
	return 0;
}


/*! 
 * net_fd_resumed - called to indicate urb has been received
 * @param function_instance
* @return int
 */
int net_fd_resumed (struct usbd_function_instance *function_instance)
{
        //struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;

        //printk(KERN_INFO"%s: \n", __FUNCTION__);
	TRACE_MSG0(NTT, "RESUMED");

	return 0;
}



//______________________________________module_init and module_exit________________________________

/*! macstrtest -
 */
static int macstrtest(char *mac_address_str)
{
        int l = 0;

        if (mac_address_str) {
                l = strlen(mac_address_str);
        }
        return ((l != 0) && (l != 12));
}

/*!
 * @brief net_fd_init - function driver usb part intialization
 *
 * @param info_str
 * @param local
 * @param remote
 * @return non-zero for failure.
 */
int net_fd_init(char *info_str, char *local, char *remote)
{
        local_dev_addr_str = local;
        remote_dev_addr_str = remote;
        TRACE_MSG2(NTT, "local: %s remote: %s", 
                        local_dev_addr_str ? local_dev_addr_str : "",
                        remote_dev_addr_str ? remote_dev_addr_str : ""
                        );





#ifdef CONFIG_OTG_NETWORK_BLAN_FERMAT
        TRACE_MSG0(NTT,"fermat");
        fermat_init();
#endif

        #if 0
#ifdef CONFIG_OTG_NETWORK_EP0TEST
        /*
         * ep0test - test that bus interface can do ZLP on endpoint zero
         *
         * This will artificially force iProduct string descriptor to be
         * exactly the same as the endpoint zero packetsize.  When the host
         * requests this string it will request it not knowing the strength
         * and will use a max length of 0xff. The bus interface driver must
         * send a ZLP to terminate the transaction.
         *
         * The iProduct descriptor is used because both the Linux and
         * Windows usb implmentations fetch this in a default enumeration. 
         *
         */
        //UNLESS (ep0test) 
        //        ep0test = usbd_endpoint_zero_wMaxPacketsize(function);

        if (ep0test) {
                switch (ep0test) {
                case 8:  npd->simple_driver->device_description->iProduct = "012"; break;
                case 16: npd->simple_driver->device_description->iProduct = "0123456"; break;
                case 32: npd->simple_driver->device_description->iProduct = "0123456789abcde"; break;
                case 64: npd->simple_driver->device_description->iProduct = "0123456789abcdef0123456789abcde"; break;
                default: printk(KERN_ERR"%s: ep0test: bad value: %d, must be one of 8, 16, 32 or 64\n", 
                                         __FUNCTION__, ep0test); return -EINVAL;
                         break;
                }
                TRACE_MSG1(NTT,"ep0test: iProduct set to: %s",
                           npd->simple_driver->device_description->iProduct);
                printk(KERN_INFO"%s: ep0test: iProduct set to: %s\n", __FUNCTION__, 
                                npd->simple_driver->device_description->iProduct);
        }
        else {
                TRACE_MSG0(NTT,"ep0test: not set");
                printk(KERN_INFO"%s: ep0test: not set\n", __FUNCTION__);
        }
#endif /* CONFIG_OTG_NETWORK_EP0TEST */


#ifdef CONFIG_OTG_NETWORK_LOCAL_MACADDR
        if (NULL == p->local_mac_address_str) {
                /* There is a configured override, and it has NOT been
                   overridden in turn by a module load time parameter, so use it. */
                p->local_mac_address_str = CONFIG_OTG_NETWORK_LOCAL_MACADDR;
        }
#endif
#ifdef CONFIG_OTG_NETWORK_REMOTE_MACADDR
        if (NULL == p->remote_mac_address_str) {
                /* There is a configured override, and it has NOT been
                   overridden in turn by a module load time parameter, so use it. */
                p->remote_mac_address_str = CONFIG_OTG_NETWORK_REMOTE_MACADDR;
        }
#endif
        if ((macstrtest(p->local_mac_address_str) || macstrtest(p->remote_mac_address_str))) {
                TRACE_MSG2(NTT,"bad size %s %s", p->local_mac_address_str, p->remote_mac_address_str);
                return -EINVAL;
        }

        set_address(p->local_mac_address_str, npd->local_dev_addr);
        set_address(p->remote_mac_address_str, npd->remote_dev_addr);
        npd->encapsulation = simple_crc;
        
        #endif
        return make_crc_table();
}

//_____________________________________________________________________________

/*! 
 * @brief net_fd_exit - driver exit
 *
 * Cleans up the module. Deregisters the function driver and destroys the network object.
 */
void net_fd_exit(void)
{
        if (network_crc32_table) {
                lkfree(network_crc32_table);
                network_crc32_table = NULL;
        }
}

