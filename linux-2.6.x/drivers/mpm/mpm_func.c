/*
 * Copyright 2006-2007 Motorola, Inc.
 * 
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
 * 10/04/2006  Motorola  Initial version.
 * 01/18/2007  Motorola  Tie DSM to IDLE and reduce IOI timeout to 10ms
 * 01/28/2007  Motorola  Add long IOI timeout API for SD Card
 * 02/06/2007  Motorola  Resolve DSM race.
 * 04/12/2007  Motorola  Check in idle for busy drivers/modules.
 */

#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/mpm.h>
#include <linux/rtc.h>
#ifdef CONFIG_MOT_FEAT_PM_DESENSE
#include <asm/setup.h>
#endif

DECLARE_WAIT_QUEUE_HEAD(mpm_wq);
spinlock_t devlk = SPIN_LOCK_UNLOCKED;

static mpm_devq_t dq = {
    .event_tail = 0,
    .event_head = 0,
};

#ifdef CONFIG_MOT_FEAT_PM_DESENSE
/*
 * mpm_no_desense_panic controls whether to panic if the desense data
 * is invalid.  mpm_no_desense_panic is off by default, so the phone
 * panics when the desense data is invalid.
 */
static int mpm_no_desense_panic = 0;
#endif /* CONFIG_MOT_FEAT_PM_DESENSE */

/* MPM registered driver info */
static mpm_driver_info_t mpmdi = {0};
mpm_driver_info_t *mpmdip = &mpmdi;
spinlock_t mpm_driver_info_lock = SPIN_LOCK_UNLOCKED;

static void queue_mpm_event(mpm_event_t );

/* Dummy functions for initial value of callback routines */
static int mpm_dummy_function(void);
#ifdef CONFIG_MOT_FEAT_PM_STATS
static void mpm_test_dummy_function(int, ...);
#endif

/*
 * MPM callback routines
 */

/* This function is called by device drivers when an IOI(Interrupt Of Interest)
 * occurs. 
 */
mpm_callback_t mpm_handle_ioi = mpm_dummy_function;
/* This function is called by SD driver when an IOI occurs (This is a shortterm hack)
 * occurs. 
 */
mpm_callback_t mpm_handle_long_ioi = mpm_dummy_function;
/* This function is called by periodic stopwatch driver when a periodic job is 
 * started. 
 */
mpm_callback_t mpm_periodic_jobs_started = mpm_dummy_function;
/* This function is called by periodic stopwatch driver when all the periodic jobs
 * are completed. 
 */
mpm_callback_t mpm_periodic_jobs_done = mpm_dummy_function;
/* This function is called by the MPM driver when a device event is queued to 
 * indicate change of mpm low power mode state to LPM_AWAKE.
 */
mpm_callback_t mpm_set_awake_state = mpm_dummy_function;
/* This function is called by suspend_enter() after interrupts are disabled to 
 * check if the system is ready to enter low power mode
 */
mpm_callback_t mpm_ready_to_sleep = mpm_dummy_function;

/* This function is called by IDLE to start/initiate the sleep process if the Power
 * Management subsystem is in a state that allows the system to enter DSM.
 * Note: the caller of mpm_start_sleep relies on the fact that mpm_dummy_function 
 * returns 0.
 */
mpm_callback_t mpm_start_sleep = mpm_dummy_function;

/* This function is called by suspend_enter() after the machine has
 * woken up from sleeping, but before interrupts are enabled to
 * complete processing that must be done before interrupts are
 * enabled.
 */
mpm_callback_t mpm_resume_from_sleep = mpm_dummy_function;

#ifdef CONFIG_MOT_FEAT_PM_STATS
/* This function is used to report test points from various points in the
 * suspend/resume sequence
 */
mpm_test_callback_t mpm_report_test_point = mpm_test_dummy_function;

/* MPM statistics pointer. */
mpm_stats_t *mpmsp = NULL;

spinlock_t mpmsplk = SPIN_LOCK_UNLOCKED;
#endif


int mpm_queue_empty(void)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&devlk, flags);
    if (dq.event_head == dq.event_tail)
    {
        ret = 1;
    }
    spin_unlock_irqrestore(&devlk, flags);
    return ret;
}

void mpm_get_queued_event(mpm_event_t *evt)
{
    unsigned long flags;
    spin_lock_irqsave(&devlk, flags);
    dq.event_tail = (dq.event_tail + 1) % MPM_MAX_EVENTS;
    *evt = dq.events[dq.event_tail];

    spin_unlock_irqrestore(&devlk, flags);
    return;
}


