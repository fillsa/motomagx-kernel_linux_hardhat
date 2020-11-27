/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2006 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-licensisr_locke.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  -----------------
 * 10/06/2006   Motorola  Added SPI support
 * 10/27/2006   Motorola  Added support for generic ArgonLV-based phones.
 *
 */

/*!
 * @file spi-core.c
 * @brief This file contains the implementation of the SPI driver main services
 *
 *
 * @ingroup SPI
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>


#if defined(CONFIG_MOT_FEAT_GPIO_API_MC13783)
#include <asm/mot-gpio.h>
#endif /* CONFIG_MOT_FEAT_GPIO_API_MC13783 */

#include "spi-internal.h"
#include "registers.h"

/*!
 * This structure is a way for the low level driver to define their own
 * \b spi_port structure. This structure includes elements that are
 * specifically required by this low-level driver.
 */
typedef struct {
	/*!
	 * The SPI config.
	 */
	spi_config *hw_config;
	/*!
	 * Number of received bytes.
	 */
	int bytes_received;
	/*!
	 * The number of bytes to send
	 */
	int bytes_to_send;
	/*!
	 * The reception buffer
	 */
	char *rx_buffer;

	/*!
	 * The pointer on the current SPI config
	 */
	void *current_config_handle;

	/*!
	 *  synchronous interface Mutex
	 */
	struct semaphore sync_interface;
	/*!
	 * Interrupt number
	 */
	unsigned long int_cspi;
} spi_mxc_port;

/*!
 * spin_lock to control access to SPI tx/rx FIFOs.
 */
spinlock_t mxc_spi_lock = SPIN_LOCK_UNLOCKED;

struct mxc_spi_unique_def *mxc_spi_unique_def;
unsigned long spi_ipg_clk;
unsigned long spi_max_bit_rate;

/*!
 * Global variable which contains the context of the SPI driver
 */
spi_mxc_port spi_port[CONFIG_SPI_NB_MAX];

/*!
 * Global variable which contains the adresses of the SPI driver
 */
spi_mxc_add spi_add[CONFIG_SPI_NB_MAX];

/*!
 * Global variable which indicates which SPI is used
 */
bool spi_present[CONFIG_SPI_NB_MAX];

extern void gpio_spi_active(int cspi_mod);
extern void gpio_spi_inactive(int cspi_mod);
extern unsigned long spi_max_bit_rate;

static struct mxc_spi_unique_def spi_ver_0_7 = {
	.interrupt_irq_bit_shift = INTERRUPT_IRQ_BIT_SHIFT_0_7,
	.bit_count_shift = BIT_COUNT_SHIFT_0_5,
	.rdy_control_shift = RDY_CONTROL_SHIFT_0_5,
	.chip_select_shift = CHIP_SELECT_SHIFT_0_5,
	.bit_count_mask = BIT_COUNT_MASK_0_5,
	.spi_status_transfer_complete = SPI_STATUS_TRANSFER_COMPLETE_0_7,
	.spi_status_bit_count_overflow = SPI_STATUS_BIT_COUNT_OVERFLOW_0_7,
};

static struct mxc_spi_unique_def spi_ver_0_5 = {
	.interrupt_irq_bit_shift = INTERRUPT_IRQ_BIT_SHIFT,
	.bit_count_shift = BIT_COUNT_SHIFT_0_5,
	.rdy_control_shift = RDY_CONTROL_SHIFT_0_5,
	.chip_select_shift = CHIP_SELECT_SHIFT_0_5,
	.bit_count_mask = BIT_COUNT_MASK_0_5,
	.spi_status_transfer_complete = SPI_STATUS_TRANSFER_COMPLETE,
	.spi_status_bit_count_overflow = SPI_STATUS_BIT_COUNT_OVERFLOW,
};

static struct mxc_spi_unique_def spi_ver_0_4 = {
	.interrupt_irq_bit_shift = INTERRUPT_IRQ_BIT_SHIFT,
	.bit_count_shift = BIT_COUNT_SHIFT,
	.rdy_control_shift = RDY_CONTROL_SHIFT,
	.chip_select_shift = CHIP_SELECT_SHIFT,
	.bit_count_mask = BIT_COUNT_MASK,
	.spi_status_transfer_complete = SPI_STATUS_TRANSFER_COMPLETE,
	.spi_status_bit_count_overflow = SPI_STATUS_BIT_COUNT_OVERFLOW,
};

/*!
 * This function is called when an interrupt occurs on the SPI modules.
 * It is the interrupt handler for the SPI modules.
 *
 * @param        irq        the irq number
 * @param        dev_id     the pointer on the device
 * @param        regs       the interrupt parameters
 *
 * @return       The function returns IRQ_RETVAL(1) when handled.
 */
