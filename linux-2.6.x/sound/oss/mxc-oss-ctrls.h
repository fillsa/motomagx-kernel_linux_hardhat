/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 *
 * Copyright (C) 2005 Freescale Semiconductor, Inc. All rights reserved.
 *
 * History :
 *        04/06/29 : Creation
 */

/*!
 * @defgroup SOUND_OSS_MXC_PMIC MXC and PMIC Sound Driver for OSS
 */

/*!
 * @file mxc-oss-ctrls.h
 * @brief IOCTL definitions for the MXC OSS sound driver.
 *
 * @ingroup SOUND_OSS_MXC_PMIC
 */

#ifndef _MXC_OSS_CTRLS_H
#define _MXC_OSS_CTRLS_H

/*!
 * Allows the user to add extra IOCTL to get access to MXC registers
 * For debugging/testing only.
 */
#include "mxc-oss-dbg.h"

/*!
 * To set the PMIC adder configuration, use the audio control 
 * SNDCTL_PMIC_WRITE_OUT_MIXER.\n 
 * Possible arguments are:
 * @see PMIC_AUDIO_ADDER_STEREO
 * @see PMIC_AUDIO_ADDER_STEREO_OPPOSITE
 * @see PMIC_AUDIO_ADDER_MONO
 * @see PMIC_AUDIO_ADDER_MONO_OPPOSITE \n 
 * \n To get the adder configuration use :
 * @see SNDCTL_PMIC_READ_OUT_ADDER
*/
#define SNDCTL_PMIC_WRITE_OUT_ADDER     _SIOWR('Z', 7, int)

/*!
 * To get the PMIC adder configuration, use the audio control 
 * SNDCTL_PMIC_READ_OUT_MIXER.\n 
 * Possible returned values are :
 * @see PMIC_AUDIO_ADDER_STEREO
 * @see PMIC_AUDIO_ADDER_STEREO_OPPOSITE
 * @see PMIC_AUDIO_ADDER_MONO
 * @see PMIC_AUDIO_ADDER_MONO_OPPOSITE \n 
 * \n To set the adder configuration use :
 * @see SNDCTL_PMIC_WRITE_OUT_ADDER
*/
#define SNDCTL_PMIC_READ_OUT_ADDER        _SIOR('Z', 6, int)

/*! 
 * Argument for the PMIC adder configuration
 * @see SNDCTL_PMIC_WRITE_OUT_ADDER
 * @see SNDCTL_PMIC_READ_OUT_ADDER
 */
#define PMIC_AUDIO_ADDER_STEREO                1
/*! 
 * Argument for the PMIC adder configuration
 * @see SNDCTL_PMIC_WRITE_OUT_ADDER
 * @see SNDCTL_PMIC_READ_OUT_ADDER
 */
#define PMIC_AUDIO_ADDER_STEREO_OPPOSITE       2
/*! 
 * Argument for the PMIC adder configuration
 * @see SNDCTL_PMIC_WRITE_OUT_ADDER
 * @see SNDCTL_PMIC_READ_OUT_ADDER
 */
#define PMIC_AUDIO_ADDER_MONO                  4
/*! 
 * Argument for the PMIC adder configuration
 * @see SNDCTL_PMIC_WRITE_OUT_ADDER
 * @see SNDCTL_PMIC_READ_OUT_ADDER
 */
#define PMIC_AUDIO_ADDER_MONO_OPPOSITE         8

/*!
 * To get the PMIC balance configuration, use the audio control 
 * SNDCTL_PMIC_READ_OUT_BALANCE.\n 
 * Range is 0 (-21 dB left) to 100 (-21 dB right), linear, 3dB step ; 
 * 50 is no balance.
 * \n    Examples:
 * \n      0 : -21dB left   50 : balance deactivated   100 : -21 dB right
 * @see SNDCTL_PMIC_WRITE_OUT_BALANCE 
*/
#define SNDCTL_PMIC_READ_OUT_BALANCE      _SIOR('Z', 8, int)

/*!
 * To set the PMIC balance configuration, use the audio control 
 * SNDCTL_PMIC_WRITE_OUT_BALANCE.\n 
 * Range is 0 (-21 dB left) to 100 (-21 dB right), linear, 3dB step ; 
 * 50 is no balance.
 * \n    Examples:
 * \n      0 : -21dB left   50 : balance deactivated   100 : -21 dB right
 * @see SNDCTL_PMIC_READ_OUT_BALANCE
 */
#define SNDCTL_PMIC_WRITE_OUT_BALANCE      _SIOWR('Z', 9, int)

