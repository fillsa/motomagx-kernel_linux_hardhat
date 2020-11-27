/*
 * otg/ocd/mxc/mc13783.c -- Freescale MC13783 Connectivity Transceiver Controller driver
 * @(#) balden@seth2.belcarratech.com|otg/ocd/mxc/mc13783.c|20051116203403|43713
 *
 *      Copyright (c) 2005 Belcarra
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
 * @file otg/ocd/mxc/mxc-mc13783.c
 * @brief MC13783 Transciever Controller Driver
 *
 * This is a simple transceiver driver for the MC13783 transceiver
 * using the Freescale MC13783 connectivity driver.
 *
 * @ingroup FSOTG
 *
 * 
 */

#include <asm/delay.h>

#include <otg/pcd-include.h>
#include <asm/arch/gpio.h>

#include <core/mc13783_external.h>
#include <core/mc13783_event.h>
#include <module/mc13783_connectivity.h>


WORK_ITEM mc13783_work_bh;
WORK_ITEM mc13783_otg_wq;


int global_flag = 0;
int global_flag_array[20], start_flag, end_flag;
int det_dm_hi, det_dp_hi;

#define PUDP_FLAG_SET 1
#define PUDP_FLAG_RESET 2
#define UPD_FLAG_SET 4
#define UPD_FLAG_RESET 8
#define UDM_FLAG_SET 16
#define UDM_FLAG_RESET 32
#define DRV_VBUS_SET 64
#define DRV_VBUS_RESET 128
#define PUDM_FLAG_SET 256
#define PUDM_FLAG_RESET 512
#define DISCHRG_VBUS_SET 1024
#define DISCHRG_VBUS_RESET 2048
#define CHRG_VBUS_SET 4096
#define CHRG_VBUS_RESET 8192


//#define VBUS_TIMER

/*! mc13783_bh
 *  */
void mc13783_bh(void *arg)
{
	TRACE_MSG0(TCD, "--");
	global_flag = global_flag_array[start_flag];
	if (global_flag & PUDP_FLAG_SET){	//set DP pullup
		mc13783_convity_set_speed_mode (FALSE); //set high speed
		mc13783_convity_set_pull_down_switch(PD_PU, TRUE);  //variable 1.5K pull-up switch in
	}
	if (global_flag & PUDP_FLAG_RESET){	//reset DP pullup
		mc13783_convity_set_speed_mode (FALSE); //set high speed
		mc13783_convity_set_pull_down_switch(PD_PU, FALSE);  //variable 1.5K pull-up switch off
	}
        if (global_flag & PUDM_FLAG_SET){	//set DM pullup
                mc13783_convity_set_speed_mode (TRUE); //set low speed
		mc13783_convity_set_pull_down_switch(PD_PU, TRUE);   //variable 1.5K pull-up switch in
        }		
        if (global_flag & PUDM_FLAG_RESET){	//reset DM pullup
                mc13783_convity_set_speed_mode (TRUE); //set low speed
                mc13783_convity_set_pull_down_switch(PD_PU, FALSE);   //variable 1.5K pull-up switch off
        }
	if (global_flag & UPD_FLAG_SET){	//set DP pulldown
		mc13783_convity_set_pull_down_switch(PD_UPD_15, TRUE);   //DP pull down switch is on
	}
	if (global_flag & UPD_FLAG_RESET){	//reset DP pulldown
		mc13783_convity_set_pull_down_switch(PD_UPD_15, FALSE);  //DP pull down switch is off
	}
	if (global_flag & UDM_FLAG_SET){	//set DM pulldown
		mc13783_convity_set_pull_down_switch(PD_UDM_15, TRUE);   //DP pull down switch is on
	}
	if (global_flag & UDM_FLAG_RESET){	//reset DM pulldown
		mc13783_convity_set_pull_down_switch(PD_UDM_15, FALSE);  //DP pull down switch is off
	}
	if (global_flag & DRV_VBUS_SET){	//enable vbus voltage
//		mc13783_convity_set_output (FALSE, FALSE);     //disable VUSB
		mc13783_convity_set_output (TRUE, TRUE);     //enable VBUS
	}
	if (global_flag & DRV_VBUS_RESET){	//disable vbus voltage
//		mc13783_convity_set_output (FALSE, FALSE);     //disable VUSB
		mc13783_convity_set_output (TRUE, FALSE);     //disable VBUS
	}
	if (global_flag & CHRG_VBUS_SET){	//enable vbus
#ifndef VBUS_TIMER		
		mc13783_convity_set_output (TRUE, TRUE);     //enable VBUS
#else		
		mc13783_convity_set_vbus_reg (3);         //set VBUS on for 30 ms
#endif		
	}
	if (global_flag & CHRG_VBUS_RESET){	//disable vbus
		mc13783_convity_set_output (TRUE, FALSE);     //disable VBUS
	}
	if (global_flag & DISCHRG_VBUS_SET){	//discharge vbus
		mc13783_convity_set_output (TRUE, FALSE);     //disable VBUS
	        mc13783_convity_set_vusb_voltage (TRUE);  //set the VUSB voltage to 3.3
	        mc13783_convity_set_output (FALSE, TRUE);     //enable VUSB
	}
	if (global_flag & DISCHRG_VBUS_RESET){    //discharge vbus disable
		mc13783_convity_set_output (TRUE, FALSE);     //disable VBUS
		mc13783_convity_set_output (FALSE, TRUE);     //enable VUSB
//		mc13783_convity_set_output (FALSE, FALSE);     //disable VUSB
	}	
#if 1
	TRACE_MSG3 (TCD, "gloabl flag %d start_flag %d end_flag %d", global_flag, start_flag, end_flag);
#endif	
#if 0	
	printk (KERN_INFO"%d %d %d %d %d %d %d %d\n", (global_flag & PU_FLAG_SET), (global_flag & PU_FLAG_RESET), \
			(global_flag & UPD_FLAG_SET), (global_flag & UPD_FLAG_RESET), \
			(global_flag & UDM_FLAG_SET), (global_flag & UDM_FLAG_RESET), \
			(global_flag & VBUSPDENB_RESET), (global_flag & VBUSREGEN_RESET));
#endif	
	global_flag = 0;
	global_flag_array[start_flag] = 0;
        if (start_flag ++ > 15)
                start_flag = 0;
	if (start_flag != end_flag)
		SCHEDULE_WORK(mc13783_work_bh);		
	
}



