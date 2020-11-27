/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright Motorola 2006
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ---------------------
 * 10/06/2006   Motorola  Fix read/write issues
 *
 */

 /*!
  * @file registers.c
  * @brief This file contains the implementation of the low level SPI
  * module functions.
  *
  * @ingroup SPI
  */

#include <linux/module.h>
#include <asm/io.h>

#include "registers.h"

extern unsigned long spi_ipg_clk;
extern unsigned long spi_max_bit_rate;
/*!
 * Global variable which contains the context of the SPI driver
 */
extern spi_mxc_add spi_add[CONFIG_SPI_NB_MAX];
extern bool spi_present[CONFIG_SPI_NB_MAX];
extern struct mxc_spi_unique_def *mxc_spi_unique_def;

inline unsigned long spi_get_base_address(int spi_id)
{
	if (spi_id == 0) {
		return IO_ADDRESS(CSPI1_BASE_ADDR);
	}

	if (spi_id == 1) {
		return IO_ADDRESS(CSPI2_BASE_ADDR);
	}
#ifdef CONFIG_MXC_SPI_SELECT3
	if (spi_id == SPI3) {
		return IO_ADDRESS(CSPI3_BASE_ADDR);
	}
#endif				/* CONFIG_MXC_SPI_SELECT3 */

	return 0;
}

/*!
 * This function configure the SPBA access module.
 *
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_init_spba(void)
{
	return 0;
}

/*!
 * This function releases configure of SPBA access module.
 *
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_release_spba(void)
{
	return 0;
}

/*!
 * This function clears the interrupt source(s).
 *
 * @param        mod         the module number
 * @param        irqs        the irq(s) to clear (can be a combination)
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_clear_interrupt(module_nb_t mod, spi_irqs_t irqs)
{
	if (irqs & ~((1 << mxc_spi_unique_def->interrupt_irq_bit_shift) - 1)) {
		return -1;
	}

	MODIFY_REGISTER_32(0, irqs, _reg_CSPI(spi_add[mod].stat_address));

	return 0;
}

/*!
 * This function disables the SPI module.
 *
 * @param        mod        the module number
 */
void spi_disable(module_nb_t mod)
{
	MODIFY_REGISTER_32(1 << STATE_ENABLE_DISABLE_SHIFT, 0,
			   _reg_CSPI(spi_add[mod].ctrl_address));
}

/*!
 * This function disables interrupt(s)
 *
 * @param        mod         the module number
 * @param        irqs        the irq(s) to reset (can be a combination)
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_disable_interrupt(module_nb_t mod, spi_irqs_t irqs)
{
	if (irqs & ~((1 << mxc_spi_unique_def->interrupt_irq_bit_shift) - 1)) {
		return -1;
	}

	MODIFY_REGISTER_32(irqs, 0, _reg_CSPI(spi_add[mod].int_address));

	return 0;
}

/*!
 * This function enables the SPI module.
 *
 * @param        mod        the module number
 */
void spi_enable(module_nb_t mod)
{
	MODIFY_REGISTER_32(0, 1 << STATE_ENABLE_DISABLE_SHIFT,
			   _reg_CSPI(spi_add[mod].ctrl_address));
}

/*!
 * This function enables interrupt(s)
 *
 * @param        mod         the module number
 * @param        irqs        the irq(s) to set (can be a combination)
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_enable_interrupt(module_nb_t mod, spi_irqs_t irqs)
{
	if (irqs & ~((1 << mxc_spi_unique_def->interrupt_irq_bit_shift) - 1)) {
		return -1;
	}

	MODIFY_REGISTER_32(0, irqs, _reg_CSPI(spi_add[mod].int_address));

	return 0;
}

/*!
 * This function gets the current baud rate for the SPI module.
 *
 * @return        This function returns the current baud rate.
 */
unsigned long spi_get_baudrate(module_nb_t mod)
{
	return (spi_ipg_clk >>
		(MIN_DATA_RATE_SHIFT +
		 ((_reg_CSPI(spi_add[mod].ctrl_address) >>
		   DATA_RATE_SHIFT) & DATA_RATE_MASK)));
}

/*!
 * This function returns the SPI module interrupt status.
 *
 * @return        This function returns the interrupt status.
 */
