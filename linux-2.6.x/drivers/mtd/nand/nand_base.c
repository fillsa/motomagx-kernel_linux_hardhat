/*
 *  drivers/mtd/nand.c
 
 *
 *  Overview:
 *   This is the generic MTD driver for NAND flash devices. It should be
 *   capable of working with almost all NAND chips currently available.
 *   Basic support for AG-AND chips is provided.
 *   
 *	Additional technical information is available on
 *	http://www.linux-mtd.infradead.org/tech/nand.html
 *	
 *  Copyright (C) 2005-2007 Motorola, Inc.
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 * 		  2002 Thomas Gleixner (tglx@linutronix.de)
 *
 *  02-08-2004  tglx: support for strange chips, which cannot auto increment 
 *		pages on read / read_oob
 *
 *  03-17-2004  tglx: Check ready before auto increment check. Simon Bayes
 *		pointed this out, as he marked an auto increment capable chip
 *		as NOAUTOINCR in the board driver.
 *		Make reads over block boundaries work too
 *
 *  04-14-2004	tglx: first working version for 2k page size chips
 *  
 *  05-19-2004  tglx: Basic support for Renesas AG-AND chips
 *
 *  09-24-2004  tglx: add support for hardware controllers (e.g. ECC) shared
 *		among multiple independend devices. Suggestions and initial patch
 *		from Ben Dooks <ben-mtd@fluff.org>
 *
 *  12-05-2004	dmarlin: add workaround for Renesas AG-AND chips "disturb" issue.
 *		Basically, any block not rewritten may lose data when surrounding blocks
 *		are rewritten many times.  JFFS2 ensures this doesn't happen for blocks 
 *		it uses, but the Bad Block Table(s) may not be rewritten.  To ensure they
 *		do not lose data, force them to be rewritten when some of the surrounding
 *		blocks are erased.  Rather than tracking a specific nearby block (which 
 *		could itself go bad), use a page address 'mask' to select several blocks 
 *		in the same area, and rewrite the BBT when any of them are erased.
 *
 *  01-03-2005	dmarlin: added support for the device recovery command sequence for Renesas 
 *		AG-AND chips.  If there was a sudden loss of power during an erase operation,
 * 		a "device recovery" operation must be performed when power is restored
 * 		to ensure correct operation.
 *
 *  01-20-2005	dmarlin: added support for optional hardware specific callback routine to 
 *		perform extra error status checks on erase and write failures.  This required
 *		adding a wrapper function for nand_read_ecc.
 *
 * 05-06-2005	feature CONFIG_MTD_NAND_BBM added by Motorola, Inc.
 *		added bad block management support. With the reserved block pool 
 *		available, nand_base driver will do bad block replacement based on
 *		the BBM map (Bad Block Map) stored in flash.
 *
 * 06-10-2005   feature CONFIG_MOT_FEAT_MTD_FS added by Motorola, Inc.
 *		added support to use string name for both mtd char and block device.
 *
 * 08-20-2005	vwool: suspend/resume added
 *
 * 11-01-2005   vwool: NAND page layouts introduces for HW ECC handling
 *
 * 04-10-2006   feature CONFIG_MOT_FEAT_NAND_RDDIST added by Motorola, Inc.
 *              the nand_base driver will work with nand_watchdog (user space daemon),
 *              while doing block read: automaticlly fix block if 1bit ECC error detected.
 *              while block fixing requested by nand_watchdog: the block will be fixed
 *              for read distrub recovery.
 *
 * 05-15-2006	feature CONFIG_MOT_FEAT_NAND_BLKCNT_TEST added by Motorola, Inc.
 *		add the capablity to do nand flash block access (read|write|erase)
 *		measurement.
 *
 * 08-20-2006	bug reported under WFN455, temporary fixed by Motorola, Inc.
 *
 * 09-26-2006	feature CONFIG_MOT_FEAT_KPANIC added by Motorola, Inc.
 *		Whenever the kernel panics, the printk ring buffer will be written
 *		out to a dedicated flash partition. 
 *
 * 10-20-2006	feature CONFIG_MOT_FEAT_MTD_AUTO_BBM added by Motorola, Inc.
 *		provides new API mtd->block_replace() to mtd upper layer and filesystem
 *		add auto BBM into nand block erase function.
 *
 * 01-03-2007	Motorola: adding FL_RDDIST_FIXING state into CONFIG_MOT_FEAT_NAND_RDDIST
 *		make sure the execution of read disturb fixing is a fully atomic process.
 *
 * 04-15-2007   Motorola: update nand_read_distfix function and add RDDIST fix reason code.
 *
 * 06-15-2007   Motorola: update read disturb max value for threshold from 2^8 to 2^16.
 *
 * Credits:
 *	David Woodhouse for adding multichip support  
 *	
 *	Aleph One Ltd. and Toby Churchill Ltd. for supporting the
 *	rework for 2K page size chips
 *
 * TODO:
 *	Enable cached programming for 2k page size chips
 *	Check, if mtd->ecctype should be set to MTD_ECC_HW
 *	if we have HW ecc support.
 *	The AG-AND chips have nice features for speed improvement,
 *	which are not supported yet. Read / program 4 pages in one go.
 *
 * $Id: nand_base.c,v 1.145 2005/05/31 20:32:53 gleixner Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/compatmac.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <asm/io.h>

#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/partitions.h>
#endif

#ifdef CONFIG_MOT_FEAT_KPANIC
#include <asm/arch/hardware.h>
#endif /* CONFIG_MOT_FEAT_KPANIC */

#if defined(CONFIG_MTD_NAND_BBM)
/* the rsvdblock_offset is the start address for rsv partition*/
extern unsigned long rsvdblock_offset;
#endif

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
#define MAX_RDDIST_BLOCKS       8
#include <linux/vmalloc.h>
/* nand_rddist_block array stores 8 most recent
 * blocks which have been detetced as read disturb
 * block, nand_rddist_block_cnt is index for the 
 * array, which points to the most recent one */
static u16 nand_rddist_block[MAX_RDDIST_BLOCKS];
static u8 nand_rddist_block_cnt = 0;
static int rddist_in_progress = 0;
/* g_read_dist_info is a global varible to store
 * all infomation about read disturbance feature */
struct nand_rddist_info g_read_dist_info = {
	.threshold = 0,			/* threshold for block read cnt */
	.threshold_flag = 0,		/* threshold_flag be set if any block reaches threshold */ 
};
/* g_blkrd_waitq - a global varible to handle
 * block read count request */
DECLARE_WAIT_QUEUE_HEAD(g_blkrd_waitq);
#ifdef CONFIG_MOT_FEAT_NAND_BLKCNT_TEST
struct nand_test_info g_write_test_info = {
	.threshold = 0,
	.threshold_flag = 0
};

struct nand_test_info g_erase_test_info = {
	.threshold = 0,
	.threshold_flag = 0
};
DECLARE_WAIT_QUEUE_HEAD(g_blkwt_waitq);
DECLARE_WAIT_QUEUE_HEAD(g_blkers_waitq);
#endif /*CONFIG_MOT_FEAT_NAND_BLKCNT_TEST*/
#endif /*CONFIG_MOT_FEAT_NAND_RDDIST*/

#ifdef CONFIG_MOT_FEAT_MTD_FS
int mtd_fs_runtime_info=0;
#endif

#ifdef CONFIG_MOT_FEAT_KPANIC
extern int kpanic_in_progress;
#endif /* CONFIG_MOT_FEAT_KPANIC */

/* Define default oob placement schemes for large and small page devices */
static struct nand_oobinfo nand_oob_8 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 3,
	.eccpos = {0, 1, 2},
	.oobfree = { {3, 2}, {6, 2} }
};

static struct nand_oobinfo nand_oob_16 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 6,
	.eccpos = {0, 1, 2, 3, 6, 7},
	.oobfree = { {8, 8} }
};

static struct nand_oobinfo nand_oob_64 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47, 
		48, 49, 50, 51, 52, 53, 54, 55, 
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};

/* This is used for padding purposes in nand_write_oob/nand_write_oob_hwecc */
#define FFCHARS_SIZE		2048
static u_char ffchars[FFCHARS_SIZE];

/*
 * NAND low-level MTD interface functions
 */
static void nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len);
static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len);
static int nand_verify_buf(struct mtd_info *mtd, const u_char *buf, int len);

static int nand_read (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf);
static int nand_read_ecc (struct mtd_info *mtd, loff_t from, size_t len,
			  size_t * retlen, u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel);
static int nand_read_oob (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf);
static int nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char * buf);
static int nand_write_ecc (struct mtd_info *mtd, loff_t to, size_t len,
			   size_t * retlen, const u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel);
static int nand_write_oob (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char *buf);
static int nand_writev (struct mtd_info *mtd, const struct kvec *vecs,
			unsigned long count, loff_t to, size_t * retlen);
static int nand_writev_ecc (struct mtd_info *mtd, const struct kvec *vecs,
			unsigned long count, loff_t to, size_t * retlen, u_char *eccbuf, struct nand_oobinfo *oobsel);
static int nand_erase (struct mtd_info *mtd, struct erase_info *instr);
static void nand_sync (struct mtd_info *mtd);
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
static int nand_read_distfix (struct mtd_info *mtd, int block, int reason_code);
static int nand_rddist_check (struct mtd_info *mtd, loff_t ofs);
static void nand_increment_rdcnt (struct nand_chip *this, int page);
static void nand_increment_erasecnt (struct nand_chip *this, int page);
#endif /*CONFIG_MOT_FEAT_NAND_RDDIST*/
/* Some internal functions */
static int nand_write_page (struct mtd_info *mtd, struct nand_chip *this, int page, u_char *oob_buf,
		struct nand_oobinfo *oobsel, int mode);
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
static int nand_verify_pages (struct mtd_info *mtd, struct nand_chip *this, int page, int numpages, 
	u_char *oob_buf, struct nand_oobinfo *oobsel, int chipnr, int oobmode);
#else
#define nand_verify_pages(...) (0)
#endif
		
static int nand_get_device (struct nand_chip *this, struct mtd_info *mtd, int new_state);
#ifdef CONFIG_MOT_FEAT_KPANIC
static int knand_get_device (struct nand_chip *this, struct mtd_info *mtd, int new_state);
#endif /* CONFIG_MOT_FEAT_KPANIC */

#ifdef CONFIG_MOT_FEAT_MTD_AUTO_BBM
static int nand_block_replace (struct mtd_info *mtd, loff_t ofs, int dev_lock);
#endif

/**
 * nand_release_device - [GENERIC] release chip
 * @mtd:	MTD device structure
 * 
 * Deselect, release chip lock and wake up anyone waiting on the device 
 */
static void nand_release_device (struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	/* keep read disturb fix process atomic, do not change device state;
	 * do not release the locked device while read disturb fix in progress */
	if (rddist_in_progress && this->state == FL_RDDIST_FIXING)
		return;
#endif
	/* De-select the NAND device */
	this->select_chip(mtd, -1);

	if (this->controller) {
		/* Release the controller and the chip */
		spin_lock(&this->controller->lock);
		this->controller->active = NULL;
		this->state = FL_READY;
#ifdef CONFIG_MOT_FEAT_KPANIC
		if (!kpanic_in_progress)
#endif /* CONFIG_MOT_FEAT_KPANIC */
		wake_up(&this->controller->wq);
		spin_unlock(&this->controller->lock);
	} else {
		/* Release the chip */
		spin_lock(&this->chip_lock);
		this->state = FL_READY;
#ifdef CONFIG_MOT_FEAT_KPANIC
                if (!kpanic_in_progress)
#endif /* CONFIG_MOT_FEAT_KPANIC */
		wake_up(&this->wq);
		spin_unlock(&this->chip_lock);
	}
}

/**
 * nand_read_byte - [DEFAULT] read one byte from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 8bit buswith
 */
static u_char nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	return readb(this->IO_ADDR_R);
}

/**
 * nand_write_byte - [DEFAULT] write one byte to the chip
 * @mtd:	MTD device structure
 * @byte:	pointer to data byte to write
 *
 * Default write function for 8it buswith
 */
static void nand_write_byte(struct mtd_info *mtd, u_char byte)
{
	struct nand_chip *this = mtd->priv;
	writeb(byte, this->IO_ADDR_W);
}

/**
 * nand_read_byte16 - [DEFAULT] read one byte endianess aware from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 16bit buswith with 
 * endianess conversion
 */
static u_char nand_read_byte16(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	return (u_char) cpu_to_le16(readw(this->IO_ADDR_R));
}

/**
 * nand_write_byte16 - [DEFAULT] write one byte endianess aware to the chip
 * @mtd:	MTD device structure
 * @byte:	pointer to data byte to write
 *
 * Default write function for 16bit buswith with
 * endianess conversion
 */
static void nand_write_byte16(struct mtd_info *mtd, u_char byte)
{
	struct nand_chip *this = mtd->priv;
	writew(le16_to_cpu((u16) byte), this->IO_ADDR_W);
}

/**
 * nand_read_word - [DEFAULT] read one word from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 16bit buswith without 
 * endianess conversion
 */
static u16 nand_read_word(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	return readw(this->IO_ADDR_R);
}

/**
 * nand_write_word - [DEFAULT] write one word to the chip
 * @mtd:	MTD device structure
 * @word:	data word to write
 *
 * Default write function for 16bit buswith without 
 * endianess conversion
 */
static void nand_write_word(struct mtd_info *mtd, u16 word)
{
	struct nand_chip *this = mtd->priv;
	writew(word, this->IO_ADDR_W);
}

/**
 * nand_select_chip - [DEFAULT] control CE line
 * @mtd:	MTD device structure
 * @chip:	chipnumber to select, -1 for deselect
 *
 * Default select function for 1 chip devices.
 */
static void nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct nand_chip *this = mtd->priv;
	switch(chip) {
	case -1:
		this->hwcontrol(mtd, NAND_CTL_CLRNCE);	
		break;
	case 0:
		this->hwcontrol(mtd, NAND_CTL_SETNCE);
		break;

	default:
		BUG();
	}
}

/**
 * nand_write_buf - [DEFAULT] write buffer to chip
 * @mtd:	MTD device structure
 * @buf:	data buffer
 * @len:	number of bytes to write
 *
 * Default write function for 8bit buswith
 */
static void nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i=0; i<len; i++)
		writeb(buf[i], this->IO_ADDR_W);
}

/**
 * nand_read_buf - [DEFAULT] read chip data into buffer 
 * @mtd:	MTD device structure
 * @buf:	buffer to store date
 * @len:	number of bytes to read
 *
 * Default read function for 8bit buswith
 */
static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i=0; i<len; i++)
		buf[i] = readb(this->IO_ADDR_R);
}

/**
 * nand_verify_buf - [DEFAULT] Verify chip data against buffer 
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 8bit buswith
 */
static int nand_verify_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i=0; i<len; i++)
		if (buf[i] != readb(this->IO_ADDR_R))
			return -EFAULT;

	return 0;
}

/**
 * nand_write_buf16 - [DEFAULT] write buffer to chip
 * @mtd:	MTD device structure
 * @buf:	data buffer
 * @len:	number of bytes to write
 *
 * Default write function for 16bit buswith
 */
static void nand_write_buf16(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;
	
	for (i=0; i<len; i++)
		writew(p[i], this->IO_ADDR_W);
		
}

/**
 * nand_read_buf16 - [DEFAULT] read chip data into buffer 
 * @mtd:	MTD device structure
 * @buf:	buffer to store date
 * @len:	number of bytes to read
 *
 * Default read function for 16bit buswith
 */
static void nand_read_buf16(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i=0; i<len; i++)
		p[i] = readw(this->IO_ADDR_R);
}

/**
 * nand_verify_buf16 - [DEFAULT] Verify chip data against buffer 
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 16bit buswith
 */
static int nand_verify_buf16(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i=0; i<len; i++)
		if (p[i] != readw(this->IO_ADDR_R))
			return -EFAULT;

	return 0;
}

/**
 * nand_block_bad - [DEFAULT] Read bad block marker from the chip
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 * @getchip:	0, if the chip is already selected
 *
 * Check, if the block is bad. 
 */
static int nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	int page, chipnr, res = 0;
	struct nand_chip *this = mtd->priv;
	u16 bad;
	
	if (getchip) {
		page = (int)(ofs >> this->page_shift);
		chipnr = (int)(ofs >> this->chip_shift);

		/* Grab the lock and see if the device is available */
		nand_get_device (this, mtd, FL_READING);

		/* Select the NAND device */
		this->select_chip(mtd, chipnr);
	} else 
		page = (int) ofs;	

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
                nand_increment_rdcnt(this, page);
#endif
	if (this->options & NAND_BUSWIDTH_16) {
		this->cmdfunc (mtd, NAND_CMD_READOOB, this->badblockpos & 0xFE, page & this->pagemask);
		bad = cpu_to_le16(this->read_word(mtd));
		if (this->badblockpos & 0x1)
			bad >>= 8;
		if ((bad & 0xFF) != 0xff)
			res = 1;
	} else {
		this->cmdfunc (mtd, NAND_CMD_READOOB, this->badblockpos, page & this->pagemask);
		if (this->read_byte(mtd) != 0xff)
			res = 1;
	}
		
	if (getchip) {
		/* Deselect and wake up anyone waiting on the device */
		nand_release_device(mtd);
	}	

	return res;
}

#if defined(CONFIG_MTD_NAND_BBM) 
/**
 * nand_translate_block - get translated block from reserve pool
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 *
 * return translated block from reserved pool - offset from device start
 */
loff_t nand_translate_block(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = mtd->priv;
	int orig_block;
	loff_t page_ofs, translate_block_ofs;
retry:
	/* calculate the block number and page offset */
	orig_block = (int)(ofs >> this->phys_erase_shift);
	page_ofs = ofs - (loff_t)(orig_block <<  this->phys_erase_shift);

	/* check, if no translate returns back the original offset */
	if (this->bbm[orig_block] == 0xffff)
		return ofs;

	/* get the translate block */
	translate_block_ofs = (this->bbm[orig_block] << this->phys_erase_shift) + rsvdblock_offset + page_ofs;
	DEBUG (MTD_DEBUG_LEVEL0, "original bad block# %d with offset 0x%08x, translate_block_ofs 0x%08x\n", 
		orig_block, (unsigned int)ofs, (unsigned int)translate_block_ofs);

	/* make sure the translate is a good block */
	if (nand_isbad_bbt(mtd, translate_block_ofs, 0)) {
		ofs = translate_block_ofs;
		goto retry;
	}
	/* must have the good block for the translate */
	return translate_block_ofs;
}
#endif /* CONFIG_MTD_NAND_BBM */

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
/**
 * nand_do_block_mark - mark a block with given code
 * @mtd:        MTD device structure
 * @ofs:        offset from device start
 *
 * This is temporarily marking a block bad with block reserved code
 * after the ECC detected/corrected 1BIT read distrub erorr. The BBM
 * software will recove the read distrub erorr by reflashing this block.
 * After the reflashing, the temporarily marked bad block with reserved
 * code be removed and reclaiming as good block.
 */
