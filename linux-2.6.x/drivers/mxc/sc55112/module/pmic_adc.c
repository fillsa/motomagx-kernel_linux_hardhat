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
 * @brief This is the main file of sc55112 Digitizer driver.
 *
 * @ingroup PMIC_sc55112_ADC
 */

/*
 * Includes
 */
#include <asm/ioctl.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/wait.h>

#include "../core/sc55112_config.h"
#include "asm/arch/pmic_status.h"
#include <asm/arch/pmic_external.h>
#include "asm/arch/pmic_power.h"

#include "asm/arch/pmic_adc.h"
#include "sc55112_adc_regs.h"

static int pmic_adc_major;
static const char *PMIC_ADC_DEVICE_NAME = "sc55112_adc";

static type_event_notification adcdone_event;
static type_event_notification tsi_event;
static type_event_notification whi_event;
static type_event_notification wli_event;
static bool adc_waiting_data;
static t_comparator_cb comp_cb;
static int data_ready;
static int buf_size = 4096;	/* Buffer size */
static short *buffer;
static int buf_count;
static bool read_ts_installed = false;

static DECLARE_WAIT_QUEUE_HEAD(queue_tsi_read);
static DECLARE_WAIT_QUEUE_HEAD(adcdone_it);
static DECLARE_WAIT_QUEUE_HEAD(adc_tsi);
static DECLARE_WAIT_QUEUE_HEAD(pen_down);

/*
 * PMIC ADC API
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

/*!
 * This is the suspend of power management for the sc55112 ADC API.
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
	/* not supported */
	return -1;
};

/*!
 * This is the resume of power management for the sc55112 ADC API.
 * It supports RESTORE state.
 *
 * @param        dev            the device
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int pmic_adc_resume(struct device *dev, u32 level)
{
	/* not supported */
	return -1;
};

static struct device_driver pmic_adc_driver_ldm = {
	.name = "PMIC_ADC",
	.bus = &platform_bus_type,
	.probe = NULL,
	.remove = NULL,
	.suspend = pmic_adc_suspend,
	.resume = pmic_adc_resume,
};

/*!
 * This is the callback function called on ADCDone PMIC event.
 */
static void callback_adcdone(void *param)
{
	if (adc_waiting_data) {
		wake_up(&adcdone_it);
	}
}

/*!
 * This function is used to update buffer of touch screen value in read mode.
 */
int update_buffer(void *not_used)
{
	t_touch_sample ts_value = { 0, 0, 0 };

	DEFINE_WAIT(wait);

	do {
		/*
		 * Wait for a pen down event notification.
		 */
		prepare_to_wait(&pen_down, &wait, TASK_INTERRUPTIBLE);
		schedule();
		finish_wait(&pen_down, &wait);

		TRACEMSG("Got pen_down event.\n");

		/*
		 * Get the touchpanel (X,Y) and pressure samples. Continue
		 * getting samples until the pressure is zero (i.e., the
		 * user is no longer pressing on the touchpanel).
		 *
		 * Note that we must initialize the pressure to zero before
		 * entering the sampling loop so that we avoid the msleep()
		 * delay that is required between successive samples.
		 */
		ts_value.pressure = 0;
		do {
			if (ts_value.pressure > 0) {
				/* Sleep for 100 milliseconds before resampling
				 * the touchpanel. We must do this between
				 * touchpanel samples so that the (X,Y)
				 * coordinates have a fixed and known time
				 * interval which can then be used to determine
				 * the relative speed between samples.
				 */
				msleep(100);
			}

			/* Call our PMIC function to obtain the touchpanel's
			 * (X,Y) coordinate and pressure readings.
			 */
			pmic_adc_get_touch_sample(&ts_value);

			TRACEMSG("touchpanel (x=%d, y=%d, p=%d)\n",
				 ts_value.x_position,
				 ts_value.y_position, ts_value.pressure);

			/* Add the new touchpanel sample to the data queue if
			 * there is still room available. Note that we must
			 * return a final sample with the (X,Y) and pressure
			 * values all zero to mark the end of a group of
			 * samples.
			 */
			if (buf_count < (buf_size - 4)) {
				buffer[buf_count + 0] =
				    (short)ts_value.pressure;
				buffer[buf_count + 1] =
				    (short)ts_value.x_position;
				buffer[buf_count + 2] =
				    (short)ts_value.y_position;
				buffer[buf_count + 3] = 0;	/* not used */

				buf_count += 4;
			}

			/* Signal the application to read the touchpanel
			 * samples that are currently available in the data
			 * queue.
			 */
			data_ready = 1;
			wake_up(&queue_tsi_read);

		} while (ts_value.pressure >= 1);

	} while (read_ts_installed);

	return 0;
}

