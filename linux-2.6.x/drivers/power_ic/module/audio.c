/*
 * Copyright (C) 2004-2008 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
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
 * Motorola 2008-Feb-18 - Support for external audio amplifier
 * Motorola 2007-Mar-15 - Always keep track of the last accessory mode
 * Motorola 2007-Jan-08 - Remove setting of stereo dac and codec clocks 
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Oct-11 - New Audio interfaces.
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Oct-04 - Support audio output path for linear vibrator.
 * Motorola 2006-Aug-28 - Add Codec Clock and Stereo DAC Clock support.
 * Motorola 2006-Aug-23 - Use msleep instead of mdelay.
 * Motorola 2006-Aug-21 - Fix to disable stereo DAC in call.
 * Motorola 2006-Aug-16 - Use configuration flag for Ceramic Speaker support.
 * Motorola 2006-Aug-09 - Fix error code for power_ic_audio_power_off
 * Motorola 2006-Aug-03 - Add Mono Adder and Balance Control Support
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-25 - Remove vaudio_on calls, done by AM now
 * Motorola 2006-Jun-28 - Montavista header upmerge changes
 * Motorola 2006-Jun-27 - Add support for ceramic speaker
 * Motorola 2006-Jun-19 - Fix montavista upmerge conditionals
 * Motorola 2006-Jun-06 - Reimplement old EMU algorithm
 * Motorola 2006-May-31 - Fix Downlink Audio after call answered while playing a multimedia file
 * Motorola 2006-May-15 - Moved variable declaration to start of function.
 * Motorola 2006-May-06 - Implement EMU Common Algorithm
 * Motorola 2006-Apr-17 - Add support for Mono BT
 * Motorola 2006-Mar-23 - Fix STDAC issue
 * Motorola 2006-Feb-10 - Add ArgonLV support
 * Motorola 2006-Jan-23 - STAC workaround for SPI Sequence
 * Motorola 2006-Jan-17 - EMU Accessory TX Audio change
 * Motorola 2006-Jan-17 - CODEC and STDAC SPI Sequence Changes
 * Motorola 2005-Nov-22 - Finalized code and clean up.
 * Motorola 2005-Jun-09 - Tuned stereo DAC and CODEC
 * Motorola 2004-Feb-17 - Initial Creation
 */

/*!
 * @file audio.c
 *
 * @ingroup poweric_audio
 *
 * @brief Power IC audio module
 *
 * This file contains all of the functions and data structures required to implement the
 * interface module between audio manager from user space and power IC driver. This audio
 * module gives the user more flexibility to control the audio registers.
 */

/*=================================================================================================
                                        INCLUDE FILES
==================================================================================================*/
#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/moto_accy.h>
#include <linux/power_ic_audio.h>
#include <linux/power_ic_kernel.h>
#include <linux/module.h>
#include <stdbool.h>

#ifdef CONFIG_ARCH_MXC91321
#include <asm/arch/mxc91321.h>
#elif defined(CONFIG_ARCH_MXC91231)
#include <asm/arch/mxc91231.h>
#endif

#include "audio.h"
#include "../core/gpio.h"
#include "../core/os_independent.h"

#include "emu_glue_utils.h" 
#include "emu_atlas.h"

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
                                     LOCAL CONSTANTS
==================================================================================================*/

/*! @name Ceramic speaker masks.
 * These values define the masks used to change the mode of the ceramic speaker.
 */
/*@{*/
/*! defines for ceramic speaker */
#define CERAMIC_SPEAKER_ENABLE       0x000100
#define CERAMIC_SPEAKER_DISABLE      0x000000
/*@}*/

/*! @brief Define for Atlas GPO2 bit. */
#define ATLAS_GPO2_INDEX    8

/*==================================================================================================
                                        LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL VARIABLES
==================================================================================================*/
/*! @brief Keeps track of audio configuration. */
AUDIO_CONFIG_T audio_config;

