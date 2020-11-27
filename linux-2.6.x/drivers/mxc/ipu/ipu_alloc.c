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

/*!
 * @file ipu_alloc.c
 *
 * @brief simple ipu allocation driver for linux
 *
 * @ingroup IPU
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <asm/semaphore.h>
#include "ipu.h"
#include "ipu_prv.h"

/* #define IPU_MALLOC_TEST 1 */
static u32 gIPUPoolStart = 0;
static u32 gTotalPages = 0;
static u32 gAlignment = IPU_PAGE_ALIGN;
static memDesc *gAllocatedDesc = NULL;
static struct semaphore gIPUSema;

/*!
 * ipu_pool_initialize
 *
 * @param       memPool         start address of the pool
 * @param       poolSize        memory pool size
 * @param       alignment       alignment for example page alignmnet will be 4K
 *
 * @return      0 for success  -1 for errors
 */
int ipu_pool_initialize(u32 memPool, u32 poolSize, u32 alignment)
{
	gAlignment = alignment;
	if (gAlignment == 0) {
		printk("ipu_pool_initialize : gAlignment can not be zero.\n");
		gAlignment = IPU_PAGE_ALIGN;
	}

	gTotalPages = poolSize / gAlignment;
	gIPUPoolStart = (u32) memPool;

	gAllocatedDesc = (memDesc *) kmalloc(sizeof(memDesc), GFP_KERNEL);
	if (!gAllocatedDesc) {
		printk("ipu_pool_initialize : kmalloc failed \n");
		return (-1);
	}

	gAllocatedDesc->start = 0;
	gAllocatedDesc->end = 0;
	gAllocatedDesc->next = NULL;

	init_MUTEX(&gIPUSema);
	return (0);
}

/*!
 * ipu_malloc
 *
 * @param       size        memory pool size
 *
 * @return      physical address, 0 for error
 */
u32 ipu_malloc(u32 size)
{
	memDesc *prevDesc = NULL;
	memDesc *nextDesc = NULL;
	memDesc *currentDesc = NULL;
	u32 pages = (size + gAlignment - 1) / gAlignment;

	FUNC_START;
	DPRINTK("ipu_malloc alloacte page %x  gTotalPages %x\n", pages,
		gTotalPages);

	if ((size == 0) || (pages > gTotalPages))
		return 0;

	down(&gIPUSema);

	currentDesc = (memDesc *) kmalloc(sizeof(memDesc), GFP_KERNEL);
	if (!currentDesc) {
		printk("ipu_malloc: kmalloc failed \n");
		up(&gIPUSema);
		return 0;
	}

	/* Create the first Allocated descriptor */
	if (!gAllocatedDesc->next) {
		gAllocatedDesc->next = currentDesc;
		currentDesc->start = 0;
		currentDesc->end = pages;
		currentDesc->next = NULL;
		up(&gIPUSema);
		DPRINTK("ipu_malloc 1st current->start %x current->end %x\n",
			currentDesc->start, currentDesc->end);
		return (gIPUPoolStart + currentDesc->start * gAlignment);
	}

	/* Find the free spot */
	prevDesc = gAllocatedDesc;
	while (prevDesc->next) {
		nextDesc = prevDesc->next;
		if (pages <= nextDesc->start - prevDesc->end) {
			currentDesc->start = prevDesc->end;
			currentDesc->end = currentDesc->start + pages;
			currentDesc->next = nextDesc;
			prevDesc->next = currentDesc;
			DPRINTK("find middle cur->start %x cur->end %x\n",
				currentDesc->start, currentDesc->end);
			break;
		}
		prevDesc = nextDesc;
	}

	/* Do not find the free spot inside the chain, append to the end */
	if (!prevDesc->next) {
		if (pages > (gTotalPages - prevDesc->end)) {
			up(&gIPUSema);
			DPRINTK("page %x gTotalPages %x prevDesc->end %x",
				pages, gTotalPages, prevDesc->end);
			return 0;
		} else {
			currentDesc->start = prevDesc->end;
			currentDesc->end = currentDesc->start + pages;
			currentDesc->next = NULL;
			prevDesc->next = currentDesc;
			DPRINTK("append end:cur->start %x cur->end %x\n",
				currentDesc->start, currentDesc->end);
		}
	}

	up(&gIPUSema);

	DPRINTK("ipu_malloc: return %x\n",
		gIPUPoolStart + currentDesc->start * gAlignment);
	FUNC_END;
	return (gIPUPoolStart + currentDesc->start * gAlignment);
}

