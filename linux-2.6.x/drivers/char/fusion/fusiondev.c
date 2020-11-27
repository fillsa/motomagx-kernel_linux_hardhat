/*
 *	Fusion Kernel Module
 *
 *	(c) Copyright 2002-2003  Convergence GmbH
 *
 *      Written by Denis Oliver Kropp <dok@directfb.org>
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <asm/uaccess.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 2)
#include <linux/device.h>
#endif

#include <linux/fusion.h>

#include "call.h"
#include "fusiondev.h"
#include "fusionee.h"
#include "property.h"
#include "reactor.h"
#include "ref.h"
#include "skirmish.h"

#if 0
#define DEBUG(x...)  printk (KERN_DEBUG "Fusion: " x)
#else
#define DEBUG(x...)  do {} while (0)
#endif

#ifndef FUSION_MAJOR 
static unsigned int fusion_major = 0;
#else
static unsigned int fusion_major = FUSION_MAJOR;
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Denis Oliver Kropp <dok@directfb.org>");

struct proc_dir_entry *proc_fusion_dir;

#define NUM_MINORS 8

static FusionDev  *fusion_devs[NUM_MINORS] = { 0 };
static DECLARE_MUTEX(devs_lock);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
static devfs_handle_t devfs_handles[NUM_MINORS];
static inline unsigned iminor(struct inode *inode)
{
        return MINOR(inode->i_rdev);
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 2)
static struct class_simple *fusion_class;
#endif

/******************************************************************************/

void
fusion_sleep_on(wait_queue_head_t *q, struct semaphore *lock, signed long *timeout)
{
     wait_queue_t wait;

     init_waitqueue_entry (&wait, current);

     current->state = TASK_INTERRUPTIBLE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
     spin_lock (&q->lock);
     __add_wait_queue (q, &wait);
     spin_unlock (&q->lock);

     up (lock);

     if (timeout)
          *timeout = schedule_timeout(*timeout);
     else
          schedule();

     spin_lock (&q->lock);
     __remove_wait_queue (q, &wait);
     spin_unlock (&q->lock);
#else
     write_lock (&q->lock);
     __add_wait_queue (q, &wait);
     write_unlock (&q->lock);

     up (lock);

     if (timeout)
          *timeout = schedule_timeout(*timeout);
     else
          schedule();

     write_lock (&q->lock);
     __remove_wait_queue (q, &wait);
     write_unlock (&q->lock);
#endif
}

/******************************************************************************/

static int
fusiondev_stat_read_proc(char *buf, char **start, off_t offset,
                         int len, int *eof, void *private)
{
     FusionDev *dev     = private;
     int        written = 0;

     written += snprintf( buf, len,
                          "lease/purchase   cede      attach     detach      "
                          "ref up   ref down  prevail/swoop dismiss\n" );
     if (written < offset) {
          offset -= written;
          written = 0;
     }

     if (written < len) {
          written += snprintf( buf+written, len - written,
                               "%10d %10d  %10d %10d  %10d %10d  %10d %10d\n",
                               dev->stat.property_lease_purchase,
                               dev->stat.property_cede,
                               dev->stat.reactor_attach,
                               dev->stat.reactor_detach,
                               dev->stat.ref_up,
                               dev->stat.ref_down,
                               dev->stat.skirmish_prevail_swoop,
                               dev->stat.skirmish_dismiss );
          if (written < offset) {
               offset -= written;
               written = 0;
          }
     }

     *start = buf + offset;
     written -= offset;
     if (written > len) {
          *eof = 0;
          return len;
     }

     *eof = 1;
     return(written<0) ? 0 : written;
}

/******************************************************************************/

