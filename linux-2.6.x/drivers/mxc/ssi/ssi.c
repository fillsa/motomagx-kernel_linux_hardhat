/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2006,2007  Motorola, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  
 * 02111-1307, USA
 * 
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ---------------------------
 * 04/28/2006   Motorola  Fixed bug causing audio data loss during A/v capture
 * 03/21/2007   Motorola  Add poll system call support to the audio driver
 *

 */

/*!
 * @file ssi.c
 * @brief This file contains the implementation of the SSI driver main services
 *
 *
 * @ingroup SSI
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
//#include <asm/arch/spba.h>      /* SPBA configuration for SSI2 */
#include <asm/arch/clock.h>
#include <linux/interrupt.h>

#include "registers.h"
#include "ssi.h"

spinlock_t ssi_lock = SPIN_LOCK_UNLOCKED;
#ifdef SSI_TEST

#include <linux/devfs_fs_kernel.h>      /* dev fs interface */

#define  MONO_BUFFER_SIZE     8192
#define  STEREO_BUFFER_SIZE   80000

static unsigned int open_count = 0;

/*!
 * SSI1 major
 */
static int major_ssi1;

/*!
 * SSI2 major
 */
static int major_ssi2;

static unsigned char *data_depeche[2];
static unsigned long data_size[2];
static unsigned char *usr_data_depeche[2];
static unsigned long usr_data_size[2];
static unsigned char *temp_ptr;
 
/* Variables copied over from ssi_ioctl. This is because we want to access them from the
   interrupt handler. We also don't want automatic variables in the interrupt handler.  
   Moreover, access to global variables is faster */
static unsigned long uRdIndx, uWrIndx;
static unsigned long i, k, m;
static unsigned int left,right;
static ssi_mod mod = 0;
static int temp, buf_index;
static bool capture_flag;
static bool record_buffering;
/* End of variables from ssi_ioctl */

/* Wait queue */
static wait_queue_head_t ssi1_wait_queue;
static wait_queue_head_t ssi2_wait_queue;

#endif

EXPORT_SYMBOL(ssi_ac97_frame_rate_divider);
EXPORT_SYMBOL(ssi_ac97_get_command_address_register);
EXPORT_SYMBOL(ssi_ac97_get_command_data_register);
EXPORT_SYMBOL(ssi_ac97_get_tag_register);
EXPORT_SYMBOL(ssi_ac97_mode_enable);
EXPORT_SYMBOL(ssi_ac97_tag_in_fifo);
EXPORT_SYMBOL(ssi_ac97_read_command);
EXPORT_SYMBOL(ssi_ac97_set_command_address_register);
EXPORT_SYMBOL(ssi_ac97_set_command_data_register);
EXPORT_SYMBOL(ssi_ac97_set_tag_register);
EXPORT_SYMBOL(ssi_ac97_variable_mode);
EXPORT_SYMBOL(ssi_ac97_write_command);
EXPORT_SYMBOL(ssi_clock_idle_state);
EXPORT_SYMBOL(ssi_clock_off);
EXPORT_SYMBOL(ssi_enable);
EXPORT_SYMBOL(ssi_get_data);
EXPORT_SYMBOL(ssi_get_status);
EXPORT_SYMBOL(ssi_i2s_mode);
EXPORT_SYMBOL(ssi_interrupt_disable);
EXPORT_SYMBOL(ssi_interrupt_enable);
EXPORT_SYMBOL(ssi_network_mode);
EXPORT_SYMBOL(ssi_receive_enable);
EXPORT_SYMBOL(ssi_rx_bit0);
EXPORT_SYMBOL(ssi_rx_clock_direction);
EXPORT_SYMBOL(ssi_rx_clock_divide_by_two);
EXPORT_SYMBOL(ssi_rx_clock_polarity);
EXPORT_SYMBOL(ssi_rx_clock_prescaler);
EXPORT_SYMBOL(ssi_rx_early_frame_sync);
EXPORT_SYMBOL(ssi_rx_fifo_counter);
EXPORT_SYMBOL(ssi_rx_fifo_enable);
EXPORT_SYMBOL(ssi_rx_fifo_full_watermark);
EXPORT_SYMBOL(ssi_rx_flush_fifo);
EXPORT_SYMBOL(ssi_rx_frame_direction);
EXPORT_SYMBOL(ssi_rx_frame_rate);
EXPORT_SYMBOL(ssi_rx_frame_sync_active);
EXPORT_SYMBOL(ssi_rx_frame_sync_length);
EXPORT_SYMBOL(ssi_rx_mask_time_slot);
EXPORT_SYMBOL(ssi_rx_prescaler_modulus);
EXPORT_SYMBOL(ssi_rx_shift_direction);
EXPORT_SYMBOL(ssi_rx_word_length);
EXPORT_SYMBOL(ssi_set_data);
EXPORT_SYMBOL(ssi_set_wait_states);
EXPORT_SYMBOL(ssi_synchronous_mode);
EXPORT_SYMBOL(ssi_system_clock);
EXPORT_SYMBOL(ssi_transmit_enable);
EXPORT_SYMBOL(ssi_two_channel_mode);
EXPORT_SYMBOL(ssi_tx_bit0);
EXPORT_SYMBOL(ssi_tx_clock_direction);
EXPORT_SYMBOL(ssi_tx_clock_divide_by_two);
EXPORT_SYMBOL(ssi_tx_clock_polarity);
EXPORT_SYMBOL(ssi_tx_clock_prescaler);
EXPORT_SYMBOL(ssi_tx_early_frame_sync);
EXPORT_SYMBOL(ssi_tx_fifo_counter);
EXPORT_SYMBOL(ssi_tx_fifo_empty_watermark);
EXPORT_SYMBOL(ssi_tx_fifo_enable);
EXPORT_SYMBOL(ssi_tx_flush_fifo);
EXPORT_SYMBOL(ssi_tx_frame_direction);
EXPORT_SYMBOL(ssi_tx_frame_rate);
EXPORT_SYMBOL(ssi_tx_frame_sync_active);
EXPORT_SYMBOL(ssi_tx_frame_sync_length);
EXPORT_SYMBOL(ssi_tx_mask_time_slot);
EXPORT_SYMBOL(ssi_tx_prescaler_modulus);
EXPORT_SYMBOL(ssi_tx_shift_direction);
EXPORT_SYMBOL(ssi_tx_word_length);

void set_register_bits(unsigned int mask, unsigned int data,
		       unsigned int offset, unsigned int ssi)
{
	volatile unsigned long reg = 0;
	unsigned int base_addr = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&ssi_lock, flags);
	base_addr = (ssi == SSI1) ? IO_ADDRESS(SSI1_BASE_ADDR) :
	    IO_ADDRESS(SSI2_BASE_ADDR);
	reg = __raw_readl(base_addr + offset);
	reg = (reg & (~mask)) | (data & mask);
	__raw_writel(reg, base_addr + offset);
	spin_unlock_irqrestore(&ssi_lock, flags);
}

volatile unsigned long getreg_value(unsigned int offset, unsigned int ssi)
{
	volatile unsigned long reg = 0;
	unsigned int base_addr = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&ssi_lock, flags);
	base_addr = (ssi == SSI1) ? IO_ADDRESS(SSI1_BASE_ADDR) :
	    IO_ADDRESS(SSI2_BASE_ADDR);
	reg = __raw_readl(base_addr + offset);
	spin_unlock_irqrestore(&ssi_lock, flags);

	return reg;
}

