/*
 * drivers/dpm/policy.c  Dynamic Power Management Policies
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Copyright (C) 2002, International Business Machines Corporation
 * All Rights Reserved
 *
 * Robert Paulsen
 * IBM Linux Technology Center
 * rpaulsen@us.ibm.com
 * August, 2002
 *
 */

/* TODO:

   Rethink init/enable/disable: It may be redundant and/or unsafe
   Fix initialization and stats
*/

#include <linux/dpm.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/preempt.h>
#include <linux/ltt-events.h>

#include <asm/semaphore.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#undef TRACE
#if defined(TRACE)
#define trace(args...) do { printk("TRACE: "); printk(args); } while(0)
#else
#define trace(args...) do {} while(0)
#endif

struct dpm_md dpm_md;

static struct dpm_opt nop_op = {
	.name  = "[nop]",
	.flags = DPM_OP_NOP,
};

extern void dpm_force_off_constrainers(struct dpm_opt *opt);

unsigned long dpm_compute_lpj(unsigned long ref, u_int div, u_int mult)
{
	unsigned long new_jiffy_l, new_jiffy_h;

	/*
	 * Recalculate loops_per_jiffy.  We do it this way to
	 * avoid math overflow on 32-bit machines.  Maybe we
	 * should make this architecture dependent?  If you have
	 * a better way of doing this, please replace!
	 *
	 *    new = old * mult / div
	 */
	new_jiffy_h = ref / div;
	new_jiffy_l = (ref % div) / 100;
	new_jiffy_h *= mult;
	new_jiffy_l = new_jiffy_l * mult / div;

	return new_jiffy_h + new_jiffy_l * 100;
}

/****************************************************************************

DPM Synchronization and Operating Point Changes
===============================================

There are 2 aspects to synchronization in DPM: First, the usual requirement of
serializing access to shared data structures, and second, the idea of
synchronizing the operating point and the current operating state.  The second
condition arises because setting an operating point may complete asynchronously
for a number of reasons, whereas the operating state change that causes the
operating point change succeeds immediately.

Access to most of the global variables representing the current state of DPM
and the current policy are protected by a spinlock, dpm_policy_lock.  The use
of this lock appears in only a few critical places.

Setting the operating point, reading the value of the current operating point
or changing the current policy may only be done while holding the semaphore
_dpm_lock.  Access to the _dpm_lock is abstracted by the dpm_lock() and
dpm_unlock() calls as explained below.  (The semaphore should only be accessed
this way to simplify future development).

The _dpm_lock must be held (by a call to a dpm_lock function) by any caller of
the interfaces that set the operating point, change the policy, or enable or
disable DPM.  Note that the corresponding call to dpm_unlock() may be
explicitly required, or implicit (see dpm_set_opt_async() below).

For simplicity, the calls that create operating points and policies also use
dpm_lock() and dpm_unlock() to protect access to the non-active policies as
well. Since these are normally initialization calls, this should not interfere
with the operation of the system once initialized.

Three interfaces are provided for obtaining the _dpm_lock:

void dpm_lock();
int dpm_lock_interruptible();
int dpm_trylock();

dpm_lock_interruptible() returns -ERESTARTSYS if the wait for the _dpm_lock was
interrupted, and dpm_trylock() returns -EBUSY if the semaphore is currently
held.

Once the _dpm_lock is held, two interfaces are provided for setting the
operating point:

int dpm_set_opt_async()
int dpm_set_opt_sync();

Neither of these interfaces takes parameters since under DPM the operating
point to select is always implied by the current policy and operating state.
If the system is already at the correct operating point then no change is
required or made.  To avoid deadlock, the caller must not be holding the
dpm_policy_lock when either of these calls is made.

dpm_set_opt_async() launches a change in the operating point that will
potentially terminate asynchronously.  This interface never blocks the caller,
thus there is no guarantee that the system is actually running at the implied
operating point when control returns to the caller. This call is used by
dpm_set_os() during an operating state change.  Note since this call terminates
asynchronously, the call to dpm_unlock() is implicitly made when the operating
point change is complete.  I.e., the caller obtains the _dpm_lock with
dpm_lock(), calls dpm_set_opt_async(), then continues.

dpm_set_opt_sync() launches a synchronous change in the operating point.  This
call will block the caller as necessary during the call, thus it can only be
issued from a process context.  When control returns to the caller, the caller
can be sure that the implied operating point was set, and that the system is
currently running at the correct operating point for the given policy and
operating state.  This call is used by dpm_set_policy() and the device
constraint update code to guarantee that the change to a new policy, or changes
to operating point classes as a result of device constraits are reflected in
the operating point.

Note that regardless of whether an operating point change is synchrounous or
asynchronous, it is still possible that the operating state may change during
the call.  Setting the operating point is (currently) not preemptible,
therefore at the time that the operating point change is complete, it may no
longer be the correct operating point for the operating state.  This condition
is always handled by the dpm_set_opt*() routines, which will launch a tasklet
to re-synchronize the operating point to the operating state.

It is possible that due to poorly designed policies and asynchronous
termination of operating point changes that the operating point will always lag
behind the operating state.  This is only a performance issue, not a
correctness issue.  Since a valid policy has a valid operating point for every
operating state, and changes to the policy and changes in devices constraints
always use dpm_set_opt_sync(), there will never be a case where the current
operating point does not support device constraints.

****************************************************************************/

