/*
 * Copyright © 2004-2008, Motorola, All Rights Reserved.
 *
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this list of conditions
 *   and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, this list of
 *   conditions and the following disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * - Neither the name of Motorola nor the names of its contributors may be used to endorse or
 *   promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Motorola 2008-Feb-20 - Add ioctls POWER_IC_IOCTL_ATOD_BATT_AND_CURR_UNPHASED and POWER_IC_IOCTL_ATOD_CHRGR_CURR  
 * Motorola 2008-Apr-22 - Support shared led region
 * Motorola 2008-Feb-18 - Support for external audio amplifier
 * Motorola 2008-Jan-29 - Added IOCTL for setting morphing mode
 * Motorola 2007-Jul-05 - Remove external access functions which toggle atod channel.
 * Motorola 2007-Jun-21 - Adding I/O control to get power path setting.
 * Motorola 2007-Jun-19 - NAND secure boot time improvement 
 * Motorola 2007-May-30 - Added digital loopback ioctl for tcmd
 * Motorola 2007-Apr-23 - Remove Power Control 1 initialization.
 * Motorola 2007-Mar-28 - Added IOCTL for Wlan TCMD
 * Motorola 2007-Mar-19 - Handle PowerCut
 * Motorola 2007-Feb-01 - Add IOCTL for FET_CTRL and FET_OVRD
 * Motorola 2006-Dec-22 - Add form factor interface
 * Motorola 2006-Dec-07 - Fix issue reported by klocwork
 * Motorola 2006-Nov-09 - Remove LED register initialization
 * Motorola 2006-Oct-11 - New Audio interfaces.
 * Motorola 2006-Oct-04 - Support audio output path for linear vibrator.
 * Motorola 2006-Sep-28 - Add charger connection status function
 * Motorola 2006-Aug-28 - Add Codec Clock and Stereo DAC Clock support.
 * Motorola 2006-Aug-04 - Support ambient light sensor
 * Motorola 2006-Aug-04 - Turn off transflash power when card not in use
 * Motorola 2006-Aug-03 - Mono Adder and Balance Control support
 * Motorola 2006-Jul-31 - Updated comments
 * Motorola 2006-Jul-26 - Added IOCTL for setting the lithium coin cell enable bit.
 * Motorola 2006-Jul-17 - Thermistor on/off for AtoD reading
 * Motorola 2006-Jul-17 - Added define for SPI init tables
 * Motorola 2006-Jun-22 - Added IOCTL for setting flash mode
 * Motorola 2006-May-31 - Added new interface functions to provide backlight status.
 * Motorola 2006-May-17 - USB State new functions
 * Motorola 2006-May-06 - Implement EMU Common Algorithm
 * Motorola 2006-Apr-26 - Removed timing ioctls.
 * Motorola 2006-Apr-17 - VAUDIO and BIASEN new function
 * Motorola 2006-Apr-12 - Added new interface functions to control backlight brightness.
 * Motorola 2006-Apr-11 - USB Charge State Addition
 * Motorola 2006-Feb-06 - battery rom interface
 * Motorola 2005-Sep-01 - Charger Overvoltage support
 * Motorola 2005-Mar-25 - Atlas support
 * Motorola 2005-Feb-28 - Addition of Charger Interface
 * Motorola 2004-Dec-17 - Initial Creation
 */

#ifndef __POWER_IC_H__
#define __POWER_IC_H__

/*!
 * @mainpage Power IC driver
 *
 * @section intro Introduction
 *
 * @subsection intro_purpose Purpose
 *
 * The purpose of this documentation is to document the design and programming interface
 * of the power IC device driver for various Linux platforms.
 *
 * @subsection intro_target Target Audience
 *
 * This document is intended to be used by software developers.
 *
 * @subsection intro_problems Problem Reporting Instructions
 *
 * Problems or corrections to this document must be reported using DDTS.
 *
 * @section design Design Information
 *
 * The documentation is divided up on a per-module basis.  Click on the
 * Modules link above for the detailed design information.
 *
 * @section api API Information
 *
 * For a description of the API for the driver, see the documentation for the
 * following files:
 *
 * - lights_backlight.h - Backlight control interface.
 * - lights_funlights.h - Interface for funlight control.
 * - moto_accy.h - Accessory-related interface.
 * - power_ic.h - Interface for the main functionality provided by the driver.
 */

/*!
 * @defgroup poweric_core Power IC core
 *
 * This module makes up the core of the low-level power IC driver.  It implements
 * support for the reading and writing of registers and manages the interrupts from
 * the power IC.  The module also provides the main interface to the remainder of
 * the power IC modules (e.g. RTC, A/D converter, etc.).
 */

/*!
 * @defgroup poweric_accy Power IC accessory driver
 *
 * This module provides the interface for accessory operations through /dev/accy.
 */

/*!
 * @defgroup poweric_atod Power IC AtoD converter driver
 *
 * This module provides the interface to the power IC for AtoD conversions.
 */

/*!
 * @defgroup poweric_audio Power IC audio driver
 *
 * This module controls the audio functions of the power IC.
 */

/*!
 * @defgroup poweric_charger Power IC charger driver
 *
 * This module provides the interface to control various charging functions.
 */

/*!
 * @defgroup poweric_debounce Power IC debounce
 *
 * This module debounces signals from the power IC, primarily keypresses.
 */

/*!
 * @defgroup poweric_debug Power IC debug
 *
 * This is a group of source code that is not compiled by default, but is
 * included with the driver for developer builds to aid debugging.
 */

/*!
 * @defgroup poweric_emu Power IC EMU driver
 *
 * This module handles the detection of all devices attached to the EMU bus.
 */

/*!
 * @defgroup poweric_lights Power IC lighting driver
 *
 * This module controls all of the lights (keypad, display, fun coloured stuff, etc)
 * on a phone.
 */

/*!
 * @defgroup poweric_periph Power IC peripheral driver
 *
 * This module makes up the interface to the power IC for peripherals.  This includes
 * things like the vibrator, Bluetooth, and the flash card.
 */

/*!
 * @defgroup poweric_power_management Power IC power management driver
 *
 * This module provides interfaces for managing various power-related functions.
 */

/*!
 * @defgroup poweric_rtc Power IC RTC driver
 *
 * This module makes up the interface to the power IC for the real-time clock.
 */

/*!
 * @defgroup poweric_tcmd_ioctl Power IC TCMD ioctl support
 *
 * This module will contain all power-related ioctls that are required for TCMD
 * support unless the requirements can be met with ones that already exist for normal
 * operation.
 */

/*!
 * @defgroup poweric_brt Power IC BRT support
 *
 * This module will contain all battery rom data-related ioctls
 */

/*!
 * @file power_ic.h
 *
 * @ingroup poweric_core
 *
 * @brief Contains power IC driver interface information (types, enums, macros, functions, etc.)
 *
 * This file contains the following information:
 * - User-space interface
 *   - @ref ioctl_core        "Core ioctl commands"
 *   - @ref ioctl_atod        "AtoD converter ioctl commands"
 *   - @ref ioctl_audio       "Audio ioctl commands"
 *   - @ref ioctl_charger     "Charger ioctl commands"
 *   - @ref ioctl_lights      "Lighting ioctl commands"
 *   - @ref ioctl_periph      "Peripheral ioctl commands"
 *   - @ref ioctl_pwr_mgmt    "Power management ioctl commands"
 *   - @ref ioctl_rtc         "RTC ioctl commands"
 *   - @ref ioctl_tcmd        "Test Command ioctl commands"
 *   - @ref ioctl_brt         "BRT ioctl commands"
 *   - @ref ioctl_types       "Types used in ioctl calls"
 */

#include <linux/ioctl.h>
#include <linux/lights_backlight.h>
#include <linux/lights_funlights.h>
#include <stdbool.h>

/* Including the kernel version of the time header from user-space is causing some
 * headaches. Until someone comes up with a better idea, use the kernel version for
 * kernel builds and the system's version for user-space. We only really care about
 * timeval anway...
 */
#ifdef __KERNEL__
#include <linux/time.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif


/*******************************************************************************************
 * Universal constants
 *
 *    These are constants that are universal to use of the power IC driver from both kernel
 *    and user space..
 ******************************************************************************************/
/*! The major number of the power IC driver. */
#define POWER_IC_MAJOR_NUM 220

/*! The name of the device in /dev. */
#define POWER_IC_DEV_NAME "power_ic"

/*! The number of individual AtoD readings returned for the AtoD raw conversion request. */
#define POWER_IC_ATOD_NUM_RAW_RESULTS 8

/******************************************************************************************
* NOTE
*
*    #ifdefs are not recognized outside of the kernel, so they cannot be used in this
*    global header.
*
*    DO NOT USE THESE REGISTERS DIRECTLY!!!!
*
* BE WARNED:
*
*    We need to be able to support multiple platforms and if you directly access these
*    registers, your stuff will break.
******************************************************************************************/

/*!@cond INTERNAL */
/*! Number of registers in SPI Init tables */
#define  NUM_INIT_REGS           31
/*!@endcond */

#define  NUM_INIT_TABLE_COLUMNS  2

