/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2005-2007 Motorola, Inc. All rights reserved. 
 *
 * System Timer Interrupt reconfigured to run in free-run mode.
 * Author: Vitaly Wool
 * Copyright 2004 MontaVista Software Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*!
 * @file time.c
 * @brief This file contains OS tick and wdog timer implementations.
 *
 * This file contains OS tick and wdog timer implementations.
 *
 * @ingroup Timers
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/hrtime.h>
#include <asm/mach/time.h>
#include <asm/io.h>
#include "time_priv.h"

/*!
 * This function converts system timer ticks to microseconds
 *
 * @param  x	system timer ticks
 *
 * @return elapsed microseconds
 */
unsigned long __noinstrument clock_to_usecs(unsigned long x)
{
	return (unsigned long)(x * (tick_nsec / 1000)) / LATCH;
}

#ifdef CONFIG_KFI
/*!
 * OS timer state flag
 */
static int os_timer_initialized = 0;

/*!
 * This function is needed by KFI to convert machine cycles to microseconds
 *
 * @param mputicks	number of machine cycles
 *
 * @return elapsed microseconds
 */
unsigned long __noinstrument machinecycles_to_usecs(unsigned long mputicks)
{
	return clock_to_usecs(mputicks);
}

/*!
 * This function is needed by KFI to obtain current number of machine cycles elapsed
 *
 * @return elapsed machine cycles, or 0 if GPT timer is not initialized
 */
unsigned long __noinstrument do_getmachinecycles(void)
{
	return os_timer_initialized ? __raw_readl(MXC_GPT_GPTCNT) : 0;
}
#endif

/*
 * WatchDog
 */
typedef struct {
	volatile __u16 WDOG_WCR;	/* 16bit watchdog control reg */
	volatile __u16 WDOG_WSR;	/* 16bit watchdog service reg */
	volatile __u16 WDOG_WRSR;	/* 16bit watchdog reset status reg */
} wdog_reg_t;

/*!
 * The base addresses for the WDOG module
 */
static volatile wdog_reg_t *wdog_base[1] = {
	(volatile wdog_reg_t *)IO_ADDRESS(WDOG_BASE_ADDR),
};

/*!
 * The corresponding WDOG won't be serviced unless the corresponding global
 * variable is set to a non-zero value.
 */
volatile unsigned short g_wdog1_enabled;

/* WDOG WCR register's WT value */
static int wdog_tmout[1] = { WDOG1_TIMEOUT };

/*!
 * This function provides the required service for the watchdog to avoid
 * the timeout.
 */
#ifndef CONFIG_MOT_FEAT_KPANIC
static inline
#endif /* CONFIG_MOT_FEAT_KPANIC */ 
void kick_wd(void)
{
	if (g_wdog1_enabled) {
		/* issue the service sequence instructions */
		wdog_base[0]->WDOG_WSR = 0x5555;
		wdog_base[0]->WDOG_WSR = 0xAAAA;
	}
}

/*!
 * This is the watchdog initialization routine to setup the timeout
 * value and enable it.
 */
void mxc_wd_init(int port)
{
	unsigned volatile short timeout =
	    ((wdog_tmout[port] / 1000) * 2) << WDOG_WT;

	if (port == 0) {
		/* enable WD, suspend WD in DEBUG mode */
		wdog_base[port]->WDOG_WCR = timeout | WCR_WOE_BIT |
		    WCR_SRS_BIT | WCR_WDA_BIT | WCR_WDE_BIT | WCR_WDBG_BIT;
	}
}

#ifdef WDOG_SERVICE_PERIOD
static int g_wdog_count = 0;
#endif

/*!
 * This is the timer interrupt service routine to do required tasks.
 * It also services the WDOG timer at the frequency of twice per WDOG
 * timeout value. For example, if the WDOG's timeout value is 4 (2
 * seconds since the WDOG runs at 0.5Hz), it will be serviced once
 * every 2/2=1 second.
 *
 * @param  irq          GPT interrupt source number (not used)
 * @param  dev_id       this parameter is not used
 * @param  regs         pointer to a structure containing the processor
 *                      registers and state prior to servicing the interrupt
 * @return always returns \b IRQ_HANDLED as defined in
 *         include/linux/interrupt.h.
 */
