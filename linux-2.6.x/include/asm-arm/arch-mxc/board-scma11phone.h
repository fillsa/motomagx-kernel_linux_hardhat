/*
 * include/asm-arm/arch-mxc/board-scma11phone.h - hardware defines for
 *                                                Motorola's SCM-A11-based
 *                                                phones.
 * 
 * Copyright 2005 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2006-2007 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Date     Author    Comment
 * 10/2006  Motorola  Added board specific definitions for Motorola products.
 * 01/2007  Motorola  Added support for dynamic IPU memory pool config.
 */

#ifndef __ASM_ARM_ARCH_BOARD_SCMA11PHONE_H_
#define __ASM_ARM_ARCH_BOARD_SCMA11PHONE_H_

/*
 * Include Files
 */
#include <linux/config.h>
#include <asm/arch/board.h>

/* Start of physical RAM */
#define PHYS_OFFSET	        (0x90000000UL)

/* I2C configuration */
/*!
 * This defines the number of I2C modules in the MXC platform
 * Defined as 1, as MC13783 on ADS uses the other pins
 */
#define I2C_NR                  1
/*!
 * This define specifies the frequency divider value to be written into
 * the I2C \b IFDR register.
 */
#define I2C1_FRQ_DIV            0x07

/*!
 * @name MXC UART board level configurations
 */
/*! @{ */
/*!
 * Specify the max baudrate for the MXC UARTs for your board, do not specify a max
 * baudrate greater than 1500000. This is used while specifying the UART Power
 * management constraints.
 */
#ifdef CONFIG_MOT_FEAT_BT_MAXUARTBAUDRATE
#define MAX_UART_BAUDRATE       3000000
#else
#define MAX_UART_BAUDRATE       1500000
#endif /* CONFIG_MOT_FEAT_BT_MAXUARTBAUDRATE */
/*!
 * Specifies if the Irda transmit path is inverting
 */
#define MXC_IRDA_TX_INV         MXC_UARTUCR3_INVT
/*!
 * Specifies if the Irda receive path is inverting
 */
#define MXC_IRDA_RX_INV         0
/* UART 1 configuration */
/*!
 * This define specifies if the UART port is configured to be in DTE or
 * DCE mode. There exists a define like this for each UART port. Valid
 * values that can be used are \b MODE_DTE or \b MODE_DCE.
 */
#define UART1_MODE              MODE_DCE
/*!
 * This define specifies if the UART is to be used for IRDA. There exists a
 * define like this for each UART port. Valid values that can be used are
 * \b IRDA or \b NO_IRDA.
 */
#define UART1_IR                NO_IRDA

/*!
 * This define is used to enable or disable a particular UART port. If
 * disabled, the UART will not be registered in the file system and the user
 * will not be able to access it. There exists a define like this for each UART
 * port. Specify a value of 1 to enable the UART and 0 to disable it.
 */
#define UART1_ENABLED           1
/*! @} */
/* UART 2 configuration */
#define UART2_MODE              MODE_DCE
#define UART2_IR                NO_IRDA
#define UART2_ENABLED           1
/* UART 3 configuration */
#define UART3_MODE              MODE_DCE
#define UART3_IR                NO_IRDA
#define UART3_ENABLED           1

#define MXC_LL_UART_PADDR	UART2_BASE_ADDR
#define MXC_LL_UART_VADDR	AIPS1_IO_ADDRESS(UART2_BASE_ADDR)
#undef  MXC_LL_EXT_UART_16BIT_BUS

/*!
 * @name Memory Size parameters
 */
/*! @{ */
/*!
 * Size of SDRAM memory
 */
#define SDRAM_MEM_SIZE          SZ_64M
/*!
 * Size of IPU buffer memory
 * Note: Currently the (MXCIPU_MEM_SIZE)MB of memory is located in 
 * SDRAM right after the memory allocated for kernel.
 * Need to allocate 5MB to support 2MP camera 
 */
#ifndef CONFIG_MOT_FEAT_IPU_MEM_ADDR
#define MXCIPU_MEM_SIZE         (SZ_4M + SZ_1M)
#endif /* CONFIG_MOT_FEAT_IPU_MEM_ADDR */
/*!
 * Size of DSP (Digital Signal Processor) image in SDRAM 
 */
#define DSP_MEM_SIZE		SZ_4M
/*!
 * Size of memory available to kernel
 * This is unused when we pass in the mem= parameter via the command line.
 * Changing the dsp memory size, ipu memory size, and sdram memory size
 * in this case has no effect.
 */
#ifdef CONFIG_MOT_FEAT_IPU_MEM_ADDR
/* MEM_SIZE is only used when the "mem=" boot cmdline option is missing,
 * but CONFIG_MOT_FEAT_IPU_MEM_ADDR won't work if "mem=" is not specified.
 * Thus the value of the following MEM_SIZE has no meaning in normal working
 * conditions.
 */
