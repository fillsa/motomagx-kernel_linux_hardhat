/*
 * sound/oss/pnx4008/pnx4008-dsp.c
 *
 * driver for Philips PNX4008 Audio and Teak DSP
 *
 * Features:
 *  - OSS Sound Support:
 *    - Sampling rates: 8, 11.025, 16, 22.05, 32, 44.1, 48KHz
 *    - Format: Signed 16 bit little-endian stereo/mono (mono is software emulated)
 *  - Selection of recording modes:
 *    - both left and right channels (average of both in mono)
 *    - left only channel
 *    - right only channel
 *  - Selection of output:
 *    - DAC1
 *    - DAC2
 *  - Recording, playback and simultaneous recording/playback
 *  - Volume control for recording and playback
 *  - Loading DSP image from a user-specified location
 *  - Variable DSP PLL frequency as specified in the DSP binary file
 *  - Option to dynamically disable/enable clocks/amplifiers
 *    (power savings vs loud clicks during clock/amplifier enable/disable)
 *
 * Author: Dmitry Chigirev <source@mvista.com>
 *
 * register initialization is based on code examples provided by Philips
 * Copyright (c) 2005 Koninklijke Philips Electronics N.V.
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <asm/arch/hardware.h>
#include <linux/bitops.h>
#include <asm/arch/pm.h>
#include <linux/interrupt.h>
#include <asm/arch/gpio.h>

#include <linux/wait.h>

#include "sound_config.h"	/*include many macro definiation */
#include "dev_table.h"		/*sound_install_audiodrv()... */

#include <linux/soundcard.h>

#define MODULE_NAME "PNX4008-DSP"
#define PKMOD MODULE_NAME ": "

#define DSP_RX_BUF_SIZE_FACT 16
#define DSP_TX_BUF_SIZE_FACT 15

#define DSP_MAX_FRAG_SIZE_FACT 13

#define DSP_RX_BUF_SIZE (1 << DSP_RX_BUF_SIZE_FACT)
#define DSP_TX_BUF_SIZE (1 << DSP_TX_BUF_SIZE_FACT)

#define STOP_CMD                0x1
#define PLAY_CMD                0x2
#define RETURN_BUF_BASE_ADD     0x3
#define RETURN_BUF_SIZE         0x4
#define SET_ARM_BUF_BASE_ADD    0x5
#define SET_ARM_BUF_SIZE        0x6
#define SET_SRC_BASE_ADD        0x7	/* format : <SET_SRC_BASE_ADD> <ADD_HIGH> <ADD_LOW> */
#define SET_SRC_SIZE_IN_BYTES   0x8	/* format : <SET_SRC_BASE_ADD> <SIZE_HIGH> <SIZE_LOW> */
#define RECORD_CMD              0x9
#define PLAY_RECORD_CMD         0xA
#define SET_SRC_BASE_ADD_RECORD 0xB
#define SET_SRC_SIZE_IN_BYTES_RECORD 0xC
#define PLAY_SAMPLING_FREQ_CMD       0xD
#define RECORD_SAMPLING_FREQ_CMD     0xE

#define AUDIO_CFG_96000 0
#define AUDIO_CFG_48000 1
#define AUDIO_CFG_44100 2
#define AUDIO_CFG_32000 3
#define AUDIO_CFG_22050 4
#define AUDIO_CFG_16000 5
#define AUDIO_CFG_11025 6
#define AUDIO_CFG_8000  7

#define MAX_VOLUME 100

#define SPI_CTRL  IO_ADDRESS((PNX4008_PWRMAN_BASE + 0xC4))

#define PIO_OUTP_SET IO_ADDRESS((PNX4008_PIO_BASE + 0x04))

#define DSP_PDATA IO_ADDRESS((PNX4008_DSP_BASE + 0x00))
#define DSP_PADR  IO_ADDRESS((PNX4008_DSP_BASE + 0x04))
#define DSP_PCFG  IO_ADDRESS((PNX4008_DSP_BASE + 0x08))
#define DSP_PSTS  IO_ADDRESS((PNX4008_DSP_BASE + 0x0C))
#define DSP_PSEM  IO_ADDRESS((PNX4008_DSP_BASE + 0x10))
#define DSP_PMASK IO_ADDRESS((PNX4008_DSP_BASE + 0x14))
#define DSP_PCLEAR    IO_ADDRESS((PNX4008_DSP_BASE + 0x18))
#define DSP_APBP_SEM  IO_ADDRESS((PNX4008_DSP_BASE + 0x1C))
#define DSP_APBP_COM0 IO_ADDRESS((PNX4008_DSP_BASE + 0x20))
#define DSP_APBP_REP0 IO_ADDRESS((PNX4008_DSP_BASE + 0x24))
#define DSP_APBP_COM1 IO_ADDRESS((PNX4008_DSP_BASE + 0x28))
#define DSP_APBP_REP1 IO_ADDRESS((PNX4008_DSP_BASE + 0x2C))
#define DSP_APBP_COM2 IO_ADDRESS((PNX4008_DSP_BASE + 0x30))
#define DSP_APBP_REP2 IO_ADDRESS((PNX4008_DSP_BASE + 0x34))

#define BTDMP_MUX_CTRL    IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x00))
#define FRAME_SYNC_MATCH1 IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x04))
#define FRAME_SYNC_MATCH2 IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x08))
#define FRAME_SYNC_CTRL1 IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x0C))
#define FRAME_SYNC_CTRL2 IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x10))
#define CODEC_RESET_CTRL IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x14))
#define DAC1_I2S_CTRL  IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x18))
#define DAC1_SCK_DIV   IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x1C))
#define DAC1_WS_DIV    IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x20))
#define DAC1_CTRL_INTI IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x24))
#define DAC1_STAT      IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x28))
#define DAC1_CTRL_DAC  IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x2C))
#define DAC2_I2S_CTRL  IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x30))
#define DAC2_SCK_DIV   IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x34))
#define DAC2_WS_DIV    IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x38))
#define DAC2_CTRL_INTI IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x3C))
#define DAC2_STAT      IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x40))
#define DAC2_CTRL_DAC  IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x44))
#define ADC_I2S_CTRL IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x48))
#define ADC_SCK_DIV  IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x4C))
#define ADC_WS_DIV   IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x50))
#define ADC_DEC_CTRL IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x54))
#define ADC_CTRL     IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x58))
#define ADC_STAT     IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x5C))

#define DSPPLL_CTRL   IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x60))
#define DSPCLK_CTRL   IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x64))
#define AUDIOCLK_CTRL IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x68))
#define AUDIOPLL_CTRL IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x6C))
#define AUDIOPLL_FB   IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x70))
#define AUDIO_DAC1DIV IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x74))
#define AUDIO_DAC2DIV IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x78))
#define AUDIO_ADCDIV  IO_ADDRESS((PNX4008_AUDIOCONFIG_BASE + 0x7C))

#define DSP_PCFG_MEMSEL_SHIFT 12
#define DSP_PCFG_MEMSEL_MASK  (0xf<<12)
#define DSP_PCFG_MEMSEL_INTERNAL_MEM 0
#define DSP_PCFG_MEMSEL_ZSI_MEM      (1 << 12)
#define DSP_PCFG_MEMSEL_X_MEM        (2 << 12)
#define DSP_PCFG_MEMSEL_Y_MEM        (3 << 12)
#define DSP_PCFG_MEMSEL_Z_MEM        (4 << 12)
#define DSP_PCFG_MEMSEL_P_WRITE_ONLY_MEM (5 << 12)
#define DSP_PCFG_MEMSEL_EXP_BUS_0_MEM    (6 << 12)
#define DSP_PCFG_MEMSEL_EXP_BUS_1_MEM    (7 << 12)
#define DSP_PCFG_RRIE2    (1<<11)
#define DSP_PCFG_RRIE1    (1<<10)
#define DSP_PCFG_RRIE0    (1<<9)
#define DSP_PCFG_WFEIE    (1<<8)
#define DSP_PCFG_WFFIE    (1<<7)
#define DSP_PCFG_RFNEIE   (1<<6)
#define DSP_PCFG_RFFIE    (1<<5)
#define DSP_PCFG_RS       (1<<4)
#define DSP_PCFG_DRS_SHIFT 2
#define DSP_PCFG_DRS_MASK (3<<2)
#define DSP_PCFG_AIM      (1<<1)
#define DSP_PCFG_DSPR     1

#define DSP_PSTS_RCOMIM2  (1<<15)
#define DSP_PSTS_RCOMIM1  (1<<14)
#define DSP_PSTS_RCOMIM0  (1<<13)
#define DSP_PSTS_RRI2     (1<<12)
#define DSP_PSTS_RRI1     (1<<11)
#define DSP_PSTS_RRI      (1<<10)
#define DSP_PSTS_PSEMI    (1<<9)
#define DSP_PSTS_WFEI     (1<<8)
#define DSP_PSTS_WFFI     (1<<7)
#define DSP_PSTS_RFNEI    (1<<6)
#define DSP_PSTS_RFFI     (1<<5)
#define DSP_PSTS_PRST     (1<<2)
#define DSP_PSTS_WTIP     (1<<1)
#define DSP_PSTS_RTIP     1

#define SET_BITS(reg, mask) __raw_writel(__raw_readl(reg)|(mask), (reg))
#define CLR_BITS(reg, mask)  __raw_writel(__raw_readl(reg)&(~mask), (reg))
#define GET_BITS(reg, mask) (__raw_readl(reg)&(mask))

#define SET_BITS_W(reg, mask) __raw_writew(__raw_readw(reg)|(mask), (reg))
#define CLR_BITS_W(reg, mask)  __raw_writew(__raw_readw(reg)&(~mask), (reg))
#define GET_BITS_W(reg, mask) (__raw_readw(reg)&(mask))

