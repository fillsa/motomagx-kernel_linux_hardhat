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
 * 12/14/2006  Motorola  Changed arguments to mxc_pm_dither_control.
 * 12/25/2006  Motorola  Changed local_irq_disable/local_irq_enable to
 *                       local_irq_save/local_irq_restore.
 */

/*!
 * @file mxc_pm.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for the CRM_MCU and DPLL in MXC91321.
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
#include <asm/arch/mxc_pm.h>
#include <asm/arch/clock.h>
#include <asm/arch/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include "crm_regs.h"

#define LO_INTR_EN              IO_ADDRESS(AVIC_BASE_ADDR + 0x14)

/* Address offsets of MXC91321 CRM_MCU(CCM) registers */

#define CRM_MCU_MCR             IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x00)
#define CRM_MCU_MPDR0           IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x04)
#define CRM_MCU_MPDR1           IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x08)
#define CRM_MCU_MPCTL           IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x10)
#define CRM_MCU_MCGR0           IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x1c)
#define CRM_MCU_MCGR1           IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x20)
#define CRM_MCU_MCGR2           IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x24)
#define CRM_MCU_COSR            IO_ADDRESS(CRM_MCU_BASE_ADDR + 0x18)

/* Bit definitions of CRM_MCU registers */

#define MCR_LPMD1               0x40000000
#define MCR_LPMD0               0x20000000
#define MCR_WBEN                0x80000000
#define MCR_SBYOS               0x00002000
#define MCR_DPE                 0x00000002

/* AHB, IPG max frequency and Reference freq values */
#define AHB_FREQ_MAX            100000000
#define IPG_FREQ_MAX            50000000
#define AHB_MAX_DIV             8
#define IPG_MAX_DIV             2

/*
 * Timer Interrupt
 */
#define AVIC_TIMER_DIS		0xC7FFFFFF
#define AVIC_TIMER_EN		0x38000000

/*
 * LPMD Mask
 */
#define LPM_MASK		0x9FFFFFFF

/*
 * COSR Register Mask
 */
#define COSR_REG_MASK		0xFFF803FF
#define COSR_CKO2_EN		0x00002000
#define COSR_CKO2_MASK		0xFFFC3FFF

/*
 * BRMM Mask
 */
#define BRMM_MASK		0xFFFFFFF8

/*
 * AHB/IPG/BRMM Mask
 */
#define AHB_IPG_BRMM_DIV_MASK	0xFFFFFF07

/*!
 * To find the BRM divider which supports the next lowest ARM frequency
 * supported using Integer Scaling ie., changing BRMM value to get
 * the desired ARM frequency
 *
 * @param   armfreq    The desired ARM frequency in MHz
 *
 * @return             Returns BRM divider value
 */
static int mxc_pm_findnext(long armfreq)
{
	unsigned long mpdr, mcu_main_clk, max_clk, t_pll, t_max, t_arm;
	unsigned int brmm = 0, max_pdf;

	if (armfreq < 100 || armfreq > 400) {
		return -INVALID_ARM_FREQ;
	}
	mcu_main_clk = (mxc_pll_clock(MCUPLL) + 999999) / 1000000;
	mpdr = __raw_readl(CRM_MCU_MPDR0);
	max_pdf =
	    (mpdr & MXC_CCM_MPDR0_MAX_PDF_MASK) >> MXC_CCM_MPDR0_MAX_PDF_OFFSET;
	max_clk = mcu_main_clk / (max_pdf + 1);
	/* Calculate different clock periods */
	t_pll = 1000000 / mcu_main_clk;
	t_max = 1000000 / max_clk;
	t_arm = 1000000 / armfreq;
	if ((t_arm >= t_pll) && (t_arm <= (((2 * t_pll) + (t_max)) / 3))) {
		brmm = 1;
	} else if ((t_arm >= (((2 * t_pll) + (t_max)) / 3)) &&
		   (t_arm <= ((t_pll + t_max) / 2))) {
		brmm = 2;
	} else if ((t_arm >= ((t_pll + t_max) / 2)) &&
		   (t_arm <= ((t_pll + (2 * t_max)) / 3))) {
		brmm = 3;
	} else if ((t_arm >= ((t_pll + (2 * t_max)) / 3)) && (t_arm <= t_max)) {
		brmm = 4;
	} else if (t_arm >= t_max) {
		brmm = 4;
	}
	return brmm;
}

