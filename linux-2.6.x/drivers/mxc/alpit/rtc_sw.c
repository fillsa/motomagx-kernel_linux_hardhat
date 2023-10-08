/*
 *  rtc_sw.c - EZX RTC Stopwatch
 *
 *  Copyright (C) 2006-2008 Motorola, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Adds ability to program periodic interrupts from user space that
 *  can wake the phone out of low power modes.
 *
 *  Note: At the time of implementation this driver was used by a small
 *  number of applications (at most 5). The initial implementation was based
 *  heavily on the EZX version of this driver for the purposes of keeping the
 *  API uniform and also allowing us to quickly merge changes, etc... If 
 *  driver is used on a larger scale some optimizations and potential API
 *  changes should be looked into:
 *   - The poll and read interfaces can be modified to exhibit more normal
 *     behavior. Currently the read interface is used interchangeably with
 *     the poll interface by applications waiting for notification that their
 *     job is ready to be completed.
 *   - The driver is implemented with a doubly linked list. It could easily
 *     be converted to use the Linux standard for singly linked lists.
 *   - Due to the way current is used throughout this driver it is not
 *     thread safe.
 */

/*
 * DATE          AUTHOR         COMMENT
 * ----          ------         --------
 * 10/04/2006    Motorola       Initial version, heavily based on EZX RTC 
 *                              Stopwatch code.
 * 11/08/2006    Motorola       Added kernel API.
 * 11/15/2006    Motorola       Minor Bug Fix.
 * 12/08/2006    Motorola       Fixed Power Management usage in RTC Stopwatch driver
 * 01/04/2007    Motorola       Power management support to account for time in DSM
 * 02/23/2007    Motorola       Bug fix: Incorrectly updated clock when resume was
 *                              called without a suspend powering down the phone.
 * 03/07/2007	 Motorola	Update fuzzy timer implementation to match EZX
 * 03/08/2007    Motorola       Replaced do_settimeofday() with
 *				set_normalized_timespec() in rtcsw_resume().
 * 03/16/2007    Motorola       Fixed incorrect wraparound handlings.
 * 03/27/2007    Motorola       Added fix to maintain interrupt state in resume
 *                              routine.
 * 04/29/2007    Motorola       Added support for GPT running at 32KHz.
 * 10/25/2007    Motorola       Improved periodic job state collection for debug.
 * 11/13/2007    Motorola	Resloved display time in CLI is one min delayed from 
 * 				phone clock, re-align to xtime when xtime changed
 * 				somewhere.	
 * 11/15/2007    Motorola       Upmerge from 6.1.(re-align to xtime when xtime changed somewhere.)
 * 12/3/2007     Motorola       Added API to get closest RTC timeout
 * 12/17/2008    Motorola       Calculate the time drift and make the time accurate.
 */


/*!
 * Design:
 * 
 * The goal of this driver is to provide a means for applications and kernel code to
 * receive an event even when the device is suspended.  This functionality is similar
 * to /dev/rtc but with a much higher accuracy then is offered by /dev/rtc, with a
 * kernel interface, and in conformance with mpm power management.
 *
 * The power management provided by mpm is more aggressive at suspending the device then
 * is normally done.  This driver's interface has the concept of a periodic job.
 * A periodic job is one that must be performed even if the device has been suspended,
 * but will put the device back into a suspended state when it is done.
 * Because of this after each event has been triggered the driver must be informed that
 * it may go back to sleep.  For the user space interface this must be done explicitly.
 * For the kernel interface is it assumed that once the callback function has finished
 * running that the device may be suspended again.  If this is not the case then the
 * kernel code using the rtc_sw APIs must block the suspend from happening themselves.
 * To see how to do this please refer to the documentation of mpm.
 * 
 * Synchronization is rather complex in this driver because we want to allow the user to
 * both cancel and restart a timer from interrupt context.  To do this we will use a
 * transaction counter system to flag a timer when it has been canceled and is invalid,
 * and then if we cannot handle that cancellation right now to process it when the timer
 * would actually have gone off.
 */


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/errno.h>
#include <linux/timex.h>
#include <linux/signal.h>
#include <linux/time.h>
#include <linux/mpm.h>
#include <linux/kernel.h>
#include <linux/device.h>

#include <asm/bitops.h>
#include <asm/delay.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/div64.h>
#include <asm/arch/clock.h>
#include <asm/arch/epit.h>
#include <asm/arch/timex.h>
#include <asm/mach/time.h>

#include "rtc_sw.h"

/*
 * Protects access to all shared data.  Shared data includes all initialized tasks.
 */
static spinlock_t request_lock;

/*
 * Head of stopwatch request list.
 * This is protected by request_lock
 */
static struct rtc_sw_request *rtc_sw_request_head = NULL;
static DECLARE_WAIT_QUEUE_HEAD(rtc_sw_wait);

/*
 * Holds previously read EPIT value.
 */
static unsigned long epit_last = 0;

/*
 * Holds the the number of ticks seen from when the epit was enabled.
 */
static unsigned long ticks_recorded = 0;

/*
 * Holds the number of periodic jobs running only when this reaches 0 will we allow
 * the device to suspend.
 */
static int jobs_running = 0;

/*
 * RTC Stopwatch timer types
 */
enum {
    RTC_SW_STRICT = 0,
    RTC_SW_FUZZ = 1,
    RTC_SW_STRICT_TASKLET = 2
};

/**
 * This is used to indicate when we remove a request from the list if that
 * request is being canceled or if it will be left running.
 */
typedef enum {
    LEAVE_RUNNING = 0,
    CANCEL = 1
} job_canceled;

static void rtc_sw_task_tasklet_callback(unsigned long data);
static unsigned long rtc_sw_task_internal_stop(rtc_sw_task_t * task);
static int rtcsw_suspend(struct device *dev, u32 state, u32 level);
static int rtcsw_resume(struct device *dev, u32 level);

static struct device_driver rtcsw_driver = {
	.name = "rtc_stopwatch",
	.bus = &platform_bus_type,
	.suspend = rtcsw_suspend,
        .probe = NULL,
        .remove = NULL,
	.resume = rtcsw_resume,
};

static void rtcsw_release(struct device *dev)
{
        /* do nothing */
}

