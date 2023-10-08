/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright (C) 2007 Motorola Inc.
 * Copyright (C) 2001-2003 Red Hat, Inc.
 *
 * Created by David Woodhouse <dwmw2@infradead.org>
 *
 * This file is part of JFFS2, the Journalling Flash File System v2.
 *
 *       Copyright (C) 2001, 2002 Red Hat, Inc.
 *
 * JFFS2 is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 or (at your option) any later 
 * version.
 *
 * JFFS2 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with JFFS2; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * As a special exception, if other files instantiate templates or use
 * macros or inline functions from these files, or you compile these
 * files and link them with other works to produce a work based on these
 * files, these files do not by themselves cause the resulting work to be
 * covered by the GNU General Public License. However the source code for
 * these files must still be made available in accordance with section (3)
 * of the GNU General Public License.
 *
 * This exception does not invalidate any other reasons why a work based on
 * this file might be covered by the GNU General Public License.
 *
 * For information on obtaining alternative licences for JFFS2, see 
 * http://sources.redhat.com/jffs2/jffs2-licence.html
 *
 *
 * $Id: fs.c,v 1.55 2005/03/18 10:17:18 gleixner Exp $
 *
 */

/* ChangeLog:
 * (mm-dd-yyyy)  Author      Comment
 * 03-23-2007    Motorola    Give more accurate available space with statfs.
 * 06-25-2007    Motorola    Update EM regarding new Driver release.
 * 06-28-2007    Motorola    Modify compliance issues in hardhat source files.
 * 08-02-2007    Motorola    Add comments for oss compliance.
 * 08-07-2007	 Mororola    Fix a bug in jffs2 to prevent kernel panic.
 * 08-10-2007    Motorola    Add comments.
 * 10-18-2007    Motorola    Fix deadlock problem in JFFS2.
 * 10-29-2007   Motorola     Change inode blocks when update
 * 11-06-2007    Motorola    Upmerge from 6.1.  (Change inode blocks when update)
 * 11-13-2007    Motorola    Fix deadlock problem in JFFS2.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/mtd/mtd.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/vfs.h>
#include <linux/crc32.h>
#include "nodelist.h"

static int jffs2_flash_setup(struct jffs2_sb_info *c);

static int jffs2_do_setattr (struct inode *inode, struct iattr *iattr)
{
	struct jffs2_full_dnode *old_metadata, *new_metadata;
	struct jffs2_inode_info *f = JFFS2_INODE_INFO(inode);
	struct jffs2_sb_info *c = JFFS2_SB_INFO(inode->i_sb);
	struct jffs2_raw_inode *ri;
	unsigned short dev;
	unsigned char *mdata = NULL;
	int mdatalen = 0;
	unsigned int ivalid;
	uint32_t phys_ofs, alloclen;
	int ret;
	D1(printk(KERN_DEBUG "jffs2_setattr(): ino #%lu\n", inode->i_ino));
	ret = inode_change_ok(inode, iattr);
	if (ret) 
		return ret;

	/* Special cases - we don't want more than one data node
	   for these types on the medium at any time. So setattr
	   must read the original data associated with the node
	   (i.e. the device numbers or the target name) and write
	   it out again with the appropriate data attached */
	if (S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode)) {
		/* For these, we don't actually need to read the old node */
		dev = old_encode_dev(inode->i_rdev);
		mdata = (char *)&dev;
		mdatalen = sizeof(dev);
		D1(printk(KERN_DEBUG "jffs2_setattr(): Writing %d bytes of kdev_t\n", mdatalen));
	} else if (S_ISLNK(inode->i_mode)) {
		mdatalen = f->metadata->size;
		mdata = kmalloc(f->metadata->size, GFP_USER);
		if (!mdata)
			return -ENOMEM;
		ret = jffs2_read_dnode(c, f, f->metadata, mdata, 0, mdatalen);
		if (ret) {
			kfree(mdata);
			return ret;
		}
		D1(printk(KERN_DEBUG "jffs2_setattr(): Writing %d bytes of symlink target\n", mdatalen));
	}

	ri = jffs2_alloc_raw_inode();
	if (!ri) {
		if (S_ISLNK(inode->i_mode))
			kfree(mdata);
		return -ENOMEM;
	}
		
	ret = jffs2_reserve_space(c, sizeof(*ri) + mdatalen, &phys_ofs, &alloclen, ALLOC_NORMAL);
	if (ret) {
		jffs2_free_raw_inode(ri);
		if (S_ISLNK(inode->i_mode & S_IFMT))
			 kfree(mdata);
		return ret;
	}
	down(&f->sem);
	ivalid = iattr->ia_valid;
	
	ri->magic = cpu_to_je16(JFFS2_MAGIC_BITMASK);
	ri->nodetype = cpu_to_je16(JFFS2_NODETYPE_INODE);
	ri->totlen = cpu_to_je32(sizeof(*ri) + mdatalen);
	ri->hdr_crc = cpu_to_je32(crc32(0, ri, sizeof(struct jffs2_unknown_node)-4));

	ri->ino = cpu_to_je32(inode->i_ino);
	ri->version = cpu_to_je32(++f->highest_version);

	ri->uid = cpu_to_je16((ivalid & ATTR_UID)?iattr->ia_uid:inode->i_uid);
	ri->gid = cpu_to_je16((ivalid & ATTR_GID)?iattr->ia_gid:inode->i_gid);

	if (ivalid & ATTR_MODE)
		if (iattr->ia_mode & S_ISGID &&
		    !in_group_p(je16_to_cpu(ri->gid)) && !capable(CAP_FSETID))
			ri->mode = cpu_to_jemode(iattr->ia_mode & ~S_ISGID);
		else 
			ri->mode = cpu_to_jemode(iattr->ia_mode);
	else
		ri->mode = cpu_to_jemode(inode->i_mode);


	ri->isize = cpu_to_je32((ivalid & ATTR_SIZE)?iattr->ia_size:inode->i_size);
	ri->atime = cpu_to_je32(I_SEC((ivalid & ATTR_ATIME)?iattr->ia_atime:inode->i_atime));
	ri->mtime = cpu_to_je32(I_SEC((ivalid & ATTR_MTIME)?iattr->ia_mtime:inode->i_mtime));
	ri->ctime = cpu_to_je32(I_SEC((ivalid & ATTR_CTIME)?iattr->ia_ctime:inode->i_ctime));

	ri->offset = cpu_to_je32(0);
	ri->csize = ri->dsize = cpu_to_je32(mdatalen);
	ri->compr = JFFS2_COMPR_NONE;
	if (ivalid & ATTR_SIZE && inode->i_size < iattr->ia_size) {
		/* It's an extension. Make it a hole node */
		ri->compr = JFFS2_COMPR_ZERO;
		ri->dsize = cpu_to_je32(iattr->ia_size - inode->i_size);
		ri->offset = cpu_to_je32(inode->i_size);
	}
	ri->node_crc = cpu_to_je32(crc32(0, ri, sizeof(*ri)-8));
	if (mdatalen)
		ri->data_crc = cpu_to_je32(crc32(0, mdata, mdatalen));
	else
		ri->data_crc = cpu_to_je32(0);

	new_metadata = jffs2_write_dnode(c, f, ri, mdata, mdatalen, phys_ofs, ALLOC_NORMAL);
	if (S_ISLNK(inode->i_mode))
		kfree(mdata);
	
	if (IS_ERR(new_metadata)) {
		jffs2_complete_reservation(c);
		jffs2_free_raw_inode(ri);
		up(&f->sem);
		return PTR_ERR(new_metadata);
	}
	/* It worked. Update the inode */
	inode->i_atime = ITIME(je32_to_cpu(ri->atime));
	inode->i_ctime = ITIME(je32_to_cpu(ri->ctime));
	inode->i_mtime = ITIME(je32_to_cpu(ri->mtime));
	inode->i_mode = jemode_to_cpu(ri->mode);
	inode->i_uid = je16_to_cpu(ri->uid);
	inode->i_gid = je16_to_cpu(ri->gid);


	old_metadata = f->metadata;

	if (ivalid & ATTR_SIZE && inode->i_size > iattr->ia_size)
		jffs2_truncate_fraglist (c, &f->fragtree, iattr->ia_size);

	if (ivalid & ATTR_SIZE && inode->i_size < iattr->ia_size) {
		jffs2_add_full_dnode_to_inode(c, f, new_metadata);
		inode->i_size = iattr->ia_size;
		f->metadata = NULL;
	} else {
		f->metadata = new_metadata;
	}
	if (old_metadata) {
		jffs2_mark_node_obsolete(c, old_metadata->raw);
		jffs2_free_full_dnode(old_metadata);
	}
	jffs2_free_raw_inode(ri);

	up(&f->sem);
	jffs2_complete_reservation(c);

	/* We have to do the vmtruncate() without f->sem held, since
	   some pages may be locked and waiting for it in readpage(). 
	   We are protected from a simultaneous write() extending i_size
	   back past iattr->ia_size, because do_truncate() holds the
	   generic inode semaphore. */
	if (ivalid & ATTR_SIZE && inode->i_size > iattr->ia_size)
		vmtruncate(inode, iattr->ia_size);

        /* update inode blocks */
        inode->i_blocks = (inode->i_size + 511) >> 9;

	return 0;
}

