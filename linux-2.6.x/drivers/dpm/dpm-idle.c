/*
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
 * Copyright (C) 2002, MontaVista Software <source@mvista.com>.
 *
 * Based on ibm405lp_dpm.c by Bishop Brock, Copyright (C) 2002,
 * International Business Machines Corporation.
 */

#include <linux/config.h>
#include <linux/dpm.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/string.h>

#include <asm/delay.h>
#include <asm/hardirq.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/uaccess.h>

/****************************************************************************
 *  DPM Idle Handler
 ****************************************************************************/

/*
   The idle handler is one of the most important parts of DPM, as very
   significant amounts of energy are saved by moving to a low-power idle state
   whenever possible.  The basic coding of the core of this routine is simply:

   dpm_set_os(DPM_IDLE_STATE);
   machine-dependent-idle-routine();
   dpm_set_os(DPM_IDLE_TASK_STATE);

   The added complexity found here is introduced to avoid unnecessary work, and
   especially to reduce the latencies associated with going in and out of idle.
   Idle power can be greatly reduced by moving to a very low-frequency
   operating point, but we also need to be aware of the impact on interrupt
   latencies.  The DPM implementation of idle attempts to balance these
   competing needs.

   We support 2 "idle" states: DPM_IDLE_TASK_STATE and DPM_IDLE_STATE.  The
   idle thread is marked as a "no-state" task, so that operating point changes
   are not automatically made when the idle thread is scheduled. The
   "idle-task" state is used for the majority of the idle thread.  Interrupts
   that occur during idle are handled in this state as well. The "idle" state
   is only entered from the idle-task state, and only for the express purpose
   of allowing an ultra-low-power operating point.

   The introduction of the idle-task state supports a stepped voltage and
   frequency scaling at idle.  On the IBM 405LP we would not want to go from,
   e.g., 266/133 @ 1.8 V directly to 8/8 @ 1.0 V and back.  Why not?  Because
   we would get "stuck" at 8MHz even though we need to wake up and resume
   useful work, e.g., we would have to set the 266/133 operating point while
   running at 8/8.  So instead when going idle first step down to idle-task,
   e.g., 100/50 @ 1.0 V, and then step down to e.g. 8/8 to halt.  The interrupt
   that takes us out of idle takes us back to idle-task (100/50) for interrupt
   processing and the potential return to 266/133.

   The best policies for this implementation will be able to transition between
   idle-task and idle without voltage scaling or driver notification. In these
   cases the transitions are handled with minimal latency by simple frequency
   scaling. */

static inline void
quick_idle(void)
{
	dpm_quick_enter_state(DPM_IDLE_STATE);
	dpm_md_idle();
	dpm_quick_enter_state(DPM_IDLE_TASK_STATE);
}

static void
full_idle(struct dpm_opt *idle_task_opt, struct dpm_opt *idle_opt)
{
	dpm_quick_enter_state(DPM_IDLE_STATE);
#ifdef CONFIG_DPM_STATS
	dpm_update_stats(&idle_opt->stats, &idle_task_opt->stats);
#endif
	dpm_set_opt(idle_opt, DPM_SYNC);
	dpm_md_idle();
	dpm_set_opt(idle_task_opt, DPM_SYNC);
	dpm_quick_enter_state(DPM_IDLE_TASK_STATE);
#ifdef CONFIG_DPM_STATS
	dpm_update_stats(&idle_task_opt->stats, &idle_opt->stats);
#endif
}


/* If DPM is currently disabled here we simply do the standard
   idle wait.

   If we're not actually in DPM_IDLE_TASK_STATE, we need to go back and get
   into this state.  This could happen in rare instances - an interrupt between
   dpm_set_os() and the critical section.

   If we are not yet at the idle-task operating point, or if there is no
   difference between idle-task and idle, we can enter/exit the idle state
   quickly since it's only for statistical purposes.  This is also true if for
   some reason we can't get the DPM lock, since obviously an asynchronous event
   is going to have to occur to clear the lock, and this event is going to take
   us out of idle.

   Otherwise the full idle shutdown is done. */


void
dpm_idle(void)
{
	unsigned long flags;
	struct dpm_opt *idle_task_opt, *idle_opt;

	current->dpm_state = DPM_NO_STATE;
	dpm_set_os(DPM_IDLE_TASK_STATE);
	local_irq_save(flags);

	if (! need_resched()) {
		if (!dpm_enabled) {
			dpm_md_idle();

		} else if (dpm_active_state != DPM_IDLE_TASK_STATE) {


		} else {
			idle_task_opt = dpm_choose_opt(dpm_active_policy,
						       DPM_IDLE_TASK_STATE);
			idle_opt = dpm_choose_opt(dpm_active_policy,
						  DPM_IDLE_STATE);

			if (dpm_trylock()) {
				dpm_md_idle();
			} else {

				if ((dpm_active_opt != idle_task_opt) ||
				    (idle_task_opt == idle_opt)) {

					quick_idle();
					dpm_unlock();
				} else {
					dpm_unlock();
					full_idle(idle_task_opt, idle_opt);
				}
			}
		}
	}
	local_irq_restore(flags);
}

