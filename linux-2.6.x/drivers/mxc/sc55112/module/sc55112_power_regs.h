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

#ifndef _sc55112_POWER_REGS_H
#define _sc55112_POWER_REGS_H

/*!
 * @file sc55112_power_regs.h
 * @brief This file contains all sc55112 power control register definitions.
 *
 * @ingroup PMIC_sc55112_POWER
 */

/*
 * Register bit field positions (left shift)
 */
/* PWRCTRL */
#define sc55112_PWRCTRL_PCT_LSH             0
#define sc55112_PWRCTRL_PCEN_LSH            8
#define sc55112_PWRCTRL_PC_COUNT_LSH        9
#define sc55112_PWRCTRL_PC_COUNT_EN_LSH     13
#define sc55112_PWRCTRL_WARM_EN_LSH         14
#define sc55112_PWRCTRL_USER_OFF_SPI_LSH    15
#define sc55112_PWRCTRL_VHOLD_LSH           16
#define sc55112_PWRCTRL_32OUT_EN_LSH        18
#define sc55112_PWRCTRL_MEM_TMR_LSH         19
#define sc55112_PWRCTRL_MEM_ALLON_LSH       23

/* RTC_DAY */
#define sc55112_RTC_DAY_PC_MAX_CNT_LSH      15

/* SWCTRL */
#define sc55112_SWCTRL_SW1_EN_LSH           0
#define sc55112_SWCTRL_SW1_MODE_LSH         1
#define sc55112_SWCTRL_SW1_LSH              2
#define sc55112_SWCTRL_SW1_STBY_LVS_EN_LSH  5
#define sc55112_SWCTRL_SW2_MODE_LSH         6
#define sc55112_SWCTRL_SW1_LVS_LSH          7
#define sc55112_SWCTRL_SW3_EN_LSH           10
#define sc55112_SWCTRL_SW3_LSH              11
#define sc55112_SWCTRL_SW3_STBY_EN_LSH      12
#define sc55112_SWCTRL_SW2_LSH              13
#define sc55112_SWCTRL_SW2_LVS_LSH          16
#define sc55112_SWCTRL_SW1_STBY_LSH         19
#define sc55112_SWCTRL_SW2_EN_LSH           22
#define sc55112_SWCTRL_SW2_DFVS_EN_LSH      23

/* VREG */
#define sc55112_VREG_VMMC_LP_LSH            0
#define sc55112_VREG_V1_EN_LSH              1
#define sc55112_VREG_V1_LP_LSH              2
#define sc55112_VREG_V1_LSH                 3
#define sc55112_VREG_V1_STBY_LSH            6
#define sc55112_VREG_V2_EN_LSH              7
#define sc55112_VREG_V2_LP_LSH              8
#define sc55112_VREG_V2_STBY_LSH            9
#define sc55112_VREG_V3_EN_LSH              10
#define sc55112_VREG_V3_LP_LSH              11
#define sc55112_VREG_V3_STBY_LSH            12
#define sc55112_VREG_V3_LSH                 13
#define sc55112_VREG_VSIM_EN_LSH            14
#define sc55112_VREG_VSIM_LSH               15
#define sc55112_VREG_V_VIB_EN_LSH           16
#define sc55112_VREG_V_VIB_LSH              17
#define sc55112_VREG_VMMC_LSH               18
#define sc55112_VREG_V4_EN_LSH              22
#define sc55112_VREG_V4_STBY_LSH            23

/*
 * Register bit field width
 */
/* PWRCTRL */
#define sc55112_PWRCTRL_PCT_WID             8
#define sc55112_PWRCTRL_PCEN_WID            1
#define sc55112_PWRCTRL_PC_COUNT_WID        4
#define sc55112_PWRCTRL_PC_COUNT_EN_WID     1
#define sc55112_PWRCTRL_WARM_EN_WID         1
#define sc55112_PWRCTRL_USER_OFF_SPI_WID    1
#define sc55112_PWRCTRL_VHOLD_WID           2
#define sc55112_PWRCTRL_32OUT_EN_WID        1
#define sc55112_PWRCTRL_MEM_TMR_WID         4
#define sc55112_PWRCTRL_MEM_ALLON_WID       1

/* RTC_DAY */
#define sc55112_RTC_DAY_PC_MAX_CNT_WID      4

/* SWCTRL */
#define sc55112_SWCTRL_SW1_EN_WID           1
#define sc55112_SWCTRL_SW1_MODE_WID         1
#define sc55112_SWCTRL_SW1_WID              3
#define sc55112_SWCTRL_SW1_STBY_LVS_EN_WID  1
#define sc55112_SWCTRL_SW2_MODE_WID         1
#define sc55112_SWCTRL_SW1_LVS_WID          3
#define sc55112_SWCTRL_SW3_EN_WID           1
#define sc55112_SWCTRL_SW3_WID              1
#define sc55112_SWCTRL_SW3_STBY_EN_WID      1
#define sc55112_SWCTRL_SW2_WID              3
#define sc55112_SWCTRL_SW2_LVS_WID          3
#define sc55112_SWCTRL_SW1_STBY_WID         3
#define sc55112_SWCTRL_SW2_EN_WID           1
#define sc55112_SWCTRL_SW2_DFVS_EN_WID      1

//Linear votlage regulator
#define sc55112_VREG_VMMC_LP_WID            1
#define sc55112_VREG_V1_EN_WID              1
#define sc55112_VREG_V1_LP_WID              1
#define sc55112_VREG_V1_WID                 3
#define sc55112_VREG_V1_STBY_WID            1
#define sc55112_VREG_V2_EN_WID              1
#define sc55112_VREG_V2_LP_WID              1
#define sc55112_VREG_V2_STBY_WID            1
#define sc55112_VREG_V3_EN_WID              1
#define sc55112_VREG_V3_LP_WID              1
#define sc55112_VREG_V3_STBY_WID            1
#define sc55112_VREG_V3_WID                 1
#define sc55112_VREG_VSIM_EN_WID            1
#define sc55112_VREG_VSIM_WID               1
#define sc55112_VREG_V_VIB_EN_WID           1
#define sc55112_VREG_V_VIB_WID              1
#define sc55112_VREG_VMMC_WID               4
#define sc55112_VREG_V4_EN_WID              1
#define sc55112_VREG_V4_STBY_WID            1

/*
 * Register bit field write values
 */
/* PWRCTRL */
#define sc55112_PWRCTRL_PCT_MAX                     255
#define sc55112_PWRCTRL_PCEN_DISABLE                0
#define sc55112_PWRCTRL_PCEN_ENABLE                 1
#define sc55112_PWRCTRL_PC_COUNT_MAX                15
#define sc55112_PWRCTRL_PC_COUNT_MIN                0
#define sc55112_PWRCTRL_PC_COUNT_EN_DISABLE         0
#define sc55112_PWRCTRL_PC_COUNT_EN_ENABLE          1
#define sc55112_PWRCTRL_WARM_EN_DISABLE             0
#define sc55112_PWRCTRL_WARM_EN_ENABLE              1
#define sc55112_PWRCTRL_USER_OFF_SPI_ENABLE         1
#define sc55112_PWRCTRL_VHOLD_MAX                   3
#define sc55112_PWRCTRL_32OUT_EN_DISABLE            0
#define sc55112_PWRCTRL_32OUT_EN_ENABLE             1
#define sc55112_PWRCTRL_MEM_TMR_MAX                 15
#define sc55112_PWRCTRL_MEM_ALLON_DISABLE           0
#define sc55112_PWRCTRL_MEM_ALLON_ENABLE            1

/* RTC_DAY */
#define sc55112_RTC_DAY_PC_MAX_CNT_MAX              15

// Switch mode regulator
//SW1
#define sc55112_SWCTRL_SW1_EN_DISABLE               0
#define sc55112_SWCTRL_SW1_EN_ENABLE                1
#define sc55112_SWCTRL_SW1_MODE_SYNC_RECT_EN        0
#define sc55112_SWCTRL_SW1_MODE_PULSE_SKIP_EN       1
#define sc55112_SWCTRL_SW1_STBY_LVS_EN_DISABLE      0
#define sc55112_SWCTRL_SW1_STBY_LVS_EN_ENABLE       1

// SW1, SW1_LVS and SW1_STBY are using same encoding as below
#define sc55112_SWCTRL_SW1_1V               0
#define sc55112_SWCTRL_SW1_1_10V            1
#define sc55112_SWCTRL_SW1_1_20V            2
#define sc55112_SWCTRL_SW1_1_30V            3
#define sc55112_SWCTRL_SW1_1_40V            4
#define sc55112_SWCTRL_SW1_1_55V            5
#define sc55112_SWCTRL_SW1_1_625V           6
#define sc55112_SWCTRL_SW1_1_875V           7

//SW2
#define sc55112_SWCTRL_SW2_EN_DISABLE       0
#define sc55112_SWCTRL_SW2_EN_ENABLE        1

#define sc55112_SWCTRL_SW2_MODE_SYNC_RECT_EN        0
#define sc55112_SWCTRL_SW2_MODE_PULSE_SKIP_EN       1

#define sc55112_SWCTRL_SW2_DFVS_EN_DISABLE          0
#define sc55112_SWCTRL_SW2_DFVS_EN_ENABLE           1

// SW2, SW2_LVS are using same encoding as below
#define sc55112_SWCTRL_SW2_1V               0
#define sc55112_SWCTRL_SW2_1_10V            1
#define sc55112_SWCTRL_SW2_1_20V            2
#define sc55112_SWCTRL_SW2_1_30V            3
#define sc55112_SWCTRL_SW2_1_40V            4
#define sc55112_SWCTRL_SW2_1_55V            5
#define sc55112_SWCTRL_SW2_1_625V           6
#define sc55112_SWCTRL_SW2_1_875V           7

//SW3
#define sc55112_SWCTRL_SW3_EN_DISABLE       0
#define sc55112_SWCTRL_SW3_EN_ENABLE        1
#define sc55112_SWCTRL_SW3_STBY_EN_DISABLE  0
#define sc55112_SWCTRL_SW3_STBY_EN_ENABLE   1

// SW3 is using encoding as below
#define sc55112_SWCTRL_SW3_5_10V            0
#define sc55112_SWCTRL_SW3_5_60V            1

//Linear votlage regulator
//V1
#define sc55112_VREG_V1_EN_DISABLE          0
#define sc55112_VREG_V1_EN_ENABLE           1
#define sc55112_VREG_V1_LP_CTL              0
#define sc55112_VREG_V1_LP_ENABLE           1
#define sc55112_VREG_V1_STBY_NO_INTERACTION 0
#define sc55112_VREG_V1_STBY_LOW_POWER      1
#define sc55112_VREG_V1_2_775V              0
#define sc55112_VREG_V1_1_20V               1
#define sc55112_VREG_V1_1_30V               2
#define sc55112_VREG_V1_1_40V               3
#define sc55112_VREG_V1_1_55V               4
#define sc55112_VREG_V1_1_75V               5
#define sc55112_VREG_V1_1_875V              6
#define sc55112_VREG_V1_2_475V              7

//V2
#define sc55112_VREG_V2_EN_DISABLE          0
#define sc55112_VREG_V2_EN_ENABLE           1
#define sc55112_VREG_V2_LP_CTL              0
#define sc55112_VREG_V2_LP_ENABLE           1
#define sc55112_VREG_V2_STBY_NO_INTERACTION 0
#define sc55112_VREG_V2_STBY_LOW_POWER      1

//V3
#define sc55112_VREG_V3_EN_DISABLE          0
#define sc55112_VREG_V3_EN_ENABLE           1
#define sc55112_VREG_V3_LP_CTL              0
#define sc55112_VREG_V3_LP_ENABLE           1
#define sc55112_VREG_V3_STBY_NO_INTERACTION 0
#define sc55112_VREG_V3_STBY_LOW_POWER      1
#define sc55112_VREG_V3_2_775V              0
#define sc55112_VREG_V3_1_875V              1

//V_SIM
#define sc55112_VREG_VSIM_EN_DISABLE        0
#define sc55112_VREG_VSIM_EN_ENABLE         1
#define sc55112_VREG_VSIM_1_8V              0
#define sc55112_VREG_VSIM_3V                1

//V_VIB
#define sc55112_VREG_V_VIB_EN_DISABLE       0
#define sc55112_VREG_V_VIB_EN_ENABLE        1
#define sc55112_VREG_V_VIB_2V               0
#define sc55112_VREG_V_VIB_3V               1

//V_MMC
#define sc55112_VREG_VMMC_LP_DISABLE        0
#define sc55112_VREG_VMMC_LP_ENABLE         1
#define sc55112_VREG_VMMC_OFF               0
#define sc55112_VREG_VMMC_1_6V              1
#define sc55112_VREG_VMMC_1_8V              2
#define sc55112_VREG_VMMC_2V                3
#define sc55112_VREG_VMMC_2_2V              4
#define sc55112_VREG_VMMC_2_4V              5
#define sc55112_VREG_VMMC_2_6V              6
#define sc55112_VREG_VMMC_2_8V              7
#define sc55112_VREG_VMMC_3V                8
#define sc55112_VREG_VMMC_3_2V              9
#define sc55112_VREG_VMMC_3_3V              10
#define sc55112_VREG_VMMC_3_4V              11

//V4
#define sc55112_VREG_V4_EN_DISABLE          0
#define sc55112_VREG_V4_EN_ENABLE           1
#define sc55112_VREG_V4_STBY_NO_INTERACTION 0
#define sc55112_VREG_V4_STBY_LOW_POWER      1

#endif				/* _sc55112_POWER_REGS_H */