static int
fusiondev_init (FusionDev *dev)
{
     int ret;

     ret = fusionee_init (dev);
     if (ret)
          goto error_fusionee;

     ret = fusion_ref_init (dev);
     if (ret)
          goto error_ref;

     ret = fusion_skirmish_init (dev);
     if (ret)
          goto error_skirmish;

     ret = fusion_property_init (dev);
     if (ret)
          goto error_property;

     ret = fusion_reactor_init (dev);
     if (ret)
          goto error_reactor;

     ret = fusion_call_init (dev);
     if (ret)
          goto error_call;

     create_proc_read_entry("stat", 0, dev->proc_dir,
                            fusiondev_stat_read_proc, dev);

     return 0;


error_call:
     fusion_reactor_deinit (dev);

error_reactor:
     fusion_property_deinit (dev);

error_property:
     fusion_skirmish_deinit (dev);

error_skirmish:
     fusion_ref_deinit (dev);

error_ref:
     fusionee_deinit (dev);

error_fusionee:
     return ret;
}

static void
fusiondev_deinit (FusionDev *dev)
{
     remove_proc_entry ("stat", dev->proc_dir);

     fusion_call_deinit (dev);
     fusion_reactor_deinit (dev);
     fusion_property_deinit (dev);
     fusion_skirmish_deinit (dev);
     fusion_ref_deinit (dev);
     fusionee_deinit (dev);
}

/******************************************************************************/

static int
fusion_open (struct inode *inode, struct file *file)
{
     int ret;
     int fusion_id;
     int minor = iminor(inode);

     DEBUG( "fusion_open\n" );

     if (down_interruptible (&devs_lock))
          return -EINTR;

     if (!fusion_devs[minor]) {
          char buf[4];

          fusion_devs[minor] = kmalloc (sizeof(FusionDev), GFP_KERNEL);
          if (!fusion_devs[minor]) {
               up (&devs_lock);
               return -ENOMEM;
          }

          memset (fusion_devs[minor], 0, sizeof(FusionDev));

          snprintf (buf, 4, "%d", minor);

          fusion_devs[minor]->proc_dir = proc_mkdir (buf, proc_fusion_dir);

          ret = fusiondev_init (fusion_devs[minor]);
          if (ret) {
               remove_proc_entry (buf, proc_fusion_dir);

               kfree (fusion_devs[minor]);
               fusion_devs[minor] = NULL;

               up (&devs_lock);

               return ret;
          }
     }
     else if (file->f_flags & O_EXCL) {
          up (&devs_lock);
          return -EBUSY;
     }

     ret = fusionee_new (fusion_devs[minor], &fusion_id);
     if (ret) {
          if (!fusion_devs[minor]->refs) {
               fusiondev_deinit (fusion_devs[minor]);

               remove_proc_entry (fusion_devs[minor]->proc_dir->name,
                                  proc_fusion_dir);

               kfree (fusion_devs[minor]);
               fusion_devs[minor] = NULL;
          }

          up (&devs_lock);

          return ret;
     }

     fusion_devs[minor]->refs++;

     up (&devs_lock);


     file->private_data = (void*) fusion_id;

     return 0;
}

static int
fusion_release (struct inode *inode, struct file *file)
{
     int ret;
     int minor     = iminor(inode);
     int fusion_id = (int) file->private_data;

     DEBUG( "fusion_release\n" );

     ret = fusionee_destroy (fusion_devs[minor], fusion_id);
     if (ret)
          return ret;

     down (&devs_lock);

     if (! --fusion_devs[minor]->refs) {
          fusiondev_deinit (fusion_devs[minor]);

          remove_proc_entry (fusion_devs[minor]->proc_dir->name,
                             proc_fusion_dir);

          kfree (fusion_devs[minor]);
          fusion_devs[minor] = NULL;
     }

     up (&devs_lock);

     return 0;
}

static int
fusion_flush (struct file *file)
{
     int        fusion_id = (int) file->private_data;
     FusionDev *dev       = fusion_devs[iminor(file->f_dentry->d_inode)];

     (void) fusion_id;

     DEBUG( "fusion_flush (0x%08x %d)\n", fusion_id, current->pid );

     if (current->flags & PF_EXITING)
          fusion_skirmish_dismiss_all_from_pid (dev, current->pid);

     return 0;
}

