/*
 * include/asm-arm/arch-pnx4008/spi.h
 *
 * Author: Vitaly Wool <vwool@ru.mvista.com>
 *
 * PNX4008-specific tweaks for SPI block
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ASM_ARCH_SPI_H__
#define __ASM_ARCH_SPI_H__

#include <linux/spi.h>
#include <linux/err.h>

#include <asm/dma.h>

#include <asm/arch/dma.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pm.h>
#include <asm/arch/gpio-compat.h>
#include <asm/arch/pnxalloc.h>

#include <asm/hardware/clock.h>

#define SPI_ADR_OFFSET_GLOBAL              0x0000	/* R/W - Global control register */
#define SPI_ADR_OFFSET_CON                 0x0004	/* R/W - Control register */
#define SPI_ADR_OFFSET_FRM                 0x0008	/* R/W - Frame count register */
#define SPI_ADR_OFFSET_IER                 0x000C	/* R/W - Interrupt enable register */
#define SPI_ADR_OFFSET_STAT                0x0010	/* R/W - Status register */
#define SPI_ADR_OFFSET_DAT                 0x0014	/* R/W - Data register */
#define SPI_ADR_OFFSET_DAT_MSK             0x0018	/*  W  - Data register for using mask mode */
#define SPI_ADR_OFFSET_MASK                0x001C	/* R/W - Mask register */
#define SPI_ADR_OFFSET_ADDR                0x0020	/* R/W - Address register */
#define SPI_ADR_OFFSET_TIMER_CTRL_REG      0x0400	/* R/W - IRQ timer control register */
#define SPI_ADR_OFFSET_TIMER_COUNT_REG     0x0404	/* R/W - Timed interrupt period register */
#define SPI_ADR_OFFSET_TIMER_STATUS_REG    0x0408	/* R;R/C - RxDepth and interrupt status register */

/* Bit definitions for SPI_GLOBAL register */
#define SPI_GLOBAL_BLRES_SPI               0x00000002	/*  R/W - Software reset, active high */
#define SPI_GLOBAL_SPI_ON                  0x00000001	/* R/W - SPI interface on */

/* Bit definitions for SPI_CON register */
#define SPI_CON_SPI_BIDIR                  0x00800000	/* R/W - SPI data in bidir. mux */
#define SPI_CON_SPI_BHALT                  0x00400000	/* R/W - Halt control */
#define SPI_CON_SPI_BPOL                   0x00200000	/* R/W - Busy signal active polarity */
#define SPI_CON_SPI_SHFT                   0x00100000	/* R/W - Data shifting enable in mask mode */
#define SPI_CON_SPI_MSB                    0x00080000	/* R/W - Transfer LSB or MSB first */
#define SPI_CON_SPI_EN                     0x00040000	/* R/W - SPULSE signal enable */
#define SPI_CON_SPI_MODE                   0x00030000	/* R/W - SCK polarity and phase modes */
#define SPI_CON_SPI_MODE3                  0x00030000	/* R/W - SCK polarity and phase mode 3 */
#define SPI_CON_SPI_MODE2                  0x00020000	/* R/W - SCK polarity and phase mode 2 */
#define SPI_CON_SPI_MODE1                  0x00010000	/* R/W - SCK polarity and phase mode 1 */
#define SPI_CON_SPI_MODE0                  0x00000000	/* R/W - SCK polarity and phase mode 0 */
#define SPI_CON_RxTx                       0x00008000	/* R/W - Transfer direction */
#define SPI_CON_THR                        0x00004000	/* R/W - Threshold selection */
#define SPI_CON_SHIFT_OFF                  0x00002000	/* R/W - Inhibits generation of clock pulses on sck_spi pin */
#define SPI_CON_BITNUM                     0x00001E00	/* R/W - No of bits to tx or rx in one block transfer */
#define SPI_CON_CS_EN                      0x00000100	/* R/W - Disable use of CS in slave mode */
#define SPI_CON_MS                         0x00000080	/* R/W - Selection of master or slave mode */
#define SPI_CON_RATE                       0x0000007F	/* R/W - Transmit/receive rate */

#define SPI_CON_RATE_0                     0x00
#define SPI_CON_RATE_1                     0x01
#define SPI_CON_RATE_2                     0x02
#define SPI_CON_RATE_3                     0x03
#define SPI_CON_RATE_4                     0x04
#define SPI_CON_RATE_13                     0x13

/* Bit definitions for SPI_FRM register */
#define SPI_FRM_SPIF                       0x0000FFFF	/* R/W - Number of frames to be transfered */

