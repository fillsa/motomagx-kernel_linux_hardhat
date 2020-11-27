/*
 * linux/sound/oss/mxc_audio_mc13783.c
 *
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

 /*!
  * @file       mxc_audio_mc13783.c
  * @brief      this file implements the mxc sound driver interface for OSS
  *             framework. The mxc sound driver supports mono/stereo recording
  *             (there are some limitations due to hardware), mono/stereo
  *             playback and audio mixing. Recording supports 8000 khz and
  *             16000 khz sample rate. Playback supports 8000, 11025, 16000,
  *             22050, 24000, 32000, 44100 and 48000 khz for mono and stereo.
  *
  * @ingroup    SOUND_DRV
  */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/preempt.h>
#include <linux/delay.h>

#include <asm/uaccess.h>
#include <asm/dma.h>
#include <asm/system.h>

#include <asm/arch/audio_controls.h>
#include <asm/hardware.h>
#include <asm/arch/dma.h>
#include <asm/arch/spba.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/mach-types.h>

#include <ssi/ssi.h>
#include <ssi/registers.h>
#include <dam/dam.h>

#include <mc13783_legacy/core/mc13783_external.h>
#include <mc13783_legacy/module/mc13783_audio.h>
#include <mc13783_legacy/module/mc13783_audio_defs.h>

#include "sound_config.h"	/*include many macro definition */
#include "dev_table.h"		/*sound_install_audiodrv()... */
#include "mxc_audio_mc13783.h"

/* #define DEBUG */

#ifdef DEBUG
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

#define KERNEL_MSG(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

/*!
 * This structure stores current state of
 * mc13783 board. It is used to set/get current
 * audio configuration.
 */
struct mc13783_device {
	/*!
	 * This variable tells us whether mc13783 will use
	 * the stereodac or the codec.
	 */
	int device;

	/*!
	 * This variable holds the sample rate currently being used.
	 */
	int sample_rate;

	/*!
	 * This variable holds the current protocol mc13783 is using.
	 * mc13783 can use one of three protocols at any given time:
	 * normal, network and I2S.
	 */
	int protocol;

	/*!
	 * This variables tells us whether mc13783 runs in
	 * master mode (mc13783 generates audio clocks)or slave mode (AP side
	 * generates audio clocks)
	 *
	 * Currently the default mode is master mode because mc13783 clocks have
	 * higher precision.
	 */
	int mode;

	/* This variable holds the value representing the
	 * base clock mc13783 will use to generate internal
	 * clocks (BCL clock and FrameSync clock)
	 */
	int pll;

	/*!
	 * This variable holds the SSI to which mc13783 is currently connected.
	 */
	int ssi;

	/*!
	 * This variable tell us whether bit clock is inverted or not.
	 */
	int bcl_inverted;

	/*!
	 * This variable tell us whether frame clock is inverted or not.
	 */
	int fs_inverted;

	/*!
	 * This variable holds the pll used for mc13783 audio operations.
	 */
	int pll_rate;

	/*!
	 * This variable holds the adder mode being used. 4 options are
	 * possible: mono, stereo, mono_opposit and stereo_opposit.
	 */
	int adder_mode;

	/*!
	 * This variable holds the filter that mc13783 is applying to
	 * CODEC operations.
	 */
	int codec_filter;

	/*!
	 * This variable holds the current balance.
	 */
	int balance;
};

/*!
 * This structure represents device's state from an OS point of view.
 */
struct device_data {
	/*!
	 * SSI port to be used on AP side.
	 */
	int ssi;

	/*!
	 * FIFO used to receive/send samples from/to SSI.
	 */
	int ssi_fifo;

	/*!
	 * ID of this device. Assigned by OSS framework.
	 */
	int device_index;

	/*!
	 * ID of DMA write channel.
	 */
	int dma_wchannel;

	/*!
	 * ID of DMA read channel.
	 */
	int dma_rchannel;

	/*!
	 * This variable holds the audio sample's bit width.
	 * Currently MXC driver supports only 16 bit width.
	 */
	int bits_per_sample;

	/*!
	 * Semaphore used to control the access to this structure.
	 */
	struct semaphore sem;

	/*!
	 * This variable holds driver's state (busy/free)
	 */
	int busy;

	/*!
	 * This variable holds driver's state in terms of audio transfers
	 * (transferring/free)
	 */
	int transferring;

	/*!
	 * This variable holds the open mode requested by the user.
	 * Open mode can be write, read or write-read.
	 */
	int open_mode;

	/*!
	 * Frequency at which samples will be sent to mc13783.
	 */
	int sample_rate;

	/*!
	 * This variable holds the number of audio channels present
	 * in current audio stream (1 = MONO, 2 = STEREO)
	 */
	int num_channels;

	/*!
	 * This structure holds mc13783 current configuration.
	 */
	struct mc13783_device mc13783;

	/*!
	 * This variable holds the address of a non-cacheable buffer.
	 * This is needed to avoid memory corruption when doing DMA
	 * transfers when L2 cache is set to WB mode.
	 */
	dma_addr_t raw_buffer;

	/*!
	 * This variable holds the size of the non-cacheable buffer.
	 */
	int raw_count;

	/*!
	 * This variable tells whether if this device is acting as
	 * input device (recording) or output device (playback)
	 */
	int direction;
};

/*!
 * This structure is used to store pre-calculated values
 * to set ccm_clk and prescaler divider correctly.
 *
 * The calculation of these values are entirely based on
 * current USBPLL frequency. As this frequency is different
 * for each platform, the values are different for each supported
 * board.
 *
 * IMPORTANT: These structure is used only when AP side is the
 * provider of the audio clocks (BCL and FS) Normally this mode
 * is used only for testing purposes.
 *
 * Currently i300-30  EVB sets USBPLL to 288 MHZ, iMX31 sets USBPLL to
 * 96 Mhz and MXC 91231 sets USBPLL to 240 Mhz.
 *
 * For each supported sample rate, there is one structure like this one.
 *
 */
struct freq_coefficients {
	/*!
	 * This variable holds the sample rates supported by our driver.
	 * Supported sample rates are:
	 *
	 * - 8Khz
	 * - 11Khz
	 * - 16Khz
	 * - 22Khz
	 * - 24Khz
	 * - 32Khz
	 * - 44Khz
	 * - 48Khz
	 * - 96Khz
	 */
	int sample_rate;

	/*!
	 * This variable holds the divider to be applied to the ccm_ssi_clk
	 * clock. This divider is used to get a base frequency from which
	 * the desired sample rate will be calculated. In other words we
	 * apply the following division:
	 *
	 * USBPLL/ccm_div
	 *
	 * ccm_ssi_clk clock is currently set to 96Mhz on iMX31, 288 Mhz on
	 * i300-30EVB and 240 Mhz on MXC91231.
	 */
	int ccm_div;

	/*!
	 * This variable holds the prescaler divider to be applied to
	 * USBPLL/ccm_div result. Please note that this value is multiplied
	 * by 2 internally when SSI's STCCR register is set.
	 * (see set_audio_clocks and configure_ssi_tx functions)
	 */
	int prescaler;
};

 /**/ struct mixer_data {
	/*!
	 * ID of the mixer device. Assigned by OSS framework.
	 */
	int mixer_dev;

	/*!
	 * This variable holds the current active output device(s)
	 */
	int output_device;

	/*!
	 * This variable holds the current active input device.
	 */
	int input_device;

	/*!
	 * This variable holds the current volume for active ouput device(s)
	 */
	int output_volume;

	/*!
	 * This variable holds the current volume for active input device.
	 */
	int input_volume;

	/*!
	 * Semaphore used to control the access to this structure.
	 */
	struct semaphore sem;
};

/*!
 * This array holds the structures representing the two
 * devices (CODEC and STEREODAC) used by the MXC OSS Sound driver.
 */
static struct device_data *devices[] = { NULL, NULL };

/*!
 * This variable holds the state of the mxc OSS mixer device
 */
static struct mixer_data *mxc_mc13783_mixer;

/*!
 * Structure used to store precalculated values to get
 * correct Bit Clock and Frame Sync when playing sound
 * in MCU master mode.
 *
 * Remember that ccm_div field values have
 * been multiplied by 2 for FP accuracy (i.e if we
 * want divide the clock by 10, we must pass
 * value 20 to mxc_set_clocks_div function)
 *
 * MXC driver uses the following equations to calculate FS and BC clocks:
 *
 * bit_clock = ssi_clock / [(div2 + 1) x (7 x psr + 1) x (prescaler + 1) x 2]
 * where
 *
 * ssi_clock = SSIx_BAUD/ccm_divider (x in SSIx_BAUD can be 1 or 2)
 * ccm_divider = defined in struct freq_coefficients.
 * prescaler = defined in struct freq_coefficients.
 * div2 = 0.
 * psr = 0.
 *     
 * frame_sync_clock = (bit_clock) / [(frdc + 1) x word_length]
 * where
 * frdc = 1 for stereo, 0 for mono.
 * word_length = 16 (sample bits)
 *
 * Note: SSIx_BAUD is sourced by USBPLL.
 */
static struct freq_coefficients freq_values[SAMPLE_RATE_MAX] = {
	{8000, CCM_DIVIDER_8KHZ, PRESCALER_8KHZ},
	{11025, CCM_DIVIDER_11KHZ, PRESCALER_11KHZ},
	{16000, CCM_DIVIDER_16KHZ, PRESCALER_16KHZ},
	{22050, CCM_DIVIDER_22KHZ, PRESCALER_22KHZ},
	{24000, CCM_DIVIDER_24KHZ, PRESCALER_24KHZ},
	{32000, CCM_DIVIDER_32KHZ, PRESCALER_32KHZ},
	{44100, CCM_DIVIDER_44KHZ, PRESCALER_44KHZ},
	{48000, CCM_DIVIDER_48KHZ, PRESCALER_48KHZ},
	{96000, CCM_DIVIDER_96KHZ, PRESCALER_96KHZ},
};

/*!
 * This function returns the optimal values to
 * get a desired sample rate (among supported ones)
 * Values stored in freq_coefficients array are valid if
 * the following conditions are honored:
 *
 * 1 - DIV2 bit in SSI STCCR register (SSI Transmit and
 *     Receive Clock Control Registers) is set to 0.
 * 2 - PSR bit in SSI STCCR is set to 0.
 * 3 - DC bits in SSI STCCR are set to 0 for mono audio or
 *     1 for stereo audio.
 * 4 - WL bits in SSI STCCR are set to 7 (16 bit word length)
 *
 * @param       sample_rate:    sample rate requested.
 *
 * @return      This function returns a structure holding the correct
 *              values to set BC and FS clocks for this sample rate,
 *              NULL if an unsupported sample rate is requested.
 */
inline struct freq_coefficients *get_ccm_divider(int sample_rate)
{
	int i;

	for (i = 0; i < SAMPLE_RATE_MAX; i++) {
		if (freq_values[i].sample_rate == sample_rate) {
			return &freq_values[i];
		}
	}

	return NULL;
}

