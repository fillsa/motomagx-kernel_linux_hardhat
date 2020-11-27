/*
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
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

#define CRM_CONTROL                     IO_ADDRESS(DSM_BASE_ADDR + 0x50)
#define LPMD1                           0x00000002
#define LPMD0                           0x00000001

/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
void arch_idle(void)
{
	unsigned long crm_ctrl;

	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks.
	 */
	if ((__raw_readl(AVIC_VECTOR) & MXC_WFI_ENABLE) != 0) {
		crm_ctrl = (__raw_readl(CRM_CONTROL) & ~(LPMD1)) | (LPMD0);
		__raw_writel(crm_ctrl, CRM_CONTROL);
		cpu_do_idle();
	}
}

/*
 * This function resets the system. It is called by machine_restart().
 *
 * @param  mode         indicates different kinds of resets
 */
void arch_reset(char mode)
{
	/* Workaround reset problem by manually reset WEIM */
	__raw_writel(0x00001E00, IO_ADDRESS(WEIM_CTRL_CS0 + CSCRU));
	__raw_writel(0x20000D01, IO_ADDRESS(WEIM_CTRL_CS0 + CSCRL));
	__raw_writel(0x0, IO_ADDRESS(WEIM_CTRL_CS0 + CSCRA));
	if ((__raw_readw(IO_ADDRESS(WDOG_BASE_ADDR)) & 0x4) != 0) {
		asm("cpsid iaf");
		while (1) {
		}
	} else {
		__raw_writew(__raw_readw(IO_ADDRESS(WDOG_BASE_ADDR)) | 0x4,
			     IO_ADDRESS(WDOG_BASE_ADDR));
	}
}