/*! Enumeration of all registers in the power IC(s) */
typedef enum
{
    /* Beginning of Atlas Registers */
    POWER_IC_REG_ATLAS_FIRST_REG,

    POWER_IC_REG_ATLAS_INT_STAT_0 = POWER_IC_REG_ATLAS_FIRST_REG,  /*!< Interrupt Status 0 */
    POWER_IC_REG_ATLAS_INT_MASK_0,            /*!< Interrupt Mask 0 */
    POWER_IC_REG_ATLAS_INT_SENSE_0,           /*!< Interrupt Sense 0 */
    POWER_IC_REG_ATLAS_INT_STAT_1,            /*!< Interrupt Status 1 */
    POWER_IC_REG_ATLAS_INT_MASK_1,            /*!< Interrupt Mask 1 */
    POWER_IC_REG_ATLAS_INT_SENSE_1,           /*!< Interrupt Sense 1 */
    POWER_IC_REG_ATLAS_PWRUP_SENSE,           /*!< Power Up Mode Sense */
    POWER_IC_REG_ATLAS_REVISION,              /*!< Revision */
    POWER_IC_REG_ATLAS_SEMAPHORE,             /*!< Semaphore */
    POWER_IC_REG_ATLAS_ARB_PERIPH_AUDIO,      /*!< Arbitration Peripheral Audio */
    POWER_IC_REG_ATLAS_ARB_SWITCHERS,         /*!< Arbitration Switchers */
    POWER_IC_REG_ATLAS_ARB_REG_0,             /*!< Arbitration Regulators 0 */
    POWER_IC_REG_ATLAS_ARB_REG_1,             /*!< Arbitration Regulators 1 */
    POWER_IC_REG_ATLAS_PWR_CONTROL_0,         /*!< Power Control 0 */
    POWER_IC_REG_ATLAS_PWR_CONTROL_1,         /*!< Power Control 1 */
    POWER_IC_REG_ATLAS_PWR_CONTROL_2,         /*!< Power Control 2 */
    POWER_IC_REG_ATLAS_REGEN_ASSIGN,          /*!< Regen Assignment */
    POWER_IC_REG_ATLAS_CONTROL_SPARE,         /*!< Control Spare */
    POWER_IC_REG_ATLAS_MEMORY_A,              /*!< Memory A */
    POWER_IC_REG_ATLAS_MEMORY_B,              /*!< Memory B */
    POWER_IC_REG_ATLAS_RTC_TIME,              /*!< RTC Time */
    POWER_IC_REG_ATLAS_RTC_ALARM,             /*!< RTC Alarm */
    POWER_IC_REG_ATLAS_RTC_DAY,               /*!< RTC Day */
    POWER_IC_REG_ATLAS_RTC_DAY_ALARM,         /*!< RTC Day Alarm */
    POWER_IC_REG_ATLAS_SWITCHERS_0,           /*!< Switchers 0 */
    POWER_IC_REG_ATLAS_SWITCHERS_1,           /*!< Switchers 1 */
    POWER_IC_REG_ATLAS_SWITCHERS_2,           /*!< Switchers 2 */
    POWER_IC_REG_ATLAS_SWITCHERS_3,           /*!< Switchers 3 */
    POWER_IC_REG_ATLAS_SWITCHERS_4,           /*!< Switchers 4 */
    POWER_IC_REG_ATLAS_SWITCHERS_5,           /*!< Switchers 5 */
    POWER_IC_REG_ATLAS_REG_SET_0,             /*!< Regulator Setting 0 */
    POWER_IC_REG_ATLAS_REG_SET_1,             /*!< Regulator Setting 1 */
    POWER_IC_REG_ATLAS_REG_MODE_0,            /*!< Regulator Mode 0 */
    POWER_IC_REG_ATLAS_REG_MODE_1,            /*!< Regulator Mode 1 */
    POWER_IC_REG_ATLAS_PWR_MISC,              /*!< Power Miscellaneous */
    POWER_IC_REG_ATLAS_PWR_SPARE,             /*!< Power Spare */
    POWER_IC_REG_ATLAS_AUDIO_RX_0,            /*!< Audio Rx 0 */
    POWER_IC_REG_ATLAS_AUDIO_RX_1,            /*!< Audio Rx 1 */
    POWER_IC_REG_ATLAS_AUDIO_TX,              /*!< Audio Tx */
    POWER_IC_REG_ATLAS_SSI_NETWORK,           /*!< SSI Network */
    POWER_IC_REG_ATLAS_AUDIO_CODEC,           /*!< Audio Codec */
    POWER_IC_REG_ATLAS_AUDIO_STEREO_DAC,      /*!< Audio Stereo DAC */
    POWER_IC_REG_ATLAS_AUDIO_SPARE,           /*!< Audio Spare */
    POWER_IC_REG_ATLAS_ADC_0,                 /*!< ADC 0 */
    POWER_IC_REG_ATLAS_ADC_1,                 /*!< ADC 1 */
    POWER_IC_REG_ATLAS_ADC_2,                 /*!< ADC 2 */
    POWER_IC_REG_ATLAS_ADC_3,                 /*!< ADC 3 */
    POWER_IC_REG_ATLAS_ADC_4,                 /*!< ADC 4 */
    POWER_IC_REG_ATLAS_CHARGER_0,             /*!< Charger 0 */
    POWER_IC_REG_ATLAS_USB_0,                 /*!< USB 0 */
    POWER_IC_REG_ATLAS_CHARGE_USB_1,          /*!< USB 1 */
    POWER_IC_REG_ATLAS_LED_CONTROL_0,         /*!< LED Control 0 */
    POWER_IC_REG_ATLAS_LED_CONTROL_1,         /*!< LED Control 1 */
    POWER_IC_REG_ATLAS_LED_CONTROL_2,         /*!< LED Control 2 */
    POWER_IC_REG_ATLAS_LED_CONTROL_3,         /*!< LED Control 3 */
    POWER_IC_REG_ATLAS_LED_CONTROL_4,         /*!< LED Control 4 */
    POWER_IC_REG_ATLAS_LED_CONTROL_5,         /*!< LED Control 5 */
    POWER_IC_REG_ATLAS_SPARE,                 /*!< Spare */
    POWER_IC_REG_ATLAS_TRIM_0,                /*!< Trim 0 */
    POWER_IC_REG_ATLAS_TRIM_1,                /*!< Trim 1 */
    POWER_IC_REG_ATLAS_TEST_0,                /*!< Test 0 */
    POWER_IC_REG_ATLAS_TEST_1,                /*!< Test 1*/
    POWER_IC_REG_ATLAS_TEST_2,                /*!< Test 2 */
    POWER_IC_REG_ATLAS_TEST_3,                /*!< Test 3 */

    POWER_IC_REG_ATLAS_LAST_REG = POWER_IC_REG_ATLAS_TEST_3,

    /* End of Atlas Registers */

    POWER_IC_REG_NUM_REGS_ATLAS
} POWER_IC_REGISTER_T;

/*! Enumeration of possible power IC events */
typedef enum
{
    POWER_IC_EVENT_ATLAS_FIRST_REG,

    /* Interrupt Status 0 Register */
    POWER_IC_EVENT_ATLAS_ADCDONEI = POWER_IC_EVENT_ATLAS_FIRST_REG, /*!< AtoD Conversion complete interrupt */
    POWER_IC_EVENT_ATLAS_ADCBISDONEI,    /*!< ADCBIS complete interrupt */
    POWER_IC_EVENT_ATLAS_TSI,            /*!< Touchscreen interrupt */
    POWER_IC_EVENT_ATLAS_WHIGHI,         /*!< ADC reading above high limit interrupt */
    POWER_IC_EVENT_ATLAS_WLOWI,          /*!< ADC reading below low limit interrupt */
    POWER_IC_EVENT_ATLAS_RESERVED1,      /*!< For future use */
    POWER_IC_EVENT_ATLAS_CHGDETI,        /*!< Charger Attached interrupt */
    POWER_IC_EVENT_ATLAS_CHGOVI,         /*!< Charger overvoltage detection interrupt */
    POWER_IC_EVENT_ATLAS_CHGREVI,        /*!< Charger path reverse current interrupt */
    POWER_IC_EVENT_ATLAS_CHGSHORTI,      /*!< Charger path short current interrupt */
    POWER_IC_EVENT_ATLAS_CCCVI,          /*!< BP regulator V or I interrupt */
    POWER_IC_EVENT_ATLAS_CHGCURRI,       /*!< Charge current below threshold warning interrupt */
    POWER_IC_EVENT_ATLAS_BPONI,          /*!< BP turn on threshold interrupt */
    POWER_IC_EVENT_ATLAS_LOBATLI,        /*!< Low battery low threshold warning interrupt */
    POWER_IC_EVENT_ATLAS_LOBATHI,        /*!< Low battery high threshold warning interrupt */
    POWER_IC_EVENT_ATLAS_RESERVED2,      /*!< For future use */
    POWER_IC_EVENT_ATLAS_USBI,           /*!< USB VBUS detect interrupt */
    POWER_IC_EVENT_ATLAS_USB2V0S,        /*!< USB2V0 sense bit, unused in mask interrupt */
    POWER_IC_EVENT_ATLAS_USB0V8S,        /*!< USB0V8 sense bit, unused in mask interrupt */
    POWER_IC_EVENT_ATLAS_IDI,            /*!< USB ID detect interrupt */
    POWER_IC_EVENT_ATLAS_ID_FLOAT = POWER_IC_EVENT_ATLAS_IDI,
    POWER_IC_EVENT_ATLAS_ID_GROUND,      /*!< USB ID Ground sense bit (unused interrupt status/mask) */
    POWER_IC_EVENT_ATLAS_SE1I,           /*!< Single ended 1 detect interrupt */
    POWER_IC_EVENT_ATLAS_CKDETI,         /*!< Carkit detect interrupt */
    POWER_IC_EVENT_ATLAS_RESERVED3,      /*!< For future use */

    POWER_IC_EVENT_ATLAS_FIRST_REG_LAST = POWER_IC_EVENT_ATLAS_RESERVED3,

    POWER_IC_EVENT_ATLAS_SECOND_REG,

    /* Interrupt Status 1 Register */
    POWER_IC_EVENT_ATLAS_1HZI = POWER_IC_EVENT_ATLAS_SECOND_REG,           /*!< 1 Hz timetick */
    POWER_IC_EVENT_ATLAS_TODAI,          /*!< Time of day alarm interrupt */
    POWER_IC_EVENT_ATLAS_RESERVED4,      /*!< For future use */
    POWER_IC_EVENT_ATLAS_ONOFD1I,        /*!< ON1B event interrupt */
    POWER_IC_EVENT_ATLAS_ONOFD2I,        /*!< ON2B event interrupt */
    POWER_IC_EVENT_ATLAS_ONOFD3I,        /*!< ON3B event interrupt */
    POWER_IC_EVENT_ATLAS_SYSRSTI,        /*!< System reset interrupt */
    POWER_IC_EVENT_ATLAS_RTCRSTI,        /*!< RTC reset event interrupt */
    POWER_IC_EVENT_ATLAS_PCI,            /*!< Power cut event interrupt */
    POWER_IC_EVENT_ATLAS_WARMI,          /*!< Warm start event interrupt */
    POWER_IC_EVENT_ATLAS_MEMHLDI,        /*!< Memory hold event interrupt */
    POWER_IC_EVENT_ATLAS_PWRRDYI,        /*!< Power Gate and DVS power ready interrupt */
    POWER_IC_EVENT_ATLAS_THWARNLI,       /*!< Thermal warning low threshold interrupt */
    POWER_IC_EVENT_ATLAS_THWARNHI,       /*!< Thermal warning high threshold interrupt */
    POWER_IC_EVENT_ATLAS_CLKI,           /*!< Clock source change interrupt */
    POWER_IC_EVENT_ATLAS_SEMAFI,         /*!< Semaphore */
    POWER_IC_EVENT_ATLAS_RESERVED5,      /*!< For future use */
    POWER_IC_EVENT_ATLAS_MC2BI,          /*!< Microphone bias 2 detect interrupt */
    POWER_IC_EVENT_ATLAS_HSDETI,         /*!< Headset attach interrupt */
    POWER_IC_EVENT_ATLAS_HSLI,           /*!< Stereo headset detect interrupt */
    POWER_IC_EVENT_ATLAS_ALSPTHI,        /*!< Thermal shutdown Alsp interrupt */
    POWER_IC_EVENT_ATLAS_AHSSHORTI,      /*!< Short circuit on Ahs outputs interrupt */
    POWER_IC_EVENT_ATLAS_RESERVED6,      /*!< For future use */
    POWER_IC_EVENT_ATLAS_RESERVED7,      /*!< For future use */

    POWER_IC_EVENT_ATLAS_SECOND_REG_LAST = POWER_IC_EVENT_ATLAS_RESERVED7,

    POWER_IC_EVENT_NUM_EVENTS_ATLAS
} POWER_IC_EVENT_T;

