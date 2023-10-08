/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/serializer.c
 *
 * SCM-A11 implementation of Motorola GPIO API to control LCD serializer.
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

#if defined(CONFIG_MOT_FEAT_GPIO_API_SERIALIZER)
/*!
 * Setup GPIO for the serializer RESET 
 *
 * @param   asserted    If set to "assert", serializer will be held in reset.
 */
void gpio_lcd_serializer_reset(enum gpio_signal_assertion asserted)
{
#if defined(CONFIG_MACH_SCMA11REF) || defined(CONFIG_MACH_LIDO) || defined(CONFIG_MACH_NEVIS)
    /* GPIO_SIGNAL_SER_RST_B is serializer reset */
    gpio_signal_set_data(GPIO_SIGNAL_SER_RST_B,
            (asserted == ASSERT_GPIO_SIGNAL) ? GPIO_DATA_LOW : GPIO_DATA_HIGH);
#endif
	/* GPIO_SIGNAL_DISP_RST_B is display reset */ // old     /* GPIO_SIGNAL_DISP_RST_B is serializer reset */
    gpio_signal_set_data(GPIO_SIGNAL_DISP_RST_B,
            (asserted == ASSERT_GPIO_SIGNAL) ? GPIO_DATA_LOW : GPIO_DATA_HIGH);
}

/*!
 * Setup GPIO for serializer STBY
 *
 * @param   asserted    If set to "assert", serializer will be ins standby.
 */
void gpio_lcd_serializer_stby(enum gpio_signal_assertion asserted)
{
    /* 
     * Pull the serializer out of STBY. Must occur at least 
     * 20us after calling gpio_lcd_serializer_reset
     */
    gpio_signal_set_data(GPIO_SIGNAL_SER_EN,
            (asserted == ASSERT_GPIO_SIGNAL) ? GPIO_DATA_LOW : GPIO_DATA_HIGH);
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_SERIALIZER */
