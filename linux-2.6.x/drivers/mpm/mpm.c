/*
 * mpm.c - PM Driver to support DVFS and low power modes.
 *
 * Copyright (C) 2006-2007 Motorola, Inc.                                     
 *
 */

/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Date        Author            Comment
 * ==========  ================  ========================
 * 10/04/2006  Motorola          Initial version.
 * 12/04/2006  Motorola          Improved DSM time reporting for debug
 * 12/14/2006  Motorola          No longer constraining to 399.
 * 12/25/2006  Motorola          Changed local_irq_disable/local_irq_enable
 *                               to local_irq_save/local_irq_restore.
 * 01/05/2007  Motorola          Add datalog improvement
 * 01/18/2007  Motorola          Tie DSM to IDLE and reduce IOI timeout to 10ms
 * 01/19/2007  Motorola          Resolved issue with 133Mhz operating point
 *                               and its divider ratios.
 * 01/28/2007  Motorola          Add long IOI timeout API for SD Card
 * 02/12/2007  Motorola          Resolve race conditions in DSM state machine.
 * 04/04/2007  Motorola          Added ArgonLV support
 * 04/12/2007  Motorola          Check in idle for busy drivers/modules.
 * 10/25/2007  Motorola          Improved periodic job state collection for debug.
 * 11/14/2007  Motorola          Add #error when chipset is not spported by MOTO PM.
 * 11/23/2007  Motorola          Add a config macro to use Bluetooth LED to 
 *                               indicate whether the phone is in DSM.
 */

#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/mpm.h>
#include <linux/dpm.h>
#include <linux/rtc.h>
#include <asm/arch/clock.h>
#include <asm/arch/mxc_ipc.h>
#include <linux/ks_otp_api.h>
#include "mpm_sysfs.h"
#include "mpmdrv.h"
#ifdef CONFIG_MOT_FEAT_PM_STATS
#include <asm/timex.h>
#include <stdarg.h>
#endif
#ifdef CONFIG_MOT_TURBO_INDICATOR
#include <linux/power_ic_kernel.h>
#endif

#define FREQ_MARGIN 5000000

#define NOT_CONSTRAINING_TO_399

#ifdef NOT_CONSTRAINING_TO_399
#ifdef CONFIG_MACH_ARGONLVPHONE
static mpm_op_t valid_op_init[] = {128000000, 231000000, 385000000, 514000000
#elif CONFIG_MACH_SCMA11PHONE
static mpm_op_t valid_op_pass2[] = { 133000000, 266000000, 399000000, 532000000
#ifdef CONFIG_FEAT_MXC31_CORE_OVERCLOCK_740
										, 636000000, 740000000
#elif CONFIG_FEAT_MXC31_CORE_OVERCLOCK_780
										, 665000000, 780000000
#endif // CONFIG_FEAT_MXC31_CORE_OVERCLOCK_
#else // CONFIG_MACH_ARGONLVPHONE || CONFIG_MACH_SCMA11PHONE
#error Motorola Power Management is only supported for SCM-A11 and ArgonLV
#endif // CONFIG_MACH_ARGONLVPHONE || CONFIG_MACH_SCMA11PHONE
											};
#else // NOT_CONSTRAINING_TO_399
static mpm_op_t valid_op_pass2[] = { 399000000 };
#endif

static const mpm_op_t *valid_op=NULL;
static DECLARE_MUTEX (rd_sem);

static int num_op=0;
static int debug = 0;
static unsigned long orig_lpj;
static unsigned long orig_freq;

#ifdef CONFIG_MOT_FEAT_PM_STATS
static mpm_opst_list_t *op_head=NULL;
static kmem_cache_t *opstat_cache=NULL;
static char buf[3048];
static int opstat = 0;

static void flush_oplist(void);
static void collect_op_stat(mpm_op_t, struct timeval *, struct timeval *);
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS)
static mpm_pjist_list_t *pji_head=NULL;
static kmem_cache_t *pjistat_cache=NULL;
static mpm_pjcst_list_t *pjc_head=NULL;
static kmem_cache_t *pjcstat_cache=NULL;
static int pjstat = 0;
static int pjsmod = 0;
static int pjwakeup = 0;
static unsigned long pjstime = 0;
static void flush_pjlist(void);
#endif // #if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS)
#endif

static void mpm_remove(struct device *);
static unsigned long mpm_compute_lpj(unsigned long newf);

module_param (debug, int, S_IRUGO | S_IWUSR);

/* Array of sleep modes: */
static suspend_state_t sleep_modes[] = {
    PM_SUSPEND_STOP,  /* Stop Mode */
    PM_SUSPEND_MEM    /* Deep Sleep Mode (DSM) */
};

/* Number of available sleep modes */
static int num_sm = sizeof(sleep_modes) / sizeof(sleep_modes[0]);

/* Array containing sleep interval values */
static mpm_sleepinterval_t *sleep_intervals = NULL;

/* Low Power mode state variable */
static atomic_t mpm_lpm_state = ATOMIC_INIT(LPM_STATE_AWAKE);
static struct timer_list mpm_suspend_timer; /* Interrupt inactivity timer */
static unsigned long mpm_last_ioi_timestamp; /* Timestamp in jiffies for the
                                                most recent occurrence of IOI */
/* IOI timer in jiffies, time to delay sleep retry due to IOI */
static unsigned long mpm_interrupt_inactivity_interval;

/* IOI timer in jiffies, time to delay sleep retry due to long IOI */
static unsigned long mpm_interrupt_inactivity_long_interval;

/* retry timer in jiffies, time to delay due to driver reject */
static unsigned long mpm_retry_sleep_interval;

/* LPM transition in jiffies, time to delay between low power modes */
static unsigned long mpm_lpm_transition_interval;

#define MPM_IOI_TIMEOUT    10         /* IOI Timeout set to 10ms */
#define MPM_RETRY_TIMEOUT  500        /* Retry DSM after driver reject */
                                      /* timeout set to 500 ms */
#define MPM_LONG_IOI_TIMEOUT 2000     /* For SD Card, 2 second IOI */

#define WORK_QUEUE_NAME "mpm_workq"
static void mpm_pm_suspend_work(void *);
static struct work_struct suspend_task;
static struct workqueue_struct *suspend_worker_thread;

/* Statically create a work queue structure */
static DECLARE_WORK(suspend_task, mpm_pm_suspend_work, NULL);

DECLARE_MUTEX(sleep_interval_array_sem);

static int requested_sleep_mode = 0;

static int num_wq_queued = 0;

static mpm_desense_info_t last_desense = {0};
static int last_desense_result = 0;
static unsigned char security_mode = MOT_SECURITY_MODE_PRODUCTION;


/*
 * Return a pointer to the name of the given low power mode action.
 */
static const char * const lpm_action_name(int action)
{
    switch (action)
    {
	case LPM_ACTION_AWAKE_REQUEST:
	    return "AWAKE_REQUEST";
	case LPM_ACTION_SLEEP_REQUEST:
	    return "SLEEP_REQUEST";
	case LPM_ACTION_SYSTEM_IDLE:
	    return "SYSTEM_IDLE";
	case LPM_ACTION_SUSPEND_TIMER_EXPIRES:
	    return "SUSPEND_TIMER_EXPIRES";
	case LPM_ACTION_RESUME_FROM_SLEEP:
	    return "RESUME_FROM_SLEEP";
	case LPM_ACTION_HANDLE_IOI:
	    return "HANDLE_IOI";
	case LPM_ACTION_HANDLE_LONG_IOI:
	    return "HANDLE_LONG_IOI";
	case LPM_ACTION_SUSPEND_SUCCEEDED:
	    return "SUSPEND_SUCCEEDED";
	case LPM_ACTION_SUSPEND_FAILED:
	    return "SUSPEND_FAILED";
	case LPM_ACTION_READY_TO_SLEEP_IN_WQ:
	    return "READY_TO_SLEEP_IN_WQ";
	case LPM_ACTION_PERIODIC_JOBS_DONE:
	    return "PERIODIC_JOBS_DONE";
	default:
	    return "UNKNOWN";
    }
}

/*
 * Return a pointer to the name of the given low power mode state.
 */
static const char * const lpm_state_name(int state)
{
    switch (state)
    {
	case LPM_STATE_UNDEFINED:
	    return "UNDEFINED";
	case LPM_STATE_AWAKE:
	    return "AWAKE";
	case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
	    return "WAITING_FOR_SLEEP_TIMER";
	case LPM_STATE_WAITING_FOR_IDLE:
	    return "WAITING_FOR_IDLE";
	case LPM_STATE_INITIATING_SLEEP:
	    return "INITIATING_SLEEP";
	case LPM_STATE_SLEEPING:
	    return "SLEEPING";
	case LPM_STATE_WAKING_UP:
	    return "WAKING_UP";
	case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
	    return "WAITING_FOR_PJ_COMPLETION";
	default:
	    return "UNKNOWN";
    }
}

/*
 * Name: start_suspend_timer
 *
 * Arguments:
 *     interval:  the amount of time (in jiffies) in the future that
 *                the timer should expire.
 *
 * Return value:
 *     None
 *
 * start_suspend_timer starts the suspend timer.  This function assumes
 * that the timer is not currently running.  It is the responsibility of
 * the caller to ensure that the suspend timer is not running.
 * The expiration time is set to the current jiffies + the given interval.
 * start_suspend_timer is the only place where the suspend timer is
 * started.  It is modified by extend_suspend_timer, but only started
 * in this function.
 *
 * Notes:
 *     start_suspend_timer may be called from any context.
 */
static void start_suspend_timer (unsigned long interval)
{
    mpm_suspend_timer.expires = jiffies + interval;
    add_timer(&mpm_suspend_timer);

    MPM_DPRINTK("MPM: New suspend timer = %ld. Jiffies = %ld. "
		"inactivity interval = %ld\n",
		mpm_suspend_timer.expires, jiffies, interval);
}

/*
 * Name: extend_suspend_timer
 *
 * Arguments:
 *     interval:  the amount of time (in jiffies) in the future that
 *                the timer should expire.
 *
 * Return value:
 *     None
 *
 * extend_suspend_timer modifies the suspend timer expiration time.
 * The suspend timer must be running and have a valid expiration
 * time.  It is the responsibility of the caller to ensure that
 * the suspend timer is running when this function is called.
 *
 * extend_suspend_timer never shortens the time remaining on the
 * suspend timer.  If the requested interval is later than the
 * existing timer, then the timer is given the new expiration time.
 * If the requested interval is earlier than the existing timer, then
 * the existing timer remains unchanged.
 *
 * Notes:
 *     extend_suspend_timer may be called from any context.
 */
static void extend_suspend_timer (unsigned long interval)
{
    unsigned long new_expiration;

    /*
     * We only modify the timer if the new expiration is later than
     * the current expiration.
     * Note that we assume that the mpm_suspend_timer.expires
     * value is valid.  This is true as long as a timer is running.
     */
    new_expiration = jiffies + interval;
    if (time_after(new_expiration, mpm_suspend_timer.expires))
    {
	mod_timer(&mpm_suspend_timer, new_expiration);

        MPM_DPRINTK("MPM: Extended suspend timer = %ld. Jiffies = %ld. "
		    "inactivity interval = %ld\n",
		    mpm_suspend_timer.expires, jiffies, interval);
    }
}

/*
 * Name: mpm_state_change
 *
 * Arguments:
 *     caller:    pointer to string containing the name of the caller.
 *                The string must be NUL-terminated.
 *     action:    the requested action
 *
 * Return value:
 *     newstate:  the new state
 *
 * mpm_state_change runs the MPM DSM state machine.  The given
 * action determines what state changes and related semantic
 * actions occur.
 *
 * If the LPM state changes, it is done in this function.  No
 * state changes are allowed anywhere else.
 *
 * Notes:
 *     mpm_state_change may be called from any context.
 */
