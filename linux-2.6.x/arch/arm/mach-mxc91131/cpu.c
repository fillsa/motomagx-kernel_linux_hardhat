/*
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *  Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*!
 * @file cpu.c
 *
 * @brief This file contains the CPU initialization code.
 *
 * @ingroup System
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/cacheflush.h>
#include <linux/proc_fs.h>

const u32 system_rev_tbl[SYSTEM_REV_NUM][2] = {
	/* SREV, own defined ver */
	{0x00, CHIP_REV_1_0},	/* MXC91131 PASS 1 */
	{0x10, CHIP_REV_2_0},	/* MXC91131 PASS 2 TODO: confirm */
};

/*!
 * CPU initialization. It is called by fixup_mxc_board()
 */
void __init mxc_cpu_init(void)
{
	/* Setup Peripheral Port Remap register for AVIC */
	asm("ldr r0, =0x40000015				\n\
	 mcr p15, 0, r0, c15, c2, 4");
}

/*!
 * Post CPU init code
 *
 * @return 0 always
 */
static int __init post_cpu_init(void)
{
	volatile unsigned long aips_reg;

	/*
	 * S/W workaround: Clear the off platform peripheral modules
	 * Supervisor Protect bit for SDMA to access them.
	 */
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x40));
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x44));
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x48));
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x4C));
	aips_reg = __raw_readl(IO_ADDRESS(AIPS1_BASE_ADDR + 0x50));
	aips_reg &= 0x00FFFFFF;
	__raw_writel(aips_reg, IO_ADDRESS(AIPS1_BASE_ADDR + 0x50));

	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x40));
	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x44));
	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x48));
	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x4C));
	aips_reg = __raw_readl(IO_ADDRESS(AIPS2_BASE_ADDR + 0x50));
	aips_reg &= 0x00FFFFFF;
	__raw_writel(aips_reg, IO_ADDRESS(AIPS2_BASE_ADDR + 0x50));

	/* Make sure CLOCK_TICK_RATE is defined correctly */
	if (mxc_get_clocks(GPT_CLK) != CLOCK_TICK_RATE) {
		printk(KERN_ALERT
		       "\n\n\n!!!!!!!WRONG CLOCK_TICK_RATE!!!!!!!\n");
		printk(KERN_ALERT "It is defined to be %d\n", CLOCK_TICK_RATE);
		printk(KERN_ALERT "But actual rate is %d\n\n\n",
		       (u32) mxc_get_clocks(GPT_CLK));
		mdelay(5000);
	}

	return 0;
}

postcore_initcall(post_cpu_init);

static int mxc_clock_read_proc(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{
	char *p = page;
	int len;

	p += sprintf(p, "MCUPLL:\t\t%lu\n", mxc_pll_clock(MCUPLL));
	p += sprintf(p, "USBPLL:\t\t%lu\n", mxc_pll_clock(USBPLL));
	p += sprintf(p, "DSPPLL:\t\t%lu\n", mxc_pll_clock(DSPPLL));
	p += sprintf(p, "CPU_CLK:\t%lu\n", mxc_get_clocks(CPU_CLK));
	p += sprintf(p, "AHB_CLK:\t%lu\n", mxc_get_clocks(AHB_CLK));
	p += sprintf(p, "IPG_CLK:\t%lu\n", mxc_get_clocks(IPG_CLK));
	p += sprintf(p, "NFC_CLK:\t%lu\n", mxc_get_clocks(NFC_CLK));
	p += sprintf(p, "USB_CLK:\t%lu\n", mxc_get_clocks(USB_CLK));
	p += sprintf(p, "UART1_BAUD:\t%lu\n", mxc_get_clocks(UART1_BAUD));
	p += sprintf(p, "UART2_BAUD:\t%lu\n", mxc_get_clocks(UART2_BAUD));
	p += sprintf(p, "UART3_BAUD:\t%lu\n", mxc_get_clocks(UART3_BAUD));
	p += sprintf(p, "I2C_CLK:\t%lu\n", mxc_get_clocks(I2C_CLK));
	p += sprintf(p, "IPU_CLK:\t%lu\n", mxc_get_clocks(IPU_CLK));
	p += sprintf(p, "SDMA_CLK:\t%lu\n", mxc_get_clocks(SDMA_CLK));
	p += sprintf(p, "SDHC1_CLK:\t%lu\n", mxc_get_clocks(SDHC1_CLK));
	p += sprintf(p, "SDHC2_CLK:\t%lu\n", mxc_get_clocks(SDHC2_CLK));

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

static int __init mxc_setup_proc_entry(void)
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *res;

	res = create_proc_read_entry("cpu/clocks", 0, NULL,
				     mxc_clock_read_proc, NULL);
	if (!res) {
		printk(KERN_ERR "Failed to create proc/cpu/clocks\n");
		return -ENOMEM;
	}
#endif
	return 0;
}

late_initcall(mxc_setup_proc_entry);