/*==================================================================================================
                                     EXPORTED SYMBOLS
==================================================================================================*/
#ifndef DOXYGEN_SHOULD_SKIP_THIS
EXPORT_SYMBOL(power_ic_audio_conn_mode_set);
#ifdef CONFIG_CERAMICSPEAKER
EXPORT_SYMBOL(power_ic_audio_ceramic_speaker_en);
#endif
EXPORT_SYMBOL(power_ic_audio_set_reg_mask_audio_rx_0);
EXPORT_SYMBOL(power_ic_audio_set_reg_mask_audio_rx_1);
EXPORT_SYMBOL(power_ic_audio_set_reg_mask_audio_tx);
EXPORT_SYMBOL(power_ic_audio_set_reg_mask_ssi_network);
EXPORT_SYMBOL(power_ic_audio_set_reg_mask_audio_codec);
EXPORT_SYMBOL(power_ic_audio_set_reg_mask_audio_stereo_dac);
EXPORT_SYMBOL(power_ic_audio_ext_audio_amp_en);
#endif

/*==================================================================================================
                                     Local Functions
==================================================================================================*/

/*==================================================================================================
                                     GLOBAL FUNCTIONS
==================================================================================================*/
/*!
 * @brief Set up the conn mode for EMU
 * This function configures the conn mode for EMU Headsets
 *
 * @param  conn_mode      connection mode
 *
 * @return returns 0 if successful.
 */

int power_ic_audio_conn_mode_set(POWER_IC_EMU_CONN_MODE_T conn_mode)
{
    MOTO_ACCY_MASK_T connected_device = moto_accy_get_all_devices();

    /* If the connection mode is stereo audio, set the headset to stereo mode. */
    if(conn_mode == POWER_IC_EMU_CONN_MODE_STEREO_AUDIO)
    {
        /* Only allow the conn mode to be set to stereo if a stereo headset is connected */
        if (ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_HEADSET_EMU_STEREO))
        {
            /* Save configuration */
            audio_config.headset_mode = MOTO_ACCY_HEADSET_MODE_STEREO;
        
            if (power_ic_emu_hw_locked == false)
        {
            /* Apply configuration */
                emu_util_set_emu_headset_mode(audio_config.headset_mode);
        }
    }
    }
    /* For external mic, the conn mode is set to mono audio. */
    else if(conn_mode == POWER_IC_EMU_CONN_MODE_MONO_AUDIO)
    {
        /* If any type of EMU headset connected, we need to set the headset mode to mono. */
        if ((ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_HEADSET_EMU_MONO)) ||
            (ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_HEADSET_EMU_STEREO)))
        {
            /* Save configuration */
            audio_config.headset_mode = MOTO_ACCY_HEADSET_MODE_MONO;
            
            if (power_ic_emu_hw_locked == false)
            {
                /* Apply configuration */
                emu_util_set_emu_headset_mode(audio_config.headset_mode);
            }
        }
        else if ((ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_CARKIT_MID)) ||
                 (ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_CARKIT_FAST)))
        {
            /* Save configuration */
            audio_config.conn_mode = POWER_IC_EMU_CONN_MODE_MONO_AUDIO;
            
            if (power_ic_emu_hw_locked == false)
            {
                /* Apply configuration */
                EMU_SET_EMU_CONN_MODE(audio_config.conn_mode);
            }
        }
    }
    /* For everything else, set the conn mode to None, which will be USB. */
    else
    {
        /* If any type of EMU headset connected, we need to set the headset mode to none. */
        if ((ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_HEADSET_EMU_MONO)) ||
            (ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_HEADSET_EMU_STEREO)))
        {
            /* Save configuration */
                audio_config.headset_mode = MOTO_ACCY_HEADSET_MODE_NONE;
            
            if (power_ic_emu_hw_locked == false)
            {
                /* Apply configuration */
                emu_util_set_emu_headset_mode(audio_config.headset_mode);
            }
        }
        else
        {
            /* Save configuration */
                audio_config.conn_mode = POWER_IC_EMU_CONN_MODE_USB;
            
            if (power_ic_emu_hw_locked == false)
            {
                /* Apply configuration */ 
                EMU_SET_EMU_CONN_MODE(audio_config.conn_mode);
            }
        }
    }

    return 0;
}