/*!
 * This is the callback function called on TSI PMIC event, used in synchronous
 * call.
 */
static void callback_tsi(void *param)
{
	if (read_ts_installed) {
		wake_up(&pen_down);
	}
}

static void ts_read_install(void)
{
	unsigned short val;

	/* Subscribe to Touch Screen interrupt */
	tsi_event.event = EVENT_TSI;
	tsi_event.callback = callback_tsi;
	tsi_event.param = NULL;
	pmic_event_subscribe(tsi_event);

	/* Enable V2 */
	pmic_power_regulator_on(V2);

	/* set touch screen in standby mode */
	pmic_adc_set_touch_mode(TS_STANDBY);

	/* Do a conversion
	 * This is a bug of sc55112 -- we have to do a conversion to
	 * pull up voltage of TSX1 and TSY1
	 */
	pmic_adc_convert(AD7, &val);

	read_ts_installed = true;

	kernel_thread(update_buffer, NULL, CLONE_VM | CLONE_FS);
}

static void ts_read_uninstall(void)
{
	/* Unsubscribe the Touch Screen interrupt. */
	tsi_event.event = EVENT_TSI;
	pmic_event_unsubscribe(tsi_event);
	read_ts_installed = false;

	pmic_adc_set_touch_mode(TS_NONE);
}

