/*
 * otg/ocd/zasevb/zasevb-isp1301.c -- ZASEVB ISP1301 Transceiver Controller driver
 * @(#) balden@seth2.belcarratech.com|otg/ocd/zasevb/zasevb-isp1301.c|20051116205001|06485
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
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
 * @file otg/ocd/zasevb/zasevb-isp1301.c
 * @brief ZAS EVB USB ISP1301 Transceiver Driver
 *
 * This is a wrapper for the ISP1301 driver to support it on the ZASEVB. 
 *
 * Notes
 *
 * 1. Ensure that S2.1 and S2.2 are both in the ON position.
 *
 * 2. The ISP1301 uses the system I2C bus, this is done throught the i2c-l26.c library.
 * This library must be opened with the correct adapter name.
 *
 * 3. The ISP1301 uses a GPIO for interrupts. The GPIO must be properly setup using
 * iomux().
 *
 * 4. The ISP1301 and USBOTG HWMODE must both be setup to match. Currently it
 * appears that only the SEO-SEO / DAT-Bidirectional combination works correctly. 
 *
 * @ingroup ZASEVB
 *
 */


#include <otg/pcd-include.h>

#if defined(CONFIG_OTG_ZASEVB_ISP1301)

#include <asm/arch/gpio.h>

#include <otghw/mxc-hardware.h>
#include <asm/arch/board.h>

#include <otghw/isp1301.h>
#include <otghw/isp1301-hardware.h>


#ifndef PLL2_BASE_ADDR
#define PLL2_BASE_ADDR                          0x5004C000
#endif
#define PLL2_DP_CTL                             IO_ADDRESS(PLL2_BASE_ADDR + 0x00)
#define PLL2_DP_CONFIG                          IO_ADDRESS(PLL2_BASE_ADDR + 0x04)
#define PLL2_DP_OP                              IO_ADDRESS(PLL2_BASE_ADDR + 0x08)
#define PLL2_DP_MFD                             IO_ADDRESS(PLL2_BASE_ADDR + 0x0c)
#define PLL2_DP_MFN                             IO_ADDRESS(PLL2_BASE_ADDR + 0x10)
#define PLL2_DP_HFSOP                           IO_ADDRESS(PLL2_BASE_ADDR + 0x1c)
#define PLL2_DP_HFSMFD                          IO_ADDRESS(PLL2_BASE_ADDR + 0x20)
#define PLL2_DP_HFSMFN                          IO_ADDRESS(PLL2_BASE_ADDR + 0x24)



#if defined(CONFIG_MACH_I30030EVB) || defined(CONFIG_MACH_MXC30030EVB)
#define ZGPIO_PORT 0
#define ZGPIO_PIN  2
#endif/* defined(CONFIG_MACH_I30030EVB)*/

#if defined(CONFIG_MACH_MXC27530EVB)
#define ZGPIO_PORT 2
#define ZGPIO_PIN  12
#endif /* CONFIG_MACH_MXC27530EVB */

#ifdef CONFIG_MACH_MXC91131EVB
#define ZGPIO_PORT 1
#define ZGPIO_PIN 30
#endif /* CONFIG_ARCH_MXC91131 */


void mxc_set_transceiver_mode(int mode);
void mxc_main_clock_on(void);
void mxc_disable_interrupts (void);
void isp1301_exit(void);
int mxc_iomux_gpio_isp1301_set (int);
int mxc_iomux_gpio_isp1301_reset (void);



#if 0	//removing gpios

/* ********************************************************************************************* */
/*!
 * zasevb_gpio_int_hndlr() - gpio interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 * @return interrupt handler status
 * This disables the gpio interrup and schedules the isp1301 bottom half handler.
 *
 */             
static irqreturn_t zasevb_gpio_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{        
	gpio_config_int_en(ZGPIO_PORT, ZGPIO_PIN, FALSE);

        TRACE_MSG0(TCD, "ZASEVB GPIO INTERRUPT: SCHEDULE WORK");
        isp1301_bh_wakeup(FALSE);

        return IRQ_HANDLED;
}

/*!
 * zasevb_isp1301_bh()- call isp1301 bottom half handler
 * @param arg
 * This is a wrapper to the isp1301 bottom half handler, it
 * re-enables the gpio interrupt after processing complete.
 */