static irqreturn_t spi_isr_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	int handled = 0;
	int nb_bytes = 0;
	module_nb_t module = 0;
	spi_mxc_port *port = NULL;
	unsigned int buf_size_empty = 0;

	port = (spi_mxc_port *) (dev_id);

	module = port->hw_config->module_number;
	buf_size_empty = port->bytes_to_send - port->bytes_received;

	DBG_PRINTK("spi%d : spi_isr_handler : rx_fifo_data_ready \n",
		   module + 1);

	if (buf_size_empty > 0) {
		nb_bytes = spi_get_rx_data(module, port->rx_buffer,
					   buf_size_empty);
	}
	spi_clear_interrupt(module, SPI_STATUS_RX_FIFO_DATA_READY);
	port->rx_buffer += nb_bytes;
	port->bytes_received += nb_bytes;

	if (port->bytes_received >= port->bytes_to_send) {
		/* wakeup a synchronous call to spi_send_frame */
		up(&(port->sync_interface));
	}

	handled = 1;

	return IRQ_RETVAL(handled);
}

/*!
 * This function gets a SPI device handler.
 * It allocates a spi_config structure to manage the SPI device configuration.
 *
 * @param        config        the SPI device configuration
 * @return       This function returns a handle or 0 if error
 */
void *spi_get_device_id(spi_config * config)
{
	void *handle;

	if (config->bit_rate > spi_max_bit_rate) {
		return 0;
	}

	if (config->tx_delay > MAX_TX_DELAY) {
		return 0;
	}

	if ((config->bit_count <= 0) ||
	    ((config->bit_count % TRANSFER_8_BITS) != 0)) {
		return 0;
	}

	handle = (void *)config;

	return handle;
}

/*!
 * This function inititializes register address and set default configuration.
 *
 * @param        spi        the module number
 * @return       This function returns 0 if successful, -EPERM otherwise.
 */
int spi_init_default_conf(module_nb_t spi)
{
	int error = 0;

#ifdef CONFIG_MOT_WFN446
        if(spi >= CONFIG_SPI_NB_MAX) {
            return -EPERM;
        }
#endif

	/* Configure SPI registers adresses */
	spi_add[spi].rx_address = spi_add[spi].base_address +
	    OFFSET_CSPI_RXDATAREG;
	spi_add[spi].tx_address = spi_add[spi].base_address +
	    OFFSET_CSPI_TXDATAREG;
	spi_add[spi].ctrl_address = spi_add[spi].base_address +
	    OFFSET_CSPI_CONTROLREG;
	spi_add[spi].int_address = spi_add[spi].base_address +
	    OFFSET_CSPI_INTREG;
	spi_add[spi].stat_address = spi_add[spi].base_address +
	    OFFSET_CSPI_STATREG;
	spi_add[spi].period_address = spi_add[spi].base_address +
	    OFFSET_CSPI_PERIODREG;
	spi_add[spi].test_address = spi_add[spi].base_address +
	    OFFSET_CSPI_TESTREG;
	spi_add[spi].reset_address = spi_add[spi].base_address +
	    OFFSET_CSPI_RESETREG;

	/* SPI current handle configuration */
	spi_port[spi].current_config_handle = NULL;

	/* SPI exchange mutex initialization */
	init_MUTEX_LOCKED(&(spi_port[spi].sync_interface));

	/* Software reset of the module */
	spi_disable(spi);
	/* Module enabled */
	spi_enable(spi);

	/* Disable all interrupt sources */
	error = spi_disable_interrupt(spi,
				      SPI_STATUS_TX_FIFO_EMPTY |
				      SPI_STATUS_TX_FIFO_HALF_EMPTY |
				      mxc_spi_unique_def->
				      spi_status_transfer_complete |
				      SPI_STATUS_RX_FIFO_DATA_READY |
				      SPI_STATUS_TX_FIFO_FULL |
				      SPI_STATUS_RX_FIFO_HALF_FULL |
				      SPI_STATUS_RX_FIFO_FULL |
				      SPI_STATUS_RX_FIFO_OVERFLOW |
				      mxc_spi_unique_def->
				      spi_status_bit_count_overflow);

	if (error != 0) {
		return error;
	}
	/* Request IRQ */
	/*
	   error = request_irq(spi_port[spi].int_cspi, spi_isr_handler, 0,
	   "cspi IRQ", (void*)(&spi_port[spi]));
	   if (error != 0 ){
	   return error;
	   }
	 */

	/* enable IT */
	/*
	   error = spi_enable_interrupt(spi, SPI_STATUS_RX_FIFO_DATA_READY);
	   if (error != 0 ){
	   return error;
	   }
	 */

	return 0;
}