/*!
 * This function initializes all ADC registers with default values. This
 * function also registers the interrupt events.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_init(void)
{
	unsigned int value, mask;
	adc_waiting_data = false;
	comp_cb = NULL;

	/* Subscribe to ADCDONE interrupt */
	adcdone_event.event = EVENT_ADCDONE2I;
	adcdone_event.callback = callback_adcdone;
	adcdone_event.param = NULL;
	CHECK_ERROR(pmic_event_subscribe(adcdone_event));

	/* Enable ADC */
	value = BITFVAL(sc55112_ADC1_ADEN, sc55112_ADC1_ADEN_ENABLE);
	mask = BITFMASK(sc55112_ADC1_ADEN);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function disables the ADC, deregisters the interrupt events.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_deinit(void)
{
	adcdone_event.event = EVENT_ADCDONE2I;
	tsi_event.event = EVENT_TSI;

	CHECK_ERROR(pmic_event_unsubscribe(adcdone_event));
	CHECK_ERROR(pmic_event_unsubscribe(tsi_event));

	return PMIC_SUCCESS;
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
PMIC_STATUS pmic_adc_convert(unsigned short channel, unsigned short *result)
{
	int grp, ch, liadc, ada;
	unsigned int value, mask;
	DEFINE_WAIT(wait);

	/* Find the group */
	if ((channel & 0xFF) != 0) {
		grp = 0;
		ch = channel;
	} else if ((channel & 0xFF00) != 0) {
		grp = 1;
		ch = channel >> 8;
	} else {
		return PMIC_PARAMETER_ERROR;
	}

	if (channel == LICELL) {
		liadc = sc55112_ADC1_LIADC_ENABLE;
	} else {
		liadc = sc55112_ADC1_LIADC_DISABLE;
	}

	/* Configure ADC */
	value = BITFVAL(sc55112_ADC1_ADEN, sc55112_ADC1_ADEN_ENABLE) |
	    BITFVAL(sc55112_ADC1_RAND, sc55112_ADC1_RAND_GROUP) |
	    BITFVAL(sc55112_ADC1_AD_SEL1, grp) |
	    BITFVAL(sc55112_ADC1_ATOX, sc55112_ADC1_ATOX_DELAY_FIRST) |
	    BITFVAL(sc55112_ADC1_LIADC, liadc);
	mask = BITFMASK(sc55112_ADC1_ADEN) |
	    BITFMASK(sc55112_ADC1_RAND) |
	    BITFMASK(sc55112_ADC1_AD_SEL1) |
	    BITFMASK(sc55112_ADC1_ATOX) | BITFMASK(sc55112_ADC1_LIADC);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

	/* Start Conversion */
	value = BITFVAL(sc55112_ADC2_ADINC1, sc55112_ADC2_ADINC1_NO_INCR) |
	    BITFVAL(sc55112_ADC2_ADINC2, sc55112_ADC2_ADINC2_NO_INCR) |
	    BITFVAL(sc55112_ADC2_ASC, sc55112_ADC2_ASC_START_ADC);
	mask = BITFMASK(sc55112_ADC2_ADINC1) |
	    BITFMASK(sc55112_ADC2_ADINC2) | BITFMASK(sc55112_ADC2_ASC);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC2, value, mask));

	/* Waiting for ADC done */
	adc_waiting_data = true;
	prepare_to_wait(&adcdone_it, &wait, TASK_INTERRUPTIBLE);
	schedule();
	finish_wait(&adcdone_it, &wait);
	adc_waiting_data = false;

	/* Read result */
	for (ada = 0; ada < 8; ada++) {
		if (((ch >> ada) & 0x1) != 0) {
			break;
		}
	}

	value = BITFVAL(sc55112_ADC1_ADA1, ada);
	mask = BITFMASK(sc55112_ADC1_ADA1);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

	CHECK_ERROR(pmic_read_reg(PRIO_ADC, REG_ADC2, &value));

	*result = BITFEXT(value, sc55112_ADC2_ADD1);

	return PMIC_SUCCESS;
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
PMIC_STATUS pmic_adc_convert_8x(unsigned short channel, unsigned short *result)
{
	int grp, ch, liadc;
	unsigned int value, mask;
	int i;
	DEFINE_WAIT(wait);

	/* Find the group */
	if ((channel & 0xFF) != 0) {
		grp = 0;
		ch = channel;
	} else {
		grp = 1;
		ch = channel >> 8;
	}

	if (channel == LICELL) {
		liadc = sc55112_ADC1_LIADC_ENABLE;
	} else {
		liadc = sc55112_ADC1_LIADC_DISABLE;
	}

	/* Configure ADC */
	value = BITFVAL(sc55112_ADC1_ADEN, sc55112_ADC1_ADEN_ENABLE) |
	    BITFVAL(sc55112_ADC1_RAND, sc55112_ADC1_RAND_CHANNEL) |
	    BITFVAL(sc55112_ADC1_ADA1, ch) |
	    BITFVAL(sc55112_ADC1_AD_SEL1, grp) |
	    BITFVAL(sc55112_ADC1_ATOX, sc55112_ADC1_ATOX_DELAY_FIRST) |
	    BITFVAL(sc55112_ADC1_LIADC, liadc);
	mask = BITFMASK(sc55112_ADC1_ADEN) |
	    BITFMASK(sc55112_ADC1_RAND) |
	    BITFMASK(sc55112_ADC1_AD_SEL1) |
	    BITFMASK(sc55112_ADC1_ATOX) | BITFMASK(sc55112_ADC1_LIADC);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

	/* Start Conversion */
	value = BITFVAL(sc55112_ADC2_ADINC1, sc55112_ADC2_ADINC1_INCR) |
	    BITFVAL(sc55112_ADC2_ADINC2, sc55112_ADC2_ADINC2_INCR) |
	    BITFVAL(sc55112_ADC2_ASC, sc55112_ADC2_ASC_START_ADC);
	mask = BITFMASK(sc55112_ADC2_ADINC1) |
	    BITFMASK(sc55112_ADC2_ADINC2) | BITFMASK(sc55112_ADC2_ASC);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC2, value, mask));

	/* Waiting for ADC done */
	adc_waiting_data = true;
	prepare_to_wait(&adcdone_it, &wait, TASK_INTERRUPTIBLE);
	schedule();
	finish_wait(&adcdone_it, &wait);
	adc_waiting_data = false;

	/* Read result */
	value = BITFVAL(sc55112_ADC1_ADA1, 0) | BITFVAL(sc55112_ADC1_ADA2, 4);
	mask = BITFMASK(sc55112_ADC1_ADA1) | BITFMASK(sc55112_ADC1_ADA2);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

	for (i = 0; i < 4; i++) {
		CHECK_ERROR(pmic_read_reg(PRIO_ADC, REG_ADC2, &value));
		result[i] = BITFEXT(value, sc55112_ADC2_ADD1);
		result[i + 4] = BITFEXT(value, sc55112_ADC2_ADD2);
	}

	return PMIC_SUCCESS;
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
PMIC_STATUS pmic_adc_convert_multichnnel(unsigned short channels,
					 unsigned short *result)
{
	int grp, liadc;
	unsigned int value, mask;
	unsigned short temp_result[16];
	int i;
	unsigned short *result_out;
	DEFINE_WAIT(wait);

	/* Find the group */
	for (grp = 0; grp < 2; grp++) {

		if ((grp == 0) && ((channels & 0xFF) == 0)) {
			break;
		}

		if ((grp == 1) && ((channels & 0xFF00) == 0)) {
			break;
		}

		if (channels | LICELL) {
			liadc = sc55112_ADC1_LIADC_ENABLE;
		} else {
			liadc = sc55112_ADC1_LIADC_DISABLE;
		}

		/* Configure ADC */
		value = BITFVAL(sc55112_ADC1_ADEN, sc55112_ADC1_ADEN_ENABLE) |
		    BITFVAL(sc55112_ADC1_RAND, sc55112_ADC1_RAND_GROUP) |
		    BITFVAL(sc55112_ADC1_AD_SEL1, grp) |
		    BITFVAL(sc55112_ADC1_ATOX, sc55112_ADC1_ATOX_DELAY_FIRST) |
		    BITFVAL(sc55112_ADC1_LIADC, liadc);
		mask = BITFMASK(sc55112_ADC1_ADEN) |
		    BITFMASK(sc55112_ADC1_RAND) |
		    BITFMASK(sc55112_ADC1_AD_SEL1) |
		    BITFMASK(sc55112_ADC1_ATOX) | BITFMASK(sc55112_ADC1_LIADC);
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

		/* Start Conversion */
		value = BITFVAL(sc55112_ADC2_ADINC1, sc55112_ADC2_ADINC1_INCR) |
		    BITFVAL(sc55112_ADC2_ADINC2, sc55112_ADC2_ADINC2_INCR) |
		    BITFVAL(sc55112_ADC2_ASC, sc55112_ADC2_ASC_START_ADC);
		mask = BITFMASK(sc55112_ADC2_ADINC1) |
		    BITFMASK(sc55112_ADC2_ADINC2) | BITFMASK(sc55112_ADC2_ASC);
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC2, value, mask));

		/* Waiting for ADC done */
		adc_waiting_data = true;
		prepare_to_wait(&adcdone_it, &wait, TASK_INTERRUPTIBLE);
		schedule();
		finish_wait(&adcdone_it, &wait);
		adc_waiting_data = false;

		/* Read result */
		value = BITFVAL(sc55112_ADC1_ADA1, 0) |
		    BITFVAL(sc55112_ADC1_ADA2, 4);
		mask = BITFMASK(sc55112_ADC1_ADA1) |
		    BITFMASK(sc55112_ADC1_ADA2);
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

		for (i = 0; i < 4; i++) {
			CHECK_ERROR(pmic_read_reg(PRIO_ADC, REG_ADC2, &value));
			temp_result[grp * 4 + i] =
			    BITFEXT(value, sc55112_ADC2_ADD1);
			temp_result[grp * 4 + i + 4] =
			    BITFEXT(value, sc55112_ADC2_ADD2);
		}
	}

	/* Copy result to calling function */
	result_out = result;
	for (i = 0; i < 16; i++) {
		if ((channels & (1 << i)) != 0) {
			*result_out = temp_result[i];
			result_out++;
		}
	}

	return PMIC_SUCCESS;
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
	CHECK_ERROR(pmic_write_reg(PRIO_ADC,
				   REG_ADC1,
				   BITFVAL(sc55112_ADC1_TS_M, touch_mode),
				   BITFMASK(sc55112_ADC1_TS_M)));
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

	CHECK_ERROR(pmic_read_reg(PRIO_ADC, REG_ADC1, &value));

	*touch_mode = BITFEXT(value, sc55112_ADC1_TS_M);

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
	unsigned int value, mask;

	DEFINE_WAIT(wait);

	/* Read the (X,Y) coordinates from the touchpanel. */

	/* Configure the ADC for X-Position measurement mode. Note
	 * that we are using TSY2 (instead of TSY1 as documented in
	 * the sc55112 DTS document) in order to workaround a
	 * possible hardware bug. Currently TSY1 always reads back
	 * in the range of 8-12 which is invalid. But it does
	 * appear as if we can get a valid X-coordinate by using
	 * the value from TSY2.
	 */
	value = BITFVAL(sc55112_ADC1_ADEN, sc55112_ADC1_ADEN_ENABLE) |
	    BITFVAL(sc55112_ADC1_RAND, sc55112_ADC1_RAND_GROUP) |
	    BITFVAL(sc55112_ADC1_AD_SEL1, 1) |
	    BITFVAL(sc55112_ADC1_ATOX, sc55112_ADC1_ATOX_DELAY_FIRST) |
	    BITFVAL(sc55112_ADC1_TS_M, TS_X_POSITION) |
	    BITFVAL(sc55112_ADC1_ADA1, sc55112_ADC1_ADA_TSY2) |
	    BITFVAL(sc55112_ADC1_ADA2, sc55112_ADC1_ADA_TSY2) |
	    BITFVAL(sc55112_ADC1_TS_REFENB, sc55112_ADC1_TS_REFENB_ENABLE);
	mask = 0xFFFFFF;

	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

	/* Start the ADC conversions. */
	value = BITFVAL(sc55112_ADC2_ASC, sc55112_ADC2_ASC_START_ADC);
	mask = BITFMASK(sc55112_ADC2_ASC);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC2, value, mask));

	/* Wait for the ADC conversions to complete. */
	adc_waiting_data = true;
	prepare_to_wait(&adcdone_it, &wait, TASK_INTERRUPTIBLE);
	schedule();
	finish_wait(&adcdone_it, &wait);
	adc_waiting_data = false;

	/* Read the ADC results that are included within the ADC2 register. */
	CHECK_ERROR(pmic_read_reg(PRIO_ADC, REG_ADC2, &value));

	/* Extract and save the X-coordinate value. */
	touch_sample->x_position = BITFEXT(value, sc55112_ADC2_ADD1);

	/* Configure the ADC for Y-Position measurement mode. This
	 * would normally provide us with both X and Y coordinates
	 * but due to a possible hardware bug with measuring the
	 * X position, we only use the Y-coordinate that is obtained
	 * here.
	 */
	value = BITFVAL(sc55112_ADC1_ADEN, sc55112_ADC1_ADEN_ENABLE) |
	    BITFVAL(sc55112_ADC1_RAND, sc55112_ADC1_RAND_GROUP) |
	    BITFVAL(sc55112_ADC1_AD_SEL1, 1) |
	    BITFVAL(sc55112_ADC1_ATOX, sc55112_ADC1_ATOX_DELAY_FIRST) |
	    BITFVAL(sc55112_ADC1_TS_M, TS_Y_POSITION) |
	    BITFVAL(sc55112_ADC1_ADA1, sc55112_ADC1_ADA_TSX1) |
	    BITFVAL(sc55112_ADC1_ADA2, sc55112_ADC1_ADA_TSY1) |
	    BITFVAL(sc55112_ADC1_TS_REFENB, sc55112_ADC1_TS_REFENB_ENABLE);
	mask = 0xFFFFFF;

	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

	/* Start the ADC conversions. */
	value = BITFVAL(sc55112_ADC2_ASC, sc55112_ADC2_ASC_START_ADC);
	mask = BITFMASK(sc55112_ADC2_ASC);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC2, value, mask));

	/* Wait for the ADC conversions to complete. */
	adc_waiting_data = true;
	prepare_to_wait(&adcdone_it, &wait, TASK_INTERRUPTIBLE);
	schedule();
	finish_wait(&adcdone_it, &wait);
	adc_waiting_data = false;

	/* Read the ADC results that are included within the ADC2 register. */
	CHECK_ERROR(pmic_read_reg(PRIO_ADC, REG_ADC2, &value));

	/* Extract and save the Y-coordinate value. */
	touch_sample->y_position = BITFEXT(value, sc55112_ADC2_ADD1);

	if ((touch_sample->x_position < TS_X_MIN) ||
	    (touch_sample->x_position > TS_X_MAX) ||
	    (touch_sample->y_position < TS_Y_MIN) ||
	    (touch_sample->y_position > TS_Y_MAX)) {
		touch_sample->x_position = 0;
		touch_sample->y_position = 0;
		touch_sample->pressure = 0;

		/* Set the touch screen back to standby mode. */
		pmic_adc_set_touch_mode(TS_STANDBY);

		return PMIC_SUCCESS;
	}

	/* Configure the ADC for Pressure measurement mode. */
	value = BITFVAL(sc55112_ADC1_ADEN, sc55112_ADC1_ADEN_ENABLE) |
	    BITFVAL(sc55112_ADC1_RAND, sc55112_ADC1_RAND_GROUP) |
	    BITFVAL(sc55112_ADC1_AD_SEL1, 1) |
	    BITFVAL(sc55112_ADC1_ATOX, sc55112_ADC1_ATOX_DELAY_FIRST) |
	    BITFVAL(sc55112_ADC1_TS_M, TS_PRESSURE) |
	    BITFVAL(sc55112_ADC1_ADA1, sc55112_ADC1_ADA_TSX1) |
	    BITFVAL(sc55112_ADC1_TS_REFENB, sc55112_ADC1_TS_REFENB_ENABLE);
	mask = 0xFFFFFF;

	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

	/* Start the ADC conversion. */
	value = BITFVAL(sc55112_ADC2_ASC, sc55112_ADC2_ASC_START_ADC);
	mask = BITFMASK(sc55112_ADC2_ASC);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC2, value, mask));

	/* Wait for ADC conversion to be completed. */
	adc_waiting_data = true;
	prepare_to_wait(&adcdone_it, &wait, TASK_INTERRUPTIBLE);
	schedule();
	finish_wait(&adcdone_it, &wait);
	adc_waiting_data = false;

	/* Read the ADC conversion result. */
	CHECK_ERROR(pmic_read_reg(PRIO_ADC, REG_ADC2, &value));

	/* Extract and save the pressure measurement. */
	touch_sample->pressure = BITFEXT(value, sc55112_ADC2_ADD1);

	/* Set the touch screen back to standby mode. */
	pmic_adc_set_touch_mode(TS_STANDBY);

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
	unsigned int value, mask;
	int i;
	DEFINE_WAIT(wait);

	/* Configure ADC */
	value = BITFVAL(sc55112_ADC1_ADEN, sc55112_ADC1_ADEN_ENABLE) |
	    BITFVAL(sc55112_ADC1_RAND, mode) |
	    BITFVAL(sc55112_ADC1_AD_SEL1, 0) |
	    BITFVAL(sc55112_ADC1_BATT_I_ADC, sc55112_ADC1_BATT_I_ADC_ENABLE) |
	    BITFVAL(sc55112_ADC1_ATOX, sc55112_ADC1_ATOX_DELAY_FIRST);
	mask = 0xFFFFFF;

	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));

	/* Start Conversion */
	value = BITFVAL(sc55112_ADC2_ADINC1, sc55112_ADC2_ADINC1_NO_INCR) |
	    BITFVAL(sc55112_ADC2_ADINC2, sc55112_ADC2_ADINC2_NO_INCR) |
	    BITFVAL(sc55112_ADC2_ASC, sc55112_ADC2_ASC_START_ADC);
	mask = BITFMASK(sc55112_ADC2_ADINC1) |
	    BITFMASK(sc55112_ADC2_ADINC2) | BITFMASK(sc55112_ADC2_ASC);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC2, value, mask));

	/* Waiting for ADC done */
	adc_waiting_data = true;
	prepare_to_wait(&adcdone_it, &wait, TASK_INTERRUPTIBLE);
	schedule();
	finish_wait(&adcdone_it, &wait);
	adc_waiting_data = false;

	/* Read result */
	if (mode == ADC_8CHAN_1X) {
		value = BITFVAL(sc55112_ADC1_ADA1, 0);
		mask = BITFMASK(sc55112_ADC1_ADA1);
		CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value, mask));
		result[0] = BITFEXT(value, sc55112_ADC2_ADD1);
	} else {
		for (i = 0; i < 4; i++) {
			value = BITFVAL(sc55112_ADC1_ADA1, 0) |
			    BITFVAL(sc55112_ADC1_ADA2, 4);
			mask = BITFMASK(sc55112_ADC1_ADA1) |
			    BITFMASK(sc55112_ADC1_ADA2);
			CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_ADC1, value,
						   mask));
			result[i] = BITFEXT(value, sc55112_ADC2_ADD1);
			result[i + 4] = BITFEXT(value, sc55112_ADC2_ADD2);
		}
	}

	return PMIC_SUCCESS;
}