/*!
 * SSI interrupt handler routine.
 *
 * @param   irq    the interrupt number
 * @param   dev_id driver private data
 * @param   regs   holds a snapshot of the processor's context before the 
 *                 processor entered the interrupt code
 * @return the function returns \b IRQ_RETVAL(1) -  interrupt was handled
 */
static irqreturn_t ssi1_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    if (capture_flag == false)
    {
        /* disable interrupt first */
        ssi_interrupt_disable (mod, ssi_tx_fifo_0_empty | ssi_tx_interrupt_enable);

        /* If there are <= 2 samples in the FIFO then put more data in the FIFO */
        if ((getreg_value(MXC_SSISFCSR, mod) & 0xF00) <= 0x200) 
        {
            /* Copy 6 samples in the SSI FIFO */
            for (k = 0; k < 6 && data_size[mod] > 0; k++)
            {
                left = (unsigned int)(((data_depeche[mod][i+1]<<8)&0xFF00)|
                                       (data_depeche[mod][i]&0x00FF));
                ssi_set_data(mod, ssi_fifo_0, left);
                data_size[mod] -= 2;
                i += 2;
            }
        }      

        if (data_size[mod] < 2)
        {
            wake_up_interruptible (&ssi1_wait_queue);
        }
        else
	{
            ssi_interrupt_enable (mod, ssi_tx_fifo_0_empty | ssi_tx_interrupt_enable);
	}
    }
    else
    {
        ssi_interrupt_disable (mod, ssi_rx_fifo_0_full | ssi_rx_interrupt_enable);

        i = (getreg_value(MXC_SSISFCSR, SSI1) & 0xF000) >> 12;
        for(k = 0; k < i; k++) {
          if(uWrIndx >= STEREO_BUFFER_SIZE) uWrIndx = 0;
          temp = ssi_get_data (SSI1, ssi_fifo_0); 
          data_depeche[1][uWrIndx]   = temp & 0x00FF;
          data_depeche[1][uWrIndx+1] = (temp & 0xFF00)>>8; 
          uWrIndx += 2;
        }

        if(uRdIndx <= uWrIndx)
          i = uWrIndx - uRdIndx;
        else if(uRdIndx > uWrIndx)
          i = STEREO_BUFFER_SIZE - uRdIndx + uWrIndx;

        if ((i >= data_size[1]) && (data_size[1] != 0)) {
            wake_up_interruptible (&ssi1_wait_queue);
        }

        ssi_interrupt_enable (mod, ssi_rx_fifo_0_full | ssi_rx_interrupt_enable);
    }

    return IRQ_RETVAL(1);
}

static irqreturn_t ssi2_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    /* disable interrupt first */
    ssi_interrupt_disable (mod, ssi_tx_fifo_1_empty | ssi_tx_interrupt_enable);

    if ((getreg_value(MXC_SSISFCSR, mod) & 0xF000000) <= 0x2000000) 
    {
        /* Copy 6 samples in the SSI FIFO */
        for (k = 0; k < 6 && data_size[buf_index] >= 4; k++)
        {
            left = (unsigned int)(((data_depeche[buf_index][i+1]<<8)&0xFF00)|
                                   (data_depeche[buf_index][i]&0x00FF));
            right = (unsigned int)(((data_depeche[buf_index][i+3]<<8)&0xFF00)|
                                    (data_depeche[buf_index][i+2]&0x00FF));
            ssi_set_data (mod, ssi_fifo_0, left);
            ssi_set_data (mod, ssi_fifo_1, right);
            data_size[buf_index] -= 4;
            i += 4;
        }
    }

    if (data_size[buf_index] < 4)
    {
        if (data_size[(buf_index + 1) % 2] > 4)
	{
            buf_index = (buf_index + 1) % 2;
            i = m;
            ssi_interrupt_enable (mod, ssi_tx_fifo_1_empty | ssi_tx_interrupt_enable);	
        }
        wake_up_interruptible (&ssi2_wait_queue);
    }
    else
    {
        ssi_interrupt_enable (mod, ssi_tx_fifo_1_empty | ssi_tx_interrupt_enable);
    }

    return IRQ_RETVAL(1);
}


void set_register(unsigned int data, unsigned int offset, unsigned int ssi)
{
	unsigned int base_addr = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&ssi_lock, flags);
	base_addr = (ssi == SSI1) ? IO_ADDRESS(SSI1_BASE_ADDR) :
	    IO_ADDRESS(SSI2_BASE_ADDR);
	__raw_writel(data, base_addr + offset);
	spin_unlock_irqrestore(&ssi_lock, flags);

}

/*!
 * This function controls the AC97 frame rate divider.
 *
 * @param        module               the module number
 * @param        frame_rate_divider   the AC97 frame rate divider
 */
void ssi_ac97_frame_rate_divider(ssi_mod module,
				 unsigned char frame_rate_divider)
{
	unsigned int reg = 0;

	reg = getreg_value(MXC_SSISACNT, module);
	reg |= ((frame_rate_divider & AC97_FRAME_RATE_MASK)
		<< AC97_FRAME_RATE_DIVIDER_SHIFT);
	set_register(reg, MXC_SSISACNT, module);
}

/*!
 * This function gets the AC97 command address register.
 *
 * @param        module        the module number
 * @return       This function returns the command address slot information. 
 */
unsigned int ssi_ac97_get_command_address_register(ssi_mod module)
{
	return getreg_value(MXC_SSISACADD, module);
}

/*!
 * This function gets the AC97 command data register.
 *
 * @param        module        the module number
 * @return       This function returns the command data slot information.
 */
unsigned int ssi_ac97_get_command_data_register(ssi_mod module)
{
	return getreg_value(MXC_SSISACDAT, module);
}

/*!
 * This function gets the AC97 tag register.
 *
 * @param        module        the module number
 * @return       This function returns the tag information.
 */
unsigned int ssi_ac97_get_tag_register(ssi_mod module)
{
	return getreg_value(MXC_SSISATAG, module);
}

/*!
 * This function controls the AC97 mode.
 *
 * @param        module        the module number
 * @param        state         the AC97 mode state (enabled or disabled)
 */
void ssi_ac97_mode_enable(ssi_mod module, bool state)
{
	unsigned int reg = 0;

	reg = getreg_value(MXC_SSISACNT, module);
	if (state == true) {
		reg |= (1 << AC97_MODE_ENABLE_SHIFT);
	} else {
		reg &= ~(1 << AC97_MODE_ENABLE_SHIFT);
	}

	set_register(reg, MXC_SSISACNT, module);
}

/*!
 * This function controls the AC97 tag in FIFO behavior.
 *
 * @param        module        the module number
 * @param        state         the tag in fifo behavior (Tag info stored in Rx FIFO 0 if true, 
 * Tag info stored in SATAG register otherwise)
 */
