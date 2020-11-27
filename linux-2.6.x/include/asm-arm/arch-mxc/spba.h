/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
#ifndef __ASM_ARM_ARCH_MXC_SPBA_H__
#define __ASM_ARM_ARCH_MXC_SPBA_H__

#ifdef __KERNEL__

/*!
 * @defgroup SPBA Shared Peripheral Bus Arbiter (SPBA)
 * @ingroup MSL
 */

/*!
 * @file spba.h
 * @brief This file contains the Shared Peripheral Bus Arbiter (spba) API.
 *
 * @ingroup SPBA
 */
#define MXC_SPBA_RAR_MASK       0x7

/*!
 * Defines three SPBA masters: A - ARM, C - SDMA (no master B for MX31)
 */
enum spba_masters {
	SPBA_MASTER_A = 1,
	SPBA_MASTER_B = 2,
	SPBA_MASTER_C = 4,
};

/*!
 * This function allows the three masters (A, B, C) to take ownership of a
 * shared peripheral.
 *
 * @param  mod          specified module as defined in \b enum \b #spba_module
 * @param  master       one of more (or-ed together) masters as defined in \b enum \b #spba_masters
 *
 * @return 0 if successful; -1 otherwise.
 */
int spba_take_ownership(int mod, int master);

/*!
 * This function releases the ownership for a shared peripheral.
 *
 * @param  mod          specified module as defined in \b enum \b #spba_module
 * @param  master       one of more (or-ed together) masters as defined in \b enum \b #spba_masters
 *
 * @return 0 if successful; -1 otherwise.
 */
int spba_rel_ownership(int mod, int master);

#endif				/* __KERNEL__ */

#endif				/* __ASM_ARM_ARCH_MXC_SPBA_H__ */
