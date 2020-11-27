/*
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *  Copyright (C) 2006-2007 Motorola, Inc.
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

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 09-Oct-2006  Motorola        Include Motorola board specific header files.
 * 09-Nov-2006  Motorola        Adjusted ArgonLV-related definitions.
 * 01-Jun-2007  Motorola	Add CONFIG_MOT_FEAT_CHECKSUM in.
 */

/*!
 * @file hardware.h
 * @brief This file contains the hardware definitions of the board.
 *
 * @ingroup System
 */
#ifndef __ASM_ARM_ARCH_HARDWARE_H_
#define __ASM_ARM_ARCH_HARDWARE_H_

#include <asm/sizes.h>
#include <linux/config.h>

/*!
 * makes bool as int type
 */
#define bool int

/*!
 * defines false as 0
 */
#define false 		0

/*!
 * defines true as 1
 */
#define true		1

/*!
 * defines PCIO_BASE (not used but needed for compilation)
 */
#define PCIO_BASE		0

/*!
 * This macro is used to get certain bit field from a number
 */
#define MXC_GET_FIELD(val, len, sh)          ((val >> sh) & ((1 << len) - 1))

/*!
 * This macro is used to set certain bit field inside a number
 */
#define MXC_SET_FIELD(val, len, sh, nval)    ((val & ~(((1 << len) - 1) << sh)) | nval << sh)

/* This is used to turn on/off debugging */
#define MXC_TRACE
#ifdef MXC_TRACE
/*!
 * This is used for error checking in debugging mode.
 */
#ifdef CONFIG_MOT_FEAT_CHKSUM
#define MXC_ERR_CHK(a) \
        do { \
                if ((a)) { \
                        printk("Error at line %d in function %s in file %s", \
                                __LINE__, __FUNCTION__, "FILE"); \
                        BUG(); \
                } \
        } \
        while (0)
#else
#define MXC_ERR_CHK(a) \
        do { \
                if ((a)) { \
                        printk("Error at line %d in function %s in file %s", \
                                __LINE__, __FUNCTION__, __FILE__); \
                        BUG(); \
                } \
        } \
        while (0)
#endif
#else
#define MXC_ERR_CHK(a)
#endif

/*
 * ---------------------------------------------------------------------------
 * Processor specific defines
 * ---------------------------------------------------------------------------
 */
#ifdef CONFIG_ARCH_MXC91231
#include <asm/arch/mxc91231.h>
#endif

#if defined(CONFIG_ARCH_MXC91321) || defined(CONFIG_ARCH_MXC91331)
#include <asm/arch/mxc91321.h>
#endif

#ifdef CONFIG_ARCH_MXC91131
#include <asm/arch/mxc91131.h>
#endif

#ifdef CONFIG_ARCH_MX3
#include <asm/arch/mx31.h>
#endif

#include <asm/arch/mxc.h>

#define MXC_MAX_GPIO_LINES      (GPIO_NUM_PIN * GPIO_PORT_NUM)

/*
 * ---------------------------------------------------------------------------
 * Board specific defines
 * ---------------------------------------------------------------------------
 */
#define MXC_EXP_IO_BASE         (MXC_GPIO_BASE + MXC_MAX_GPIO_LINES)

#ifdef CONFIG_MACH_MXC27530EVB
#include <asm/arch/board-mxc27530evb.h>
#endif

#if defined(CONFIG_MACH_MXC30030EVB) || defined(CONFIG_MACH_I30030EVB)
#include <asm/arch/board-mxc30030evb.h>
#endif

#ifdef CONFIG_MACH_I30030ADS
#include <asm/arch/board-i30030ads.h>
#endif

#ifdef CONFIG_MACH_MXC30030ADS
#include <asm/arch/board-mxc30030ads.h>
#endif

#ifdef CONFIG_MACH_MXC91131EVB
#include <asm/arch/board-mxc91131evb.h>
#endif

#ifdef CONFIG_MACH_MX31ADS
#include <asm/arch/board-mx31ads.h>
#endif

#if defined(CONFIG_MACH_SCMA11PHONE)
#include <asm/arch/board-scma11phone.h>
#endif

#if defined(CONFIG_MACH_ARGONLVPHONE)
#include <asm/arch/board-argonlvphone.h>
#endif

#ifndef MXC_MAX_EXP_IO_LINES
#define MXC_MAX_EXP_IO_LINES 0
#endif

#define MXC_MAX_INTS            (MXC_GPIO_BASE + \
                                MXC_MAX_GPIO_LINES + \
                                MXC_MAX_EXP_IO_LINES)

#endif				/* __ASM_ARM_ARCH_HARDWARE_H_ */