void ssi_ac97_tag_in_fifo(ssi_mod module, bool state)
{
	unsigned int reg = 0;

	reg = getreg_value(MXC_SSISACNT, module);
	if (state == true) {
		reg |= (1 << AC97_TAG_IN_FIFO_SHIFT);
	} else {
		reg &= ~(1 << AC97_TAG_IN_FIFO_SHIFT);
	}

	set_register(reg, MXC_SSISACNT, module);
}

/*!
 * This function controls the AC97 read command.
 *
 * @param        module        the module number
 * @param        state         the next AC97 command is a read command or not
 */
void ssi_ac97_read_command(ssi_mod module, bool state)
{
	unsigned int reg = 0;

	reg = getreg_value(MXC_SSISACNT, module);
	if (state == true) {
		reg |= (1 << AC97_READ_COMMAND_SHIFT);
	} else {
		reg &= ~(1 << AC97_READ_COMMAND_SHIFT);
	}

	set_register(reg, MXC_SSISACNT, module);
}

/*!
 * This function sets the AC97 command address register.
 *
 * @param        module        the module number
 * @param        address       the command address slot information
 */
void ssi_ac97_set_command_address_register(ssi_mod module, unsigned int address)
{
	set_register(address, MXC_SSISACADD, module);
}

/*!
 * This function sets the AC97 command data register.
 *
 * @param        module        the module number
 * @param        data          the command data slot information
 */
void ssi_ac97_set_command_data_register(ssi_mod module, unsigned int data)
{
	set_register(data, MXC_SSISACDAT, module);
}

/*!
 * This function sets the AC97 tag register.
 *
 * @param        module        the module number
 * @param        tag           the tag information
 */
void ssi_ac97_set_tag_register(ssi_mod module, unsigned int tag)
{
	set_register(tag, MXC_SSISATAG, module);
}

/*!
 * This function controls the AC97 variable mode.
 *
 * @param        module        the module number
 * @param        state         the AC97 variable mode state (enabled or disabled)
 */ void ssi_ac97_variable_mode(ssi_mod module, bool state)
{
	unsigned int reg = 0;

	reg = getreg_value(MXC_SSISACNT, module);
	if (state == true) {
		reg |= (1 << AC97_VARIABLE_OPERATION_SHIFT);
	} else {
		reg &= ~(1 << AC97_VARIABLE_OPERATION_SHIFT);
	}

	set_register(reg, MXC_SSISACNT, module);
}

/*!
 * This function controls the AC97 write command.
 *
 * @param        module        the module number
 * @param        state         the next AC97 command is a write command or not
 */
void ssi_ac97_write_command(ssi_mod module, bool state)
{
	unsigned int reg = 0;

	reg = getreg_value(MXC_SSISACNT, module);
	if (state == true) {
		reg |= (1 << AC97_WRITE_COMMAND_SHIFT);
	} else {
		reg &= ~(1 << AC97_WRITE_COMMAND_SHIFT);
	}

	set_register(reg, MXC_SSISACNT, module);
}

/*!
 * This function controls the idle state of the transmit clock port during SSI internal gated mode.
 *
 * @param        module        the module number
 * @param        state         the clock idle state
 */
void ssi_clock_idle_state(ssi_mod module, idle_state state)
{
	set_register_bits(1 << SSI_CLOCK_IDLE_SHIFT,
			  state << SSI_CLOCK_IDLE_SHIFT, MXC_SSISCR, module);
}

/*!
 * This function turns off/on the ccm_ssi_clk to reduce power consumption.
 *
 * @param        module        the module number
 * @param        state         the state for ccm_ssi_clk (true: turn off, else:turn on)
 */
void ssi_clock_off(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_CLOCK_OFF_SHIFT,
			  state << SSI_CLOCK_OFF_SHIFT, MXC_SSISOR, module);
}

/*!
 * This function enables/disables the SSI module.
 *
 * @param        module        the module number
 * @param        state         the state for SSI module
 */
void ssi_enable(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_ENABLE_SHIFT, state << SSI_ENABLE_SHIFT,
			  MXC_SSISCR, module);
}

/*!
 * This function gets the data word in the Receive FIFO of the SSI module.
 *
 * @param        module        the module number
 * @param        fifo          the Receive FIFO to read
 * @return       This function returns the read data.
 */
unsigned int ssi_get_data(ssi_mod module, fifo_nb fifo)
{
	unsigned int result = 0;

	if (ssi_fifo_0 == fifo) {
		result = getreg_value(MXC_SSISRX0, module);
	} else {
		result = getreg_value(MXC_SSISRX1, module);
	}

	return result;
}

/*!
 * This function returns the status of the SSI module (SISR register) as a combination of status.
 *
 * @param        module        the module number
 * @return       This function returns the status of the SSI module
 */
ssi_status_enable_mask ssi_get_status(ssi_mod module)
{
	unsigned int result;

	result = getreg_value(MXC_SSISISR, module);
	result &= ((1 << SSI_IRQ_STATUS_NUMBER) - 1);

	return (ssi_status_enable_mask) result;
}

/*!
 * This function selects the I2S mode of the SSI module.
 *
 * @param        module        the module number
 * @param        mode          the I2S mode 
 */
void ssi_i2s_mode(ssi_mod module, mode_i2s mode)
{
	set_register_bits(3 << SSI_I2S_MODE_SHIFT, mode << SSI_I2S_MODE_SHIFT,
			  MXC_SSISCR, module);
}

/*!
 * This function disables the interrupts of the SSI module.
 *
 * @param        module        the module number
 * @param        mask          the mask of the interrupts to disable 
 */
void ssi_interrupt_disable(ssi_mod module, ssi_status_enable_mask mask)
{
      // set_register_bits(mask, 0, MXC_SSISIER, module);
      ModifyRegister32(mask, 0, _reg_SSI_SIER(module));
}

/*!
 * This function enables the interrupts of the SSI module.
 *
 * @param        module        the module number
 * @param        mask          the mask of the interrupts to enable 
 */
void ssi_interrupt_enable(ssi_mod module, ssi_status_enable_mask mask)
{
      // set_register_bits(0, mask, MXC_SSISIER, module);
      ModifyRegister32(0, mask, _reg_SSI_SIER(module));
}

/*!
 * This function enables/disables the network mode of the SSI module.
 *
 * @param        module        the module number
 * @param        state         the network mode state 
 */
void ssi_network_mode(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_NETWORK_MODE_SHIFT,
			  state << SSI_NETWORK_MODE_SHIFT, MXC_SSISCR, module);
}

/*!
 * This function enables/disables the receive section of the SSI module.
 *
 * @param        module        the module number
 * @param        state         the receive section state 
 */
void ssi_receive_enable(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_RECEIVE_ENABLE_SHIFT,
			  state << SSI_RECEIVE_ENABLE_SHIFT, MXC_SSISCR,
			  module);
}

/*!
 * This function configures the SSI module to receive data word at bit position 0 or 23 in the Receive shift register.
 *
 * @param        module        the module number
 * @param        state         the state to receive at bit 0 
 */
void ssi_rx_bit0(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_BIT_0_SHIFT, state << SSI_BIT_0_SHIFT,
			  MXC_SSISRCR, module);
}

/*!
 * This function controls the source of the clock signal used to clock the Receive shift register.
 *
 * @param        module        the module number
 * @param        direction     the clock signal direction 
 */
