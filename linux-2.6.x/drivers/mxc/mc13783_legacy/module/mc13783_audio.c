/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

/*!
 * @file mc13783_audio.c
 * @brief This is the main file of mc13783 audio driver.
 *
 * @ingroup MC13783_AUDIO
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "../core/mc13783_external.h"
#include "../core/mc13783_config.h"

#include "mc13783_audio_defs.h"
#include "mc13783_audio.h"

#define MC13783_LOAD_ERROR_MSG		\
"mc13783 card was not correctly detected. Stop loading mc13783 Audio driver\n"

/*
 * Global variables
 */
static int mc13783_audio_major;
static int mc13783_audio_detected = 0;

/*!
 * Number of users waiting in suspendq
 */
static int swait = 0;

/*!
 * To indicate whether any of the audio devices are suspending
 */
static int suspend_flag = 0;

/*!
 * The suspendq is used to block application calls
 */
static wait_queue_head_t suspendq;

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(mc13783_audio_output_set_conf);
EXPORT_SYMBOL(mc13783_audio_output_get_conf);
EXPORT_SYMBOL(mc13783_audio_output_disable);
EXPORT_SYMBOL(mc13783_audio_output_headset_left_det);
EXPORT_SYMBOL(mc13783_audio_output_cdcout);
EXPORT_SYMBOL(mc13783_audio_output_bias_conf);
EXPORT_SYMBOL(mc13783_audio_output_config_mixer);
EXPORT_SYMBOL(mc13783_audio_output_mixer_input);
EXPORT_SYMBOL(mc13783_audio_output_pga);
EXPORT_SYMBOL(mc13783_audio_output_set_volume);
EXPORT_SYMBOL(mc13783_audio_output_get_volume);
EXPORT_SYMBOL(mc13783_audio_output_set_balance);
EXPORT_SYMBOL(mc13783_audio_output_get_balance);
EXPORT_SYMBOL(mc13783_audio_st_dac_enable);
EXPORT_SYMBOL(mc13783_audio_st_dac_select_ssi);
EXPORT_SYMBOL(mc13783_audio_st_dac_select_cli);
EXPORT_SYMBOL(mc13783_audio_st_dac_conf_pll);
EXPORT_SYMBOL(mc13783_audio_st_dac_protocol);
EXPORT_SYMBOL(mc13783_audio_st_dac_master_slave);
EXPORT_SYMBOL(mc13783_audio_st_dac_set_sample_rate);
EXPORT_SYMBOL(mc13783_audio_st_dac_get_sample_rate);
EXPORT_SYMBOL(mc13783_audio_st_dac_bit_clock_invert);
EXPORT_SYMBOL(mc13783_audio_st_dac_frame_sync_invert);
EXPORT_SYMBOL(mc13783_audio_st_dac_clk_en);
EXPORT_SYMBOL(mc13783_audio_st_dac_conf_network_mode);
EXPORT_SYMBOL(mc13783_audio_st_dac_conf_network_secondary);
EXPORT_SYMBOL(mc13783_audio_st_dac_output_mixing_gain);
EXPORT_SYMBOL(mc13783_audio_st_dac_network_nb_of_timeslots);
EXPORT_SYMBOL(mc13783_audio_st_dac_reset_filter);
EXPORT_SYMBOL(mc13783_audio_codec_enable);
EXPORT_SYMBOL(mc13783_audio_codec_select_ssi);
EXPORT_SYMBOL(mc13783_audio_codec_select_cli);
EXPORT_SYMBOL(mc13783_audio_codec_conf_pll);
EXPORT_SYMBOL(mc13783_audio_codec_master_slave);
EXPORT_SYMBOL(mc13783_audio_codec_bit_clock_invert);
EXPORT_SYMBOL(mc13783_audio_codec_frame_sync_invert);
EXPORT_SYMBOL(mc13783_audio_codec_clk_en);
EXPORT_SYMBOL(mc13783_audio_codec_protocol);
EXPORT_SYMBOL(mc13783_audio_codec_dithering_en);
EXPORT_SYMBOL(mc13783_audio_codec_trans_high_pass_filter_en);
EXPORT_SYMBOL(mc13783_audio_codec_receiv_high_pass_filter_en);
EXPORT_SYMBOL(mc13783_audio_codec_conf_network_mode);
EXPORT_SYMBOL(mc13783_audio_codec_conf_network_secondary);
EXPORT_SYMBOL(mc13783_audio_codec_output_mixing_gain);
EXPORT_SYMBOL(mc13783_audio_codec_set_sample_rate);
EXPORT_SYMBOL(mc13783_audio_codec_reset_filter);
EXPORT_SYMBOL(mc13783_audio_input_set_gain);
EXPORT_SYMBOL(mc13783_audio_input_get_gain);
EXPORT_SYMBOL(mc13783_audio_input_device);
EXPORT_SYMBOL(mc13783_audio_input_state_device);
EXPORT_SYMBOL(mc13783_audio_event_sub);
EXPORT_SYMBOL(mc13783_audio_event_unsub);
EXPORT_SYMBOL(mc13783_audio_loaded);
EXPORT_SYMBOL(mc13783_audio_set_phantom_ground);
EXPORT_SYMBOL(mc13783_audio_codec_set_latching_edge);
EXPORT_SYMBOL(mc13783_audio_codec_get_latching_edge);
EXPORT_SYMBOL(mc13783_audio_set_autodetect);

/*
 * Internal function
 */
int mc13783_audio_event(t_audio_int event, void *callback, bool sub);

