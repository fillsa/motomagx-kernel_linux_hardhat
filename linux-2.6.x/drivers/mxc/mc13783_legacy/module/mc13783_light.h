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
 * @defgroup MC13783_LIGHT mc13783 Light Driver
 * @ingroup MC13783DRVRS
 */

/*!
 * @file mc13783_light.h
 * @brief This is the header of mc13783 Light driver.
 *
 * @ingroup MC13783_LIGHT
 */

/*
 * Includes
 */
#include <asm/ioctl.h>

#ifndef         __MC13783_LIGHT_H__
#define         __MC13783_LIGHT_H__

#define         MC13783_LIGHT_INIT                       _IOWR('L',1, int)
#define         MC13783_LIGHT_CHECK                      _IOWR('L',2, int)
#define         MC13783_LIGHT_CHECK_LED                  _IOWR('L',3, int)
#define         MC13783_LIGHT_CHECK_BACKLIGHT            _IOWR('L',4, int)
#define         MC13783_LIGHT_CHECK_FUN_LIGHT            _IOWR('L',5, int)

#ifdef __ALL_TRACES__
#define TRACEMSG_LIGHT(fmt,args...)  printk(fmt,##args)
#else				/* __ALL_TRACES__ */
#define TRACEMSG_LIGHT(fmt,args...)
#endif				/* __ALL_TRACES__ */

#define __WAIT_TEST

/*
 * LIGHT mc13783 API
 */

/*!
 * This enumeration represents all types of backlight.
 */
typedef enum {
	/*!
	 * Main backlight
	 */
	BACKLIGHT_MAIN,
	/*!
	 * Auxiliary backlight
	 */
	BACKLIGHT_AUX,
	/*!
	 * Key Pad Backlight
	 */
	BACKLIGHT_KEYPAD,
} t_backlight;

/*!
 * This enumeration represents types of led.
 */
typedef enum {
	/*!
	 * Red led
	 */
	LED_RED,
	/*!
	 * Green led
	 */
	LED_GREEN,
	/*!
	 * Blue led
	 */
	LED_BLUE,
} t_led;

/*!
 * This enumeration defines ramp type.
 */
typedef enum {
	/*!
	 * No ramp
	 */
	RAMP_NO,
	/*!
	 * Ramp Up
	 */
	RAMP_UP,
	/*!
	 * Ramp down
	 */
	RAMP_DOWN,
	/*!
	 * Ramp up and down
	 */
	RAMP_UP_DOWN,
} t_ramp;

/*!
 * This enumeration defines Tri-color type.
 */
typedef enum {
	/*!
	 * Tri-Color 1
	 */
	TRI_COLOR_1,
	/*!
	 * Tri-Color 2
	 */
	TRI_COLOR_2,
	/*!
	 * Tri-Color 3
	 */
	TRI_COLOR_3,
} t_tri_color;

/*!
 * This enumeration defines banks type.
 */
typedef enum {
	/*!
	 * Bank 1
	 */
	BANK_1,
	/*!
	 * Bank 2
	 */
	BANK_2,
	/*!
	 * Bank 3
	 */
	BANK_3,
} t_bank;

/*!
 * This enumeration defines all type of current level.
 */
typedef enum {
	/*!
	 * This level is 0 mA for backlight and 12 mA for Led
	 * (6 mA if half current is enabled)
	 */
	LEVEL_0,
	/*!
	 * This level is 3 mA for main and aux backlight, 12 mA for Key pad.
	 * 18 mA for led current.
	 */
	LEVEL_1,
	/*!
	 * This level is 6 mA for main and aux backlight, 24 mA for Key pad.
	 * 30 mA for led current.
	 */
	LEVEL_2,
	/*!
	 * This level is 9 mA for main and aux backlight, 36 mA for Key pad.
	 * 42 mA for led current.
	 */
	LEVEL_3,
	/*!
	 * This level is 12 mA for main and aux backlight, 48 mA for Key pad.
	 * Not used for led.
	 */
	LEVEL_4,
	/*!
	 * This level is 15 mA for main and aux backlight, 60 mA for Key pad.
	 * Not used for led.
	 */
	LEVEL_5,
	/*!
	 * This level is 18 mA for main and aux backlight, 72 mA for Key pad.
	 * Not used for led.
	 */
	LEVEL_6,
	/*!
	 * This level is 21 mA for main and aux backlight, 84 mA for Key pad.
	 * Not used for led.
	 */
	LEVEL_7,
} t_current;

