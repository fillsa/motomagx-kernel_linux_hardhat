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
 * @file pmic_adc.c
 * @brief This is the main file of mc13783 adc driver.
 *
 * @ingroup PMIC_ADC
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
#include "asm/arch/pmic_power.h"
#include "../core/pmic_config.h"

#include "asm/arch/pmic_adc.h"
#include "pmic_adc_defs.h"

#define NB_ADC_REG      5

static int pmic_adc_major;
static int buf_size = 4096;	/* Buffer size */

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
EXPORT_SYMBOL(pmic_adc_init);
EXPORT_SYMBOL(pmic_adc_deinit);
EXPORT_SYMBOL(pmic_adc_convert);
EXPORT_SYMBOL(pmic_adc_convert_8x);
EXPORT_SYMBOL(pmic_adc_convert_multichnnel);
EXPORT_SYMBOL(pmic_adc_set_touch_mode);
EXPORT_SYMBOL(pmic_adc_get_touch_mode);
EXPORT_SYMBOL(pmic_adc_get_touch_sample);
EXPORT_SYMBOL(pmic_adc_get_battery_current);
EXPORT_SYMBOL(pmic_adc_active_comparator);
EXPORT_SYMBOL(pmic_adc_deactive_comparator);
EXPORT_SYMBOL(pmic_adc_install_ts);
EXPORT_SYMBOL(pmic_adc_remove_ts);

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
static int pmic_adc_suspend(struct device *dev, u32 state, u32 level)
{
	unsigned int reg_value = 0;

	switch (level) {
	case SUSPEND_DISABLE:
		suspend_flag = 1;
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_0,
					   DEF_ADC_0, PMIC_ALL_BITS));
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_1,
					   reg_value, PMIC_ALL_BITS));
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_2,
					   reg_value, PMIC_ALL_BITS));
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_3,
					   DEF_ADC_3, PMIC_ALL_BITS));
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_4,
					   reg_value, PMIC_ALL_BITS));
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
static int pmic_adc_resume(struct device *dev, u32 level)
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
	t_touch_sample ts_value;
	wait_ts_poll = 0;
	while (1) {
		if (tsi_poll) {
			pmic_adc_get_touch_sample(&ts_value);
			if (ts_value.x_position > 50) {
				callback_tsi();
			}
		}
		msleep(100);
	};
	return 0;
}

#endif

static struct device_driver pmic_adc_driver_ldm = {
	.name = "MC13783_ADC",
	.bus = &platform_bus_type,
	.suspend = pmic_adc_suspend,
	.resume = pmic_adc_resume,
};

static struct platform_device pmic_adc_ldm = {
	.name = "MC13783_ADC",
	.id = 1,
};

/*
 * Call back functions
 */

/*!
 * This is the callback function called on TSI mc13783 event, used in synchronous call.
 */
static void callback_tsi(void)
{
	TRACEMSG_ADC(_K_D("*** TSI IT mc13783 PMIC_ADC_GET_TOUCH_SAMPLE ***"));
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
 * This function is used to update buffer of touch screen value in read mode.
 */
void update_buffer(void)
{
	t_touch_sample ts_value;
	unsigned int adc_0_reg = 0, adc_1_reg = 0;
	TRACEMSG_ADC(_K_D("*** update buffer ***"));
	do {
		mc13783_adc_read_ts(&ts_value, false);
		if (buf_count < (buf_size - 4)) {
			buffer[buf_count + 0] = (short)ts_value.pressure;
			buffer[buf_count + 1] = (short)ts_value.x_position;
			buffer[buf_count + 2] = (short)ts_value.y_position;
			buffer[buf_count + 3] = 0;	/* not used */
			buf_count += 4;
		}
		data_ready = 1;
		wake_up(&queue_tsi_read);
		msleep(100);
	} while (ts_value.pressure >= 1);

	/* configure adc to wait tsi interrupt */
	adc_0_reg = 0x001c00 | (ADC_BIS * adc_ts);
	pmic_write_reg(PRIO_ADC, REG_ADC_0, adc_0_reg, PMIC_ALL_BITS);
	adc_1_reg = 0x300001 | (ADC_BIS * adc_ts);
	pmic_write_reg(PRIO_ADC, REG_ADC_1, adc_1_reg, PMIC_ALL_BITS);
}

/*!
 * This function is the read interface of mc13783 ADC, It returns touch screen value.
 *
 * @param        file        pointer on the file
 * @param        buf         the user space buffer
 * @param        count       number of date
 * @param        ppos        pointer position
 *
 * @return       This function returns number of date read.
 */
static ssize_t pmic_adc_read(struct file *file, char *buf, size_t count,
			     loff_t * ppos)
{
	int ret = 0;
	int cpt_wrong_value = 0;

	TRACEMSG_ADC(_K_D("pmic_adc_read()"));
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
		copy_to_user(buf, buffer, buf_count * sizeof(short));
		ret = buf_count;
		cpt_wrong_value = 0;
	} else {
		copy_to_user(buf, buffer, 4 * sizeof(short));
		cpt_wrong_value++;
	}

	if (cpt_wrong_value <= 10) {
		reg_value = 0x02;
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_INTERRUPT_STATUS_0,
					   reg_value, PMIC_ALL_BITS));
	}

	TRACEMSG_ADC(_K_D("mc13783_adc : DataReady %d"), data_ready);
	TRACEMSG_ADC(_K_D("mc13783_adc : contact %d, x %d, y %d, pad %d"),
		     buffer[0], buffer[1], buffer[2], buffer[3]);

	data_ready = 0;
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0;
	buffer[3] = 0;
	buf_count = 0;
	return ret;
}

