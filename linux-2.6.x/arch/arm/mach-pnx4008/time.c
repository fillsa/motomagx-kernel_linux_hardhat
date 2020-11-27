/*
 * arch/arm/mach-pnx4008/time.c
 *
 * PNX4008 Timers: KFI, HRT, System
 *
 * Authors: Vitaly Wool, Dmitry Chigirev, Grigory Tolstolytkin <source@mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/kallsyms.h>

#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/leds.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>

#include <linux/time.h>
#include <linux/hrtime.h>
#include <linux/timex.h>
#include <asm/errno.h>

/*! Note: all timers are UPCOUNTING */

#ifdef CONFIG_KFI

/* os timer state flag */
static int pnx_timer_initialized = 0;

unsigned long __noinstrument machinecycles_to_usecs(unsigned long ticks)
{
	return TICKS2USECS(ticks);
}

unsigned long __noinstrument do_getmachinecycles(void)
{
	return (pnx_timer_initialized ? __raw_readl(HSTIM_COUNTER) : 0);
}
#endif				/* CONFIG_KFI */

/*!
 * Returns number of us since last clock interrupt.  Note that interrupts
 * will have been disabled by do_gettimeoffset()
 */
static unsigned long pnx4008_gettimeoffset(void)
{
	u32 ticks_to_match =
	    __raw_readl(HSTIM_MATCH0) - __raw_readl(HSTIM_COUNTER);
	u32 elapsed = LATCH - ticks_to_match;
	return (elapsed * (tick_nsec / 1000)) / LATCH;
}

/*!
 * IRQ handler for the timer
 */
static irqreturn_t pnx4008_timer_interrupt(int irq, void *dev_id,
					   struct pt_regs *regs)
{
#ifdef CONFIG_HIGH_RES_TIMERS
	if (__raw_readl(HSTIM_INT) & MATCH1_INT) {
		__raw_writel(__raw_readl(HSTIM_MCTRL) & ~MR1_INT, HSTIM_MCTRL);
		__raw_writel(MATCH1_INT, HSTIM_INT);

		do_hr_timer_int();
	}
#endif				/* CONFIG_HIGH_RES_TIMERS */

#ifdef CONFIG_FRD
	if (__raw_readl(HSTIM_INT) & MATCH1_INT)
		return IRQ_NONE;
#endif
	if (__raw_readl(HSTIM_INT) & MATCH0_INT) {

		write_seqlock(&xtime_lock);

		do {
			timer_tick(regs);

			/*
			 * this algorithm takes care of possible delay
			 * for this interrupt handling longer than a normal
			 * timer period
			 */
			__raw_writel(__raw_readl(HSTIM_MATCH0) + LATCH,
				     HSTIM_MATCH0);
			__raw_writel(MATCH0_INT, HSTIM_INT);	/* clear interrupt */

			/*
			 * The goal is to keep incrementing HSTIM_MATCH0
			 * register until HSTIM_MATCH0 indicates time after
			 * what HSTIM_COUNTER indicates.
			 */
		} while ((signed)
			 (__raw_readl(HSTIM_MATCH0) -
			  __raw_readl(HSTIM_COUNTER)) < 0);

		write_sequnlock(&xtime_lock);
	}

	return IRQ_HANDLED;
}

static struct irqaction pnx4008_timer_irq = {
	.name = "Kernel Timer",
	.flags =
#ifndef CONFIG_FRD
	    SA_NODELAY | SA_INTERRUPT,
#else
	    SA_NODELAY | SA_INTERRUPT | SA_SHIRQ,
#endif
	.handler = pnx4008_timer_interrupt
};

/*!
 * Set up timer and timer interrupt.
 */
static __init void pnx4008_setup_timer(void)
{
	__raw_writel(RESET_COUNT, MSTIM_CTRL);
	while (__raw_readl(MSTIM_COUNTER)) ;	/* wait for reset to complete. 100% guarantee event */
	__raw_writel(0, MSTIM_CTRL);	/* stop the timer */
	__raw_writel(0, MSTIM_MCTRL);

	__raw_writel(RESET_COUNT, HSTIM_CTRL);
	while (__raw_readl(HSTIM_COUNTER)) ;	/* wait for reset to complete. 100% guarantee event */
	__raw_writel(0, HSTIM_CTRL);
	__raw_writel(0, HSTIM_MCTRL);
	__raw_writel(0, HSTIM_CCR);
	__raw_writel(12, HSTIM_PMATCH);	/* scale down to 1 MHZ */
	__raw_writel(LATCH, HSTIM_MATCH0);
	__raw_writel(MR0_INT, HSTIM_MCTRL);

	setup_irq(HSTIMER_INT, &pnx4008_timer_irq);

	__raw_writel(COUNT_ENAB | DEBUG_EN, HSTIM_CTRL);	/*start timer, stop when JTAG active */
#ifdef CONFIG_KFI
	pnx_timer_initialized = 1;
#endif
}

