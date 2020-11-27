/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/* 
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 * 
 * http://www.opensource.org/licenses/gpl-license.html 
 * http://www.gnu.org/copyleft/gpl.html
 */

/* 
 * Encoder device driver (kernel module)
 *
 * Copyright (C) 2005  Hantro Products Oy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
/* needed for __init,__exit directives */
#include <linux/init.h>
/* needed for remap_pfn_range */
#include <linux/mm.h>
/* obviously, for kmalloc */
#include <linux/slab.h>
/* for struct file_operations, register_chrdev() */
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>
/* this header files wraps some common module-space operations ... 
   here we use mem_map_reserve() macro */
#include <linux/page-flags.h>
/* needed for virt_to_phys() */
#include <asm/io.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
/* devfs */
#include <linux/devfs_fs_kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>

#define NON_PAGE_ALIGNED        1	/* when base address not page aligned */
#define CONS_ALLOC              1	/* use consistent_alloc */
#if CONS_ALLOC
static u32 hmp4e_phys;
#endif

/* our own stuff */
#include "mxc_hmp4e.h"
#include <asm/hardware.h>
#include <linux/pm.h>
#define APM_HMP4E 0
/* use apm for hmp4e driver
   note, after hmp4e is inserted, apm level will be set to LOW, i.e.
   HCLK >= 88MHz, in order to acheive 30fps */

/* module description */
MODULE_AUTHOR("Hantro Products Oy");
MODULE_DESCRIPTION("Device driver for Hantro's MPEG4 encoder HW");
MODULE_SUPPORTED_DEVICE("4400 MPEG4 Encoder");

/* these could be module params in the future */

#define ENC_IO_SIZE                 (16*4)	/* bytes */
#define HMP4E_BUF_SIZE              512000	/* bytes */

#define ENC_HW_ID                   0x00001882

unsigned long base_port = MPEG4_ENC_BASE_ADDR;	//0x10026400;
unsigned int irq = INT_MPEG4_ENC;	//51;

MODULE_PARM(base_port, "l");
MODULE_PARM(irq, "i");

/* and this is our MAJOR; use 0 for dynamic allocation (recommended)*/
static int hmp4e_major = 0;

/* For module reference count */
static int count = 0;

/* here's all the must remember stuff */
typedef struct {
	char *buffer;
	unsigned int buffsize;
	unsigned long iobaseaddr;
	unsigned int iosize;
	volatile u32 *hwregs;
	unsigned int irq;
	struct fasync_struct *async_queue;
} hmp4e_t;

static hmp4e_t hmp4e_data;	/* dynamic allocation? */

/* 
 * avoid "enable_irq(x) unbalanced from ..."
 * error messages from the kernel, since {ena,dis}able_irq()
 * calls are stacked in kernel.
 */
static bool irq_enable = false;

static int AllocMemory(void);
static void FreeMemory(void);

static int ReserveIO(void);
static void ReleaseIO(void);

static int MapBuffers(struct file *filp, struct vm_area_struct *vma);
static int MapHwRegs(struct file *filp, struct vm_area_struct *vma);
static void ResetAsic(hmp4e_t * dev);

/* local */
static void hmp4ehw_clock_enable(void);
static void hmp4ehw_clock_disable(void);
/*readable by other modules through ipc */
/*static int g_hmp4e_busy = 0;*/

static irqreturn_t hmp4e_isr(int irq, void *dev_id, struct pt_regs *regs);

/* VM operations */
static struct page *hmp4e_vm_nopage(struct vm_area_struct *vma,
				    unsigned long address, int *write_access)
{
	PDEBUG("hmp4e_vm_nopage: problem with mem access\n");
	return NOPAGE_SIGBUS;	/* send a SIGBUS */
}

static void hmp4e_vm_open(struct vm_area_struct *vma)
{
	count++;
	PDEBUG("hmp4e_vm_open:\n");
}

