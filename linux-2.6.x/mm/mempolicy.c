/*
 * Simple NUMA memory policy for the Linux kernel.
 *
 * Copyright 2003,2004 Andi Kleen, SuSE Labs.
 * Copyright 2004,2005 MontaVista Software.
 * Subject to the GNU Public License, version 2.
 *
 * NUMA policy allows the user to give hints in which node(s) memory should
 * be allocated.
 *
 * Support four policies per VMA and per process:
 *
 * The VMA policy has priority over the process policy for a page fault.
 *
 * interleave     Allocate memory interleaved over a set of nodes,
 *                with normal fallback if it fails.
 *                For VMA based allocations this interleaves based on the
 *                offset into the backing object or offset into the mapping
 *                for anonymous memory. For process policy an process counter
 *                is used.
 * bind           Only allocate memory on a specific set of nodes,
 *                no fallback.
 * preferred       Try a specific node first before normal fallback.
 *                As a special case node -1 here means do the allocation
 *                on the local CPU. This is normally identical to default,
 *                but useful to set in a VMA when you have a non default
 *                process policy.
 * default        Allocate on the local node first, or when on a VMA
 *                use the process policy. This is what Linux always did
 *		  in a NUMA aware kernel and still does by, ahem, default.
 *
 * The process policy is applied for most non interrupt memory allocations
 * in that process' context. Interrupts ignore the policies and always
 * try to allocate on the local CPU. The VMA policy is only applied for memory
 * allocations for a VMA in the VM.
 *
 * Currently there are a few corner cases in swapping where the policy
 * is not applied, but the majority should be handled. When process policy
 * is used it is not remembered over swap outs/swap ins.
 *
 * Only the highest zone in the zone hierarchy gets policied. Allocations
 * requesting a lower zone just use default policy. This implies that
 * on systems with highmem kernel lowmem allocation don't get policied.
 * Same with GFP_DMA allocations.
 *
 * For shmfs/tmpfs/hugetlbfs shared memory the policy is shared between
 * all users and remembered even when nobody has memory mapped.
 */

/* Notebook:
   Page cache pages can now be policied, by adding a shared_policy tree to
   inodes (actually located in address_space). One entry in the tree for
   each mapped region of a file. Generic files now have set_policy and
   get_policy methods in generic_file_vm_ops [stevel@mvista.com].

   Added a page-migration feature, whereby existing pte-mapped or filemap
   pagecache pages that are/can be mapped to the given virtual memory
   region, that do not satisfy the NUMA policy, are moved to a new
   page that satisfies the policy. Enabled by the new mbind flag
   MPOL_MF_MOVE [stevel@mvista.com].

   statistics for bigpages.

   global policy for page cache? currently it uses per-file policies in
   address_space (see first item above).

   handle mremap for shared memory (currently ignored for the policy)
   grows down?

   make bind policy root only? It can trigger oom much faster and the
   kernel is not always grateful with that.

   could replace all the switch()es with a mempolicy_ops structure.
*/

#include <linux/mempolicy.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/hugetlb.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/nodemask.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/compat.h>
#include <linux/mempolicy.h>
#include <linux/rmap.h>
#include <linux/swap.h>
#include <asm/tlbflush.h>
#include <asm/pgalloc.h>
#include <asm/uaccess.h>

static kmem_cache_t *policy_cache;
static kmem_cache_t *sn_cache;

#undef DEBUG_SHARED_POLICY

#define PDprintk(fmt...)
#define PMDprintk(fmt...) printk(KERN_DEBUG fmt)

/* forward references */
static struct mempolicy * get_vma_policy(struct vm_area_struct *vma,
					 unsigned long addr);
static int __mpol_node_valid(int nid, struct mempolicy *pol);

/* Highest zone. An specific allocation for a zone below that is not
   policied. */
static int policy_zone;

static struct mempolicy default_policy = {
	.refcnt = ATOMIC_INIT(1), /* never free it */
	.policy = MPOL_DEFAULT,
};

/* Check if all specified nodes are online */
static int nodes_online(unsigned long *nodes)
{
	DECLARE_BITMAP(online2, MAX_NUMNODES);

	bitmap_copy(online2, nodes_addr(node_online_map), MAX_NUMNODES);
	if (bitmap_empty(online2, MAX_NUMNODES))
		set_bit(0, online2);
	if (!bitmap_subset(nodes, online2, MAX_NUMNODES))
		return -EINVAL;
	return 0;
}

/* Do sanity checking on a policy */
static int mpol_check_policy(int mode, unsigned long *nodes)
{
	int empty = bitmap_empty(nodes, MAX_NUMNODES);

	switch (mode) {
	case MPOL_DEFAULT:
		if (!empty)
			return -EINVAL;
		break;
	case MPOL_BIND:
	case MPOL_INTERLEAVE:
		/* Preferred will only use the first bit, but allow
		   more for now. */
		if (empty)
			return -EINVAL;
		break;
	}
	return nodes_online(nodes);
}

/* Copy a node mask from user space. */
static int get_nodes(unsigned long *nodes, unsigned long __user *nmask,
		     unsigned long maxnode, int mode)
{
	unsigned long k;
	unsigned long nlongs;
	unsigned long endmask;

	--maxnode;
	bitmap_zero(nodes, MAX_NUMNODES);
	if (maxnode == 0 || !nmask)
		return 0;
	if (maxnode > PAGE_SIZE)
		return -EINVAL;

	nlongs = BITS_TO_LONGS(maxnode);
	if ((maxnode % BITS_PER_LONG) == 0)
		endmask = ~0UL;
	else
		endmask = (1UL << (maxnode % BITS_PER_LONG)) - 1;

	/* When the user specified more nodes than supported just check
	   if the non supported part is all zero. */
	if (nlongs > BITS_TO_LONGS(MAX_NUMNODES)) {
		if (nlongs > PAGE_SIZE/sizeof(long))
			return -EINVAL;
		for (k = BITS_TO_LONGS(MAX_NUMNODES); k < nlongs; k++) {
			unsigned long t;
			if (get_user(t,  nmask + k))
				return -EFAULT;
			if (k == nlongs - 1) {
				if (t & endmask)
					return -EINVAL;
			} else if (t)
				return -EINVAL;
		}
		nlongs = BITS_TO_LONGS(MAX_NUMNODES);
		endmask = ~0UL;
	}

	if (copy_from_user(nodes, nmask, nlongs*sizeof(unsigned long)))
		return -EFAULT;
	nodes[nlongs-1] &= endmask;
	return mpol_check_policy(mode, nodes);
}

