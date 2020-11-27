
#ifndef _LINUX_VST_H
#define _LINUX_VST_H
/*
 * linux/vst.h
 *
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
 * Main header file for VST (Variable Sleep Time, tick elimination)
 *
 * Author: George Anzinger george@mvista.com>
 *
 * Copyright (C) 2003 MontaVista, Software, Inc. 
 * Copyright 2003 Sony Corporation.
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 
 * This file is licensed under  the terms of the GNU General Public 
 * License version 2. This program is licensed "as is" without any 
 * warranty of any kind, whether express or implied.
 */
#include <linux/config.h>
#ifdef CONFIG_VST
#include <asm/vst.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <asm/atomic.h>
#ifndef vst_bump_jiffies_by
#define vst_bump_jiffies_by(x) (jiffies_64 += (x))
#endif

extern int do_vst_setup(void);
extern unsigned long VST_max_time;
extern int vst_successful_exit;
extern int vst_external_intr_exit;
extern int vst_skipped_interrupts;
extern cpumask_t vst_cpus;
extern raw_spinlock_t vst_int_disable_lock;
extern void run_local_timers(void);
#define vst_setup() do_vst_setup()


#define in_vst_sleep() cpu_isset(smp_processor_id(), vst_cpus)
#define VST_VISIT_COUNT int visit_count;
#define BUMP_VST_VISIT_COUNT base->visit_count++
#ifdef CONFIG_HIGH_RES_TIMERS
extern int hrtimer_is_pending(void);
#else
#define hrtimer_is_pending() 0
#endif
#else
#define VST_VISIT_COUNT 
#define BUMP_VST_VISIT_COUNT
#ifdef CONFIG_IDLE
/*
 * The IDLE code is entered via vst_setup() so, if needed define it here.
 */
extern int do_vst_setup(void);
#define vst_setup() do_vst_setup()
#endif
/*
 * The VST system determines how long the system can sleep by scanning
 * the timer list to find the next timer.  Once found the following
 * function is called to: 
 *
 * Set up a hardware timer to interrupt (and wake the system) at the
 * specified time.  The time is given as jiffies, NOT as a delta from
 * now.  The wake up should occure on or just before that jiffie "edge".
 *
 * It is possible that the specified jiffies will be too large for the
 * hardware timer (or possibly the math).  In this case the
 * vst_sleep_till() code should just use the largest number it can.
 *
 */
#define vst_sleep_till(jiffies) /* This is dummy code for the no vst case */
/* 
 * The above function must call the following function to indicate that
 * the given cpu is in the vst_sleep state.  (This function sets a bit
 * in the vst_cpus mask.)  This function also calls
 * vst_stop_jiffie_int() if all cpus are now in the vst_sleep state.
 */
#define vst_sleeping()
/*
 * The vst_wakeup() function must call the following function to
 * indicate that the given cpu is no longer sleeping.  This function
 * clears the given cpus bit in the vst_cpus mask and calls the
 * vst_start_jiffie_int() to restart the jiffies interrupts.
 */
#define vst_waking()

/* The following call is made by the vst code when it determines that
 * jiffies interrupts are to be stopped.  In a UP system, this will be
 * at the same time as the vst_sleep_till call, but in SMP systems, all
 * cpus must be in the sleep_till state before this call is made.  It is
 * assumed that the timer that vst_sleep_till() uses differs from the
 * jiffies timer.  If this is not the case it may be possible to ignore
 * this call (but a dummy must be defined).
 */
#define vst_stop_jiffie_int()
/*
 * The following function should restart jiffie interrupts.  It should
 * also adjust jiffies to be the correct value.
 */
#define vst_start_jiffie_int()
/*
 * The following calls are made only on smp systems.  They mark the
 * entry and exit of the cpu to the VST state.  It is intended that
 * these be used to do cpu specific things to shut down/ bring up things
 * like watchdog timers.  In the x86 platform this is, in fact, what
 * they do.  On UP systems these calls are not made.  These calls MUST
 * be defined as macros.
 */