static ssize_t
fusion_read (struct file *file, char *buf, size_t count, loff_t *ppos)
{
     int        fusion_id = (int) file->private_data;
     FusionDev *dev       = fusion_devs[iminor(file->f_dentry->d_inode)];

     DEBUG( "fusion_read (%d)\n", count );

     return fusionee_get_messages (dev, fusion_id, buf, count,
                                   !(file->f_flags & O_NONBLOCK));
}

static unsigned int
fusion_poll (struct file *file, poll_table * wait)
{
     int        fusion_id = (int) file->private_data;
     FusionDev *dev       = fusion_devs[iminor(file->f_dentry->d_inode)];

     DEBUG( "fusion_poll\n" );

     return fusionee_poll (dev, fusion_id, file, wait);
}

static int
lounge_ioctl (FusionDev *dev, int fusion_id,
              unsigned int cmd, unsigned long arg)
{
     int             ret;
     FusionEnter     enter;
     FusionKill      kill;
     FusionEntryInfo info;

     switch (_IOC_NR(cmd)) {
          case _IOC_NR(FUSION_ENTER):
               if (copy_from_user (&enter, (FusionEnter*) arg, sizeof(enter)))
                    return -EFAULT;

               if (enter.api.major != FUSION_API_MAJOR || enter.api.minor > FUSION_API_MINOR)
                    return -ENOPROTOOPT;

               enter.fusion_id = fusion_id;

               if (copy_to_user ((FusionEnter*) arg, &enter, sizeof(enter)))
                    return -EFAULT;

               return 0;

          case _IOC_NR(FUSION_KILL):
               if (copy_from_user (&kill, (FusionKill*) arg, sizeof(kill)))
                    return -EFAULT;

               return fusionee_kill (dev, fusion_id,
                                     kill.fusion_id, kill.signal, kill.timeout_ms);

          case _IOC_NR(FUSION_ENTRY_SET_INFO):
               if (copy_from_user (&info, (FusionEntryInfo*) arg, sizeof(info)))
                    return -EFAULT;

               switch (info.type) {
                    case FT_SKIRMISH:
                         return fusion_entry_set_info (&dev->skirmish, &info);

                    case FT_PROPERTY:
                         return fusion_entry_set_info (&dev->properties, &info);

                    case FT_REACTOR:
                         return fusion_entry_set_info (&dev->reactor, &info);

                    default:
                         return -ENOSYS;
               }

          case _IOC_NR(FUSION_ENTRY_GET_INFO):
               if (copy_from_user (&info, (FusionEntryInfo*) arg, sizeof(info)))
                    return -EFAULT;

               switch (info.type) {
                    case FT_SKIRMISH:
                         ret = fusion_entry_get_info (&dev->skirmish, &info);
                         break;

                    case FT_PROPERTY:
                         ret = fusion_entry_get_info (&dev->properties, &info);
                         break;

                    case FT_REACTOR:
                         ret = fusion_entry_get_info (&dev->reactor, &info);
                         break;

                    default:
                         return -ENOSYS;
               }

               if (ret)
                    return ret;

               if (copy_to_user ((FusionEntryInfo*) arg, &info, sizeof(info)))
                    return -EFAULT;

               return 0;
     }

     return -ENOSYS;
}

static int
messaging_ioctl (FusionDev *dev, int fusion_id,
                 unsigned int cmd, unsigned long arg)
{
     FusionSendMessage send;

     switch (_IOC_NR(cmd)) {
          case _IOC_NR(FUSION_SEND_MESSAGE):
               if (copy_from_user (&send, (FusionSendMessage*) arg, sizeof(send)))
                    return -EFAULT;

               if (send.msg_size <= 0)
                    return -EINVAL;

               /* message data > 64k should be stored in shared memory */
               if (send.msg_size > 0x10000)
                    return -EMSGSIZE;

               return fusionee_send_message (dev, fusion_id, send.fusion_id, FMT_SEND,
                                             send.msg_id, send.msg_size, send.msg_data);
     }

     return -ENOSYS;
}

