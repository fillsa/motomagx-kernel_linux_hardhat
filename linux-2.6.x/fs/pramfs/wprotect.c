/*
 * FILE NAME fs/pramfs/wprotect.c
 *
 * BRIEF DESCRIPTION
 *
 * Write protection for the filesystem pages.
 *
 * Author: Steve Longerbeam <stevel@mvista.com, or source@mvista.com>
 *
 * Copyright 2003 Sony Corporation
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 2003-2004 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pram_fs.h>
#include <linux/mm.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/tlbflush.h>

// init_mm.page_table_lock must be held before calling!
static void pram_page_writeable(unsigned long addr, int rw)
{
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	
	pgdp = pgd_offset_k(addr);
	if (!pgd_none(*pgdp)) {
		pmdp = pmd_offset(pgdp, addr);
		if (!pmd_none(*pmdp)) {
			pte_t pte;
			ptep = pte_offset_kernel(pmdp, addr);
			pte = *ptep;
			if (pte_present(pte)) {
				pte = rw ? pte_mkwrite(pte) :
					pte_wrprotect(pte);
				set_pte(ptep, pte);
			}
		}
	}
}


// init_mm.page_table_lock must be held before calling!
void pram_writeable(void * vaddr, unsigned long size, int rw)
{
        unsigned long addr = (unsigned long)vaddr & PAGE_MASK;
	unsigned long end = (unsigned long)vaddr + size;
	unsigned long start = addr;

	do {
		pram_page_writeable(addr, rw);
		addr += PAGE_SIZE;
	} while (addr && (addr < end));


	/*
	 * NOTE: we will always flush just one page (one TLB
	 * entry) except possibly in one case: when a new
	 * filesystem is initialized at mount time, when pram_read_super
	 * calls pram_lock_range to make the super block, inode
	 * table, and bitmap writeable.
	 */
#if defined(__arm__) || defined(__mc68000__) || defined(CONFIG_H8300)
	/*
	 * FIXME: so far only these archs have flush_tlb_kernel_page(),
	 * for the rest just use flush_tlb_kernel_range(). Not ideal
	 * to use _range() because many archs just flush the whole TLB.
	 */
	if (end <= start + PAGE_SIZE)
		flush_tlb_kernel_page(start);
	else
#endif
		flush_tlb_kernel_range(start, end);
}
