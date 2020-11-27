/*
 *  linux/arch/arm/kernel/fiq.c
 *
 *  Copyright (C) 1998 Russell King
 *  Copyright (C) 1998, 1999 Phil Blundell
 *  Copyright (C) 2007-2008 Motorola, Inc.
 *
 *  FIQ support written by Philip Blundell <philb@gnu.org>, 1998.
 *
 *  FIQ support re-written by Russell King to be more generic
 *
 * We now properly support a method by which the FIQ handlers can
 * be stacked onto the vector.  We still do not support sharing
 * the FIQ vector itself.
 *
 * Operation is as follows:
 *  1. Owner A claims FIQ:
 *     - default_fiq relinquishes control.
 *  2. Owner A:
 *     - inserts code.
 *     - sets any registers,
 *     - enables FIQ.
 *  3. Owner B claims FIQ:
 *     - if owner A has a relinquish function.
 *       - disable FIQs.
 *       - saves any registers.
 *       - returns zero.
 *  4. Owner B:
 *     - inserts code.
 *     - sets any registers,
 *     - enables FIQ.
 *  5. Owner B releases FIQ:
 *     - Owner A is asked to reacquire FIQ:
 *	 - inserts code.
 *	 - restores saved registers.
 *	 - enables FIQ.
 *  6. Goto 3
 *
 * Revision History:
 * Date         Author    Comment
 * ---------    --------  ---------------------------
 * 01/26/2007   Motorola  Fixed the asm clobber issue. It is fixed in the
 *                        later versions of Linux kernel.
 * 10/15/2007   Motorola  FIQ related modified.
 * 10/21/2007   Motorola  Introduce the image resolution information into FIQ handler
 * 03/11/2008   Motorola  Added WDOG2 FIQ handler to common FIQ handler
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/devfs_fs_kernel.h>

#include <asm/cacheflush.h>
#include <asm/fiq.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#if !defined(CONFIG_MOT_FEAT_DEBUG_WDOG) && !defined(CONFIG_FIQ)
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#warning This file requires GCC 3.3.x or older to build.  Alternatively,
#warning please talk to GCC people to resolve the issues with the
#warning assembly clobber list.
#endif
#endif

static unsigned long no_fiq_insn;

/* Default reacquire function
 * - we always relinquish FIQ control
 * - we always reacquire FIQ control
 */
static int fiq_def_op(void *ref, int relinquish)
{
	if (!relinquish)
		set_fiq_handler(&no_fiq_insn, sizeof(no_fiq_insn));

	return 0;
}

static struct fiq_handler default_owner = {
	.name	= "default",
	.fiq_op = fiq_def_op,
};

static struct fiq_handler *current_fiq = &default_owner;

int show_fiq_list(struct seq_file *p, void *v)
{
	if (current_fiq != &default_owner)
		seq_printf(p, "FIQ:              %s\n", current_fiq->name);

	return 0;
}

void set_fiq_handler(void *start, unsigned int length)
{
	memcpy((void *)0xffff001c, start, length);
	flush_icache_range(0xffff001c, 0xffff001c + length);
	if (!vectors_high())
		flush_icache_range(0x1c, 0x1c + length);
}

/*
 * Taking an interrupt in FIQ mode is death, so both these functions
 * disable irqs for the duration. 
 */
#if defined(CONFIG_MOT_FEAT_DEBUG_WDOG) || defined(CONFIG_FIQ)
void __attribute__((naked)) set_fiq_regs(struct pt_regs *regs)
{
        register unsigned long tmp;
        asm volatile (
        "mov    ip, sp\n\
        stmfd   sp!, {fp, ip, lr, pc}\n\
        sub     fp, ip, #4\n\
        mrs     %0, cpsr\n\
        msr     cpsr_c, %2      @ select FIQ mode\n\
        mov     r0, r0\n\
        ldmia   %1, {r8 - r14}\n\
        msr     cpsr_c, %0      @ return to SVC mode\n\
        mov     r0, r0\n\
        ldmfd   sp, {fp, sp, pc}"
        : "=&r" (tmp)
        : "r" (&regs->ARM_r8), "I" (PSR_I_BIT | PSR_F_BIT | FIQ_MODE));
}
#else
void set_fiq_regs(struct pt_regs *regs)
{
	register unsigned long tmp;
	__asm__ volatile (
	"mrs	%0, cpsr\n\
	msr	cpsr_c, %2	@ select FIQ mode\n\
	mov	r0, r0\n\
	ldmia	%1, {r8 - r14}\n\
	msr	cpsr_c, %0	@ return to SVC mode\n\
	mov	r0, r0"
	: "=&r" (tmp)
	: "r" (&regs->ARM_r8), "I" (PSR_I_BIT | PSR_F_BIT | FIQ_MODE)
	/* These registers aren't modified by the above code in a way
	   visible to the compiler, but we mark them as clobbers anyway
	   so that GCC won't put any of the input or output operands in
	   them.  */
	: "r8", "r9", "r10", "r11", "r12", "r13", "r14");
}
#endif

