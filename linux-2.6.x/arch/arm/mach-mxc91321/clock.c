/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2007 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Changelog:
 * Date        Author            Comment
 * ==========  ================  ========================
 * 03/09/2007  Motorola          Added DVFS support
 * 04/04/2007  Motorola          Fix clock gating for doze mode
 * 05/15/2007  Motorola          Return correct frequency when 32Khz
 *                               clock source is used for GPT.
 * 06/12/2007  Motorola          Update static clock gating configuration
 * 06/18/2007  Motorola          Added mxc_pll_release_pll and  mxc_pll_request_pll
 *                               have them simply return without doing anything. 
 * 10/23/2007  Motorola          Fix sahara clock gating issue.  
 * 10/30/2007  Motorola          Add Support OWIRE_CLK for mxc_get_clocks().
 * 12/27/2007  Motorola          Didn't printk OWIRE
 */

/*!
 * @file clock.c
 * @brief API for setting up and retrieving clocks.
 *
 * This file contains API for setting up and retrieving clocks.
 *
 * @ingroup CLOCKS
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/arch/clock.h>

#include "crm_regs.h"
#include "iomux.h"

#define MXC_CKIH_FREQ                       26000000
#define MXC_PLL_REF_CLK                     MXC_CKIH_FREQ

#ifdef CONFIG_MOT_FEAT_PM
#define AHB_FREQ_MAX			    128500000
#else
#define AHB_FREQ_MAX			    133000000
#endif

/*
 * Number of PLLs that is used for Core DVFS
 */
#define CORE_DVFS_PLL                       2
/*
 * Number of core operating freuencies for each PLL
 */
#define DVFS_OP_NUM                         5

/*
 * The core operating points 212, 177 and 133MHZ are currently not implemented.
 * These operating points are used only when the user really wants to 
 * lower frequency fast when working with TPLL otherwise it should not be used 
 * since these points operate at 1.6V and hence will not save power
 */
/* 
 * The table is populated by: Example sample calculation shown in row 2
 *********************************************************************
 * BRMM	   Active PLL	AHB	     FORMULA			CORE *
 *           (MHz)     (MHz)                                    (MHz)*
 *********************************************************************
 *  0        399        133	  Core clk = PLL      	 	 399 *
 *-------------------------------------------------------------------*
 *  1        399        133    ((2 * t_PLL) + (t_ahb))/3 =           *
 *                             ((2 * 2.5nsec) + (7.5nsec))/3 =       *
 *                                    4.16nsec                       *
 *                                Core clk = 1/4.16nsec =            *
 *                                    ~240Mhz                    240 *
 *-------------------------------------------------------------------*
 *  2        399        133      (t_PLL + t_ahb)/2               200 *
 *-------------------------------------------------------------------*
 *  3        399        133    ((t_PLL) + (2 * t_ahb))/3         171 *
 *-------------------------------------------------------------------*
 *  4        399        133       Core clk = AHB		 133 *
 *-------------------------------------------------------------------*
 *  0        532        133	  Core clk = PLL      	 	 532 *
 *-------------------------------------------------------------------*
 *  1        532        133    ((2 * t_PLL) + (t_ahb))/3         266 *
 *-------------------------------------------------------------------*
 *  2        532        133      (t_PLL + t_ahb)/2               212 *
 *-------------------------------------------------------------------*
 *  3        532        133    ((t_PLL) + (2 * t_ahb))/3         177 *
 *-------------------------------------------------------------------*
 *  4        532        133       Core clk = AHB		 133 *
 *-------------------------------------------------------------------*
 */
#ifdef CONFIG_MOT_FEAT_PM
static unsigned int core_pll_op_128[CORE_DVFS_PLL][DVFS_OP_NUM] = {
        {385000000, 231000000, 193000000, 165000000, 128000000},
        {514000000, 257000000, 206000000, 171000000, 128000000}
};
#else
static unsigned int core_pll_op_133[CORE_DVFS_PLL][DVFS_OP_NUM] = {
	{399000000, 240000000, 200000000, 171000000, 133000000},
	{532000000, 266000000, 212000000, 177000000, 133000000}
};
#endif

/*
 * Core operating points when AHB clock is 100MHz
 * (same formula as above applies)
 */
static unsigned int core_pll_op_100[CORE_DVFS_PLL][DVFS_OP_NUM] = {
	{399000000, 200000000, 160000000, 133000000, 100000000},
	{500000000, 214000000, 166000000, 136000000, 100000000}
};

/*!
 * Spinlock to protect CRM register accesses
 */
static DEFINE_RAW_SPINLOCK(mxc_crm_lock);

/*!
 * This function returns the PLL output value in Hz based on pll.
 * @param       pll     PLL as defined in enum plls
 * @return      PLL value in Hz.
 */
