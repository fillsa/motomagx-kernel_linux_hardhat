/*
 * otg/isotest_fd/host.c
 * @(#) balden@seth2.belcarratech.com|otg/functions/isotest/host.c|20051116204957|17905
 *
 *      Copyright (c) 2003-2004 Belcarra
 *
 *      Copyright (c)  2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         dn Initial distribution
 * 10/18/2006         Motorola         ma sk pw Add Open Src Software language
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

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
 * USB ISO Test
 *
 */

//#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>

#include <linux/kernel.h>
//#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
//#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/usb.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>

#include <linux/vmalloc.h>

#include <asm/atomic.h>
#include <asm/io.h>


#if defined(CONFIG_ARCH_SA1100) || defined (CONFIG_ARCH_PXA)
#include <asm/dma.h>
#include <asm/mach/dma.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/types.h>
#endif

#if defined(CONFIG_MIPS_AU1000) || defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
#include <asm/au1000.h>
#include <asm/au1000_dma.h>
#include <asm/mipsregs.h>
#endif

#if defined(CONFIG_ARCH_SAMSUNG)
#include <asm/arch/timers.h>
#include <asm/arch/hardware.h>
#endif

#include "test.h"


/* Use our own dbg macro */
#undef dbg
#define dbg(format, arg...) do { if (debug) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg); } while (0)

#define MIN(a,b) (((a) < (b))?(a):(b))
#define MAX(a,b) (((a) > (b))?(a):(b))

#define THROW(x) goto x
#define CATCH(x) while(0) x:
#define THROW_IF(e, x) if (e) { goto x; }
#define BREAK_IF(x) if (x) { break; }
#define CONTINUE_IF(x) if (x) { continue; }
#define RETURN_IF(y) if (y) { return; }
#define RETURN_ZERO_IF(y) if (y) { return 0; }
#define RETURN_NULL_IF(y) if (y) { return NULL; }


/* Version Information */
#define DRIVER_VERSION "v0.9"
#define DRIVER_AUTHOR "sl@belcarra.com"
#define DRIVER_DESC "USB ISO Test"

/* Define these values to match your device */

#ifdef CONFIG_OTG_ISOTEST_VENDORID
        #undef USB_ISOTEST_VENDOR_ID
        #define USB_ISOTEST_VENDOR_ID   CONFIG_OTG_ISOTEST_VENDORID
#else
        #define USB_ISOTEST_VENDOR_ID   0xfff0
#endif

#ifdef CONFIG_OTG_ISOTEST_PRODUCTID
        #undef USB_ISOTEST_PRODUCT_ID
        #define USB_ISOTEST_PRODUCT_ID  CONFIG_OTG_ISOTEST_PRODUCTID
#else
        #define USB_ISOTEST_PRODUCT_ID  0xfff1
#endif

/* Module paramaters */
//MODULE_PARM(debug, "i");
//MODULE_PARM_DESC(debug, "Debug enabled or not");

static int send;
MODULE_PARM(send, "i");
MODULE_PARM_DESC(send, "send test");

static int recv;
MODULE_PARM(recv, "i");
MODULE_PARM_DESC(recv, "recv test");

static u32    vendor_id;         // no default
static u32    product_id;        // no default


MODULE_PARM_DESC(vendor_id, "User specified USB idVendor");
MODULE_PARM_DESC(product_id, "User specified USB idProduct");
MODULE_PARM(vendor_id, "i");
MODULE_PARM(product_id, "i");


/* table of devices that work with this driver */
static struct usb_device_id isotest_table [] = {
        { USB_DEVICE(USB_ISOTEST_VENDOR_ID, USB_ISOTEST_PRODUCT_ID) },
        { },                            /* extra entry */
        { },                            /* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, isotest_table);

/* ********************************************************************************************* */


/* ********************************************************************************************* */
/*
struct iso_test_data {

        // sender info
        u32     sender_id;
        time_t  send_time;
        u32     send_crc;

        // loop info
        u32     recv_id;
        time_t  recv_time;
        u32     recv_crc;

        // payload
        u32     size;
        u8      data[0];
};
*/

#define IN_URBS 10
#define OUT_URBS 10




/* Structure to hold all of our device specific stuff */
struct usb_isotest {
        struct usb_device *     udev;           /* save off the usb device pointer */
        struct usb_interface *  interface;      /* the interface for this device */

