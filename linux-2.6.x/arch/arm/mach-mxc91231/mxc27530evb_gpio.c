/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <linux/module.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/clock.h>
#include <linux/delay.h>

#include "iomux.h"

/*!
 * @file mxc27530evb_gpio.c
 *
 * @brief This file contains all the GPIO setup functions for the board.
 *
 * @ingroup GPIO
 */

/*!
 * This system-wise GPIO function initializes the pins during system startup.
 * All the statically linked device drivers should put the proper GPIO initialization
 * code inside this function. It is called by \b fixup_mxc_board() during
 * system startup. This function is board specific.
 */
void mxc27530evb_gpio_init(void)
{
	/* config CS5 */

	iomux_config_mux(AP_GPIO_AP_C15, OUTPUTCONFIG_FUNC4, INPUTCONFIG_NONE);
}

/*!
 * Setup GPIO for a UART port to be active
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_active(int port, int no_irda)
{
	unsigned int pbc_bctrl1_set = 0;

	/*
	 * Configure the IOMUX control registers for the UART signals
	 * and enable the UART transceivers
	 */
	switch (port) {
		/* UART 1 IOMUX Configs */
	case 0:
		iomux_config_mux(AP_U1_TXD, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(AP_U1_CTS_B, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(AP_U1_RXD, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(AP_U1_RTS_B, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		/*
		 * Enable the UART 1 Transceiver or Irda Transmitter
		 */
		if (no_irda == 1) {
			pbc_bctrl1_set |= PBC_BCTRL1_UENA;
		} else {
			iomux_config_mux(AP_GPIO_AP_C17, OUTPUTCONFIG_DEFAULT,
					 INPUTCONFIG_DEFAULT);
			gpio_config(2, 17, true, GPIO_INT_NONE);
			/* Clear the SD/Mode signal */
			gpio_set_data(2, 17, 0);
			pbc_bctrl1_set |= PBC_BCTRL1_IREN;
		}
		break;
		/* UART 2 IOMUX Configs */
	case 1:

		iomux_config_mux(SP_USB_SE0_VM, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);	/* TXD */
		iomux_config_mux(SP_USB_DAT_VP, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);	/* RXD */
		iomux_config_mux(SP_USB_RXD, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);	/* RTS */
		iomux_config_mux(AP_U2_CTS_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);	/* CTS */
		iomux_config_mux(AP_U2_RI_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);	/* RI */
		iomux_config_mux(AP_U2_DTR_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);	/* DTR */
		iomux_config_mux(AP_U2_DSR_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);	/* DSR */
		/*
		 * Enable the UART 2 Transceiver
		 */
		pbc_bctrl1_set |= PBC_BCTRL1_UENCE;
		break;
		/* UART 3 IOMUX Configs */
	case 2:
		iomux_config_mux(SP_U3_TXD, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_U3_CTS_B, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_U3_RXD, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_U3_RTS_B, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		/*
		 * Enable the UART 3 Transceiver or Irda Transmitter
		 */
		if (no_irda == 1) {
			pbc_bctrl1_set |= PBC_BCTRL1_UENB;
		} else {
			pbc_bctrl1_set |= PBC_BCTRL1_IREN;
		}
		break;
	default:
		break;
	}
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
	/*
	 * TODO: Configure the Pad registers for the UART pins
	 */
}

/*!
 * Setup GPIO for a UART port to be inactive
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_inactive(int port, int no_irda)
{
	unsigned int pbc_bctrl1_clr = 0;

	/*
	 * Disable the UART Transceiver by configuring the GPIO pin
	 */
	switch (port) {
	case 0:
		/*
		 * Disable the UART 1 Transceiver
		 */
		if (no_irda == 1) {
			pbc_bctrl1_clr |= PBC_BCTRL1_UENA;
		} else {
			pbc_bctrl1_clr |= PBC_BCTRL1_IREN;
		}
		break;
	case 1:
		/*
		 * Disable the UART 2 Transceiver
		 */
		pbc_bctrl1_clr |= PBC_BCTRL1_UENCE;
		break;
	case 2:
		/*
		 * Disable the UART 3 Transceiver
		 */
		if (no_irda == 1) {
			pbc_bctrl1_clr |= PBC_BCTRL1_UENB;
		} else {
			pbc_bctrl1_clr |= PBC_BCTRL1_IREN;
		}
		break;
	default:
		break;
	}
	__raw_writew(pbc_bctrl1_clr, PBC_BASE_ADDRESS + PBC_BCTRL1_CLEAR);
}

/*
 * The function is stubbed out and does not apply for MXC91231
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
	iomux_config_mux(AP_KPROW0, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPROW1, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPROW2, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPROW3, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPROW4, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPROW5, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_GPIO_AP_B17, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_GPIO_AP_B18, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPCOL0, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPCOL1, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPCOL2, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPCOL3, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPCOL4, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_KPCOL5, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_GPIO_AP_B22, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
	iomux_config_mux(AP_GPIO_AP_B23, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
}

EXPORT_SYMBOL(gpio_keypad_active);

/*!
 * Setup GPIO for keypad to be inactive
 */
void gpio_keypad_inactive(void)
{
	iomux_config_mux(AP_KPROW0, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPROW1, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPROW2, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPROW3, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPROW4, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPROW5, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_GPIO_AP_B17, OUTPUTCONFIG_DEFAULT,
			 INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_GPIO_AP_B18, OUTPUTCONFIG_DEFAULT,
			 INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPCOL0, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPCOL1, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPCOL2, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPCOL3, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPCOL4, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_KPCOL5, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_GPIO_AP_B22, OUTPUTCONFIG_DEFAULT,
			 INPUTCONFIG_DEFAULT);
	iomux_config_mux(AP_GPIO_AP_B23, OUTPUTCONFIG_DEFAULT,
			 INPUTCONFIG_DEFAULT);
}

EXPORT_SYMBOL(gpio_keypad_inactive);

/*!
 * Setup GPIO for a CSPI device to be active
 *
 * @param  cspi_mod         an CSPI device
 */
void gpio_spi_active(int cspi_mod)
{
	switch (cspi_mod) {
	case 0:
		/* SPI1_SS0 IOMux configuration */
		iomux_config_mux(SP_SPI1_SS0, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI1 MISO IOMux configuration */
		iomux_config_mux(SP_SPI1_MISO, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI1 MOSI IOMux configuration */
		iomux_config_mux(SP_SPI1_MOSI, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI1_SS1 IOMux configuration */
		iomux_config_mux(SP_SPI1_SS1, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI1 CLK IOMux configuration */
		iomux_config_mux(SP_SPI1_CLK, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		break;
	case 1:
		/* SPI2 MOSI IOMux configuration */
		iomux_config_mux(SP_SD2_CMD, OUTPUTCONFIG_FUNC2,
				 INPUTCONFIG_FUNC2);
		/* SPI2 SPI2_CLK IOMux configuration */
		iomux_config_mux(SP_SD2_CLK, OUTPUTCONFIG_FUNC2,
				 INPUTCONFIG_FUNC2);
		/* SPI2 SPI2_MISO IOMux configuration */
		iomux_config_mux(SP_SD2_DAT0, OUTPUTCONFIG_FUNC2,
				 INPUTCONFIG_FUNC2);
		/* SPI2 SPI2_SS3 IOMux configuration */
		iomux_config_mux(SP_SD2_DAT3, OUTPUTCONFIG_FUNC2,
				 INPUTCONFIG_FUNC2);
		break;
	default:
		break;
	}
}

/*!
 * Setup GPIO for a CSPI device to be inactive
 *
 * @param  cspi_mod         a CSPI device
 */
void gpio_spi_inactive(int cspi_mod)
{
	switch (cspi_mod) {
	case 0:
		/* SPI1_SS0 IOMux configuration */
		iomux_config_mux(SP_SPI1_SS0, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI1 MISO IOMux configuration */
		iomux_config_mux(SP_SPI1_MISO, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI1 MOSI IOMux configuration */
		iomux_config_mux(SP_SPI1_MOSI, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI1_SS1 IOMux configuration */
		iomux_config_mux(SP_SPI1_SS1, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI1 CLK IOMux configuration */
		iomux_config_mux(SP_SPI1_CLK, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		break;
	case 1:
		/* SPI2 MOSI IOMux configuration */
		iomux_config_mux(SP_SD2_CMD, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI2 SPI2_CLK IOMux configuration */
		iomux_config_mux(SP_SD2_CLK, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI2 SPI2_MISO IOMux configuration */
		iomux_config_mux(SP_SD2_DAT0, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		/* SPI2 SPI2_SS3 IOMux configuration */
		iomux_config_mux(SP_SD2_DAT3, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		break;
	default:
		break;
	}
}

/*!
 * Setup GPIO for an I2C device to be active
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_active(int i2c_num)
{
	switch (i2c_num) {
	case 0:
		iomux_config_mux(AP_I2CLK, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(AP_I2DAT, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
	default:
		break;
	}
}

/*!
 * Setup GPIO for an I2C device to be inactive
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_inactive(int i2c_num)
{
	switch (i2c_num) {
	case 0:
		iomux_config_mux(AP_I2CLK, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(AP_I2DAT, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		break;
	default:
		break;
	}
}

/*!
 * Setup 1-Wire to be active
 */
void gpio_owire_active(void)
{
	/*
	 * Configure the IOMUX control register for 1-wire signals.
	 */
	iomux_config_mux(AP_OWDAT, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
}

/*!
 * Setup 1-Wire to be active
 */
void gpio_owire_inactive(void)
{
	/*
	 * Configure the IOMUX control register for 1-wire signals.
	 */
	iomux_config_mux(AP_OWDAT, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);
}

/*! macros related to hadrware workaround on MXC91231. This modification allows
 * to send sound to port 7 instead of port 4
 * See dam.c to complete the workaround for SSI MXC91231 pass1
 */
#define MXC91231_EVB_PASS1 1
#define MXC91231_EVB_PASS1_SSI1 1
#define MXC91231_EVB_PASS1_SSI2 1

/*!
 * This function configures the IOMux block for MC13783 standard operations.
 *
 */
void gpio_mc13783_active(void)
{
	iomux_config_mux(AP_ED_INT5, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
//      edio_config(ED_INT5, 0, EDIO_INT_RISE_EDGE);

#ifdef MXC91231_EVB_PASS1
#ifdef MXC91231_EVB_PASS1_SSI1
	iomux_config_mux(AP_GPIO_AP_C8, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
#endif
#ifdef MXC91231_EVB_PASS1_SSI2
	iomux_config_mux(AP_GPIO_AP_C16, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
#endif
#endif
}

/*!
 * This function clears the MC13783 intrrupt.
 *
 */
void gpio_mc13783_clear_int(void)
{
//      edio_clear_int(ED_INT5);
//      edio_get_int(ED_INT5);
}

/*!
 * This function return the SPI connected to MC13783.
 *
 */
int gpio_mc13783_get_spi(void)
{
	/* MXC275-30 EVB -> SPI2 = 1 */
	return 1;
}

/*!
 * This function return the SPI smave select for MC13783.
 *
 */
int gpio_mc13783_get_ss(void)
{
	/* MXC275-30 EVB -> SS = 3 */
	return 3;
}

/*!
 * This function configures and drives a gpio to tell MC13783 to
 * power down.
 */
void gpio_mc13783_power_off(void)
{
	local_irq_disable();

	/*
	 * Configure GPIO C15 as an output, as a GPIO, and as LOW.
	 * This is on the cpu schematic as XC_WDOG_AP_B, which is
	 * connected to the MC13783 WDI input pin.  When MC13783 is in
	 * RUN mode, setting this line LOW will turn it off.
	 */

	/* Configure GPIO C15 as an output */
	gpio_config(2, 15, true, GPIO_INT_NONE);

	/* Configure GPIO C15 as a GPIO in the IOMUX */
	iomux_config_mux(AP_GPIO_AP_C15, OUTPUTCONFIG_DEFAULT,
			 INPUTCONFIG_NONE);

	/* Configure GPIO C15 as low */
	gpio_set_data(2, 15, 0);

	while (1) ;
}

/*!
 * Setup GPIO for SDHC to be active
 *
 * @param module SDHC module number
 */
void gpio_sdhc_active(int module)
{
	unsigned int pbc_bctrl1_set = 0;
	switch (module) {
	case 0:
		iomux_config_mux(SP_SD1_CMD, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_SD1_CLK, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_SD1_DAT0, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_SD1_DAT1, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_SD1_DAT2, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_SD1_DAT3, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		/*
		 * select 2.53 - 3.32v voltage for the MMC1
		 * interface.
		 */
		pbc_bctrl1_set |= PBC_BCTRL1_MCP1;
		break;
	case 1:
		iomux_config_mux(SP_SD2_CMD, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_NONE);
		iomux_config_mux(SP_SD2_CLK, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_NONE);
		iomux_config_mux(SP_SD2_DAT0, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_SD2_DAT1, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_SD2_DAT2, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		iomux_config_mux(SP_SD2_DAT3, OUTPUTCONFIG_FUNC1,
				 INPUTCONFIG_FUNC1);
		/*
		 * select 2.53 - 3.32v voltage for the MMC2
		 * interface.
		 */
		pbc_bctrl1_set |= PBC_BCTRL1_MCP2;
		break;
	default:
		break;
	}
	pbc_bctrl1_set |= PBC_BCTRL1_MCP2;
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
}

EXPORT_SYMBOL(gpio_sdhc_active);

/*!
 * Setup GPIO for SDHC1 to be inactive
 *
 * @param module SDHC module number
 */
void gpio_sdhc_inactive(int module)
{
	unsigned int pbc_bctrl1_set = 0;
	switch (module) {
	case 0:
		iomux_config_mux(SP_SD1_CMD, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD1_CLK, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD1_DAT0, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD1_DAT1, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD1_DAT2, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD1_DAT3, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		pbc_bctrl1_set &= PBC_BCTRL1_MCP1;
		break;
	case 1:
		iomux_config_mux(SP_SD2_CMD, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD2_CLK, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD2_DAT0, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD2_DAT1, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD2_DAT2, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		iomux_config_mux(SP_SD2_DAT3, OUTPUTCONFIG_DEFAULT,
				 INPUTCONFIG_DEFAULT);
		pbc_bctrl1_set &= PBC_BCTRL1_MCP2;
		break;
	default:
		break;
	}
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
}

EXPORT_SYMBOL(gpio_sdhc_inactive);

/*
 * Probe for the mmc card.
 * @return The function returns 0 if card is inserted, 1 if removed.
 */
int sdhc_find_card(void)
{
	return gpio_get_data(3, 23);
}

EXPORT_SYMBOL(sdhc_find_card);

/*!
 * Invert the IOMUX/GPIO for SDHC1 SD1_DET.
 *
 * @param flag Flag represents whether the mmc card is inserted/removed.
 *             Using this sensitive level of GPIO signal is changed.
 *
 **/
void sdhc_intr_clear(int flag)
{
	if (flag) {
		set_irq_type(IOMUX_TO_IRQ(SP_SD2_DAT1), IRQT_FALLING);
	} else {
		set_irq_type(IOMUX_TO_IRQ(SP_SD2_DAT1), IRQT_RISING);
	}
}

EXPORT_SYMBOL(sdhc_intr_clear);

/*!
 * Setup the IOMUX/GPIO for SDHC1 SD1_DET.
 *
 * @param  host Pointer to MMC/SD host structure.
 * @param  handler      GPIO ISR function pointer for the GPIO signal.
 * @return The function returns 0 on success and -1 on failure.
 *
 **/
int sdhc_intr_setup(void *host,
		    irqreturn_t(*handler) (int, void *, struct pt_regs *))
{
	int ret;

	/* use SD2_DAT1 as GPIO for SD1_DET */
	iomux_config_mux(SP_SD2_DAT1, OUTPUTCONFIG_DEFAULT,
			 INPUTCONFIG_DEFAULT);

	/* check if a card in the slot if so we need to start with
	 * the proper edge definition
	 */
	sdhc_intr_clear(sdhc_find_card());

	ret = request_irq(IOMUX_TO_IRQ(SP_SD2_DAT1), handler,
			  0, "MXCMMC", host);
	return ret;
}

EXPORT_SYMBOL(sdhc_intr_setup);

/*!
 * Clear the IOMUX/GPIO for SDHC1 SD1_DET.
 */
void sdhc_intr_destroy(void *host)
{
	free_irq(IOMUX_TO_IRQ(SP_SD2_DAT1), host);
}

EXPORT_SYMBOL(sdhc_intr_destroy);

/*
 * Find the minimum clock for SDHC.
 *
 * @param clk SDHC module number.
 * @return Returns the minimum SDHC clock.
 */
unsigned int sdhc_get_min_clock(enum mxc_clocks clk)
{
	return (mxc_get_clocks(SDHC1_CLK) / 9) / 32;
}

EXPORT_SYMBOL(sdhc_get_min_clock);

/*
 * Find the maximum clock for SDHC.
 * @param clk SDHC module number.
 * @return Returns the maximum SDHC clock.
 */
unsigned int sdhc_get_max_clock(enum mxc_clocks clk)
{
	return mxc_get_clocks(SDHC1_CLK) / 2;
}

EXPORT_SYMBOL(sdhc_get_max_clock);

/*!
 * Setup GPIO for LCD to be active
 *
 */
void gpio_lcd_active(void)
{
	u16 pbc_bctrl1_set = 0;

	/* RTS */
	iomux_config_mux(AP_IPU_D2_CS, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);

	pbc_bctrl1_set = (u16) PBC_BCTRL1_LCDON;
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
}

/*!
 * Setup GPIO for LCD to be inactive
 *
 */
void gpio_lcd_inactive(void)
{
	u16 pbc_bctrl1_set = 0;

	/* RTS */
	iomux_config_mux(AP_IPU_D2_CS, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);

	pbc_bctrl1_set = (u16) PBC_BCTRL1_LCDON;
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET + 2);
}

/*!
 * Setup GPIO for sensor to be active
 *
 */
void gpio_sensor_active(void)
{
	u16 temp;

	temp = PBC_BCTRL1_SENSORON;

	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
}

void slcd_gpio_config(void)
{
	gpio_config(2, 22, true, GPIO_INT_NONE);
	gpio_set_data(2, 22, 1);
	iomux_config_mux(AP_ED_INT4, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT);

	gpio_set_data(2, 22, 0);
	msleep(100);
	gpio_set_data(2, 22, 1);
	msleep(100);
}
