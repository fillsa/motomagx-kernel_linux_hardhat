/*
 * kernel/power/main.c - PM subsystem core functionality.
 *
 * Copyright (c) 2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Lab
 * Copyright 2006-2007 Motorola, Inc.
 * 
 * This file is released under the GPLv2
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/*
 * Revision History:
 *
 * Date        Author    Comment
 * 10/04/2006  Motorola  Include module.h and mpm.h.
 *                       Rescan blocks upon resume to prevent
 *                       filesystem corruption.
 *                       ifdef'd out pm_prepare_console because it was
 *                       unneeded and corrupting framebuffer memory.
 *                       Exported symbol pm_suspend to support the
 *                       Motorola Power Management (MPM) module.
 *                       Added MPM logging support.
 * 02/14/2007  Motorola  Resolve race condition in DSM code.
 */

#include <linux/suspend.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/reboot.h>
#ifdef CONFIG_MOT_FEAT_PM
#include <linux/module.h>
#include <linux/mpm.h>
#endif

#include "power.h"

DECLARE_MUTEX(pm_sem);

struct pm_ops * pm_ops = NULL;
suspend_disk_method_t pm_disk_mode = PM_DISK_SHUTDOWN;

static void clear_waker(void);

/**
 *	pm_set_ops - Set the global power method table. 
 *	@ops:	Pointer to ops structure.
 */

void pm_set_ops(struct pm_ops * ops)
{
	down(&pm_sem);
	pm_ops = ops;
	up(&pm_sem);
}


#ifdef CONFIG_SUSPEND_REMOUNTFS
/*
 * Code to preserve filesystem data during suspend.
 */

#include <linux/fs.h>
#include <linux/buffer_head.h>

struct suspremount {
	struct super_block *sb;
	struct suspremount *next;
};

static struct suspremount *suspremount_list;

extern void mtdblock_flush_all(void);

static void susp_remount_one_ro(struct super_block *sb)
{
	int retval, flags;
	struct suspremount *remountp;

	shrink_dcache_sb(sb);
	fsync_super(sb);

	/* If we are remounting RDONLY and current sb is read/write,
	   make sure there are no rw files opened */
	if (!(sb->s_flags & MS_RDONLY)) {
		if (!fs_may_remount_ro(sb))
			return;
	}

	flags = MS_RDONLY;

	if (sb->s_op->remount_fs) {
		lock_super(sb);
		retval = sb->s_op->remount_fs(sb, &flags, NULL);
		unlock_super(sb);

		if (! retval) {
			if ((remountp = (struct suspremount *)
			     kmalloc(sizeof(struct suspremount), GFP_KERNEL))
			    != NULL) {
				remountp->sb = sb;
				remountp->next  = suspremount_list;
				suspremount_list = remountp;
			}
		}
	}
}

/*
 * Remount filesystems prior to suspend, in case the
 * power source is removed (ie, battery removed) or
 * battery dies during suspend.
 */

static void susp_remount_all_ro(void)
{
	struct super_block *sb;

	suspremount_list = NULL;

#ifdef CONFIG_MOT_WFN406
rescan:
#endif
	spin_lock(&sb_lock);
	list_for_each_entry(sb, &super_blocks, s_list) {
		sb->s_count++;
		spin_unlock(&sb_lock);
		down_read(&sb->s_umount);
#ifdef CONFIG_MOT_WFN406
		if (list_empty(&sb->s_list))
		{
			drop_super(sb);
			goto rescan;
		}
#endif
		if (sb->s_root && sb->s_bdev && !(sb->s_flags & MS_RDONLY)) {
			susp_remount_one_ro(sb);
		}
		drop_super(sb);
		spin_lock(&sb_lock);
	}
	spin_unlock(&sb_lock);
		
#ifdef CONFIG_MTD_BLOCK
	/*
	 * Flush mtdblock caches.
	 */

	
	mtdblock_flush_all();
#endif
}

static void resume_remount_rw(void)
{
	struct suspremount *remountp;

	remountp = suspremount_list;

	while (remountp != NULL) {
		struct suspremount *tp;
		struct super_block *sb;
		int flags, ret;

		sb = remountp->sb;
		flags = 0;
		if (sb->s_op && sb->s_op->remount_fs) {
			ret = sb->s_op->remount_fs(sb, &flags, NULL);
			if (ret)
				printk("resume_remount_rw: error %d\n", ret);
		}

		tp = remountp->next;
		kfree(remountp);
		remountp = tp;
	}

	suspremount_list = NULL;
}

#else /* CONFIG_SUSPEND_REMOUNTFS */
static void susp_remount_all_ro(void)
{
}
static void resume_remount_rw(void)
{
}
#endif /* CONFIG_SUSPEND_REMOUNTFS */

