/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2006  Motorola, Inc.
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
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ---------------------------
 * 04/28/2006   Motorola  Fixed bug causing audio data loss during A/V capture 
 *
 */


 /*!
  * @defgroup SSI Synchronous Serial Interface (SSI) Driver
  */

 /*!
  * @file ssi.h
  * @brief This header file contains SSI driver functions prototypes.
  *
  * @ingroup SSI
  */

#ifndef __SSI_H__
#define __SSI_H__
#define SSI_TEST 1

#include "ssi_types.h"

#ifdef SSI_TEST

#define SSI_IOCTL             0x54
#define SSI_CONFIG_FOR_ATLAS _IOWR(SSI_IOCTL, 1, wave_configuration)
#define SSI_GET_BUFFER1      _IOWR(SSI_IOCTL, 2, unsigned int)
#define SSI_GET_BUFFER2      _IOWR(SSI_IOCTL, 3, unsigned int)
#define SSI_GET_BUFFER_SIZE1 _IOWR(SSI_IOCTL, 4, unsigned long)
#define SSI_GET_BUFFER_SIZE2 _IOWR(SSI_IOCTL, 5, unsigned long)
#define SSI_PLAY_STEREO_DATA _IOWR(SSI_IOCTL, 6, int)
#define SSI_PLAY_MIX         _IOWR(SSI_IOCTL, 7, int)
#define SSI_PLAY_MONO_DATA   _IOWR(SSI_IOCTL, 8, int)
#define SSI_RECORD_DATA      _IOWR(SSI_IOCTL, 9, int)

#define BUF_SIZE	     1024 * 8

/*!
 * Wave file configuration.
 */
typedef struct {
        /*!
         * Mode test
         */
        unsigned short mode_test;
        /*!
         * SSI
         */
        unsigned short id_SSI;
        /*!
         * Number of channels (1:MONO, 2:STEREO)
         */
        unsigned short num_channels;
        /*!
         * Sample rate
         */
        unsigned long sample_rate;
        /*!
         * Bits per sample
         */
        unsigned short bits_per_sample;
        /*!
         * Sample size
         */
        unsigned long sample_size;
} wave_configuration;

#endif

/*!
 * This function controls the AC97 frame rate divider.
 *
 * @param        module               the module number
 * @param        frame_rate_divider   the AC97 frame rate divider
 */
void ssi_ac97_frame_rate_divider(ssi_mod module,
				 unsigned char frame_rate_divider);

/*!
 * This function gets the AC97 command address register.
 *
 * @param        module        the module number
 * @return       This function returns the command address slot information.
 */
unsigned int ssi_ac97_get_command_address_register(ssi_mod module);

/*!
 * This function gets the AC97 command data register.
 *
 * @param        module        the module number
 * @return       This function returns the command data slot information.
 */
unsigned int ssi_ac97_get_command_data_register(ssi_mod module);

/*!
 * This function gets the AC97 tag register.
 *
 * @param        module        the module number
 * @return       This function returns the tag information.
 */
unsigned int ssi_ac97_get_tag_register(ssi_mod module);

/*!
 * This function controls the AC97 mode.
 *
 * @param        module        the module number
 * @param        state         the AC97 mode state (enabled or disabled)
 */
void ssi_ac97_mode_enable(ssi_mod module, bool state);

/*!
 * This function controls the AC97 tag in FIFO behavior.
 *
 * @param        module        the module number
 * @param        state         the tag in fifo behavior (Tag info stored in Rx FIFO 0 if TRUE, 
 * Tag info stored in SATAG register otherwise)
 */
void ssi_ac97_tag_in_fifo(ssi_mod module, bool state);

/*!
 * This function controls the AC97 read command.
 *
 * @param        module        the module number
 * @param        state         the next AC97 command is a read command or not
 */
void ssi_ac97_read_command(ssi_mod module, bool state);

/*!
 * This function sets the AC97 command address register.
 *
 * @param        module        the module number
 * @param        address       the command address slot information
 */
void ssi_ac97_set_command_address_register(ssi_mod module,
					   unsigned int address);

/*!
 * This function sets the AC97 command data register.
 *
 * @param        module        the module number
 * @param        data          the command data slot information
 */
void ssi_ac97_set_command_data_register(ssi_mod module, unsigned int data);

/*!
 * This function sets the AC97 tag register.
 *
 * @param        module        the module number
 * @param        tag           the tag information
 */
void ssi_ac97_set_tag_register(ssi_mod module, unsigned int tag);