#define vst_smp_cpu_enter()
#define vst_smp_cpu_exit()
/*
 * The following function will be called on each interrupt and is
 * charged with pulling the system out of the sleep state.  This
 * function should:
 *
 * A.) Verify that the given cpu is in vst_sleep (see function provided
 * to do this).
 *
 * B.) Call vst_waking() to indicate that the cpu is now awake.
 *
 * C.) Set up the timer interrupt for an interrupt on the next jiffie
 * (Note that the vst_waking() call will restart the jiffies interrupt.)
 * and adjust the jiffies time to reflect current time.  At this time
 * the code should know how many timer interrupts were skipped.  This
 * value should be added to vst_skipped_interrupts.
 *
 * D.) Determine if the call is for the end of the requested vst sleep
 * (i.e. due to the interrupt requested and, if so:
 * a) bump vst_successful_exit (it is defined as an extern in this file).
 * b) call run_local_timers() to request a scan of the timer list.
 * Otherwise, the wake up is do to an external interrupt so 
 * vst_external_intr_exit should be bumped.
 *
 * 
 * The system may, and often will, wake up long before the time
 * requested above.
 *
 * This code should, of course, do nothing (quickly) if the system is
 * not in a vst sleep.  Registers are provided to allow calls to do_timer()
 * to do the time catchup.
 *
 * There are two possibilities for the way an arch does the idle
 * instruction:
 *
 * A.) The instruction is entered with the interrupt system on and,
 * exits (i.e. moves to the next instruction) after an interrupt.  This
 * is how the x86 systems work.  In this case the instruction to turn on
 * the interrupt system MUST be just before the idle instruction so that
 * (by hardware convention) the instruction is entered prior to any
 * pending interrupt.
 *
 * B.) The instruction is entered with the interrupt system off and
 * exits when an interrupt becomes pending.  In this case the next
 * instruction (or so) must turn on the interrupt system, at which time
 * the pending interrupt(s) will be serviced.
 *
 * You must code a call to vst_wakeup() in each possible interrupt path.
 * In case B) you could just code the call between the idle instruction
 * and the enabling of the interrupt system.  In case A.) all possible
 * interrupts must include the call.
 *
 * We define the call with two parameters here, to match expected usage
 * where the first parameter would be a *regs and the second the irq or
 * some other flag (in x86 this is used to indicate that the call came
 * from the timer expire interrupt which is a totally seperate interrupt
 * in the x86 which does not go through do_IRQ).  Since this is not seen
 * by any common code, an arch can code what ever it wants here.  Do
 * note, however, that the arch/vst.h is not included in the case of
 * CONFIG_VST being undefined so, if you include this file in an arch
 * file, you will have this definition.  You could just include the
 * arch/vst.h an avoid this define...
 */
#define vst_wakeup(regs, flag)


/*
 * The following function should be inserted in the archs idle code just
 * prior to the sleep instruction (hlt in x86).  If vst is configured in
 * this function will scan the timer list and call the above
 * vst_sleep_till() function (or not) as determined by the scan.  It is
 * imperative that the sleep instruction not restart after interruption
 * and that the idle code again call this function prior to the next
 * sleep instruction.  (This requirement is met by the instruction/
 * hardware saving the next instruction address on an interrupt rather
 * than the current instruction.  This is the case for the x86 hlt and,
 * I suppose, most other hlt instructions.)
 * 
 * This function will return true if the entry to vst_sleep (and idle)
 * is successful.  In this case it will return with the interrupt system
 * off.  If the entry is not successful (i.e. an interrupt happened that
 * either touched the timer list or caused some other task to run) this
 * function will return false, with the interrupt system on.  The
 * correct thing to do here is to restart the idle entry.

 * Please note you are not being asked to implement this function, but
 * just to insert this call in your archs idle loop just prior to the
 * idle shutdown.  This function returns with the interrupt system off
 * and it should not be turned on until an interrupt will be guaranteed
 * to pull the cpu out of the idle state.
 */
