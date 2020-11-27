/*
 * linux/arch/arm/mach-mx3/mx31ads_gpio.c
 *
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include "iomux.h"

/*!
 * @file mx31ads_gpio.c
 *
 * @brief This file contains all the GPIO setup functions for the board.
 *
 * @ingroup GPIO
 */

void gpio_activate_audio_ports(void);

/*!
 * This system-wise GPIO function initializes the pins during system startup.
 * All the statically linked device drivers should put the proper GPIO initialization
 * code inside this function. It is called by \b fixup_mx31ads() during
 * system startup. This function is board specific.
 */
void mx31ads_gpio_init(void)
{
	/* config CS4 */
	mxc_request_iomux(MX31_PIN_CS4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);

	/*Connect DAM ports 4 & 5 to enable audio I/O */
	gpio_activate_audio_ports();
}

/*!
 * Setup GPIO for a UART port to be active
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_active(int port, int no_irda)
{
	unsigned int pbc_bctrl1_clr = 0, pbc_bctrl2_set = 0, pbc_bctrl2_clr = 0;
	/*
	 * Configure the IOMUX control registers for the UART signals
	 */
	switch (port) {
		/* UART 1 IOMUX Configs */
	case 0:
		mxc_request_iomux(MX31_PIN_RXD1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_TXD1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_RTS1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CTS1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_DTR_DCE1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_DSR_DCE1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_RI_DCE1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_DCD_DCE1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);

		/* Enable the transceiver */
		pbc_bctrl1_clr |= PBC_BCTRL1_UENCE;
		pbc_bctrl2_set |= PBC_BCTRL2_USELC;
		break;
		/* UART 2 IOMUX Configs */
	case 1:
		mxc_request_iomux(MX31_PIN_TXD2, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_RXD2, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);

		if (no_irda == 1) {
			mxc_request_iomux(MX31_PIN_RTS2, OUTPUTCONFIG_FUNC,
					  INPUTCONFIG_FUNC);
			mxc_request_iomux(MX31_PIN_CTS2, OUTPUTCONFIG_FUNC,
					  INPUTCONFIG_FUNC);
			mxc_request_iomux(MX31_PIN_DTR_DCE2, OUTPUTCONFIG_FUNC,
					  INPUTCONFIG_FUNC);
			pbc_bctrl1_clr |= PBC_BCTRL1_UENCE;
			pbc_bctrl2_clr |= PBC_BCTRL2_USELC;
		} else {
			pbc_bctrl1_clr |= PBC_BCTRL1_IREN;
			pbc_bctrl2_clr |= PBC_BCTRL2_IRDA_MOD;
		}
		break;
		/* UART 3 IOMUX Configs */
	case 2:
		mxc_request_iomux(MX31_PIN_CSPI3_MOSI, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_CSPI3_MISO, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_CSPI3_SCLK, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_CSPI3_SPI_RDY, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);

		pbc_bctrl1_clr |= PBC_BCTRL1_UENB;
		pbc_bctrl2_clr |= PBC_BCTRL2_USELB;
		break;
		/* UART 4 IOMUX Configs */
	case 3:
		mxc_request_iomux(MX31_PIN_ATA_CS0, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_ATA_CS1, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_ATA_DIOR, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_ATA_DIOW, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);

		pbc_bctrl1_clr |= PBC_BCTRL1_UENB;
		pbc_bctrl2_set |= PBC_BCTRL2_USELB;
		break;
		/* UART 5 IOMUX Configs */
	case 4:
		mxc_request_iomux(MX31_PIN_PC_VS2, OUTPUTCONFIG_ALT2,
				  INPUTCONFIG_ALT2);
		mxc_request_iomux(MX31_PIN_PC_RST, OUTPUTCONFIG_ALT2,
				  INPUTCONFIG_ALT2);
		mxc_request_iomux(MX31_PIN_PC_BVD1, OUTPUTCONFIG_ALT2,
				  INPUTCONFIG_ALT2);
		mxc_request_iomux(MX31_PIN_PC_BVD2, OUTPUTCONFIG_ALT2,
				  INPUTCONFIG_ALT2);

		pbc_bctrl1_clr |= PBC_BCTRL1_UENA;
		pbc_bctrl2_set |= PBC_BCTRL2_USELA;
		break;
	default:
		break;
	}

	__raw_writew(pbc_bctrl1_clr, PBC_BASE_ADDRESS + PBC_BCTRL1_CLEAR);
	__raw_writew(pbc_bctrl2_set, PBC_BASE_ADDRESS + PBC_BCTRL2_SET);
	__raw_writew(pbc_bctrl2_clr, PBC_BASE_ADDRESS + PBC_BCTRL2_CLEAR);
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
	unsigned int pbc_bctrl1_set = 0;

	switch (port) {
	case 0:
		mxc_request_gpio(MX31_PIN_RXD1);
		mxc_request_gpio(MX31_PIN_TXD1);
		mxc_request_gpio(MX31_PIN_RTS1);
		mxc_request_gpio(MX31_PIN_CTS1);
		mxc_request_gpio(MX31_PIN_DTR_DCE1);
		mxc_request_gpio(MX31_PIN_DSR_DCE1);
		mxc_request_gpio(MX31_PIN_RI_DCE1);
		mxc_request_gpio(MX31_PIN_DCD_DCE1);

		mxc_free_iomux(MX31_PIN_RXD1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_TXD1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_RTS1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_CTS1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_DTR_DCE1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_DSR_DCE1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_RI_DCE1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_DCD_DCE1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);

		pbc_bctrl1_set |= PBC_BCTRL1_UENCE;
		break;
	case 1:
		mxc_request_gpio(MX31_PIN_TXD2);
		mxc_request_gpio(MX31_PIN_RXD2);

		mxc_free_iomux(MX31_PIN_TXD2, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_RXD2, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);

		if (no_irda == 1) {
			mxc_request_gpio(MX31_PIN_RTS2);
			mxc_request_gpio(MX31_PIN_CTS2);
			mxc_request_gpio(MX31_PIN_DTR_DCE2);

			mxc_free_iomux(MX31_PIN_RTS2, OUTPUTCONFIG_GPIO,
				       INPUTCONFIG_GPIO);
			mxc_free_iomux(MX31_PIN_CTS2, OUTPUTCONFIG_GPIO,
				       INPUTCONFIG_GPIO);
			mxc_free_iomux(MX31_PIN_DTR_DCE2, OUTPUTCONFIG_GPIO,
				       INPUTCONFIG_GPIO);

			pbc_bctrl1_set |= PBC_BCTRL1_UENCE;
		} else {
			pbc_bctrl1_set |= PBC_BCTRL1_IREN;
		}
		break;
	case 2:
		mxc_request_gpio(MX31_PIN_CSPI3_MOSI);
		mxc_request_gpio(MX31_PIN_CSPI3_MISO);
		mxc_request_gpio(MX31_PIN_CSPI3_SCLK);
		mxc_request_gpio(MX31_PIN_CSPI3_SPI_RDY);

		mxc_free_iomux(MX31_PIN_CSPI3_MOSI, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_CSPI3_MISO, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_CSPI3_SCLK, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_CSPI3_SPI_RDY, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);

		pbc_bctrl1_set |= PBC_BCTRL1_UENB;
		break;
	case 3:
		mxc_request_gpio(MX31_PIN_ATA_CS0);
		mxc_request_gpio(MX31_PIN_ATA_CS1);
		mxc_request_gpio(MX31_PIN_ATA_DIOR);
		mxc_request_gpio(MX31_PIN_ATA_DIOW);

		mxc_free_iomux(MX31_PIN_ATA_CS0, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_ATA_CS1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_ATA_DIOR, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_ATA_DIOW, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);

		pbc_bctrl1_set |= PBC_BCTRL1_UENB;
		break;
	case 4:
		mxc_request_gpio(MX31_PIN_PC_VS2);
		mxc_request_gpio(MX31_PIN_PC_BVD1);
		mxc_request_gpio(MX31_PIN_PC_BVD2);
		mxc_request_gpio(MX31_PIN_PC_RST);

		mxc_free_iomux(MX31_PIN_PC_VS2, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_PC_BVD1, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_PC_BVD2, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);
		mxc_free_iomux(MX31_PIN_PC_RST, OUTPUTCONFIG_GPIO,
			       INPUTCONFIG_GPIO);

		pbc_bctrl1_set |= PBC_BCTRL1_UENA;
		break;
	default:
		break;
	}
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
}

