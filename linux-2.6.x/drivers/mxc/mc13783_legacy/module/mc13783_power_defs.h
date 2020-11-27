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
 * @file mc13783_power_defs.h
 * @brief This is the header define of mc13783 Power driver.
 *
 * @ingroup MC13783_POWER
 */

/*
 * Includes
 */

#ifndef         _MC13783_POWER_DEFS_H
#define         _MC13783_POWER_DEFS_H

#include "mc13783_power.h"

/*
 * Power Up Mode Sense bits.
 */

#define         STATE_ICTEST_MASK       0x000001

#define         STATE_CLKSEL_BIT        1
#define         STATE_CLKSEL_MASK       0x000002

#define         STATE_PUMS1_BITS        2
#define         STATE_PUMS1_MASK        0x00000C

#define         STATE_PUMS2_BITS        4
#define         STATE_PUMS2_MASK        0x000030

#define         STATE_PUMS3_BITS        6
#define         STATE_PUMS3_MASK        0x0000C0

#define         STATE_CHRGM1_BITS       8
#define         STATE_CHRGM1_MASK       0x000300

#define         STATE_CHRGM2_BITS       10
#define         STATE_CHRGM2_MASK       0x000C00

#define         STATE_UMOD_BITS         12
#define         STATE_UMOD_MASK         0x003000

#define         STATE_USBEN_BIT         14
#define         STATE_USBEN_MASK        0x004000

#define         STATE_SW1A_J_B_BIT      15
#define         STATE_SW1A_J_B_MASK     0x008000

#define         STATE_SW2A_J_B_BIT      16
#define         STATE_SW2A_J_B_MASK     0x010000

/*!
 * This tab define bit for regen of all regulator.
 */
int REGULATOR_REGEN_BIT[REGU_NUMBER] = {
	0,			/* VAUDIO */
	1,			/* VIOHI  */
	2,			/* VIOLO  */
	3,			/* VDIG   */
	4,			/* VGEN   */
	5,			/* VRFDIG */
	6,			/* VRFREF */
	7,			/* VRFCP  */
	-1,			/* VSIM   */
	-1,			/* VESIM  */
	8,			/* VCAM   */
	9,			/* VRFBG  */
	-1,			/* VVIB   */
	10,			/* VRF1   */
	11,			/* VRF2   */
	12,			/* VMMC1  */
	13,			/* VMMC2  */
	16,			/* VGPO1  */
	17,			/* VGPO2  */
	18,			/* VGPO3  */
	19,			/* VGPO4  */
};

#define         BIT_REGEN               20

#define         BIT_VESIM               21
#define         BIT_VMMC1               22
#define         BIT_VMMC2               23

/*!
 * This tab define bit for enable of all regulator.
 */
int REGULATOR_EN_BIT[REGU_NUMBER] = {
	0,			/* VAUDIO */
	3,			/* VIOHI  */
	6,			/* VIOLO  */
	9,			/* VDIG   */
	12,			/* VGEN   */
	15,			/* VRFDIG */
	18,			/* VRFREF */
	21,			/* VRFCP  */
	100,			/* VSIM   */
	103,			/* VESIM  */
	106,			/* VCAM   */
	109,			/* VRFBG  */
	111,			/* VVIB   */
	112,			/* VRF1   */
	115,			/* VRF2   */
	118,			/* VMMC1  */
	121,			/* VMMC2  */
	206,			/* VGPO1  */
	208,			/* VGPO2  */
	210,			/* VGPO3  */
	212,			/* VGPO4  */
};

/*!
 * This tab define bit for controlled by standby of all regulator.
 */
int REGULATOR_SDBY_BIT[REGU_NUMBER] = {
	1,			/* VAUDIO */
	4,			/* VIOHI  */
	7,			/* VIOLO  */
	10,			/* VDIG   */
	13,			/* VGEN   */
	16,			/* VRFDIG */
	19,			/* VRFREF */
	22,			/* VRFCP  */
	101,			/* VSIM   */
	104,			/* VESIM  */
	107,			/* VCAM   */
	110,			/* VRFBG  */
	-1,			/* VVIB   */
	113,			/* VRF1   */
	116,			/* VRF2   */
	119,			/* VMMC1  */
	122,			/* VMMC2  */
	207,			/* VGPO1  */
	209,			/* VGPO2  */
	211,			/* VGPO3  */
	213,			/* VGPO4  */
};

/*!
 * This tab define bit for operating mode of all regulator.
 */
