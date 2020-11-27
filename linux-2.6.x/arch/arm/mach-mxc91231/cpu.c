/*
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *  Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *  Copyright (C) 2007-2008 Motorola, Inc.
 *  
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
 * 02111-1307, USA
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 05/11/2007         Motorola         Enable PLL2 Lock/Unlock Change
 * 02/28/2008         Motorola         Fix flicker when showing logo
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
#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/cacheflush.h>
#include <linux/proc_fs.h>

#ifdef CONFIG_MOT_FEAT_PM
#include "crm_regs.h"
#endif
const u32 system_rev_tbl[SYSTEM_REV_NUM][2] = {
	/* SREV, own defined ver */
	{0x00, CHIP_REV_1_0},	/* MXC91231 PASS 1 */
	{0x10, CHIP_REV_2_0},	/* MXC91231 PASS 2 */
	{0x11, CHIP_REV_2_1},	/* MXC91231 PASS 2.1 */
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

#ifdef CONFIG_MOT_FEAT_PM
	unsigned long clk_gate_value;
#endif

	if (system_rev == CHIP_REV_1_0) {
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
	}

#ifdef CONFIG_MOT_FEAT_PM
	
        /* MXC_CRMAP_Axxx is defined as IO_ADDRESS(CRM_AP_BASE_ADDR + 0xnn) */

        /* ACGCR */

	clk_gate_value = __raw_readl(MXC_CRMAP_ACGCR);
	
	clk_gate_value &= 
		~(MXC_ACGCR_ACG0_RUN | MXC_ACGCR_ACG1_RUN |
		  MXC_ACGCR_ACG2_RUN | MXC_ACGCR_ACG3_RUN |
		  MXC_ACGCR_ACG4_RUN | MXC_ACGCR_ACG5_RUN |
		  MXC_ACGCR_ACG6_RUN | MXC_ACGCR_ACG7_RUN);

	clk_gate_value |= 
		(MXC_ACGCR_ACG0_STOP | MXC_ACGCR_ACG1_STOP |
		 MXC_ACGCR_ACG2_STOP | MXC_ACGCR_ACG3_STOP |
		 MXC_ACGCR_ACG4_STOP | MXC_ACGCR_ACG5_STOP |
		 MXC_ACGCR_ACG6_STOP | MXC_ACGCR_ACG7_STOP);

	__raw_writel(clk_gate_value, MXC_CRMAP_ACGCR);

	/* ACCGCR */

	clk_gate_value = __raw_readl(MXC_CRMAP_ACCGCR);

	clk_gate_value &= 
		~(MXC_CRMAP_ACCGCR_ACCG0_MASK | MXC_CRMAP_ACCGCR_ACCG1_MASK |
		  MXC_CRMAP_ACCGCR_ACCG2_MASK);

	clk_gate_value |= 
		(ACCGCR_0_VALUE | ACCGCR_1_VALUE |
		 ACCGCR_2_VALUE);

	__raw_writel(clk_gate_value, MXC_CRMAP_ACCGCR);

	/* AMLPMRA */

	clk_gate_value = __raw_readl(MXC_CRMAP_AMLPMRA);

	clk_gate_value &= 
		~(MXC_CRMAP_AMLPMRA_MLPMA1_MASK | MXC_CRMAP_AMLPMRA_MLPMA2_MASK | 
		  MXC_CRMAP_AMLPMRA_MLPMA3_MASK | MXC_CRMAP_AMLPMRA_MLPMA4_MASK | 
		  MXC_CRMAP_AMLPMRA_MLPMA6_MASK | MXC_CRMAP_AMLPMRA_MLPMA7_MASK_PASS2);

	clk_gate_value |= 
		(AMLPMRA_1_VALUE | AMLPMRA_2_VALUE | 
		 AMLPMRA_3_VALUE | AMLPMRA_4_VALUE | 
		 AMLPMRA_6_VALUE | AMLPMRA_7_VALUE);

	__raw_writel(clk_gate_value, MXC_CRMAP_AMLPMRA);

	/* AMLPMRB */

	clk_gate_value = __raw_readl(MXC_CRMAP_AMLPMRB);

	clk_gate_value &= ~(MXC_CRMAP_AMLPMRB_MLPMB0_MASK);

	clk_gate_value |= AMLPMRB_0_VALUE;

	__raw_writel(clk_gate_value, MXC_CRMAP_AMLPMRB);


	/* AMLPMRC */

	clk_gate_value = __raw_readl(MXC_CRMAP_AMLPMRC);

	clk_gate_value &= 
		~(MXC_CRMAP_AMLPMRC_MLPMC0_MASK | MXC_CRMAP_AMLPMRC_MLPMC1_MASK |
		  MXC_CRMAP_AMLPMRC_MLPMC2_MASK | MXC_CRMAP_AMLPMRC_MLPMC3_MASK |
		  MXC_CRMAP_AMLPMRC_MLPMC4_MASK | MXC_CRMAP_AMLPMRC_MLPMC5_MASK |
		  MXC_CRMAP_AMLPMRC_MLPMC7_MASK | MXC_CRMAP_AMLPMRC_MLPMC9_MASK);

	clk_gate_value |= 
		(AMLPMRC_0_VALUE | AMLPMRC_1_VALUE |
		 AMLPMRC_2_VALUE | AMLPMRC_3_VALUE |
		 AMLPMRC_4_VALUE | AMLPMRC_5_VALUE |
		 AMLPMRC_7_VALUE | AMLPMRC_9_VALUE);

	__raw_writel(clk_gate_value, MXC_CRMAP_AMLPMRC);


	/* AMLPMRD */

	clk_gate_value = __raw_readl(MXC_CRMAP_AMLPMRD);

	clk_gate_value &= 
		~(MXC_CRMAP_AMLPMRD_MLPMD2_MASK | MXC_CRMAP_AMLPMRD_MLPMD3_MASK |
		  MXC_CRMAP_AMLPMRD_MLPMD4_MASK | MXC_CRMAP_AMLPMRD_MLPMD7_MASK);
        
	clk_gate_value |= 
		(AMLPMRD_2_VALUE | AMLPMRD_3_VALUE |
		 AMLPMRD_4_VALUE | AMLPMRD_7_VALUE);

	__raw_writel(clk_gate_value, MXC_CRMAP_AMLPMRD);


	/* AMLPMRE1 */

	clk_gate_value = __raw_readl(MXC_CRMAP_AMLPMRE1);

	clk_gate_value &= 
		~(MXC_CRMAP_AMLPMRE1_MLPME0_MASK | MXC_CRMAP_AMLPMRE1_MLPME1_MASK |
		  MXC_CRMAP_AMLPMRE1_MLPME2_MASK_PASS2 | MXC_CRMAP_AMLPMRE1_MLPME3_MASK |
		  MXC_CRMAP_AMLPMRE1_MLPME4_MASK | MXC_CRMAP_AMLPMRE1_MLPME5_MASK |
		  MXC_CRMAP_AMLPMRE1_MLPME6_MASK | MXC_CRMAP_AMLPMRE1_MLPME7_MASK);

	clk_gate_value |= 
		(AMLPMRE1_0_VALUE | AMLPMRE1_1_VALUE |
		 AMLPMRE1_2_VALUE | AMLPMRE1_3_VALUE |
		 AMLPMRE1_4_VALUE | AMLPMRE1_5_VALUE |
		 AMLPMRE1_6_VALUE | AMLPMRE1_7_VALUE);

	__raw_writel(clk_gate_value, MXC_CRMAP_AMLPMRE1);


	/* AMLPMRE2 */

	clk_gate_value = __raw_readl(MXC_CRMAP_AMLPMRE2);
 
	clk_gate_value &= ~(MXC_CRMAP_AMLPMRE2_MLPME0_MASK);

	clk_gate_value |= AMLPMRE2_0_VALUE;

	__raw_writel(clk_gate_value, MXC_CRMAP_AMLPMRE2);


	/* AMLPMRF */

	clk_gate_value = __raw_readl(MXC_CRMAP_AMLPMRF);

	clk_gate_value &= 
		~(MXC_CRMAP_AMLPMRF_MLPMF0_MASK_PASS2 | MXC_CRMAP_AMLPMRF_MLPMF1_MASK |
		  MXC_CRMAP_AMLPMRF_MLPMF2_MASK | MXC_CRMAP_AMLPMRF_MLPMF3_MASK |
		  MXC_CRMAP_AMLPMRF_MLPMF5_MASK | MXC_CRMAP_AMLPMRF_MLPMF6_MASK);

	clk_gate_value |= 
		(AMLPMRF_0_VALUE | AMLPMRF_1_VALUE |
		 AMLPMRF_2_VALUE | AMLPMRF_3_VALUE |
		 AMLPMRF_5_VALUE | AMLPMRF_6_VALUE);

	__raw_writel(clk_gate_value, MXC_CRMAP_AMLPMRF);


	/* Set AMLPMRG */

	clk_gate_value = __raw_readl(MXC_CRMAP_AMLPMRG);

	clk_gate_value &= 
		~(MXC_CRMAP_AMLPMRG_MLPMG0_MASK | MXC_CRMAP_AMLPMRG_MLPMG1_MASK |
		  MXC_CRMAP_AMLPMRG_MLPMG2_MASK | MXC_CRMAP_AMLPMRG_MLPMG3_MASK |
		  MXC_CRMAP_AMLPMRG_MLPMG4_MASK | MXC_CRMAP_AMLPMRG_MLPMG5_MASK |
		  MXC_CRMAP_AMLPMRG_MLPMG6_MASK | MXC_CRMAP_AMLPMRG_MLPMG7_MASK |
		  MXC_CRMAP_AMLPMRG_MLPMG9_MASK);

	clk_gate_value |= 
		(AMLPMRG_0_VALUE | AMLPMRG_1_VALUE |
		 AMLPMRG_2_VALUE | AMLPMRG_3_VALUE |
		 AMLPMRG_4_VALUE | AMLPMRG_5_VALUE |
		 AMLPMRG_6_VALUE | AMLPMRG_7_VALUE |
		 AMLPMRG_9_VALUE);

	__raw_writel(clk_gate_value, MXC_CRMAP_AMLPMRG);