static unsigned long dsp_audio_status;
enum {
	AUDIO_OPENED = 0,
	AUDIO_POWER_OFF,
	AUDIO_TX_INITED,
	AUDIO_RX_INITED,
	AUDIO_RUNNING,
	DSP_REGION_INITED,
	AUDIO_REGION_INITED,
	SPI2_REGION_INITED,
	AUDIO_DAC1_SELECTED,
	AUDIO_DAC2_SELECTED,
	AUDIO_LINE_SELECTED,
	AUDIO_LINE1_SELECTED,
	AUDIO_LINE2_SELECTED,
	DSP_IRQ_INITED,
	AUDIO_TX_ZERO_DATA,
	DSP_PLL_ACQUIRED,
	DSP_CODE_LOADED,
	GPO_14_INITED,
	GPO_15_INITED,
	GPO_16_INITED,
};

struct dsp_data_rec {
	unsigned int size;
	int addr;
	short dat[8];
};

const int audio_cfg_sample_rate[] = {
	1,			/* AUDIO_CFG_96000 */
	2,			/* AUDIO_CFG_48000 */
	2,			/* AUDIO_CFG_44100 */
	3,			/* AUDIO_CFG_32000 */
	4,			/* AUDIO_CFG_22050 */
	6,			/* AUDIO_CFG_16000 */
	8,			/* AUDIO_CFG_11025 */
	12			/* AUDIO_CFG_8000 */
};

enum ceva_dsp_memspace {
	CEVA_HPI_INTERNAL_MEM = 0,
	CEVA_HPI_ZSI_MEM,
	CEVA_HPI_X_MEM,
	CEVA_HPI_Y_MEM,
	CEVA_HPI_Z_MEM,
	CEVA_HPI_P_WRITE_ONLY_MEM,
	CEVA_HPI_EXP_BUS_0_MEM,
	CEVA_HPI_EXP_BUS_1_MEM
};

static int dsp_audio_mixerdev = -1;
static int dsp_audio_dev = -1;

static int dsp_audio_speed = 44100;
static int audio_sampling_freq_id = -1;	/*used internally */
static int dsp_audio_channels = 2;
static int dsp_mixer_out_src;	/*selection of DAC1 or DAC2 */
static int dsp_mixer_rec_src;	/*selection of Stereo LINE, LINE Left, LINE Right */
static int audio_dac_id;	/*used internally */
static int dsp_pll_freq;	/*used internally */

static char audio_adc_vol_l = 70;
static char audio_adc_vol_r = 70;
static char audio_adc_vol_1 = 70;
static char audio_adc_vol_2 = 70;
static char audio_dac1_vol_l = 70;
static char audio_dac1_vol_r = 70;
static char audio_dac2_vol_l = 70;
static char audio_dac2_vol_r = 70;

static dma_addr_t dsp_rx_buf_phys;
static dma_addr_t dsp_tx_buf_phys;

static u8 *dsp_rx_buf_virt;
static u8 *dsp_tx_buf_virt;
static u32 dsp_rx_buf_count;
static u32 dsp_tx_buf_count;

static u32 dsp_fragment_size;
static u32 dsp_fragment_size_factor;
static u32 dsp_input_count_save;
static unsigned long dsp_input_buf_save;
static unsigned long dsp_output_buf_margin;

static struct clk *pll3_clk;

static int dsp_load_code_enable_clock(void);
static int dsp_audio_open(int dev, int mode);
static void dsp_audio_close(int dev);
static void
dsp_audio_output_block(int dev, unsigned long buf, int count, int intrflag);
static void
dsp_audio_start_input(int dev, unsigned long buf, int count, int intrflag);
static int dsp_audio_ioctl(int dev, unsigned int cmd, void __user * arg);
static int dsp_audio_prepare_for_output(int dev, int bufsize, int nbufs);
static int dsp_audio_prepare_for_input(int dev, int bufsize, int nbufs);
static void dsp_audio_halt_io(int dev);
static int dsp_audio_select_speed(int dev, int speed);
static unsigned int dsp_audio_set_bits(int dev, unsigned int bits);
static signed short dsp_audio_set_channels(int dev, signed short channels);
static void dsp_audio_set_speed(int open_mode, int speed, int dac2output);
static irqreturn_t dsp_audio_interrupt(int irq, void *data,
				       struct pt_regs *regs);

static int dsp_audio_remove(struct device *dev);

static inline void config_audio_adc(int sampling_freq_id)
{
	__raw_writel(audio_cfg_sample_rate[sampling_freq_id], ADC_SCK_DIV);	/* Set ADC SCK div 2, SCK = fout_256x48kHz/2 =  128x48kHz */
	__raw_writel(128, ADC_WS_DIV);	/* set DAC1 WS div to 128, WS = SCK/128 =  48kHz */
	__raw_writel(0, ADC_CTRL);
	__raw_writel(0, ADC_DEC_CTRL);

	SET_BITS(ADC_DEC_CTRL, (1 << 18)); /* Fix for record crakling noise */
	SET_BITS(ADC_DEC_CTRL, (1 << 19)); /* Fix for record crakling noise */


	SET_BITS(ADC_CTRL, (1 << 12));
	CLR_BITS(BTDMP_MUX_CTRL, (1 << 3));	/*Disables Loopback0 */
	SET_BITS(ADC_I2S_CTRL, (1 << 1));	/* Enables audio clock from PLL to ADC, WS and SCK */
	SET_BITS(ADC_I2S_CTRL, 1);	/* Enables clocking of I2S Transmitter */
}

static inline void config_audio_dac1(int sampling_freq_id)
{
	SET_BITS(DAC1_CTRL_DAC, (1 << 2));	/* enable 2 complement data   */
	SET_BITS(DAC1_CTRL_DAC, (1 << 5));	/* enable clock to 1/n divider from pll  */

	if(sampling_freq_id > AUDIO_CFG_44100)
	{
		SET_BITS(DAC1_CTRL_DAC, (1 << 3));	/* low sampling frequency mode, use 256*fs */
		SET_BITS(DAC1_CTRL_DAC, (1 << 4));	/* low sampling frequency mode, use 256*fs */
	}


	__raw_writel(audio_cfg_sample_rate[sampling_freq_id], DAC1_SCK_DIV);	/* Set DAC1 SCK div 2, SCK = fout_256x48kHz/2 =  128x48kHz */
	__raw_writel(128, DAC1_WS_DIV);	/* set DAC1 WS div to 128, WS = SCK/128 =  48kHz */

	/* The IC is not working as per spec. Phase Reversal is there. Set this bit to make it normal */
	SET_BITS(DAC1_CTRL_INTI, (1 << 26));
	SET_BITS(DAC1_I2S_CTRL, 1);	/* Enables audio clock and enables clocking of the I2S */
	SET_BITS(DAC1_I2S_CTRL, (1 << 3));	/* Sampled on positive clock edge. Also valid for DAC2 */
	__raw_writel((1 << 2), DAC2_I2S_CTRL);	/* Delayed I2S sync pulse. Disable clocking of DAC2 */

	CLR_BITS(BTDMP_MUX_CTRL, (1 << 11));
}

static inline void config_audio_dac2(int sampling_freq_id)
{
	SET_BITS(DAC2_CTRL_DAC, (1 << 2));	/* enable 2 complement data   */
	SET_BITS(DAC2_CTRL_DAC, (1 << 5));	/* enable clock to 1/n divider from pll  */

	if(sampling_freq_id > AUDIO_CFG_44100)
	{
		SET_BITS(DAC2_CTRL_DAC, (1 << 3));	/* low sampling frequency mode, use 256*fs */
		SET_BITS(DAC2_CTRL_DAC, (1 << 4));	/* low sampling frequency mode, use 256*fs */
	}

	__raw_writel(audio_cfg_sample_rate[sampling_freq_id], DAC2_SCK_DIV);	/* Set DAC1 SCK div 2, SCK = fout_256x48kHz/2 =  128x48kHz */
	__raw_writel(128, DAC2_WS_DIV);	/* set DAC1 WS div to 128, WS = SCK/128 =  48kHz */

	/* The IC is not working as per spec. Phase Reversal is there. Set this bit to make it normal */
	SET_BITS(DAC2_CTRL_INTI, (1 << 26));
	SET_BITS(DAC2_I2S_CTRL, 1);	/* Enables audio clock and enables clocking of the I2S */
	SET_BITS(DAC2_I2S_CTRL, (1 << 1));	/* Enables audio clock and enables clocking of the I2S */
	SET_BITS(DAC2_I2S_CTRL, (1 << 2));	/* Delayed I2S sync pulse. Also valid for DAC1 */
	__raw_writel((1 << 3), DAC1_I2S_CTRL);	/* Sampled on positive clock edge. Disable clocking of DAC1 */

	SET_BITS(BTDMP_MUX_CTRL, (1 << 11));
}

static inline void disable_audio_clock(void)
{
	audio_sampling_freq_id = -1;
	CLR_BITS(AUDIOPLL_CTRL, (1 << 21) | (1 << 5));	/* Power down Audio PLL */
}

static int switch_to_dirty_13mhz(u32 clk_reg)
{
	int i;

	/*if 13Mhz clock selected, select 13'MHz (dirty) source from OSC */
	if (!GET_BITS(clk_reg, 1)) {
		SET_BITS(clk_reg, (1 << 1));	/* Trigger switch to 13'MHz (dirty) clock */
		i = 10000;
		while ((!GET_BITS(clk_reg, 1)) && (i != 0)) {	/* Poll for clock switch to complete */
			i--;
		}
		if (i == 0) {
			return -1;
		}
	}

	return 0;
}

