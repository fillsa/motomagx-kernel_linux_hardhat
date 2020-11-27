/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system. 
 * yaffs_mtdif.h  NAND mtd interface wrappers
 *
 * Copyright (C) 2006 Motorola Inc.
 * Copyright (C) 2002 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* ChangeLog:
 * (mm-dd-yyyy) Author    Comment
 * 01-05-2006   Motorola  Add BBT support to YAFFS2
 */

/* Note: Only YAFFS headers are LGPL, YAFFS C code is covered by GPL.
 *
 * $Id: yaffs_mtdif.h,v 1.2 2005/07/19 20:41:59 charles Exp $
 */

#ifndef __YAFFS_MTDIF_H__
#define __YAFFS_MTDIF_H__

#include "yaffs_guts.h"

int nandmtd_WriteChunkToNAND(yaffs_Device * dev, int chunkInNAND,
			     const __u8 * data, const yaffs_Spare * spare);
int nandmtd_ReadChunkFromNAND(yaffs_Device * dev, int chunkInNAND, __u8 * data,
			      yaffs_Spare * spare);
int nandmtd_EraseBlockInNAND(yaffs_Device * dev, int blockNumber);
int nandmtd_InitialiseNAND(yaffs_Device * dev);

#ifdef CONFIG_MOT_FEAT_MTD_FS
int nandmtd_QueryNANDBlock(struct yaffs_DeviceStruct *dev, int blockNo,
                            yaffs_BlockState * state, int *sequenceNumber);

#endif

#endif