/*!
 * Calculating and setting possible divider values for MAX and IPG
 *
 * @param   armfreq    The ARM frequency desired or changed as a result
 *                     of PLL or Integer scaling
 * @param   ahbfreq    The desired AHB frequency
 * @param   ipfreq     The desired IP frequency
 * @param   intorpll   If Integer scaling or PLL scaling is desired. This is
 *                     used in order to get the correct PLL frequency
 *
 * @return             Returns -FREQ_OUT_OF_RANGE if AHB freq is
 *                     greater than 133MHz or if IPG freq is greater than
 *                     66.5MHz and 0 on success
 */
static int mxc_pm_div_adjust(long armfreq, long ahbfreq, long ipfreq,
			     int intorpll)
{
	unsigned long max_clk;
	unsigned int max_pdf, ipg_pdf;

	if (ipfreq < 0 || ahbfreq > AHB_FREQ_MAX || ipfreq > IPG_FREQ_MAX
	    || (ahbfreq / ipfreq) > 2) {
		return -FREQ_OUT_OF_RANGE;
	}
	if (armfreq < 1000000 || armfreq > 400000000) {
		return -INVALID_ARM_FREQ;
	}
	if (intorpll > 0) {
		/* If Integer Scaling is used */
		armfreq =
		    ((mxc_pll_clock(MCUPLL) + 999999) / 1000000) * 1000000;
	}
	armfreq = armfreq / 1000000;
	ahbfreq = ahbfreq / 1000000;
	ipfreq = ipfreq / 1000000;
	max_pdf = (armfreq / ahbfreq) - 1;
	max_clk = armfreq / (max_pdf + 1);
	ipg_pdf = (max_clk / ipfreq) - 1;
	if ((max_clk > ahbfreq) || (max_clk > AHB_FREQ_MAX)) {
		max_pdf++;
		max_clk = armfreq / (max_pdf + 1);
	}
	if (max_pdf > AHB_MAX_DIV) {
		/* If max_pdf > 8 then return error */
		return -AHB_MAX_DIV_ERR;
	}
	if ((max_clk / (ipg_pdf + 1)) > ipfreq) {
		ipg_pdf++;
	}
	if (ipg_pdf > IPG_MAX_DIV) {
		/* If ipg_div > 2 then return error */
		return -IPG_MAX_DIV_ERR;
	}
	__raw_writel((((__raw_readl(CRM_MCU_MPDR0) & AHB_IPG_BRMM_DIV_MASK) |
		       (max_pdf << MXC_CCM_MPDR0_MAX_PDF_OFFSET))
		      | (ipg_pdf << MXC_CCM_MPDR0_IPG_PDF_OFFSET)),
		     CRM_MCU_MPDR0);
	return 0;
}

/*!
 * Implementing steps required to set Integer Scaling (which means
 * changing BRMM value in order to get the desired ARM frequency)
 * Also, PDF dividers are changed to maintain the AHB and IP frequency
 *
 * @param   armfreq    The desired ARM frequency. AHB and IP
 *                     frequency are changed depending on ARM
 *                     frequency and the divider values.
 * @param   ahbfreq    The desired AHB frequency
 * @param   ipfreq     The desired IP frequency
 *
 * @return             Returns 0 on success or
 *                     Returns -PLL_LESS_ARM_ERR if pllfreq is less than
 *                     desired core freq
 */
