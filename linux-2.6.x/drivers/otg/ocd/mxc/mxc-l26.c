/*
 * otg/mxc/mxc-l26.c - Linux 2.6 Freescale USBOTG aware Host Controller Driver (HCD)
 * @(#) balden@seth2.belcarratech.com|otg/ocd/mxc/mxc-l26.c|20051116203403|39957
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies
 *
 * By:
 *      Stuart Lynne <sl@belcara.com>
 *      Bruce Balden <balden@belcara.com>
 *      Tony Tang    <tt@belcara.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 02/17/2006         Motorola         3G bring up 
 * 06/12/2006         Motorola         OTG name change
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
 */
/*!
 * @file otg/ocd/mxc/mxc-l26.c
 * @brief Freescale USB Host Controller Driver
 *
 * This is a complete and self contained Linux 2.6 USB Host Driver.
 *
 * It also conforms to the requirements for use as an OTG HCD driver
 * in the Belcarra OTG Stack.
 *
 * The hardware is an integrated design, so it also works in conjunction 
 * with the mxc OCD and PCD drivers to share the hardware under the direction
 * of the OTG State Machine.
 * 
 *
 * @ingroup FSOTG
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <linux/usb.h>
#include <linux/delay.h>

#include <otg/otg-compat.h>

#include <core/hcd.h>

#include <otg/usbp-hub.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-utils.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <asm/mot-gpio.h>  
#include <otghw/mxc-hardware.h>
#include <asm/arch/board.h>
#include <asm/memory.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>


#include "mxc-hcd.h"

/* ********************************************************************************************* */
/* ********************************************************************************************* */
extern void mxc_hcd_schedule_irq(struct mxc_hcd *mxc_hcd);
extern void mxc_hcd_finish_req_irq(struct mxc_hcd *mxc_hcd, struct mxc_req *mxc_req, struct urb *urb, int killed);
extern void mxc_host_clock_on(void);
extern void mxc_host_clock_off(void);
extern fs_data_buff *rel_data_buff(struct mxc_hcd *mxc_hcd, fs_data_buff *db);
extern void rel_etd_irq(struct mxc_hcd *mxc_hcd, int etdn);
extern void mxc_hcd_hw_rh_port_feature(struct mxc_hcd *mxc_hcd, u16 wValue, u16 wIndex, int set_flag);
extern char *port_feature_name[];

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! Transfer Request Management
 */

/*! mxc_get_request - get unused mxc_request or allocate a new one
 * @param mxc_hcd
 */
struct mxc_req * mxc_get_request(struct mxc_hcd *mxc_hcd)
{
        struct mxc_req          *mxc_req = NULL;
        struct mxc_req          *mxc_req_tmp = NULL;
        unsigned long           flags;

        /* attempt to find first entry in list */

        local_irq_save(flags);
        list_for_each_entry(mxc_req_tmp, &mxc_hcd->unused, queue) {
                /* delete from list */
                mxc_req = mxc_req_tmp;
                list_del(&mxc_req->queue);
                break;
        }
        local_irq_restore(flags);

        /* otherwise attempt to allocate and initialize */

