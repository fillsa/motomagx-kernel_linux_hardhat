/*
 * drivers/char/inotify.c - inode-based file event notifications
 *
 * Authors:
 *	John McCutchan	<ttb@tentacle.dhs.org>
 *	Robert Love	<rml@novell.com>
 *
 * Copyright (C) 2005 John McCutchan
 * Copyright (C) 2006-2008 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Revision History:
 *
 * Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Initial revision.
 * 01-Dec-2006  Motorola        Changed inotify_super_block_umount list processing
 * 28-Mar-2007  Motorola        Change from list_for_each_entry to list_for_each_entry_safe 
 *                              in function inotify_super_block_umount.
 * 17-Oct-2007  Motorola        Add spinlock protection for inode 
 * 23-Oct-2007  Motorola        Add spinlock protection for inode 
 * 06-Dec-2007  Motorola        Adding inode checking during umounting
 * 19-Dec-2007  Motorola        Adding inode checking during umounting
 * 12-Jun-2008  Motorola	Make sure freed wd is not reused immediately in LJ7.4
 * 25-Jun-2008  Motorola        Make sure the freed wd is not used immediately
 * 30-Jun-2008	Motorola	Make sure the released wd is not used immediately
 * 11-Sep-2008  Motorola        Update the debug info.
 * 28-Oct-2008  Motorola        Update debug info
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/idr.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/writeback.h>
#include <linux/inotify.h>

#include <asm/ioctls.h>

static atomic_t inotify_cookie;
static kmem_cache_t *watch_cachep;
static kmem_cache_t *event_cachep;
static kmem_cache_t *inode_data_cachep;

static int sysfs_attrib_max_user_devices;
static int sysfs_attrib_max_user_watches;
static unsigned int sysfs_attrib_max_queued_events;

/*
 * struct inotify_device - represents an open instance of an inotify device
 *
 * For each inotify device, we need to keep track of the events queued on it,
 * a list of the inodes that we are watching, and so on.
 *
 * CAUTION!!! Original lock ordering has been obsoleted!
 *
 * It is impossible for function inotify_inode_queue_event to lock dev lock 
 * before locking inode->i_lock. 
 * 
 * Current lock ordering is:
 * inode_lock (used to safely walk inode_in_use list)
 * 	inode->i_lock (only needed for getting ref on inode_data)
 * 		dev->lock (protects dev)
 *
 * # This structure is protected by 'lock'.  Lock ordering:
 * #
 * # dev->lock (protects dev)
 * #	inode_lock (used to safely walk inode_in_use list)
 * #		inode->i_lock (only needed for getting ref on inode_data)
 */
struct inotify_device {
	wait_queue_head_t 	wait;
	struct idr		idr;
	struct list_head 	events;
	struct list_head 	watches;
	spinlock_t		lock;
	unsigned int		queue_size;
	unsigned int		event_count;
	unsigned int		max_events;
	struct user_struct	*user;
	u32			last_wd;	/* the last wd allocated */
};

struct inotify_watch {
	s32 			wd;	/* watch descriptor */
	u32			mask;	/* event mask for this watch */
	struct inode		*inode;	/* associated inode */
	struct inotify_device	*dev;	/* associated device */
	struct list_head	d_list;	/* entry in device's list */
	struct list_head	i_list; /* entry in inotify_data's list */
};

/*
 * A list of these is attached to each instance of the driver.  In read(), this
 * this list is walked and all events that can fit in the buffer are returned.
 */
struct inotify_kernel_event {
	struct inotify_event	event;
	struct list_head        list;
	char			*filename;
};

static ssize_t show_max_queued_events(struct class_device *class, char *buf)
{
	return sprintf(buf, "%d\n", sysfs_attrib_max_queued_events);
}

static ssize_t store_max_queued_events(struct class_device *class,
				       const char *buf, size_t count)
{
	unsigned int max;

	if (sscanf(buf, "%u", &max) > 0 && max > 0) {
		sysfs_attrib_max_queued_events = max;
		return strlen(buf);
	}
	return -EINVAL;
}