int jffs2_setattr(struct dentry *dentry, struct iattr *iattr)
{
	return jffs2_do_setattr(dentry->d_inode, iattr);
}

int jffs2_statfs(struct super_block *sb, struct kstatfs *buf)
{
	struct jffs2_sb_info *c = JFFS2_SB_INFO(sb);
	unsigned long avail;

	buf->f_type = JFFS2_SUPER_MAGIC;
	buf->f_bsize = 1 << PAGE_SHIFT;
	buf->f_blocks = c->flash_size >> PAGE_SHIFT;
	buf->f_files = 0;
	buf->f_ffree = 0;
	buf->f_namelen = JFFS2_MAX_NAME_LEN;

	spin_lock(&c->erase_completion_lock);

	avail = c->dirty_size + c->free_size;
	if (avail > c->sector_size * c->resv_blocks_write)
		avail -= c->sector_size * c->resv_blocks_write;
	else
		avail = 0;

#ifdef CONFIG_MOT_FEAT_MTD_FS 
	/* It is possible to have the avail space greater than zero and not be able to 
     * write data due to differences in the algorithms to get available space 
     * between jffs2_statfs and jffs2_reserve_space function.
	 *
	 * It is really confusing to user that 'df' shows the available space,
	 * but when user tries to write, it fails. We are using the algorithm similar 
	 * to jffs2_reserve_space to give more accurate available space information
	 * to user.  The jffs2_reserve_space function has comments of this
	 * calculation.
	 */

	if (avail && (c->nr_free_blocks + c->nr_erasing_blocks < c->resv_blocks_write)) {

		unsigned long dirty, avail_space;

		dirty= c->dirty_size + c->erasing_size - (c->nr_erasing_blocks * c->sector_size) +
			   c->unchecked_size;

		if (dirty < c->nospc_dirty_size) {
			avail=0;
		}

		avail_space = c->free_size + c->dirty_size + c->erasing_size + c->unchecked_size;
		if ( avail &&
		   ((avail_space / c->sector_size) <= c->resv_blocks_write) ) {

			avail=0; 
		}
	}
#endif

	buf->f_bavail = buf->f_bfree = avail >> PAGE_SHIFT;

	D2(jffs2_dump_block_lists(c));

	spin_unlock(&c->erase_completion_lock);

	return 0;
}


