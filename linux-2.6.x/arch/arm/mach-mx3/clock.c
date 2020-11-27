/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
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

#define CKIH_CLK_FREQ                           26000000
#define CKIL_CLK_FREQ                           32768

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
	signed long mfi, mfn, mfd, pdf, ref_clk, pll_out;
	volatile unsigned long reg, ccmr;
	unsigned int prcs;

	ccmr = __raw_readl(MXC_CCM_CCMR);
	prcs = (ccmr & MXC_CCM_CCMR_PRCS_MASK) >> MXC_CCM_CCMR_PRCS_OFFSET;
	if (prcs == 0x1) {
		ref_clk = CKIL_CLK_FREQ * 1024;
	} else {
		ref_clk = CKIH_CLK_FREQ;
	}

	if (pll == MCUPLL) {
		if ((ccmr & MXC_CCM_CCMR_MPE) == 0) {
			return ref_clk;
		}
		if ((ccmr & MXC_CCM_CCMR_MDS) != 0) {
			return ref_clk;
		}
		reg = __raw_readl(MXC_CCM_MPCTL);
	} else if (pll == USBPLL) {
		reg = __raw_readl(MXC_CCM_UPCTL);
	} else if (pll == SERIALPLL) {
		reg = __raw_readl(MXC_CCM_SRPCTL);
	}
	pdf =
	    (signed long)((reg & MXC_CCM_PCTL_PD_MASK) >>
			  MXC_CCM_PCTL_PD_OFFSET);
	mfd =
	    (signed long)((reg & MXC_CCM_PCTL_MFD_MASK) >>
			  MXC_CCM_PCTL_MFD_OFFSET);

	mfi = (signed long)((reg & MXC_CCM_PCTL_MFI_MASK) >>
			    MXC_CCM_PCTL_MFI_OFFSET);
	mfi = (mfi <= 5) ? 5 : mfi;
	mfn = (signed long)(reg & MXC_CCM_PCTL_MFN_MASK);
	mfn = (mfn < 0x200) ? mfn : (mfn - 0x400);

	pll_out = (2 * ref_clk * mfi + ((2 * ref_clk / (mfd + 1)) * mfn)) /
	    (pdf + 1);
	return pll_out;
}

/*!
 * This function returns the mcu main clock frequency
 *
 * @return      mcu main clock value in Hz.
 */