static int mpm_state_change(const char *caller, int action)
{
    int oldstate;
    int newstate = LPM_STATE_UNDEFINED;
    unsigned long flags;
    unsigned long interval;

    local_irq_save(flags);

    oldstate = atomic_read(&mpm_lpm_state);

    switch (action)
    {
	/*
         * ===== LPM_ACTION_AWAKE_REQUEST =============================
	 */
	case LPM_ACTION_AWAKE_REQUEST:
	    switch (oldstate)
	    {
		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
		    del_timer_sync(&mpm_suspend_timer);
		    newstate = LPM_STATE_AWAKE;
		    break;

		case LPM_STATE_AWAKE:
		case LPM_STATE_WAITING_FOR_IDLE:
		case LPM_STATE_INITIATING_SLEEP:
		case LPM_STATE_WAKING_UP:
		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
		    newstate = LPM_STATE_AWAKE;
		    break;

		case LPM_STATE_SLEEPING:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;

	/*
         * ===== LPM_ACTION_SLEEP_REQUEST =============================
	 */
	case LPM_ACTION_SLEEP_REQUEST:
	    switch (oldstate)
	    {
		case LPM_STATE_AWAKE:
		    /*
		     * We have received a request to go to
		     * sleep.  Start a timer for at most
		     * mpm_lpm_transition_interval jiffies.  We
		     * subtract off any time since the last interrupt
		     * of interest.  If we haven't had any IOIs for
		     * the last mpm_lpm_transition_interval time, then
		     * just go to the LPM_STATE_WAITING_FOR_IDLE state
		     * without starting a timer.
		     */
                    if (time_before(jiffies, mpm_last_ioi_timestamp +
                                    mpm_lpm_transition_interval))
                    {
                        mpm_suspend_timer.expires = mpm_last_ioi_timestamp +
                                                    mpm_lpm_transition_interval;
                        MPM_DPRINTK("MPM: New suspend timeout = %ld. "
                                     "Jiffies = %ld. Last IOI timestamp = %ld. "
                                     "inactivity interval = %ld\n",
                                     mpm_suspend_timer.expires, jiffies,
                                     mpm_last_ioi_timestamp,
                                     mpm_lpm_transition_interval);
                        add_timer(&mpm_suspend_timer);
			newstate = LPM_STATE_WAITING_FOR_SLEEP_TIMER;
                    }
                    else
                    {
                        MPM_DPRINTK("MPM: Skipping sleep timer. "
                                     "Jiffies = %ld. Last IOI timestamp = %ld. "
                                     "inactivity interval = %ld\n",
                                     jiffies, mpm_last_ioi_timestamp,
                                     mpm_lpm_transition_interval);
			newstate = LPM_STATE_WAITING_FOR_IDLE;
                    }
		    break;

		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
		case LPM_STATE_WAITING_FOR_IDLE:
		case LPM_STATE_INITIATING_SLEEP:
		case LPM_STATE_WAKING_UP:
		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
		    MPM_DPRINTK("MPM: Unexpected state transition: "
				"Received LPM_ACTION_SLEEP request "
				"while already trying to go to sleep. ");
		    newstate = oldstate;
		    break;

		case LPM_STATE_SLEEPING:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;

	/*
         * ===== LPM_ACTION_SYSTEM_IDLE ===============================
	 */
	case LPM_ACTION_SYSTEM_IDLE:
	    switch (oldstate)
	    {
		case LPM_STATE_WAITING_FOR_IDLE:
		    /*
		     * The following code ensures that one and only one
		     * work queue can be queued at one time.  If a work
		     * queue is already queued, then don't start another.
		     * When the existing work queue finishes, we'll get
		     * back to idle and then start up another one if we
		     * still need to.
		     *
		     * If any driver/module has indicated that it is
		     * busy via mpm_driver_advise, then we won't start
		     * the work queue.  The purpose of this check is
		     * so that the driver/module doesn't ultimately
		     * reject the suspend, causing a large delay in
		     * getting back to sleep.
		     */
                    if ((num_wq_queued == 0) && (mpm_num_busy_drivers() == 0))
		    {
			num_wq_queued++;
			newstate = LPM_STATE_INITIATING_SLEEP;
			queue_work(suspend_worker_thread, &suspend_task);
		    }
		    else
		    {
#ifdef CONFIG_MOT_FEAT_PM_STATS
			if (num_wq_queued == 0)
			{
                            MPM_REPORT_TEST_POINT(1,
                                    MPM_TEST_SUSPEND_DELAYED_BY_BUSY_DRIVERS);
			}
#endif
		        newstate = oldstate;
		    }
		    break;

		case LPM_STATE_AWAKE:
		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
		case LPM_STATE_INITIATING_SLEEP:
		case LPM_STATE_WAKING_UP:
		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
		    newstate = oldstate;
		    break;

		case LPM_STATE_SLEEPING:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;

	/*
         * ===== LPM_ACTION_SUSPEND_TIMER_EXPIRES =====================
	 */
        case LPM_ACTION_SUSPEND_TIMER_EXPIRES:
	    switch (oldstate)
	    {
		case LPM_STATE_AWAKE:
		    newstate = oldstate;
		    break;

		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
		    newstate = LPM_STATE_WAITING_FOR_IDLE;
		    break;

		case LPM_STATE_WAITING_FOR_IDLE:
		case LPM_STATE_INITIATING_SLEEP:
		case LPM_STATE_WAKING_UP:
		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
		case LPM_STATE_SLEEPING:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;

	/*
         * ===== LPM_ACTION_HANDLE_IOI ================================
         * ===== LPM_ACTION_HANDLE_LONG_IOI ===========================
	 */
	case LPM_ACTION_HANDLE_IOI:
	case LPM_ACTION_HANDLE_LONG_IOI:
	    switch (oldstate)
	    {
		case LPM_STATE_AWAKE:
		    newstate = oldstate;
		    break;

		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
#ifdef CONFIG_MOT_FEAT_PM_STATS
		    MPM_REPORT_TEST_POINT(1, MPM_TEST_SUSPEND_DELAYED_BY_IOI);
#endif
		    if (action == LPM_ACTION_HANDLE_IOI)
			interval = mpm_interrupt_inactivity_interval;
		    else
			interval = mpm_interrupt_inactivity_long_interval;

		    extend_suspend_timer(interval);
		    newstate = oldstate;
		    break;

		case LPM_STATE_WAITING_FOR_IDLE:
		case LPM_STATE_INITIATING_SLEEP:
		case LPM_STATE_WAKING_UP:
		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
#ifdef CONFIG_MOT_FEAT_PM_STATS
		    MPM_REPORT_TEST_POINT(1, MPM_TEST_SUSPEND_DELAYED_BY_IOI);
#endif
		    if (action == LPM_ACTION_HANDLE_IOI)
			interval = mpm_interrupt_inactivity_interval;
		    else
			interval = mpm_interrupt_inactivity_long_interval;
		    start_suspend_timer(interval);
		    newstate = LPM_STATE_WAITING_FOR_SLEEP_TIMER;
		    break;

		case LPM_STATE_SLEEPING:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;

	/*
         * ===== LPM_ACTION_READY_TO_SLEEP_IN_WQ ======================
	 */
	case LPM_ACTION_READY_TO_SLEEP_IN_WQ:
	    switch (oldstate)
	    {
		case LPM_STATE_AWAKE:
		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
		case LPM_STATE_WAITING_FOR_IDLE:
		    /*
		     * These states are accessible if an IOI
		     * occurred while we were suspending devices
		     * (with interrupts enabled).  The IOI would
		     * start a timer and that's how we get to
		     * LPM_STATE_WAITING_FOR_SLEEP_TIMER.  If the
		     * timer expires before we get to sleep, then we
		     * could be in LPM_STATE_WAITING_FOR_IDLE.  We
		     * can't get beyond LPM_STATE_WAITING_FOR_IDLE
		     * because only one work queue is allowed to
		     * execute at a time.
		     */
		    newstate = oldstate;
		    MPM_REPORT_TEST_POINT(1, MPM_TEST_SUSPEND_CANCELLED_BY_USER_ACTIVITY);
		    break;

		case LPM_STATE_INITIATING_SLEEP:
		    /*
		     * If we get here, then all of the checks for
		     * going to sleep have successfully completed
		     * except for periodic jobs.  We are only ever
		     * in the LPM_STATE_WAITING_FOR_PJ_COMPLETION
		     * state when it is the only condition that
		     * causes us not to go to sleep.  As soon as
		     * another condition occurs that causes us
		     * to not go to sleep (e.g. IOI), we change
		     * to a state that reflects that.
		     */
		    if ((rtc_sw_periodic_jobs_running() == PJ_RUNNING) ||
			((security_mode != MOT_SECURITY_MODE_PRODUCTION) &&
			 (mxc_ipc_datalog_transfer_ongoing() == DATALOG_RUNNING)))
		    {
			newstate = LPM_STATE_WAITING_FOR_PJ_COMPLETION;
			MPM_REPORT_TEST_POINT(1, MPM_TEST_SUSPEND_DELAYED_BY_PJ);
		    }
		    else
		    {
			newstate = LPM_STATE_SLEEPING;
		    }
		    break;

		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
		    /*
		     * Don't report that we've been delayed by
		     * periodic jobs here.  It's already been
		     * reported.
		     */
		    newstate = oldstate;
		    break;

		case LPM_STATE_WAKING_UP:
		case LPM_STATE_SLEEPING:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;

	/*
         * ===== LPM_ACTION_PERIODIC_JOBS_DONE ========================
	 */
	case LPM_ACTION_PERIODIC_JOBS_DONE:
	    switch (oldstate)
	    {
		case LPM_STATE_AWAKE:
		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
		case LPM_STATE_WAITING_FOR_IDLE:
		case LPM_STATE_INITIATING_SLEEP:
		case LPM_STATE_WAKING_UP:
		    newstate = oldstate;
		    break;

		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
		    newstate = LPM_STATE_WAITING_FOR_IDLE;
		    break;

		case LPM_STATE_SLEEPING:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;

	/*
         * ===== LPM_ACTION_SUSPEND_SUCCEEDED =========================
	 *
	 * This action occurs only after we have successfully gone
	 * to sleep and woken up.  If we didn't go to sleep for any
	 * reason, then this action is not called.  We call the
	 * LPM_ACTION_SUSPEND_FAILED action instead.
	 */
	case LPM_ACTION_SUSPEND_SUCCEEDED:
	    switch (oldstate)
	    {
		/*
		 * Once we reach LPM_STATE_WAITING_FOR_IDLE, we cannot
		 * start a work queue until the previous one has
		 * finished (see LPM_ACTION_SYSTEM_IDLE).  Even if the
		 * machine goes idle, it cannot start the work queue
		 * because num_wq_queued is still positive.  If we get
		 * here and the state is LPM_STATE_WAITING_FOR_IDLE,
		 * then we just leave the state alone and exit from
		 * the work queue.  Barring any changes to the state,
		 * we will create a new work queue to go back to
		 * sleep when the machine next reaches the idle
		 * loop.  If we are in states LPM_STATE_AWAKE or
		 * LPM_STATE_WAITING_FOR_SLEEP_TIMER, then we just
		 * need to leave the states as they are and exit out
		 * of this work queue as fast as possible.
		 */
		case LPM_STATE_AWAKE:
		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
		case LPM_STATE_WAITING_FOR_IDLE:
		    newstate = oldstate;
		    break;

		/*
		 * We have woken up from pm_suspend and have not seen
		 * an IOI.  We can only be in LPM_STATE_WAKING_UP if
		 * we woke up because of a periodic job or a non-IOI
		 * interrupt.  We check here for periodic jobs and
		 * set the state appropriately if that's why we woke
		 * up.  If we woke up because of a non-IOI interrupt,
		 * then start up a timer.  The timer is longer than
		 * the timer for IOI interrupts because we don't know
		 * about it and give it more time to handle what needs
		 * to be done.
		 */
		case LPM_STATE_WAKING_UP:
		    if ((rtc_sw_periodic_jobs_running() == PJ_RUNNING) ||
			((security_mode != MOT_SECURITY_MODE_PRODUCTION) &&
			 (mxc_ipc_datalog_transfer_ongoing() == DATALOG_RUNNING)))
		    {
			newstate = LPM_STATE_WAITING_FOR_PJ_COMPLETION;
		    }
		    else
		    {
		        newstate = LPM_STATE_WAITING_FOR_SLEEP_TIMER;
		        start_suspend_timer(mpm_retry_sleep_interval);
		    }
		    break;

		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
		case LPM_STATE_INITIATING_SLEEP:
		case LPM_STATE_SLEEPING:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;

	/*
         * ===== LPM_ACTION_SUSPEND_FAILED ============================
	 *
	 * This action is requested after we have called pm_suspend and
	 * have detected that the pm_suspend failed.  It might have failed
	 * for many reasons -- driver rejected, an IOI occurred, an awake
	 * request was received.  This action is called with interrupts
	 * enabled.
	 */
	case LPM_ACTION_SUSPEND_FAILED:
	    switch (oldstate)
	    {
		case LPM_STATE_AWAKE:
		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
		case LPM_STATE_WAITING_FOR_IDLE:
		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
		    newstate = oldstate;
		    break;

		case LPM_STATE_INITIATING_SLEEP:
		case LPM_STATE_SLEEPING:
		    /*
		     * We can get here with LPM_STATE_SLEEPING if a
		     * driver rejects the suspend after interrupts are
		     * disabled.
		     */
		    start_suspend_timer(mpm_retry_sleep_interval);
		    newstate = LPM_STATE_WAITING_FOR_SLEEP_TIMER;
		    break;

		case LPM_STATE_WAKING_UP:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;

	/*
         * ===== LPM_ACTION_RESUME_FROM_SLEEP =========================
	 *
	 * This action occurs only after we have successfully woken up
	 * from sleep.  If there was a failure to sleep, then we do
	 * not go through the LPM_STATE_WAKING_UP state.
	 *
	 * This action is called with interrupts disabled.
	 */
	case LPM_ACTION_RESUME_FROM_SLEEP:
	    switch (oldstate)
	    {
		case LPM_STATE_SLEEPING:
		    newstate = LPM_STATE_WAKING_UP;
		    break;

		case LPM_STATE_AWAKE:
		case LPM_STATE_WAITING_FOR_SLEEP_TIMER:
		case LPM_STATE_WAITING_FOR_IDLE:
		case LPM_STATE_INITIATING_SLEEP:
		case LPM_STATE_WAKING_UP:
		case LPM_STATE_WAITING_FOR_PJ_COMPLETION:
		case LPM_STATE_UNDEFINED:
		default:
		    /* Shouldn't happen. */
		    newstate = LPM_STATE_UNDEFINED;
		    break;
	    }
	    break;


	default:
	    /* Shouldn't happen. */
	    newstate = LPM_STATE_UNDEFINED;
	    break;
    }

    if (newstate == LPM_STATE_UNDEFINED)
    {
#ifdef CONFIG_MOT_FEAT_PM_STATS
	MPM_REPORT_TEST_POINT(4, MPM_TEST_STATE_TRANSITION_ERROR,
			      caller, action, oldstate);
#endif
        printk(KERN_ERR "MPM: mpm_state_change/%s(%ld): "
	       "STATE TRANSITION ERROR: action=%s(%d) old=%s(%d)\n",
	       caller, rtc_sw_msfromboot(),
	       lpm_action_name(action), action,
	       lpm_state_name(oldstate), oldstate);

	/*
	 * We might want to panic here, since this is a state
	 * transition that "can't happen".  If we get here
	 * on a shipping product, then transition to the
	 * initial state where we try to go to sleep.
	 * This is the safest state to go to because it is
	 * the farthest from going to sleep, but it still
	 * puts us on the path of going to sleep.  There is
	 * little that can go wrong when the pmdaemon requests
	 * that we stay awake, so we're assuming that that
	 * transition will never fail.  All of the interesting
	 * cases that we might miss occur while we're trying
	 * to go to sleep, so just restart that part of the
	 * state machine.
	 */
	newstate = LPM_STATE_WAITING_FOR_SLEEP_TIMER;
	del_timer_sync(&mpm_suspend_timer);
	start_suspend_timer(mpm_lpm_transition_interval);
    }

    if (newstate != oldstate)
    {
        MPM_DPRINTK("MPM: mpm_state_change/%s(%ld): "
		    "action=%s(%d) old=%s(%d) new=%s(%d)\n",
		    caller, rtc_sw_msfromboot(),
		    lpm_action_name(action), action,
		    lpm_state_name(oldstate), oldstate,
		    lpm_state_name(newstate), newstate);

        atomic_set(&mpm_lpm_state, newstate);
    }

    local_irq_restore(flags);

    return newstate;
}


/*
 * Name: mpm_ready_to_sleep_callback
 *
 * Arguments: None.
 *
 * Return value:
 *           MPM_GO_TO_SLEEP : Indicates that system can go into low power mode.
 *           MPM_DO_NOT_SLEEP: Indicates system should not enter low power mode.
 *
 * Notes:
 *   This function is called in a process context with interrupts
 *   disabled.
 */
static int mpm_ready_to_sleep_callback(void)
{
    int ret = MPM_GO_TO_SLEEP;
    int newstate;

    newstate = mpm_state_change(__FUNCTION__, LPM_ACTION_READY_TO_SLEEP_IN_WQ);

    /*
     * If the new state is LPM_STATE_SLEEPING, then nothing can stop
     * us now from going to sleep.  We have met all of the
     * requirements for going to sleep and the next step
     * will be for the work queue to go to sleep.  Interrupts
     * will not be enabled until we actually go to sleep.
     */
    if (newstate == LPM_STATE_SLEEPING)
	ret = MPM_GO_TO_SLEEP;
    else
	ret = MPM_DO_NOT_SLEEP;

    return ret;
}

/*
 * Name: mpm_resume_from_sleep_callback
 *
 * Arguments: None
 *
 * Return value:
 *            This function always returns 0.
 *
 * mpm_resume_from_sleep_callback is called when the machine wakes up
 * from DSM mode.  It is called with interrupts disabled and the state
 * is expected to be LPM_STATE_SLEEPING.
 *
 * Interrupts have been disabled since the machine went to sleep so
 * there is no further information available on why we're waking up.
 * All we do here is to change the state to LPM_STATE_WAKING_UP.
 *
 * Notes:
 *   This function is called in a process context with interrupts
 *   disabled.
 */
static int mpm_resume_from_sleep_callback(void)
{
    mpm_state_change(__FUNCTION__, LPM_ACTION_RESUME_FROM_SLEEP);

    return 0;
}


/*
 * Name: mpm_pm_suspend_work
 *
 * Arguments: Pointer to argument list.
 *
 * Return value:
 *            None.
 *
 * mpm_pm_suspend_work is the function called by the suspend_task
 * work queue to put the machine to sleep.  We choose to do the
 * final processing of going to sleep in a work queue so that
 * we can execute in a process context with interrupts enabled.
 * Putting devices to sleep must start with interrupts enabled.
 *
 * By the time we get here, we have passed almost all requirements to
 * go to sleep.  Since interrupts are not blocked yet, something may
 * yet happen that causes us not to go to sleep, but this is where we
 * call the Linux function (pm_suspend) to put us to sleep.  At some
 * point in pm_suspend, interrupts are disabled and the final checks
 * are made to determine if we really can go to sleep.  The final
 * checks are made with interrupts disabled to provide a race-free
 * way of checking.
 *
 * Notes:
 * This function is called in a process context with interrupts
 * enabled.
 */
static void mpm_pm_suspend_work(void *arg)
{
    int cur_lpm_state;
    int ret;

    cur_lpm_state = atomic_read(&mpm_lpm_state);
    MPM_DPRINTK("MPM: mpm_pm_suspend_work. Current lpm state = %d\n",
                cur_lpm_state);

    /*
     * We will only continue executing this work queue if we
     * are in the LPM_STATE_INITIATING_SLEEP state.  If we aren't in
     * LPM_STATE_INITIATING_SLEEP, then some other action has caused us to
     * transition to another state that is inappropriate for running
     * the work queue, so we just quit.
     */
    if (cur_lpm_state != LPM_STATE_INITIATING_SLEEP)
    {
	num_wq_queued--;
	return;
    }

#ifdef CONFIG_MOT_DSM_INDICATOR
    lights_exit_turbo_mode();
#endif

    /* TODO:
     * Add logic to select sleep mode here...
     * For the first pass, go directly into DSM */
    requested_sleep_mode = MPM_DSM;

    /*
     * The call to pm_suspend will return after the system has
     * successfully entered the sleep mode and was then woken up,
     * or if the sleep attempt failed.
     */
    ret = pm_suspend(sleep_modes[requested_sleep_mode]);

    if (ret)
        mpm_state_change(__FUNCTION__, LPM_ACTION_SUSPEND_FAILED);
    else
        mpm_state_change(__FUNCTION__, LPM_ACTION_SUSPEND_SUCCEEDED);

#ifdef MPM_DEBUG
    cur_lpm_state = atomic_read(&mpm_lpm_state);
    MPM_DPRINTK("MPM: End of mpm_pm_suspend_work. Current lpm state = %d\n",
		cur_lpm_state);
#endif

#ifdef CONFIG_MOT_DSM_INDICATOR
    lights_enter_turbo_mode();
#endif

    num_wq_queued--;
}


/*
 * Name: mpm_suspend_timeout_handler
 *
 * Arguments: "data" to be passed from timer setup to the timer handler.
 *
 * Return value: None.
 *
 * Notes:
 *   This function is called in interrupt context on expiry of suspend_timer.
 */
static void mpm_suspend_timeout_handler(unsigned long data)
{
    mpm_state_change(__FUNCTION__, LPM_ACTION_SUSPEND_TIMER_EXPIRES);
}


/*
 * Name: mpm_handle_ioi_callback
 *
 * Arguments: None.
 *
 * Return value: Zero.
 *
 * Notes:
 *   This function is called in an interrupt context.
 */
static int mpm_handle_ioi_callback(void)
{
#ifdef MPM_DEBUG
    unsigned int lr;

    asm volatile("mov %0, r14" : "=r" (lr));
    MPM_DPRINTK("MPM: mpm_handle_ioi_callback: %ld lr=0x%08x\n", rtc_sw_msfromboot(), lr);
#endif /* MPM_DEBUG */

    /* Update ioi timestamp */
    mpm_last_ioi_timestamp = jiffies;

    mpm_state_change(__FUNCTION__, LPM_ACTION_HANDLE_IOI);

    return 0;
}

/*
 * Name: mpm_handle_long_ioi_callback
 *
 * Arguments: None.
 *
 * Return value: Zero.
 *
 * Notes:
 *   This function is called in an interrupt context.
 */
static int mpm_handle_long_ioi_callback(void)
{
#ifdef MPM_DEBUG
    unsigned int lr;

    asm volatile("mov %0, r14" : "=r" (lr));
    MPM_DPRINTK("MPM: mpm_handle_long_ioi_callback: %ld lr=0x%08x\n", rtc_sw_msfromboot(), lr);
#endif /* MPM_DEBUG */

    /* Update ioi timestamp */
    mpm_last_ioi_timestamp = jiffies;

    mpm_state_change(__FUNCTION__, LPM_ACTION_HANDLE_LONG_IOI);

    return 0;
}

/*
 * Name: mpm_periodic_jobs_started_callback
 *
 * Arguments: None.
 *
 * Return value: Zero.
 *
 * Notes:
 *   This function is a place holder for any processing to be done
 *   when periodic jobs are started.
 *   This function is called in a interrupt context.
 */
static int mpm_periodic_jobs_started_callback(void)
{
    MPM_DPRINTK("MPM: In mpm_periodic_jobs_started_callback. \n");
    return 0;
}


/*
 * Name: mpm_start_sleep_callback
 *
 * Arguments: None.
 *
 * Return value: 0 - Indicates to Idle loop that we are not in a Ready to sleep
 *                   state, continue with WFI.
 *               1 - Indicates to Idle loop that we have just started a Work
 *                   Queue
 *
 * Notes:
 *   This function is called from the Idle loop to check if we should
 *   initiate sleep.  The sleep work queue is started if we are in the
 *   LPM_STATE_WAITING_FOR_IDLE state.  If not, simply return to the idle
 *   loop and allow it to enter WFI.
 *   This function is called from the kernel's idle loop.
 */
static int mpm_start_sleep_callback(void)
{
    unsigned long flags;
    int ret_code = 0;
    int newstate;

    /* Disable interrupts. */
    local_irq_save(flags);

    newstate = mpm_state_change(__FUNCTION__, LPM_ACTION_SYSTEM_IDLE);

    if (newstate == LPM_STATE_INITIATING_SLEEP)
	ret_code = 1;

    /* Enable interrupts. */
    local_irq_restore(flags);

    return ret_code;
}

/*
 * Name: mpm_periodic_jobs_done_callback
 *
 * Arguments: None.
 *
 * Return value: Zero.
 *
 * Notes:
 *   This function is called in a process context.
 */
static int mpm_periodic_jobs_done_callback(void)
{
    mpm_state_change(__FUNCTION__, LPM_ACTION_PERIODIC_JOBS_DONE);

    return 0;
}

/*
 * Name: mpm_set_awake_state_callback
 *
 * Arguments: None.
 *
 * Return value: Zero
 *
 * Notes:
 *   This function is called in a process context.
 */
static int mpm_set_awake_state_callback(void)
{
    mpm_state_change(__FUNCTION__, LPM_ACTION_AWAKE_REQUEST);

    return 0;
}

#ifdef CONFIG_MOT_FEAT_PM_STATS
/*
 * Callback routine for gathering statistics on low power modes.
 * This is the function that is called when MPM_REPORT_TEST_POINT()
 * is invoked.
 */
static void mpm_report_test_point_callback(int argc, ...)
{
    mpm_test_point_t test_point=-1;
    char *data = NULL;
    va_list args;
    unsigned long timestamp;
    int mode;
    int indx = 0;
    int i;
    unsigned int bucket;
    u32 rtc_before;
    u32 rtc_after;
    unsigned long flags;
    unsigned long driver_flags;
    u32 avic_nipndl;
    u32 avic_nipndh;

    timestamp = rtc_sw_msfromboot();

    va_start(args, argc);
    test_point = va_arg(args, mpm_test_point_t);

    switch(test_point) {

        case MPM_TEST_DEVICE_SUSPEND_START:
            MPM_DPRINTK("MPM_TEST_DEVICE_SUSPEND_START\n");
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                mpmsp->device_suspend_start_time = timestamp;
	    }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_DEVICE_SUSPEND_DONE:
            MPM_DPRINTK("MPM_TEST_DEVICE_SUSPEND_DONE\n");
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                mpmsp->device_suspend_done_time = timestamp;
                mpmsp->last_suspend_time =
                    mpmsp->device_suspend_done_time -
		    mpmsp->device_suspend_start_time;
                mpmsp->total_suspend_time += mpmsp->last_suspend_time;
	    }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_DEVICE_SUSPEND_INTERRUPTS_DISABLED_START:
            MPM_DPRINTK("MPM_TEST_DEVICE_SUSPEND_INTERRUPTS_DISABLED_START\n");
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                mpmsp->interrupt_disable_time = timestamp;
	    }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_DEVICE_SUSPEND_INTERRUPTS_DISABLED_DONE:
            MPM_DPRINTK("MPM_TEST_DEVICE_SUSPEND_INTERRUPTS_DISABLED_DONE\n");
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                mpmsp->interrupt_enable_time = timestamp;
                mpmsp->last_suspend_time_interrupts_blocked =
                    mpmsp->interrupt_enable_time -
		    mpmsp->interrupt_disable_time;
                mpmsp->total_suspend_time_interrupts_blocked +=
                    mpmsp->last_suspend_time_interrupts_blocked;
	    }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_DEVICE_SUSPEND_FAILED:
            MPM_DPRINTK("MPM_TEST_DEVICE_SUSPEND_FAILED\n");
	    data = va_arg(args, char *);
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                mpmsp->suspend_device_fail_count++;
                strncpy(mpmsp->failed_dev_name, data, MPM_FAILED_DEV_NAME_LEN);
                mpmsp->failed_dev_name[MPM_FAILED_DEV_NAME_LEN - 1] = '\0';
                mpmsp->failed_dev_name_len = strlen(mpmsp->failed_dev_name);
            }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_SUSPEND_CANCELLED_BY_USER_ACTIVITY:
            MPM_DPRINTK("MPM_TEST_SUSPEND_CANCELLED_BY_USER_ACTIVITY\n");
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                mpmsp->suspend_cancel_count++;
            }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_SUSPEND_DELAYED_BY_IOI:
            MPM_DPRINTK("MPM_TEST_SUSPEND_DELAYED_BY_IOI\n");
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                mpmsp->suspend_delay_ioi_count++;
            }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_SUSPEND_DELAYED_BY_PJ:
            MPM_DPRINTK("MPM_TEST_SUSPEND_DELAYED_BY_PJ\n");
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                mpmsp->suspend_delay_pj_count++;
            }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_DSM_ENTER:
            MPM_DPRINTK("MPM_TEST_DSM_ENTER\n");
            break;

        case MPM_TEST_WAIT_EXIT:
        case MPM_TEST_DOZE_EXIT:
        case MPM_TEST_STOP_EXIT:
        case MPM_TEST_DSM_EXIT:
	    mode = 0;
	    switch (test_point)
	    {
		case MPM_TEST_WAIT_EXIT:
		    mode = cpumode2indx(WAIT_MODE);
                    MPM_DPRINTK("MPM_TEST_WAIT_EXIT\n");
		    break;
                case MPM_TEST_DOZE_EXIT:
		    mode = cpumode2indx(DOZE_MODE);
                    MPM_DPRINTK("MPM_TEST_DOZE_EXIT\n");
		    break;
		case MPM_TEST_STOP_EXIT:
		    mode = cpumode2indx(STOP_MODE);
                    MPM_DPRINTK("MPM_TEST_STOP_EXIT\n");
		    break;
		case MPM_TEST_DSM_EXIT:
		    mode = cpumode2indx(DSM_MODE);
                    MPM_DPRINTK("MPM_TEST_DSM_EXIT\n");
		    break;
	    }

            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
	        indx = mpmsp->pmmode[mode].indx;

                mpmsp->pmmode[mode].timestamp[indx] = timestamp;
                mpmsp->pmmode[mode].count++;

	        rtc_before = va_arg(args, u32);
	        rtc_after = va_arg(args, u32);
	        avic_nipndl = va_arg(args, u32);
	        avic_nipndh = va_arg(args, u32);
	        mpmsp->pmmode[mode].avic_nipndl[indx] = avic_nipndl;
	        mpmsp->pmmode[mode].avic_nipndh[indx] = avic_nipndh;

#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST)  || defined(CONFIG_MACH_PAROS)
                if ((avic_nipndl & 0x10000000) && pjstat)
                {
                    pjwakeup = 1;
                }
#endif
		/*
	         * fls() returns the number of the first bit set in the
	         * argument starting with the most significant bit.  The
	         * most significant bit number is 32, the least is 1.
	         * fls() returns 0 if the argument is 0.
	         *
		 */
		while (avic_nipndl) {
		    i = fls(avic_nipndl) - 1;
                    mpmsp->pmmode[mode].interrupt_count[i]++;
		    avic_nipndl &= ~(1 << i);
		}
		while (avic_nipndh) {
		    i = fls(avic_nipndh) - 1;
                    mpmsp->pmmode[mode].interrupt_count[i+32]++;
		    avic_nipndh &= ~(1 << i);
		}

	        mpmsp->pmmode[mode].duration[indx] =
		    rtc_sw_internal_to_ms(rtc_before - rtc_after);

	        if (mpmsp->pmmode[mode].duration[indx] >
		    mpmsp->pmmode[mode].maxduration)
	        {
                    mpmsp->pmmode[mode].maxduration =
	                mpmsp->pmmode[mode].duration[indx];
	        }

	        if (mpmsp->pmmode[mode].duration[indx] <
		    mpmsp->pmmode[mode].minduration)
	        {
                    mpmsp->pmmode[mode].minduration =
	                mpmsp->pmmode[mode].duration[indx];
	        }

                mpmsp->pmmode[mode].total_time += mpmsp->pmmode[mode].duration[indx];

	        /*
	         * We set up buckets thusly.
	         *     0ms -   127ms  (bucket index  0)
	         *   128ms -   255ms  (bucket index  1)
	         *   256ms -   511ms  (bucket index  2)
	         *   512ms -  1023ms  (bucket index  3)
	         *  1024ms -  2047ms  (bucket index  4)
	         *  2048ms -  4095ms  (bucket index  5)
	         *  4096ms -  8191ms  (bucket index  6)
	         *  8192ms - 16383ms  (bucket index  7)
	         * 16384ms - 32767ms  (bucket index  8)
	         * 32768ms - 65535ms  (bucket index  9)
	         * >=65536ms          (bucket index 10)
	         */
	        bucket = fls(mpmsp->pmmode[mode].duration[indx]);
	        if (bucket <= 7)
		    bucket = 0;
	        else if (bucket >= 17)
		    bucket = MAX_DURATION_BUCKETS - 1;
	        else
		    bucket -= 7;
	        mpmsp->pmmode[mode].duration_bucket[bucket]++;

	        if (++mpmsp->pmmode[mode].indx == MAX_MODE_TIMES)
		    mpmsp->pmmode[mode].indx = 0;
            }
            spin_unlock_irqrestore(&mpmsplk, flags);

            break;

        case MPM_TEST_SLEEP_ATTEMPT_FAILED:
            MPM_DPRINTK("MPM_TEST_SLEEP_ATTEMPT_FAILED\n");
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                mpmsp->failed_dsm_count++;
            }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_STATE_TRANSITION_ERROR:
            MPM_DPRINTK("MPM_TEST_STATE_TRANSITION_ERROR\n");
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
	        indx = mpmsp->state_transition_error_indx;
                mpmsp->state_transition_error_count++;

	        data = va_arg(args, char *);
		mpmsp->transition_errors[indx].action = va_arg(args, u32);
		mpmsp->transition_errors[indx].oldstate = va_arg(args, u32);

		mpmsp->transition_errors[indx].timestamp = timestamp;
		strncpy(mpmsp->transition_errors[indx].caller, data,
			MPM_TRANS_ERR_CALLER_LEN);
                mpmsp->transition_errors[indx].
		        caller[MPM_TRANS_ERR_CALLER_LEN - 1] = '\0';

	        if (++mpmsp->state_transition_error_indx == MAX_TRANS_ERRORS)
		    mpmsp->state_transition_error_indx = 0;
            }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        case MPM_TEST_SUSPEND_DELAYED_BY_BUSY_DRIVERS:
            /* Don't print here because the volume of these messages is huge. */
            /* MPM_DPRINTK("MPM_TEST_SUSPEND_DELAYED_BY_BUSY_DRIVERS\n"); */
            spin_lock_irqsave(&mpmsplk, flags);
            if (LPM_STATS_ENABLED()) {
                spin_lock_irqsave(&mpm_driver_info_lock, driver_flags);

		indx = mpmsp->driver_stats.busy_count_indx;
                mpmsp->driver_stats.total_idle_delay_count++;
                mpmsp->driver_stats.timestamp[indx] = timestamp;
                memcpy(mpmsp->driver_stats.busy_arr[indx],
		       mpmdip->busy_p,
		       mpm_bitarray_size_in_bytes(mpmdip->size_in_bits));

	        if (++mpmsp->driver_stats.busy_count_indx == MAX_IDLE_DELAYS_SAVED)
		    mpmsp->driver_stats.busy_count_indx = 0;

                spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
            }
            spin_unlock_irqrestore(&mpmsplk, flags);
            break;

        default:
            MPM_DPRINTK(KERN_WARNING "Invalid test point event %d\n",
			(int)test_point);
            break;
    }

    va_end(args);
}
#endif