#ifndef vst_setup
#define vst_setup() 0
#endif
/*
 * Locking: The vst_sleep_till() function is entered with interrupts
 * off.  It should take what ever locks it needs to protect the timer
 * etc. but need not worry about the interrupt system (but must leave it
 * off).
 *
 * The three variables that vst_wakeup() is to touch, i.e.:
 * vst_successful_exit
 * vst_skipped_interrupts
 * vst_external_intr_exit
 *
 * should all be touched under the write_seqlock_irq(&xtime_lock).  This
 * lock is needed to touch jiffies so this should not be a problem.
 */

/*
 * Summary: The arch include file "asm/vst.h" must define the following:
 * vst_sleep_till(jiffies)   set up end of sleep interrupt
 * vst_stop_jiffie_int()     stop jiffies interrupter
 * vst_start_jiffie_int()    restart jiffies interrupter and adjust jiffies
 * vst_wakeup(regs)          wake up after a vst sleep
 * vst_setup()               enter vst (called from idle)
 */

#endif

#define MAX_TIMER_INTERVAL	LONG_MAX

extern atomic_t vst_no_timers, vst_short_timers, vst_long_timers;
extern unsigned long vst_start, vst_stop;
extern int vst_idle_jiffies;
extern int vst_jiffie_clock_disabled;

/* Possible return values from find_next_timer: */
enum {
	SCAN_INTERRUPTUS,       /* An interrupt happend so we abort the scan */
	SHORT_NEXT_TIMER,	/* Next timer is less than threshold */
	RETURNED_NEXT_TIMER,	/* *next contains when next timer will expire */
	NO_NEXT_TIMER		/* No timers in lists */
};
/*
 * This could be a real problem if the stop gets interrupted.  Protect
 * the whole thing with a spinlock.
 */
#ifndef vst_smp_cpu_enter
#define vst_smp_cpu_enter()
#endif
#ifndef vst_smp_cpu_exit
#define vst_smp_cpu_exit()
#endif
#ifdef CONFIG_VST
static inline void vst_sleeping(void)
{
	unsigned long flags;

	spin_lock_irqsave(&vst_int_disable_lock, flags); 
	cpu_set(smp_processor_id(), vst_cpus); 
	if (cpus_equal(vst_cpus, cpu_online_map)) { 
		vst_stop_jiffie_int();	
	}
	vst_smp_cpu_enter();
	spin_unlock_irqrestore(&vst_int_disable_lock, flags); 
} 
static inline void vst_waking(void)
{
	unsigned long flags;

	spin_lock_irqsave(&vst_int_disable_lock, flags);
	vst_smp_cpu_exit();
	if (cpus_equal(vst_cpus,cpu_online_map ))	
		vst_start_jiffie_int();	
	cpu_clear(smp_processor_id(), vst_cpus);
	spin_unlock_irqrestore(&vst_int_disable_lock, flags);
} 
/* 
 * Should only need the following if SMP where the 'other' cpu(s) might
 * push jiffies while we were asleep.  In that case we may need to run
 * the timer list to catch up (should be no work, however).
 */
#ifdef CONFIG_SMP
#include <linux/timer-list.h>

#define conditional_run_timers()				\
{								\
	tvec_base_t *base  = &__get_cpu_var(tvec_bases);	\
	if (base->timer_jiffies != jiffies)			\
		run_local_timers();				\
}
#else
#define conditional_run_timers();
#endif /* CONFIG_SMP */
#endif /* CONFIG_VST */
#ifndef RAW_SPIN_LOCK_UNLOCKED
typedef raw_spinlock_t spinlock_t
#define RAW_SPIN_LOCK_UNLOCKED SPIN_LOCK_UNLOCKED 
#endif
#endif /* _LINUX_VST_H */
