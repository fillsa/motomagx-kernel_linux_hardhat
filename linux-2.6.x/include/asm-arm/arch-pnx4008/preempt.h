/*
 *  include/asm-arm/arch-pnx4008/preempt.h
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __PNX4008_PREEMPT_H
#define __PNX4008_PREEMPT_H

#include <asm/arch/platform.h>
#include <asm/arch/timex.h>

static inline unsigned long clock_diff( unsigned long start, unsigned long stop )
{
	return stop - start;
}

#define readclock_init()
#define readclock() __raw_readl(HSTIM_COUNTER)
#define INTERRUPTS_ENABLED(x) (!((x) & PSR_I_BIT))  /* assume that x is PSR */
#define TICKS_PER_USEC 1
#define clock_to_usecs(x) TICKS2USECS(x)
#define ARCH_PREDEFINES_TICKS_PER_USEC TICKS_PER_USEC

#endif /* __PNX4008_PREEMPT_H */