/*!
 * This function returns a pointer to the structure of
 * the audio device requested. Currently MXC OSS driver
 * supports two devices: CODEC (connected to SSI1) and
 * STEREODAC (connected to SSI2)
 *
 * @param       d       Index of device requested.
 *
 * @return              This function returns a pointer to the
 *                      structure representing the audio device.
 */
static inline struct device_data *get_device_data(int d)
{
	return (struct device_data *)audio_devs[d]->devc;
}

/*!
 * This function returns a pointer to the structure of
 * the MXC audio mixer device.
 *
 * @param       d       Index of device requested.
 *
 * @return              This function returns a pointer to the
 *                      structure representing the mixer device.
 */
static inline struct mixer_data *get_mixer_data(int d)
{
	return (struct mixer_data *)mixer_devs[d]->devc;
}

/*!
 * This function disables CODEC's amplifiers, volume and clock.
 */
inline void disable_codec(void)
{
	mc13783_audio_codec_enable(0);
	//mc13783_audio_output_pga(false, rs_codec);
	mc13783_audio_output_set_volume(rs_codec, 0x0);
	mc13783_audio_codec_clk_en(0);
}

/*!
 * This function disables STEREODAC's amplifiers, volume and clock.
 */
inline void disable_stereodac(void)
{
	mc13783_audio_st_dac_enable(0);
	mc13783_audio_output_pga(false, rs_st_dac);
	mc13783_audio_output_set_volume(rs_st_dac, 0x0);
	mc13783_audio_st_dac_clk_en(0);
}

/*!
 * This function get values used to set sample rate in mc13783 registers.
 * It receives an OSS value representing the sample rate.
 *
 * @param       device  device structure.
 */
inline void normalize_speed_for_mc13783(struct device_data *device)
{
	switch (device->sample_rate) {
	case 8000:
		device->mc13783.sample_rate = sr_8k;
		break;

	case 11025:
		device->mc13783.sample_rate = sr_11025;
		break;

	case 16000:
		device->mc13783.sample_rate = sr_16k;
		break;

	case 22050:
		device->mc13783.sample_rate = sr_22050;
		break;

	case 24000:
		device->mc13783.sample_rate = sr_24k;
		break;

	case 32000:
		device->mc13783.sample_rate = sr_32k;
		break;

	case 44100:
		device->mc13783.sample_rate = sr_44100;
		break;

	case 48000:
		device->mc13783.sample_rate = sr_48k;
		break;

	case 64000:
		device->mc13783.sample_rate = sr_64k;
		break;

	case 96000:
		device->mc13783.sample_rate = sr_96k;
		break;

	default:
		device->mc13783.sample_rate = sr_8k;
	}

}

/*!
 * This function normalizes speed given by the user
 * if speed is not supported, the function will
 * calculate the nearest one.
 *
 * @param       mc13783_dev   dev id of the mc13783 device being
 *                            used (codec or stereodac).
 * @param       speed         speed requested by the user.
 *
 * @return                    The normalized speed.
 */
inline int adapt_speed(int mc13783_dev, int speed)
{
	if (mc13783_dev == rs_codec) {
		if ((speed != 8000) && (speed != 16000)) {
			speed = 8000;
		}

		return speed;
	} else {
		/* speeds from 8k to 96k */
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
	}

	return speed;
}

/*!
 * This function sets the output device(s) in mc13783. It takes an
 * OSS value and modifies mc13783 registers using mc13783-specific values.
 *
 * @param       val     OSS value. Thsi value defines the output device(s)
 *                      mc13783 should activate for current audio playback
 *                      session.
 */
inline void set_output_device_for_mc13783(int val)
{
	if (val & SOUND_MASK_PHONEOUT) {
		mc13783_audio_output_set_conf(od_earpiece, os_mixer_source);
	} else {
		mc13783_audio_output_disable(od_earpiece);
	}

	if (val & SOUND_MASK_SPEAKER) {
		mc13783_audio_output_set_conf(od_handsfree, os_mixer_source);
	} else {
		mc13783_audio_output_disable(od_handsfree);
	}

	if (val & SOUND_MASK_VOLUME) {
		mc13783_audio_output_set_conf(od_headset, os_mixer_source);
	} else {
		mc13783_audio_output_disable(od_headset);
	}

	if (val & SOUND_MASK_PCM) {
		mc13783_audio_output_set_conf(od_line_out, os_mixer_source);
	} else {
		mc13783_audio_output_disable(od_line_out);
	}
}

/*!
 * This function sets the input device in mc13783. It takes an
 * OSS value and modifies mc13783 registers using mc13783-specific values.
 *
 * @param       val     OSS value. This value defines the input device that
 *                      mc13783 should activate to get audio signal (recording)
 */
inline void set_input_device_for_mc13783(int val)
{
	mc13783_audio_input_device(false, id_handset1);
	mc13783_audio_input_device(false, id_handset2);
	mc13783_audio_input_device(false, id_headset);
	mc13783_audio_input_device(false, id_line_in);

	switch (val) {
	case SOUND_MASK_PHONEIN:	/* J4 on mc13783 EVB board */
		mc13783_audio_input_device(true, id_handset1);
		mc13783_audio_input_device(true, id_handset2);
		break;

	case SOUND_MASK_MIC:	/* J3 on mc13783 EVB board */
		mc13783_audio_input_device(true, id_headset);
		break;

	case SOUND_MASK_LINE:	/* J7 on mc13783 EVB board */
		mc13783_audio_input_device(true, id_line_in);
		break;

	default:
		break;
	}
}

/*!
 * This function sets the mc13783 input device's volume.
 *
 * @param       val     Volume to be applied. This value can go
 *                      from 0 (mute) to 100 (max volume)
 */
inline void set_input_volume_for_mc13783(int val)
{
	int leftdb, rightdb;
	int left, right;

	left = (val & 0x00ff);
	right = ((val & 0xff00) >> 8);

	leftdb = (left * MC13783_INPUT_VOLUME_MAX) / INPUT_VOLUME_MAX;
	rightdb = (right * MC13783_INPUT_VOLUME_MAX) / INPUT_VOLUME_MAX;

	mc13783_audio_input_set_gain(leftdb, rightdb);
}

/*!
 * This function sets the mc13783 output device's volume.
 * @param       mc13783_dev   dev id of the mc13783 device being
 *                            used (codec or stereodac).
 *
 * @param       volume        Volume to be applied. This value can go
 *                            from 0 (mute) to 100 (max volume)
 */
void set_output_volume_for_mc13783(int mc13783_dev, int volume)
{
	int leftdb, rightdb;
	int right, left;

	left = (volume & 0x00ff);
	right = ((volume & 0xff00) >> 8);

	leftdb = (left * MC13783_OUTPUT_VOLUME_MAX) / OUTPUT_VOLUME_MAX;
	rightdb = (right * MC13783_OUTPUT_VOLUME_MAX) / OUTPUT_VOLUME_MAX;

	mc13783_audio_output_set_volume(mc13783_dev, rightdb);
}

/*!
 * This function sets the mode for the mono adder in mc13783.
 * The mono adder in the stereo channel can be used in four
 * different modes: stereo (right and left channel independent),
 * stereo opposite (left channel in opposite phase), mono (right
 * and left channel added), mono opposite (as mono but with
 * outputs in opposite phase)
 *
 * @param       dev     Index of the device to which this settings will apply.
 * @param       val     Adder mode.
 *
 * @return              The adder mode applied.
 */
int set_output_adder(int dev, int val)
{
	struct device_data *device = NULL;

	device = get_device_data(dev);
	down_interruptible(&device->sem);

	switch (val) {
	case MC13783_AUDIO_ADDER_STEREO:
		mc13783_audio_output_config_mixer(ac_stereo);
		break;

	case MC13783_AUDIO_ADDER_STEREO_OPPOSITE:
		mc13783_audio_output_config_mixer(ac_stereo_opposit);
		break;

	case MC13783_AUDIO_ADDER_MONO:
		mc13783_audio_output_config_mixer(ac_mono);
		break;

	case MC13783_AUDIO_ADDER_MONO_OPPOSITE:
		mc13783_audio_output_config_mixer(ac_mono_opposit);
		break;

	default:
		KERNEL_MSG(KERN_WARNING "Invalid parameter. Adder set \
                                to stereo opposite\n");
		mc13783_audio_output_config_mixer(ac_stereo_opposit);
		val = MC13783_AUDIO_ADDER_STEREO_OPPOSITE;
	}

	device->mc13783.adder_mode = val;
	up(&device->sem);

	return val;
}

/*!
 * This function gets the mode for the mono adder in mc13783.
 * The mono adder in the stereo channel can be used in four
 * different modes: stereo (right and left channel independent),
 * stereo opposite (left channel in opposite phase), mono (right
 * and left channel added), mono opposite (as mono but with
 * outputs in opposite phase)
 *
 * @param       dev     Index of the device from which we get this settings.
 *
 * @return              The current adder mode.
 */
int get_output_adder(int dev)
{
	struct device_data *device = NULL;
	int res = 0;

	device = get_device_data(dev);

	down_interruptible(&device->sem);
	res = device->mc13783.adder_mode;
	up(&device->sem);

	return res;
}

/*!
 * This function gets the digital filter(s) currently used in the CODEC.
 *
 * @param       dev     Index of the device (should be the CODEC) containing
 *                      the filter's information.
 *
 * @return              The current digital filter if current device is the
 *                      CODEC, -1 otherwise.
 */
int get_codec_filter(int dev)
{
	struct device_data *device = NULL;
	int res = -1;

	device = get_device_data(dev);
	down_interruptible(&device->sem);

	if (device->mc13783.device == rs_codec) {
		res = device->mc13783.codec_filter;
	}

	up(&device->sem);
	return res;
}

/*!
 * This function sets the digital filter(s) currently for the CODEC.
 *
 * @param       dev     Index of the device to which this settings will apply.
 * @param       mask    Value of filters to be set.
 *
 * @return              On success this function returns the applied digital
 *                      filter(s), -EINVAL otherwise.
 */
int set_codec_filter(int dev, int mask)
{
	struct device_data *device = NULL;
	int filter = 0;

	device = get_device_data(dev);
	down_interruptible(&device->sem);

	if (device->mc13783.device != rs_codec) {
		up(&device->sem);
		return -EINVAL;
	}

	if (mask & MC13783_CODEC_FILTER_HIGH_PASS_IN) {
		mc13783_audio_codec_receiv_high_pass_filter_en(1);
		device->mc13783.codec_filter |=
		    MC13783_CODEC_FILTER_HIGH_PASS_IN;
	} else {
		mc13783_audio_codec_receiv_high_pass_filter_en(0);
		device->mc13783.codec_filter &=
		    ~MC13783_CODEC_FILTER_HIGH_PASS_IN;
	}

	if (mask & MC13783_CODEC_FILTER_HIGH_PASS_OUT) {
		mc13783_audio_codec_trans_high_pass_filter_en(1);
		device->mc13783.codec_filter |=
		    MC13783_CODEC_FILTER_HIGH_PASS_OUT;
	} else {
		mc13783_audio_codec_trans_high_pass_filter_en(0);
		device->mc13783.codec_filter &=
		    ~MC13783_CODEC_FILTER_HIGH_PASS_OUT;
	}

	if (mask & MC13783_CODEC_FILTER_DITHERING) {
		mc13783_audio_codec_dithering_en(1);
		device->mc13783.codec_filter |= MC13783_CODEC_FILTER_DITHERING;
	} else {
		mc13783_audio_codec_dithering_en(0);
		device->mc13783.codec_filter &= ~MC13783_CODEC_FILTER_DITHERING;
	}

	if (mask & MC13783_CODEC_FILTER_DISABLE) {
		mc13783_audio_codec_trans_high_pass_filter_en(0);
		mc13783_audio_codec_receiv_high_pass_filter_en(0);
		mc13783_audio_codec_dithering_en(0);

		device->mc13783.codec_filter = MC13783_CODEC_FILTER_DISABLE;
	}

	filter = device->mc13783.codec_filter;
	up(&device->sem);

	return filter;
}

