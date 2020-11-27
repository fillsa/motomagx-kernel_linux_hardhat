/*
 * otg/ocd/mxc/mxc-pmic.c -- Freescale pmic Connectiviey Transceiver Controller driver
 * @(#) balden@seth2.belcarratech.com|otg/ocd/mxc/mxc-roadrunner.c|20051116203403|31667
 *
 *      Copyright (c) 2005 Belcarra
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 08/15/2006         Motorola         Initial distribution
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
 * @file otg/ocd/mxc/mxc-roadrunner.c
 * @brief pmic Transciever Controller Driver
 *
 * This is a simple transceiver driver for the pmic transceiver
 * using the Freescale pmic connectivity driver.
 *
 * @ingroup FSOTG
 *
 * 
 */

#include <otg/pcd-include.h>
#include <asm/arch/gpio.h>

#include <asm/arch/pmic_convity.h>
#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_status.h>

PMIC_CONVITY_HANDLE rr_handle = (PMIC_CONVITY_HANDLE)NULL;	
PMIC_STATUS rc     = PMIC_SUCCESS;


WORK_ITEM pmic_work_bh;
WORK_ITEM pmic_otg_wq;


int global_flag = 0;

#define PU_FLAG_SET 1
#define PU_FLAG_RESET 2
#define UPD_FLAG_SET 4
#define UPD_FLAG_RESET 8
#define UDM_FLAG_SET 16
#define UDM_FLAG_RESET 32
#define VBUSPDENB_RESET 64
#define VBUSREGEN_RESET 128

/*! pmic_bh
 *  */
void pmic_bh(void *arg)
{
unsigned int reg_write;
unsigned int reg_mask;


	TRACE_MSG0(TCD, "--");
	
	if (global_flag & PU_FLAG_SET){
		reg_mask = 0x000004;
		reg_write = 0x000004;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
	}
	if (global_flag & PU_FLAG_RESET){
		reg_mask = 0x000004;
		reg_write = 0x000000;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
	}
	if (global_flag & UPD_FLAG_SET){
		reg_mask = 0x000008;
		reg_write = 0x000008;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
	}
	if (global_flag & UPD_FLAG_RESET){
		reg_mask = 0x000008;
		reg_write = 0x000000;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
	}
	if (global_flag & UDM_FLAG_SET){
		reg_mask = 0x000010;
		reg_write = 0x000010;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
	}
	if (global_flag & UDM_FLAG_RESET){
		reg_mask = 0x000010;
		reg_write = 0x000000;
		pmic_write_reg(PRIO_CONN, REG_BUSCTRL, reg_write, reg_mask);
	}
        if (global_flag & VBUSPDENB_RESET){
	}
        if (global_flag & VBUSREGEN_RESET){
	}

#if 1
        printk (KERN_INFO"%d %d %d %d %d %d %d %d\n", (global_flag & PU_FLAG_SET), (global_flag & PU_FLAG_RESET), \
                        (global_flag & UPD_FLAG_SET), (global_flag & UPD_FLAG_RESET), \
                        (global_flag & UDM_FLAG_SET), (global_flag & UDM_FLAG_RESET), \
                        (global_flag & VBUSPDENB_RESET), (global_flag & VBUSREGEN_RESET));
#endif

	global_flag = 0;	
	
#if 0
	if (global_flag & PU_FLAG_SET)
		pmic_convity_set_pull_down_switch(PD_PU, TRUE);
	if (global_flag & PU_FLAG_RESET)
		pmic_convity_set_pull_down_switch(PD_PU, FALSE);
	if (global_flag & UPD_FLAG_SET)
		pmic_convity_set_pull_down_switch(PD_UPD_15, TRUE);
	if (global_flag & UPD_FLAG_RESET)
		pmic_convity_set_pull_down_switch(PD_UPD_15, FALSE);
	if (global_flag & UDM_FLAG_SET)
		pmic_convity_set_pull_down_switch(PD_UDM_15, TRUE);
	if (global_flag & UDM_FLAG_RESET)
		pmic_convity_set_pull_down_switch(PD_UDM_15, FALSE);
        if (global_flag & VBUSPDENB_RESET){
		pmic_convity_set_pull_down_switch(PD_PU, FALSE);
		pmic_convity_set_pull_down_switch(PD_UPD_15, TRUE);
		pmic_convity_set_pull_down_switch(PD_UDM_15, TRUE);
		pmic_convity_set_vbus (FALSE);
	}
        if (global_flag & VBUSREGEN_RESET)
	        pmic_convity_set_output (TRUE, FALSE);
	
#if 0	
	printk (KERN_INFO"%d %d %d %d %d %d %d %d\n", (global_flag & PU_FLAG_SET), (global_flag & PU_FLAG_RESET), \
			(global_flag & UPD_FLAG_SET), (global_flag & UPD_FLAG_RESET), \
			(global_flag & UDM_FLAG_SET), (global_flag & UDM_FLAG_RESET), \
			(global_flag & VBUSPDENB_RESET), (global_flag & VBUSREGEN_RESET));
#endif	
	global_flag = 0;
	
#endif	
}



