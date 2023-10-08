#ifndef _LINUX_FS_NOTIFY_H
#define _LINUX_FS_NOTIFY_H

/*
 * include/linux/fs_notify.h - generic hooks for filesystem notification, to
 * reduce in-source duplication from both dnotify and inotify.
 *
 * We don't compile any of this away in some complicated menagerie of ifdefs.
 * Instead, we rely on the code inside to optimize away as needed.
 *
 * (C) Copyright 2005 Robert Love
 * Copyright (C) 2006, 2007 Motorola, Inc.
 *
 * Revision History:
 *
 * Date         Author          Comment
 * ===========  ==============  ==============================================
 * 07-Mar-2007  Motorola        Add IN_ISDIR functionality after refer to linux kernel source code v2.6.14.
 *                              For details see: 
 *                              http://www.linux-m32r.org/lxr/http/source/include/linux/fsnotify.h?v=2.6.14
 * 27-Mar-2007  Motorola        Add the fifth parameter is_dir for function fsnotify_move.                       
 */

#ifdef __KERNEL__

#include <linux/dnotify.h>
#include <linux/inotify.h>

/*
 * fsnotify_move - file old_name at old_dir was moved to new_name at new_dir
 * 
 * Add one parameter for fucntion fsnotify_move to make the events IN_MOVED_FROM/IN_MOVED_TO independent
 * with event IN_ISDIR.
 */
static inline void fsnotify_move(struct inode *old_dir, struct inode *new_dir,
				const char *old_name, const char *new_name, int is_dir)
{
	u32 cookie;
	u32 isdir = 0;

	if (old_dir == new_dir)
		inode_dir_notify(old_dir, DN_RENAME);
	else {
		inode_dir_notify(old_dir, DN_DELETE);
		inode_dir_notify(new_dir, DN_CREATE);
	}

	cookie = inotify_get_cookie();
	if (is_dir)
		is_dir = IN_ISDIR;

	inotify_inode_queue_event(old_dir, IN_MOVED_FROM | is_dir, cookie, old_name);
	inotify_inode_queue_event(new_dir, IN_MOVED_TO | is_dir, cookie, new_name);
}

/*
 * fsnotify_unlink - file was unlinked
 */
static inline void fsnotify_unlink(struct inode *inode, struct inode *dir,
				   struct dentry *dentry)
{
	inode_dir_notify(dir, DN_DELETE);
	inotify_inode_queue_event(dir, IN_DELETE_FILE, 0, dentry->d_name.name);
	inotify_inode_queue_event(inode, IN_DELETE_SELF, 0, NULL);

	inotify_inode_is_dead(inode);
	d_delete(dentry);
}

/*
 * fsnotify_rmdir - directory was removed
 */
static inline void fsnotify_rmdir(struct dentry *dentry, struct inode *inode,
				  struct inode *dir)
{
	inode_dir_notify(dir, DN_DELETE);
	inotify_inode_queue_event(dir, IN_DELETE_SUBDIR | IN_ISDIR, 0, dentry->d_name.name);
	inotify_inode_queue_event(inode, IN_DELETE_SELF | IN_ISDIR, 0, NULL);

	inotify_inode_is_dead(inode);
	d_delete(dentry);
}

/*
 * fsnotify_create - filename was linked in
 */
static inline void fsnotify_create(struct inode *inode, const char *filename)
{
	inode_dir_notify(inode, DN_CREATE);
	inotify_inode_queue_event(inode, IN_CREATE_FILE, 0, filename);
}

/*
 * fsnotify_mkdir - directory 'name' was created
 */
static inline void fsnotify_mkdir(struct inode *inode, const char *name)
{
	inode_dir_notify(inode, DN_CREATE);
	inotify_inode_queue_event(inode, IN_CREATE_SUBDIR | IN_ISDIR, 0, name);
}

/*
 * fsnotify_access - file was read
 */
static inline void fsnotify_access(struct dentry *dentry, struct inode *inode,
				   const char *filename)
{
	u32 mask = IN_ACCESS;

	if (S_ISDIR(inode->i_mode))
		mask |= IN_ISDIR;

	dnotify_parent(dentry, DN_ACCESS);
	inotify_dentry_parent_queue_event(dentry, mask, 0,
					  dentry->d_name.name);
	inotify_inode_queue_event(inode, mask, 0, NULL);
}

/*
 * fsnotify_modify - file was modified
 */
