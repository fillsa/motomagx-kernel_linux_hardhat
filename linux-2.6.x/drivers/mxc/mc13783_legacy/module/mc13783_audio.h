/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/* mc13783_audio_st_dac_conf_network_mode
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
 * @defgroup MC13783_AUDIO mc13783 Audio Driver
 * @ingroup MC13783DRVRS
 */

/*!
 * @file mc13783_audio.h
 * @brief This is the header of mc13783 audio driver.
 *
 * @ingroup MC13783_AUDIO
 */

#ifndef         __MC13783_AUDIO_H__
#define         __MC13783_AUDIO_H__

/*
 * Includes
 */
#include <asm/ioctl.h>

#define         INIT_AUDIO                      _IOWR('A',1, int)
#define         CHECK_VOLUME                    _IOWR('A',2, int)
#define         CHECK_OUT                       _IOWR('A',3, int)
#define         CHECK_IN                        _IOWR('A',4, int)
#define         CHECK_CODEC                     _IOWR('A',5, int)
#define         CHECK_STDAC                     _IOWR('A',6, int)
#define         CONFIG_AUDIO                    _IOWR('A',7, int)

#ifdef __ALL_TRACES__
#define TRACEMSG_AUDIO(fmt,args...)  printk(fmt,##args)
#else				/* __ALL_TRACES__ */
#define TRACEMSG_AUDIO(fmt,args...)
#endif				/* __ALL_TRACES__ */

/*!
 * This enumeration represents all mc13783 audio output .
 */
typedef enum {
	/*!
	 * Phone EARPIECE (ASP) output
	 */
	od_earpiece,
	/*!
	 * The loudspeaker for HANDSFREE or ringing (ALSP) output
	 */
	od_handsfree,
	/*!
	 * The stereo headset (AHSR and AHSL) output
	 */
	od_headset,
	/*!
	 * The stereo line output
	 */
	od_line_out,
} output_device;

/*!
 * define input device
 */
typedef enum {
	/*!
	 * handset 1
	 */
	id_handset1,
	/*!
	 * handset 2
	 */
	id_handset2,
	/*!
	 * headset
	 */
	id_headset,
	/*!
	 * line in
	 */
	id_line_in,
} input_device;

/*!
 * This enumeration defines type of audio source for an output
 */
typedef enum {
	/*!
	 * The CODEC source
	 */
	os_codec_source,
	/*!
	 * The mixer source
	 */
	os_mixer_source,
} output_source;

/*!
 * This enumeration defines all amplifiers
 */
typedef enum {
	/*!
	 * ST DAC
	 */
	rs_st_dac,
	/*!
	 * CODEC
	 */
	rs_codec,
	/*!
	 * line in
	 */
	rs_line_in,
} receive_signal;

/*!
 * This enumeration defines type of configuration of mixer
 */
typedef enum {
	/*!
	 * configure mixer in stereo mode
	 */
	ac_stereo,
	/*!
	 * configure mixer in stereo opposite mode
	 */
	ac_stereo_opposit,
	/*!
	 * configure mixer in mono mode
	 */
	ac_mono,
	/*!
	 * configure mixer in mono opposite mode
	 */
	ac_mono_opposit,
} adder_config;

/*!
 * define channel
 */
typedef enum {
	/*!
	 * define right channel
	 */
	c_right,

	/*!
	 * define left channel
	 */
	c_left,
} channel;

/*!
 * define input clock
 */
typedef enum {
	/*!
	 * cli_a input clock
	 */
	ci_cli_a,
	/*!
	 * cli_b input clock
	 */
	ci_cli_b,
} clock_in;

/*!
 * SSI digital audio I/O paths
 */
typedef enum {
	/*!
	 * SSI 0 FS1, BLC1 RX1 digital audio I/O paths
	 */
	sb_ssi_0,
	/*!
	 * SSI 1 FS2, BLC2 RX2 digital audio I/O paths
	 */
	sb_ssi_1,
} ssi_bus;

/*!
 * define master slave mode
 */
typedef enum {
	/*!
	 * configuration in master mode
	 */
	ms_master,
	/*!
	 * configuration in slave mode
	 */
	ms_slave,
} master_slave;

/*!
 * define master slave mode
 */