/*! mc13783_bh_wakeup - wakeup the mc13783 bottom half
 *  */
void mc13783_bh_wakeup(void)
{
        TRACE_MSG0(TCD, "--");
        SCHEDULE_WORK(mc13783_work_bh);
}


/* ********************************************************************************************** */
/*! mxc_mc13783_vbus - Do we have Vbus (cable attached?)
 * Return non-zero if Vbus is detected.
 *
 */
int mxc_mc13783_vbus (struct otg_instance *otg)
{
        #if 1
        t_sense_bits sense_bits;
        if (mc13783_get_sense(&sense_bits)) {
                printk(KERN_INFO"%s: mc13783_get_sense() failed\n", __FUNCTION__);
                return 0;
        }
        return sense_bits.sense_usb2v0s;
        #else
        return mc13783_check_sense(sense_usb2v0s);
        #endif
}

void mc13783_otg_event_bh_old (void *arg)
{
        otg_event_set_irq(tcd_instance->otg, 1, mxc_mc13783_vbus(tcd_instance->otg), B_SESS_VLD, TCD, "B_SESS_VLD");
}

void mc13783_otg_event_bh (void *arg)
{
        u64 inputs;
        t_sense_bits sense_bits;
        static BOOL force = TRUE;
        static u64 inputs_saved = 0;

        if (mc13783_get_sense(&sense_bits)) {
                printk(KERN_INFO"%s: mc13783_get_sense() failed\n", __FUNCTION__);
		return;
        }
	inputs = (sense_bits.sense_usb4v4s ? VBUS_VLD : VBUS_VLD_) |
		 (sense_bits.sense_usb2v0s ? (B_SESS_VLD | A_SESS_VLD) : (B_SESS_VLD_ | A_SESS_VLD_)) |
		 (sense_bits.sense_usb0v8s ? B_SESS_END_ : B_SESS_END) |
		 (sense_bits.sense_id_gnds ? ID_GND : ID_GND_) |
                 (sense_bits.sense_id_floats ? ID_FLOAT : ID_FLOAT_) |
		 (sense_bits.sense_se1s ? SE1_DET : SE1_DET_) |
		 (det_dp_hi ? DP_HIGH : DP_HIGH_) |
		 (det_dm_hi ? DM_HIGH : DM_HIGH_);
		 
//	printk(KERN_INFO" inputs: %8X\n", inputs);
	TRACE_MSG4(TCD, "MC13783 EVENT: sense_bits: %8x otg inputs: %8x saved: %x diff: %x", 
                        sense_bits.sense_se1s, inputs, inputs_saved, inputs ^ inputs_saved);

        RETURN_UNLESS(force || (inputs ^ inputs_saved));

        inputs_saved = inputs;
        otg_event(tcd_instance->otg, inputs, TCD, "MC13783 OTG EVENT");


//	gpio_config_int_en(2, 17, TRUE);
//	gpio_config_int_en(2, 16, TRUE);

//	gpio_clear_int (2, 17);
//	gpio_clear_int (2, 16);

}