static struct platform_device rtcsw_device = {
        .name = "rtc_stopwatch",
        .id = 0,
        .dev = {
                .release = rtcsw_release,
        },
};

static unsigned int rtc_sw_poll(struct file *file, poll_table *wait)
{
        struct rtc_sw_request *req = NULL;
        int data = 0;

        /* add wait queue to poll_table structure */
        poll_wait(file, &rtc_sw_wait, wait);

        spin_lock_irq(&request_lock);

        req = rtc_sw_request_head;

        /* check if current process's request has expired */
        while (req != NULL) {
            if ((req->type != RTC_SW_STRICT_TASKLET) &&
                (req->callback_data.process == current) &&
                (req->running > 0)) {
                /* request is ready to be handled */
                req->running--;
                req->pending++;
#ifdef CONFIG_MOT_FEAT_PM_STATS
                mpm_collect_pj_stat(1, current->comm, current->pid);
#endif
                RTC_SW_DPRINTK("\nEPIT: %s() %d - \"%s\" running - %d pending - %d\n", __FUNCTION__, current->pid, current->comm,req->running, req->pending ); 
                data = 1;
                break;
            }
            req = req->next;
        }
        
        spin_unlock_irq(&request_lock);
        
        return (data) ? (POLLIN | POLLRDNORM) : 0;
}

static loff_t rtc_sw_llseek(struct file *file, loff_t offset, int origin)
{
        return -ESPIPE;
}

ssize_t rtc_sw_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    DECLARE_WAITQUEUE(wait, current);
    struct rtc_sw_request *req = NULL;
    unsigned long data = 0;
    int ret = 0;
    
    add_wait_queue(&rtc_sw_wait, &wait);
    
    spin_lock_irq(&request_lock);
    
 repeat:
    set_current_state(TASK_INTERRUPTIBLE);
    
    req = rtc_sw_request_head;
    
    while (req != NULL) {
        /* check if current process's request has expired */
        if ((req->type != RTC_SW_STRICT_TASKLET) &&
            (req->callback_data.process == current) &&
            (req->running > 0)) {
            /* request is ready to be handled */
            
            req->running--;
            req->pending++;
            RTC_SW_DPRINTK("\nEPIT: %s() %d - \"%s\" running - %d pending - %d\n", __FUNCTION__, current->pid, current->comm,req->running, req->pending ); 
            data = 1;
            break;
        }
        req = req->next;
    }
    
    if (data != 1) {
        /* no requests are ready to be handled */
        
        if (file->f_flags & O_NONBLOCK) {
            ret = -EAGAIN;
            spin_unlock_irq(&request_lock);
            goto out;
        }
        if (!signal_pending(current)) {
            spin_unlock_irq(&request_lock);
            schedule();
            spin_lock_irq(&request_lock);
            goto repeat;
        }
        else {
            ret = -ERESTARTSYS;
            spin_unlock_irq(&request_lock);
            goto out;
        }
    }
    
    spin_unlock_irq(&request_lock);
    
    if(count >= 4) {
        ret = put_user(data, (unsigned long *)buf);
        if (!ret) {
            ret = sizeof(unsigned long);
        }
    }
    
 out:
    set_current_state(TASK_RUNNING);
    remove_wait_queue(&rtc_sw_wait, &wait);
    
    return ret;
}

/*
 * Takes the request pointed to by *req and removes it from the
 * request list. We assume that the caller has grabbed the request_lock. 
 *
 * If cancel is CANCEL then it is assumed that req has been canceled, and
 * jobs_running will be updated accordingly.
 */
static void detach_from_list(struct rtc_sw_request *req, job_canceled cancel)
{

    if (req == NULL) {
        RTC_SW_DPRINTK(KERN_WARNING "Trying to detach NULL request.\n");
        return;
    }
    else if (req->prev == NULL) {
        /* at head of list */
        rtc_sw_request_head = req->next;
        if (req->next != NULL) {
            req->next->prev = NULL;
        }
    }
    else if ((req->prev != NULL) && (req->next == NULL)) {
        /* request is in last position */
        req->prev->next = NULL;
    }
    else { /* ((req->prev != NULL) && (req->next != NULL)) */
        /* request is in the middle of the list */
        req->prev->next = req->next;
        req->next->prev = req->prev;
    }
    
    /* The request should be disconnected. */
    req->next = NULL;
    req->prev = NULL;
    
    if(cancel == CANCEL) {
        /*We are canceling job as we remove it.*/

        atomic_inc(&req->transaction);
        jobs_running -= req->running;
        req->running = 0;
        jobs_running -= req->pending;
        req->pending = 0;
        RTC_SW_DPRINTK("\nEPIT: %s() canceled running job jobs_running - %d\n", __FUNCTION__, jobs_running);
    }
}

/*
 * Allocate a request.  Used by the device file interface.
 */
static struct rtc_sw_request *request_create(void)
{
    struct rtc_sw_request *req;

    req = (struct rtc_sw_request *)kmalloc(sizeof(struct rtc_sw_request),
                                           GFP_KERNEL);

    if (req == NULL) {
        printk(KERN_WARNING "Attempt to malloc request failed.\n");
    }
    
    return req;
}

/* 
 * Detach this process's request from the list and return it.
 * If no request is found return NULL. Assume the caller has the request lock.
 *
 * This is only called by the device file interface.
 */
static struct rtc_sw_request *request_del_process(struct task_struct *p)
{
    struct rtc_sw_request *req = NULL;

    if (p == NULL) {
        return NULL;
    }

    req = rtc_sw_request_head;

    while (req != NULL)  {
        if ((req->type != RTC_SW_STRICT_TASKLET) &&
            req->callback_data.process == p) {
            /* If request matches current process, detach it */
            detach_from_list(req, CANCEL);
            break;
        }
        req = req->next;
    }

    return req;
}

/*
 * Detach this process's request from the list that matches the given
 * time, and return the request. If no request is found, return NULL.
 * Assume the caller has the request lock.
 *
 * This is only called by the device file interface.
 */
static struct rtc_sw_request *request_del_time(unsigned long ticks)
{
    struct rtc_sw_request *req;
    
    if (!ticks) {
        RTC_SW_DPRINTK(KERN_WARNING "Looking for request with 0 interval.\n");
        return NULL;
    }
    
    req = rtc_sw_request_head;
    