/*!
 * This function sets the output balance configuration.
 *
 * @param        dev    Index used to get device structure.
 * @param        val    0 to 49 = left balance.
 *                      50 = no balance at all.
 *                      51 to 100 = right balance.
 *                      On mc13783 board balance goes from -21dB to 0db.
 *                 
 * @return              the new output balance.
 */
int set_output_balance(int dev, int val)
{
	struct device_data *device = NULL;
	int channel = 0;
	int bal = 0;

	device = get_device_data(dev);
	down_interruptible(&device->sem);

	NORMALIZE_BALANCE(val);

	channel = (val >= NO_BALANCE) ? c_right : c_left;
	if (val > NO_BALANCE) {
		bal = ((val - NO_BALANCE) / MC13783_BALANCE_MAX);
	} else if (val < NO_BALANCE) {
		bal = abs((val / MC13783_BALANCE_MAX) - MC13783_BALANCE_MAX);
	} else {
		bal = 0;
	}

	DPRINTK("Setting balance : channel = %d, balance = %d\n", channel, bal);

	mc13783_audio_output_set_balance(channel, bal);
	device->mc13783.balance = val;

	up(&device->sem);
	return val;
}

/*!
 * This function gets the output balance.
 *
 * @param        dev    Index used to get device structure.
 *
 * @return       the current output balance.
 */
int get_output_balance(int dev)
{
	struct device_data *device = NULL;
	int balance;

	device = get_device_data(dev);
	down_interruptible(&device->sem);

	balance = device->mc13783.balance;

	up(&device->sem);
	return balance;
}

/*!
 * This function selects the output source(s) in mc13783
 * (earpiece, handsfree, headset and line_out)
 *
 * @param        dev    Index used to get mixer structure
 * @param        val    bitfield that encodes the selected output
 *                      sources. If zero is passed then all
 *                      output sources will be deselected.
 *                
 * @return              the selected output source(s).
 */
int set_output_device(int dev, int val)
{
	down_interruptible(&mxc_mc13783_mixer->sem);

	if ((val & MXC_OUTPUTS) == 0) {
		KERNEL_MSG("Output configuration not supported\n");
		up(&mxc_mc13783_mixer->sem);
		return -EINVAL;
	}

	if (val & SOUND_MASK_PHONEOUT) {
		mc13783_audio_output_set_conf(od_earpiece, os_mixer_source);
	} else {
		mc13783_audio_output_disable(od_earpiece);
	}

	if (val & SOUND_MASK_SPEAKER) {
		mc13783_audio_output_set_conf(od_handsfree, os_mixer_source);
	} else {
		mc13783_audio_output_disable(od_handsfree);
	}

	if (val & SOUND_MASK_VOLUME) {
		mc13783_audio_output_set_conf(od_headset, os_mixer_source);
	} else {
		mc13783_audio_output_disable(od_headset);
	}

	if (val & SOUND_MASK_PCM) {
		mc13783_audio_output_set_conf(od_line_out, os_mixer_source);
	} else {
		mc13783_audio_output_disable(od_line_out);
	}

	mxc_mc13783_mixer->output_device = val;
	DPRINTK("current device = %d\n", mxc_mc13783_mixer->output_device);

	up(&mxc_mc13783_mixer->sem);
	return val;
}

/*!
 * This function gets the output source(s) in mc13783.
 * (headset | handsfree | lineout | earpiece)
 *
 * @param        dev    Index used to get mixer structure.
 *                
 * @return              the current output source(s)
 */
int get_output_device(int dev)
{
	int val;

	down_interruptible(&mxc_mc13783_mixer->sem);

	DPRINTK("current device = %d\n", mxc_mc13783_mixer->output_device);

	val = mxc_mc13783_mixer->output_device;
	up(&mxc_mc13783_mixer->sem);

	return val;
}

/*!
 * This function selects the input source in mc13783.
 * (mic or line_in or handset)
 *
 * @param        dev    Index used to get mixer structure.
 * @param        val    bitfield that encodes the selected input
 *                      source. If zero is passed then all
 *                      input sources will be deselected.
 *                
 * @return              On success this function returns the selected
 *                      input source, -EINVAL otherwise.
 */
int set_input_device(int dev, int val)
{
	int ret = 0;

	down_interruptible(&mxc_mc13783_mixer->sem);

	if ((val & MXC_INPUTS) == 0) {
		KERNEL_MSG(KERN_WARNING "Input configuration not supported\n");
		ret = -EINVAL;
		goto out;
	}

	mc13783_audio_input_device(false, id_handset1);
	mc13783_audio_input_device(false, id_handset2);
	mc13783_audio_input_device(false, id_headset);
	mc13783_audio_input_device(false, id_line_in);

	switch (val) {
	case SOUND_MASK_PHONEIN:	/* J4 on mc13783 EVB board */
		mc13783_audio_input_device(true, id_handset1);
		mc13783_audio_input_device(true, id_handset2);
		break;

	case SOUND_MASK_MIC:	/* J3 on mc13783 EVB board */
		mc13783_audio_input_device(true, id_headset);
		break;

	case SOUND_MASK_LINE:	/* J7 on mc13783 EVB board */
		mc13783_audio_input_device(true, id_line_in);
		break;

	default:
		KERNEL_MSG(KERN_WARNING "Input configuration not supported\n");
		ret = -EINVAL;
		goto out;
	}

	mxc_mc13783_mixer->input_device = val;
	ret = val;

	DPRINTK("current device = %d\n", ret);
      out:
	up(&mxc_mc13783_mixer->sem);
	return ret;
}

/*!
 * This function gets the input source in mc13783.
 * (mic or line_in or handset)
 *
 * @param        dev    Index used to get mixer structure.
 *                
 * @return              the current input source.
 */
int get_input_device(int dev)
{
	int val = 0;

	down_interruptible(&mxc_mc13783_mixer->sem);

	DPRINTK("current device = %d\n", mxc_mc13783_mixer->input_device);

	if ((mxc_mc13783_mixer->input_device & MXC_INPUTS) == 0) {
		KERNEL_MSG(KERN_WARNING "Unknown input device found\n");
		up(&mxc_mc13783_mixer->sem);
		return -1;
	}

	val = mxc_mc13783_mixer->input_device;
	up(&mxc_mc13783_mixer->sem);
	return val;
}

/*!
 * This function gets the volume of a specified output device.
 *
 * @param        dev            Index used to get mixer structure.
 * @param        out_device     The output device.
 *                
 * @return                      the current volume of the output device.
 */
int get_output_volume(int dev, int out_device)
{
	int output_volume = 0;

	down_interruptible(&mxc_mc13783_mixer->sem);

	if ((out_device & mxc_mc13783_mixer->output_device) == 0) {
		KERNEL_MSG("Output device not found\n");
	}

	DPRINTK("current volume = %d\n", mxc_mc13783_mixer->output_volume);
	output_volume = mxc_mc13783_mixer->output_volume;

	up(&mxc_mc13783_mixer->sem);
	return output_volume;
}

/*!
 * This function sets the volume of a specified output device.
 *
 * @param        dev            Index used to get mixer structure.
 * @param        out_device     The output device.
 * @param        val            the volume to apply to the output device.
 *                
 * @return                      The new volume of the output device.
 */
int set_output_volume(int dev, int out_device, int val)
{
	int leftdb, rightdb;
	int right, left;
	int cur_dev;

	left = (val & 0x00ff);
	right = ((val & 0xff00) >> 8);

	NORMALIZE_OUTPUT_VOLUME(left);
	NORMALIZE_OUTPUT_VOLUME(right);

	leftdb = (left * MC13783_OUTPUT_VOLUME_MAX) / OUTPUT_VOLUME_MAX;
	rightdb = (right * MC13783_OUTPUT_VOLUME_MAX) / OUTPUT_VOLUME_MAX;

	down_interruptible(&mxc_mc13783_mixer->sem);

	cur_dev = mxc_mc13783_mixer->output_device;
	if ((cur_dev & out_device) == 0) {
		KERNEL_MSG(KERN_WARNING "Output device not found\n");
		up(&mxc_mc13783_mixer->sem);
		return -ENODEV;
	}

	mc13783_audio_output_set_volume(rs_st_dac, rightdb);
	mc13783_audio_output_set_volume(rs_codec, rightdb);

	/* In addition, if volume is zero, deactivate stereo amplifiers */
	if (right == 0) {
		mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0, BIT_AHSREN, 0);
		mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0, BIT_AHSLEN, 0);
	} else if (mxc_mc13783_mixer->output_volume == 0) {
		mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0, BIT_AHSREN, 1);
		mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0, BIT_AHSLEN, 1);
	}

	mxc_mc13783_mixer->output_volume = ((right << 8) | right);
	up(&mxc_mc13783_mixer->sem);

	return val;
}

/*!
 * This function gets the volume of a specified input device.
 *
 * @param        dev            Index used to get mixer structure.
 * @param        in_device      The input device.
 *                
 * @return                      the current volume of the input device.
 */
int get_input_volume(int dev, int in_device)
{
	int input_volume = 0;

	down_interruptible(&mxc_mc13783_mixer->sem);

	if (in_device != mxc_mc13783_mixer->input_device) {
		KERNEL_MSG(KERN_WARNING "Input device not found\n");
	}

	input_volume = mxc_mc13783_mixer->input_volume;

	up(&mxc_mc13783_mixer->sem);
	return input_volume;
}

/*!
 * This function sets the volume for current input device
 * (mic, handset or line in).
 *
 * @param        dev            Index used to get mixer structure.
 * @param        in_device      The input device.
 * @param        val            first byte contains volume for left channel, second
 *                              byte contains volume for right channel.
 *
 * @return                      the new volume for current input device. -ENODEV
 *                              otherwise.
 */