/*!
 * This function implements the open method on a MC13783 ADC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_adc_open(struct inode *inode, struct file *file)
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
static int pmic_adc_free(struct inode *inode, struct file *file)
{
	TRACEMSG_ADC(_K_D("mc13783_adc : mc13783_adc_free()"));
	return 0;
}

/*!
 * This function initializes all ADC registers with default values. This
 * function also registers the interrupt events.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
int pmic_adc_init(void)
{
	unsigned int reg_value = 0, i = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	for (i = 0; i < ADC_NB_AVAILABLE; i++) {
		adc_dev[i] = ADC_FREE;
	}
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_0, DEF_ADC_0,
				   PMIC_ALL_BITS));
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_1, reg_value,
				   PMIC_ALL_BITS));
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_2, reg_value,
				   PMIC_ALL_BITS));
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_3, DEF_ADC_3,
				   PMIC_ALL_BITS));
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_4, reg_value,
				   PMIC_ALL_BITS));

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
	CHECK_ERROR(pmic_event_init(&event_adc));
	event_adc.event = EVENT_ADCBISDONEI;
	event_adc.callback = (void *)callback_adcdone;
	CHECK_ERROR(pmic_event_subscribe(&event_adc));

	/* sub to ADCDoneBis IT */
	CHECK_ERROR(pmic_event_init(&event_adc_bis));
	event_adc_bis.event = EVENT_ADCDONEI;
	event_adc_bis.callback = (void *)callback_adcbisdone;
	CHECK_ERROR(pmic_event_subscribe(&event_adc_bis));

	/* sub to Touch Screen IT */
	CHECK_ERROR(pmic_event_init(&tsi_event));
	tsi_event.event = EVENT_TSI;
	tsi_event.callback = (void *)callback_tsi;
	CHECK_ERROR(pmic_event_subscribe(&tsi_event));

	/* ADC reading above high limit */
	CHECK_ERROR(pmic_event_init(&adc_comp_h));
	adc_comp_h.event = EVENT_WHIGHI;
	adc_comp_h.callback = (void *)callback_adc_comp_high;
	CHECK_ERROR(pmic_event_subscribe(&adc_comp_h));

#ifdef __TSI_FAILED
	tsi_poll = false;
	kernel_thread(polling_ts, NULL, CLONE_VM | CLONE_FS);
#endif
	return PMIC_SUCCESS;
}