/*! pmic_bh_wakeup - wakeup the pmic bottom half
 *  */
void pmic_bh_wakeup(void)
{
        TRACE_MSG0(TCD, "--");
        SCHEDULE_WORK(pmic_work_bh);
}


/* ********************************************************************************************** */
/*! mxc_pmic_vbus - Do we have Vbus (cable attached?)
 * Return non-zero if Vbus is detected.
 *
 */
int mxc_pmic_vbus (struct otg_instance *otg)
{
	return 0;
}

void pmic_otg_event_bh (void *arg)
{
        otg_event_set_irq(tcd_instance->otg, 1, mxc_pmic_vbus(tcd_instance->otg), B_SESS_VLD, TCD, "B_SESS_VLD");
}


void pmic_otg_wakeup(void)
{
        TRACE_MSG0(TCD, "--");
        SCHEDULE_WORK(pmic_otg_wq);
}



/*! mxc_pmic_id - Do we have Vbus (cable attached?)
 * Return non-zero if Vbus is detected.
 *
 */
int mxc_pmic_id (struct otg_instance *otg)
{
	return 0;
}


/* ********************************************************************************************* */
/*! mxc_pmic_tcd_en() - used to enable 
 *
 */
void mxc_pmic_tcd_en(struct otg_instance *otg, u8 flag)
{
        switch (flag) {
        case SET:
        case PULSE:
                TRACE_MSG0(TCD, "SET/PULSE");
		pmic_otg_wakeup ();
                break;
        case RESET:
                TRACE_MSG0(TCD, "RESET");
                break;
        }
}


/*! mxc_pmic_chrg_vbus - used to enable or disable B-device Vbus charging
 */
void mxc_pmic_chrg_vbus(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "CHRG_VBUS_SET");
                break;
        case RESET:
                TRACE_MSG0(TCD, "CHRG_VBUS_RESET");
                break;
        case PULSE:
                break;
        }
}

/*! mxc_pmic_drv_vbus - used to enable or disable A-device driving Vbus 
 */
void mxc_pmic_drv_vbus(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "DRV_VBUS_SET");
                break;
        case RESET:
                TRACE_MSG0(TCD, "DRV_VBUS_RESET");
                break;
        }
}

/*! mxc_pmic_dischrg_vbus - used to enable Vbus discharge
 */
void mxc_pmic_dischrg_vbus(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_mx21_vbus_drain - used to enable Vbus discharge
 */
void mxc_pmic_mx21_vbus_drain_func(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_dp_pullup_func - used to enable or disable peripheral connecting to bus
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
void mxc_pmic_dp_pullup_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "Set DP PULLUP");
		global_flag |= PU_FLAG_SET;
		pmic_bh_wakeup();
                break;

        case RESET:
                TRACE_MSG0(TCD, "Clr DP PULLUP");
		global_flag |= PU_FLAG_RESET;
		pmic_bh_wakeup();
                break;
        }
}

/*! mxc_pmic_dm_pullup_func - used to enable or disable peripheral connecting to bus
 *
 */
void mxc_pmic_dm_pullup_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "Set DM PULLUP");
                break;

        case RESET:
                TRACE_MSG0(TCD, "Clr DM PULLUP");
                break;
        }
}