static ssize_t show_max_user_devices(struct class_device *class, char *buf)
{
	return sprintf(buf, "%d\n", sysfs_attrib_max_user_devices);
}

static ssize_t store_max_user_devices(struct class_device *class,
				      const char *buf, size_t count)
{
	int max;

	if (sscanf(buf, "%d", &max) > 0 && max > 0) {
		sysfs_attrib_max_user_devices = max;
		return strlen(buf);
	}
	return -EINVAL;
}

static ssize_t show_max_user_watches(struct class_device *class, char *buf)
{
	return sprintf(buf, "%d\n", sysfs_attrib_max_user_watches);
}

static ssize_t store_max_user_watches(struct class_device *class,
				      const char *buf, size_t count)
{
	int max;

	if (sscanf(buf, "%d", &max) > 0 && max > 0) {
		sysfs_attrib_max_user_watches = max;
		return strlen(buf);
	}
	return -EINVAL;
}

static CLASS_DEVICE_ATTR(max_queued_events, S_IRUGO | S_IWUSR,
	show_max_queued_events, store_max_queued_events);
static CLASS_DEVICE_ATTR(max_user_devices, S_IRUGO | S_IWUSR,
	show_max_user_devices, store_max_user_devices);
static CLASS_DEVICE_ATTR(max_user_watches, S_IRUGO | S_IWUSR,
	show_max_user_watches, store_max_user_watches);

static inline void __get_inode_data(struct inotify_inode_data *data)
{
	atomic_inc(&data->count);
}

/*
 * get_inode_data - pin an inotify_inode_data structure.  Returns the structure
 * if successful and NULL on failure, which can only occur if inotify_data is
 * not yet allocated.  The inode must be pinned prior to invocation.
 */
static inline struct inotify_inode_data * get_inode_data(struct inode *inode)
{
	struct inotify_inode_data *data;

	spin_lock(&inode->i_lock);
	data = inode->inotify_data;
	if (data)
		__get_inode_data(data);
	spin_unlock(&inode->i_lock);

	return data;
}

/*
 * put_inode_data - drop our reference on an inotify_inode_data and the
 * inode structure in which it lives.  If the reference count on inotify_data
 * reaches zero, free it.
 */
static inline void put_inode_data(struct inode *inode)
{
        //spin_lock(&inode->i_lock);
	if (atomic_dec_and_test(&inode->inotify_data->count)) {
		kmem_cache_free(inode_data_cachep, inode->inotify_data);
		inode->inotify_data = NULL;
	}
        //spin_unlock(&inode->i_lock);
}

/*
 * find_inode - resolve a user-given path to a specific inode and return a nd
 */
static int find_inode(const char __user *dirname, struct nameidata *nd)
{
	int error;

	error = __user_walk(dirname, LOOKUP_FOLLOW, nd);
	if (error)
		return error;

	/* you can only watch an inode if you have read permissions on it */
	return permission(nd->dentry->d_inode, MAY_READ, NULL);
}

static struct inotify_kernel_event * kernel_event(s32 wd, u32 mask, u32 cookie,
						  const char *filename)
{
	struct inotify_kernel_event *kevent;

	kevent = kmem_cache_alloc(event_cachep, GFP_ATOMIC);
	if (!kevent)
		return NULL;

	/* we hand this out to user-space, so zero it just in case */
	memset(&kevent->event, 0, sizeof(struct inotify_event));

	kevent->event.wd = wd;
	kevent->event.mask = mask;
	kevent->event.cookie = cookie;
	INIT_LIST_HEAD(&kevent->list);

	if (filename) {
		size_t len, rem, event_size = sizeof(struct inotify_event);

		/*
		 * We need to pad the filename so as to properly align an
		 * array of inotify_event structures.  Because the structure is
		 * small and the common case is a small filename, we just round
		 * up to the next multiple of the structure's sizeof.  This is
		 * simple and safe for all architectures.
		 */
		len = strlen(filename) + 1;
		rem = event_size - len;
		if (len > event_size) {
			rem = event_size - (len % event_size);
			if (len % event_size == 0)
				rem = 0;
		}
		len += rem;

		kevent->filename = kmalloc(len, GFP_ATOMIC);
		if (!kevent->filename) {
			kmem_cache_free(event_cachep, kevent);
			return NULL;
		}
		memset(kevent->filename, 0, len);
		strncpy(kevent->filename, filename, strlen(filename));
		kevent->event.len = len;
	} else {
		kevent->event.len = 0;
		kevent->filename = NULL;
	}

	return kevent;
}

