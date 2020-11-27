/*
 * linux/drivers/char/mxc_rtc.c
 *
 * Freescale MXC Real Time Clock interface for Linux
 *
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright(C) 2006-2007 Motorola, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Date     Author    Comment
 * 10/2006  Motorola  Added power management support
 *                    Added placeholder for bug in setting alarm more than
 *                    one day in the future
 * 01/2007  Motorola  Moved power management support to rtc_sw driver.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <asm/rtc.h>
#include <asm/mach/time.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/mpm.h>
#include <asm/hardware.h>
#include "mxc_rtc.h"

/*!
 * @file mxc_rtc.c
 * @brief Real Time Clock interface for Linux
 *
 * This file contains Real Time Clock interface for Linux.
 *
 * @ingroup Timers
 */

#define RTC_VERSION		"1.0"

static u32 rtc_freq = 2;	/* minimun value for PIE */
static unsigned long rtc_status;

static struct rtc_time g_rtc_alarm = {
	.tm_year = 0,
	.tm_mon = 0,
	.tm_mday = 0,
	.tm_hour = 0,
	.tm_mon = 0,
	.tm_sec = 0,
};

static DEFINE_SPINLOCK(rtc_lock);

/*!
 * This function is used to obtain the RTC time or the alarm value in
 * second.
 *
 * @param  time_alarm   use MXC_RTC_TIME for RTC time value; MXC_RTC_ALARM for alarm value
 *
 * @return The RTC time or alarm time in second.
 */
static u32 get_alarm_or_time(int time_alarm)
{
	u32 day, hr, min, sec, hr_min;

	if (time_alarm == MXC_RTC_TIME) {
		day = *_reg_RTC_DAYR;
		hr_min = *_reg_RTC_HOURMIN;
		sec = *_reg_RTC_SECOND;
	} else if (time_alarm == MXC_RTC_ALARM) {
		day = *_reg_RTC_DAYALARM;
		hr_min = (0x0000FFFF) & (*_reg_RTC_ALRM_HM);
		sec = *_reg_RTC_ALRM_SEC;
	} else {
		panic("wrong value for time_alarm=%d\n", time_alarm);
	}

	hr = hr_min >> 8;
	min = hr_min & 0x00FF;

	return ((((day * 24 + hr) * 60) + min) * 60 + sec);
}

/*!
 * This function sets the RTC alarm value or the time value.
 *
 * @param  time_alarm   the new alarm value to be updated in the RTC
 * @param  time         use MXC_RTC_TIME for RTC time value; MXC_RTC_ALARM for alarm value
 */
static void set_alarm_or_time(int time_alarm, u32 time)
{
	u32 day, hr, min, sec, temp;

	day = time / 86400;
	time -= day * 86400;
	/* time is within a day now */
	hr = time / 3600;
	time -= hr * 3600;
	/* time is within an hour now */
	min = time / 60;
	sec = time - min * 60;

	temp = (hr << 8) + min;

	if (time_alarm == MXC_RTC_TIME) {
		*_reg_RTC_DAYR = day;
		*_reg_RTC_SECOND = sec;
		*_reg_RTC_HOURMIN = temp;
	} else if (time_alarm == MXC_RTC_ALARM) {
		*_reg_RTC_DAYALARM = day;
		*_reg_RTC_ALRM_SEC = sec;
		*_reg_RTC_ALRM_HM = temp;
	} else {
		panic("wrong value for time_alarm=%d\n", time_alarm);
	}
}

/*!
 * This function updates the RTC alarm registers and then clears all the
 * interrupt status bits.
 *
 * @param  alrm         the new alarm value to be updated in the RTC
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int rtc_update_alarm(struct rtc_time *alrm)
{
	struct rtc_time alarm_tm, now_tm;
	unsigned long now, time;
	int ret;

#ifdef CONFIG_MOT_WFN419
        /* The fix we made for this WSAN was made in arch/arm/common/rtctime.c.
           But we suspect that the eventual fix we get through Montavista will
           be made in this function. This function needs to allow the setting
           of alarms for more than a day in the future. */
#endif

	now = get_alarm_or_time(MXC_RTC_TIME);
	rtc_time_to_tm(now, &now_tm);
	rtc_next_alarm_time(&alarm_tm, &now_tm, alrm);
	ret = rtc_tm_to_time(&alarm_tm, &time);

	/* clear all the interrupt status bits */
	*_reg_RTC_RTCISR = *_reg_RTC_RTCISR;

	set_alarm_or_time(MXC_RTC_ALARM, time);

	return ret;
}