/*!
 * This is the suspend of power management for the mc13783 audio API.
 * It supports SAVE and POWER_DOWN state.
 *
 * @param        dev            the device
 * @param        state          the state
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_audio_suspend(struct device *dev, u32 state, u32 level)
{
	switch (level) {
	case SUSPEND_DISABLE:
		suspend_flag = 1;
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		CHECK_ERROR(mc13783_audio_init_reg());
		break;
	}

	return 0;
};

/*!
 * This is the resume of power management for the mc13783 audio API.
 * It supports RESTORE state.
 *
 * @param        dev            the device
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int mc13783_audio_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_POWER_ON:
		/* nothing for mc13783 audio */
		break;
	case RESUME_RESTORE_STATE:
		break;
	case RESUME_ENABLE:
		/* nothing for mc13783 audio */
		suspend_flag = 0;
		while (swait > 0) {
			swait--;
			wake_up_interruptible(&suspendq);
		}
		break;
	}

	return 0;
};

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mc13783_audio_driver_ldm = {
	.name = "mc13783_audio",
	.bus = &platform_bus_type,
	.suspend = mc13783_audio_suspend,
	.resume = mc13783_audio_resume,
};

/*!
 * This is platform device structure for adding mc13783 audio device
 */
static struct platform_device mc13783_audio_ldm = {
	.name = "mc13783_audio",
	.id = 1,
};

/*
 * Audio mc13783 API
 */

/*!
 * This function initializes audio registers of mc13783.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_init_reg(void)
{
	CHECK_ERROR(mc13783_write_reg_value(PRIO_AUDIO, REG_AUDIO_RX_0,
					    DEF_AUDIO_RX_0));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_AUDIO, REG_AUDIO_RX_1,
					    DEF_AUDIO_RX_1));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_AUDIO, REG_AUDIO_TX,
					    DEF_AUDIO_TX));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
					    DEF_AUDIO_SSI));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_AUDIO, REG_AUDIO_CODEC,
					    DEF_AUDIO_CODEC));
	CHECK_ERROR(mc13783_write_reg_value(PRIO_AUDIO, REG_AUDIO_STEREO_DAC,
					    DEF_AUDIO_STDAC));
	return 0;
}

/*
 * output function API
 */

