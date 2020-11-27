/*
 *  Copyright (C) 2005,2007 Motorola, Inc.
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 * 
 * Changelog:
 *
 *  Date         Author    Comment
 * ----------   --------  --------------------------
 * 02/25/2005   Motorola   Add IRQ macros and structure timer_update_handler
 * 06/28/2007   Motorola   Modify compliance issues in hardhat source files.
 * 08/01/2007   Motorola   Add comments for oss compliance.
 * 08/11/2007   Motorola   Add comments.
 */


#ifndef __ASM_ARM_IRQ_H
#define __ASM_ARM_IRQ_H

#include <asm/arch/irqs.h>

#ifndef irq_canonicalize
#define irq_canonicalize(i)	(i)
#endif

#ifndef NR_IRQS
#define NR_IRQS	128
#endif

/*
 * Use this value to indicate lack of interrupt
 * capability
 */
#ifndef NO_IRQ
#define NO_IRQ	((unsigned int)(-1))
#endif

struct irqaction;

#define IRQ_INPROGRESS  1       /* IRQ handler active - do not enter! */
#define IRQ_DISABLED    2       /* IRQ disabled - do not enter! */
#define IRQ_PENDING     4       /* IRQ pending - replay on enable */
#define IRQ_NODELAY 512     /* IRQ must run immediately */
# define SA_NODELAY 0x01000000

extern void disable_irq_nosync(unsigned int);
extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

#define __IRQT_FALEDGE	(1 << 0)
#define __IRQT_RISEDGE	(1 << 1)
#define __IRQT_LOWLVL	(1 << 2)
#define __IRQT_HIGHLVL	(1 << 3)

#define IRQT_NOEDGE	(0)
#define IRQT_RISING	(__IRQT_RISEDGE)
#define IRQT_FALLING	(__IRQT_FALEDGE)
#define IRQT_BOTHEDGE	(__IRQT_RISEDGE|__IRQT_FALEDGE)
#define IRQT_LOW	(__IRQT_LOWLVL)
#define IRQT_HIGH	(__IRQT_HIGHLVL)
#define IRQT_PROBE	(1 << 4)

int set_irq_type(unsigned int irq, unsigned int type);
void disable_irq_wake(unsigned int irq);
void enable_irq_wake(unsigned int irq);
int setup_irq(unsigned int, struct irqaction *);

struct irqaction;
struct pt_regs;
int handle_IRQ_event(unsigned int, struct pt_regs *, struct irqaction *);

struct timer_update_handler {
	int(*function)(int irq, void *dev_id, struct pt_regs *regs);
	int skip;
};

#ifdef CONFIG_RTHAL
extern struct rt_hal rthal;
#endif

#endif

