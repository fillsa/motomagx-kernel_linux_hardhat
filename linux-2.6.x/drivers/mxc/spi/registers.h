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
  * @file ../spi/registers.h
  * @brief This header file contains SPI driver low level definition to access module registers.
  *
  * @ingroup SPI
  */

#ifndef __SPI_REGISTERS_H__
#define __SPI_REGISTERS_H__

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include "spi-internal.h"

#define OFFSET_CSPI_RXDATAREG   0x00
#define OFFSET_CSPI_TXDATAREG   0x04
#define OFFSET_CSPI_CONTROLREG  0x08
#define OFFSET_CSPI_INTREG      0x0C

#define OFFSET_CSPI_DMAREG      0x10
#define OFFSET_CSPI_STATREG     0x14
#define OFFSET_CSPI_PERIODREG   0x18
#define OFFSET_CSPI_TESTREG     0x1C
#define OFFSET_CSPI_RESETREG 	0x0

#define MXC_CSPICONREG_XCH			0x4

#define _reg_CSPI(a) (*((volatile unsigned long *) \
                     (IO_ADDRESS(a))))

#define SPBA_CPU_SPI            0x07

#define _reg_SPBA(a) (*((volatile unsigned long *) \
                     (IO_ADDRESS(a))))

#define MAX_BIT_COUNT           256

#define MODIFY_REGISTER_32(a,b,c) c = ( ( c & (~(a)) ) | b )

struct mxc_spi_unique_def {
	unsigned int interrupt_irq_bit_shift;
	unsigned int bit_count_shift;
	unsigned int rdy_control_shift;
	unsigned int chip_select_shift;
	unsigned int bit_count_mask;
	unsigned int spi_status_transfer_complete;
	unsigned int spi_status_bit_count_overflow;
};

#define MIN_DATA_RATE_SHIFT         2
#define DATA_RATE_SHIFT            16
#define MODE_MASTER_SLAVE_SHIFT     1
#define EXCHANGE_BIT_SHIFT          2
#define SMC_CONTROL_SHIFT           3
#define STATE_ENABLE_DISABLE_SHIFT  0
#define SS_POLARITY_SHIFT           7
#define SS_WAVEFORM_SHIFT           6
#define PHASE_SHIFT                 5
#define POLARITY_SHIFT              4

#define DATA_RATE_MASK            0x7

#define INTERRUPT_IRQ_BIT_SHIFT     9
#define INTERRUPT_IRQ_BIT_SHIFT_0_7 8

#define BIT_COUNT_SHIFT             8
#define RDY_CONTROL_SHIFT          20
#define CHIP_SELECT_SHIFT          24
#define BIT_COUNT_MASK          0x01f

#define BIT_COUNT_SHIFT_0_5        20
#define RDY_CONTROL_SHIFT_0_5       8
#define CHIP_SELECT_SHIFT_0_5      12
#define BIT_COUNT_MASK_0_5     0x0fff

/*!
 * This enumeration describes the bit masks to access the interrupt and the status registers.
 *
 */
typedef enum {
	/*!
	 * the Transfer Complete bit
	 */
	SPI_STATUS_TRANSFER_COMPLETE = (1 << 8),
	SPI_STATUS_TRANSFER_COMPLETE_0_7 = (1 << 7),
	/*!
	 * the Bit Count overflow bit
	 */
	SPI_STATUS_BIT_COUNT_OVERFLOW = (1 << 7),
	SPI_STATUS_BIT_COUNT_OVERFLOW_0_7 = 0,
	/*!
	 * the Rx Fifo overflow bit
	 */
	SPI_STATUS_RX_FIFO_OVERFLOW = (1 << 6),
	/*!
	 * the Rx Fifo full bit
	 */
	SPI_STATUS_RX_FIFO_FULL = (1 << 5),
	/*!
	 * the Rx Fifo half full bit
	 */
	SPI_STATUS_RX_FIFO_HALF_FULL = (1 << 4),
	/*!
	 * the Rx Fifo data ready bit
	 */
	SPI_STATUS_RX_FIFO_DATA_READY = (1 << 3),
	/*!
	 * the Tx Fifo full bit
	 */
	SPI_STATUS_TX_FIFO_FULL = (1 << 2),
	/*!
	 * the Tx Fifo half empty bit
	 */
	SPI_STATUS_TX_FIFO_HALF_EMPTY = (1 << 1),
	/*!
	 * the Tx Fifo empty bit
	 */
	SPI_STATUS_TX_FIFO_EMPTY = (1 << 0)
} spi_irqs_t;

