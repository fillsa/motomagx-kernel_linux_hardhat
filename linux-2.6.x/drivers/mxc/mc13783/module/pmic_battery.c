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
 * @file pmic_battery.c
 * @brief This is the main file of Pmic Battery driver.
 *
 * @ingroup PMIC_BATTERY
 */

/*
 * Includes
 */
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/wait.h>

#include "asm/arch/pmic_status.h"
#include "asm/arch/pmic_external.h"
#include "asm/arch/pmic_battery.h"
#include "asm/arch/pmic_adc.h"
#include "pmic_battery_defs.h"
#include "linux/device.h"
#include "linux/delay.h"
#include "../core/pmic_config.h"

static int pmic_battery_major;

/*!
 * Number of users waiting in suspendq
 */
static int swait = 0;

/*!
 * To indicate whether any of the battery devices are suspending
 */
static int suspend_flag = 0;

/*!
 * The suspendq is used to block application calls
 */
static wait_queue_head_t suspendq;

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(pmic_batt_enable_charger);
EXPORT_SYMBOL(pmic_batt_disable_charger);
EXPORT_SYMBOL(pmic_batt_set_charger);
EXPORT_SYMBOL(pmic_batt_get_charger_setting);
EXPORT_SYMBOL(pmic_batt_get_charge_current);
EXPORT_SYMBOL(pmic_batt_enable_eol);
EXPORT_SYMBOL(pmic_batt_bp_enable_eol);
EXPORT_SYMBOL(pmic_batt_disable_eol);
EXPORT_SYMBOL(pmic_batt_set_out_control);
EXPORT_SYMBOL(pmic_batt_set_threshold);
EXPORT_SYMBOL(pmic_batt_led_control);
EXPORT_SYMBOL(pmic_batt_set_reverse_supply);
EXPORT_SYMBOL(pmic_batt_set_unregulated);
EXPORT_SYMBOL(pmic_batt_set_5k_pull);
EXPORT_SYMBOL(pmic_batt_event_subscribe);
EXPORT_SYMBOL(pmic_batt_event_unsubscribe);

/*!
 * This is the suspend of power management for the pmic battery API.
 * It suports SAVE and POWER_DOWN state.
 *
 * @param        dev            the device
 * @param        state          the state
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int pmic_battery_suspend(struct device *dev, u32 state, u32 level)
{
	unsigned int reg_value = 0;

	switch (level) {
	case SUSPEND_DISABLE:
		suspend_flag = 1;
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		CHECK_ERROR(pmic_write_reg(PRIO_BATTERY,
					   REG_CHARGER, reg_value,
					   PMIC_ALL_BITS));
		break;
	}

	return 0;
};

/*!
 * This is the resume of power management for the pmic battery API.
 * It suports RESTORE state.
 *
 * @param        dev            the device
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int pmic_battery_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_POWER_ON:
		/* nothing for Pmic battery */
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

static struct device_driver pmic_battery_driver_ldm = {
	.name = "PMIC_battery",
	.bus = &platform_bus_type,
	.suspend = pmic_battery_suspend,
	.resume = pmic_battery_resume,
};

static struct platform_device pmic_battery_ldm = {
	.name = "PMIC_battery",
	.id = 1,
};

