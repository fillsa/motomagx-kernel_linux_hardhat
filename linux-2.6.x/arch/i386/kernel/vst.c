/*
 *  linux/arch/i386/kernel/vst.c
 *
 *  VST code for x86.
 *
 *  2004            VST and IDLE code, by George Anzinger
 *                  Copyright (C)2004 by MontaVista Software.
 *
 * 2004 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/vst.h>
#include <linux/irq.h>   /* to get the disable/enable irq code */
#include <linux/hrtime.h> /* get_arch_cycles, arch_chcles_per_jiffy */
#include <linux/time.h>  /* xtime_lock */
#include <asm/hw_irq.h>

#define stop_timer()   /* just let it expire.... */

long vst_max_jiffies_sleep;

void do_vst_wakeup(struct pt_regs *regs, int irq_flag)
{
	unsigned long jiffies_delta, flags;

	if (!in_vst_sleep()) 
		return;

	vst_waking();

	write_seqlock_irqsave(&xtime_lock, flags);

	if (irq_flag )
		vst_successful_exit++;
	else
		vst_external_intr_exit++;
	stop_timer();
	/*
	 * OK, now we need to get jiffies up to the right value.  Here
	 * we lean on the HRT patch to give us some notion of where we
	 * are.
	 * Fix me: This needs to be based on an expanded notion of time.
	 * One word TSC is too limited.  Change to two words...
	 */
	jiffies_delta = (unsigned long)get_arch_cycles(jiffies) / 
		arch_cycles_per_jiffy;
	if (jiffies_delta) {
		/*
		 * One or more jiffie has elapsed.  Do all but the last one
		 * here and then let the next timer interrutpt get the last 
		 * and update the wall clock.
		 */
		jiffies_64 += --jiffies_delta;
		last_update += jiffies_delta * arch_cycles_per_jiffy;
		vst_skipped_interrupts += jiffies_delta;

		run_local_timers();
	} else {
		conditional_run_timers();
	}
	write_sequnlock_irqrestore(&xtime_lock, flags);
	return;
}
void vst_stop_jiffie_int(void)
{
	irq_desc[0].handler->disable(0);
	vst_jiffie_clock_disabled = 1;
}
extern int timer_ack;
extern spinlock_t i8259A_lock;
#include "io_ports.h"
void vst_start_jiffie_int(void)
{
	vst_jiffie_clock_disabled = 0;
	irq_desc[0].handler->enable(0);
}