static int
call_ioctl (FusionDev *dev, int fusion_id,
            unsigned int cmd, unsigned long arg)
{
     int               id;
     int               ret;
     FusionCallNew     call;
     FusionCallExecute execute;
     FusionCallReturn  call_ret;

     switch (_IOC_NR(cmd)) {
          case _IOC_NR(FUSION_CALL_NEW):
               if (copy_from_user (&call, (FusionCallNew*) arg, sizeof(call)))
                    return -EFAULT;

               ret = fusion_call_new (dev, fusion_id, &call);
               if (ret)
                    return ret;

               if (put_user (call.call_id, (int*) arg)) {
                    fusion_call_destroy (dev, fusion_id, call.call_id);
                    return -EFAULT;
               }
               return 0;

          case _IOC_NR(FUSION_CALL_EXECUTE):
               if (copy_from_user (&execute, (FusionCallExecute*) arg, sizeof(execute)))
                    return -EFAULT;

               ret = fusion_call_execute (dev, fusion_id, &execute);
               if (ret)
                    return ret;

               if (put_user (execute.ret_val, (int*) arg))
                    return -EFAULT;
               return 0;

          case _IOC_NR(FUSION_CALL_RETURN):
               if (copy_from_user (&call_ret, (FusionCallReturn*) arg, sizeof(call_ret)))
                    return -EFAULT;

               return fusion_call_return (dev, fusion_id, &call_ret);

          case _IOC_NR(FUSION_CALL_DESTROY):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_call_destroy (dev, fusion_id, id);
     }

     return -ENOSYS;
}

static int
ref_ioctl (FusionDev *dev, int fusion_id,
           unsigned int cmd, unsigned long arg)
{
     int              id;
     int              ret;
     int              refs;
     FusionRefWatch   watch;
     FusionRefInherit inherit;

     switch (_IOC_NR(cmd)) {
          case _IOC_NR(FUSION_REF_NEW):
               ret = fusion_ref_new (dev, &id);
               if (ret)
                    return ret;

               if (put_user (id, (int*) arg)) {
                    fusion_ref_destroy (dev, id);
                    return -EFAULT;
               }
               return 0;

          case _IOC_NR(FUSION_REF_UP):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_ref_up (dev, id, fusion_id);

          case _IOC_NR(FUSION_REF_UP_GLOBAL):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_ref_up (dev, id, 0);

          case _IOC_NR(FUSION_REF_DOWN):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_ref_down (dev, id, fusion_id);

          case _IOC_NR(FUSION_REF_DOWN_GLOBAL):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_ref_down (dev, id, 0);

          case _IOC_NR(FUSION_REF_ZERO_LOCK):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_ref_zero_lock (dev, id, fusion_id);

          case _IOC_NR(FUSION_REF_ZERO_TRYLOCK):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_ref_zero_trylock (dev, id, fusion_id);

          case _IOC_NR(FUSION_REF_UNLOCK):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_ref_unlock (dev, id, fusion_id);

          case _IOC_NR(FUSION_REF_STAT):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               ret = fusion_ref_stat (dev, id, &refs);
               if (ret)
                    return ret;

               return refs;

          case _IOC_NR(FUSION_REF_WATCH):
               if (copy_from_user (&watch, (FusionRefWatch*) arg, sizeof(watch)))
                    return -EFAULT;

               return fusion_ref_watch (dev, watch.id, watch.call_id, watch.call_arg);

          case _IOC_NR(FUSION_REF_INHERIT):
               if (copy_from_user (&inherit, (FusionRefInherit*) arg, sizeof(inherit)))
                    return -EFAULT;

               return fusion_ref_inherit (dev, inherit.id, inherit.from);

          case _IOC_NR(FUSION_REF_DESTROY):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_ref_destroy (dev, id);
     }

     return -ENOSYS;
}