int mxc_pm_intscale(long armfreq, long ahbfreq, long ipfreq)
{
	unsigned long mpdr, mcu_main_clk, max_clk, t_pll, t_max, t_arm;
	long armfreq_mhz, ahbfreq_mhz, ipfreq_mhz;
	unsigned int brmm = 0, max_pdf, ipg_pdf, ret_val = 0;

	if (ahbfreq != AHB_FREQ_MAX || ipfreq != IPG_FREQ_MAX) {
		return -BUS_FREQ_INVALID;
	}
	if (armfreq < 1000000 || armfreq > 400000000) {
		return -INVALID_ARM_FREQ;
	}
	mcu_main_clk = (mxc_pll_clock(MCUPLL) + 999999) / 1000000;
	armfreq_mhz = armfreq / 1000000;
	ahbfreq_mhz = ahbfreq / 1000000;
	ipfreq_mhz = ipfreq / 1000000;
	if (armfreq_mhz > mcu_main_clk) {
		return -PLL_LESS_ARM_ERR;
	}
	mpdr = __raw_readl(CRM_MCU_MPDR0);
	max_pdf =
	    (mpdr & MXC_CCM_MPDR0_MAX_PDF_MASK) >> MXC_CCM_MPDR0_MAX_PDF_OFFSET;
	ipg_pdf =
	    (mpdr & MXC_CCM_MPDR0_IPG_PDF_MASK) >> MXC_CCM_MPDR0_IPG_PDF_OFFSET;
	max_clk = mcu_main_clk / (max_pdf + 1);
	if ((ahbfreq <= AHB_FREQ_MAX) && (max_clk != ahbfreq_mhz)) {
		ret_val = mxc_pm_div_adjust(armfreq, ahbfreq, ipfreq, 1);
		if (ret_val < 0) {
			return ret_val;
		}
	}
	mpdr = __raw_readl(CRM_MCU_MPDR0);
	max_pdf =
	    (mpdr & MXC_CCM_MPDR0_MAX_PDF_MASK) >> MXC_CCM_MPDR0_MAX_PDF_OFFSET;
	ipg_pdf =
	    (mpdr & MXC_CCM_MPDR0_IPG_PDF_MASK) >> MXC_CCM_MPDR0_IPG_PDF_OFFSET;
	max_clk = mcu_main_clk / (max_pdf + 1);
	t_pll = 10000 / mcu_main_clk;
	t_max = 10000 / max_clk;
	t_arm = 10000 / armfreq_mhz;
	if (t_arm == t_pll) {
		brmm = 0;
	} else if (t_arm == (((2 * t_pll) + (t_max)) / 3)) {
		brmm = 1;
	} else if (t_arm == ((t_pll + t_max) / 2)) {
		brmm = 2;
	} else if (t_arm == ((t_pll + (2 * t_max)) / 3)) {
		brmm = 3;
	} else if (t_arm == t_max) {
		brmm = 4;
	} else {
		brmm = mxc_pm_findnext(armfreq_mhz);
		if (brmm < 0) {
			return brmm;
		}
	}
	mpdr = __raw_readl(CRM_MCU_MPDR0);
	__raw_writel(((mpdr & BRMM_MASK) | brmm), CRM_MCU_MPDR0);
	return 0;
}

/*!
 * To calculate and set MFI, MFN, MFD values. Using this the output frequency
 * whose value is calculated using,
 * 2 * REF_FREQ * (MF / PDF), where
 * REF_FREQ is 26 Mhz
 * MF = MFI + (MFN + MFD)
 * PDF is assumed to be 1
 *
 * @param   armfreq    The desired ARM frequency
 * @param   ahbfreq    The desired AHB frequency
 * @param   ipfreq     The desired IP frequency
 *
 * @return             Returns 0 on success or
 *                     Returns -1 on error
 */