/*!
 * This function is used to start charging a battery. For different charger,
 * different voltage and current range are supported. \n
 *
 *
 * @param      chgr        Charger as defined in \b t_batt_charger.
 * @param      c_voltage   Charging voltage.
 * @param      c_current   Charging current.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_enable_charger(t_batt_charger chgr,
				     unsigned char c_voltage,
				     unsigned char c_current)
{
	unsigned int val, mask, reg;

	val = 0;
	mask = 0;
	reg = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	switch (chgr) {
	case BATT_MAIN_CHGR:
		val = BITFVAL(MC13783_BATT_DAC_DAC, c_current) |
		    BITFVAL(MC13783_BATT_DAC_V_DAC, c_voltage);
		mask = BITFMASK(MC13783_BATT_DAC_DAC) |
		    BITFMASK(MC13783_BATT_DAC_V_DAC);
		reg = REG_CHARGER;
		break;

	case BATT_CELL_CHGR:
		val = BITFVAL(MC13783_BATT_DAC_V_COIN, c_voltage) |
		    BITFVAL(MC13783_BATT_DAC_COIN_CH_EN,
			    MC13783_BATT_DAC_COIN_CH_EN_ENABLED);
		mask = BITFMASK(MC13783_BATT_DAC_V_COIN) |
		    BITFMASK(MC13783_BATT_DAC_COIN_CH_EN);
		reg = REG_POWER_CONTROL_0;
		break;

	case BATT_TRCKLE_CHGR:
		val = BITFVAL(MC13783_BATT_DAC_TRCKLE, c_current);
		mask = BITFMASK(MC13783_BATT_DAC_TRCKLE);
		reg = REG_CHARGER;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, reg, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function turns off a charger.
 *
 * @param      chgr        Charger as defined in \b t_batt_charger.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_disable_charger(t_batt_charger chgr)
{
	unsigned int val, mask, reg;

	val = 0;
	mask = 0;
	reg = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	switch (chgr) {
	case BATT_MAIN_CHGR:
		val = BITFVAL(MC13783_BATT_DAC_DAC, 0) |
		    BITFVAL(MC13783_BATT_DAC_V_DAC, 0);
		mask = BITFMASK(MC13783_BATT_DAC_DAC) |
		    BITFMASK(MC13783_BATT_DAC_V_DAC);
		reg = REG_CHARGER;
		break;

	case BATT_CELL_CHGR:
		val = BITFVAL(MC13783_BATT_DAC_COIN_CH_EN,
			      MC13783_BATT_DAC_COIN_CH_EN_DISABLED);
		mask = BITFMASK(MC13783_BATT_DAC_COIN_CH_EN);
		reg = REG_POWER_CONTROL_0;
		break;

	case BATT_TRCKLE_CHGR:
		val = BITFVAL(MC13783_BATT_DAC_TRCKLE, 0);
		mask = BITFMASK(MC13783_BATT_DAC_TRCKLE);
		reg = REG_CHARGER;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, reg, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function is used to change the charger setting.
 *
 * @param      chgr        Charger as defined in \b t_batt_charger.
 * @param      c_voltage   Charging voltage.
 * @param      c_current   Charging current.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_charger(t_batt_charger chgr,
				  unsigned char c_voltage,
				  unsigned char c_current)
{
	unsigned int val, mask, reg;

	val = 0;
	mask = 0;
	reg = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	switch (chgr) {
	case BATT_MAIN_CHGR:
		val = BITFVAL(MC13783_BATT_DAC_DAC, c_current) |
		    BITFVAL(MC13783_BATT_DAC_V_DAC, c_voltage);
		mask = BITFMASK(MC13783_BATT_DAC_DAC) |
		    BITFMASK(MC13783_BATT_DAC_V_DAC);
		reg = REG_CHARGER;
		break;

	case BATT_CELL_CHGR:
		val = BITFVAL(MC13783_BATT_DAC_V_COIN, c_voltage);
		mask = BITFMASK(MC13783_BATT_DAC_V_COIN);
		reg = REG_POWER_CONTROL_0;
		break;

	case BATT_TRCKLE_CHGR:
		val = BITFVAL(MC13783_BATT_DAC_TRCKLE, c_current);
		mask = BITFMASK(MC13783_BATT_DAC_TRCKLE);
		reg = REG_CHARGER;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, reg, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function is used to retrive the charger setting.
 *
 * @param      chgr        Charger as defined in \b t_batt_charger.
 * @param      c_voltage   Output parameter for charging voltage setting.
 * @param      c_current   Output parameter for charging current setting.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_get_charger_setting(t_batt_charger chgr,
					  unsigned char *c_voltage,
					  unsigned char *c_current)
{
	unsigned int val, reg;

	reg = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	switch (chgr) {
	case BATT_MAIN_CHGR:
	case BATT_TRCKLE_CHGR:
		reg = REG_CHARGER;
		break;
	case BATT_CELL_CHGR:
		reg = REG_POWER_CONTROL_0;
		break;
	default:
		return PMIC_PARAMETER_ERROR;
	}

	CHECK_ERROR(pmic_read_reg(PRIO_BATTERY, reg, &val, PMIC_ALL_BITS));

	switch (chgr) {
	case BATT_MAIN_CHGR:
		*c_voltage = BITFEXT(val, MC13783_BATT_DAC_V_DAC);;
		*c_current = BITFEXT(val, MC13783_BATT_DAC_DAC);
		break;

	case BATT_CELL_CHGR:
		*c_voltage = BITFEXT(val, MC13783_BATT_DAC_V_COIN);
		*c_current = 0;
		break;

	case BATT_TRCKLE_CHGR:
		*c_voltage = 0;
		*c_current = BITFEXT(val, MC13783_BATT_DAC_TRCKLE);
		return PMIC_NOT_SUPPORTED;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function is retrives the main battery charging current.
 *
 * @param      c_current   Output parameter for charging current setting.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_get_charge_current(unsigned short *c_current)
{
	t_channel channel;
	unsigned short result[8];

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	channel = CHARGE_CURRENT;
	CHECK_ERROR(pmic_adc_convert(channel, result));
	*c_current = result[0];

	return PMIC_SUCCESS;
}

/*!
 * This function enables End-of-Life comparator. Not supported on
 * mc13783. Use pmic_batt_bp_enable_eol function.
 *
 * @param      threshold  End-of-Life threshold.
 *
 * @return     This function returns PMIC_UNSUPPORTED
 */
