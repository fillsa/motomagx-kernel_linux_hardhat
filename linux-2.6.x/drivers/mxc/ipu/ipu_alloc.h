/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU Lesser General 
 * Public License.  You may obtain a copy of the GNU Lesser General 
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */

/*!
 * @file ipu_alloc.h
 *
 * @brief simple ipu allocation driver for linux
 *
 * @ingroup IPU
 */
#ifndef _IPU_ALLOC_H_
#define _IPU_ALLOC_H_

#define IPU_PAGE_ALIGN   ((u32) 0x00001000)

typedef struct _memDesc {
	u32 start;
	u32 end;
	struct _memDesc *next;
} memDesc;

/*!
 * ipu_pool_initialize
 *
 * @param       memPool         start address of the pool
 * @param       poolSize        memory pool size
 * @param       alignment       alignment for example page alignmnet will be 0x1000
 *
 * @return      0 for success  -1 for errors
 */
int ipu_pool_initialize(u32 memPool, u32 poolSize, u32 alignment);

/*!
 * ipu_malloc
 *
 * @param       size        memory pool size
 *
 * @return      physical address, 0 for error
 */
u32 ipu_malloc(u32 size);

/*!
 * ipu_free
 *
 * @param       physical        physical address try to free
 *
 */
void ipu_free(u32 physical);

#endif