/*!
 * This function enable one output and sets source for this output.
 *
 * @param        out_type    	define the type of output
 * @param        src_audio	select source
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_set_conf(output_device out_type,
				  output_source src_audio)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	switch (out_type) {
	case od_earpiece:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ASPEN, true));
		if (src_audio == os_mixer_source) {
			CHECK_ERROR(mc13783_set_reg_bit
				    (PRIO_AUDIO, REG_AUDIO_RX_0, BIT_ASPSEL,
				     true));
		} else {
			CHECK_ERROR(mc13783_set_reg_bit
				    (PRIO_AUDIO, REG_AUDIO_RX_0, BIT_ASPSEL,
				     false));
		}
		break;
	case od_handsfree:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ALSPREF, true));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ALSPEN, true));
		if (src_audio == os_mixer_source) {
			CHECK_ERROR(mc13783_set_reg_bit
				    (PRIO_AUDIO, REG_AUDIO_RX_0, BIT_ALSPSEL,
				     true));
		} else {
			CHECK_ERROR(mc13783_set_reg_bit
				    (PRIO_AUDIO, REG_AUDIO_RX_0, BIT_ALSPSEL,
				     false));
		}
		break;
	case od_headset:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_AHSREN, true));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_AHSLEN, true));
		if (src_audio == os_mixer_source) {
			CHECK_ERROR(mc13783_set_reg_bit
				    (PRIO_AUDIO, REG_AUDIO_RX_0, BIT_AHSSEL,
				     true));
		} else {
			CHECK_ERROR(mc13783_set_reg_bit
				    (PRIO_AUDIO, REG_AUDIO_RX_0, BIT_AHSSEL,
				     false));
		}
		break;
	case od_line_out:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ARXREN, true));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ARXLEN, true));
		if (src_audio == os_mixer_source) {
			CHECK_ERROR(mc13783_set_reg_bit
				    (PRIO_AUDIO, REG_AUDIO_RX_0, BIT_ARXSEL,
				     true));
		} else {
			CHECK_ERROR(mc13783_set_reg_bit
				    (PRIO_AUDIO, REG_AUDIO_RX_0, BIT_ARXSEL,
				     false));
		}
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return 0;
}

/*!
 * This function reads configuration of outputs.
 *
 * @param        out_type    	select the type of output
 * @param        src_audio	return source selected
 * @param        out_state	return state of this output
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_get_conf(output_device out_type,
				  output_source * src_audio, bool * out_state)
{
	unsigned int reg_value;
	unsigned int enable_bit;
	unsigned int select_bit;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	switch (out_type) {
	case od_earpiece:
		enable_bit = BIT_ASPEN;
		select_bit = BIT_ASPSEL;
		break;
	case od_handsfree:
		enable_bit = BIT_ALSPEN;
		select_bit = BIT_ALSPSEL;
		break;
	case od_headset:
		enable_bit = BIT_AHSREN;
		select_bit = BIT_AHSSEL;
		break;
	case od_line_out:
		enable_bit = BIT_ARXREN;
		select_bit = BIT_ARXSEL;
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}

	CHECK_ERROR(mc13783_read_reg(PRIO_AUDIO, REG_AUDIO_RX_0, &reg_value));
	if ((reg_value & (0x1 << enable_bit)) != 0) {
		*out_state = true;
	} else {
		*out_state = false;
	}
	if ((reg_value & 0x1 << select_bit) != 0) {
		*src_audio = os_mixer_source;
	} else {
		*src_audio = os_codec_source;
	}
	return 0;
}

/*!
 * This function disables an output.
 *
 * @param        out_type    define the type of output
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_disable(output_device out_type)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	switch (out_type) {
	case od_earpiece:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ASPEN, false));
		break;
	case od_handsfree:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ALSPREF, false));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ALSPEN, false));
		break;
	case od_headset:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_AHSREN, false));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_AHSLEN, false));
		break;
	case od_line_out:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ARXREN, false));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ARXLEN, false));
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return 0;
}

/*!
 * This function configure the bias.
 *
 * @param        en_dis    Enable / Disable bias
 * @param        speed     select bias speed
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_bias_conf(bool en_dis, t_bspeed speed)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
					BIT_BIASED, en_dis));
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
					BIT_BIASSPEED, speed));
	return 0;
}

/*!
 * This function configure output CDC out enable.
 *
 * @param        en_dis    Enable / Disable CDCOUT
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_cdcout(bool en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
					BIT_CDCOUTEN, en_dis));
	return 0;
}

/*!
 * This function configure headset left channel detect.
 *
 * @param        en_dis    Enable / Disable headset left channel detect
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_headset_left_det(bool en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
					BIT_HSLDETEN, en_dis));
	return 0;
}

/*!
 * This function configures the output audio mixer.
 *
 * @param        conf_adder    select the configuration of adder (mono/stereo..)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_config_mixer(adder_config conf_adder)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_RX_1,
				     BITS_ADDER_CONF, (int)(conf_adder), 2);
}

/*!
 * This function enables or disables mixer input.
 *
 * @param        input    select an input
 * @param        en_dis   enable or disable this input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_en_dis_mixer_input(receive_signal input, bool en_dis)
{
	switch (input) {
	case rs_st_dac:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ADDSTDC, en_dis));
		break;
	case rs_codec:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ADDCDC, en_dis));
		break;
	case rs_line_in:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_0,
						BIT_ADDRXIN, en_dis));
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return 0;
}

/*!
 * This function enables/disables mixer input.
 *
 * @param        en_dis   enable or disable input
 * @param        input    select an input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_mixer_input(bool en_dis, receive_signal input)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_audio_output_en_dis_mixer_input(input, en_dis);
}

/*!
 * This function enables or disables amplifier input.
 *
 * @param        input    select an input
 * @param        en_dis   enable or disable this input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_pga_en_dis_amplifier(receive_signal input, bool en_dis)
{
	switch (input) {
	case rs_st_dac:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_1,
						BIT_AMPSTDC, en_dis));
		break;
	case rs_codec:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_1,
						BIT_AMPCDC, en_dis));
		break;
	case rs_line_in:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_1,
						BIT_AMPRXIN, en_dis));
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return 0;
}

/*!
 * This function enables / disables amplifier input.
 *
 * @param        en_dis   Enable or disable PGA
 * @param        input    select an input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_pga(bool en_dis, receive_signal input)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_audio_pga_en_dis_amplifier(input, en_dis);
}

/*!
 * This function sets the volume on selected amplifier.
 *
 * @param        volume_type    select the amplifier
 * @param        volume_value   value of the new volume
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_set_volume(receive_signal volume_type,
				    int volume_value)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	switch (volume_type) {
	case rs_st_dac:
		CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_RX_1,
						  BIT_STDAC_VOLUME,
						  (int)(volume_value), 4));
		break;
	case rs_codec:
		CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_RX_1,
						  BIT_CODEC_VOLUME,
						  (int)(volume_value), 4));
		break;
	case rs_line_in:
		CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_RX_1,
						  BIT_LINE_VOLUME,
						  (int)(volume_value), 4));
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	return 0;
}

/*!
 * This function gets the volume on selected amplifier.
 *
 * @param        volume_type    select the amplifier
 * @param        volume_value   pointer on value of the volume
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_get_volume(receive_signal volume_type,
				    int *volume_value)
{
	unsigned int reg_value;
	unsigned int volume_bit;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	switch (volume_type) {
	case rs_st_dac:
		volume_bit = BIT_STDAC_VOLUME;
		break;
	case rs_codec:
		volume_bit = BIT_CODEC_VOLUME;
		break;
	case rs_line_in:
		volume_bit = BIT_LINE_VOLUME;
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	CHECK_ERROR(mc13783_read_reg(PRIO_AUDIO, REG_AUDIO_RX_1, &reg_value));
	*volume_value = ((reg_value >> volume_bit) & 0x0F);
	return 0;
}

/*!
 * This function sets the balance.
 *
 * @param        channel_select  select the channel (left/right)
 * @param        attenuation	 value of attenuation
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_set_balance(channel channel_select, int attenuation)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_RX_1,
					BIT_L_R_BALANCE,
					(bool) (channel_select)));

	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_RX_1,
					  BITS_BALANCE, (int)(attenuation), 3));
	return 0;
}

/*!
 * This function gets the balance.
 *
 * @param        channel_select  select the channel (left/right)
 * @param        attenuation	 pointer on value of attenuation
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_get_balance(channel channel_select, int *attenuation)
{
	unsigned int reg_value;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	CHECK_ERROR(mc13783_read_reg(PRIO_AUDIO, REG_AUDIO_RX_1, &reg_value));
	*attenuation = ((reg_value >> BITS_BALANCE) & 0x07);
	channel_select = (channel) ((reg_value >> BIT_L_R_BALANCE) & 0x01);
	return 0;
}

/*!
 * This function enables/disables headset autodetection feature.
 * Autodetection is tightly related to headset amplifiers (bits 9,10 in
 * mc13783 register 36) For all operations on STDAC or on CODEC, autodetection
 * must be enabled.
 *
 * @param        val   enable or disable
 *
 * @return       This function returns 0 if successful, -EBUSY if device is
 *		 in suspended mode.
 */