/*
 * Name: mpm_set_initiate_sleep_state
 *
 * Arguments: None.
 *
 * Return value: None.
 *
 * Notes:
 *   This function is called by sysfs prior to invoking low power modes.
 */
void mpm_set_initiate_sleep_state(void)
{
    mpm_state_change(__FUNCTION__, LPM_ACTION_SLEEP_REQUEST);
}


static ssize_t
mpm_read (struct file *fp, char *bufptr, size_t buf_sz, loff_t * ppos)
{
    int free_sp;
    mpm_event_t event;

    if (!capable (CAP_SYS_ADMIN))
        return -EPERM;

    if (buf_sz < sizeof (mpm_event_t))
        return -EINVAL;

    if (down_interruptible (&rd_sem))
    {
        return -ERESTARTSYS;
    }

    if (mpm_queue_empty ())
    {
        if (fp->f_flags & O_NONBLOCK)
        {
            up (&rd_sem);
            return -EAGAIN;
        }

        if (wait_event_interruptible (mpm_wq, !mpm_queue_empty ()))
        {
            up (&rd_sem);
            return -ERESTARTSYS;
        }
    }
    free_sp = buf_sz;
    while ((free_sp >= sizeof (event)) && !mpm_queue_empty ())
    {
        mpm_get_queued_event (&event);

        if (copy_to_user (bufptr, &event, sizeof (event)))
        {
            if (free_sp < buf_sz)
            {
                /* seems like we did successfully copy at least one event */
                printk(KERN_WARNING "MPM: read: copy_to_user failed\n");
                break;
            }
            up (&rd_sem);
            return -EFAULT;
        }
        bufptr += sizeof (event);
        free_sp -= sizeof (event);
    }
    up (&rd_sem);
    return buf_sz - free_sp;
}

