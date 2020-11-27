/*
 *  include/asm-arm/arch-pnx4008/frd.h
 *
 * Author: Dmitry Pervushin <dpervushin@ru.mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __PNX4008_FRD_H
#define __PNX4008_FRD_H

#define FRD_4MS 4615

#define FRD_TIMER_IRQ		ARCH_TIMER_IRQ

#ifndef CONFIG_FRD_USE_TIMER_IRQ
#define FRD_TIMER_IRQ_ACK	frd_pnx4008_irq_ack()
#define FRD_TIMER_LATCH		frd_pnx4008_timer_latch()
#define FRD_TIMER_START	frd_pnx4008_timer_start();
#define FRD_CHECK_SHARED_INTERRUPT   do { if(!(__raw_readl(HSTIM_INT) & MATCH2_INT)) return IRQ_NONE; } while (0)
#define FRD_IRQ_FLAGS 		SA_SHIRQ

static inline void frd_pnx4008_irq_ack(void)
{
	__raw_writel(__raw_readl(HSTIM_COUNTER) + FRD_4MS, HSTIM_MATCH2);
	__raw_writel(MATCH2_INT, HSTIM_INT);
}

static inline void frd_pnx4008_timer_start(void)
{
	__raw_writel(__raw_readl(HSTIM_COUNTER) + FRD_4MS, HSTIM_MATCH2);
	__raw_writel(__raw_readl(HSTIM_MCTRL) | MR2_INT, HSTIM_MCTRL);
	printk("FRD timer has been started.\n");
}

static inline long frd_pnx4008_timer_latch(void)
{
	/* simulate periodic timer */
	unsigned long retval = FRD_4MS + __raw_readl(HSTIM_COUNTER) - __raw_readl(HSTIM_MATCH2);
	while (retval >= FRD_4MS)
		retval -= FRD_4MS;
	return retval;
}

#endif /* CONFIG_FRD_USE_TIMER_IRQ */

/* frd default clock function using sched_clock  */
static inline unsigned long long frd_clock(void)
{
        return __raw_readl(HSTIM_COUNTER);
}

#endif