        UNLESS(mxc_req) {
               RETURN_NULL_UNLESS((mxc_req = CKMALLOC(sizeof(struct mxc_req), GFP_ATOMIC)));
               INIT_LIST_HEAD(&mxc_req->queue);
               local_irq_save(flags);
               mxc_hcd->allocated_count++;
               local_irq_restore(flags);
        }
        return mxc_req;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! mxc_hcd_urq_enqueue -
 * @param hcd
 * @param urb
 * @param mem_flags
 *
 * New Style L2.6.10 USB Core urb enqueue.
 */
int mxc_hcd_urb_enqueue( struct usb_hcd *hcd, struct urb *urb, int mem_flags)
{
        struct mxc_hcd          *mxc_hcd = hcd_to_mxc(hcd);
        struct usb_device       *udev = urb->dev;
        struct hcd_dev          *hdev = (struct hcd_dev *) udev->hcpriv;

        //int                     dev_addr = usb_pipedevice(urb->pipe);
        unsigned int            pipe = urb->pipe;
        int                     is_out = usb_pipeout(pipe);
        int                     type = usb_pipetype(pipe);
        int                     endpoint = usb_pipeendpoint(pipe);
        unsigned long           flags;

        struct mxc_req          *mxc_req = NULL;

        TRACE_MSG5(HCD, "hcd: %x urb: %x endpoint: %02x is_out: %d type: %0d", hcd, urb, endpoint, is_out, type);

        /* data address needs to be on a 32-bit boundary. */

        RETURN_EPIPE_IF (0x3 & (u32)urb->transfer_buffer);

        /* get request */

        RETURN_ENOMEM_UNLESS((mxc_req = mxc_get_request(mxc_hcd)));

        /* initialize request */

        mxc_req->urb = urb;
        mxc_req->etdn = -1;
        mxc_req->etd_urb_state = EPQ_NOTUSED;
        mxc_req->remaining = 0;
        memset((void *)&mxc_req->sdp_etd, 0, sizeof(mxc_req->sdp_etd));
        mxc_req->setup_dma = mxc_req->transfer_dma = 0;
        urb->hcpriv = mxc_req;

        /* XXX MXC DMA can only transfer full buffers, so we need to provide
         * an intermediate buffer for it to use.
         */
        mxc_req->bounce_buffer = NULL;
        if (!is_out && (urb->transfer_buffer_length % 128)) {

                int length = ((urb->transfer_buffer_length + 128) / 128) * 128;

                TRACE_MSG2(HCD, "bounce buffer length: %d -> %d", urb->transfer_buffer_length, length);

                UNLESS((mxc_req->bounce_buffer = CKMALLOC(length + 32, GFP_KERNEL))) {
                        local_irq_save(flags);
                        list_add_tail(&mxc_req->queue, &mxc_hcd->unused);
                        local_irq_restore(flags);
                        spin_unlock(&urb->lock);
                        return -ENOMEM;
                }

                mxc_req->transfer_buffer_save = urb->transfer_buffer;
                mxc_req->bounce_addr = mxc_req->bounce_buffer;
                urb->transfer_buffer = mxc_req->bounce_addr;

                //if (is_out)
                //        memcpy(mxc_req->bounce_addr, mxc_req->transfer_buffer_save, urb->transfer_buffer_length);
        }

        /* map setup_packet and transfer_buffer */ 
        
        if (urb->setup_packet)
                mxc_req->setup_dma = dma_map_single ( mxc_hcd->hcd.self.controller,
                                urb->setup_packet, sizeof (struct usb_ctrlrequest), DMA_TO_DEVICE);

        if (urb->transfer_buffer && urb->transfer_buffer_length)
                mxc_req->transfer_dma = dma_map_single ( mxc_hcd->hcd.self.controller,
                                urb->transfer_buffer, urb->transfer_buffer_length,
                                usb_pipein (urb->pipe) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);


        TRACE_MSG5(HCD, "setup_packet: %x %x transfer_buffer: %x %x %d",
                        urb->setup_packet, mxc_req->setup_dma,
                        urb->transfer_buffer, mxc_req->transfer_dma,
                        urb->transfer_buffer_length);

        /* in case of unlink-during-submit */

        spin_lock(&urb->lock);

        if (urb->status != -EINPROGRESS) {
                printk(KERN_INFO"%s: EINPROGRESS\n", __FUNCTION__); 
                TRACE_MSG2(HCD, "status != EINPROGRESS hcd: %x urb: %x", hcd, urb);
                local_irq_save(flags);
                list_add_tail(&mxc_req->queue, &mxc_hcd->unused);
                local_irq_restore(flags);
                spin_unlock(&urb->lock);
                return -ENOMEM;
        }
        spin_unlock(&urb->lock);

        local_irq_save(flags);
        mxc_hcd->request_count++;
        list_add_tail(&mxc_req->queue, &mxc_hcd->inactive);
        local_irq_restore(flags);

        /* if ep[EPNUM(endpoint,is_out)] is NULL then nothing is currently scheduled
         * for this device to that endpoint
         */
        UNLESS (hdev->ep[EPNUM(endpoint, is_out)]) {
                TRACE_MSG2(HCD, "SCHEDULING  hcd: %x urb: %x", hcd, urb);
                local_irq_save(flags);
                mxc_hcd_schedule_irq(mxc_hcd);
                local_irq_restore(flags);
        }
        return 0;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! mxc_hcd_giveback_req - give an urb back to the hcd driver
 * @param mxc_hcd
 * @param mxc_req
 * @param status
 *
 * This function will set the status of an urb IFF it is currently EINPROGRESS
 * and then use usb_hcd_giveback_req() to pass control of the urb back to the
 * hcd driver.
 */
void mxc_hcd_giveback_req_irq(struct mxc_hcd *mxc_hcd, struct mxc_req *mxc_req, int status)
{
        struct urb              *urb = mxc_req->urb;
        unsigned int            pipe = urb->pipe;
        int                     is_out = usb_pipeout(pipe);

        TRACE_MSG2(HCD, "urb: %x status: %x", mxc_req->urb, status);

        /* unmap setup_packet and transfer_buffer */ 

        if (urb->setup_packet)
                dma_unmap_single (mxc_hcd->hcd.self.controller, mxc_req->setup_dma,
                                sizeof (struct usb_ctrlrequest), DMA_TO_DEVICE);

        if (urb->transfer_buffer && urb->transfer_buffer_length)
                dma_unmap_single (mxc_hcd->hcd.self.controller,
                                mxc_req->transfer_dma, urb->transfer_buffer_length,
                                usb_pipein (urb->pipe) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);

        if (mxc_req->bounce_buffer) {

                if (!is_out)
                        memcpy(mxc_req->transfer_buffer_save, mxc_req->bounce_addr, urb->transfer_buffer_length);

                urb->transfer_buffer = mxc_req->transfer_buffer_save;
                LKFREE(mxc_req->bounce_buffer);
                mxc_req->bounce_buffer = NULL;
        }

#if 0   
        if (urb->transfer_buffer) {
                int i;
                u8 *cp = urb->transfer_buffer;

                TRACE_MSG1(HCD, "NEXT TX: length: %d", urb->actual_length);

                for (i = 0; i < urb->actual_length;  i+= 8)

                        TRACE_MSG8(HCD, "BUF:  %02x %02x %02x %02x %02x %02x %02x %02x",
                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                  );
        }
#endif


        /* Call the upper layer completion routine.  Decrement the ref count
         * (release our interest in this urb).
         */
        if (urb->status == -EINPROGRESS) urb->status = status;

        urb->hcpriv = NULL;
        usb_hcd_giveback_urb(&mxc_hcd->hcd, mxc_req->urb, NULL /* XXX */);
        list_add_tail(&mxc_req->queue, &mxc_hcd->unused);
}


/*! mxc_hcd_urb_dequeue -
 * @param hcd
 * @param urb
 * @return error if not found
 *
 * Used by hcd driver to dequeue a previously enqueued urb that has not
 * completed.
 *
 * This should find and stop the request and then use the giveback
 * procedure to pass control of the urb back to the hcd layer.
 */
int mxc_hcd_urb_dequeue(struct usb_hcd *hcd, struct urb *urb)
{
        struct mxc_hcd          *mxc_hcd = hcd_to_mxc(hcd);
        struct mxc_req          *mxc_req;
        struct mxc_req          *mxc_req_save;

        unsigned long           flags;
        int                     etdn;

        unsigned int            pipe = urb->pipe;
        int                     is_out = usb_pipeout(pipe);
        int                     type = usb_pipetype(pipe);
        int                     endpoint = usb_pipeendpoint(pipe);

        TRACE_MSG5(HCD, "hcd: %x urb: %x endpoint: %02x is_out: %d type: %0d", hcd, urb, endpoint, is_out, type);

        // XXX 
        // mxc_req = urb->hcpriv;

        local_irq_save(flags);

        /* search through inactive list */

        list_for_each_entry_safe(mxc_req, mxc_req_save, &mxc_hcd->inactive, queue) {
                CONTINUE_IF(mxc_req->urb != urb);

                TRACE_MSG1(HCD, "urb: %x found inactive", urb); 

                /* found it - delete from active list, restore irq, giveback and return */

                list_del(&mxc_req->queue);
                mxc_hcd_giveback_req_irq(mxc_hcd, mxc_req, 0);  // check if 0 is correct status to return
                local_irq_restore(flags);
                return 0;
        }

        /* search through active array */

        for (etdn = 0; etdn < NUM_ETDS; etdn++) {
                mxc_req = mxc_hcd->active[etdn];
                CONTINUE_UNLESS(mxc_req);
                CONTINUE_IF(mxc_req->urb != urb);

                /* found it - finish it (and giveback), restore irq and return */
                TRACE_MSG1(HCD, "urb: %x found active", urb); 

                mxc_hcd_finish_req_irq(mxc_hcd, mxc_req, mxc_req->urb, TRUE); // XXX This should work - need to test 
                local_irq_restore(flags);
                return 0;
        }


        /* didn't find it - restore irq, return error */

        TRACE_MSG1(HCD, "urb: %x not found", urb); 
        local_irq_restore(flags);

        return -EINVAL;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! OTG Support Functions
 */

/*! MXC Hardware bottom half and OTG Init
 * Called as a "bottom-half" to do usbcore registration once all the HW is usable.
 */
static void mxc_hcd_bh_init(void *arg)
{
        struct hcd_instance *hcd = (struct hcd_instance *) arg;
        otg_event(hcd->otg, HCD_OK, HCD, "HCD_INIT SET HCD_OK");
        TRACE_MSG0(HCD,"finished");
}

static void mxc_hcd_bh_exit(void *arg)
{
        struct hcd_instance *hcd = (struct hcd_instance *) arg;
        otg_event(hcd->otg, HCD_OK, HCD, "HCD_INIT RESET (EXIT) HCD_OK");
}

                                
/*! hcd_init_func - per host controller common initialization
 *                                      
 * This is called to initialize / de-initialize the HCD, all except the last
 * stage of registering the root hub, because that needs to wait until rh_hcd_en_func()
 *                                      
 * We start work items to do this.
 *                              
 */                     
void hcd_init_func (struct otg_instance *otg, u8 flag)
{       
        struct hcd_instance *hcd = otg->hcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;
                
        switch (flag) { 
        case SET:               
                // Schedule BH for mxc_hcd_bh_init...
                TRACE_MSG0(HCD, "HCD_INIT: SET");             
                PREPARE_WORK_ITEM(hcd->bh, mxc_hcd_bh_init, hcd);
                SCHEDULE_WORK(hcd->bh);
                TRACE_MSG0(HCD, "mxc_hcd_bh_init() schedule finished");
                break;  
                        
        case RESET:
                TRACE_MSG0(HCD, "HCD_INIT: RESET");
                PREPARE_WORK_ITEM(hcd->bh, mxc_hcd_bh_exit, hcd);
                SCHEDULE_WORK(hcd->bh);
                break;
        }       
}                       
                        
/*!
 * mxc_hcd_en_func() - otg hcd enable output function
 * @param otg
 * @param flag
 */
void mxc_hcd_en_func(struct otg_instance *otg, u8 flag)
{
        struct hcd_instance *hcd = otg->hcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;
        struct tcd_instance *tcd = otg->tcd;
        int i;
        u32 hwmode = fs_rl(OTG_CORE_HWMODE);
        unsigned long flags;

        /* puts OTG capable port into a state where host is enabled */

        local_irq_save(flags);
        //fs_andl(OTG_CORE_HWMODE, 0xfffffff0);                  // clear

        switch (flag) {
        case SET:
                TRACE_MSG0(HCD, "SET");
                //if (hwmode & MODULE_CRECFG_HOST) {
                //        printk(KERN_INFO"%s: warning FUNC STILL SET\n", __FUNCTION__);
                //}
                //printk(KERN_INFO"%s: SET\n", __FUNCTION__);
                fs_orl(OTG_CORE_HWMODE, MODULE_CRECFG_HOST);           // set to software hnp
                hcd_instance->active = TRUE;
                if (!tcd->id) {
                        TRACE_MSG1(HCD, "FRM_INTRVL: %8x current", fs_rl(OTG_CORE_FRM_INTVL));
                        //fs_wl(OTG_CORE_FRM_INTVL, 4000 | MODULE_RESET_FRAME);
                        TRACE_MSG1(HCD, "FRM_INTRVL: %8x reset", fs_rl(OTG_CORE_FRM_INTVL));

                }

                break;
        case RESET:
                TRACE_MSG0(HCD, "RESET");
                hcd_instance->active = FALSE;
                fs_andl(OTG_CORE_HWMODE, ~MODULE_CRECFG_HOST);           // set to software hnp
                break;
        }

        local_irq_restore(flags);
        TRACE_MSG1(HCD, "HWMODE: %08x", fs_rl(OTG_CORE_HWMODE));
}

u32 otg_core_frm_intvl;

/*!
 * mxc_hcd_rh_func() - otg hcd root hub output function
 * @param otg
 * @param flag
 */
void mxc_hcd_rh_func(struct otg_instance *otg, u8 flag)
{
        struct hcd_instance *hcd = otg->hcd;
        struct tcd_instance *tcd = otg->tcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;
        //printk(KERN_INFO"%s:\n", __FUNCTION__); 
        int i;  
        u32 mask;
        unsigned long flags;

        RETURN_UNLESS (hcd && mxc_hcd);
        //printk(KERN_INFO"%s:\n", __FUNCTION__); 

        switch (flag) {   
        case SET:
                //printk(KERN_INFO"%s: SET\n", __FUNCTION__);

                local_irq_save(flags);

		/* reset the host core
		 */
                fs_wl_clr(HCD, OTG_CORE_RST_CTRL, MODULE_RSTRH | MODULE_RSTHSIE | MODULE_RSTHC);
                while (fs_rl(OTG_CORE_RST_CTRL));
		
                mxc_host_clock_on();


                for (i = 0; i < NUM_DATA_BUFFS; i++) 
                        (void) rel_data_buff(mxc_hcd, ((fs_data_buff *)OTG_DATA_BASE)+i);

                for (i = 0; i < NUM_ETDS; i++) 
                        rel_etd_irq(mxc_hcd,i);

                fs_wl(OTG_HOST_CONTROL, HOST_CONTROL_HCRESET | HOST_CONTROL_RMTWUEN | 
                                HOST_CONTROL_HCUSBSTE_RESET | HOST_CONTROL_CTLBLKSR_11);


                TRACE_MSG0(HCD, "HW HCD_ENABLE_SET");
                fs_andl(OTG_CORE_HNP_CSTAT, ~(MODULE_MASTER | MODULE_SLAVE | MODULE_CMPEN | 
                                        MODULE_BGEN | MODULE_SWAUTORST | MODULE_ABBUSREQ));

                fs_rl(OTG_CORE_HNP_CSTAT); 

                fs_orl(OTG_CORE_HNP_CSTAT, MODULE_MASTER | MODULE_CMPEN | MODULE_BGEN | MODULE_ABBUSREQ);
                fs_orl(OTG_CORE_HNP_CSTAT, MODULE_ARMTHNPE | MODULE_BHNPEN); // XXX

                fs_wl(OTG_HOST_CONTROL, HOST_CONTROL_HCUSBSTE_OPERATIONAL);

                //hcd_hw_enable_interrupts(bus_hcpriv);

                mxc_hcd->int_mask = (HOST_PSCINT_EN | HOST_FMOFINT_EN | HOST_HERRINT_EN | 
                                HOST_RESDETINT_EN | /* HOST_SOFINT_EN | */ HOST_DONEINT_EN | HOST_SORINT_EN);

                TRACE_MSG1(HCD, "HW HCD_ENABLE_SET: mask: %02x", mxc_hcd->int_mask);

                #if 1
                // R1: sec 23.11.15 pg 23-54
                fs_rl(OTG_CORE_HNP_CSTAT); 
                fs_rl(OTG_HOST_CONTROL);
                fs_rl(OTG_HOST_ROOTHUB_STATUS);
                fs_rl(OTG_HOST_PORT_STATUS_1);
                fs_rl(OTG_HOST_PORT_STATUS_2);
                fs_rl(OTG_HOST_PORT_STATUS_3);

                fs_wl(OTG_HOST_SINT_STEN, mxc_hcd->int_mask);
                fs_rl(OTG_HOST_SINT_STEN);

                mxc_hcd->otg_port_enabled = TRUE;
                //printk(KERN_INFO"%s: is_b_host FALSE\n", __FUNCTION__); 

                // XXX need to get ID_GND 

                mxc_hcd->hcd.self.is_b_host = !tcd->id;

                if (!tcd->id)
                        fs_orl(OTG_CORE_HNP_CSTAT, MODULE_SWAUTORST); // XXX

                for (i = 1; i <= mxc_hcd->bNbrPorts; i++) {
                        mxc_hcd_hw_rh_port_feature(mxc_hcd, PORT_POWER, i, TRUE);
                        //mxc_hcd->hub_port_change_status |= (1 << i);
                }

                #endif
                local_irq_restore(flags);

                break;

        case RESET:

                mxc_hcd->otg_port_enabled = FALSE;              // XXX check this
                mxc_hcd->hcd.self.is_b_host = TRUE;
                //printk(KERN_INFO"%s: is_b_host TRUE\n", __FUNCTION__); 

                #if 0
                local_irq_save(flags);

                //printk(KERN_INFO"%s: RESET\n", __FUNCTION__);
                TRACE_MSG0(HCD, "HW HCD_ENABLE_RESET");

                //hcd_hw_disable_interrupts(bus_hcpriv);
                // R1: sec 23.11.15 pg 23-54
                fs_wl(OTG_HOST_SINT_STAT, 0);

                fs_andl(OTG_HOST_CONTROL, ~HOST_CONTROL_HCUSBSTE_OPERATIONAL);

                // Shut down hardware if not already shut down....
                //hcd_hw_disable_interrupts(bus_hcpriv);

                mxc_host_clock_off();
                local_irq_restore(flags);
                #endif
                break;
        }
}

/*!
 * mxc_loc_sof_func() - otg loc sof output function
 * @param otg
 * @param flag
 */
void mxc_loc_sof_func(struct otg_instance *otg, u8 flag)
{
        struct hcd_instance *hcd = otg->hcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;
        //struct tcd_instance *tcd = otg->tcd;
        int i;
        u32 hwmode = fs_rl(OTG_CORE_HWMODE);
        unsigned long flags;

        RETURN_UNLESS (hcd && mxc_hcd);
        //printk(KERN_INFO"%s:\n", __FUNCTION__); 

        switch (flag) {   
        case SET:
                //printk(KERN_INFO"%s: SET\n", __FUNCTION__);

                local_irq_save(flags);

                #if 0
		/* reset the host core
		 */
                fs_wl_clr(HCD, OTG_CORE_RST_CTRL, MODULE_RSTRH | MODULE_RSTHSIE | MODULE_RSTHC);
                while (fs_rl(OTG_CORE_RST_CTRL));
		
                mxc_host_clock_on();

                for (i = 0; i < NUM_DATA_BUFFS; i++) 
                        (void) rel_data_buff(mxc_hcd, ((fs_data_buff *)OTG_DATA_BASE)+i);

                for (i = 0; i < NUM_ETDS; i++) 
                        rel_etd_irq(mxc_hcd,i);

                fs_wl(OTG_HOST_CONTROL, HOST_CONTROL_HCRESET | HOST_CONTROL_RMTWUEN | 
                                HOST_CONTROL_HCUSBSTE_RESET | HOST_CONTROL_CTLBLKSR_11);


                TRACE_MSG0(HCD, "HW HCD_ENABLE_SET");
                fs_andl(OTG_CORE_HNP_CSTAT, ~(MODULE_MASTER | MODULE_SLAVE | MODULE_CMPEN | 
                                        MODULE_BGEN | MODULE_SWAUTORST | MODULE_ABBUSREQ));
                fs_rl(OTG_CORE_HNP_CSTAT); 

                fs_orl(OTG_CORE_HNP_CSTAT, MODULE_MASTER | MODULE_CMPEN | MODULE_BGEN | MODULE_ABBUSREQ);
                fs_orl(OTG_CORE_HNP_CSTAT, MODULE_ARMTHNPE | MODULE_BHNPEN); // XXX

                fs_wl(OTG_HOST_CONTROL, HOST_CONTROL_HCUSBSTE_OPERATIONAL);

                //hcd_hw_enable_interrupts(bus_hcpriv);

                mxc_hcd->int_mask = (HOST_PSCINT_EN | HOST_FMOFINT_EN | HOST_HERRINT_EN | 
                                HOST_RESDETINT_EN | /* HOST_SOFINT_EN | */ HOST_DONEINT_EN | HOST_SORINT_EN);

                TRACE_MSG1(HCD, "HW HCD_ENABLE_SET: mask: %02x", mxc_hcd->int_mask);


                // R1: sec 23.11.15 pg 23-54
                fs_rl(OTG_CORE_HNP_CSTAT); 
                fs_rl(OTG_HOST_CONTROL);
                fs_rl(OTG_HOST_ROOTHUB_STATUS);
                fs_rl(OTG_HOST_PORT_STATUS_1);
                fs_rl(OTG_HOST_PORT_STATUS_2);
                fs_rl(OTG_HOST_PORT_STATUS_3);

                fs_wl(OTG_HOST_SINT_STEN, mxc_hcd->int_mask);
                fs_rl(OTG_HOST_SINT_STEN);

                mxc_hcd->otg_port_enabled = TRUE;

                for (i = 1; i <= mxc_hcd->bNbrPorts; i++) {
                        mxc_hcd_hw_rh_port_feature(mxc_hcd, PORT_POWER, i, TRUE);
                        //mxc_hcd->hub_port_change_status |= (1 << i);
                }

                #endif
                local_irq_restore(flags);

                break;

        case RESET:
                local_irq_save(flags);

                //printk(KERN_INFO"%s: RESET\n", __FUNCTION__);
                TRACE_MSG0(HCD, "HW HCD_ENABLE_RESET");

                //hcd_hw_disable_interrupts(bus_hcpriv);
                // R1: sec 23.11.15 pg 23-54
                fs_wl(OTG_HOST_SINT_STAT, 0);

                fs_andl(OTG_HOST_CONTROL, ~HOST_CONTROL_HCUSBSTE_OPERATIONAL);

                // Shut down hardware if not already shut down....
                //hcd_hw_disable_interrupts(bus_hcpriv);

                mxc_host_clock_off();
                local_irq_restore(flags);
                break;
        }

}

                
void mxc_hcd_loc_suspend_func(struct otg_instance *otg, u8 on)
{
        struct hcd_instance *hcd = otg->hcd;
        struct mxc_hcd  *mxc_hcd = hcd ? hcd->privdata : NULL;
        int i;
        unsigned long flags;

        RETURN_UNLESS (hcd && mxc_hcd);

        switch (on) {
        case SET:
                TRACE_MSG0(HCD, "OUTPUT: RH LOC_SUSPEND SET");
                break;

        case RESET:
                TRACE_MSG0(HCD, "OUTPUT: RH LOC_SUSPEND RESET");
                break;
        }
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! 
 * mxc_pcd_framenum() - get current framenum
 */             
static u16 
mxc_hcd_framenum (struct otg_instance *instance)
{               
        //printk(KERN_INFO"%s: framenum: %04x\n", __FUNCTION__,    fs_rl(OTG_HOST_FRM_NUM) );
        return fs_rl(OTG_HOST_FRM_NUM);
}



#if !defined(OTG_C99)
#if defined(LINUX26)
int mxc_hcd_mod_init_l26(void);
void mxc_hcd_mod_exit_l26(void);
void mxc_hcd_mod_exit_l26(void);
struct hcd_ops hcd_ops = { mxc_hcd_mod_init_l26, mxc_hcd_mod_exit_l26, };
#else /* defined(LINUX26) */
int mxc_hcd_mod_init_l24(void);
void mxc_hcd_mod_exit_l24(void);
struct hcd_ops hcd_ops = { mxc_hcd_mod_init_l24, mxc_hcd_mod_exit_l24, };
#endif /* defined(LINUX26) */

/*!
 * fs_hcd_global_init() - initialize global vars for non C99 systems
 */
void fs_hcd_global_init(void)
{
        //printk(KERN_INFO"%s:\n", __FUNCTION__); 

        ZERO(hcd_ops);
        hcd_ops.name = "MX21 HCD";
        hcd_ops.max_ports = 1;
        hcd_ops.capabilities = 0;
                                                       // module
        hcd_ops.mod_init = mxc_hcd_mod_init;           // called for module init
#ifdef MODULE
        hcd_ops.mod_exit = mxc_hcd_mod_exit;           // called for module exit
#endif
                                                       // otg state machine
        hcd_ops.hcd_init_func = hcd_init_func;         // initialize when otg enabled
        hcd_ops.hcd_en_func = mxc_hcd_en_func;         // setup hardware as host
        hcd_ops.hcd_rh_func = mxc_hcd_rh_func;         // start root hub
        hcd_ops.loc_suspend_func = mxc_hcd_loc_suspend_func;        // enable port on hub
        hcd_ops.loc_sof_func = mxc_loc_sof_func;       // enable port on hub
        hcd_ops.framenum = mxc_hcd_framenum;
};

/* ********************************************************************************************* */
#else /* !defined(OTG_C99) */
int mxc_hcd_mod_init_l26(void);
void mxc_hcd_mod_exit_l26(void);
struct hcd_ops hcd_ops = {
        .mod_init = mxc_hcd_mod_init_l26,              // called for module init
#ifdef MODULE
        .mod_exit = mxc_hcd_mod_exit_l26,              // called for module exit
#endif

        .name = "MX21 HCD",
        .max_ports = 1,
        .capabilities = 0,
                                                // module
        .hcd_init_func = hcd_init_func,         // initialize when otg enabled
        .hcd_en_func = mxc_hcd_en_func,         // setup hardware as host
        .hcd_rh_func = mxc_hcd_rh_func,         // start root hub
        .loc_suspend_func = mxc_hcd_loc_suspend_func,  // enable port on hub
        .loc_sof_func = mxc_loc_sof_func,        // enable port on hub
        .framenum = mxc_hcd_framenum,
};
#endif /* !defined(OTG_C99) */


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*! Linux 2.6 USB Device Support
 * New style linux 2.6.10 USB Core support
 */

/*
 * From drivers/usb/host/hcd.c:
 *    disables the endpoint: cancels any pending urbs, then synchronizes with
 *    the hcd to make sure all endpoint state is gone from hardware. use for
 *    set_configuration, set_interface, driver removal, physical disconnect.
 */
void mxc_hcd_endpoint_disable(struct usb_hcd *hcd, struct hcd_dev *hdev, int epnum)
{
        struct mxc_hcd          *mxc_hcd = hcd_to_mxc(hcd);
        struct mxc_req          *mxc_req;
        unsigned long           flags;
        int                     is_out = !(epnum & USB_DIR_IN);

        TRACE_MSG0(HCD, "--");
        local_irq_save(flags);
        if ((mxc_req = hdev->ep[EPNUM(epnum, is_out)])) {
                //printk(KERN_INFO"%s: disabled\n", __FUNCTION__); 
                rel_etd_irq(mxc_hcd, mxc_req->etdn);
                hdev->ep[EPNUM(epnum, is_out)] = NULL;

                // XXX giveback?
                mxc_hcd_giveback_req_irq(mxc_hcd, mxc_req, 0);  // check if 0 is correct status to return
        }
        local_irq_restore(flags);
}

int mxc_rh_frame(struct usb_hcd *hcd)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);
        TRACE_MSG0(HCD, "--");
        return fs_rl(OTG_HOST_FRM_NUM);
}

static void
mxc_rh_descriptor ( struct mxc_hcd *mxc_hcd, struct usbp_hub_descriptor *desc) 
{
        u16             temp = 0;

        desc->bDescLength = 9;
        desc->bDescriptorType = 0x29;

        desc->bNbrPorts = mxc_hcd->bNbrPorts;
        desc->wHubCharacteristics = mxc_hcd->wHubCharacteristics;
        desc->bPwrOn2PwrGood = mxc_hcd->bPwrOn2PwrGood;
        desc->bHubContrCurrent = 0;
        desc->DeviceRemovable = mxc_hcd->DeviceRemovable;
        desc->PortPwrCtrlMask = mxc_hcd->PortPwrCtrlMask;
}


/*! mxc_rh_status_data() - provide hub status data to usb core root hub
 *
 * This function is used by the linux 2.6 virtual root hub to poll for 
 * changes.
 *
 */
int mxc_rh_status_data(struct usb_hcd *hcd, char *buf)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);

