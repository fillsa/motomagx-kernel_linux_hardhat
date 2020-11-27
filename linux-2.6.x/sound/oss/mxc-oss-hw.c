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
 */

/*!
 * @file mxc-oss-hw.c
 * @brief This file implements the hardware-dependent part of the OSS
 *        sound driver.
 *
 * This source file implements the functions used to configure the
 * following audio-related hardware components:
 *
 * - SSI
 * - SDMA
 * - Digital Audio MUX
 * - PMIC (using the unified PMIC APIs)
 *
 * This source file is based originally on the mxc-hwdep.c file that
 * was used to implement the OSS support for the mc13783 PMIC.
 *
 * @ingroup SOUND_OSS_MXC_PMIC
 */

#include <linux/kernel.h>	/* For printk() debugging messages.       */

#include <asm/arch/clock.h>	/* For SSI clock configuration.      */
#include <asm/arch/pmic_audio.h>	/* For PMIC API definitions.         */

#include "sound_config.h"	/* OSS sound framework macro definitions  */
#include "dev_table.h"		/* OSS sound framework API definitions    */

#include "mxc-oss-hw.h"		/* PMIC for OSS hardware interface APIs   */
#include "mxc-oss-dbg.h"	/* PMIC for OSS driver debug definitions  */

/*
 *************************************************************************
 * GLOBAL STATIC DEFINITIONS
 *************************************************************************
 */

/*! The following SSI bit clock configuration settings are based upon
 *  16 bits/word and 4 words/frame. These values are used to configure
 *  the USB PLL and the SSI clock generator in order to produce the
 *  required bit clock and frame sync signals when the SSI is in master
 *  mode. These values are not used when the SSI is operating in slave
 *  mode.
 */
static const struct {
	unsigned int samplingRate_Hz;	/*< Sampling rate (Hz)           */
	struct {
		unsigned long target_ccm_ssi_clk_Hz;	/*< SSI master clock freq (Hz)   */
		unsigned char useDiv2;	/*< Enable/disable Div2 divider  */
		unsigned char usePSR;	/*< Enable/disable /8 divider    */
		unsigned char prescalerModulus;	/*< Prescaler modulus value      */
		unsigned long targetBitClock_Hz;	/*< SSI master clock target (Hz) */
	} clockParam;
} ssiClockSetup[] = {
	{
		8000,		/* Clock settings for 8 kHz sampling rate.   */
		{
			12288000,	/* Ideal input clock = 1.2288 MHz    */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    12,		/* Use /12 prescaler modulus         */
			    512000,	/* SSI master clock target = 512 kHz */
		}
	}, {
		11025,		/* Clock settings for 11.025 kHz sampling rate. */
		{
			11289600,	/* Ideal input clock = 11.2896 MHz   */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    8,		/* Use /8 prescaler modulus          */
			    705600,	/* SSI master clock target = 705.6 kHz*/
		}
	}, {
		12000,		/* Clock settings for 12 kHz sampling rate.  */
		{
			12288000,	/* Ideal input clock = 12.288 MHz    */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    8,		/* Use /8 prescaler modulus          */
			    768000	/* SSI master clock target = 768 kHz */
		}
	}, {
		16000,		/* Clock settings for 16 kHz sampling rate.  */
		{
			12288000,	/* Ideal input clock = 12.288 MHz    */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    6,		/* Use /6 prescaler modulus          */
			    1024000,	/* SSI master clock target = 1.024 MHz*/
		}
	}, {
		22050,		/* Clock settings for 22.05 kHz sampling rate.*/
		{
			11289600,	/* Ideal input clock = 11.2896 MHz   */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    4,		/* Use /4 prescaler modulus          */
			    1411200,	/* SSI master clock target = 1.4112 MHz */
		}
	}, {
		24000,		/* Clock settings for 24 kHz sampling rate.  */
		{
			12288000,	/* Ideal input clock = 12.288 MHz    */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    4,		/* Use /4 prescaler modulus          */
			    1536000	/* SSI master clock target = 1.536 MHz */
		}
	}, {
		32000,		/* Clock settings for 32 kHz sampling rate.  */
		{
			12288000,	/* Ideal input clock = 12.288 MHz    */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    3,		/* Use /3 prescaler modulus          */
			    2048000,	/* SSI master clock target = 2.048 MHz */
		}
	}, {
		44100,		/* Clock settings for 44.1 kHz sampling rate. */
		{
			11289600,	/* Ideal input clock = 11.2896 MHz   */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    2,		/* Use /2 prescaler modulus          */
			    2822400	/* SSI master clock target = 2.8224 MHz */
		}
	}, {
		48000,		/* Clock settings for 48 kHz sampling rate.  */
		{
			12288000,	/* Ideal input clock = 12.288 MHz    */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    2,		/* Use /2 prescaler modulus          */
			    3072000	/* SSI master clock target = 3.072 MHz */
		}
	}, {
		64000,		/* Clock settings for 64 kHz sampling rate.  */
		{
			12288000,	/* Ideal input clock = 12.288 MHz    */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    3,		/* Use /3 prescaler modulus          */
			    4096000	/* SSI master clock target = 4.096 MHz */
		}
	}, {
		96000,		/* Clock settings for 96 kHz sampling rate.  */
		{
			12288000,	/* Ideal input clock = 12.288 MHz    */
			    false,	/* Disable Div2 divider              */
			    false,	/* Disable /8 divider                */
			    2,		/* Use /2 prescaler modulus          */
			    6144000	/* SSI master clock target = 6.144 MHz */
		}
	},
};

/*! Define the size of the SSI master clock setup table. @see ssiClockSetup */
static const unsigned int nSSIClockSetup = sizeof(ssiClockSetup) /
    sizeof(ssiClockSetup[0]);

/*!
 * This function configures the USB PLL to act as the clock source for the
 * SSI's internal clock generator.
 *
 * For the sake of simplicity, we have fixed both the bits/word and words/frame
 * values at 16 and 4, respectively. The SSI can be configured in many other
 * operating modes but fixing these two parameters greatly simplifies the
 * configuration of the SSI and the PMIC.
 *
 * @param        ssi_index [in] the SSI module (SSI1 or SSI2) being configured
 * @param        ssiClockSettingIndex [in] the index into ssiClockSetup[]
 * @param        samplingRate_Hz [in] sampling rate of the audio stream
 *
 */