/*! Defines the possible settings for the state of a power IC peripheral */
typedef enum
{
    POWER_IC_PERIPH_OFF,
    POWER_IC_PERIPH_ON
} POWER_IC_PERIPH_ONOFF_T;

/*! Sample rate for the ST Dac*/
typedef enum
{
    POWER_IC_ST_DAC_SR_8000,
    POWER_IC_ST_DAC_SR_11025,
    POWER_IC_ST_DAC_SR_12000,
    POWER_IC_ST_DAC_SR_16000,
    POWER_IC_ST_DAC_SR_22050,
    POWER_IC_ST_DAC_SR_24000,
    POWER_IC_ST_DAC_SR_32000,
    POWER_IC_ST_DAC_SR_44100,
    POWER_IC_ST_DAC_SR_48000
} POWER_IC_ST_DAC_SR_T;

/*! Sample rate for the codec */
typedef enum
{
    POWER_IC_CODEC_SR_8000,
    POWER_IC_CODEC_SR_16000
} POWER_IC_CODEC_SR_T;

/*! Settings for mono adder */
typedef enum
{
    POWER_IC_MONO_LEFT_RIGHT_IND,
    POWER_IC_MONO_STEREO_OPP,
    POWER_IC_MONO_STEREO_MONO_CONV,
    POWER_IC_MONO_MONO_OPP
} POWER_IC_AUD_MONO_ADDER_T;

/*! Attenuation in dB for balance control */
typedef enum
{
    POWER_IC_BAL_CTRL_R_0,
    POWER_IC_BAL_CTRL_R_3,
    POWER_IC_BAL_CTRL_R_6,
    POWER_IC_BAL_CTRL_R_9,
    POWER_IC_BAL_CTRL_R_12,
    POWER_IC_BAL_CTRL_R_15,
    POWER_IC_BAL_CTRL_R_18,
    POWER_IC_BAL_CTRL_R_21,
    POWER_IC_BAL_CTRL_L_0,
    POWER_IC_BAL_CTRL_L_3,
    POWER_IC_BAL_CTRL_L_6,
    POWER_IC_BAL_CTRL_L_9,
    POWER_IC_BAL_CTRL_L_12,
    POWER_IC_BAL_CTRL_L_15,
    POWER_IC_BAL_CTRL_L_18,
    POWER_IC_BAL_CTRL_L_21
} POWER_IC_AUD_BAL_CTRL_T;

/* Settings for Codec clock. */
typedef enum
{
    POWER_IC_AUD_CODEC_CLOCK_13_MHZ,  /* CLI = 13.0  MHz */
    POWER_IC_AUD_CODEC_CLOCK_15_MHZ,  /* CLI = 15.36 MHz */
    POWER_IC_AUD_CODEC_CLOCK_16_MHZ,  /* CLI = 16.8  MHz */
    POWER_IC_AUD_CODEC_CLOCK_NA_1,    /* This setting is unused. */
    POWER_IC_AUD_CODEC_CLOCK_26_MHZ,  /* CLI = 26.0  MHz */
    POWER_IC_AUD_CODEC_CLOCK_NA_2,    /* This setting is unused. */
    POWER_IC_AUD_CODEC_CLOCK_NA_3,    /* This setting is unused. */
    POWER_IC_AUD_CODEC_CLOCK_33_MHZ   /* CLI = 33.6  MHz */
} POWER_IC_AUD_CODEC_CLOCK_T;

/* Settings for Stereo DAC clock. */
typedef enum
{
    POWER_IC_AUD_ST_DAC_CLOCK_13_MHZ, /* CLI = 13.0   MHz */
    POWER_IC_AUD_ST_DAC_CLOCK_15_MHZ, /* CLI = 15.36  MHz */
    POWER_IC_AUD_ST_DAC_CLOCK_16_MHZ, /* CLI = 16.8   MHz */
    POWER_IC_AUD_ST_DAC_CLOCK_NA,     /* This setting is unused.*/
    POWER_IC_AUD_ST_DAC_CLOCK_26_MHZ, /* CLI = 26.0   MHz */
    POWER_IC_AUD_ST_DAC_CLOCK_12_MHZ, /* CLI = 12.0   MHz */
    POWER_IC_AUD_ST_DAC_CLOCK_3_MHZ,  /* CLI = 3.6864 MHz */
    POWER_IC_AUD_ST_DAC_CLOCK_33_MHZ  /* CLI = 33.6   MHz */
} POWER_IC_AUD_ST_DAC_CLOCK_T;

/*! For the AtoD interface, these are the individual channels that can be requested. */
typedef enum
{
    POWER_IC_ATOD_CHANNEL_PA_THERM,
    POWER_IC_ATOD_CHANNEL_BATT,
    POWER_IC_ATOD_CHANNEL_BATT_CURR,
    POWER_IC_ATOD_CHANNEL_BPLUS,
    POWER_IC_ATOD_CHANNEL_CHARGER_ID,
    POWER_IC_ATOD_CHANNEL_CHRG_CURR,
    POWER_IC_ATOD_CHANNEL_COIN_CELL,
    POWER_IC_ATOD_CHANNEL_MOBPORTB,
    POWER_IC_ATOD_CHANNEL_TEMPERATURE,
    POWER_IC_ATOD_CHANNEL_AMBIENT_LIGHT
} POWER_IC_ATOD_CHANNEL_T;

#define POWER_IC_ATOD_NUM_CHANNELS (POWER_IC_ATOD_CHANNEL_AMBIENT_LIGHT + 1)

/*! The timing requested for the battery/current conversion. */
typedef enum
{
    POWER_IC_ATOD_TIMING_IMMEDIATE,
    POWER_IC_ATOD_TIMING_IN_BURST,
    POWER_IC_ATOD_TIMING_OUT_OF_BURST
} POWER_IC_ATOD_TIMING_T;

/*! Indication of whether a hardware-timed conversion completed or timed out. */
typedef enum
{
    POWER_IC_ATOD_CONVERSION_TIMEOUT,
    POWER_IC_ATOD_CONVERSION_COMPLETE
} POWER_IC_ATOD_CONVERSION_TIMEOUT_T;

/*! The direction in which current will be measured for the battery/current conversion. */
typedef enum
{
    POWER_IC_ATOD_CURR_POLARITY_DISCHARGE,
    POWER_IC_ATOD_CURR_POLARITY_CHARGE
} POWER_IC_ATOD_CURR_POLARITY_T;

/*! Indication of AD6 should be taken in PA therm sense or Licell voltage. */
typedef enum
{
    POWER_IC_ATOD_AD6_PA_THERM,
    POWER_IC_ATOD_AD6_LI_CELL
} POWER_IC_ATOD_AD6_T;

/*! Power-up reasons */
typedef enum
{
    POWER_IC_POWER_UP_REASON_NONE,
    POWER_IC_POWER_UP_REASON_FIRST_POWER_KEY_LONG,    /*!< indicates sense bit still active */
    POWER_IC_POWER_UP_REASON_FIRST_POWER_KEY_SHORT,   /*!< indicates only status bit active */
    POWER_IC_POWER_UP_REASON_SECOND_POWER_KEY_LONG,   /*!< indicates sense bit still active */
    POWER_IC_POWER_UP_REASON_SECOND_POWER_KEY_SHORT,  /*!< indicates only status bit active */
    POWER_IC_POWER_UP_REASON_THIRD_POWER_KEY_LONG,    /*!< indicates sense bit still active */
    POWER_IC_POWER_UP_REASON_THIRD_POWER_KEY_SHORT,   /*!< indicates only status bit active */
    POWER_IC_POWER_UP_REASON_CHARGER,
    POWER_IC_POWER_UP_REASON_POWER_CUT,
    POWER_IC_POWER_UP_REASON_ALARM
} POWER_IC_POWER_UP_REASON_T;



/*! Power paths that can be selected. */
typedef enum
{
    POWER_IC_CHARGER_POWER_DUAL_PATH,                 /*!< Dual-path mode under hardware control. */
    POWER_IC_CHARGER_POWER_CURRENT_SHARE,             /*!< Current-share under hardware control. */
    POWER_IC_CHARGER_POWER_DUAL_PATH_SW_OVERRIDE,     /*!< Dual-path mode forced under software control. */
    POWER_IC_CHARGER_POWER_CURRENT_SHARE_SW_OVERRIDE  /*!< Current-share forced under software control. */
} POWER_IC_CHARGER_POWER_PATH_T;

/*! USB Charge States that can be selected */
typedef enum
{
    POWER_IC_CHARGER_CHRG_STATE_CHARGE,            /*!< High charge state (500mA) */
    POWER_IC_CHARGER_CHRG_STATE_SLOW_CHARGE,       /*!< Low charge state (100mA) */
    POWER_IC_CHARGER_CHRG_STATE_SUSPEND,           /*!< USB Suspend mode */
    POWER_IC_CHARGER_CHRG_STATE_FAIL_ENUM,         /*!< USB Failed Enumeration */
    POWER_IC_CHARGER_CHRG_STATE_NOT_ALLOWED,       /*!< USB state not allowed */
    POWER_IC_CHARGER_CHRG_STATE_NONE_CURR,         /*!< No current available */
    POWER_IC_CHARGER_CHRG_STATE_INVALID,           /*!< Invalid USB State */
    POWER_IC_CHARGER_CHRG_STATE_UNKNOWN = 0xFF     /*!< Initial Unknown State */
} POWER_IC_CHARGER_CHRG_STATE_T;

/*! SIM Voltages that can be selected. */
typedef enum
{
    POWER_IC_SIM_VOLT_18,        /*!< indicates a 1.8 voltage setting for SIM */
    POWER_IC_SIM_VOLT_30         /*!< indicates a 3.0 voltage setting for SIM */
} POWER_IC_SIM_VOLTAGE_T;

/*! Identifiers for bits that can be read/written to using the Backup Memory API. */
typedef enum
{
    POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_FLASH_MODE, /*!< Signal MBM to go into flash mode after reboot. */
    POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_PANIC,      /*!< Signal MBM that a panic occurred. */
    POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_FOTA_MODE,  /*!< Signal MBM to go in FOTA mode after reboot. */
    POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_FLASH_FAIL, /*!< Signal that flash operation failed.*/
    POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_SOFT_RESET, /*!< Signal the soft reset occurred. */
    POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_WDOG_RESET, /*!< Signal that watchdog was reset. */
    POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_ROOTFS_SEC_FAIL,    /*!< Signal that secure rootfs failed. */
    POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_LANG_PACK_SEC_FAIL, /*!< Signal that secure language pack failed. */

    POWER_IC_BACKUP_MEMORY_ID_NUM_ELEMENTS             /*!< Number of ID elements defined. */
} POWER_IC_BACKUP_MEMORY_ID_T;