/*!
 * This function controls the AC97 variable mode.
 *
 * @param        module        the module number
 * @param        state         the AC97 variable mode state (enabled or disabled)
 */
void ssi_ac97_variable_mode(ssi_mod module, bool state);

/*!
 * This function controls the AC97 write command.
 *
 * @param        module        the module number
 * @param        state         the next AC97 command is a write command or not
 */
void ssi_ac97_write_command(ssi_mod module, bool state);

/*!
 * This function controls the idle state of the transmit clock port during SSI internal gated mode.
 *
 * @param        module        the module number
 * @param        state         the clock idle state
 */
void ssi_clock_idle_state(ssi_mod module, idle_state state);

/*!
 * This function turns off/on the ccm_ssi_clk to reduce power consumption.
 *
 * @param        module        the module number
 * @param        state         the state for ccm_ssi_clk (true: turn off, else:turn on)
 */
void ssi_clock_off(ssi_mod module, bool state);

/*!
 * This function enables/disables the SSI module.
 *
 * @param        module        the module number
 * @param        state         the state for SSI module
 */
void ssi_enable(ssi_mod module, bool state);

/*!
 * This function gets the data word in the Receive FIFO of the SSI module.
 *
 * @param        module        the module number
 * @param        fifo          the Receive FIFO to read
 * @return       This function returns the read data.
 */
unsigned int ssi_get_data(ssi_mod module, fifo_nb fifo);

/*!
 * This function returns the status of the SSI module (SISR register) as a combination of status.
 *
 * @param        module        the module number
 * @return       This function returns the status of the SSI module.
 */
ssi_status_enable_mask ssi_get_status(ssi_mod module);

/*!
 * This function selects the I2S mode of the SSI module.
 *
 * @param        module        the module number
 * @param        mode          the I2S mode 
 */
void ssi_i2s_mode(ssi_mod module, mode_i2s mode);

/*!
 * This function disables the interrupts of the SSI module.
 *
 * @param        module        the module number
 * @param        mask          the mask of the interrupts to disable 
 */
void ssi_interrupt_disable(ssi_mod module, ssi_status_enable_mask mask);

/*!
 * This function enables the interrupts of the SSI module.
 *
 * @param        module        the module number
 * @param        mask          the mask of the interrupts to enable 
 */
void ssi_interrupt_enable(ssi_mod module, ssi_status_enable_mask mask);

/*!
 * This function enables/disables the network mode of the SSI module.
 *
 * @param        module        the module number
 * @param        state         the network mode state 
 */
void ssi_network_mode(ssi_mod module, bool state);

/*!
 * This function enables/disables the receive section of the SSI module.
 *
 * @param        module        the module number
 * @param        state         the receive section state 
 */
void ssi_receive_enable(ssi_mod module, bool state);

/*!
 * This function configures the SSI module to receive data word at bit position 0 or 23 in the Receive shift register.
 *
 * @param        module        the module number
 * @param        state         the state to receive at bit 0 
 */
void ssi_rx_bit0(ssi_mod module, bool state);

/*!
 * This function controls the source of the clock signal used to clock the Receive shift register.
 *
 * @param        module        the module number
 * @param        direction     the clock signal direction 
 */
void ssi_rx_clock_direction(ssi_mod module, ssi_tx_rx_direction direction);

/*!
 * This function configures the divide-by-two divider of the SSI module for the receive section.
 *
 * @param        module        the module number
 * @param        state         the divider state 
 */
void ssi_rx_clock_divide_by_two(ssi_mod module, bool state);

/*!
 * This function controls which bit clock edge is used to clock in data.
 *
 * @param        module        the module number
 * @param        polarity      the clock polarity 
 */
void ssi_rx_clock_polarity(ssi_mod module, ssi_tx_rx_clock_polarity polarity);

/*!
 * This function configures a fixed divide-by-eight clock prescaler divider of the SSI module in series with the variable prescaler for the receive section.
 *
 * @param        module        the module number
 * @param        state         the prescaler state 
 */
void ssi_rx_clock_prescaler(ssi_mod module, bool state);

/*!
 * This function controls the early frame sync configuration.
 *
 * @param        module        the module number
 * @param        early         the early frame sync configuration 
 */
void ssi_rx_early_frame_sync(ssi_mod module, ssi_tx_rx_early_frame_sync early);

/*!
 * This function gets the number of data words in the Receive FIFO.
 *
 * @param        module        the module number
 * @param        fifo          the fifo 
 * @return       This function returns the number of words in the Rx FIFO.
 */
