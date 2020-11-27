/*
 * otg/ocd/zasevb/zasevb-mxc_mc13783.c -- Freescale mc13783 Connectivity driver
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
 * @file otg/ocd/zasevb/zasevb-mc13783.c
 * @brief mc13783 USB Host Controller Driver
 *
 * This is a wrapper for the Freescale mc13783 driver to support it on the ZASEVB. 
 *
 * Notes
 *
 * 1. Ensure that S2.1 is ON and S2.2 is OFF.
 *
 * 2. Note that this driver has not been tested on the ZAS EVB.
 *
 * 3. The mc13783 Transceiver and USBOTG HWMODE must both be setup to match. Currently it
 * appears that only the SEO-SEO / DAT-Bidirectional combination works correctly. 
 *
 * @ingroup ZASEVB
 *
 */

#include <asm/delay.h>

#include <otg/pcd-include.h>
#ifdef CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY

#include <asm/arch/gpio.h>

#include <otghw/mxc-hardware.h>

#include <otghw/isp1301.h>
#include <otghw/isp1301-hardware.h>

/*
 * These files are currently located in 
 *      drivers/mxc/mc13783_legacy
 */
#include <core/mc13783_external.h>
#include <core/mc13783_event.h>
#include <module/mc13783_connectivity.h>


/* ********************************************************************************************* */
extern int mxc91231_tcd_mod_init (void);
extern void mxc91231_tcd_mod_exit (void);
struct tcd_instance *mxc91231_tcd_instance;
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
 * mxc91231_tcd_global_init() - non c99 global initializer
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

#if 0 

static irqreturn_t gpio_c16_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
static int counter;
u32 value;

        gpio_config_int_en(2, 16, FALSE);
	counter ++;
	value = gpio_get_data (2, 16);
        printk (KERN_INFO"Get the %d interrupt on C16 with value %d\n", counter, value);
	gpio_config_int_en(2, 16, TRUE);
        return IRQ_HANDLED;
}

static irqreturn_t gpio_c17_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
static int counter;
u32 value;

       gpio_config_int_en(2, 17, FALSE);
       counter ++;
       value = gpio_get_data (2, 17);
       printk (KERN_INFO"Get the %d interrupt C17 with value %d\n", counter, value);
       gpio_config_int_en(2, 17, TRUE);
       return IRQ_HANDLED;
}

#endif

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

#if 0	
        #ifdef CONFIG_OTG_ZASEVB_DIFFERENTIAL_UNIDIRECTIONAL
        int mode = XCVR_D_D;
        #elif CONFIG_OTG_ZASEVB_DIFFERENTIAL_BIDIRECTIONAL
        #elif CONFIG_OTG_ZASEVB_SINGLE_ENDED_UNIDIRECTIONAL
        #elif CONFIG_OTG_ZASEVB_SINGLE_ENDED_BIDIRECTIONAL
        int mode = XCVR_SE0_SE0;
        #endif /* CONFIG_OTG_ZASEVB_.... */
#endif

#if 1 
        #ifdef CONFIG_OTG_ZASEVB_DIFFERENTIAL_BIDIRECTIONAL
        int hwmode = XCVR_D_D;
        int newmode = XCVR_D_D;
        #elif CONFIG_OTG_ZASEVB_DIFFERENTIAL_UNIDIRECTIONAL
        int hwmode = XCVR_D_SE0_NEW;
	int newmode = XCVR_D_D;
        #elif CONFIG_OTG_ZASEVB_SINGLE_ENDED_UNIDIRECTIONAL
        int hwmode = XCVR_SE0_D_NEW;
        int newmode = XCVR_SE0_D_NEW;
        #elif CONFIG_OTG_ZASEVB_SINGLE_ENDED_BIDIRECTIONAL
        int hwmode = XCVR_SE0_SE0;
        int newmode = XCVR_SE0_SE0;
        #else
        #error Please Configure Transceiver Mode
        #endif /* CONFIG_OTG_ZASEVB_.... */