/* Generate a custom zonelist for the BIND policy. */
static struct zonelist *bind_zonelist(unsigned long *nodes)
{
	struct zonelist *zl;
	int num, max, nd;

	max = 1 + MAX_NR_ZONES * bitmap_weight(nodes, MAX_NUMNODES);
	zl = kmalloc(sizeof(void *) * max, GFP_KERNEL);
	if (!zl)
		return NULL;
	num = 0;
	for (nd = find_first_bit(nodes, MAX_NUMNODES);
	     nd < MAX_NUMNODES;
	     nd = find_next_bit(nodes, MAX_NUMNODES, 1+nd)) {
		int k;
		for (k = MAX_NR_ZONES-1; k >= 0; k--) {
			struct zone *z = &NODE_DATA(nd)->node_zones[k];
			if (!z->present_pages)
				continue;
			zl->zones[num++] = z;
			if (k > policy_zone)
				policy_zone = k;
		}
	}
	BUG_ON(num >= max);
	zl->zones[num] = NULL;
	return zl;
}

/* Create a new policy */
static struct mempolicy *mpol_new(int mode, unsigned long *nodes)
{
	struct mempolicy *policy;

	PDprintk("setting mode %d nodes[0] %lx\n", mode, nodes[0]);
	if (mode == MPOL_DEFAULT)
		return NULL;
	policy = kmem_cache_alloc(policy_cache, GFP_KERNEL);
	if (!policy)
		return ERR_PTR(-ENOMEM);
	atomic_set(&policy->refcnt, 1);
	switch (mode) {
	case MPOL_INTERLEAVE:
		bitmap_copy(policy->v.nodes, nodes, MAX_NUMNODES);
		break;
	case MPOL_PREFERRED:
		policy->v.preferred_node = find_first_bit(nodes, MAX_NUMNODES);
		if (policy->v.preferred_node >= MAX_NUMNODES)
			policy->v.preferred_node = -1;
		break;
	case MPOL_BIND:
		policy->v.zonelist = bind_zonelist(nodes);
		if (policy->v.zonelist == NULL) {
			kmem_cache_free(policy_cache, policy);
			return ERR_PTR(-ENOMEM);
		}
		break;
	}
	policy->policy = mode;
	return policy;
}


/*
 * The given page doesn't match a file mapped VMA's policy. If the
 * page is unused, remove it from the page cache, so that a new page
 * can be later reallocated to the cache using the correct policy.
 * Returns 0 if the page was removed from the cache, < 0 if failed.
 *
 * We use invalidate_mapping_pages(), which doesn't try very hard.
 * It won't remove pages which are locked (won't wait for a lock),
 * dirty, under writeback, or mapped by pte's. If the page is currently
 * being mapped, give up right away (but this is unlikely for ELF files
 * marked with policies, because mbind is called the very first time the
 * file is mapped, by the kernel for executables and ld.so, and by libc
 * for shared libs). If the page is not mapped, wait for locks and
 * writebacks, and try to free again.
 */
static int
remove_filemap_page(struct page * page,
		    struct vm_area_struct *vma,
		    pgoff_t pgoff)
{
	struct address_space *mapping = vma->vm_file->f_mapping;
	int release_attempt = 0;

	/*
	 * the page in the cache is not in any of the nodes this
	 * VMA's policy wants it to be in. Can we remove it?
	 */
	do {
		unsigned long ret = invalidate_mapping_pages(mapping,
							     pgoff, pgoff);
		struct page * p = find_get_page(mapping, pgoff);
		
		if (ret && !p) {
			PMDprintk("%s page %p:%lu released "
				  "after %d tries\n",
				  vma->vm_file->f_dentry->d_name.name,
				  page, pgoff, release_attempt);
			return 0;
		}

		if (p) {
			/*
			 * the page is being used by other pagetable mappings,
			 * or is currently locked, dirty, or under writeback.
			 */
			PMDprintk("%s page %p:%lu release attempt #%d failed, "
				  "m:l:d:wb = %d:%d:%d:%d\n",
				  vma->vm_file->f_dentry->d_name.name,
				  p, pgoff, release_attempt,
				  page_mapped(p), PageLocked(p),
				  PageDirty(p), PageWriteback(p));
			
			if (page_mapped(p)) {
				page_cache_release(p); /* find_get_page above*/
				break;
			}
		
			wait_on_page_locked(p);
			wait_on_page_writeback(p);
			page_cache_release(p);  /* find_get_page above */
		}
	} while (++release_attempt < 3);

	PMDprintk("%s page %p:%lu release failed, m:l:d:wb = %d:%d:%d:%d\n",
		  vma->vm_file->f_dentry->d_name.name,
		  page, pgoff,
		  page_mapped(page), PageLocked(page),
		  PageDirty(page), PageWriteback(page));
	return -EIO;
}

/*
 * The given pte-mapped page doesn't match a VMA's policy. Allocate a new
 * page using the policy, copy contents from old to new, free
 * the old page, map in the new page. This is very similar to a COW.
 */
