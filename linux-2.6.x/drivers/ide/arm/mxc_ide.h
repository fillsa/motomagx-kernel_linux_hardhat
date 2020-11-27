#ifndef _MXC_IDE_H_
#define _MXC_IDE_H_

/* 
 * linux/drivers/ide/arm/mxc_ide.h
 * 
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
 *
 */

/*!
 * @defgroup ATA ATA/IDE Driver
 */

/*!
 * @file mxc_ide.h
 *
 * @brief MXC ATA/IDE hardware register and bit definitions.
 *
 * @ingroup ATA
 */

#define IDE_ARM_IO   IO_ADDRESS((ATA_BASE_ADDR + 0xA0) )
#define IDE_ARM_CTL  IO_ADDRESS((ATA_BASE_ADDR + 0xD8) )
#define IDE_ARM_IRQ  INT_ATA

/* 
 * Interface control registers
 */

#define MXC_IDE_FIFO_DATA_32    IO_ADDRESS((ATA_BASE_ADDR + 0x18) )
#define MXC_IDE_FIFO_DATA_16    IO_ADDRESS((ATA_BASE_ADDR + 0x1C) )
#define MXC_IDE_FIFO_FILL       IO_ADDRESS((ATA_BASE_ADDR + 0x20) )
#define MXC_IDE_ATA_CONTROL     IO_ADDRESS((ATA_BASE_ADDR + 0x24) )
#define MXC_IDE_INTR_PENDING    IO_ADDRESS((ATA_BASE_ADDR + 0x28) )
#define MXC_IDE_INTR_ENABLE     IO_ADDRESS((ATA_BASE_ADDR + 0x2C) )
#define MXC_IDE_INTR_CLEAR      IO_ADDRESS((ATA_BASE_ADDR + 0x30) )
#define MXC_IDE_FIFO_ALARM      IO_ADDRESS((ATA_BASE_ADDR + 0x34) )

/* 
 * Control register bit definitions
 */

#define MXC_IDE_CTRL_FIFO_RST_B      0x80
#define MXC_IDE_CTRL_ATA_RST_B       0x40
#define MXC_IDE_CTRL_FIFO_TX_EN      0x20
#define MXC_IDE_CTRL_FIFO_RCV_EN     0x10
#define MXC_IDE_CTRL_DMA_PENDING     0x08
#define MXC_IDE_CTRL_DMA_ULTRA       0x04
#define MXC_IDE_CTRL_DMA_WRITE       0x02
#define MXC_IDE_CTRL_IORDY_EN        0x01

/* 
 * Interrupt registers bit definitions
 */

#define MXC_IDE_INTR_ATA_INTRQ1      0x80
#define MXC_IDE_INTR_FIFO_UNDERFLOW  0x40
#define MXC_IDE_INTR_FIFO_OVERFLOW   0x20
#define MXC_IDE_INTR_CTRL_IDLE       0x10
#define MXC_IDE_INTR_ATA_INTRQ2      0x08

/* 
 * timing registers 
 */

#define MXC_IDE_TIME_OFF        IO_ADDRESS((ATA_BASE_ADDR + 0x00) )
#define MXC_IDE_TIME_ON         IO_ADDRESS((ATA_BASE_ADDR + 0x01) )
#define MXC_IDE_TIME_1          IO_ADDRESS((ATA_BASE_ADDR + 0x02) )
#define MXC_IDE_TIME_2w         IO_ADDRESS((ATA_BASE_ADDR + 0x03) )

#define MXC_IDE_TIME_2r         IO_ADDRESS((ATA_BASE_ADDR + 0x04) )
#define MXC_IDE_TIME_AX         IO_ADDRESS((ATA_BASE_ADDR + 0x05) )
#define MXC_IDE_TIME_PIO_RDX    IO_ADDRESS((ATA_BASE_ADDR + 0x06) )
#define MXC_IDE_TIME_4          IO_ADDRESS((ATA_BASE_ADDR + 0x07) )

#define MXC_IDE_TIME_9          IO_ADDRESS((ATA_BASE_ADDR + 0x08) )
#define MXC_IDE_TIME_M          IO_ADDRESS((ATA_BASE_ADDR + 0x09) )
#define MXC_IDE_TIME_JN         IO_ADDRESS((ATA_BASE_ADDR + 0x0A) )
#define MXC_IDE_TIME_D          IO_ADDRESS((ATA_BASE_ADDR + 0x0B) )

#define MXC_IDE_TIME_K          IO_ADDRESS((ATA_BASE_ADDR + 0x0C) )
#define MXC_IDE_TIME_ACK        IO_ADDRESS((ATA_BASE_ADDR + 0x0D) )
#define MXC_IDE_TIME_ENV        IO_ADDRESS((ATA_BASE_ADDR + 0x0E) )
#define MXC_IDE_TIME_RPX        IO_ADDRESS((ATA_BASE_ADDR + 0x0F) )

#define MXC_IDE_TIME_ZAH        IO_ADDRESS((ATA_BASE_ADDR + 0x10) )
#define MXC_IDE_TIME_MLIX       IO_ADDRESS((ATA_BASE_ADDR + 0x11) )
#define MXC_IDE_TIME_DVH        IO_ADDRESS((ATA_BASE_ADDR + 0x12) )
#define MXC_IDE_TIME_DZFS       IO_ADDRESS((ATA_BASE_ADDR + 0x13) )

#define MXC_IDE_TIME_DVS        IO_ADDRESS((ATA_BASE_ADDR + 0x14) )
#define MXC_IDE_TIME_CVH        IO_ADDRESS((ATA_BASE_ADDR + 0x15) )
#define MXC_IDE_TIME_SS         IO_ADDRESS((ATA_BASE_ADDR + 0x16) )
#define MXC_IDE_TIME_CYC        IO_ADDRESS((ATA_BASE_ADDR + 0x17) )

/* 
 * other facts
 */
#define MXC_IDE_FIFO_SIZE	64	/* DMA FIFO size in halfwords */
#define MXC_IDE_SDMA_WATERMARK	32	/* SDMA watermark level in bytes */
#define MXC_IDE_SDMA_BD_NR	(512/3/4)	/* Number of BDs per channel */
#define MXC_IDE_SDMA_BD_SIZE_MAX 0xFC00	/* max size of scatterlist segment */

#endif				/* !_MXC_IDE_H_ */