unsigned char ssi_rx_fifo_counter(ssi_mod module, fifo_nb fifo);

/*!
 * This function enables the Receive FIFO.
 *
 * @param        module        the module number
 * @param        fifo          the fifo to enable
 * @param        enabled       the state of the fifo, enabled or disabled 
 */
void ssi_rx_fifo_enable(ssi_mod module, fifo_nb fifo, bool enabled);

/*!
 * This function controls the threshold at which the RFFx flag will be set.
 *
 * @param        module        the module number
 * @param        fifo          the fifo to enable
 * @param        watermark     the watermark
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_rx_fifo_full_watermark(ssi_mod module,
			       fifo_nb fifo, unsigned char watermark);

/*!
 * This function flushes the Receive FIFOs.
 *
 * @param        module        the module number
 */
void ssi_rx_flush_fifo(ssi_mod module);

/*!
 * This function controls the direction of the Frame Sync signal for the receive section.
 *
 * @param        module        the module number
 * @param        direction     the Frame Sync signal direction
 */
void ssi_rx_frame_direction(ssi_mod module, ssi_tx_rx_direction direction);

/*!
 * This function configures the Receive frame rate divider for the receive section.
 *
 * @param        module        the module number
 * @param        ratio         the divide ratio from 1 to 32
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_rx_frame_rate(ssi_mod module, unsigned char ratio);

/*!
 * This function controls the Frame Sync active polarity for the receive section.
 *
 * @param        module        the module number
 * @param        active        the Frame Sync active polarity
 */
void ssi_rx_frame_sync_active(ssi_mod module,
			      ssi_tx_rx_frame_sync_active active);

/*!
 * This function controls the Frame Sync length (one word or one bit long) for the receive section.
 *
 * @param        module        the module number
 * @param        length        the Frame Sync length
 */
void ssi_rx_frame_sync_length(ssi_mod module,
			      ssi_tx_rx_frame_sync_length length);

/*!
 * This function configures the time slot(s) to mask for the receive section.
 *
 * @param        module        the module number
 * @param        mask          the mask to indicate the time slot(s) masked
 */
void ssi_rx_mask_time_slot(ssi_mod module, unsigned int mask);

/*!
 * This function configures the Prescale divider for the receive section.
 *
 * @param        module        the module number
 * @param        divider       the divide ratio from 1 to  256
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_rx_prescaler_modulus(ssi_mod module, unsigned int divider);

/*!
 * This function controls whether the MSB or LSB will be received first in a sample.
 *
 * @param        module        the module number
 * @param        direction     the shift direction
 */
void ssi_rx_shift_direction(ssi_mod module,
			    ssi_tx_rx_shift_direction direction);

/*!
 * This function configures the Receive word length.
 *
 * @param        module        the module number
 * @param        length        the word length
 */
void ssi_rx_word_length(ssi_mod module, ssi_word_length length);

/*!
 * This function sets the data word in the Transmit FIFO of the SSI module.
 *
 * @param        module        the module number
 * @param        fifo          the FIFO number
 * @param        data          the data to load in the FIFO
 */
void ssi_set_data(ssi_mod module, fifo_nb fifo, unsigned int data);

/*!
 * This function controls the number of wait states between the core and SSI.
 *
 * @param        module        the module number
 * @param        wait          the number of wait state(s)
 */
void ssi_set_wait_states(ssi_mod module, ssi_wait_states wait);

/*!
 * This function enables/disables the synchronous mode of the SSI module.
 *
 * @param        module        the module number
 * @param        state         the synchronous mode state
 */
void ssi_synchronous_mode(ssi_mod module, bool state);

/*!
 * This function allows the SSI module to output the SYS_CLK at the SRCK port.
 *
 * @param        module        the module number
 * @param        state         the system clock state
 */
void ssi_system_clock(ssi_mod module, bool state);

/*!
 * This function enables/disables the transmit section of the SSI module.
 *
 * @param        module        the module number
 * @param        state         the transmit section state
 */
void ssi_transmit_enable(ssi_mod module, bool state);

/*!
 * This function allows the SSI module to operate in the two channel mode.
 *
 * @param        module        the module number
 * @param        state         the two channel mode state
 */
void ssi_two_channel_mode(ssi_mod module, bool state);

/*!
 * This function configures the SSI module to transmit data word from bit position 0 or 23 in the Transmit shift register.
 *
 * @param        module        the module number
 * @param        state         the transmit from bit 0 state 
 */
void ssi_tx_bit0(ssi_mod module, bool state);