/*! States to determine possible CONN modes, this table is tied to hardware */
typedef enum
{
    POWER_IC_EMU_CONN_MODE_USB,             /*000*/
    POWER_IC_EMU_CONN_MODE_UART1,           /*001*/
    POWER_IC_EMU_CONN_MODE_UART2,           /*010*/
    POWER_IC_EMU_CONN_MODE_RESERVED,        /*011*/
    POWER_IC_EMU_CONN_MODE_MONO_AUDIO,      /*100*/
    POWER_IC_EMU_CONN_MODE_STEREO_AUDIO,    /*101*/
    POWER_IC_EMU_CONN_MODE_LOOPBACK_RIGHT,  /*110*/
    POWER_IC_EMU_CONN_MODE_LOOPBACK_LEFT,   /*111*/

    POWER_IC_EMU_CONN_MODE__NUM
} POWER_IC_EMU_CONN_MODE_T;

/*! States to determine the transceiver states */
typedef enum
{
    EMU_XCVR_OFF,
    EMU_XCVR_PPD_DETECT,
    EMU_XCVR_SPD_DETECT,
    EMU_XCVR_PPD,
    EMU_XCVR_PPD_AUDIO,
    EMU_XCVR_SPD,
    EMU_XCVR_SPD_AUDIO,
    EMU_XCVR_USB_HOST,
    EMU_XCVR_FACTORY_MODE,

    EMU_XCVR__NUM
} EMU_XCVR_T;

/*! For battery rom interface */
#define BRT_BYTE_SIZE 128
#define BRT_UID_SIZE 8

typedef enum
{
    BRT_NONE = 0,
    BRT_W_DATA,
    BRT_WO_DATA,
    BRT_INVALID
} BRT_DATA_T;

typedef enum
{
    POWER_IC_FORM_FACTOR_FLIP,
    POWER_IC_FORM_FACTOR_SLIDER,
    POWER_IC_FORM_FACTOR_CANDYBAR,
    POWER_IC_FORM_FACTOR_UNKNOWN
} POWER_IC_FORM_FACTOR_T;


#ifndef DOXYGEN_SHOULD_SKIP_THIS /* This stuff just clutters the documentation. Don't bother. */
/*!
 * @name ioctl() command ranges
 *
 * These are the ranges of ioctl commands for the power IC driver. These cannot be used
 * directly from user-space, but are used to construct the request parameters used in ioctl()
 * calls to the driver.
 */

/* @{ */

/* Base of all the ioctl() commands that will be handled by the driver core. */
#define POWER_IC_IOC_CMD_CORE_BASE          0x00
/* Last of the range of ioctl() commands reserved for the driver core. */
#define POWER_IC_IOC_CMD_CORE_LAST_CMD      (POWER_IC_IOC_CMD_CORE_BASE + 0x0F)

/* Base of all the ioctl() commands that will be handled by the peripheral module. */
#define POWER_IC_IOC_CMD_PERIPH_BASE        (POWER_IC_IOC_CMD_CORE_LAST_CMD + 1)
/* This is the last of the range of ioctl() commands reserved for the peripheral interface. */
#define POWER_IC_IO_CMD_PERIPH_LAST_CMD     (POWER_IC_IOC_CMD_PERIPH_BASE + 0x0F)

/* Base for the ioctl() commands for the RTC */
#define POWER_IC_IOC_RTC_BASE               (POWER_IC_IO_CMD_PERIPH_LAST_CMD + 1)
/* Last ioctl() command reserved for the RTC. */
#define POWER_IC_IOC_RTC_LAST_CMD           (POWER_IC_IOC_RTC_BASE + 0x0F)

/*! Base for the ioctl() commands for AtoD requests */
#define POWER_IC_IOC_ATOD_BASE              (POWER_IC_IOC_RTC_LAST_CMD + 1)
/*! Last ioctl() command reserved for AtoD. */
#define POWER_IC_IOC_ATOD_LAST_CMD          (POWER_IC_IOC_ATOD_BASE + 0x0F)

/*! Base for the ioctl() commands for the power management module. */
#define POWER_IC_IOC_PMM_BASE               (POWER_IC_IOC_ATOD_LAST_CMD + 1)
/*! Last ioctl() command reserved for power management. */
#define POWER_IC_IOC_PMM_LAST_CMD           (POWER_IC_IOC_PMM_BASE + 0x0F)

/* Base of all the ioctl() commands that will be handled by the audio module. */
#define POWER_IC_IOC_CMD_AUDIO_BASE         (POWER_IC_IOC_PMM_LAST_CMD + 1)
/* Last ioctl() command reserved for the audio interface. */
#define POWER_IC_IOC_CMD_AUDIO_LAST_CMD     (POWER_IC_IOC_CMD_AUDIO_BASE + 0x0F)

/* This is the base of all the ioctl() commands that will be handled by the lights module. */
#define POWER_IC_IOC_LIGHTS_BASE            (POWER_IC_IOC_CMD_AUDIO_LAST_CMD + 1)
/* Last ioctl() command reserved for the lights interface. */
#define POWER_IC_IOC_LIGHTS_LAST_CMD        (POWER_IC_IOC_LIGHTS_BASE + 0x0F)

/* This is the base of all the ioctl() commands that will be handled by the charger module. */
#define POWER_IC_IOC_CMD_CHARGER_BASE       (POWER_IC_IOC_LIGHTS_LAST_CMD + 1)
/* Last ioctl() command reserved for the charger interface. */
#define POWER_IC_IOC_CHARGER_LAST_CMD       (POWER_IC_IOC_CMD_CHARGER_BASE + 0x0F)

/* Base of all the ioctl() commands that will be handled by the tcmd_ioctl module. */
#define POWER_IC_IOC_CMD_TCMD_BASE          (POWER_IC_IOC_CHARGER_LAST_CMD + 1)
/* Last ioctl() command reserved for the tcmd_ioctl module. */
#define POWER_IC_IOC_CMD_TCMD_LAST_CMD      (POWER_IC_IOC_CMD_TCMD_BASE + 0x1F)

/* Base of all the ioctl() commands that will be handled by the brt module. */
#define POWER_IC_IOC_CMD_BRT_BASE          (POWER_IC_IOC_CMD_TCMD_LAST_CMD + 1)
/* Last ioctl() command reserved for the brt_ioctl module. */
#define POWER_IC_IOC_CMD_BRT_LAST_CMD      (POWER_IC_IOC_CMD_BRT_BASE + 0x0F)

/* @} End of ioctl range constants. -------------------------------------------------------------- */

/* Old IOC_CMD numbers that we have removed. These were clearly commented that they should
 * not be used in calls to ioctl(), but they got used anyway. These are here only to allow
 * the build to complete so we don't hold too much up right now.
 *
 * In case you missed it, DO NOT USE THESE. EVER.
 */
#define POWER_IC_IOC_LIGHTS_BACKLIGHTS_SET           (POWER_IC_IOC_LIGHTS_BASE + 0x00)
#define POWER_IC_IOC_LIGHTS_FL_SET_CONTROL           (POWER_IC_IOC_LIGHTS_BASE + 0x01)
#define POWER_IC_IOC_LIGHTS_FL_UPDATE                (POWER_IC_IOC_LIGHTS_BASE + 0x02)

#endif /* Doxygen skips over this... */

/*******************************************************************************************
 * Driver ioctls
 *
 * All of the ioctl commands supported by the driver are in this section.
 *
 * Note: when adding new ioctl commands, each should be fully documented in the format used
 * for the existing commands. This will be the only documentation available for each ioctl,
 * so it is in our best interest to provide useful and consistent information here.
 *
 * Typically, each ioctl command should have the following:
 *
 * A brief, one-sentence description prefixed with doxygen's brief tag.
 *
 * A paragraph with a more detailed description of the operation of the command.
 *
 * A paragraph describing inputs to the command.
 *
 * A paragraph describing outputs from the command.
 *
 * optionally, one or more notes about the command (prefxed by Doxygen's note tag).
 ******************************************************************************************/

/*!
 * @anchor ioctl_core
 * @name Core ioctl() commands
 *
 * These are the commands that can be passed to ioctl() to request low-level operations on the
 * power IC driver. In general. <b>the basic register access commands should not be used unless
 * absolutely necessary</b>. Instead, the abstracted interfaces should be used to read and write
 * values to registers as these are supported without any name changes between platforms.
 */

/* @{ */

/*!
 * @brief Reads a register.
 *
 * This command reads the entire contents of a single specified register and passes back its
 * contents.
 *
 * The register is specified in the reg field of the passed POWER_IC_REG_ACCESS_T.
 *
 * The value read is returned in the value field of the structure.
 */
#define POWER_IC_IOCTL_READ_REG \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CORE_BASE + 0x00), POWER_IC_REG_ACCESS_T *)

/*!
 * @brief Writes a register.
 *
 * This command overwrites the entire contents of a single specified register.
 *
 * The register is specified in the reg field of the passed POWER_IC_REG_ACCESS_T and the
 * new value to be written is passed in the value field of the structure.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_WRITE_REG \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CORE_BASE + 0x01), POWER_IC_REG_ACCESS_T *)

/*!
 * @brief Reads a subset of bits from a register.
 *
 * This command reads a contiguous set of bits from a single specified register. The
 * driver reads from the register specified and does the necessary masking and shifting so
 * that only the specified bits are passed back, shifted so that the first bit read is bit
 * zero of the value passed back.
 *
 * The register is specified in the reg field of the passed POWER_IC_REG_ACCESS_T, the
 * index of the first (least significant) bit is specified in the index field, and the
 * number of bits including the first bit is specified in the num_bits field.
 *
 * The value read starts from bit zero of the returned value, which is passed back in the
 * value field of the structure.
 */
#define POWER_IC_IOCTL_READ_REG_BITS \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CORE_BASE + 0x02), POWER_IC_REG_ACCESS_T *)

/*!
 * @brief Writes a subset of bits from a register.
 *
 * This command overwrites a contiguous set of bits from a single specified register. The
 * driver takes the value and shifts it to the correct position in the register, overwriting
 * the subset of bits specified. The remaining bits are not changed.
 *
 * The register is specified in the reg field of the passed POWER_IC_REG_ACCESS_T, the index
 * of the first (least significant) bit is specified in the index field, the number of bits
 * (including the first bit) is specified in the num_bits field and the value to be shifted
 * and written is specified in the value field.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_WRITE_REG_BITS \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CORE_BASE + 0x03), POWER_IC_REG_ACCESS_T *)
/*!
 * @brief Writes a subset of bits from a register.
 *
 * This command overwrites a possibly non-contiguous set of bits from a single specified
 * register. This is similar to POWER_IC_IOCTL_WRITE_REG_BITS above, but instead the value
 * specified must be bit-exact compared to the register to be written (i.e. the bits you
 * want to set/clear in the register must be set the same way in the value passed in to
 * the command.
 *
 * The register is specified in the reg field of the passed POWER_IC_REG_ACCESS_T
 * and the bitmask describing which register bits to be changed is specified by the
 * num_bits field (each bit set will result in that bit in the register being overwritten)
 * and the bit-exact value to be written is passed in the value field of the structure.
 *
 * This command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_WRITE_REG_MASK \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CORE_BASE + 0x04), POWER_IC_REG_ACCESS_T *)

/*!
 * @brief Gets the powerup reason.
 *
 * This command retrieves the reason the phone powered up from the driver. When the driver
 * starts, it checks for possible reasons for powering up and remembers the reason, which
 * is retrieved later via this command.
 *
 * The command has no inputs.
 *
 * The command passes back the powerup reason via the passed pointer to a
 * POWER_IC_POWER_UP_REASON_T.
 */