/* curently installed policies and operating points */
LIST_HEAD(dpm_policies);
LIST_HEAD(dpm_classes);
LIST_HEAD(dpm_opts);

DECLARE_MUTEX(_dpm_lock);
spinlock_t dpm_policy_lock = SPIN_LOCK_UNLOCKED;

/* the currently active policy */
struct dpm_policy *dpm_active_policy;

/* the currently active operating state, class, and operating point */
dpm_state_t dpm_active_state = DPM_NO_STATE;
struct dpm_opt *dpm_active_opt;
struct dpm_class *dpm_active_class;

/* is DPM initialized and enabled? */
int dpm_enabled;
int dpm_initialized;

#ifdef CONFIG_DPM_STATS
#include <asm/div64.h>

struct dpm_stats dpm_state_stats[DPM_STATES];

/*
 * Start counting DPM stats from the time DPM was enabled... in the case of
 * operating states the stats are updated from the time userspace is started.
 */

void
dpm_stats_reset(void)
{
	int i;

	preempt_disable();
	for (i = 0; i < DPM_STATES; i++) {
		dpm_state_stats[i].total_time = 0;
		dpm_state_stats[i].start_time = 0;
		dpm_state_stats[i].count = 0;
	}

	if (dpm_active_state != DPM_NO_STATE) {
		dpm_state_stats[dpm_active_state].start_time = dpm_time();
		dpm_state_stats[dpm_active_state].count = 1;
	}

	preempt_enable();
}


unsigned long long
dpm_update_stats(struct dpm_stats *new, struct dpm_stats *old)
{
	unsigned long long now = dpm_time();

	if (old)
		old->total_time += now - old->start_time;

	if (new) {
		new->start_time = now;
		new->count += 1;
	}

	return now;
}
#else
#define dpm_update_stats(a,b) do {} while (0)
#define dpm_stats_reset() do {} while (0)
#endif /* CONFIG_DPM_STATS */

struct dpm_opt *
dpm_choose_opt(struct dpm_policy *policy, int state)
{
	struct dpm_opt *opt = NULL;

	if (policy->classopt[state].opt) {
		opt = policy->classopt[state].opt;

		if (opt->flags & DPM_OP_FORCE)
			dpm_force_off_constrainers(opt);
		else if (! dpm_check_constraints(opt))
			opt = NULL;

		dpm_active_class = NULL;
	}
	else {
		int i;

		for (i = 0; i < policy->classopt[state].class->nops; i++) {
			if (dpm_check_constraints(
				    policy->classopt[state].class->ops[i])) {
				opt = policy->classopt[state].class->ops[i];
				break;
			}
		}

		dpm_active_class = policy->classopt[state].class;
	}

	return opt;
}



