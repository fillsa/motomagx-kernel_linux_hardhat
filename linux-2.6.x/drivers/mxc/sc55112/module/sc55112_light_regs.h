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

#ifndef _sc55112_GPS_LIGHT_H
#define _sc55112_GPS_LIGHT_H

/*!
 * @file sc55112_light_regs.h
 * @brief This file contains all sc55112 light register definitions.
 *
 * @ingroup PMIC_sc55112_LIGHT
 */

/*
 * Register bit field positions (left shift)
 */
#define sc55112_LED_BKLT1_LEDBLEN_LSH               0
#define sc55112_LED_BKLT1_LEDI_LSH                  1
#define sc55112_LED_BKLT1_LED1DC_LSH                4
#define sc55112_LED_BKLT1_LED2DC_LSH                8
#define sc55112_LED_BKLT1_LED1RAMPUP_LSH            16
#define sc55112_LED_BKLT1_LED2RAMPUP_LSH            17
#define sc55112_LED_BKLT1_LED1RAMPDOWN_LSH          19
#define sc55112_LED_BKLT1_LED2RAMPDOWN_LSH          20
#define sc55112_LED_BKLT1_BKLT1CYCLETIME_LSH        22

#define sc55112_LED_BKLT2_LED1TRIODE_LSH            0
#define sc55112_LED_BKLT2_LED2TRIODE_LSH            1
#define sc55112_LED_BKLT2_LED1STROBESLOW_LSH        3
#define sc55112_LED_BKLT2_LED1STROBEFAST_LSH        4
#define sc55112_LED_BKLT2_LED2STROBESLOW_LSH        5
#define sc55112_LED_BKLT2_LED2STROBEFAST_LSH        6
#define sc55112_LED_BKLT2_AUDMODEN_LSH              9
#define sc55112_LED_BKLT2_AUDLED1_LSH               10
#define sc55112_LED_BKLT2_AUDLED2_LSH               11
#define sc55112_LED_BKLT2_AUDLEDR_LSH               13
#define sc55112_LED_BKLT2_AUDLEDG_LSH               14
#define sc55112_LED_BKLT2_AUDLEDB_LSH               15
#define sc55112_LED_BKLT2_AUDGAIN_LSH               16
#define sc55112_LED_BKLT2_AUDPATHSEL_LSH            18
#define sc55112_LED_BKLT2_AUDLPFBYP_LSH             19
#define sc55112_LED_BKLT2_SLEWLIMEN_LSH             20
#define sc55112_LED_BKLT2_SKIP_LSH                  21
#define sc55112_LED_BKLT2_FLRGB_LSH                 22

#define sc55112_LED_TC1_LEDR_EN_LSH                 0
#define sc55112_LED_TC1_LEDG_EN_LSH                 1
#define sc55112_LED_TC1_LEDB_EN_LSH                 2
#define sc55112_LED_TC1_LEDR_CTRL_LSH               3
#define sc55112_LED_TC1_LEDG_CTRL_LSH               8
#define sc55112_LED_TC1_LEDB_CTRL_LSH               13
#define sc55112_LED_TC1_LEDR_I_LSH                  18
#define sc55112_LED_TC1_LEDG_I_LSH                  20
#define sc55112_LED_TC1_LEDB_I_LSH                  22

#define sc55112_LED_TC2_LED_TC_EN_LSH               0
#define sc55112_LED_TC2_LED_TC2_TRIODE1_LSH         1
#define sc55112_LED_TC2_LED_TC2_TRIODE2_LSH         2
#define sc55112_LED_TC2_LED_TC2_TRIODE3_LSH         3
#define sc55112_LED_TC2_CYCLE_TIME_LSH              4
#define sc55112_LED_TC2_BLENDED_RAMPS_SLOW_LSH      6
#define sc55112_LED_TC2_BLENDED_RAMPS_FAST_LSH      7
#define sc55112_LED_TC2_SAW_RAMPS_SLOW_LSH          8
#define sc55112_LED_TC2_SAW_RAMPS_FAST_LSH          9
#define sc55112_LED_TC2_BLENDED_BOWTIE_SLOW_LSH     10
#define sc55112_LED_TC2_BLENDED_BOWTIE_FAST_LSH     11
#define sc55112_LED_TC2_STROBE_LEDR_SLOW_LSH        12
#define sc55112_LED_TC2_STROBE_LEDR_FAST_LSH        13
#define sc55112_LED_TC2_STROBE_LEDG_SLOW_LSH        14
#define sc55112_LED_TC2_STROBE_LEDG_FAST_LSH        15
#define sc55112_LED_TC2_STROBE_LEDB_SLOW_LSH        16
#define sc55112_LED_TC2_STROBE_LEDB_FAST_LSH        17
#define sc55112_LED_TC2_LEDR_RAMP_UP_LSH            18
#define sc55112_LED_TC2_LEDG_RAMP_UP_LSH            19
#define sc55112_LED_TC2_LEDB_RAMP_UP_LSH            20
#define sc55112_LED_TC2_LEDR_RAMP_DOWN_LSH          21
#define sc55112_LED_TC2_LEDG_RAMP_DOWN_LSH          22
#define sc55112_LED_TC2_LEDB_RAMP_DOWN_LSH          23
/*
 * Register bit field width
 */
#define sc55112_LED_BKLT1_LEDBLEN_WID               1
#define sc55112_LED_BKLT1_LEDI_WID                  3
#define sc55112_LED_BKLT1_LED1DC_WID                4
#define sc55112_LED_BKLT1_LED2DC_WID                4
#define sc55112_LED_BKLT1_LED1RAMPUP_WID            1
#define sc55112_LED_BKLT1_LED2RAMPUP_WID            1
#define sc55112_LED_BKLT1_LED1RAMPDOWN_WID          1
#define sc55112_LED_BKLT1_LED2RAMPDOWN_WID          1
#define sc55112_LED_BKLT1_BKLT1CYCLETIME_WID        2

#define sc55112_LED_BKLT2_LED1TRIODE_WID            1
#define sc55112_LED_BKLT2_LED2TRIODE_WID            1
#define sc55112_LED_BKLT2_LED1STROBESLOW_WID        1
#define sc55112_LED_BKLT2_LED1STROBEFAST_WID        1
#define sc55112_LED_BKLT2_LED2STROBESLOW_WID        1
#define sc55112_LED_BKLT2_LED2STROBEFAST_WID        1
#define sc55112_LED_BKLT2_AUDMODEN_WID              1
#define sc55112_LED_BKLT2_AUDLED1_WID               1
#define sc55112_LED_BKLT2_AUDLED2_WID               1
#define sc55112_LED_BKLT2_AUDLEDR_WID               1
#define sc55112_LED_BKLT2_AUDLEDG_WID               1
#define sc55112_LED_BKLT2_AUDLEDB_WID               1
#define sc55112_LED_BKLT2_AUDGAIN_WID               2
#define sc55112_LED_BKLT2_AUDPATHSEL_WID            1
#define sc55112_LED_BKLT2_AUDLPFBYP_WID             1
#define sc55112_LED_BKLT2_SLEWLIMEN_WID             1
#define sc55112_LED_BKLT2_SKIP_WID                  1
#define sc55112_LED_BKLT2_FLRGB_WID                 1

#define sc55112_LED_TC1_LEDR_EN_WID                 1
#define sc55112_LED_TC1_LEDG_EN_WID                 1
#define sc55112_LED_TC1_LEDB_EN_WID                 1
#define sc55112_LED_TC1_LEDR_CTRL_WID               5
#define sc55112_LED_TC1_LEDG_CTRL_WID               5
#define sc55112_LED_TC1_LEDB_CTRL_WID               5
#define sc55112_LED_TC1_LEDR_I_WID                  2
#define sc55112_LED_TC1_LEDG_I_WID                  2
#define sc55112_LED_TC1_LEDB_I_WID                  2

#define sc55112_LED_TC2_LED_TC_EN_WID               1
#define sc55112_LED_TC2_LED_TC2_TRIODE1_WID         1
#define sc55112_LED_TC2_LED_TC2_TRIODE2_WID         1
#define sc55112_LED_TC2_LED_TC2_TRIODE3_WID         1
#define sc55112_LED_TC2_CYCLE_TIME_WID              2
#define sc55112_LED_TC2_BLENDED_RAMPS_SLOW_WID      1
#define sc55112_LED_TC2_BLENDED_RAMPS_FAST_WID      1
#define sc55112_LED_TC2_SAW_RAMPS_SLOW_WID          1
#define sc55112_LED_TC2_SAW_RAMPS_FAST_WID          1
#define sc55112_LED_TC2_BLENDED_BOWTIE_SLOW_WID     1
#define sc55112_LED_TC2_BLENDED_BOWTIE_FAST_WID     1
#define sc55112_LED_TC2_STROBE_LEDR_SLOW_WID        1
#define sc55112_LED_TC2_STROBE_LEDR_FAST_WID        1
#define sc55112_LED_TC2_STROBE_LEDG_SLOW_WID        1
#define sc55112_LED_TC2_STROBE_LEDG_FAST_WID        1
#define sc55112_LED_TC2_STROBE_LEDB_SLOW_WID        1
#define sc55112_LED_TC2_STROBE_LEDB_FAST_WID        1
#define sc55112_LED_TC2_LEDR_RAMP_UP_WID            1
#define sc55112_LED_TC2_LEDG_RAMP_UP_WID            1
#define sc55112_LED_TC2_LEDB_RAMP_UP_WID            1
#define sc55112_LED_TC2_LEDR_RAMP_DOWN_WID          1
#define sc55112_LED_TC2_LEDG_RAMP_DOWN_WID          1
#define sc55112_LED_TC2_LEDB_RAMP_DOWN_WID          1

/*
 * Register bit field write values
 */
/* BKLT1 */
#define sc55112_LED_BKLT1_LEDBLEN_DISABLE           0
#define sc55112_LED_BKLT1_LEDBLEN_ENABLE            1
#define sc55112_LED_BKLT1_LEDRAMPUP_DISABLE         0
#define sc55112_LED_BKLT1_LEDRAMPUP_ENABLE          1
#define sc55112_LED_BKLT1_LEDRAMPDOWN_DISABLE       0
#define sc55112_LED_BKLT1_LEDRAMPDOWN_ENABLE        1

/* BKLT2 */
#define sc55112_LED_BKLT2_LEDSTROBESLOW_DISABLE     0
#define sc55112_LED_BKLT2_LEDSTROBESLOW_ENABLE      1
#define sc55112_LED_BKLT2_LEDSTROBEFAST_DISABLE     0
#define sc55112_LED_BKLT2_LEDSTROBEFAST_ENABLE      1
#define sc55112_LED_BKLT2_SLEWLIMEN_DISABLE         0
#define sc55112_LED_BKLT2_SLEWLIMEN_ENABLE          1

/*audio coupling */
#define sc55112_LED_BKLT2_LED1TRIODE_DISABLE        0
#define sc55112_LED_BKLT2_LED1TRIODE_ENABLE         1
#define sc55112_LED_BKLT2_LED2TRIODE_DISABLE        0
#define sc55112_LED_BKLT2_LED2TRIODE_ENABLE         1
#define sc55112_LED_BKLT2_AUDMODEN_DISABLE          0
#define sc55112_LED_BKLT2_AUDMODEN_ENABLE           1
#define sc55112_LED_BKLT2_AUDLED_DISABLE            0
#define sc55112_LED_BKLT2_AUDLED_ENABLE             1
#define sc55112_LED_BKLT2_AUDPATHSEL_RX             0
#define sc55112_LED_BKLT2_AUDPATHSEL_TX             1
#define sc55112_LED_BKLT2_AUDLPFBYP_BYPASS          0
#define sc55112_LED_BKLT2_AUDLPFBYP_LPF             1

/*used by tri-color LED */
#define sc55112_LED_BKLT2_SKIP_NOSKIP               0
#define sc55112_LED_BKLT2_SKIP_SKIP                 1
#define sc55112_LED_BKLT2_FLRGB_RGINDICATOR         0
#define sc55112_LED_BKLT2_FLRGB_FUNLIGHT            1

/* TC1 */
#define sc55112_LED_TC1_LED_DISABLE                 0
#define sc55112_LED_TC1_LED_ENABLE                  1

/* TC2 */
#define sc55112_LED_TC2_LED_TC_DISABLE              0
#define sc55112_LED_TC2_LED_TC_ENABLE               1
#define sc55112_LED_TC2_LED_TC2_TRIODE_DISABLE      0
#define sc55112_LED_TC2_LED_TC2_TRIODE_ENABLE       1
#define sc55112_LED_TC2_BLENDED_RAMPS_SLOW_DISABLE  0
#define sc55112_LED_TC2_BLENDED_RAMPS_SLOW_ENABLE   1
#define sc55112_LED_TC2_BLENDED_RAMPS_FAST_DISABLE  0
#define sc55112_LED_TC2_BLENDED_RAMPS_FAST_ENABLE   1
#define sc55112_LED_TC2_SAW_RAMPS_SLOW_DISABLE      0
#define sc55112_LED_TC2_SAW_RAMPS_SLOW_ENABLE       1
#define sc55112_LED_TC2_SAW_RAMPS_FAST_DISABLE      0
#define sc55112_LED_TC2_SAW_RAMPS_FAST_ENABLE       1
#define sc55112_LED_TC2_BLENDED_BOWTIE_SLOW_DISABLE 0
#define sc55112_LED_TC2_BLENDED_BOWTIE_SLOW_ENABLE  1
#define sc55112_LED_TC2_BLENDED_BOWTIE_FAST_DISABLE 0
#define sc55112_LED_TC2_BLENDED_BOWTIE_FAST_ENABLE  1
#define sc55112_LED_TC2_STROBE_LED_SLOW_DISABLE     0
#define sc55112_LED_TC2_STROBE_LED_SLOW_ENABLE      1
#define sc55112_LED_TC2_STROBE_LED_FAST_DISABLE     0
#define sc55112_LED_TC2_STROBE_LED_FAST_ENABLE      1
#define sc55112_LED_TC2_LED_RAMP_UP_DISABLE         0
#define sc55112_LED_TC2_LED_RAMP_UP_ENABLE          1
#define sc55112_LED_TC2_LED_RAMP_DOWN_DISABLE       0
#define sc55112_LED_TC2_LED_RAMP_DOWN_ENABLE        1

#endif				/* _sc55112_GPS_LIGHT_H */