int mc13783_audio_set_autodetect(int val)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}

	val = (val == 0) ? 0 : 1;

	mc13783_set_reg_bit(0, REG_AUDIO_RX_0, BIT_VAUDIOFORCE, val);
	mc13783_set_reg_bit(0, REG_AUDIO_RX_0, BIT_HSDETAUTOB, val);
	mc13783_set_reg_bit(0, REG_AUDIO_RX_0, BIT_HSDETEN, val);

	return 0;
}

/*
 * ST DAC configuration
 */

/*!
 * This function enables or disable the ST DAC of mc13783
 *
 * @param        en_dis   enable or disable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_enable(int en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}

	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_STEREO_DAC,
				   BIT_EN, en_dis);
}

/*!
 * This function selects input SSI bus for the ST DAC
 *
 * @param        ssi_in   select SSI bus used.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_select_ssi(ssi_bus ssi_in)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_STEREO_DAC, BIT_SSISEL,
				   ssi_in);
}

/*!
 * This function selects input clock for the ST DAC
 *
 * @param        cli_in   select clock input (CLIA or CLIB).
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_select_cli(clock_in cli_in)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_STEREO_DAC,
					BIT_CLKSEL, (bool) cli_in));
	return 0;
}

/*!
 * This function selects the PLL clock input frequencies
 *
 * @param        input_freq_pll   select configuration input clock PLL.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_conf_pll(int input_freq_pll)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_STEREO_DAC,
					  BITS_CONF_CLK, input_freq_pll, 3));
	return 0;
}

/*!
 * This function selects Master/Slave configuration of ST DAC
 *
 * @param        ms    select master/slave
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_master_slave(master_slave ms)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_STEREO_DAC, BIT_SM,
				   (bool) ms);
}

/*!
 * This function configures the bit clock format
 *
 * @param        bc_inv    if true the bit clock is inverted
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_bit_clock_invert(bool bc_inv)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_STEREO_DAC,
					BIT_BCLINV, bc_inv));
	CHECK_ERROR(mc13783_audio_st_dac_reset_filter());
	return 0;
}

/*!
 * This function configures the frame sync format
 *
 * @param        fs_inv    if true the frame sync is inverted
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_frame_sync_invert(bool fs_inv)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_STEREO_DAC, BIT_FSINV,
				   fs_inv);
}

/*!
 * This function configures the SPDIF mode
 *
 * @param        en_dis    Enable or disable SPDIF.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_spdif(bool en_dis)
{
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_STEREO_DAC, BIT_SPDIF,
				   en_dis);
}

/*!
 * This function configures the bus protocol selection
 *
 * @param        bus    type of if true the frame sync offset is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_protocol(t_bus_protocol bus)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_STEREO_DAC,
					  BIT_BUSPROTOCOL, (int)bus, 2));
	return 0;
}

/*!
 * This function configures clock enable of ST DAC
 *
 * @param        clk_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_clk_en(bool clk_en_des)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_STEREO_DAC, BIT_CLK_EN,
				   clk_en_des);
}

/*!
 * This function selects the sample rate of the ST DAC
 *
 * @param        sr    select sample rate
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_set_sample_rate(sample_rate sr)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_STEREO_DAC,
					  BITS_CONF_SR, (int)sr, 4));
	CHECK_ERROR(mc13783_audio_st_dac_reset_filter());
	return 0;
}

/*!
 * This function returns the sample rate of the ST DAC
 *
 * @param        sr    select sample rate
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_get_sample_rate(sample_rate * sr)
{
	unsigned int reg_value;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_read_reg(PRIO_AUDIO, REG_AUDIO_STEREO_DAC,
				     &reg_value));
	*sr = (sample_rate) ((reg_value >> BITS_CONF_SR) & 0x0F);
	return 0;
}

/*!
 * This function configures the number of timeslots in network mode of ST DAC
 *
 * @param        ts_number		number of timeslots
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_network_nb_of_timeslots(int ts_number)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
				     BITS_SLOTS0, (int)ts_number, 2);
}

/*!
 * This function configures the network mode of ST DAC
 *
 * @param        primary_timeslots      define the primary timeslots

 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_conf_network_mode(timeslots primary_timeslots)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
				     BITS_RXSLOTS0, (int)primary_timeslots, 2);
}

/*!
 * This function configures the network mixing mode of ST DAC
 *
 * @param        secondary_timeslots   define the secondary timeslots
 * @param        mix_mode		       configuration of mixing mode

 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_conf_network_secondary(timeslots secondary_timeslots,
						mixing_mode mix_mode)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
					  BITS_RXSECSLOTS0,
					  (int)secondary_timeslots, 2));

	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
					  BITS_RXSECGAIN, (int)mix_mode, 2));
	return 0;
}

/*!
 * This function configures the gain applied to the summed timeslots
 *
 * @param        mix_out_att    if true the gain of result of mixing is -6dB
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_output_mixing_gain(bool mix_out_att)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
				   BITS_STDAC_SUM, mix_out_att);
}

/*!
 * This function reset digital filter of ST dac
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_reset_filter(void)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_STEREO_DAC,
				   BIT_RESET, true);
}

/*
 * CODEC configuration
 */
