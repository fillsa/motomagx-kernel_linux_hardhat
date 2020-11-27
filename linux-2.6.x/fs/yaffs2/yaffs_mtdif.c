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
 * 09-11-2006   Motorola  Make sure eccpos bytes match mxc_nd
 */

const char *yaffs_mtdif_c_version =
    "$Id: yaffs_mtdif.c,v 1.12 2005/09/20 23:14:14 charles Exp $";

#include "yportenv.h"


#include "yaffs_mtdif.h"

#include "linux/mtd/mtd.h"
#include "linux/types.h"
#include "linux/time.h"
#include "linux/mtd/nand.h"

#if defined (CONFIG_MOT_FEAT_MTD_FS) && defined(CONFIG_MTD_NAND_MXC)
/* The eccbytes and eccpos fields should be matched as in nand/mxc_nd.c */
static struct nand_oobinfo yaffs_oobinfo = {
	.useecc = 1,
	.eccbytes = 5,
	.eccpos = {6, 7, 8, 9, 10}
};
#else
static struct nand_oobinfo yaffs_oobinfo = {
	.useecc = 1,
#ifdef CONFIG_YAFFS_MXC_MODE
	.eccbytes = 3,
	.eccpos = {6, 7, 8},
#else /* CONFIG_YAFFS_MXC_MODE */
	.eccbytes = 6,
	.eccpos = {8, 9, 10, 13, 14, 15}
#endif /* CONFIG_YAFFS_MXC_MODE */
};

#endif

static struct nand_oobinfo yaffs_noeccinfo = {
	.useecc = 0,
};

int nandmtd_WriteChunkToNAND(yaffs_Device * dev, int chunkInNAND,
			     const __u8 * data, const yaffs_Spare * spare)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	size_t dummy;
	int retval = 0;

	loff_t addr = ((loff_t) chunkInNAND) * dev->nBytesPerChunk;

	__u8 *spareAsBytes = (__u8 *) spare;

	if (data && spare) {
		if (dev->useNANDECC)
			retval =
			    mtd->write_ecc(mtd, addr, dev->nBytesPerChunk,
					   &dummy, data, spareAsBytes,
					   &yaffs_oobinfo);
		else
			retval =
			    mtd->write_ecc(mtd, addr, dev->nBytesPerChunk,
					   &dummy, data, spareAsBytes,
					   &yaffs_noeccinfo);
	} else {
		if (data)
			retval =
			    mtd->write(mtd, addr, dev->nBytesPerChunk, &dummy,
				       data);
		if (spare)
			retval =
			    mtd->write_oob(mtd, addr, YAFFS_BYTES_PER_SPARE,
					   &dummy, spareAsBytes);
	}

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

int nandmtd_ReadChunkFromNAND(yaffs_Device * dev, int chunkInNAND, __u8 * data,
			      yaffs_Spare * spare)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	size_t dummy;
	int retval = 0;

	loff_t addr = ((loff_t) chunkInNAND) * dev->nBytesPerChunk;

	__u8 *spareAsBytes = (__u8 *) spare;

	if (data && spare) {
		if (dev->useNANDECC) {	
			/* Careful, this call adds 2 ints */
			/* to the end of the spare data.  Calling function */
			/* should allocate enough memory for spare, */
			/* i.e. [YAFFS_BYTES_PER_SPARE+2*sizeof(int)]. */
			retval =
			    mtd->read_ecc(mtd, addr, dev->nBytesPerChunk,
					  &dummy, data, spareAsBytes,
					  &yaffs_oobinfo);
		} else {
			retval =
			    mtd->read_ecc(mtd, addr, dev->nBytesPerChunk,
					  &dummy, data, spareAsBytes,
					  &yaffs_noeccinfo);
		}
	} else {
		if (data)
			retval =
			    mtd->read(mtd, addr, dev->nBytesPerChunk, &dummy,
				      data);
		if (spare)
			retval =
			    mtd->read_oob(mtd, addr, YAFFS_BYTES_PER_SPARE,
					  &dummy, spareAsBytes);
	}

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

int nandmtd_EraseBlockInNAND(yaffs_Device * dev, int blockNumber)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	__u32 addr =
	    ((loff_t) blockNumber) * dev->nBytesPerChunk
		* dev->nChunksPerBlock;
	struct erase_info ei;
	int retval = 0;

	ei.mtd = mtd;
	ei.addr = addr;
	ei.len = dev->nBytesPerChunk * dev->nChunksPerBlock;
	ei.time = 1000;
	ei.retries = 2;
	ei.callback = NULL;
	ei.priv = (u_long) dev;

	/* Todo finish off the ei if required */

	retval = mtd->erase(mtd, &ei);

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

int nandmtd_InitialiseNAND(yaffs_Device * dev)
{
	return YAFFS_OK;
}

#ifdef CONFIG_MOT_FEAT_MTD_FS
int nandmtd_QueryNANDBlock(struct yaffs_DeviceStruct *dev, int blockNo,
			    yaffs_BlockState * state, int *sequenceNumber)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	static yaffs_Spare spareFF;
	static int init=0;
	int retval;

	if (!init) {
		memset(&spareFF, 0xFF, sizeof(spareFF));
		init = 1;
	}

	T(YAFFS_TRACE_MTD,
	  (TSTR("nandmtd_QueryNANDBlock %d" TENDSTR), blockNo));
	retval =
	    mtd->block_isbad(mtd,
			     blockNo * dev->nChunksPerBlock *
			     dev->nBytesPerChunk);

	*sequenceNumber = 0;

	if (retval) {
		T(YAFFS_TRACE_MTD, (TSTR("block is bad" TENDSTR)));

		*state = YAFFS_BLOCK_STATE_DEAD;

	} else {
	        yaffs_Spare spare0;
		memset(&spare0, 0xFF, sizeof(spare0));

                nandmtd_ReadChunkFromNAND(dev, blockNo * dev->nChunksPerBlock,
		                          NULL, &spare0);

                if (memcmp(&spareFF, &spare0, sizeof(spareFF)) == 0) 
			*state = YAFFS_BLOCK_STATE_EMPTY;
 		else 
			*state = YAFFS_BLOCK_STATE_NEEDS_SCANNING;

	}
	T(YAFFS_TRACE_MTD,
	  (TSTR("block is bad seq %d state %d" TENDSTR), *sequenceNumber,
	   *state));

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}
#endif