        u8                    closing;
        u8                    num_interrupt_in;       /* number of interrupt in endpoints we have */
        u8                    num_iso_in;             /* number of iso in endpoints we have */
        u8                    num_iso_out;            /* number of iso out endpoints we have */

        int                     iso_in_size;            /* the size of the receive buffer */
        struct urb *            iso_in_urbs[IN_URBS];   /* the urb used to send data */
        u8                    iso_in_endpointAddr;    /* the address of the iso in endpoint */
        int                     in_urbs;

        int                     iso_out_size;           /* the size of the send buffer */
        struct urb *            iso_out_urbs[OUT_URBS]; /* the urb used to send data */
        u8                    iso_out_endpointAddr;   /* the address of the iso out endpoint */
        int                     out_urbs;

        struct semaphore        sem;                    /* locks this structure */
        struct tq_struct        iso_bh;
        wait_queue_head_t       iso_wq;

        struct isotest_stats    stats;

        int                     first;
};


/* local function prototypes */

static void * isotest_probe     (struct usb_device *, unsigned int , const struct usb_device_id *);
static void isotest_disconnect  (struct usb_device *, void *);


void isotest_schedule_bh(struct usb_isotest *isotest);


/* ISO OUT - Transmit ************************************************************************** */


#if 1
static void isotest_iso_out_free_urb(struct urb *urb)
{
        struct usb_isotest *isotest;
        int i;
        unsigned long   flags;

        //printk(KERN_INFO"%s: urb: %p\n", __FUNCTION__, urb);

        RETURN_IF(!urb);

        if (urb->transfer_buffer) {
                kfree(urb->transfer_buffer);
        }

        isotest = (struct usb_isotest *)urb->context;
        usb_free_urb(urb);

        RETURN_IF(!isotest);

        local_irq_save (flags);
        for (i = 0; i < OUT_URBS; i++) {
                CONTINUE_IF(isotest->iso_out_urbs[i] != urb);
                //printk(KERN_INFO"%s: zeroing %d urb: %p\n", __FUNCTION__, i, urb);
                isotest->iso_out_urbs[i] = NULL;
                break;
        }
        local_irq_restore (flags);
}
#endif

static int iso_transfer_count;

int iso_out_submit(struct usb_isotest *isotest, struct urb *urb) 
{
        int                     i;
        int                     j;
        int                     rc = 0;

        //printk(KERN_INFO"%s: transfer: %d size: %d frames: %x\n", __FUNCTION__, 
        //                iso_transfer_count, urb->transfer_buffer_length, urb->number_of_packets);

        RETURN_ZERO_IF(isotest->closing);

        urb->dev = isotest->udev;

        iso_transfer_count++;

        for (j = urb->transfer_buffer_length, i = 0; i < urb->number_of_packets; i++) {

                int                     send = MIN(isotest->iso_out_size, j);

                u8                      *cp = urb->transfer_buffer + (isotest->iso_out_size * i);

                j -= send;

                urb->iso_frame_desc[i].offset = i * isotest->iso_out_size;
                urb->iso_frame_desc[i].length = send;

                // iso_transfer_count
                *cp++ = cpu_to_le16(iso_transfer_count) & 0xff;
                *cp++ = (cpu_to_le16(iso_transfer_count) >> 8) & 0xff;
                *cp++ = (cpu_to_le16(iso_transfer_count) >> 16) & 0xff;
                *cp++ = (cpu_to_le16(iso_transfer_count) >> 24) & 0xff;

                // iso transfer length
                *cp++ = cpu_to_le16(urb->transfer_buffer_length) & 0xff;
                *cp++ = (cpu_to_le16(urb->transfer_buffer_length) >> 8) & 0xff;

                // iso frame size
                *cp++ = cpu_to_le16(isotest->iso_out_size) & 0xff;
                *cp++ = (cpu_to_le16(isotest->iso_out_size) >> 8) & 0xff;

                // total frames
                *cp++ = cpu_to_le16(urb->number_of_packets) & 0xff;
                *cp++ = (cpu_to_le16(urb->number_of_packets) >> 8) & 0xff;

                // this packet number
                *cp++ = cpu_to_le16(i+1) & 0xff;
                *cp++ = (cpu_to_le16(i+1) >> 8) & 0xff;

        }

        //printk(KERN_INFO"%s: submitting\n", __FUNCTION__);

        RETURN_ZERO_IF(!(rc = usb_submit_urb(urb)));
        printk(KERN_INFO"%s: FAILED rc: %x\n", __FUNCTION__, rc);

        return rc;
}

void isotest_iso_out_complete (struct urb *urb)
{
        struct usb_isotest *isotest = (struct usb_isotest *)urb->context;
        int rc;

        //printk(KERN_INFO"%s: urb: %p\n", __FUNCTION__, urb);

        RETURN_IF(!urb);

        if (urb->status /* && (urb->status != -ENOENT) && (urb->status != -ECONNRESET)*/) {
                //printk(KERN_INFO"%s: - nonzero write iso status received: %d\n", __FUNCTION__, urb->status);
        }

        iso_out_submit(isotest, urb);
}


#define ISO_SEND_TOTAL  1000
#define ISO_SEND_TRANSFERS 1200

static int out_count;

struct urb *iso_out_start(struct usb_isotest *isotest)
{
        struct urb *            urb = NULL;
        int                     rc = 0;
        int                     i;
        int                     j;


