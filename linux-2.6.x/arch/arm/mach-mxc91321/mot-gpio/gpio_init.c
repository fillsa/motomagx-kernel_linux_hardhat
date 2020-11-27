/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/gpio_init.c
 *
 * Initialize IOMUX and GPIO registers at boot time.
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
 * 28-Nov-2006  Motorola        Added call to GPU fixup function.
 */

#include "mot-gpio-argonlv.h"

/**
 * This function is called by mxc_board_init() at boot time.
 */
void __init argonlvphone_gpio_init(void)
{
    unsigned i;

#if defined(CONFIG_MACH_BUTEREF)
    buteref_gpio_signal_fixup();
#elif defined(CONFIG_MOT_FEAT_GPIO_API_GPU)
    gpu_gpio_signal_fixup();
#endif

    /* configure ArgonLV IOMUX pad registers to initial state */
    argonlv_iomux_pad_init();

#if !defined(CONFIG_MACH_BUTEREF)
    /*
     * Care must be taken when transferring pin GPIO24 from Func mux set setting
     * to the the GPIO mux setting. GPIO24 is connected to the WDOG_AP signal;
     * if the logical value on the pin falls, the phone will shut down.
     */
    iomux_config_mux(PIN_GPIO29, OUTPUTCONFIG_ALT5, INPUTCONFIG_NONE);
    gpio_signal_config(GPIO_SIGNAL_WDOG_AP, GPIO_GDIR_OUTPUT, GPIO_INT_NONE);
    gpio_signal_set_data(GPIO_SIGNAL_WDOG_AP, GPIO_DATA_HIGH);
    iomux_config_mux(PIN_GPIO24, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
#endif

    /* configure GPIO registers to desired initial state */
    for(i = 0; i < MAX_GPIO_SIGNAL; i++) {
        if(initial_gpio_settings[i].port != GPIO_INVALID_PORT) {
            gpio_tracemsg("GPIO port: 0x%08x signal: 0x%08x output: 0x%08x "
                    "data: 0x%08x",
                    initial_gpio_settings[i].port,
                    initial_gpio_settings[i].sig_no,
                    initial_gpio_settings[i].out,
                    initial_gpio_settings[i].data);

            /* set data */
            if(initial_gpio_settings[i].data != GPIO_DATA_INVALID) {
                gpio_set_data(initial_gpio_settings[i].port,
                        initial_gpio_settings[i].sig_no,
                        initial_gpio_settings[i].data);
            }

            /* set direction */
        gpio_config(initial_gpio_settings[i].port,
                initial_gpio_settings[i].sig_no,
                    initial_gpio_settings[i].out,
                GPIO_INT_NONE); /* setup interrupts later */
        }
    }

    /* configure IOMUX settings to their prescribed initial state */
    argonlv_iomux_mux_init();
}

