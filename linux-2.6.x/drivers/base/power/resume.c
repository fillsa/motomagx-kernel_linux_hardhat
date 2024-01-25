/*
 * resume.c - Functions for waking devices up.
 *
 * Copyright (c) 2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Labs
 * Copyright 2006-2008 Motorola, Inc.
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
 * 04/04/2007  Motorola  Fixed bug in device suspend debug functionality.
 * 09/24/2008  Motorola  Add log to track kpanic
 */

#include <linux/device.h>
#include "power.h"

#ifdef CONFIG_MOT_FEAT_PM_DEVICE_SUSPEND_DEBUG
/* Pointer for tracking serial device during suspend/resume */
extern struct device *serial_dev;
#endif

extern int sysdev_resume(void);


/**
 *	resume_device - Restore state for one device.
 *	@dev:	Device.
 *
 */

int resume_device(struct device * dev)
{
	int error = 0;

	if (dev->bus && dev->bus->resume) {

#ifdef CONFIG_MOT_FEAT_PM_DEVICE_SUSPEND_DEBUG
		dev_dbg(dev, "resuming\n");
#endif
		error = dev->bus->resume(dev);

		if (!error)
			assert_constraints(dev->constraints);
	}

	return error;
}

#ifdef  DBG_TRACK_KPANIC
static inline void tmp_dump_dev(unsigned long p[])
{
    unsigned int dump_offset=0;
    printk("try dump dev, dev=%p \n",p);
    for (dump_offset=0;dump_offset<(sizeof(struct device)/4);dump_offset++) {
        if (0==dump_offset%4) printk("\n%p: ", &p[dump_offset]);
        printk("0x%08lx ",p[dump_offset] );
    }
    printk("\n");
}

static inline void tmp_look_dpm_off(struct list_head * tmp_ptr,char * tmp_str)
{
    unsigned int tmp_count=0;
    

    printk("Try to look up dpm_off list through %s\n",tmp_str);
    while ( 100 > tmp_count ) {
        printk("%s=%p \n",tmp_str,tmp_ptr);
        if ( 0xbf000000 > (unsigned long)tmp_ptr) {
            printk("some %s of dpm_off list is wrong  \n",tmp_str);
            tmp_count=7777;
            break;
        }
        if ( tmp_ptr == &dpm_off ) {
            tmp_count=9999;
            break;
        }
        if ( strstr(tmp_str,"next")) {
            tmp_ptr=tmp_ptr->next;
        } else {
            tmp_ptr=tmp_ptr->prev;
        }
        tmp_count++;
    }

    if ( 9999 == tmp_count) {
        printk("all %s of dpm_off is OK \n",tmp_str);
    } else {
        if ( 7777 != tmp_count ) {
             printk("some %s of dpm_off is wrong! \n",tmp_str);
        }
    }

}
                                                                                                                                                      
#endif


void dpm_resume(void)
{
	down(&dpm_list_sem);

#ifdef CONFIG_MOT_FEAT_PM_DEVICE_SUSPEND_DEBUG
        /* If serial was suspended, resume it first */
        if(serial_dev) {
		get_device(serial_dev);
		list_add_tail(&serial_dev->power.entry, &dpm_active);
                resume_device(serial_dev);
		put_device(serial_dev);                
                serial_dev = NULL;
        }
#endif

	while(!list_empty(&dpm_off)) {
		struct list_head * entry = dpm_off.next;
		struct device * dev = to_device(entry);
#ifdef  DBG_TRACK_KPANIC
                if ( 0xbf000000 > (unsigned long)entry ) {
                    printk("find wrong value of entry from dpm_list \n");
                    tmp_look_dpm_off(dpm_off.prev,"prev");
                } else { 
                    if ( 0xbf000000 >(unsigned long)entry->next || 0xbf000000 > (unsigned long)entry->prev) {
                        printk("find wrong value in dpm_off list,dump device struct below: \n");
                        tmp_dump_dev((unsigned long *)dev);
                        tmp_look_dpm_off(dpm_off.next,"next");
                        tmp_look_dpm_off(dpm_off.prev,"prev");
                    }
                }
#endif
		get_device(dev);
		list_del_init(entry);
		list_add_tail(entry, &dpm_active);

		up(&dpm_list_sem);
		if (!dev->power.prev_state)
			resume_device(dev);
		down(&dpm_list_sem);
		put_device(dev);
	}
	up(&dpm_list_sem);
}


/**
 *	device_resume - Restore state of each device in system.
 *
 *	Walk the dpm_off list, remove each entry, resume the device,
 *	then add it to the dpm_active list.
 */

void device_resume(void)
{
	down(&dpm_sem);
	dpm_resume();
	up(&dpm_sem);
}

EXPORT_SYMBOL_GPL(device_resume);


/**
 *	device_power_up_irq - Power on some devices.
 *
 *	Walk the dpm_off_irq list and power each device up. This
 *	is used for devices that required they be powered down with
 *	interrupts disabled. As devices are powered on, they are moved to
 *	the dpm_suspended list.
 *
 *	Interrupts must be disabled when calling this.
 */

void dpm_power_up(void)
{
	while(!list_empty(&dpm_off_irq)) {
		struct list_head * entry = dpm_off_irq.next;
		struct device * dev = to_device(entry);

		get_device(dev);
		list_del_init(entry);
		list_add_tail(entry, &dpm_active);
		resume_device(dev);
		put_device(dev);
	}
}


/**
 *	device_pm_power_up - Turn on all devices that need special attention.
 *
 *	Power on system devices then devices that required we shut them down
 *	with interrupts disabled.
 *	Called with interrupts disabled.
 */

void device_power_up(void)
{
	sysdev_resume();
	dpm_power_up();
}

EXPORT_SYMBOL_GPL(device_power_up);