/*!
 * This function disables the ADC, de-registers the interrupt events.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_deinit(void)
{
	event_adc.event = EVENT_ADCBISDONEI;
	event_adc_bis.event = EVENT_ADCDONEI;
	tsi_event.event = EVENT_TSI;
	adc_comp_h.event = EVENT_WHIGHI;

	CHECK_ERROR(pmic_event_unsubscribe(&event_adc));
	CHECK_ERROR(pmic_event_unsubscribe(&event_adc_bis));
	CHECK_ERROR(pmic_event_unsubscribe(&tsi_event));
	CHECK_ERROR(pmic_event_unsubscribe(&adc_comp_h));

	return PMIC_SUCCESS;
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
PMIC_STATUS mc13783_adc_convert(t_adc_param * adc_param)
{
	bool use_bis = false;
	unsigned int adc_0_reg = 0, adc_1_reg = 0, reg_1 = 0, result_reg =
	    0, i = 0;
	unsigned int result = 0;
	int mc13783_ver;
	DEFINE_WAIT(wait);

	TRACEMSG_ADC(_K_D("mc13783 ADC - mc13783_adc_convert ...."));
	if (suspend_flag == 1) {
		return PMIC_ERROR;
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
		TRACEMSG_ADC(_K_D("mc13783 ADC - pmic_write_reg ...."));
		adc_0_reg = 0x001c00 | (ADC_BIS * use_bis);
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_0, adc_0_reg,
					   PMIC_ALL_BITS));
		adc_1_reg = 0x300001 | (ADC_BIS * adc_ts);
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_1, adc_1_reg,
					   PMIC_ALL_BITS));
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
		adc_0_reg = adc_param->read_mode & 0x00003F;
		/* add auto inc */
		adc_0_reg |= ADC_INC;

		if (use_bis) {
			/* add adc bis */
			adc_0_reg |= ADC_BIS;
		}

		mc13783_ver = mxc_pmic_get_ic_version();
		if (mc13783_ver >= 20) {
			if (adc_param->chrgraw_devide_5) {
				adc_0_reg |= ADC_CHRGRAW_D5;
			}
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
		adc_0_reg = 0x003c00 | (ADC_BIS * use_bis) | ADC_INC;
	}
	TRACEMSG_ADC(_K_I("Write Reg %i = %x"), REG_ADC_0, adc_0_reg);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_0, adc_0_reg,
				   PMIC_ALL_BITS));

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
		adc_1_reg = 0x000409 | (ADC_BIS * use_bis) | ADC_NO_ADTRIG;
	}

	reg_1 = adc_1_reg;
	if (use_bis == 0) {
		data_ready_adc_1 = false;
		adc_1_reg |= ASC_ADC;
		data_ready_adc_1 = true;
		TRACEMSG_ADC(_K_I("Write Reg %i = %x"), REG_ADC_1, adc_1_reg);
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_1, adc_1_reg,
					   PMIC_ALL_BITS));
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
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_1, adc_1_reg,
					   PMIC_ALL_BITS));
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
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_1, 4 << ADC_CH_1_POS,
				   ADC_CH_0_MASK | ADC_CH_1_MASK));
	for (i = 0; i <= 3; i++) {
		CHECK_ERROR(pmic_read_reg(PRIO_ADC, result_reg, &result,
					  PMIC_ALL_BITS));
		TRACEMSG_ADC(_K_I("result %i = %x"), result_reg, result);
		adc_param->value[i] = ((result & ADD1_RESULT_MASK) >> 2);
		adc_param->value[i + 4] = ((result & ADD2_RESULT_MASK) >> 14);
	}

	if (adc_param->read_ts) {
		adc_param->ts_value.x_position = adc_param->value[2];
		adc_param->ts_value.y_position = adc_param->value[5];
		adc_param->ts_value.contact_resistance = adc_param->value[6];
	}
	mc13783_adc_release(use_bis);
	return PMIC_SUCCESS;
}

/*!
 * This function select the required read_mode for a specific channel.
 *
 * @param        channel   The channel to be sampled
 *
 * @return       This function returns the requires read_mode
 */
t_reading_mode mc13783_set_read_mode(t_channel channel)
{
	t_reading_mode read_mode = 0;

	switch (channel) {
	case LICELL:
		read_mode = M_LITHIUM_CELL;
		break;
	case CHARGE_CURRENT:
		read_mode = M_CHARGE_CURRENT;
		break;
	case BATTERY_CURRENT:
		read_mode = M_BATTERY_CURRENT;
		break;
	case THERMISTOR:
		read_mode = M_THERMISTOR;
		break;
	case DIE_TEMP:
		read_mode = M_DIE_TEMPERATURE;
		break;
	case USB_ID:
		read_mode = M_UID;
		break;
	default:
		read_mode = 0;
	}

	return read_mode;
}

