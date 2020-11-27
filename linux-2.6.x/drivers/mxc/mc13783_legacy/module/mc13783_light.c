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
 * @file mc13783_light.c
 * @brief This is the main file of mc13783 Light and Backlight driver.
 *
 * @ingroup MC13783_LIGHT
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "../core/mc13783_external.h"
#include "../core/mc13783_config.h"

#include "mc13783_light.h"
#include "mc13783_light_defs.h"

#define NB_LIGHT_REG      		6
#define MC13783_LOAD_ERROR_MSG		\
"mc13783 card was not correctly detected. Stop loading mc13783 Light driver\n"

static int mc13783_light_major;
static int mc13783_light_detected = 0;

/*!
 * Number of users waiting in suspendq
 */
static int swait = 0;

/*!
 * To indicate whether any of the light devices are suspending
 */
static int suspend_flag = 0;

/*!
 * The suspendq is used to block application calls
 */
static wait_queue_head_t suspendq;

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(mc13783_light_set_bl_and_led_bais);
EXPORT_SYMBOL(mc13783_light_get_bl_and_led_bais);
EXPORT_SYMBOL(mc13783_light_set_led_master_egde_slowing);
EXPORT_SYMBOL(mc13783_light_get_led_master_egde_slowing);
EXPORT_SYMBOL(mc13783_light_set_bl_master_egde_slowing);
EXPORT_SYMBOL(mc13783_light_get_bl_master_egde_slowing);
EXPORT_SYMBOL(mc13783_light_bl_set_conf);
EXPORT_SYMBOL(mc13783_light_bl_get_conf);
EXPORT_SYMBOL(mc13783_light_set_bl_ramp_triode);
EXPORT_SYMBOL(mc13783_light_get_bl_ramp_triode);
EXPORT_SYMBOL(mc13783_light_set_bl_period);
EXPORT_SYMBOL(mc13783_light_get_bl_period);
EXPORT_SYMBOL(mc13783_light_set_conf_fun_light_pattern);
EXPORT_SYMBOL(mc13783_light_get_conf_fun_light_pattern);
EXPORT_SYMBOL(mc13783_light_led_set_conf);
EXPORT_SYMBOL(mc13783_light_led_get_conf);
EXPORT_SYMBOL(mc13783_light_set_period_and_triode);
EXPORT_SYMBOL(mc13783_light_get_period_and_triode);
EXPORT_SYMBOL(mc13783_light_led_set_ramp);
EXPORT_SYMBOL(mc13783_light_led_get_ramp);
EXPORT_SYMBOL(mc13783_light_set_half_current_mode);
EXPORT_SYMBOL(mc13783_light_get_half_current_mode);
EXPORT_SYMBOL(mc13783_light_set_boost_mode);
EXPORT_SYMBOL(mc13783_light_config_boost_mode);
EXPORT_SYMBOL(mc13783_light_gets_boost_mode);

EXPORT_SYMBOL(mc13783_light_init_reg);
EXPORT_SYMBOL(mc13783_light_loaded);

/*!
 * This is the suspend of power management for the mc13783 light API.
 * It supports SAVE and POWER_DOWN state.
 *
 * @param        dev            the device
 * @param        state          the state
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_light_suspend(struct device *dev, u32 state, u32 level)
{
	switch (level) {
	case SUSPEND_DISABLE:
		suspend_flag = 1;
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		/* switch off all leds and backlights */
		CHECK_ERROR(mc13783_light_init_reg());
		break;
	}

	return 0;
};

/*!
 * This is the resume of power management for the mc13783 light API.
 * It supports RESTORE state.
 *
 * @param        dev            the device
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_light_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_POWER_ON:
		/* nothing for mc13783 light */
		/* we can add blinking led in the future */
		break;
	case RESUME_RESTORE_STATE:
		break;
	case RESUME_ENABLE:
		suspend_flag = 0;
		while (swait > 0) {
			swait--;
			wake_up_interruptible(&suspendq);
		}
		break;
	}
	return 0;
};

static struct device_driver mc13783_light_driver_ldm = {
	.name = "mc13783_light",
	.bus = &platform_bus_type,
	.suspend = mc13783_light_suspend,
	.resume = mc13783_light_resume,
};

static struct platform_device mc13783_light_ldm = {
	.name = "mc13783_light",
	.id = 1,
};

/*
 * Light and Backlight mc13783 API
 */