static void queue_mpm_event(mpm_event_t event)
{
    unsigned long flags;
    spin_lock_irqsave(&devlk, flags);

    dq.event_head = (dq.event_head + 1) % MPM_MAX_EVENTS;
    if (dq.event_head == dq.event_tail) 
    {
        printk(KERN_ERR "mpm:  event queue overflowed\n");
        dq.event_tail = (dq.event_tail + 1) % MPM_MAX_EVENTS;
    }
    dq.events[dq.event_head] = event;
    spin_unlock_irqrestore(&devlk, flags);
    wake_up_interruptible(&mpm_wq);
    return;
}



void mpm_event_notify(short type, short kind, int info)
{
    mpm_event_t event;

    event.type = type;
    event.kind = kind;
    event.info = info;
    queue_mpm_event(event);
    (void)mpm_set_awake_state();

    return;
}

/*
 * mpm_get_extra_size returns the number of extra bytes required after
 * the mpm_stats_t structure that will hold all of the dynamic data
 * (bitarrays, name_arr, driver names, etc.).
 *
 * mpm_get_extra_size intentionally does not obtain the
 * mpm_driver_info_lock.  It is the responsibility of the caller to
 * ensure that the mpm_driver_info_lock is held before calling this
 * function.  This function does not obtain the lock because it is
 * called in places where the lock must already be held, so this
 * function can't obtain the lock (deadlock).
 */
int mpm_get_extra_size(void)
{
    int extra_size;

    /*
     * The extra space is consumed by these items, in this order:
     *   registered driver bitarray
     *   busy driver bitarray
     *   array of pointers to driver name strings
     *   driver name strings
     */
    extra_size = sizeof_registered_bitarray(mpmdip->size_in_bits) +
		 sizeof_busy_bitarray(mpmdip->size_in_bits) +
		 sizeof_name_ptr_array(mpmdip->size_in_bits) +
		 sizeof_name_strings(mpmdip->total_name_size);

#ifdef CONFIG_MOT_FEAT_PM_STATS
    /* Add space for busy_arr array of bitarrays. */
    if (LPM_STATS_ENABLED())
        extra_size += sizeof_busy_arr_bitarrays(mpmdip->size_in_bits);
#endif /* CONFIG_MOT_FEAT_PM_STATS */

    return extra_size;
}

/*
 * mpm_register_with_mpm is called by any driver/module that wishes
 * to register with the MPM.  Registration with the MPM is required
 * by any driver/module that wants to send advice to the MPM via
 * mpm_driver_advise.
 *
 * mpm_register_with_mpm does not restrict the number of
 * registrations.
 *
 * mpm_register_with_mpm returns a nonnegative number that represents
 * the assigned driver number if the call is successful.  Negative
 * values indicate an error return.
 *
 * The results are undefined if mpm_register_with_mpm is called in an
 * interrupt context.
 * 
 * mpm_register_with_mpm returns
 *     Assigned driver number (nonnegative)
 *     -ENOMEM if can't allocate memory
 */
