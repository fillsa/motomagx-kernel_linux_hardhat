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
 * @file mc13783_adc.c
 * @brief This is the main file of mc13783 adc driver.
 *
 * @ingroup MC13783_ADC
 */

#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/device.h>
#include <linux/delay.h>

#include "../core/mc13783_config.h"
#include "../core/mc13783_event.h"

#include "mc13783_adc.h"
#include "mc13783_adc_defs.h"

static int mc13783_adc_major;
static int buf_size = 4096;	/* Buffer size */
static int mc13783_adc_detected = 0;

/* internal function */
static void callback_tsi(void);
static void callback_adcdone(void);
static void callback_adcbisdone(void);

/*!
 * Number of users waiting in suspendq
 */
static int swait = 0;

/*!
 * To indicate whether any of the adc devices are suspending
 */
static int suspend_flag = 0;

/*!
 * The suspendq is used by blocking application calls
 */
static wait_queue_head_t suspendq;

/*
 * ADC mc13783 API
 */
/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(mc13783_adc_init_reg);
EXPORT_SYMBOL(mc13783_adc_init_param);
EXPORT_SYMBOL(mc13783_adc_convert);
EXPORT_SYMBOL(mc13783_adc_init_monitor_param);
EXPORT_SYMBOL(mc13783_adc_monitoring_start);
EXPORT_SYMBOL(mc13783_adc_monitoring_stop);
EXPORT_SYMBOL(mc13783_adc_loaded);

static DECLARE_WAIT_QUEUE_HEAD(adcdone_it);
static DECLARE_WAIT_QUEUE_HEAD(adcbisdone_it);
static DECLARE_WAIT_QUEUE_HEAD(adc_tsi);
static DECLARE_WAIT_QUEUE_HEAD(queue_tsi_read);

static short *buffer;		/* the buffer */
static int buf_count;

static type_event_notification tsi_event;
static type_event_notification event_adc;
static type_event_notification event_adc_bis;
static type_event_notification adc_comp_h;
static type_event_notification adc_comp_l;

unsigned int reg_value;
static int data_ready;
static bool data_ready_adc_1;
static bool data_ready_adc_2;
static bool read_ts_installed;
static bool adc_ts;
static bool wait_ts;
static bool monitor_en;
static bool monitor_adc;
static t_check_mode wcomp_mode;
void (*monitoring_cb) (void);	/*call back to be called when event is detected. */

static DECLARE_WAIT_QUEUE_HEAD(queue_adc_busy);
static t_adc_state adc_dev[2];

/*!
 * This is the suspend of power management for the mc13783 ADC API.
 * It supports SAVE and POWER_DOWN state.
 *
 * @param        dev            the device
 * @param        state          the state
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_adc_suspend(struct device *dev, u32 state, u32 level)
{
	unsigned int reg_value = 0;

	switch (level) {
	case SUSPEND_DISABLE:
		suspend_flag = 1;
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		CHECK_ERROR(mc13783_write_reg_value(PRIO_ADC,
						    REG_ADC_0, DEF_ADC_0));
		CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_1, &reg_value));
		CHECK_ERROR(mc13783_write_reg_value(PRIO_ADC, REG_ADC_3,
						    DEF_ADC_3));
		break;
	}

	return 0;
};

/*!
 * This is the resume of power management for the mc13783 adc API.
 * It supports RESTORE state.
 *
 * @param        dev            the device
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_adc_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_POWER_ON:
		/* nothing for mc13783 adc */
		break;
	case RESUME_RESTORE_STATE:
		break;
	case RESUME_ENABLE:
		/* nothing for mc13783 adc */
		suspend_flag = 0;
		while (swait > 0) {
			swait--;
			wake_up_interruptible(&suspendq);
		}
		break;
	}

	return 0;
};

#ifdef __TSI_FAILED

DECLARE_WAIT_QUEUE_HEAD(adc_tsi_poll);
static bool wait_ts_poll;
static int tsi_poll;

int polling_ts(void *arg)
{
	t_touch_screen ts_value;
	wait_ts_poll = 0;
	while (1) {
		if (tsi_poll) {
			mc13783_adc_read_ts(&ts_value, false);
			if (ts_value.x_position > 50) {
				callback_tsi();
			}
		}
		msleep(100);
	};
	return 0;
}

#endif

static struct device_driver mc13783_adc_driver_ldm = {
	.name = "mc13783_adc",
	.bus = &platform_bus_type,
	.suspend = mc13783_adc_suspend,
	.resume = mc13783_adc_resume,
};

static struct platform_device mc13783_adc_ldm = {
	.name = "mc13783_adc",
	.id = 1,
};

/*
 * Call back functions
 */

/*!
 * This is the callback function called on TSI mc13783 event, used in
 * synchronous call.
 */
static void callback_tsi(void)
{
	TRACEMSG_ADC(_K_D("*** TSI IT mc13783 GET_TOUCH_SCREEN_VALUE ***"));
	if (wait_ts) {
		wake_up(&adc_tsi);
	}
#ifdef __TSI_FAILED
	if (wait_ts_poll) {
		wake_up(&adc_tsi_poll);
	}
#endif
	if (read_ts_installed) {
		update_buffer();
	}
}

/*!
 * This is the callback function called on ADCDone mc13783 event.
 */
static void callback_adcdone(void)
{
	TRACEMSG_ADC(_K_D("* adcdone it callback *"));
	if (data_ready_adc_1) {
		wake_up(&adcdone_it);
	}
}

/*!
 * This is the callback function called on ADCDone mc13783 event.
 */