/*!
 * This function sets the master bias for backlight and led.
 * Enable or disable the LEDEN bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_bl_and_led_bais(bool en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_LIGHT, LREG_0, BIT_LEDEN, en_dis));
	return ERROR_NONE;
}

/*!
 * This function gets the master bias for backlight and led.
 * return value of the LEDEN bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_bl_and_led_bais(bool * en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_LIGHT, LREG_0, BIT_LEDEN, en_dis));
	return ERROR_NONE;
}

/*!
 * This function sets the master Analog Edge Slowing for led.
 * Enable or disable the SLEWLIMTC bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_led_master_egde_slowing(bool en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_LIGHT, LREG_1, BIT_SLEWLIMTC,
					en_dis));
	return ERROR_NONE;
}

/*!
 * This function gets the master Analog Edge Slowing for led.
 * return the SLEWLIMTC bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_led_master_egde_slowing(bool * en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_LIGHT, LREG_1, BIT_SLEWLIMTC, en_dis));
	return ERROR_NONE;
}

/*!
 * This function sets the master Analog Edge Slowing for backlight.
 * Enable or disable the SLEWLIMBL bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_bl_master_egde_slowing(bool en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_LIGHT, LREG_2, BIT_SLEWLIMBL,
					en_dis));
	return ERROR_NONE;
}

/*!
 * This function gets the master Analog Edge Slowing for backlight.
 * return the SLEWLIMBL bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_bl_master_egde_slowing(bool * en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_LIGHT, LREG_2, BIT_SLEWLIMBL, en_dis));
	return ERROR_NONE;
}

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
			      int duty_cycle)
{
	unsigned int reg_value = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	if (duty_cycle > 15) {
		return ERROR_BAD_INPUT;
	}

	CHECK_ERROR(mc13783_read_reg(PRIO_LIGHT, LREG_2, &reg_value));

	switch (backlight_type) {
	case BACKLIGHT_MAIN:
		reg_value = reg_value & (~MASK_CURRENT_LEVEL);
		reg_value = reg_value | value;
		reg_value = reg_value & (~MASK_DUTY_CYCLE);
		reg_value = reg_value | (duty_cycle << BIT_DUTY_CYCLE);
		break;
	case BACKLIGHT_AUX:
		reg_value = reg_value & (~(MASK_CURRENT_LEVEL << INDEX_AUX));
		reg_value = reg_value | (value << INDEX_AUX);
		reg_value = reg_value & (~(MASK_DUTY_CYCLE << INDEX_AUX));
		reg_value = reg_value | (duty_cycle << (BIT_DUTY_CYCLE
							+ INDEX_AUX));
		break;
	case BACKLIGHT_KEYPAD:
		reg_value = reg_value & (~(MASK_CURRENT_LEVEL << INDEX_KYD));
		reg_value = reg_value | (value << INDEX_KYD);
		reg_value = reg_value & (~(MASK_DUTY_CYCLE << INDEX_KYD));
		reg_value = reg_value | (duty_cycle << (BIT_DUTY_CYCLE
							+ INDEX_KYD));
		break;
	default:
		return ERROR_BAD_INPUT;
	}
	CHECK_ERROR(mc13783_write_reg(PRIO_LIGHT, LREG_2, &reg_value));
	return ERROR_NONE;
}

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
			      int *duty_cycle)
{
	unsigned int reg_value = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_read_reg(PRIO_LIGHT, LREG_2, &reg_value));

	switch (backlight_type) {
	case BACKLIGHT_MAIN:
		*value = (t_current) (reg_value & (MASK_CURRENT_LEVEL));
		*duty_cycle = (int)((reg_value & (MASK_DUTY_CYCLE))
				    >> BIT_DUTY_CYCLE);

		break;
	case BACKLIGHT_AUX:
		*value = (t_current) ((reg_value & (MASK_CURRENT_LEVEL <<
						    INDEX_AUX)) >> INDEX_AUX);
		*duty_cycle = (int)((reg_value & (MASK_DUTY_CYCLE << INDEX_AUX))
				    >> (BIT_DUTY_CYCLE + INDEX_AUX));
		break;
	case BACKLIGHT_KEYPAD:
		*value = (t_current) ((reg_value & (MASK_CURRENT_LEVEL <<
						    INDEX_KYD)) >> INDEX_KYD);
		*duty_cycle = (int)((reg_value & (MASK_DUTY_CYCLE <<
						  INDEX_KYD)) >> (BIT_DUTY_CYCLE
								  + INDEX_KYD));
		break;
	default:
		return ERROR_BAD_INPUT;
	}

	return ERROR_NONE;
}

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
				     bool triode, t_ramp ramp_type)
{
	unsigned int reg_value = 0;
	unsigned int clear_val = 0;
	unsigned int triode_val = 0;
	unsigned int rampup_val = 0;
	unsigned int rampdown_val = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	CHECK_ERROR(mc13783_read_reg(PRIO_LIGHT, LREG_0, &reg_value));

	switch (backlight_type) {
	case BACKLIGHT_MAIN:
		clear_val = ~(MASK_TRIODE_MAIN_BL | MASK_DOWN_MAIN_BL |
			      MASK_UP_MAIN_BL);
		triode_val = MASK_TRIODE_MAIN_BL;
		rampup_val = MASK_UP_MAIN_BL;
		rampdown_val = MASK_DOWN_MAIN_BL;
		break;
	case BACKLIGHT_AUX:
		clear_val = ~((MASK_TRIODE_MAIN_BL << INDEX_AUXILIARY) |
			      (MASK_DOWN_MAIN_BL << INDEX_AUXILIARY) |
			      (MASK_UP_MAIN_BL << INDEX_AUXILIARY));
		triode_val = (MASK_TRIODE_MAIN_BL << INDEX_AUXILIARY);
		rampup_val = (MASK_UP_MAIN_BL << INDEX_AUXILIARY);
		rampdown_val = (MASK_DOWN_MAIN_BL << INDEX_AUXILIARY);
		break;
	case BACKLIGHT_KEYPAD:
		clear_val = ~((MASK_TRIODE_MAIN_BL << INDEX_KEYPAD) |
			      (MASK_DOWN_MAIN_BL << INDEX_KEYPAD) |
			      (MASK_UP_MAIN_BL << INDEX_KEYPAD));
		triode_val = (MASK_TRIODE_MAIN_BL << INDEX_KEYPAD);
		rampup_val = (MASK_UP_MAIN_BL << INDEX_KEYPAD);
		rampdown_val = (MASK_DOWN_MAIN_BL << INDEX_KEYPAD);

		break;
	default:
		return ERROR_BAD_INPUT;
	}

	reg_value = (reg_value & clear_val);

	if (triode) {
		reg_value = (reg_value | triode_val);
	}

	switch (ramp_type) {
	case RAMP_NO:
		break;
	case RAMP_UP:
		reg_value = (reg_value | rampup_val);
		break;
	case RAMP_DOWN:
		reg_value = (reg_value | rampdown_val);
		break;
	case RAMP_UP_DOWN:
		reg_value = (reg_value | (rampup_val | rampdown_val));
		break;
	default:
		return ERROR_BAD_INPUT;
	}

	CHECK_ERROR(mc13783_write_reg(PRIO_LIGHT, LREG_0, &reg_value));
	return ERROR_NONE;
}

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
				     bool * triode, t_ramp * ramp_type)
{
	unsigned int reg_value = 0;
	unsigned int triode_val = 0;
	unsigned int rampup_val = 0;
	unsigned int rampdown_val = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	CHECK_ERROR(mc13783_read_reg(PRIO_LIGHT, LREG_0, &reg_value));

	triode_val = false;
	rampup_val = false;
	rampdown_val = false;

	switch (backlight_type) {
	case BACKLIGHT_MAIN:
		reg_value = reg_value & (MASK_TRIODE_MAIN_BL |
					 MASK_DOWN_MAIN_BL | MASK_UP_MAIN_BL);
		if (reg_value & MASK_TRIODE_MAIN_BL)
			triode_val = true;
		if (reg_value & MASK_UP_MAIN_BL)
			rampup_val = true;
		if (reg_value & MASK_DOWN_MAIN_BL)
			rampdown_val = true;
		break;
	case BACKLIGHT_AUX:
		reg_value = reg_value & ((MASK_TRIODE_MAIN_BL
					  << INDEX_AUXILIARY) |
					 (MASK_DOWN_MAIN_BL << INDEX_AUXILIARY)
					 | (MASK_UP_MAIN_BL <<
					    INDEX_AUXILIARY));

		if (reg_value & (MASK_TRIODE_MAIN_BL << INDEX_AUXILIARY)) {
			triode_val = true;
		}
		if (reg_value & (MASK_UP_MAIN_BL << INDEX_AUXILIARY)) {
			rampup_val = true;
		}
		if (reg_value & (MASK_DOWN_MAIN_BL << INDEX_AUXILIARY)) {
			rampdown_val = true;
		}
		break;
	case BACKLIGHT_KEYPAD:
		reg_value = reg_value & ((MASK_TRIODE_MAIN_BL << INDEX_KEYPAD) |
					 (MASK_DOWN_MAIN_BL << INDEX_KEYPAD) |
					 (MASK_UP_MAIN_BL << INDEX_KEYPAD));
		if (reg_value & (MASK_TRIODE_MAIN_BL << INDEX_KEYPAD)) {
			triode_val = true;
		}
		if (reg_value & (MASK_UP_MAIN_BL << INDEX_KEYPAD)) {
			rampup_val = true;
		}
		if (reg_value & (MASK_DOWN_MAIN_BL << INDEX_KEYPAD)) {
			rampdown_val = true;
		}
		break;
	default:
		return ERROR_BAD_INPUT;
	}

	if (triode_val) {
		*triode = true;
	} else {
		*triode = false;
	}
	*ramp_type = RAMP_NO;

	if (rampup_val) {
		*ramp_type = RAMP_UP;
	}
	if (rampdown_val) {
		*ramp_type = RAMP_DOWN;
		if (rampup_val) {
			*ramp_type = RAMP_UP_DOWN;
		}

	}
	return ERROR_NONE;
}

/*!
 * This function configure period for backlight
 *
 * @param    period Define the period for backlight
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_bl_period(int period)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if (period > 3) {
		return ERROR_BAD_INPUT;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_LIGHT, LREG_2, BITS_PERIOD_BL,
					  (int)period, 2));
	return ERROR_NONE;
}

/*!
 * This function return period configuration of backlight
 *
 * @param    period        return period value
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_bl_period(int *period)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_get_reg_value(PRIO_LIGHT, LREG_2, BITS_PERIOD_BL,
					  period, 2));
	return ERROR_NONE;

}

/*!
 * This function sets Fun Light Pattern and Tree-color bank for fun light pattern
 *
 * @param    fun_pattern Define the fun light pattern
 * @param    bank1       enable or disable Tree-color bank 1 activation for Fun
 *                       Light pattern
 * @param    bank2       enable or disable Tree-color bank 2 activation for Fun
 *                       Light pattern
 * @param    bank3       enable or disable Tree-color bank 3 activation for Fun
 *                       Light pattern
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_conf_fun_light_pattern(t_fun_pattern fun_pattern,
					     bool bank1, bool bank2, bool bank3)
{
	unsigned int fun_conf = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	fun_conf = ((int)fun_pattern);
	if (bank1) {
		fun_conf |= (MASK_BK1_FL >> BITS_FUN_LIGHT);
	}
	if (bank2) {
		fun_conf |= (MASK_BK2_FL >> BITS_FUN_LIGHT);
	}
	if (bank3) {
		fun_conf |= (MASK_BK3_FL >> BITS_FUN_LIGHT);
	}

	CHECK_ERROR(mc13783_set_reg_value(PRIO_LIGHT, LREG_0, BITS_FUN_LIGHT,
					  fun_conf, 0xF));
	return ERROR_NONE;
}

/*!
 * This function gets Fun Light Pattern and Tree-color bank for fun light pattern
 *
 * @param    fun_pattern Define the fun light pattern
 * @param    bank1       return value of Tree-color bank 1 activation for Fun
 *                       Light pattern
 * @param    bank2       return value of Tree-color bank 2 activation for Fun
 *                       Light pattern
 * @param    bank3       return value of Tree-color bank 3 activation for Fun
 *                       Light pattern
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_conf_fun_light_pattern(t_fun_pattern * fun_pattern,
					     bool * bank1, bool * bank2,
					     bool * bank3)
{
	unsigned int fun_conf = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_get_reg_value(PRIO_LIGHT, LREG_0, BITS_FUN_LIGHT,
					  &fun_conf, 7));

	*fun_pattern = (fun_conf) & (MASK_FUN_LIGHT >> BITS_FUN_LIGHT);
	fun_conf = fun_conf >> 4;
	*bank1 = false;
	*bank2 = false;
	*bank3 = false;
	if (fun_conf & 0x01)
		*bank1 = true;
	if (fun_conf & 0x02)
		*bank2 = true;
	if (fun_conf & 0x04)
		*bank3 = true;
	return ERROR_NONE;
}

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
			       int duty_cycle)
{
	unsigned int reg_conf = 0;
	unsigned int index_cl = 0;
	unsigned int index_dc = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	if (value > LEVEL_3) {
		return ERROR_BAD_INPUT;
	}
	if (duty_cycle > DUTY_CYCLE_MAX) {
		return ERROR_BAD_INPUT;
	}

	switch (bank) {
	case BANK_1:
		reg_conf = LREG_3;
		break;
	case BANK_2:
		reg_conf = LREG_4;
		break;
	case BANK_3:
		reg_conf = LREG_5;
		break;
	default:
		return ERROR_BAD_INPUT;
	}

	switch (led) {
	case LED_RED:
		index_cl = BITS_CL_RED;
		index_dc = BITS_DC_RED;
		break;
	case LED_GREEN:
		index_cl = BITS_CL_RED + LONG_CURRENT_LEVEL;
		index_dc = BITS_DC_RED + LONG_DUTY_CYCLE;
		break;
	case LED_BLUE:
		index_cl = BITS_CL_RED + (2 * LONG_CURRENT_LEVEL);
		index_dc = BITS_DC_RED + (2 * LONG_DUTY_CYCLE);
		break;
	default:
		;
	}

	CHECK_ERROR(mc13783_set_reg_value(PRIO_LIGHT, reg_conf, index_cl, value,
					  LONG_CURRENT_LEVEL));
	CHECK_ERROR(mc13783_set_reg_value(PRIO_LIGHT, reg_conf, index_dc,
					  duty_cycle, LONG_DUTY_CYCLE));

	return ERROR_NONE;
}

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
			       int *duty_cycle)
{
	unsigned int reg_conf = 0;
	unsigned int index_cl = 0;
	unsigned int index_dc = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	switch (bank) {
	case BANK_1:
		reg_conf = LREG_3;
		break;
	case BANK_2:
		reg_conf = LREG_4;
		break;
	case BANK_3:
		reg_conf = LREG_5;
		break;
	default:
		return ERROR_BAD_INPUT;
	}

	switch (led) {
	case LED_RED:
		index_cl = BITS_CL_RED;
		index_dc = BITS_DC_RED;
		break;
	case LED_GREEN:
		index_cl = BITS_CL_RED + LONG_CURRENT_LEVEL;
		index_dc = BITS_DC_RED + LONG_DUTY_CYCLE;
		break;
	case LED_BLUE:
		index_cl = BITS_CL_RED + (2 * LONG_CURRENT_LEVEL);
		index_dc = BITS_DC_RED + (2 * LONG_DUTY_CYCLE);
		break;
	default:
		return ERROR_BAD_INPUT;
	}

	CHECK_ERROR(mc13783_get_reg_value(PRIO_LIGHT, reg_conf, index_cl,
					  (unsigned int *)value,
					  LONG_CURRENT_LEVEL));
	CHECK_ERROR(mc13783_get_reg_value
		    (PRIO_LIGHT, reg_conf, index_dc, (unsigned int *)duty_cycle,
		     LONG_DUTY_CYCLE));

	return ERROR_NONE;
}

/*!
 * This function sets period and Triode Mode of one bank
 *
 * @param    bank       Define type of bank
 * @param    period     Define value of period
 * @param    triode_mode enable or disable triode mode
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_period_and_triode(t_bank bank, int period,
					bool triode_mode)
{
	unsigned int reg_conf = 0;
	unsigned int reg_val = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	if (period > 3) {
		return ERROR_BAD_INPUT;
	}

	switch (bank) {
	case BANK_1:
		reg_conf = LREG_3;
		break;
	case BANK_2:
		reg_conf = LREG_4;
		break;
	case BANK_3:
		reg_conf = LREG_5;
		break;
	default:
		return ERROR_BAD_INPUT;
	}
	reg_val = period | (0x04 * triode_mode);

	CHECK_ERROR(mc13783_set_reg_value
		    (PRIO_LIGHT, reg_conf, BIT_PERIOD, reg_val, 3));

	return ERROR_NONE;
}

/*!
 * This function gets period and Triode Mode of one bank
 *
 * @param    bank       Define type of bank
 * @param    period     Define value of period
 * @param    triode_mode enable or disable triode mode
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_period_and_triode(t_bank bank, int *period,
					bool * triode_mode)
{
	unsigned int reg_conf = 0;
	unsigned int reg_val = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	switch (bank) {
	case BANK_1:
		reg_conf = LREG_3;
		break;
	case BANK_2:
		reg_conf = LREG_4;
		break;
	case BANK_3:
		reg_conf = LREG_5;
		break;
	default:
		return ERROR_BAD_INPUT;
	}

	CHECK_ERROR(mc13783_get_reg_value(PRIO_LIGHT, reg_conf, BIT_PERIOD,
					  &reg_val, 3));

	*period = reg_val & 0x03;
	*triode_mode = (bool) (reg_val >> 2);

	return ERROR_NONE;
}

/*!
 * This function configure ramp for led
 *
 * @param    led       Define type of led (red, green or blue)
 * @param    tri_color Define type of tri_color
 * @param    ramp      Define type of ramp
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_led_set_ramp(t_led led, t_tri_color tri_color, t_ramp ramp)
{
	bool up = false, down = false;
	unsigned int index_up = 0, index_down = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	switch (ramp) {
	case RAMP_NO:
		break;
	case RAMP_UP:
		up = true;
		break;
	case RAMP_DOWN:
		down = true;
		break;
	case RAMP_UP_DOWN:
		up = true;
		down = true;
		break;
	default:
		return ERROR_BAD_INPUT;
	}

	index_up = led + 6 * tri_color;
	index_down = index_up + 3;

	CHECK_ERROR(mc13783_set_reg_bit(PRIO_LIGHT, LREG_1, index_up, false));
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_LIGHT, LREG_1, index_down, false));

	if (up) {
		CHECK_ERROR(mc13783_set_reg_bit
			    (PRIO_LIGHT, LREG_1, index_up, true));
	}

	if (down) {
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_LIGHT, LREG_1, index_down,
						true));
	}

	return ERROR_NONE;
}

/*!
 * This function return config of led ramp
 *
 * @param    led       Define type of led (red, green or blue)
 * @param    tri_color Define type of tri_color
 * @param    ramp      Define type of ramp
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_led_get_ramp(t_led led, t_tri_color tri_color, t_ramp * ramp)
{
	bool up = false, down = false;
	unsigned int index_up = 0, index_down = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	index_up = led + 6 * tri_color;
	index_down = index_up + 3;

	CHECK_ERROR(mc13783_get_reg_bit(PRIO_LIGHT, LREG_1, index_up, &up));
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_LIGHT, LREG_1, index_down, &down));

	*ramp = RAMP_NO;
	if (up) {
		*ramp = RAMP_UP;
	}

	if (down) {
		*ramp = RAMP_DOWN;
		if (up) {
			*ramp = RAMP_UP_DOWN;
		}
	}
	return ERROR_NONE;
}

/*!
 * This function sets the half current mode.
 * Enable or disable the TC1HALF bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_set_half_current_mode(bool en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_LIGHT, LREG_1, BIT_TC1HALF, en_dis));
	return ERROR_NONE;
}

/*!
 * This function gets the half current mode.
 * return the TC1HALF bit of mc13783
 *
 * @param    en_dis   Enable or disable the main bias
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_get_half_current_mode(bool * en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_LIGHT, LREG_1, BIT_TC1HALF, en_dis));
	return ERROR_NONE;
}

/*!
 * This function enables the boost mode.
 * Only on mc13783 2.0 or higher
 *
 * @param       en_dis   Enable or disable the boost mode
 *
 * @return      This function returns 0 if successful.
 */
