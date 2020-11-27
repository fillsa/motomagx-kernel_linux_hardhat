/*
 * Copyright (c) 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General License as published by
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
 * @file pmic_audio.c
 * @brief Implementation of the PMIC Audio driver APIs.
 *
 * The PMIC Audio driver and this API were developed to support the
 * audio playback, recording, and mixing capabilities of the power
 * management ICs that are available from Freescale Semiconductor, Inc.
 * In particular, both the mc13783 and Phoenix sc55112 power management
 * ICs are currently supported. All of the common features of each
 * power management IC are available as well as access to all device-
 * specific features. However, attempting to use a device-specific
 * feature on a platform on which it is not supported will simply
 * return an error status.
 *
 * The following operating modes are supported:
 *
 * @verbatim
       Operating Mode               mc13783   sc55112
       ---------------------------- -----   ----------
       Stereo DAC Playback           Yes        Yes
       Stereo DAC Input Mixing       Yes        Yes
       Voice CODEC Playback          Yes        Yes
       Voice CODEC Input Mixing      Yes        No
       Voice CODEC Mono Recording    Yes        Yes
       Voice CODEC Stereo Recording  Yes        No
       Microphone Bias Control       Yes        Yes
       Output Amplifier Control      Yes        Yes
       Output Mixing Control         Yes        Yes
       Input Amplifier Control       Yes        Yes
       Master/Slave Mode Select      Yes        Yes
       Anti Pop Bias Circuit Control Yes        Yes
   @endverbatim
 *
 * Note that the Voice CODEC may also be referred to as the Telephone
 * CODEC in the PMIC DTS documentation. Also note that, while the power
 * management ICs do provide similar audio capabilities, each PMIC may
 * support additional configuration settings and features. Therefore, it
 * is highly recommended that the appropriate power management IC DTS
 * documents be used in conjunction with this API interface. The following
 * documents were used in the development of this API:
 *
 * [1] mc13783 (MC13783) Detailed Technical Specification (Level 3),
 *     Rev 1.3 - 04/09/17. Freescale Semiconductor, Inc.
 *
 * [2] iDEN Phoenix Platform Level 3 Detailed Technical Specification,
 *     Rev 2.2.1 5-12-2005. Motorola, Inc.
 *
 * @ingroup PMIC_SC55112_AUDIO
 */

#include <asm/arch/pmic_audio.h>	/* For PMIC Audio driver interface.    */
#include <asm/arch/pmic_external.h>	/* For PMIC Protocol driver interface. */
#include <asm/arch/pmic_adc.h>	/* For PMIC ADC driver interface.      */

#include <linux/interrupt.h>	/* For tasklet interface.              */
#include <linux/device.h>	/* For kernel module interface.        */
#include <linux/spinlock.h>	/* For spinlock interface.             */

/*
 * sc55112 Audio API
 */

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(MIN_STDAC_SAMPLING_RATE_HZ);
EXPORT_SYMBOL(MAX_STDAC_SAMPLING_RATE_HZ);
EXPORT_SYMBOL(pmic_audio_open);
EXPORT_SYMBOL(pmic_audio_close);
EXPORT_SYMBOL(pmic_audio_set_protocol);
EXPORT_SYMBOL(pmic_audio_get_protocol);
EXPORT_SYMBOL(pmic_audio_enable);
EXPORT_SYMBOL(pmic_audio_disable);
EXPORT_SYMBOL(pmic_audio_reset);
EXPORT_SYMBOL(pmic_audio_reset_all);
EXPORT_SYMBOL(pmic_audio_set_callback);
EXPORT_SYMBOL(pmic_audio_clear_callback);
EXPORT_SYMBOL(pmic_audio_get_callback);
EXPORT_SYMBOL(pmic_audio_antipop_enable);
EXPORT_SYMBOL(pmic_audio_antipop_disable);
EXPORT_SYMBOL(pmic_audio_digital_filter_reset);
EXPORT_SYMBOL(pmic_audio_vcodec_set_clock);
EXPORT_SYMBOL(pmic_audio_vcodec_get_clock);
EXPORT_SYMBOL(pmic_audio_vcodec_set_rxtx_timeslot);
EXPORT_SYMBOL(pmic_audio_vcodec_get_rxtx_timeslot);
EXPORT_SYMBOL(pmic_audio_vcodec_set_secondary_txslot);
EXPORT_SYMBOL(pmic_audio_vcodec_get_secondary_txslot);
EXPORT_SYMBOL(pmic_audio_vcodec_set_config);
EXPORT_SYMBOL(pmic_audio_vcodec_clear_config);
EXPORT_SYMBOL(pmic_audio_vcodec_get_config);
EXPORT_SYMBOL(pmic_audio_vcodec_enable_bypass);
EXPORT_SYMBOL(pmic_audio_vcodec_disable_bypass);
EXPORT_SYMBOL(pmic_audio_stdac_set_clock);
EXPORT_SYMBOL(pmic_audio_stdac_get_clock);
EXPORT_SYMBOL(pmic_audio_stdac_set_rxtx_timeslot);
EXPORT_SYMBOL(pmic_audio_stdac_get_rxtx_timeslot);
EXPORT_SYMBOL(pmic_audio_stdac_set_config);
EXPORT_SYMBOL(pmic_audio_stdac_clear_config);
EXPORT_SYMBOL(pmic_audio_stdac_get_config);
EXPORT_SYMBOL(pmic_audio_input_set_config);
EXPORT_SYMBOL(pmic_audio_input_clear_config);
EXPORT_SYMBOL(pmic_audio_input_get_config);
EXPORT_SYMBOL(pmic_audio_vcodec_set_mic);
EXPORT_SYMBOL(pmic_audio_vcodec_get_mic);
EXPORT_SYMBOL(pmic_audio_vcodec_set_mic_on_off);
EXPORT_SYMBOL(pmic_audio_vcodec_get_mic_on_off);
EXPORT_SYMBOL(pmic_audio_vcodec_set_record_gain);
EXPORT_SYMBOL(pmic_audio_vcodec_get_record_gain);
EXPORT_SYMBOL(pmic_audio_vcodec_enable_micbias);
EXPORT_SYMBOL(pmic_audio_vcodec_disable_micbias);
EXPORT_SYMBOL(pmic_audio_vcodec_enable_mixer);
EXPORT_SYMBOL(pmic_audio_vcodec_disable_mixer);
EXPORT_SYMBOL(pmic_audio_stdac_enable_mixer);
EXPORT_SYMBOL(pmic_audio_stdac_disable_mixer);
EXPORT_SYMBOL(pmic_audio_output_set_port);
EXPORT_SYMBOL(pmic_audio_output_get_port);
EXPORT_SYMBOL(pmic_audio_output_set_stereo_in_gain);
EXPORT_SYMBOL(pmic_audio_output_get_stereo_in_gain);
EXPORT_SYMBOL(pmic_audio_output_set_pgaGain);
EXPORT_SYMBOL(pmic_audio_output_get_pgaGain);
EXPORT_SYMBOL(pmic_audio_output_enable_mixer);
EXPORT_SYMBOL(pmic_audio_output_disable_mixer);
EXPORT_SYMBOL(pmic_audio_output_set_balance);
EXPORT_SYMBOL(pmic_audio_output_get_balance);
EXPORT_SYMBOL(pmic_audio_output_enable_mono_adder);
EXPORT_SYMBOL(pmic_audio_output_disable_mono_adder);
EXPORT_SYMBOL(pmic_audio_output_set_mono_adder_gain);
EXPORT_SYMBOL(pmic_audio_output_get_mono_adder_gain);
EXPORT_SYMBOL(pmic_audio_output_set_config);
EXPORT_SYMBOL(pmic_audio_output_clear_config);
EXPORT_SYMBOL(pmic_audio_output_get_config);
EXPORT_SYMBOL(pmic_audio_output_enable_phantom_ground);
EXPORT_SYMBOL(pmic_audio_output_disable_phantom_ground);
#ifdef DEBUG
EXPORT_SYMBOL(pmic_audio_dump_registers);
#endif				/* DEBUG */

#ifdef DEBUG
#define TRACEMSG(fmt,args...)  printk(fmt,##args)
#else				/* DEBUG */
#define TRACEMSG(fmt,args...)
#endif				/* DEBUG */

/*!
 * Define the minimum sampling rate (in Hz) that is supported by the
 * Stereo DAC.
 */
const unsigned MIN_STDAC_SAMPLING_RATE_HZ = 8000;

/*!
 * Define the maximum sampling rate (in Hz) that is supported by the
 * Stereo DAC.
 */
const unsigned MAX_STDAC_SAMPLING_RATE_HZ = 48000;

/*! @def SET_BITS
 * Set a register field to a given value.
 */
#define SET_BITS(reg, field, value)    (((value) << reg.field.offset) & \
                                        reg.field.mask)
/*! @def GET_BITS
 * Get the current value of a given register field.
 */
#define GET_BITS(reg, field, value)    (((value) & reg.field.mask) >> \
                                        reg.field.offset)

/*!
 * @brief Define the possible states for a device handle.
 *
 * This enumeration is used to track the current state of each device handle.
 */
typedef enum {
	HANDLE_FREE,		/*!< Handle is available for use. */
	HANDLE_IN_USE		/*!< Handle is currently in use.  */
} HANDLE_STATE;

/*!
 * @brief Identifies the hardware interrupt source.
 *
 * This enumeration identifies which of the possible hardware interrupt
 * sources actually caused the current interrupt handler to be called.
 */
typedef enum {
	CORE_EVENT_A1I,		/*!< Detected an A1 amplifier event. */
	CORE_EVENT_PTT		/*!< Detected a Push-to-Talk event.  */
} PMIC_CORE_EVENT;

/*!
 * @brief This structure is used to track the state of a microphone input.
 */
typedef struct {
	PMIC_AUDIO_INPUT_PORT mic;	/*!< Microphone input port.      */
	PMIC_AUDIO_INPUT_MIC_STATE micOnOff;	/*!< Microphone On/Off state.    */
	PMIC_AUDIO_MIC_AMP_MODE ampMode;	/*!< Input amplifier mode.       */
	PMIC_AUDIO_MIC_GAIN gain;	/*!< Input amplifier gain level. */
} PMIC_MICROPHONE_STATE;

/*!
 * @brief Tracks whether a headset is currently attached or not.
 */
typedef enum {
	NO_HEADSET,		/*!< No headset currently attached. */
	HEADSET_ON		/*!< Headset has been attached.     */
} HEADSET_STATUS;

/*!
 * @brief This structure is used to define a specific hardware register field.
 *
 * All hardware register fields are defined using an offset to the LSB
 * and a mask. The offset is used to right shift a register value before
 * applying the mask to actually obtain the value of the field.
 */
typedef struct {
	const unsigned char offset;	/*!< Offset of LSB of register field.          */
	const unsigned int mask;	/*!< Mask value used to isolate register field. */
} REGFIELD;

/*!
 * @brief This structure lists all fields of the AUD_CODEC hardware register.
 */
typedef struct {
	REGFIELD AUDIHPF;	/*!< Audio Input High-pass Filter Enable    */
	REGFIELD SMB;		/*!< Master/Slave Mode Select               */
	REGFIELD AUDOHPF;	/*!< Audio Output High-pass Filter Enable   */
	REGFIELD CD_TS;		/*!< Tri-state FSYNC, TX, and BITCLK Enable */
	REGFIELD DLM;		/*!< Digital Loopback Mode Enable           */
	REGFIELD ADITH;		/*!< Audio Signal Dithering Enable          */
	REGFIELD CDC_CLK;	/*!< Select CODEC PLL Clock Input           */
	REGFIELD CLK_INV;	/*!< Clock Invert Enable                    */
	REGFIELD FS_INV;	/*!< Frame Sync Invert Enable               */
	REGFIELD DF_RESET;	/*!< Digital Filter Reset Enable            */
	REGFIELD CDC_EN;	/*!< Voice CODEC Enable                     */
	REGFIELD CDC_CLK_EN;	/*!< Voice CODEC Clock Master Mode Enable   */
	REGFIELD FS_8K_16K;	/*!< Select 8 kHz or 16 kHz Sampling Rate   */
	REGFIELD DIG_AUD_IN;	/*!< Select Audio Input Data Bus            */
	REGFIELD CDC_SLOT_NMB;	/*!< Select Audio Data Timeslot Number      */
	REGFIELD CDC_PRI_ADJ;	/*!< Pri/Sec Processor Arbitration Control  */
} REGISTER_AUD_CODEC;

/*!
 * @brief This variable is used to access the AUD_CODEC hardware register.
 *
 * This variable defines how to access all of the fields within the
 * AUD_CODEC hardware register. The initial values consist of the offset
 * and mask values needed to access each of the register fields.
 */
static const REGISTER_AUD_CODEC regAUD_CODEC = {
	{0, 0x000001},		/* AUDIHPF      */
	{1, 0x000002},		/* SMB          */
	{2, 0x000004},		/* AUDOHPF      */
	{3, 0x000008},		/* CD_TS        */
	{4, 0x000010},		/* DLM          */
	{5, 0x000020},		/* ADITH        */
	{6, 0x0001c0},		/* CDC_CLK      */
	{9, 0x000200},		/* CLK_INV      */
	{10, 0x000400},		/* FS_INV       */
	{11, 0x000800},		/* DF_RESET     */
	{12, 0x001000},		/* CDC_EN       */
	{13, 0x002000},		/* CDC_CLK_EN   */
	{14, 0x004000},		/* FS_8K_16K    */
	{15, 0x008000},		/* DIG_AUD_IN   */
	{16, 0x030000},		/* CDC_SLOT_NMB */
	/* Unused       */
	/* Unused       */
	/* Unused       */
	/* Unused       */
	/* Unused       */
	{23, 0x800000}		/* CDC_PRI_ADJ  */
};

/*!
 * @brief This structure lists all fields of the ST_DAC hardware register.
 */
typedef struct {
	REGFIELD SMB_ST_DAC;	/*!< Master/Slave Mode Select             */
	REGFIELD ST_CLK;	/*!< Stereo DAC Clock Input Select        */
	REGFIELD ST_CLK_EN;	/*!< Stereo DAC Master Clock Mode Enable  */
	REGFIELD DF_RESET_ST_DAC;	/*!< Digital Filter Reset Enable          */
	REGFIELD ST_DAC_EN;	/*!< Stereo DAC Enable                    */
	REGFIELD SR;		/*!< Stereo DAC Sampling Rate Select      */
	REGFIELD DIG_AUD_IN_ST_DAC;	/*!< Stereo DAC Input Data Bus Select     */
	REGFIELD DIG_AUD_FS;	/*!< Stereo DAC Frame Sync Select         */
	REGFIELD BCLK;		/*!< Stereo DAC Bit Clock Select          */
	REGFIELD ST_CLK_INV;	/*!< Input Clock Invert Enable            */
	REGFIELD ST_FS_INV;	/*!< Input Frame Sync Invert Enable       */
	REGFIELD AUD_MIX_EN;	/*!< Stereo DAC Mixer Enable              */
	REGFIELD AUD_MIX_LVL;	/*!< Stereo DAC Mixer Level Select        */
	REGFIELD AUD_MIX_PRI_ADJ;	/*!< Pri/Sec Processor Arbitration Select */
	REGFIELD ST_DAC_PRI_ADJ;	/*!< Pri/Sec Processor Arbitration Select */
} REGISTER_ST_DAC;

/*!
 * @brief This variable is used to access the ST_DAC hardware register.
 *
 * This variable defines how to access all of the fields within the
 * ST_DAC hardware register. The initial values consist of the offset
 * and mask values needed to access each of the register fields.
 */
static const REGISTER_ST_DAC regST_DAC = {
	{0, 0x000001},		/* SMB_ST_DAC        */
	{2, 0x00001c},		/* ST_CLK            */
	{5, 0x000020},		/* ST_CLK_EN         */
	{6, 0x000040},		/* DF_RESET_ST_DAC   */
	{7, 0x000080},		/* ST_DAC_EN         */
	{8, 0x000f00},		/* SR                */
	{12, 0x001000},		/* DIG_AUD_IN_ST_DAC */
	{13, 0x006000},		/* DIG_AUD_FS        */
	{15, 0x018000},		/* BCLK              */
	{17, 0x020000},		/* ST_CLK_INV        */
	{18, 0x040000},		/* ST_FS_INV         */
	{19, 0x080000},		/* AUD_MIX_EN        */
	{20, 0x100000},		/* AUD_MIX_LVL       */
	{21, 0x600000},		/* AUD_MIX_PRI_ADJ   */
	{23, 0x800000}		/* ST_DAC_PRI_ADJ    */
};

/*!
 * @brief This structure lists all of the fields in the RX_AUD_AMPS hardware register.
 */
typedef struct {
	REGFIELD A1_EN;		/*!< Amplifier A1 Enable                         */
	REGFIELD A2_EN;		/*!< Amplifier A2 Enable                         */
	REGFIELD A2_ADJ_LOCK;	/*!< Pri/Sec Processor Arbitration Control       */
	REGFIELD A4_EN;		/*!< Amplifier A4 Enable                         */
	REGFIELD ARIGHT_EN;	/*!< Right Channel Amplifier Enable              */
	REGFIELD ALEFT_EN;	/*!< Left Channel Amplifier Enable               */
	REGFIELD CD_BYP;	/*!< Voice CODEC Bypass Enable                   */
	REGFIELD CDC_SW;	/*!< Voice CODEC Output Enable                   */
	REGFIELD ST_DAC_SW;	/*!< Stereo DAC Output Enable                    */
	REGFIELD PGA_IN_SW;	/*!< External Stereo Input Enable                */
	REGFIELD AUDOG;		/*!< PGA Gain Control                            */
	REGFIELD A1CTRL;	/*!< Non-inverting Output of A1 Amplifier Enable */
	REGFIELD A1ID_RX;	/*!< A1 Interrupt Override Enable                */
	REGFIELD MONO;		/*!< Mono Adder Enable                           */
	REGFIELD AUDOG_PRI_ADJ;	/*!< Pri/Sec Processor Arbitration Control       */
	REGFIELD MONO_PRI_ADJ;	/*!< Pri/Sec Processor Arbitration Control       */
	REGFIELD RX_PRI_ADJ;	/*!< Pri/Sec Processor Arbitration Control       */
} REGISTER_RX_AUD_AMPS;

/*!
 * @brief This variable is used to access the RX_AUD_AMPS hardware register.
 *
 * This variable defines how to access all of the fields within the
 * RX_AUD_AMPS hardware register. The initial values consist of the offset
 * and mask values needed to access each of the register fields.
 */
static const REGISTER_RX_AUD_AMPS regRX_AUD_AMPS = {
	{0, 0x000001},		/* A1_EN         */
	{1, 0x000002},		/* A2_EN         */
	{2, 0x000004},		/* A2_ADJ_LOCK   */
	{3, 0x000008},		/* A4_EN         */
	{4, 0x000010},		/* ARIGHT_EN     */
	{5, 0x000020},		/* ALEFT_EN      */
	{6, 0x000040},		/* CD_BYP        */
	{7, 0x000080},		/* CDC_SW        */
	{8, 0x000100},		/* ST_DAC_SW     */
	{9, 0x000200},		/* PGA_IN_SW     */
	/* Unused        */
	/* Unused        */
	{12, 0x00f000},		/* AUDOG         */
	{16, 0x010000},		/* A1CTRL        */
	{17, 0x020000},		/* A1ID_RX       */
	{18, 0x0c0000},		/* MONO          */
	{20, 0x100000},		/* AUDOG_PRI_ADJ */
	{21, 0x200000},		/* MONO_PRI_ADJ  */
	{22, 0xc00000}		/* RX_PRI_ADJ    */
};

/*!
 * @brief This structure lists all fields of the TX_AUD_AMPS hardware register.
 *
 * Note that the MB_ON1 and MB_ON2 bit positions have been reversed in the
 * sc55112 DTS documentation up to and including Rev 3.0. In reality,
 * MB_ON1 is bit 10 and MB_ON2 is bit 11. This problem has been confirmed and
 * a change request to update the DTS has been submitted.
 */
typedef struct {
	REGFIELD AUDIG;		/*!< Input Amplifier PGA Gain Select       */
	REGFIELD A3_EN;		/*!< A3 Amplifier Enable                   */
	REGFIELD A3_MUX;	/*!< A3 MUX Enable                         */
	REGFIELD A5_EN;		/*!< A5 Amplifier Enable                   */
	REGFIELD A5_MUX;	/*!< A5 MUX Enable                         */
	REGFIELD EXT_MIC_MUX;	/*!< External Microphone Input Enable      */
	REGFIELD MB_ON1;	/*!< Microphone Bias Circuit 1 Enable      */
	REGFIELD MB_ON2;	/*!< Microphone Bias Circuit 2 Enable      */
	REGFIELD A1ID_TX;	/*!< A1 Interrupt Override Enable          */
	REGFIELD A1_CONFIG;	/*!< Amplifier A1 Enable Override          */
	REGFIELD V3_6MB_ON2;	/*!< Headset Detection Enable              */
	REGFIELD MBUTTON_HS_ADET_EN;	/*!< Headset Detection Enable              */
	REGFIELD HS_SHRT_DET_EN;	/*!< Headset Short Detect Enable           */
	REGFIELD AUDIO_BIAS;	/*!< Audio Circuits Enable                 */
	REGFIELD AUDIO_BIAS_ARB;	/*!< Pri/Sec Processor Arbitration Control */
	REGFIELD AUDIO_BIAS_SPEED;	/*!< Linear Ramp of the VAG Slope Select   */
	REGFIELD AUDIG_PRI_ADJ;	/*!< Pri/Sec Processor Arbitration Control */
	REGFIELD TX_PRI_ADJ;	/*!< Pri/Sec Processor Arbitration Control */
} REGISTER_TX_AUD_AMPS;

/*!
 * @brief This variable is used to access the TX_AUD_AMPS hardware register.
 *
 * This variable defines how to access all of the fields within the
 * TX_AUD_AMPS hardware register. The initial values consist of the offset
 * and mask values needed to access each of the register fields.
 */
static const REGISTER_TX_AUD_AMPS regTX_AUD_AMPS = {
	{0, 0x00001f},		/* AUDIG              */
	{5, 0x000020},		/* A3_EN              */
	{6, 0x000040},		/* A3_MUX             */
	{7, 0x000080},		/* A5_EN              */
	{8, 0x000100},		/* A5_MUX             */
	{9, 0x000200},		/* EXT_MIC_MUX        */
	{10, 0x000400},		/* MB_ON1             */
	{11, 0x000800},		/* MB_ON2             */
	{12, 0x001000},		/* A1ID_TX            */
	{13, 0x002000},		/* A1_CONFIG          */
	/* Unused             */
	{15, 0x008000},		/* V3_6MB_ON2         */
	{16, 0x010000},		/* MBUTTON_HS_ADET_EN */
	{17, 0x020000},		/* HS_SHRT_DET_EN     */
	{18, 0x040000},		/* AUDIO_BIAS         */
	{19, 0x080000},		/* AUDIO_BIAS_ARB     */
	{20, 0x100000},		/* AUDIO_BIAS_SPEED   */
	{21, 0x200000},		/* AUDIO_PRI_ADJ      */
	{22, 0xc00000}		/* TX_PRI_ADJ         */
};

/*!
 * @brief This structure lists all fields of the SWCTRL hardware register.
 */
typedef struct {
	REGFIELD SW1_EN;
	REGFIELD SW1_MODE;
	REGFIELD SW1;
	REGFIELD SW1_STBY_LVS_EN;
	REGFIELD SW2_MODE;
	REGFIELD SW1_LVS;
	REGFIELD SW3_EN;
	REGFIELD SW3;
	REGFIELD SW3_STBY_EN;
	REGFIELD SW2;
	REGFIELD SW2_LVS;
	REGFIELD SW1_STBY;
	REGFIELD SW2_EN;
	REGFIELD SW2_DFVS_EN;
} REGISTER_SWCTRL;

/*!
 * @brief This variable is used to access the SWCTRL hardware register.
 *
 * This variable defines how to access all of the fields within the
 * SWCTRL hardware register. The initial values consist of the offset
 * and mask values needed to access each of the register fields.
 */
static const REGISTER_SWCTRL regSWCTRL = {
	{0, 0x000001},		/* SW1_EN             */
	{1, 0x000002},		/* SW1_MODE           */
	{2, 0x00001c},		/* SW1                */
	{5, 0x000020},		/* SW1_STBY_LVS_EN    */
	{6, 0x000040},		/* SW2_MODE           */
	{7, 0x000380},		/* SW1_LVS            */
	{10, 0x000400},		/* SW3_EN             */
	{11, 0x000800},		/* SW3                */
	{12, 0x001000},		/* SW3_STBY_EN        */
	{13, 0x00e000},		/* SW2                */
	{16, 0x070000},		/* SW2_LVS            */
	{19, 0x380000},		/* SW1_STBY           */
	{22, 0x400000},		/* SW2_EN             */
	{23, 0x800000}		/* SW2_DFVS_EN        */
};

/*! Define a mask to access the entire hardware register. */
static const unsigned int REG_FULLMASK = 0xffffff;

/*! Reset value for the AUD_CODEC register. */
static const unsigned int RESET_AUD_CODEC = 0x000007;

/*! Reset value for the ST_DAC register.
 *
 *  Note that we avoid resetting any of the arbitration bits.
 */
static const unsigned int RESET_ST_DAC = 0x019e01;

/*! Reset value for the RX_AUD_AMPS register. */
static const unsigned int RESET_RX_AUD_AMPS = 0x008000;

/*! Reset value for the TX_AUD_AMPS register.
 *
 *  Note that we avoid resetting any of the arbitration bits.
 */
static const unsigned int RESET_TX_AUD_AMPS = 0x000000;

/*! Constant NULL value for initializing/reseting the audio handles. */
static const PMIC_AUDIO_HANDLE AUDIO_HANDLE_NULL = (PMIC_AUDIO_HANDLE) NULL;

/*!
 * @brief This structure maintains the current state of the Stereo DAC.
 */