static int nand_do_block_mark(struct mtd_info *mtd, loff_t ofs, u8 code)
{
	struct nand_chip *this = mtd->priv;
	int block;

	/* Get block number */
	block = ((int) ofs) >> this->bbt_erase_shift;
	/** set the bbt code:
	* 00b:         block is good
	* 01b:         block is marked bad due to wear
	* 10b:         block is reserved 
	* 11b:         block is factory marked bad
	*/
	DEBUG (MTD_DEBUG_LEVEL0,"block %d has been marked with %x in bbt\n", block, code);
	if (code == 0x00)
		/* set it to be good block */
		this->bbt[block >> 2] &= ~(0x01 << ((block & 0x03) << 1));
	else
		/* set it to be bad block */
		this->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);

	/* update the bbt */
	return nand_update_bbt (mtd, ofs);
}
#endif /*CONFIG_MOT_FEAT_NAND_RDDIST*/
/**
 * nand_default_block_markbad - [DEFAULT] mark a block bad
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
*/
static int nand_default_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = mtd->priv;
	u_char buf[2] = {0, 0};
	size_t	retlen;
	int block;
	
	/* Get block number */
	block = ((int) ofs) >> this->bbt_erase_shift;
	if (this->bbt)
		this->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);

	/* Do we have a flash based bad block table ? */
	if (this->options & NAND_USE_FLASH_BBT)
		return nand_update_bbt (mtd, ofs);
		
	/* We write two bytes, so we dont have to mess with 16 bit access */
	ofs += mtd->oobsize + (this->badblockpos & ~0x01);
	return nand_write_oob (mtd, ofs , 2, &retlen, buf);
}

/** 
 * nand_check_wp - [GENERIC] check if the chip is write protected
 * @mtd:	MTD device structure
 * Check, if the device is write protected 
 *
 * The function expects, that the device is already selected 
 */
static int nand_check_wp (struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	/* Check the WP bit */
	this->cmdfunc (mtd, NAND_CMD_STATUS, -1, -1);
	return (this->read_byte(mtd) & NAND_STATUS_WP) ? 0 : 1; 
}

/**
 * nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 * @getchip:	0, if the chip is already selected
 * @allowbbt:	1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int nand_block_checkbad (struct mtd_info *mtd, loff_t ofs, int getchip, int allowbbt)
{
	struct nand_chip *this = mtd->priv;

	if (!this->bbt)
		return this->block_bad(mtd, ofs, getchip);

#if defined(CONFIG_MTD_NAND_BBM)
/*
 * With the Bad Block management feature, this function has been modified to
 * return good (return 0) if the physical block is bad, but it can be replaced
 * by a good block from reserved pool.  It returns bad (return 1) only if the
 * bad block with no replacement
 */
	if (nand_isbad_bbt (mtd, ofs, allowbbt)) {  /* bbt said: bad block */
		if (ofs != nand_translate_block(mtd, ofs))
		/* has replacement - return as good block */
			return 0;
		else
		/* no replcaement - return block bad */
			return 1;
	}
	else /* bbt said: block is good */
		return 0;
#else
	return nand_isbad_bbt (mtd, ofs, allowbbt);
#endif
}

/* 
 * Wait for the ready pin, after a command
 * The timeout is catched later.
 */
static void nand_wait_ready(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	unsigned long	timeo = jiffies + 2;

#ifdef CONFIG_MOT_FEAT_KPANIC
	if (kpanic_in_progress) {
		#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321)
		/* generate a compare event in 2 jiffies */
		*(volatile __u32*)MXC_GPT_GPTOCR1 = 2 * LATCH;
		/* while the compare event has not been generated... */
		while (!(*(volatile __u32*)MXC_GPT_GPTSR & 1)) {
			if (this->dev_ready(mtd))
				return;
		}
		#else
		#error GPT counter register not available on non-scma11 and non-argonplus platforms
		#endif /* CONFIG_ARCH_MXC91231 || CONFIG_ARCH_MXC91331 */
	}
	else
#endif /* CONFIG_MOT_FEAT_KPANIC */
	/* wait until command is processed or timeout occures */
	do {
		if (this->dev_ready(mtd))
			return;
	} while (time_before(jiffies, timeo));	
	printk (KERN_WARNING "nand_wait_ready timeout occured\n");
}

/**
 * nand_command - [DEFAULT] Send command to NAND device
 * @mtd:	MTD device structure
 * @command:	the command to be sent
 * @column:	the column address for this command, -1 if none
 * @page_addr:	the page address for this command, -1 if none
 *
 * Send command to NAND device. This function is used for small page
 * devices (256/512 Bytes per page)
 */
static void nand_command (struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	register struct nand_chip *this = mtd->priv;

	/* Begin command latch cycle */
	this->hwcontrol(mtd, NAND_CTL_SETCLE);
	/*
	 * Write out the command to the device.
	 */
	if (command == NAND_CMD_SEQIN) {
		int readcmd;

		if (column >= mtd->oobblock) {
			/* OOB area */
			column -= mtd->oobblock;
			readcmd = NAND_CMD_READOOB;
		} else if (column < 256) {
			/* First 256 bytes --> READ0 */
			readcmd = NAND_CMD_READ0;
		} else {
			column -= 256;
			readcmd = NAND_CMD_READ1;
		}
		this->write_byte(mtd, readcmd);
	}
	this->write_byte(mtd, command);

	/* Set ALE and clear CLE to start address cycle */
	this->hwcontrol(mtd, NAND_CTL_CLRCLE);

	if (column != -1 || page_addr != -1) {
		this->hwcontrol(mtd, NAND_CTL_SETALE);

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (this->options & NAND_BUSWIDTH_16)
				column >>= 1;
			this->write_byte(mtd, column);
		}
		if (page_addr != -1) {
			this->write_byte(mtd, (unsigned char) (page_addr & 0xff));
			this->write_byte(mtd, (unsigned char) ((page_addr >> 8) & 0xff));
			/* One more address cycle for devices > 32MiB */
			if (this->chipsize > (32 << 20))
				this->write_byte(mtd, (unsigned char) ((page_addr >> 16) & 0x0f));
		}
		/* Latch in address */
		this->hwcontrol(mtd, NAND_CTL_CLRALE);
	}
	
	/* 
	 * program and erase have their own busy handlers 
	 * status and sequential in needs no delay
	*/
	switch (command) {
			
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
		this->hwcontrol(mtd, NAND_CTL_SETCLE);
		this->write_byte(mtd, NAND_CMD_STATUS);
		this->hwcontrol(mtd, NAND_CTL_CLRCLE);
		while ( !(this->read_byte(mtd) & NAND_STATUS_READY));
		return;

	/* This applies to read commands */	
	default:
		/* 
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		*/
		if (!this->dev_ready) {
			udelay (this->chip_delay);
			return;
		}	
	}
	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay (100);

	nand_wait_ready(mtd);
}

/**
 * nand_command_lp - [DEFAULT] Send command to NAND large page device
 * @mtd:	MTD device structure
 * @command:	the command to be sent
 * @column:	the column address for this command, -1 if none
 * @page_addr:	the page address for this command, -1 if none
 *
 * Send command to NAND device. This is the version for the new large page devices
 * We dont have the seperate regions as we have in the small page devices.
 * We must emulate NAND_CMD_READOOB to keep the code compatible.
 *
 */
static void nand_command_lp (struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	register struct nand_chip *this = mtd->priv;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->oobblock;
		command = NAND_CMD_READ0;
	}
	
		
	/* Begin command latch cycle */
	this->hwcontrol(mtd, NAND_CTL_SETCLE);
	/* Write out the command to the device. */
	this->write_byte(mtd, (command & 0xff));
	/* End command latch cycle */
	this->hwcontrol(mtd, NAND_CTL_CLRCLE);

	if (column != -1 || page_addr != -1) {
		this->hwcontrol(mtd, NAND_CTL_SETALE);

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (this->options & NAND_BUSWIDTH_16)
				column >>= 1;
			this->write_byte(mtd, column & 0xff);
			this->write_byte(mtd, column >> 8);
		}	
		if (page_addr != -1) {
			this->write_byte(mtd, (unsigned char) (page_addr & 0xff));
			this->write_byte(mtd, (unsigned char) ((page_addr >> 8) & 0xff));
			/* One more address cycle for devices > 128MiB */
			if (this->chipsize > (128 << 20))
				this->write_byte(mtd, (unsigned char) ((page_addr >> 16) & 0xff));
		}
		/* Latch in address */
		this->hwcontrol(mtd, NAND_CTL_CLRALE);
	}
	
	/* 
	 * program and erase have their own busy handlers 
	 * status, sequential in, and deplete1 need no delay
	 */
	switch (command) {
			
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
	case NAND_CMD_DEPLETE1:
		return;

	/* 
	 * read error status commands require only a short delay
	 */
	case NAND_CMD_STATUS_ERROR:
	case NAND_CMD_STATUS_ERROR0:
	case NAND_CMD_STATUS_ERROR1:
	case NAND_CMD_STATUS_ERROR2:
	case NAND_CMD_STATUS_ERROR3:
		udelay(this->chip_delay);
		return;

	case NAND_CMD_RESET:
		if (this->dev_ready)	
			break;
		udelay(this->chip_delay);
		this->hwcontrol(mtd, NAND_CTL_SETCLE);
		this->write_byte(mtd, NAND_CMD_STATUS);
		this->hwcontrol(mtd, NAND_CTL_CLRCLE);
		while ( !(this->read_byte(mtd) & NAND_STATUS_READY));
		return;

	case NAND_CMD_READ0:
		/* Begin command latch cycle */
		this->hwcontrol(mtd, NAND_CTL_SETCLE);
		/* Write out the start read command */
		this->write_byte(mtd, NAND_CMD_READSTART);
		/* End command latch cycle */
		this->hwcontrol(mtd, NAND_CTL_CLRCLE);
		/* Fall through into ready check */
		
	/* This applies to read commands */	
	default:
		/* 
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		*/
		if (!this->dev_ready) {
			udelay (this->chip_delay);
			return;
		}	
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay (100);

	nand_wait_ready(mtd);
}

/**
 * nand_get_device - [GENERIC] Get chip for selected access
 * @this:	the nand chip descriptor
 * @mtd:	MTD device structure
 * @new_state:	the state which is requested 
 *
 * Get the device and lock it for exclusive access
 */
static int nand_get_device (struct nand_chip *this, struct mtd_info *mtd, int new_state)
{
	struct nand_chip *active;
	spinlock_t *lock;
	wait_queue_head_t *wq;
	DECLARE_WAITQUEUE (wait, current);

	lock = (this->controller) ? &this->controller->lock : &this->chip_lock;
	wq = (this->controller) ? &this->controller->wq : &this->wq;
retry:
	active = this;
	spin_lock(lock);

	/* Hardware controller shared among independend devices */
	if (this->controller) {
		if (this->controller->active)
			active = this->controller->active;
		else
			this->controller->active = this;
	}
	if (active == this && this->state == FL_READY) {
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
		/* 
		 * When read_distfix thread first time get and hold
		 * the nand_device, save the thread owner, set the 
		 * rddist_in_progress flag, and set this->state to
		 * FL_RDDIST_FIXING 
		 */  
		if (new_state == FL_RDDIST_FIXING) {
			this->owner = (struct task*) current;
			rddist_in_progress = 1;
		}
#endif
		this->state = new_state;
		spin_unlock(lock);
		return 0;
	}
	if (new_state == FL_PM_SUSPENDED) {
		spin_unlock(lock);
		return (this->state == FL_PM_SUSPENDED) ? 0 : -EAGAIN;
	}

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	/*
	 * Only the owner of read_distfix thread can re-access the nand flash, and
	 * return to the caller while nand_device is still in FL_RDDIST_FIXING state.
	 * All other threads' request will be added to wait_queue.
	 */
	if ((this->owner == (struct task*)current) && this->state == FL_RDDIST_FIXING) {
		spin_unlock(lock);
		return 0;
	}
#endif
	set_current_state(TASK_UNINTERRUPTIBLE);
	add_wait_queue(wq, &wait);
	spin_unlock(lock);
	schedule();
	remove_wait_queue(wq, &wait);
	goto retry;
}

#ifdef CONFIG_MOT_FEAT_KPANIC
/*
 * Try to get the device and lock it for exclusive access. This routine is only
 * called in the panic context.
 * 
 * @return	0 on success, 1 on failure
 */
static int knand_get_device (struct nand_chip *this, struct mtd_info *mtd, int new_state)
{
        struct nand_chip *active;
        spinlock_t *lock;

        lock = (this->controller) ? &this->controller->lock : &this->chip_lock;

        active = this;

	/* return if we cannot get the lock */
	if (!spin_trylock(lock))
		return 1;

        /* Hardware controller shared among independent devices */
        if (this->controller) {
                if (this->controller->active)
                        active = this->controller->active;
                else
                        this->controller->active = this;
        }
        if (active == this && this->state == FL_READY) {
                this->state = new_state;
                spin_unlock(lock);
                return 0;
        }
        spin_unlock(lock);
	return 1;
}
#endif /* CONFIG_MOT_FEAT_KPANIC */

/**
 * nand_wait - [DEFAULT]  wait until the command is done
 * @mtd:	MTD device structure
 * @this:	NAND chip structure
 * @state:	state to select the max. timeout value
 *
 * Wait for command done. This applies to erase and program only
 * Erase can take up to 400ms and program up to 20ms according to 
 * general NAND and SmartMedia specs
 *
*/
static int nand_wait(struct mtd_info *mtd, struct nand_chip *this, int state)
{

	unsigned long	timeo = jiffies;
	int	status;
	
	if (state == FL_ERASING) {
		 timeo += (HZ * 400) / 1000;
#ifdef CONFIG_MOT_FEAT_KPANIC
		if (kpanic_in_progress) {
			#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321)
			/* generate a compare event in 400 ms */
			*(volatile __u32*)MXC_GPT_GPTOCR1 = *(volatile __u32*)MXC_GPT_GPTCNT + (CLOCK_TICK_RATE / 1000) * 400;
			#else
			#error GPT counter register not available on non-scma11 and non-argonplus platforms
			#endif /* CONFIG_ARCH_MXC91231 || CONFIG_ARCH_MXC91331 */
		}
#endif /* CONFIG_MOT_FEAT_KPANIC */
	}
	else {
		 timeo += (HZ * 20) / 1000;
#ifdef CONFIG_MOT_FEAT_KPANIC
		if (kpanic_in_progress) {
			#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321)
			/* generate a compare event in 20 ms */
			*(volatile __u32*)MXC_GPT_GPTOCR1 = *(volatile __u32*)MXC_GPT_GPTCNT + (CLOCK_TICK_RATE / 1000) * 20;
			#else
                        #error GPT counter register not available on non-scma11 and non-argonplus platforms
                        #endif /* CONFIG_ARCH_MXC91231 || CONFIG_ARCH_MXC91331 */
		}
#endif /* CONFIG_MOT_FEAT_KPANIC */
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay (100);

	if ((state == FL_ERASING) && (this->options & NAND_IS_AND))
		this->cmdfunc (mtd, NAND_CMD_STATUS_MULTI, -1, -1);
	else	
		this->cmdfunc (mtd, NAND_CMD_STATUS, -1, -1);

#ifdef CONFIG_MOT_FEAT_KPANIC
	if (kpanic_in_progress) {
		#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321)
		/* while the compare event has not been generated... */
		while (!(*(volatile __u32*)MXC_GPT_GPTSR & 1)) {
			/* Check, if we were interrupted */
                	if (this->state != state)
                        	return 0;

			if (this->dev_ready) {
				if (this->dev_ready(mtd))
					break;
			} else {
				if (this->read_byte(mtd) & NAND_STATUS_READY)
					break;
                	}
		}
		#else
		#error GPT counter register not available on non-scma11 and non-argonplus platforms
		#endif /* CONFIG_ARCH_MXC91231 || CONFIG_ARCH_MXC91331 */
	}
	else
#endif /* CONFIG_MOT_FEAT_KPANIC */
	while (time_before(jiffies, timeo)) {		
		/* Check, if we were interrupted */
		if (this->state != state)
			return 0;

		if (this->dev_ready) {
			if (this->dev_ready(mtd))
				break;	
		} else {
			if (this->read_byte(mtd) & NAND_STATUS_READY)
				break;
		}
		cond_resched();
	}
	status = (int) this->read_byte(mtd);
	return status;
}

/**
 * nand_write_page - [GENERIC] write one page
 * @mtd:	MTD device structure
 * @this:	NAND chip structure
 * @page: 	startpage inside the chip, must be called with (page & this->pagemask)
 * @oob_buf:	out of band data buffer
 * @oobsel:	out of band selecttion structre
 * @cached:	1 = enable cached programming if supported by chip
 *
 * Nand_page_program function is used for write and writev !
 * This function will always program a full page of data
 * If you call it with a non page aligned buffer, you're lost :)
 *
 * Cached programming is not supported yet.
 */
static int nand_write_page (struct mtd_info *mtd, struct nand_chip *this, int page, 
	u_char *oob_buf,  struct nand_oobinfo *oobsel, int cached)
{
	int 	i, oobidx, status;
	u_char	ecc_code[40];
	int	eccmode = oobsel->useecc ? this->eccmode : NAND_ECC_NONE;
	int  	*oob_config = oobsel->eccpos;
	int	datidx = 0, eccidx = 0, eccsteps = this->eccsteps;
	int	eccbytes = 0;
	
	/* FIXME: Enable cached programming */
	cached = 0;

	/* Send command to begin auto page programming */
	this->cmdfunc (mtd, NAND_CMD_SEQIN, 0x00, page);

	/* Write out complete page of data, take care of eccmode */
	switch (eccmode) {
	/* No ecc, write all */
	case NAND_ECC_NONE:
		printk (KERN_WARNING "Writing data without ECC to NAND-FLASH is not recommended\n");
		this->write_buf(mtd, this->data_poi, mtd->oobblock);
		this->write_buf(mtd, oob_buf, mtd->oobsize);
		break;
		
	/* Software ecc 3/256, write all */
	case NAND_ECC_SOFT:
		for (; eccsteps; eccsteps--) {
			this->calculate_ecc(mtd, &this->data_poi[datidx], ecc_code);
			for (i = 0; i < 3; i++, eccidx++)
				oob_buf[oob_config[eccidx]] = ecc_code[i];
			datidx += this->eccsize;
		}
		this->write_buf(mtd, this->data_poi, mtd->oobblock);
		this->write_buf(mtd, oob_buf, mtd->oobsize);
		break;
	default:
		eccbytes = this->eccbytes;

		if (! this->layout) {
			for (; eccsteps; eccsteps--) {
				/* enable hardware ecc logic for write */
				this->enable_hwecc(mtd, NAND_ECC_WRITE);
				this->write_buf(mtd, &this->data_poi[datidx], this->eccsize);
				this->calculate_ecc(mtd, &this->data_poi[datidx], ecc_code);
				for (i = 0; i < eccbytes; i++, eccidx++)
					oob_buf[oob_config[eccidx]] = ecc_code[i];
				/* If the hardware ecc provides syndromes then
				 * the ecc code must be written immidiately after
				 * the data bytes (words) */
				if (this->options & NAND_HWECC_SYNDROME)
					this->write_buf(mtd, ecc_code, eccbytes);
				datidx += this->eccsize;
			}

			if (this->options & NAND_HWECC_SYNDROME)
				this->write_buf(mtd, &oob_buf[oobsel->eccbytes], mtd->oobsize -
						oobsel->eccbytes);
			else
				this->write_buf(mtd, oob_buf, mtd->oobsize);


			break;
		}

		for (oobidx = 0; eccsteps; eccsteps--) {
			int j = 0, last_datidx = datidx, last_oobidx;
			for (; this->layout[j].length; j++) {
				int len = this->layout[j].length;
				int oidx = oobidx;
				switch (this->layout[j].type) {
				case ITEM_TYPE_DATA:
					this->enable_hwecc(mtd, NAND_ECC_WRITE);
					this->write_buf(mtd, &this->data_poi[datidx], this->layout[j].length);
					datidx += len;
					break;
				case ITEM_TYPE_ECC:
					this->enable_hwecc(mtd, NAND_ECC_WRITESYN);
					this->calculate_ecc(mtd, &this->data_poi[last_datidx], &ecc_code[eccidx]);
					for (last_oobidx = oobidx; oobidx < last_oobidx + len; oobidx++, eccidx++)
						oob_buf[oobidx] = ecc_code[eccidx];
					if (this->options & NAND_BUSWIDTH_16) {
						if (oidx & 1) {
							oidx--;
							len++;
						}
						if (len & 1)
							len--;
					}
					this->write_buf(mtd, &oob_buf[oidx], len);
					break;
				case ITEM_TYPE_OOB:
					this->enable_hwecc(mtd, NAND_ECC_WRITEOOB);
					if (this->options & NAND_BUSWIDTH_16) {
						if (oidx & 1) {
							oidx--;
							len++;
						}
						if (len & 1)
							len--;
					}
					this->write_buf(mtd, &oob_buf[oidx], len);
					oobidx += len;
					break;
				}
			}
					
		}
		break;
	}
										
	/* Send command to actually program the data */
	this->cmdfunc (mtd, cached ? NAND_CMD_CACHEDPROG : NAND_CMD_PAGEPROG, -1, -1);

	if (!cached) {
		/* call wait ready function */
		status = this->waitfunc (mtd, this, FL_WRITING);

		/* See if operation failed and additional status checks are available */
		if ((status & NAND_STATUS_FAIL) && (this->errstat)) {
			status = this->errstat(mtd, this, FL_WRITING, status, page);
		}

		/* See if device thinks it succeeded */
		if (status & NAND_STATUS_FAIL) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write, page 0x%08x, ", __FUNCTION__, page);
			return -EIO;
		}
	} else {
		/* FIXME: Implement cached programming ! */
		/* wait until cache is ready*/
		// status = this->waitfunc (mtd, this, FL_CACHEDRPG);
	}
	return 0;	
}

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
/**
 * nand_verify_pages - [GENERIC] verify the chip contents after a write
 * @mtd:	MTD device structure
 * @this:	NAND chip structure
 * @page: 	startpage inside the chip, must be called with (page & this->pagemask)
 * @numpages:	number of pages to verify
 * @oob_buf:	out of band data buffer
 * @oobsel:	out of band selecttion structre
 * @chipnr:	number of the current chip
 * @oobmode:	1 = full buffer verify, 0 = ecc only
 *
 * The NAND device assumes that it is always writing to a cleanly erased page.
 * Hence, it performs its internal write verification only on bits that 
 * transitioned from 1 to 0. The device does NOT verify the whole page on a
 * byte by byte basis. It is possible that the page was not completely erased 
 * or the page is becoming unusable due to wear. The read with ECC would catch 
 * the error later when the ECC page check fails, but we would rather catch 
 * it early in the page write stage. Better to write no data than invalid data.
 */