/*!
 * This function is the RTC interrupt service routine.
 *
 * @param  irq          RTC IRQ number
 * @param  dev_id       device ID which is not used
 * @param  regs         pointer to a structure containing the processor
 *                      registers and state prior to servicing the interrupt.
 *                      It is not used in this function.
 *
 * @return IRQ_HANDLED as defined in the include/linux/interrupt.h file.
 */
static irqreturn_t rtc_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	u32 status;
	u32 events = 0;

	spin_lock(&rtc_lock);

        mpm_handle_ioi();
	status = (*_reg_RTC_RTCISR) & (*_reg_RTC_RTCIENR);
	/* clear interrupt sources */
	*_reg_RTC_RTCISR = status;

	/* clear alarm interrupt if it has occurred */
	if (status & RTC_ALM_BIT) {
		status &= ~RTC_ALM_BIT;
	}

	/* update irq data & counter */
	if (status & RTC_ALM_BIT) {
		events |= (RTC_AF | RTC_IRQF);
	}
	if (status & RTC_1HZ_BIT) {
		events |= (RTC_UF | RTC_IRQF);
	}
	if (status & PIT_ALL_ON) {
		events |= (RTC_PF | RTC_IRQF);
	}

	rtc_update(1, events);

	if ((status & RTC_ALM_BIT) && rtc_periodic_alarm(&g_rtc_alarm)) {
		rtc_update_alarm(&g_rtc_alarm);
	}

	spin_unlock(&rtc_lock);

	return IRQ_HANDLED;
}

/*!
 * This function is used to open the RTC driver by registering the RTC
 * interrupt service routine.
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_open(void)
{
	if (test_and_set_bit(1, &rtc_status))
		return -EBUSY;
	return 0;
}

/*!
 * clear all interrupts and release the IRQ
 */
static void mxc_rtc_release(void)
{
	spin_lock_irq(&rtc_lock);
	*_reg_RTC_RTCIENR = 0;	/* Disable all rtc interrupts */
	*_reg_RTC_RTCISR = 0xFFFFFFFF;	/* Clear all interrupt status */
	spin_unlock_irq(&rtc_lock);
	rtc_status = 0;
}

/*!
 * This function is used to support some ioctl calls directly.
 * Other ioctl calls are supported indirectly through the
 * arm/common/rtctime.c file.
 *
 * @param  cmd          ioctl command as defined in include/linux/rtc.h
 * @param  arg          value for the ioctl command
 *
 * @return  0 if successful or negative value otherwise.
 */
static int mxc_rtc_ioctl(unsigned int cmd, unsigned long arg)
{
	int i;

	switch (cmd) {
	case RTC_PIE_OFF:
		*_reg_RTC_RTCIENR &= ~PIT_ALL_ON;
		return 0;
	case RTC_IRQP_SET:
		if (arg < 2 || arg > MAX_PIE_FREQ || (arg % 2) != 0)
			return -EINVAL;	/* Also make sure a power of 2Hz */
		if ((arg > 64) && (!capable(CAP_SYS_RESOURCE)))
			return -EACCES;
		rtc_freq = arg;
		return 0;
	case RTC_IRQP_READ:
		return put_user(rtc_freq, (u32 *) arg);
	case RTC_PIE_ON:
		for (i = 0; i < MAX_PIE_NUM; i++) {
			if (PIE_BIT_DEF[i][0] == rtc_freq) {
				break;
			}
		}
		if (i == MAX_PIE_NUM) {
			return -EACCES;
		}
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR |= PIE_BIT_DEF[i][1];
		spin_unlock_irq(&rtc_lock);
		return 0;
	case RTC_AIE_OFF:
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR &= ~RTC_ALM_BIT;
		spin_unlock_irq(&rtc_lock);
		return 0;

	case RTC_AIE_ON:
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR |= RTC_ALM_BIT;
		spin_unlock_irq(&rtc_lock);
		return 0;

	case RTC_UIE_OFF:	/* UIE is for the 1Hz interrupt */
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR &= ~RTC_1HZ_BIT;
		spin_unlock_irq(&rtc_lock);
		return 0;

	case RTC_UIE_ON:
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR |= RTC_1HZ_BIT;
		spin_unlock_irq(&rtc_lock);
		return 0;
	}

	return -EINVAL;
}

/*!
 * This function reads the current RTC time into tm in Gregorian date.
 *
 * @param  tm           contains the RTC time value upon return
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_read_time(struct rtc_time *tm)
{
	u32 val;

	/* Avoid roll-over from reading the different registers */
	do {
		val = get_alarm_or_time(MXC_RTC_TIME);
	} while (val != get_alarm_or_time(MXC_RTC_TIME));

	rtc_time_to_tm(val, tm);

	return 0;
}