#define POWER_IC_IOCTL_GET_POWER_UP_REASON \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CORE_BASE + 0x05), POWER_IC_POWER_UP_REASON_T *)

/*!
 * @brief Gets hardware information.
 *
 * This command fetches information about the hardware's type and any revision information
 * that might be available for the hardware.
 *
 * The command has no inputs.
 *
 * The command passes back the power IC hardware type and revision information in the
 * POWER_IC_HARDWARE_T structure, which is passed by pointer.
 *
 * @note Under most circumstances, a caller does not need to know whether the power IC is an
 * Atlas or something else. This is only intended for the few cases where more information
 * is needed about the hardware, e.g. to interpret the results of an AtoD conversion
 * or similar. <b>Please do not make unnecessary use of this command.</b>
 */
#define POWER_IC_IOCTL_GET_HARDWARE_INFO \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CORE_BASE + 0x06), POWER_IC_HARDWARE_T *)

/*!
 * @brief Reads a Backup Memory register bit(s).
 *
 * This command reads part of either the Backup Memory A or Backup Memory B registers based on the
 * ID provided and the backup_memory_table located in external.c and passes back its value.
 *
 * The ID is specified in the reg field of the passed POWER_IC_REG_ACCESS_T.
 *
 * The value read is returned in the value field of the structure.
 */
#define POWER_IC_IOCTL_READ_BACKUP_MEMORY \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CORE_BASE + 0x07), POWER_IC_BACKUP_MEM_ACCESS_T *)

/*!
 * @brief Writes a Backup Memory register bit(s).
 *
 * This command overwrites part of either the Backup Memory A or Backup Memory B registers based on
 * the ID provided and the backup_memory_table located in external.c.
 *
 * The ID is specified in the reg field of the passed POWER_IC_REG_ACCESS_T and the
 * new value to be written is passed in the value field of the structure.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_WRITE_BACKUP_MEMORY \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CORE_BASE + 0x08), POWER_IC_BACKUP_MEM_ACCESS_T *)

/* @} End of core ioctls.  ------------------------------------------------------------------------*/

/*!
 * @anchor ioctl_periph
 * @name Peripheral ioctl() commands
 *
 * These are the commands that can be used through ioctl() to perform various peripheral
 * operations. The caller does not need to know which platform these commands are being
 * performed on.
 */

/* @{ */
/*!
 * @brief Sets the vibrator level.
 *
 * This command sets the level at which the vibrator should spin when it is turned on.
 *
 * This command takes a single int, indicating the level at which the vibrator should turn.
 * The available range of levels is 0..3, with higher levels resulting in the vibrator
 * spinning faster.
 *
 * This command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_PERIPH_SET_VIBRATOR_LEVEL \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_PERIPH_BASE + 0x00), int)

/*!
 * @brief Turns the vibrator on and off.
 *
 * This command is the on/off switch for the vibrator. When on, the vibrator will spin at the
 * level set by POWER_IC_IOCTL_PERIPH_SET_VIBRATOR_LEVEL.
 *
 * This command takes a single POWER_IC_PERIPH_ONOFF_T, indicating whether the vibrator should be
 * turned on or off.
 *
 * This command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_PERIPH_SET_VIBRATOR_ON \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_PERIPH_BASE + 0x01), POWER_IC_PERIPH_ONOFF_T)

/*!
 * @brief Controls power to Camera.
 *
 * This command controls the regulator that powers the Camera hardware, allowing the
 * caller to turn the hardware on and off.
 *
 * The command takes a single POWER_IC_PERIPH_ONOFF_T, indicating whether the Camera should be
 * turned on or off.
 *
 * This command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_PERIPH_SET_CAMERA_ON \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_PERIPH_BASE + 0x02), POWER_IC_PERIPH_ONOFF_T)
/* @} End of power management ioctls.  ------------------------------------------------------------*/


/*!
 * @anchor ioctl_rtc
 * @name Real-time clock (RTC) ioctl() commands
 *
 * These are the commands that can be used through ioctl() to perform various operations on
 * the real-time clock. The caller does not need to know which platform these commands are being
 * performed on.
 */

/* @{ */

/*!
 * @brief Gets the current RTC time.
 *
 * This command reads the current time from the hardware.
 *
 * This command takes no inputs.
 *
 * The command passes back the current time in the passed timeval structure. The time
 * is expressed in the unix-standard number of seconds since January, 1 1970 00:00:00 UTC.
 */
#define POWER_IC_IOCTL_GET_TIME \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_RTC_BASE + 0x00), struct timeval *)

/*!
 * @brief Sets the current RTC time.
 *
 * This command writes the passed time to the hardware RTC.
 *
 * The command takes the current time in the passed timeval structure. The time should be
 * expressed in the unix-standard number of seconds since January, 1 1970 00:00:00 UTC.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_SET_TIME \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_RTC_BASE + 0x01), struct timeval *)

/*!
 * @brief Gets the currently set RTC alarm.
 *
 * This command reads the current programmed alarm from the hardware.
 *
 * This command takes no inputs.
 *
 * The command passes back the alarm time in the passed timeval structure. The time
 * is expressed in the unix-standard number of seconds since January, 1 1970 00:00:00 UTC.
 */
#define POWER_IC_IOCTL_GET_ALARM_TIME \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_RTC_BASE + 0x02), struct timeval *)

/*!
 * @brief Programs the RTC alarm.
 *
 * This command writes the passed alarm time to the hardware RTC.
 *
 * The command takes the alarm time in the passed timeval structure. The time should be
 * expressed in the unix-standard number of seconds since January, 1 1970 00:00:00 UTC.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_SET_ALARM_TIME \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_RTC_BASE + 0x03), struct timeval *)
/* @} End of RTC ioctls.  -------------------------------------------------------------------------*/

/*!
 * @anchor ioctl_atod
 * @name Analogue to Digital converter (AtoD) ioctl() commands
 *
 * These are the commands that can be used through ioctl() to perform various AtoD
 * conversions. The caller does not need to know which platform these commands are
 * being performed on in order to perform a conversion, but may need to know to
 * interpret the measurements correctly.
 */

/* @{ */

/*!
 * @brief Converts a single channel.
 *
 * This command performs a conversion on a single AtoD channel. The results of the
 * conversion are averaged and phasing is applied to the result (if available for
 * the converted channel).
 *
 * The command takes the channel to be converted in the channel field of the passed
 * POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T structure.
 *
 * The averaged (and possibly phased) AtoD measurement is returned in the result field of the
 * structure.
 */
#define POWER_IC_IOCTL_ATOD_SINGLE_CHANNEL \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x00), POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T *)
        
/*!
 * @brief Converts battery voltage and current and returns the phased actual DAC values.
 *
 * This command performs a conversion of the battery voltage and current. The results of the
 * conversion are averaged, phased, and converted to mA for charger current.
 *
 * The averaged and phased AtoD measurements for the battery voltage and current are
 * passed back in the batt_result and curr_result fields of the structure.  The current value passed
 * back has been converted to milliamps.
 */
#define POWER_IC_IOCTL_ATOD_BATT_AND_CURR \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x01), POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *)

/*!
 * @brief Converts set of channels.
 *
 * This command performs a conversion of a set of AtoD channels, taking one sample for each
 * channel converted. The measurements are phased where phasing is available.
 *
 * The command takes no inputs.
 *
 * The results for all of the converted channels are passed back to the caller in the
 * POWER_IC_ATOD_RESULT_GENERAL_CONVERSION_T structure passed by pointer to the ioctl() call.
 */
#define POWER_IC_IOCTL_ATOD_GENERAL \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x02), POWER_IC_ATOD_RESULT_GENERAL_CONVERSION_T *)

/*!
 * @brief Enables a hardware-timed conversion.
 *
 * This command is different from the other conversions in that no conversion is performed
 * within the duration of the command. This command sets the hardware to perform a conversion
 * of the battery voltage and current that is triggered relative to a transmit burst. Once
 * set, this command returns and the hardware waits for the transmit burst to occur. If
 * no transmit occurs, the conversion will never be performed. The conversion results can
 * be polled by using the read() system call on the power IC driver.
 *
 * The timing of the conversion (in/out of burst) is specified in the timing field of the structure.
 *
 * The command has no output other than the returned error code for the ioctl() call. The
 * status of the conversion and the results are retrieved via the read() system call on the
 * power IC device.
 */
#define POWER_IC_IOCTL_ATOD_BEGIN_CONVERSION \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x03), POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *)

/*!
 * @brief Cancels a hardware-timed conversion.
 *
 * This command disables a hardware-timed conversion previously set up by the
 * POWER_IC_IOCTL_ATOD_BEGIN_CONVERSION command. If no conversion has been set up
 * or a conversion has already been completed, the command will return an error.
 *
 * The command takes no inputs.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_ATOD_CANCEL_CONVERSION \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x04), int)

/*!
 * @brief Programs the AtoD phasing.
 *
 * This command programs the phasing applied to various AtoD conversion results. The
 * phasing applied is an array of 12 bytes in the same format as is used for all
 * products' AtoD phasing.
 *
 * The command takes a pointer to an array of phasing bytes.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 *
 * @note The driver will reject all phasing that is deemed to be unprogrammed. If
 * any slope byte is seen as 0x00 or 0xFF, the phasing will be rejected and the
 * driver will continue to use the previously programmed values.
 */
#define POWER_IC_IOCTL_ATOD_SET_PHASING \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x05), unsigned char *)

/*!
 * @brief Converts battery voltage and current for Phasing.
 *
 * This command performs a conversion of the battery voltage and current. The results of the
 * conversion are averaged and phasing is applied to the result (if available).
 *
 * The averaged and phased raw AtoD measurements for the battery voltage and current are
 * passed back in the batt_result and curr_result fields of the structure.  The current value passed
 * back is in DAC values.
 */
#define POWER_IC_IOCTL_ATOD_BATT_AND_CURR_PHASED \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x06), POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *)
        
 /*!
 * @brief Converts battery voltage and current and returns the unphased raw A2D measurements.
 *
 * This command performs a conversion of the battery voltage and current. The results of the
 * conversion are averaged only and not phased.
 *
 * The averaged and unphased raw AtoD measurements for the battery voltage and current are
 * passed back in the batt_result and curr_result fields of the structure.  The current value passed
 * back is in DAC values.
 */
#define POWER_IC_IOCTL_ATOD_BATT_AND_CURR_UNPHASED \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x07), POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *)       

