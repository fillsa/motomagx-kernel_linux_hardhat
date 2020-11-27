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
 * @defgroup MC13783_ADC mc13783 Digitizer Driver
 * @ingroup MC13783DRVRS
 */

/*!
 * @file mc13783_adc.h
 * @brief This is the header of mc13783 adc driver.
 *
 * @ingroup MC13783_ADC
 */

#ifndef __MC13783_ADC_H__
#define __MC13783_ADC_H__

#include <asm/ioctl.h>

#define ADC_TEST_CODE

/*!
 * IOCTL user space interface.
 */

/*!
 * Initialize ADC register with default value.
 */
#define INIT_ADC                        _IOWR('D',1, int)
/*!
 * Test the ADC convert interface.
 */
#ifdef ADC_TEST_CODE
#define ADC_TEST_CONVERT                _IOWR('D',2, int)
#endif				/*ADC_TEST_CODE */
/*!
 * Return the touch screen value interface.
 */
#define GET_TOUCH_SCREEN_VALUE          _IOWR('D',3, int)
/*!
 * Install touch screen read interface.
 */
#define TOUCH_SCREEN_READ_INSTALL       _IOWR('D',4, int)
/*!
 * Remove touch screen read interface.
 */
#define TOUCH_SCREEN_READ_UNINSTALL     _IOWR('D',5, int)
/*!
 * Test monitoring interface.
 */
#ifdef ADC_TEST_CODE
#define ADC_TEST_MONITORING             _IOWR('D',6, int)
#endif				/* ADC_TEST_CODE */

/*!
 * This structure is used to report touch screen value.
 */
typedef struct {
	/*!
	 * Touch Screen X position
	 */
	unsigned int x_position;
	/*!
	 * Touch Screen X position1
	 */
	unsigned int x_position1;
	/*!
	 * Touch Screen X position2
	 */
	unsigned int x_position2;
	/*!
	 * Touch Screen X position3
	 */
	unsigned int x_position3;
	/*!
	 * Touch Screen Y position
	 */
	unsigned int y_position;
	/*!
	 * Touch Screen Y position1
	 */
	unsigned int y_position1;
	/*!
	 * Touch Screen Y position2
	 */
	unsigned int y_position2;
	/*!
	 * Touch Screen Y position3
	 */
	unsigned int y_position3;
	/*!
	 * Touch Screen contact value
	 */
	unsigned int contact_resistance;
} t_touch_screen;

/*!
 * This enumeration, lists all input channels for ADC.
 */
typedef enum channel {
	/*!
	 * Battery Voltage
	 */
	BATTERY_VOLTAGE,
	/*!
	 * Battery Current
	 */
	BATTERY_CURRENT,
	/*!
	 * Application Supply
	 */
	APPLICATION_SUPPLAY,
	/*!
	 * Charger Voltage
	 */
	CHARGE_VOLTAGE,
	/*!
	 * Charger Current
	 */
	CHARGE_CURRENT,
	/*!
	 * General Purpose ADIN5
	 */
	GENERAL_PURPOSE_ADIN5,
	/*!
	 * General Purpose ADIN6
	 */
	GENERAL_PURPOSE_ADIN6,
	/*!
	 * General Purpose ADIN7
	 */
	GENERAL_PURPOSE_ADIN7,
	/*!
	 * Touch screen X or General Purpose ADIN8
	 */
	TS_X_POSITION1,
	/*!
	 * Touch screen X or General Purpose ADIN9
	 */
	TS_X_POSITION2,
	/*!
	 * Touch screen X or General Purpose ADIN10
	 */
	TS_X_POSITION3,
	/*!
	 * Touch screen Y or General Purpose ADIN11
	 */
	TS_Y_POSITION1,
	/*!
	 * Touch screen Y
	 */
	TS_Y_POSITION2,
	/*!
	 * Touch screen Y
	 */
	TS_Y_POSITION3,
	/*!
	 * Touch screen contact 1
	 */
	TS_CONTACT_RESISTANCE1,
	/*!
	 * Touch screen contact 2
	 */
	TS_CONTACT_RESISTANCE2,
} t_channel;

/*!
 * This enumeration, is used to configure the mode of ADC.
 */
