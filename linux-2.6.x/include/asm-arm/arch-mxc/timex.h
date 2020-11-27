/*
 *  Copyright (C) 1999 ARM Limited
 *  Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
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
 */

#ifndef __ASM_ARM_ARCH_MXC_TIMEX_H__
#define __ASM_ARM_ARCH_MXC_TIMEX_H__

#include <asm/arch/hardware.h>
#include <asm/io.h>

/*!
 * @defgroup Timers RTC, OS tick, Watchdog Timers and High Resolution Timer
 * @ingroup MSL
 */

/*!
 * @file timex.h
 * @brief This file contains the macro for clock tick rate needed by other
 * kernel functions.
 *
 * @ingroup Timers
 */

/*
 * TODO: Need to find out the actual timer tick rate
 */
/*!
 * defines the OS clock tick rate
 */
#define CLOCK_TICK_RATE		(MXC_TIMER_CLK / MXC_TIMER_DIVIDER)

/*!
 * REVISIT: Document me
 */
static inline cycles_t get_cycles(void)
{
	return __raw_readl(MXC_GPT_GPTCNT);
}

#endif				/* __ASM_ARM_ARCH_MXC_TIMEX_H__ */
