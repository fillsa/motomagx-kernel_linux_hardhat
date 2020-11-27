/*
 * FILE NAME fs/pramfs/super.c
 *
 * BRIEF DESCRIPTION
 *
 * Super block operations.
 *
 * Author: Steve Longerbeam <stevel@mvista.com, or source@mvista.com>
 *
 * Copyright 2003 Sony Corporation
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 2003-2004 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/parser.h>
#include <linux/vfs.h>
#include <linux/pram_fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>

MODULE_AUTHOR("Steve Longerbeam, stevel@mvista.com");
MODULE_DESCRIPTION("Protected/Persistent RAM Filesystem");
MODULE_LICENSE("GPL");

static struct super_operations pram_sops;

#ifndef MODULE
extern struct list_head super_blocks;

struct super_block * find_pramfs_super(void)
{
	struct list_head *p;
	list_for_each(p, &super_blocks) {
		struct super_block * s = sb_entry(p);
		if (s->s_magic == PRAM_SUPER_MAGIC)
			return s;
	}
	return NULL;
}
EXPORT_SYMBOL(find_pramfs_super);
#endif

static void pram_set_blocksize(struct super_block * sb, unsigned long size)
{
	int bits;
	for (bits = 9, size >>= 9; size >>= 1; bits++)
		;
	if (bits > 12)
		bits = 12;
	sb->s_blocksize_bits = bits;
	sb->s_blocksize = (1<<bits);
}

static inline void * pram_ioremap(unsigned long phys_addr, size_t size)
{
	void * retval = ioremap(phys_addr, size);
#ifndef CONFIG_PRAMFS_NOWP
	if (retval) {
		spin_lock(&init_mm.page_table_lock);
		pram_writeable(retval, size, 0);
		spin_unlock(&init_mm.page_table_lock);
	}
#endif
	return retval;
}


static int pram_fill_super (struct super_block * sb, void * data, int silent)
{
	char *p;
	struct pram_super_block * super;
	struct pram_inode * root_i;
	struct pram_sb_info * sbi=NULL;
	pram_off_t root_offset;
	unsigned long flags;
	unsigned long maxsize, blocksize;
	int retval = -EINVAL;
	
	/*
	 * The physical location of the pram image is specified as
	 * a mount parameter.  This parameter is mandatory for obvious
	 * reasons.  Some validation is made on the phys address but this
	 * is not exhaustive and we count on the fact that someone using
	 * this feature is supposed to know what he/she's doing.
	 */
	if (!data || !(p = strstr((char *)data, "physaddr="))) {
		pram_err("unknown physical address for pramfs image\n");
		goto out;
	}

	sbi = kmalloc(sizeof(*sbi), GFP_KERNEL);
	if (!sbi)
		return -ENOMEM;
	sb->s_fs_info = sbi;
	memset(sbi, 0, sizeof(*sbi));

	sbi->phys_addr = simple_strtoul(p + 9, NULL, 0);
	if (sbi->phys_addr & (PAGE_SIZE-1)) {
		pram_err("physical address 0x%lx for pramfs isn't "
			  "aligned to a page boundary\n",
			  sbi->phys_addr);
		goto out;
	}

	if (sbi->phys_addr == 0) {
		pram_err("physical address for pramfs image can't be 0\n");
		goto out;
	}

	if ((p = strstr((char *)data, "init="))) {
		unsigned long bpi, num_inodes, bitmap_size;
		unsigned long num_blocks;
		pram_off_t bitmap_start;

		maxsize = simple_strtoul(p + 5, NULL, 0);
		pram_info("creating an empty pramfs of size %lu\n", maxsize);
		
		sbi->virt_addr = pram_ioremap(sbi->phys_addr, maxsize);
		if (!sbi->virt_addr) {
			pram_err("ioremap of the pramfs image failed\n");
			goto out;
		}
		
		if ((p = strstr((char *)data, "bs=")))
			blocksize = simple_strtoul(p + 3, NULL, 0);
		else
			blocksize = PRAM_DEF_BLOCK_SIZE;

		pram_set_blocksize(sb, blocksize);
		blocksize = sb->s_blocksize;
		
		if ((p = strstr((char *)data, "bpi=")))
			bpi = simple_strtoul(p + 4, NULL, 0);
		else {
			/* default is that 5% of the filesystem is
			   devoted to the inode table */
			bpi = 20 * PRAM_INODE_SIZE;
		}
		
		if ((p = strstr((char *)data, "N=")))
			num_inodes = simple_strtoul(p + 2, NULL, 0);
		else
			num_inodes = maxsize / bpi;

		/* up num_inodes such that the end of the inode table
		   (and start of bitmap) is on a block boundary */
		bitmap_start = PRAM_SB_SIZE + (num_inodes<<PRAM_INODE_BITS);
		if (bitmap_start & (blocksize - 1))
			bitmap_start = (bitmap_start + blocksize) &
				~(blocksize-1);
		num_inodes = (bitmap_start - PRAM_SB_SIZE) >> PRAM_INODE_BITS;
		
  		num_blocks = (maxsize - bitmap_start) >> sb->s_blocksize_bits;
		
		/* calc the data blocks in-use bitmap size in bytes */
		if (num_blocks & 7)
			bitmap_size = ((num_blocks + 8) & ~7) >> 3;
		else
			bitmap_size = num_blocks >> 3;
		/* round it up to the nearest blocksize boundary */
		if (bitmap_size & (blocksize - 1))
			bitmap_size = (bitmap_size + blocksize) &
				~(blocksize-1);
		
		pram_info("blocksize %lu, num inodes %lu, num blocks %lu\n",
			  blocksize, num_inodes, num_blocks);
		pram_dbg("bitmap start 0x%08x, bitmap size %lu\n",
			 (unsigned long)bitmap_start, bitmap_size);
		pram_dbg("max name length %d\n", PRAM_NAME_LEN);

		super = pram_get_super(sb);
		pram_lock_range(super, bitmap_start + bitmap_size);

		/* clear out super-block and inode table */
		memset(super, 0, bitmap_start);
		super->s_size = maxsize;
		super->s_blocksize = blocksize;
		super->s_inodes_count = num_inodes;
		super->s_blocks_count = num_blocks;
		super->s_free_inodes_count = num_inodes - 1;
		super->s_bitmap_blocks = bitmap_size >> sb->s_blocksize_bits;
		super->s_free_blocks_count =
			num_blocks - super->s_bitmap_blocks;
		super->s_free_inode_hint = 1;
		super->s_bitmap_start = bitmap_start;
		super->s_magic = PRAM_SUPER_MAGIC;
		pram_sync_super(super);

		root_i = pram_get_inode(sb, PRAM_ROOT_INO);
		root_i->i_mode = S_IFDIR | S_IRUGO | S_IXUGO | S_IWUSR;
		root_i->i_links_count = 2;
		root_i->i_d.d_parent = PRAM_ROOT_INO;
		pram_sync_inode(root_i);

		pram_init_bitmap(sb);

		pram_unlock_range(super, bitmap_start + bitmap_size);

		goto setup_sb;
	}

	pram_info("checking physical address 0x%lx for pramfs image\n",
		   sbi->phys_addr);
	
	/* Map only one page for now. Will remap it when fs size is known. */
	sbi->virt_addr = pram_ioremap(sbi->phys_addr, PAGE_SIZE);
	if (!sbi->virt_addr) {
		pram_err("ioremap of the pramfs image failed\n");
		goto out;
	}

	super = pram_get_super(sb);
	
	/* Do sanity checks on the superblock */
	if (super->s_magic != PRAM_SUPER_MAGIC) {
		pram_err("wrong magic\n");
		goto out;
	}
	/* Read the superblock */
	if (pram_calc_checksum((u32*)super, PRAM_SB_SIZE>>2)) {
		pram_err("checksum error in super block!\n");
		goto out;
	}

	/* get feature flags first */
	// FIXME: implement fs features?