static unsigned long mxc_mcu_main_clock(void)
{
	volatile unsigned long pmcr0 = __raw_readl(MXC_CCM_PMCR0);
	unsigned long dfsup1 = pmcr0 & MXC_CCM_PMCR0_DFSUP1;

	if (dfsup1 == 0) {
		return mxc_pll_clock(SERIALPLL);
	}
	return mxc_pll_clock(MCUPLL);
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
	unsigned long pll, ret_val = 0, hclk;
	unsigned long mcu_pdf, max_pdf, ipg_pdf, nfc_pdf, hsp_pdf, csi_pdf,
	    usb_pdf;
	unsigned long ssi1_prepdf, ssi1_pdf, per_pdf;
	unsigned long ssi2_prepdf, ssi2_pdf, firi_prepdf, firi_pdf, usb_prepdf;
	unsigned long msti_pdf;
	volatile unsigned long ccmr = __raw_readl(MXC_CCM_CCMR);
	volatile unsigned long pdr0 = __raw_readl(MXC_CCM_PDR0);
	volatile unsigned long pdr1 = __raw_readl(MXC_CCM_PDR1);
	volatile unsigned long pdr2 = __raw_readl(MXC_CCM_PDR2);

	max_pdf =
	    (pdr0 & MXC_CCM_PDR0_MAX_PODF_MASK) >> MXC_CCM_PDR0_MAX_PODF_OFFSET;
	ipg_pdf =
	    (pdr0 & MXC_CCM_PDR0_IPG_PODF_MASK) >> MXC_CCM_PDR0_IPG_PODF_OFFSET;
	per_pdf =
	    (pdr0 & MXC_CCM_PDR0_PER_PODF_MASK) >> MXC_CCM_PDR0_PER_PODF_OFFSET;

	switch (clk) {
	case CKIL_CLK:
		ret_val = CKIL_CLK_FREQ;
		break;
	case CKIH_CLK:
		ret_val = CKIH_CLK_FREQ;
		break;
	case CPU_CLK:
		pll = mxc_mcu_main_clock();
		mcu_pdf = pdr0 & MXC_CCM_PDR0_MCU_PODF_MASK;
		ret_val = pll / (mcu_pdf + 1);
		break;
	case AHB_CLK:
		pll = mxc_mcu_main_clock();
		ret_val = pll / (max_pdf + 1);
		break;
	case IPG_CLK:
		pll = mxc_mcu_main_clock();
		hclk = pll / (max_pdf + 1);
		ret_val = hclk / (ipg_pdf + 1);
		break;
	case NFC_CLK:
		pll = mxc_mcu_main_clock();
		hclk = pll / (max_pdf + 1);
		nfc_pdf = (pdr0 & MXC_CCM_PDR0_NFC_PODF_MASK) >>
		    MXC_CCM_PDR0_NFC_PODF_OFFSET;
		ret_val = hclk / (nfc_pdf + 1);
		break;
	case IPU_CLK:
		pll = mxc_mcu_main_clock();
		hsp_pdf = (pdr0 & MXC_CCM_PDR0_HSP_PODF_MASK) >>
		    MXC_CCM_PDR0_HSP_PODF_OFFSET;
		ret_val = pll / (hsp_pdf + 1);
		break;
	case USB_CLK:
		pll = mxc_pll_clock(USBPLL);
		usb_pdf = (pdr1 & MXC_CCM_PDR1_USB_PODF_MASK) >>
		    MXC_CCM_PDR1_USB_PODF_OFFSET;
		usb_prepdf = (pdr1 & MXC_CCM_PDR1_USB_PRDF_MASK) >>
		    MXC_CCM_PDR1_USB_PRDF_OFFSET;
		ret_val = pll / ((usb_prepdf + 1) * (usb_pdf + 1));
		break;
	case CSI_BAUD:
		if ((ccmr & MXC_CCM_CCMR_CSCS) == 0) {
			pll = mxc_pll_clock(USBPLL);
		} else {
			pll = mxc_pll_clock(SERIALPLL);
		}
		csi_pdf = (pdr0 & MXC_CCM_PDR0_CSI_PODF_MASK) >>
		    MXC_CCM_PDR0_CSI_PODF_OFFSET;
		ret_val = pll / (csi_pdf + 1);
		break;
	case UART1_BAUD:
	case UART2_BAUD:
	case UART3_BAUD:
	case UART4_BAUD:
	case UART5_BAUD:
	case I2C_CLK:
	case I2C2_CLK:
	case I2C3_CLK:
	case SDHC1_CLK:
	case SDHC2_CLK:
		if ((ccmr & MXC_CCM_CCMR_PERCS) == 0) {
			pll = mxc_pll_clock(USBPLL);
			ret_val = pll / (per_pdf + 1);
		} else {
			pll = mxc_mcu_main_clock();
			hclk = pll / (max_pdf + 1);
			ret_val = hclk / (ipg_pdf + 1);

		}
		break;
	case SSI1_BAUD:
		pll = mxc_pll_clock((ccmr & MXC_CCM_CCMR_SSI1S_MASK) >>
				    MXC_CCM_CCMR_SSI1S_OFFSET);
		ssi1_pdf = (pdr1 & MXC_CCM_PDR1_SSI1_PODF_MASK) >>
		    MXC_CCM_PDR1_SSI1_PODF_OFFSET;
		ssi1_prepdf = (pdr1 & MXC_CCM_PDR1_SSI1_PRE_PODF_MASK) >>
		    MXC_CCM_PDR1_SSI1_PRE_PODF_OFFSET;
		ret_val = pll / ((ssi1_prepdf + 1) * (ssi1_pdf + 1));
		break;
	case SSI2_BAUD:
		pll = mxc_pll_clock((ccmr & MXC_CCM_CCMR_SSI2S_MASK) >>
				    MXC_CCM_CCMR_SSI2S_OFFSET);
		ssi2_pdf = (pdr1 & MXC_CCM_PDR1_SSI2_PODF_MASK) >>
		    MXC_CCM_PDR1_SSI2_PODF_OFFSET;
		ssi2_prepdf = (pdr1 & MXC_CCM_PDR1_SSI2_PRE_PODF_MASK) >>
		    MXC_CCM_PDR1_SSI2_PRE_PODF_OFFSET;
		ret_val = pll / ((ssi2_prepdf + 1) * (ssi2_pdf + 1));
		break;
	case FIRI_BAUD:
		pll = mxc_pll_clock((ccmr & MXC_CCM_CCMR_FIRS_MASK) >>
				    MXC_CCM_CCMR_FIRS_OFFSET);
		firi_pdf = (pdr1 & MXC_CCM_PDR1_FIRI_PODF_MASK) >>
		    MXC_CCM_PDR1_FIRI_PODF_OFFSET;
		firi_prepdf = (pdr1 & MXC_CCM_PDR1_FIRI_PRE_PODF_MASK) >>
		    MXC_CCM_PDR1_FIRI_PRE_PODF_OFFSET;
		ret_val = pll / ((firi_prepdf + 1) * (firi_pdf + 1));
		break;
	case MBX_CLK:
		pll = mxc_mcu_main_clock();
		ret_val = pll / (max_pdf + 1);
		ret_val = ret_val / 2;
		break;
	case MSTICK1_BAUD:
		pll = mxc_pll_clock(USBPLL);
		msti_pdf = (pdr2 & MXC_CCM_PDR2_MST1_PDF_MASK) >>
		    MXC_CCM_PDR2_MST1_PDF_OFFSET;
		ret_val = pll / (msti_pdf + 1);
		break;
	case MSTICK2_BAUD:
		pll = mxc_pll_clock(USBPLL);
		msti_pdf = (pdr2 & MXC_CCM_PDR2_MST2_PDF_MASK) >>
		    MXC_CCM_PDR2_MST2_PDF_OFFSET;
		ret_val = pll / (msti_pdf + 1);
		break;
	case GPT_CLK:
		ret_val = 66500000 / MXC_TIMER_DIVIDER;
		break;
	default:
		ret_val = 66516666;
		break;
	}
	return ret_val;
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
	unsigned long ret_val = 0;
	volatile unsigned long ccmr = __raw_readl(MXC_CCM_CCMR);

	switch (clk) {
	case CSI_BAUD:
		if ((ccmr & MXC_CCM_CCMR_CSCS) == 0) {
			ret_val = mxc_pll_clock(USBPLL);
		} else {
			ret_val = mxc_pll_clock(SERIALPLL);
		}
		break;
	default:
		break;
	}
	return ret_val;
}