static void callback_adcbisdone(void)
{
	TRACEMSG_ADC(_K_D("* adcdone bis it callback *"));
	if (data_ready_adc_2) {
		wake_up(&adcbisdone_it);
	}
}

/*!
 * This is the callback function called on mc13783 event.
 */
static void callback_adc_comp_high(void)
{
	TRACEMSG_ADC(_K_D("* adc comp it high *"));
	if (wcomp_mode == CHECK_HIGH || wcomp_mode == CHECK_LOW_OR_HIGH) {
		/* launch callback */
		if (monitoring_cb != NULL) {
			monitoring_cb();
		}
	}
}

/*!
 * This is the callback function called on mc13783 event.
 */
static void callback_adc_comp_low(void)
{
	TRACEMSG_ADC(_K_D("* adc comp it low *"));
	if (wcomp_mode == CHECK_LOW || wcomp_mode == CHECK_LOW_OR_HIGH) {
		/* launch callback */
		if (monitoring_cb != NULL) {
			monitoring_cb();
		}
	}
}

#ifdef ADC_TEST_CODE
/*!
 * This is the callback function called on mc13783 event.
 */
static void callback_wcomp(void)
{
	TRACEMSG_ADC(_K_D("\tADC Monitoring test event"));
}
#endif

/*!/
 * This function is used to update buffer of touch screen value in read mode.
 */
static void update_buffer(void)
{
	t_touch_screen ts_value;
	unsigned int adc_0_reg = 0, adc_1_reg = 0;
	TRACEMSG_ADC(_K_D("*** update buffer ***"));
	do {
		mc13783_adc_read_ts(&ts_value, false);
		/* Filter the sample read */
		/* If the sample is invalid, the below function returns -1 */
		if (-1 != mc13783_adc_filter(&ts_value)) {

			if (buf_count < (buf_size - 4)) {
				buffer[buf_count + 0] =
				    (short)ts_value.contact_resistance;
				buffer[buf_count + 1] =
				    (short)ts_value.x_position;
				buffer[buf_count + 2] =
				    (short)ts_value.y_position;
				buffer[buf_count + 3] = 0;	/* not used */
				buf_count += 4;
			}

			TRACEMSG_ADC(_K_D("Resistance: %d XPos: %d YPos: %d\n",
					  (short)ts_value.contact_resistance,
					  (short)ts_value.x_position,
					  (short)ts_value.y_position));

			data_ready = 1;
			wake_up(&queue_tsi_read);
		}
		msleep(30);
	} while (ts_value.contact_resistance >= 1);

	/* configure adc to wait tsi interrupt */
	adc_0_reg = ADC_WAIT_TSI_0 | (ADC_BIS * adc_ts);
	mc13783_write_reg(PRIO_ADC, REG_ADC_0, &adc_0_reg);
	adc_1_reg = ADC_WAIT_TSI_1 | (ADC_BIS * adc_ts);
	mc13783_write_reg(PRIO_ADC, REG_ADC_1, &adc_1_reg);
}

/*!
 * This function performs filtering and rejection of excessive noise prone
 * samples.
 *
 * @param        ts_curr     Touch screen value
 *
 * @return       This function returns 0 on success, -1 otherwise.
 */