#endif																						
		
        		
        printk(KERN_INFO"%s: AAAA22\n",__FUNCTION__);

        TRACE_MSG0(TCD, "1. MC13783 Connectivity");

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

        #if defined(CONFIG_MACH_I30030EVB) || defined(CONFIG_ARCH_I30030EVB)

        printk(KERN_INFO"IOMUX setting for I30030EVB\n");
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);

	#endif /* CONFIG_ARCH_I30030EVB */
								
#endif
	
        switch(hwmode) {
        case XCVR_D_SE0_NEW:
                TRACE_MSG0(TCD, "D_D - vp_vm_bidirectional");
                printk(KERN_INFO"%s: D_D - Differential Unidirectional\n", __FUNCTION__);
//                #if defined(CONFIG_ARCH_MXC91231) 
//                iomux_config_mux(AP_GPIO_AP_C16,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
//                iomux_config_mux(AP_GPIO_AP_C17,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
//                #endif
                mc13783_convity_set_single_ended_mode(FALSE);
                mc13783_convity_set_directional_mode(FALSE);
                break;
        case XCVR_SE0_D_NEW:
                TRACE_MSG0(TCD, "SE0_D");
                printk(KERN_INFO"%s: SE0_D - Single Ended Unidirectional\n", __FUNCTION__);
                mc13783_convity_set_single_ended_mode(TRUE);
                mc13783_convity_set_directional_mode(FALSE);
                break;
        case XCVR_D_D:
                TRACE_MSG0(TCD, "D_SE0");
                printk(KERN_INFO"%s: D_SE0 - Differential Bidirectional\n", __FUNCTION__);
                mc13783_convity_set_single_ended_mode(FALSE);
                mc13783_convity_set_directional_mode(TRUE);
                break;

        case XCVR_SE0_SE0:
        	TRACE_MSG0(TCD, "SE0_SE0 - SEO_bidirectional");
                printk(KERN_INFO"%s: SE0_SE0 - Single Ended Bidirectional\n", __FUNCTION__);
                mc13783_convity_set_single_ended_mode(TRUE);
                mc13783_convity_set_directional_mode(TRUE);
        	break;
        }


        TRACE_MSG0(TCD, "7. SET HWMODE");
        mxc_set_transceiver_mode(newmode);
        mc13783_convity_set_var_disconnect (TRUE); // variable 1k5 and UDP/UDM pull-down are disconnected. (PULLOVER)
	mc13783_convity_set_usb_transceiver (TRUE); //USB transceiver is disabled (USBXCVREN) 
	mc13783_convity_set_udp_auto_connect (FALSE); //variable UDP is not automatically connected (SE0CONN)
	mc13783_convity_set_pull_down_switch (PD_UDP_150, FALSE); //150K UDP pull-up switch is out (DP150KPU)
	mc13783_convity_set_udp_pull(FALSE); //1.5K UDP pull-up and USB xcver is controlled by SPI bits.(USBCNTRL)
	mc13783_convity_set_output (TRUE, FALSE);	//disable vbus
	mc13783_convity_set_output (FALSE, FALSE);	//disable vusb
	mc13783_convity_set_output (FALSE, TRUE);        //enable vusb

#if 1	
        
        for (i=48; i<51; i++){
                   mc13783_read_reg (PRIO_CONN, i, &reg_value);
                   printk (KERN_INFO"Register %d = %8X\n", i, reg_value);
        }
#endif