static int
copy_mapped_page(struct page * page, struct mempolicy *pol,
		 struct vm_area_struct *vma, unsigned long addr,
		 pmd_t *pmd)
{
	struct mm_struct *mm = vma->vm_mm;
	struct page * new_page;
	struct vm_area_struct pvma;
	pte_t *page_table;
	pte_t entry;
	
	if (!PageReserved(page))
		page_cache_get(page);
	spin_unlock(&mm->page_table_lock);
	if (unlikely(anon_vma_prepare(vma)))
		goto err_no_mem;

	/* Create a pseudo vma that just contains the policy */
	memset(&pvma, 0, sizeof(struct vm_area_struct));
	pvma.vm_end = PAGE_SIZE;
	pvma.vm_pgoff = vma->vm_pgoff;
	pvma.vm_policy = pol;
	new_page = alloc_page_vma(GFP_HIGHUSER, &pvma, addr);
	if (!new_page)
		goto err_no_mem;

	if (page == ZERO_PAGE(addr))
		clear_user_highpage(new_page, addr);
	else
		copy_user_highpage(new_page, page, addr);

	spin_lock(&mm->page_table_lock);
	page_table = pte_offset_map(pmd, addr);
	if (PageAnon(page))
		mm->anon_rss--;
	if (PageReserved(page))
		++mm->rss;
	else
		page_remove_rmap(page);
	
	flush_cache_page(vma, addr, page_to_pfn(page));
	entry = pte_mkdirty(mk_pte(new_page, vma->vm_page_prot));
	if (likely(vma->vm_flags & VM_WRITE))
		entry = pte_mkwrite(entry);
	ptep_establish(vma, addr, page_table, entry);
	update_mmu_cache(vma, addr, entry);
	lru_cache_add_active(new_page);
	page_add_anon_rmap(new_page, vma, addr);

	pte_unmap(page_table);
	page_cache_release(page); /* release our ref on the old page */
	page_cache_release(page); /* release our pte ref on the old page */

	PMDprintk("%s pte-mapped page %p:%lu copied, addr=%08lx\n",
		  PageAnon(page) ? "anonymous" :
		   (char *)vma->vm_file->f_dentry->d_name.name,
		  page, page_index(page), addr);
	return 0;

 err_no_mem:
	spin_lock(&mm->page_table_lock);
	return -ENOMEM;
}

/* Ensure all existing pages in a VMA follow the policy. */
static int
move_verify_pages(struct vm_area_struct *vma, unsigned long flags)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long addr;
	unsigned long start = vma->vm_start;
	unsigned long end = vma->vm_end;
	
	if (!(flags & (MPOL_MF_MOVE | MPOL_MF_STRICT)))
		return 0;

	for (addr = start; addr < end; addr += PAGE_SIZE) {
		struct mempolicy *pol;
		struct page *p;
		pte_t *pte;
		pmd_t *pmd;
		pgd_t *pgd;
		int err;
		
		pol = get_vma_policy(vma, addr);
		
		/*
		 * Let's see if there is a pte-mapped page that doesn't
		 * satisfy the policy. If so, release the old mapping
		 * and create a new mapping to a new page.
		 */
		spin_lock(&mm->page_table_lock);

		pgd = pgd_offset(mm, addr);
		if (pgd_none(*pgd))
			goto check_pagecache;
		pmd = pmd_offset(pgd, addr);
		if (pmd_none(*pmd))
			goto check_pagecache;
		p = NULL;
		pte = pte_offset_map(pmd, addr);
		if (pte_present(*pte))
			p = pte_page(*pte);
		pte_unmap(pte);
		if (p) {
			err = 0;
			if (!__mpol_node_valid(page_to_nid(p), pol)) {
				if (!(flags & MPOL_MF_MOVE))
					err = -EIO;
				else
					err = copy_mapped_page(p, pol, vma,
							       addr, pmd);
			}
			if (err && (flags & MPOL_MF_STRICT)) {
				spin_unlock(&mm->page_table_lock);
				return err;
			}
		}

	check_pagecache:
		spin_unlock(&mm->page_table_lock);

		/*
		 * if this is a file mapping, check for invalid page
		 * cache pages.
		 */
		if (vma->vm_ops && vma->vm_ops->nopage) {
			struct address_space *mapping =
				vma->vm_file->f_mapping;
			unsigned long pgoff =
				((addr - vma->vm_start) >> PAGE_CACHE_SHIFT) +
				vma->vm_pgoff;
			
			p = find_get_page(mapping, pgoff);
			if (p) {
				err = 0;
				if (!__mpol_node_valid(page_to_nid(p), pol)) {
					if (!(flags & MPOL_MF_MOVE))
						err = -EIO;
					else
						err = remove_filemap_page(
							p,vma,pgoff);
				}
				page_cache_release(p);  /* find_get_page */
				if (err && (flags & MPOL_MF_STRICT))
					return err;
			}
		}
		
	}

	return 0;
}

/* Step 1: check the range */
static struct vm_area_struct *
check_range(struct mm_struct *mm, unsigned long start, unsigned long end,
	    struct mempolicy *policy, unsigned long flags)
{
	struct vm_area_struct *first, *vma, *prev;

	first = find_vma(mm, start);
	if (!first)
		return ERR_PTR(-EFAULT);
	prev = NULL;
	for (vma = first; vma && vma->vm_start < end; vma = vma->vm_next) {
		if (!vma->vm_next && vma->vm_end < end)
			return ERR_PTR(-EFAULT);
		if (prev && prev->vm_end < vma->vm_start)
			return ERR_PTR(-EFAULT);
		prev = vma;
	}
	return first;
}

/* Apply policy to a single VMA */
static int policy_vma(struct vm_area_struct *vma, struct mempolicy *new,
		      unsigned long flags)
{
	int err = 0;
	struct mempolicy *old = vma->vm_policy;

	PDprintk("vma %lx-%lx/%lx vm_ops %p vm_file %p set_policy %p\n",
		 vma->vm_start, vma->vm_end, vma->vm_pgoff,
		 vma->vm_ops, vma->vm_file,
		 vma->vm_ops ? vma->vm_ops->set_policy : NULL);

	if (vma->vm_ops && vma->vm_ops->set_policy)
		err = vma->vm_ops->set_policy(vma, new, flags);
	if (!err) {
		mpol_get(new);
		vma->vm_policy = new;
		mpol_free(old);
		if (flags & (MPOL_MF_MOVE | MPOL_MF_STRICT))
			err = move_verify_pages(vma, flags);
	}
	return err;
}