static int mc13783_adc_filter(t_touch_screen * ts_curr)
{
	unsigned int ydiff1, ydiff2, ydiff3, xdiff1, xdiff2, xdiff3;
	unsigned int sample_sumx, sample_sumy;
	static unsigned int prev_x[FILTLEN], prev_y[FILTLEN];
	int index = 0;
	unsigned int y_curr, x_curr;
	static int filt_count = 0;
	/* Added a variable filt_type to decide filtering at run-time */
	unsigned int filt_type = 0;

	ydiff1 = abs(ts_curr->y_position1 - ts_curr->y_position2);
	ydiff2 = abs(ts_curr->y_position2 - ts_curr->y_position3);
	ydiff3 = abs(ts_curr->y_position1 - ts_curr->y_position3);
	//  printk(KERN_EMERG"Filter - y %d %d %d\n",ts_curr->y_position1,ts_curr->y_position2,ts_curr->y_position3);
	if ((ydiff1 > DELTA_Y_MAX) ||
	    (ydiff2 > DELTA_Y_MAX) || (ydiff3 > DELTA_Y_MAX)) {
		TRACEMSG_ADC(_K_D("mc13783_adc_filter: Ret pos 1\n"));
		return -1;
	}

	xdiff1 = abs(ts_curr->x_position1 - ts_curr->x_position2);
	xdiff2 = abs(ts_curr->x_position2 - ts_curr->x_position3);
	xdiff3 = abs(ts_curr->x_position1 - ts_curr->x_position3);

	// printk(KERN_EMERG"Filter - x %d %d %d\n",ts_curr->x_position1,ts_curr->x_position2,ts_curr->x_position3);

	if ((xdiff1 > DELTA_X_MAX) ||
	    (xdiff2 > DELTA_X_MAX) || (xdiff3 > DELTA_X_MAX)) {
		TRACEMSG_ADC(_K_D("mc13783_adc_filter: Ret pos 2\n"));
		return -1;
	}
	/* Compute two closer values among the three available Y readouts */

	if (ydiff1 < ydiff2) {
		if (ydiff1 < ydiff3) {
			// Sample 0 & 1 closest together
			sample_sumy = ts_curr->y_position1 +
			    ts_curr->y_position2;
		} else {
			// Sample 0 & 2 closest together
			sample_sumy = ts_curr->y_position1 +
			    ts_curr->y_position3;
		}
	} else {
		if (ydiff2 < ydiff3) {
			// Sample 1 & 2 closest together
			sample_sumy = ts_curr->y_position2 +
			    ts_curr->y_position3;
		} else {
			// Sample 0 & 2 closest together
			sample_sumy = ts_curr->y_position1 +
			    ts_curr->y_position3;
		}
	}

	/*
	 * Compute two closer values among the three available X
	 * readouts
	 */
	if (xdiff1 < xdiff2) {
		if (xdiff1 < xdiff3) {
			// Sample 0 & 1 closest together
			sample_sumx = ts_curr->x_position1 +
			    ts_curr->x_position2;
		} else {
			// Sample 0 & 2 closest together
			sample_sumx = ts_curr->x_position1 +
			    ts_curr->x_position3;
		}
	} else {
		if (xdiff2 < xdiff3) {
			// Sample 1 & 2 closest together
			sample_sumx = ts_curr->x_position2 +
			    ts_curr->x_position3;
		} else {
			// Sample 0 & 2 closest together
			sample_sumx = ts_curr->x_position1 +
			    ts_curr->x_position3;
		}
	}

	/* Contact resistance =0 implies pen-up .. Clear the filter */

	if (ts_curr->contact_resistance == 0) {
		filt_count = 0;
		TRACEMSG_ADC(_K_D("PEN UP DETECTED\n"));
	}

	/*
	 * Wait FILTER_MIN_DELAY number of samples to restart
	 * filtering
	 */
	if (filt_count < FILTER_MIN_DELAY) {
		/*
		 * Current output is the average of the two closer
		 * values and no filtering is used
		 */
		y_curr = (sample_sumy / 2);
		x_curr = (sample_sumx / 2);
		ts_curr->y_position = y_curr;
		ts_curr->x_position = x_curr;
		filt_count++;
	} else {
		if (abs(sample_sumx - (prev_x[0] + prev_x[1])) >
		    (DELTA_X_MAX * 16)) {
			TRACEMSG_ADC(_K_D("mc13783_adc_filter: : Ret pos 3\n"));
			return -1;
		}
		if (abs(sample_sumy - (prev_y[0] + prev_y[1])) >
		    (DELTA_Y_MAX * 16)) {
			TRACEMSG_ADC(_K_D("mc13783_adc_filter: Ret pos 4 "
					  "%d, %d, %d, %d, %d, %d \n",
					  sample_sumy, prev_y[0], prev_y[1],
					  ts_curr->y_position1,
					  ts_curr->y_position2,
					  ts_curr->y_position3));
			return -1;
		}
		sample_sumy /= 2;
		sample_sumx /= 2;
		/* Use hard filtering if the sample difference < 10 */
		if ((abs(sample_sumy - prev_y[0]) > 10) ||
		    (abs(sample_sumx - prev_x[0]) > 10)) {
			filt_type = 1;
		}

		/*
		 * Current outputs are the average of three previous
		 * values and the present readout
		 */
		y_curr = sample_sumy;
		for (index = 0; index < FILTLEN; index++) {
			if (filt_type == 0) {
				y_curr = y_curr + (prev_y[index]);
			} else {
				y_curr = y_curr + (prev_y[index] / 3);
			}
		}
		if (filt_type == 0) {
			y_curr = y_curr >> 2;
		} else {
			y_curr = y_curr >> 1;
		}
		ts_curr->y_position = y_curr;

		x_curr = sample_sumx;
		for (index = 0; index < FILTLEN; index++) {
			if (filt_type == 0) {
				x_curr = x_curr + (prev_x[index]);
			} else {
				x_curr = x_curr + (prev_x[index] / 3);
			}
		}
		if (filt_type == 0) {
			x_curr = x_curr >> 2;
		} else {
			x_curr = x_curr >> 1;
		}
		ts_curr->x_position = x_curr;

	}

	/* Update previous X and Y values */
	for (index = (FILTLEN - 1); index > 0; index--) {
		prev_x[index] = prev_x[index - 1];
		prev_y[index] = prev_y[index - 1];
	}

	/*
	 * Current output will be the most recent past for the
	 * next sample
	 */
	prev_y[0] = y_curr;
	prev_x[0] = x_curr;

	return 0;
}

/*!
 * This function is the read interface of mc13783 ADC, It returns touch screen
 * value.
 *
 * @param        file        pointer on the file
 * @param        buf         the user space buffer
 * @param        count       number of date
 * @param        ppos        pointer position
 *
 * @return       This function returns number of date read.
 */
static ssize_t mc13783_adc_read(struct file *file, char *buf, size_t count,
				loff_t * ppos)
{
	int ret = 0;
	int i = 0;

	TRACEMSG_ADC(_K_D("mc13783_adc_read()"));
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}

	/* return value of buffer with all touch screen value */
	/* is the buffer is empty return 0 */

	if (buf_count != 0) {
		/* Return read value */
		if (buf_count <= count) {
			copy_to_user(buf, buffer, buf_count * sizeof(short));
			ret = buf_count;
			/*printk(KERN_EMERG"RES: %d X: %d Y: %d BYTES: %d\n",buffer[0],buffer[1],buffer[2],ret); */

			/* Reset the first four elements of
			 * buffer before next adc read.
			 * If the user call the read function
			 * again whereas the Touch Screen
			 * has not been used, a four element
			 * buffer will be returned
			 * containing NULL values.
			 */
			buffer[0] = 0;
			buffer[1] = 0;
			buffer[2] = 0;
			buffer[3] = 0;
			buf_count = 0;
			data_ready = 0;
		} else {
			copy_to_user(buf, buffer, count * sizeof(short));
			ret = count;
			for (i = 0; i < (buf_count - count); i++) {
				buffer[i] = buffer[count + i];
			}
			buf_count = buf_count - count;
		}
	} else {
		/* No value to read. Return a four element buffer */
		/* containing NULL values. */
		copy_to_user(buf, buffer, 4 * sizeof(short));
	}

	reg_value = ADC_INT_BISDONEI;
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_INTERRUPT_STATUS_0,
				      &reg_value));

	TRACEMSG_ADC(_K_D("mc13783_adc : DataReady %d"), data_ready);
	TRACEMSG_ADC(_K_D("mc13783_adc : contact %d, x %d, y %d, pad %d"),
		     buffer[0], buffer[1], buffer[2], buffer[3]);

	return ret;
}