/*!
 * This function triggers a conversion and returns one sampling result of one
 * channel.
 *
 * @param        channel   The channel to be sampled
 * @param        result    The pointer to the conversion result. The memory
 *                         should be allocated by the caller of this function.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_convert(t_channel channel, unsigned short *result)
{
	t_adc_param adc_param;
	PMIC_STATUS ret;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	mc13783_adc_init_param(&adc_param);
	TRACEMSG_ADC(_K_D("pmic_adc_convert"));
	adc_param.read_ts = false;
	adc_param.read_mode = mc13783_set_read_mode(channel);

	adc_param.single_channel = true;
	/* Find the group */
	if ((channel >= BATTERY_VOLTAGE) && (channel <= GENERAL_PURPOSE_ADIN7)) {
		adc_param.channel_0 = channel;
		adc_param.group = false;
	} else if ((channel >= TS_X_POSITION1) && (channel <= USB_ID)) {
		adc_param.channel_0 = channel;
		adc_param.group = true;
	} else {
		return PMIC_PARAMETER_ERROR;
	}

	ret = mc13783_adc_convert(&adc_param);

	*result = adc_param.value[0];

	return ret;
}

/*!
 * This function triggers a conversion and returns eight sampling results of
 * one channel.
 *
 * @param        channel   The channel to be sampled
 * @param        result    The pointer to array to store eight sampling results.
 *                         The memory should be allocated by the caller of this
 *                         function.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_convert_8x(t_channel channel, unsigned short *result)
{
	t_adc_param adc_param;
	int i;
	PMIC_STATUS ret;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	mc13783_adc_init_param(&adc_param);
	TRACEMSG_ADC(_K_D("pmic_adc_convert_8x"));
	adc_param.read_ts = false;
	adc_param.single_channel = true;
	adc_param.read_mode = mc13783_set_read_mode(channel);
	if ((channel >= BATTERY_VOLTAGE) && (channel <= GENERAL_PURPOSE_ADIN7)) {
		adc_param.channel_0 = channel;
		adc_param.channel_1 = channel;
		adc_param.group = false;
	} else if ((channel >= TS_X_POSITION1) && (channel <= USB_ID)) {
		adc_param.channel_0 = channel;
		adc_param.channel_1 = channel;
		adc_param.group = true;
	} else {
		return PMIC_PARAMETER_ERROR;
	}

	ret = mc13783_adc_convert(&adc_param);

	for (i = 0; i <= 7; i++) {
		result[i] = adc_param.value[i];
	}

	return ret;
}

/*!
 * This function triggers a conversion and returns sampling results of each
 * specified channel.
 *
 * @param        channels  This input parameter is bitmap to specify channels
 *                         to be sampled.
 * @param        result    The pointer to array to store sampling results.
 *                         The memory should be allocated by the caller of this
 *                         function.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_convert_multichnnel(t_channel channels,
					 unsigned short *result)
{
	t_adc_param adc_param;
	int i;
	PMIC_STATUS ret;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}

	mc13783_adc_init_param(&adc_param);
	TRACEMSG_ADC(_K_D("pmic_adc_convert_multichnnel"));
	adc_param.read_ts = false;

	adc_param.single_channel = false;
	if ((channels >= BATTERY_VOLTAGE) &&
	    (channels <= GENERAL_PURPOSE_ADIN7)) {
		adc_param.channel_0 = channels;
		adc_param.channel_1 = ((channels + 4) % 4) + 4;
		adc_param.group = false;
	} else if ((channels >= TS_X_POSITION1) && (channels <= USB_ID)) {
		adc_param.channel_0 = channels;
		adc_param.channel_1 = ((channels + 4) % 8) + 8;
		adc_param.group = true;
	} else {
		return PMIC_PARAMETER_ERROR;
	}
	adc_param.read_mode = 0x00003f;
	adc_param.read_ts = false;

	ret = mc13783_adc_convert(&adc_param);

	for (i = 0; i <= 7; i++) {
		result[i] = adc_param.value[i];
	}

	return ret;
}

/*!
 * This function sets touch screen operation mode.
 *
 * @param        touch_mode   Touch screen operation mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_set_touch_mode(t_touch_mode touch_mode)
{
	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_write_reg(PRIO_ADC,
				   REG_ADC_0,
				   BITFVAL(MC13783_ADC0_TS_M, touch_mode),
				   BITFMASK(MC13783_ADC0_TS_M)));
	return PMIC_SUCCESS;
}

/*!
 * This function retrieves the current touch screen operation mode.
 *
 * @param        touch_mode   Pointer to the retrieved touch screen operation
 *                            mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_get_touch_mode(t_touch_mode * touch_mode)
{
	unsigned int value;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_ADC, REG_ADC_0, &value, PMIC_ALL_BITS));

	*touch_mode = BITFEXT(value, MC13783_ADC0_TS_M);

	return PMIC_SUCCESS;
}

/*!
 * This function retrieves the current touch screen (X,Y) coordinates.
 *
 * @param        touch_sample Pointer to touch sample.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_get_touch_sample(t_touch_sample * touch_sample)
{
	return mc13783_adc_read_ts(touch_sample, true);
}

/*!
 * This function read the touch screen value.
 *
 * @param        ts_value    return value of touch screen
 * @param        wait_tsi    if true, this function is synchronous (wait in TSI event).
 *
 * @return       This function returns 0.
 */