        RETURN_ZERO_UNLESS(mxc_hcd->otg_port_enabled);
        RETURN_ZERO_UNLESS(mxc_hcd->virt_hub_port_change_status);

        *buf = mxc_hcd->virt_hub_port_change_status;
        mxc_hcd->virt_hub_port_change_status = 0;

        TRACE_MSG1(HCD, "changed buf: %02x", *buf);
        return 1;
}

int mxc_rh_control( struct usb_hcd  *hcd, u16 typeReq, u16 wValue, u16 wIndex, char *buf, u16 wLength)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);
        unsigned long   flags;
        u32             status;

        TRACE_MSG4(HCD, "typeReq: %04x wValue: %04x wIndex: %04x wLength: %04x", typeReq, wValue, wIndex, wLength);

        switch (typeReq) {
        case ClearHubFeature:
        case SetHubFeature:
                TRACE_MSG0(HCD, "ClearHubFeature/SetHubFeature"); 
                //mxc_rh_hub_feature(mxc_hcd, wValue, wIndex, typeReq == SetHubFeature);
                return 0;

        case SetPortFeature: {
                u32 virt_port_status;
                TRACE_MSG0(HCD, "SetPortFeature"); 
                RETURN_EPIPE_IF (wIndex > mxc_hcd->bNbrPorts || wLength);
                virt_port_status = mxc_hcd->virt_port_status[wIndex - 1];
                /* Feature selector is wValue, wIndex is 1-origin
                 */
                // SET feature
                switch (wValue) {
                case PORT_RESET:
                        mxc_hcd->virt_port_status[wIndex - 1] |= (1 << C_PORT_RESET | (1 << wValue));
                        mxc_hcd->virt_hub_port_change_status |= (1 << wIndex);
                        break;
                case PORT_SUSPEND:
                case PORT_POWER:
                        mxc_hcd->virt_port_status[wIndex - 1] |= (1 << wValue);
                        break;
                }
                TRACE_MSG3(HCD, "[%d]: %08x -> %08x", wIndex, virt_port_status, mxc_hcd->virt_port_status[wIndex -1]);
                //printk(KERN_INFO"%s: SetPortFeature[%d]: wValue: %d %08x -> %08x %s\n", 
                //                __FUNCTION__, wIndex, wValue, virt_port_status, mxc_hcd->virt_port_status[wIndex -1],
                //                port_feature_name[wValue]);
                return 0;
        }
        case ClearPortFeature: {
                u32 virt_port_status;
                TRACE_MSG0(HCD, "ClearPortFeature"); 
                RETURN_EPIPE_IF ((wIndex > mxc_hcd->bNbrPorts) || wLength);
                virt_port_status = mxc_hcd->virt_port_status[wIndex - 1];
                /* Feature selector is wValue, wIndex is 1-origin
                 */
                // CLEAR feature (valid features from USB2.0 11.24.2.2 pg 423).
                switch (wValue) {
                case PORT_ENABLE: // Disable port.
                case PORT_SUSPEND: // Cause a Host initiated resume, or no-op if already active.
                case PORT_POWER: // Put port in powered-off state.
                case C_PORT_CONNECTION: // clear the PORT_CONNECTION change bit
                case C_PORT_RESET: // clear the PORT_RESET change bit
                case C_PORT_ENABLE: // clear the PORT_ENABLE change bit
                case C_PORT_SUSPEND: // clear the PORT_SUSPEND change bit
                case C_PORT_OVER_CURRENT: // clear the PORT_OVERCURRENT change bit
                        mxc_hcd->virt_port_status[wIndex - 1] &= ~(1 << wValue);
                        break; 
                }
                TRACE_MSG3(HCD, "[%d]: %08x -> %08x", wIndex, virt_port_status, mxc_hcd->virt_port_status[wIndex -1]);
                //printk(KERN_INFO"%s: ClearPortFeature[%d]: wValue: %d %08x -> %08x %s\n", 
                //                __FUNCTION__, wIndex, wValue, virt_port_status, mxc_hcd->virt_port_status[wIndex -1],
                //                port_feature_name[wValue]);
                return 0;
        }

        case GetHubDescriptor:
                //printk(KERN_INFO"%s: GetHubDescriptor\n", __FUNCTION__); 
                TRACE_MSG0(HCD, "GetHubDescriptor"); 
                mxc_rh_descriptor(mxc_hcd, (struct usbp_hub_descriptor *) buf);
                return 0;

        case GetHubStatus:
                //printk(KERN_INFO"%s: GetHubStatus\n", __FUNCTION__); 
                //*(__le32 *) buf = cpu_to_le32(0);       // XXX this seems to be de rigeur for 2.6 root hubs...
                *(__le32 *) buf = cpu_to_le32(fs_rl(OTG_HOST_ROOTHUB_STATUS) & 0xffff);
                TRACE_MSG1(HCD, "GetHubStatus: %04x", *(__le32 *)buf); 
                return 0;

        case GetPortStatus:
                //printk(KERN_INFO"%s: GetPortStatus\n", __FUNCTION__); 
                TRACE_MSG0(HCD, "GetPortStatus"); 
                RETURN_EPIPE_IF ((wIndex > mxc_hcd->bNbrPorts));
                /* use saved port change status and reset it
                 */
                status = mxc_hcd->virt_port_status[wIndex - 1]; // | fs_rl(fs_host_port_stat(wIndex))
                *(__le32 *) buf = cpu_to_le32( status); 

                TRACE_MSG3(HCD, "GetPortStatus: port: %d port_change_status: %08x buf: %08x", 
                                wIndex, status, *(__le32 *) buf); 

                //printk(KERN_INFO"%s: GetPortStatus port: %d status: %08x\n", 
                //                __FUNCTION__, wIndex, status); 
                
                /* Mini-state machine to progress to PORT_CONNECT in virtual root hub port */
                if (status & (1 << PORT_RESET)) {
                        mxc_hcd->virt_port_status[wIndex - 1] &= ~(1 << PORT_RESET);
                        mxc_hcd->virt_port_status[wIndex - 1] |= 
                                (1 << C_PORT_RESET) |
                                (1 << C_PORT_ENABLE) |
                                (1 << PORT_ENABLE)
                                ;
                        mxc_hcd->virt_hub_port_change_status |= (1 << wIndex);
                        //printk(KERN_INFO"%s: GetPortStatus port: %d status: %08x CLEARED PORT_RESET\n", 
                        //                __FUNCTION__, wIndex, mxc_hcd->virt_port_status[wIndex - 1]); 
                }
                return 0;

        default:
                //printk(KERN_INFO"%s: Default\n", __FUNCTION__); 
                /* "protocol stall" on error */
                return -EPIPE;
        }
}

