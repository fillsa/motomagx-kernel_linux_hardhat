/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

 /*!
  * @file spi-internal.h
  * @brief This header file contains SPI driver core functions prototypes.
  *
  * @ingroup SPI
  */

#ifndef __SPI_INTERNAL_H__
#define __SPI_INTERNAL_H__

#include <asm/hardware.h>
#include "spi.h"

//#define DBG_SPI 1

#ifdef DBG_SPI
#define DBG_PRINTK(fmt, args...) printk(fmt,##args)
#else
#define DBG_PRINTK(fmt, args...)
#endif

#define MAX_TX_DELAY            1000000
#define TRANSFER_8_BITS  8

#if defined(CONFIG_ARCH_MX3)
#define CONFIG_SPI_NB_MAX      0x03
#else
#define CONFIG_SPI_NB_MAX      0x02
#endif

/*!
 * This structure is a way for the low level driver to define their own
 * \b spi_port adresses structure. This structure includes elements that are
 * specifically required by this low-level driver.
 */
typedef struct {
	/*!
	 * The SPI Base Register Address.
	 */
	unsigned long base_address;
	/*!
	 * The SPI RX Register Address.
	 */
	unsigned long rx_address;
	/*!
	 * The SPI TX Register Address.
	 */
	unsigned long tx_address;
	/*!
	 * The SPI Control Register Address.
	 */
	unsigned long ctrl_address;
	/*!
	 * The SPI Interrupt Register Address.
	 */
	unsigned long int_address;
	/*!
	 * The SPI STAT Register Address.
	 */
	unsigned long stat_address;
	/*!
	 * The SPI PERIOD Register Address.
	 */
	unsigned long period_address;
	/*!
	 * The SPI TEST Register Address.
	 */
	unsigned long test_address;
	/*!
	 * The SPIRESET Register Address.
	 */
	unsigned long reset_address;
} spi_mxc_add;

#endif				/* __SPI_INTERNAL_H__ */
