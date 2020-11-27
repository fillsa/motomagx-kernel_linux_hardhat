/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

/*!
 * @file mc13783_connectivity.c
 * @brief This is the main file of mc13783 Connectivity driver.
 *
 * @ingroup MC13783_CONNECTIVITY
 */

#include <linux/wait.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "../core/mc13783_external.h"
#include "../core/mc13783_config.h"

#include "mc13783_connectivity.h"
#include "mc13783_connectivity_defs.h"

#define MC13783_LOAD_ERROR_MSG		\
"mc13783 card was not correctly detected. Stop loading mc13783 Connectivity driver\n"

/*
 * Global variables
 */

#ifdef __TEST_CODE_ENABLE__
static void callback_convity_it_usb_detect(void);
static void callback_convity_it(void);
#endif				/* __TEST_CODE_ENABLE__ */

static DECLARE_WAIT_QUEUE_HEAD(queue_usb_det);
static int mc13783_connectivity_major;
static int mc13783_convity_detected = 0;

/*
 * Audio mc13783 API
 */

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(mc13783_convity_set_speed_mode);
EXPORT_SYMBOL(mc13783_convity_get_speed_mode);
EXPORT_SYMBOL(mc13783_convity_set_state);
EXPORT_SYMBOL(mc13783_convity_get_state);
EXPORT_SYMBOL(mc13783_convity_set_pull_down_switch);
EXPORT_SYMBOL(mc13783_convity_get_pull_down_switch);
EXPORT_SYMBOL(mc13783_convity_set_vbus);
EXPORT_SYMBOL(mc13783_convity_get_vbus);
EXPORT_SYMBOL(mc13783_convity_set_vbus_reg);
EXPORT_SYMBOL(mc13783_convity_get_vbus_reg);
EXPORT_SYMBOL(mc13783_convity_set_dlp_timer);
EXPORT_SYMBOL(mc13783_convity_get_dlp_timer);
EXPORT_SYMBOL(mc13783_convity_set_udp_auto_connect);
EXPORT_SYMBOL(mc13783_convity_get_udp_auto_connect);
EXPORT_SYMBOL(mc13783_convity_set_usb_transceiver);
EXPORT_SYMBOL(mc13783_convity_get_usb_transceiver);
EXPORT_SYMBOL(mc13783_convity_set_var_disconnect);
EXPORT_SYMBOL(mc13783_convity_get_var_disconnect);
EXPORT_SYMBOL(mc13783_convity_set_interface_mode);
EXPORT_SYMBOL(mc13783_convity_get_interface_mode);
EXPORT_SYMBOL(mc13783_convity_set_single_ended_mode);
EXPORT_SYMBOL(mc13783_convity_get_single_ended_mode);
EXPORT_SYMBOL(mc13783_convity_set_directional_mode);
EXPORT_SYMBOL(mc13783_convity_get_directional_mode);
EXPORT_SYMBOL(mc13783_convity_pulse);
EXPORT_SYMBOL(mc13783_convity_set_udm_pull);
EXPORT_SYMBOL(mc13783_convity_get_udm_pull);
EXPORT_SYMBOL(mc13783_convity_set_udp_pull);
EXPORT_SYMBOL(mc13783_convity_get_udp_pull);
EXPORT_SYMBOL(mc13783_convity_set_vusb_reg);
EXPORT_SYMBOL(mc13783_convity_get_vusb_reg);
EXPORT_SYMBOL(mc13783_convity_set_vusb_voltage);
EXPORT_SYMBOL(mc13783_convity_get_vusb_voltage);
EXPORT_SYMBOL(mc13783_convity_set_output);
EXPORT_SYMBOL(mc13783_convity_get_output);
EXPORT_SYMBOL(mc13783_convity_event_sub);
EXPORT_SYMBOL(mc13783_convity_event_unsub);
EXPORT_SYMBOL(mc13783_convity_swaps_txrx_en);
EXPORT_SYMBOL(mc13783_convity_get_swaps_txrx);
EXPORT_SYMBOL(mc13783_convity_tx_tristates_en);
EXPORT_SYMBOL(mc13783_convity_get_tx_tristates);
EXPORT_SYMBOL(mc13783_convity_loaded);