static int config_audio_clock(int open_mode, int sampling_freq_id,
			      int dac2output)
{
	unsigned long i;

	if (dac2output)
		i = 2;
	else
		i = 1;

	if ((audio_sampling_freq_id == sampling_freq_id) && (audio_dac_id == i))
		return 0;

	audio_dac_id = i;

	CLR_BITS(AUDIOPLL_CTRL, (1 << 21) | (1 << 5));	/* Power down Audio PLL */

	__raw_writel(0, AUDIO_DAC2DIV);	/* Stop DAC2 clock */
	__raw_writel(0, AUDIO_DAC1DIV);	/* Stop DAC1 clock */
	__raw_writel(0, AUDIO_ADCDIV);	/* Stop ADC clock */

	SET_BITS(AUDIOCLK_CTRL, (1 << 4) | (1 << 6));	/* Select AudioPLL as audio clock source for DAC and ADC */
	CLR_BITS(AUDIOCLK_CTRL, (1 << 2) | (1 << 3));	/* Select Audio PLL source as 13Mhz div 8 */
	SET_BITS(AUDIOCLK_CTRL, (1 << 5));	/* Enable Audio PLL clock source.(disable when not needed) */

	i = switch_to_dirty_13mhz(AUDIOCLK_CTRL);

	if (i < 0) {
		printk(KERN_ERR PKMOD
		       "failed to select dirty 13MHz clock for audio PLL\n");
		return i;
	}

	if (sampling_freq_id == AUDIO_CFG_44100 ||
	    sampling_freq_id == AUDIO_CFG_22050
	    || sampling_freq_id == AUDIO_CFG_11025) {
		__raw_writel(0x0B3E1B1B, AUDIOPLL_FB);	/* Set the Audio PLL feedback dividers */
		__raw_writel(0x0040B1FE, AUDIOPLL_CTRL);	/* Set the Audio PLL pre and post dividers */
	}

	else {
		__raw_writel(0x183E126D, AUDIOPLL_FB);	/* Set the Audio PLL feedback dividers */
		__raw_writel(0x00403E04, AUDIOPLL_CTRL);	/* Set the Audio PLL pre and post dividers */
	}

	if (open_mode & OPEN_WRITE) {
		if (dac2output)
			config_audio_dac2(sampling_freq_id);
		else
			config_audio_dac1(sampling_freq_id);
		__raw_writel(1, AUDIO_DAC1DIV);	/* Set DAC1, dividers to nominal values */
		__raw_writel(1, AUDIO_DAC2DIV);	/* Set DAC2, dividers to nominal values */
	}

	if (open_mode & OPEN_READ) {
		config_audio_adc(sampling_freq_id);
		__raw_writel(1, AUDIO_ADCDIV);	/* Set ADC, dividers to nominal values */
	}

	SET_BITS(AUDIOPLL_CTRL, (1 << 21));	/* Power up Audio PLL */

	i = 10000;

	while ((GET_BITS(AUDIOPLL_CTRL, 1) == 0) && (i != 0)) {	/* Poll for PLL lock                */
		i--;
	}

	if (i == 0) {
		printk(KERN_ERR PKMOD "failed to lock audio PLL\n");
		return -1;
	}

	audio_sampling_freq_id = sampling_freq_id;

	return 0;
}

static void send_dsp_command(int cmd)
{
	unsigned int t;
	int i = 0;

	while (1) {
		t = __raw_readw(DSP_PSTS);
		if ((t & 0x2000) == 0)
			break;

		if (i == 10000) {
			/*formerly wait_COM0_empty FAILED */
			printk(KERN_ERR PKMOD
			       "failed to send a DSP command 0x%x\n", cmd);
			return;
		}
		i++;
	}

	__raw_writew(cmd, DSP_APBP_COM0);
}

static inline void set_playback_freq(unsigned int sampling_freq)
{
	send_dsp_command(PLAY_SAMPLING_FREQ_CMD);
	send_dsp_command((sampling_freq >> 0x10) & 0xFFFF);
	send_dsp_command(sampling_freq & 0xFFFF);
}

static inline void set_record_freq(unsigned int sampling_freq)
{
	send_dsp_command(RECORD_SAMPLING_FREQ_CMD);
	send_dsp_command((sampling_freq >> 0x10) & 0xFFFF);
	send_dsp_command(sampling_freq & 0xFFFF);
}

static inline void set_playback_base_addr(unsigned int base_add)
{
	/* command for physical start address of host side buffer */
	send_dsp_command(SET_SRC_BASE_ADD);
	send_dsp_command((base_add >> 0x10) & 0xffff);
	send_dsp_command(base_add & 0xffff);
}

static inline void set_playback_size(unsigned int size)
{
	/* command for size of host side buffer */
	send_dsp_command(SET_SRC_SIZE_IN_BYTES);
	send_dsp_command((size >> 0x10) & 0xffff);
	send_dsp_command(size & 0xffff);
}

static inline void set_record_base_addr(unsigned int base_add)
{
	send_dsp_command(SET_SRC_BASE_ADD_RECORD);
	send_dsp_command((base_add >> 0x10) & 0xffff);
	send_dsp_command(base_add & 0xffff);
}

static inline void set_record_size(unsigned int size)
{
	send_dsp_command(SET_SRC_SIZE_IN_BYTES_RECORD);
	send_dsp_command((size >> 0x10) & 0xffff);
	send_dsp_command(size & 0xffff);
}

static inline u16 read_rx_buf_count(void)
{
	return __raw_readw(DSP_APBP_REP0) & 0xFFFC;
}

static inline u16 read_tx_buf_count(void)
{
	return __raw_readw(DSP_APBP_REP1) & 0xFFFE;
}

#define DSP_TX_INT_READY DSP_PSTS_RRI1
#define DSP_RX_INT_READY DSP_PSTS_RRI

static inline u16 read_int_status(void)
{
	return __raw_readw(DSP_PSTS);
}

#define CEVA_HPI_DEFAULT_TIMEOUT	1000000	/* processor dependant */
/*****************************************************************************************
 * ceva_hpi_write_buffer()
 *
 * description: Writes "Size" 16 bit integer words from host start address "pSrc"
 *              to DSP start address "destAddr" in DSP memory space "memsel".
 *              The HPI is set up for auto-increment mode, and the host checks
 *              each time for the FIFO write buffer being full. It also performs a
 *              (processor dependant) timeout check.
 *
 * parameters:
 *   - pSrc[IN]: is a pointer to the start of the buffer to be written to the DSP.
 *   - Size[IN]: is the number of 16 bit integer words to write to the DSP
 *   - destAddr[IN]: is the DSP start destination address where the data is to be written to
 *   - memspace[IN]: is the DSP destination memory space type e.g X/Y/Z etc...
 *
 * returns: 0 if all words written OK, else returns a non-zero error
 *          code to flag an error condition.
 *
 *******************************************************************************************/
static int ceva_hpi_write_buffer(short int *pSrc, unsigned int Size,
				 short int destAddr,
				 enum ceva_dsp_memspace memSpace)
{
	long n;
	signed long timeoutCnt = CEVA_HPI_DEFAULT_TIMEOUT;	/* setup timeout count */
	short unsigned int oldCfg;

	if (Size > 8)
		return -EFAULT;

	oldCfg = __raw_readw(DSP_PCFG);	/* save old HPI_CFG word */

	/* set up auto inc mode */
	SET_BITS_W(DSP_PCFG, DSP_PCFG_AIM);	/* Select AudioPLL as audio clock source  */

	/* set up DSP memory space to write to */
	__raw_writew(((__raw_readw(DSP_PCFG) & ~(DSP_PCFG_MEMSEL_MASK)) |
		      (memSpace << DSP_PCFG_MEMSEL_SHIFT)), DSP_PCFG);

	/* write dest start address to register */
	__raw_writew(destAddr, DSP_PADR);

	for (n = 0; n < Size; n++) {
		while (1) {	/*loop to write data or time out */
			/* read FIFO status bit from HSTS reg */
			if (!(GET_BITS_W(DSP_PSTS, DSP_PSTS_WFFI))) {
				/* write FIFO not full so write next word */
				/* write next word to HDATA register, and auto inc next write address */
				__raw_writew((short)*pSrc++, DSP_PDATA);
				/* reset timeout count to max */
				timeoutCnt = CEVA_HPI_DEFAULT_TIMEOUT;
				break;
			} else {	/* write FIFO IS full */

				if (--timeoutCnt <= 0) {
					/* restore previous mode on exit for HPI_CFG word */
					__raw_writew(oldCfg, DSP_PCFG);
					return -ETIME;
				}
			}
		}
	}

	/*
	 * have written all data to FIFO...
	 * now wait until the write FIFO is empty so that when fuunction returns
	 * all the data has been actually written to the DSP memory
	 */

	timeoutCnt = CEVA_HPI_DEFAULT_TIMEOUT;
	while (1) {
		/* get FIFO empty status */
		if (GET_BITS_W(DSP_PSTS, DSP_PSTS_WFEI))
			break;	/* its empty, get me out of here....... */
		/* decrement timeout count - if zero then flag a write timeout error and return */
		if (--timeoutCnt <= 0) {
			/* restore previous mode on exit for HPI_CFG word */
			__raw_writew(oldCfg, DSP_PCFG);
			return -ETIME;
		}
	}

	/* restore previous mode on exit for HPI_CFG word */
	__raw_writew(oldCfg, DSP_PCFG);

	/* disable up auto inc mode on exit */
	CLR_BITS_W(DSP_PCFG, DSP_PCFG_AIM);
	return 0;
}

char dsp_fname[] = CONFIG_SOUND_PNX4008_DSP_BIN_NAME;

static inline int read_dsp_pll_freq(struct file *dsp_file)
{
	unsigned long data;
	unsigned long offset;
	int err_num;
	offset = 0;

	err_num = kernel_read(dsp_file, offset, (char *)&data, 4);

	if (err_num < 0) {
		printk(KERN_ERR PKMOD
		       "failed to read DSP code from %s, offset %ld\n",
		       dsp_fname, offset);
		return err_num;
	}
	return data;
}

