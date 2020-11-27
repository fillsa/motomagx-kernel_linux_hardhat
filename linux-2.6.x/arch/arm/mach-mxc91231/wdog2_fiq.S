/*
 * Copyright Motorola 2007
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
 * ----------   --------  --------------------
 * 01/26/2007   Motorola  Initial version. Watchdog debug support
 *
 */

#include <linux/linkage.h>
#include <asm/assembler.h>

	.text

	.global wdog2_fiq_handler_end

ENTRY(wdog2_fiq_handler_start)
	mov	r10, #0x800000		@ The bit for INT_WDOG2
	str	r10, [r8, #0x10]	@ AVIC_INTENABLEH, only INT_WDOG2
	mov	r10, #0			@ r8 was set to AVIC_BASE in time.c
	str	r10, [r8, #0x18]	@ AVIC_INTTYPEH, all are normal int
	mrs	r10, spsr		@ Read SPSR
	bic	r10, r10, #0xC0		@ enable I and F bits
	msr	spsr_c, r10		@ Save SPSR
	subs	pc, lr, #4		@ Return to the prev mode. SPSR will
					@ be used when we return.
wdog2_fiq_handler_end:

