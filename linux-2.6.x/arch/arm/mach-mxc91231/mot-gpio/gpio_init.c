/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/gpio_init.c
 *
 * Initial GPIO register settings for SCM-A11 platform. Settings based
 * on SCM-A11 platform pin list dated 22-Sep-2006.
 *
 * Copyright (C) 2006-2008 Motorola, Inc.
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
 * 19-Oct-2006  Motorola        Initial revision.
 * 02-Jan-2007  Motorola        Added support for Lido P2.
 * 31-Jan-2007  Motorola        Bluetooth current drain improvements.
 * 28-Jun-2007  Motorola        Pin remapping for xPIXL.
 * 13-Aug-2008  Motorola	GP_AP_C8 toggle workaround for 300uA BT power issue.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#include "mot-gpio-scma11.h"

/**
 * This function is called by mxc_board_init() at boot time.
 */
void __init scma11phone_gpio_init(void)
{
    int i;

    /* set iomux pad registers to the prescribed state */
    scma11_iomux_pad_init();

    /* make board-specific fixups */
#if defined(CONFIG_MACH_SCMA11REF)
    scma11ref_gpio_signal_fixup();
#elif defined(CONFIG_MACH_ELBA)
    elba_gpio_signal_fixup();
#elif defined(CONFIG_MACH_LIDO)
    lido_gpio_signal_fixup();
#elif defined(CONFIG_MACH_XPIXL)
    pixl_gpio_signal_fixup();
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

            /* 
             * We set the data for a signal and then configure the direction
             * of the signal.  Section 52.3.1.1 (GPIO Data Register) of
             * the SCM-A11 DTS indicates that this is a reasonable thing to do.
             */
            if(initial_gpio_settings[i].data != GPIO_DATA_INVALID) {
                gpio_set_data(initial_gpio_settings[i].port,
                        initial_gpio_settings[i].sig_no,
                        initial_gpio_settings[i].data);
            }

            gpio_config(initial_gpio_settings[i].port,
                    initial_gpio_settings[i].sig_no,
                    initial_gpio_settings[i].out,
                    GPIO_INT_NONE); /* setup interrupts later */
        }
    }
    
    /* configure IOMUX settings to their prescribed initial state */
    scma11_iomux_mux_init();

    /* make board-specific fixups */
#if defined(CONFIG_MACH_SCMA11REF)
    scma11ref_iomux_mux_fixup();
#elif defined(CONFIG_MACH_LIDO)
    lido_iomux_mux_fixup();
#elif defined(CONFIG_MACH_XPIXL)
    pixl_iomux_mux_fixup();
#endif

#if defined(CONFIG_MOT_FEAT_GPIO_API_BTPOWER)
    /* disable UART1 for Bluetooth current drain improvement */
    gpio_bluetooth_power_set_data(0);

#if defined(CONFIG_MACH_NEVIS)
    /* The following toggle is used to workaround BT bootup 300uA issue */
    gpio_bluetooth_power_fixup();
#endif /* CONFIG_MACH_NEVIS */

#endif

    return;
}
