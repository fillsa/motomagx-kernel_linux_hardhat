/*
 * otg/ocd/brassboard/brassboard-mc13783-sierra.c -- Freescale mc13783 Connectivity driver
 * 
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2007 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 04/29/2005         Motorola         new file based on "zasevb-mc13783.c",
 *                                        use sierra MC13783 support for usbotg transciever
 *                                        NOTE: Using Sierra power_ic core API directly since
 *                                        Conn API is not ready, need to move when
 *                                        Sierra declares their MC13783 Conn support done.
 * 06/27/2005         Motorola         changes to adapt to Sierra's accy detection, stack will
 *                                        hard start based on a user space cable event,
 *                                        as polled by app /usr/local/bin/usbstart  
 * 01/05/2006         Motorola         Merge with Belcarra 2.6.2 stack
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 08/24/2007         Motorola         Changes for Open src compliance.
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
 * @file otg/ocd/brassboard/brassboard-mc13783-sierra.c
 * @brief BRASSBOARD MC13783 Connectivity Transceiver Driver 
 *
 * This is a wrapper for the MC13783 Connectivity driver to support it on the
 * Brassboard board. 
 *
 * Notes
 *
 * 1. Ensure that S2.1 is ON and S2.2 is OFF.
 *
 * @ingroup BRASSBOARD
 * * 2. Note that this driver has not been tested on the ZAS EVB.
 *
 * 3. The mc13783 Transceiver and USBOTG HWMODE must both be setup to match. Currently it
 * appears that only the SEO-SEO / DAT-Bidirectional combination works correctly. 
 *
 * @ingroup ZASEVB
 *
 */

#include <asm/delay.h>

#include <otg/pcd-include.h>
// #include <asm/arch/gpio.h>
#include <asm/mot-gpio.h>

#include <otghw/mxc-hardware.h>

#include <otghw/isp1301.h>
#include <otghw/isp1301-hardware.h>
#include <linux/power_ic_kernel.h>

#define GPIO_PORT 2
#define GPIO_PIN  12

// defines from FSL mc13783 reg bits
#define         BIT_DATSE0              17
#define         BIT_BIDIR               18

/* ********************************************************************************************* */
extern int  mxc91231_tcd_mod_init (void);
extern void  mxc91231_tcd_mod_exit (void);
struct tcd_instance * mxc91231_tcd_instance;
/* ********************************************************************************************* */
int mxc_mc13783_vbus (struct otg_instance *otg);
int mxc_mc13783_id (struct otg_instance *otg);
void mxc_mc13783_tcd_en(struct otg_instance *otg, u8 flag);
void mxc_mc13783_chrg_vbus(struct otg_instance *otg, u8 flag);
void mxc_mc13783_drv_vbus(struct otg_instance *otg, u8 flag);
void mxc_mc13783_dischrg_vbus(struct otg_instance *otg, u8 flag);
void mxc_mc13783_mx21_vbus_drain_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_dp_pullup_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_dm_pullup_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_dp_pulldown_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_dm_pulldown_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_peripheral_host_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_dm_det_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_dp_det_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_cr_det_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_bdis_acon_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_id_pulldown_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_audio_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_uart_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_mono_func(struct otg_instance *otg, u8 flag);
void mxc_mc13783_mod_init(void);
void mxc_mc13783_mod_exit(void);
void mxc_set_transceiver_mode(int);

int mxc_iomux_gpio_mc13783_set (int);
int mxc_iomux_gpio_mc13783_reset (void);


/* ********************************************************************************************* */
#if !defined(OTG_C99)
struct tcd_ops tcd_ops;
/*!
 *  mxc91231_tcd_global_init() - non c99 global initializer
 */
void mxc91231_tcd_global_init(void)
{
        ZERO(tcd_ops);

        tcd_ops.id = mxc_mc13783_id;
        tcd_ops.vbus = mxc_mc13783_vbus;
        //tcd_ops.tcd_init_func = mx2_tcd_init;
        
        tcd_ops.tcd_en_func = mxc_mc13783_tcd_en;;
        tcd_ops.chrg_vbus_func = mxc_mc13783_chrg_vbus;
        tcd_ops.drv_vbus_func = mxc_mc13783_drv_vbus;
        tcd_ops.dischrg_vbus_func = mxc_mc13783_dischrg_vbus;
        tcd_ops.dp_pullup_func = mxc_mc13783_dp_pullup_func;
        tcd_ops.dm_pullup_func = mxc_mc13783_dm_pullup_func;
        tcd_ops.dp_pulldown_func = mxc_mc13783_dp_pulldown_func;
        tcd_ops.dm_pulldown_func = mxc_mc13783_dm_pulldown_func;

        tcd_ops.overcurrent_func = NULL;
        tcd_ops.dm_det_func = mxc_mc13783_dm_det_func;
        tcd_ops.dp_det_func = mxc_mc13783_dp_det_func;
        tcd_ops.cr_det_func = mxc_mc13783_cr_det_func;
        //tcd_ops.charge_pump_func = mx2_charge_pump_func;
        //tcd_ops.bdis_acon_func = mxc_mc13783_bdis_acon_func;
        tcd_ops.peripheral_host_func = mxc_mc13783_peripheral_host_func;
        tcd_ops.mx21_vbus_drain_func = mxc_mc13783_mx21_vbus_drain_func;
        tcd_ops.id_pulldown_func = mxc_mc13783_id_pulldown_func;
        tcd_ops.audio_func = mxc_mc13783_audio_func;
        tcd_ops.uart_func = mxc_mc13783_uart_func;
        tcd_ops.mono_func = mxc_mc13783_mono_func;

        tcd_ops.mod_init = mxc91231_tcd_mod_init;
        tcd_ops.mod_exit = mxc91231_tcd_mod_exit;
}
#else /* !defined(OTG_C99) */
struct tcd_ops tcd_ops = {

        //.id = mxc_mc13783_id,
        //.vbus = mxc_mc13783_vbus,
        //.tcd_init_func = mx2_tcd_init,

        .tcd_en_func = mxc_mc13783_tcd_en,
        .chrg_vbus_func = mxc_mc13783_chrg_vbus,
        .drv_vbus_func = mxc_mc13783_drv_vbus,
        .dischrg_vbus_func = mxc_mc13783_dischrg_vbus,
        .dp_pullup_func = mxc_mc13783_dp_pullup_func,
        .dm_pullup_func = mxc_mc13783_dm_pullup_func,
        .dp_pulldown_func = mxc_mc13783_dp_pulldown_func,
        .dm_pulldown_func = mxc_mc13783_dm_pulldown_func,

        .overcurrent_func = NULL,
        .dm_det_func = mxc_mc13783_dm_det_func,
        .dp_det_func = mxc_mc13783_dp_det_func,
        .cr_det_func = mxc_mc13783_cr_det_func,
        //.charge_pump_func = mx2_charge_pump_func,
        //tcd_ops.bdis_acon_func = mxc_mc13783_bdis_acon_func,
        .peripheral_host_func = mxc_mc13783_peripheral_host_func,
        .mx21_vbus_drain_func = mxc_mc13783_mx21_vbus_drain_func,
        .id_pulldown_func = mxc_mc13783_id_pulldown_func,
        .audio_func = mxc_mc13783_audio_func,
        .uart_func = mxc_mc13783_uart_func,
        .mono_func = mxc_mc13783_mono_func,

        .mod_init = mxc91231_tcd_mod_init,
        .mod_exit = mxc91231_tcd_mod_exit,
};
#endif /* !defined(OTG_C99) */

/* ********************************************************************************************* */
void mxc91231_tcd_mod_exit (void);

/*!
 * mxc91231_tcd_mod_init() - initial tcd setup
 * This performs the platform specific hardware setup for the MX2ADS.
 */
int mxc91231_tcd_mod_init (void)
{
        int i2c = 1;
        int gpio = 1;
        bool res;
        unsigned int reg_value;
        int i;

#if 1
        #ifdef CONFIG_OTG_BRASSBOARD_DIFFERENTIAL_BIDIRECTIONAL
        int hwmode = XCVR_D_D;
        int newmode = XCVR_D_D;
        #elif CONFIG_OTG_BRASSBOARD_DIFFERENTIAL_UNIDIRECTIONAL
        int hwmode = XCVR_D_SE0_NEW;
    	int newmode = XCVR_D_D;
        #elif CONFIG_OTG_BRASSBOARD_SINGLE_ENDED_UNIDIRECTIONAL
        int hwmode = XCVR_SE0_D_NEW;
        int newmode = XCVR_SE0_D_NEW;
        #elif CONFIG_OTG_BRASSBOARD_SINGLE_ENDED_BIDIRECTIONAL
        int hwmode = XCVR_SE0_SE0;
        int newmode = XCVR_SE0_SE0;
        #else
        #error Please Configure Transceiver Mode
        #endif /* CONFIG_OTG_BRASSBOARD_.... */
#endif		
        		
        printk(KERN_INFO"%s: AAAA22\n",__FUNCTION__);

        TRACE_MSG0(TCD, "1. mc13783 Connectivity");

        mxc_mc13783_mod_init();

        TRACE_MSG0(TCD, "2. Transceiver setup");

        switch(hwmode) {
        case XCVR_D_D:
        case XCVR_SE0_D_NEW:
        case XCVR_D_SE0_NEW:
                break;

        case XCVR_SE0_SE0:
                // this works with XCVR_SE0_SE0 if AP_GPIO_AP_C16 not configured
                //isp1301_configure(dat_se0_bidirectional, spd_susp_reg);        // XCVR_SEO_SE0
                // XXX configure mc13783 transceiver here
                break;
        }

        //isp1301_configure(vp_vm_bidirectional, spd_susp_reg);        // XCVR_D_D

        TRACE_MSG0(TCD, "5. SET TCD OPS");
        THROW_UNLESS(mxc91231_tcd_instance = otg_set_tcd_ops(&tcd_ops), error);

	mxc_iomux_gpio_mc13783_set (hwmode);
	
#if 0
	
        /*
         *
         *  Default
         *      USB_TXEO_B      OE      (9 OE)          
         *      USB_DAT_VP      DAT_VP  (14 DAT/VP)
         *      USB_SE0_VM      SE0_VM  (13 SE0/VM)
         *
         *      USB_RXD         RCV     (12 RCV)
         *
         *      USB_VP          VP      (11 VP)
         *      USB_VM          VM      (10 VM)
         *
         *  AR_USB_VP           GP_AP_C16       MUX3 - USB_VP1          AP_GPIO_AP_C16
         *  AR_USB_VM           GP_AP_C17       MUX3 - USB_VM1          AP_GPIO_AP_C17
         */
        TRACE_MSG0(OCD, "6. Setup USBOTG IOMUX");

        #if defined(CONFIG_ARCH_MXC91231) 
        
        printk(KERN_INFO"IOMUX setting for MXC91231\n");
        iomux_config_mux(SP_USB_TXOE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_DAT_VP, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_SE0_VM, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_RXD,    OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	
	#endif

        #if defined(CONFIG_MACH_ARGONLVPHONE)
        printk(KERN_INFO"IOMUX setting for MXC91331 and MXC91321\n");
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);

	#endif /* CONFIG_MACH_ARGONLVPHONE */
								
#endif
	
        switch(hwmode) {
        case XCVR_D_SE0_NEW:
                TRACE_MSG0(TCD, "D_D - vp_vm_bidirectional");
                printk(KERN_INFO"%s: D_D - Differential Unidirectional\n", __FUNCTION__);
//                #if defined(CONFIG_ARCH_MXC91231) 
//                iomux_config_mux(AP_GPIO_AP_C16,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
//                iomux_config_mux(AP_GPIO_AP_C17,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
//                #endif
//                mc13783_convity_set_single_ended_mode(FALSE);
//                mc13783_convity_set_directional_mode(FALSE);
   	        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_DATSE0, FALSE);
	        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_BIDIR, FALSE);
                break;
        case XCVR_SE0_D_NEW:
                TRACE_MSG0(TCD, "SE0_D");
                printk(KERN_INFO"%s: SE0_D - Single Ended Unidirectional\n", __FUNCTION__);
