#ifndef ASM_I386_VST
#define ASM_I386_VST 
/*
 * asm-i386/vst.h
 *
 * This file is licensed under  the terms of the GNU General Public 
 * License version 2. This program is licensed "as is" without any 
 * warranty of any kind, whether express or implied.
 *
 * I386/asm header file for VST (Variable Sleep Time, tick elimination)
 *
 * Author: George Anzinger george@mvista.com>
 *
 * Copyright (C) 2003 MontaVista, Software, Inc. 
 * Copyright 2003 Sony Corporation.
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 
 */
#include <linux/hrtime.h>
#include <asm/ptrace.h>
#include <linux/kernel.h>
extern int hrtimer_is_pending(void);

static inline void vst_sleeping(void);

extern long vst_max_jiffies_sleep;

static inline void vst_sleep_till(unsigned long rjiffies)
{
	if (unlikely(!vst_max_jiffies_sleep))
		vst_max_jiffies_sleep = LONG_MAX / arch_cycles_per_jiffy;
	if (unlikely((rjiffies - jiffies) > vst_max_jiffies_sleep))
		rjiffies = jiffies + vst_max_jiffies_sleep;
	schedule_hr_timer_int(rjiffies, 0);
	vst_sleeping();
}
#ifdef CONFIG_SMP
extern void disable_nmi_watchdog(void);
extern void  enable_nmi_watchdog(void);
#define vst_smp_cpu_enter() disable_nmi_watchdog()
#define vst_smp_cpu_exit() enable_nmi_watchdog()
#endif
extern void vst_stop_jiffie_int(void);
extern void vst_start_jiffie_int(void);
extern  void do_vst_wakeup(struct pt_regs *regs, int irq_flag);
#define vst_wakeup(regs, irq_flag) do_vst_wakeup(regs, irq_flag)

#endif