spi_irqs_t spi_get_status(module_nb_t mod)
{
	return ((spi_irqs_t) (_reg_CSPI(spi_add[mod].stat_address) &
			      ((1 << mxc_spi_unique_def->
				interrupt_irq_bit_shift) - 1)));
}

/*!
 * This function returns the transfer length for the specified module.
 *
 * @param        mod        the module number
 * @return       This function returns the transfer length.
 */
unsigned int spi_get_transfer_length(module_nb_t mod)
{
	unsigned int shift, mask;

	shift = mxc_spi_unique_def->bit_count_shift;
	mask = mxc_spi_unique_def->bit_count_mask;

	return (unsigned int)(1 + ((_reg_CSPI(spi_add[mod].ctrl_address) &
				    (mask << shift)) >> shift));
}

/*!
 * This function gets the received data.
 *
 * @param        mod        the module number
 * @param        ptr        the pointer on the Rx buffer
 * @param        size       empty size of the buffer
 * @return       This function returns the number of bytes received.
 */
unsigned int spi_get_rx_data(module_nb_t mod, unsigned char *ptr, int size)
{
	unsigned int length, remainder;
	unsigned long base_addr;
	unsigned char *p = NULL;
	unsigned int val, total;

	total = 0;
	p = ptr;
	remainder = (size % 4);
	length = size - remainder;

	base_addr = spi_get_base_address(mod);

#ifdef CONFIG_MOT_WFN431
        /* spi_get_rx_data needs to check if data is ready after XCH bit is 0 */
         
        if(spi_rx_fifo_data_ready(mod) == 0)
                return 0;
#endif /* CONFIG_MOT_WFN431 */

	/* Get into this loop only if SPI received 4 or more bytes */
	while (length > 0) {
		val = 0;

		/* Read top of RX FIFO to get next value */
		val = __raw_readl(base_addr + OFFSET_CSPI_RXDATAREG);

		/* Copy bytes to our buffer */
		*(p + 0) = (unsigned char)((val & 0xff000000) >> 24);
		*(p + 1) = (unsigned char)((val & 0x00ff0000) >> 16);
		*(p + 2) = (unsigned char)((val & 0x0000ff00) >> 8);
		*(p + 3) = (unsigned char)((val & 0x000000ff) >> 0);

		length -= 4;
		total += 4;
		p += 4;

		if (spi_rx_fifo_data_ready(mod) == 0) {
			break;
		}
	}

	/* copy now remainder bytes to our buffer */
	if (remainder > 0) {
		int shift = 24;
		int i = 0;

#ifdef CONFIG_MOT_WFN431
                /* Need to wait until data is ready */ 
                if(spi_rx_fifo_data_ready(mod) == 0)
                        return total;
#endif /* CONFIG_MOT_WFN431 */

		val = __raw_readl(base_addr + OFFSET_CSPI_RXDATAREG);
		while (i < remainder) {
			*(p + i) =
			    (unsigned int)((val & (0xff << shift)) >> shift);
			shift -= 8;
			i++;
		}

		total += remainder;
	}

	return total;
}

/*!
 * This function initiates data exchange.
 *
 * @param        mod        the module number
 */
void spi_init_exchange(module_nb_t mod)
{
	_reg_CSPI(spi_add[mod].ctrl_address) |= (1 << EXCHANGE_BIT_SHIFT);
}

/*!
 * This function loads the transmit fifo.
 *
 * @param        mod        the module number
 * @param        buffer     the data to put in the TxFIFO
 * @param        size       this is the bytes count value
 */
void spi_put_tx_data(module_nb_t mod, unsigned char *buffer, int size)
{
	unsigned int length, remainder;
	unsigned long base_addr;
	unsigned char *p = NULL;
	unsigned int val;

	p = buffer;
	remainder = (size % 4);
	length = size - remainder;

	base_addr = spi_get_base_address(mod);

	while (length > 0) {
		val = 0;
		val =
		    (*(p + 0) << 24) | (*(p + 1) << 16) | (*(p + 2) << 8) | *(p
									      +
									      3);
		__raw_writel(val, base_addr + OFFSET_CSPI_TXDATAREG);
		length -= 4;
		p += 4;
	}

	if (remainder > 0) {
		int shift = 24;
		int i = 0;

		val = 0;
		while (i < remainder) {
			val |= (unsigned int)(*(p + i) << shift);
			shift -= 8;
			i++;
		}

		__raw_writel(val, base_addr + OFFSET_CSPI_TXDATAREG);
	}

	spi_init_exchange(mod);
}

