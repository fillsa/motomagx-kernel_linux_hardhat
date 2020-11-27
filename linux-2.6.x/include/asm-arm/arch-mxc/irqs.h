/*
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

#ifndef __ASM_ARM_ARCH_MXC_IRQS_H_
#define __ASM_ARM_ARCH_MXC_IRQS_H_

#include <asm/arch/hardware.h>

/*!
 * @defgroup Interrupt Interrupt Controller (AVIC)
 * @ingroup MSL
 */
/*!
 * @file irqs.h
 * @brief This file defines the number of normal interrupts and fast interrupts
 *
 * @ingroup Interrupt
 */

#define MXC_IRQ_TO_EDIO(nr)     ((nr) - MXC_EXT_BASE)
#define MXC_IRQ_IS_EDIO(nr)  	((nr) >= MXC_EXT_BASE)

#define MXC_IRQ_TO_EXPIO(irq)	(irq - MXC_EXP_IO_BASE)

#define MXC_IRQ_TO_GPIO(irq)	(irq - MXC_GPIO_BASE)
#define MXC_GPIO_TO_IRQ(x)	(MXC_GPIO_BASE + x)

/*!
 * REVISIT: document me
 */
#define ARCH_TIMER_IRQ	INT_GPT

/*!
 * Number of normal interrupts
 */
#define NR_IRQS         (MXC_MAX_INTS + 1)

/*!
 * Number of fast interrupts
 */
#define NR_FIQS		(MXC_MAX_INTS + 1)

#endif				/* __ASM_ARM_ARCH_MXC_IRQS_H_ */