void jffs2_clear_inode (struct inode *inode)
{
	/* We can forget about this inode for now - drop all 
	 *  the nodelists associated with it, etc.
	 */
	struct jffs2_sb_info *c = JFFS2_SB_INFO(inode->i_sb);
	struct jffs2_inode_info *f = JFFS2_INODE_INFO(inode);
	
	D1(printk(KERN_DEBUG "jffs2_clear_inode(): ino #%lu mode %o\n", inode->i_ino, inode->i_mode));

	jffs2_do_clear_inode(c, f);
}

void jffs2_read_inode (struct inode *inode)
{
	struct jffs2_inode_info *f;
	struct jffs2_sb_info *c;
	struct jffs2_raw_inode latest_node;
	int ret;

	D1(printk(KERN_DEBUG "jffs2_read_inode(): inode->i_ino == %lu\n", inode->i_ino));

	f = JFFS2_INODE_INFO(inode);
	c = JFFS2_SB_INFO(inode->i_sb);

	jffs2_init_inode_info(f);
	down(&f->sem);
			
	ret = jffs2_do_read_inode(c, f, inode->i_ino, &latest_node);

	if (ret) {
		make_bad_inode(inode);
		up(&f->sem);
		return;
	}
	inode->i_mode = jemode_to_cpu(latest_node.mode);
	inode->i_uid = je16_to_cpu(latest_node.uid);
	inode->i_gid = je16_to_cpu(latest_node.gid);
	inode->i_size = je32_to_cpu(latest_node.isize);
	inode->i_atime = ITIME(je32_to_cpu(latest_node.atime));
	inode->i_mtime = ITIME(je32_to_cpu(latest_node.mtime));
	inode->i_ctime = ITIME(je32_to_cpu(latest_node.ctime));

	inode->i_nlink = f->inocache->nlink;

	inode->i_blksize = PAGE_SIZE;
	inode->i_blocks = (inode->i_size + 511) >> 9;
	
	switch (inode->i_mode & S_IFMT) {
		jint16_t rdev;

	case S_IFLNK:
		inode->i_op = &jffs2_symlink_inode_operations;
		break;
		
	case S_IFDIR:
	{
		struct jffs2_full_dirent *fd;

		for (fd=f->dents; fd; fd = fd->next) {
			if (fd->type == DT_DIR && fd->ino)
				inode->i_nlink++;
		}
		/* and '..' */
		inode->i_nlink++;
		/* Root dir gets i_nlink 3 for some reason */
		if (inode->i_ino == 1)
			inode->i_nlink++;

		inode->i_op = &jffs2_dir_inode_operations;
		inode->i_fop = &jffs2_dir_operations;
		break;
	}
	case S_IFREG:
		inode->i_op = &jffs2_file_inode_operations;
		inode->i_fop = &jffs2_file_operations;
		inode->i_mapping->a_ops = &jffs2_file_address_operations;
		inode->i_mapping->nrpages = 0;
		break;

	case S_IFBLK:
	case S_IFCHR:
		/* Read the device numbers from the media */
		D1(printk(KERN_DEBUG "Reading device numbers from flash\n"));
		if (jffs2_read_dnode(c, f, f->metadata, (char *)&rdev, 0, sizeof(rdev)) < 0) {
			/* Eep */
			printk(KERN_NOTICE "Read device numbers for inode %lu failed\n", (unsigned long)inode->i_ino);
			up(&f->sem);
			jffs2_do_clear_inode(c, f);
			make_bad_inode(inode);
			return;
		}			

	case S_IFSOCK:
	case S_IFIFO:
		inode->i_op = &jffs2_file_inode_operations;
		init_special_inode(inode, inode->i_mode,
				   old_decode_dev((je16_to_cpu(rdev))));
		break;

	default:
		printk(KERN_WARNING "jffs2_read_inode(): Bogus imode %o for ino %lu\n", inode->i_mode, (unsigned long)inode->i_ino);
	}

	up(&f->sem);

	D1(printk(KERN_DEBUG "jffs2_read_inode() returning\n"));
}