#if defined(CONFIG_MOT_FEAT_DEBUG_WDOG) || defined(CONFIG_FIQ)
void __attribute__((naked)) get_fiq_regs(struct pt_regs *regs)
{
        register unsigned long tmp;
        asm volatile (
        "mov    ip, sp\n\
        stmfd   sp!, {fp, ip, lr, pc}\n\
        sub     fp, ip, #4\n\
        mrs     %0, cpsr\n\
        msr     cpsr_c, %2      @ select FIQ mode\n\
        mov     r0, r0\n\
        stmia   %1, {r8 - r14}\n\
        msr     cpsr_c, %0      @ return to SVC mode\n\
        mov     r0, r0\n\
        ldmfd   sp, {fp, sp, pc}"
        : "=&r" (tmp)
        : "r" (&regs->ARM_r8), "I" (PSR_I_BIT | PSR_F_BIT | FIQ_MODE));
}
#else
void get_fiq_regs(struct pt_regs *regs)
{
	register unsigned long tmp;
	__asm__ volatile (
	"mrs	%0, cpsr\n\
	msr	cpsr_c, %2	@ select FIQ mode\n\
	mov	r0, r0\n\
	stmia	%1, {r8 - r14}\n\
	msr	cpsr_c, %0	@ return to SVC mode\n\
	mov	r0, r0"
	: "=&r" (tmp)
	: "r" (&regs->ARM_r8), "I" (PSR_I_BIT | PSR_F_BIT | FIQ_MODE)
	/* These registers aren't modified by the above code in a way
	   visible to the compiler, but we mark them as clobbers anyway
	   so that GCC won't put any of the input or output operands in
	   them.  */
	: "r8", "r9", "r10", "r11", "r12", "r13", "r14");
}
#endif

int claim_fiq(struct fiq_handler *f)
{
	int ret = 0;

	if (current_fiq) {
		ret = -EBUSY;

		if (current_fiq->fiq_op != NULL)
			ret = current_fiq->fiq_op(current_fiq->dev_id, 1);
	}

	if (!ret) {
		f->next = current_fiq;
		current_fiq = f;
	}

	return ret;
}

void release_fiq(struct fiq_handler *f)
{
	if (current_fiq != f) {
		printk(KERN_ERR "%s FIQ trying to release %s FIQ\n",
		       f->name, current_fiq->name);
		dump_stack();
		return;
	}

	do
		current_fiq = current_fiq->next;
	while (current_fiq->fiq_op(current_fiq->dev_id, 0));
}

void enable_fiq(int fiq)
{
	enable_irq(fiq + FIQ_START);
}

void disable_fiq(int fiq)
{
	disable_irq(fiq + FIQ_START);
}

EXPORT_SYMBOL(set_fiq_handler);
EXPORT_SYMBOL(set_fiq_regs);
EXPORT_SYMBOL(get_fiq_regs);
EXPORT_SYMBOL(claim_fiq);
EXPORT_SYMBOL(release_fiq);
EXPORT_SYMBOL(enable_fiq);
EXPORT_SYMBOL(disable_fiq);

void __init init_FIQ(void)
{
	no_fiq_insn = *(unsigned long *)0xffff001c;
}

#ifdef CONFIG_MOT_FEAT_FIQ_IN_C

#ifndef _IPU_REGS_INCLUDED_
#define _IPU_REGS_INCLUDED_
 
/* IPU Common registers */
#define IPU_REG_BASE            IO_ADDRESS(IPU_CTRL_BASE_ADDR)
 
#define IPU_CHA_BUF0_RDY        (IPU_REG_BASE + 0x0004)
#define IPU_CHA_BUF1_RDY        (IPU_REG_BASE + 0x0008)
 
#define IPU_IMA_ADDR            (IPU_REG_BASE + 0x0020)
#define IPU_IMA_DATA            (IPU_REG_BASE + 0x0024)
 