static void mxc_config_pll(const ssi_mod ssi_index,
			   const int ssiClockSettingIndex,
			   const unsigned int samplingRate_Hz)
{
#if defined(DEBUG) || defined(DEBUG_ALL)
	static const int bitsPerWord = 16;
	static const int wordsPerFrame = 4;

	unsigned long ssiTargetBitClock_Hz = bitsPerWord * wordsPerFrame *
	    samplingRate_Hz;
#endif				/* defined(DEBUG) || defined(DEBUG_ALL) */

	unsigned long usbPLLfreq_Hz = 0;
	unsigned long usbPLLDivider = 0;
	unsigned char usbPLLDividerFraction = 0;
	enum mxc_clocks pllClockSelect;
	unsigned long ssiActualBitClock_Hz;

	/*! Identify which SSI clock we want to activate. */
	if (ssi_index == SSI1) {
		pllClockSelect = SSI1_BAUD;
	} else if (ssi_index == SSI2) {
		pllClockSelect = SSI2_BAUD;
	} else {
		/*! Invalid SSI index. */
		DPRINTK("!!! ERROR: attempting to use invalid SSI%d !!!\n",
			ssi_index + 1);
		return;
	}

	/*! Configure and enable the SSI internal clock source using
	 *  the USB PLL as the master clock signal.
	 *
	 *  To calculate the closest USB PLL divider value:
	 *
	 *  1. Divide the USB PLL frequency by the desired SSI clock input
	 *     frequency (scaling by 10 to include one decimal place of
	 *     precision).
	 *  2. If the last digit is 0, 1, or 2, then round down to zero.
	 *     For example, 162 would become 160.
	 *  3. If the last digit is 3, 4, 5, 6, or 7, then round it to 5
	 *     (since the USB PLL divider only supports fractional division
	 *     by 0.5). For example, 166 would become 165.
	 *  4. If the last digit is 8 or 9, then round up to the next
	 *     multiple of 10. For example, 168 would become 170.
	 *  5. Finally, convert the PLL divider to the correct register
	 *     value.
	 */
	usbPLLfreq_Hz = mxc_pll_clock(USBPLL);
	usbPLLDivider = (usbPLLfreq_Hz * 10) /
	    ssiClockSetup[ssiClockSettingIndex].clockParam.
	    target_ccm_ssi_clk_Hz;
	usbPLLDividerFraction = (unsigned char)(usbPLLDivider % 10);
	usbPLLDivider = usbPLLDivider / 10 * 2;
	if (usbPLLDividerFraction > 7) {
		/*! Add 1.0 to the PLL divider. Note that the first bit is reserved
		 *  for specifying a 0.5 fractional divider so we actually need to
		 *  increment the second bit in the value of usbPLLDivider.
		 */
		usbPLLDivider += 2;
	} else if (usbPLLDividerFraction > 2) {
		/*! Add 0.5 to the PLL divider. Note that the first bit is reserved
		 *  for specifying a 0.5 fractional divider so we just add one to set
		 *  the bit.
		 */
		usbPLLDivider++;
	}

	/*! Configure the USB PLL to be the SSI internal clock source. */
	mxc_set_clocks_pll(pllClockSelect, USBPLL);
	mxc_set_clocks_div(pllClockSelect, usbPLLDivider);

	/*! Enable the PLL to start generating an SSI bit clock. */
	mxc_clks_enable(pllClockSelect);

	DPRINTK("INFO: USB PLL frequency = %lu Hz\n", usbPLLfreq_Hz);
	DPRINTK("INFO: SSI%d PLL divider  = %ld\n", ssi_index + 1,
		usbPLLDivider);
	DPRINTK("INFO: SSI%d ccm_ssi_clk  = %lu Hz\n", ssi_index + 1,
		mxc_get_clocks(pllClockSelect));

	DPRINTK("INFO: SSI%d Prescaler Modulus = %d\n", ssi_index + 1,
		ssiClockSetup[ssiClockSettingIndex].clockParam.
		prescalerModulus);

	ssiActualBitClock_Hz = mxc_get_clocks(pllClockSelect) /
	    (ssiClockSetup[ssiClockSettingIndex].clockParam.useDiv2 ? 2 : 1) /
	    (ssiClockSetup[ssiClockSettingIndex].clockParam.usePSR ? 8 : 1) /
	    ssiClockSetup[ssiClockSettingIndex].clockParam.prescalerModulus / 2;

	DPRINTK("INFO: Bit clock actual = %lu Hz\n", ssiActualBitClock_Hz);
	DPRINTK("INFO: Bit clock target = %lu Hz\n", ssiTargetBitClock_Hz);
	DPRINTK("INFO: Bit clock error  = %ld Hz\n",
		(long)(ssiActualBitClock_Hz - ssiTargetBitClock_Hz));
}

/*!
 * This function resets the SSI bus as recommended in the MXC Reference Manual.
 *
 * @param        index : the target SSI module
 * @return        None
 *
 */
void mxc_reset_ssi(const int index)
{
	FUNC_START;
	ssi_enable(index, false);
	ssi_enable(index, true);

	FUNC_END;
}

/*!
 * This function configures the SSI bus (including the correct clock settings
 * for either master or slave mode).
 *
 * @param        drv_inst [in] info about the current transfer
 *
 */
void mxc_setup_ssi(const mxc_state_t * const drv_inst)
{
	ssi_mod ssi_index = drv_inst->ssi_index;
	int i;

	FUNC_START;
	/*! Disable the SSI transmitter and receiver. */
	ssi_transmit_enable(ssi_index, false);
	ssi_receive_enable(ssi_index, false);

	/*! Next disable the SSI so that we can reconfigure the items that
	 *  can only be changed when the SSI is disabled (see the SSI chapter
	 *  of the MXC Reference Manual for complete details).
	 *
	 *  Note that this will also flush all data from both the transmit and
	 *  receive FIFOs.
	 */
	ssi_enable(ssi_index, false);

	/*! Disable all SSI interrupts (specifically the DMA interrupts). */
	ssi_interrupt_disable(ssi_index, MXC_SSI_INT_DISABLE_ALL);

	/*! Disable the SSI internal clock source. */
	mxc_clks_disable((ssi_index == SSI1) ? SSI1_BAUD : SSI2_BAUD);

	/*! Configure for synchronous 4-wire network mode. */
	ssi_system_clock(ssi_index, false);
	ssi_i2s_mode(ssi_index, i2s_normal);
	ssi_network_mode(ssi_index, true);
	ssi_synchronous_mode(ssi_index, true);
	ssi_two_channel_mode(ssi_index, false);

	/*! FIFOs must be disabled before SDMA is configured */
	ssi_tx_fifo_enable(ssi_index, drv_inst->ssi_fifo_nb, false);
	ssi_rx_fifo_enable(ssi_index, drv_inst->ssi_fifo_nb, false);

	/*! Configure full and empty FIFO watermark levels. */
	ssi_rx_fifo_full_watermark(ssi_index, ssi_fifo_0, RX_FIFO_FULL_WML);
	ssi_rx_fifo_full_watermark(ssi_index, ssi_fifo_1, RX_FIFO_FULL_WML);
	ssi_tx_fifo_empty_watermark(ssi_index, ssi_fifo_0, TX_FIFO_EMPTY_WML);
	ssi_tx_fifo_empty_watermark(ssi_index, ssi_fifo_1, TX_FIFO_EMPTY_WML);

	DPRINTK("INFO: FIFO0 TX Empty %d, RX Full %d\n",
		TX_FIFO_EMPTY_WML, RX_FIFO_FULL_WML);

	/*! SSI transmitter configuration for audio playback. This is also the
	 *  receiver configuration for audio recording since we are using
	 *  synchronous mode. Therefore, there is no need to separately specify
	 *  any SSI receiver settings.
	 */
	ssi_tx_early_frame_sync(ssi_index, ssi_frame_sync_one_bit_before);
	ssi_tx_frame_sync_length(ssi_index, ssi_frame_sync_one_bit);
	ssi_tx_frame_sync_active(ssi_index, ssi_frame_sync_active_high);
	ssi_tx_clock_polarity(ssi_index, ssi_clock_on_rising_edge);
	ssi_tx_shift_direction(ssi_index, ssi_msb_first);
	ssi_tx_bit0(ssi_index, true);
	ssi_tx_word_length(ssi_index, ssi_16_bits);
	ssi_tx_frame_rate(ssi_index, (unsigned char)4);

	/*! Disable DMA interrupt generation. We will enable DMA interrupts
	 *  later when we actually start the first DMA operation.
	 *
	 *  If we are using FIFO mode, then we will just leave the DMA
	 *  interrupts disabled.
	 */
	ssi_interrupt_disable(ssi_index, ssi_rx_dma_interrupt_enable);
	ssi_interrupt_disable(ssi_index, ssi_tx_dma_interrupt_enable);

	/*! Configure the SSI for either master or slave clocking mode. */
	if (drv_inst->clock_master == MXC_AUDIO_CLOCKING_SSI_MASTER) {
		DPRINTK("INFO: Configuring SSI%d as bus clock master\n",
			ssi_index + 1);

		/*! Search the SSI clock configuration table for the appropriate
		 *  clock settings to be used based upon the desired sampling rate.
		 */
		for (i = 0; i < nSSIClockSetup; i++) {
			if (drv_inst->pmic.in_out_sr_Hz ==
			    ssiClockSetup[i].samplingRate_Hz) {
				DPRINTK
				    ("INFO: Using sampling rate of %d Hz (table index %d) "
				     "for SSI%d\n", drv_inst->pmic.in_out_sr_Hz,
				     i, ssi_index + 1);
				break;
			}
		}

		if (i >= nSSIClockSetup) {
			/*! An unsupported sampling rate was given. Just leave the SSI
			 *  disabled.
			 */
			DPRINTK
			    ("INFO: Invalid sampling rate %d given for SSI%d\n",
			     drv_inst->pmic.in_out_sr_Hz, ssi_index + 1);
			FUNC_ERR;
			return;
		}

		/*! Configure the SSI clock dividers to support the desired
		 *  sampling rate.
		 */
		ssi_tx_clock_divide_by_two(ssi_index,
					   ssiClockSetup[i].clockParam.useDiv2);
		ssi_tx_clock_prescaler(ssi_index,
				       ssiClockSetup[i].clockParam.usePSR);
		ssi_tx_prescaler_modulus(ssi_index,
					 ssiClockSetup[i].clockParam.
					 prescalerModulus);

		/*! Configure the SSI to generate bit clock and frame sync
		 *  using the internal clock source (SSI in master mode).
		 */
		ssi_tx_frame_direction(ssi_index, ssi_tx_rx_internally);
		ssi_tx_clock_direction(ssi_index, ssi_tx_rx_internally);

		mxc_config_pll(ssi_index, i, drv_inst->pmic.in_out_sr_Hz);
	} else {
		DPRINTK("INFO: Configuring SSI%d as bus clock slave\n",
			ssi_index + 1);

		/*! Configure the SSI to use externally generated bitclock
		 *  and frame sync signals (SSI in slave mode).
		 */
		ssi_tx_frame_direction(ssi_index, ssi_tx_rx_externally);
		ssi_tx_clock_direction(ssi_index, ssi_tx_rx_externally);
	}

	/*! We can reenable the SSI now and complete the configuration process
	 *  by setting the Tx and Rx timeslot masks.
	 *
	 *  Note that the SSI has to be reenabled first before the Tx and Rx
	 *  mask registers can be successfully updated.
	 *
	 *  Also note that we do not reenable the SSI transmitter and/or receiver
	 *  until we actually have data to be sent/received (which occurs later
	 *  when the audio application actually initiates an I/O operation).
	 */
	ssi_enable(ssi_index, true);
	ssi_tx_mask_time_slot(ssi_index, drv_inst->ssi_tx_timeslots);
	ssi_rx_mask_time_slot(ssi_index, drv_inst->ssi_rx_timeslots);

	DPRINTK("INFO: SSI%d TX timeslot mask 0x%08x\n", ssi_index + 1,
		drv_inst->ssi_tx_timeslots);
	DPRINTK("INFO: SSI%d RX timeslot mask 0x%08x\n", ssi_index + 1,
		drv_inst->ssi_rx_timeslots);

	FUNC_END;
}

