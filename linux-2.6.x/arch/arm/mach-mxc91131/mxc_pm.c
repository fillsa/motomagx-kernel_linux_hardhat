/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2006 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/*
 * Revision History:
 *
 * Date        Author    Comment
 * 10/04/2006  Motorola  Added functions mxc_pm_dither_control,
 *                       mxc_pm_dither_setup, and mxc_pm_dither_report
 *                       to support PLL dithering.
 * 12/25/2006  Motorola  Changed local_irq_disable/local_irq_enable to
 *                       local_irq_save/local_irq_restore.
 */

/*!
 * @file mxc_pm.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for the CRM_AP, DPLL and DSM modules.
 *
 * @ingroup LPMD
 */

/*
 * Include Files
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/mm.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/circ_buf.h>
#include <linux/delay.h>
#include <asm/hardware.h>
#include <asm/arch/mxc_pm.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/mach/time.h>

#include "crm_regs.h"

#define HI_INTR IO_ADDRESS(AVIC_BASE_ADDR + 0x48)
#define LO_INTR IO_ADDRESS(AVIC_BASE_ADDR + 0x4c)

#define HI_INTR_EN IO_ADDRESS(AVIC_BASE_ADDR + 0x10)
#define LO_INTR_EN IO_ADDRESS(AVIC_BASE_ADDR + 0x14)

#define LO_VOLT 0
#define HI_VOLT 1

/*
 * Software Workarounds for Hardware issues
 *
 * SDRAM Controller ratio issue:
 * ARM:AHB ratio must be 1:1, 1:2, or 1:3.
 * Means we can't have arm frequency = 532 MHz because we would
 * need a 1:4 ratio to get AHB=133 Mhz.
 */
#define PM_SDRAMC_RATIO_ISSUE      0

/*
 * ARM_DIV issue
 *
 * This issue not yet verified by hardware team.
 *
 * Processor hangs when setting ARM divider to divide by 1
 * Workaround is to leave it at divide by 2. The implications
 * are that requesting 399 MHz will give us PLL=399MHZ but
 * the ARM:AHB:IP freqs are cut in half:  200:100:49.5 Mhz
 * This only matters when PM_SDRAMC_RATIO_ISSUE (above)
 * is set to 1.  Both these settings default to 0.
 */
#define PM_ARM_DIV_ISSUE           0

#define MAX_PLL_FREQ            ((unsigned long)532000000)

/*!
 * Round a frequency to MHz.
 *
 * @param    freq    The frequency in Hz.
 *
 * @return   freq    The frequency in MHz.
 */
static inline long mhz(long freq)
{
	/*
	 * If freq isn't an integer multiple of 1000000, then don't
	 * round up or down.  Instead, truncate and add 1MHz because
	 * we will always get <= the freq we ask for.  For instance
	 * if we request 399000000 Hz we may get 398998890 Hz but
	 * we won't get 399131313 Hz.
	 */
	if ((freq % 1000000) != 0) {
		freq = (freq / 1000000) + 1;
	} else {
		freq = (freq / 1000000);
	}
	return freq;
}

/*
 * Setting used for ap_delay - unit delay is 1/32768 seconds
 */
#define AP_DELAY_SETTING                        ((unsigned long)(3<<16))

#if 0
/*
 * Bit definitions of ASCSR, ACDR in CRM_AP and CSCR in CRM_COM
 */
#define ACDR_ARM_DIV_MASK                       ((unsigned long)0x00000F00)
#define ACDR_ARM_DIV_OFFSET                     ((unsigned long)8)
#define ACDR_AHB_DIV_MASK                       ((unsigned long)0x000000F0)
#define ACDR_AHB_DIV_OFFSET                     ((unsigned long)4)
#define ACDR_IP_DIV_MASK                        ((unsigned long)0x0000000F)
#define ACDR_IP_DIV_OFFSET                      ((unsigned long)0)

/*
 * Bit definitions of ACGCR in CRM_AP
 */
#define ACG0_STOP_WAIT                          ((unsigned long)0x00000001)
#define ACG0_STOP                               ((unsigned long)0x00000003)
#define ACG0_RUN                                ((unsigned long)0x00000007)
#define ACG0_DISABLED                           ((unsigned long)0x00000000)

#define ACG1_STOP_WAIT                          ((unsigned long)0x00000008)
#define ACG1_STOP                               ((unsigned long)0x00000018)
#define ACG1_RUN                                ((unsigned long)0x00000038)
#define ACG1_DISABLED                           ((unsigned long)0x00000000)

#define ACG2_STOP_WAIT                          ((unsigned long)0x00000040)
#define ACG2_STOP                               ((unsigned long)0x000000C0)
#define ACG2_RUN                                ((unsigned long)0x000001C0)
#define ACG2_DISABLED                           ((unsigned long)0x00000000)

#define ACG3_STOP_WAIT                          ((unsigned long)0x00000200)
#define ACG3_STOP                               ((unsigned long)0x00000600)
#define ACG3_RUN                                ((unsigned long)0x00000E00)
#define ACG3_DISABLED                           ((unsigned long)0x00000000)

#define ACG4_STOP_WAIT                          ((unsigned long)0x00001000)
#define ACG4_STOP                               ((unsigned long)0x00003000)
#define ACG4_RUN                                ((unsigned long)0x00007000)
#define ACG4_DISABLED                           ((unsigned long)0x00000000)

#define ACG5_STOP_WAIT                          ((unsigned long)0x00010000)
#define ACG5_STOP                               ((unsigned long)0x00030000)
#define ACG5_RUN                                ((unsigned long)0x00070000)
#define ACG5_DISABLED                           ((unsigned long)0x00000000)

#define ACG6_STOP_WAIT                          ((unsigned long)0x00080000)
#define ACG6_STOP                               ((unsigned long)0x00180000)
#define ACG6_RUN                                ((unsigned long)0x00380000)
#define ACG6_DISABLED                           ((unsigned long)0x00000000)
#endif
#define NUM_GATE_CTRL                           6

/*
 * Address offsets of DSM registers
 */
#define DSM_MEAS_TIME                           IO_ADDRESS(DSM_BASE_ADDR + 0x08)
#define DSM_SLEEP_TIME                          IO_ADDRESS(DSM_BASE_ADDR + 0x0c)
#define DSM_RESTART_TIME                        IO_ADDRESS(DSM_BASE_ADDR + 0x10)
#define DSM_WAKE_TIME                           IO_ADDRESS(DSM_BASE_ADDR + 0x14)
#define DSM_WARM_TIME                           IO_ADDRESS(DSM_BASE_ADDR + 0x18)
#define DSM_LOCK_TIME                           IO_ADDRESS(DSM_BASE_ADDR + 0x1c)
#define DSM_CONTROL0                            IO_ADDRESS(DSM_BASE_ADDR + 0x20)
#define DSM_CONTROL1                            IO_ADDRESS(DSM_BASE_ADDR + 0x24)
#define DSM_CTREN                               IO_ADDRESS(DSM_BASE_ADDR + 0x28)
#define DSM_INT_STATUS                          IO_ADDRESS(DSM_BASE_ADDR + 0x34)
#define DSM_MASK_REG                            IO_ADDRESS(DSM_BASE_ADDR + 0x38)
#define DSM_CRM_CONTROL                         IO_ADDRESS(DSM_BASE_ADDR + 0x50)

/*
 * Bit definitions of various registers in DSM
 */
#define CRM_CTRL_DVFS_BYP                       ((unsigned long)0x00000008)
#define CRM_CTRL_DVFS_VCTRL                     ((unsigned long)0x00000004)
#define CRM_CTRL_LPMD1                          ((unsigned long)0x00000002)
#define CRM_CTRL_LPMD0                          ((unsigned long)0x00000001)

#define CONTROL0_STBY_COMMIT_EN                 ((unsigned long)0x00000200)
#define CONTROL0_RESTART                        ((unsigned long)0x00000010)
#define CONTROL0_MSTR_EN                        ((unsigned long)0x00000001)

#define CONTROL1_WAKEUP_DISABLE                 ((unsigned long)0x00004000)
#define CONTROL1_RST_CNT32_EN                   ((unsigned long)0x00000800)
#define CONTROL1_SLEEP                          ((unsigned long)0x00000100)

#define INT_STATUS_WKTIME_INT                   ((unsigned long)0x00000200)
#define INT_STATUS_SLEEP_INT                    ((unsigned long)0x00000100)
#define INT_STATUS_RWK                          ((unsigned long)0x00000080)
#define INT_STATUS_NGK_INT                      ((unsigned long)0x00000040)
#define INT_STATUS_PGK_INT                      ((unsigned long)0x00000020)
#define INT_STATUS_STPL1T_INT                   ((unsigned long)0x00000010)
#define INT_STATUS_RSTRT_INT                    ((unsigned long)0x00000008)
#define INT_STATUS_LOCK_INT                     ((unsigned long)0x00000004)
#define INT_STATUS_WTIME_INT                    ((unsigned long)0x00000002)
#define INT_STATUS_MDONE_INT                    ((unsigned long)0x00000001)

#define DSM_CTREN_CNT32                         ((unsigned long)0x00000001)

/*
 * Address offsets of DPLL registers
 */
