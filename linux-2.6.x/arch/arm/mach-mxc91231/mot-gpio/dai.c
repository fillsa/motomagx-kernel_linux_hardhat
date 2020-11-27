/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/dai.c
 *
 * SCM-A11 implementation of Motorola GPIO API for digital audio support.
 *
 * Copyright 2006 Motorola, Inc. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API_DAI)
/**
 * Multiplexing Use Case 3 -- DAI on UART2 pins
 */
void gpio_dai_enable(void)
{
    /* this is a NO OP for everything except SCM-A11 Reference Design */
#if defined(CONFIG_MACH_SCMA11REF)
    /*
     *  Pin U2_DSR_B and pin DAI_FS are configured for DAI at boot.
     *
     *  Pin U2_RI_B is usually in GPIO mode for the GPIO_DISP_SD signal.
     *
     *  On P0C, pin U2_DTR_B is usually in GPIO mode for the
     *  GPIO_DISP_CM signal.
     */

    /*
     * SCM-A11 Reference Design Signal Name: DAI_CLK
     * SCM-A11 Package Pin Name: U2_RI_B
     * Selected Function Name: AD4_TXC
     * Selected Direction: Input/Output
     *
     * Primary function out of reset: U2_RI_B
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C27
     */
    iomux_config_mux(AP_U2_RI_B, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
#endif /* CONFIG_MACH_SCMA11REF */
}


/**
 * Return DAI/UART2 to default state.
 */
void gpio_dai_disable(void)
{
    /* this is a NO OP for everything except SCM-A11 Reference Design */
#if defined(CONFIG_MACH_SCMA11REF)
    iomux_config_mux(AP_U2_RI_B, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
#endif /* CONFIG_MACH_SCMA11REF */
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_DAI */