#define list_to_inotify_kernel_event(pos)	\
		list_entry((pos), struct inotify_kernel_event, list)

#define inotify_dev_get_event(dev)		\
		(list_to_inotify_kernel_event(dev->events.next))

/*
 * inotify_dev_queue_event - add a new event to the given device
 *
 * Caller must hold dev->lock.
 */
static void inotify_dev_queue_event(struct inotify_device *dev,
				    struct inotify_watch *watch, u32 mask,
				    u32 cookie, const char *filename)
{
	struct inotify_kernel_event *kevent, *last;

	/* drop this event if it is a dupe of the previous */
	last = inotify_dev_get_event(dev);
	if (dev->event_count && last->event.mask == mask &&
			last->event.wd == watch->wd) {
		const char *lastname = last->filename;

		if (!filename && !lastname)
			return;
		if (filename && lastname && !strcmp(lastname, filename))
			return;
	}

	/*
	 * the queue has already overflowed and we have already sent the
	 * Q_OVERFLOW event
	 */
	if (dev->event_count > dev->max_events)
		return;

	/* the queue has just overflowed and we need to notify user space */
	if (dev->event_count == dev->max_events) {
		kevent = kernel_event(-1, IN_Q_OVERFLOW, cookie, NULL);
		goto add_event_to_queue;
	}

	kevent = kernel_event(watch->wd, mask, cookie, filename);

add_event_to_queue:
	if (!kevent)
		return;

	/* queue the event and wake up anyone waiting */
	dev->event_count++;
	dev->queue_size += sizeof(struct inotify_event) + kevent->event.len;
	list_add_tail(&kevent->list, &dev->events);
	wake_up_interruptible(&dev->wait);
}

static inline int inotify_dev_has_events(struct inotify_device *dev)
{
	return !list_empty(&dev->events);
}

/*
 * inotify_dev_event_dequeue - destroy an event on the given device
 *
 * Caller must hold dev->lock.
 */
static void inotify_dev_event_dequeue(struct inotify_device *dev)
{
	struct inotify_kernel_event *kevent;

	if (!inotify_dev_has_events(dev))
		return;

	kevent = inotify_dev_get_event(dev);
	list_del_init(&kevent->list);
	if (kevent->filename)
		kfree(kevent->filename);

	dev->event_count--;
	dev->queue_size -= sizeof(struct inotify_event) + kevent->event.len;	

	kmem_cache_free(event_cachep, kevent);
}

/*
 * inotify_dev_get_wd - returns the next WD for use by the given dev
 *
 * This function can sleep.
 */
static int inotify_dev_get_wd(struct inotify_device *dev,
			     struct inotify_watch *watch)
{
	int ret;

	if (atomic_read(&dev->user->inotify_watches) >=
			sysfs_attrib_max_user_watches)
		return -ENOSPC;

repeat:
	if (!idr_pre_get(&dev->idr, GFP_KERNEL))
		return -ENOSPC;
	spin_lock(&dev->lock);
	ret = idr_get_new_above(&dev->idr, watch, dev->last_wd + 1, &watch->wd);
	if (!ret)
	{
		dev->last_wd = watch->wd;
	}
	spin_unlock(&dev->lock);
	if (ret == -EAGAIN) /* more memory is required, try again */
		goto repeat;
	else if (ret)       /* the idr is full! */
		return -ENOSPC;

	atomic_inc(&dev->user->inotify_watches);

	return 0;
}