#if 0
	if (super->s_features & ~PRAM_SUPPORTED_FLAGS) {
		pram_err("unsupported filesystem features\n");
		goto out;
	}
#endif
	
	blocksize = super->s_blocksize;
	pram_set_blocksize(sb, blocksize);
	
	maxsize = super->s_size;
	pram_info("pramfs image appears to be %lu KB in size\n", maxsize>>10);
	pram_info("blocksize %lu\n", blocksize);

	/* Read the root inode */
	root_i = pram_get_inode(sb, PRAM_ROOT_INO);
	
	if (pram_calc_checksum((u32*)root_i, PRAM_INODE_SIZE>>2)) {
		pram_err("checksum error in root inode!\n");
		goto out;
	}

	/* Check that the root inode is in a sane state */
	if (root_i->i_d.d_next) {
		pram_err("root->next not NULL??!!\n");
		goto out;
	}

	if (!S_ISDIR(root_i->i_mode)) {
		pram_err("root is not a directory!\n");
		goto out;
	}

	root_offset = root_i->i_type.dir.head;
	if (root_offset == 0)
		pram_info("empty filesystem\n");

	/* Remap the whole filesystem now */
	iounmap(sbi->virt_addr);
	sbi->virt_addr = pram_ioremap(sbi->phys_addr, maxsize);
	if (!sbi->virt_addr) {
		pram_err("ioremap of the pramfs image failed\n");
		goto out;
	}
	super = pram_get_super(sb);
	root_i = pram_get_inode(sb, PRAM_ROOT_INO);

	/* Set it all up.. */
 setup_sb:
	sbi->maxsize = maxsize;
	sb->s_magic = sbi->magic = super->s_magic;
	sbi->features = super->s_features;

	sb->s_op = &pram_sops;
	sb->s_root = d_alloc_root(pram_fill_new_inode(sb, root_i));

	retval = 0;
 out:
	if (retval && sbi->virt_addr)
		iounmap(sbi->virt_addr);

	return retval;
}