#define MXC_PLL0_DP_CTL                         IO_ADDRESS(ADPLL_BASE_ADDR + 0x00)
#define MXC_PLL0_DP_CONFIG                      IO_ADDRESS(ADPLL_BASE_ADDR + 0x04)
#define MXC_PLL0_DP_OP                          IO_ADDRESS(ADPLL_BASE_ADDR + 0x08)
#define MXC_PLL0_DP_MFD                         IO_ADDRESS(ADPLL_BASE_ADDR + 0x0c)
#define MXC_PLL0_DP_MFN                         IO_ADDRESS(ADPLL_BASE_ADDR + 0x10)
#define MXC_PLL0_DP_HFSOP                       IO_ADDRESS(ADPLL_BASE_ADDR + 0x1c)
#define MXC_PLL0_DP_HFSMFD                      IO_ADDRESS(ADPLL_BASE_ADDR + 0x20)
#define MXC_PLL0_DP_HFSMFN                      IO_ADDRESS(ADPLL_BASE_ADDR + 0x24)

#define MXC_PLL1_DP_CTL                         IO_ADDRESS(UDPLL_BASE_ADDR + 0x00)
#define MXC_PLL1_DP_OP                          IO_ADDRESS(UDPLL_BASE_ADDR + 0x08)
#define MXC_PLL1_DP_MFD                         IO_ADDRESS(UDPLL_BASE_ADDR + 0x0c)
#define MXC_PLL1_DP_MFN                         IO_ADDRESS(UDPLL_BASE_ADDR + 0x10)
#define MXC_PLL1_DP_HFSOP                       IO_ADDRESS(UDPLL_BASE_ADDR + 0x1c)
#define MXC_PLL1_DP_HFSMFD                      IO_ADDRESS(UDPLL_BASE_ADDR + 0x20)
#define MXC_PLL1_DP_HFSMFN                      IO_ADDRESS(UDPLL_BASE_ADDR + 0x24)

#define MXC_PLL2_DP_CTL                         IO_ADDRESS(BDPLL_BASE_ADDR + 0x00)
#define MXC_PLL2_DP_OP                          IO_ADDRESS(BDPLL_BASE_ADDR + 0x08)
#define MXC_PLL2_DP_MFD                         IO_ADDRESS(BDPLL_BASE_ADDR + 0x0c)
#define MXC_PLL2_DP_MFN                         IO_ADDRESS(BDPLL_BASE_ADDR + 0x10)
#define MXC_PLL2_DP_HFSOP                       IO_ADDRESS(BDPLL_BASE_ADDR + 0x1c)
#define MXC_PLL2_DP_HFSMFD                      IO_ADDRESS(BDPLL_BASE_ADDR + 0x20)
#define MXC_PLL2_DP_HFSMFN                      IO_ADDRESS(BDPLL_BASE_ADDR + 0x24)

/*
 * Bit definitions of DPLL-IP
 */
#define DP_MFI                                  ((unsigned long)0x00000050)
#define DP_PDF                                  ((unsigned long)0x00000001)
#define DP_MFD                                  ((unsigned long)0x00000000)
#define DP_MFN                                  ((unsigned long)0x00000000)
#define DP_HFS_MFI                              ((unsigned long)0x00000050)
#define DP_HFS_PDF                              ((unsigned long)0x00000001)
#define DP_HFS_MFD                              ((unsigned long)0x00000000)
#define DP_HFS_MFN                              ((unsigned long)0x00000000)
#define DP_HFSM                                 ((unsigned long)0x00000080)
#define DP_AREN                                 ((unsigned long)0x00000002)

#define DP_OP_PDF_MASK                          ((unsigned long)0x0000000F)
#define DP_OP_MFI_MASK                          ((unsigned long)0x000000F0)
#define DP_OP_MFI_SHIFT                         ((unsigned long)4)
#define DP_MFD_MASK                             ((unsigned long)((1<<27)-1))
#define DP_MFN_MASK                             ((unsigned long)((1<<27)-1))

#define DP_CTL_REF_CLK_DIV_MASK                 ((unsigned long)0x00000300)
#define DP_CTL_REF_CLK_DIV_SHIFT                ((unsigned long)8)
#define DP_CTL_LRF                              ((unsigned long)0x00000001)

/*
 * Bit definitions of DP_CTL register
 */
#define DP_CTL_UPEN                             ((unsigned long)0x00000020)
#define DP_CTL_RST                              ((unsigned long)0x00000010)
#define DP_CTL_BRMO                             ((unsigned long)0x00000002)

/*
 * Reference clock inputs to PLLs
 */
#define CKIH_CLK_FREQ               16800000
#define DIGRF_CLK_FREQ              26000000

#undef INTEGER_SCALING
#undef PLL_RELOCK
typedef enum {
	INTEGER_SCALING = 0,
	PLL_RELOCK
} mxc_dvfs_scaling_method;

#define MXC_TESTTIME ((int)0)

#ifdef CONFIG_MXC_LL_PM_DEBUG
#define PM_DEBUG(fmt, args...) printk(fmt,## args)

static inline void mxc_pm_debug_print_status(void)
{
	unsigned int adcr, dp_ctl;

	adcr = __raw_readl(MXC_CRM_AP_ADCR);
	dp_ctl = __raw_readl(MXC_PLL0_DP_CTL);
//        dp_ctl = (__raw_readl(pll_dp_ctlreg[0]));

	printk("ADCR = 0x%08x ; DP_CTL = 0x%08x ", (int)adcr, (int)dp_ctl);

	if (adcr & ADCR_VSTAT)
		printk("VSTAT=1 (lo)  ");
	else
		printk("VSTAT=0 (hi)  ");

	if (dp_ctl & DP_HFSM)
		printk("HFSM=1 (hi)\n");
	else
		printk("HFSM=0 (lo)\n");

	PM_DEBUG
	    ("Dividers: ACDR=0x%03x = %d:%d:%d LFDF=%d DIV_BYP=%d DFS_DIV_EN=%d\n",
	     (int)__raw_readl(MXC_CRM_AP_ACDR),
	     (int)mxc_get_clocks_div(CPU_CLK),
	     (int)(mxc_get_clocks_div(AHB_CLK) * mxc_get_clocks_div(CPU_CLK)),
	     (int)(mxc_get_clocks_div(IPG_CLK) * mxc_get_clocks_div(CPU_CLK)),
	     (int)(1 << ((adcr >> 8) & 0x3)), (int)((adcr >> 1) & 0x1),
	     (int)((adcr >> 5) & 0x1));

	PM_DEBUG("ARM %dMHz (AHB %dMHz IP %dHz) PLLfreq = %dMHz\n\n",
		 (int)mhz(mxc_get_clocks(CPU_CLK)),
		 (int)mhz(mxc_get_clocks(AHB_CLK)),
		 (int)mxc_get_clocks(IPG_CLK), (int)mhz(mxc_pll_clock(MCUPLL)));
}

static inline void pm_debug_longdelay(void)
{
	int i;
	if (MXC_TESTTIME > 0) {
		for (i = 0; i < (MXC_TESTTIME * 1000); i++) {
			udelay(1);
		}
	}
}

static inline void pm_debug_printk_dividers(void)
{
	PM_DEBUG("Dividers set to: %d:%d:%d  ACDR = 0x%03x\n",
		 (int)mxc_get_clocks_div(CPU_CLK),
		 (int)(mxc_get_clocks_div(AHB_CLK) *
		       mxc_get_clocks_div(CPU_CLK)),
		 (int)(mxc_get_clocks_div(IPG_CLK) *
		       mxc_get_clocks_div(CPU_CLK)),
		 (int)__raw_readl(MXC_CRM_AP_ACDR));
}