static inline int download_dsp_image(struct file *dsp_file)
{
	struct dsp_data_rec data;
	unsigned long offset;
	int err_num;
	offset = 4;
	err_num = 0;

	__raw_writew(1, DSP_PCFG);	/*put DSP into reset */

	/* download image */
	do {
		err_num =
		    kernel_read(dsp_file, offset, (char *)&data,
				sizeof(struct dsp_data_rec));

		if (err_num < 0) {
			printk(KERN_ERR PKMOD
			       "failed to read DSP code from %s, offset %ld\n",
			       dsp_fname, offset);
			goto out;
		}

		offset += sizeof(struct dsp_data_rec);

		if ((err_num = ceva_hpi_write_buffer(data.dat, data.size,
						     data.addr,
						     CEVA_HPI_P_WRITE_ONLY_MEM))
		    < 0) {
			printk(KERN_ERR PKMOD
			       "failed to program DSP code, addr: %x\n",
			       data.addr);
			goto out;
		}

	} while (data.size != 0);

	do {
		err_num =
		    kernel_read(dsp_file, offset, (char *)&data,
				sizeof(struct dsp_data_rec));
		if (err_num < 0) {
			printk(KERN_ERR PKMOD
			       "failed to read DSP data from %s, offset %ld\n",
			       dsp_fname, offset);
			goto out;
		}

		offset += sizeof(struct dsp_data_rec);

		if ((err_num = ceva_hpi_write_buffer(data.dat, data.size,
						     data.addr,
						     CEVA_HPI_INTERNAL_MEM)) <
		    0) {
			printk(KERN_ERR PKMOD
			       "failed to program DSP data, addr: %x\n",
			       data.addr);
			goto out;
		}
	} while (data.size != 0);

      out:
	__raw_writew(0, DSP_PCFG);	/*take DSP out of reset */

	return err_num;
}

static void enable_dsp_interrupt(void)
{
	read_rx_buf_count();
	read_tx_buf_count();
	SET_BITS_W(DSP_PCFG, DSP_PCFG_RRIE0);
	SET_BITS_W(DSP_PCFG, DSP_PCFG_RRIE1);	/* Enable REP0 and REP1 interrupts */
}

static void disable_dsp_interrupt(void)
{
	CLR_BITS_W(DSP_PCFG, DSP_PCFG_RRIE0);
	CLR_BITS_W(DSP_PCFG, DSP_PCFG_RRIE1);	/* Disable REP0 and REP1 interrupts */
}

static inline void disable_dsp_clock(void)
{
	CLR_BITS(DSPCLK_CTRL, (1 << 4) | (1 << 11));
	clk_disable(pll3_clk);
}

static int enable_dsp_clock(void)
{
	int ret;
	SET_BITS(DSPCLK_CTRL, (1 << 6));	/*select PLL3 as source */
	ret = clk_set_rate(pll3_clk, dsp_pll_freq);
	if (ret < 0)
		return ret;
	ret = clk_enable(pll3_clk);
	if (ret < 0)
		return ret;
	SET_BITS(DSPCLK_CTRL, (1 << 4) | (1 << 11));
	return 0;
}

static inline int prepare_dsp_for_download(void)
{
	int x;
	/* clr intp -> boot from external DSP memory */
	/* mask out PBUSY, this signal may be undefined during a short period after reset */
	CLR_BITS(DSPCLK_CTRL, (1 << 13) | (1 << 14));

	/* Assert reset to dsp and to dsp vpb (preset_n signal) */
	CLR_BITS(DSPCLK_CTRL, (1 << 5) | (1 << 12));

	udelay(10);

	/* De-assert preset_n and rst_n */
	SET_BITS(DSPCLK_CTRL, (1 << 5) | (1 << 12));

	udelay(10);

	/* Enable PBUSY */
	SET_BITS(DSPCLK_CTRL, (1 << 14));

	/* Enable APBP reset and target = PRAM */
	__raw_writew(DSP_PCFG_MEMSEL_P_WRITE_ONLY_MEM | DSP_PCFG_DSPR |
		     DSP_PCFG_AIM, DSP_PCFG);

	/* Poll until all DSP peripherals are reset */
	x = 10000;

	while ((GET_BITS_W(DSP_PSTS, (1 << 2)) == (1 << 2)) && (x != 0)) {
		x--;
	}

	if (x == 0) {
		printk(KERN_ERR PKMOD "failed to assert APBP reset in DSP\n");
		return -ETIME;
	}

	__raw_writew(0x0000, DSP_PADR);	/* Download to address 0x0000 */

	/* Write DSP OP codes directly into program RAM */
	/* the following code should wait for write FIFO not full flag !!!! */
	__raw_writew(0x438C, DSP_PDATA);	/* rst    ##0x0100,mod0                        */
	__raw_writew(0x0100, DSP_PDATA);
	__raw_writew(0x438C, DSP_PDATA);	/* rst    ##0x0200,mod0                        */
	__raw_writew(0x0200, DSP_PDATA);
	__raw_writew(0x0480, DSP_PDATA);	/* load   #(APBP_BASE_PG),page                 */
	__raw_writew(0x5E06, DSP_PDATA);	/* mov    ##0x0001, r7                         */
	__raw_writew(0x0001, DSP_PDATA);
	__raw_writew(0x2CCC, DSP_PDATA);	/* mov    r7, APBP_APBP_SEM_S                  */
	__raw_writew(0x0000, DSP_PDATA);	/* nop                                         */
	__raw_writew(0x4180, DSP_PDATA);	/* br     0x0000                               */
	__raw_writew(0x0000, DSP_PDATA);

	/* Poll until all data is written */
	x = 10000;

	while ((GET_BITS_W(DSP_PSTS, (1 << 1)) == (1 << 1)) && (x != 0)) {
		x--;
	}

	if (x == 0) {
		printk(KERN_ERR PKMOD "failed to program DSP opcode\n");
		return -ETIME;
	}

	__raw_writew(DSP_PCFG_MEMSEL_P_WRITE_ONLY_MEM, DSP_PCFG);	/* De-assert APBP reset */

	/* Poll until DSP is ready to go into All-Stop-Mode */
	x = 10000;

	while (1) {
		/* written this way to provide visibility */
		int i = GET_BITS_W(DSP_APBP_SEM, 1);

		if (i == 1)
			break;
		x--;

		if (x == 0) {
			printk(KERN_ERR PKMOD
			       "failed to deassert APBP reset in DSP\n");
			return -ETIME;
		}
	}

	/*
	 * The following code enable chaining on DMA channel 0 of the Teak
	 * DMA channel 0 is used by the APBP to read/write from/to DSP memory,
	 * as chaining is not enabled, the DMA channel stops after 2^16 transfers....
	 */

	/* code should select DMA channel0, assuming all is OK as the code sequence able resets the DSP...... */
	__raw_writew(0x81DC, DSP_PADR);	/* Teak's DMA channel 0 configure 1 register */
	__raw_writew(0, DSP_PCFG);	/* reset for transfer */
	__raw_writew(DSP_PCFG_MEMSEL_ZSI_MEM, DSP_PCFG);	/* set MMIO space */
	__raw_writew(0x10, DSP_PDATA);	/* setting channel mode enable from channel zero */

}
static int config_dsp(void)
{
	int ret;
	struct file *dsp_file = NULL;

	dsp_file = filp_open(dsp_fname, O_RDONLY, 00);

	if (IS_ERR(dsp_file)) {
		printk(KERN_ERR PKMOD
		       "failed to open %s to read DSP code\n", dsp_fname);
		return PTR_ERR(dsp_file);
	}

	ret = read_dsp_pll_freq(dsp_file);
	if (ret < 0) {
		filp_close(dsp_file, current->files);
		return ret;
	}
	dsp_pll_freq = ret;

	disable_dsp_interrupt();
	disable_dsp_clock();
	ret = enable_dsp_clock();
	if (ret < 0) {
		filp_close(dsp_file, current->files);
		return ret;
	}

	ret = prepare_dsp_for_download();
	if (ret < 0) {
		disable_dsp_clock();
		filp_close(dsp_file, current->files);
		return ret;
	}

	ret = download_dsp_image(dsp_file);

	filp_close(dsp_file, current->files);

	if (ret < 0) {
		disable_dsp_clock();
		return ret;
	}

	mdelay(4);		/* may fail to run first time without this delay */
	send_dsp_command(STOP_CMD);
	return 0;
}

static void adc_amp_enable(int enable)
{
	if (test_bit(SPI2_REGION_INITED, &dsp_audio_status)) {
		/* Enable/Disable Power to MIC amp thru VDDADC28 and VDDADC12.
		   This will make SPI2 unusable. Requires ADC_POW jumper installed */
		if (enable) {
			SET_BITS(SPI_CTRL, (1 << 4));
			CLR_BITS(SPI_CTRL, (1 << 5));
			SET_BITS(SPI_CTRL, (1 << 6));
		} else {
			CLR_BITS(SPI_CTRL, (1 << 6));
		}
	}
	/* Enables/disables MIC amplifier */
	if (test_bit(GPO_14_INITED, &dsp_audio_status))
		pnx4008_gpio_write_pin(GPO_14, enable);
}

static void dac1_amp_enable(int enable)
{
	if (test_bit(GPO_15_INITED, &dsp_audio_status))
		pnx4008_gpio_write_pin(GPO_15, enable);
}

static void dac2_amp_enable(int enable)
{
	if (test_bit(GPO_16_INITED, &dsp_audio_status))
		pnx4008_gpio_write_pin(GPO_16, enable);
}

static void set_audio_adc_vol(unsigned char vol_l, unsigned char vol_r)
{
	u16 tmp_left = ((u16) vol_l * 0xff) / MAX_VOLUME;
	u16 tmp_right = ((u16) vol_r * 0xff) / MAX_VOLUME;

#ifdef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	adc_amp_enable(vol_l || vol_r);
#endif

	tmp_left = ((tmp_left - 0x80) << 8) & 0xff00;
	tmp_right = (tmp_right - 0x80) & 0xff;

	/*
	 * TODO: This is blindly setting the DB as zero. The idea here is we
	 * should set the gain as zero.
	 * So need to calculate the default value of audio_adc_vol_l and
	 * audio_adc_vol_r.
	 */
	__raw_writel(ADC_DEC_CTRL,0xffff0000);

}

