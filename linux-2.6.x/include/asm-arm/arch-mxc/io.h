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

/*!
 * @file io.h
 * @brief This file contains some memory mapping macros.
 * @note There is no real ISA or PCI buses. But have to define these macros
 * for some drivers to compile.
 *
 * @ingroup System
 */

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

//#include <asm/arch/hardware.h>

/*! Allow IO space to be anywhere in the memory */
#define IO_SPACE_LIMIT 0xffffffff

/*!
 * io address mapping macro
 */
#define __io(a)			((void __iomem *)(a))

#define __mem_pci(a)		(a)
#define __mem_isa(a)		(a)

/*!
 * Validate the pci memory address for ioremap.
 */
#define iomem_valid_addr(iomem,size)	(1)

/*!
 * Convert PCI memory space to a CPU physical address
 */
#define iomem_to_phys(iomem)	(iomem)

#endif