/*!
 * This is the callback function called on WHI event.
 */
static void callback_whi(void *param)
{
	TRACEMSG(_K_D("* WHI it callback *"));

	if (comp_cb != NULL) {
		comp_cb(GTWHIGH);
	}
}

/*!
 * This is the callback function called on WLI event.
 */
static void callback_wli(void *param)
{
	TRACEMSG(_K_D("* WLI it callback *"));

	if (comp_cb != NULL) {
		comp_cb(LTWLOW);
	}
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
				       t_comparator_cb callback)
{
	unsigned int value, mask;

	/* Set callback function */
	comp_cb = callback;

	/* Subscribe to comparator high interrupt */
	whi_event.event = EVENT_WHI;
	whi_event.callback = callback_whi;
	whi_event.param = NULL;
	CHECK_ERROR(pmic_event_subscribe(whi_event));

	/* Subscribe to comparator low interrupt */
	wli_event.event = EVENT_WLI;
	wli_event.callback = callback_wli;
	wli_event.param = NULL;
	CHECK_ERROR(pmic_event_subscribe(wli_event));

	/* Set thresholds & enable comparator */
	value = BITFVAL(sc55112_ADC1_ADEN, sc55112_ADC1_ADEN_ENABLE) |
	    BITFVAL(sc55112_RTC_CAL_ENCLK16M, sc55112_RTC_CAL_ENCLK16M_ENABLE) |
	    BITFVAL(sc55112_RTC_CAL_WHIGH, high) |
	    BITFVAL(sc55112_RTC_CAL_WLOW, low);
	mask = BITFMASK(sc55112_ADC1_ADEN) |
	    BITFMASK(sc55112_RTC_CAL_ENCLK16M) |
	    BITFMASK(sc55112_RTC_CAL_WHIGH) | BITFMASK(sc55112_RTC_CAL_WLOW);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_RTC_CAL, value, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function deactivates the comparator.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_deactive_comparator(void)
{
	unsigned int value, mask;

	/* Set thresholds to maximum and minimum to disable comparator */
	value = BITFVAL(sc55112_RTC_CAL_WHIGH, 0xFF) |
	    BITFVAL(sc55112_RTC_CAL_WLOW, 0);
	mask = BITFMASK(sc55112_RTC_CAL_WHIGH) | BITFMASK(sc55112_RTC_CAL_WLOW);
	CHECK_ERROR(pmic_write_reg(PRIO_ADC, REG_RTC_CAL, value, mask));

	/* Unsubscribes interrupt events */
	CHECK_ERROR(pmic_event_subscribe(whi_event));
	CHECK_ERROR(pmic_event_subscribe(wli_event));

	/* Clear callback function */
	comp_cb = NULL;

	return PMIC_SUCCESS;
}

/*!
 * This function implements the open method on a PMIC ADC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_adc_open(struct inode *inode, struct file *file)
{
	TRACEMSG(_K_D("pmic_adc : pmic_adc_open()"));
	return 0;
}

/*!
 * This function implements the release method on a PMIC ADC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_adc_release(struct inode *inode, struct file *file)
{
	TRACEMSG(_K_D("pmic_adc : pmic_adc_free()"));
	return 0;
}

/*!
 * This function is the read interface of PMIC ADC, It returns touch screen value.
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
	int ret = 0, i;
	TRACEMSG(_K_D("pmic_adc_read()"));

	/* return value of buffer with all touch screen value */
	/* is the buffer is empty return 0 */

	if ((buf_count != 0) && (count >= buf_count)) {
		copy_to_user(buf, buffer, buf_count * sizeof(short));
		ret = buf_count;
	} else {
		copy_to_user(buf, buffer, 4 * sizeof(short));
	}

	TRACEMSG(_K_D("pmic_adc : DataReady %d"), data_ready);
	TRACEMSG(_K_D("pmic_adc : contact %d, x %d, y %d, pad %d"),
		 buffer[0], buffer[1], buffer[2], buffer[3]);

	data_ready = 0;
	for (i = 0; i < buf_count; i++) {
		buffer[i] = 0;
	}
	buf_count = 0;

	return ret;
}

