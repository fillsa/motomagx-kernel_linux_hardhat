/*
 *  linux/drivers/misc/ucb1x00-ts.c
 *
 *  Copyright (C) 2001 Russell King, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * 21-Jan-2002 <jco@ict.es> :
 *
 * Added support for synchronous A/D mode. This mode is useful to
 * avoid noise induced in the touchpanel by the LCD, provided that
 * the UCB1x00 has a valid LCD sync signal routed to its ADCSYNC pin.
 * It is important to note that the signal connected to the ADCSYNC
 * pin should provide pulses even when the LCD is blanked, otherwise
 * a pen touch needed to unblank the LCD will never be read.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/slab.h>

#include <asm/dma.h>
#include <asm/semaphore.h>

#include "ucb1x00.h"


struct ucb1x00_ts {
	struct input_dev	idev;
	struct ucb1x00		*ucb;

	struct completion	irq_wait;
	struct semaphore	sem;
	struct completion	init_exit;
	struct task_struct	*rtask;
	int			use_count;
	u16			x_res;
	u16			y_res;

	int			restart:1;
	int			adcsync:1;
};

static int adcsync = UCB_NOSYNC;

static inline void ucb1x00_ts_evt_add(struct ucb1x00_ts *ts, u16 pressure, u16 x, u16 y)
{
	input_report_abs(&ts->idev, ABS_X, x);
	input_report_abs(&ts->idev, ABS_Y, y);
	input_report_abs(&ts->idev, ABS_PRESSURE, pressure);
	input_sync(&ts->idev);
}

static inline void ucb1x00_ts_event_release(struct ucb1x00_ts *ts)
{
	input_report_abs(&ts->idev, ABS_PRESSURE, 0);
	input_sync(&ts->idev);
}

/*
 * Switch to interrupt mode.
 */
static inline void ucb1x00_ts_mode_int(struct ucb1x00_ts *ts)
{
	int val = UCB_TS_CR_TSMX_POW | UCB_TS_CR_TSPX_POW |
		  UCB_TS_CR_TSMY_GND | UCB_TS_CR_TSPY_GND |
		  UCB_TS_CR_MODE_INT;
	if (ts->ucb->id == UCB_ID_1400_BUGGY)
		val &= ~(UCB_TS_CR_TSMX_POW | UCB_TS_CR_TSPX_POW);
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR, val);
}

/*
 * Switch to pressure mode, and read pressure.  We don't need to wait
 * here, since both plates are being driven.
 */
static inline unsigned int ucb1x00_ts_read_pressure(struct ucb1x00_ts *ts)
{
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR,
			UCB_TS_CR_TSMX_POW | UCB_TS_CR_TSPX_POW |
			UCB_TS_CR_TSMY_GND | UCB_TS_CR_TSPY_GND |
			UCB_TS_CR_MODE_PRES | UCB_TS_CR_BIAS_ENA);

	return ucb1x00_adc_read(ts->ucb, UCB_ADC_INP_TSPY, ts->adcsync);
}

/*
 * Switch to X position mode and measure Y plate.  We switch the plate
 * configuration in pressure mode, then switch to position mode.  This
 * gives a faster response time.  Even so, we need to wait about 55us
 * for things to stabilise.
 */
static inline unsigned int ucb1x00_ts_read_xpos(struct ucb1x00_ts *ts)
{
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR,
			UCB_TS_CR_TSMX_GND | UCB_TS_CR_TSPX_POW |
			UCB_TS_CR_MODE_PRES | UCB_TS_CR_BIAS_ENA);
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR,
			UCB_TS_CR_TSMX_GND | UCB_TS_CR_TSPX_POW |
			UCB_TS_CR_MODE_PRES | UCB_TS_CR_BIAS_ENA);
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR,
			UCB_TS_CR_TSMX_GND | UCB_TS_CR_TSPX_POW |
			UCB_TS_CR_MODE_POS | UCB_TS_CR_BIAS_ENA);

	udelay(55);

	return ucb1x00_adc_read(ts->ucb, UCB_ADC_INP_TSPY, ts->adcsync);
}

/*
 * Switch to Y position mode and measure X plate.  We switch the plate
 * configuration in pressure mode, then switch to position mode.  This
 * gives a faster response time.  Even so, we need to wait about 55us
 * for things to stabilise.
 */
static inline unsigned int ucb1x00_ts_read_ypos(struct ucb1x00_ts *ts)
{
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR,
			UCB_TS_CR_TSMY_GND | UCB_TS_CR_TSPY_POW |
			UCB_TS_CR_MODE_PRES | UCB_TS_CR_BIAS_ENA);
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR,
			UCB_TS_CR_TSMY_GND | UCB_TS_CR_TSPY_POW |
			UCB_TS_CR_MODE_PRES | UCB_TS_CR_BIAS_ENA);
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR,
			UCB_TS_CR_TSMY_GND | UCB_TS_CR_TSPY_POW |
			UCB_TS_CR_MODE_POS | UCB_TS_CR_BIAS_ENA);

	udelay(55);

	return ucb1x00_adc_read(ts->ucb, UCB_ADC_INP_TSPX, ts->adcsync);
}

