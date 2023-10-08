/*
 *  Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *    - Platform specific register memory map
 *
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
 *
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 09-Oct-2006  Motorola        Added several #defines to support Motorola
 *                              specific features and several WFN bug fixes
 *                              for invalid defines.
 * 03-Nov-2006  Motorola        Adjusted ArgonLV related defines.
 * 02-Jan-2007  Motorola        Added CS3 settings
 * 01-Mar-2007  Motorola        Added FIQ_START for watchdog debug support.
 * 06-Mar-2007  Motorola        Added FSL IPCv2 changes for WFN487
 * 24-Apr-2007  Motorola	Added the definition for 32KHz clock.
 * 09-Aug-2007  Motorola        Added support for new ArgonLV
 */

#ifndef __ASM_ARM_ARCH_MXC_MXC91321_H__
#define __ASM_ARM_ARCH_MXC_MXC91321_H__

#include <linux/config.h>

#include <asm/arch/mxc91321_pins.h>

#ifdef CONFIG_MOT_FEAT_32KHZ_GPT
#define MXC_TIMER_CLK           32768
#define MXC_TIMER_DIVIDER	1
#else
#if defined(CONFIG_MACH_ARGONLVPHONE)
#define MXC_TIMER_CLK           64250000
#define MXC_TIMER_DIVIDER       4
#else
#define MXC_TIMER_CLK           49875000
#define MXC_TIMER_DIVIDER       5
#endif
#endif /* CONFIG_MOT_FEAT_32KHZ_GPT */

#define MU_MUX_GN_INTS          0

/*
 * UART Chip level Configuration that a user may not have to edit. These
 * configuration vary depending on how the UART module is integrated with
 * the ARM core
 */
#define MXC_UART_NR 4
/*!
 * This option is used to set or clear the RXDMUXSEL bit in control reg 3.
 * Certain platforms need this bit to be set in order to receive Irda data.
 */
#define MXC_UART_IR_RXDMUX      0
/*!
 * This option is used to set or clear the RXDMUXSEL bit in control reg 3.
 * Certain platforms need this bit to be set in order to receive UART data.
 */
#define MXC_UART_RXDMUX         0

// end FIXME

/*
 * IRAM
 */
#define IRAM_BASE_ADDR          0x1FFFC000
#define IRAM_BASE_ADDR_VIRT     0xD0000000
#define IRAM_SIZE               SZ_16K

/*
 * L2CC.
 */
#define L2CC_BASE_ADDR          0x30000000
#define L2CC_BASE_ADDR_VIRT     0xD1000000
#define L2CC_SIZE               SZ_64K

/*
 * SMC
 */
#define SMC_BASE_ADDR          0x40000000
#define SMC_BASE_ADDR_VIRT     0xD2000000
#define SMC_SIZE               SZ_16M

/*
 * SiRF
 */
#define SIRF_BASE_ADDR          0x42000000
#define SIRF_BASE_ADDR_VIRT     0xD3000000
#define SIRF_SIZE               SZ_16M

/*
 * AIPS 1
 */
#define AIPS1_BASE_ADDR         0x43F00000
#define AIPS1_BASE_ADDR_VIRT    0xD4000000
#define AIPS1_SIZE              SZ_1M
#define MAX_BASE_ADDR           0x43F04000
#define EVTMON_BASE_ADDR        0x43F08000
#define CLKCTL_BASE_ADDR        0x43F0C000
#define ETB_SLOT4_BASE_ADDR     0x43F10000
#define ETB_SLOT5_BASE_ADDR     0x43F14000
#define ECT_CTIO_BASE_ADDR      0x43F18000
#define I2C_BASE_ADDR           0x43F80000
#define MU_BASE_ADDR            0x43F84000
#define WDOG2_BASE_ADDR         0x43F88000
#define UART1_BASE_ADDR         0x43F90000
#define UART2_BASE_ADDR         0x43F94000
#define OWIRE_BASE_ADDR         0x43F9C000
#define SSI1_BASE_ADDR          0x43FA0000
#define CSPI1_BASE_ADDR         0x43FA4000
#define KPP_BASE_ADDR           0x43FA8000

/*
 * SPBA
 */