#ifdef CONFIG_CERAMICSPEAKER
/*!
 * @brief Enable or disable the ceramic speaker.
 * This function will enable or disable the ceramic speaker based on the input (enable=true and
 * disable=false).
 *
 * @param  en_val   True or false to respectively enable or disable the ceramic speaker.
 *
 * @return Returns 0 if successful.
 */

int power_ic_audio_ceramic_speaker_en(bool en_val)
{
    int reg_val;

    /* Enable or disable the ceramic speaker bit(s). */
    reg_val = (en_val) ? CERAMIC_SPEAKER_ENABLE : CERAMIC_SPEAKER_DISABLE;

    /* Write the new register value, and save pass/fail to reg_val. */
    reg_val = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_PWR_MISC, CERAMIC_SPEAKER_ENABLE, reg_val);

    /* Check for a failure on writing the new register value. */
    if (reg_val != 0)
    {
        /* Display a meaningful message on error. */
        if (en_val == 1)
        {
            printk("POWER IC AUDIO: Error in turning on ceramic speaker.\n");
        }
        else
        {
            printk("POWER IC AUDIO: Error in turning off ceramic speaker.\n");
        }
    }

    return reg_val;
}
#endif /* CONFIG_CERAMICSPEAKER */

/*!
 * @brief Set the values of the Audio Rx 0 register bits that are used by Audio.
 * This function is called in order to set the Audio Rx 0 register bits in the provided mask that
 * are used by Audio to the provided values.
 *
 * @param  mask     Bitmask indicating which bits are to be modified.
 * @param  value    New values for modified bits.
 *
 * @return Returns 0 if successful.
 */

int power_ic_audio_set_reg_mask_audio_rx_0(unsigned int mask, unsigned int value)
{
    /* The following bits are protected and should not be modified using this function. */
    mask &= ~(POWER_IC_AUDIO_REG_AUDIO_RX_0_BIT20_RESERVED
              | POWER_IC_AUDIO_REG_AUDIO_RX_0_HSDETEN
              | POWER_IC_AUDIO_REG_AUDIO_RX_0_HSDETAUTOB
              | POWER_IC_AUDIO_REG_AUDIO_RX_0_HSLDETEN);

    /* Write the new register bit values. */
    return power_ic_set_reg_mask(POWER_IC_REG_ATLAS_AUDIO_RX_0, mask, value);
}

/*!
 * @brief Set the values of the Audio Rx 1 register bits that are used by Audio.
 * This function is called in order to set the Audio Rx 1 register bits in the provided mask that
 * are used by Audio to the provided values.
 *
 * @param  mask     Bitmask indicating which bits are to be modified.
 * @param  value    New values for modified bits.
 *
 * @return Returns 0 if successful.
 */

int power_ic_audio_set_reg_mask_audio_rx_1(unsigned int mask, unsigned int value)
{
    /* The following bits are protected and should not be modified using this function. */
    mask &= ~(POWER_IC_AUDIO_REG_AUDIO_RX_1_BIT22_UNUSED
              | POWER_IC_AUDIO_REG_AUDIO_RX_1_BIT23_UNUSED);

    /* Write the new register bit values. */
    return power_ic_set_reg_mask(POWER_IC_REG_ATLAS_AUDIO_RX_1, mask, value);
}

/*!
 * @brief Set the values of the Audio TX register bits that are used by Audio.
 * This function is called in order to set the Audio TX register bits in the provided mask that
 * are used by Audio to the provided values.
 *
 * @param  mask     Bitmask indicating which bits are to be modified.
 * @param  value    New values for modified bits.
 *
 * @return Returns 0 if successful.
 */

