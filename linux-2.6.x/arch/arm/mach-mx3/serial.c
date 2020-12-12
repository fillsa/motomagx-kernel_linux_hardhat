/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */
/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*!
 * @file serial.c
 *
 * @brief This file contains the UART initiliazation.
 *
 * @ingroup System
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/serial.h>
#include <asm/hardware.h>
#include <asm/arch/mxc_uart.h>
#include <asm/arch/spba.h>

#if defined(CONFIG_SERIAL_MXC) || defined(CONFIG_SERIAL_MXC_MODULE)

/* UART 1 configuration */
/*!
 * This option allows to choose either an interrupt-driven software controlled
 * hardware flow control (set this option to 0) or hardware-driven hardware
 * flow control (set this option to 1).
 */
#define UART1_HW_FLOW           1
/*!
 * This specifies the threshold at which the CTS pin is deasserted by the
 * RXFIFO. Set this value in Decimal to anything from 0 to 32 for
 * hardware-driven hardware flow control. Read the HW spec while specifying
 * this value. When using interrupt-driven software controlled hardware
 * flow control set this option to -1.
 */
#define UART1_UCR4_CTSTL        16
/*!
 * This is option to enable (set this option to 1) or disable DMA data transfer
 */
#define UART1_DMA_ENABLE        0
/*!
 * Specify the size of the DMA receive buffer. The minimum buffer size is 512
 * bytes. The buffer size should be a multiple of 256.
 */
#define UART1_DMA_RXBUFSIZE     1024
/*!
 * Specify the MXC UART's Receive Trigger Level. This controls the threshold at
 * which a maskable interrupt is generated by the RxFIFO. Set this value in
 * Decimal to anything from 0 to 32. Read the HW spec while specifying this
 * value.
 */
#define UART1_UFCR_RXTL         16
/*!
 * Specify the MXC UART's Transmit Trigger Level. This controls the threshold at
 * which a maskable interrupt is generated by the TxFIFO. Set this value in
 * Decimal to anything from 0 to 32. Read the HW spec while specifying this
 * value.
 */
#define UART1_UFCR_TXTL         16
/* UART 2 configuration */
#define UART2_HW_FLOW           0
#define UART2_UCR4_CTSTL        -1
#define UART2_DMA_ENABLE        0
#define UART2_DMA_RXBUFSIZE     512
#define UART2_UFCR_RXTL         16
#define UART2_UFCR_TXTL         16
/* UART 3 configuration */
#define UART3_HW_FLOW           1
#define UART3_UCR4_CTSTL        16
#define UART3_DMA_ENABLE        1
#define UART3_DMA_RXBUFSIZE     1024
#define UART3_UFCR_RXTL         16
#define UART3_UFCR_TXTL         16
/* UART 4 configuration */
#define UART4_HW_FLOW           1
#define UART4_UCR4_CTSTL        16
#define UART4_DMA_ENABLE        0
#define UART4_DMA_RXBUFSIZE     512
#define UART4_UFCR_RXTL         16
#define UART4_UFCR_TXTL         16
/* UART 5 configuration */
#define UART5_HW_FLOW           1
#define UART5_UCR4_CTSTL        16
#define UART5_DMA_ENABLE        0
#define UART5_DMA_RXBUFSIZE     512
#define UART5_UFCR_RXTL         16
#define UART5_UFCR_TXTL         16

/*
 * UART Chip level Configuration that a user may not have to edit. These
 * configuration vary depending on how the UART module is integrated with
 * the ARM core
 */
/*
 * Is the MUXED interrupt output sent to the ARM core
 */
#define INTS_NOTMUXED           0
#define INTS_MUXED              1

/* UART 1 configuration */
/*!
 * This define specifies whether the muxed ANDed interrupt line or the
 * individual interrupts from the UART port is integrated with the ARM core.
 * There exists a define like this for each UART port. Valid values that can
 * be used are \b INTS_NOTMUXED or \b INTS_MUXED.
 */
#define UART1_MUX_INTS          INTS_MUXED
/*!
 * This define specifies the transmitter interrupt number or the interrupt
 * number of the ANDed interrupt in case the interrupts are muxed. There exists
 * a define like this for each UART port.
 */