/*!
 * This function configures the PMIC Stereo DAC. The required master
 * clock setup is also done if the Stereo DAC is to be used in master
 * mode.
 *
 * @param        drv_inst [in] audio driver instance
 * @return       0 if success
 *
 */
int mxc_setup_pmic_stdac(const mxc_state_t * const drv_inst)
{
	PMIC_AUDIO_CLOCK_IN_SOURCE clockIn;
	PMIC_AUDIO_STDAC_CLOCK_IN_FREQ clockFreq;
	PMIC_AUDIO_BUS_MODE masterSlaveModeSelect;
	PMIC_AUDIO_STDAC_SAMPLING_RATE stdac_rate;

	FUNC_START;
	/*! Connect the PMIC Stereo DAC to the correct data bus and
	 *  also configure the correct data bus protocol and clock
	 *  master/slave mode.
	 */
	if (drv_inst->dam_dest != port_5) {
		DPRINTK
		    ("!!! ERROR: Attempting to use invalid Audio MUX Port%d for "
		     "Stereo DAC (require Port5) !!!\n",
		     drv_inst->dam_dest + 1);
		goto CATCH;
	}

	DPRINTK("INFO: Stereo DAC using AUDMUX Port 5/Data Bus 2\n");

	DPRINTK("INFO: Stereo DAC sampling rate is %d Hz\n",
		drv_inst->pmic.in_out_sr_Hz);

	switch (drv_inst->pmic.in_out_sr_Hz) {
	case 8000:
		stdac_rate = STDAC_RATE_8_KHZ;
		break;
	case 11025:
		stdac_rate = STDAC_RATE_11_025_KHZ;
		break;
	case 12000:
		stdac_rate = STDAC_RATE_12_KHZ;
		break;
	case 16000:
		stdac_rate = STDAC_RATE_16_KHZ;
		break;
	case 22050:
		stdac_rate = STDAC_RATE_22_050_KHZ;
		break;
	case 24000:
		stdac_rate = STDAC_RATE_24_KHZ;
		break;
	case 32000:
		stdac_rate = STDAC_RATE_32_KHZ;
		break;
	case 44100:
		stdac_rate = STDAC_RATE_44_1_KHZ;
		break;
	case 48000:
		stdac_rate = STDAC_RATE_48_KHZ;
		break;
	case 64000:
		stdac_rate = STDAC_RATE_64_KHZ;
		break;
	case 96000:
		stdac_rate = STDAC_RATE_96_KHZ;
		break;
	default:
		DPRINTK
		    ("!!! ERROR: Invalid Stereo DAC sampling rate %d Hz !!!\n",
		     drv_inst->pmic.in_out_sr_Hz);
		goto CATCH;
	}

	/*! Start by disabling the Stereo DAC to avoid any noise that may
	 *  result when we change the clock configuration.
	 */
	if (pmic_audio_disable(drv_inst->pmic.handle) != PMIC_SUCCESS) {
		DPRINTK("!!! ERROR: pmic_audio_disable() failed !!!\n");
		goto CATCH;
	}

	/*! Select Stereo DAC options based upon master/slave mode. */
	if (drv_inst->clock_master == MXC_AUDIO_CLOCKING_PMIC_MASTER) {
		masterSlaveModeSelect = BUS_MASTER_MODE;

		/*! Configure the Stereo DAC clock settings for master mode. */
		clockIn = CLOCK_IN_CLKIN;
		clockFreq = STDAC_CLI_33_6MHZ;

		DPRINTK("INFO: STDAC master mode using CLKIN @ 33.6 MHz\n");
	} else {
		masterSlaveModeSelect = BUS_SLAVE_MODE;

		/*! Configure the Stereo DAC clock settings for slave mode. */
		clockIn = CLOCK_IN_FSYNC;
		clockFreq = STDAC_FSYNC_IN_PLL;

		DPRINTK("INFO: STDAC slave mode using FSYNC\n");
	}

	/* Configure the Stereo DAC's clock source and frequency. */
	if (pmic_audio_stdac_set_clock
	    (drv_inst->pmic.handle, clockIn, clockFreq, stdac_rate,
	     NO_INVERT) != PMIC_SUCCESS) {
		DPRINTK("!!! ERROR: mxc_setup_pmic_stdac(): "
			"pmic_audio_stdac_set_clock() failed !!!\n");
		goto CATCH;
	}

	/* Configure the Stereo DAC's audio data bus protocol. */
	if (pmic_audio_set_protocol(drv_inst->pmic.handle, AUDIO_DATA_BUS_2,
				    NETWORK_MODE, masterSlaveModeSelect,
				    USE_4_TIMESLOTS) != PMIC_SUCCESS) {
		DPRINTK
		    ("!!! ERROR: mxc_setup_pmic_stdac(): pmic_audio_set_protocol() "
		     "failed !!!\n");
		goto CATCH;
	}

	/* Finally, enable the Stereo DAC. */
	if (pmic_audio_enable(drv_inst->pmic.handle) != PMIC_SUCCESS) {
		DPRINTK
		    ("!!! ERROR: mxc_setup_pmic_stdac(): pmic_audio_enable() "
		     "failed !!!\n");
		goto CATCH;
	}

	DPRINTK("INFO: Stereo DAC enabled\n");

	FUNC_END;
	return 0;

      CATCH:
	FUNC_ERR;
	return -1;
}