static int nand_verify_pages (struct mtd_info *mtd, struct nand_chip *this, int page, int numpages, 
	u_char *oob_buf, struct nand_oobinfo *oobsel, int chipnr, int oobmode)
{
	int 	i, j, datidx = 0, oobofs = 0, res = -EIO;
	int	eccsteps = this->eccsteps;
	int	hweccbytes; 
	u_char 	oobdata[64];

	hweccbytes = (this->options & NAND_HWECC_SYNDROME) ? (oobsel->eccbytes / eccsteps) : 0;

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	nand_increment_rdcnt(this, page);
#endif
	/* Send command to read back the first page */
	this->cmdfunc (mtd, NAND_CMD_READ0, 0, page);

	for(;;) {
		for (j = 0; j < eccsteps; j++) {
			/* Loop through and verify the data */
			if (this->verify_buf(mtd, &this->data_poi[datidx], mtd->eccsize)) {
				DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write verify, page 0x%08x ", __FUNCTION__, page);
				goto out;
			}
			datidx += mtd->eccsize;
			/* Have we a hw generator layout ? */
			if (!hweccbytes)
				continue;
			if (this->verify_buf(mtd, &this->oob_buf[oobofs], hweccbytes)) {
				DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write verify, page 0x%08x ", __FUNCTION__, page);
				goto out;
			}
			oobofs += hweccbytes;
		}

		/* check, if we must compare all data or if we just have to
		 * compare the ecc bytes
		 */
		if (oobmode) {
			if (this->verify_buf(mtd, &oob_buf[oobofs], mtd->oobsize - hweccbytes * eccsteps)) {
				DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write verify, page 0x%08x ", __FUNCTION__, page);
				goto out;
			}
		} else {
			/* Read always, else autoincrement fails */
			this->read_buf(mtd, oobdata, mtd->oobsize - hweccbytes * eccsteps);

			if (oobsel->useecc != MTD_NANDECC_OFF && !hweccbytes) {
				int ecccnt = oobsel->eccbytes;
		
				for (i = 0; i < ecccnt; i++) {
					int idx = oobsel->eccpos[i];
					if (oobdata[idx] != oob_buf[oobofs + idx] ) {
						DEBUG (MTD_DEBUG_LEVEL0,
					       	"%s: Failed ECC write "
						"verify, page 0x%08x, " "%6i bytes were succesful\n", __FUNCTION__, page, i);
						goto out;
					}
				}
			}	
		}
		oobofs += mtd->oobsize - hweccbytes * eccsteps;
		page++;
		numpages--;

		/* Apply delay or wait for ready/busy pin 
		 * Do this before the AUTOINCR check, so no problems
		 * arise if a chip which does auto increment
		 * is marked as NOAUTOINCR by the board driver.
		 * Do this also before returning, so the chip is
		 * ready for the next command.
		*/
		if (!this->dev_ready) 
			udelay (this->chip_delay);
		else
			nand_wait_ready(mtd);

		/* All done, return happy */
		if (!numpages)
			return 0;
		
			
		/* Check, if the chip supports auto page increment */ 
		if (!NAND_CANAUTOINCR(this)) {
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
			nand_increment_rdcnt(this, page);
#endif
			this->cmdfunc (mtd, NAND_CMD_READ0, 0x00, page);
		}
	}
	/* 
	 * Terminate the read command. We come here in case of an error
	 * So we must issue a reset command.
	 */
out:	 
	this->cmdfunc (mtd, NAND_CMD_RESET, -1, -1);
	return res;
}
#endif

/**
 * nand_read - [MTD Interface] MTD compability function for nand_do_read_ecc
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 *
 * This function simply calls nand_do_read_ecc with oob buffer and oobsel = NULL
 * and flags = 0xff
 */
static int nand_read (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	printk(KERN_INFO "nand_read() should not be called directly here.\n");
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	{
		int ret, read_dist_block = 0;
		ret = nand_do_read_ecc(mtd, from, len, retlen, buf, NULL, &mtd->oobinfo, 0xff, &read_dist_block);
		if (read_dist_block != 0) 
			mtd->read_distfix(mtd, read_dist_block, RDDIST_BITERR);
		return ret;
	}
#else
	return nand_do_read_ecc (mtd, from, len, retlen, buf, NULL, &mtd->oobinfo, 0xff);
#endif
}


/**
 * nand_read_ecc - [MTD Interface] MTD compability function for nand_do_read_ecc
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 * @oob_buf:	filesystem supplied oob data buffer
 * @oobsel:	oob selection structure
 *
 * This function simply calls nand_do_read_ecc with flags = 0xff
 */
static int nand_read_ecc (struct mtd_info *mtd, loff_t from, size_t len,
			  size_t * retlen, u_char * buf, u_char * oob_buf, struct nand_oobinfo *oobsel)
{
	/* use userspace supplied oobinfo, if zero */
	if (oobsel == NULL)
		oobsel = &mtd->oobinfo;
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	{
		int ret, read_dist_block = 0;
		ret = nand_do_read_ecc(mtd, from, len, retlen, buf, oob_buf, oobsel, 0xff, &read_dist_block);
		if (read_dist_block != 0) 
			mtd->read_distfix(mtd, read_dist_block, RDDIST_BITERR);
		return ret;
	}
#else
	return nand_do_read_ecc(mtd, from, len, retlen, buf, oob_buf, oobsel, 0xff);
#endif
}

/**
 * nand_do_read_ecc - [MTD Interface] Read data with ECC
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 * @oob_buf:	filesystem supplied oob data buffer (can be NULL)
 * @oobsel:	oob selection structure
 * @flags:	flag to indicate if nand_get_device/nand_release_device should be preformed
 *		and how many corrected error bits are acceptable:
 *		  bits 0..7 - number of tolerable errors
 *		  bit  8    - 0 == do not get/release chip, 1 == get/release chip
 *
 * NAND read with ECC
 */
int nand_do_read_ecc (struct mtd_info *mtd, loff_t from, size_t len,
			     size_t * retlen, u_char * buf, u_char * oob_buf, 
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
			     struct nand_oobinfo *oobsel, int flags, int *rddist_block)
#else
			     struct nand_oobinfo *oobsel, int flags)
#endif
{
	int i, j, col, realpage, page, end, ecc, chipnr, sndcmd = 1;
	int read = 0, oob = 0, oobidx, ecc_status = 0, ecc_failed = 0, eccidx;
	struct nand_chip *this = mtd->priv;
	u_char *data_poi, *oob_data = oob_buf;
	u_char ecc_calc[32];
	u_char ecc_code[32];
        int eccmode, eccsteps;
	int	*oob_config, datidx;
	int	blockcheck = (1 << (this->phys_erase_shift - this->page_shift)) - 1;
	int	eccbytes;
	int	compareecc = 1;
	int	oobreadlen;
#if defined(CONFIG_MTD_NAND_BBM)
	loff_t  from_rb;
	int page_cnt = 0, block_replace_flag = 0;
#endif

	DEBUG (MTD_DEBUG_LEVEL3, "nand_read_ecc: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	if (flags & NAND_GET_DEVICE)
		nand_get_device (this, mtd, FL_READING);

	/* Autoplace of oob data ? Use the default placement scheme */
	if (oobsel->useecc == MTD_NANDECC_AUTOPLACE)
		oobsel = this->autooob;
		
	eccmode = oobsel->useecc ? this->eccmode : NAND_ECC_NONE;
	oob_config = oobsel->eccpos;

	/* Select the NAND device */
	chipnr = (int)(from >> this->chip_shift);
	this->select_chip(mtd, chipnr);

	/* First we calculate the starting page */
	realpage = (int) (from >> this->page_shift);

#if defined(CONFIG_MTD_NAND_BBM)
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	if (nand_rddist_check(mtd, from))
		/* read disturb fix inprogress, don't do block replacement */
		DEBUG (MTD_DEBUG_LEVEL0, "no block replacement\n");
	else 
#endif
	/* Check for block replacement if the given page is in bad block */
	if (nand_isbad_bbt (mtd, from, 0)) {
		from_rb = nand_translate_block(mtd, from);
		if (from_rb != from) {
			block_replace_flag = 1;
			DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: badblock offset:0x%08x, replblock offset:0x%08x\n",
				(unsigned int)from, (unsigned int)from_rb);
			realpage = (int) (from_rb >> this->page_shift);
		}
	}
#endif

	page = realpage & this->pagemask;
	/* Get raw starting column */
	col = from & (mtd->oobblock - 1);

	end = mtd->oobblock;
	ecc = this->eccsize;
	eccbytes = this->eccbytes;
	
	if ((eccmode == NAND_ECC_NONE) || (this->options & NAND_HWECC_SYNDROME))
		compareecc = 0;

	oobreadlen = mtd->oobsize;
	if (this->options & NAND_HWECC_SYNDROME) 
		oobreadlen -= oobsel->eccbytes;

	/* Loop until all data read */
	while (read < len) {
		
		int aligned = (!col && (len - read) >= end);
		/* 
		 * If the read is not page aligned, we have to read into data buffer
		 * due to ecc, else we read into return buffer direct
		 */
		if (aligned)
			data_poi = &buf[read];
		else 
			data_poi = this->data_buf;
		
		/* Check, if we have this page in the buffer 
		 *
		 * FIXME: Make it work when we must provide oob data too,
		 * check the usage of data_buf oob field
		 */
		if (realpage == this->pagebuf && !oob_buf) {
			/* aligned read ? */
			if (aligned)
				memcpy (data_poi, this->data_buf, end);
			goto readdata;
		}

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
			nand_increment_rdcnt(this, page);
#endif


		/* Check, if we must send the read command */
		if (sndcmd) {
			this->cmdfunc (mtd, NAND_CMD_READ0, 0x00, page);
			sndcmd = 0;
		}	

		/* get oob area, if we have no oob buffer from fs-driver */
		if (!oob_buf || oobsel->useecc == MTD_NANDECC_AUTOPLACE ||
			oobsel->useecc == MTD_NANDECC_AUTOPL_USR)
			oob_data = &this->data_buf[end];

		eccsteps = this->eccsteps;
		
		switch (eccmode) {
		case NAND_ECC_NONE: {	/* No ECC, Read in a page */
			static unsigned long lastwhinge = 0;
			if ((lastwhinge / HZ) != (jiffies / HZ)) {
				printk (KERN_WARNING "Reading data from NAND FLASH without ECC is not recommended\n");
				lastwhinge = jiffies;
			}
			this->read_buf(mtd, data_poi, end);
			break;
		}
			
		case NAND_ECC_SOFT:	/* Software ECC 3/256: Read in a page + oob data */
			this->read_buf(mtd, data_poi, end);
			for (i = 0, datidx = 0; eccsteps; eccsteps--, i+=3, datidx += ecc) 
				this->calculate_ecc(mtd, &data_poi[datidx], &ecc_calc[i]);
			this->read_buf(mtd, &oob_data[mtd->oobsize - oobreadlen], oobreadlen);
			break;	

		default:
			if (! this->layout) {
				for (i = 0, datidx = 0; eccsteps; eccsteps--, i+=eccbytes, datidx += ecc) {
					this->enable_hwecc(mtd, NAND_ECC_READ);
					this->read_buf(mtd, &data_poi[datidx], ecc);

					/* HW ecc with syndrome calculation must read the
					 * syndrome from flash immidiately after the data */
					if (!compareecc) {
						/* Some hw ecc generators need to know when the
						 * syndrome is read from flash */
						this->enable_hwecc(mtd, NAND_ECC_READSYN);
						this->read_buf(mtd, &oob_data[i], eccbytes);
						/* We calc error correction directly, it checks the hw
						 * generator for an error, reads back the syndrome and
						 * does the error correction on the fly */
						ecc_status = this->correct_data(mtd, &data_poi[datidx], &oob_data[i], &ecc_code[i]);
						if ((ecc_status == -1) || (ecc_status > (flags && 0xff))) {
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
							printk (KERN_NOTICE "nand_read_ecc:%d Failed ECC read, page 0x%08x on chip %d\n",
								__LINE__, page, chipnr);
#else
							DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: "
								"Failed ECC read, page 0x%08x on chip %d\n", page, chipnr);
#endif
							ecc_failed++;
						}
					} else {
						this->calculate_ecc(mtd, &data_poi[datidx], &ecc_calc[i]);
					}	
				}

				this->read_buf(mtd, &oob_data[mtd->oobsize - oobreadlen], oobreadlen);

				break;
			}				

			for (oobidx = 0, datidx = 0, eccidx = 0; eccsteps; eccsteps--) {
				int last_datidx = datidx, last_oobidx = oobidx;
				for (j = 0; this->layout[j].length; j++) {
					int len = this->layout[j].length;
					int oidx = oobidx;
					switch (this->layout[j].type) {
					case ITEM_TYPE_DATA:
						DEBUG (MTD_DEBUG_LEVEL3, "%s: reading %d bytes of data\n", __FUNCTION__, this->layout[j].length);
						this->enable_hwecc(mtd, NAND_ECC_READ);
						this->read_buf(mtd, &data_poi[datidx], len);
						datidx += this->layout[j].length;
						break;

					case ITEM_TYPE_ECC:
						DEBUG (MTD_DEBUG_LEVEL3, "%s: reading %d ecc bytes\n", __FUNCTION__, this->layout[j].length);
						/* let the particular driver decide whether to read ECC */
						this->enable_hwecc(mtd, NAND_ECC_READSYN);
						if (this->options & NAND_BUSWIDTH_16) {
							if (oidx & 1) {
								oidx--;
								len++;
							}
							if (len & 1)
								len--;
						}

						this->read_buf(mtd, &oob_data[oidx], len);
						if (!compareecc) {
							/* We calc error correction directly, it checks the hw
							 * generator for an error, reads back the syndrome and
							 * does the error correction on the fly */
							ecc_status = this->correct_data(mtd, &data_poi[last_datidx], &oob_data[last_oobidx], &ecc_code[eccidx]);
							if ((ecc_status == -1) || (ecc_status > (flags && 0xff))) {
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
								printk (KERN_NOTICE "nand_read_ecc:%d Failed ECC read, page 0x%08x on chip %d\n",
									__LINE__, page, chipnr);
#else
								DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: "
									"Failed ECC read, page 0x%08x on chip %d\n", page, chipnr);
#endif
								ecc_failed++;
							}
						} else
							this->calculate_ecc(mtd, &data_poi[last_datidx], &ecc_calc[eccidx]);
						oobidx += this->layout[j].length;
						eccidx += this->layout[j].length;
						break;
					case ITEM_TYPE_OOB:
						DEBUG (MTD_DEBUG_LEVEL3, "%s: reading %d free oob bytes\n", __FUNCTION__, this->layout[j].length);
						this->enable_hwecc(mtd, NAND_ECC_READOOB);
						if (this->options & NAND_BUSWIDTH_16) {
							if (oidx & 1) {
								oidx--;
								len++;
							}
							if (len & 1)
								len--;
						}

						this->read_buf(mtd, &oob_data[oidx], len);
						oobidx += this->layout[j].length;
						break;
					}
				}
			}
			break;						
		}

		/* Skip ECC check, if not requested (ECC_NONE or HW_ECC with syndromes) */
		if (!compareecc)
			goto readoob;	
		
		/* Pick the ECC bytes out of the oob data */
		for (j = 0; j < oobsel->eccbytes; j++)
			ecc_code[j] = oob_data[oob_config[j]];

		/* correct data, if neccecary */
		for (i = 0, j = 0, datidx = 0; i < this->eccsteps; i++, datidx += ecc) {
			ecc_status = this->correct_data(mtd, &data_poi[datidx], &ecc_code[j], &ecc_calc[j]);
			
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
			/* read disturb error checking ... */
			if ((ecc_status == 1)&&(rddist_block)) {
				printk (KERN_NOTICE "nand_read_ecc:%d 1bit ECC error fixed at page 0x%08x\n",__LINE__, page);
				*rddist_block = (realpage<<this->page_shift)>>this->phys_erase_shift;
				/* set the ecc_status to 0, since the block is fixed to prevent yaffs 
				 * from marking the block as bad
				 */
				ecc_status = 0;
			}
#endif
			/* Get next chunk of ecc bytes */
			j += eccbytes;
			
			/* Check, if we have a fs supplied oob-buffer, 
			 * This is the legacy mode. Used by YAFFS1
			 * Should go away some day
			 */
			if (oob_buf && oobsel->useecc == MTD_NANDECC_PLACE) { 
				int *p = (int *)(&oob_data[mtd->oobsize]);
				p[i] = ecc_status;
			}

			if ((ecc_status == -1) || (ecc_status > (flags && 0xff))) {
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
				printk (KERN_NOTICE "nand_read_ecc:%d Failed ECC read, page 0x%08x\n",__LINE__, page);
#else
				DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: " "Failed ECC read, page 0x%08x\n", page);
#endif
				ecc_failed++;
			}
		}		

	readoob:
		/* check, if we have a fs supplied oob-buffer */
		if (oob_buf) {
			/* without autoplace. Legacy mode used by YAFFS1 */
			switch(oobsel->useecc) {
			case MTD_NANDECC_AUTOPLACE:
			case MTD_NANDECC_AUTOPL_USR:
				/* Walk through the autoplace chunks */
				for (i = 0; oobsel->oobfree[i][1]; i++) {
					int from = oobsel->oobfree[i][0];
					int num = oobsel->oobfree[i][1];
					memcpy(&oob_buf[oob], &oob_data[from], num);
					oob += num;
				}
				break;
			case MTD_NANDECC_PLACE:
				/* YAFFS1 legacy mode */
				oob_data += this->eccsteps * sizeof (int);
			default:
				oob_data += mtd->oobsize;
			}
		}
	readdata:
		/* Partial page read, transfer data into fs buffer */
		if (!aligned) { 
			for (j = col; j < end && read < len; j++)
				buf[read++] = data_poi[j];
			this->pagebuf = realpage;	
		} else		
			read += mtd->oobblock;

		/* Apply delay or wait for ready/busy pin 
		 * Do this before the AUTOINCR check, so no problems
		 * arise if a chip which does auto increment
		 * is marked as NOAUTOINCR by the board driver.
		*/
		if (!this->dev_ready) 
			udelay (this->chip_delay);
		else
			nand_wait_ready(mtd);
			
		if (read == len)
			break;	

		/* For subsequent reads align to page boundary. */
		col = 0;
		/* Increment page address */
		realpage++;

		page = realpage & this->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			this->select_chip(mtd, -1);
			this->select_chip(mtd, chipnr);
		}
		/* Check, if the chip supports auto page increment 
		 * or if we have hit a block boundary. 
		*/ 
		if (!NAND_CANAUTOINCR(this) || !(page & blockcheck))
			sndcmd = 1;				
