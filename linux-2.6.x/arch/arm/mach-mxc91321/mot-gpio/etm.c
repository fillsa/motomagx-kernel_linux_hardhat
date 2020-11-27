/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/etm.c
 *
 * ArgonLV implementation of ETM signal mux control.
 *
 * Copyright 2006 Motorola, Inc.
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

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 01-Nov-2006  Motorola        Initial revision.
 * 07-Dec-2006  Motorola        Added etm_enable_trigger_clock.
 */

#include "mot-gpio-argonlv.h"

#ifdef CONFIG_MOT_FEAT_GPIO_API_ETM
/**
 * Setup IOMUX for ETM.
 *
 * @param   alternative Choices of pins onto which to place ETM functions.
 */
void etm_iomux_config(enum etm_iomux alternative)
{
    switch(alternative) {
        case ETM_MUX_IPU: /* ETM on IPU port (WJ1500) */
            /*
             * Pin Name: IPU_VSYNC3
             * Signal Name: IPU_VSYNC3
             * Function: TRCLK (ipp_do_etm_traceclk)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_VSYNC3, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * WJ1500 Pin 8 is connected to IPU_DBACK/IPU_HSYNC/KEY_COL(1) which
             * are all marked as DNP.
             */
            /*
             * Pin Name: IPU_VSYNC0
             * Signal Name: IPU_VSYNC0
             * Function: EXTRIG (ipp_ind_arm11p_ext_etm_extin3)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_VSYNC0, OUTPUTCONFIG_ALT1, 
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_7
             * Signal Name: IPU_FDAT(7)
             * Function: TRACE7 (ipp_do_etm_tracedata[7])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_7, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_6
             * Signal Name: IPU_FDAT(6)
             * Function: TRACE6 (ipp_do_etm_tracedata[6])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_6, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_5
             * Signal Name: IPU_FDAT(5)
             * Function: TRACE5 (ipp_do_etm_tracedata[5])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_5, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_4
             * Signal Name: IPU_FDAT(4)
             * Function: TRACE4 (ipp_do_etm_tracedata[4])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_4, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_3
             * Signal Name: IPU_FDAT(3)
             * Function: TRACE3 (ipp_do_etm_tracedata[3])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_3, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_2
             * Signal Name: IPU_FDAT(2)
             * Function: TRACE2 (ipp_do_etm_tracedata[2])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_2, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_1
             * Signal Name: IPU_FDAT(1)
             * Function: TRACE1 (ipp_do_etm_tracedata[1])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_1, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * WJ1500 Pin 30 is connected to KEY_ROW(1) and GND_BB both of
             * which have resistors marked DNP.
             */
            /*
             * WJ1500 Pin 32 is connected to KEY_ROW(0) and GND_BB both of
             * which have resistors marked DNP.
             */
            /*
             * Pin Name: IPU_FDAT_17
             * Signal Name: IPU_FDAT(17)
             * Function: TRCTL (ipp_do_etm_tracectl)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_17, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_0
             * Signal Name: IPU_FDAT(0)
             * Function: TRACE0 (ipp_do_etm_tracedata[0])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_0, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_8
             * Signal Name: IPU_FDAT(8)
             * Function: TRACE8 (ipp_do_etm_tracedata[8])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_8, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_9
             * Signal Name: IPU_FDAT(9)
             * Function: TRACE9 (ipp_do_etm_tracedata[9])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_9, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_10
             * Signal Name: IPU_FDAT(10)
             * Function: TRACE10 (ipp_do_etm_tracedata[10])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_10, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_11
             * Signal Name: IPU_FDAT(11)
             * Function: TRACE11 (ipp_do_etm_tracedata[11])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_11, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_12
             * Signal Name: IPU_FDAT(12)
             * Function: TRACE12 (ipp_do_etm_tracedata[12])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_12, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_13
             * Signal Name: IPU_FDAT(13)
             * Function: TRACE13 (ipp_do_etm_tracedata[13])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_13, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_14
             * Signal Name: IPU_FDAT(14)
             * Function: TRACE14 (ipp_do_etm_tracedata[14])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_14, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_FDAT_15
             * Signal Name: IPU_FDAT(15)
             * Function: TRACE15 (ipp_do_etm_tracedata[15])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_FDAT_15, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            break; /* ETM on IPU port (WJ1500) */

        case ETM_MUX_CSI_KPP: /* ETM on CSI/KPP port (WJ1502) */
            /*
             * Pin Name: KEY_COL1
             * Signal Name: KEY_COL(1)
             * Function: TRCLK (ipp_do_etm_traceclk)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_COL1, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * WJ1502 Pin 8 is connected to CSI_DBACK/IPU_CSI_VSYNC/KEY_COL(1)
             * all of which are connected via resistors marked DNP.
             */
            /*
             * Pin Name: EVTI_B
             * Signal Name: EVTI_B
             * Function: ETM_EXTRIG (ipp_ind_arm11p_ext_etm_extin3)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_EVTI_B, OUTPUTCONFIG_ALT2, INPUTCONFIG_ALT2);
            /*
             * Pin Name: KEY_COL7
             * Signal Name: KEY_COL(7)
             * Function: TRACE7 (ipp_do_etm_tracedata[7])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_COL7, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * Pin Name: KEY_COL6
             * Signal Name: KEY_COL(6)
             * Function: TRACE6 (ipp_do_etm_tracedata[6])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_COL6, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * Pin Name: KEY_COL5
             * Signal Name: KEY_COL(5)
             * Function: TRACE5 (ipp_do_etm_tracedata[5])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_COL5, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * Pin Name: KEY_COL4
             * Signal Name: KEY_COL(4)
             * Function: TRACE4 (ipp_do_etm_tracedata[4])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_COL4, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * Pin Name: KEY_ROW7
             * Signal Name: KEY_ROW(7)
             * Function: TRACE3 (ipp_do_etm_tracedata[3])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_ROW7, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * Pin Name: KEY_ROW6
             * Signal Name: KEY_ROW(6)
             * Function: TRACE2 (ipp_do_etm_tracedata[2])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_ROW6, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * Pin Name: KEY_ROW5
             * Signal Name: KEY_ROW(5)
             * Function: TRACE1 (ipp_do_etm_tracedata[1])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_ROW5, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * WJ1502 Pin 30 is connected to KEY_ROW(1) and GND_BB both of
             * which have resistors marked DNP.
             */
            /*
             * WJ1502 Pin 32 is connected to KEY_ROW(0) and GND_BB both of
             * which have resistors marked DNP.
             */
            /*
             * Pin Name: KEY_COL3
             * Signal Name: KEY_COL(3)
             * Function: TRCTL (ipp_do_etm_tracectl)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_COL3, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * Pin Name: KEY_ROW4
             * Signal Name: KEY_ROW(4)
             * Function: TRACE0 (ipp_do_etm_tracedata[0])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_KEY_ROW4, OUTPUTCONFIG_ALT3,
                    INPUTCONFIG_ALT2);
            /*
             * Pin Name: IPU_CSI_DATA_0
             * Signal Name: IPU_CSI_DATA(0)
             * Function: TRACE8 (ipp_do_etm_tracedata[8])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_CSI_DATA_0, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_CSI_DATA_1
             * Signal Name: IPU_CSI_DATA(1)
             * Function: TRACE9 (ipp_do_etm_tracedata[9])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_CSI_DATA_1, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_CSI_DATA_2
             * Signal Name: IPU_CSI_DATA(2)
             * Function: TRACE10 (ipp_do_etm_tracedata[10])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_CSI_DATA_2, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_CSI_DATA_3
             * Signal Name: IPU_CSI_DATA(3)
             * Function: TRACE11 (ipp_do_etm_tracedata[11])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_CSI_DATA_3, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_CSI_DATA_4
             * Signal Name: IPU_CSI_DATA(4)
             * Function: TRACE12 (ipp_do_etm_tracedata[12])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_CSI_DATA_4, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_CSI_DATA_5
             * Signal Name: IPU_CSI_DATA(5)
             * Function: TRACE13 (ipp_do_etm_tracedata[13])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_CSI_DATA_5, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_CSI_DATA_6
             * Signal Name: IPU_CSI_DATA(6)
             * Function: TRACE14 (ipp_do_etm_tracedata[14])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_CSI_DATA_6, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            /*
             * Pin Name: IPU_CSI_DATA_7
             * Signal Name: IPU_CSI_DATA(7)
             * Function: TRACE15 (ipp_do_etm_tracedata[15])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_IPU_CSI_DATA_7, OUTPUTCONFIG_ALT1,
                    INPUTCONFIG_ALT1);
            break; /* ETM on CSI/KPP (WJ1502) */

        case ETM_MUX_DEFAULT: /* ETM on ETM port */
        default:
            /*
             * Pin Name: TRACE0
             * Signal Name: TRACE(0)
             * Function: TRACE0 (ipp_do_etm_tracedata[0])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE0, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE1
             * Signal Name: TRACE(1)
             * Function: TRACE1 (ipp_do_etm_tracedata[1])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE1, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE2
             * Signal Name: TRACE(2)
             * Function: TRACE2 (ipp_do_etm_tracedata[2])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE2, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE3
             * Signal Name: TRACE(3)
             * Function: TRACE3 (ipp_do_etm_tracedata[3])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE3, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE4
             * Signal Name: TRACE(4)
             * Function: TRACE4 (ipp_do_etm_tracedata[4])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE4, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE5
             * Signal Name: TRACE(5)
             * Function: TRACE5 (ipp_do_etm_tracedata[5])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE5, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE6
             * Signal Name: TRACE(6)
             * Function: TRACE6 (ipp_do_etm_tracedata[6])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE6, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE7
             * Signal Name: TRACE(7)
             * Function: TRACE7 (ipp_do_etm_tracedata[7])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE7, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE8
             * Signal Name: TRACE(8)
             * Function: TRACE8 (ipp_do_etm_tracedata[8])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE8, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE9
             * Signal Name: TRACE(9)
             * Function: TRACE9 (ipp_do_etm_tracedata[9])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE9, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE10
             * Signal Name: TRACE(10)
             * Function: TRACE10 (ipp_do_etm_tracedata[10])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE10, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE11
             * Signal Name: TRACE(11)
             * Function: TRACE11 (ipp_do_etm_tracedata[11])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE11, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE12
             * Signal Name: TRACE(12)
             * Function: TRACE12 (ipp_do_etm_tracedata[12])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE12, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE13
             * Signal Name: TRACE(13)
             * Function: TRACE13 (ipp_do_etm_tracedata[13])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE13, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE14
             * Signal Name: TRACE(14)
             * Function: TRACE14 (ipp_do_etm_tracedata[14])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE14, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE15
             * Signal Name: TRACE(15)
             * Function: TRACE15 (ipp_do_etm_tracedata[15])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE15, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE16
             * Signal Name: TRACE(16)
             * Function: TRACE16 (ipp_do_etm_tracedata[16])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE16, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE17
             * Signal Name: TRACE(17)
             * Function: TRACE17 (ipp_do_etm_tracedata[17])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE17, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE18
             * Signal Name: TRACE(18)
             * Function: TRACE18 (ipp_do_etm_tracedata[18])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE18, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE19
             * Signal Name: TRACE(19)
             * Function: TRACE19 (ipp_do_etm_tracedata[19])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE19, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE20
             * Signal Name: TRACE(20)
             * Function: TRACE20 (ipp_do_etm_tracedata[20])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE20, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE21
             * Signal Name: TRACE(21)
             * Function: TRACE21 (ipp_do_etm_tracedata[21])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE21, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE22
             * Signal Name: TRACE(22)
             * Function: TRACE22 (ipp_do_etm_tracedata[22])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE22, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE23
             * Signal Name: TRACE(23)
             * Function: TRACE23 (ipp_do_etm_tracedata[23])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE23, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE24
             * Signal Name: TRACE(24)
             * Function: TRACE24 (ipp_do_etm_tracedata[24])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE24, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE25
             * Signal Name: TRACE(25)
             * Function: TRACE25 (ipp_do_etm_tracedata[25])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE25, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE26
             * Signal Name: TRACE(26)
             * Function: TRACE26 (ipp_do_etm_tracedata[26])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE26, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE27
             * Signal Name: TRACE(27)
             * Function: TRACE27 (ipp_do_etm_tracedata[27])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE27, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE28
             * Signal Name: TRACE(28)
             * Function: TRACE28 (ipp_do_etm_tracedata[28])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE28, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE29
             * Signal Name: TRACE(29)
             * Function: TRACE29 (ipp_do_etm_tracedata[29])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE29, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE30
             * Signal Name: TRACE(30)
             * Function: TRACE30 (ipp_do_etm_tracedata[30])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE30, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRACE31
             * Signal Name: TRACE(31)
             * Function: TRACE31 (ipp_do_etm_tracedata[31])
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRACE31, OUTPUTCONFIG_FUNC,
                    INPUTCONFIG_FUNC);
            /*
             * Pin Name: DBGACK
             * Signal Name: DBGACK
             * Function: DBGACK (arm_dbgack)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_DBGACK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
            /*
             * Pin Name: TRCLK
             * Signal Name: TRCLK
             * Function: TRCLK (ipp_do_etm_traceclk)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_TRCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
            /*
             * Pin Name: SRST
             * Signal Name: TRCTL
             * Function: TRCTL (ipp_do_etm_tracectl)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_SRST, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
            /*
             * Pin Name: EXTRIG
             * Signal Name: EXTRIG
             * Function: EXTRIG (ipp_ind_arm11p_ext_etm_extin3)
             * Reset Default: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
             */
            iomux_config_mux(PIN_EXTRIG, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
            break; /* ETM on ETM port */
    }

}


/**
 * Adjust clock settings so that ETM trigger clock behaves properly.
 */
void etm_enable_trigger_clock(void)
{
    /* stub */
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_ETM */