void mc13783_otg_wakeup(void)
{
        TRACE_MSG0(TCD, "start");
        SCHEDULE_WORK(mc13783_otg_wq);
        TRACE_MSG0(TCD, "finsih");
}

/**
 * The following functions are commented off because they are not used
 * and are causing KW errors. This might be needed for EVB bring up.
 **/
#if 0 
static irqreturn_t gpio_c17_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
       gpio_config_int_en(2, 17, FALSE);
       udelay (100);
       if (gpio_get_data (2, 17) == 1){
                   det_dm_hi = 1;
//		   mc13783_otg_wakeup ();
//		   gpio_config_int_en(2, 16, FALSE);
       }
       else{
                   det_dm_hi = 0;
//		   gpio_config_int_en(2, 17, TRUE);
       }
       TRACE_MSG1(TCD, "Changing the state of DM to %d", det_dm_hi);
       mc13783_otg_wakeup ();
       gpio_config_int_en(2, 17, TRUE);
       return IRQ_HANDLED;
}
static irqreturn_t gpio_c16_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
       gpio_config_int_en(2, 16, FALSE);
       udelay(100);
       if (gpio_get_data (2, 16) == 1){
                    det_dp_hi = 1;
//	            mc13783_otg_wakeup ();
//		    gpio_config_int_en(2, 17, FALSE);
       }
       else{
                    det_dp_hi = 0;
//		    gpio_config_int_en(2, 16, TRUE);
       }
       TRACE_MSG1(TCD, "Changing the state of DP to %d", det_dp_hi);
       mc13783_otg_wakeup ();
       gpio_config_int_en(2, 16, TRUE);
       return IRQ_HANDLED;
}

#endif



/*! mxc_mc13783_id - Do we have Vbus (cable attached?)
 * Return non-zero if Vbus is detected.
 *
 */
int mxc_mc13783_id (struct otg_instance *otg)
{
        #if 1
        t_sense_bits sense_bits;
        if (mc13783_get_sense(&sense_bits)) {
                printk(KERN_INFO"%s: mc13783_get_sense() failed\n", __FUNCTION__);
                return 0;
        }
        return sense_bits.sense_id_gnds;
        #else
        return mc13783_check_sense(sense_id_gnds);
        #endif
}


/* ********************************************************************************************* */
/*! mxc_mc13783_tcd_en() - used to enable 
 *
 */
void mxc_mc13783_tcd_en(struct otg_instance *otg, u8 flag)
{
        switch (flag) {
        case SET:
        case PULSE:
                TRACE_MSG0(TCD, "SET/PULSE");
		mc13783_otg_wakeup ();
//                otg_event_set_irq(tcd_instance->otg, 1, mxc_mc13783_vbus(tcd_instance->otg), B_SESS_VLD, TCD, "B_SESS_VLD");
                break;
        case RESET:
                TRACE_MSG0(TCD, "RESET");
                break;
        }
}


/*! mxc_mc13783_chrg_vbus - used to enable or disable B-device Vbus charging
 */
void mxc_mc13783_chrg_vbus(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "CHRG_VBUS_SET");
                global_flag_array[end_flag] = CHRG_VBUS_SET;
                mc13783_bh_wakeup();
                break;
        case RESET:
                TRACE_MSG0(TCD, "CHRG_VBUS_RESET");
                global_flag_array[end_flag] = CHRG_VBUS_RESET;
                mc13783_bh_wakeup();
                break;
        case PULSE:
                break;
        }
        if (end_flag ++ > 15)
                end_flag = 0;
	
	
}