typedef struct {
	PMIC_AUDIO_HANDLE handle;	/*!< Handle used to access
					   the Stereo DAC.         */
	HANDLE_STATE handleState;	/*!< Current handle state.   */
	PMIC_AUDIO_DATA_BUS busID;	/*!< Data bus used to access
					   the Stereo DAC.         */
	PMIC_AUDIO_BUS_PROTOCOL protocol;	/*!< Data bus protocol.      */
	PMIC_AUDIO_BUS_MODE masterSlave;	/*!< Master/Slave mode
						   select.                 */
	PMIC_AUDIO_NUMSLOTS numSlots;	/*!< Number of timeslots
					   used.                   */
	PMIC_AUDIO_CALLBACK callback;	/*!< Event notification
					   callback function
					   pointer.                */
	PMIC_AUDIO_EVENTS eventMask;	/*!< Event notification mask. */
	PMIC_AUDIO_CLOCK_IN_SOURCE clockIn;	/*!< Stereo DAC clock input
						   source select.          */
	PMIC_AUDIO_STDAC_SAMPLING_RATE samplingRate;	/*!< Stereo DAC sampling rate
							   select.                 */
	PMIC_AUDIO_STDAC_CLOCK_IN_FREQ clockFreq;	/*!< Stereo DAC clock input
							   frequency.              */
	PMIC_AUDIO_CLOCK_INVERT invert;	/*!< Stereo DAC clock signal
					   invert select.          */
	PMIC_AUDIO_STDAC_TIMESLOTS timeslot;	/*!< Stereo DAC data
						   timeslots select.       */
	PMIC_AUDIO_STDAC_CONFIG config;	/*!< Stereo DAC configuration
					   options.                */
} PMIC_AUDIO_STDAC_STATE;

/*!
 * @brief This variable maintains the current state of the Stereo DAC.
 *
 * This variable tracks the current state of the Stereo DAC audio hardware
 * along with any information that is required by the device driver to
 * manage the hardware (e.g., callback functions and event notification
 * masks).
 *
 * The initial values represent the reset/power on state of the Stereo DAC.
 */
static PMIC_AUDIO_STDAC_STATE stDAC = {
	(PMIC_AUDIO_HANDLE) NULL,	/* handle       */
	HANDLE_FREE,		/* handleState  */
	AUDIO_DATA_BUS_2,	/* busID        */
	NORMAL_MSB_JUSTIFIED_MODE,	/* protocol     */
	BUS_SLAVE_MODE,		/* masterSlave  */
	USE_2_TIMESLOTS,	/* numSlots     */
	(PMIC_AUDIO_CALLBACK) NULL,	/* callback     */
	(PMIC_AUDIO_EVENTS) NULL,	/* eventMask    */
	CLOCK_IN_CLKIN,		/* clockIn      */
	STDAC_RATE_44_1_KHZ,	/* samplingRate */
	STDAC_CLI_13MHZ,	/* clockFreq    */
	NO_INVERT,		/* invert       */
	USE_TS0_TS1,		/* timeslot     */
	(PMIC_AUDIO_STDAC_CONFIG) 0	/* config       */
};

/*!
 * @brief This structure maintains the current state of the Voice CODEC.
 */
typedef struct {
	PMIC_AUDIO_HANDLE handle;	/*!< Handle used to access
					   the Voice CODEC.       */
	HANDLE_STATE handleState;	/*!< Current handle state.  */
	PMIC_AUDIO_DATA_BUS busID;	/*!< Data bus used to access
					   the Voice CODEC.       */
	PMIC_AUDIO_BUS_PROTOCOL protocol;	/*!< Data bus protocol.     */
	PMIC_AUDIO_BUS_MODE masterSlave;	/*!< Master/Slave mode
						   select.                */
	PMIC_AUDIO_NUMSLOTS numSlots;	/*!< Number of timeslots
					   used.                  */
	PMIC_AUDIO_CALLBACK callback;	/*!< Event notification
					   callback function
					   pointer.               */
	PMIC_AUDIO_EVENTS eventMask;	/*!< Event notification
					   mask.                  */
	PMIC_AUDIO_CLOCK_IN_SOURCE clockIn;	/*!< Voice CODEC clock input
						   source select.         */
	PMIC_AUDIO_VCODEC_SAMPLING_RATE samplingRate;	/*!< Voice CODEC sampling
							   rate select.           */
	PMIC_AUDIO_VCODEC_CLOCK_IN_FREQ clockFreq;	/*!< Voice CODEC clock input
							   frequency.             */
	PMIC_AUDIO_CLOCK_INVERT invert;	/*!< Voice CODEC clock
					   signal invert select.  */
	PMIC_AUDIO_VCODEC_TIMESLOT timeslot;	/*!< Voice CODEC data
						   timeslot select.       */
#if 0
	/* Secondary channel recording is not supported by the sc55112 PMIC
	 * Voice CODEC so there is no need for this field.
	 */
	PMIC_AUDIO_VCODEC_TIMESLOT secondaryTXtimeslot;
#endif
	PMIC_AUDIO_VCODEC_CONFIG config;	/*!< Voice CODEC
						   configuration
						   options.            */
	PMIC_AUDIO_INPUT_CONFIG inputConfig;	/*!< Voice CODEC
						   recording
						   configuration.      */
	PMIC_MICROPHONE_STATE leftChannelMic;	/*!< Left channel
						   microphone
						   configuration.      */
	PMIC_MICROPHONE_STATE rightChannelMic;	/*!< Right channel
						   microphone
						   configuration.      */
} PMIC_AUDIO_VCODEC_STATE;

/*!
 * @brief This variable maintains the current state of the Voice CODEC.
 *
 * This variable tracks the current state of the Voice CODEC audio hardware
 * along with any information that is required by the device driver to
 * manage the hardware (e.g., callback functions and event notification
 * masks).
 *
 * The initial values represent the reset/power on state of the Voice CODEC.
 */
static PMIC_AUDIO_VCODEC_STATE vCodec = {
	(PMIC_AUDIO_HANDLE) NULL,	/* handle          */
	HANDLE_FREE,		/* handleState     */
	AUDIO_DATA_BUS_1,	/* busID           */
	NETWORK_MODE,		/* protocol        */
	BUS_SLAVE_MODE,		/* masterSlave     */
	USE_4_TIMESLOTS,	/* numSlots        */
	(PMIC_AUDIO_CALLBACK) NULL,	/* callback        */
	(PMIC_AUDIO_EVENTS) NULL,	/* eventMask       */
	CLOCK_IN_CLKIN,		/* clockIn         */
	VCODEC_RATE_8_KHZ,	/* samplingRate    */
	VCODEC_CLI_13MHZ,	/* clockFreq       */
	NO_INVERT,		/* invert          */
	USE_TS0,		/* timeslot        */
	INPUT_HIGHPASS_FILTER | OUTPUT_HIGHPASS_FILTER,	/* config          */
	(PMIC_AUDIO_INPUT_CONFIG) NULL,	/* inputConfig     */
	/* leftChannelMic  */
	{NO_MIC,		/*    mic          */
	 MICROPHONE_OFF,	/*    micOnOff     */
	 CURRENT_TO_VOLTAGE,	/*    ampMode      */
	 MIC_GAIN_0DB		/*    gain         */
	 },
	/* rightChannelMic */
	{NO_MIC,		/*    mic          */
	 MICROPHONE_OFF,	/*    micOnOff     */
	 AMP_OFF,		/*    ampMode      */
	 MIC_GAIN_0DB		/*    gain         */
	 }
};

/*!
 * @brief This maintains the current state of the External Stereo Input.
 */
typedef struct {
	PMIC_AUDIO_HANDLE handle;	/*!< Handle used to access the
					   External Stereo Inputs.     */
	HANDLE_STATE handleState;	/*!< Current handle state.       */
	PMIC_AUDIO_CALLBACK callback;	/*!< Event notification callback
					   function pointer.           */
	PMIC_AUDIO_EVENTS eventMask;	/*!< Event notification mask.    */
	PMIC_AUDIO_STEREO_IN_GAIN inputGain;	/*!< External Stereo Input
						   amplifier gain level.       */
} PMIC_AUDIO_EXT_STEREO_IN_STATE;

/*!
 * @brief This maintains the current state of the External Stereo Input.
 *
 * This variable tracks the current state of the External Stereo Input audio
 * hardware along with any information that is required by the device driver
 * to manage the hardware (e.g., callback functions and event notification
 * masks).
 *
 * The initial values represent the reset/power on state of the External
 * Stereo Input.
 */
static PMIC_AUDIO_EXT_STEREO_IN_STATE extStereoIn = {
	(PMIC_AUDIO_HANDLE) NULL,	/* handle      */
	HANDLE_FREE,		/* handleState */
	(PMIC_AUDIO_CALLBACK) NULL,	/* callback    */
	(PMIC_AUDIO_EVENTS) NULL,	/* eventMask   */
	STEREO_IN_GAIN_0DB	/* inputGain   */
};

/*!
 * @brief This maintains the current state of the Audio Output Section.
 */
typedef struct {
	PMIC_AUDIO_OUTPUT_PORT outputPort;	/*!< Current audio
						   output port.     */
	PMIC_AUDIO_OUTPUT_PGA_GAIN outputPGAGain;	/*!< Output PGA gain
							   level.           */
	PMIC_AUDIO_OUTPUT_BALANCE_GAIN balanceLeftGain;	/*!< Left channel
							   balance gain
							   level.           */
	PMIC_AUDIO_OUTPUT_BALANCE_GAIN balanceRightGain;	/*!< Right channel
								   balance gain
								   level.           */
	PMIC_AUDIO_MONO_ADDER_OUTPUT_GAIN monoAdderGain;	/*!< Mono adder gain
								   level.           */
	PMIC_AUDIO_OUTPUT_CONFIG config;	/*!< Audio output
						   section config
						   options.         */
} PMIC_AUDIO_AUDIO_OUTPUT_STATE;

/*!
 * @brief This variable maintains the current state of the Audio Output Section.
 *
 * This variable tracks the current state of the Audio Output Section.
 *
 * The initial values represent the reset/power on state of the Audio
 * Output Section.
 */
static PMIC_AUDIO_AUDIO_OUTPUT_STATE audioOutput = {
	(PMIC_AUDIO_OUTPUT_PORT) NULL,	/* outputPort       */
	OUTPGA_GAIN_0DB,	/* outputPGAGain    */
	BAL_GAIN_0DB,		/* balanceLeftGain  */
	BAL_GAIN_0DB,		/* balanceRightGain */
	MONOADD_GAIN_0DB,	/* monoAdderGain    */
	(PMIC_AUDIO_OUTPUT_CONFIG) 0	/* config           */
};

/*! The current headset status. */
static HEADSET_STATUS headsetState = NO_HEADSET;

/*! Voltage level for the Push-to-Talk (PTT) button. */
static unsigned int ptt_button_level = 0;

/*! Define a 1 ms wait interval that is needed to ensure that certain
 *  hardware operations are successfully completed.
 */
static const unsigned long delay_1ms = (HZ / 1000);

/*!
 * @brief This spinlock is used to provide mutual exclusion.
 *
 * Create a spinlock that can be used to provide mutually exclusive
 * read/write access to the globally accessible data structures
 * that were defined above. Mutually exclusive access is required to
 * ensure that the audio data structures are consistent at all times
 * when possibly accessed by multiple threads of execution (for example,
 * while simultaneously handling a user request and an interrupt event).
 *
 * We need to use a spinlock whenever we do need to provide mutual
 * exclusion while possibly executing in a hardware interrupt context.
 * Spinlocks should be held for the minimum time that is necessary
 * because hardware interrupts are disabled while a spinlock is held.
 */
static spinlock_t lock = SPIN_LOCK_UNLOCKED;

/*!
 * @brief This mutex is used to provide mutual exclusion.
 *
 * Create a mutex that can be used to provide mutually exclusive
 * read/write access to the globally accessible data structures
 * that were defined above. Mutually exclusive access is required to
 * ensure that the audio data structures are consistent at all times
 * when possibly accessed by multiple threads of execution.
 *
 * Note that we use a mutex instead of the spinlock whenever disabling
 * interrupts while in the critical section is not required. This helps
 * to minimize kernel interrupt handling latency.
 */
static DECLARE_MUTEX(mutex);

/* Prototype for the audio driver tasklet function. */
static void pmic_audio_tasklet(unsigned long arg);

/*!
 * @brief Tasklet handler for the audio driver.
 *
 * Declare a tasklet that will do most of the processing for both A1I
 * and PTTI interrupt events. Note that we cannot do all of the required
 * processing within the interrupt handler itself because we may need to
 * call the ADC driver to measure voltages as well as calling any
 * user-registered callback functions.
 */
DECLARE_TASKLET(audioTasklet, pmic_audio_tasklet, 0);

/*!
 * @brief Global variable to track currently active interrupt events.
 *
 * This global variable is used to keep track of all of the currently
 * active interrupt events for the audio driver. Note that access to this
 * variable may occur while within an interrupt context and, therefore,
 * must be guarded by using a spinlock.
 */
static PMIC_CORE_EVENT eventID = 0;

/* Prototypes for all static audio driver functions. */
static PMIC_STATUS pmic_audio_mic_boost_enable(void);
static PMIC_STATUS pmic_audio_mic_boost_disable(void);
static PMIC_STATUS pmic_audio_close_handle(const PMIC_AUDIO_HANDLE handle);
static PMIC_STATUS pmic_audio_reset_device(const PMIC_AUDIO_HANDLE handle);
static PMIC_STATUS pmic_audio_deregister(PMIC_AUDIO_CALLBACK * const callback,
					 PMIC_AUDIO_EVENTS * const eventMask);
static void pmic_audio_event_handler(void *param);

/*************************************************************************
 * Audio device access APIs.
 *************************************************************************
 */

/*!
 * @name General Setup and Configuration APIs
 * Functions for general setup and configuration of the PMIC Audio
 * hardware.
 */
/*@{*/

/*!
 * @brief Request exclusive access to the PMIC Audio hardware.
 *
 * Attempt to open and gain exclusive access to a key PMIC audio hardware
 * component (e.g., the Stereo DAC or the Voice CODEC). Depending upon the
 * type of audio operation that is desired and the nature of the audio data
 * stream, the Stereo DAC and/or the Voice CODEC will be a required hardware
 * component and needs to be acquired by calling this function.
 *
 * If the open request is successful, then a numeric handle is returned
 * and this handle must be used in all subsequent function calls to complete
 * the configuration of either the Stereo DAC or the Voice CODEC and along
 * with any other associated audio hardware components that will be needed.
 *
 * The same handle must also be used in the close call when use of the PMIC
 * audio hardware is no longer required.
 *
 * The open request will fail if the requested audio hardware component has
 * already been acquired by a previous open call but not yet closed.
 *
 * @param[out]  handle          Device handle to be used for subsequent PMIC
 *                              audio API calls.
 * @param[in]   device          The required PMIC audio hardware component.
 *
 * @retval      PMIC_SUCCESS         If the open request was successful
 * @retval      PMIC_PARAMETER_ERROR If the handle argument is NULL.
 * @retval      PMIC_ERROR           If the audio hardware component is
 *                                   unavailable.
 */
PMIC_STATUS pmic_audio_open(PMIC_AUDIO_HANDLE * const handle,
			    const PMIC_AUDIO_SOURCE device)
{
	PMIC_STATUS rc = PMIC_ERROR;

	if (handle == (PMIC_AUDIO_HANDLE *) NULL) {
		/* Do not dereference a NULL pointer. */
		return PMIC_PARAMETER_ERROR;
	}

	/* We only need to acquire a mutex here because the interrupt handler
	 * never modifies the device handle or device handle state. Therefore,
	 * we don't need to worry about conflicts with the interrupt handler
	 * or the need to execute in an interrupt context.
	 *
	 * But we do need a critical section here to avoid problems in case
	 * multiple calls to pmic_audio_open() are made since we can only allow
	 * one of them to succeed.
	 */
	down_interruptible(&mutex);

	/* Check the current device handle state and acquire the handle if
	 * it is available.
	 */
	if ((device == STEREO_DAC) && (stDAC.handleState == HANDLE_FREE)) {
		stDAC.handle = (PMIC_AUDIO_HANDLE) (&stDAC);
		stDAC.handleState = HANDLE_IN_USE;
		*handle = stDAC.handle;

		rc = PMIC_SUCCESS;
	} else if ((device == VOICE_CODEC)
		   && (vCodec.handleState == HANDLE_FREE)) {
		vCodec.handle = (PMIC_AUDIO_HANDLE) (&vCodec);
		vCodec.handleState = HANDLE_IN_USE;
		*handle = vCodec.handle;

		rc = PMIC_SUCCESS;
	} else if ((device == EXTERNAL_STEREO_IN) &&
		   (extStereoIn.handleState == HANDLE_FREE)) {
		extStereoIn.handle = (PMIC_AUDIO_HANDLE) (&extStereoIn);
		extStereoIn.handleState = HANDLE_IN_USE;
		*handle = extStereoIn.handle;

		rc = PMIC_SUCCESS;

	} else {

		*handle = AUDIO_HANDLE_NULL;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Terminate further access to the PMIC audio hardware.
 *
 * Terminate further access to the PMIC audio hardware that was previously
 * acquired by calling pmic_audio_open(). This now allows another thread to
 * successfully call pmic_audio_open() to gain access.
 *
 * Note that we will shutdown/reset the Voice CODEC or Stereo DAC as well as
 * any associated audio input/output components that are no longer required.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the close request was successful.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_audio_close(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* We need a critical section here to maintain a consistent state. */
	down_interruptible(&mutex);

	/* We can now call pmic_audio_close_handle() to actually do the work. */
	rc = pmic_audio_close_handle(handle);

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Configure the data bus protocol to be used.
 *
 * Provide the parameters needed to properly configure the audio data bus
 * protocol so that data can be read/written to either the Stereo DAC or
 * the Voice CODEC.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   busID           Select data bus to be used.
 * @param[in]   protocol        Select the data bus protocol.
 * @param[in]   masterSlave     Select the data bus timing mode.
 * @param[in]   numSlots        Define the number of timeslots (only if in
 *                              master mode).
 *
 * @retval      PMIC_SUCCESS         If the protocol was successful configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or the protocol parameters
 *                                   are invalid.
 */
PMIC_STATUS pmic_audio_set_protocol(const PMIC_AUDIO_HANDLE handle,
				    const PMIC_AUDIO_DATA_BUS busID,
				    const PMIC_AUDIO_BUS_PROTOCOL protocol,
				    const PMIC_AUDIO_BUS_MODE masterSlave,
				    const PMIC_AUDIO_NUMSLOTS numSlots)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int ST_DAC_MASK =
	    SET_BITS(regST_DAC, DIG_AUD_IN_ST_DAC, 1) | SET_BITS(regST_DAC,
								 DIG_AUD_FS,
								 3) |
	    SET_BITS(regST_DAC, SMB_ST_DAC, 1) | SET_BITS(regST_DAC, ST_CLK_EN,
							  1) |
	    SET_BITS(regST_DAC, BCLK, 3);
	const unsigned int VCODEC_MASK =
	    SET_BITS(regAUD_CODEC, DIG_AUD_IN, 1) | SET_BITS(regAUD_CODEC,
							     CDC_CLK_EN,
							     1) |
	    SET_BITS(regAUD_CODEC, SMB, 1);
	unsigned int reg_value = 0;

	/* Enter a critical section so that we can ensure only one
	 * state change request is completed at a time.
	 */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		/* Make sure that the sc55112 PMIC supports the requested
		 * Stereo DAC data bus protocol.
		 */
		if ((protocol == NETWORK_MODE) &&
		    !((numSlots == USE_2_TIMESLOTS) ||
		      (numSlots == USE_4_TIMESLOTS) ||
		      (numSlots == USE_8_TIMESLOTS))) {
			/* The Stereo DAC only supports 2, 4, or 8 time slots when in
			 * Network Mode.
			 */
			TRACEMSG
			    ("%s: Invalid numSlots parameter (Stereo DAC only supports "
			     "2,4, or 8 timeslots in Network Mode)\n",
			     __FILE__);
			rc = PMIC_NOT_SUPPORTED;
		} else if (((protocol == NORMAL_MSB_JUSTIFIED_MODE) ||
			    (protocol == I2S_MODE)) &&
			   (numSlots != USE_2_TIMESLOTS)) {
			/* The Stereo DAC only supports the use of 2 time slots when in
			 * Normal Mode or I2S Mode.
			 */
			TRACEMSG
			    ("%s: Invalid numSlots parameter (Stereo DAC only supports "
			     "2 timeslots in Normal or I2S Mode)\n", __FILE__);
			rc = PMIC_NOT_SUPPORTED;
		} else if ((masterSlave == BUS_MASTER_MODE) &&
			   ((stDAC.clockIn == CLOCK_IN_MCLK) ||
			    (stDAC.clockIn == CLOCK_IN_FSYNC) ||
			    (stDAC.clockIn == CLOCK_IN_BITCLK))) {
			/* Invalid clock inputs for master mode. */
			TRACEMSG
			    ("%s: Invalid Stereo DAC clock selection for Master Mode\n",
			     __FILE__);
			rc = PMIC_PARAMETER_ERROR;
		} else if ((masterSlave == BUS_SLAVE_MODE) &&
			   ((stDAC.clockIn == CLOCK_IN_DEFAULT) ||
			    (stDAC.clockIn == CLOCK_IN_CLKIN))) {
			/* Invalid clock inputs for slave mode. */
			TRACEMSG
			    ("%s: Invalid Stereo DAC clock selection for Slave Mode\n",
			     __FILE__);
			rc = PMIC_PARAMETER_ERROR;
		} else if ((vCodec.handleState == HANDLE_IN_USE) &&
			   (vCodec.busID == busID)) {
			/* The requested data bus is already in use by the Voice CODEC. */
			TRACEMSG
			    ("%s: Requested data bus already in use by the Voice "
			     "CODEC\n", __FILE__);
			rc = PMIC_PARAMETER_ERROR;
		} else {
			/* The Voice CODEC is not currently being used but we still need
			 * to switch it to the other data bus so that there is no conflict
			 * with the Stereo DAC.
			 */
			if (vCodec.busID == busID) {
				if (busID == AUDIO_DATA_BUS_1) {
					reg_value =
					    SET_BITS(regAUD_CODEC, DIG_AUD_IN,
						     AUDIO_DATA_BUS_2);
				} else {
					reg_value =
					    SET_BITS(regAUD_CODEC, DIG_AUD_IN,
						     AUDIO_DATA_BUS_1);
				}

				/* Note that this update to AUD_CODEC register also has the
				 * side effect of disabling the Voice CODEC master mode clock
				 * outputs (if they are currently enabled) and changing the
				 * Voice CODEC back to slave mode. We have to do this now
				 * because we want the Voice CODEC to be in a safe idle
				 * state when we switch it over to the other data bus.
				 */
				if (pmic_write_reg
				    (PRIO_AUDIO, REG_AUD_CODEC, reg_value,
				     VCODEC_MASK) != PMIC_SUCCESS) {
					rc = PMIC_ERROR;
				}
			}

			if (rc != PMIC_ERROR) {
				/* We can now proceed with the Stereo DAC configuration. */
				reg_value =
				    SET_BITS(regST_DAC, DIG_AUD_IN_ST_DAC,
					     busID) | SET_BITS(regST_DAC,
							       DIG_AUD_FS,
							       protocol) |
				    SET_BITS(regST_DAC, SMB_ST_DAC,
					     masterSlave);

				if (masterSlave == BUS_MASTER_MODE) {
					/* Enable clock outputs when in master mode. */
					reg_value |=
					    SET_BITS(regST_DAC, ST_CLK_EN, 1);
				}

				if (numSlots == USE_2_TIMESLOTS) {
					/* Use 2 time slots (left, right). */
					reg_value |=
					    SET_BITS(regST_DAC, BCLK, 3);
				} else if (numSlots == USE_4_TIMESLOTS) {
					/* Use 4 time slots (left, right, 2 other). */
					reg_value |=
					    SET_BITS(regST_DAC, BCLK, 2);
				} else {
					/* Use 8 time slots (left, right, 6 other). */
					reg_value |=
					    SET_BITS(regST_DAC, BCLK, 1);
				}

				rc = pmic_write_reg(PRIO_AUDIO, REG_ST_DAC,
						    reg_value, ST_DAC_MASK);

				if (rc == PMIC_SUCCESS) {
					stDAC.busID = busID;
					stDAC.protocol = protocol;
					stDAC.masterSlave = masterSlave;
					stDAC.numSlots = numSlots;
				}
			}
		}
	} else if ((handle == vCodec.handle) &&
		   (vCodec.handleState == HANDLE_IN_USE)) {
		/* The sc55112 Voice CODEC only supports network mode with 4
		 * timeslots.
		 */
		if ((protocol != NETWORK_MODE) || (numSlots != USE_4_TIMESLOTS)) {
			rc = PMIC_NOT_SUPPORTED;
		} else if ((stDAC.handleState == HANDLE_IN_USE) &&
			   (stDAC.busID == busID)) {
			/* The requested data bus is already in use by the Stereo DAC. */
			rc = PMIC_PARAMETER_ERROR;
		} else {
			/* The Stereo DAC is not currently being used but we still need
			 * to switch it to the other data bus so that there is no conflict
			 * with the Voice CODEC.
			 */
			if (stDAC.busID == busID) {
				if (busID == AUDIO_DATA_BUS_1) {
					reg_value =
					    SET_BITS(regST_DAC,
						     DIG_AUD_IN_ST_DAC,
						     AUDIO_DATA_BUS_2);
				} else {
					reg_value =
					    SET_BITS(regST_DAC,
						     DIG_AUD_IN_ST_DAC,
						     AUDIO_DATA_BUS_1);
				}

				if (pmic_write_reg
				    (PRIO_AUDIO, REG_ST_DAC, reg_value,
				     ST_DAC_MASK) != PMIC_SUCCESS) {
					rc = PMIC_ERROR;
				}
			}

			if (rc != PMIC_ERROR) {
				/* Note that the Voice CODEC only operates in network mode
				 * with 4 timeslots so no further configuration is required
				 * for the "protocol" and "numSlots" parameters.
				 */
				reg_value =
				    SET_BITS(regAUD_CODEC, DIG_AUD_IN,
					     busID) | SET_BITS(regAUD_CODEC,
							       SMB,
							       masterSlave);

				if (masterSlave == BUS_MASTER_MODE) {
					reg_value |=
					    SET_BITS(regAUD_CODEC, CDC_CLK_EN,
						     1);
				}

				rc = pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC,
						    reg_value, VCODEC_MASK);

				if (rc == PMIC_SUCCESS) {
					vCodec.busID = busID;
					vCodec.protocol = protocol;
					vCodec.masterSlave = masterSlave;
					vCodec.numSlots = numSlots;
				}
			}
		}
	}

	/* Exit critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Retrieve the current data bus protocol configuration.
 *
 * Retrieve the parameters that define the current audio data bus protocol.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  busID           The data bus being used.
 * @param[out]  protocol        The data bus protocol being used.
 * @param[out]  masterSlave     The data bus timing mode being used.
 * @param[out]  numSlots        The number of timeslots being used (if in
 *                              master mode).
 *
 * @retval      PMIC_SUCCESS         If the protocol was successful retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_audio_get_protocol(const PMIC_AUDIO_HANDLE handle,
				    PMIC_AUDIO_DATA_BUS * const busID,
				    PMIC_AUDIO_BUS_PROTOCOL * const protocol,
				    PMIC_AUDIO_BUS_MODE * const masterSlave,
				    PMIC_AUDIO_NUMSLOTS * const numSlots)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	if ((busID != (PMIC_AUDIO_DATA_BUS *) NULL) &&
	    (protocol != (PMIC_AUDIO_BUS_PROTOCOL *) NULL) &&
	    (masterSlave != (PMIC_AUDIO_BUS_MODE *) NULL) &&
	    (numSlots != (PMIC_AUDIO_NUMSLOTS *) NULL)) {
		/* Enter a critical section so that we return a consistent state. */
		down_interruptible(&mutex);

		if ((handle == stDAC.handle) &&
		    (stDAC.handleState == HANDLE_IN_USE)) {
			*busID = stDAC.busID;
			*protocol = stDAC.protocol;
			*masterSlave = stDAC.masterSlave;
			*numSlots = stDAC.numSlots;

			rc = PMIC_SUCCESS;
		} else if ((handle == vCodec.handle) &&
			   (vCodec.handleState == HANDLE_IN_USE)) {
			*busID = vCodec.busID;
			*protocol = vCodec.protocol;
			*masterSlave = vCodec.masterSlave;
			*numSlots = vCodec.numSlots;

			rc = PMIC_SUCCESS;
		}

		/* Exit critical section. */
		up(&mutex);
	}

	return rc;
}

