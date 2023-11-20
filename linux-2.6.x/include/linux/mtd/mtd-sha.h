/*
 *  include/mtd/mtd-sha.h
 *
 *	Copyright 2007 Motorola, Inc.
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
 *
 */
/* Function prototypes */
struct mtd_info;
struct mtdblk_sha;

void mtdblock_write_to_atlas_and_panic(struct mtd_info *);
void mtdblock_verify_sha_sum(struct mtd_info *, struct mtdblk_sha *, 
		unsigned char *, unsigned int, unsigned long);
void mtdblock_hdt_sanity_check(struct mtdblk_sha *);
void sec_hash_lru_init(unsigned);
void sec_hash_add_code_group(unsigned, unsigned, unsigned);
void mtdblock_setup_crypto(struct mtdblk_sha *, struct mtd_info *, unsigned);


struct mtdblk_hdt
{
	uint32_t        hdt_barker; /* (constant as per the 23338-FAD) */
	/* used to logically locate sec_hash_block start */
	uint32_t	hash_block_start_offset_from_image_start;
	/* size of block containing sha1sum values, sec_hash_block_padding and HDT */
	uint32_t        sec_hash_block_size;
	/* chunk_size is the size of block into which cg-image is divided */
	uint32_t        chunk_size;
	uint32_t        number_of_chunks; /* number of chunks of image */
	uint32_t        unused; /* was hash_inval_freq */
	uint32_t        unused_reserved_for_expansion_1;
	uint32_t        unused_reserved_for_expansion_2;
	uint32_t        unused_reserved_for_expansion_3;
	/* componenet security version */
	uint32_t        image_csv;
};

/* 
 * This structure is used to maintain the state for
 * sha verification in kernel feature
 */
struct mtdblk_sha {
	struct mtdblk_hdt hdt;
	unsigned long *cache_bitmap;
	unsigned long *sha_hash_bitmap;
	unsigned long lost_blocks;
	struct crypto_tfm *crypto_tfm;
	uint8_t *sha_hashes;
	unsigned int code_group;
};

#define HDT_SIZE  (sizeof(struct mtdblk_hdt))
#define SIZE_OF_SHA_SUM (20) /* Sha sum is 20 bytes long */
#define HDT_BARKER (0x48445442)
#define CSF_SIZE (0x800)
#define LRU_INIT_PAGES (40)

