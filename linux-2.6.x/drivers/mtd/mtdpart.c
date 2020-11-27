/*
 * Simple MTD partitioning layer
 *
 * Copyright (C) 2005-2008 Motorola, Inc.
 * (C) 2000 Nicolas Pitre <nico@cam.org>
 *
 * This code is GPL
 *
 * $Id: mtdpart.c,v 1.52 2005/01/12 22:34:33 gleixner Exp $
 *
 * 	02-21-2002	Thomas Gleixner <gleixner@autronix.de>
 *			added support for read_oob, write_oob
 *
 * 05-06-2005   feature CONFIG_MTD_NAND_BBM added by Motorola, Inc.
 *		create the block reserve pool partition.
 *		verify the size and start address of the reserve pool partition.
 *
 * 09-26-2005   feature CONFIG_MOT_FEAT_KPANIC added by Motorola, Inc.
 *              Whenever the kernel panics, the printk ring buffer will be written
 *              out to a dedicated flash partition.
 *    
 * 04-10-2006   feature CONFIG_MOT_FEAT_NAND_RDDIST added by Motorola, Inc.
 *              the nand_base driver will work with nand_watchdog (user space daemon), 
 *              while doing block read: automaticlly fix block if 1bit ECC error detected.
 *              while block fixing requested by nand_watchdog: the block will be fixed
 *              for read distrub recovery.
 *
 * 11-29-2006	feature CONFIG_MOT_FEAT_MEMDUMP added by Motorola, Inc.
 *		dump the memory content to the mass storage partition on panic
 *
 * 04-15-2007   update part_read_distfix function with reason_code for CONFIG_MOT_FEAT_NAND_RDDIST
 *		feature
 * 01-03-2008	initialize rsvdblock_offset to avoid this variable being used without initialization. 
 * 02-26-2008   change flash rsvblock address for xpixl
 */	

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/config.h>
#include <linux/kmod.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/compatmac.h>
#ifdef CONFIG_MOT_FEAT_KPANIC
#include <linux/string.h>
#endif /* CONFIG_MOT_FEAT_KPANIC */

#if defined(CONFIG_MTD_NAND_BBM)
/*
 * reserved block offset 
 * the last NAND partition will be used as reserved blocks 
 */
#if defined(CONFIG_MACH_PICO) || defined(CONFIG_MACH_MARCO) || defined(CONFIG_MACH_NEVIS)
unsigned long rsvdblock_offset = 0x07ce0000;

#elif defined(CONFIG_MACH_XPIXL)
unsigned long rsvdblock_offset = 0x1f560000;

#else
/* 
 * ATTENTION: rsvdblock_offset should be initialized here.
 * You can refer to the above initialization for Pico/Marco.
 * Find the correct value in bootup kernel log.
 */
unsigned long rsvdblock_offset;
#endif

#endif /* CONFIG_MTD_NAND_BBM */

#ifdef CONFIG_MOT_FEAT_KPANIC
struct mtd_info *kpanic_partition = NULL;
#endif /* CONFIG_MOT_FEAT_KPANIC */

#ifdef CONFIG_MOT_FEAT_MEMDUMP
struct mtd_info *memdump_partition = NULL;
char dumppart_name[64];
#endif /* CONFIG_MOT_FEAT_MEMDUMP */

/* Our partition linked list */
static LIST_HEAD(mtd_partitions);

/* Our partition node structure */
struct mtd_part {
	struct mtd_info mtd;
	struct mtd_info *master;
	u_int32_t offset;
	int index;
	struct list_head list;
	int registered;
};

/*
 * Given a pointer to the MTD object in the mtd_part structure, we can retrieve
 * the pointer to that structure with this macro.
 */
#define PART(x)  ((struct mtd_part *)(x))

	
/* 
 * MTD methods which simply translate the effective address and pass through
 * to the _real_ device.
 */

static int part_read (struct mtd_info *mtd, loff_t from, size_t len, 
			size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	if (from >= mtd->size)
		len = 0;
	else if (from + len > mtd->size)
		len = mtd->size - from;
	if (part->master->read_ecc == NULL)	
		return part->master->read (part->master, from + part->offset, 
					len, retlen, buf);
	else
		return part->master->read_ecc (part->master, from + part->offset, 
					len, retlen, buf, NULL, &mtd->oobinfo);
}