unsigned long mxc_pll_clock(enum plls pll)
{
	unsigned long mfi = 0, mfn = 0, mfd, pdf, ref_clk;
	unsigned long long temp;
	volatile unsigned long reg;

	if (pll == MCUPLL) {
		reg = __raw_readl(MXC_CCM_MPCTL);
		mfi = (reg & MXC_CCM_MCUPCTL_MFI_MASK) >>
		    MXC_CCM_MCUPCTL_MFI_OFFSET;
		mfi = (mfi <= 5) ? 5 : mfi;
		mfn = (reg & MXC_CCM_MCUPCTL_MFN_MASK);
		mfn = (mfn <= 0x400) ? mfn : (mfn - 0x800);
	} else if (pll == USBPLL) {
		reg = __raw_readl(MXC_CCM_UPCTL);
		mfi = (reg & MXC_CCM_USBPCTL_MFI_MASK) >>
		    MXC_CCM_USBPCTL_MFI_OFFSET;
		mfi = (mfi <= 5) ? 5 : mfi;
		mfn = reg & MXC_CCM_USBPCTL_MFN_MASK;
	} else if (pll == TURBOPLL) {
		reg = __raw_readl(MXC_CCM_TPCTL);
		mfi = (reg & MXC_CCM_TPCTL_MFI_MASK) >>
		    MXC_CCM_TPCTL_MFI_OFFSET;
		mfi = (mfi <= 5) ? 5 : mfi;
		mfn = reg & MXC_CCM_TPCTL_MFN_MASK;
		mfn = (mfn <= 0x400) ? mfn : (mfn - 0x800);
	}

	pdf = (reg & MXC_CCM_PCTL_PDF_MASK) >> MXC_CCM_PCTL_PDF_OFFSET;
	mfd = (reg & MXC_CCM_PCTL_MFD_MASK) >> MXC_CCM_PCTL_MFD_OFFSET;
	ref_clk = MXC_PLL_REF_CLK;

	if (mfn < 1024) {
		temp = (unsigned long long)2 *ref_clk * mfn;
		do_div(temp, mfd + 1);
		temp = (unsigned long long)2 *ref_clk * mfi + temp;
		do_div(temp, pdf + 1);
	} else {
		temp = (unsigned long long)2 *ref_clk * (2048 - mfn);
		do_div(temp, mfd + 1);
		temp = (unsigned long long)2 *ref_clk * mfi - temp;
		do_div(temp, pdf + 1);
	}

	return (unsigned long)temp;
}

/*!
 * This function returns the parent clock values in Hz.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks_parent(enum mxc_clocks clk)
{
	unsigned long ret_val = 0, clksel = 0;
	volatile unsigned long reg;

	reg = __raw_readl(MXC_CCM_MCR);

	switch (clk) {
	case CSI_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		clksel = (reg & MXC_CCM_MCR_CSIS_MASK) >>
		    MXC_CCM_MCR_CSIS_OFFSET;
		if (clksel == 0) {
			ret_val = mxc_pll_clock(USBPLL);
		} else if (clksel == 1) {
			ret_val = mxc_pll_clock(MCUPLL);
		} else if (clksel == 2) {
			ret_val = mxc_pll_clock(TURBOPLL);
		} else {
			ret_val = MXC_CKIH_FREQ;
		}
#else
		ret_val = mxc_pll_clock(USBPLL);
#endif
		break;
#ifdef CONFIG_ARCH_MXC91321
	case SDHC1_CLK:
		clksel = (reg & MXC_CCM_MCR_SDHC1S_MASK) >>
		    MXC_CCM_MCR_SDHC1S_OFFSET;
		if (clksel == 0) {
			ret_val = mxc_pll_clock(USBPLL);
		} else if (clksel == 1) {
			ret_val = mxc_pll_clock(MCUPLL);
		} else if (clksel == 2) {
			ret_val = mxc_pll_clock(TURBOPLL);
		} else {
			ret_val = MXC_CKIH_FREQ;
		}
		break;
	case SDHC2_CLK:
		clksel = (reg & MXC_CCM_MCR_SDHC2S_MASK) >>
		    MXC_CCM_MCR_SDHC2S_OFFSET;
		if (clksel == 0) {
			ret_val = mxc_pll_clock(USBPLL);
		} else if (clksel == 1) {
			ret_val = mxc_pll_clock(MCUPLL);
		} else if (clksel == 2) {
			ret_val = mxc_pll_clock(TURBOPLL);
		} else {
			ret_val = MXC_CKIH_FREQ;
		}
		break;
#endif
	case SSI1_BAUD:
		clksel = reg & MXC_CCM_MCR_SSIS1;
		ret_val = mxc_pll_clock(clksel);
		break;
	case SSI2_BAUD:
		clksel = reg & MXC_CCM_MCR_SSIS2;
		ret_val = mxc_pll_clock(clksel);
		break;
	case FIRI_BAUD:
		clksel = reg & MXC_CCM_MCR_FIRS;
		ret_val = mxc_pll_clock(clksel);
		break;
	default:
		break;
	}
	return ret_val;
}

/*!
 * This function returns the active PLL's clock
 *
 * @return      PLL's clock that is currently active on the AP side. 
 *              Possible PLLs are MCUPLL and TURBOPLL
 */
static unsigned long mxc_mcu_active_pll_clk(void)
{
	if ((__raw_readl(MXC_CCM_MPDR0) & (MXC_CCM_MPDR0_TPSEL)) != 0) {
		return mxc_pll_clock(TURBOPLL);
	} else {
		return mxc_pll_clock(MCUPLL);
	}
}