static irqreturn_t mxc_timer_interrupt(int irq, void *dev_id,
				       struct pt_regs *regs)
{
	unsigned int next_match;

#ifdef	CONFIG_HIGH_RES_TIMERS
	u32 reg;
	if (__raw_readl(MXC_GPT_GPTSR) & GPTSR_OF2) {
		__raw_writel(GPTSR_OF2, MXC_GPT_GPTSR);
		reg = __raw_readl(MXC_GPT_GPTIR);
		reg &= ~GPTIR_OF2IE;
		reg = __raw_writel(reg, MXC_GPT_GPTIR);
		do_hr_timer_int();
	}
#endif

	write_seqlock(&xtime_lock);

	if (__raw_readl(MXC_GPT_GPTSR) & GPTSR_OF1)
		do {
#ifdef WDOG_SERVICE_PERIOD
			if (g_wdog1_enabled &&
			    (++g_wdog_count >=
			     ((WDOG_SERVICE_PERIOD / 1000) * HZ))) {
				kick_wd();
				g_wdog_count = 0;	/* reset */
			}
#else
			kick_wd();
#endif				/* WDOG_SERVICE_PERIOD */
			timer_tick(regs);
			next_match = __raw_readl(MXC_GPT_GPTOCR1) + LATCH;
			__raw_writel(GPTSR_OF1, MXC_GPT_GPTSR);
			__raw_writel(next_match, MXC_GPT_GPTOCR1);
		} while ((signed long)(next_match -
				       __raw_readl(MXC_GPT_GPTCNT)) <= 0);

	write_sequnlock(&xtime_lock);

	return IRQ_HANDLED;
}

/*!
 * This function is used to obtain the number of microseconds since the last
 * timer interrupt. Note that interrupts is disabled by do_gettimeofday().
 *
 * @return the number of microseconds since the last timer interrupt.
 */
static unsigned long __noinstrument mxc_gettimeoffset(void)
{
#ifdef CONFIG_MOT_FEAT_GETTIMEOFFSET_2618
	long ticks_to_match, elapsed, usec;

	/* Get ticks before next timer match */
	ticks_to_match =
		__raw_readl(MXC_GPT_GPTOCR1) - __raw_readl(MXC_GPT_GPTCNT);

	/* We need elapsed ticks since last match */
	elapsed = LATCH - ticks_to_match;

	/* Now convert them to usec */
	usec = (unsigned long)(elapsed * (tick_nsec / 1000)) / LATCH;

	return usec;
#else
	unsigned long ticks_to_match, elapsed, usec, tick_usec, i;

	/* Get ticks before next timer match */
	ticks_to_match =
	    __raw_readl(MXC_GPT_GPTOCR1) - __raw_readl(MXC_GPT_GPTCNT);

	/* We need elapsed ticks since last match */
	elapsed = LATCH - ticks_to_match;

	/* Now convert them to usec */
	/* Insure no overflow when calculating the usec below */
	for (i = 1, tick_usec = tick_nsec / 1000;; i *= 2) {
		tick_usec /= i;
		if ((0xFFFFFFFF / tick_usec) > elapsed)
			break;
	}
	usec = (unsigned long)(elapsed * tick_usec) / (LATCH / i);

	return usec;
#endif
}

/*!
 * The OS tick timer interrupt structure.
 */
static struct irqaction timer_irq = {
	.name = "MXC Timer Tick",
	.flags = SA_INTERRUPT | SA_SHIRQ | SA_NODELAY,
	.handler = mxc_timer_interrupt
};

/*!
 * This function is used to initialize the GPT to produce an interrupt
 * every 10 msec. It is called by the start_kernel() during system startup.
 */
void __init mxc_init_time(void)
{
	u32 reg;
	reg = __raw_readl(MXC_GPT_GPTCR);
	reg &= ~GPTCR_ENABLE;
	__raw_writel(reg, MXC_GPT_GPTCR);
	reg |= GPTCR_SWR;
	__raw_writel(reg, MXC_GPT_GPTCR);

	while ((__raw_readl(MXC_GPT_GPTCR) & GPTCR_SWR) != 0)
		mb();

	reg = __raw_readl(MXC_GPT_GPTCR);

	reg = 0 * GPTCR_OM3_CLEAR | GPTCR_FRR | GPTCR_CLKSRC_HIGHFREQ;

	__raw_writel(reg, MXC_GPT_GPTCR);

	__raw_writel(MXC_TIMER_DIVIDER - 1, MXC_GPT_GPTPR);

	reg = __raw_readl(MXC_GPT_GPTCNT);
	reg += LATCH;
	__raw_writel(reg, MXC_GPT_GPTOCR1);

	setup_irq(INT_GPT, &timer_irq);

	reg = __raw_readl(MXC_GPT_GPTCR);
	reg = GPTCR_FRR | GPTCR_CLKSRC_HIGHFREQ |
	    GPTCR_STOPEN |
	    GPTCR_DOZEN | GPTCR_WAITEN | GPTCR_ENMOD | GPTCR_ENABLE;
	__raw_writel(reg, MXC_GPT_GPTCR);

	__raw_writel(GPTIR_OF1IE, MXC_GPT_GPTIR);

#ifdef WDOG1_ENABLE
	mxc_wd_init(0);
	g_wdog1_enabled = 1;
#else
	g_wdog1_enabled = (wdog_base[0]->WDOG_WCR) & WCR_WDE_BIT;
#endif

	kick_wd();

#ifdef CONFIG_KFI
	os_timer_initialized = 1;
#endif
}