/*!
 * @brief Enable the Stereo DAC or the Voice CODEC.
 *
 * Explicitly enable the Stereo DAC or the Voice CODEC to begin audio
 * playback or recording as required. This should only be done after
 * successfully configuring all of the associated audio components (e.g.,
 * microphones, amplifiers, etc.).
 *
 * Note that the timed delays used in this function are necessary to
 * ensure reliable operation of the Voice CODEC and Stereo DAC. The
 * Stereo DAC seems to be particularly sensitive and it has been observed
 * to fail to generate the required master mode clock signals if it is
 * not allowed enough time to initialize properly.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the device was successful enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_ERROR           If the device could not be enabled.
 */
PMIC_STATUS pmic_audio_enable(const PMIC_AUDIO_HANDLE handle)
{
	const unsigned int AUDIO_BIAS_ENABLE = SET_BITS(regTX_AUD_AMPS,
							AUDIO_BIAS, 1);
	const unsigned int STDAC_ENABLE = SET_BITS(regST_DAC, ST_DAC_EN, 1);
	const unsigned int VCODEC_ENABLE = SET_BITS(regAUD_CODEC, CDC_EN, 1);
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_value = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		/* Must first set the audio bias bit to power up the audio circuits. */
		pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, AUDIO_BIAS_ENABLE,
			       AUDIO_BIAS_ENABLE);

		/* Pause for 1 ms to let the audio circuits ramp up. We must use
		 * TASK_UNINTERRUPTIBLE here because we want to delay for at least
		 * 1 ms.
		 *
		 * Note that we will pause for a minimum of 1 jiffie (i.e., kernel
		 * clock tick) if the minimum clock resolution is greater than 1 ms.
		 */
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout((delay_1ms > 0) ? delay_1ms : 1);

		/* Then we can enable the Stereo DAC. */
		rc = pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, STDAC_ENABLE,
				    STDAC_ENABLE);

		pmic_read_reg(PRIO_AUDIO, REG_ST_DAC, &reg_value);
		if (!(reg_value & SET_BITS(regST_DAC, ST_DAC_EN, 1))) {
			TRACEMSG("Failed to enable the Stereo DAC\n");
			rc = PMIC_ERROR;
		}
	} else if ((handle == vCodec.handle)
		   && (vCodec.handleState == HANDLE_IN_USE)) {
		/* Must first set the audio bias bit to power up the audio circuits. */
		pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, AUDIO_BIAS_ENABLE,
			       AUDIO_BIAS_ENABLE);

		/* Pause for 1 ms to let the audio circuits ramp up. We must use
		 * TASK_UNINTERRUPTIBLE here because we want to delay for at least
		 * 1 ms.
		 *
		 * Note that we will pause for a minimum of 1 jiffie (i.e., kernel
		 * clock tick) if the minimum clock resolution is greater than 1 ms.
		 */
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout((delay_1ms > 0) ? delay_1ms : 1);

		/* Then we can enable the Voice CODEC. */
		rc = pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC, VCODEC_ENABLE,
				    VCODEC_ENABLE);

		pmic_read_reg(PRIO_AUDIO, REG_AUD_CODEC, &reg_value);
		if (!(reg_value & SET_BITS(regAUD_CODEC, CDC_EN, 1))) {
			TRACEMSG("Failed to enable the Voice CODEC\n");
			rc = PMIC_ERROR;
		}
	}

	if (rc == PMIC_SUCCESS) {
		/* Pause for another 1 ms to let the hardware stabilize after
		 * enabling. We must use TASK_UNINTERRUPTIBLE here because we
		 * want to delay for at least 1 ms.
		 *
		 * Note that we will pause for a minimum of 1 jiffie (i.e., kernel
		 * clock tick) if the minimum clock resolution is greater than 1 ms.
		 */
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout((delay_1ms > 0) ? delay_1ms : 1);
	}

	/* Exit critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Disable the Stereo DAC or the Voice CODEC.
 *
 * Explicitly disable the Stereo DAC or the Voice CODEC to end audio
 * playback or recording as required.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the device was successful disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_ERROR           If the device could not be disabled.
 */
PMIC_STATUS pmic_audio_disable(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int AUDIO_BIAS_ENABLE = SET_BITS(regTX_AUD_AMPS,
							AUDIO_BIAS, 1);
	const unsigned int STDAC_DISABLE = SET_BITS(regST_DAC, ST_DAC_EN, 1);
	const unsigned int VCODEC_DISABLE = SET_BITS(regAUD_CODEC, CDC_EN, 1);

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {

		rc = pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, 0, STDAC_DISABLE);
	} else if ((handle == vCodec.handle)
		   && (vCodec.handleState == HANDLE_IN_USE)) {
		rc = pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC, 0,
				    VCODEC_DISABLE);
	}

	/* We can also power down all of the audio circuits to minimize power
	 * usage if no device handles are currently active.
	 */
	if ((stDAC.handleState == HANDLE_FREE) &&
	    (vCodec.handleState == HANDLE_FREE) &&
	    (extStereoIn.handleState == HANDLE_FREE)) {
		/* Yes, we can power down all of the audio circuits. */
		pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, 0,
			       AUDIO_BIAS_ENABLE);
	}

	/* Exit critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Reset the selected audio hardware control registers to their
 *        power on state.
 *
 * This resets all of the audio hardware control registers currently
 * associated with the device handle back to their power on states. For
 * example, if the handle is associated with the Stereo DAC and a
 * specific output port and output amplifiers, then this function will
 * reset all of those components to their initial power on state.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the reset operation was successful.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_ERROR           If the reset was unsuccessful.
 */
PMIC_STATUS pmic_audio_reset(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	rc = pmic_audio_reset_device(handle);

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Reset all audio hardware control registers to their power on state.
 *
 * This resets all of the audio hardware control registers back to their
 * power on states. Use this function with care since it also invalidates
 * (i.e., automatically closes) all currently opened device handles.
 *
 * @retval      PMIC_SUCCESS         If the reset operation was successful.
 * @retval      PMIC_ERROR           If the reset was unsuccessful.
 */
PMIC_STATUS pmic_audio_reset_all(void)
{
	PMIC_STATUS rc = PMIC_SUCCESS;

	/* We need a critical section here to maintain a consistent state. */
	down_interruptible(&mutex);

	/* First close all opened device handles, also deregisters callbacks. */
	pmic_audio_close_handle(stDAC.handle);
	pmic_audio_close_handle(vCodec.handle);
	pmic_audio_close_handle(extStereoIn.handle);

	if (pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, RESET_ST_DAC, PMIC_ALL_BITS)
	    != PMIC_SUCCESS) {
		rc = PMIC_ERROR;
	} else {
		/* Also reset the driver state information to match. Note that we
		 * keep the device handle and event callback settings unchanged
		 * since these don't affect the actual hardware and we rely on
		 * the user to explicitly close the handle or deregister callbacks
		 */
		stDAC.busID = AUDIO_DATA_BUS_2;
		stDAC.protocol = NORMAL_MSB_JUSTIFIED_MODE;
		stDAC.masterSlave = BUS_SLAVE_MODE;
		stDAC.numSlots = USE_2_TIMESLOTS;
		stDAC.clockIn = CLOCK_IN_CLKIN;
		stDAC.samplingRate = STDAC_RATE_44_1_KHZ;
		stDAC.clockFreq = STDAC_CLI_13MHZ;
		stDAC.invert = NO_INVERT;
		stDAC.timeslot = USE_TS0_TS1;
		stDAC.config = (PMIC_AUDIO_STDAC_CONFIG) 0;
	}

	if (pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC, RESET_AUD_CODEC,
			   PMIC_ALL_BITS) != PMIC_SUCCESS) {
		rc = PMIC_ERROR;
	} else {
		/* Also reset the driver state information to match. Note that we
		 * keep the device handle and event callback settings unchanged
		 * since these don't affect the actual hardware and we rely on
		 * the user to explicitly close the handle or deregister callbacks
		 */
		vCodec.busID = AUDIO_DATA_BUS_1;
		vCodec.protocol = NETWORK_MODE;
		vCodec.masterSlave = BUS_SLAVE_MODE;
		vCodec.numSlots = USE_4_TIMESLOTS;
		vCodec.clockIn = CLOCK_IN_CLKIN;
		vCodec.samplingRate = VCODEC_RATE_8_KHZ;
		vCodec.clockFreq = VCODEC_CLI_13MHZ;
		vCodec.invert = NO_INVERT;
		vCodec.timeslot = USE_TS0;
		vCodec.config = INPUT_HIGHPASS_FILTER | OUTPUT_HIGHPASS_FILTER;
	}

	if (pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS, RESET_RX_AUD_AMPS,
			   PMIC_ALL_BITS) != PMIC_SUCCESS) {
		rc = PMIC_ERROR;
	} else {
		/* Also reset the driver state information to match. */
		audioOutput.outputPort = (PMIC_AUDIO_OUTPUT_PORT) NULL;
		audioOutput.outputPGAGain = OUTPGA_GAIN_0DB;
		audioOutput.balanceLeftGain = BAL_GAIN_0DB;
		audioOutput.balanceRightGain = BAL_GAIN_0DB;
		audioOutput.monoAdderGain = MONOADD_GAIN_0DB;
		audioOutput.config = (PMIC_AUDIO_OUTPUT_CONFIG) 0;
	}

	if (pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, RESET_TX_AUD_AMPS,
			   PMIC_ALL_BITS) != PMIC_SUCCESS) {
		rc = PMIC_ERROR;
	} else {
		/* Also reset the driver state information to match. Note that we
		 * reset the vCodec fields since all of the input/recording
		 * devices are only connected to the Voice CODEC and are managed
		 * as part of the Voice CODEC state.
		 */
		vCodec.inputConfig = (PMIC_AUDIO_INPUT_CONFIG) NULL;
		vCodec.leftChannelMic.mic = NO_MIC;
		vCodec.leftChannelMic.micOnOff = MICROPHONE_OFF;
		vCodec.leftChannelMic.ampMode = CURRENT_TO_VOLTAGE;
		vCodec.leftChannelMic.gain = MIC_GAIN_0DB;
		vCodec.rightChannelMic.mic = NO_MIC;
		vCodec.rightChannelMic.micOnOff = MICROPHONE_OFF;
		vCodec.rightChannelMic.ampMode = AMP_OFF;
		vCodec.rightChannelMic.gain = MIC_GAIN_0DB;
	}

	/* Finally, also reset any global state variables. */
	headsetState = NO_HEADSET;
	ptt_button_level = 0;

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set the Audio callback function.
 *
 * Register a callback function that will be used to signal PMIC audio
 * events. For example, the OSS audio driver should register a callback
 * function in order to be notified of headset connect/disconnect events.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   func            A pointer to the callback function.
 * @param[in]   eventMask       A mask selecting events to be notified.
 *
 * @retval      PMIC_SUCCESS         If the callback was successfully
 *                                   registered.
 * @retval      PMIC_PARAMETER_ERROR If the handle or the eventMask is invalid.
 */
PMIC_STATUS pmic_audio_set_callback(const PMIC_AUDIO_HANDLE handle,
				    const PMIC_AUDIO_CALLBACK func,
				    const PMIC_AUDIO_EVENTS eventMask)
{
	unsigned long flags;
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	type_event_notification eventNotify;

	/* We need to start a critical section here to ensure a consistent state
	 * in case simultaneous calls to pmic_audio_set_callback() are made. In
	 * that case, we must serialize the calls to ensure that the "callback"
	 * and "eventMask" state variables are always consistent.
	 *
	 * Note that we don't actually need to acquire the spinlock until later
	 * when we are finally ready to update the "callback" and "eventMask"
	 * state variables which are shared with the interrupt handler.
	 */
	down_interruptible(&mutex);

	/* Verify that we have an active device handle, the callback function
	 * pointer is not NULL, and that the event mask is non-zero. This avoids
	 * any potential problems with invalid arguments when we later try to
	 * register a callback.
	 */
	if ((((handle == stDAC.handle) &&
	      (stDAC.handleState == HANDLE_IN_USE)) ||
	     ((handle == vCodec.handle) &&
	      (vCodec.handleState == HANDLE_IN_USE)) ||
	     ((handle == extStereoIn.handle) &&
	      (extStereoIn.handleState == HANDLE_IN_USE))) &&
	    (func != (PMIC_AUDIO_CALLBACK) NULL) && (eventMask != 0)) {
		/* Make sure that a callback hasn't already been registered for the
		 * current device handle.
		 */
		if (((handle == stDAC.handle) &&
		     (stDAC.callback != (PMIC_AUDIO_CALLBACK) NULL)) ||
		    ((handle == vCodec.handle) &&
		     (vCodec.callback != (PMIC_AUDIO_CALLBACK) NULL)) ||
		    ((handle == extStereoIn.handle) &&
		     (extStereoIn.callback != (PMIC_AUDIO_CALLBACK) NULL))) {
			/* Notify caller that a callback has already been registered for
			 * the current device handle.
			 */
			TRACEMSG
			    ("%s: Another callback has already been registered\n",
			     __FILE__);
			rc = PMIC_ERROR;
		} else {
			/* Register for PMIC events from the core protocol driver. */
			if ((eventMask & HEADSET_DETECTED) ||
			    (eventMask & HEADSET_STEREO) ||
			    (eventMask & HEADSET_MONO) ||
			    (eventMask & HEADSET_REMOVED)) {
				/* We need to register for the A1 amplifier interrupt. */
				eventNotify.event = EVENT_A1I;
				eventNotify.callback = pmic_audio_event_handler;
				eventNotify.param = (void *)(CORE_EVENT_A1I);
				rc = pmic_event_subscribe(eventNotify);

				if (rc != PMIC_SUCCESS) {
					TRACEMSG
					    ("%s: pmic_event_subscribe() for EVENT_A1I "
					     "failed\n", __FILE__);
					goto End;
				}
				TRACEMSG(KERN_INFO
					 "Registered for EVENT_A1I\n");
			}

			if ((eventMask & PTT_BUTTON_PRESS) ||
			    (eventMask & PTT_BUTTON_RANGE) ||
			    (eventMask & PTT_SHORT_OR_INVALID)) {
				/* We need to register for the Push-to-Talk (PTT) interrupt. */
				eventNotify.event = EVENT_PTTI;
				eventNotify.callback = pmic_audio_event_handler;
				eventNotify.param = (void *)(CORE_EVENT_PTT);
				rc = pmic_event_subscribe(eventNotify);

				if (rc != PMIC_SUCCESS) {
					TRACEMSG
					    ("%s: pmic_event_subscribe() for EVENT_PTTI"
					     "failed\n", __FILE__);
					goto Cleanup_PTT;
				}
				TRACEMSG(KERN_INFO
					 "Registered for EVENT_PTTI\n");
			}

			/* We also need the spinlock here to avoid possible problems
			 * with the interrupt handler tasklet when we update the
			 * "callback" and "eventMask" state variables.
			 */
			spin_lock_irqsave(&lock, flags);

			/* Successfully registered for all events. */
			if (handle == stDAC.handle) {
				stDAC.callback = func;
				stDAC.eventMask = eventMask;
			} else if (handle == vCodec.handle) {
				vCodec.callback = func;
				vCodec.eventMask = eventMask;
			} else if (handle == extStereoIn.handle) {
				extStereoIn.callback = func;
				extStereoIn.eventMask = eventMask;
			}

			/* The spinlock is no longer needed now that we've finished
			 * updating the "callback" and "eventMask" state variables.
			 */
			spin_unlock_irqrestore(&lock, flags);
		}
	}

	goto End;

	/* This section unregisters any already registered events if we should
	 * encounter an error partway through the registration process. Note
	 * that we don't check the return status here since it is already set
	 * to PMIC_ERROR before we get here.
	 */
      Cleanup_PTT:

	if ((eventMask & HEADSET_DETECTED) ||
	    (eventMask & HEADSET_STEREO) ||
	    (eventMask & HEADSET_MONO) || (eventMask & HEADSET_REMOVED)) {
		eventNotify.event = EVENT_A1I;
		eventNotify.callback = pmic_audio_event_handler;
		eventNotify.param = (void *)(CORE_EVENT_A1I);
		pmic_event_unsubscribe(eventNotify);
	}

      End:
	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Deregisters the existing audio callback function.
 *
 * Deregister the callback function that was previously registered by calling
 * pmic_audio_set_callback().
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the callback was successfully
 *                                   deregistered.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_audio_clear_callback(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* We need a critical section to maintain a consistent state. */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) &&
	    (stDAC.handleState == HANDLE_IN_USE) &&
	    (stDAC.callback != (PMIC_AUDIO_CALLBACK) NULL)) {
		rc = pmic_audio_deregister(&(stDAC.callback),
					   &(stDAC.eventMask));
	} else if ((handle == vCodec.handle)
		   && (vCodec.handleState == HANDLE_IN_USE)
		   && (vCodec.callback != (PMIC_AUDIO_CALLBACK) NULL)) {
		rc = pmic_audio_deregister(&(vCodec.callback),
					   &(vCodec.eventMask));
	} else if ((handle == extStereoIn.handle)
		   && (extStereoIn.handleState == HANDLE_IN_USE)
		   && (extStereoIn.callback != (PMIC_AUDIO_CALLBACK) NULL)) {
		rc = pmic_audio_deregister(&(extStereoIn.callback),
					   &(extStereoIn.eventMask));
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current audio callback function settings.
 *
 * Get the current callback function and event mask.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  func            The current callback function.
 * @param[out]  eventMask       The current event selection mask.
 *
 * @retval      PMIC_SUCCESS         If the callback information was
 *                                   successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_audio_get_callback(const PMIC_AUDIO_HANDLE handle,
				    PMIC_AUDIO_CALLBACK * const func,
				    PMIC_AUDIO_EVENTS * const eventMask)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* We only need to acquire the mutex here because we will not be updating
	 * anything that may affect the interrupt handler. We just need to ensure
	 * that the callback fields are not changed while we are in the critical
	 * section by calling either pmic_audio_set_callback() or
	 * pmic_audio_clear_callback().
	 */
	down_interruptible(&mutex);

	if ((func != (PMIC_AUDIO_CALLBACK *) NULL) &&
	    (eventMask != (PMIC_AUDIO_EVENTS *) NULL)) {
		if ((handle == stDAC.handle) &&
		    (stDAC.handleState == HANDLE_IN_USE)) {
			*func = stDAC.callback;
			*eventMask = stDAC.eventMask;

			rc = PMIC_SUCCESS;
		} else if ((handle == vCodec.handle) &&
			   (vCodec.handleState == HANDLE_IN_USE)) {
			*func = vCodec.callback;
			*eventMask = vCodec.eventMask;

			rc = PMIC_SUCCESS;
		} else if ((handle == extStereoIn.handle) &&
			   (extStereoIn.handleState == HANDLE_IN_USE)) {
			*func = extStereoIn.callback;
			*eventMask = extStereoIn.eventMask;

			rc = PMIC_SUCCESS;
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Enable the anti-pop circuitry to avoid extra noise when inserting
 *        or removing a external device (e.g., a headset).
 *
 * Enable the use of the built-in anti-pop circuitry to prevent noise from
 * being generated when an external audio device is inserted or removed
 * from an audio plug. A slow ramp speed may be needed to avoid extra noise.
 *
 * @param[in]   rampSpeed       The desired anti-pop circuitry ramp speed.
 *
 * @retval      PMIC_SUCCESS         If the anti-pop circuitry was successfully
 *                                   enabled.
 * @retval      PMIC_ERROR           If the anti-pop circuitry could not be
 *                                   enabled.
 */
PMIC_STATUS pmic_audio_antipop_enable(const PMIC_AUDIO_ANTI_POP_RAMP_SPEED
				      rampSpeed)
{
	PMIC_STATUS rc = PMIC_ERROR;
	const unsigned int reg_write = SET_BITS(regTX_AUD_AMPS, AUDIO_BIAS, 1);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	/* For the sc55112 PMIC, the anti-pop circuitry is enabled by simply
	 * setting the AUDIO_BIAS bit in the TX_AUD_AMPS register. We ignore the
	 * ramp speed parameter since the ramp speed is fixed by the hardware.
	 */
	rc = pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, reg_write, reg_write);

	return rc;
}

/*!
 * @brief Disable the anti-pop circuitry.
 *
 * Disable the use of the built-in anti-pop circuitry to prevent noise from
 * being generated when an external audio device is inserted or removed
 * from an audio plug.
 *
 * @retval      PMIC_SUCCESS         If the anti-pop circuitry was successfully
 *                                   disabled.
 * @retval      PMIC_ERROR           If the anti-pop circuitry could not be
 *                                   disabled.
 */
PMIC_STATUS pmic_audio_antipop_disable(void)
{
	PMIC_STATUS rc = PMIC_ERROR;
	const unsigned int reg_mask = SET_BITS(regTX_AUD_AMPS, AUDIO_BIAS, 1);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	/* For the sc55112 PMIC, the anti-pop circuitry is disabled by simply
	 * clearing the AUDIO_BIAS bit in the TX_AUD_AMPS register.
	 */
	rc = pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, 0, reg_mask);

	return rc;
}

/*!
 * @brief Performs a reset of the Voice CODEC/Stereo DAC digital filter.
 *
 * The digital filter should be reset whenever the clock or sampling rate
 * configuration has been changed.
 *
 * Note that a special back-to-back SPI write sequence (see the sc55112
 * DTS document) is required in order to reliably reset the sc55112's
 * Voice CODEC digital filter. However, this special procedure is not
 * required for the Stereo DAC.
 *
 * @retval      PMIC_SUCCESS         If the digital filter was successfully
 *                                   reset.
 * @retval      PMIC_ERROR           If the digital filter could not be reset.
 */
PMIC_STATUS pmic_audio_digital_filter_reset(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write1 = 0;
	unsigned int reg_write2 = 0;
	unsigned int reg_mask = 0;

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		reg_write1 = SET_BITS(regST_DAC, DF_RESET_ST_DAC, 1);
		reg_mask = SET_BITS(regST_DAC, DF_RESET_ST_DAC, 1);

		if ((pmic_write_reg
		     (PRIO_AUDIO, REG_ST_DAC, reg_write1,
		      reg_mask) == PMIC_SUCCESS)) {
			rc = PMIC_SUCCESS;
		} else {
			rc = PMIC_ERROR;
		}
	} else if ((handle == vCodec.handle) &&
		   (vCodec.handleState == HANDLE_IN_USE)) {
		/* We must use the back-to-back SPI write procedure as documented in
		 * the sc55112 DTS document.
		 */
		reg_write1 = 0;
		reg_write2 = SET_BITS(regAUD_CODEC, CDC_EN, 1) |
		    SET_BITS(regAUD_CODEC, DF_RESET, 1);

		reg_mask = SET_BITS(regAUD_CODEC, CDC_EN, 1) |
		    SET_BITS(regAUD_CODEC, DF_RESET, 1);

		/* The first register write disables the Voice CODEC while the second
		 * register write triggers the digital filter reset.
		 */
		if ((pmic_write_reg
		     (PRIO_AUDIO, REG_AUD_CODEC, reg_write1,
		      reg_mask) == PMIC_SUCCESS)
		    &&
		    (pmic_write_reg
		     (PRIO_AUDIO, REG_AUD_CODEC, reg_write2,
		      reg_mask) == PMIC_SUCCESS)) {
			rc = PMIC_SUCCESS;
		} else {
			rc = PMIC_ERROR;
		}
	}

	if (rc == PMIC_SUCCESS) {
		/* Pause for 1 ms to let the hardware complete the filter reset.
		 * We must use TASK_UNINTERRUPTIBLE here because we want to delay
		 * for at least 1 ms.
		 *
		 * Note that we will pause for a minimum of 1 jiffie (i.e., kernel
		 * clock tick) if the minimum clock resolution is greater than 1 ms.
		 */
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout((delay_1ms > 0) ? delay_1ms : 1);
	}

	return rc;
}