void ssi_rx_clock_direction(ssi_mod module, ssi_tx_rx_direction direction)
{
	set_register_bits(1 << SSI_CLOCK_DIRECTION_SHIFT,
			  direction << SSI_CLOCK_DIRECTION_SHIFT, MXC_SSISRCR,
			  module);
}

/*!
 * This function configures the divide-by-two divider of the SSI module for the receive section.
 *
 * @param        module        the module number
 * @param        state         the divider state 
 */
void ssi_rx_clock_divide_by_two(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_DIVIDE_BY_TWO_SHIFT,
			  state << SSI_DIVIDE_BY_TWO_SHIFT, MXC_SSISRCCR,
			  module);
}

/*!
 * This function controls which bit clock edge is used to clock in data.
 *
 * @param        module        the module number
 * @param        polarity      the clock polarity 
 */
void ssi_rx_clock_polarity(ssi_mod module, ssi_tx_rx_clock_polarity polarity)
{
	set_register_bits(1 << SSI_CLOCK_POLARITY_SHIFT,
			  polarity << SSI_CLOCK_POLARITY_SHIFT, MXC_SSISRCR,
			  module);
}

/*!
 * This function configures a fixed divide-by-eight clock prescaler divider of the SSI module in series with the variable prescaler for the receive section.
 *
 * @param        module        the module number
 * @param        state         the prescaler state 
 */
void ssi_rx_clock_prescaler(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_PRESCALER_RANGE_SHIFT,
			  state << SSI_PRESCALER_RANGE_SHIFT,
			  MXC_SSISRCCR, module);
}

/*!
 * This function controls the early frame sync configuration.
 *
 * @param        module        the module number
 * @param        early         the early frame sync configuration 
 */
void ssi_rx_early_frame_sync(ssi_mod module, ssi_tx_rx_early_frame_sync early)
{
	set_register_bits(1 << SSI_EARLY_FRAME_SYNC_SHIFT,
			  early << SSI_EARLY_FRAME_SYNC_SHIFT,
			  MXC_SSISRCR, module);
}

/*!
 * This function gets the number of data words in the Receive FIFO.
 *
 * @param        module        the module number
 * @param        fifo          the fifo 
 * @return       This function returns the number of words in the Rx FIFO.
 */
unsigned char ssi_rx_fifo_counter(ssi_mod module, fifo_nb fifo)
{
	unsigned char result;
	result = 0;

	if (ssi_fifo_0 == fifo) {
		result = getreg_value(MXC_SSISFCSR, module);
		result &= (0xF << SSI_RX_FIFO_0_COUNT_SHIFT);
		result = result >> SSI_RX_FIFO_0_COUNT_SHIFT;
	} else {
		result = getreg_value(MXC_SSISFCSR, module);
		result &= (0xF << SSI_RX_FIFO_1_COUNT_SHIFT);
		result = result >> SSI_RX_FIFO_1_COUNT_SHIFT;
	}

	return result;
}

/*!
 * This function enables the Receive FIFO.
 *
 * @param        module        the module number
 * @param        fifo          the fifo to enable
 * @param        enable        the state of the fifo, enabled or disabled
 */

void ssi_rx_fifo_enable(ssi_mod module, fifo_nb fifo, bool enable)
{
	volatile unsigned int reg;

	reg = getreg_value(MXC_SSISRCR, module);
	if (enable == true) {
		reg |= ((1 + fifo) << SSI_FIFO_ENABLE_0_SHIFT);
	} else {
		reg &= ~((1 + fifo) << SSI_FIFO_ENABLE_0_SHIFT);
	}

	set_register(reg, MXC_SSISRCR, module);
}

/*!
 * This function controls the threshold at which the RFFx flag will be set.
 *
 * @param        module        the module number
 * @param        fifo          the fifo to enable
 * @param        watermark     the watermark
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_rx_fifo_full_watermark(ssi_mod module,
			       fifo_nb fifo, unsigned char watermark)
{
	int result = -1;
	result = -1;

	if ((watermark > SSI_MIN_FIFO_WATERMARK) &&
	    (watermark <= SSI_MAX_FIFO_WATERMARK)) {
		if (ssi_fifo_0 == fifo) {
			set_register_bits(0xf << SSI_RX_FIFO_0_WATERMARK_SHIFT,
					  watermark <<
					  SSI_RX_FIFO_0_WATERMARK_SHIFT,
					  MXC_SSISFCSR, module);
		} else {
			set_register_bits(0xf << SSI_RX_FIFO_1_WATERMARK_SHIFT,
					  watermark <<
					  SSI_RX_FIFO_1_WATERMARK_SHIFT,
					  MXC_SSISFCSR, module);
		}

		result = 0;
	}

	return result;
}

/*!
 * This function flushes the Receive FIFOs.
 *
 * @param        module        the module number
 */
void ssi_rx_flush_fifo(ssi_mod module)
{
	set_register_bits(0, 1 << SSI_RECEIVER_CLEAR_SHIFT, MXC_SSISOR, module);
}

/*!
 * This function controls the direction of the Frame Sync signal for the receive section.
 *
 * @param        module        the module number
 * @param        direction     the Frame Sync signal direction
 */
void ssi_rx_frame_direction(ssi_mod module, ssi_tx_rx_direction direction)
{
	set_register_bits(1 << SSI_FRAME_DIRECTION_SHIFT,
			  direction << SSI_FRAME_DIRECTION_SHIFT,
			  MXC_SSISRCR, module);
}

/*!
 * This function configures the Receive frame rate divider for the receive section.
 *
 * @param        module        the module number
 * @param        ratio         the divide ratio from 1 to 32
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_rx_frame_rate(ssi_mod module, unsigned char ratio)
{
	int result = -1;

	if ((ratio >= SSI_MIN_FRAME_DIVIDER_RATIO) &&
	    (ratio <= SSI_MAX_FRAME_DIVIDER_RATIO)) {
		set_register_bits(SSI_FRAME_DIVIDER_MASK <<
				  SSI_FRAME_RATE_DIVIDER_SHIFT,
				  (ratio - 1) << SSI_FRAME_RATE_DIVIDER_SHIFT,
				  MXC_SSISRCCR, module);
		result = 0;
	}

	return result;
}

/*!
 * This function controls the Frame Sync active polarity for the receive section.
 *
 * @param        module        the module number
 * @param        active        the Frame Sync active polarity
 */
void ssi_rx_frame_sync_active(ssi_mod module,
			      ssi_tx_rx_frame_sync_active active)
{
	set_register_bits(1 << SSI_FRAME_SYNC_INVERT_SHIFT,
			  active << SSI_FRAME_SYNC_INVERT_SHIFT,
			  MXC_SSISRCR, module);
}

/*!
 * This function controls the Frame Sync length (one word or one bit long) for the receive section.
 *
 * @param        module        the module number
 * @param        length        the Frame Sync length
 */
void ssi_rx_frame_sync_length(ssi_mod module,
			      ssi_tx_rx_frame_sync_length length)
{
	set_register_bits(1 << SSI_FRAME_SYNC_LENGTH_SHIFT,
			  length << SSI_FRAME_SYNC_LENGTH_SHIFT,
			  MXC_SSISRCR, module);
}

/*!
 * This function configures the time slot(s) to mask for the receive section.
 *
 * @param        module        the module number
 * @param        mask          the mask to indicate the time slot(s) masked
 */