int set_input_volume(int dev, int in_device, int val)
{
	int leftdb, rightdb;
	int left, right;

	left = (val & 0x00ff);
	right = ((val & 0xff00) >> 8);

	NORMALIZE_INPUT_VOLUME(left);
	NORMALIZE_INPUT_VOLUME(right);

	leftdb = (left * MC13783_INPUT_VOLUME_MAX) / INPUT_VOLUME_MAX;
	rightdb = (right * MC13783_INPUT_VOLUME_MAX) / INPUT_VOLUME_MAX;

	down_interruptible(&mxc_mc13783_mixer->sem);

	DPRINTK("current device = %d\n", mxc_mc13783_mixer->input_device);

	if (mxc_mc13783_mixer->input_device != in_device) {
		KERNEL_MSG(KERN_WARNING "Input device not found\n");
		up(&mxc_mc13783_mixer->sem);
		return -ENODEV;
	}

	mc13783_audio_input_set_gain(leftdb, rightdb);
	mxc_mc13783_mixer->input_volume = ((right << 8) | left);

	up(&mxc_mc13783_mixer->sem);
	return val;
}

/*!
 * This function sets the clock provider. Two options
 * can be possible: mc13783 master and MCU master.
 * set_clock_source is called from SNDCTL_CLK_SET_MASTER
 * IOCTL.
 */
int set_audio_clock_master(int dev, int val)
{
	struct device_data *device = NULL;
	int mode = 0;

	device = get_device_data(dev);
	down_interruptible(&device->sem);

	if (device->transferring == 1) {
		return -EBUSY;
	}

	if (val == MXC_AUDIO_CLOCKING_MC13783_MASTER) {
		DPRINTK("mc13783 master mode\n");
		mode = ms_master;
	} else {
		DPRINTK("mc13783 slave mode\n");
		mode = ms_slave;
	}

	mc13783_audio_st_dac_master_slave(mode);
	device->mc13783.mode = mode;

	up(&device->sem);
	return 0;
}

/*!
 * This function configures the STEREODAC for playback/recording.
 *
 * main configured elements are:
 *      - audio path on mc13783
 *      - external clock to generate BC and FS clocks
 *      - mc13783 mode (master or slave)
 *      - protocol
 *      - sample rate
 *
 * @param       device          structure representing the current device.
 */
void configure_stereodac(struct device_data *device)
{
	struct mc13783_device *mc13783;
	int mc13783_clock_source = 0;

	DPRINTK("configuring stereodac\n");

	mc13783 = &device->mc13783;
	set_output_volume_for_mc13783(rs_st_dac,
				      mxc_mc13783_mixer->output_volume);
	mc13783_audio_st_dac_select_ssi((mc13783->ssi ==
					 SSI1) ? sb_ssi_0 : sb_ssi_1);

	mc13783_clock_source = (machine_is_mx31ads())? ci_cli_b : ci_cli_a;
	mc13783_audio_st_dac_select_cli(mc13783_clock_source);
	mc13783_audio_st_dac_master_slave(mc13783->mode);
	mc13783_audio_st_dac_bit_clock_invert(mc13783->bcl_inverted);
	mc13783_audio_st_dac_frame_sync_invert(mc13783->fs_inverted);
	mc13783_audio_st_dac_protocol(mc13783->protocol);
	mc13783_audio_st_dac_conf_pll(mc13783->pll_rate);
	mc13783_audio_st_dac_set_sample_rate(mc13783->sample_rate);

	DPRINTK("sample rate = %d\n", mc13783->sample_rate);

	mc13783_audio_st_dac_clk_en(1);
	mc13783_audio_output_pga(true, rs_st_dac);

	set_output_device_for_mc13783(mxc_mc13783_mixer->output_device);

	mc13783_audio_output_mixer_input(true, rs_st_dac);
	mc13783_audio_st_dac_enable(1);
}

/*!
 * This function configures the CODEC for playback/recording.
 *
 * main configured elements are:
 *      - audio path on mc13783
 *      - external clock to generate BC and FS clocks
 *      - mc13783 mode (master or slave)
 *      - protocol
 *      - sample rate
 *
 * @param       device          structure representing the current device.
 */
void configure_codec(struct device_data *device)
{
	struct mc13783_device *mc13783;
	int mc13783_clock_source = 0;

	DPRINTK("configuring codec\n");

	mc13783 = &device->mc13783;

	mc13783_audio_codec_select_ssi((mc13783->ssi == SSI1) ?
				       sb_ssi_0 : sb_ssi_1);

	mc13783_clock_source = (machine_is_mx31ads())? ci_cli_b : ci_cli_a;

	mc13783_audio_codec_select_cli(mc13783_clock_source);
	mc13783_audio_codec_master_slave(mc13783->mode);
	mc13783_audio_codec_bit_clock_invert(mc13783->bcl_inverted);
	mc13783_audio_codec_frame_sync_invert(mc13783->fs_inverted);
	mc13783_audio_codec_protocol(mc13783->protocol);

	mc13783_audio_codec_conf_pll(mc13783->pll_rate);
	mc13783_audio_codec_set_sample_rate(mc13783->sample_rate);

	DPRINTK("sample rate = %d\n", mc13783->sample_rate);

	mc13783_audio_codec_clk_en(1);

	if (device->direction == AUDIO_PLAYBACK) {
		mc13783_audio_output_pga(true, rs_codec);

		set_output_volume_for_mc13783(rs_codec,
					      mxc_mc13783_mixer->output_volume);
		set_output_device_for_mc13783(mxc_mc13783_mixer->output_device);

	} else {
		set_input_device_for_mc13783(mxc_mc13783_mixer->input_device);
		set_input_volume_for_mc13783(mxc_mc13783_mixer->input_volume);
	}
	mc13783_audio_output_mixer_input(true, rs_codec);

	mc13783_audio_codec_enable(1);
}

/*!
 * This is a callback which will be called
 * when a TX transfer finishes. The call occurs
 * in interrupt context.
 *
 * @param       arg     pointer to the structure of the
 *                      device making the transfer.
 */
static void sdma_tx_callback(void *arg)
{
	struct device_data *dev;
	int index;

	dev = (struct device_data *)arg;
	index = dev->device_index;

	dma_unmap_single(NULL, dev->raw_buffer, dev->raw_count, DMA_TO_DEVICE);
	DMAbuf_outputintr(index, 1);
}

/*!
 * This is a callback which will be called
 * when a RX transfer finishes. The call occurs
 * in interrupt context.
 *
 * @param       arg     pointer to the structure of the
 *                      device making the transfer.
 */
static void sdma_rx_callback(void *arg)
{
	struct device_data *dev;
	int index;

	dev = (struct device_data *)arg;
	index = dev->device_index;

	dma_unmap_single(NULL, dev->raw_buffer,
			 dev->raw_count, DMA_FROM_DEVICE);
	DMAbuf_inputintr(index);
}

/*!
 * This function configures the DMA channel used to transfer
 * audio from MCU to mc13783.
 *
 * @param       dev             pointer to the structure of the current device.
 * @param       callback        pointer to function that will be
 *                              called when a SDMA TX transfer finishes.
 *
 * @return                      Returns 0 on success, -1 otherwise.
 */
int configure_write_channel(struct device_data *dev, void (*callback) (void *))
{
	dma_channel_params params;
	int channel = 0;
	int result = 0;

	result = mxc_request_dma(&channel, "OSS TX SDMA");
	if (result < 0) {
		KERNEL_MSG(KERN_WARNING "Error: not dma channel available\n");
		return -1;
	}

	memset(&params, 0, sizeof(dma_channel_params));

	params.bd_number = 1;
	params.arg = dev;
	params.callback = callback;
	params.transfer_type = emi_2_per;
	params.watermark_level = SDMA_TXFIFO_WATERMARK;

	params.word_size = TRANSFER_16BIT;

	if (dev->ssi == SSI1) {
		DPRINTK("activating connection SSI1 - SDMA\n");
		params.per_address = SSI1_BASE_ADDR;
		params.event_id = DMA_REQ_SSI1_TX1;
		params.peripheral_type = SSI;
	} else {
		DPRINTK("activating connection SSI2 - SDMA\n");
		params.per_address = SSI2_BASE_ADDR;
		params.event_id = DMA_REQ_SSI2_TX1;
		params.peripheral_type = SSI_SP;
	}

	mxc_dma_setup_channel(channel, &params);
	dev->dma_wchannel = channel;

	return 0;
}

/*!
 * This function configures the DMA channel used to transfer
 * audio from mc13783 to MCU.
 *
 * @param       dev             pointer to the structure of the current device.
 * @param       callback        pointer to function that will be
 *                              called when a SDMA RX transfer finishes.
 *
 * @return                      Returns 0 on success, -1 otherwise.
 */
int configure_read_channel(struct device_data *dev, void (*callback) (void *))
{
	dma_channel_params params;
	int channel = 0;
	int result = 0;

	result = mxc_request_dma(&channel, "OSS RX SDMA");
	if (result < 0) {
		KERNEL_MSG(KERN_WARNING "Error: not dma channel available\n");
		return -1;
	}

	memset(&params, 0, sizeof(dma_channel_params));

	params.bd_number = 1;
	params.arg = dev;
	params.callback = callback;
	params.transfer_type = per_2_emi;
	params.watermark_level = SDMA_RXFIFO_WATERMARK;

	params.word_size = TRANSFER_16BIT;

	if (dev->ssi == SSI1) {
		DPRINTK("activating connection SSI1 - SDMA\n");
		params.per_address = (SSI1_BASE_ADDR + 0x8);
		params.event_id = DMA_REQ_SSI1_RX1;
		params.peripheral_type = SSI;
	} else {
		DPRINTK("activating connection SSI2 - SDMA\n");
		params.per_address = (SSI2_BASE_ADDR + 0x8);
		params.event_id = DMA_REQ_SSI2_RX1;
		params.peripheral_type = SSI_SP;
	}

	mxc_dma_setup_channel(channel, &params);
	dev->dma_rchannel = channel;

	return 0;
}

/*!
 * This function configures the DMA logic to do audio transfers.
 *
 * @param       dev             pointer to the structure of the current device.
 *
 * @return                      Returns 0 on success, -EIO otherwise.
 */
int configure_sdma(struct device_data *dev)
{
	int res = 0;

	if (dev->open_mode & OPEN_WRITE) {
		DPRINTK("activating SDMA TX\n");
		res = configure_write_channel(dev, sdma_tx_callback);
		if (res == -1) {
			return -EIO;
		}
	}

	if (dev->open_mode & OPEN_READ) {
		DPRINTK("activating SDMA RX\n");
		res = configure_read_channel(dev, sdma_rx_callback);
		if (res == -1) {
			return -EIO;
		}
	}

	return res;
}

/*!
 * This function applies precalculated values for
 * each supported audio frequency. This settings
 * are applied only if MCU master mode is selected.
 *
 * @param       device  pointer to the structure of the current device.
 */