/*!
 * ipu_free
 *
 * @param       physical        physical address try to free
 *
 */
void ipu_free(u32 physical)
{
	memDesc *prevDesc = NULL;
	memDesc *nextDesc = NULL;
	u32 pages = (physical - gIPUPoolStart) / gAlignment;

	FUNC_START;
	DPRINTK("ipu_free alloacte page %x \n", pages);
	/* Protect the memory pool data structures. */
	down(&gIPUSema);

	prevDesc = gAllocatedDesc;
	while (prevDesc->next) {
		nextDesc = prevDesc->next;
		if (nextDesc->start == pages) {
			prevDesc->next = nextDesc->next;
			DPRINTK("ipu_free next->start %x next->end %x \n",
				nextDesc->start, nextDesc->end);
			DPRINTK("ipu_free prev->start %x prev->end %x \n",
				prevDesc->start, prevDesc->end);
			kfree(nextDesc);
			break;
		}
		prevDesc = prevDesc->next;
	}
	/* All done with memory pool data structures. */
	up(&gIPUSema);
	FUNC_END;
}

/*
 * ipu_malloc_test	 debug only
 *
 */
static __init int ipu_malloc_test(void)
{
#ifdef IPU_MALLOC_TEST
	u32 address1, address2, address3, address4;
	ipu_pool_initialize(MXCIPU_MEM_ADDRESS, MXCIPU_MEM_SIZE, IPU_PAGE_ALIGN);
	address1 = ipu_malloc(0x100);
	if (address1 != MXCIPU_MEM_ADDRESS)
		printk("expect %x addess1 = %x\n",
		       MXCIPU_MEM_ADDRESS, address1);

	address2 = ipu_malloc(0x10000);
	if (address2 != MXCIPU_MEM_ADDRESS + 0x1000)
		printk("expect %x addess2 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0x1000, address2);

	address3 = ipu_malloc(0x200);
	if (address3 != MXCIPU_MEM_ADDRESS + 0x11000)
		printk("expect %x addess3 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0x11000, address3);

	address4 = ipu_malloc(0x20000);
	if (address4 != MXCIPU_MEM_ADDRESS + 0x12000)
		printk("expect %x addess4 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0x12000, address4);

	ipu_free(address2);
	address2 = ipu_malloc(0x1000);
	if (address2 != MXCIPU_MEM_ADDRESS + 0x1000)
		printk("expect %x addess2 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0x1000, address2);

	ipu_free(address3);
	address3 = ipu_malloc(0x2000);
	if (address3 != MXCIPU_MEM_ADDRESS + 0x2000)
		printk("expect %x addess3 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0x2000, address3);

	ipu_free(address4);
	address4 = ipu_malloc(0xc000);
	if (address4 != MXCIPU_MEM_ADDRESS + 0x4000)
		printk("expect %x addess4 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0x4000, address4);

	ipu_free(address2);
	address2 = ipu_malloc(0x20000);
	if (address2 != MXCIPU_MEM_ADDRESS + 0x10000)
		printk("expect %x addess2 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0x10000, address2);

	ipu_free(address1);
	address1 = ipu_malloc(0x40000);
	if (address1 != MXCIPU_MEM_ADDRESS + 0x30000)
		printk("expect %x addess1 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0x30000, address1);

	ipu_free(address2);
	address2 = ipu_malloc(0x40000);
	if (address2 != MXCIPU_MEM_ADDRESS + 0x70000)
		printk("expect %x addess2 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0x70000, address2);

	ipu_free(address3);
	address3 = ipu_malloc(0x50000);
	if (address3 != MXCIPU_MEM_ADDRESS + 0xb0000)
		printk("expect %x addess3 = %x \n",
		       MXCIPU_MEM_ADDRESS + 0xb0000, address3);

	ipu_free(address3);
	address3 = ipu_malloc(0x50001);
	if (address3 != 0)
		printk("expect %x addess3 = %x \n", 0, address3);

	ipu_free(address1);
	ipu_free(address2);
	ipu_free(address3);
	ipu_free(address4);
#endif
	return 0;
}

module_init(ipu_malloc_test);

/* Exported symbols for modules. */
EXPORT_SYMBOL(ipu_pool_initialize);
EXPORT_SYMBOL(ipu_malloc);
EXPORT_SYMBOL(ipu_free);

MODULE_PARM(video_nr, "i");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Graphic memory allocation for Mxc");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("video");