int mpm_register_with_mpm(const char *name)
{
    unsigned char *p;
    unsigned long *old_p = NULL;
    int newsize, oldsize;
    int newsize_in_bytes, oldsize_in_bytes;
    int new_name_arr_size, old_name_arr_size;
    int bit;
    int i;
    char *cp;
    int malloc_size;
    int total_name_size;
    unsigned long driver_flags;
    char *driver_name;
    int namelen;
#ifdef CONFIG_MOT_FEAT_PM_STATS
    int new_busy_arr_size = 0;
    int old_busy_arr_size = 0;
    unsigned long *old_busy_arr_p = NULL;
    unsigned long *new_busy_arr_p = NULL;
    unsigned long mpmsplk_flags;
#endif /* CONFIG_MOT_FEAT_PM_STATS */

    namelen = strlen(name);
    if (namelen == 0)
    {
	return -EINVAL;
    }

    driver_name = kmalloc (namelen + 1, GFP_KERNEL);
    if (driver_name == NULL)
    {
	return -ENOMEM;
    }

    strlcpy(driver_name, name, namelen + 1);

#ifdef CONFIG_MOT_FEAT_PM_STATS
    spin_lock_irqsave(&mpmsplk, mpmsplk_flags);
#endif /* CONFIG_MOT_FEAT_PM_STATS */
    spin_lock_irqsave(&mpm_driver_info_lock, driver_flags);

    if ((bit = find_first_zero_bit(mpmdip->registered_p,
                                   mpmdip->size_in_bits)) ==
	mpmdip->size_in_bits)
    {
	/*
	 * We didn't find an unallocated bit in the registered_p
	 * bitarray, so we need more bits.  Save the old size so we
	 * can detect if we lose the race to create more bits.
	 */
	oldsize           = mpmdip->size_in_bits;
        oldsize_in_bytes  = sizeof_registered_bitarray(oldsize);
	old_name_arr_size = sizeof_name_ptr_array(oldsize);
        total_name_size   = sizeof_name_strings(mpmdip->total_name_size);

	/*
	 * The malloc space is consumed by these items, in this order:
	 *   registered driver bitarray
	 *   busy driver bitarray
	 *   array of pointers to driver name strings
	 */
        newsize = oldsize + BITS_PER_LONG;
        newsize_in_bytes = sizeof_registered_bitarray(newsize);
	new_name_arr_size = sizeof_name_ptr_array(newsize);
        malloc_size = newsize_in_bytes +        /* registered_p bitarray */
		      newsize_in_bytes +        /* busy_p bitarray */
		      new_name_arr_size;        /* array of name pointers */

	/*
	 * We allocate both the registered_p and busy_p bitarrays, the
	 * saved busy bitarrays, and the name_arr array of pointers
	 * all at the same time in order to only make one allocation.
	 */
        if ((p = kmalloc (malloc_size, GFP_ATOMIC)) == NULL)
	{
            spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
#ifdef CONFIG_MOT_FEAT_PM_STATS
            spin_unlock_irqrestore(&mpmsplk, mpmsplk_flags);
#endif /* CONFIG_MOT_FEAT_PM_STATS */
	    kfree(driver_name);
	    return -ENOMEM;
	}

	/* Clear the malloced space. */
	memset(p, 0, malloc_size);

	/* Save the existing pointer so we can free it later. */
	old_p = mpmdip->registered_p;

	/* Copy the bitarrays and the name_arr array. */
        memcpy(k_registered_bitarray_addr(p, newsize),
               mpmdip->registered_p, oldsize_in_bytes);
        memcpy(k_busy_bitarray_addr(p, newsize),
               mpmdip->busy_p, oldsize_in_bytes);
        memcpy(k_name_ptr_array_addr(p, newsize),
               mpmdip->name_arr, old_name_arr_size);

#ifdef CONFIG_MOT_FEAT_PM_STATS
        if (LPM_STATS_ENABLED())
	{
            old_busy_arr_size = sizeof_busy_arr_bitarrays(mpmdip->size_in_bits);
            new_busy_arr_size = sizeof_busy_arr_bitarrays(newsize);
            if ((new_busy_arr_p = kmalloc(new_busy_arr_size, GFP_ATOMIC)) == NULL)
	    {
                spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
                spin_unlock_irqrestore(&mpmsplk, mpmsplk_flags);
	        kfree(driver_name);
	        kfree(p);
	        return -ENOMEM;
	    }

            memset(new_busy_arr_p, 0, new_busy_arr_size);

	    /* Save the existing pointer so we can free it later. */
	    old_busy_arr_p = mpmsp->driver_stats.busy_arr[0];

            cp = (char *)new_busy_arr_p;
            for (i=0; i<MAX_IDLE_DELAYS_SAVED; i++)
            {
                memcpy(cp, mpmsp->driver_stats.busy_arr[i], oldsize_in_bytes);
                mpmsp->driver_stats.busy_arr[i] = (unsigned long *)cp;
                cp += newsize_in_bytes;
            }
        }
#endif /* CONFIG_MOT_FEAT_PM_STATS */

	/* Fix the pointers in the mpm_stats_t structure. */
        mpmdip->size_in_bits = newsize;
        mpmdip->registered_p = k_registered_bitarray_addr(p, newsize);
        mpmdip->busy_p       = k_busy_bitarray_addr(p, newsize);
        mpmdip->name_arr     = k_name_ptr_array_addr(p, newsize);

	/* Allocate the first bit in the new part of the bitarray. */
	bit = oldsize;
    }

    __set_bit(bit, mpmdip->registered_p);
    mpmdip->registered_count++;

    mpmdip->name_arr[bit] = driver_name;
    mpmdip->total_name_size += namelen + 1;

    spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
#ifdef CONFIG_MOT_FEAT_PM_STATS
    spin_unlock_irqrestore(&mpmsplk, mpmsplk_flags);
#endif /* CONFIG_MOT_FEAT_PM_STATS */

    if (old_p)
	kfree(old_p);
#ifdef CONFIG_MOT_FEAT_PM_STATS
    if (old_busy_arr_p)
	kfree(old_busy_arr_p);
#endif /* CONFIG_MOT_FEAT_PM_STATS */

    return bit;
}


