/*
 * Copyright Motorola 2006
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  -------------------
 * 10/06/2006   Motorola  Support for lmbench
 *
 */


#include <linux/linkage.h>
#include <asm/assembler.h>
/*
 * Function: v6_early_abort
 *
 * Params  : r2 = address of aborted instruction
 *         : r3 = saved SPSR
 *
 * Returns : r0 = address of abort
 *	   : r1 = FSR, bit 11 = write
 *	   : r2-r8 = corrupted
 *	   : r9 = preserved
 *	   : sp = pointer to registers
 *
 * Purpose : obtain information about current aborted instruction.
 */
	.align	5
ENTRY(v6_early_abort)
	mrc	p15, 0, r1, c5, c0, 0		@ get FSR
	mrc	p15, 0, r0, c6, c0, 0		@ get FAR
/*
 * Faulty SWP instruction on 1136 doesn't set bit 11 in DFSR.
 * Set bit 11 in the FSR if we find a SWP at the faulting instruction.
 * Leave FSR alone for all other ARM instructions.
 */
	tst	r3, #PSR_J_BIT			@ Java?  If so:
	bicne	r1, r1, #1 << 11 | 1 << 10	@ clear bits 11 and 10 of FSR
	tsteq	r3, #PSR_T_BIT			@ Thumb?
	movne	pc, lr				@ Done if Java or Thumb
/*
 * Check for an ARM SWP or SWPB instruction: (instr & 0x0FB000F0) == 0x01000090
 */
	ldr	r3, [r2]			@ read aborted ARM instruction
	bic     r3, r3, #0xf000000f
	bic	r3, r3, #0x004f0000
	mov	r2, #0x1000000
	bic	r3, r3, #0x0000ff00
	add	r2, r2, #0x90
	cmp	r3, r2
	orreq	r1, r1, #1 << 11		@ SWP/SWPB, so set write bit.
	mov	pc, lr