/*! mxc_pmic_dp_pulldown_func - used to enable or disable peripheral connecting to bus
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
void mxc_pmic_dp_pulldown_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "Set DP PULLUP");
		global_flag |= UPD_FLAG_SET;
		pmic_bh_wakeup();
                break;

        case RESET:
                TRACE_MSG0(TCD, "Clr DP PULLUP");
		global_flag |= UPD_FLAG_RESET;
		pmic_bh_wakeup();
                break;
        }
}

/*! mxc_pmic_dm_pulldown_func - used to enable or disable peripheral connecting to bus
 *
 */
void mxc_pmic_dm_pulldown_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(TCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(TCD, "Set DM PULLUP");
		global_flag |= UDM_FLAG_SET;
		pmic_bh_wakeup();
                break;

        case RESET:
                TRACE_MSG0(TCD, "Clr DM PULLUP");
		global_flag |= UDM_FLAG_RESET;
		pmic_bh_wakeup();
                break;
        }
}

/*! mxc_pmic_peripheral_host_func - used to enable or disable peripheral connecting to bus
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
void mxc_pmic_peripheral_host_func(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_dm_det_func - used to enable or disable D- detect
 *
 */
void mxc_pmic_dm_det_func(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_dp_det_func - used to enable or disable D+ detect
 *
 */
void mxc_pmic_dp_det_func(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_cr_det_func - used to enable or disable D+ detect
 *
 */
void mxc_pmic_cr_det_func(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_bdis_acon_func - used to enable or disable auto a-connect
 *
 */
void mxc_pmic_bdis_acon_func(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_id_pulldown_func - used to enable or disable ID pulldown
 *
 */
void mxc_pmic_id_pulldown_func(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_audio_func - used to enable or disable Carkit Interrupt
 *
 */
void mxc_pmic_audio_func(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_uart_func - used to enable or disable transparent uart mode
 *
 */
void mxc_pmic_uart_func(struct otg_instance *otg, u8 flag)
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

/*! mxc_pmic_mono_func - used to enable or disable mono audio connection
 *
 */
void mxc_pmic_mono_func(struct otg_instance *otg, u8 flag)
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



void mxc_pmic_mod_init (void);
void mxc_pmic_mod_exit (void);

void mxc_pmic_mod_exit(void)
{

	rc = pmic_convity_clear_callback(rr_handle);
	if (rc != PMIC_SUCCESS)
	{
		TRACE_MSG1(TCD, "pmic_convity_clear_callback() returned error %d", rc);
	}
	rc = pmic_convity_close(rr_handle);
	if (rc != PMIC_SUCCESS)
	{
		TRACE_MSG1(TCD, "pmic_convity_close() returned error %d", rc);
    	}
		
					
        while (PENDING_WORK_ITEM(pmic_work_bh)) {
                printk(KERN_ERR"%s: waiting for pmic_work_bh\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }
        while (PENDING_WORK_ITEM(pmic_otg_wq)) {
                printk(KERN_ERR"%s: waiting for pmic_otg_wq\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }

#if 0
	pmic_convity_set_interface_mode(IM_RS232_1);
	pmic_convity_set_var_disconnect(TRUE);

	pmic_convity_set_udp_auto_connect (FALSE);
        pmic_convity_set_udp_pull (FALSE);
	pmic_convity_set_pull_down_switch(PD_PU, FALSE);
        pmic_convity_set_pull_down_switch(PD_UPD_15, FALSE);
        pmic_convity_set_pull_down_switch(PD_UDM_15, FALSE);
								
	
        pmic_convity_event_unsub(IT_CONVITY_USBI, pmic_usbi_handler);
        pmic_convity_event_unsub(IT_CONVITY_IDI, pmic_idi_handler);
        pmic_convity_event_unsub(IT_CONVITY_SE1I, pmic_se1i_handler);
			
#endif		
}


void pmic_detect_event (const PMIC_CONVITY_EVENTS event)
{
unsigned int flags = 0;

	flags = 0;
	switch (event){
	    case USB_DETECT_4V4_RISE:
		TRACE_MSG0(TCD, "4V4 rise");
		printk (KERN_INFO"4V4 rise\n");
		flags &= ~(VBUS_VLD_);
		flags |= VBUS_VLD;
	    break;
	    case USB_DETECT_4V4_FALL:
		TRACE_MSG0(TCD, "4V4 fall");
		printk (KERN_INFO"4V4 fall\n");
		flags &= ~(VBUS_VLD);
		flags |= VBUS_VLD_;
	    break;
	    case USB_DETECT_2V0_RISE:
		TRACE_MSG0(TCD, "2V0 rise");
		printk (KERN_INFO"2V0 rise\n");
		flags &= ~(B_SESS_VLD_ | A_SESS_VLD_);
		flags |= (B_SESS_VLD | A_SESS_VLD);
	    break;
	    case USB_DETECT_2V0_FALL:
		TRACE_MSG0(TCD, "2V0 fall");
		printk (KERN_INFO"2V0 fall\n");
		flags &= ~(B_SESS_VLD | A_SESS_VLD);
		flags |= (B_SESS_VLD_ | A_SESS_VLD_);
	    break;
	    case USB_DETECT_0V8_RISE:
		TRACE_MSG0(TCD, "0V8 rise");
		printk (KERN_INFO"0V8 rise\n");
		flags &= ~(B_SESS_END_);
		flags |= B_SESS_END;
	    break;
	    case USB_DETECT_0V8_FALL:
		TRACE_MSG0(TCD, "0V8 fall");
		printk (KERN_INFO"0V8 fall\n");
		flags &= ~(B_SESS_END);
		flags |= B_SESS_END_;
	    break;
	    case USB_DETECT_MINI_A:
		TRACE_MSG0(TCD, "mini A");
		printk (KERN_INFO"mini A\n");
		flags &= ~(ID_GND);
		flags |= ID_GND;
	    break;
	    case USB_DETECT_MINI_B:
		TRACE_MSG0(TCD, "mini B");
		printk (KERN_INFO"mini B\n");
		flags &= ~(ID_FLOAT);
		flags |= ID_FLOAT;
	    break;
	    case USB_DETECT_NON_USB_ACCESSORY:
		TRACE_MSG0(TCD, "non-usb");
		printk (KERN_INFO"non-usb\n");
	    break;	    
	    case USB_DETECT_FACTORY_MODE:
		TRACE_MSG0(TCD, "factory mode");
		printk (KERN_INFO"factory mode\n");
	    break;	    
	}
#if 0	
	flags = (USB_DETECT_4V4_RISE ? VBUS_VLD : VBUS_VLD_) |
		(USB_DETECT_2V0_RISE ? (B_SESS_VLD | A_SESS_VLD) : (B_SESS_VLD_ | A_SESS_VLD_)) |
		(USB_DETECT_0V8_RISE ? B_SESS_END : B_SESS_END_);
#endif	
	printk(KERN_INFO"Flags = %d\n", flags);
        otg_event(tcd_instance->otg, flags, TCD, "PMIC EVENT");
	
}


/* ********************************************************************************************* */


void mxc_pmic_mod_init(void)
{

	t_pmic_version t = pmic_get_version();

	TRACE_MSG0(TCD, "2. Transceiver setup");
	
	rc = pmic_convity_open(&rr_handle, USB);
	if (rc != PMIC_SUCCESS)
	{
	    TRACE_MSG1 (TCD, "Error in connecting to tranciever %d", rc);
	    return;
	}
	rc = pmic_convity_usb_set_xcvr(rr_handle, USB_SINGLE_ENDED_UNIDIR_RX);
        if (rc != PMIC_SUCCESS)
        {
            TRACE_MSG1 (TCD, "Error in tranciever mode %d", rc);
            return;
        }
	printk (KERN_INFO"%s: RoadRunner version %d\n", __FUNCTION__, t);

	rc = pmic_convity_set_callback(rr_handle, pmic_detect_event, 
	    USB_DETECT_4V4_RISE | USB_DETECT_4V4_FALL |
	    USB_DETECT_2V0_RISE | USB_DETECT_2V0_FALL |
	    USB_DETECT_0V8_RISE | USB_DETECT_0V8_FALL |
	    USB_DETECT_MINI_A | USB_DETECT_MINI_B);
	if (rc != PMIC_SUCCESS)
	{
	    TRACE_MSG1 (TCD, "Error in handling events %d", rc);
	    return;
	}

	TRACE_MSG0(TCD, "Setup the work item");
	PREPARE_WORK_ITEM(pmic_work_bh, &pmic_bh, NULL);
	PREPARE_WORK_ITEM(pmic_otg_wq, &pmic_otg_event_bh, NULL);


	
		       
}