#define IPU_INT_CTRL_1          (IPU_REG_BASE + 0x0028)
 
#define IPU_INT_STAT_1          (IPU_REG_BASE + 0x003C)
#define IPU_INT_STAT_2          (IPU_REG_BASE + 0x0040)
#define IPU_INT_STAT_3          (IPU_REG_BASE + 0x0044)
#define IPU_INT_STAT_4          (IPU_REG_BASE + 0x0048)
#define IPU_INT_STAT_5          (IPU_REG_BASE + 0x004C)

#endif

#define FIQ_C_HANDLER_STACK_LEN 8192

static unsigned char mxc_fiq_stack[FIQ_C_HANDLER_STACK_LEN];
volatile unsigned long ipu_reserved_buff_fiq[1203];
#define DMAParamAddr(dma_ch) \
        (0x10000 | (((unsigned long)dma_ch) << 4))

/*
 * FIQ handler in C
 * Don't call other kernel functions here;
 * Don't access vmalloc memory;
 * Don't access user space memory;
 */
static unsigned int last_fiq_event = 0;
static void __attribute__ ((naked)) mxc_fiq_handler(void)
{
	int *p;
	unsigned int *pint;
        volatile unsigned long buf_num,reg;

   	asm __volatile__ (
                "mov    ip, sp;"
                "stmdb  sp!, {r0-r12, lr};" /* Save registers except sp */
                "sub     fp, ip, #256"
                );

	
	p = (unsigned int*)AVIC_FIVECSR;
	switch(*p)
	{
	case 55:
		// Watchdog 2 interrupt
		last_fiq_event = *p;

#ifdef CONFIG_MOT_FEAT_DEBUG_WDOG
		__asm __volatile (
			"mrs r0, spsr;"
			"orr r0, r0, #0xc0;"
			"bic r0, r0, #0x80;"
			"msr spsr_c, r0"
		);
		pint = AVIC_INTENABLEL;
		*pint = 0; /* Disable low interrupt */
		pint = AVIC_INTENABLEH;
		*pint = 1 << (55 - 32); /* Only enable wdog2 interrupt */
		pint = AVIC_INTTYPEH;  /* Watchdog didn't trigger FIQ */
		*pint = 0;
#endif /* CONFIG_MOT_FEAT_DEBUG_WDOG */
		break;
        case 42: //for IPU 4K transfer interrupt.
              buf_num =  ipu_reserved_buff_fiq[1200];
              last_fiq_event = *p; 
              //disable SENSOR_EOF interrupts
              __raw_writel(0x00000000UL,IPU_INT_CTRL_1);
              //read eof	      
              reg = __raw_readl(IPU_INT_STAT_1) & 0x00000080;
              //clear all of interrupt state registers
              __raw_writel(0xFFFFFFFF, IPU_INT_STAT_1);
              __raw_writel(0xFFFFFFFF, IPU_INT_STAT_2);
              __raw_writel(0xFFFFFFFF, IPU_INT_STAT_3);
              __raw_writel(0xFFFFFFFF, IPU_INT_STAT_4);
              __raw_writel(0xFFFFFFFF, IPU_INT_STAT_5);
              if(reg)
              {
                //clear eof
                 __raw_writel(0x00000080UL,IPU_INT_STAT_1);
                if((buf_num < 1200) &&(buf_num >= 0))
                {
                  if(ipu_reserved_buff_fiq[buf_num] != NULL)
                  {
                    if ((buf_num%2) == 0)
                    {
                        /* update physical address for idma buffer 0 */
                        __raw_writel((unsigned long)0x10078, IPU_IMA_ADDR);
                        __raw_writel((uint32_t) ipu_reserved_buff_fiq[buf_num], IPU_IMA_DATA);
                        /* set buffer 0 ready bit */
                        __raw_writel((unsigned long)0x00000080, IPU_CHA_BUF0_RDY);
                    } 
                    else 
                    {
                        /* update physical address for idma buffer 1 */
                        __raw_writel((unsigned long)0x10079, IPU_IMA_ADDR);
                        __raw_writel((uint32_t) ipu_reserved_buff_fiq[buf_num], IPU_IMA_DATA);
                        /* set buffer 1 ready bit */
                        __raw_writel((unsigned long)0x00000080, IPU_CHA_BUF1_RDY);
                    }
                  }
                }
                ipu_reserved_buff_fiq[1200] ++;
                //Note:ipu_reserved_buff_fiq[1201] = nr_pages+2
                if((ipu_reserved_buff_fiq[1200]) == (ipu_reserved_buff_fiq[1201]))
                {
                    ipu_reserved_buff_fiq[1202] = 1;//flag for IDMA over
                }                
              }
              //enable interrupt
              __raw_writel(0x00000080UL,IPU_INT_CTRL_1);
              break;
	}

	asm __volatile__ (
		"ldmia	sp!, {r0-r12, lr};"
		"subs	pc, lr, #4;"
		);
}

