/*
 * Copyright (C) 1999 ARM Limited
 * Copyright (C) 2000 Deep Blue Solutions Ltd
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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

#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/system.h>

/*!
 * @defgroup MSL Machine Specific Layer (MSL)
 */

/*!
 * @defgroup System System-wide Misc Files for MSL
 * @ingroup MSL
 */

/*!
 * @file system.h
 * @brief This file contains idle and reset functions.
 *
 * @ingroup System
 */

#define AVIC_NIPNDH             IO_ADDRESS(AVIC_BASE_ADDR + 0x58)
#define AVIC_NIPNDL             IO_ADDRESS(AVIC_BASE_ADDR + 0x5c)
#define LO_INTR_EN              IO_ADDRESS(AVIC_BASE_ADDR + 0x14)

/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks.
	 */
	if ((__raw_readl(AVIC_VECTOR) & MXC_WFI_ENABLE) != 0) {
		__raw_writel((__raw_readl(AVIC_VECTOR) | 0x00000004),
			     AVIC_VECTOR);
		cpu_do_idle();

		if ((__raw_readl(AVIC_NIPNDH)) || (__raw_readl(AVIC_NIPNDL))) {
			if ((__raw_readl(LO_INTR_EN) & 0x38000000) == 0) {
				/* Enable timer interrupt */
				__raw_writel((__raw_readl(LO_INTR_EN) |
					      0x38000000), LO_INTR_EN);
			}
			/* Enable Interrupts */
			local_irq_enable();
			if (__raw_readl(AVIC_VECTOR) == 0x0000000c) {
				cpu_do_idle();
			}
		}
	}
}

/*
 * This function resets the system. It is called by machine_restart().
 *
 * @param  mode         indicates different kinds of resets
 */
void arch_reset(char mode)
{
	if ((__raw_readw(IO_ADDRESS(WDOG_BASE_ADDR)) & 0x4) != 0) {
		/* If WDOG enabled, wait till it's timed out */
		asm("cpsid iaf");
		while (1) {
		}
	} else {
		__raw_writew(__raw_readw(IO_ADDRESS(WDOG_BASE_ADDR)) | 0x4,
			     IO_ADDRESS(WDOG_BASE_ADDR));
	}
}
