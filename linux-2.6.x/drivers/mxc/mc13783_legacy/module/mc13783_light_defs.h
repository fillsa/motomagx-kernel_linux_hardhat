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

#ifndef         _MC13783_LIGHT_DEFS_H
#define         _MC13783_LIGHT_DEFS_H

#define LREG_0   REG_LED_CONTROL_0
#define LREG_1   REG_LED_CONTROL_1
#define LREG_2   REG_LED_CONTROL_2
#define LREG_3   REG_LED_CONTROL_3
#define LREG_4   REG_LED_CONTROL_4
#define LREG_5   REG_LED_CONTROL_5

/* REG_LED_CONTROL_0 */

#define         BIT_LEDEN               0
#define         MASK_UP_MAIN_BL         0x002
#define         MASK_DOWN_MAIN_BL       0x010
#define         MASK_TRIODE_MAIN_BL     0x080
#define         INDEX_AUXILIARY         1
#define         INDEX_KEYPAD            2
#define         BITS_FUN_LIGHT          17
#define         MASK_FUN_LIGHT          0x1E0000
#define         MASK_BK1_FL             0x200000
#define         MASK_BK2_FL             0x400000
#define         MASK_BK3_FL             0x800000

#define         BIT_BOOSTEN             10
#define         BITS_BOOST              11

#define         MAX_BOOST_ABMS          3
#define         MAX_BOOST_ABR           2

/* REG_LED_CONTROL_1 */

#define         BIT_SLEWLIMTC           23
#define         BIT_TC1HALF             18

/* REG_LED_CONTROL_2 */

#define         BIT_SLEWLIMBL           23
#define         BIT_CURRENT_LEVEL       0
#define         MASK_CURRENT_LEVEL      0x000007
#define         BIT_DUTY_CYCLE          9
#define         MASK_DUTY_CYCLE         0x001E00
#define         INDEX_AUX               3
#define         INDEX_KYD               6
#define         BITS_PERIOD_BL          21

/* REG_LED_CONTROL_3 4 5 */
#define         BITS_CL_RED             0
#define         LONG_CURRENT_LEVEL      2
#define         BITS_DC_RED             6
#define         LONG_DUTY_CYCLE         5
#define         INDEX_GREEN_CL          2
#define         INDEX_BLUE_CL           4
#define         BIT_PERIOD              21

#define         DUTY_CYCLE_MAX          31

#endif				/*  _MC13783_LIGHT_DEFS_H */
