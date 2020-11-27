/*
 * otg/ocd/zasevb/zasevb-l26.c - ZAS EVB OTG Peripheral and OTG Controller Drivers Module Initialization
 * @(#) balden@seth2.belcarratech.com|otg/ocd/zasevb/zasevb-l26.c|20051116205001|09345
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
 * 06/12/2006         Motorola         OTG name change 
 * 08/09/2006         Motorola         nfs over USB 
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
 * @defgroup ZASEVB Freescale ZAS EVB
 * @ingroup platformgroup
 */
/*!
 * @file otg/ocd/zasevb/zasevb-l26.c
 * @brief ZAS EVB USB Host Controller Driver
 *
 *
 * ZAS OTG PCD/HCD/OCD/TCD Initialization
 *
 * This file initializes all of the low level hardware drivers for the ZAS EVB.
 *
 * Notes.
 *
 *
 * @ingroup ZASEVB
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

#if defined(CONFIG_OTG_ZASEVB_ISP1301) || defined(CONFIG_OTG_ZASEVB_ISP1301_MODULE)
#define OTG_USE_I2C
#else
#define OTG_DISABLE_I2C
#endif
#ifdef OTG_USE_I2C
extern int i2c_mod_init(void);
extern void i2c_mod_exit(void);
#endif
extern int mxc_procfs_init(void);
extern void mxc_procfs_exit(void);
//extern void fs_mod_exit(void);

#if !defined(OTG_C99)
extern void pcd_global_init(void);
extern void fs_ocd_global_init(void);
extern void zasevb_tcd_global_init(void);
extern void fs_pcd_global_init(void);
#endif /* !defined(OTG_C99) */

#if defined(CONFIG_OTG_GPTR)
extern int mxc_gptcr_mod_init (int divisor, int multiplier);
void mxc_gptcr_mod_exit (void);
#endif /* defined(CONFIG_OTG_GPTR) */

#if defined(CONFIG_OTG_HRT)
extern int mxc_hrt_mod_init (int divisor, int multiplier);
void mxc_hrt_mod_exit (void);
#endif /* defined(CONFIG_OTG_GPTR) */


/*!
 * zasevb_modexit() - This is used as module exit, and as cleanup if modinit fails.
 */
static void zasevb_modexit (void)
{
        struct otg_instance *otg = ocd_instance->otg;

        //printk(KERN_INFO"%s: AAAA\n", __FUNCTION__);
        
        if (otg)
                otg_exit(otg);
        
        mxc_procfs_exit();

        /* Disable GPT
         */
        #if defined(CONFIG_OTG_GPTR)
      	mxc_gptcr_mod_exit();
        #endif /* defined(CONFIG_OTG_GPTR) */

        #if defined(CONFIG_OTG_HRT)
      	mxc_hrt_mod_exit();
        #endif /* defined(CONFIG_OTG_GPTR) */


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
        
        printk(KERN_INFO"%s: EEEE\n", __FUNCTION__); 


}

/*!
 * zasevb_modinit() - linux module initialization
 *
 * This needs to initialize the hcd, pcd and tcd drivers. This includes tcd and possibly hcd
 * for some architectures.
 *
 */
static int zasevb_modinit (void)
{
        struct otg_instance *otg;

        //printk(KERN_INFO"%s: AAAA\n", __FUNCTION__);


        //if (MODPARM(serial_number_str) && strlen(MODPARM(serial_number_str)))
        //        printk(KERN_INFO"%s: %s\n", __FUNCTION__, MODPARM(serial_number_str)) ;


        
        #if !defined(OTG_C99)
        pcd_global_init();
        fs_ocd_global_init();
        zasevb_tcd_global_init();
        fs_pcd_global_init();
        #endif /* !defined(OTG_C99) */

#ifdef OTG_USE_I2C
        i2c_mod_init();
#endif

        mxc_procfs_init();

        OCD = otg_trace_obtain_tag();
        TRACE_MSG0(OCD, "1. OCD");

        /* ZAS EVB Platform setup
         */
        TRACE_MSG4(OCD, "BCTRL Version: %04x Status: %04x 1: %04x 2: %04x",
                        readw(PBC_BASE_ADDRESS ),
                        readw(PBC_BASE_ADDRESS + PBC_BSTAT),
                        readw(PBC_BASE_ADDRESS + PBC_BCTRL1_SET),
                        readw(PBC_BASE_ADDRESS + PBC_BCTRL2_SET));

        /* ZAS EVB Clock setup
         */

#ifdef CONFIG_ARCH_MXC91331
#define ZASEVB_MULTIPLIER       12              
#define ZASEVB_DIVISOR          775             // ~10.
#else
#define ZASEVB_MULTIPLIER       12              
#define ZASEVB_DIVISOR 155
#endif

        TRACE_MSG0(OCD, "2. Setup GPT");
        #if defined(CONFIG_OTG_GPTR)
        mxc_gptcr_mod_init(ZASEVB_DIVISOR, ZASEVB_MULTIPLIER);
        #endif /* defined(CONFIG_OTG_GPTR) */

        #if defined(CONFIG_OTG_HRT)
        mxc_hrt_mod_init(ZASEVB_DIVISOR, ZASEVB_MULTIPLIER);
        #endif /* defined(CONFIG_OTG_GPTR) */

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

        TRACE_MSG0(PCD, "7. otg_init");
        if (MODPARM(serial_number_str) && strlen(MODPARM(serial_number_str))) {

                TRACE_MSG1(PCD, "serial_number_str: %s", MODPARM(serial_number_str));
                otg_serial_number (otg, MODPARM(serial_number_str));
        }
        otg_init(otg);

        return 0;

        CATCH(error) {
                //zasevb_modexit();
                return -EINVAL;
        }

        return 0;
}

#if 0
/* ************************************************************************************* */
static int __init_or_module
zasevb_remove(struct device *dev)
{
        return 0;
}


static int __init
zasevb_probe(struct device *dev)
{               
}
static int
zasevb_suspend(struct device *dev, u32 state, u32 phase)
{       
        return 0;
}               
        
static int
zasevb_resume(struct device *dev, u32 phase)
{               
        return 0;
}               

static struct device_driver zasevb_driver = {

        .name =         (char *) "ZASEVB-MXC-USBOTG,
        .bus =          &otg_bus_type,

        .probe =        zasevb_probe,
        .remove =       zasevb_remove,

        .suspend =      zasevb_suspend,
        .resume =       zasevb_resume,

};


static int __init zasevb_init(void)
{
        if (usb_disabled())
                return -ENODEV;

        INFO("driver %s, %s\n", hcd_name, DRIVER_VERSION);
        return driver_register(&zasevb_driver);
}

static void __exit zasevb_exit(void)
{
        driver_unregister(&zasevb_driver);
}
#endif

#ifdef CONFIG_OTG_NFS
late_initcall (zasevb_modinit);
#else
module_init (zasevb_modinit);
#endif
module_exit (zasevb_modexit);



