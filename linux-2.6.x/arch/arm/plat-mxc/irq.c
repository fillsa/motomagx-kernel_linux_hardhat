/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2005 MontaVista, Inc. All Rights Reserved.
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
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>

/*
 *****************************************
 * EDIO Registers                        *
 *****************************************
 */
#ifdef EDIO_BASE_ADDR

static const int mxc_edio_irq_map[] = {
	INT_EXT_INT0,
	INT_EXT_INT1,
	INT_EXT_INT2,
	INT_EXT_INT3,
	INT_EXT_INT4,
	INT_EXT_INT5,
	INT_EXT_INT6,
	INT_EXT_INT7,
};

static u32 edio_irq_type[MXC_MAX_EXT_LINES] = {
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
	IRQT_LOW,
};

static int irq_to_edio(int irq)
{
	int i;
	int edio = -1;

	for (i = 0; i < MXC_MAX_EXT_LINES; i++) {
		if (mxc_edio_irq_map[i] == irq) {
			edio = i;
			break;
		}
	}
	return edio;
}

static void mxc_irq_set_edio_level(int irq, int trigger)
{
	int edio;
	unsigned short rval;

	edio = irq_to_edio(irq);
	rval = __raw_readw(EDIO_EPPAR);
	rval = (rval & (~(0x3 << (edio * 2)))) | (trigger << (edio * 2));
	__raw_writew(rval, EDIO_EPPAR);
}

static void mxc_irq_set_edio_dir(int irq, int dir)
{
	int edio;
	unsigned short rval;

	edio = irq_to_edio(irq);

	rval = __raw_readw(EDIO_EPDR);
	rval &= ~(1 << edio);
	rval |= (0 << edio);
	__raw_writew(rval, (EDIO_EPDR));

	/* set direction */
	rval = __raw_readw(EDIO_EPDDR);

	if (dir)
		/* out */
		rval |= (1 << edio);
	else
		/* in */
		rval &= ~(1 << edio);

	__raw_writew(rval, EDIO_EPDDR);

}

/*
 * Allows tuning the IRQ type , trigger and priority
 */
static void mxc_irq_set_edio(int irq, int fiq, int priority, int trigger)
{

	mxc_irq_set_edio_dir(irq, 0);
	/* set level */
	mxc_irq_set_edio_level(irq, trigger);
}
#endif

/*!
 * Disable interrupt number "irq" in the AVIC
 *
 * @param  irq          interrupt source number
 */
static void mxc_mask_irq(unsigned int irq)
{
	__raw_writel(irq, AVIC_INTDISNUM);
}

/*!
 * Enable interrupt number "irq" in the AVIC
 *
 * @param  irq          interrupt source number
 */
static void mxc_unmask_irq(unsigned int irq)
{
	__raw_writel(irq, AVIC_INTENNUM);
}

static struct irqchip mxc_avic_chip = {
	.ack = mxc_mask_irq,
	.mask = mxc_mask_irq,
	.unmask = mxc_unmask_irq,
};

#ifdef EDIO_BASE_ADDR
static void mxc_ack_edio(u32 irq)
{
	u16 edio = (u16) irq_to_edio(irq);
	if (edio_irq_type[edio] == IRQT_LOW) {
		/* Mask interrupt for level sensitive */
		mxc_mask_irq(irq);
	} else if (edio_irq_type[edio] == IRQT_HIGH) {
		/* clear edio interrupt */
		__raw_writew((1 << edio), EDIO_EPFR);
		/* dummy read for edio workaround */
		__raw_readw(EDIO_EPFR);
		/* Mask interrupt for level sensitive */
		mxc_mask_irq(irq);
	} else {
		/* clear edio interrupt */
		__raw_writew((1 << edio), EDIO_EPFR);
		/* dummy read for edio workaround */
		__raw_readw(EDIO_EPFR);
	}
}

static int mxc_edio_set_type(u32 irq, u32 type)
{
	edio_irq_type[irq_to_edio(irq)] = type;

	switch (type) {
	case IRQT_RISING:
		mxc_irq_set_edio_level(irq, 1);
		set_irq_handler(irq, do_edge_IRQ);
		break;
	case IRQT_FALLING:
		mxc_irq_set_edio_level(irq, 2);
		set_irq_handler(irq, do_edge_IRQ);
		break;
	case IRQT_BOTHEDGE:
		mxc_irq_set_edio_level(irq, 3);
		set_irq_handler(irq, do_edge_IRQ);
		break;
	case IRQT_LOW:
		mxc_irq_set_edio_level(irq, 0);
		set_irq_handler(irq, do_level_IRQ);
		break;
	case IRQT_HIGH:
		/* EDIO doesn't really support high-level interrupts,
		 * so we're faking it
		 */
		mxc_irq_set_edio_level(irq, 1);
		set_irq_handler(irq, do_level_IRQ);
		break;
	default:
		return -EINVAL;
		break;
	}
	return 0;
}

static struct irqchip mxc_edio_chip = {
	.ack = mxc_ack_edio,
	.mask = mxc_mask_irq,
	.unmask = mxc_unmask_irq,
	.type = mxc_edio_set_type,
};

#endif

/*!
 * This function initializes the AVIC hardware and disables all the
 * interrupts. It registers the interrupt enable and disable functions
 * to the kernel for each interrupt source.
 */
void __init mxc_init_irq(void)
{
	int i;
	u32 reg;

	/* put the AVIC into the reset value with
	 * all interrupts disabled
	 */
	__raw_writel(0, AVIC_INTCNTL);
	__raw_writel(0x1f, AVIC_NIMASK);

	/* disable all interrupts */
	__raw_writel(0, AVIC_INTENABLEH);
	__raw_writel(0, AVIC_INTENABLEL);

	/* all IRQ no FIQ */
	__raw_writel(0, AVIC_INTTYPEH);
	__raw_writel(0, AVIC_INTTYPEL);
#ifdef CONFIG_MOT_WFN453
	for (i = 0; i <= MXC_MAX_INT_LINES; i++) {
#else
	for (i = 0; i < MXC_MAX_INT_LINES; i++) {
#endif
#ifdef EDIO_BASE_ADDR
		if (irq_to_edio(i) != -1) {
			mxc_irq_set_edio(i, 0, 0, 0);
			set_irq_chip(i, &mxc_edio_chip);
		} else
#endif
		{
			set_irq_chip(i, &mxc_avic_chip);
		}
		set_irq_handler(i, do_level_IRQ);
		set_irq_flags(i, IRQF_VALID);
	}

	/* Set WDOG2's interrupt the highest priority level (bit 28-31) */
	reg = __raw_readl(AVIC_NIPRIORITY6);
	reg |= (0xF << 28);
	__raw_writel(reg, AVIC_NIPRIORITY6);

	printk(KERN_INFO "MXC IRQ initialized\n");
}
