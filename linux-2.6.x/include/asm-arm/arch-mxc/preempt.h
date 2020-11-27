/*
 * linux/include/asm-arm/arch-mxc/preempt.h
 *
 * Timing support for preempt stats on MXC
 *
 * Author: MontaVista Software, Inc. <source@mvista.com>
 *
 * Copyright (C) 2007 Motorola, Inc.
 *
 * 2003-2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * Date     Author    Comment
 * 04/2007  Motorola  Added support for 32KHz clock source.
 */

#ifndef __ASM_ARCH_PREEM_LATENCY_H__
#define __ASM_ARCH_PREEM_LATENCY_H__

/*!
 * @defgroup PREEMPT Preemption Latency Timing
 * @ingroup MSL
 */

/*!
 * @file preempt.h
 *
 * @brief MontaVista Preemption Latency Timing
 *
 *
 * @ingroup PREEMPT
 */

#include <linux/init.h>
#include <asm/hardware.h>
#include <asm/io.h>

/*!
 * This macro is used to start a monotonic incremental counter which can be
 * used for various kernel timing measurements. Since the timer used for
 * periodic system timer interrupts already provides for a monotonic
 * incremental counter, we define this as NULL.
 */
#define readclock_init()

/*!
 * Returns time delta between START and STOP readclock() time samples.
 */
static inline unsigned long clock_diff(unsigned long start, unsigned long stop)
{
	return (stop - start);
}

/*!
 * This macro returns a monotonic incremental counter value.
 */
#define readclock()	__raw_readl(MXC_GPT_GPTCNT)

#define INTERRUPTS_ENABLED(x) (!(x & PSR_I_BIT))

extern unsigned long clock_to_usecs(unsigned long);

/*!
 * Tells interrupt latency instrumentation to use predefined TICKS_PER_USEC value
 */
#define ARCH_PREDEFINES_TICKS_PER_USEC

#ifdef CONFIG_MOT_FEAT_32KHZ_GPT
#define TICKS_PER_USEC          0
#else
#if	CLOCK_TICK_RATE < USEC_PER_SEC
#error	CLOCK_TICK_RATE must be greater than USEC_PER_SEC
#endif

#define TICKS_PER_USEC		(CLOCK_TICK_RATE / USEC_PER_SEC)
#endif /* CONFIG_MOT_FEAT_32KHZ_GPT */

#endif