/**
 *	suspend_prepare - Do prep work before entering low-power state.
 *	@state:		State we're entering.
 *
 *	This is common code that is called for each state that we're 
 *	entering. Allocate a console, stop all processes, then make sure
 *	the platform can enter the requested state.
 */

static int suspend_prepare(suspend_state_t state)
{
	int error = 0;

	if (!pm_ops || !pm_ops->enter)
		return -EPERM;

#ifndef CONFIG_MOT_FEAT_PM
	/* 
	 * We do not wish to enable the DSM console since it is corrupting the 
	 * framebuffer memory by clearing it to zero 
	 */
	pm_prepare_console();
#endif

#ifndef CONFIG_MOT_FEAT_PM
	if (freeze_processes()) {
		error = -EAGAIN;
		goto Thaw;
	}
#endif

	susp_remount_all_ro();

	if (pm_ops->prepare) {
		if ((error = pm_ops->prepare(state)))
			goto Remount;
	}

#ifdef CONFIG_MOT_FEAT_PM_STATS
        MPM_REPORT_TEST_POINT(1, MPM_TEST_DEVICE_SUSPEND_START);
#endif
	if ((error = device_suspend(state))) {
#ifdef CONFIG_MOT_FEAT_PM_STATS
                MPM_REPORT_TEST_POINT(1, MPM_TEST_DEVICE_SUSPEND_DONE);
#endif
		goto Finish;
        }
	return 0;
 Finish:
	if (pm_ops->finish)
		pm_ops->finish(state);

 Remount:
	resume_remount_rw();
 Thaw:
#ifndef CONFIG_MOT_FEAT_PM
	thaw_processes();
	pm_restore_console();
#endif
	return error;
}


static suspend_state_t last_suspend_state;

suspend_state_t pm_state_resumed(void)
{
	return last_suspend_state;
}


static int suspend_enter(u32 state)
{
	int error = 0;
	unsigned long flags;

	clear_waker();
	local_irq_save(flags);

#ifdef CONFIG_MOT_FEAT_PM
        if (mpm_ready_to_sleep() == MPM_DO_NOT_SLEEP) {
#ifdef CONFIG_MOT_FEAT_PM_STATS
                MPM_REPORT_TEST_POINT(1, MPM_TEST_DEVICE_SUSPEND_DONE);
#endif
		error = -EAGAIN;
                goto Done;
        }

#ifdef CONFIG_MOT_FEAT_PM_STATS
        MPM_REPORT_TEST_POINT(1, MPM_TEST_DEVICE_SUSPEND_INTERRUPTS_DISABLED_START);
#endif
#endif /* CONFIG_MOT_FEAT_PM */

        /*
         * Motorola split this operation to make the function call first,
         * then check the error value after reporting an mpm test point.
         */
	error = device_power_down(state);

#ifdef CONFIG_MOT_FEAT_PM_STATS
        MPM_REPORT_TEST_POINT(1, MPM_TEST_DEVICE_SUSPEND_INTERRUPTS_DISABLED_DONE);
        MPM_REPORT_TEST_POINT(1, MPM_TEST_DEVICE_SUSPEND_DONE);
#endif

	if (error)
	        goto Done;

again:
	last_suspend_state = state;
	error = pm_ops->enter(state);

	if (!error && pm_ops->wake) {
		if (pm_ops->wake(state) == 1)
			goto again;
	}

#ifdef CONFIG_MOT_FEAT_PM
	/*
	 * Call mpm_resume_from_sleep to set the state before
	 * device_power_up so that any changes in state during
	 * device_power_up are properly handled.
	 */
        mpm_resume_from_sleep();
#endif /* endif CONFIG_MOT_FEAT_PM */

	device_power_up();

 Done:
	local_irq_restore(flags);
	return error;
}


/**
 *	suspend_finish - Do final work before exiting suspend sequence.
 *	@state:		State we're coming out of.
 *
 *	Call platform code to clean up, restart processes, and free the 
 *	console that we've allocated.
 */

static void suspend_finish(suspend_state_t state)
{
	device_resume();
	if (pm_ops && pm_ops->finish)
		pm_ops->finish(state);

	resume_remount_rw();
#ifndef CONFIG_MOT_FEAT_PM
	thaw_processes();
	pm_restore_console();
#endif
}




char * pm_states[] = {
	[PM_SUSPEND_STANDBY]	= "standby",
	[PM_SUSPEND_MEM]	= "mem",
	[PM_SUSPEND_DISK]	= "disk",
	NULL,
};


/**
 *	enter_state - Do common work of entering low-power state.
 *	@state:		pm_state structure for state we're entering.
 *
 *	Make sure we're the only ones trying to enter a sleep state. Fail
 *	if someone has beat us to it, since we don't want anything weird to
 *	happen when we wake up.
 *	Then, do the setup for suspend, enter the state, and cleaup (after
 *	we've woken up).
 */