static int part_point (struct mtd_info *mtd, loff_t from, size_t len, 
			size_t *retlen, u_char **buf)
{
	struct mtd_part *part = PART(mtd);
	if (from >= mtd->size)
		len = 0;
	else if (from + len > mtd->size)
		len = mtd->size - from;
	return part->master->point (part->master, from + part->offset, 
				    len, retlen, buf);
}
static void part_unpoint (struct mtd_info *mtd, u_char *addr, loff_t from, size_t len)
{
	struct mtd_part *part = PART(mtd);

	part->master->unpoint (part->master, addr, from + part->offset, len);
}


static int part_read_ecc (struct mtd_info *mtd, loff_t from, size_t len, 
			size_t *retlen, u_char *buf, u_char *eccbuf, struct nand_oobinfo *oobsel)
{
	struct mtd_part *part = PART(mtd);
	if (oobsel == NULL)
		oobsel = &mtd->oobinfo;
	if (from >= mtd->size)
		len = 0;
	else if (from + len > mtd->size)
		len = mtd->size - from;
	return part->master->read_ecc (part->master, from + part->offset, 
					len, retlen, buf, eccbuf, oobsel);
}

static int part_read_oob (struct mtd_info *mtd, loff_t from, size_t len, 
			size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	if (from >= mtd->size)
		len = 0;
	else if (from + len > mtd->size)
		len = mtd->size - from;
	return part->master->read_oob (part->master, from + part->offset, 
					len, retlen, buf);
}

#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
static uint32_t part_get_offset (struct mtd_info *mtd)
{
	struct mtd_part *part = PART(mtd);
	return part->offset;
}
static int part_read_distfix (struct mtd_info *mtd, int block, int reason_code)
{
	struct mtd_part *part = PART(mtd);
	int fixblock = block + (int)(part->offset/part->master->erasesize);
	return part->master->read_distfix (part->master, fixblock, reason_code);
}
#endif /*CONFIG_MOT_FEAT_NAND_RDDIST*/

static int part_read_user_prot_reg (struct mtd_info *mtd, loff_t from, size_t len, 
			size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	return part->master->read_user_prot_reg (part->master, from, 
					len, retlen, buf);
}

static int part_get_user_prot_info (struct mtd_info *mtd,
				    struct otp_info *buf, size_t len)
{
	struct mtd_part *part = PART(mtd);
	return part->master->get_user_prot_info (part->master, buf, len);
}

static int part_read_fact_prot_reg (struct mtd_info *mtd, loff_t from, size_t len, 
			size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	return part->master->read_fact_prot_reg (part->master, from, 
					len, retlen, buf);
}

static int part_get_fact_prot_info (struct mtd_info *mtd,
				    struct otp_info *buf, size_t len)
{
	struct mtd_part *part = PART(mtd);
	return part->master->get_fact_prot_info (part->master, buf, len);
}

static int part_write (struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (to >= mtd->size)
		len = 0;
	else if (to + len > mtd->size)
		len = mtd->size - to;
	if (part->master->write_ecc == NULL)	
		return part->master->write (part->master, to + part->offset, 
					len, retlen, buf);
	else
		return part->master->write_ecc (part->master, to + part->offset, 
					len, retlen, buf, NULL, &mtd->oobinfo);
							
}

#ifdef CONFIG_MOT_FEAT_MTD_AUTO_BBM
static int part_block_replace (struct mtd_info *mtd, loff_t ofs, int lock)
{
	struct mtd_part *part = PART(mtd);
	return part->master->block_replace (part->master, ofs + part->offset, lock);
}
#endif

static int part_write_ecc (struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const u_char *buf,
			 u_char *eccbuf, struct nand_oobinfo *oobsel)
{
	struct mtd_part *part = PART(mtd);
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (oobsel == NULL)
		oobsel = &mtd->oobinfo;
	if (to >= mtd->size)
		len = 0;
	else if (to + len > mtd->size)
		len = mtd->size - to;
	return part->master->write_ecc (part->master, to + part->offset, 
					len, retlen, buf, eccbuf, oobsel);
}

static int part_write_oob (struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (to >= mtd->size)
		len = 0;
	else if (to + len > mtd->size)
		len = mtd->size - to;
	return part->master->write_oob (part->master, to + part->offset, 
					len, retlen, buf);
}

