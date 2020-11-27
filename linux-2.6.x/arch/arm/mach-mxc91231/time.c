/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2008 Motorola, Inc.
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
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  --------------------
 * 10/06/2006   Motorola  Kernel panic support 
 * 01/12/2007   Motorola  Added watchdog debug support
 * 02/09/2007   Motorola  Rewirte the check start code of wdog make it more readable
 * 03/28/2007   Motorola  FN482: Fixed the calculation of timer
 *                        interrupt match in schedule_hr_timer_int 
 * 04/24/2007   Motorola  Added 32KHz clock support for GPT.
 * 11/13/2007   Motorola  Start the kernel thread to kick the watch dog 
 * 03/11/2008   Motorola  Adapted WDOG2 FIQ handler to FIQ handler in C language
 * 04/14/2008   Motorola  Improved mxc_gettimeoffset()
 */

/*!
 * @file time.c
 * @brief This file contains RTC, OS tick and wdog timer implementations.
 *
 * This file contains RTC, OS tick and wdog timer implementations.
 *
 * @ingroup Timers
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/hrtime.h>
#include <linux/wait.h>
#include <linux/proc_fs.h>
#include <linux/rtc.h>
#include <asm/mach/time.h>
#include <asm/io.h>
#include "time_priv.h"

#ifdef CONFIG_MOT_FEAT_DEBUG_WDOG
#include <asm/fiq.h>
#include <asm/arch/clock.h>
#endif

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

/*!
 * OS timer state flag, used by both KFI and PRINTK_TIME
 * patches
 */
#if defined(CONFIG_KFI) || defined(CONFIG_MOT_FEAT_PRINTK_TIMES)
static int os_timer_initialized = 0;
#endif /* defined(CONFIG_KFI) || defined(CONFIG_MOT_FEAT_PRINTK_TIMES) */

#ifdef CONFIG_KFI
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

#ifdef CONFIG_MOT_FEAT_PRINTK_TIMES
/*
 * This function is needed for displaying timing information (upto nanosecond resolution) next to each printk message.
 *
 * @return elapsed nanoseconds
 */
unsigned long long get_curr_ns_time(void) {
	return os_timer_initialized ? (((unsigned long long)(__raw_readl(MXC_GPT_GPTCNT))) * (NSEC_PER_SEC/CLOCK_TICK_RATE)) : 0;
}
#endif /* CONFIG_MOT_FEAT_PRINTK_TIMES */

/*
 * WatchDog
 */
typedef struct {
	volatile __u16 WDOG_WCR;	/* 16bit watchdog control reg */
	volatile __u16 WDOG_WSR;	/* 16bit watchdog service reg */
	volatile __u16 WDOG_WRSR;	/* 16bit watchdog reset status reg */
} wdog_reg_t;

/*!
 * The base addresses for the WDOG modules
 */
static volatile wdog_reg_t *wdog_base[2] = {
	(volatile wdog_reg_t *)IO_ADDRESS(WDOG1_BASE_ADDR),
	(volatile wdog_reg_t *)IO_ADDRESS(WDOG2_BASE_ADDR),
};

/*!
 * The corresponding WDOG won't be serviced unless the corresponding global
 * variable is set to a non-zero value.
 */
volatile unsigned short g_wdog1_enabled, g_wdog2_enabled;

/* WDOG WCR register's WT value */
static int wdog_tmout[2] = { WDOG1_TIMEOUT, WDOG2_TIMEOUT };

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
	if (g_wdog2_enabled) {
		/* issue the service sequence instructions */
		wdog_base[1]->WDOG_WSR = 0x5555;
		wdog_base[1]->WDOG_WSR = 0xAAAA;
	}
}

#ifdef CONFIG_MOT_FEAT_DEBUG_WDOG
static struct fiq_handler wdog2_fh = {
	name: "WDOG2"
};
#endif /* CONFIG_MOT_FEAT_DEBUG_WDOG */

/*!
 * This is the watchdog initialization routine to setup the timeout
 * value and enable it.
 */