int mxc_rh_suspend(struct usb_hcd *hcd)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);
        TRACE_MSG0(HCD, "--");
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        return 0;
}

int mxc_rh_resume(struct usb_hcd *hcd)
{
        struct mxc_hcd  *mxc_hcd = hcd_to_mxc(hcd);
        TRACE_MSG0(HCD, "--");
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        return 0;
}


static struct hc_driver mxc_hc_driver = {
        .description =          "mxc-hcd",
        
        /*
         * generic hardware linkage
         */             
        .flags =                HCD_USB11,
        
        /*
         * managing i/o requests and associated device resources
         */
        .urb_enqueue =          mxc_hcd_urb_enqueue,
        .urb_dequeue =          mxc_hcd_urb_dequeue,
        .endpoint_disable =     mxc_hcd_endpoint_disable,
        
        /* 
         * periodic schedule support 
         */     
        .get_frame_number =     mxc_rh_frame,
                
        /*
         * root hub support
         */
        .hub_status_data =      mxc_rh_status_data,
        .hub_control =          mxc_rh_control,
        .hub_suspend =          mxc_rh_suspend,
        .hub_resume =           mxc_rh_resume,
};              
        
/* ********************************************************************************************* */
/*! Linux 2.6 Device Driver Support
 *
 * The Linux 2.6 Core requires that host driver be registered as a device driver. These
 * implement the requirements for the Linux Device Driver support.
 *
 * N.B. currently we are using the probe function to configure the host hardware.
 *
 * XXX The iomux need to be moved to arm/mxc-xxx/gpio-xxx.c 
 */