/* Bit definitions for SPI_IER register */
#define SPI_IER_SPI_INTCS                  0x00000004	/* R/W - SPI CS level change interrupt enable  */
#define SPI_IER_SPI_INTEOT                 0x00000002	/* R/W - End of transfer interrupt enable */
#define SPI_IER_SPI_INTTHR                 0x00000001	/* R/W - FIFO threshold interrupt enable */

/* Bit definitions for SPI_STAT register */
#define SPI_STAT_SPI_INTCLR                0x00000100	/* R/WC - Clear interrupt */
#define SPI_STAT_SPI_EOT                   0x00000080	/* R/W - End of transfer */
#define SPI_STAT_SPI_BSY_SL                0x00000040	/* R/W - Status of the input pin spi_busy */
#define SPI_STAT_SPI_CSL                   0x00000020	/* R/W - Indication of the edge on SPI_CS that caused an int.  */
#define SPI_STAT_SPI_CSS                   0x00000010	/* R/W - Level of SPI_CS has changed in slave mode */
#define SPI_STAT_SPI_BUSY                  0x00000008	/* R/W - A data transfer is ongoing  */
#define SPI_STAT_SPI_BF                    0x00000004	/* R/W - FIFO is full */
#define SPI_STAT_SPI_THR                   0x00000002	/* R/W - No of entries in Tx/Rx FIFO */
#define SPI_STAT_SPI_BE                    0x00000001	/* R/W - FIFO is empty */

/* Bit definitions for SPI_DAT register */
#define SPI_DAT_SPID                       0x0000FFFF	/* R/W - SPI data bits  */

/* Bit definitions for SPI_DAT_MSK register */
#define SPI_DAT_MSK_SPIDM                  0x0000FFFF	/*  W  - SPI data bits to send using the masking mode */

/* Bit definitions for SPI_MASK register */
#define SPI_MASK_SPIMSK                    0x0000FFFF	/* R/W - Masking bits used for validating data bits */

/* Bit definitions for SPI_ADDR register */
#define SPI_ADDR_SPIAD                     0x0000FFFF	/* R/W - Address bits to add to the data bits */

/* Bit definitions for SPI_TIMER_CTRL_REG register */
#define SPI_TIMER_CTRL_REG_TIRQE           0x00000004	/* R/W - Timer interrupt enable */
#define SPI_TIMER_CTRL_REG_PIRQE           0x00000002	/* R/W - Peripheral interrupt enable */
#define SPI_TIMER_CTRL_REG_MODE            0x00000001	/* R/W - Mode */

/* Bit definitions for SPI_TIMER_COUNT_REG register */
#define SPI_TIMER_COUNT_REG                0x0000FFFF	/* R/W - Timed interrupt period */

/* Bit definitions for SPI_TIMER_STATUS_REG register */
#define SPI_TIMER_STATUS_REG_INT_STAT      0x00008000	/* R/C - Interrupt status */
#define SPI_TIMER_STATUS_REG_FIFO_DEPTH    0x0000007F	/*  R  - FIFO depth value (for debug purpose) */

#define SPIPNX_STAT_EOT SPI_STAT_SPI_EOT
#define SPIPNX_STAT_THR SPI_STAT_SPI_THR
#define SPIPNX_IER_EOT SPI_IER_SPI_INTEOT
#define SPIPNX_IER_THR SPI_IER_SPI_INTTHR
#define SPIPNX_STAT_CLRINT SPI_STAT_SPI_INTCLR

#define SPIPNX_GLOBAL_RESET_SPI SPI_GLOBAL_BLRES_SPI
#define SPIPNX_GLOBAL_SPI_ON 	SPI_GLOBAL_SPI_ON
#define SPIPNX_CON_MS		SPI_CON_MS
#define SPIPNX_CON_BIDIR	SPI_CON_SPI_BIDIR
#define SPIPNX_CON_RXTX		SPI_CON_RxTx
#define SPIPNX_CON_THR		SPI_CON_THR
#define SPIPNX_CON_SHIFT_OFF	SPI_CON_SHIFT_OFF
#define SPIPNX_DATA		SPI_ADR_OFFSET_DAT
#define SPI_PNX4008_CLOCK_IN  	104000000
#define SPI_PNX4008_CLOCK 	2670000
#define SPIPNX_CLOCK ((((SPI_PNX4008_CLOCK_IN / SPI_PNX4008_CLOCK) - 2) / 2) & SPI_CON_RATE_MASK )
#define SPIPNX_DATA_REG(s) ( (u32)(s) + SPIPNX_DATA )

