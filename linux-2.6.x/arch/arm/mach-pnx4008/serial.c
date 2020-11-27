/*
 *  linux/arch/arm/mach-pnx4008/serial.c
 *
 *  PNX4008 UART initialization
 *
 *  Copyright:	MontaVista Software Inc. (c) 2005
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>

#include <asm/io.h>

#include <asm/arch/platform.h>
#include <asm/arch/hardware.h>

#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <asm/arch/pm.h>

#include "clock.h"

#define UART_3		0
#define UART_4		1
#define UART_5		2
#define UART_6		3
#define UART_UNKNOWN	(-1)

#define UART3_BASE_VA	IO_ADDRESS(PNX4008_UART3_BASE)
#define UART4_BASE_VA	IO_ADDRESS(PNX4008_UART4_BASE)
#define UART5_BASE_VA	IO_ADDRESS(PNX4008_UART5_BASE)
#define UART6_BASE_VA	IO_ADDRESS(PNX4008_UART6_BASE)

#define UART_DIVS_VA	IO_ADDRESS(PNX4008_PWRMAN_BASE + 0xd0)

#define UART_FCR_OFFSET		8
#define UART_FIFO_SIZE		64

#define make_pair(X, Y)	(((u16)(X) << 8) | (u16)(Y))
#define DIVS_STOP	make_pair(0, 0)
#define DIVS_1		make_pair(19, 134)	/* 115.2 kbs max */
#define DIVS_2		make_pair(38, 67)	/* 460.8 kbs max */
#define DIVS_DIRECT	make_pair(1, 1)

#define UART_CLK_VA	IO_ADDRESS(PNX4008_UARTCTRL_BASE + 0x4)
#define OFF_MODE	0x0
#define ON_MODE		0x1
#define AUTO_MODE	0x2

static void set_divs(int i, u16 divs)
{
	u32 address = UART_DIVS_VA + 4 * i;
	__raw_writew(divs, address);
}

static void set_mode(int i, int mode)
{
	int shift = 2 * i + 4;
	u32 mask = 0x3 << shift;
	u32 t;

	t = __raw_readl(UART_CLK_VA);
	t = (t & ~mask) | ((u32) mode << shift);
	__raw_writel(t, UART_CLK_VA);
}

static void uart_switch_off(int i)
{
	char name[10];
	struct clk *clk;

	sprintf(name, "uart%d_ck", i + 3);	/* starting from uart3 */

	clk = clk_get(0, name);
	if (!IS_ERR(clk)) {
		set_divs(i, DIVS_STOP);
		set_mode(i, OFF_MODE);

		clk_set_rate(clk, 0);
		clk_put(clk);
	} else
		printk("failed to stop uart%d\n", i + 3);
}

static void uart_switch_on(int i)
{
	char name[10];
	struct clk *clk;

	sprintf(name, "uart%d_ck", i + 3);	/* starting from uart3 */

	clk = clk_get(0, name);
	if (!IS_ERR(clk)) {
		clk_set_rate(clk, 1);
		clk_put(clk);

		set_mode(i, ON_MODE);
		set_divs(i, DIVS_DIRECT);
	} else
		printk("failed to start uart%d\n", i + 3);
}

void pnx4008_uart_init(void)
{
	u32 tmp;
	int i = UART_FIFO_SIZE;

	__raw_writel(0xC1, UART5_BASE_VA + UART_FCR_OFFSET);
	__raw_writel(0xC1, UART3_BASE_VA + UART_FCR_OFFSET);

	/* Send a NULL to fix the UART HW bug */
	__raw_writel(0x00, UART5_BASE_VA);
	__raw_writel(0x00, UART3_BASE_VA);

	while (i--) {
		tmp = __raw_readl(UART5_BASE_VA);
		tmp = __raw_readl(UART3_BASE_VA);
	}
	__raw_writel(0, UART5_BASE_VA + UART_FCR_OFFSET);
	__raw_writel(0, UART3_BASE_VA + UART_FCR_OFFSET);

	/* setup wakeup interrupt */
	start_int_set_rising_edge(SE_U3_RX_INT);
	start_int_ack(SE_U3_RX_INT);
	start_int_umask(SE_U3_RX_INT);

	start_int_set_rising_edge(SE_U5_RX_INT);
	start_int_ack(SE_U5_RX_INT);
	start_int_umask(SE_U5_RX_INT);
}

static int uart_id(struct uart_port *port)
{
	int ret = UART_UNKNOWN;

	switch ((u32) port->membase) {
	case UART3_BASE_VA:
		ret = UART_3;
		break;

	case UART4_BASE_VA:
		ret = UART_4;
		break;

	case UART5_BASE_VA:
		ret = UART_5;
		break;

	case UART6_BASE_VA:
		ret = UART_6;
		break;

	default:
		ret = UART_UNKNOWN;
		break;
	}

	return ret;
}

void pnx4008_uart_pm(struct uart_port *port,
		     unsigned int state, unsigned int oldstate)
{
	int i = uart_id(port);

	if (i == UART_UNKNOWN)
		return;

	(state) ? uart_switch_off(i) : uart_switch_on(i);
}
