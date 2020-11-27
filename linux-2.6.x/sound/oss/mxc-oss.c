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
 */

/*!
 * @file mxc-oss.c
 * @brief Implementation of the MXC sound driver for the OSS framework.
 *
 * This source file implements the OSS audio device driver interface
 * for use with the MXC platforms and the PMIC audio hardware components.
 * This implementation uses the unified PMIC APIs for Linux that is
 * intended to support both the mc13783 and sc55112 PMICs. However,
 * currently the unified PMIC APIs have only been implemented and tested
 * with the sc55112 PMIC.
 *
 * More specifically, this driver has only been tested so far using the
 * sc55112 PMIC as implemented for the iDEN Sphinx board. The iDEN
 * Sphinx board defines the actual external audio connections (e.g.,
 * audio jacks) for the sc55112 PMIC.
 *
 * This driver has been successfully tested using the following scenarios:
 *
 * - MP3 and WAV audio file playback using the Stereo DAC and the Qtopia
 *   MediaPlayer application
 * - raw PCM audio file playback and recording using the Voice CODEC and
 *   the "cat" command (e.g., you can use "cat < /dev/sound/dsp > rec.pcm"
 *   to record from the Voice CODEC using it's default configuration and,
 *   similarly, the "cat rec.pcm > /dev/sound/dsp" command will playback
 *   an audio file using the Voice CODEC's default configuration)
 *
 * The OSS-side of the device driver interface handles the transfer of
 * audio streams to/from OSS audio applications. The MXC and PMIC-side
 * of the device driver handles the transfer of audio streams to/from
 * the PMIC Stereo DAC and Voice CODEC using the SSI, SDMA, and Audio
 * MUX components provided with the MXC platforms.
 *
 * @see mxc-oss-hw.c
 *
 * Note that while the OSS framework will automatically create the proper
 * device name entries within the /dev/sound directory, you may still need
 * to create additional device links in the /dev directory to enable
 * applications to run properly. Typically, you will need at least the
 * following links for most typical OSS audio applications (e.g., the
 * QTopia MediaPlayer):
 *
 * @verbatim
   /dev/dsp   -> /dev/sound/dsp   (for access to the PMIC Voice CODEC)
   /dev/dsp   -> /dev/sound/dsp1  (for access to the PMIC Stereo DAC)
   /dev/mixer -> /dev/sound/mixer (for control of the PMIC audio
                                   input/output sections)
   @endverbatim
 *
 * The /dev/dsp link should point to whatever audio device is to be used
 * by default. QTopia expects to use /dev/dsp and /dev/mixer by default
 * so both of these entries must point to valid audio devices. Audio
 * applications may, however, directly open either /dev/sound/dsp or
 * /dev/sound/dsp1 if they require specific playback or recording
 * capabilities.
 *
 * Please refer to the appropriate Detailed Technical Specifications (DTS)
 * document for complete details and specifications for each PMIC.
 *
 * The Open Sound System Programmer's Guide is a good reference to use
 * for information about the OSS audio/dsp/mixer devices and what device
 * driver support is required.
 *
 * This file was based originally on the mxc-audio.c file that was used
 * to implement the OSS audio driver for the mc13783 PMIC.
 *
 * However, the original mxc-audio.c file has been extensively changed
 * to produce this implementation. A brief summary of the changes include:
 *
 * - use of the new PMIC APIs to provide a generic PMIC interface
 * - changed /dev/sound/dsp to always connect with the PMIC Voice CODEC
 * - changed /dev/sound/dsp1 to always connect with the PMIC Stereo DAC
 * - various code cleanups and removed functions that were not being used
 *
 * Note that only the SSI SDMA mode (selected using the CONFIG_SOUND_MXC_SDMA
 * kernel configuration variable) has been tested so far. The alternative FIFO
 * mode (selected using the CONFIG_SOUND_MXC_FIFO kernel configuration variable)
 * has not yet been tested. However, the SDMA mode is the preferred mode of
 * operation.
 *
 * The following items still need to be completed and tested:
 *
 * - the audio mixer IOCTL interface to reconfigure the audio input/output
 *   sections
 * - headset insertion/removal mono/stereo detection
 * - headset Push-to-Talk (PTT) button handling
 * - the use of the PMIC mixer
 * - simultaneous (i.e., full duplex) playback and recording using
 *   the Voice CODEC
 * - investigate ways to improve the audio quality by reducing the
 *   background noise
 * - development of a low-level mc13783 PMIC driver using the unified
 *   PMIC APIs and integration with this driver so that we end up
 *   needing only a single OSS sound driver regardless of the PMIC
 *   that is used
 *
 * @ingroup SOUND_OSS_MXC_PMIC
 */

#include <linux/module.h>	/* Linux kernel module definitions        */

#include "sound_config.h"	/* OSS sound framework macro definitions  */
#include "dev_table.h"		/* OSS sound framework API definitions    */

#include "mxc-oss-hw.h"		/* PMIC for OSS hardware interface APIs   */
#include "mxc-oss-ctrls.h"	/* PMIC for OSS driver IOCTL definitions  */
#include "mxc-oss-dbg.h"	/* PMIC for OSS driver debug definitions  */

#include <asm/arch/spba.h>	/* SPBA configuration for SSI2            */

#ifdef DEBUG
#include <pmic_external.h>	/* For access to PMIC low-level API */
#endif				/* DEBUG */

/*
 *************************************************************************
 * GLOBAL STATIC DEFINITIONS
 *************************************************************************
 */

/*!
 *  Define the internal Digital Audio MUX port for SSI1.
 */
static const dam_port SSI1_AUDMUX_PORT = port_1;	/* Fixed by hardware. */

/*!
 *  Define the internal Digital Audio MUX port for SSI2.
 */
static const dam_port SSI2_AUDMUX_PORT = port_2;	/* Fixed by hardware. */

/*!
 *  Define the external Digital Audio MUX port for the Voice CODEC.
 *
 *  Note that even though it is also possible to configure the Voice CODEC
 *  and the Stereo DAC to use either Audio MUX Port4 or Audio MUX Port5
 *  (depending upon which one is currently unused), it does help to
 *  simplify the code a lot if we just decide to only use Audio MUX Port4
 *  for the Voice CODEC and Audio MUX Port5 for the Stereo DAC.
 *
 *  Furthermore, the current sc55112 hardware implementation only
 *  supports audio recording through Audio MUX Port 4 (since the RX line
 *  for Audio MUX Port5 is tied permanently to ground). Therefore, we can
 *  only use Audio MUX Port4 to connect with the Voice CODEC if we want to
 *  do any recording operations.
 */
static const dam_port VCODEC_AUDMUX_PORT = port_4;

/*!
 *  Define the external Digital Audio MUX port for the Stereo DAC.
 *
 *  The current Virtio model for the Audio MUX only connects Port 4 to
 *  the PMIC. However, on the actual hardware, Port 4 is connected to
 *  the PMIC's Digital Audio Bus 0 and Port 5 is connected to the PMIC's
 *  Digital Audio Bus 1.
 *
 *  When using the Virtio model, we can only connect one SSI to either
 *  the Voice CODEC or the Stereo DAC using Audio MUX Port 4 at a time.
 *  However, when using the actual hardware, we can simultaneously use
 *  both SSIs along with the Voice CODEC and the Stereo DAC. In this
 *  case we have assigned Audio MUX Port 4 to connect with the Voice
 *  CODEC and Audio MUX Port 5 to connect with the Stereo DAC.
 */
static const dam_port STDAC_AUDMUX_PORT =
#ifdef DRV_FOR_VIRTIO_MODEL
    port_4			/* Fixed by Virtio.   */
#else				/* DRV_FOR_VIRTIO_MODEL */
    port_5
#endif				/* DRV_FOR_VIRTIO_MODEL */
    ;

/*! Define the supported audio formats for SDMA and FIFO operating modes.
 *
 *  Note that we only support the 16-bit mode because both the PMIC Voice
 *  CODEC and Stereo DAC require a 16-bit linear PCM audio data stream.
 */
static const unsigned int AUDIO_FORMATS = AFMT_S16_LE;

/*! Define the SPBA master controllers for SDMA and FIFO operating modes. */
static const unsigned int SPBA_MASTER_SELECT =
#ifdef CONFIG_SOUND_MXC_SDMA
    /*
     * SPBA configuration for SSI2 - SDMA and MCU are set.
     */
    SPBA_MASTER_C | SPBA_MASTER_A
#else				/* CONFIG_SOUND_MXC_FIFO */
    /*
     * SPBA configuration for SSI2 - MCU only is set.
     */
    SPBA_MASTER_A
#endif
    ;

/*! SSI timeslot mask to disable all timeslots in network mode.          */
static const unsigned SSI_TIMESLOT_MASK_ALL = ~(0x0);

/*! SSI timeslot mask to enable the first timeslot in network mode.      */
static const unsigned SSI_TIMESLOT_MASK_MONO = ~(0x1);

/*! SSI timeslot mask to enable the first two timeslots in network mode. */
static const unsigned SSI_TIMESLOT_MASK_STEREO = ~(0x3);

/*! Define the default output port(s) for the PMIC Stereo DAC. */
static const PMIC_AUDIO_OUTPUT_PORT STEREO_DAC_DEFAULT_OUTPUT =
    STEREO_HEADSET_LEFT | STEREO_HEADSET_RIGHT;

/*! Define the default output port(s) for the PMIC Voice CODEC. */
static const PMIC_AUDIO_OUTPUT_PORT VOICE_CODEC_DEFAULT_OUTPUT =
    STEREO_HEADSET_RIGHT;

/*! Define the number of audio channels to be used for Mono mode. */
static const unsigned N_CHANNELS_MONO = 1;

/*! Define the number of audio channels to be used for Stereo mode. */
static const unsigned N_CHANNELS_STEREO = 2;

#ifdef CONFIG_SOUND_MXC_FIFO

/*! Define the FIFO buffer size to be 16 kB. */
#define MXC_FIFO_SIZE   16 * 1024
static char snd_bufplay[2][MXC_FIFO_SIZE];	/*< FIFO mode playback buffer.  */
static char snd_bufrec[MXC_FIFO_SIZE];	/*< FIFO mode recording buffer. */

#endif				/* CONFIG_SOUND_MXC_FIFO */

/*!
 * Global variable that points to the allocated audio card data structure.
 * @see init_mxc_audio()
 *
 * This global variable is also used during clean-up time to free the
 * structure.
 * @see cleanup_mxc_audio()
 */
static mxc_card_t *card_instance = NULL;

/*!
 * The mc13783 PMIC supports both stereo output and stereo recording whereas
 * the sc55112 PMIC only supports stereo output.
 */
static const unsigned MXC_STEREO_CHANNELS_MASK =
#ifdef CONFIG_MXC_PMIC_SC55112
    MXC_STEREO_OUT_SRC_MASK;
#endif				/* CONFIG_MXC_PMIC_SC55112 */