void jffs2_dirty_inode(struct inode *inode)
{
	struct iattr iattr;

	if (!(inode->i_state & I_DIRTY_DATASYNC)) {
		D2(printk(KERN_DEBUG "jffs2_dirty_inode() not calling setattr() for ino #%lu\n", inode->i_ino));
		return;
	}

	D1(printk(KERN_DEBUG "jffs2_dirty_inode() calling setattr() for ino #%lu\n", inode->i_ino));

	iattr.ia_valid = ATTR_MODE|ATTR_UID|ATTR_GID|ATTR_ATIME|ATTR_MTIME|ATTR_CTIME;
	iattr.ia_mode = inode->i_mode;
	iattr.ia_uid = inode->i_uid;
	iattr.ia_gid = inode->i_gid;
	iattr.ia_atime = inode->i_atime;
	iattr.ia_mtime = inode->i_mtime;
	iattr.ia_ctime = inode->i_ctime;

	jffs2_do_setattr(inode, &iattr);
}

int jffs2_remount_fs (struct super_block *sb, int *flags, char *data)
{
	struct jffs2_sb_info *c = JFFS2_SB_INFO(sb);

	if (c->flags & JFFS2_SB_FLAG_RO && !(sb->s_flags & MS_RDONLY))
		return -EROFS;

	/* We stop if it was running, then restart if it needs to.
	   This also catches the case where it was stopped and this
	   is just a remount to restart it.
	   Flush the writebuffer, if neccecary, else we loose it */
	if (!(sb->s_flags & MS_RDONLY)) {
		jffs2_stop_garbage_collect_thread(c);
		down(&c->alloc_sem);
		jffs2_flush_wbuf_pad(c);
		up(&c->alloc_sem);
	}	

	if (!(*flags & MS_RDONLY))
		jffs2_start_garbage_collect_thread(c);
	
	*flags |= MS_NOATIME;

	return 0;
}