static unsigned int pmic_adc_poll(struct file *filp, poll_table * wait)
{
	unsigned int ret = 0;
	poll_wait(filp, &queue_tsi_read, wait);
	if (data_ready) {
		ret = POLLIN | POLLRDNORM;
	} else {
		ret = 0;
	}
	return ret;
}

/*!
 * This function implements IOCTL controls on a PMIC ADC device.
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

	switch (cmd) {
	case PMIC_ADC_INIT:
		pmic_adc_init();
		break;

	case PMIC_ADC_DEINIT:
		pmic_adc_deinit();
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
		CHECK_ERROR_KFREE(pmic_adc_convert(convert_param->channel,
						   convert_param->result),
				  (kfree(convert_param)));

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
		CHECK_ERROR_KFREE(pmic_adc_convert_8x(convert_param->channel,
						      convert_param->result),
				  (kfree(convert_param)));

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

		CHECK_ERROR_KFREE(pmic_adc_convert_multichnnel
				  (convert_param->channel,
				   convert_param->result),
				  (kfree(convert_param)));

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

	case GET_TOUCH_SCREEN_VALUE:
		TRACEMSG("pmic_adc_ioctl: GET_TOUCH_SCREEN_VALUE\n");

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
						       comp_param->callback));
		kfree(comp_param);
		break;

	case PMIC_ADC_DEACTIVE_COMPARATOR:
		CHECK_ERROR(pmic_adc_deactive_comparator());
		break;

	case TOUCH_SCREEN_READ_INSTALL:
		ts_read_install();
		TRACEMSG("pmic_adc_ioctl: TOUCH_SCREEN_READ_INSTALL\n");
		break;

	case TOUCH_SCREEN_READ_UNINSTALL:
		ts_read_uninstall();
		TRACEMSG("pmic_adc_ioctl: TOUCH_SCREEN_READ_UNINSTALL\n");
		break;

	default:
		TRACEMSG("pmic_adc_ioctl: unsupported ioctl command 0x%x\n",
			 cmd);
		return -EINVAL;
	}
	return 0;
}

static struct platform_device pmic_adc_ldm = {
	.name = "PMIC_ADC",
	.id = 1,
};

static struct file_operations pmic_adc_fops = {
	.owner = THIS_MODULE,
	.poll = pmic_adc_poll,
	.read = pmic_adc_read,
	.ioctl = pmic_adc_ioctl,
	.open = pmic_adc_open,
	.release = pmic_adc_release,
};

/*
 * Initialization and Exit
 */
