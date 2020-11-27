/*
 * PXA27x standby mode
 *
 * Author: David Burrage
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/hardware.h>

#include <asm/arch/pxa-regs.h>

		.text

ENTRY(pxa_cpu_standby)
	stmfd	sp!, {r1 - r9, lr}		@ save registers on stack
	mov r1,     #0x18
	mov r2,     #2
	ldr	r3, 	=PSSR
	mov	r7, 	#UNCACHED_PHYS_0	@ Read mem context in.
	ldr	r8, 	[r7]

	b   1f

	.align  5
1:
	mcr p14, 0, r2, c7, c0, 0	@ put the system into Standby

	@ Since after wakeup the bit will set automatically, some times
	@ we need to clear them.
	str r1, [r3]

	ldmfd	sp!, {r1 - r9, pc}	@ Restore regs and return to caller