//                mc13783_convity_set_single_ended_mode(TRUE);
//                mc13783_convity_set_directional_mode(FALSE);
	        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_DATSE0, TRUE);
	        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_BIDIR, FALSE);
                break;
        case XCVR_D_D:
                TRACE_MSG0(TCD, "D_SE0");
                printk(KERN_INFO"%s: D_SE0 - Differential Bidirectional\n", __FUNCTION__);
//                mc13783_convity_set_single_ended_mode(FALSE);
//                mc13783_convity_set_directional_mode(TRUE);
//                iomux_config_mux(AP_GPIO_AP_C16,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
//                iomux_config_mux(AP_GPIO_AP_C17,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
        	power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_DATSE0, FALSE);
	        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_BIDIR, TRUE);
                break;
        case XCVR_SE0_SE0:
	  	TRACE_MSG0(TCD, "SE0_SE0 - SEO_bidirectional");
		printk(KERN_INFO"%s: SE0_SE0 - Single Ended Bidirectional\n", __FUNCTION__);
		printk("OTG_DEBUG: set GPIO 16 and 17 to defaults, config BIT_DATSE0 and BIT_BIDIR for 3 wire\n");
//                mc13783_convity_set_single_ended_mode(TRUE);
//

		#if defined(CONFIG_ARCH_MXC91231) 
       		iomux_config_mux(AP_GPIO_AP_C16,OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
        	iomux_config_mux(AP_GPIO_AP_C17,OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
		#endif
	    	power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_DATSE0, TRUE);
	    	power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_BIDIR, TRUE);
            // debugging stuff
            // unsigned int reg_value;
            // power_ic_read_reg(POWER_IC_REG_ATLAS_USB_0, &reg_value);
            // printk("OTG_DEBUG: Atlas POWER_IC_REG_ATLAS_USB_0 register value is: %#8X\n", reg_value);

            break;
        }


        TRACE_MSG0(TCD, "7. SET HWMODE");
        mxc_set_transceiver_mode(newmode);

#if 0 
// Need to test if the following lines are needed
        mc13783_convity_set_var_disconnect (TRUE); // variable 1k5 and UDP/UDM pull-down are disconnected. (PULLOVER)
	mc13783__convity_set_usb_transceiver (TRUE); //USB transceiver is disabled (USBXCVREN) 
	mc13783__convity_set_udp_auto_connect (FALSE); //variable UDP is not automatically connected (SE0CONN)
	mc13783__convity_set_pull_down_switch (PD_UDP_150, FALSE); //150K UDP pull-up switch is out (DP150KPU)
	mc13783__convity_set_udp_pull(FALSE); //1.5K UDP pull-up and USB xcver is controlled by SPI bits.(USBCNTRL)
	mc13783__convity_set_output (TRUE, FALSE);	//disable vbus
	mc13783__convity_set_output (FALSE, FALSE);	//disable vusb
	mc13783__convity_set_output (FALSE, TRUE);        //enable vusb
#endif

#if 0 // Need to turn this on just find power_ic call to replace mc13783 with	
        
        for (i=48; i<51; i++){
                   mc13783_read_reg (PRIO_CONN, i, &reg_value);
                   printk (KERN_INFO"Register %d = %8X\n", i, reg_value);
        }
#endif

        /* Success! */
	


        TRACE_MSG0(TCD, "8. Success!");

        CATCH(error) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
        	UNLESS (i2c) i2c_close();
        	//SHP
                //UNLESS (gpio) gpio_free_irq (3, GPIO_PIN, GPIO_HIGH_PRIO);
                return -EINVAL;
        }
        TRACE_MSG0(TCD, "MX2_MOD_TCD_INIT FINISHED");
        return 0;
}

/*! 
 * mxc91231_tcd_mod_exit() - de-initialize
 * This is called from mx2-ocd.c
 */
void mxc91231_tcd_mod_exit (void)
{
        struct otg_instance *otg = tcd_instance->otg;
        TRACE_MSG0(TCD, "MX2_MOD_TCD_EXIT");

    	mxc_iomux_gpio_mc13783_reset ();

        mxc_mc13783_mod_exit();       
        mxc91231_tcd_instance = otg_set_tcd_ops(NULL);

}


