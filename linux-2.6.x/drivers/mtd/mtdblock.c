/* 
 * Direct MTD block device access
 *
 * $Id: mtdblock.c,v 1.65 2004/11/16 18:28:59 dwmw2 Exp $
 *
 * Copyright (C) 2006 - 2007  Motorola, Inc.
 * (C) 2000-2003 Nicolas Pitre <nico@cam.org>
 * (C) 1999-2003 David Woodhouse <dwmw2@infradead.org>
 *
 * ChangeLog:
 *(mm-dd-yyyy)  Author    Comment
 * 06-10-2006	Motorola  added CONFIG_MOT_FEAT_SECURE_DRM feature.
 *			  make sure "user" and "pds" partitions are handled 
 *			  as protected device.
 *
 * 10-20-2006	Motorola  added CONFIG_MOT_FEAT_MTD_AUTO_BBM feature.
 *			  provides automatically bad block replacement on block
 *			  device layer.
 *
 * 02-23-2007   Motorla   Renamed MOT_FEAT_SECURE_DRM feature to 
 *                        MOT_FEAT_SECURE_MTD
 *
 * 03-15-2007   Motorola added 'rsv' partition as a protected device.
 * 09-17-2007   Motorola added CONFIG_MOT_FEAT_MTD_SHA feature. 
 *                       Reduces bootup time by doing on demand shasum verification 
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>
#ifdef CONFIG_MOT_FEAT_MTD_SHA
#include <linux/bitmap.h>
#include <linux/crypto.h>
#include <asm/scatterlist.h>
#include <linux/mm.h>
#include <linux/mtd/mtd-sha.h>
#endif /* ifdef CONFIG_MOT_FEAT_MTD_SHA */

static struct mtdblk_dev {
	struct mtd_info *mtd;
	int count;
	struct semaphore cache_sem;
	unsigned char *cache_data;
	unsigned long cache_offset;
	unsigned int cache_size;
	enum { STATE_EMPTY, STATE_CLEAN, STATE_DIRTY } cache_state;
#ifdef CONFIG_MOT_FEAT_MTD_SHA
	struct mtdblk_sha *sha_state;
#endif /* CONFIG_MOT_FEAT_MTD_SHA */
} *mtdblks[MAX_MTD_DEVICES];

#ifdef CONFIG_MOT_FEAT_MTD_SHA
static int lru_init=0;
#endif /* CONFIG_MOT_FEAT_MTD_SHA */

#ifdef CONFIG_MOT_FEAT_SECURE_MTD // old #ifdef CONFIG_MOT_FEAT_SECURE_DRM
static inline int is_protected_device(struct mtd_info *mtd)
{
	return ((mtd) && (mtd->name) &&
		((strcmp(mtd->name, "pds") == 0) ||
		 (strcmp(mtd->name, "user") == 0)||
		 (strcmp(mtd->name, "rsv") == 0)));
}
#endif /* CONFIG_MOT_FEAT_SECURE_DRM */

/*
 * Cache stuff...
 * 
 * Since typical flash erasable sectors are much larger than what Linux's
 * buffer cache can handle, we must implement read-modify-write on flash
 * sectors for each block write requests.  To avoid over-erasing flash sectors
 * and to speed things up, we locally cache a whole flash sector while it is
 * being written to until a different sector is required.
 */

static void erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *)done->priv;
	wake_up(wait_q);
}

static int erase_write (struct mtd_info *mtd, unsigned long pos, 
			int len, const char *buf)
{
	struct erase_info erase;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	size_t retlen;
	int ret;

	/*
	 * First, let's erase the flash block.
	 */

	init_waitqueue_head(&wait_q);
	erase.mtd = mtd;
	erase.callback = erase_callback;
	erase.addr = pos;
	erase.len = len;
	erase.priv = (u_long)&wait_q;

	set_current_state(TASK_INTERRUPTIBLE);
	add_wait_queue(&wait_q, &wait);

	ret = MTD_ERASE(mtd, &erase);
	if (ret) {
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&wait_q, &wait);
		printk (KERN_WARNING "mtdblock: erase of region [0x%lx, 0x%x] "
				     "on \"%s\" failed\n",
			pos, len, mtd->name);
		return ret;
	}

	schedule();  /* Wait for erase to finish. */
	remove_wait_queue(&wait_q, &wait);

	/*
	 * Next, writhe data to flash.
	 */
