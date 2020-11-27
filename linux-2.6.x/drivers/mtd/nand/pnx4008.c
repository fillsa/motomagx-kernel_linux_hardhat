/*
 *  drivers/mtd/nand/pnx4008.c
 *
 *  Copyright (c) 2005 MontaVista Software, Inc <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Support for SanDisk MLC controller on Philips pnx4008 board
 */

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/dma-mapping.h>

#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/dma.h>
#include <asm/errno.h>

#include <asm/arch/hardware.h>
#include <asm/arch/dma.h>
#include <asm/arch/gpio.h>

#include "pnx4008.h"

#define MLC_DATA_PAGE 512
#define MLC_OOB_SIZE  6
#define MLC_ECC_SIZE  10
#define MLC_PAGE (MLC_DATA_PAGE+MLC_OOB_SIZE+MLC_ECC_SIZE)

struct pnx4008_mtd_s {
	struct mtd_info mtd;
	struct nand_chip nand;
	int dma;
	struct completion done;
	u32 flashclk_reg;
	struct device *dev;
	int hwecc_mode;
	void *saved_ll;
	u32 saved_ll_dma;
	void (*rdbuf) (struct mtd_info * mtd, u8 * buf, int len);
	void (*wrbuf) (struct mtd_info * mtd, const u8 * buf, int len);
};

static struct nand_oobinfo pnx4008_nand_hw_eccoob = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	/*
	 * Should be 40, but it's too much for the structure.
	 * However, we rely only on layout in nand_base.c so that
	 * doesn't matter really.
	 */
	.eccbytes = 32,
	/*
	 * The SanDisk page is organized as follows:
	 *
	 * | .... 512 data bytes .... | 6 bytes OOB | 10 bytes ECC |
	 * | .... 512 data bytes .... | 6 bytes OOB | 10 bytes ECC |
	 * | .... 512 data bytes .... | 6 bytes OOB | 10 bytes ECC |
	 * | .... 512 data bytes .... | 6 bytes OOB | 10 bytes ECC |
	 *
	 * i.e, 4 sub-pages with OOB follows data bytes
	 */
	.eccpos = {6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		   16 + 6, 16 + 7, 16 + 8, 16 + 9, 16 + 10, 16 + 11, 16 + 12,
		   16 + 13, 16 + 14, 16 + 15,
		   32 + 6, 32 + 7, 32 + 8, 32 + 9, 32 + 10, 32 + 11, 32 + 12,
		   32 + 13, 32 + 14, 32 + 15,
		   48 + 6, 48 + 7,
		   },
	.oobfree = {{2, 4}, {16, 6}, {32, 6}, {48, 6}},
};

static struct pnx4008_mtd_s *pnx4008_nand_data = NULL;
static int mlc_flash_dma
#ifdef CONFIG_MTD_NAND_PNX4008_DMA
	=1
#endif
;
static int mlc_hwecc
#ifdef CONFIG_MTD_NAND_PNX4008_HWECC
	=1
#endif
;

static int pnx4008_nand_ready(struct mtd_info *mtd);
static void pnx4008_nand_command(struct mtd_info *mtd, unsigned command,
				 int column, int page_addr);
static void pnx4008_nand_hwcontrol(struct mtd_info *mtd, int hwc);
static void pnx4008_enable_hwecc(struct mtd_info *this, int ecc_mode);
static int pnx4008_calculate_ecc(struct mtd_info *mtd, const u_char * data,
				 u_char * ecc_code);
static int pnx4008_correct_data(struct mtd_info *mtd, u_char * data,
				u_char * read_ecc, u_char * calc_ecc);

#define MLC_WRITE( nand, offset, what ) __raw_writel( what, (u8*)( ((struct nand_chip*)nand)->IO_ADDR_R) + offset )
#define MLC_READ( nand, offset ) __raw_readl( (u8*)( ((struct nand_chip*)nand)->IO_ADDR_R) + offset )
#define MLC_READ2( nand, offset ) __raw_readb( (u8*)( ((struct nand_chip*)nand)->IO_ADDR_W) + offset )
#define MLC_WRITE2( nand, offset, what ) __raw_writeb( what, (u8*)( ((struct nand_chip*)nand)->IO_ADDR_W) + offset )