/*
 * Connectivity mc13783 API
 */

#ifdef __TEST_CODE_ENABLE__
/*!
 * This is the callback function called on USB detect event.
 */
static void callback_convity_it_usb_detect(void)
{
	TRACEMSG_CONNECTIVITY(_K_I
			      ("*** USB connect detected TEST mc13783 ***"));
	wake_up(&queue_usb_det);

}

/*!
 * This is the callback function is used in test.
 */
static void callback_convity_it(void)
{
	TRACEMSG_CONNECTIVITY(_K_I("*** Connectivity IT TEST mc13783 ***"));
}

#endif				/* __TEST_CODE_ENABLE__ */

/*!
 * This function enables low speed mode for USB.
 *
 * @param        low_speed  	if true, the low speed mode is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_speed_mode(bool low_speed)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_FSENB, low_speed);
}

/*!
 * This function returns the USB speed mode selected.
 *
 * @param        low_speed      return value of speed mode if false,
 *                              the full speed mode is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_speed_mode(bool * low_speed)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_FSENB, low_speed);
}

/*!
 * This function enables/disables USB suspend mode.
 *
 * @param        suspend  	if true, the suspend mode is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_state(bool suspend)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_USBSUSPEND, suspend);
}

/*!
 * This function returns the USB suspend mode selected.
 *
 * @param        suspend        return value of suspend mode if true,
 *                              the suspend mode is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_state(bool * suspend)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_USBSUSPEND, suspend);
}

/*!
 * This function configures pull-down switched in/out.
 *
 * @param         pdswtch 	define type of pull down switcher.
 * @param         in            if true, switch is in else is out.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_pull_down_switch(t_pd_type pdswtch, bool in)
{
	switch (pdswtch) {
	case PD_PU:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_USBPU,
						in));
		break;
	case PD_UPD_15:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_UDPPD,
						in));
		break;
	case PD_UDM_15:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_UDMPD,
						in));
		break;
	case PD_UDP_150:
		CHECK_ERROR(mc13783_set_reg_bit
			    (PRIO_CONN, REG_USB, BIT_DP150KPU, in));
		break;
	case PD_UID:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_IDPD,
						in));
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return ERROR_NONE;
}

/*!
 * This function returns state of pull-down switcher in/out.
 *
 * @param         pdswtch 	define type of pull down switcher.
 * @param         in            return value, if true, switch is 'in' else is
 *                              'out'.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_pull_down_switch(t_pd_type pdswtch, bool * in)
{
	switch (pdswtch) {
	case PD_PU:
		CHECK_ERROR(mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_USBPU,
						in));
		break;
	case PD_UPD_15:
		CHECK_ERROR(mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_UDPPD,
						in));
		break;
	case PD_UDM_15:
		CHECK_ERROR(mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_UDMPD,
						in));
		break;
	case PD_UDP_150:
		CHECK_ERROR(mc13783_get_reg_bit
			    (PRIO_CONN, REG_USB, BIT_DP150KPU, in));
		break;
	case PD_UID:
		CHECK_ERROR(mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_IDPD,
						in));
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return ERROR_NONE;
}

/*!
 * This function enables/disables VBUS.
 * This API configures the VBUSPDENB bits of USB register
 * @param        vbus 	if true, vbus pull-down NMOS switch is off.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_vbus(t_vusb vbus)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_VBUSPDENB,
				   (bool) vbus);
}

/*!
 * This function returns state of VBUS.
 *
 * @param        vbus 	if true, vbus pull-down NMOS switch is off.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_vbus(t_vusb * vbus)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_VBUSPDENB,
				   (bool *) vbus);
}

/*!
 * This function configures the vbus regulator current limit.
 *
 * @param        current_limit 	value of current limit.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_vbus_reg(t_current_limit current_limit)
{
	return mc13783_set_reg_value(PRIO_CONN, REG_USB, BITS_CURRENT_LIMIT,
				     (int)current_limit, LONG_CURRENT_LIMIT);
}

/*!
 * This function gets the vbus regulator current limit.
 *
 * @param        current_limit 	return value of current limit.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_vbus_reg(t_current_limit * current_limit)
{
	return mc13783_get_reg_value(PRIO_CONN, REG_USB, BITS_CURRENT_LIMIT,
				     (int *)current_limit, LONG_CURRENT_LIMIT);
}

/*!
 * This function enables/disables DLP Timer.
 * This API configures the DLPSRP bits of USB register
 *
 * @param        dlp 	if true, DLP Timer is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_dlp_timer(bool dlp)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_DLPSRP, dlp);
}

/*!
 * This function returns state of DLP Timer.
 *
 * @param        dlp 	if true, DLP Timer is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_dlp_timer(bool * dlp)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_DLPSRP, dlp);
}

/*!
 * This function configures auto connect of UDP.
 * This API configures the SE0CONN bits of USB register
 *
 * @param        udp_ac 	if true, UDP is automatically connected when SE0
 *                              is detected..
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_udp_auto_connect(bool udp_ac)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_SE0CONN, udp_ac);
}

/*!
 * This function returns UDP auto connect state.
 *
 * @param        udp_ac 	if true, UDP is automatically connected when SE0
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_udp_auto_connect(bool * udp_ac)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_SE0CONN, udp_ac);
}

/*!
 * This function configures USB transceiver.
 * This API configures the USBXCVREN bits of USB register
 *
 * @param        usb_trans 	if true, USB transceiver is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_usb_transceiver(bool usb_trans)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_USBXCVREN,
				   usb_trans);
}

/*!
 * This function gets state of USB transceiver.
 *
 * @param        usb_trans 	if true, USB transceiver is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_usb_transceiver(bool * usb_trans)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_USBXCVREN,
				   usb_trans);
}

/*!
 * This function connects or disconnects 1k5 pull-up and UDP/UMD pull-down.
 * This API configures the PULLOVER bits of USB register
 *
 * @param        connect 	if true, variable 1k5 and UDP/UDM pull-down
 *                              are disconnected.
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_var_disconnect(bool connect)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_PULLOVER, connect);
}

/*!
 * This function returns 1k5 pull-up and UDP/UMD pull-down.
 *
 * @param        connect 	if true, variable 1k5 and UDP/UDM pull-down
 *                              are disconnected.
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_var_disconnect(bool * connect)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_PULLOVER, connect);
}

/*!
 * This function configures connectivity interface mode.
 *
 * @param        mode 	define mode of interface.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_interface_mode(t_interface_mode mode)
{
	switch (mode) {
	case IM_USB:
		CHECK_ERROR(mc13783_set_reg_value(PRIO_CONN, REG_USB,
						  BITS_INTERFACE_MODE,
						  (int)MODE_USB,
						  LONG_INTERFACE_MODE));
		break;
	case IM_RS232_1:
		CHECK_ERROR(mc13783_set_reg_value(PRIO_CONN, REG_USB,
						  BITS_INTERFACE_MODE,
						  (int)MODE_RS232_1,
						  LONG_INTERFACE_MODE));
		break;
	case IM_RS232_2:
		CHECK_ERROR(mc13783_set_reg_value(PRIO_CONN, REG_USB,
						  BITS_INTERFACE_MODE,
						  (int)MODE_RS232_2,
						  LONG_INTERFACE_MODE));
		break;
	case IM_MONO_AUDIO:
		CHECK_ERROR(mc13783_set_reg_value(PRIO_CONN, REG_USB,
						  BITS_INTERFACE_MODE,
						  (int)MODE_AUDIO_MONO,
						  LONG_INTERFACE_MODE));
		break;
	case IM_STEREO_AUDIO:
		CHECK_ERROR(mc13783_set_reg_value(PRIO_CONN, REG_USB,
						  BITS_INTERFACE_MODE,
						  (int)MODE_AUDIO_ST,
						  LONG_INTERFACE_MODE));
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return ERROR_NONE;
}

/*!
 * This function gets connectivity interface mode.
 *
 * @param        mode 	return mode of interface.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_interface_mode(t_interface_mode * mode)
{
	int mode_val = 0;

	CHECK_ERROR(mc13783_get_reg_value
		    (PRIO_CONN, REG_USB, BITS_INTERFACE_MODE, (int *)&mode_val,
		     LONG_INTERFACE_MODE));

	switch (mode_val) {
	case MODE_USB:
		*mode = (t_interface_mode) IM_USB;
		break;
	case MODE_RS232_1:
		*mode = (t_interface_mode) IM_RS232_1;
		break;
	case MODE_RS232_2:
		*mode = (t_interface_mode) IM_RS232_2;
		break;
	case MODE_AUDIO_MONO:
		*mode = (t_interface_mode) IM_MONO_AUDIO;
		break;
	case MODE_AUDIO_ST:
		*mode = (t_interface_mode) IM_STEREO_AUDIO;
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return ERROR_NONE;
}

/*!
 * This function configures single/differential USB mode.
 *
 * This API configures the DATSE0 bits of USB register
 * @param        sgle 	if true, single ended is selected,
 *                      if false, differential is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_single_ended_mode(bool sgle)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_DATSE0, sgle);
}

/*!
 * This function gets single/differential USB mode.
 *
 * @param        sgle 	if true, single ended is selected,
 *                      if false, differential is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_single_ended_mode(bool * sgle)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_DATSE0, sgle);
}

/*!
 * This function configures directional USB mode.
 * This API configures the BIDIR bits of USB register
 * @param        bidir 	if true, unidirectional is selected,
 *                      if false, bidirectional is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_directional_mode(bool bidir)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_BIDIR, bidir);
}

/*!
 * This function gets directional USB mode.
 *
 * @param        bidir 	if true, unidirectional is selected,
 *                      if false, bidirectional is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_directional_mode(bool * bidir)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_BIDIR, bidir);
}

/*!
 * This function launches a pulse on UID or UDM line.
 *
 * @param        line 	define line type.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_pulse(t_pulse_line line)
{
	switch (line) {
	case PL_UID:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_IDPULSE,
						true));
		break;
	case PL_UDM:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_DMPULSE,
						true));
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return ERROR_NONE;
}

/*!
 * This function configures UID pull type.
 * This API configures the IDPUCNTRL bits of USB register
 *
 * @param        pull 	if true, pulled high by 4uA.
 *                      if false, pulled high through 220k resistor
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_udm_pull(bool pull)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_IDPUCNTRL, pull);
}

/*!
 * This function returns UID pull type.
 *
 * @param        pull 	if true, pulled high by 4uA.
 *                      if false, pulled high through 220k resistor
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_udm_pull(bool * pull)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_IDPUCNTRL, pull);
}

/*!
 * This function configures UDP pull up type.
 * This API configures the USBCNTRL bits of USB register
 *
 * @param        pull 	if true, controlled by USBEN pin
 *                      if false, controlled by SPI bits.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_udp_pull(bool pull)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_USB, BIT_USBCNTRL, pull);
}

/*!
 * This function returns UDP pull up type.
 *
 * @param        pull 	if true, controlled by USBEN pin
 *                      if false, controlled by SPI bits.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_udp_pull(bool * pull)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_USB, BIT_USBCNTRL, pull);
}

/*!
 * This function configures VUSB regulator.
 *
 * @param        reg_value      define value of regulator.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_vusb_reg(t_vusb_regulator reg_value)
{
	return mc13783_set_reg_value(PRIO_CONN, REG_CHARGE_USB_SPARE,
				     BITS_VUSB_IN, (int)reg_value,
				     LONG_VUSB_IN);
}

/*!
 * This function returns VUSB regulator.
 *
 * @param        reg_value      return value of regulator.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_vusb_reg(t_vusb_regulator * reg_value)
{
	return mc13783_get_reg_value(PRIO_CONN, REG_CHARGE_USB_SPARE,
				     BITS_VUSB_IN, (int *)reg_value,
				     LONG_VUSB_IN);
}

/*!
 * This function controls the VUSB output voltage.
 * This API configures the VUSB bits of USB register
 *
 * @param        out 	if true, output voltage is set to 3.3V
 *                      if false, output voltage is set to 2.775V.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_vusb_voltage(t_vusb out)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE, BIT_VUSB,
				   (bool) out);
}

/*!
 * This function reads the VUSB output voltage.
 *
 * @param        out 	if true, output voltage is set to 3.3V
 *                      if false, output voltage is set to 2.775V.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_vusb_voltage(t_vusb * out)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE, BIT_VUSB,
				   (bool *) out);
}

/*!
 * This function enables/disables VUSB and VBUS output.
 * This API configures the VUSBEN and VBUSEN bits of USB register
 *
 * @param        out_type true, for VUSB
 *                        false, for VBUS
 * @param        out 	if true, output is enabled
 *                      if false, output is disabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_output(bool out_type, bool out)
{
	if (out_type == 0) {
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE,
						BIT_VUSBEN, out));
	} else {
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE,
						BIT_VBUSEN, out));
	}
	return ERROR_NONE;
}

/*!
 * This function reads VUSB and VBUS output.
 *
 * @param        out_type true, for VUSB
 *                        false, for VBUS
 * @param        out 	if true, output is enabled
 *                      if false, output is disabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_output(bool out_type, bool * out)
{
	if (out_type == 0) {
		CHECK_ERROR(mc13783_get_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE,
						BIT_VUSBEN, out));
	} else {
		CHECK_ERROR(mc13783_get_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE,
						BIT_VBUSEN, out));
	}
	return ERROR_NONE;
}

/*!
 * This function configures Swaps TX and RX in RS232 mode.
 * It sets the RSPOL bits of USB register
 * Only on mc13783 2.0 or higher
 *
 * @param        en 	if true, swaps TX and RX in RS232 mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_swaps_txrx_en(bool en)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE, BIT_RSPOL,
				   en);
}

/*!
 * This function gets Swaps TX and RX configuration mode.
 * It gets the RSPOL bits of USB register
 * Only on mc13783 2.0 or higher
 *
 * @param        en 	if true, swaps TX and RX in RS232 mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_swaps_txrx(bool * en)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE, BIT_RSPOL,
				   en);
}

/*!
 * This function configures TX tristates mode.
 * It sets the RSTRI bits of USB register
 * Only on mc13783 2.0 or higher
 *
 * @param        en 	if true, TX is in tristates mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_tx_tristates_en(bool en)
{
	return mc13783_set_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE, BIT_RSTRI,
				   en);
}

/*!
 * This function gets TX tristates mode in RS232.
 * It gets the RSTRI bits of USB register
 * Only on mc13783 2.0 or higher
 *
 * @param        en 	if true, TS is in tristates mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_tx_tristates(bool * en)
{
	return mc13783_get_reg_bit(PRIO_CONN, REG_CHARGE_USB_SPARE, BIT_RSTRI,
				   en);
}

/*!
 * This function is used to un/subscribe on connectivity event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 * @param        sub      	define if Un/subscribe event.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_event(t_convity_int event, void *callback, bool sub)
{
	type_event_notification con_event;

	mc13783_event_init(&con_event);
	con_event.callback = callback;
	switch (event) {
	case IT_CONVITY_USBI:
		con_event.event = EVENT_USBI;
		break;
	case IT_CONVITY_IDI:
		con_event.event = EVENT_IDI;
		break;
	case IT_CONVITY_SE1I:
		con_event.event = EVENT_SE1I;
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	if (sub == true) {
		CHECK_ERROR(mc13783_event_subscribe(con_event));
	} else {
		CHECK_ERROR(mc13783_event_unsubscribe(con_event));
	}
	return ERROR_NONE;
}

/*!
 * This function is used to subscribe on connectivity event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_event_sub(t_convity_int event, void *callback)
{
	return mc13783_convity_event(event, callback, true);
}

/*!
 * This function is used to un subscribe on connectivity event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_event_unsub(t_convity_int event, void *callback)
{
	return mc13783_convity_event(event, callback, false);
}

#ifdef __TEST_CODE_ENABLE__

/*!
 * This function is used to check value of test.
 *
 * @param        val_ret  	return value of the test.
 * @param        waiting_val  	the correct value.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_check_val(int val_ret, int waiting_val)
{
	if (val_ret != waiting_val) {
		TRACEMSG_CONNECTIVITY(_K_I("ERROR in Connectivity API"));
		return ERROR_BAD_INPUT;
	}
	return ERROR_NONE;
}

#endif				/* __TEST_CODE_ENABLE__ */