static void hmp4e_vm_close(struct vm_area_struct *vma)
{
	count--;
	PDEBUG("hmp4e_vm_close:\n");
}

static struct vm_operations_struct hmp4e_vm_ops = {
      open:hmp4e_vm_open,
      close:hmp4e_vm_close,
      nopage:hmp4e_vm_nopage,
};

/* the device's mmap method. The VFS has kindly prepared the process's
 * vm_area_struct for us, so we examine this to see what was requested.
 */
static int hmp4e_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int result;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
#if NON_PAGE_ALIGNED
	int ofs;

	ofs = hmp4e_data.iobaseaddr & (PAGE_SIZE - 1);
#endif

	PDEBUG("hmp4e_mmap: size = %lu off = 0x%08lx\n",
	       vma->vm_end - vma->vm_start, offset);

	if (offset == 0)
		result = MapBuffers(filp, vma);
#if NON_PAGE_ALIGNED
	else if (offset == hmp4e_data.iobaseaddr - ofs)
#else
	else if (offset == hmp4e_data.iobaseaddr)
#endif
		result = MapHwRegs(filp, vma);
	else
		result = -EINVAL;

	if (result == 0) {
		vma->vm_ops = &hmp4e_vm_ops;
		/* open is not implicitly called when mmap is called */
		hmp4e_vm_open(vma);
	}

	return result;
}

static int hmp4e_ioctl(struct inode *inode, struct file *filp,
		       unsigned int cmd, unsigned long arg)
{
	int err = 0;

	PDEBUG("ioctl cmd 0x%08ux\n", cmd);
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != HMP4E_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > HMP4E_IOC_MAXNR)
		return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {
	case HMP4E_IOCHARDRESET:
		/*
		 * reset the counter to 1, to allow unloading in case
		 * of problems. Use 1, not 0, because the invoking
		 * process has the device open.
		 */
		while (count > 0)
			count--;
		count++;
		break;

	case HMP4E_IOCGBUFBUSADDRESS:
#if !CONS_ALLOC
		__put_user(virt_to_bus(hmp4e_data.buffer),
			   (unsigned long *)arg);
#else
		__put_user(hmp4e_phys, (unsigned long *)arg);
#endif
		break;

	case HMP4E_IOCGBUFSIZE:
		__put_user(hmp4e_data.buffsize, (unsigned int *)arg);
		break;

	case HMP4E_IOCGHWOFFSET:
		__put_user(hmp4e_data.iobaseaddr, (unsigned long *)arg);
		break;

	case HMP4E_IOCGHWIOSIZE:
		__put_user(hmp4e_data.iosize, (unsigned int *)arg);
		break;

	case HMP4E_IOC_CLI:
		if (irq_enable == true) {
			disable_irq(hmp4e_data.irq);
			irq_enable = false;
		}
		break;

	case HMP4E_IOC_STI:
		if (irq_enable == false) {
			enable_irq(hmp4e_data.irq);
			irq_enable = true;
		}
		break;
	}
	return 0;
}

static int hmp4e_open(struct inode *inode, struct file *filp)
{
	int result;
	hmp4e_t *dev = &hmp4e_data;

	filp->private_data = (void *)dev;

	if (count > 0)
		return -EBUSY;

	result = request_irq(dev->irq, hmp4e_isr, 0, "hmp4e", (void *)dev);
	if (result == -EINVAL) {
		printk(KERN_ERR "hmp4e: Bad irq number or handler\n");
		return result;
	} else if (result == -EBUSY) {
		printk(KERN_ERR "hmp4e: IRQ %d busy, change your config\n",
		       dev->irq);
		return result;
	}

	if (irq_enable == false) {
		irq_enable = true;
	}
	/* enable_irq(dev->irq); */

	count++;
	PDEBUG("dev opened\n");
	return 0;
}