/*! mxc_mc13783_drv_vbus - used to enable or disable A-device driving Vbus 
 */
void mxc_mc13783_drv_vbus(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "DRV_VBUS_SET");
                global_flag_array[end_flag] = DRV_VBUS_SET;
                mc13783_bh_wakeup();
                break;
        case RESET:
                TRACE_MSG0(TCD, "DRV_VBUS_RESET");
                global_flag_array[end_flag] = DRV_VBUS_RESET;
                mc13783_bh_wakeup();
                break;
        }
        if (end_flag ++ > 15)
                end_flag = 0;
	
}

/*! mxc_mc13783_dischrg_vbus - used to enable Vbus discharge
 */
void mxc_mc13783_dischrg_vbus(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "DISCHRG VBUS SET");
                global_flag_array[end_flag] = DISCHRG_VBUS_SET;
                mc13783_bh_wakeup();
                break;
        case RESET:
                TRACE_MSG0(TCD, "DISCHRG VBUS RESET");
                global_flag_array[end_flag] = DISCHRG_VBUS_RESET;
                mc13783_bh_wakeup();
                break;
        }
}

/*! mxc_mc13783_mx21_vbus_drain - used to enable Vbus discharge
 */
void mxc_mc13783_mx21_vbus_drain_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "OUTPUT: TCD_DISCHRG_VBUS_SET");
                break;
        case RESET:
                TRACE_MSG0(TCD, "OUTPUT: TCD_DISCHRG_VBUS_RESET");
                break;
        }
}

/*! mxc_mc13783_dp_pullup_func - used to enable or disable peripheral connecting to bus
 *
 * C.f. 5.1.6, 5.1.7, 5.2.4 and 5.2.5
 *
 *                              host    peripheral
 *              d+ pull-up      clr     set
 *              d+ pull-down    set     clr
 *
 *              d- pull-up      clr     clr
 *              d- pull-down    set     set
 *
 */
void mxc_mc13783_dp_pullup_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "ISP1301_LOC_CONN SET - Set DP PULLUP");
//		global_flag |= PU_FLAG_SET;
		global_flag_array[end_flag] = PUDP_FLAG_SET;
		mc13783_bh_wakeup();
//               mc13783_convity_set_pull_down_switch(PD_PU, TRUE);
                break;

        case RESET:
                TRACE_MSG0(TCD, "ISP1301_LOC_CONN RESET - Clr DP PULLUP");
//		global_flag |= PU_FLAG_RESET;
		global_flag_array[end_flag] = PUDP_FLAG_RESET;
		mc13783_bh_wakeup();
//                mc13783_convity_set_pull_down_switch(PD_PU, FALSE);
                break;
        }
	if (end_flag ++ > 15)
		end_flag = 0;
}

/*! mxc_mc13783_dm_pullup_func - used to enable or disable peripheral connecting to bus
 *
 */
void mxc_mc13783_dm_pullup_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "ISP1301_LOC_CONN SET - Set DM PULLUP");
//		global_flag |= PU_FLAG_SET;
		global_flag_array[end_flag] = PUDM_FLAG_SET;
		mc13783_bh_wakeup();
                //mc13783_convity_set_pull_down_switch(PD_UDB_15, TRUE);

                break;

        case RESET:
                TRACE_MSG0(TCD, "ISP1301_LOC_CONN RESET - Clr DM PULLUP");
//              global_flag |= PU_FLAG_RESET;
		global_flag_array[end_flag] = PUDM_FLAG_RESET;
		mc13783_bh_wakeup();
                //mc13783_convity_set_pull_down_switch(PD_UDB_15, FALSE);
                break;
        }
        if (end_flag ++ > 15)
                end_flag = 0;
	
}

/*! mxc_mc13783_dp_pulldown_func - used to enable or disable peripheral connecting to bus
 *
 * C.f. 5.1.6, 5.1.7, 5.2.4 and 5.2.5
 *
 *                              host    peripheral
 *              d+ pull-up      clr     set
 *              d+ pull-down    set     clr
 *
 *              d- pull-up      clr     clr
 *              d- pull-down    set     set
 *
 */