static int
skirmish_ioctl (FusionDev *dev, int fusion_id,
                unsigned int cmd, unsigned long arg)
{
     int id;
     int ret;

     switch (_IOC_NR(cmd)) {
          case _IOC_NR(FUSION_SKIRMISH_NEW):
               ret = fusion_skirmish_new (dev, &id);
               if (ret)
                    return ret;

               if (put_user (id, (int*) arg)) {
                    fusion_skirmish_destroy (dev, id);
                    return -EFAULT;
               }
               return 0;

          case _IOC_NR(FUSION_SKIRMISH_PREVAIL):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_skirmish_prevail (dev, id, fusion_id);

          case _IOC_NR(FUSION_SKIRMISH_SWOOP):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_skirmish_swoop (dev, id, fusion_id);

          case _IOC_NR(FUSION_SKIRMISH_DISMISS):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_skirmish_dismiss (dev, id, fusion_id);

          case _IOC_NR(FUSION_SKIRMISH_DESTROY):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_skirmish_destroy (dev, id);
     }

     return -ENOSYS;
}

static int
property_ioctl (FusionDev *dev, int fusion_id,
                unsigned int cmd, unsigned long arg)
{
     int id;
     int ret;

     switch (_IOC_NR(cmd)) {
          case _IOC_NR(FUSION_PROPERTY_NEW):
               ret = fusion_property_new (dev, &id);
               if (ret)
                    return ret;

               if (put_user (id, (int*) arg)) {
                    fusion_property_destroy (dev, id);
                    return -EFAULT;
               }
               return 0;

          case _IOC_NR(FUSION_PROPERTY_LEASE):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_property_lease (dev, id, fusion_id);

          case _IOC_NR(FUSION_PROPERTY_PURCHASE):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_property_purchase (dev, id, fusion_id);

          case _IOC_NR(FUSION_PROPERTY_CEDE):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_property_cede (dev, id, fusion_id);

          case _IOC_NR(FUSION_PROPERTY_HOLDUP):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_property_holdup (dev, id, fusion_id);

          case _IOC_NR(FUSION_PROPERTY_DESTROY):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_property_destroy (dev, id);
     }

     return -ENOSYS;
}

static int
reactor_ioctl (FusionDev *dev, int fusion_id,
               unsigned int cmd, unsigned long arg)
{
     int                   id;
     int                   ret;
     FusionReactorDispatch dispatch;

     switch (_IOC_NR(cmd)) {
          case _IOC_NR(FUSION_REACTOR_NEW):
               ret = fusion_reactor_new (dev, &id);
               if (ret)
                    return ret;

               if (put_user (id, (int*) arg)) {
                    fusion_reactor_destroy (dev, id);
                    return -EFAULT;
               }
               return 0;

          case _IOC_NR(FUSION_REACTOR_ATTACH):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_reactor_attach (dev, id, fusion_id);

          case _IOC_NR(FUSION_REACTOR_DETACH):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_reactor_detach (dev, id, fusion_id);

          case _IOC_NR(FUSION_REACTOR_DISPATCH):
               if (copy_from_user (&dispatch,
                                   (FusionReactorDispatch*) arg, sizeof(dispatch)))
                    return -EFAULT;

               if (dispatch.msg_size <= 0)
                    return -EINVAL;

               /* message data > 64k should be stored in shared memory */
               if (dispatch.msg_size > 0x10000)
                    return -EMSGSIZE;

               return fusion_reactor_dispatch (dev, dispatch.reactor_id,
                                               dispatch.self ? 0 : fusion_id,
                                               dispatch.msg_size, dispatch.msg_data);

          case _IOC_NR(FUSION_REACTOR_DESTROY):
               if (get_user (id, (int*) arg))
                    return -EFAULT;

               return fusion_reactor_destroy (dev, id);
     }

     return -ENOSYS;
}