/*!
 * This function implements the open method on a MC13783 ADC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_adc_open(struct inode *inode, struct file *file)
{
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	TRACEMSG_ADC(_K_D("mc13783_adc : mc13783_adc_open()"));
	return 0;
}

/*!
 * This function implements the release method on a MC13783 ADC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_adc_free(struct inode *inode, struct file *file)
{
	TRACEMSG_ADC(_K_D("mc13783_adc : mc13783_adc_free()"));
	return 0;
}

/*!
 * This function is called to initialize all ADC registers with default values.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_init_reg(void)
{
	unsigned int reg_value = 0, i = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	for (i = 0; i < ADC_NB_AVAILABLE; i++) {
		adc_dev[i] = ADC_FREE;
	}
	CHECK_ERROR(mc13783_write_reg_value(PRIO_ADC, REG_ADC_0, DEF_ADC_0));
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_1, &reg_value));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_ADC, REG_ADC_3, DEF_ADC_3));

	data_ready_adc_1 = false;
	data_ready_adc_2 = false;
	read_ts_installed = false;
	adc_ts = false;
	wait_ts = false;
	monitor_en = false;
	monitor_adc = false;
	wcomp_mode = CHECK_LOW;
	monitoring_cb = NULL;

	/* sub to ADCDone IT */
	CHECK_ERROR(mc13783_event_init(&event_adc));
	event_adc.event = EVENT_ADCDONEI;
	event_adc.callback = callback_adcdone;
	CHECK_ERROR(mc13783_event_subscribe(event_adc));

	/* sub to ADCDoneBis IT */
	CHECK_ERROR(mc13783_event_init(&event_adc_bis));
	event_adc_bis.event = EVENT_ADCBISDONEI;
	event_adc_bis.callback = callback_adcbisdone;
	CHECK_ERROR(mc13783_event_subscribe(event_adc_bis));

	/* sub to Touch Screen IT */
	CHECK_ERROR(mc13783_event_init(&tsi_event));
	tsi_event.event = EVENT_TSI;
	tsi_event.callback = callback_tsi;
	CHECK_ERROR(mc13783_event_subscribe(tsi_event));

	/* ADC reading above high limit */
	CHECK_ERROR(mc13783_event_init(&adc_comp_h));
	adc_comp_h.event = EVENT_WHIGHI;
	adc_comp_h.callback = callback_adc_comp_high;
	CHECK_ERROR(mc13783_event_subscribe(adc_comp_h));

#ifdef ADC_TEST_CODE
	/* ADC reading below low limit */
	CHECK_ERROR(mc13783_event_init(&adc_comp_l));
	adc_comp_l.event = EVENT_WLOWI;
	adc_comp_l.callback = callback_adc_comp_low;
	CHECK_ERROR(mc13783_event_subscribe(adc_comp_l));
#endif

#ifdef __TSI_FAILED
	tsi_poll = false;
	kernel_thread(polling_ts, NULL, CLONE_VM | CLONE_FS);
#endif
	return 0;
}

/*!
 * This function initializes adc_param structure.
 *
 * @param        adc_param     Structure to be initialized.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_init_param(t_adc_param * adc_param)
{
	int i = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	adc_param->delay = 0;
	adc_param->conv_delay = false;
	adc_param->single_channel = false;
	adc_param->group = false;
	adc_param->channel_0 = BATTERY_VOLTAGE;
	adc_param->channel_1 = BATTERY_VOLTAGE;
	adc_param->read_mode = 0;
	adc_param->wait_tsi = 0;
	adc_param->chrgraw_devide_5 = true;
	adc_param->read_ts = false;
	adc_param->ts_value.x_position = 0;
	adc_param->ts_value.y_position = 0;
	adc_param->ts_value.contact_resistance = 0;
	for (i = 0; i <= MAX_CHANNEL; i++) {
		adc_param->value[i] = 0;
	}
	return 0;
}

/*!
 * This function starts the convert.
 *
 * @param        adc_param      contains all adc configuration and return value.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_convert(t_adc_param * adc_param)
{
	bool use_bis = false;
	unsigned int adc_0_reg = 0, adc_1_reg = 0, reg_1 = 0, result_reg = 0;
	unsigned int i = 0;
	unsigned int result = 0;
	DEFINE_WAIT(wait);

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if (adc_param->wait_tsi) {
#ifdef __TSI_FAILED
		TRACEMSG_ADC(_K_D("wait tsi ...."));
		tsi_poll = true;
		wait_ts_poll = true;
		prepare_to_wait(&adc_tsi_poll, &wait, TASK_UNINTERRUPTIBLE);
		schedule();
		finish_wait(&adc_tsi_poll, &wait);
		wait_ts_poll = false;
		tsi_poll = true;

#else
		/* we need to set ADCEN 1 for TSI interrupt on mc13783 1.x */
		/* configure adc to wait tsi interrupt */
		adc_0_reg = ADC_WAIT_TSI_0 | (ADC_BIS * use_bis);
		CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_0, &adc_0_reg));
		adc_1_reg = ADC_WAIT_TSI_1 | (ADC_BIS * adc_ts);
		CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_1, &adc_1_reg));
		TRACEMSG_ADC(_K_D("wait tsi ...."));
		wait_ts = true;
		prepare_to_wait(&adc_tsi, &wait, TASK_UNINTERRUPTIBLE);
		schedule();
		finish_wait(&adc_tsi, &wait);
		wait_ts = false;
