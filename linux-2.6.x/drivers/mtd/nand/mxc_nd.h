/*
 *  Copyright 2005 Motorola, Inc.
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
 * @file mxc_nd.h
 * 
 * @brief This file contains the NAND Flash Controller register information. 
 *
 * ChangeLog:
 * (mm-dd-yyyy) Author    Comment
 * 11-15-2005	Motorola  implemented CONFIG_MOT_FEAT_MTD_FS feature.
 *			  defined addresses for NFC page_size & data_width config 
 *			  in CRM-COM register.
 *
 * @ingroup NAND_MTD
 */

#ifndef MXCND_H
#define MXCND_H

#include <asm/hardware.h>

/*
 * Addresses for NFC registers
 */
#define NFC_BUF_SIZE            (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE00)))
#define NFC_BUF_ADDR            (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE04)))
#define NFC_FLASH_ADDR          (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE06)))
#define NFC_FLASH_CMD           (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE08)))
#define NFC_CONFIG              (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE0A)))
#define NFC_ECC_STATUS_RESULT   (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE0C)))
#define NFC_RSLTMAIN_AREA       (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE0E)))
#define NFC_RSLTSPARE_AREA      (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE10)))
#define NFC_WRPROT              (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE12)))
#define NFC_UNLOCKSTART_BLKADDR (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE14)))
#define NFC_UNLOCKEND_BLKADDR   (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE16)))
#define NFC_NF_WRPRST           (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE18)))
#define NFC_CONFIG1             (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE1A)))
#define NFC_CONFIG2             (*((volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0xE1C)))

/*!
 * Addresses for NFC RAM BUFFER Main area 0, 1, 2, 3
 */
#define MAIN_AREA0        (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0x000)
#define MAIN_AREA1        (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0x200)
#define MAIN_AREA2        (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0x400)
#define MAIN_AREA3        (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0x600)

/*!
 * Addresses for NFC SPARE BUFFER Spare area 0, 1 ,2, 3
 */
#define SPARE_AREA0       (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0x800)
#define SPARE_AREA1       (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0x810)
#define SPARE_AREA2       (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0x820)
#define SPARE_AREA3       (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR + 0x830)

#ifdef CONFIG_MOT_FEAT_MTD_FS
/*!
 * Addresses for NFC page_size & data_width config in CRM-COM register
 */
#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91131)
#define NF_PGSZ_DWITH   (*((volatile unsigned long *)IO_ADDRESS(CRM_COM_BASE_ADDR + 0x00C)))
#elif defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321)
#define NF_PGSZ_DWITH   (*((volatile unsigned long *)IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x00C)))
#endif
#endif /* CONFIG_MOT_FEAT_MTD_FS */

/*! 
 * Set INT to 0, FCMD to 1, rest to 0 in NFC_CONFIG2 Register for Command
 * operation
 */
#define NFC_CMD            0x1

/*! 
 * Set INT to 0, FADD to 1, rest to 0 in NFC_CONFIG2 Register for Address
 * operation
 */
#define NFC_ADDR           0x2

/*! 
 * Set INT to 0, FDI to 1, rest to 0 in NFC_CONFIG2 Register for Input
 * operation
 */
#define NFC_INPUT          0x4

/*! 
 * Set INT to 0, FDO to 001, rest to 0 in NFC_CONFIG2 Register for Data Output
 * operation
 */
#define NFC_OUTPUT         0x8

/*! 
 * Set INT to 0, FD0 to 010, rest to 0 in NFC_CONFIG2 Register for Read ID
 * operation
 */
#define NFC_ID             0x10

/*! 
 * Set INT to 0, FDO to 100, rest to 0 in NFC_CONFIG2 Register for Read Status
 * operation
 */
#define NFC_STATUS         0x20

/*! 
 * Set INT to 1, rest to 0 in NFC_CONFIG2 Register for Read Status
 * operation
 */
#define NFC_INT            0x8000

#define NFC_SP_EN           (1 << 2)
#define NFC_ECC_EN          (1 << 3)
#define NFC_INT_MSK         (1 << 4)
#define NFC_BIG             (1 << 5)
#define NFC_RST             (1 << 6)
#define NFC_CE              (1 << 7)

#endif				/* MXCND_H */