        int                     size = ((ISO_SEND_TOTAL % isotest->iso_out_size) < 20) ? ISO_SEND_TOTAL + 20 : ISO_SEND_TOTAL;
        int                     frames = (size / isotest->iso_out_size) + 1;

        //RETURN_NULL_IF(out_count-- <= 0);

        //printk(KERN_INFO"%s: %d %02x\n", __FUNCTION__, out_count, isotest->iso_out_endpointAddr);


        //printk(KERN_INFO"%s: frames: %x packet: %d size: %d\n", __FUNCTION__, frames, isotest->iso_out_size, size);

        // allocate urb and buffer, fill buffer with some data
        THROW_IF(!(urb = usb_alloc_urb(frames + 1)), error);

        THROW_IF (!(urb->transfer_buffer = kmalloc(size, GFP_ATOMIC)), error);

        for (i = 0; i < size; i++) {
                unsigned char *cp = urb->transfer_buffer + i;
                *cp = i % 256;
        }

        //printk(KERN_INFO"%s: CCC\n", __FUNCTION__);

        urb->hcpriv = NULL;
        urb->context = isotest;
        urb->transfer_flags = USB_ISO_ASAP;
        urb->complete = isotest_iso_out_complete;
        urb->pipe = usb_sndisocpipe(isotest->udev, isotest->iso_out_endpointAddr);

        urb->transfer_buffer_length = size;

        urb->number_of_packets = frames;

        THROW_IF((rc = iso_out_submit(isotest, urb)), error);

        //printk(KERN_INFO"%s: OK frames: %d\n", __FUNCTION__, frames);