/*!
 * This function calculates the pre and post divider values for a clock
 *
 * @param div  divider value passed in
 * @param div1 returns the pre-divider value
 * @param div2 returns the post-divider value
 * @param lim1 limit of divider 1
 * @param lim2 limit of divider 2
 */
void mxc_clk_getdivs(unsigned int div, unsigned int *div1,
		     unsigned int *div2, int lim1, int lim2)
{
	int i;

	if (div <= lim1) {
		*div1 = div;
		*div2 = 1;
		return;
	}
	if (div <= lim2) {
		*div1 = 1;
		*div2 = div;
		return;
	}
	for (i = 2; i < (lim1 + 1); i++) {
		if ((div % i) == 0) {
			*div1 = i;
			*div2 = div / i;
			return;
		}
	}

	*div1 = 1;
	*div2 = lim2;
	return;
}

/*!
 * This function sets the PLL source for a clock.
 *
 * @param clk     as defined in enum mxc_clocks
 * @param pll_num the PLL that you wish to use as source for this clock
 */
void mxc_set_clocks_pll(enum mxc_clocks clk, enum plls pll_num)
{
	volatile unsigned long ccmr;
	unsigned long flags;

	spin_lock_irqsave(&mxc_crm_lock, flags);
	ccmr = __raw_readl(MXC_CCM_CCMR);

	switch (clk) {
	case CPU_CLK:
		if (pll_num == MCUPLL) {
			ccmr &= ~MXC_CCM_CCMR_MDS;
		} else {
			ccmr |= MXC_CCM_CCMR_MDS;
		}
		break;
	case CSI_BAUD:
		if (pll_num == USBPLL) {
			ccmr &= ~MXC_CCM_CCMR_CSCS;
		} else {
			ccmr |= MXC_CCM_CCMR_CSCS;
		}
		break;
	case SSI1_BAUD:
		ccmr = (ccmr & (~MXC_CCM_CCMR_SSI1S_MASK)) |
		    (pll_num << MXC_CCM_CCMR_SSI1S_OFFSET);
		break;
	case SSI2_BAUD:
		ccmr = (ccmr & (~MXC_CCM_CCMR_SSI2S_MASK)) |
		    (pll_num << MXC_CCM_CCMR_SSI2S_OFFSET);
		break;
	case FIRI_BAUD:
		ccmr = (ccmr & (~MXC_CCM_CCMR_FIRS_MASK)) |
		    (pll_num << MXC_CCM_CCMR_FIRS_OFFSET);
		break;
	default:
		printk
		    ("This clock does not have ability to choose its clock source\n");
		break;
	}
	__raw_writel(ccmr, MXC_CCM_CCMR);
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
	return;
}