static inline u8 pnx4008_read_data(struct nand_chip *this)
{
	int cnt = 0xFFF;

	while (!(MLC_READ(this, MLC_ISR) & MLC_ISR_READY) && cnt--) {
		continue;
	}
	return MLC_READ2(this, 0);
}

static u8 pnx4008_nand_read_byte(struct mtd_info *mtd)
{
	return pnx4008_read_data(mtd->priv);
}

static void pnx4008_nand_write_byte(struct mtd_info *mtd, u8 byte)
{
	MLC_WRITE2(mtd->priv, 0, byte);
}

static void pnx4008_nand_dma_handler(int irq, int cause, void *context,
				     struct pt_regs *regs)
{
	if (cause & DMA_TC_INT)
		complete(context);
}

static inline int
pnx4008_nand_config_dma(struct pnx4008_mtd_s *ctx, void *vaddr, u32 dma_buf,
			int len, int mode)
{
	pnx4008_dma_config_t cfg;
	pnx4008_dma_ch_config_t ch_cfg;
	pnx4008_dma_ch_ctrl_t ch_ctrl;
	const int peripheral_id = PER_MLC_NDF_SREC;
	int err = 0;
	int channel;

	if (!ctx) {
		err = -EINVAL;
		goto out;
	}
	channel = ctx->dma;

	memset(&cfg, 0, sizeof(cfg));

	if (mode == DMA_MODE_WRITE) {
		ch_cfg.flow_cntrl = FC_MEM2PER_DMA;
		cfg.src_addr = dma_buf;
		ch_cfg.src_per = 0;
		cfg.dest_addr = PNX4008_FLASH_DATA;
		ch_cfg.dest_per = peripheral_id;
		ch_ctrl.di = 0;
		ch_ctrl.si = 1;
	} else if (mode == DMA_MODE_READ) {
		ch_cfg.flow_cntrl = FC_PER2MEM_DMA;
		cfg.dest_addr = dma_buf;
		ch_cfg.dest_per = 0;
		cfg.src_addr = PNX4008_FLASH_DATA;
		ch_cfg.src_per = peripheral_id;
		ch_ctrl.si = 0;
		ch_ctrl.di = 1;
	} else {
		return -EFAULT;
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
	ch_ctrl.dwidth = ch_ctrl.swidth = WIDTH_BYTE;
	ch_ctrl.dbsize = 1;
	ch_ctrl.sbsize = 1;
	ch_ctrl.tr_size = len;

	if (0 > (err = pnx4008_dma_pack_config(&ch_cfg, &cfg.ch_cfg))) {
		goto out;
	}
	if (0 > (err = pnx4008_dma_pack_control(&ch_ctrl, &cfg.ch_ctrl))) {
		if (err == -E2BIG) {
			printk(KERN_DEBUG
			       "Too big buffer for DMA... splitting\n");
			pnx4008_dma_split_head_entry(&cfg, &ch_ctrl);
			ctx->saved_ll = cfg.ll;
			ctx->saved_ll_dma = cfg.ll_dma;
			err = 0;
		} else
			goto out;
	}
	err = pnx4008_config_channel(channel, &cfg);
out:
	return err;
}

static void pnx4008_nand_wrbuf_dma(struct mtd_info *mtd, const u8 * buf,
				   int len)
{
	struct nand_chip *this = mtd->priv;
	struct pnx4008_mtd_s *ctx = this->priv;
	dma_addr_t dma_addr;
	void *vaddr;
	vaddr = dma_alloc_coherent(ctx->dev, len, &dma_addr, GFP_KERNEL);
	if (!vaddr)
		return;

	memcpy(vaddr, buf, len);
	if (pnx4008_nand_config_dma(ctx, vaddr, dma_addr, len, DMA_MODE_WRITE) <
	    0)
		goto out;
	init_completion(&ctx->done);
	pnx4008_dma_ch_enable(ctx->dma);
	wait_for_completion(&ctx->done);
	pnx4008_dma_ch_disable(ctx->dma);
	if (ctx->saved_ll) {
		pnx4008_free_ll(ctx->saved_ll_dma, ctx->saved_ll);
		ctx->saved_ll = NULL;
	}
out:
	dma_free_coherent(ctx->dev, len, vaddr, dma_addr);
}

static void pnx4008_nand_wrbuf_pio(struct mtd_info *mtd, const u8 * buf,
				   int len)
{
	while (len--)
		pnx4008_nand_write_byte(mtd, *buf++);
}

static void pnx4008_nand_write_buf(struct mtd_info *mtd, const u8 * buf,
				   int len)
{
	struct nand_chip *this = mtd->priv;
	struct pnx4008_mtd_s *ctx = this->priv;

	DEBUG(MTD_DEBUG_LEVEL3, "Writing '%d' bytes\n", len);
	ctx->wrbuf(mtd, buf, len);
}

static void pnx4008_nand_rdbuf_dma(struct mtd_info *mtd, u8 * buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct pnx4008_mtd_s *ctx = this->priv;
	dma_addr_t dma_addr;
	void *vaddr;

	vaddr = dma_alloc_coherent(ctx->dev, len, &dma_addr, GFP_KERNEL);
	if (!vaddr)
		return;

	if (pnx4008_nand_config_dma(ctx, vaddr, dma_addr, len, DMA_MODE_READ) <
	    0)
		goto out;
	init_completion(&ctx->done);
	pnx4008_dma_ch_enable(ctx->dma);
	wait_for_completion(&ctx->done);
	pnx4008_dma_ch_disable(ctx->dma);
	memcpy(buf, vaddr, len);
	if (ctx->saved_ll) {
		pnx4008_free_ll(ctx->saved_ll_dma, ctx->saved_ll);
		ctx->saved_ll = NULL;
	}
out:
	dma_free_coherent(ctx->dev, len, vaddr, dma_addr);
}

static void pnx4008_nand_rdbuf_pio(struct mtd_info *mtd, u8 * buf, int len)
{
	while (len--)
		*buf++ = pnx4008_nand_read_byte(mtd);
}

static void pnx4008_nand_read_buf(struct mtd_info *mtd, u8 * buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct pnx4008_mtd_s *ctx = this->priv;

	if (ctx->hwecc_mode != NAND_ECC_READSYN)
		ctx->rdbuf(mtd, buf, len);
	else
		memset(buf, 0xFF, len);
}

static int pnx4008_nand_verify_buf(struct mtd_info *mtd, const u8 * buf,
				   int len)
{
	return 0;		/* Verified OK */
}

static int pnx4008_nand_suspend(struct device *dev, u32 state, u32 level)
{
	int retval = 0;

#ifdef CONFIG_PM
	struct mtd_info *mtd = dev_get_drvdata(dev);
	if (mtd && mtd->suspend && level == SUSPEND_SAVE_STATE) {
		retval = mtd->suspend(mtd);
		if (retval == 0) {
			pnx4008_nand_data->flashclk_reg =
			    __raw_readl(PNX4008_FLASHCLK_CTRL_REG);
			__raw_writel(0, PNX4008_FLASHCLK_CTRL_REG);
			udelay(1);
		}
	}
#endif
	return retval;
}

static int pnx4008_nand_resume(struct device *dev, u32 level)
{
#ifdef CONFIG_PM
	struct mtd_info *mtd = dev_get_drvdata(dev);

	if (mtd && mtd->resume && level == RESUME_RESTORE_STATE) {
		mtd->resume(mtd);
		__raw_writel(pnx4008_nand_data->flashclk_reg,
			     PNX4008_FLASHCLK_CTRL_REG);
		udelay(1);
	}
#endif
	return 0;
}

#ifdef CONFIG_MTD_PARTITIONS
static struct mtd_partition static_partition[] = {
	{.name = "Booting Image",
	 .offset = 0,
	 .size = 0x20000,
	 .mask_flags = MTD_WRITEABLE	/* force read-only */
	 },
	{.name = "root fs",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 0xBE0000,
	 },
	{.name = "U-Boot",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 0x100000,
	 .mask_flags = MTD_WRITEABLE	/* force read-only */
	},
	{.name = "kernel",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 0x1300000,
	},
	{.name = "user fs",
	 .size = MTDPART_SIZ_FULL,
	 .offset = MTDPART_OFS_APPEND,
	},
};

const char *part_probes[] = {
	"cmdlinepart",
	NULL,
};

#endif

struct page_layout_item pnx4008_nand_layout[] = {
	{ .type = ITEM_TYPE_DATA, .length = MLC_DATA_PAGE },
	{ .type = ITEM_TYPE_OOB, .length = MLC_OOB_SIZE, },
	{ .type = ITEM_TYPE_ECC, .length = MLC_ECC_SIZE, },
	{ .type = 0, .length = 0, },
};

static int __init pnx4008_nand_probe(struct device *dev)
{
	struct nand_chip *this;
	struct mtd_info *mtd;
#ifdef CONFIG_MTD_PARTITIONS
	struct mtd_partition *dynamic_partition = 0;
#endif
	int err = 0;
	u32 flashclk_init = FLASHCLK_MLC_CLOCKS | FLASHCLK_MLC_SELECT;

	pnx4008_nand_data = kmalloc(sizeof(struct pnx4008_mtd_s), GFP_KERNEL);
	if (!pnx4008_nand_data) {
		printk(KERN_ERR "%s: failed to allocate mtd_info\n",
		       __FUNCTION__);
		err = -ENOMEM;
		goto out;
	}
	memset(pnx4008_nand_data, 0, sizeof(struct pnx4008_mtd_s));
	pnx4008_nand_data->dma = -1;
	pnx4008_nand_data->hwecc_mode = -1;

	pnx4008_nand_data->dev = dev;

	/* structures must be linked */
	this = &pnx4008_nand_data->nand;
	mtd = &pnx4008_nand_data->mtd;
	mtd->priv = this;
	this->priv = pnx4008_nand_data;

	this->chip_delay = 30;
	this->IO_ADDR_R = (void __iomem *)IO_ADDRESS(PNX4008_MLC_FLASH_BASE);
	this->IO_ADDR_W = (void __iomem *)IO_ADDRESS(PNX4008_FLASH_DATA);
	this->options = NAND_SAMSUNG_LP_OPTIONS | NAND_SKIP_BBTSCAN;
	this->hwcontrol = pnx4008_nand_hwcontrol;
	this->dev_ready = pnx4008_nand_ready;
	this->eccmode = NAND_ECC_SOFT;
	this->cmdfunc = pnx4008_nand_command;
	this->read_byte = pnx4008_nand_read_byte;
	this->write_byte = pnx4008_nand_write_byte;
	this->write_buf = pnx4008_nand_write_buf;
	this->verify_buf = pnx4008_nand_verify_buf;
	this->read_buf = pnx4008_nand_read_buf;
	if (mlc_hwecc) {
		this->eccmode = NAND_ECC_HW6_512;
		this->options |= NAND_HWECC_SYNDROME;
		this->autooob = &pnx4008_nand_hw_eccoob;
		this->enable_hwecc = pnx4008_enable_hwecc;
		this->calculate_ecc = pnx4008_calculate_ecc;
		this->correct_data = pnx4008_correct_data;
		this->layout = pnx4008_nand_layout;
	}

	if (mlc_flash_dma) {
		pnx4008_nand_data->dma =
		    pnx4008_request_channel("MLC Flash", -1,
					    pnx4008_nand_dma_handler,
					    &pnx4008_nand_data->done);
		if (pnx4008_nand_data->dma < 0) {
			printk(KERN_ERR
			       "%s: cannot request DMA channel (%d), falling back to non-DMA mode.\n",
			       __FUNCTION__, pnx4008_nand_data->dma);
			mlc_flash_dma = 0;
		} else {
			flashclk_init |= FLASHCLK_NAND_RnB_REQ_E;
		}
	}

	if (mlc_flash_dma) {
		pnx4008_nand_data->rdbuf = pnx4008_nand_rdbuf_dma;
		pnx4008_nand_data->wrbuf = pnx4008_nand_wrbuf_dma;
	} else {
		pnx4008_nand_data->rdbuf = pnx4008_nand_rdbuf_pio;
		pnx4008_nand_data->wrbuf = pnx4008_nand_wrbuf_pio;
	}

	__raw_writel(flashclk_init, PNX4008_FLASHCLK_CTRL_REG);
	pnx4008_nand_data->flashclk_reg = flashclk_init;

	MLC_WRITE(this, MLC_LOCK_PR, MLC_UNLOCK_MAGIC);
	/* Setting Buswidth for controller */
	MLC_WRITE(this, MLC_ICR,
		  MLC_ICR_WIDTH8 | MLC_ICR_AWC3 |
		  MLC_ICR_LARGEBLK | MLC_ICR_WP_DISABLED);

	pnx4008_gpio_write_pin(GPO_01, 1);

	/* disable /WP by writing to MLC_WP */
	MLC_WRITE(this, MLC_LOCK_PR, MLC_UNLOCK_MAGIC);
	MLC_WRITE(this, MLC_WP, 1);

	MLC_WRITE(this, MLC_LOCK_PR, MLC_UNLOCK_MAGIC);
	MLC_WRITE(this, MLC_TIME, FLASH_MLC_TIME_tWP(4) | FLASH_MLC_TIME_tWH(7) | FLASH_MLC_TIME_tRP(6) | FLASH_MLC_TIME_tREH(2) | FLASH_MLC_TIME_tRHZ(3) | FLASH_MLC_TIME_tRWB(3) | FLASH_MLC_TIME_tCEA(0));	/* 104 Hz */
	/* wait 6 uSeconds for device to become ready */
	udelay(6);

	printk("Sandisk MLC controller on pnx4008, %s mode, %s ECC\n",
	       mlc_flash_dma ? "DMA" : "PIO",
	       mlc_hwecc ? "hardware" : "software");

	if (nand_scan(mtd, 1)) {
		goto out_1;
	}
#ifdef CONFIG_MTD_PARTITIONS
	err = parse_mtd_partitions(mtd, part_probes, &dynamic_partition, 0);
	if (err > 0)
		err = add_mtd_partitions(mtd, dynamic_partition, err);
	else
		err = add_mtd_partitions(mtd,
					 static_partition,
					 ARRAY_SIZE(static_partition));
	if (err < 0) {
		goto out_2;
	}
#endif
	dev_set_drvdata(dev, mtd);
	return 0;

#ifdef CONFIG_MTD_PARTITIONS
out_2:
	nand_release(mtd);
#endif

out_1:
	kfree(pnx4008_nand_data);
out:
	return err;
}

static int __exit pnx4008_nand_remove(struct device *dev)
{
	dev_set_drvdata(dev, NULL);

	nand_release(&pnx4008_nand_data->mtd);
	if (pnx4008_nand_data->dma >= 0) {
		pnx4008_free_channel(pnx4008_nand_data->dma);
	}
	kfree(pnx4008_nand_data);

	__raw_writel(0, PNX4008_FLASHCLK_CTRL_REG);

	return 0;
}

static struct device_driver pnx4008_nand_driver = {
	.name = "pnx4008-flash",
	.bus = &platform_bus_type,
	.probe = pnx4008_nand_probe,
	.remove = __exit_p(pnx4008_nand_remove),
	.suspend = pnx4008_nand_suspend,
	.resume = pnx4008_nand_resume,
};

static int __init pnx4008_nand_init(void)
{
	return driver_register(&pnx4008_nand_driver);
}

static void __exit pnx4008_nand_exit(void)
{
	driver_unregister(&pnx4008_nand_driver);
}

module_exit(pnx4008_nand_exit);
module_init(pnx4008_nand_init);

static int pnx4008_nand_ready(struct mtd_info *mtd)
{
	const u32 mask = MLC_ISR_READY;
	u32 rd;

	rd = MLC_READ(mtd->priv, MLC_ISR);
	DEBUG(MTD_DEBUG_LEVEL3, "%s: rd = %08X\n", __FUNCTION__, rd);
	return ((rd & mask) == mask) ? 1 : 0;
}

static void pnx4008_nand_hwcontrol(struct mtd_info *mtd, int hwc)
{
}

static void pnx4008_nand_command(struct mtd_info *mtd, unsigned command,
				 int column, int page_addr)
{
	struct nand_chip *this = mtd->priv;
	int cnt = 0xFFF;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += MLC_DATA_PAGE;
		command = NAND_CMD_READ0;
	}

	/* Write out the command to the device. */
	DEBUG(MTD_DEBUG_LEVEL3, "Writing command '%x'\n", command);
	MLC_WRITE(this, MLC_CMD, command);

	if (column != -1 || page_addr != -1) {
		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (this->options & NAND_BUSWIDTH_16)
				column >>= 1;
			MLC_WRITE(this, MLC_ADDR, column & 0xff);
			MLC_WRITE(this, MLC_ADDR, (column >> 8) & 0xff);
		}
		if (page_addr != -1) {
			MLC_WRITE(this, MLC_ADDR, page_addr & 0xff);
			MLC_WRITE(this, MLC_ADDR, (page_addr >> 8) & 0xff);

			/* One more address cycle for devices > 128MiB */
			if (this->chipsize > (128 << 20)) {
				MLC_WRITE(this, MLC_ADDR,
					  (page_addr >> 16) & 0xff);
			}
		}
	}
	/*
	 * program and erase have their own busy handlers
	 * status and sequential in needs no delay
	 */
	switch (command) {
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
		return;
	case NAND_CMD_RESET:
		if (this->dev_ready)
			break;
		udelay(this->chip_delay);
		MLC_WRITE(this, MLC_CMD, NAND_CMD_STATUS);
		cnt = 0xFFF;
		while ((pnx4008_read_data(this) & 0x40) == 0 && cnt--)
			continue;
		return;
	case NAND_CMD_READ0:
		/* Write out the start read command */
		MLC_WRITE(this, MLC_CMD, NAND_CMD_READSTART);
		/* fall down */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */
		if (!this->dev_ready) {
			udelay(this->chip_delay);
			return;
		}
	}
	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);
	/* wait until command is processed */
	cnt = 0xFFF;
	while (!this->dev_ready(mtd) && cnt--)
		continue;
}

