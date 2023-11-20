/*
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *  Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *  Copyright (C) 2006-2007 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 * 
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  -----------------------
 * 01/05/2006   Motorola  pull in the new drop from MontaVista into our vobs 
 * 07/15/2006   Motorola  pull in the new drop from MontaVista into our vobs
 * 10/06/2006   Motorola  Support for new product
 * 06/11/2007   Motorola  Initialize clocks at boot
 * 07/16/2007   Motorola  Implement Dynamic Clock Gating (SPI)
 * 08/09/2007   Motorola  Support for new Argon IC
 * 09/05/2007   Motorola  Gate off I2C clock at boot
 * 10/12/2007   Motorola  Gate off sahara clock at boot
 * 10/17/2007   Motorola  Change clock initialization
 * 10/24/2007	Motorola  Keep CSPI clock on at boot
 * 10/31/2007   Motorola  Enable SDHC clock
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

#ifdef CONFIG_MOT_FEAT_PM
#include "crm_regs.h"
#endif

#ifdef CONFIG_ARCH_MXC91331
const u32 system_rev_tbl[SYSTEM_REV_NUM][2] = {
	/* SREV, own defined ver */
	{0x00, CHIP_REV_2_0},
	{0x40, CHIP_REV_2_1},	/* MXC91331 PASS 2.1 */
};
#endif
#ifdef CONFIG_ARCH_MXC91321
const u32 system_rev_tbl[SYSTEM_REV_NUM][2] = {
	/* SREV, own defined ver */
	{0x00, CHIP_REV_1_0},
#ifdef CONFIG_MOT_WFN423
	{0x11, CHIP_REV_1_1},
	{0x22, CHIP_REV_1_2},
#endif
	{0x24, CHIP_REV_1_2_2},
	{0x50, CHIP_REV_2_3},	
	{0x52, CHIP_REV_2_3_2},	
};
#endif

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
	if (mxc_get_clocks(GPT_CLK) / MXC_TIMER_DIVIDER != CLOCK_TICK_RATE) {
		printk(KERN_ALERT
		       "\n\n\n!!!!!!!WRONG CLOCK_TICK_RATE!!!!!!!\n");
		printk(KERN_ALERT "It is defined to be %d\n", CLOCK_TICK_RATE);
		printk(KERN_ALERT "But actual rate is %d\n\n\n",
		       (u32) mxc_get_clocks(GPT_CLK));
		mdelay(5000);
	}

#ifdef CONFIG_MOT_FEAT_PM
        /* Initialize clocks that drivers aren't handling yet */
        /* MCGR0 */
        mxc_clks_disable(I2C_CLK);
//        mxc_clks_disable(CSPI1_CLK); // * 10/24/2007	Motorola  Keep CSPI clock on at boot
        mxc_clks_enable(FIRI_BAUD);
        mxc_clks_disable(EPIT2_CLK);
        mxc_clks_enable(EDIO_CLK);
        mxc_clks_enable(PWM_CLK);
        mxc_clks_enable(OWIRE_CLK);
        mxc_clks_disable(SSI1_BAUD);

        /* MCGR1 */
        mxc_clks_disable(MPEG4_CLK);
        mxc_clks_enable(IIM_CLK);
        mxc_clks_enable(SIM1_CLK);
        mxc_clks_enable(SIM2_CLK);
        mxc_clks_enable(GEM_CLK);
        mxc_clks_enable(USB_CLK);
//        mxc_clks_disable(CSPI2_CLK); // * 10/24/2007	Motorola  Keep CSPI clock on at boot
        mxc_clks_enable(SDHC2_CLK);
        mxc_clks_enable(SDHC1_CLK);
        mxc_clks_disable(SSI2_BAUD);

        /* MCGR2 */
        mxc_clks_enable(SDMA_CLK);
        mxc_clks_enable(RTRMCU_CLK);
        mxc_clks_enable(KPP_CLK);
        mxc_clks_enable(ECT_CLK);
        mxc_clks_enable(SMC_CLK);
        mxc_clks_enable(RTIC_CLK);
        mxc_clks_enable(SPBA_CLK);
        mxc_clks_disable(SAHARA_CLK);
#endif

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