/*
 *************************************************************************
 * LOCAL STATIC FUNCTIONS
 *************************************************************************
 */

/*!
 * This function retrieves the driver instance-specific information.
 *
 * @param    dev [in]
 * @param    drv_inst [out] contains specific information
 * @return   0 if success, -1 if drv_inst is NULL.
 *
 */
static int mxc_get_drv_data(const int dev, mxc_state_t ** drv_inst)
{
	struct audio_operations *adev = audio_devs[dev];
	int ret = 0;

	if (adev) {
		/* Retrieve the device context that was given in the
		 * sound_install_audiodrv() call.
		 */
		*drv_inst = (mxc_state_t *) adev->devc;
		if (!drv_inst) {
			ret = -1;
		}
	} else {
		goto _err;
	}

	return ret;

      _err:
	*drv_inst = NULL;
	FUNC_ERR;
	return -1;
}

/*!
 * This function reads the bit depth (HW dependent)
 * Note: this function does not access the PMIC to get this value,
 * as it is always 16. But this impacts the DMA configuration
 *
 * @param        drv_inst [in] info about the current transfer
 * @return       returns AFMT_S16_LE or AFMT_U8 : the number of bits of
 *               an audio sample
 *
 */
static unsigned int mxc_get_bits(mxc_state_t * const drv_inst)
{
	signed short nBits = 0;

	down_interruptible(&drv_inst->mutex);
	nBits = drv_inst->in_out_bits_per_sample;
	up(&drv_inst->mutex);

	return nBits;
}

/*!
 * This function writes the audio sample word size.
 *
 * Note that the PMIC hardware only operates in 16 bits/sample mode so we
 * always use this as a return value.
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        bits [in] the number of bits of an audio sample
 * @return       AFMT_S16_LE
 *
 */
static unsigned int mxc_set_bits(mxc_state_t * const drv_inst, const int bits)
{
	int nBits = 0;

	DPRINTK("INFO: requesting %d bits/sample\n", bits);

	/*! We only support 16 bits/sample since that is what is required by
	 *  the PMIC Voice CODEC and Stereo DAC.
	 */
	down_interruptible(&drv_inst->mutex);
	nBits = drv_inst->in_out_bits_per_sample;
	up(&drv_inst->mutex);

	DPRINTK("INFO: using %d bits/sample\n", nBits);

	return nBits;
}

/*!
 * This function handles the OSS command that sets the bit depth.
 *
 * @param    dev      [in] the device's major number
 * @param    bits     [in] audio sample bit number.
 *                    If 0, then don't set but just return the current value.
 * @return   the value set in the device
 *           (may be different that the one requested)
 *
 */
static unsigned int mxc_audio_setbits(const int dev, const unsigned int bits)
{
	mxc_state_t *drv_inst = NULL;
	unsigned int ssi_bits = 0;

	/*! Get the drv instance private data */
	if (mxc_get_drv_data(dev, &drv_inst))
		return 0;

	DPRINTK("INFO: bits = %d\n", bits);

	switch (bits) {
	case 0:

		ssi_bits = mxc_get_bits(drv_inst);
		break;

	default:

		ssi_bits = mxc_set_bits(drv_inst, bits);
		break;
	}

	return ssi_bits;
}

/*!
 * This function returns the number of audio channels being used (Mono/Stereo).
 *
 * @param        drv_inst [in] info about the current transfer
 * @return       the number of channels for each audio sample
 *
 */
static signed short mxc_get_channels(mxc_state_t * const drv_inst)
{
	signed short nChannels = 0;

	down_interruptible(&drv_inst->mutex);
	nChannels = drv_inst->in_out_channel_num;
	up(&drv_inst->mutex);

	return nChannels;
}

/*!
 * This function sets the number of audio channels to use (Mono/Stereo).
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        channels [in] the number of channels of the audio stream
 * @return        None
 *
 */
static unsigned short mxc_set_channels(mxc_state_t * const drv_inst,
				       const unsigned short channels)
{
	signed short nChannels = channels;

	FUNC_START;
	DPRINTK("INFO: Configuring %d channels\n", channels);

	down_interruptible(&drv_inst->mutex);

	if ((drv_inst->device_busy)
	    || (channels == drv_inst->in_out_channel_num)) {
		/*! Cannot change the number of channels while audio playback/recording
		 *  is still active. Instead, we will just return the current mode.
		 *
		 *  We also just return the current value if it already matches the
		 *  requested value.
		 */
		nChannels = drv_inst->in_out_channel_num;
	} else {
		/*! Check the parameter validity. We only support Mono (1 channel)
		 *  or Stereo (2 channels).
		 */
		if (nChannels > 2)
			nChannels = 2;
		if (nChannels < 1)
			nChannels = 1;

		/*! No need to change the hardware configuration if we are already
		 *  in the requested mode.
		 */
		if (nChannels != drv_inst->in_out_channel_num) {
			/*! Set the number of channels to be used. */
			if (nChannels == 1) {
				/*! Mono Mode => use only the first timeslot in each frame. */
				drv_inst->ssi_tx_timeslots =
				    SSI_TIMESLOT_MASK_MONO;
				drv_inst->ssi_rx_timeslots =
				    SSI_TIMESLOT_MASK_MONO;
			} else {
				/*! Stereo Mode => use first two timeslots in each frame
				 *  (left audio channel first, followed immediately by the
				 *  right audio channel).
				 */
				drv_inst->ssi_tx_timeslots =
				    SSI_TIMESLOT_MASK_STEREO;

				if (drv_inst->pmic.device == STEREO_DAC) {
					/*! The Stereo DAC has no recording capability, so we
					 *  mask all of the Rx timeslots.
					 */
					drv_inst->ssi_rx_timeslots =
					    SSI_TIMESLOT_MASK_ALL;
				} else {
					/*! Note that the sc55112 Voice CODEC can only record
					 *  in mono mode but the mc13783 Voice CODEC can also record
					 *  in stereo mode so we'll allow for 2 receive timeslots.
					 */
					drv_inst->ssi_rx_timeslots =
					    SSI_TIMESLOT_MASK_STEREO;
				}
			}
			drv_inst->in_out_channel_num = nChannels;
		}
	}

	up(&drv_inst->mutex);

	DPRINTK("INFO: Using %s mode\n", (nChannels == 1) ? "Mono" : "Stereo");

	FUNC_END;
	return (nChannels);
}

/*!
 * This function handles the OSS command that sets the channel number
 * (1 for mono, 2 for stereo).
 *
 * @param    dev      [in] the device's major number
 * @param    channels [in] the channel number to set (1 or 2)
 *                         If 0, then do not set, instead return current value.
 * @return   the channel number set in the device
 *           (may be different than the one requested)
 *
 */
static signed short mxc_audio_setchannel(const int dev,
					 const signed short channels)
{
	mxc_state_t *drv_inst = NULL;
	signed short nChannels = channels;

	/*! Get the drv instance private data */
	if (mxc_get_drv_data(dev, &drv_inst))
		return 0;

	switch (channels) {
	case 0:

		nChannels = mxc_get_channels(drv_inst);
		break;

	default:

		nChannels = mxc_set_channels(drv_inst, channels);
		break;
	}

	return nChannels;
}

/*!
 * This function returns the current sampling rate frequency (Hz).
 *
 * @param        drv_inst [in] info about the current transfer
 * @return        the current sampling rate (Hz)
 *
 */
static int mxc_get_speed(mxc_state_t * const drv_inst)
{
	int ret;

	down_interruptible(&drv_inst->mutex);
	ret = drv_inst->pmic.in_out_sr_Hz;
	up(&drv_inst->mutex);

	return ret;
}

/*!
 * This function sets the audio sampling rate frequency (Hz).
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        speed [in] the audio stream sampling frequency
 * @return        None
 *
 */
static int mxc_set_speed(mxc_state_t * const drv_inst, int speed)
{
	FUNC_START;
	down_interruptible(&drv_inst->mutex);

	/*! Do not change the sampling rate if we are in the middle of a
	 *  recording/playback operation or if the requested sampling rate
	 *  is already set.
	 */
	if ((drv_inst->device_busy) || (drv_inst->pmic.in_out_sr_Hz == speed)) {
		/*! Yes, the SSI channel is already active or it is already
		 *  configured for the desired sampling rate. Therefore, no
		 *  reconfiguration is allowed or required.
		 */
		speed = drv_inst->pmic.in_out_sr_Hz;

		DPRINTK("INFO: Using current SSI%d sampling rate of %d\n",
			drv_inst->ssi_index + 1, speed);
	} else {
		/*! No, the SSI channel is currently inactive so we can reconfigure
		 *  it to our desired sampling frequency (if it is supported by the
		 *  hardware).
		 */

		if (drv_inst->pmic.device == VOICE_CODEC) {
			/* The Voice CODEC has only 8 kHz and 16 kHz sampling rates. */
			if (speed >= (8000 + 16000) / 2) {
				speed = 16000;
			} else {
				speed = 8000;
			}

			if (speed != drv_inst->pmic.in_out_sr_Hz) {
				drv_inst->pmic.in_out_sr_Hz = speed;
			}

			DPRINTK
			    ("INFO: VOICE CODEC using sampling rate of %d Hz\n",
			     speed);
		} else if (drv_inst->pmic.device == STEREO_DAC) {
			/* Stereo DAC sampling rates from 8 kHz to 96 kHz (for mc13783 PMIC)
			 * and 8 kHz to 48 kHz (for sc55112 PMIC).
			 *
			 * Note that it is OK to check for the 64 kHz and 96 kHz sampling
			 * rates even when using the sc55112 PMIC because we've already
			 * made sure to limit the requested sampling rate to the value of
			 * MAX_SAMPLING_RATE defined above.
			 */
			if (speed >= (64000 + 96000) / 2) {
				speed = 96000;
			} else if (speed >= (48000 + 64000) / 2) {
				speed = 64000;
			} else if (speed >= (44100 + 48000) / 2) {
				speed = 48000;
			} else if (speed >= (32000 + 44100) / 2) {
				speed = 44100;
			} else if (speed >= (24000 + 32000) / 2) {
				speed = 32000;
			} else if (speed >= (22050 + 24000) / 2) {
				speed = 24000;
			} else if (speed >= (16000 + 22050) / 2) {
				speed = 22050;
			} else if (speed >= (12000 + 16000) / 2) {
				speed = 16000;
			} else if (speed >= (11025 + 12000) / 2) {
				speed = 12000;
			} else if (speed >= (8000 + 11025) / 2) {
				speed = 11025;
			} else {
				speed = 8000;
			}

			if (speed != drv_inst->pmic.in_out_sr_Hz) {
				drv_inst->pmic.in_out_sr_Hz = speed;
			}

			DPRINTK
			    ("INFO: Stereo DAC using sampling rate of %d Hz\n",
			     speed);
		}

		DPRINTK("INFO: SSI%d using sampling rate of %d Hz\n",
			drv_inst->ssi_index + 1, speed);
	}

	up(&drv_inst->mutex);

	FUNC_END;
	return speed;
}

