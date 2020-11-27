/*
 * Copyright 2005 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __ASM_ARM_ARCH_BOARD_EVB_H_
#define __ASM_ARM_ARCH_BOARD_EVB_H_

/*
 * Include Files
 */
#include <linux/config.h>
#include <asm/arch/board.h>

/* Start of physical RAM */
#define PHYS_OFFSET	        (0x80000000UL)

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
#define I2C1_FRQ_DIV            0x32

/*!
 * @name MXC UART EVB board level configurations
 */
/*! @{ */
/*!
 * Specify the max baudrate for the MXC UARTs for your board, do not specify a max
 * baudrate greater than 1500000. This is used while specifying the UART Power
 * management constraints.
 */
#define MAX_UART_BAUDRATE       1500000
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
/* UART 4 configuration */
#define UART4_MODE              MODE_DCE
#define UART4_IR                IRDA
#ifdef CONFIG_MXC_FIR_MODULE
#define UART4_ENABLED           0
#else
#define UART4_ENABLED           1
#endif

#define MXC_LL_UART_PADDR	UART2_BASE_ADDR
#define MXC_LL_UART_VADDR	AIPS1_IO_ADDRESS(UART2_BASE_ADDR)
#define MXC_LL_EXT_UART_16BIT_BUS	1

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
 */
#define MXCIPU_MEM_SIZE         SZ_8M
/*!
 * Size of memory available to kernel
 */
#define MEM_SIZE                (SDRAM_MEM_SIZE - MXCIPU_MEM_SIZE)
/*!
 * Physical address start of IPU buffer memory
 */
#define MXCIPU_MEM_ADDRESS      (PHYS_OFFSET + MEM_SIZE)
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
/*! This is System IRQ used by CS8900A for interrupt generation */
#define CS8900AIRQ              INT_EXT_INT5
/*! This is I/O Base address used to access registers of CS8900A on MXC ADS */
#define CS8900A_BASE_ADDRESS    (PBC_BASE_ADDRESS + PBC_CS8900A_IOBASE + 0x300)
/*! @} */

#define MXC_PMIC_INT_LINE	INT_EXT_INT1

#endif				/* __ASM_ARM_ARCH_BOARD_EVB_H_ */