#endif
	}

	use_bis = mc13783_adc_request();

	/* CONFIGURE ADC REG 0 */
	adc_0_reg = 0;
	adc_1_reg = 0;
	if (adc_param->read_ts == false) {
		adc_0_reg = adc_param->read_mode & ADC_CONF_0;
		/* add auto inc */
		adc_0_reg |= ADC_INC;

		if (use_bis) {
			/* add adc bis */
			adc_0_reg |= ADC_BIS;
		}

		if (adc_param->chrgraw_devide_5) {
			adc_0_reg |= ADC_CHRGRAW_D5;
		}

		if (adc_param->single_channel) {
			adc_1_reg |= ADC_SGL_CH;
		}

		if (adc_param->conv_delay) {
			adc_1_reg |= ADC_ATO;
		}

		if (adc_param->group) {
			adc_1_reg |= ADC_ADSEL;
		}

		if (adc_param->single_channel) {
			adc_1_reg |= ADC_SGL_CH;
		}

		adc_1_reg |= (adc_param->channel_0 << ADC_CH_0_POS) &
		    ADC_CH_0_MASK;
		adc_1_reg |= (adc_param->channel_1 << ADC_CH_1_POS) &
		    ADC_CH_1_MASK;
	} else {
		adc_0_reg = ADC_POS_MODE_0 | (ADC_BIS * use_bis) | ADC_INC;
	}
	TRACEMSG_ADC(_K_I("Write Reg %i = %x"), REG_ADC_0, adc_0_reg);
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_0, &adc_0_reg));

	/* CONFIGURE ADC REG 1 */
	if (adc_param->read_ts == false) {
		adc_1_reg |= ADC_NO_ADTRIG;
		adc_1_reg |= ADC_EN;
		adc_1_reg |= (adc_param->delay << ADC_DELAY_POS) &
		    ADC_DELAY_MASK;
		if (use_bis) {
			adc_1_reg |= ADC_BIS;
		}
	} else {
		/* configure and start convert to read x and y position */
		/* configure to read 2 value in channel selection 1 & 2 */
		adc_1_reg = ADC_POS_MODE_1 | (ADC_BIS * use_bis) |
		    ADC_NO_ADTRIG;
	}

	reg_1 = adc_1_reg;
	if (use_bis == 0) {
		data_ready_adc_1 = false;
		adc_1_reg |= ASC_ADC;
		data_ready_adc_1 = true;
		TRACEMSG_ADC(_K_I("Write Reg %i = %x"), REG_ADC_1, adc_1_reg);
		CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_1, &adc_1_reg));
		TRACEMSG_ADC(_K_D("wait adc done"));
		prepare_to_wait(&adcdone_it, &wait, TASK_UNINTERRUPTIBLE);
		schedule_timeout(1);
		finish_wait(&adcdone_it, &wait);
		TRACEMSG_ADC(_K_D("IT adc done"));
		data_ready_adc_1 = false;
	} else {
		data_ready_adc_2 = false;
		adc_1_reg |= ASC_ADC;
		data_ready_adc_2 = true;
		CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_1, &adc_1_reg));
		TRACEMSG_ADC(_K_D("wait adc done bis"));
		prepare_to_wait(&adcbisdone_it, &wait, TASK_UNINTERRUPTIBLE);
		schedule_timeout(1);
		finish_wait(&adcbisdone_it, &wait);
		data_ready_adc_2 = false;
	}
	/* read result and store in adc_param */
	result = 0;
	if (use_bis == 0) {
		result_reg = REG_ADC_2;
	} else {
		result_reg = REG_ADC_4;
	}
	for (i = 0; i <= 3; i++) {
		CHECK_ERROR(mc13783_read_reg(PRIO_ADC, result_reg, &result));
		TRACEMSG_ADC(_K_I("result %i = %x"), result_reg, result);
		adc_param->value[i] = ((result & ADD1_RESULT_MASK) >> 2);
		adc_param->value[i + 4] = ((result & ADD2_RESULT_MASK) >> 14);
	}

	if (adc_param->read_ts) {
		adc_param->ts_value.x_position = adc_param->value[2];
		adc_param->ts_value.x_position1 = adc_param->value[0];
		adc_param->ts_value.x_position2 = adc_param->value[1];
		adc_param->ts_value.x_position3 = adc_param->value[2];
		adc_param->ts_value.y_position1 = adc_param->value[3];
		adc_param->ts_value.y_position2 = adc_param->value[4];
		adc_param->ts_value.y_position3 = adc_param->value[5];
		adc_param->ts_value.y_position = adc_param->value[5];
		adc_param->ts_value.contact_resistance = adc_param->value[6];

	}
	mc13783_adc_release(use_bis);
	return 0;
}

/*!
 * This function request a ADC.
 *
 * @return      This function returns index of ADC to be used (0 or 1)
                if successful.
                return -1 if error.
 */
