/*
 *  linux/arch/arm/kernel/vst.c
 *
 *  VST code for ARM.
 *
 *  2004            VST and IDLE code, by George Anzinger
 *
 * 2004 (c) MontaVista Software, Inc.
 * Copyright 2004 Sony Corporation.
 * Copyright 2004 Matsushita Electric Industrial Co., Ltd.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/vst.h>
#include <linux/hrtime.h> /* get_arch_cycles, arch_cycles_per_jiffy */
#include <linux/time.h>   /* xtime_lock */
#include <asm/irq.h>      /* to get the disable/enable irq code */
#include <asm/mach/irq.h>

#define stop_timer()   /* just let it expire.... */
void do_vst_wakeup(struct pt_regs *regs, int irq_flag)
{
	unsigned long jiffies_delta, jiffies_f = jiffies;
	unsigned long flags;

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
	 */
	jiffies_delta = get_arch_cycles(jiffies_f) / arch_cycles_per_jiffy;

	if (jiffies_delta) {
		/*
		 * One or more jiffie has elapsed.  Do all but the last one
		 * here and then call do_timer() to get the last and update
		 * the wall clock.
		 */
		jiffies_delta--;
		vst_bump_jiffies_by(jiffies_delta);
		vst_skipped_interrupts += jiffies_delta;

		run_local_timers();
	} else {
		conditional_run_timers();
	}

	write_sequnlock_irqrestore(&xtime_lock, flags);
	
	return;
}

#ifndef _arch_defined_vst_jiffie_mask
void vst_stop_jiffie_int(void)
{
	irq_desc[ARCH_TIMER_IRQ].chip->mask(ARCH_TIMER_IRQ);
}

void vst_start_jiffie_int(void)
{
	irq_desc[ARCH_TIMER_IRQ].chip->unmask(ARCH_TIMER_IRQ);
}
#endif
