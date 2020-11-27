/*
 * include/asm-arm/arch-ixp4xx/preempt.h
 *
 * Timing support for preempt-stats, kfi, ilatency patches
 *
 * Author: Deepak Saxena <dsaxena@mvista.com>
 *
 * NOTE:
 *
 * This implementation requires the XScale PMU exclusively for preemptible
 * latency measurements. The PMU cannot be used for other performance
 * measurements concurrently with preemption latency measurement support.
 *
 * 2001-2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef _ASM_ARCH_PREEMT_H
#define _ASM_ARCH_PREEMT_H

#define readclock(x)	do {						\
		__asm__ __volatile__("mrc	p14, 0, %0, c1, c1, 0"	\
			: "=r" (x));					\
	} while (0)

static inline unsigned clock_to_usecs(unsigned x)
{
	extern unsigned int processor_id;
	if(processor_id & 0x00000030)
		return ((x) / 266);
	else if(processor_id & 0x00000010)
		return ((x) / 400);
	else
		return ((x) / 533);
}

static inline unsigned __ticks_per_usec(void)
{
	extern unsigned int processor_id;

	if(processor_id & 0x00000030)
		return 266;
	else if(processor_id & 0x00000010)
		return 400;
	else
		return 533;
}

#define TICKS_PER_USEC	__ticks_per_usec()

#define INTERRUPTS_ENABLED(x)   (!(x & PSR_I_BIT))

#endif