/*
 * inotify_dev_put_wd - release the given WD on the given device
 *
 * Caller must hold dev->lock.
 */
static int inotify_dev_put_wd(struct inotify_device *dev, s32 wd)
{
	if (!dev || wd < 0)
		return -1;

	atomic_dec(&dev->user->inotify_watches);
	idr_remove(&dev->idr, wd);

	return 0;
}

/*
 * create_watch - creates a watch on the given device.
 *
 * Grabs dev->lock, so the caller must not hold it.
 */
static struct inotify_watch *create_watch(struct inotify_device *dev,
					  u32 mask, struct inode *inode)
{
	struct inotify_watch *watch;

	watch = kmem_cache_alloc(watch_cachep, GFP_KERNEL);
	if (!watch)
		return NULL;

	watch->mask = mask;
	watch->inode = inode;
	watch->dev = dev;
	INIT_LIST_HEAD(&watch->d_list);
	INIT_LIST_HEAD(&watch->i_list);

	if (inotify_dev_get_wd(dev, watch)) {
		kmem_cache_free(watch_cachep, watch);
		return NULL;
	}

	return watch;
}

/*
 * delete_watch - removes the given 'watch' from the given 'dev'
 *
 * Caller must hold dev->lock.
 */
static void delete_watch(struct inotify_device *dev,
			 struct inotify_watch *watch)
{
	inotify_dev_put_wd(dev, watch->wd);
	kmem_cache_free(watch_cachep, watch);
}

/*
 * inotify_find_dev - find the watch associated with the given inode and dev
 *
 * Caller must hold dev->lock.
 * FIXME: Needs inotify_data->lock too.  Don't need dev->lock, just pin it.
 */
static struct inotify_watch *inode_find_dev(struct inode *inode,
					    struct inotify_device *dev)
{
	struct inotify_watch *watch;

	if (!inode->inotify_data)
		return NULL;

	list_for_each_entry(watch, &inode->inotify_data->watches, i_list) {
		if (watch->dev == dev)
			return watch;
	}

	return NULL;
}

/*
 * dev_find_wd - given a (dev,wd) pair, returns the matching inotify_watcher
 *
 * Returns the results of looking up (dev,wd) in the idr layer.  NULL is
 * returned on error.
 *
 * The caller must hold dev->lock.
 */
static inline struct inotify_watch *dev_find_wd(struct inotify_device *dev,
						u32 wd)
{
	return idr_find(&dev->idr, wd);
}

static int inotify_dev_is_watching_inode(struct inotify_device *dev,
					 struct inode *inode)
{
	struct inotify_watch *watch;

	list_for_each_entry(watch, &dev->watches, d_list) {
		if (watch->inode == inode)
			return 1;
	}

	return 0;
}

/*
 * inotify_dev_add_watcher - add the given watcher to the given device instance
 *
 * Caller must hold dev->lock.
 */
static int inotify_dev_add_watch(struct inotify_device *dev,
				 struct inotify_watch *watch)
{
	if (!dev || !watch)
		return -EINVAL;

	list_add(&watch->d_list, &dev->watches);
	return 0;
}

/*
 * inotify_dev_rm_watch - remove the given watch from the given device
 *
 * Caller must hold dev->lock because we call inotify_dev_queue_event().
 */
static int inotify_dev_rm_watch(struct inotify_device *dev,
				struct inotify_watch *watch)
{
	if (!watch)
		return -EINVAL;

	inotify_dev_queue_event(dev, watch, IN_IGNORED, 0, NULL);
	list_del_init(&watch->d_list);

	return 0;
}

/*
 * inode_add_watch - add a watch to the given inode
 *
 * Callers must hold dev->lock, because we call inode_find_dev().
 */