    while (req != NULL) {
        if ((req->type != RTC_SW_STRICT_TASKLET) &&
            (req->callback_data.process == current) &&
            (req->interval == ticks)) {
            /* If request matches ticks values,
               detach it. */
            detach_from_list(req, CANCEL);
            break;
        }
        req = req->next;
    }
    
    return req;
}

/*
 * Add a request to the request list. Insert it into the front. Assume
 * we have the request lock.
 */
static void request_add(struct rtc_sw_request *req)
{
    req->prev = NULL;

    if (rtc_sw_request_head == NULL) {
        req->next = NULL;
    }
    else {
        rtc_sw_request_head->prev = req;
        req->next = rtc_sw_request_head;
    }
    
    rtc_sw_request_head = req;
}

/*
 * Check rtc_sw_time passed by user to make sure fields are within
 * valid boundaries. We don't support intervals less than or equal to
 * one hundredth of a second.
 */
static int rtc_sw_time_check(struct rtc_sw_time *rtime)
{
    unsigned int hr, min, sec, hun;
    
    if (rtime == NULL) {
        RTC_SW_DPRINTK(KERN_WARNING "rtc_sw_time is NULL\n");
        return -1;
    }
    
    hr  = rtime->hours;
    min = rtime->minutes;
    sec = rtime->seconds;
    hun = rtime->hundredths;
    
    /* check the values passed in */
    if ((hr > 23) || (min > 59) || (sec > 59) || (hun > 99)) {
        printk(KERN_WARNING "invalid rtc_sw_time\n");
        return -1;
    }
    
    /* we do not support zero time intervals */
    if ((hr == 0) && (min == 0) && (sec == 0) && (hun == 0)) {
        return -1;
    }
    
    /* the rtc_sw_time struct was valid */
    return 0;
}

static unsigned long inline ms_to_ticks(unsigned long ms)
{
    unsigned long long tmp = (unsigned long long)ms;
    /*EPIT_COUNTER_TICK_RATE is ticks per second => ticks/sec * milli sec == milli ticks*/
    tmp *= EPIT_COUNTER_TICK_RATE;
    
    do_div(tmp, 1000);        
    return (unsigned long)tmp;
}

static unsigned long inline ticks_to_ms(unsigned long ticks)
{
    unsigned long long tmp = (unsigned long long)ticks;
    
    return (unsigned long)((tmp * 1000) / EPIT_COUNTER_TICK_RATE);
}

static unsigned long inline drift_in_ns(unsigned long ticks)
{
    unsigned long long tmp  = (unsigned long long)ticks;

    return (unsigned long)(((tmp * 1000) % EPIT_COUNTER_TICK_RATE) * 1000000 / EPIT_COUNTER_TICK_RATE);                                                                                                                                                         
}

static unsigned long inline hundredths_to_ticks(unsigned long hun)
{
    unsigned long long tmp = (unsigned long long)hun;
    /*EPIT_COUNTER_TICK_RATE is ticks per second => ticks/sec * hun sec == hun ticks*/
    tmp *= EPIT_COUNTER_TICK_RATE;
    
    do_div(tmp, 100);
    return (unsigned long)tmp;
}

static unsigned long inline ticks_to_hundredths(unsigned long ticks)
{
    unsigned long long tmp = (unsigned long long)ticks;
    
    return (unsigned long)((tmp * 100) / EPIT_COUNTER_TICK_RATE);
}

/*
 * Set EPIT Compare register to new value. Value passed in is in ticks 
 * and is an offset to the current EPITCNR value. Assume we have the request lock.
 */
static void set_rtc_sw_reg(unsigned long ticks)
{
    if (ticks == 0) {
        /*I think we need to handle this situation better then we currently are.*/
        RTC_SW_DPRINTK(KERN_WARNING "Offset can't be zero.\n");
    }
    
    /*It is a count down counter do subtract the number of ticks*/
    *_reg_EPIT_EPITCMPR = *_reg_EPIT_EPITCNR - ticks;
}

/*
 * Get the time elapsed since the last time we read the EPIT counter.
 * Return value in ticks. Assume the caller has grabbed the request lock.
 */
static unsigned long get_slice_passed(void)
{
    unsigned long ticks_diff;

    ticks_diff = epit_last - *_reg_EPIT_EPITCNR;
    epit_last = *_reg_EPIT_EPITCNR;

    ticks_recorded += ticks_diff;
    
    return ticks_diff;
}

/*
 * Update the request passed in by waking it up or subtracting the time 
 * elapsed. Assume we are holding the request lock.
 */
static void inline update_one_request(struct rtc_sw_request *req, unsigned long ticks)
{
    if (req->remain <= ticks) {
        /*It is being run, so it can no longer be the next to run, this should really be from the 
         order of the list*/
        req->next_to_run = false;
        req->running++;
        jobs_running++;

        atomic_set(&req->cookie, atomic_inc_return(&req->transaction));

        RTC_SW_DPRINTK("\nEPIT: %s() started new job jobs_running - %d req->running - %d\n", __FUNCTION__, jobs_running, req->running);

        if(req->type == RTC_SW_STRICT_TASKLET) {
            detach_from_list(req,LEAVE_RUNNING);

            tasklet_schedule(&req->callback_data.task_data.task);
            req->remain = 0;
#ifdef CONFIG_MOT_FEAT_PM_STATS
            mpm_collect_pj_stat((unsigned long)req->callback_data.task_data.orig_func, \
                                "NULL", 1);
#endif
        } else {
            RTC_SW_DPRINTK("\nEPIT: %s() waking up process %d - \"%s\"\n", __FUNCTION__, req->callback_data.process->pid, req->callback_data.process->comm);
            /* wake up the process */
            wake_up_process(req->callback_data.process);
            req->remain = req->interval;
#ifdef CONFIG_MOT_FEAT_PM_STATS
            mpm_collect_pj_stat(1, req->callback_data.process->comm, \
                                req->callback_data.process->pid);
#endif
        }
    } else {
        /* subtract the time that has elapsed */
        req->remain -= ticks;
    }
}

/*
 * Update all requests in the list, decreasing them by the time that has
 * elapsed. Assume we are holding the request lock.
 */