/*!
 * This function configures the PMIC Voice CODEC. The required master
 * clock setup is also done if the Voice CODEC is to be used in master
 * mode.
 *
 * @param         drv_inst [in] audio driver instance
 * @return        0 if success
 *
 */
int mxc_setup_pmic_codec(const mxc_state_t * const drv_inst)
{
	PMIC_AUDIO_CLOCK_IN_SOURCE clockIn;
	PMIC_AUDIO_VCODEC_CLOCK_IN_FREQ clockFreq;
	PMIC_AUDIO_BUS_MODE masterSlaveModeSelect;
	PMIC_AUDIO_VCODEC_SAMPLING_RATE vcodec_rate;

	FUNC_START;
	/*! The Voice CODEC can only be connected to Audio MUX Port 4
	 *  and the PMIC Data Bus 1 because that is the only path that
	 *  has the RXD pin connected to support audio recording.
	 *
	 *  It is possible to connect the Voice CODEC to Audio MUX Port 5
	 *  and PMIC Data Bus 2 if only playback is required. However, to
	 *  keep things simple, we'll reserve Audio MUX Port 4 for use with
	 *  the Voice CODEC and Audio MUX Port 5 for use with the Stereo
	 *  DAC (which doesn't have any recording capabilities).
	 */
	if (drv_inst->dam_dest != port_4) {
		DPRINTK
		    ("!!! ERROR: Attempting to use invalid Audio MUX Port%d for "
		     "Voice CODEC (require Port4) !!!\n",
		     drv_inst->dam_dest + 1);
		goto CATCH;
	}

	DPRINTK("INFO: Voice CODEC using AUDMUX Port 4/Data Bus 1\n");

	DPRINTK("INFO: Voice CODEC sampling rate is %d Hz\n",
		drv_inst->pmic.in_out_sr_Hz);

	switch (drv_inst->pmic.in_out_sr_Hz) {
	case 8000:

		vcodec_rate = VCODEC_RATE_8_KHZ;
		break;

	case 16000:

		vcodec_rate = VCODEC_RATE_16_KHZ;
		break;

	default:
		DPRINTK
		    ("!!! ERROR: Invalid Voice CODEC sampling rate %d Hz !!!\n",
		     drv_inst->pmic.in_out_sr_Hz);
		goto CATCH;
	}

	/*! Start by disabling the Voice CODEC to avoid any noise that may
	 *  result when we change the clock configuration.
	 */
	if (pmic_audio_disable(drv_inst->pmic.handle) != PMIC_SUCCESS) {
		DPRINTK("!!! ERROR: pmic_audio_disable() failed !!!\n");
		goto CATCH;
	}

	/*! Select Voice CODEC options based upon master/slave mode. */
	if (drv_inst->clock_master == MXC_AUDIO_CLOCKING_PMIC_MASTER) {
		masterSlaveModeSelect = BUS_MASTER_MODE;

		/*! Configure the Voice CODEC clock settings for master mode. */
		clockIn = CLOCK_IN_CLKIN;
		clockFreq = VCODEC_CLI_33_6MHZ;

		DPRINTK
		    ("INFO: Voice CODEC master mode using CLKIN @ 33.6 MHz\n");
	} else {
		masterSlaveModeSelect = BUS_SLAVE_MODE;

		/*! Configure the Voice CODEC clock settings for slave mode. */
		clockIn = CLOCK_IN_CLKIN;
		clockFreq = VCODEC_CLI_33_6MHZ;

		DPRINTK
		    ("INFO: Voice CODEC slave mode using CLKIN @ 33.6 MHz\n");
	}

	/*! Configure the Voice CODEC's clock source and frequency. */
	if (pmic_audio_vcodec_set_clock(drv_inst->pmic.handle, clockIn,
					clockFreq, vcodec_rate, NO_INVERT) !=
	    PMIC_SUCCESS) {
		DPRINTK
		    ("!!! ERROR: pmic_audio_vcodec_set_clock() failed !!!\n");
		goto CATCH;
	}

	/*! Configure the Voice CODEC's audio data bus protocol. */
	if (pmic_audio_set_protocol(drv_inst->pmic.handle, AUDIO_DATA_BUS_1,
				    NETWORK_MODE, masterSlaveModeSelect,
				    USE_4_TIMESLOTS) != PMIC_SUCCESS) {
		DPRINTK("!!! ERROR: pmic_audio_set_protocol() failed !!!\n");
		goto CATCH;
	}

	/*! Also default to using the first timeslot out of the 4 available in
	 *  each frame.
	 */
	if (pmic_audio_vcodec_set_rxtx_timeslot(drv_inst->pmic.handle,
						USE_TS0) != PMIC_SUCCESS) {
		DPRINTK("!!! ERROR: pmic_audio_vcodec_set_rxtx_timeslot() "
			"failed !!!\n");
		goto CATCH;
	}

	/*! Finally, enable the Voice CODEC. */
	if (pmic_audio_enable(drv_inst->pmic.handle) != PMIC_SUCCESS) {
		DPRINTK("!!! ERROR: pmic_audio_enable() failed !!!\n");
		goto CATCH;
	}

	DPRINTK("INFO: Voice CODEC enabled\n");

	FUNC_END;
	return 0;

      CATCH:
	FUNC_ERR;
	return -1;
}

/*!
 * This function initializes the Digital Audio MUX configuration by setting
 * Port1-Port7 to be in normal synchronous mode.
 *
 */
static void mxc_init_dam(void)
{
#ifndef MXC91231_EVB_PASS1
	dam_port port;

	FUNC_START;
	for (port = port_1; port < port_7; port++) {
		dam_select_mode(port, normal_mode);
		dam_set_synchronous(port, true);
	}

	FUNC_END;
#endif				/*MXC91231_EVB_PASS1 */
}

/*!
 * This function sets the Digital Audio MUX configuration for either the
 * Voice CODEC or the Stereo DAC. The standard configuration is as
 * follows:
 *
 * - SSI1 <=> Audio MUX Port1 <=> AD1/DAI0 <=> Voice CODEC
 * - SSI2  => Audio MUX Port2  => AD2/DAI1  => Stereo DAC
 *
 * The Voice CODEC must be connected to Audio MUX Port4/AD1 since this is the
 * only port that has the RXD signal connected to support audio recording.
 * The RXD signal is permanently grounded on Audio MUX Port5/AD2 on the mxc91131
 * and sc55112 platform.
 *
 * While it is technically possible to operate the Stereo DAC using SSI1 and
 * AD1/DAI0, we only support the configuration given above to keep things as
 * simple as possible.
 *
 * @param        drv_inst [in] audio driver instance
 * @return       0 in case of success
 *
 */
int mxc_setup_dam(const mxc_state_t * const drv_inst)
{
	FUNC_START;
	dam_port internal_port = drv_inst->dam_src;
	dam_port external_port = drv_inst->dam_dest;

	DPRINTK
	    ("INFO: Configuring Audio MUX internal Port%d and external Port%d\n",
	     internal_port + 1, external_port + 1);

	/*! Set synchronous mode since we don't really need to have separate
	 *  transmit and receive clocks. This also means that we will be
	 *  using only a 4-wire interface (TX, RX, BitClock, and Frame Sync).
	 */
	dam_set_synchronous(internal_port, true);
	dam_set_synchronous(external_port, true);

	/*! Use Normal Mode so that the Audio MUX will simply pass
	 *  everything between the SSI and PMIC without any changes.
	 */
	dam_select_mode(internal_port, normal_mode);
	dam_select_mode(external_port, normal_mode);

	/*! Configure the Bit Clock and Frame Sync connections based upon
	 *  who is acting as the master (the SSI or the PMIC). Note that we
	 *  only need to configure the transmit signals since we're using
	 *  synchronous mode.
	 */
	if (drv_inst->clock_master == MXC_AUDIO_CLOCKING_SSI_MASTER) {
		dam_select_TxClk_direction(internal_port, signal_in);
		dam_select_TxFS_direction(internal_port, signal_in);

		dam_select_TxClk_direction(external_port, signal_out);
		dam_select_TxFS_direction(external_port, signal_out);

		dam_select_TxClk_source(external_port, false, internal_port);
		dam_select_TxFS_source(external_port, false, internal_port);
	} else {
		dam_select_TxClk_direction(internal_port, signal_out);
		dam_select_TxFS_direction(internal_port, signal_out);

		dam_select_TxClk_direction(external_port, signal_in);
		dam_select_TxFS_direction(external_port, signal_in);

		dam_select_TxClk_source(internal_port, false, external_port);
		dam_select_TxFS_source(internal_port, false, external_port);
	}

	/*! Configure TX and RX connections between the internal and external
	 *  Audio MUX ports. Note that we only need to specify the RxD
	 *  connection and this will automatically also connect the TxD
	 */
	dam_select_RxD_source(external_port, internal_port);
	dam_select_RxD_source(internal_port, external_port);

	FUNC_END;
	return 0;
}

