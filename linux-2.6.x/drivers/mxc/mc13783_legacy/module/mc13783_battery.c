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
 * @file mc13783_battery.c
 * @brief This is the main file of mc13783 Battery driver.
 *
 * @ingroup MC13783_BATTERY
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "../core/mc13783_config.h"
#include "../core/mc13783_event.h"
#include "../core/mc13783_external.h"

#include "mc13783_battery.h"
#include "mc13783_battery_defs.h"

//#define DEBUG

#ifdef DEBUG
#define TRACEMSG_BATTERY(fmt,args...)  printk("%s: " fmt, __FUNCTION__, ## args)
#else				/* DEBUG */
#define TRACEMSG_BATTERY(fmt,args...)
#endif				/* DEBUG */

#define MC13783_LOAD_ERROR_MSG		\
"mc13783 card was not correctly detected. Stop loading mc13783 Battery driver\n"

static int mc13783_battery_major;
static int mc13783_battery_detected = 0;

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
EXPORT_SYMBOL(mc13783_battery_set_out_voltage);
EXPORT_SYMBOL(mc13783_battery_get_out_voltage);
EXPORT_SYMBOL(mc13783_battery_set_dac_current);
EXPORT_SYMBOL(mc13783_battery_get_dac_current);
EXPORT_SYMBOL(mc13783_battery_set_trickle_current);
EXPORT_SYMBOL(mc13783_battery_get_trickle_current);
EXPORT_SYMBOL(mc13783_battery_set_out_control);
EXPORT_SYMBOL(mc13783_battery_get_out_control);
EXPORT_SYMBOL(mc13783_battery_set_reverse);
EXPORT_SYMBOL(mc13783_battery_get_reverse);
EXPORT_SYMBOL(mc13783_battery_set_threshold);
EXPORT_SYMBOL(mc13783_battery_get_threshold);
EXPORT_SYMBOL(mc13783_battery_set_unregulator);
EXPORT_SYMBOL(mc13783_battery_get_unregulator);
EXPORT_SYMBOL(mc13783_battery_set_charge_led);
EXPORT_SYMBOL(mc13783_battery_get_charge_led);
EXPORT_SYMBOL(mc13783_battery_set_5k_pull);
EXPORT_SYMBOL(mc13783_battery_get_5k_pull);
EXPORT_SYMBOL(mc13783_battery_event_sub);
EXPORT_SYMBOL(mc13783_battery_event_unsub);
EXPORT_SYMBOL(mc13783_battery_loaded);

/*
 * Internal function
 */
int mc13783_battery_event(t_bat_int event, void *callback, bool sub);

#ifdef __TEST_CODE_ENABLE__
/*!
 * This is the callback function called on TSI mc13783 event, used in synchronous
 * call.
 */
static void callback_battery_test(void)
{
	TRACEMSG_BATTERY(_K_I("*** Battery IT TEST mc13783 ***"));
}
#endif				/* __TEST_CODE_ENABLE__ */

/*!
 * This is the suspend of power management for the mc13783 battery API.
 * It supports SAVE and POWER_DOWN state.
 *
 * @param        dev            the device
 * @param        state          the state
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_battery_suspend(struct device *dev, u32 state, u32 level)
{
	unsigned int reg_value = 0;

	switch (level) {
	case SUSPEND_DISABLE:
		suspend_flag = 1;
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		CHECK_ERROR(mc13783_write_reg(PRIO_BATTERY, REG_CHARGER,
					      &reg_value));
		break;
	}

	return 0;
};

/*!
 * This is the resume of power management for the mc13783 battery API.
 * It supports RESTORE state.
 *
 * @param        dev            the device
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_battery_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_POWER_ON:
		/* nothing for mc13783 battery */
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

static struct device_driver mc13783_battery_driver_ldm = {
	.name = "mc13783_battery",
	.bus = &platform_bus_type,
	.suspend = mc13783_battery_suspend,
	.resume = mc13783_battery_resume,
};

static struct platform_device mc13783_battery_ldm = {
	.name = "mc13783_battery",
	.id = 1,
};

/*!
 * This function sets output voltage of charge regulator.
 *
 * @param        out_voltage  	output voltage
 *
 * @return       This function returns 0 if successful, -EINVAL if voltage value
 *		 is not valid, or -ENXIO if mc13783 IC is not available.
 */