static void pnx4008_enable_hwecc(struct mtd_info *mtd, int ecc_mode)
{
	struct nand_chip *this = mtd->priv;
	struct pnx4008_mtd_s *data = this->priv;
	int cnt = 0xFFF;

	data->hwecc_mode = ecc_mode;
	switch (data->hwecc_mode) {
	case NAND_ECC_READ:
		while (!this->dev_ready(mtd) && cnt--)
			continue;
		MLC_WRITE(this, MLC_ECC_DEC, 1);	/* start decode cycle */
		break;
	case NAND_ECC_WRITESYN:
		MLC_WRITE(this, MLC_ECC_ENC, 0xFF);
		while (!this->dev_ready(mtd) && cnt--)
			continue;
	case NAND_ECC_WRITE:
		break;
	}
}

static int pnx4008_calculate_ecc(struct mtd_info *mtd, const u_char * bytes,
				 u_char * ecc_code)
{
	return 0;
}

static inline int pnx4008_nand_ecc_ready(struct mtd_info *mtd)
{
	return ((MLC_READ(mtd->priv, MLC_ISR) & MLC_ISR_READY) ==
		MLC_ISR_READY);
}

static int pnx4008_correct_data(struct mtd_info *mtd, u_char * bytes,
				u_char * read_ecc, u_char * calc_ecc)
{
	struct nand_chip *this = mtd->priv;
	struct pnx4008_mtd_s *data = this->priv;
	u32 status;
	static u8 page[MLC_PAGE];	/* FIXME: hardcoded constant */
	int cnt = 0xFFF;

	if (data->hwecc_mode != NAND_ECC_READSYN)
		return -EINVAL;

	do {
		status = MLC_READ(this, MLC_ISR);
	} while ((status & MLC_ISR_ECCREADY) != MLC_ISR_ECCREADY && cnt--);

	if (status & MLC_ISR_ERRORS) {
		DEBUG(MTD_DEBUG_LEVEL3, "%s: errors found, correcting...\n", __FUNCTION__);
		data->hwecc_mode = NAND_ECC_READ;
		this->read_buf(mtd, page, sizeof(page));
		memcpy(bytes, page, MLC_DATA_PAGE);
		memcpy(read_ecc, page + MLC_DATA_PAGE, MLC_ECC_SIZE);
	} else {
		data->hwecc_mode = NAND_ECC_READ;
		this->read_buf(mtd, page, MLC_ECC_SIZE);
	}

	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dmitry pervushin <dpervushin@ru.mvista.com>");
MODULE_PARM(mlc_flash_dma, "i");
MODULE_PARM(mlc_hwecc, "i");