/*!
 * @brief Get the most recent PTT button voltage reading.
 *
 * This function returns the most recent reading for the PTT button voltage.
 * The value may be used during the processing of the PTT_BUTTON_RANGE event
 * as part of the headset ID detection process.
 *
 * @retval      PMIC_SUCCESS         If the most recent PTT button voltage was
 *                                   returned.
 * @retval      PMIC_PARAMETER_ERROR If a NULL pointer argument was given.
 */
PMIC_STATUS pmic_audio_get_ptt_button_level(unsigned int *const level)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* No critical section is required here since all we care about is
	 * returning the most recent PTT button detect voltage level.
	 */

	if (level != (unsigned int *)NULL) {
		*level = ptt_button_level;

		rc = PMIC_SUCCESS;
	}

	return rc;
}

#ifdef DEBUG

/*!
 * @brief Provide a hexadecimal dump of all PMIC audio registers (DEBUG only)
 *
 * This function is intended strictly for debugging purposes only and will
 * print the current values of the following PMIC registers:
 *
 * - AUD_CODEC
 * - ST_DAC
 * - RX_AUD_AMPS
 * - TX_AUD_AMPS
 *
 * The register fields will also be decoded.
 *
 * Note that we don't dump any of the arbitration bits because we cannot
 * access the true arbitration bit settings when reading the registers
 * from the secondary SPI bus.
 *
 * Also note that we must not call this function with interrupts disabled,
 * for example, while holding a spinlock, because calls to pmic_read_reg()
 * eventually end up in the SPI driver which will want to perform a
 * schedule() operation. If schedule() is called with interrupts disabled,
 * then you will see messages like the following:
 *
 * BUG: scheduling while atomic: ...
 *
 */