/*!
 * This function sets the internal RTC time based on tm in Gregorian date.
 *
 * @param  tm           the time value to be set in the RTC
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_set_time(struct rtc_time *tm)
{
	unsigned long time;
	int ret;

	ret = rtc_tm_to_time(tm, &time);
	if (ret == 0) {
		/* Avoid roll-over from reading the different registers */
		do {
			set_alarm_or_time(MXC_RTC_TIME, time);
		} while (time != get_alarm_or_time(MXC_RTC_TIME));
	}
	return ret;
}

/*!
 * This function is the actual implementation of set_rtc() which is used
 * by programs like NTPD to sync the internal RTC with some external clock
 * source.
 *
 * @return 0 if successful; non-zero if there is pending alarm.
 */
static int mxc_set_rtc(void)
{
	unsigned long current_time = xtime.tv_sec;
	struct rtc_time temp_time;

	if ((*_reg_RTC_RTCIENR & RTC_ALM_BIT) != 0) {
		/* make sure not to forward the clock over an alarm */
		unsigned long alarm = get_alarm_or_time(MXC_RTC_ALARM);
		if (current_time >= alarm &&
		    alarm >= get_alarm_or_time(MXC_RTC_TIME))
			return -ERESTARTSYS;
	}

	rtc_time_to_tm(current_time, &temp_time);

	return mxc_rtc_set_time(&temp_time);
}

/*!
 * This function reads the current alarm value into the passed in \b alrm
 * argument. It updates the \b alrm's pending field value based on the whether
 * an alarm interrupt occurs or not.
 *
 * @param  alrm         contains the RTC alarm value upon return
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_read_alarm(struct rtc_wkalrm *alrm)
{
	rtc_time_to_tm(get_alarm_or_time(MXC_RTC_ALARM), &alrm->time);
	alrm->pending = (((*_reg_RTC_RTCISR) & RTC_ALM_BIT) != 0) ? 1 : 0;

	return 0;
}

/*!
 * This function sets the RTC alarm based on passed in alrm.
 *
 * @param  alrm         the alarm value to be set in the RTC
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_set_alarm(struct rtc_wkalrm *alrm)
{
	struct rtc_time atm, ctm;
	int ret;

	spin_lock_irq(&rtc_lock);
	if (rtc_periodic_alarm(&alrm->time)) {
		if (alrm->time.tm_sec > 59 ||
		    alrm->time.tm_hour > 23 ||
		    alrm->time.tm_min > 59) {
			ret = -EINVAL;
			goto out;
		}
		mxc_rtc_read_time(&ctm);
		rtc_next_alarm_time(&atm, &ctm, &alrm->time);
		ret = rtc_update_alarm(&atm);
	} else {
		if ((ret = rtc_valid_tm(&alrm->time)))
			goto out;
		ret = rtc_update_alarm(&alrm->time);
	}
	if (ret == 0) {
		memcpy(&g_rtc_alarm, &alrm->time, sizeof(struct rtc_time));

		if (alrm->enabled) {
			*_reg_RTC_RTCIENR |= RTC_ALM_BIT;
		} else {
			*_reg_RTC_RTCIENR &= ~RTC_ALM_BIT;
		}
	}
out:
	spin_unlock_irq(&rtc_lock);

	return ret;
}

/*!
 * This function is used to provide the content for the /proc/driver/rtc
 * file.
 *
 * @param  buf          the buffer to hold the information that the driver wants to write
 *
 * @return  The number of bytes written into the rtc file.
 */
static int mxc_rtc_proc(char *buf)
{
	char *p = buf;

	p += sprintf(p, "alarm_IRQ\t: %s\n",
		     (((*_reg_RTC_RTCIENR) & RTC_ALM_BIT) != 0) ? "yes" : "no");
	p += sprintf(p, "update_IRQ\t: %s\n",
		     (((*_reg_RTC_RTCIENR) & RTC_1HZ_BIT) != 0) ? "yes" : "no");
	p += sprintf(p, "periodic_IRQ\t: %s\n",
		     (((*_reg_RTC_RTCIENR) & PIT_ALL_ON) != 0) ? "yes" : "no");
	p += sprintf(p, "periodic_freq\t: %d\n", rtc_freq);

	return p - buf;
}

/*!
 * The RTC driver structure
 */
static struct rtc_ops mxc_rtc_ops = {
	.owner = THIS_MODULE,
	.open = mxc_rtc_open,
	.release = mxc_rtc_release,
	.ioctl = mxc_rtc_ioctl,
	.read_time = mxc_rtc_read_time,
	.set_time = mxc_rtc_set_time,
	.read_alarm = mxc_rtc_read_alarm,
	.set_alarm = mxc_rtc_set_alarm,
	.proc = mxc_rtc_proc,
};

