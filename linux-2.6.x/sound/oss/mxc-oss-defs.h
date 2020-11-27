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
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * History :
 *      04/05/18: Removed the dmabuf structure.
 *      04/08/12: Added dam destination port as a configuration param
 *      04/11/19: TLSbo44269: mixer rework
 *      04/11/26: TLSbo45095: Add word_size in the SDMA configuration
 *      05/03/03: TLSbo48139: HW configuration bring up
 */

/*!
 * @file mxc-oss-defs.h
 * @brief Common definitions for the MXC OSS sound driver.
 *
 * @ingroup SOUND_OSS_MXC_PMIC
 */

#ifndef _MXC_OSS_DEFS_H
#define _MXC_OSS_DEFS_H

#include <asm/atomic.h>
#include <linux/soundcard.h>
#include <asm/arch/pmic_audio.h>
#include <ssi.h>
#include <dam.h>

#include "mxc-oss-ctrls.h"

/*! This macro checks that the returned value by specified operation is
    zero. If not, then goto _err */
#define TRY(a) \
        do { \
                if ( (a) != 0 ) goto _err; \
        } while (0);

/*! This macro checks that the returned value by specified operation is 
    the one specified as an arg. If not, then goto _err */
#define TRYEQ(a, b) \
        do { \
                if ( (a) != (b) ) goto _err; \
        } while (0);

/*! This macro checks that the returned value by specified operation is 
    not the one specified as an arg. If not, then goto _err */
#define TRYNEQ(a, b) \
        do { \
                if ( (a) == (b) ) goto _err; \
        } while (0);

/*! This macro checks that the returned value by specified operation is 
    accepted by the hardware (returned value is zero).
    If yes, then break, else, continue */
#define TRYOK(a) \
        do { \
                if ( (a) == 0 ) goto _next; \
        } while (0);

/*! This macro is associated with the TRY macro. */
#define CATCH _err

/*
 * The hardware has 2 SSI buses in full-duplex mode to carry audio
 * Capabilities are limited to 2 play + 1 rec
 */
/*! Number of SSIs connected to MCU */
#define NR_HW_CH                        2
/*! Number of possible record at a time */
#define NR_HW_CH_RX                     1
/*! Number of possible play at a time */
#define NR_HW_CH_TX                     2

/*! Use the DAM port_4 as the default output port */
#define DAM_DEFAULT_OUT_PORT            port_4

/*! Use the SSI fifo #0 as the default fifo */
#define SSI_DEFAULT_FIFO_NB             ssi_fifo_0

/*! sound devices defines for converters (can/cna) */
#define PMIC_DEVICE_OUT_NONE           0x0
#define PMIC_DEVICE_IN_NONE            0x0
#define PMIC_DEVICE_OUT_STDAC          0x1
#define PMIC_DEVICE_OUT_CODEC          0x2
#define PMIC_DEVICE_OUT_LINE_IN        0x4
#define PMIC_DEVICE_IN_CODEC           0x8

/*! Defines for "connected_device" */
#define PMIC_PHYSICAL_DEVICE_NONE      0x0
#define PMIC_PHYSICAL_DEVICE_STDAC     0x1
#define PMIC_PHYSICAL_DEVICE_CODEC     0x2

#define MXC_STEREO_IN_SRC_MASK   (SOUND_MASK_PHONEIN)
#define MXC_MONO_IN_SRC_MASK     (SOUND_MASK_LINE | SOUND_MASK_MIC)
#define MXC_RECORDING_SRC_MASK   \
        (MXC_STEREO_IN_SRC_MASK | MXC_MONO_IN_SRC_MASK)
#define MXC_STEREO_OUT_SRC_MASK  (SOUND_MASK_VOLUME | SOUND_MASK_PCM)
#define MXC_MONO_OUT_SRC_MASK    (SOUND_MASK_PHONEOUT | SOUND_MASK_SPEAKER)
#define MXC_PLAYING_SRC_MASK     \
        (MXC_STEREO_OUT_SRC_MASK | MXC_MONO_OUT_SRC_MASK)

/*! the default master for audio clocking (MCU or SSI) */
#define CLK_DEFAULT_MASTER              MXC_AUDIO_CLOCKING_PMIC_MASTER

/*! sound devices defines for Sources (lines) */
#define PMIC_SRC_OUT_NONE              0
#define PMIC_SRC_IN_NONE               0
#define PMIC_SRC_OUT_EARPIECE          SOUND_MASK_PHONEOUT
#define PMIC_SRC_OUT_HANDSFREE         SOUND_MASK_SPEAKER
#define PMIC_SRC_OUT_HEADSET           SOUND_MASK_VOLUME
#define PMIC_SRC_OUT_LINE_OUT          SOUND_MASK_PCM
#define PMIC_SRC_IN_HANDSET            SOUND_MASK_PHONEIN
#define PMIC_SRC_IN_LINE_IN            SOUND_MASK_LINE
#define PMIC_SRC_IN_HEADSET            SOUND_MASK_MIC