static int mc13783_adc_request(void)
{
	int adc_index = -1;
	if (((adc_dev[0] == ADC_USED) && (adc_dev[1] == ADC_USED))) {
		/* all ADC is used wait... */
		wait_event(queue_adc_busy, 0);
	} else if (adc_dev[0] == ADC_FREE) {
		adc_dev[0] = ADC_USED;
		adc_index = 0;
	} else if (adc_dev[1] == ADC_FREE) {
		adc_dev[1] = ADC_USED;
		adc_index = 1;
	}
	TRACEMSG_ADC(_K_D("mc13783_adc : request ADC %d"), adc_index);
	return adc_index;
}

/*!
 * This function release an ADC.
 *
 * @param        adc_index     index of ADC to be released.
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_adc_release(int adc_index)
{
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}

	TRACEMSG_ADC(_K_D("mc13783_adc : release ADC %d"), adc_index);
	if ((adc_dev[adc_index] == ADC_MONITORING) ||
	    (adc_dev[adc_index] == ADC_USED)) {
		adc_dev[adc_index] = ADC_FREE;
		wake_up(&queue_adc_busy);
		return 0;
	}
	return -1;
}

/*!
 * This function enables the touch screen read interface.
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_adc_install_ts(void)
{
#ifndef __TSI_FAILED
	int adc_0_reg = 0, adc_1_reg = 0;
#endif
	TRACEMSG_ADC(_K_D("mc13783_adc : install TS"));
	if (read_ts_installed) {
		TRACEMSG_ADC(_K_D
			     ("mc13783_adc : Error TS is already installed"));
		return -1;
	}
#ifdef __TSI_FAILED
	tsi_poll = true;
#else
	/* we need to set ADCEN 1 for TSI interrupt on mc13783 1.x */
	/* configure adc to wait tsi interrupt */
	adc_0_reg = ADC_WAIT_TSI_0 | (ADC_BIS * adc_ts);
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_0, &adc_0_reg));
	adc_1_reg = ADC_WAIT_TSI_1 | (ADC_BIS * adc_ts);
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_1, &adc_1_reg));
#endif
	read_ts_installed = true;
	return 0;
}

/*!
 * This function disables the touch screen read interface.
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_adc_remove_ts(void)
{
	TRACEMSG_ADC(_K_D("mc13783_adc : remove TS"));
	if (read_ts_installed) {
		mc13783_adc_release(adc_ts);
		read_ts_installed = false;
	}
#ifdef __TSI_FAILED
	tsi_poll = false;
#endif
	return 0;
}

/*!
 * This function initializes monitoring structure.
 *
 * @param        monitor     Structure to be initialized.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_init_monitor_param(t_monitoring_param * monitor)
{
	TRACEMSG_ADC(_K_D("mc13783_adc : init monitor"));
	monitor->delay = 0;
	monitor->conv_delay = false;
	monitor->channel = BATTERY_VOLTAGE;
	monitor->read_mode = 0;
	monitor->comp_low = 0;
	monitor->comp_high = 0;
	monitor->group = 0;
	monitor->check_mode = CHECK_LOW_OR_HIGH;
	monitor->callback = NULL;
	return 0;
}

/*!
 * This function starts the monitoring ADC convert.
 *
 * @param        monitor     contains all adc monitoring configuration.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_monitoring_start(t_monitoring_param * monitor)
{
	bool use_bis = false;
	unsigned int adc_0_reg = 0, adc_1_reg = 0, adc_3_reg = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if (monitor_en) {
		TRACEMSG_ADC(_K_D
			     ("mc13783_adc : monitoring already configured"));
		return -1;
	}
	monitor_en = true;
	use_bis = mc13783_adc_request();
	if (use_bis < 0) {
		TRACEMSG_ADC(_K_D("mc13783_adc : request error"));
		return use_bis;
	}
	monitor_adc = use_bis;

	adc_0_reg = 0;

	/* TO DO ADOUT CONFIGURE */
	adc_0_reg = monitor->read_mode & ADC_MODE_MASK;
	if (use_bis) {
		/* add adc bis */
		adc_0_reg |= ADC_BIS;
	}
	adc_0_reg |= ADC_WCOMP;

	/* CONFIGURE ADC REG 1 */
	adc_1_reg = 0;
	adc_1_reg |= ADC_EN;
	if (monitor->conv_delay) {
		adc_1_reg |= ADC_ATO;
	}
	if (monitor->group) {
		adc_1_reg |= ADC_ADSEL;
	}
	adc_1_reg |= (monitor->channel << ADC_CH_0_POS) & ADC_CH_0_MASK;
	adc_1_reg |= (monitor->delay << ADC_DELAY_POS) & ADC_DELAY_MASK;
	if (use_bis) {
		adc_1_reg |= ADC_BIS;
	}

	adc_3_reg |= (monitor->comp_high << ADC_WCOMP_H_POS) & ADC_WCOMP_H_MASK;
	adc_3_reg |= (monitor->comp_low << ADC_WCOMP_L_POS) & ADC_WCOMP_L_MASK;
	if (use_bis) {
		adc_3_reg |= ADC_BIS;
	}

	wcomp_mode = monitor->check_mode;
	/* call back to be called when event is detected. */
	monitoring_cb = monitor->callback;

	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_0, &adc_0_reg));
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_1, &adc_1_reg));
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_3, &adc_3_reg));
	return 0;
}

/*!
 * This function stops the monitoring ADC convert.
 *
 * @param        monitor     contains all adc monitoring configuration.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_monitoring_stop(t_monitoring_param * monitor)
{
	unsigned int reg_value = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if (!monitor_en) {
		TRACEMSG_ADC(_K_D("mc13783_adc : adc monitoring free"));
		return -1;
	}

	if (monitor_en) {
		reg_value = ADC_BIS;
	}

	/* clear all reg value */
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_0, &reg_value));
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_1, &reg_value));
	CHECK_ERROR(mc13783_write_reg(PRIO_ADC, REG_ADC_3, &reg_value));

	reg_value = 0;

	mc13783_adc_release(monitor_adc);
	monitor_en = false;
	return 0;
}

