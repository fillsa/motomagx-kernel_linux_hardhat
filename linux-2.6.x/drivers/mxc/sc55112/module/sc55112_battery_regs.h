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

#ifndef _sc55112_BATTERY_REGS_H
#define _sc55112_BATTERY_REGS_H

/*!
 * @file sc55112_battery_regs.h
 * @brief This file contains all sc55112 battery register definitions.
 *
 * @ingroup PMIC_sc55112_BATTERY
 */

/*
 * Register bit field positions (left shift)
 */
#define sc55112_BATT_DAC_DAC_LSH                    0
#define sc55112_BATT_DAC_EXT_ISENSE_LSH             9
#define sc55112_BATT_DAC_V_COIN_LSH                 10
#define sc55112_BATT_DAC_I_COIN_LSH                 14
#define sc55112_BATT_DAC_COIN_CH_EN_LSH             15
#define sc55112_BATT_DAC_ENB_UVCMP_SYNC_LSH         16
#define sc55112_BATT_DAC_EOL_SEL_LSH                17
#define sc55112_BATT_DAC_EOL_CMP_EN_LSH             20
#define sc55112_BATT_DAC_ENB_CLK_SYNC_LSH           21
#define sc55112_BATT_DAC_PD_SEL_LSH                 22

/*
 * Register bit field width
 */
#define sc55112_BATT_DAC_DAC_WID                    8
#define sc55112_BATT_DAC_EXT_ISENSE_WID             1
#define sc55112_BATT_DAC_V_COIN_WID                 3
#define sc55112_BATT_DAC_I_COIN_WID                 1
#define sc55112_BATT_DAC_COIN_CH_EN_WID             1
#define sc55112_BATT_DAC_ENB_UVCMP_SYNC_WID         1
#define sc55112_BATT_DAC_EOL_SEL_WID                3
#define sc55112_BATT_DAC_EOL_CMP_EN_WID             1
#define sc55112_BATT_DAC_ENB_CLK_SYNC_WID           1
#define sc55112_BATT_DAC_PD_SEL_WID                 2

/*
 * Register bit field write values
 */
#define sc55112_BATT_DAC_EXT_ISENSE_ENABLE          0
#define sc55112_BATT_DAC_EXT_ISENSE_DISABLE         1
#define sc55112_BATT_DAC_I_COIN_LOW                 0
#define sc55112_BATT_DAC_I_COIN_HIGH                1
#define sc55112_BATT_DAC_COIN_CH_EN_DISABLED        0
#define sc55112_BATT_DAC_COIN_CH_EN_ENABLED         1
#define sc55112_BATT_DAC_EOL_CMP_EN_DISABLE         0
#define sc55112_BATT_DAC_EOL_CMP_EN_ENABLE          1

#endif				/* _sc55112_BATTERY_REGS_H */
