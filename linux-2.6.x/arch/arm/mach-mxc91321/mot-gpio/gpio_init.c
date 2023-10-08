/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/gpio_init.c
 *
 * Initialize IOMUX and GPIO registers at boot time.
 *
 * Copyright 2006-2007 Motorola, Inc.
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
 * 20-Feb-2007  Motorola        Update for HWCFG tree.
 */

#include <asm/mot-gpio.h>
#include "mot-gpio-argonlv.h"

/**
 * This function is called by mxc_board_init() at boot time.
 */
void __init argonlvphone_gpio_init(void)
{

#if defined(CONFIG_MACH_BUTEREF)
    buteref_gpio_signal_fixup();
#elif defined(CONFIG_MOT_FEAT_GPIO_API_GPU)
    gpu_gpio_signal_fixup();
#endif

    /* configure ArgonLV IOMUX pad registers to initial state */
    mot_iomux_pad_init();

#if !defined(CONFIG_MACH_BUTEREF)


#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
    /* read the GPIO signal mappings from the HWCFG tree */
    mot_gpio_update_mappings(); // 20-Feb-2007  Motorola        Update for HWCFG tree.
#endif /* if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)*/

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
    mot_gpio_init();

    /* configure IOMUX settings to their prescribed initial state */
    mot_iomux_mux_init();
}

