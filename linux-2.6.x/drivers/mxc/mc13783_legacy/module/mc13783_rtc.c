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
 * @file mc13783_rtc.c
 * @brief This is the main file of mc13783 rtc driver.
 *
 * @ingroup MC13783_RTC
 */

/*
 * Includes
 */
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "../core/mc13783_external.h"
#include "../core/mc13783_config.h"

#include "mc13783_rtc.h"

#define MC13783_LOAD_ERROR_MSG		\
"mc13783 card was not correctly detected. Stop loading mc13783 RTC driver\n"

/*
 * Global variables
 */
static int mc13783_rtc_major;
static void callback_alarm_asynchronous(void);
static void callback_alarm_synchronous(void);
static unsigned int mc13783_rtc_poll(struct file *file, poll_table * wait);
static DECLARE_WAIT_QUEUE_HEAD(queue_alarm);
static DECLARE_WAIT_QUEUE_HEAD(mc13783_rtc_wait);
static type_event_notification alarm_event;
static int mc13783_rtc_detected = 0;
static bool mc13783_rtc_done = 0;

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(mc13783_rtc_set_time);
EXPORT_SYMBOL(mc13783_rtc_get_time);
EXPORT_SYMBOL(mc13783_rtc_set_time_alarm);
EXPORT_SYMBOL(mc13783_rtc_get_time_alarm);
EXPORT_SYMBOL(mc13783_rtc_wait_alarm);
EXPORT_SYMBOL(mc13783_rtc_event_sub);
EXPORT_SYMBOL(mc13783_rtc_event_unsub);
EXPORT_SYMBOL(mc13783_rtc_loaded);

/*
 * Real Time Clock mc13783 API
 */

/*!
 * This is the callback function called on TSI mc13783 event, used in asynchronous
 * call.
 */
static void callback_alarm_asynchronous(void)
{
	mc13783_rtc_done = true;
}

/*!
 * This is the callback function is used in test code for (un)sub.
 */
static void callback_test_sub(void)
{
	TRACEMSG_ALL_TIME(_K_I("*****************************************"));
	TRACEMSG_ALL_TIME(_K_I("***** mc13783 RTC 'Alarm IT CallBack' *****"));
	TRACEMSG_ALL_TIME(_K_I("*****************************************"));
}

/*!
 * This is the callback function called on TSI mc13783 event, used in synchronous
 * call.
 */
static void callback_alarm_synchronous(void)
{
	TRACEMSG_ALL_TIME(_K_D("*** Alarm IT mc13783 ***"));
	wake_up(&queue_alarm);
}

