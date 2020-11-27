/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/etm.c
 *
 * SCM-A11 implementation of Motorola GPIO API for ETM.
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
 * 06-Dec-2006  Motorola        Initial revision.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#include "../crm_regs.h"

#ifdef CONFIG_MOT_FEAT_GPIO_API_ETM
/**
 * Configure IOMUX for ETM on alternative pins.
 *
 * @param   alternative Set of pins onto which ETM should be MUXed.
 */
void etm_iomux_config(enum etm_iomux alternative)
{
    printk(KERN_ALERT "%s: setting IOMUX for ETM on %s port\n", __FUNCTION__,
            (alternative == 0) ? "ETM" : ((alternative == 1) ? "IPU" : "KPP"));
    
    switch(alternative) {
        case ETM_MUX_IPU: /* ETM on IPU port (WJ8003) */
            /* 
             * Pin Name: IPU_PAR_RS
             * Signal Name: IPU_PAR_RS
             * Reset Default: FUNC1
             * Function: EXTRIG (Pin list does not list EXTRIG here.)
             */
            iomux_config_mux(AP_IPU_PAR_RS, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /*
             * Pin Name: IPU_D3_CLS
             * Signal Name: IPU_D3_CLS
             * Reset Default: FUNC1
             * Function: TRACECTL
             */
            iomux_config_mux(AP_IPU_D3_CLS, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_RD
             * Signal Name: IPU_RD_SDD
             * Reset Default: FUNC1
             * Function: DBGACK
             */
            iomux_config_mux(AP_IPU_RD, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_WR
             * Signal Name: IPU_WR
             * Reset Default: FUNC1
             * Function: TRCLK
             */
            iomux_config_mux(AP_IPU_WR, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD0
             * Signal Name: IPU_LD(0)
             * Reset Default: FUNC1
             * Function: TRACE0
             */
            iomux_config_mux(AP_IPU_LD0, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD1
             * Signal Name: IPU_LD(1)
             * Reset Default: FUNC1
             * Function: TRACE1
             */
            iomux_config_mux(AP_IPU_LD1, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD2
             * Signal Name: IPU_LD(2)
             * Reset Default: FUNC1
             * Function: TRACE2
             */
            iomux_config_mux(AP_IPU_LD2, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD3
             * Signal Name: IPU_LD(3)
             * Reset Default: FUNC1
             * Function: TRACE3
             */
            iomux_config_mux(AP_IPU_LD3, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD4
             * Signal Name: IPU_LD(4)
             * Reset Default: FUNC1
             * Function: TRACE4
             */
            iomux_config_mux(AP_IPU_LD4, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD5
             * Signal Name: IPU_LD(5)
             * Reset Default: FUNC1
             * Function: TRACE5
             */
            iomux_config_mux(AP_IPU_LD5, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD6
             * Signal Name: IPU_LD(6)
             * Reset Default: FUNC1
             * Function: TRACE6
             */
            iomux_config_mux(AP_IPU_LD6, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD7
             * Signal Name: IPU_LD(7)
             * Reset Default: FUNC1
             * Function: TRACE7
             */
            iomux_config_mux(AP_IPU_LD7, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD8
             * Signal Name: IPU_LD(8)
             * Reset Default: FUNC1
             * Function: TRACE8
             */
            iomux_config_mux(AP_IPU_LD8, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /*
             * Pin Name: IPU_LD9
             * Signal Name: IPU_LD(9)
             * Reset Default: FUNC1
             * Function: TRACE9
             */
            iomux_config_mux(AP_IPU_LD9, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD10
             * Signal Name: IPU_LD(10)
             * Reset Default: FUNC1
             * Function: TRACE10
             */
            iomux_config_mux(AP_IPU_LD10, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD11
             * Signal Name: IPU_LD(11)
             * Reset Default: FUNC1
             * Function: TRACE11
             */
            iomux_config_mux(AP_IPU_LD11, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD12
             * Signal Name: IPU_LD(12)
             * Reset Default: FUNC1
             * Function: TRACE12
             */
            iomux_config_mux(AP_IPU_LD12, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD13
             * Signal Name: IPU_LD(13)
             * Reset Default: FUNC1
             * Function: TRACE13
             */
            iomux_config_mux(AP_IPU_LD13, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD14
             * Signal Name: IPU_LD(14)
             * Reset Default: FUNC1
             * Function: TRACE14
             */
            iomux_config_mux(AP_IPU_LD14, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: IPU_LD15
             * Signal Name: IPU_LD(15)
             * Reset Default: FUNC1
             * Function: TRACE15
             */
            iomux_config_mux(AP_IPU_LD15, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /*
             * P1A schematic shows signal PWM_BKL connected to WJ8003 pin 30
             * via unplaced jumper WP8000.
             */
            /*
             * P1A schematic shows signal GPIO_DISP_RST_B connected to WJ8003
             * pin 32 via unplaced jumper WP8001.
             */
            /*
             * P1A schematic shows signal PROC_RESET_B connected to WJ8003
             * pin 9 via resistor 0 Ohm WR8006 marked DNP.
             */
            break; /* ETM_MUX_IPU -- ETM on IPU port (WJ8003) */

        case ETM_MUX_CSI_KPP: /* ETM on CSI/KPP port (WJ8002) */
            /* 
             * Pin Name: KPROW0
             * Signal Name: KPP_ROW0
             * Reset Default: FUNC1
             * Function: TRCLK
             */
            iomux_config_mux(AP_KPROW0, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /*
             * P1A schematic shows signal KPP_ROW(2) connected to WJ8002
             * pin 8; however, there is no IOMUX setting for pin KPROW2
             * related to ETM.
             *
             * I'm assuming the schematic should show KPP_COL(1).
             */
            /*
             * Pin Name: KPCOL1
             * Signal Name: KPP_COL1
             * Reset Default: FUNC1
             * Function: DBGACK
             */
            iomux_config_mux(AP_KPCOL1, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /* 
             * Pin Name: KPCOL2
             * Signal Name: KPP_COL2
             * Reset Default: FUNC1
             * Function: EXTRIG
             */
            iomux_config_mux(AP_KPCOL2, OUTPUTCONFIG_FUNC3, INPUTCONFIG_NONE);
            /* 
             * Pin Name: GP_AP_B23
             * Signal Name: POWER_RDY
             * Reset Default: DEFAULT
             * Function: TRACE7
             */
            iomux_config_mux(AP_GPIO_AP_B23, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: GP_AP_B22
             * Signal Name: GP_AP_B22
             * Reset Default: DEFAULT
             * Function: TRACE6
             */
            iomux_config_mux(AP_GPIO_AP_B22, OUTPUTCONFIG_FUNC5, 
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: KPCOL5
             * Signal Name: KPP_COL5
             * Reset Default: FUNC1
             * Function: TRACE5
             */
            iomux_config_mux(AP_KPCOL5, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /* 
             * Pin Name: KPCOL4
             * Signal Name: KPP_COL4
             * Reset Default: FUNC1
             * Function: TRACE4
             */
            iomux_config_mux(AP_KPCOL4, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: GP_AP_B18
             * Signal Name: GPIO_DISP_RST_B
             * Reset Default: DEFAULT
             * Function: TRACE3
             */
            iomux_config_mux(AP_GPIO_AP_B18, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: GP_AP_B17
             * Signal Name: PWM_BKL
             * Reset Default: DEFAULT
             * Function: TRACE2
             */
            iomux_config_mux(AP_GPIO_AP_B17, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: KPROW5
             * Signal Name: LOBAT_B
             * Reset Default: FUNC1
             * Function: TRACE1
             */
            iomux_config_mux(AP_KPROW5, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: KPCOL3
             * Signal Name: KPP_COL3
             * Reset Default: FUNC1
             * Function: TRACECTL
             */
            iomux_config_mux(AP_KPCOL3, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: KPROW4
             * Signal Name: KPP_ROW4
             * Reset Default: FUNC1
             * Function: TRACE0
             */
            iomux_config_mux(AP_KPROW4, OUTPUTCONFIG_FUNC5,
                    INPUTCONFIG_NONE);
            /* 
             * Pin Name: CSI_D0
             * Signal Name: GPIO_CAM_RST_B
             * Reset Default: FUNC1
             * Function: TRACE8
             */
            iomux_config_mux(AP_CSI_D0, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /* 
             * Pin Name: CSI_D1
             * Signal Name: GPIO_CAM_PD
             * Reset Default: FUNC1
             * Function: TRACE9
             */
            iomux_config_mux(AP_CSI_D1, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /* 
             * Pin Name: CSI_D2
             * Signal Name: CSI_D(0)
             * Reset Default: FUNC1
             * Function: TRACE10
             */
            iomux_config_mux(AP_CSI_D2, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /* 
             * Pin Name: CSI_D3
             * Signal Name: CSI_D(1)
             * Reset Default: FUNC1
             * Function: TRACE11
             */
            iomux_config_mux(AP_CSI_D3, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /* 
             * Pin Name: CSI_D4
             * Signal Name: CSI_D(2)
             * Reset Default: FUNC1
             * Function: TRACE12
             */
            iomux_config_mux(AP_CSI_D4, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /* 
             * 
             * Pin Name: CSI_D5
             * Signal Name: CSI_D(3)
             * Reset Default: FUNC1
             * Function: TRACE13
             */
            iomux_config_mux(AP_CSI_D5, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /* 
             * Pin Name: CSI_D6
             * Signal Name: CSI_D(4)
             * Reset Default: FUNC1
             * Function: TRACE14
             */
            iomux_config_mux(AP_CSI_D6, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /* 
             * Pin Name: CSI_D7
             * Signal Name: CSI_D(5)
             * Reset Default: FUNC1
             * Function: TRACE15
             */
            iomux_config_mux(AP_CSI_D7, OUTPUTCONFIG_FUNC5, INPUTCONFIG_NONE);
            /*
             * P1A schematic shows signal PROC_RESET_B connected to WJ8000
             * pin 9 via resistor 0 Ohm WR8006 marked DNP.
             */

            break; /* ETM_MUX_CSI_KPP -- ETM on CSI/KPP port (WJ8002) */

        case ETM_MUX_DEFAULT:
        default:
            /* ETM pins on SCM-A11 Development Package are not IOMUXable */
            break;
    }
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_ETM */