/*!
 * This function implements IOCTL controls on a mc13783 connectivity device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int mc13783_connectivity_ioctl(struct inode *inode, struct file *file,
				      unsigned int cmd, unsigned long arg)
{
	int i = 0, ret_val = 0;
#ifdef __TEST_CODE_ENABLE__
	DEFINE_WAIT(wait);
#endif

	if (_IOC_TYPE(cmd) != 'C')
		return -ENOTTY;

	switch (cmd) {
	case MC13783_CONNECTIVITY_INIT:
		TRACEMSG_CONNECTIVITY(_K_D("Init Connectivity"));
		CHECK_ERROR(0);
		break;

#ifdef __TEST_CODE_ENABLE__

	case MC13783_CONNECTIVITY_CHECK_API:
		CHECK_ERROR(mc13783_convity_set_speed_mode(true));
		CHECK_ERROR(mc13783_convity_get_speed_mode(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_speed_mode(false));

		CHECK_ERROR(mc13783_convity_set_state(true));
		CHECK_ERROR(mc13783_convity_get_state(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_state(false));

		for (i = PD_PU; i <= PD_UID; i++) {
			CHECK_ERROR(mc13783_convity_set_pull_down_switch
				    (i, true));
			CHECK_ERROR(mc13783_convity_get_pull_down_switch
				    (i, &ret_val));
			CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
			CHECK_ERROR(mc13783_convity_set_pull_down_switch(i,
									 false));
			CHECK_ERROR(mc13783_convity_get_pull_down_switch(i,
									 &ret_val));
			CHECK_ERROR(mc13783_convity_check_val(ret_val, false));
		}

		CHECK_ERROR(mc13783_convity_set_vbus(VUSB_3_3));
		CHECK_ERROR(mc13783_convity_get_vbus((t_vusb *) & ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, VUSB_3_3));
		CHECK_ERROR(mc13783_convity_set_vbus(VUSB_2_775));
		for (i = CL_200; i <= CL_910; i++) {
			CHECK_ERROR(mc13783_convity_set_vbus_reg(i));
			CHECK_ERROR(mc13783_convity_get_vbus_reg
				    ((t_current_limit *) & ret_val));
			CHECK_ERROR(mc13783_convity_check_val(ret_val, i));
		}
		CHECK_ERROR(mc13783_convity_set_vbus_reg(CL_200));
		CHECK_ERROR(mc13783_convity_set_dlp_timer(true));
		CHECK_ERROR(mc13783_convity_get_dlp_timer(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_dlp_timer(false));
		CHECK_ERROR(mc13783_convity_set_udp_auto_connect(true));
		CHECK_ERROR(mc13783_convity_get_udp_auto_connect(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_udp_auto_connect(false));
		CHECK_ERROR(mc13783_convity_set_usb_transceiver(true));
		CHECK_ERROR(mc13783_convity_get_usb_transceiver(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_usb_transceiver(false));
		CHECK_ERROR(mc13783_convity_set_var_disconnect(true));
		CHECK_ERROR(mc13783_convity_get_var_disconnect(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_var_disconnect(false));
		for (i = IM_USB; i <= IM_STEREO_AUDIO; i++) {
			CHECK_ERROR(mc13783_convity_set_interface_mode(i));
			CHECK_ERROR(mc13783_convity_get_interface_mode
				    ((t_interface_mode *) & ret_val));
			CHECK_ERROR(mc13783_convity_check_val(ret_val, i));
		}
		CHECK_ERROR(mc13783_convity_set_interface_mode(IM_USB));

		CHECK_ERROR(mc13783_convity_set_single_ended_mode(true));
		CHECK_ERROR(mc13783_convity_get_single_ended_mode(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_single_ended_mode(false));

		CHECK_ERROR(mc13783_convity_set_directional_mode(true));
		CHECK_ERROR(mc13783_convity_get_directional_mode(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_directional_mode(false));

		CHECK_ERROR(mc13783_convity_set_udm_pull(true));
		CHECK_ERROR(mc13783_convity_get_udm_pull(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_udm_pull(false));

		CHECK_ERROR(mc13783_convity_set_udp_pull(true));
		CHECK_ERROR(mc13783_convity_get_udp_pull(&ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_udp_pull(false));

		for (i = VUSB_BOOST; i <= VUSB_BP; i++) {
			CHECK_ERROR(mc13783_convity_set_vusb_reg
				    ((t_vusb_regulator) i));
			CHECK_ERROR(mc13783_convity_get_vusb_reg
				    ((t_vusb_regulator *) & ret_val));
			CHECK_ERROR(mc13783_convity_check_val(ret_val, i));
		}
		CHECK_ERROR(mc13783_convity_set_vusb_reg(IM_USB));

		CHECK_ERROR(mc13783_convity_set_vusb_voltage(true));
		CHECK_ERROR(mc13783_convity_get_vusb_voltage
			    ((t_vusb *) & ret_val));
		CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
		CHECK_ERROR(mc13783_convity_set_vusb_voltage(false));

		for (i = 0; i <= 1; i++) {
			CHECK_ERROR(mc13783_convity_set_output(i, true));
			CHECK_ERROR(mc13783_convity_get_output(i, &ret_val));
			CHECK_ERROR(mc13783_convity_check_val(ret_val, true));
			CHECK_ERROR(mc13783_convity_set_output(i, false));
			CHECK_ERROR(mc13783_convity_get_output(i, &ret_val));
			CHECK_ERROR(mc13783_convity_check_val(ret_val, false));
		}

		for (i = IT_CONVITY_USBI; i <= IT_CONVITY_SE1I; i++) {
			CHECK_ERROR(mc13783_convity_event_sub(i,
							      callback_convity_it));
			CHECK_ERROR(mc13783_convity_event_unsub(i,
								callback_convity_it));
		}

		break;
	case MC13783_CONNECTIVITY_CHECK_USB_DETECT:
		TRACEMSG_CONNECTIVITY(_K_I("Test USB detect"));
		CHECK_ERROR(mc13783_convity_event_sub(IT_CONVITY_USBI,
						      callback_convity_it_usb_detect));
		TRACEMSG_CONNECTIVITY(_K_I("Wait for USB Plugging..."));
		prepare_to_wait(&queue_usb_det, &wait, TASK_UNINTERRUPTIBLE);
		schedule_timeout(500 * HZ);
		finish_wait(&queue_usb_det, &wait);
		CHECK_ERROR(mc13783_convity_event_unsub(IT_CONVITY_USBI,
							callback_convity_it_usb_detect));
		TRACEMSG_CONNECTIVITY(_K_D("Check USB detect done"));
		break;

#endif				/* __TEST_CODE_ENABLE__ */

	default:
		TRACEMSG_CONNECTIVITY(_K_D("%d unsupported ioctl command"),
				      (int)cmd);
		return -EINVAL;
	}
	return ERROR_NONE;
}