#if 0  /* breaks the phone */
	/* APGCR */

	clk_gate_value = __raw_readl(MXC_CRMAP_APGCR);

	clk_gate_value &= ~(MXC_CRMAP_APGCR_PGC0_MASK | MXC_CRMAP_APGCR_PGC1_MASK);

	clk_gate_value |= (APGCR_0_VALUE | APGCR_1_VALUE);

	__raw_writel(clk_gate_value, MXC_CRMAP_APGCR);

	/* APRA */

	clk_gate_value = __raw_readl(MXC_CRMAP_APRA);

        /* Single bits all enabled - no need to mask */
	clk_gate_value |= 
		(MXC_CRMAP_APRA_UART1EN | MXC_CRMAP_APRA_UART2EN | 
		 MXC_CRMAP_APRA_SAHARA_DIV2_CLK_EN_PASS2 | 
		 MXC_CRMAP_APRA_MQSPI_EN_PASS2 | MXC_CRMAP_APRA_UART3EN | 
		 MXC_CRMAP_APRA_SIMEN | MXC_CRMAP_APRA_EL1T_EN_PASS2);

	__raw_writel(clk_gate_value, MXC_CRMAP_APRA);

	/* APRB */

	clk_gate_value = __raw_readl(MXC_CRMAP_APRB);

        /* Single bits all enabled - no need to mask */
	clk_gate_value |= 
		(MXC_CRMAP_APRB_SDHC1EN | MXC_CRMAP_APRB_SDHC2EN);

	__raw_writel(clk_gate_value, MXC_CRMAP_APRB);

	/* ACDER1 */

	clk_gate_value = __raw_readl(MXC_CRMAP_ACDER1);

	clk_gate_value &= 
		~(MXC_CRMAP_ACDER1_CSIEN_PASS2 | MXC_CRMAP_ACDER1_SSI2EN |
		  MXC_CRMAP_ACDER1_SSI1EN);

	clk_gate_value |= MXC_CRMAP_ACDER1_CSIEN_PASS2;

	__raw_writel(clk_gate_value, MXC_CRMAP_ACDER1);

	/* ACDER2 */

	clk_gate_value = __raw_readl(MXC_CRMAP_ACDER2);

        /* Single bits all enabled - no need to mask */
	clk_gate_value |= 
		(MXC_CRMAP_ACDER2_NFCEN | MXC_CRMAP_ACDER2_USBEN);
		  
	__raw_writel(clk_gate_value, MXC_CRMAP_ACDER2);

#endif /* breaks the phone */
#endif /* CONFIG_MOT_FEAT_PM */
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
