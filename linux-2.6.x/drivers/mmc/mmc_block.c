/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 *  Cambridge, MA 02139, USA
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
/* Copyright (C)  2006-2008 Motorola, Inc.
 * Date		Author			Comment
 * 11/17/2006	Motorola		Support SDA 2.0
 * 11/23/2006	Motorola		Force remove md when blk removed
 * 03/08/2007	Motorola		Add an interface for hotplug event
 * 07/04/2007	Motorola		Add support for MC/MR
 * 09/28/2007   Motorola  		Jump out from dead loop when SDHC is in error
 * 03/25/2008	Motorola		Add support 4bit for SDA2.0 cards
 * 09/23/2008	Motorola		Support shredding card
 */
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/devfs_fs_kernel.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/protocol.h>
#include <linux/mmc/mmc.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include "mmc_queue.h"

static int major = MMC_BLK_DEV_NUM;

/*
 * There is one mmc_blk_data per slot.
 */
struct mmc_blk_data {
	spinlock_t	lock;
	struct gendisk	*disk;
	struct mmc_queue queue;

	unsigned int	usage;
	unsigned int	block_bits;
	unsigned int	suspended;
	int changed;
};

static DECLARE_MUTEX(open_lock);

extern int mmc_erase_allcard(struct mmc_card * card);

static struct mmc_blk_data *mmc_blk_get(struct gendisk *disk)
{
	struct mmc_blk_data *md;

	down(&open_lock);
	md = disk->private_data;
	if (md && md->usage == 0)
		md = NULL;
	if (md)
		md->usage++;
	up(&open_lock);

	return md;
}

static void mmc_blk_put(struct mmc_blk_data *md)
{
	down(&open_lock);
	md->usage--;
	if (md->usage == 0) {
		put_disk(md->disk);
		mmc_cleanup_queue(&md->queue);
		kfree(md);
	}
	up(&open_lock);
}

static int mmc_blk_open(struct inode *inode, struct file *filp)
{
	struct mmc_blk_data *md;
	int ret = -ENXIO;

	md = mmc_blk_get(inode->i_bdev->bd_disk);
	if (md) {
		if (md->usage > 1 || md->changed)
			check_disk_change(inode->i_bdev);
		ret = 0;

		if ((filp->f_mode & FMODE_WRITE) &&
			mmc_card_readonly(md->queue.card))
			ret = -EROFS;
	}

	return ret;
}

static int mmc_blk_release(struct inode *inode, struct file *filp)
{
	struct mmc_blk_data *md = inode->i_bdev->bd_disk->private_data;

	mmc_blk_put(md);
	return 0;
}

static int mmc_blk_media_change(struct gendisk *gd)
{
	struct mmc_blk_data *md = gd->private_data;
	int changed;

	changed = (md->changed ? 1 : 0);
	md->changed = 0;

	return changed;	
}

static int mmc_blk_revalidate(struct gendisk *gd)
{
	struct mmc_blk_data *md = gd->private_data;
	struct mmc_card *card = md->queue.card;	

	if(mmc_card_present(card)) {
		set_capacity(md->disk, card->csd.capacity);
	}	

	return 0;
}

