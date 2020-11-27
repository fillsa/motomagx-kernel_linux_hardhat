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

#ifndef _sc55112_ADC_REGS_H
#define _sc55112_ADC_REGS_H

/*!
 * @file sc55112_adc_regs.h
 * @brief This file contains all sc55112 ADC register definitions.
 *
 * @ingroup PMIC_sc55112_ADC
 */

/*
 * Register bit field positions (left shift)
 */
/* ADC1 */
#define sc55112_ADC1_ADEN_LSH                  0
#define sc55112_ADC1_RAND_LSH                  1
#define sc55112_ADC1_AD_SEL1_LSH               2
#define sc55112_ADC1_ADA1_LSH                  4
#define sc55112_ADC1_ADA2_LSH                  7
#define sc55112_ADC1_ATO_LSH                   10
#define sc55112_ADC1_ATOX_LSH                  14
#define sc55112_ADC1_TS_M_LSH                  17
#define sc55112_ADC1_LIADC_LSH                 20
#define sc55112_ADC1_TS_REFENB_LSH             21
#define sc55112_ADC1_VTERM_EN_LSH              22
#define sc55112_ADC1_BATT_I_ADC_LSH            23

/* ADC2 */
#define sc55112_ADC2_ADD1_LSH                  0
#define sc55112_ADC2_ADD2_LSH                  10
#define sc55112_ADC2_ADINC1_LSH                20
#define sc55112_ADC2_ADINC2_LSH                21
#define sc55112_ADC2_ASC_LSH                   22
#define sc55112_ADC2_ADTRIG_ONESHOT_LSH        23

/* RTC_CAL */
#define sc55112_RTC_CAL_ENCLK16M_LSH           0
#define sc55112_RTC_CAL_CAL_PRD_LSH            1
#define sc55112_RTC_CAL_SMPL_WIN_LSH           5
#define sc55112_RTC_CAL_CAL_MODE_LSH           8
#define sc55112_RTC_CAL_SWCLK_FREQ_LSH         10
#define sc55112_RTC_CAL_WHIGH_LSH              12
#define sc55112_RTC_CAL_WLOW_LSH               18

/*
 * Register bit field width
 */
#define sc55112_ADC1_ADEN_WID                  1
#define sc55112_ADC1_RAND_WID                  1
#define sc55112_ADC1_AD_SEL1_WID               1
#define sc55112_ADC1_ADA1_WID                  3
#define sc55112_ADC1_ADA2_WID                  3
#define sc55112_ADC1_ATO_WID                   4
#define sc55112_ADC1_ATOX_WID                  1
#define sc55112_ADC1_TS_M_WID                  3
#define sc55112_ADC1_LIADC_WID                 1
#define sc55112_ADC1_TS_REFENB_WID             1
#define sc55112_ADC1_VTERM_EN_WID              1
#define sc55112_ADC1_BATT_I_ADC_WID            1

/* ADC2 */
#define sc55112_ADC2_ADD1_WID                  10
#define sc55112_ADC2_ADD2_WID                  10
#define sc55112_ADC2_ADINC1_WID                1
#define sc55112_ADC2_ADINC2_WID                1
#define sc55112_ADC2_ASC_WID                   1
#define sc55112_ADC2_ADTRIG_ONESHOT_WID        1

/* RTC_CAL */
#define sc55112_RTC_CAL_ENCLK16M_WID           1
#define sc55112_RTC_CAL_CAL_PRD_WID            4
#define sc55112_RTC_CAL_SMPL_WIN_WID           3
#define sc55112_RTC_CAL_CAL_MODE_WID           2
#define sc55112_RTC_CAL_SWCLK_FREQ_WID         2
#define sc55112_RTC_CAL_WHIGH_WID              6
#define sc55112_RTC_CAL_WLOW_WID               6

/*
 * Register bit field write values
 */
/* ADC1 */
#define sc55112_ADC1_ADEN_ENABLE               1
#define sc55112_ADC1_ADEN_DISABLE              0
#define sc55112_ADC1_RAND_CHANNEL              1
#define sc55112_ADC1_RAND_GROUP                0
#define sc55112_ADC1_ADA_LICELL                0
#define sc55112_ADC1_ADA_BATSENSE              1
#define sc55112_ADC1_ADA_RAWEXTBPLUSSENSE      2
#define sc55112_ADC1_ADA_MPBSENSE              3
#define sc55112_ADC1_ADA_AD4                   4
#define sc55112_ADC1_ADA_AD5_CHG_ISENSE        5
#define sc55112_ADC1_ADA_AD6_BATT_I_ADC        6
#define sc55112_ADC1_ADA_USB_ID                7
#define sc55112_ADC1_ADA_AD7                   0
#define sc55112_ADC1_ADA_AD8                   1
#define sc55112_ADC1_ADA_AD9                   2
#define sc55112_ADC1_ADA_TSX1                  3
#define sc55112_ADC1_ADA_TSX2                  4
#define sc55112_ADC1_ADA_TSY1                  5
#define sc55112_ADC1_ADA_TSY2                  6
#define sc55112_ADC1_ADA_GND                   7
#define sc55112_ADC1_ATOX_DELAY_FIRST          0
#define sc55112_ADC1_ATOX_DELAY_EACH           1
#define sc55112_ADC1_TS_M_X_POS                0
#define sc55112_ADC1_TS_M_Y_POS                1
#define sc55112_ADC1_TS_M_PRESSURE             2
#define sc55112_ADC1_TS_M_PLATE_X              3
#define sc55112_ADC1_TS_M_PLATE_Y              4
#define sc55112_ADC1_TS_M_STBY                 5
#define sc55112_ADC1_TS_M_NON_TS               6
#define sc55112_ADC1_LIADC_ENABLE              1
#define sc55112_ADC1_LIADC_DISABLE             0
#define sc55112_ADC1_TS_REFENB_ENABLE          0
#define sc55112_ADC1_TS_REFENB_DISABLE         1
#define sc55112_ADC1_VTERM_EN_ENABLE           1
#define sc55112_ADC1_VTERM_EN_RESET            0
#define sc55112_ADC1_BATT_I_ADC_ENABLE         1
#define sc55112_ADC1_BATT_I_ADC_DISABLE        0

/* ADC2 */
#define sc55112_ADC2_ADINC1_NO_INCR            0
#define sc55112_ADC2_ADINC1_INCR               1
#define sc55112_ADC2_ADINC2_NO_INCR            0
#define sc55112_ADC2_ADINC2_INCR               1
#define sc55112_ADC2_ASC_START_ADC             1

/* RTC_CAL */
#define sc55112_RTC_CAL_ENCLK16M_DISABLE       0
#define sc55112_RTC_CAL_ENCLK16M_ENABLE        1

#endif				/* _sc55112_ADC_REGS_H */