void set_audio_clocks(struct device_data *device)
{
	struct freq_coefficients *ccm_div;
	unsigned long clock = 0;
	int direction;
	int ssi_clock;
	int ssi;

	ssi = device->ssi;
	direction = device->direction;

	clock = mxc_pll_clock(USBPLL);
	DPRINTK("USBPLL's clock frequency = %d\n", (int)clock);

	if (direction == AUDIO_PLAYBACK) {
		ssi_tx_frame_rate(ssi, 2);
	} else {
		ssi_rx_frame_rate(ssi, 1);
	}

	ccm_div = get_ccm_divider(device->sample_rate);
	ssi_clock = (ssi == SSI1) ? SSI1_BAUD : SSI2_BAUD;

	/*disable ssi clock */
	mxc_clks_disable(ssi_clock);

	/*set the clock divider for this audio session */
	mxc_set_clocks_div(ssi_clock, device->num_channels);

	/* enable clock */
	mxc_clks_enable(ssi_clock);

	/* Set prescaler value. Prescaler value is divided by
	 * 2 because ssi internally multiplies it by 2.
	 */
	if (direction == AUDIO_PLAYBACK) {
		ssi_tx_prescaler_modulus(ssi,
					 (unsigned char)(ccm_div->prescaler /
							 2));
	} else {
		ssi_rx_prescaler_modulus(ssi,
					 (unsigned char)(ccm_div->prescaler /
							 2));
	}

	DPRINTK("prescaler value = %d\n", ccm_div->prescaler);

	/* clocks are provided by MCU, thus we use "internally" mode */
	if (direction == AUDIO_PLAYBACK) {
		ssi_tx_frame_direction(ssi, ssi_tx_rx_internally);
		ssi_tx_clock_direction(ssi, ssi_tx_rx_internally);
	} else {
		ssi_rx_frame_direction(ssi, ssi_tx_rx_internally);
		ssi_rx_clock_direction(ssi, ssi_tx_rx_internally);
	}

	clock = mxc_get_clocks(ssi_clock);
	DPRINTK("Current SSI clock frequency = %d\n", (int)clock);
}

/*!
 * This function configures the SSI in order to receive audio
 * from mc13783 (recording). Configuration of SSI consists mainly in
 * setting the following:
 *
 * 1) SSI to use (SSI1 or SSI2)
 * 2) SSI mode (normal or network. We use always network mode)
 * 3) SSI STCCR register settings, which control the sample rate (BCL and
 *    FS clocks)
 * 4) Watermarks for SSI FIFOs as well as timeslots to be used.
 * 5) Enable SSI.
 */
void configure_ssi_rx(struct device_data *device)
{
	int ssi;

	ssi = device->ssi;

	ssi_enable(ssi, false);
	ssi_synchronous_mode(ssi, true);
	ssi_network_mode(ssi, true);

	ssi_rx_early_frame_sync(ssi, ssi_frame_sync_one_bit_before);
	ssi_rx_frame_sync_length(ssi, ssi_frame_sync_one_bit);
	ssi_rx_fifo_enable(ssi, device->ssi_fifo, true);
	ssi_rx_bit0(ssi, true);

	ssi_rx_fifo_full_watermark(ssi, device->ssi_fifo, RX_WATERMARK);

	/* We never use the divider by 2 implemented in SSI */
	ssi_rx_clock_divide_by_two(ssi, 0);

	/* Set prescaler range (a fixed divide-by-eight prescaler
	 * in series with the variable prescaler) to 0 as we don't
	 * need it.
	 */
	ssi_rx_clock_prescaler(ssi, 0);

	/* Currently, only supported sample length is 16 bits */
	ssi_rx_word_length(ssi, ssi_16_bits);

	if (device->mc13783.mode == ms_master) {
		/* set direction of clocks ("externally" means that clocks come
		 * from mc13783 to MCU)
		 */
		ssi_rx_frame_direction(ssi, ssi_tx_rx_externally);
		ssi_rx_clock_direction(ssi, ssi_tx_rx_externally);

		/* Frame Rate Divider Control.
		 * In Normal mode, this ratio determines the word
		 * transfer rate. In Network mode, this ration sets
		 * the number of words per frame.
		 */
		ssi_rx_frame_rate(ssi, 2);
	 /*OJO*/} else {
		/* set BCL and FS clocks if MCU master mode */
		set_audio_clocks(device);
	}

	ssi_enable(ssi, true);
}

/*!
 * This function configures the SSI in order to send audio
 * to mc13783 (playback). Configuration of SSI consists mainly in
 * setting the following:
 *
 * 1) SSI to use (SSI1 or SSI2)
 * 2) SSI mode (normal or network. We use always network mode)
 * 3) SSI STCCR register settings, which control the sample rate (BCL and
 *    FS clocks)
 * 4) Watermarks for SSI FIFOs as well as timeslots to be used.
 * 5) Enable SSI.
 */
void configure_ssi_tx(struct device_data *device)
{
	int ssi;

	ssi = device->ssi;

	ssi_enable(ssi, false);
	ssi_synchronous_mode(ssi, true);

	/* OJO */
	if (device->num_channels == 1) {
		ssi_network_mode(ssi, false);
	} else {
		ssi_network_mode(ssi, true);
	}

	ssi_tx_early_frame_sync(ssi, ssi_frame_sync_one_bit_before);
	ssi_tx_frame_sync_length(ssi, ssi_frame_sync_one_bit);
	ssi_tx_fifo_enable(ssi, device->ssi_fifo, true);
	ssi_tx_bit0(ssi, true);

	ssi_tx_fifo_empty_watermark(ssi, device->ssi_fifo, TX_WATERMARK);

	/* We never use the divider by 2 implemented in SSI */
	ssi_tx_clock_divide_by_two(ssi, 0);

	/* Set prescaler range (a fixed divide-by-eight prescaler
	 * in series with the variable prescaler) to 0. Later It can
	 * be configured if MCU is the clock provider
	 */
	ssi_tx_clock_prescaler(ssi, 0);

	/*Currently, only supported sample length is 16 bits */
	ssi_tx_word_length(ssi, ssi_16_bits);

	if (device->mc13783.mode == ms_master) {
		/* mc13783 is the provider of clocks */
		ssi_tx_frame_direction(ssi, ssi_tx_rx_externally);
		ssi_tx_clock_direction(ssi, ssi_tx_rx_externally);

		/* Frame Rate Divider Control.
		 * In Normal mode, this ratio determines the word
		 * transfer rate. In Network mode, this ration sets
		 * the number of words per frame. OJO
		 */
		if (device->num_channels == 1) {
			ssi_tx_frame_rate(ssi, 1);
		} else {
			ssi_tx_frame_rate(ssi, 2);
		}
	} else {
		/*set BCL and FS clocks if MCU master mode */
		set_audio_clocks(device);
	}

	ssi_enable(ssi, true);
}

/*!
 * This function configures mc13783 IC for playing back.
 *
 * @param       device  pointer to the structure of the current device.
 *
 * @return              0 on success, -1 otherwise.
 */
int configure_mc13783_playback(struct device_data *device)
{
	device->mc13783.ssi = device->ssi;

	mc13783_audio_codec_set_latching_edge(true);

	if (device->mc13783.device == rs_st_dac) {
		mc13783_audio_st_dac_network_nb_of_timeslots(TIMESLOTS_2);
		 /*OJO*/ mc13783_audio_st_dac_conf_network_mode(t_mode_0);
		mc13783_audio_st_dac_conf_network_secondary(t_mode_1,
							    mm_no_mixing);
		mc13783_audio_st_dac_output_mixing_gain(mm_att_0dB);
		configure_stereodac(device);

	} else {
		if (((device->sample_rate != 16000) &&
		     (device->sample_rate != 8000)) ||
		    (device->num_channels == 2)) {
			KERNEL_MSG(KERN_WARNING "sample rate not supported \
                                        by CODEC\n");
			return -1;
		}

		mc13783_audio_codec_conf_network_mode(t_mode_0);
		mc13783_audio_codec_conf_network_secondary(t_mode_1,
							   t_mode_2,
							   mm_no_mixing);
		mc13783_audio_codec_output_mixing_gain(mm_no_mixing);
		configure_codec(device);
	}

	return 0;
}

/*!
 * This function configures mc13783 IC for recording.
 *
 * @param       device  pointer to the structure of the current device.
 *
 * @return              0 on success, -1 otherwise.
 */
int configure_mc13783_recording(struct device_data *device)
{
	device->mc13783.ssi = device->ssi;

	if (((device->sample_rate != 16000) && (device->sample_rate != 8000))) {
		KERNEL_MSG(KERN_WARNING "sample rate not supported by CODEC\n");
		return -1;
	}

	/*       
	   mc13783_audio_codec_set_latching_edge(true);
	   mc13783_audio_codec_conf_network_mode(t_mode_0);
	   mc13783_audio_codec_conf_network_secondary(t_mode_1,
	   t_mode_2, mm_no_mixing);
	   mc13783_audio_codec_output_mixing_gain(mm_no_mixing);
	 */

	configure_codec(device);
	return 0;
}

/*!
 * This function configures audio multiplexer to support
 * audio mixing in mc13783 master mode.
 *
 * @param       device  pointer to the structure of the current device.
 */
void configure_dam_mc13783_master_mix(struct device_data *device)
{
	dam_reset_register(port_1);
	dam_reset_register(port_2);
	dam_reset_register(port_4);
	dam_reset_register(port_5);

	dam_select_mode(port_1, normal_mode);
	dam_select_mode(port_2, normal_mode);
	dam_select_mode(port_4, internal_network_mode);

	dam_set_synchronous(port_1, true);
	dam_set_synchronous(port_2, true);
	dam_set_synchronous(port_4, true);

	dam_select_RxD_source(port_1, port_4);
	dam_select_RxD_source(port_2, port_4);
	dam_select_RxD_source(port_4, port_1);

	dam_select_TxFS_direction(port_2, signal_out);
	dam_select_TxFS_source(port_2, false, port_4);

	dam_select_TxClk_direction(port_2, signal_out);
	dam_select_TxClk_source(port_2, false, port_4);

	/*added from mc13783 master mode */
	dam_select_RxFS_direction(port_2, signal_out);
	dam_select_RxFS_source(port_2, false, port_4);

	/*added from mc13783 master mode */
	dam_select_RxClk_direction(port_2, signal_out);
	dam_select_RxClk_source(port_2, false, port_4);

	dam_select_TxFS_direction(port_1, signal_out);
	dam_select_TxFS_source(port_1, false, port_4);

	dam_select_TxClk_direction(port_1, signal_out);
	dam_select_TxClk_source(port_1, false, port_4);

	dam_select_RxFS_direction(port_1, signal_out);
	dam_select_RxFS_source(port_1, false, port_4);

	/*added from mc13783 master mode */
	dam_select_RxClk_direction(port_1, signal_out);
	dam_select_RxClk_source(port_1, false, port_4);

	dam_set_internal_network_mode_mask(port_4, 0xfc);
}

/*!
 * This function configures audio multiplexer to support
 * recording/playback in mc13783 master mode.
 *
 * @param       device  pointer to the structure of the current device.
 */