        CATCH(error) {
                printk(KERN_INFO"%s: FAILED rc: %d\n", __FUNCTION__, rc);
                //isotest_iso_out_free_urb(urb);
                return NULL;
        }
        return urb;
}

/* ISO IN - Receive **************************************************************************** */

static int in_count = 5;
static int in_submitted;
static int in_resubmitted;
static long in_completed;
static long in_total_received;

#if 0
static void isotest_iso_in_free_urb(struct urb *urb)
{
        struct usb_isotest *isotest;
        int i;
        unsigned long   flags;

        //printk(KERN_INFO"%s: urb: %p\n", __FUNCTION__, urb);

        RETURN_IF(!urb);

        if (urb->transfer_buffer) {
                kfree(urb->transfer_buffer);
        }

        isotest = (struct usb_isotest *)urb->context;
        usb_free_urb(urb);

        RETURN_IF(!isotest);

        local_irq_save (flags);
        for (i = 0; i < IN_URBS; i++) {
                CONTINUE_IF(isotest->iso_in_urbs[i] != urb);
                //printk(KERN_INFO"%s: clearing %d urb: %p\n", __FUNCTION__, i, urb);
                isotest->iso_in_urbs[i] = NULL;
                break;
        }
        local_irq_restore (flags);
}
#endif

struct urb *iso_in_start(struct usb_isotest *isotest);

int iso_in_submit(struct usb_isotest *isotest, struct urb *urb) 
{
        int                     i;
        int                     j;
        int                     rc = 0;

        //printk(KERN_INFO"%s: %p\n", __FUNCTION__, urb->complete);
        
        RETURN_ZERO_IF(isotest->closing);

        urb->dev = isotest->udev;
        urb->actual_length = 0;

        for (j = urb->transfer_buffer_length, i = 0; i < urb->number_of_packets; i++) {
                
                int                     send = MIN(isotest->iso_in_size, j);
                j -= send;
                urb->iso_frame_desc[i].offset = i * isotest->iso_in_size;
                urb->iso_frame_desc[i].length = send;
        }       

        RETURN_ZERO_IF(!(rc = usb_submit_urb(urb)));
        printk(KERN_INFO"%s: FAILED rc: %x\n", __FUNCTION__, rc);

        //isotest_iso_in_free_urb(urb);
        return rc;
}


/**
 * isotest_iso_in_complete
 */
void isotest_iso_in_complete (struct urb *urb)
{
        struct usb_isotest *isotest = (struct usb_isotest *)urb->context;
        int i;
        int rc;
        int status;

        //printk(KERN_INFO"%s: urb: %p\n", __FUNCTION__, urb);

        RETURN_IF(!urb);

        if (isotest->closing) {
                //printk(KERN_INFO"%s: urb: %p pre urbs: %d\n", __FUNCTION__, urb, isotest->in_urbs);
                if (urb->transfer_buffer) {
                        kfree(urb->transfer_buffer);
                }
                usb_free_urb(urb);
                isotest->in_urbs--;
                //printk(KERN_INFO"%s: urb: %p pre urbs: %d\n", __FUNCTION__, urb, isotest->in_urbs);
                return;
        }
        status = urb->status;

        if (status /* && (status != -ENOENT) && (status != -ECONNRESET)*/) {
                //printk(KERN_INFO"%s: - urb: %p nonzero write iso status received: %x\n", __FUNCTION__, urb, status);
        }

        else if (urb->actual_length) {
                //printk(KERN_INFO"%s: urb: %p lenght: %d\n", __FUNCTION__, urb, urb->actual_length);
                in_completed++;
                in_total_received += urb->actual_length;

                //printk(KERN_INFO"%s: ", __FUNCTION__);
                for (i = 0; i < urb->number_of_packets; i++) {

                        iso_trace_recv_data(&isotest->stats, 
                                        urb->transfer_buffer + urb->iso_frame_desc[i].offset,
                                        urb->iso_frame_desc[i].actual_length, 0);

                //        printk("%d:%d:%x ", i, 
                //                        urb->iso_frame_desc[i].actual_length,
                //                        urb->iso_frame_desc[i].status);


                }       
                //printk("\n");
        }

        THROW_IF((rc = iso_in_submit(isotest, urb)), error);

        in_resubmitted++;

        CATCH(error) {
                printk(KERN_INFO"%s: FAILED rc: %x\n", __FUNCTION__, rc);
        }
}


struct urb *iso_in_start(struct usb_isotest *isotest)
{
        struct urb *            urb = NULL;
        int                     frames = (ISO_SEND_TOTAL + isotest->iso_in_size) / isotest->iso_in_size;
        int                     iso_transfer_size;
        int                     rc = 0;