static unsigned int
mpm_poll (struct file *fp, poll_table * wait)
{
    unsigned int mask = 0;

    if (!capable (CAP_SYS_ADMIN))
        return -EPERM;

    poll_wait (fp, &mpm_wq, wait);


    if (!mpm_queue_empty ())
    {
        mask = (POLLIN | POLLRDNORM);
    }

    return mask;
}



static int
mpm_ioctl (struct inode *inode, struct file *filp, u_int cmd, u_long arg)
{
    int retval = 0;
    mpm_phoneattr_t phone_attr = 0;
    mpm_op_t mpm_op;
#ifdef CONFIG_MOT_FEAT_PM_STATS
    mpm_event_t evt;
#endif
#ifdef MPM_DEBUG
    int i;
#endif
    int sm_cnt;
    mpm_sleepinterval_t *local_ptr = NULL;
#ifdef CONFIG_MOT_FEAT_PM_DESENSE
    mpm_desense_info_t desense;
#endif /* CONFIG_MOT_FEAT_PM_DESENSE */

    if (!capable (CAP_SYS_ADMIN))
        return -EPERM;

    switch (cmd)
    {
        case MPM_IOC_GET_NUMOP:
            /* return the number of operating point supported */
            if (put_user (num_op, (int __user *) arg))
            {
                retval = -EFAULT;
                goto out;
            }
            break;

        case MPM_IOC_GETALL_OP:
            if (copy_to_user ((mpm_op_t *) arg, valid_op,
                        (num_op * sizeof(valid_op[0]))))
            {
                retval = -EFAULT;
                goto out;
            }
            break;

        case MPM_IOC_GETCUR_OP:
            if (put_user (mpm_getcur_op(), (mpm_op_t __user *) arg))
            {
                retval = -EFAULT;
                goto out;
            }
            break;

        case MPM_IOC_SET_OP:
            if (get_user( mpm_op, (mpm_op_t __user *) arg))
            {
                retval = -EFAULT;
                goto out;
            }

#ifdef MPM_DEBUG
            for (i=0; i < num_op; i++ )
            {
                if ( mpm_op == valid_op[i])
                    break;
            }
            if ( i >= num_op )
            {
                MPM_DPRINTK("passed in OP is not valid %lu\n", mpm_op);
            }
#endif

            /* we assume a valid operating point is passed in */
            retval = mpm_set_op(mpm_op);

            break;

        case MPM_IOC_GET_NUMSLPMODES:
            if (put_user (num_sm, (int __user *) arg))
            {
                retval = -EFAULT;
                goto out;
            }
            break;

        case MPM_IOC_SET_SLEEP_INTERVALS:
            MPM_DPRINTK("MPM:  Setting sleep intervals\n");

            /* Get the number of sleep modes to use */
            __get_user(sm_cnt, &((mpm_sleepinterval_t __user *)arg)->cnt);

            /* Allocate memory for the struct using a local pointer
               and copy the provided data into it */
            local_ptr = kmalloc
                (offsetof(mpm_sleepinterval_t, si)+(sm_cnt*sizeof(int)),
                 GFP_KERNEL);
            if(local_ptr == NULL) {
                printk(KERN_ERR "MPM: malloc failed for sleep intervals\n");
                retval = -EFAULT;
                goto out;
            }
            if(copy_from_user
               (local_ptr->si,
                ((mpm_sleepinterval_t __user*)arg)->si,
                sizeof(int)*sm_cnt)) {
                kfree(local_ptr);
                retval = -EFAULT;
                goto out;
            }
            local_ptr->cnt = sm_cnt;

            /* If a previous set of intervals has already been set,
               update the pointer with the new struct, then free the old one */
            if(down_interruptible(&sleep_interval_array_sem)) {
                printk(KERN_ERR "Failed to get sleep_interval_array_sem\n");
                kfree(local_ptr);
                retval = -EFAULT;
                goto out;
            }
            kfree(sleep_intervals);
            sleep_intervals = local_ptr;
            up(&sleep_interval_array_sem);

            mpm_lpm_transition_interval = msecs_to_jiffies(sleep_intervals->si[0]);

#ifdef MPM_DEBUG
            MPM_DPRINTK("MPM:  Got number of sleep modes %d\n", sm_cnt);
            MPM_DPRINTK("MPM:  Got sleep intervals:\n");
            if(down_interruptible(&sleep_interval_array_sem)) {
                printk(KERN_ERR "Failed to get sleep_interval_array_sem\n");
            }
            else {
                for(i = 0; i < sm_cnt; i++) {
                    MPM_DPRINTK
                        ("sleep_intervals[%d]: %d\n",i,sleep_intervals->si[i]);
                }
                up(&sleep_interval_array_sem);
            }
#endif
            break;

        case MPM_IOC_INITIATE_SLEEP:
	    mpm_state_change(__FUNCTION__, LPM_ACTION_SLEEP_REQUEST);
            break;

        case MPM_IOC_AWAKE_REQ:
	    mpm_state_change(__FUNCTION__, LPM_ACTION_AWAKE_REQUEST);
            break;

        case MPM_IOC_CONFIG_DESENSE:
#ifdef CONFIG_MOT_FEAT_PM_DESENSE
            MPM_DPRINTK("MPM: Configuring desense values.\n");

	    /* Copy the desense structure from user space. */
            if (copy_from_user (&desense, (mpm_desense_info_t __user *)arg,
                                sizeof (mpm_desense_info_t)))
	    {
                retval = -EFAULT;
                goto out;
            }

            MPM_DPRINTK("MPM:     version: %d\n", desense.version);
            MPM_DPRINTK("MPM:     core_normal_pll_ppm: 0x%08x\n",
					desense.ap_core_normal_pll_ppm);
            MPM_DPRINTK("MPM:     core_turbo_pll_ppm:  0x%08x\n",
					desense.ap_core_turbo_pll_ppm);
            MPM_DPRINTK("MPM:     usb_pll_ppm:         0x%08x\n",
					desense.ap_usb_pll_ppm);

	    if (desense.version != MPM_DESENSE_INFO_VERSION_LATEST)
	    {
		retval = -EFAULT;
		goto out;
	    }

	    /* Send the information to the MXC PM driver. */
	    retval = mxc_pm_dither_setup (desense.ap_core_normal_pll_ppm,
				          desense.ap_core_turbo_pll_ppm,
				          desense.ap_usb_pll_ppm);

	    /*
	     * If we successfully get desense data from the BP but the
	     * data is bad (failure return from mxc_pm_dither_setup),
	     * then we panic the phone.  We want to ensure that bad
	     * flex data from the BP is caught and resolved.  In
	     * order to allow the phone to boot after the desense
	     * data is detected as invalid, the cmdline supports a
	     * no_desense_panic flag that can be set to "on".  This
	     * allows engineers to rebuild the ATAGs and successfully
	     * boot the phone in order to fix the invalid desense data.
	     */
	    if ((retval < 0) && mpm_panic_with_invalid_desense()) {
#ifdef CONFIG_MOT_FEAT_FB_PANIC_TEXT
		set_fb_panic_text("AP desense bad");
#endif
		panic("AP desense invalid: "
		      "core=0x%08x turbo=0x%08x usb=0x%08x",
		      desense.ap_core_normal_pll_ppm,
		      desense.ap_core_turbo_pll_ppm,
		      desense.ap_usb_pll_ppm);
	    }

	    /* Save information for later perusal. */
	    last_desense = desense;
	    last_desense_result = retval;

#else  /* !CONFIG_MOT_FEAT_PM_DESENSE */

            MPM_DPRINTK("MPM: Desense not implemented on this hardware.\n");

#endif /* CONFIG_MOT_FEAT_PM_DESENSE */

            break;

        case MPM_IOC_REFLASH:
            break;

#ifdef CONFIG_MOT_FEAT_PM_STATS
        case MPM_IOC_INSERT_DEV_EVENT:
            if (copy_from_user (&evt, (mpm_event_t *) arg,
                                sizeof (mpm_event_t)))
            {
                retval = -EFAULT;
                goto out;
            }

            mpm_event_notify (evt.type, evt.kind, evt.info);
            break;

        case MPM_IOC_PRINT_OPSTAT:
            mpm_print_opstat(buf, sizeof(buf));
            MPM_DPRINTK("printing buffer \n");
            MPM_DPRINTK("%s\n", buf);
            break;

        case MPM_IOC_START_OPSTAT:
            mpm_start_opstat();
            break;

        case MPM_IOC_STOP_OPSTAT:
            mpm_stop_opstat();
            break;

#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS)
        case MPM_IOC_PRINT_PJSTAT:
            mpm_print_pjstat(buf, sizeof(buf));
            MPM_DPRINTK("printing buffer \n");
            MPM_DPRINTK("%s\n", buf);
            break;

        case MPM_IOC_START_PJSTAT:
            mpm_start_pjstat(1);
            break;

        case MPM_IOC_STOP_PJSTAT:
            mpm_stop_pjstat();
            break;
#endif


#endif
        case MPM_IOC_GET_PHONEATTR:
#if defined(CONFIG_MOT_FEAT_FLIP)
            phone_attr |= MPMA_FLIP;
#endif
#if defined(CONFIG_MOT_FEAT_SLIDER)
            phone_attr |= MPMA_SLIDER;
#endif
#if defined(CONFIG_MOT_FEAT_CANDYBAR)
            phone_attr |= MPMA_CANDYBAR;
#endif
#if defined(CONFIG_MACH_SCMA11REF)
	    /*
	     * On the SCMA11ref, there is no flip detection available
	     * because the hardware doesn't support it.
	     * We can't enable CONFIG_MOT_FEAT_FLIP for SCMA11ref
	     * because the GPIO functions don't exist.  In order
	     * to work around the issue, we detect here whether
	     * we're building for SCMA11ref and set the phoneattr to
	     * indicate that the SCMA11ref is a flip phone.
	     */
	    phone_attr |= MPMA_FLIP;
            phone_attr &= ~MPMA_CANDYBAR;
	    phone_attr &= ~MPMA_SLIDER;
#endif
            if (put_user (phone_attr, (mpm_phoneattr_t __user *) arg))
            {
                retval = -EFAULT;
                goto out;
            }
            break;
        default:
            retval = -EINVAL;
            break;
    }
  out:
    return retval;
}

