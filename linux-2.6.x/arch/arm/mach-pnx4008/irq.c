/*
 * arch/arm/mach-pnx4008/irq.c
 *
 * PNX4008 IRQ controller driver
 *
 * Author: Dmitry Chigirev <source@mvista.com>
 *
 * Based on reference code received from Philips:
 * Copyright (C) 2003 Philips Semiconductors
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/system.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/arch/irq.h>

/* Manual: Chapter 20, page 195 */

static u8 pnx4008_iqr_type[NR_IRQS] = PNX4008_IRQ_TYPES;

#define INTC_BIT(irq) (1<<((irq)&0x1F))

#define INTC_ER(irq)    IO_ADDRESS((PNX4008_INTCTRLMIC_BASE + 0x0 + (((irq)&(0x3<<5))<<9)))
#define INTC_RSR(irq)   IO_ADDRESS((PNX4008_INTCTRLMIC_BASE + 0x4 + (((irq)&(0x3<<5))<<9)))
#define INTC_SR(irq)    IO_ADDRESS((PNX4008_INTCTRLMIC_BASE + 0x8 + (((irq)&(0x3<<5))<<9)))
#define INTC_APR(irq)   IO_ADDRESS((PNX4008_INTCTRLMIC_BASE + 0xC + (((irq)&(0x3<<5))<<9)))
#define INTC_ATR(irq)   IO_ADDRESS((PNX4008_INTCTRLMIC_BASE + 0x10 + (((irq)&(0x3<<5))<<9)))
#define INTC_ITR(irq)   IO_ADDRESS((PNX4008_INTCTRLMIC_BASE + 0x14 + (((irq)&(0x3<<5))<<9)))

static void pnx4008_mask_irq(unsigned int irq)
{
	__raw_writel(__raw_readl(INTC_ER(irq)) & ~INTC_BIT(irq), INTC_ER(irq));	/* mask interrupt */
}

static void pnx4008_unmask_irq(unsigned int irq)
{
	__raw_writel(__raw_readl(INTC_ER(irq)) | INTC_BIT(irq), INTC_ER(irq));	/* unmask interrupt */
}

static void pnx4008_mask_ack_irq(unsigned int irq)
{
	__raw_writel(__raw_readl(INTC_ER(irq)) & ~INTC_BIT(irq), INTC_ER(irq));	/* mask interrupt */
	__raw_writel(INTC_BIT(irq), INTC_SR(irq));	/* clear interrupt status */
}

static int pnx4008_set_irq_type(unsigned int irq, unsigned int type)
{
	switch (type) {
	case IRQT_RISING:
		__raw_writel(__raw_readl(INTC_ATR(irq)) | INTC_BIT(irq), INTC_ATR(irq));	/*edge sensitive */
		__raw_writel(__raw_readl(INTC_APR(irq)) | INTC_BIT(irq), INTC_APR(irq));	/*rising edge */
		set_irq_handler(irq, do_edge_IRQ);
		break;
	case IRQT_FALLING:
		__raw_writel(__raw_readl(INTC_ATR(irq)) | INTC_BIT(irq), INTC_ATR(irq));	/*edge sensitive */
		__raw_writel(__raw_readl(INTC_APR(irq)) & ~INTC_BIT(irq), INTC_APR(irq));	/*falling edge */
		set_irq_handler(irq, do_edge_IRQ);
		break;
	case IRQT_LOW:
		__raw_writel(__raw_readl(INTC_ATR(irq)) & ~INTC_BIT(irq), INTC_ATR(irq));	/*level sensitive */
		__raw_writel(__raw_readl(INTC_APR(irq)) & ~INTC_BIT(irq), INTC_APR(irq));	/*low level */
		set_irq_handler(irq, do_level_IRQ);
		break;
	case IRQT_HIGH:
		__raw_writel(__raw_readl(INTC_ATR(irq)) & ~INTC_BIT(irq), INTC_ATR(irq));	/*level sensitive */
		__raw_writel(__raw_readl(INTC_APR(irq)) | INTC_BIT(irq), INTC_APR(irq));	/* high level */
		set_irq_handler(irq, do_level_IRQ);
		break;
	default:
		printk(KERN_ERR "PNX4008 IRQ: Unsupported irq type %d\n", type);
		return -1;
	}
	return 0;
}

static struct irqchip pnx4008_irq_chip = {
	.ack = pnx4008_mask_ack_irq,
	.mask = pnx4008_mask_irq,
	.unmask = pnx4008_unmask_irq,
	.type = pnx4008_set_irq_type,
};

void __init PNX4008_init_irq(void)
{
	unsigned int i;

	/* configure and enable IRQ 0,1,30,31 (cascade interrupts) mask all others */
	pnx4008_set_irq_type(0, pnx4008_iqr_type[0]);
	pnx4008_set_irq_type(1, pnx4008_iqr_type[1]);
	pnx4008_set_irq_type(30, pnx4008_iqr_type[30]);
	pnx4008_set_irq_type(31, pnx4008_iqr_type[31]);

	__raw_writel((1 << 31) | (1 << 30) | (1 << 1) | 1, INTC_ER(0));
	__raw_writel(0, INTC_ER(32));
	__raw_writel(0, INTC_ER(64));

	/* configure all other IRQ's */
	for (i = 2; i < NR_IRQS; i++) {
		if (i == 30)
			i += 2;	/*skip configuring IRQ 0,1,30,31 */
		set_irq_flags(i, IRQF_VALID);
		set_irq_chip(i, &pnx4008_irq_chip);
		pnx4008_set_irq_type(i, pnx4008_iqr_type[i]);
	}
}