static void set_audio_dac1_vol(unsigned char vol_l, unsigned char vol_r)
{
	u16 tmp_left = ((u16) (MAX_VOLUME - vol_l) * 0xfc) / MAX_VOLUME;
	u16 tmp_right = ((u16) (MAX_VOLUME - vol_r) * 0xfc) / MAX_VOLUME;

#ifdef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	dac1_amp_enable(vol_l || vol_r);
#endif
	tmp_left = (tmp_left << 8) & 0xff00;
	tmp_right = tmp_right & 0xff;

	__raw_writel((__raw_readl(DAC1_CTRL_INTI) & 0xffff0000) | tmp_left |
		     tmp_right, DAC1_CTRL_INTI);
}

static void set_audio_dac2_vol(unsigned char vol_l, unsigned char vol_r)
{
	u16 tmp_left = ((u16) (MAX_VOLUME - vol_l) * 0xfc) / MAX_VOLUME;
	u16 tmp_right = ((u16) (MAX_VOLUME - vol_r) * 0xfc) / MAX_VOLUME;

#ifdef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	dac2_amp_enable(vol_l || vol_r);
#endif

	tmp_left = (tmp_left << 8) & 0xff00;
	tmp_right = tmp_right & 0xff;

	__raw_writel((__raw_readl(DAC2_CTRL_INTI) & 0xffff0000) | tmp_left |
		     tmp_right, DAC2_CTRL_INTI);
}

static void adjust_rec_volume(void)
{
	clear_bit(AUDIO_LINE1_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_LINE2_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_LINE_SELECTED, &dsp_audio_status);

	if (dsp_mixer_rec_src & SOUND_MASK_LINE1) {
		set_audio_adc_vol(audio_adc_vol_1, 0);
		set_bit(AUDIO_LINE1_SELECTED, &dsp_audio_status);
	} else if (dsp_mixer_rec_src & SOUND_MASK_LINE2) {
		set_audio_adc_vol(0, audio_adc_vol_2);
		set_bit(AUDIO_LINE2_SELECTED, &dsp_audio_status);
	} else {
		set_audio_adc_vol(audio_adc_vol_l, audio_adc_vol_r);
		set_bit(AUDIO_LINE_SELECTED, &dsp_audio_status);
	}
}

static void adjust_play_volume(void)
{
	clear_bit(AUDIO_DAC2_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_DAC1_SELECTED, &dsp_audio_status);

	if (dsp_mixer_out_src & SOUND_MASK_ALTPCM) {
		set_audio_dac2_vol(audio_dac2_vol_l, audio_dac2_vol_r);
		set_audio_dac1_vol(0, 0);
		set_bit(AUDIO_DAC2_SELECTED, &dsp_audio_status);
	} else {
		set_audio_dac1_vol(audio_dac1_vol_l, audio_dac1_vol_r);
		set_audio_dac2_vol(0, 0);
		set_bit(AUDIO_DAC1_SELECTED, &dsp_audio_status);
	}
}

#ifdef CONFIG_PM

static DECLARE_WAIT_QUEUE_HEAD(dsp_audio_wait);

static inline u32 dsp_audio_timeout(struct dma_buffparms *dmap)
{
	/*wait approx. 2 fragments */
	return ((u32) dmap->fragment_size * HZ) /
	    ((u32) dsp_audio_channels * (u32) dsp_audio_speed);
}

static inline void dsp_audio_wait_for_end_of_play(void)
{
	struct audio_operations *adev = audio_devs[dsp_audio_dev];
	u32 timeout = 0;
	u32 timeout1 = 0;

	if (adev->open_mode & OPEN_WRITE) {
		timeout = dsp_audio_timeout(adev->dmap_out);
	}

	if (adev->open_mode & OPEN_READ) {
		timeout1 = dsp_audio_timeout(adev->dmap_in);
	}

	if (timeout1 > timeout)
		timeout = timeout1;

	sleep_on_timeout(&dsp_audio_wait, timeout);
}

static inline void dsp_audio_do_suspend(void)
{
	set_bit(AUDIO_POWER_OFF, &dsp_audio_status);

	if (!test_bit(AUDIO_OPENED, &dsp_audio_status))
		return;

	audio_devs[dsp_audio_dev]->go = 0;
	dsp_audio_wait_for_end_of_play();

	if (test_bit(AUDIO_RUNNING, &dsp_audio_status))
		dsp_audio_halt_io(dsp_audio_dev);

	set_audio_dac1_vol(0, 0);
	set_audio_dac2_vol(0, 0);
	set_audio_adc_vol(0, 0);
	clear_bit(AUDIO_DAC2_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_DAC1_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_LINE_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_LINE1_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_LINE2_SELECTED, &dsp_audio_status);
	disable_audio_clock();
	disable_dsp_clock();
#ifndef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	adc_amp_enable(0);
	dac1_amp_enable(0);
	dac2_amp_enable(0);
#endif
}

static int dsp_audio_suspend(struct device *dev, u32 state, u32 level)
{
	switch (level) {
	case SUSPEND_DISABLE:
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		dsp_audio_do_suspend();
		break;
	}

	return 0;
}

static inline void dsp_audio_do_resume(void)
{
	clear_bit(AUDIO_POWER_OFF, &dsp_audio_status);

	if (!test_bit(AUDIO_OPENED, &dsp_audio_status))
		return;

#ifndef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	adc_amp_enable(1);
	dac1_amp_enable(1);
	dac2_amp_enable(1);
#endif
	enable_dsp_clock();
	audio_devs[dsp_audio_dev]->go = 1;

	if (audio_devs[dsp_audio_dev]->open_mode & OPEN_WRITE) {
		DMAbuf_outputintr(dsp_audio_dev, 1);
	}

	if (audio_devs[dsp_audio_dev]->open_mode & OPEN_READ) {
		DMAbuf_inputintr(dsp_audio_dev);
	}
}

static int dsp_audio_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_POWER_ON:
		dsp_audio_do_resume();
		break;
	case RESUME_RESTORE_STATE:
		break;
	case RESUME_ENABLE:
		break;
	}
	return 0;
}
#endif

static struct audio_driver dsp_audio_driver = {
	.owner = THIS_MODULE,
	.open = dsp_audio_open,
	.close = dsp_audio_close,
	.output_block = dsp_audio_output_block,
	.start_input = dsp_audio_start_input,
	.ioctl = dsp_audio_ioctl,
	.prepare_for_input = dsp_audio_prepare_for_input,
	.prepare_for_output = dsp_audio_prepare_for_output,
	.halt_io = dsp_audio_halt_io,
	.set_speed = dsp_audio_select_speed,
	.set_bits = dsp_audio_set_bits,
	.set_channels = dsp_audio_set_channels,
};

static int dsp_load_code_enable_clock(void)
{
	int tmp;
	if (!test_bit(DSP_CODE_LOADED, &dsp_audio_status)) {

		tmp = config_dsp();
		if (tmp < 0) {
			/*error messages will come from inside */
			return tmp;
		}

		/*interrupts may be flooded during DSP initialization */
		tmp = request_irq(DSP_P_INT, dsp_audio_interrupt,
				  0, MODULE_NAME, &dsp_audio_driver);
		if (tmp < 0) {
			printk(KERN_ERR PKMOD "failed to register irq\n");
			disable_dsp_clock();
			return tmp;
		}
		set_bit(DSP_IRQ_INITED, &dsp_audio_status);

		set_bit(DSP_CODE_LOADED, &dsp_audio_status);
	}
	return 0;
}

/*start the device*/
static int dsp_audio_open(int dev, int mode)
{
	/*power state must be ON */
	if (test_bit(AUDIO_POWER_OFF, &dsp_audio_status))
		return -EPERM;

	/* only one open allowed */
	if (test_and_set_bit(AUDIO_OPENED, &dsp_audio_status))
		return -EBUSY;

	if (dsp_load_code_enable_clock() < 0) {
		clear_bit(AUDIO_OPENED, &dsp_audio_status);
		return -EFAULT;
	}
#ifdef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	enable_dsp_clock();
#endif

	return 0;
}

/*stop the device*/
static void dsp_audio_close(int dev)
{
	if (test_bit(AUDIO_RUNNING, &dsp_audio_status))
		dsp_audio_halt_io(dev);

#ifdef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	disable_audio_clock();
	disable_dsp_clock();
#endif

	clear_bit(AUDIO_OPENED, &dsp_audio_status);
}

static void dsp_audio_trigger(void)
{
	int open_mode;

	if (test_and_set_bit(AUDIO_RUNNING, &dsp_audio_status))
		return;

	send_dsp_command(STOP_CMD);

	open_mode = audio_devs[dsp_audio_dev]->open_mode;

	dsp_audio_set_speed(open_mode, dsp_audio_speed,
			    (dsp_mixer_out_src & SOUND_MASK_ALTPCM));

	if (open_mode & OPEN_WRITE) {
		set_playback_base_addr(dsp_tx_buf_phys);
		set_playback_size(DSP_TX_BUF_SIZE);
	}

	if (open_mode & OPEN_READ) {
		set_record_base_addr(dsp_rx_buf_phys);
		set_record_size(DSP_RX_BUF_SIZE);
	}

	enable_dsp_interrupt();

	if ((open_mode & OPEN_WRITE) && (open_mode & OPEN_READ)) {
		send_dsp_command(PLAY_RECORD_CMD);
	} else {

		if (open_mode & OPEN_WRITE) {
			send_dsp_command(PLAY_CMD);
		}

		if (open_mode & OPEN_READ) {
			send_dsp_command(RECORD_CMD);
		}
	}
	mdelay(1); /* This is required for the DSP to get ready */
}