/*!
 * This function enables or disable the CODEC of mc13783
 *
 * @param        en_dis   enable or disable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_enable(int en_dis)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}

	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC, BIT_EN, en_dis);
}

/*!
 * This function selects input SSI bus for the CODEC
 *
 * @param        ssi_in   select SSI bus used.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_select_ssi(ssi_bus ssi_in)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC,
				   BIT_SSISEL, ssi_in);
}

/*!
 * This function selects input clock for the CODEC
 *
 * @param        cli_in   select clock input (CLIA or CLIB).
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_select_cli(clock_in cli_in)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC, BIT_CLKSEL,
				   (bool) cli_in);
}

/*!
 * This function selects the PLL clock input frequencies
 *
 * @param        input_freq_pll   select input clock PLL.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_conf_pll(int input_freq_pll)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_CODEC, BITS_CONF_CLK,
				     input_freq_pll, 3);
}

/*!
 * This function selects Master/Slave configuration of CODEC
 *
 * @param        ms    select master/slave
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_master_slave(master_slave ms)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC, BIT_SM,
				   (bool) ms);
}

/*!
 * This function configures the bit clock format for CODEC
 *
 * @param        bc_inv    if true the bit clock is inverted
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_bit_clock_invert(bool bc_inv)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC, BIT_BCLINV,
					bc_inv));
	CHECK_ERROR(mc13783_audio_codec_reset_filter());
	return 0;

}

/*!
 * This function configures the frame sync format for CODEC
 *
 * @param        fs_inv    if true the frame sync is inverted
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_frame_sync_invert(bool fs_inv)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC, BIT_FSINV,
				   fs_inv);
}

/*!
 * This function configures the bus protocol selection
 *
 * @param        bus    type of if true the frame sync offset is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_protocol(t_bus_protocol bus)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_CODEC,
					  BIT_BUSPROTOCOL, (int)bus, 2));
	return 0;
}

/*!
 * This function configures clock enable of CODEC
 *
 * @param        clk_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_clk_en(bool clk_en_des)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC, BIT_CLK_EN,
				   clk_en_des);
}

/*!
 * This function enables dithering filter of CODEC
 *
 * @param        dith_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_dithering_en(bool dith_en_des)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC,
					BIT_CODEC_DITH, dith_en_des));
	CHECK_ERROR(mc13783_audio_codec_reset_filter());
	return 0;
}

/*!
 * This function enables transmit high pass dithering
 *
 * @param        thpf_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_trans_high_pass_filter_en(bool thpf_en_des)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC,
					BIT_CODEC_THPF, thpf_en_des));
	CHECK_ERROR(mc13783_audio_codec_reset_filter());
	return 0;
}

/*!
 * This function enables receive high pass dithering
 *
 * @param        rhpf_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_receiv_high_pass_filter_en(bool rhpf_en_des)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC,
					BIT_CODEC_RHPF, rhpf_en_des));
	CHECK_ERROR(mc13783_audio_codec_reset_filter());
	return 0;
}

/*!
 * This function configures the timeslots assignment
 *
 * @param        primary_timeslots      define the primary timeslots
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_conf_network_mode(timeslots primary_timeslots)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
				     BITS_CODEC_TXRXSLOTS0,
				     (int)primary_timeslots, 2);
}

/*!
 * This function configures the network mixing mode
 *
 * @param        secondary_rxtimeslots       define the secondary Rx timeslots
 * @param        secondary_txtimeslots       define the secondary Tx timeslots
 * @param        mix_mode		     configuration of mixing mode

 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_conf_network_secondary(timeslots secondary_rxtimeslots,
					       timeslots secondary_txtimeslots,
					       mixing_mode mix_mode)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
					  BITS_CODEC_TXSECSLOTS0,
					  (int)secondary_txtimeslots, 2));

	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
					  BITS_CODEC_RXSECSLOTS0,
					  (int)secondary_rxtimeslots, 2));

	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
					  BITS_CODEC_RXSECGAIN,
					  (int)mix_mode, 2));
	return 0;
}

/*!
 * This function configures the gain applied to the summed timeslots on CODEC
 *
 * @param        mix_out_att    if true the gain of result of mixing is -6dB
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_output_mixing_gain(bool mix_out_att)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_SSI_NETWORK,
				   BITS_CODEC_SUM, mix_out_att);
}

/*!
 * This function selects the sample rate of the CODEC
 *
 * @param        sr    select sample rate
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_set_sample_rate(sample_rate sr)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	switch (sr) {
	case (sr_8k):
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC,
						BIT_FS8K16K, false));
		break;
	case (sr_16k):
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC,
						BIT_FS8K16K, true));
		break;
	default:
		printk("Sampling frequency (%d) not supported by Codec\n", sr);
		return 1;
	}
	mc13783_audio_codec_reset_filter();
	return 0;
}

/*!
 * This function reset digital filter of codec
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_reset_filter(void)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_CODEC,
				   BIT_RESET, true);
}

/*
 * audio input functions
 */