static int inode_add_watch(struct inode *inode, struct inotify_watch *watch)
{
	int ret;

	if (!inode || !watch)
		return -EINVAL;

	/* 
	 * i_lock couldn't be locked while dev->lock has been locked to avoid 
	 * spinlock deadlock. i_lock is locked before dev->lock locking.
	 */
	if (!inode->inotify_data) {
		/* inotify_data is not attached to the inode, so add it */
		inode->inotify_data = kmem_cache_alloc(inode_data_cachep,
						       GFP_ATOMIC);
		if (!inode->inotify_data) {
			ret = -ENOMEM;
			goto out_lock;
		}

		atomic_set(&inode->inotify_data->count, 0);
		INIT_LIST_HEAD(&inode->inotify_data->watches);
		spin_lock_init(&inode->inotify_data->lock);
	} else if (inode_find_dev(inode, watch->dev)) {
		/* a watch is already associated with this (inode,dev) pair */
		ret = -EINVAL;
		goto out_lock;
	}
	__get_inode_data(inode->inotify_data);

	list_add(&watch->i_list, &inode->inotify_data->watches);

	return 0;
out_lock:
	return ret;
}

static int inode_rm_watch(struct inode *inode,
			  struct inotify_watch *watch)
{
	if (!inode || !watch || !inode->inotify_data)
		return -EINVAL;

	list_del_init(&watch->i_list);

	/* clean up inode->inotify_data */
	put_inode_data(inode);	

	return 0;
}

/* Kernel API */

/*
 * inotify_inode_queue_event - queue an event with the given mask, cookie, and
 * filename to any watches associated with the given inode.
 *
 * inode must be pinned prior to calling.
 */
void inotify_inode_queue_event(struct inode *inode, u32 mask, u32 cookie,
			       const char *name)
{
	struct inotify_watch *watch;

	spin_lock(&inode->i_lock);

	if (!inode->inotify_data)
	{
		spin_unlock(&inode->i_lock);
		return;
	}

	list_for_each_entry(watch, &inode->inotify_data->watches, i_list) {
		if (watch->mask & mask) {
			struct inotify_device *dev = watch->dev;
			spin_lock(&dev->lock);
			inotify_dev_queue_event(dev, watch, mask, cookie, name);
			spin_unlock(&dev->lock);
		}
	}
	spin_unlock(&inode->i_lock);
}
EXPORT_SYMBOL_GPL(inotify_inode_queue_event);

void inotify_dentry_parent_queue_event(struct dentry *dentry, u32 mask,
				       u32 cookie, const char *filename)
{
	struct dentry *parent;
	struct inode *inode;

	spin_lock(&dentry->d_lock);
	parent = dentry->d_parent;
	inode = parent->d_inode;
	if (inode->inotify_data) {
		dget(parent);
		spin_unlock(&dentry->d_lock);
		inotify_inode_queue_event(inode, mask, cookie, filename);
		dput(parent);
	} else
		spin_unlock(&dentry->d_lock);
}
EXPORT_SYMBOL_GPL(inotify_dentry_parent_queue_event);

u32 inotify_get_cookie(void)
{
	atomic_inc(&inotify_cookie);
	return atomic_read(&inotify_cookie);
}
EXPORT_SYMBOL_GPL(inotify_get_cookie);

/*
 * Caller must hold dev->lock.
 */
static void __remove_watch(struct inotify_watch *watch,
			   struct inotify_device *dev)
{
	struct inode *inode;

	inode = watch->inode;

	inode_rm_watch(inode, watch);
	inotify_dev_rm_watch(dev, watch);
	delete_watch(dev, watch);

	iput(inode);
}

/*
 * destroy_watch - remove a watch from both the device and the inode.
 *
 * watch->inode must be pinned.  We drop a reference before returning.  Grabs
 * dev->lock.
 */
static void remove_watch(struct inotify_watch *watch)
{
	struct inotify_device *dev = watch->dev;
	struct inode *inode = watch->inode;

	if(!inode)
	{
		printk("%s:%d Invalidate watch.\n", __FILE__, __LINE__);
		return;
	}

	spin_lock(&inode->i_lock);
	spin_lock(&dev->lock);

	__remove_watch(watch, dev);

	spin_unlock(&dev->lock);
	spin_unlock(&inode->i_lock);
}

