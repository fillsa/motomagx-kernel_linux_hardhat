/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/ethernet.c
 *
 * ArgonLV implementation of Ethernet interrupt control.
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

#include <linux/config.h>
#include <linux/module.h>

#if defined(CONFIG_MOT_FEAT_BRDREV)
#include <asm/boardrev.h>
#endif

#include "mot-gpio-argonlv.h"

#if defined(CONFIG_MOT_FEAT_GPIO_API_ETHERNET)
/**
 * Configure GPIO to receive ethernet interrupts.
 *
 * @param handler The function that will handle the Ethernet interrupt.
 * @param irq_flags Interrupt options. (Usually 0.)
 * @param devname Name of the device requesting the interrupt.
 * @param dev_id Handle to the device driver (or null).
 *
 * @return Zero on success; non-zero on failure.
 */
int enet_request_irq(
        irqreturn_t (*handler)(int, void *, struct pt_regs *),
        unsigned long irq_flags, const char * devname, void *dev_id)
{
#if defined(CONFIG_MOT_FEAT_BRDREV) && defined(CONFIG_MACH_BUTEREF)
    if((boardrev() >= BOARDREV_P4A) && (boardrev() != BOARDREV_UNKNOWN)) {
        return -EBUSY;
    }
#endif /* CONFIG_MOT_FEAT_BRDREV && CONFIG_MACH_BUTEREF */

    /* Pin GPIO8 muxed to MCU GPIO Port Pin 8 at boot. */
    gpio_signal_config(GPIO_SIGNAL_ENET_INT_B, GPIO_GDIR_INPUT,
           GPIO_INT_HIGH_LEV);
    return gpio_signal_request_irq(GPIO_SIGNAL_ENET_INT_B,
            GPIO_HIGH_PRIO, handler, irq_flags, devname, dev_id);
}


/**
 * Configure GPIO to no longer receive ethernet interrupts.
 *
 * @param dev_id Handle to the device driver (or null).
 *
 * @return Zero on success; non-zero on failure.
 */
int enet_free_irq(void *dev_id)
{
#if defined(CONFIG_MOT_FEAT_BRDREV) && defined(CONFIG_MACH_BUTEREF)
    if((boardrev() >= BOARDREV_P4A) && (boardrev() != BOARDREV_UNKNOWN)) {
        return -EBUSY;
    }
#endif /* CONFIG_MOT_FEAT_BRDREV && CONFIG_MACH_BUTEREF */

    /* ENET_INT_B -- GPIO_MCU_8 -- ethernet interrupt */
    gpio_signal_config(GPIO_SIGNAL_ENET_INT_B, GPIO_GDIR_INPUT,
            GPIO_INT_NONE);
    gpio_signal_free_irq(GPIO_SIGNAL_ENET_INT_B,
            GPIO_HIGH_PRIO);
    return 0;
}


/**
 * Clear any pending Ethernet interrupt.
 */
void enet_clear_int(void)
{
    /* NO OP */
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_ETHERNET */