#if defined(CONFIG_MTD_NAND_BBM)
		/* Increment page address */
		page_cnt++;
		/* do bad block check, if we have hit a block boundary */
		if (!(page & blockcheck)) {
			// check, if it is in a replacement block 
			if (block_replace_flag) {
				// set page back to original block 
				realpage = (int) (from >> this->page_shift) + page_cnt;
				block_replace_flag = 0;
			}
			// check, if the given start page is in bad block
			if (nand_isbad_bbt (mtd, (realpage << this->page_shift), 0)) {
				from_rb = nand_translate_block(mtd, (realpage << this->page_shift));
				if (from_rb != (realpage << this->page_shift)) {
					realpage = (int) (from_rb >> this->page_shift);
					block_replace_flag = 1;
				}
			}
			page = realpage & this->pagemask;
		}
#endif
	}

	/* Deselect and wake up anyone waiting on the device */
	if (flags & NAND_GET_DEVICE)
		nand_release_device(mtd);

	/*
	 * Return success, if no ECC failures, else -EBADMSG
	 * fs driver will take care of that, because
	 * retlen == desired len and result == -EBADMSG
	 */
	*retlen = read;
	return ecc_failed ? -EBADMSG : 0;
}

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
/***
 * nand_rddist_check  -  [NAND internal function]
 * @mtd:        MTD device structure
 * @offset:	offset to the block
 *
 * return:	1: was read disturb block
 *		0: not read disturb block
 */
static int nand_rddist_check(struct mtd_info* mtd, loff_t ofs)
{
	struct nand_chip *this = mtd->priv;
	int i, rd_dist_block = (int) (ofs >> this->phys_erase_shift);

	for (i=0; i<MAX_RDDIST_BLOCKS; i++)
		if (rd_dist_block == nand_rddist_block[i])
			/* it is read disturb block */
			return 1;
	/* not read disturb block */
	return 0;
}

/**
 * nand_read_dist_fix - [NAND internal function]
 * @mtd:	MTD device structure
 * @rd_dist_block: given block where read disturb fix will be performed
 * @reason_code:   two reasons for doing read disturb fix.
 *		   RDDIST_BITERR - given block 1bit error detected
 *		   RDDIST_CNTFIX - given block exceeded read access threshold.
 *
 * This function fixes the read disturb error after the rd_dist_block be set by
 * nand_do_read_ecc.
 */
static int nand_read_distfix (struct mtd_info *mtd, int rd_dist_block, int reason_code) 
{
	loff_t from, to;
	struct nand_chip *this = mtd->priv;
	struct nand_oobinfo *oobsel = &mtd->oobinfo;
	u_char *mainbuf, *oobbuf, *rdbackbuf, *rdbackoob;
	size_t rlen;
	int i, ret=-1, status, numpages, rsvdblock_idx, rdbackblock=0, numoobfree=0;

	DEBUG(MTD_DEBUG_LEVEL0,"nand_read_distfix: block %d need to be fixed\n",rd_dist_block);

	/*+++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 1 - allocate all needed memory.
	 *++++++++++++++++++++++++++++++++++++++++++++++++*/

	/* step 1.1 - allocate block size of memory for storing one block main data */
	mainbuf = vmalloc(mtd->erasesize);
	if (!mainbuf) {
		printk (KERN_ERR "mainbuf - Out of memory (size %d)\n",mtd->erasesize);
		return -ENOMEM;
	}
	memset(mainbuf, 0x0, mtd->erasesize);

	/* step 1.2 - allocate block size of memory for storing one block read back data */
	rdbackbuf = vmalloc(mtd->erasesize);
	if (!rdbackbuf) {
		printk (KERN_ERR "rdbackbuf - Out of memory (size %d)\n",mtd->erasesize);
		vfree(mainbuf);
		return -ENOMEM;
	}
	memset(rdbackbuf, 0x0, mtd->erasesize);

	/* step 1.3 - allocate block size of memory for storing one block oob data */
	numpages = mtd->erasesize/mtd->oobblock;

	/* calculate the oob free bytes for one page:
	 *
	 * NOTE: The oobfree info is based on the master mtd structure.
	 * 	 It was the super set of all the slave mtd partitions.
	 * 	 Using the master mtd structure of oob free info will
	 * 	 cover all slave mtd partitions needs. 
	 */
	for (i = 0; oobsel->oobfree[i][1]; i++)
		numoobfree += (int)oobsel->oobfree[i][1];
	
	oobbuf = vmalloc(numoobfree*numpages);
	if (!oobbuf) {
		printk (KERN_ERR "mainbuf - Out of memory (size %d)\n",
			numoobfree*numpages);
		vfree(mainbuf);
		vfree(rdbackbuf);
		return -ENOMEM;
	}
	memset(oobbuf, 0x0, numoobfree*numpages);

	/* step 1.4 - allocate block size of memory for storing one block oob data */
	rdbackoob = vmalloc(numoobfree*numpages);
	if (!rdbackoob) {
		printk (KERN_ERR "mainbuf - Out of memory (size %d)\n",
			numoobfree*numpages);
		vfree(mainbuf);
		vfree(rdbackbuf);
		vfree(oobbuf);
		return -ENOMEM;
	}
	memset(rdbackoob, 0x0, numoobfree*numpages);

	/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 2 - calculate the block offset for rd_dist_block
	 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	from = (loff_t) (rd_dist_block << this->phys_erase_shift);
	
	/* step 2.1 - Check for block replacement if the given page is in bad block */
	if (nand_isbad_bbt (mtd, from, 0))
		from = nand_translate_block(mtd, from);

	rd_dist_block = (int) (from >> this->phys_erase_shift);
	rsvdblock_idx = get_bbm_index(mtd, this->bbm);
	if (rsvdblock_idx == -1)
	{
		printk(KERN_WARNING "no more reserved block available.\n");
		goto done;
	}
	DEBUG(MTD_DEBUG_LEVEL1,"blockoffset:%08lx, read_dist_block:%d, " 
			"rsvdblock_offset:%08lx, rsvdblock_idx:%d\n", 
			(unsigned long)from, rd_dist_block, 
			(unsigned long)rsvdblock_offset,  rsvdblock_idx);
	
	/****
	 * NOTE:
	 * To make the whole operations for read disturb fix need to be atomic 
	 * to prevent race condition, set nand device in FL_RDDIST_FIXING state
	 */
	nand_get_device (this, mtd, FL_RDDIST_FIXING);

	/*++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 3 - read the whole block into memory - mainbuf 
	 *++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	ret = nand_do_read_ecc(mtd, from, mtd->erasesize, &rlen, mainbuf, oobbuf, &mtd->oobinfo, 0xff, NULL);
	if (ret < 0) {
		printk (KERN_ERR "step 3:nand_do_read_ecc failed on offset: 0x%08lx\n", (unsigned long)from);
		goto done;
	}

	/*++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 4 - save the data into reserved block
	 *+++++++++++++++++++++++++++++++++++++++++++++++++++*/
	
	/* step 4.1 - do block erase first on the reserved block */
	to = (loff_t)rsvdblock_offset + (((loff_t)rsvdblock_idx)<<this->phys_erase_shift);
	nand_get_device (this, mtd, FL_ERASING);
	this->erase_cmd(mtd, to >> this->page_shift); 
	status = this->waitfunc (mtd, this, FL_ERASING);
	nand_release_device(mtd);
	if (status & NAND_STATUS_FAIL) {
		printk (KERN_ERR "step 4.1:nand_erase_cmd failed on offset: 0x%08lx\n", (unsigned long)to);
		/* mark this reserved block as bad blopck */ 
		nand_do_block_mark(mtd, to, 0x01);
		goto done;
	}
	/* step 4.2 - writing the data to reserved block ... */
	ret = nand_write_ecc(mtd, to, mtd->erasesize, &rlen, mainbuf, oobbuf, &mtd->oobinfo);
	if (ret < 0) {
		printk (KERN_ERR "step 4.2:nand_write_ecc failed on offset: 0x%08lx\n", (unsigned long)to);
		/* mark this reserved block as bad blopck */
		nand_do_block_mark(mtd, to, 0x01);
		goto done;
	}

	/* step 4.3 - read back the data and comparing with the original data */
	nand_do_read_ecc(mtd, to, mtd->erasesize, &rlen, rdbackbuf, rdbackoob, &mtd->oobinfo, 0xff, NULL);
	if (memcmp(mainbuf, rdbackbuf, mtd->erasesize) != 0) {
		printk (KERN_ERR "step 4.3:block %d - data read back check failed on offset: 0x%08lx\n",
			rd_dist_block, (unsigned long)to);
		/* mark this reserved block as bad blopck */
		nand_do_block_mark(mtd, to, 0x01);
		goto done;
	}
	if (memcmp(oobbuf, rdbackoob, numoobfree*numpages) != 0) {
		printk (KERN_ERR "step 4.3:block %d  - oob read back check failed on offset: 0x%08lx\n",
			rd_dist_block, (unsigned long)to);
		/* mark this reserved block as bad blopck */
		nand_do_block_mark(mtd, to, 0x01);
		goto done;
	}

	/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 5 - update BBM to temporailly create map for rd_dist_block
	 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	this->bbm[rd_dist_block] = rsvdblock_idx;
	ret = update_bbm(mtd, (uint8_t*)this->bbm, to>>this->chip_shift);
	if (ret < 0) {
		printk(KERN_ERR "step 5:update_bbm() failed\n");
		goto done;
	}

	/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 6 - check nand_rddist_block array to see if rd_dist_block was in the array
	 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	if (reason_code == RDDIST_BITERR)
	    for (i=0; i<MAX_RDDIST_BLOCKS; i++) {
		if (rd_dist_block == nand_rddist_block[i]) {
			/* this block was fixed before, 
			 * now, it is the time to mark as permanetal bad block.
			 */ 
			printk(KERN_INFO "step 6:block %d should be retired and marked as bad block.\n", 
				rd_dist_block);
			nand_do_block_mark(mtd, from, 0x01);
			nand_rddist_block[i] = 0;
			goto cleanup;
		}
	    }

	/* put the nand_rddist_block into circular array */
	nand_rddist_block[nand_rddist_block_cnt] = rd_dist_block;

	/****
	 * NOTE:
	 * if power failure occured between step 7 to step 11, the rd_dist_block will 
	 * be marked as bad block permanently.
	 */

	/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 7 - update BBT to mark the rd_dist_block as bad block 
	 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	ret = nand_do_block_mark(mtd, from, 0x01);
	if (ret < 0) {
		printk(KERN_ERR "step 7:nand_do_block_mark() to mark as bad block failed.\n");
		goto done;
	}
	/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 8 - erase the read disturb error block for re-flash 
	 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	nand_get_device (this, mtd, FL_ERASING);
	this->erase_cmd(mtd, from >> this->page_shift);
	status = this->waitfunc (mtd, this, FL_ERASING);
	nand_release_device(mtd);
	if (status & NAND_STATUS_FAIL) {
		printk (KERN_ERR "step 8:nand_erase_cmd failed on offset: 0x%08lx\n", (unsigned long)to);
		/* make the read disturb error block as permanetal bad block, replaced with
		 * replaced with a good block from reserved pool */
		goto done;
	}

	/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 9 - re-write back to the recovered read disturb block 
	 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	ret = nand_write_ecc(mtd, from, mtd->erasesize, &rlen, mainbuf, oobbuf, &mtd->oobinfo);
	if (ret < 0) {
		printk (KERN_ERR "step 9:write operation failed! block %d retired.\n", 
			(unsigned int) (from>>this->phys_erase_shift));
		/* make the read disturb error block as permanetal bad block, replaced with
		 * replaced with a good block from reserved pool */ 
		goto done;
	}

	/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 10 - read back the data and comparing with the original data
	 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	memset(rdbackbuf, 0x00, mtd->erasesize);
	nand_do_read_ecc(mtd, from, mtd->erasesize, &rlen, rdbackbuf, rdbackoob, &mtd->oobinfo, 0xff, &rdbackblock);
	/* check to see if we have permantally correctable ECC error in same block 
	 * if so, mark it as bad block permanetly and replace with one from reserved pool 
	 */
	if (rdbackblock == rd_dist_block) {
		printk (KERN_ERR "step 10:block %d retired with permantally correctable ECC error\n", rd_dist_block);
		/* make the read disturb error block as permanetal bad block, replaced with
		 * replaced with a good block from reserved pool */
		 goto done;
	}
	if (memcmp(mainbuf, rdbackbuf, mtd->erasesize) != 0) {
		printk (KERN_ERR "step 10:block %d  - data read back check failed on offset: 0x%08lx\n",
			rd_dist_block, (unsigned long)to);
		/* make the read disturb error block as permanetal bad block, replaced with
		 * replaced with a good block from reserved pool */
		goto done;
	}
	if (memcmp(oobbuf, rdbackoob, numoobfree*numpages) != 0) {
		printk (KERN_ERR "step 10:block %d  - oob read back check failed on offset: 0x%08lx\n",
			rd_dist_block, (unsigned long)to);
		/* make the read disturb error block as permanetal bad block, replaced with
		 * replaced with a good block from reserved pool */
		goto done;
	}

	/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 * step 11 - update BBT to re-claim the rd_dist_block as fixed block 
	 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	ret = nand_do_block_mark(mtd, from, 0x00);
	if (ret < 0) {
		printk(KERN_ERR "step 11:nand_do_block_mark() to re-claim as fixed block failed.\n");
		goto done;
	}
	/*++++++++++++++++++++++++++++++++++++++
	 * step 12 - update the BBM for clean up 
	 *+++++++++++++++++++++++++++++++++++++*/
	this->bbm[rd_dist_block] = 0xffff;
	ret = update_bbm(mtd, (uint8_t*)this->bbm, from>>this->chip_shift);
	if (ret < 0) {
		printk(KERN_ERR "step 12:update_bbm() failed\n");
		goto done;
	}
	printk(KERN_INFO "read disturb on block %d has been fixed.\n",rd_dist_block);

	/* reset the nand_rddist_block[nand_rddist_block_cnt]
	 * if reason_code is RDDIST_CNTFIX, otherwise update
	 * nand_rddist_block_cnt */
	if (reason_code == RDDIST_CNTFIX)
		nand_rddist_block[nand_rddist_block_cnt] = 0;
	else {
		nand_rddist_block_cnt++;
		nand_rddist_block_cnt %= MAX_RDDIST_BLOCKS;
	}
	goto cleanup;

done:	
	/*++++++++++++++++++++++++++++++++++
	 * step 13.1 - done with error cases
	 *+++++++++++++++++++++++++++++++++*/
	 for (i=0; i<MAX_RDDIST_BLOCKS; i++)
	 	nand_rddist_block[i] = 0;
cleanup:
	/*+++++++++++++++++
	 * step 13.2 - done!
	 *++++++++++++++++*/
	/* clean up the resource */
	if (mainbuf) vfree(mainbuf);
	if (rdbackbuf) vfree(rdbackbuf);
	if (oobbuf) vfree(oobbuf);
	if (rdbackoob) vfree(rdbackoob);
	/* release the nand device */
	rddist_in_progress = 0;
	nand_release_device(mtd);
	return ret;
}
#endif /*CONFIG_MOT_FEAT_NAND_RDDIST*/