/*!
 * This function sets the divider value for a clock.
 *
 * @param clk as defined in enum mxc_clocks
 * @param div the division factor to be used for the clock (For SSI & CSI, pass
 *            in 2 times the expected division value to account for FP vals on certain
 *            platforms)
 */
void mxc_set_clocks_div(enum mxc_clocks clk, unsigned int div)
{
	volatile unsigned long reg;
	unsigned long flags;
	unsigned int d = 0, div1 = 0, div2 = 0;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (clk) {
	case AHB_CLK:
		reg = __raw_readl(MXC_CCM_PDR0);
		reg = (reg & (~MXC_CCM_PDR0_MAX_PODF_MASK)) |
		    ((div - 1) << MXC_CCM_PDR0_MAX_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR0);
		break;
	case CPU_CLK:
		reg = __raw_readl(MXC_CCM_PDR0);
		reg = (reg & (~MXC_CCM_PDR0_MCU_PODF_MASK)) |
		    ((div - 1) << MXC_CCM_PDR0_MCU_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR0);
		break;
	case IPG_CLK:
		reg = __raw_readl(MXC_CCM_PDR0);
		reg = (reg & (~MXC_CCM_PDR0_IPG_PODF_MASK)) |
		    ((div - 1) << MXC_CCM_PDR0_IPG_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR0);
		break;
	case NFC_CLK:
		reg = __raw_readl(MXC_CCM_PDR0);
		reg = (reg & (~MXC_CCM_PDR0_NFC_PODF_MASK)) |
		    ((div - 1) << MXC_CCM_PDR0_NFC_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR0);
		break;
	case CSI_BAUD:
		reg = __raw_readl(MXC_CCM_PDR0);
		d = div / 2;
		mxc_clk_getdivs(d, &div1, &div2, 8, 64);
		reg = (reg & (~MXC_CCM_PDR0_CSI_PRDF_MASK)) |
		    ((div1 - 1) << MXC_CCM_PDR0_CSI_PRDF_OFFSET);
		reg = (reg & (~MXC_CCM_PDR0_CSI_PODF_MASK)) |
		    ((div2 - 1) << MXC_CCM_PDR0_CSI_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR0);
		break;
	case IPU_CLK:
		reg = __raw_readl(MXC_CCM_PDR0);
		reg = (reg & (~MXC_CCM_PDR0_HSP_PODF_MASK)) |
		    ((div - 1) << MXC_CCM_PDR0_HSP_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR0);
		break;
	case UART1_BAUD:
	case UART2_BAUD:
	case UART3_BAUD:
	case UART4_BAUD:
	case UART5_BAUD:
	case I2C_CLK:
	case I2C2_CLK:
	case I2C3_CLK:
		reg = __raw_readl(MXC_CCM_PDR0);
		reg = (reg & (~MXC_CCM_PDR0_PER_PODF_MASK)) |
		    ((div - 1) << MXC_CCM_PDR0_PER_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR0);
		break;
	case SSI1_BAUD:
		reg = __raw_readl(MXC_CCM_PDR1);
		d = div / 2;
		mxc_clk_getdivs(d, &div1, &div2, 8, 64);
		reg = (reg & (~MXC_CCM_PDR1_SSI1_PRE_PODF_MASK)) |
		    ((div1 - 1) << MXC_CCM_PDR1_SSI1_PRE_PODF_OFFSET);
		reg = (reg & (~MXC_CCM_PDR1_SSI1_PODF_MASK)) |
		    ((div2 - 1) << MXC_CCM_PDR1_SSI1_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR1);
		break;
	case SSI2_BAUD:
		reg = __raw_readl(MXC_CCM_PDR1);
		d = div / 2;
		mxc_clk_getdivs(d, &div1, &div2, 8, 64);
		reg = (reg & (~MXC_CCM_PDR1_SSI2_PRE_PODF_MASK)) |
		    ((div1 - 1) << MXC_CCM_PDR1_SSI2_PRE_PODF_OFFSET);
		reg = (reg & (~MXC_CCM_PDR1_SSI2_PODF_MASK)) |
		    ((div2 - 1) << MXC_CCM_PDR1_SSI2_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR1);
		break;
	case FIRI_BAUD:
		reg = __raw_readl(MXC_CCM_PDR1);
		mxc_clk_getdivs(d, &div1, &div2, 8, 64);
		reg = (reg & (~MXC_CCM_PDR1_FIRI_PRE_PODF_MASK)) |
		    ((div1 - 1) << MXC_CCM_PDR1_FIRI_PRE_PODF_OFFSET);
		reg = (reg & (~MXC_CCM_PDR1_FIRI_PODF_MASK)) |
		    ((div2 - 1) << MXC_CCM_PDR1_FIRI_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR1);
		break;
	case USB_CLK:
		reg = __raw_readl(MXC_CCM_PDR1);
		mxc_clk_getdivs(d, &div1, &div2, 4, 8);
		reg = (reg & (~MXC_CCM_PDR1_USB_PRDF_MASK)) |
		    ((div1 - 1) << MXC_CCM_PDR1_USB_PRDF_OFFSET);
		reg = (reg & (~MXC_CCM_PDR1_USB_PODF_MASK)) |
		    ((div2 - 1) << MXC_CCM_PDR1_USB_PODF_OFFSET);
		__raw_writel(reg, MXC_CCM_PDR1);
		break;
	default:
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
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_UART1;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case UART2_BAUD:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_UART2;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case UART3_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_UART3;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case UART4_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_UART4;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case UART5_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_UART5;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case I2C_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_I2C1;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case I2C2_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_I2C2;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case I2C3_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_I2C3;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case SSI1_BAUD:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_SSI1;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case SSI2_BAUD:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg |= MXC_CCM_CGR2_SSI2;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case USB_CLK:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_USBOTG;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case FIRI_BAUD:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg |= MXC_CCM_CGR2_FIRI;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case CSI_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_CSI;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case IPU_CLK:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_IPU;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case KPP_CLK:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_KPP;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case SDHC1_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_SD_MMC1;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case SDHC2_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_SD_MMC2;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case CSPI1_CLK:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg |= MXC_CCM_CGR2_CSPI1;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case CSPI2_CLK:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg |= MXC_CCM_CGR2_CSPI2;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case CSPI3_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_CSPI3;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case ATA_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_ATA;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case MBX_CLK:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg |= MXC_CCM_CGR2_GACC;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case RTIC_CLK:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg |= MXC_CCM_CGR2_RTIC;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case RNG_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_RNG;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case SDMA_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg |= MXC_CCM_CGR0_SDMA;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case MPEG4_CLK:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_HANTRO;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case MSTICK1_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_MEMSTICK1;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case MSTICK2_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg |= MXC_CCM_CGR1_MEMSTICK2;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is called to disable the individual module clocks
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
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_UART1;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case UART2_BAUD:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_UART2;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case UART3_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_UART3;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case UART4_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_UART4;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case UART5_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_UART5;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case I2C_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_I2C1;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case I2C2_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_I2C2;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case I2C3_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_I2C3;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case SSI1_BAUD:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_SSI1;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case SSI2_BAUD:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg &= ~MXC_CCM_CGR2_SSI2;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case USB_CLK:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_USBOTG;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case FIRI_BAUD:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg &= ~MXC_CCM_CGR2_FIRI;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case CSI_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_CSI;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case IPU_CLK:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_IPU;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case KPP_CLK:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_KPP;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case SDHC1_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_SD_MMC1;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case SDHC2_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_SD_MMC2;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case CSPI1_CLK:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg &= ~MXC_CCM_CGR2_CSPI1;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case CSPI2_CLK:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg &= ~MXC_CCM_CGR2_CSPI2;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case CSPI3_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_CSPI3;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case ATA_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_ATA;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case MBX_CLK:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg &= ~MXC_CCM_CGR2_GACC;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case RTIC_CLK:
		reg = __raw_readl(MXC_CCM_CGR2);
		reg &= ~MXC_CCM_CGR2_RTIC;
		__raw_writel(reg, MXC_CCM_CGR2);
		break;
	case RNG_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_RNG;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case SDMA_CLK:
		reg = __raw_readl(MXC_CCM_CGR0);
		reg &= ~MXC_CCM_CGR0_SDMA;
		__raw_writel(reg, MXC_CCM_CGR0);
		break;
	case MPEG4_CLK:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_HANTRO;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case MSTICK1_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_MEMSTICK1;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	case MSTICK2_BAUD:
		reg = __raw_readl(MXC_CCM_CGR1);
		reg &= ~MXC_CCM_CGR1_MEMSTICK2;
		__raw_writel(reg, MXC_CCM_CGR1);
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is called to read the contents of a CCM register
 *
 * @param reg_offset the CCM register that will read
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
 * This function is called to modify the contents of a CCM register
 *
 * @param reg_offset the CCM register that will read
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

/*!
 * This function is used to modify PLL registers to generate the required
 * frequency.
 *
 * @param  pll_num  the PLL that you wish to modify
 * @param  mfi      multiplication factor integer part
 * @param  pdf      pre-division factor
 * @param  mfd      multiplication factor denominator
 * @param  mfn      multiplication factor numerator
 */
void mxc_pll_set(enum plls pll_num, unsigned int mfi, unsigned int pdf,
		 unsigned int mfd, unsigned int mfn)
{
	volatile unsigned long ccmr;
	unsigned long flags;
	unsigned long new_pll = 0;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	if (pll_num == MCUPLL) {
		/* Swap to reference clock and disable PLL */
		ccmr = __raw_readl(MXC_CCM_CCMR);
		ccmr |= MXC_CCM_CCMR_MDS;
		ccmr &= ~MXC_CCM_CCMR_MPE;
		__raw_writel(ccmr, MXC_CCM_CCMR);
	}

	/* Change the Pll value */
	new_pll = (mfi << MXC_CCM_PCTL_MFI_OFFSET) |
	    (mfn << MXC_CCM_PCTL_MFN_OFFSET) |
	    (mfd << MXC_CCM_PCTL_MFD_OFFSET) | (pdf << MXC_CCM_PCTL_PD_OFFSET);

	if (pll_num == MCUPLL) {
		__raw_writel(new_pll, MXC_CCM_MPCTL);
	} else if (pll_num == USBPLL) {
		__raw_writel(new_pll, MXC_CCM_UPCTL);
	} else if (pll_num == SERIALPLL) {
		__raw_writel(new_pll, MXC_CCM_SRPCTL);
	}

	if (pll_num == MCUPLL) {
		/* Swap to the new value */
		ccmr = __raw_readl(MXC_CCM_CCMR);
		ccmr |= MXC_CCM_CCMR_MPE;
		ccmr &= ~MXC_CCM_CCMR_MDS;
		__raw_writel(ccmr, MXC_CCM_CCMR);
	}

	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * Configure clock output on CKO1/CKO2 pins
 *
 * @param   opt     The desired clock needed to measure. Possible
 *                  values are, CKOH_AP_SEL, CKOH_AHB_SEL or CKOH_IP_SEL
 *
 */
void mxc_set_clock_output(enum mxc_clk_out output, enum mxc_clocks clk, int div)
{
	/* TODO */
}

EXPORT_SYMBOL(mxc_pll_set);
EXPORT_SYMBOL(mxc_pll_clock);
EXPORT_SYMBOL(mxc_get_clocks);
EXPORT_SYMBOL(mxc_get_clocks_parent);
EXPORT_SYMBOL(mxc_set_clocks_pll);
EXPORT_SYMBOL(mxc_set_clocks_div);
EXPORT_SYMBOL(mxc_clks_disable);
EXPORT_SYMBOL(mxc_clks_enable);
EXPORT_SYMBOL(mxc_ccm_get_reg);
EXPORT_SYMBOL(mxc_ccm_modify_reg);
EXPORT_SYMBOL(mxc_set_clock_output);