/*!
 * This function returns the current RTC seconds counter value
 *
 * @return RTC seconds
 */
static inline u32 mxc_get_rtc_time(void)
{
	return *_reg_RTC_SECOND;
}

/*! MXC RTC Power management control */
#ifndef CONFIG_MOT_FEAT_PM
static struct timespec mxc_rtc_delta;
#endif

/*!
 * This function is called to save the system time delta relative to
 * the MXC RTC when enterring a low power state. This time delta is
 * then used on resume to adjust the system time to account for time
 * loss while suspended.
 *
 * @param   dev   not used
 * @param   state Power state to enter.
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_rtc_suspend(struct device *dev, u32 state, u32 level)
{
#ifndef CONFIG_MOT_FEAT_PM
	struct timespec tv;

	if (level == SUSPEND_POWER_DOWN) {
		/* calculate time delta for suspend */
		tv.tv_nsec = 0;
		tv.tv_sec = get_alarm_or_time(MXC_RTC_TIME);
		save_time_delta(&mxc_rtc_delta, &tv);
	}

	return 0;
#else
        /* Functionality here has been moved to the RTC SW driver */
	return 0;
#endif
}

/*!
 * This function is called to correct the system time based on the
 * current MXC RTC time relative to the time delta saved during
 * suspend.
 *
 * @param   dev   not used
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_rtc_resume(struct device *dev, u32 level)
{
#ifndef CONFIG_MOT_FEAT_PM
	struct timespec tv;

	switch (level) {
	case RESUME_POWER_ON:
	case RESUME_RESTORE_STATE:
		/* do nothing */
		break;
	case RESUME_ENABLE:
		tv.tv_nsec = 0;
		tv.tv_sec = get_alarm_or_time(MXC_RTC_TIME);
		restore_time_delta(&mxc_rtc_delta, &tv);
		break;
	}
	return 0;
#else
        /* Functionality here has been moved to the RTC SW driver */
        return 0;
#endif
}

/*!
 * Contains pointers to the power management callback functions.
 */
static struct device_driver mxc_rtc_driver = {
	.name = "mxc_rtc",
	.bus = &platform_bus_type,
	.suspend = mxc_rtc_suspend,
	.resume = mxc_rtc_resume,
};

/*!
 * This is platform device structure for adding RTC
 */
static struct platform_device mxc_rtc_device = {
	.name = "mxc_rtc",
	.id = 0,
};

/*!
 * This function creates the /proc/driver/rtc file and registers the device RTC
 * in the /dev/misc directory. It also reads the RTC value from external source
 * and setup the internal RTC properly.
 *
 * @return  -1 if RTC is failed to initialize; 0 is successful.
 */
static int __init mxc_rtc_init(void)
{
	struct rtc_time;
	struct timespec tv;
	int ret;

	mxc_clks_enable(RTC_CLK);

	/* Configure and enable the RTC */
	*_reg_RTC_RTCCTL = RTC_INPUT_CLK | RTC_ENABLE_BIT;
	if (((*_reg_RTC_RTCCTL) & RTC_ENABLE_BIT) == 0) {
		printk(KERN_ERR "rtc: hardware module can't be enabled!\n");
		return -1;
	}

	if ((ret = request_irq(INT_RTC, rtc_interrupt, 0, "rtc", NULL)) != 0) {
		printk(KERN_ERR "rtc: IRQ%d already in use.\n", INT_RTC);
		return ret;
	}

	register_rtc(&mxc_rtc_ops);

	tv.tv_nsec = 0;
	tv.tv_sec = get_alarm_or_time(MXC_RTC_TIME);
	do_settimeofday(&tv);

	ret = driver_register(&mxc_rtc_driver);
	if (ret == 0) {
		ret = platform_device_register(&mxc_rtc_device);
		if (ret != 0) {
			driver_unregister(&mxc_rtc_driver);
		}
	}

	/* do it last to insure everything loaded ok */
	set_rtc = mxc_set_rtc;

	printk("Real Time Clock Driver v%s\n", RTC_VERSION);

	return ret;
}

/*!
 * This function removes the /proc/driver/rtc file and un-registers the
 * device RTC from the /dev/misc directory.
 */
static void __exit mxc_rtc_exit(void)
{
	unregister_rtc(&mxc_rtc_ops);
	free_irq(INT_RTC, NULL);
	driver_unregister(&mxc_rtc_driver);
	platform_device_unregister(&mxc_rtc_device);
	mxc_rtc_release();
}

module_init(mxc_rtc_init);
module_exit(mxc_rtc_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Realtime Clock Driver (RTC)");
MODULE_LICENSE("GPL");