/*****************************************************************************
 * dpm_next_opt() returns the operating point that needs to be activated next,
 * or NULL if the operating point is up-to-date or the DPM system is disabled.
 * Since this call looks at the value of the current operating point, it can
 * only be made when the _dpm_lock is held.
 *****************************************************************************/

static inline struct dpm_opt *
dpm_next_opt(void)
{
	struct dpm_opt *opt = NULL;

	if (! spin_trylock(&dpm_policy_lock))
		return NULL;
	if (dpm_enabled && dpm_active_state != DPM_NO_STATE) {
		opt = dpm_choose_opt(dpm_active_policy,dpm_active_state);
		if (opt == dpm_active_opt)
			opt = NULL;
	}
	spin_unlock(&dpm_policy_lock);
	return opt;
}

/*****************************************************************************
 * Set the operating point implied by the current DPM policy. These calls can
 * only be made while holding _dpm_lock, and the release of
 * _dpm_lock is implied by the call (see below).
 *****************************************************************************/

static struct dpm_opt temp_opt = { name : "[System Operating Point]" };

int
dpm_set_opt(struct dpm_opt *new, unsigned flags)
{
	int error;

	if (new->flags & DPM_OP_NOP) {
		if (flags & DPM_UNLOCK)
			dpm_unlock();
		return 0;
	}

	/* Support for setting the operating point when DPM is not running, and
	   setting the first operating point. */

	if (!dpm_enabled || !dpm_active_opt) {
		if (dpm_md_get_opt(&temp_opt)) {
			printk(KERN_ERR "dpm_default_set_opt: "
			      "DPM disabled and system "
			      "operating point is illegal!\n");

			if (flags & DPM_UNLOCK)
				dpm_unlock();
			return -EINVAL;
		}
		dpm_active_opt = &temp_opt;
		dpm_active_class = NULL;
	}

	/*
	 * Remove the IRQ disable since in some cases scheduling is needed
	 * to set an operating point (only sleep mode).  The spinlock
	 * should suffice.  If the machine-dependent code needs interrupts
	 * turned off during the code used for that platform for that
	 * operating point set sequence then IRQs will need to be disabled
	 * in that code instead.
	 */
	error = dpm_md.set_opt(dpm_active_opt, new);

	if (error == 0) {
		dpm_update_stats(&new->stats, &dpm_active_opt->stats);
		dpm_active_opt = new;
		mb();
	}

	ltt_ev_dpm(LTT_EV_DPM_OP, strlen(new->name) + 1, new->name);

	if (flags & DPM_UNLOCK)
		dpm_unlock();

	return error;
}

/*****************************************************************************
 * Set operating point asynchronously.  The _dpm_lock will be cleared whenever
 * the change in operating point is complete.
 *****************************************************************************/

int
dpm_set_opt_async(void)
{
	struct dpm_opt *opt = dpm_next_opt();

	if (opt) {
		dpm_trace(DPM_TRACE_SET_OPT_ASYNC, opt);
		return dpm_set_opt(opt, DPM_UNLOCK);
	} else {
		dpm_trace(DPM_TRACE_SET_OPT_ASYNC, NULL);
		dpm_unlock();
		return 0;
	}
}

/*****************************************************************************
 * Set operating point synchronously.  The caller must clear _dpm_lock after the
 * call returns.
 *****************************************************************************/

int
dpm_set_opt_sync(void)
{
	struct dpm_opt *opt = dpm_next_opt();

	if (opt) {
		dpm_trace(DPM_TRACE_SET_OPT_SYNC, opt);
		return dpm_set_opt(opt, DPM_SYNC);
	} else
		dpm_trace(DPM_TRACE_SET_OPT_SYNC, NULL);
	return 0;
}