static void update_request_list(unsigned long ticks)
{
    struct rtc_sw_request *req = NULL;
    struct rtc_sw_request *next = NULL;

    req = rtc_sw_request_head;

    if (req == NULL) {
        RTC_SW_DPRINTK(KERN_WARNING "\nEPIT: %s() No requests in list\n",__FUNCTION__);
    }

    while (req != NULL)  {
        next = req->next;
        update_one_request(req, ticks);
        req = next;
    }
}

/*
 * Get the request with the least remaining time before expiring.
 * Assume the caller holds the request lock.
 */
static struct rtc_sw_request *get_lowest_request(void)
{
    struct rtc_sw_request *req = NULL;
    struct rtc_sw_request *lowest = NULL;

    req =  rtc_sw_request_head;

    /* make an initial selection for the request with the least
       time remaining */
    if (req != NULL) {
        lowest = req;
        req = req->next;
    }

    while (req != NULL) {
        /* if we find something that has to go off sooner than
           lowest, replace lowest */
        if (req->remain < lowest->remain) {
            lowest = req;
        }
        req = req->next;
    }

    return lowest;
}

/*
 * Take the request passed in and have its value programmed into the EPIT 
 * Compare register. This should be called on the request with the lowest 
 * remaining interval time. Assume the caller holds the request lock.
 */
static void put_request_in_use(struct rtc_sw_request *req)
{
    if(req != NULL) {
        /* clear any old interrupts */
        *_reg_EPIT_EPITSR = 1;

        /* program new value into EPIT Compare register */
        set_rtc_sw_reg(req->remain);
        wmb();
        
        req-> next_to_run = 1;
    }
}

/*
 * If a request was deleted that was currently in use we need to reprogram
 * the EPIT to a new value. Assume the caller has grabbed the request lock.
 */
static void find_new_request(void) 
{
    struct rtc_sw_request *req;
    unsigned long slice;

    if (rtc_sw_request_head != NULL) {
        /* Disable timer */
        *_reg_EPIT_EPITCR &= ~EPITCR_EN;
        

        /* There are still requests left. Set a valid value
           in the compare register for the next interrupt. */
        slice = get_slice_passed();
        if (slice) {
            update_request_list(slice);
        }

        req = get_lowest_request();
        put_request_in_use(req);
        
        /* resume the counter */
        *_reg_EPIT_EPITCR |= EPITCR_EN;
    }
    
}

/*
 * Get the least common multiple.
 */
static unsigned long get_least_multiple(unsigned long first, unsigned long second)
{
    unsigned long num1, num2;
    u64 result;
    unsigned long temp;

    if (second >= first) {
        temp = second;
        second = first;
        first = temp;
    }

    num1 = first;
    num2 = second;

    while (second != 0) {
        temp = first % second;
        first = second;
        second = temp;
    }

    if (first != 0) {
        result = num1 / first;
        result = result * (u64)num2;
    } else {
        result = -1;
    }

    return result;
}

/*
 * Grab the first fuzzy request we see. Assume we have the request lock.
 */
static inline struct rtc_sw_request *get_fuzz_req(void)
{
    struct rtc_sw_request *req = NULL;
    struct rtc_sw_request *fuzz = NULL;

    req = rtc_sw_request_head;

    while (req != NULL) {
        if (req->type == RTC_SW_FUZZ) {
            fuzz = req;
            break;
        }
        req = req->next;
    }
    
    return fuzz;
}

/*
 * Grab the most closely aligned fuzzy request. Assume we have the request lock, least multiple version.
 */
static inline struct rtc_sw_request *get_fuzz_req_least_multiple(unsigned long interval)
{
    unsigned long lowest = LONG_MAX;
    u64 temp;
    struct rtc_sw_request *req = NULL;
    struct rtc_sw_request *fuzz = NULL;

    req = rtc_sw_request_head;

    while (req != NULL) {
        if (req->type == RTC_SW_FUZZ) {
            temp =  get_least_multiple(req->interval/10000, interval/10000);
            if (temp < lowest) {
                    lowest = temp;
            fuzz = req;
            }
        }
        req = req->next;
    }
    
    return fuzz;
}

/*
 * When an application exits, this function is called to delete any
 * request nodes that were created for it.
 */ 
static void rtc_sw_app_exit(void)
{
    struct rtc_sw_request *req = NULL;
    int request_was_in_use = 0;

    spin_lock_irq(&request_lock);

    /* remove all requests that belong to this process */
    while ((req = request_del_process(current)) != NULL) {
        if (req->next_to_run) {
            request_was_in_use = 1;
        }
        FREE_REQUEST(req);
    }

    if (request_was_in_use) {
        find_new_request();
    }

    spin_unlock_irq(&request_lock);
}

int rtc_sw_periodic_jobs_running(void)
{
    if(jobs_running > 0) {
        return PJ_RUNNING;
    } else {
        return PJ_NOT_RUNNING;
    }
}

unsigned long rtc_sw_msfromboot(void)
{
    unsigned long ticks_from_boot =  ticks_recorded;
    ticks_from_boot += epit_last - *_reg_EPIT_EPITCNR;
    return ticks_to_ms(ticks_from_boot);
}

EXPORT_SYMBOL(rtc_sw_msfromboot);

unsigned long rtc_sw_get_internal_ticks(void)
{
    return *_reg_EPIT_EPITCNR;
}

EXPORT_SYMBOL(rtc_sw_get_internal_ticks);

unsigned long rtc_sw_internal_to_ms(unsigned long ticks)
{
    return ticks_to_ms(ticks);
}
EXPORT_SYMBOL(rtc_sw_internal_to_ms);

/*
 * rtc_sw_fuzz_req_realign
 *
 * This function is called when xtime is changed somewhere else and
 * consequently the fuzz request originally alignment is broken.
 *
 * We then use this function to re-align all the fuzz requests to the
 * new xtime.
 */
