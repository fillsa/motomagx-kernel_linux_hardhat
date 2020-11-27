/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system. 
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

/* ChangeLog:
 * (mm-dd-yyyy) Author    Comment
 * 10-24-2006   Motorola  Added shredding support to YAFFS2
 */
 
const char *yaffs_nand_c_version =
    "$Id: yaffs_nand.c,v 1.4 2006/10/13 08:52:49 charles Exp $";

#include "yaffs_nand.h"
#include "yaffs_tagscompat.h"
#include "yaffs_tagsvalidity.h"


int yaffs_ReadChunkWithTagsFromNAND(yaffs_Device * dev, int chunkInNAND,
					   __u8 * buffer,
					   yaffs_ExtendedTags * tags)
{
	int result;
	yaffs_ExtendedTags localTags;
	
	int realignedChunkInNAND = chunkInNAND - dev->chunkOffset;
	
	/* If there are no tags provided, use local tags to get prioritised gc working */
	if(!tags)
		tags = &localTags;

	if (dev->readChunkWithTagsFromNAND)
		result = dev->readChunkWithTagsFromNAND(dev, realignedChunkInNAND, buffer,
						      tags);
	else
		result = yaffs_TagsCompatabilityReadChunkWithTagsFromNAND(dev,
									realignedChunkInNAND,
									buffer,
									tags);	
	if(tags && 
	   tags->eccResult > YAFFS_ECC_RESULT_NO_ERROR){
	
		yaffs_BlockInfo *bi = yaffs_GetBlockInfo(dev, chunkInNAND/dev->nChunksPerBlock);
                yaffs_HandleChunkError(dev,bi);
	}
								
	return result;
}

int yaffs_WriteChunkWithTagsToNAND(yaffs_Device * dev,
						   int chunkInNAND,
						   const __u8 * buffer,
						   yaffs_ExtendedTags * tags)
{
	chunkInNAND -= dev->chunkOffset;

	
	if (tags) {
		tags->sequenceNumber = dev->sequenceNumber;
		tags->chunkUsed = 1;
		if (!yaffs_ValidateTags(tags)) {
			T(YAFFS_TRACE_ERROR,
			  (TSTR("Writing uninitialised tags" TENDSTR)));
			YBUG();
		}
		T(YAFFS_TRACE_WRITE,
		  (TSTR("Writing chunk %d tags %d %d" TENDSTR), chunkInNAND,
		   tags->objectId, tags->chunkId));
	} else {

#ifdef CONFIG_MOT_FEAT_YAFFS_SHREDDER
		if (!(dev->sMountOptions & YAFFS_MOUNT_SHRED)) {
			T(YAFFS_TRACE_ERROR, (TSTR("Writing with no tags" TENDSTR)));
			YBUG();
		}
#else
		T(YAFFS_TRACE_ERROR, (TSTR("Writing with no tags" TENDSTR)));
		YBUG();
#endif
	}

	if (dev->writeChunkWithTagsToNAND)
		return dev->writeChunkWithTagsToNAND(dev, chunkInNAND, buffer,
						     tags);
	else
		return yaffs_TagsCompatabilityWriteChunkWithTagsToNAND(dev,
								       chunkInNAND,
								       buffer,
								       tags);
}

int yaffs_MarkBlockBad(yaffs_Device * dev, int blockNo)
{
	blockNo -= dev->blockOffset;

;
	if (dev->markNANDBlockBad)
		return dev->markNANDBlockBad(dev, blockNo);
	else
		return yaffs_TagsCompatabilityMarkNANDBlockBad(dev, blockNo);
}

int yaffs_QueryInitialBlockState(yaffs_Device * dev,
						 int blockNo,
						 yaffs_BlockState * state,
						 unsigned *sequenceNumber)
{
	blockNo -= dev->blockOffset;

	if (dev->queryNANDBlock)
		return dev->queryNANDBlock(dev, blockNo, state, sequenceNumber);
	else
		return yaffs_TagsCompatabilityQueryNANDBlock(dev, blockNo,
							     state,
							     sequenceNumber);
}


int yaffs_EraseBlockInNAND(struct yaffs_DeviceStruct *dev,
				  int blockInNAND)
{
	int result;

	blockInNAND -= dev->blockOffset;


	dev->nBlockErasures++;
	result = dev->eraseBlockInNAND(dev, blockInNAND);

	/* If at first we don't succeed, try again *once*.*/
	if (!result)
		result = dev->eraseBlockInNAND(dev, blockInNAND);	
	return result;
}

int yaffs_InitialiseNAND(struct yaffs_DeviceStruct *dev)
{
	return dev->initialiseNAND(dev);
}


 