/*!
 * This function allows to configure the module in loopback mode.
 *
 * @param        mod        the module number
 * @param        enable     the boolean to enable or disable the loopback mode
 */
void spi_loopback_mode(module_nb_t mod, bool enable)
{
	if (enable == true) {
		_reg_CSPI(spi_add[mod].test_address) |= 0x4000;
	} else {
		_reg_CSPI(spi_add[mod].test_address) &= ~(0x4000);
	}
}

/*!
 * This function selects the slave select.
 *
 * @param        mod        the module number
 * @param        nb         the slave select to assert
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_select_ss(module_nb_t mod, unsigned char nb)
{
	if (nb > 3) {
		return -1;
	}

	MODIFY_REGISTER_32(0, (nb << mxc_spi_unique_def->chip_select_shift),
			   _reg_CSPI(spi_add[mod].ctrl_address));

	return 0;
}

/*!
 * This function sets the baud rate for the SPI module.
 *
 * @param        mod        the module number
 * @param        baud       the baud rate
 * @return       This function returns the baud rate set.
 */
unsigned long spi_set_baudrate(module_nb_t mod, unsigned long baud)
{
	unsigned int divisor, shift;

	if (baud <= 0) {
		return -1;
	}

	/* Calculate required divisor (rounded) */
	divisor = (spi_ipg_clk + baud / 2) / baud;

	shift = 0;

	while (divisor > (1 << (MIN_DATA_RATE_SHIFT + shift + 1)) &&
	       shift < DATA_RATE_MASK) {
		shift++;
	}

	MODIFY_REGISTER_32((DATA_RATE_MASK << DATA_RATE_SHIFT),
			   (shift << DATA_RATE_SHIFT),
			   _reg_CSPI(spi_add[mod].ctrl_address));

	return (spi_ipg_clk >> (MIN_DATA_RATE_SHIFT + shift));
}

/*!
 * This function sets the mode for the SPI module.
 *
 * @param        mod        the module number
 * @param        mode       the operation mode
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_mode(module_nb_t mod, spi_mode_t mode)
{
	switch (mode) {
	case SPI_SLAVE:
		/* Configure as a slave */
		MODIFY_REGISTER_32(1 << MODE_MASTER_SLAVE_SHIFT, 0,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;

	case SPI_MASTER:
		/* Configure as a master */
		MODIFY_REGISTER_32(0, 1 << MODE_MASTER_SLAVE_SHIFT,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;
	default:
		return -1;
	}
	return 0;
}

/*!
 * This function sets the module phase.
 *
 * @param        mod        the module number
 * @param        phase      the phase
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_phase(module_nb_t mod, spi_phase_t phase)
{
	switch (phase) {
	case SPI_PHASE_0:
		/* Phase 0 mode */
		MODIFY_REGISTER_32(1 << PHASE_SHIFT, 0,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;

	case SPI_PHASE_1:
		/* Phase 1 mode */
		MODIFY_REGISTER_32(0, 1 << PHASE_SHIFT,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;
	default:
		return -1;
	}
	return 0;
}

/*!
 * This function sets the module polarity.
 *
 * @param        mod        the module number
 * @param        polarity   the polarity
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_polarity(module_nb_t mod, spi_polarity_t polarity)
{
	switch (polarity) {
	case SPI_POLARITY_ACTIVE_HIGH:
		/* Polarity active at high level */
		MODIFY_REGISTER_32(1 << POLARITY_SHIFT, 0,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;

	case SPI_POLARITY_ACTIVE_LOW:
		/* Polarity active at low level */
		MODIFY_REGISTER_32(0, 1 << POLARITY_SHIFT,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;
	default:
		return -1;
	}
	return 0;
}