/*!
 * This enumeration describes the SPI mode.
 *
 */
typedef enum {
	/*!
	 * the slave mode
	 */
	SPI_SLAVE = 0,
	/*!
	 * the master mode
	 */
	SPI_MASTER = 1
} spi_mode_t;

/*!
 * This enumeration describes the SPI phase.
 *
 */
typedef enum {
	/*!
	 * phase = 0
	 */
	SPI_PHASE_0,
	/*!
	 * phase = 1
	 */
	SPI_PHASE_1
} spi_phase_t;

/*!
 * This enumeration describes the SPI polarity.
 *
 */
typedef enum {
	/*!
	 * polarity is active when low.
	 */
	SPI_POLARITY_ACTIVE_LOW,
	/*!
	 * polarity is active when high.
	 */
	SPI_POLARITY_ACTIVE_HIGH
} spi_polarity_t;

/*!
 * This enumeration describes the SPI Rdy signal.
 *
 */
typedef enum {
	/*!
	 * SPI Rdy signal is ignored.
	 */
	SPI_RDY_IGNORE,
	/*!
	 * SPI Rdy signal on falling edge.
	 */
	SPI_RDY_FALLING,
	/*!
	 * SPI Rdy signal on rising edge.
	 */
	SPI_RDY_RISING
} spi_rdy_control_t;

/*!
 * This enumeration describes the SPI Slave Select polarity.
 *
 */
typedef enum {
	/*!
	 * SPI Slave Select active when low.
	 */
	SPI_SS_ACTIVE_LOW,
	/*!
	 * SPI Slave Select active when high.
	 */
	SPI_SS_ACTIVE_HIGH
} spi_ss_polarity_t;

/*!
 * This enumeration describes Slave Select waveform.
 *
 */
typedef enum {
	/*!
	 * Slave Select keep asserted between bursts.
	 */
	SPI_LOW_BTN_BURST,
	/*!
	 * Slave Select returns high between bursts.
	 */
	SPI_PULSE_BTN_BURST
} spi_ss_waveform_t;

/*!
 * This function configure the SPBA access module.
 *
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_init_spba(void);

/*!
 * This function releases configure of SPBA access module.
 *
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_release_spba(void);

/*!
 * This function clears the interrupt source(s).
 *
 * @param        mod         the module number
 * @param        irqs        the irq(s) to clear (can be a combination)
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_clear_interrupt(module_nb_t mod, spi_irqs_t irqs);

/*!
 * This function disables the SPI module.
 *
 * @param        mod        the module number
 */
void spi_disable(module_nb_t mod);

/*!
 * This function disables interrupt(s)
 *
 * @param        mod         the module number
 * @param        irqs        the irq(s) to reset (can be a combination)
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_disable_interrupt(module_nb_t mod, spi_irqs_t irqs);

/*!
 * This function enables the SPI module.
 *
 * @param        mod        the module number
 */
void spi_enable(module_nb_t mod);

/*!
 * This function enables interrupt(s)
 *
 * @param        mod         the module number
 * @param        irqs        the irq(s) to set (can be a combination)
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_enable_interrupt(module_nb_t mod, spi_irqs_t irqs);

/*!
 * This function gets the current baud rate for the SPI module.
 *
 * @return        This function returns the current baud rate.
 */
unsigned long spi_get_baudrate(module_nb_t mod);

/*!
 * This function returns the SPI module interrupt status.
 *
 * @return        This function returns the interrupt status.
 */