#define PM_DEBUG_STABILITY(fmt, args...) \
       pm_debug_longdelay(); \
       printk(fmt,## args);

#define PM_DEBUG_STATUS(fmt, args...) \
       printk(fmt,## args); \
       mxc_pm_debug_print_status();

#else
#define PM_DEBUG(fmt, args...)

static inline void pm_debug_longdelay(void)
{
}

static inline void mxc_pm_debug_print_status(void)
{
}

static inline void pm_debug_printk_dividers(void)
{
}

#define PM_DEBUG_STABILITY(fmt, args...)
#define PM_DEBUG_STATUS(fmt, args...)

#endif				/* MXC_LL_PM_DEBUG */

/*
 * Configuration Options
 */
/* Only one of the following options can be chosen */
#undef CONFIG_PLL_NOCLOCK	/* This option is broken in HW */
#define CONFIG_PLL_PATREF 1
#undef CONFIG_ALT_PLL		/* This option is currently not implemented */
/*
 * The following options used for DSM
 */
#define COUNT32_RESET           1
#define COUNT32_NOT_RESET       0
#define CONFIG_RESET_COUNT32    COUNT32_RESET

#define NUM_PLL 3

static unsigned int pll_dp_opreg[NUM_PLL] =
    { MXC_PLL0_DP_OP, MXC_PLL1_DP_OP, MXC_PLL2_DP_OP };

static unsigned int pll_dp_ctlreg[NUM_PLL] =
    { MXC_PLL0_DP_CTL, MXC_PLL1_DP_CTL, MXC_PLL2_DP_CTL };

static unsigned int pll_dp_mfnreg[NUM_PLL] =
    { MXC_PLL0_DP_MFN, MXC_PLL1_DP_MFN, MXC_PLL2_DP_MFN };

static unsigned int pll_dp_mfdreg[NUM_PLL] =
    { MXC_PLL0_DP_MFD, MXC_PLL1_DP_MFD, MXC_PLL2_DP_MFD };

static unsigned int pll_dp_hfs_opreg[NUM_PLL] =
    { MXC_PLL0_DP_HFSOP, MXC_PLL1_DP_HFSOP, MXC_PLL2_DP_HFSOP };

static unsigned int pll_dp_hfs_mfnreg[NUM_PLL] =
    { MXC_PLL0_DP_HFSMFN, MXC_PLL1_DP_HFSMFN, MXC_PLL2_DP_HFSMFN };

static unsigned int pll_dp_hfs_mfdreg[NUM_PLL] =
    { MXC_PLL0_DP_HFSMFD, MXC_PLL1_DP_HFSMFD, MXC_PLL2_DP_HFSMFD };

static unsigned int gate_stop_wait[NUM_GATE_CTRL] =
    { ACG0_STOP_WAIT, ACG1_STOP_WAIT, ACG3_STOP_WAIT,
	ACG4_STOP_WAIT, ACG5_STOP_WAIT, ACG6_STOP_WAIT
};

static unsigned int gate_stop[NUM_GATE_CTRL] =
    { ACG0_STOP, ACG1_STOP, ACG3_STOP, ACG4_STOP, ACG5_STOP, ACG6_STOP };

static unsigned int gate_run[NUM_GATE_CTRL] =
    { ACG0_RUN, ACG1_RUN, ACG3_RUN, ACG4_RUN, ACG5_RUN, ACG6_RUN };

/*!
 * Set ARM divisor in ACDR.  Don't change AHB and IP divisors.
 *
 * @param   armdiv      arm divisor value (divide-by-X).
 */
static inline void set_arm_div(int armdiv)
{
	unsigned int acdr_val;

	PM_DEBUG("set_arm_div arg: %d\n", (int)armdiv);

	mxc_set_clocks_div(CPU_CLK, armdiv);
	acdr_val = __raw_readl(MXC_CRMAP_ACDR);
	acdr_val = __raw_readl(MXC_CRMAP_ACDR);

	pm_debug_printk_dividers();
	PM_DEBUG_STABILITY("%d msec after setting arm divider\n", MXC_TESTTIME);
}

/*!
 * Get the refclk frequency for the PLL
 *
 * @param   pll_num     0 for AP PLL, 1 for BP PLL
 *
 * @return              The refclk rate in Hz
 */
static long mxc_pm_get_pll_refclk(int pll_num)
{
	int ref_clk_sel;
	long ref_clk, dp_ctl;

	dp_ctl = __raw_readl(pll_dp_ctlreg[pll_num]);

	ref_clk_sel =
	    (dp_ctl & DP_CTL_REF_CLK_DIV_MASK) >> DP_CTL_REF_CLK_DIV_SHIFT;

	switch (ref_clk_sel) {

	case DP_CTL_REF_CLK_SEL_CKIH:
		ref_clk = CKIH_CLK_FREQ;
		break;

	case DP_CTL_REF_CLK_SEL_CKIH_X2:
		ref_clk = (CKIH_CLK_FREQ * 2);
		break;

	case DP_CTL_REF_CLK_SEL_DIGRF:
		ref_clk = DIGRF_CLK_FREQ;
		break;

	case DP_CTL_REF_CLK_SEL_DIGRF_X2:
		ref_clk = (DIGRF_CLK_FREQ * 2);
		break;

	default:
		ref_clk = 0;
	}

	/*
	 * PM_DEBUG("%s: ref_clk_sel=%d  PLL refclk = %d Hz\n",
	 *        __FUNCTION__, (int)ref_clk_sel, (int)ref_clk );
	 */

	return ref_clk;
}

/*!
 * Get the PLL Frequency.  This function lets the caller select whether
 * to get the HFS or LFS mode PLL setting independent of which mode we
 * currently are in.  This is important because on MXC91131, Integer scaling
 * always uses the PLL in HFSM mode, even at low voltage.
 *
 * @param   hfsm        = 0 = get the PLL setting for the LFS mode
 *                      = 1 = get the PLL setting for the HFS mode
 *
 * @return              PLL rate in Hz
 */
static long mxc_pm_get_pll_freq(int hfsm)
{
	int pll_num;
	signed long mfi, mfn, mfd, pdf, ref_clk, pll_out;
	unsigned long dp_ctl, dp_op, dp_mfd, dp_mfn;

	/* Is ARM clock from AP PLL or BP PLL? */
	pll_num = ((__raw_readl(MXC_CRMAP_ASCSR) & MXC_CRMAP_ASCSR_APSEL) != 0);

	dp_ctl = __raw_readl(pll_dp_ctlreg[pll_num]);

	if (hfsm) {
		dp_op = __raw_readl(pll_dp_hfs_opreg[pll_num]);
		dp_mfd = __raw_readl(pll_dp_hfs_mfdreg[pll_num]);
		dp_mfn = __raw_readl(pll_dp_hfs_mfnreg[pll_num]);
	} else {
		dp_op = __raw_readl(pll_dp_opreg[pll_num]);
		dp_mfd = __raw_readl(pll_dp_mfdreg[pll_num]);
		dp_mfn = __raw_readl(pll_dp_mfnreg[pll_num]);
	}

	pdf = (dp_op & DP_OP_PDF_MASK);
	mfi = (dp_op & DP_OP_MFI_MASK) >> DP_OP_MFI_SHIFT;
	mfi = (mfi <= 5) ? 5 : mfi;
	mfd = dp_mfd & DP_MFD_MASK;
	mfn = dp_mfn & DP_MFN_MASK;
	mfn = (mfn <= 0x4000000) ? mfn : (mfn - 0x10000000);

	ref_clk = mxc_pm_get_pll_refclk(pll_num);

	pll_out = (2 * ref_clk * mfi + ((2 * ref_clk / (mfd + 1)) * mfn)) /
	    (pdf + 1);

	PM_DEBUG("%s: PLL HFSM = %d Hz\n", __FUNCTION__, (int)pll_out);
	/*
	 * PM_DEBUG("%s: dp_op=0x%02x  dp_mfd=0x%08x  dp_mfn=0x%08x PLL HFSM = %d Hz\n",
	 *       __FUNCTION__, (int)dp_op, (int)dp_mfd, (int)dp_mfn, (int)pll_out );
	 */

	return pll_out;
}

/*!
 * Find out if ADCR is set up for Integer scaling or PLL scaling
 *
 * @return              Scaling method = INTEGER_SCALING = 0
 *                                     = PLL_RELOCK = 1
 */
static int mxc_pm_get_scaling_method(void)
{
	unsigned long adcr_val;
	int scaling_method;

	adcr_val = __raw_readl(MXC_CRMAP_ADCR);
	if ((adcr_val & MXC_CRMAP_ADCR_DFS_DIV_EN) != 0) {
		scaling_method = INTEGER_SCALING;
	} else {
		scaling_method = PLL_RELOCK;
	}

	return scaling_method;
}

/*!
 * Set the bits in ADCR for Integer scaling or PLL scaling.
 *
 * @param    scaling_method  If voltage is HI_VOLT, go to high voltage,
 *                           else go to low voltage.
 *
 * @return   None
 */
static void mxc_pm_set_scaling_method(mxc_dvfs_scaling_method scaling_method)
{
	unsigned long adcr_val;

	adcr_val = __raw_readl(MXC_CRMAP_ADCR);

	/*
	 * Set DIV_EN bit to select integer scaling, and enable the DFS
	 * divider.  Disable alternate PLL method.
	 */
	if ((adcr_val & MXC_CRMAP_ADCR_VSTAT) != 0) {
		PM_DEBUG("Error: Changing DIV_EN while Low voltage\n");
	}

	if (scaling_method == INTEGER_SCALING) {
		PM_DEBUG("Setting scaling method to integer scaling\n");
		/*
		 * Clear the div_byp bit to enable the DFS divider.  Alternate PLL
		 * method disabled.  Set DIV_EN bit to select integer scaling.
		 */
		adcr_val = (adcr_val | MXC_CRMAP_ADCR_DFS_DIV_EN) &
		    ~MXC_CRMAP_ADCR_ALT_PLL & ~MXC_CRMAP_ADCR_DIV_BYP;
		__raw_writel(adcr_val, MXC_CRMAP_ADCR);
	} else {
		PM_DEBUG("Setting scaling method to PLL scaling\n");
#if (CONFIG_PLL_PATREF == 1)
		/*
		 * PLL Relock with PAT_REF supplying clock while PLL is relocking:
		 * Set the div_byp bit to disable the DFS divider.  Alternate
		 * PLL method disabled.  Clear DIV_EN bit to select PLL scaling.
		 */
		adcr_val =
		    (adcr_val | MXC_CRMAP_ADCR_DIV_BYP | MXC_CRMAP_ADCR_CLK_ON)
		    & ~MXC_CRMAP_ADCR_ALT_PLL & ~MXC_CRMAP_ADCR_DFS_DIV_EN;
		__raw_writel(adcr_val, MXC_CRMAP_ADCR);
#elif (CONFIG_PLL_NOCLOCK == 1)
		/*
		 * PLL Relock with no clock while PLL is relocking:
		 * Set the div_byp bit to disable the DFS divider.  Alternate
		 * PLL method disabled.  Clear DIV_EN bit to select PLL scaling.
		 */
		adcr_val = (adcr_val | MXC_CRMAP_ADCR_DIV_BYP) &
		    ~MXC_CRMAP_ADCR_ALT_PLL & ~MXC_CRMAP_ADCR_DFS_DIV_EN &
		    ~MXC_CRMAP_ADCR_CLK_ON;
		__raw_writel(adcr_val, MXC_CRMAP_ADCR);
#endif
	}
	adcr_val = __raw_readl(MXC_CRMAP_ADCR);
}

/*!
 * Request voltage change.  If going to high voltage, wait of voltage to settle.
 *
 * @param    requested_voltage    If voltage is HI_VOLT, go to high voltage,
 *                                else go to low voltage.
 *
 * @return   None
 */
static void mxc_pm_change_voltage(int requested_voltage)
{
	/* Wait on voltage settling timer to clear (should already be cleared */
	do {
		udelay(1);
	} while ((__raw_readl(MXC_CRMAP_ADCR) & MXC_CRMAP_ADCR_TSTAT) != 0);

	if (requested_voltage == HI_VOLT) {
		PM_DEBUG("mxc_pm_change_voltage - going to Hi voltage\n");

		/* Clear VCTRL to signal that we want AP to go to high voltage. */
		__raw_writel(__raw_readl(DSM_CRM_CONTROL) &
			     ~CRM_CTRL_DVFS_VCTRL, DSM_CRM_CONTROL);

		/* Wait on voltage settling timer to clear */
		do {
			udelay(1);
		} while ((__raw_readl(MXC_CRMAP_ADCR) & MXC_CRMAP_ADCR_TSTAT) !=
			 0);

	} else {
		PM_DEBUG("mxc_pm_change_voltage - going to Lo voltage\n");

		/* Set VCTRL to signal that we want AP to go to low voltage. */
		__raw_writel(__raw_readl(DSM_CRM_CONTROL) | CRM_CTRL_DVFS_VCTRL,
			     DSM_CRM_CONTROL);
	}

	PM_DEBUG_STATUS("%s - after voltage change:\n", __FUNCTION__);
	PM_DEBUG_STABILITY("%d msec after changing voltage\n", MXC_TESTTIME);
}

/*!
 * Calculating possible divider value for AHB
 * This is called AFTER the PLLs change.
 *
 * @param   armfreq    The desired ARM frequency in Hz
 * @param   ahbfreq    The desired AHB frequency in Hz
 * @param   ipfreq     The desired IP frequency in Hz
 */
static void mxc_pm_set_clock_dividers(long armfreq, long ahbfreq, long ipfreq)
{
	unsigned long acdr, current_acdr, armdiv, ahbdiv, ipdiv, pllfreq;
	int pll_num;

	pll_num = ((__raw_readl(MXC_CRMAP_ASCSR) & MXC_CRMAP_ASCSR_APSEL) != 0);

	/* Get current AP PLL clock */
	if ((__raw_readl(MXC_CRMAP_ASCSR) & MXC_CRMAP_ASCSR_APSEL) != 0) {
		pllfreq = mxc_pll_clock(DSPPLL);
	} else {
		pllfreq = mxc_pll_clock(MCUPLL);
	}

	PM_DEBUG("%s called: %d %d %d pll=%d\n", __FUNCTION__,
		 (int)armfreq, (int)ahbfreq, (int)ipfreq, (int)pllfreq);

	armdiv = mhz(pllfreq) / mhz(armfreq);
	if (armdiv == 0) {
		armdiv = 1;
	}
	if (mhz(pllfreq / armdiv) > mhz(armfreq)) {
		armdiv++;
	}

	ahbdiv = armfreq / ahbfreq;
	if (ahbdiv == 0) {
		ahbdiv = 1;
	}
	if (mhz(armfreq / ahbdiv) > mhz(ahbfreq)) {
		ahbdiv++;
	}

	/*
	 * The SDRAM Controller Ratio issue limits ARM:AHB divisor ratios to
	 * be either 1:1, 1:2, or 1:3.  So we can't have armfreq=532MHz
	 * (would be 1:4) so we do pllfreq=532, armfreq=266 at high voltage
	 * as the closest we can get for developmental purposes.
	 */
	if ((PM_SDRAMC_RATIO_ISSUE) && (ahbdiv > 3)) {
		/* PM_DEBUG("%s ahbdiv is %d (>3) so adjusting\n",
		   __FUNCTION__, (int)ahbdiv ); */

		armfreq /= 2;

		armdiv = mhz(pllfreq) / mhz(armfreq);
		if (armdiv == 0) {
			armdiv = 1;
		}
		if (mhz(pllfreq / armdiv) > mhz(armfreq)) {
			armdiv++;
		}

		/* PM_DEBUG("new armdiv = %d\n", (int)armdiv ); */

		ahbdiv = armfreq / ahbfreq;
		if (ahbdiv == 0) {
			ahbdiv = 1;
		}
		if (mhz(armfreq / ahbdiv) > mhz(ahbfreq)) {
			ahbdiv++;
		}
	}

	ipdiv = armfreq / ipfreq;
	if (ipdiv == 0) {
		ipdiv = 1;
	}
	if (mhz(armfreq / ipdiv) > mhz(ipfreq)) {
		ipdiv++;
	}

	/* If ahb == 133 MHz, make sure ip == 66.5 Mhz. */
	if (ahbfreq == AHB_FREQ) {
		ipdiv = 2 * ahbdiv;
	}

	/*
	 * Translate armdiv factor for ARM_DIV bit field
	 */
	if (armdiv == 1) {
		armdiv = 8;
	} else if (armdiv <= 6) {
		armdiv -= 2;
	} else {
		armdiv = (armdiv / 2) + 1;
	}

	PM_DEBUG("Divider fields will be %x %x %x\n",
		 (int)armdiv, (int)ahbdiv, (int)ipdiv);

	/* Write out arm, ahb, ip dividers to ACDR */
	acdr = (armdiv << MXC_CRMAP_ACDR_ARM_DIV_OFFSET) |
	    (ahbdiv << MXC_CRMAP_ACDR_AHB_DIV_OFFSET) | ipdiv;

	current_acdr = __raw_readl(MXC_CRMAP_ACDR);
	PM_DEBUG("ACDR is 0x%03x; Want ACDR=0x%03x\n", (int)current_acdr,
		 (int)acdr);

	/* First write out AHB, IP div if they have changed */
	if ((acdr & (MXC_CRMAP_ACDR_AHB_DIV_MASK | MXC_CRMAP_ACDR_IP_DIV_MASK))
	    !=
	    (current_acdr &
	     (MXC_CRMAP_ACDR_AHB_DIV_MASK | MXC_CRMAP_ACDR_IP_DIV_MASK))) {
		current_acdr =
		    (current_acdr & MXC_CRMAP_ACDR_ARM_DIV_MASK) | (acdr &
								    (MXC_CRMAP_ACDR_AHB_DIV_MASK
								     |
								     MXC_CRMAP_ACDR_IP_DIV_MASK));
		PM_DEBUG("Writing ACDR=0x%08x\n", (int)current_acdr);
		__raw_writel(current_acdr, MXC_CRMAP_ACDR);
		current_acdr = __raw_readl(MXC_CRMAP_ACDR);
		current_acdr = __raw_readl(MXC_CRMAP_ACDR);
	}

	/*
	 * Workaround for ARM_DIV issue is to leave the ARM_DIV at
	 * divide by 2 after scaling to 399 MHz.
	 */
	/*
	 * Next write out ARM_DIV if it has changed
	 */
	if ((acdr != current_acdr) &&
	    (((PM_ARM_DIV_ISSUE) == 0) || (mhz(armfreq) != 399))) {
		PM_DEBUG("Writing ACDR=0x%08x\n", (int)acdr);
		__raw_writel(acdr, MXC_CRMAP_ACDR);
		acdr = __raw_readl(MXC_CRMAP_ACDR);
		acdr = __raw_readl(MXC_CRMAP_ACDR);
	}

	PM_DEBUG("Dividers set to: %d:%d:%d\n",
		 (int)mxc_get_clocks_div(CPU_CLK),
		 (int)(mxc_get_clocks_div(AHB_CLK) *
		       mxc_get_clocks_div(CPU_CLK)),
		 (int)(mxc_get_clocks_div(IPG_CLK) *
		       mxc_get_clocks_div(CPU_CLK)));

	PM_DEBUG_STABILITY("%d msec after setting dividers.  ACDR=0x%03x\n",
			   MXC_TESTTIME, (int)__raw_readl(MXC_CRMAP_ACDR));
}

/*!
 * Calculating possible LFDF value for Integer Scaling
 *
 * @param   value       The divider value to be set for the DFS block.
 *                      Possible values are 0,2,4,8.
 *                      This occurs during Integer scaling
 *
 * @return  None
 */
static void mxc_pm_set_lfdf(int value)
{
	unsigned long adcr_val;

	PM_DEBUG("%s - param is %d\n", __FUNCTION__, (int)value);

	if (((__raw_readl(MXC_CRMAP_ADCR)) & MXC_CRMAP_ADCR_VSTAT) != 0) {
		PM_DEBUG("%s - Error: Setting LFDF while Low Voltage\n",
			 __FUNCTION__);
	}

	adcr_val = __raw_readl(MXC_CRMAP_ADCR) &
	    ~(MXC_CRMAP_ADCR_LFDF_LSB | MXC_CRMAP_ADCR_LFDF_MSB);
	switch (value) {
	case 1:
		break;
	case 2:
		adcr_val |= MXC_CRMAP_ADCR_LFDF_LSB;
		break;
	case 4:
		adcr_val |= MXC_CRMAP_ADCR_LFDF_MSB;
		break;
	case 8:
		adcr_val |= (MXC_CRMAP_ADCR_LFDF_LSB | MXC_CRMAP_ADCR_LFDF_MSB);
		break;
	default:
		PM_DEBUG("invalid LFDF setting\n");
		adcr_val |= MXC_CRMAP_ADCR_LFDF_LSB;
		break;
	}
	__raw_writel(adcr_val, MXC_CRMAP_ADCR);
	adcr_val = __raw_readl(MXC_CRMAP_ADCR);
}

/*!
 * Calculate and write MFI, MFN, MFD values. Using this the output frequency
 * whose value is calculated using,
 * 2 * REF_FREQ * (MF / PDF), where
 * REF_FREQ is 26 Mhz
 * MF = MFI + (MFN + MFD)
 * PDF is assumed to be 1
 *
 * @param   armfreq     The desired ARM frequency in Hz
 * @param   pll_num     PLL that has to be changed
 * @param   reg_type    Register type to indicate low-frequency
 *                      or high frequency setting.
 *                      0 - Low Frequency
 *                      1 - High Frequency (Shadow registers)
 *
 * @return              Returns 0 on success or
 *                      Returns -1 on error
 */
static int mxc_pm_program_pll(long armfreq, int pll_num, int reg_type)
{
	signed long dp_opval, mfi, mfn, mfd, ref_clk, freq_out;
	signed long mfi_save = 0, mfn_save = 0, freq_out_save = 0;
	int pdf;

	PM_DEBUG("%s armfreq=%d  pll_num=%d  reg_type=%d\n",
		 __FUNCTION__, (int)armfreq, pll_num, reg_type);
	PM_DEBUG("PLL settings (LFS) are: op:0x%08x mfn:0x%08x mfd0x%08x\n",
		 (int)__raw_readl(pll_dp_opreg[pll_num]),
		 (int)__raw_readl(pll_dp_mfnreg[pll_num]),
		 (int)__raw_readl(pll_dp_mfdreg[pll_num]));
	PM_DEBUG("PLL settings (HFS) are: op:0x%08x mfn:0x%08x mfd:0x%08x\n",
		 (int)__raw_readl(pll_dp_hfs_opreg[pll_num]),
		 (int)__raw_readl(pll_dp_hfs_mfnreg[pll_num]),
		 (int)__raw_readl(pll_dp_hfs_mfdreg[pll_num]));

	ref_clk = mxc_pm_get_pll_refclk(pll_num);
	pdf = 0;

	/*
	 * if (armfreq < (REF_FREQ*10) ) {
	 *       pdf = 1;
	 * }
	 */

	mfd = (REF_FREQ / 20000) - 1;
	mfi = 5;
	freq_out_save = 0;

	/* We have pdf and mfd fixed.  Now loop to determine mfi and mfn. */
	while ((mfi <= 15) && (freq_out_save != armfreq)) {

		mfn =
		    (((armfreq / 100000) * (pdf + 1) * (mfd + 1)) -
		     (mfi * (mfd + 1) * (ref_clk / 50000)))
		    / (ref_clk / 50000);

		/* mfn must be less than mfd */
		if (mfn >= mfd) {
			mfi++;
			continue;
		}

		mfn = (mfn <= 0x4000000) ? mfn : (mfn - 0x10000000);

		freq_out =
		    ((2 * ref_clk * mfi) +
		     ((2 * ref_clk / (mfd + 1)) * mfn)) / (pdf + 1);

		/*
		 * PM_DEBUG( "mfi = %2d  mfn = %5d  mfd = %d  pdf = %d  freq = %d\n",
		 *       (int)mfi, (int)mfn, (int)mfd, (int)pdf, (int)freq_out );
		 */

		if ((freq_out <= armfreq) && (freq_out > freq_out_save)) {
			/* PM_DEBUG("previous was saved!\n"); */
			mfi_save = mfi;
			mfn_save = mfn;
			freq_out_save = freq_out;
		}
		mfi++;
	}

	if ((freq_out_save == 0) || (freq_out_save < (armfreq - 2000000))) {
		PM_DEBUG("%s: didn't find valid settings\n", __FUNCTION__);
		return -1;
	}

	mfi = mfi_save;
	mfn = mfn_save;
	freq_out = freq_out_save;
	dp_opval = (mfi << 4) | pdf;

	/*
	 * PM_DEBUG("PLL settings will be: op:0x%08x mfn:0x%08x mfd:0x%08x\n",
	 *        (int)dp_opval, (int)mfn, (int)mfd);
	 */

	if (reg_type == 0) {
		__raw_writel(dp_opval, pll_dp_opreg[pll_num]);
		__raw_writel(mfn, pll_dp_mfnreg[pll_num]);
		__raw_writel(mfd, pll_dp_mfdreg[pll_num]);
	} else {
		__raw_writel(dp_opval, pll_dp_hfs_opreg[pll_num]);
		__raw_writel(mfn, pll_dp_hfs_mfnreg[pll_num]);
		__raw_writel(mfd, pll_dp_hfs_mfdreg[pll_num]);
	}

	/*
	 * Wait for the PLL to relock
	 */
	do {
		udelay(1);
	} while ((__raw_readl(pll_dp_ctlreg[pll_num]) & DP_CTL_LRF) == 0);

	PM_DEBUG("%s: after PLL relock:\n", __FUNCTION__);
	if (reg_type == 0) {
		PM_DEBUG
		    ("PLL settings (LFS) are: op:0x%08x mfn:0x%08x mfd:0x%08x\n",
		     (int)__raw_readl(pll_dp_opreg[pll_num]),
		     (int)__raw_readl(pll_dp_mfnreg[pll_num]),
		     (int)__raw_readl(pll_dp_mfdreg[pll_num]));
	} else {
		PM_DEBUG
		    ("PLL settings (HFS) are: op:0x%08x mfn:0x%08x mfd:0x%08x\n",
		     (int)__raw_readl(pll_dp_hfs_opreg[pll_num]),
		     (int)__raw_readl(pll_dp_hfs_mfnreg[pll_num]),
		     (int)__raw_readl(pll_dp_hfs_mfdreg[pll_num]));
	}

	PM_DEBUG_STABILITY("%d msec after programming pll\n", MXC_TESTTIME);

	return 0;
}

/*!
 * Handles all cases of going from low voltage to high voltage.
 *
 * @param   armfreq     The desired ARM frequency in Hz.
 * @param   ahbfreq     The desired AHB frequency in Hz
 * @param   ipfreq      The desired IP frequency in Hz
 *
 * @return              Returns 0 on success or
 *                      Returns -PLL_LESS_ARM_ERR if PLL frequency is
 *                      less than the desired ARM frequency
 *                      Returns -INTEGER_DVFS_NOT_ALLOW if the desired
 *                      core frequency is greater than 133MHz
 *                      Returns -INT_DFS_LOW_NOT_ALLOW if the desired
 *                      core frequency transition is in low-voltage
 */
static int mxc_pm_pll_scale_to_high_voltage(long armfreq, long ahbfreq,
					    long ipfreq)
{
	unsigned long hfs_pllfreq;
	int pll_num, ret_val;

	PM_DEBUG("%s\n", __FUNCTION__);

	pll_num = ((__raw_readl(MXC_CRMAP_ASCSR) & MXC_CRMAP_ASCSR_APSEL) != 0);

	/*
	 * The reason this function exists:
	 *
	 * It is forbidden to change DFS_DIV_EN at low voltage.
	 * Therefore if you are at low voltage, and you got there by
	 * integer scaling, you have to integer scale to get to hi voltage.
	 * Same for PLL scaling.
	 */

	/*
	 * Change to hi voltage, doing PLL scaling
	 * We were at the LFS PLL setting, now relock to
	 * the HFS PLL setting, which we've already determined
	 * will be a multiple of the armfreq
	 */
	PM_DEBUG("PLL scale to high voltage first\n");

	/*
	 * The SDRAM Controller Ratio issue limits ARM:AHB divisor ratios to
	 * be either 1:1, 1:2, or 1:3.  So we can't have armfreq=532MHz
	 * (would be 1:4) so we do pllfreq=532, armfreq=266 at high voltage
	 * as the closest we can get for developmental purposes.
	 */
	if ((PM_SDRAMC_RATIO_ISSUE) && (mxc_get_clocks_div(CPU_CLK) == 1)) {
		set_arm_div(2);
	}

	/*
	 * Get HFSM PLL clock rate.  For Integer scaling on MXC91131, hardware
	 * always uses the HFSM settings.
	 */
	hfs_pllfreq = mhz(mxc_pm_get_pll_freq(1));

	/*
	 * Is the HFS PLL setting an integer multiple of the desired armfreq?
	 */
	if ((mhz(armfreq) != hfs_pllfreq) &&
	    (mhz(armfreq) * 2 != hfs_pllfreq) &&
	    (mhz(armfreq) * 4 != hfs_pllfreq) &&
	    (mhz(armfreq) * 8 != hfs_pllfreq)) {

		/*
		 * Find a PLL frequency that is an integer multiple of the
		 * desired armfreq
		 */
		hfs_pllfreq = armfreq * 8;
		while ((hfs_pllfreq > MAX_PLL_FREQ) && (hfs_pllfreq > armfreq)) {
			hfs_pllfreq /= 2;
		};
		PM_DEBUG("OK lets do hfs pll = %d\n", (int)hfs_pllfreq);

		/*
		 * If we found a good freq, set HFS PLL setting to it.
		 */
		if ((hfs_pllfreq < (REF_FREQ * 10)) ||
		    (hfs_pllfreq > MAX_PLL_FREQ) || (hfs_pllfreq < armfreq)) {
			PM_DEBUG
			    ("Int scale: failed to PLL scale to good freq\n");
			return -1;
		}

		ret_val = mxc_pm_program_pll(hfs_pllfreq, pll_num, 1);
		if (ret_val != 0) {
			return -PLL_DVFS_FREQ_TOO_LOW;
		}
	}

	/* Go to hi voltage.  PLL will relock to HFS setting */
	mxc_pm_change_voltage(HI_VOLT);

	return 0;
}

/*!
 * Check the parameters, return errors or success.
 *
 * @param   armfreq     The desired ARM frequency in Hz.
 * @param   ahbfreq     The desired AHB frequency in Hz
 * @param   ipfreq      The desired IP frequency in Hz
 *
 * @return              Returns 0 on success or
 *                      Returns -PLL_LESS_ARM_ERR if PLL frequency is
 *                      less than the desired ARM frequency
 *                      Returns -INTEGER_DVFS_NOT_ALLOW if the desired
 *                      core frequency is greater than 133MHz
 *                      Returns -INT_DFS_LOW_NOT_ALLOW if the desired
 *                      core frequency transition is in low-voltage
 */
int mxc_pm_check_parameters(long armfreq, long ahbfreq, long ipfreq)
{
	long arm_to_ahb, ahb_to_ip;

	if ((armfreq < 1000000) || (ahbfreq < 1000000) || (ipfreq < 1000000)) {
		PM_DEBUG("%s invalid ahb or ip\n", __FUNCTION__);
		return -MXC_PM_INVALID_PARAM;
	}

	if ((ahbfreq > AHB_FREQ) || (ipfreq > IPG_FREQ)) {
		PM_DEBUG("%s invalid ahb or ip\n", __FUNCTION__);
		return -AHB_IPG_EXCEED_LIMIT;
	}

	if (((armfreq % ahbfreq) != 0) || ((ahbfreq % ipfreq) != 0)) {
		PM_DEBUG("%s invalid ahb or ip\n", __FUNCTION__);
		return -MXC_PM_INVALID_PARAM;
	}

	arm_to_ahb = armfreq / ahbfreq;
	ahb_to_ip = ahbfreq / ipfreq;

	if ((arm_to_ahb > 16) || (ahb_to_ip > 16)) {
		PM_DEBUG("%s invalid ahb or ip\n", __FUNCTION__);
		return -MXC_PM_INVALID_PARAM;
	}

	/* Everything OK */
	return 0;
}

/*!
 * Integer Scaling
 *
 * @param   armfreq     The desired ARM frequency in Hz.
 * @param   ahbfreq     The desired AHB frequency in Hz
 * @param   ipfreq      The desired IP frequency in Hz
 *
 * @return              Returns 0 on success or
 *                      Returns -PLL_LESS_ARM_ERR if PLL frequency is
 *                      less than the desired ARM frequency
 *                      Returns -INTEGER_DVFS_NOT_ALLOW if the desired
 *                      core frequency is greater than 133MHz
 *                      Returns -INT_DFS_LOW_NOT_ALLOW if the desired
 *                      core frequency transition is in low-voltage
 */
int mxc_pm_intscale(long armfreq, long ahbfreq, long ipfreq)
{
	unsigned long hfs_pllfreq, lfdf_ratio;
	int pll_num, ret_val;

	PM_DEBUG("%s\n", __FUNCTION__);

	ret_val = mxc_pm_check_parameters(armfreq, ahbfreq, ipfreq);
	if (ret_val < 0) {
		return ret_val;
	}

	/* Find out what PLL the AP is clocked by */
	pll_num = ((__raw_readl(MXC_CRMAP_ASCSR) & MXC_CRMAP_ASCSR_APSEL) != 0);

	/*
	 * If we're at the low voltage:
	 * Either simply do integer scaling to hi V or
	 * Program PLL for a useful freq, set armdiv=2, and pll scale
	 * to hi V.
	 */
	if ((__raw_readl(MXC_CRMAP_ADCR) & MXC_CRMAP_ADCR_VSTAT) != 0) {
		if (mxc_pm_get_scaling_method() == INTEGER_SCALING) {
			mxc_pm_change_voltage(HI_VOLT);
		} else {
			ret_val =
			    mxc_pm_pll_scale_to_high_voltage(armfreq, ahbfreq,
							     ipfreq);
			if (ret_val < 0) {
				return ret_val;
			}
		}
	}

	mxc_pm_set_scaling_method(INTEGER_SCALING);

	/*
	 * Integer scaling means that after we transition to low voltage,
	 * Arm frequency = HFS PLL frequency / ARM Divider / LFDF Divider.
	 *
	 * The AHB and IPG dividers are updated automatically
	 * by the hardware to maintain the 133 and 66.5MHz frequency
	 */

	/*
	 * Get HFSM PLL clock rate.  For Integer scaling on MXC91131, hardware
	 * always uses the HFSM settings.
	 */
	hfs_pllfreq = mhz(mxc_pm_get_pll_freq(1));

	/*
	 * If SDRAMC ratio issue and CRM issue exists, then AP_DIV may be 2.
	 * So when doing Integer DVFS, lfdf divider should be properly chosen to
	 * take this divide by 2 into account. The driver won't change the ARM
	 * divider after LFDF divider is updated and voltage changes.
	 */
	if (mxc_get_clocks_div(CPU_CLK) == 2) {
		hfs_pllfreq /= 2;
	}

	lfdf_ratio = 8;
	while (((mhz(armfreq) * lfdf_ratio) > hfs_pllfreq) && (lfdf_ratio > 1)) {
		lfdf_ratio /= 2;
	}

	if (armfreq < BREAKPT_FREQ) {
		mxc_pm_set_clock_dividers(mxc_pm_get_pll_freq(1), ahbfreq,
					  ipfreq);

		mxc_pm_set_lfdf(lfdf_ratio);

		mxc_pm_change_voltage(LO_VOLT);
	} else {
		mxc_pm_set_clock_dividers(armfreq, ahbfreq, ipfreq);
	}

	return 0;
}

/*!
 * Implementing steps required to set PLL Scaling using patref, noclock, alt_pll
 *
 * @param   armfreq     The desired ARM frequency in Hz. AHB and IP
 *                      frequency are changed depending on ARM
 *                      frequency and the divider values.
 * @param   ahbfreq     The desired AHB frequency in Hz
 * @param   ipfreq      The desired IP frequency in Hz
 *
 * @return              Returns 0 on success or
 *                      Returns -1 on error
 */
int mxc_pm_pllscale(long armfreq, long ahbfreq, long ipfreq)
{
	int pll_num, ret_val;

	PM_DEBUG("%s\n", __FUNCTION__);

	ret_val = mxc_pm_check_parameters(armfreq, ahbfreq, ipfreq);
	if (ret_val < 0) {
		return ret_val;
	}

	/*
	 * We keep the pdf==1 so can't do PLL relock to frequencies
	 * lower than REF_FREQ * 10 == 336 MHz.
	 */
	if (armfreq < (REF_FREQ * 10)) {
		return -PLL_DVFS_FREQ_TOO_LOW;
	}

	/* Find out whether AP being clocked from AP PLL or BP PLL. */
	pll_num = ((__raw_readl(MXC_CRMAP_ASCSR) & MXC_CRMAP_ASCSR_APSEL) != 0);

	/*
	 * If we're at the low voltage, either:
	 * Do integer scaling to hi V, then do PLL scaling, or
	 * Program PLL for a useful freq and PLL scale to hi V or
	 * PLL Scale without going to high voltage.
	 */
	if ((__raw_readl(MXC_CRMAP_ADCR) & MXC_CRMAP_ADCR_VSTAT) != 0) {

		if (mxc_pm_get_scaling_method() == INTEGER_SCALING) {

			/* Do integer scaling to high voltage */
			mxc_pm_change_voltage(HI_VOLT);

			/* At high voltage, we can change scaling method */
			mxc_pm_set_scaling_method(PLL_RELOCK);

			ret_val = mxc_pm_program_pll(armfreq, pll_num, 0);
			if (ret_val < 0) {
				return ret_val;
			}

			if (armfreq < BREAKPT_FREQ) {
				mxc_pm_change_voltage(LO_VOLT);
			}
		} else {
			mxc_pm_set_scaling_method(PLL_RELOCK);

			ret_val =
			    mxc_pm_pll_scale_to_high_voltage(armfreq, ahbfreq,
							     ipfreq);
			if (ret_val < 0) {
				return ret_val;
			}

			if (armfreq < BREAKPT_FREQ) {
				ret_val =
				    mxc_pm_program_pll(armfreq, pll_num, 0);
				if (ret_val < 0) {
					return ret_val;
				}
				mxc_pm_change_voltage(LO_VOLT);
			}
		}
	} else {
		/* We're already at high voltage; do PLL relock */
		mxc_pm_set_scaling_method(PLL_RELOCK);

		ret_val = mxc_pm_program_pll(armfreq, pll_num, 0);
		if (ret_val < 0) {
			return ret_val;
		}

		if (armfreq < BREAKPT_FREQ) {
			mxc_pm_change_voltage(LO_VOLT);
		}
	}

	mxc_pm_set_clock_dividers(armfreq, ahbfreq, ipfreq);

	return 0;
}

/*!
 * To find the suitable scaling techniques if user is unknown of the choice
 * between Integer scaling and PLL scaling
 *
 * @param       armfreq         The desired ARM frequency in Hz
 * @param       ahbfreq         The desired AHB frequency in Hz
 * @param       ipfreq          The desired IP frequency in Hz
 *
 * @return             Returns 0 on success
 */
int mxc_pm_dvfs(unsigned long armfreq, long ahbfreq, long ipfreq)
{
	int retval;
#ifdef CONFIG_MXC_LL_PM_DEBUG
	unsigned long current_armfreq, current_ahbfreq, current_ipfreq;
#endif

	retval = mxc_pm_check_parameters(armfreq, ahbfreq, ipfreq);
	if (retval < 0) {
		return retval;
	}

	PM_DEBUG_STATUS("%s: requested ARM=%d.  Current HFS PLL=%d\n",
			__FUNCTION__, (int)armfreq,
			(int)mhz(mxc_pm_get_pll_freq(1)));

	/*
	 * PLL scaling can only get us frequencies >= 336MHz.
	 * So, we use integer scaling for 336MHz and even if the divider factor
	 * may not be integer multiple
	 */
	if (armfreq < (REF_FREQ * 10)) {
		PM_DEBUG("\nInt scaling to armfreq = %d\n", (int)armfreq);
		retval = mxc_pm_intscale(armfreq, ahbfreq, ipfreq);
	} else {
		PM_DEBUG("\nPLL scaling to armfreq = %d\n", (int)armfreq);
		retval = mxc_pm_pllscale(armfreq, ahbfreq, ipfreq);
	}

#ifdef CONFIG_MXC_LL_PM_DEBUG
	PM_DEBUG_STATUS("\n%s: After the dust settles:\n", __FUNCTION__);

	current_armfreq = mhz(mxc_get_clocks(CPU_CLK));

	PM_DEBUG("Requested: %d  Got %d  PLLfreq = %d\n\n",
		 (int)mhz(armfreq),
		 (int)current_armfreq, (int)mhz(mxc_pll_clock(MCUPLL)));

	if (((mhz(armfreq) == 133) && (current_armfreq != 133)) ||
	    ((mhz(armfreq) == 266) && (current_armfreq != 266)) ||
#if (PM_ARM_DIV_ISSUE == 0)
	    ((mhz(armfreq) == 399) && (current_armfreq != 399)) ||
#else
	    ((mhz(armfreq) == 399) && (current_armfreq != 200)) ||
#endif
#if (PM_SDRAMC_RATIO_ISSUE == 0)
	    ((mhz(armfreq) == 532) && (current_armfreq != 532))) {
#else
	    ((mhz(armfreq) == 532) && (current_armfreq != 266))) {
#endif
		PM_DEBUG("Halting because we got the wrong freq.\n");
		while (1) {
		};
	}
#endif				/* CONFIG_MXC_LL_PM_DEBUG */

	return retval;
}

/*!
 * Interrupt service routine registered to handle the DSM interrupt.
 *
 * @param   irq         the interrupt number
 * @param   dev_id      driver private data
 * @param   regs        holds a snapshot of the processor's context
 *                      before the processor entered the interrupt code
 *
 * @return              The function returns:
 *                      \b IRQ_RETVAL(1) if interrupt was handled and
 *                      \b IRQ_RETVAL(0) if the interrupt was not handled.
 *                      \b IRQ_RETVAL is defined in \b include/linux/interrupt.h.
 */
static irqreturn_t mxc_pm_inthand(int irq, void *dev_id, struct pt_regs *regs)
{
	int handled = 0;
	unsigned int sreg1, status = 0;

	sreg1 = __raw_readl(DSM_INT_STATUS);
	if (sreg1 & INT_STATUS_RWK) {
		/* When wake up is caused by external interrupt */
		__raw_writel((__raw_readl(DSM_MASK_REG) | 0xFFFFFC00),
			     DSM_MASK_REG);
		__raw_writel((__raw_readl(DSM_INT_STATUS) | INT_STATUS_RWK),
			     DSM_INT_STATUS);
		handled = 1;
	} else if (sreg1 & INT_STATUS_WKTIME_INT) {
		/* When wake up is caused by wakeup timer */
		status |= (INT_STATUS_WKTIME_INT);
		__raw_writel(status, DSM_INT_STATUS);
		handled = 1;
	} else {
		/* for all other interrupts */
		status |= 0x0000017F;
		__raw_writel(status, DSM_INT_STATUS);
		handled = 1;
	}
	return IRQ_RETVAL(handled);
}

/*!
 * Implementing steps required to transition to low-power modes
 *
 * @param       mode            The desired low-power mode. Possible values
 *                              are WAIT_MODE, STOP_MODE, and DSM_MODE.
 *
 * @return                      None
 */
void mxc_pm_lowpower(int mode)
{
	unsigned int crm_ctrl, ctrl_slp;
#ifdef CONFIG_MOT_WFN478
        unsigned long flags;
#endif

	if ((__raw_readl(AVIC_VECTOR) & MXC_WFI_ENABLE) == 0) {
		/*
		 * If JTAG is connected then WFI is not enabled
		 * So return
		 */
		return;
	}

	/* Check parameter */
	if ((mode != WAIT_MODE) && (mode != STOP_MODE) && (mode != DSM_MODE)) {
		return;
	}

#ifdef CONFIG_MOT_WFN478
        local_irq_save(flags);
#else
	local_irq_disable();
#endif

	/* Turning on keypad clock to run during STOP mode */
	__raw_writel((__raw_readl(MXC_CRMAP_AMLPMRA) | 0x00000800),
		     MXC_CRMAP_AMLPMRA);

	switch (mode) {
	case WAIT_MODE:
		/* Writing '01' to CRM_lpmd bits */
		crm_ctrl =
		    (__raw_readl(DSM_CRM_CONTROL) & ~CRM_CTRL_LPMD1) |
		    CRM_CTRL_LPMD0;
		__raw_writel(crm_ctrl, DSM_CRM_CONTROL);
		break;

	case STOP_MODE:
		/* Bypass all acknowledgements */
		__raw_writel((__raw_readl(MXC_CRMAP_AMCR) | 0x007F0000),
			     MXC_CRMAP_AMCR);

		/* Enable KPP Interrupt */
		__raw_writel((__raw_readl(LO_INTR_EN) | 0x01000000),
			     LO_INTR_EN);

		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & 0xCFFFFFFF),
			     LO_INTR_EN);

		/* Writing '00' to CRM_lpmd bits */
		crm_ctrl =
		    __raw_readl(DSM_CRM_CONTROL) & ~CRM_CTRL_LPMD1 &
		    ~CRM_CTRL_LPMD0;
		__raw_writel(crm_ctrl, DSM_CRM_CONTROL);
		break;

	case DSM_MODE:
		/* Bypass all acknowledgements and enable WellBias */
		__raw_writel((__raw_readl(MXC_CRMAP_AMCR) | 0x007F0001),
			     MXC_CRMAP_AMCR);

		/* Enable KPP Interrupt */
		__raw_writel((__raw_readl(LO_INTR_EN) | 0x01000000),
			     LO_INTR_EN);

		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & 0xCFFFFFFF),
			     LO_INTR_EN);

		/* Enable DSM */
		__raw_writel((__raw_readl(DSM_CONTROL0) | CONTROL0_MSTR_EN),
			     DSM_CONTROL0);

		/* Enable SLEEP and WAKE UP DISABLE */
		ctrl_slp =
		    __raw_readl(DSM_CONTROL1) | CONTROL1_SLEEP |
		    CONTROL1_WAKEUP_DISABLE;
		__raw_writel(ctrl_slp, DSM_CONTROL1);

		if (CONFIG_RESET_COUNT32) {
			/* Set RST_COUNT32_EN bit */
			__raw_writel((__raw_readl(DSM_CONTROL1) |
				      CONTROL1_RST_CNT32_EN), DSM_CONTROL1);
		}

		/* Set STBY_COMMIT_EN bit */
		__raw_writel((__raw_readl(DSM_CONTROL0) |
			      CONTROL0_STBY_COMMIT_EN), DSM_CONTROL0);

		/* Set RESTART bit */
		__raw_writel((__raw_readl(DSM_CONTROL0) | CONTROL0_RESTART),
			     DSM_CONTROL0);

		if ((__raw_readl(DSM_CONTROL1) & CONTROL1_RST_CNT32_EN)) {
			/*
			 * If RST_CNT32_EN bit is set, all the registers
			 * can be cleared to 0
			 */
			__raw_writel(0x00000200, DSM_SLEEP_TIME);
			__raw_writel(0, DSM_WAKE_TIME);
			__raw_writel(0, DSM_WARM_TIME);
			__raw_writel(0, DSM_LOCK_TIME);
			__raw_writel(0, DSM_RESTART_TIME);
		} else {
			/*
			 * The max value for COUNT32 register before it is reset is
			 * 65 seconds before it is reset. So, I have used some arbitrary
			 * values such that the total time for DSM which is 0x00055fff
			 * here is within this value. These values are bound to change
			 * after testing.
			 */
			__raw_writel(0x00000200, DSM_SLEEP_TIME);
			__raw_writel(0x001fffff, DSM_WAKE_TIME);
			__raw_writel(0x00000500, DSM_WARM_TIME);
			__raw_writel(0x00000700, DSM_LOCK_TIME);
			__raw_writel(0x001fffff, DSM_RESTART_TIME);
		}
		__raw_writel((__raw_readl(DSM_CTREN) | DSM_CTREN_CNT32),
			     DSM_CTREN);

		/* Writing '00' to CRM_lpmd bits */
		crm_ctrl =
		    __raw_readl(DSM_CRM_CONTROL) & ~CRM_CTRL_LPMD1 &
		    ~CRM_CTRL_LPMD0;
		__raw_writel(crm_ctrl, DSM_CRM_CONTROL);
		break;
	}

	/* Executing CP15 (Wait-for-Interrupt) Instruction */
	cpu_do_idle();

	/* Enable timer interrupt */
	__raw_writel((__raw_readl(LO_INTR_EN) | 0x30000000), LO_INTR_EN);

