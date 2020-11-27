/*
 * otg/ocd/brassboard/brassboard-tr.c - ZAS OTG Peripheral and OTG Controller Drivers Module Initialization
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 06/09/2006         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
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
/*!
 * @defgroup BRASSBOARD Motorola Brassboard
 * @ingroup platformgroup
 */
/*!
 * @file otg/ocd/brassboard/brassboard-tr.c
 * @brief BRASSBOARD USB Host Controller Driver
 *
 *
 * ZAS OTG PCD/HCD/OCD/TCD Initialization
 *
 * This file initializes all of the low level hardware drivers for the Motorola Phone Board.
 *
 * 10/07/2005 - intialized the function pointer 'pm_iomux_config' to configure USB XCVR (iomux etc) after OS sleep modes.
 * @ingroup BRASSBOARD
 * 
 */

/*
#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-bus.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-tcd.h>
#include <otg/otg-pcd.h>
#include <otg/otg-ocd.h>
*/

#include <otg/pcd-include.h>
#include <linux/config.h>
#include <linux/module.h>

#include <linux/pci.h>
#include <asm/arch/gpio.h>
#include <asm/arch/board.h>

#include <otghw/mxc-hardware.h>

MOD_AUTHOR ("sl@belcarra.com");                 
MOD_DESCRIPTION ("Belcarra MXC");
EMBED_LICENSE();                        


MOD_PARM_STR(serial_number_str, "Serial Number String", NULL);

/* OTG Driver 
 */
otg_tag_t OCD;
struct ocd_instance *ocd_instance;
extern struct ocd_ops ocd_ops;

/* Transceiver Driver
 */
otg_tag_t TCD;
struct tcd_instance *tcd_instance;
extern struct tcd_ops tcd_ops;


/* Peripheral Driver
 */
otg_tag_t PCD;
struct pcd_instance *pcd_instance;
#if !defined(CONFIG_USB_HOST)
extern struct pcd_ops pcd_ops;
#else /* !defined(CONFIG_USB_HOST) */
irqreturn_t mxc_pcd_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
        return IRQ_HANDLED;
}
#endif /* !defined(CONFIG_USB_HOST) */

/* Host Driver
 */
otg_tag_t HCD;          
struct hcd_instance *hcd_instance;
#if defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE)
extern struct hcd_ops hcd_ops;

#else /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */
irqreturn_t hcd_hw_int_hndlr(int irq, void *dev_id, struct pt_regs *regs)
{
        return IRQ_HANDLED;
}
#endif /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */

/* ************************************************************************************* */

#if defined(CONFIG_OTG_GPTR)
extern int mxc_gptcr_mod_init (void);
void mxc_gptcr_mod_exit (void);
#endif /* defined(CONFIG_OTG_GPTR) */

#if defined(CONFIG_OTG_HRT)
extern int mxc_hrt_mod_init (void);
void mxc_hrt_mod_exit (void);
#endif /* defined(CONFIG_OTG_GPTR) */


/*!
 * brassboard_modexit() - This is used as module exit, and as cleanup if modinit fails.
 */
static void brassboard_modexit (void)
{
        struct otg_instance *otg = ocd_instance->otg;

        printk(KERN_INFO"%s: AAAA\n", __FUNCTION__);
        
        if (otg)
                otg_exit(otg);
        

        printk(KERN_INFO"%s: AAAA\n", __FUNCTION__); 

        #if !defined(CONFIG_USB_HOST)
        if (pcd_ops.mod_exit) pcd_ops.mod_exit();
        pcd_instance = otg_set_pcd_ops(NULL);
        PCD = otg_trace_invalidate_tag(PCD);
        #else /* !defined(CONFIG_USB_HOST) */
        printk(KERN_INFO"%s: PCD DRIVER N/A\n", __FUNCTION__); 
        #endif /* !defined(CONFIG_USB_HOST) */
        		
        printk(KERN_INFO"%s: BBBB\n", __FUNCTION__); 

        #if defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE)
        if (hcd_ops.mod_exit) hcd_ops.mod_exit();
        hcd_instance = otg_set_hcd_ops(NULL);
        HCD = otg_trace_invalidate_tag(HCD);
        #else /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */
        printk(KERN_INFO"%s: HCD DRIVER N/A\n", __FUNCTION__); 
        #endif /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */

        printk(KERN_INFO"%s: CCCC\n", __FUNCTION__); 

        if (tcd_ops.mod_exit) tcd_ops.mod_exit();
        tcd_instance = otg_set_tcd_ops(NULL);
        TCD = otg_trace_invalidate_tag(TCD);
       
        printk(KERN_INFO"%s: DDDD\n", __FUNCTION__); 

        if (ocd_ops.mod_exit) ocd_ops.mod_exit();
        ocd_instance = otg_set_ocd_ops(NULL);
        OCD = otg_trace_invalidate_tag(OCD);
        
        #if defined(CONFIG_OTG_HRT)
        mxc_hrt_mod_exit();
        #endif /* defined(CONFIG_OTG_GPTR) */
        printk(KERN_INFO"%s: EEEE\n", __FUNCTION__); 


}