/*!
 * This function returns the main clock values in Hz.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks(enum mxc_clocks clk)
{
	unsigned long pll, ret_val = 0;
	unsigned long brmm, max_pdf, ipg_pdf, nfc_pdf;
	unsigned long prepdf = 0, pdf = 0;
	int tpsel = 0;
	volatile unsigned long reg1 = __raw_readl(MXC_CCM_MPDR0);
	volatile unsigned long reg2 = __raw_readl(MXC_CCM_MPDR1);
#ifdef CONFIG_ARCH_MXC91321
	volatile unsigned long reg3 = __raw_readl(MXC_CCM_MPDR2);
#endif

	max_pdf =
	    (reg1 & MXC_CCM_MPDR0_MAX_PDF_MASK) >> MXC_CCM_MPDR0_MAX_PDF_OFFSET;
	ipg_pdf =
	    (reg1 & MXC_CCM_MPDR0_IPG_PDF_MASK) >> MXC_CCM_MPDR0_IPG_PDF_OFFSET;
	pll = mxc_mcu_active_pll_clk();

	switch (clk) {
	case CPU_CLK:
		brmm = reg1 & MXC_CCM_MPDR0_BRMM_MASK;
		tpsel =
		    (reg1 & MXC_CCM_MPDR0_TPSEL) >> MXC_CCM_MPDR0_TPSEL_OFFSET;
		if (brmm >= 5) {
			printk("Wrong BRMM value in the CRM_AP, MPDR0 reg \n");
			return 0;
		}
		if (mxc_get_clocks(AHB_CLK) == AHB_FREQ_MAX) {
#ifdef CONFIG_MOT_FEAT_PM
                        ret_val = core_pll_op_128[tpsel][brmm];
#else
			ret_val = core_pll_op_133[tpsel][brmm];
#endif
		} else {
			ret_val = core_pll_op_100[tpsel][brmm];
		}
		break;
	case AHB_CLK:
	case IPU_CLK:
		ret_val = pll / (max_pdf + 1);
		break;
	case IPG_CLK:
#ifdef CONFIG_MOT_WFN441
        case SIM1_CLK:
        case SIM2_CLK:
#endif
	case UART1_BAUD:
	case UART2_BAUD:
	case UART3_BAUD:
	case UART4_BAUD:
	case OWIRE_CLK:
	case I2C_CLK:
#ifdef CONFIG_MOT_FEAT_32KHZ_GPT
                ret_val = pll / ((max_pdf + 1) * (ipg_pdf + 1));
                break;
	case GPT_CLK:
                ret_val = MXC_TIMER_CLK; 
#else
        case GPT_CLK:
		ret_val = pll / ((max_pdf + 1) * (ipg_pdf + 1));
#endif /* CONFIG_MOT_FEAT_32KHZ_GPT */
		break;
#ifdef CONFIG_ARCH_MXC91321
	case SDHC1_CLK:
		if (reg3 & MXC_CCM_MPDR2_SDHC1DIS) {
			printk(KERN_WARNING "SDHC1 Baud clock is disabled\n");
		}
		pll = mxc_get_clocks_parent(clk);
		pdf = (reg3 & MXC_CCM_MPDR2_SDHC1_PDF_MASK) >>
		    MXC_CCM_MPDR2_SDHC1_PDF_OFFSET;
		ret_val = pll / (pdf + 1);
		break;
	case SDHC2_CLK:
		if (reg3 & MXC_CCM_MPDR2_SDHC2DIS) {
			printk(KERN_WARNING "SDHC2 Baud clock is disabled\n");
		}
		pll = mxc_get_clocks_parent(clk);
		pdf = (reg3 & MXC_CCM_MPDR2_SDHC2_PDF_MASK) >>
		    MXC_CCM_MPDR2_SDHC2_PDF_OFFSET;
		ret_val = pll / (pdf + 1);
		break;
#else
	case SDHC1_CLK:
	case SDHC2_CLK:
		ret_val = pll / ((max_pdf + 1) * (ipg_pdf + 1));
		break;

#endif
	case NFC_CLK:
		nfc_pdf = (reg1 & MXC_CCM_MPDR0_NFC_PDF_MASK) >>
		    MXC_CCM_MPDR0_NFC_PDF_OFFSET;
		ret_val = pll / ((max_pdf + 1) * (nfc_pdf + 1));
		break;
	case USB_CLK:
#ifdef CONFIG_ARCH_MXC91321
		if (reg2 & MXC_CCM_MPDR1_USB_DIS) {
			printk(KERN_WARNING "USB Baud clock is disabled\n");
		}
#endif
		pdf = (reg2 & MXC_CCM_MPDR1_USB_PDF_MASK) >>
		    MXC_CCM_MPDR1_USB_PDF_OFFSET;
		pll = mxc_pll_clock(USBPLL);
		ret_val = pll / (pdf + 1);
		break;
	case SSI1_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		if (reg2 & MXC_CCM_MPDR1_SSI1_DIS) {
			printk(KERN_WARNING "SSI1 Baud clock is disabled\n");
		}
#endif
		prepdf = (reg2 & MXC_CCM_MPDR1_SSI1_PREPDF_MASK) >>
		    MXC_CCM_MPDR1_SSI1_PREPDF_OFFSET;
		pdf = (reg2 & MXC_CCM_MPDR1_SSI1_PDF_MASK) >>
		    MXC_CCM_MPDR1_SSI1_PDF_OFFSET;
		pll = mxc_get_clocks_parent(clk);
		ret_val = pll / ((prepdf + 1) * (pdf + 1));
		break;
	case SSI2_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		if (reg2 & MXC_CCM_MPDR1_SSI2_DIS) {
			printk(KERN_WARNING "SSI2 Baud clock is disabled\n");
		}
#endif
		prepdf = (reg2 & MXC_CCM_MPDR1_SSI2_PREPDF_MASK) >>
		    MXC_CCM_MPDR1_SSI2_PREPDF_OFFSET;
		pdf = (reg2 & MXC_CCM_MPDR1_SSI2_PDF_MASK) >>
		    MXC_CCM_MPDR1_SSI2_PDF_OFFSET;
		pll = mxc_get_clocks_parent(clk);
		ret_val = pll / ((prepdf + 1) * (pdf + 1));
		break;
	case CSI_BAUD:
		pdf = (reg1 & MXC_CCM_MPDR0_CSI_PDF_MASK) >>
		    MXC_CCM_MPDR0_CSI_PDF_OFFSET;
#ifdef CONFIG_ARCH_MXC91321
		if (reg1 & MXC_CCM_MPDR0_CSI_DIS) {
			printk(KERN_WARNING "CSI Baud clock is disabled\n");
		}
		pll = mxc_get_clocks_parent(clk);
		if (reg1 & MXC_CCM_MPDR0_CSI_PRE) {
			prepdf = 2;
		} else {
			prepdf = 1;
		}
		/* Take care of the fractional part of the post divider */
		if (reg1 & MXC_CCM_MPDR0_CSI_FPDF) {
			ret_val = (2 * pll) / (prepdf * (2 * pdf + 3));
		} else {
			ret_val = pll / (prepdf * (pdf + 1));
		}