int REGULATOR_OP_MODE_BIT[REGU_NUMBER] = {
	2,			/* VAUDIO */
	5,			/* VIOHI  */
	8,			/* VIOLO  */
	11,			/* VDIG   */
	14,			/* VGEN   */
	17,			/* VRFDIG */
	20,			/* VRFREF */
	23,			/* VRFCP  */
	102,			/* VSIM   */
	105,			/* VESIM  */
	108,			/* VCAM   */
	-1,			/* VRFBG  */
	-1,			/* VVIB   */
	114,			/* VRF1   */
	117,			/* VRF2   */
	120,			/* VMMC1  */
	123,			/* VMMC2  */
	-1,			/* VGPO1  */
	-1,			/* VGPO2  */
	-1,			/* VGPO3  */
	-1,			/* VGPO4  */
};

/*!
 * This tab define bit for settings of all regulator.
 */
int REGULATOR_SETTING_BITS[REGU_NUMBER] = {
	-1,			/* VAUDIO *//* reg / bit / long */
	-1,			/* VIOHI  */
	202,			/* VIOLO  */
	402,			/* VDIG   */
	603,			/* VGEN   */
	902,			/* VRFDIG */
	1102,			/* VRFREF */
	1301,			/* VRFCP  */
	1401,			/* VSIM   */
	1501,			/* VESIM  */
	1603,			/* VCAM   */
	-1,			/* VRFBG  */
	10002,			/* VVIB   */
	10202,			/* VRF1   */
	10402,			/* VRF2   */
	10602,			/* VMMC1  */
	10903,			/* VMMC2  */
	-1,			/* VGPO1  */
	-1,			/* VGPO2  */
	-1,			/* VGPO3  */
	-1,			/* VGPO4  */
};

/* switcher conf bits */

#define         BIT_SET                 0
#define         MASK_SET                0x00003F

#define         BIT_SET_DVS             6
#define         MASK_SET_DVS            0x000FC0

#define         BIT_SET_SDBY            12
#define         MASK_SET_SDBY           0x03F000

#define         BIT_OP_MODE             0
#define         MASK_OP_MODE            0x000003

#define         BIT_OP_SDBY             2
#define         MASK_OP_SDBY            0x00000C

#define         BIT_SPD_DVS             6
#define         MASK_SPD_DVS            0x0000C0

#define         BIT_PANIC               8
#define         MASK_PANIC              0x000100

#define         BIT_SOFTSTART           9
#define         MASK_SOFTSTART          0x000200

#define         BITS_SW                 0
#define         LONG_SW                 17

#define         BITS_CONF_SW            0
#define         LONG_CONF_SW            10

/* Power control bits */

#define         BIT_POWER_CUT           0
#define         BIT_WARM_EN             2
#define         BIT_USER_OFF            3
#define         BIT_32K_EN              6

#define         BIT_VBKUP2AUTOMH        7
#define         BIT_BATTDETEN           19
#define         BIT_VIBPINCTRL          14

#define         BIT_PC_CNT_EN           1
#define         BIT_AUTO_OFF_EN         4
#define         BIT_KEEP_32K_UOFF       5

#define         BITS_TIMER              0
#define         LONG_TIMER              8

#define         BITS_CUT_CUNT           8
#define         LONG_CUT_CUNT           4

#define         BITS_MAX_PW_CUT         12
#define         LONG_MAX_PW_CUT         4

#define         BITS_EXT_PW_CUT         16
#define         LONG_EXT_PW_CUT         4

#define         BIT_TIMER_INF           20

#define         BIT_VBLUP_EN            8
#define         BIT_VBLUP_AUTO_EN       9

#define         BITS_VBLUP_VOLT         10
#define         LONG_VBLUP_VOLT         2

#define         VBLUP2_OFFSET           5

#define         BITS_BP_THRESHOLD       16
#define         LONG_BP_THRESHOLD       2

#define         BIT_EOL_FCT             18

#define         BITS_CHARGE_VOLT        20
#define         LONG_CHARGE_VOLT        3

#define         BIT_CHARGER_EN          23

#define         BIT_AUTO_RESTART        0

#define         BIT_RESET_ON            1

#define         BITS_DEB_TIME_ON        4
#define         LONG_DEB_TIME_ON        2

#define         BITS_PLL_EN             18
#define         BITS_PLL_MULTI          19
#define         LONG_PLL_MULTI          3

#define         BIT_SW3_EN              20
#define         BITS_SW3_SET            18
#define         LONG_SW3_SET            2
#define         BIT_SW3_CTR_DBY         21
#define         BIT_SW3_MODE            22

#endif				/*  _MC13783_POWER_DEFS_H */
