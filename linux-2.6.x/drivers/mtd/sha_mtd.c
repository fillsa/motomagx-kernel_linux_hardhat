/*
 *  drivers/mtd/sha_mtd.c
 *
 *	Copyright (C) 2007-2008 Motorola, Inc.
 *
 * This file contains code to implement shasum based security checks when
 * CONFIG_MOT_FEAT_MTD_SHA is enabled 
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
 *
 * ChangeLog:
 * (mm-dd-yyyy) Author    Comment
 * 09-20-2007   Motorola  implemented CONFIG_MOT_FEAT_MTD_HASH feature.
 *                        shasum based security checks in kernel.
 * 01-07-2008   Motorola  Sync the enum name of KERNEL_BACKUP_MEMORY_ID_ROOTFS_SEC_FAIL 
 *                        and KERNEL_BACKUP_MEMORY_ID_LANG_PACK_SEC_FAIL with power-ic-api.h
 * 
 */

#include <linux/types.h>
#ifdef CONFIG_MOT_POWER_IC_ATLAS
#include <linux/power_ic_kernel.h>
#elif CONFIG_MOT_FEAT_POWER_IC_API
#include <asm-arm/power-ic-api.h>
#else
#error POWER_IC_are you shure?
#endif /* CONFIG_MOT_POWER_IC_ATLAS */
#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>
#include <linux/mtd/mtd-sha.h>
#include <linux/crypto.h>
#include <asm/scatterlist.h>
#include <linux/bitmap.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>


#define SEC_HASH_PAGE_SIZE     512*5
#define SEC_HASH_SHA1_PER_PAGE (SEC_HASH_PAGE_SIZE/SIZE_OF_SHA_SUM)

static struct sec_hash_page
{
	unsigned             code_group;
	unsigned             hash_no;
	char                 page[SEC_HASH_PAGE_SIZE];
	struct sec_hash_page *next;
	struct sec_hash_page *prev;
} *sec_hash_pages,*sec_hash_lru_head,*sec_hash_lru_tail,*sec_hash_free;


static struct sec_hash_code_group
{
	unsigned                   code_group;
	unsigned                   no_of_hashes;
	unsigned                   start_page_no;
	struct sec_hash_page       **ptrs;
	struct sec_hash_code_group *next;
} *sec_hash_code_groups;

/*
 * mtdblock_write_to_atlas_and_panic function is used to panic 
 * incase security validation fails.
 * A Atlas register bit is set which will cause Motorola boot module
 * to remain in flash mode.
 */