PMIC_STATUS mc13783_adc_read_ts(t_touch_sample * touch_sample, int wait_tsi)
{
	t_adc_param param;
	TRACEMSG_ADC(_K_D("mc13783_adc : mc13783_adc_read_ts"));
	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	if (wait_ts) {
		TRACEMSG_ADC(_K_D("mc13783_adc : error TS busy "));
		return PMIC_ERROR;
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
	if ((param.ts_value.x_position < TS_X_MAX) &&
	    (param.ts_value.x_position > TS_X_MIN) &&
	    (param.ts_value.y_position < TS_Y_MAX) &&
	    (param.ts_value.y_position > TS_Y_MIN)) {
		touch_sample->x_position = param.ts_value.x_position;
		touch_sample->y_position = param.ts_value.y_position;
		touch_sample->pressure = param.ts_value.contact_resistance + 1;
	} else {
		touch_sample->x_position = 0;
		touch_sample->y_position = 0;
		touch_sample->pressure = 0;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function starts a Battery Current mode conversion.
 *
 * @param        mode      Conversion mode.
 * @param        result    Battery Current measurement result.
 *                         if \a mode = ADC_8CHAN_1X, the result is \n
 *                             result[0] = (BATTP - BATT_I) \n
 *                         if \a mode = ADC_1CHAN_8X, the result is \n
 *                             result[0] = BATTP \n
 *                             result[1] = BATT_I \n
 *                             result[2] = BATTP \n
 *                             result[3] = BATT_I \n
 *                             result[4] = BATTP \n
 *                             result[5] = BATT_I \n
 *                             result[6] = BATTP \n
 *                             result[7] = BATT_I
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_get_battery_current(t_conversion_mode mode,
					 unsigned short *result)
{
	PMIC_STATUS ret;
	t_channel channel;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	channel = BATTERY_CURRENT;

	if (mode == ADC_8CHAN_1X) {
		ret = pmic_adc_convert(channel, result);
	} else {
		ret = pmic_adc_convert_8x(channel, result);
	}
	return PMIC_SUCCESS;
}

/*!
 * This function request a ADC.
 *
 * @return      This function returns index of ADC to be used (0 or 1) if successful.
                return -1 if error.
 */
int mc13783_adc_request(void)
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
int mc13783_adc_release(int adc_index)
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
PMIC_STATUS pmic_adc_install_ts(void)
{
#ifndef __TSI_FAILED
	int adc_0_reg = 0, adc_1_reg = 0;
#endif
	TRACEMSG_ADC(_K_D("mc13783_adc : install TS"));
	if (read_ts_installed) {
		TRACEMSG_ADC(_K_D
			     ("mc13783_adc : Error TS is already installed"));
		return PMIC_ERROR;
	}
#ifdef __TSI_FAILED
	tsi_poll = true;
#else
	/* we need to set ADCEN 1 for TSI interrupt on mc13783 1.x */
	/* configure adc to wait tsi interrupt */
	adc_0_reg = 0x001c00 | (ADC_BIS * adc_ts);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_0, adc_0_reg,
				   PMIC_ALL_BITS));
	adc_1_reg = 0x300001 | (ADC_BIS * adc_ts);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_1, adc_1_reg,
				   PMIC_ALL_BITS));