void mxc_mc13783_dp_pulldown_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "ISP1301_LOC_CONN SET - Set DP PULLDOWN");
//		global_flag |= UPD_FLAG_SET;
		global_flag_array[end_flag] = UPD_FLAG_SET;
		mc13783_bh_wakeup();
//                mc13783_convity_set_pull_down_switch(PD_UPD_15, TRUE);
                break;

        case RESET:
                TRACE_MSG0(TCD, "ISP1301_LOC_CONN RESET - Clr DP PULLDOWN");
//		global_flag |= UPD_FLAG_RESET;
		global_flag_array[end_flag] = UPD_FLAG_RESET;
		mc13783_bh_wakeup();
//               mc13783_convity_set_pull_down_switch(PD_UPD_15, TRUE);
                break;
        }
        if (end_flag ++ > 15)
                end_flag = 0;
	
}

/*! mxc_mc13783_dm_pulldown_func - used to enable or disable peripheral connecting to bus
 *
 */
void mxc_mc13783_dm_pulldown_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "ISP1301_LOC_CONN SET - Set DM PULLDOWN");
//		global_flag |= UDM_FLAG_SET;
		global_flag_array[end_flag] = UDM_FLAG_SET;		
		mc13783_bh_wakeup();
//                mc13783_convity_set_pull_down_switch(PD_UDM_15, TRUE);
                break;

        case RESET:
                TRACE_MSG0(TCD, "ISP1301_LOC_CONN RESET - Clr DM PULLDOWN");
//		global_flag |= UDM_FLAG_RESET;
		global_flag_array[end_flag] = UDM_FLAG_RESET;
		mc13783_bh_wakeup();
//                mc13783_convity_set_pull_down_switch(PD_UDM_15, TRUE);
                break;
        }
        if (end_flag ++ > 15)
	       end_flag = 0;
	
}

/*! mxc_mc13783_peripheral_host_func - used to enable or disable peripheral connecting to bus
 *
 * A-Device             D+ pulldown     D- pulldown
 *      idle            set             set
 *      host            set             set
 *      peripheral      reset           set
 *
 * B-Device
 *      idle            set             set
 *      host            set             set
 *      peripheral      reset           set
 */
void mxc_mc13783_peripheral_host_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:       // peripheral
                TRACE_MSG0(TCD, "SET - CLR DP PULLDOWN");
                break;

        case RESET:     // host
                TRACE_MSG0(TCD, "RESET - SET DM PULLDOWN");
                break;
        }
}

/*! mxc_mc13783_dm_det_func - used to enable or disable D- detect
 *
 */
void mxc_mc13783_dm_det_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "setting DM_HI detect");
#if 0		
		#ifdef CONFIG_ARCH_MXC91231	
		iomux_config_mux(AP_GPIO_AP_C17,OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
		gpio_config(2, 17, false, GPIO_INT_RISE_EDGE); //GPIO_INT_HIGH_LEV);
		gpio_request_irq(2, 17, GPIO_HIGH_PRIO, gpio_c17_int_hndlr,
		                                SA_SHIRQ, "DM_DET", NULL);
		gpio_config_int_en(2, 17, TRUE);  // XXX this might not be needed
		#endif		
#endif		
                break;

        case RESET:
                TRACE_MSG0(TCD, "reseting DM_HI detect");
#if 0		
		#ifdef CONFIG_ARCH_MXC91231
		gpio_config_int_en(2, 17, FALSE);
		gpio_free_irq (2, 17, GPIO_HIGH_PRIO);
		#endif		
#endif		
                break;
        }
}

/*! mxc_mc13783_dp_det_func - used to enable or disable D+ detect
 *
 */
void mxc_mc13783_dp_det_func(struct otg_instance *otg, u8 flag)

{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "setting DP_HI detect");
#if 0		
		#ifdef CONFIG_ARCH_MXC91231
                iomux_config_mux(AP_GPIO_AP_C16,OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
                gpio_config(2, 16, false, GPIO_INT_RISE_EDGE); //GPIO_INT_HIGH_LEV);
                gpio_request_irq(2, 16, GPIO_HIGH_PRIO, gpio_c16_int_hndlr,
                                                SA_SHIRQ, "DP_DET", NULL);
                gpio_config_int_en(2, 16, TRUE);  // XXX this might not be needed
		#endif		
#endif		
                break;

        case RESET:
                TRACE_MSG0(TCD, "reseting DP_HI detect");
#if 0		
		#ifdef CONFIG_ARCH_MXC91231
		gpio_config_int_en(2, 16, FALSE);
		gpio_free_irq (2, 16, GPIO_HIGH_PRIO);
		#endif		
#endif		
                break;
        }
}