#define SPBA0_BASE_ADDR         0x50000000
#define SPBA0_BASE_ADDR_VIRT    0xD4100000
#define SPBA0_SIZE              SZ_1M
#if defined(CONFIG_ARCH_MXC91331)
#define SIM2_BASE_ADDR          0x50000000
#elif defined(CONFIG_ARCH_MXC91321)
#define IOMUXC_BASE_ADDR        0x50000000
#endif
#define MMC_SDHC1_BASE_ADDR     0x50004000
#define MMC_SDHC2_BASE_ADDR     0x50008000
#define UART3_BASE_ADDR         0x5000C000
#define CSPI2_BASE_ADDR         0x50010000
#define SSI2_BASE_ADDR          0x50014000
#define SIM1_BASE_ADDR          0x50018000
#define IIM_BASE_ADDR           0x5001C000
#define USBOTG_BASE_ADDR        0x50020000
#define HAC_BASE_ADDR           0x50024000
#define SAHARA_BASE_ADDR        0x50024000
#define UART4_BASE_ADDR         0x50028000
#define GPIO2_BASE_ADDR         0x5002C000
#if defined(CONFIG_ARCH_MXC91331)
#define IOMUXC_BASE_ADDR        0x50030000
#elif defined(CONFIG_ARCH_MXC91321)
#if defined(CONFIG_MOT_WFN428)
#define SIM2_BASE_ADDR          0x50030000
#else
#error SIM2_BASE_ADDR not defined
#endif
#endif

#define GEMK_BASE_ADDR          0x50034000
#define SDMA_CTI_BASE_ADDR      0x50038000
#define SPBA_CTRL_BASE_ADDR     0x5003C000

/*!
 * Defines for SPBA modules
 */
#ifdef CONFIG_MOT_WFN396
#define SPBA_IOMUX              0x00
#else
#define SPBA_SIM2               0x00
#endif
#define SPBA_SDHC1              0x04
#define SPBA_SDHC2              0x08
#define SPBA_UART3              0x0C
#define SPBA_CSPI2              0x10
#define SPBA_SSI2               0x14
#define SPBA_SIM1               0x18
#define SPBA_IIM                0x1C
#define SPBA_USB_OTG            0x20
#define SPBA_SAHARA             0x24
#define SPBA_HAC                0x24
#define SPBA_UART4              0x28
#define SPBA_GPIO_SDMA          0x2C
#ifdef CONFIG_MOT_WFN396
#define SPBA_SIM2               0x30
#else
#define SPBA_IOMUX              0x30
#endif
#define SPBA_GEM                0x34

/*
 * AIPS 2
 */
#define AIPS2_BASE_ADDR         0x53F00000
#define AIPS2_BASE_ADDR_VIRT    0xD4200000
#define AIPS2_SIZE              SZ_1M
#define CRM_MCU_BASE_ADDR       0x53F80000
#define ECT_MCU_CTI_BASE_ADDR   0x53F84000
#define EDIO_BASE_ADDR          0x53F88000
#define FIRI_BASE_ADDR          0x53F8C000
#define GPT1_BASE_ADDR          0x53F90000
#define EPIT1_BASE_ADDR         0x53F94000
#define EPIT2_BASE_ADDR         0x53F98000
#define SCC_BASE_ADDR           0x53FAC000
#define RNGA_BASE_ADDR          0x53FB0000
#define RTR_BASE_ADDR           0x53FB4000
#define IPU_CTRL_BASE_ADDR      0x53FC0000
#define AUDMUX_BASE_ADDR        0x53FC4000
#define MPEG4_ENC_BASE_ADDR     0x53FC8000
#define GPIO1_BASE_ADDR         0x53FCC000
#define SDMA_BASE_ADDR          0x53FD4000
#define RTC_BASE_ADDR           0x53FD8000
#define WDOG1_BASE_ADDR         0x53FDC000
#define PWM_BASE_ADDR           0x53FE0000
#define RTIC_BASE_ADDR          0x53FEC000

/*
 * ROMP and AVIC
 */
#define ROMP_BASE_ADDR          0x60000000
#define ROMP_BASE_ADDR_VIRT     0xD4300000
#define ROMP_SIZE               SZ_64K