int mxc_pm_pllscale(long armfreq, long ahbfreq, long ipfreq)
{
	signed long mf, mfi, mfn, mfd, pdf, ref_freq, freq_out, pllfreq;
	int brmm, ret_val = 0;
	unsigned long mpctl = __raw_readl(CRM_MCU_MPCTL);

	if (ahbfreq != AHB_FREQ_MAX || ipfreq != IPG_FREQ_MAX) {
		return -BUS_FREQ_INVALID;
	}
	if (armfreq < 1000000 || armfreq > 400000000) {
		return -INVALID_ARM_FREQ;
	}
	if (((armfreq % ahbfreq) != 0) || ((armfreq % ipfreq) != 0)) {
		return -INVALID_ARM_FREQ;
	}
	pdf =
	    (signed long)((mpctl & MXC_CCM_PCTL_PDF_MASK) >>
			  MXC_CCM_PCTL_PDF_OFFSET);
	ref_freq = mxc_get_clocks(CKIH_CLK);;
	pllfreq = mxc_pll_clock(MCUPLL);
	mf = (armfreq * (pdf + 1)) / (2 * ref_freq);
	mfi = (int)mf;
	mfn = (armfreq - (2 * mf * ref_freq)) / 1000000;
	mfd = (2 * ref_freq) / 1000000;
	mfi = (mfi <= 5) ? 5 : mfi;
	mfn = (mfn <= 0x400) ? mfn : (mfn - 0x800);
	/*
	 * Calculate freq_out to see whether required freq is
	 * less than the calculated freq. If calc freq is greater then
	 * the actual output freq value should be minimum of current
	 * PLL freq or total possible min freq
	 */
	freq_out = (2 * ref_freq * mfi + ((2 * ref_freq / (mfd + 1)) * mfn)) /
	    (pdf + 1);
	if (freq_out > armfreq) {
		if (pllfreq <= freq_out) {
			/* retain mfd and mfn values to old ones */
			mfn = mpctl & 0xFFFFF800;
		} else if (mfi == 0x5) {
			/*
			 * Reset mfn and mfd to 0, so we get minimun
			 * possible frequency obtainable using
			 * PLL scaling which is 260MHz when pdf = 1
			 */
			mfn = mfd = 0;
		}
	}
	if (armfreq > pllfreq) {
		/*
		 * If armfreq is greater than the pllfreq, calculate
		 * the dividers before changing the PLL
		 */
		ret_val = mxc_pm_div_adjust(armfreq, ahbfreq, ipfreq, -1);
		if (ret_val < 0) {
			return ret_val;
		}
	}
	/* Check if BRMM is 1 */
	brmm = __raw_readl(CRM_MCU_MPDR0) & 0x00000007;
	mpctl = (mpctl & 0xFC000000) | (mfd << 16) | (mfi << 11) | mfn;
	__raw_writel(mpctl, CRM_MCU_MPCTL);
	if (brmm > 0) {
		__raw_writel((__raw_readl(CRM_MCU_MPDR0) & BRMM_MASK),
			     CRM_MCU_MPDR0);
	}
	if (armfreq < pllfreq) {
		ret_val = mxc_pm_div_adjust(armfreq, ahbfreq, ipfreq, -1);
		if (ret_val < 0) {
			return ret_val;
		}
	}
	return ret_val;
}

/*!
 * To find the suitable scaling techniques if user is unknown of the choice
 * between Integer scaling and PLL scaling
 *
 * @param       armfreq         The desired ARM frequency in Hz
 * @param       ahbfreq         The desired AHB frequency in Hz
 * @param       ipfreq          The desired IP frequency in Hz
 *
 * @return             Returns 0 on success and non-zero value on error
 */
int mxc_pm_dvfs(unsigned long armfreq, long ahbfreq, long ipfreq)
{
	int ret_val;

	if (ahbfreq != AHB_FREQ_MAX || ipfreq != IPG_FREQ_MAX) {
		return -BUS_FREQ_INVALID;
	}
	if (armfreq < 1000000 || armfreq > 400000000) {
		return -INVALID_ARM_FREQ;
	}
	/* PLL relock techniques uses no_clock option. So,
	 * Integer scaling is only used */
	ret_val = mxc_pm_intscale(armfreq, ahbfreq, ipfreq);

	return ret_val;
}