/*!
 * This function read the touch screen value.
 *
 * @param        ts_value    return value of touch screen
 * @param        wait_tsi    if true, this function is synchronous
 *                           (wait in TSI event).
 *
 * @return       This function returns 0.
 */
static int mc13783_adc_read_ts(t_touch_screen * ts_value, int wait_tsi)
{

	t_adc_param param;
	TRACEMSG_ADC(_K_D("mc13783_adc : Read TS"));
	if (wait_ts) {
		TRACEMSG_ADC(_K_D("mc13783_adc : error TS busy "));
		return -1;
	}
	mc13783_adc_init_param(&param);
	TRACEMSG_ADC(_K_D("Read TS"));
	param.wait_tsi = wait_tsi;
	param.read_ts = true;
#ifdef __TSI_FAILED
	do {
		mc13783_adc_convert(&param);
	} while (param.ts_value.x_position < 1);
#else
	mc13783_adc_convert(&param);

#endif
	/* check if x-y is ok */
	if ((param.ts_value.x_position1 < TS_X_MAX) &&
	    (param.ts_value.x_position1 >= TS_X_MIN) &&
	    (param.ts_value.y_position1 < TS_Y_MAX) &&
	    (param.ts_value.y_position1 >= TS_Y_MIN) &&
	    (param.ts_value.x_position2 < TS_X_MAX) &&
	    (param.ts_value.x_position2 >= TS_X_MIN) &&
	    (param.ts_value.y_position2 < TS_Y_MAX) &&
	    (param.ts_value.y_position2 >= TS_Y_MIN) &&
	    (param.ts_value.x_position3 < TS_X_MAX) &&
	    (param.ts_value.x_position3 >= TS_X_MIN) &&
	    (param.ts_value.y_position3 < TS_Y_MAX) &&
	    (param.ts_value.y_position3 >= TS_Y_MIN)) {
		ts_value->x_position = param.ts_value.x_position;
		ts_value->x_position1 = param.ts_value.x_position1;
		ts_value->x_position2 = param.ts_value.x_position2;
		ts_value->x_position3 = param.ts_value.x_position3;
		ts_value->y_position = param.ts_value.y_position;
		ts_value->y_position1 = param.ts_value.y_position1;
		ts_value->y_position2 = param.ts_value.y_position2;
		ts_value->y_position3 = param.ts_value.y_position3;
		ts_value->contact_resistance =
		    param.ts_value.contact_resistance + 1;

	} else {
		ts_value->x_position = 0;
		ts_value->y_position = 0;
		ts_value->contact_resistance = 0;

	}

	return 0;
}

/*!
 * This function is the poll interface of MC13783 ADC device.
 *
 * @param        filp        pointer on the file
 * @param        wait        poll table
 *
 * @return       The poll method returns a bit mask indicating whether
                 nonblocking reads or writes are possible.
 */
static unsigned int mc13783_adc_poll(struct file *filp, poll_table * wait)
{
	unsigned int ret = 0;

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	poll_wait(filp, &queue_tsi_read, wait);
	if (data_ready) {
		ret = POLLIN | POLLRDNORM;
	} else {
		ret = 0;
	}
	return ret;
}