#define AVIC_BASE_ADDR          0x68000000
#define AVIC_BASE_ADDR_VIRT     0xD4310000
#define AVIC_SIZE               SZ_64K

/*
 * NAND, SDRAM, WEIM, M3IF, EMI controllers
 */
#define X_MEMC_BASE_ADDR        0xB8000000
#define X_MEMC_BASE_ADDR_VIRT   0xD4320000
#define X_MEMC_SIZE             SZ_64K

#define NFC_BASE_ADDR           X_MEMC_BASE_ADDR
#define ESDCTL_BASE_ADDR        0xB8001000
#define WEIM_BASE_ADDR          0xB8002000
#define M3IF_BASE_ADDR          0xB8003000

/*
 * Memory regions and CS
 */
#define IPU_MEM_BASE_ADDR       0x70000000
#define CSD0_BASE_ADDR          0x80000000
#define CSD1_BASE_ADDR          0x90000000
#define CS0_BASE_ADDR           0xA0000000
#define CS1_BASE_ADDR           0xA8000000
#define CS2_BASE_ADDR           0xB0000000
#define CS2_BASE_ADDR_VIRT      0xEA000000
#define CS2_SIZE                SZ_16M
#define CS3_BASE_ADDR           0xB2000000
#ifdef CONFIG_MOT_FEAT_ANTIOCH
#define CS3_BASE_ADDR_VIRT      0xEC000000
#define CS3_SIZE                SZ_16M
#endif
#define CS4_BASE_ADDR           0xB4000000
#define CS4_BASE_ADDR_VIRT      0xEB000000
#define CS4_SIZE                SZ_16M
#define CS5_BASE_ADDR           0xB6000000

/*!
 * This macro defines the physical to virtual address mapping for all the
 * peripheral modules. It is used by passing in the physical address as x
 * and returning the virtual address. If the physical address is not mapped,
 * it returns 0xDEADBEEF
 */
#ifdef CONFIG_MOT_FEAT_ANTIOCH
#define IO_ADDRESS(x)   \
        (((x >= IRAM_BASE_ADDR) && (x < (IRAM_BASE_ADDR + IRAM_SIZE))) ? IRAM_IO_ADDRESS(x):\
        ((x >= L2CC_BASE_ADDR) && (x < (L2CC_BASE_ADDR + L2CC_SIZE))) ? L2CC_IO_ADDRESS(x):\
        ((x >= SMC_BASE_ADDR) && (x < (SMC_BASE_ADDR + SMC_SIZE))) ? SMC_IO_ADDRESS(x):\
        ((x >= SIRF_BASE_ADDR) && (x < (SIRF_BASE_ADDR + SIRF_SIZE))) ? SIRF_IO_ADDRESS(x):\
        ((x >= AIPS1_BASE_ADDR) && (x < (AIPS1_BASE_ADDR + AIPS1_SIZE))) ? AIPS1_IO_ADDRESS(x):\
        ((x >= SPBA0_BASE_ADDR) && (x < (SPBA0_BASE_ADDR + SPBA0_SIZE))) ? SPBA0_IO_ADDRESS(x):\
        ((x >= AIPS2_BASE_ADDR) && (x < (AIPS2_BASE_ADDR + AIPS2_SIZE))) ? AIPS2_IO_ADDRESS(x):\
        ((x >= ROMP_BASE_ADDR) && (x < (ROMP_BASE_ADDR + ROMP_SIZE))) ? ROMP_IO_ADDRESS(x):\
        ((x >= AVIC_BASE_ADDR) && (x < (AVIC_BASE_ADDR + AVIC_SIZE))) ? AVIC_IO_ADDRESS(x):\
        ((x >= CS2_BASE_ADDR) && (x < (CS2_BASE_ADDR + CS2_SIZE))) ? CS2_IO_ADDRESS(x):\
        ((x >= CS3_BASE_ADDR) && (x < (CS3_BASE_ADDR + CS3_SIZE))) ? CS3_IO_ADDRESS(x):\
        ((x >= CS4_BASE_ADDR) && (x < (CS4_BASE_ADDR + CS4_SIZE))) ? CS4_IO_ADDRESS(x):\
        ((x >= X_MEMC_BASE_ADDR) && (x < (X_MEMC_BASE_ADDR + X_MEMC_SIZE))) ? X_MEMC_IO_ADDRESS(x):\
        0xDEADBEEF)