static int __init pmic_adc_module_init(void)
{
	int ret = 0;
	pmic_adc_major = register_chrdev(0, PMIC_ADC_DEVICE_NAME,
					 &pmic_adc_fops);

	if (pmic_adc_major < 0) {
		TRACEMSG(_K_D("Unable to get a major for %s",
			      PMIC_ADC_DEVICE_NAME));
		return pmic_adc_major;
	}

	devfs_mk_cdev(MKDEV(pmic_adc_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      PMIC_ADC_DEVICE_NAME);

	/* Allocate the buffer */
	buffer = (short *)kmalloc(buf_size * sizeof(short), GFP_KERNEL);
	if (buffer == NULL) {
		TRACEMSG(_K_D("Unable to allocate buffer for %s device\n",
			      PMIC_ADC_DEVICE_NAME));
		return -ENOMEM;
	}
	memset(buffer, 0, buf_size * sizeof(short));

	CHECK_ERROR(pmic_adc_init());

	ret = driver_register(&pmic_adc_driver_ldm);

	if (ret == 0) {
		ret = platform_device_register(&pmic_adc_ldm);
		if (ret != 0) {
			driver_unregister(&pmic_adc_driver_ldm);
		} else {
			printk(KERN_INFO "PMIC ADC loaded\n");
		}
	}
	return 0;
}

static void __exit pmic_adc_module_exit(void)
{
	devfs_remove(PMIC_ADC_DEVICE_NAME);
	unregister_chrdev(pmic_adc_major, PMIC_ADC_DEVICE_NAME);
	driver_unregister(&pmic_adc_driver_ldm);
	platform_device_unregister(&pmic_adc_ldm);
	printk(KERN_INFO "PMIC ADC unloaded\n");
}

/*
 * Module entry points
 */

module_init(pmic_adc_module_init);
module_exit(pmic_adc_module_exit);

MODULE_DESCRIPTION("PMIC ADC device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
