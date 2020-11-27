/*
 *  linux/arch/arm/kernel/boardrev.c
 *
 *           Copyright Motorola 2005-2006
 *
 * This file implements a capability for checking board revisions
 * in the kernel/drivers.
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
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ------------------------
 * 08/10/2005   Motorola  Initial revision.
 * 03/17/2006   Motorola  Exported boardrev symbol
 *
 */

#include <linux/config.h>

#if defined(CONFIG_MOT_FEAT_BRDREV)

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>

#include <asm/setup.h>

/* __ARCH_ARM_BOARDREV_C is used in asm/boardrev.h. */
#define __ARCH_ARM_BOARDREV_C
#include <asm/boardrev.h>

static u32 brdrev = BOARDREV_UNKNOWN;

/*
 * The boardrev function is the interface for obtaining the board
 * revision.  If the board has no supported revisions, then boardrev
 * returns the default value -- BOARDREV_UNKNOWN.
 */
u32 boardrev(void)
{
	return brdrev;
}
EXPORT_SYMBOL(boardrev);

/*
 * boardrev_setup searches the brdrevs table for a match.  If one is
 * found, then brdrev is set appropriately.  If no match is found,
 * then brdrev is set to BOARDREV_UNKNOWN.
 */
static int __init boardrev_setup(char *str)
{
	struct brdrevinfo *p;

	/*
	 * Search the list for the given board revision.  If we don't
	 * find it, then the board revision is BOARDREV_UNKNOWN.
	 */
	for (p=brdrevs; p->br_name; p++) {
		/* br_namelen is large enough to check for the NUL. */
		if (!strncmp(p->br_name, str, p->br_namelen)) {
			brdrev = p->br_val;
			printk (KERN_WARNING "Board Revision detected: %s\n",
					p->br_name);
			return 1;
		}
	}

	brdrev = BOARDREV_UNKNOWN;
	printk (KERN_WARNING "Unrecognized Board Revision detected: %s\n", str);
	printk (KERN_WARNING "Setting Board Revision to BOARDREV_UNKNOWN\n");
	return 1;
}
__setup("brdrev=", boardrev_setup);

#endif /* defined(CONFIG_MOT_FEAT_BRDREV) */