void rtc_sw_fuzz_req_realign(void)
{
        unsigned long slice;
        unsigned long hundredths;
        unsigned long tmp;
        struct rtc_sw_request *req = NULL;
        struct rtc_sw_request *fuzz_list = NULL;

       spin_lock_irq(&request_lock);
        *_reg_EPIT_EPITCR &= ~EPITCR_EN;

        /* if no fuzz req, return quickly */
        req = get_fuzz_req();
        if (req == NULL)
                goto out;

        /* Get interval since the last time we updated the
         * request list. If the list is empty slice is 0. */
        slice = get_slice_passed();

        /* update the list before we insert another request */
        if (slice) {
                update_request_list(slice);
        }

      /* kick out the fuzz requests to fuzz_list */
        while (req) {
                detach_from_list(req, LEAVE_RUNNING);
                req->next = fuzz_list;
                fuzz_list = req;

                req = get_fuzz_req();
        }

        /* re-align each req in fuzz_list, and re-insert the req */
        hundredths = (60 * 100) - ((xtime.tv_sec % 60) * 100 +
                        xtime.tv_nsec / 10000000);
        tmp = hundredths_to_ticks(hundredths);


        req = fuzz_list;
        while (fuzz_list) {
                fuzz_list = req->next;

                req->remain = tmp % req->interval;
                request_add(req);
                req = fuzz_list;
        }

        /* reschedule the next stopwatch interrupt */
        /* req is now the next request that needs to be scheduled */
        req = get_lowest_request();

        put_request_in_use(req);

out:
        /* resume the counter */
        *_reg_EPIT_EPITCR |= EPITCR_EN;
        spin_unlock_irq(&request_lock);
}








static int rtc_sw_ioctl(struct inode *inode, struct file *file,
                        unsigned int cmd, unsigned long arg)
{
    struct rtc_sw_request *req = NULL;
    struct rtc_sw_request *fuzz = NULL;
    struct rtc_sw_time tm;
    unsigned long hundredths;
    unsigned long ticks;
    unsigned long slice;
    unsigned long ms_from_boot;
    int request_was_in_use = 0;
    
    switch (cmd) {
    case RTC_SW_SETTIME:
        
        if (copy_from_user(&tm, (struct rtc_sw_time *)arg,
                           sizeof(struct rtc_sw_time))) {
            return -EFAULT;
        }
        
        if (rtc_sw_time_check(&tm)) {
            RTC_SW_DPRINTK(KERN_WARNING "rtc_sw_time invalid\n");
            return -EINVAL;
        }
        
        hundredths = HOURS_TO_HUNDREDTHS(tm.hours) +
            MINUTES_TO_HUNDREDTHS(tm.minutes) +
            SECONDS_TO_HUNDREDTHS(tm.seconds) +
            tm.hundredths;
        
        ticks = hundredths_to_ticks(hundredths);
        
        req = request_create();
        
        if(req == NULL) {
            RTC_SW_DPRINTK(KERN_WARNING "request_create failed\n");
            return -ENOMEM;
        }
        
        /* initialize request before we add it to the list */
        req->interval = ticks;
        req->remain = req->interval;
        req->type = RTC_SW_STRICT;

        atomic_set(&req->transaction, 0);
        atomic_set(&req->cookie, -1);
        req->running = 0;
        req->pending = 0;
        req->next_to_run = 0;
        req->callback_data.process = current;
        
        spin_lock_irq(&request_lock);
        
        *_reg_EPIT_EPITCR &= ~EPITCR_EN;
        
        /* Get interval since the last time we updated the
         * request list. If the list is empty slice is 0. */
        slice = get_slice_passed();
        
        /* update the list before we insert another request */
        if (slice) {
            update_request_list(slice);
        }
        
        request_add(req);
        
        /* reschedule the next stopwatch interrupt */
        /* req is now the next request that needs to be scheduled */
        req = get_lowest_request();
        
        put_request_in_use(req);
        
        /* resume the counter */
        *_reg_EPIT_EPITCR |= EPITCR_EN;
        
        spin_unlock_irq(&request_lock);
        
        return 0;
        
    case RTC_SW_SETTIME_FUZZ:
        if (copy_from_user(&tm, (struct rtc_sw_time *)arg,
                           sizeof(struct rtc_sw_time))) {
            return -EFAULT;
        }
        
        if (rtc_sw_time_check(&tm)) {
            RTC_SW_DPRINTK(KERN_WARNING "rtc_sw_time invalid\n");
            return -EINVAL;
        }
        
        hundredths = HOURS_TO_HUNDREDTHS(tm.hours) +
            MINUTES_TO_HUNDREDTHS(tm.minutes) +
            SECONDS_TO_HUNDREDTHS(tm.seconds) +
            tm.hundredths;
        
        ticks =  hundredths_to_ticks(hundredths);
        
        req = request_create();
        
        if(req == NULL) {
            RTC_SW_DPRINTK(KERN_WARNING "request_create failed\n");
            return -ENOMEM;
        }
        
        /* initialize request before we add it to the list */
        /* Set request status to APP_SLEEP_READY to allow the
         * phone to suspend to DSM before its timer has expired.
         */
        req->interval = ticks;
        req->remain = req->interval;
        req->type = RTC_SW_FUZZ;

        atomic_set(&req->transaction, 0);
        atomic_set(&req->cookie, -1);
        req->running = 0;
        req->pending = 0;
        req->next_to_run = 0;
        req->callback_data.process = current;

        spin_lock_irq(&request_lock);
        
        /* check if EPITCR is running, disable it if it is */
        *_reg_EPIT_EPITCR &= ~EPITCR_EN;
        
        /* Get interval since the last time we updated the
         * request list. If the list is empty slice is 0. */
        slice = get_slice_passed();
        
        /* update the list before we insert another request */
        if (slice) {
            update_request_list(slice);
        }
        
        /* check if there is a fuzzy request in the list */
        fuzz = get_fuzz_req_least_multiple(req->interval); //  * 03/07/2007	 Motorola	Update fuzzy timer implementation to match EZX
        
        if (fuzz != NULL) {
            /* sync with the previous fuzzy request */
            req->remain = fuzz->remain % req->interval;
        }
        else {
            /* Align the request to next minute.
             * This is done for the case of a CLI fuzzy request.
             * The CLI application will want to wake up on the
             * turn of each minute to update the clock. */
            /* FIXME: no protection for xtime changing later */
            hundredths = (60 * 100) - ((xtime.tv_sec % 60) * 100 +
                                       xtime.tv_nsec / 10000000);
             req->remain = hundredths_to_ticks(hundredths) % req->interval;
        }
        
        request_add(req);
        
        /* reschedule the next stopwatch interrupt */
        /* req is now the next request that needs to be scheduled */
        req = get_lowest_request();
        
        put_request_in_use(req);
        
        /* resume the counter */
        *_reg_EPIT_EPITCR |= EPITCR_EN;
        
        spin_unlock_irq(&request_lock);
        
        return 0;
        
    case RTC_SW_DELTIME:
        if (copy_from_user(&tm, (struct rtc_sw_time *)arg,
                           sizeof(struct rtc_sw_time))) {
            return -EFAULT;
        }
        
        if (rtc_sw_time_check(&tm)) {
            RTC_SW_DPRINTK(KERN_WARNING "rtc_sw_time invalid\n");
            return -EINVAL;
        }
        
        hundredths = HOURS_TO_HUNDREDTHS(tm.hours) +
            MINUTES_TO_HUNDREDTHS(tm.minutes) +
            SECONDS_TO_HUNDREDTHS(tm.seconds) +
            tm.hundredths;
        
        ticks = hundredths_to_ticks(hundredths);
        
        spin_lock_irq(&request_lock);
        
        /* 
         * Remove all requests belonging to this process that
         * match the given time.
         */
        while ((req = request_del_time(ticks)) != NULL) {
            if (req->next_to_run) {
                request_was_in_use = 1;
            }
            FREE_REQUEST(req);
        }
        
        if (request_was_in_use) {
            find_new_request();
        }
        
        spin_unlock_irq(&request_lock);
        
        return 0;
        
    case RTC_SW_JOB_DONE:
        spin_lock_irq(&request_lock);
        
        RTC_SW_DPRINTK("\nEPIT: %s() RTC_SW_JOB_DONE called from %d - \"%s\" \n", __FUNCTION__, current->pid, current->comm);
        
#ifdef CONFIG_MOT_FEAT_PM_STATS 
        mpm_collect_pj_stat(0, current->comm, current->pid);
#endif

        req = rtc_sw_request_head;
        
        /* set the APP_SLEEP_READY status bit */
        /* also check to see if we can send a sleep message to PM */
        while (req != NULL ) {
            if(req->type != RTC_SW_STRICT_TASKLET) {
                if (req->callback_data.process == current ) {
                    RTC_SW_DPRINTK("\nEPIT: %s() done found a req for the process %d | pending %d\n",__FUNCTION__, current->pid, req->pending);
                    if(req->pending > 0) {
                        jobs_running -= req->pending;
                        req->pending = 0;
                        RTC_SW_DPRINTK("\nEPIT: %s() done ioctl on process %d - \"%s\" jobs running %d\n", __FUNCTION__, current->pid, current->comm, jobs_running);
                        break;
                    }
                }
            }
            req = req->next;
        }
        
        RTC_SW_DPRINTK("\nEPIT: %s() jobs running %d \n", __FUNCTION__, jobs_running);

        if(rtc_sw_periodic_jobs_running() == PJ_NOT_RUNNING) {
            RTC_SW_DPRINTK("\nEPIT: %s() Done Going to sleep\n", __FUNCTION__);
            mpm_periodic_jobs_done();
        }
        
        spin_unlock_irq(&request_lock);
        
        return 0;
        
    case RTC_SW_APP_EXIT:
        /* Delete all requests that the user has made */
        rtc_sw_app_exit();
        
        return 0;
        
    case RTC_SW_MS_FROM_BOOT:
        /*Get the number of ms from boot*/
        ms_from_boot = rtc_sw_msfromboot();
        return put_user(ms_from_boot, (unsigned long *)arg);
    default:
        RTC_SW_DPRINTK("EPIT: %s() invalid ioctl cmd %x\n", __FUNCTION__ , cmd);
        
        return -EINVAL;
    }
}