/*!
 * This function configures the SPI module from an hardware perspective.
 *
 * @return       This function returns 0 if successful, -EPERM otherwise.
 */
int spi_hw_init(void)
{
	int spi = 0;
	int error = 0;

	/* Configure GPIO */
	for (spi = 0; spi < CONFIG_SPI_NB_MAX; spi++) {
		if (spi_present[spi]) {
			gpio_spi_active(spi);
		}
	}

	/* Configure SPI base adresses */
	spi_add[SPI1].base_address = CSPI1_BASE_ADDR;
	spi_add[SPI2].base_address = CSPI2_BASE_ADDR;

#if  defined(CONFIG_ARCH_MX3)
	if (CONFIG_SPI_NB_MAX>=3)
		spi_add[SPI3].base_address = CSPI3_BASE_ADDR;
#endif
		

	/* Configure SPBA */
	spi_init_spba();

	/* Setup SPI version specific defines */
	if (machine_is_mxc27530evb() || machine_is_scma11phone()) {
		if (system_rev >= CHIP_REV_2_0) {
			mxc_spi_unique_def = &spi_ver_0_7;
		} else {
			mxc_spi_unique_def = &spi_ver_0_4;
		}
	} else if (machine_is_mxc91131evb()) {
		mxc_spi_unique_def = &spi_ver_0_5;
	} else if (machine_is_mxc30030evb() || machine_is_mxc30030ads() || machine_is_argonlvphone()) {
		mxc_spi_unique_def = &spi_ver_0_7;
	} else {
		mxc_spi_unique_def = &spi_ver_0_4;
	}

	/* Set interrupt number */
	spi_port[SPI1].int_cspi = INT_CSPI1;
	spi_port[SPI2].int_cspi = INT_CSPI2;

#if  defined(CONFIG_ARCH_MX3)
	if (CONFIG_SPI_NB_MAX>=3)
		spi_port[SPI3].int_cspi = INT_CSPI3;
#endif
	

	for (spi = 0; spi < CONFIG_SPI_NB_MAX; spi++) {
		if (spi_present[spi]) {
			error = spi_init_default_conf(spi);
			if (error != 0) {
				goto cleanup;
			}
		}
	}

	return 0;

      cleanup:
	/* Free all SPI GPIO lines */
	for (spi = 0; spi < CONFIG_SPI_NB_MAX; spi++) {
		if (spi_present[spi]) {
			gpio_spi_inactive(spi);
		}
	}

	return error;
}

/*!
 * This function configures the hardware SPI for the currrent client.
 * Lock must be taken
 *
 * @param        mod              the module number
 * @param        client_config    client hardware configuration.
 * @return       This function returns 0 if successful, -EPERM otherwise.
 */
static int spi_hard_config(module_nb_t mod, spi_config * client_config)
{
	int error = 0;
	error = spi_set_baudrate(mod, client_config->bit_rate);
	if (error < 0) {
		return error;
	}
	spi_set_transfer_length(mod, client_config->bit_count);
	error = spi_select_ss(mod, client_config->ss_asserted);
	if (error < 0) {
		return error;
	}

	if (client_config->master_mode == true) {
		spi_set_mode(mod, SPI_MASTER);
	} else {
		spi_set_mode(mod, SPI_SLAVE);
	}

	if (client_config->active_high_ss_polarity == true) {
		spi_set_ss_polarity(mod, SPI_SS_ACTIVE_HIGH);
	} else {
		spi_set_ss_polarity(mod, SPI_SS_ACTIVE_LOW);
	}

	if (client_config->ss_low_between_bursts == true) {
		spi_set_ss_waveform(mod, SPI_LOW_BTN_BURST);
	} else {
		spi_set_ss_waveform(mod, SPI_PULSE_BTN_BURST);
	}

	if (client_config->phase == true) {
		spi_set_phase(mod, SPI_PHASE_1);
	} else {
		spi_set_phase(mod, SPI_PHASE_0);
	}

	if (client_config->active_high_polarity == true) {
		spi_set_polarity(mod, SPI_POLARITY_ACTIVE_HIGH);
	} else {
		spi_set_polarity(mod, SPI_POLARITY_ACTIVE_LOW);
	}
	return 0;
}

/*!
 * This function sends a SPI frame.
 *
 * @param        buffer		The buffer to send, this buffer must
 *				be allocated with 'nb' bytes size by user
 * @param        bytes          The number of bytes to send, can not be more than 32 bytes.
 * @param        client_config  The handle to identify the SPI device.
 *
 * @return       		This function returns the number of bytes sent
 *				minus the number of received bytes, -1 in case of error.
 */
