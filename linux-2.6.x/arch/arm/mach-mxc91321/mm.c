/*
 *  Copyright (C) 1999,2000 Arm Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *  Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *    - add MXC specific definitions
 * Copyright 2007 Motorola, Inc.
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

/*
 * Revision History:
 *
 * Date        Author    Comment
 * 01/02/2007  Motorola  Added CS3 to the memory map
 */

#include <linux/mm.h>
#include <linux/init.h>
#include <asm/hardware.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>

/*!
 * @file mm.c
 *
 * @brief This file creates static mapping between physical to virtual memory.
 *
 * @ingroup Memory
 */

/*!
 * This structure defines the MXC memory map.
 */
static struct map_desc mxc_io_desc[] __initdata = {
	/*  Virtual Address   Physical Address  Size         Type */
	{IRAM_BASE_ADDR_VIRT, IRAM_BASE_ADDR, IRAM_SIZE, MT_DEVICE},
	{L2CC_BASE_ADDR_VIRT, L2CC_BASE_ADDR, L2CC_SIZE, MT_DEVICE},
	{X_MEMC_BASE_ADDR_VIRT, X_MEMC_BASE_ADDR, X_MEMC_SIZE, MT_DEVICE},
	{ROMP_BASE_ADDR_VIRT, ROMP_BASE_ADDR, ROMP_SIZE, MT_DEVICE},
	{AVIC_BASE_ADDR_VIRT, AVIC_BASE_ADDR, AVIC_SIZE, MT_DEVICE},
	{AIPS1_BASE_ADDR_VIRT, AIPS1_BASE_ADDR, AIPS1_SIZE, MT_DEVICE},
	{SPBA0_BASE_ADDR_VIRT, SPBA0_BASE_ADDR, SPBA0_SIZE, MT_DEVICE},
	{AIPS2_BASE_ADDR_VIRT, AIPS2_BASE_ADDR, AIPS2_SIZE, MT_DEVICE},
	{CS2_BASE_ADDR_VIRT, CS2_BASE_ADDR, CS2_SIZE, MT_DEVICE},
#ifdef CONFIG_MOT_FEAT_ANTIOCH
	{CS3_BASE_ADDR_VIRT, CS3_BASE_ADDR, CS3_SIZE, MT_DEVICE},
#endif
	{CS4_BASE_ADDR_VIRT, CS4_BASE_ADDR, CS4_SIZE, MT_DEVICE},
};

/*!
 * This function initializes the memory map. It is called during the
 * system startup to create static physical to virtual memory map for
 * the IO modules.
 */
void __init mxc_map_io(void)
{
	iotable_init(mxc_io_desc, ARRAY_SIZE(mxc_io_desc));
}