void jffs2_write_super (struct super_block *sb)
{
	struct jffs2_sb_info *c = JFFS2_SB_INFO(sb);
	sb->s_dirt = 0;

	if (sb->s_flags & MS_RDONLY)
		return;

	D1(printk(KERN_DEBUG "jffs2_write_super()\n"));
	jffs2_garbage_collect_trigger(c);
	jffs2_erase_pending_blocks(c, 0);
	jffs2_flush_wbuf_gc(c, 0);
}


/* jffs2_new_inode: allocate a new inode and inocache, add it to the hash,
   fill in the raw_inode while you're at it. */
struct inode *jffs2_new_inode (struct inode *dir_i, int mode, struct jffs2_raw_inode *ri)
{
	struct inode *inode;
	struct super_block *sb = dir_i->i_sb;
	struct jffs2_sb_info *c;
	struct jffs2_inode_info *f;
	int ret;

	D1(printk(KERN_DEBUG "jffs2_new_inode(): dir_i %ld, mode 0x%x\n", dir_i->i_ino, mode));

	c = JFFS2_SB_INFO(sb);
	
	inode = new_inode(sb);
	
	if (!inode)
		return ERR_PTR(-ENOMEM);

	f = JFFS2_INODE_INFO(inode);
	jffs2_init_inode_info(f);
	down(&f->sem);

	memset(ri, 0, sizeof(*ri));
	/* Set OS-specific defaults for new inodes */
	ri->uid = cpu_to_je16(current->fsuid);

	if (dir_i->i_mode & S_ISGID) {
		ri->gid = cpu_to_je16(dir_i->i_gid);
		if (S_ISDIR(mode))
			mode |= S_ISGID;
	} else {
		ri->gid = cpu_to_je16(current->fsgid);
	}
	ri->mode =  cpu_to_jemode(mode);
	ret = jffs2_do_new_inode (c, f, mode, ri);
	if (ret) {
		make_bad_inode(inode);
		iput(inode);
		return ERR_PTR(ret);
	}
	inode->i_nlink = 1;
	inode->i_ino = je32_to_cpu(ri->ino);
	inode->i_mode = jemode_to_cpu(ri->mode);
	inode->i_gid = je16_to_cpu(ri->gid);
	inode->i_uid = je16_to_cpu(ri->uid);
	inode->i_atime = inode->i_ctime = inode->i_mtime = CURRENT_TIME_SEC;
	ri->atime = ri->mtime = ri->ctime = cpu_to_je32(I_SEC(inode->i_mtime));

	inode->i_blksize = PAGE_SIZE;
	inode->i_blocks = 0;
	inode->i_size = 0;

	insert_inode_hash(inode);

	return inode;
}