static atomic_t mpm_avail = ATOMIC_INIT (1);

static int
mpm_release (struct inode *inode, struct file *filp)
{
    atomic_inc (&mpm_avail);    /* release the device */

    return 0;
}

void mpm_start_opstat(void)
{
#ifdef CONFIG_MOT_FEAT_PM_STATS
    mpm_op_t cf;
    struct timeval stv;
    if ( opstat == 1 )
    {
        /* stat collection already started */
        return;
    }
    if (opstat_cache == NULL )
    {
        opstat_cache = kmem_cache_create("opstat",
            sizeof(mpm_opst_list_t), 0, SLAB_HWCACHE_ALIGN,
            NULL, NULL);
        if (opstat_cache == NULL )
        {
            MPM_DPRINTK("Error creating slab cache \n");
            return;
        }
    }

    opstat = 1;
    cf =  mpm_getcur_op();
    do_gettimeofday(&stv);
    collect_op_stat(cf, &stv, NULL );

#endif
}


void mpm_stop_opstat(void)
{
#ifdef CONFIG_MOT_FEAT_PM_STATS
    struct timeval etv;
    if (opstat == 1)
    {
        opstat = 0;
        do_gettimeofday(&etv);
        collect_op_stat(0, NULL, &etv);
    }
#endif
}


#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS)
void mpm_start_pjstat(int mode)
{
#ifdef CONFIG_MOT_FEAT_PM_STATS
    if (pjstat == 1)
    {
        /* wakeups collection already started, we only change mode */
        pjsmod = mode;
        return;
    }

    if (mode == 0)
    {
        /* collection mode is incorrect */
        return;
    }

    if (pjistat_cache == NULL )
    {
        pjistat_cache = kmem_cache_create("pjistat",
            sizeof(mpm_pjist_list_t), 0, SLAB_HWCACHE_ALIGN,
            NULL, NULL);
        if (pjistat_cache == NULL )
        {
            MPM_DPRINTK("Error creating slab cache \n");
            return;
        }
    }
    if (pjcstat_cache == NULL )
    {
        pjcstat_cache = kmem_cache_create("pjcstat",
            sizeof(mpm_pjcst_list_t), 0, SLAB_HWCACHE_ALIGN,
            NULL, NULL);
        if (pjcstat_cache == NULL )
        {
            MPM_DPRINTK("Error creating slab cache \n");
            return;
        }
    }

    pjstat = 1;
    pjsmod = mode;
    pjwakeup = 0;
    pjstime = 0;
#endif
}

void mpm_stop_pjstat(void)
{
#ifdef CONFIG_MOT_FEAT_PM_STATS
    if (pjstat == 1)
    {
        pjstat = 0;
        pjsmod = 0;
        flush_pjlist();
    }
#endif
}


#ifdef CONFIG_MOT_FEAT_PM_STATS
static void update_pjstat(mpm_pjstat_t *pjstat, int waiting)
{
    int i = 0;
    unsigned long ctv = 0;
    unsigned long duration = 0;

    ctv = rtc_sw_msfromboot();

    if (pjstat->waiting == waiting)
    {
        MPM_DPRINTK("The same periodic job run again before done \n");
        pjstat->cur_time = ctv;
        return;
    }

    pjstat->waiting = waiting;
    if (waiting)
    {
        pjstat->cur_time = ctv;
        return;
    }

    duration = ctv - pjstat->cur_time;
    pjstat->cur_time = ctv;
    pjstat->count ++;

    i = fls(duration);
    if (i <= 2)
    {
        i = 0;
    }
    else if (i >= (MAX_BUCKETS + 1))
    {
        i = MAX_BUCKETS - 1;
    }
    else
    {
        i -= 2;
    }

    if (pjwakeup)
    {
        pjstat->wakeuptimes[i] ++;
        pjwakeup = 0;
    }
    pjstat->averageduration[i] = ((pjstat->averageduration[i] * \
                                   pjstat->durationcount[i]) + \
                                  duration) / \
                                 (pjstat->durationcount[i] + 1);
    pjstat->durationcount[i] ++;

    if (pjstat->maxduration < duration)
    {
        pjstat->maxduration = duration;
    }
    if (pjstat->minduration > duration)
    {
        pjstat->minduration = duration;
    }
}

static void
init_pjstat(mpm_pjstat_t *pjstat, int waiting)
{
    int i = 0;
    unsigned long ctv = 0;

    ctv = rtc_sw_msfromboot();

    pjstat->cur_time = ctv;
    pjstat->count = 0;
    pjstat->maxduration = 0;
    pjstat->minduration = -1;
    for (i = 0; i < MAX_BUCKETS; i++)
    {
        pjstat->averageduration[i] = 0;
        pjstat->durationcount[i] = 0;
        pjstat->wakeuptimes[i] = 0;
    }
    pjstat->waiting = waiting;
}


static void store_pjist(const unsigned long orig_func, int waiting)
{
    mpm_pjist_list_t *pji_node = NULL;
    static mpm_pjist_list_t *curr_node = NULL;

    pji_node = pji_head;
    while (pji_node != NULL)
    {
        if (pji_node->orig_func == orig_func)
        {
            update_pjstat(& (pji_node->pjstat), waiting);
            break;
        }
        pji_node = pji_node->next;
    }

    if (pji_node == NULL)
    {
        pji_node = kmem_cache_alloc(pjistat_cache, GFP_KERNEL);
        if ( pji_node == NULL )
        {
            MPM_DPRINTK("Cache alloc returned NULL \n");
            return;
        }

        init_pjstat(& (pji_node->pjstat), waiting);
        pji_node->orig_func = orig_func;
        pji_node->next = NULL;

        if ( pji_head == NULL )
        {
            /* starting to build the list */
            pji_head = pji_node;
            if (pjstime == 0)
            {
                pjstime = rtc_sw_msfromboot();
            }
        }
        else
        {
            curr_node->next = pji_node;
        }
        curr_node = pji_node;
    }
}