static inline void write_stereo_sound(s16 * src_addr, s16 * addr, int count)
{
	int i;
	for (i = 0; i < count / 2; i++) {
		*addr++ = *src_addr++;
		dsp_tx_buf_count += 2;
		if (dsp_tx_buf_count >= DSP_TX_BUF_SIZE) {
			addr -= DSP_TX_BUF_SIZE / 2;
			dsp_tx_buf_count -= DSP_TX_BUF_SIZE;
		}
	}
}

static inline void write_mono_sound(s16 * src_addr, s16 * addr, int count)
{
	int i;
	for (i = 0; i < count / 2; i++) {
		*addr++ = *src_addr;
		dsp_tx_buf_count += 2;
		if (dsp_tx_buf_count >= DSP_TX_BUF_SIZE) {
			addr -= DSP_TX_BUF_SIZE / 2;
			dsp_tx_buf_count -= DSP_TX_BUF_SIZE;
		}
		*addr++ = *src_addr;
		dsp_tx_buf_count += 2;
		if (dsp_tx_buf_count >= DSP_TX_BUF_SIZE) {
			addr -= DSP_TX_BUF_SIZE / 2;
			dsp_tx_buf_count -= DSP_TX_BUF_SIZE;
		}
		src_addr++;
	}
}

/*this function is a part of output dma buffer processing*/
static void
dsp_audio_output_block(int dev, unsigned long buf, int count, int intrflag)
{
	s16 *addr;
	s16 *src_addr;
	u32 tmp_size;

	if (!test_bit(AUDIO_TX_INITED, &dsp_audio_status)
	    && test_bit(AUDIO_RUNNING, &dsp_audio_status)) {
		dsp_tx_buf_count = read_tx_buf_count();
		tmp_size = dsp_fragment_size * 2;
		if (tmp_size < count)
			tmp_size = count;
		dsp_tx_buf_count =
		    (dsp_tx_buf_count + tmp_size) & (DSP_TX_BUF_SIZE - 1);
	}

	addr = (u16 *) (dsp_tx_buf_virt + dsp_tx_buf_count);
	src_addr = (u16 *) phys_to_virt(buf);

	if (dsp_audio_channels == 2) {
		write_stereo_sound(src_addr, addr, count);
		dsp_output_buf_margin = count / 2;
	} else {
		write_mono_sound(src_addr, addr, count);
		dsp_output_buf_margin = count;
	}

	if (dsp_output_buf_margin < dsp_fragment_size)
		dsp_output_buf_margin = dsp_fragment_size;

	if (!test_and_set_bit(AUDIO_TX_INITED, &dsp_audio_status)) {
		dsp_audio_trigger();
		adjust_play_volume();
	}

}

/*this function is a part of input dma buffer processing*/
static void
dsp_audio_start_input(int dev, unsigned long buf, int count, int intrflag)
{
	if (!test_and_set_bit(AUDIO_RX_INITED, &dsp_audio_status)) {
		dsp_audio_trigger();
		dsp_rx_buf_count = read_rx_buf_count();
		adjust_rec_volume();
	}
	if (dsp_audio_channels == 2)
		dsp_input_count_save = count;
	else
		dsp_input_count_save = count * 2;
	dsp_input_buf_save = buf;
}

static int dsp_audio_ioctl(int dev, unsigned int cmd, void __user * arg)
{
	int val, ret = 0;

	switch (cmd) {
	case SOUND_PCM_WRITE_RATE:
		if (get_user(val, (int *)arg))
			return -EFAULT;
		ret = dsp_audio_select_speed(dev, val);
		break;
	case SOUND_PCM_READ_RATE:
		ret = dsp_audio_speed;
		break;
	case SOUND_PCM_WRITE_CHANNELS:
		if (get_user(val, (int *)arg))
			return -EFAULT;
		ret = dsp_audio_set_channels(dev, val);
		break;
	case SOUND_PCM_READ_CHANNELS:
		ret = dsp_audio_channels;
		break;
	case SNDCTL_DSP_SETFMT:
		if (get_user(val, (int *)arg))
			return -EFAULT;
		ret = dsp_audio_set_bits(dev, val);
		break;
	case SNDCTL_DSP_GETFMTS:
	case SOUND_PCM_READ_BITS:
		ret = AFMT_S16_LE;
		break;
	default:
		return -EINVAL;
	}
	return put_user(ret, (int *)arg);
}

/*this function is a part of output dma buffer processing*/
static int dsp_audio_prepare_for_output(int dev, int bufsize, int nbufs)
{
	audio_devs[dev]->dmap_out->flags |= DMA_NODMA;
	return 0;
}

/*this function is a part of input dma buffer processing*/
static int dsp_audio_prepare_for_input(int dev, int bufsize, int nbufs)
{
	audio_devs[dev]->dmap_in->flags |= DMA_NODMA;
	return 0;
}

/*restart the device*/
static void dsp_audio_halt_io(int dev)
{

	send_dsp_command(STOP_CMD);

#ifdef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	set_audio_dac1_vol(0, 0);
	set_audio_dac2_vol(0, 0);
	set_audio_adc_vol(0, 0);
#endif

	disable_dsp_interrupt();

	clear_bit(AUDIO_RUNNING, &dsp_audio_status);
	clear_bit(AUDIO_TX_INITED, &dsp_audio_status);
	clear_bit(AUDIO_RX_INITED, &dsp_audio_status);
	clear_bit(AUDIO_TX_ZERO_DATA, &dsp_audio_status);

#ifdef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	clear_bit(AUDIO_DAC2_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_DAC1_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_LINE_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_LINE1_SELECTED, &dsp_audio_status);
	clear_bit(AUDIO_LINE2_SELECTED, &dsp_audio_status);
#endif

	memset(dsp_tx_buf_virt, 0x00, DSP_TX_BUF_SIZE);

	dsp_tx_buf_count = 0;
	dsp_rx_buf_count = 0;
}

/* actually set sampling rate */
static void dsp_audio_set_speed(int open_mode, int speed, int dac2output)
{
	switch (speed) {
	case 8000:
		set_playback_freq(speed);
		set_record_freq(speed);
		config_audio_clock(open_mode, AUDIO_CFG_8000, dac2output);
		break;
	case 11025:
		set_playback_freq(speed);
		set_record_freq(speed);
		config_audio_clock(open_mode, AUDIO_CFG_11025, dac2output);
		break;
	case 16000:
		set_playback_freq(speed);
		set_record_freq(speed);
		config_audio_clock(open_mode, AUDIO_CFG_16000, dac2output);
		break;
	case 22050:
		set_playback_freq(speed);
		set_record_freq(speed);
		config_audio_clock(open_mode, AUDIO_CFG_22050, dac2output);
		break;
	case 32000:
		set_playback_freq(speed);
		set_record_freq(speed);
		config_audio_clock(open_mode, AUDIO_CFG_32000, dac2output);
		break;
	case 44100:
		set_playback_freq(speed);
		set_record_freq(speed);
		config_audio_clock(open_mode, AUDIO_CFG_44100, dac2output);
		break;
	case 48000:
		set_playback_freq(speed);
		set_record_freq(speed);
		config_audio_clock(open_mode, AUDIO_CFG_48000, dac2output);
		break;
	}
}

static inline void dsp_audio_update_fragment_size(int dev)
{
	audio_devs[dev]->min_fragment =
	    dsp_fragment_size_factor - 2 + dsp_audio_channels;

	/* Keep the fragment size as minimal for 8Khz, otherwise long loop back delay */
	audio_devs[dev]->max_fragment =
	    dsp_fragment_size_factor - 2 + dsp_audio_channels;

	audio_devs[dev]->dmap_in->needs_reorg = 1;
	audio_devs[dev]->dmap_out->needs_reorg = 1;
}

/* interface configuring sampling rate */
static int dsp_audio_select_speed(int dev, int speed)
{
	switch (speed) {
	case 8000:
		dsp_fragment_size_factor = 9;
		break;
	case 11025:
	case 16000:
	case 22050:
	case 32000:
	case 44100:
	case 48000:
		dsp_fragment_size_factor = 11;
		break;
	case 0:
		return dsp_audio_speed;
	default:
		return -EINVAL;
	}
	dsp_fragment_size = 1 << dsp_fragment_size_factor;
	dsp_audio_speed = speed;
	dsp_audio_update_fragment_size(dev);
	return dsp_audio_speed;
}

/* set number of bits, only 16 supported*/
static unsigned int dsp_audio_set_bits(int dev, unsigned int bits)
{
	switch (bits) {
	case 0:
	case AFMT_S16_LE:
		return AFMT_S16_LE;
	default:
		return -EINVAL;
	}
}

/* set number of channels*/
static signed short dsp_audio_set_channels(int dev, signed short channels)
{
	switch (channels) {
	case 1:		/*mono */
	case 2:		/*stereo */
		dsp_audio_channels = channels;
		dsp_audio_update_fragment_size(dev);
	case 0:
		return dsp_audio_channels;
	default:
		return -EINVAL;
	}
}

static void audio_volume_check(int user_vol, unsigned char *left,
			       unsigned char *right)
{
	if (right) {
		*right = (user_vol >> 8) & 0xff;
		if (*right > MAX_VOLUME)
			*right = MAX_VOLUME;
	}
	*left = user_vol & 0xff;
	if (*left > MAX_VOLUME)
		*left = MAX_VOLUME;
}