static int
mmc_blk_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct block_device *bdev = inode->i_bdev;
	struct mmc_blk_data *md = inode->i_bdev->bd_disk->private_data;
	struct mmc_card *card = md->queue.card;
	int ret = 0;

	switch (cmd) {
	case BLKRRPART: {
		if (!capable(CAP_SYS_ADMIN))
                        return -EACCES;
                return mmc_blk_revalidate(bdev->bd_disk);
	}	
	case HDIO_GETGEO: { 
		struct hd_geometry geo;

		memset(&geo, 0, sizeof(struct hd_geometry));
		geo.cylinders	= get_capacity(bdev->bd_disk) / (4 * 16);
		geo.heads	= 4;
		geo.sectors	= 16;
		geo.start	= get_start_sect(bdev);

		return copy_to_user((void __user *)arg, &geo, sizeof(geo))
			? -EFAULT : 0;
	}
	case _IOR('I', 0x0f05, int):  /* For old MMC defines. */
	case IOCMMCGETCARDSTATUS: {
		/*
		 * The existing MMC driver returns the following statuses:
		 *   0x01 - Card is write protected
		 *   0x02 - Card is locked
		 *   0x04 - Attempt at a the last lock failed
		 *   0x08 - Locking not supported
		 */
		ret = IOCMMC_STATUS_LOCK_NOT_SUPPORTED;
		ret |= (mmc_card_readonly(card)? IOCMMC_STATUS_WRITE_PROTECTED : 0);
		ret |= (mmc_card_locked(card)? IOCMMC_STATUS_LOCKED : 0);
		ret |= (md->changed ? IOCMMC_STATUS_MEDIA_CHANGED : 0);

		return put_user(ret, (unsigned long *)arg);
	}
	case _IOR('I', 0x0f06, int):  /* For old MMC defines. */
	case IOCMMCGETCARDTYPE: {
		/*
		 * Return the card type as follows 
		 *  0 - MMC Card
		 *  1 - SD Card
		 *  2 - High Capacity SD Card
		 */
#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
		ret = (mmc_card_hcsd(card) ? TYPE_HCSD : (mmc_card_sd(card) ? TYPE_SDT : TYPE_MMC));
#else
		ret = mmc_card_sd(card) ? TYPE_SDT : TYPE_MMC;
#endif
/*
#if defined(CONFIG_MACH_PICO) && defined(CONFIG_MOT_FEAT_MEGASIM)
		if( (md->disk->first_minor >> MMC_SHIFT )==1 )
			ret = 5;
#endif
#if defined(CONFIG_MACH_ELBA) && defined(CONFIG_MOT_FEAT_INTERN_SD)
		if( (md->disk->first_minor >> MMC_SHIFT )==1 )
			ret = 4;
#endif
*/
		return put_user(ret, (unsigned long *)arg);
	}
        case _IOR('I', 0x0f09, int):  /* For old MMC defines. */
        case IOCMMCGETSIZE: {
		/*
		 * Returns the size of disk in bytes 
		 */
		ret = get_capacity(inode->i_bdev->bd_disk)*(1<<md->block_bits);
		return put_user(ret, (unsigned long *)arg);
	}
        case _IOR('I', 0x0f02, int):   /* For old MMC defines. */
        case IOCMMCGETCARDCID: {
            /*
             * Returns the 16 byte CID register 
             */
		unsigned char *cid = (unsigned char *)card->raw_cid;
		return copy_to_user((void *)arg, cid, 16);
	}
	case IOCMMCGETCARDCSD: {
      	    /*
       	     * Returns the 16 byte CSD register
     	     */
		unsigned char *csd = (unsigned char *)card->raw_csd;
		return copy_to_user((void *)arg,csd,16);
	}
       	case IOCMMCGETCARDOCR: {
     		u32 ocr = card->host->last_ocr;
		return put_user(ocr, (unsigned long *)arg);
	}
	case IOCMMCGETCARDDSR: {
	    /* DSR Register not supported now */
		return -EINVAL;
	}
	case IOCMMCLOCKCARD: {
				     printk("mmc_block ioctl: IOCMMCLOCKCARD not support \n");		      
				     return put_user(ret, (unsigned long *)arg);
			     }
	case IOCMMCUNLOCKCARD: {
				       printk("mmc_block ioctl: IOCMMCUNLOCKCARD not support \n");		      
				       return put_user(ret, (unsigned long *)arg);
			       }
	case IOCMMCERASECARD: {
				      int ret = mmc_erase_allcard(card);
				      printk("mxclay: erase result %d \n",ret);
				      return put_user(ret, (unsigned long *)arg);
			      }
	default:
		printk("mmc_block ioctl %d : Should not be here!\n", cmd);
	}

	return -ENOTTY;
}

static struct block_device_operations mmc_bdops = {
	.open			= mmc_blk_open,
	.release		= mmc_blk_release,
	.ioctl			= mmc_blk_ioctl,
	.media_changed		= mmc_blk_media_change,
	.revalidate_disk	= mmc_blk_revalidate,
	.owner			= THIS_MODULE,
};

struct mmc_blk_request {
	struct mmc_request	mrq;
	struct mmc_command	cmd;
	struct mmc_command	stop;
	struct mmc_data		data;
};