void inotify_super_block_umount(struct super_block *sb)
{
	struct inode *inode, *inext;
	/* inode_in_use, inode_unused, s_dirty, s_io */
	struct list_head *pinode_list[4];
	int i;

	/* Not all inotify inode is in inode_in_use list. */
	pinode_list[0] = &inode_in_use;
	pinode_list[1] = &inode_unused;
	pinode_list[2] = &sb->s_dirty;
	pinode_list[3] = &sb->s_io;

	spin_lock(&inode_lock);
	for(i = 0; i < 4; i++)
	{
		/*
		 * We hold the inode_lock, so the inodes are not going anywhere, and
		 * we grab a reference on inotify_data before walking its list of
		 * watches.
		 */
		list_for_each_entry_safe(inode, inext, pinode_list[i], i_list) {
			struct inotify_inode_data *inode_data;
			struct inotify_watch *watch;
			struct inotify_watch *swatch;

			if (inode->i_sb != sb)
				continue;

			inode_data = get_inode_data(inode);
			if (!inode_data)
				continue;
		
			spin_lock(&inode->i_lock);
			list_for_each_entry_safe(watch, swatch,&inode_data->watches, i_list) {
				struct inotify_device *dev = watch->dev;
				spin_lock(&dev->lock);
				inotify_dev_queue_event(dev, watch, IN_UNMOUNT, 0,
						NULL);
				__remove_watch(watch, dev);
				spin_unlock(&dev->lock);
			}
			put_inode_data(inode);
			spin_unlock(&inode->i_lock);
		}
	}

	spin_unlock(&inode_lock);
}
EXPORT_SYMBOL_GPL(inotify_super_block_umount);

/*
 * inotify_inode_is_dead - an inode has been deleted, cleanup any watches
 */
void inotify_inode_is_dead(struct inode *inode)
{
	struct inotify_watch *watch, *next;
	struct inotify_inode_data *data;

	data = get_inode_data(inode);
	if (!data)
		return;
	
	spin_lock(&inode->i_lock);
	list_for_each_entry_safe(watch, next, &data->watches, i_list)
		remove_watch(watch);
	put_inode_data(inode);
	spin_unlock(&inode->i_lock);
}
EXPORT_SYMBOL_GPL(inotify_inode_is_dead);

/* The driver interface is implemented below */

static unsigned int inotify_poll(struct file *file, poll_table *wait)
{
        struct inotify_device *dev;

        dev = file->private_data;

        poll_wait(file, &dev->wait, wait);

        if (inotify_dev_has_events(dev))
                return POLLIN | POLLRDNORM;

        return 0;
}

static ssize_t inotify_read(struct file *file, char __user *buf,
			    size_t count, loff_t *pos)
{
	size_t event_size;
	struct inotify_device *dev;
	char __user *start;
	DECLARE_WAITQUEUE(wait, current);

	start = buf;
	dev = file->private_data;

	/* We only hand out full inotify events */
	event_size = sizeof(struct inotify_event);
	if (count < event_size)
		return 0;

	while (1) {
		int has_events;

		spin_lock(&dev->lock);
		has_events = inotify_dev_has_events(dev);
		spin_unlock(&dev->lock);
		if (has_events)
			break;

		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (signal_pending(current))
			return -EINTR;

		add_wait_queue(&dev->wait, &wait);
		set_current_state(TASK_INTERRUPTIBLE);

		schedule();

		set_current_state(TASK_RUNNING);		
		remove_wait_queue(&dev->wait, &wait);
	}

	while (count >= event_size) {
		struct inotify_kernel_event *kevent;

		spin_lock(&dev->lock);
		if (!inotify_dev_has_events(dev)) {
			spin_unlock(&dev->lock);
			break;
		}
		kevent = inotify_dev_get_event(dev);
		spin_unlock(&dev->lock);		

		/* We can't send this event, not enough space in the buffer */
		if (event_size + kevent->event.len > count)
			break;

		/* Copy the entire event except the string to user space */
		if (copy_to_user(buf, &kevent->event, event_size)) 
			return -EFAULT;

		buf += event_size;
		count -= event_size;

		/* Copy the filename to user space */
		if (kevent->filename) {
			if (copy_to_user(buf, kevent->filename,
					 kevent->event.len))
				return -EFAULT;
			buf += kevent->event.len;
			count -= kevent->event.len;
		}

		spin_lock(&dev->lock);
		inotify_dev_event_dequeue(dev);
		spin_unlock(&dev->lock);
	}

	return buf - start;
}