static void
mxc_hcd_stop(struct usb_hcd *hcd)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        TRACE_MSG0(HCD, "--");
        del_timer_sync(&hcd->rh_timer);
}

static int __init_or_module
mxc_hcd_remove(struct device *dev)
{
        struct mxc_hcd          *mxc_hcd = NULL;

        printk(KERN_INFO"%s: AAAA\n", __FUNCTION__); 
        TRACE_MSG0(HCD, "--");

        RETURN_EINVAL_UNLESS((mxc_hcd = dev_get_drvdata(dev)));
#if 0

        usb_disconnect(&mxc_hcd->hcd.self.root_hub);

        usb_deregister_bus(&mxc_hcd->hcd.self);


        mxc_hcd_stop(&mxc_hcd->hcd);

        //mxc_hcd->hcd.self.release = &usb_hcd_release;


        //mxc_hcd_mode_exit_l24();
        hcd_instance->privdata = NULL;

        // remove requests from unused

        // XXX free mxc_hcd

        LKFREE(mxc_hcd);
#else
	
	if (HCD_IS_RUNNING(mxc_hcd->hcd.state))
		mxc_hcd->hcd.state = USB_STATE_QUIESCING;

	usb_disconnect(&mxc_hcd->hcd.self.root_hub);
	
	mxc_hcd_stop(&mxc_hcd->hcd);

	if (!list_empty(&mxc_hcd->hcd.self.bus_list))
		usb_deregister_bus(&mxc_hcd->hcd.self);

	if (mxc_hcd->hcd.irq >= 0)
		free_irq(mxc_hcd->hcd.irq, mxc_hcd);
       
	kfree(mxc_hcd);
#endif

        printk(KERN_INFO"%s: BBBB\n", __FUNCTION__); 

        return 0;
}