#ifdef CONFIG_MOT_FEAT_MTD_AUTO_BBM
bbm_retry:
	ret = MTD_WRITE (mtd, pos, len, &retlen, buf);
	if (ret) {
		if (mtd->block_replace) {
			DEBUG(MTD_DEBUG_LEVEL0, "mtdblock: block_replace with pos %08x\n", (unsigned int)pos);
			if (mtd->block_replace(mtd, pos, 0)) {
				printk (KERN_ERR "mtdblock: out of replacement block for pos %08x\n",
					(unsigned int)pos);
				return ret;
			}
			/* try to write again with replacement block */
			goto bbm_retry;
		}
		return ret;
	}
#else

	ret = MTD_WRITE (mtd, pos, len, &retlen, buf);
	if (ret)
		return ret;
#endif
	if (retlen != len)
		return -EIO;
	return 0;
}


static int write_cached_data (struct mtdblk_dev *mtdblk)
{
	struct mtd_info *mtd = mtdblk->mtd;
	int ret;

	if (mtdblk->cache_state != STATE_DIRTY)
		return 0;

	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: writing cached data for \"%s\" "
			"at 0x%lx, size 0x%x\n", mtd->name, 
			mtdblk->cache_offset, mtdblk->cache_size);
	
	ret = erase_write (mtd, mtdblk->cache_offset, 
			   mtdblk->cache_size, mtdblk->cache_data);
	if (ret)
		return ret;

	/*
	 * Here we could argubly set the cache state to STATE_CLEAN.
	 * However this could lead to inconsistency since we will not 
	 * be notified if this content is altered on the flash by other 
	 * means.  Let's declare it empty and leave buffering tasks to
	 * the buffer cache instead.
	 */
	mtdblk->cache_state = STATE_EMPTY;
	return 0;
}


static int do_cached_write (struct mtdblk_dev *mtdblk, unsigned long pos, 
			    int len, const char *buf)
{
	struct mtd_info *mtd = mtdblk->mtd;
	unsigned int sect_size = mtdblk->cache_size;
	size_t retlen;
	int ret;

	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: write on \"%s\" at 0x%lx, size 0x%x\n",
		mtd->name, pos, len);
	
	if (!sect_size)
		return MTD_WRITE (mtd, pos, len, &retlen, buf);

	while (len > 0) {
		unsigned long sect_start = (pos/sect_size)*sect_size;
		unsigned int offset = pos - sect_start;
		unsigned int size = sect_size - offset;
		if( size > len ) 
			size = len;

		if (size == sect_size) {
			/* 
			 * We are covering a whole sector.  Thus there is no
			 * need to bother with the cache while it may still be
			 * useful for other partial writes.
			 */
			ret = erase_write (mtd, pos, size, buf);
			if (ret)
				return ret;
		} else {
			/* Partial sector: need to use the cache */

			if (mtdblk->cache_state == STATE_DIRTY &&
			    mtdblk->cache_offset != sect_start) {
				ret = write_cached_data(mtdblk);
				if (ret) 
					return ret;
			}

			if (mtdblk->cache_state == STATE_EMPTY ||
			    mtdblk->cache_offset != sect_start) {
				/* fill the cache with the current sector */
				mtdblk->cache_state = STATE_EMPTY;
				ret = MTD_READ(mtd, sect_start, sect_size, &retlen, mtdblk->cache_data);
				if (ret)
					return ret;
				if (retlen != sect_size)
					return -EIO;

				mtdblk->cache_offset = sect_start;
				mtdblk->cache_size = sect_size;
				mtdblk->cache_state = STATE_CLEAN;
			}

			/* write data to our local cache */
			memcpy (mtdblk->cache_data + offset, buf, size);
			mtdblk->cache_state = STATE_DIRTY;
		}

		buf += size;
		pos += size;
		len -= size;
	}

	return 0;
}


