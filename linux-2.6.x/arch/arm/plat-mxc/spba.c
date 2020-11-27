/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
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

#include <linux/config.h>
#include <linux/types.h>
#include <linux/module.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/spba.h>

/*!
 * @file spba.c
 *
 * @brief This file contains the SPBA API implementation details.
 *
 * @ingroup SPBA
 */

static DEFINE_RAW_SPINLOCK(spba_lock);

#define SPBA_MASTER_MIN                 1
#define SPBA_MASTER_MAX                 7

/*!
 * the base addresses for the SPBA modules
 */
static unsigned long spba_base = (unsigned long)IO_ADDRESS(SPBA_CTRL_BASE_ADDR);

/*!
 * This function allows the three masters (A, B, C) to take ownership of a
 * shared peripheral.
 *
 * @param  mod          specified module as defined in \b enum \b #spba_module
 * @param  master       one of more (or-ed together) masters as defined in \b enum \b #spba_masters
 *
 * @return 0 if successful; -1 otherwise.
 */
int spba_take_ownership(int mod, int master)
{
	__u32 spba_flags;
	__u32 rtn_val = -1;

	if (master < SPBA_MASTER_MIN || master > SPBA_MASTER_MAX) {
		printk("spba_take_ownership() invalide master= %d\n", master);
		BUG();		/* oops */
	}

	spin_lock_irqsave(&spba_lock, spba_flags);
	__raw_writel(master, spba_base + mod);

	if ((__raw_readl(spba_base + mod) & MXC_SPBA_RAR_MASK) == master) {
		rtn_val = 0;
	}

	spin_unlock_irqrestore(&spba_lock, spba_flags);

	return rtn_val;
}

/*!
 * This function releases the ownership for a shared peripheral.
 *
 * @param  mod          specified module as defined in \b enum \b #spba_module
 * @param  master       one of more (or-ed together) masters as defined in \b enum \b #spba_masters
 *
 * @return 0 if successful; -1 otherwise.
 */
int spba_rel_ownership(int mod, int master)
{
	__u32 spba_flags;
	volatile unsigned long rar;

	if (master < SPBA_MASTER_MIN || master > SPBA_MASTER_MAX) {
		printk("spba_take_ownership() invalide master= %d\n", master);
		BUG();		/* oops */
	}

	if ((__raw_readl(spba_base + mod) & master) == 0) {
		return 0;	/* does not own it */
	}

	spin_lock_irqsave(&spba_lock, spba_flags);

	/* Since only the last 3 bits are writeable, doesn't need to mask off
	   bits 31-3 */
	rar = __raw_readl(spba_base + mod) & (~master);
	__raw_writel(rar, spba_base + mod);

	if ((__raw_readl(spba_base + mod) & master) != 0) {
		spin_unlock_irqrestore(&spba_lock, spba_flags);
		return -1;
	}

	spin_unlock_irqrestore(&spba_lock, spba_flags);
	return 0;
}

EXPORT_SYMBOL(spba_take_ownership);
EXPORT_SYMBOL(spba_rel_ownership);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("SPBA");
MODULE_LICENSE("GPL");