#else
		pll = mxc_get_clocks_parent(clk);
		ret_val = pll / (pdf + 1);
#endif
		break;
	case FIRI_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		if (reg2 & MXC_CCM_MPDR1_FIRI_DIS) {
			printk(KERN_WARNING "FIRI Baud clock is disabled\n");
		}
#endif
		prepdf = (reg2 & MXC_CCM_MPDR1_FIRI_PREPDF_MASK) >>
		    MXC_CCM_MPDR1_FIRI_PREPDF_OFFSET;
		pdf = (reg2 & MXC_CCM_MPDR1_FIRI_PDF_MASK) >>
		    MXC_CCM_MPDR1_FIRI_PDF_OFFSET;
		pll = mxc_get_clocks_parent(clk);
		ret_val = pll / ((prepdf + 1) * (pdf + 1));
		break;
	case CKIH_CLK:
		return MXC_CKIH_FREQ;
		break;
	default:
		printk("This clock: %d not supported yet \n", clk);
		break;
	}

	return ret_val;
}

/*!
 * This function sets the PLL source for a clock.
 *
 * @param clk     as defined in enum mxc_clocks
 * @param pll_num the PLL that you wish to use as source for this clock
 */
void mxc_set_clocks_pll(enum mxc_clocks clk, enum plls pll_num)
{
	volatile unsigned long mcr;
	unsigned long flags;

	spin_lock_irqsave(&mxc_crm_lock, flags);
	mcr = __raw_readl(MXC_CCM_MCR);
	switch (clk) {
	case SSI1_BAUD:
		if (pll_num == USBPLL) {
			mcr |= MXC_CCM_MCR_SSIS1;
		} else {
			mcr &= ~MXC_CCM_MCR_SSIS1;
		}
		break;
	case SSI2_BAUD:
		if (pll_num == USBPLL) {
			mcr |= MXC_CCM_MCR_SSIS2;
		} else {
			mcr &= ~MXC_CCM_MCR_SSIS2;
		}
		break;
	case FIRI_BAUD:
		if (pll_num == USBPLL) {
			mcr |= MXC_CCM_MCR_FIRS;
		} else {
			mcr &= ~MXC_CCM_MCR_FIRS;
		}
		break;
#ifdef CONFIG_ARCH_MXC91321
	case CSI_BAUD:
		if (pll_num == USBPLL) {
			mcr &= ~MXC_CCM_MCR_CSIS_MASK;
		} else if (pll_num == MCUPLL) {
			mcr = (mcr & (~MXC_CCM_MCR_CSIS_MASK)) |
			    (0x1 << MXC_CCM_MCR_CSIS_OFFSET);
		} else if (pll_num == TURBOPLL) {
			mcr = (mcr & (~MXC_CCM_MCR_CSIS_MASK)) |
			    (0x2 << MXC_CCM_MCR_CSIS_OFFSET);
		} else {
			mcr |= MXC_CCM_MCR_CSIS_MASK;
		}
		break;
	case SDHC1_CLK:
		if (pll_num == USBPLL) {
			mcr &= ~MXC_CCM_MCR_SDHC1S_MASK;
		} else if (pll_num == MCUPLL) {
			mcr = (mcr & (~MXC_CCM_MCR_SDHC1S_MASK)) |
			    (0x1 << MXC_CCM_MCR_SDHC1S_OFFSET);
		} else if (pll_num == TURBOPLL) {
			mcr = (mcr & (~MXC_CCM_MCR_SDHC1S_MASK)) |
			    (0x2 << MXC_CCM_MCR_SDHC1S_OFFSET);
		} else {
			mcr |= MXC_CCM_MCR_SDHC1S_MASK;
		}
		break;
	case SDHC2_CLK:
		if (pll_num == USBPLL) {
			mcr &= ~MXC_CCM_MCR_SDHC2S_MASK;
		} else if (pll_num == MCUPLL) {
			mcr = (mcr & (~MXC_CCM_MCR_SDHC2S_MASK)) |
			    (0x1 << MXC_CCM_MCR_SDHC2S_OFFSET);
		} else if (pll_num == TURBOPLL) {
			mcr = (mcr & (~MXC_CCM_MCR_SDHC2S_MASK)) |
			    (0x2 << MXC_CCM_MCR_SDHC2S_OFFSET);
		} else {
			mcr |= MXC_CCM_MCR_SDHC2S_MASK;
		}
		break;
#endif
	default:
		printk("This clock does not have ability to choose the PLL\n");
		break;
	}

	__raw_writel(mcr, MXC_CCM_MCR);
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function calculates the pre and post divider values for a clock
 *
 * @param div  divider value passed in
 * @param div1 returns the pre-divider value
 * @param div2 returns the post-divider value
 */
void mxc_clk_getdivs(unsigned int div, unsigned int *div1, unsigned int *div2)
{
	int i;

	if (div <= 8) {
		*div1 = div;
		*div2 = 1;
		return;
	}
	if (div <= 32) {
		*div1 = 1;
		*div2 = div;
		return;
	}
	for (i = 2; i < 9; i++) {
		if ((div % i) == 0) {
			*div1 = i;
			*div2 = div / i;
			return;
		}
	}
	/* Set to default value */
	*div1 = 2;
	*div2 = 12;
	return;
}

/*!
 * This function sets the division factor for a clock.
 *
 * @param clk as defined in enum mxc_clocks
 * @param div the division factor to be used for the clock (For SSI & CSI, pass in
 *            2 times the expected division value to account for FP vals)
 */
void mxc_set_clocks_div(enum mxc_clocks clk, unsigned int div)
{
	volatile unsigned long reg;
	unsigned long flags;
	unsigned int d, div1, div2;
#ifdef CONFIG_ARCH_MXC91321
	unsigned long pll = 0;
	unsigned long csi_ref_freq = 288000000;
	int set_fpdf = 0;
#endif

	spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (clk) {
	case AHB_CLK:
		reg = __raw_readl(MXC_CCM_MPDR0);
		reg = (reg & (~MXC_CCM_MPDR0_MAX_PDF_MASK)) |
		    ((div - 1) << MXC_CCM_MPDR0_MAX_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR0);
		break;
	case IPG_CLK:
		reg = __raw_readl(MXC_CCM_MPDR0);
		reg = (reg & (~MXC_CCM_MPDR0_IPG_PDF_MASK)) |
		    ((div - 1) << MXC_CCM_MPDR0_IPG_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR0);
		break;
	case NFC_CLK:
		reg = __raw_readl(MXC_CCM_MPDR0);
		reg = (reg & (~MXC_CCM_MPDR0_NFC_PDF_MASK)) |
		    ((div - 1) << MXC_CCM_MPDR0_NFC_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR0);
		break;
	case CSI_BAUD:
		reg = __raw_readl(MXC_CCM_MPDR0);
		d = div / 2;
#ifdef CONFIG_ARCH_MXC91321
		if ((div % 2) != 0) {
			set_fpdf = 1;
		}
		/* Find clk source to decide if Pre divider needs to be set */
		pll = mxc_get_clocks_parent(clk);
		/*
		 * Set the pre-divider bit if the freq into the dividers is
		 * greater than 288 MHz
		 */
		if (pll > csi_ref_freq) {
			reg |= MXC_CCM_MPDR0_CSI_PRE;
			if ((d % 2) != 0) {
				set_fpdf = 1;
			}
			d = d / 2;

		} else {
			reg &= ~MXC_CCM_MPDR0_CSI_PRE;
		}
		/* Set the fractional part of the post divider if required */
		if (set_fpdf == 1) {
			reg |= MXC_CCM_MPDR0_CSI_FPDF;
		} else {
			reg &= ~MXC_CCM_MPDR0_CSI_FPDF;
		}
#endif
		reg = (reg & (~MXC_CCM_MPDR0_CSI_PDF_MASK)) |
		    ((d - 1) << MXC_CCM_MPDR0_CSI_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR0);
		break;
	case USB_CLK:
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg = (reg & (~MXC_CCM_MPDR1_USB_PDF_MASK)) |
		    ((div - 1) << MXC_CCM_MPDR1_USB_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR1);
		break;
	case SSI1_BAUD:
		reg = __raw_readl(MXC_CCM_MPDR1);
		d = div / 2;
		mxc_clk_getdivs(d, &div1, &div2);
		reg = (reg & (~MXC_CCM_MPDR1_SSI1_PREPDF_MASK)) |
		    ((div1 - 1) << MXC_CCM_MPDR1_SSI1_PREPDF_OFFSET);
		reg = (reg & (~MXC_CCM_MPDR1_SSI1_PDF_MASK)) |
		    ((div2 - 1) << MXC_CCM_MPDR1_SSI1_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR1);
		break;
	case SSI2_BAUD:
		reg = __raw_readl(MXC_CCM_MPDR1);
		d = div / 2;
		mxc_clk_getdivs(d, &div1, &div2);
		reg = (reg & (~MXC_CCM_MPDR1_SSI2_PREPDF_MASK)) |
		    ((div1 - 1) << MXC_CCM_MPDR1_SSI2_PREPDF_OFFSET);
		reg = (reg & (~MXC_CCM_MPDR1_SSI2_PDF_MASK)) |
		    ((div2 - 1) << MXC_CCM_MPDR1_SSI2_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR1);
		break;
	case FIRI_BAUD:
		reg = __raw_readl(MXC_CCM_MPDR1);
		d = div / 2;
		mxc_clk_getdivs(d, &div1, &div2);
		reg = (reg & (~MXC_CCM_MPDR1_FIRI_PREPDF_MASK)) |
		    ((div1 - 1) << MXC_CCM_MPDR1_FIRI_PREPDF_OFFSET);
		reg = (reg & (~MXC_CCM_MPDR1_FIRI_PDF_MASK)) |
		    ((div2 - 1) << MXC_CCM_MPDR1_FIRI_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR1);
		break;
#ifdef CONFIG_ARCH_MXC91321
	case SDHC1_CLK:
		reg = __raw_readl(MXC_CCM_MPDR2);
		reg = (reg & (~MXC_CCM_MPDR2_SDHC1_PDF_MASK)) |
		    ((div - 1) << MXC_CCM_MPDR2_SDHC1_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR2);
		break;
	case SDHC2_CLK:
		reg = __raw_readl(MXC_CCM_MPDR2);
		reg = (reg & (~MXC_CCM_MPDR2_SDHC2_PDF_MASK)) |
		    ((div - 1) << MXC_CCM_MPDR2_SDHC2_PDF_OFFSET);
		__raw_writel(reg, MXC_CCM_MPDR2);
		break;
#endif
	default:
		break;
	}
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * Configure clock output on CKO1/CKO2 pins
 *
 * @param       output     as defined in enum mxc_clk_out
 * @param       clk        as defined in enum mxc_clocks
 * @param       div        clock output divider value
 *
 */
void mxc_set_clock_output(enum mxc_clk_out output, enum mxc_clocks clk, int div)
{
	unsigned long flags;
	const int div_value[] =
	    { 0, 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };
	u32 reg = __raw_readl(MXC_CCM_COSR);

	spin_lock_irqsave(&mxc_crm_lock, flags);

	/* Select AP CKOH */
	if (output == CKO2) {
		reg &= ~(MXC_CCM_COSR_CKO2S_MASK |
			 MXC_CCM_COSR_CKO2DV_MASK) | MXC_CCM_COSR_CKO2EN;
	} else if (output == CKO1) {
		reg &= ~(MXC_CCM_COSR_CKO1S_MASK |
			 MXC_CCM_COSR_CKO1DV_MASK) | MXC_CCM_COSR_CKO1EN;
	}

	switch (clk) {
	case CPU_CLK:
		/* To select AP clock */
		if (output == CKO2) {
			reg = 0x00036000 |
			    div_value[div] << MXC_CCM_COSR_CKO2DV_OFFSET;
		}
		break;
	case AHB_CLK:
		/* To select AHB clock */
		if (output == CKO2) {
			reg = 0x00026000 |
			    div_value[div] << MXC_CCM_COSR_CKO2DV_OFFSET;
		}
		break;

	case IPG_CLK:
		/* To select IP clock */
		if (output == CKO2) {
			reg = 0x0002E000 |
			    div_value[div] << MXC_CCM_COSR_CKO2DV_OFFSET;
		}
		break;
	default:
		break;
	}

	__raw_writel(reg, MXC_CCM_COSR);
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is called to gate off the individual module clocks
 *
 * @param       clk     as defined in enum mxc_clocks
 */
void mxc_clks_disable(enum mxc_clocks clk)
{
	unsigned long flags;
	volatile unsigned long reg;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (clk) {
	case UART1_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_UART1;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case UART2_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_UART2;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case UART3_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_UART3;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case UART4_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_UART4;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SSI1_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg |= MXC_CCM_MPDR1_SSI1_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_SSI1;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case CSPI1_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_CSPI1;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case FIRI_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg |= MXC_CCM_MPDR1_FIRI_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_FIRI;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case GPT_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_GPTMCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case RTC_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_RTC;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case EPIT1_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_EPIT1MCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case EPIT2_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_EPIT2MCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case EDIO_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_EDIO;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case WDOG_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_WDOGMCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case PWM_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_PWMMCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case I2C_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg &= ~MXC_CCM_MCGR0_I2C;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case CSI_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR0);
		reg |= MXC_CCM_MPDR0_CSI_DIS;
		__raw_writel(reg, MXC_CCM_MPDR0);
#endif
		iomux_config_mux(PIN_IPU_CSI_MCLK, OUTPUTCONFIG_GPIO,
				 INPUTCONFIG_NONE);
		break;
	case IPU_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_IPU;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case MPEG4_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_MPEG4;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SIM1_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_SIM1;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SIM2_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_SIM2;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case HAC_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_HAC;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case GEM_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_GEM;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case USB_CLK:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg |= MXC_CCM_MPDR1_USB_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_USBOTG;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case CSPI2_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_CSPI2;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SDHC2_CLK:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR2);
		reg |= MXC_CCM_MPDR2_SDHC2DIS;
		__raw_writel(reg, MXC_CCM_MPDR2);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_SDHC2;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SDHC1_CLK:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR2);
		reg |= MXC_CCM_MPDR2_SDHC1DIS;
		__raw_writel(reg, MXC_CCM_MPDR2);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_SDHC1;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SSI2_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg |= MXC_CCM_MPDR1_SSI2_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg &= ~MXC_CCM_MCGR1_SSI2;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SDMA_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg &= ~MXC_CCM_MCGR2_SDMA;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case RNG_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg &= ~MXC_CCM_MCGR2_RNG;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case KPP_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg &= ~MXC_CCM_MCGR2_KPP;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case MU_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg &= ~MXC_CCM_MCGR2_MU;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case SPBA_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg &= ~MXC_CCM_MCGR2_SPBA;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
#ifdef CONFIG_MOT_WFN441
        case NFC_CLK:
                break;
#endif
#ifdef CONFIG_MOT_FEAT_PM
        case OWIRE_CLK:
                reg = __raw_readl(MXC_CCM_MCGR0);
                reg &= ~MXC_CCM_MCGR0_OWIRE;
		__raw_writel(reg, MXC_CCM_MCGR0);
                break;
            
        case RTIC_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg &= ~MXC_CCM_MCGR2_RTIC;
		__raw_writel(reg, MXC_CCM_MCGR2);
                break;
        case IIM_CLK:
                reg = __raw_readl(MXC_CCM_MCGR1);
                reg &= ~MXC_CCM_MCGR1_IIM;
		__raw_writel(reg, MXC_CCM_MCGR1);
                break;
        case SMC_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg &= ~MXC_CCM_MCGR2_SMC;
		__raw_writel(reg, MXC_CCM_MCGR2);
                break;
        case ECT_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg &= ~MXC_CCM_MCGR2_ECT;
		__raw_writel(reg, MXC_CCM_MCGR2);
                break;
        case RTRMCU_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg &= ~MXC_CCM_MCGR2_RTRMCU;
		__raw_writel(reg, MXC_CCM_MCGR2);
                break;
        case EMI_CLK:
                reg = __raw_readl(MXC_CCM_MCGR1);
                reg &= ~MXC_CCM_MCGR1_EMI;
		__raw_writel(reg, MXC_CCM_MCGR1);
                break;
        case SAHARA_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg &= ~MXC_CCM_MCGR2_SAHARA;
                __raw_writel(reg, MXC_CCM_MCGR2);
                break;     
#endif
	default:
		printk("The gateoff for this clock(%d) is not implemented\n",
                       clk);
		break;
	}
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is called to enable the individual module clocks
 *
 * @param       clk     as defined in enum mxc_clocks
 */
void mxc_clks_enable(enum mxc_clocks clk)
{
	unsigned long flags;
	volatile unsigned long reg;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (clk) {
	case UART1_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_UART1_MASK) | MXC_CCM_MCGR0_UART1_EN;
#else
		reg |= MXC_CCM_MCGR0_UART1;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case UART2_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_UART2_MASK) | MXC_CCM_MCGR0_UART2_EN;
#else
		reg |= MXC_CCM_MCGR0_UART2;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case UART3_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR1_UART3_MASK) | MXC_CCM_MCGR1_UART3_EN;
