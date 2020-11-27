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
 * @defgroup MC13783_BATTERY mc13783 Battery Driver
 * @ingroup MC13783DRVRS
 */

/*!
 * @file mc13783_battery.h
 * @brief This is the header of mc13783 Battery driver.
 *
 * @ingroup MC13783_BATTERY
 */

#ifndef         __MC13783_BATTERY_H__
#define         __MC13783_BATTERY_H__

#include <asm/ioctl.h>

#define         MC13783_BATTERY_INIT                       _IOWR('B', 0, void*)
#define         MC13783_BATTERY_CHECK                      _IOWR('B', 1, void*)

/*!
 * These macros represent output voltages available
 * on mc13783 IC.
 */
#define OUT_VOLT_405			0x0

/* The two following macros have identical values. This is to
 * cope with differences between mc13783 2.x and 3.x
 */
#define OUT_VOLT_4375			0x1
#define OUT_VOLT_410			0x1

#define OUT_VOLT_415			0x2
#define OUT_VOLT_420			0x3
#define OUT_VOLT_425			0x4
#define OUT_VOLT_430			0x5
#define OUT_VOLT_440			0x6
#define OUT_VOLT_450			0x7

/*!
 * This enumeration represents all regulator current
 */
typedef enum {
	/*!
	 * charge current 0 mA
	 */
	CURRENT_0 = 0,
	/*!
	 * charge current 90 mA
	 */
	CURRENT_90,
	/*!
	 * charge current 180 mA
	 */
	CURRENT_180,
	/*!
	 * charge current 272 mA
	 */
	CURRENT_272,
	/*!
	 * charge current 363 mA
	 */
	CURRENT_363,
	/*!
	 * charge current 454 mA
	 */
	CURRENT_454,
	/*!
	 * charge current 545 mA
	 */
	CURRENT_545,
	/*!
	 * charge current 636 mA
	 */
	CURRENT_636,
	/*!
	 * charge current 727 mA
	 */
	CURRENT_727,
	/*!
	 * charge current 818 mA
	 */
	CURRENT_818,
	/*!
	 * charge current 909 mA
	 */
	CURRENT_909,
	/*!
	 * charge current 1 A
	 */
	CURRENT_1000,
	/*!
	 * charge current 1090 mA
	 */
	CURRENT_1090,
	/*!
	 * charge current 1181 mA
	 */
	CURRENT_1181,
	/*!
	 * charge current 1636 mA
	 */
	CURRENT_1636,
	/*!
	 * charge current max
	 */
	CURRENT_FULL,
} t_dac_current;

/*!
 * This enumeration represents all trickle current
 */
typedef enum {
	/*!
	 * Trickle charge current 0 mA
	 */
	TRICKLE_CURRENT_0 = 0,
	/*!
	 * Trickle charge current 12 mA
	 */
	TRICKLE_CURRENT_12,
	/*!
	 * Trickle charge current 24 mA
	 */
	TRICKLE_CURRENT_24,
	/*!
	 * Trickle charge current 36 mA
	 */
	TRICKLE_CURRENT_36,
	/*!
	 * Trickle charge current 48 mA
	 */
	TRICKLE_CURRENT_48,
	/*!
	 * Trickle charge current 60 mA
	 */
	TRICKLE_CURRENT_60,
	/*!
	 * Trickle charge current 72 mA
	 */
	TRICKLE_CURRENT_72,
	/*!
	 * Trickle charge current 84 mA
	 */
	TRICKLE_CURRENT_84,
} t_trickle_current;

/*!
 * This enumeration of all types of output controls
 */
typedef enum {
	/*!
	 * controlled hardware
	 */
	CONTROL_HARDWARE = 0,
	/*!
	 * BPFET is driven low, BATTFET is driven high
	 */
	CONTROL_BPFET_LOW,
	/*!
	 * BPFET is driven high, BATTFET is driven low
	 */
	CONTROL_BPFET_HIGH,
} t_control;

/*!
 * This enumeration define all battery interrupt
 */
