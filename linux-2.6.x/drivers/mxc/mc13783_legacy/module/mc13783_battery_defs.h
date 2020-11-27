/*
 * Copyright 200666666 Freescale Semiconductor, Inc. All Rights Reserved.
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

#ifndef         _MC13783_BATTERY_DEFS_H
#define         _MC13783_BATTERY_DEFS_H

#define         MC13783_BATTERY_STRING    "mc13783_battery"

/* REG_CHARGE */
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

#define         DEF_VALUE               0

#define         BAT_THRESHOLD_MAX       3

#endif				/*  _MC13783_BATTERY_DEFS_H */