int mc13783_light_set_boost_mode(bool en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_LIGHT, LREG_0, BIT_BOOSTEN, en_dis));
	return ERROR_NONE;
}

/*!
 * This function gets the boost mode.
 * Only on mc13783 2.0 or higher
 *
 * @param       en_dis   Enable or disable the boost mode
 *
 * @return      This function returns 0 if successful.
 */
int mc13783_light_get_boost_mode(bool * en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_LIGHT, LREG_0, BIT_BOOSTEN, en_dis));
	return ERROR_NONE;
}

/*!
 * This function sets boost mode configuration
 * Only on mc13783 2.0 or higher
 *
 * @param    abms      Define adaptive boost mode selection
 * @param    abr       Define adaptive boost reference
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_config_boost_mode(unsigned int abms, unsigned int abr)
{
	unsigned int conf_boost = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	if (abms > MAX_BOOST_ABMS) {
		return ERROR_BAD_INPUT;
	}

	if (abr > MAX_BOOST_ABR) {
		return ERROR_BAD_INPUT;
	}

	conf_boost = abms | (abr << 3);

	CHECK_ERROR(mc13783_set_reg_value(PRIO_LIGHT, LREG_0, BITS_BOOST,
					  conf_boost, 5));

	return ERROR_NONE;
}

/*!
 * This function gets boost mode configuration
 * Only on mc13783 2.0 or higher
 *
 * @param    abms      Define adaptive boost mode selection
 * @param    abr       Define adaptive boost reference
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_gets_boost_mode(unsigned int *abms, unsigned int *abr)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}

	CHECK_ERROR(mc13783_get_reg_value(PRIO_LIGHT, LREG_0, BITS_BOOST,
					  abms, 3));
	CHECK_ERROR(mc13783_get_reg_value(PRIO_LIGHT, LREG_0, BITS_BOOST + 3,
					  abr, 2));
	return ERROR_NONE;
}

/*!
 * This function initialize Light registers of mc13783 with 0.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_init_reg(void)
{
	CHECK_ERROR(mc13783_write_reg_value(PRIO_LIGHT, LREG_0, 0));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_LIGHT, LREG_1, 0));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_LIGHT, LREG_2, 0));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_LIGHT, LREG_3, 0));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_LIGHT, LREG_4, 0));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_LIGHT, LREG_5, 0));
	return ERROR_NONE;
}

#ifdef __TEST_CODE_ENABLE__

/*!
 * This function tests Backlight management.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_test_backlight(void)
{
	bool conf1 = false;
	bool r_conf1 = false;
	unsigned int r_period = 0, period = 0;
	unsigned int r_dutycycle = 0, dutycycle = 0;
	t_current r_level = 0, level = 0;
	t_ramp ramp = 0, r_ramp = 0;
	t_backlight backlight = 0;

	mc13783_light_init_reg();

	CHECK_ERROR(mc13783_light_set_bl_and_led_bais(true));

	TRACEMSG_LIGHT(_K_I("Enable BL Master Light"));
	CHECK_ERROR(mc13783_light_set_bl_master_egde_slowing(true));
	TRACEMSG_LIGHT(_K_D("Get LED Master Light config"));
	CHECK_ERROR(mc13783_light_get_bl_master_egde_slowing(&r_conf1));
	if (!r_conf1) {
		TRACEMSG_LIGHT(_K_I("ERROR in BL MASTER config"));
		return -EINVAL;
	}

	for (backlight = BACKLIGHT_MAIN; backlight <= BACKLIGHT_KEYPAD;
	     backlight++) {
		for (level = LEVEL_1; level < LEVEL_7; level += 3) {
			for (dutycycle = 0; dutycycle < 16; dutycycle += 5) {
				period = dutycycle >> 2;
				conf1 = !conf1;
				TRACEMSG_LIGHT(_K_I("Set Backlight %d, level %d"
						    " Duty Cycle %d, period %d"),
					       backlight, level, dutycycle,
					       period);
				CHECK_ERROR(mc13783_light_bl_set_conf
					    (backlight, level, dutycycle));
				CHECK_ERROR(mc13783_light_set_bl_period
					    (period));
				r_level = 0;
				r_dutycycle = 0;
				r_period = 0;
#ifdef __WAIT_TEST
				msleep(1000);
#endif
				CHECK_ERROR(mc13783_light_bl_get_conf(backlight,
								      &r_level,
								      &r_dutycycle));
				CHECK_ERROR(mc13783_light_get_bl_period
					    (&r_period));
				if ((r_level != level)
				    || (r_dutycycle != dutycycle)
				    || (r_period != period)) {
					TRACEMSG_LIGHT(_K_I
						       ("ERROR in Backlight config"));
					return -EINVAL;
				}
			}
		}
	}

	for (backlight = BACKLIGHT_MAIN; backlight <= BACKLIGHT_KEYPAD;
	     backlight++) {
		for (ramp = RAMP_NO; ramp < RAMP_UP_DOWN; ramp++) {
			conf1 = !conf1;
			TRACEMSG_LIGHT(_K_I("Set BL %d ramp %d triode %d"),
				       backlight, ramp, conf1);
			CHECK_ERROR(mc13783_light_set_bl_ramp_triode
				    (backlight, conf1, ramp));
#ifdef __WAIT_TEST
			msleep(3000);
#endif
			CHECK_ERROR(mc13783_light_get_bl_ramp_triode(backlight,
								     &r_conf1,
								     &r_ramp));
			if ((ramp != r_ramp) || (conf1 != r_conf1)) {
				TRACEMSG_LIGHT(_K_I("ERROR in BL ramps"));
				return -EINVAL;
			}
		}
	}

	TRACEMSG_LIGHT(_K_I("Disable BL Master Light"));
	CHECK_ERROR(mc13783_light_set_bl_master_egde_slowing(false));
	TRACEMSG_LIGHT(_K_D("Get LED Master Light config"));
	CHECK_ERROR(mc13783_light_get_bl_master_egde_slowing(&r_conf1));
	if (r_conf1) {
		TRACEMSG_LIGHT(_K_I("ERROR in BL MASTER config"));
		return -EINVAL;
	}

	CHECK_ERROR(mc13783_light_set_bl_and_led_bais(false));

	return ERROR_NONE;

}

/*!
 * This function tests Fun Light management.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_test_fun_light(void)
{
	bool conf1 = false, conf2 = false, conf3 = false;
	bool r_conf1 = false, r_conf2 = false, r_conf3 = false;
	t_fun_pattern fun_pattern = BLENDED_RAMPS_SLOW,
	    r_fun_pattern = BLENDED_RAMPS_SLOW;

	mc13783_light_init_reg();

	CHECK_ERROR(mc13783_light_set_bl_and_led_bais(true));
	for (fun_pattern = BLENDED_RAMPS_SLOW;
	     fun_pattern <= CHASING_LIGHT_BGR_FAST; fun_pattern++) {
		conf1 = false;
		conf2 = false;
		conf3 = false;
		if (0x01 & fun_pattern)
			conf1 = true;
		if (0x02 & fun_pattern)
			conf2 = true;
		if (0x04 & fun_pattern)
			conf3 = true;
		TRACEMSG_LIGHT(_K_I("Set Fun Pattern %d, B1 %d, B2 %d, B3 %d"),
			       fun_pattern, conf1, conf2, conf3);
		CHECK_ERROR(mc13783_light_set_conf_fun_light_pattern
			    (fun_pattern, conf1, conf2, conf3));
		r_fun_pattern = 0;
		r_conf1 = false;
		r_conf2 = false;
		r_conf3 = false;
#ifdef __WAIT_TEST
		msleep(10000);
#endif
		CHECK_ERROR(mc13783_light_get_conf_fun_light_pattern
			    (&r_fun_pattern, &r_conf1, &r_conf2, &r_conf3));
		if ((fun_pattern != r_fun_pattern) || (r_conf1 != conf1)
		    || (r_conf2 != conf2) || (r_conf3 != conf3)) {
			TRACEMSG_LIGHT(_K_I("ERROR Fun Pattern"));
			return -EINVAL;
		}
	}

	CHECK_ERROR(mc13783_light_set_bl_and_led_bais(false));

	return ERROR_NONE;

}

/*!
 * This function tests led management
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_light_test_led(void)
{
	bool conf1 = false;
	bool r_conf1 = false;
	unsigned int r_period = 0, period = 0;
	unsigned int r_dutycycle = 0, dutycycle = 0;
	t_led led = 0;
	t_bank bank = 0;
	t_current r_level = 0, level = 0;
	t_tri_color tri_color = 0;
	t_ramp ramp = 0, r_ramp = 0;

	mc13783_light_init_reg();

	TRACEMSG_LIGHT(_K_I("Enable Master Light"));
	CHECK_ERROR(mc13783_light_set_bl_and_led_bais(true));
	TRACEMSG_LIGHT(_K_D("Get Master Light config"));
	CHECK_ERROR(mc13783_light_get_bl_and_led_bais(&r_conf1));
	if (!r_conf1) {
		TRACEMSG_LIGHT(_K_I("ERROR in MASTER config"));
		return -EINVAL;
	}
	TRACEMSG_LIGHT(_K_I("Enable LED Master Light"));
	CHECK_ERROR(mc13783_light_set_led_master_egde_slowing(true));
	TRACEMSG_LIGHT(_K_D("Get LED Master Light config"));
	CHECK_ERROR(mc13783_light_get_led_master_egde_slowing(&r_conf1));
	if (!r_conf1) {
		TRACEMSG_LIGHT(_K_I("ERROR in LED MASTER config"));
		return -EINVAL;
	}

	for (bank = BANK_1; bank <= BANK_3; bank++) {
		for (led = LED_RED; led <= LED_BLUE; led++) {
			for (level = LEVEL_1; level < LEVEL_4; level++) {
				for (dutycycle = 0; dutycycle < 32;
				     dutycycle += 5) {
					period = 3 - level;
					conf1 = !conf1;
					TRACEMSG_LIGHT(_K_I("Set LED %d, bank "
							    "%d, level %d Duty Cycle %d, period %d,"
							    "Triode %d"),
						       led, bank, level,
						       dutycycle, period,
						       conf1);
					CHECK_ERROR(mc13783_light_led_set_conf
						    (led, bank, level,
						     dutycycle));
					CHECK_ERROR
					    (mc13783_light_set_period_and_triode
					     (bank, period, conf1));
#ifdef __WAIT_TEST
					msleep(200);
#endif
					r_level = 0;
					r_dutycycle = 0;
					r_period = 0;
					r_conf1 = false;
					CHECK_ERROR(mc13783_light_led_get_conf
						    (led, bank, &r_level,
						     &r_dutycycle));
					CHECK_ERROR
					    (mc13783_light_get_period_and_triode
					     (bank, &r_period, &r_conf1));
					if ((r_level != level)
					    || (r_dutycycle != dutycycle)
					    || (r_period != period)
					    || (conf1 != conf1)) {
						TRACEMSG_LIGHT(_K_I
							       ("ERROR in led config"));
						return -EINVAL;
					}
					CHECK_ERROR(mc13783_light_led_set_conf
						    (led, bank, 0, 0));
					CHECK_ERROR
					    (mc13783_light_set_period_and_triode
					     (bank, 0, 0));
				}
			}
		}
	}

	TRACEMSG_LIGHT(_K_I("TEST LED RAMP"));
	for (led = LED_RED; led <= LED_BLUE; led++) {
		for (tri_color = TRI_COLOR_1; tri_color <= TRI_COLOR_3;
		     tri_color++) {
			for (ramp = RAMP_NO; ramp < RAMP_UP_DOWN; ramp++) {
				TRACEMSG_LIGHT(_K_I("Set Led %d ramp %d tri "
						    "color %d"), led, ramp,
					       tri_color);
				CHECK_ERROR(mc13783_light_led_set_ramp
					    (led, tri_color, ramp));
				CHECK_ERROR(mc13783_light_set_period_and_triode
					    (BANK_1, 3, true));
				CHECK_ERROR(mc13783_light_led_set_conf
					    (led, BANK_1, LEVEL_2, 15));
#ifdef __WAIT_TEST
				msleep(3000);
#endif
				CHECK_ERROR(mc13783_light_led_get_ramp(led,
								       tri_color,
								       &r_ramp));
				if (ramp != r_ramp) {
					TRACEMSG_LIGHT(_K_I
						       ("ERROR in led ramps"));
					return -EINVAL;
				}
				CHECK_ERROR(mc13783_light_led_set_ramp(led,
								       tri_color,
								       RAMP_NO));
			}
		}
	}

	TRACEMSG_LIGHT(_K_I("Enable Half current mode"));
	CHECK_ERROR(mc13783_light_set_half_current_mode(false));
	TRACEMSG_LIGHT(_K_D("Get half current"));
	CHECK_ERROR(mc13783_light_get_half_current_mode(&r_conf1));
	if (r_conf1) {
		TRACEMSG_LIGHT(_K_I("ERROR in half config"));
		return -EINVAL;
	}
	TRACEMSG_LIGHT(_K_I("Disable Half current mode"));
	CHECK_ERROR(mc13783_light_set_half_current_mode(false));
	TRACEMSG_LIGHT(_K_D("Get half current"));
	CHECK_ERROR(mc13783_light_get_half_current_mode(&r_conf1));
	if (r_conf1) {
		TRACEMSG_LIGHT(_K_I("ERROR in half config"));
		return -EINVAL;
	}

	TRACEMSG_LIGHT(_K_I("Disable Master Light"));
	CHECK_ERROR(mc13783_light_set_bl_and_led_bais(false));
	TRACEMSG_LIGHT(_K_D("Get Master Light config"));
	CHECK_ERROR(mc13783_light_get_bl_and_led_bais(&r_conf1));
	if (r_conf1) {
		TRACEMSG_LIGHT(_K_I("ERROR in MASTER config"));
		return -EINVAL;
	}
	TRACEMSG_LIGHT(_K_I("disable LED Master Light"));
	CHECK_ERROR(mc13783_light_set_led_master_egde_slowing(false));
	TRACEMSG_LIGHT(_K_D("Get LED Master Light config"));
	CHECK_ERROR(mc13783_light_get_led_master_egde_slowing(&r_conf1));
	if (r_conf1) {
		TRACEMSG_LIGHT(_K_I("ERROR in LED MASTER config"));
		return -EINVAL;
	}
	return ERROR_NONE;
}

#endif				/* __TEST_CODE_ENABLE__ */

