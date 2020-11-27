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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 */

 /*!
  * @file       mxc_audio_mc13783.h
  * @brief      Internal macros and definitions used by the implementation of
  *             the MXC mc13783 OSS audio driver.
  *
  * @ingroup    SOUND_DRV
  */

#ifndef __MXC_AUDIO_MC13783_H__
#define __MXC_AUDIO_MC13783_H__

#ifdef __KERNEL__

#include <dam/dam_types.h>

#define AUDIO_PLAYBACK                                  0x0
#define AUDIO_RECORDING                                 0x1

/*!
 * This macro normalizes the number of channels requested
 * by the user, if needed.
 */
#define NORMALIZE_NUM_CHANNEL(c)                        \
do{                                                     \
        if ((c) > 2){                                   \
                (c) = 2;                                \
        }                                               \
                                                        \
        if ((c) < 1){                                   \
                (c) = 1;                                \
        }                                               \
}while (0)

/*!
 * This macro normalizes the value of balance requested
 * by the user, if needed.
 */
#define NORMALIZE_BALANCE(v)                            \
do{                                                     \
        if ((v) < BALANCE_MIN){                         \
                 (v) = BALANCE_MIN;                     \
        }                                               \
                                                        \
        if ((v) > BALANCE_MAX ){                        \
                (v) = BALANCE_MAX;                      \
        }                                               \
}while (0)

/*!
 * This macro normalizes the value of volume for input
 * devices requested by the user, if needed.
 */
#define NORMALIZE_INPUT_VOLUME(v)                       \
do{                                                     \
        if ((v) > INPUT_VOLUME_MAX){                    \
                (v) = INPUT_VOLUME_MAX;                 \
        }else if ((v) < INPUT_VOLUME_MIN){              \
                (v) = INPUT_VOLUME_MIN;                 \
        }                                               \
}while (0)

/*!
 * This macro normalizes the value of volume for output
 * devices requested by the user, if needed.
 */
#define NORMALIZE_OUTPUT_VOLUME(v)                      \
do{                                                     \
        if ((v) > OUTPUT_VOLUME_MAX){                   \
                (v) = OUTPUT_VOLUME_MAX;                \
        }else if ((v) < OUTPUT_VOLUME_MIN){             \
                (v) = OUTPUT_VOLUME_MIN;                \
        }                                               \
}while (0)

/*!
 * This macro normalizes the sample rate requested by
 * the user, if needed.
 */
#define NORMALIZE_SPEED(s)                              \
do{                                                     \
        if ((s) > SAMPLE_FREQ_MAX){                     \
                (s) = SAMPLE_FREQ_MAX;                  \
        }                                               \
                                                        \
        if ((s) < SAMPLE_FREQ_MIN){                     \
                (s) = SAMPLE_FREQ_MIN;                  \
        }                                               \
}while (0)

/*!
 * These macros define the available modes for mc13783 IC.
 */
#define MC13783_MASTER                                    0x1
#define MC13783_SLAVE                                     0x2

/*!
 * These macros define the audiomux ports MCU uses to communicate.
 * with mc13783 IC
 */
#define DAM_PORT_4                                      port_4
#define DAM_PORT_5                                      port_5

/*!
 * These macros define the available audio devices in mc13783 IC.
 */
#define MC13783_STEREODAC                                 0x1
#define MC13783_CODEC                                     0x2

/*!
 * These macros define the watermark level when transferring audio
 * samples. These values configure SSI FIFO's watermark level.
 * One watermark is for recording, one for playback.
 */
#define TX_WATERMARK                                    0x4
#define RX_WATERMARK                                    0x6

/*!
 * These macros define the watermark level when transferring audio
 * samples. These values configure SDMA channel watermark level.
 * One watermark is for recording, one for playback.
 */
#define SDMA_TXFIFO_WATERMARK                           0x4
#define SDMA_RXFIFO_WATERMARK                           0x6

/*!
 * These macros define the number of timeslots per frame.
 * Audio driver uses 2 timelots for mono audio, 4 for stereo.
 */