#endif
	read_ts_installed = true;
	return PMIC_SUCCESS;
}

/*!
 * This function disables the touch screen read interface.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_adc_remove_ts(void)
{
	TRACEMSG_ADC(_K_D("mc13783_adc : remove TS"));
	if (read_ts_installed) {
		mc13783_adc_release(adc_ts);
		read_ts_installed = false;
	}
#ifdef __TSI_FAILED
	tsi_poll = false;
#endif
	return PMIC_SUCCESS;
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
 * This function actives the comparator.  When comparator is active and ADC
 * is enabled, the 8th converted value will be digitally compared against the
 * window defined by WLOW and WHIGH registers.
 *
 * @param        low      Comparison window low threshold (WLOW).
 * @param        high     Comparison window high threshold (WHIGH).
 * @param        callback Callback function to be called when the converted
 *                        value is beyond the comparison window.  The callback
 *                        function will pass a parameter of type
 *                        \b t_comp_expection to indicate the reason of
 *                        comparator exception.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_active_comparator(unsigned char low,
				       unsigned char high,
				       t_channel channel,
				       t_comparator_cb callback)
{
	bool use_bis = false;
	unsigned int adc_0_reg = 0, adc_1_reg = 0, adc_3_reg = 0;
	t_monitoring_param monitoring;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	if (monitor_en) {
		TRACEMSG_ADC(_K_D
			     ("mc13783_adc : monitoring already configured"));
		return PMIC_ERROR;
	}
	monitor_en = true;
	mc13783_adc_init_monitor_param(&monitoring);
	monitoring.comp_low = low;
	monitoring.comp_high = high;
	monitoring.channel = channel;
	monitoring.callback = (void *)callback;

	use_bis = mc13783_adc_request();
	if (use_bis < 0) {
		TRACEMSG_ADC(_K_D("mc13783_adc : request error"));
		return PMIC_ERROR;
	}
	monitor_adc = use_bis;

	adc_0_reg = 0;

	/* TO DO ADOUT CONFIGURE */
	adc_0_reg = monitoring.read_mode & ADC_MODE_MASK;
	if (use_bis) {
		/* add adc bis */
		adc_0_reg |= ADC_BIS;
	}
	adc_0_reg |= ADC_WCOMP;

	/* CONFIGURE ADC REG 1 */
	adc_1_reg = 0;
	adc_1_reg |= ADC_EN;
	if (monitoring.conv_delay) {
		adc_1_reg |= ADC_ATO;
	}
	if (monitoring.group) {
		adc_1_reg |= ADC_ADSEL;
	}
	adc_1_reg |= (monitoring.channel << ADC_CH_0_POS) & ADC_CH_0_MASK;
	adc_1_reg |= (monitoring.delay << ADC_DELAY_POS) & ADC_DELAY_MASK;
	if (use_bis) {
		adc_1_reg |= ADC_BIS;
	}

	adc_3_reg |= (monitoring.comp_high << ADC_WCOMP_H_POS) &
	    ADC_WCOMP_H_MASK;
	adc_3_reg |= (monitoring.comp_low << ADC_WCOMP_L_POS) &
	    ADC_WCOMP_L_MASK;
	if (use_bis) {
		adc_3_reg |= ADC_BIS;
	}

	wcomp_mode = monitoring.check_mode;
	/* call back to be called when event is detected. */
	monitoring_cb = monitoring.callback;

	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_0, adc_0_reg,
				   PMIC_ALL_BITS));
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_1, adc_1_reg,
				   PMIC_ALL_BITS));
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_3, adc_3_reg,
				   PMIC_ALL_BITS));
	return PMIC_SUCCESS;
}