static int hmp4e_fasync(int fd, struct file *filp, int mode)
{
	hmp4e_t *dev = (hmp4e_t *) filp->private_data;

	PDEBUG("fasync called\n");

	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

#ifdef HMP4E_DEBUG
static void dump_regs(unsigned long data);
#endif

static int hmp4e_release(struct inode *inode, struct file *filp)
{
	hmp4e_t *dev = (hmp4e_t *) filp->private_data;

	/* this is necessary if user process exited asynchronously */
	if (irq_enable == true) {
		disable_irq(dev->irq);
		irq_enable = false;
	}
#ifdef HMP4E_DEBUG
	dump_regs((unsigned long)dev);	/* dump the regs */
#endif
	ResetAsic(dev);		/* reset hardware */

	/* free the encoder IRQ */
	free_irq(dev->irq, (void *)dev);

	/* remove this filp from the asynchronusly notified filp's */
	hmp4e_fasync(-1, filp, 0);

	/* g_hmp4e_busy = 0; */

	count--;
	PDEBUG("dev closed\n");
	return 0;
}

/* VFS methods */
static struct file_operations hmp4e_fops = {
      mmap:hmp4e_mmap,
      open:hmp4e_open,
      release:hmp4e_release,
      ioctl:hmp4e_ioctl,
      fasync:hmp4e_fasync,
};

int __init hmp4e_init(void)
{
	int result;

	/* if you want to test the module, you obviously need to "mknod". */
	PDEBUG("module init\n");

	printk(KERN_INFO "hmp4e: base_port=0x%08lx irq=%i\n", base_port, irq);

	/* make sure clock is enabled */
	hmp4ehw_clock_enable();

	hmp4e_data.iobaseaddr = base_port;
	hmp4e_data.iosize = ENC_IO_SIZE;
	hmp4e_data.irq = irq;

	result = register_chrdev(hmp4e_major, "hmp4e", &hmp4e_fops);
	if (result < 0) {
		printk(KERN_INFO "hmp4e: unable to get major %d\n",
		       hmp4e_major);
		goto err;
	} else if (result != 0) {	/* this is for dynamic major */
		hmp4e_major = result;
	}

	devfs_mk_cdev(MKDEV(result, 0), S_IFCHR | S_IRUGO | S_IWUSR, "hmp4e");

	/* register for ipc */
	/* in case other emma device want to get the status of each other 
	   g_hmp4e_busy = 0;
	   inter_module_register("string_hmp4e_busy", THIS_MODULE,&g_hmp4e_busy); */
	result = ReserveIO();
	if (result < 0) {
		goto err;
	}

	ResetAsic(&hmp4e_data);	/* reset hardware */

	result = AllocMemory();
	if (result < 0) {
		ReleaseIO();
		goto err;
	}

	printk(KERN_INFO "hmp4e: module inserted. Major = %d\n", hmp4e_major);

	return 0;

      err:
	printk(KERN_INFO "hmp4e: module not inserted\n");
	unregister_chrdev(hmp4e_major, "hmp4e");
	return result;
}

/* changed to devfs */
void __exit hmp4e_cleanup(void)
{
	/* unregister ipc 
	   inter_module_unregister("string_hmp4e_busy"); */

	devfs_remove("hmp4e");
	unregister_chrdev(hmp4e_major, "hmp4e");

	hmp4ehw_clock_disable();

	FreeMemory();

	ReleaseIO();

	printk(KERN_INFO "Encoder driver is unloaded sucessfully\n");
}

module_init(hmp4e_init);
module_exit(hmp4e_cleanup);

static int AllocMemory(void)
{
#if !CONS_ALLOC
	struct page *pg;
	char *start_addr, *end_addr;
	u32 order, size;

	PDEBUG("hmp4e: AllocMemory: Begin\n");

	hmp4e_data.buffer = NULL;
	hmp4e_data.buffsize = HMP4E_BUF_SIZE;
	PDEBUG("hmp4e: AllocMemory: Buffer size %d\n", hmp4e_data.buffsize);

	for (order = 0, size = PAGE_SIZE; size < hmp4e_data.buffsize;
	     order++, size <<= 1) ;
	hmp4e_data.buffsize = size;

	/* alloc memory */
	start_addr = (char *)__get_free_pages(GFP_KERNEL, order);

	if (!start_addr) {
		PDEBUG("hmp4e: failed to allocate memory\n");
		PDEBUG("hmp4e: AllocMemory: End (-ENOMEM)\n");
		return -ENOMEM;
	} else {
		memset(start_addr, 0, hmp4e_data.buffsize);	/* clear the mem */

		hmp4e_data.buffer = start_addr;
	}
	end_addr = start_addr + hmp4e_data.buffsize - 1;

	PDEBUG("Alloc buffer 0x%08lx -- 0x%08lx\n", (long)start_addr,
	       (long)end_addr);

	/* now we've got the kernel memory, but it can still be
	 * swapped out. We need to stop the VM system from removing our
	 * pages from main memory. To do this we just need to set the PG_reserved 
	 * bit on each page, via mem_map_reserve() macro.
	 */

	/* If we don't set the reserved bit, the user-space application sees 
	 * all-zeroes pages. This is because remap_pfn_range() won't allow you to
	 * map non-reserved pages (check remap_pte_range()).
	 * The pte's will be cleared, resulting in a page faulting in a new zeroed 
	 * page instead of the pages we are trying to mmap(). 
	 */
	for (pg = virt_to_page(start_addr); pg <= virt_to_page(end_addr); pg++) {
		mem_map_reserve(pg);
	}
#else
	hmp4e_data.buffsize = (HMP4E_BUF_SIZE + PAGE_SIZE - 1) &
	    ~(PAGE_SIZE - 1);
	hmp4e_data.buffer = dma_alloc_coherent(NULL, hmp4e_data.buffsize,
					       (dma_addr_t *) & hmp4e_phys,
					       GFP_DMA | GFP_KERNEL);
	if (!hmp4e_data.buffer) {
		return -ENOMEM;
	}
	memset(hmp4e_data.buffer, 0, hmp4e_data.buffsize);
#endif
	PDEBUG("hmp4e: AllocMemory: End (0)\n");
	return 0;
}

static void FreeMemory(void)
{
#if !CONS_ALLOC
	struct page *pg;
	u32 size, order;

	/* first unreserve */
	for (pg = virt_to_page(hmp4e_data.buffer);
	     pg < virt_to_page(hmp4e_data.buffer + hmp4e_data.buffsize); pg++) {
		mem_map_unreserve(pg);
	}
	for (order = 0, size = PAGE_SIZE; size < hmp4e_data.buffsize;
	     order++, size <<= 1) ;
	/* and now free */
	free_pages((long)hmp4e_data.buffer, order);
	PDEBUG("Free buffer 0x%08lx -- 0x%08lx\n", (long)hmp4e_data.buffer,
	       (long)(hmp4e_data.buffer + hmp4e_data.buffsize - 1));
#else
	dma_free_coherent(NULL, hmp4e_data.buffsize, (void *)hmp4e_data.buffer,
			  hmp4e_phys);
#endif
}

static int ReserveIO(void)
{
	long int hwid;

	hmp4e_data.hwregs = (volatile u32 *)IO_ADDRESS(hmp4e_data.iobaseaddr);
	if (hmp4e_data.hwregs == NULL) {
		printk(KERN_INFO "hmp4e: failed to ioremap HW regs\n");
		ReleaseIO();
		return -EBUSY;
	}

	hwid = __raw_readl(hmp4e_data.hwregs + 7);

	if ((hwid & 0x0000ffff) != ENC_HW_ID) {
		printk(KERN_INFO "hmp4e: HW not found at 0x%08lx\n",
		       hmp4e_data.iobaseaddr);
		ReleaseIO();
		return -EBUSY;
	} else {
		printk(KERN_INFO "hmp4e: Compatble HW found with ID: 0x%08lx\n",
		       hwid);
	}

	return 0;
}

static void ReleaseIO(void)
{
}

static int MapBuffers(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long phys;
	unsigned long start = (unsigned long)vma->vm_start;
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

	/* if userspace tries to mmap beyond end of our buffer, fail */
	if (size > hmp4e_data.buffsize) {
		PDEBUG("hmp4e: MapBuffers (-EINVAL)\n");
		return -EINVAL;
	}

	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

#if !CONS_ALLOC
	/* Remember this won't work for vmalloc()d memory ! */
	phys = virt_to_phys(hmp4e_data.buffer);
#else
	phys = hmp4e_phys;
#endif

	if (remap_pfn_range
	    (vma, start, phys >> PAGE_SHIFT, size, vma->vm_page_prot)) {
		PDEBUG("hmp4e: MapBuffers (-EAGAIN)\n");
		return -EAGAIN;
	}

	PDEBUG("hmp4e: MapBuffers (0)\n");
	return 0;
}

static int MapHwRegs(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long phys;
	unsigned long start = (unsigned long)vma->vm_start;
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);
#if NON_PAGE_ALIGNED
	int ofs;

	ofs = hmp4e_data.iobaseaddr & (PAGE_SIZE - 1);
#endif

	/* if userspace tries to mmap beyond end of our buffer, fail */
	if (size > PAGE_SIZE)
		return -EINVAL;

	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/* Remember this won't work for vmalloc()d memory ! */
	phys = hmp4e_data.iobaseaddr;

#if NON_PAGE_ALIGNED
	if (remap_pfn_range(vma, start, (phys - ofs) >> PAGE_SHIFT,
			    hmp4e_data.iosize + ofs, vma->vm_page_prot))
#else
	if (remap_pfn_range(vma, start, phys >> PAGE_SHIFT, hmp4e_data.iosize,
			    vma->vm_page_prot))
#endif
	{
		return -EAGAIN;
	}

	return 0;
}