typedef enum {
	/*!
	 * Charge detection interrupt
	 */
	BAT_IT_CHG_DET,
	/*!
	 * Charge over voltage detection it
	 */
	BAT_IT_CHG_OVERVOLT,
	/*!
	 * Charge path reverse current it
	 */
	BAT_IT_CHG_REVERSE,
	/*!
	 * Charge path short circuitin revers supply mode it
	 */
	BAT_IT_CHG_SHORT_CIRCUIT,
	/*!
	 * Charger has switched its mode (CC to CV or CV to CC)
	 */
	BAT_IT_CCCV,
	/*!
	 * Charge current has dropped below its threshold
	 */
	BAT_IT_BELOW_THRESHOLD,
} t_bat_int;

/*
 * BATTERY mc13783 API
 */

/*!
 * This function sets output voltage of charge regulator.
 *
 * @param        out_voltage  	output voltage
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_out_voltage(int out_voltage);

/*!
 * This function gets output voltage of charge regulator.
 *
 * @param        out_voltage  	output voltage
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_out_voltage(int *out_voltage);

/*!
 * This function sets the current of the main charger DAC.
 *
 * @param        current_val  	current of main DAC.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_dac_current(t_dac_current current_val);

/*!
 * This function gets the current of the main charger DAC.
 *
 * @param        current_val  	current of main DAC.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_dac_current(t_dac_current * current_val);

/*!
 * This function sets the current of the trickle charger.
 *
 * @param        current_val  	current of the trickle charger.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_trickle_current(t_trickle_current current_val);

/*!
 * This function gets the current of the trickle charger.
 *
 * @param        current_val  	current of the trickle charger.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_trickle_current(t_trickle_current * current_val);

/*!
 * This function sets the output controls.
 * It sets the FETOVRD and FETCTRL bits of mc13783
 *
 * @param        control  	type of control.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_out_control(t_control control);

/*!
 * This function gets the output controls.
 * It gets the FETOVRD and FETCTRL bits of mc13783
 *
 * @param        control  	type of control.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_out_control(t_control * control);

/*!
 * This function sets reverse mode.
 *
 * @param        mode  	if true, reverse mode is enable.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_reverse(bool mode);

/*!
 * This function gets reverse mode.
 *
 * @param        mode  	if true, reverse mode is enable.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_reverse(bool * mode);

/*!
 * This function sets over voltage threshold.
 *
 * @param        threshold  	value of over voltage threshold.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_threshold(int threshold);

/*!
 * This function gets over voltage threshold.
 *
 * @param        threshold  	value of over voltage threshold.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_threshold(int *threshold);

/*!
 * This function sets unregulated charge.
 *
 * @param        unregul  	if true, the regulator charge is disabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_unregulator(bool unregul);

/*!
 * This function gets unregulated charge state.
 *
 * @param        unregul  	if true, the regulator charge is disabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_unregulator(bool * unregul);

/*!
 * This function enables/disables charge led.
 *
 * @param        led  	Enable/disable led for charge.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_charge_led(bool led);

/*!
 * This function returns state of charge led.
 *
 * @param        led  	state of charge led.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_charge_led(bool * led);

/*!
 * This function enables/disables a 5K pull down at VBUS.
 *
 * @param        en_dis  	Enable/disable pull down at VBUS.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_5k_pull(bool en_dis);

/*!
 * This function returns 5K pull down at VBUS configuration.
 *
 * @param        en_dis  	Enable/disable pull down at VBUS.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_5k_pull(bool * en_dis);

/*!
 * This function is used to subscribe on battery event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_event_sub(t_bat_int event, void *callback);

/*!
 * This function is used to un subscribe on battery event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_event_unsub(t_bat_int event, void *callback);

/*!
 * This function is used to tell if mc13783 Battery driver has been correctly loaded.
 *
 * @return       This function returns 1 if Battery was successfully loaded
 * 		 0 otherwise.
 */
int mc13783_battery_loaded(void);

#endif				/* __MC13783_BATTERY_H__ */