/*!
 * This function wait the Alarm event
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_wait_alarm(void)
{
	DEFINE_WAIT(wait);
	mc13783_event_init(&alarm_event);
	alarm_event.event = EVENT_TODAI;
	alarm_event.callback = callback_alarm_synchronous;
	CHECK_ERROR(mc13783_event_subscribe(alarm_event));
	prepare_to_wait(&queue_alarm, &wait, TASK_UNINTERRUPTIBLE);
	schedule();
	finish_wait(&queue_alarm, &wait);
	CHECK_ERROR(mc13783_event_unsubscribe(alarm_event));
	return ERROR_NONE;
}

/*!
 * This function set the real time clock of mc13783
 *
 * @param        mc13783_time  	value of date and time
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_set_time(struct timeval *mc13783_time)
{
	unsigned int tod_reg_val = 0;
	unsigned int day_reg_val = 0;
	tod_reg_val = mc13783_time->tv_sec % 86400;
	day_reg_val = mc13783_time->tv_sec / 86400;

	CHECK_ERROR(mc13783_write_reg_value
		    (PRIO_RTC, REG_RTC_TIME, tod_reg_val));
	CHECK_ERROR(mc13783_write_reg_value
		    (PRIO_RTC, REG_RTC_DAY, day_reg_val));

	return ERROR_NONE;
}

/*!
 * This function get the real time clock of mc13783
 *
 * @param        mc13783_time  	return value of date and time
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_get_time(struct timeval *mc13783_time)
{
	unsigned int tod_reg_val = 0;
	unsigned int day_reg_val = 0;

	CHECK_ERROR(mc13783_read_reg(PRIO_RTC, REG_RTC_TIME, &tod_reg_val));
	CHECK_ERROR(mc13783_read_reg(PRIO_RTC, REG_RTC_DAY, &day_reg_val));
	mc13783_time->tv_sec = (unsigned long)((unsigned long)(tod_reg_val &
							       0x0001FFFF) +
					       (unsigned long)(day_reg_val *
							       86400));
	return ERROR_NONE;
}

/*!
 * This function set the real time clock alarm of mc13783
 *
 * @param        mc13783_time  	value of date and time
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_set_time_alarm(struct timeval *mc13783_time)
{
	unsigned int tod_reg_val = 0;
	unsigned int day_reg_val = 0;
	tod_reg_val = mc13783_time->tv_sec % 86400;
	day_reg_val = mc13783_time->tv_sec / 86400;

	CHECK_ERROR(mc13783_write_reg_value
		    (PRIO_RTC, REG_RTC_ALARM, tod_reg_val));
	CHECK_ERROR(mc13783_write_reg_value
		    (PRIO_RTC, REG_RTC_DAY_ALARM, day_reg_val));

	return ERROR_NONE;
}

/*!
 * This function get the real time clock alarm of mc13783
 *
 * @param        mc13783_time  	return value of date and time
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_get_time_alarm(struct timeval *mc13783_time)
{
	unsigned int tod_reg_val = 0;
	unsigned int day_reg_val = 0;

	CHECK_ERROR(mc13783_read_reg(PRIO_RTC, REG_RTC_ALARM, &tod_reg_val));
	CHECK_ERROR(mc13783_read_reg
		    (PRIO_RTC, REG_RTC_DAY_ALARM, &day_reg_val));

	mc13783_time->tv_sec = (unsigned long)((unsigned long)(tod_reg_val &
							       0x0001FFFF) +
					       (unsigned long)(day_reg_val *
							       86400));

	return ERROR_NONE;
}

/*!
 * This function is used to un/subscribe on rtc event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 * @param        sub      	define if Un/subscribe event.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_event(t_rtc_int event, void *callback, bool sub)
{
	type_event_notification rtc_event;

	mc13783_event_init(&rtc_event);
	rtc_event.callback = callback;
	switch (event) {
	case RTC_IT_ALARM:
		rtc_event.event = EVENT_TODAI;
		break;
	case RTC_IT_1HZ:
		rtc_event.event = EVENT_E1HZI;
		break;
	case RTC_IT_RST:
		rtc_event.event = EVENT_RTCRSTI;
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	if (sub == true) {
		CHECK_ERROR(mc13783_event_subscribe(rtc_event));
	} else {
		CHECK_ERROR(mc13783_event_unsubscribe(rtc_event));
	}
	return ERROR_NONE;
}

/*!
 * This function is used to subscribe on rtc event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_event_sub(t_rtc_int event, void *callback)
{
	CHECK_ERROR(mc13783_rtc_event(event, callback, true));
	return ERROR_NONE;

}

/*!
 * This function is used to un subscribe on rtc event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_event_unsub(t_rtc_int event, void *callback)
{
	CHECK_ERROR(mc13783_rtc_event(event, callback, false));
	return ERROR_NONE;

}

/* Called without the kernel lock - fine */
static unsigned int mc13783_rtc_poll(struct file *file, poll_table * wait)
{
	/*poll_wait(file, &mc13783_rtc_wait, wait); */

	if (mc13783_rtc_done)
		return POLLIN | POLLRDNORM;
	return 0;
}

/*!
 * This function implements IOCTL controls on a mc13783 rtc device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int mc13783_rtc_ioctl(struct inode *inode, struct file *file,
			     unsigned int cmd, unsigned long arg)
{
	struct timeval *mc13783_time = NULL;

	if (_IOC_TYPE(cmd) != 'R')
		return -ENOTTY;

	if (arg) {
		if ((mc13783_time = kmalloc(sizeof(struct timeval), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(mc13783_time, (struct timeval *)arg,
				   sizeof(struct timeval))) {
			return -EFAULT;
		}
	}

	switch (cmd) {
	case MC13783_RTC_SET_TIME:
		TRACEMSG_RTC(_K_D("SET RTC"));
		CHECK_ERROR(mc13783_rtc_set_time(mc13783_time));
		break;
	case MC13783_RTC_GET_TIME:
		TRACEMSG_RTC(_K_D("GET RTC"));
		CHECK_ERROR(mc13783_rtc_get_time(mc13783_time));
		break;
	case MC13783_RTC_SET_ALARM:
		TRACEMSG_RTC(_K_D("SET RTC ALARM"));
		CHECK_ERROR(mc13783_rtc_set_time_alarm(mc13783_time));
		break;
	case MC13783_RTC_GET_ALARM:
		TRACEMSG_RTC(_K_D("GET RTC ALARM"));
		CHECK_ERROR(mc13783_rtc_get_time_alarm(mc13783_time));
		break;
	case MC13783_RTC_WAIT_ALARM:
		TRACEMSG_RTC(_K_I("WAIT ALARM..."));
		CHECK_ERROR(mc13783_rtc_event_sub(RTC_IT_ALARM,
						  callback_test_sub));
		CHECK_ERROR(mc13783_rtc_wait_alarm());
		TRACEMSG_RTC(_K_I("ALARM DONE"));
		CHECK_ERROR(mc13783_rtc_event_unsub(RTC_IT_ALARM,
						    callback_test_sub));
		break;
	case MC13783_RTC_ALARM_REGISTER:
		TRACEMSG_RTC(_K_I("MC13783 RTC ALARM REGISTER"));
		mc13783_event_init(&alarm_event);
		alarm_event.event = EVENT_TODAI;
		alarm_event.callback = callback_alarm_asynchronous;
		CHECK_ERROR(mc13783_event_subscribe(alarm_event));
		break;
	case MC13783_RTC_ALARM_UNREGISTER:
		TRACEMSG_RTC(_K_I("MC13783 RTC ALARM UNREGISTER"));
		mc13783_event_init(&alarm_event);
		alarm_event.event = EVENT_TODAI;
		alarm_event.callback = callback_alarm_asynchronous;
		CHECK_ERROR(mc13783_event_unsubscribe(alarm_event));
		mc13783_rtc_done = false;
		break;
	default:
		TRACEMSG_RTC(_K_D("%d unsupported ioctl command"), (int)cmd);
		return -EINVAL;
	}

	if (arg) {
		if (copy_to_user((struct timeval *)arg, mc13783_time,
				 sizeof(struct timeval))) {
			return -EFAULT;
		}
		kfree(mc13783_time);
	}

	return ERROR_NONE;
}

/*!
 * This function implements the open method on a mc13783 rtc device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_rtc_open(struct inode *inode, struct file *file)
{
	return ERROR_NONE;
}

/*!
 * This function implements the release method on a mc13783 rtc device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_rtc_release(struct inode *inode, struct file *file)
{
	return ERROR_NONE;
}

/*!
 * This function is called to put the rtc in a low power state.
 * There is no need for power handlers for the rtc device.
 * The rtc cannot be suspended.
 *
 * @param   dev   the device structure used to give information on which rtc
 *                device (0 through 3 channels) to suspend
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mc13783_rtc_suspend(struct device *dev, u32 state, u32 level)
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
 * This function is called to resume the rtc from a low power state.
 *
 * @param   dev   the device structure used to give information on which rtc
 *                device (0 through 3 channels) to suspend
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mc13783_rtc_resume(struct device *dev, u32 level)
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
static struct device_driver mc13783_rtc_driver_ldm = {
	.name = "mc13783_rtc",
	.bus = &platform_bus_type,
	.suspend = mc13783_rtc_suspend,
	.resume = mc13783_rtc_resume,
};

/*!
 * This is platform device structure for adding rtc
 */
static struct platform_device mc13783_rtc_ldm = {
	.name = "mc13783_rtc",
	.id = 1,
};

static struct file_operations mc13783_rtc_fops = {
	.owner = THIS_MODULE,
	.ioctl = mc13783_rtc_ioctl,
	.poll = mc13783_rtc_poll,
	.open = mc13783_rtc_open,
	.release = mc13783_rtc_release,
};

int mc13783_rtc_loaded(void)
{
	return mc13783_rtc_detected;
}

/*
 * Initialization and Exit
 */
static int __init mc13783_rtc_init(void)
{
	int ret = 0;

	if (mc13783_core_loaded() == 0) {
		printk(KERN_INFO MC13783_LOAD_ERROR_MSG);
		return -1;
	}

	mc13783_rtc_major =
	    register_chrdev(0, "mc13783_rtc", &mc13783_rtc_fops);
	if (mc13783_rtc_major < 0) {
		TRACEMSG_RTC(_K_D("Unable to get a major for mc13783_rtc"));
		return -1;
	}

	devfs_mk_cdev(MKDEV(mc13783_rtc_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "mc13783_rtc");

	ret = driver_register(&mc13783_rtc_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&mc13783_rtc_ldm);
		if (ret != 0) {
			driver_unregister(&mc13783_rtc_driver_ldm);
		} else {
			mc13783_rtc_detected = 1;
			printk(KERN_INFO "mc13783 RTC loaded\n");
		}
	}

	return ret;
}

static void __exit mc13783_rtc_exit(void)
{
	devfs_remove("mc13783_rtc");
	unregister_chrdev(mc13783_rtc_major, "mc13783_rtc");
	printk(KERN_INFO "mc13783_rtc : successfully unloaded");
}

/*
 * Module entry points
 */

subsys_initcall(mc13783_rtc_init);
module_exit(mc13783_rtc_exit);

MODULE_DESCRIPTION("mc13783_rtc driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
