/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/flip.c
 *
 * ArgonLV implementation of flip event interrupt detection.
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

#if defined(CONFIG_MOT_FEAT_FLIP)
/**
 * Register a handler for the flip event.
 *
 * @param   handler     Handler for the interrupt.
 * @param   irq_flags   Flags to pass to request_irq.
 * @param   devname     Unique name for the interrupt.
 * @param   dev_id      Parameter to pass to the interrupt handler.
 *
 * @return  Zero on success; non-zero on error.
 */
int  gpio_flip_request_irq(gpio_irq_handler handler, unsigned long irq_flags,
        const char *devname, void *dev_id)
{
    /* set the proper triggering for the interrupt in addition to
     * clearing any pending interrupts */
    gpio_flip_clear_int();

    return gpio_signal_request_irq(GPIO_SIGNAL_FLIP_DETECT,
            GPIO_HIGH_PRIO, handler, irq_flags, devname, dev_id);
}


/**
 * Unregister a previously requested flip interrupt handler.
 *
 * @param   dev_id      Paramter to pass to free_irq.
 *
 * @return  Zero on success; non-zero on error.
 */
int  gpio_flip_free_irq(void *dev_id)
{
    return gpio_signal_free_irq(GPIO_SIGNAL_FLIP_DETECT, GPIO_HIGH_PRIO);
}


/**
 * Clear any pending interrupts. This function will also adjust the edge
 * triggering of the interrupt based on the current state of FLIP_DETECT
 * signal. Since GPIO interrupts don't support both-edge interrupt triggering
 * we better hope we don't miss an interrupt.
 */
void gpio_flip_clear_int(void)
{
    gpio_signal_clear_int(GPIO_SIGNAL_FLIP_DETECT);
    
    gpio_signal_config(GPIO_SIGNAL_FLIP_DETECT, GPIO_GDIR_INPUT,
            gpio_flip_open() ? GPIO_INT_FALL_EDGE : GPIO_INT_RISE_EDGE);
}


/**
 * Determine the status of the flip.
 *
 * @return  True if the flip is flopped.
 */
int  gpio_flip_open(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_FLIP_DETECT);
}
#endif /* CONFIG_MOT_FEAT_FLIP */

