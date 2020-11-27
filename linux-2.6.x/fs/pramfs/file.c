/*
 * FILE NAME fs/pramfs/file.c
 *
 * BRIEF DESCRIPTION
 *
 * File operations for files.
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
#include <linux/fs.h>
#include <linux/pram_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uio.h>
#include <asm/uaccess.h>

static int pram_open_file(struct inode * inode, struct file * filp)
{
	filp->f_flags |= O_DIRECT;
	return generic_file_open(inode, filp);
}

/*
 * Called when an inode is released. Note that this is different
 * from pram_open_file: open gets called at every open, but release
 * gets called only when /all/ the files are closed.
 */
static int pram_release_file (struct inode * inode, struct file * filp)
{
	return 0;
}

int pram_direct_IO(int rw, struct kiocb *iocb,
		   const struct iovec *iov,
		   loff_t offset, unsigned long nr_segs)
{
	struct file *file = iocb->ki_filp;
	struct inode *inode = file->f_mapping->host;
	struct super_block * sb = inode->i_sb;
	int progress = 0, retval = 0;
	struct pram_inode * pi;
	void * tmp = NULL;
	unsigned long blocknr, blockoff, flags;
	int num_blocks, blocksize_mask, blocksize, blocksize_bits;
	char __user *buf = iov->iov_base;
	size_t length = iov->iov_len;
	
	if (length < 0)
                return -EINVAL;
	if ((rw == READ) && (offset + length > inode->i_size))
		length = inode->i_size - offset;
	if (!length)
		goto out;

	blocksize_bits = inode->i_sb->s_blocksize_bits;
	blocksize = 1 << blocksize_bits;
	blocksize_mask = blocksize - 1;

	/* find starting block number to access */
	blocknr = offset >> blocksize_bits;
	/* find starting offset within starting block */
	blockoff = offset & blocksize_mask;
	/* find number of blocks to access */
	num_blocks = (blockoff + length + blocksize_mask) >> blocksize_bits;
		
	if (rw == WRITE) {
		// prepare a temporary buffer to hold a user data block
		// for writing.
		tmp = kmalloc(blocksize, GFP_KERNEL);
		if (!tmp)
			return -ENOMEM;
		/* now allocate the data blocks we'll need */
		retval = pram_alloc_blocks(inode, blocknr, num_blocks);
		if (retval)
			goto fail;
	}
	
	pi = pram_get_inode(inode->i_sb, inode->i_ino);
	
	while (length) {
		int count;
		pram_off_t block = pram_find_data_block(inode, blocknr++);
		u8* bp = (u8*)pram_get_block(sb, block);
		if (!bp)
			goto fail;
		
		count = blockoff + length > blocksize ?
			blocksize - blockoff : length;
		
		if (rw == READ) {
			copy_to_user(buf, &bp[blockoff], count);
		} else {
			copy_from_user(tmp, buf, count);

			pram_lock_block(inode->i_sb, bp);
			memcpy(&bp[blockoff], tmp, count);
			pram_unlock_block(inode->i_sb, bp);
		}

		progress += count;
		buf += count;
		length -= count;
		blockoff = 0;
	}
	
	retval = progress;
 fail:
	if (tmp)
		kfree(tmp);
 out:
	return retval;
}



struct file_operations pram_file_operations = {
	llseek:		generic_file_llseek,
	read:		generic_file_read,
	write:		generic_file_write,
	ioctl:		pram_ioctl,
	mmap:		generic_file_mmap,
	open:		pram_open_file,
	release:	pram_release_file,
	fsync:		pram_sync_file,
};

struct inode_operations pram_file_inode_operations = {
	truncate:	pram_truncate,
};
