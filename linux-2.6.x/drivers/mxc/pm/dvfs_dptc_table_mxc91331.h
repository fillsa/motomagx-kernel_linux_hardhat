/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*!
 * @file dptc.h
 *
 * @brief MXC91331 dvfs & dptc table file.
 *
 * @ingroup PM
 */
#ifndef __DVFS_DPTC_TABLE_MXC91331_H__
#define __DVFS_DPTC_TABLE_MXC91331_H__

/*!
 * Default DPTC table definition
 */
#define NUM_OF_FREQS 1
#define NUM_OF_WP    17

static char *default_table_str = "WORKING POINT 17\n\
\n\
WP 0x1c\n\
WP 0x1b\n\
WP 0x1a\n\
WP 0x19\n\
WP 0x18\n\
WP 0x17\n\
WP 0x16\n\
WP 0x15\n\
WP 0x14\n\
WP 0x13\n\
WP 0x12\n\
WP 0x11\n\
WP 0x10\n\
WP 0xf\n\
WP 0xe\n\
WP 0xd\n\
WP 0xc\n\
\n\
DCVR 0xffc00000 0x96a408f0 0xffc00000 0xe7b76db8\n\
DCVR 0xffc00000 0x96e418f0 0xffc00000 0xe7f76db8\n\
DCVR 0xffc00000 0x972418f0 0xffc00000 0xe8377db8\n\
DCVR 0xffc00000 0x976428f0 0xffc00000 0xe8b78dbc\n\
DCVR 0xffc00000 0x97a428f0 0xffc00000 0xe9378dbc\n\
DCVR 0xffc00000 0x97e428f4 0xffc00000 0xe9778dbc\n\
DCVR 0xffc00000 0x982438f4 0xffc00000 0xe9f79dc0\n\
DCVR 0xffc00000 0x986438f4 0xffc00000 0xea77adc0\n\
DCVR 0xffc00000 0x98e448f4 0xffc00000 0xeaf7bdc4\n\
DCVR 0xffc00000 0x992448f8 0xffc00000 0xeb77cdc4\n\
DCVR 0xffc00000 0x996458f8 0xffc00000 0xec37ddc8\n\
DCVR 0xffc00000 0x99e468f8 0xffc00000 0xecf7edc8\n\
DCVR 0xffc00000 0x9a2468f8 0xffc00000 0xed77fdcc\n\
DCVR 0xffc00000 0x9aa478fc 0xffc00000 0xedf80dcc\n\
DCVR 0xffc00000 0x9ae488fc 0xffc00000 0xeef82dd0\n\
DCVR 0xffc00000 0x9b6488fc 0xffc00000 0xefb82dd0\n\
DCVR 0xffc00000 0x9be49900 0xffc00000 0xf0785dd4\n\
";

#endif