#else
#define IO_ADDRESS(x)   \
        (((x >= IRAM_BASE_ADDR) && (x < (IRAM_BASE_ADDR + IRAM_SIZE))) ? IRAM_IO_ADDRESS(x):\
        ((x >= L2CC_BASE_ADDR) && (x < (L2CC_BASE_ADDR + L2CC_SIZE))) ? L2CC_IO_ADDRESS(x):\
        ((x >= SMC_BASE_ADDR) && (x < (SMC_BASE_ADDR + SMC_SIZE))) ? SMC_IO_ADDRESS(x):\
        ((x >= SIRF_BASE_ADDR) && (x < (SIRF_BASE_ADDR + SIRF_SIZE))) ? SIRF_IO_ADDRESS(x):\
        ((x >= AIPS1_BASE_ADDR) && (x < (AIPS1_BASE_ADDR + AIPS1_SIZE))) ? AIPS1_IO_ADDRESS(x):\
        ((x >= SPBA0_BASE_ADDR) && (x < (SPBA0_BASE_ADDR + SPBA0_SIZE))) ? SPBA0_IO_ADDRESS(x):\
        ((x >= AIPS2_BASE_ADDR) && (x < (AIPS2_BASE_ADDR + AIPS2_SIZE))) ? AIPS2_IO_ADDRESS(x):\
        ((x >= ROMP_BASE_ADDR) && (x < (ROMP_BASE_ADDR + ROMP_SIZE))) ? ROMP_IO_ADDRESS(x):\
        ((x >= AVIC_BASE_ADDR) && (x < (AVIC_BASE_ADDR + AVIC_SIZE))) ? AVIC_IO_ADDRESS(x):\
        ((x >= CS2_BASE_ADDR) && (x < (CS2_BASE_ADDR + CS2_SIZE))) ? CS2_IO_ADDRESS(x):\
        ((x >= CS4_BASE_ADDR) && (x < (CS4_BASE_ADDR + CS4_SIZE))) ? CS4_IO_ADDRESS(x):\
        ((x >= X_MEMC_BASE_ADDR) && (x < (X_MEMC_BASE_ADDR + X_MEMC_SIZE))) ? X_MEMC_IO_ADDRESS(x):\
        0xDEADBEEF)
#endif
/*
 * define the address mapping macros: in physical address order
 */

#define IRAM_IO_ADDRESS(x)  \
        (((x) - IRAM_BASE_ADDR) + IRAM_BASE_ADDR_VIRT)

#define L2CC_IO_ADDRESS(x)  \
        (((x) - L2CC_BASE_ADDR) + L2CC_BASE_ADDR_VIRT)

#define SMC_IO_ADDRESS(x)   \
        (((x) - SMC_BASE_ADDR) + SMC_BASE_ADDR_VIRT)

#define SIRF_IO_ADDRESS(x)  \
        (((x) - SIRF_BASE_ADDR) + SIRF_BASE_ADDR_VIRT)

#define AIPS1_IO_ADDRESS(x) \
        (((x) - AIPS1_BASE_ADDR) + AIPS1_BASE_ADDR_VIRT)

#define SPBA0_IO_ADDRESS(x)  \
        (((x) - SPBA0_BASE_ADDR) + SPBA0_BASE_ADDR_VIRT)

#define AIPS2_IO_ADDRESS(x) \
        (((x) - AIPS2_BASE_ADDR) + AIPS2_BASE_ADDR_VIRT)

#define ROMP_IO_ADDRESS(x) \
        (((x) - ROMP_BASE_ADDR) + ROMP_BASE_ADDR_VIRT)

#define AVIC_IO_ADDRESS(x)  \
        (((x) - AVIC_BASE_ADDR) + AVIC_BASE_ADDR_VIRT)

#define CS2_IO_ADDRESS(x)   \
        (((x) - CS2_BASE_ADDR) + CS2_BASE_ADDR_VIRT)

#ifdef CONFIG_MOT_FEAT_ANTIOCH
#define CS3_IO_ADDRESS(x)   \
        (((x) - CS3_BASE_ADDR) + CS3_BASE_ADDR_VIRT)