static void store_pjcst(const char *comm, const int pid, int waiting)
{
    int commlen = 0;
    mpm_pjcst_list_t *pjc_node = NULL;
    static mpm_pjcst_list_t *curr_node = NULL;

    pjc_node = pjc_head;
    while (pjc_node != NULL)
    {
        if (pjc_node->pid == pid)
        {
            update_pjstat(& (pjc_node->pjstat), waiting);
            break;
        }
        pjc_node = pjc_node->next;
    }

    if (pjc_node == NULL)
    {
        pjc_node = kmem_cache_alloc(pjcstat_cache, GFP_KERNEL);
        if ( pjc_node == NULL )
        {
            MPM_DPRINTK("Cache alloc returned NULL \n");
            return;
        }

        init_pjstat(& (pjc_node->pjstat), waiting);
        pjc_node->pid = pid;
        commlen = strlen(comm);
        if ( commlen > (COMM_SIZE - 1) )
        {
            commlen = COMM_SIZE - 1;
        }
        memcpy(pjc_node->process_comm, comm, commlen);
        pjc_node->process_comm[commlen] = '\0';
        pjc_node->next = NULL;

        if ( pjc_head == NULL )
        {
            /* starting to build the list */
            pjc_head = pjc_node;
            if (pjstime == 0)
            {
                pjstime = rtc_sw_msfromboot();
            }
        }
        else
        {
            curr_node->next = pjc_node;
        }
        curr_node = pjc_node;
    }
}

static void mpm_collect_pj_stat_callback(const unsigned long orig_func,
                                         const char *comm,
                                         const int pid)
{
    int clpmstat = 0;

    if ( pjstat == 0 || comm == NULL )
    {
        return;
    }

    clpmstat = atomic_read(&mpm_lpm_state);

    if ( pjsmod != 1 && clpmstat == LPM_STATE_AWAKE )
    {
        return;
    }

    if ( orig_func == 0 )
    {
        store_pjcst(comm, pid, 0);
    }
    else if ( orig_func == 1 )
    {
        store_pjcst(comm, pid, 1);
    }
    else
    {
        store_pjist(orig_func, pid);
    }
}
#endif

#endif
static int
mpm_open (struct inode *inode, struct file *filp)
{
    if ((!capable (CAP_SYS_ADMIN)) || ((filp->f_flags & O_ACCMODE) != O_RDONLY))
    {
        return -EPERM;    /* wrong mode */
    }

/* Allow only one process to open the mpm device. */

    if (!atomic_dec_and_test (&mpm_avail))
    {
        atomic_inc (&mpm_avail);
        return -EBUSY;    /* already open */
    }
    return 0;
}

static int
mpm_stats_open (struct inode *inode, struct file *filp)
{
    if ((!capable (CAP_SYS_ADMIN)) || ((filp->f_flags & O_ACCMODE) != O_RDONLY))
    {
        return -EPERM;    /* wrong mode */
    }

    return 0;
}

static int
mpm_stats_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static int
mpm_stats_ioctl (struct inode *inode, struct file *filp, u_int cmd, u_long arg)
{
    int retval = 0;
    mpmstats_req_t mpmstats_req;
#ifdef CONFIG_MOT_FEAT_PM_STATS
    mpm_stats_t *sp;
    unsigned long mpmsplk_flags;
    unsigned long driver_flags;
    char **name_arr;
    char *namep;
    char *user_namep;
    unsigned long *busy_arr;
    unsigned long *user_busy_arr;
    int user_size;
    int namelen;
    int i;
#endif

    if (!capable (CAP_SYS_ADMIN))
        return -EPERM;

    switch (cmd)
    {
        case MPM_STATS_IOC_GET_MPMSTATS_SIZE:
            spin_lock_irqsave(&mpm_driver_info_lock, driver_flags);
            user_size = sizeof(mpm_stats_t) + mpm_get_extra_size();
            spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);

            if (put_user (user_size, (int __user *) arg))
                retval = -EFAULT;

	    break;

        case MPM_STATS_IOC_GET_MPMSTATS:
	    /* Copy in the request structure from the caller. */
            if (copy_from_user (&mpmstats_req, (mpmstats_req_t *) arg,
				sizeof(mpmstats_req_t)))
	    {
		retval = -EFAULT;
		break;
	    }

	    /*
	     * The size must be at least the size of the mpm_stats_t
	     * structure, so check that first.  Further checks will be
	     * made if the caller provided at least the size of the
	     * mpm_stats_t structure.
	     */
	    if (mpmstats_req.size < sizeof(mpm_stats_t))
	    {
		retval = -EINVAL;
		break;
	    }

#ifdef CONFIG_MOT_FEAT_PM_STATS
	    if (!LPM_STATS_ENABLED())
	    {
		/*
		 * If we clear the entire structure, then we also
		 * clear the stats_enabled flag in the structure.  If
		 * stats_enabled is 0, then none of the data in the
		 * structure is valid.  We just clear it all to be
		 * consistent.
		 */
		if (clear_user (mpmstats_req.addr, sizeof(mpm_stats_t)))
		{
                    retval = -EFAULT;
                    break;
		}
	    }
	    else
	    {
try_again:
		/*
		 * Get the size of the space needed to return to the
		 * user and allocate the memory.  We may yet lose the
		 * race if another thread changes the size after we
		 * release the lock, but we'll detect it and retry the
		 * allocation.
		 */
	        spin_lock_irqsave(&mpm_driver_info_lock, driver_flags);
		user_size = sizeof(mpm_stats_t) + mpm_get_extra_size();
		spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);

	        if (mpmstats_req.size < user_size)
		{
		    retval = -EOVERFLOW;
		    break;
		}

		sp = kmalloc (user_size, GFP_KERNEL);
		if (sp == NULL)
		{
		    retval = -ENOMEM;
		    break;
		}

	        spin_lock_irqsave(&mpmsplk, mpmsplk_flags);

	        if (!LPM_STATS_ENABLED()) {
		    memset(sp, 0, user_size);
	        } else {
	            spin_lock_irqsave(&mpm_driver_info_lock, driver_flags);

		    /*
		     * Check if the size changed while we didn't hold
		     * the lock.  If it did, then just go back until
		     * we don't lose the race.
		     */
                    if (user_size != sizeof(mpm_stats_t) + mpm_get_extra_size())
		    {
	                spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
	                spin_unlock_irqrestore(&mpmsplk, mpmsplk_flags);
			kfree(sp);
			goto try_again;
		    }

		    memcpy(sp, mpmsp, sizeof(mpm_stats_t));
		    memcpy(&sp->driver_info, mpmdip, sizeof(mpm_driver_info_t));

		    /* Convert to user addresses. */
                    sp->driver_info.registered_p =
                        u_registered_bitarray_addr(mpmstats_req.addr,
                                                   mpmdip->size_in_bits);

                    sp->driver_info.busy_p =
			u_busy_bitarray_addr(mpmstats_req.addr,
                                             mpmdip->size_in_bits);

		    sp->driver_info.name_arr =
			u_name_ptr_array_addr(mpmstats_req.addr,
                                              mpmdip->size_in_bits);

                    name_arr = u_name_ptr_array_addr(sp, mpmdip->size_in_bits);

		    memcpy(u_registered_bitarray_addr(sp, mpmdip->size_in_bits),
			   mpmdip->registered_p,
			   sizeof_registered_bitarray(mpmdip->size_in_bits));
		    memcpy(u_busy_bitarray_addr(sp, mpmdip->size_in_bits),
			   mpmdip->busy_p,
			   sizeof_busy_bitarray(mpmdip->size_in_bits));

		    /*
		     * namep is the address in sp where the driver
		     * name strings reside. user_namep is the
		     * corresponding user address for the start of the
		     * strings.
		     */
		    namep = u_name_strings_addr(sp, mpmdip->size_in_bits);
                    user_namep = u_name_strings_addr(mpmstats_req.addr,
						     mpmdip->size_in_bits);

		    /*
		     * Copy the driver name strings and the user space
		     * pointers to the proper locations in sp.
		     */
		    for (i=0; i<mpmdip->size_in_bits; i++)
		    {
		        if (mpmdip->name_arr[i] != NULL)
		        {
			    namelen = strlen(mpmdip->name_arr[i]);
                            strcpy(namep, mpmdip->name_arr[i]);
			    name_arr[i] = user_namep;
			    namep += namelen + 1;
			    user_namep += namelen + 1;
		        }
			else
			{
			    name_arr[i] = NULL;
			}
		    }

		    /*
		     * Copy the busy_arr and the user space
		     * pointers to the proper locations in sp.
		     */
		    busy_arr =
			u_busy_arr_bitarrays_addr(sp,
                                                  mpmdip->size_in_bits,
                                                  mpmdip->total_name_size);
                    user_busy_arr =
			u_busy_arr_bitarrays_addr(mpmstats_req.addr,
                                                  mpmdip->size_in_bits,
                                                  mpmdip->total_name_size);
		    memcpy(busy_arr, mpmsp->driver_stats.busy_arr[0],
			   sizeof_busy_arr_bitarrays(mpmdip->size_in_bits));

		    for (i=0; i<MAX_IDLE_DELAYS_SAVED; i++)
		    {
                        sp->driver_stats.busy_arr[i] = user_busy_arr;
                        user_busy_arr +=
                             mpm_bitarray_size_in_bytes(mpmdip->size_in_bits) /
			     sizeof(unsigned long);
		    }

	            spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
		}

	        spin_unlock_irqrestore(&mpmsplk, mpmsplk_flags);

	        sp->cur_time = rtc_sw_msfromboot();

                if (copy_to_user (mpmstats_req.addr, sp, user_size))
                {
                    retval = -EFAULT;
	        }

		kfree(sp);
	    }
#else  /* ! CONFIG_MOT_FEAT_PM_STATS */
	    if (clear_user (mpmstats_req.addr, sizeof(mpm_stats_t)))
	    {
                retval = -EFAULT;
                break;
	    }
#endif /* CONFIG_MOT_FEAT_PM_STATS */
            break;

        default:
            retval = -EINVAL;
            break;
    }
  out:
    return retval;
}

#ifdef CONFIG_PM

/*!
 * This function is called to put the mpm driver in a low power state.
 *
 * @param   dev   the device structure used to give information on mpm
 *                to suspend.
 * @param   state the power state the device is entering.
 * @param   level the stage in device suspension process that we want the
 *                device to be put in.
 *
 * @return  The function always returns 0.
 */
static int
mpm_suspend (struct device *dev, u32 state, u32 level)
{
    switch (level)
    {
        case SUSPEND_DISABLE:
            break;

        case SUSPEND_SAVE_STATE:
            break;

        case SUSPEND_POWER_DOWN:
            break;
    }
    return 0;
}

/*!
 * This function is called to bring the mpm  back from a low power state.
 *
 * @param   dev   the device structure used to give information on mpm
 *                to resume.
 * @param   level the stage in device resumption process that we want the
 *                device to be put in.
 *
 * @return  The function always returns 0.
 */
static int
mpm_resume (struct device *dev, u32 level)
{
    switch (level)
    {
        case RESUME_POWER_ON:
            break;

        case RESUME_RESTORE_STATE:
            break;

        case RESUME_ENABLE:
            break;
    }
    return 0;
}


static void mpm_remove(struct device *dev)
{
}

#else
#define mpm_suspend NULL
#define mpm_resume  NULL
#define mpm_remove NULL
#endif /* CONFIG_PM */

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mpm_drv = {
    .name = MPM_DEVICE_NAME,
    .bus = &platform_bus_type,
    .suspend = mpm_suspend,
    .resume = mpm_resume,
};

/*! Device Definition for mpm. */
static struct platform_device mpm_dev = {
    .name = MPM_DEVICE_NAME,
    .id = 0,
    .dev = {
        .release = mpm_remove,
    },
};




static struct file_operations mpm_fops = {
    .owner = THIS_MODULE,
    .read = mpm_read,
    .poll = mpm_poll,
    .ioctl = mpm_ioctl,
    .open = mpm_open,
    .release = mpm_release,
};
static struct miscdevice mpm_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MPM_DEVICE_NAME,
    .fops = &mpm_fops,
    .devfs_name = MPM_DEVICE_NAME,
};

static struct file_operations mpm_stats_fops = {
    .owner = THIS_MODULE,
    .open = mpm_stats_open,
    .ioctl = mpm_stats_ioctl,
    .release = mpm_stats_release,
};

static struct miscdevice mpm_stats_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MPM_STATS_DEVICE_NAME,
    .fops = &mpm_stats_fops,
    .devfs_name = MPM_STATS_DEVICE_NAME,
};