/*!
 * This function implements the open method on a mc13783 connectivity device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_connectivity_open(struct inode *inode, struct file *file)
{
	return ERROR_NONE;
}

/*!
 * This function implements the release method on a mc13783 connectivity device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_connectivity_release(struct inode *inode, struct file *file)
{
	return ERROR_NONE;
}

/*!
 * This function is called to put the connectivity in a low power state.
 * There is no need for power handlers for the connectivity device.
 * The connectivity cannot be suspended.
 *
 * @param   dev   the device structure used to give information on which
 *                connectivity device (0 through 3 channels) to suspend
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mc13783_connectivity_suspend(struct device *dev, u32 state,
					u32 level)
{
	switch (level) {
	case SUSPEND_DISABLE:
		break;
	case SUSPEND_SAVE_STATE:
		/* TBD */
		break;
	case SUSPEND_POWER_DOWN:
		/* Turn off clock */
		break;
	}
	return 0;
}

/*!
 * This function is called to resume the connectivity from a low power state.
 *
 * @param   dev   the device structure used to give information on which
 *                connectivity device (0 through 3 channels) to suspend
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mc13783_connectivity_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_POWER_ON:
		/* Turn on clock */
		break;
	case RESUME_RESTORE_STATE:
		/* TBD */
		break;
	case RESUME_ENABLE:
		break;
	}
	return 0;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mc13783_connectivity_driver_ldm = {
	.name = "mc13783_connectivity",
	.bus = &platform_bus_type,
	.suspend = mc13783_connectivity_suspend,
	.resume = mc13783_connectivity_resume,
};