#define TIMESLOTS_2                                     0x3
#define TIMESLOTS_4                                     0x2

/*!
 * This macro defines the number of sample rates supported.
 */
#define SAMPLE_RATE_MAX                                 0x9

/*!
 * These macros define the pll that mc13783 will use for audio operations.
 * selection of the pll depends on mode and on frequency provided
 * by the master clock in CLIA/CLIB signals.
 */
#define CLK_SELECT_SLAVE_BCL                            0x7
#define CLK_SELECT_SLAVE_CLI                            0x5
#define CLK_SELECT_MASTER_CLI                           0x4

/*!
 * This macro defines the value in mc13783 registers
 * to set the minimum balance supported (0dB)
 */
#define MC13783_BALANCE_MIN                               0x0

/*!
 * This macro defines the value in mc13783 registers
 * to set the maximum balance supported (-21dB)
 */
#define MC13783_BALANCE_MAX                               0x7

/*!
 * This macro defines the value in OSS framework to set
 * the minimum balance supported (-21dB left)
 */
#define BALANCE_MIN                                     0x0

/*!
 * This macro defines the value in OSS framework to set
 * no balance at all.
 */
#define NO_BALANCE                                      0x32

/*!
 * This macro defines the value in OSS framework to set
 * the maximum balance supported (-21dB right)
 */
#define BALANCE_MAX                                     0x64

/*!
 * This macro defines the value in mc13783 registers to set
 * the minimum volume (mute) supported (-8dB) for input devices.
 */
#define MC13783_INPUT_VOLUME_MIN                          0x0

/*!
 * This macro defines the value in mc13783 registers to set
 * the maximum volume supported (+23dB) for input devices.
 */
#define MC13783_INPUT_VOLUME_MAX                          0x1f

/*!
 * This macro defines the value in mc13783 registers to set
 * the minimum volume (mute) supported (-33dB) for output devices.
 */
#define MC13783_OUTPUT_VOLUME_MIN                         MC13783_INPUT_VOLUME_MIN

/*!
 * This macro defines the value in mc13783 registers to set
 * the maximum volume supported (+6dB) for output devices.
 */
#define MC13783_OUTPUT_VOLUME_MAX                         0xf

/*!
 * This macro defines the value in OSS framework to set
 * the minimum volume supported (-8dB) for input devices.
 */
#define INPUT_VOLUME_MIN                                0x0

/*!
 * This macro defines the value in OSS framework to set
 * the maxium volume supported (+23dB) for input devices.
 */
#define INPUT_VOLUME_MAX                                0x64

/*!
 * This macro defines the value in OSS framework to set
 * the minimum volume supported (-39dB) for output devices.
 */
#define OUTPUT_VOLUME_MIN                               0x0

/*!
 * This macro defines the value in OSS framework to set
 * the maxium volume supported (+6dB) for output devices.
 */
#define OUTPUT_VOLUME_MAX                               0x64

/*!
 * This macro defines the maximum sample rate supported
 * by the mc13783 IC.
 */
#define SAMPLE_FREQ_MAX                                 96000

/*!
 * This macro defines the minimum sample rate supported
 * by the mc13783 IC.
 */
#define SAMPLE_FREQ_MIN                                 8000

/*!
 * Define channels available on mc13783. This is mainly used
 * in mixer interface to control different input/output
 * devices.
 */
/* headset, lineout */
#define MXC_STEREO_OUTPUT                               \
( SOUND_MASK_VOLUME | SOUND_MASK_PCM )

/* handset */
#define MXC_STEREO_INPUT                                \
( SOUND_MASK_PHONEIN )

/* earpiece, handsfree */
#define MXC_MONO_OUTPUT                                 \
( SOUND_MASK_PHONEOUT | SOUND_MASK_SPEAKER )

/* linein, mic */
#define MXC_MONO_INPUT                                  \
( SOUND_MASK_LINE | SOUND_MASK_MIC )

#define MXC_INPUTS                                      \
( MXC_STEREO_INPUT | MXC_MONO_INPUT )
#define MXC_OUTPUTS                                     \
( MXC_STEREO_OUTPUT | MXC_MONO_OUTPUT )