/* Timer Clock Control in PM register */
#define TIMCLK_CTRL_REG  IO_ADDRESS((PNX4008_PWRMAN_BASE + 0xBC))
#define WATCHDOG_CLK_EN                   1
#define TIMER_CLK_EN                      2	/* HS and MS timers? */

static u32 timclk_ctrl_reg_save;

void pnx4008_timer_suspend(void)
{
	timclk_ctrl_reg_save = __raw_readl(TIMCLK_CTRL_REG);
	__raw_writel(0, TIMCLK_CTRL_REG);	/* disable timers */
}

void pnx4008_timer_resume(void)
{
	__raw_writel(timclk_ctrl_reg_save, TIMCLK_CTRL_REG);	/* enable timers */
}

struct sys_timer PNX4008_timer = {
	.init = pnx4008_setup_timer,
	.offset = pnx4008_gettimeoffset,
	.suspend = pnx4008_timer_suspend,
	.resume = pnx4008_timer_resume,
};

#ifdef CONFIG_HIGH_RES_TIMERS

/*!
 * This function schedules the next hr clock interrupt at the time specified
 * by ref_jiffies and ref_cycles.
 *
 *      If a previously scheduled interrupt has not happened yet, it is
 *      discarded and replaced by the new one.  (In practice, this will
 *      happen when a new timer is entered prior to the one last asked
 *      for.)
 *
 *      Arch can assume that
 *      	0 <= hr_clock_cycles <= arch_cycles_per_jiffy
 *      	-10 <= (jiffies - ref_jiffies) <= 10 jiffies
 *
 *      Locking: This function is called under the spin_lock_irq for the
 *
 * @param  ref_jiffies  jiffy (Hz) boundary during which to schedule subHz
 *              	interrupt
 * @param  ref_cycles   number of system timer clock ticks to delay from
 *              	jiffy (Hz) boundary
 *
 * @return non-zero (-ETIME) if the specified time is already passed; 0 on
 * 	success
 */
int schedule_hr_timer_int(unsigned ref_jiffies, int ref_cycles)
{
	int temp_cycles;

	BUG_ON(ref_cycles < 0);

	temp_cycles = (ref_jiffies - jiffies) * arch_cycles_per_jiffy +
	    ref_cycles;

	if (unlikely(temp_cycles <= 0))
		return -ETIME;

	__raw_writel(__raw_readl(HSTIM_COUNTER) + temp_cycles, HSTIM_MATCH1);
	__raw_writel(__raw_readl(HSTIM_MCTRL) | MR1_INT, HSTIM_MCTRL);

	return 0;
}

/*!
 * This function returns time elapsed from the reference jiffies to present
 * in the units of hr_clock_cycles.
 *
 *      Arch code can assume
 *      	0 <= (jiffies - ref_jiffies) <= 10 jiffies
 *      	ref_jiffies may not increase monotonically.
 *
 *      Locking: The caller will provide locking equivalent to read_lock
 *      on the xtime_lock.  The callee need only lock hardware if needed.
 *
 * @param  ref_jiffies  reference jiffies
 *
 * @return number of hr_clock_cycles elapsed from the reference jiffies to
 * 	present.
 */
int get_arch_cycles(unsigned ref_jiffies)
{
	int ret;
	unsigned temp_jiffies;
	unsigned diff_jiffies;

	do {
		temp_jiffies = jiffies;
		barrier();

		ret = __raw_readl(HSTIM_MATCH0) - __raw_readl(HSTIM_COUNTER);
		ret = arch_cycles_per_jiffy - ret;

		if (unlikely(diff_jiffies = jiffies - ref_jiffies))
			ret += diff_jiffies * arch_cycles_per_jiffy;

		barrier();
	} while (unlikely(temp_jiffies != jiffies));

	return ret;
}

#endif