/*!
 * This enumeration of Fun Light Pattern.
 */
typedef enum {
	/*!
	 * Blended ramps slow
	 */
	BLENDED_RAMPS_SLOW,
	/*!
	 * Blended ramps fast
	 */
	BLENDED_RAMPS_FAST,
	/*!
	 * Saw ramps slow
	 */
	SAW_RAMPS_SLOW,
	/*!
	 * Saw ramps fast
	 */
	SAW_RAMPS_FAST,
	/*!
	 * Blended inverse ramps slow
	 */
	BLENDED_INVERSE_RAMPS_SLOW,
	/*!
	 * Blended inverse ramps fast
	 */
	BLENDED_INVERSE_RAMPS_FAST,
	/*!
	 * Chasing Light RGB Slow
	 */
	CHASING_LIGHT_RGB_SLOW,
	/*!
	 * Chasing Light RGB fast
	 */
	CHASING_LIGHT_RGB_FAST,
	/*!
	 * Chasing Light BGR Slow
	 */
	CHASING_LIGHT_BGR_SLOW,
	/*!
	 * Chasing Light BGR fast
	 */
	CHASING_LIGHT_BGR_FAST,
} t_fun_pattern;

/*!
 * This function sets the master bias for backlight and led.
 * Enable or disable the LEDEN bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_bl_and_led_bais(bool en_dis);

/*!
 * This function gets the master bias for backlight and led.
 * return value of the LEDEN bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_bl_and_led_bais(bool * en_dis);

/*!
 * This function sets the master Analog Edge Slowing for led.
 * Enable or disable the SLEWLIMTC bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_led_master_egde_slowing(bool en_dis);

/*!
 * This function gets the master Analog Edge Slowing for led.
 * return the SLEWLIMTC bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_led_master_egde_slowing(bool * en_dis);

/*!
 * This function sets the master Analog Edge Slowing for backlight.
 * Enable or disable the SLEWLIMBL bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_bl_master_egde_slowing(bool en_dis);

/*!
 * This function gets the master Analog Edge Slowing for backlight.
 * return the SLEWLIMBL bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_bl_master_egde_slowing(bool * en_dis);

/*!
 * This function configure backlight
 *
 * @param    backlight_type Define type of backlight (main, aux..)
 * @param    value          Define value of current  level
 * @param    duty_cycle     Define value of duty cycle on time ratio x/15
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_bl_set_conf(t_backlight backlight_type, t_current value,
			      int duty_cycle);

/*!
 * This function return configure of one backlight
 *
 * @param    *backlight_type Define type of backlight (main, aux..)
 * @param    *value          Define value of current  level
 * @param    *duty_cycle     Define value of duty cycle on time ratio x/15
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_bl_get_conf(t_backlight backlight_type, t_current * value,
			      int *duty_cycle);

/*!
 * This function config ramp and Triode of backlight
 *
 * @param    backlight_type Define type of backlight (main, aux..)
 * @param    triode         enable or disable triode mode
 * @param    ramp_type      Define type of ramp (up, down...)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_bl_ramp_triode(t_backlight backlight_type,
				     bool triode, t_ramp ramp_type);

/*!
 * This function return ramp and triode config of backlight
 *
 * @param    backlight_type Define type of backlight (main, aux..)
 * @param    triode         enable or disable triode mode
 * @param    ramp_type      Define type of ramp (up, down...)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_bl_ramp_triode(t_backlight backlight_type,
				     bool * triode, t_ramp * ramp_type);

/*!
 * This function configure period for backlight
 *
 * @param    period Define the period for backlight
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_bl_period(int period);

/*!
 * This function return period configuration of backlight
 *
 * @param    period        return period value
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_bl_period(int *period);

/*!
 * This function sets Fun Light Pattern and Tree-color bank for fun light pattern
 *
 * @param    fun_pattern Define the fun light pattern
 * @param    bank1       enable or disable Tree-color bank 1 activation for Fun Light pattern
 * @param    bank2       enable or disable Tree-color bank 2 activation for Fun Light pattern
 * @param    bank3       enable or disable Tree-color bank 3 activation for Fun Light pattern
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_conf_fun_light_pattern(t_fun_pattern fun_pattern,
					     bool bank1, bool bank2,
					     bool bank3);

/*!
 * This function gets Fun Light Pattern and Tree-color bank for fun light pattern
 *
 * @param    fun_pattern Define the fun light pattern
 * @param    bank1       return value of Tree-color bank 1 activation for Fun Light pattern
 * @param    bank2       return value of Tree-color bank 2 activation for Fun Light pattern
 * @param    bank3       return value of Tree-color bank 3 activation for Fun Light pattern
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_conf_fun_light_pattern(t_fun_pattern * fun_pattern,
					     bool * bank1, bool * bank2,
					     bool * bank3);

/*!
 * This function configure led
 *
 * @param    led        Define type of led (red, green or blue)
 * @param    bank       Define type of bank
 * @param    value      Define value of current  level
 * @param    duty_cycle Define value of duty cycle on time ratio x/31
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_led_set_conf(t_led led, t_bank bank, t_current value,
			       int duty_cycle);

/*!
 * This function gets config of one led
 *
 * @param    led        Define type of led (red, green or blue)
 * @param    bank       Define type of bank
 * @param    value      Define value of current  level
 * @param    duty_cycle Define value of duty cycle on time ratio x/31
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_led_get_conf(t_led led, t_bank bank, t_current * value,
			       int *duty_cycle);

/*!
 * This function sets period and Triode Mode of bank 1
 *
 * @param    bank       Define type of bank
 * @param    period     Define value of period
 * @param    triode_mode enable or disable triode mode
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_period_and_triode(t_bank bank, int period,
					bool triode_mode);

/*!
 * This function gets period and Triode Mode of bank 1
 *
 * @param    bank       Define type of bank
 * @param    period     Define value of period
 * @param    triode_mode enable or disable triode mode
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_period_and_triode(t_bank bank, int *period,
					bool * triode_mode);

/*!
 * This function configure ramp for led
 *
 * @param    led       Define type of led (red, green or blue)
 * @param    tri_color Define type of tri_color
 * @param    ramp      Define type of ramp
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_led_set_ramp(t_led led, t_tri_color tri_color, t_ramp ramp);

/*!
 * This function return config of led ramp
 *
 * @param    led       Define type of led (red, green or blue)
 * @param    tri_color Define type of tri_color
 * @param    ramp      Define type of ramp
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_led_get_ramp(t_led led, t_tri_color tri_color, t_ramp * ramp);

/*!
 * This function sets the half current mode.
 * Enable or disable the TC1HALF bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_half_current_mode(bool en_dis);

/*!
 * This function gets the half current mode.
 * return the TC1HALF bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_half_current_mode(bool * en_dis);

/*!
 * This function enables the boost mode.
 * Only on mc13783 2.0 or higher
 *
 * @param       en_dis   Enable or disable the boost mode
 *
 * @return      This function returns 0 if successful.
 */

int mc13783_light_set_boost_mode(bool en_dis);

/*!
 * This function gets the boost mode.
 * Only on mc13783 2.0 or higher
 *
 * @param       en_dis   Enable or disable the boost mode
 *
 * @return      This function returns 0 if successful.
 */
int mc13783_light_get_boost_mode(bool * en_dis);

/*!
 * This function sets boost mode configuration
 * Only on mc13783 2.0 or higher
 *
 * @param    abms      Define adaptive boost mode selection
 * @param    abr       Define adaptive boost reference
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_config_boost_mode(unsigned int abms, unsigned int abr);

/*!
 * This function gets boost mode configuration
 * Only on mc13783 2.0 or higher
 *
 * @param    abms      Define adaptive boost mode selection
 * @param    abr       Define adaptive boost reference
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_gets_boost_mode(unsigned int *abms, unsigned int *abr);

/*!
 * This function initialize Light registers of mc13783 with 0.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_init_reg(void);

/*!
 * This function is used to tell if mc13783 Light driver has been correctly loaded.
 *
 * @return       This function returns 1 if Light was successfully loaded
 * 		 0 otherwise.
 */
int mc13783_light_loaded(void);

#endif				/* __MC13783_LIGHT_H__ */