void mxc_fiq_init(void)
{
	struct pt_regs fiq_regs;

	fiq_regs.ARM_r8 = (long)mxc_fiq_handler;
	fiq_regs.ARM_sp = (long)(mxc_fiq_stack + sizeof(mxc_fiq_stack) - 4);
	printk(KERN_ERR"#####R8 is %x. sp is %x.\n", fiq_regs.ARM_r8, fiq_regs.ARM_sp);
	set_fiq_regs(&fiq_regs);
	
}

/*
 * mxc_fiq_irq_switch - Setting interrupt to trigger IRQ or FIQ.
 * 			vector: interrupt vector. scope 0 ~ 63
 *			irq2fiq:0 normal IRQ, else FIQ.
 */
int mxc_fiq_irq_switch(int vector, int irq2fiq)
{
	if(vector >= 64 || vector < 0)
	{
		printk(KERN_WARNING"Invalid interrupt vector! %d.\n", vector);
		return -1;
	}

	if(vector >=32)
	{
		if(irq2fiq)
		{
			/* Using FIQ */
			__raw_writel((1 << (vector - 32)) | __raw_readl(AVIC_INTTYPEH), AVIC_INTTYPEH);
		}
		else
		{
			/* Using IRQ */
			__raw_writel(~(1 << (vector - 32)) & __raw_readl(AVIC_INTTYPEH), AVIC_INTTYPEH);
		}
	}
	else
	{
		if(irq2fiq)
		{
			/* Using FIQ */
			__raw_writel((1 << vector) | __raw_readl(AVIC_INTTYPEL), AVIC_INTTYPEL);
		}
		else
		{
			/* Using IRQ */
			__raw_writel(~(1 << vector) & __raw_readl(AVIC_INTTYPEL), AVIC_INTTYPEL);
		}
	}
	return 0;
}

EXPORT_SYMBOL(mxc_fiq_irq_switch);
EXPORT_SYMBOL(ipu_reserved_buff_fiq);

static ssize_t mxc_fiq_dev_write (struct file *file, const char __user *buf, size_t count,
                             loff_t *offset)
{
	struct pt_regs fiq_regs;
	int i;

	memset(&fiq_regs, 0, sizeof(fiq_regs));
	get_fiq_regs(&fiq_regs);
	
	for(i = 0; i < 18; i++)
		printk(KERN_ERR"No %d:%x.\n", i, fiq_regs.uregs[i]);
	printk(KERN_ERR"Last FIQ is %d.\n", last_fiq_event);
        printk("ipu_reserved_buff_fiq[1200]=%d\n",ipu_reserved_buff_fiq[1200]);
        printk("!hreg is %x.\n", __raw_readl(AVIC_INTTYPEH));

	return count;
}

static ssize_t mxc_fiq_dev_read (struct file *file, char __user *buf, size_t count,
                            loff_t *offset)
{
	return 0;
}

static struct file_operations mxc_fiq_dev_fops = {
	.owner		= THIS_MODULE,
        .read		= mxc_fiq_dev_read,
        .write		= mxc_fiq_dev_write,
};

static int __init mxc_fiq_dev_init(void)
{
	dev_t dev;

	dev = MKDEV(199, 0);
	devfs_mk_cdev(dev, S_IFCHR|S_IRUSR|S_IWUSR|S_IRGRP, "mfiq");

	if(register_chrdev(199, "mfiq", &mxc_fiq_dev_fops))
	{
		printk(KERN_WARNING"FIQ Debug interface init failed.\n");
	}

	return 0;
}

static void __exit mxc_fiq_dev_exit(void)
{
	unregister_chrdev(199, "mfiq");
}

module_init(mxc_fiq_dev_init);
module_exit(mxc_fiq_dev_exit);
#endif /* CONFIG_MOT_FEAT_FIQ_IN_C */