/*****************************************************************************
 * Resynchronize the operating state and the operating point without
 * blocking. If we don't get the lock it doesn't matter, since whenever the
 * lock holder releases the lock the resynchronization will be tried again.
 *****************************************************************************/

static inline void
dpm_resync(void)
{

	dpm_trace(DPM_TRACE_RESYNC);
	if (! dpm_trylock())
		dpm_set_opt_async();
}

void
dpm_resync_task(unsigned long ignore)
{
	dpm_resync();
}

/*****************************************************************************
 * unlock the DPM
 *
 * If the operating point and operating state are not in sync when _dpm_lock is
 * released, a tasklet is launched to resynchronize them. A tasklet is used
 * rather than simply calling dpm_set_op directly to avoid deep recursions.
 * (I'm not sure this has worked, though).
 *
 * (The locking functions are inline in dpm_policy.h)
 *
 * This is not static since it needs to be called from dpm_policy.c
 *****************************************************************************/

DECLARE_TASKLET(dpm_resync_tasklet, dpm_resync_task, 0);

void
dpm_unlock(void)
{
	int retry;

	retry = dpm_next_opt() != NULL;
	dpm_trace(DPM_TRACE_UNLOCK, retry);
	up(&_dpm_lock);
	if (retry)
		tasklet_schedule(&dpm_resync_tasklet);
}

/*****************************************************************************
 * Enter a new operating state for statistical purposes.  Returns 1 if the new
 * state may require a change in operating point and 0 otherwise.
 *
 * The normal case that occurs during task scheduling, where we go from task
 * state to task state, is quickly ignored, as are changes to the
 * DPM_NO_STATE and changes when DPM is not running.  Otherwise,
 * dpm_enter_state() has advertised that we are in a new state, and indicates
 * whether an operating point change is required.
 *
 * Note the system invariant that the operating point always eventually
 * catches up with changes to the operating state.  This is what makes it
 * correct here to check for common operating points.  We know
 * that if a common operating point is not the current operating point, it
 * will be soon.
 *
 * The 'quick' variant (in dpm.h) is called out separately to reduce latency
 * for critical operating state changes where the following are known: 1) The
 * dpm_policy_lock is held and/or interrupts are properly disabled.  2) DPM is
 * enabled.  3) The new state is neither DPM_NO_STATE nor the same as the
 * active state.  4) Any operating point change is being handled elsewhere.
 *****************************************************************************/

static int
dpm_enter_state(int new_state)
{
	int ret = 0;

	if (! spin_trylock(&dpm_policy_lock)) {
		dpm_quick_enter_state(new_state);
		return 0;
	}

        if ((new_state == dpm_active_state) ||
            (new_state == DPM_NO_STATE) ||
            !dpm_enabled) {
		spin_unlock(&dpm_policy_lock);
		return ret;
        }

        if ((dpm_active_policy->classopt[new_state].class !=
             dpm_active_policy->classopt[dpm_active_state].class) ||
            (dpm_active_policy->classopt[new_state].opt !=
             dpm_active_policy->classopt[dpm_active_state].opt))
                ret = 1;

	dpm_quick_enter_state(new_state);
        spin_unlock(&dpm_policy_lock);
        return ret;
}


/*****************************************************************************
 * set operating state
 *
 * This is used by the kernel to inform the DPM that the operating state has
 * changed and that a new operating point should (possibly) be set as a
 * result.
 *
 * If an operating point change is required it is attempted. If we can't get
 * the lock here, then the operating point change will be activated when the
 * current lock holder releases the lock.
 *****************************************************************************/

void
dpm_set_os(dpm_state_t new_state)
{
	dpm_trace(DPM_TRACE_SET_OS, new_state);
	ltt_ev_dpm(LTT_EV_DPM_OS, new_state, NULL);
	if (dpm_enter_state(new_state))
		dpm_resync();
}