PMIC_STATUS pmic_batt_enable_eol(unsigned char threshold)
{
	return PMIC_NOT_SUPPORTED;
}

/*!
 * This function enables End-of-Life comparator.
 *
 * @param      typical  Falling Edge Threshold threshold.
 * 			@verbatim
			BPDET	UVDET	LOBATL
			____	_____   ___________
			0	2.6	UVDET + 0.2
			1	2.6	UVDET + 0.3
			2	2.6	UVDET + 0.4
			3	2.6	UVDET + 0.5
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_bp_enable_eol(t_bp_threshold typical)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	val = BITFVAL(MC13783_BATT_DAC_EOL_CMP_EN,
		      MC13783_BATT_DAC_EOL_CMP_EN_ENABLE) |
	    BITFVAL(MC13783_BATT_DAC_EOL_SEL, typical);
	mask = BITFMASK(MC13783_BATT_DAC_EOL_CMP_EN) |
	    BITFMASK(MC13783_BATT_DAC_EOL_SEL);

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_POWER_CONTROL_0,
				   val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function disables End-of-Life comparator.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_disable_eol(void)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	val = BITFVAL(MC13783_BATT_DAC_EOL_CMP_EN,
		      MC13783_BATT_DAC_EOL_CMP_EN_DISABLE);
	mask = BITFMASK(MC13783_BATT_DAC_EOL_CMP_EN);

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_POWER_CONTROL_0,
				   val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function sets the output controls.
 * It sets the FETOVRD and FETCTRL bits of mc13783
 *
 * @param        control        type of control.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_batt_set_out_control(t_control control)
{
	unsigned int val, mask;
	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (control) {
	case CONTROL_HARDWARE:
		val = BITFVAL(MC13783_BATT_DAC_FETOVRD_EN, 0) |
		    BITFVAL(MC13783_BATT_DAC_FETCTRL_EN, 0);
		mask = BITFMASK(MC13783_BATT_DAC_FETOVRD_EN) |
		    BITFMASK(MC13783_BATT_DAC_FETCTRL_EN);
		break;
	case CONTROL_BPFET_LOW:
		val = BITFVAL(MC13783_BATT_DAC_FETOVRD_EN, 1) |
		    BITFVAL(MC13783_BATT_DAC_FETCTRL_EN, 0);
		mask = BITFMASK(MC13783_BATT_DAC_FETOVRD_EN) |
		    BITFMASK(MC13783_BATT_DAC_FETCTRL_EN);
		break;
	case CONTROL_BPFET_HIGH:
		val = BITFVAL(MC13783_BATT_DAC_FETOVRD_EN, 1) |
		    BITFVAL(MC13783_BATT_DAC_FETCTRL_EN, 1);
		mask = BITFMASK(MC13783_BATT_DAC_FETOVRD_EN) |
		    BITFMASK(MC13783_BATT_DAC_FETCTRL_EN);
		break;
	default:
		return PMIC_PARAMETER_ERROR;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_CHARGER, val, mask));
	return PMIC_SUCCESS;
}

/*!
 * This function sets over voltage threshold.
 *
 * @param        threshold      value of over voltage threshold.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_batt_set_threshold(int threshold)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	if (threshold > BAT_THRESHOLD_MAX) {
		return PMIC_PARAMETER_ERROR;
	}
	val = BITFVAL(MC13783_BATT_DAC_OVCTRL, threshold);
	mask = BITFMASK(MC13783_BATT_DAC_OVCTRL);
	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_CHARGER, val, mask));
	return PMIC_SUCCESS;
}

/*!
 * This function controls charge LED.
 *
 * @param      on   If on is ture, LED will be turned on,
 *                  or otherwise, LED will be turned off.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_led_control(bool on)
{
	unsigned val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	val = BITFVAL(MC13783_BATT_DAC_LED_EN, on);
	mask = BITFMASK(MC13783_BATT_DAC_LED_EN);

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_CHARGER, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function sets reverse supply mode.
 *
 * @param      enable     If enable is ture, reverse supply mode is enable,
 *                        or otherwise, reverse supply mode is disabled.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_reverse_supply(bool enable)
{
	unsigned val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	val = BITFVAL(MC13783_BATT_DAC_REVERSE_SUPPLY, enable);
	mask = BITFMASK(MC13783_BATT_DAC_REVERSE_SUPPLY);

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_CHARGER, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function sets unregulatored charging mode on main battery.
 *
 * @param      enable     If enable is ture, unregulated charging mode is
 *                        enable, or otherwise, disabled.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_unregulated(bool enable)
{
	unsigned val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	val = BITFVAL(MC13783_BATT_DAC_UNREGULATED, enable);
	mask = BITFMASK(MC13783_BATT_DAC_UNREGULATED);

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_CHARGER, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function sets a 5K pull down at CHRGRAW.
 * To be used in the dual path charging configuration.
 *
 * @param      enable     If enable is true, 5k pull down is
 *                        enable, or otherwise, disabled.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_5k_pull(bool enable)
{
	unsigned val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	val = BITFVAL(MC13783_BATT_DAC_5K, enable);
	mask = BITFMASK(MC13783_BATT_DAC_5K);

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_CHARGER, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function is used to un/subscribe on battery event IT.
 *
 * @param        event          type of event.
 * @param        callback       event callback function.
 * @param        sub            define if Un/subscribe event.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS mc13783_battery_event(t_batt_event event, void *callback, bool sub)
{
	type_event_notification bat_event;

	pmic_event_init(&bat_event);
	bat_event.callback = callback;
	switch (event) {
	case BAT_IT_CHG_DET:
		bat_event.event = EVENT_WLOWI;
		break;
	case BAT_IT_CHG_OVERVOLT:
		bat_event.event = EVENT_CHGOVI;
		break;
	case BAT_IT_CHG_REVERSE:
		bat_event.event = EVENT_CHGREVI;
		break;
	case BAT_IT_CHG_SHORT_CIRCUIT:
		bat_event.event = EVENT_CHGSHORTI;
		break;
	case BAT_IT_CCCV:
		bat_event.event = EVENT_CCCVI;
		break;
	case BAT_IT_BELOW_THRESHOLD:
		bat_event.event = EVENT_CHRGCURRI;
		break;
	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}
	if (sub == true) {
		CHECK_ERROR(pmic_event_subscribe(&bat_event));
	} else {
		CHECK_ERROR(pmic_event_unsubscribe(&bat_event));
	}
	return 0;
}

/*!
 * This function is used to subscribe on battery event IT.
 *
 * @param        event          type of event.
 * @param        callback       event callback function.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_batt_event_subscribe(t_batt_event event, void *callback)
{
	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	return mc13783_battery_event(event, callback, true);
}

/*!
 * This function is used to un subscribe on battery event IT.
 *
 * @param        event          type of event.
 * @param        callback       event callback function.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_batt_event_unsubscribe(t_batt_event event, void *callback)
{
	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	return mc13783_battery_event(event, callback, false);
}

/*!
 * This function implements IOCTL controls on a PMIC Battery device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int pmic_battery_ioctl(struct inode *inode, struct file *file,
			      unsigned int cmd, unsigned long arg)
{
	t_charger_setting *chgr_setting;
	unsigned short c_current;
	t_eol_setting *eol_setting;

	if (_IOC_TYPE(cmd) != 'p')
		return -ENOTTY;

	switch (cmd) {
	case PMIC_BATT_CHARGER_CONTROL:
		if ((chgr_setting = kmalloc(sizeof(t_charger_setting),
					    GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(chgr_setting, (t_charger_setting *) arg,
				   sizeof(t_charger_setting))) {
			kfree(chgr_setting);
			return -EFAULT;
		}

		if (chgr_setting->on != false) {
			CHECK_ERROR(pmic_batt_enable_charger(chgr_setting->chgr,
							     chgr_setting->
							     c_voltage,
							     chgr_setting->
							     c_current));
		} else {
			CHECK_ERROR(pmic_batt_disable_charger
				    (chgr_setting->chgr));
		}

		kfree(chgr_setting);
		break;

	case PMIC_BATT_SET_CHARGER:
		if ((chgr_setting = kmalloc(sizeof(t_charger_setting),
					    GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(chgr_setting, (t_charger_setting *) arg,
				   sizeof(t_charger_setting))) {
			kfree(chgr_setting);
			return -EFAULT;
		}

		CHECK_ERROR(pmic_batt_set_charger(chgr_setting->chgr,
						  chgr_setting->c_voltage,
						  chgr_setting->c_current));

		kfree(chgr_setting);
		break;

	case PMIC_BATT_GET_CHARGER:
		if ((chgr_setting = kmalloc(sizeof(t_charger_setting),
					    GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(chgr_setting, (t_charger_setting *) arg,
				   sizeof(t_charger_setting))) {
			kfree(chgr_setting);
			return -EFAULT;
		}

		CHECK_ERROR(pmic_batt_get_charger_setting(chgr_setting->chgr,
							  &chgr_setting->
							  c_voltage,
							  &chgr_setting->
							  c_current));
		if (copy_to_user
		    ((t_charger_setting *) arg, chgr_setting,
		     sizeof(t_charger_setting))) {
			return -EFAULT;
		}

		kfree(chgr_setting);
		break;

	case PMIC_BATT_GET_CHARGER_CURRENT:
		CHECK_ERROR(pmic_batt_get_charge_current(&c_current));
		if (copy_to_user((unsigned char *)arg, &c_current,
				 sizeof(unsigned char *))) {
			return -EFAULT;
		}

		break;

	case PMIC_BATT_EOL_CONTROL:
		if ((eol_setting = kmalloc(sizeof(t_eol_setting), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(eol_setting, (t_eol_setting *) arg,
				   sizeof(t_eol_setting))) {
			kfree(eol_setting);
			return -EFAULT;
		}

		if (eol_setting->enable != false) {
			CHECK_ERROR(pmic_batt_bp_enable_eol
				    (eol_setting->typical));
		} else {
			CHECK_ERROR(pmic_batt_disable_eol());
		}

		kfree(eol_setting);
		break;

	case PMIC_BATT_SET_OUT_CONTROL:
		CHECK_ERROR(pmic_batt_set_out_control((t_control) arg));
		break;

	case PMIC_BATT_SET_THRESHOLD:
		CHECK_ERROR(pmic_batt_set_threshold((int)arg));
		break;

	case PMIC_BATT_LED_CONTROL:
		CHECK_ERROR(pmic_batt_led_control((bool) arg));
		break;

	case PMIC_BATT_REV_SUPP_CONTROL:
		CHECK_ERROR(pmic_batt_set_reverse_supply((bool) arg));
		break;

	case PMIC_BATT_UNREG_CONTROL:
		CHECK_ERROR(pmic_batt_set_unregulated((bool) arg));
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

/*!
 * This function implements the open method on a Pmic battery device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_battery_open(struct inode *inode, struct file *file)
{
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	return 0;
}

/*!
 * This function implements the release method on a Pmic battery device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_battery_release(struct inode *inode, struct file *file)
{
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	return 0;
}

static struct file_operations pmic_battery_fops = {
	.owner = THIS_MODULE,
	.ioctl = pmic_battery_ioctl,
	.open = pmic_battery_open,
	.release = pmic_battery_release,
};

/*
 * Init and Exit
 */