typedef volatile struct {
	u32 global;		/* 0x000             */
	u32 con;		/* 0x004             */
	u32 frm;		/* 0x008             */
	u32 ier;		/* 0x00C             */
	u32 stat;		/* 0x010             */
	u32 dat;		/* 0x014             */
	u32 dat_msk;		/* 0x018             */
	u32 mask;		/* 0x01C             */
	u32 addr;		/* 0x020             */
	u32 _d0[(SPI_ADR_OFFSET_TIMER_CTRL_REG -
		 (SPI_ADR_OFFSET_ADDR + sizeof(u32))) / sizeof(u32)];
	u32 timer_ctrl;		/* 0x400             */
	u32 timer_count;	/* 0x404             */
	u32 timer_status;	/* 0x408             */
}
vhblas_spiregs, *pvhblas_spiregs;

#define SPI_CON_RATE_MASK			0x7f

static void spipnx_arch_set_clock_rate( int clock_khz, int ref_clock_khz, vhblas_spiregs* regs )
{
	u32 spi_clock;

	if( !clock_khz )
		spi_clock = ref_clock_khz < 104000 ? 0 : 1;
	else
		spi_clock = ref_clock_khz / (2*clock_khz + 2000);
	regs->con &= ~SPI_CON_RATE_MASK;
	regs->con |= spi_clock;
}

static inline void spipnx_clk_stop(void *clk_data)
{
	struct clk *clk = (struct clk *)clk_data;
	if (clk && !IS_ERR(clk)) {
		clk_set_rate(clk, 0);
		clk_put(clk);
	}
}

static inline void spipnx_clk_start(void *clk_data)
{
	struct clk *clk = (struct clk *)clk_data;
	if (clk && !IS_ERR(clk)) {
		clk_set_rate(clk, 1);
		clk_put(clk);
	}
}

static inline void *spipnx_clk_init(struct platform_device *dev)
{
	struct clk *clk = clk_get(0, dev->id == 0 ? "spi0_ck" : "spi1_ck");
	if (IS_ERR(clk))
		printk("spi%d_ck pointer err\n", dev->id);
	return clk;
}

static inline void spipnx_clk_free(void * clk)
{
	clk_put((struct clk *)clk);
}

static inline void spipnx_arch_do_scale(u32 saved_clock_khz, vhblas_spiregs* regs)
{
	struct clk *div_clk= clk_get(0, "hclk_ck");
	struct clk *arm_clk = clk_get(0, "ck_pll4");
	if (!IS_ERR(arm_clk) && !IS_ERR(div_clk))
		spipnx_arch_set_clock_rate(
			saved_clock_khz,
			clk_get_rate(arm_clk) / clk_get_rate(div_clk),
			regs);
	if (!IS_ERR(arm_clk))
		clk_put(arm_clk);
	if (!IS_ERR(div_clk))
		clk_put(div_clk);
}

static inline void spipnx_arch_init(int saved_clock_khz, vhblas_spiregs *regs)
{
	struct clk *div_clk= clk_get(0, "hclk_ck");
	struct clk *arm_clk = clk_get(0, "ck_pll4");
	if (!IS_ERR(arm_clk) && !IS_ERR(div_clk))
		spipnx_arch_set_clock_rate(
			saved_clock_khz,
			clk_get_rate(arm_clk) / clk_get_rate(div_clk),
			regs);
	if (!IS_ERR(arm_clk))
		clk_put(arm_clk);
	if (!IS_ERR(div_clk))
		clk_put(div_clk);

	regs->con |= SPI_CON_SPI_BIDIR | SPI_CON_SPI_BHALT | SPI_CON_SPI_BPOL |
		SPI_CON_THR | SPI_CON_MS;
}

typedef enum {
	SELECT = 0,
	UNSELECT,
	INIT,
	BEFORE_READ,
	AFTER_READ,
	BEFORE_WRITE,
	AFTER_WRITE,
	MESSAGE_START,
	MESSAGE_END,
} spi_cb_type_t;

static inline void spipnx_arch_control(struct spi_device *device, int type, u32 ctl)
{
	if (type == MESSAGE_START || type == MESSAGE_END)
		if( ctl ) {
			dev_dbg( &device->dev, "ctl=%x\n", ctl );
			GPIO_WritePin( GPO_02, ctl );
		}
}

static inline int spipnx_request_dma(
		int *dma_channel, char *name,
		void (*handler)( int, int, void*, struct pt_regs*),
		void *dma_context)
{
	int err;

	if (*dma_channel != -1) {
		err = *dma_channel;
		goto out;
	}
	err = pnx4008_request_channel(name, -1, handler, dma_context);
	if (err >= 0) {
		*dma_channel = err;
	}
out:
	return err < 0 ? err : 0;
}

static inline void spipnx_release_dma(int *dma_channel)
{
	if (*dma_channel >= 0) {
		pnx4008_free_channel(*dma_channel);
	}
	*dma_channel = -1;
}