static int
mxc_hcd_start(struct usb_hcd *hcd)
{
        struct usb_device       *udev = NULL;

        /* chip has been reset, VBUS power is off */


        RETURN_ENOMEM_UNLESS((udev = usb_alloc_dev(NULL, &hcd->self, 0)));

        udev->speed = USB_SPEED_FULL;
        hcd->state = USB_STATE_RUNNING;
        hcd->can_wakeup = FALSE; // TRUE

        //if (hcd->board)
        //        hcd->hcd.can_wakeup = hcd->board->can_wakeup;

        THROW_IF (hcd_register_root(udev, hcd), error);

        //if (hcd->board && hcd->board->power)
        //        hub_set_power_budget(udev, hcd->board->power * 2);
        return 0;
        CATCH(error) {
                //printk(KERN_INFO"%s: ZZZZ\n", __FUNCTION__);
                usb_put_dev(udev);
                mxc_hcd_stop(hcd);
                return -ENODEV;
        }
}

//void usb_hcd_release(struct usb_bus *bus);
static void mxc_hcd_release(struct usb_bus *bus)
{
        printk(KERN_INFO"%s: AAAA\n", __FUNCTION__);
        TRACE_MSG0(HCD, "--");
        //usb_hcd_release((struct usb_bus *) dev);
}