/*!
 * This function implements IOCTL controls on a mc13783 light device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int mc13783_light_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg)
{
	if (_IOC_TYPE(cmd) != 'L')
		return -ENOTTY;

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}

	switch (cmd) {
	case MC13783_LIGHT_INIT:
		TRACEMSG_LIGHT(_K_D("Init Light"));
		/* initialization all Light register with 0 */
		CHECK_ERROR(mc13783_light_init_reg());
		break;
#ifdef __TEST_CODE_ENABLE__
	case MC13783_LIGHT_CHECK:
		TRACEMSG_LIGHT(_K_D("Check All Light"));
		CHECK_ERROR(mc13783_light_test_led());
		CHECK_ERROR(mc13783_light_test_backlight());
		CHECK_ERROR(mc13783_light_test_fun_light());
		CHECK_ERROR(mc13783_light_init_reg());
		break;
	case MC13783_LIGHT_CHECK_FUN_LIGHT:
		/* Fun Light test */
		TRACEMSG_LIGHT(_K_D("CHECK FUN LIGHT"));
		CHECK_ERROR(mc13783_light_test_fun_light());
		CHECK_ERROR(mc13783_light_init_reg());
		break;
	case MC13783_LIGHT_CHECK_LED:
		TRACEMSG_LIGHT(_K_D("CHECK LED"));
		CHECK_ERROR(mc13783_light_test_led());
		CHECK_ERROR(mc13783_light_init_reg());
		break;
	case MC13783_LIGHT_CHECK_BACKLIGHT:
		TRACEMSG_LIGHT(_K_D("CHECK BACKLIGHT"));
		CHECK_ERROR(mc13783_light_test_backlight());
		CHECK_ERROR(mc13783_light_init_reg());
		break;
