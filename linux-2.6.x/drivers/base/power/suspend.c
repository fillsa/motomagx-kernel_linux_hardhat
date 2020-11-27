/*
 * suspend.c - Functions for putting devices to sleep.
 *
 * Copyright (c) 2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Labs
 * Copyright 2006 Motorola, Inc.
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
 * 10/04/2006  Motorola  Enhanced debugging support for device suspend/resume.
 *                       Added MPM logging for device suspend/resume.
 */

#include <linux/device.h>
#include "power.h"
#ifdef CONFIG_MOT_FEAT_PM_STATS
#include <linux/mpm.h>
#endif

#ifdef CONFIG_MOT_FEAT_PM_DEVICE_SUSPEND_DEBUG
struct device *serial_dev=NULL;
#ifdef CONFIG_ARCH_MXC91231
/* Pointer for tracking serial device during suspend/resume */
#define SERIAL_DEVICE_NAME "mxcintuart.2"
#else
#error CONFIG_MOT_FEAT_PM_DEVICE_SUSPEND_DEBUG is only supported for SCMA11
#endif
#endif

extern int sysdev_suspend(u32 state);

/*
 * The entries in the dpm_active list are in a depth first order, simply
 * because children are guaranteed to be discovered after parents, and
 * are inserted at the back of the list on discovery.
 *
 * All list on the suspend path are done in reverse order, so we operate
 * on the leaves of the device tree (or forests, depending on how you want
 * to look at it ;) first. As nodes are removed from the back of the list,
 * they are inserted into the front of their destintation lists.
 *
 * Things are the reverse on the resume path - iterations are done in
 * forward order, and nodes are inserted at the back of their destination
 * lists. This way, the ancestors will be accessed before their descendents.
 */


/**
 *	suspend_device - Save state of one device.
 *	@dev:	Device.
 *	@state:	Power state device is entering.
 */

int suspend_device(struct device * dev, u32 state)
{
	int error = 0;

	dev_dbg(dev, "suspending\n");

	dev->power.prev_state = dev->power.power_state;

	if (dev->bus && dev->bus->suspend && !dev->power.power_state)
		error = dev->bus->suspend(dev, state);

	if (! error)
		deassert_constraints(dev->constraints);

	return error;
}


/**
 *	device_suspend - Save state and stop all devices in system.
 *	@state:		Power state to put each device in.
 *
 *	Walk the dpm_active list, call ->suspend() for each device, and move
 *	it to dpm_off.
 *	Check the return value for each. If it returns 0, then we move the
 *	the device to the dpm_off list. If it returns -EAGAIN, we move it to
 *	the dpm_off_irq list. If we get a different error, try and back out.
 *
 *	If we hit a failure with any of the devices, call device_resume()
 *	above to bring the suspended devices back to life.
 *
 */

int device_suspend(u32 state)
{
	int error = 0;

	down(&dpm_sem);
	down(&dpm_list_sem);
	while (!list_empty(&dpm_active) && error == 0) {
		struct list_head * entry = dpm_active.prev;
		struct device * dev = to_device(entry);

		get_device(dev);
		up(&dpm_list_sem);

#ifdef CONFIG_MOT_FEAT_PM_DEVICE_SUSPEND_DEBUG
                 /* Save the serial device to suspend last */
                if(strcmp(kobject_name(&dev->kobj), SERIAL_DEVICE_NAME) == 0) {
                        pr_debug("Leaving %s on for now\n", SERIAL_DEVICE_NAME);
                        serial_dev = dev;
                        /* Remove the item from the active list */
                        list_del(&dev->power.entry);
                        continue;
                }
#endif
		error = suspend_device(dev, state);

		down(&dpm_list_sem);

		/* Check if the device got removed */
		if (!list_empty(&dev->power.entry)) {
			/* Move it to the dpm_off or dpm_off_irq list */
			if (!error) {
				list_del(&dev->power.entry);
				list_add(&dev->power.entry, &dpm_off);
			} else if (error == -EAGAIN) {
				list_del(&dev->power.entry);
				list_add(&dev->power.entry, &dpm_off_irq);
				error = 0;
			}
		}
		if (error) {
#ifdef CONFIG_MOT_FEAT_PM
                        printk("Delaying suspend sequence because device %s is busy "
                               "(retval %d)\n", kobject_name(&dev->kobj), error);
#else
			printk(KERN_ERR "Could not suspend device %s: "
				"error %d\n", kobject_name(&dev->kobj), error);
#endif
#ifdef CONFIG_MOT_FEAT_PM_STATS
                        MPM_REPORT_TEST_POINT(2, MPM_TEST_DEVICE_SUSPEND_FAILED,
                                              kobject_name(&dev->kobj));
#endif
                }
		put_device(dev);
	}

#ifdef CONFIG_MOT_FEAT_PM_DEVICE_SUSPEND_DEBUG
        /* Now suspend the serial device */
        if(serial_dev) {
                error = suspend_device(serial_dev, state);
        }
#endif
        
	up(&dpm_list_sem);
	if (error)
		dpm_resume();
	up(&dpm_sem);
	return error;
}

EXPORT_SYMBOL_GPL(device_suspend);


/**
 *	device_power_down - Shut down special devices.
 *	@state:		Power state to enter.
 *
 *	Walk the dpm_off_irq list, calling ->power_down() for each device that
 *	couldn't power down the device with interrupts enabled. When we're
 *	done, power down system devices.
 */

int device_power_down(u32 state)
{
	int error = 0;
	struct device * dev;

	list_for_each_entry_reverse(dev, &dpm_off_irq, power.entry) {
		if ((error = suspend_device(dev, state)))
			break;
	}
	if (error)
		goto Error;
	if ((error = sysdev_suspend(state)))
		goto Error;
 Done:
	return error;
 Error:
#ifdef CONFIG_MOT_FEAT_PM_STATS
        MPM_REPORT_TEST_POINT(2, MPM_TEST_DEVICE_SUSPEND_FAILED,
                              kobject_name(&dev->kobj));
#endif
	dpm_power_up();
	goto Done;
}

EXPORT_SYMBOL_GPL(device_power_down);