#else
		reg |= MXC_CCM_MCGR1_UART3;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case UART4_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR1_UART4_MASK) | MXC_CCM_MCGR1_UART4_EN;
#else
		reg |= MXC_CCM_MCGR1_UART4;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SSI1_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg &= ~MXC_CCM_MPDR1_SSI1_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_SSI1_MASK) | MXC_CCM_MCGR0_SSI1_EN;
#else
		reg |= MXC_CCM_MCGR0_SSI1;
#endif

		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case CSPI1_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_CSPI1_MASK) | MXC_CCM_MCGR0_CSPI1_EN;
#else
		reg |= MXC_CCM_MCGR0_CSPI1;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case FIRI_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg &= ~MXC_CCM_MPDR1_FIRI_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_FIRI_MASK) | MXC_CCM_MCGR0_FIRI_EN;
#else
		reg |= MXC_CCM_MCGR0_FIRI;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case GPT_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_GPTMCU_MASK) | MXC_CCM_MCGR0_GPTMCU_EN;
#else
		reg |= MXC_CCM_MCGR0_GPTMCU;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case RTC_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_RTC_MASK) | MXC_CCM_MCGR0_RTC_EN;
#else
		reg |= MXC_CCM_MCGR0_RTC;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case EPIT1_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_EPIT1MCU_MASK) | MXC_CCM_MCGR0_EPIT1MCU_EN;
