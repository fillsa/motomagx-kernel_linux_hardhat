/*
 *  linux/include/asm-arm/fiq.h
 *
 * Support for FIQ on ARM architectures.
 * Written by Philip Blundell <philb@gnu.org>, 1998
 * Re-written by Russell King
 */
/*
 * Copyright (C) 2007 Motorola, Inc.
 *
 * Date           Author            Comment
 * ===========  ==========  ====================================
 * 10/15/2007   Motorola    FIQ related modified.
 */

#ifndef __ASM_FIQ_H
#define __ASM_FIQ_H

#include <asm/ptrace.h>

struct fiq_handler {
	struct fiq_handler *next;
	/* Name
	 */
	const char *name;
	/* Called to ask driver to relinquish/
	 * reacquire FIQ
	 * return zero to accept, or -<errno>
	 */
	int (*fiq_op)(void *, int relinquish);
	/* data for the relinquish/reacquire functions
	 */
	void *dev_id;
};

extern int claim_fiq(struct fiq_handler *f);
extern void release_fiq(struct fiq_handler *f);
extern void set_fiq_handler(void *start, unsigned int length);
extern void set_fiq_regs(struct pt_regs *regs);
extern void get_fiq_regs(struct pt_regs *regs);
extern void enable_fiq(int fiq);
extern void disable_fiq(int fiq);

#ifdef CONFIG_MOT_FEAT_FIQ_IN_C
int mxc_fiq_irq_switch(int vector, int irq2fiq);

#endif //CONFIG_MOT_FEAT_FIQ_IN_C

#endif