/*!
 * This function sets the rdy input waveform.
 *
 * @param        mod        the module number
 * @param        control    the rdy input waveform
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_spi_rdy_input_waveform(module_nb_t mod, spi_rdy_control_t control)
{
	unsigned int temp;

	temp = mxc_spi_unique_def->rdy_control_shift;

	switch (control) {
	case SPI_RDY_IGNORE:
		/* Configured to ignore Rdy signal */
		MODIFY_REGISTER_32(3 << temp, 0,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;

	case SPI_RDY_FALLING:
		/* Configure Rdy signal on falling edge */
		MODIFY_REGISTER_32(2 << temp, 1 << temp,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;

	case SPI_RDY_RISING:
		/* Configure Rdy signal on rising edge */
		MODIFY_REGISTER_32(1 << temp, 2 << temp,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;
	default:
		return -1;
	}
	return 0;
}

/*!
 * This function sets the SS polarity.
 *
 * @param        mod        the module number
 * @param        polarity   the SS polarity
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_ss_polarity(module_nb_t mod, spi_ss_polarity_t polarity)
{
	switch (polarity) {
	case SPI_SS_ACTIVE_LOW:
		/* Slave Select active on low level */
		MODIFY_REGISTER_32(1 << SS_POLARITY_SHIFT, 0,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;

	case SPI_SS_ACTIVE_HIGH:
		/* Slave Select active on high level */
		MODIFY_REGISTER_32(0, 1 << SS_POLARITY_SHIFT,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;
	default:
		return -1;
	}
	return 0;
}

/*!
 * This function sets the Slave Select waveform.
 *
 * @param        mod        the module number
 * @param        waveform   the SS waveform
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_ss_waveform(module_nb_t mod, spi_ss_waveform_t waveform)
{
	switch (waveform) {
	case SPI_LOW_BTN_BURST:
		/* Slave Select doesn't change between bursts */
		MODIFY_REGISTER_32(1 << SS_WAVEFORM_SHIFT, 0,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;

	case SPI_PULSE_BTN_BURST:
		/* Slave Select not asserted between bursts */
		MODIFY_REGISTER_32(0, 1 << SS_WAVEFORM_SHIFT,
				   _reg_CSPI(spi_add[mod].ctrl_address));
		break;
	default:
		return -1;
	}
	return 0;
}

/*!
 * This function sets the length of the transfer for the SPI module.
 *
 * @param        mod               the module number
 * @param        transferlength    the length of the transfer in bits
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_set_transfer_length(module_nb_t mod, unsigned int transferlength)
{
	unsigned int shift, mask;

	shift = mxc_spi_unique_def->bit_count_shift;
	mask = mxc_spi_unique_def->bit_count_mask;

	if (((transferlength - 1) > mask) || (transferlength == 0)) {
		return -1;
	}

	MODIFY_REGISTER_32(mask << shift,
			   ((transferlength - 1) & mask) << shift,
			   _reg_CSPI(spi_add[mod].ctrl_address));
	return 0;
}

/*!
 * This function tests the Rx FIFO data ready flag.
 *
 * @param        mod        the module number
 * @return       This function returns the flag state (1 or 0).
 */
int spi_rx_fifo_data_ready(module_nb_t mod)
{
	int result;
	result = _reg_CSPI(spi_add[mod].stat_address) &
	    SPI_STATUS_RX_FIFO_DATA_READY;
	return result;
}

/*!
 * This function tests the Transfer Complete flag.
 *
 * @param        mod         the module number
 * @return       This function returns the flag state (1 or 0).
 */
int spi_transfer_complete(module_nb_t mod)
{
	unsigned int temp;

	temp = mxc_spi_unique_def->spi_status_transfer_complete;

	return (_reg_CSPI(spi_add[mod].stat_address) & temp);
}

/*!
 * This function tests the Tx FIFO empty flag.
 *
 * @param        mod        the module number
 * @return       This function returns the flag state (1 or 0).
 */
int spi_tx_fifo_empty(module_nb_t mod)
{
	int result;
	result = _reg_CSPI(spi_add[mod].stat_address) &
	    SPI_STATUS_TX_FIFO_EMPTY;
	return result;
}

/*!
 * This function tests the Tx FIFO half empty flag.
 *
 * @param        mod        the module number
 * @return       This function returns the flag state (1 or 0).
 */
int spi_tx_fifo_half_empty(module_nb_t mod)
{
	int result;
	result = _reg_CSPI(spi_add[mod].stat_address) &
	    SPI_STATUS_TX_FIFO_HALF_EMPTY;
	return result;
}

EXPORT_SYMBOL(spi_set_transfer_length);