static int __init pmic_battery_init(void)
{
	int ret = 0;

	pmic_battery_major = register_chrdev(0, PMIC_BATTERY_STRING,
					     &pmic_battery_fops);

	if (pmic_battery_major < 0) {
		TRACEMSG_BATTERY(_K_D
				 ("Unable to get a major for pmic_battery"));
		return -1;
	}
	init_waitqueue_head(&suspendq);

	devfs_mk_cdev(MKDEV(pmic_battery_major, 0), S_IFCHR | S_IRUGO |
		      S_IWUSR, PMIC_BATTERY_STRING);

	ret = driver_register(&pmic_battery_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&pmic_battery_ldm);
		if (ret != 0) {
			driver_unregister(&pmic_battery_driver_ldm);
		}
	}

	printk(KERN_INFO "mc13783 Battery loading\n");

	return ret;
}

static void __exit pmic_battery_exit(void)
{
	devfs_remove(PMIC_BATTERY_STRING);

	driver_unregister(&pmic_battery_driver_ldm);

	platform_device_unregister(&pmic_battery_ldm);

	unregister_chrdev(pmic_battery_major, PMIC_BATTERY_STRING);
	TRACEMSG_BATTERY(_K_I("Pmic_battery : successfully unloaded"));
}

/*
 * Module entry points
 */

module_init(pmic_battery_init);
module_exit(pmic_battery_exit);

MODULE_DESCRIPTION("pmic_battery driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