void ssi_rx_mask_time_slot(ssi_mod module, unsigned int mask)
{
	set_register_bits(0xFFFFFFFF, mask, MXC_SSISRMSK, module);
}

/*!
 * This function configures the Prescale divider for the receive section.
 *
 * @param        module        the module number
 * @param        divider       the divide ratio from 1 to  256
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_rx_prescaler_modulus(ssi_mod module, unsigned int divider)
{
	int result = -1;

	if ((divider >= SSI_MIN_PRESCALER_MODULUS_RATIO) &&
	    (divider <= SSI_MAX_PRESCALER_MODULUS_RATIO)) {

		set_register_bits(SSI_PRESCALER_MODULUS_MASK <<
				  SSI_PRESCALER_MODULUS_SHIFT,
				  (divider - 1) << SSI_PRESCALER_MODULUS_SHIFT,
				  MXC_SSISRCCR, module);
		result = 0;
	}

	return result;
}

/*!
 * This function controls whether the MSB or LSB will be received first in a sample.
 *
 * @param        module        the module number
 * @param        direction     the shift direction
 */
void ssi_rx_shift_direction(ssi_mod module, ssi_tx_rx_shift_direction direction)
{
	set_register_bits(1 << SSI_SHIFT_DIRECTION_SHIFT,
			  direction << SSI_SHIFT_DIRECTION_SHIFT,
			  MXC_SSISRCR, module);
}

/*!
 * This function configures the Receive word length.
 *
 * @param        module        the module number
 * @param        length        the word length
 */
void ssi_rx_word_length(ssi_mod module, ssi_word_length length)
{
	set_register_bits(SSI_WORD_LENGTH_MASK << SSI_WORD_LENGTH_SHIFT,
			  length << SSI_WORD_LENGTH_SHIFT,
			  MXC_SSISRCCR, module);
}

/*!
 * This function sets the data word in the Transmit FIFO of the SSI module.
 *
 * @param        module        the module number
 * @param        fifo          the FIFO number
 * @param        data          the data to load in the FIFO
 */

void ssi_set_data(ssi_mod module, fifo_nb fifo, unsigned int data)
{
	if (ssi_fifo_0 == fifo) {
		set_register(data, MXC_SSISTX0, module);
	} else {
		set_register(data, MXC_SSISTX1, module);
	}
}

/*!
 * This function controls the number of wait states between the core and SSI.
 *
 * @param        module        the module number
 * @param        wait          the number of wait state(s)
 */
void ssi_set_wait_states(ssi_mod module, ssi_wait_states wait)
{
	set_register_bits(SSI_WAIT_STATE_MASK << SSI_WAIT_SHIFT,
			  wait << SSI_WAIT_SHIFT, MXC_SSISOR, module);
}

/*!
 * This function enables/disables the synchronous mode of the SSI module.
 *
 * @param        module        the module number
 * @param        state         the synchronous mode state
 */
void ssi_synchronous_mode(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_SYNCHRONOUS_MODE_SHIFT,
			  state << SSI_SYNCHRONOUS_MODE_SHIFT,
			  MXC_SSISCR, module);
}

/*!
 * This function allows the SSI module to output the SYS_CLK at the SRCK port.
 *
 * @param        module        the module number
 * @param        state         the system clock state
 */
void ssi_system_clock(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_SYSTEM_CLOCK_SHIFT,
			  state << SSI_SYSTEM_CLOCK_SHIFT, MXC_SSISCR, module);
}

/*!
 * This function enables/disables the transmit section of the SSI module.
 *
 * @param        module        the module number
 * @param        state         the transmit section state
 */
void ssi_transmit_enable(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_TRANSMIT_ENABLE_SHIFT,
			  state << SSI_TRANSMIT_ENABLE_SHIFT,
			  MXC_SSISCR, module);
}

/*!
 * This function allows the SSI module to operate in the two channel mode.
 *
 * @param        module        the module number
 * @param        state         the two channel mode state
 */
void ssi_two_channel_mode(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_TWO_CHANNEL_SHIFT,
			  state << SSI_TWO_CHANNEL_SHIFT, MXC_SSISCR, module);
}

/*!
 * This function configures the SSI module to transmit data word from bit position 0 or 23 in the Transmit shift register.
 *
 * @param        module        the module number
 * @param        state         the transmit from bit 0 state 
 */
void ssi_tx_bit0(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_BIT_0_SHIFT,
			  state << SSI_BIT_0_SHIFT, MXC_SSISTCR, module);
}

/*!
 * This function controls the direction of the clock signal used to clock the Transmit shift register.
 *
 * @param        module        the module number
 * @param        direction     the clock signal direction 
 */
void ssi_tx_clock_direction(ssi_mod module, ssi_tx_rx_direction direction)
{
	set_register_bits(1 << SSI_CLOCK_DIRECTION_SHIFT,
			  direction << SSI_CLOCK_DIRECTION_SHIFT,
			  MXC_SSISTCR, module);
}

/*!
 * This function configures the divide-by-two divider of the SSI module for the transmit section.
 *
 * @param        module        the module number
 * @param        state         the divider state 
 */
void ssi_tx_clock_divide_by_two(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_DIVIDE_BY_TWO_SHIFT,
			  state << SSI_DIVIDE_BY_TWO_SHIFT,
			  MXC_SSISTCCR, module);
}

/*!
 * This function controls which bit clock edge is used to clock out data.
 *
 * @param        module        the module number
 * @param        polarity      the clock polarity 
 */
void ssi_tx_clock_polarity(ssi_mod module, ssi_tx_rx_clock_polarity polarity)
{
	set_register_bits(1 << SSI_CLOCK_POLARITY_SHIFT,
			  polarity << SSI_CLOCK_POLARITY_SHIFT,
			  MXC_SSISTCR, module);
}

/*!
 * This function configures a fixed divide-by-eight clock prescaler divider of the SSI module in series with the variable prescaler for the transmit section.
 *
 * @param        module        the module number
 * @param        state         the prescaler state 
 */
void ssi_tx_clock_prescaler(ssi_mod module, bool state)
{
	set_register_bits(1 << SSI_PRESCALER_RANGE_SHIFT,
			  state << SSI_PRESCALER_RANGE_SHIFT,
			  MXC_SSISTCCR, module);
}

/*!
 * This function controls the early frame sync configuration for the transmit section.
 *
 * @param        module        the module number
 * @param        early         the early frame sync configuration 
 */
void ssi_tx_early_frame_sync(ssi_mod module, ssi_tx_rx_early_frame_sync early)
{
	set_register_bits(1 << SSI_EARLY_FRAME_SYNC_SHIFT,
			  early << SSI_EARLY_FRAME_SYNC_SHIFT,
			  MXC_SSISTCR, module);
}

/*!
 * This function gets the number of data words in the Transmit FIFO.
 *
 * @param        module        the module number
 * @param        fifo          the fifo 
 * @return       This function returns the number of words in the Tx FIFO.
 */
