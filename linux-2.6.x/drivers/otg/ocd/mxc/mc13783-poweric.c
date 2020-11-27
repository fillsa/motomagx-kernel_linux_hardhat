/*
 * otg/ocd/mxc/mxc-mc13783-poweric.c -- Sierra MC13783 Connectivity Transceiver Controller driver
 *
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
 *
 *      Copyright (c) 2005 Belcarra
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 04/29/2005         Motorola         new file based on "mxc-mc13783.c", 
 *                                        use Motorola mc13783 support for usbotg transciever
 *		                          NOTE: Using Motorola power_ic core API directly since 
 *                                        Conn API is not ready, need to move when team 
 *                                        declares their mc13783 Conn support done.
 * 06/27/2005         Motorola         change to Motorola's conn APIs, removed h/w config 
 *                                        calls, using accy detect from user space to start 
 *                                        up and not getting usbi events 
 * 10/25/2005  	      Motorola         made changes to call power_ic as bottom half as 
 *                                        seen in mxc_mc13783.c
 * 01/05/2006         Motorola         Merge in Belcarra 2.6.2 release
 * 06/12/2006         Motorola         OTG name change
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 * 12/20/2006         Motorola         Add power_ic_kernel.h include 
 */
/*!
 * @file otg/ocd/mxc/mxc-mc13783-poweric.c
 * @brief mc13783 Transciever Controller Driver using Motorola driver
 *
 * This is a simple transceiver driver for the mc13783 transceiver
 * using the Motorola mc13783 connectivity driver.
 *
 * @ingroup FSOTG
 *
 * 
 */

#include <asm/delay.h>

#include <otg/pcd-include.h>
#include <asm/mot-gpio.h>

#include <core/mc13783_external.h> // Used for the t_sense structure definition

#include <linux/power_ic_kernel.h>

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


// temp defines for bits in mc13783 reg USB0 
// need to remove this when going to Motorola's Conn API
#define         BIT_FSENB               0
#define         BIT_USBSUSPEND          1
#define         BIT_USBPU               2
#define         BIT_UDPPD               3
#define         BIT_UDMPD               4
#define         BIT_DP150KPU            5
#define         BIT_VBUSPDENB           6
#define         BITS_CURRENT_LIMIT      7
#define         LONG_CURRENT_LIMIT      3
#define         BIT_DLPSRP              10
#define         BIT_SE0CONN             11
#define         BIT_USBXCVREN           12
#define         BIT_PULLOVER            13
#define         BITS_INTERFACE_MODE     14
#define         LONG_INTERFACE_MODE     3
#define         BIT_DATSE0              17
#define         BIT_BIDIR               18
#define         BIT_USBCNTRL            19
#define         BIT_IDPD                20
#define         BIT_IDPULSE             21
#define         BIT_IDPUCNTRL           22
#define         BIT_DMPULSE             23


// temp defines for bits in mc13783 reg USB1
#define         MODE_USB                0

// Vbus sense bits for mc13783 Int Sense0 reg
#define		BIT_USB4V4S		16
#define		BIT_USB2V0S		17
#define		BIT_USB0V8S		18
#define   	BIT_IDFLOATS            19
#define 	BIT_IDGNDS		20
#define 	BIT_SE1S		21


/*! mc13783_bh
 *  */
void mc13783_bh(void *arg)
{
	TRACE_MSG0(TCD, "--");
	global_flag = global_flag_array[start_flag];
	if (global_flag & PUDP_FLAG_SET){	//set DP pullup
    	//mc13783_convity_set_speed_mode (FALSE); //set high speed
                //mc13783_convity_set_pull_down_switch(PD_PU, TRUE);  //variable 1.5K pull-up switch in
		power_ic_periph_set_usb_pull_up(TRUE);
	}
	if (global_flag & PUDP_FLAG_RESET){	//reset DP pullup
		//mc13783_convity_set_speed_mode (FALSE); //set high speed
                //mc13783_convity_set_pull_down_switch(PD_PU, FALSE);  //variable 1.5K pull-up switch off
		power_ic_periph_set_usb_pull_up(FALSE); // Turn off the USB pullup
	}
        if (global_flag & PUDM_FLAG_SET){	//set DM pullup
                //mc13783_convity_set_speed_mode (TRUE); //set low speed
                //mc13783_convity_set_pull_down_switch(PD_PU, TRUE);   //variable 1.5K pull-up switch in
		power_ic_periph_set_usb_pull_up(TRUE);
        }		
        if (global_flag & PUDM_FLAG_RESET){	//reset DM pullup
                //mc13783_convity_set_speed_mode (TRUE); //set low speed
                //mc13783_convity_set_pull_down_switch(PD_PU, FALSE);   //variable 1.5K pull-up switch off
                power_ic_periph_set_usb_pull_up(FALSE); // Turn off the USB pullup
        }
	if (global_flag & UPD_FLAG_SET){	//set DP pulldown
		power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_UDPPD, TRUE);
                //mc13783_convity_set_pull_down_switch(PD_UPD_15, TRUE);   //DP pull down switch is on
	}
	if (global_flag & UPD_FLAG_RESET){	//reset DP pulldown
		power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_UDPPD, FALSE);
                //mc13783_convity_set_pull_down_switch(PD_UPD_15, FALSE);  //DP pull down switch is off
	}
	if (global_flag & UDM_FLAG_SET){	//set DM pulldown
		power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_UDMPD, TRUE); 
                //mc13783_convity_set_pull_down_switch(PD_UDM_15, TRUE);   //DP pull down switch is on
	}
	if (global_flag & UDM_FLAG_RESET){	//reset DM pulldown
		power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_UDMPD, FALSE);
                //mc13783_convity_set_pull_down_switch(PD_UDM_15, FALSE);  //DP pull down switch is off
	}