/*!
 * This function returns which input source(s) is (are) selected in the audio
 * chip (PMIC-dependent)
 * One or more input source(s) may be selected
 *
 * @param        drv_inst [in] info about the current transfer
 * @return       the selected input source(s), 0 if no input source selected
 *               or available
 *
 */
int pmic_get_src_in_selected(mxc_state_t * const drv_inst)
{
	int ret;

	down_interruptible(&drv_inst->mutex);
	ret = drv_inst->pmic.in_line;
	up(&drv_inst->mutex);

	return ret;
}

/*!
 * This function returns which output source(s) is (are) selected in
 * the audio chip (PMIC-dependent)
 * One or more output source(s) may be selected
 *
 * @param        drv_inst [in] info about the current transfer
 * @return       the selected output source(s),
 *               0 if no input source selected or available
 *
 */
int pmic_get_src_out_selected(mxc_state_t * const drv_inst)
{
	int ret;

	down_interruptible(&drv_inst->mutex);
	ret = drv_inst->pmic.out_line;
	up(&drv_inst->mutex);

	return ret;
}

/*!
 * This function selects the input source in the audio chip (PMIC)
 * Only one input source can be selected at a time
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        src_in   [in] the input source to select
 *               (if 0 then default all selected src are deselected)
 * @return       the selected input source, 0 if no input source selected
 *               or available
 *
 */
int pmic_select_src_in(mxc_state_t * const drv_inst, const int src_in)
{
	FUNC_START;
	down_interruptible(&drv_inst->mutex);
	drv_inst->pmic.in_line = src_in;
	up(&drv_inst->mutex);

	FUNC_END;
	return src_in;
}

/*!
 * This function selects the output source in the audio chip (PMIC-dependent)
 * One or more output source(s) may be simultaneously selected.
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        src_out  [in] the output source to select
 * @return       the selected output source, 0 if no input source selected
 *               or available
 *
 */
int pmic_select_src_out(mxc_state_t * const drv_inst, const int src_out)
{
	FUNC_START;
	down_interruptible(&drv_inst->mutex);
	drv_inst->pmic.out_line = src_out;
	up(&drv_inst->mutex);

	FUNC_END;
	return src_out;
}

/*!
 * This function selects the input device in the audio chip (PMIC-dependent)
 *
 * @param        drv_inst [in] audio driver instance
 * @param        vol      [in] the volume to set
 * @return       vol the volume effectively set
 *
 */
static int pmic_set_dev_in_volume(mxc_state_t * const drv_inst, const int vol)
{
	int vol_left = vol & 0xFF;
	int vol_left_db = 0;
	int vol_right = (vol & 0xFF00) >> 8;
	int vol_right_db = 0;
	int ret = vol;

	FUNC_START;
	/*! Clip the left and right channel volume levels to the OSS min/max
	 *  volume levels.
	 */
	if (vol_left > OSS_IN_VOLUME_MAX) {
		vol_left = OSS_IN_VOLUME_MAX;
	}
	if (vol_left < OSS_IN_VOLUME_MIN) {
		vol_left = OSS_IN_VOLUME_MIN;
	}
	if (vol_right > OSS_IN_VOLUME_MAX) {
		vol_right = OSS_IN_VOLUME_MAX;
	}
	if (vol_right < OSS_IN_VOLUME_MIN) {
		vol_right = OSS_IN_VOLUME_MIN;
	}

	/*! Convert volume levels to corresponding amplifier dB gain levels. */
	vol_left_db = CONVERT_IN_VOLUME_OSS_TO_HW(vol_left);
	vol_right_db = CONVERT_IN_VOLUME_OSS_TO_HW(vol_right);

	/* FIXME: debugging only */
	vol_left_db = vol_right_db = 0;

	DPRINTK("INFO: recording gain : left = %d dB, right = %d dB\n",
		vol_left_db, vol_right_db);

	/*! Also need to convert dB gain to matching enumerated value. */
	vol_left_db += MIC_GAIN_0DB;
	vol_right_db += MIC_GAIN_0DB;

	TRY(pmic_audio_vcodec_set_record_gain(drv_inst->pmic.handle,
					      AMP_OFF, vol_left_db,
					      CURRENT_TO_VOLTAGE, vol_right_db))

	    ret = ((vol_right << 8) & 0xFF00) | (vol_left & 0xFF);

	drv_inst->pmic.in_volume = ret;

	FUNC_END;
	return ret;

      CATCH:
	FUNC_ERR;
	return -1;
}

/*!
 * This function sets the volume of the output device in the audio chip
 * (PMIC-dependent)
 * PGA gain range is identical for Codec (Left) and StDAC (L+R).
 * Note: Only left value is taken into account
 *
 * @param        drv_inst [in] audio driver instance
 * @param        vol [in] the volume to set (L+R coded in a word)
 * @return       vol the volume effectively set, -1 if error
 *
 */
int pmic_set_dev_out_volume(mxc_state_t * const drv_inst, const int vol)
{
	int vol_left = (vol & 0xFF00) >> 8;
	int vol_right = vol & 0xFF;
	int vol_left_db = 0;
	int vol_right_db = 0;
	int delta = 0;
	int ret = 0;

	FUNC_START;
	DPRINTK("INFO: vol_left = %d, vol_right = %d\n", vol_left, vol_right);

	/*! Clip the left and right channel volume levels to the OSS min/max
	 *  volume levels.
	 */
	if (vol_left < OSS_OUT_VOLUME_MIN) {
		vol_left = OSS_OUT_VOLUME_MIN;
	}
	if (vol_left > OSS_OUT_VOLUME_MAX) {
		vol_left = OSS_OUT_VOLUME_MAX;
	}
	if (vol_right < OSS_OUT_VOLUME_MIN) {
		vol_right = OSS_OUT_VOLUME_MIN;
	}
	if (vol_right > OSS_OUT_VOLUME_MAX) {
		vol_right = OSS_OUT_VOLUME_MAX;
	}

	/*! Convert volume levels to corresponding amplifier dB gain levels. */
	vol_left_db = CONVERT_OUT_VOLUME_OSS_TO_HW(vol_left);
	vol_right_db = CONVERT_OUT_VOLUME_OSS_TO_HW(vol_right);

	DPRINTK("INFO: playback gain left = %d dB, right = %d dB\n",
		vol_left_db, vol_right_db);

	/*! Also need to convert dB gain to matching enumerated value. */
	vol_left_db = vol_left_db / 3 + OUTPGA_GAIN_0DB;
	vol_right_db = vol_right_db / 3 + OUTPGA_GAIN_0DB;

	delta = vol_left - vol_right;

	if (delta == 0) {
		/* Only update the actual hardware PGA gain right now if we've
		 * already opened an audio device.
		 *
		 * Otherwise, this just becomes a successful no-op and we'll save
		 * the volume level for later use after we've actually opened an
		 * audio device.
		 */
		if ((drv_inst->pmic.handle != (PMIC_AUDIO_HANDLE) NULL) &&
		    (pmic_audio_output_set_pgaGain(drv_inst->pmic.handle,
						   vol_left_db) !=
		     PMIC_SUCCESS)) {
			goto CATCH;
		}
	}
#ifdef CONFIG_MXC_PMIC_RR

	/*! The sc55112 PMIC does not have a balance control so just
	 *  use the average of the left and right channel volume levels.
	 */
	else {
		vol_left = (vol_left + vol_right) / 2;
		vol_right = vol_left;
		vol_left_db = (vol_left_db + vol_right_db) / 2;
		vol_right_db = vol_left_db;

		TRY(pmic_audio_output_set_pgaGain(drv_inst->pmic.handle,
						  vol_left_db))
	}

#endif				/* CONFIG_MXC_PMIC_RR */

	ret = ((vol_left << 8) & 0xFF00) | (vol_right & 0xFF);

	drv_inst->pmic.out_volume = ret;

	FUNC_END;
	return ret;

      CATCH:
	FUNC_ERR;
	return -1;
}