EXPORT_SYMBOL(dpm_set_os);

/*****************************************************************************
 * initialize the DPM
 *****************************************************************************/
int
dynamicpower_init(void)
{
	trace("in dynamicpower_init\n");

	if (dpm_initialized) {
		trace("DPM already initialized");
		return -EALREADY;
	}

	/* mutex-style semaphore for access to policies and opts */
	init_MUTEX(&_dpm_lock);

	dpm_active_policy = 0;	/* this leaves the DPM temporarily
				   disabled until a policy is
				   activated */
	dpm_enabled = 0;
	dpm_initialized = 1;
	dpm_active_state = DPM_TASK_STATE;


	trace("DPM is now initialized\n");

	return 0;
}

/*****************************************************************************
 * (temporarily) disable the DPM
 *****************************************************************************/
int
dynamicpower_disable(void)
{

	trace("in dynamicpower_disable\n");

	if (! dpm_enabled) {
		trace("DPM already disabled");
		return -EALREADY;
	}

	dpm_lock();

	dpm_enabled = 0;
	dpm_md_cleanup();
	dpm_active_opt = NULL;
	dpm_active_class = NULL;

	dpm_unlock();

	trace("DPM is now disabled\n");

	return 0;
}

/*****************************************************************************
 * re-enable the DPM
 * dpm_enabled = 1 implies that DPM is initialized and there is an active
 * policy. The 'enable' call is really designed to be used after a temporary
 * 'disable'.  All that's required to start DPM is to initialize it and set a
 * policy.
 *****************************************************************************/

/* Need to think through enable/disable */

int
dynamicpower_enable(void)
{

	trace("in dynamicpower_enable\n");

	if (dpm_enabled) {
		trace("DPM already enabled");
		return -EALREADY;
	}

	dpm_lock();

	if (dpm_active_policy) {
		dpm_enabled = 1;
		mb();
		dpm_md_startup();
		dpm_stats_reset();
		dpm_set_opt_sync();
		trace("DPM is now enabled\n");
	} else {
		trace("No active policy, dpm_enable is ignored\n");
	}

	dpm_unlock();
	return 0;
}

/*****************************************************************************
 * Suspend/Resume DPM
 * The current operating point is saved and restored. This
 * interface is designed to be used by system suspend/resume code, to safely
 * save/restore the DPM operating point across a system power-down, where the
 * firmware may resume the system at a random operating point.  This does not
 * require DPM to be enabled. Note that DPM remains locked across the
 * suspend/resume.
 *****************************************************************************/

static struct dpm_opt suspend_opt = { name : "[Suspended Op. Point]" };
struct dpm_opt *suspended_opt;

int
dynamicpm_suspend(void)
{
	int err;

	trace("in dpm_suspend\n");

	dpm_lock();

	if (dpm_enabled && dpm_active_opt) {
		suspended_opt = dpm_active_opt;
	} else {
		suspended_opt = &suspend_opt;
		if ((err = dpm_md_get_opt(suspended_opt))) {
			printk(KERN_CRIT
			       "DPM can not suspend the current op. point!\n");
			suspended_opt = NULL;
			return err;
		}
	}
	return 0;
}

void
dynamicpm_resume(void)
{
	trace("in dpm_resume\n");

	if (suspended_opt) {
		dpm_active_opt = NULL;	/* Force reinitialization of DPM */
		dpm_active_class = NULL;
		dpm_set_opt(suspended_opt, DPM_SYNC);
		suspended_opt = NULL;
	}
	dpm_unlock();
}


/*****************************************************************************
 * Create a named operating point
 * The alternate entry point can be used to create anonymous operating points
 *****************************************************************************/