/*!
 * This function sets the input gain..
 *
 * @param        left    left channel of input
 * @param        right   right channel of input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_input_set_gain(int left, int right)
{
	unsigned int gain_value = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	gain_value = ((right & 0x01F) | (left & 0x01F) << 5);

	CHECK_ERROR(mc13783_set_reg_value(PRIO_AUDIO, REG_AUDIO_TX,
					  BITS_TX_GAIN_R, gain_value, 10));

	CHECK_ERROR(mc13783_read_reg(PRIO_AUDIO, REG_AUDIO_TX, &gain_value));

	return 0;
}

/*!
 * This function gets the input gain.
 *
 * @param        left    return left channel input gain
 * @param        right   return right channel input gain
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_input_get_gain(int *left, int *right)
{
	unsigned int gain_value = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	CHECK_ERROR(mc13783_read_reg(PRIO_AUDIO, REG_AUDIO_TX, &gain_value));
	*right = (gain_value >> BITS_TX_GAIN_R) & 0x01F;
	*left = (gain_value >> BITS_TX_GAIN_L) & 0x01F;
	return 0;
}

/*!
 * This function enables or disables input.
 *
 * @param        in_dev    select the input
 * @param        en_dis   enable or disable input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_input_en_dis_device(input_device in_dev, bool en_dis)
{
	switch (in_dev) {
	case id_handset1:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_TX,
						BIT_TX_HANDSET1, en_dis));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_TX,
						BIT_TX_MC1BEN, en_dis));
		break;
	case id_handset2:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_TX,
						BIT_TX_HANDSET2, en_dis));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_TX,
						BIT_TX_MC1BEN, en_dis));
		break;
	case id_headset:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_TX,
						BIT_TX_HEADSET, en_dis));
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_TX,
						BIT_TX_MC2BEN, en_dis));
		break;
	case id_line_in:
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_AUDIO, REG_AUDIO_TX,
						BIT_TX_LINE_IN, en_dis));
		break;
	default:
		return ERROR_BAD_INPUT;
	}
	return 0;
}

/*!
 * This function enables / disables input.
 *
 * @param        en_dis    enable or disable input
 * @param        in_dev    select the input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_input_device(bool en_dis, input_device in_dev)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_audio_input_en_dis_device(in_dev, en_dis);
}

/*!
 * This function returns state of input.
 *
 * @param        in_dev    select the input
 * @param        in_en     Enable or disable input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_input_state_device(input_device in_dev, bool * in_en)
{
	int en_dis_bit = 0;
	unsigned int reg_value = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	switch (in_dev) {
	case id_handset1:
		en_dis_bit = BIT_TX_HANDSET1;
		break;
	case id_handset2:
		en_dis_bit = BIT_TX_HANDSET2;
		break;
	case id_headset:
		en_dis_bit = BIT_TX_HEADSET;
		break;
	case id_line_in:
		en_dis_bit = BIT_TX_LINE_IN;
		break;
	default:
		return ERROR_BAD_INPUT;
	}
	CHECK_ERROR(mc13783_read_reg(PRIO_AUDIO, REG_AUDIO_TX, &reg_value));
	if ((reg_value & (0x1 << en_dis_bit)) != 0) {
		*in_en = true;
	} else {
		*in_en = false;
	}
	return 0;
}

/*!
 * This function is to enable/disable phantom ground. This is used mainly
 * when connecting low-impedance audio devices (such as headset) If phantom
 * ground bit is not set, stereo output will be mono (L+R signals mixed on
 * both channels) instead of real stereo.
 *
 * @param        activate	if this parameter is 0, phantom ground is
 *				enabled (phantom ground is by default disabled)
 *				If it is 1, phantom ground is disabled.
 *
 * @return       		This function always returns 0.
 */
int mc13783_audio_set_phantom_ground(int activate)
{
	mc13783_set_reg_bit(0, REG_AUDIO_RX_0, BIT_HSPGDIS, activate);
	return 0;
}

/*!
 * This function is used to enable or disable latching edge delay in
 * mc13783 ICs v3.0 and up. For mc13783 version 2.x this function does not
 * modify any mc13783 register.
 *
 * @param        latching_edge           if true delay is added.
 *
 * @return       This function returns 0 if successful. If no mc13783
 *		 card was detected it returns -1.
 */
int mc13783_audio_codec_set_latching_edge(bool latching_edge)
{
	int mc13783_version = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	mc13783_version = mxc_mc13783_get_ic_version();

	if (mc13783_version == -1) {
		printk(KERN_WARNING "mc13783 card was not detected\n");
		return -1;
	}

	if (mc13783_version >= 30) {
		mc13783_set_reg_bit(0, REG_AUDIO_SSI_NETWORK,
				    BIT_LATCHING_EDGE, latching_edge);
	}

	return 0;
}

/*!
 * This function gets the current latching edge delay.
 *
 * @param        latching_edge           if true delay is added.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_get_latching_edge(bool * latching_edge)
{
	int mc13783_version = 0;

	if (suspend_flag == 1) {
		return -EBUSY;
	}

	mc13783_version = mxc_mc13783_get_ic_version();

	if (mc13783_version == -1) {
		printk(KERN_WARNING "mc13783 card was not detected\n");
		return -1;
	}

	if (mc13783_version >= 30) {
		mc13783_get_reg_bit(0, REG_AUDIO_SSI_NETWORK,
				    BIT_LATCHING_EDGE, latching_edge);
	}

	return 0;
}

/*!
 * This function is used to un/subscribe on audio event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 * @param        sub      	define if Un/subscribe event.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_event(t_audio_int event, void *callback, bool sub)
{
	type_event_notification audio_event;

	mc13783_event_init(&audio_event);
	audio_event.callback = callback;
	switch (event) {
	case AUDIO_IT_MC2BI:
		audio_event.event = EVENT_MC2BI;
		break;
	case AUDIO_IT_HSDETI:
		audio_event.event = EVENT_HSDETI;
		break;
	case AUDIO_IT_HSLI:
		audio_event.event = EVENT_HSLI;
		break;
	case AUDIO_IT_ALSPTHI:
		audio_event.event = EVENT_ALSPTHI;
		break;
	case AUDIO_IT_AHSSHORTI:
		audio_event.event = EVENT_AHSSHORTI;
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	if (sub == true) {
		CHECK_ERROR(mc13783_event_subscribe(audio_event));
	} else {
		CHECK_ERROR(mc13783_event_unsubscribe(audio_event));
	}
	return ERROR_NONE;
}

/*!
 * This function is used to subscribe on audio event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_event_sub(t_audio_int event, void *callback)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_audio_event(event, callback, true);
}

/*!
 * This function is used to un subscribe on audio event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_event_unsub(t_audio_int event, void *callback)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return mc13783_audio_event(event, callback, false);
}

/*
 * audio device function
 */

/*!
 * This function implements IOCTL controls on a mc13783 audio device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int mc13783_audio_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg)
{
	int write_value = 0;
	int read_value = 0;
	int type = 0;
	bool val = false;
	int R = 0, L = 0;
	output_source src_out, read_src_out;
	output_device out_type;
	input_device in_type = id_handset1;
	receive_signal receive_sig;
	sample_rate samp_rate, samp_rate_read;

	if (_IOC_TYPE(cmd) != 'A')
		return -ENOTTY;

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}

	switch (cmd) {
	case INIT_AUDIO:
		TRACEMSG_AUDIO(_K_D("Init audio"));
		/* initialization all audio register */
		CHECK_ERROR(mc13783_audio_init_reg());
		break;

