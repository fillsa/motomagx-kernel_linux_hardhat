/*
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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

#ifndef __ASM_ARM_ARCH_SYSTEM_H_
#define __ASM_ARM_ARCH_SYSTEM_H_

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

/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
extern void arch_idle(void);

/*
 * This function resets the system. It is called by machine_restart().
 *
 * @param  mode         indicates different kinds of resets
 */
extern void arch_reset(char mode);

#endif				/* __ASM_ARM_ARCH_MXC_SYSTEM_H_ */