int
_dpm_create_opt(struct dpm_opt **p, const char *name,
		const dpm_md_pp_t * md_pp, int npp)
{
	struct dpm_opt *opt;
	int ret;

	/* get memory for opt */
	if (!
	    (opt =
	     (struct dpm_opt *) kmalloc(sizeof (struct dpm_opt), GFP_KERNEL))) {
		return -ENOMEM;
	}
	trace("%s @ 0x%08lx\n", name, (unsigned long)opt);
	memset(opt, 0, sizeof(struct dpm_opt));
	if (!(opt->name = (char *) kmalloc(strlen(name) + 1, GFP_KERNEL))) {
		kfree(opt);
		return -ENOMEM;
	}

	/* initialize and validate the opt */
	strcpy(opt->name, name);
	memcpy(&opt->pp, md_pp, npp * sizeof(dpm_md_pp_t));
	ret = dpm_md_init_opt(opt);
	if (ret) {
		kfree(opt->name);
		kfree(opt);
		return ret;
	}
	INIT_LIST_HEAD(&opt->list);
	*p = opt;
	dpm_sysfs_new_op(opt);
	return 0;
}

int
dpm_create_opt(const char *name, const dpm_md_pp_t * md_pp, int npp)
{
	int ret;
	struct dpm_opt *opt;

	trace("in dpm_create_opt for \"%s\"\n", name);

	dpm_lock();

	/* ensure name is unique */
	list_find(opt, name, dpm_opts, struct dpm_opt);
	if (opt) {
		dpm_unlock();
		return -EEXIST;
	}

	/* create the opt */
	ret = _dpm_create_opt(&opt, name, md_pp, npp);

	/* add opt to our list */
	if (!ret)
		list_add(&opt->list, &dpm_opts);

	dpm_unlock();
	return ret;
}

/*****************************************************************************
 * destroy an operating point
 * Assumes _dpm_lock is held and the opt is no longer needed *anywhere*
 *****************************************************************************/
void
destroy_opt(struct dpm_opt *opt)
{
	dpm_sysfs_destroy_op(opt);
	list_del(&opt->list);
	kfree(opt->name);
	kfree(opt);
}

/*****************************************************************************
 * create a named class of operating points (to be used to map to an operating
 * state)
 *****************************************************************************/

int
dpm_create_class(const char *name, char **op_names, unsigned nops)
{
	int i;
	struct dpm_class *cls;

	trace("in dpm_create_class for \"%s\"\n", name);

	dpm_lock();

	/* ensure class is not empty */
	if (nops == 0) {
		dpm_unlock();
		return -EINVAL;
	}

	/* ensure name is unique */
	list_find(cls, name, dpm_classes, struct dpm_class);
	if (cls) {
		dpm_unlock();
		return -EEXIST;
	}

	/* get memory for class */
	cls = (struct dpm_class *) kmalloc(sizeof (struct dpm_class), GFP_KERNEL);
	if (!cls) {
		dpm_unlock();
		return -ENOMEM;
	}
	trace("%s @ 0x%08lx\n", name, (unsigned long)cls);
	memset(cls, 0, sizeof (struct dpm_class));
	/* get memory for array of pointers to operating points */
	cls->ops =
	    (struct dpm_opt **) kmalloc(nops * sizeof (struct dpm_opt *),
					GFP_KERNEL);
	if (!cls->ops) {
		kfree(cls);
		dpm_unlock();
		return -ENOMEM;
	}

	/* get memory for class name */
	cls->name = (char *) kmalloc(strlen(name) + 1, GFP_KERNEL);
	if (!cls->name) {
		kfree(cls->ops);
		kfree(cls);
		dpm_unlock();
		return -ENOMEM;
	}

	/* find named op points and put their pointers in the class */
	for (i = 0; i < nops; ++i) {
		struct dpm_opt *opt;
		list_find(opt, op_names[i], dpm_opts, struct dpm_opt);
		if (!opt) {
			kfree(cls->name);
			kfree(cls->ops);
			kfree(cls);
			dpm_unlock();
			return -ENOENT;
		}
		cls->ops[i] = opt;
	}
	strcpy(cls->name, name);
	cls->nops = nops;
	/* add class to our list */
	list_add(&cls->list, &dpm_classes);

	dpm_unlock();
	dpm_sysfs_new_class(cls);

	return 0;
}