/**
 * nand_read_oob - [MTD Interface] NAND read out-of-band
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 *
 * NAND read out-of-band data from the spare area
 */
static int nand_read_oob (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	int i, col, page, chipnr;
	struct nand_chip *this = mtd->priv;
	int	blockcheck = (1 << (this->phys_erase_shift - this->page_shift)) - 1;

	DEBUG (MTD_DEBUG_LEVEL3, "nand_read_oob: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

#if defined(CONFIG_MTD_NAND_BBM)
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	if (nand_rddist_check(mtd, from))
		/* read disturb fix inprogress, don't do block replacement */
		DEBUG (MTD_DEBUG_LEVEL0, "no block replacement\n");
	else 
#endif
	/* Check for block replacement if the given page is in bad block */
	if (nand_isbad_bbt (mtd, from, 0)) {
		from = nand_translate_block(mtd, from);
	}
#endif
	/* Shift to get page */
	page = (int)(from >> this->page_shift);
	chipnr = (int)(from >> this->chip_shift);
	
	/* Mask to get column */
	col = from & (mtd->oobsize - 1);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_read_oob: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_device (this, mtd , FL_READING);

	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	nand_increment_rdcnt(this, page);
#endif
	/* Send the read command */
	this->cmdfunc (mtd, NAND_CMD_READOOB, col, page & this->pagemask);
	/* 
	 * Read the data, if we read more than one page
	 * oob data, let the device transfer the data !
	 */
	i = 0;
	while (i < len) {
		int thislen = mtd->oobsize - col;
		thislen = min_t(int, thislen, len);
		this->read_buf(mtd, &buf[i], thislen);
		i += thislen;
		
		/* Apply delay or wait for ready/busy pin 
		 * Do this before the AUTOINCR check, so no problems
		 * arise if a chip which does auto increment
		 * is marked as NOAUTOINCR by the board driver.
		*/
		if (!this->dev_ready) 
			udelay (this->chip_delay);
		else
			nand_wait_ready(mtd);

		/* Read more ? */
		if (i < len) {
			page++;
			col = 0;

			/* Check, if we cross a chip boundary */
			if (!(page & this->pagemask)) {
				chipnr++;
				this->select_chip(mtd, -1);
				this->select_chip(mtd, chipnr);
			}
				
			/* Check, if the chip supports auto page increment 
			 * or if we have hit a block boundary. 
			*/ 
			if (!NAND_CANAUTOINCR(this) || !(page & blockcheck)) {
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
				nand_increment_rdcnt(this, page);
#endif
				/* For subsequent page reads set offset to 0 */
			        this->cmdfunc (mtd, NAND_CMD_READOOB, 0x0, page & this->pagemask);
			}
		}
	}

	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(mtd);

	/* Return happy */
	*retlen = len;
	return 0;
}

/**
 * nand_read_oob_hwecc - [MTD Interface] NAND read out-of-band (HW ECC)
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @oob_buf:	the databuffer to put data
 *
 * NAND read out-of-band data from the spare area
 * W/o assumptions that are valid only for software ECC
 */
static int nand_read_oob_hwecc (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * oob_buf)
{

	int i, j, col, realpage, page, end, ecc, chipnr, sndcmd = 1, reallen = 0;
	int read = 0;
	struct nand_chip *this = mtd->priv;
	u_char *oob_data = oob_buf;
        int 	eccsteps;
	int	blockcheck = (1 << (this->phys_erase_shift - this->page_shift)) - 1;


	DEBUG (MTD_DEBUG_LEVEL3, "%s: from = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) from, (int) len);

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size) {
		*retlen = 0;
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_device (this, mtd, FL_READING);

	/* Select the NAND device */
	chipnr = (int)(from >> this->chip_shift);
	this->select_chip(mtd, chipnr);

	/* First we calculate the starting page */
	realpage = (int) (from >> this->page_shift);
	page = realpage & this->pagemask;

	/* Get raw starting column */
	col = from & (mtd->oobblock - 1);

	end = mtd->oobblock;
	ecc = this->eccsize;
	
	/* Loop until all data read */
	while (read < len) {
		/* Check, if we must send the read command */
		if (sndcmd) {
			this->cmdfunc (mtd, NAND_CMD_READ0, 0x00, page);
			sndcmd = 0;
		}	

		eccsteps = this->eccsteps;
		
		for (; eccsteps; eccsteps--) {
			for (j = 0; this->layout[j].length; j++) {
				i = 0;
				switch (this->layout[j].type) {
				case ITEM_TYPE_DATA:
					DEBUG (MTD_DEBUG_LEVEL3, "%s: dummy data read\n", __FUNCTION__);
					reallen += this->layout[j].length;
					if (this->options & NAND_BUSWIDTH_16) 
						this->cmdfunc (mtd, NAND_CMD_READ0, reallen & ~1, page);
					else
						this->cmdfunc (mtd, NAND_CMD_READ0, reallen, page);
					break;

				case ITEM_TYPE_ECC:
				case ITEM_TYPE_OOB:
					DEBUG (MTD_DEBUG_LEVEL3, "%s: %s bytes read\n", __FUNCTION__, this->layout[j].type == ITEM_TYPE_ECC ? "ecc" : "oob");
					i = min_t(int, col, this->layout[j].length);
					if (i) {
						reallen += i;
						if (this->options & NAND_BUSWIDTH_16)
							this->cmdfunc (mtd, NAND_CMD_READ0, reallen & ~1, page);
						else
							this->cmdfunc (mtd, NAND_CMD_READ0, reallen, page);
					}
					col -= i;

					if (this->layout[j].type == ITEM_TYPE_ECC)
						this->enable_hwecc(mtd, NAND_ECC_READSYN);
					else
						this->enable_hwecc(mtd, NAND_ECC_READOOB);
					i = min_t(int, len - read, this->layout[j].length - i);
					if (i) {
						if (this->options & NAND_BUSWIDTH_16) {
							if (reallen & 1) {
								oob_data[0] = cpu_to_le16(this->read_word(mtd)) >> 8;
								oob_data++; i--; reallen++;
							}
							if (i & 1)
								this->read_buf(mtd, oob_data, i - 1);
							else
								this->read_buf(mtd, oob_data, i);
 						}
						else
							this->read_buf(mtd, oob_data, i);
						reallen += i;
					}
					if (oob_buf + len == oob_data + i) {
						read += i;
						goto out;
	 				}
					break;
				}
				read += i;
				oob_data += i;

			}
		}		
out:

		/* Apply delay or wait for ready/busy pin 
		 * Do this before the AUTOINCR check, so no problems
		 * arise if a chip which does auto increment
		 * is marked as NOAUTOINCR by the board driver.
		*/
		if (!this->dev_ready) 
			udelay (this->chip_delay);
		else
			nand_wait_ready(mtd);
			
		if (read == len)
			break;	

		/* For subsequent reads align to page boundary. */
		reallen = col = 0;
		/* Increment page address */
		realpage++;

		page = realpage & this->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			this->select_chip(mtd, -1);
			this->select_chip(mtd, chipnr);
		}
		/* Check, if the chip supports auto page increment 
		 * or if we have hit a block boundary. 
		*/ 
		if (!NAND_CANAUTOINCR(this) || !(page & blockcheck))
			sndcmd = 1;				
	}
	
	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(mtd);

	*retlen = read;
	/*
	 * Return success
	 */
	return 0;

}


/**
 * nand_read_raw - [GENERIC] Read raw data including oob into buffer
 * @mtd:	MTD device structure
 * @buf:	temporary buffer
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @ooblen:	number of oob data bytes to read
 *
 * Read raw data including oob into buffer
 */
int nand_read_raw (struct mtd_info *mtd, uint8_t *buf, loff_t from, size_t len, size_t ooblen)
{
	struct nand_chip *this = mtd->priv;
	int page = (int) (from >> this->page_shift);
	int chip = (int) (from >> this->chip_shift);
	int sndcmd = 1;
	int cnt = 0;
	int pagesize = mtd->oobblock + mtd->oobsize;
	int	blockcheck = (1 << (this->phys_erase_shift - this->page_shift)) - 1;

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_read_raw: Attempt read beyond end of device\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_device (this, mtd , FL_READING);

	this->select_chip (mtd, chip);
	
	/* Add requested oob length */
	len += ooblen;
	
	while (len) {
		if (sndcmd) {
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
			nand_increment_rdcnt(this, page);
#endif
			this->cmdfunc (mtd, NAND_CMD_READ0, 0, page & this->pagemask);
		}
		sndcmd = 0;	

		this->read_buf (mtd, &buf[cnt], pagesize);

		len -= pagesize;
		cnt += pagesize;
		page++;
		
		if (!this->dev_ready) 
			udelay (this->chip_delay);
		else
			nand_wait_ready(mtd);
			
		/* Check, if the chip supports auto page increment */ 
		if (!NAND_CANAUTOINCR(this) || !(page & blockcheck))
			sndcmd = 1;
	}

	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(mtd);
	return 0;
}


/** 
 * nand_prepare_oobbuf - [GENERIC] Prepare the out of band buffer 
 * @mtd:	MTD device structure
 * @fsbuf:	buffer given by fs driver
 * @oobsel:	out of band selection structre
 * @autoplace:	1 = place given buffer into the oob bytes
 * @numpages:	number of pages to prepare
 *
 * Return:
 * 1. Filesystem buffer available and autoplacement is off,
 *    return filesystem buffer
 * 2. No filesystem buffer or autoplace is off, return internal
 *    buffer
 * 3. Filesystem buffer is given and autoplace selected
 *    put data from fs buffer into internal buffer and
 *    retrun internal buffer
 *
 * Note: The internal buffer is filled with 0xff. This must
 * be done only once, when no autoplacement happens
 * Autoplacement sets the buffer dirty flag, which
 * forces the 0xff fill before using the buffer again.
 *
*/
static u_char * nand_prepare_oobbuf (struct mtd_info *mtd, u_char *fsbuf, struct nand_oobinfo *oobsel,
		int autoplace, int numpages)
{
	struct nand_chip *this = mtd->priv;
	int i, len, ofs;

	/* Zero copy fs supplied buffer */
	if (fsbuf && !autoplace) 
		return fsbuf;

	/* Check, if the buffer must be filled with ff again */
	if (this->oobdirty) {	
		memset (this->oob_buf, 0xff, 
			mtd->oobsize << (this->phys_erase_shift - this->page_shift));
		this->oobdirty = 0;
	}	
	
	/* If we have no autoplacement or no fs buffer use the internal one */
	if (!autoplace || !fsbuf)
		return this->oob_buf;
	
	/* Walk through the pages and place the data */
	this->oobdirty = 1;
	ofs = 0;
	while (numpages--) {
		for (i = 0, len = 0; len < mtd->oobavail; i++) {
			int to = ofs + oobsel->oobfree[i][0];
			int num = oobsel->oobfree[i][1];
			memcpy (&this->oob_buf[to], fsbuf, num);
			len += num;
			fsbuf += num;
		}
#ifdef CONFIG_MOT_WFN457
		ofs += mtd->oobsize;
#else
		ofs += mtd->oobavail;
#endif
	}
	return this->oob_buf;
}

#define NOTALIGNED(x) (x & (mtd->oobblock-1)) != 0

/**
 * nand_write - [MTD Interface] compability function for nand_write_ecc
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @buf:	the data to write
 *
 * This function simply calls nand_write_ecc with oob buffer and oobsel = NULL
 *
*/
static int nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char * buf)
{
	return (nand_write_ecc (mtd, to, len, retlen, buf, NULL, NULL));
}
			   
/**
 * nand_write_ecc - [MTD Interface] NAND write with ECC
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @buf:	the data to write
 * @eccbuf:	filesystem supplied oob data buffer
 * @oobsel:	oob selection structure
 *
 * NAND write with ECC
 */
