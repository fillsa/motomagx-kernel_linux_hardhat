/*
 * linux/drivers/ide/arm/mxc_ide.c
 *
 * Based on Simtec BAST IDE driver
 * Copyright (c) 2003-2004 Simtec Electronics
 *  Ben Dooks <ben@simtec.co.uk>
 *
 * Copyright 2004-2005 Freescale Semiconductor, Inc. All Rights Reserved.
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
 *
 */

/*!
 * @file mxc_ide.c
 *
 * @brief ATA driver
 *
 * @ingroup ATA
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/ide.h>
#include <linux/init.h>

#include <asm/mach-types.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/hardware.h>
#include <asm/arch/board.h>
#include <asm/arch/clock.h>
#include <asm/arch/sdma.h>
#include "mxc_ide.h"

extern void gpio_ata_active(void);
extern void gpio_ata_inactive(void);
static int mxc_ide_config_drive(ide_drive_t * drive, u8 xfer_mode);

/* List of registered interfaces */
static ide_hwif_t *ifs[1];

/*! Private data for the drive structure. */
typedef struct {
	struct device *dev;	/*!< The device */
	int dma_read_chan;	/*!< DMA channel sdma api gave us for reads */
	int dma_write_chan;	/*!< DMA channel sdma api gave us for writes */
	int ultra;		/*!< Remember when we're in ultra mode */
	int nr_bd;		/*!< How many BDs currently active if SDMA */
	u8 enable;		/*!< Current hardware interrupt mask */
} mxc_ide_private_t;

/*! ATA transfer mode for set_ata_bus_timing() */
enum ata_mode {
	PIO,			/*!< Specifies PIO mode */
	MDMA,			/*!< Specifies MDMA mode */
	UDMA			/*!< Specifies UDMA mode */
};

/*
 * This structure contains the timing parameters for
 * ATA bus timing in the 5 PIO modes.  The timings
 * are in nanoseconds, and are converted to clock
 * cycles before being stored in the ATA controller
 * timing registers.
 */
static struct {
	short t0, t1, t2_8, t2_16, t2i, t4, t9, tA;
} pio_specs[] = {
	[0] = {
	.t0 = 600,.t1 = 70,.t2_8 = 290,.t2_16 = 165,.t2i = 0,.t4 =
		    30,.t9 = 20,.tA = 50},[1] = {
	.t0 = 383,.t1 = 50,.t2_8 = 290,.t2_16 = 125,.t2i = 0,.t4 =
		    20,.t9 = 15,.tA = 50},[2] = {
	.t0 = 240,.t1 = 30,.t2_8 = 290,.t2_16 = 100,.t2i = 0,.t4 =
		    15,.t9 = 10,.tA = 50},[3] = {
	.t0 = 180,.t1 = 30,.t2_8 = 80,.t2_16 = 80,.t2i = 0,.t4 =
		    10,.t9 = 10,.tA = 50},[4] = {
	.t0 = 120,.t1 = 25,.t2_8 = 70,.t2_16 = 70,.t2i = 0,.t4 =
		    10,.t9 = 10,.tA = 50}
};

#define NR_PIO_SPECS (sizeof pio_specs / sizeof pio_specs[0])

/*
 * This structure contains the timing parameters for
 * ATA bus timing in the 3 MDMA modes.  The timings
 * are in nanoseconds, and are converted to clock
 * cycles before being stored in the ATA controller
 * timing registers.
 */
static struct {
	short t0M, tD, tH, tJ, tKW, tM, tN, tJNH;
} mdma_specs[] = {
	[0] = {
	.t0M = 480,.tD = 215,.tH = 20,.tJ = 20,.tKW = 215,.tM = 50,.tN =
		    15,.tJNH = 20},[1] = {
	.t0M = 150,.tD = 80,.tH = 15,.tJ = 5,.tKW = 50,.tM = 30,.tN =
		    10,.tJNH = 15},[2] = {
	.t0M = 120,.tD = 70,.tH = 10,.tJ = 5,.tKW = 25,.tM = 25,.tN =
		    10,.tJNH = 10}
};

#define NR_MDMA_SPECS (sizeof mdma_specs / sizeof mdma_specs[0])

/*
 * This structure contains the timing parameters for
 * ATA bus timing in the 6 UDMA modes.  The timings
 * are in nanoseconds, and are converted to clock
 * cycles before being stored in the ATA controller
 * timing registers.
 */
static struct {
	short t2CYC, tCYC, tDS, tDH, tDVS, tDVH, tCVS, tCVH, tFS_min, tLI_max,
	    tMLI, tAZ, tZAH, tENV_min, tSR, tRFS, tRP, tACK, tSS, tDZFS;
} udma_specs[] = {
	[0] = {
	.t2CYC = 235,.tCYC = 114,.tDS = 15,.tDH = 5,.tDVS = 70,.tDVH =
		    6,.tCVS = 70,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 50,.tRFS = 75,.tRP = 160,.tACK = 20,.tSS =
		    50,.tDZFS = 80},[1] = {
	.t2CYC = 156,.tCYC = 75,.tDS = 10,.tDH = 5,.tDVS = 48,.tDVH =
		    6,.tCVS = 48,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 30,.tRFS = 70,.tRP = 125,.tACK = 20,.tSS =
		    50,.tDZFS = 63},[2] = {
	.t2CYC = 117,.tCYC = 55,.tDS = 7,.tDH = 5,.tDVS = 34,.tDVH =
		    6,.tCVS = 34,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 20,.tRFS = 60,.tRP = 100,.tACK = 20,.tSS =
		    50,.tDZFS = 47},[3] = {
	.t2CYC = 86,.tCYC = 39,.tDS = 7,.tDH = 5,.tDVS = 20,.tDVH =
		    6,.tCVS = 20,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 20,.tRFS = 60,.tRP = 100,.tACK = 20,.tSS =
		    50,.tDZFS = 35},[4] = {
	.t2CYC = 57,.tCYC = 25,.tDS = 5,.tDH = 5,.tDVS = 7,.tDVH =
		    6,.tCVS = 7,.tCVH = 6,.tFS_min = 0,.tLI_max =
		    100,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 50,.tRFS = 60,.tRP = 100,.tACK = 20,.tSS =
		    50,.tDZFS = 25},[5] = {
	.t2CYC = 38,.tCYC = 17,.tDS = 4,.tDH = 5,.tDVS = 5,.tDVH =
		    6,.tCVS = 10,.tCVH = 10,.tFS_min = 0,.tLI_max =
		    75,.tMLI = 20,.tAZ = 10,.tZAH = 20,.tENV_min =
		    20,.tSR = 20,.tRFS = 50,.tRP = 85,.tACK = 20,.tSS =
		    50,.tDZFS = 40}
};