unsigned char ssi_tx_fifo_counter(ssi_mod module, fifo_nb fifo)
{
	unsigned char result = 0;

	if (ssi_fifo_0 == fifo) {
		result = getreg_value(MXC_SSISFCSR, module);
		result &= (0xF << SSI_TX_FIFO_0_COUNT_SHIFT);
		result >>= SSI_TX_FIFO_0_COUNT_SHIFT;
	} else {
		result = getreg_value(MXC_SSISFCSR, module);
		result &= (0xF << SSI_TX_FIFO_1_COUNT_SHIFT);
		result >>= SSI_TX_FIFO_1_COUNT_SHIFT;
	}

	return result;
}

/*!
 * This function controls the threshold at which the TFEx flag will be set.
 *
 * @param        module        the module number
 * @param        fifo          the fifo to enable
 * @param        watermark     the watermark
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_tx_fifo_empty_watermark(ssi_mod module,
				fifo_nb fifo, unsigned char watermark)
{
	int result = -1;

	if ((watermark > SSI_MIN_FIFO_WATERMARK) &&
	    (watermark <= SSI_MAX_FIFO_WATERMARK)) {
		if (ssi_fifo_0 == fifo) {
			set_register_bits(0xf << SSI_TX_FIFO_0_WATERMARK_SHIFT,
					  watermark <<
					  SSI_TX_FIFO_0_WATERMARK_SHIFT,
					  MXC_SSISFCSR, module);
		} else {
			set_register_bits(0xf << SSI_TX_FIFO_1_WATERMARK_SHIFT,
					  watermark <<
					  SSI_TX_FIFO_1_WATERMARK_SHIFT,
					  MXC_SSISFCSR, module);
		}

		result = 0;
	}

	return result;
}

/*!
 * This function enables the Transmit FIFO.
 *
 * @param        module        the module number
 * @param        fifo          the fifo to enable
 * @param        enable        the state of the fifo, enabled or disabled
 */

void ssi_tx_fifo_enable(ssi_mod module, fifo_nb fifo, bool enable)
{
	unsigned int reg;

	reg = getreg_value(MXC_SSISTCR, module);
	if (enable == true) {
		reg |= ((1 + fifo) << SSI_FIFO_ENABLE_0_SHIFT);
	} else {
		reg &= ~((1 + fifo) << SSI_FIFO_ENABLE_0_SHIFT);
	}

	set_register(reg, MXC_SSISTCR, module);
}

/*!
 * This function flushes the Transmit FIFOs.
 *
 * @param        module        the module number
 */
void ssi_tx_flush_fifo(ssi_mod module)
{
	set_register_bits(1 << SSI_TRANSMITTER_CLEAR_SHIFT, 1 << SSI_TRANSMITTER_CLEAR_SHIFT,
			  MXC_SSISOR, module);
}

/*!
 * This function controls the direction of the Frame Sync signal for the transmit section.
 *
 * @param        module        the module number
 * @param        direction     the Frame Sync signal direction
 */
void ssi_tx_frame_direction(ssi_mod module, ssi_tx_rx_direction direction)
{
	set_register_bits(1 << SSI_FRAME_DIRECTION_SHIFT,
			  direction << SSI_FRAME_DIRECTION_SHIFT,
			  MXC_SSISTCR, module);
}

/*!
 * This function configures the Transmit frame rate divider.
 *
 * @param        module        the module number
 * @param        ratio         the divide ratio from 1 to 32
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_tx_frame_rate(ssi_mod module, unsigned char ratio)
{
	int result = -1;

	if ((ratio >= SSI_MIN_FRAME_DIVIDER_RATIO) &&
	    (ratio <= SSI_MAX_FRAME_DIVIDER_RATIO)) {

		set_register_bits(SSI_FRAME_DIVIDER_MASK <<
				  SSI_FRAME_RATE_DIVIDER_SHIFT,
				  (ratio - 1) << SSI_FRAME_RATE_DIVIDER_SHIFT,
				  MXC_SSISTCCR, module);
		result = 0;
	}

	return result;
}

/*!
 * This function controls the Frame Sync active polarity for the transmit section.
 *
 * @param        module        the module number
 * @param        active        the Frame Sync active polarity
 */
void ssi_tx_frame_sync_active(ssi_mod module,
			      ssi_tx_rx_frame_sync_active active)
{
	set_register_bits(1 << SSI_FRAME_SYNC_INVERT_SHIFT,
			  active << SSI_FRAME_SYNC_INVERT_SHIFT,
			  MXC_SSISTCR, module);
}

/*!
 * This function controls the Frame Sync length (one word or one bit long) for the transmit section.
 *
 * @param        module        the module number
 * @param        length        the Frame Sync length
 */
void ssi_tx_frame_sync_length(ssi_mod module,
			      ssi_tx_rx_frame_sync_length length)
{
	set_register_bits(1 << SSI_FRAME_SYNC_LENGTH_SHIFT,
			  length << SSI_FRAME_SYNC_LENGTH_SHIFT,
			  MXC_SSISTCR, module);
}

/*!
 * This function configures the time slot(s) to mask for the transmit section.
 *
 * @param        module        the module number
 * @param        mask          the mask to indicate the time slot(s) masked
 */
void ssi_tx_mask_time_slot(ssi_mod module, unsigned int mask)
{
	set_register_bits(0xFFFFFFFF, mask, MXC_SSISTMSK, module);
}

/*!
 * This function configures the Prescale divider for the transmit section.
 *
 * @param        module        the module number
 * @param        divider       the divide ratio from 1 to  256
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_tx_prescaler_modulus(ssi_mod module, unsigned int divider)
{
	int result = -1;

	if ((divider >= SSI_MIN_PRESCALER_MODULUS_RATIO) &&
	    (divider <= SSI_MAX_PRESCALER_MODULUS_RATIO)) {

		set_register_bits(SSI_PRESCALER_MODULUS_MASK <<
				  SSI_PRESCALER_MODULUS_SHIFT,
				  (divider - 1) << SSI_PRESCALER_MODULUS_SHIFT,
				  MXC_SSISTCCR, module);
		result = 0;
	}

	return result;
}

/*!
 * This function controls whether the MSB or LSB will be transmitted first in a sample.
 *
 * @param        module        the module number
 * @param        direction     the shift direction
 */
void ssi_tx_shift_direction(ssi_mod module, ssi_tx_rx_shift_direction direction)
{
	set_register_bits(1 << SSI_SHIFT_DIRECTION_SHIFT,
			  direction << SSI_SHIFT_DIRECTION_SHIFT,
			  MXC_SSISTCR, module);
}

/*!
 * This function configures the Transmit word length.
 *
 * @param        module        the module number
 * @param        length        the word length
 */
void ssi_tx_word_length(ssi_mod module, ssi_word_length length)
{
	set_register_bits(SSI_WORD_LENGTH_MASK << SSI_WORD_LENGTH_SHIFT,
			  length << SSI_WORD_LENGTH_SHIFT,
			  MXC_SSISTCCR, module);
}


#ifdef SSI_TEST

static char ssi1_buf[BUF_SIZE];
static char ssi2_buf[BUF_SIZE];

struct ssi_priv_data{
	int ssi;
	int size;
	char* buf;
};

static struct ssi_priv_data ssi1_priv = {
	0,
	0,
	0
};

static struct ssi_priv_data ssi2_priv = {
	0,
	0,
	0
};