ssize_t spi_send_frame(unsigned char *buffer, unsigned int bytes,
		       spi_config * client_config)
{
	unsigned long reg;
	module_nb_t mod;
	int error, result;

	if ((buffer == NULL) || (client_config == NULL)) {
		return -1;
	}
	result = -1;
	spin_lock(&mxc_spi_lock);

	if ((bytes * 8) > (client_config->bit_count * 8)) {
		goto error_out;
	}

	mod = client_config->module_number;
	spi_port[mod].hw_config = client_config;

	spi_port[mod].bytes_to_send = bytes;
	spi_port[mod].bytes_received = 0;

	if (spi_port[mod].current_config_handle != client_config) {
		error = spi_hard_config(mod, (void *)client_config);
		if (error < 0) {
			goto error_out;
		}

		spi_port[mod].current_config_handle = (void *)client_config;
	}

	spi_put_tx_data(mod, buffer, bytes);
	DBG_PRINTK("spi%d : sent %d bytes\n", mod + 1, bytes);

	do {
		reg = __raw_readl(IO_ADDRESS(spi_add[mod].ctrl_address));

	} while (reg & MXC_CSPICONREG_XCH);

	spi_port[mod].rx_buffer = buffer;
	result = 0;
	do {
		result += spi_get_rx_data(mod, buffer, bytes);
		buffer += result;
	} while (spi_port[mod].bytes_to_send > result);

	DBG_PRINTK("spi%d : received %d bytes\n", mod + 1, result);
	spi_port[mod].bytes_received = result;
	result = bytes - result;

      error_out:
	spin_unlock(&mxc_spi_lock);
	return result;
}

int mxc_spi_is_active(int spi_id)
{
#ifdef CONFIG_MOT_WFN446
	if ((spi_id < 0) || (spi_id >= CONFIG_SPI_NB_MAX)) {
#else
	if ((spi_id < 0) || (spi_id > CONFIG_SPI_NB_MAX)) {
#endif
		return 0;
	}

	return spi_present[spi_id];
}

/*!
 * This function implements the init function of the SPI device.
 * This function is called when the module is loaded.
 *
 * @return       This function returns 0.
 */
static int __init mxc_spi_init(void)
{
	printk("CSPI driver\n");

	spi_ipg_clk = mxc_get_clocks(IPG_CLK);
	spi_max_bit_rate = (spi_ipg_clk / 4);

#ifdef CONFIG_MXC_SPI_SELECT1
	spi_present[SPI1] = true;
	printk("SPI1 loaded\n");
#else				/* CONFIG_MXC_SPI_SELECT1 */
	spi_present[SPI1] = false;
#endif				/* CONFIG_MXC_SPI_SELECT1 */

#ifdef CONFIG_MXC_SPI_SELECT2
	spi_present[SPI2] = true;
	printk("SPI2 loaded\n");
#else				/* CONFIG_MXC_SPI_SELECT2 */
	spi_present[SPI2] = false;
#endif				/* CONFIG_MXC_SPI_SELECT2 */

#ifndef CONFIG_MOT_WFN408
#ifdef CONFIG_MXC_SPI_SELECT3
	spi_present[SPI3] = true;
	printk("SPI3 loaded\n");
#else				/* CONFIG_MXC_SPI_SELECT3 */
	spi_present[SPI3] = false;
#endif				/* CONFIG_MXC_SPI_SELECT3 */
#endif /* ! CONFIG_MOT_WFN408 */

	return spi_hw_init();


}

/*!
 * This function implements the exit function of the SPI device.
 * This function is called when the module is unloaded.
 *
 */
static void __exit mxc_spi_exit(void)
{
	if (spi_present[SPI1] == 1) {
		free_irq(spi_port[SPI1].int_cspi, spi_isr_handler);
		gpio_spi_inactive(SPI1);
	}

	if (spi_present[SPI2] == 1) {
		free_irq(spi_port[SPI2].int_cspi, spi_isr_handler);
		gpio_spi_inactive(SPI2);
	}

#ifndef CONFIG_MOT_WFN408
	if (spi_present[SPI3] == 1) {
		free_irq(spi_port[SPI3].int_cspi, spi_isr_handler);
		gpio_spi_inactive(SPI3);
	}
#endif /* ! CONFIG_MOT_WFN408 */

	spi_release_spba();
	printk("SPI driver successfully unloaded\n");
}

subsys_initcall(mxc_spi_init);
module_exit(mxc_spi_exit);

EXPORT_SYMBOL(spi_get_device_id);
EXPORT_SYMBOL(spi_send_frame);
EXPORT_SYMBOL(mxc_spi_unique_def);
EXPORT_SYMBOL(mxc_spi_is_active);

MODULE_DESCRIPTION("SPI device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