static int do_cached_read (struct mtdblk_dev *mtdblk, unsigned long pos, 
			   int len, char *buf)
{
	struct mtd_info *mtd = mtdblk->mtd;
	unsigned int sect_size = mtdblk->cache_size;
	size_t retlen;
	int ret;

	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: read on \"%s\" at 0x%lx, size 0x%x\n", 
			mtd->name, pos, len);
	
	if (!sect_size)
		return MTD_READ (mtd, pos, len, &retlen, buf);

	while (len > 0) {
		unsigned long sect_start = (pos/sect_size)*sect_size;
		unsigned int offset = pos - sect_start;
		unsigned int size = sect_size - offset;
		if (size > len) 
			size = len;
#ifdef CONFIG_MOT_FEAT_MTD_SHA
		if (((mtd->flags & MTD_SHA_ENABLED) == MTD_SHA_ENABLED) && 
			((pos/mtdblk->sha_state->hdt.chunk_size) < 
			 mtdblk->sha_state->hdt.number_of_chunks) &&
			!test_and_set_bit(pos/mtdblk->sha_state->hdt.chunk_size, 
			mtdblk->sha_state->sha_hash_bitmap)) {
			ret = MTD_READ(mtd, sect_start, 
					mtdblk->sha_state->hdt.chunk_size, 
					&retlen, mtdblk->cache_data);
			if (ret || (retlen != mtdblk->cache_size)) {
				printk("ERROR :: do_cached_read :: MTD_READ failure, panicing\n");
				mtdblock_write_to_atlas_and_panic(mtdblk->mtd);
			}

			mtdblk->cache_offset = sect_start;
			mtdblk->cache_state = STATE_CLEAN;

			/* mtdblock_verify_sha sum will not return incase of
			 * failure, It will AP panic */
			mtdblock_verify_sha_sum(mtdblk->mtd, 
					mtdblk->sha_state, 
					mtdblk->cache_data,
					mtdblk->cache_size,pos);

			memcpy (buf, mtdblk->cache_data + offset, size);
		} else {
#endif /* CONFIG_MOT_FEAT_MTD_SHA */
			/*
			 * Check if the requested data is already cached
			 * Read the requested amount of data from our internal cache if it
			 * contains what we want, otherwise we read the data directly
			 * from flash.
			 */
			if (mtdblk->cache_state != STATE_EMPTY &&
			    mtdblk->cache_offset == sect_start) {
				memcpy (buf, mtdblk->cache_data + offset, size);
			} else {
				ret = MTD_READ (mtd, pos, size, &retlen, buf);
				if (ret)
					return ret;
				if (retlen != size)
					return -EIO;
			}
#ifdef CONFIG_MOT_FEAT_MTD_SHA
		}
#endif /* CONFIG_MOT_FEAT_MTD_SHA */

		buf += size;
		pos += size;
		len -= size;
	}

	return 0;
}

static int mtdblock_readsect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];

#ifdef CONFIG_MOT_FEAT_SECURE_MTD // old #ifdef CONFIG_MOT_FEAT_SECURE_DRM
	if (is_protected_device(mtdblk->mtd))
	{
		return -EPERM;
	}
#endif /* CONFIG_MOT_FEAT_SECURE_DRM */

	return do_cached_read(mtdblk, block<<9, 512, buf);
}

static int mtdblock_writesect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];

#ifdef CONFIG_MOT_FEAT_SECURE_MTD // old #ifdef CONFIG_MOT_FEAT_SECURE_DRM
	if (is_protected_device(mtdblk->mtd))
	{
		return -EPERM;
	}
#endif /* CONFIG_MOT_FEAT_SECURE_DRM */

	if (unlikely(!mtdblk->cache_data && mtdblk->cache_size)) {
		mtdblk->cache_data = vmalloc(mtdblk->mtd->erasesize);
		if (!mtdblk->cache_data)
			return -EINTR;
		/* -EINTR is not really correct, but it is the best match
		 * documented in man 2 write for all cases.  We could also
		 * return -EAGAIN sometimes, but why bother?
		 */
	}
	return do_cached_write(mtdblk, block<<9, 512, buf);
}