/*! the default sources at driver opening */
#ifdef DRV_FOR_VIRTIO_MODEL
#define PMIC_DEFAULT_SRC_IN            PMIC_SRC_IN_LINE_IN
#define PMIC_DEFAULT_SRC_OUT           PMIC_SRC_OUT_LINE_OUT
#else
#define PMIC_DEFAULT_SRC_IN            PMIC_SRC_IN_HEADSET
#define PMIC_DEFAULT_SRC_OUT           PMIC_SRC_OUT_HEADSET
#endif

/*
 * sc55112 PMIC-specific Definitions
 */

#ifdef CONFIG_MXC_PMIC_SC55112

/*! Recording volume range in sc55112. */
#define PMIC_IN_VOLUME_MIN                    0	/*   0dB */
#define PMIC_IN_VOLUME_MAX                   31	/* +31dB */
/*! Playback volume range in sc55112. */
#define PMIC_OUT_VOLUME_MIN                 -24	/* -24dB */
#define PMIC_OUT_VOLUME_MAX                  21	/* +21dB */

/*! Balance control range in sc55112 (not supported by hardware). */
#define PMIC_OUT_BALANCE_MIN                  0	/* 0dB   */
#define PMIC_OUT_BALANCE_MAX                  0	/* 0dB   */

/* Volume to balance conversion. */
#define PMIC_VOL_TO_BALANCE_RATIO          ((21+24)/(21+21))

#endif				/* CONFIG_MXC_PMIC_SC55112 */

/*! Recording volume range in OSS. */
#define OSS_IN_VOLUME_MIN                     0	/*    0% */
#define OSS_IN_VOLUME_MAX                   100	/*  100% */
/*! Playback volume range in OSS. */
#define OSS_OUT_VOLUME_MIN                    0	/*    0% */
#define OSS_OUT_VOLUME_MAX                  100	/*  100% */

/*! Balance control range in OSS. */
#define OSS_OUT_BALANCE_MIN                   0	/* -21dB left channel      */
#define OSS_OUT_BALANCE_MAX                 100	/* -21dB right channel     */
/*! Middle balance value in OSS. */
#define OSS_OUT_NO_BALANCE                   50	/* 0dB left+right channels */

/*! 0 <= volume <= 100, left channel and right channel volume
 *
 *  output: PMIC_OUT_VOLUME_MIN <= x <= PMIC_OUT_VOLUME_MAX
 *
 *  Note that the PMIC output PGA gain levels are in steps of 3 dB.
 *  Therefore, when converting to a hardware gain level, we finish
 *  with a divide by 3 followed by a multiply by 3 so that we always
 *  end up with a value that is an integer multiple of 3.
 */
#define CONVERT_OUT_VOLUME_OSS_TO_HW(volume) \
        ( ((((volume) * (PMIC_OUT_VOLUME_MAX - PMIC_OUT_VOLUME_MIN) / \
             (OSS_OUT_VOLUME_MAX - OSS_OUT_VOLUME_MIN)) + \
            PMIC_OUT_VOLUME_MIN) / 3) * 3 )
#define CONVERT_OUT_VOLUME_HW_TO_OSS(volume)    \
        ( ((volume) - PMIC_OUT_VOLUME_MIN) * \
          ((OSS_OUT_VOLUME_MAX - OSS_OUT_VOLUME_MIN) / \
          (PMIC_OUT_VOLUME_MAX - PMIC_OUT_VOLUME_MIN)) )
#define CONVERT_IN_VOLUME_OSS_TO_HW(volume) \
        ( ((volume) * (PMIC_IN_VOLUME_MAX - PMIC_IN_VOLUME_MIN) / \
           (OSS_IN_VOLUME_MAX - OSS_IN_VOLUME_MIN)) +          \
        PMIC_IN_VOLUME_MIN )
#define CONVERT_IN_VOLUME_HW_TO_OSS(volume)    \
        ( ((volume) - PMIC_IN_VOLUME_MIN) * \
          ((OSS_IN_VOLUME_MAX - OSS_IN_VOLUME_MIN) /  \
           (PMIC_IN_VOLUME_MAX - PMIC_IN_VOLUME_MIN)) )