EXPORT_SYMBOL(g_wdog1_enabled);
EXPORT_SYMBOL(mxc_wd_init);

struct sys_timer mxc_timer = {
	.init = mxc_init_time,
	.offset = mxc_gettimeoffset,
};

#ifdef CONFIG_HIGH_RES_TIMERS
/*
 * High-Res Timer Implementation
 *
 * 2004-2005 (c) MontaVista Software, Inc.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*!
 * This function schedules the next hr clock interrupt at the time specified
 * by ref_jiffies and ref_cycles.
 *
 *	If a previously scheduled interrupt has not happened yet, it is
 *	discarded and replaced by the new one.  (In practice, this will
 *	happen when a new timer is entered prior to the one last asked
 *	for.)
 *
 *	Arch can assume that
 *		0 <= hr_clock_cycles <= arch_cycles_per_jiffy
 *		-10 <= (jiffies - ref_jiffies) <= 10 jiffies
 *
 *	Locking: This function is called under the spin_lock_irq for the
 *	timer list, thus interrupts are off.
 *
 * @param  ref_jiffies  jiffy (Hz) boundary during which to schedule subHz
 *			interrupt
 * @param  ref_cycles   number of system timer clock ticks to delay from
 *			jiffy (Hz) boundary
 *
 * @return non-zero (-ETIME) if the specified time is already passed; 0 on
 *	   success
 */
int schedule_hr_timer_int(unsigned ref_jiffies, int ref_cycles)
{
	int temp_cycles;
	u32 reg;

	BUG_ON(ref_cycles < 0);

	/*
	 * Get offset from next jiffy
	 * NOTE: GPTOCR1 is always the GPT match value for jiffies+1
	 */
	temp_cycles = (ref_jiffies - jiffies) * arch_cycles_per_jiffy +
		    ref_cycles;

	if (unlikely(temp_cycles <= 0))
		return -ETIME;

	reg = __raw_readl(MXC_GPT_GPTICR1) + temp_cycles;
	if (unlikely(reg <= __raw_readl(MXC_GPT_GPTCNT)))
		return -ETIME;
	__raw_writel(reg, MXC_GPT_GPTOCR2);
	reg = __raw_readl(MXC_GPT_GPTIR);
	reg |= GPTIR_OF2IE;
	__raw_writel(reg, MXC_GPT_GPTIR);

	return 0;
}

/*!
 * This function returns time elapsed from the reference jiffies to present
 * in the units of hr_clock_cycles.
 *
 *	Arch code can assume
 *		0 <= (jiffies - ref_jiffies) <= 10 jiffies
 *		ref_jiffies may not increase monotonically.
 *
 *	Locking: The caller will provide locking equivalent to read_lock
 *	on the xtime_lock.  The callee need only lock hardware if needed.
 *
 * @param  ref_jiffies  reference jiffies
 *
 * @return number of hr_clock_cycles elapsed from the reference jiffies to
 *	   present.
 */
int get_arch_cycles(unsigned ref_jiffies)
{
	int ret;
	unsigned temp_jiffies;
	unsigned diff_jiffies;

	do {
		/* snapshot jiffies */
		temp_jiffies = jiffies;
		barrier();

		/* calculate cycles since the current jiffy */
		ret =
		    __raw_readl(MXC_GPT_GPTOCR1) - __raw_readl(MXC_GPT_GPTCNT);
		ret = arch_cycles_per_jiffy - ret;

		/* compensate for ref_jiffies in the past */
		if (unlikely(diff_jiffies = jiffies - ref_jiffies))
			ret += diff_jiffies * arch_cycles_per_jiffy;

		barrier();
		/* repeat if we didn't have a consistent view of the world */
	} while (unlikely(temp_jiffies != jiffies));

	return ret;
}

EXPORT_SYMBOL(get_arch_cycles);
#endif