static struct mpm_callback_fns mpm_callback_functions = {
    .handle_ioi = mpm_handle_ioi_callback,
    .handle_long_ioi = mpm_handle_long_ioi_callback,
    .periodic_jobs_started = mpm_periodic_jobs_started_callback,
    .periodic_jobs_done = mpm_periodic_jobs_done_callback,
    .set_awake_state = mpm_set_awake_state_callback,
    .ready_to_sleep = mpm_ready_to_sleep_callback,
    .start_sleep = mpm_start_sleep_callback,
    .resume_from_sleep = mpm_resume_from_sleep_callback,
#ifdef CONFIG_MOT_FEAT_PM_STATS
    .report_test_point = mpm_report_test_point_callback,
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS)
    .collect_pj_stat = mpm_collect_pj_stat_callback,
#endif
#endif
};

static int __init
mpm_init (void)
{
    int retval;

    /*
     * PASS2 and higher.  We no longer support PASS1.
     */
#ifdef CONFIG_MACH_ARGONLVPHONE
    valid_op = valid_op_init;
    num_op = sizeof(valid_op_init)/sizeof(valid_op_init[0]);
#elif CONFIG_MACH_SCMA11PHONE
    valid_op = valid_op_pass2;
    num_op = sizeof(valid_op_pass2)/sizeof(valid_op_pass2[0]);
#endif // CONFIG_MACH_ARGONLVPHONE || CONFIG_MACH_SCMA11PHONE

    /* register with linux pm subsystem */
    retval = driver_register (&mpm_drv);
    if (retval)
    {
        printk(KERN_ERR "MPM: Failed to register driver; "
			"exiting retval=%d\n", retval);
	return retval;
    }

    retval = platform_device_register (&mpm_dev);
    if (retval)
    {
        printk(KERN_ERR "MPM: Failed to register with platform; "
			"exiting retval=%d\n", retval);
        driver_unregister (&mpm_drv);
        return retval;
    }

    retval = misc_register (&mpm_device);
    if (retval)
    {
        printk (KERN_ERR "MPM: can't misc_register mpm_device "
			 "retval=%d\n", retval);
        platform_device_unregister(&mpm_dev);
        driver_unregister(&mpm_drv);
        return retval;
    }

    retval = misc_register (&mpm_stats_device);
    if (retval)
    {
        printk (KERN_ERR "MPM: can't misc_register mpm_stats_device "
			 "retval=%d\n", retval);
	misc_deregister (&mpm_device);
        platform_device_unregister(&mpm_dev);
        driver_unregister(&mpm_drv);
        return retval;
    }

    /* initialise sysfs */
    retval = init_mpm_sysfs();
    if (retval != 0)
        printk (KERN_WARNING "MPM: unable to register mpm sysfs subsystem "
			     "retval=%d\n", retval);

    orig_lpj = loops_per_jiffy;
    orig_freq = (mpm_getcur_op())/1000;
    printk(KERN_WARNING "Current cpu frequency: %lu Mhz \n", orig_freq/1000);

    /* Initialize mpm_last_ioi_timestamp to current time in jiffies */
    mpm_last_ioi_timestamp = jiffies;
    mpm_interrupt_inactivity_interval = msecs_to_jiffies (MPM_IOI_TIMEOUT);
    mpm_interrupt_inactivity_long_interval = msecs_to_jiffies (MPM_LONG_IOI_TIMEOUT);
    mpm_retry_sleep_interval = msecs_to_jiffies (MPM_RETRY_TIMEOUT);

    /* Create a new worker thread */
    suspend_worker_thread = create_workqueue(WORK_QUEUE_NAME);

    /* Initialize suspend timer mpm_suspend_timer structure. */
    init_timer(&mpm_suspend_timer);
    mpm_suspend_timer.data = 0;
    mpm_suspend_timer.function = mpm_suspend_timeout_handler; // Suspend timer interrupt handler

    /* Register callback functions with the static portion of mpm driver */
    mpm_callback_register(&mpm_callback_functions);

#ifdef CONFIG_MOT_FEAT_PM_STATS
    /* Initialize lpm stats */
    mpm_reset_lpm_stats();
#endif
    mot_otp_get_mode(&security_mode, NULL);

#ifdef CONFIG_MOT_DSM_INDICATOR
    lights_enter_turbo_mode();
#endif

    return 0;
}

static void __exit
mpm_exit (void)
{
    /* Cleanup before exit.. deregister callback functions, stop timers, flush workqueue */
    mpm_callback_deregister();

    /* Wait on all pending work on the given worker thread */
    flush_workqueue(suspend_worker_thread);

    /* Destroy a worker thread */
    destroy_workqueue(suspend_worker_thread);

#ifdef CONFIG_MOT_FEAT_PM_STATS
    if ( opstat_cache != NULL )
    {
        kmem_cache_destroy(opstat_cache);
        opstat_cache = NULL;
    }
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS)
    if ( pjistat_cache != NULL )
    {
        kmem_cache_destroy(pjistat_cache);
        pjistat_cache = NULL;
    }
    if ( pjcstat_cache != NULL )
    {
        kmem_cache_destroy(pjcstat_cache);
        pjcstat_cache = NULL;
    }
#endif
#endif
    /* clean up sysfs */
    exit_mpm_sysfs();

    platform_device_unregister(&mpm_dev);
    driver_unregister(&mpm_drv);
    misc_deregister(&mpm_device);
    misc_deregister(&mpm_stats_device);

    /* free sleep_intervals memory */
    kfree(sleep_intervals);
}



#ifdef CONFIG_MOT_FEAT_PM_STATS

/*  There are 4 conditions:
    1. Both start time and end time are NULL. In this case
       reject the call and just return. This is an invalid/error case.
    2. Start time is not NULL, but End time is NULL. In this case,
    we have just started to collect op statistics. Since, we don't have
    the end time yet, just store the start time and freq and return. Do not
    allocate memory for node.
    3. Start time and End time are not NULL. This is the case where a frequency
    change has happened. So, the End time corresponds to the previous freq,
    and the Start time is for the current/new frequency. Since, we have now
    got the End time for prev frequency, allocate memory for the node, and store
    the saved start time, saved freq and the just received End time. Save
    the current freq and Start time (for current freq) in local variables.
    4. Start time is NULL, but End time is not NULL. This is the case where we
    have stopped collecting op stats. Since, we already have the start time
    and freq saved in local variables, we can now allocate a new node and save
    all the three values in the structure.  There is nothing to be saved again
    in the local variables.

*/

static void
collect_op_stat (mpm_op_t freq, struct timeval *stv,
                                 struct timeval *etv)
{
    mpm_opst_list_t *op_node = NULL;
    static mpm_opst_list_t *curr_node = NULL;
    static struct timeval pstv;
    static mpm_op_t pfreq = 0;
    if ( stv == NULL && etv == NULL )
    {
        return;
    }

    /* Just save the freq and start time */
    if ( stv != NULL  && etv == NULL )
    {
        /* This is the first node */
        pstv = *stv;
        pfreq = freq;
        return;
    }


    /* Ok so now we have both start time and end time */
    /* Allocate the node and save them */

    op_node = kmem_cache_alloc(opstat_cache, GFP_KERNEL);

    if ( op_node == NULL )
    {
        MPM_DPRINTK("Cache alloc returned NULL \n");
        return;
    }

    op_node->opst.cpu_freq = pfreq;
    op_node->opst.start_time = pstv;
    op_node->opst.end_time = *etv;
    op_node->next = NULL;

    if ( op_head == NULL )
    {
        /* starting to build the list */
        /* both op_head and curr_node should be NULL */
        op_head = op_node;
    }
    else
    {
        curr_node->next = op_node;
    }

    curr_node = op_node;
    if ( stv != NULL )
    {
        /* store the start time and freq for next node */
        pstv = *stv;
        pfreq = freq;
    }

}


static void flush_oplist(void)
{
    mpm_opst_list_t *node;
    while ( op_head != NULL )
    {
        node = op_head->next;
        kmem_cache_free(opstat_cache, op_head);
        op_head = node;
    };


}

#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS)
static void flush_pjlist(void)
{
    mpm_pjist_list_t *pji_node;
    mpm_pjcst_list_t *pjc_node;

    while ( pji_head != NULL )
    {
        pji_node = pji_head->next;
        kmem_cache_free(pjistat_cache, pji_head);
        pji_head = pji_node;
    }
    while ( pjc_head != NULL )
    {
        pjc_node = pjc_head->next;
        kmem_cache_free(pjcstat_cache, pjc_head);
        pjc_head = pjc_node;
    }
    pjstime = 0;
}
#endif
#endif


void mpm_print_opstat(char *buf, int buflen)
{
#ifdef CONFIG_MOT_FEAT_PM_STATS
    int j, restart_opstat=0, skip_cumt=0;
    mpm_op_t of, nf;
    long *op_cumt;
    mpm_opst_list_t *node;
    struct timeval t;

    if ( buf == NULL )
    {
        MPM_DPRINTK("passed in buffer is NULL \n");
        return;
    }

    buf[0] = '\0';

    if ( op_head == NULL )
    {
        MPM_DPRINTK("OP Stat collection not started \n");
        return;
    }

    if (opstat == 1 )
    {
        restart_opstat = 1;
        mpm_stop_opstat();
    }

    op_cumt = (long *)kmalloc(sizeof(long)*num_op, GFP_KERNEL);

    if (op_cumt == NULL )
    {
        skip_cumt = 1;
    }
    else
    {
        for ( j = 0; j < num_op; j++ )
        {
            op_cumt[j] = 0;
        }
    }

    snprintf(buf, buflen-1, "Freq(hz)    Start time(sec.usec)"  \
        "    End time(sec.usec)   Total Time(sec.usec) \n");


    for ( node = op_head; node != NULL; node = node->next )
    {
        if ( eusec(node) < susec(node) )
        {
            eusec(node) += 1000000;
            --esec(node);
        }
        t.tv_usec = eusec(node) - susec(node);
        t.tv_sec = esec(node) - ssec(node);

        snprintf(buf+strlen(buf), buflen-strlen(buf)-1, "%10lu",  \
                node->opst.cpu_freq);

        snprintf(buf+strlen(buf), buflen-strlen(buf)-1, "%10lus.%06luu" \
         "  %10lus.%06luu   %10lus.%06luu \n", ssec(node), \
         susec(node),  esec(node), eusec(node), t.tv_sec, t.tv_usec);

        if ( skip_cumt == 1)
        {
            continue;
        }

        /* calculate cumulative time spent in each freq */
        for ( j = 0; j < num_op ; j++ )
        {
            if ( valid_op[j] == node->opst.cpu_freq )
            {

                op_cumt[j] += timeval_to_jiffies(&t);
                break;
            }
        }
    }


    /* calculate latency time to move from old freq to new freq */

    snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
    "\nOld Freq(hz)  New Freq (hz)    lat time(s.usec)\n");

    for ( node = op_head; node != NULL; node = node->next )
    {
        if ( node->next != NULL )
        {
            of = node->opst.cpu_freq;
            nf = node->next->opst.cpu_freq;
            if ( susec(node->next) < eusec(node))
            {
                --ssec(node->next);
                susec(node->next) += 1000000;
            }
            t.tv_sec = ssec(node->next) - esec(node);
            t.tv_usec = susec(node->next) - eusec(node);
            snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
            "%lu     %lu         %lu(sec).%lu(usec)\n", of,  nf, \
            t.tv_sec, t.tv_usec);
        }
    }

    if ( skip_cumt == 0 )
    {

        snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
        "\nFrequency(hz)  cum time(Jiffies)\n");

        for ( j = 0; j < num_op; j++ )
        {
            snprintf(buf+strlen(buf),buflen-strlen(buf)-1, \
            "%10lu      %10lu\n", valid_op[j], op_cumt[j]);
        }

        kfree(op_cumt);
    }

    flush_oplist();
    if (restart_opstat == 1)
    {
        mpm_start_opstat();
    }
#endif
}