/*****************************************************************************
 * destroy a class
 * Assumes _dpm_lock is held and the class is no longer needed *anywhere*
 *****************************************************************************/
void
destroy_class(struct dpm_class *cls)
{
	dpm_sysfs_destroy_class(cls);
	list_del(&cls->list);
	kfree(cls->ops);
	kfree(cls->name);
	kfree(cls);
}

int
dpm_map_policy_state(struct dpm_policy *policy, int state, char *classopt)
{
	list_find(policy->classopt[state].opt, classopt, dpm_opts,
		  struct dpm_opt);

	if(!policy->classopt[state].opt) {
		list_find(policy->classopt[state].class, classopt,
			  dpm_classes, struct dpm_class);
		if(!policy->classopt[state].class)
			return -1;
	}

	return 0;
}

/*****************************************************************************
 * create power policy
 *****************************************************************************/
int
dpm_create_policy(const char *name, char **classopt_names, int nopts)
{
	int i;
	struct dpm_policy *policy;

	trace("in dpm_install_policy for \"%s\" policy\n", name);

	dpm_lock();

	/* ensure unique name */
	list_find(policy, name, dpm_policies, struct dpm_policy);
	if (policy) {
		dpm_unlock();
		return -EEXIST;
	}

	/* get memory for policy */
	policy =
	    (struct dpm_policy *) kmalloc(sizeof (struct dpm_policy),
					  GFP_KERNEL);
	if (!policy) {
		dpm_unlock();
		return -ENOMEM;
	}
	trace("%s @ 0x%08lx\n", name, (unsigned long)policy);
	memset(policy, 0, sizeof (struct dpm_policy));
	/* get memory for policy name */
	policy->name = (char *) kmalloc(strlen(name) + 1, GFP_KERNEL);
	if (!policy->name) {
		kfree(policy);
		dpm_unlock();
		return -ENOMEM;
	}

	/* initialize the policy */
	for (i = 0; i < DPM_STATES; ++i) {
		if ((i >= nopts) || !classopt_names[i]) {
			policy->classopt[i].opt	= &nop_op;
		} else {
			if (dpm_map_policy_state(policy, i, classopt_names[i])
			    < 0) {
				kfree(policy->name);
				kfree(policy);
				dpm_unlock();
				return -ENOENT;
			}
		}
	}
	strcpy(policy->name, name);

	/* add policy to our list */
	list_add(&policy->list, &dpm_policies);
	dpm_sysfs_new_policy(policy);
	trace("installed \"%s\" policy\n", name);
	dpm_unlock();
	return 0;
}

/*****************************************************************************
 * destroy a power policy
 * Assumes _dpm_lock is held and the policy is no longer needed *anywhere*
 *****************************************************************************/
void
destroy_policy(struct dpm_policy *policy)
{
	dpm_sysfs_destroy_policy(policy);
	list_del(&policy->list);
	kfree(policy->name);
	kfree(policy);
}

/*****************************************************************************
 * uninstall power policy
 *****************************************************************************/
int
dpm_destroy_policy(const char *name)
{
	struct dpm_policy *policy;

	trace("processing destroy request for \"%s\"\n", name);

	dpm_lock();

	/* find the named policy */
	list_find(policy, name, dpm_policies, struct dpm_policy);
	if (!policy) {
		dpm_unlock();
		return -ENOENT;
	}

	/* can't uninstall active policy */
	if (policy == dpm_active_policy) {
		dpm_unlock();
		return -EBUSY;
	}

	/* remove the policy */
	destroy_policy(policy);

	dpm_unlock();
	trace("destroyed \"%s\" policy\n", name);
	return 0;
}