void mxc_wd_init(int port)
{
#ifdef CONFIG_MOT_FEAT_DEBUG_WDOG
	extern unsigned char wdog2_fiq_handler_start, wdog2_fiq_handler_end;
	struct pt_regs fiq_regs;
#endif /* CONFIG_MOT_FEAT_DEBUG_WDOG */
	unsigned volatile short timeout =
	    ((wdog_tmout[port] / 1000) * 2) << WDOG_WT;

	if (port == 0) {
		/* enable WD, suspend WD in DEBUG mode and low power mode */
		wdog_base[port]->WDOG_WCR = timeout | WCR_WOE_BIT |
		    WCR_SRS_BIT | WCR_WDA_BIT | WCR_WDE_BIT | WCR_WDBG_BIT | WCR_WDZST_BIT;
	} else {
		/* enable WD, suspend WD in DEBUG, low power modes and WRE=1 */
#ifndef CONFIG_MOT_FEAT_DEBUG_WDOG
		wdog_base[port]->WDOG_WCR = timeout | WCR_WOE_BIT |
		    WCR_SRS_BIT | WCR_WRE_BIT | WCR_WDE_BIT | WCR_WDBG_BIT;
#else
		mxc_clks_enable(WDOG2_CLK);

		wdog_base[port]->WDOG_WCR = timeout | WCR_WOE_BIT |
		    WCR_WDA_BIT | WCR_SRS_BIT |  WCR_WDE_BIT | WCR_WRE_BIT | WCR_WDBG_BIT | WCR_WDZST_BIT;
		/* enable FIQ for INT_WDOG2 */
#ifdef CONFIG_MOT_FEAT_FIQ_IN_C
		__raw_writel(1 << (INT_WDOG2 - 32), AVIC_INTTYPEH);
		enable_fiq(INT_WDOG2);
#else
		if (claim_fiq(&wdog2_fh))
			printk("Couldn't claim FIQ for INT_WDOG2\n");
		else {
			printk("FIQ claimed for INT_WDOG2\n");	
			__raw_writel(1 << (INT_WDOG2 - 32), AVIC_INTTYPEH);

			/* FIQ handler must be no more 
			 * than to 0x200-0x01c (484) bytes 
			 */
			set_fiq_handler(&wdog2_fiq_handler_start, &wdog2_fiq_handler_end - &wdog2_fiq_handler_start);
			fiq_regs.ARM_r8 = (long)(AVIC_BASE);
			set_fiq_regs(&fiq_regs);
			enable_fiq(INT_WDOG2);
		}
#endif /* CONFIG_MOT_FEAT_FIQ_IN_C */
#endif /* CONFIG_MOT_FEAT_DEBUG_WDOG */
	}

#ifdef CONFIG_MOT_WFN321
        wdog_base[port]->WDOG_WCR |= WCR_WDZST_BIT;
#endif
}

#ifdef WDOG_SERVICE_PERIOD
static int g_wdog_count = 0;
static DECLARE_WAIT_QUEUE_HEAD(mxc_watchdog_wait);
static unsigned short wdog_reset_status = 0;

//#define MXC_WDT_KICK_THREAD_DEBUG	1

static int mxc_wdt_kick_thread(void *unused)
{
	int ret;

	daemonize("mxcwdt_kick");
#ifdef MXC_WDT_KICK_THREAD_DEBUG
	allow_signal(SIGKILL);
#endif

	for(;;) {
		ret = wait_event_interruptible(mxc_watchdog_wait,
			g_wdog_count >= ((WDOG_SERVICE_PERIOD / 1000) * HZ));
#ifdef MXC_WDT_KICK_THREAD_DEBUG
		if (ret < 0) {
			printk("Signal received, exit ...\n");
			break;
		}
#endif
		kick_wd();
		g_wdog_count = 0;
	}

	return 0;
}
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
	if (__raw_readl(MXC_GPT_GPTSR) & GPTSR_OF2) {
		__raw_writel(GPTSR_OF2, MXC_GPT_GPTSR);
		__raw_writel(__raw_readl(MXC_GPT_GPTIR) & ~GPTIR_OF2IE, MXC_GPT_GPTIR);
		do_hr_timer_int();
	}