int mc13783_battery_set_out_voltage(int out_voltage)
{
	int mc13783_version = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	if ((out_voltage < OUT_VOLT_405) || (out_voltage > OUT_VOLT_450)) {
		return -EINVAL;
	}

	mc13783_version = mxc_mc13783_get_ic_version();
	if (mc13783_version == -1) {
		return -ENXIO;
	}

	if ((mc13783_version >= 30) && (out_voltage == OUT_VOLT_410)) {
		return -EINVAL;
	}

	if ((mc13783_version < 30) && (out_voltage == OUT_VOLT_4375)) {
		return -EINVAL;
	}

	return mc13783_set_reg_value(0, REG_CHARGER, BITS_OUT_VOLTAGE,
				     out_voltage, LONG_OUT_VOLTAGE);
}

/*!
 * This function gets output voltage of charge regulator.
 *
 * @param        out_voltage  	output voltage
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_out_voltage(int *out_voltage)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_get_reg_value(PRIO_BATTERY, REG_CHARGER,
				     BITS_OUT_VOLTAGE, (int *)out_voltage,
				     LONG_OUT_VOLTAGE);
}

/*!
 * This function sets the current of the main charger DAC.
 *
 * @param        current_val  	current of main dac.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_dac_current(t_dac_current current_val)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_value(PRIO_BATTERY, REG_CHARGER,
				     BITS_CURRENT_MAIN, (int)current_val,
				     LONG_CURRENT_MAIN);
}

/*!
 * This function gets the current of the main charger DAC.
 *
 * @param        current_val  	current of main dac.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_dac_current(t_dac_current * current_val)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_get_reg_value(PRIO_BATTERY, REG_CHARGER,
				     BITS_CURRENT_MAIN, (int *)current_val,
				     LONG_CURRENT_MAIN);
}

/*!
 * This function sets the current of the trickle charger.
 *
 * @param        current_val  	current of the trickle charger.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_trickle_current(t_trickle_current current_val)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_value(PRIO_BATTERY, REG_CHARGER,
				     BITS_CURRENT_TRICKLE, (int)current_val,
				     LONG_CURRENT_TRICKLE);
}

/*!
 * This function gets the current of the trickle charger.
 *
 * @param        current_val  	current of the trickle charger.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_trickle_current(t_trickle_current * current_val)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_get_reg_value(PRIO_BATTERY, REG_CHARGER,
				     BITS_CURRENT_TRICKLE, (int *)current_val,
				     LONG_CURRENT_TRICKLE);
}

/*!
 * This function sets the output controls.
 * It sets the FETOVRD and FETCTRL bits of mc13783
 *
 * @param        control  	type of control.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_out_control(t_control control)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	switch (control) {
	case CONTROL_HARDWARE:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_BATTERY, REG_CHARGER,
						BIT_FETOVRD, 0));
		break;
	case CONTROL_BPFET_LOW:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_BATTERY, REG_CHARGER,
						BIT_FETOVRD, 1));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_BATTERY, REG_CHARGER,
						BIT_FETCTRL, 0));
		break;
	case CONTROL_BPFET_HIGH:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_BATTERY, REG_CHARGER,
						BIT_FETOVRD, 1));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_BATTERY, REG_CHARGER,
						BIT_FETCTRL, 1));
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return ERROR_NONE;
}

/*!
 * This function gets the output controls.
 * It gets the FETOVRD and FETCTRL bits of mc13783
 *
 * @param        control  	type of control.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_out_control(t_control * control)
{
	int ovrd = 0, ctrl = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_BATTERY, REG_CHARGER, BIT_FETOVRD,
					&ovrd));
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_BATTERY, REG_CHARGER, BIT_FETCTRL,
					&ctrl));

	if (ovrd == 0) {
		*control = CONTROL_HARDWARE;
	} else {
		if (ctrl == 0) {
			*control = CONTROL_BPFET_LOW;
		} else {
			*control = CONTROL_BPFET_HIGH;
		}
	}
	return ERROR_NONE;
}

/*!
 * This function sets reverse mode.
 *
 * @param        mode  	if true, reverse mode is enable.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_reverse(bool mode)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_BATTERY, REG_CHARGER, BIT_RVRSMODE,
				   mode);
}

/*!
 * This function gets reverse mode.
 *
 * @param        mode  	if true, reverse mode is enable.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_reverse(bool * mode)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_get_reg_bit(PRIO_BATTERY, REG_CHARGER, BIT_RVRSMODE,
				   mode);
}

/*!
 * This function sets over voltage threshold.
 *
 * @param        threshold  	value of over voltage threshold.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_threshold(int threshold)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if (threshold > BAT_THRESHOLD_MAX) {
		return ERROR_BAD_INPUT;
	}
	return mc13783_set_reg_value(PRIO_BATTERY, REG_CHARGER,
				     BITS_OVERVOLTAGE, (int)threshold,
				     LONG_OVERVOLTAGE);
}

/*!
 * This function gets over voltage threshold.
 *
 * @param        threshold  	value of over voltage threshold.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_threshold(int *threshold)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_get_reg_value(PRIO_BATTERY, REG_CHARGER,
				     BITS_OVERVOLTAGE, (int *)threshold,
				     LONG_OVERVOLTAGE);
}

/*!
 * This function sets unregulated charge.
 *
 * @param        unregul  	if true, the regulator charge is disabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_unregulator(bool unregul)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_BATTERY, REG_CHARGER,
				   BIT_UNREGULATED, unregul);
}

/*!
 * This function gets unregulated charge state.
 *
 * @param        unregul  	if true, the regulator charge is disabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_unregulator(bool * unregul)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_get_reg_bit(PRIO_BATTERY, REG_CHARGER,
				   BIT_UNREGULATED, unregul);
}

/*!
 * This function enables/disables charge led.
 *
 * @param        led  	Enable/disable led for charge.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_charge_led(bool led)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(0, REG_CHARGER, BIT_CHRG_LED, led);
}

/*!
 * This function returns state of charge led.
 *
 * @param        led  	state of charge led.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_charge_led(bool * led)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}

	return mc13783_get_reg_bit(0, REG_CHARGER, BIT_CHRG_LED, led);
}

/*!
 * This function enables/disables a 5K pull down.
 *
 * @param        en_dis  	Enable/disable pull down.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_set_5k_pull(bool en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_BATTERY, REG_CHARGER, BIT_CHRGRAWPDEN,
				   en_dis);
}

/*!
 * This function returns 5K pull down configuration.
 *
 * @param        en_dis  	Enable/disable pull down.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_get_5k_pull(bool * en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_get_reg_bit(PRIO_BATTERY, REG_CHARGER, BIT_CHRGRAWPDEN,
				   en_dis);
}

/*!
 * This function is used to un/subscribe on battery event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 * @param        sub      	define if Un/subscribe event.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_event(t_bat_int event, void *callback, bool sub)
{
	type_event_notification bat_event;

	mc13783_event_init(&bat_event);
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
		return ERROR_BAD_INPUT;
		break;
	}
	if (sub == true) {
		CHECK_ERROR(mc13783_event_subscribe(bat_event));
	} else {
		CHECK_ERROR(mc13783_event_unsubscribe(bat_event));
	}
	return ERROR_NONE;
}

/*!
 * This function is used to subscribe on battery event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_event_sub(t_bat_int event, void *callback)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_battery_event(event, callback, true);
}

/*!
 * This function is used to un subscribe on battery event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_battery_event_unsub(t_bat_int event, void *callback)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_battery_event(event, callback, false);
}

/*!
 * This function implements IOCTL controls on a mc13783 battery device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int mc13783_battery_ioctl(struct inode *inode, struct file *file,
				 unsigned int cmd, unsigned long arg)
{
	int i = 0, ret_val = 0;

	if (_IOC_TYPE(cmd) != 'B')
		return -ENOTTY;
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}

	switch (cmd) {
	case MC13783_BATTERY_INIT:
		TRACEMSG_BATTERY(_K_D("Init Battery"));
		CHECK_ERROR(mc13783_set_reg_value(PRIO_BATTERY, REG_CHARGER,
						  0, (int)DEF_VALUE, 24));
		break;

#ifdef __TEST_CODE_ENABLE__
	case MC13783_BATTERY_CHECK:
		TRACEMSG_BATTERY("Check mc13783 Battery API\n");

		CHECK_ERROR(mc13783_battery_set_charge_led(true));
		CHECK_ERROR(mc13783_battery_get_charge_led(&ret_val));

		if (ret_val != true) {
			TRACEMSG_BATTERY("ERROR in led config\n");
		}

		CHECK_ERROR(mc13783_battery_set_out_voltage(OUT_VOLT_405));
		CHECK_ERROR(mc13783_battery_get_out_voltage(&ret_val));
		if (ret_val != OUT_VOLT_405) {
			TRACEMSG_BATTERY("ERROR in out voltage config\n");
			return -EINVAL;
		}

		CHECK_ERROR(mc13783_battery_set_out_voltage(0));
		for (i = TRICKLE_CURRENT_0; i <= TRICKLE_CURRENT_12; i++) {
			CHECK_ERROR(mc13783_battery_set_trickle_current(i));
			CHECK_ERROR(mc13783_battery_get_trickle_current
				    ((t_trickle_current *) & ret_val));
			if (ret_val != i) {
				TRACEMSG_BATTERY
				    ("ERROR in trickle current config\n");
				return -EINVAL;
			}
		}

		CHECK_ERROR(mc13783_battery_set_trickle_current
			    (TRICKLE_CURRENT_0));

		for (i = CONTROL_HARDWARE; i <= CONTROL_BPFET_HIGH; i++) {
			CHECK_ERROR(mc13783_battery_set_out_control(i));
			CHECK_ERROR(mc13783_battery_get_out_control((t_control
								     *) &
								    ret_val));
			if (ret_val != i) {
				TRACEMSG_BATTERY("ERROR in contol config\n");
				return -EINVAL;
			}
		}

		CHECK_ERROR(mc13783_battery_set_out_control(CONTROL_HARDWARE));

		for (i = 0; i <= 3; i++) {
			CHECK_ERROR(mc13783_battery_set_threshold(i));
			CHECK_ERROR(mc13783_battery_get_threshold(&ret_val));
			if (ret_val != i) {
				TRACEMSG_BATTERY("ERROR in threshold config\n");
				return -EINVAL;
			}
		}

		CHECK_ERROR(mc13783_battery_set_threshold(0));

		CHECK_ERROR(mc13783_battery_set_unregulator(true));
		CHECK_ERROR(mc13783_battery_get_unregulator(&ret_val));
		if (ret_val != true) {
			TRACEMSG_BATTERY("ERROR in unregulator config\n");
		}

		CHECK_ERROR(mc13783_battery_set_unregulator(false));
		CHECK_ERROR(mc13783_battery_get_unregulator(&ret_val));
		if (ret_val != false) {
			TRACEMSG_BATTERY("ERROR in unregulator config\n");
		}

		CHECK_ERROR(mc13783_battery_set_5k_pull(true));
		CHECK_ERROR(mc13783_battery_get_5k_pull(&ret_val));
		if (ret_val != false) {
			TRACEMSG_BATTERY("ERROR in led config\n");
		}

		for (i = BAT_IT_CHG_DET; i <= BAT_IT_BELOW_THRESHOLD; i++) {
			CHECK_ERROR(mc13783_battery_event_sub(i,
							      callback_battery_test));
			CHECK_ERROR(mc13783_battery_event_unsub(i,
								callback_battery_test));
		}

		CHECK_ERROR(mc13783_set_reg_value(PRIO_BATTERY, REG_CHARGER,
						  0, (int)DEF_VALUE, 24));
		break;

#endif				/* __TEST_CODE_ENABLE__ */
	default:
		printk(KERN_WARNING "%d unsupported ioctl command\n", cmd);
		return -EINVAL;
	}

	return ERROR_NONE;
}