static int nand_write_ecc (struct mtd_info *mtd, loff_t to, size_t len,
			   size_t * retlen, const u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel)
{
	int startpage, page, ret = -EIO, oob = 0, written = 0, chipnr;
	int autoplace = 0, numpages, totalpages;
	struct nand_chip *this = mtd->priv;
	u_char *oobbuf, *bufstart;
	int	ppblock = (1 << (this->phys_erase_shift - this->page_shift));
#if defined(CONFIG_MTD_NAND_BBM)
	loff_t 	to_rb;
	int block_replace_flag = 0;
#endif
#ifdef CONFIG_MOT_FEAT_KPANIC
	int status;
#endif /* CONFIG_MOT_FEAT_KPANIC */

	DEBUG (MTD_DEBUG_LEVEL3, "nand_write_ecc: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Initialize retlen, in case of early exit */
	*retlen = 0;

	/* Do not allow write past end of device */
	if ((to + len) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: Attempt to write past end of page\n");
		return -EINVAL;
	}

	/* reject writes, which are not page aligned */	
	if (NOTALIGNED (to) || NOTALIGNED(len)) {
		printk (KERN_NOTICE "nand_write_ecc: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

#ifdef CONFIG_MOT_FEAT_KPANIC
        if (kpanic_in_progress) {
                status = knand_get_device (this, mtd, FL_WRITING);
                if (status)
                        return -EBUSY;
        }
        else
#endif /* CONFIG_MOT_FEAT_KPANIC */
	/* Grab the lock and see if the device is available */
	nand_get_device (this, mtd, FL_WRITING);

	/* Calculate chipnr */
	chipnr = (int)(to >> this->chip_shift);
	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Check, if it is write protected */
	if (nand_check_wp(mtd))
		goto out;

	/* if oobsel is NULL, use chip defaults */
	if (oobsel == NULL) 
		oobsel = &mtd->oobinfo;		
		
	/* Autoplace of oob data ? Use the default placement scheme */
	if (oobsel->useecc == MTD_NANDECC_AUTOPLACE) {
		oobsel = this->autooob;
		autoplace = 1;
	}
	if (oobsel->useecc == MTD_NANDECC_AUTOPL_USR)
		autoplace = 1;

	/* Setup variables and oob buffer */
	totalpages = len >> this->page_shift;
	page = (int) (to >> this->page_shift);

#if defined(CONFIG_MTD_NAND_BBM)
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	if (nand_rddist_check(mtd, to))
		/* read disturb fix inprogress, don't do block replacement */
		DEBUG (MTD_DEBUG_LEVEL0, "no block replacement\n");
	else 
#endif
	// Check for block replacement if the given page is in bad block 
	if (nand_isbad_bbt (mtd, to, 0)) {
		to_rb = nand_translate_block(mtd, to);
		if (to_rb != to ) {
			DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: badblock offset:0x%08x, replblock offset:0x%08x\n",
				(unsigned int)to, (unsigned int)to_rb);
			page = (int) (to_rb >> this->page_shift);
			block_replace_flag = 1;
		}
	}
#endif
#ifdef CONFIG_MOT_FEAT_NAND_BLKCNT_TEST
	/* keep tracking the g_write_test_info.blk_tbl */
	if ((g_write_test_info.threshold)&&
	   ((g_write_test_info.blk_tbl[(page<<this->page_shift)>>this->phys_erase_shift]++)>=g_write_test_info.threshold)){
		g_write_test_info.threshold_flag = 1;
		wake_up_interruptible(&g_blkwt_waitq);
		printk(KERN_INFO "block %d exceed write access threshold %d.\n",
			(page<<this->page_shift)>>this->phys_erase_shift, g_write_test_info.threshold);
	}
#endif

	/* Invalidate the page cache, if we write to the cached page */
	if (page <= this->pagebuf && this->pagebuf < (page + totalpages))  
		this->pagebuf = -1;
	
	/* Set it relative to chip */
	page &= this->pagemask;
	startpage = page;
	/* Calc number of pages we can write in one go */
	numpages = min (ppblock - (startpage  & (ppblock - 1)), totalpages);
	oobbuf = nand_prepare_oobbuf (mtd, eccbuf, oobsel, autoplace, numpages);
	bufstart = (u_char *)buf;

	/* Loop until all data is written */
	while (written < len) {

		this->data_poi = (u_char*) &buf[written];
		/* Write one page. If this is the last page to write
		 * or the last page in this block, then use the
		 * real pageprogram command, else select cached programming
		 * if supported by the chip.
		 */
		ret = nand_write_page (mtd, this, page, &oobbuf[oob], oobsel, (--numpages > 0));
		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: write_page failed %d\n", ret);
			goto out;
		}	
		/* Next oob page */
		oob += mtd->oobsize;
		/* Update written bytes count */
		written += mtd->oobblock;
		if (written == len) 
			goto cmp;
		
		/* Increment page address */
		page++;

		/* Have we hit a block boundary ? Then we have to verify and
		 * if verify is ok, we have to setup the oob buffer for
		 * the next pages.
		*/
		if (!(page & (ppblock - 1))){
			int ofs;
			this->data_poi = bufstart;
			ret = nand_verify_pages (mtd, this, startpage, 
				page - startpage,
				oobbuf, oobsel, chipnr, (eccbuf != NULL));
			if (ret) {
				DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: verify_pages failed %d\n", ret);
				goto out;
			}	
			*retlen = written;

			ofs = autoplace ? mtd->oobavail : mtd->oobsize;
			if (eccbuf)
				eccbuf += (page - startpage) * ofs;
			totalpages -= page - startpage;
			numpages = min (totalpages, ppblock);

#if defined(CONFIG_MTD_NAND_BBM)
			// check, if it is in a replacement block 
			if (block_replace_flag) {
				// set page back to original block 
				page = (int) (to >> this->page_shift) + numpages;
				block_replace_flag = 0;
			}
			// check, if the given start page is in a bad block
			if (nand_isbad_bbt (mtd, (page << this->page_shift), 0)) {
				to_rb = nand_translate_block(mtd, (page << this->page_shift));
				if (to_rb != (page << this->page_shift)) {
					block_replace_flag = 1;
					page = (int) (to_rb >> this->page_shift);
				}
			}
#endif
#ifdef CONFIG_MOT_FEAT_NAND_BLKCNT_TEST
			/* keep tracking the g_write_test_info.blk_tbl */
			if ((g_write_test_info.threshold)&&
			   ((g_write_test_info.blk_tbl[(page<<this->page_shift)>>this->phys_erase_shift]++)>=g_write_test_info.threshold)) {
				g_write_test_info.threshold_flag = 1;
				wake_up_interruptible(&g_blkwt_waitq);
				printk(KERN_INFO "block %d exceed write access threshold %d.\n",
					(page<<this->page_shift)>>this->phys_erase_shift, g_write_test_info.threshold);
			}
#endif
			page &= this->pagemask;
			startpage = page;
			oobbuf = nand_prepare_oobbuf (mtd, eccbuf, oobsel, 
					autoplace, numpages);
			oob = 0;
			/* Check, if we cross a chip boundary */
			if (!page) {
				chipnr++;
				this->select_chip(mtd, -1);
				this->select_chip(mtd, chipnr);
			}
		}
	}
	/* Verify the remaining pages */
cmp:
	this->data_poi = bufstart;
 	ret = nand_verify_pages (mtd, this, startpage, totalpages,
		oobbuf, oobsel, chipnr, (eccbuf != NULL));
	if (!ret)
		*retlen = written;
	else	
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: verify_pages failed %d\n", ret);

out:
	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(mtd);

	return ret;
}


/**
 * nand_write_oob - [MTD Interface] NAND write out-of-band
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @buf:	the data to write
 *
 * NAND write out-of-band
 */
static int nand_write_oob (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char * buf)
{
	int column, page, status, ret = -EIO, chipnr;
	struct nand_chip *this = mtd->priv;

	DEBUG (MTD_DEBUG_LEVEL3, "nand_write_oob: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

#if defined(CONFIG_MTD_NAND_BBM)
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	if (nand_rddist_check(mtd, to))
		/* read disturb fix inprogress, don't do block replacement */
		DEBUG (MTD_DEBUG_LEVEL0, "no block replacement\n");
	else 
#endif
	/* Check for block replacement if the given page is in bad block */
	if (nand_isbad_bbt (mtd, to, 0)) {
		to = nand_translate_block(mtd, to);
	}
#endif
#ifdef CONFIG_MOT_FEAT_NAND_BLKCNT_TEST
	/* keep tracking the g_write_test_info.blk_tbl */
	if ((g_write_test_info.threshold)&&
	   ((g_write_test_info.blk_tbl[to>>this->phys_erase_shift]++)>=g_write_test_info.threshold)){
		g_write_test_info.threshold_flag = 1;
		wake_up_interruptible(&g_blkwt_waitq);
		printk(KERN_INFO "block %d exceed write access threshold %d.\n",
			to>>this->phys_erase_shift, g_write_test_info.threshold);
	}
#endif

	/* Shift to get page */
	page = (int) (to >> this->page_shift);
	chipnr = (int) (to >> this->chip_shift);

	/* Mask to get column */
	column = to & (mtd->oobsize - 1);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow write past end of page */
	if ((column + len) > mtd->oobsize) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_oob: Attempt to write past end of page\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_device (this, mtd, FL_WRITING);

	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Reset the chip. Some chips (like the Toshiba TC5832DC found
	   in one of my DiskOnChip 2000 test units) will clear the whole
	   data page too if we don't do this. I have no clue why, but
	   I seem to have 'fixed' it in the doc2000 driver in
	   August 1999.  dwmw2. */
	this->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Check, if it is write protected */
	if (nand_check_wp(mtd))
		goto out;
	
	/* Invalidate the page cache, if we write to the cached page */
	if (page == this->pagebuf)
		this->pagebuf = -1;

	if (NAND_MUST_PAD(this)) {
		/* Write out desired data */
		this->cmdfunc (mtd, NAND_CMD_SEQIN, mtd->oobblock, page & this->pagemask);
		/* prepad 0xff for partial programming */
		this->write_buf(mtd, ffchars, column);
		/* write data */
		this->write_buf(mtd, buf, len);
		/* postpad 0xff for partial programming */
		this->write_buf(mtd, ffchars, mtd->oobsize - (len+column));
	} else {
		/* Write out desired data */
		this->cmdfunc (mtd, NAND_CMD_SEQIN, mtd->oobblock + column, page & this->pagemask);
		/* write data */
		this->write_buf(mtd, buf, len);
	}
	/* Send command to program the OOB data */
	this->cmdfunc (mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = this->waitfunc (mtd, this, FL_WRITING);

	/* See if device thinks it succeeded */
	if (status & NAND_STATUS_FAIL) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_oob: " "Failed write, page 0x%08x\n", page);
		ret = -EIO;
		goto out;
	}
	/* Return happy */
	*retlen = len;

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	nand_increment_rdcnt(this, page);
#endif
	/* Send command to read back the data */
	this->cmdfunc (mtd, NAND_CMD_READOOB, column, page & this->pagemask);

	if (this->verify_buf(mtd, buf, len)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_oob: " "Failed write verify, page 0x%08x\n", page);
		ret = -EIO;
		goto out;
	}
#endif
	ret = 0;
out:
	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(mtd);

	return ret;
}

/**
 * nand_write_oob_hwecc - [MTD Interface] NAND write out-of-band
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @oob_buf:	the data to write
 *
 * NAND write out-of-band
 * W/o assumptions that are valid only for software ECC
 */
static int nand_write_oob_hwecc (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char * oob_buf)
{
	int column, page, status, ret = -EIO, chipnr, eccsteps;
	int fflen, old_fflen, ooblen;
	struct nand_chip *this = mtd->priv;

	DEBUG (MTD_DEBUG_LEVEL3, "%s: to = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) to, (int) len);

	/* Shift to get page */
	page = (int) (to >> this->page_shift);
	chipnr = (int) (to >> this->chip_shift);

	/* Mask to get column */
	column = to & (mtd->oobsize - 1);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow write past end of page */
	if ((column + len) > mtd->oobsize) {
		DEBUG (MTD_DEBUG_LEVEL0, "%s: Attempt to write past end of page\n", __FUNCTION__);
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_device (this, mtd, FL_WRITING);

	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Reset the chip. Some chips (like the Toshiba TC5832DC found
	   in one of my DiskOnChip 2000 test units) will clear the whole
	   data page too if we don't do this. I have no clue why, but
	   I seem to have 'fixed' it in the doc2000 driver in
	   August 1999.  dwmw2. */
	this->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Check, if it is write protected */
	if (nand_check_wp(mtd))
		goto out;
	
	/* Invalidate the page cache, if we write to the cached page */
	if (page == this->pagebuf)
		this->pagebuf = -1;

	/* Write out desired data */
	this->cmdfunc (mtd, NAND_CMD_SEQIN, 0, page & this->pagemask);

	eccsteps = this->eccsteps;
		
	for (fflen = 0, ooblen = 0, old_fflen = 0; eccsteps; eccsteps--) {
		int i, j;
		for (j = 0; this->layout[j].length; j++) {
			switch (this->layout[j].type) {
			case ITEM_TYPE_DATA:
				if (this->options & NAND_COMPLEX_OOB_WRITE) {
					this->enable_hwecc(mtd, NAND_ECC_WRITE);
					this->write_buf(mtd, ffchars, this->layout[j].length);
					fflen += this->layout[j].length;
				} else {
					if (old_fflen < fflen) {
						this->cmdfunc (mtd, NAND_CMD_PAGEPROG, -1, -1);
						status = this->waitfunc (mtd, this, FL_WRITING);
						if (status & NAND_STATUS_FAIL) {
							DEBUG (MTD_DEBUG_LEVEL0, "%s: Failed write, page 0x%08x\n", __FUNCTION__, page);
							ret = -EIO;
							goto out;
						}
					}
					fflen += this->layout[j].length;
					if (this->options & NAND_BUSWIDTH_16 && (fflen + ooblen) & 1)
						this->cmdfunc (mtd, NAND_CMD_SEQIN, fflen + ooblen - 1, page & this->pagemask);
					else
						this->cmdfunc (mtd, NAND_CMD_SEQIN, fflen + ooblen, page & this->pagemask);
					old_fflen = fflen;
				}
				break;

			case ITEM_TYPE_ECC:
			case ITEM_TYPE_OOB:
				if (this->layout[j].type == ITEM_TYPE_ECC)
					this->enable_hwecc(mtd, NAND_ECC_WRITESYN);
				else
					this->enable_hwecc(mtd, NAND_ECC_WRITEOOB);
				i = min_t(int, column, this->layout[j].length);
				if (i) {
					/*
					 * if i is odd, then we're in the
					 * situation when we either stopped at
					 * i-1 or at 1
					 */
					if (this->options & NAND_BUSWIDTH_16 && i & 1)
						i--;
					/*
					 * handle specific case: i was 1
					 * i. e. write (0th, 1th) bytes
					 */
					if (i == 0) {
						this->write_word(mtd, cpu_to_le16((oob_buf[0] << 8) | 0xff));
						i++;
						ooblen++;
					} else
						/* write i-1 (even number) */
						this->write_buf(mtd, ffchars, i);
				}
				column -= i;
				fflen += i;
				/*
				 * do we have anything else to write
				 * for this layout item?
				 */
				i = min_t(int, len + column - ooblen, this->layout[j].length - i);
				if (i) {
					if (column) {
						/*
						 * we're here? this means that
						 * column now equals to 1
						 */
						this->write_word(mtd, cpu_to_le16((oob_buf[0] << 8) | 0xff));
						i--;
						ooblen++;
						column--;
					}
					if (i & 1)
						i--;
					this->write_buf(mtd, &oob_buf[ooblen], i);
				}
				ooblen += i;
				/*
				 * do we have to write the 1-byte tail?
				 */
				if (ooblen == len - 1) {
					this->write_word(mtd, cpu_to_le16(oob_buf[ooblen]) | 0xff00);
					ooblen += 2;
				}
				if (ooblen >= len) {
					if (NAND_MUST_PAD(this))
						this->write_buf(mtd, ffchars, mtd->oobsize + mtd->oobblock - fflen - ooblen);
					goto finish;
				}
				break;
			}
		}
	}
finish:
	/* Send command to program the OOB data */
	this->cmdfunc (mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = this->waitfunc (mtd, this, FL_WRITING);

	/* See if device thinks it succeeded */
	if (status & NAND_STATUS_FAIL) {
		DEBUG (MTD_DEBUG_LEVEL0, "%s: Failed write, page 0x%08x\n", __FUNCTION__, page);
		ret = -EIO;
		goto out;
	}
	/* Return happy */
	*retlen = len;

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
#warning "Verify for OOB data in HW ECC case is NOT YET implemented"
#endif
	ret = 0;
out:
	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(mtd);

	return ret;
}

/**
 * nand_writev - [MTD Interface] compabilty function for nand_writev_ecc
 * @mtd:	MTD device structure
 * @vecs:	the iovectors to write
 * @count:	number of vectors
 * @to:		offset to write to
 * @retlen:	pointer to variable to store the number of written bytes
 *
 * NAND write with kvec. This just calls the ecc function
 */
static int nand_writev (struct mtd_info *mtd, const struct kvec *vecs, unsigned long count, 
		loff_t to, size_t * retlen)
{
	return (nand_writev_ecc (mtd, vecs, count, to, retlen, NULL, NULL));	
}

/**
 * nand_writev_ecc - [MTD Interface] write with iovec with ecc
 * @mtd:	MTD device structure
 * @vecs:	the iovectors to write
 * @count:	number of vectors
 * @to:		offset to write to
 * @retlen:	pointer to variable to store the number of written bytes
 * @eccbuf:	filesystem supplied oob data buffer
 * @oobsel:	oob selection structure
 *
 * NAND write with iovec with ecc
 */
static int nand_writev_ecc (struct mtd_info *mtd, const struct kvec *vecs, unsigned long count, 
		loff_t to, size_t * retlen, u_char *eccbuf, struct nand_oobinfo *oobsel)
{
	int i, page, len, total_len, ret = -EIO, written = 0, chipnr;
	int oob, numpages, autoplace = 0, startpage;
	struct nand_chip *this = mtd->priv;
	int	ppblock = (1 << (this->phys_erase_shift - this->page_shift));
	u_char *oobbuf, *bufstart;
#if defined(CONFIG_MTD_NAND_BBM)
	loff_t  to_rb;
	int block_replace_flag = 0;
#endif

	/* Preset written len for early exit */
	*retlen = 0;

	/* Calculate total length of data */
	total_len = 0;
	for (i = 0; i < count; i++)
		total_len += (int) vecs[i].iov_len;

	DEBUG (MTD_DEBUG_LEVEL3,
	       "nand_writev: to = 0x%08x, len = %i, count = %ld\n", (unsigned int) to, (unsigned int) total_len, count);

	/* Do not allow write past end of page */
	if ((to + total_len) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_writev: Attempted write past end of device\n");
		return -EINVAL;
	}

	/* reject writes, which are not page aligned */	
	if (NOTALIGNED (to) || NOTALIGNED(total_len)) {
		printk (KERN_NOTICE "nand_write_ecc: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_device (this, mtd, FL_WRITING);

	/* Get the current chip-nr */
	chipnr = (int) (to >> this->chip_shift);
	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Check, if it is write protected */
	if (nand_check_wp(mtd))
		goto out;

	/* if oobsel is NULL, use chip defaults */
	if (oobsel == NULL) 
		oobsel = &mtd->oobinfo;		

	/* Autoplace of oob data ? Use the default placement scheme */
	if (oobsel->useecc == MTD_NANDECC_AUTOPLACE) {
		oobsel = this->autooob;
		autoplace = 1;
	}
	if (oobsel->useecc == MTD_NANDECC_AUTOPL_USR)
		autoplace = 1;

	/* Setup start page */
	page = (int) (to >> this->page_shift);

#if defined(CONFIG_MTD_NAND_BBM)
#if defined(CONFIG_MOT_FEAT_NAND_RDDIST)
	if (nand_rddist_check(mtd, to))
		/* Read disturb fix inprogress, don't do block replacement */
		DEBUG (MTD_DEBUG_LEVEL0, "no block replacement\n");
	else
#endif
	/* Check for block replacement if the given page is in bad block */
	if (nand_isbad_bbt (mtd, to, 0)) {
		to_rb = nand_translate_block(mtd, to);
		if (to_rb != to ) {
			DEBUG (MTD_DEBUG_LEVEL0, "nand_writev_ecc: badblock offset:0x%08x, replblock offset:0x%08x\n",
				(unsigned int)to, (unsigned int)to_rb);
			page = (int) (to_rb >> this->page_shift);
			block_replace_flag = 1;
		}
	}
#endif
	
	/* Invalidate the page cache, if we write to the cached page */
	if (page <= this->pagebuf && this->pagebuf < ((to + total_len) >> this->page_shift))  
		this->pagebuf = -1;

	startpage = page & this->pagemask;

	/* Loop until all kvec' data has been written */
	len = 0;
	while (count) {
		/* If the given tuple is >= pagesize then
		 * write it out from the iov
		 */
		if ((vecs->iov_len - len) >= mtd->oobblock) {
			/* Calc number of pages we can write
			 * out of this iov in one go */
			numpages = (vecs->iov_len - len) >> this->page_shift;
			/* Do not cross block boundaries */
			numpages = min (ppblock - (startpage & (ppblock - 1)), numpages);
			oobbuf = nand_prepare_oobbuf (mtd, NULL, oobsel, autoplace, numpages);
			bufstart = (u_char *)vecs->iov_base;
			bufstart += len;
			this->data_poi = bufstart;
			oob = 0;
			for (i = 1; i <= numpages; i++) {
				/* Write one page. If this is the last page to write
				 * then use the real pageprogram command, else select 
				 * cached programming if supported by the chip.
				 */
				ret = nand_write_page (mtd, this, page & this->pagemask, 
					&oobbuf[oob], oobsel, i != numpages);
				if (ret)
					goto out;
				this->data_poi += mtd->oobblock;
				len += mtd->oobblock;
				oob += mtd->oobsize;
				page++;
			}
			/* Check, if we have to switch to the next tuple */
			if (len >= (int) vecs->iov_len) {
				vecs++;
				len = 0;
				count--;
			}
		} else {
			/* We must use the internal buffer, read data out of each 
			 * tuple until we have a full page to write
			 */
			int cnt = 0;
			while (cnt < mtd->oobblock) {
				if (vecs->iov_base != NULL && vecs->iov_len) 
					this->data_buf[cnt++] = ((u_char *) vecs->iov_base)[len++];
				/* Check, if we have to switch to the next tuple */
				if (len >= (int) vecs->iov_len) {
					vecs++;
					len = 0;
					count--;
				}
			}
			this->pagebuf = page;	
			this->data_poi = this->data_buf;	
			bufstart = this->data_poi;
			numpages = 1;		
			oobbuf = nand_prepare_oobbuf (mtd, NULL, oobsel, autoplace, numpages);
			ret = nand_write_page (mtd, this, page & this->pagemask,
				oobbuf, oobsel, 0);
			if (ret)
				goto out;
			page++;
		}

		this->data_poi = bufstart;
		ret = nand_verify_pages (mtd, this, startpage, numpages, oobbuf, oobsel, chipnr, 0);
		if (ret)
			goto out;
			
		written += mtd->oobblock * numpages;

#if defined(CONFIG_MTD_NAND_BBM)
		/* Check, if it is in a replacement block */
		if (block_replace_flag) {
			/* Set page back to original block */
			page = (int) (to >> this->page_shift) + numpages;
			block_replace_flag = 0;
		}
		/* Check, if the given start page is in a bad block */
		if (nand_isbad_bbt (mtd, (page << this->page_shift), 0)) {
			to_rb = nand_translate_block(mtd, (page << this->page_shift));
			if (to_rb != (page << this->page_shift)) {
				block_replace_flag = 1;
				page = (int) (to_rb >> this->page_shift);
			}
		}
#endif
	
		/* All done ? */
		if (!count)
			break;

		startpage = page & this->pagemask;
		/* Check, if we cross a chip boundary */
		if (!startpage) {
			chipnr++;
			this->select_chip(mtd, -1);
			this->select_chip(mtd, chipnr);
		}
	}
	ret = 0;
out:
	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(mtd);

	*retlen = written;
	return ret;
}

/**
 * single_erease_cmd - [GENERIC] NAND standard block erase command function
 * @mtd:	MTD device structure
 * @page:	the page address of the block which will be erased
 *
 * Standard erase command for NAND chips
 */
static void single_erase_cmd (struct mtd_info *mtd, int page)
{
	struct nand_chip *this = mtd->priv;
	/* Send commands to erase a block */
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page);
	this->cmdfunc (mtd, NAND_CMD_ERASE2, -1, -1);
}

/**
 * multi_erease_cmd - [GENERIC] AND specific block erase command function
 * @mtd:	MTD device structure
 * @page:	the page address of the block which will be erased
 *
 * AND multi block erase command function
 * Erase 4 consecutive blocks
 */
static void multi_erase_cmd (struct mtd_info *mtd, int page)
{
	struct nand_chip *this = mtd->priv;
	/* Send commands to erase a block */
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page++);
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page++);
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page++);
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page);
	this->cmdfunc (mtd, NAND_CMD_ERASE2, -1, -1);
}

/**
 * nand_erase - [MTD Interface] erase block(s)
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 *
 * Erase one ore more blocks
 */
static int nand_erase (struct mtd_info *mtd, struct erase_info *instr)
{
	return nand_erase_nand (mtd, instr, 0);
}
 
#define BBT_PAGE_MASK	0xffffff3f
/**
 * nand_erase_intern - [NAND Interface] erase block(s)
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 * @allowbbt:	allow erasing the bbt area
 *
 * Erase one ore more blocks
 */
int nand_erase_nand (struct mtd_info *mtd, struct erase_info *instr, int allowbbt)
{
	int page, len, status, pages_per_block, ret, chipnr;
	struct nand_chip *this = mtd->priv;
	int rewrite_bbt[NAND_MAX_CHIPS]={0};	/* flags to indicate the page, if bbt needs to be rewritten. */
	unsigned int bbt_masked_page;		/* bbt mask to compare to page being erased. */
						/* It is used to see if the current page is in the same */
						/*   256 block group and the same bank as the bbt. */
#ifdef CONFIG_MOT_WFN455
	int retry_cnt = 0;
	cycles_t start_erase = 0, end_erase = 0;
#endif
#if defined(CONFIG_MTD_NAND_BBM)
	loff_t  orig_addr = instr->addr;
	int block_replace_flag = 0;
#endif

	DEBUG (MTD_DEBUG_LEVEL3,
	       "nand_erase: start = 0x%08x, len = %i\n", (unsigned int) instr->addr, (unsigned int) instr->len);

	/* Start address must align on block boundary */
	if (instr->addr & ((1 << this->phys_erase_shift) - 1)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Unaligned address\n");
		return -EINVAL;
	}

	/* Length must align on block boundary */
	if (instr->len & ((1 << this->phys_erase_shift) - 1)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Length not block aligned\n");
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if ((instr->len + instr->addr) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Erase past end of device\n");
		return -EINVAL;
	}

	instr->fail_addr = 0xffffffff;

#ifdef CONFIG_MOT_FEAT_KPANIC
	if (kpanic_in_progress) {
		status = knand_get_device (this, mtd, FL_ERASING);
		if (status)
			return -EBUSY;
	}
	else
#endif /* CONFIG_MOT_FEAT_KPANIC */
	/* Grab the lock and see if the device is available */
	nand_get_device (this, mtd, FL_ERASING);

	/* Shift to get first page */
	page = (int) (instr->addr >> this->page_shift);
	chipnr = (int) (instr->addr >> this->chip_shift);

	/* Calculate pages in each block */
	pages_per_block = 1 << (this->phys_erase_shift - this->page_shift);

	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Check the WP bit */
	/* Check, if it is write protected */
	if (nand_check_wp(mtd)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Device is write protected!!!\n");
		instr->state = MTD_ERASE_FAILED;
		goto erase_exit;
	}

	/* if BBT requires refresh, set the BBT page mask to see if the BBT should be rewritten */
	if (this->options & BBT_AUTO_REFRESH) {
		bbt_masked_page = this->bbt_td->pages[chipnr] & BBT_PAGE_MASK;
	} else {
		bbt_masked_page = 0xffffffff;	/* should not match anything */
	}

	/* Loop through the pages */
	len = instr->len;

	instr->state = MTD_ERASING;

	while (len) {
#if defined(CONFIG_MTD_NAND_BBM)
		/* check, if it is in a replacement block */
		if (block_replace_flag) {
			/* set page back to original block */
			page = (int) (orig_addr >> this->page_shift);
			block_replace_flag = 0;
		}
		/* check, if the given start page is in a bad block */
		if (nand_isbad_bbt (mtd, (page << this->page_shift), allowbbt)) {
			instr->addr = nand_translate_block(mtd, (page << this->page_shift));
			/* Check if we have a bad block with no replacement, we do not erase bad blocks ! */
			if (instr->addr == orig_addr) {
				printk (KERN_WARNING "nand_erase: attempt to erase a bad block at page 0x%08x\n", page);
				instr->state = MTD_ERASE_FAILED;
				goto erase_exit;
			}
			block_replace_flag = 1;
			DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: badblck at 0x%08x replced by 0x%08x\n",
				(unsigned int) (page << this->page_shift), (unsigned int) instr->addr);
			/* updating page offset with the replacement page offset */
			page = (int) (instr->addr >> this->page_shift);
		}
		orig_addr += (1 << this->phys_erase_shift);
#else
		/* Check if we have a bad block, we do not erase bad blocks ! */
		if (nand_block_checkbad(mtd, ((loff_t) page) << this->page_shift, 0, allowbbt)) {
			printk (KERN_WARNING "nand_erase: attempt to erase a bad block at page 0x%08x\n", page);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}
#endif	

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
		nand_increment_erasecnt (this, page);
#endif

		/* Invalidate the page cache, if we erase the block which contains 
		   the current cached page */
		if (page <= this->pagebuf && this->pagebuf < (page + pages_per_block))
			this->pagebuf = -1;
#ifdef CONFIG_MOT_WFN455
	retry:
		start_erase = get_cycles();
		this->erase_cmd (mtd, page & this->pagemask);
		end_erase = get_cycles();
#else
		this->erase_cmd (mtd, page & this->pagemask);
#endif
		status = this->waitfunc (mtd, this, FL_ERASING);

		/* See if operation failed and additional status checks are available */
		if ((status & NAND_STATUS_FAIL) && (this->errstat)) {
			status = this->errstat(mtd, this, FL_ERASING, status, page);
		}

		/* See if block erase succeeded */
		if (status & NAND_STATUS_FAIL) {
#ifdef CONFIG_MOT_FEAT_MTD_AUTO_BBM
			/* get replacement block from reserve pool */
			if (!allowbbt && (nand_block_replace(mtd, page << this->page_shift, 1) == 0)) 
				goto auto_bbm_exit;
#endif
			DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: " "Failed erase, page 0x%08x\n", page);
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr = (page << this->page_shift);
			goto erase_exit;
		}

#ifdef CONFIG_MOT_WFN455
		if ((end_erase-start_erase)/(CLOCK_TICK_RATE/1000) < 1)  /* block erase time less than 1 millisec */
		{
			if (retry_cnt++ < 10)
			{
				printk(KERN_NOTICE "HW bug: %d retry to erase on page 0x%08x\n",retry_cnt ,page);
				goto retry;
			}
			else
			{
				printk(KERN_ERR "block erase failed with 10 retries on page 0x%08x\n", page);
				instr->state = MTD_ERASE_FAILED;
				instr->fail_addr = (page << this->page_shift);
				goto erase_exit;
			}
		}
#endif
#ifdef CONFIG_MOT_FEAT_MTD_AUTO_BBM
	auto_bbm_exit:
#endif
		/* if BBT requires refresh, set the BBT rewrite flag to the page being erased */
		if (this->options & BBT_AUTO_REFRESH) {
			if (((page & BBT_PAGE_MASK) == bbt_masked_page) && 
			     (page != this->bbt_td->pages[chipnr])) {
				rewrite_bbt[chipnr] = (page << this->page_shift);
			}
		}
		
		/* Increment page address and decrement length */
		len -= (1 << this->phys_erase_shift);
		page += pages_per_block;
		
		/* Check, if we cross a chip boundary */
		if (len && !(page & this->pagemask)) {
			chipnr++;
			this->select_chip(mtd, -1);
			this->select_chip(mtd, chipnr);

			/* if BBT requires refresh and BBT-PERCHIP, 
			 *   set the BBT page mask to see if this BBT should be rewritten */
			if ((this->options & BBT_AUTO_REFRESH) && (this->bbt_td->options & NAND_BBT_PERCHIP)) {
				bbt_masked_page = this->bbt_td->pages[chipnr] & BBT_PAGE_MASK;
			}
		}
	}
	instr->state = MTD_ERASE_DONE;

erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;
	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/* Deselect and wake up anyone waiting on the device */
#ifdef CONFIG_MTD_FEAT_MTD_AUTO_BBM
	if (this->state != FL_READY)
		nand_release_device(mtd);
#else
	nand_release_device(mtd);
#endif
	/* if BBT requires refresh and erase was successful, rewrite any selected bad block tables */
	if ((this->options & BBT_AUTO_REFRESH) && (!ret)) {
		for (chipnr = 0; chipnr < this->numchips; chipnr++) {
			if (rewrite_bbt[chipnr]) {
				/* update the BBT for chip */
				DEBUG (MTD_DEBUG_LEVEL0, "nand_erase_nand: nand_update_bbt (%d:0x%0x 0x%0x)\n", 
					chipnr, rewrite_bbt[chipnr], this->bbt_td->pages[chipnr]);
				nand_update_bbt (mtd, rewrite_bbt[chipnr]);
			}
		}
	}

	/* Return more or less happy */
	return ret;
}

/**
 * nand_sync - [MTD Interface] sync
 * @mtd:	MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
static void nand_sync (struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

	DEBUG (MTD_DEBUG_LEVEL3, "nand_sync: called\n");

	/* Grab the lock and see if the device is available */
	nand_get_device (this, mtd, FL_SYNCING);
	/* Release it and go back */
	nand_release_device (mtd);
}


#ifdef CONFIG_MOT_FEAT_MTD_AUTO_BBM
/**
 * nand_block_replace - [MTD Interface] compability function for nand_write_ecc
 * @mtd:	MTD device structure
 * @ofs:	original block offset
 * @lock:	device lock flag
 *
 * return	0: success on block replacement; -1: failure
*/
static int nand_block_replace (struct mtd_info *mtd, loff_t ofs, int lock)
{
	struct nand_chip *this = mtd->priv;
	int status = 0, ret = -1, rsvdblock_idx;
	loff_t r_ofs;

retry:
	rsvdblock_idx = get_bbm_index(mtd, this->bbm);
	if (rsvdblock_idx == -1) {
		printk (KERN_ERR "nand_block_replacement: out of reserved block\n");
		return -1;
	}

	/* make sure that the current block was not marked as bad */
	ofs = nand_translate_block(mtd, ofs);

	/* do erase on the replacement block */
	r_ofs = (loff_t)rsvdblock_offset + (((loff_t)rsvdblock_idx)<<this->phys_erase_shift);
	if (!lock) 
		/* get the device if we don't have it */
		nand_get_device (this, mtd, FL_ERASING);
	this->erase_cmd(mtd, r_ofs >> this->page_shift);
	status = this->waitfunc (mtd, this, FL_ERASING);

	/* frist, update memory version of BBM map: 
	 * remapping with replacement block from reserved pool */
	this->bbm[ofs >> this->phys_erase_shift] = rsvdblock_idx;

	/* then, release the lock for updating the flash version of BBM, BBT .... */
	nand_release_device(mtd);
	lock = 0;

	/* update BBM blocks: both primary and mirror bbm blocks */
	ret = update_bbm(mtd, (uint8_t*) this->bbm, ofs>>this->chip_shift);
	if (ret) {
		printk (KERN_ERR "nand_block_replacement: update_bbm on block offset 0x%08x failed.\n",
			(unsigned int)ofs);
		/* reset the memory version of BBM map, remove the remapping */
		this->bbm[ofs >> this->phys_erase_shift] = 0xffff;
		return -1;
	}
		
	/* update BBT table: mark the current block as bad block */
	ret = this->block_markbad(mtd, ofs);
	if (ret) {
		printk (KERN_ERR "nand_block_replacement: block_markbad on block offset 0x%08x failed.\n",
			(unsigned int)ofs);
		/* reset the memory version of BBM map, remove the remapping */
		this->bbm[ofs >> this->phys_erase_shift] = 0xffff;
		/* revert the BBM updates */
		update_bbm(mtd, (uint8_t*)this->bbm, ofs>>this->chip_shift);
		return -1;
	}

	/* check the if erase failed on the replacement block, go back try again */
	if (status & NAND_STATUS_FAIL)
		goto retry;

	DEBUG (MTD_DEBUG_LEVEL0, "nand_block_replacement: block %d replaced with reserve block %d\n",
		(int)(ofs>>this->phys_erase_shift), this->bbm[ofs>>this->phys_erase_shift]);

	return ret;
}
#endif

/**
 * nand_block_isbad - [MTD Interface] Check whether the block at the given offset is bad
 * @mtd:	MTD device structure
 * @ofs:	offset relative to mtd start
 */
static int nand_block_isbad (struct mtd_info *mtd, loff_t ofs)
{
	/* Check for invalid offset */
	if (ofs > mtd->size) 
		return -EINVAL;
	
	return nand_block_checkbad (mtd, ofs, 1, 0);
}

/**
 * nand_block_markbad - [MTD Interface] Mark the block at the given offset as bad
 * @mtd:	MTD device structure
 * @ofs:	offset relative to mtd start
 */
static int nand_block_markbad (struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = mtd->priv;
	int ret;

        if ((ret = nand_block_isbad(mtd, ofs))) {
        	/* If it was bad already, return success and do nothing. */
		if (ret > 0)
			return 0;
        	return ret;
        }

	return this->block_markbad(mtd, ofs);
}

/**
 * nand_suspend - [MTD Interface] Suspend the NAND flash
 * @mtd:	MTD device structure
 */
static int nand_suspend(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	struct timeval now;
	static time_t pre_tv_sec = 0;
	
	do_gettimeofday(&now);
	if ((now.tv_sec-pre_tv_sec >= 3600*24)||(pre_tv_sec == 0)) 
	{
		int i;
		for (i = 0; i < g_read_dist_info.numblocks; i++)
		/* if we do need update the nand blkcnt for the past 24 hours */
		if (g_read_dist_info.blk_tbl[RDCNT_TBL][i]||g_read_dist_info.blk_tbl[ERASECNT_TBL][i])
		{
			g_read_dist_info.threshold_flag = 1;
			wake_up_interruptible(&g_blkrd_waitq);
			DEBUG(MTD_DEBUG_LEVEL0, "woke up blkrd_waitq and updated nand blkcnt to flash.\n");
			break;
		}
		pre_tv_sec = now.tv_sec;
	}
#endif
	return nand_get_device (this, mtd, FL_PM_SUSPENDED);
}

/**
 * nand_resume - [MTD Interface] Resume the NAND flash
 * @mtd:	MTD device structure
 */
static void nand_resume(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

	if (this->state == FL_PM_SUSPENDED)
		nand_release_device(mtd);
	else
		printk(KERN_ERR "resume() called for the chip which is not "
				"in suspended state\n");
}

/**
 * nand_scan - [NAND Interface] Scan for the NAND device
 * @mtd:	MTD device structure
 * @maxchips:	Number of chips to scan for
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values. Buffers are allocated if
 * they are not provided by the board driver
 *
 */
int nand_scan (struct mtd_info *mtd, int maxchips)
{
	int i, nand_maf_id, nand_dev_id, busw, maf_id;
	struct nand_chip *this = mtd->priv;
	
	/* Get buswidth to select the correct functions*/
	busw = this->options & NAND_BUSWIDTH_16;

	/* check for proper chip_delay setup, set 20us if not */
	if (!this->chip_delay)
		this->chip_delay = 20;

	/* check, if a user supplied command function given */
	if (this->cmdfunc == NULL)
		this->cmdfunc = nand_command;

	/* check, if a user supplied wait function given */
	if (this->waitfunc == NULL)
		this->waitfunc = nand_wait;

	if (!this->select_chip)
		this->select_chip = nand_select_chip;
	if (!this->write_byte)
		this->write_byte = busw ? nand_write_byte16 : nand_write_byte;
	if (!this->read_byte)
		this->read_byte = busw ? nand_read_byte16 : nand_read_byte;
	if (!this->write_word)
		this->write_word = nand_write_word;
	if (!this->read_word)
		this->read_word = nand_read_word;
	if (!this->block_bad)
		this->block_bad = nand_block_bad;
	if (!this->block_markbad)
		this->block_markbad = nand_default_block_markbad;
	if (!this->write_buf)
		this->write_buf = busw ? nand_write_buf16 : nand_write_buf;
	if (!this->read_buf)
		this->read_buf = busw ? nand_read_buf16 : nand_read_buf;
	if (!this->verify_buf)
		this->verify_buf = busw ? nand_verify_buf16 : nand_verify_buf;
	if (!this->scan_bbt)
		this->scan_bbt = nand_default_bbt;

	/* 'ff' the ffchars */
	memset(ffchars, 0xff, FFCHARS_SIZE);

	/* Select the device */
	this->select_chip(mtd, 0);

	/* Send the command for reading device ID */
	this->cmdfunc (mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	nand_maf_id = this->read_byte(mtd);
	nand_dev_id = this->read_byte(mtd);

	/* Print and store flash device information */
	for (i = 0; nand_flash_ids[i].name != NULL; i++) {
				
		if (nand_dev_id != nand_flash_ids[i].id) 
			continue;

		if (!mtd->name) mtd->name = nand_flash_ids[i].name;
		this->chipsize = nand_flash_ids[i].chipsize << 20;
		
		/* New devices have all the information in additional id bytes */
		if (!nand_flash_ids[i].pagesize) {
			int extid;
			/* The 3rd id byte contains non relevant data ATM */
			extid = this->read_byte(mtd);
			/* The 4th id byte is the important one */
			extid = this->read_byte(mtd);
			/* Calc pagesize */
			mtd->oobblock = 1024 << (extid & 0x3);
			extid >>= 2;
			/* Calc oobsize */
			mtd->oobsize = (8 << (extid & 0x03)) * (mtd->oobblock / 512);
			extid >>= 2;
			/* Calc blocksize. Blocksize is multiples of 64KiB */
			mtd->erasesize = (64 * 1024)  << (extid & 0x03);
			extid >>= 2;
			/* Get buswidth information */
			busw = (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;
		} else {
			/* Old devices have this data hardcoded in the
			 * device id table */
			mtd->erasesize = nand_flash_ids[i].erasesize;
			mtd->oobblock = nand_flash_ids[i].pagesize;
			mtd->oobsize = mtd->oobblock / 32;
			busw = nand_flash_ids[i].options & NAND_BUSWIDTH_16;
		}

		/* Try to identify manufacturer */
		for (maf_id = 0; nand_manuf_ids[maf_id].id != 0x0; maf_id++) {
			if (nand_manuf_ids[maf_id].id == nand_maf_id)
				break;
		}

		/* Check, if buswidth is correct. Hardware drivers should set
		 * this correct ! */
		if (busw != (this->options & NAND_BUSWIDTH_16)) {
			printk (KERN_INFO "NAND device: Manufacturer ID:"
				" 0x%02x, Chip ID: 0x%02x (%s %s)\n", nand_maf_id, nand_dev_id, 
				nand_manuf_ids[maf_id].name , mtd->name);
			printk (KERN_WARNING 
				"NAND bus width %d instead %d bit\n", 
					(this->options & NAND_BUSWIDTH_16) ? 16 : 8,
					busw ? 16 : 8);
			this->select_chip(mtd, -1);
			return 1;	
		}
		
		/* Calculate the address shift from the page size */	
		this->page_shift = ffs(mtd->oobblock) - 1;
		this->bbt_erase_shift = this->phys_erase_shift = ffs(mtd->erasesize) - 1;
		this->chip_shift = ffs(this->chipsize) - 1;

		/* Set the bad block position */
		this->badblockpos = mtd->oobblock > 512 ? 
			NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;

		/* Get chip options, preserve non chip based options */
		this->options &= ~NAND_CHIPOPTIONS_MSK;
		this->options |= nand_flash_ids[i].options & NAND_CHIPOPTIONS_MSK;
		/* Set this as a default. Board drivers can override it, if neccecary */
		this->options |= NAND_NO_AUTOINCR;
		/* Check if this is a not a samsung device. Do not clear the options
		 * for chips which are not having an extended id.
		 */	
		if (nand_maf_id != NAND_MFR_SAMSUNG && !nand_flash_ids[i].pagesize)
			this->options &= ~NAND_SAMSUNG_LP_OPTIONS;
		
		/* Check for AND chips with 4 page planes */
		if (this->options & NAND_4PAGE_ARRAY)
			this->erase_cmd = multi_erase_cmd;
		else
			this->erase_cmd = single_erase_cmd;

		/* Do not replace user supplied command function ! */
		if (mtd->oobblock > 512 && this->cmdfunc == nand_command)
			this->cmdfunc = nand_command_lp;
				
		printk (KERN_INFO "NAND device: Manufacturer ID:"
			" 0x%02x, Chip ID: 0x%02x (%s %s)\n", nand_maf_id, nand_dev_id, 
			nand_manuf_ids[maf_id].name , nand_flash_ids[i].name);
		break;
	}

	if (!nand_flash_ids[i].name) {
		printk (KERN_WARNING "No NAND device found!!!\n");
		this->select_chip(mtd, -1);
		return 1;
	}

	for (i=1; i < maxchips; i++) {
		this->select_chip(mtd, i);

		/* Send the command for reading device ID */
		this->cmdfunc (mtd, NAND_CMD_READID, 0x00, -1);

		/* Read manufacturer and device IDs */
		if (nand_maf_id != this->read_byte(mtd) ||
		    nand_dev_id != this->read_byte(mtd))
			break;
	}
	if (i > 1)
		printk(KERN_INFO "%d NAND chips detected\n", i);
	
	/* Allocate buffers, if neccecary */
	if (!this->oob_buf) {
		size_t len;
		len = mtd->oobsize << (this->phys_erase_shift - this->page_shift);
		this->oob_buf = kmalloc (len, GFP_KERNEL);
		if (!this->oob_buf) {
			printk (KERN_ERR "nand_scan(): Cannot allocate oob_buf\n");
			return -ENOMEM;
		}
		this->options |= NAND_OOBBUF_ALLOC;
	}
	
	if (!this->data_buf) {
		size_t len;
		len = mtd->oobblock + mtd->oobsize;
		this->data_buf = kmalloc (len, GFP_KERNEL);
		if (!this->data_buf) {
			if (this->options & NAND_OOBBUF_ALLOC)
				kfree (this->oob_buf);
			printk (KERN_ERR "nand_scan(): Cannot allocate data_buf\n");
			return -ENOMEM;
		}
		this->options |= NAND_DATABUF_ALLOC;
	}

	/* Store the number of chips and calc total size for mtd */
	this->numchips = i;
	mtd->size = i * this->chipsize;
	/* Convert chipsize to number of pages per chip -1. */
	this->pagemask = (this->chipsize >> this->page_shift) - 1;
	/* Preset the internal oob buffer */
	memset(this->oob_buf, 0xff, mtd->oobsize << (this->phys_erase_shift - this->page_shift));

	/* If no default placement scheme is given, select an
	 * appropriate one */
	if (!this->autooob) {
		/* Select the appropriate default oob placement scheme for
		 * placement agnostic filesystems */
		switch (mtd->oobsize) { 
		case 8:
			this->autooob = &nand_oob_8;
			break;
		case 16:
			this->autooob = &nand_oob_16;
			break;
		case 64:
			this->autooob = &nand_oob_64;
			break;
		default:
			printk (KERN_WARNING "No oob scheme defined for oobsize %d\n",
				mtd->oobsize);
			BUG();
		}
	}
	
	/* The number of bytes available for the filesystem to place fs dependend
	 * oob data */
	mtd->oobavail = 0;
	for (i = 0; this->autooob->oobfree[i][1]; i++)
		mtd->oobavail += this->autooob->oobfree[i][1];

	/* 
	 * check ECC mode, default to software
	 * if 3byte/512byte hardware ECC is selected and we have 256 byte pagesize
	 * fallback to software ECC 
	*/
	this->eccsize = 256;	/* set default eccsize */	
	this->eccbytes = 3;

	switch (this->eccmode) {
	case NAND_ECC_HW12_2048:
		if (mtd->oobblock < 2048) {
			printk(KERN_WARNING "2048 byte HW ECC not possible on %d byte page size, fallback to SW ECC\n",
			       mtd->oobblock);
			this->eccmode = NAND_ECC_SOFT;
			this->calculate_ecc = nand_calculate_ecc;
			this->correct_data = nand_correct_data;
		} else
			this->eccsize = 2048;
		break;

	case NAND_ECC_HW3_512: 
	case NAND_ECC_HW6_512: 
	case NAND_ECC_HW8_512: 
		if (mtd->oobblock == 256) {
			printk (KERN_WARNING "512 byte HW ECC not possible on 256 Byte pagesize, fallback to SW ECC \n");
			this->eccmode = NAND_ECC_SOFT;
			this->calculate_ecc = nand_calculate_ecc;
			this->correct_data = nand_correct_data;
		} else 
			this->eccsize = 512; /* set eccsize to 512 */
		break;
			
	case NAND_ECC_HW3_256:
		break;
		
	case NAND_ECC_NONE: 
		printk (KERN_WARNING "NAND_ECC_NONE selected by board driver. This is not recommended !!\n");
		this->eccmode = NAND_ECC_NONE;
		break;

	case NAND_ECC_SOFT:	
		this->calculate_ecc = nand_calculate_ecc;
		this->correct_data = nand_correct_data;
		break;

	default:
		printk (KERN_WARNING "Invalid NAND_ECC_MODE %d\n", this->eccmode);
		BUG();	
	}	

	/* Check hardware ecc function availability and adjust number of ecc bytes per 
	 * calculation step
	*/
	switch (this->eccmode) {
	case NAND_ECC_HW12_2048:
		this->eccbytes += 4;
	case NAND_ECC_HW8_512: 
		this->eccbytes += 2;
	case NAND_ECC_HW6_512: 
		this->eccbytes += 3;
	case NAND_ECC_HW3_512: 
	case NAND_ECC_HW3_256:
		if (this->calculate_ecc && this->correct_data && this->enable_hwecc)
			break;
		printk (KERN_WARNING "No ECC functions supplied, Hardware ECC not possible\n");
		BUG();	
	}
		
	mtd->eccsize = this->eccsize;
	
	/* Set the number of read / write steps for one page to ensure ECC generation */
	switch (this->eccmode) {
	case NAND_ECC_HW12_2048:
		this->eccsteps = mtd->oobblock / 2048;
		break;
	case NAND_ECC_HW3_512:
	case NAND_ECC_HW6_512:
	case NAND_ECC_HW8_512:
		this->eccsteps = mtd->oobblock / 512;
		break;
	case NAND_ECC_HW3_256:
	case NAND_ECC_SOFT:	
		this->eccsteps = mtd->oobblock / 256;
		break;
		
	case NAND_ECC_NONE: 
		this->eccsteps = 1;
		break;
	}
	
	mtd->eccsize = this->eccsize;

	/* Initialize state, waitqueue and spinlock */
	this->state = FL_READY;
	init_waitqueue_head (&this->wq);
	spin_lock_init (&this->chip_lock);

	/* De-select the device */
	this->select_chip(mtd, -1);

	/* Invalidate the pagebuffer reference */
	this->pagebuf = -1;

	/* Fill in remaining MTD driver data */
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH | MTD_ECC;
	mtd->ecctype = MTD_ECC_SW;
	mtd->erase = nand_erase;
	mtd->point = NULL;
	mtd->unpoint = NULL;
	mtd->read = nand_read;
	mtd->write = nand_write;
	mtd->read_ecc = nand_read_ecc;
	mtd->write_ecc = nand_write_ecc;

	if ((this->eccmode != NAND_ECC_NONE && this->eccmode != NAND_ECC_SOFT)
	    && this->layout) {
		mtd->read_oob = nand_read_oob_hwecc;
		mtd->write_oob = nand_write_oob_hwecc;
	} else {
		mtd->read_oob = nand_read_oob;
		mtd->write_oob = nand_write_oob;
	}

	mtd->readv = NULL;
	mtd->writev = nand_writev;
	mtd->writev_ecc = nand_writev_ecc;
#ifdef CONFIG_MOT_FEAT_MTD_AUTO_BBM
	mtd->block_replace = nand_block_replace;
#endif
	mtd->sync = nand_sync;
	mtd->lock = NULL;
	mtd->unlock = NULL;
	mtd->suspend = nand_suspend;
	mtd->resume = nand_resume;
	mtd->block_isbad = nand_block_isbad;
	mtd->block_markbad = nand_block_markbad;

	/* and make the autooob the default one */
	memcpy(&mtd->oobinfo, this->autooob, sizeof(mtd->oobinfo));

	mtd->owner = THIS_MODULE;

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	/* add one more api for mtd level */
	mtd->read_distfix = nand_read_distfix;

	/* allocate memory for g_read_dist_info.blk_tbl - (1 byte per block) */
	g_read_dist_info.numblocks = mtd->size >> this->phys_erase_shift;
	if (!(g_read_dist_info.blk_tbl[RDCNT_TBL] = kmalloc(g_read_dist_info.numblocks * sizeof(block_cnt_type), GFP_KERNEL))) {
		printk (KERN_ERR "g_read_dist_info.blk_tbl[RDCNT_TBL] - Out of memory (size %d)\n", g_read_dist_info.numblocks * sizeof(block_cnt_type));
		return -ENOMEM;
	}
	if (!(g_read_dist_info.blk_tbl[ERASECNT_TBL] = kmalloc(g_read_dist_info.numblocks * sizeof(block_cnt_type), GFP_KERNEL))) {
                printk (KERN_ERR "g_read_dist_info.blk_tbl[ERASECNT_TBL] - Out of memory (size %d)\n", g_read_dist_info.numblocks * sizeof(block_cnt_type));
                return -ENOMEM;
        }
	memset(g_read_dist_info.blk_tbl[RDCNT_TBL], 0x0, g_read_dist_info.numblocks * sizeof(block_cnt_type));
	memset(g_read_dist_info.blk_tbl[ERASECNT_TBL], 0x0, g_read_dist_info.numblocks * sizeof(block_cnt_type));
#ifdef CONFIG_MOT_FEAT_NAND_BLKCNT_TEST
	/* allocate memory for g_write_test_info.blk_tbl - (1 byte per block) */
	if (!(g_write_test_info.blk_tbl = kmalloc(g_read_dist_info.numblocks*sizeof(block_cnt_type), GFP_KERNEL))) {
		printk (KERN_ERR "g_write_test_info.blk_tbl - Out of memory (size %d)\n", g_read_dist_info.numblocks);
		return -ENOMEM;
	}
	memset(g_write_test_info.blk_tbl, 0x0, g_read_dist_info.numblocks*sizeof(block_cnt_type));
	/* allocate memory for g_erase_test_info.blk_tbl - (1 byte per block) */
	if (!(g_erase_test_info.blk_tbl = kmalloc(g_read_dist_info.numblocks*sizeof(block_cnt_type), GFP_KERNEL))) {
		printk (KERN_ERR "g_erase_test_info.blk_tbl - Out of memory (size %d)\n", g_read_dist_info.numblocks);
		return -ENOMEM;
	}
	memset(g_erase_test_info.blk_tbl, 0x0, g_read_dist_info.numblocks*sizeof(block_cnt_type));
#endif /*CONFIG_MOT_FEAT_NAND_BLKCNT_TEST*/
#endif /*CONFIG_MOT_FEAT_NAND_RDDIST*/
	
	/* Check, if we should skip the bad block table scan */
	if (this->options & NAND_SKIP_BBTSCAN)
		return 0;

	/* Build bad block table */
	return this->scan_bbt (mtd);
}

/**
 * nand_release - [NAND Interface] Free resources held by the NAND device 
 * @mtd:	MTD device structure
*/
void nand_release (struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

#ifdef CONFIG_MTD_PARTITIONS
	/* Deregister partitions */
	del_mtd_partitions (mtd);
#endif
	/* Deregister the device */
	del_mtd_device (mtd);

	/* Free bad block table memory, if allocated */
	if (this->bbt)
		kfree (this->bbt);
#ifdef CONFIG_MTD_NAND_BBM
	/* Free bad block map memory, if allocated */
	if (this->bbm)
		kfree (this->bbm);
#endif
	/* Buffer allocated by nand_scan ? */
	if (this->options & NAND_OOBBUF_ALLOC)
		kfree (this->oob_buf);
	/* Buffer allocated by nand_scan ? */
	if (this->options & NAND_DATABUF_ALLOC)
		kfree (this->data_buf);
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	/* Buffer table allocated by RDDIST feature ? */
	if (g_read_dist_info.blk_tbl[RDCNT_TBL])
		kfree (g_read_dist_info.blk_tbl[RDCNT_TBL]);
	if (g_read_dist_info.blk_tbl[ERASECNT_TBL])
                kfree (g_read_dist_info.blk_tbl[ERASECNT_TBL]);

#ifdef CONFIG_MOT_FEAT_NAND_BLKCNT_TEST
	/* Buffer tbales allocated by NAND test ? */
	if (g_write_test_info.blk_tbl)
		kfree (g_write_test_info.blk_tbl);
	if (g_erase_test_info.blk_tbl)
		kfree (g_erase_test_info.blk_tbl);
#endif /*CONFIG_MOT_FEAT_NAND_BLKCNT_TEST*/
#endif /*CONFIG_MOT_FEAT_NAND_RDDIST*/
}

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
static void nand_increment_rdcnt (struct nand_chip *this, int page)
{
	int block = 0;

	/* Calculate block based on page */
	block = (page<<this->page_shift)>>this->phys_erase_shift;

        /* keep tracking the g_read_dist_info.blk_tbl */
        g_read_dist_info.blk_tbl[RDCNT_TBL][block]++; 

        if ((g_read_dist_info.threshold)&&
           ((g_read_dist_info.blk_tbl[RDCNT_TBL][block])>=g_read_dist_info.threshold)) {
                g_read_dist_info.threshold_flag = 1;
                wake_up_interruptible(&g_blkrd_waitq);
                DEBUG(MTD_DEBUG_LEVEL0, "block %d exceed read access threshold %d.\n",
                        block, g_read_dist_info.threshold);
        }
}

static void nand_increment_erasecnt (struct nand_chip *this, int page)
{
	int block = 0;

	/* Calculate block based on page */
        block = (page<<this->page_shift)>>this->phys_erase_shift;

	/* reset the g_read_dist_info.blk_tbl's RDCNT_TBL */
	g_read_dist_info.blk_tbl[RDCNT_TBL][block]=0;
	/* keep tracking the g_read_dist_info.blk_tbl's ERASECNT_TBL */
        g_read_dist_info.blk_tbl[ERASECNT_TBL][block]++;

#ifdef CONFIG_MOT_FEAT_NAND_BLKCNT_TEST
        g_erase_test_info.blk_tbl[(page<<this->page_shift)>>this->phys_erase_shift]++;

        /* keep tracking the g_erase_test_info.blk_tbl */
        if ((g_erase_test_info.threshold)&&
            ((g_erase_test_info.blk_tbl[(page<<this->page_shift)>>this->phys_erase_shift])>=g_erase_test_info.threshold)) {
                g_erase_test_info.threshold_flag = 1;
                wake_up_interruptible(&g_blkers_waitq);
                printk(KERN_INFO "block %d exceed erase access threshold %d.\n",
                        (page<<this->page_shift)>>this->phys_erase_shift, g_erase_test_info.threshold);
        }
#endif

}
#endif /*CONFIG_MOT_FEAT_NAND_RDDIST*/

EXPORT_SYMBOL_GPL (nand_scan);
EXPORT_SYMBOL_GPL (nand_release);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Steven J. Hill <sjhill@realitydiluted.com>, Thomas Gleixner <tglx@linutronix.de>");
MODULE_DESCRIPTION ("Generic NAND flash driver code");