/*!
 * brassboard_modinit() - linux module initialization
 *
 * This needs to initialize the hcd, pcd and tcd drivers. This includes tcd and possibly hcd
 * for some architectures.
 *
 */
static int brassboard_modinit (void)
{
        struct otg_instance *otg;

        printk(KERN_INFO"%s: AAAAEntry\n", __FUNCTION__);


#ifdef OTG_USE_I2C
        i2c_mod_init();
#endif

        TRACE_MSG0(OCD, "2. Setup HRT");

        #if defined(CONFIG_OTG_HRT)
        mxc_hrt_mod_init();
        #endif /* defined(CONFIG_OTG_GPTR) */

        OCD = otg_trace_obtain_tag();
        TRACE_MSG0(OCD, "1. OCD");


        THROW_UNLESS(ocd_instance = otg_set_ocd_ops(&ocd_ops), error);
        THROW_IF((ocd_ops.mod_init ? ocd_ops.mod_init() : 0), error);

        TRACE_MSG0(PCD, "3. PCD");
        #if !defined(CONFIG_USB_HOST)
        PCD = otg_trace_obtain_tag();
        THROW_UNLESS(pcd_instance = otg_set_pcd_ops(&pcd_ops), error);
        THROW_IF((pcd_ops.mod_init ? pcd_ops.mod_init() : 0), error);
        #else /* !defined(CONFIG_USB_HOST) */
        printk(KERN_INFO"%s: PCD DRIVER N/A\n", __FUNCTION__); 
        #endif /* !defined(CONFIG_USB_HOST) */

        TRACE_MSG0(PCD, "4. PCD");
        TCD = otg_trace_obtain_tag();
        THROW_UNLESS(tcd_instance = otg_set_tcd_ops(&tcd_ops), error);
        printk(KERN_INFO"%s: PCD DRIVER N/A\n", __FUNCTION__); 

        THROW_IF((tcd_ops.mod_init ? tcd_ops.mod_init() : 0), error);

        TRACE_MSG0(PCD, "5. check");

        #if defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE)
        HCD = otg_trace_obtain_tag();
        THROW_UNLESS(hcd_instance = otg_set_hcd_ops(&hcd_ops), error);
        THROW_IF((hcd_ops.mod_init) ? hcd_ops.mod_init() : 0, error);
        #else /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */
        printk(KERN_INFO"%s: PCD DRIVER N/A\n", __FUNCTION__); 
        #endif /* defined(CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)|| defined(CONFIG_OTG_DEVICE) */

        TRACE_MSG0(PCD, "6. check");
        THROW_UNLESS(ocd_instance && (otg = ocd_instance->otg), error);



/* The pointer 'pm_iomux_config' is used to point to the function that would 
 * configure the iomuxes required for the XCVR to work in USB mode.
 */	
	if(tcd_ops.mod_init) {
		pm_iomux_config = tcd_ops.mod_init;
		TRACE_MSG0(OCD, "pm_iomux_ptr INIT");
	}
        TRACE_MSG0(PCD, "7. otg_init");

        if (MODPARM(serial_number_str) && strlen(MODPARM(serial_number_str))) {

                TRACE_MSG1(PCD, "serial_number_str: %s", MODPARM(serial_number_str));
                otg_serial_number (otg, MODPARM(serial_number_str));
        }

        otg_init(otg);
        return 0;

        CATCH(error) {
        	printk(KERN_INFO"\n\n****Error INSMODing brassboard_modinit****\n\n");
                brassboard_modexit();
                return -EINVAL;
        }
}
#ifdef CONFIG_OTG_NFS
late_initcall (brassboard_modinit);
#else
module_init (brassboard_modinit);
#endif
module_exit (brassboard_modexit);