static int mmc_blk_prep_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	int stat = BLKPREP_OK;

	/*
	 * If we have no device, we haven't finished initialising.
	 */
	if (!md || !mq->card) {
		printk(KERN_ERR "%s: killing request - no device/host\n",
		       req->rq_disk->disk_name);
		stat = BLKPREP_KILL;
	}

	if (md->suspended) {
		blk_plug_device(md->queue.queue);
		stat = BLKPREP_DEFER;
	}

	/*
	 * Check for excessive requests.
	 */
	if (req->sector + req->nr_sectors > get_capacity(req->rq_disk)) {
		printk("bad request size\n");
		stat = BLKPREP_KILL;
	}

	return stat;
}

static int mmc_blk_issue_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	int ret;
DBG(2, "mmc_blk_issue_rq. req nrsects is 0x%lx \n", req->nr_sectors);
	if (mmc_card_claim_host(card))
		goto cmd_err;

	do {
		struct mmc_blk_request brq;
		struct mmc_command cmd;

		memset(&brq, 0, sizeof(struct mmc_blk_request));
		brq.mrq.cmd = &brq.cmd;
		brq.mrq.data = &brq.data;

#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
		if (mmc_card_hcsd(card)) {
			brq.cmd.arg = req->sector;
			brq.cmd.flags = MMC_RSP_R1;
			brq.data.req = req;
			brq.data.timeout_ns = card->csd.tacc_ns * 10;
			brq.data.timeout_clks = card->csd.tacc_clks * 10;
			brq.data.blksz_bits = 9;
			brq.data.blocks = req->nr_sectors;
		} else
#endif
		{
			brq.cmd.arg = req->sector << 9;
			brq.cmd.flags = MMC_RSP_R1;
			brq.data.req = req;
			brq.data.timeout_ns = card->csd.tacc_ns * 10;
			brq.data.timeout_clks = card->csd.tacc_clks * 10;
			brq.data.blksz_bits = md->block_bits;
			brq.data.blocks = req->nr_sectors >> (md->block_bits - 9);
		}

		brq.stop.opcode = MMC_STOP_TRANSMISSION;
		brq.stop.arg = 0;
		brq.stop.flags = MMC_RSP_R1B;

		if (rq_data_dir(req) == READ) {
			brq.cmd.opcode = brq.data.blocks > 1 ? MMC_READ_MULTIPLE_BLOCK : MMC_READ_SINGLE_BLOCK;
			brq.data.flags |= MMC_DATA_READ;
		} else {
			brq.cmd.opcode = brq.data.blocks > 1 ? MMC_WRITE_MULTIPLE_BLOCK : MMC_WRITE_BLOCK;
			brq.cmd.flags = MMC_RSP_R1B;
			brq.data.flags |= MMC_DATA_WRITE;

			/* sync the write data to all caches */
			if (md->usage > 1)
				md->changed = 1; 
		}
		brq.mrq.stop = brq.data.blocks > 1 ? &brq.stop : NULL;

		brq.data.sg = mq->sg;
		brq.data.sg_len = blk_rq_map_sg(req->q, req, brq.data.sg);

		mmc_wait_for_req(card->host, &brq.mrq);
		if (brq.cmd.error) {
			printk(KERN_ERR "%s: error %d sending read/write command\n",
			       req->rq_disk->disk_name, brq.cmd.error);
			goto cmd_err;
		}

		if (brq.data.error) {
			printk(KERN_ERR "%s: error %d transferring data\n",
			       req->rq_disk->disk_name, brq.data.error);
			goto cmd_err;
		}

		if (brq.stop.error) {
			printk(KERN_ERR "%s: error %d sending stop command\n",
			       req->rq_disk->disk_name, brq.stop.error);
			goto cmd_err;
		}

		do {
			int err;

			cmd.opcode = MMC_SEND_STATUS;
			cmd.arg = card->rca << 16;
			cmd.flags = MMC_RSP_R1;
			err = mmc_wait_for_cmd(card->host, &cmd, 5);
			if (err) {
				printk(KERN_ERR "%s: error %d requesting status\n",
				       req->rq_disk->disk_name, err);
				goto cmd_err;
			}
//#ifdef CONFIG_MMC_BLOCK_BROKEN_RFD
			/* Work-around for broken cards setting READY_FOR_DATA
			 * when not actually ready.
			 */
			/* some megasim cards have this issue,
			 * enable this work-around.
			 */
			if (R1_CURRENT_STATE(cmd.resp[0]) == 7)
				cmd.resp[0] &= ~R1_READY_FOR_DATA;
//#endif

                        /* in the drop test, the sdhc is in a bad status, 
                         * that the status value bacome 0, so jump out */
			if (cmd.resp[0] == 0)
				goto cmd_err;

		} while (!(cmd.resp[0] & R1_READY_FOR_DATA));

#if 0
		if (cmd.resp[0] & ~0x00000900)
			printk(KERN_ERR "%s: status = %08x\n",
			       req->rq_disk->disk_name, cmd.resp[0]);
		if (mmc_decode_status(cmd.resp))
			goto cmd_err;
#endif

		/*
		 * A block was successfully transferred.
		 */
		spin_lock_irq(&md->lock);
		ret = end_that_request_chunk(req, 1, brq.data.bytes_xfered);
		if (!ret) {
			/*
			 * The whole request completed successfully.
			 */
			add_disk_randomness(req->rq_disk);
			blkdev_dequeue_request(req);
			end_that_request_last(req);
		}
		spin_unlock_irq(&md->lock);
	} while (ret);

	mmc_card_release_host(card);

	return 1;

 cmd_err:
	mmc_card_release_host(card);

	/*
	 * This is a little draconian, but until we get proper
	 * error handling sorted out here, its the best we can
	 * do - especially as some hosts have no idea how much
	 * data was transferred before the error occurred.
	 */
	spin_lock_irq(&md->lock);
	do {
		ret = end_that_request_chunk(req, 0,
				req->current_nr_sectors << 9);
	} while (ret);

	add_disk_randomness(req->rq_disk);
	blkdev_dequeue_request(req);
	end_that_request_last(req);
	spin_unlock_irq(&md->lock);

	return 0;
}