static int inotify_open(struct inode *inode, struct file *file)
{
	struct inotify_device *dev;
	struct user_struct *user;
	int ret;

	user = get_uid(current->user);

	if (atomic_read(&user->inotify_devs) >= sysfs_attrib_max_user_devices) {
		ret = -EMFILE;
		goto out_err;
	}

	dev = kmalloc(sizeof(struct inotify_device), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		goto out_err;
	}

	atomic_inc(&current->user->inotify_devs);	

	idr_init(&dev->idr);

	INIT_LIST_HEAD(&dev->events);
	INIT_LIST_HEAD(&dev->watches);
	init_waitqueue_head(&dev->wait);

	dev->event_count = 0;
	dev->queue_size = 0;
	dev->max_events = sysfs_attrib_max_queued_events;
	dev->user = user;
	dev->last_wd = 0;
	spin_lock_init(&dev->lock);

	file->private_data = dev;

	return 0;
out_err:
	free_uid(current->user);
	return ret;
}

/*
 * inotify_release_all_watches - destroy all watches on a given device
 *
 * FIXME: We need a lock on the watch here.
 */
static void inotify_release_all_watches(struct inotify_device *dev)
{
	struct inotify_watch *watch, *next;

	list_for_each_entry_safe(watch, next, &dev->watches, d_list)
		remove_watch(watch);
}

/*
 * inotify_release_all_events - destroy all of the events on a given device
 */
static void inotify_release_all_events(struct inotify_device *dev)
{
	spin_lock(&dev->lock);
	while (inotify_dev_has_events(dev))
		inotify_dev_event_dequeue(dev);
	spin_unlock(&dev->lock);
}

static int inotify_release(struct inode *inode, struct file *file)
{
	struct inotify_device *dev;

	dev = file->private_data;

	inotify_release_all_watches(dev);
	inotify_release_all_events(dev);

	atomic_dec(&dev->user->inotify_devs);
	free_uid(dev->user);

	kfree(dev);

	return 0;
}

static int inotify_add_watch(struct inotify_device *dev,
			     struct inotify_watch_request *request)
{
	struct inode *inode;
	struct inotify_watch *watch;
	struct nameidata nd;
	int ret;

	ret = find_inode((const char __user*) request->name, &nd);
	if (ret)
		return ret;

	/* held in place by references in nd */
	inode = nd.dentry->d_inode;

	spin_lock(&dev->lock);

	/*
	 * This handles the case of re-adding a directory we are already
	 * watching, we just update the mask and return 0
	 */
	if (inotify_dev_is_watching_inode(dev, inode)) {
		struct inotify_watch *owatch;	/* the old watch */

		owatch = inode_find_dev(inode, dev);
		owatch->mask = request->mask;
		spin_unlock(&dev->lock);
		path_release(&nd);

		return owatch->wd;
	}

	spin_unlock(&dev->lock);

	watch = create_watch(dev, request->mask, inode);
	if (!watch) {
		path_release(&nd);
		return -ENOSPC;
	}

	spin_lock(&inode->i_lock);
	spin_lock(&dev->lock);

	/* We can't add anymore watches to this device */
	if (inotify_dev_add_watch(dev, watch)) {
		delete_watch(dev, watch);
		spin_unlock(&dev->lock);
		spin_unlock(&inode->i_lock);
		path_release(&nd);
		return -EINVAL;
	}

