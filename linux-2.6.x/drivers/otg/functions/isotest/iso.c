/*
 * otg/isotest_fd/iso.c
 * @(#) balden@seth2.belcarratech.com|otg/functions/isotest/iso.c|20051116204957|19120
 *
 *      Copyright (c) 2003-2004 Belcarra
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2005-2006 Motorola, Inc.
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
 */

#include <linux/config.h>
#include <otg/otg-compat.h>
#include <otg/otg-module.h>



MODULE_AUTHOR ("sl@belcarra.com, tbr@belcarra.com");
MODULE_DESCRIPTION ("ISO Loop Test Function");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,17)
MODULE_LICENSE("GPL");
#endif

//#include <linux/init.h>
//#include <linux/list.h>
//#include <asm/uaccess.h>
//#include <linux/slab.h>
//#include <linux/interrupt.h>

//#include <linux/smp_lock.h>
//#include <linux/ctype.h>
//#include <linux/timer.h>
//#include <linux/string.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#define ISO iso_if_trace_tag
extern otg_tag_t ISO;

//USBD_MODULE_INFO ("isotest_fd 2.0-beta");

#include "test.h"
#include "fermat.h"

#define ISO_OUT        0x00
#define ISO_IN         0x01

#define ENDPOINTS       0x02

struct isotest_if_private {
        int interface;
        struct usbd_function_instance *function;
        rwlock_t rwlock;

        int open;
        int closing;

        struct WORK_STRUCT iso_bh;

        u8 bEndpointAddress[ENDPOINTS];
        u8 wMaxPacketSize[ENDPOINTS];

        struct isotest_stats isotest_stats;

        int out_count;
        int in_count;
        int send_size;
        int iso_transfer_in_count;

};


u8 isotest_requested_endpoints[ENDPOINTS+1] = {
        USB_DIR_OUT | USB_ENDPOINT_ISOCHRONOUS,
        USB_DIR_IN | USB_ENDPOINT_ISOCHRONOUS,
        0,
};

#define ISO_OUT_PKTSIZE 90
#define ISO_IN_PKTSIZE 90
#define isotest_requested_transferSizes xfer_sizes

u16 xfer_sizes[ENDPOINTS+1] = {
        ISO_OUT_PKTSIZE,
        ISO_IN_PKTSIZE,
        0,
};



/* Module Parameters ************************************************************************* */
MODULE_PARM (xfer_sizes, "3-3h");
MODULE_PARM_DESC (xfer_sizes, "Requested transfer sizes for each endpoint; default 90 for iso in and out");

// packet sizes
static u32 in = ISO_IN_PKTSIZE;
MODULE_PARM (in, "i");
MODULE_PARM_DESC (in, "in size");

static u32 out = ISO_OUT_PKTSIZE;
MODULE_PARM (out, "i");
MODULE_PARM_DESC (out, "out size");

static int fermat=0;
MODULE_PARM (fermat, "i");
MODULE_PARM_DESC (fermat, "Apply randomization to buffer");

static int custom=0;
MODULE_PARM (custom, "i");
MODULE_PARM_DESC (custom, "Supply custom pattern via xmit_pattern parameter");

static int print_all=0;
MODULE_PARM (print_all, "i");
MODULE_PARM_DESC (print_all, "Print all buffers, not just the first");

#define ZERO4 0,0,0,0
#define ZERO16 ZERO4,ZERO4,ZERO4,ZERO4
#define ZERO64 ZERO16,ZERO16,ZERO16,ZERO16
static u8 xmit_pattern[256]={1,0xE,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe, ZERO16,ZERO16,ZERO16,ZERO64,ZERO64,ZERO64};
MODULE_PARM(xmit_pattern,"1-256b");
MODULE_PARM_DESC(xmit_pattern, "pattern to be transmitted, count, size, payload; size is "
                "the size of the payload following. count is the number of times to repeat the pattern (0=infinite)");
static struct {
        int     count;
        int     size;
        u8      *payload_start, *payload_end;
        u8      *payload;
        int     total_size;
        int     sent;
} xps; // Transmit pattern state

static void xmit_pattern_state_init(void);
static u8 xmit_pattern_next(void);
static void fill_xmit_buffer(u32 size, u8*buffer);

static void xmit_pattern_state_init(){
        xps.count = xmit_pattern[0];
        xps.size = xmit_pattern[1];
        xps.payload_start = xmit_pattern+2;
        xps.payload_end = xps.payload_start + xps.size;
        xps.payload = xps.payload_start;
        xps.sent = 0;
        xps.total_size = xps.count * xps.size;
}

