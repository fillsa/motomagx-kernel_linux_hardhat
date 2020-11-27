/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/mc13783.c
 *
 * SCM-A11 implementation of Motorola GPIO API for Atlas support.
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

#ifdef CONFIG_MOT_FEAT_GPIO_API_MC13783
/*!
 * This function configures the IOMux block for Atlas standard operations.
 *
 */
void gpio_mc13783_active(void)
{
}


/*!
 * This function clears the Atlas interrupt.
 *
 */
void gpio_mc13783_clear_int(void) 
{
}


/*!
 * This function return the SPI connected to Atlas.
 *
 */
int gpio_mc13783_get_spi(void) 
{
        /* SCMA11 -> SPI2 = 1 */
        return 1;
}


/*!
 * This function return the SPI smave select for Atlas.
 *
 */
int gpio_mc13783_get_ss(void) 
{
        /* SCMA11BB -> SS = 2 */
        return 2;
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_MC13783 */