/*
 * Switch to X plate resistance mode.  Set MX to ground, PX to
 * supply.  Measure current.
 */
static inline unsigned int ucb1x00_ts_read_xres(struct ucb1x00_ts *ts)
{
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR,
			UCB_TS_CR_TSMX_GND | UCB_TS_CR_TSPX_POW |
			UCB_TS_CR_MODE_PRES | UCB_TS_CR_BIAS_ENA);
	return ucb1x00_adc_read(ts->ucb, 0, ts->adcsync);
}

/*
 * Switch to Y plate resistance mode.  Set MY to ground, PY to
 * supply.  Measure current.
 */
static inline unsigned int ucb1x00_ts_read_yres(struct ucb1x00_ts *ts)
{
	ucb1x00_reg_write(ts->ucb, UCB_TS_CR,
			UCB_TS_CR_TSMY_GND | UCB_TS_CR_TSPY_POW |
			UCB_TS_CR_MODE_PRES | UCB_TS_CR_BIAS_ENA);
	return ucb1x00_adc_read(ts->ucb, 0, ts->adcsync);
}

/*
 * This is a RT kernel thread that handles the ADC accesses
 * (mainly so we can use semaphores in the UCB1200 core code
 * to serialise accesses to the ADC).  The UCB1400 access
 * functions are expected to be able to sleep as well.
 */
static int ucb1x00_thread(void *_ts)
{
	struct ucb1x00_ts *ts = _ts;
	struct task_struct *tsk = current;
	int valid;

	ts->rtask = tsk;

	daemonize("ktsd");
	/* only want to receive SIGKILL */
	allow_signal(SIGKILL);

	/*
	 * We could run as a real-time thread.  However, thus far
	 * this doesn't seem to be necessary.
	 */
//	tsk->policy = SCHED_FIFO;
//	tsk->rt_priority = 1;

	complete(&ts->init_exit);

	valid = 0;

	for (;;) {
		unsigned int x, y, p, val;

		ts->restart = 0;

		ucb1x00_adc_enable(ts->ucb);

		x = ucb1x00_ts_read_xpos(ts);
		y = ucb1x00_ts_read_ypos(ts);
		p = ucb1x00_ts_read_pressure(ts);

		/*
		 * Switch back to interrupt mode.
		 */
		ucb1x00_ts_mode_int(ts);
		ucb1x00_adc_disable(ts->ucb);

		set_task_state(tsk, TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ / 100);
		if (signal_pending(tsk))
			break;

		ucb1x00_enable(ts->ucb);
		val = ucb1x00_reg_read(ts->ucb, UCB_TS_CR);

		if (val & (UCB_TS_CR_TSPX_LOW | UCB_TS_CR_TSMX_LOW)) {
			ucb1x00_enable_irq(ts->ucb, UCB_IRQ_TSPX, UCB_FALLING);
			ucb1x00_disable(ts->ucb);

			/*
			 * If we spat out a valid sample set last time,
			 * spit out a "pen off" sample here.
			 */
			if (valid) {
				ucb1x00_ts_event_release(ts);
				valid = 0;
			}

			/*
			 * Since ucb1x00_enable_irq() might sleep due
			 * to the way the UCB1400 regs are accessed, we
			 * can't use set_task_state() before that call,
			 * and not changing state before enabling the
			 * interrupt is racy.
			 */
			wait_for_completion_interruptible(&ts->irq_wait);
		} else {
			ucb1x00_disable(ts->ucb);

			/*
			 * Filtering is policy.  Policy belongs in user
			 * space.  We therefore leave it to user space
			 * to do any filtering they please.
			 */
			if (!ts->restart) {
				ucb1x00_ts_evt_add(ts, p, x, y);
				valid = 1;
			}

			set_task_state(tsk, TASK_INTERRUPTIBLE);
			schedule_timeout(HZ / 100);
		}

		if (signal_pending(tsk))
			break;
	}

	ts->rtask = NULL;
	complete_and_exit(&ts->init_exit, 0);
}

/*
 * We only detect touch screen _touches_ with this interrupt
 * handler, and even then we just schedule our task.
 */
static void ucb1x00_ts_irq(int idx, void *id)
{
	struct ucb1x00_ts *ts = id;
	ucb1x00_disable_irq(ts->ucb, UCB_IRQ_TSPX, UCB_FALLING);
	complete(&ts->irq_wait);
}

