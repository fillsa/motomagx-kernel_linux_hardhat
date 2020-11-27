/*
 * otg/ocd/zasevb/zasevb-pmic-rr.c -- Freescale PMIC Connectivity driver
 * @(#) balden@seth2.belcarratech.com|otg/ocd/zasevb/zasevb-roadrunner.c|20051116205001|10571
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
 *
 */

/*!
 * @file otg/ocd/zasevb/zasevb-roadrunner.c
 * @brief pmic USB Host Controller Driver
 *
 * This is a wrapper for the Freescale PMIC driver to support it on the ZASEVB. 
 *
 * @ingroup ZASEVB
 *
 */


#include <asm/arch/pmic_convity.h> /* For PMIC Connectivity driver interface. */
#include <asm/arch/pmic_external.h>

#include <asm/delay.h>
#include <otg/pcd-include.h>

#ifdef CONFIG_OTG_ZASEVB_PMIC

#include <asm/arch/gpio.h>

#include <otghw/mxc-hardware.h>

/*
 * These files are currently located in 
 *      drivers/mxc/roadrunner
 */


/* ********************************************************************************************* */
extern int scma11_tcd_mod_init (void);
extern void scma11_tcd_mod_exit (void);
struct tcd_instance *scma11_tcd_instance;
/* ********************************************************************************************* */
int mxc_pmic_vbus (struct otg_instance *otg);
int mxc_pmic_id (struct otg_instance *otg);
void mxc_pmic_tcd_en(struct otg_instance *otg, u8 flag);
void mxc_pmic_chrg_vbus(struct otg_instance *otg, u8 flag);
void mxc_pmic_drv_vbus(struct otg_instance *otg, u8 flag);
void mxc_pmic_dischrg_vbus(struct otg_instance *otg, u8 flag);
void mxc_pmic_mx21_vbus_drain_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dp_pullup_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dm_pullup_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dp_pulldown_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dm_pulldown_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_peripheral_host_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dm_det_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_dp_det_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_cr_det_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_bdis_acon_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_id_pulldown_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_audio_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_uart_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_mono_func(struct otg_instance *otg, u8 flag);
void mxc_pmic_mod_init(void);
void mxc_pmic_mod_exit(void);
void mxc_set_transceiver_mode(int);
/* ********************************************************************************************* */
#if !defined(OTG_C99)
struct tcd_ops tcd_ops;
/*!
 * scma11_tcd_global_init() - non c99 global initializer
 */
void scma11_tcd_global_init(void)
{
        ZERO(tcd_ops);

        tcd_ops.id = mxc_pmic_id;
        tcd_ops.vbus = mxc_pmic_vbus;
        //tcd_ops.tcd_init_func = mx2_tcd_init;
        
        tcd_ops.tcd_en_func = mxc_pmic_tcd_en;;
        tcd_ops.chrg_vbus_func = mxc_pmic_chrg_vbus;
        tcd_ops.drv_vbus_func = mxc_pmic_drv_vbus;
        tcd_ops.dischrg_vbus_func = mxc_pmic_dischrg_vbus;
        tcd_ops.dp_pullup_func = mxc_pmic_dp_pullup_func;
        tcd_ops.dm_pullup_func = mxc_pmic_dm_pullup_func;
        tcd_ops.dp_pulldown_func = mxc_pmic_dp_pulldown_func;
        tcd_ops.dm_pulldown_func = mxc_pmic_dm_pulldown_func;

        tcd_ops.overcurrent_func = NULL;
        tcd_ops.dm_det_func = mxc_pmic_dm_det_func;
        tcd_ops.dp_det_func = mxc_pmic_dp_det_func;
        tcd_ops.cr_det_func = mxc_pmic_cr_det_func;
        //tcd_ops.charge_pump_func = mx2_charge_pump_func;
        //tcd_ops.bdis_acon_func = mxc_pmic_bdis_acon_func;
        tcd_ops.peripheral_host_func = mxc_pmic_peripheral_host_func;
        tcd_ops.mx21_vbus_drain_func = mxc_pmic_mx21_vbus_drain_func;
        tcd_ops.id_pulldown_func = mxc_pmic_id_pulldown_func;
        tcd_ops.audio_func = mxc_pmic_audio_func;
        tcd_ops.uart_func = mxc_pmic_uart_func;
        tcd_ops.mono_func = mxc_pmic_mono_func;

        tcd_ops.mod_init = scma11_tcd_mod_init;
        tcd_ops.mod_exit = scma11_tcd_mod_exit;
}
#else /* !defined(OTG_C99) */
struct tcd_ops tcd_ops = {

        .id = mxc_pmic_id,
        .vbus = mxc_pmic_vbus,
        //.tcd_init_func = mx2_tcd_init,

        .tcd_en_func = mxc_pmic_tcd_en,
        .chrg_vbus_func = mxc_pmic_chrg_vbus,
        .drv_vbus_func = mxc_pmic_drv_vbus,
        .dischrg_vbus_func = mxc_pmic_dischrg_vbus,
        .dp_pullup_func = mxc_pmic_dp_pullup_func,
        .dm_pullup_func = mxc_pmic_dm_pullup_func,
        .dp_pulldown_func = mxc_pmic_dp_pulldown_func,
        .dm_pulldown_func = mxc_pmic_dm_pulldown_func,