typedef enum {
	/*!
	 * bias speed in slow mode
	 */
	sp_slow,
	/*!
	 * bias speed in fast mode
	 */
	sp_fast,
} t_bspeed;

/*!
 * define bus protocol selection
 */
typedef enum {
	/*!
	 * protocol normal mode
	 */
	bp_normal,
	/*!
	 * protocol network mode
	 */
	bp_network,
	/*!
	 * protocol i2s mode
	 */
	bp_i2s,

} t_bus_protocol;

/*!
 * define all sample rate supported by the ST DAC
 */
typedef enum {
	/*!
	 * sample rate 8 kHz
	 */
	sr_8k,
	/*!
	 * sample rate 11025 Hz
	 */
	sr_11025,
	/*!
	 * sample rate 12 kHz
	 */
	sr_12k,
	/*!
	 * sample rate 16 kHz
	 */
	sr_16k,
	/*!
	 * sample rate 22050 Hz
	 */
	sr_22050,
	/*!
	 * sample rate 24 kHz
	 */
	sr_24k,
	/*!
	 * sample rate 32 kHz
	 */
	sr_32k,
	/*!
	 * sample rate 44100 Hz
	 */
	sr_44100,
	/*!
	 * sample rate 48 kHz
	 */
	sr_48k,
	/*!
	 * sample rate 64 kHz
	 */
	sr_64k,
	/*!
	 * sample rate 96 kHz
	 */
	sr_96k,
} sample_rate;

/*!
 * define timeslots in network mode
 */
typedef enum {
	/*!
	 * select TS0 and TS1 in stereo or TS0 in mono
	 */
	t_mode_0,
	/*!
	 * select TS2 and TS3 in stereo or TS1 in mono
	 */
	t_mode_1,
	/*!
	 * select TS4 and TS5 in stereo or TS2 in mono
	 */
	t_mode_2,
	/*!
	 * select TS6 and TS7 in stereo or TS3 in mono
	 */
	t_mode_3,
} timeslots;

/*!
 * define timeslots in network mode
 */
typedef enum {
	/*!
	 * disable the mixing
	 */
	mm_no_mixing,
	/*!
	 * mixing without attenuation
	 */
	mm_att_0dB,
	/*!
	 * mixing with -6dB attenuation
	 */
	mm_att_6dB,
	/*!
	 * mixing with -12dB attenuation
	 */
	mm_att_12dB,
} mixing_mode;

/*!
 * This enumeration define all audio interrupts
 */
typedef enum {
	/*!
	 * microphone bias 2 detect
	 */
	AUDIO_IT_MC2BI = 0,
	/*!
	 * Headset attach
	 */
	AUDIO_IT_HSDETI,
	/*!
	 * Stereo headset detect
	 */
	AUDIO_IT_HSLI,
	/*!
	 * Thermal shutdown Alsp
	 */
	AUDIO_IT_ALSPTHI,
	/*!
	 * Short circuit on Ahs outputs
	 */
	AUDIO_IT_AHSSHORTI,
} t_audio_int;

/*
 * Audio mc13783 API
 */

/*!
 * This function enables one output and set source for this output.
 *
 * @param        out_type    	define the type of output
 * @param        src_audio	select source
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_set_conf(output_device out_type,
				  output_source src_audio);

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
				  output_source * src_audio, bool * out_state);

/*!
 * This function disables an output.
 *
 * @param        out_type    define the type of output
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_disable(output_device out_type);

/*!
 * This function configures the bias.
 *
 * @param        en_dis    Enable / disable bias
 * @param        speed     select bias speed
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_bias_conf(bool en_dis, t_bspeed speed);

/*!
 * This function configures output CDC out enable.
 *
 * @param        en_dis    Enable / disable CDCOUT
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_cdcout(bool en_dis);

/*!
 * This function configures headset left channel detect.
 *
 * @param        en_dis    Enable / disable headset left channel detect
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_headset_left_det(bool en_dis);

/*!
 * This function configures the output audio mixer.
 *
 * @param        conf_adder    select the configuration of adder (mono/stereo...)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_config_mixer(adder_config conf_adder);

/*!
 * This function enables/disables mixer input.
 *
 * @param        en_dis   enable or disable input
 * @param        input    select an input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_mixer_input(bool en_dis, receive_signal input);

/*!
 * This function enables/disables amplifier input.
 *
 * @param        en_dis   enable or disable input
 * @param        input    select an input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_pga(bool en_dis, receive_signal input);

/*!
 * This function sets the volume on selected amplifier.
 *
 * @param        volume_type    select the amplifier
 * @param        volume_value   value of the new volume
 *
 * @return       This function returns 0 if successful.
 */