int jffs2_do_fill_super(struct super_block *sb, void *data, int silent)
{
	struct jffs2_sb_info *c;
	struct inode *root_i;
	int ret;
	size_t blocks;

	c = JFFS2_SB_INFO(sb);

#ifndef CONFIG_JFFS2_FS_WRITEBUFFER
	if (c->mtd->type == MTD_NANDFLASH) {
		printk(KERN_ERR "jffs2: Cannot operate on NAND flash unless jffs2 NAND support is compiled in.\n");
		return -EINVAL;
	}
	if (c->mtd->type == MTD_DATAFLASH) {
		printk(KERN_ERR "jffs2: Cannot operate on DataFlash unless jffs2 DataFlash support is compiled in.\n");
		return -EINVAL;
	}
#endif

	c->flash_size = c->mtd->size;

	/* 
	 * Check, if we have to concatenate physical blocks to larger virtual blocks
	 * to reduce the memorysize for c->blocks. (kmalloc allows max. 128K allocation)
	 */
	c->sector_size = c->mtd->erasesize; 
	blocks = c->flash_size / c->sector_size;
	if (!(c->mtd->flags & MTD_NO_VIRTBLOCKS)) {
		while ((blocks * sizeof (struct jffs2_eraseblock)) > (128 * 1024)) {
			blocks >>= 1;
			c->sector_size <<= 1;
		}	
	}

	/*
	 * Size alignment check
	 */
	if ((c->sector_size * blocks) != c->flash_size) {
		c->flash_size = c->sector_size * blocks;		
		printk(KERN_INFO "jffs2: Flash size not aligned to erasesize, reducing to %dKiB\n",
			c->flash_size / 1024);
	}

	if (c->sector_size != c->mtd->erasesize)
		printk(KERN_INFO "jffs2: Erase block size too small (%dKiB). Using virtual blocks size (%dKiB) instead\n", 
			c->mtd->erasesize / 1024, c->sector_size / 1024);

	if (c->flash_size < 5*c->sector_size) {
		printk(KERN_ERR "jffs2: Too few erase blocks (%d)\n", c->flash_size / c->sector_size);
		return -EINVAL;
	}

	c->cleanmarker_size = sizeof(struct jffs2_unknown_node);
	/* Joern -- stick alignment for weird 8-byte-page flash here */

	/* NAND (or other bizarre) flash... do setup accordingly */
	ret = jffs2_flash_setup(c);
	if (ret)
		return ret;

	c->inocache_list = kmalloc(INOCACHE_HASHSIZE * sizeof(struct jffs2_inode_cache *), GFP_KERNEL);
	if (!c->inocache_list) {
		ret = -ENOMEM;
		goto out_wbuf;
	}
	memset(c->inocache_list, 0, INOCACHE_HASHSIZE * sizeof(struct jffs2_inode_cache *));

	if ((ret = jffs2_do_mount_fs(c)))
		goto out_inohash;

	ret = -EINVAL;

	D1(printk(KERN_DEBUG "jffs2_do_fill_super(): Getting root inode\n"));
	root_i = iget(sb, 1);
	if (is_bad_inode(root_i)) {
		D1(printk(KERN_WARNING "get root inode failed\n"));
		goto out_nodes;
	}

	D1(printk(KERN_DEBUG "jffs2_do_fill_super(): d_alloc_root()\n"));
	sb->s_root = d_alloc_root(root_i);
	if (!sb->s_root)
		goto out_root_i;

	sb->s_maxbytes = 0xFFFFFFFF;
	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = JFFS2_SUPER_MAGIC;
	if (!(sb->s_flags & MS_RDONLY))
		jffs2_start_garbage_collect_thread(c);

	return 0;

 out_root_i:
	iput(root_i);
 out_nodes:
	jffs2_free_ino_caches(c);
	jffs2_free_raw_node_refs(c);
	if (c->mtd->flags & MTD_NO_VIRTBLOCKS)
		vfree(c->blocks);
	else
		kfree(c->blocks);
 out_inohash:
	kfree(c->inocache_list);
 out_wbuf:
	jffs2_flash_cleanup(c);

	return ret;
}

void jffs2_gc_release_inode(struct jffs2_sb_info *c,
				   struct jffs2_inode_info *f)
{
	iput(OFNI_EDONI_2SFFJ(f));
}