static inline void fsnotify_modify(struct dentry *dentry, struct inode *inode,
				   const char *filename)
{
	u32 mask = IN_MODIFY;

	if (S_ISDIR(inode->i_mode)) 
		mask |= IN_ISDIR;

	dnotify_parent(dentry, DN_MODIFY);
	inotify_dentry_parent_queue_event(dentry, mask, 0, filename);
	inotify_inode_queue_event(inode, mask, 0, NULL);
}

/*
 * fsnotify_open - file was opened
 */
static inline void fsnotify_open(struct dentry *dentry, struct inode *inode,
				 const char *filename)
{
	u32 mask = IN_OPEN;

	if (S_ISDIR(inode->i_mode)) 
		mask |= IN_ISDIR;	

	inotify_inode_queue_event(inode, mask, 0, NULL);
	inotify_dentry_parent_queue_event(dentry, mask, 0, filename);
}

/*
 * fsnotify_close - file was closed
 */
static inline void fsnotify_close(struct dentry *dentry, struct inode *inode,
				  mode_t mode, const char *filename)
{
	u32 mask;

	mask = (mode & FMODE_WRITE) ? IN_CLOSE_WRITE : IN_CLOSE_NOWRITE;

	if (S_ISDIR(inode->i_mode)) 
		mask |= IN_ISDIR;

	inotify_dentry_parent_queue_event(dentry, mask, 0, filename);
	inotify_inode_queue_event(inode, mask, 0, NULL);
}

/*
 * fsnotify_change - notify_change event.  file was modified and/or metadata
 * was changed.
 */
static inline void fsnotify_change(struct dentry *dentry, unsigned int ia_valid)
{
	struct inode *inode = dentry->d_inode; 
	int dn_mask = 0;
	u32 in_mask = 0;

	if (ia_valid & ATTR_UID) {
		in_mask |= IN_ATTRIB;
		dn_mask |= DN_ATTRIB;
	}
	if (ia_valid & ATTR_GID) {
		in_mask |= IN_ATTRIB;
		dn_mask |= DN_ATTRIB;
	}
	if (ia_valid & ATTR_SIZE) {
		in_mask |= IN_MODIFY;
		dn_mask |= DN_MODIFY;
	}
	/* both times implies a utime(s) call */
	if ((ia_valid & (ATTR_ATIME | ATTR_MTIME)) == (ATTR_ATIME | ATTR_MTIME))
	{
		in_mask |= IN_ATTRIB;
		dn_mask |= DN_ATTRIB;
	} else if (ia_valid & ATTR_ATIME) {
		in_mask |= IN_ACCESS;
		dn_mask |= DN_ACCESS;
	} else if (ia_valid & ATTR_MTIME) {
		in_mask |= IN_MODIFY;
		dn_mask |= DN_MODIFY;
	}
	if (ia_valid & ATTR_MODE) {
		in_mask |= IN_ATTRIB;
		dn_mask |= DN_ATTRIB;
	}

	if (dn_mask)
		dnotify_parent(dentry, dn_mask);
	if (in_mask) {
		if (S_ISDIR(inode->i_mode))
			in_mask |= IN_ISDIR;
		inotify_inode_queue_event(dentry->d_inode, in_mask, 0, NULL);
		inotify_dentry_parent_queue_event(dentry, in_mask, 0,
						  dentry->d_name.name);
	}
}

/*
 * fsnotify_sb_umount - filesystem unmount
 */
static inline void fsnotify_sb_umount(struct super_block *sb)
{
	inotify_super_block_umount(sb);
}

/*
 * fsnotify_flush - flush time!
 */
static inline void fsnotify_flush(struct file *filp, fl_owner_t id)
{
	dnotify_flush(filp, id);
}

#ifdef CONFIG_INOTIFY	/* inotify helpers */

/*
 * fsnotify_oldname_init - save off the old filename before we change it
 *
 * this could be kstrdup if only we could add that to lib/string.c
 */
static inline char *fsnotify_oldname_init(struct dentry *old_dentry)
{
	char *old_name;

	old_name = kmalloc(strlen(old_dentry->d_name.name) + 1, GFP_KERNEL);
	if (old_name)
		strcpy(old_name, old_dentry->d_name.name);
	return old_name;
}

/*
 * fsnotify_oldname_free - free the name we got from fsnotify_oldname_init
 */
static inline void fsnotify_oldname_free(const char *old_name)
{
	kfree(old_name);
}

#else	/* CONFIG_INOTIFY */

static inline char *fsnotify_oldname_init(struct dentry *old_dentry)
{
	return NULL;
}

static inline void fsnotify_oldname_free(const char *old_name)
{
}

#endif	/* ! CONFIG_INOTIFY */

#endif	/* __KERNEL__ */

#endif	/* _LINUX_FS_NOTIFY_H */