spi_irqs_t spi_get_status(module_nb_t mod);

/*!
 * This function returns the transfer length for the specified module.
 *
 * @param        mod        the module number
 * @return       This function returns the transfer length.
 */
unsigned int spi_get_transfer_length(module_nb_t mod);

/*!
 * This function gets the received data.
 *
 * @param        mod        the module number
 * @param        ptr        the pointer on the Rx buffer
 * @param        size       empty size of the buffer
 * @return       This function returns the number of bytes received.
 */
unsigned int spi_get_rx_data(module_nb_t mod, unsigned char *ptr, int size);

/*!
 * This function loads the transmit fifo.
 *
 * @param        mod        the module number
 * @param        buffer     the data to put in the TxFIFO
 * @param        size       this is the bit count value
 */
void spi_put_tx_data(module_nb_t mod, unsigned char *buffer, int size);

/*!
 * This function initiates data exchange.
 *
 * @param        mod        the module number
 */
void spi_init_exchange(module_nb_t mod);

/*!
 * This function allows to configure the module in loopback mode.
 *
 * @param        mod        the module number
 * @param        enable     the boolean to enable or disable the loopback mode
 */
void spi_loopback_mode(module_nb_t mod, bool enable);

/*!
 * This function selects the slave select.
 *
 * @param        mod        the module number
 * @param        nb         the slave select to assert
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_select_ss(module_nb_t mod, unsigned char nb);

/*!
 * This function sets the baud rate for the SPI module.
 *
 * @param        mod        the module number
 * @param        baud       the baud rate
 * @return       This function returns the baud rate set.
 */
unsigned long spi_set_baudrate(module_nb_t mod, unsigned long baud);

/*!
 * This function sets the mode for the SPI module.
 *
 * @param        mod        the module number
 * @param        mode       the operation mode
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_mode(module_nb_t mod, spi_mode_t mode);

/*!
 * This function sets the module phase.
 *
 * @param        mod        the module number
 * @param        phase      the phase
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_phase(module_nb_t mod, spi_phase_t phase);

/*!
 * This function sets the module polarity.
 *
 * @param        mod        the module number
 * @param        polarity   the polarity
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_polarity(module_nb_t mod, spi_polarity_t polarity);

/*!
 * This function sets the rdy input waveform.
 *
 * @param        mod        the module number
 * @param        control    the rdy input waveform
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_spi_rdy_input_waveform(module_nb_t mod, spi_rdy_control_t control);

/*!
 * This function sets the SS polarity.
 *
 * @param        mod        the module number
 * @param        polarity   the SS polarity
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_ss_polarity(module_nb_t mod, spi_ss_polarity_t polarity);

/*!
 * This function sets the Slave Select waveform.
 *
 * @param        mod        the module number
 * @param        waveform   the SS waveform
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_ss_waveform(module_nb_t mod, spi_ss_waveform_t waveform);

/*!
 * This function sets the length of the transfer for the SPI module.
 *
 * @param        mod               the module number
 * @param        transferlength    the length of the transfer in bits
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_transfer_length(module_nb_t mod, unsigned int transferlength);

/*!
 * This function tests the Rx FIFO data ready flag.
 *
 * @param        mod        the module number
 * @return       This function returns the flag state (1 or 0).
 */
int spi_rx_fifo_data_ready(module_nb_t mod);

/*!
 * This function tests the Transfer Complete flag.
 *
 * @param        mod         the module number
 * @return       This function returns the flag state (1 or 0).
 */
int spi_transfer_complete(module_nb_t mod);

/*!
 * This function tests the Tx FIFO empty flag.
 *
 * @param        mod        the module number
 * @return       This function returns the flag state (1 or 0).
 */
int spi_tx_fifo_empty(module_nb_t mod);

/*!
 * This function tests the Tx FIFO half empty flag.
 *
 * @param        mod        the module number
 * @return       This function returns the flag state (1 or 0).
 */
int spi_tx_fifo_half_empty(module_nb_t mod);

#endif				//__SPI_REGISTERS_H__