#ifdef CONFIG_MOT_WFN478
        local_irq_restore(flags);
#else
	local_irq_enable();
#endif
}

/*!
 * Implementing Level 1 CRM Gate Control. Level 2 gate control
 * is provided at module level using LPMD registers
 *
 * @param   group       The desired clock gate control register bits.
 *                      Possible values are 0 through 6
 * @param   opt         The desired option requesting clock to run
 *                      during stop and wait modes or just during
 *                      the stop mode. Possible values are
 *                      GATE_STOP_WAIT and GATE_STOP.
 */
void mxc_pm_clockgate(int group, int opt)
{
	if (opt == GATE_STOP_WAIT) {
		__raw_writel(((__raw_readl(MXC_CRMAP_ACGCR) &
			       (~gate_run[group])) | (gate_stop_wait[group])),
			     MXC_CRMAP_ACGCR);
	} else if (opt == GATE_STOP) {
		__raw_writel(((__raw_readl(MXC_CRMAP_ACGCR) &
			       (~gate_run[group])) | (gate_stop[group])),
			     MXC_CRMAP_ACGCR);
	}
}

#ifdef CONFIG_MOT_FEAT_PM
/************************************************************************************
 *  PLLs dithering                                                                  *
 ************************************************************************************/

/*!
 * Dithering enable/disable of the AP and USB PLLs
 * Must be called only if a non 0 ppm value has been setup via mxc_pm_dither_setup
 *
 * @param   enable    0: disable, 1: enable
 * @param   pll_number of the type ap_pll_number_t: PLL to enable/disable
 *
 * @return 0 OK, -1 wrong PLL number
 */