        //RETURN_NULL_IF(in_count-- <= 0);

        //printk(KERN_INFO"%s: %d %02x\n", __FUNCTION__, in_count, isotest->iso_in_endpointAddr);

        //iso_transfer_size = ((ISO_SEND_TOTAL % isotest->iso_in_size) > 20) ?
        //        ISO_SEND_TOTAL :
        //        ISO_SEND_TOTAL + (20 - (ISO_SEND_TOTAL % isotest->iso_in_size));

        iso_transfer_size = frames * isotest->iso_in_size;

        THROW_IF(!(urb = usb_alloc_urb(frames + 1)), error);
        THROW_IF (!(urb->transfer_buffer = kmalloc(iso_transfer_size, GFP_ATOMIC)), error);
        memset(urb->transfer_buffer, 0, isotest->iso_in_size);

        urb->hcpriv = NULL;
        urb->context = isotest;
        urb->transfer_flags = USB_ISO_ASAP;
        urb->complete = isotest_iso_in_complete;
        urb->pipe = usb_rcvisocpipe(isotest->udev, isotest->iso_in_endpointAddr);

        urb->transfer_buffer_length = iso_transfer_size;
        urb->number_of_packets = frames;

        THROW_IF((rc = iso_in_submit(isotest, urb)), error);
        in_submitted++;
        isotest->in_urbs++;
        //printk(KERN_INFO"%s: new urbs: %d\n", __FUNCTION__, isotest->in_urbs);
        return urb;