int power_ic_audio_set_reg_mask_audio_tx(unsigned int mask, unsigned int value)
{
    /* The following bits are protected and should not be modified using this function. */
    mask &= ~(POWER_IC_AUDIO_REG_AUDIO_TX_BIT4_RESERVED
              | POWER_IC_AUDIO_REG_AUDIO_TX_MC2BDETDBNC
              | POWER_IC_AUDIO_REG_AUDIO_TX_MC2BDETEN);

    /* Write the new register bit values. */
    return power_ic_set_reg_mask(POWER_IC_REG_ATLAS_AUDIO_TX, mask, value);
}

/*!
 * @brief Set the values of the SSI Network register bits that are used by Audio.
 * This function is called in order to set the SSI Network register bits in the provided mask that
 * are used by Audio to the provided values.
 *
 * @param  mask     Bitmask indicating which bits are to be modified.
 * @param  value    New values for modified bits.
 *
 * @return Returns 0 if successful.
 */

int power_ic_audio_set_reg_mask_ssi_network(unsigned int mask, unsigned int value)
{
    /* The following bits are protected and should not be modified using this function. */
    mask &= ~(POWER_IC_AUDIO_REG_SSI_NETWORK_BIT0_RESERVED
              | POWER_IC_AUDIO_REG_SSI_NETWORK_BIT1_RESERVED
              | POWER_IC_AUDIO_REG_SSI_NETWORK_BIT21_RESERVED
              | POWER_IC_AUDIO_REG_SSI_NETWORK_BIT22_UNUSED
              | POWER_IC_AUDIO_REG_SSI_NETWORK_BIT23_UNUSED);

    /* Write the new register bit values. */
    return power_ic_set_reg_mask(POWER_IC_REG_ATLAS_SSI_NETWORK, mask, value);
}

/*!
 * @brief Set the values of the Audio Codec register bits that are used by Audio.
 * This function is called in order to set the Audio Codec register bits in the provided mask that
 * are used by Audio to the provided values.
 *
 * @param  mask     Bitmask indicating which bits are to be modified.
 * @param  value    New values for modified bits.
 *
 * @return Returns 0 if successful.
 */

int power_ic_audio_set_reg_mask_audio_codec(unsigned int mask, unsigned int value)
{
    /* The following bits are protected and should not be modified using this function. */
    mask &= ~(POWER_IC_AUDIO_REG_AUDIO_CODEC_BIT21_UNUSED
              | POWER_IC_AUDIO_REG_AUDIO_CODEC_BIT22_UNUSED
              | POWER_IC_AUDIO_REG_AUDIO_CODEC_BIT23_UNUSED);

    /* Write the new register bit values. */
    return power_ic_set_reg_mask(POWER_IC_REG_ATLAS_AUDIO_CODEC, mask, value);
}

/*!
 * @brief Set the values of the Audio Stereo DAC register bits that are used by Audio.
 * This function is called in order to set the Audio Stereo DAC register bits in the provided mask
 * that are used by Audio to the provided values.
 *
 * @param  mask     Bitmask indicating which bits are to be modified.
 * @param  value    New values for modified bits.
 *
 * @return Returns 0 if successful.
 */

int power_ic_audio_set_reg_mask_audio_stereo_dac(unsigned int mask, unsigned int value)
{
    /* The following bits are protected and should not be modified using this function. */
    mask &= ~(POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT13_RESERVED
              | POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT14_RESERVED
              | POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT21_UNUSED
              | POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT22_UNUSED
              | POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT23_UNUSED);

    /* Write the new register bit values. */
    return power_ic_set_reg_mask(POWER_IC_REG_ATLAS_AUDIO_STEREO_DAC, mask, value);
}

/*!
 * @brief Enable or disable the external audio amplifier.
 * This function will enable or disable the external audio amplifier based on the input.
 *
 * @param  en_val   1 or greater: enable amplifier
 *                  0: disable amplifier
 *
 * @return Returns 0 if successful, other values for unsuccessful.
 */
 