/*! mxc_mc13783_cr_det_func - used to enable or disable D+ detect
 *
 */
void mxc_mc13783_cr_det_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "setting CR_INT detect");
                break;

        case RESET:
                TRACE_MSG0(TCD, "reseting CR_INT detect");
                break;
        }
}

/*! mxc_mc13783_bdis_acon_func - used to enable or disable auto a-connect
 *
 */
void mxc_mc13783_bdis_acon_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "setting BDIS ACON");
                break;

        case RESET:
                TRACE_MSG0(TCD, "reseting BDIS ACON");
                break;
        }
}

/*! mxc_mc13783_id_pulldown_func - used to enable or disable ID pulldown
 *
 */
void mxc_mc13783_id_pulldown_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "setting ID PULLDOWN");
                break;

        case RESET:
                TRACE_MSG0(TCD, "reseting ID PULLDOWN");
                break;
        }
}

/*! mxc_mc13783_audio_func - used to enable or disable Carkit Interrupt
 *
 */
void mxc_mc13783_audio_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "SET AUDIO_EN");
                break;

        case RESET:
                TRACE_MSG0(TCD, "RESET AUDIO_EN");
                break;
        }
}

/*! mxc_mc13783_uart_func - used to enable or disable transparent uart mode
 *
 */
void mxc_mc13783_uart_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "setting UART_EN");
                /* XXX enable uart */
                break;

        case RESET:
                TRACE_MSG0(TCD, "reseting UART_EN");
                /* XXX disable uart */
                break;
        }
}

/*! mxc_mc13783_mono_func - used to enable or disable mono audio connection
 *
 */
void mxc_mc13783_mono_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "setting MONO");
                /* XXX enable mono output */
                break;

        case RESET:
                TRACE_MSG0(TCD, "reseting MONO");
                /* XXX disable mono output */
                break;
        }
}


/* ********************************************************************************************* */
/*!
 * mxc_mc13783_usbi_handler() - event handler
 * 
 * 
 *
 */             
void mc13783_usbi_handler (void)
{       

        TRACE_MSG0(TCD, "--");
	mc13783_otg_wakeup ();
	
#if 0	
        #if 1
        t_sense_bits sense_bits;
        if (mc13783_get_sense(&sense_bits)) {
                printk(KERN_INFO"%s: mc13783_get_sense() failed\n", __FUNCTION__);
                return;
        }
        
        TRACE_MSG3(TCD, " EVENT: 4V4S: %d 2V0S: %d 0V8S: %d",
                        sense_bits.sense_usb4v4s, 
                        sense_bits.sense_usb2v0s, 
                        sense_bits.sense_usb0v8s
                        );

        otg_event(tcd_instance->otg, 
                        (sense_bits.sense_usb4v4s ? VBUS_VLD : VBUS_VLD_) |
                        (sense_bits.sense_usb2v0s ? (B_SESS_VLD | A_SESS_VLD) : (B_SESS_VLD_ | A_SESS_VLD_)) |
                        (sense_bits.sense_usb0v8s ? B_SESS_END : B_SESS_END_),
                        TCD, "MC13783 USBI");
        #else
        otg_event(tcd_instance->otg, 
                        (mc13783_check_sense(sense_usb4v4s) ? VBUS_VLD : VBUS_VLD_) |
                        (mc13783_check_sense(sense_usb4v4s) ? (B_SESS_VLD | A_SESS_VLD) : (B_SESS_VLD_ | A_SESS_VLD_)) |
                        (mc13783_check_sense(sense_usb0v8s) ? B_SESS_END : B_SESS_END_),
                        TCD, "MC13783 USBI");
        #endif
#endif
}