/*!
 * This function implements the open method on a mc13783 battery device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_battery_open(struct inode *inode, struct file *file)
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
 * This function implements the release method on a mc13783 battery device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_battery_release(struct inode *inode, struct file *file)
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

static struct file_operations mc13783_battery_fops = {
	.owner = THIS_MODULE,
	.ioctl = mc13783_battery_ioctl,
	.open = mc13783_battery_open,
	.release = mc13783_battery_release,
};

int mc13783_battery_loaded(void)
{
	return mc13783_battery_detected;
}

/*
 * Init and Exit
 */

static int __init mc13783_battery_init(void)
{
	int ret = 0;

	if (mc13783_core_loaded() == 0) {
		printk(KERN_INFO MC13783_LOAD_ERROR_MSG);
		return -1;
	}

	mc13783_battery_major = register_chrdev(0, MC13783_BATTERY_STRING,
						&mc13783_battery_fops);
	if (mc13783_battery_major < 0) {
		TRACEMSG_BATTERY(_K_D
				 ("Unable to get a major for mc13783_battery"));
		return -1;
	}
	init_waitqueue_head(&suspendq);

	devfs_mk_cdev(MKDEV(mc13783_battery_major, 0), S_IFCHR | S_IRUGO |
		      S_IWUSR, MC13783_BATTERY_STRING);

	ret = driver_register(&mc13783_battery_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&mc13783_battery_ldm);
		if (ret != 0) {
			driver_unregister(&mc13783_battery_driver_ldm);
		} else {
			mc13783_battery_detected = 1;
			printk(KERN_INFO "mc13783 Battery loaded\n");
		}
	}

	return ret;
}

static void __exit mc13783_battery_exit(void)
{
	devfs_remove(MC13783_BATTERY_STRING);

	driver_unregister(&mc13783_battery_driver_ldm);
	platform_device_unregister(&mc13783_battery_ldm);
	unregister_chrdev(mc13783_battery_major, MC13783_BATTERY_STRING);

	printk(KERN_INFO "mc13783_battery : successfully unloaded");
}

/*
 * Module entry points
 */

module_init(mc13783_battery_init);
module_exit(mc13783_battery_exit);

MODULE_DESCRIPTION("mc13783_battery driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