	ret = inode_add_watch(inode, watch);
	if (ret < 0) {
		list_del_init(&watch->d_list);
		delete_watch(dev, watch);
		spin_unlock(&dev->lock);
		spin_unlock(&inode->i_lock);
		path_release(&nd);
		return ret;
	}

	spin_unlock(&dev->lock);
	spin_unlock(&inode->i_lock);

	/*
	 * Demote the reference to nameidata to a reference to the inode held
	 * by the watch.
	 */
	spin_lock(&inode_lock);
	__iget(inode);
	spin_unlock(&inode_lock);
	path_release(&nd);

	return watch->wd;
}

static int inotify_ignore(struct inotify_device *dev, s32 wd)
{
	struct inotify_watch *watch;
	struct inode * inode;
	int ret = 0;

	spin_lock(&dev->lock);
	watch = dev_find_wd(dev, wd);
	spin_unlock(&dev->lock);
	if (!watch) {
		ret = -EINVAL;
		goto out;
	}

	if((inode = watch->inode) == NULL){
		ret = -EINVAL;
		goto out;
	}

	spin_lock(&inode->i_lock);
	spin_lock(&dev->lock);

	__remove_watch(watch, dev);

	spin_unlock(&dev->lock);
	spin_unlock(&inode->i_lock);

out:
	return ret;
}

/*
 * inotify_ioctl() - our device file's ioctl method
 *
 * The VFS serializes all of our calls via the BKL and we rely on that.  We
 * could, alternatively, grab dev->lock.  Right now lower levels grab that
 * where needed.
 */
static int inotify_ioctl(struct inode *ip, struct file *fp,
			 unsigned int cmd, unsigned long arg)
{
	struct inotify_device *dev;
	struct inotify_watch_request request;
	void __user *p;
	s32 wd;

	dev = fp->private_data;
	p = (void __user *) arg;

	switch (cmd) {
	case INOTIFY_WATCH:
		if (copy_from_user(&request, p, sizeof (request)))
			return -EFAULT;
		return inotify_add_watch(dev, &request);
	case INOTIFY_IGNORE:
		if (copy_from_user(&wd, p, sizeof (wd)))
			return -EFAULT;
		return inotify_ignore(dev, wd);
	case FIONREAD:
		return put_user(dev->queue_size, (int __user *) p);
	default:
		return -ENOTTY;
	}
}

static struct file_operations inotify_fops = {
	.owner		= THIS_MODULE,
	.poll		= inotify_poll,
	.read		= inotify_read,
	.open		= inotify_open,
	.release	= inotify_release,
	.ioctl		= inotify_ioctl,
};

static struct miscdevice inotify_device = {
	.minor  = MISC_DYNAMIC_MINOR,
	.name	= "inotify",
	.fops	= &inotify_fops,
	.devfs_name	= "inotify", /* Added for our devfs */
};

static int __init inotify_init(void)
{
	struct class_device *class;
	int ret;

	ret = misc_register(&inotify_device);
	if (ret)
		return ret;

	sysfs_attrib_max_queued_events = 512;
	sysfs_attrib_max_user_devices = 64;
	sysfs_attrib_max_user_watches = 16384;

	class = inotify_device.class;
	class_device_create_file(class, &class_device_attr_max_queued_events);
	class_device_create_file(class, &class_device_attr_max_user_devices);
	class_device_create_file(class, &class_device_attr_max_user_watches);

	atomic_set(&inotify_cookie, 0);

	watch_cachep = kmem_cache_create("inotify_watch_cache",
			sizeof(struct inotify_watch), 0, SLAB_PANIC,
			NULL, NULL);

	event_cachep = kmem_cache_create("inotify_event_cache",
			sizeof(struct inotify_kernel_event), 0,
			SLAB_PANIC, NULL, NULL);

	inode_data_cachep = kmem_cache_create("inotify_inode_data_cache",
			sizeof(struct inotify_inode_data), 0, SLAB_PANIC,
			NULL, NULL);

	printk(KERN_INFO "inotify device minor=%d\n", inotify_device.minor);

	return 0;
}

module_init(inotify_init);
