/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2007 Motorola, Inc. 
 * 
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * 
 * DATE          AUTHOR         COMMMENT
 * ----          ------         --------
 * 03/12/2007    Motorola       Added a UART suspend flag.
 *
 */

#ifndef SERIAL_MXC_H

#define SERIAL_MXC_H

#include <linux/serial_core.h>
#include <asm/arch/clock.h>

/*!
 * @defgroup UART Universal Asynchronous Receiver Transmitter (UART) Driver
 */

/*!
 * @file mxc_uart.h
 *
 * @brief This file contains the UART configuration structure definition.
 *
 *
 * @ingroup UART
 */

/*!
 * This structure is used to store the the physical and virtual
 * addresses of the UART DMA receive buffer.
 */
typedef struct {
	/*!
	 * DMA Receive buffer virtual address
	 */
	int *rx_buf;
	/*!
	 * DMA Receive buffer physical address
	 */
	int *rx_handle;
} mxc_uart_rxdmamap;

/*!
 * This structure is a way for the low level driver to define their own
 * \b uart_port structure. This structure includes the core \b uart_port
 * structure that is provided by Linux as an element and has other
 * elements that are specifically required by this low-level driver.
 */
typedef struct {
	/*!
	 * The port structure holds all the information about the UART
	 * port like base address, and so on.
	 */
	struct uart_port port;
	/*!
	 * Flag to determine if the interrupts are muxed.
	 */
	int ints_muxed;
	/*!
	 * Array that holds the receive and master interrupt numbers
	 * when the interrupts are not muxed.
	 */
	int irqs[2];
	/*!
	 * Flag to determine the DTE/DCE mode.
	 */
	int mode;
	/*!
	 * Flag to hold the IR mode of the port.
	 */
	int ir_mode;
	/*!
	 * Flag to enable/disable the UART port.
	 */
	int enabled;
	/*!
	 * Flag to indicate if we wish to use hardware-driven hardware
	 * flow control.
	 */
	int hardware_flow;
	/*!
	 * Holds the threshold value at which the CTS line is deasserted in
	 * case we use hardware-driven hardware flow control.
	 */
	unsigned int cts_threshold;
	/*!
	 * Flag to enable/disable DMA data transfer.
	 */
	int dma_enabled;
	/*!
	 * Holds the DMA receive buffer size.
	 */
	int dma_rxbuf_size;
	/*!
	 * DMA Receive buffers information
	 */
	mxc_uart_rxdmamap *rx_dmamap;
	/*!
	 * DMA RX buffer id
	 */
	int dma_rxbuf_id;
	/*!
	 * DMA Transmit buffer virtual address
	 */
	char *tx_buf;
	/*!
	 * DMA Transmit buffer physical address
	 */
	dma_addr_t tx_handle;
	/*!
	 * Holds the RxFIFO threshold value.
	 */
	unsigned int rx_threshold;
	/*!
	 * Holds the TxFIFO threshold value.
	 */
	unsigned int tx_threshold;
	/*!
	 * Information whether this is a shared UART
	 */
	unsigned int shared;
	/*!
	 * Clock id from clock.h
	 */
	enum mxc_clocks clock_id;
	/*!
	 * Flag for UART suspend state
	 */
	int uart_suspended; 
} uart_mxc_port;

#endif				/* SERIAL_MXC_H */