/*!
 * This function implements IOCTL controls on a SSI device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int ssi_ioctl(struct inode *inode, \
                     struct file *file, \
                     unsigned int cmd, \
                     unsigned long arg)
{
        int result = 0;

        if (_IOC_TYPE(cmd) != SSI_IOCTL)
                return -ENOTTY;

        switch (cmd) {
        case SSI_GET_BUFFER1:
                usr_data_depeche[0] = (unsigned char*)(arg);
                break;

        case SSI_GET_BUFFER2:
                usr_data_depeche[1] = (unsigned char*)(arg);
                break;

        case SSI_GET_BUFFER_SIZE1:
                usr_data_size[0] = (unsigned long)(arg); 
                break;

        case SSI_GET_BUFFER_SIZE2:
                usr_data_size[1] = (unsigned long)(arg); 
                break;

        case SSI_PLAY_MONO_DATA:
                mod = (ssi_mod)(arg);
                capture_flag = false;
                temp_ptr = usr_data_depeche[0];
                ssi_transmit_enable(mod, true);                            
               
                while((usr_data_size[0] > 0) && (temp_ptr != NULL)) 
                {
                    i = 0;

                    if (usr_data_size[0] < MONO_BUFFER_SIZE) 
                    {
                        copy_from_user (data_depeche[mod], temp_ptr, usr_data_size[0]);
                        data_size[mod] = usr_data_size[0];
                        usr_data_size[0] = 0;
		    }
                    else 
                    {
                        copy_from_user (data_depeche[mod], temp_ptr, MONO_BUFFER_SIZE);
                        data_size[mod] = MONO_BUFFER_SIZE;
                        usr_data_size[0] -= MONO_BUFFER_SIZE;
		    }

                    temp_ptr += data_size[mod];

                    if (data_size[mod] > 0) 
                    {
                        /* Enable transmit interrupts, TE, TFE */
                        ssi_interrupt_enable (mod, ssi_tx_fifo_0_empty | ssi_tx_interrupt_enable);    

                        /* Now go to sleep */
		        wait_event_interruptible (ssi1_wait_queue, data_size[mod] < 2);
                     }                   
		}

                ssi_interrupt_disable (mod, ssi_tx_fifo_0_empty | ssi_tx_interrupt_enable); 
                ssi_transmit_enable(mod, false);
                break;

        case SSI_PLAY_STEREO_DATA:
                mod = (ssi_mod)(arg);
                capture_flag = false;
                temp_ptr = usr_data_depeche[mod];

                ssi_transmit_enable (mod, true);

                while ((usr_data_size[mod] > 0) && (temp_ptr != NULL)) 
                {
                    if (data_size[0] < 4)
                    {
                        m = 0;

                        if (usr_data_size[mod] < STEREO_BUFFER_SIZE) 
                        {
                            copy_from_user (data_depeche[0], temp_ptr, usr_data_size[mod]);
                            data_size[0] = usr_data_size[mod];
                            usr_data_size[mod] = 0;
		        }
                        else 
                        {
                            copy_from_user (data_depeche[0], temp_ptr, STEREO_BUFFER_SIZE);
                            data_size[0] = STEREO_BUFFER_SIZE;
                            usr_data_size[mod] -= STEREO_BUFFER_SIZE;
		        }

                        temp_ptr += data_size[0];
		    }

                    if (data_size[1] < 4 && usr_data_size[mod] > 0)
                    {
                        m = 0;

                        if (usr_data_size[mod] < STEREO_BUFFER_SIZE) 
                        {
                            copy_from_user (data_depeche[1], temp_ptr, usr_data_size[mod]);
                            data_size[1] = usr_data_size[mod];
                            usr_data_size[mod] = 0;
		        }
                        else 
                        {
                            copy_from_user (data_depeche[1], temp_ptr, STEREO_BUFFER_SIZE);
                            data_size[1] = STEREO_BUFFER_SIZE;
                            usr_data_size[mod] -= STEREO_BUFFER_SIZE;
		        }

                        temp_ptr += data_size[1];
		    }                    

                    if (data_size[0] > 0 || data_size[1] > 0) 
                    {
                        /* Enable transmit interrupts, TE, TFE */
                        ssi_interrupt_enable (mod, ssi_tx_fifo_1_empty | ssi_tx_interrupt_enable);   
                    }

                    if (data_size[0] > 0 && data_size[1] > 0) 
                    {
                        /* Now go to sleep */
	     	        wait_event_interruptible (ssi2_wait_queue, ((data_size[0] < 4) || 
                                                                    (data_size[1] < 4)) );
                    }
 		}
            
                break;

        case SSI_RECORD_DATA:
                mod = (ssi_mod)(arg);
                if(capture_flag == false) {
                  capture_flag = true;
                  record_buffering = true;
                  uRdIndx = 0;
                  uWrIndx = 0;
                  ssi_receive_enable (SSI1, true);
                  ssi_interrupt_enable (SSI1, ssi_rx_fifo_0_full | ssi_rx_interrupt_enable);
                }
                temp_ptr = usr_data_depeche[0];

                while ((usr_data_size[0] > 0) && (temp_ptr != NULL)) 
                {                                       
                    data_size[1] = 0;

                    if(record_buffering == true) {
                      data_size[1] = 2*usr_data_size[0];
                      wait_event_interruptible (ssi1_wait_queue, i >= data_size[1]);
                      record_buffering = false;
                    }
                    /* printk("FYDBG:uRdIndx=%ld uWrIndx=%ld\n",uRdIndx, uWrIndx); */
                    if(uRdIndx < uWrIndx) {
                      if(uWrIndx-uRdIndx >= usr_data_size[0]) {
                        copy_to_user (temp_ptr, data_depeche[1]+uRdIndx, usr_data_size[0]);
                        uRdIndx += usr_data_size[0];
                        usr_data_size[0] = 0;
                      }
                      else
                        data_size[1] = usr_data_size[0];
                    }
                    else if(uRdIndx > uWrIndx) {
                      if((STEREO_BUFFER_SIZE-uRdIndx) >= usr_data_size[0]) {
                        copy_to_user (temp_ptr, data_depeche[1]+uRdIndx, usr_data_size[0]);
                        uRdIndx += usr_data_size[0];
                        usr_data_size[0] = 0;
                      }
                      else if((STEREO_BUFFER_SIZE-uRdIndx+uWrIndx) >= usr_data_size[0]) {
                        copy_to_user (temp_ptr, data_depeche[1]+uRdIndx, STEREO_BUFFER_SIZE-uRdIndx);
                        temp_ptr += (STEREO_BUFFER_SIZE-uRdIndx);
                        copy_to_user (temp_ptr, data_depeche[1], usr_data_size[0]-(STEREO_BUFFER_SIZE-uRdIndx));
                        uRdIndx = usr_data_size[0]-(STEREO_BUFFER_SIZE-uRdIndx);
                        usr_data_size[0] = 0;
                      }
                      else
                        data_size[1] = usr_data_size[0];
                    }
                    else {
                      data_size[1] = usr_data_size[0];
                    }
                    if(data_size[1] != 0)
                      wait_event_interruptible (ssi1_wait_queue, i >= data_size[1]);
                }
                break;

        case SSI_PLAY_MIX:
                break;

        default:
                break;
        }

        return result;
}

