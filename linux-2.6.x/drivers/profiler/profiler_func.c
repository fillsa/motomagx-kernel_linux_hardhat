/*
 * Copyright 2006 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/*
 * Revision History:
 *
 * Date        Author    Comment
 * 10/04/2006  Motorola  Initial version.
 */
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/timex.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/arch/system.h>

void arch_idle_wrapper(void)
{
    arch_idle();
}
EXPORT_SYMBOL(arch_idle_wrapper);

unsigned long get_cycles_wrapper(void)
{
    return get_cycles();	
}
EXPORT_SYMBOL(get_cycles_wrapper);