#define MMC_NUM_MINORS	(256 >> MMC_SHIFT)

static unsigned long dev_use[MMC_NUM_MINORS/(8*sizeof(unsigned long))];

static struct mmc_blk_data *mmc_blk_alloc(struct mmc_card *card)
{
	struct mmc_blk_data *md;
	int devidx, ret;
#ifdef CONFIG_MOT_FEAT_MMCSD_DEV_HOST_BIND	
	struct platform_device *pdev = to_platform_device(card->dev.parent);

	devidx = pdev->id;
#else
	devidx = find_first_zero_bit(dev_use, MMC_NUM_MINORS);
#endif
	if (devidx >= MMC_NUM_MINORS)
		return ERR_PTR(-ENOSPC);
	__set_bit(devidx, dev_use);

	md = kmalloc(sizeof(struct mmc_blk_data), GFP_KERNEL);
	if (md) {
		memset(md, 0, sizeof(struct mmc_blk_data));

		md->disk = alloc_disk(1 << MMC_SHIFT);
		if (md->disk == NULL) {
			kfree(md);
			md = ERR_PTR(-ENOMEM);
			goto out;
		}

		spin_lock_init(&md->lock);
		md->usage = 1;

		ret = mmc_init_queue(&md->queue, card, &md->lock);
		if (ret) {
			put_disk(md->disk);
			kfree(md);
			md = ERR_PTR(ret);
			goto out;
		}
		md->queue.prep_fn = mmc_blk_prep_rq;
		md->queue.issue_fn = mmc_blk_issue_rq;
		md->queue.data = md;

		md->disk->major	= major;
		md->disk->first_minor = devidx << MMC_SHIFT;
		md->disk->fops = &mmc_bdops;
		md->disk->private_data = md;
		md->disk->queue = md->queue.queue;
		md->disk->driverfs_dev = &card->dev;

		sprintf(md->disk->disk_name, "mmcblk%d", devidx);
		sprintf(md->disk->devfs_name, "mmc/blk%d", devidx);

		md->block_bits = card->csd.read_blkbits;
		md->changed = 1;

		blk_queue_hardsect_size(md->queue.queue, 1 << md->block_bits);
		set_capacity(md->disk, card->csd.capacity);
	}
 out:
	return md;
}