static int audio_mixer_ioctl(int dev, unsigned int cmd, void __user * arg)
{
	int ret = 0;

	/* Only accepts mixer (type 'M') ioctls */
	if (_IOC_TYPE(cmd) != 'M') {
		return -EINVAL;
	}

	switch (cmd) {
	case SOUND_MIXER_READ_DEVMASK:
		/* Mask available channels */
		ret =
		    SOUND_MASK_PCM | SOUND_MASK_ALTPCM | SOUND_MASK_LINE |
		    SOUND_MASK_LINE1 | SOUND_MASK_LINE2;
		break;
	case SOUND_MIXER_READ_RECMASK:
		/* Mask available recording channels */
		ret = SOUND_MASK_LINE | SOUND_MASK_LINE1 | SOUND_MASK_LINE2;
		break;
	case SOUND_MIXER_READ_OUTMASK:
		/* Mask available recording channels */
		ret = SOUND_MASK_PCM | SOUND_MASK_ALTPCM;
		break;
	case SOUND_MIXER_READ_STEREODEVS:
		/* Mask stereo capable channels */
		ret = SOUND_MASK_PCM | SOUND_MASK_ALTPCM | SOUND_MASK_LINE;
		break;
	case SOUND_MIXER_READ_CAPS:
		/* Only one recording source at a time */
		ret = SOUND_CAP_EXCL_INPUT;
		break;
	case SOUND_MIXER_WRITE_PCM:
		/* Set and return new volume */
		if (get_user(ret, (int *)arg)) {
			return -EFAULT;
		}
		audio_volume_check(ret, &audio_dac1_vol_l, &audio_dac1_vol_r);
		if (test_bit(AUDIO_DAC1_SELECTED, &dsp_audio_status))
			set_audio_dac1_vol(audio_dac1_vol_l, audio_dac1_vol_r);
	case SOUND_MIXER_READ_PCM:
		/* Return volume */
		ret = audio_dac1_vol_l | (audio_dac1_vol_r << 8);
		break;
	case SOUND_MIXER_WRITE_ALTPCM:
		/* Set and return new volume */
		if (get_user(ret, (int *)arg)) {
			return -EFAULT;
		}
		audio_volume_check(ret, &audio_dac2_vol_l, &audio_dac2_vol_r);
		if (test_bit(AUDIO_DAC2_SELECTED, &dsp_audio_status))
			set_audio_dac2_vol(audio_dac2_vol_l, audio_dac2_vol_r);
	case SOUND_MIXER_READ_ALTPCM:
		/* Return volume */
		ret = audio_dac2_vol_l | (audio_dac2_vol_r << 8);
		break;
	case SOUND_MIXER_WRITE_LINE:	/*recording from both left and right */
		/* Set and return new volume */
		if (get_user(ret, (int *)arg)) {
			return -EFAULT;
		}
		audio_volume_check(ret, &audio_adc_vol_l, &audio_adc_vol_r);
		if (test_bit(AUDIO_LINE_SELECTED, &dsp_audio_status))
			set_audio_adc_vol(audio_adc_vol_l, audio_adc_vol_r);
	case SOUND_MIXER_READ_LINE:
		/* Return input volume */
		ret = audio_adc_vol_l | (audio_adc_vol_r << 8);
		break;
	case SOUND_MIXER_WRITE_LINE1:	/*recording from left only */
		/* Set and return new volume */
		if (get_user(ret, (int *)arg)) {
			return -EFAULT;
		}
		audio_volume_check(ret, &audio_adc_vol_1, 0);
		if (test_bit(AUDIO_LINE1_SELECTED, &dsp_audio_status))
			set_audio_adc_vol(audio_adc_vol_1, 0);
	case SOUND_MIXER_READ_LINE1:
		/* Return input volume */
		ret = audio_adc_vol_1 | (audio_adc_vol_1 << 8);
		break;
	case SOUND_MIXER_WRITE_LINE2:	/*recording from right only */
		/* Set and return new volume */
		if (get_user(ret, (int *)arg)) {
			return -EFAULT;
		}
		audio_volume_check(ret, &audio_adc_vol_2, 0);
		if (test_bit(AUDIO_LINE2_SELECTED, &dsp_audio_status))
			set_audio_adc_vol(0, audio_adc_vol_2);
	case SOUND_MIXER_READ_LINE2:
		/* Return input volume */
		ret = audio_adc_vol_2 | (audio_adc_vol_2 << 8);
		break;
	case SOUND_MIXER_WRITE_RECSRC:
		if (get_user(ret, (int *)arg)) {
			return -EFAULT;
		}
		dsp_mixer_rec_src =
		    ret & (SOUND_MASK_LINE | SOUND_MASK_LINE1 |
			   SOUND_MASK_LINE2);

		/*changing volume of the new rec src now, because
		   it is just left or right channel, and both are always available */

		if (test_bit(AUDIO_LINE_SELECTED, &dsp_audio_status) ||
		    test_bit(AUDIO_LINE1_SELECTED, &dsp_audio_status) ||
		    test_bit(AUDIO_LINE2_SELECTED, &dsp_audio_status)) {
			adjust_rec_volume();
		}

	case SOUND_MIXER_READ_RECSRC:
		ret = dsp_mixer_rec_src;
		break;
	case SOUND_MIXER_WRITE_OUTSRC:
		if (get_user(ret, (int *)arg)) {
			return -EFAULT;
		}
		dsp_mixer_out_src = ret & (SOUND_MASK_PCM | SOUND_MASK_ALTPCM);
		/*new dac volume will be set only after play restart - when audio stream is
		   redirected to the other dac */
	case SOUND_MIXER_READ_OUTSRC:
		ret = dsp_mixer_out_src;
		break;
	default:
		return -EINVAL;
	}

	return put_user(ret, (int *)arg);
}

static struct mixer_operations audio_mixer_operations = {
	.owner = THIS_MODULE,
	.id = "DSP MIXER",
	.name = "DSP MIXER",
	.ioctl = audio_mixer_ioctl
};

static void write_zero_data(u32 length)
{
	u32 tmp_size;
	tmp_size = DSP_TX_BUF_SIZE - dsp_tx_buf_count;
	if (tmp_size > length)
		tmp_size = length;
	memset(dsp_tx_buf_virt + dsp_tx_buf_count, 0x00, tmp_size);
	dsp_tx_buf_count += tmp_size;
	if (dsp_tx_buf_count >= DSP_TX_BUF_SIZE)
		dsp_tx_buf_count = 0;
	tmp_size = length - tmp_size;
	memset(dsp_tx_buf_virt + dsp_tx_buf_count, 0x00, tmp_size);
	dsp_tx_buf_count += tmp_size;
	if (dsp_tx_buf_count >= DSP_TX_BUF_SIZE)
		dsp_tx_buf_count = 0;
}

/* DSP output */
static void dsp_audio_tx_handler(u32 dma_tx_count)
{
	u32 zero_frag_len;
	struct dma_buffparms *dmap = audio_devs[dsp_audio_dev]->dmap_out;

	if (!((dma_tx_count + dsp_output_buf_margin - dsp_tx_buf_count)
	      & (DSP_TX_BUF_SIZE >> 1))) {

		if ((dmap->qlen > 1) || ((dmap->qlen > 0) &&
					 ((dmap->flags & DMA_POST) ||
					  dmap->qlen >= dmap->nbufs - 1))) {
			clear_bit(AUDIO_TX_ZERO_DATA, &dsp_audio_status);
			DMAbuf_outputintr(dsp_audio_dev, 1);
		} else {
			zero_frag_len = dsp_output_buf_margin * 2;
			if (!test_bit(AUDIO_TX_ZERO_DATA, &dsp_audio_status)) {
				write_zero_data(zero_frag_len);
				set_bit(AUDIO_TX_ZERO_DATA, &dsp_audio_status);
			} else {
				if (test_bit
				    (AUDIO_RX_INITED, &dsp_audio_status)) {
					write_zero_data(DSP_TX_BUF_SIZE -
							zero_frag_len);
					clear_bit(AUDIO_TX_INITED,
						  &dsp_audio_status);
					clear_bit(AUDIO_TX_ZERO_DATA,
						  &dsp_audio_status);
				} else
					dsp_audio_halt_io(dsp_audio_dev);
			}
		}
	}
}

static inline void read_mono_ave_sound(s16 * addr, s16 * dest_addr)
{
	int i;
	s32 tmp_data;
	for (i = 0; i < dsp_input_count_save / 4; i++) {
		tmp_data = *addr++;
		dsp_rx_buf_count += 2;
		if (dsp_rx_buf_count >= DSP_RX_BUF_SIZE) {
			dsp_rx_buf_count -= DSP_RX_BUF_SIZE;
			addr -= DSP_RX_BUF_SIZE / 2;
		}

		tmp_data += *addr++;
		dsp_rx_buf_count += 2;
		if (dsp_rx_buf_count >= DSP_RX_BUF_SIZE) {
			dsp_rx_buf_count -= DSP_RX_BUF_SIZE;
			addr -= DSP_RX_BUF_SIZE / 2;
		}
		*dest_addr++ = tmp_data / 2;
	}
}

static inline void read_mono_add_sound(s16 * addr, s16 * dest_addr)
{
	int i;
	s32 tmp_data;
	for (i = 0; i < dsp_input_count_save / 4; i++) {
		tmp_data = *addr++;
		dsp_rx_buf_count += 2;
		if (dsp_rx_buf_count >= DSP_RX_BUF_SIZE) {
			dsp_rx_buf_count -= DSP_RX_BUF_SIZE;
			addr -= DSP_RX_BUF_SIZE / 2;
		}

		tmp_data += *addr++;
		dsp_rx_buf_count += 2;
		if (dsp_rx_buf_count >= DSP_RX_BUF_SIZE) {
			dsp_rx_buf_count -= DSP_RX_BUF_SIZE;
			addr -= DSP_RX_BUF_SIZE / 2;
		}
		*dest_addr++ = tmp_data;
	}
}

static inline void read_stereo_sound(s16 * addr, s16 * dest_addr)
{
	int i;
	for (i = 0; i < dsp_input_count_save / 2; i++) {
		*dest_addr++ = *addr++;
		dsp_rx_buf_count += 2;
		if (dsp_rx_buf_count >= DSP_RX_BUF_SIZE) {
			dsp_rx_buf_count -= DSP_RX_BUF_SIZE;
			addr -= DSP_RX_BUF_SIZE / 2;
		}
	}
}