/*!
 * This function implements IOCTL controls on a MC13783 ADC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_adc_ioctl(struct inode *inode, struct file *file,
			     unsigned int cmd, unsigned long arg)
{
	t_touch_screen *ts_value;
	unsigned int i = 0;
	t_adc_param param;
	t_monitoring_param mon_param;

	if (_IOC_TYPE(cmd) != 'D')
		return -ENOTTY;

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}

	switch (cmd) {
	case INIT_ADC:
		TRACEMSG_ADC(_K_D("init adc"));
		CHECK_ERROR(mc13783_adc_init_reg());
		break;
#ifdef ADC_TEST_CODE
	case ADC_TEST_CONVERT:
		TRACEMSG_ALL_TIME(_K_I("Test ADC Read interface"));

		TRACEMSG_ALL_TIME(_K_I("ADC Read all 8 channels group 0"));
		mc13783_adc_init_param(&param);
		param.channel_0 = BATTERY_VOLTAGE;
		param.channel_1 = CHARGE_CURRENT;
		mc13783_adc_convert(&param);
		TRACEMSG_ALL_TIME(_K_I("Read value : "));
		for (i = 0; i < 8; i++) {
			TRACEMSG_ALL_TIME(_K_I("\tADC Value %d = %d"), i,
					  param.value[i]);
		}
		TRACEMSG_ALL_TIME(_K_I("ADC Read all 8 channels group 1"));
		mc13783_adc_init_param(&param);
		param.group = true;
		param.channel_0 = BATTERY_CURRENT;
		param.channel_1 = GENERAL_PURPOSE_ADIN5;
		mc13783_adc_convert(&param);
		TRACEMSG_ALL_TIME(_K_I("Read value : "));
		for (i = 0; i < 8; i++) {
			TRACEMSG_ALL_TIME(_K_I("\tADC Value %d = %d"), i,
					  param.value[i]);
		}

		TRACEMSG_ALL_TIME(_K_I
				  ("ADC Read channel #1 (BATTERY_CURRENT) group 0"));
		mc13783_adc_init_param(&param);
		param.single_channel = true;
		param.group = false;
		param.channel_0 = BATTERY_CURRENT;
		param.conv_delay = true;
		param.delay = 4;
		param.read_mode = M_CHARGE_CURRENT | M_THERMISTOR;
		mc13783_adc_convert(&param);
		TRACEMSG_ALL_TIME(_K_I("Read value : "));
		for (i = 0; i < 8; i++) {
			TRACEMSG_ALL_TIME(_K_I("\tADC Value %d = %d"), i,
					  param.value[i]);
		}

		TRACEMSG_ALL_TIME(_K_I("Read Touch Screen"));
		param.read_ts = true;
		mc13783_adc_convert(&param);
		TRACEMSG_ALL_TIME(_K_I("Read value : "));
		for (i = 0; i < 8; i++) {
			TRACEMSG_ALL_TIME(_K_I("\tADC Value %d = %d"), i,
					  param.value[i]);
		}
		break;
#endif
	case GET_TOUCH_SCREEN_VALUE:
		TRACEMSG_ADC(_K_D("get adc touch screen value"));
		if ((ts_value = kmalloc(sizeof(t_touch_screen), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(ts_value, (t_touch_screen *) arg,
				   sizeof(t_touch_screen))) {
			return -EFAULT;
		}
		CHECK_ERROR(mc13783_adc_read_ts(ts_value, true));

		TRACEMSG_ADC(_K_D("x position = %d\n"), (int)
			     ts_value->x_position);
		TRACEMSG_ADC(_K_D("y position = %d\n"), (int)
			     ts_value->y_position);
		TRACEMSG_ADC(_K_D("contact resistance = %d\n"), (int)
			     ts_value->contact_resistance);
		if (copy_to_user((t_touch_screen *) arg, ts_value,
				 sizeof(t_touch_screen))) {
			return -EFAULT;
		}

		kfree(ts_value);
		break;
	case TOUCH_SCREEN_READ_INSTALL:
		TRACEMSG_ADC(_K_D("test adc touch screen"));
		mc13783_adc_install_ts();
		break;
	case TOUCH_SCREEN_READ_UNINSTALL:
		TRACEMSG_ADC(_K_D("test adc touch screen unsubscribe"));
		mc13783_adc_remove_ts();
		break;
#ifdef ADC_TEST_CODE
	case ADC_TEST_MONITORING:
		TRACEMSG_ALL_TIME(_K_D("Test ADC monitoring functions"));
		mc13783_adc_init_monitor_param(&mon_param);
		mon_param.channel = 1;
		mon_param.comp_low = 3;
		mon_param.comp_high = 60;
		mon_param.callback = callback_wcomp;
		mon_param.check_mode = CHECK_LOW_OR_HIGH;
		mc13783_adc_monitoring_start(&mon_param);
		mc13783_adc_monitoring_stop(&mon_param);
		break;
#endif
	default:
		TRACEMSG_ADC(_K_D("unsupported ioctl command"));
		return -EINVAL;
	}
	return 0;
}

static struct file_operations mc13783_adc_fops = {
	.owner = THIS_MODULE,
	.poll = mc13783_adc_poll,
	.read = mc13783_adc_read,
	.ioctl = mc13783_adc_ioctl,
	.open = mc13783_adc_open,
	.release = mc13783_adc_free,
};

int mc13783_adc_loaded(void)
{
	return mc13783_adc_detected;
}

/*
 * Initialization and Exit
 */
static int __init mc13783_adc_init(void)
{
	int i = 0;
	int ret = 0;

	if (mc13783_core_loaded() == 0) {
		printk(KERN_INFO MC13783_LOAD_ERROR_MSG);
		return -1;
	}

	mc13783_adc_major =
	    register_chrdev(0, "mc13783_adc", &mc13783_adc_fops);
	if (mc13783_adc_major < 0) {
		TRACEMSG_ADC(_K_D("Unable to get a major for mc13783_adc"));
		return mc13783_adc_major;
	}
	init_waitqueue_head(&suspendq);

	devfs_mk_cdev(MKDEV(mc13783_adc_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "mc13783_adc");

	/* Allocate the buffer */
	buffer = (short *)kmalloc(buf_size * sizeof(short), GFP_KERNEL);
	if (buffer == NULL) {
		TRACEMSG_ADC(_K_D
			     ("unable to allocate buffer for mc13783 adc device\n"));
		return -ENOMEM;
	}
	CHECK_ERROR(mc13783_adc_init_reg());
	for (i = 0; i < buf_size; i++) {
		buffer[i] = 0;
	}

	data_ready = 0;
	buf_count = 0;

	ret = driver_register(&mc13783_adc_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&mc13783_adc_ldm);
		if (ret != 0) {
			driver_unregister(&mc13783_adc_driver_ldm);
		} else {
			mc13783_adc_detected = 1;
			printk(KERN_INFO "mc13783 ADC loaded\n");
		}
	}

	return ret;
}

static void __exit mc13783_adc_exit(void)
{
	mc13783_event_unsubscribe(event_adc);
	mc13783_event_unsubscribe(event_adc_bis);
	mc13783_event_unsubscribe(tsi_event);
	mc13783_event_unsubscribe(adc_comp_h);
#ifdef ADC_TEST_CODE
	mc13783_event_unsubscribe(adc_comp_l);
#endif

	kfree(buffer);

	devfs_remove("mc13783_adc");

	unregister_chrdev(mc13783_adc_major, "mc13783_adc");
	driver_unregister(&mc13783_adc_driver_ldm);
	platform_device_unregister(&mc13783_adc_ldm);

	printk(KERN_INFO "mc13783 adc : successfully unloaded\n");
}

/*
 * Module entry points
 */

module_init(mc13783_adc_init);
module_exit(mc13783_adc_exit);

MODULE_DESCRIPTION("mc13783 ADC device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
