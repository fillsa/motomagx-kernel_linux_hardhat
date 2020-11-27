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
 * @file mc13783_adc_defs.h
 * @brief This header contains all defines.
 *
 * @ingroup MC13783_ADC
 */

#ifndef _MC13783_ADC__DEFS_H
#define _MC13783_ADC__DEFS_H

#define MC13783_ADC_DEVICE "/dev/mc13783_adc"

/*
#define __ALL_TRACES__
*/
#ifdef __ALL_TRACES__
#define TRACEMSG_ADC(fmt,args...)  printk(fmt,##args)
#else				/* __ALL_TRACES__ */
#define TRACEMSG_ADC(fmt,args...)
#endif				/* __ALL_TRACES__ */

/*
 * Define
 */
#define         DEF_ADC_0     0x008000
#define         DEF_ADC_3     0x000080

#define ADC_NB_AVAILABLE        2

#define MAX_CHANNEL             7

#define TS_X_MAX                1200
#define TS_Y_MAX                1200

#define TS_X_MIN                45
#define TS_Y_MIN                45

/*
 * Maximun allowed variation in the three X/Y co-ordinates acquired from
 * touch-screen
 */
#define DELTA_Y_MAX 50
#define DELTA_X_MAX 50

/* Upon clearing the filter, this is the delay in restarting the filter */
#define FILTER_MIN_DELAY 4

/* Length of X and Y Touch screen filters */
#define FILTLEN 3

/* Defined to log all TS filtering messages and values */
#define TS_LOG_VALUES 2

#define NB_ADC_REG      5
#define MC13783_LOAD_ERROR_MSG            \
"mc13783 card was not correctly detected. Stop loading mc13783 ADC driver\n"

/*
 * ADC 0
 */
#define ADC_WAIT_TSI_0		0x001C00
#define ADC_POS_MODE_0		0x003C00
#define ADC_CONF_0		0x00003F

/*
 * ADC 1
 */
#define ADC_EN                  0x000001
#define ADC_SGL_CH              0x000002
#define ADC_ADSEL               0x000008
#define ADC_CH_0_POS            5
#define ADC_CH_0_MASK           0x0000E0
#define ADC_CH_1_POS            8
#define ADC_CH_1_MASK           0x000700
#define ADC_DELAY_POS           11
#define ADC_DELAY_MASK          0x07F800
#define ADC_ATO                 0x080000
#define ASC_ADC                 0x100000
#define ADC_CHRGRAW_D5          0x008000
#define ADC_WAIT_TSI_1		0x300001
#define ADC_POS_MODE_1		0x000409

/*
 * ADC 2 - 4
 */
#define ADD1_RESULT_MASK        0x00000FFC
#define ADD2_RESULT_MASK        0x00FFC000
#define ADC_TS_MASK             0x00FFCFFC

/*
 * ADC 3
 */
#define ADC_INC                 0x030000
#define ADC_BIS                 0x800000
#define ADC_NO_ADTRIG           0x200000
#define ADC_WCOMP               0x040000
#define ADC_WCOMP_H_POS         0
#define ADC_WCOMP_L_POS         9
#define ADC_WCOMP_H_MASK        0x00003F
#define ADC_WCOMP_L_MASK        0x007E00

#define ADC_MODE_MASK           0x00003F

/*
 * Interrupt Status 0
 */
#define ADC_INT_BISDONEI        0x000020

/*!
 * Define state mode of ADC.
 */
typedef enum adc_state {
	/*!
	 * Free.
	 */
	ADC_FREE,
	/*!
	 * Used.
	 */
	ADC_USED,
	/*!
	 * Monitoring
	 */
	ADC_MONITORING,
} t_adc_state;

/*!
 * This function is used to update buffer of touch screen value in read mode.
 */
static void update_buffer(void);

/*!
 * This function performs filtering and rejection of excessive noise prone
 * samples.
 *
 * @param        ts_curr     Touch screen value
 *
 * @return       This function returns 0 on success, -1 otherwise.
 */
static int mc13783_adc_filter(t_touch_screen * ts_curr);

/*!
 * This function read the touch screen value.
 *
 * @param        ts_value    return value of touch screen
 * @param        wait_tsi    if true, this function is synchronous
                             (wait in TSI event).
 *
 * @return       This function returns 0.
 */
static int mc13783_adc_read_ts(t_touch_screen * ts_value, int wait_tsi);

/*!
 * This function request a ADC.
 *
 * @return      This function returns index of ADC to be used (0 or 1)
                if successful.
                return -1 if error.
 */
static int mc13783_adc_request(void);

/*!
 * This function release an ADC.
 *
 * @param        adc_index     index of ADC to be released.
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_adc_release(int adc_index);

/*!
 * This function enables the touch screen read interface.
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_adc_install_ts(void);

/*!
 * This function disables the touch screen read interface.
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_adc_remove_ts(void);

#endif				/* _MC13783_ADC__DEFS_H */