static int
mmc_blk_set_blksize(struct mmc_blk_data *md, struct mmc_card *card)
{
	struct mmc_command cmd;
	int err;

	mmc_card_claim_host(card);
	cmd.opcode = MMC_SET_BLOCKLEN;
	cmd.arg = 1 << card->csd.read_blkbits;
	cmd.flags = MMC_RSP_R1;
	err = mmc_wait_for_cmd(card->host, &cmd, 5);
	mmc_card_release_host(card);

	if (err) {
		printk(KERN_ERR "%s: unable to set block size to %d: %d\n",
			md->disk->disk_name, cmd.arg, err);
		return -EINVAL;
	}

	return 0;
}

static int mmc_blk_probe(struct mmc_card *card)
{
	struct mmc_blk_data *md;
	int err;
DBG(2,"mmc_blk_probe: begin \n");
	/* modified for the switch command should be support by many sd card */
	if( card->csd.cmdclass & ~0x5ff)	
		return -ENODEV;

	if (card->csd.read_blkbits < 9) {
		printk(KERN_WARNING "%s: read blocksize too small (%u)\n",
			mmc_card_id(card), 1 << card->csd.read_blkbits);
		return -ENODEV;
	}

	md = mmc_blk_alloc(card);
	if (IS_ERR(md))
		return PTR_ERR(md);

	err = mmc_blk_set_blksize(md, card);
	if (err)
		goto out;

	printk(KERN_INFO "%s: %s %s %uKiB %s\n",
		md->disk->disk_name, mmc_card_id(card), mmc_card_name(card),
		(card->csd.capacity << card->csd.read_blkbits) / 1024,
		mmc_card_readonly(card)?"(ro)":"");

	mmc_set_drvdata(card, md);
	add_disk(md->disk);

	return 0;

 out:
	mmc_blk_put(md);

	return err;
}

static void mmc_blk_remove(struct mmc_card *card)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		int devidx;
		del_gendisk(md->disk);

		/*
		 * I think this is needed.
		 */
		md->disk->queue = NULL;

		devidx = md->disk->first_minor >> MMC_SHIFT;
		__clear_bit(devidx, dev_use);

		/* force remove md */
		md->usage = 1;
		mmc_blk_put(md);
	}
	mmc_set_drvdata(card, NULL);
}

unsigned int mmc_blk_get_refcount(int module)
{
	struct block_device *bdev;
	unsigned int part_ref_count = 0;
	
	bdev = bdget(MKDEV(major, module << MMC_SHIFT));
	
	if (bdev) {
		part_ref_count = bdev->bd_part_count;	
		bdput(bdev);
	}

	return part_ref_count;
}

EXPORT_SYMBOL(mmc_blk_get_refcount);

#ifdef CONFIG_PM
static int mmc_blk_suspend(struct mmc_card *card, u32 state)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		mmc_queue_suspend(&md->queue);
	}
	return 0;
}

static int mmc_blk_resume(struct mmc_card *card)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		//mmc_blk_set_blksize(md, card);
		mmc_queue_resume(&md->queue);
	}
	return 0;
}
#else
#define	mmc_blk_suspend	NULL
#define mmc_blk_resume	NULL
#endif

static struct mmc_driver mmc_driver = {
	.drv		= {
		.name	= "mmcblk",
	},
	.probe		= mmc_blk_probe,
	.remove		= mmc_blk_remove,
	.suspend	= mmc_blk_suspend,
	.resume		= mmc_blk_resume,
};

static int __init mmc_blk_init(void)
{
	int res = -ENOMEM;

	res = register_blkdev(major, "mmc");
	if (res < 0) {
		printk(KERN_WARNING "Unable to get major %d for MMC media: %d\n",
		       major, res);
		goto out;
	}
	if (major == 0)
		major = res;

	devfs_mk_dir("mmc");
	devfs_mk_bdev(MKDEV(major, 0), S_IFBLK|S_IRUGO|S_IWUSR, "mmca");
	devfs_mk_bdev(MKDEV(major, 1), S_IFBLK|S_IRUGO|S_IWUSR, "mmca1");

	return mmc_register_driver(&mmc_driver);

 out:
	return res;
}

static void __exit mmc_blk_exit(void)
{
	mmc_unregister_driver(&mmc_driver);
	devfs_remove("mmc");
	unregister_blkdev(major, "mmc");
}

module_init(mmc_blk_init);
module_exit(mmc_blk_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multimedia Card (MMC) block device driver");

module_param(major, int, 0444);
MODULE_PARM_DESC(major, "specify the major device number for MMC block driver");