#define UART1_INT1              INT_UART1
/*!
 * This define specifies the receiver interrupt number. If the interrupts of
 * the UART are muxed, then we specify here a dummy value -1. There exists a
 * define like this for each UART port.
 */
#define UART1_INT2              -1
/*!
 * This specifies the master interrupt number. If the interrupts of the UART
 * are muxed, then we specify here a dummy value of -1. There exists a define
 * like this for each UART port.
 */
#define UART1_INT3              -1
/*!
 * This specifies if the UART is a shared peripheral. It holds the shared
 * peripheral number if it is shared or -1 if it is not shared. There exists
 * a define like this for each UART port.
 */
#define UART1_SHARED_PERI       -1
/* UART 2 configuration */
#define UART2_MUX_INTS          INTS_MUXED
#define UART2_INT1              INT_UART2
#define UART2_INT2              -1
#define UART2_INT3              -1
#define UART2_SHARED_PERI       -1
/* UART 3 configuration */
#define UART3_MUX_INTS          INTS_MUXED
#define UART3_INT1              INT_UART3
#define UART3_INT2              -1
#define UART3_INT3              -1
#define UART3_SHARED_PERI       SPBA_UART3
/* UART 4 configuration */
#define UART4_MUX_INTS          INTS_MUXED
#define UART4_INT1              INT_UART4
#define UART4_INT2              -1
#define UART4_INT3              -1
#define UART4_SHARED_PERI       -1
/* UART 5 configuration */
#define UART5_MUX_INTS          INTS_MUXED
#define UART5_INT1              INT_UART5
#define UART5_INT2              -1
#define UART5_INT3              -1
#define UART5_SHARED_PERI       -1

/*!
 * This is an array where each element holds information about a UART port,
 * like base address of the UART, interrupt numbers etc. This structure is
 * passed to the serial_core.c file. Based on which UART is used, the core file
 * passes back the appropriate port structure as an argument to the control
 * functions.
 */
static uart_mxc_port mxc_ports[] = {
	[0] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART1_BASE_ADDR),
			.mapbase = UART1_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART1_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 0,
			},
	       .ints_muxed = UART1_MUX_INTS,
	       .irqs = {UART1_INT2, UART1_INT3},
	       .mode = UART1_MODE,
	       .ir_mode = UART1_IR,
	       .enabled = UART1_ENABLED,
	       .hardware_flow = UART1_HW_FLOW,
	       .cts_threshold = UART1_UCR4_CTSTL,
	       .dma_enabled = UART1_DMA_ENABLE,
	       .dma_rxbuf_size = UART1_DMA_RXBUFSIZE,
	       .rx_threshold = UART1_UFCR_RXTL,
	       .tx_threshold = UART1_UFCR_TXTL,
	       .shared = UART1_SHARED_PERI,
	       .clock_id = UART1_BAUD,
	       },
	[1] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART2_BASE_ADDR),
			.mapbase = UART2_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART2_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 1,
			},
	       .ints_muxed = UART2_MUX_INTS,
	       .irqs = {UART2_INT2, UART2_INT3},
	       .mode = UART2_MODE,
	       .ir_mode = UART2_IR,
	       .enabled = UART2_ENABLED,
	       .hardware_flow = UART2_HW_FLOW,
	       .cts_threshold = UART2_UCR4_CTSTL,
	       .dma_enabled = UART2_DMA_ENABLE,
	       .dma_rxbuf_size = UART2_DMA_RXBUFSIZE,
	       .rx_threshold = UART2_UFCR_RXTL,
	       .tx_threshold = UART2_UFCR_TXTL,
	       .shared = UART2_SHARED_PERI,
	       .clock_id = UART2_BAUD,
	       },
	[2] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART3_BASE_ADDR),
			.mapbase = UART3_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART3_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 2,
			},
	       .ints_muxed = UART3_MUX_INTS,
	       .irqs = {UART3_INT2, UART3_INT3},
	       .mode = UART3_MODE,
	       .ir_mode = UART3_IR,
	       .enabled = UART3_ENABLED,
	       .hardware_flow = UART3_HW_FLOW,
	       .cts_threshold = UART3_UCR4_CTSTL,
	       .dma_enabled = UART3_DMA_ENABLE,
	       .dma_rxbuf_size = UART3_DMA_RXBUFSIZE,
	       .rx_threshold = UART3_UFCR_RXTL,
	       .tx_threshold = UART3_UFCR_TXTL,
	       .shared = UART3_SHARED_PERI,
	       .clock_id = UART3_BAUD,
	       },
	[3] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART4_BASE_ADDR),
			.mapbase = UART4_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART4_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 3,
			},
	       .ints_muxed = UART4_MUX_INTS,
	       .irqs = {UART4_INT2, UART4_INT3},
	       .mode = UART4_MODE,
	       .ir_mode = UART4_IR,
	       .enabled = UART4_ENABLED,
	       .hardware_flow = UART4_HW_FLOW,
	       .cts_threshold = UART4_UCR4_CTSTL,
	       .dma_enabled = UART4_DMA_ENABLE,
	       .dma_rxbuf_size = UART4_DMA_RXBUFSIZE,
	       .rx_threshold = UART4_UFCR_RXTL,
	       .tx_threshold = UART4_UFCR_TXTL,
	       .shared = UART4_SHARED_PERI,
	       .clock_id = UART4_BAUD,
	       },
	[4] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART5_BASE_ADDR),
			.mapbase = UART5_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART5_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 4,
			},
	       .ints_muxed = UART5_MUX_INTS,
	       .irqs = {UART5_INT2, UART5_INT3},
	       .mode = UART5_MODE,
	       .ir_mode = UART5_IR,
	       .enabled = UART5_ENABLED,
	       .hardware_flow = UART5_HW_FLOW,
	       .cts_threshold = UART5_UCR4_CTSTL,
	       .dma_enabled = UART5_DMA_ENABLE,
	       .dma_rxbuf_size = UART5_DMA_RXBUFSIZE,
	       .rx_threshold = UART5_UFCR_RXTL,
	       .tx_threshold = UART5_UFCR_TXTL,
	       .shared = UART5_SHARED_PERI,
	       .clock_id = UART5_BAUD,
	       },
};

