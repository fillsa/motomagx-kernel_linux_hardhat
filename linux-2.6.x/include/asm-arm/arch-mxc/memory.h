/*
 *  Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
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
 */

#ifndef __ASM_ARM_ARCH_MXC_MEMORY_H_
#define __ASM_ARM_ARCH_MXC_MEMORY_H_

#include <asm/arch/hardware.h>

/*!
 * @defgroup Memory Memory Map
 * @ingroup MSL
 */

/*!
 * @file memory.h
 * @brief This file contains macros needed by the Linux kernel and drivers.
 *
 * @ingroup Memory
 */

/*!
 * Physical start address of the first bank of RAM
 */
#if defined(PHYS_OFFSET)

/*! This is needed for L2 Cache support */
#define PHYS_OFFSET_ASM	        PHYS_OFFSET
#define PAGE_OFFSET_ASM	        0xC0000000

#else
#error "Define PHYS_OFFSET to the base of the physical memory's PA"
#endif

/*!
 * Virtual view <-> DMA view memory address translations
 * This macro is used to translate the virtual address to an address
 * suitable to be passed to set_dma_addr()
 */
#define __virt_to_bus(a)	__virt_to_phys(a)

/*!
 * Used to convert an address for DMA operations to an address that the
 * kernel can use.
 */
#define __bus_to_virt(a)	__phys_to_virt(a)

#ifdef CONFIG_DISCONTIGMEM

/*!
 * The MXC's memory is physically contiguous, but to
 * support and test Memory Type Based Allocation, we need
 * to artificially create some memory banks. In this simple
 * example, the physical memory is split into 8MB banks.
 */

#define	NODES_SHIFT	2
#define MXC_NUMNODES	(1 << NODES_SHIFT)
#define NODE_MAX_MEM_SHIFT	23
#define NODE_MAX_MEM_SIZE	(1<<NODE_MAX_MEM_SHIFT)

#define SET_NODE(mi, nid) { \
	(mi)->bank[(nid)].start = PHYS_OFFSET + (nid) * NODE_MAX_MEM_SIZE; \
	(mi)->bank[(nid)].size =  NODE_MAX_MEM_SIZE; \
	(mi)->bank[(nid)].node = (nid); \
	node_set_online(nid); \
}

/*!
 * Given a kernel address, find the home node of the underlying memory.
 */
#define KVADDR_TO_NID(addr) \
	(((unsigned long)(addr) - PAGE_OFFSET) >> NODE_MAX_MEM_SHIFT)

/*!
 * Given a page frame number, convert it to a node id.
 */
#define PFN_TO_NID(pfn) \
	(((pfn) - PHYS_PFN_OFFSET) >> (NODE_MAX_MEM_SHIFT - PAGE_SHIFT))

/*!
 * Given a kaddr, ADDR_TO_MAPBASE finds the owning node of the memory
 * and returns the mem_map of that node.
 */
#define ADDR_TO_MAPBASE(kaddr) NODE_MEM_MAP(KVADDR_TO_NID(kaddr))

/*!
 * Given a page frame number, find the owning node of the memory
 * and returns the mem_map of that node.
 */
#define PFN_TO_MAPBASE(pfn)    NODE_MEM_MAP(PFN_TO_NID(pfn))

/*!
 * Given a kaddr, LOCAL_MAR_NR finds the owning node of the memory
 * and returns the index corresponding to the appropriate page in the
 * node's mem_map.
 */
#define LOCAL_MAP_NR(addr) \
	(((unsigned long)(addr) & (NODE_MAX_MEM_SIZE - 1)) >> PAGE_SHIFT)

#endif				/* CONFIG_DISCONTIGMEM */

#endif				/* __ASM_ARM_ARCH_MXC_MEMORY_H_ */
