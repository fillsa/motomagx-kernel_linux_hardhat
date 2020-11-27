/*
 *  include/asm-arm/arch-mxc/frd.h
 *
 * Initially based on include/asm-arm/arch-omap/frd.h
 * Author: Sven Thorsten Dietrich <sdietrich@mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef _ASM_ARCH_FRD_H
#define _ASM_ARCH_FRD_H

/* !
 * @defgroup FRD Preemption Latency Timing
 * @ingroup MSL
 */

#include <linux/config.h>
#include <asm/arch/preempt.h>
#include <asm/io.h>
#include <asm/arch/timex.h>

#define FRD_TIMER_IRQ		ARCH_TIMER_IRQ
#define FRD_TIMER_LATCH		__raw_readl(MXC_GPT_GPTCNT)
#define FRD_TIMER_START		do { } while(0)
#define FRD_TIMER_INIT		__raw_readl(MXC_GPT_GPTOCR1)
#define FRD_SCALE_ABS_TICKS

static inline unsigned long long frd_clock(void)
{
	return __raw_readl(MXC_GPT_GPTCNT);
}

#endif
