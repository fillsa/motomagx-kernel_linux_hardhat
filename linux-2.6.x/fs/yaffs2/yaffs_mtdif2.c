/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system. 
 * yaffs_mtdif.c  NAND mtd wrapper functions.
 *
 * Copyright (C) 2006 Motorola Inc.
 * Copyright (C) 2002 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/* ChangeLog:
 * (mm-dd-yyyy) Author    Comment
 * 09-11-2006   Motorola  Applied Large Page NAND patch.
 *                        Supported the 'data shred' feature.
 */


/* mtd interface for YAFFS2 */

const char *yaffs_mtdif2_c_version =
    "$Id: yaffs_mtdif2.c,v 1.7 2005/08/11 01:07:43 marty Exp $";

#include "yportenv.h"


#include "yaffs_mtdif2.h"

#include "linux/mtd/mtd.h"
#include "linux/types.h"
#include "linux/time.h"

#include "yaffs_packedtags2.h"

/* #define YAFFS2_OOB_DEBUG */

#if defined(CONFIG_MOT_FEAT_MTD_FS) && defined(YAFFS2_OOB_DEBUG)
static void print_oob_info(yaffs_Device *dev, char *func, char *action)
{
	if (mtd_fs_runtime_info >= 3)
	{
		int index;
		printk("%s: %s - show packed format\n", func, action); 
		for ( index=0; index < 64; index++ )
		{
			if ((index % 16) == 0) printk("\n");
			printk("0x%02x ", dev->spareBuffer[index]);
		}
		printk("\n\n");
	}
}
#endif

#ifdef CONFIG_MOT_WFN439
void nandmtd2_pt2buf(yaffs_Device *dev, yaffs_PackedTags2 *pt, int is_raw)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	__u8 *ptab = (__u8 *)pt; /* packed tags as bytes */
	
	int	i, j = 0, k, n;

	/* Pack buffer with 0xff */
	for (i = 0; i < mtd->oobsize; i++)
		dev->spareBuffer[i] = 0xff;
		
	if(!is_raw){
		memcpy(dev->spareBuffer,pt,sizeof(yaffs_PackedTags2));
	} else {
		j = 0;
		k = mtd->oobinfo.oobfree[j][0];
		n = mtd->oobinfo.oobfree[j][1];

		if (n == 0) {
			T(YAFFS_TRACE_ERROR, (TSTR("No OOB space for tags" TENDSTR)));
			YBUG();
		}

		for (i = 0; i < sizeof(yaffs_PackedTags2); i++) {
			if (n == 0) {
				j++;
				k = mtd->oobinfo.oobfree[j][0];
				n = mtd->oobinfo.oobfree[j][1];
				if (n == 0) {
					T(YAFFS_TRACE_ERROR, (TSTR("No OOB space for tags" TENDSTR)));
					YBUG();
				}
			}
			dev->spareBuffer[k] = ptab[i];
			k++;
			n--;
		}
	}

#if defined(CONFIG_MOT_FEAT_MTD_FS) && defined(YAFFS2_OOB_DEBUG)
	print_oob_info (dev, __FUNCTION__, "Write");
#endif

}

void nandmtd2_buf2pt(yaffs_Device *dev, yaffs_PackedTags2 *pt, int is_raw)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	int	i, j = 0, k, n;
	__u8 *ptab = (__u8 *)pt; /* packed tags as bytes */


	if (!is_raw) {
	
		memcpy(pt,dev->spareBuffer,sizeof(yaffs_PackedTags2));
	} else {
		j = 0;
		k = mtd->oobinfo.oobfree[j][0];
		n = mtd->oobinfo.oobfree[j][1];

		if (n == 0) {
			T(YAFFS_TRACE_ERROR, (TSTR("No space in OOB for tags" TENDSTR)));
			YBUG();
		}

		for (i = 0; i < sizeof(yaffs_PackedTags2); i++) {
			if (n == 0) {
				j++;
				k = mtd->oobinfo.oobfree[j][0];
				n = mtd->oobinfo.oobfree[j][1];
				if (n == 0) {
					T(YAFFS_TRACE_ERROR, (TSTR("No space in OOB for tags" TENDSTR)));
					YBUG();
				}
			}
			ptab[i] = dev->spareBuffer[k];
			k++;
			n--;
		}
	}