/*!
 * This function deactivates the comparator.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_deactive_comparator(void)
{
	unsigned int reg_value = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	if (!monitor_en) {
		TRACEMSG_ADC(_K_D("mc13783_adc : adc monitoring free"));
		return PMIC_ERROR;
	}

	if (monitor_en) {
		reg_value = ADC_BIS;
	}

	/* clear all reg value */
	CHECK_ERROR(pmic_write_reg
		    (PRIO_ADC, REG_ADC_0, reg_value, PMIC_ALL_BITS));
	CHECK_ERROR(pmic_write_reg
		    (PRIO_ADC, REG_ADC_1, reg_value, PMIC_ALL_BITS));
	CHECK_ERROR(pmic_write_reg
		    (PRIO_ADC, REG_ADC_3, reg_value, PMIC_ALL_BITS));

	reg_value = 0;

	if (monitor_adc) {
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_4, reg_value,
					   PMIC_ALL_BITS));
	} else {
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC_2, reg_value,
					   PMIC_ALL_BITS));
	}

	mc13783_adc_release(monitor_adc);
	monitor_en = false;
	return PMIC_SUCCESS;
}

static unsigned int pmic_adc_poll(struct file *filp, poll_table * wait)
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
 * @return       This function returns 0 if successful.
 */
static int pmic_adc_ioctl(struct inode *inode, struct file *file,
			  unsigned int cmd, unsigned long arg)
{
	t_adc_convert_param *convert_param;
	t_touch_mode touch_mode;
	t_touch_sample touch_sample;
	unsigned short b_current;
	t_adc_comp_param *comp_param;

	if ((_IOC_TYPE(cmd) != 'p') && (_IOC_TYPE(cmd) != 'D'))
		return -ENOTTY;

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}

	switch (cmd) {
	case PMIC_ADC_INIT:
		TRACEMSG_ADC(_K_D("init adc"));
		CHECK_ERROR(pmic_adc_init());
		break;
	case PMIC_ADC_DEINIT:
		TRACEMSG_ADC(_K_D("deinit adc"));
		CHECK_ERROR(pmic_adc_deinit());
		break;
	case PMIC_ADC_CONVERT:
		if ((convert_param = kmalloc(sizeof(t_adc_convert_param),
					     GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(convert_param, (t_adc_convert_param *) arg,
				   sizeof(t_adc_convert_param))) {
			kfree(convert_param);
			return -EFAULT;
		}
		CHECK_ERROR(pmic_adc_convert(convert_param->channel,
					     convert_param->result));

		if (copy_to_user((t_adc_convert_param *) arg, convert_param,
				 sizeof(t_adc_convert_param))) {
			return -EFAULT;
		}
		kfree(convert_param);
		break;
	case PMIC_ADC_CONVERT_8X:
		if ((convert_param = kmalloc(sizeof(t_adc_convert_param),
					     GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(convert_param, (t_adc_convert_param *) arg,
				   sizeof(t_adc_convert_param))) {
			kfree(convert_param);
			return -EFAULT;
		}
		CHECK_ERROR(pmic_adc_convert_8x(convert_param->channel,
						convert_param->result));

		if (copy_to_user((t_adc_convert_param *) arg, convert_param,
				 sizeof(t_adc_convert_param))) {
			return -EFAULT;
		}
		kfree(convert_param);
		break;

	case PMIC_ADC_CONVERT_MULTICHANNEL:
		if ((convert_param = kmalloc(sizeof(t_adc_convert_param),
					     GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(convert_param, (t_adc_convert_param *) arg,
				   sizeof(t_adc_convert_param))) {
			kfree(convert_param);
			return -EFAULT;
		}

		CHECK_ERROR(pmic_adc_convert_multichnnel(convert_param->channel,
							 convert_param->
							 result));

		if (copy_to_user((t_adc_convert_param *) arg, convert_param,
				 sizeof(t_adc_convert_param))) {
			return -EFAULT;
		}
		kfree(convert_param);
		break;

	case PMIC_ADC_SET_TOUCH_MODE:
		CHECK_ERROR(pmic_adc_set_touch_mode((t_touch_mode) arg));
		break;

	case PMIC_ADC_GET_TOUCH_MODE:
		CHECK_ERROR(pmic_adc_get_touch_mode(&touch_mode));
		if (copy_to_user((t_touch_mode *) arg, &touch_mode,
				 sizeof(t_touch_mode))) {
			return -EFAULT;
		}
		break;

	case PMIC_ADC_GET_TOUCH_SAMPLE:
		TRACEMSG("pmic_adc_ioctl: " "PMIC_ADC_GET_TOUCH_SAMPLE\n");

		CHECK_ERROR(pmic_adc_get_touch_sample(&touch_sample));
		if (copy_to_user((t_touch_sample *) arg, &touch_sample,
				 sizeof(t_touch_sample))) {
			return -EFAULT;
		}
		break;

	case PMIC_ADC_GET_BATTERY_CURRENT:
		CHECK_ERROR(pmic_adc_get_battery_current(ADC_8CHAN_1X,
							 &b_current));
		if (copy_to_user((unsigned short *)arg, &b_current,
				 sizeof(unsigned short))) {
			return -EFAULT;
		}
		break;

	case PMIC_ADC_ACTIVATE_COMPARATOR:
		if ((comp_param = kmalloc(sizeof(t_adc_comp_param), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(comp_param, (t_adc_comp_param *) arg,
				   sizeof(t_adc_comp_param))) {
			kfree(comp_param);
			return -EFAULT;
		}
		CHECK_ERROR(pmic_adc_active_comparator(comp_param->wlow,
						       comp_param->whigh,
						       comp_param->channel,
						       comp_param->callback));
		break;

	case PMIC_ADC_DEACTIVE_COMPARATOR:
		CHECK_ERROR(pmic_adc_deactive_comparator());
		break;

	case PMIC_TOUCH_SCREEN_READ_INSTALL:
		TRACEMSG_ADC(_K_D("test adc touch screen"));
		pmic_adc_install_ts();
		break;

	case PMIC_TOUCH_SCREEN_READ_UNINSTALL:
		TRACEMSG_ADC(_K_D("test adc touch screen unsubscribe"));
		pmic_adc_remove_ts();
		break;

	default:
		TRACEMSG("pmic_adc_ioctl: unsupported ioctl command 0x%x\n",
			 cmd);
		return -EINVAL;
	}
	return 0;
}

static struct file_operations mc13783_adc_fops = {
	.owner = THIS_MODULE,
	.poll = pmic_adc_poll,
	.read = pmic_adc_read,
	.ioctl = pmic_adc_ioctl,
	.open = pmic_adc_open,
	.release = pmic_adc_free,
};

/*
 * Initialization and Exit
 */
static int __init pmic_adc_module_init(void)
{
	int i = 0;
	int ret = 0;
	pmic_adc_major = register_chrdev(0, "pmic_adc", &mc13783_adc_fops);

	if (pmic_adc_major < 0) {
		TRACEMSG_ADC(_K_D("Unable to get a major for pmic_adc"));
		return pmic_adc_major;
	}
	init_waitqueue_head(&suspendq);

	devfs_mk_cdev(MKDEV(pmic_adc_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "pmic_adc");

	/* Allocate the buffer */
	buffer = (short *)kmalloc(buf_size * sizeof(short), GFP_KERNEL);
	if (buffer == NULL) {
		TRACEMSG_ADC(_K_D
			     ("unable to allocate buffer for mc13783 adc device\n"));
	}
	CHECK_ERROR(pmic_adc_init());
	for (i = 0; i < buf_size; i++) {
		buffer[i] = 0;
	}

	data_ready = 0;
	buf_count = 0;

	ret = driver_register(&pmic_adc_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&pmic_adc_ldm);
		if (ret != 0) {
			driver_unregister(&pmic_adc_driver_ldm);
		}
	}

	printk(KERN_INFO "Pmic ADC loading\n");
	return ret;
}

static void __exit pmic_adc_module_exit(void)
{
	pmic_event_unsubscribe(&event_adc);
	pmic_event_unsubscribe(&event_adc_bis);
	pmic_event_unsubscribe(&tsi_event);
	pmic_event_unsubscribe(&adc_comp_h);
	devfs_remove("pmic_adc");
	unregister_chrdev(pmic_adc_major, "pmic_adc");
	driver_unregister(&pmic_adc_driver_ldm);
	platform_device_unregister(&pmic_adc_ldm);
	TRACEMSG_ADC("pmic adc : successfully unloaded\n");
}

/*
 * Module entry points
 */

module_init(pmic_adc_module_init);
module_exit(pmic_adc_module_exit);

MODULE_DESCRIPTION("PMIC ADC device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