int mxc_pm_dither_control(int enable, ap_pll_number_t pll_number)
{
	/* Unimplemented for this hardware */
        return 0;
}

/*!
 * Dithering setup: pass the PLLs ppm values. This function calculates
 *   and stores the MFN+- values locally for the AP PLL,
 *   sets up the USB PLL dithering, and enables the dithering.
 *
 * @param   ap_core_normal_pll_ppm    ppm value of AP core PLL, normal mode
 * @param   ap_core_turbo_pll_ppm     ppm value of AP core PLL, turbo mode
 * @param   ap_usb_pll_ppm            ppm value of USB PLL
 *
 * @return   Returns 0 on success, -1 on invalid ppm value(s)
 */
int mxc_pm_dither_setup(u16 ap_core_normal_pll_ppm,
                        u16 ap_core_turbo_pll_ppm,
			u16 ap_usb_pll_ppm)
{
	/* Unimplemented for this hardware */
	return 0;
}


/*!
 * Dithering report: return dithering information for all AP PLLs
 * The dithering information includes the PPM and MFN+- values for
 * HFS and LFS.
 *
 * @param   pllvals    ptr to dithering info for all AP PLLs
 */
void mxc_pm_dither_report(ap_all_plls_mfn_values_t *pllvals)
{
	/* Unimplemented for this hardware */
        memset(pllvals, 0, sizeof(*pllvals));
}
#endif /* CONFIG_MOT_FEAT_PM */