/*!
 * This function controls the direction of the clock signal used to clock the Transmit shift register.
 *
 * @param        module        the module number
 * @param        direction     the clock signal direction 
 */
void ssi_tx_clock_direction(ssi_mod module, ssi_tx_rx_direction direction);

/*!
 * This function configures the divide-by-two divider of the SSI module for the transmit section.
 *
 * @param        module        the module number
 * @param        state         the divider state 
 */
void ssi_tx_clock_divide_by_two(ssi_mod module, bool state);

/*!
 * This function controls which bit clock edge is used to clock out data.
 *
 * @param        module        the module number
 * @param        polarity      the clock polarity 
 */
void ssi_tx_clock_polarity(ssi_mod module, ssi_tx_rx_clock_polarity polarity);

/*!
 * This function configures a fixed divide-by-eight clock prescaler divider of the SSI module in series with the variable prescaler for the transmit section.
 *
 * @param        module        the module number
 * @param        state         the prescaler state 
 */
void ssi_tx_clock_prescaler(ssi_mod module, bool state);

/*!
 * This function controls the early frame sync configuration for the transmit section.
 *
 * @param        module        the module number
 * @param        early         the early frame sync configuration 
 */
void ssi_tx_early_frame_sync(ssi_mod module, ssi_tx_rx_early_frame_sync early);

/*!
 * This function gets the number of data words in the Transmit FIFO.
 *
 * @param        module        the module number
 * @param        fifo          the fifo 
 * @return       This function returns the number of words in the Tx FIFO.
 */
unsigned char ssi_tx_fifo_counter(ssi_mod module, fifo_nb fifo);

/*!
 * This function controls the threshold at which the TFEx flag will be set.
 *
 * @param        module        the module number
 * @param        fifo          the fifo to enable
 * @param        watermark     the watermark
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_tx_fifo_empty_watermark(ssi_mod module, fifo_nb fifo,
				unsigned char watermark);

/*!
 * This function enables the Transmit FIFO.
 *
 * @param        module        the module number
 * @param        fifo          the fifo to enable
 * @param        enable        the state of the FIFO, enabled or disabled
 */
void ssi_tx_fifo_enable(ssi_mod module, fifo_nb fifo, bool enable);

/*!
 * This function flushes the Transmit FIFOs.
 *
 * @param        module        the module number
 */
void ssi_tx_flush_fifo(ssi_mod module);

/*!
 * This function controls the direction of the Frame Sync signal for the transmit section.
 *
 * @param        module        the module number
 * @param        direction     the Frame Sync signal direction
 */
void ssi_tx_frame_direction(ssi_mod module, ssi_tx_rx_direction direction);

/*!
 * This function configures the Transmit frame rate divider.
 *
 * @param        module        the module number
 * @param        ratio         the divide ratio from 1 to 32
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_tx_frame_rate(ssi_mod module, unsigned char ratio);

/*!
 * This function controls the Frame Sync active polarity for the transmit section.
 *
 * @param        module        the module number
 * @param        active        the Frame Sync active polarity
 */
void ssi_tx_frame_sync_active(ssi_mod module,
			      ssi_tx_rx_frame_sync_active active);

/*!
 * This function controls the Frame Sync length (one word or one bit long) for the transmit section.
 *
 * @param        module        the module number
 * @param        length        the Frame Sync length
 */
void ssi_tx_frame_sync_length(ssi_mod module,
			      ssi_tx_rx_frame_sync_length length);

/*!
 * This function configures the time slot(s) to mask for the transmit section.
 *
 * @param        module        the module number
 * @param        mask          the mask to indicate the time slot(s) masked
 */
void ssi_tx_mask_time_slot(ssi_mod module, unsigned int mask);

/*!
 * This function configures the Prescale divider for the transmit section.
 *
 * @param        module        the module number
 * @param        divider       the divide ratio from 1 to  256
 * @return       This function returns the result of the operation (0 if successful, -1 otherwise).
 */
int ssi_tx_prescaler_modulus(ssi_mod module, unsigned int divider);

/*!
 * This function controls whether the MSB or LSB will be transmited first in a sample.
 *
 * @param        module        the module number
 * @param        direction     the shift direction
 */
void ssi_tx_shift_direction(ssi_mod module,
			    ssi_tx_rx_shift_direction direction);

/*!
 * This function configures the Transmit word length.
 *
 * @param        module        the module number
 * @param        length        the word length
 */
void ssi_tx_word_length(ssi_mod module, ssi_word_length length);

#endif				//__SSI_H__