/*!
 * @brief Converts a single channel.
 *
 * This command performs a conversion on a single AtoD channel. However, unlike the
 * POWER_IC_IOCTL_ATOD_SINGLE_CHANNEL the results are not averaged nor are they phased.
 * Instead, all of the samples will be passed back to the caller.
 *
 * The command takes the channel to be converted in the channel field of the passed
 * POWER_IC_ATOD_REQUEST_RAW_T structure.
 *
 * The command passes back all of the results in the results[] field of the passed structure
 * and number of samples taken will be recorded in the num_results field.
 *
 * @note This is really only intended for testing purposes. Since the results are unphased and
 * not averaged, making decisions on the results of this conversion is probably not a good idea.
 */
#define POWER_IC_IOCTL_ATOD_RAW \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x08), POWER_IC_ATOD_REQUEST_RAW_T *)
        
/*!
 * @brief Uses a single channel conversion on charger current.
 *
 * This command performs a conversion on a single AtoD channel for charger current. The results of the
 * conversion are averaged and phasing is applied to the result.
 *
 * The command takes the POWER_IC_ATOD_CHANNEL_CHRG_CURR to be converted in the channel field of the passed
 * POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T structure.
 *
 * The averaged and phased AtoD measurement on the charger current (in mA) is returned in the result field of the
 * structure.
 */
#define POWER_IC_IOCTL_ATOD_CHRGR_CURR \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_ATOD_BASE + 0x09), POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T *)
        
/* @} End of AtoD ioctls ------------------------------------------------------------------------- */

/*!
 * @name Audio ioctl() commands
 * @anchor ioctl_audio
 *
 * These are the commands that can be used through ioctl() to perform various audio-related
 * operations. The caller does not need to know which platform these commands are
 * being performed on.
 */

/* @{ */
/*!
 * @brief Used to set the audio connection mode.
 */
#define POWER_IC_IOCTL_AUDIO_CONN_MODE_SET \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_AUDIO_BASE + 0x00), POWER_IC_EMU_CONN_MODE_T *)
/*!
 * @brief Used to enable/disable the ceramic speaker
 *
 * This command is only supported for products that have a ceramic speaker.
 */
#define POWER_IC_IOCTL_AUDIO_CERAMIC_SPEAKER_EN \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_AUDIO_BASE + 0x01), bool)
/*!
 * @brief Used to change Audio Rx 0 register
 */
#define POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_RX_0 \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_AUDIO_BASE + 0x02), POWER_IC_AUDIO_SET_REG_MASK_T *)
/*!
 * @brief Used to change Audio Rx 1 register
 */
#define POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_RX_1 \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_AUDIO_BASE + 0x03), POWER_IC_AUDIO_SET_REG_MASK_T *)
/*!
 * @brief Used to change Audio Tx register
 */
#define POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_TX \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_AUDIO_BASE + 0x04), POWER_IC_AUDIO_SET_REG_MASK_T *)
/*!
 * @brief Used to change SSI Network register
 */
#define POWER_IC_IOCTL_AUDIO_SET_REG_MASK_SSI_NETWORK \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_AUDIO_BASE + 0x05), POWER_IC_AUDIO_SET_REG_MASK_T *)
/*!
 * @brief Used to change Audio Codec register
 */
#define POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_CODEC \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_AUDIO_BASE + 0x06), POWER_IC_AUDIO_SET_REG_MASK_T *)
/*!
 * @brief Used to change Audio Stereo DAC register
 */
#define POWER_IC_IOCTL_AUDIO_SET_REG_MASK_AUDIO_STEREO_DAC \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_AUDIO_BASE + 0x07), POWER_IC_AUDIO_SET_REG_MASK_T *)
/*!
 * @brief Used to enable/disable the external audio amplifier
 *
 * This command is only supported for the products that use an external audio amplifier.
 */
#define POWER_IC_IOCTL_EXT_AUDIO_AMP_EN \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_AUDIO_BASE + 0x08), unsigned char)
/* @} End of Audio ioctls.  -----------------------------------------------------------------------*/

/*!
 * @name Lighting ioctl() commands
 * @anchor ioctl_lights
 *
 * These are the commands that can be used through ioctl() to perform various operations.
 * on the phone's backlights/funlights. The caller does not need to know which platform these
 * commands are being performed on.
 */

/* @{ */
/*!
 * @brief Set the intensity of a backlight.
 *
 * Sets the intensity of the backlight requested to the percentage passed in.  See
 * LIGHTS_BACKLIGHT_IOCTL_T for more details.
 */
#define POWER_IC_IOCTL_LIGHTS_BACKLIGHTS_SET \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x00), LIGHTS_BACKLIGHT_IOCTL_T *)
/*!
 * @brief Grants the control of a fun lights region to an app.
 *
 * Enables the regions specified in the region mask for control by the app which is passed in.
 * When bits in the mask are clear access is returned to lower priority apps.  Please note an app
 * will only be granted access when a lower priority task currently has access.  If a higher priority
 * task currently has control the requesting app will be granted access once all higher priority
 * tasks have released control.  If a higher priority task has control the request for access
 * will be queued, if this is not desired the function must be called again with the request bit
 * cleared.  See LIGHTS_FL_SET_T for more details on the parameters.
 */
#define POWER_IC_IOCTL_LIGHTS_FL_SET_CONTROL \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x01), LIGHTS_FL_SET_T *)
/*!
 * @brief Sets the value of a fun lights region and updates the lights if the app has control.
 *
 * Assigns colors to regions for the app which is passed in. If the app is not currently active
 * the color assignment is cached but will not be active. In the case the LED is not active
 * the corresponding bit in the return mask will be clear.  See LIGHTS_FL_UPDATE_T for more
 * details on the parameters.
 */
#define POWER_IC_IOCTL_LIGHTS_FL_UPDATE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x02), LIGHTS_FL_UPDATE_T *)

/*!
 * @brief Set the intensity of a backlight.
 *
 * Sets the intensity of the backlight requested to the percentage passed in.  See
 * LIGHTS_BACKLIGHT_IOCTL_T for more details on the parameters.
 */
#define POWER_IC_IOCTL_LIGHTS_BACKLIGHTS_STEP_SET \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x03), LIGHTS_BACKLIGHT_IOCTL_T *)
/*!
 * @brief Get the number of backlight steps supported by the hardware.
 *
 * Returns the number of brightness steps implemented in the hardware driver for a type of
 * backlight. The number of steps will include the value for off.
 */
#define POWER_IC_IOCTL_LIGHTS_BACKLIGHT_GET_STEPS \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x04), LIGHTS_BACKLIGHT_IOCTL_T *)
/*!
 * @brief  Convert a step to a percentage based on the current hardware.
 *
 * Returns a percentage based on the step and backlight passed in.  The return value is suitable
 * for use by POWER_IC_IOCTL_LIGHTS_BACKLIGHTS_SET.  See LIGHTS_BACKLIGHT_IOCTL_T for more details
 * on the parameters.
 */
#define POWER_IC_IOCTL_LIGHTS_BACKLIGHT_STEPS_TO_PERCENT \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x05), LIGHTS_BACKLIGHT_IOCTL_T *)

/*!
 * @brief  Get the current backlight step value of the required region.
 *
 * Returns the step based on the region passed in. See LIGHTS_BACKLIGHT_IOCTL_T for more details
 * on the parameters.
 */
#define POWER_IC_IOCTL_LIGHTS_BACKLIGHT_GET_STEPS_VAL \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x06), LIGHTS_BACKLIGHT_IOCTL_T *)

/*!
 * @brief  Get a percentage based on the step value of a particular region.
 *
 * Returns a percentage based on the step and backlight passed in.See LIGHTS_BACKLIGHT_IOCTL_T for more details
 * on the parameters.
 */
#define POWER_IC_IOCTL_LIGHTS_BACKLIGHT_GET_VAL \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x07), LIGHTS_BACKLIGHT_IOCTL_T *)

/*!
 * @brief  Enable the ambient light sensor circuit.
 *
 * The procedure is like followings:
 * 1. Enable the ambient light sensor using the POWER_IC_IOCTL_LIGHTS_LIGHT_SENSOR_START ioctl.
 * 2. Wait the correct amount of time for the circuit to settle based on the hardware in use.
 * 3. Read the ambient light sensor atod value using POWER_IC_IOCTL_LIGHTS_LIGHT_SENSOR_GET_LUMA.
 * 4. Disable the ambient light sensor using the POWER_IC_IOCTL_LIGHTS_LIGHT_SENSOR_STOP ioctl.
 *
 * @note The sensor must be disabled after reading in order to maximize battery life.
 */
#define POWER_IC_IOCTL_LIGHTS_LIGHT_SENSOR_START \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x08),char)

/*!
 * @brief Read ambient light sensor ATOD value.
 *
 * Read out Atod value of the ambient light sensor and return to the user.
 */
#define POWER_IC_IOCTL_LIGHTS_LIGHT_SENSOR_GET_LUMA\
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x09),int)

/*!
 * @brief Disable the ambient light sensor circuit
 */
#define POWER_IC_IOCTL_LIGHTS_LIGHT_SENSOR_STOP\
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x0A),char)

/*!
 * @brief Set morphing mode
 */
#define POWER_IC_IOCTL_LIGHTS_SET_MORPHING_MODE\
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_LIGHTS_BASE + 0x0B),MORPHING_MODE_E)

/* @} End of lighting ioctls.  -------------------------------------------------------------------- */

/*!
 * @anchor ioctl_charger
 * @name Charger ioctl() commands
 *
 * These are the commands that can be used through ioctl() to control the charging hardware.
 */

/* @{ */

/*!
 * @brief Sets the charge voltage.
 *
 * This command programs the maximum voltage that the battery will be charged up to.
 * While it is expected that the main 4.2V level used will never change, there are
 * potentially some differences in the other levels used depending on the hardware type
 * and revision.
 *
 * The command takes an int, which is the requested setting for VCHRG as per the hardware
 * specification.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_CHARGER_SET_CHARGE_VOLTAGE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x00), int)

/*!
 * @brief Sets the main charge current.
 *
 * This command programs the maximum current that will be fed into the battery while
 * charging. This is only an upper limit - the charging hardware is free to charge
 * at a limit lower than that set depending on the battery voltage and how much current
 * an attached charger can supply.
 *
 * The command takes an int, which is the requested setting for ICHRG as per the hardware
 * specification. For the most part this translates into 100's of milliamps of current,
 * but there are discontinuities at the top end of the range.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 *
 * @note The main charge current and trickle charge current settings are mutually exclusive.
 * Setting this control will result in the trickle charge current being set to zero.
 */
#define POWER_IC_IOCTL_CHARGER_SET_CHARGE_CURRENT \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x01), int)

/*!
 * @brief Sets the trickle charge current.
 *
 * This command programs the maximum trickle current that will be fed into the battery
 * while charging. This is only an upper limit - the charging hardware is free to charge
 * at a limit lower than that set depending on the battery voltage and how much current
 * an attached charger can supply.
 *
 * The command takes an int, which is the requested setting for ICHRG_TR as per the hardware
 * specification. This should translate to roughly 12 mA of current per count.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 *
 * @note The trickle charge current and main charge current settings are mutually exclusive.
 * Setting this control will result in the main charge current being set to zero.
 */
#define POWER_IC_IOCTL_CHARGER_SET_TRICKLE_CURRENT \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x02), int)