static int part_write_user_prot_reg (struct mtd_info *mtd, loff_t from, size_t len, 
			size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	return part->master->write_user_prot_reg (part->master, from, 
					len, retlen, buf);
}

static int part_lock_user_prot_reg (struct mtd_info *mtd, loff_t from, size_t len) 
{
	struct mtd_part *part = PART(mtd);
	return part->master->lock_user_prot_reg (part->master, from, len);
}

static int part_writev (struct mtd_info *mtd,  const struct kvec *vecs,
			 unsigned long count, loff_t to, size_t *retlen)
{
	struct mtd_part *part = PART(mtd);
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (part->master->writev_ecc == NULL)	
		return part->master->writev (part->master, vecs, count,
					to + part->offset, retlen);
	else
		return part->master->writev_ecc (part->master, vecs, count,
					to + part->offset, retlen,
					NULL, &mtd->oobinfo);
}

static int part_readv (struct mtd_info *mtd,  struct kvec *vecs,
			 unsigned long count, loff_t from, size_t *retlen)
{
	struct mtd_part *part = PART(mtd);
	if (part->master->readv_ecc == NULL)	
		return part->master->readv (part->master, vecs, count,
					from + part->offset, retlen);
	else
		return part->master->readv_ecc (part->master, vecs, count,
					from + part->offset, retlen, 
					NULL, &mtd->oobinfo);
}

static int part_writev_ecc (struct mtd_info *mtd,  const struct kvec *vecs,
			 unsigned long count, loff_t to, size_t *retlen,
			 u_char *eccbuf,  struct nand_oobinfo *oobsel)
{
	struct mtd_part *part = PART(mtd);
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (oobsel == NULL)
		oobsel = &mtd->oobinfo;
	return part->master->writev_ecc (part->master, vecs, count,
					to + part->offset, retlen,
					eccbuf, oobsel);
}

static int part_readv_ecc (struct mtd_info *mtd,  struct kvec *vecs,
			 unsigned long count, loff_t from, size_t *retlen,
			 u_char *eccbuf,  struct nand_oobinfo *oobsel)
{
	struct mtd_part *part = PART(mtd);
	if (oobsel == NULL)
		oobsel = &mtd->oobinfo;
	return part->master->readv_ecc (part->master, vecs, count,
					from + part->offset, retlen, 
					eccbuf, oobsel);
}

static int part_erase (struct mtd_info *mtd, struct erase_info *instr)
{
	struct mtd_part *part = PART(mtd);
	int ret;
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (instr->addr >= mtd->size)
		return -EINVAL;
	instr->addr += part->offset;
	ret = part->master->erase(part->master, instr);
	return ret;
}

void mtd_erase_callback(struct erase_info *instr)
{
	if (instr->mtd->erase == part_erase) {
		struct mtd_part *part = PART(instr->mtd);

		if (instr->fail_addr != 0xffffffff)
			instr->fail_addr -= part->offset;
		instr->addr -= part->offset;
	}
	if (instr->callback)
		instr->callback(instr);
}
EXPORT_SYMBOL_GPL(mtd_erase_callback);

static int part_lock (struct mtd_info *mtd, loff_t ofs, size_t len)
{
	struct mtd_part *part = PART(mtd);
	if ((len + ofs) > mtd->size) 
		return -EINVAL;
	return part->master->lock(part->master, ofs + part->offset, len);
}

static int part_unlock (struct mtd_info *mtd, loff_t ofs, size_t len)
{
	struct mtd_part *part = PART(mtd);
	if ((len + ofs) > mtd->size) 
		return -EINVAL;
	return part->master->unlock(part->master, ofs + part->offset, len);
}

static void part_sync(struct mtd_info *mtd)
{
	struct mtd_part *part = PART(mtd);
	part->master->sync(part->master);
}

static int part_suspend(struct mtd_info *mtd)
{
	struct mtd_part *part = PART(mtd);
	return part->master->suspend(part->master);
}

static void part_resume(struct mtd_info *mtd)
{
	struct mtd_part *part = PART(mtd);
	part->master->resume(part->master);
}

static int part_block_isbad (struct mtd_info *mtd, loff_t ofs)
{
	struct mtd_part *part = PART(mtd);
	if (ofs >= mtd->size)
		return -EINVAL;
	ofs += part->offset;
	return part->master->block_isbad(part->master, ofs);
}

