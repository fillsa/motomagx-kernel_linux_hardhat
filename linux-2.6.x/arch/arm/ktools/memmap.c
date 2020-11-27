/*
 * FILE NAME arch/arm/ktools/memmap.c
 *
 * BRIEF MODULE DESCRIPTION
 *
 * Author: David Singleton dsingleton@mvista.com MontaVista Software, Inc.
 *
 * 2001-2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/namespace.h>
#include <linux/mm.h>
#include <linux/kallsyms.h>
#include <linux/proc_fs.h>
#include <linux/mount.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#if defined(CONFIG_KMOD)
#include <linux/kmod.h>
#endif

#define VERSION 1.1
#define MODULE_VERS "1.1"
#define MODULE_NAME "memmap"
#define PREFIX "MEM ACC"
#define DPRINT(args...) if (mem_acc_debug) printk(PREFIX args)
#define BITS_PER_INT 	32
#define ONE_LINE	65

                                                       
extern int lowwater;
static int highwater = 0;
static int mem_acc_debug = 0;
static int index = 0;

static void
count_pmd_pages(struct mm_struct * mm, struct vm_area_struct * vma,
		pmd_t *dir, unsigned long address, unsigned long end,
		signed char *data_buf, int node_map)
{
	pte_t * pte;
	unsigned long pmd_end;
	struct page *page;
	unsigned long pfn;
	int val, index;

	if (pmd_none(*dir))
		return;
	pmd_end = (address + PMD_SIZE) & PMD_MASK;
	if (end > pmd_end)
		end = pmd_end;
	index = 0;
	do {
		pte = pte_offset_map(dir, address);
		if (!pte_none(pte) && pte_present(*pte)) {
			pfn = pte_pfn(*pte);
			if (pfn_valid(pfn)) {
				page = pfn_to_page(pfn);
				val = node_map ? page_to_nid(page) :
					page_count(page);
				val = (val > 99) ? 99 : val;
				data_buf[index] = val;
			}
		}
		address += PAGE_SIZE;
		pte++;
		index++;
	} while (address && (address < end));
	return;
}

static void
count_pgd_pages(struct mm_struct * mm, struct vm_area_struct * vma,
		pgd_t *dir, unsigned long address, unsigned long end,
		signed char *data_buf, int node_map)
{
	pmd_t * pmd;
	unsigned long pgd_end, next_addr;

	if (pgd_none(*dir))
		return;
	pmd = pmd_offset(dir, address);
	pgd_end = (address + PGDIR_SIZE) & PGDIR_MASK;
	if (pgd_end && (end > pgd_end))
		end = pgd_end;
	do {
		count_pmd_pages(mm, vma, pmd, address, end, data_buf,
				node_map);
		next_addr = (address + PMD_SIZE) & PMD_MASK;
		data_buf += ((next_addr - address) >> PAGE_SHIFT);
		address = next_addr;
		pmd++;
	} while (address && (address < end));
	return;
}

void
count_physical_pages(struct mm_struct * mm, struct vm_area_struct * vma,
		     signed char *data_buf, int node_map)
{
	pgd_t *pgdir;
	unsigned long end;
	unsigned long address, next_addr;

#ifdef CONFIG_XIP_KERNEL
	if ((vma->vm_flags & VM_XIP) && (vma->vm_flags & VM_WRITE) == 0) {
		return;
	}
#endif
	end = vma->vm_end;
	address = vma->vm_start;
	if (address > VMALLOC_END)
		return;
	spin_lock(&mm->page_table_lock);
	pgdir = pgd_offset(mm, address);
	do {
		count_pgd_pages(mm, vma, pgdir, address, end, data_buf,
				node_map);
		next_addr = (address + PGDIR_SIZE) & PGDIR_MASK;
		data_buf += ((next_addr - address) >> PAGE_SHIFT);
		address = next_addr;
		pgdir++;
	} while (address && (address < end));
	spin_unlock(&mm->page_table_lock);
	return;
}

EXPORT_SYMBOL(count_physical_pages);

/*
 * We walk all the page structs and translate them to pfns
 * We get the free pages by the page count, count == 0.
 *
 * We have an array of unsigned ints of which each bit in an int
 * represents one page in memory.
 *
 * The pfn is the index into the array, pfn 0 == index 0 (index = pfn / 32).
 * We figure out which bit of the unsigned int to set by the following:
 * bit = pfn % 32, we then set that bit to zero to denote it's free via:
 * array[index] &= ~(1<<bit);   then we pump the data to /proc
 */
int show_physical_memory(char *page, char **start, off_t off,
			 int count, int *eof, void *data)
{
	static unsigned int *bitmap = NULL;
	pg_data_t *pgdat;
	struct page *p;
	int mark;
	int len = 0;
	int total, free;
	int i, bit;
	int num_ints = (num_physpages + BITS_PER_INT - 1) / BITS_PER_INT;
	num_ints = (num_ints + 7) & ~7;
	
	if (off == 0) {
		if (bitmap == NULL) {
			bitmap = (unsigned int *)
				kmalloc(num_ints * sizeof (int), GFP_KERNEL);
		}
		
		preempt_disable();
		/*
		 * one means used, zero means free, set them all to 1,
		 * clear them as we find them free.
		 */
		
		for (i = 0; i < num_ints; i++) {
			bitmap[i] = 0xffffffff;
		}
		total = free = 0;
		
		for_each_pgdat(pgdat) {
			for (i=0, p = pgdat->node_mem_map;
			     i < pgdat->node_spanned_pages; i++, p++) {
				int pfn = page_to_pfn(p);
				if (pfn_valid(pfn) && !PageReserved(p) &&
				    page_count(p) == 0) {
					free++;
					bit = total % BITS_PER_INT;
					bitmap[total/BITS_PER_INT] &=
						~(1<<bit);
					if (mem_acc_debug)
						printk("i %d bit %d pfn %d "
						       "pa 0x%lx c %d\n",
						       index, bit, pfn,
						       page_to_phys(p),
						       page_count(p));
				}
				total++;
			}
		}
		
		mark = total - free;
		if (mark < lowwater) {
			lowwater = mark;
		}
		if (mark > highwater) {
			highwater = mark;
		}

		index = 0;
		len = sprintf(page, "High water used pages %d "
			      "Low water used pages %d\n",
			      highwater, lowwater);
	} else
		preempt_disable();

	for (; index < num_ints && len+ONE_LINE < count; index += 8) {
		len += sprintf(page + len,
			       "%08x%08x%08x%08x%08x%08x%08x%08x\n",
			       bitmap[index + 0], bitmap[index + 1],
			       bitmap[index + 2], bitmap[index + 3],
			       bitmap[index + 4], bitmap[index + 5],
			       bitmap[index + 6], bitmap[index + 7]);
	}
	if (index >= num_ints || len > count) {
		*eof = 1;
		if (bitmap) {
			preempt_enable();
			kfree(bitmap);
			preempt_disable();
			bitmap = NULL;
		}
	}
	
        *start = page;
	preempt_enable();
        return len;
}
EXPORT_SYMBOL(show_physical_memory);