void mtdblock_write_to_atlas_and_panic(struct mtd_info *mtd)
{
	printk("INFO :: mtdblock_write_to_atlas_and_panic called\n");
	if(mtd == NULL) {
#ifdef CONFIG_MOT_POWER_IC_ATLAS
		power_ic_backup_memory_write(
				POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_ROOTFS_SEC_FAIL
#elif CONFIG_MOT_FEAT_POWER_IC_API
		kernel_power_ic_backup_memory_write(
				KERNEL_BACKUP_MEMORY_ID_ROOTFS_SEC_FAIL
#endif
									, 1);
		panic("Panic: Generic mtdblock security module failure\n");
	} else if((strncmp("root", mtd->name, 4)) == 0 ) {
#ifdef CONFIG_MOT_POWER_IC_ATLAS
		power_ic_backup_memory_write(
				POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_ROOTFS_SEC_FAIL
#elif CONFIG_MOT_FEAT_POWER_IC_API
		kernel_power_ic_backup_memory_write(
				KERNEL_BACKUP_MEMORY_ID_ROOTFS_SEC_FAIL
#endif
									, 1);
		panic("Panic: Rootfs securiy verification failure\n");
	} else if((strncmp("lang", mtd->name, 4)) == 0 ) {
#ifdef CONFIG_MOT_POWER_IC_ATLAS
		power_ic_backup_memory_write(
				POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_LANG_PACK_SEC_FAIL
#elif CONFIG_MOT_FEAT_POWER_IC_API
		kernel_power_ic_backup_memory_write(
				KERNEL_BACKUP_MEMORY_ID_LANG_PACK_SEC_FAIL
#endif
									, 1);
		panic("Panic: lang pack securiy verification failure\n");
	}

#ifdef CONFIG_MOT_POWER_IC_ATLAS
		power_ic_backup_memory_write(
				POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_ROOTFS_SEC_FAIL
#elif CONFIG_MOT_FEAT_POWER_IC_API
	kernel_power_ic_backup_memory_write(
			KERNEL_BACKUP_MEMORY_ID_ROOTFS_SEC_FAIL
#endif
									, 1);
	panic("Panic: Unknown mtdblock security module failure\n");
}

/*
 * sec_hash_lru_init - LRU init function. 
 * Sets up initial structures and allocates memory based on 
 * number_of_cached_pages specified. This needs to be called only once
 * during bootup.
 */
void sec_hash_lru_init(unsigned number_of_cached_pages)
{
	unsigned idx;

	sec_hash_pages = (struct sec_hash_page *)vmalloc(sizeof(struct sec_hash_page)*number_of_cached_pages);
	if(!sec_hash_pages) {
		panic("ERROR :: LRU :: Could not allocate mem for sec_hash_pages, panic\n");
	}
	
	for(idx=0;idx < (number_of_cached_pages-1);++idx) {
		sec_hash_pages[idx].next = &(sec_hash_pages[idx+1]);
	}
	sec_hash_pages[number_of_cached_pages-1].next = 0;

	for(idx=1;idx < number_of_cached_pages;++idx) {
		sec_hash_pages[idx].prev= &(sec_hash_pages[idx-1]);
	}

	sec_hash_pages[0].prev= 0;
	sec_hash_free = sec_hash_pages;
	sec_hash_lru_head = 0;
	sec_hash_lru_tail = 0;
	sec_hash_code_groups = 0;
}

/* 
 * sec_hash_add_code_group - LRU setup function. 
 * This is used to add a codegroup (aka mtd partition)
 * into the LRU scheme. It needs to be called only once per codegroup that
 * has mtd sha feature turned on. There is a check for duplicate calls.
 */
void sec_hash_add_code_group ( unsigned code_group_number,
						 unsigned no_of_hashes,
						 unsigned start_page_no )
{
	struct sec_hash_code_group *new_code_group;
	struct sec_hash_code_group *dup_code_group;
	unsigned no_page_sha1_pages;
	unsigned idx;

	dup_code_group = sec_hash_code_groups;
	if(dup_code_group != 0) {
		do {
			if(dup_code_group->code_group == code_group_number) {
				return;
			}
			dup_code_group = dup_code_group->next;
		} while(dup_code_group != 0);
	}

	new_code_group = (struct sec_hash_code_group *)vmalloc(sizeof(struct sec_hash_code_group));
	if (!new_code_group) {
		panic("ERROR :: LRU :: Could not allocated new_code_group\n");
	}

	new_code_group->code_group=code_group_number;
	new_code_group->no_of_hashes=no_of_hashes;
	new_code_group->next=0;
	new_code_group->start_page_no=start_page_no;
	no_page_sha1_pages = no_of_hashes/SEC_HASH_SHA1_PER_PAGE;
	if(no_of_hashes%SEC_HASH_SHA1_PER_PAGE != 0) {
		++no_page_sha1_pages;
	}
	new_code_group->ptrs = (struct sec_hash_page **)vmalloc(sizeof(struct sec_hash_page *)*no_page_sha1_pages);
	if (!(new_code_group->ptrs)) {
		panic("ERROR :: LRU :: Could not allocated new_code_group->ptrs\n");
	}

	for(idx=0;idx<no_page_sha1_pages;++idx) {
		new_code_group->ptrs[idx] = 0;
	}

	if(sec_hash_code_groups == 0) {
		sec_hash_code_groups = new_code_group;
	}
	else {
		struct sec_hash_code_group *last;
		for(last=sec_hash_code_groups;last->next != 0;last = last->next)
			/* Nothing to do here */;
		last->next = new_code_group;
	}
}

/*
 * sec_hash_touch_page - This function moves a page to the 
 * head of the link list. 
 */
static void sec_hash_touch_page(struct sec_hash_page *page)
{
	if(page!=sec_hash_lru_head) {
		if(page==sec_hash_lru_tail) {
			if(page->prev != 0) {
				sec_hash_lru_tail=page->prev;
			}
		}

		if(page->prev != 0) {
			page->prev->next = page->next;
		}
		if(page->next != 0) {
			page->next->prev = page->prev;
		}
		page->next = sec_hash_lru_head;
		sec_hash_lru_head->prev = page;
		sec_hash_lru_head = page;
	}
}

/* 
 * sech_hash_page - This function is used to read in a page worths of 
 * SHA hashes from the flash. It will update the link list and move
 * the recently read page to the head of the list.
 */
static void sec_hash_page(struct mtd_info *mtd,unsigned code_group,unsigned hash_no)
{
	struct sec_hash_page *to_page=0;
	struct sec_hash_code_group *cg=0;
	int page_index;
	size_t retlen;
	int ret;

	if(sec_hash_free!=0) {
		to_page = sec_hash_free;
		sec_hash_free = to_page->next;

		if(sec_hash_free != 0) {
			sec_hash_free->prev = 0;
		}
		to_page->prev = 0;

		if(sec_hash_lru_head != 0) {
			to_page->next = sec_hash_lru_head;
			sec_hash_lru_head->prev = to_page;
			sec_hash_lru_head = to_page;
		} else {
			to_page->next = 0;
			sec_hash_lru_head = to_page;
			sec_hash_lru_tail = to_page;
		}
	} else {
		to_page = sec_hash_lru_tail;
		sec_hash_touch_page(to_page);
		page_index=to_page->hash_no/SEC_HASH_SHA1_PER_PAGE;

		for(cg = sec_hash_code_groups;cg!=0;cg=cg->next) {
			if(cg->code_group == to_page->code_group) {
				cg->ptrs[page_index] = 0;
				break;
			}
		}
	}
	page_index = hash_no/SEC_HASH_SHA1_PER_PAGE;

	to_page->hash_no = page_index*SEC_HASH_SHA1_PER_PAGE;
	to_page->code_group = code_group;


	for(cg = sec_hash_code_groups;cg!=0;cg=cg->next) {
		if(cg->code_group == code_group) {
			unsigned start_page_no;

			cg->ptrs[page_index] = to_page;
			start_page_no=cg->start_page_no+(page_index*SEC_HASH_PAGE_SIZE);
			ret = MTD_READ(mtd, start_page_no,SEC_HASH_PAGE_SIZE,&retlen,to_page->page);
			if (ret || (retlen != (SEC_HASH_PAGE_SIZE))) {
				printk("ERROR :: LRU :: MTD_READ error will panic\n");
				mtdblock_write_to_atlas_and_panic(mtd);
			}
			break;
		}
	}
}

/*
 * sec_hash_read_hash - This function will return a shasum for the 
 * specified codegroup. If the page exist in a LRU cache it will 
 * touch the page to move it to the head of the linked list and then
 * return it from the cache. Else it will  use sec_hash_page to read 
 * it in.
 */
int sec_hash_read_hash(struct mtd_info *mtd,
		unsigned code_group_number,unsigned hash_no,char *out_value)
{
	struct sec_hash_code_group *hash_ptr;
	int return_value=0;

	for(hash_ptr=sec_hash_code_groups;hash_ptr != 0;hash_ptr = hash_ptr->next) {
		if(hash_ptr->code_group == code_group_number) {
			int page_index=hash_no/SEC_HASH_SHA1_PER_PAGE;
			int page_offset=hash_no%SEC_HASH_SHA1_PER_PAGE;

			if(hash_ptr->ptrs[page_index] == 0) {
				/* page fault */
				sec_hash_page(mtd,code_group_number,hash_no);
			} else {
				/* update LRU */
				sec_hash_touch_page(hash_ptr->ptrs[page_index]);
			}

			if(hash_ptr->ptrs[page_index] != 0) {
				memcpy(out_value,&(hash_ptr->ptrs[page_index]->page[page_offset*SIZE_OF_SHA_SUM]),
						SIZE_OF_SHA_SUM);
				return_value = 1;
			}
			break;
		}
	}
	return return_value;
}

/* 
 * mtdblock_hdt_sanity_check - performs basic sanity checks
 * on the HDT values.
 */
void mtdblock_hdt_sanity_check(struct mtdblk_sha *sha_state)
{
	/* Motorola boot module needs to catch these actually,
	 * so if this code is triggered there is a defect there */ 
	if(sha_state->hdt.hdt_barker != HDT_BARKER) {
		printk("ERROR :: MTD_SHA :: Invalid hdt barker, panic\n");
		mtdblock_write_to_atlas_and_panic(NULL);
	}
	if((SIZE_OF_SHA_SUM * sha_state->hdt.number_of_chunks) + 
			sizeof(struct mtdblk_hdt) > sha_state->hdt.sec_hash_block_size){
		printk("ERROR :: MTD_SHA :: Sec hash block size too small, panic\n");
		mtdblock_write_to_atlas_and_panic(NULL);
	}
}

/* 
 * mtdblock_setup_crypto will do allocation for using the kernel crypto APIs 
 */
void mtdblock_setup_crypto(struct mtdblk_sha *sha_state, struct mtd_info *mtd, unsigned int cache_size)
{
	unsigned int numbits;
	unsigned int numbytes;

	sha_state->crypto_tfm = crypto_alloc_tfm("sha1", 0);
	if (!sha_state->crypto_tfm) {
		panic("ERROR :: MTD_SHA :: Could not allocate for crypto\n");
	}

	/* numbits indicates number of chunks we can validate */
	numbits = (mtd->size + cache_size - 1)
			/ cache_size;
	numbytes = ((numbits + BITS_PER_LONG - 1) / BITS_PER_LONG)
			* sizeof(unsigned long);
	sha_state->sha_hash_bitmap = vmalloc(numbytes);
	if (!sha_state->sha_hash_bitmap) {
		panic("ERROR :: MTD_SHA :: Could not allocate for sha_state->sha_hash_bitmap\n");
	}
	memset(sha_state->sha_hash_bitmap, 0, numbytes);
}

/* 
 * mtdblock_verify_sha_sum will calculate shasum on the data read from NAND and will
 * compare it with the sha sum stored on flash. It will error out if they do not
 * match
 */
void mtdblock_verify_sha_sum(struct mtd_info *mtd, 
		struct mtdblk_sha *sha_state, unsigned char *cache_data, 
		unsigned int cache_size,unsigned long pos)
{
	struct scatterlist sg;
	uint8_t digest[SIZE_OF_SHA_SUM];
	int i;
	int ret;
	unsigned int request_index=(pos/sha_state->hdt.chunk_size);
	uint8_t shasum[SIZE_OF_SHA_SUM];

	ret = sec_hash_read_hash(mtd, sha_state->code_group, request_index, shasum); 
	if (ret == 0) {
		printk("ERROR :: LRU :: sec_hash_read_hash failed\n");
		mtdblock_write_to_atlas_and_panic(mtd);
	}

	memset((void *)digest, 0xFF, SIZE_OF_SHA_SUM);
	crypto_digest_init(sha_state->crypto_tfm);
	sg.page = virt_to_page(cache_data);
	sg.offset = offset_in_page(cache_data);
	sg.length = cache_size;
	crypto_digest_update(sha_state->crypto_tfm, &sg, 1);
	crypto_digest_final(sha_state->crypto_tfm, digest);
       	for (i = 0; i < SIZE_OF_SHA_SUM; ++i)
		if ( digest[i] != shasum[i] ) {
			printk("ERROR :: shasum comparison failed");
			printk("for %s chunk %ul, need to panic\n",
			mtd->name, 
			(unsigned)pos/sha_state->hdt.chunk_size );
			mtdblock_write_to_atlas_and_panic(mtd);
		}
}	