static int part_block_markbad (struct mtd_info *mtd, loff_t ofs)
{
	struct mtd_part *part = PART(mtd);
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (ofs >= mtd->size)
		return -EINVAL;
	ofs += part->offset;
	return part->master->block_markbad(part->master, ofs);
}

/* 
 * This function unregisters and destroy all slave MTD objects which are 
 * attached to the given master MTD object.
 */

int del_mtd_partitions(struct mtd_info *master)
{
	struct list_head *node;
	struct mtd_part *slave;

	for (node = mtd_partitions.next;
	     node != &mtd_partitions;
	     node = node->next) {
		slave = list_entry(node, struct mtd_part, list);
		if (slave->master == master) {
			struct list_head *prev = node->prev;
			__list_del(prev, node->next);
			if(slave->registered)
				del_mtd_device(&slave->mtd);
			kfree(slave);
			node = prev;
		}
	}

	return 0;
}

/*
 * This function, given a master MTD object and a partition table, creates
 * and registers slave MTD objects which are bound to the master according to
 * the partition definitions.
 * (Q: should we register the master MTD object as well?)
 */

int add_mtd_partitions(struct mtd_info *master, 
		       const struct mtd_partition *parts,
		       int nbparts)
{
	struct mtd_part *slave;
	u_int32_t cur_offset = 0;
	int i;

	printk (KERN_NOTICE "Creating %d MTD partitions on \"%s\":\n", nbparts, master->name);

	for (i = 0; i < nbparts; i++) {

		/* allocate the partition structure */
		slave = kmalloc (sizeof(*slave), GFP_KERNEL);
		if (!slave) {
			printk ("memory allocation error while creating partitions for \"%s\"\n",
				master->name);
			del_mtd_partitions(master);
			return -ENOMEM;
		}
		memset(slave, 0, sizeof(*slave));
		list_add(&slave->list, &mtd_partitions);

		/* set up the MTD object for this partition */
		slave->mtd.type = master->type;
		slave->mtd.flags = master->flags & ~parts[i].mask_flags;
		slave->mtd.size = parts[i].size;
		slave->mtd.oobblock = master->oobblock;
		slave->mtd.oobsize = master->oobsize;
		slave->mtd.ecctype = master->ecctype;
		slave->mtd.eccsize = master->eccsize;

		slave->mtd.name = parts[i].name;
		slave->mtd.bank_size = master->bank_size;
		slave->mtd.owner = master->owner;

		slave->mtd.read = part_read;
		slave->mtd.write = part_write;
#ifdef CONFIG_MOT_FEAT_MTD_AUTO_BBM
		if (master->block_replace)
			slave->mtd.block_replace = part_block_replace;
#endif
		if(master->point && master->unpoint){
			slave->mtd.point = part_point;
			slave->mtd.unpoint = part_unpoint;
		}
		
		if (master->read_ecc)
			slave->mtd.read_ecc = part_read_ecc;
		if (master->write_ecc)
			slave->mtd.write_ecc = part_write_ecc;
		if (master->read_oob)
			slave->mtd.read_oob = part_read_oob;
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
		if (master->read_distfix)
			slave->mtd.read_distfix = part_read_distfix;
		slave->mtd.get_part_offset = part_get_offset;
#endif /*CONFIG_MOT_FEAT_NAND_RDDIST*/
		if (master->write_oob)
			slave->mtd.write_oob = part_write_oob;
		if(master->read_user_prot_reg)
			slave->mtd.read_user_prot_reg = part_read_user_prot_reg;
		if(master->read_fact_prot_reg)
			slave->mtd.read_fact_prot_reg = part_read_fact_prot_reg;
		if(master->write_user_prot_reg)
			slave->mtd.write_user_prot_reg = part_write_user_prot_reg;
		if(master->lock_user_prot_reg)
			slave->mtd.lock_user_prot_reg = part_lock_user_prot_reg;
		if(master->get_user_prot_info)
			slave->mtd.get_user_prot_info = part_get_user_prot_info;
		if(master->get_fact_prot_info)
			slave->mtd.get_fact_prot_info = part_get_fact_prot_info;
		if (master->sync)
			slave->mtd.sync = part_sync;
		if (!i && master->suspend && master->resume) {
				slave->mtd.suspend = part_suspend;
				slave->mtd.resume = part_resume;
		}
		if (master->writev)
			slave->mtd.writev = part_writev;
		if (master->readv)
			slave->mtd.readv = part_readv;
		if (master->writev_ecc)
			slave->mtd.writev_ecc = part_writev_ecc;
		if (master->readv_ecc)
			slave->mtd.readv_ecc = part_readv_ecc;
		if (master->lock)
			slave->mtd.lock = part_lock;
		if (master->unlock)
			slave->mtd.unlock = part_unlock;
		if (master->block_isbad)
			slave->mtd.block_isbad = part_block_isbad;
		if (master->block_markbad)
			slave->mtd.block_markbad = part_block_markbad;
		slave->mtd.erase = part_erase;
		slave->master = master;
		slave->offset = parts[i].offset;
		slave->index = i;

		if (slave->offset == MTDPART_OFS_APPEND)
			slave->offset = cur_offset;
		if (slave->offset == MTDPART_OFS_NXTBLK) {
			u_int32_t emask = master->erasesize-1;
			slave->offset = (cur_offset + emask) & ~emask;
			if (slave->offset != cur_offset) {
				printk(KERN_NOTICE "Moving partition %d: "
				       "0x%08x -> 0x%08x\n", i,
				       cur_offset, slave->offset);
			}
		}
		if (slave->mtd.size == MTDPART_SIZ_FULL)
			slave->mtd.size = master->size - slave->offset;
		cur_offset = slave->offset + slave->mtd.size;
	
		printk (KERN_NOTICE "0x%08x-0x%08x : \"%s\"\n", slave->offset, 
			slave->offset + slave->mtd.size, slave->mtd.name);

		/* let's do some sanity checks */
		if (slave->offset >= master->size) {
				/* let's register it anyway to preserve ordering */
			slave->offset = 0;
			slave->mtd.size = 0;
			printk ("mtd: partition \"%s\" is out of reach -- disabled\n",
				parts[i].name);
		}
		if (slave->offset + slave->mtd.size > master->size) {
			slave->mtd.size = master->size - slave->offset;
			printk ("mtd: partition \"%s\" extends beyond the end of device \"%s\" -- size truncated to %#x\n",
				parts[i].name, master->name, slave->mtd.size);
		}
		if (master->numeraseregions>1) {
			/* Deal with variable erase size stuff */
			int i;
			struct mtd_erase_region_info *regions = master->eraseregions;
			
			/* Find the first erase regions which is part of this partition. */
			for (i=0; i < master->numeraseregions && slave->offset >= regions[i].offset; i++)
				;

			for (i--; i < master->numeraseregions && slave->offset + slave->mtd.size > regions[i].offset; i++) {
				if (slave->mtd.erasesize < regions[i].erasesize) {
					slave->mtd.erasesize = regions[i].erasesize;
				}
			}
		} else {
			/* Single erase size */
			slave->mtd.erasesize = master->erasesize;
		}

		if ((slave->mtd.flags & MTD_WRITEABLE) && 
		    (slave->offset % slave->mtd.erasesize)) {
			/* Doesn't start on a boundary of major erase size */
			/* FIXME: Let it be writable if it is on a boundary of _minor_ erase size though */
			slave->mtd.flags &= ~MTD_WRITEABLE;
			printk ("mtd: partition \"%s\" doesn't start on an erase block boundary -- force read-only\n",
				parts[i].name);
		}
		if ((slave->mtd.flags & MTD_WRITEABLE) && 
		    (slave->mtd.size % slave->mtd.erasesize)) {
			slave->mtd.flags &= ~MTD_WRITEABLE;
			printk ("mtd: partition \"%s\" doesn't end on an erase block -- force read-only\n",
				parts[i].name);
		}

		/* copy oobinfo from master */ 
		memcpy(&slave->mtd.oobinfo, &master->oobinfo, sizeof(slave->mtd.oobinfo));

		if(parts[i].mtdp)
		{	/* store the object pointer (caller may or may not register it */
			*parts[i].mtdp = &slave->mtd;
			slave->registered = 0;
		}
		else
		{
			/* register our partition */
			add_mtd_device(&slave->mtd);
			slave->registered = 1;
		}

#ifdef CONFIG_MOT_FEAT_KPANIC
		/* save a pointer to mtd_info struct for the kpanic partition */
		if (!strcmp(slave->mtd.name, "kpanic")) {
			kpanic_partition = &slave->mtd;
		}
#ifdef CONFIG_MOT_FEAT_MEMDUMP
		else if (!strcmp(slave->mtd.name, dumppart_name)) {
			memdump_partition = &slave->mtd;
		}
#endif /* CONFIG_MOT_FEAT_MEMDUMP  */

#endif /* CONFIG_MOT_FEAT_KPANIC */

#if defined(CONFIG_MTD_NAND_BBM)
		/* preset the reserved block offset */
		if  (master->type == MTD_NANDFLASH && i == nbparts-1) {
			int bbmblocks = 0;
			int rsvdblocks = 0;
			if (strcmp(slave->mtd.name, "rsv"))
				panic("%s's \"rsv\" partition does not exist, NAND Bad Block cann't be handled!\n",
					master->name);

			if (rsvdblock_offset != slave->offset)
				printk(KERN_ERR "BUG: rsvdblock_offset (0x%08x) will be re-initialized to 0x%08x\n", rsvdblock_offset, slave->offset);
			rsvdblock_offset = slave->offset;
			printk (KERN_INFO "%s's capacity:%dMB, reserved block offset:0x%08x\n",
					master->name, (master->size/(1024*1024)), rsvdblock_offset);
		
			/*
			* check the block number for the BBT/BBM reserved block pool...
			* reserved block number for NAND flash should be 2% of the total blocks,
			* plus: 
			*	add extra 1 NFI, 2 BBT and 2 BBMs blocks.
			* 
			* how to calculate BBMs:
			* 	- 2 bytes for each block, 
			* 	- bbmblocks = ((totalblock*2)+(blocksize-1))/blocksize;
			*/

			bbmblocks = ((master->size/master->erasesize)*2+(master->erasesize-1))/master->erasesize;
			rsvdblocks = ((master->size/master->erasesize)/100)*2+3+bbmblocks*2;

			if (rsvdblock_offset != (master->size - rsvdblocks*master->erasesize))
				panic("%s:0x%08x's rsv partition must start at offset:0x%08x\n", 
					master->name, master->size, master->size-(rsvdblocks*master->erasesize));
		}
#endif
	}

	return 0;
}