/*! 0 <= balance <= 100,  0-50  left channel balance,
 *                       50-100 right channel balance
 *
 *  output: PMIC_OUT_BAL_MIN <= x <= PMIC_OUT_BAL_MAX
 */
#define CONVERT_BALANCE_OSS_TO_HW(balance) \
        (((balance) * (PMIC_OUT_BALANCE_MAX - PMIC_OUT_BALANCE_MIN) / 50) + \
        PMIC_OUT_BALANCE_MIN)
#define CONVERT_BALANCE_HW_TO_OSS(balance) \
        (((balance) - PMIC_OUT_BALANCE_MIN) * (50 / (PMIC_OUT_BALANCE_MAX + \
        PMIC_OUT_BALANCE_MIN)))

/*
 * MXC91331 specific definitions
 */

#ifdef CONFIG_ARCH_MXC91331

/* Prescaler modulus base definition for A+ */
#define MXC_SYSTEM_CLK                   49887500

#define _reg_MCU_MCR   (*((volatile unsigned long *) \
                              (IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x00))))
#define _reg_MCU_MPDR1      (*((volatile unsigned long *) \
                              (IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x08))))

#endif /* CONFIG_ARCH_MXC91331 */

/*
 * mxc91231 Platform-specific Definitions
 */

#ifdef CONFIG_ARCH_MXC91231

/* Prescaler modulus base definition for mxc91231 */
#define MXC_SYSTEM_CLK                   24000000

#define _reg_CRM_AP_ASCSR   (*((volatile unsigned long *) \
                              (IO_ADDRESS(CRM_AP_BASE_ADDR + 0x00))))
#define _reg_CRM_AP_ACDER1      (*((volatile unsigned long *) \
                              (IO_ADDRESS(CRM_AP_BASE_ADDR + 0x08))))

#endif /* CONFIG_ARCH_MXC91231 */

/*
 * mxc91131 Platform-specific Definitions
 */

#ifdef CONFIG_ARCH_MXC91131

/* Prescaler modulus base definition for mxc91131 */
#define MXC_SYSTEM_CLK                   24000000

#define _reg_CRM_AP_ASCSR   (*((volatile unsigned long *) \
                              (IO_ADDRESS(CRM_AP_BASE_ADDR + 0x00))))
#define _reg_CRM_AP_ACDER1      (*((volatile unsigned long *) \
                              (IO_ADDRESS(CRM_AP_BASE_ADDR + 0x08))))
#endif /* CONFIG_ARCH_MXC91131 */

/*
 * MX3 Platform-specific Definitions
 */

#ifdef CONFIG_ARCH_MX3

/* Prescaler modulus base definition for MX3 */
#define MXC_SYSTEM_CLK                   49887500

#endif				/* CONFIG_ARCH_MX3 */

/* Prescaler modulus for mxc91231 (DIV2=0, PSR=0) */
#define MXC_PM_BASE_MONO_16BITS         (MXC_SYSTEM_CLK/16/4)

/* Prescaler modulus definitions for Mono/16 bits */
#define MXC_PM_8000                     (MXC_PM_BASE_MONO_16BITS/8000)
#define MXC_PM_11025                    (MXC_PM_BASE_MONO_16BITS/11025)
#define MXC_PM_12000                    (MXC_PM_BASE_MONO_16BITS/12000)
#define MXC_PM_16000                    (MXC_PM_BASE_MONO_16BITS/16000)
#define MXC_PM_22050                    (MXC_PM_BASE_MONO_16BITS/22050)
#define MXC_PM_24000                    (MXC_PM_BASE_MONO_16BITS/24000)
#define MXC_PM_32000                    (MXC_PM_BASE_MONO_16BITS/32000)
#define MXC_PM_44100                    (MXC_PM_BASE_MONO_16BITS/44100)
#define MXC_PM_48000                    (MXC_PM_BASE_MONO_16BITS/48000)
#define MXC_PM_64000                    (MXC_PM_BASE_MONO_16BITS/64000)
#define MXC_PM_96000                    (MXC_PM_BASE_MONO_16BITS/96000)

/* SSI */
#define MXC_SSI_INT_DISABLE_ALL        0xFFFFFFFF

/*! FIFO size for SSI */
#define SSI_FIFO_SIZE                   8
/*! Tx FIFO Empty Watermark level */
#define TX_FIFO_EMPTY_WML               4
/*! Rx FIFO Full Watermark level  */
#define RX_FIFO_FULL_WML                6

/* Definitions for other platform configuration. */
#define ModifyRegister(a,b,c)   ((c) = ( ( (c)&(~(a)) ) | (b) ))