static u8 xmit_pattern_next(){

        // payload always points at the next character to send
        u8      next_value = *xps.payload++;

        xps.sent += 1;

        if (xps.payload >= xps.payload_end)
                xps.payload = xps.payload_start; //Rewind buffer

        // if Total pattern has been sent; rewind
        if (xps.sent >= xps.total_size){
                xmit_pattern_state_init();
        }

        return next_value;
}

static void fill_xmit_buffer(u32 size, u8 *buffer)
{
        u8 * limit = buffer + size;
        xmit_pattern_state_init();
        while(buffer < limit){
                *buffer++ = xmit_pattern_next();
        }
}

static void fill_xmit_buffer_default(u32 size, u8 * buffer){
        int  j;
        for(j = 0; j < size; j++){
                buffer[j] = j & 0xff;
        }
}

static void print_buffer(u32 size, u8 * buffer){
        // Print up to 16 bytes of the buffer, first time called only
        static int first = 1;
        int n = ( size >=16 ? 16 : size);
        int j;
        if((first) || (print_all)){
                char *prefix = first? "\n" : "";
                printk(KERN_INFO "%sxmit buffer:", prefix);
                for( j=0 ; j < n; j++){
                        printk("%02x",buffer[j]);
                }
                printk("\n");
                first = 0;
        }
}



/* ************************************************************************** */
int isotest_urb_sent (struct usbd_urb *urb, int rc);
int isotest_recv_urb (struct usbd_urb *urb, int rc);

void schedule_bh(struct usbd_interface_instance *interface_instance);

/* Transmit Function *************************************************************************** */

static int isotest_xmit_data (struct usbd_interface_instance *interface_instance)
{
        struct isotest_if_private *isotest = interface_instance->function.privdata;
        int i; 
        int j; 
        int frames;
        int size = ((isotest->send_size % in) < 20) ? isotest->send_size + 20 : isotest->send_size;
        struct usbd_urb *urb;

        //printk(KERN_INFO"%s: open: %d in_count: %d\n", __FUNCTION__, isotest.open, in_count);

        RETURN_ZERO_IF(!isotest->open);

        RETURN_EINVAL_IF(!(urb = usbd_alloc_urb (&interface_instance->function, ISO_IN, size, isotest_urb_sent)));

        frames = (size / in) + 1;

        isotest->iso_transfer_in_count++;

        for (i = 0; i < frames; i++) {

                u8 *cp = urb->buffer + (i * in);
                if(custom){
                        (void) fill_xmit_buffer(in, cp);
                }
                else {
                        (void) fill_xmit_buffer_default(in,cp);
                }
                if(fermat){
                        fermat_encode(cp, in);
                }
                print_buffer(in, cp);

                // iso_transfer_count
                *cp++ = cpu_to_le16(isotest->iso_transfer_in_count) & 0xff;
                *cp++ = (cpu_to_le16(isotest->iso_transfer_in_count) >> 8) & 0xff;
                *cp++ = (cpu_to_le16(isotest->iso_transfer_in_count) >> 16) & 0xff;
                *cp++ = (cpu_to_le16(isotest->iso_transfer_in_count) >> 24) & 0xff;

                // iso transfer length
                *cp++ = cpu_to_le16(size) & 0xff;
                *cp++ = (cpu_to_le16(size) >> 8) & 0xff;

                // iso frame size
                *cp++ = cpu_to_le16(in) & 0xff;
                *cp++ = (cpu_to_le16(in) >> 8) & 0xff;

                // total frames
                *cp++ = cpu_to_le16(frames) & 0xff;
                *cp++ = (cpu_to_le16(frames) >> 8) & 0xff;

                // this packet number
                *cp++ = cpu_to_le16(i+1) & 0xff;
                *cp++ = (cpu_to_le16(i+1) >> 8) & 0xff;

        }
        urb->actual_length = size;
        return usbd_start_in_urb (urb);
}


/* isotest_urb_sent - called to indicate URB transmit finished
 * @urb: pointer to struct usbd_urb
 * @rc: result
 */
int isotest_urb_sent (struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
        struct isotest_if_private *isotest = urb->function_privdata;

        schedule_bh(interface_instance);
        usbd_free_urb (urb);

        return 0;
}


/* USB Device Functions ************************************************************************ */

/* isotest_recv_urb - called to indicate URB has been received
 * @urb - pointer to struct usbd_urb
 *
 * Return non-zero if we failed and urb is still valid (not disposed)
 */
int isotest_recv_urb (struct usbd_urb *urb, int rc)
{
        struct usbd_function_instance *function_instance = urb->function_instance;
        struct isotest_if_private *isotest = urb->function_privdata;

        iso_trace_recv_data(&isotest->isotest_stats, urb->buffer, urb->actual_length, 0);

        // restart_recv urb
        return (usbd_start_out_urb (urb));
}