#if defined(CONFIG_MOT_FEAT_MTD_FS) && defined(YAFFS2_OOB_DEBUG)
	print_oob_info (dev, __FUNCTION__, "Read");
#endif

}

#endif /* CONFIG_MOT_WFN439 */


int nandmtd2_WriteChunkWithTagsToNAND(yaffs_Device * dev, int chunkInNAND,
				      const __u8 * data,
				      const yaffs_ExtendedTags * tags)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	size_t dummy;
	int retval = 0;

	loff_t addr = ((loff_t) chunkInNAND) * dev->nBytesPerChunk;

	yaffs_PackedTags2 pt;

	T(YAFFS_TRACE_MTD,
	  (TSTR
	   ("nandmtd2_WriteChunkWithTagsToNAND chunk %d data %p tags %p"
	    TENDSTR), chunkInNAND, data, tags));

	if (tags) {
		yaffs_PackTags2(&pt, tags);
	}

#ifdef CONFIG_MOT_WFN439
	if (data && tags) {
		nandmtd2_pt2buf(dev, &pt, 0);
		retval =
		mtd->write_ecc(mtd, addr, dev->nBytesPerChunk,
			&dummy, data, dev->spareBuffer, NULL);

	} else {

#ifdef CONFIG_MOT_FEAT_YAFFS_SHREDDER
		if ((dev->sMountOptions & YAFFS_MOUNT_SHRED) && data)
		{
		    retval =
		    mtd->write(mtd, addr, dev->nBytesPerChunk, &dummy, data);
		    if (retval)
		    {
			    T(YAFFS_TRACE_ERROR, (TSTR("Data Shredding Failed" TENDSTR)));
		    }
		   
		   /* Even if shrdding fails, we treat it as success. */
		   return YAFFS_OK;
		}

		T(YAFFS_TRACE_ALWAYS,
		  (TSTR
		  ("Write chunk with null tags or data!" TENDSTR)));
		YBUG();

#else /* !CONFIG_MOT_FEAT_YAFFS_SHREDDER */

		T(YAFFS_TRACE_ALWAYS,
		  (TSTR
		  ("Write chunk with null tags or data!" TENDSTR)));
		YBUG();

#endif /* CONFIG_MOT_FEAT_YAFFS_SHREDDER */

	}

#else /* !CONFIG_MOT_WFN439 */
	if (data && tags) {
		if (dev->useNANDECC)
			retval =
			    mtd->write_ecc(mtd, addr, dev->nBytesPerChunk,
					   &dummy, data, (__u8 *) & pt, NULL);
		else
			retval =
			    mtd->write_ecc(mtd, addr, dev->nBytesPerChunk,
					   &dummy, data, (__u8 *) & pt, NULL);
	} else {
		if (data)
			retval =
			    mtd->write(mtd, addr, dev->nBytesPerChunk, &dummy,
				       data);
		if (tags) {
			int i, j, len;
			__u8 *p = (__u8 *)&pt;

			memset(&dev->spareBuffer, 0xff, mtd->oobsize);
			for (i = 0, j = 0;
			     mtd->oobinfo.oobfree[i][1] && j < sizeof(pt);
			     i++) {
				len = min(sizeof(pt) - j,
					mtd->oobinfo.oobfree[i][1]);
				memcpy(&dev->spareBuffer[mtd->oobinfo.oobfree[i][0]], p + j, len);
				j += len;
			}

			retval =
			    mtd->write_oob(mtd, addr, mtd->oobsize, &dummy,
 					   (__u8 *) dev->spareBuffer);
		}
	}
#endif /* CONFIG_MOT_WFN439 */

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

int nandmtd2_ReadChunkWithTagsFromNAND(yaffs_Device * dev, int chunkInNAND,
				       __u8 * data, yaffs_ExtendedTags * tags)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	size_t dummy;
	int retval = 0;

	loff_t addr = ((loff_t) chunkInNAND) * dev->nBytesPerChunk;

	yaffs_PackedTags2 pt;

	T(YAFFS_TRACE_MTD,
	  (TSTR
	   ("nandmtd2_ReadChunkWithTagsToNAND chunk %d data %p tags %p"
	    TENDSTR), chunkInNAND, data, tags));