static int enter_state(suspend_state_t state)
{
	int error;

	if (down_trylock(&pm_sem))
		return -EBUSY;

	/* Suspend is hard to get right on SMP. */
	if (num_online_cpus() != 1) {
		error = -EPERM;
		goto Unlock;
	}

	if (state == PM_SUSPEND_DISK) {
		error = pm_suspend_disk();
		goto Unlock;
	}

	pr_debug("PM: Preparing system for suspend\n");
	if ((error = suspend_prepare(state)))
		goto Unlock;

	pr_debug("PM: Entering state.\n");
	error = suspend_enter(state);

	pr_debug("PM: Finishing up.\n");
	suspend_finish(state);
 Unlock:
	up(&pm_sem);

#ifdef CONFIG_MOT_FEAT_PM_STATS
        if (error) {
                MPM_REPORT_TEST_POINT(1, MPM_TEST_SLEEP_ATTEMPT_FAILED);
        }
#endif
	return error;
}

/*
 * This is main interface to the outside world. It needs to be
 * called from process context.
 */
int software_suspend(void)
{
	return enter_state(PM_SUSPEND_DISK);
}


/**
 *	pm_suspend - Externally visible function for suspending system.
 *	@state:		Enumarted value of state to enter.
 *
 *	Determine whether or not value is within range, get state 
 *	structure, and enter (above).
 */

int pm_suspend(suspend_state_t state)
{
	if (state > PM_SUSPEND_ON && state < PM_SUSPEND_MAX)
		return enter_state(state);
	return -EINVAL;
}



decl_subsys(power,NULL,NULL);


/**
 *	state - control system power state.
 *
 *	show() returns what states are supported, which is hard-coded to
 *	'standby' (Power-On Suspend), 'mem' (Suspend-to-RAM), and
 *	'disk' (Suspend-to-Disk).
 *
 *	store() accepts one of those strings, translates it into the 
 *	proper enumerated value, and initiates a suspend transition.
 */

static ssize_t state_show(struct subsystem * subsys, char * buf)
{
	int i;
	char * s = buf;

	for (i = 0; i < PM_SUSPEND_MAX; i++) {
		if (pm_states[i])
			s += sprintf(s,"%s ",pm_states[i]);
	}
	s += sprintf(s,"\n");
	return (s - buf);
}

static ssize_t state_store(struct subsystem * subsys, const char * buf, size_t n)
{
	suspend_state_t state = PM_SUSPEND_STANDBY;
	char ** s;
	char *p;
	int error;
	int len;

	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;

	for (s = &pm_states[state]; state < PM_SUSPEND_MAX; s++, state++) {
		if (*s && !strncmp(buf, *s, len))
			break;
	}
	if (*s)
		error = enter_state(state);
	else
		error = -EINVAL;
	return error ? error : n;
}

power_attr(state);

static ssize_t fastshutdown_show(struct subsystem * subsys, char * buf)
{
	return sprintf(buf,"enabled\n");
}

static ssize_t fastshutdown_store(struct subsystem * subsys, const char * buf,
				  size_t n)
{
	/*
	 * Actions taken:
	 *
	 *    1. Freeze all user processes.
	 *
	 *    2. Remount filesystems read-only to flush data and mark
	 *       filesystems clean.
	 *
	 *    3. Halt.
	 */

	printk(KERN_EMERG "Fast shutdown initiated.\n");
	freeze_processes();
	susp_remount_all_ro();
#ifdef CONFIG_KFI
	kfi_dump_log(NULL);
#endif
	machine_power_off();

	/*
	 * Power off not implemented on this platform.  Do the best we can
	 * by powering off all devices and entering standby mode, re-entering
	 * each time awoken.
	 */

	device_suspend(3);

	while (1)
		pm_ops && pm_ops->enter(PM_SUSPEND_STANDBY);
}

power_attr(fastshutdown);


static void clear_waker(void)
{
	sysfs_remove_link(&power_subsys.kset.kobj, "waker");
}

void pm_set_waker(struct device *dev)
{
	clear_waker();
	sysfs_create_link(&power_subsys.kset.kobj, &dev->kobj, "waker");
}

void pm_set_class_waker(struct class_device *dev)
{
	clear_waker();
	sysfs_create_link(&power_subsys.kset.kobj, &dev->kobj, "waker");
}

static struct attribute * g[] = {
	&state_attr.attr,
	&fastshutdown_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};


static int __init pm_init(void)
{
	int error = subsystem_register(&power_subsys);
	if (!error)
		error = sysfs_create_group(&power_subsys.kset.kobj,&attr_group);
	return error;
}

core_initcall(pm_init);

EXPORT_SYMBOL(pm_set_waker);

#ifdef CONFIG_MOT_FEAT_PM
EXPORT_SYMBOL_GPL(pm_suspend);
#endif