#define _reg_SSI2_SPBA      (*((volatile unsigned long *) \
                              (IO_ADDRESS(SPBA_CTRL_BASE_ADDR + SPBA_SSI2))))
/*! 
 * @brief Holds the information about the PMIC audio hardware setup.
 *
 */
struct _pmic_state {
	/*! Device handle to the PMIC. */
	PMIC_AUDIO_HANDLE handle;

	/*! The attached audio device (Voice CODEC/Stereo DAC). */
	PMIC_AUDIO_SOURCE device;

	/*! Selected audio output port (headset, handset, etc.). */
	PMIC_AUDIO_OUTPUT_PORT out_line;

	/*! Selected audio input port (mono microphone, etc.). */
	PMIC_AUDIO_INPUT_PORT in_line;

	/*! Select microphone input bias circuit. */
	PMIC_AUDIO_MIC_BIAS micBias;

	/* Sound device output/playback parameters. */
	PMIC_AUDIO_OUTPUT_PGA_GAIN out_volume;	/*!< output volume. L+R coded */
	int out_balance;	/*!< output balance           */
	PMIC_AUDIO_MONO_ADDER_MODE out_adder_mode;	/*!< mono adder cfg  */

	/*! Sound device input/recording parameters (if applicable). */
	int in_volume;		/*!< input volume. L+R coded  */
	int in_bias;		/*!< input bias               */

	/*! PMIC config for both input and output configuration. */
	PMIC_AUDIO_VCODEC_CONFIG in_out_filter;	/*!< codec filtering          */
	int in_out_sr_Hz;	/*!< sample rate (Hz)         */
};

struct _mxc_card;

/*!
 * @brief Holds all the information used by a sound driver instance.
 *
 */
struct _mxc_state {
	/*! Card information. */
	struct _mxc_card *card;

	/*! PMIC hardware state information. */
	struct _pmic_state pmic;

	/*! Playback and recording counters. */
	int play_cnt, rec_cnt;

	/*! Audio device availability - true when playing/recording. */
	int device_busy;

	/*! The SSI bus to be used (SSI1 or SSI2). */
	ssi_mod ssi_index;

	/*! SSI availability - Set when the instance is used */
	int ssi_allocated;

	/*! SSI FIFO number to be used (0 or 1). */
	fifo_nb ssi_fifo_nb;

	/*! The SSI Tx timeslot mask setting. */
	int ssi_tx_timeslots;

	/*! The SSI Rx timeslot mask setting. */
	int ssi_rx_timeslots;

	/*! Internal Digital Audio MUX port to be used. */
	dam_port dam_src;

	/*! External Digital Audio MUX port to be used. */
	dam_port dam_dest;

	/*! The number of audio channels used (1 for Mono, 2 for Stereo). */
	int in_out_channel_num;

	/*! The audio stream bit depth (8 or 16 bits/sample). */
	int in_out_bits_per_sample;

	/*! Audio playback DMA buffer pointer. */
	int dma_playbuf;

	/*! Audio recording DMA buffer pointer. */
	int dma_recbuf;

	/*! Audio recording/playback triggering control. */
	int judge[2];
#define J_PLAY 0
#define J_REC 1

	/*! File opening mode (READ for recording, WRITE for playback). */
	int open_mode;

	/*! Audio data bus clock master/slave selection. */
	int clock_master;

	/*! /dev/sound device name entry index. */
	int dev_index;

	/*! Indicates whether the can is used or not */
	int using_can;

	/*! Indicates whether the cna is used or not */
	int using_cna;

	/*! Single open lock mechanism, only used for recording. */
	wait_queue_head_t open_wait;

	/*! Used to prevent inter-instance race conditions where an interrupt
	 *  context may be involved.
	 */
	spinlock_t drv_inst_lock;

	/*! Mutex for making consistent state changes to this data structure
	 *  and the audio hardware.
	 */
	struct semaphore mutex;
};

/*!
 * @brief Holds all the information about a sound card.
 *
 */
struct _mxc_card {
	/*! Instance data for each /dev/dspX device. */
	struct _mxc_state *state[NR_HW_CH];

	/*! Index for accessing the /dev/mixer device. */
	int dev_mixer;
};

typedef struct _mxc_card mxc_card_t;
typedef struct _mxc_state mxc_state_t;
typedef struct _pmic_state pmic_state_t;

/*!
 * @brief Gives the direction of an audio channel.
 *
*/
typedef enum {
	eDIR_TX_PLAY = 0,	/*!< 0=out */
	eDIR_RX_REC,		/*!< 1= in */
	eDIR_BOTH		/*!< full-duplex */
} e_transfer_dir_t;

#endif				/* _MXC_OSS_DEFS_H */