#ifdef  __TEST_CODE_ENABLE__

	case CHECK_VOLUME:
		type = rs_st_dac;
		for (receive_sig = 0; receive_sig <= rs_line_in; receive_sig++) {
			for (write_value = 0; write_value <= 0x0f;
			     write_value++) {
				read_value = 0;
				CHECK_ERROR(mc13783_audio_output_set_volume
					    (receive_sig, write_value));

				CHECK_ERROR(mc13783_audio_output_get_volume
					    (receive_sig, &read_value));

				if (write_value != read_value) {
					TRACEMSG_AUDIO(_K_I
						       ("Volume control error"));
					TRACEMSG_AUDIO(_K_I
						       ("Out %d value %d \n"),
						       receive_sig, read_value);
					return ERROR_INUSE;
				}
			}
		}
		CHECK_ERROR(mc13783_audio_output_set_balance(c_left, 5));
		CHECK_ERROR(mc13783_audio_output_get_balance(c_left,
							     &read_value));
		if (read_value != 5) {
			TRACEMSG_AUDIO(_K_I("Output balance error"));
			return ERROR_INUSE;
		}

		CHECK_ERROR(mc13783_audio_init_reg());
		break;
	case CHECK_OUT:
		read_value = 0;
		write_value = 0;
		src_out = os_mixer_source;
		out_type = od_earpiece;
		read_src_out = os_codec_source;
		for (out_type = 0; out_type <= od_line_out; out_type++) {
			CHECK_ERROR(mc13783_audio_output_set_conf(out_type,
								  src_out));

			CHECK_ERROR(mc13783_audio_output_get_conf(out_type,
								  &read_src_out,
								  &val));
			if (val == false) {
				TRACEMSG_AUDIO(_K_I
					       ("Output %d enable control error"),
					       out_type);
				return ERROR_INUSE;
			}
			if (read_src_out != src_out) {
				TRACEMSG_AUDIO(_K_I
					       ("Output %d enable control error"),
					       out_type);
				return ERROR_INUSE;
			}
			CHECK_ERROR(mc13783_audio_output_disable(out_type));
			CHECK_ERROR(mc13783_audio_output_get_conf(out_type,
								  &read_src_out,
								  &val));
			if (val == true) {
				TRACEMSG_AUDIO(_K_I("Out %d EN control error"),
					       out_type);

				return ERROR_INUSE;
			}
		}
		CHECK_ERROR(mc13783_audio_output_bias_conf(true, true));
		CHECK_ERROR(mc13783_audio_output_bias_conf(false, false));
		CHECK_ERROR(mc13783_audio_output_cdcout(true));
		CHECK_ERROR(mc13783_audio_output_cdcout(false));
		CHECK_ERROR(mc13783_audio_output_headset_left_det(false));
		CHECK_ERROR(mc13783_audio_output_config_mixer(ac_stereo));
		CHECK_ERROR(mc13783_audio_output_mixer_input(rs_st_dac, true));
		CHECK_ERROR(mc13783_audio_output_pga(true, rs_st_dac));
		CHECK_ERROR(mc13783_audio_output_pga(false, rs_st_dac));
		CHECK_ERROR(mc13783_audio_init_reg());
		break;
	case CHECK_IN:
		read_value = 0;
		write_value = 0;
		in_type = id_handset1;

		for (in_type = 0; in_type <= id_headset; in_type++) {
			CHECK_ERROR(mc13783_audio_input_device(true, in_type));
			CHECK_ERROR(mc13783_audio_input_state_device(in_type,
								     &val));
			if (val == false) {
				TRACEMSG_AUDIO(_K_I("Input %d enable \
                                			control error"), in_type);
				return ERROR_INUSE;
			}
			CHECK_ERROR(mc13783_audio_input_device(false, in_type));
			CHECK_ERROR(mc13783_audio_input_state_device(in_type,
								     &val));
			if (val == true) {
				TRACEMSG_AUDIO(_K_I("Input %d enable \
                                			control error"), in_type);
				return ERROR_INUSE;
			}
		}
		R = 5;
		L = 0xA;
		CHECK_ERROR(mc13783_audio_input_set_gain(R, L));
		R = 0;
		L = 0;
		CHECK_ERROR(mc13783_audio_input_get_gain(&R, &L));
		if ((R != 5) || (L != 0xA)) {
			TRACEMSG_AUDIO(_K_I("Input Gain control error"));
			return ERROR_INUSE;
		}
		CHECK_ERROR(mc13783_audio_init_reg());
		break;
	case CHECK_CODEC:
		CHECK_ERROR(mc13783_audio_codec_enable(true));
		CHECK_ERROR(mc13783_audio_codec_select_ssi(sb_ssi_1));
		CHECK_ERROR(mc13783_audio_codec_select_cli(ci_cli_a));
		CHECK_ERROR(mc13783_audio_codec_conf_pll(0));
		CHECK_ERROR(mc13783_audio_codec_master_slave(ms_slave));
		CHECK_ERROR(mc13783_audio_codec_bit_clock_invert(true));
		CHECK_ERROR(mc13783_audio_codec_frame_sync_invert(true));
		CHECK_ERROR(mc13783_audio_codec_protocol(bp_normal));
		CHECK_ERROR(mc13783_audio_codec_clk_en(true));
		CHECK_ERROR(mc13783_audio_codec_conf_network_mode(t_mode_3));
		CHECK_ERROR(mc13783_audio_codec_output_mixing_gain(true));
		CHECK_ERROR(mc13783_audio_codec_dithering_en(true));
		CHECK_ERROR(mc13783_audio_codec_receiv_high_pass_filter_en
			    (true));
		CHECK_ERROR(mc13783_audio_codec_dithering_en(false));
		CHECK_ERROR(mc13783_audio_codec_receiv_high_pass_filter_en
			    (false));
		CHECK_ERROR(mc13783_audio_init_reg());
		break;
	case CHECK_STDAC:
		CHECK_ERROR(mc13783_audio_st_dac_enable(true));
		CHECK_ERROR(mc13783_audio_st_dac_select_ssi(sb_ssi_0));
		CHECK_ERROR(mc13783_audio_st_dac_select_cli(ci_cli_a));
		CHECK_ERROR(mc13783_audio_st_dac_conf_pll(0));
		CHECK_ERROR(mc13783_audio_st_dac_master_slave(ms_slave));
		CHECK_ERROR(mc13783_audio_st_dac_bit_clock_invert(true));
		CHECK_ERROR(mc13783_audio_st_dac_frame_sync_invert(true));
		CHECK_ERROR(mc13783_audio_st_dac_protocol(bp_normal));
		CHECK_ERROR(mc13783_audio_st_dac_clk_en(true));
		CHECK_ERROR(mc13783_audio_st_dac_conf_network_mode(t_mode_0));
		CHECK_ERROR(mc13783_audio_st_dac_output_mixing_gain(true));
		CHECK_ERROR(mc13783_audio_st_dac_conf_network_secondary
			    (t_mode_1, mm_att_12dB));
		CHECK_ERROR(mc13783_audio_st_dac_spdif(true));
		for (samp_rate = sr_8k; samp_rate <= sr_96k; samp_rate++) {
			samp_rate_read = sr_8k;
			CHECK_ERROR(mc13783_audio_st_dac_set_sample_rate
				    (samp_rate));
			CHECK_ERROR(mc13783_audio_st_dac_get_sample_rate
				    (&samp_rate_read));
			if (samp_rate_read != samp_rate) {
				TRACEMSG_AUDIO(_K_I("ST DAC control error"));
				return ERROR_INUSE;
			}
		}

		CHECK_ERROR(mc13783_audio_codec_set_latching_edge(true));
		CHECK_ERROR(mc13783_audio_codec_get_latching_edge(&val));
		if (val == false) {
			TRACEMSG_AUDIO(_K_I
				       ("Unable to set latching edge to true: %d"),
				       val);
			return ERROR_INUSE;
		}
		CHECK_ERROR(mc13783_audio_codec_set_latching_edge(false));
		CHECK_ERROR(mc13783_audio_codec_get_latching_edge(&val));
		if (val == true) {
			TRACEMSG_AUDIO(_K_I
				       ("Unable to set latching est to false: %d"),
				       val);
			return ERROR_INUSE;
		}

		CHECK_ERROR(mc13783_audio_init_reg());
		break;