/*  Used for OTG
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
//		mc13783convity_set_output (FALSE, FALSE);     //disable VUSB
	}
*/ 
	
	TRACE_MSG3 (TCD, "gloabl flag %d start_flag %d end_flag %d", global_flag, start_flag, end_flag);

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
	unsigned int reg_value = power_ic_event_sense_read(POWER_IC_EVENT_ATLAS_USB2V0S);

        if (reg_value < 0) {
                printk(KERN_INFO"%s: power_ic_get_sense() failed\n", __FUNCTION__);
                return 0;
        }
        return reg_value; 
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

// Note: power_ic has USB4V4S labelled as USBI, which is incorrect

	// Get the sense bits, return if any fail to be read. 
	if ( (sense_bits.sense_usb4v4s = power_ic_event_sense_read(POWER_IC_EVENT_ATLAS_USBI)) < 0) {
                printk(KERN_INFO"%s: mc13783_get_sense() usb4v4s failed\n", __FUNCTION__);
		return;
        }       
	if ( (sense_bits.sense_usb2v0s = power_ic_event_sense_read(POWER_IC_EVENT_ATLAS_USB2V0S)) < 0) {
                printk(KERN_INFO"%s: mc13783_get_sense() usb2v0s failed\n", __FUNCTION__);
		return;
        }       
	if ( (sense_bits.sense_usb0v8s = power_ic_event_sense_read(POWER_IC_EVENT_ATLAS_USB0V8S)) < 0) {
                printk(KERN_INFO"%s: mc13783_get_sense() usb0v8s failed\n", __FUNCTION__);
		return;
        }       
	if ( (sense_bits.sense_id_floats = power_ic_event_sense_read(POWER_IC_EVENT_ATLAS_ID_FLOAT)) < 0) {
                printk(KERN_INFO"%s: mc13783_get_sense() id_floats failed\n", __FUNCTION__);
		return;
        }       
	if ( (sense_bits.sense_id_gnds = power_ic_event_sense_read(POWER_IC_EVENT_ATLAS_ID_GROUND)) < 0) {
                printk(KERN_INFO"%s: mc13783_get_sense() id_gnds failed\n", __FUNCTION__);
		return;
        }       
	if ( (sense_bits.sense_se1s = power_ic_event_sense_read(POWER_IC_EVENT_ATLAS_SE1I)) < 0) {
                printk(KERN_INFO"%s: mc13783_get_sense() se1s failed\n", __FUNCTION__);
		return;
        }       

    // Factory cable check meant for USB B device only. If IDGNDS is True 
    // adjust the value of IDGNDS to be False, so the state machine thinks it's a
    // traditional device and not a Dual role device.
#ifdef CONFIG_OTG_USB_PERIPHERAL
    if ( sense_bits.sense_id_gnds ) {
        sense_bits.sense_id_gnds = FALSE;
	    TRACE_MSG0(TCD, "Factory Cable detected and IDGND modified to false"); 
	    // printk("Factory Cable detected and IDGND modified to false\n"); 
    }