/*!
 * This function sets the output balance configuration (PMIC-dependent)
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        val     (0 to 49 = left bal, 50=disable,
 *                        51 to 100 right bal, balance from -21dB to 0)
 * @return        the new output balance, -1 if error
 *
 */
int pmic_set_output_balance(mxc_state_t * const drv_inst, const int val)
{
	/* FIXME: TODO: */
	return -1;
}

/*!
 * This function gets the output balance (PMIC-dependent)
 *
 * @param        drv_inst [in] info about the current transfer
 * @return       the current output balance, -1 if error
 *
 */
int pmic_get_output_balance(mxc_state_t * const drv_inst)
{
	int ret;

	down_interruptible(&drv_inst->mutex);
	ret = drv_inst->pmic.out_balance;
	up(&drv_inst->mutex);

	return ret;
}

/*!
 * This function checks if the Voice CODEC or Stereo DAC is still in use
 * (i.e., still in a playback or recording operation).
 *
 * @param        drv_inst [in] info about the current transfer
 * @return       0 if device can be disabled, 1 if still in use
 * @see          pmic_enable_out_device pmic_enable_in_device
 *
 */
static int pmic_check_device_still_in_use(mxc_state_t * const drv_inst)
{
	FUNC_START;
	if (drv_inst->device_busy)
		DPRINTK("INFO: Audio still in use\n");

	FUNC_END;
	return drv_inst->device_busy;
}

/*!
 * This function enables/disables the PMIC output section.
 *
 * Note that this has does not connect/disconnect the device to the SSI
 * port. Instead we select and configure the PMIC headset/speaker jacks
 * and output amplifiers here.
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        en_flag [in] 1 to enable, 0 to disable
 * @return       1 if device was enabled, 0 if device was disabled, -1 if error
 * @see          pmic_enable_in_device
 *
 */
int pmic_enable_out_device(mxc_state_t * const drv_inst, const int en_flag)
{
	int ret = 0;

	FUNC_START;
	if (en_flag) {
		DPRINTK("INFO: Connecting %s to audio output section\n",
			drv_inst->pmic.device == VOICE_CODEC ? "Voice CODEC" :
			"Stereo DAC");

		/*! Select the output port. */
		if (pmic_audio_output_set_port(drv_inst->pmic.handle,
					       drv_inst->pmic.out_line) !=
		    PMIC_SUCCESS) {
			DPRINTK
			    ("!!! ERROR: pmic_audio_output_set_port() failed !!!\n");
			FUNC_ERR;
			return -1;
		}

		/*! Set the output volume level. This will eventually call
		 *  pmic_audio_output_set_pgaGain() to update the PMIC audio
		 *  output PGA amplifier gain. */
		if (pmic_set_dev_out_volume(drv_inst, drv_inst->pmic.out_volume)
		    < 0) {
			DPRINTK
			    ("!!! ERROR: pmic_audio_output_set_pgaGain() failed !!!\n");
			FUNC_ERR;
			return -1;
		}

		ret = 1;
	} else if (!pmic_check_device_still_in_use(drv_inst)) {
		DPRINTK("INFO: Free output device\n");

		/*! Disable the output port. */
		if (pmic_audio_output_clear_port(drv_inst->pmic.handle,
						 drv_inst->pmic.out_line) !=
		    PMIC_SUCCESS) {
			DPRINTK
			    ("!!! ERROR: pmic_audio_output_clear_port() failed !!!\n");
			goto CATCH;
		}
	}

	FUNC_END;
	return ret;

      CATCH:
	FUNC_ERR;
	return -1;
}

/*!
 * This function enables/disables the PMIC input section.
 *
 * Note that this has does not connect/disconnect the device to the SSI
 * port. Instead we select and configure the PMIC microphone ports and
 * input amplifiers here.
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        en_flag [in] 1 to enable, 0 to disable
 * @return       1 if device was enabled, 0 if device was disabled, -1 if error
 * @see          pmic_enable_out_device
 *
 */
int pmic_enable_in_device(mxc_state_t * const drv_inst, const int en_flag)
{
	int ret = 0;

	FUNC_START;
	/*! Only the PMIC Voice CODEC supports a recording capability. */
	if (drv_inst->pmic.device != VOICE_CODEC) {
		DPRINTK
		    ("!!! ERROR: Attempting to connect the Stereo DAC to the "
		     "input section\n");
		FUNC_ERR;
		return -1;
	} else if (en_flag) {
		DPRINTK
		    ("INFO: Connecting Voice CODEC to the audio input section\n");

		/*! Select the input port. */
		if ((pmic_audio_vcodec_set_mic(drv_inst->pmic.handle, NO_MIC,
					       drv_inst->pmic.in_line) !=
		     PMIC_SUCCESS) ||
		    (pmic_audio_vcodec_set_mic_on_off(drv_inst->pmic.handle,
						      MICROPHONE_OFF,
						      MICROPHONE_ON) !=
		     PMIC_SUCCESS)) {
			DPRINTK("!!! ERROR: pmic_audio_vcodec_set_mic() or "
				"pmic_audio_vcodec_set_mic_on_off() failed !!!\n");
			FUNC_ERR;
			return -1;
		}

		/*! Also enable the appropriate microphone bias circuit. */
		DPRINTK("INFO: Enabling MIC_BIAS%d\n",
			(drv_inst->pmic.micBias == MIC_BIAS1) ? 1 : 2);
		pmic_audio_vcodec_enable_micbias(drv_inst->pmic.handle,
						 drv_inst->pmic.micBias);

		/*! Finally, set the input gain level. */
		if (pmic_set_dev_in_volume(drv_inst, drv_inst->pmic.in_volume) <
		    0) {
			FUNC_ERR;
			return -1;
		}

		ret = 1;
	} else if (!pmic_check_device_still_in_use(drv_inst)) {
		DPRINTK("INFO: Free input device\n");

		/*! Disable the input port. */
		if (pmic_audio_vcodec_set_mic_on_off(drv_inst->pmic.handle,
						     MICROPHONE_OFF,
						     MICROPHONE_OFF) !=
		    PMIC_SUCCESS) {
			DPRINTK
			    ("!!! ERROR: pmic_audio_vcodec_set_mic_on_off() failed "
			     "!!!\n");
			goto CATCH;
		}

		ret = 0;
	}

	FUNC_END;
	return ret;

      CATCH:
	FUNC_ERR;
	return -1;
}

/*!
 * This function selects the audio input/recording device.
 *
 * @param        drv_inst [in] audio device driver instance
 * @return       1 if output device was successfully opened,
 *               0 if output device could not be opened
 *
 */