/* Step 2: apply policy to a range and do splits. */
static int mbind_range(struct vm_area_struct *vma, unsigned long start,
		       unsigned long end, struct mempolicy *new,
		       unsigned long flags)
{
	struct vm_area_struct *next;
	int err;

	err = 0;
	for (; vma && vma->vm_start < end; vma = next) {
		next = vma->vm_next;
		if (vma->vm_start < start)
			err = split_vma(vma->vm_mm, vma, start, 1);
		if (!err && vma->vm_end > end)
			err = split_vma(vma->vm_mm, vma, end, 0);
		if (!err)
			err = policy_vma(vma, new, flags);
		if (err)
			break;
	}
	return err;
}

/* Change policy for a memory range */
asmlinkage long sys_mbind(unsigned long start, unsigned long len,
			  unsigned long mode,
			  unsigned long __user *nmask, unsigned long maxnode,
			  unsigned flags)
{
	struct vm_area_struct *vma;
	struct mm_struct *mm = current->mm;
	struct mempolicy *new;
	unsigned long end;
	DECLARE_BITMAP(nodes, MAX_NUMNODES);
	int err;

	if ((flags & ~(unsigned long)
	     (MPOL_MF_STRICT | MPOL_MF_MOVE | MPOL_MF_NOREPLACE)) ||
	    mode > MPOL_MAX)
		return -EINVAL;
	if (start & ~PAGE_MASK)
		return -EINVAL;
	if (mode == MPOL_DEFAULT)
		flags &= ~(MPOL_MF_STRICT | MPOL_MF_MOVE | MPOL_MF_NOREPLACE);
	len = (len + PAGE_SIZE - 1) & PAGE_MASK;
	end = start + len;
	if (end < start)
		return -EINVAL;
	if (end == start)
		return 0;

	err = get_nodes(nodes, nmask, maxnode, mode);
	if (err)
		return err;

	new = mpol_new(mode, nodes);
	if (IS_ERR(new))
		return PTR_ERR(new);

	PDprintk("mbind %lx-%lx mode:%ld nodes:%lx\n",start,start+len,
			mode,nodes[0]);

	down_write(&mm->mmap_sem);
	vma = check_range(mm, start, end, new, flags);
	err = PTR_ERR(vma);
	if (!IS_ERR(vma))
		err = mbind_range(vma, start, end, new, flags);
	up_write(&mm->mmap_sem);
	mpol_free(new);
	return err;
}

/* Set the process memory policy */
asmlinkage long sys_set_mempolicy(int mode, unsigned long __user *nmask,
				   unsigned long maxnode)
{
	int err;
	struct mempolicy *new;
	DECLARE_BITMAP(nodes, MAX_NUMNODES);

	if (mode > MPOL_MAX)
		return -EINVAL;
	err = get_nodes(nodes, nmask, maxnode, mode);
	if (err)
		return err;
	new = mpol_new(mode, nodes);
	if (IS_ERR(new))
		return PTR_ERR(new);
	mpol_free(current->mempolicy);
	current->mempolicy = new;
	if (new && new->policy == MPOL_INTERLEAVE)
		current->il_next = find_first_bit(new->v.nodes, MAX_NUMNODES);
	return 0;
}

/* Fill a zone bitmap for a policy */
static void get_zonemask(struct mempolicy *p, unsigned long *nodes)
{
	int i;

	bitmap_zero(nodes, MAX_NUMNODES);
	switch (p->policy) {
	case MPOL_BIND:
		for (i = 0; p->v.zonelist->zones[i]; i++)
			__set_bit(p->v.zonelist->zones[i]->zone_pgdat->node_id, nodes);
		break;
	case MPOL_DEFAULT:
		break;
	case MPOL_INTERLEAVE:
		bitmap_copy(nodes, p->v.nodes, MAX_NUMNODES);
		break;
	case MPOL_PREFERRED:
		/* or use current node instead of online map? */
		if (p->v.preferred_node < 0)
			bitmap_copy(nodes, nodes_addr(node_online_map), MAX_NUMNODES);
		else
			__set_bit(p->v.preferred_node, nodes);
		break;
	default:
		BUG();
	}
}

static int lookup_node(struct mm_struct *mm, unsigned long addr)
{
	struct page *p;
	int err;

	err = get_user_pages(current, mm, addr & PAGE_MASK, 1, 0, 0, &p, NULL);
	if (err >= 0) {
		err = page_to_nid(p);
		put_page(p);
	}
	return err;
}

/* Copy a kernel node mask to user space */
static int copy_nodes_to_user(unsigned long __user *mask, unsigned long maxnode,
			      void *nodes, unsigned nbytes)
{
	unsigned long copy = ALIGN(maxnode-1, 64) / 8;

	if (copy > nbytes) {
		if (copy > PAGE_SIZE)
			return -EINVAL;
		if (clear_user((char __user *)mask + nbytes, copy - nbytes))
			return -EFAULT;
		copy = nbytes;
	}
	return copy_to_user(mask, nodes, copy) ? -EFAULT : 0;
}