#else
		reg |= MXC_CCM_MCGR0_EPIT1MCU;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case EPIT2_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_EPIT2MCU_MASK) | MXC_CCM_MCGR0_EPIT2MCU_EN;
#else
		reg |= MXC_CCM_MCGR0_EPIT2MCU;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case EDIO_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_EDIO_MASK) | MXC_CCM_MCGR0_EDIO_EN;
#else
		reg |= MXC_CCM_MCGR0_EDIO;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case WDOG_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_WDOGMCU_MASK) | MXC_CCM_MCGR0_WDOGMCU_EN;
#else
		reg |= MXC_CCM_MCGR0_WDOGMCU;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case PWM_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_PWMMCU_MASK) | MXC_CCM_MCGR0_PWMMCU_EN;
#else
		reg |= MXC_CCM_MCGR0_PWMMCU;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case I2C_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR0_I2C_MASK) | MXC_CCM_MCGR0_I2C_EN;
#else
		reg |= MXC_CCM_MCGR0_I2C;
#endif
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case CSI_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR0);
		reg &= ~MXC_CCM_MPDR0_CSI_DIS;
		__raw_writel(reg, MXC_CCM_MPDR0);
#endif
		iomux_config_mux(PIN_IPU_CSI_MCLK, OUTPUTCONFIG_FUNC,
				 INPUTCONFIG_NONE);
		break;
	case IPU_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR1_IPU_MASK) | MXC_CCM_MCGR1_IPU_EN;