/* ********************************************************************************************* */

void iso_start_in(struct usbd_interface_instance *interface_instance, int count)
{
        struct isotest_if_private *isotest = interface_instance->function.privdata;
        isotest->in_count = count;
        schedule_bh(interface_instance);
}

void iso_start_out(struct usbd_interface_instance *interface_instance, int count)
{
        struct isotest_if_private *isotest = interface_instance->function.privdata;
        isotest->out_count = count;
        schedule_bh(interface_instance);
}

static void
bottomhalf(void *data)
{       
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)data;
        struct isotest_if_private *isotest = interface_instance->function.privdata;

        if (isotest->closing) 
                PREPARE_WORK_ITEM(isotest->iso_bh, NULL, NULL);

        else if (isotest->open) 
                isotest_xmit_data (interface_instance);

}



void schedule_bh(struct usbd_interface_instance *interface_instance) 
{
        struct isotest_if_private *isotest = interface_instance->function.privdata;
        unsigned long   flags;

        local_irq_save (flags);
        PREPARE_WORK_ITEM(isotest->iso_bh, bottomhalf, interface_instance);
        SCHEDULE_WORK(isotest->iso_bh);
        local_irq_restore (flags);

}




/* ********************************************************************************************* */


/*! 
 * @brief isotest_if_set_configuration - called to indicate urb has been received
 * @param function
 * @param configuration
 * @return int
 */
static int isotest_if_set_configuration (struct usbd_function_instance *function, int configuration)
{       
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct isotest_if_private *isotest= interface_instance->function.privdata;
        int i;

        
        TRACE_MSG3(ISO, "ISO_IF[%d] %x cfg: %d ", interface_instance->wIndex, function, configuration);
        
        // XXX Need to differentiate between non-zero, zero and non-zero done twice

        function->privdata = isotest;
        
        for (i = ISO_OUT; i < ISO_IN; i++) {
                isotest->bEndpointAddress[i] = usbd_endpoint_bEndpointAddress(function, i, 0);
                isotest->wMaxPacketSize[i] = usbd_endpoint_wMaxPacketSize(function, i, 0);
        }
        
        TRACE_MSG6(ISO, "ISO_IF[%2d] Configured: %d bEndpointAddress: %02x %02 size: %02x %02x",
                        interface_instance->wIndex, configuration, 
                        isotest->bEndpointAddress[0], isotest->bEndpointAddress[1], 
                        isotest->wMaxPacketSize[0], isotest->wMaxPacketSize[1] );

        //isotest_if_send(function);                                           // start sending
        schedule_bh(interface_instance);

        return 0;
}

        
/*!     
 8 @brief isotest_if_endpoint_cleared - called by the USB Device Core when endpoint cleared
 * @param function The function instance for this driver
 * @param bEndpointAddress
 * @return none
 */     
static void isotest_if_endpoint_cleared (struct usbd_function_instance *function, int bEndpointAddress)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct isotest_if_private *isotest_= interface_instance->function.privdata;
        TRACE_MSG1(ISO,"CLEARED bEndpointAddress: %02x", bEndpointAddress);
        //isotest_if_send(function);                                           // re-start sending
        schedule_bh(interface_instance);
}



/* ********************************************************************************************* */
/*! isotest_if_function_enable - called by USB Device Core to enable the driver
 * @param function The function instance for this driver to use.
 * @return non-zero if error.
 */
static int isotest_if_function_enable (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct isotest_if_private *isotest= NULL;
        int i;

        RETURN_EINVAL_UNLESS((isotest= CKMALLOC(sizeof(struct isotest_if_private), GFP_KERNEL)));
        isotest->send_size = 1000;
        interface_instance->function.privdata = (void *)isotest;
        PREPARE_WORK_ITEM(isotest->iso_bh, bottomhalf, (void *)interface_instance);
        isotest->closing = 0;
        for (i = 0; i < 2; i++) {
                struct usbd_urb *urb;
                BREAK_IF(!(urb = usbd_alloc_urb (function, ISO_OUT, 
                                                usbd_endpoint_transferSize(
                                                        function, ISO_OUT,usbd_high_speed(function)), 
                                                isotest_recv_urb
                                                )));
                if (usbd_start_out_urb(urb)) 
                        usbd_free_urb(urb);
        }
        iso_start_in(interface_instance, 100);
        schedule_bh(interface_instance);
        return 0;
}

/*! isotest_if_function_disable - called by the USB Device Core to disable the driver
 * @param function The function instance for this driver
 */
static void isotest_if_function_disable (struct usbd_function_instance *function)
{
        struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function;
        struct isotest_if_private *isotest= interface_instance->function.privdata;
        interface_instance->function.privdata = NULL;
        isotest->closing = 1;
        LKFREE(isotest);
}