int mc13783_audio_output_set_volume(receive_signal volume_type,
				    int volume_value);

/*!
 * This function gets the volume on selected amplifier.
 *
 * @param        volume_type    select the amplifier
 * @param        volume_value   pointer on value of the volume
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_get_volume(receive_signal volume_type,
				    int *volume_value);

/*!
 * This function sets the balance.
 *
 * @param        channel_select  select the channel (left/right)
 * @param        attenuation	 value of attenuation
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_set_balance(channel channel_select, int attenuation);

/*!
 * This function gets the balance.
 *
 * @param        channel_select  select the channel (left/right)
 * @param        attenuation	 pointer on value of attenuation
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_output_get_balance(channel channel_select, int *attenuation);

/*
 * ST DAC
 */

/*!
 * This function enables or disables the ST DAC of mc13783
 *
 * @param        en_dis   enable or disable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_enable(int en_dis);

/*!
 * This function selects input SSI bus for the ST DAC
 *
 * @param        ssi_in   select SSI bus used.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_select_ssi(ssi_bus ssi_in);

/*!
 * This function selects input clock for the ST DAC
 *
 * @param        cli_in   select clock input (CLIA or CLIB).
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_select_cli(clock_in cli_in);

/*!
 * This function selects the PLL clock input frequencies
 *
 * @param        input_freq_pll   select input clock PLL.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_conf_pll(int input_freq_pll);

/*!
 * This function selects Master/Slave configuration of ST DAC
 *
 * @param        ms    select master/slave
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_master_slave(master_slave ms);

/*!
 * This function selects the sample rate of the ST DAC
 *
 * @param        sr    select sample rate
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_set_sample_rate(sample_rate sr);

/*!
 * This function returns the sample rate of the ST DAC
 *
 * @param        sr    select sample rate
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_get_sample_rate(sample_rate * sr);

/*!
 * This function configures the bit clock format
 *
 * @param        bc_inv    if true the bit clock is inverted
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_bit_clock_invert(bool bc_inv);

/*!
 * This function configures the frame sync format
 *
 * @param        fs_inv    if true the frame sync is inverted
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_frame_sync_invert(bool fs_inv);

/*!
 * This function configures the SPDIF mode
 *
 * @param        en_dis    Enable or disable SPDIF.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_spdif(bool en_dis);

/*!
 * This function configures the bus protocol selection
 *
 * @param        bus    type of if true the frame sync offset is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_protocol(t_bus_protocol bus);

/*!
 * This function configures clock enable of ST DAC
 *
 * @param        clk_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_clk_en(bool clk_en_des);

/*!
 * This function configures the network mode of ST DAC
 *
 * @param        primary_timeslots      define the primary timeslots

 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_conf_network_mode(timeslots primary_timeslots);

/*!
 * This function configures the network mixing mode of ST DAC
 *
 * @param        secondary_timeslots   define the secondary timeslots
 * @param        mix_mode		       configuration of mixing mode

 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_conf_network_secondary(timeslots secondary_timeslots,
						mixing_mode mix_mode);

/*!
 * This function configures the gain applied to the summed timeslots
 *
 * @param        mix_out_att    if true the gain of result of mixing is -6dB
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_output_mixing_gain(bool mix_out_att);

/*!
 * This function configures number of timeslots.
 *
 * @param        ts_number    number of timeslots
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_network_nb_of_timeslots(int ts_number);

/*!
 * This function reset digital filter of ST dac
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_st_dac_reset_filter(void);

/*
 * CODEC
 */