void zasevb_isp1301_bh(void *arg)
{
        TRACE_MSG0(TCD, "ZASEVB GPIO INTERRUPT: ISP1301_BH");
        isp1301_bh(arg);
        TRACE_MSG0(TCD, "ZASEVB GPIO INTERRUPT: REENABLE");
	gpio_config_int_en(ZGPIO_PORT, ZGPIO_PIN, TRUE);
}

#endif

/* ********************************************************************************************* */
extern int zasevb_tcd_mod_init (void);
extern void zasevb_tcd_mod_exit (void);
struct tcd_instance *zasevb_tcd_instance;
#if !defined(OTG_C99)
struct tcd_ops tcd_ops;
/*!
 * zasevb_tcd_global_init() - non c99 global initializer
 */
void zasevb_tcd_global_init(void)
{
	ZERO(tcd_ops);


        tcd_ops.vbus = isp1301_vbus;
        tcd_ops.id = isp1301_id;

        tcd_ops.tcd_en_func = isp1301_tcd_en;
        tcd_ops.chrg_vbus_func = isp1301_chrg_vbus;
        tcd_ops.drv_vbus_func = isp1301_drv_vbus;
        tcd_ops.dischrg_vbus_func = isp1301_dischrg_vbus;
        tcd_ops.dp_pullup_func = isp1301_dp_pullup_func;
        tcd_ops.dm_pullup_func = isp1301_dm_pullup_func;
        tcd_ops.dp_pulldown_func = isp1301_dp_pulldown_func;
        tcd_ops.dm_pulldown_func = isp1301_dm_pulldown_func;

        tcd_ops.overcurrent_func = NULL;
        tcd_ops.dm_det_func = isp1301_dm_det_func;
        tcd_ops.dp_det_func = isp1301_dp_det_func;
        tcd_ops.cr_det_func = isp1301_cr_det_func;
        tcd_ops.peripheral_host_func = isp1301_peripheral_host_func;
        //tcd_ops.mx21_vbus_drain_func = isp1301_mx21_vbus_drain_func;
        tcd_ops.id_pulldown_func = isp1301_id_pulldown_func;
        tcd_ops.audio_func = isp1301_audio_func;
        tcd_ops.uart_func = isp1301_uart_func;
        tcd_ops.mono_func = isp1301_mono_func;

        tcd_ops.mod_init = zasevb_tcd_mod_init;
        tcd_ops.mod_exit = zasevb_tcd_mod_exit;
}
#else /* !defined(OTG_C99) */
struct tcd_ops tcd_ops = {

        //.vbus = isp1301_vbus,
        //.id = isp1301_id,

        .tcd_en_func = isp1301_tcd_en,
        .chrg_vbus_func = isp1301_chrg_vbus,
        .drv_vbus_func = isp1301_drv_vbus,
        .dischrg_vbus_func = isp1301_dischrg_vbus,
        .dp_pullup_func = isp1301_dp_pullup_func,
        .dm_pullup_func = isp1301_dm_pullup_func,
        .dp_pulldown_func = isp1301_dp_pulldown_func,
        .dm_pulldown_func = isp1301_dm_pulldown_func,

        .overcurrent_func = NULL,
        .dm_det_func = isp1301_dm_det_func,
        .dp_det_func = isp1301_dp_det_func,
        .cr_det_func = isp1301_cr_det_func,
        .peripheral_host_func = isp1301_peripheral_host_func,
        //.mx21_vbus_drain_func = isp1301_mx21_vbus_drain_func,
        .id_pulldown_func = isp1301_id_pulldown_func,
        .audio_func = isp1301_audio_func,
        .uart_func = isp1301_uart_func,
        .mono_func = isp1301_mono_func,

        .mod_init = zasevb_tcd_mod_init,
        .mod_exit = zasevb_tcd_mod_exit,
};
#endif /* !defined(OTG_C99) */

/* ********************************************************************************************* */
void zasevb_tcd_mod_exit (void);

#define ADAPTER_NAME "MXC I2C Adapter"

/*!
 * zasevb_tcd_mod_init() - initial tcd setup
 * This performs the platform specific hardware setup for the MX2ADS.
 */