#endif

#define CS4_IO_ADDRESS(x)   \
        (((x) - CS4_BASE_ADDR) + CS4_BASE_ADDR_VIRT)

#define X_MEMC_IO_ADDRESS(x)  \
        (((x) - X_MEMC_BASE_ADDR) + X_MEMC_BASE_ADDR_VIRT)

/*
 * DMA request assignments
 */
#define DMA_REQ_CTI        31
#define DMA_REQ_NFC        30
#define DMA_REQ_SSI1_TX1   29
#define DMA_REQ_SSI1_RX1   28
#define DMA_REQ_SSI1_TX2   27
#define DMA_REQ_SSI1_RX2   26
#define DMA_REQ_SSI2_TX1   25
#define DMA_REQ_SSI2_RX1   24
#define DMA_REQ_SSI2_TX2   23
#define DMA_REQ_SSI2_RX2   22
#define DMA_REQ_SDHC2      21
#define DMA_REQ_SDHC1      20
#define DMA_REQ_UART1_TX   19
#define DMA_REQ_UART1_RX   18
#define DMA_REQ_UART2_TX   17
#define DMA_REQ_UART2_RX   16
#define DMA_REQ_GPIO_MCU0  15
#define DMA_REQ_GPIO_MCU1  14
#define DMA_REQ_FIRI_TX    13
#define DMA_REQ_FIRI_RX    12
#define DMA_REQ_UART4_TX   13
#define DMA_REQ_UART4_RX   12
#define DMA_REQ_USB2       11
#define DMA_REQ_USB1       10
#define DMA_REQ_CSPI1_TX   9
#define DMA_REQ_CSPI1_RX   8
#define DMA_REQ_CSPI2_TX   7
#define DMA_REQ_CSPI2_RX   6
#define DMA_REQ_SIM1       5
#define DMA_REQ_SIM2       4
#define DMA_REQ_UART3_TX   3
#define DMA_REQ_UART3_RX   2
#ifdef CONFIG_MOT_WFN483
#define DMA_REQ_GEM        1
#else
#define DMA_REQ_CCM        1
#endif
#define DMA_REQ_reserved   0

#ifdef CONFIG_MOT_WFN483
/*!
* Following define the SDMA Channel assigned for SuperGem Script
*/
#define SUPER_GEM_CH       11 
#endif

/*
 * Interrupt numbers
 *
 * TODO: Interrupt numbers for first virtio release.
 * To be finalized once board schematic/Virtio ready.
 */
#define INT_RESV0               0
#define INT_GEM                 1
#define INT_HAC                 2
#define INT_SAHARA              2
#define INT_MU3                 3
#define INT_MU2                 4
#define INT_MU1                 5
#define INT_MU0                 6
#define INT_ELIT2               7
#define INT_MMC_SDHC2           8
#define INT_MMC_SDHC1           9
#define INT_I2C                 10
#define INT_SSI2                11
#define INT_SSI1                12
#define INT_CSPI2               13
#define INT_CSPI1               14
#define INT_EXT_INT7            15
#define INT_UART3               16
#define INT_RESV17              17
#define INT_CCM_MCU_DVFS        17
#define INT_RESV18              18
#define INT_ECT                 19
#define INT_SIM_DATA            20
#define INT_SIM_GENERAL         21
#define INT_RNGA                22
#define INT_RTR                 23
#define INT_KPP                 24
#define INT_RTC                 25
#define INT_PWM                 26
#define INT_EPIT2               27
#define INT_EPIT1               28
#define INT_GPT                 29
#define INT_UART2               30
#define INT_RESV31              31
#define INT_RESV32              32
#define INT_NANDFC              33
#define INT_SDMA                34
#define INT_USBOTG_GRP_ASYNC    35
#define INT_USBOTG_MNP          36
#define INT_USBOTG_HOST         37
#define INT_USBOTG_FUNC         38
#define INT_USBOTG_DMA          39
#define INT_USBOTG_CTRL         40
#define INT_ELIT1               41
#define INT_IPU                 42
#define INT_UART1               43
#define INT_RESV44              44
#define INT_RESV45              45
#define INT_IIM                 46
#define INT_MU_RX_OR            47
#define INT_MU_TX_OR            48
#define INT_SCC_SCM             49
#define INT_EXT_INT6            50
#define INT_GPIOMCU             51
#define INT_GPIO1               INT_GPIOMCU
#define INT_GPIOSDMA            52
#define INT_GPIO2               INT_GPIOSDMA
#define INT_CCM                 53
#define INT_UART4_FIRI_OR       54
#define INT_WDOG2               55
#define INT_SIRF_EXT_INT5_OR    56
#define INT_EXT_INT5            56
#define INT_SIRF_EXT_INT4_OR    57
#define INT_EXT_INT4            57
#define INT_EXT_INT3            58
#define INT_RTIC                59
#define INT_MPEG4_ENC           60
#define INT_HANTRO              60
#define INT_EXT_INT0            61
#define INT_EXT_INT1            62
#define INT_EXT_INT2            63