/*!
 * This is platform device structure for adding connectivity
 */
static struct platform_device mc13783_connectivity_ldm = {
	.name = "mc13783_connectivity",
	.id = 1,
};

static struct file_operations mc13783_connectivity_fops = {
	.owner = THIS_MODULE,
	.ioctl = mc13783_connectivity_ioctl,
	.open = mc13783_connectivity_open,
	.release = mc13783_connectivity_release,
};

int mc13783_convity_loaded(void)
{
	return mc13783_convity_detected;
}

/*
 * Init and Exit
 */

static int __init mc13783_connectivity_init(void)
{
	int ret = 0;

	if (mc13783_core_loaded() == 0) {
		printk(KERN_INFO MC13783_LOAD_ERROR_MSG);
		return -1;
	}

	mc13783_connectivity_major = register_chrdev(0, "mc13783_connectivity",
						     &mc13783_connectivity_fops);

	if (mc13783_connectivity_major < 0) {
		TRACEMSG_CONNECTIVITY(_K_D
				      ("Unable to get a major for mc13783_connectivity"));
		return -1;
	}

	devfs_mk_cdev(MKDEV(mc13783_connectivity_major, 0), S_IFCHR | S_IRUGO
		      | S_IWUSR, "mc13783_connectivity");

	ret = driver_register(&mc13783_connectivity_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&mc13783_connectivity_ldm);
		if (ret != 0) {
			driver_unregister(&mc13783_connectivity_driver_ldm);
		} else {
			mc13783_convity_detected = 1;
			printk(KERN_INFO "mc13783 Connectivity loaded\n");
		}
	}

	/* set some reasonable mc13783 default values */
	if (mc13783_convity_detected == 1) {
		mc13783_convity_set_output(1, 0);	//disable VBUS
		mc13783_convity_set_output(0, 0);	//disable VBUS

		mc13783_convity_set_speed_mode(0);	//set high speed

		/* variable 1.5K pull-up switch off */
		mc13783_convity_set_pull_down_switch(PD_PU, 0);
		/* variable 1.5K pull-up switch off */
		mc13783_convity_set_pull_down_switch(PD_PU, 0);
		/* DP pull down switch is on */
		mc13783_convity_set_pull_down_switch(PD_UPD_15, 1);
		/* DP pull down switch is on */
		mc13783_convity_set_pull_down_switch(PD_UDM_15, 1);

		mc13783_convity_set_interface_mode(IM_RS232_1);
		mc13783_convity_set_var_disconnect(1);

		mc13783_convity_set_udp_auto_connect(0);
		mc13783_convity_set_udp_pull(0);
	}

	return ret;
}

static void __exit mc13783_connectivity_exit(void)
{
	devfs_remove("mc13783_connectivity");

	driver_unregister(&mc13783_connectivity_driver_ldm);
	platform_device_unregister(&mc13783_connectivity_ldm);
	unregister_chrdev(mc13783_connectivity_major, "mc13783_connectivity");

	printk(KERN_INFO "mc13783_connectivity : successfully unloaded");
}

/*
 * Module entry points
 */

module_init(mc13783_connectivity_init);
module_exit(mc13783_connectivity_exit);

MODULE_DESCRIPTION("mc13783_connectivity driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
