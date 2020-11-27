/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/ethernet.c
 *
 * SCM-A11 implementation of Motorola GPIO API for Ethernet.
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

#if defined(CONFIG_MOT_FEAT_GPIO_API_ETHERNET)
/**
 * Install handler for Ethernet interrupts.
 *
 * @param   handler     Handler for the interrupt.
 * @param   irq_flags   Interrupt request flags.
 * @param   devname     Name associated with device driver requesting interrupt.
 * @param   dev_id      Unique identifier for the device driver.
 *
 * @return  Zero on success; non-zero on failure.
 */
int enet_request_irq(irqreturn_t (*handler)(int, void *, struct pt_regs *),
        unsigned long irq_flags, const char * devname, void *dev_id)
{
    set_irq_type(INT_EXT_INT3, IRQT_RISING);
    return request_irq(INT_EXT_INT3, handler, irq_flags, devname, dev_id);
}


/**
 * Remove handler for Ethernet interrupt.
 *
 * @param   dev_id      Unique identifier for the device driver.
 *
 * @return  Zero.
 */
int enet_free_irq(void *dev_id)
{
    free_irq(INT_EXT_INT3, dev_id);
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