/* Retrieve NUMA policy */
asmlinkage long sys_get_mempolicy(int __user *policy,
				  unsigned long __user *nmask,
				  unsigned long maxnode,
				  unsigned long addr, unsigned long flags)
{
	int err, pval;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma = NULL;
	struct mempolicy *pol = current->mempolicy;

	if (flags & ~(unsigned long)(MPOL_F_NODE|MPOL_F_ADDR))
		return -EINVAL;
	if (nmask != NULL && maxnode < MAX_NUMNODES)
		return -EINVAL;
	if (flags & MPOL_F_ADDR) {
		down_read(&mm->mmap_sem);
		vma = find_vma_intersection(mm, addr, addr+1);
		if (!vma) {
			up_read(&mm->mmap_sem);
			return -EFAULT;
		}
		if (vma->vm_ops && vma->vm_ops->get_policy)
			pol = vma->vm_ops->get_policy(vma, addr);
		else
			pol = vma->vm_policy;
	} else if (addr)
		return -EINVAL;

	if (!pol)
		pol = &default_policy;

	if (flags & MPOL_F_NODE) {
		if (flags & MPOL_F_ADDR) {
			err = lookup_node(mm, addr);
			if (err < 0)
				goto out;
			pval = err;
		} else if (pol == current->mempolicy &&
				pol->policy == MPOL_INTERLEAVE) {
			pval = current->il_next;
		} else {
			err = -EINVAL;
			goto out;
		}
	} else
		pval = pol->policy;

	if (vma) {
		up_read(&current->mm->mmap_sem);
		vma = NULL;
	}

	if (policy && put_user(pval, policy))
		return -EFAULT;

	err = 0;
	if (nmask) {
		DECLARE_BITMAP(nodes, MAX_NUMNODES);
		get_zonemask(pol, nodes);
		err = copy_nodes_to_user(nmask, maxnode, nodes, sizeof(nodes));
	}

 out:
	if (vma)
		up_read(&current->mm->mmap_sem);
	return err;
}

#ifdef CONFIG_COMPAT

asmlinkage long compat_sys_get_mempolicy(int __user *policy,
				     compat_ulong_t __user *nmask,
				     compat_ulong_t maxnode,
				     compat_ulong_t addr, compat_ulong_t flags)
{
	long err;
	unsigned long __user *nm = NULL;
	unsigned long nr_bits, alloc_size;
	DECLARE_BITMAP(bm, MAX_NUMNODES);

	nr_bits = min_t(unsigned long, maxnode-1, MAX_NUMNODES);
	alloc_size = ALIGN(nr_bits, BITS_PER_LONG) / 8;

	if (nmask)
		nm = compat_alloc_user_space(alloc_size);

	err = sys_get_mempolicy(policy, nm, nr_bits+1, addr, flags);

	if (!err && nmask) {
		err = copy_from_user(bm, nm, alloc_size);
		/* ensure entire bitmap is zeroed */
		err |= clear_user(nmask, ALIGN(maxnode-1, 8) / 8);
		err |= compat_put_bitmap(nmask, bm, nr_bits);
	}

	return err;
}

asmlinkage long compat_sys_set_mempolicy(int mode, compat_ulong_t __user *nmask,
				     compat_ulong_t maxnode)
{
	long err = 0;
	unsigned long __user *nm = NULL;
	unsigned long nr_bits, alloc_size;
	DECLARE_BITMAP(bm, MAX_NUMNODES);

	nr_bits = min_t(unsigned long, maxnode-1, MAX_NUMNODES);
	alloc_size = ALIGN(nr_bits, BITS_PER_LONG) / 8;

	if (nmask) {
		err = compat_get_bitmap(bm, nmask, nr_bits);
		nm = compat_alloc_user_space(alloc_size);
		err |= copy_to_user(nm, bm, alloc_size);
	}

	if (err)
		return -EFAULT;

	return sys_set_mempolicy(mode, nm, nr_bits+1);
}

asmlinkage long compat_sys_mbind(compat_ulong_t start, compat_ulong_t len,
			     compat_ulong_t mode, compat_ulong_t __user *nmask,
			     compat_ulong_t maxnode, compat_ulong_t flags)
{
	long err = 0;
	unsigned long __user *nm = NULL;
	unsigned long nr_bits, alloc_size;
	DECLARE_BITMAP(bm, MAX_NUMNODES);

	nr_bits = min_t(unsigned long, maxnode-1, MAX_NUMNODES);
	alloc_size = ALIGN(nr_bits, BITS_PER_LONG) / 8;

	if (nmask) {
		err = compat_get_bitmap(bm, nmask, nr_bits);
		nm = compat_alloc_user_space(alloc_size);
		err |= copy_to_user(nm, bm, alloc_size);
	}

	if (err)
		return -EFAULT;

	return sys_mbind(start, len, mode, nm, nr_bits+1, flags);
}

#endif

/* Return effective policy for a VMA */
static struct mempolicy *
get_vma_policy(struct vm_area_struct *vma, unsigned long addr)
{
	struct mempolicy *pol = current->mempolicy;

	if (vma) {
		if (vma->vm_ops && vma->vm_ops->get_policy)
		        pol = vma->vm_ops->get_policy(vma, addr);
		else if (vma->vm_policy &&
				vma->vm_policy->policy != MPOL_DEFAULT)
			pol = vma->vm_policy;
	}
	if (!pol)
		pol = &default_policy;
	return pol;
}

/* Return a zonelist representing a mempolicy */
static struct zonelist *zonelist_policy(unsigned gfp, struct mempolicy *policy)
{
	int nd;

	switch (policy->policy) {
	case MPOL_PREFERRED:
		nd = policy->v.preferred_node;
		if (nd < 0)
			nd = numa_node_id();
		break;
	case MPOL_BIND:
		/* Lower zones don't get a policy applied */
		if (gfp >= policy_zone)
			return policy->v.zonelist;
		/*FALL THROUGH*/
	case MPOL_INTERLEAVE: /* should not happen */
	case MPOL_DEFAULT:
		nd = numa_node_id();
		break;
	default:
		nd = 0;
		BUG();
	}
	return NODE_DATA(nd)->node_zonelists + (gfp & GFP_ZONEMASK);
}

