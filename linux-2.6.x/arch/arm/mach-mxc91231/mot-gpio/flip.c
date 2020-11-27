/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/flip.c
 *
 * SCM-A11 implementation of Motorola GPIO API for Flip Detection.
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

#if defined(CONFIG_MOT_FEAT_FLIP) 
/*
 * Flip Interrupt Functions
 *
 * SCM-A11 Package Pin Name: ED_INT3
 * Lido P1 Signal: FLIP_OPEN (Misc)
 * Selected Primary Function: GP_AP_C21 (Input)
 * Selected Secondary Function: ED_INT3 (Input)
 */

/**
 * Register the flip interrupt request.
 */
int gpio_flip_request_irq(gpio_irq_handler handler, unsigned long irq_flags,
        const char *devname, void *dev_id)
{
    set_irq_type(INT_EXT_INT3, IRQT_BOTHEDGE);
    return request_irq(INT_EXT_INT3, handler, irq_flags, devname, dev_id);
}


/**
 * Free the flip interrupt request.
 */
int gpio_flip_free_irq(void *dev_id)
{
    free_irq(INT_EXT_INT3, dev_id);
    return 0;
}


/**
 * Clear the flip interrupt if it is set.
 */
void gpio_flip_clear_int(void)
{
    /* NOOP */
}


/**
 * Get the current state of the flip.
 *
 * @return  High value (non-zero) indicates flip is open; low value (zero)
 * indicates that flip is closed.
 */
int gpio_flip_open(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_FLIP_OPEN);
}

#endif /* CONFIG_MOT_FEAT_FLIP */