void configure_dam_mc13783_master(struct device_data *device)
{
	int source_port;
	int target_port;

	DPRINTK("Set MC13783 as clock provider\n");

	if (device->ssi == SSI1) {
		DPRINTK("SSI1. source port = 1, target port = 4\n");
		source_port = port_1;
		target_port = port_4;
	} else {
		DPRINTK("SSI2. source port = 2, target port = 5\n");
		source_port = port_2;
		target_port = port_5;
	}

	dam_reset_register(source_port);
	dam_reset_register(target_port);

	dam_select_mode(source_port, normal_mode);
	/* dam_select_mode(target_port, normal_mode); */
	dam_select_mode(target_port, internal_network_mode);

	dam_set_synchronous(source_port, true);
	dam_set_synchronous(target_port, true);

	dam_select_RxD_source(source_port, target_port);
	dam_select_RxD_source(target_port, source_port);

	dam_select_TxFS_direction(source_port, signal_out);
	dam_select_TxFS_source(source_port, false, target_port);

	dam_select_TxClk_direction(source_port, signal_out);
	dam_select_TxClk_source(source_port, false, target_port);

	dam_select_RxFS_direction(source_port, signal_out);
	dam_select_RxFS_source(source_port, false, target_port);

	dam_select_RxClk_direction(source_port, signal_out);
	dam_select_RxClk_source(source_port, false, target_port);

	dam_set_internal_network_mode_mask(target_port, 0xfc);

	writel(0x0031010, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x38);
}

/*!
 * This function configures audio multiplexer to support
 * recording/playback in MCU master mode.
 *
 * @param       device  pointer to the structure of the current device.
 */
void configure_dam_mcu_master(struct device_data *device)
{
	int internal_port;
	int external_port;

	DPRINTK("Set MCU as clock provider\n");

	if (device->ssi == SSI1) {
		DPRINTK("SSI1. source port = 1, target port = 4\n");
		internal_port = port_1;
		external_port = port_4;
	} else {
		DPRINTK("SSI2. source port = 2, target port = 5\n");
		internal_port = port_2;
		external_port = port_5;
	}

	dam_reset_register(internal_port);
	dam_reset_register(external_port);

	dam_select_mode(internal_port, normal_mode);
	dam_select_mode(external_port, normal_mode);

	dam_set_synchronous(internal_port, true);
	dam_set_synchronous(external_port, true);

	dam_select_RxD_source(external_port, internal_port);
	dam_select_RxD_source(internal_port, external_port);

	dam_select_TxFS_direction(external_port, signal_out);
	dam_select_TxFS_source(external_port, false, internal_port);

	if (machine_is_mxc27530evb()) {
		DPRINTK("Applying soft config for MXC91231"
			" hadware workaround\n");

		if (device->ssi == SSI1) {
			dam_reset_register(port_7);
			dam_select_mode(port_7, normal_mode);
			dam_set_synchronous(port_7, false);
			dam_select_RxClk_direction(port_7, signal_out);
			dam_select_RxClk_source(port_7, false, internal_port);
		} else {
			dam_set_synchronous(port_5, false);
			dam_select_RxClk_source(port_5, false, port_2);
			dam_select_RxClk_direction(port_5, signal_out);
		}
	} else {
		dam_select_TxClk_direction(external_port, signal_out);
		dam_select_TxClk_source(external_port, false, internal_port);
	}
}

/*!
 * This function opens an mc13783 audio device (STEREODAC for /dev/sound/dsp
 * CODEC for /dev/sound/dsp1)
 *
 * @param       dev     the device's major number.
 * @param       mode    The parameter flag is one of OPEN_READ, OPEN_WRITE or
 *                      OPEN_READWRITE.
 *
 * @return              0 if success.
 *                      EBUSY if the driver instance already in use.
 *                      EACCES if requesting OPEN_READ on STEREODAC.
 */
static int mxc_audio_mc13783_open(int dev, int mode)
{
	struct device_data *device = NULL;
	int res;

	device = get_device_data(dev);
	down_interruptible(&device->sem);

	if (device->busy == 1) {
		up(&device->sem);
		return -EBUSY;
	}

	if ((device->ssi == SSI2) && (mode & OPEN_READ)) {
		up(&device->sem);
		KERNEL_MSG(KERN_WARNING "Invalid operation: recording sound \
                                using /dev/sound/dsp1 (STEREODAC)");
		return -EACCES;
	}

	device->open_mode = mode;
	res = configure_sdma(device);
	mc13783_audio_output_bias_conf(1, 1);
	device->busy = 1;

	audio_devs[dev]->go = 1;
	up(&device->sem);
	msleep(10);

	return res;
}

/*!
 * This function closes an mc13783 audio device (STEREODAC for /dev/sound/dsp1
 * CODEC for /dev/sound/dsp)
 *
 * @param       dev     the device's major number.
 */
static void mxc_audio_mc13783_close(int dev)
{
	struct device_data *device = NULL;

	device = get_device_data(dev);

	if (device->open_mode & OPEN_READ) {
		mxc_free_dma(device->dma_rchannel);
		device->dma_rchannel = -1;
	}

	if (device->open_mode & OPEN_WRITE) {
		mxc_free_dma(device->dma_wchannel);
		device->dma_wchannel = -1;
	}

	device->busy = 0;
}

/*!
 * This function sets/gets the current word length. MXC OSS driver
 * only supports 16 bits format.
 *
 * @param       dev     Device's index.
 * @param       bits    new bit width to apply.
 *                     
 * @return              This function returns always 16 bit width value.
 */
static unsigned int mxc_audio_mc13783_set_bits(int dev, unsigned int bits)
{
	return AFMT_S16_LE;
}

/*!
 * This function configures number of channels for next audio operation
 * (recording/playback) Number of channels define if sound is stereo
 * or mono.
 *
 * @param       dev             index used to get a pointer to the current
 *                              device's structure.
 * @param       channels        number of channels. If this argument is 0,
 *                              then function returns the current number of
 *                              channels.
 *
 * @return                      On success this function returns the number
 *                              of channels, -EBUSY if device is busy.
 */
static signed short mxc_audio_mc13783_set_channels(int dev,
						   signed short channels)
{
	struct device_data *device = NULL;

	device = get_device_data(dev);

	if (channels == 0) {
		channels = device->num_channels;
	} else {
		NORMALIZE_NUM_CHANNEL(channels);
		device->num_channels = channels;

		if (device->num_channels == 2) {	/* OJO */
			ssi_tx_mask_time_slot(device->ssi, 0xfffffffc);
			ssi_rx_mask_time_slot(device->ssi, 0xfffffffc);
		} else {
			ssi_tx_mask_time_slot(device->ssi, 0xfffffffe);
			ssi_rx_mask_time_slot(device->ssi, 0xfffffffe);
		}
	}

	DPRINTK("inside set_channels function. Channel = %d\n", channels);
	return channels;
}

/*!
 * This function configures the sample rate for next audio operation
 * (recording/playback) Sample rate can go from 8Khz to 48 khz.
 * (see device_data struct comments) This function is called by
 * OSS framework as well as by user-space apps (ioctl)
 *
 * @param       dev             index used to get a pointer to the current
 *                              device's structure.
 * @param       speed           sample rate to apply. If this argument is 0,
 *                              then function returns the current sample rate.
 *
 * @return                      On success this function returns the sample rate,
 *                              -EBUSY if device is busy.
 */
static int mxc_audio_mc13783_set_speed(int dev, int speed)
{
	struct device_data *device = NULL;

	device = get_device_data(dev);

	if (speed != 0) {
		speed = adapt_speed(device->mc13783.device, speed);

		device->sample_rate = speed;
		normalize_speed_for_mc13783(device);

		DPRINTK("sample rate set: %d\n", speed);
	} else {
		speed = device->sample_rate;
	}

	return speed;
}

/*!
 * This function triggers an audio transfer. As triggering is
 * done in prepare_for_output or prepare_input function,
 * this function does nothing.
 */
static void mxc_audio_mc13783_trigger(int dev, int state)
{
	/* nothing to be done here */
}

/*!
 * This function configures the hardware to allow audio
 * recording operations. It is called by OSS framework.
 *
 * @param       dev             Index used to get a pointer to the current
 *                              device's structure.
 * @param       bsize           Size of the buffer that will be used to store
 *                              audio samples coming from mc13783.
 * @param       bcount          This parameter is not used.
 *
 * @return                      On success this function returns 0, -EINVAL
 *                              if open mode is not OPEN_READ.
 */
static int mxc_audio_mc13783_prepare_input(int dev, int bsize, int bcount)
{
	struct device_data *device = NULL;

	DPRINTK("Hardware specific configuration\n");

	device = get_device_data(dev);
	down_interruptible(&device->sem);

	if (device->mc13783.mode == ms_master) {
		DPRINTK("mc13783 configuration for mc13783 master mode\n");
		configure_dam_mc13783_master(device);
	} else {
		DPRINTK("mc13783 configuration for mc13783 slave mode\n");
		configure_dam_mcu_master(device);
	}

	if ((device->open_mode & OPEN_READ) == 0) {
		up(&device->sem);
		return -EINVAL;
	}

	DPRINTK("ssi configuration for RX mode\n");

	device->direction = AUDIO_RECORDING;
	configure_ssi_rx(device);
	ssi_interrupt_enable(device->ssi, ssi_rx_dma_interrupt_enable);

	if (configure_mc13783_recording(device) == -1) {
		up(&device->sem);
		return -EAGAIN;
	}

	ssi_interrupt_enable(device->ssi, ssi_rx_fifo_0_full);
	ssi_receive_enable(device->ssi, true);

	up(&device->sem);
	device->transferring = 1;

	DPRINTK("Hardware specific configuration finished\n");
	return 0;
}

/*!
 * This function configures the hardware to allow audio
 * playback operations. It is called by OSS framework.
 *
 * @param       dev             Index used to get a pointer to the current
 *                              device's structure.
 * @param       bsize           Size of the buffer that will be used to store
 *                              audio samples coming from mc13783.
 * @param       bcount          Unused parameter.
 *
 * @return                      On success this function returns 0, -EINVAL
 *                              if open mode is not OPEN_READ, -EAGAIN if
 *                              mc13783 configuration failed.
 */
static int mxc_audio_mc13783_prepare_output(int dev, int bsize, int bcount)
{
	struct device_data *device = NULL;

	DPRINTK("Hardware specific configuration\n");

	device = get_device_data(dev);

	if (device->mc13783.mode == ms_master) {
		DPRINTK("mc13783 configuration for mc13783 master mode\n");
		configure_dam_mc13783_master(device);
	} else {
		DPRINTK("mc13783 configuration for mc13783 slave mode\n");
		configure_dam_mcu_master(device);
	}

	if ((device->open_mode & OPEN_WRITE) == 0) {
		return -EINVAL;
	}

	mc13783_audio_set_phantom_ground(0);

	DPRINTK("ssi configuration for TX mode\n");

	device->direction = AUDIO_PLAYBACK;
	configure_ssi_tx(device);
	ssi_interrupt_enable(device->ssi, ssi_tx_dma_interrupt_enable);

	if (configure_mc13783_playback(device) == -1) {
		return -EAGAIN;
	}

	ssi_interrupt_enable(device->ssi, ssi_tx_fifo_0_empty);
	ssi_transmit_enable(device->ssi, true);

	device->transferring = 1;

	DPRINTK("Hardware specific configuration finished\n");
	return 0;
}