/*!
 * To get the Voice CODEC microphone bias configuration, use the audio control 
 * SNDCTL_PMIC_READ_IN_BIAS.\n 
 * Possible returned values are :
 * \n         0 : bias disabled
 * \n         1 : bias enabled
 * @see SNDCTL_PMIC_WRITE_IN_BIAS
 */
#define SNDCTL_PMIC_READ_IN_BIAS           _SIOR('Z', 10, int)

/*!
 * To set the Voice Codec microphone bias configuration, use the audio control
 * SNDCTL_PMIC_WRITE_IN_BIAS.\n 
 * Possible arguments are :
 * \n         0 : to disable the bias
 * \n         1 : to enable the bias
 * @see SNDCTL_PMIC_READ_IN_BIAS
 */
#define SNDCTL_PMIC_WRITE_IN_BIAS           _SIOWR('Z', 11, int)

/*!
 * To set the Voice CODEC filter configuration, use the audio control 
 * SNDCTL_PMIC_WRITE_CODEC_FILTER.
 * The new configuration replaces the old one.\n 
 * Possible arguments are :
 * @see PMIC_CODEC_FILTER_DISABLE
 * @see PMIC_CODEC_FILTER_HIGH_PASS_IN
 * @see PMIC_CODEC_FILTER_HIGH_PASS_OUT
 * @see PMIC_CODEC_FILTER_DITHERING \n 
 * \n To get the codec filter configuration, use :
 * @see SNDCTL_PMIC_READ_CODEC_FILTER
 */
#define SNDCTL_PMIC_WRITE_CODEC_FILTER      _SIOWR('Z', 20, int)

/*!
 * To get the Voice CODEC filter configuration, use the audio control
 * SNDCTL_PMIC_READ_CODEC_FILTER.
 * The new configuration replaces the old one.\n 
 * Possible returned values are :
 * @see PMIC_CODEC_FILTER_DISABLE
 * @see PMIC_CODEC_FILTER_HIGH_PASS_IN
 * @see PMIC_CODEC_FILTER_HIGH_PASS_OUT
 * @see PMIC_CODEC_FILTER_DITHERING \n 
 * \n To set the codec filter configuration, use :
 * @see SNDCTL_PMIC_WRITE_CODEC_FILTER
 */
#define SNDCTL_PMIC_READ_CODEC_FILTER       _SIOR('Z', 21, int)

/*!
 * Argument for the PMIC codec filter configuration
 * @see SNDCTL_PMIC_WRITE_CODEC_FILTER 
 * @see SNDCTL_PMIC_READ_CODEC_FILTER 
 */
#define PMIC_CODEC_FILTER_DISABLE              0
/*!
 * Argument for the PMIC codec filter configuration
 * @see SNDCTL_PMIC_WRITE_CODEC_FILTER 
 * @see SNDCTL_PMIC_READ_CODEC_FILTER 
 */
#define PMIC_CODEC_FILTER_HIGH_PASS_IN         1
/*!
 * Argument for the PMIC codec filter configuration
 * @see SNDCTL_PMIC_WRITE_CODEC_FILTER 
 * @see SNDCTL_PMIC_READ_CODEC_FILTER 
 */
#define PMIC_CODEC_FILTER_HIGH_PASS_OUT        2
/*!
 * Argument for the PMIC codec filter configuration
 * @see SNDCTL_PMIC_WRITE_CODEC_FILTER 
 * @see SNDCTL_PMIC_READ_CODEC_FILTER 
 */
#define PMIC_CODEC_FILTER_DITHERING            4

/*!
 * Argument for the system audio clocking selection
 * @see MXC_AUDIO_CLOCKING_SSI_MASTER
 * @see SNDCTL_CLK_SET_MASTER
 */
#define MXC_AUDIO_CLOCKING_PMIC_MASTER         0

/*!
 * Argument for the system audio clocking selection
 * @see MXC_AUDIO_CLOCKING_PMIC_MASTER
 * @see SNDCTL_CLK_SET_MASTER
 */
#define MXC_AUDIO_CLOCKING_SSI_MASTER           1

/*
 * To set the master/slave clock configuration, use the audio control 
 * SNDCTL_CLK_SET_MASTER.\n 
 * Possible arguments are :
 * \n         1 : for SSI master, PMIC slave
 * \n         2 : for PMIC master, SSI slave
 * Possible returned values are :
 * \n         1 : SSI master, PMIC slave
 * \n         2 : PMIC master, SSI slave
 */
#define SNDCTL_CLK_SET_MASTER                   _SIOR('Z', 30, int)

#endif				/* _MXC_OSS_CTRLS_H */