static irqreturn_t 
rtc_sw_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    unsigned long slice;
    struct rtc_sw_request *req;
    
    RTC_SW_DPRINTK("\nEPIT: %s()\n", __FUNCTION__);
    
    /*We do not have to return PJ_RUNNING from rtc_sw_periodic_jobs_running at this time.
      only after the interrupt returns.*/
    mpm_periodic_jobs_started();
    
    spin_lock(&request_lock);
    
    /* stop counting */
    *_reg_EPIT_EPITCR &= ~EPITCR_EN;
    
    /* clear interrupt source */
    *_reg_EPIT_EPITSR = 1;
    
    /* Figure out how much time has passed since the last
     * time we updated the request list */
    slice = get_slice_passed();
    if (slice) {
        update_request_list(slice);
    }
    else {
        RTC_SW_DPRINTK(KERN_WARNING "EPIT: %s() rtc_sw_interrupt with slice 0\n", __FUNCTION__ );
    }
    
    /* Find the request in our list with the smallest remaining
     * interval. Program the EPIT Compare register with that
     * value. */
    req = get_lowest_request();
    if (req != NULL) {
        set_rtc_sw_reg(req->remain);
        wmb();
        
        req->next_to_run = 1;
    }
    
    /* restart the clock */
    *_reg_EPIT_EPITCR |= EPITCR_EN;
    
    if(rtc_sw_periodic_jobs_running() == PJ_NOT_RUNNING) {
        /*There are no jobs going to be run, this is really the case when
          The interrupt went off when it probably should not have.*/
        
        RTC_SW_DPRINTK("\nEPIT: %s() Done Going to sleep\n", __FUNCTION__);
        mpm_periodic_jobs_done();
    }
    
    spin_unlock(&request_lock);
    
    return IRQ_HANDLED;
}

static struct irqaction rtc_sw_irq = {
        .name           = RTC_SW,
        .handler        = rtc_sw_interrupt,
        .flags          = SA_INTERRUPT
};

static int rtc_sw_open(struct inode *inode, struct file *file)
{
        /* Do nothing. Multiple opens are accepted. */
        return 0;
}

static int rtc_sw_release(struct inode *inode, struct file *file)
{
        /* Delete any requests the user has made. */
        rtc_sw_app_exit();

        return 0;
}