/*!
 * This function is called whenever a new audio block needs to be
 * transferred to mc13783. The function receives the address and the size
 * of the new block and start a new DMA transfer.
 *
 * OSS framework calls this function.
 *
 * @param       dev             Index used to get a pointer to the current
 *                              device's structure.
 * @param       buf             Address of the buffer containing the audio
 *                              samples. This parameter is not used.
 * @param       count           Size of buffer (in bytes)
 * @param       it_flag         Unused parameter.
 */
static void mxc_audio_mc13783_output_block(int dev, unsigned long buf,
					   int count, int it_flag)
{
	struct device_data *device = NULL;
	dma_request_t sdma_request;
	struct dma_buffparms *dmap;

	device = get_device_data(dev);

	dmap = audio_devs[dev]->dmap_out;
	dmap->flags |= DMA_NOTIMEOUT | DMA_NODMA;

	memset(&sdma_request, 0, sizeof(dma_request_t));

	device->raw_count = count;
	device->raw_buffer = dma_map_single(NULL,
					    (dmap->raw_buf +
					     dmap->qhead * dmap->fragment_size),
					    count, DMA_TO_DEVICE);

	sdma_request.sourceAddr = (char *)device->raw_buffer;
	sdma_request.count = count;
	mxc_dma_set_config(device->dma_wchannel, &sdma_request, 0);
	mxc_dma_start(device->dma_wchannel);
}

/*!
 * This function is called whenever a new audio block needs to be
 * transferred from mc13783. The function receives the address and the size
 * of the block that will store the audio samples and start a new DMA transfer.
 *
 * OSS framework calls this function.
 *
 * @param       dev             Index used to get a pointer to the current
 *                              device's structure.
 * @param       buf             Address of the buffer containing the audio
 *                              samples. This parameter is not used.
 * @param       count           Size of buffer (in bytes)
 * @param       it_flag         Unused parameter.
 */
static void mxc_audio_mc13783_start_input(int dev, unsigned long buf,
					  int count, int it_flag)
{
	struct device_data *device = NULL;
	dma_request_t sdma_request;
	struct dma_buffparms *dmap;

	device = get_device_data(dev);

	dmap = audio_devs[dev]->dmap_in;
	dmap->flags |= DMA_NOTIMEOUT | DMA_NODMA;

	memset(&sdma_request, 0, sizeof(dma_request_t));

	device->raw_count = count;
	device->raw_buffer = dma_map_single(NULL,
					    (dmap->raw_buf +
					     dmap->qtail * dmap->fragment_size),
					    count, DMA_FROM_DEVICE);

	sdma_request.destAddr = (char *)device->raw_buffer;
	sdma_request.count = count;

	mxc_dma_set_config(device->dma_rchannel, &sdma_request, 0);
	mxc_dma_start(device->dma_rchannel);
}

/*!
 * This function resets the audio hardware, once audio transfers are finished.
 * It is called by OSS framework.
 *
 * @param       dev          Index used to get a pointer to the current
 *                           device's structure.
 */
static void mxc_audio_mc13783_halt_io(int dev)
{
	struct device_data *device = NULL;

	DPRINTK("Reset hardware\n");

	device = get_device_data(dev);

	if (device->mc13783.device == rs_st_dac) {
		disable_stereodac();
	} else {
		if (device->direction == AUDIO_PLAYBACK) {
			mc13783_audio_output_pga(false, rs_codec);
		}

		disable_codec();
	}

	if (device->direction == AUDIO_PLAYBACK) {
		ssi_transmit_enable(device->ssi, false);
		ssi_interrupt_disable(device->ssi, ssi_tx_dma_interrupt_enable);
		ssi_tx_fifo_enable(device->ssi, ssi_fifo_0, false);
	} else {
		ssi_receive_enable(device->ssi, false);
		ssi_interrupt_disable(device->ssi, ssi_rx_dma_interrupt_enable);
		ssi_rx_fifo_enable(device->ssi, ssi_fifo_0, false);
	}

	ssi_enable(device->ssi, false);
	device->transferring = 0;
}

/*!
 * This function handles the user commands to the mixer driver
 *
 * @param    dev      the device's major number
 * @param    cmd      a symbol that describes the command to apply
 * @param    arg      the command argument
 *
 * @return            0 if success, else error
 */
static int mxc_audio_mc13783_mixer_ioctl(int dev, unsigned int cmd,
					 void __user * arg)
{
	int val = 0;
	int ret = 0;

	switch (cmd) {
	case SOUND_MIXER_READ_DEVMASK:
		DPRINTK("SOUND_MIXER_READ_DEVMASK ioctl called\n");
		val = MXC_STEREO_OUTPUT | MXC_MONO_OUTPUT |
		    MXC_STEREO_INPUT | MXC_MONO_INPUT;
		break;

	case SOUND_MIXER_READ_OUTMASK:
		DPRINTK("SOUND_MIXER_READ_OUTMASK ioctl called\n");
		val = MXC_STEREO_OUTPUT | MXC_MONO_OUTPUT;
		break;

	case SOUND_MIXER_READ_RECMASK:
		DPRINTK("SOUND_MIXER_READ_RECMASK ioctl called\n");
		val = MXC_STEREO_INPUT | MXC_MONO_INPUT;
		break;

	case SOUND_MIXER_READ_OUTSRC:
		DPRINTK("SOUND_MIXER_READ_OUTSRC ioctl called\n");
		val = get_output_device(dev);
		break;

	case SOUND_MIXER_WRITE_OUTSRC:
		DPRINTK("SOUND_MIXER_WRITE_OUTSRC ioctl called\n");
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		if (val & (MXC_STEREO_OUTPUT | MXC_MONO_OUTPUT)) {
			ret = set_output_device(dev, val);
		} else {
			return -EINVAL;
		}

		if (ret > 0) {
			ret = 0;
		}

		break;

	case SOUND_MIXER_READ_RECSRC:
		DPRINTK("SOUND_MIXER_READ_RECSRC ioctl called\n");
		val = get_input_device(dev);
		break;

	case SOUND_MIXER_WRITE_RECSRC:
		DPRINTK("SOUND_MIXER_WRITE_RECSRC ioctl called\n");
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		if ((val == 0) || (val & (MXC_STEREO_INPUT | MXC_MONO_INPUT))) {
			ret = set_input_device(dev, val);
		} else {
			KERNEL_MSG(KERN_WARNING "invalid ioctl value for  \
                                        SOUND_MIXER_WRITE_RECSRC\n");
			return -EINVAL;
		}

		if (ret >= 0) {
			ret = 0;
		}

		break;

	case SOUND_MIXER_READ_STEREODEVS:
		val = MXC_STEREO_OUTPUT | MXC_STEREO_INPUT;
		break;

	case SOUND_MIXER_READ_CAPS:
		val = SOUND_CAP_EXCL_INPUT;
		break;

	case SOUND_MIXER_READ_VOLUME:	/* headset */
		val = get_output_volume(dev, SOUND_MASK_VOLUME);
		break;

	case SOUND_MIXER_READ_PCM:	/* stereo line out */
		val = get_output_volume(dev, SOUND_MASK_PCM);
		break;

	case SOUND_MIXER_READ_SPEAKER:	/* handsfree */
		val = get_output_volume(dev, SOUND_MASK_SPEAKER);
		break;

	case SOUND_MIXER_READ_PHONEOUT:	/* earpiece */
		val = get_output_volume(dev, SOUND_MASK_PHONEOUT);
		break;

	case SOUND_MIXER_WRITE_VOLUME:	/* headset */
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		ret = set_output_volume(dev, SOUND_MASK_VOLUME, val);
		if (ret >= 0) {
			ret = 0;
		}

		break;

	case SOUND_MIXER_WRITE_PCM:	/* stereo line out */
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		ret = set_output_volume(dev, SOUND_MASK_PCM, val);
		if (ret >= 0) {
			ret = 0;
		}

		break;

	case SOUND_MIXER_WRITE_SPEAKER:	/* handset */
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		ret = set_output_volume(dev, SOUND_MASK_SPEAKER, val);
		if (ret >= 0) {
			ret = 0;
		}

		break;

	case SOUND_MIXER_WRITE_PHONEOUT:	/* earpiece */
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		ret = set_output_volume(dev, SOUND_MASK_PHONEOUT, val);
		if (ret >= 0) {
			ret = 0;
		}

		break;

	case SOUND_MIXER_READ_MIC:	/* headset */
		val = get_input_volume(dev, SOUND_MASK_MIC);
		break;

	case SOUND_MIXER_READ_PHONEIN:	/* handset (mono or stereo) */
		val = get_input_volume(dev, SOUND_MASK_PHONEIN);
		break;

	case SOUND_MIXER_READ_LINE:	/* line input */
		val = get_input_volume(dev, SOUND_MASK_LINE);
		break;

	case SOUND_MIXER_WRITE_MIC:	/* headset */
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		ret = set_input_volume(dev, SOUND_MASK_MIC, val);
		if (ret >= 0) {
			ret = 0;
		}

		DPRINTK("SOUND_MIXER_WRITE_MIC value = %d\n", val);

		break;

	case SOUND_MIXER_WRITE_PHONEIN:	/* handset (mono or stereo) */
		DPRINTK("SOUND_MIXER_WRITE_PHONEIN ioctl called\n");
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		ret = set_input_volume(dev, SOUND_MASK_PHONEIN, val);
		if (ret >= 0) {
			ret = 0;
		}

		break;

	case SOUND_MIXER_WRITE_LINE:	/* line input */
		DPRINTK("SOUND_MIXER_WRITE_PHONEIN ioctl called\n");
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		ret = set_input_volume(dev, SOUND_MASK_LINE, val);
		if (ret >= 0) {
			ret = 0;
		}

		break;

	case SNDCTL_MC13783_WRITE_OUT_BALANCE:
		DPRINTK("SNDCTL_MC13783_WRITE_OUT_BALANCE IOCTL\n");
		if (get_user(val, (int *)arg)) {
			return -EFAULT;
		}

		val = set_output_balance(dev, val);
		break;

	default:
		return -EINVAL;
	}

	put_user(val, (int __user *)arg);
	return ret;
}

/*!
 * This function handles the user commands to the audio driver(s)
 *
 * @param    dev      the device's major number
 * @param    cmd      a symbol that describes the command to apply
 * @param    arg      the command argument
 *
 * @return            0 if success, else error
 */