#define MXC_MAX_INT_LINES       63
#define MXC_MAX_EXT_LINES       8

#ifdef CONFIG_MOT_FEAT_DEBUG_WDOG
#define FIQ_START               0
#endif

/*!
 * Interrupt Number for ARM11 PMU
 */
#define ARM11_PMU_IRQ           INT_RESV0

#define MXC_GPIO_BASE           (MXC_MAX_INT_LINES + 1)

/*!
 * Number of GPIO port as defined in the IC Spec
 */
#define GPIO_PORT_NUM           2
/*!
 * Number of GPIO pins per port
 */
#define GPIO_NUM_PIN            32

#ifdef CONFIG_MOT_FEAT_GPIO_API
#define GPIO_MCU_PORT           0
#define GPIO_SHARED_PORT        1
#endif

#define WEIM_CTRL_CS0           WEIM_BASE_ADDR
#define CSCRU                   0x00
#define CSCRL                   0x04
#define CSCRA                   0x08

#ifdef CONFIG_MOT_FEAT_ANTIOCH
#define WEIM_CTRL_CS3		(WEIM_BASE_ADDR + 0x30)
#define WEIM_CONFIG_REG		(WEIM_BASE_ADDR + 0x60)

/* Defines for CSCRU */
#define WEIM_EDC_SHIFT		0
#define WEIM_WSC_SHIFT		8


/* Defines for CSCRL */
#define WEIM_CSEN_SHIFT		0
#define WEIM_DSZ_SHIFT		8
#define WEIM_EBWN_SHIFT		16
#define WEIM_EBWA_SHIFT		20

/* Defines for CSCRA */
#define WEIM_DWW_SHIFT		6
#define WEIM_EBRN_SHIFT		24
#define WEIM_EBRA_SHIFT		28
#endif

/*
 *  These are useconds NOT ticks.
 *
 */
#define mSEC_1                  1000
#define mSEC_5                  (mSEC_1 * 5)
#define mSEC_10                 (mSEC_1 * 10)
#define mSEC_25                 (mSEC_1 * 25)
#define SEC_1                   (mSEC_1 * 1000)

#define CHIP_REV_1_0            0x10
#ifdef CONFIG_MOT_WFN423
#define CHIP_REV_1_1		0x11
#define CHIP_REV_1_2		0x12
#endif
#define CHIP_REV_1_2_2		0x13
#define CHIP_REV_2_0            0x20
#define CHIP_REV_2_1            0x21
#define CHIP_REV_2_3            0x22
#define CHIP_REV_2_3_2          0x23    

#ifdef CONFIG_ARCH_MXC91331
#define PROD_SIGNATURE          0x4	/* For MXC91331 */
#define SYSTEM_REV_MIN          CHIP_REV_2_0
#define SYSTEM_REV_NUM          2
#endif
#ifdef CONFIG_ARCH_MXC91321
#define PROD_SIGNATURE          0x6	/* For MXC91321 */
#define SYSTEM_REV_MIN          CHIP_REV_1_0
#ifdef CONFIG_MOT_WFN423
#define SYSTEM_REV_NUM          6
#else
#define SYSTEM_REV_NUM          1
#endif
#endif

#endif				/* __ASM_ARM_ARCH_MXC_MXC91321_H__ */