void pmic_audio_dump_registers(void)
{
	unsigned int reg_value = 0;

	TRACEMSG(">>> PMIC Audio Register Dump:\n");

	/* Dump the AUD_CODEC (Voice CODEC) register. */
	if (pmic_read_reg(PRIO_AUDIO, REG_AUD_CODEC, &reg_value) ==
	    PMIC_SUCCESS) {
		TRACEMSG("\n  AUD_CODEC = 0x%08x\n", reg_value);

		/* Also decode the register fields. */
		TRACEMSG("               AUDIHPF = %d (%s)\n",
			 GET_BITS(regAUD_CODEC, AUDIHPF, reg_value),
			 (GET_BITS(regAUD_CODEC, AUDIHPF, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                   SMB = %d (%s Mode)\n",
			 GET_BITS(regAUD_CODEC, SMB, reg_value),
			 (GET_BITS(regAUD_CODEC, SMB, reg_value)) ?
			 "Slave" : "Master");
		TRACEMSG("               AUDOHPF = %d (%s)\n",
			 GET_BITS(regAUD_CODEC, AUDOHPF, reg_value),
			 (GET_BITS(regAUD_CODEC, AUDOHPF, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                 CD_TS = %d (%s)\n",
			 GET_BITS(regAUD_CODEC, CD_TS, reg_value),
			 (GET_BITS(regAUD_CODEC, CD_TS, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                   DLM = %d (%s)\n",
			 GET_BITS(regAUD_CODEC, DLM, reg_value),
			 (GET_BITS(regAUD_CODEC, DLM, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                 ADITH = %d (%s)\n",
			 GET_BITS(regAUD_CODEC, ADITH, reg_value),
			 (GET_BITS(regAUD_CODEC, ADITH, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("               CDC_CLK = 0x%x (",
			 GET_BITS(regAUD_CODEC, CDC_CLK, reg_value));
		switch (GET_BITS(regAUD_CODEC, CDC_CLK, reg_value)) {
		case 0:
			TRACEMSG("CLK_IN = 13.0 MHz");
			break;
		case 1:
			TRACEMSG("CLK_IN = 15.36 MHz");
			break;
		case 2:
			TRACEMSG("CLK_IN = 26 MHz");
			break;
		case 3:
			TRACEMSG("CLK_IN = 33.6 MHz");
			break;
		default:
			TRACEMSG("INVALID VALUE");
		}
		TRACEMSG(")\n");
		TRACEMSG("               CLK_INV = %d (%s)\n",
			 GET_BITS(regAUD_CODEC, CLK_INV, reg_value),
			 (GET_BITS(regAUD_CODEC, CLK_INV, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                FS_INV = %d (%s)\n",
			 GET_BITS(regAUD_CODEC, FS_INV, reg_value),
			 (GET_BITS(regAUD_CODEC, FS_INV, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG
		    ("              DF_RESET = %d (Should always read back as 0)\n",
		     GET_BITS(regAUD_CODEC, DF_RESET, reg_value));
		TRACEMSG("                CDC_EN = %d (%s)\n",
			 GET_BITS(regAUD_CODEC, CDC_EN, reg_value),
			 (GET_BITS(regAUD_CODEC, CDC_EN, reg_value)) ? "Enabled"
			 : "Disabled");
		TRACEMSG("            CDC_CLK_EN = %d (%s)\n",
			 GET_BITS(regAUD_CODEC, CDC_CLK_EN, reg_value),
			 (GET_BITS(regAUD_CODEC, CDC_CLK_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("             FS_8K_16K = %d (%d kHz sampling rate)\n",
			 GET_BITS(regAUD_CODEC, FS_8K_16K, reg_value),
			 (GET_BITS(regAUD_CODEC, FS_8K_16K, reg_value)) ? 16 :
			 8);
		TRACEMSG("            DIG_AUD_IN = %d (Digital Audio Bus %d)\n",
			 GET_BITS(regAUD_CODEC, DIG_AUD_IN, reg_value),
			 (GET_BITS(regAUD_CODEC, DIG_AUD_IN, reg_value)) ? 1 :
			 0);
		TRACEMSG("          CDC_SLOT_NMB = %d (use time slot %d)\n",
			 GET_BITS(regAUD_CODEC, CDC_SLOT_NMB, reg_value),
			 GET_BITS(regAUD_CODEC, CDC_SLOT_NMB, reg_value));
	} else {
		TRACEMSG("!!! ERROR: Failed to read AUD_CODEC register !!!\n");
	}

	/* Dump the ST_DAC (Stereo DAC) register. */
	if (pmic_read_reg(PRIO_AUDIO, REG_ST_DAC, &reg_value) == PMIC_SUCCESS) {
		TRACEMSG("\n  ST_DAC = 0x%08x\n", reg_value);

		/* Also decode the register fields. */
		TRACEMSG("            SMB_ST_DAC = %d (%s Mode)\n",
			 GET_BITS(regST_DAC, SMB_ST_DAC, reg_value),
			 (GET_BITS(regST_DAC, SMB_ST_DAC, reg_value)) ?
			 "Slave" : "Master");
		TRACEMSG("                ST_CLK = 0x%x (",
			 GET_BITS(regST_DAC, ST_CLK, reg_value));
		switch (GET_BITS(regST_DAC, ST_CLK, reg_value)) {
		case 0:
			TRACEMSG("CLK_IN = 13.0 MHz");
			break;
		case 1:
			TRACEMSG("CLK_IN = 15.36 MHz");
			break;
		case 2:
			TRACEMSG("CLK_IN = 26 MHz");
			break;
		case 3:
			TRACEMSG("CLK_IN = 33.6 MHz");
			break;
		case 5:
			TRACEMSG("MCLK pin, PLL disabled");
			break;
		case 6:
			TRACEMSG("FSYNC pin");
			break;
		case 7:
			TRACEMSG("BitCLK pin");
			break;
		default:
			TRACEMSG("INVALID VALUE");
		}
		TRACEMSG(")\n");
		TRACEMSG("             ST_CLK_EN = %d (%s)\n",
			 GET_BITS(regST_DAC, ST_CLK_EN, reg_value),
			 (GET_BITS(regST_DAC, ST_CLK_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG
		    ("       DF_RESET_ST_DAC = %d (Should always read back as 0)\n",
		     GET_BITS(regST_DAC, DF_RESET_ST_DAC, reg_value));
		TRACEMSG("             ST_DAC_EN = %d (%s)\n",
			 GET_BITS(regST_DAC, ST_DAC_EN, reg_value),
			 (GET_BITS(regST_DAC, ST_DAC_EN, reg_value)) ? "Enabled"
			 : "Disabled");
		TRACEMSG("                    SR = 0x%x (",
			 GET_BITS(regST_DAC, SR, reg_value));
		switch (GET_BITS(regST_DAC, SR, reg_value)) {
		case 0:
			TRACEMSG("8 kHz");
			break;
		case 1:
			TRACEMSG("11.025 kHz");
			break;
		case 2:
			TRACEMSG("12 kHz");
			break;
		case 3:
			TRACEMSG("16 kHz");
			break;
		case 4:
			TRACEMSG("22.05 kHz");
			break;
		case 5:
			TRACEMSG("24 kHz");
			break;
		case 6:
			TRACEMSG("32 kHz");
			break;
		case 7:
			TRACEMSG("44.1 kHz");
			break;
		case 8:
			TRACEMSG("48 kHz");
			break;
		default:
			TRACEMSG("INVALID VALUE");
		}
		TRACEMSG(")\n");
		TRACEMSG("     DIG_AUD_IN_ST_DAC = %d (Digital Audio Bus %d)\n",
			 GET_BITS(regST_DAC, DIG_AUD_IN_ST_DAC, reg_value),
			 (GET_BITS(regST_DAC, DIG_AUD_IN_ST_DAC, reg_value)) ? 1
			 : 0);
		TRACEMSG("            DIG_AUD_FS = 0x%x (",
			 GET_BITS(regST_DAC, DIG_AUD_FS, reg_value));
		switch (GET_BITS(regST_DAC, DIG_AUD_FS, reg_value)) {
		case 0:
			TRACEMSG("Normal/MSB Justified Mode");
			break;
		case 1:
			TRACEMSG("Time-slotted Network Mode");
			break;
		case 2:
			TRACEMSG("I2S Mode");
			break;
		default:
			TRACEMSG("INVALID VALUE");
		}
		TRACEMSG(")\n");
		TRACEMSG("                  BCLK = 0x%x (",
			 GET_BITS(regST_DAC, BCLK, reg_value));
		switch (GET_BITS(regST_DAC, BCLK, reg_value)) {
		case 1:
			TRACEMSG("8 time slots");
			break;
		case 2:
			TRACEMSG("4 time slots");
			break;
		case 3:
			TRACEMSG("2 time slots");
			break;
		default:
			TRACEMSG("INVALID VALUE");
		}
		TRACEMSG(")\n");
		TRACEMSG("            ST_CLK_INV = %d (%s)\n",
			 GET_BITS(regST_DAC, ST_CLK_INV, reg_value),
			 (GET_BITS(regST_DAC, ST_CLK_INV, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("             ST_FS_INV = %d (%s)\n",
			 GET_BITS(regST_DAC, ST_FS_INV, reg_value),
			 (GET_BITS(regST_DAC, ST_FS_INV, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("            AUD_MIX_EN = %d (%s)\n",
			 GET_BITS(regST_DAC, AUD_MIX_EN, reg_value),
			 (GET_BITS(regST_DAC, AUD_MIX_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("           AUD_MIX_LVL = %d (Gain = %s)\n",
			 GET_BITS(regST_DAC, AUD_MIX_LVL, reg_value),
			 (GET_BITS(regST_DAC, AUD_MIX_LVL, reg_value)) ?
			 "1" : "0.5");
	} else {
		TRACEMSG("!!! ERROR: Failed to read ST_DAC register !!!\n");
	}

	/* Dump the RX_AUD_AMPS (audio output section) register. */
	if (pmic_read_reg(PRIO_AUDIO, REG_RX_AUD_AMPS, &reg_value) ==
	    PMIC_SUCCESS) {
		TRACEMSG("\n  RX_AUD_AMPS = 0x%08x\n", reg_value);

		/* Also decode the register fields. */
		TRACEMSG("                 A1_EN = %d (%s)\n",
			 GET_BITS(regRX_AUD_AMPS, A1_EN, reg_value),
			 (GET_BITS(regRX_AUD_AMPS, A1_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                 A2_EN = %d (%s)\n",
			 GET_BITS(regRX_AUD_AMPS, A2_EN, reg_value),
			 (GET_BITS(regRX_AUD_AMPS, A2_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG
		    ("           A2_ADJ_LOCK = %d (Always reads back zero)\n",
		     GET_BITS(regRX_AUD_AMPS, A2_ADJ_LOCK, reg_value));
		TRACEMSG("                 A4_EN = %d (%s)\n",
			 GET_BITS(regRX_AUD_AMPS, A4_EN, reg_value),
			 (GET_BITS(regRX_AUD_AMPS, A4_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("             ARIGHT_EN = %d (%s)\n",
			 GET_BITS(regRX_AUD_AMPS, ARIGHT_EN, reg_value),
			 (GET_BITS(regRX_AUD_AMPS, ARIGHT_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("              ALEFT_EN = %d (%s)\n",
			 GET_BITS(regRX_AUD_AMPS, ALEFT_EN, reg_value),
			 (GET_BITS(regRX_AUD_AMPS, ALEFT_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                CD_BYP = %d (%s)\n",
			 GET_BITS(regRX_AUD_AMPS, CD_BYP, reg_value),
			 (GET_BITS(regRX_AUD_AMPS, CD_BYP, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                CDC_SW = %d (%s)\n",
			 GET_BITS(regRX_AUD_AMPS, CDC_SW, reg_value),
			 (GET_BITS(regRX_AUD_AMPS, CDC_SW, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("             ST_DAC_SW = %d (%s)\n",
			 GET_BITS(regRX_AUD_AMPS, ST_DAC_SW, reg_value),
			 (GET_BITS(regRX_AUD_AMPS, ST_DAC_SW, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("             PGA_IN_SW = %d (%s)\n",
			 GET_BITS(regRX_AUD_AMPS, PGA_IN_SW, reg_value),
			 (GET_BITS(regRX_AUD_AMPS, PGA_IN_SW, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                 AUDOG = 0x%x (",
			 GET_BITS(regRX_AUD_AMPS, AUDOG, reg_value));
		switch (GET_BITS(regRX_AUD_AMPS, AUDOG, reg_value)) {
		case 0:
			TRACEMSG("-24dB");
			break;
		case 1:
			TRACEMSG("-21dB");
			break;
		case 2:
			TRACEMSG("-18dB");
			break;
		case 3:
			TRACEMSG("-15dB");
			break;
		case 4:
			TRACEMSG("-12dB");
			break;
		case 5:
			TRACEMSG("-9dB");
			break;
		case 6:
			TRACEMSG("-6dB");
			break;
		case 7:
			TRACEMSG("-3dB");
			break;
		case 8:
			TRACEMSG("0dB");
			break;
		case 9:
			TRACEMSG("+3dB");
			break;
		case 10:
			TRACEMSG("+6dB");
			break;
		case 11:
			TRACEMSG("+9dB");
			break;
		case 12:
			TRACEMSG("+12dB");
			break;
		case 13:
			TRACEMSG("+15dB");
			break;
		case 14:
			TRACEMSG("+18dB");
			break;
		case 15:
			TRACEMSG("+21dB");
			break;
		default:
			TRACEMSG("INVALID VALUE");
		}
		TRACEMSG(")\n");
		TRACEMSG
		    ("                A1CTRL = %d (%s non-inverting A1 output)\n",
		     GET_BITS(regRX_AUD_AMPS, A1CTRL, reg_value),
		     (GET_BITS(regRX_AUD_AMPS, A1CTRL, reg_value)) ? "Disabled"
		     : "Enabled");
		TRACEMSG
		    ("               A1ID_RX = %d (Always reads back zero)\n",
		     GET_BITS(regRX_AUD_AMPS, A1ID_RX, reg_value));
		TRACEMSG("                  MONO = %d (",
			 GET_BITS(regRX_AUD_AMPS, MONO, reg_value));
		switch (GET_BITS(regRX_AUD_AMPS, MONO, reg_value)) {
		case 0:
			TRACEMSG("Separate Right/Left Channels");
			break;
		case 1:
			TRACEMSG("Right+Left");
			break;
		case 2:
			TRACEMSG("Right+Left -3dB");
			break;
		case 3:
			TRACEMSG("Right+Left -6dB");
			break;
		default:
			TRACEMSG("INVALID VALUE");
		}
		TRACEMSG(")\n");
		TRACEMSG
		    ("         AUDOG_PRI_ADJ = %d (Always reads back zero)\n",
		     GET_BITS(regRX_AUD_AMPS, AUDOG_PRI_ADJ, reg_value));
		TRACEMSG
		    ("          MONO_PRI_ADJ = %d (Always reads back zero)\n",
		     GET_BITS(regRX_AUD_AMPS, MONO_PRI_ADJ, reg_value));
		TRACEMSG
		    ("            RX_PRI_ADJ = %d (Always reads back zero)\n",
		     GET_BITS(regRX_AUD_AMPS, RX_PRI_ADJ, reg_value));
	} else {
		TRACEMSG
		    ("!!! ERROR: Failed to read RX_AUD_AMPS register !!!\n");
	}

	/* Dump the TX_AUD_AMPS (audio input section) register. */
	if (pmic_read_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, &reg_value) ==
	    PMIC_SUCCESS) {
		TRACEMSG("\n  TX_AUD_AMPS = 0x%08x\n", reg_value);

		/* Also decode the register fields. Again, note that the MB_ON1 and
		 * MB_ON2 bits are reversed from what is shown in the sc55112 DTS
		 * document up to and including Rev 3.0.
		 */
		TRACEMSG("                 AUDIG = %d (+%ddB)\n",
			 GET_BITS(regTX_AUD_AMPS, AUDIG, reg_value),
			 GET_BITS(regTX_AUD_AMPS, AUDIG, reg_value));
		TRACEMSG("                 A3_EN = %d (%s)\n",
			 GET_BITS(regTX_AUD_AMPS, A3_EN, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, A3_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                A3_MUX = %d (%s)\n",
			 GET_BITS(regTX_AUD_AMPS, A3_MUX, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, A3_MUX, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                 A5_EN = %d (%s)\n",
			 GET_BITS(regTX_AUD_AMPS, A5_EN, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, A5_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                A5_MUX = %d (%s)\n",
			 GET_BITS(regTX_AUD_AMPS, A5_MUX, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, A5_MUX, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("           EXT_MIC_MUX = %d (%s)\n",
			 GET_BITS(regTX_AUD_AMPS, EXT_MIC_MUX, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, EXT_MIC_MUX, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                MB_ON1 = %d (Microphone Bias1 %s)\n",
			 GET_BITS(regTX_AUD_AMPS, MB_ON1, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, MB_ON1, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                MB_ON2 = %d (Microphone Bias2 %s)\n",
			 GET_BITS(regTX_AUD_AMPS, MB_ON2, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, MB_ON2, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG
		    ("               A1ID_TX = %d (Always reads back zero)\n",
		     GET_BITS(regTX_AUD_AMPS, A1ID_TX, reg_value));
		TRACEMSG("             A1_CONFIG = %d (A1_EN %s)\n",
			 GET_BITS(regTX_AUD_AMPS, A1_CONFIG, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, A1_CONFIG, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("            V3_6MB_ON2 = %d (Phantom Ground %s)\n",
			 GET_BITS(regTX_AUD_AMPS, V3_6MB_ON2, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, V3_6MB_ON2, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("    MBUTTON_HS_ADET_EN = %d (%s)\n",
			 GET_BITS(regTX_AUD_AMPS, MBUTTON_HS_ADET_EN,
				  reg_value), (GET_BITS(regTX_AUD_AMPS,
							MBUTTON_HS_ADET_EN,
							reg_value)) ? "Enabled"
			 : "Disabled");
		TRACEMSG("        HS_SHRT_DET_EN = %d (%s)\n",
			 GET_BITS(regTX_AUD_AMPS, HS_SHRT_DET_EN, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, HS_SHRT_DET_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("            AUDIO_BIAS = %d (%s)\n",
			 GET_BITS(regTX_AUD_AMPS, AUDIO_BIAS, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, AUDIO_BIAS, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG
		    ("        AUDIO_BIAS_ARB = %d (Always reads back zero)\n",
		     GET_BITS(regTX_AUD_AMPS, AUDIO_BIAS_ARB, reg_value));
		TRACEMSG("      AUDIO_BIAS_SPEED = %d (%s)\n",
			 GET_BITS(regTX_AUD_AMPS, AUDIO_BIAS_SPEED, reg_value),
			 (GET_BITS(regTX_AUD_AMPS, AUDIO_BIAS_SPEED, reg_value))
			 ? "Fast" : "Slow");
		TRACEMSG
		    ("         AUDIG_PRI_ADJ = %d (Always reads back zero)\n",
		     GET_BITS(regTX_AUD_AMPS, AUDIG_PRI_ADJ, reg_value));
		TRACEMSG
		    ("            TX_PRI_ADJ = %d (Always reads back zero)\n",
		     GET_BITS(regTX_AUD_AMPS, TX_PRI_ADJ, reg_value));
	} else {
		TRACEMSG
		    ("!!! ERROR: Failed to read TX_AUD_AMPS register !!!\n");
	}

	/* Also dump SWCTRL (Switching Regulators) register. */
	if (pmic_read_reg(PRIO_PWM, REG_SWCTRL, &reg_value) == PMIC_SUCCESS) {
		TRACEMSG("\n  SWCTRL = 0x%08x\n", reg_value);

		/* Also decode the register fields. */
		TRACEMSG("                SW1_EN = %d (SW1 %s)\n",
			 GET_BITS(regSWCTRL, SW1_EN, reg_value),
			 (GET_BITS(regSWCTRL, SW1_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("              SW1_MODE = %d\n",
			 GET_BITS(regSWCTRL, SW1_MODE, reg_value));
		TRACEMSG("                   SW1 = 0x%x\n",
			 GET_BITS(regSWCTRL, SW1, reg_value));
		TRACEMSG("       SW1_STBY_LVS_EN = %d\n",
			 GET_BITS(regSWCTRL, SW1_STBY_LVS_EN, reg_value));
		TRACEMSG("              SW2_MODE = %d\n",
			 GET_BITS(regSWCTRL, SW2_MODE, reg_value));
		TRACEMSG("               SW1_LVS = 0x%x\n",
			 GET_BITS(regSWCTRL, SW1_LVS, reg_value));
		TRACEMSG("                SW3_EN = %d (SW3 %s)\n",
			 GET_BITS(regSWCTRL, SW3_EN, reg_value),
			 (GET_BITS(regSWCTRL, SW3_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("                   SW3 = %d (%sV)\n",
			 GET_BITS(regSWCTRL, SW3, reg_value),
			 GET_BITS(regSWCTRL, SW3, reg_value) ? "5.6" : "5.1");
		TRACEMSG("           SW3_STBY_EN = %d\n",
			 GET_BITS(regSWCTRL, SW3_STBY_EN, reg_value));
		TRACEMSG("                   SW2 = 0x%x\n",
			 GET_BITS(regSWCTRL, SW2, reg_value));
		TRACEMSG("               SW2_LVS = 0x%x\n",
			 GET_BITS(regSWCTRL, SW2_LVS, reg_value));
		TRACEMSG("              SW1_STBY = 0x%x\n",
			 GET_BITS(regSWCTRL, SW1_STBY, reg_value));
		TRACEMSG("                SW2_EN = %d (SW2 %s)\n",
			 GET_BITS(regSWCTRL, SW2_EN, reg_value),
			 (GET_BITS(regSWCTRL, SW2_EN, reg_value)) ?
			 "Enabled" : "Disabled");
		TRACEMSG("           SW2_DFVS_EN = %d (%s)\n",
			 GET_BITS(regSWCTRL, SW2_DFVS_EN, reg_value),
			 (GET_BITS(regSWCTRL, SW2_DFVS_EN, reg_value)) ?
			 "Enabled" : "Disabled");
	}
}

#endif				/* DEBUG */

/*@}*/

/*************************************************************************
 * General Voice CODEC configuration.
 *************************************************************************
 */

/*!
 * @name General Voice CODEC Setup and Configuration APIs
 * Functions for general setup and configuration of the PMIC Voice
 * CODEC hardware.
 */
/*@{*/

/*!
 * @brief Set the Voice CODEC clock source and operating characteristics.
 *
 * Define the Voice CODEC clock source and operating characteristics. This
 * must be done before the Voice CODEC is enabled. The Voice CODEC will be
 * in a disabled state when this function returns.
 *
 * Note that we automatically perform a digital filter reset operation
 * when we change the clock configuration.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   clockIn         Select the clock signal source.
 * @param[in]   clockFreq       Select the clock signal frequency.
 * @param[in]   samplingRate    Select the audio data sampling rate.
 * @param[in]   invert          Enable inversion of the frame sync and/or
 *                              bit clock inputs.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC clock settings were
 *                                   successfully configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or clock configuration was
 *                                   invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC clock configuration
 *                                   could not be set.
 */
PMIC_STATUS pmic_audio_vcodec_set_clock(const PMIC_AUDIO_HANDLE handle,
					const PMIC_AUDIO_CLOCK_IN_SOURCE
					clockIn,
					const PMIC_AUDIO_VCODEC_CLOCK_IN_FREQ
					clockFreq,
					const PMIC_AUDIO_VCODEC_SAMPLING_RATE
					samplingRate,
					const PMIC_AUDIO_CLOCK_INVERT invert)
{
	const unsigned int VCODEC_DISABLE = SET_BITS(regAUD_CODEC, CDC_EN, 1);
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	/* Validate all of the calling parameters. */
	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    ((clockIn == CLOCK_IN_DEFAULT) ||
	     (clockIn == CLOCK_IN_CLKIN)) &&
	    ((clockFreq == VCODEC_CLI_13MHZ) ||
	     (clockFreq == VCODEC_CLI_15_36MHZ) ||
	     (clockFreq == VCODEC_CLI_26MHZ) ||
	     (clockFreq == VCODEC_CLI_33_6MHZ)) &&
	    ((samplingRate == VCODEC_RATE_8_KHZ) ||
	     (samplingRate == VCODEC_RATE_16_KHZ)) &&
	    !((clockFreq == VCODEC_CLI_13MHZ) &&
	      (samplingRate == VCODEC_RATE_16_KHZ))) {
		/* Note that for the sc55112, the clockIn parameter is a
		 * no-op since the hardware defaults to using only CLK_IN.
		 */

		/* Start by disabling the Voice CODEC and any existing clock output. */
		pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC, 0, VCODEC_DISABLE);

		/* Pause briefly to ensure that the Voice CODEC is completely
		 * disabled before changing the clock configuration.
		 */
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout((delay_1ms > 0) ? delay_1ms : 1);

		/* Set the CDC_CLK bits to match the input clock frequency. */
		reg_mask |= SET_BITS(regAUD_CODEC, CDC_CLK, 7);
		if (clockFreq == VCODEC_CLI_15_36MHZ) {
			reg_write |= SET_BITS(regAUD_CODEC, CDC_CLK, 1);
		} else if (clockFreq == VCODEC_CLI_26MHZ) {
			reg_write |= SET_BITS(regAUD_CODEC, CDC_CLK, 2);
		} else if (clockFreq == VCODEC_CLI_33_6MHZ) {
			reg_write |= SET_BITS(regAUD_CODEC, CDC_CLK, 3);
		}

		/* Set the desired sampling rate. */
		reg_mask |= SET_BITS(regAUD_CODEC, FS_8K_16K, 1);
		if (samplingRate == VCODEC_RATE_16_KHZ) {
			reg_write |= SET_BITS(regAUD_CODEC, FS_8K_16K, 1);
		}

		/* Set the optional clock inversion. */
		reg_mask |= SET_BITS(regAUD_CODEC, CLK_INV, 1);
		if (invert & INVERT_BITCLOCK) {
			reg_write |= SET_BITS(regAUD_CODEC, CLK_INV, 1);
		}

		/* Set the optional frame sync inversion. */
		reg_mask |= SET_BITS(regAUD_CODEC, FS_INV, 1);
		if (invert & INVERT_FRAMESYNC) {
			reg_write |= SET_BITS(regAUD_CODEC, FS_INV, 1);
		}

		/* Write the configuration to the AUD_CODEC register. */
		rc = pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC, reg_write,
				    reg_mask);

		if (rc == PMIC_SUCCESS) {
			/* Also reset the digital filter now that we've changed the
			 * clock configuration.
			 */
			pmic_audio_digital_filter_reset(handle);

			vCodec.clockIn = clockIn;
			vCodec.clockFreq = clockFreq;
			vCodec.samplingRate = samplingRate;
			vCodec.invert = invert;
		}
	} else if (clockFreq == VCODEC_CLI_16_8MHZ) {
		/* sc55112 does not support a 16.8 MHz input clock frequency. */
		rc = PMIC_NOT_SUPPORTED;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the Voice CODEC clock source and operating characteristics.
 *
 * Get the current Voice CODEC clock source and operating characteristics.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  clockIn         The clock signal source.
 * @param[out]  clockFreq       The clock signal frequency.
 * @param[out]  samplingRate    The audio data sampling rate.
 * @param[out]  invert          Inversion of the frame sync and/or
 *                              bit clock inputs is enabled/disabled.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC clock settings were
 *                                   successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC clock configuration
 *                                   could not be retrieved.
 */
PMIC_STATUS pmic_audio_vcodec_get_clock(const PMIC_AUDIO_HANDLE handle,
					PMIC_AUDIO_CLOCK_IN_SOURCE *
					const clockIn,
					PMIC_AUDIO_VCODEC_CLOCK_IN_FREQ *
					const clockFreq,
					PMIC_AUDIO_VCODEC_SAMPLING_RATE *
					const samplingRate,
					PMIC_AUDIO_CLOCK_INVERT * const invert)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure that we return a consistent state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    (clockIn != (PMIC_AUDIO_CLOCK_IN_SOURCE *) NULL) &&
	    (clockFreq != (PMIC_AUDIO_VCODEC_CLOCK_IN_FREQ *) NULL) &&
	    (samplingRate != (PMIC_AUDIO_VCODEC_SAMPLING_RATE *) NULL) &&
	    (invert != (PMIC_AUDIO_CLOCK_INVERT *) NULL)) {
		*clockIn = vCodec.clockIn;
		*clockFreq = vCodec.clockFreq;
		*samplingRate = vCodec.samplingRate;
		*invert = vCodec.invert;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set the Voice CODEC primary audio channel timeslot.
 *
 * Set the Voice CODEC primary audio channel timeslot. This function must be
 * used if the default timeslot for the primary audio channel is to be changed.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   timeslot        Select the primary audio channel timeslot.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC primary audio channel
 *                                   timeslot was successfully configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or audio channel timeslot
 *                                   was invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC primary audio channel
 *                                   timeslot could not be set.
 */
PMIC_STATUS pmic_audio_vcodec_set_rxtx_timeslot(const PMIC_AUDIO_HANDLE handle,
						const PMIC_AUDIO_VCODEC_TIMESLOT
						timeslot)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	const unsigned int reg_mask = SET_BITS(regAUD_CODEC, CDC_SLOT_NMB, 3);

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    ((timeslot == USE_TS0) || (timeslot == USE_TS1) ||
	     (timeslot == USE_TS2) || (timeslot == USE_TS3))) {
		reg_write = SET_BITS(regAUD_CODEC, CDC_SLOT_NMB, timeslot);

		rc = pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC, reg_write,
				    reg_mask);

		if (rc == PMIC_SUCCESS) {
			vCodec.timeslot = timeslot;
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current Voice CODEC primary audio channel timeslot.
 *
 * Get the current Voice CODEC primary audio channel timeslot.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  timeslot        The primary audio channel timeslot.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC primary audio channel
 *                                   timeslot was successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC primary audio channel
 *                                   timeslot could not be retrieved.
 */
PMIC_STATUS pmic_audio_vcodec_get_rxtx_timeslot(const PMIC_AUDIO_HANDLE handle,
						PMIC_AUDIO_VCODEC_TIMESLOT *
						const timeslot)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    (timeslot != (PMIC_AUDIO_VCODEC_TIMESLOT *) NULL)) {
		*timeslot = vCodec.timeslot;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set the Voice CODEC secondary recording audio channel timeslot.
 *
 * Set the Voice CODEC secondary audio channel timeslot. This function must be
 * used if the default timeslot for the secondary audio channel is to be
 * changed. The secondary audio channel timeslot is used to transmit the audio
 * data that was recorded by the Voice CODEC from the secondary audio input
 * channel.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   timeslot        Select the secondary audio channel timeslot.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC secondary audio channel
 *                                   timeslot was successfully configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or audio channel timeslot
 *                                   was invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC secondary audio channel
 *                                   timeslot could not be set.
 */
PMIC_STATUS pmic_audio_vcodec_set_secondary_txslot(const PMIC_AUDIO_HANDLE
						   handle,
						   const
						   PMIC_AUDIO_VCODEC_TIMESLOT
						   timeslot)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		/* The sc55112 PMIC Voice CODEC only supports mono recording. */
		rc = PMIC_NOT_SUPPORTED;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the Voice CODEC secondary recording audio channel timeslot.
 *
 * Get the Voice CODEC secondary audio channel timeslot.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  timeslot        The secondary audio channel timeslot.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC secondary audio channel
 *                                   timeslot was successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC secondary audio channel
 *                                   timeslot could not be retrieved.
 */
PMIC_STATUS pmic_audio_vcodec_get_secondary_txslot(const PMIC_AUDIO_HANDLE
						   handle,
						   PMIC_AUDIO_VCODEC_TIMESLOT *
						   const timeslot)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    (timeslot != (PMIC_AUDIO_VCODEC_TIMESLOT *) NULL)) {
		rc = PMIC_NOT_SUPPORTED;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set/Enable the Voice CODEC options.
 *
 * Set or enable various Voice CODEC options. The available options include
 * the use of dithering, highpass digital filters, and loopback modes.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   config          The Voice CODEC options to enable.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC options were
 *                                   successfully configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or Voice CODEC options
 *                                   were invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC options could not be
 *                                   successfully set/enabled.
 */
PMIC_STATUS pmic_audio_vcodec_set_config(const PMIC_AUDIO_HANDLE handle,
					 const PMIC_AUDIO_VCODEC_CONFIG config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    !(config & ANALOG_LOOPBACK)) {
		if (config & DITHERING) {
			reg_write = SET_BITS(regAUD_CODEC, ADITH, 1);
			reg_mask = SET_BITS(regAUD_CODEC, ADITH, 1);
		}

		if (config & INPUT_HIGHPASS_FILTER) {
			reg_write |= SET_BITS(regAUD_CODEC, AUDIHPF, 1);
			reg_mask |= SET_BITS(regAUD_CODEC, AUDIHPF, 1);
		}

		if (config & OUTPUT_HIGHPASS_FILTER) {
			reg_write |= SET_BITS(regAUD_CODEC, AUDOHPF, 1);
			reg_mask |= SET_BITS(regAUD_CODEC, AUDOHPF, 1);
		}

		if (config & DIGITAL_LOOPBACK) {
			reg_write |= SET_BITS(regAUD_CODEC, DLM, 1);
			reg_mask |= SET_BITS(regAUD_CODEC, DLM, 1);
		}

		if (config & VCODEC_MASTER_CLOCK_OUTPUTS) {
			reg_write |= SET_BITS(regAUD_CODEC, CDC_CLK_EN, 1);
			reg_mask |= SET_BITS(regAUD_CODEC, CDC_CLK_EN, 1);
		}

		if (config & TRISTATE_TS) {
			reg_write |= SET_BITS(regAUD_CODEC, CD_TS, 1);
			reg_mask |= SET_BITS(regAUD_CODEC, CD_TS, 1);
		}

		if (reg_mask == 0) {
			/* We should not reach this point without having to configure
			 * anything so we flag it as an error.
			 */
			rc = PMIC_ERROR;
		} else {
			rc = pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC,
					    reg_write, reg_mask);
		}

		if (rc == PMIC_SUCCESS) {
			vCodec.config |= config;
		}
	} else if (config & ANALOG_LOOPBACK) {
		rc = PMIC_NOT_SUPPORTED;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Clear/Disable the Voice CODEC options.
 *
 * Clear or disable various Voice CODEC options.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   config          The Voice CODEC options to be cleared/disabled.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC options were
 *                                   successfully cleared/disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or the Voice CODEC options
 *                                   were invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC options could not be
 *                                   cleared/disabled.
 */
PMIC_STATUS pmic_audio_vcodec_clear_config(const PMIC_AUDIO_HANDLE handle,
					   const PMIC_AUDIO_VCODEC_CONFIG
					   config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    !(config & ANALOG_LOOPBACK)) {
		if (config & DITHERING) {
			reg_mask = SET_BITS(regAUD_CODEC, ADITH, 1);
		}

		if (config & INPUT_HIGHPASS_FILTER) {
			reg_mask |= SET_BITS(regAUD_CODEC, AUDIHPF, 1);
		}

		if (config & OUTPUT_HIGHPASS_FILTER) {
			reg_mask |= SET_BITS(regAUD_CODEC, AUDOHPF, 1);
		}

		if (config & DIGITAL_LOOPBACK) {
			reg_mask |= SET_BITS(regAUD_CODEC, DLM, 1);
		}

		if (config & VCODEC_MASTER_CLOCK_OUTPUTS) {
			reg_mask |= SET_BITS(regAUD_CODEC, CDC_CLK_EN, 1);
		}

		if (config & TRISTATE_TS) {
			reg_mask |= SET_BITS(regAUD_CODEC, CD_TS, 1);
		}

		if (reg_mask == 0) {
			/* We can reach this point without having to write anything
			 * to the AUD_CODEC register since there is a possible no-op
			 * for the VCODEC_RESET_DIGITAL_FILTER option. Therefore, we
			 * just assume success and proceed.
			 */
			rc = PMIC_SUCCESS;
		} else {
			rc = pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC,
					    reg_write, reg_mask);
		}

		if (rc == PMIC_SUCCESS) {
			vCodec.config &= ~config;
		}
	} else if (config & ANALOG_LOOPBACK) {
		rc = PMIC_NOT_SUPPORTED;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current Voice CODEC options.
 *
 * Get the current Voice CODEC options.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  config          The current set of Voice CODEC options.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC options were
 *                                   successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC options could not be
 *                                   retrieved.
 */
PMIC_STATUS pmic_audio_vcodec_get_config(const PMIC_AUDIO_HANDLE handle,
					 PMIC_AUDIO_VCODEC_CONFIG *
					 const config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    (config != (PMIC_AUDIO_VCODEC_CONFIG *) NULL)) {
		*config = vCodec.config;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Enable the Voice CODEC bypass audio pathway.
 *
 * Enables the Voice CODEC bypass pathway for audio data. This allows direct
 * output of the voltages on the TX data bus line to the output amplifiers
 * (bypassing the digital-to-analog converters within the Voice CODEC).
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC bypass was successfully
 *                                   enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC bypass could not be
 *                                   enabled.
 */
PMIC_STATUS pmic_audio_vcodec_enable_bypass(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int reg_write = SET_BITS(regRX_AUD_AMPS, CD_BYP, 1);
	const unsigned int reg_mask = SET_BITS(regRX_AUD_AMPS, CD_BYP, 1);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS, reg_write,
				    reg_mask);
	}

	return rc;
}

/*!
 * @brief Disable the Voice CODEC bypass audio pathway.
 *
 * Disables the Voice CODEC bypass pathway for audio data. This means that
 * the TX data bus line will deliver digital data to the digital-to-analog
 * converters within the Voice CODEC.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC bypass was successfully
 *                                   disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC bypass could not be
 *                                   disabled.
 */
PMIC_STATUS pmic_audio_vcodec_disable_bypass(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int reg_write = 0;
	const unsigned int reg_mask = SET_BITS(regRX_AUD_AMPS, CD_BYP, 1);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS, reg_write,
				    reg_mask);
	}

	return rc;
}

/*@}*/

/*************************************************************************
 * General Stereo DAC configuration.
 *************************************************************************
 */

/*!
 * @name General Stereo DAC Setup and Configuration APIs
 * Functions for general setup and configuration of the PMIC Stereo
 * DAC hardware.
 */
/*@{*/

/*!
 * @brief Set the Stereo DAC clock source and operating characteristics.
 *
 * Define the Stereo DAC clock source and operating characteristics. This
 * must be done before the Stereo DAC is enabled. The Stereo DAC will be
 * in a disabled state when this function returns.
 *
 * Note that we automatically perform a digital filter reset operation
 * when we change the clock configuration.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   clockIn         Select the clock signal source.
 * @param[in]   clockFreq       Select the clock signal frequency.
 * @param[in]   samplingRate    Select the audio data sampling rate.
 * @param[in]   invert          Enable inversion of the frame sync and/or
 *                              bit clock inputs.
 *
 * @retval      PMIC_SUCCESS         If the Stereo DAC clock settings were
 *                                   successfully configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or clock configuration was
 *                                   invalid.
 * @retval      PMIC_ERROR           If the Stereo DAC clock configuration
 *                                   could not be set.
 */
PMIC_STATUS pmic_audio_stdac_set_clock(const PMIC_AUDIO_HANDLE handle,
				       const PMIC_AUDIO_CLOCK_IN_SOURCE clockIn,
				       const PMIC_AUDIO_STDAC_CLOCK_IN_FREQ
				       clockFreq,
				       const PMIC_AUDIO_STDAC_SAMPLING_RATE
				       samplingRate,
				       const PMIC_AUDIO_CLOCK_INVERT invert)
{
	const unsigned int STDAC_DISABLE = SET_BITS(regST_DAC, ST_DAC_EN, 1);
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	/* Validate all of the calling parameters. */
	if ((handle == stDAC.handle) &&
	    (stDAC.handleState == HANDLE_IN_USE) &&
	    ((clockIn == CLOCK_IN_DEFAULT) ||
	     (clockIn == CLOCK_IN_CLKIN) ||
	     (clockIn == CLOCK_IN_MCLK) ||
	     (clockIn == CLOCK_IN_FSYNC) ||
	     (clockIn == CLOCK_IN_BITCLK)) &&
	    ((clockFreq == STDAC_CLI_13MHZ) ||
	     (clockFreq == STDAC_CLI_15_36MHZ) ||
	     (clockFreq == STDAC_CLI_26MHZ) ||
	     (clockFreq == STDAC_CLI_33_6MHZ) ||
	     (clockFreq == STDAC_MCLK_PLL_DISABLED) ||
	     (clockFreq == STDAC_FSYNC_IN_PLL) ||
	     (clockFreq == STDAC_BCLK_IN_PLL)) &&
	    ((samplingRate >= STDAC_RATE_8_KHZ) &&
	     (samplingRate <= STDAC_RATE_48_KHZ))) {
		/* Also verify that the clock input source and frequency are valid. */
		if (((clockIn == CLOCK_IN_CLKIN) &&
		     (clockFreq != STDAC_CLI_13MHZ) &&
		     (clockFreq != STDAC_CLI_15_36MHZ) &&
		     (clockFreq != STDAC_CLI_26MHZ) &&
		     (clockFreq != STDAC_CLI_33_6MHZ)) ||
		    ((clockIn == CLOCK_IN_MCLK) &&
		     (clockFreq != STDAC_MCLK_PLL_DISABLED)) ||
		    ((clockIn == CLOCK_IN_FSYNC) &&
		     (clockFreq != STDAC_FSYNC_IN_PLL)) ||
		    ((clockIn == CLOCK_IN_BITCLK) &&
		     (clockFreq != STDAC_BCLK_IN_PLL))) {
			rc = PMIC_PARAMETER_ERROR;
		} else {
			/* Start by disabling the Stereo DAC and any existing clock
			 * output.
			 */
			pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, 0,
				       STDAC_DISABLE);

			/* Pause briefly to ensure that the Stereo DAC is completely
			 * disabled before changing the clock configuration.
			 */
			set_current_state(TASK_UNINTERRUPTIBLE);
			schedule_timeout((delay_1ms > 0) ? delay_1ms : 1);

			/* Set the ST_CLK bits to match the input clock source and
			 * frequency.
			 */
			reg_mask |= SET_BITS(regST_DAC, ST_CLK, 7);

			if (clockFreq == STDAC_CLI_15_36MHZ) {
				reg_write |= SET_BITS(regST_DAC, ST_CLK, 1);
			} else if (clockFreq == STDAC_CLI_26MHZ) {
				reg_write |= SET_BITS(regST_DAC, ST_CLK, 2);
			} else if (clockFreq == STDAC_CLI_33_6MHZ) {
				reg_write |= SET_BITS(regST_DAC, ST_CLK, 3);
			} else if (clockFreq == STDAC_MCLK_PLL_DISABLED) {
				reg_write |= SET_BITS(regST_DAC, ST_CLK, 5);
			} else if (clockFreq == STDAC_FSYNC_IN_PLL) {
				reg_write |= SET_BITS(regST_DAC, ST_CLK, 6);
			} else if (clockFreq == STDAC_BCLK_IN_PLL) {
				reg_write |= SET_BITS(regST_DAC, ST_CLK, 7);
			}

			/* Set the desired sampling rate. */
			reg_mask |= SET_BITS(regST_DAC, SR, 15);
			if (samplingRate == STDAC_RATE_11_025_KHZ) {
				reg_write |= SET_BITS(regST_DAC, SR, 1);
			} else if (samplingRate == STDAC_RATE_12_KHZ) {
				reg_write |= SET_BITS(regST_DAC, SR, 2);
			} else if (samplingRate == STDAC_RATE_16_KHZ) {
				reg_write |= SET_BITS(regST_DAC, SR, 3);
			} else if (samplingRate == STDAC_RATE_22_050_KHZ) {
				reg_write |= SET_BITS(regST_DAC, SR, 4);
			} else if (samplingRate == STDAC_RATE_24_KHZ) {
				reg_write |= SET_BITS(regST_DAC, SR, 5);
			} else if (samplingRate == STDAC_RATE_32_KHZ) {
				reg_write |= SET_BITS(regST_DAC, SR, 6);
			} else if (samplingRate == STDAC_RATE_44_1_KHZ) {
				reg_write |= SET_BITS(regST_DAC, SR, 7);
			} else if (samplingRate == STDAC_RATE_48_KHZ) {
				reg_write |= SET_BITS(regST_DAC, SR, 8);
			}

			/* Set the optional clock inversion. */
			reg_mask |= SET_BITS(regST_DAC, ST_CLK_INV, 1);
			if (invert & INVERT_BITCLOCK) {
				reg_write |= SET_BITS(regST_DAC, ST_CLK_INV, 1);
			}

			/* Set the optional frame sync inversion. */
			reg_mask |= SET_BITS(regST_DAC, ST_FS_INV, 1);
			if (invert & INVERT_FRAMESYNC) {
				reg_write |= SET_BITS(regST_DAC, ST_FS_INV, 1);
			}

			/* Write the configuration to the ST_DAC register. */
			rc = pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, reg_write,
					    reg_mask);

			if (rc == PMIC_SUCCESS) {
				/* Also reset the digital filter now that we've changed the
				 * clock configuration.
				 */
				pmic_audio_digital_filter_reset(handle);

				stDAC.clockIn = clockIn;
				stDAC.clockFreq = clockFreq;
				stDAC.samplingRate = samplingRate;
				stDAC.invert = invert;
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the Stereo DAC clock source and operating characteristics.
 *
 * Get the current Stereo DAC clock source and operating characteristics.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  clockIn         The clock signal source.
 * @param[out]  clockFreq       The clock signal frequency.
 * @param[out]  samplingRate    The audio data sampling rate.
 * @param[out]  invert          Inversion of the frame sync and/or
 *                              bit clock inputs is enabled/disabled.
 *
 * @retval      PMIC_SUCCESS         If the Stereo DAC clock settings were
 *                                   successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle invalid.
 * @retval      PMIC_ERROR           If the Stereo DAC clock configuration
 *                                   could not be retrieved.
 */
PMIC_STATUS pmic_audio_stdac_get_clock(const PMIC_AUDIO_HANDLE handle,
				       PMIC_AUDIO_CLOCK_IN_SOURCE *
				       const clockIn,
				       PMIC_AUDIO_STDAC_SAMPLING_RATE *
				       const samplingRate,
				       PMIC_AUDIO_STDAC_CLOCK_IN_FREQ *
				       const clockFreq,
				       PMIC_AUDIO_CLOCK_INVERT * const invert)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) &&
	    (stDAC.handleState == HANDLE_IN_USE) &&
	    (clockIn != (PMIC_AUDIO_CLOCK_IN_SOURCE *) NULL) &&
	    (samplingRate != (PMIC_AUDIO_STDAC_SAMPLING_RATE *) NULL) &&
	    (clockFreq != (PMIC_AUDIO_STDAC_CLOCK_IN_FREQ *) NULL) &&
	    (invert != (PMIC_AUDIO_CLOCK_INVERT *) NULL)) {
		*clockIn = stDAC.clockIn;
		*samplingRate = stDAC.samplingRate;
		*clockFreq = stDAC.clockFreq;
		*invert = stDAC.invert;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set the Stereo DAC primary audio channel timeslot.
 *
 * Set the Stereo DAC primary audio channel timeslot. This function must be
 * used if the default timeslot for the primary audio channel is to be changed.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   timeslot        Select the primary audio channel timeslot.
 *
 * @retval      PMIC_SUCCESS         If the Stereo DAC primary audio channel
 *                                   timeslot was successfully configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or audio channel timeslot
 *                                   was invalid.
 * @retval      PMIC_ERROR           If the Stereo DAC primary audio channel
 *                                   timeslot could not be set.
 */
PMIC_STATUS pmic_audio_stdac_set_rxtx_timeslot(const PMIC_AUDIO_HANDLE handle,
					       const PMIC_AUDIO_STDAC_TIMESLOTS
					       timeslot)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		if (timeslot != USE_TS0_TS1) {
			rc = PMIC_NOT_SUPPORTED;
		} else {
			/* This is a no-op for the sc55112 Stereo DAC since it
			 * can only use timeslots 0 and 1.
			 */
			stDAC.timeslot = timeslot;
			rc = PMIC_SUCCESS;
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current Stereo DAC primary audio channel timeslot.
 *
 * Get the current Stereo DAC primary audio channel timeslot.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  timeslot        The primary audio channel timeslot.
 *
 * @retval      PMIC_SUCCESS         If the Stereo DAC primary audio channel
 *                                   timeslot was successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the Stereo DAC primary audio channel
 *                                   timeslot could not be retrieved.
 */
PMIC_STATUS pmic_audio_stdac_get_rxtx_timeslot(const PMIC_AUDIO_HANDLE handle,
					       PMIC_AUDIO_STDAC_TIMESLOTS *
					       const timeslot)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) &&
	    (stDAC.handleState == HANDLE_IN_USE) &&
	    (timeslot != (PMIC_AUDIO_STDAC_TIMESLOTS *) NULL)) {
		*timeslot = stDAC.timeslot;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set/Enable the Stereo DAC options.
 *
 * Set or enable various Stereo DAC options. The available options include
 * resetting the digital filter and enabling the bus master clock outputs.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   config          The Stereo DAC options to enable.
 *
 * @retval      PMIC_SUCCESS         If the Stereo DAC options were
 *                                   successfully configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or Stereo DAC options
 *                                   were invalid.
 * @retval      PMIC_ERROR           If the Stereo DAC options could not be
 *                                   successfully set/enabled.
 */
PMIC_STATUS pmic_audio_stdac_set_config(const PMIC_AUDIO_HANDLE handle,
					const PMIC_AUDIO_STDAC_CONFIG config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		if (config & STDAC_MASTER_CLOCK_OUTPUTS) {
			reg_write |= SET_BITS(regST_DAC, ST_CLK_EN, 1);
			reg_mask |= SET_BITS(regST_DAC, ST_CLK_EN, 1);
		}

		rc = pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, reg_write,
				    reg_mask);

		if (rc == PMIC_SUCCESS) {
			stDAC.config |= config;
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Clear/Disable the Stereo DAC options.
 *
 * Clear or disable various Stereo DAC options.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   config          The Stereo DAC options to be cleared/disabled.
 *
 * @retval      PMIC_SUCCESS         If the Stereo DAC options were
 *                                   successfully cleared/disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or the Stereo DAC options
 *                                   were invalid.
 * @retval      PMIC_ERROR           If the Stereo DAC options could not be
 *                                   cleared/disabled.
 */
PMIC_STATUS pmic_audio_stdac_clear_config(const PMIC_AUDIO_HANDLE handle,
					  const PMIC_AUDIO_STDAC_CONFIG config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		/* Note that clearing the digital filter reset bit is a no-op
		 * since this bit is self clearing.
		 */

		if (config & STDAC_MASTER_CLOCK_OUTPUTS) {
			reg_mask |= SET_BITS(regST_DAC, ST_CLK_EN, 1);
		}

		if (reg_mask != 0) {
			rc = pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, reg_write,
					    reg_mask);

			if (rc == PMIC_SUCCESS) {
				stDAC.config &= ~config;
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current Stereo DAC options.
 *
 * Get the current Stereo DAC options.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  config          The current set of Stereo DAC options.
 *
 * @retval      PMIC_SUCCESS         If the Stereo DAC options were
 *                                   successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the Stereo DAC options could not be
 *                                   retrieved.
 */
PMIC_STATUS pmic_audio_stdac_get_config(const PMIC_AUDIO_HANDLE handle,
					PMIC_AUDIO_STDAC_CONFIG * const config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == stDAC.handle) &&
	    (stDAC.handleState == HANDLE_IN_USE) &&
	    (config != (PMIC_AUDIO_STDAC_CONFIG *) NULL)) {
		*config = stDAC.config;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*@}*/

/*************************************************************************
 * Audio input section configuration.
 *************************************************************************
 */

/*!
 * @name Audio Input Setup and Configuration APIs
 * Functions for general setup and configuration of the PMIC audio
 * input hardware.
 */
/*@{*/

/*!
 * @brief Set/Enable the audio input section options.
 *
 * Set or enable various audio input section options. The only available
 * option right now is to enable the automatic disabling of the microphone
 * input amplifiers when a microphone/headset is inserted or removed.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   config          The audio input section options to enable.
 *
 * @retval      PMIC_SUCCESS         If the audio input section options were
 *                                   successfully configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or audio input section
 *                                   options were invalid.
 * @retval      PMIC_ERROR           If the audio input section options could
 *                                   not be successfully set/enabled.
 */
PMIC_STATUS pmic_audio_input_set_config(const PMIC_AUDIO_HANDLE handle,
					const PMIC_AUDIO_INPUT_CONFIG config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		if (config == MIC_AMP_AUTO_DISABLE) {
			reg_write = SET_BITS(regRX_AUD_AMPS, A1ID_RX, 1);
			reg_mask = SET_BITS(regRX_AUD_AMPS, A1ID_RX, 1);

			rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
					    reg_write, reg_mask);

			if (rc == PMIC_SUCCESS) {
				vCodec.inputConfig |= config;
			}
		} else {
			rc = PMIC_NOT_SUPPORTED;
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Clear/Disable the audio input section options.
 *
 * Clear or disable various audio input section options.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   config          The audio input section options to be
 *                              cleared/disabled.
 *
 * @retval      PMIC_SUCCESS         If the audio input section options were
 *                                   successfully cleared/disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or the audio input section
 *                                   options were invalid.
 * @retval      PMIC_ERROR           If the audio input section options could
 *                                   not be cleared/disabled.
 */
PMIC_STATUS pmic_audio_input_clear_config(const PMIC_AUDIO_HANDLE handle,
					  const PMIC_AUDIO_INPUT_CONFIG config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		if (config == MIC_AMP_AUTO_DISABLE) {
			reg_mask = SET_BITS(regRX_AUD_AMPS, A1ID_RX, 1);

			rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
					    reg_write, reg_mask);

			if (rc == PMIC_SUCCESS) {
				vCodec.inputConfig &= ~config;
			}
		} else {
			rc = PMIC_NOT_SUPPORTED;
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current audio input section options.
 *
 * Get the current audio input section options.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  config          The current set of audio input section options.
 *
 * @retval      PMIC_SUCCESS         If the audio input section options were
 *                                   successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the audio input section options could
 *                                   not be retrieved.
 */
PMIC_STATUS pmic_audio_input_get_config(const PMIC_AUDIO_HANDLE handle,
					PMIC_AUDIO_INPUT_CONFIG * const config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    (config != (PMIC_AUDIO_INPUT_CONFIG *) NULL)) {
		*config = vCodec.inputConfig;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*@}*/

/*************************************************************************
 * Audio recording using the Voice CODEC.
 *************************************************************************
 */

/*!
 * @name Audio Recording Using the Voice CODEC Setup and Configuration APIs
 * Functions for general setup and configuration of the PMIC Voice CODEC
 * to perform audio recording.
 */
/*@{*/

/*!
 * @brief Select the microphone inputs to be used for Voice CODEC recording.
 *
 * Select left (mc13783-only) and right microphone inputs for Voice CODEC
 * recording. It is possible to disable or not use a particular microphone
 * input channel by specifying NO_MIC as a parameter.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   leftChannel     Select the left microphone input channel.
 * @param[in]   rightChannel    Select the right microphone input channel.
 *
 * @retval      PMIC_SUCCESS         If the microphone input channels were
 *                                   successfully enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or microphone input ports
 *                                   were invalid.
 * @retval      PMIC_ERROR           If the microphone input channels could
 *                                   not be successfully enabled.
 */
PMIC_STATUS pmic_audio_vcodec_set_mic(const PMIC_AUDIO_HANDLE handle,
				      const PMIC_AUDIO_INPUT_PORT leftChannel,
				      const PMIC_AUDIO_INPUT_PORT rightChannel)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		if ((leftChannel != NO_MIC) || (rightChannel == MIC1_LEFT)) {
			/* The sc55112 Voice CODEC only supports mono recording
			 * using the right microphone channel.
			 */
			rc = PMIC_NOT_SUPPORTED;
		} else {
			if (rightChannel == MIC1_RIGHT_MIC_MONO) {
				reg_write = SET_BITS(regTX_AUD_AMPS, A3_EN, 1) |
				    SET_BITS(regTX_AUD_AMPS, A3_MUX, 1);
				reg_mask = SET_BITS(regTX_AUD_AMPS, A3_EN, 1) |
				    SET_BITS(regTX_AUD_AMPS, A3_MUX, 1) |
				    SET_BITS(regTX_AUD_AMPS, A5_EN, 1) |
				    SET_BITS(regTX_AUD_AMPS, A5_MUX, 1) |
				    SET_BITS(regTX_AUD_AMPS, EXT_MIC_MUX, 1);
			} else if (rightChannel == MIC2_AUX) {
				pmic_audio_output_enable_phantom_ground(handle);

				reg_write = SET_BITS(regTX_AUD_AMPS, A5_EN, 1) |
				    SET_BITS(regTX_AUD_AMPS, A5_MUX, 1);
				reg_mask = SET_BITS(regTX_AUD_AMPS, A3_EN, 1) |
				    SET_BITS(regTX_AUD_AMPS, A3_MUX, 1) |
				    SET_BITS(regTX_AUD_AMPS, A5_EN, 1) |
				    SET_BITS(regTX_AUD_AMPS, A5_MUX, 1) |
				    SET_BITS(regTX_AUD_AMPS, EXT_MIC_MUX, 1);
			} else if (rightChannel == TXIN_EXT) {
				reg_write =
				    SET_BITS(regTX_AUD_AMPS, EXT_MIC_MUX, 1);
				reg_mask =
				    SET_BITS(regTX_AUD_AMPS, A3_EN,
					     1) | SET_BITS(regTX_AUD_AMPS,
							   A3_MUX,
							   1) |
				    SET_BITS(regTX_AUD_AMPS, A5_EN,
					     1) | SET_BITS(regTX_AUD_AMPS,
							   A5_MUX,
							   1) |
				    SET_BITS(regTX_AUD_AMPS, EXT_MIC_MUX, 1);
			}

			if (reg_mask != 0) {
				rc = pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS,
						    reg_write, reg_mask);

				if (rc == PMIC_SUCCESS) {
					vCodec.leftChannelMic.mic = leftChannel;
					vCodec.rightChannelMic.mic =
					    rightChannel;
				}
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current microphone inputs being used for Voice CODEC
 *        recording.
 *
 * Get the left (mc13783-only) and right microphone inputs currently being
 * used for Voice CODEC recording.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  leftChannel     The left microphone input channel.
 * @param[out]  rightChannel    The right microphone input channel.
 *
 * @retval      PMIC_SUCCESS         If the microphone input channels were
 *                                   successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the microphone input channels could
 *                                   not be retrieved.
 */
PMIC_STATUS pmic_audio_vcodec_get_mic(const PMIC_AUDIO_HANDLE handle,
				      PMIC_AUDIO_INPUT_PORT * const leftChannel,
				      PMIC_AUDIO_INPUT_PORT *
				      const rightChannel)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    (leftChannel != (PMIC_AUDIO_INPUT_PORT *) NULL) &&
	    (rightChannel != (PMIC_AUDIO_INPUT_PORT *) NULL)) {
		*leftChannel = vCodec.leftChannelMic.mic;
		*rightChannel = vCodec.rightChannelMic.mic;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Enable/disable the microphone input.
 *
 * This function enables/disables the current microphone input channel. The
 * input amplifier is automatically turned off when the microphone input is
 * disabled.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[in]   leftChannel     The left microphone input channel state.
 * @param[in]   rightChannel    the right microphone input channel state.
 *
 * @retval      PMIC_SUCCESS         If the microphone input channels were
 *                                   successfully reconfigured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or microphone input states
 *                                   were invalid.
 * @retval      PMIC_ERROR           If the microphone input channels could
 *                                   not be reconfigured.
 */
PMIC_STATUS pmic_audio_vcodec_set_mic_on_off(const PMIC_AUDIO_HANDLE handle,
					     const PMIC_AUDIO_INPUT_MIC_STATE
					     leftChannel,
					     const PMIC_AUDIO_INPUT_MIC_STATE
					     rightChannel)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		if (leftChannel != MICROPHONE_OFF) {
			/* The sc55112 Voice CODEC only supports mono recording
			 * using the right microphone channel.
			 */
			rc = PMIC_NOT_SUPPORTED;
		} else {
			if (rightChannel == MICROPHONE_ON) {
				if (vCodec.rightChannelMic.mic ==
				    MIC1_RIGHT_MIC_MONO) {
					reg_write =
					    SET_BITS(regTX_AUD_AMPS, A3_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A3_MUX, 1);
					reg_mask =
					    SET_BITS(regTX_AUD_AMPS, A3_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A3_MUX,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A5_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A5_MUX,
						     1) |
					    SET_BITS(regTX_AUD_AMPS,
						     EXT_MIC_MUX, 1);
				} else if (vCodec.rightChannelMic.mic ==
					   MIC2_AUX) {
					pmic_audio_output_enable_phantom_ground
					    (handle);
					pmic_audio_mic_boost_enable();

					reg_write =
					    SET_BITS(regTX_AUD_AMPS, A5_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A5_MUX, 1);
					reg_mask =
					    SET_BITS(regTX_AUD_AMPS, A3_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A3_MUX,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A5_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A5_MUX,
						     1) |
					    SET_BITS(regTX_AUD_AMPS,
						     EXT_MIC_MUX, 1);
				} else if (vCodec.rightChannelMic.mic ==
					   TXIN_EXT) {
					reg_write =
					    SET_BITS(regTX_AUD_AMPS,
						     EXT_MIC_MUX, 1);
					reg_mask =
					    SET_BITS(regTX_AUD_AMPS, A3_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A3_MUX,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A5_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A5_MUX,
						     1) |
					    SET_BITS(regTX_AUD_AMPS,
						     EXT_MIC_MUX, 1);
				}
			} else if (rightChannel == MICROPHONE_OFF) {
				if (vCodec.rightChannelMic.mic ==
				    MIC1_RIGHT_MIC_MONO) {
					reg_mask =
					    SET_BITS(regTX_AUD_AMPS, A3_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A3_MUX, 1);
				} else if (vCodec.rightChannelMic.mic ==
					   MIC2_AUX) {
					pmic_audio_output_disable_phantom_ground
					    (handle);
					pmic_audio_mic_boost_disable();

					reg_mask =
					    SET_BITS(regTX_AUD_AMPS, A5_EN,
						     1) |
					    SET_BITS(regTX_AUD_AMPS, A5_MUX,
						     1) |
					    SET_BITS(regTX_AUD_AMPS,
						     EXT_MIC_MUX, 1);
				} else if (vCodec.rightChannelMic.mic ==
					   TXIN_EXT) {
					reg_mask =
					    SET_BITS(regTX_AUD_AMPS,
						     EXT_MIC_MUX, 1);
				}
			}

			if (reg_mask != 0) {
				rc = pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS,
						    reg_write, reg_mask);

				if (rc == PMIC_SUCCESS) {
					vCodec.leftChannelMic.micOnOff =
					    leftChannel;
					vCodec.rightChannelMic.micOnOff =
					    rightChannel;
				}
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Return the current state of the microphone inputs.
 *
 * This function returns the current state (on/off) of the microphone
 * input channels.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 * @param[out]  leftChannel     The current left microphone input channel
 *                              state.
 * @param[out]  rightChannel    the current right microphone input channel
 *                              state.
 *
 * @retval      PMIC_SUCCESS         If the microphone input channel states
 *                                   were successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the microphone input channel states
 *                                   could not be retrieved.
 */
PMIC_STATUS pmic_audio_vcodec_get_mic_on_off(const PMIC_AUDIO_HANDLE handle,
					     PMIC_AUDIO_INPUT_MIC_STATE *
					     const leftChannel,
					     PMIC_AUDIO_INPUT_MIC_STATE *
					     const rightChannel)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    (leftChannel != (PMIC_AUDIO_INPUT_MIC_STATE *) NULL) &&
	    (rightChannel != (PMIC_AUDIO_INPUT_MIC_STATE *) NULL)) {
		*leftChannel = vCodec.leftChannelMic.micOnOff;
		*rightChannel = vCodec.rightChannelMic.micOnOff;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set the microphone input amplifier mode and gain level.
 *
 * This function sets the current microphone input amplifier operating mode
 * and gain level.
 *
 * @param[in]   handle           Device handle from pmic_audio_open() call.
 * @param[in]   leftChannelMode  The left microphone input amplifier mode.
 * @param[in]   leftChannelGain  The left microphone input amplifier gain level.
 * @param[in]   rightChannelMode The right microphone input amplifier mode.
 * @param[in]   rightChannelGain The right microphone input amplifier gain
 *                               level.
 *
 * @retval      PMIC_SUCCESS         If the microphone input amplifiers were
 *                                   successfully reconfigured.
 * @retval      PMIC_PARAMETER_ERROR If the handle or microphone input amplifier
 *                                   modes or gain levels were invalid.
 * @retval      PMIC_ERROR           If the microphone input amplifiers could
 *                                   not be reconfigured.
 */
PMIC_STATUS pmic_audio_vcodec_set_record_gain(const PMIC_AUDIO_HANDLE handle,
					      const PMIC_AUDIO_MIC_AMP_MODE
					      leftChannelMode,
					      const PMIC_AUDIO_MIC_GAIN
					      leftChannelGain,
					      const PMIC_AUDIO_MIC_AMP_MODE
					      rightChannelMode,
					      const PMIC_AUDIO_MIC_GAIN
					      rightChannelGain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		if ((leftChannelMode != AMP_OFF) ||
		    (rightChannelMode == AMP_OFF) ||
		    (rightChannelMode != CURRENT_TO_VOLTAGE) ||
		    (rightChannelGain < MIC_GAIN_0DB) ||
		    (rightChannelGain > MIC_GAIN_PLUS_31DB)) {
			rc = PMIC_NOT_SUPPORTED;
		} else {
			/* Note that the right channel amplifier mode of CURRENT_TO_VOLTAGE
			 * is a no-op since this is the only mode supported by the
			 * sc55112 amplifier.
			 */

			reg_write = SET_BITS(regTX_AUD_AMPS, AUDIG,
					     rightChannelGain - MIC_GAIN_0DB);
			reg_mask = regTX_AUD_AMPS.AUDIG.mask;

			rc = pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS,
					    reg_write, reg_mask);

			if (rc == PMIC_SUCCESS) {
				vCodec.leftChannelMic.ampMode = leftChannelMode;
				vCodec.leftChannelMic.gain = leftChannelGain;
				vCodec.rightChannelMic.ampMode =
				    rightChannelMode;
				vCodec.rightChannelMic.gain = rightChannelGain;
			}
		}
	}

	/* Exit critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current microphone input amplifier mode and gain level.
 *
 * This function gets the current microphone input amplifier operating mode
 * and gain level.
 *
 * @param[in]   handle           Device handle from pmic_audio_open() call.
 * @param[out]  leftChannelMode  The left microphone input amplifier mode.
 * @param[out]  leftChannelGain  The left microphone input amplifier gain level.
 * @param[out]  rightChannelMode The right microphone input amplifier mode.
 * @param[out]  rightChannelGain The right microphone input amplifier gain
 *                               level.
 *
 * @retval      PMIC_SUCCESS         If the microphone input amplifier modes
 *                                   and gain levels were successfully
 *                                   retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the microphone input amplifier modes
 *                                   and gain levels could not be retrieved.
 */
PMIC_STATUS pmic_audio_vcodec_get_record_gain(const PMIC_AUDIO_HANDLE handle,
					      PMIC_AUDIO_MIC_AMP_MODE *
					      const leftChannelMode,
					      PMIC_AUDIO_MIC_GAIN *
					      const leftChannelGain,
					      PMIC_AUDIO_MIC_AMP_MODE *
					      const rightChannelMode,
					      PMIC_AUDIO_MIC_GAIN *
					      const rightChannelGain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == vCodec.handle) &&
	    (vCodec.handleState == HANDLE_IN_USE) &&
	    (leftChannelMode != (PMIC_AUDIO_MIC_AMP_MODE *) NULL) &&
	    (leftChannelGain != (PMIC_AUDIO_MIC_GAIN *) NULL) &&
	    (rightChannelMode != (PMIC_AUDIO_MIC_AMP_MODE *) NULL) &&
	    (rightChannelGain != (PMIC_AUDIO_MIC_GAIN *) NULL)) {
		*leftChannelMode = vCodec.leftChannelMic.ampMode;
		*leftChannelGain = vCodec.leftChannelMic.gain;
		*rightChannelMode = vCodec.rightChannelMic.ampMode;
		*rightChannelGain = vCodec.rightChannelMic.gain;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Enable a microphone bias circuit.
 *
 * This function enables one of the available microphone bias circuits.
 *
 * @param[in]   handle           Device handle from pmic_audio_open() call.
 * @param[in]   biasCircuit      The microphone bias circuit to be enabled.
 *
 * @retval      PMIC_SUCCESS         If the microphone bias circuit was
 *                                   successfully enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or selected microphone bias
 *                                   circuit was invalid.
 * @retval      PMIC_ERROR           If the microphone bias circuit could not
 *                                   be enabled.
 */
PMIC_STATUS pmic_audio_vcodec_enable_micbias(const PMIC_AUDIO_HANDLE handle,
					     const PMIC_AUDIO_MIC_BIAS
					     biasCircuit)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		if (biasCircuit & MIC_BIAS1) {
			reg_write = SET_BITS(regTX_AUD_AMPS, MB_ON1, 1);
			reg_mask = SET_BITS(regTX_AUD_AMPS, MB_ON1, 1);
		}

		if (biasCircuit & MIC_BIAS2) {
			reg_write |= SET_BITS(regTX_AUD_AMPS, MB_ON2, 1);
			reg_mask |= SET_BITS(regTX_AUD_AMPS, MB_ON2, 1);
		}

		if (reg_mask != 0) {
			rc = pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS,
					    reg_write, reg_mask);
		}
	}

	return rc;
}

/*!
 * @brief Disable a microphone bias circuit.
 *
 * This function disables one of the available microphone bias circuits.
 *
 * @param[in]   handle           Device handle from pmic_audio_open() call.
 * @param[in]   biasCircuit      The microphone bias circuit to be disabled.
 *
 * @retval      PMIC_SUCCESS         If the microphone bias circuit was
 *                                   successfully disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or selected microphone bias
 *                                   circuit was invalid.
 * @retval      PMIC_ERROR           If the microphone bias circuit could not
 *                                   be disabled.
 */
PMIC_STATUS pmic_audio_vcodec_disable_micbias(const PMIC_AUDIO_HANDLE handle,
					      const PMIC_AUDIO_MIC_BIAS
					      biasCircuit)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		if (biasCircuit & MIC_BIAS1) {
			reg_mask = SET_BITS(regTX_AUD_AMPS, MB_ON1, 1);
		}

		if (biasCircuit & MIC_BIAS2) {
			reg_mask |= SET_BITS(regTX_AUD_AMPS, MB_ON2, 1);
		}

		if (reg_mask != 0) {
			rc = pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS,
					    reg_write, reg_mask);
		}
	}

	return rc;
}

/*@}*/

/*************************************************************************
 * Audio Playback Using the Voice CODEC.
 *************************************************************************
 */

/*!
 * @name Audio Playback Using the Voice CODEC Setup and Configuration APIs
 * Functions for general setup and configuration of the PMIC Voice CODEC
 * to perform audio playback.
 */
/*@{*/

/*!
 * @brief Configure and enable the Voice CODEC mixer.
 *
 * This function configures and enables the Voice CODEC mixer.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   rxSecondaryTimeslot The timeslot used for the secondary audio
 *                                  channel.
 * @param[in]   gainIn              The secondary audio channel gain level.
 * @param[in]   gainOut             The mixer output gain level.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC mixer was successfully
 *                                   configured and enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or mixer configuration
 *                                   was invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC mixer could not be
 *                                   reconfigured or enabled.
 */
PMIC_STATUS pmic_audio_vcodec_enable_mixer(const PMIC_AUDIO_HANDLE handle,
					   const PMIC_AUDIO_VCODEC_TIMESLOT
					   rxSecondaryTimeslot,
					   const PMIC_AUDIO_VCODEC_MIX_IN_GAIN
					   gainIn,
					   const PMIC_AUDIO_VCODEC_MIX_OUT_GAIN
					   gainOut)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		/* The sc55112 Voice CODEC does not support mixing. */
		rc = PMIC_NOT_SUPPORTED;
	}

	return rc;
}

/*!
 * @brief Disable the Voice CODEC mixer.
 *
 * This function disables the Voice CODEC mixer.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the Voice CODEC mixer was successfully
 *                                   disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the Voice CODEC mixer could not be
 *                                   disabled.
 */
PMIC_STATUS pmic_audio_vcodec_disable_mixer(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	if ((handle == vCodec.handle) && (vCodec.handleState == HANDLE_IN_USE)) {
		/* The sc55112 Voice CODEC does not support mixing. */
		rc = PMIC_NOT_SUPPORTED;
	}

	return rc;
}

/*@}*/

/*************************************************************************
 * Audio Playback Using the Stereo DAC.
 *************************************************************************
 */

/*!
 * @name Audio Playback Using the Stereo DAC Setup and Configuration APIs
 * Functions for general setup and configuration of the PMIC Stereo DAC
 * to perform audio playback.
 */
/*@{*/

/*!
 * @brief Configure and enable the Stereo DAC mixer.
 *
 * This function configures and enables the Stereo DAC mixer.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   rxSecondaryTimeslot The timeslot used for the secondary audio
 *                                  channel.
 * @param[in]   gainIn              The secondary audio channel gain level.
 * @param[in]   gainOut             The mixer output gain level.
 *
 * @retval      PMIC_SUCCESS         If the Stereo DAC mixer was successfully
 *                                   configured and enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or mixer configuration
 *                                   was invalid.
 * @retval      PMIC_ERROR           If the Stereo DAC mixer could not be
 *                                   reconfigured or enabled.
 */
PMIC_STATUS pmic_audio_stdac_enable_mixer(const PMIC_AUDIO_HANDLE handle,
					  const PMIC_AUDIO_STDAC_TIMESLOTS
					  rxSecondaryTimeslot,
					  const PMIC_AUDIO_STDAC_MIX_IN_GAIN
					  gainIn,
					  const PMIC_AUDIO_STDAC_MIX_OUT_GAIN
					  gainOut)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		if (gainIn == STDAC_NO_MIX) {
			/* Flag as an error if we call this function but request that
			 * mixing is disabled.
			 */
			rc = PMIC_ERROR;
		} else if (!((gainIn == STDAC_MIX_IN_0DB) ||
			     (gainIn == STDAC_MIX_IN_MINUS_6DB)) ||
			   (gainOut != STDAC_MIX_OUT_0DB) ||
			   (rxSecondaryTimeslot != USE_TS2_TS3)) {
			/* sc55112 Stereo DAC only supports an input gain of either
			 * 1 (0dB) or 0.5 (-6dB) and an output gain of 1 (0dB).
			 *
			 * The Stereo DAC also only supports summing timeslots (t0)+(t2)
			 * and (t1)+(t3).
			 */
			rc = PMIC_NOT_SUPPORTED;
		} else {
			/* Enable the Stereo DAC mixer and also set the input gain
			 * to either 0dB or -6dB. Note that the output gain is fixed
			 * at 0dB so nothing needs to be done.
			 *
			 * And nothing needs to be done to select using timeslots t2
			 * and t3 since that is the only option supported by the
			 * sc55112 Stereo DAC mixer.
			 */
			reg_write = SET_BITS(regST_DAC, AUD_MIX_EN, 1);

			if (gainIn == STDAC_MIX_IN_0DB) {
				reg_write |=
				    SET_BITS(regST_DAC, AUD_MIX_LVL, 1);
			}

			reg_mask = SET_BITS(regST_DAC, AUD_MIX_EN, 1) |
			    SET_BITS(regST_DAC, AUD_MIX_LVL, 1);

			rc = pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, reg_write,
					    reg_mask);
		}
	}

	return rc;
}

/*!
 * @brief Disable the Stereo DAC mixer.
 *
 * This function disables the Stereo DAC mixer.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the Stereo DAC mixer was successfully
 *                                   disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the Stereo DAC mixer could not be
 *                                   disabled.
 */
PMIC_STATUS pmic_audio_stdac_disable_mixer(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int reg_write = 0;
	const unsigned int reg_mask = SET_BITS(regST_DAC, AUD_MIX_EN, 1);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		rc = pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, reg_write,
				    reg_mask);
	}

	return rc;
}

/*@}*/

/*************************************************************************
 * Audio Output Control
 *************************************************************************
 */

/*!
 * @name Audio Output Section Setup and Configuration APIs
 * Functions for general setup and configuration of the PMIC audio output
 * section to support playback.
 */
/*@{*/

/*!
 * @brief Select the audio output ports.
 *
 * This function selects the audio output ports to be used. This also enables
 * the appropriate output amplifiers.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   port                The audio output ports to be used.
 *
 * @retval      PMIC_SUCCESS         If the audio output ports were successfully
 *                                   acquired.
 * @retval      PMIC_PARAMETER_ERROR If the handle or output ports were
 *                                   invalid.
 * @retval      PMIC_ERROR           If the audio output ports could not be
 *                                   acquired.
 */
PMIC_STATUS pmic_audio_output_set_port(const PMIC_AUDIO_HANDLE handle,
				       const PMIC_AUDIO_OUTPUT_PORT port)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		if ((port & MONO_LOUDSPEAKER) || (port & MONO_CDCOUT) ||
		    (port & STEREO_LEFT_LOW_POWER) || (port & STEREO_OUT_LEFT)
		    || (port & STEREO_OUT_RIGHT)) {
			rc = PMIC_NOT_SUPPORTED;
		} else {
			if (handle == stDAC.handle) {
				reg_write =
				    SET_BITS(regRX_AUD_AMPS, ST_DAC_SW, 1);
				reg_mask =
				    SET_BITS(regRX_AUD_AMPS, ST_DAC_SW, 1);
			} else if (handle == vCodec.handle) {
				reg_write = SET_BITS(regRX_AUD_AMPS, CDC_SW, 1);
				reg_mask = SET_BITS(regRX_AUD_AMPS, CDC_SW, 1);
			} else if (handle == extStereoIn.handle) {
				reg_write =
				    SET_BITS(regRX_AUD_AMPS, PGA_IN_SW, 1);
				reg_mask =
				    SET_BITS(regRX_AUD_AMPS, PGA_IN_SW, 1);
			}

			if (port & MONO_SPEAKER) {
				reg_write |= SET_BITS(regRX_AUD_AMPS, A1_EN, 1);
				reg_mask |= SET_BITS(regRX_AUD_AMPS, A1_EN, 1);
			}
			if (port & MONO_ALERT) {
				reg_write |= SET_BITS(regRX_AUD_AMPS, A2_EN, 1);
				reg_mask |= SET_BITS(regRX_AUD_AMPS, A2_EN, 1);
			}
			if (port & MONO_EXTOUT) {
				reg_write |= SET_BITS(regRX_AUD_AMPS, A4_EN, 1);
				reg_mask |= SET_BITS(regRX_AUD_AMPS, A4_EN, 1);
			}
			if (port & STEREO_HEADSET_LEFT) {
				reg_write |=
				    SET_BITS(regRX_AUD_AMPS, ALEFT_EN, 1);
				reg_mask |=
				    SET_BITS(regRX_AUD_AMPS, ALEFT_EN, 1);
			}
			if (port & STEREO_HEADSET_RIGHT) {
				reg_write |=
				    SET_BITS(regRX_AUD_AMPS, ARIGHT_EN, 1);
				reg_mask |=
				    SET_BITS(regRX_AUD_AMPS, ARIGHT_EN, 1);
			}

			rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
					    reg_write, reg_mask);

			if (rc == PMIC_SUCCESS) {
				/* Special handling for the phantom ground circuit is
				 * recommended if either ALEFT/ARIGHT amplifiers are enabled.
				 */
				if (port &
				    (STEREO_HEADSET_LEFT |
				     STEREO_HEADSET_RIGHT)) {
					/* We should also enable the phantom ground circuit when
					 * using either ALEFT/ARIGHT to improve the quality of the
					 * audio output.
					 */
					pmic_audio_output_enable_phantom_ground
					    (handle);
				} else {
					/* Otherwise disable the phantom ground circuit when not
					 * using either ALEFT/ARIGHT to reduce power consumption.
					 */
					pmic_audio_output_disable_phantom_ground
					    (handle);
				}

				audioOutput.outputPort = port;
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Deselect/disable the audio output ports.
 *
 * This function disables the audio output ports that were previously enabled
 * by calling pmic_audio_output_set_port().
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   port                The audio output ports to be disabled.
 *
 * @retval      PMIC_SUCCESS         If the audio output ports were successfully
 *                                   disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or output ports were
 *                                   invalid.
 * @retval      PMIC_ERROR           If the audio output ports could not be
 *                                   disabled.
 */
PMIC_STATUS pmic_audio_output_clear_port(const PMIC_AUDIO_HANDLE handle,
					 const PMIC_AUDIO_OUTPUT_PORT port)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		if ((port & MONO_LOUDSPEAKER) || (port & MONO_CDCOUT) ||
		    (port & STEREO_LEFT_LOW_POWER) || (port & STEREO_OUT_LEFT)
		    || (port & STEREO_OUT_RIGHT)) {
			rc = PMIC_NOT_SUPPORTED;
		} else {
			if (handle == stDAC.handle) {
				reg_mask =
				    SET_BITS(regRX_AUD_AMPS, ST_DAC_SW, 1);
			} else if (handle == vCodec.handle) {
				reg_mask = SET_BITS(regRX_AUD_AMPS, CDC_SW, 1);
			} else if (handle == extStereoIn.handle) {
				reg_mask =
				    SET_BITS(regRX_AUD_AMPS, PGA_IN_SW, 1);
			}

			if (port & MONO_SPEAKER) {
				reg_mask |= SET_BITS(regRX_AUD_AMPS, A1_EN, 1);
			}
			if (port & MONO_ALERT) {
				reg_mask |= SET_BITS(regRX_AUD_AMPS, A2_EN, 1);
			}
			if (port & MONO_EXTOUT) {
				reg_mask |= SET_BITS(regRX_AUD_AMPS, A4_EN, 1);
			}
			if (port & STEREO_HEADSET_LEFT) {
				reg_mask |=
				    SET_BITS(regRX_AUD_AMPS, ALEFT_EN, 1);
			}
			if (port & STEREO_HEADSET_RIGHT) {
				reg_mask |=
				    SET_BITS(regRX_AUD_AMPS, ARIGHT_EN, 1);
			}

			rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
					    reg_write, reg_mask);

			if (rc == PMIC_SUCCESS) {
				if ((port & STEREO_HEADSET_LEFT) &&
				    (port & STEREO_HEADSET_RIGHT)) {
					/* Disable the phantom ground since we've now disabled
					 * both of the stereo headset outputs.
					 */
					pmic_audio_output_disable_phantom_ground
					    (handle);
				}

				audioOutput.outputPort &= ~port;
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current audio output ports.
 *
 * This function retrieves the audio output ports that are currently being
 * used.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[out]  port                The audio output ports currently being used.
 *
 * @retval      PMIC_SUCCESS         If the audio output ports were successfully
 *                                   retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the audio output ports could not be
 *                                   retrieved.
 */
PMIC_STATUS pmic_audio_output_get_port(const PMIC_AUDIO_HANDLE handle,
				       PMIC_AUDIO_OUTPUT_PORT * const port)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((((handle == stDAC.handle) &&
	      (stDAC.handleState == HANDLE_IN_USE)) ||
	     ((handle == vCodec.handle) &&
	      (vCodec.handleState == HANDLE_IN_USE)) ||
	     ((handle == extStereoIn.handle) &&
	      (extStereoIn.handleState == HANDLE_IN_USE))) &&
	    (port != (PMIC_AUDIO_OUTPUT_PORT *) NULL)) {
		*port = audioOutput.outputPort;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set the gain level for the external stereo inputs.
 *
 * This function sets the gain levels for the external stereo inputs.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   gain                The external stereo input gain level.
 *
 * @retval      PMIC_SUCCESS         If the gain level was successfully set.
 * @retval      PMIC_PARAMETER_ERROR If the handle or gain level was invalid.
 * @retval      PMIC_ERROR           If the gain level could not be set.
 */
PMIC_STATUS pmic_audio_output_set_stereo_in_gain(const PMIC_AUDIO_HANDLE handle,
						 const PMIC_AUDIO_STEREO_IN_GAIN
						 gain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if ((handle == extStereoIn.handle) &&
	    (extStereoIn.handleState == HANDLE_IN_USE)) {
		/* Note that the sc55112 does not support a separate preamplifier
		 * for the external stereo inputs. Therefore, selecting a 0DB gain
		 * is considered a successful no-op while anything else will return
		 * PMIC_NOT_SUPPORTED.
		 */
		if (gain == STEREO_IN_GAIN_0DB) {
			rc = PMIC_SUCCESS;
		} else {
			rc = PMIC_NOT_SUPPORTED;
		}
	}

	return rc;
}

/*!
 * @brief Get the current gain level for the external stereo inputs.
 *
 * This function retrieves the current gain levels for the external stereo
 * inputs.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[out]  gain                The current external stereo input gain
 *                                  level.
 *
 * @retval      PMIC_SUCCESS         If the gain level was successfully
 *                                   retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the gain level could not be retrieved.
 */
PMIC_STATUS pmic_audio_output_get_stereo_in_gain(const PMIC_AUDIO_HANDLE handle,
						 PMIC_AUDIO_STEREO_IN_GAIN *
						 const gain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((handle == extStereoIn.handle) &&
	    (extStereoIn.handleState == HANDLE_IN_USE) &&
	    (gain != (PMIC_AUDIO_STEREO_IN_GAIN *) NULL)) {
		*gain = extStereoIn.inputGain;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set the output PGA gain level.
 *
 * This function sets the audio output PGA gain level.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   gain                The output PGA gain level.
 *
 * @retval      PMIC_SUCCESS         If the gain level was successfully set.
 * @retval      PMIC_PARAMETER_ERROR If the handle or gain level was invalid.
 * @retval      PMIC_ERROR           If the gain level could not be set.
 */
PMIC_STATUS pmic_audio_output_set_pgaGain(const PMIC_AUDIO_HANDLE handle,
					  const PMIC_AUDIO_OUTPUT_PGA_GAIN gain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	unsigned int reg_mask = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		if ((gain < OUTPGA_GAIN_MINUS_24DB) ||
		    (gain > OUTPGA_GAIN_PLUS_21DB)) {
			rc = PMIC_NOT_SUPPORTED;
		} else {
			reg_write = SET_BITS(regRX_AUD_AMPS, AUDOG,
					     gain - OUTPGA_GAIN_MINUS_24DB);
			reg_mask = SET_BITS(regRX_AUD_AMPS, AUDOG, 15);

			rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
					    reg_write, reg_mask);

			if (rc == PMIC_SUCCESS) {
				audioOutput.outputPGAGain = gain;
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the output PGA gain level.
 *
 * This function retrieves the current audio output PGA gain level.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[out]  gain                The current output PGA gain level.
 *
 * @retval      PMIC_SUCCESS         If the gain level was successfully
 *                                   retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the gain level could not be retrieved.
 */
PMIC_STATUS pmic_audio_output_get_pgaGain(const PMIC_AUDIO_HANDLE handle,
					  PMIC_AUDIO_OUTPUT_PGA_GAIN *
					  const gain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((((handle == stDAC.handle) &&
	      (stDAC.handleState == HANDLE_IN_USE)) ||
	     ((handle == vCodec.handle) &&
	      (vCodec.handleState == HANDLE_IN_USE)) ||
	     ((handle == extStereoIn.handle) &&
	      (extStereoIn.handleState == HANDLE_IN_USE))) &&
	    (gain != (PMIC_AUDIO_OUTPUT_PGA_GAIN *) NULL)) {
		*gain = audioOutput.outputPGAGain;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Enable the output mixer.
 *
 * This function enables the output mixer for the audio stream that
 * corresponds to the current handle (i.e., the Voice CODEC, Stereo DAC, or
 * the external stereo inputs).
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the mixer was successfully enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the mixer could not be enabled.
 */
PMIC_STATUS pmic_audio_output_enable_mixer(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		/* The PGA amplifiers in the sc55112 audio output section are
		 * always configured as inverting summing amplifiers so nothing
		 * needs to be done to enable this input mixing capability.
		 */
		rc = PMIC_SUCCESS;
	}

	return rc;
}

/*!
 * @brief Disable the output mixer.
 *
 * This function disables the output mixer for the audio stream that
 * corresponds to the current handle (i.e., the Voice CODEC, Stereo DAC, or
 * the external stereo inputs).
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the mixer was successfully disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the mixer could not be disabled.
 */
PMIC_STATUS pmic_audio_output_disable_mixer(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		/* The PGA amplifiers in the sc55112 audio output section are
		 * always configured as inverting summing amplifiers. Therefore,
		 * we cannot disable the mixing capability. Instead, we need to
		 * deselect the particular audio input signals by calling
		 * pmic_audio_output_set_port().
		 */
		rc = PMIC_NOT_SUPPORTED;
	}

	return rc;
}

/*!
 * @brief Configure and enable the output balance amplifiers.
 *
 * This function configures and enables the output balance amplifiers.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   leftGain            The desired left channel gain level.
 * @param[in]   rightGain           The desired right channel gain level.
 *
 * @retval      PMIC_SUCCESS         If the output balance amplifiers were
 *                                   successfully configured and enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or gain levels were invalid.
 * @retval      PMIC_ERROR           If the output balance amplifiers could not
 *                                   be reconfigured or enabled.
 */
PMIC_STATUS pmic_audio_output_set_balance(const PMIC_AUDIO_HANDLE handle,
					  const PMIC_AUDIO_OUTPUT_BALANCE_GAIN
					  leftGain,
					  const PMIC_AUDIO_OUTPUT_BALANCE_GAIN
					  rightGain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		/* The sc55112 audio output section does not support balance
		 * control between the left and right channels.
		 */
		rc = PMIC_NOT_SUPPORTED;
	}

	return rc;
}

/*!
 * @brief Get the current output balance amplifier gain levels.
 *
 * This function retrieves the current output balance amplifier gain levels.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[out]  leftGain            The current left channel gain level.
 * @param[out]  rightGain           The current right channel gain level.
 *
 * @retval      PMIC_SUCCESS         If the output balance amplifier gain levels
 *                                   were successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the output balance amplifier gain levels
 *                                   could be retrieved.
 */
PMIC_STATUS pmic_audio_output_get_balance(const PMIC_AUDIO_HANDLE handle,
					  PMIC_AUDIO_OUTPUT_BALANCE_GAIN *
					  const leftGain,
					  PMIC_AUDIO_OUTPUT_BALANCE_GAIN *
					  const rightGain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((((handle == stDAC.handle) &&
	      (stDAC.handleState == HANDLE_IN_USE)) ||
	     ((handle == vCodec.handle) &&
	      (vCodec.handleState == HANDLE_IN_USE)) ||
	     ((handle == extStereoIn.handle) &&
	      (extStereoIn.handleState == HANDLE_IN_USE))) &&
	    ((leftGain != (PMIC_AUDIO_OUTPUT_BALANCE_GAIN *) NULL) &&
	     (rightGain != (PMIC_AUDIO_OUTPUT_BALANCE_GAIN *) NULL))) {
		*leftGain = audioOutput.balanceLeftGain;
		*rightGain = audioOutput.balanceRightGain;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Configure and enable the output mono adder.
 *
 * This function configures and enables the output mono adder.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   mode                The desired mono adder operating mode.
 *
 * @retval      PMIC_SUCCESS         If the mono adder was successfully
 *                                   configured and enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle or mono adder mode was
 *                                   invalid.
 * @retval      PMIC_ERROR           If the mono adder could not be reconfigured
 *                                   or enabled.
 */
PMIC_STATUS pmic_audio_output_enable_mono_adder(const PMIC_AUDIO_HANDLE handle,
						const PMIC_AUDIO_MONO_ADDER_MODE
						mode)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int reg_write = SET_BITS(regRX_AUD_AMPS, MONO, 1);
	const unsigned int reg_mask = SET_BITS(regRX_AUD_AMPS, MONO, 3);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		if (mode == MONO_ADDER_OFF) {
			rc = PMIC_ERROR;
		} else if (mode != MONO_ADD_LEFT_RIGHT) {
			rc = PMIC_NOT_SUPPORTED;
		} else {
			rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
					    reg_write, reg_mask);
		}
	}

	return rc;
}

/*!
 * @brief Disable the output mono adder.
 *
 * This function disables the output mono adder.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the mono adder was successfully
 *                                   disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the mono adder could not be disabled.
 */
PMIC_STATUS pmic_audio_output_disable_mono_adder(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int reg_write = 0;
	const unsigned int reg_mask = SET_BITS(regRX_AUD_AMPS, MONO, 3);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS, reg_write,
				    reg_mask);
	}

	return rc;
}

/*!
 * @brief Configure the mono adder output gain level.
 *
 * This function configures the mono adder output amplifier gain level.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   gain                The desired output gain level.
 *
 * @retval      PMIC_SUCCESS         If the mono adder output amplifier gain
 *                                   level was successfully set.
 * @retval      PMIC_PARAMETER_ERROR If the handle or gain level was invalid.
 * @retval      PMIC_ERROR           If the mono adder output amplifier gain
 *                                   level could not be reconfigured.
 */
PMIC_STATUS pmic_audio_output_set_mono_adder_gain(const PMIC_AUDIO_HANDLE
						  handle,
						  const
						  PMIC_AUDIO_MONO_ADDER_OUTPUT_GAIN
						  gain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write = 0;
	const unsigned int reg_mask = SET_BITS(regRX_AUD_AMPS, MONO, 3);

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		if ((gain != MONOADD_GAIN_MINUS_6DB) &&
		    (gain != MONOADD_GAIN_MINUS_3DB) &&
		    (gain != MONOADD_GAIN_0DB)) {
			rc = PMIC_ERROR;
		} else {
			if (gain == MONOADD_GAIN_MINUS_6DB) {
				reg_write = SET_BITS(regRX_AUD_AMPS, MONO, 3);
			} else if (gain == MONOADD_GAIN_MINUS_3DB) {
				reg_write = SET_BITS(regRX_AUD_AMPS, MONO, 2);
			} else {
				reg_write = SET_BITS(regRX_AUD_AMPS, MONO, 1);
			}

			rc = pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
					    reg_write, reg_mask);

			if (rc == PMIC_SUCCESS) {
				audioOutput.monoAdderGain = gain;
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current mono adder output gain level.
 *
 * This function retrieves the current mono adder output amplifier gain level.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[out]  gain                The current output gain level.
 *
 * @retval      PMIC_SUCCESS         If the mono adder output amplifier gain
 *                                   level was successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the mono adder output amplifier gain
 *                                   level could not be retrieved.
 */
PMIC_STATUS pmic_audio_output_get_mono_adder_gain(const PMIC_AUDIO_HANDLE
						  handle,
						  PMIC_AUDIO_MONO_ADDER_OUTPUT_GAIN
						  * const gain)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((((handle == stDAC.handle) &&
	      (stDAC.handleState == HANDLE_IN_USE)) ||
	     ((handle == vCodec.handle) &&
	      (vCodec.handleState == HANDLE_IN_USE)) ||
	     ((handle == extStereoIn.handle) &&
	      (extStereoIn.handleState == HANDLE_IN_USE))) &&
	    (gain != (PMIC_AUDIO_MONO_ADDER_OUTPUT_GAIN *) NULL)) {
		*gain = audioOutput.monoAdderGain;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Set various audio output section options.
 *
 * This function sets one or more audio output section configuration
 * options. The currently supported options include whether to disable
 * the non-inverting mono speaker output, enabling the loudspeaker common
 * bias circuit, enabling detection of headset insertion/removal, and
 * whether to automatically disable the headset amplifiers when a headset
 * insertion/removal has been detected.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   config              The desired audio output section
 *                                  configuration options to be set.
 *
 * @retval      PMIC_SUCCESS         If the desired configuration options were
 *                                   all successfully set.
 * @retval      PMIC_PARAMETER_ERROR If the handle or configuration options
 *                                   were invalid.
 * @retval      PMIC_ERROR           If the desired configuration options
 *                                   could not be set.
 */
PMIC_STATUS pmic_audio_output_set_config(const PMIC_AUDIO_HANDLE handle,
					 const PMIC_AUDIO_OUTPUT_CONFIG config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write_RX = 0;
	unsigned int reg_mask_RX = 0;
	unsigned int reg_write_TX = 0;
	unsigned int reg_mask_TX = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		if ((config & MONO_LOUDSPEAKER_COMMON_BIAS) ||
		    (config & HEADSET_DETECT_ENABLE)) {
			rc = PMIC_NOT_SUPPORTED;
		} else {
			if (config & MONO_SPEAKER_INVERT_OUT_ONLY) {
				reg_write_RX =
				    SET_BITS(regRX_AUD_AMPS, A1CTRL, 1);
				reg_mask_RX =
				    SET_BITS(regRX_AUD_AMPS, A1CTRL, 1);
			}

			if (config & HEADSET_DETECT_ENABLE) {
				reg_write_TX =
				    SET_BITS(regTX_AUD_AMPS, V3_6MB_ON2,
					     1) | SET_BITS(regTX_AUD_AMPS,
							   MBUTTON_HS_ADET_EN,
							   1) |
				    SET_BITS(regTX_AUD_AMPS, HS_SHRT_DET_EN, 1);
				reg_mask_TX =
				    SET_BITS(regTX_AUD_AMPS, V3_6MB_ON2,
					     1) | SET_BITS(regTX_AUD_AMPS,
							   MBUTTON_HS_ADET_EN,
							   1) |
				    SET_BITS(regTX_AUD_AMPS, HS_SHRT_DET_EN, 1);
			}

			if (((reg_mask_RX != 0) &&
			     (pmic_write_reg
			      (PRIO_AUDIO, REG_RX_AUD_AMPS, reg_write_RX,
			       reg_mask_RX) != PMIC_SUCCESS))
			    || ((reg_mask_TX != 0)
				&&
				(pmic_write_reg
				 (PRIO_AUDIO, REG_TX_AUD_AMPS, reg_write_TX,
				  reg_mask_TX) != PMIC_SUCCESS))) {
				rc = PMIC_ERROR;
			} else {
				audioOutput.config |= config;
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Clear various audio output section options.
 *
 * This function clears one or more audio output section configuration
 * options.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[in]   config              The desired audio output section
 *                                  configuration options to be cleared.
 *
 * @retval      PMIC_SUCCESS         If the desired configuration options were
 *                                   all successfully cleared.
 * @retval      PMIC_PARAMETER_ERROR If the handle or configuration options
 *                                   were invalid.
 * @retval      PMIC_ERROR           If the desired configuration options
 *                                   could not be cleared.
 */
PMIC_STATUS pmic_audio_output_clear_config(const PMIC_AUDIO_HANDLE handle,
					   const PMIC_AUDIO_OUTPUT_CONFIG
					   config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	unsigned int reg_write_RX = 0;
	unsigned int reg_mask_RX = 0;
	unsigned int reg_write_TX = 0;
	unsigned int reg_mask_TX = 0;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if (((handle == stDAC.handle) &&
	     (stDAC.handleState == HANDLE_IN_USE)) ||
	    ((handle == vCodec.handle) &&
	     (vCodec.handleState == HANDLE_IN_USE)) ||
	    ((handle == extStereoIn.handle) &&
	     (extStereoIn.handleState == HANDLE_IN_USE))) {
		if ((config & MONO_LOUDSPEAKER_COMMON_BIAS) ||
		    (config & HEADSET_DETECT_ENABLE)) {
			rc = PMIC_NOT_SUPPORTED;
		} else {
			if (config & MONO_SPEAKER_INVERT_OUT_ONLY) {
				reg_write_RX = 0;
				reg_mask_RX =
				    SET_BITS(regRX_AUD_AMPS, A1CTRL, 1);
			}

			if (config & HEADSET_DETECT_ENABLE) {
				reg_write_TX = 0;
				reg_mask_TX =
				    SET_BITS(regTX_AUD_AMPS, V3_6MB_ON2,
					     1) | SET_BITS(regTX_AUD_AMPS,
							   MBUTTON_HS_ADET_EN,
							   1) |
				    SET_BITS(regTX_AUD_AMPS, HS_SHRT_DET_EN, 1);
			}

			if (((reg_mask_RX != 0) &&
			     (pmic_write_reg
			      (PRIO_AUDIO, REG_RX_AUD_AMPS, reg_write_RX,
			       reg_mask_RX) != PMIC_SUCCESS))
			    || ((reg_mask_TX != 0)
				&&
				(pmic_write_reg
				 (PRIO_AUDIO, REG_TX_AUD_AMPS, reg_write_TX,
				  reg_mask_TX) != PMIC_SUCCESS))) {
				rc = PMIC_ERROR;
			} else {
				audioOutput.config &= ~config;
			}
		}
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Get the current audio output section options.
 *
 * This function retrieves the current audio output section configuration
 * option settings.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 * @param[out]  config              The current audio output section
 *                                  configuration option settings.
 *
 * @retval      PMIC_SUCCESS         If the current configuration options were
 *                                   successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the current configuration options
 *                                   could not be retrieved.
 */
PMIC_STATUS pmic_audio_output_get_config(const PMIC_AUDIO_HANDLE handle,
					 PMIC_AUDIO_OUTPUT_CONFIG *
					 const config)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Use a critical section to ensure a consistent hardware state. */
	down_interruptible(&mutex);

	if ((((handle == stDAC.handle) &&
	      (stDAC.handleState == HANDLE_IN_USE)) ||
	     ((handle == vCodec.handle) &&
	      (vCodec.handleState == HANDLE_IN_USE)) ||
	     ((handle == extStereoIn.handle) &&
	      (extStereoIn.handleState == HANDLE_IN_USE))) &&
	    (config != (PMIC_AUDIO_OUTPUT_CONFIG *) NULL)) {
		*config = audioOutput.config;

		rc = PMIC_SUCCESS;
	}

	/* Exit the critical section. */
	up(&mutex);

	return rc;
}

/*!
 * @brief Enable the phantom ground circuit that is used to help identify
 *        the type of headset that has been inserted.
 *
 * This function enables the phantom ground circuit that is used to help
 * identify the type of headset (e.g., stereo or mono) that has been inserted.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the phantom ground circuit was
 *                                   successfully enabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the phantom ground circuit could not
 *                                   be enabled.
 */
PMIC_STATUS pmic_audio_output_enable_phantom_ground(const PMIC_AUDIO_HANDLE
						    handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int reg_write = SET_BITS(regTX_AUD_AMPS, V3_6MB_ON2, 1);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if (((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE))
	    || ((handle == vCodec.handle)
		&& (vCodec.handleState == HANDLE_IN_USE))) {
		rc = pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, reg_write,
				    reg_write);
	}

	return rc;
}

/*!
 * @brief Disable the phantom ground circuit that is used to help identify
 *        the type of headset that has been inserted.
 *
 * This function disables the phantom ground circuit that is used to help
 * identify the type of headset (e.g., stereo or mono) that has been inserted.
 *
 * @param[in]   handle              Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the phantom ground circuit was
 *                                   successfully disabled.
 * @retval      PMIC_PARAMETER_ERROR If the handle was invalid.
 * @retval      PMIC_ERROR           If the phantom ground circuit could not
 *                                   be disabled.
 */
PMIC_STATUS pmic_audio_output_disable_phantom_ground(const PMIC_AUDIO_HANDLE
						     handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;
	const unsigned int reg_mask = SET_BITS(regTX_AUD_AMPS, V3_6MB_ON2, 1);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	if (((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE))
	    || ((handle == vCodec.handle)
		&& (vCodec.handleState == HANDLE_IN_USE))) {
		rc = pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, 0, reg_mask);
	}

	return rc;
}

/*@}*/

/**************************************************************************
 * Static functions.
 **************************************************************************
 */

/*!
 * @name Audio Driver Internal Support Functions
 * These non-exported internal functions are used to support the functionality
 * of the exported audio APIs.
 */
/*@{*/

/*!
 * @brief Enables the 5.6V boost for the microphone bias 2 circuit.
 *
 * This function enables the switching regulator SW3 and configures it to
 * provide the 5.6V boost that is required for driving the microphone bias 2
 * circuit when using a 5-pole jack configuration (which is the case for the
 * Sphinx board).
 *
 * @retval      PMIC_SUCCESS         The 5.6V boost was successfully enabled.
 * @retval      PMIC_ERROR           Failed to enable the 5.6V boost.
 */
static PMIC_STATUS pmic_audio_mic_boost_enable(void)
{
	PMIC_STATUS rc = PMIC_ERROR;
	const unsigned int reg_write = SET_BITS(regSWCTRL, SW3_EN, 1) |
	    SET_BITS(regSWCTRL, SW3, 1);
	const unsigned int reg_mask = SET_BITS(regSWCTRL, SW3_EN, 1) |
	    SET_BITS(regSWCTRL, SW3, 1);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	rc = pmic_write_reg(PRIO_PWM, REG_SWCTRL, reg_write, reg_mask);

	return rc;
}

/*!
 * @brief Disables the 5.6V boost for the microphone bias 2 circuit.
 *
 * This function disables the switching regulator SW3 to turn off the 5.6V
 * boost for the microphone bias 2 circuit.
 *
 * @retval      PMIC_SUCCESS         The 5.6V boost was successfully disabled.
 * @retval      PMIC_ERROR           Failed to disable the 5.6V boost.
 */
static PMIC_STATUS pmic_audio_mic_boost_disable(void)
{
	PMIC_STATUS rc = PMIC_ERROR;
	const unsigned int reg_write = 0;
	const unsigned int reg_mask = SET_BITS(regSWCTRL, SW3_EN, 1);

	/* No critical section required here since we are not updating any
	 * global data.
	 */

	rc = pmic_write_reg(PRIO_PWM, REG_SWCTRL, reg_write, reg_mask);

	return rc;
}

/*!
 * @brief Free a device handle previously acquired by calling pmic_audio_open().
 *
 * Terminate further access to the PMIC audio hardware that was previously
 * acquired by calling pmic_audio_open(). This now allows another thread to
 * successfully call pmic_audio_open() to gain access.
 *
 * Note that we will shutdown/reset the Voice CODEC or Stereo DAC as well as
 * any associated audio input/output components that are no longer required.
 *
 * Also note that this function should only be called with the mutex already
 * acquired.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the close request was successful.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
static PMIC_STATUS pmic_audio_close_handle(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	/* Match up the handle to the audio device and then close it. */
	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		if (stDAC.callback != (PMIC_AUDIO_CALLBACK) NULL) {
			rc = pmic_audio_deregister(&(stDAC.callback),
						   &(stDAC.eventMask));
		} else {
			rc = PMIC_SUCCESS;
		}

		/* Also shutdown the Stereo DAC hardware. The simplest way to
		 * do this is to simply call pmic_audio_reset_device() which will
		 * restore the ST_DAC register to it's initial power-on state.
		 *
		 * This will also shutdown the audio output section if no one
		 * else is still using it.
		 */
		pmic_audio_reset_device(stDAC.handle);

		if (rc == PMIC_SUCCESS) {
			stDAC.handle = AUDIO_HANDLE_NULL;
			stDAC.handleState = HANDLE_FREE;
		}
	} else if ((handle == vCodec.handle) &&
		   (vCodec.handleState == HANDLE_IN_USE)) {
		if (vCodec.callback != (PMIC_AUDIO_CALLBACK) NULL) {
			rc = pmic_audio_deregister(&(vCodec.callback),
						   &(vCodec.eventMask));
		} else {
			rc = PMIC_SUCCESS;
		}

		/* Also shutdown the Voice CODEC and audio input hardware. The
		 * simplest way to do this is to simply call pmic_audio_reset_device()
		 * which will restore the AUD_CODEC register to it's initial
		 * power-on state.
		 *
		 * This will also shutdown the audio output section if no one
		 * else is still using it.
		 */
		pmic_audio_reset_device(vCodec.handle);

		if (rc == PMIC_SUCCESS) {
			vCodec.handle = AUDIO_HANDLE_NULL;
			vCodec.handleState = HANDLE_FREE;
		}
	} else if ((handle == extStereoIn.handle) &&
		   (extStereoIn.handleState == HANDLE_IN_USE)) {
		if (extStereoIn.callback != (PMIC_AUDIO_CALLBACK) NULL) {
			rc = pmic_audio_deregister(&(extStereoIn.callback),
						   &(extStereoIn.eventMask));
		} else {
			rc = PMIC_SUCCESS;
		}

		/* Call pmic_audio_reset_device() here to shutdown the audio output
		 * section if no one else is still using it.
		 */
		pmic_audio_reset_device(extStereoIn.handle);

		if (rc == PMIC_SUCCESS) {
			extStereoIn.handle = AUDIO_HANDLE_NULL;
			extStereoIn.handleState = HANDLE_FREE;
		}
	}

	return rc;
}

/*!
 * @brief Reset the selected audio hardware control registers to their
 *        power on state.
 *
 * This resets all of the audio hardware control registers currently
 * associated with the device handle back to their power on states. For
 * example, if the handle is associated with the Stereo DAC and a
 * specific output port and output amplifiers, then this function will
 * reset all of those components to their initial power on state.
 *
 * This function can only be called if the mutex has already been acquired.
 *
 * @param[in]   handle          Device handle from pmic_audio_open() call.
 *
 * @retval      PMIC_SUCCESS         If the reset operation was successful.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_ERROR           If the reset was unsuccessful.
 */
static PMIC_STATUS pmic_audio_reset_device(const PMIC_AUDIO_HANDLE handle)
{
	PMIC_STATUS rc = PMIC_PARAMETER_ERROR;

	if ((handle == stDAC.handle) && (stDAC.handleState == HANDLE_IN_USE)) {
		/* Also shutdown the audio output section if nobody else is using it. */
		if ((vCodec.handleState == HANDLE_FREE) &&
		    (extStereoIn.handleState == HANDLE_FREE)) {
			pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
				       RESET_RX_AUD_AMPS, REG_FULLMASK);
		}

		rc = pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, RESET_ST_DAC,
				    REG_FULLMASK);

		if (rc == PMIC_SUCCESS) {
			/* Also reset the driver state information to match. Note that we
			 * keep the device handle and event callback settings unchanged
			 * since these don't affect the actual hardware and we rely on
			 * the user to explicitly close the handle or deregister callbacks
			 */
			stDAC.busID = AUDIO_DATA_BUS_2;
			stDAC.protocol = NORMAL_MSB_JUSTIFIED_MODE;
			stDAC.masterSlave = BUS_SLAVE_MODE;
			stDAC.numSlots = USE_2_TIMESLOTS;
			stDAC.clockIn = CLOCK_IN_CLKIN;
			stDAC.samplingRate = STDAC_RATE_44_1_KHZ;
			stDAC.clockFreq = STDAC_CLI_13MHZ;
			stDAC.invert = NO_INVERT;
			stDAC.timeslot = USE_TS0_TS1;
			stDAC.config = (PMIC_AUDIO_STDAC_CONFIG) 0;
		}
	} else if ((handle == vCodec.handle)
		   && (vCodec.handleState == HANDLE_IN_USE)) {
		/* Disable the audio input section when disabling the Voice CODEC. */
		pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, RESET_TX_AUD_AMPS,
			       REG_FULLMASK);

		/* Also shutdown the audio output section if nobody else is using it. */
		if ((stDAC.handleState == HANDLE_FREE) &&
		    (extStereoIn.handleState == HANDLE_FREE)) {
			pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
				       RESET_RX_AUD_AMPS, REG_FULLMASK);
		}

		rc = pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC, RESET_AUD_CODEC,
				    REG_FULLMASK);

		if (rc == PMIC_SUCCESS) {
			/* Also reset the driver state information to match. Note that we
			 * keep the device handle and event callback settings unchanged
			 * since these don't affect the actual hardware and we rely on
			 * the user to explicitly close the handle or deregister callbacks
			 */
			vCodec.busID = AUDIO_DATA_BUS_1;
			vCodec.protocol = NETWORK_MODE;
			vCodec.masterSlave = BUS_SLAVE_MODE;
			vCodec.numSlots = USE_4_TIMESLOTS;
			vCodec.clockIn = CLOCK_IN_CLKIN;
			vCodec.samplingRate = VCODEC_RATE_8_KHZ;
			vCodec.clockFreq = VCODEC_CLI_13MHZ;
			vCodec.invert = NO_INVERT;
			vCodec.timeslot = USE_TS0;
			vCodec.config = INPUT_HIGHPASS_FILTER |
			    OUTPUT_HIGHPASS_FILTER;
		}
	} else if ((handle == extStereoIn.handle) &&
		   (extStereoIn.handleState == HANDLE_IN_USE)) {
		/* Also shutdown the audio output section if nobody else is using it. */
		if ((stDAC.handleState == HANDLE_FREE) &&
		    (extStereoIn.handleState == HANDLE_FREE)) {
			pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
				       RESET_RX_AUD_AMPS, REG_FULLMASK);
		}

		/* We don't need to reset any other registers for this case. */
		rc = PMIC_SUCCESS;
	}

	return rc;
}

/*!
 * @brief Deregister the callback function and event mask currently associated
 *        with an audio device handle.
 *
 * This function deregisters any existing callback function and event mask for
 * the given audio device handle. This is done by either calling the
 * pmic_audio_clear_callback() API or by closing the device handle.
 *
 * Note that this function should only be called with the mutex already
 * acquired. We will also acquire the spinlock here to prevent possible
 * race conditions with the interrupt handler.
 *
 * @param[in]   callback            The current event callback function pointer.
 * @param[in]   eventMask           The current audio event mask.
 *
 * @retval      PMIC_SUCCESS         If the callback function and event mask
 *                                   were both successfully deregistered.
 * @retval      PMIC_ERROR           If either the callback function or the
 *                                   event mask was not successfully
 *                                   deregistered.
 */
static PMIC_STATUS pmic_audio_deregister(PMIC_AUDIO_CALLBACK * const callback,
					 PMIC_AUDIO_EVENTS * const eventMask)
{
	unsigned long flags;
	type_event_notification eventNotify;
	PMIC_STATUS rc = PMIC_SUCCESS;

	/* Deregister each of the PMIC events that we had previously
	 * registered for by calling pmic_event_subscribe().
	 */
	if (*eventMask & (HEADSET_DETECTED |
			  HEADSET_STEREO | HEADSET_MONO | HEADSET_REMOVED)) {
		/* We need to deregister for the A1 amplifier interrupt. */
		eventNotify.event = EVENT_A1I;
		eventNotify.callback = pmic_audio_event_handler;
		eventNotify.param = (void *)(CORE_EVENT_A1I);
		if (pmic_event_unsubscribe(eventNotify) == PMIC_SUCCESS) {
			*eventMask &= ~(HEADSET_DETECTED |
					HEADSET_STEREO |
					HEADSET_MONO | HEADSET_REMOVED);
			TRACEMSG(KERN_INFO "Deregistered for EVENT_A1I\n");
		} else {
			rc = PMIC_ERROR;
		}
	}

	if (*eventMask & (PTT_BUTTON_PRESS |
			  PTT_BUTTON_RANGE | PTT_SHORT_OR_INVALID)) {
		/* We need to deregister for the Push-to-Talk (PTT) interrupt. */
		eventNotify.event = EVENT_PTTI;
		eventNotify.callback = pmic_audio_event_handler;
		eventNotify.param = (void *)(CORE_EVENT_PTT);
		rc = pmic_event_subscribe(eventNotify);

		if (rc == PMIC_SUCCESS) {
			*eventMask &= ~(PTT_BUTTON_PRESS |
					PTT_BUTTON_RANGE |
					PTT_SHORT_OR_INVALID);
			TRACEMSG(KERN_INFO "Deregistered for EVENT_PTTI\n");
		} else {
			rc = PMIC_ERROR;
		}
	}

	if (rc == PMIC_SUCCESS) {
		/* We need to grab the spinlock here to create a critical section to
		 * avoid any possible race conditions with the interrupt handler
		 * tasklet.
		 */
		spin_lock_irqsave(&lock, flags);

		/* Restore the initial reset values for the callback function
		 * and event mask parameters. This should be NULL and zero,
		 * respectively.
		 */
		*callback = (PMIC_AUDIO_CALLBACK) NULL;
		*eventMask = 0;

		/* Exit the critical section. */
		spin_unlock_irqrestore(&lock, flags);
	}

	return rc;
}

/*!
 * @brief This is the default event handler for the audio driver.
 *
 * This function is called by the low-level PMIC interrupt handler whenever
 * a registered event occurs. This function will perform whatever additional
 * processing may be required to fully identify the event that just occur and
 * then call the appropriate callback function to complete the handling of the
 * event.
 *
 * @param[in]   param               The parameter that was provided when this
 *                                  event handler was registered (unused).
 */
static void pmic_audio_event_handler(void *param)
{
	unsigned long flags;

	/* Update the global list of active interrupt events. */
	spin_lock_irqsave(&lock, flags);
	eventID |= (PMIC_CORE_EVENT) (param);
	spin_unlock_irqrestore(&lock, flags);

	/* Schedule the tasklet to be run as soon as it is convenient to do so. */
	tasklet_schedule(&audioTasklet);
}

/*!
 * @brief This is the audio driver tasklet that handles interrupt events.
 *
 * This function is scheduled by the audio driver interrupt handler
 * pmic_audio_event_handler() to complete the processing of both the
 * A1I and PTTI interrupt events.
 *
 * Since this tasklet runs with interrupts enabled, we can safely call
 * the ADC driver, if necessary, to properly detect the type of headset
 * that is attached and to call any user-registered callback functions.
 *
 * @param[in]   arg                 The parameter that was provided above in
 *                                  the DECLARE_TASKLET() macro (unused).
 */
void pmic_audio_tasklet(unsigned long arg)
{
	static const unsigned int ALEFT_AMP = 0x20;
	static const unsigned int PTTI_MASK = 0x800;
	static const unsigned HEADSET_DETECT_ADC_CHANNEL = 7;
	static const unsigned short ADC_FULLSCALE = 0x3ff;
	static const unsigned short ADC_FULLSCALE_VOLTAGE = 230;
	static const unsigned short ADC_SCALE_FACTOR = 100;
	static const unsigned V2_DETECT_THRESHOLD = 264;
	static const unsigned SHORT_DETECT_THRESHOLD = 5;
	static enum {
		PTT_OFF,
		PTT_WAIT,
		PTT_DET
	} ptt_state = PTT_OFF;
	unsigned short adcResult = 0;
	unsigned voltageHeadset_IDScaled = 0;
	PMIC_AUDIO_EVENTS activeEvents = 0;
	unsigned long flags = 0;

	if (eventID == CORE_EVENT_A1I) {
		spin_lock_irqsave(&lock, flags);
		eventID &= ~CORE_EVENT_A1I;
		spin_unlock_irqrestore(&lock, flags);

		/* Explicitly initialize this just to be safe. */
		ptt_state = PTT_OFF;

		if (headsetState == HEADSET_ON) {
			/* The only way to get an A1 interrupt with a headset attached
			 * is by removing the headset. Therefore, we simply flag the
			 * headset removal event and set the headsetState value to match.
			 */
			headsetState = NO_HEADSET;
			activeEvents = HEADSET_REMOVED;
		} else {
			/* No headset was previously attached so this A1 interrupt must
			 * be signalling a headset insertion event.
			 *
			 * Follow the Headset ID Detection Flow Diagram given in the
			 * sc55112 DTS document.
			 *
			 * Start by temporarily disabling the Aleft amplifier, masking the
			 * PTT comparator interrupt, and performing an AD7 measurement.
			 */
			if ((pmic_write_reg
			     (PRIO_AUDIO, REG_RX_AUD_AMPS, 0,
			      ALEFT_AMP) != PMIC_SUCCESS)
			    ||
			    (pmic_write_reg
			     (PRIO_CORE, REG_IMR, PTTI_MASK,
			      PTTI_MASK) != PMIC_SUCCESS)) {
				TRACEMSG(KERN_INFO
					 "WARNING: pmic_audio_event_handler() failed "
					 "to mask PTT interrupt.\n");
			} else {
				/* Now read the ADC comparator voltage level to determine
				 * the type of headset or PTT button event. Note that the
				 * sc55112 DTS document gives V2=2.775V and we use a
				 * +/-5% range to determine a lower bound for the value of
				 * V2_DETECT_THRESHOLD = 2.64V.
				 *
				 * Similarly, we use a +/-5% range to determine the upper
				 * bound for the value of SHORT_DETECT_THRESHOLD = 0.05V
				 * (since a short circuit would ideally read 0V).
				 */
				if (pmic_adc_convert
				    (HEADSET_DETECT_ADC_CHANNEL,
				     &adcResult) != PMIC_SUCCESS) {
					TRACEMSG(KERN_INFO
						 "WARNING: pmic_audio_event_handler() "
						 "failed to read AD7 voltage following "
						 "an A1 interrupt.\n");
				} else {
					voltageHeadset_IDScaled =
					    ((unsigned)adcResult *
					     ADC_SCALE_FACTOR) / ADC_FULLSCALE *
					    ADC_FULLSCALE_VOLTAGE;
					TRACEMSG(KERN_INFO
						 "Headset detect voltage = %u V "
						 "(x10000)\n",
						 voltageHeadset_IDScaled);
					if (voltageHeadset_IDScaled >=
					    V2_DETECT_THRESHOLD) {
						/* Found Mono Headset, do not enable the Aleft amplifier
						 * but unmask the PTT interrupt.
						 */
						activeEvents =
						    HEADSET_DETECTED |
						    HEADSET_MONO;
						if (pmic_write_reg
						    (PRIO_CORE, REG_IMR, 0,
						     PTTI_MASK) !=
						    PMIC_SUCCESS) {
							TRACEMSG(KERN_INFO
								 "WARNING: "
								 "pmic_audio_event_handler() "
								 "failed to reenable PTT "
								 "interrupt.\n");
						}

						headsetState = HEADSET_ON;

						/* Now wait for a PTT interrupt to signal button
						 * press.
						 */
						ptt_state = PTT_WAIT;
					} else if (voltageHeadset_IDScaled >=
						   SHORT_DETECT_THRESHOLD) {
						/* Found Stereo Headset, enable the Aleft amplifier and
						 * unmask the PTT interrupt.
						 */
						activeEvents =
						    HEADSET_DETECTED |
						    HEADSET_STEREO;
						if ((pmic_write_reg
						     (PRIO_AUDIO,
						      REG_RX_AUD_AMPS,
						      ALEFT_AMP,
						      ALEFT_AMP) !=
						     PMIC_SUCCESS)
						    ||
						    (pmic_write_reg
						     (PRIO_CORE, REG_IMR,
						      PTTI_MASK,
						      PTTI_MASK) !=
						     PMIC_SUCCESS)) {
							TRACEMSG(KERN_INFO
								 "WARNING: "
								 "pmic_audio_event_handler() "
								 "failed to reenable Aleft "
								 "amplifier or the "
								 "PTT interrupt.\n");
						}

						headsetState = HEADSET_ON;

						/* Now wait for a PTT interrupt before starting another
						 * AD7 read on the PTT_DET pin.
						 */
						ptt_state = PTT_DET;
					} else {
						activeEvents =
						    PTT_SHORT_OR_INVALID;
					}
				}
			}
		}
	} else if (eventID == CORE_EVENT_PTT) {
		spin_lock_irqsave(&lock, flags);
		eventID &= ~CORE_EVENT_PTT;
		spin_unlock_irqrestore(&lock, flags);

		if ((ptt_state == PTT_OFF) || (ptt_state == PTT_WAIT)) {
			activeEvents = PTT_BUTTON_PRESS;
		} else {
			activeEvents = PTT_BUTTON_RANGE;

			if (pmic_adc_convert
			    (HEADSET_DETECT_ADC_CHANNEL,
			     &adcResult) != PMIC_SUCCESS) {
				TRACEMSG(KERN_INFO
					 "WARNING: pmic_audio_event_handler() "
					 "failed to read AD7 voltage for PTT "
					 "button level.\n");
			} else {
				ptt_button_level =
				    ((unsigned)adcResult * ADC_SCALE_FACTOR) /
				    ADC_FULLSCALE * ADC_FULLSCALE_VOLTAGE;
				TRACEMSG(KERN_INFO "PTT button level = %u\n",
					 ptt_button_level);
			}
		}
	}

	/* Begin a critical section here so that we don't register/deregister
	 * for events or open/close the audio driver while the existing event
	 * handler (if it is currently defined) is in the middle of handling
	 * the current event.
	 */
	spin_lock_irqsave(&lock, flags);

	if (activeEvents & ~(stDAC.eventMask |
			     vCodec.eventMask | extStereoIn.eventMask)) {
		/* Default handler for events without callbacks is just to ignore
		 * the events and clear them from the activeEvents list.
		 */
		activeEvents &= (stDAC.eventMask |
				 vCodec.eventMask | extStereoIn.eventMask);
	}

	/* Finally, call the user-defined callback functions if required. */
	if ((stDAC.handleState == HANDLE_IN_USE) &&
	    (stDAC.callback != NULL) && (activeEvents & stDAC.eventMask)) {
		(*stDAC.callback) (activeEvents);
	}
	if ((vCodec.handleState == HANDLE_IN_USE) &&
	    (vCodec.callback != NULL) && (activeEvents & vCodec.eventMask)) {
		(*vCodec.callback) (activeEvents);
	}
	if ((extStereoIn.handleState == HANDLE_IN_USE) &&
	    (extStereoIn.callback != NULL) &&
	    (activeEvents & extStereoIn.eventMask)) {
		(*extStereoIn.callback) (activeEvents);
	}

	/* Exit the critical section. */
	spin_unlock_irqrestore(&lock, flags);
}

/*@}*/

/**************************************************************************
 * Module initialization and termination functions.
 *
 * Note that if this code is compiled into the kernel, then the
 * module_init() function will be called within the device_initcall()
 * group.
 **************************************************************************
 */

/*!
 * @name Audio Driver Loading/Unloading Functions
 * These non-exported internal functions are used to support the audio
 * device driver initialization and de-initialization operations.
 */
/*@{*/

/*!
 * @brief This is the audio device driver initialization function.
 *
 * This function is called by the kernel when this device driver is first
 * loaded.
 */
static int __init sc55112_audio_init(void)
{
	TRACEMSG(KERN_INFO "sc55112 Audio loading\n");

	return 0;
}

/*!
 * @brief This is the audio device driver de-initialization function.
 *
 * This function is called by the kernel when this device driver is about
 * to be unloaded.
 */
static void __exit sc55112_audio_exit(void)
{
	TRACEMSG(KERN_INFO "sc55112 Audio unloaded\n");

	/* Close all device handles that are still open. This will also
	 * deregister any callbacks that may still be active.
	 */
	if (stDAC.handleState == HANDLE_IN_USE) {
		pmic_audio_close(stDAC.handle);
	}
	if (vCodec.handleState == HANDLE_IN_USE) {
		pmic_audio_close(vCodec.handle);
	}
	if (extStereoIn.handleState == HANDLE_IN_USE) {
		pmic_audio_close(extStereoIn.handle);
	}

	/* Explicitly reset all of the audio registers so that there is no
	 * possibility of leaving the sc55112 audio hardware in a state
	 * where it can cause problems if there is no device driver loaded.
	 */
	pmic_write_reg(PRIO_AUDIO, REG_ST_DAC, RESET_ST_DAC, REG_FULLMASK);
	pmic_write_reg(PRIO_AUDIO, REG_AUD_CODEC, RESET_AUD_CODEC,
		       REG_FULLMASK);
	pmic_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS, RESET_RX_AUD_AMPS,
		       REG_FULLMASK);
	pmic_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, RESET_TX_AUD_AMPS,
		       REG_FULLMASK);
}

/*@}*/

/*
 * Module entry points and description information.
 */

module_init(sc55112_audio_init);
module_exit(sc55112_audio_exit);

MODULE_DESCRIPTION("sc55112 Audio device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