/*!
 * This function enables or disables the CODEC of mc13783
 *
 * @param        en_dis   enable or disable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_enable(int en_dis);

/*!
 * This function selects input SSI bus for the CODEC
 *
 * @param        ssi_in   select SSI bus used.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_select_ssi(ssi_bus ssi_in);

/*!
 * This function selects input clock for the CODEC
 *
 * @param        cli_in   select clock input (CLIA or CLIB).
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_select_cli(clock_in cli_in);

/*!
 * This function selects the PLL clock input frequencies
 *
 * @param        input_freq_pll   select input clock PLL.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_conf_pll(int input_freq_pll);

/*!
 * This function selects Master/Slave configuration of CODEC
 *
 * @param        ms    select master/slave
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_master_slave(master_slave ms);

/*!
 * This function configures the bit clock format for CODEC
 *
 * @param        bc_inv    if true the bit clock is inverted
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_bit_clock_invert(bool bc_inv);

/*!
 * This function configures the frame sync format for CODEC
 *
 * @param        fs_inv    if true the frame sync is inverted
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_frame_sync_invert(bool fs_inv);

/*!
 * This function configures the bus protocol selection
 *
 * @param        bus    type of if true the frame sync offset is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_protocol(t_bus_protocol bus);

/*!
 * This function configures clock enable of CODEC
 *
 * @param        clk_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_clk_en(bool clk_en_des);

/*!
 * This function enables dithering filter of CODEC
 *
 * @param        dith_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_dithering_en(bool dith_en_des);

/*!
 * This function enables transmit high pass dithering
 *
 * @param        thpf_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_trans_high_pass_filter_en(bool thpf_en_des);

/*!
 * This function enables receive high pass dithering
 *
 * @param        rhpf_en_des    if true the clock is enable
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_receiv_high_pass_filter_en(bool rhpf_en_des);

/*!
 * This function configures the timeslots assignment
 *
 * @param        primary_timeslots      define the primary timeslots
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_conf_network_mode(timeslots primary_timeslots);

/*!
 * This function configures the network mixing mode
 *
 * @param        secondary_rxtimeslots   define the secondary rx timeslots
 * @param        secondary_txtimeslots   define the secondary rx timeslots
 * @param        mix_mode		       configuration of mixing mode

 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_conf_network_secondary(timeslots secondary_rxtimeslots,
					       timeslots secondary_txtimeslots,
					       mixing_mode mix_mode);

/*!
 * This function configures the gain applied to the summed timeslots on CODEC
 *
 * @param        mix_out_att    if true the gain of result of mixing is -6dB
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_output_mixing_gain(bool mix_out_att);

/*!
 * This function selects the sample rate of the CODEC
 *
 * @param        sr    select sample rate
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_set_sample_rate(sample_rate sr);

/*!
 * This function reset digital filter of codec
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_reset_filter(void);

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
int mc13783_audio_input_set_gain(int left, int right);

/*!
 * This function gets the input gain.
 *
 * @param        left    return left channel input gain
 * @param        right   return right channel input gain
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_input_get_gain(int *left, int *right);

/*!
 * This function enables/disables input.
 *
 * @param        en_dis    enable or disable input
 * @param        in_dev    select the input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_input_device(bool en_dis, input_device in_dev);

/*!
 * This function returns state of input.
 *
 * @param        in_dev    select the input
 * @param        in_en     Enable or disable input
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_input_state_device(input_device in_dev, bool * in_en);

/*!
 * This function is used to enable or disable latching edge delay.
 *
 * @param        bool           if true delay is added.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_set_latching_edge(bool latching_edge);

/*!
 * This function gets the current latching edge delay.
 *
 * @param        bool           if true delay is added.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_codec_get_latching_edge(bool * latching_edge);

/*!
 * This function initializes all audio register with default value.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_init_reg(void);

/*!
 * This function is used to subscribe on audio event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_event_sub(t_audio_int event, void *callback);

/*!
 * This function is used to un subscribe on audio event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_audio_event_unsub(t_audio_int event, void *callback);

/*!
 * This function is used to tell if mc13783 Audio driver has been correctly loaded.
 *
 * @return       This function returns 1 if Audio was successfully loaded
 * 		 0 otherwise.
 */
int mc13783_audio_loaded(void);

/*!
 * This function enables/disables phantom ground. This is used mainly
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
int mc13783_audio_set_phantom_ground(int activate);

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
int mc13783_audio_set_autodetect(int val);

#endif				/* __MC13783_AUDIO_H__ */