#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS)
void mpm_print_pjstat(char *buf, int buflen)
{
#ifdef CONFIG_MOT_FEAT_PM_STATS
    int i = 0;
    unsigned long totaltime = 0;
    mpm_pjist_list_t *pji_node;
    mpm_pjcst_list_t *pjc_node;

    if ( buf == NULL )
    {
        MPM_DPRINTK("passed in buffer is NULL \n");
        return;
    }

    buf[0] = '\0';

    if ( pji_head == NULL && pjc_head == NULL )
    {
        MPM_DPRINTK("Wakeup Stat collection not started \n");
        return;
    }

    totaltime = rtc_sw_msfromboot();
    totaltime = totaltime - pjstime;
    snprintf(buf, buflen-1, "Periodic job information..." \
             "\nCollection time: %ld (ms)\n", totaltime);
    if (! LPM_STATS_ENABLED())
    {
        snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                 "Please enable LPM stat to get wakeup_times\n");
    }

    if ( pji_head == NULL )
    {
        snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                "No interrupt source...\n");
    }
    else
    {
        pji_node = pji_head;
        while ( pji_node != NULL )
        {
            snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                    "\n\norig_func  " \
                    "total times\tmax duration\tmin duration\n");
            snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                    "%8x   %d\t\t", pji_node->orig_func, \
                    pji_node->pjstat.count);
            if (pji_node->pjstat.count > 0)
            {
                snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                        "%lu\t\t%lu\n", \
                        pji_node->pjstat.maxduration, \
                        pji_node->pjstat.minduration);

                snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                        "    Count of durations by range: (ms)\n\t" \
                        "duration:\ttimes:\taverage:  wakeup_times:\n");
                if (pji_node->pjstat.durationcount[0])
                {
                    snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                             "\t%-4d - %-4d\t%d\t%d\t  %d\n", 0, 1<<2, \
                             pji_node->pjstat.durationcount[0], \
                             pji_node->pjstat.averageduration[0], \
                             pji_node->pjstat.wakeuptimes[0]);
                }
                for (i = 1; i < MAX_BUCKETS-1; i++)
                {
                    if (pji_node->pjstat.durationcount[i])
                    {
                        snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                                "\t%-4d - %-4d\t%d\t%d\t  %d\n", 1<<(i+1), 1<<(i+2), \
                                pji_node->pjstat.durationcount[i], \
                                pji_node->pjstat.averageduration[i], \
                                pji_node->pjstat.wakeuptimes[i]);
                    }
                }
                if (pji_node->pjstat.durationcount[i])
                {
                    snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                             "\t      > %-4d\t%d\t%d\t  %d\n", 1<<(i+1), \
                             pji_node->pjstat.durationcount[i], \
                             pji_node->pjstat.averageduration[i], \
                             pji_node->pjstat.wakeuptimes[i]);
                }
            }
            else
            {
                snprintf(buf+strlen(buf), buflen-strlen(buf)-1, "#\t\t#\n");
            }

            if (buflen <= strlen(buf))
            {
                flush_pjlist();
                MPM_DPRINTK("Warning: buffer is full \n");
                return;
            }
            pji_node = pji_node->next;
        }
    }

    if ( pjc_head == NULL )
    {
        snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                "No call back source...\n");
    }
    else
    {
        pjc_node = pjc_head;
        while ( pjc_node != NULL )
        {
            snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                    "\n\npid\tsource comm\ttotal times\t" \
                    "max duration\tmin duration\n");
            snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                    "%d\t%-16s%d\t\t", \
                    pjc_node->pid, \
                    pjc_node->process_comm, \
                    pjc_node->pjstat.count);
            if (pjc_node->pjstat.count > 0)
            {
                snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                        "%lu\t\t%lu\n", \
                        pjc_node->pjstat.maxduration, \
                        pjc_node->pjstat.minduration);

                snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                        "    Count of durations by range: (ms)\n\t" \
                        "duration:\ttimes:\taverage:  wakeup_times:\n");
                if (pjc_node->pjstat.durationcount[0])
                {
                    snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                             "\t%-4d - %-4d\t%d\t%d\t  %d\n", 0, 1<<2, \
                             pjc_node->pjstat.durationcount[0], \
                             pjc_node->pjstat.averageduration[0], \
                             pjc_node->pjstat.wakeuptimes[0]);
                }
                for (i = 1; i < MAX_BUCKETS-1; i++)
                {
                    if (pjc_node->pjstat.durationcount[i])
                    {
                        snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                                "\t%-4d - %-4d\t%d\t%d\t  %d\n", 1<<(i+1), 1<<(i+2), \
                                pjc_node->pjstat.durationcount[i], \
                                pjc_node->pjstat.averageduration[i], \
                                pjc_node->pjstat.wakeuptimes[i]);
                    }
                }
                if (pjc_node->pjstat.durationcount[i])
                {
                    snprintf(buf+strlen(buf), buflen-strlen(buf)-1, \
                             "\t      > %-4d\t%d\t%d\t  %d\n", 1<<(i+1),  \
                             pjc_node->pjstat.durationcount[i], \
                             pjc_node->pjstat.averageduration[i], \
                             pjc_node->pjstat.wakeuptimes[i]);
                }
            }
            else
            {
                snprintf(buf+strlen(buf), buflen-strlen(buf)-1, "#\t\t#\n");
            }

            if (buflen <= strlen(buf))
            {
                flush_pjlist();
                MPM_DPRINTK("Warning: buffer is full \n");
                return;
            }
            pjc_node = pjc_node->next;
        }
    }
    
    flush_pjlist();
#endif
}

#endif

/*
 * This function returns the most recent desense request received by
 * the MPM along with the result.
 */
void mpm_get_last_desense_request(mpm_desense_info_t *desense, int *result)
{
	*desense = last_desense;
	*result = last_desense_result;
}

#ifdef CONFIG_MOT_FEAT_PM_STATS

/*
 * This function initializes the given mpm_stats_t structure.
 * Locking of the structure is the responsibility of the caller.
 * This code must hold these locks:
 *     mpmsplk
 *     mpm_driver_info_lock
 */
static void initialize_mpm_stats(mpm_stats_t *sp)
{
    int i;
    unsigned long save_busy_arr[MAX_IDLE_DELAYS_SAVED];

    memcpy (save_busy_arr, sp->driver_stats.busy_arr,
            sizeof(save_busy_arr));
    memset (sp, 0, sizeof(mpm_stats_t));
    sp->stat_start_time = rtc_sw_msfromboot();
    sp->stats_enabled = 1;
    for (i=0; i<MAX_PMMODES; i++)
	sp->pmmode[i].minduration = -1;
    memcpy (sp->driver_stats.busy_arr, save_busy_arr,
            sizeof(save_busy_arr));
    memset (sp->driver_stats.busy_arr[0], 0,
            sizeof_busy_arr_bitarrays(mpmdip->size_in_bits));
}

void mpm_lpm_stat_ctl(int enable)
{
    mpm_stats_t *sp;
    char *cp;
    unsigned long flags;
    unsigned long driver_flags;
    int newsize;
    int newsize_in_bytes;
    unsigned long *new_busy_arr_p;
    int i;

    if (enable && !LPM_STATS_ENABLED()) {
	/*
	 * Use a local pointer for the structure until it is
	 * completely initialized.  Then move it to mpmsp.
	 */
	sp = kmalloc(sizeof(mpm_stats_t), GFP_KERNEL);
	if (sp == NULL)
	    return;

	newsize = mpmdip->size_in_bits;
	newsize_in_bytes = mpm_bitarray_size_in_bytes(newsize);
        new_busy_arr_p = kmalloc(sizeof_busy_arr_bitarrays(newsize), GFP_KERNEL);
	if (new_busy_arr_p == NULL) {
	    kfree(sp);
	    return;
	}

	spin_lock_irqsave(&mpmsplk, flags);
        if (LPM_STATS_ENABLED()) {
            /*
             * We lost the race and someone else started
             * statistics gathering.  Just hand everything
             * back and return.
             */
            spin_unlock_irqrestore(&mpmsplk, flags);
            kfree(new_busy_arr_p);
            kfree(sp);
            return;
        }

        spin_lock_irqsave(&mpm_driver_info_lock, driver_flags);
        if (mpmdip->size_in_bits != newsize)
	{
            spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
            spin_unlock_irqrestore(&mpmsplk, flags);
            kfree(new_busy_arr_p);
            kfree(sp);
	    return;
	}

        /* Fill in the busy_arr array with the appropriate pointers. */
        cp = (char *)new_busy_arr_p;
        for (i=0; i<MAX_IDLE_DELAYS_SAVED; i++)
        {
            sp->driver_stats.busy_arr[i] = (unsigned long *)cp;
            cp += newsize_in_bytes;
        }   

	initialize_mpm_stats(sp);

        mpmsp = sp;

        spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
        spin_unlock_irqrestore(&mpmsplk, flags);
    }
    else if (!enable && LPM_STATS_ENABLED()) {
	spin_lock_irqsave(&mpmsplk, flags);
	if (!LPM_STATS_ENABLED()) {
	    spin_unlock_irqrestore(&mpmsplk, flags);
	    return;
	}
        spin_lock_irqsave(&mpm_driver_info_lock, driver_flags);
	sp = mpmsp;
	mpmsp = NULL;
        spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
	spin_unlock_irqrestore(&mpmsplk, flags);
	kfree(sp->driver_stats.busy_arr[0]);
	kfree(sp);
    }
}

/*
 * Reset all of the PM statistics.
 *
 * All of the statistics are contained in the mpm_stats structure,
 * so it's easy enough to reset everything.
 */
void mpm_reset_lpm_stats(void)
{
    unsigned long flags;
    unsigned long driver_flags;

    spin_lock_irqsave(&mpmsplk, flags);
    spin_lock_irqsave(&mpm_driver_info_lock, driver_flags);

    if (LPM_STATS_ENABLED()) {
        initialize_mpm_stats(mpmsp);
    }

    spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
    spin_unlock_irqrestore(&mpmsplk, flags);
}
#endif

int mpm_getnum_op(void)
{
    return num_op;
}

const mpm_op_t* const mpm_getall_op(void)
{
    return valid_op;
}

mpm_op_t mpm_getcur_op(void)
{
    mpm_op_t cur_op;
    int i;

    cur_op = mxc_get_clocks(CPU_CLK);

    /* check if we got a valid freq */
    for ( i = 0; i < num_op ; i++ )
    {
        /* Queried frequency value will not exactly
           match the frequency requested to be set.
           Consider queried freq to be valid if it falls
           within a permissible range of allowable ops.
         */
        if ( cur_op < (valid_op[i] + FREQ_MARGIN) &&
                cur_op > (valid_op[i] - FREQ_MARGIN))
        {
            return valid_op[i];
        }
    }
        MPM_DPRINTK("Error: Got invalid value for Currently set freq %lu\n",
                cur_op);
    return 0;
}

int
mpm_set_op (mpm_op_t arg)
{
    unsigned long cur_cpu_freq;
    int retval = 0;
#ifdef CONFIG_MOT_FEAT_PM_STATS
    struct timeval stv, etv;
#endif

    /* Get the currently set operating point */
    if ((cur_cpu_freq = mpm_getcur_op()) == 0)
    {
        /* error */
        MPM_DPRINTK (" Got a value of 0 for currently set freq \n");
        retval = -1;
        goto out;
    }
    else if (cur_cpu_freq != arg)
    {
#ifdef CONFIG_MOT_FEAT_PM_STATS
        do_gettimeofday (&etv);
#endif

#ifdef CONFIG_MOT_FEAT_PM_BUTE
        printk("DVFS is not supported\n");
        retval = -1;
#else
        retval = mxc_pm_dvfs (arg, MPM_AHB_FREQ, MPM_IP_FREQ);
#endif

        if (retval == 0)
        {
#ifdef CONFIG_MOT_FEAT_PM_STATS
            do_gettimeofday (&stv);
#endif
            loops_per_jiffy = mpm_compute_lpj(arg/1000);

            MPM_DPRINTK("New Freq = %lu \n", arg);
            MPM_DPRINTK("New LPJ = %lu \n", loops_per_jiffy);

#ifndef CONFIG_MOT_DSM_INDICATOR
#ifdef CONFIG_MOT_TURBO_INDICATOR
            if (arg >= 532000000)
                lights_enter_turbo_mode();

            if (cur_cpu_freq == 532000000)
                lights_exit_turbo_mode();
#endif
#endif

#ifdef CONFIG_MOT_FEAT_PM_STATS
            if (opstat)
            {
                collect_op_stat (arg, &stv, &etv);
            }
#endif
        }
        else
        {
            MPM_DPRINTK ("requested freq could not be changed - error = %d\n",\
                         retval);
            goto out;
        }
    }
    /* otherwise requested freq is already set */

out:
    return retval;
}

/* This routine has been picked up as is from the DPM source code */
static unsigned long
mpm_compute_lpj( unsigned long new_freq)
{
    unsigned long new_jiffy_l, new_jiffy_h;

    /*
     * Recalculate loops_per_jiffy.  We do it this way to
     * avoid math overflow on 32-bit machines.  Maybe we
     * should make this architecture dependent?  If you have
     * a better way of doing this, please replace!
     *
     *    newlpj = orig_lpj * newfreq / orig_freq
     */
    new_jiffy_h = orig_lpj/orig_freq;
    new_jiffy_l = (orig_lpj % orig_freq) / 100;
    new_jiffy_h *= new_freq;
    new_jiffy_l = new_jiffy_l * new_freq/orig_freq;

    return new_jiffy_h + new_jiffy_l * 100;
}



EXPORT_SYMBOL(mpm_getcur_op);
EXPORT_SYMBOL(mpm_set_op);
EXPORT_SYMBOL(mpm_getnum_op);
EXPORT_SYMBOL(mpm_getall_op);
EXPORT_SYMBOL(mpm_start_opstat);
EXPORT_SYMBOL(mpm_stop_opstat);
EXPORT_SYMBOL(mpm_print_opstat);
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS)
EXPORT_SYMBOL(mpm_start_pjstat);
EXPORT_SYMBOL(mpm_stop_pjstat);
EXPORT_SYMBOL(mpm_print_pjstat);
#endif
#ifdef CONFIG_MOT_FEAT_PM_STATS
EXPORT_SYMBOL(mpm_lpm_stat_ctl);
EXPORT_SYMBOL(mpm_reset_lpm_stats);
#endif

module_init (mpm_init);
module_exit (mpm_exit);

MODULE_AUTHOR ("Motorola");
MODULE_DESCRIPTION ("Motorola Power Management driver");
MODULE_LICENSE ("GPL");
