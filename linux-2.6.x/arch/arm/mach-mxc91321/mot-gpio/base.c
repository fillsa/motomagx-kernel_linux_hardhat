/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/base.c
 *
 * ArgonLV implementation of Motorola GPIO API common functions.
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
 * 27-Oct-2006  Motorola        Initial revision.
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
    switch (port) {
        /* UART4 IOMUX Configuration - IRDA */
        case 3:     /* UART4 */
            if (no_irda == 0) {
                /* Pin GPIO14 muxed to MCU GPIO Port Pin 14 at boot. */
                gpio_signal_set_data(GPIO_SIGNAL_IRDA_SD, GPIO_DATA_LOW);
            } 
            break;

        case 0:     /* UART1 */
        case 1:     /* UART2 */
        case 2:     /* UART3 */
        default:
            break;
    }
}


/*!
 * Setup GPIO for a UART port to be inactive
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_inactive(int port, int no_irda)
{
    switch (port) {
        case 3:     /* UART4 */
            gpio_signal_set_data(GPIO_SIGNAL_IRDA_SD, GPIO_DATA_HIGH);
            break;

        case 0:     /* UART1 */
        case 1:     /* UART2 */
        case 2:     /* UART3 */
        default:
            break;
    }
}


/*!
 * Configure the IOMUX GPR register to receive shared SDMA UART events
 *
 * @param  port         a UART port
 */
void config_uartdma_event(int port) 
{
    switch (port) {
        case 3:
            /* Configure to receive UART 4 SDMA events. */
            /* See Table 4-47 (GEN_P_REG1 register connection
             *   description in ArgonLV DTS 1.2. */
            iomux_config_gpr(MUX_PGP_FIRI, 0);
            break;
            
        default:
            break;
    }
}


/**
 * Setup GPIO for a Keypad to be active
 */
void gpio_keypad_active(void)
{
    return;
} 


/**
 * Setup GPIO for a keypad to be inactive
 */
void gpio_keypad_inactive(void)
{
    return;
}


/**
 * Setup GPIO for a CSPI device to be active
 *
 * @param  cspi_mod         an CSPI device
 */
void gpio_spi_active(int cspi_mod)
{
    return;
}


/**
 * Setup GPIO for a CSPI device to be inactive
 *
 * @param  cspi_mod         a CSPI device
 */
void gpio_spi_inactive(int cspi_mod)
{
    return;
}


/*!
 * Setup GPIO for an I2C device to be active
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_active(int i2c_num) 
{
}


/*!
 * Setup GPIO for an I2C device to be inactive
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_inactive(int i2c_num) 
{
}
#endif /* CONFIG_MOT_FEAT_GPIO_API */