/*!
 * This function is used to load the module.
 *
 * @return  returns 0 on success (always)
 */
static int __init mxc_pm_init_module(void)
{
	unsigned long adcr_val, dp_ctl;

	PM_DEBUG_STATUS("%s - entry:  CSCR=0x%08x\n", __FUNCTION__,
			(int)(__raw_readl(MXC_CRMCOM_CSCR)));

	/* Registering DSM interrupt handler */
	if (request_irq(INT_DSM_AP, mxc_pm_inthand, 0, "mxc_pm_dsm", NULL)) {
		printk("Low-Level PM Driver module failed to load\n");
		return -EBUSY;
	}

	/*
	 * MXC91131 initialization
	 *
	 * Redboot will program both the Normal and HFS PLL registers
	 * to 399 MHz and lock the PLL.
	 *
	 * We will do an Integer Scaling DVFS transition to high voltage
	 * then a PLL Scaling transition to low voltage to synchronize
	 * the VSTAT and HFSM bits.
	 */

	mxc_pm_set_scaling_method(INTEGER_SCALING);

	/*
	 * Set delay value for ap_delay to wait for voltage to settle
	 * before changing frequency.
	 */
	adcr_val =
	    (__raw_readl(MXC_CRMAP_ADCR) & 0x0000ffff) | AP_DELAY_SETTING;
	__raw_writel(adcr_val, MXC_CRMAP_ADCR);

	/*
	 * Clear the DVFS_BYP bypass bit to allow voltage change.
	 */
	__raw_writel(__raw_readl(DSM_CRM_CONTROL) & ~CRM_CTRL_DVFS_BYP,
		     DSM_CRM_CONTROL);

	mxc_pm_change_voltage(HI_VOLT);

	/*
	 * MXC91131-specific weirdness:
	 * At this point, VSTAT=0 (hi voltage) and HFSM=1 (hi voltage mode)
	 * So these bits are synchronized.
	 * Now for all PLL transitions They will stay synchronized.
	 * For Integer transitions, HFSM stays at 1 even when at the low
	 * voltage and the PLL takes it settings from the HFS shadow
	 * registers even at low voltage.
	 */

	mxc_pm_set_lfdf(1);

	PM_DEBUG_STATUS("%s: after going to Hi Voltage:\n", __FUNCTION__);

	/* Now do PLL scaling request to low voltage */

	mxc_pm_set_scaling_method(PLL_RELOCK);

	/*
	 * Set AREN bit for PLL Auto restart enable for PLL scaling
	 */
	__raw_writel(__raw_readl(MXC_PLL0_DP_CONFIG) | DP_AREN,
		     MXC_PLL0_DP_CONFIG);

	/*
	 * Set VCTRL to signal that we want AP to go to low voltage.
	 */
	mxc_pm_change_voltage(LO_VOLT);

	PM_DEBUG_STATUS("%s: after going back to Lo Voltage:\n", __FUNCTION__);

	/*
	 * Select ARM clock divided by 10 to go out on CK_OH so it can be
	 * measured.
	 */
	/* mxc_pm_ckohsel(1); */

	/* Init settings for PLL */
	dp_ctl =
	    __raw_readl(MXC_PLL0_DP_CTL) | DP_CTL_UPEN | DP_CTL_RST |
	    DP_CTL_BRMO;
	__raw_writel(dp_ctl, MXC_PLL0_DP_CTL);

	/*
	 * Possible int divisors are 1 through 16.
	 * These are theoretical values. Most settings
	 * require a ratio of 1:4:8 for ARM:AHB:IP. Also,
	 * by default these divisors are set to 1:4:8 because,
	 * in case of integer scaling, the ratio is changed to
	 * 1:2:4 for low freq setting and in case of PLL scaling this
	 * remains as such
	 */
	printk("Low-Level PM Driver module loaded\n");
	return 0;
}

/*!
 * This function is used to unload the module
 */
static void __exit mxc_pm_cleanup_module(void)
{
	printk("Low-Level PM Driver module Unloaded\n");
}

module_init(mxc_pm_init_module);
module_exit(mxc_pm_cleanup_module);

EXPORT_SYMBOL(mxc_pm_dvfs);
EXPORT_SYMBOL(mxc_pm_intscale);
EXPORT_SYMBOL(mxc_pm_pllscale);
EXPORT_SYMBOL(mxc_pm_clockgate);
EXPORT_SYMBOL(mxc_pm_lowpower);
#ifdef CONFIG_MOT_FEAT_PM
/* De-sense */
EXPORT_SYMBOL(mxc_pm_dither_control);
EXPORT_SYMBOL(mxc_pm_dither_setup);
EXPORT_SYMBOL(mxc_pm_dither_report);
#endif /* CONFIG_MOT_FEAT_PM */

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC91131 Low-level PM Driver");
MODULE_LICENSE("GPL");