/* Do dynamic interleaving for a process */
static unsigned interleave_nodes(struct mempolicy *policy)
{
	unsigned nid, next;
	struct task_struct *me = current;

	nid = me->il_next;
	BUG_ON(nid >= MAX_NUMNODES);
	next = find_next_bit(policy->v.nodes, MAX_NUMNODES, 1+nid);
	if (next >= MAX_NUMNODES)
		next = find_first_bit(policy->v.nodes, MAX_NUMNODES);
	me->il_next = next;
	return nid;
}

/* Do static interleaving for a VMA with known offset. */
static unsigned offset_il_node(struct mempolicy *pol,
		struct vm_area_struct *vma, unsigned long off)
{
	unsigned nnodes = bitmap_weight(pol->v.nodes, MAX_NUMNODES);
	unsigned target = (unsigned)off % nnodes;
	int c;
	int nid = -1;

	c = 0;
	do {
		nid = find_next_bit(pol->v.nodes, MAX_NUMNODES, nid+1);
		c++;
	} while (c <= target);
	BUG_ON(nid >= MAX_NUMNODES);
	BUG_ON(!test_bit(nid, pol->v.nodes));
	return nid;
}

/* Allocate a page in interleaved policy.
   Own path because it needs to do special accounting. */
static struct page *alloc_page_interleave(unsigned gfp, unsigned order, unsigned nid)
{
	struct zonelist *zl;
	struct page *page;

	BUG_ON(!node_online(nid));
	zl = NODE_DATA(nid)->node_zonelists + (gfp & GFP_ZONEMASK);
	page = __alloc_pages(gfp, order, zl);
	if (page && page_zone(page) == zl->zones[0]) {
		zl->zones[0]->pageset[get_cpu()].interleave_hit++;
		put_cpu();
	}
	return page;
}


/**
 * 	alloc_page_vma	- Allocate a page for a VMA.
 *
 * 	@gfp:
 *      %GFP_USER    user allocation.
 *      %GFP_KERNEL  kernel allocations,
 *      %GFP_HIGHMEM highmem/user allocations,
 *      %GFP_FS      allocation should not call back into a file system.
 *      %GFP_ATOMIC  don't sleep.
 *
 * 	@vma:  Pointer to VMA or NULL if not available.
 *	@addr: Virtual Address of the allocation. Must be inside the VMA.
 *
 * 	This function allocates a page from the kernel page pool and applies
 *	a NUMA policy associated with the VMA or the current process.
 *	When VMA is not NULL caller must hold down_read on the mmap_sem of the
 *	mm_struct of the VMA to prevent it from going away. Should be used for
 *	all allocations for pages that will be mapped into
 * 	user space. Returns NULL when no page can be allocated.
 *
 *	Should be called with the mm_sem of the vma hold.
 */
struct page *
alloc_page_vma(unsigned gfp, struct vm_area_struct *vma, unsigned long addr)
{
	struct mempolicy *pol = get_vma_policy(vma, addr);

	if (unlikely(pol->policy == MPOL_INTERLEAVE)) {
		unsigned nid;
		if (vma) {
			unsigned long off;
			BUG_ON(addr >= vma->vm_end);
			BUG_ON(addr < vma->vm_start);
			off = vma->vm_pgoff;
			off += (addr - vma->vm_start) >> PAGE_SHIFT;
			nid = offset_il_node(pol, vma, off);
		} else {
			/* fall back to process interleaving */
			nid = interleave_nodes(pol);
		}
		return alloc_page_interleave(gfp, 0, nid);
	}
	return __alloc_pages(gfp, 0, zonelist_policy(gfp, pol));
}

/**
 * 	alloc_pages_current - Allocate pages.
 *
 *	@gfp:
 *		%GFP_USER   user allocation,
 *      	%GFP_KERNEL kernel allocation,
 *      	%GFP_HIGHMEM highmem allocation,
 *      	%GFP_FS     don't call back into a file system.
 *      	%GFP_ATOMIC don't sleep.
 *	@order: Power of two of allocation size in pages. 0 is a single page.
 *
 *	Allocate a page from the kernel page pool.  When not in
 *	interrupt context and apply the current process NUMA policy.
 *	Returns NULL when no page can be allocated.
 */
struct page *alloc_pages_current(unsigned gfp, unsigned order)
{
	struct mempolicy *pol = current->mempolicy;

	if (!pol || in_interrupt())
		pol = &default_policy;
	if (pol->policy == MPOL_INTERLEAVE)
		return alloc_page_interleave(gfp, order, interleave_nodes(pol));
	return __alloc_pages(gfp, order, zonelist_policy(gfp, pol));
}
EXPORT_SYMBOL(alloc_pages_current);

/* Slow path of a mempolicy copy */
struct mempolicy *__mpol_copy(struct mempolicy *old)
{
	struct mempolicy *new = kmem_cache_alloc(policy_cache, GFP_KERNEL);

	if (!new)
		return ERR_PTR(-ENOMEM);
	*new = *old;
	atomic_set(&new->refcnt, 1);
	if (new->policy == MPOL_BIND) {
		int sz = ksize(old->v.zonelist);
		new->v.zonelist = kmalloc(sz, SLAB_KERNEL);
		if (!new->v.zonelist) {
			kmem_cache_free(policy_cache, new);
			return ERR_PTR(-ENOMEM);
		}
		memcpy(new->v.zonelist, old->v.zonelist, sz);
	}
	return new;
}

/* Slow path of a mempolicy comparison */
int __mpol_equal(struct mempolicy *a, struct mempolicy *b)
{
	if (!a || !b)
		return 0;
	if (a->policy != b->policy)
		return 0;
	switch (a->policy) {
	case MPOL_DEFAULT:
		return 1;
	case MPOL_INTERLEAVE:
		return bitmap_equal(a->v.nodes, b->v.nodes, MAX_NUMNODES);
	case MPOL_PREFERRED:
		return a->v.preferred_node == b->v.preferred_node;
	case MPOL_BIND: {
		int i;
		for (i = 0; a->v.zonelist->zones[i]; i++)
			if (a->v.zonelist->zones[i] != b->v.zonelist->zones[i])
				return 0;
		return b->v.zonelist->zones[i] == NULL;
	}
	default:
		BUG();
		return 0;
	}
}