#else
		reg |= MXC_CCM_MCGR1_IPU;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case MPEG4_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR1_MPEG4_MASK) | MXC_CCM_MCGR1_MPEG4_EN;
#else
		reg |= MXC_CCM_MCGR1_MPEG4;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SIM1_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR1_SIM1_MASK) | MXC_CCM_MCGR1_SIM1_EN;
#else
		reg |= MXC_CCM_MCGR1_SIM1;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SIM2_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR1_SIM2_MASK) | MXC_CCM_MCGR1_SIM2_EN;
#else
		reg |= MXC_CCM_MCGR1_SIM2;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case HAC_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_HAC;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case GEM_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR1_GEM_MASK) | MXC_CCM_MCGR1_GEM_EN;
#else
		reg |= MXC_CCM_MCGR1_GEM;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case USB_CLK:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg &= ~MXC_CCM_MPDR1_USB_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
		reg = (reg & ~MXC_CCM_MCGR1_USBOTG_MASK) | MXC_CCM_MCGR1_USBOTG_EN;
#else
		reg |= MXC_CCM_MCGR1_USBOTG;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case CSPI2_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR1_CSPI2_MASK) | MXC_CCM_MCGR1_CSPI2_EN;
#else
		reg |= MXC_CCM_MCGR1_CSPI2;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SDHC2_CLK:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR2);
		reg &= ~MXC_CCM_MPDR2_SDHC2DIS;
		__raw_writel(reg, MXC_CCM_MPDR2);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR1_SDHC2_MASK) | MXC_CCM_MCGR1_SDHC2_EN;
#else
		reg |= MXC_CCM_MCGR1_SDHC2;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SDHC1_CLK:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR2);
		reg &= ~MXC_CCM_MPDR2_SDHC1DIS;
		__raw_writel(reg, MXC_CCM_MPDR2);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
		reg = (reg & ~MXC_CCM_MCGR1_SDHC1_MASK) | MXC_CCM_MCGR1_SDHC1_EN;
#else
		reg |= MXC_CCM_MCGR1_SDHC1;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SSI2_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg &= ~MXC_CCM_MPDR1_SSI2_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
#ifdef CONFIG_MOT_FEAT_PM
		reg = (reg & ~MXC_CCM_MCGR1_SSI2_MASK) | MXC_CCM_MCGR1_SSI2_EN;
#else
		reg |= MXC_CCM_MCGR1_SSI2;
#endif
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SDMA_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR2_SDMA_MASK) | MXC_CCM_MCGR2_SDMA_EN;
#else
		reg |= MXC_CCM_MCGR2_SDMA;
#endif
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case RNG_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg |= MXC_CCM_MCGR2_RNG;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case KPP_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR2_KPP_MASK) | MXC_CCM_MCGR2_KPP_EN;
#else
		reg |= MXC_CCM_MCGR2_KPP;