#endif				/* __TEST_CODE_ENABLE__ */
	default:
		TRACEMSG_LIGHT(_K_D("%d unsupported ioctl command"), (int)cmd);
		return -EINVAL;
	}
	return ERROR_NONE;
}

/*!
 * This function implements the open method on a mc13783 light device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_light_open(struct inode *inode, struct file *file)
{
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	return ERROR_NONE;
}

/*!
 * This function implements the release method on a mc13783 light device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_light_release(struct inode *inode, struct file *file)
{
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	return ERROR_NONE;
}

static struct file_operations mc13783_light_fops = {
	.owner = THIS_MODULE,
	.ioctl = mc13783_light_ioctl,
	.open = mc13783_light_open,
	.release = mc13783_light_release,
};

int mc13783_light_loaded(void)
{
	return mc13783_light_detected;
}

/*
 * Initialization and Exit
 */

static int __init mc13783_light_init(void)
{
	int ret = ERROR_NONE;

	if (mc13783_core_loaded() == 0) {
		printk(KERN_INFO MC13783_LOAD_ERROR_MSG);
		return -1;
	}

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}

	mc13783_light_major = register_chrdev(0, "mc13783_light",
					      &mc13783_light_fops);
	if (mc13783_light_major < 0) {
		TRACEMSG_LIGHT(_K_D("Unable to get a major for mc13783_light"));
		return -1;
	}

	init_waitqueue_head(&suspendq);

	devfs_mk_cdev(MKDEV(mc13783_light_major, 0),
		      S_IFCHR | S_IRUGO | S_IWUSR, "mc13783_light");

	CHECK_ERROR(mc13783_light_init_reg());

	ret = driver_register(&mc13783_light_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&mc13783_light_ldm);
		if (ret != 0) {
			driver_unregister(&mc13783_light_driver_ldm);
		} else {
			mc13783_light_detected = 1;
			printk(KERN_INFO "mc13783 Light loaded\n");
		}
	}

	return ret;
}

static void __exit mc13783_light_exit(void)
{
	devfs_remove("mc13783_light");

	driver_unregister(&mc13783_light_driver_ldm);
	platform_device_unregister(&mc13783_light_ldm);
	unregister_chrdev(mc13783_light_major, "mc13783_light");

	printk(KERN_INFO "mc13783_light : successfully unloaded");
}

/*
 * Module entry points
 */

subsys_initcall(mc13783_light_init);
module_exit(mc13783_light_exit);

MODULE_DESCRIPTION("mc13783_light driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