/* Slow path of a mpol destructor. */
void __mpol_free(struct mempolicy *p)
{
	if (!atomic_dec_and_test(&p->refcnt))
		return;
	if (p->policy == MPOL_BIND)
		kfree(p->v.zonelist);
	p->policy = MPOL_DEFAULT;
	kmem_cache_free(policy_cache, p);
}

/*
 * Hugetlb policy. Same as above, just works with node numbers instead of
 * zonelists.
 */

/* Find first node suitable for an allocation */
int mpol_first_node(struct vm_area_struct *vma, unsigned long addr)
{
	struct mempolicy *pol = get_vma_policy(vma, addr);

	switch (pol->policy) {
	case MPOL_DEFAULT:
		return numa_node_id();
	case MPOL_BIND:
		return pol->v.zonelist->zones[0]->zone_pgdat->node_id;
	case MPOL_INTERLEAVE:
		return interleave_nodes(pol);
	case MPOL_PREFERRED:
		return pol->v.preferred_node >= 0 ?
				pol->v.preferred_node : numa_node_id();
	}
	BUG();
	return 0;
}

/* Find secondary valid nodes for an allocation */
static int __mpol_node_valid(int nid, struct mempolicy *pol)
{
	switch (pol->policy) {
	case MPOL_PREFERRED:
	case MPOL_DEFAULT:
	case MPOL_INTERLEAVE:
		return 1;
	case MPOL_BIND: {
		struct zone **z;
		for (z = pol->v.zonelist->zones; *z; z++)
			if ((*z)->zone_pgdat->node_id == nid)
				return 1;
		return 0;
	}
	default:
		BUG();
		return 0;
	}
}

int mpol_node_valid(int nid, struct vm_area_struct *vma, unsigned long addr)
{
	return __mpol_node_valid(nid, get_vma_policy(vma, addr));
}

/*
 * Shared memory backing store policy support.
 *
 * Remember policies even when nobody has shared memory mapped.
 * The policies are kept in Red-Black tree linked from the inode.
 * They are protected by the sp->lock spinlock, which should be held
 * for any accesses to the tree.
 */

/* lookup first element intersecting start-end */
/* Caller holds sp->lock */
static struct sp_node *
sp_lookup(struct shared_policy *sp, unsigned long start, unsigned long end)
{
	struct rb_node *n = sp->root.rb_node;

	while (n) {
		struct sp_node *p = rb_entry(n, struct sp_node, nd);
		if (start >= p->end)
			n = n->rb_right;
		else if (end <= p->start)
			n = n->rb_left;
		else
			break;
	}
	if (!n)
		return NULL;
	for (;;) {
		struct sp_node *w = NULL;
		struct rb_node *prev = rb_prev(n);
		if (!prev)
			break;
		w = rb_entry(prev, struct sp_node, nd);
		if (w->end <= start)
			break;
		n = prev;
	}
	return rb_entry(n, struct sp_node, nd);
}

/* Insert a new shared policy into the list. */
/* Caller holds sp->lock */
static void sp_insert(struct shared_policy *sp, struct sp_node *new)
{
	struct rb_node **p = &sp->root.rb_node;
	struct rb_node *parent = NULL;
	struct sp_node *nd;

	while (*p) {
		parent = *p;
		nd = rb_entry(parent, struct sp_node, nd);
		if (new->start < nd->start)
			p = &(*p)->rb_left;
		else if (new->end > nd->end)
			p = &(*p)->rb_right;
		else
			BUG();
	}
	rb_link_node(&new->nd, parent, p);
	rb_insert_color(&new->nd, &sp->root);
	PDprintk("inserting %lx-%lx: %d\n", new->start, new->end,
		 new->policy ? new->policy->policy : 0);
}

/* Find shared policy intersecting idx */
struct mempolicy *
mpol_shared_policy_lookup(struct shared_policy *sp, unsigned long idx)
{
	struct mempolicy *pol = NULL;
	struct sp_node *sn;

	if (!sp->root.rb_node)
		return NULL;
	spin_lock(&sp->lock);
	sn = sp_lookup(sp, idx, idx+1);
	if (sn) {
		mpol_get(sn->policy);
		pol = sn->policy;
	}
	spin_unlock(&sp->lock);
	return pol;
}

static void sp_delete(struct shared_policy *sp, struct sp_node *n)
{
	PDprintk("deleting %lx-l%x\n", n->start, n->end);
	rb_erase(&n->nd, &sp->root);
	mpol_free(n->policy);
	kmem_cache_free(sn_cache, n);
}

struct sp_node *
sp_alloc(unsigned long start, unsigned long end, struct mempolicy *pol)
{
	struct sp_node *n = kmem_cache_alloc(sn_cache, GFP_KERNEL);

	if (!n)
		return NULL;
	n->start = start;
	n->end = end;
	mpol_get(pol);
	n->policy = pol;
	return n;
}

