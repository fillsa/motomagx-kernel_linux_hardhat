/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file dvfs_dptc.h
 *
 * @brief MXC dvfs & dptc header file. 
 * 
 * @ingroup PM
 */
#ifndef __DVFS_DPTC_H__
#define __DVFS_DPTC_H__

#include <linux/config.h>
#include <asm/arch/pm_api.h>
#include <asm/arch/sdma.h>
#include <asm/hardware.h>
#include <asm/arch/dvfs_dptc_struct.h>

#ifdef CONFIG_ARCH_MX3
#include "dvfs_dptc_table_mx3.h"
#endif
#ifdef CONFIG_ARCH_MXC91331
#include "dvfs_dptc_table_mxc91331.h"
#endif

#ifdef CONFIG_MXC_DVFS
#ifndef CONFIG_MXC_DPTC
/*!
 * DPTC Module Name 
 */
#define DEVICE_NAME	"dvfs"

/*!
 * DPTC driver node Name 
 */
#define NODE_NAME	"dvfs"
#endif				/* ifndef CONFIG_MXC_DPTC */
#ifdef CONFIG_MXC_DPTC
/*!
 * DPTC Module Name 
 */
#define DEVICE_NAME	"dvfs_dptc"

/*!
 * DPTC driver node Name 
 */
#define NODE_NAME	"dvfs_dptc"
#endif				/* ifdef CONFIG_MXC_DPTC */
#else				/* ifdef CONFIG_MXC_DVFS */
/*!
 * DPTC Module Name 
 */
#define DEVICE_NAME	"dptc"

/*!
 * DPTC driver node Name 
 */
#define NODE_NAME	"dptc"
#endif				/* ifdef CONFIG_MXC_DVFS */

#define MAX_TABLE_SIZE 8192

#endif				/* __DPTC_H__ */