#if 0	//test for interrupt on changing DP

	iomux_config_mux(AP_GPIO_AP_C16,OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
	iomux_config_mux(AP_GPIO_AP_C17,OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
				
        gpio_config(2, 16, false, GPIO_INT_RISE_EDGE);
        gpio = gpio_request_irq(2, 16, GPIO_HIGH_PRIO, gpio_c16_int_hndlr,
		                        SA_SHIRQ, "VP1", NULL);
        THROW_IF(gpio, error);
	gpio_config_int_en(2, 16, TRUE);  // XXX this might not be needed

	gpio_config(2, 17, false, GPIO_INT_RISE_EDGE);
        gpio = gpio_request_irq(2, 17, GPIO_HIGH_PRIO, gpio_c17_int_hndlr,
                                        SA_SHIRQ, "VP1", NULL);
        THROW_IF(gpio, error);
        gpio_config_int_en(2, 17, TRUE);  // XXX this might not be needed
				
	
	while (1){
		udelay(1000);
	}
#endif	
	
#if 0 
	while(1){
//		mc13783_convity_set_var_disconnect (FALSE);
//		mc13783_convity_set_udp_pull (FALSE);
//		mc13783_convity_set_udp_auto_connect (TRUE);
//		mc13783_convity_set_speed_mode (TRUE);
//		mc13783_convity_set_pull_down_switch(PD_PU, FALSE);
		
	}
#endif
#if 0	//beautiful pulse between zero and 3.3 on DP 
	     mc13783_convity_set_speed_mode (FALSE); //set high speed
	while(1){
             mc13783_convity_set_pull_down_switch(PD_PU, TRUE);	//variable 1.5K pull-up switch in
             mc13783_convity_set_pull_down_switch(PD_UPD_15, FALSE);  //DP pull down switch is off
             udelay(1000);
	     mc13783_convity_set_pull_down_switch(PD_PU, FALSE);   //variable 1.5K pull-up switch off
             mc13783_convity_set_pull_down_switch(PD_UPD_15, TRUE);   //DP pull down switch is on 
             udelay(1000);
	}
#endif

#if 0 //beautiful pulse between zero and 3.3 on DM
	mc13783_convity_set_speed_mode (TRUE); //set low speed
	while(1){
             mc13783_convity_set_pull_down_switch(PD_PU, TRUE);   //variable 1.5K pull-up switch in
             mc13783_convity_set_pull_down_switch(PD_UDM_15, FALSE);  //DP pull down switch is off
             udelay(1000);
             mc13783_convity_set_pull_down_switch(PD_PU, FALSE);   //variable 1.5K pull-up switch off
             mc13783_convity_set_pull_down_switch(PD_UDM_15, TRUE);   //DP pull down switch is on
             udelay(1000);
	}
#endif

#if 0	//checking VBUS 
	mc13783_convity_set_output (TRUE, TRUE);	//enable VBUS
	udelay(1000);
//	mc13783_convity_set_output (TRUE, FALSE);     //disable VBUS
	while(1){
		udelay(1000);
	}
#endif	

#if 0
	while (1){
		mc13783_convity_set_vbus (FALSE);	//pull-down NMOS switch is on
	}
#endif	
	
#if 0 
	mc13783_convity_set_vusb_voltage (TRUE);	//set the VUSB voltage to 3.3
	mc13783_convity_set_output (FALSE, TRUE);     //enable VUSB
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
 * mxc91231_tcs_mod_exit() - de-initialize
 * This is called from mx2-ocd.c
 */
void mxc91231_tcd_mod_exit (void)
{
        struct otg_instance *otg = tcd_instance->otg;
        TRACE_MSG0(TCD, "MX2_MOD_TCD_EXIT");

	mxc_iomux_gpio_mc13783_reset ();
#if 0 
        #if defined(CONFIG_ARCH_MXC91231)
        iomux_config_mux(SP_USB_TXOE_B, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        iomux_config_mux(SP_USB_DAT_VP, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        iomux_config_mux(SP_USB_SE0_VM, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        iomux_config_mux(SP_USB_RXD,    OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        #endif
        
        #if defined(CONFIG_MACH_I30030EVB) || defined(CONFIG_ARCH_I30030EVB)
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        #endif /* CONFIG_ARCH_I30030EVB */
#endif										
        
        mxc_mc13783_mod_exit();       
        mxc91231_tcd_instance = otg_set_tcd_ops(NULL);

}
#endif /* CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY */


