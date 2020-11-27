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
 * @file pmic_rtc.c
 * @brief This is the main file of PMIC RTC driver.
 *
 * @ingroup PMIC_RTC
 */

/*
 * Includes
 */
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/device.h>

#include "../core/pmic_config.h"
#include "pmic_rtc_defs.h"
#include "asm/arch/pmic_status.h"
#include "asm/arch/pmic_rtc.h"

#define PMIC_LOAD_ERROR_MSG		\
"Pmic card was not correctly detected. Stop loading Pmic RTC driver\n"

/*
 * Global variables
 */
static int pmic_rtc_major;
static void callback_alarm_asynchronous(void);
static void callback_alarm_synchronous(void);
static unsigned int pmic_rtc_poll(struct file *file, poll_table * wait);
static DECLARE_WAIT_QUEUE_HEAD(queue_alarm);
static DECLARE_WAIT_QUEUE_HEAD(pmic_rtc_wait);
static type_event_notification alarm_event;
//VBU
static type_event_notification rtc_event;
static int pmic_rtc_detected = 0;
static bool pmic_rtc_done = 0;

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(pmic_rtc_set_time);
EXPORT_SYMBOL(pmic_rtc_get_time);
EXPORT_SYMBOL(pmic_rtc_set_time_alarm);
EXPORT_SYMBOL(pmic_rtc_get_time_alarm);
EXPORT_SYMBOL(pmic_rtc_wait_alarm);
EXPORT_SYMBOL(pmic_rtc_event_sub);
EXPORT_SYMBOL(pmic_rtc_event_unsub);
EXPORT_SYMBOL(pmic_rtc_loaded);

/*
 * Real Time Clock Pmic API
 */

/*!
 * This is the callback function called on TSI Pmic event, used in asynchronous
 * call.
 */
static void callback_alarm_asynchronous(void)
{
	pmic_rtc_done = true;
}

/*!
 * This is the callback function is used in test code for (un)sub.
 */
static void callback_test_sub(void)
{
	TRACEMSG_ALL_TIME(_K_I("*****************************************"));
	TRACEMSG_ALL_TIME(_K_I("***** Pmic RTC 'Alarm IT CallBack' *****"));
	TRACEMSG_ALL_TIME(_K_I("*****************************************"));
}

/*!
 * This is the callback function called on TSI Pmic event, used in synchronous
 * call.
 */
static void callback_alarm_synchronous(void)
{
	TRACEMSG_ALL_TIME(_K_D("*** Alarm IT Pmic ***"));
	wake_up(&queue_alarm);
}

/*!
 * This function wait the Alarm event
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_rtc_wait_alarm(void)
{
	DEFINE_WAIT(wait);
	pmic_event_init(&alarm_event);
	alarm_event.event = EVENT_TODAI;
	alarm_event.callback = (void *)callback_alarm_synchronous;
	CHECK_ERROR(pmic_event_subscribe(&alarm_event));
	prepare_to_wait(&queue_alarm, &wait, TASK_UNINTERRUPTIBLE);
	schedule();
	finish_wait(&queue_alarm, &wait);
	CHECK_ERROR(pmic_event_unsubscribe(&alarm_event));
	return PMIC_SUCCESS;
}

/*!
 * This function set the real time clock of PMIC
 *
 * @param        pmic_time  	value of date and time
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_rtc_set_time(struct timeval * pmic_time)
{
	unsigned int tod_reg_val = 0;
	unsigned int day_reg_val = 0;
	unsigned int mask, value;

	tod_reg_val = pmic_time->tv_sec % 86400;
	day_reg_val = pmic_time->tv_sec / 86400;

	mask = BITFMASK(MC13783_RTCTIME_TIME);
	value = BITFVAL(MC13783_RTCTIME_TIME, tod_reg_val);
	CHECK_ERROR(pmic_write_reg(PRIO_RTC, REG_RTC_TIME, value, mask));

	mask = BITFMASK(MC13783_RTCDAY_DAY);
	value = BITFVAL(MC13783_RTCDAY_DAY, day_reg_val);
	CHECK_ERROR(pmic_write_reg(PRIO_RTC, REG_RTC_DAY, value, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function get the real time clock of PMIC
 *
 * @param        pmic_time  	return value of date and time
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_rtc_get_time(struct timeval * pmic_time)
{
	unsigned int tod_reg_val = 0;
	unsigned int day_reg_val = 0;
	unsigned int mask, value;

	mask = BITFMASK(MC13783_RTCTIME_TIME);
	CHECK_ERROR(pmic_read_reg(PRIO_RTC, REG_RTC_TIME, &value, mask));
	tod_reg_val = BITFEXT(value, MC13783_RTCTIME_TIME);

	mask = BITFMASK(MC13783_RTCDAY_DAY);
	CHECK_ERROR(pmic_read_reg(PRIO_RTC, REG_RTC_DAY, &value, mask));
	day_reg_val = BITFEXT(value, MC13783_RTCDAY_DAY);

	pmic_time->tv_sec = (unsigned long)((unsigned long)(tod_reg_val &
							    0x0001FFFF) +
					    (unsigned long)(day_reg_val *
							    86400));
	return PMIC_SUCCESS;
}

/*!
 * This function set the real time clock alarm of PMIC
 *
 * @param        pmic_time  	value of date and time
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_rtc_set_time_alarm(struct timeval * pmic_time)
{
	unsigned int tod_reg_val = 0;
	unsigned int day_reg_val = 0;
	unsigned int mask, value;

	tod_reg_val = pmic_time->tv_sec % 86400;
	day_reg_val = pmic_time->tv_sec / 86400;

	mask = BITFMASK(MC13783_RTCALARM_TIME);
	value = BITFVAL(MC13783_RTCALARM_TIME, tod_reg_val);
	CHECK_ERROR(pmic_write_reg(PRIO_RTC, REG_RTC_ALARM, value, mask));

	mask = BITFMASK(MC13783_RTCALARM_DAY);
	value = BITFVAL(MC13783_RTCALARM_DAY, day_reg_val);
	CHECK_ERROR(pmic_write_reg(PRIO_RTC, REG_RTC_DAY_ALARM, value, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function get the real time clock alarm of PMIC
 *
 * @param        pmic_time  	return value of date and time
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_rtc_get_time_alarm(struct timeval * pmic_time)
{
	unsigned int tod_reg_val = 0;
	unsigned int day_reg_val = 0;
	unsigned int mask, value;

	mask = BITFMASK(MC13783_RTCALARM_TIME);
	CHECK_ERROR(pmic_read_reg(PRIO_RTC, REG_RTC_ALARM, &value, mask));
	tod_reg_val = BITFEXT(value, MC13783_RTCALARM_TIME);

	mask = BITFMASK(MC13783_RTCALARM_DAY);
	CHECK_ERROR(pmic_read_reg(PRIO_RTC, REG_RTC_DAY_ALARM, &value, mask));
	day_reg_val = BITFEXT(value, MC13783_RTCALARM_DAY);

	pmic_time->tv_sec = (unsigned long)((unsigned long)(tod_reg_val &
							    0x0001FFFF) +
					    (unsigned long)(day_reg_val *
							    86400));

	return PMIC_SUCCESS;
}

/*!
 * This function is used to un/subscribe on RTC event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 * @param        sub      	define if Un/subscribe event.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_rtc_event(t_rtc_int event, void *callback, bool sub)
{
//VBU
//        type_event_notification rtc_event;

	pmic_event_init(&rtc_event);
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
		return PMIC_PARAMETER_ERROR;
		break;
	}
	if (sub == true) {
		CHECK_ERROR(pmic_event_subscribe(&rtc_event));
	} else {
		CHECK_ERROR(pmic_event_unsubscribe(&rtc_event));
	}
	return PMIC_SUCCESS;
}

/*!
 * This function is used to subscribe on RTC event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_rtc_event_sub(t_rtc_int event, void *callback)
{
	CHECK_ERROR(pmic_rtc_event(event, callback, true));
	return PMIC_SUCCESS;
}

/*!
 * This function is used to un subscribe on RTC event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_rtc_event_unsub(t_rtc_int event, void *callback)
{
	CHECK_ERROR(pmic_rtc_event(event, callback, false));
	return PMIC_SUCCESS;
}

/* Called without the kernel lock - fine */
static unsigned int pmic_rtc_poll(struct file *file, poll_table * wait)
{
	/*poll_wait(file, &pmic_rtc_wait, wait); */

	if (pmic_rtc_done)
		return POLLIN | POLLRDNORM;
	return 0;
}