struct jffs2_inode_info *jffs2_gc_fetch_inode(struct jffs2_sb_info *c,
						     int inum, int nlink)
{
	struct inode *inode;
	struct jffs2_inode_cache *ic;
	if (!nlink) {
		/* The inode has zero nlink but its nodes weren't yet marked
		   obsolete. This has to be because we're still waiting for 
		   the final (close() and) iput() to happen.

		   There's a possibility that the final iput() could have 
		   happened while we were contemplating. In order to ensure
		   that we don't cause a new read_inode() (which would fail)
		   for the inode in question, we use ilookup() in this case
		   instead of iget().

		   The nlink can't _become_ zero at this point because we're 
		   holding the alloc_sem, and jffs2_do_unlink() would also
		   need that while decrementing nlink on any inode.
		*/
		inode = ilookup(OFNI_BS_2SFFJ(c), inum);
		if (!inode) {
			D1(printk(KERN_DEBUG "ilookup() failed for ino #%u; inode is probably deleted.\n",
				  inum));

			spin_lock(&c->inocache_lock);
			ic = jffs2_get_ino_cache(c, inum);
			if (!ic) {
				D1(printk(KERN_DEBUG "Inode cache for ino #%u is gone.\n", inum));
				spin_unlock(&c->inocache_lock);
				return NULL;
			}
			if (ic->state != INO_STATE_CHECKEDABSENT) {
				/* Wait for progress. Don't just loop */
				D1(printk(KERN_DEBUG "Waiting for ino #%u in state %d\n",
					  ic->ino, ic->state));
				sleep_on_spinunlock(&c->inocache_wq, &c->inocache_lock);
			} else {
				spin_unlock(&c->inocache_lock);
			}

			return NULL;
		}
	} else {
		/* Inode has links to it still; they're not going away because
		   jffs2_do_unlink() would need the alloc_sem and we have it.
		   Just iget() it, and if read_inode() is necessary that's OK.
		*/
		inode = iget(OFNI_BS_2SFFJ(c), inum);
		if (!inode)
			return ERR_PTR(-ENOMEM);
	}
	if (is_bad_inode(inode)) {
		printk(KERN_NOTICE "Eep. read_inode() failed for ino #%u. nlink %d\n",
		       inum, nlink);
		/* NB. This will happen again. We need to do something appropriate here. */
		iput(inode);
		return ERR_PTR(-EIO);
	}

	return JFFS2_INODE_INFO(inode);
}

unsigned char *jffs2_gc_fetch_page(struct jffs2_sb_info *c, 
				   struct jffs2_inode_info *f, 
				   unsigned long offset,
				   unsigned long *priv)
{
	struct inode *inode = OFNI_EDONI_2SFFJ(f);
	struct page *pg;

	/* read_cache_page_async_trylock will return -EBUSY
	 * if it is not possible to lock the cache page. If we
	 * get -EBUSY, then avoid a deadlock between
	 * cache page locks and f->sem.
	 */
	pg = read_cache_page_async_trylock(inode->i_mapping,
					   offset >> PAGE_CACHE_SHIFT,
					   (void *)jffs2_do_readpage_unlock,
					   inode);
	if (IS_ERR(pg))
		return (void *)pg;
	
	*priv = (unsigned long)pg;
	return kmap(pg);
}

void jffs2_gc_release_page(struct jffs2_sb_info *c,
			   unsigned char *ptr,
			   unsigned long *priv)
{
	struct page *pg = (void *)*priv;

	kunmap(pg);
	page_cache_release(pg);
}

static int jffs2_flash_setup(struct jffs2_sb_info *c) {
	int ret = 0;
	
	if (jffs2_cleanmarker_oob(c)) {
		/* NAND flash... do setup accordingly */
		ret = jffs2_nand_flash_setup(c);
		if (ret)
			return ret;
	}

	/* add setups for other bizarre flashes here... */
	if (jffs2_nor_ecc(c)) {
		ret = jffs2_nor_ecc_flash_setup(c);
		if (ret)
			return ret;
	}
	
	/* and Dataflash */
	if (jffs2_dataflash(c)) {
		ret = jffs2_dataflash_setup(c);
		if (ret)
			return ret;
	}

	/* and Intel "Sibley" flash */
	if (jffs2_nor_wbuf_flash(c)) {
		ret = jffs2_nor_wbuf_flash_setup(c);
		if (ret)
			return ret;
	}

	return ret;
}

void jffs2_flash_cleanup(struct jffs2_sb_info *c) {

	if (jffs2_cleanmarker_oob(c)) {
		jffs2_nand_flash_cleanup(c);
	}

	/* add cleanups for other bizarre flashes here... */
	if (jffs2_nor_ecc(c)) {
		jffs2_nor_ecc_flash_cleanup(c);
	}
	
	/* and DataFlash */
	if (jffs2_dataflash(c)) {
		jffs2_dataflash_cleanup(c);
	}

	/* and Intel "Sibley" flash */
	if (jffs2_nor_wbuf_flash(c)) {
		jffs2_nor_wbuf_flash_cleanup(c);
	}
}