/*
 * mpm_unregister_with_mpm is called by any driver/module that wishes
 * to unregister with the MPM.
 *
 * The driver is marked as no longer busy when it is unregistered and
 * the count of busy drivers is updated if necessary.
 * The registered_p and busy_p bitarrays never shrink in size.
 *
 * The results are undefined if mpm_unregister_with_mpm is called in
 * an interrupt context.
 */
void mpm_unregister_with_mpm(int driver_num)
{
    unsigned long flags;
    int size_in_bits;
    char *driver_name;

    spin_lock_irqsave(&mpm_driver_info_lock, flags);

    if ((driver_num < 0) || (driver_num >= mpmdip->size_in_bits))
    {
	size_in_bits = mpmdip->size_in_bits;
        spin_unlock_irqrestore(&mpm_driver_info_lock, flags);
        printk(KERN_ERR "MPM: mpm_driver_bit_unallocate: "
			"Bad driver_num=%d, size=%d\n",
	       driver_num, size_in_bits);
	return;
    }

    if (__test_and_clear_bit(driver_num, mpmdip->busy_p))
	mpmdip->busy_count--;
    if (__test_and_clear_bit(driver_num, mpmdip->registered_p))
        mpmdip->registered_count--;

    driver_name = mpmdip->name_arr[driver_num];
    mpmdip->total_name_size -= strlen(driver_name) + 1;
    mpmdip->name_arr[driver_num] = NULL;

    spin_unlock_irqrestore(&mpm_driver_info_lock, flags);

    kfree(driver_name);
}

/*
 * mpm_driver_advise is called by any driver/module that wishes
 * to send advice to the MPM about its power management activities.
 *
 * mpm_driver_advise is not designed to handle nested calls.  The
 * last advice sent from the driver/module is the one that is in
 * effect.
 *
 * mpm_driver_advise can be called from any context.
 */
int mpm_driver_advise(int driver_num, int advice)
{
    unsigned long flags;

    spin_lock_irqsave(&mpm_driver_info_lock, flags);

    if ((driver_num < 0) || (driver_num >= mpmdip->size_in_bits))
    {
        spin_unlock_irqrestore(&mpm_driver_info_lock, flags);
        printk(KERN_ERR "MPM: mpm_driver_advise: "
			"Bad driver_num=%d, size=%d\n",
	                driver_num, mpmdip->size_in_bits);
	return -EINVAL;
    }

    if (!__test_bit(driver_num, mpmdip->registered_p))
    {
        spin_unlock_irqrestore(&mpm_driver_info_lock, flags);
        printk(KERN_ERR "MPM: mpm_driver_advise: "
			"Unallocated driver_num %d\n",
	       driver_num);
	return -EINVAL;
    }

    switch (advice)
    {
    case MPM_ADVICE_DRIVER_IS_NOT_BUSY:
        /* If the driver was busy, then decrement mpm_busy_drivers_count. */
        if (__test_and_clear_bit(driver_num, mpmdip->busy_p))
	    mpmdip->busy_count--;
        break;

    case MPM_ADVICE_DRIVER_IS_BUSY:
        /* If the driver was not busy, then increment mpm_busy_drivers_count. */
        if (!__test_and_set_bit(driver_num, mpmdip->busy_p))
	    mpmdip->busy_count++;
        break;

    default:
        spin_unlock_irqrestore(&mpm_driver_info_lock, flags);
        printk(KERN_ERR "MPM: mpm_driver_advise: Bad advice %d\n",
	       advice);
	return -EINVAL;
        break;
    }

    spin_unlock_irqrestore(&mpm_driver_info_lock, flags);

    return 0;
}

/*
 * mpm_num_busy_drivers returns number of drivers that have currently
 * advised the MPM that they are busy.
 *
 * mpm_num_busy_drivers can be called from any context.
 */
int mpm_num_busy_drivers(void)
{
    return mpmdip->busy_count;
}   

/*
 * Register/Deregister the callback routines. 
 * Functions defined in the dynamically loaded portion of MPM driver
 * need to register with the static portion of MPM driver, to be accessible
 * by other device drivers, kernel modules.
 */
void mpm_callback_register(struct mpm_callback_fns *mpm_callback_fns_ptr)
{
    mpm_handle_ioi = mpm_callback_fns_ptr->handle_ioi;
    mpm_handle_long_ioi = mpm_callback_fns_ptr->handle_long_ioi;
    mpm_periodic_jobs_started= mpm_callback_fns_ptr->periodic_jobs_started;
    mpm_periodic_jobs_done = mpm_callback_fns_ptr->periodic_jobs_done;
    mpm_set_awake_state = mpm_callback_fns_ptr->set_awake_state;
    mpm_ready_to_sleep = mpm_callback_fns_ptr->ready_to_sleep;
    mpm_start_sleep = mpm_callback_fns_ptr->start_sleep;
    mpm_resume_from_sleep = mpm_callback_fns_ptr->resume_from_sleep;
#ifdef CONFIG_MOT_FEAT_PM_STATS
    mpm_report_test_point = mpm_callback_fns_ptr->report_test_point;
#endif
}