#ifdef CONFIG_ARCH_MX3

/*!
 * These macros define the divider used to get correct
 * SSI main clock frequency. This is used only when MCU
 * is the clock provider (mc13783 slave mode) Normally clocks
 * are provided by mc13783.
 */
/* TODO: Calculate values for iMX.31 platform */
#define CCM_DIVIDER_8KHZ                                48 /**/
#define CCM_DIVIDER_11KHZ                               8 /**/
#define CCM_DIVIDER_16KHZ                               24 /**/
#define CCM_DIVIDER_22KHZ                               4 /**/
#define CCM_DIVIDER_24KHZ                               6
#define CCM_DIVIDER_32KHZ                               12 /**/
#define CCM_DIVIDER_44KHZ                               2 /**/
#define CCM_DIVIDER_48KHZ                               3
#define CCM_DIVIDER_96KHZ                               4
#define PRESCALER_8KHZ                                  39 /**/
#define PRESCALER_11KHZ                                 170 /**/
#define PRESCALER_16KHZ                                 39 /**/
#define PRESCALER_22KHZ                                 170 /**/
#define PRESCALER_24KHZ                                 125
#define PRESCALER_32KHZ                                 39 /**/
#define PRESCALER_44KHZ                                 170 /**/
#define PRESCALER_48KHZ                                 125
#define PRESCALER_96KHZ                                 47
#endif				/* CONFIG_ARCH_MX3 */
#ifdef CONFIG_ARCH_MXC91231
/*!
 * These macros define the divider used to get correct
 * SSI main clock frequency. This is used only when MCU
 * is the clock provider (mc13783 slave mode) Normally clocks
 * are provided by mc13783.
 */
#define CCM_DIVIDER_8KHZ                                4
#define CCM_DIVIDER_11KHZ                               8
#define CCM_DIVIDER_16KHZ                               4
#define CCM_DIVIDER_22KHZ                               4
#define CCM_DIVIDER_24KHZ                               2
#define CCM_DIVIDER_32KHZ                               2
#define CCM_DIVIDER_44KHZ                               2
#define CCM_DIVIDER_48KHZ                               2
#define CCM_DIVIDER_96KHZ                               2
#define PRESCALER_8KHZ                                  188
#define PRESCALER_11KHZ                                 68
#define PRESCALER_16KHZ                                 94
#define PRESCALER_22KHZ                                 68
#define PRESCALER_24KHZ                                 125
#define PRESCALER_32KHZ                                 94
#define PRESCALER_44KHZ                                 68
#define PRESCALER_48KHZ                                 62
#define PRESCALER_96KHZ                                 31
#endif				/* CONFIG_ARCH_MXC91231 */
#if defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321)
/*!
 * These macros define the divider used to get correct
 * SSI main clock frequency. This is used only when MCU
 * is the clock provider (mc13783 slave mode) Normally clocks
 * are provided by mc13783.
 */
#define CCM_DIVIDER_8KHZ                                16
#define CCM_DIVIDER_11KHZ                               8
#define CCM_DIVIDER_16KHZ                               16
#define CCM_DIVIDER_22KHZ                               8
#define CCM_DIVIDER_24KHZ                               6
#define CCM_DIVIDER_32KHZ                               16
#define CCM_DIVIDER_44KHZ                               8
#define CCM_DIVIDER_48KHZ                               3
#define CCM_DIVIDER_96KHZ                               4
#define PRESCALER_8KHZ                                  140
#define PRESCALER_11KHZ                                 204
#define PRESCALER_16KHZ                                 70
#define PRESCALER_22KHZ                                 102
#define PRESCALER_24KHZ                                 125
#define PRESCALER_32KHZ                                 35
#define PRESCALER_44KHZ                                 51
#define PRESCALER_48KHZ                                 125
#define PRESCALER_96KHZ                                 47
#endif				/* CONFIG_ARCH_MXC91331 || CONFIG_ARCH_MXC91321 */
#endif				/* __KERNEL__ */
#endif				/* __MXC_AUDIO_MC13783_H__ */
