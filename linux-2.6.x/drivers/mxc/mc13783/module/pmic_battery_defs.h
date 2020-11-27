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

#ifndef         _PMIC_BATTERY_DEFS_H
#define         _PMIC_BATTERY_DEFS_H

#define         PMIC_BATTERY_STRING    "pmic_battery"

#ifdef __ALL_TRACES__
#define TRACEMSG_BATTERY(fmt,args...)  printk(fmt,##args)
#else				/* __ALL_TRACES__ */
#define TRACEMSG_BATTERY(fmt,args...)
#endif				/* __ALL_TRACES__ */

/* REG_CHARGE */
#define MC13783_BATT_DAC_V_DAC_LSH		0
#define MC13783_BATT_DAC_V_DAC_WID		3
#define MC13783_BATT_DAC_DAC_LSH			3
#define MC13783_BATT_DAC_DAC_WID			4
#define	MC13783_BATT_DAC_TRCKLE_LSH		7
#define	MC13783_BATT_DAC_TRCKLE_WID		3
#define MC13783_BATT_DAC_FETOVRD_EN_LSH		10
#define MC13783_BATT_DAC_FETOVRD_EN_WID		1
#define MC13783_BATT_DAC_FETCTRL_EN_LSH		11
#define MC13783_BATT_DAC_FETCTRL_EN_WID		1
#define MC13783_BATT_DAC_REVERSE_SUPPLY_LSH	13
#define MC13783_BATT_DAC_REVERSE_SUPPLY_WID	1
#define MC13783_BATT_DAC_OVCTRL_LSH		15
#define MC13783_BATT_DAC_OVCTRL_WID		2
#define MC13783_BATT_DAC_UNREGULATED_LSH		17
#define MC13783_BATT_DAC_UNREGULATED_WID		1
#define MC13783_BATT_DAC_LED_EN_LSH		18
#define MC13783_BATT_DAC_LED_EN_WID		1
#define MC13783_BATT_DAC_5K_LSH			19
#define MC13783_BATT_DAC_5K_WID			1

#define         BITS_OUT_VOLTAGE        0
#define         LONG_OUT_VOLTAGE        3
#define         BITS_CURRENT_MAIN       3
#define         LONG_CURRENT_MAIN       4
#define         BITS_CURRENT_TRICKLE    7
#define         LONG_CURRENT_TRICKLE    3
#define         BIT_FETOVRD             10
#define         BIT_FETCTRL             11
#define         BIT_RVRSMODE            13
#define         BITS_OVERVOLTAGE        15
#define         LONG_OVERVOLTAGE        2
#define         BIT_UNREGULATED         17
#define         BIT_CHRG_LED            18
#define         BIT_CHRGRAWPDEN         19

/* REG_POWXER_CONTROL_0 */
#define MC13783_BATT_DAC_V_COIN_LSH		20
#define MC13783_BATT_DAC_V_COIN_WID		3
#define MC13783_BATT_DAC_COIN_CH_EN_LSH		23
#define MC13783_BATT_DAC_COIN_CH_EN_WID		1
#define MC13783_BATT_DAC_COIN_CH_EN_ENABLED	1
#define MC13783_BATT_DAC_COIN_CH_EN_DISABLED	0
#define MC13783_BATT_DAC_EOL_CMP_EN_LSH		18
#define MC13783_BATT_DAC_EOL_CMP_EN_WID		1
#define MC13783_BATT_DAC_EOL_CMP_EN_ENABLE	1
#define MC13783_BATT_DAC_EOL_CMP_EN_DISABLE	0
#define MC13783_BATT_DAC_EOL_SEL_LSH		16
#define MC13783_BATT_DAC_EOL_SEL_WID		2

#define         DEF_VALUE               0

#define         BAT_THRESHOLD_MAX       3

#endif				/*  _PMIC_BATTERY_DEFS_H */