/*!
 * This function handles the OSS command that sets the sampling frequency (Hz).
 *
 * @param    dev    [in] the device's major number
 * @param    speed  [in] the sampling frequency.
 *                       If 0, then don't set but just return the current value.
 * @return   the value set in the device
 *           (may be different that the one requested)
 *
 */
static int mxc_audio_setspeed(const int dev, const int speed)
{
	mxc_state_t *drv_inst = NULL;
	int freq = 0;

	/*! Get the drv instance private data */
	if (mxc_get_drv_data(dev, &drv_inst))
		return 0;

	switch (speed) {
	case 0:

		freq = mxc_get_speed(drv_inst);
		break;

	default:

		freq = mxc_set_speed(drv_inst, speed);
		break;
	}

	return freq;
}

/*!
 * This function handles the OSS SNDCTL_CLK_SET_MASTER IOCTL command
 * that selects the clock master device. One of the following modes
 * may be selected:
 *
 * - MXC_AUDIO_CLOCKING_PMIC_MASTER (for PMIC as clock master)
 * - MXC_AUDIO_CLOCKING_SSI_MASTER (for SSI as clock master)
 *
 * Note that we can only change the clock master mode when there is
 * no playback/recording operation in progress.
 *
 * @param    drv_inst [in] the device's major number
 * @param    mode     [in] the clock master mode to be used
 *
 * @return   the current clock master selection
 *           (may be different that the mode requested)
 */
static int mxc_setup_clk_master(mxc_state_t * const drv_inst, int mode)
{
	FUNC_START;
	down_interruptible(&drv_inst->mutex);

	if ((mode == MXC_AUDIO_CLOCKING_SSI_MASTER) ||
	    (mode == MXC_AUDIO_CLOCKING_PMIC_MASTER)) {
		/*! We cannot change the clock configuration while audio
		 *  playback or recording is in progress.
		 */
		if (!drv_inst->device_busy) {
			/*! The PMIC audio device is currently free/idle so we can
			 *  immediately update the master/slave clocking configuration.
			 */
			drv_inst->clock_master = mode;
		} else {
			/*! If we are in the middle of a playback/recording
			 *  operation, then just return the current master
			 *  clock selection.
			 */
			mode = drv_inst->clock_master;
		}
	} else {
		mode = drv_inst->clock_master;
	}

	up(&drv_inst->mutex);

	FUNC_END;
	return mode;
}

/*!
 * This function handles the user commands to the mixer driver.
 *
 * @param    dev      the device's major number
 * @param    cmd      a symbol that describes the command to apply
 * @param    arg      the command argument
 * @return   0 if success, else error
 * @see      mxc_audio_ioctl
 *
 */