/*!
 * This function implements IOCTL controls on a PMIC RTC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int pmic_rtc_ioctl(struct inode *inode, struct file *file,
			  unsigned int cmd, unsigned long arg)
{
	struct timeval *pmic_time = NULL;

	if (_IOC_TYPE(cmd) != 'p')
		return -ENOTTY;

	if (arg) {
		if ((pmic_time = kmalloc(sizeof(struct timeval),
					 GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(pmic_time, (struct timeval *)arg,
				   sizeof(struct timeval))) {
			return -EFAULT;
		}
	}

	switch (cmd) {
	case PMIC_RTC_SET_TIME:
		TRACEMSG_RTC(_K_D("SET RTC"));
		CHECK_ERROR(pmic_rtc_set_time(pmic_time));
		break;
	case PMIC_RTC_GET_TIME:
		TRACEMSG_RTC(_K_D("GET RTC"));
		CHECK_ERROR(pmic_rtc_get_time(pmic_time));
		break;
	case PMIC_RTC_SET_ALARM:
		TRACEMSG_RTC(_K_D("SET RTC ALARM"));
		CHECK_ERROR(pmic_rtc_set_time_alarm(pmic_time));
		break;
	case PMIC_RTC_GET_ALARM:
		TRACEMSG_RTC(_K_D("GET RTC ALARM"));
		CHECK_ERROR(pmic_rtc_get_time_alarm(pmic_time));
		break;
	case PMIC_RTC_WAIT_ALARM:
		TRACEMSG_RTC(_K_I("WAIT ALARM..."));
		CHECK_ERROR(pmic_rtc_event_sub(RTC_IT_ALARM,
					       callback_test_sub));
		CHECK_ERROR(pmic_rtc_wait_alarm());
		TRACEMSG_RTC(_K_I("ALARM DONE"));
		CHECK_ERROR(pmic_rtc_event_unsub(RTC_IT_ALARM,
						 callback_test_sub));
		break;
	case PMIC_RTC_ALARM_REGISTER:
		TRACEMSG_RTC(_K_I("PMIC RTC ALARM REGISTER"));
		pmic_event_init(&alarm_event);
		alarm_event.event = EVENT_TODAI;
		alarm_event.callback = (void *)callback_alarm_asynchronous;
		CHECK_ERROR(pmic_event_subscribe(&alarm_event));
		break;
	case PMIC_RTC_ALARM_UNREGISTER:
		TRACEMSG_RTC(_K_I("PMIC RTC ALARM UNREGISTER"));
		pmic_event_init(&alarm_event);
		alarm_event.event = EVENT_TODAI;
		alarm_event.callback = (void *)callback_alarm_asynchronous;
		CHECK_ERROR(pmic_event_unsubscribe(&alarm_event));
		pmic_rtc_done = false;
		break;
	default:
		TRACEMSG_RTC(_K_D("%d unsupported ioctl command"), (int)cmd);
		return -EINVAL;
	}

	if (arg) {
		if (copy_to_user((struct timeval *)arg, pmic_time,
				 sizeof(struct timeval))) {
			return -EFAULT;
		}
		kfree(pmic_time);
	}

	return 0;
}

/*!
 * This function implements the open method on a PMIC RTC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_rtc_open(struct inode *inode, struct file *file)
{
	return 0;
}

/*!
 * This function implements the release method on a PMIC RTC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_rtc_release(struct inode *inode, struct file *file)
{
	return 0;
}

/*!
 * This function is called to put the RTC in a low power state.
 * There is no need for power handlers for the RTC device.
 * The RTC cannot be suspended.
 *
 * @param   dev   the device structure used to give information on which RTC
 *                device (0 through 3 channels) to suspend
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int pmic_rtc_suspend(struct device *dev, u32 state, u32 level)
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
 * This function is called to resume the RTC from a low power state.
 *
 * @param   dev   the device structure used to give information on which RTC
 *                device (0 through 3 channels) to suspend
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int pmic_rtc_resume(struct device *dev, u32 level)
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
static struct device_driver pmic_rtc_driver_ldm = {
	.name = "Pmic_rtc",
	.bus = &platform_bus_type,
	.suspend = pmic_rtc_suspend,
	.resume = pmic_rtc_resume,
};

/*!
 * This is platform device structure for adding RTC
 */
static struct platform_device pmic_rtc_ldm = {
	.name = "Pmic_rtc",
	.id = 1,
};

static struct file_operations pmic_rtc_fops = {
	.owner = THIS_MODULE,
	.ioctl = pmic_rtc_ioctl,
	.poll = pmic_rtc_poll,
	.open = pmic_rtc_open,
	.release = pmic_rtc_release,
};

int pmic_rtc_loaded(void)
{
	return pmic_rtc_detected;
}

/*
 * Initialization and Exit
 */
static int __init pmic_rtc_init(void)
{
	int ret = 0;

	pmic_rtc_major = register_chrdev(0, "pmic_rtc", &pmic_rtc_fops);
	if (pmic_rtc_major < 0) {
		TRACEMSG_RTC(_K_D("Unable to get a major for pmic_rtc"));
		return -1;
	}

	devfs_mk_cdev(MKDEV(pmic_rtc_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "pmic_rtc");

	ret = driver_register(&pmic_rtc_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&pmic_rtc_ldm);
		if (ret != 0) {
			driver_unregister(&pmic_rtc_driver_ldm);
		} else {
			pmic_rtc_detected = 1;
			printk(KERN_INFO "Pmic RTC loaded\n");
		}
	}

	return ret;
}

static void __exit pmic_rtc_exit(void)
{
	devfs_remove("pmic_rtc");
	unregister_chrdev(pmic_rtc_major, "pmic_rtc");
	printk(KERN_INFO "Pmic_rtc : successfully unloaded");
}

/*
 * Module entry points
 */

subsys_initcall(pmic_rtc_init);
module_exit(pmic_rtc_exit);

MODULE_DESCRIPTION("Pmic_rtc driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