EXPORT_SYMBOL(add_mtd_partitions);
EXPORT_SYMBOL(del_mtd_partitions);

static DEFINE_SPINLOCK(part_parser_lock);
static LIST_HEAD(part_parsers);

static struct mtd_part_parser *get_partition_parser(const char *name)
{
	struct list_head *this;
	void *ret = NULL;
	spin_lock(&part_parser_lock);

	list_for_each(this, &part_parsers) {
		struct mtd_part_parser *p = list_entry(this, struct mtd_part_parser, list);

		if (!strcmp(p->name, name) && try_module_get(p->owner)) {
			ret = p;
			break;
		}
	}
	spin_unlock(&part_parser_lock);

	return ret;
}

int register_mtd_parser(struct mtd_part_parser *p)
{
	spin_lock(&part_parser_lock);
	list_add(&p->list, &part_parsers);
	spin_unlock(&part_parser_lock);

	return 0;
}

int deregister_mtd_parser(struct mtd_part_parser *p)
{
	spin_lock(&part_parser_lock);
	list_del(&p->list);
	spin_unlock(&part_parser_lock);
	return 0;
}

int parse_mtd_partitions(struct mtd_info *master, const char **types, 
			 struct mtd_partition **pparts, unsigned long origin)
{
	struct mtd_part_parser *parser;
	int ret = 0;
		
	for ( ; ret <= 0 && *types; types++) {
		parser = get_partition_parser(*types);
#ifdef CONFIG_KMOD
		if (!parser && !request_module("%s", *types))
				parser = get_partition_parser(*types);
#endif
		if (!parser) {
			printk(KERN_NOTICE "%s partition parsing not available\n",
			       *types);
			continue;
		}
		ret = (*parser->parse_fn)(master, pparts, origin);
		if (ret > 0) {
			printk(KERN_NOTICE "%d %s partitions found on MTD device %s\n", 
			       ret, parser->name, master->name);
		}
		put_partition_parser(parser);
	}
	return ret;
}

EXPORT_SYMBOL_GPL(parse_mtd_partitions);
EXPORT_SYMBOL_GPL(register_mtd_parser);
EXPORT_SYMBOL_GPL(deregister_mtd_parser);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicolas Pitre <nico@cam.org>");
MODULE_DESCRIPTION("Generic support for partitioning of MTD devices");