/* Insert a policy range. */
static int shared_policy_insert(struct shared_policy *sp, unsigned long start,
				unsigned long end, struct sp_node *new,
				unsigned long flags)
{
	struct sp_node *n, *new2 = NULL;

restart:
	spin_lock(&sp->lock);
	n = sp_lookup(sp, start, end);
	/* Take care of old policies in the same range. */
	while (n && n->start < end) {
		struct rb_node *next = rb_next(&n->nd);
		if (n->start >= start) {
			if (n->end <= end) {
				if (!(flags & MPOL_MF_NOREPLACE))
					sp_delete(sp, n);
				else if (start == n->start && end == n->end)
					goto out; /* drop */
				else if (start == n->start)
					start = n->end;
				else if (end == n->end)
					end = n->start;
				else {
					if (!new2) {
						spin_unlock(&sp->lock);
						new2 = sp_alloc(n->end, end,
								new->policy);
						if (!new2)
							return -ENOMEM;
						goto restart;
					}
					end = n->start;
					sp_insert(sp, new2);
					new2 = NULL;
					break;
				}
			} else {
				if (!(flags & MPOL_MF_NOREPLACE))
					n->start = end;
				else if (start == n->start)
					goto out; /* drop */
				else
					end = n->start;
			}
		} else {
			/* Old policy spanning whole new range. */
			if (n->end > end) {
				if (flags & MPOL_MF_NOREPLACE)
					goto out; /* drop */
				if (!new2) {
					spin_unlock(&sp->lock);
					new2 = sp_alloc(end, n->end, n->policy);
					if (!new2)
						return -ENOMEM;
					goto restart;
				}
				n->end = start;
				sp_insert(sp, new2);
				new2 = NULL;
				break;
			} else {
				if (!(flags & MPOL_MF_NOREPLACE))
					n->end = start;
				else if (end == n->end)
					goto out; /* drop */
				else
					start = n->end;
			}
		}
		if (!next)
			break;
		n = rb_entry(next, struct sp_node, nd);
	}
	if (new) {
		if (flags & MPOL_MF_NOREPLACE) {
			/* start,end may have changed */
			new->start = start;
			new->end = end;
		}
		sp_insert(sp, new);
	}
 out:
	spin_unlock(&sp->lock);
	if (new2) {
		mpol_free(new2->policy);
		kmem_cache_free(sn_cache, new2);
	}
	return 0;
}


#ifdef DEBUG_SHARED_POLICY
/* useful debug for the shared policy tree */
static void dump_sp(struct rb_node * node)
{
	struct sp_node * sn;
	
	if (!node) {
		printk(KERN_DEBUG "LEAF\n");
		return;
	}
	
	sn = rb_entry(node, struct sp_node, nd);
	printk(KERN_DEBUG "%p: start=%ld, end=%ld, pol=%p, color=%s, "
	       "l=%p, r=%p\n",
	       node, sn->start, sn->end, sn->policy,
	       node->rb_color == RB_RED ? "RED" : "BLACK",
	       node->rb_left, node->rb_right);

	if (node->rb_left)
		dump_sp(node->rb_left);
	if (node->rb_right)
		dump_sp(node->rb_right);
}
#endif

int mpol_set_shared_policy(struct shared_policy *info,
			   struct vm_area_struct *vma,
			   struct mempolicy *npol,
			   unsigned long flags)
{
	int err;
	struct sp_node *new = NULL;
	unsigned long sz = vma_pages(vma);

	PDprintk("set_shared_policy %lx sz %lu %d %lx\n",
		 vma->vm_pgoff,
		 sz, npol? npol->policy : -1,
		npol ? npol->v.nodes[0] : -1);

	if (npol) {
		new = sp_alloc(vma->vm_pgoff, vma->vm_pgoff + sz, npol);
		if (!new)
			return -ENOMEM;
	}
	
#ifdef DEBUG_SHARED_POLICY
	printk(KERN_DEBUG "%s: BEFORE, want %ld-->%ld:\n",
	       vma->vm_file->f_dentry->d_name.name,
	       vma->vm_pgoff, vma->vm_pgoff+sz);
	dump_sp(info->root.rb_node);
#endif
	
	err = shared_policy_insert(info, vma->vm_pgoff, vma->vm_pgoff+sz,
				   new, flags);

#ifdef DEBUG_SHARED_POLICY
	printk(KERN_DEBUG "%s: AFTER:\n", vma->vm_file->f_dentry->d_name.name);
	dump_sp(info->root.rb_node);
#endif
	
	if (err && new)
		kmem_cache_free(sn_cache, new);
	return err;
}

/* Free a backing policy store on inode delete. */
void mpol_free_shared_policy(struct shared_policy *p)
{
	struct sp_node *n;
	struct rb_node *next;

	if (!p->root.rb_node)
		return;
	spin_lock(&p->lock);
	next = rb_first(&p->root);
	while (next) {
		n = rb_entry(next, struct sp_node, nd);
		next = rb_next(&n->nd);
		mpol_free(n->policy);
		kmem_cache_free(sn_cache, n);
	}
	spin_unlock(&p->lock);
	p->root = RB_ROOT;
}

struct page *
alloc_page_shared_policy(unsigned gfp, struct shared_policy *sp,
			 unsigned long idx)
{
	struct page *page;

	if (sp->root.rb_node) {
		struct vm_area_struct pvma;
		/* Create a pseudo vma that just contains the policy */
		memset(&pvma, 0, sizeof(struct vm_area_struct));
		pvma.vm_end = PAGE_SIZE;
		pvma.vm_pgoff = idx;
		pvma.vm_policy = mpol_shared_policy_lookup(sp, idx);
		page = alloc_page_vma(gfp, &pvma, 0);
		mpol_free(pvma.vm_policy);
	} else {
		page = alloc_pages(gfp, 0);
	}

	return page;
}

/* assumes fs == KERNEL_DS */
void __init numa_policy_init(void)
{
	policy_cache = kmem_cache_create("numa_policy",
					 sizeof(struct mempolicy),
					 0, SLAB_PANIC, NULL, NULL);

	sn_cache = kmem_cache_create("shared_policy_node",
				     sizeof(struct sp_node),
				     0, SLAB_PANIC, NULL, NULL);

	/* Set interleaving policy for system init. This way not all
	   the data structures allocated at system boot end up in node zero. */

	if (sys_set_mempolicy(MPOL_INTERLEAVE, nodes_addr(node_online_map),
							MAX_NUMNODES) < 0)
		printk("numa_policy_init: interleaving failed\n");
}

/* Reset policy of current process to default.
 * Assumes fs == KERNEL_DS */
void numa_default_policy(void)
{
	sys_set_mempolicy(MPOL_DEFAULT, NULL, 0);
}