#define NR_UDMA_SPECS (sizeof udma_specs / sizeof udma_specs[0])

/*!
 * Calculate values for the ATA bus timing registers and store
 * them into the hardware.
 *
 * @param       mode        Selects PIO, MDMA or UDMA modes
 *
 * @param       speed       Specifies the sub-mode number
 *
 * @return      EINVAL      speed out of range, or illegal mode
 */
static int set_ata_bus_timing(int speed, enum ata_mode mode)
{
	/* get the bus clock cycle time, in ns */

	int T = 2 * 1000 * 1000 * 1000 / mxc_get_clocks(AHB_CLK);

	/* every mode gets the same t_off and t_on */

	__raw_writeb(3, MXC_IDE_TIME_OFF);
	__raw_writeb(3, MXC_IDE_TIME_ON);

	switch (mode) {
	case PIO:
		if (speed < 0 || speed >= NR_PIO_SPECS) {
			return -EINVAL;
		}
		__raw_writeb((pio_specs[speed].t1 + T) / T, MXC_IDE_TIME_1);
		__raw_writeb((pio_specs[speed].t2_8 + T) / T, MXC_IDE_TIME_2w);

		__raw_writeb((pio_specs[speed].t2_8 + T) / T, MXC_IDE_TIME_2r);
		__raw_writeb((pio_specs[speed].tA + T) / T + 2, MXC_IDE_TIME_AX);
		__raw_writeb(1, MXC_IDE_TIME_PIO_RDX);
		__raw_writeb((pio_specs[speed].t4 + T) / T, MXC_IDE_TIME_4);

		__raw_writeb((pio_specs[speed].t9 + T) / T, MXC_IDE_TIME_9);

		break;
	case MDMA:
		if (speed < 0 || speed >= NR_MDMA_SPECS) {
			return -EINVAL;
		}
		__raw_writeb((mdma_specs[speed].tM + T) / T, MXC_IDE_TIME_M);
		__raw_writeb((mdma_specs[speed].tJNH + T) / T, MXC_IDE_TIME_JN);
		__raw_writeb((mdma_specs[speed].tD + T) / T, MXC_IDE_TIME_D);

		__raw_writeb((mdma_specs[speed].tKW + T) / T, MXC_IDE_TIME_K);

		break;
	case UDMA:
		if (speed < 0 || speed >= NR_UDMA_SPECS) {
			return -EINVAL;
		}
		__raw_writeb((udma_specs[speed].tACK + T) / T, MXC_IDE_TIME_ACK);
		__raw_writeb((udma_specs[speed].tENV_min + T) / T, MXC_IDE_TIME_ENV);
		__raw_writeb((udma_specs[speed].tRP + T) / T + 2, MXC_IDE_TIME_RPX);

		__raw_writeb((udma_specs[speed].tZAH + T) / T, MXC_IDE_TIME_ZAH);
		__raw_writeb((udma_specs[speed].tMLI + T) / T, MXC_IDE_TIME_MLIX);
		__raw_writeb((udma_specs[speed].tDVH + T) / T, MXC_IDE_TIME_DVH);
		__raw_writeb((udma_specs[speed].tDZFS + T) / T, MXC_IDE_TIME_DZFS);

		__raw_writeb((udma_specs[speed].tDVS + T) / T, MXC_IDE_TIME_DVS);
		__raw_writeb((udma_specs[speed].tCVH + T) / T, MXC_IDE_TIME_CVH);
		__raw_writeb((udma_specs[speed].tSS + T) / T, MXC_IDE_TIME_SS);
		__raw_writeb((udma_specs[speed].tCYC + T) / T, MXC_IDE_TIME_CYC);

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*!
 * Placeholder for code to make any hardware tweaks
 * necessary to select a drive.  Currently we are
 * not aware of any.
 */
static void mxc_ide_selectproc(ide_drive_t * drive)
{
	return;
}

/*!
 * Called to set the PIO mode.
 *
 * @param   pio    Specifies the PIO mode number desired
 */
static void mxc_ide_tune(ide_drive_t * drive, u8 pio)
{
	set_ata_bus_timing(pio, PIO);
}

/*!
 * Hardware-specific interrupt service routine for the ATA driver,
 * called mainly just to dismiss the interrupt at the hardware, and
 * to indicate whether there actually was an interrupt pending.
 *
 * The generic IDE related interrupt handling is done by the IDE layer.
 */
static int mxc_ide_ack_intr(struct hwif_s *hw)
{
	unsigned char status = __raw_readb(MXC_IDE_INTR_PENDING);
	unsigned char enable = __raw_readb(MXC_IDE_INTR_ENABLE);

	/*
	 * The only interrupts we can actually dismiss are the FIFO conditions.
	 * INTRQ comes from the bus, and must be dismissed according to IDE
	 * protocol, which will be done by the IDE layer, even when SDMA
	 * is invovled (SDMA can see it, but can't dismiss it).
	 */
	__raw_writeb(status, MXC_IDE_INTR_CLEAR);

	if (status & enable & ~MXC_IDE_INTR_ATA_INTRQ2) {
		printk(KERN_ERR "mxc_ide_ack_intr: unexpected interrupt, "
		       "status=0x%02X\n", status);
	}

	return status ? 1 : 0;
}

/*!
 * Decodes the specified transfer mode and sets both timing and ultra modes
 *
 * @param       drive       Specifies the drive
 *
 * @param       xfer_mode   Specifies the desired transfer mode
 *
 * @return      EINVAL      Illegal mode specified
 */
static int mxc_ide_set_speed(ide_drive_t * drive, u8 xfer_mode)
{
	mxc_ide_private_t *priv = (mxc_ide_private_t *) HWIF(drive)->hwif_data;

	switch (xfer_mode) {
	case XFER_UDMA_7:
	case XFER_UDMA_6:
	case XFER_UDMA_5:
	case XFER_UDMA_4:
	case XFER_UDMA_3:
	case XFER_UDMA_2:
	case XFER_UDMA_1:
	case XFER_UDMA_0:
		priv->ultra = 1;
		return set_ata_bus_timing(xfer_mode - XFER_UDMA_0, UDMA);
		break;
	case XFER_MW_DMA_2:
	case XFER_MW_DMA_1:
	case XFER_MW_DMA_0:
		priv->ultra = 0;
		return set_ata_bus_timing(xfer_mode - XFER_MW_DMA_0, MDMA);
		break;
	case XFER_PIO_4:
	case XFER_PIO_3:
	case XFER_PIO_2:
	case XFER_PIO_1:
	case XFER_PIO_0:
		return set_ata_bus_timing(xfer_mode - XFER_PIO_0, PIO);
		break;
	}

	return -EINVAL;
}

/*!
 * Called when the IDE layer is disabling DMA on a drive.
 *
 * @param       drive       Specifies the drive
 *
 * @return      0 if successful
 */
static int mxc_ide_dma_off_quietly(ide_drive_t * drive)
{
	drive->using_dma = 0;

	return 0;
}

/*!
 * Called by the IDE layer when something goes wrong
 *
 * @param       drive       Specifies the drive
 *
 */
static void mxc_ide_resetproc(ide_drive_t * drive)
{
	printk("%s: resetting ATA controller\n", __func__);

	__raw_writeb(0x00, MXC_IDE_ATA_CONTROL);
	udelay(100);
	__raw_writeb(MXC_IDE_CTRL_ATA_RST_B, MXC_IDE_ATA_CONTROL);
	udelay(100);
}

/*!
 * Turn on DMA for a drive.
 *
 * @param       drive       Specifies the drive
 *
 * @return      0 if successful
 */
static int mxc_ide_dma_on(ide_drive_t * drive)
{
	/* consult the list of known "bad" drives */
	if (__ide_dma_bad_drive(drive))
		return 1;

	/*
	 * Go to UDMA3 mode.  Beyond that you'll no doubt
	 * need an Ultra-100 cable (the 80 pin type with
	 * ground leads between all the signals)
	 */
	printk(KERN_ERR "%s: enabling UDMA3 mode\n", drive->name);
	mxc_ide_config_drive(drive, XFER_UDMA_3);
	drive->using_dma = 1;

	blk_queue_max_hw_segments(drive->queue, MXC_IDE_SDMA_BD_NR);
	blk_queue_max_hw_segments(drive->queue, MXC_IDE_SDMA_BD_NR);
	blk_queue_max_segment_size(drive->queue, MXC_IDE_SDMA_BD_SIZE_MAX);

	if (HWIF(drive)->ide_dma_host_on(drive))
		return 1;

	return 0;
}

/*!
 * Turn on DMA if the drive can handle it.
 *
 * @param       drive       Specifies the drive
 *
 * @return      0 if successful
 */
static int mxc_ide_dma_check(ide_drive_t * drive)
{
	struct hd_driveid *id = drive->id;

	if (id && (id->capability & 1)) {
		/*
		 * Enable DMA on any drive that has
		 * UltraDMA (mode 0/1/2/3/4/5/6) enabled
		 */
		if ((id->field_valid & 4) && ((id->dma_ultra >> 8) & 0x7f))
			return mxc_ide_dma_on(drive);
		/*
		 * Enable DMA on any drive that has mode2 DMA
		 * (multi or single) enabled
		 */
		if (id->field_valid & 2)	/* regular DMA */
			if ((id->dma_mword & 0x404) == 0x404 ||
			    (id->dma_1word & 0x404) == 0x404)
				return mxc_ide_dma_on(drive);

		/* Consult the list of known "good" drives */
		if (__ide_dma_good_drive(drive))
			return mxc_ide_dma_on(drive);
	}

	return mxc_ide_dma_off_quietly(drive);
}

/*!
 * The DMA is done, and the drive is done.  We'll check the BD array for
 * errors, and unmap the scatter-gather list.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 means all is well, 1 means SDMA signalled an error on one
 *              or more BDs, 2 means SDMA didn't signal any errors, but one
 *              or more BDs came back with BD_DONE set, which implies SDMA
 *              didn't process the BD.
 */
static int mxc_ide_dma_end(ide_drive_t * drive)
{
	struct request *rq = HWGROUP(drive)->rq;
	int i;
	ide_hwif_t *hwif = HWIF(drive);
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;
	int chan = rq_data_dir(rq) ? priv->dma_write_chan : priv->dma_read_chan;
	int dma_stat = 0;

	for (i = 0; i < priv->nr_bd; i++) {
		dma_request_t dma_cfg;

		mxc_dma_get_config(chan, &dma_cfg, i);

		if (dma_cfg.bd_done || dma_cfg.bd_error) {
			printk(KERN_ERR "%s: DMA fragment %d anomally, "
			       "done=%d, error=%d\n", drive->name, i,
			       dma_cfg.bd_done, dma_cfg.bd_error);
			if (dma_stat != 1)
				dma_stat = dma_cfg.bd_error ? 1 : 2;
		}
	}

	BUG_ON(drive->waiting_for_dma == 0);
	drive->waiting_for_dma = 0;

	/*
	 * We'll unmap the sg table regardless of status.
	 */
	dma_unmap_sg(priv->dev, hwif->sg_table, hwif->sg_nents,
		     hwif->sg_dma_direction);

	return dma_stat;
}

/*!
 * The end-of-DMA interrupt handler
 *
 * @param       drive       Specifies the drive
 *
 * @return      ide_stopped or ide_started
 */
static ide_startstop_t mxc_ide_dma_intr(ide_drive_t * drive)
{
	u8 stat, dma_stat;
	struct request *rq = HWGROUP(drive)->rq;
	ide_hwif_t *hwif = HWIF(drive);
	u8 fifo_fill;

	if (!rq)
		return ide_stopped;

	fifo_fill = __raw_readb(MXC_IDE_FIFO_FILL);
	BUG_ON(fifo_fill);

	dma_stat = mxc_ide_dma_end(drive);
	stat = hwif->INB(IDE_STATUS_REG);	/* get drive status */
	if (OK_STAT(stat, DRIVE_READY, drive->bad_wstat | DRQ_STAT)) {
		if (!dma_stat) {
			DRIVER(drive)->end_request(drive, 1, rq->nr_sectors);
			return ide_stopped;
		}
		printk(KERN_ERR "%s: mxc_ide_dma_intr: bad DMA status (0x%x)\n",
		       drive->name, dma_stat);
	}

	return DRIVER(drive)->error(drive, "mxc_ide_dma_intr", stat);
}

typedef enum {
	/*! Enable ATA_INTRQ on the CPU */
	INTRQ_MCU,

	/*! Enable ATA_INTRQ on the SDMA engine */
	INTRQ_SDMA
} intrq_which_t;

/*!
 * Directs the IDE INTRQ signal to SDMA or to the CPU
 *
 * @param       hwif       Specifies the IDE controller
 *
 * @param       which       \b INTRQ_SDMA or \b INTRQ_MCU
 *
 */
static void mxc_ide_set_intrq(ide_hwif_t * hwif, intrq_which_t which)
{
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;
	unsigned char enable = __raw_readb(MXC_IDE_INTR_ENABLE);

	switch (which) {
	case INTRQ_SDMA:
		enable &= ~MXC_IDE_INTR_ATA_INTRQ2;
		enable |= MXC_IDE_INTR_ATA_INTRQ1;
		break;
	case INTRQ_MCU:
		enable &= ~MXC_IDE_INTR_ATA_INTRQ1;
		enable |= MXC_IDE_INTR_ATA_INTRQ2;
		break;
	}
	priv->enable = enable;

	__raw_writeb(enable, MXC_IDE_INTR_ENABLE);
}

/*!
 * Helper routine to configure drive speed
 *
 * @param       drive       Specifies the drive
 *
 * @param       xfer_mode   Specifies the desired transfer mode
 *
 * @return      0 = success, non-zero otherwise
 */
static int mxc_ide_config_drive(ide_drive_t * drive, u8 xfer_mode)
{
	int err;
	ide_hwif_t *hwif = HWIF(drive);
	mxc_ide_private_t *priv = (mxc_ide_private_t *) (hwif->hwif_data);
	u8 prev = priv->enable;

	mxc_ide_set_speed(drive, xfer_mode);

	/*
	 * Work around an ADS hardware bug:
	 *
	 * OK, ide_config_drive_speed() is the right thing to call but it's
	 * going to leave an interrupt pending.  We have to jump through hoops
	 * because the ADS board doesn't correctly terminate INTRQ.  Instead
	 * of pulling it low, it pulls it high (asserted), so when the drive
	 * tri-states it, the MX31 sees it as an interrupt.  So we'll disable
	 * the hardware interrupt here, and restore it a bit later, after we've
	 * selected the drive again and let it drive INTRQ low for a bit.
	 *
	 * On later ADS boards, when presumably the correct INTRQ termination
	 * is in place, these extra hoops won't be necessary, but they
	 * shouldn't hurt, either.
	 */
	priv->enable = 0;
	__raw_writeb(0, MXC_IDE_INTR_ENABLE);

	err = ide_config_drive_speed(drive, xfer_mode);

	if (__raw_readb(MXC_IDE_INTR_PENDING) &
	    (MXC_IDE_INTR_ATA_INTRQ2 | MXC_IDE_INTR_ATA_INTRQ1)) {
		/*
		 * OK, the 'bad thing' is happening, so we'll clear nIEN to
		 * get the drive to drive INTRQ, then we'll read status to
		 * clear any pending interrupt.  This sequence gets us by
		 * the spurious and stuck interrupt we see otherwise.
		 */
		SELECT_DRIVE(drive);
		udelay(2);
		hwif->OUTB(0, IDE_CONTROL_REG);
		udelay(2);
		hwif->INB(IDE_STATUS_REG);
		udelay(2);
	}

	priv->enable = prev;
	__raw_writeb(prev, MXC_IDE_INTR_ENABLE);

	return err;
}

/*!
 * Masks drive interrupts temporarily at the hardware level
 *
 * @param       drive       Specifies the drive
 *
 * @param       mask        1 = disable interrupts, 0 = re-enable
 *
 */
static void mxc_ide_maskproc(ide_drive_t * drive, int mask)
{
	mxc_ide_private_t *priv =
	    (mxc_ide_private_t *) (HWIF(drive)->hwif_data);
	BUG_ON(!priv);

	if (mask) {
		__raw_writeb(0, MXC_IDE_INTR_ENABLE);
	} else {
		__raw_writeb(priv->enable, MXC_IDE_INTR_ENABLE);
	}
}

/*!
 * SDMA completion callback routine.  This gets called after the SDMA request
 * has completed, and as a result of the BD_INTR flag on the buffer descriptor.
 * All we need to do here is return ownership of the drive's INTRQ signal back
 * to the CPU, which will immediately raise the interrupt to the IDE layer.
 *
 * @param       arg         The drive we're servicing
 *
 */
static void mxc_ide_dma_callback(void *arg)
{
	ide_hwif_t *hwif = HWIF((ide_drive_t *) arg);

	/*
	 * Redirect ata_intrq back to us instead of the SDMA.
	 */
	mxc_ide_set_intrq(hwif, INTRQ_MCU);
}

/*!
 * DMA set up.  This is called once per DMA request to the drive.  It converts
 * the scatter-gather list into the SDMA channel's buffer descriptor (BD) array
 * and prepares both the ATA controller and the SDMA engine for the DMA
 * transfer.
 *
 * @param       drive     The drive we're servicing
 *
 * @return      0 on success, non-zero otherwise
 */
static int mxc_ide_dma_setup(ide_drive_t * drive)
{
	struct request *rq = HWGROUP(drive)->rq;
	ide_hwif_t *hwif = HWIF(drive);
	struct scatterlist *sg = hwif->sg_table;
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;
	int dma_ultra = priv->ultra ? MXC_IDE_CTRL_DMA_ULTRA : 0;
	int i;
	int chan;
	u8 ata_control;
	u8 fifo_fill;

	BUG_ON(!rq);
	BUG_ON(!priv);
	BUG_ON(drive->waiting_for_dma);

	if (rq_data_dir(rq)) {
		chan = priv->dma_write_chan;
	} else {
		chan = priv->dma_read_chan;
	}

	/*
	 * Set up the SDMA interrupt callback
	 */
	mxc_dma_set_callback(chan, mxc_ide_dma_callback, (void *)drive);

	/*
	 * If the ATA FIFO isn't empty, we shouldn't even be here
	 */
	fifo_fill = __raw_readb(MXC_IDE_FIFO_FILL);
	BUG_ON(fifo_fill);

	/*
	 * Map the request's scatter-gather list into our SDMA
	 * channel's buffer descriptor (BD) array.
	 */
	ide_map_sg(drive, rq);

	hwif->sg_dma_direction = rq_data_dir(rq) ? DMA_TO_DEVICE :
	    DMA_FROM_DEVICE;

	hwif->sg_nents = dma_map_sg(priv->dev, sg, hwif->sg_nents,
				    hwif->sg_dma_direction);
	BUG_ON(!hwif->sg_nents);
	BUG_ON(hwif->sg_nents > MXC_IDE_SDMA_BD_NR);

	for (i = 0; i < hwif->sg_nents; i++) {
		dma_request_t dma_cfg;
		int last_sg = (i == hwif->sg_nents - 1);

		BUG_ON(!sg->dma_address);
		BUG_ON(!sg->length);
		BUG_ON(sg->length > MXC_IDE_SDMA_BD_SIZE_MAX);

		/*
		 * Configure an SMDA BD to handle the segment
		 */
		if (rq_data_dir(rq)) {
			/* write */
			dma_cfg.sourceAddr = (__u8 *) sg->dma_address;
		} else {
			/* read */
			dma_cfg.destAddr = (__u8 *) sg->dma_address;
		}
		dma_cfg.count = sg->length;
		dma_cfg.bd_done = 1;	// makes SDMA owner
		dma_cfg.bd_cont = !last_sg;
		mxc_dma_set_config(chan, &dma_cfg, i);

		sg++;
	}

	priv->nr_bd = i;

	/*
	 * Prepare the ATA controller for the DMA
	 */
	if (rq_data_dir(rq)) {
		/* write */
		ata_control = MXC_IDE_CTRL_FIFO_RST_B |
		    MXC_IDE_CTRL_ATA_RST_B |
		    MXC_IDE_CTRL_FIFO_TX_EN |
		    MXC_IDE_CTRL_DMA_PENDING |
		    dma_ultra | MXC_IDE_CTRL_DMA_WRITE;
	} else {
		/* read */
		ata_control = MXC_IDE_CTRL_FIFO_RST_B |
		    MXC_IDE_CTRL_ATA_RST_B |
		    MXC_IDE_CTRL_FIFO_RCV_EN |
		    MXC_IDE_CTRL_DMA_PENDING | dma_ultra;
	}

	__raw_writeb(ata_control, MXC_IDE_ATA_CONTROL);
	__raw_writeb(MXC_IDE_SDMA_WATERMARK / 2, MXC_IDE_FIFO_ALARM);

	/*
	 * Route ata_intrq to the SDMA engine, and not to us.
	 */
	mxc_ide_set_intrq(hwif, INTRQ_SDMA);

	/*
	 * The SDMA and ATA controller are ready to go.
	 * mxc_ide_dma_start() will start the SDMA script,
	 * and mxc_ide_dma_exec_cmd() will tickle the drive, which
	 * actually initiates the DMA transfer on the ATA bus.
	 * The ATA controller is DMA slave for both read and write.
	 */
	BUG_ON(drive->waiting_for_dma);
	drive->waiting_for_dma = 1;

	return 0;
}

/*!
 * DMA timeout notification.  This gets called when the IDE layer above
 * us times out waiting for a request.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 to attempt recovery, otherwise, an additional tick count
 *              to keep waiting
 */
static int mxc_ide_dma_timer_expiry(ide_drive_t * drive)
{
	printk(KERN_ERR "%s: fifo_fill=%d\n", drive->name,
	       __raw_readb(MXC_IDE_FIFO_FILL));

	return 0;
}

/*!
 * Called by the IDE layer to start a DMA request on the specified drive.
 * The request has already been prepared by \b mxc_ide_dma_setup().  All
 * we need to do is pass the command through while specifying our timeout
 * handler.
 *
 * @param       drive       The drive we're servicing
 *
 * @param       cmd         The IDE command for the drive
 *
 */
static void mxc_ide_dma_exec_cmd(ide_drive_t * drive, u8 cmd)
{
	ide_execute_command(drive, cmd, mxc_ide_dma_intr, 2 * WAIT_CMD,
			    mxc_ide_dma_timer_expiry);
}

/*!
 * Called by the IDE layer to start the DMA controller.  The request has
 * already been prepared by \b mxc_ide_dma_setup().  All we do here
 * is tickle the SDMA channel.
 *
 * @param       drive       The drive we're servicing
 *
 */
static void mxc_ide_dma_start(ide_drive_t * drive)
{
	struct request *rq = HWGROUP(drive)->rq;
	ide_hwif_t *hwif = HWIF(drive);
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;
	int chan = rq_data_dir(rq) ? priv->dma_write_chan : priv->dma_read_chan;

	BUG_ON(!chan);

	/*
	 * Tickle the SDMA channel.  This starts the channel, but it is likely
	 * that SDMA will yield and go idle before the DMA request arrives from
	 * the drive.  Nonetheless, at least the context will be hot.
	 */
	mxc_dma_start(chan);
}

/*!
 * There is a race between the DMA interrupt and the timeout interrupt.  This
 * gets called during the IDE layer's timeout interrupt to see if the DMA
 * interrupt has also occured or is pending.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      1 means there is a DMA interrupt pending, 0 otherwise
 */
static int mxc_ide_dma_test_irq(ide_drive_t * drive)
{
	unsigned char status = __raw_readb(MXC_IDE_INTR_PENDING);

	/*
	 * We need to test the interrupt status without dismissing any.
	 */

	return status & (MXC_IDE_INTR_ATA_INTRQ1
			 | MXC_IDE_INTR_ATA_INTRQ2) ? 1 : 0;
}

/*!
 * Called to turn off DMA on this drive's controller, such as when a DMA error
 * occured and the IDE layer wants to fail back to PIO mode.  We won't do
 * anything special, leaving the SDMA channel allocated for future attempts.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 if successful, non-zero otherwise
 */
static int mxc_ide_dma_host_off(ide_drive_t * drive)
{
	return 0;
}

/*!
 * Called to turn on DMA on this drive's controller.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 if successful, non-zero otherwise
 */
static int mxc_ide_dma_host_on(ide_drive_t * drive)
{
	return 0;
}

/*!
 * Called for special handling on timeouts.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 if successful, non-zero otherwise
 */
static int mxc_ide_dma_timeout(ide_drive_t * drive)
{
	return 0;
}

/*!
 * Called for special handling on lost irq's.
 *
 * @param       drive       The drive we're servicing
 *
 * @return      0 if successful, non-zero otherwise
 */
static int mxc_ide_dma_lost_irq(ide_drive_t * drive)
{
	return 0;
}

/*!
 * Called once per controller to set up DMA
 *
 * @param       hwif       Specifies the IDE controller
 *
 */
static void mxc_ide_dma_init(ide_hwif_t * hwif)
{
	int err;
	mxc_ide_private_t *priv = (mxc_ide_private_t *) hwif->hwif_data;
	dma_channel_params read_params = {
		.peripheral_type = ATA,
		.transfer_type = per_2_emi,
		.word_size = TRANSFER_32BIT,
		.watermark_level = MXC_IDE_SDMA_WATERMARK,
		.bd_number = MXC_IDE_SDMA_BD_NR,
		/*
		 * We'll set the callback later, when we know which
		 * drive is being accessed.
		 */
		.callback = NULL,
		.arg = NULL,
		/*
		 * per_address for reads points to the base of the
		 * ATA register file.  The ata_2_mcu script adds 0x18 to
		 * get to the FIFO register.  By the way, only the
		 * bottom 18 bits matter, because the script converts
		 * it to a 16-bit internal SDMA address by shifting
		 * the and masking.
		 */
		.per_address = (__u32) ATA_DMA_BASE_ADDR,
		/*
		 * event_id2 gets the fifo_alarm SDMA
		 * DMA request event number.  This event number gets
		 * converted into a mask that must go into R0 of the
		 * SDMA context for this channel.  R1 gets the other
		 * event_id.  ATA is special--other scripts only use R1.
		 */
		.event_id2 = DMA_REQ_ATA_RX,
		/*
		 * DMA_REQ_ATA_TX_END is actually shared between the
		 * two scripts, but only write really needs it.  In
		 * fact, it has been seen to occur at the beginning
		 * of a read, which confuses the ata_2_mcu SDMA script.
		 * So we'll just leave it out for reads, which will
		 * always be multiple sector-sized transfers.
		 */
		.event_id = 0,
	}, write_params = {
		.peripheral_type = ATA,.transfer_type = emi_2_per,.word_size =
		    TRANSFER_32BIT,.watermark_level =
		    MXC_IDE_SDMA_WATERMARK,.bd_number = MXC_IDE_SDMA_BD_NR,
		    /*
		     * We'll set the callback later, when we know which
		     * drive is being accessed.
		     */
		    .callback = NULL,.arg = NULL,
		    /*
		     * per_address for write, unlike read, needs to point
		     * directly to the ATA FIFO.  The mcu_2_ata script doesn't
		     * add the 0x18 like ata_2_mcu does.
		     */
		    .per_address = (__u32) ATA_DMA_BASE_ADDR + 0x18,
		    /*
		     * As with read, event_id2 gets the fifo_alarm SDMA
		     * DMA request event number.  This event number gets
		     * converted into a mask that must go into R0 of the
		     * SDMA context for this channel.  R1 gets the other
		     * event_id.  ATA is special--other scripts only use R1.
		     */
		    .event_id2 = DMA_REQ_ATA_TX,
		    /*
		     * Writes need DMA_REQ_ATA_TX_END to complete.
		     */
	.event_id = DMA_REQ_ATA_TX_END,};

	hwif->dmatable_cpu = NULL;
	hwif->dmatable_dma = 0;
	hwif->speedproc = mxc_ide_set_speed;
	hwif->resetproc = mxc_ide_resetproc;
	hwif->autodma = 0;

	/*
	 * Allocate the SDMA channels
	 */
	err = mxc_request_dma(&priv->dma_read_chan, "MXC ATA RX");
	if (err < 0) {
		printk(KERN_ERR "%s: couldn't get RX DMA channel, err=%d\n",
		       hwif->name, err);
		goto err_out;
	}
	err = mxc_request_dma(&priv->dma_write_chan, "MXC ATA TX");
	if (err < 0) {
		printk(KERN_ERR "%s: couldn't get TX DMA channel, err=%d\n",
		       hwif->name, err);
		goto err_out;
	}

	/*
	 * Set up SDMA channels.  These calls sleep.
	 */
	err = mxc_dma_setup_channel(priv->dma_read_chan, &read_params);
	if (err < 0) {
		printk(KERN_ERR "%s: couldn't set up read DMA chan, err=%d\n",
		       hwif->name, err);
		goto err_out;
	}
	err = mxc_dma_setup_channel(priv->dma_write_chan, &write_params);
	if (err < 0) {
		printk(KERN_ERR "%s: couldn't set up read DMA chan, err=%d\n",
		       hwif->name, err);
		goto err_out;
	}
	printk(KERN_INFO "%s: read chan=%d (%d BDs), write chan=%d (%d BDs)\n",
	       hwif->name,
	       priv->dma_read_chan, read_params.bd_number,
	       priv->dma_write_chan, write_params.bd_number);

	/*
	 * Allocate and set up the read SDMA channel
	 */
	set_ata_bus_timing(0, UDMA);

	/*
	 * All ready now
	 */
	hwif->atapi_dma = 1;
	hwif->ultra_mask = 0x7f;
	hwif->mwdma_mask = 0x07;
	hwif->swdma_mask = 0x07;
	hwif->udma_four = 1;

	hwif->ide_dma_off_quietly = mxc_ide_dma_off_quietly;
	hwif->ide_dma_on = mxc_ide_dma_on;
	hwif->ide_dma_check = mxc_ide_dma_check;
	hwif->dma_setup = mxc_ide_dma_setup;
	hwif->dma_exec_cmd = mxc_ide_dma_exec_cmd;
	hwif->dma_start = mxc_ide_dma_start;
	hwif->ide_dma_end = mxc_ide_dma_end;
	hwif->ide_dma_test_irq = mxc_ide_dma_test_irq;
	hwif->ide_dma_host_off = mxc_ide_dma_host_off;
	hwif->ide_dma_host_on = mxc_ide_dma_host_on;
	hwif->ide_dma_timeout = mxc_ide_dma_timeout;
	hwif->ide_dma_lostirq = mxc_ide_dma_lost_irq;

	hwif->hwif_data = (void *)priv;
	return;

      err_out:
	hwif->atapi_dma = 0;
	if (priv->dma_read_chan)
		mxc_free_dma(priv->dma_read_chan);
	if (priv->dma_write_chan)
		mxc_free_dma(priv->dma_write_chan);
	kfree(priv);
	return;
}

/*!
 * MXC-specific IDE registration helper routine.  Among other things,
 * we tell the IDE layer where our standard IDE drive registers are,
 * through the \b io_ports array.  The IDE layer sends commands to and
 * reads status directly from attached IDE drives through these drive
 * registers.
 *
 * @param   base   Base address of memory mapped IDE standard drive registers
 *
 * @param   aux    Address of the auxilliary ATA control register
 *
 * @param   irq    IRQ number for our hardware
 *
 * @param   hwifp  Pointer to hwif structure to be updated by the IDE layer
 *
 * @return  ENODEV if no drives are present, 0 otherwise
 */
static int __init
mxc_ide_register(unsigned long base, unsigned int aux, int irq,
		 ide_hwif_t ** hwifp, mxc_ide_private_t * priv)
{
	int i = 0;
	hw_regs_t hw;
	ide_hwif_t *hwif = &ide_hwifs[0];
	const int regsize = 4;

	memset(&hw, 0, sizeof(hw));

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw.io_ports[i] = (unsigned long)base;
		base += regsize;
	}
	hw.io_ports[IDE_CONTROL_OFFSET] = aux;
	hw.irq = irq;
	hw.ack_intr = &mxc_ide_ack_intr;

	*hwifp = hwif;
	ide_register_hw(&hw, hwifp);

	hwif->selectproc = &mxc_ide_selectproc;
	hwif->tuneproc = &mxc_ide_tune;
	hwif->maskproc = &mxc_ide_maskproc;
	hwif->hwif_data = (void *)priv;
	mxc_ide_set_intrq(hwif, INTRQ_MCU);

	/*
	 * The IDE layer will set hwif->present if we have devices attached,
	 * if we don't, discard the interface reset the ports.
	 */
	if (!hwif->present) {
		printk(KERN_INFO "ide%d: Bus empty, interface released.\n",
		       hwif->index);
		default_hwif_iops(hwif);
		for (i = IDE_DATA_OFFSET; i <= IDE_CONTROL_OFFSET; ++i)
			hwif->io_ports[i] = 0;
		hwif->chipset = ide_unknown;
		hwif->noprobe = 1;
		return -ENODEV;
	}

	mxc_ide_dma_init(hwif);

	for (i = 0; i < MAX_DRIVES; i++) {
		mxc_ide_dma_check(hwif->drives + i);
	}

	return 0;
}

/*!
 * Driver initialization routine.  Prepares the hardware for ATA activity.
 */
static int __init mxc_ide_init(void)
{
	int index = 0;
	mxc_ide_private_t *priv;

	printk("MXC: IDE driver, (c) 2004-2005 Freescale Semiconductor\n");

	__raw_writeb(MXC_IDE_CTRL_ATA_RST_B, MXC_IDE_ATA_CONTROL);

	/* Select group B pins, and enable the interface */

	__raw_writew(PBC_BCTRL2_ATA_SEL, PBC_BASE_ADDRESS + PBC_BCTRL2_SET);
	__raw_writew(PBC_BCTRL2_ATA_EN, PBC_BASE_ADDRESS + PBC_BCTRL2_CLEAR);

	/* Configure the pads */

	gpio_ata_active();

	/* Set initial timing and mode */

	set_ata_bus_timing(0, PIO);

	/* Reset the interface */

	mxc_ide_resetproc(NULL);

	/*
	 * Enable hardware interrupts.
	 * INTRQ2 goes to us, so we enable it here, but we'll need to ignore
	 * it when SDMA is doing the transfer.
	 */
	__raw_writeb(MXC_IDE_INTR_ATA_INTRQ2, MXC_IDE_INTR_ENABLE);

	/*
	 * Allocate a private structure
	 */
	priv = (mxc_ide_private_t *) kmalloc(sizeof *priv, GFP_KERNEL);
	if (priv == NULL)
		return ENOMEM;
	memset(priv, 0, sizeof *priv);
	priv->dev = NULL;	// dma_map_sg() ignores it anyway

	/*
	 * Now register
	 */

	index =
	    mxc_ide_register(IDE_ARM_IO, IDE_ARM_CTL, IDE_ARM_IRQ, &ifs[0],
			     priv);
	if (index == -1) {
		printk(KERN_ERR "Unable to register the MXC IDE driver\n");
		kfree(priv);
		return ENODEV;
	}
#ifdef ATA_USE_IORDY
	/* turn on IORDY protocol */

	udelay(25);
	__raw_writeb(MXC_IDE_CTRL_ATA_RST_B | MXC_IDE_CTRL_IORDY_EN,
	       MXC_IDE_ATA_CONTROL);
#endif

	return 0;
}

/*!
 * Driver exit routine.  Clean up.
 */
static void __exit mxc_ide_exit(void)
{
	ide_hwif_t *hwif = ifs[0];
	mxc_ide_private_t *priv;

	WARN_ON(!hwif);
	priv = (mxc_ide_private_t *) hwif->hwif_data;
	WARN_ON(!priv);

	/*
	 * Unregister the interface at the IDE layer.  This should shut
	 * down any drives and pending I/O.
	 */
	ide_unregister(hwif->index);

	/*
	 * Disable hardware interrupts.
	 */
	__raw_writeb(0, MXC_IDE_INTR_ENABLE);

	/*
	 * Deallocate DMA channels.  Free's BDs and everything.
	 */
	if (priv->dma_read_chan)
		mxc_free_dma(priv->dma_read_chan);
	if (priv->dma_write_chan)
		mxc_free_dma(priv->dma_write_chan);

	/* Disable the interface */

	__raw_writew(PBC_BCTRL2_ATA_EN, PBC_BASE_ADDRESS + PBC_BCTRL2_SET);

	/*
	 * Free the pins
	 */
	gpio_ata_inactive();

	/*
	 * Free the private structure.
	 */
	kfree(priv);
	hwif->hwif_data = NULL;

}

module_init(mxc_ide_init);
module_exit(mxc_ide_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION
    ("Freescale MX3 IDE (based on Simtec BAST IDE driver by Ben Dooks <ben@simtec.co.uk>)");