void rtc_sw_task_init(rtc_sw_task_t * ptask, void (*func)(unsigned long data), unsigned long data)
{
    /* initialize request before we add it to the list */
    ptask->interval = 0;
    ptask->remain = 0;

    ptask->type = RTC_SW_STRICT_TASKLET;

    atomic_set(&(ptask->transaction),0);
    atomic_set(&(ptask->cookie),-1);
    ptask->running = 0;
    ptask->pending = 0;
    ptask->next_to_run = 0;

    ptask->next = NULL;
    ptask->prev = NULL;

    ptask->callback_data.task_data.orig_data = data;
    ptask->callback_data.task_data.orig_func = func;
    tasklet_init(&(ptask->callback_data.task_data.task), rtc_sw_task_tasklet_callback, (unsigned long)ptask);
}
EXPORT_SYMBOL(rtc_sw_task_init);

void rtc_sw_task_kill(rtc_sw_task_t * task)
{
    unsigned long time_left;
    spin_lock_irq(&request_lock);
    time_left = rtc_sw_task_internal_stop(task);
    spin_unlock_irq(&request_lock);
    if(time_left == 0) {
        /*tasklet_kill cannot be called while the spinlock is held because
          it could result in a dead lock.  This is because, it might block while
          waiting for the tasklet function to finish, and the tasklet function will
          grab the lock*/
        tasklet_kill(&(task->callback_data.task_data.task));
    }
}
EXPORT_SYMBOL(rtc_sw_task_kill);


/**
 * Stop the task from running.  It is assumed that the request_lock is held
 */
static unsigned long rtc_sw_task_internal_stop(rtc_sw_task_t * task)
{
    struct rtc_sw_request *req = task;
    unsigned long time_left = 0;

    atomic_inc(&task->transaction);

    time_left = rtc_sw_task_time_left(task);

    /*
     * before we remove it check to see if it was really in the list
     * it is removed from the list when fires that way we know if it has
     * fired or not yet.
     */
    if((req->next != NULL) || (req->prev != NULL) || (req == rtc_sw_request_head)) {
        /* It is currently in the list so we cannot return a time left of 0 because it will not fire*/ 
        if(time_left == 0) {
            time_left = 1;
        }
    }
    else {
        /*It is not in the list so when the tasklet associated with this runs, it will ignore the request*/
        return 0;
    }

    /* 
     * Remove the request containing this task in it
     */
    detach_from_list(req, CANCEL);
    if (req->next_to_run) {
        find_new_request();
    }

    return time_left;
}

unsigned long rtc_sw_task_stop(rtc_sw_task_t * task)
{
    /*TODO This should probably be combined with rtc_sw_task_internal_stop*/
    unsigned long flags;
    unsigned long time_left;
    spin_lock_irqsave(&request_lock, flags);
    time_left = rtc_sw_task_internal_stop(task);
    spin_unlock_irqrestore(&request_lock, flags);
    return time_left;
}

EXPORT_SYMBOL(rtc_sw_task_stop);

unsigned long rtc_sw_task_time_left(rtc_sw_task_t * task)
{
    long ticks_left = (long)task->remain;
    ticks_left -= epit_last - *_reg_EPIT_EPITCNR;
    if(ticks_left < 0)
    {
        ticks_left = 0;
    }
    return ticks_to_ms((unsigned long)ticks_left);
}
EXPORT_SYMBOL(rtc_sw_task_time_left);


static void rtc_sw_task_tasklet_callback(unsigned long data)
{
    unsigned long flags;
    rtc_sw_task_t * task = (rtc_sw_task_t *)data;
    if(atomic_read(&task->cookie) == atomic_read(&task->transaction)) {
        task->callback_data.task_data.orig_func(task->callback_data.task_data.orig_data);
    }

    spin_lock_irqsave(&request_lock, flags);

    jobs_running -= task->running;
    task->running = 0;
    jobs_running -= task->pending;
    task->pending = 0;

    RTC_SW_DPRINTK("\nEPIT: %s() tasklet done jobs_running %d\n", __FUNCTION__,jobs_running);

#ifdef CONFIG_MOT_FEAT_PM_STATS
    mpm_collect_pj_stat((unsigned long)task->callback_data.task_data.orig_func, \
                        "NULL", 0);
#endif

    if(rtc_sw_periodic_jobs_running() == PJ_NOT_RUNNING) {
        RTC_SW_DPRINTK("\nEPIT: %s() Done Going to sleep\n", __FUNCTION__);
        mpm_periodic_jobs_done();
    }
    spin_unlock_irqrestore(&request_lock, flags);
}

void rtc_sw_task_schedule(unsigned long offset, rtc_sw_task_t * task)
{
    unsigned long ticks;
    struct rtc_sw_request *req = task;
    unsigned long slice;
    unsigned long flags;
    
    ticks = ms_to_ticks(offset);
    
    req->interval = ticks;
    req->remain = req->interval;
    
    spin_lock_irqsave(&request_lock, flags);

    atomic_inc(&req->transaction);
    
    *_reg_EPIT_EPITCR &= ~EPITCR_EN;
    
    /* Get interval since the last time we updated the
     * request list. If the list is empty slice is 0. */
    slice = get_slice_passed();
    
    /* update the list before we insert another request */
    if (slice) {
        update_request_list(slice);
    }
    
    if((req->next == NULL) && (req->prev == NULL) && req != rtc_sw_request_head) {
        request_add(req);
    }
    
    /* reschedule the next stopwatch interrupt */
    /* req is now the next request that needs to be scheduled */
    req = get_lowest_request();
    
    put_request_in_use(req);
    
    /* resume the counter */
    *_reg_EPIT_EPITCR |= EPITCR_EN;
    
    spin_unlock_irqrestore(&request_lock, flags);
}
EXPORT_SYMBOL(rtc_sw_task_schedule);

// * 12/3/2007     Motorola       Added API to get closest RTC timeout
unsigned int rtc_sw_get_closest_timeout(void)
{
    unsigned int timeout = 0;
    struct rtc_sw_request * req;

    spin_lock(&request_lock);
    req = get_lowest_request();
    if(req)
        timeout = req->remain;
    spin_unlock(&request_lock);
    return ticks_to_ms(timeout);
}
EXPORT_SYMBOL(rtc_sw_get_closest_timeout);

static struct file_operations rtc_sw_fops = {
        .owner          = THIS_MODULE,
        .llseek         = rtc_sw_llseek,
        .read           = rtc_sw_read,
        .poll           = rtc_sw_poll,
        .ioctl          = rtc_sw_ioctl,
        .open           = rtc_sw_open,
        .release        = rtc_sw_release,
};