/*!
 * Implementing steps required to transition to low-power modes
 *
 * @param   mode    The desired low-power mode. Possible values are,
 *                  WAIT_MODE, DOZE_MODE, STOP_MODE or DSM_MODE
 *
 */
void mxc_pm_lowpower(int mode)
{
	unsigned int crm_ctrl;
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
	if ((mode != WAIT_MODE) && (mode != STOP_MODE) && (mode != DSM_MODE) &&
	    (mode != DOZE_MODE)) {
		return;
	}

#ifdef CONFIG_MOT_WFN478
        local_irq_save(flags);
#else
	local_irq_disable();
#endif
	switch (mode) {
	case WAIT_MODE:
		/*
		 * Handled by kernel using arch_idle()
		 * There are no LPM bits for WAIT mode which
		 * are set by software
		 */
		break;
	case DOZE_MODE:
		/* Writing '00' to CRM_lpmd bits */
		/* Bypass all acknowledgements */
		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & AVIC_TIMER_DIS),
			     LO_INTR_EN);
		/* Writing '10' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(CRM_MCU_MCR) & LPM_MASK) | (MCR_LPMD1)) &
		    (~MCR_LPMD0);
		__raw_writel(crm_ctrl, CRM_MCU_MCR);
		break;
	case STOP_MODE:
		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & AVIC_TIMER_DIS),
			     LO_INTR_EN);
		__raw_writel((__raw_readl(CRM_MCU_MCR) & (~MCR_DPE)),
			     CRM_MCU_MCR);
		/* Writing '11' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(CRM_MCU_MCR) & LPM_MASK) | (MCR_LPMD1)) |
		    (MCR_LPMD0);
		__raw_writel(crm_ctrl, CRM_MCU_MCR);
		break;
	case DSM_MODE:
		__raw_writel((__raw_readl(CRM_MCU_MCR) | (MCR_WBEN)),
			     CRM_MCU_MCR);
		__raw_writel((__raw_readl(CRM_MCU_MCR) | (MCR_SBYOS)),
			     CRM_MCU_MCR);
		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & AVIC_TIMER_DIS),
			     LO_INTR_EN);
		__raw_writel((__raw_readl(CRM_MCU_MCR) & (~MCR_DPE)),
			     CRM_MCU_MCR);
		/* Writing '11' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(CRM_MCU_MCR) & LPM_MASK) | (MCR_LPMD1)) |
		    (MCR_LPMD0);
		__raw_writel(crm_ctrl, CRM_MCU_MCR);
		break;
	}
	/* Executing CP15 (Wait-for-Interrupt) Instruction */
	arch_idle();
	/* Enable timer interrupt */
	__raw_writel((__raw_readl(LO_INTR_EN) | AVIC_TIMER_EN), LO_INTR_EN);
#ifdef CONFIG_MOT_WFN478
        local_irq_restore(flags);
#else
	local_irq_enable();
#endif
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
int mxc_pm_dither_control(int enable, enum plls pll_number)
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
 * @return   Returns an Integer on success
 */
static int __init mxc_pm_init_module(void)
{
	unsigned long cosr;

	cosr = __raw_readl(CRM_MCU_COSR);
	__raw_writel(((cosr & COSR_CKO2_MASK) | (COSR_CKO2_EN)), CRM_MCU_COSR);
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

EXPORT_SYMBOL(mxc_pm_intscale);
EXPORT_SYMBOL(mxc_pm_pllscale);
EXPORT_SYMBOL(mxc_pm_dvfs);
EXPORT_SYMBOL(mxc_pm_lowpower);
#ifdef CONFIG_MOT_FEAT_PM
/* De-sense */
EXPORT_SYMBOL(mxc_pm_dither_control);
EXPORT_SYMBOL(mxc_pm_dither_setup);
EXPORT_SYMBOL(mxc_pm_dither_report);
#endif /* CONFIG_MOT_FEAT_PM */

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC91321 Low-level PM Driver");
MODULE_LICENSE("GPL");