static void mxc_dev_release(struct device *device)
{
        printk(KERN_INFO"%s: AAAA\n", __FUNCTION__);
        TRACE_MSG0(HCD, "--");
        //usb_hcd_release((struct usb_bus *) dev);
}

/*
void usb_hcd_release(struct usb_bus *bus);
static void test_hcd_release(struct device *dev)
{
        printk(KERN_INFO"%s: \n", __FUNCTION__);
        usb_hcd_release((struct usb_bus *) dev);
}
*/


static int __init
mxc_hcd_probe(struct device *dev)
{               
        struct platform_device  *platform_device = NULL;
        struct mxc_hcd          *mxc_hcd = NULL;
        //struct urb              **active_urbs = NULL:
        //struct usb_device       *mxc_usb_device = NULL;
        int                     i;

        dev->release = mxc_dev_release;

        // XXX this is not correct
        // dev->dma_mask = 0xffffffffffffffff;

        THROW_UNLESS((mxc_hcd = CKMALLOC(sizeof (struct mxc_hcd), GFP_KERNEL)), error);

        INIT_LIST_HEAD(&mxc_hcd->unused);
        INIT_LIST_HEAD(&mxc_hcd->inactive);

        platform_device = container_of(dev, struct platform_device, dev);
        dev_set_drvdata(dev, mxc_hcd);

        usb_bus_init(&mxc_hcd->hcd.self);
        mxc_hcd->hcd.self.controller = dev;
        mxc_hcd->hcd.self.bus_name = dev->bus_id;
        mxc_hcd->hcd.self.op = &usb_hcd_operations;
        mxc_hcd->hcd.self.hcpriv = mxc_hcd;
        mxc_hcd->hcd.self.is_b_host = TRUE;
        mxc_hcd->hcd.self.otg_port = 1;
        mxc_hcd->root_hub_desc_a = fs_rl(OTG_HOST_ROOTHUB_DESCA);
        mxc_hcd->root_hub_desc_b = fs_rl(OTG_HOST_ROOTHUB_DESCB);
        mxc_hcd->bNbrPorts = mxc_hcd->root_hub_desc_a & 0xff;
        mxc_hcd->wHubCharacteristics = (mxc_hcd->root_hub_desc_a >> 8) & 0xff;
        mxc_hcd->bPwrOn2PwrGood = (mxc_hcd->root_hub_desc_a >> 24) & 0xff;
        mxc_hcd->DeviceRemovable = mxc_hcd->root_hub_desc_b & 0xff;
        mxc_hcd->PortPwrCtrlMask = (mxc_hcd->root_hub_desc_b >> 16) & 0xff;
        mxc_hcd->allocated_count = mxc_hcd->active_count = 0;

        TRACE_MSG2(HCD, "root_hub descA: %08x descB: %08x", mxc_hcd->root_hub_desc_a, mxc_hcd->root_hub_desc_b);

        TRACE_MSG5(HCD, "bNbrPorts: %02x wHubCharacteristics: %02x bPwrOn2PwrGood: %02x "
                        "DeviceRemovalbe: %02x PortPwrCtrlMask: %02x", 
                        mxc_hcd->bNbrPorts,
                        mxc_hcd->wHubCharacteristics,
                        mxc_hcd->bPwrOn2PwrGood,
                        mxc_hcd->DeviceRemovable,
                        mxc_hcd->PortPwrCtrlMask
                        );


        INIT_LIST_HEAD(&mxc_hcd->hcd.dev_list);
        //mxc_hcd->hcd.self.release = &usb_hcd_release;
        mxc_hcd->hcd.self.release = &mxc_hcd_release;

        // mxc_hcd->hcd.description = mxc_hc_driver.description;
        mxc_hcd->hcd.product_desc = "MXC HCD";

        init_timer(&mxc_hcd->hcd.rh_timer);
        mxc_hcd->hcd.driver = &mxc_hc_driver;
        mxc_hcd->hcd.irq = -1;
        mxc_hcd->hcd.state = USB_STATE_HALT;

        spin_lock_init(&mxc_hcd->lock);

        #if 0
        for (i = 0; i < 16; i++) {
                INIT_LIST_HEAD(&mxc_hcd->queued_urbs_both[0][i]);
                INIT_LIST_HEAD(&mxc_hcd->queued_urbs_both[1][i]);
        }
        #endif

        //mxc_hcd->first_dev = dev;

        THROW_IF(usb_register_bus(&mxc_hcd->hcd.self), error);

        hcd_instance->privdata = &mxc_hcd->hcd;

        THROW_IF(mxc_hcd_start(&mxc_hcd->hcd), error);

        /*
         * Configure Host ports #2 / #3
         */
       #ifdef CONFIG_ARCH_MXC91231
        iomux_config_mux(AP_IPU_LD8, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);         // UH1_TXOE_B
        iomux_config_mux(AP_IPU_LD9, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);         // UH1_SPEED
        iomux_config_mux(AP_IPU_LD10, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);         // UH1_SUSPEND
        iomux_config_mux(AP_IPU_LD11, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);        // UH1_TXDP
        iomux_config_mux(AP_IPU_LD12, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);        // UH1_TXDM
        iomux_config_mux(AP_IPU_LD13, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);        // UH1_RXD
        iomux_config_mux(AP_IPU_LD14, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);        // UH1_RCDP
        iomux_config_mux(AP_IPU_LD15, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);        // UH1_RXDM

        iomux_config_mux(AP_GPIO_AP_B22, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);        // UH1_OVR
        iomux_config_mux(AP_GPIO_AP_B23, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);        // UH1_PWR


        iomux_config_mux(SP_UH2_TXOE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_UH2_SPEED, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_UH2_SUSPEND, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_UH2_TXDP, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_UH2_TXDM, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_UH2_RXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_UH2_RXDP, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_UH2_RXDM, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_UH2_OVR, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_UH2_PWR, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);


        #endif /* CONFIG_ARCH_MXC91231 */

        #ifdef CONFIG_ARCH_MXC91321
        //iomux_config_mux(PIN_GPIO3,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);      // USB1_SPEED
        //iomux_config_mux(PIN_GPIO4,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);      // USB1_PWR

        //iomux_config_mux(PIN_GPIO5,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);      // USB1_OC
        //iomux_config_mux(PIN_GPIO5,  OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);      // USB1_OC
        //iomux_config_mux(PIN_GPIO5,  OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);      // USB1_OC
        iomux_config_mux(PIN_GPIO5,  OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);      // USB1_OC

        printk(KERN_INFO"%s: NONE/GPIO\n", __FUNCTION__); 
        #endif /* CONFIG_ARCH_MXC91321 */

        #ifdef CONFIG_ARCH_MXC91131
        iomux_config_mux(UH1_OVR_PIN, MUX0_OUT, MUX0_IN);
        iomux_config_mux(UH2_OVR_PIN, MUX0_OUT, MUX0_IN);
        printk(KERN_INFO"%s: MXC91131 GPIO\n", __FUNCTION__);
        #endif /* CONFIG_ARCH_MXC91131 */

        #if 0
        for (i = 1; i <= mxc_hcd->bNbrPorts; i++) {
                //mxc_hcd_hw_rh_port_feature(mxc_hcd, PORT_RESET, i, TRUE);
                mxc_hcd_hw_rh_port_feature(mxc_hcd, C_PORT_RESET, i, FALSE);
                mxc_hcd_hw_rh_port_feature(mxc_hcd, C_PORT_OVER_CURRENT, i, FALSE);
        }

        for (i = 1; i <= mxc_hcd->bNbrPorts; i++) {
                mxc_hcd_hw_rh_port_feature(mxc_hcd, PORT_POWER, i, TRUE);
                mxc_hcd_hw_rh_port_feature(mxc_hcd, PORT_ENABLE, i, TRUE);
        }
        #endif

        return 0;


        //mxc_hcd_mode_init_l24();
        CATCH(error) {
                //usb_put_dev(mxc_usb_device);
                if (mxc_hcd) {
                        if (mxc_hcd) mxc_hcd_stop(&mxc_hcd->hcd);
                         LKFREE(mxc_hcd);
                }
                //if (active_urbs) LKFREE(active_urbs);
                return -ENODEV;
        }
}
static int
mxc_hcd_suspend(struct device *dev, u32 state, u32 phase)
{       
        //printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return -EINVAL;
}               
        