/*!
 * @brief Sets the power path.
 *
 * This command sets the path that supplies power to the phone. The hardware can be set
 * to either supply current from the battery (current share) or from the attached charger
 * (dual-path).
 *
 * The command takes an POWER_IC_CHARGER_POWER_PATH_T, indicates whether the hardware should
 * be set in current-share or dual-path mode.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 *
 * @note Setting current-share mode when no battery is attached will result in an instant
 * powerdown.
 */
#define POWER_IC_IOCTL_CHARGER_SET_POWER_PATH \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x03), POWER_IC_CHARGER_POWER_PATH_T)

/*!
 * @brief Reads overvoltage state.
 *
 * This function reads the charger overvoltage sense bit from the hardware and returns it
 * to the caller.
 *
 * The command expects a pointer to an int, where the command will store the read state of the
 * overvoltage detect hardware. This will be zero if no overvoltage condition existsm and
 * greater than zero if overvoltage has been detected.
 *
 * This command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_CHARGER_GET_OVERVOLTAGE \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x04), int *)

/*!
 * @brief Resets the overvoltage hardware.
 *
 * This function is used to reset the overvoltage hardware after a problem is detected.
 * once an overvoltage condition occurs, charging will be disabled in hardware until it is
 * reset.
 *
 * This commend takes no inputs.
 *
 * This command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_CHARGER_RESET_OVERVOLTAGE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x05), int)

/*!
 * @brief Sets the USB Charger State.
 *
 * This command sets the the USB Charger State so that SBCM can determine the amount of current
 * allowed to be pulled from the port.
 *
 * The command takes an POWER_IC_CHARGER_CHRG_STATE_T, indicates the state of the charger
 *
 * The command has no output other than the returned error code for the ioctl() call.
 *
 */
#define POWER_IC_IOCTL_CHARGER_SET_USB_STATE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x06), POWER_IC_CHARGER_CHRG_STATE_T)

/*!
 * @brief Reads the USB Charger State.
 *
 * This function reads the state of the charger and returns it to the user.
 *
 * The command expects a pointer to POWER_IC_CHARGER_CHRG_STATE_T, where the command will
 * store the read state of the charger.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 *
 */
#define POWER_IC_IOCTL_CHARGER_GET_USB_STATE \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x07), POWER_IC_CHARGER_CHRG_STATE_T *)

/*!
 * @brief Sets the power cut.
 *
 * This command sets the power cut enable bit.
 *
 * The command takes an int, which is the requested setting for PCEN.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 */
#define POWER_IC_IOCTL_CHARGER_SET_POWER_CUT \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x08), int)

/*!
 * @brief Reads the power cut state.
 *
 * This function reads the state of power cut and returns it to the user.
 *
 * The command expects a pointer to int, where the command will store the read state of power cut.
 *
 * The command has no output other than the returned error code for the ioctl() call.
 *
 */
#define POWER_IC_IOCTL_CHARGER_GET_POWER_CUT \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_CHARGER_BASE + 0x09), int *)

/* @} End of charger control ioctls.  -------------------------------------------------------------*/

/*!
 * @name Test ioctl() commands
 * @anchor ioctl_tcmd
 *
 * These are the commands that are not normally used as part of the phone's normal
 * operation, but are intended for use in test commands.
 *
 * @todo The tcmd ioctl commands need to be better documented.
 */
/* @{ */

/*! Configures the transceiver to the requested state, if an out of range state is passed
 *  the off state is used.  */
#define POWER_IC_IOCTL_CMD_TCMD_EMU_TRANS_STATE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x00), EMU_XCVR_T)
/*! Sets the MONO adder to the requested state, if an invalid state is requested nothing
 * is done to hardware and -EFAULT is returned to user. */
#define POWER_IC_IOCTL_CMD_TCMD_MONO_ADDER_STATE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x01), POWER_IC_TCMD_MONO_ADDER_T)
/*! Returns the current value of the IDFLOATS bit */
#define POWER_IC_IOCTL_CMD_TCMD_IDFLOATS_READ \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x02), unsigned int *)
/*! Returns the current value of the IDGNDS bit */
#define POWER_IC_IOCTL_CMD_TCMD_IDGNDS_READ \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x03), unsigned int *)
/*! Returns the current value of the headset detect bit */
#define POWER_IC_IOCTL_CMD_TCMD_A1SNS_READ \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x04), unsigned int *)
/*! Returns the current value of the mic bias MB2SNS bit */
#define POWER_IC_IOCTL_CMD_TCMD_MB2SNS_READ \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x05), unsigned int *)
/*! Passing 1 sets the reverse mode to on passing 0 sets it to off */
#define POWER_IC_IOCTL_CMD_TCMD_REVERSE_MODE_STATE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x06), int)
/*! Returns the current value of the bits that determine the source of the VUSB input */
#define POWER_IC_IOCTL_CMD_TCMD_VUSBIN_READ \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x07), unsigned int *)
/*! Set the bits that determine the source of the VUSB input */
#define POWER_IC_IOCTL_CMD_TCMD_VUSBIN_STATE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x08), int)
/*! Returns 1 if the external 32KHz oscillator is present and 0 if it is not */
#define POWER_IC_IOCTL_CMD_TCMD_CLKSTAT_READ \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x09), unsigned int *)
/*! Passing 1 Enables the charging of the coincell passing 0 sets it to no charge */
#define POWER_IC_IOCTL_CMD_TCMD_COINCHEN_STATE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x0A), int)
/*! Sets the CONN mode to the requested state, if an invalid state is requested nothing
 * is done to hardware and -EFAULT is returned to user. */
#define POWER_IC_IOCTL_CMD_TCMD_EMU_CONN_STATE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x0B), POWER_IC_EMU_CONN_MODE_T)
/*! Sets the Atlas external regulator voltage output(High - 3.1V Low- 0V) GPO4 */
#define POWER_IC_IOCTL_CMD_TCMD_GPO4_EXT_VOLTAGE_OUTPUT \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x0C), int)
#define POWER_IC_IOCTL_CMD_TCMD_FLASH_MODE_WRITE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x0D), int)
/*! Sets the Atlas codec receive gain and stereo dac gain settings. */
#define POWER_IC_IOCTL_CMD_TCMD_WRITE_OGAIN \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x0E), int)
/*! Returns the current value of the stereo dac gain setting. */
#define POWER_IC_IOCTL_CMD_TCMD_READ_OGAIN \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x0F), int *)
/*! Sets ATLAS VMMC2 */
#define POWER_IC_IOCTL_CMD_TCMD_VMMC2_SET \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x10), int)
/*! Sets ATLAS VDIG */
#define POWER_IC_IOCTL_CMD_TCMD_VDIG_SET \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x11), int)
/*! Sets ATLAS VESIM */
#define POWER_IC_IOCTL_CMD_TCMD_VESIM_SET \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x12), int)
/*! Sets ATLAS codec digital loopback */
#define POWER_IC_IOCTL_CMD_TCMD_WRITE_DIGITAL_LPB\
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_TCMD_BASE + 0x13), int)
/* @} End of tcmd ioctls  -------------------------------------------------------------------------*/

/*!
 * @name BRT ioctl() commands
 * @anchor ioctl_brt
 *
 * These are the commands used to perform reading on battery rom data
 *
 * @todo The brt ioctl commands need to be better documented.
 */
/* @{ */

/*! Returns the current battery rom data */
#define POWER_IC_IOCTL_CMD_BRT_DATA_READ \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_BRT_BASE + 0x00), POWER_IC_BRT_DATA_REQUEST_T*)
/*! Returns the current value of the unique id in BRT */
#define POWER_IC_IOCTL_CMD_BRT_UID_READ \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_BRT_BASE + 0x01), POWER_IC_BRT_UID_REQUEST_T*)
/* @} End of brt ioctls  -------------------------------------------------------------------------*/



/*!
 * @anchor ioctl_types
 * @name ioctl() types
 *
 * These are the types of data passed in various ioctl() requests.
 */

/* @{ */

/*!
 * @brief Type of data used for power IC register access.
 *
 * This structure is passed as the final parameter to ioctl() when using the commands:
 * POWER_IC_IOCTL_READ_REG, POWER_IC_IOCTL_WRITE_REG, POWER_IC_IOCTL_READ_REG_BITS,
 * POWER_IC_IOCTL_WRITE_REG_BITS, or POWER_IC_IOCTL_WRITE_REG_MASK.
 */
typedef struct
{
    POWER_IC_REGISTER_T reg;        /*!< Register to be read/written. */
    int index;                      /*!< Index of first bit for read/write bits operations. */
    int num_bits;                   /*!< Number of bits to be written for r/w bits, or bitmask for write mask. */
    int value;                      /*!< Value written for writes, value read put here for reads. */
} POWER_IC_REG_ACCESS_T;

/*!
 * @brief Type of data used for power IC backup memory access.
 *
 * This structure is passed as the final parameter to ioctl() when using the commands:
 * POWER_IC_IOCTL_READ_BACKUP_MEMORY and POWER_IC_IOCTL_WRITE_BACKUP_MEMORY.
 */
typedef struct
{
    int value;                      /*!< Value written for writes, value read put here for reads. */
    POWER_IC_BACKUP_MEMORY_ID_T id; /*!< ID of Backup Memory element to be read/written. */
} POWER_IC_BACKUP_MEM_ACCESS_T;

/*!
 * @brief Chipsets reported by POWER_IC_IOCTL_GET_HARDWARE_INFO.
 *
 * This is the set of chipsets that can be reported in the POWER_IC_HARDWARE_T retrieved by the
 * POWER_IC_IOCTL_GET_HARDWARE_INFO command. Under normal circumstances, POWER_IC_CHIPSET_UNKNOWN
 * should never be reported.
 */
typedef enum
{
    POWER_IC_CHIPSET_ATLAS,

    /* Add new chipsets above this line. */
    POWER_IC_CHIPSET_UNKNOWN = 0xFF
} POWER_IC_CHIPSET_T;

/*!
 * @brief Type returned by POWER_IC_IOCTL_GET_HARDWARE_INFO.
 *
 * This is all of the information retrieved by the POWER_IC_IOCTL_GET_HARDWARE_INFO command.
 * For the two sets of revision history, revision1 will represent the main power IC (e.g.
 * Atlas) and revision2 will represent other hardware alongside it.
 *
 * @note For some hardware types, revision information is not available for the hardware. In
 * these cases, the corresponding revision will be set to POWER_IC_HARDWARE_NO_REV_INFO.
 */
typedef struct
{
    POWER_IC_CHIPSET_T chipset;  /*!< Indicates type of hardware. */
    int revision1;               /*!< Revision info for primary IC. */
    int revision2;               /*!< Revision info for secondary IC. */
    POWER_IC_FORM_FACTOR_T form_factor;   /*!< Phone style.*/
    char soc_bt_shared;          /*!< Shared SOC and BT LED region. */
} POWER_IC_HARDWARE_T;

/*! Revision info in POWER_IC_HARDWARE_T will be set to this if no information is available. */
#define POWER_IC_HARDWARE_NO_REV_INFO    0xFFFFFFFF