int zasevb_tcd_mod_init (void)
{
	int i2c = 1;
        int gpio = 1;

        /* ------------------------------------------------------------------------ */
	#ifdef CONFIG_OTG_ZASEVB_DIFFERENTIAL_UNIDIRECTIONAL
        int hwmode = XCVR_D_SE0_NEW;   
	int newmode = XCVR_D_D;		
	isp1301_tx_mode_t tx_mode = vp_vm_unidirectional;	// MXC91231 ok
	printk (KERN_INFO"Current setting is DIFFERENTIAL UNIDIRECTIONAL\n");

        /* ------------------------------------------------------------------------ */
	#elif CONFIG_OTG_ZASEVB_SINGLE_ENDED_UNIDIRECTIONAL
        int hwmode = XCVR_SE0_D_NEW;   
	int newmode = XCVR_SE0_D_NEW;
        isp1301_tx_mode_t tx_mode = dat_se0_unidirectional;     //  MXC91331 ok
	printk (KERN_INFO"Current setting is SINGLE ENDED UNIDIRECTIONAL\n");

        /* ------------------------------------------------------------------------ */
	#elif CONFIG_OTG_ZASEVB_DIFFERENTIAL_BIDIRECTIONAL
	int hwmode = XCVR_D_D;
        int newmode = XCVR_D_D;
	isp1301_tx_mode_t tx_mode = vp_vm_bidirectional;       // MXC91331 ok
        printk (KERN_INFO"Current setting is DIFFERENTIAL BIDIRECTIONAL\n");

        /* ------------------------------------------------------------------------ */
	#elif CONFIG_OTG_ZASEVB_SINGLE_ENDED_BIDIRECTIONAL
        int hwmode = XCVR_SE0_SE0;   
	int newmode = XCVR_SE0_SE0;
        isp1301_tx_mode_t tx_mode = dat_se0_bidirectional;	//MXC91231 ok
	printk (KERN_INFO"Current setting is SINGLE ENDED BIDIRECTIONAL\n");

        /* ------------------------------------------------------------------------ */
        #else
        #error Please Configure Transceiver Mode
	#endif /* CONFIG_OTG_ZASEVB_.... */
        /* ------------------------------------------------------------------------ */
	

        TRACE_MSG0(TCD, "1. I2C setup");

	THROW_IF ((i2c = i2c_configure(ADAPTER_NAME, ISP1301_I2C_ADDR_HIGH)), error);

        TRACE_MSG0(TCD, "2. ISP1301 module setup");
//        isp1301_mod_init(&zasevb_isp1301_bh);

        TRACE_MSG0(TCD, "3. SET TCD OPS");
        THROW_UNLESS(zasevb_tcd_instance = otg_set_tcd_ops(&tcd_ops), error);

        TRACE_MSG0(TCD, "4. ISP1301 device setup");

	mxc_iomux_gpio_isp1301_set (hwmode);
	 

        #ifdef CONFIG_ARCH_MXC91131
        writel (0x00000051, PLL2_DP_HFSOP);
        writel (0x00000051, PLL2_DP_OP);
        #endif /* CONFIG_ARCH_MXC91131 */
			
        /* ------------------------------------------------------------------------ */
        TRACE_MSG0(TCD, "7. SET HWMODE");
        isp1301_configure(tx_mode, spd_susp_reg);        
        mxc_main_clock_on();
        //mxc_host_clock_on();
        //mxc_func_clock_on();
	mxc_set_transceiver_mode(newmode);

		
	
        /* Success!
         */
        TRACE_MSG0(TCD, "8. Success!");

        CATCH(error) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
                UNLESS (i2c) i2c_close();
//                UNLESS (gpio) gpio_free_irq (ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO);
                return -EINVAL;
        }
        TRACE_MSG0(TCD, "MX2_MOD_TCD_INIT FINISHED");
        return 0;
}

/*! 
 * zasevb_tcd_mod_exit() - de-initialize
 * This is called from mx2-ocd.c
 */
void zasevb_tcd_mod_exit (void)
{
        struct otg_instance *otg = tcd_instance->otg;
        TRACE_MSG0(TCD, "MX2_MOD_TCD_EXIT");
	isp1301_mod_exit();
	mxc_disable_interrupts();
//	gpio_free_irq (ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO);
	mxc_iomux_gpio_isp1301_reset();
        zasevb_tcd_instance = otg_set_tcd_ops(NULL);
        isp1301_exit();
}
#endif /* CONFIG_ZASEVB_ISP1301 */