static int
mxc_hcd_resume(struct device *dev, u32 phase)
{               
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return -EINVAL;
}               

/*! mxc_hcd_driver - define a device_driver structure for this driver
 */
static struct device_driver mxc_hcd_driver = {

        .name =         "mxc-hcd",
        .bus =          &platform_bus_type,

        .probe =        mxc_hcd_probe,
        .remove =       mxc_hcd_remove,

        .suspend =      mxc_hcd_suspend,
        .resume =       mxc_hcd_resume,

};

/*! mxc_hcd_platform - define a platform_device structure for this driver
 */ 
static struct platform_device mxc_hcd_platform = {
        .name       = "mxc-hcd",
        .id         = 1,
};

/* ********************************************************************************************* */
/* ********************************************************************************************* */

extern char *etd_urb_state_name[];

static int mxc_hcd_show(struct seq_file *s, void *unused)
{
        struct mxc_hcd          *mxc_hcd = hcd_instance->privdata;      // XXX this should come from dev_id
        struct mxc_req          *mxc_req;
        struct mxc_req          *mxc_req_save;

        int                     etdn;
        unsigned long           flags;


        seq_printf(s, "MXC HCD\n\n");
 
        for (etdn = 0; etdn < NUM_ETDS; etdn++) {

                local_irq_save(flags);

                if ((mxc_req = mxc_hcd->active[etdn])) {
                        seq_printf(s, "[%02x] Active urb: %08x epnum: %02x state: %s\n", etdn, 
                                        mxc_req->urb, 
                                        mxc_req->epnum, 
                                        etd_urb_state_name[mxc_req->etd_urb_state] 
                                        );
                }

                local_irq_restore(flags);

        }
        seq_printf(s, "\n");

        local_irq_save(flags);
        etdn = 0;
        list_for_each_entry(mxc_req, &mxc_hcd->inactive, queue) {
                seq_printf(s, "[%02x] Inactive urb: %08x epnum: %02x state: %s\n", etdn++, 
                                mxc_req->urb, 
                                mxc_req->epnum, 
                                etd_urb_state_name[mxc_req->etd_urb_state] 
                          );
        }
        local_irq_restore(flags);

        seq_printf(s, "\n");
        seq_printf(s, "Requests: Allocated: %d Active: %d Inactive: %d Total: %d\n", 
                        mxc_hcd->allocated_count, mxc_hcd->active_count, etdn, mxc_hcd->request_count);

        seq_printf(s, "\n");
        return 0;
}


static int mxc_hcd_open(struct inode *inode, struct file *file)
{
        return single_open(file, mxc_hcd_show, PDE(inode)->data);
}

static struct file_operations mxc_hcd_proc_ops = {
        .open           = mxc_hcd_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static const char proc_filename[] = "mxchcd";


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*! Module init/exit
 *
 * Register this a linux device driver and platform. This provides the necessary framework
 * expected by the Linux 2.6 USB Core.
 *
 */


int mxc_hcd_mod_init_l26(void)
{
        u32 hwmode;
        int driver_registered = 0;
        int platform_registered = 0;
        struct proc_dir_entry *pde = NULL;

        printk(KERN_INFO"%s: MXC-HCD.C (dev)\n", __FUNCTION__); 

        if (usb_disabled()) {
                //printk(KERN_INFO"%s: YYYYY\n", __FUNCTION__); 
                return -ENOMEM;
        }

        THROW_IF((driver_registered = driver_register(&mxc_hcd_driver)), error);

        THROW_IF((platform_registered = platform_registered = platform_device_register(&mxc_hcd_platform)), error);

        THROW_UNLESS((pde = create_proc_entry(proc_filename, 0, NULL)), error);
        pde->proc_fops = &mxc_hcd_proc_ops;

        CATCH(error) {
                //printk(KERN_INFO"%s: ZZZZ\n", __FUNCTION__); 
                if (platform_registered) platform_device_unregister(&mxc_hcd_platform);
                if (driver_registered) driver_unregister(&mxc_hcd_driver);
                if (pde) remove_proc_entry(proc_filename, NULL);
        }

        return 0;
}

#ifdef MODULE
void mxc_hcd_mod_exit_l26(void)
{
        printk(KERN_INFO"%s: AAAA\n", __FUNCTION__); 
        remove_proc_entry(proc_filename, NULL);

        printk(KERN_INFO"%s: driver_unregister BBBB\n", __FUNCTION__); 
        driver_unregister(&mxc_hcd_driver);

        printk(KERN_INFO"%s: platform_device_unregister CCCC\n", __FUNCTION__); 
        platform_device_unregister(&mxc_hcd_platform);

        printk(KERN_INFO"%s: DDDD\n", __FUNCTION__); 
		
}
#endif