static int ucb1x00_ts_open(struct input_dev *idev)
{
	struct ucb1x00_ts *ts = (struct ucb1x00_ts *)idev;
	int ret = 0;

	if (down_interruptible(&ts->sem))
		return -EINTR;

	if (ts->use_count++ != 0)
		goto out;

	if (ts->rtask)
		panic("ucb1x00: rtask running?");

	init_completion(&ts->irq_wait);
	ret = ucb1x00_hook_irq(ts->ucb, UCB_IRQ_TSPX, ucb1x00_ts_irq, ts);
	if (ret < 0)
		goto out;

	/*
	 * If we do this at all, we should allow the user to
	 * measure and read the X and Y resistance at any time.
	 */
	ucb1x00_adc_enable(ts->ucb);
	ts->x_res = ucb1x00_ts_read_xres(ts);
	ts->y_res = ucb1x00_ts_read_yres(ts);
	ucb1x00_adc_disable(ts->ucb);

	init_completion(&ts->init_exit);
	ret = kernel_thread(ucb1x00_thread, ts, CLONE_KERNEL);
	if (ret >= 0) {
		wait_for_completion(&ts->init_exit);
		ret = 0;
	} else {
		ucb1x00_free_irq(ts->ucb, UCB_IRQ_TSPX, ts);
	}

 out:
	if (ret)
		ts->use_count--;
	up(&ts->sem);
	return ret;
}

/*
 * Release touchscreen resources.  Disable IRQs.
 */
static void ucb1x00_ts_close(struct input_dev *idev)
{
	struct ucb1x00_ts *ts = (struct ucb1x00_ts *)idev;

	down(&ts->sem);
	if (--ts->use_count == 0) {
		if (ts->rtask) {
			send_sig(SIGKILL, ts->rtask, 1);
			wait_for_completion(&ts->init_exit);
		}

		ucb1x00_enable(ts->ucb);
		ucb1x00_free_irq(ts->ucb, UCB_IRQ_TSPX, ts);
		ucb1x00_reg_write(ts->ucb, UCB_TS_CR, 0);
		ucb1x00_disable(ts->ucb);
	}
	up(&ts->sem);
}

#if 0
static int ucb1x00_ts_resume(struct device *_dev, u32 level)
{
	struct ucb1x00_device *dev = ucb1x00_dev(_dev);
	struct ucb1x00_ts *ts = ucb1x00_get_drvdata(dev);

	if (level == RESUME_ENABLE && ts->rtask != NULL) {
		/*
		 * Restart the TS thread to ensure the
		 * TS interrupt mode is set up again
		 * after sleep.
		 */
		ts->restart = 1;
		complete(&ts->irq_wait);
	}
	return 0;
}
#endif


/*
 * Initialisation.
 */
static int ucb1x00_ts_add(struct class_device *dev)
{
	struct ucb1x00 *ucb = classdev_to_ucb1x00(dev);
	struct ucb1x00_ts *ts;

	ts = kmalloc(sizeof(struct ucb1x00_ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	memset(ts, 0, sizeof(struct ucb1x00_ts));

	ts->ucb = ucb;
	ts->adcsync = adcsync;
	init_MUTEX(&ts->sem);

	ts->idev.name       = "Touchscreen panel";
	ts->idev.id.product = ts->ucb->id;
	ts->idev.open       = ucb1x00_ts_open;
	ts->idev.close      = ucb1x00_ts_close;

	__set_bit(EV_ABS, ts->idev.evbit);
	__set_bit(ABS_X, ts->idev.absbit);
	__set_bit(ABS_Y, ts->idev.absbit);
	__set_bit(ABS_PRESSURE, ts->idev.absbit);

	input_register_device(&ts->idev);

	ucb->ts_data = ts;

	return 0;
}

static void ucb1x00_ts_remove(struct class_device *dev)
{
	struct ucb1x00 *ucb = classdev_to_ucb1x00(dev);
	struct ucb1x00_ts *ts = ucb->ts_data;

	input_unregister_device(&ts->idev);
	kfree(ts);
}

static struct class_interface ucb1x00_ts_interface = {
	.add		= ucb1x00_ts_add,
	.remove		= ucb1x00_ts_remove,
};

static int __init ucb1x00_ts_init(void)
{
	return ucb1x00_register_interface(&ucb1x00_ts_interface);
}

static void __exit ucb1x00_ts_exit(void)
{
	ucb1x00_unregister_interface(&ucb1x00_ts_interface);
}

#ifndef MODULE

/*
 * Parse kernel command-line options.
 *
 * syntax : ucbts=[sync|nosync],...
 */
static int __init ucb1x00_ts_setup(char *str)
{
	char *p;

	while ((p = strsep(&str, ",")) != NULL) {
		if (strcmp(p, "sync") == 0)
			adcsync = UCB_SYNC;
	}

	return 1;
}

__setup("ucbts=", ucb1x00_ts_setup);

#else

MODULE_PARM(adcsync, "i");
MODULE_PARM_DESC(adcsync, "Enable use of ADCSYNC signal");

#endif

module_init(ucb1x00_ts_init);
module_exit(ucb1x00_ts_exit);

MODULE_AUTHOR("Russell King <rmk@arm.linux.org.uk>");
MODULE_DESCRIPTION("UCB1x00 touchscreen driver");
MODULE_LICENSE("GPL");