#endif				/* __TEST_CODE_ENABLE__ */

	case CONFIG_AUDIO:
		CHECK_ERROR(mc13783_audio_init_reg());
		break;
	default:
		TRACEMSG_AUDIO(_K_D("%d unsupported ioctl command"), (int)cmd);
		return -EINVAL;
	}
	return ERROR_NONE;
}

/*!
 * This function implements the open method on a mc13783 audio device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_audio_open(struct inode *inode, struct file *file)
{
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	return ERROR_NONE;
}

/*!
 * This function implements the release method on a mc13783 audio device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_audio_release(struct inode *inode, struct file *file)
{
	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	return ERROR_NONE;
}

/*!
 * This structure contains pointers to the normal POSIX operations
 * available on this driver.
 */
static struct file_operations mc13783_audio_fops = {
	.owner = THIS_MODULE,
	.ioctl = mc13783_audio_ioctl,
	.open = mc13783_audio_open,
	.release = mc13783_audio_release,
};

int mc13783_audio_loaded(void)
{
	return mc13783_audio_detected;
}

/*
 * Init and Exit
 */

static int __init mc13783_audio_init(void)
{
	int ret = 0;

	if (mc13783_core_loaded() == 0) {
		printk(KERN_INFO MC13783_LOAD_ERROR_MSG);
		return -1;
	}

	mc13783_audio_major = register_chrdev(0, "mc13783_audio",
					      &mc13783_audio_fops);
	if (mc13783_audio_major < 0) {
		TRACEMSG_AUDIO(_K_D("Unable to get a major for mc13783_audio"));
		return -1;
	}
	init_waitqueue_head(&suspendq);

	devfs_mk_cdev(MKDEV(mc13783_audio_major, 1),
		      S_IFCHR | S_IRUGO | S_IWUSR, "mc13783_audio");
	ret = driver_register(&mc13783_audio_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&mc13783_audio_ldm);
		if (ret != 0) {
			driver_unregister(&mc13783_audio_driver_ldm);
		} else {
			mc13783_audio_detected = 1;
			printk(KERN_INFO "mc13783 Audio loaded\n");
		}
	}

	return ret;
}

static void __exit mc13783_audio_exit(void)
{
	devfs_remove("mc13783_audio");

	driver_unregister(&mc13783_audio_driver_ldm);
	platform_device_unregister(&mc13783_audio_ldm);
	unregister_chrdev(mc13783_audio_major, "mc13783_audio");
	TRACEMSG_AUDIO(("mc13783_audio : successfully unloaded"));
}

/*
 * Module entry points
 */

module_init(mc13783_audio_init);
module_exit(mc13783_audio_exit);

MODULE_DESCRIPTION("mc13783_audio driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