static inline void spipnx_start_dma(int dma_channel)
{
	pnx4008_dma_ch_enable(dma_channel);
}

struct spipnx_dma_transfer_t
{
	dma_addr_t dma_buffer;
	void *saved_ll;
	u32 saved_ll_dma;
};

static inline void spipnx_stop_dma(int dma_channel, void* dev_buf)
{
	struct spipnx_dma_transfer_t *buf = dev_buf;

	pnx4008_dma_ch_disable(dma_channel);
	if (buf->saved_ll) {
		pnx4008_free_ll(buf->saved_ll_dma, buf->saved_ll);
		buf->saved_ll = NULL;
	}
}

static inline int spipnx_setup_dma(u32 iostart, int slave_nr, int dma_channel,
				int mode, void* params, int len)
{
	int err = 0;
	struct spipnx_dma_transfer_t *buff = params;

	pnx4008_dma_config_t cfg;
	pnx4008_dma_ch_config_t ch_cfg;
	pnx4008_dma_ch_ctrl_t ch_ctrl;

	memset(&cfg, 0, sizeof(cfg));

	if (mode == DMA_MODE_READ) {
		cfg.dest_addr = buff->dma_buffer; /* buff->dma_buffer; */
		cfg.src_addr = (u32) SPIPNX_DATA_REG(iostart);
		ch_cfg.flow_cntrl = FC_PER2MEM_DMA;
		ch_cfg.src_per = slave_nr;
		ch_cfg.dest_per = 0;
		ch_ctrl.di = 1;
		ch_ctrl.si = 0;
	} else if (mode == DMA_MODE_WRITE) {
		cfg.src_addr = buff->dma_buffer; /* buff->dma_buffer; */
		cfg.dest_addr = (u32) SPIPNX_DATA_REG(iostart);
		ch_cfg.flow_cntrl = FC_MEM2PER_DMA;
		ch_cfg.dest_per = slave_nr;
		ch_cfg.src_per = 0;
		ch_ctrl.di = 0;
		ch_ctrl.si = 1;
	} else {
		err = -EINVAL;
	}

	ch_cfg.halt = 0;
	ch_cfg.active = 1;
	ch_cfg.lock = 0;
	ch_cfg.itc = 1;
	ch_cfg.ie = 1;
	ch_ctrl.tc_mask = 1;
	ch_ctrl.cacheable = 0;
	ch_ctrl.bufferable = 0;
	ch_ctrl.priv_mode = 1;
	ch_ctrl.dest_ahb1 = 0;
	ch_ctrl.src_ahb1 = 0;
	ch_ctrl.dwidth = WIDTH_BYTE;
	ch_ctrl.swidth = WIDTH_BYTE;
	ch_ctrl.dbsize = 1;
	ch_ctrl.sbsize = 1;
	ch_ctrl.tr_size = len;
	if (0 > (err = pnx4008_dma_pack_config(&ch_cfg, &cfg.ch_cfg))) {
		goto out;
	}
	if (0 > (err = pnx4008_dma_pack_control(&ch_ctrl, &cfg.ch_ctrl))) {
		if ( err == -E2BIG ) {
			printk( KERN_DEBUG"buffer is too large, splitting\n" );
			pnx4008_dma_split_head_entry(&cfg, &ch_ctrl);
			buff->saved_ll = cfg.ll;
			buff->saved_ll_dma = cfg.ll_dma;
			err = 0;
		} else {
			goto out;
		}
	}
	err = pnx4008_config_channel(dma_channel, &cfg);
out:
	return err;
}

static inline void spipnx_start_event_init(int id)
{
	int se = id==0 ? SE_SPI1_DATAIN_INT : SE_SPI2_DATAIN_INT;

	/*setup wakeup interrupt*/
	start_int_set_rising_edge(se);
	start_int_ack(se);
	start_int_umask(se);
}

static inline void spipnx_start_event_deinit(int id)
{
	int se = id==0 ? SE_SPI1_DATAIN_INT : SE_SPI2_DATAIN_INT;

	start_int_mask(se);
}

struct spipnx_wifi_params {
	int gpio_cs_line;
};
#define SPIPNX_WIFI "spipnx-wifi"

static inline void spipnx_arch_add_devices (struct device *bus, int id)
{
	extern struct spipnx_wifi_params spipnx_wifi_params;

	dev_dbg (bus, "adding devices on bus id '%d' \n", id );
	switch (id)
	{
	case 0:
		spi_device_add (bus, "EEPROM", NULL );
		spi_device_add (bus, SPIPNX_WIFI, &spipnx_wifi_params );
		break;
	default:
		break;
	}
	return;
}

#endif /* __ASM_ARCH_SPI_H___ */