void mc13783_idi_handler (void)
{        

        TRACE_MSG0(TCD, "--");
	mc13783_otg_wakeup ();
	
#if 0	
        #if 1
        t_sense_bits sense_bits;
        if (mc13783_get_sense(&sense_bits)) {
                printk(KERN_INFO"%s: mc13783_get_sense() failed\n", __FUNCTION__);
                return;
        }
        
        TRACE_MSG2(TCD, "MC13783 EVENT: IDGNDS: %d IDFLOATS: %d", sense_bits.sense_id_gnds, sense_bits.sense_id_floats);

        otg_event(tcd_instance->otg, 
                        (sense_bits.sense_id_gnds ? ID_GND : ID_GND_ ) | 
                        (sense_bits.sense_id_floats ? ID_FLOAT : ID_FLOAT_),
                        TCD, "MC13783 IDI");
        #else
        otg_event(tcd_instance->otg, 
                        (mc13783_check_sense(sense_id_gnds) ? ID_GND : ID_GND_ ) | 
                        (mc13783_check_sense(sense_id_floats) ? ID_FLOAT : ID_FLOAT_),
                        TCD, "MC13783 IDI");
        #endif
#endif
}

void mc13783_se1i_handler (void)
{        

        TRACE_MSG0(TCD, "--");
	mc13783_otg_wakeup ();

#if 0	
        #if 1
        t_sense_bits sense_bits;
        if (mc13783_get_sense(&sense_bits)) {
                printk(KERN_INFO"%s: mc13783_get_sense() failed\n", __FUNCTION__);
                return;
        }
        TRACE_MSG1(TCD, "MC13783 EVENT: se1: %d", sense_bits.sense_se1s);
        otg_event(tcd_instance->otg, 
                        (sense_bits.sense_se1s ? SE1_DET : SE1_DET_) ,
                        TCD, "MC13783 SE1");
        #else
        otg_event(tcd_instance->otg, 
                        (mc13783_check_sense(sense_se1s) ? SE1_DET : SE1_DET_) ,
                        TCD, "MC13783 SE1");
        #endif
#endif	
}



void mxc_mc13783_mod_init (void);
void mxc_mc13783_mod_exit (void);

void mxc_mc13783_mod_exit(void)
{
					
        while (PENDING_WORK_ITEM(mc13783_work_bh)) {
                printk(KERN_ERR"%s: waiting for mc13783_work_bh\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }
        while (PENDING_WORK_ITEM(mc13783_otg_wq)) {
                printk(KERN_ERR"%s: waiting for mc13783_otg_wq\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }

	mc13783_convity_set_interface_mode(IM_RS232_1);
	mc13783_convity_set_var_disconnect(TRUE);

	mc13783_convity_set_udp_auto_connect (FALSE);
        mc13783_convity_set_udp_pull (FALSE);
	mc13783_convity_set_pull_down_switch(PD_PU, FALSE);
        mc13783_convity_set_pull_down_switch(PD_UPD_15, FALSE);
        mc13783_convity_set_pull_down_switch(PD_UDM_15, FALSE);
								
	
        mc13783_convity_event_unsub(IT_CONVITY_USBI, mc13783_usbi_handler);
        mc13783_convity_event_unsub(IT_CONVITY_IDI, mc13783_idi_handler);
        mc13783_convity_event_unsub(IT_CONVITY_SE1I, mc13783_se1i_handler);
			
		
}


/* ********************************************************************************************* */


void mxc_mc13783_mod_init(void)
{

        bool transceiver;

	TRACE_MSG0(TCD, "Setup the work item");
	PREPARE_WORK_ITEM(mc13783_work_bh, &mc13783_bh, NULL);
	PREPARE_WORK_ITEM(mc13783_otg_wq, &mc13783_otg_event_bh, NULL);

        if (mc13783_convity_get_usb_transceiver(&transceiver)) 
                printk(KERN_INFO"%s: mc13783_convity_get_usb_transciver failed\n", __FUNCTION__);

        printk(KERN_INFO"%s: tw: %02x\n", __FUNCTION__, transceiver);

        mc13783_convity_event_sub(IT_CONVITY_USBI, mc13783_usbi_handler);
        mc13783_convity_event_sub(IT_CONVITY_IDI, mc13783_idi_handler);
        mc13783_convity_event_sub(IT_CONVITY_SE1I, mc13783_se1i_handler);

	
        // XXX it may be more appropriate to do this in the enable function
        // and reset to something when disaabled
        mc13783_convity_set_interface_mode(IM_USB);
        mc13783_convity_set_var_disconnect(TRUE);
        mc13783_convity_set_udp_pull (TRUE);

	start_flag = end_flag = 0;
 
}