#ifdef CONFIG_MOT_WFN439 
	if (data && tags) {
		retval =
		    mtd->read_ecc(mtd, addr, dev->nBytesPerChunk,
			&dummy, data, dev->spareBuffer, NULL);

		nandmtd2_buf2pt(dev, &pt, 0);
	} else {
		if (data)
		    retval =
		        mtd->read(mtd, addr, dev->nBytesPerChunk, &dummy,
				      data);
		if (tags) {
		    retval =
		        mtd->read_oob(mtd, addr, mtd->oobsize, &dummy,
					  dev->spareBuffer);
			nandmtd2_buf2pt(dev, &pt, 1);
		}
	}
	
#else
	if (data && tags) {
		if (dev->useNANDECC) {
			retval =
			    mtd->read_ecc(mtd, addr, dev->nBytesPerChunk,
					  &dummy, data, dev->spareBuffer,
					  NULL);
		} else {
			retval =
			    mtd->read_ecc(mtd, addr, dev->nBytesPerChunk,
					  &dummy, data, dev->spareBuffer,
					  NULL);
		}
		memcpy(&pt, dev->spareBuffer, sizeof(pt));
	} else {
		if (data)
			retval =
			    mtd->read(mtd, addr, dev->nBytesPerChunk, &dummy,
				      data);
		if (tags) {
			int i, j, len;
			__u8 *p = (__u8 *)&pt;

			retval =
			    mtd->read_oob(mtd, addr, mtd->oobsize, &dummy,
					  dev->spareBuffer);

			for (i = 0, j = 0;
			     mtd->oobinfo.oobfree[i][1] && j < sizeof(pt);
			     i++) {
				len = min(sizeof(pt) - j,
					mtd->oobinfo.oobfree[i][1]);
				memcpy(p + j, &dev->spareBuffer[mtd->oobinfo.oobfree[i][0]], len);
				j += len;
			}
		}
	}
#endif  /* CONFIG_MOT_WFN439 */

	if (tags)
		yaffs_UnpackTags2(tags, &pt);

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

int nandmtd2_MarkNANDBlockBad(struct yaffs_DeviceStruct *dev, int blockNo)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	int retval;
	T(YAFFS_TRACE_MTD,
	  (TSTR("nandmtd2_MarkNANDBlockBad %d" TENDSTR), blockNo));

	retval =
	    mtd->block_markbad(mtd,
			       blockNo * dev->nChunksPerBlock *
			       dev->nBytesPerChunk);

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;

}

int nandmtd2_QueryNANDBlock(struct yaffs_DeviceStruct *dev, int blockNo,
			    yaffs_BlockState * state, int *sequenceNumber)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	int retval;

	T(YAFFS_TRACE_MTD,
	  (TSTR("nandmtd2_QueryNANDBlock %d" TENDSTR), blockNo));
	retval =
	    mtd->block_isbad(mtd,
			     blockNo * dev->nChunksPerBlock *
			     dev->nBytesPerChunk);

	if (retval) {
		T(YAFFS_TRACE_MTD, (TSTR("block is bad" TENDSTR)));

		*state = YAFFS_BLOCK_STATE_DEAD;
		*sequenceNumber = 0;
	} else {
		yaffs_ExtendedTags t;
		nandmtd2_ReadChunkWithTagsFromNAND(dev,
						   blockNo *
						   dev->nChunksPerBlock, NULL,
						   &t);

		if (t.chunkUsed) {
			*sequenceNumber = t.sequenceNumber;
			*state = YAFFS_BLOCK_STATE_NEEDS_SCANNING;
		} else {
			*sequenceNumber = 0;
			*state = YAFFS_BLOCK_STATE_EMPTY;
		}
	}
	T(YAFFS_TRACE_MTD,
	  (TSTR("block is bad seq %d state %d" TENDSTR), *sequenceNumber,
	   *state));

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