int pmic_select_dev_in(mxc_state_t * const drv_inst)
{
	FUNC_START;
	if (drv_inst->pmic.device == STEREO_DAC) {
		/*! The Stereo DAC has no recording capability. */
		DPRINTK
		    ("!!! ERROR: attempt to record using the Stereo DAC !!!\n");
		goto CATCH;
	} else if (drv_inst->pmic.handle != (PMIC_AUDIO_HANDLE) NULL) {
		/* Do nothing, already connected to PMIC audio input device. */
		;
	}
#ifdef DRV_FOR_VIRTIO_MODEL

	/* Make sure that SSI2 and the Stereo DAC are not already in use because
	 * the Virtio model only connects the Digital Audio MUX Port 4 to the
	 * PMIC.
	 */
	else if (drv_inst->card->state[SSI2]->pmic.handle !=
		 (PMIC_AUDIO_HANDLE) NULL) {
		/* Fail the open request because Digital Audio MUX Port 4 is already
		 * in use by SSI2 and the Stereo DAC.
		 */
		DPRINTK
		    ("!!! ERROR: [virtio] Audio MUX port 4 already in use !!!\n");
		goto CATCH;
	}
#endif				/* DRV_FOR_VIRTIO_MODEL */

	else if (pmic_audio_open(&(drv_inst->pmic.handle),
				 drv_inst->pmic.device) != PMIC_SUCCESS) {
		DPRINTK("!!! ERROR: pmic_audio_open() failed !!!\n");
		goto CATCH;
	} else {
		DPRINTK("INFO: successful pmic_audio_open()\n");
	}

	FUNC_END;
	return 1;

      CATCH:
	FUNC_ERR;
	return 0;
}

/*!
 * This function opens the audio output device (either the Stereo DAC or the
 * Voice CODEC).
 *
 * @param        drv_inst [in] audio driver instance
 * @return       1 if output device was successfully opened,
 *               0 if output device could not be opened
 *
 */
int pmic_select_dev_out(mxc_state_t * const drv_inst)
{
	FUNC_START;
	if (drv_inst->pmic.handle != (PMIC_AUDIO_HANDLE) NULL) {
		/* Do nothing, already connected to PMIC audio output device. */
		printk
		    ("INFO: Do nothing, already connected to PMIC audio output device\n");
		;
	}
#ifdef DRV_FOR_VIRTIO_MODEL
	/* Make sure that the other SSI is not already in use because the Virtio
	 * model only connects the Digital Audio MUX Port 4 to the PMIC.
	 *
	 * Note that it is OK to simply check the status of both SSI buses here.
	 */
	else if (drv_inst->card->state[SSI1]->pmic.handle !=
		 (PMIC_AUDIO_HANDLE) NULL) {
		/* Fail the open request because Digital Audio MUX Port 4 is already
		 * in use by SSI1 and the Voice CODEC.
		 */
		goto CATCH;
	} else if (drv_inst->card->state[SSI2]->pmic.handle !=
		   (PMIC_AUDIO_HANDLE) NULL) {
		/* Fail the open request because Digital Audio MUX Port 4 is already
		 * in use by SSI2 and the Stereo DAC.
		 */
		goto CATCH;
	}
#endif				/* DRV_FOR_VIRTIO_MODEL */
	else if (pmic_audio_open(&(drv_inst->pmic.handle),
				 drv_inst->pmic.device) == PMIC_SUCCESS) {
		DPRINTK("INFO: successful pmic_audio_open()\n");
	} else {
		DPRINTK("!!! ERROR: pmic_audio_open() failed !!!\n");
		goto CATCH;
	}

	FUNC_END;
	return 1;

      CATCH:
	FUNC_ERR;
	return 0;
}

/*!
 * This function gets the output mixer configuration (PMIC-dependent)
 *
 * @param        drv_inst [in] info about the current transfer
 * @return        the current mixer configuration, -1 if error
 *
 */
int pmic_get_output_mixer(mxc_state_t * const drv_inst)
{
	/* FIXME: TODO: */
	return -1;
}

/*!
 * This function sets the output mixer configuration (PMIC-dependent)
 *
 * @param        drv_inst [in] info about the current transfer
 * @return        the current mixer configuration, -1 if error
 *
 */
int pmic_set_output_mixer(mxc_state_t * const drv_inst)
{
	/* FIXME: TODO: */
	return -1;
}

/*!
 * This function sets the output mono adder configuration.
 *
 * @param         drv_inst [in] info about the current transfer
 * @param         val [in] the working mode of the adder
 * @return        the new mono adder configuration, -1 if error
 * @see           pmic_audio_output_enable_mono_adder
 *
 */
int pmic_set_output_adder(mxc_state_t * const drv_inst, const int val)
{
	PMIC_AUDIO_MONO_ADDER_MODE mode = MONO_ADDER_OFF;

	FUNC_START;
	switch (val) {
	case (PMIC_AUDIO_ADDER_STEREO):

		mode = MONO_ADDER_OFF;
		break;

	case (PMIC_AUDIO_ADDER_STEREO_OPPOSITE):

		mode = STEREO_OPPOSITE_PHASE;
		break;

	case (PMIC_AUDIO_ADDER_MONO):

		mode = MONO_ADD_LEFT_RIGHT;
		break;

	case (PMIC_AUDIO_ADDER_MONO_OPPOSITE):

		mode = MONO_ADD_OPPOSITE_PHASE;
		break;

	default:
		DPRINTK("!!! ERROR: pmic_set_output_adder(): Invalid val "
			"parameter %d !!!\n", val);
		goto CATCH;
	}

	down_interruptible(&drv_inst->mutex);

	if (pmic_audio_output_enable_mono_adder(drv_inst->pmic.handle,
						mode) == PMIC_SUCCESS) {
		drv_inst->pmic.out_adder_mode = mode;
	} else {
		up(&drv_inst->mutex);
		goto CATCH;
	}

	up(&drv_inst->mutex);

	FUNC_END;
	return val;

      CATCH:
	FUNC_ERR;
	return -1;
}

/*!
 * This function gets the output mixer configuration (PMIC-dependent)
 *
 * @param        drv_inst [in] info about the current transfer
 * @return        the current mono adder configuration, -1 if error
 *
 */
int pmic_get_output_adder(mxc_state_t * const drv_inst)
{
	int ret;

	down_interruptible(&drv_inst->mutex);
	ret = drv_inst->pmic.out_adder_mode;
	up(&drv_inst->mutex);

	return ret;
}

/*!
 * This function returns true if output src is connected
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        src_mask [in] the playing source
 * @return       0 if not connected
 *
 */
static inline int is_src_out_connected(mxc_state_t * const drv_inst,
				       const int src_mask)
{
	int ret;

	ret = (drv_inst->pmic.out_line & src_mask);

	return ret;
}

/*!
 * This function sets the output volume of the given source (PMIC-dependent)
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        src_mask [in] the playing source to set
 * @param        val        [in] the new output volume
 * @return       the new output volume, -EINVAL if source inactive
 *
 */
int pmic_set_src_out_volume(mxc_state_t * const drv_inst, const int src_mask,
			    const int val)
{
	int out_volume = 0;

	down_interruptible(&drv_inst->mutex);

	/*! Check if the device is connected to our SSI */
	if (!is_src_out_connected(drv_inst, src_mask)) {
		out_volume = -EINVAL;
	} else {
		out_volume = pmic_set_dev_out_volume(drv_inst, val);
	}

	up(&drv_inst->mutex);

	return out_volume;
}

/*!
 * This function returns true if input src is connected
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        src_mask [in] the recording source to set
 * @return       0 if not connected
 *
 */
static inline int is_src_in_connected(mxc_state_t * const drv_inst,
				      const int src_mask)
{
	return (drv_inst->pmic.in_line & src_mask);
}

/*!
 * This function sets the input volume of the given source (PMIC-dependent)
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        src_mask [in] the recording source to set
 * @param        val        [in] the new output volume
 * @return       the new input volume, -EINVAL if source inactive
 *
 */
int pmic_set_src_in_volume(mxc_state_t * const drv_inst, const int src_mask,
			   const int val)
{
	int in_volume = 0;

	down_interruptible(&drv_inst->mutex);

	/*! Check if the device is connected to our SSI */
	if (!is_src_in_connected(drv_inst, src_mask)) {
		in_volume = -EINVAL;
	} else {
		in_volume = pmic_set_dev_in_volume(drv_inst, val);
	}

	up(&drv_inst->mutex);

	return in_volume;
}

