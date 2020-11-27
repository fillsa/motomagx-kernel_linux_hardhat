/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/base.c
 *
 * SCM-A11 implementation of core functions in Motorola GPIO API.
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

#if defined(CONFIG_MOT_FEAT_GPIO_API)
/*!
 * Setup GPIO for a UART port to be active
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_active(int port, int no_irda)
{
    /* UART IOMUX settings configured at boot. */
}


/*!
 * Setup GPIO for a UART port to be inactive
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_inactive(int port, int no_irda)
{
    /* UART IOMUX settings configured at boot. */
}


/**
 * The function is stubbed out and does not apply for SCM-A11
 *
 * @param  port         a UART port
 */
void config_uartdma_event(int port)
{
        return;
}


/*!
 *  Setup GPIO for keypad to be active
 */
void gpio_keypad_active(void)
{
    /* Keypad IOMUX settings configured at boot. */
}


/*!
 * Setup GPIO for keypad to be inactive
 */
void gpio_keypad_inactive(void)
{
    /* Keypad IOMUX settings configured at boot. */
}


/*!
 * Setup GPIO for a CSPI device to be active
 *
 * @param  cspi_mod         an CSPI device
 */
void gpio_spi_active(int cspi_mod)
{
    /* 
     * SPI1 is not used on the SCM-A11 Wingboards; it conflicts with the 
     * UART1 GPIO settings on P0 Wingboards.
     *
     * SPI2 is setup by the BP. This is setup is initiated by the MBM at
     * boot time.
     */
}


/*!
 * Setup GPIO for a CSPI device to be inactive
 *
 * @param  cspi_mod         a CSPI device
 */
void gpio_spi_inactive(int cspi_mod)
{
    /* No SPI IOMUX changes are required. */
}


/*!
 * Setup GPIO for an I2C device to be active
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_active(int i2c_num)
{
    /* I2C IOMUX settings configured at boot. */
}


/*!
 * Setup GPIO for an I2C device to be inactive
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_inactive(int i2c_num)
{
    /* I2C IOMUX settings configured at boot. */
}
#endif /* CONFIG_MOT_FEAT_GPIO_API */