#define MEM_SIZE                (SDRAM_MEM_SIZE - DSP_MEM_SIZE)
#else
#define MEM_SIZE                (SDRAM_MEM_SIZE - DSP_MEM_SIZE - MXCIPU_MEM_SIZE)
#endif
/*!
 * Size and physical start address of IPU buffer memory
 * 5MB is a safe value for 2 MP camera support. 
 * The exact size depends on the hardware configuration.
 */
#if !defined(CONFIG_MOT_FEAT_IPU_MEM_ADDR)
#define MXCIPU_MEM_ADDRESS      (PHYS_OFFSET + MEM_SIZE)
#endif
/*! @} */

/*!
 * @name Keypad Configurations FIXME
 */
/*! @{ */
/*!
 * Maximum number of rows (0 to 7)
 */
#define MAXROW                  8
/*!
 * Maximum number of columns (0 to 7)
 */
#define MAXCOL                  8
/*! @} */

#if defined (CONFIG_MACH_SCMA11REF)
/*!
 * @name PBC Controller parameters
 */
/*! @{ */
/*!
 * Base address of PBC controller
 */
#define PBC_BASE_ADDRESS	IO_ADDRESS(CS4_BASE_ADDR)
/* Offsets for the PBC Controller register */
/*!
 * PBC Board status register offset
 */
#define PBC_BSTAT               0x000002
/*!
 * PBC Board control register 1 set address.
 */
#define PBC_BCTRL1_SET          0x000004
/*!
 * PBC Board control register 1 clear address.
 */
#define PBC_BCTRL1_CLEAR        0x000006
/*!
 * PBC Board control register 2 set address.
 */
#define PBC_BCTRL2_SET          0x000008
/*!
 * PBC Board control register 2 clear address.
 */
#define PBC_BCTRL2_CLEAR        0x00000A
/*!
 * External UART A.
 */
#define PBC_SC16C652_UARTA      0x010000
/*!
 * External UART B.
 */
#define PBC_SC16C652_UARTB      0x010010
/*!
 * Ethernet Controller IO base address.
 */
#define PBC_CS8900A_IOBASE      0x020000
/*!
 * Ethernet Controller Memory base address.
 */
#define PBC_CS8900A_MEMBASE     0x021000
/*!
 * Ethernet Controller DMA base address.
 */
#define PBC_CS8900A_DMABASE     0x022000
/*!
 * External chip select 0.
 */
#define PBC_XCS0                0x040000
/*!
 * LCD Display enable.
 */
#define PBC_LCD_EN_B            0x060000
/*!
 * Code test debug enable.
 */
#define PBC_CODE_B              0x070000
/*!
 * PSRAM memory select.
 */
#define PBC_PSRAM_B             0x5000000

/* PBC Board Control Register 1 bit definitions */
#define PBC_BCTRL1_ERST         0x0001	/* Ethernet Reset */
#define PBC_BCTRL1_URST         0x0002	/* Reset External UART controller */
#define PBC_BCTRL1_UENA         0x0004	/* Enable UART A transceiver */
#define PBC_BCTRL1_UENB         0x0008	/* Enable UART B transceiver */
#define PBC_BCTRL1_UENCE        0x0010	/* Enable UART CE transceiver */
#define PBC_BCTRL1_IREN         0x0020	/* Enable the IRDA transmitter */
#define PBC_BCTRL1_LED0         0x0040	/* Used to control LED 0 (green) */
#define PBC_BCTRL1_LED1         0x0080	/* Used to control LED 1 (yellow) */
#define PBC_BCTRL1_SENSORON     0x0600	/* Enable the Sensor */
#define PBC_BCTRL1_SIMP         0x0800	/* SIM Power Select */
#define PBC_BCTRL1_MCP1         0x1000	/* MMC 1 Power Select */
#define PBC_BCTRL1_MCP2         0x2000	/* MMC 2 Power Select */
#define PBC_BCTRL1_BEND         0x4000	/* Big Endian Select */
#define PBC_BCTRL1_LCDON        0x8000	/* Enable the LCD */

/* PBC Board Control Register 2 bit definitions */
#define PBC_BCTRL2_UH1M         0x0100	/* USB Host 1 input mode */
#define PBC_BCTRL2_USBSP        0x0200	/* USB speed */
#define PBC_BCTRL2_USBSD        0x0400	/* USB Suspend */
/*! @} */

/*!
 * @name  Defines Base address and IRQ used for CS8900A Ethernet Controller on MXC Boards
 */
/*! @{*/
/*! This is I/O Base address used to access registers of CS8900A on MXC ADS */
#define CS8900A_BASE_ADDRESS    (PBC_BASE_ADDRESS + PBC_CS8900A_IOBASE + 0x300)
/*! @} */
#endif /* CONFIG_MACH_SCMA11REF */

#define MXC_PMIC_INT_LINE	INT_EXT_INT5

/*
 * Board specific REF, AHB and IPG frequencies
 */
#define AHB_FREQ                133000000
#define IPG_FREQ                66500000

/*
 * Globals used for IPC channel assignment over SDMA
 */
#define IPC_RX_CHANNEL 1
#define IPC_TX_CHANNEL 2

#endif				/* __ASM_ARM_ARCH_BOARD_SCMA11PHONE_H_ */