        .overcurrent_func = NULL,
        .dm_det_func = mxc_pmic_dm_det_func,
        .dp_det_func = mxc_pmic_dp_det_func,
        .cr_det_func = mxc_pmic_cr_det_func,
        //.charge_pump_func = mx2_charge_pump_func,
        //tcd_ops.bdis_acon_func = mxc_pmic_bdis_acon_func,
        .peripheral_host_func = mxc_pmic_peripheral_host_func,
        .mx21_vbus_drain_func = mxc_pmic_mx21_vbus_drain_func,
        .id_pulldown_func = mxc_pmic_id_pulldown_func,
        .audio_func = mxc_pmic_audio_func,
        .uart_func = mxc_pmic_uart_func,
        .mono_func = mxc_pmic_mono_func,

        .mod_init = scma11_tcd_mod_init,
        .mod_exit = scma11_tcd_mod_exit,
};
#endif /* !defined(OTG_C99) */

/* ********************************************************************************************* */
void scma11_tcd_mod_exit (void);

/*!
 * scma11_tcd_mod_init() - initial tcd setup
 * This performs the platform specific hardware setup for the MX2ADS.
 */
int scma11_tcd_mod_init (void)
{

        int i2c = 1;
        int gpio = 1;
        bool res;
        unsigned int reg_value;
        int i;

#if 0	
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

        TRACE_MSG0(TCD, "1. pmic Connectivity");

        mxc_pmic_mod_init();

	//using single ended bidirectional
        int hwmode = XCVR_SE0_D_NEW;
        int newmode = XCVR_SE0_D_NEW;


        TRACE_MSG0(TCD, "3. SET TCD OPS");
        THROW_UNLESS(scma11_tcd_instance = otg_set_tcd_ops(&tcd_ops), error);
	
        TRACE_MSG0(OCD, "4. Setup USBOTG IOMUX");	
        
	iomux_config_mux(USB_TXOE_B_PIN,  MUX0_OUT, MUX0_IN);
        iomux_config_mux(USB_DAT_VP_PIN,  MUX0_OUT, MUX0_IN);
        iomux_config_mux(USB_SE0_VM_PIN,  MUX0_OUT, MUX0_IN);
        iomux_config_mux(USB_RXD_PIN,     MUX0_OUT, MUX0_IN);
//        iomux_config_mux(U3CE_DSR_B_PIN,  MUX2_OUT, MUX2_IN);   // USB_VP1
//        iomux_config_mux(U3CE_DTR_B_PIN,  MUX2_OUT, MUX2_IN);   // USB_VM1
							
	
        TRACE_MSG0(TCD, "5. SET HWMODE");
        mxc_set_transceiver_mode(newmode);
		

#if 0	//pulse on DP pull-up between zero and 3.3
	while(1){
		unsigned int reg_write = 0x000000;
	        unsigned int reg_mask  = 0x000004;
		
		reg_write = 0x000000;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
		
		udelay (1000);
		reg_write = 0x000004;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
		
		udelay(1000);
	
	}
#endif

#if 0	//puls on DP	
	while(1){
		unsigned int reg_write = 0x000000;
	        unsigned int reg_mask  = 0x00000C;
		
		reg_write = 0x000004;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
		
		udelay (1000);
		reg_write = 0x00000C;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
		
		udelay(1000);
	
	}
#endif


        TRACE_MSG0(TCD, "8. Success!");

        CATCH(error) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
                UNLESS (i2c) i2c_close();
                //SHP
                //UNLESS (gpio) gpio_free_irq (3, GPIO_PIN, GPIO_HIGH_PRIO);
                return -EINVAL;
	}                    

	
#if 0

        TRACE_MSG0(TCD, "7. SET HWMODE");
        mxc_set_transceiver_mode(newmode);
        pmic_convity_set_output (FALSE, TRUE);
        pmic_convity_set_var_disconnect (TRUE);
//	pmic_convity_set_pull_down_switch(PD_UDP_150, FALSE);
	pmic_convity_set_usb_transceiver (FALSE);


        TRACE_MSG0(TCD, "8. Success!");

        CATCH(error) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
        	UNLESS (i2c) i2c_close();
        	//SHP
                //UNLESS (gpio) gpio_free_irq (3, GPIO_PIN, GPIO_HIGH_PRIO);
                return -EINVAL;
        }

#endif	
        TRACE_MSG0(TCD, "MX2_MOD_TCD_INIT FINISHED");
        return 0;
}

/*! 
 * scma1scma11cd_mod_exit() - de-initialize
 * This is called from mx2-ocd.c
 */
void scma11_tcd_mod_exit (void)
{
        struct otg_instance *otg = tcd_instance->otg;
        TRACE_MSG0(TCD, "MX2_MOD_TCD_EXIT");

//        mxc_pmic_mod_exit();       
        scma11_tcd_instance = otg_set_tcd_ops(NULL);

}
#endif /* CONFIG_OTG_ZASEVB_PMIC */


