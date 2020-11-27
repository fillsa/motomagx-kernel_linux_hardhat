/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/iomux_init.c
 *
 * Configure IOMUX mux control registers.
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
 */

#include "mot-gpio-argonlv.h"

/**
 * Set IOMUX pad registers to the state defined in the SCM-A11 platform
 * pin list.
 */
void __init argonlv_iomux_pad_init(void)
{
    /* No information available on Argon IOMUX pad register settings
     * at this time.
     */
}


/**
 * Set IOMUX mux registers to the state defiend in the SCM-A11 platform
 * pin list.
 */
void __init argonlv_iomux_mux_init(void)
{
    int i, j;

    /* configure IOMUX settings to their prescribed initial states */
    for(i = 0; initial_iomux_settings[i] != NULL; i++) {
        for(j = 0; initial_iomux_settings[i][j].pin != IOMUX_INVALID_PIN; j++) {
            gpio_tracemsg("IOMUX pin: 0x%08x output: 0x%02x input: 0x%02x",
                    initial_iomux_settings[i][j].pin,
                    initial_iomux_settings[i][j].output_config,
                    initial_iomux_settings[i][j].input_config);

            iomux_config_mux(initial_iomux_settings[i][j].pin,
                    initial_iomux_settings[i][j].out,
                    initial_iomux_settings[i][j].in);
        }
    }
}