int write_fifo(struct ssi_priv_data* priv)
{
        int i, left, right;
        int data_size;

	i = 0;	
	data_size = priv->size;
        ssi_transmit_enable(priv->ssi, true);
        
        while(data_size >= 4){
	        if ((getreg_value(MXC_SSISFCSR, priv->ssi) & 0xF00) <= 0x600){
        	        left =(unsigned int)(((priv->buf[i + 1] << 8) & 0xFF00)|
                                                     (priv->buf[i] & 0x00FF));
                        right =(unsigned int)(((priv->buf[i + 3] << 8) & 0xFF00)|
                                                     (priv->buf[i + 2] & 0x00FF));
                        ssi_set_data(priv->ssi, ssi_fifo_0, left);
                        ssi_set_data(priv->ssi, ssi_fifo_1, right);
                        data_size -= 4;
                        i += 4;
		}
	}

        ssi_transmit_enable(priv->ssi, false);
        return priv->size;
}

/*!
 * This function implements the open method on a SSI device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int ssi_open(struct inode *inode, struct file *file)
{
    /* DBG_PRINTK("ssi : ssi_open()\n"); */
    int res = 0;

    unsigned int minor = MINOR(inode->i_rdev);
    struct ssi_priv_data* priv;
	                
    priv = (minor == 0) ? &ssi1_priv : &ssi2_priv;
    priv->buf = (minor == 0) ? &ssi1_buf[0] : &ssi2_buf[0];
        	
    priv->ssi = minor;
    priv->size = 0;
        
    file->private_data = (void*)priv;

    if(open_count == 0) {
      /* Irq related stuff */
      data_depeche[0] = (unsigned char *)kmalloc(STEREO_BUFFER_SIZE, GFP_KERNEL);
      data_depeche[1] = (unsigned char *)kmalloc(STEREO_BUFFER_SIZE, GFP_KERNEL);

      if(!data_depeche[0] || !data_depeche[1]) 
      {
        return -ENOBUFS;
      }

      data_size[0] = 0;
      data_size[1] = 0;
      usr_data_depeche[0] = 0;
      usr_data_depeche[1] = 0;
      usr_data_size[0] = 0;
      usr_data_size[1] = 0;

      buf_index = 0;
      i = 0;

      init_waitqueue_head (&ssi1_wait_queue);
      init_waitqueue_head (&ssi2_wait_queue);

      /* Requesting irqs INT_SSI1 (11) and registering the interrupt handler */
      /* TODO: request INT_SSI1 or INT_SSI2 depending on whether /dev/ssi1 or /dev/ssi2 was opened */
      res = request_irq (INT_SSI1, ssi1_int_handler, 0, "ssi1", NULL);

      res = request_irq (INT_SSI2, ssi2_int_handler, 0, "ssi2", NULL);
    }
    open_count += 1;

    return res;
}

/*!
 * This function implements the release method on a SSI device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int ssi_free(struct inode *inode, struct file *file)
{
    struct ssi_priv_data* priv;

    if(open_count <=0) return 0;

    priv = (struct ssi_priv_data*)file->private_data;
    file->private_data = NULL;

    if(open_count == 1) {
      ssi_interrupt_disable (mod, ssi_tx_fifo_1_empty | ssi_tx_interrupt_enable);
      ssi_transmit_enable (mod, false);
      ssi_interrupt_disable (mod, ssi_rx_fifo_0_full | ssi_rx_interrupt_enable);
      ssi_receive_enable (mod, false);

      free_irq (INT_SSI1, NULL);
      free_irq (INT_SSI2, NULL);

      data_size[0] = 0;
      data_size[1] = 0;

      if(data_depeche[0]) {
        kfree(data_depeche[0]);
      }

      if(data_depeche[1]) {
        kfree(data_depeche[1]);
      }

      data_depeche[0] = 0;
      data_depeche[1] = 0;
      usr_data_depeche[0] = 0;
      usr_data_depeche[1] = 0;
      usr_data_size[0] = 0;
      usr_data_size[1] = 0;
    }
    open_count -= 1;

    return 0;
}

static ssize_t ssi_write(struct file* file, const char* buf, 
                               size_t bytes, loff_t* off)
{
	struct ssi_priv_data* priv;
	unsigned int minor;
	int res = 0;
	
	minor = MINOR(file->f_dentry->d_inode->i_rdev);
	priv = (struct ssi_priv_data*)file->private_data;
		
	res = copy_from_user(priv->buf, (void*)buf, bytes);
	if(res){
		printk("write: fault error when copying user data\n");
		return -EFAULT;
	}
	
	priv->size = bytes;
	return write_fifo(priv);
}

static ssize_t ssi_read(struct file* file, char* buf, 
                              size_t bytes, loff_t* off)
{
	struct ssi_priv_data* priv;
	unsigned int minor;
	
	minor = MINOR(file->f_dentry->d_inode->i_rdev);
	
	priv = (struct ssi_priv_data*)file->private_data;
	
	return 0;
}

/*!
 * This structure defines file operations for a SSI device.
 */
static struct file_operations ssi_fops = {
      /*!
       * the owner
       */
        .owner =        THIS_MODULE,
        
        .read  = 	ssi_read,
        
        .write = 	ssi_write,
        
      /*!
       * the ioctl operations
       */
        .ioctl =        ssi_ioctl,
        
      /*!
       * the open operation
       */
        .open =         ssi_open,
        
      /*!
       * the release operation
       */
        .release =      ssi_free,
};
#endif

/*!
 * This function implements the init function of the SSI device.
 * This function is called when the module is loaded.
 *
 * @return       This function returns 0.
 */
static int __init ssi_init(void)
{
#ifdef SSI_TEST
        printk(KERN_DEBUG "ssi : ssi_init(void) \n");

        major_ssi1 = register_chrdev(0,"ssi1", &ssi_fops);
        if (major_ssi1 < 0)
        {
                printk(KERN_WARNING "Unable to get a major for ssi1");
                return major_ssi1;
        }

        major_ssi2 = register_chrdev(0,"ssi2", &ssi_fops);

        if (major_ssi2 < 0)
        {
                printk(KERN_WARNING "Unable to get a major for ssi2");
                return major_ssi2;
        }

        printk("ssi : creating devfs entry for ssi1 \n");
        devfs_mk_cdev(MKDEV(major_ssi1,0), \
                      S_IFCHR | S_IRUGO | S_IWUSR, "ssi1" );
        printk("ssi : creating devfs entry for ssi2 \n");
        devfs_mk_cdev(MKDEV(major_ssi2,0), \
                      S_IFCHR | S_IRUGO | S_IWUSR, "ssi2" );

#endif
	return 0;
}

/*!
 * This function implements the exit function of the SPI device.
 * This function is called when the module is unloaded.
 *
 */
static void __exit ssi_exit(void)
{
#ifdef SSI_TEST
        devfs_remove("ssi1");
        unregister_chrdev(major_ssi1, "ssi1");

        devfs_remove("ssi2");
        unregister_chrdev(major_ssi2, "ssi2");

        printk(KERN_DEBUG "ssi : successfully unloaded\n");
#endif
}

module_init(ssi_init);
module_exit(ssi_exit);

MODULE_DESCRIPTION("SSI char device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