static struct platform_device mxc_uart_device1 = {
	.name = "mxcintuart",
	.id = 0,
	.dev = {
		.platform_data = &mxc_ports[0],
		},
};

static struct platform_device mxc_uart_device2 = {
	.name = "mxcintuart",
	.id = 1,
	.dev = {
		.platform_data = &mxc_ports[1],
		},
};

static struct platform_device mxc_uart_device3 = {
	.name = "mxcintuart",
	.id = 2,
	.dev = {
		.platform_data = &mxc_ports[2],
		},
};

static struct platform_device mxc_uart_device4 = {
	.name = "mxcintuart",
	.id = 3,
	.dev = {
		.platform_data = &mxc_ports[3],
		},
};

static struct platform_device mxc_uart_device5 = {
	.name = "mxcintuart",
	.id = 4,
	.dev = {
		.platform_data = &mxc_ports[4],
		},
};

static int __init mxc_init_uart(void)
{
	/* Register all the MXC UART platform device structures */
	platform_device_register(&mxc_uart_device1);
	platform_device_register(&mxc_uart_device2);

	/* Grab ownership of shared UARTs 3 and 4, only when enabled */
#if UART3_ENABLED == 1
#if UART3_DMA_ENABLE == 1
	spba_take_ownership(UART3_SHARED_PERI, (SPBA_MASTER_A | SPBA_MASTER_C));
#else
	spba_take_ownership(UART3_SHARED_PERI, SPBA_MASTER_A);
#endif				/* UART3_DMA_ENABLE */
	platform_device_register(&mxc_uart_device3);
#endif				/* UART3_ENABLED */

#if UART4_ENABLED == 1
	platform_device_register(&mxc_uart_device4);
#endif				/* UART4_ENABLED */

#if UART5_ENABLED == 1
	platform_device_register(&mxc_uart_device5);
#endif				/* UART5_ENABLED */
	return 0;
}

#else
static int __init mxc_init_uart(void)
{
	return 0;
}
#endif

arch_initcall(mxc_init_uart);