/* ********************************************************************************************* */

/*! data indexes
 */
u8 isotest_data_indexes [] = { ISO_OUT, ISO_IN, };


/*! Alternate descriptions
 */
static struct usbd_alternate_description isotest_alternate_descriptions[] = {
        { 
                .iInterface = CONFIG_OTG_ISOTEST_INTF,
                .endpoints = sizeof (isotest_data_indexes) / sizeof(u8),
                .endpoint_index = isotest_data_indexes,
        },
};


/*! List of Interface description(s)
 */
static struct usbd_interface_description isotest_interfaces[] = {
      {
              .alternates = sizeof (isotest_alternate_descriptions) / sizeof (struct usbd_alternate_description),
              .alternate_list = isotest_alternate_descriptions,
      },
};      


/*! endpoint requests
 */
static struct usbd_endpoint_request isotest_endpoint_requests[ENDPOINTS+1] = {
        { 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_ISOCHRONOUS, ISO_OUT_PKTSIZE, ISO_OUT_PKTSIZE * 4, },
        { 1, 0, 0, USB_DIR_IN | USB_ENDPOINT_ISOCHRONOUS, ISO_IN_PKTSIZE, ISO_OUT_PKTSIZE * 4, },
        { 1, },
};

/*! function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations isotest_if_function_ops = {
        .set_configuration = isotest_if_set_configuration,
        //.device_request = isotest_if_device_request,           /*!< called for each received device request */
        .endpoint_cleared = isotest_if_endpoint_cleared,
        .function_enable = isotest_if_function_enable,
        .function_disable = isotest_if_function_disable,
};

/*! isotest_interface_driver - USB Device Core function driver definition
 */
struct usbd_interface_driver isotest_interface_driver = {
        .driver.name = "isotest-if",                            /*! driver name */
        .driver.fops = &isotest_if_function_ops,                             /*!< operations table */
        .interfaces = sizeof (isotest_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = isotest_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = isotest_endpoint_requests,
};

/* ********************************************************************************************* */

otg_tag_t ISO;

/*! 
 * isotest_if_modinit() - module init
 *
 * This is called by the Linux kernel; either when the module is loaded
 * if compiled as a module, or during the system intialization if the 
 * driver is linked into the kernel.
 *
 */
static int isotest_if_modinit (void)
{
        int i;
        #if !defined(OTG_C99)
        isotest_if_global_init();
        isotest_if_ops_init();
        #endif /* defined(OTG_C99) */

        ISO = otg_trace_obtain_tag();

        // register as usb function driver
        TRACE_MSG0(ISO, "REGISTER INTERFACE");
        TRACE_MSG2(ISO, "%s %d", isotest_interface_driver.driver.name, isotest_interface_driver.endpointsRequested);

        THROW_IF (usbd_register_interface_function ( &isotest_interface_driver, "isotest-if", NULL), error);

        TRACE_MSG2(ISO, "%s %d", isotest_interface_driver.driver.name, isotest_interface_driver.endpointsRequested);

        TRACE_MSG0(ISO, "REGISTER FINISHED");

        //isotest_ep_1[4] = out&0xff;
        //isotest_ep_1[5] = (out>>8)&0xff;
        //isotest_ep_2[4] = in&0xff;
        //isotest_ep_2[5] = (in>>8)&0xff;

        // XXX should this be the endpoint request structure?
        //iso_endpoint_requests[0].fs_requestedTransferSize = out;
        //iso_endpoint_requests[1].fs_requestedTransferSize = in;

        iso_trace_init("isotest_fd");
        if(fermat){
                fermat_init();
        }

        // register us with the usb device support layer
        THROW_IF (usbd_register_interface_function (&isotest_interface_driver, "isotest-if", NULL), error);

        CATCH(error) {
                otg_trace_invalidate_tag(ISO);
                return -EINVAL;
        }
        return 0;
}

module_init (isotest_if_modinit);

#if OTG_EPILOGUE
/*! 
 * isotest_if_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded 
 * if compiled as a module. This function is never called if the 
 * driver is linked into the kernel.
 *
 * @param void
 * @return void
 */
static void isotest_if_modexit (void)
{
        iso_trace_exit("isotest_fd");

        //#ifdef CONFIG_OTG_ISO_BH
        //while (isotest_if_bh_active) {
        //        printk(KERN_ERR"%s: waiting for bh\n", __FUNCTION__);
        //        schedule_timeout(10 * HZ);
        //}
        //#endif
        usbd_deregister_interface_function (&isotest_interface_driver);
        otg_trace_invalidate_tag(ISO);
}

module_exit (isotest_if_modexit);
#endif

/* ********************************************************************************************* */