#endif

	inputs = (sense_bits.sense_usb4v4s ? VBUS_VLD : VBUS_VLD_) |
		 (sense_bits.sense_usb2v0s ? (B_SESS_VLD | A_SESS_VLD) : (B_SESS_VLD_ | A_SESS_VLD_)) |
		 (sense_bits.sense_usb0v8s ? B_SESS_END_ : B_SESS_END) |
		 (sense_bits.sense_id_gnds ? ID_GND : ID_GND_) |
                 (sense_bits.sense_id_floats ? ID_FLOAT : ID_FLOAT_) |
		 (sense_bits.sense_se1s ? SE1_DET : SE1_DET_) |
		 (det_dp_hi ? DP_HIGH : DP_HIGH_) |
		 (det_dm_hi ? DM_HIGH : DM_HIGH_);



	TRACE_MSG4(TCD, "MC13783 EVENT: sense_bits: %8x otg inputs: %8x saved: %x diff: %x", 
                        sense_bits.sense_se1s, inputs, inputs_saved, inputs ^ inputs_saved);

        RETURN_UNLESS(force || (inputs ^ inputs_saved));

        inputs_saved = inputs;
        otg_event(tcd_instance->otg, inputs, TCD, "MC13783 OTG EVENT");
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
        int value = power_ic_event_sense_read(POWER_IC_EVENT_ATLAS_ID_GROUND);
        if ( value < 0 ) {
                printk(KERN_INFO"%s: power_ic_event_sense_read() failed\n", __FUNCTION__);
                return 0;
        } 
        return value;
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
                TRACE_MSG0(TCD, "SIERRA mc13783 for MXC91231 - Set DP PULLUP");
		global_flag_array[end_flag] = PUDP_FLAG_SET;
		mc13783_bh_wakeup();
                break;

        case RESET:
                TRACE_MSG0(TCD, "SIERRA mc13783 for MXC91231 - Clr DP PULLUP");
		global_flag_array[end_flag] = PUDP_FLAG_RESET;
		mc13783_bh_wakeup();
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
                TRACE_MSG0(TCD, "SIERRA mc13783 for MXC91231 - Set DM PULLUP");
		global_flag_array[end_flag] = PUDM_FLAG_SET;
		mc13783_bh_wakeup();
                break;

        case RESET:
                TRACE_MSG0(TCD, "SIERRA mc13783 for MXC91231 - Clr DM PULLUP");
		global_flag_array[end_flag] = PUDM_FLAG_RESET;
		mc13783_bh_wakeup();
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
                TRACE_MSG0(TCD, "SIERRA mc13783 for MXC91231- Set DP PULLDOWN");
		global_flag_array[end_flag] = UPD_FLAG_SET;
		mc13783_bh_wakeup();
                break;

        case RESET:
                TRACE_MSG0(TCD, "SIERRA mc13783 for MXC91231- Clr DP PULLDOWN");
		global_flag_array[end_flag] = UPD_FLAG_RESET;
		mc13783_bh_wakeup();
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
                TRACE_MSG0(TCD, "SIERRA mc13783 for MXC91231- Set DM PULLDOWN");
		global_flag_array[end_flag] = UDM_FLAG_SET;		
		mc13783_bh_wakeup();
                break;

        case RESET:
                TRACE_MSG0(TCD, "SIERRA mc13783 for MXC91231 - Clr DM PULLDOWN");
		global_flag_array[end_flag] = UDM_FLAG_RESET;
		mc13783_bh_wakeup();
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
                break;

        case RESET:
                TRACE_MSG0(TCD, "reseting DM_HI detect");
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
                break;

        case RESET:
                TRACE_MSG0(TCD, "reseting DP_HI detect");
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
 * Called when mc13783 generates a USBI interrupt event, this is from Vbus on USB cable attach 
 *
 */             
void mc13783_usbi_handler (void)
{       

        TRACE_MSG0(TCD, "mc13783 generates a USBI interrupt event");
	mc13783_otg_wakeup ();

}

void mc13783_idi_handler (void)
{        

        TRACE_MSG0(TCD, "--");
	mc13783_otg_wakeup ();
}

void mc13783_se1i_handler (void)
{        

        TRACE_MSG0(TCD, "--");
	mc13783_otg_wakeup ();

}



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

/* Investigate if these exist
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
			
*/								
// Remove the following lines if above are needed. 	
	power_ic_periph_set_usb_pull_up(FALSE);
	power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_UDPPD, FALSE); 
	power_ic_set_reg_bit(POWER_IC_REG_ATLAS_USB_0, BIT_UDMPD, FALSE);
		
}


/* ********************************************************************************************* */


void mxc_mc13783_mod_init(void)
{

        bool transceiver;

	TRACE_MSG0(TCD, "Setup the work item");
	PREPARE_WORK_ITEM(mc13783_work_bh, &mc13783_bh, NULL);
	PREPARE_WORK_ITEM(mc13783_otg_wq, &mc13783_otg_event_bh, NULL);

/*	if (power_ic_get_reg_value(POWER_IC_REG_ATLAS_USB_0, BIT_USBXCVREN, &value, 1) < 0) { 
                printk("%s: power_ic_get_reg_value failed\n", __FUNCTION__);
	}

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
*/
	start_flag = end_flag = 0;
 
}