/*!
 * This function gets the output volume of the given source (PMIC-dependent)
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        src_mask [in] the playing source to set
 * @return        the output volume, -EINVAL if source inactive
 *
 */
int pmic_get_src_out_volume(mxc_state_t * const drv_inst, const int src_mask)
{
	int out_volume = 0;

	down_interruptible(&drv_inst->mutex);

	/*! Check if the device is connected to our SSI */
	if (!is_src_out_connected(drv_inst, src_mask)) {
		out_volume = -EINVAL;
	} else {
		out_volume = drv_inst->pmic.out_volume;
	}

	up(&drv_inst->mutex);

	return out_volume;
}

/*!
 * This function gets the input volume of the given audio source.
 *
 * @param         drv_inst [in] audio driver instance
 * @param         src_mask [in] the recording source to set
 * @return        the input volume, -EINVAL if source inactive
 *
 */
int pmic_get_src_in_volume(mxc_state_t * const drv_inst, const int src_mask)
{
	int in_volume = 0;

	down_interruptible(&drv_inst->mutex);

	/*! Check if the device is connected to our SSI */
	if (!is_src_in_connected(drv_inst, src_mask)) {
		in_volume = -EINVAL;
	} else {
		in_volume = drv_inst->pmic.in_volume;
	}

	up(&drv_inst->mutex);

	return in_volume;
}

/*!
 * This function enables/disables the Voice CODEC digital filters.
 *
 * @param        drv_inst [in] info about the current transfer
 * @param        mask [in] the new PMIC Voice CODEC configuration
 * @return       the applied filter setting if successful, -1 if error
 *
 */
int pmic_set_codec_filter(mxc_state_t * const drv_inst, const int mask)
{
	int l_mask;

	FUNC_START;
	down_interruptible(&drv_inst->mutex);

	/*! Apply command only if the Voice CODEC is connected. */
	if (drv_inst->pmic.device == VOICE_CODEC) {
		l_mask = drv_inst->pmic.in_out_filter;
	} else {
		up(&drv_inst->mutex);
		goto CATCH;
	}

	/*! Update the Voice CODEC digital filter configuration. */
	if (mask & PMIC_CODEC_FILTER_HIGH_PASS_IN) {
		l_mask |= INPUT_HIGHPASS_FILTER;
	} else {
		l_mask &= ~(INPUT_HIGHPASS_FILTER);
	}

	if (mask & PMIC_CODEC_FILTER_HIGH_PASS_OUT) {
		l_mask |= OUTPUT_HIGHPASS_FILTER;
	} else {
		l_mask &= ~(OUTPUT_HIGHPASS_FILTER);
	}

	if (mask & PMIC_CODEC_FILTER_DITHERING) {
		l_mask |= DITHERING;
	} else {
		l_mask &= ~(DITHERING);
	}

	/*! Configure the new Voice CODEC digital filter state. */
	if (l_mask != drv_inst->pmic.in_out_filter) {
		/*! Actually need to update the PMIC Voice CODEC hardware. */
		if (pmic_audio_vcodec_set_config(drv_inst->pmic.handle, l_mask)
		    == PMIC_SUCCESS) {
			drv_inst->pmic.in_out_filter = l_mask;
		} else {
			up(&drv_inst->mutex);
			goto CATCH;
		}
	}

	up(&drv_inst->mutex);

	FUNC_END;
	return mask;

      CATCH:
	FUNC_ERR;
	return -1;
}

/*!
 * This function gets the current Voice CODEC digital filter state.
 *
 * @param         drv_inst [in] audio device driver instance
 * @return        the filter configuration of the Voice CODEC, -1 if error
 *
 */
int pmic_get_codec_filter(mxc_state_t * const drv_inst)
{
	int ret = -1;

	down_interruptible(&drv_inst->mutex);

	/*! Apply command only if the Voice CODEC is connected. */
	if (drv_inst->pmic.device == VOICE_CODEC) {
		ret = drv_inst->pmic.in_out_filter;
	}

	up(&drv_inst->mutex);

	return ret;
}

/*!
 * This function frees up the PMIC device handle. All associated audio
 * hardware components are also powered down/disabled if possible.
 *
 * @param        drv_inst [in] info about the current transfer
 *
 */
void pmic_close_audio(mxc_state_t * const drv_inst)
{
	const ssi_mod ssi_index = drv_inst->ssi_index;

	if (drv_inst->pmic.handle != (PMIC_AUDIO_HANDLE) NULL) {
		/*! Disable the SSI transmitter and/or the receiver. */
		if (drv_inst->play_cnt == 0)
			ssi_transmit_enable(ssi_index, false);
		if (drv_inst->rec_cnt == 0)
			ssi_receive_enable(ssi_index, false);

		/*! Note that there is currently no way to disable or turn off
		 *  the Audio MUX ports so we'll just leave them alone.
		 */

		/*! Check if we can actually shut everything down. */
		if ((drv_inst->rec_cnt == 0) && (drv_inst->play_cnt == 0)) {
			/*! Disable the SSI and all SSI-related interrupts. */
			ssi_enable(ssi_index, false);
			ssi_interrupt_disable(ssi_index,
					      MXC_SSI_INT_DISABLE_ALL);

			/*! Shut down the SSI clocks if we were running in master mode. */
			if (drv_inst->clock_master ==
			    MXC_AUDIO_CLOCKING_SSI_MASTER) {
				/* Disable the SSI internal clock source. */
				mxc_clks_disable((ssi_index ==
						  SSI1) ? SSI1_BAUD :
						 SSI2_BAUD);
			}

			/*! Close down the PMIC audio hardware components. Note that this
			 *  will also shut down the PMIC clocks if the PMIC was running in
			 *  master mode.
			 */
			if (pmic_audio_close(drv_inst->pmic.handle) ==
			    PMIC_SUCCESS) {
				/*! Reset the PMIC device handle. */
				drv_inst->pmic.handle =
				    (PMIC_AUDIO_HANDLE) NULL;

				DPRINTK("INFO: pmic_audio_close() succeeded\n");
			} else {
				DPRINTK
				    ("!!! ERROR: pmic_audio_close() failed !!!\n");
			}
		}
	}
}

/*!
 * This function resets all of the audio hardware components including
 * those on the PMIC. This function should only be called during driver
 * loading and unloading to make sure that the hardware is in a known
 * state.
 *
 */
void mxc_audio_hw_reset(void)
{
	DPRINTK("INFO: resetting all audio hardware components\n");

	/*! Disable both SSI1 and SSI2. This also flushes all data that may
	 *  still be in the FIFOs.
	 */
	ssi_enable(SSI1, false);
	ssi_enable(SSI2, false);

	ssi_transmit_enable(SSI1, false);
	ssi_receive_enable(SSI1, false);
	ssi_transmit_enable(SSI2, false);
	ssi_receive_enable(SSI2, false);

	/*! Disable all SSI interrupts. */
	ssi_interrupt_disable(SSI1, MXC_SSI_INT_DISABLE_ALL);
	ssi_interrupt_disable(SSI2, MXC_SSI_INT_DISABLE_ALL);

	/*! Disable the SSI internal clock source. */
	ssi_system_clock(SSI1, false);
	ssi_system_clock(SSI2, false);
	mxc_clks_disable(SSI1_BAUD);
	mxc_clks_disable(SSI2_BAUD);

	/*! Disable SSI Tx and Rx FIFOs. */
	ssi_tx_fifo_enable(SSI1, 0, false);
	ssi_tx_fifo_enable(SSI1, 1, false);
	ssi_rx_fifo_enable(SSI2, 0, false);
	ssi_rx_fifo_enable(SSI2, 1, false);

	/*! Reset all Audio MUX ports. */
	mxc_init_dam();

	/*! Reset the PMIC. */
	pmic_audio_reset_all();

	DPRINTK("INFO: done resetting of all audio hardware components\n");
}