/*!
 * Configure the IOMUX GPR register to receive shared SDMA UART events
 *
 * @param  port         a UART port
 */
void config_uartdma_event(int port)
{
	switch (port) {
	case 1:
		/* Configure to receive UART 2 SDMA events */
		mxc_iomux_set_gpr(MUX_PGP_FIRI, false);
		break;
	case 2:
		/* Configure to receive UART 3 SDMA events */
		mxc_iomux_set_gpr(MUX_CSPI1_UART3, true);
		break;
	case 4:
		/* Configure to receive UART 5 SDMA events */
		mxc_iomux_set_gpr(MUX_CSPI3_UART5_SEL, true);
		break;
	default:
		break;
	}
}

/*!
 * Setup GPIO for Keypad  to be active
 *
 */
void gpio_keypad_active(void)
{
	/*
	 * Configure the IOMUX control register for keypad signals.
	 */
	mxc_request_iomux(MX31_PIN_KEY_COL0, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_COL1, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_COL2, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_COL3, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_COL4, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_COL5, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_COL6, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_COL7, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_ROW0, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_ROW1, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_ROW2, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_ROW3, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_ROW4, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_ROW5, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_ROW6, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_KEY_ROW7, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
}

EXPORT_SYMBOL(gpio_keypad_active);

/*!
 * Setup GPIO for Keypad to be inactive
 *
 */
void gpio_keypad_inactive(void)
{
	mxc_request_iomux(MX31_PIN_KEY_COL0, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_COL1, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_COL2, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_COL3, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_COL4, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_COL5, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_COL6, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_COL7, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_ROW0, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_ROW1, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_ROW2, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_ROW3, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_ROW4, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_ROW5, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_ROW6, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_request_iomux(MX31_PIN_KEY_ROW7, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
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
		/* SPI1 */
		mxc_request_iomux(MX31_PIN_CSPI1_MISO, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI1_MOSI, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI1_SCLK, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI1_SPI_RDY, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI1_SS0, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI1_SS1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI1_SS2, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		break;
	case 1:
		/* SPI2 */
		mxc_request_iomux(MX31_PIN_CSPI2_MISO, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI2_MOSI, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI2_SCLK, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI2_SPI_RDY, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI2_SS0, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI2_SS1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		break;
	case 2:
		/* SPI3 */
		/*
		   mxc_request_iomux(MX31_PIN_CSPI2_MISO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		   mxc_request_iomux(MX31_PIN_CSPI2_MOSI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		   mxc_request_iomux(MX31_PIN_CSPI2_SCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		   mxc_request_iomux(MX31_PIN_CSPI2_SPI_RDY, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		   mxc_request_iomux(MX31_PIN_CSPI2_SS0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		   mxc_request_iomux(MX31_PIN_CSPI2_SS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		   mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		 */
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
	iomux_config_mux(MX31_PIN_BATT_LINE, OUTPUTCONFIG_FUNC,
			 INPUTCONFIG_FUNC);
	iomux_config_pad(MX31_PIN_BATT_LINE, PAD_CTL_LOOPBACK);
}

/*!
 * Setup 1-Wire to be active
 */
void gpio_owire_inactive(void)
{
	/*
	 * Configure the IOMUX control register for 1-wire signals.
	 */
	iomux_config_mux(MX31_PIN_BATT_LINE, OUTPUTCONFIG_GPIO,
			 INPUTCONFIG_GPIO);
}

EXPORT_SYMBOL(gpio_owire_active);
EXPORT_SYMBOL(gpio_owire_inactive);

/*!
 * Setup GPIO for a CSPI device to be inactive
 *
 * @param  cspi_mod         a CSPI device
 */
void gpio_spi_inactive(int cspi_mod)
{
	switch (cspi_mod) {
	case 0:
		/* SPI1 */
		mxc_request_iomux(MX31_PIN_CSPI1_MISO, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI1_MOSI, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI1_SCLK, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI1_SPI_RDY, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI1_SS0, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI1_SS1, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI1_SS2, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		break;
	case 1:
		/* SPI2 */
		mxc_request_iomux(MX31_PIN_CSPI2_MISO, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI2_MOSI, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI2_SCLK, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI2_SPI_RDY, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI2_SS0, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI2_SS1, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_GPIO);
		break;
	case 2:
		/* SPI3 */
		/*
		   mxc_request_iomux(MX31_PIN_CSPI2_MISO, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		   mxc_request_iomux(MX31_PIN_CSPI2_MOSI, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		   mxc_request_iomux(MX31_PIN_CSPI2_SCLK, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		   mxc_request_iomux(MX31_PIN_CSPI2_SPI_RDY, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		   mxc_request_iomux(MX31_PIN_CSPI2_SS0, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		   mxc_request_iomux(MX31_PIN_CSPI2_SS1, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		   mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		 */
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
		mxc_request_iomux(MX31_PIN_I2C_CLK, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_I2C_DAT, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		break;
	case 1:
		mxc_request_iomux(MX31_PIN_CSPI2_MOSI, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_CSPI2_MISO, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		break;
	case 2:
		mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_CSPI2_SCLK, OUTPUTCONFIG_ALT1,
				  INPUTCONFIG_ALT1);
		break;
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
		mxc_request_iomux(MX31_PIN_I2C_CLK, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_I2C_DAT, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_FUNC);
		break;
	case 1:
		mxc_request_iomux(MX31_PIN_CSPI2_MOSI, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_CSPI2_MISO, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_ALT1);
		break;
	case 2:
		mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_ALT1);
		mxc_request_iomux(MX31_PIN_CSPI2_SCLK, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_ALT1);
		break;
	default:
		break;
	}
}

/*!
 * This function configures the IOMux block for MC13783 standard operations.
 *
 */
void gpio_mc13783_active(void)
{
	mxc_request_iomux(MX31_PIN_GPIO1_3, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_set_gpio_direction(MX31_PIN_GPIO1_3, 1);
//      mxc_set_gpio_edge_ctrl(MX31_PIN_GPIO1_3, GPIO_INT_RISE_EDGE);
}

/*!
 * This function clears the MC13783 intrrupt.
 *
 */
void gpio_mc13783_clear_int(void)
{
}

/*!
 * This function return the SPI connected to MC13783.
 *
 */
int gpio_mc13783_get_spi(void)
{
	return 1;
}

/*!
 * This function return the SPI smave select for MC13783.
 *
 */
int gpio_mc13783_get_ss(void)
{
	return 0;
}

/*!
 * This function activates DAM ports 4 & 5 to enable
 * audio I/O. Thsi function is called from mx31ads_gpio_init
 * function, which is board-specific.
 */
void gpio_activate_audio_ports(void)
{
	/* config Audio ports (4 & 5) */
	mxc_request_iomux(MX31_PIN_SCK4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_SRXD4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_STXD4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_SFS4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_SCK5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_SRXD5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_STXD5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_SFS5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
}

/*!
 * Setup GPIO for SDHC to be active
 *
 * @param module SDHC module number
 */
void gpio_sdhc_active(int module)
{
	switch (module) {
	case 0:
		mxc_request_iomux(MX31_PIN_SD1_CLK, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_CMD, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_DATA0, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_DATA1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_DATA2, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_DATA3, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		break;
	case 1:
		/* TODO:what are the pins for SDHC2? */
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_sdhc_active);

/*!
 * Setup GPIO for SDHC1 to be inactive
 *
 * @param module SDHC module number
 */
void gpio_sdhc_inactive(int module)
{
	switch (module) {
	case 0:
		mxc_request_iomux(MX31_PIN_SD1_CLK, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_NONE);
		mxc_request_iomux(MX31_PIN_SD1_CMD, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_NONE);
		mxc_request_iomux(MX31_PIN_SD1_DATA0, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_NONE);
		mxc_request_iomux(MX31_PIN_SD1_DATA1, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_NONE);
		mxc_request_iomux(MX31_PIN_SD1_DATA2, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_NONE);
		mxc_request_iomux(MX31_PIN_SD1_DATA3, OUTPUTCONFIG_GPIO,
				  INPUTCONFIG_NONE);
		break;
	case 1:
		/* TODO:what are the pins for SDHC2? */
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(gpio_sdhc_inactive);

/*!
 * Setup the IOMUX/GPIO for SDHC1 SD1_DET.
 *
 * @param  host Pointer to MMC/SD host structure.
 * @param  handler      GPIO ISR function pointer for the GPIO signal.
 * @return The function returns 0 on success and -1 on failure.
 *
 **/
int sdhc_intr_setup(void *host, gpio_irq_handler handler)
{
	return 0;
}

EXPORT_SYMBOL(sdhc_intr_setup);

/*!
 * Invert the IOMUX/GPIO for SDHC1 SD1_DET.
 *
 * @param flag Flag represents whether the card is inserted/removed.
 *
 **/
void sdhc_intr_clear(int flag)
{
}

EXPORT_SYMBOL(sdhc_intr_clear);

/*!
 * Clear the IOMUX/GPIO for SDHC1 SD1_DET.
 */
void sdhc_intr_destroy(void)
{
}

EXPORT_SYMBOL(sdhc_intr_destroy);

/*!
 * Setup GPIO for LCD to be active
 *
 */
void gpio_lcd_active(void)
{
	u16 temp;

	mxc_request_iomux(MX31_PIN_LD0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD10, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD11, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD12, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD13, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD14, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD15, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD16, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// LD16
	mxc_request_iomux(MX31_PIN_LD17, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// LD17
	mxc_request_iomux(MX31_PIN_VSYNC3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// VSYNC
	mxc_request_iomux(MX31_PIN_HSYNC, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// HSYNC
	mxc_request_iomux(MX31_PIN_FPSHIFT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// CLK
	mxc_request_iomux(MX31_PIN_DRDY0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// DRDY
	mxc_request_iomux(MX31_PIN_D3_REV, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// REV
	mxc_request_iomux(MX31_PIN_CONTRAST, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// CONTR
	mxc_request_iomux(MX31_PIN_D3_SPL, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// SPL
	mxc_request_iomux(MX31_PIN_D3_CLS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// CLS

	temp = PBC_BCTRL1_LCDON;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
}

/*!
 * Setup GPIO for LCD to be inactive
 *
 */
void gpio_lcd_inactive(void)
{
	u16 pbc_bctrl1_set = 0;

	pbc_bctrl1_set = (u16) PBC_BCTRL1_LCDON;
	__raw_writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET + 2);
}

/*!
 * camera reset/standby setup
 *
 */
void gpio_sensor_setup(void)
{
}

/*!
 * camera reset
 * @param flag: true - reset  false - out of reset.
 */
void gpio_sensor_reset(bool flag)
{
}

/*!
 * camera suspend
 * @param suspend: true - suspend mode false - normal operational mode.
 */
void gpio_sensor_suspend(bool flag)
{
}

/*!
 * Setup pins for SLCD to be active
 *
 */
void slcd_gpio_config(void)
{
	u16 temp;

	/* Reset smart lcd */
	temp = PBC_BCTRL2_LDC_RST0;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL2_CLEAR);
	msleep(2);
	/* Bring out of reset */
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL2_SET);
	msleep(2);

	mxc_request_iomux(MX31_PIN_LD0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD10, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD11, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD12, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD13, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD14, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD15, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD16, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_LD17, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);

	mxc_request_iomux(MX31_PIN_READ, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	/* read */
	mxc_request_iomux(MX31_PIN_WRITE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	/* write */
	mxc_request_iomux(MX31_PIN_PAR_RS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	/* RS */
	mxc_request_iomux(MX31_PIN_LCS0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	/* chip select */

	/* Enable smart lcd interface */
	temp = PBC_BCTRL2_LDCIO_EN;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL2_CLEAR);
}

/*!
 * Switch to the specified sensor - MX31 ADS has two
 *
 */
void gpio_sensor_select(int sensor)
{
	u16 temp;

	switch (sensor) {
	case 0:
		temp = PBC_BCTRL1_SENSOR2_ON;
		__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL1_CLEAR);
		temp = PBC_BCTRL1_SENSOR1_ON;
		__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
		break;
	case 1:
		temp = PBC_BCTRL1_SENSOR1_ON;
		__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL1_CLEAR);
		temp = PBC_BCTRL1_SENSOR2_ON;
		__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
		break;
	default:
		break;
	}
}

/*!
 * Setup GPIO for sensor to be active
 *
 */
void gpio_sensor_active(void)
{
	u16 temp;

	gpio_sensor_select(0);	// TODO - need method to select the camera

	/*
	 * Configure the iomuxen for the CSI.
	 */

	mxc_request_iomux(MX31_PIN_CSI_D4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D10, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D11, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D12, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D13, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D14, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_D15, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_HSYNC, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_MCLK, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_PIXCLK, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_CSI_VSYNC, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);

#ifdef CONFIG_MXC_IPU_CAMERA_16BIT
	/*
	 * The other 4 data bits are multiplexed on MX31.
	 */
	mxc_request_iomux(MX31_PIN_ATA_CS0, OUTPUTCONFIG_ALT2,
			  INPUTCONFIG_ALT2);
	mxc_request_iomux(MX31_PIN_ATA_CS1, OUTPUTCONFIG_ALT2,
			  INPUTCONFIG_ALT2);
	mxc_request_iomux(MX31_PIN_ATA_DIOR, OUTPUTCONFIG_ALT2,
			  INPUTCONFIG_ALT2);
	mxc_request_iomux(MX31_PIN_ATA_DIOW, OUTPUTCONFIG_ALT2,
			  INPUTCONFIG_ALT2);
#endif

	/*
	 * Now enable the CSI buffers
	 */

	temp = PBC_BCTRL2_CSI_EN;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL2_CLEAR);

#ifdef CONFIG_MXC_IPU_CAMERA_16BIT
	/*
	 * Enable the other buffer for the additional 4 data bits.
	 */
	temp = PBC_BCTRL4_CSI_MSB_EN;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL4_CLEAR);
#endif
}

/*!
 * Setup GPIO for ATA interface
 *
 */
void gpio_ata_active(void)
{
	/*
	 * Configure the GPR for ATA group B signals
	 */

	mxc_iomux_set_gpr(MUX_PGP_ATA_7 | MUX_PGP_ATA_6 | MUX_PGP_ATA_2 |
			  MUX_PGP_ATA_1, true);

	/*
	 * Configure the IOMUX for ATA group B signals
	 */

	mxc_request_iomux(MX31_PIN_CSPI1_MOSI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D0
	mxc_request_iomux(MX31_PIN_CSPI1_MISO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D1
	mxc_request_iomux(MX31_PIN_CSPI1_SS0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D2
	mxc_request_iomux(MX31_PIN_CSPI1_SS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D3
	mxc_request_iomux(MX31_PIN_CSPI1_SS2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D4
	mxc_request_iomux(MX31_PIN_CSPI1_SCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D5
	mxc_request_iomux(MX31_PIN_CSPI1_SPI_RDY, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D6
	mxc_request_iomux(MX31_PIN_STXD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D7
	mxc_request_iomux(MX31_PIN_SRXD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D8
	mxc_request_iomux(MX31_PIN_SCK3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D9
	mxc_request_iomux(MX31_PIN_SFS3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D10
	mxc_request_iomux(MX31_PIN_STXD6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D11
	mxc_request_iomux(MX31_PIN_SRXD6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D12
	mxc_request_iomux(MX31_PIN_SCK6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D13
	mxc_request_iomux(MX31_PIN_CAPTURE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D14
	mxc_request_iomux(MX31_PIN_COMPARE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_D15

	mxc_request_iomux(MX31_PIN_USBH2_STP, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_DMARQ_B
	mxc_request_iomux(MX31_PIN_USBH2_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_INTRQ_B
	mxc_request_iomux(MX31_PIN_USBH2_NXT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_DA0
	mxc_request_iomux(MX31_PIN_USBH2_DATA0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_DA1
	mxc_request_iomux(MX31_PIN_USBH2_DATA1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_DA2
	mxc_request_iomux(MX31_PIN_USBH2_DIR, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// ATA_BUFFER_DIR

	/* These ATA pins are common to Group A and Group B */

	mxc_request_iomux(MX31_PIN_ATA_CS0, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_ATA_CS1, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_ATA_DIOR, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_ATA_DIOW, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_ATA_DMACK, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_ATA_RESET_B, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);

	/* Need fast slew rate for UDMA mode */

	mxc_iomux_set_pad(MX31_PIN_CSPI1_MISO, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 0
	mxc_iomux_set_pad(MX31_PIN_CSPI1_MOSI, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 1
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS0, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 2
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS1, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 3
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS2, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 4
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SCLK, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 5
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SPI_RDY, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 6
	mxc_iomux_set_pad(MX31_PIN_STXD3, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 7
	mxc_iomux_set_pad(MX31_PIN_SRXD3, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 8
	mxc_iomux_set_pad(MX31_PIN_SCK3, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 9
	mxc_iomux_set_pad(MX31_PIN_SFS3, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 10
	mxc_iomux_set_pad(MX31_PIN_STXD3, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 11
	mxc_iomux_set_pad(MX31_PIN_SRXD6, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 12
	mxc_iomux_set_pad(MX31_PIN_SCK6, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 13
	mxc_iomux_set_pad(MX31_PIN_CAPTURE, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 14
	mxc_iomux_set_pad(MX31_PIN_COMPARE, PAD_CTL_SRE_FAST | PAD_CTL_PKE_NONE);	// data 12
}

EXPORT_SYMBOL(gpio_ata_active);

/*!
 * Restore ATA interface pins to reset values
 *
 */
void gpio_ata_inactive(void)
{
	/*
	 * Turn off ATA group B signals
	 */
	mxc_request_iomux(MX31_PIN_CSPI1_MOSI, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D0
	mxc_request_iomux(MX31_PIN_CSPI1_MISO, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D1
	mxc_request_iomux(MX31_PIN_CSPI1_SS0, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D2
	mxc_request_iomux(MX31_PIN_CSPI1_SS1, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D3
	mxc_request_iomux(MX31_PIN_CSPI1_SS2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D4
	mxc_request_iomux(MX31_PIN_CSPI1_SCLK, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D5
	mxc_request_iomux(MX31_PIN_CSPI1_SPI_RDY, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D6
	mxc_request_iomux(MX31_PIN_STXD3, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D7
	mxc_request_iomux(MX31_PIN_SRXD3, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D8
	mxc_request_iomux(MX31_PIN_SCK3, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D9
	mxc_request_iomux(MX31_PIN_SFS3, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D10
	mxc_request_iomux(MX31_PIN_STXD6, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D11
	mxc_request_iomux(MX31_PIN_SRXD6, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D12
	mxc_request_iomux(MX31_PIN_SCK6, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D13
	mxc_request_iomux(MX31_PIN_CAPTURE, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D14
	mxc_request_iomux(MX31_PIN_COMPARE, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_D15

	mxc_request_iomux(MX31_PIN_USBH2_STP, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_DMARQ_B
	mxc_request_iomux(MX31_PIN_USBH2_CLK, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_INTRQ_B
	mxc_request_iomux(MX31_PIN_USBH2_NXT, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_DA0
	mxc_request_iomux(MX31_PIN_USBH2_DATA0, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_DA1
	mxc_request_iomux(MX31_PIN_USBH2_DATA1, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_DA2
	mxc_request_iomux(MX31_PIN_USBH2_DIR, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	// ATA_BUFFER_DIR

	/* These ATA pins are common to Group A and Group B */

	mxc_request_iomux(MX31_PIN_ATA_CS0, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_NONE);
	mxc_request_iomux(MX31_PIN_ATA_CS1, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_NONE);
	mxc_request_iomux(MX31_PIN_ATA_DIOR, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_NONE);
	mxc_request_iomux(MX31_PIN_ATA_DIOW, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_NONE);
	mxc_request_iomux(MX31_PIN_ATA_DMACK, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_NONE);
	mxc_request_iomux(MX31_PIN_ATA_RESET_B, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_NONE);

	/* Needed fast slew rate for UDMA mode */

	mxc_iomux_set_pad(MX31_PIN_CSPI1_MISO, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 0
	mxc_iomux_set_pad(MX31_PIN_CSPI1_MOSI, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 1
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS0, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 2
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS1, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 3
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS2, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 4
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SCLK, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 5
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SPI_RDY, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 6
	mxc_iomux_set_pad(MX31_PIN_STXD3, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 7
	mxc_iomux_set_pad(MX31_PIN_SRXD3, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 8
	mxc_iomux_set_pad(MX31_PIN_SCK3, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 9
	mxc_iomux_set_pad(MX31_PIN_SFS3, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 10
	mxc_iomux_set_pad(MX31_PIN_STXD3, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 11
	mxc_iomux_set_pad(MX31_PIN_SRXD6, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 12
	mxc_iomux_set_pad(MX31_PIN_SCK6, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 13
	mxc_iomux_set_pad(MX31_PIN_CAPTURE, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 14
	mxc_iomux_set_pad(MX31_PIN_COMPARE, PAD_CTL_SRE_SLOW | PAD_CTL_PKE_NONE);	// data 12
}

EXPORT_SYMBOL(gpio_ata_inactive);

/*!
 * Setup EDIO/IOMUX for external UART.
 *
 * @param port UART port
 * @param irq Interrupt line to allocate
 * @param handler Function to be called when the IRQ occurs
 * @param irq_flags Interrupt type flags
 * @param devname An ascii name for the claiming device
 * @param dev_id A cookie passed back to the handler function
 * @return  Returns 0 if the interrupt was successfully requested,
 *          otherwise returns an error code.
 */
int extuart_intr_setup(unsigned int port, unsigned int irq,
		       irqreturn_t(*handler) (int, void *, struct pt_regs *),
		       unsigned long irq_flags, const char *devname,
		       void *dev_id)
{
	return 0;
}

/*!
 * Get the EDIO interrupt, clear if set.
 *
 * @param port UART port
 */
void extuart_intr_clear(unsigned int port)
{
}

/*!
 * Do IOMUX configs required to put the
 * pin back in low power mode.
 *
 * @param port UART port
 * @param irq Interrupt line to free
 * @param dev_id Device identity to free
 * @return  Returns 0 if the interrupt was successfully freed,
 *          otherwise returns an error code.
 */
int extuart_intr_cleanup(unsigned int port, unsigned int irq, void *dev_id)
{
	return 0;
}

/*
 * Find the minimum clock for SDHC.
 *
 * @param clk SDHC module number.
 * @return Returns the minimum SDHC clock.
 */
unsigned int sdhc_get_min_clock(enum mxc_clocks clk)
{
	return 150000;
}

EXPORT_SYMBOL(sdhc_get_min_clock);

/*
 * Find the maximum clock for SDHC.
 * @param clk SDHC module number.
 * @return Returns the maximum SDHC clock.
 */
unsigned int sdhc_get_max_clock(enum mxc_clocks clk)
{
	return 21000000;
}

EXPORT_SYMBOL(sdhc_get_max_clock);

/*
 * Probe for the card. If present the GPIO data would be set.
 */
int sdhc_find_card(void)
{
	return 0;
}

EXPORT_SYMBOL(sdhc_find_card);

/*
 * USB Host 1
 * pins conflict with SPI1, ATA, UART3
 */
void gpio_usbh1_active(void)
{
	mxc_request_iomux(MX31_PIN_CSPI1_MOSI, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);	/* USBH1_RXDM */
	mxc_request_iomux(MX31_PIN_CSPI1_MISO, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);	/* USBH1_RXDP */
	mxc_request_iomux(MX31_PIN_CSPI1_SS0, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);	/* USBH1_TXDM */
	mxc_request_iomux(MX31_PIN_CSPI1_SS1, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);	/* USBH1_TXDP */
	mxc_request_iomux(MX31_PIN_CSPI1_SS2, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);	/* USBH1_RCV  */
	mxc_request_iomux(MX31_PIN_CSPI1_SCLK, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);	/* USBH1_OEB (_TXOE)  */
	mxc_request_iomux(MX31_PIN_CSPI1_SPI_RDY, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);	/* USBH1_FS   */

	mxc_iomux_set_pad(MX31_PIN_CSPI1_MOSI, (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));	/* USBH1_RXDM */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_MISO, (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));	/* USBH1_RXDP */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS0, (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));	/* USBH1_TXDM */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS1, (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));	/* USBH1_TXDP */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS2, (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));	/* USBH1_RCV  */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SCLK, (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));	/* USBH1_OEB (_TXOE)  */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SPI_RDY, (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));	/* USBH1_FS   */

	mxc_iomux_set_gpr(MUX_PGP_USB_SUSPEND, true);
}

EXPORT_SYMBOL(gpio_usbh1_active);

void gpio_usbh1_inactive(void)
{
	mxc_free_iomux(MX31_PIN_CSPI1_MOSI, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH1_RXDM */
	mxc_free_iomux(MX31_PIN_CSPI1_MISO, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH1_RXDP */
	mxc_free_iomux(MX31_PIN_CSPI1_SS0, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH1_TXDM */
	mxc_free_iomux(MX31_PIN_CSPI1_SS1, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH1_TXDP */
	mxc_free_iomux(MX31_PIN_CSPI1_SS2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH1_RCV  */
	mxc_free_iomux(MX31_PIN_CSPI1_SCLK, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH1_OEB (_TXOE)  */
	mxc_free_iomux(MX31_PIN_CSPI1_SPI_RDY, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH1_FS   */

	mxc_iomux_set_pad(MX31_PIN_CSPI1_MOSI, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH1_RXDM */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_MISO, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH1_RXDP */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS0, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH1_TXDM */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS1, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH1_TXDP */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS2, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));	/* USBH1_RCV  */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SCLK, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));	/* USBH1_OEB (_TXOE)  */
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SPI_RDY, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));	/* USBH1_FS   */
}

EXPORT_SYMBOL(gpio_usbh1_inactive);

/*
 * USB Host 2
 * pins conflict with UART5, PCMCIA
 */
void gpio_usbh2_active(void)
{
	mxc_request_iomux(MX31_PIN_USBH2_CLK, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBH2_DIR, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBH2_NXT, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBH2_STP, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBH2_DATA0, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBH2_DATA1, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);

	mxc_request_iomux(MX31_PIN_PC_VS2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA2 */
	mxc_request_iomux(MX31_PIN_PC_BVD1, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA3 */
	mxc_request_iomux(MX31_PIN_PC_BVD2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA4 */
	mxc_request_iomux(MX31_PIN_PC_RST, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA5 */
	mxc_request_iomux(MX31_PIN_IOIS16, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA6 */
	mxc_request_iomux(MX31_PIN_PC_RW_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA7 */

	mxc_request_iomux(MX31_PIN_NFWE_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* ??? */
	mxc_request_iomux(MX31_PIN_NFRE_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* ??? */
	mxc_request_iomux(MX31_PIN_NFALE, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* ??? */
	mxc_request_iomux(MX31_PIN_NFCLE, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* ??? */
	mxc_request_iomux(MX31_PIN_NFWP_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* ??? */
	mxc_request_iomux(MX31_PIN_NFCE_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* ??? */

#define H2_PAD_CFG (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST | PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_100K_PU)
	mxc_iomux_set_pad(MX31_PIN_USBH2_CLK, H2_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_DIR, H2_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_NXT, H2_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_STP, H2_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_DATA0, H2_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_DATA1, H2_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_SRXD6, H2_PAD_CFG);	/* USBH2_DATA2 */
	mxc_iomux_set_pad(MX31_PIN_STXD6, H2_PAD_CFG);	/* USBH2_DATA3 */
	mxc_iomux_set_pad(MX31_PIN_SFS3, H2_PAD_CFG);	/* USBH2_DATA4 */
	mxc_iomux_set_pad(MX31_PIN_SCK3, H2_PAD_CFG);	/* USBH2_DATA5 */
	mxc_iomux_set_pad(MX31_PIN_SRXD3, H2_PAD_CFG);	/* USBH2_DATA6 */
	mxc_iomux_set_pad(MX31_PIN_STXD3, H2_PAD_CFG);	/* USBH2_DATA7 */
#undef H2_PAD_CFG

	mxc_iomux_set_gpr(MUX_PGP_UH2, true);
}

EXPORT_SYMBOL(gpio_usbh2_active);

void gpio_usbh2_inactive(void)
{
	iomux_config_gpr(MUX_PGP_UH2, false);

	iomux_config_pad(MX31_PIN_USBH2_CLK,
			 (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));
	iomux_config_pad(MX31_PIN_USBH2_DIR,
			 (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));
	iomux_config_pad(MX31_PIN_USBH2_NXT,
			 (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));
	iomux_config_pad(MX31_PIN_USBH2_STP,
			 (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));
	iomux_config_pad(MX31_PIN_USBH2_DATA0,
			 (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));
	iomux_config_pad(MX31_PIN_USBH2_DATA1,
			 (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));
	iomux_config_pad(MX31_PIN_SRXD6, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH2_DATA2 */
	iomux_config_pad(MX31_PIN_STXD6, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH2_DATA3 */
	iomux_config_pad(MX31_PIN_SFS3, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH2_DATA4 */
	iomux_config_pad(MX31_PIN_SCK3, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH2_DATA5 */
	iomux_config_pad(MX31_PIN_SRXD3, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH2_DATA6 */
	iomux_config_pad(MX31_PIN_STXD3, (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST));	/* USBH2_DATA7 */

	mxc_free_iomux(MX31_PIN_USBH2_CLK, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBH2_DIR, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBH2_NXT, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBH2_STP, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBH2_DATA0, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBH2_DATA1, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);

	mxc_free_iomux(MX31_PIN_PC_VS2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA2 */
	mxc_free_iomux(MX31_PIN_PC_BVD1, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA3 */
	mxc_free_iomux(MX31_PIN_PC_BVD2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA4 */
	mxc_free_iomux(MX31_PIN_PC_RST, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA5 */
	mxc_free_iomux(MX31_PIN_IOIS16, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA6 */
	mxc_free_iomux(MX31_PIN_PC_RW_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);	/* USBH2_DATA7 */

	mxc_free_iomux(MX31_PIN_NFWE_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_NFRE_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_NFALE, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_NFCLE, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_NFWP_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_NFCE_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
}

EXPORT_SYMBOL(gpio_usbh2_inactive);

/*
 * USB OTG HS port
 */
void gpio_usbotg_hs_active(void)
{
	mxc_request_iomux(MX31_PIN_USBOTG_DATA0, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA1, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA2, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA3, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA4, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA5, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA6, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA7, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_CLK, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DIR, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_NXT, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_STP, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);

	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA0,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA1,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA2,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA3,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA4,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA5,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA6,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA7,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_CLK,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DIR,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_NXT,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_STP,
			  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
}

EXPORT_SYMBOL(gpio_usbotg_hs_active);

void gpio_usbotg_hs_inactive(void)
{
	mxc_free_iomux(MX31_PIN_USBOTG_DATA0, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA1, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA2, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA3, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA4, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA5, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA6, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA7, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_CLK, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DIR, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_NXT, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_STP, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);

	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA0,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA1,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA2,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA3,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA4,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA5,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA6,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA7,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_CLK,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DIR,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_NXT,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
	mxc_iomux_set_pad(MX31_PIN_USBOTG_STP,
			  (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
}

EXPORT_SYMBOL(gpio_usbotg_hs_inactive);

/*
 * USB OTG FS port
 */
void gpio_usbotg_fs_active(void)
{
	mxc_request_iomux(MX31_PIN_USBOTG_DATA0, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA1, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA2, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA3, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA4, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA5, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA6, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DATA7, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_CLK, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_DIR, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_NXT, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USBOTG_STP, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_USB_PWR, OUTPUTCONFIG_FUNC,
			  INPUTCONFIG_FUNC);
}

EXPORT_SYMBOL(gpio_usbotg_fs_active);

void gpio_usbotg_fs_inactive(void)
{
	mxc_free_iomux(MX31_PIN_USBOTG_DATA0, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA1, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA2, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA3, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA4, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA5, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA6, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DATA7, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_CLK, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_DIR, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_NXT, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USBOTG_STP, OUTPUTCONFIG_GPIO,
		       INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_USB_PWR, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
}

EXPORT_SYMBOL(gpio_usbotg_fs_inactive);

/*!
 * Setup GPIO for PCMCIA interface
 *
 */
void gpio_pcmcia_active(void)
{
	u16 temp;

	mxc_request_iomux(MX31_PIN_SDBA0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	mxc_request_iomux(MX31_PIN_SDBA1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);

	iomux_config_mux(MX31_PIN_LBA, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_RW, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_EB0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_EB1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_OE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);

	iomux_config_mux(MX31_PIN_IOIS16, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_BVD1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_BVD2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_CD1_B, OUTPUTCONFIG_FUNC,
			 INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_CD2_B, OUTPUTCONFIG_FUNC,
			 INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_POE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_PWRON, OUTPUTCONFIG_FUNC,
			 INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_READY, OUTPUTCONFIG_FUNC,
			 INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_RST, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_RW_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_VS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_VS2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	iomux_config_mux(MX31_PIN_PC_WAIT_B, OUTPUTCONFIG_FUNC,
			 INPUTCONFIG_FUNC);

	/* PCMCIA VPP, VCC Enable, 1 = power on */
	temp = PBC_BCTRL2_VPP_EN | PBC_BCTRL2_VCC_EN;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL2_SET);

	/* Set up Card2 Select pin for PCMCIA, 0 = PCMCIA & SD2 */
	temp = PBC_BCTRL3_CARD2_SEL;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL3_CLEAR);

	/* PCMCIA Enable, 0 = enable */
	temp = PBC_BCTRL4_PCMCIA_EN;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL4_CLEAR);
	mdelay(1);
}

EXPORT_SYMBOL(gpio_pcmcia_active);

/*!
 * Setup GPIO for pcmcia to be inactive
 */
void gpio_pcmcia_inactive(void)
{
	u16 temp;

	/* PCMCIA Enable, 0 = enable */
	temp = PBC_BCTRL4_PCMCIA_EN;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL4_SET);

	/* Set up Card2 Select pin for PCMCIA, 0 = PCMCIA & SD2 */
	temp = PBC_BCTRL3_CARD2_SEL;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL3_SET);

	/* PCMCIA VPP, VCC Enable, 1 = power on */
	temp = PBC_BCTRL2_VPP_EN | PBC_BCTRL2_VCC_EN;
	__raw_writew(temp, PBC_BASE_ADDRESS + PBC_BCTRL2_CLEAR);

	mxc_free_iomux(MX31_PIN_SDBA0, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_SDBA1, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);

	mxc_free_iomux(MX31_PIN_LBA, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_RW, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_EB0, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_EB1, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_OE, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);

	mxc_free_iomux(MX31_PIN_IOIS16, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_BVD1, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_BVD2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_CD1_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_CD2_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_POE, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_PWRON, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_READY, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_RST, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_RW_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_VS1, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_VS2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
	mxc_free_iomux(MX31_PIN_PC_WAIT_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
}

EXPORT_SYMBOL(gpio_pcmcia_inactive);