//static void pram_write_super (struct super_block * sb)
//{
//}

int pram_statfs (struct super_block * sb, struct kstatfs * buf)
{
	struct pram_super_block * ps = pram_get_super(sb);

	buf->f_type = PRAM_SUPER_MAGIC;
	buf->f_bsize = sb->s_blocksize;
	buf->f_blocks = ps->s_blocks_count;
	buf->f_bfree = buf->f_bavail = ps->s_free_blocks_count;
	buf->f_files = ps->s_inodes_count;
	buf->f_ffree = ps->s_free_inodes_count;
	buf->f_namelen = PRAM_NAME_LEN;
	return 0;
}

int pram_remount (struct super_block * sb, int * mntflags, char * data)
{
	struct pram_super_block * ps;
	unsigned long flags;
	
	if ((*mntflags & MS_RDONLY) != (sb->s_flags & MS_RDONLY)) {
		ps = pram_get_super(sb);
		pram_lock_super(ps);
		ps->s_mtime = get_seconds(); // update mount time
		pram_unlock_super(ps);
	}
	return 0;
}

void pram_put_super (struct super_block * sb)
{
	struct pram_sb_info * sbi = (struct pram_sb_info *)sb->s_fs_info;

	/* It's unmount time, so unmap the pramfs memory */
	if (sbi->virt_addr) {
		iounmap(sbi->virt_addr);
		sbi->virt_addr = NULL;
	}

	sb->s_fs_info = NULL;
	kfree(sbi);
}

/*
 * the super block writes are all done "on the fly", so the
 * super block is never in a "dirty" state, so there's no need
 * for write_super.
 */
static struct super_operations pram_sops = {
	read_inode:	pram_read_inode,
	write_inode:	pram_write_inode,
	dirty_inode:    pram_dirty_inode,
	put_inode:	pram_put_inode,
	delete_inode:	pram_delete_inode,
	put_super:	pram_put_super,
	//write_super:	pram_write_super,
	statfs:		pram_statfs,
	remount_fs:	pram_remount,
};

static struct super_block *pram_get_sb(struct file_system_type *fs_type,
				       int flags, const char *dev_name,
				       void *data)
{
	return get_sb_single(fs_type, flags, data, pram_fill_super);
}

static struct file_system_type pram_fs_type = {
	.owner          = THIS_MODULE,
	.name           = "pramfs",
	.get_sb         = pram_get_sb,
	.kill_sb        = kill_anon_super,
};

static int __init init_pram_fs(void)
{
        return register_filesystem(&pram_fs_type);
}

static void __exit exit_pram_fs(void)
{
	unregister_filesystem(&pram_fs_type);
}

module_init(init_pram_fs)
module_exit(exit_pram_fs)