static int mtdblock_open(struct mtd_blktrans_dev *mbd)
{
	struct mtdblk_dev *mtdblk;
	struct mtd_info *mtd = mbd->mtd;
	int dev = mbd->devnum;
#ifdef CONFIG_MOT_FEAT_MTD_SHA
	int ret;
	size_t retlen;
	uint32_t start;
#endif /* CONFIG_MOT_FEAT_MTD_SHA */

	DEBUG(MTD_DEBUG_LEVEL1,"mtdblock_open\n");
	
	if (mtdblks[dev]) {
		mtdblks[dev]->count++;
		return 0;
	}
	
	/* OK, it's not open. Create cache info for it */
	mtdblk = kmalloc(sizeof(struct mtdblk_dev), GFP_KERNEL);
	if (!mtdblk)
		return -ENOMEM;

	memset(mtdblk, 0, sizeof(*mtdblk));
	mtdblk->count = 1;
	mtdblk->mtd = mtd;
#ifdef CONFIG_MOT_FEAT_MTD_SHA
	if ((mtd->flags & MTD_SHA_ENABLED) == MTD_SHA_ENABLED) {
		mtdblk->sha_state = vmalloc(sizeof(struct mtdblk_sha));
		if (!mtdblk->sha_state) {
			panic("ERROR :: MTD_SHA :: Could not allocate mem for mtdlk->sha_state\n");
		}
		memset(mtdblk->sha_state, 0, sizeof(struct mtdblk_sha));

		/* Read in the HDT */
		ret = MTD_READ (mtd,mtd->size - CSF_SIZE - HDT_SIZE, HDT_SIZE, 
				&retlen, (char *)&mtdblk->sha_state->hdt); 
		if (ret || (retlen != HDT_SIZE)) {
			printk("ERROR :: Could not read HDT, retlen :: %d\n", retlen);
			mtdblock_write_to_atlas_and_panic(mtdblk->mtd);
		} 
		if((strncmp("root", mtdblk->mtd->name, 4)) == 0 ) {
			mtdblk->sha_state->code_group=0;
		}
		if((strncmp("lang", mtdblk->mtd->name, 4)) == 0 ) {
			mtdblk->sha_state->code_group=1;
		}

		mtdblock_hdt_sanity_check(mtdblk->sha_state);

		if(lru_init == 0) {
			lru_init = 1;
			sec_hash_lru_init(LRU_INIT_PAGES);
		}
		start = mtdblk->sha_state->hdt.hash_block_start_offset_from_image_start;
		sec_hash_add_code_group(mtdblk->sha_state->code_group, 
				mtdblk->sha_state->hdt.number_of_chunks,start); 

		mtdblk->cache_size = mtdblk->sha_state->hdt.chunk_size;
		mtdblk->cache_data = vmalloc(mtdblk->cache_size);
		if(!mtdblk->cache_data) {
			panic("ERROR :: MTD_SHA :: Could not allocate mem for mtblk->cache_data\n");
		}
		mtdblock_setup_crypto(mtdblk->sha_state, mtdblk->mtd, mtdblk->cache_size);
	}
#endif /* CONFIG_MOT_FEAT_MTD_SHA */
	init_MUTEX (&mtdblk->cache_sem);
	mtdblk->cache_state = STATE_EMPTY;
#ifdef CONFIG_MOT_FEAT_MTD_SHA
	/* The below check is needed to prevent cache_data being set 
	 * to NULL when the feature is defined, since we use this 
	 * for shasum validation 
	 */
	if ( ((mtd->flags & MTD_SHA_ENABLED) != MTD_SHA_ENABLED) && 
		((mtdblk->mtd->flags & MTD_CAP_RAM) != MTD_CAP_RAM) &&
		mtdblk->mtd->erasesize) {
#else /* CONFIG_MOT_FEAT_MTD_SHA */
	if ((mtdblk->mtd->flags & MTD_CAP_RAM) != MTD_CAP_RAM &&
	    mtdblk->mtd->erasesize) {
#endif /* CONFIG_MOT_FEAT_MTD_SHA */
		mtdblk->cache_size = mtdblk->mtd->erasesize;
		mtdblk->cache_data = NULL;
	}

	mtdblks[dev] = mtdblk;
	
	DEBUG(MTD_DEBUG_LEVEL1, "ok\n");

	return 0;
}

static int mtdblock_release(struct mtd_blktrans_dev *mbd)
{
	int dev = mbd->devnum;
	struct mtdblk_dev *mtdblk = mtdblks[dev];

   	DEBUG(MTD_DEBUG_LEVEL1, "mtdblock_release\n");

	down(&mtdblk->cache_sem);
	write_cached_data(mtdblk);
	up(&mtdblk->cache_sem);

	if (!--mtdblk->count) {
		/* It was the last usage. Free the device */
		mtdblks[dev] = NULL;
		if (mtdblk->mtd->sync)
			mtdblk->mtd->sync(mtdblk->mtd);
#ifdef CONFIG_MOT_FEAT_MTD_SHA
	if ((mtdblk->mtd->flags & MTD_SHA_ENABLED) == MTD_SHA_ENABLED) {
		vfree(mtdblk->sha_state->sha_hashes);
		vfree(mtdblk->sha_state->sha_hash_bitmap);
		vfree(mtdblk->sha_state);
	}
#endif /* CONFIG_MOT_FEAT_MTD_SHA */
		vfree(mtdblk->cache_data);
		kfree(mtdblk);
	}
	DEBUG(MTD_DEBUG_LEVEL1, "ok\n");

	return 0;
}  

static int mtdblock_flush(struct mtd_blktrans_dev *dev)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];

	down(&mtdblk->cache_sem);
	write_cached_data(mtdblk);
	up(&mtdblk->cache_sem);

	if (mtdblk->mtd->sync)
		mtdblk->mtd->sync(mtdblk->mtd);
	return 0;
}

