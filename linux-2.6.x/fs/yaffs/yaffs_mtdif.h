/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system. 
 * yaffs_mtdif.h  NAND mtd interface wrappers
 *
 * Copyright (C) 2005,2007 Motorola, Inc.
 * Copyright (C) 2002 Aleph One Ltd.
 * for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 *
 * Note: Only YAFFS headers are LGPL, YAFFS C code is covered by GPL.
 *
 * $Id: yaffs_mtdif.h,v 1.4 2002/09/27 20:50:50 charles Exp $
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ---------------------------
 * 08/17/2005   Motorola  Added MarkNandBlockBad and QueryNandBlock
 * 02/15/2007   Motorola  Source Non-compliant files in kernel package.
 * 05/02/2007   Motorola  Modify compliance issues in hardhat source files.
 * 06/28/2007   Motorola  Modify compliance issues in hardhat source files.
 * 08/01/2007   Motorola  Add comments for oss compliance.
 * 08/10/2007   Motorola  Add comments.
 */

#ifndef __YAFFS_MTDIF_H__
#define __YAFFS_MTDIF_H__

#include "yaffs_guts.h"

int nandmtd_WriteChunkToNAND(yaffs_Device *dev,int chunkInNAND,const __u8 *data, yaffs_Spare *spare);
int nandmtd_ReadChunkFromNAND(yaffs_Device *dev,int chunkInNAND, __u8 *data, yaffs_Spare *spare);
int nandmtd_EraseBlockInNAND(yaffs_Device *dev, int blockNumber);
int nandmtd_InitialiseNAND(yaffs_Device *dev);

#ifdef CONFIG_MOT_FEAT_MTD_FS
int nandmtd_MarkNANDBlockBad(struct yaffs_DeviceStruct *dev, int blockNo);
int nandmtd_QueryNANDBlock(struct yaffs_DeviceStruct *dev, int blockNo);
#endif

#endif