typedef enum reading_mode {
	/*!
	 * Enables lithium cell reading
	 */
	M_LITHIUM_CELL = 0x000001,
	/*!
	 * Enables charge current reading
	 */
	M_CHARGE_CURRENT = 0x000002,
	/*!
	 * Enables battery current reading
	 */
	M_BATTERY_CURRENT = 0x000004,
	/*!
	 * Enables thermistor reading
	 */
	M_THERMISTOR = 0x000008,
	/*!
	 * Enables die temperature reading
	 */
	M_DIE_TEMPERATURE = 0x000010,
	/*!
	 * Enables UID reading
	 */
	M_UID = 0x000020,
} t_reading_mode;

/*!
 * This enumeration, is used to configure the monitoring mode.
 */
typedef enum check_mode {
	/*!
	 * Comparator low level
	 */
	CHECK_LOW,
	/*!
	 * Comparator high level
	 */
	CHECK_HIGH,
	/*!
	 * Comparator low or high level
	 */
	CHECK_LOW_OR_HIGH,
} t_check_mode;

/*!
 * This structure is used to configure and report adc value.
 */
typedef struct {
	/*!
	 * Delay before first conversion
	 */
	unsigned int delay;
	/*!
	 * sets the ATX bit for delay on all conversion
	 */
	bool conv_delay;
	/*!
	 * Sets the single channel mode
	 */
	bool single_channel;
	/*!
	 * Selects the set of inputs
	 */
	bool group;
	/*!
	 * Channel selection 1
	 */
	t_channel channel_0;
	/*!
	 * Channel selection 2
	 */
	t_channel channel_1;
	/*!
	 * Used to configure ADC mode with t_reading_mode
	 */
	t_reading_mode read_mode;
	/*!
	 * Sets the Touch screen mode
	 */
	bool read_ts;
	/*!
	 * Wait TSI event before touch screen reading
	 */
	bool wait_tsi;
	/*!
	 * Sets CHRGRAW scaling to divide by 5
	 * Only supported on 2.0 and higher
	 */
	bool chrgraw_devide_5;
	/*!
	 * Return ADC values
	 */
	unsigned int value[8];
	/*!
	 * Return touch screen values
	 */
	t_touch_screen ts_value;
} t_adc_param;

/*!
 * This structure is used to configure the monitoring mode of ADC.
 */
typedef struct {
	/*!
	 * Delay before first conversion
	 */
	unsigned int delay;
	/*!
	 * sets the ATX bit for delay on all conversion
	 */
	bool conv_delay;
	/*!
	 * Channel selection 1
	 */
	t_channel channel;
	/*!
	 * Selects the set of inputs
	 */
	bool group;
	/*!
	 * Used to configure ADC mode with t_reading_mode
	 */
	unsigned int read_mode;
	/*!
	 * Comparator low level in WCOMP mode
	 */
	unsigned int comp_low;
	/*!
	 * Comparator high level in WCOMP mode
	 */
	unsigned int comp_high;
	/*!
	 * Sets type of monitoring (low, high or both)
	 */
	t_check_mode check_mode;
	/*!
	 * Callback to be launched when event is detected
	 */
	void (*callback) (void);
} t_monitoring_param;

/* EXPORTED FUNCTIONS */

/*!
 * This function is called to initialize all ADC registers with default values.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_init_reg(void);

/*!
 * This function initializes adc_param structure.
 *
 * @param        adc_param     Structure to be initialized.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_init_param(t_adc_param * adc_param);

/*!
 * This function starts the convert.
 *
 * @param        adc_param     contains all adc configure and return value.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_convert(t_adc_param * adc_param);

/*!
 * This function initializes monitoring structure.
 *
 * @param        monitor     Structure to be initialized.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_init_monitor_param(t_monitoring_param * monitor);

/*!
 * This function starts the monitoring ADC convert.
 *
 * @param        monitor     contains all adc monitoring configure.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_monitoring_start(t_monitoring_param * monitor);

/*!
 * This function stops the monitoring ADC convert.
 *
 * @param        monitor     contains all adc monitoring configure.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_adc_monitoring_stop(t_monitoring_param * monitor);

/*!
 * This function is used to tell if mc13783 ADC driver has been correctly loaded.
 *
 * @return       This function returns 1 if ADC was successfully loaded
 * 		 0 otherwise.
 */
int mc13783_adc_loaded(void);

#endif				/* __MC13783_ADC_H__ */
