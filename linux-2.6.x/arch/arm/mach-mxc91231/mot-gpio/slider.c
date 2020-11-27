/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/slider.c
 *
 * SCM-A11 implementation of Motorola GPIO API for slider detection.
 *
 * Copyright (C) 2006, 2008 Motorola, Inc. 
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
 *
 * Date          Author        Comments
 * ===========   ==========    ==========================================
 * 01-Jan-2006  Motorola        Initial version.
 * 28-Jun-2007  Motorola        Removed MARCO specific signal code in relation to xPIXL.
 * 20-Mar-2008   Motorola      Remove code not related to singals in Nevis product
 * 03-Jul-2008  Motorola        OSS CV fix.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#if defined(CONFIG_MOT_FEAT_SLIDER)
/*
 * Slider Interrupt Functions
 *
 * SCM-A11 Package Pin Name: ED_INT3
 * Ascension P1 Signal: SLIDER_OPEN (Misc)
 * Ascension P2 Signal: SLIDER_OPEN (Misc)
 * Selected Primary Function: GP_AP_C21 (Input)
 * Selected Secondary Function: ED_INT3 (Input)
 */

/**
 * Register the slider interrupt request.
 */
int gpio_slider_request_irq(gpio_irq_handler handler, unsigned long irq_flags,
        const char *devname, void *dev_id)
{
    set_irq_type(INT_EXT_INT3, IRQT_BOTHEDGE);
    return request_irq(INT_EXT_INT3, handler, irq_flags, devname, dev_id);
}


/**
 * Free the slider interrupt request.
 */
int gpio_slider_free_irq(void *dev_id)
{
    free_irq(INT_EXT_INT3, dev_id);
    return 0;
}


/**
 * Clear the slider interrupt if it is set.
 */
void gpio_slider_clear_int(void)
{
    /* NO OP */
}


/**
 * Get the current state of the slider.
 *
 * High value (non-zero) indicates slider is open; low value (zero) indicates
 * that slider is closed.
 */
int gpio_slider_open(void)
{
#ifndef CONFIG_MACH_NEVIS
    return gpio_signal_get_data_check(GPIO_SIGNAL_SLIDER_OPEN);
#else
    return 0;
#endif
}

#endif /* CONFIG_MOT_FEAT_SLIDER */