static int
fusion_ioctl (struct inode *inode, struct file *file,
              unsigned int cmd, unsigned long arg)
{
     int        id  = (int) file->private_data;
     FusionDev *dev = fusion_devs[iminor(inode)];

     DEBUG( "fusion_ioctl (0x%08x)\n", cmd );

     switch (_IOC_TYPE(cmd)) {
          case FT_LOUNGE:
               return lounge_ioctl( dev, id, cmd, arg );

          case FT_MESSAGING:
               return messaging_ioctl( dev, id, cmd, arg );

          case FT_CALL:
               return call_ioctl( dev, id, cmd, arg );

          case FT_REF:
               return ref_ioctl( dev, id, cmd, arg );

          case FT_SKIRMISH:
               return skirmish_ioctl( dev, id, cmd, arg );

          case FT_PROPERTY:
               return property_ioctl( dev, id, cmd, arg );

          case FT_REACTOR:
               return reactor_ioctl( dev, id, cmd, arg );
     }

     return -ENOSYS;
}

static struct file_operations fusion_fops = {
     .owner   = THIS_MODULE,
     .open    = fusion_open,
     .flush   = fusion_flush,
     .release = fusion_release,
     .read    = fusion_read,
     .poll    = fusion_poll,
     .ioctl   = fusion_ioctl
};

/******************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static int __init
register_devices(void)
{
     int  i;
     int ret;

     ret = register_chrdev (fusion_major, "fusion", &fusion_fops);
     if ((fusion_major == 0) && (ret > 0))
	    fusion_major = ret;
     else if ((fusion_major == 0) && (ret < 0)){
	     printk (KERN_ERR "fusion: unable to register character device\n");
	     return -EIO;
     } else if ((fusion_major != 0) && (ret != 0)){
	     printk (KERN_ERR "fusion: unable to get major %d\n", fusion_major);
	     return -EIO;
     }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 2)
     fusion_class = class_simple_create (THIS_MODULE, "fusion");
     if (IS_ERR(fusion_class)) {
          unregister_chrdev (fusion_major, "fusion");
          return PTR_ERR(fusion_class);
     }
#endif

     devfs_mk_dir("fusion");

     for (i=0; i<NUM_MINORS; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 2)
          class_simple_device_add (fusion_class,
                                   MKDEV(fusion_major, i),
                                   NULL, "fusion%d", i);
#endif

          devfs_mk_cdev (MKDEV(fusion_major, i),
                         S_IFCHR | S_IRUSR | S_IWUSR,
                         "fusion/%d", i);
     }

     return 0;
}
#else
static int __init
register_devices(void)
{
     int  i;
     char buf[16];

     if (devfs_register_chrdev (fusion_major, "fusion", &fusion_fops)) {
          printk (KERN_ERR "fusion: unable to get major %d\n", fusion_major);
          return -EIO;
     }

     for (i=0; i<NUM_MINORS; i++) {
          snprintf (buf, 16, "fusion/%d", i);

          devfs_handles[i] = devfs_register (NULL, buf, DEVFS_FL_DEFAULT,
                                             fusion_major, i,
                                             S_IFCHR | S_IRUSR | S_IWUSR,
                                             &fusion_fops, NULL);
     }

     return 0;
}
#endif

int __init
fusion_init(void)
{
     int ret;

     ret = register_devices();
     if (ret)
          return ret;

     proc_fusion_dir = proc_mkdir ("fusion", NULL);

     return 0;
}

/******************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static void __exit
deregister_devices(void)
{
     int i;

     unregister_chrdev (fusion_major, "fusion");

     for (i=0; i<NUM_MINORS; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 2)
          class_simple_device_remove (MKDEV(fusion_major, i));
#endif

          devfs_remove ("fusion/%d", i);
     }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 2)
     class_simple_destroy (fusion_class);
#endif

     devfs_remove ("fusion");
}
#else
static void __exit
deregister_devices(void)
{
     int i;

     devfs_unregister_chrdev (fusion_major, "fusion");

     for (i=0; i<NUM_MINORS; i++)
          devfs_unregister (devfs_handles[i]);
}
#endif

void __exit
fusion_exit(void)
{
     deregister_devices();

     remove_proc_entry ("fusion", NULL);
}

module_init(fusion_init);
module_exit(fusion_exit);

