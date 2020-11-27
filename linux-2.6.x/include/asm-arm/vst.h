#ifndef ASM_ARM_VST
#define ASM_ARM_VST 
/*
 * asm-arm/vst.h
 *
 * This file is licensed under  the terms of the GNU General Public 
 * License version 2. This program is licensed "as is" without any 
 * warranty of any kind, whether express or implied.
 *
 * arm/asm header file for VST (Variable Sleep Time, tick elimination)
 *
 * Author: George Anzinger george@mvista.com>
 *
 * Copyright (C) 2003 MontaVista, Software, Inc. 
 * Copyright 2003 Sony Corporation.
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 
 */
#include <linux/sched.h>
#include <linux/hrtime.h>
#include <asm/ptrace.h>
#include <asm/arch/vst.h>

#define vst_sleep_till(jiffies) do {					\
                                  schedule_hr_timer_int(jiffies, 0);	\
                                  vst_sleeping();			\
                                  } while (0)
#ifndef _arch_defined_vst_jiffie_mask
extern void vst_stop_jiffie_int(void);
extern void vst_start_jiffie_int(void);
#endif
extern void do_vst_wakeup(struct pt_regs *regs, int irq_flag);
#define vst_wakeup(regs, irq_flag) do_vst_wakeup(regs, irq_flag)

#endif