/* DSP input */
static void dsp_audio_rx_handler(u32 dma_rx_count)
{
	s16 *addr;
	s16 *dest_addr;

	if (!((dma_rx_count -
	       (dsp_rx_buf_count +
		dsp_input_count_save)) & (DSP_RX_BUF_SIZE >> 1))) {
		addr = (s16 *) (dsp_rx_buf_virt + dsp_rx_buf_count);
		dest_addr = (s16 *) phys_to_virt(dsp_input_buf_save);
		if (dsp_audio_channels == 2) {
			read_stereo_sound(addr, dest_addr);
		} else {
			if (dsp_mixer_rec_src &
			    (SOUND_MASK_LINE1 | SOUND_MASK_LINE2)) {
				read_mono_add_sound(addr, dest_addr);
			} else {
				read_mono_ave_sound(addr, dest_addr);
			}
		}
		DMAbuf_inputintr(dsp_audio_dev);
	}
}

/*
 * DSP interrupt handler
 */
static irqreturn_t dsp_audio_interrupt(int irq, void *data,
				       struct pt_regs *regs)
{
	u32 status, dma_count;
	status = read_int_status();
	if (status & DSP_TX_INT_READY) {
		dma_count = read_tx_buf_count();	/* reading count clears the status */
		if (test_bit(AUDIO_TX_INITED, &dsp_audio_status))
			dsp_audio_tx_handler(dma_count);
	}
	if (status & DSP_RX_INT_READY) {
		dma_count = read_rx_buf_count();	/* reading count clears the status */
		if (test_bit(AUDIO_RX_INITED, &dsp_audio_status))
			dsp_audio_rx_handler(dma_count);
	}

	return IRQ_HANDLED;
}

/*initialize the device*/
static int dsp_audio_probe(struct device *dev)
{
	int tmp;
	printk(KERN_INFO "PNX4008 Sound Driver\n");

	if (!request_region(PNX4008_DSP_BASE, 0x1000, "pnx4008-dsp")) {
		printk(KERN_ERR PKMOD "DSP registers are already in use\n");
		dsp_audio_remove(dev);
		return -EBUSY;
	}
	set_bit(DSP_REGION_INITED, &dsp_audio_status);

	if (!request_region(PNX4008_AUDIOCONFIG_BASE, 0x1000, "pnx4008-dsp")) {
		printk(KERN_ERR PKMOD "AUDIO registers are already in use\n");
		dsp_audio_remove(dev);
		return -EBUSY;
	}
	set_bit(AUDIO_REGION_INITED, &dsp_audio_status);

	if (request_region(PNX4008_SPI2_BASE, 0x1000, "pnx4008-dsp")) {
		set_bit(SPI2_REGION_INITED, &dsp_audio_status);
	} else
		printk(KERN_WARNING PKMOD
		       "SPI2 is already in use - headset MIC amplifier will not function\n");

	tmp = pnx4008_gpio_register_pin(GPO_14);
	if (tmp < 0)
		printk(KERN_WARNING PKMOD
		       "GPO 14 is already in use - headset MIC amplifier will not function\n");
	else {
		set_bit(GPO_14_INITED, &dsp_audio_status);
	}

	tmp = pnx4008_gpio_register_pin(GPO_15);
	if (tmp < 0)
		printk(KERN_WARNING PKMOD
		       "GPO 15 is already in use - DAC1 amplifier will not function\n");
	else {
		set_bit(GPO_15_INITED, &dsp_audio_status);
	}

	tmp = pnx4008_gpio_register_pin(GPO_16);
	if (tmp < 0)
		printk(KERN_WARNING PKMOD
		       "GPO 16 is already in use - DAC2 amplifier will not function\n");
	else {
		set_bit(GPO_16_INITED, &dsp_audio_status);
	}

	if ((dsp_audio_dev = sound_install_audiodrv(AUDIO_DRIVER_VERSION,
						    "pnx4008-dsp",
						    &dsp_audio_driver,
						    sizeof(struct
							   audio_driver),
						    DMA_DUPLEX, AFMT_S16_LE,
						    NULL, -1, -1)) < 0) {
		printk(KERN_ERR PKMOD "too many audio devices in the system\n");
		dsp_audio_remove(dev);
		return dsp_audio_dev;
	}

	if ((dsp_audio_mixerdev = sound_alloc_mixerdev()) >= 0) {
		mixer_devs[dsp_audio_mixerdev] = &audio_mixer_operations;
	} else
		printk(KERN_WARNING PKMOD "failed to load mixer device\n");

	dsp_tx_buf_virt =
	    dma_alloc_coherent(NULL, DSP_TX_BUF_SIZE, &dsp_tx_buf_phys,
			       GFP_KERNEL);

	if (!dsp_tx_buf_virt) {
		printk(KERN_ERR PKMOD "cannot allocate DSP TX buffer\n");
		dsp_audio_remove(dev);
		return -ENOMEM;
	}

	memset(dsp_tx_buf_virt, 0x00, DSP_TX_BUF_SIZE);

	dsp_rx_buf_virt =
	    dma_alloc_coherent(NULL, DSP_RX_BUF_SIZE, &dsp_rx_buf_phys,
			       GFP_KERNEL);

	if (!dsp_rx_buf_virt) {
		printk(KERN_ERR PKMOD "cannot allocate DSP RX buffer\n");
		dsp_audio_remove(dev);
		return -ENOMEM;
	}

	pll3_clk = clk_get(0, "ck_pll3");
	if (IS_ERR(pll3_clk)) {
		printk(KERN_ERR PKMOD "failed to acquire PLL3\n");
		dsp_audio_remove(dev);
		return PTR_ERR(pll3_clk);
	}
	set_bit(DSP_PLL_ACQUIRED, &dsp_audio_status);

#ifndef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
	adc_amp_enable(1);
	dac1_amp_enable(1);
	dac2_amp_enable(1);
#endif

	return 0;
}

/*deinitialize the device*/
static int dsp_audio_remove(struct device *dev)
{
	if (test_bit(DSP_PLL_ACQUIRED, &dsp_audio_status)) {
#ifndef CONFIG_SOUND_PNX4008_DYNAMIC_AMP_ENABLE
		adc_amp_enable(0);
		dac1_amp_enable(0);
		dac2_amp_enable(0);
		disable_audio_clock();
		disable_dsp_clock();
#endif
		clk_put(pll3_clk);
		clear_bit(DSP_PLL_ACQUIRED, &dsp_audio_status);
	}

	if (test_bit(DSP_IRQ_INITED, &dsp_audio_status)) {
		free_irq(DSP_P_INT, &dsp_audio_driver);
		clear_bit(DSP_IRQ_INITED, &dsp_audio_status);
	}

	if (dsp_tx_buf_virt) {
		dma_free_coherent(NULL, DSP_TX_BUF_SIZE, dsp_tx_buf_virt,
				  dsp_tx_buf_phys);
		dsp_tx_buf_virt = 0;
	}

	if (dsp_rx_buf_virt) {
		dma_free_coherent(NULL, DSP_RX_BUF_SIZE, dsp_rx_buf_virt,
				  dsp_rx_buf_phys);
		dsp_rx_buf_virt = 0;
	}

	if (dsp_audio_dev >= 0) {
		if (dsp_audio_mixerdev >= 0)
			sound_unload_mixerdev(dsp_audio_mixerdev);
		sound_unload_audiodev(dsp_audio_dev);
		dsp_audio_mixerdev = -1;
		dsp_audio_dev = -1;
	}

	if (test_bit(GPO_16_INITED, &dsp_audio_status)) {
		pnx4008_gpio_unregister_pin(GPO_16);
		clear_bit(GPO_16_INITED, &dsp_audio_status);
	}

	if (test_bit(GPO_15_INITED, &dsp_audio_status)) {
		pnx4008_gpio_unregister_pin(GPO_15);
		clear_bit(GPO_15_INITED, &dsp_audio_status);
	}

	if (test_bit(GPO_14_INITED, &dsp_audio_status)) {
		pnx4008_gpio_unregister_pin(GPO_14);
		clear_bit(GPO_14_INITED, &dsp_audio_status);
	}

	if (test_bit(SPI2_REGION_INITED, &dsp_audio_status)) {
		release_region(PNX4008_SPI2_BASE, 0x1000);
		clear_bit(SPI2_REGION_INITED, &dsp_audio_status);
	}

	if (test_bit(AUDIO_REGION_INITED, &dsp_audio_status)) {
		release_region(PNX4008_AUDIOCONFIG_BASE, 0x1000);
		clear_bit(AUDIO_REGION_INITED, &dsp_audio_status);
	}

	if (test_bit(DSP_REGION_INITED, &dsp_audio_status)) {
		release_region(PNX4008_DSP_BASE, 0x1000);
		clear_bit(DSP_REGION_INITED, &dsp_audio_status);
	}
	return 0;
}

MODULE_AUTHOR("MontaVista Software, Inc. <source@mvista.com>");
MODULE_DESCRIPTION("PNX4008 SOUND Driver");
MODULE_LICENSE("GPL");

static struct device_driver dsp_audio_device_driver = {
	.name = "dsp-oss",
	.bus = &platform_bus_type,
	.probe = dsp_audio_probe,
	.remove = dsp_audio_remove,
#ifdef CONFIG_PM
	.suspend = dsp_audio_suspend,
	.resume = dsp_audio_resume,
#endif
};

static int __init pnx4008_dsp_init(void)
{
	return driver_register(&dsp_audio_device_driver);
}

static void __exit pnx4008_dsp_exit(void)
{
	return driver_unregister(&dsp_audio_device_driver);
}

module_init(pnx4008_dsp_init);
module_exit(pnx4008_dsp_exit);