static int mxc_mixer_ioctl(const int dev, const unsigned int cmd,
			   void __user * arg)
{
	int __user *p = arg;
	int ret = 1, val;
	mxc_state_t *drv_inst = NULL;

	DPRINTK("INFO: cmd = 0x%x\n", cmd);

	/*! Get the drv instance private data */
	if (mxc_get_drv_data(dev, &drv_inst))
		return -EINVAL;

	/*! Only accepts mixer (type 'M') IOCTLs. */
	if (_IOC_TYPE(cmd) != 'M') {
		ret = -EINVAL;
	} else {
		switch (cmd) {
		case SOUND_MIXER_READ_DEVMASK:

			/*! Available mixer channels. */
			ret = MXC_PLAYING_SRC_MASK | MXC_RECORDING_SRC_MASK;
			DPRINTK("SOUND_MIXER_READ_DEVMASK 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_OUTMASK:

			/* Mask available playing channels */
			ret = MXC_PLAYING_SRC_MASK;
			DPRINTK("SOUND_MIXER_READ_OUTMASK 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_RECMASK:

			/* Mask available recording channels */
			ret = MXC_RECORDING_SRC_MASK;
			DPRINTK("SOUND_MIXER_READ_RECMASK 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_STEREODEVS:

			/* Mask showing stereo-capable channels. */
			ret = MXC_STEREO_CHANNELS_MASK;
			DPRINTK("SOUND_MIXER_READ_STEREODEVS 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_CAPS:

			/* Only one recording source at a time */
			ret = SOUND_CAP_EXCL_INPUT;
			DPRINTK("SOUND_MIXER_READ_CAPS 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_OUTSRC:

			/* Gets the current active playing device */
			ret = pmic_get_src_out_selected(drv_inst);

			DPRINTK("SOUND_MIXER_READ_OUTSRC 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_WRITE_OUTSRC:

			/* Sets the current active playing device */
			if (get_user(val, p))
				return -EFAULT;

			if ((val == 0) || (val & MXC_PLAYING_SRC_MASK)) {
				/* no channel selected or valid src is selected  */
				DPRINTK("SOUND_MIXER_WRITE_OUTSRC 0x%.4x\n",
					val);
				ret = pmic_select_src_out(drv_inst, val);
			} else {
				/* invalid channel selection! */
				DPRINTK
				    ("SOUND_MIXER_WRITE_OUTSRC invalid channel!\n");
				ret = -EINVAL;
			}
			break;

		case SOUND_MIXER_READ_RECSRC:

			/* Gets the current active recording device */
			ret = pmic_get_src_in_selected(drv_inst);

			/* FIXME: TODO : if no src selected, must return something */

			DPRINTK("SOUND_MIXER_READ_RECSRC 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_WRITE_RECSRC:

			/* Sets the current active recording device */
			if (get_user(val, (int *)arg))
				return -EFAULT;

			if ((val == 0) || (val & MXC_RECORDING_SRC_MASK)) {
				/* no channel selected or valid src is selected  */
				DPRINTK("SOUND_MIXER_WRITE_RECSRC 0x%.4x\n",
					val);
				ret = pmic_select_src_in(drv_inst, val);
			} else {
				/* invalid channel selection! */
				DPRINTK
				    ("SOUND_MIXER_WRITE_RECSRC invalid channel!\n");
				ret = -EINVAL;
			}
			break;

		case SOUND_MIXER_WRITE_VOLUME:	/* Master output volume control. */

			/* Set and return new volume */
			if (get_user(val, p))
				return -EFAULT;

			DPRINTK("SOUND_MIXER_WRITE_VOLUME 0x%04x (desired)\n",
				val);

			ret = pmic_set_src_out_volume(drv_inst,
						      PMIC_SRC_OUT_HEADSET |
						      PMIC_SRC_OUT_EARPIECE |
						      PMIC_SRC_OUT_HANDSFREE |
						      PMIC_SRC_OUT_LINE_OUT,
						      val);

			DPRINTK("SOUND_MIXER_WRITE_VOLUME 0x%04x (set)\n", ret);
			break;

		case SOUND_MIXER_READ_VOLUME:	/* Master output volume level. */

			/* Get and return volume */
			ret = pmic_get_src_out_volume(drv_inst,
						      PMIC_SRC_OUT_HEADSET |
						      PMIC_SRC_OUT_EARPIECE |
						      PMIC_SRC_OUT_HANDSFREE |
						      PMIC_SRC_OUT_LINE_OUT);

			DPRINTK("SOUND_MIXER_READ_VOLUME 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_WRITE_PCM:	/* PCM stereo output volume. */

			/* Set and return new volume */
			if (get_user(val, p))
				return -EFAULT;

			ret =
			    pmic_set_src_out_volume(drv_inst,
						    PMIC_SRC_OUT_LINE_OUT, val);

			DPRINTK("SOUND_MIXER_WRITE_PCM 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_PCM:	/* PCM stereo volume level.  */

			/* Get and return volume */
			ret =
			    pmic_get_src_out_volume(drv_inst,
						    PMIC_SRC_OUT_LINE_OUT);

			DPRINTK("SOUND_MIXER_READ_PCM 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_SPEAKER:	/* handsfree */

			/* Get and return volume */
			ret =
			    pmic_get_src_out_volume(drv_inst,
						    PMIC_SRC_OUT_HANDSFREE);

			DPRINTK("SOUND_MIXER_READ_SPEAKER 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_PHONEOUT:	/* earpiece */

			/* Get and return volume */
			ret =
			    pmic_get_src_out_volume(drv_inst,
						    PMIC_SRC_OUT_EARPIECE);

			DPRINTK("SOUND_MIXER_READ_PHONEOUT 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_WRITE_SPEAKER:	/* handset */

			/* Set and return new volume */
			if (get_user(val, p))
				return -EFAULT;

			ret =
			    pmic_set_src_out_volume(drv_inst,
						    PMIC_SRC_OUT_HANDSFREE,
						    val);

			DPRINTK("SOUND_MIXER_WRITE_SPEAKER 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_WRITE_PHONEOUT:	/* earpiece */

			/* Set and return new volume */
			if (get_user(val, p))
				return -EFAULT;

			ret =
			    pmic_set_src_out_volume(drv_inst,
						    PMIC_SRC_OUT_EARPIECE, val);

			DPRINTK("SOUND_MIXER_WRITE_PHONEOUT 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_MIC:	/*! headset microphone volume */

			/* Get and return volume */
			ret =
			    pmic_get_src_in_volume(drv_inst,
						   PMIC_SRC_IN_HEADSET);

			DPRINTK("SOUND_MIXER_READ_MIC 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_PHONEIN:	/*! handset (mono or stereo) */

			/* Get and return volume */
			ret =
			    pmic_get_src_in_volume(drv_inst,
						   PMIC_SRC_IN_HANDSET);

			DPRINTK("SOUND_MIXER_READ_PHONEIN 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_READ_LINE:	/*! line input */

			/* Get and return volume */
			ret =
			    pmic_get_src_in_volume(drv_inst,
						   PMIC_SRC_IN_LINE_IN);

			DPRINTK("SOUND_MIXER_READ_LINE 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_WRITE_MIC:	/*! headset microphone volume */

			/* Set and return new volume */
			if (get_user(val, p))
				return -EFAULT;

			ret =
			    pmic_set_src_in_volume(drv_inst,
						   PMIC_SRC_IN_HEADSET, val);

			DPRINTK("SOUND_MIXER_WRITE_MIC 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_WRITE_PHONEIN:	/*! handset (mono or stereo) */

			/* Set and return new volume */
			if (get_user(val, p))
				return -EFAULT;

			ret =
			    pmic_set_src_in_volume(drv_inst,
						   PMIC_SRC_IN_HANDSET, val);

			DPRINTK("SOUND_MIXER_WRITE_PHONEIN 0x%.4x\n", ret);
			break;

		case SOUND_MIXER_WRITE_LINE:	/*! line input */

			/* Set and return new volume */
			if (get_user(val, p))
				return -EFAULT;

			ret = pmic_set_src_in_volume(drv_inst,
						     PMIC_SRC_IN_LINE_IN, val);

			DPRINTK("SOUND_MIXER_WRITE_LINE 0x%.4x\n", ret);
			break;

		default:

			DPRINTK("!!! ERROR: Unknown/unsupported mixer IOCTL "
				"command 0x%x\n", cmd);

			ret = -EINVAL;
		}
	}

	return put_user(ret, (int __user *)arg);
}

/*!
 * This function handles the user commands to the audio driver
 *
 * @param    dev      the device's major number
 * @param    cmd      a symbol that describes the command to apply
 * @param    arg      the command argument
 * @return   0 if success, else error
 * @see      mxc_mixer_ioctl
 *
 */
static int mxc_audio_ioctl(const int dev, const unsigned int cmd,
			   void __user * arg)
{
	int val = 0;
	int __user *p = arg;
	mxc_state_t *drv_inst = NULL;

	DPRINTK("INFO: cmd = 0x%x\n", cmd);

	if (mxc_get_drv_data(dev, &drv_inst))
		return -EINVAL;

	switch (cmd) {
	case SNDCTL_DSP_STEREO:

		/* Sets stereo/mono mode (see also SOUND_PCM_WRITE_CHANNELS). */
		if (get_user(val, p))
			return -EFAULT;

		/* val=0 is Mono audio, val=1 is Stereo audio. */
		val = mxc_audio_setchannel(dev, val + 1) - 1;
		break;

	case SOUND_PCM_WRITE_CHANNELS:

		/* Sets stereo/mono mode (see also SNDCTL_DSP_STEREO). */
		if (get_user(val, p))
			return -EFAULT;

		/* val=1 is Mono audio, val=2 is Stereo audio. */
		val = mxc_audio_setchannel(dev, val);
		break;

	case SOUND_PCM_READ_CHANNELS:

		/* Get the number of audio channels in use (Mono=1, Stereo=2). */
		val = mxc_audio_setchannel(dev, 0);
		break;

	case SOUND_PCM_SETFRAGMENT:	/* Alias for SNDCTL_DSP_SETFRAGMENT */

		/* We ignore all fragment size requests. */
		val = 0;
		break;

	case SOUND_PCM_WRITE_RATE:	/* Alias for SNDCTL_DSP_SPEED */

		/* Set the sampling rate (Hz). */
		if (get_user(val, p))
			return -EFAULT;

		DPRINTK("INFO: SOUND_PCM_WRITE_RATE 0x%x\n", val);

		val = mxc_audio_setspeed(dev, val);
		break;

	case SOUND_PCM_READ_RATE:

		/* Return current sampling rate (Hz). */
		val = mxc_audio_setspeed(dev, 0);
		break;

	case SOUND_PCM_WRITE_BITS:	/* Alias for SNDCTL_DSP_SETFMT */

		/* Set the number of bits used for each audio sample. */
		if (get_user(val, p))
			return -EFAULT;

		DPRINTK("INFO: SOUND_PCM_WRITE_BITS: val = %d\n", val);

		val = mxc_audio_setbits(dev, val);
		break;

	case SOUND_PCM_READ_BITS:

		DPRINTK("INFO: SOUND_PCM_READ_BITS\n");

		/* Return the number of bits used for each audio sample. */
		val = mxc_audio_setbits(dev, 0);
		break;

	case SNDCTL_CLK_SET_MASTER:

		/* Selects master/slave clock configuration. */
		if (get_user(val, p))
			return -EFAULT;

		val = mxc_setup_clk_master(drv_inst, val);
		break;

	case SNDCTL_PMIC_WRITE_OUT_ADDER:

		/* Configures the audio output adder. */
		if (get_user(val, p))
			return -EFAULT;

		val = pmic_set_output_adder(drv_inst, val);
		break;

	case SNDCTL_PMIC_READ_OUT_ADDER:

		/* Returns the current audio output adder configuration. */
		val = pmic_get_output_adder(drv_inst);
		break;

	case SNDCTL_PMIC_WRITE_OUT_BALANCE:

		/* Configures the audio output balance. */
		if (get_user(val, p))
			return -EFAULT;

		val = pmic_set_output_balance(drv_inst, val);
		break;

	case SNDCTL_PMIC_READ_OUT_BALANCE:

		/* Returns the current audio output balance configuration. */
		val = pmic_get_output_balance(drv_inst);
		break;

	case SNDCTL_PMIC_WRITE_CODEC_FILTER:

		/* Configures the audio input/output filter. */
		if (get_user(val, p))
			return -EFAULT;

		val = pmic_set_codec_filter(drv_inst, val);
		break;

	case SNDCTL_PMIC_READ_CODEC_FILTER:

		/* Returns the current audio input/output filter configuration. */
		if (get_user(val, p))
			return -EFAULT;

		val = pmic_get_codec_filter(drv_inst);
		break;

	default:

		DPRINTK("!!! ERROR: unknown/unsupported dsp IOCTL command "
			"0x%x !!!\n", cmd);
		return -EINVAL;
	}

	return put_user(val, (int __user *)arg);
}

/*!
 * This function halts the audio hardware and data transfers in both directions.
 *
 * @param    dev      the device's major number
 * @return   None
 *
 */
static void mxc_audio_halt(const int dev)
{
	mxc_state_t *drv_inst = NULL;

	if (mxc_get_drv_data(dev, &drv_inst))
		return;

	down_interruptible(&drv_inst->mutex);

	/* Reset the SSI. */
	mxc_reset_ssi(drv_inst->ssi_index);

	/* Disable SSI Tx interrupts, the FIFO, and the transmitter. */
	ssi_interrupt_disable(drv_inst->ssi_index, ssi_tx_dma_interrupt_enable);
	ssi_tx_fifo_enable(drv_inst->ssi_index, drv_inst->ssi_fifo_nb, false);
	ssi_transmit_enable(drv_inst->ssi_index, false);
	drv_inst->judge[J_PLAY] = 0;

	/* Disable SSI Rx interrupts, the FIFO, and the receiver. */
	ssi_interrupt_disable(drv_inst->ssi_index, ssi_rx_dma_interrupt_enable);
	ssi_rx_fifo_enable(drv_inst->ssi_index, drv_inst->ssi_fifo_nb, false);
	ssi_receive_enable(drv_inst->ssi_index, false);
	drv_inst->judge[J_REC] = 0;

	up(&drv_inst->mutex);
}

#ifdef CONFIG_SOUND_MXC_FIFO

/*!
 * This function writes the given buffer to the SSI, in FIFO mode
 *
 * @param    dev              the device's major number
 * @param    buf                the buffer to write on the bus
 * @param    count        the number of bytes to write
 * @return   0
 *
 */
static int mxc_audio_write_fifo(const int dev, const char *buf, int count)
{
	mxc_state_t *drv_inst = NULL;
	ssi_mod ssi_index;
	fifo_nb fifo_index;
	int cnt, i, j, k, left, right, loops;
	FUNC_START;
	/*! Get the drv instance private data */
	if (mxc_get_drv_data(dev, &drv_inst))
		return 0;

	ssi_index = drv_inst->ssi_index;
	fifo_index = drv_inst->ssi_fifo_nb;

	ssi_transmit_enable(ssi_index, true);
	ssi_tx_fifo_enable(ssi_index, ssi_fifo_0, true);
	ssi_tx_fifo_enable(ssi_index, ssi_fifo_1, true);

#ifdef MXC_HIZ_WORKAROUND
	ssi_tx_mask_time_slot(ssi_index, drv_inst->ssi_tx_timeslots);
#endif

	cnt = count;
	k = 0;

	/* Check whether we are handling stereo (2 channels)/mono (1 channel)
	 * audio data.
	 */
	if (drv_inst->in_out_channel_num == 2) {
		while (cnt > 0) {
			loops = ((cnt > MXC_FIFO_SIZE) ? MXC_FIFO_SIZE : cnt);
			DPRINTK("INFO: Copy from user (%d)...", loops);
			if (copy_from_user(snd_bufplay[ssi_index],
					   &(buf)[k], loops))
				goto _err;
			k += loops;
			DPRINTK(" completed\n");

			for (i = 0; i < loops; i += 4) {
				left = ((snd_bufplay[ssi_index][i + 1] << 8) |
					snd_bufplay[ssi_index][i]);
				right = ((snd_bufplay[ssi_index][i + 3] << 8) |
					 snd_bufplay[ssi_index][i + 2]);

				ssi_set_data(ssi_index, fifo_index, left);
				ssi_set_data(ssi_index, fifo_index, right);

				j = 0;
				do {
					j++;
					if (j > 1000000) {
						DPRINTK
						    ("INFO: Timeout -> exit\n");
						i = loops;
						cnt = -1;
						break;
					}
				} while (ssi_tx_fifo_counter
					 (ssi_index, fifo_index) >= 6);
			}

			cnt -= MXC_FIFO_SIZE;
		}
	} else {
		while (cnt > 0) {
			loops = ((cnt > MXC_FIFO_SIZE) ? MXC_FIFO_SIZE : cnt);
			DPRINTK("INFO: Copy from user (%d)...", loops);
			if (copy_from_user
			    (snd_bufplay[ssi_index], &(buf)[k], loops))
				goto _err;;
			k += loops;
			DPRINTK(" completed\n");

			for (i = 0; i < loops; i += 2) {
				left = ((snd_bufplay[ssi_index][i + 1] << 8) |
					snd_bufplay[ssi_index][i]);
				ssi_set_data(ssi_index, fifo_index, left);

#ifdef MXC_HIZ_WORKAROUND
				ssi_set_data(ssi_index, fifo_index, 0);
#endif

				j = 0;
				do {
					j++;
					if (j > 1000000) {
						DPRINTK
						    ("INFO: Timeout -> exit\n");
						i = loops;
						cnt = -1;
						break;
					}
				} while (ssi_tx_fifo_counter
					 (ssi_index, fifo_index) >= 6);
			}

			cnt -= MXC_FIFO_SIZE;
		}
	}

	FUNC_END;
	return (k);

      _err:
	FUNC_END;
	return -EFAULT;
}

static int mxc_audio_read_fifo(const int dev, char *const buf, int count)
{
	mxc_state_t *drv_inst = NULL;
	ssi_mod ssi_index;
	fifo_nb fifo_index;
	int cnt, w, i, j, k, left, loops;

	/*! Get the drv instance private data */
	if (mxc_get_drv_data(dev, &drv_inst))
		return 0;

	ssi_index = drv_inst->ssi_index;
	fifo_index = drv_inst->ssi_fifo_nb;

	ssi_receive_enable(ssi_index, true);
	ssi_rx_fifo_enable(ssi_index, ssi_fifo_0, true);
	ssi_rx_fifo_enable(ssi_index, ssi_fifo_1, true);

	w = cnt = count;
	k = 0;

	while (cnt > 0) {
		loops = ((cnt > MXC_FIFO_SIZE) ? MXC_FIFO_SIZE : cnt);

		for (i = 0; i < loops; i += 2) {
			left = ssi_get_data(ssi_index, fifo_index);
			snd_bufrec[i + 1] = left >> 8;
			snd_bufrec[i] = left & 0xFF;

			j = 0;
			do {
				j++;
				if (j > 1000000) {
					DPRINTK("INFO: Timeout -> exit\n");
					i = loops;
					cnt = -1;
					break;
				}
			} while (ssi_rx_fifo_counter(ssi_index, fifo_index) <=
				 2);
		}

		DPRINTK("INFO: Copy to user (%d)...", loops);
		if (copy_to_user(buf + k, (char *)snd_bufrec, loops))
			return -EFAULT;

		k += loops;
		DPRINTK(" completed (%d)\n", k);

		w -= loops;
		cnt -= MXC_FIFO_SIZE;
		if (cnt > 0)
			DPRINTK("INFO: Remains %d bytes\n", cnt);
	}

	DPRINTK("INFO: Bytes recorded: %d\n", count - w);
	return (count - w);
}

#endif				/* CONFIG_SOUND_MXC_FIFO */

#ifdef CONFIG_SOUND_MXC_SDMA

/*!
 * This function is the DMA interrupt handler for Tx
 * It is common for each Tx audio devices
 *
 * @param    arg contains specific information: the index of the DMA channel
 *           that issued the interruption
 * @return   None
 * @see sdma.c
 * @see sdma.h

 */
static void mxc_dma_tx_handler(void *const arg)
{
	mxc_state_t *drv_inst;

	drv_inst = (mxc_state_t *) arg;

	DMAbuf_outputintr(drv_inst->dev_index, 1);
}

/*!
 * This function is the DMA interrupt handler for Rx
 * It is common for each Rx audio devices
 *
 * @param    arg contains specific information:
 *        the index of the DMA channel that issued the interruption
 * @return   None
 * @see sdma.c
 * @see sdma.h
 *
 */
static void mxc_dma_rx_handler(void *const arg)
{
	mxc_state_t *drv_inst;

	drv_inst = (mxc_state_t *) arg;

//    local_save_flags(flags);
	DMAbuf_inputintr(drv_inst->dev_index);
//    local_irq_restore(flags);
}

/*!
 * This function configures the DMA controller for Tx channel
 *
 * @param    drv_inst [in] contains specific information
 * @param    params [out] info to fill for the tx transfer
 * @return   0 if success
 *
 */
static inline void mxc_setup_tx_dma(const mxc_state_t * const drv_inst,
				    dma_channel_params * const params)
{
	params->watermark_level = TX_FIFO_EMPTY_WML;
	params->transfer_type = emi_2_per;
	params->callback = mxc_dma_tx_handler;
	params->arg = (void *)drv_inst;
	params->bd_number = 1;

	switch (drv_inst->in_out_bits_per_sample) {
	case AFMT_U8:

		params->word_size = TRANSFER_8BIT;
		break;

	default:		/* AFMT_S16_LE */

		params->word_size = TRANSFER_16BIT;
	}

	DPRINTK("INFO: Tx DMA transfer size = %d bits\n",
		(params->word_size == TRANSFER_8BIT) ? 8 : 16);

	if (drv_inst->ssi_fifo_nb == ssi_fifo_0) {
		if (drv_inst->ssi_index == SSI1) {
			/* fifo 0, SSI1 */
			params->per_address = (SSI1_BASE_ADDR + 0);	/*STX10 */
			params->event_id = DMA_REQ_SSI1_TX1;
			params->peripheral_type = SSI;
		} else {
			/* fifo 0, SSI2 */
			params->per_address = (SSI2_BASE_ADDR + 0);	/*STX20 */
			params->event_id = DMA_REQ_SSI2_TX1;
			params->peripheral_type = SSI_SP;
		}
	} else {
		if (drv_inst->ssi_index == SSI1) {
			/* fifo 1, SSI1 */
			params->per_address = (SSI1_BASE_ADDR + 0x4);	/*STX11 */
			params->event_id = DMA_REQ_SSI1_TX2;
			params->peripheral_type = SSI;
		} else {
			/* fifo 1, SSI2 */
			params->per_address = (SSI2_BASE_ADDR + 0x4);	/*STX21 */
			params->event_id = DMA_REQ_SSI2_TX2;
			params->peripheral_type = SSI_SP;
		}
	}
}

/*!
 * This function configures the DMA controller for Rx channel
 *
 * @param    drv_inst [in] contains specific information
 * @param    params [out] info to fill for the rx transfer
 * @return   0 if success
 *
 */
static inline void mxc_setup_rx_dma(const mxc_state_t * const drv_inst,
				    dma_channel_params * const params)
{
	params->watermark_level = RX_FIFO_FULL_WML;
	params->transfer_type = per_2_emi;
	params->callback = mxc_dma_rx_handler;
	params->arg = (void *)drv_inst;
	params->bd_number = 1;

	switch (drv_inst->in_out_bits_per_sample) {
	case AFMT_U8:

		params->word_size = TRANSFER_8BIT;
		break;

	default:		/* AFMT_S16_LE */

		params->word_size = TRANSFER_16BIT;
	}

	DPRINTK("INFO: Rx DMA transfer size = %d bits\n",
		(params->word_size == TRANSFER_8BIT) ? 8 : 16);

	if (drv_inst->ssi_fifo_nb == ssi_fifo_0) {
		if (drv_inst->ssi_index == SSI1) {
			/* TS 0, SSI1 */
			params->per_address = (SSI1_BASE_ADDR + 0x8);	/*SRX10 */
			params->event_id = DMA_REQ_SSI1_RX1;
			params->peripheral_type = SSI;
		} else {
			/* TS 0, SSI2 */
			params->per_address = (SSI2_BASE_ADDR + 0x8);	/*SRX20 */
			params->event_id = DMA_REQ_SSI2_RX1;
			params->peripheral_type = SSI_SP;
		}
	} else {
		if (drv_inst->ssi_index == SSI1) {
			/* TS 1, SSI1 */
			params->per_address = (SSI1_BASE_ADDR + 0xC);	/*SRX11 */
			params->event_id = DMA_REQ_SSI1_RX2;
			params->peripheral_type = SSI;
		} else {
			/* TS 1, SSI2 */
			params->per_address = (SSI2_BASE_ADDR + 0xC);	/*SRX21 */
			params->event_id = DMA_REQ_SSI2_RX2;
			params->peripheral_type = SSI_SP;
		}
	}
}

/*!
 * This function configures the DMA controller and requests the DMA interrupt
 *
 * @param    drv_inst [in] contains specific information
 * @param    inout [in] the direction of DMA transfer (rx/tx)
 * @return   0 if success
 *
 */
static int mxc_setup_dma(mxc_state_t * const drv_inst,
			 const e_transfer_dir_t inout)
{
	int dma_channel = 0;
	dma_channel_params params;

	/*! Find a free DMA channel */
	if (mxc_request_dma(&dma_channel, "TX SDMA OSS") < 0)
		goto _err;

	if (inout == eDIR_TX_PLAY) {
		mxc_setup_tx_dma(drv_inst, &params);
		drv_inst->dma_playbuf = dma_channel;
	} else {
		mxc_setup_rx_dma(drv_inst, &params);
		drv_inst->dma_recbuf = dma_channel;
	}

	/* Set the SDMA configuration. */
	mxc_dma_setup_channel(dma_channel, &params);

	DPRINTK("INFO: SDMA channel allocated: %d\n", dma_channel);

	return 0;

      _err:
	FUNC_ERR;
	return -1;
}

#endif				/* CONFIG_SOUND_MXC_SDMA */

/*!
 * This function checks if the driver opening is possible (if resources free)
 * If opening allowed the field "open_mode" in mxc_state_t is initialized
 *
 * Note that the device instance mutex has already been acquired when
 * mxc_audio_open() was called.
 *
 * @param    drv_inst [in] driver instance private data
 * @param    mode [in] the driver opening mode (R, W, or RW)
 * @return   1 if opening allowed, else 0
 *
 */
static int mxc_open_allowed(mxc_state_t * const drv_inst, const int mode)
{
	int ret = 1;

	/*! The Stereo DAC only supports one open request at a time since
	 *  there is only one available playback channel.
	 */
	if ((drv_inst->pmic.device == STEREO_DAC) && (drv_inst->ssi_allocated)) {
		DPRINTK("!!! ERROR: Stereo DAC already opened\n");
		ret = 0;
	}
	/*! Also test for the READ/recording mode because recording is only
	 *  supported by the Voice CODEC.
	 */
	else if ((mode & OPEN_READ) && (drv_inst->pmic.device != VOICE_CODEC)) {
		DPRINTK("!!! ERROR: Only Voice CODEC can record\n");
		ret = 0;
	}
	/*! Finally, in the case of the Voice CODEC, we do allow separate open
	 *  requests for each of the recording and playback modes so long as
	 *  there is only one active request at a time.
	 */
	else if ((drv_inst->pmic.device == VOICE_CODEC) &&
		 (mode & drv_inst->open_mode)) {
		DPRINTK("!!! ERROR: Voice CODEC already opened\n");
		ret = 0;
	} else {
		/*! We have passed the basic validity checks and are now ready to
		 *  try performing any necessary PMIC-specific hardware setup.
		 */
		if ((mode & OPEN_READ) && (pmic_select_dev_in(drv_inst) == 0)) {
			/*! Failed to configure the audio input device. */
			DPRINTK("!!! ERROR: pmic_select_dev_in() failed\n");
			ret = 0;
		}
		if ((mode & OPEN_WRITE) && (pmic_select_dev_out(drv_inst) == 0)) {
			/*! Failed to configure the audio playback device. */
			DPRINTK("!!! ERROR: pmic_select_dev_out() failed\n");
			ret = 0;
		}

		if (ret != 0) {
			/*! The open request and PMIC hardware setup were successful,
			 *  all that's left is to update the state variables.
			 *
			 *  Note that we need to logically OR the mode with the existing
			 *  value to handle the case where the Voice CODEC was originally
			 *  opened in, for example, the recording mode and then later
			 *  opened again in the playback mode. The resulting mode should
			 *  then be both recording and playback.
			 */
			drv_inst->ssi_allocated = true;
			drv_inst->open_mode |= mode;

			/*! Update count of active playback audio streams. */
			if (mode & OPEN_WRITE)
				drv_inst->play_cnt++;

			/*! Update count of active recording audio streams. */
			if (mode & OPEN_READ)
				drv_inst->rec_cnt++;
		} else if (drv_inst->pmic.handle != (PMIC_AUDIO_HANDLE) NULL) {
			/*! The PMIC hardware setup failed, so undo anything that can be
			 *  undone and just a failure status.
			 */
			pmic_close_audio(drv_inst);
		}

		DPRINTK("INFO: Active streams: Recording [%d], Playback [%d]\n",
			drv_inst->rec_cnt, drv_inst->play_cnt);
	}

	return ret;
}

/*!
 * This function frees the SSI bus and all associated PMIC hardware components.
 *
 * Note that this function must be called with interrupts enabled because we
 * will be making calls down to both the SDMA and SPI drivers, both of which
 * will also want to reschedule. If interrupts are disabled when either the
 * SDMA or SPI drivers attempt to reschedule, then you will see warning
 * messages from the kernel similar to the following:
 *
 * BUG: scheduling while atomic ... or BUG: scheduling with irqs disabled ...
 *
 * @param    dev      the device's major number
 * @return   None
 *
 */
static void mxc_audio_close(const int dev)
{
	mxc_state_t *drv_inst = NULL;
	int dmabuf;

	/*! Get the audio driver instance private data. */
	if (mxc_get_drv_data(dev, &drv_inst))
		return;

	down_interruptible(&drv_inst->mutex);

	/*! Deallocate Tx DMA buffers if necessary. */
	if (drv_inst->open_mode & OPEN_WRITE) {
		dmabuf = drv_inst->dma_playbuf;

		if (dmabuf >= 0) {
			mxc_free_dma(dmabuf);
			drv_inst->dma_playbuf = -1;
			DPRINTK("INFO: Tx DMA channel freed: %i\n", dmabuf);
		}
	}

	/*! Deallocate Rx DMA buffers if necessary. */
	if (drv_inst->open_mode & OPEN_READ) {
		dmabuf = drv_inst->dma_recbuf;

		if (dmabuf >= 0) {
			mxc_free_dma(dmabuf);
			drv_inst->dma_recbuf = -1;
			DPRINTK("INFO: Rx DMA channel freed: %i\n", dmabuf);
		}
	}

	/*! Update the driver instance state. */
	if ((drv_inst->rec_cnt > 0) && (drv_inst->open_mode & OPEN_READ)) {
		drv_inst->rec_cnt--;

		if (drv_inst->rec_cnt == 0) {
			drv_inst->open_mode &= ~OPEN_READ;
		}
	}
	if ((drv_inst->play_cnt > 0) && (drv_inst->open_mode & OPEN_WRITE)) {
		drv_inst->play_cnt--;

		if (drv_inst->play_cnt == 0) {
			drv_inst->open_mode &= ~OPEN_WRITE;
		}
	}

	/*! If there are no active recording or playback streams left, then
	 *  we can shut everything down and reset all of the state variables.
	 */
	if ((drv_inst->rec_cnt == 0) && (drv_inst->play_cnt == 0)) {
		drv_inst->device_busy = false;
		drv_inst->ssi_allocated = false;

		/*! Note that we must explicitly reset the open_mode value to zero
		 *  here even though we've already removed the OPEN_READ and
		 *  OPEN_WRITE flags. The open_mode variable is also used by other
		 *  OSS modules to keep status information so it may not be zero
		 *  even after we've removed the OPEN_READ and OPEN_WRITE flags.
		 *
		 *  But at this point, we are sure that we can reset everything.
		 */
		drv_inst->open_mode = 0;
	}

	/*! Free (power down) PMIC audio hardware components that are no longer
	 *  required.
	 *
	 *  Note that we need to ensure that both input and output channels
	 *  are not in use before we power something down.
	 *
	 *  Also note that we should make all of these hardware changes inside
	 *  a mutual exclusion block since we want to avoid any chance of a race
	 *  condition with someone else opening up the device before we've
	 *  completed all of our shutdown steps.
	 */
	pmic_close_audio(drv_inst);

	up(&drv_inst->mutex);

	DPRINTK("INFO: Active streams : Recording [%d], Playback [%d]\n",
		drv_inst->rec_cnt, drv_inst->play_cnt);
}

/*!
 * This function tries to open the selected audio device. If the open
 * call is successful, then the audio hardware is also configured to
 * it's default configuration.
 *
 * The Voice CODEC is accessed using the /dev/sound/dsp device while
 * the Stereo DAC is accessed using /dev/sound/dsp1.
 *
 * @param    dev      the device's major number
 * @param    mode     The parameter flag is one of
 *                    OPEN_READ, OPEN_WRITE, OPEN_READWRITE
 * @return   0 if success,
 * @return   -ENXIO if initialization error
 * @return   -EBUSY if the driver instance (minor number) already in use
 *                  or if the hardware resources are not available
 * @see      sound_config.h
 *
 */
static int mxc_audio_open(const int dev, const int mode)
{
	mxc_state_t *drv_inst = NULL;
	int return_code = 0;

	/*! Get the audio driver instance private data. */
	if (mxc_get_drv_data(dev, &drv_inst)) {
		return_code = -ENXIO;
	} else {
		down_interruptible(&drv_inst->mutex);

		/*! Check if the audio driver instance supports the requested
		 *  I/O mode.
		 */
		if (mxc_open_allowed(drv_inst, mode)) {
			drv_inst->judge[J_REC] = 0;
			drv_inst->judge[J_PLAY] = 0;
		} else {
			return_code = -ENXIO;
		}

		up(&drv_inst->mutex);
	}

	return return_code;
}

/*!
 * This function arms/rearms the DMA for an output stream
 *
 * Note that this function must be called with interrupts enabled because we
 * will be calling down to the SDMA driver. The SDMA driver will attempt to
 * reschedule and this will produce the following kernel warning messages if
 * interrupts are disabled:
 *
 * BUG: scheduling while atomic ... or BUG: scheduling with irqs disabled ...
 *
 * @param    dev      the device's major number
 * @param    buf      the buffer to play
 * @param    count    the buffer size in bytes
 * @param    intrflag the OSS interrupt status flag
 * @return   None
 *
 */
static void mxc_audio_output_block(const int dev, const unsigned long buf,
				   const int count, const int intrflag)
{
#ifdef CONFIG_SOUND_MXC_SDMA

	mxc_state_t *drv_inst = NULL;
	dma_request_t sdma_request;

	/*! Get the drv instance private data */
	if (mxc_get_drv_data(dev, &drv_inst) == 0) {
		/*! Allocate a Tx DMA channel (i.e., replace the dummy DMA channel
		 *  parameter given in the original sound_install_audiodrv() call).
		 *
		 *  Note that we must wait until the first audio output block is
		 *  ready to be sent before actually allocating the DMA channel
		 *  because the number of bits/sample value may be changed by an OSS
		 *  application in between the mxc_audio_open() call and the call to
		 *  this function. But at this point, we do know the correct number
		 *  of bits/sample value that must be used to configure the word_size
		 *  parameter of the DMA channel.
		 */
		if (drv_inst->dma_playbuf < 0) {
			/*! If any errors occur while trying to allocate the DMA channel,
			 *  then we just call mxc_audio_close() to undo any partially
			 *  completed setup steps and return.
			 */
			if (mxc_setup_dma(drv_inst, eDIR_TX_PLAY)) {
				DPRINTK("!!! ERROR: Tx DMA setup failed\n");
				mxc_audio_close(dev);
				return;
			}
		}

		memset(&sdma_request, 0, sizeof(dma_request_t));

		sdma_request.sourceAddr = (char *)buf;
		sdma_request.count = count;

		mxc_dma_set_config(drv_inst->dma_playbuf, &sdma_request, 0);
		mxc_dma_start(drv_inst->dma_playbuf);
	}
#endif				/* CONFIG_SOUND_MXC_SDMA */

#ifdef CONFIG_SOUND_MXC_FIFO

	mxc_audio_write_fifo(dev, (char *)buf, count)
#endif				/* CONFIG_SOUND_MXC_FIFO */
}

/*!
 * This function arms/rearms the DMA for an input stream
 *
 * Note that this function must be called with interrupts enabled because we
 * will be calling down to the SDMA driver. The SDMA driver will attempt to
 * reschedule and this will produce the following kernel warning messages if
 * interrupts are disabled:
 *
 * BUG: scheduling while atomic ... or BUG: scheduling with irqs disabled ...
 *
 * @param    dev      the device's major number
 * @param    buf      the buffer where to receive data
 * @param    count    the buffer size in bytes
 * @param    intrflag the OSS interrupt status flag
 * @return   None
 *
 */
static void mxc_audio_start_input(const int dev, const unsigned long buf,
				  const int count, const int intrflag)
{
#ifdef CONFIG_SOUND_MXC_SDMA

	mxc_state_t *drv_inst = NULL;
	dma_request_t sdma_request;

	/*! Get the drv instance private data */
	if (mxc_get_drv_data(dev, &drv_inst) == 0) {
		/*! Allocate an Rx DMA channel (i.e., replace the dummy DMA channel
		 *  parameter given in the original sound_install_audiodrv() call).
		 *
		 *  Note that we must wait until the first audio input block is
		 *  ready to be received before actually allocating the DMA channel
		 *  because the number of bits/sample value may be changed by an OSS
		 *  application in between the mxc_audio_open() call and the call to
		 *  this function. But at this point, we do know the correct number
		 *  of bits/sample value that must be used to configure the word_size
		 *  parameter of the DMA channel.
		 */
		if (drv_inst->dma_recbuf < 0) {
			/*! If any errors occur while trying to allocate the DMA channel,
			 *  then we just call mxc_audio_close() to undo any partially
			 *  completed setup steps and return.
			 */
			if (mxc_setup_dma(drv_inst, eDIR_RX_REC)) {
				DPRINTK("!!! ERROR: Rx DMA setup failed\n");
				mxc_audio_close(dev);
				return;
			}
		}

		memset(&sdma_request, 0, sizeof(dma_request_t));

		sdma_request.destAddr = (char *)buf;
		sdma_request.count = count;

		mxc_dma_set_config(drv_inst->dma_recbuf, &sdma_request, 0);
		mxc_dma_start(drv_inst->dma_recbuf);
	}
#endif				/* CONFIG_SOUND_MXC_SDMA */

#ifdef CONFIG_SOUND_MXC_FIFO

	mxc_audio_read_fifo(dev, (char *)buf, count)
#endif				/* CONFIG_SOUND_MXC_FIFO */
}

/*!
 * This function triggers the hardware to launch audio input or output.
 *
 * Note that this function is called after calling mxc_audio_output_block()
 * or mxc_audio_start_input(). Therefore, all SSI, SDMA, Audio MUX, and PMIC
 * configuration must already be done by this point. The only things that we
 * need to do at this point is to enable the SSI transmitter/receiver as well
 * as the DMA interrupt generation capability.
 *
 * Also note that we need to be careful about adding debugging output to
 * this function as any additional delays may cause errors in the DMA
 * operations.
 *
 * @param    dev      [in] the device's major number
 * @param    state    [in] selects recording/playback mode
 * @return   None
 *
 */
static void mxc_audio_trigger(const int dev, int state)
{
#ifdef CONFIG_SOUND_MXC_SDMA

	struct dma_buffparms *dmap = NULL;
	mxc_state_t *drv_inst = NULL;

	/*! Get the audio driver instance private data */
	if (mxc_get_drv_data(dev, &drv_inst))
		return;

	down_interruptible(&drv_inst->mutex);

	state &= drv_inst->open_mode;

	if (state & PCM_ENABLE_OUTPUT) {
		dmap = audio_devs[dev]->dmap_out;

		/*! Enable the SSI DMA interrupt generation capability when we
		 *  attempt to send the first block of audio data.
		 */
		if (drv_inst->judge[J_PLAY] != 1) {
			DPRINTK("INFO: Triggering on Tx\n");

			ssi_interrupt_enable(drv_inst->ssi_index,
					     ssi_tx_dma_interrupt_enable);
			ssi_tx_fifo_enable(drv_inst->ssi_index,
					   drv_inst->ssi_fifo_nb, true);
			ssi_transmit_enable(drv_inst->ssi_index, true);
			drv_inst->judge[J_PLAY] = 1;
		}
	} else if (state & PCM_ENABLE_INPUT) {
		dmap = audio_devs[dev]->dmap_in;

		/*! Enable the SSI DMA interrupt generation capability when we
		 *  attempt to receive the first block of audio data.
		 */
		if (drv_inst->judge[J_REC] != 1) {
			DPRINTK("INFO: Triggering on Rx\n");

			ssi_interrupt_enable(drv_inst->ssi_index,
					     ssi_rx_dma_interrupt_enable);
			ssi_rx_fifo_enable(drv_inst->ssi_index,
					   drv_inst->ssi_fifo_nb, true);
			ssi_receive_enable(drv_inst->ssi_index, true);
			drv_inst->judge[J_REC] = 1;
		}
	}

	dmap->flags |= DMA_NOTIMEOUT | DMA_NODMA;

	up(&drv_inst->mutex);

#endif				/* CONFIG_SOUND_MXC_SDMA */
}

/*!
 * This function is called by OSS to prepare the hardware to play or record
 * an audio stream.
 *
 * We delay configuring the PMIC and platform audio hardware components
 * until this point so that we can minimize the number of hardware changes
 * that are made. Audio applications and the OSS sound driver will typically
 * make multiple changes to the sampling rate and other configuration
 * parameters as it prepares for a playback or recording operation. Only at
 * this point are we certain of the final configuration that will be used
 * for the audio stream.
 *
 * @param    dev      the device's major number
 * @param    bsize    size of each audio data buffer
 * @param    bcount   total number of audio data buffers to be transferred
 * @return   0 if success, -1 else
 *
 */
static int mxc_audio_prepare_for_io(const int dev, const int bsize,
				    const int bcount)
{
	mxc_state_t *drv_inst = NULL;

	/*! Get the audio driver instance private data. */
	if (mxc_get_drv_data(dev, &drv_inst))
		return -1;

	down_interruptible(&drv_inst->mutex);

	/*! Configure the PMIC audio hardware (either the Voice CODEC or the
	 *  Stereo DAC). Any invalid configurations will be detected and
	 *  result in the return of an error status.
	 */
	if (drv_inst->pmic.device == VOICE_CODEC) {
		TRY(mxc_setup_pmic_codec(drv_inst))
	} else if (drv_inst->pmic.device == STEREO_DAC) {
		TRY(mxc_setup_pmic_stdac(drv_inst))
	}

	/*! Configure the PMIC audio input and/or output sections depending
	 *  upon whether we are performing a recording and/or playback
	 *  operation.
	 */
	if (drv_inst->open_mode & OPEN_READ) {
		TRYEQ(pmic_enable_in_device(drv_inst, true), true)
	}
	if (drv_inst->open_mode & OPEN_WRITE) {
		TRYEQ(pmic_enable_out_device(drv_inst, true), true)
	}

	/*! Configure the SSI. */
	mxc_setup_ssi(drv_inst);

	/*! Configure the Digital Audio MUX. */
	mxc_setup_dam(drv_inst);

	/*! Mark the audio device as busy. This also prevents any further
	 *  changes to the audio hardware configuration settings until
	 *  playback/recording is finished or halted.
	 */
	drv_inst->device_busy = true;

	up(&drv_inst->mutex);

	return 0;

      CATCH:
	/*! Must still release the mutex before returning an error status. */
	up(&drv_inst->mutex);

	return -1;
}

/*!
 * This structure describes the audio operations available to OSS
 * applications.
 *
 */
static struct audio_driver mxc_audio_driver = {
	.owner = THIS_MODULE,
	.open = mxc_audio_open,
	.close = mxc_audio_close,
	.output_block = mxc_audio_output_block,
	.start_input = mxc_audio_start_input,
	.ioctl = mxc_audio_ioctl,
	.prepare_for_input = mxc_audio_prepare_for_io,
	.prepare_for_output = mxc_audio_prepare_for_io,
	.halt_io = mxc_audio_halt,
#if 0
	/* FIXME: TODO: Need to implement separate halt functions if the DMA_DUPLEX
	 *              flag is to be used.
	 */
	.halt_input = mxc_audio_halt_input,
	.halt_output = mxc_audio_halt_output,
#endif
	.trigger = mxc_audio_trigger,
	.set_speed = mxc_audio_setspeed,
	.set_bits = mxc_audio_setbits,
	.set_channels = mxc_audio_setchannel
};

/*!
 * This structure describes the mixer operations available to OSS
 * applications.
 *
 */
static struct mixer_operations mxc_mixer_operations = {
	.owner = THIS_MODULE,
	.id = "MXC mixer",
	.name = "MXC Mixer Driver",
	.ioctl = mxc_mixer_ioctl
};

/*!
 * This function registers each of the /dev/sound/dspX devices.
 *
 * Note that the configuration values used when calling this function are
 * totally dependent upon the actual hardware configuration being used.
 * The values that are currently defined and described below are appropriate
 * for the Freescale mxc91131 EVB platform. The settings will need to be checked
 * and possible modified if another hardware platform is being used.
 *
 * The /dev/sound/dsp device should be created using the following
 * configuration:
 *
 *     SSI1, FIFO0, Audio MUX Port 4, Voice CODEC
 *
 * We must use Audio MUX Port 4 with the Voice CODEC because Port 4 is
 * the only port with the RX data line connected. This is required in
 * order to perform audio recording.
 *
 * The /dev/sound/dsp1 device should be created using the following
 * configuration:
 *
 *     SSI2, FIFO0, Audio MUX Port 5, Stereo DAC
 *
 * We recommend using SSI2 with the Stereo DAC because this provides
 * the simplest and most direct SDMA route for transferring audio
 * data between memory and the PMIC Stereo DAC. We can also use Audio
 * MUX Port 5 with the Stereo DAC because the Stereo DAC does not have
 * any recording capabilities. This means that having the RX data line
 * on Port 5 permanently connected to ground is OK.
 *
 * @param    ssiSelect      [in] Selects either SSI1/SSI2.
 * @param    ssiFIFOSelect  [in] Selects either SSI FIFO0 or FIFO1.
 * @param    txTimeslotMask [in] Defines the SSI Tx timeslot mask.
 * @param    rxTimeslotMask [in] Defines the SSI Rx timeslot mask.
 * @param    audMUXSrcPort  [in] Selects either Audio MUX port 1/port 2.
 * @param    audMUXDestPort [in] Selects either Audio MUX port 4/port 5.
 * @param    dev            [in] Selects either PMIC Voice CODEC/Stereo DAC.
 * @param    microphone     [in] Selects default input microphone line.
 * @param    micBias        [in] Selects corresponding microphone bias circuit.
 * @param    outputPort     [in] Selects default audio output line.
 * @param    nChannels      [in] Selects default mono/stereo mode.
 * @param    samplingRateHz [in] Selects default audio sampling rate in Hz.
 * @return   0 if success, -1 else
 *
 */
static int register_dsp_device(const ssi_mod ssiSelect,
			       const unsigned ssiFIFOSelect,
			       const unsigned txTimeslotMask,
			       const unsigned rxTimeslotMask,
			       const unsigned audMUXSrcPort,
			       const unsigned audMUXDestPort,
			       const PMIC_AUDIO_SOURCE dev,
			       const PMIC_AUDIO_INPUT_PORT microphone,
			       const PMIC_AUDIO_MIC_BIAS micBias,
			       const PMIC_AUDIO_OUTPUT_PORT outputPort,
			       const unsigned nChannels,
			       const unsigned samplingRateHz)
{
	mxc_state_t *drv_inst;
	int ret = 0;

	if (!(card_instance->state[ssiSelect] =
	      (mxc_state_t *) kmalloc(sizeof(mxc_state_t), GFP_KERNEL))) {
		ret = -ENOMEM;
	} else {
		/*! Define an alias for convenience. */
		drv_inst = card_instance->state[ssiSelect];

		memset(drv_inst, 0, sizeof(mxc_state_t));

		/*! Keep a reference to the main audio card instance. */
		drv_inst->card = card_instance;

		/*! Define the default SSI configuration. */
		drv_inst->ssi_index = ssiSelect;
		drv_inst->ssi_tx_timeslots = txTimeslotMask;
		drv_inst->ssi_rx_timeslots = rxTimeslotMask;
		drv_inst->ssi_fifo_nb = ssiFIFOSelect;

		/*! Define the default Audio MUX configuration. */
		drv_inst->dam_src = audMUXSrcPort;
		drv_inst->dam_dest = audMUXDestPort;

		/*! Use dummy values for now. We will actually allocate the DMA
		 *  buffers and update these values when we actually open the
		 *  audio device for playback/recording.
		 */
		drv_inst->dma_playbuf = -1;
		drv_inst->dma_recbuf = -1;

		/*! Default audio stream configuration as being 16 bits/sample and
		 *  either Mono mode for the Voice CODEC or Stereo mode for the
		 *  Stereo DAC.
		 */
		drv_inst->in_out_channel_num = nChannels;
		drv_inst->in_out_bits_per_sample = AFMT_S16_LE;

		/*! Define the default PMIC configuration as follows:
		 *
		 *  - stereo headset output (for Stereo DAC) or mono speaker (for
		 *    Voice CODEC)
		 *  - highest output PGA gain (OSS volume level = 100)
		 *  - equal left/right output balance
		 *  - mono adder off (i.e., stereo output if possible)
		 *  - highest input amplifier gain (OSS volume level = 100)
		 *  - no input filter options
		 *  - 44.1 kHz (for Stereo DAC) or 8 kHz (for Voice CODEC) sampling
		 *    rate
		 *
		 *  A different sampling rate for either the Voice CODEC or the
		 *  Stereo DAC can be selected later by calling the function.
		 *  mxc_audio_setspeed().
		 */
		drv_inst->pmic.device = dev;
		drv_inst->pmic.out_line = outputPort;
		drv_inst->pmic.out_volume = 0x6464;	/* 0x64 = 100 */
		drv_inst->pmic.out_balance = 50;
		drv_inst->pmic.out_adder_mode = MONO_ADDER_OFF;
		drv_inst->pmic.in_line = microphone;
		drv_inst->pmic.micBias = micBias;
		drv_inst->pmic.in_volume = 0x6464;	/* 0x64 = 100 */
		drv_inst->pmic.in_bias = 0;
		drv_inst->pmic.in_out_filter = 0;
		drv_inst->pmic.in_out_sr_Hz = samplingRateHz;

		/* Default is to use PMIC as audio data bus clock master.
		 * This ensures that we can generate the exact clock signals
		 * required for all of the supported sampling rates.
		 *
		 * We cannot guarantee the ability to generate the exact
		 * clock signals when using the SSI's internal clock generator
		 * because the PLL that is actually used as the clock signal
		 * source may not be running at a suitable frequency.
		 */
		drv_inst->clock_master = MXC_AUDIO_CLOCKING_PMIC_MASTER;

		/*! Register the OSS audio driver and create the appropriate entries
		 *  in the /dev/sound directory.
		 *
		 *  Note that we register here with just dummy Rx and Tx DMA channels.
		 *  The actual DMA channels are allocated and configured when the audio
		 *  device is actually opened.
		 *
		 *  Also note that we set the flags argument to DMA_DUPLEX because the
		 *  SSIs as well as the PMIC Voice CODEC do support simultaneous
		 *  playback and recording. Using the DMA_DUPLEX flag will not have
		 *  any effect on the PMIC Stereo DAC (which has no recording
		 *  capability) since we explicitly deny any requests to operate the
		 *  Stereo DAC in recording mode.
		 */
		if ((drv_inst->dev_index = sound_install_audiodrv(AUDIO_DRIVER_VERSION, "MXC PMIC audio driver", &mxc_audio_driver, sizeof(struct audio_driver), NEEDS_RESTART | DMA_DUPLEX, AUDIO_FORMATS, (void *)card_instance->state[ssiSelect], -1,	/* Dummy TX DMA channel. */
								  -1
								  /* Dummy RX DMA channel. */
		     )) < 0) {
			drv_inst->dev_index = -1;
			ret = -EMFILE;
		} else {
			/*! Finally, initialize the spinlock and Mutex to provide
			 *  mutual exclusion.
			 */
			spin_lock_init(&drv_inst->drv_inst_lock);
			init_MUTEX(&drv_inst->mutex);

			DPRINTK("INFO: Installed audio driver at index %d\n",
				ssiSelect);
		}
	}

	return ret;
}

/*!
 * This function frees resources allocated by the driver during the
 * init_mxc_audio() call.
 *
 */
static void cleanup_mxc_audio(void)
{
	int i;
	const unsigned nInstances = sizeof(card_instance->state) /
	    sizeof(card_instance->state[0]);

	/*! If the card_instance is NULL, then nothing needs to be freed */
	if (!card_instance)
		goto _end;

	/*! Free all audio driver instance members. */
	for (i = 0; i < nInstances; i++) {
		if (card_instance->state[i] != (mxc_state_t *) NULL) {
			/*! Note that any DMA channels and buffers that were allocated
			 *  have already been freed when mxc_audio_close() was called.
			 *  So we don't need to do anything more here.
			 */

			/*! Make sure that all audio I/O has been stopped. */
			mxc_audio_halt(card_instance->state[i]->dev_index);

			/*! Free up the PMIC device handle if it is still being held. */
			pmic_close_audio(card_instance->state[i]);

			/*! Tell OSS to unload this instance of the audio driver.
			 *  This will also remove the /dev/sound/dspX entry.
			 */
			if (card_instance->state[i]->dev_index >= 0) {
				sound_unload_audiodev(card_instance->state[i]->
						      dev_index);
				DPRINTK
				    ("INFO: Uninstalled audio driver at index %d\n",
				     i);
				card_instance->state[i]->dev_index = -1;
			}

			/*! Finally, release the memory for this driver instance. */
			kfree(card_instance->state[i]);
			card_instance->state[i] = (mxc_state_t *) NULL;
		}
	}

	/*! Tell OSS to unload the mixer driver. */
	if (card_instance->dev_mixer >= 0) {
		sound_unload_mixerdev(card_instance->dev_mixer);
		DPRINTK("INFO: Uninstalled audio mixer driver\n");
		card_instance->dev_mixer = -1;
	}

	/*! One final check to see if everything was closed properly. */
	CHECK_IF_0(card_instance->play_cnt, "play_cnt");
	CHECK_IF_0(card_instance->rec_cnt, "rec_cnt");

	/*! Finally, release the memory for the audio card instance. */
	kfree(card_instance);
	card_instance = NULL;

#ifdef CONFIG_SOUND_MXC_SDMA

	/*! SPBA configuration for SSI2 to release the SDMA. */
	if (spba_rel_ownership(SPBA_SSI2, SPBA_MASTER_C)) {
		DPRINTK("!!! ERROR: Failed to release SSI2 ownership !!!\n");
	}
#endif

	/*! Reset all of the audio hardware components so that we leave
	 *  everything in a consistent state.
	 */
	mxc_audio_hw_reset();

      _end:
	DPRINTK("INFO: mxc-pmic-oss audio driver unloaded\n");
}

/*!
 * This function initializes the audio hardware components and registers
 * all audio drivers with the core OSS driver.
 *
 * If everything completes successfully, then we will have the following
 * entries in the /dev/sound directory:
 *
 * - /dev/sound/dsp for accessing the Voice CODEC
 * - /dev/sound/dsp1 for accessing the Stereo DAC
 * - /dev/sound/mixer for controlling the PMIC audio input/output sections
 *
 * We will have also created a single instance of an "audio card" data
 * structure that contains two instances of an "audio driver" data structure
 * (one instance for each /dev/sound/dspX device). The "audio card" will
 * also contain one instance of a "mixer" data structure that will be used
 * to manage the /dev/sound/mixer device.
 *
 * @return   =0 if successful, <0 for error
 *
 */
static int init_mxc_audio(void)
{
	int ret = 0;

	DPRINTK("INFO: Using MXC sound driver built on %s at %s\n",
		__DATE__, __TIME__);

	/*! Start by resetting all of the audio hardware components so that
	 *  we are sure to start off in a consistent state.
	 */
	mxc_audio_hw_reset();

	/*! Allocate and initialize the card-specific info structure. We
	 *  use a single instance of an "audio card" to manage all of the
	 *  available audio-related hardware components. This includes the
	 *  PMIC as well as all of the platform components (i.e., the SSI,
	 *  SDMA, and Audio MUX).
	 */
	if (!(card_instance = (mxc_card_t *) kmalloc(sizeof(mxc_card_t),
						     GFP_KERNEL))) {
		ret = -ENOMEM;
		goto _fail;
	}
	memset(card_instance, 0, sizeof(mxc_card_t));

	/*! Create /dev/sound/dsp for accessing the PMIC Voice CODEC. Use
	 *  the following default configuration options:
	 *
	 *  - SSI1 using FIFO0
	 *  - SSI1 in synchronous network mode using only the first Rx/Tx
	 *    timeslot
	 *  - SSI1 using SDMA
	 *  - Audio MUX Port1 connected to Audio Mux Port4
	 *  - PMIC Voice CODEC as audio input/output device
	 *  - PMIC Voice CODEC acting as bus clock master
	 *  - mono input using MIC_AUX microphone input
	 *  - enable MIC_BIAS2 microphone bias circuit
	 *  - mono output on right channel of audio jack
	 *  - use 16 kHz sampling rate
	 */
	if (register_dsp_device(SSI1,
				SSI_DEFAULT_FIFO_NB,
				SSI_TIMESLOT_MASK_MONO,
				SSI_TIMESLOT_MASK_MONO,
				SSI1_AUDMUX_PORT,
				VCODEC_AUDMUX_PORT,
				VOICE_CODEC,
				MIC2_AUX,
				MIC_BIAS2,
				STEREO_HEADSET_RIGHT,
				N_CHANNELS_MONO, 16000) != 0) {
		ret = -EMFILE;
		goto _fail;
	}

	/*! Create /dev/sound/dsp1 for accessing the PMIC Stereo DAC. Use
	 *  the following default configuration options:
	 *
	 *  - SSI2 using FIFO0
	 *  - SSI2 in synchronous network mode using only the first two
	 *    Tx timeslots (the Stereo DAC cannot record, therefore, we
	 *    don't need to use any of the Rx timeslots)
	 *  - SSI2 using SDMA
	 *  - Audio MUX Port2 connected to Audio Mux Port5
	 *  - PMIC Stereo DAC as audio input/output device
	 *  - PMIC Stereo DAC acting as bus clock master
	 *  - no recording capability
	 *  - stereo output on right+left channels of audio jack
	 *  - use 44.1 kHz sampling rate
	 *
	 *  Note that the current Virtio model for the Audio MUX only
	 *  connects Port4 to the PMIC. This limitation has been handled
	 *  in the code.
	 */
	if (register_dsp_device(SSI2,
				SSI_DEFAULT_FIFO_NB,
				SSI_TIMESLOT_MASK_STEREO,
				SSI_TIMESLOT_MASK_ALL,
				SSI2_AUDMUX_PORT,
				STDAC_AUDMUX_PORT,
				STEREO_DAC,
				NO_MIC,
				NO_BIAS,
				STEREO_DAC_DEFAULT_OUTPUT,
				N_CHANNELS_STEREO, 44100) != 0) {
		ret = -EMFILE;
		goto _fail;
	}

	/*! Create a /dev/sound/mixer device to access the PMIC mixer
	 *  devices. The mixer device is also used to control the audio
	 *  left/right balance and volume levels for both recording and
	 *  playback operations.
	 *
	 *  Note that we only need a single mixer device because there
	 *  is only one input and one output section section in the PMIC.
	 *  Therefore, the settings can be controlled by a single mixer
	 *  device for both the Stereo DAC and the Voice CODEC.
	 */
	if ((card_instance->dev_mixer =
	     sound_install_mixer(MIXER_DRIVER_VERSION,
				 mxc_mixer_operations.name,
				 &mxc_mixer_operations,
				 sizeof(struct mixer_operations),
				 (void *)card_instance)) < 0) {
		ret = -EMFILE;
		goto _fail;
	}
#ifdef CONFIG_SOUND_MXC_SDMA

	/*! SPBA configuration for using SDMA with SSI2. */
	ret |= spba_take_ownership(SPBA_SSI2, SPBA_MASTER_SELECT);

#endif				/* CONFIG_SOUND_MXC_SDMA */

#ifdef CONFIG_ARCH_MXC91331

	/*! Special platform-specific SSI configuration. */
	ModifyRegister(0x18000000, 0x00000000, _reg_MCU_MCR);
	ModifyRegister(0x0003FFFF, 0x00000C06, _reg_MCU_MPDR1);

#endif				/* CONFIG_ARCH_MXC91331 */

	/*! Sound driver successfully loaded and initialized. */
	if (ret == 0) {
		DPRINTK("INFO: mxc-pmic-oss audio driver loaded\n");
		goto _end;
	}

      _fail:
	/*! Sound driver failed to load, free all resources. */
	cleanup_mxc_audio();
	DPRINTK("!!! ERROR: mxc-pmic-oss audio driver failed to load !!!\n");

      _end:
	return ret;
}

module_init(init_mxc_audio);
module_exit(cleanup_mxc_audio);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("OSS Audio Device Driver for PMIC");
MODULE_LICENSE("GPL");