/*
 * set active power policy
 */
int
dpm_set_policy(const char *name)
{
	struct dpm_policy *new_p;

	trace("in dpm_set_policy for \"%s\" policy\n", name);

	dpm_lock();

	list_find(new_p, name, dpm_policies, struct dpm_policy);
	if (!new_p) {
		dpm_trace(DPM_TRACE_SET_POLICY, name, -ENOENT);
		dpm_unlock();
		return -ENOENT;	/* invalid name */
	}
	if (new_p == dpm_active_policy) {
		dpm_trace(DPM_TRACE_SET_POLICY, name, 0);
		trace("\"%s\" policy already activated\n", name);
		dpm_unlock();
		return 0;
	}

	dpm_update_stats(&new_p->stats,
			 dpm_active_policy ? &dpm_active_policy->stats
			 : NULL);

	dpm_active_policy = new_p;

	if (! dpm_enabled) {
		dpm_enabled = 1;
		dpm_md_startup();
		dpm_stats_reset();
	}

	/* Start the policy synchronously */

	mb();
	dpm_trace(DPM_TRACE_SET_POLICY, name, 0);
	dpm_set_opt_sync();
	dpm_unlock();

	return 0;
}

/*****************************************************************************
 * set a raw op state
 *****************************************************************************/

int
dpm_set_op_state(const char *name)
{
	int op_state;

	for (op_state = 0; op_state < DPM_STATES; op_state++)
		if (strcmp(dpm_state_names[op_state], name) == 0) {
			dpm_set_os(op_state);
			return 0;
		}

	return -ENOENT;
}

/*****************************************************************************
 * terminate the DPM
 *****************************************************************************/
int
dynamicpower_terminate(void)
{
	trace("in dynamicpower_terminate\n");

	if (!dpm_initialized)
		return 0;

	dpm_lock();

	dpm_md_cleanup();

	dpm_initialized = 0;
	dpm_enabled = 0;
	dpm_active_opt = NULL;
	dpm_active_class = NULL;

	/* destroy all entities */
	while (!list_empty(&dpm_policies))
		destroy_policy(list_entry
			       (dpm_policies.next, struct dpm_policy, list));
	while (!list_empty(&dpm_opts))
		destroy_opt(list_entry(dpm_opts.next, struct dpm_opt, list));
	while (!list_empty(&dpm_classes))
		destroy_class(list_entry(dpm_classes.next, struct dpm_class,
					 list));


	mb();
	dpm_unlock();

	trace("DPM is now terminated\n");
	printk("Dynamic Power Management is now terminated\n");

	return 0;
}

EXPORT_SYMBOL(dynamicpower_init);
EXPORT_SYMBOL(dynamicpower_terminate);
EXPORT_SYMBOL(dynamicpower_disable);
EXPORT_SYMBOL(dynamicpower_enable);
EXPORT_SYMBOL(dpm_create_opt);
EXPORT_SYMBOL(dpm_create_class);
EXPORT_SYMBOL(dpm_create_policy);
EXPORT_SYMBOL(dpm_destroy_policy);
EXPORT_SYMBOL(dpm_set_policy);

/****************************************************************************
 * install dynamic power policy support
 ****************************************************************************/
static int __init
dpm_init_module(void)
{
	int i;

	/* Set the NOP operating point params to all -1. */

	for (i = 0; i < DPM_PP_NBR; i++)
		nop_op.pp[i] = -1;

	trace("DPM is now installed\n");
	return 0;
}

/****************************************************************************
 * remove dynamic power policy support
 ****************************************************************************/
static void __exit
dpm_exit_module(void)
{
	/* disable power management policy system */
	dynamicpower_terminate();

	trace("DPM module is now unloaded\n");
}

module_init(dpm_init_module);
module_exit(dpm_exit_module);

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