static int mxc_audio_mc13783_ioctl(int dev, unsigned int cmd, void __user * arg)
{
	int val = 0;
	int ret = 0;

	switch (cmd) {
	case SOUND_PCM_WRITE_RATE:
		DPRINTK("SOUND_PCM_WRITE_RATE IOCTL: value = %d\n", val);
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		val = mxc_audio_mc13783_set_speed(dev, val);
		break;

	case SOUND_PCM_READ_RATE:
		DPRINTK("SOUND_PCM_READ_RATE IOCTL\n");
		val = mxc_audio_mc13783_set_speed(dev, 0);
		break;

	case SOUND_PCM_WRITE_CHANNELS:
		DPRINTK("SOUND_PCM_WRITE_CHANNELS IOCTL: value = %d\n", val);
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		val = mxc_audio_mc13783_set_channels(dev, val);
		break;

	case SOUND_PCM_READ_CHANNELS:
		DPRINTK("SOUND_PCM_READ_CHANNELS IOCTL\n");
		val = mxc_audio_mc13783_set_channels(dev, 0);
		break;

	case SOUND_PCM_WRITE_BITS:
		DPRINTK("SOUND_PCM_WRITE_BITS IOCTL: value = %d\n", val);
		if (get_user(val, (int __user *)arg)) {
			return -EFAULT;
		}

		val = mxc_audio_mc13783_set_bits(dev, val);
		break;

	case SOUND_PCM_READ_BITS:
		DPRINTK("SOUND_PCM_READ_BITS IOCTL\n");
		val = mxc_audio_mc13783_set_bits(dev, 0);
		break;

	case SNDCTL_MC13783_WRITE_OUT_ADDER:
		DPRINTK("SNDCTL_MC13783_WRITE_OUT_ADDER IOCTL\n");
		if (get_user(val, (int *)arg)) {
			return -EFAULT;
		}

		val = set_output_adder(dev, val);
		break;

	case SNDCTL_MC13783_READ_OUT_ADDER:
		DPRINTK("SNDCTL_MC13783_READ_OUT_ADDER IOCTL\n");
		val = get_output_adder(dev);
		break;

	case SNDCTL_MC13783_WRITE_OUT_BALANCE:
		DPRINTK("SNDCTL_MC13783_WRITE_OUT_BALANCE IOCTL\n");
		if (get_user(val, (int *)arg)) {
			return -EFAULT;
		}

		val = set_output_balance(dev, val);
		break;

	case SNDCTL_MC13783_READ_OUT_BALANCE:
		DPRINTK("SNDCTL_MC13783_READ_OUT_BALANCE IOCTL\n");
		val = get_output_balance(dev);
		break;

	case SNDCTL_MC13783_WRITE_CODEC_FILTER:
		if (get_user(val, (int *)arg)) {
			return -EFAULT;
		}

		ret = set_codec_filter(dev, val);
		if (ret >= 0) {
			ret = 0;
		}

		break;

	case SNDCTL_MC13783_READ_CODEC_FILTER:
		if (get_user(val, (int *)arg)) {
			return -EFAULT;
		}

		val = get_codec_filter(dev);
		break;

	case SNDCTL_MC13783_WRITE_PORT:
		if (get_user(val, (int *)arg)) {
			return -EFAULT;
		}

		break;

	case SNDCTL_MC13783_WRITE_MASTER_CLOCK:
		DPRINTK("SNDCTL_MC13783_WRITE_MASTER_CLOCK IOCTL\n");
		if (get_user(val, (int *)arg)) {
			return -EFAULT;
		}

		if ((val != MXC_AUDIO_CLOCKING_MC13783_MASTER) &&
		    (val != MXC_AUDIO_CLOCKING_MCU_MASTER)) {
			return -EINVAL;
		}

		ret = set_audio_clock_master(dev, val);
		break;

	default:
		return -EINVAL;
	}

	put_user(val, (int __user *)arg);
	return ret;
}

/*!
 * This structure defines operations for OSS audio device(s).
 */
static struct audio_driver mxc_audio_mc13783_ops = {
	.owner = THIS_MODULE,
	.open = mxc_audio_mc13783_open,
	.close = mxc_audio_mc13783_close,
	.output_block = mxc_audio_mc13783_output_block,
	.start_input = mxc_audio_mc13783_start_input,
	.ioctl = mxc_audio_mc13783_ioctl,
	.prepare_for_input = mxc_audio_mc13783_prepare_input,
	.prepare_for_output = mxc_audio_mc13783_prepare_output,
	.halt_io = mxc_audio_mc13783_halt_io,
	.trigger = mxc_audio_mc13783_trigger,
	.set_speed = mxc_audio_mc13783_set_speed,
	.set_bits = mxc_audio_mc13783_set_bits,
	.set_channels = mxc_audio_mc13783_set_channels,
};

/*!
 * This structure defines operations for OSS mixer device.
 */
static struct mixer_operations mxc_audio_mc13783_mixer_ops = {
	.owner = THIS_MODULE,
	.id = "mc13783 mixer",
	.name = "mxc mc13783 mixer driver",
	.ioctl = mxc_audio_mc13783_mixer_ioctl
};

/*!
 * This functions initializes the 2 audio devices supported by
 * mc13783 IC. The parameters define the type of device (CODEC or STEREODAC)
 *
 * @param       ssi     Tells the device on what SSI it will make transfers.
 * @param       fifo    Tells the device the SSI FIFO he will use
 *                      (FIFO0 or FIFO1)
 * @param       devtype Tells the device its identity (CODEC or STEREODAC)
 *
 * @return              On success, this function returns a pointer to a device
 *                      structure initialized with default values, NULL
 *                      otherwise.
 */
struct device_data *__init init_device(int ssi, int fifo, int devtype)
{
	struct device_data *device;

	device = (struct device_data *)
	    kmalloc(sizeof(struct device_data), GFP_KERNEL);
	if (device == NULL) {
		return NULL;
	}

	memset(device, 0, sizeof(struct device_data));
	sema_init(&device->sem, 1);

	/* These parameters defines the identity of
	 * the device (codec or stereodac)
	 */
	device->ssi = ssi;
	device->ssi_fifo = fifo;
	device->mc13783.device = devtype;
	device->mc13783.ssi = ssi;

	device->device_index = -1;
	device->dma_wchannel = -1;
	device->dma_rchannel = -1;
	device->sample_rate = 8000;
	device->num_channels = 1;
	device->bits_per_sample = AFMT_S16_LE;
	device->busy = 0;
	device->transferring = 0;

	device->mc13783.mode = ms_master;
	device->mc13783.protocol = bp_network;	/* OJO */
	device->mc13783.sample_rate = sr_8k;
	device->mc13783.pll = ci_cli_b;
	device->mc13783.pll_rate = CLK_SELECT_MASTER_CLI;
	device->mc13783.bcl_inverted = 0;
	device->mc13783.fs_inverted = 0;

	device->mc13783.adder_mode = ac_stereo;

	device->mc13783.balance = NO_BALANCE;

	return device;
}

/*!
 * This functions initializes the MXC mixer device.
 *
 * @return              On success, this function returns a pointer to a device
 *                      structure initialized with default values, NULL
 *                      otherwise.
 */
struct mixer_data *__init init_mixer(void)
{
	struct mixer_data *mixer;

	mixer = (struct mixer_data *)
	    kmalloc(sizeof(struct mixer_data), GFP_KERNEL);

	if (mixer == NULL) {
		return NULL;
	}

	memset(mixer, 0, sizeof(struct mixer_data));
	sema_init(&mixer->sem, 1);

	mixer->input_device = SOUND_MASK_MIC;
	mixer->output_device = SOUND_MASK_VOLUME;

	mixer->input_volume = ((50 << 8) & 0xff00) | (50 & 0x00ff);
	mixer->output_volume = ((50 << 8) & 0xff00) | (50 & 0x00ff);

	mixer->mixer_dev = -1;

	return mixer;

}

/*!
 * This function frees allocated resources and unloads the driver.
 * Specifically it frees the two device instances used by the OSS driver.
 */
void __exit mxc_audio_mc13783_exit(void)
{
	mc13783_audio_set_autodetect(0);

	if (devices[0] != NULL) {
		if (devices[0]->device_index >= 0) {
			sound_unload_audiodev(devices[0]->device_index);
		}

		kfree(devices[0]);
		devices[0] = NULL;
	}

	if (devices[1] != NULL) {
		if (devices[1]->device_index >= 0) {
			sound_unload_audiodev(devices[1]->device_index);
		}

		kfree(devices[1]);
		devices[1] = NULL;
	}

	if (mxc_mc13783_mixer->mixer_dev != -1) {
		sound_unload_mixerdev(mxc_mc13783_mixer->mixer_dev);
	}

	kfree(mxc_mc13783_mixer);
}

/*!
 * This function initializes the mxc audio driver. It allocates resources
 * for audio operations, requests irqs for SSI and creates entries in
 * filesystem for audio devices and mixer.
 */
int __init mxc_audio_mc13783_init(void)
{
	mxc_mc13783_mixer = init_mixer();
	if (mxc_mc13783_mixer == NULL) {
		return -1;
	}

	/* Define first device as the codec */
	devices[0] = init_device(SSI1, ssi_fifo_0, rs_codec);
	if (devices[0] == NULL) {
		goto error6;
	}

	/* Define second device as the stereodac */
	devices[1] = init_device(SSI2, ssi_fifo_0, rs_st_dac);
	if (devices[1] == NULL) {
		goto error5;
	}

	/* Install mixer driver */
	mxc_mc13783_mixer->mixer_dev = sound_install_mixer(MIXER_DRIVER_VERSION,
							   mxc_audio_mc13783_mixer_ops.
							   name,
							   &mxc_audio_mc13783_mixer_ops,
							   sizeof(struct
								  mixer_operations),
							   (void *)
							   &mxc_mc13783_mixer);
	if (mxc_mc13783_mixer->mixer_dev < 0) {
		goto error4;
	}

	DPRINTK("mixdev index = %d\n", mxc_mc13783_mixer->mixer_dev);
	printk("MXC mixer installed\n");

	/* Install audio driver 1, with dummy Rx and Tx DMA channels.
	 * DMA channels are allocated in open.
	 */
	devices[0]->device_index =
	    sound_install_audiodrv(AUDIO_DRIVER_VERSION,
				   "mxc audio driver 1",
				   &mxc_audio_mc13783_ops,
				   sizeof(struct audio_driver),
				   DMA_DUPLEX, AFMT_S16_LE, devices[0], -1, -1);

	if (devices[0]->device_index == -1) {
		goto error3;
	}

	/* Install audio driver 2 */
	devices[1]->device_index =
	    sound_install_audiodrv(AUDIO_DRIVER_VERSION,
				   "mxc audio driver 2",
				   &mxc_audio_mc13783_ops,
				   sizeof(struct audio_driver),
				   DMA_DUPLEX, AFMT_S16_LE, devices[1], -1, -1);

	if (devices[1]->device_index == -1) {
		goto error2;
	}

	/* Assign USBPLL to be used by SSI1/2 */
	mxc_set_clocks_pll(SSI1_BAUD, USBPLL);
	mxc_set_clocks_pll(SSI2_BAUD, USBPLL);

	/* Set autodetect feature in order to allow audio operations */
	mc13783_audio_set_autodetect(1);

	printk("MXC Audio Driver loaded\n");
	return 0;

      error2:
	sound_unload_audiodev(devices[0]->device_index);
      error3:
	sound_unload_mixerdev(mxc_mc13783_mixer->mixer_dev);
      error4:
	kfree(devices[1]);
      error5:
	kfree(devices[0]);
      error6:
	kfree(mxc_mc13783_mixer);
	return -1;
}

module_init(mxc_audio_mc13783_init);
module_exit(mxc_audio_mc13783_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("OSS Audio Device Driver for MXC/mc13783");
MODULE_LICENSE("GPL");
