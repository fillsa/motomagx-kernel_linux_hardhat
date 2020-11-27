/*
 * FILE NAME include/linux/pram_fs_sb.h
 *
 * Definitions for the PRAM filesystem.
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
#ifndef _LINUX_PRAM_FS_SB
#define _LINUX_PRAM_FS_SB

/*
 * PRAM filesystem super-block data in memory
 */
struct pram_sb_info {
	/*
	 * base physical and virtual address of PRAMFS (which is also
	 * the pointer to the super block)
	 */
	unsigned long phys_addr;
	void * virt_addr;

	u32 maxsize;
	u32 features;
	u16 magic;
};

#endif	/* _LINUX_PRAM_FS_SB */