/* The following types are used in the AtoD interface. */

/*!
 * @brief General AtoD conversion results.
 *
 * This type is the set of AtoD conversions returned from a general conversion request.
 * this is passed by address as the 3rd parameter to a POWER_IC_IOCTL_ATOD_GENERAL
 * ioctl() request.
 */
typedef struct
{
    int coin_cell;
    int battery;
    int bplus;
    int mobportb;
    int charger_id;
    int temperature;
    POWER_IC_ATOD_AD6_T ad6_sel; /*!< When it is set to zero, indicates to take atod on PA temperature */
} POWER_IC_ATOD_RESULT_GENERAL_CONVERSION_T;

/*!
 * @brief Structure passed to ioctl() for a single channel request.
 *
 * This type is passed by pointer as the 3rd parameter of a
 * POWER_IC_IOCTL_ATOD_SINGLE_CHANNEL ioctl() request.
 */
typedef struct
{
    POWER_IC_ATOD_CHANNEL_T channel; /*!< The channel that should be converted. */
    int                     result;  /*!< The result of the conversion, */
} POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T;

/*!
 * @brief Structure passed to ioctl() for a battery/current request.
 *
 * This type is passed by pointer as the 3rd parameter of a
 * POWER_IC_IOCTL_ATOD_BATT_AND_CURR ioctl() request.
 */
typedef struct
{
    POWER_IC_ATOD_TIMING_T timing;          /*!< The timing for the conversion. */
    int timeout_secs;                       /*!< If a non-immediate conversion, timeout after this number of seconds.*/
    POWER_IC_ATOD_CURR_POLARITY_T polarity; /*!< The direction in which current should be sampled. Ignored for ATLAS. */
    int batt_result;                        /*!< The battery result from the conversion. */
    int curr_result;                        /*!< The current result from the conversion. */
} POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T;

/*!
 * @brief Structure passed to ioctl() for a raw conversion request.
 *
 * This type is passed by pointer as the 3rd parameter of a
 * POWER_IC_IOCTL_ATOD_RAW ioctl() request.
 */
typedef struct
{
    POWER_IC_ATOD_CHANNEL_T channel; /*!< The channel that should be converted. */
    int                     results[POWER_IC_ATOD_NUM_RAW_RESULTS]; /*!< Results of conversion. */
    int                     num_results; /*!< Number of the results being passed back. */
} POWER_IC_ATOD_REQUEST_RAW_T;

/*!
 * @brief Structure passed to ioctl() for audio set_reg_mask requests.
 *
 * This type is passed by pointer as the 3rd parameter of any
 * POWER_IC_IOCTL_AUDIO_SET_REG_MASK_* ioctl() request.
 */
typedef struct
{
    unsigned int mask;   /* Mask of bits to be modified in the register being changed. */
    unsigned int value;  /* New values for the bits that are being modified. */
} POWER_IC_AUDIO_SET_REG_MASK_T;

/*!
 * @brief Structure passed to ioctl() for battery data request.
 *
 * This type is passed by pointer as the 3rd parameter of a
 * POWER_IC_IOCTL_CMD_BRT_DATA_READ ioctl() request.
 */
typedef struct
{
    BRT_DATA_T             brt_type; /* the type of battery rom data. */
    unsigned char          brt_data[BRT_BYTE_SIZE]; /* Result of battery rom data read */
} POWER_IC_BRT_DATA_REQUEST_T;

/*!
 * @brief Structure passed to ioctl() for battery unique id request.
 *
 * This type is passed by pointer as the 3rd parameter of a
 * POWER_IC_IOCTL_CMD_BRT_DATA_READ ioctl() request.
 */
typedef struct
{
    BRT_DATA_T             brt_type; /* the type of battery rom data. */
    unsigned char          brt_uid[BRT_UID_SIZE]; /* Result of battery unique id read */
} POWER_IC_BRT_UID_REQUEST_T;


/*! Defines the possible modes of the MONO adder, this table is tied to the
 * hardware, this case supports Atlas.  The Mono adder can be used
 * to sum the left and right channels of the stereo DAC or signals supplied
 * to the left and right PGA inputs. The Mono adder can then attenuate the
 * summed signals by 0dB, 3dB or 6dB and an identical monophonic signal
 * to the output amplifiers.
 *
 */

typedef enum
{
    POWER_IC_TCMD_MONO_ADDER_STEREO,        /*!< 00 - Right PGA and Left PGA Separated (Stereo) */
    POWER_IC_TCMD_MONO_ADDER_R_L,           /*!< 01 - Right PGA + Left PGA */
    POWER_IC_TCMD_MONO_ADDER_R_L_3DB_LOSS,  /*!< 10 - (Right PGA + Left PGA) -3 dB */
    POWER_IC_TCMD_MONO_ADDER_R_L_6DB_LOSS,  /*!< 11 - (Right PGA + Left PGA) -6 dB */

    POWER_IC_TCMD_MONO_ADDER_NUM
} POWER_IC_TCMD_MONO_ADDER_T;


/*******************************************************************************************
 * EMU proc Universal constants for SCMA-11 and BUTE platform
 *
 *    These are constants that are universal to use of the EMU proc driver from both kernel
 *    and user space..
 ******************************************************************************************/

/*! Define EMU proc event to be reported to User Space */
#define POWER_IC_EVENT_INT_VBUS 0x01
#define POWER_IC_EVENT_INT_ID   0x02
#define POWER_IC_EVENT_INT_SE1  0x04


/* Base of all the ioctl() commands that will be handled by the EMU proc . */
#define POWER_IC_IOC_CMD_EMU_GLUE_BASE           0x00
/* Last of the range of ioctl() commands reserved for the EMU proc*/
#define POWER_IC_IOC_CMD_EMU_GLUE_LAST_CMD      (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x1F)

/*******************************************************************************************
 * EMU proc ioctls
 *
 * All of the ioctl commands supported by the driver are in this section.
 *
 ******************************************************************************************/

/*!
 * @name EMU_GLUE ioctl() commands
 * @anchor ioctl_emu_glue
 *
 * These are the commands of the EMU proc ioctl for the Common Algortihm EMU glue
 *
 */
/* @{ */
/*! Returns the EMU state information */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_READ_SENSE\
        _IOWR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x00), POWER_IC_EMU_GLUE_READ_SENSE_T*)
/*! Sets the EMU lockout changes */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_LOCKOUT_CHANGES \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x01), unsigned long)
/*! Sets the VBUS 5K pull down */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_VBUS_5K_PD \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x02), unsigned long)
/*! Sets the VBUS 70K pull down */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_VBUS_70K_PD \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x03), unsigned long)
/*! Sets the reverse mode */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_REVERSE_MODE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x04), unsigned long)
/*! Sets the ID pull up */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_ID_PU \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x05), unsigned long)
/*! Sets the ID pull down */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_ID_PD \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x06), unsigned long)
/*! Sets the ID stereo pull down */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_ID_STEREO_PU \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x07), unsigned long)
/* Sets the conn mode */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_CONN_MODE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x08), POWER_IC_EMU_CONN_MODE_T)
/* Sets the dplus 150K pull up */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_DPLUS_150K_PU \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x09), unsigned long)
/* Sets the dplus 1,5K pull up */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_DPLUS_1_5K_PU \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x0A), unsigned long)
/* Sets the low speed mode */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_LOW_SPEED_MODE \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x0B), unsigned long)
/* Sets the USB suspend */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_USB_SUSPEND \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x0C), unsigned long)
/* Sets the transceiver params */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_TRANSCEIVER_PARAMS \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x0D), POWER_IC_EMU_GLUE_TRANSCEIVER_PARAMS_T *)
/* Sets the USB suspend */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_ID_INT_MASK \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x0E), unsigned long)
/* Sets the USB suspend */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_VBUS_INT_MASK \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x0F), unsigned long)
/* Sets the USB suspend */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_SE1_INT_MASK \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x10), unsigned long)
/*! Sets the FET_CTRL and FET_OVRD bit */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_SET_FET_CONTROL \
        _IOW(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x11), unsigned long)
/*! Gets the FET_CTRL and FET_OVRD bit */
#define POWER_IC_IOCTL_CMD_EMU_GLUE_GET_FET_CONTROL \
        _IOR(POWER_IC_MAJOR_NUM, (POWER_IC_IOC_CMD_EMU_GLUE_BASE + 0x12), POWER_IC_FET_CONTROL_T *)
/* @} End of emu glue  ioctls  -------------------------------------------------------------------------*/

/*!
 * @brief Structure passed to emu glue read sense ioctl().
 *
 * This type is passed by pointer as the 3rd parameter of a
 * POWER_IC_IOCTL_CMD_EMU_GLUE_READ_SENSE ioctl() request.
 */
typedef struct
{
    unsigned long sense;               /* Current state of the EMU state information */
    unsigned char clear_int_flags;     /* Specify if the EMU related interrupt should be cleared */
} POWER_IC_EMU_GLUE_READ_SENSE_T;

/*!
 * @brief Used with the POWER_IC_IOCTL_CMD_EMU_GLUE_GET_FET_CONTROL I/O control
 *
 */
typedef enum
{                                                 /* FET_CTRL FET_OVRD */
    POWER_IC_FET_CONTROL_HARDWARE_DUAL_PATH,      /*        0        0 */
    POWER_IC_FET_CONTROL_SOFTWARE_DUAL_PATH,      /*        0        1 */
    POWER_IC_FET_CONTROL_HARDWARE_INVALID,        /*        1        0 */
    POWER_IC_FET_CONTROL_SOFTWARE_CURRENT_SHARE,  /*        1        1 */
    
    POWER_IC_FET_CONTROL__NUM
} POWER_IC_FET_CONTROL_T;

/*!
 * @brief Used with the EMU_GLUE_set_transceiver_params function
 *
 */
typedef enum
{
    POWER_IC_EMU_VUSB_IN_VINBUS, /*00*/
    POWER_IC_EMU_VUSB_IN_VBUS,   /*01*/
    POWER_IC_EMU_VUSB_IN_BPLUS,  /*10*/
    POWER_IC_EMU_VUSB_IN_VBUS2,  /*11*/

    POWER_IC_EMU_VUSB_IN__NUM
} POWER_IC_EMU_VUSB_IN_T;

/*!
 * @brief Structure passed to EMU ioctl() for emu glue set transceiver params .
 *
 * This type is passed by pointer as the 3rd parameter of a
 * POWER_IC_IOCTL_CMD_EMU_GLUE_SET_TANSCEIVER_PARAMS ioctl() request.
 */
typedef struct
{
    unsigned long xcvr_en;             /*!< transceiver enable */
    unsigned long vusb_en;             /*!< VUSB enable */
    POWER_IC_EMU_VUSB_IN_T vusb_input_source;   /*!< VUSB input source */
    unsigned short vusb_voltage;       /*!< VUSB voltage */
} POWER_IC_EMU_GLUE_TRANSCEIVER_PARAMS_T;

#endif /* __POWER_IC_H__ */