int power_ic_audio_ext_audio_amp_en(unsigned char en_val)
{
#if defined(CONFIG_MACH_NEVIS)
    int return_value;
    
    /* Write the new register value */
    return_value = power_ic_set_reg_bit(POWER_IC_REG_ATLAS_PWR_MISC, ATLAS_GPO2_INDEX, (en_val ? 1 : 0));

    /* Check for a failure on writing the new register values. */
    if (return_value != 0)
    {
        printk("POWER IC AUDIO: Error in setting external audio amplifier.\n");
    }
    
    return return_value;
#else
    /* not supported */
    printk("POWER IC AUDIO: Not Supported.\n");
    return -ENOTSUPP;
#endif
} 

/*!
 * @brief Audio module ioctl function.
 * This function implements IOCTL controls on a Power_Ic audio module.
 *
 * @param  cmd     the command
 * @param  arg     the parameter
 *
 * @return returns 0 if successful.
 */
int audio_ioctl(unsigned int cmd, unsigned long arg)
{
    unsigned long val= arg;
    int ret_val = 0;

    switch (cmd) 
    {
        case POWER_IC_IOCTL_AUDIO_CONN_MODE_SET:
            tracemsg(_k_d("Setting conn mode with conn_mode=0x%X"),
                     *((POWER_IC_EMU_CONN_MODE_T *)val));
            ret_val = power_ic_audio_conn_mode_set(*((POWER_IC_EMU_CONN_MODE_T *)val));
            break;

#ifdef CONFIG_CERAMICSPEAKER
        case POWER_IC_IOCTL_AUDIO_CERAMIC_SPEAKER_EN:
            tracemsg(_k_d("Enable/disable ceramic speaker %d"), (int)val);
            ret_val = power_ic_audio_ceramic_speaker_en((bool)val);
            break;
#endif /* CONFIG_CERAMICSPEAKER */

        case POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_RX_0:
            tracemsg(_k_d("Setting Audio Rx 0 register with mask=0x%X, value=0x%X"),
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            ret_val = power_ic_audio_set_reg_mask_audio_rx_0(((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                                                             ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            break;

        case POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_RX_1:
            tracemsg(_k_d("Setting Audio Rx 1 register with mask=0x%X, value=0x%X"),
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            ret_val = power_ic_audio_set_reg_mask_audio_rx_1(((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                                                             ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            break;

        case POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_TX:
            tracemsg(_k_d("Setting Audio Tx register with mask=0x%X, value=0x%X"),
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            ret_val = power_ic_audio_set_reg_mask_audio_tx(((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                                                           ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            break;

        case POWER_IC_IOCTL_AUDIO_SET_REG_MASK_SSI_NETWORK:
            tracemsg(_k_d("Setting SSI_Network register with mask=0x%X, value=0x%X"),
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            ret_val = power_ic_audio_set_reg_mask_ssi_network(((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                                                              ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            break;

        case POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_CODEC:
            tracemsg(_k_d("Setting Audio Codec register with mask=0x%X, value=0x%X"),
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            ret_val = power_ic_audio_set_reg_mask_audio_codec(((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                                                              ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            break;

        case POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_STEREO_DAC:
            tracemsg(_k_d("Setting Audio Stereo DAC register with mask=0x%X, value=0x%X"),
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                     ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            ret_val = power_ic_audio_set_reg_mask_audio_stereo_dac(((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->mask,
                                                                   ((POWER_IC_AUDIO_SET_REG_MASK_T *)val)->value);
            break;

        case POWER_IC_IOCTL_EXT_AUDIO_AMP_EN:
            tracemsg(_k_d("Enable/disable external audio amplifier %d"), (int)val);
            ret_val = power_ic_audio_ext_audio_amp_en((unsigned char)val);
            break;
             
        default:
            printk("POWER IC AUDIO: %d unsupported audio ioctl command", (int)cmd);
            return -ENOTTY;
    }

    return ret_val;
}