static void mtdblock_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct mtd_blktrans_dev *dev = kmalloc(sizeof(*dev), GFP_KERNEL);

	if (!dev)
		return;

	memset(dev, 0, sizeof(*dev));

	dev->mtd = mtd;
	dev->devnum = mtd->index;
	dev->blksize = 512;
	dev->size = mtd->size >> 9;
	dev->tr = tr;

	if (!(mtd->flags & MTD_WRITEABLE))
		dev->readonly = 1;

	add_mtd_blktrans_dev(dev);
}

static void mtdblock_remove_dev(struct mtd_blktrans_dev *dev)
{
	del_mtd_blktrans_dev(dev);
	kfree(dev);
}

static struct mtd_blktrans_ops mtdblock_tr = {
	.name		= "mtdblock",
	.major		= 31,
	.part_bits	= 0,
	.open		= mtdblock_open,
	.flush		= mtdblock_flush,
	.release	= mtdblock_release,
	.readsect	= mtdblock_readsect,
	.writesect	= mtdblock_writesect,
	.add_mtd	= mtdblock_add_mtd,
	.remove_dev	= mtdblock_remove_dev,
	.owner		= THIS_MODULE,
};

static int __init init_mtdblock(void)
{
	return register_mtd_blktrans(&mtdblock_tr);
}

static void __exit cleanup_mtdblock(void)
{
	deregister_mtd_blktrans(&mtdblock_tr);
}

module_init(init_mtdblock);
module_exit(cleanup_mtdblock);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicolas Pitre <nico@cam.org> et al.");
MODULE_DESCRIPTION("Caching read/erase/writeback block device emulation access to MTD devices");

void mtdblock_flush_all(void)
{
	int i;

	for (i = 0; i < MAX_MTD_DEVICES; i++) {
		struct mtdblk_dev *mtdblk = mtdblks[i];

		if (mtdblk) {
			down(&mtdblk->cache_sem);
			write_cached_data(mtdblk);
			up(&mtdblk->cache_sem);

			if (mtdblk->mtd->sync)
				mtdblk->mtd->sync(mtdblk->mtd);
		}
	}
}