#endif
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case MU_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR2_MU_MASK) | MXC_CCM_MCGR2_MU_EN;
#else
		reg |= MXC_CCM_MCGR2_MU;
#endif
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case SPBA_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
#ifdef CONFIG_MOT_FEAT_PM
                reg = (reg & ~MXC_CCM_MCGR2_SPBA_MASK) | MXC_CCM_MCGR2_SPBA_EN;
#else
		reg |= MXC_CCM_MCGR2_SPBA;
#endif
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
#ifdef CONFIG_MOT_WFN441
        case NFC_CLK:
                break;
#endif
#ifdef CONFIG_MOT_FEAT_PM
        case OWIRE_CLK:
                reg = __raw_readl(MXC_CCM_MCGR0);
                reg = (reg & ~MXC_CCM_MCGR0_OWIRE_MASK) | MXC_CCM_MCGR0_OWIRE_EN;
		__raw_writel(reg, MXC_CCM_MCGR0);

                //printk("namin - OWIRE:  reg=0x%08lx\n");
                break;
        case RTIC_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg = (reg & ~MXC_CCM_MCGR2_RTIC_MASK) | MXC_CCM_MCGR2_RTIC_EN;
		__raw_writel(reg, MXC_CCM_MCGR2);
                break;
        case IIM_CLK:
                reg = __raw_readl(MXC_CCM_MCGR1);
                reg = (reg & ~MXC_CCM_MCGR1_IIM_MASK) | MXC_CCM_MCGR1_IIM_EN;
		__raw_writel(reg, MXC_CCM_MCGR1);
                break;
        case SMC_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg = (reg & ~MXC_CCM_MCGR2_SMC_MASK) | MXC_CCM_MCGR2_SMC_EN;
		__raw_writel(reg, MXC_CCM_MCGR2);
                break;
        case ECT_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg = (reg & ~MXC_CCM_MCGR2_ECT_MASK) | MXC_CCM_MCGR2_ECT_EN;
		__raw_writel(reg, MXC_CCM_MCGR2);
                break;
        case RTRMCU_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg = (reg & ~MXC_CCM_MCGR2_RTRMCU_MASK) | MXC_CCM_MCGR2_RTRMCU_EN;
		__raw_writel(reg, MXC_CCM_MCGR2);
                break;
        case EMI_CLK:
                reg = __raw_readl(MXC_CCM_MCGR1);
                reg = (reg & ~MXC_CCM_MCGR1_EMI_MASK) | MXC_CCM_MCGR1_EMI_EN;
		__raw_writel(reg, MXC_CCM_MCGR1);
                break;
        case SAHARA_CLK:
                reg = __raw_readl(MXC_CCM_MCGR2);
                reg = (reg & ~MXC_CCM_MCGR2_SAHARA_MASK) | MXC_CCM_MCGR2_SAHARA_EN;
                __raw_writel(reg, MXC_CCM_MCGR2);
                break;
#endif
	default:
		printk("The gateon for this clock(%d) is not implemented\n",
                       clk);
		break;
	}
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is called to read the contents of a CCM_MCU register
 *
 * @param reg_offset the CCM_MCU register that will read
 *
 * @return the register contents
 */
volatile unsigned long mxc_ccm_get_reg(unsigned int reg_offset)
{
	volatile unsigned long reg;

	reg = __raw_readl(reg_offset);
	return reg;
}

/*!
 * This function is called to modify the contents of a CCM_MCU register
 *
 * @param reg_offset the CCM_MCU register that will read
 * @param mask       the mask to be used to clear the bits that are to be modified
 * @param data       the data that should be written to the register
 */
void mxc_ccm_modify_reg(unsigned int reg_offset, unsigned int mask,
			unsigned int data)
{
	unsigned long flags;
	volatile unsigned long reg;

	spin_lock_irqsave(&mxc_crm_lock, flags);
	reg = __raw_readl(reg_offset);
	reg = (reg & (~mask)) | data;
	__raw_writel(reg, reg_offset);
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*
 * Why these functions always return success.
 * Argon's clock tree is very different from that of SCMA11. 
 * Because of these differences it does not make sense to lock and release PLLs 
 * to prevent them from being turned off. Despite this these APIs have been 
 * implemented as stubs for Argon in order to maintain a compatible PM API with SCMA11
 *
 */

#ifdef CONFIG_MOT_FEAT_PM
/*!
 * This function requests that a pll be locked and available.
 *
 * @param       pllnum as defined in enum plls
 *
 * @return      0 on success, -1 on invalid PLL number
 */
int mxc_pll_request_pll(enum plls pllnum)
{
        return 0;
}

/*!
 * This function releases a previous request for a pll
 *
 * @param       pllnum as defined in enum plls
 *
 * @return      0 on success, -1 on invalid PLL number, -2 if already unlocked
 */
int mxc_pll_release_pll(enum plls pllnum)
{
        return 0;
}
#endif /* CONFIG_MOT_FEAT_PM */

EXPORT_SYMBOL(mxc_pll_clock);
EXPORT_SYMBOL(mxc_get_clocks_parent);
EXPORT_SYMBOL(mxc_get_clocks);
EXPORT_SYMBOL(mxc_set_clocks_pll);
EXPORT_SYMBOL(mxc_set_clocks_div);
EXPORT_SYMBOL(mxc_clks_disable);
EXPORT_SYMBOL(mxc_clks_enable);
EXPORT_SYMBOL(mxc_ccm_get_reg);
EXPORT_SYMBOL(mxc_ccm_modify_reg);
EXPORT_SYMBOL(mxc_set_clock_output);
#ifdef CONFIG_MOT_FEAT_PM
EXPORT_SYMBOL(mxc_pll_request_pll);
EXPORT_SYMBOL(mxc_pll_release_pll);
#endif /* CONFIG_MOT_FEAT_PM */