static struct miscdevice rtc_sw_miscdev = {
        .minor          = MISC_DYNAMIC_MINOR,
        .name           = "rtc_stopwatch",
        .fops           = &rtc_sw_fops,
        .devfs_name     = RTC_SW,
};

static int __init rtc_sw_init(void)
{
        int ret;

        ret = misc_register(&rtc_sw_miscdev);
        if (ret != 0) {
                printk(KERN_ERR "rtc_sw_init can't register rtc_sw misc dev: %d\n", ret);
                return ret;
        }

	ret = driver_register(&rtcsw_driver);
	if (ret != 0) {
                printk(KERN_ERR "rtc_sw_init can't register rtc_sw driver: %d\n", ret);
                misc_deregister(&rtc_sw_miscdev);
                return ret;
	} 

        ret = platform_device_register(&rtcsw_device);
        if (ret != 0) {
                printk(KERN_ERR "rtc_sw_init can't register rtc_sw platform device: %d\n", ret);
                driver_unregister(&rtcsw_driver);
                misc_deregister(&rtc_sw_miscdev);
                return ret;
        }

        /* disable EPIT and do soft reset */
        RTC_SW_DPRINTK("\nEPIT: %s() disable counter\n", __FUNCTION__ );
        *_reg_EPIT_EPITCR &= ~EPITCR_EN;
        *_reg_EPIT_EPITCR |= EPITCR_SWR;
        while ((*_reg_EPIT_EPITCR & EPITCR_SWR) != 0) {
        }

        setup_irq(INT_EPIT, &rtc_sw_irq);

        epit_last = *_reg_EPIT_EPITCNR;
        RTC_SW_DPRINTK("\nEPIT: %s() Epit counter at:    %lu\n", __FUNCTION__ , epit_last);
        RTC_SW_DPRINTK("\nEPIT: %s() Total ticks so far: %lu\n", __FUNCTION__ , ticks_recorded);

        *_reg_EPIT_EPITCR = EPITCR_CLKSRC_32K | EPITCR_OM_TOGGLE |
                            EPITCR_STOPEN_ENABLE | EPITCR_WAITEN_ENABLE |
                            EPITCR_DBGEN_ENABLE | EPITCR_DOZEN_ENABLE;

        mxc_clks_enable(EPIT1_CLK);

        /* make sure EPIT interrupt is enabled */
        RTC_SW_DPRINTK("\nEPIT: %s() enable interrupt\n",__FUNCTION__);
        *_reg_EPIT_EPITCR |= EPITCR_OCIEN;

        RTC_SW_DPRINTK("\nEPIT: %s() enable counter\n", __FUNCTION__ );
        *_reg_EPIT_EPITCR |= EPITCR_EN;

        return ret;
}

static void __exit rtc_sw_cleanup(void)
{
        platform_device_unregister(&rtcsw_driver);
	driver_unregister(&rtcsw_driver);
        if (misc_deregister(&rtc_sw_miscdev) != 0) {
                RTC_SW_DPRINTK(KERN_ERR "EPIT: rtc_sw_cleanup can't unregister rtc_sw");
        }
}

EXPORT_SYMBOL(rtc_sw_periodic_jobs_running);

static unsigned long rtc_sw_dsm_timestamp;
static int correct_time=0;
#define GPTCR_ENABLE (1 << 0)

/*!
 * This function is called to save the system EPIT time 
 * when entering a low power state. This timestamp is
 * then used on resume to adjust the system time to 
 * account for time loss while suspended.
 *
 * @param   dev   not used
 * @param   state Power state to enter.
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function returns 0 on success, and -EAGAIN on failure if IRQs
 *          are still enabled.
 */
static int rtcsw_suspend(struct device *dev, u32 state, u32 level)
{
	u32 reg;
	if (level == SUSPEND_POWER_DOWN) {
                /* Wait until last stage of PM suspend with interrupts off */
                if(!irqs_disabled())
                        return -EAGAIN;

		/* Stop counting in GPT */
		reg = __raw_readl(MXC_GPT_GPTCR);
		reg &= ~GPTCR_ENABLE;
		__raw_writel(reg, MXC_GPT_GPTCR);

		/* Save timestamp before going in DSM */
		rtc_sw_dsm_timestamp = *_reg_EPIT_EPITCNR; 
		correct_time = 1;
	}

	return 0;
}

/*!
 * This function is called to correct the system time based on the
 * current EPIT time relative to the time stamp saved during suspend.
 *
 * @param   dev   not used
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int rtcsw_resume(struct device *dev, u32 level)
{
	struct timespec tv;
        unsigned long timediffms = 0;
        unsigned long curtime;
	unsigned long flags;
	u32 reg;

	switch (level) {
	case RESUME_POWER_ON:
		/* Check whether we need to correct the system time */
		if (!correct_time) {
			return 0;
		}
		else {
			correct_time = 0;
		}

                /* Resume counting in GPT */
		reg = __raw_readl(MXC_GPT_GPTCR);
		reg |= GPTCR_ENABLE;
		__raw_writel(reg, MXC_GPT_GPTCR);
		
                /* Account for lost time during DSM */
                curtime = *_reg_EPIT_EPITCNR;;
                timediffms = ticks_to_ms(rtc_sw_dsm_timestamp - curtime);  

                tv.tv_sec = xtime.tv_sec;
                tv.tv_nsec = xtime.tv_nsec;

                tv.tv_sec += timediffms / 1000;
                tv.tv_nsec = tv.tv_nsec + (timediffms % 1000) * 1000000 + drift_in_ns(rtc_sw_dsm_timestamp - curtime);

		write_seqlock_irqsave(&xtime_lock, flags);
		set_normalized_timespec(&wall_to_monotonic, wall_to_monotonic.tv_sec + xtime.tv_sec - tv.tv_sec, wall_to_monotonic.tv_nsec + xtime.tv_nsec - tv.tv_nsec);
		set_normalized_timespec(&xtime, tv.tv_sec, tv.tv_nsec);
		write_sequnlock_irqrestore(&xtime_lock, flags);
                break;

	case RESUME_RESTORE_STATE:
		/* do nothing */
		break;

	case RESUME_ENABLE:
                /* do nothing */
		break;
	}
	return 0;
}

module_init(rtc_sw_init);
module_exit(rtc_sw_cleanup);