#endif

	write_seqlock(&xtime_lock);

	if (__raw_readl(MXC_GPT_GPTSR) & GPTSR_OF1)
		do {
#ifdef WDOG_SERVICE_PERIOD
			if ((g_wdog1_enabled || g_wdog2_enabled) &&
			    (++g_wdog_count >=
			     ((WDOG_SERVICE_PERIOD / 1000) * HZ))) {
				wake_up_interruptible(&mxc_watchdog_wait);
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

#ifdef CONFIG_MOT_FEAT_DEBUG_WDOG

static irqreturn_t wdog2_irq_handler(int irq, void *did, struct pt_regs *regs)
{
	printk("Current Process: %s, PID: %d.\n", current->comm, current->pid);
	dump_stack();
	panic("Watchdog timeout");
	return IRQ_HANDLED;
}

static struct irqaction wdog2_irq = {
	.name = "WDOG2",
	.flags = SA_INTERRUPT,
	.handler = wdog2_irq_handler
};
#endif /* CONFIG_MOT_FEAT_DEBUG_WDOG */

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
#ifdef CONFIG_MOT_FEAT_32KHZ_GPT
	reg = 0 * GPTCR_OM3_CLEAR | GPTCR_FRR | GPTCR_CLKSRC_CLK32K;
#else
	reg = 0 * GPTCR_OM3_CLEAR | GPTCR_FRR | GPTCR_CLKSRC_HIGHFREQ;
#endif /* CONFIG_MOT_FEAT_32KHZ_GPT */

	__raw_writel(reg, MXC_GPT_GPTCR);

	__raw_writel(MXC_TIMER_DIVIDER - 1, MXC_GPT_GPTPR);

	reg = __raw_readl(MXC_GPT_GPTCNT);
	reg += LATCH;
	__raw_writel(reg, MXC_GPT_GPTOCR1);

	setup_irq(INT_GPT, &timer_irq);
#ifdef CONFIG_MOT_FEAT_DEBUG_WDOG
	setup_irq(INT_WDOG2, &wdog2_irq);
#endif  /* CONFIG_MOT_FEAT_DEBUG_WDOG */

	reg = __raw_readl(MXC_GPT_GPTCR);
#ifdef CONFIG_MOT_FEAT_32KHZ_GPT
	reg = GPTCR_FRR | GPTCR_CLKSRC_CLK32K |
	    GPTCR_STOPEN |
	    GPTCR_DOZEN | GPTCR_WAITEN | GPTCR_ENABLE;
#else
	reg = GPTCR_FRR | GPTCR_CLKSRC_HIGHFREQ |
	    GPTCR_STOPEN |
	    GPTCR_DOZEN | GPTCR_WAITEN | GPTCR_ENMOD | GPTCR_ENABLE;
#endif /* CONFIG_MOT_FEAT_32KHZ_GPT */
	__raw_writel(reg, MXC_GPT_GPTCR);

	__raw_writel(GPTIR_OF1IE, MXC_GPT_GPTIR);

#ifdef CONFIG_MOT_FEAT_WDOG_CLEANUP
	wdog_reset_status = wdog_base[0]->WDOG_WRSR;
	g_wdog1_enabled = (wdog_base[0]->WDOG_WCR) & WCR_WDE_BIT;
	if (g_wdog1_enabled)  /* WDOG 1 has been enabled by the bootloader */
	{
            /*Reset the time out of WD when although the MBM has set*/
            unsigned short wcr = wdog_base[0]->WDOG_WCR;
            wcr &= 0xff;
            wcr |= ((wdog_tmout[0] / 1000) * 2) << WDOG_WT;
            wdog_base[0]->WDOG_WCR = wcr;
            printk ("Watch dog 1 enabled by bootloader.\n");
	}
#ifdef CONFIG_MOT_FEAT_WDOG1_ENABLE
	else
	{
		mxc_wd_init(0);
		g_wdog1_enabled = 1;
		printk ("Watch dog 1 enabled.\n");
	}
#endif
#else
#ifdef WDOG1_ENABLE
	mxc_wd_init(0);
	g_wdog1_enabled = 1;
#else
	g_wdog1_enabled = (wdog_base[0]->WDOG_WCR) & WCR_WDE_BIT;
#endif
#endif
#ifdef WDOG2_ENABLE
	mxc_wd_init(1);
	g_wdog2_enabled = 1;
#else
	g_wdog2_enabled = (wdog_base[1]->WDOG_WCR) & WCR_WDE_BIT;
#endif

	kick_wd();

#if defined(CONFIG_KFI) || defined(CONFIG_MOT_FEAT_PRINTK_TIMES)
	os_timer_initialized = 1;
#endif /* defined(CONFIG_KFI) || defined(CONFIG_MOT_FEAT_PRINTK_TIMES) */
}

EXPORT_SYMBOL(g_wdog1_enabled);
EXPORT_SYMBOL(g_wdog2_enabled);
EXPORT_SYMBOL(mxc_wd_init);

struct sys_timer mxc_timer = {
	.init = mxc_init_time,
	.offset = mxc_gettimeoffset,
};

#ifdef WDOG_SERVICE_PERIOD
static int mxc_wdt_proc_read(char *buf, char **start,
                                       off_t offset, int count,
                                       int *eof, void *data)
{
	int len = 0, wrsr;
	char temp[32];

	len += sprintf(buf + len, "jiffies: %lu(10ms), rtc_sw_msfromboot: %lu(ms).\n",
			jiffies, rtc_sw_msfromboot());
	len += sprintf(buf + len,"Watchdog WRSR: 0x%X: ", wdog_reset_status);
	wrsr = wdog_reset_status & 0x3f;
	switch (wrsr) {
		case 1:
			sprintf(temp, "Software Reset\n");
			break;
		case 2:
			sprintf(temp, "Watchdog Timeout Reset\n");
			break;
		case 4:
			sprintf(temp, "Clock Monitor Reset\n");
			break;
		case 8:
			sprintf(temp, "External Reset\n");
			break;
		case 16:
			sprintf(temp, "PowerOn Reset\n");
			break;
		case 32:
			sprintf(temp, "JTAG Reset\n");
			break;
		default:
			sprintf(temp, "Impossible Multi Reset!\n");
			break;
	}
	len += sprintf(buf + len, "%s", temp);

	return len;
}

static int __init mxc_wdt_kick_thread_init(void)
{
#ifdef CONFIG_PROC_FS
	create_proc_read_entry("wdt_kick", 0, NULL, mxc_wdt_proc_read, NULL);
#endif
	kernel_thread(mxc_wdt_kick_thread, NULL, 0);
	if (wdog_reset_status & 0x2) {
		printk("Warning: This power up is because Watchdog timeout reset!\n");
	}
	return 0;
}

arch_initcall(mxc_wdt_kick_thread_init);
#endif

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
	u32 ocr2;

	BUG_ON(ref_cycles < 0);

	/*
	 * Get offset from next jiffy
	 * NOTE: GPTOCR1 is always the GPT match value for jiffies+1
	 */
	temp_cycles = (ref_jiffies - jiffies) * arch_cycles_per_jiffy +
	    ref_cycles;

	if (unlikely(temp_cycles <= 0))
		return -ETIME;

#ifdef CONFIG_MOT_WFN482
	ocr2 = __raw_readl(MXC_GPT_GPTOCR1) - arch_cycles_per_jiffy + temp_cycles;
#else
	ocr2 = __raw_readl(MXC_GPT_GPTICR1) + temp_cycles;
	if (unlikely(ocr2 <= __raw_readl(MXC_GPT_GPTCNT)))
		return -ETIME;
#endif /* CONFIG_MOT_WFN482 */

	__raw_writel(ocr2, MXC_GPT_GPTOCR2);
	__raw_writel(__raw_readl(MXC_GPT_GPTIR)| GPTIR_OF2IE, MXC_GPT_GPTIR);

#ifdef CONFIG_MOT_WFN482
	if (unlikely((signed)
			(__raw_readl(MXC_GPT_GPTOCR2) -
			 __raw_readl(MXC_GPT_GPTCNT)) <= 0)) {
		__raw_writel(__raw_readl(MXC_GPT_GPTIR) & ~GPTIR_OF2IE, MXC_GPT_GPTIR);
		return -ETIME;
    }
#endif /* CONFIG_MOT_WFN482 */

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
