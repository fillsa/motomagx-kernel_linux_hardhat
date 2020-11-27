/*
 *  linux/fs/fat/file.c
 *
 *  Copyright (C) 2007 Motorola Inc.
 *
 *  Written 1992,1993 by Werner Almesberger
 *
 *  regular file handling primitives for fat-based filesystems
 */



/* ChangeLog:
 * (mm-dd-yyyy)  Author    Comment
 * 11-15-2007    Motorola  Upmege from 6.1 (Added conditional fsync for loop device)
 */





#include <linux/time.h>
#include <linux/msdos_fs.h>
#include <linux/smp_lock.h>
#include <linux/buffer_head.h>
#ifdef CONFIG_MOT_FEAT_FAT_SYNC
#include <linux/loop.h>
#endif



static ssize_t fat_file_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *ppos);

int fat_file_fsync(struct file *filp, struct dentry *dentry, int datasync);


struct file_operations fat_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= generic_file_read,
	.write		= fat_file_write,
	.mmap		= generic_file_mmap,
	.fsync		= fat_file_fsync,
	.readv		= generic_file_readv,
	.writev		= generic_file_writev,
	.sendfile	= generic_file_sendfile,
};

struct inode_operations fat_file_inode_operations = {
	.truncate	= fat_truncate,
	.setattr	= fat_notify_change,
};

int fat_get_block(struct inode *inode, sector_t iblock,
		  struct buffer_head *bh_result, int create)
{
	struct super_block *sb = inode->i_sb;
	sector_t phys;
	int err;

	err = fat_bmap(inode, iblock, &phys);
	if (err)
		return err;
	if (phys) {
		map_bh(bh_result, sb, phys);
		return 0;
	}
	if (!create)
		return 0;
	if (iblock != MSDOS_I(inode)->mmu_private >> sb->s_blocksize_bits) {
		fat_fs_panic(sb, "corrupted file size (i_pos %lld, %lld)",
			     MSDOS_I(inode)->i_pos, MSDOS_I(inode)->mmu_private);
		return -EIO;
	}
	if (!((unsigned long)iblock & (MSDOS_SB(sb)->sec_per_clus - 1))) {
		int error;

		error = fat_add_cluster(inode);
		if (error < 0)
			return error;
	}
	MSDOS_I(inode)->mmu_private += sb->s_blocksize;
	err = fat_bmap(inode, iblock, &phys);
	if (err)
		return err;
	if (!phys)
		BUG();
	set_buffer_new(bh_result);
	map_bh(bh_result, sb, phys);
	return 0;
}

static ssize_t fat_file_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	struct inode *inode = filp->f_dentry->d_inode;
	int retval;

	retval = generic_file_write(filp, buf, count, ppos);
	if (retval > 0) {
		inode->i_mtime = inode->i_ctime = CURRENT_TIME;
		MSDOS_I(inode)->i_attrs |= ATTR_ARCH;
		mark_inode_dirty(inode);
	}
	return retval;
}

void fat_truncate(struct inode *inode)
{
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	const unsigned int cluster_size = sbi->cluster_size;
	int nr_clusters;

	/* 
	 * This protects against truncating a file bigger than it was then
	 * trying to write into the hole.
	 */
	if (MSDOS_I(inode)->mmu_private > inode->i_size)
		MSDOS_I(inode)->mmu_private = inode->i_size;

	nr_clusters = (inode->i_size + (cluster_size - 1)) >> sbi->cluster_bits;

	lock_kernel();
	fat_free(inode, nr_clusters);
	MSDOS_I(inode)->i_attrs |= ATTR_ARCH;
	unlock_kernel();
	inode->i_ctime = inode->i_mtime = CURRENT_TIME;
	mark_inode_dirty(inode);
}



int fat_file_fsync(struct file *filp, struct dentry *dentry, int datasync)
{
        struct inode * inode = dentry->d_inode;
        struct super_block * sb;
        int ret;
#ifdef CONFIG_MOT_FEAT_FAT_SYNC
        int err;
        struct loop_device *lo;
        struct file *file;
        struct address_space *mapping;
#endif
        /* sync the inode to buffers */
        write_inode_now(inode, 0);

        /* sync the superblock to buffers */
        sb = inode->i_sb;
        lock_super(sb);
        if (sb->s_op->write_super)
                sb->s_op->write_super(sb);
        unlock_super(sb);

        /* .. finally sync the buffers to disk */
        ret = sync_blockdev(sb->s_bdev);
        if (ret)
                goto out;

#ifdef CONFIG_MOT_FEAT_FAT_SYNC
        /* Check if the fat was mounted on a loop block device.
         * If so, we try to sync the block to flash immediately
         */
        if (MAJOR(sb->s_dev) == LOOP_MAJOR) {
                lo = sb->s_bdev->bd_disk->private_data;
                file = lo->lo_backing_file;
                mapping = file->f_mapping;

                ret = filemap_fdatawrite(mapping);
                down(&mapping->host->i_sem);
                if (file->f_op && file->f_op->fsync) {
                        err = file->f_op->fsync(file,file->f_dentry,1);
                        if (!ret)
                                ret = err;
                }
                up(&mapping->host->i_sem);
                err = filemap_fdatawait(mapping);
                if (!ret)
                        ret = err;
        }
#endif

out:
        return ret;
}