void mpm_callback_deregister(void)
{
    mpm_handle_ioi = mpm_dummy_function;
    mpm_handle_long_ioi = mpm_dummy_function;
    mpm_periodic_jobs_started = mpm_dummy_function;
    mpm_periodic_jobs_done = mpm_dummy_function;
    mpm_set_awake_state = mpm_dummy_function;
    mpm_ready_to_sleep = mpm_dummy_function;
    mpm_start_sleep = mpm_dummy_function;
    mpm_resume_from_sleep = mpm_dummy_function;
#ifdef CONFIG_MOT_FEAT_PM_STATS
    mpm_report_test_point = mpm_test_dummy_function;
#endif
}

/*
 * Dummy function for initial value of callback routines.
 * If power management is not active, then the callback will not have been
 * registered, in which case we need to do nothing if they are called.
 */
static int mpm_dummy_function(void)
{
    return 0;
}


#ifdef CONFIG_MOT_FEAT_PM_DESENSE
/*
 * The pmdaemon gets the desense information from the BP and forwards
 * it to the MPM driver, which forwards it on to the MXC driver.  If
 * the MXC driver detects invalid data, then it returns a failure.
 * Normally, we want to panic the phone when this occurs in order to
 * force the value to be fixed.  However, because the panic occurs
 * very early in the boot sequence, the phone becomes unbootable.  In
 * order to enable the phone to boot again so that the desense value
 * can be fixed, the no_desense_panic cmdline option is provided.
 *
 * If no_desense_panic is off (and is by default), then the phone
 * panics.  If no_desense_panic is on, then the phone does not panic
 * and allows the phone to boot even if the desense data is invalid.
 */
static void __init mpm_set_no_desense_panic(char **p)
{
        if (memcmp(*p, "on", 2) == 0) {
                mpm_no_desense_panic = 1;
                *p += 2;
        } else if (memcmp(*p, "off", 3) == 0) {
                mpm_no_desense_panic = 0;
                *p += 3;
        }
}

__early_param("no_desense_panic=", mpm_set_no_desense_panic);

int mpm_panic_with_invalid_desense()
{
	return (mpm_no_desense_panic == 0);
}
#endif /* CONFIG_MOT_FEAT_PM_DESENSE */

#ifdef CONFIG_MOT_FEAT_PM_STATS
/*
 * Dummy functions for initial value of the test point callback routines.
 * If power management is not active, then the callback will not have been
 * registered, in which case we need to do nothing if they are called.
 */
static void mpm_test_dummy_function(int argc, ...)
{
}
#endif

EXPORT_SYMBOL(mpm_queue_empty);
EXPORT_SYMBOL(mpm_get_queued_event);
EXPORT_SYMBOL(mpm_event_notify);
EXPORT_SYMBOL(mpm_wq);
EXPORT_SYMBOL(mpm_periodic_jobs_started);
EXPORT_SYMBOL(mpm_periodic_jobs_done);
EXPORT_SYMBOL(mpm_handle_ioi);
EXPORT_SYMBOL(mpm_handle_long_ioi);
EXPORT_SYMBOL(mpm_set_awake_state);
EXPORT_SYMBOL(mpm_start_sleep);
EXPORT_SYMBOL(mpm_resume_from_sleep);
EXPORT_SYMBOL(mpm_register_with_mpm);
EXPORT_SYMBOL(mpm_unregister_with_mpm);
EXPORT_SYMBOL(mpm_driver_advise);
EXPORT_SYMBOL(mpm_num_busy_drivers);
EXPORT_SYMBOL(mpm_get_extra_size);
EXPORT_SYMBOL(mpmdip);
EXPORT_SYMBOL(mpm_driver_info_lock);
EXPORT_SYMBOL(mpm_callback_register);
EXPORT_SYMBOL(mpm_callback_deregister);
#ifdef CONFIG_MOT_FEAT_PM_DESENSE
EXPORT_SYMBOL(mpm_panic_with_invalid_desense);
#endif /* CONFIG_MOT_FEAT_PM_DESENSE */
#ifdef CONFIG_MOT_FEAT_PM_STATS
EXPORT_SYMBOL(mpm_report_test_point);
EXPORT_SYMBOL(mpmsp);
EXPORT_SYMBOL(mpmsplk);
#endif