        CATCH(error) {
                printk(KERN_INFO"%s: FAILED rc: %x\n", __FUNCTION__, rc);
                //isotest_iso_in_free_urb(urb);
                return NULL;
        }
}




/* ********************************************************************************************* */

void isotest_schedule_bh(struct usb_isotest *isotest)
{
        unsigned long   flags;

        //RETURN_IF(!isotest->iso_bh.data);

        // schedule more data
        local_irq_save (flags);
        if (isotest->iso_bh.data && !isotest->iso_bh.sync) {
                MOD_INC_USE_COUNT;
                queue_task(&isotest->iso_bh, &tq_immediate);
                mark_bh(IMMEDIATE_BH);
        }
        local_irq_restore (flags);
}


static void bottomhalf(void *data)
{
        int i;
        unsigned long   flags;
        struct usb_isotest *isotest = (struct usb_isotest *) data;

        if (isotest->first) {
                //sleep_on_timeout(&isotest->iso_wq, 200);
                udelay(100);
                isotest->first = 0;
        }

        THROW_IF(!isotest, error);

        if (isotest->closing) {
                struct urb *urb;
                //printk(KERN_INFO"%s: closing\n", __FUNCTION__);

                // unlink outstanding urbs, this has side-effect of calling completion routing
                local_irq_save (flags);
                for (i = 0; i < IN_URBS; i++) {
                        CONTINUE_IF(!(urb = isotest->iso_in_urbs[i]));
                        //printk(KERN_INFO"%s: unlinking: %d IN urb: %p\n", __FUNCTION__, i, urb);
                        isotest->iso_in_urbs[i] = NULL;
                        urb->transfer_flags |= USB_ASYNC_UNLINK;
                        usb_unlink_urb(urb);
                }
                local_irq_restore (flags);

                local_irq_save (flags);
                for (i = 0; i < OUT_URBS; i++) {
                        CONTINUE_IF(!(urb = isotest->iso_out_urbs[i]));
                        //printk(KERN_INFO"%s: unlinking: %d OUT urb: %p\n", __FUNCTION__, i, urb);
                        isotest->iso_out_urbs[i] = NULL;
                        urb->transfer_flags |= USB_ASYNC_UNLINK;
                        usb_unlink_urb(urb);
                }
                local_irq_restore (flags);

                // tell disconnect that we are finished
                isotest->iso_bh.data = NULL;

        }
        else {

                //printk(KERN_INFO"%s: normal\n", __FUNCTION__);

                if (send && out_count) {
                        local_irq_save (flags);
                        for (i = 0; i < OUT_URBS; i++) {
                                CONTINUE_IF(isotest->iso_out_urbs[i]);
                                isotest->iso_out_urbs[i] = iso_out_start(isotest);
                                //printk(KERN_INFO"%s: starting: %d OUT urb: %p\n", __FUNCTION__, i, isotest->iso_out_urbs[i]);
                        }
                        local_irq_restore (flags);
                }

                if (recv && in_count) {
                        local_irq_save (flags);
                        for (i = 0; i < IN_URBS; i++) {
                                CONTINUE_IF(isotest->iso_in_urbs[i]);
                                isotest->iso_in_urbs[i] = iso_in_start(isotest);
                                //printk(KERN_INFO"%s: starting: %d IN urb: %p\n", __FUNCTION__, i, isotest->iso_in_urbs[i]);
                        }
                        local_irq_restore (flags);
                }
        }

        CATCH(error) {
                printk(KERN_ERR"%s: isotest NULL\n", __FUNCTION__);
        }
        MOD_DEC_USE_COUNT;
}

/* ********************************************************************************************* */

/* usb specific object needed to register this driver with the usb subsystem */
static struct usb_driver isotest_driver = {
        name:           "isotest",
        probe:          isotest_probe,
        disconnect:     isotest_disconnect,
        id_table:       isotest_table,
};

/**
 * isotest_probe
 *
 * Called by the usb core when a new device is connected that it thinks
 * this driver might be interested in.
 */
static void * isotest_probe(struct usb_device *udev, unsigned int ifnum, const struct usb_device_id *id)
{
        struct usb_isotest *isotest = NULL;
        struct usb_interface *interface;
        struct usb_device_descriptor *device = &udev->descriptor;
        struct usb_interface_descriptor *interface_descriptor;
        int i;

        printk(KERN_INFO"%s: %04x %04x\n", __FUNCTION__, device->idVendor, device->idProduct);

        // See if the device offered us matches what we can accept 
        //if ((device->idVendor != vendor_id) || (device->idProduct != product_id)) {
        //        printk(KERN_INFO"%s: FAILED\n", __FUNCTION__);
        // return NULL;
        //}

        // allocate memory for our device state and intialize it 
        if (!(isotest = kmalloc (sizeof(struct usb_isotest), GFP_KERNEL))) {
                printk(KERN_INFO"%s: Out of memory\n", __FUNCTION__);
                return NULL;
        }

        memset (isotest, 0x00, sizeof (*isotest));
        init_MUTEX (&isotest->sem);
        init_waitqueue_head(&isotest->iso_wq);

        isotest->udev = udev;
        isotest->first = 1;
        isotest->closing = 0;
        isotest->in_urbs = 0;
        isotest->interface = interface = &udev->actconfig->interface[ifnum];
        isotest->iso_bh.routine = bottomhalf;
        isotest->iso_bh.data = (void *)isotest;
        interface_descriptor = &interface->altsetting[0];

        // set up the endpoint information and check out the endpoints 

        for (i = 0; i < interface_descriptor->bNumEndpoints; ++i) {

                struct usb_endpoint_descriptor *endpoint = &interface_descriptor->endpoint[i];

                //printk(KERN_INFO"%s: looking at %02x\n", __FUNCTION__, endpoint->bEndpointAddress);

                if ((endpoint->bEndpointAddress & 0x80) && ((endpoint->bmAttributes & 3) == 0x01)) {

                        //printk(KERN_INFO"%s: found ISO IN %02x\n", __FUNCTION__, endpoint->bEndpointAddress);
                        isotest->iso_in_size = endpoint->wMaxPacketSize;
                        isotest->iso_in_endpointAddr = endpoint->bEndpointAddress;
                }

                if (((endpoint->bEndpointAddress & 0x80) == 0x00) && ((endpoint->bmAttributes & 3) == 0x01)) {

                        //printk(KERN_INFO"%s: found ISO OUT %02x\n", __FUNCTION__, endpoint->bEndpointAddress);

                        isotest->iso_out_size = endpoint->wMaxPacketSize;
                        isotest->iso_out_endpointAddr = endpoint->bEndpointAddress;
                }
        }

        out_count = ISO_SEND_TRANSFERS;

        // let the user know what node this device is now attached to 
        //printk(KERN_INFO"%s: USB TEST device now attached to ISOTEST\n", __FUNCTION__);

        isotest_schedule_bh(isotest);

        return isotest;
}

/**
 * isotest_disconnect
 *
 * Called by the usb core when the device is removed from the system.
 */
static void isotest_disconnect(struct usb_device *udev, void *ptr)
{
        struct usb_isotest *isotest;

        printk(KERN_INFO"%s: in_submitted: %d in_resubmitted: %d in_completed: %ld in_total: %ld\n", 
                        __FUNCTION__, in_submitted, in_resubmitted, in_completed, in_total_received);

        RETURN_IF(!(isotest = (struct usb_isotest *)ptr));

        // set flag to say we are closing
        isotest->closing = 1;
        isotest_schedule_bh(isotest);

        while (isotest->iso_bh.data) {
                isotest_schedule_bh(isotest);
                printk(KERN_INFO"%s: waiting for bh\n", __FUNCTION__);
                sleep_on_timeout(&isotest->iso_wq, 20);
        }

        while (isotest->in_urbs) {
                printk(KERN_INFO"%s: waiting for urbs\n", __FUNCTION__);
                sleep_on_timeout(&isotest->iso_wq, 20);
        }

        kfree(isotest);

        printk(KERN_INFO"%s: USB ISOTEST now disconnected\n", __FUNCTION__);
}


/* ********************************************************************************************* */

void iso_start_in(int count)
{

}

void iso_start_out(int count)
{

}


/**
 * isotest_init
 */
static int isotest_init(void)
{
        int result;

        printk(KERN_INFO"%s:\n", __FUNCTION__);

        if (vendor_id && product_id) {
                int i;
                for (i = 0; i < (sizeof(isotest_table) / sizeof(struct usb_device_id) - 1); i++) {

                        if ((isotest_table[i].idVendor == vendor_id) && isotest_table[i].idProduct == product_id ) {
                                printk(KERN_INFO"%s: vendor_id: %04x product_id: %04x already in table\n",
                                                __FUNCTION__, vendor_id, product_id);
                                break;
                        }
                        printk(KERN_INFO"%s: vendor_id: %04x product_id: %04x\n",
                                        __FUNCTION__, isotest_table[i].idVendor, isotest_table[i].idProduct);
                }
                if (!isotest_table[i].idVendor && !isotest_table[i].idProduct) {
                        printk(KERN_INFO"%s: inserting vendor_id: %04x product_id: %04x into table\n",
                                        __FUNCTION__, vendor_id, product_id);

                        isotest_table[i].match_flags = USB_DEVICE_ID_MATCH_DEVICE;
                        isotest_table[i].idVendor = vendor_id;
                        isotest_table[i].idProduct = product_id;
                        isotest_table[i].bDeviceClass = 0;
                        isotest_table[i].bDeviceSubClass = 0;
                }
        }

        iso_trace_init("isotest_host");

        /* register this driver with the USB subsystem */
        result = usb_register(&isotest_driver);
        if (result < 0) {
                printk(KERN_INFO"%s: usb_register failed for the "__FILE__" driver. Error number %d\n", KERN_INFO, result);
                return -1;
        }

        printk(KERN_INFO "%s: " DRIVER_DESC " " DRIVER_VERSION "\n", __FUNCTION__);
        return 0;
}


/**
 * isotest_exit
 */
static void isotest_exit(void)
{
        /* deregister this driver with the USB subsystem */
        usb_deregister(&isotest_driver);

        iso_trace_exit("isotest_host");
}


module_init (isotest_init);
module_exit (isotest_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,17)
MODULE_LICENSE("PRIVATE");
#endif

