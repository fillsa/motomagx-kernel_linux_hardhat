/*
 *  include/asm-arm/arch-pnx4008/hrtime.h
 *
 * Author: Grigory Tolstolytkin <gtolstolytkin@ru.mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __ASM_ARCH_HRTIME_H_
#define __ASM_ARCH_HRTIME_H_

#ifdef CONFIG_HIGH_RES_TIMERS

#include <linux/timex.h>

int schedule_hr_timer_int(unsigned, int);
int get_arch_cycles(unsigned);

#define hrtimer_use	1

/*!
 * Resolution of the High-Resolution timer in nanoseconds. This should be a
 * value that makes sense on the given arch taking into account interrupt
 * latency and so forth. This value is used to define the
 * granularity of the timer requests and it is the resolution the
 * standard talks about. It is used to define the resolution of
 * CLOCK_REALTIME_HR and CLOCK_MONOTONIC_HR.
 */
#define hr_time_resolution      (NSEC_PER_SEC / CLOCK_TICK_RATE) /* 1000 ns */

/*!
 * Number of hr_clock_cycles in a jiffy.
 */
#define arch_cycles_per_jiffy   (CLOCK_TICK_RATE / HZ)

/*!
 * This macro is called whenever the run_timer_list code has
 * caught up to the current time and determines that the next
 * interrupt should be for the next jiffy.  The code will never
 * call both schedule_hr_timer_int() and schedule_jiffies_int() in
 * the same pass. It should return true (!0) if ref_jiffies + 1 is
 * in the past.
 */
#define schedule_jiffies_int(x) (get_arch_cycles(x) >= arch_cycles_per_jiffy)

#define SC_ARCH2NSEC        20	/* int(log2(2 * CLOCK_TICK_RATE - 1)) */
#define SC_NSEC2ARCH	32

#define scaled_nsec_per_arch_cycle	\
	(SC_n(SC_ARCH2NSEC, NSEC_PER_SEC) / CLOCK_TICK_RATE)

#define scaled_arch_cycles_per_nsec	\
	(SC_n(SC_NSEC2ARCH, CLOCK_TICK_RATE) / NSEC_PER_SEC)

/*!
 * This macro converts hr_clock_cycles to nanoseconds.
 * NOTE: hr_clock_cycles may be negative.
 */
#define arch_cycle_to_nsec(cycles)	\
	mpy_sc_n(SC_ARCH2NSEC, (cycles), scaled_nsec_per_arch_cycle)

/*!
 * This macro converts nanoseconds to hr_clock_cycles.
 * NOTE: hr_clock_cycles may be negative.
 */
#define nsec_to_arch_cycle(nsec)	\
	mpy_sc_n(SC_NSEC2ARCH, (nsec), scaled_arch_cycles_per_nsec)

#endif /* CONFIG_HIGH_RES_TIMERS */
#endif /* __ASM_ARCH_HRTIME_H_ */