static irqreturn_t hmp4e_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	hmp4e_t *dev = (hmp4e_t *) dev_id;
	u32 irq_status = __raw_readl(dev->hwregs + 5);

	__raw_writel(irq_status & (~0x01), dev->hwregs + 5);	/* clear enc IRQ */
	PDEBUG("IRQ received!\n");
	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);

	return IRQ_HANDLED;
}

void ResetAsic(hmp4e_t * dev)
{
	u32 tmp;

	tmp = __raw_readl(dev->hwregs);
	tmp = tmp | 0x02;
	__raw_writel(tmp, dev->hwregs);	/* enable enc CLK */
	__raw_writel(tmp & (~0x01), dev->hwregs);	/* disable enc */
	__raw_writel(0x02, dev->hwregs);	/* clear reg 1 */
	__raw_writel(0, dev->hwregs + 5);	/* clear enc IRQ */
	__raw_writel(0, dev->hwregs);	/* disable enc CLK */
}

#ifdef HMP4E_DEBUG
void dump_regs(unsigned long data)
{
	hmp4e_t *dev = (hmp4e_t *) data;
	int i;

	PDEBUG("Reg Dump Start\n");
	for (i = 0; i < 16; i++) {
		PDEBUG("\toffset %02X = %08X\n", i * 4,
		       __raw_readl(dev->hwregs + i));
	}
	PDEBUG("Reg Dump End\n");
}
#endif

static void hmp4ehw_clock_enable(void)
{
	if ((__raw_readl(CCM_MCGR1_REG) & MCGR1_MPEG4_CLK_EN) !=
	    MCGR1_MPEG4_CLK_EN)
		__raw_writel(__raw_readl(CCM_MCGR1_REG) | MCGR1_MPEG4_CLK_EN,
			     CCM_MCGR1_REG);
}

static void hmp4ehw_clock_disable(void)
{
	__raw_writel(__raw_readl(CCM_MCGR1_REG) & ~MCGR1_MPEG4_CLK_EN,
		     CCM_MCGR1_REG);
}

MODULE_LICENSE("GPL");
