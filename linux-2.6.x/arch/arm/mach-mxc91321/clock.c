/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2006 Motorola
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
	unsigned long pll_out;
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
		temp = 2 * ref_clk * mfn;
		do_div(temp, mfd + 1);
		temp = 2 * ref_clk * mfi + temp;
		do_div(temp, pdf + 1);
	} else {
		temp = 2 * ref_clk * (2048 - mfn);
		do_div(temp, mfd + 1);
		temp = 2 * ref_clk * mfi - temp;
		do_div(temp, pdf + 1);
	}

	pll_out = temp;

	return pll_out;
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
 * This function returns the main clock values in Hz.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks(enum mxc_clocks clk)
{
	unsigned long pll, ret_val = 0, hclk;
	unsigned long brmm, max_pdf, ipg_pdf, nfc_pdf;
	unsigned long prepdf = 0, pdf = 0;
	volatile unsigned long reg1 = __raw_readl(MXC_CCM_MPDR0);
	volatile unsigned long reg2 = __raw_readl(MXC_CCM_MPDR1);
#ifdef CONFIG_ARCH_MXC91321
	volatile unsigned long reg3 = __raw_readl(MXC_CCM_MPDR2);
#endif

	max_pdf =
	    (reg1 & MXC_CCM_MPDR0_MAX_PDF_MASK) >> MXC_CCM_MPDR0_MAX_PDF_OFFSET;
	ipg_pdf =
	    (reg1 & MXC_CCM_MPDR0_IPG_PDF_MASK) >> MXC_CCM_MPDR0_IPG_PDF_OFFSET;

	switch (clk) {
	case CPU_CLK:
		brmm = reg1 & MXC_CCM_MPDR0_BRMM_MASK;
		pll = mxc_pll_clock(MCUPLL);
		if (brmm >= 5) {
			printk("Wrong BRMM value in the CRM_AP, MPDR0 reg \n");
			return 0;
		}
		hclk = pll / (max_pdf + 1);
		switch (brmm) {
		case 0:
			ret_val = pll;
			break;
		case 1:
			ret_val = pll + (pll - hclk) / 4;
			break;
		case 2:
			ret_val = pll + (pll - hclk) / 2;
			break;
		case 3:
			ret_val = pll + (pll - hclk) * 3 / 4;
			break;
		case 4:
			ret_val = hclk;
			break;
		default:
			break;
		}
		break;
	case AHB_CLK:
	case IPU_CLK:
		pll = mxc_pll_clock(MCUPLL);
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
	case I2C_CLK:
	case GPT_CLK:
		pll = mxc_pll_clock(MCUPLL);
		ret_val = pll / ((max_pdf + 1) * (ipg_pdf + 1));
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
		pll = mxc_pll_clock(MCUPLL);
		ret_val = pll / ((max_pdf + 1) * (ipg_pdf + 1));
		break;

#endif
	case NFC_CLK:
		nfc_pdf = (reg1 & MXC_CCM_MPDR0_NFC_PDF_MASK) >>
		    MXC_CCM_MPDR0_NFC_PDF_OFFSET;
		pll = mxc_pll_clock(MCUPLL);
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
 * @param   opt     The desired clock needed to measure. Possible
 *                  values are, CKOH_AP_SEL, CKOH_AHB_SEL or CKOH_IP_SEL
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
		reg |= MXC_CCM_MCGR0_UART1;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case UART2_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_UART2;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case UART3_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_UART3;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case UART4_BAUD:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_UART4;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SSI1_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg &= ~MXC_CCM_MPDR1_SSI1_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_SSI1;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case CSPI1_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_CSPI1;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case FIRI_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg &= ~MXC_CCM_MPDR1_FIRI_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_FIRI;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case GPT_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_GPTMCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case RTC_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_RTC;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case EPIT1_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_EPIT1MCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case EPIT2_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_EPIT2MCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case EDIO_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_EDIO;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case WDOG_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_WDOGMCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case PWM_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_PWMMCU;
		__raw_writel(reg, MXC_CCM_MCGR0);
		break;
	case I2C_CLK:
		reg = __raw_readl(MXC_CCM_MCGR0);
		reg |= MXC_CCM_MCGR0_I2C;
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
		reg |= MXC_CCM_MCGR1_IPU;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case MPEG4_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_MPEG4;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SIM1_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_SIM1;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SIM2_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_SIM2;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case HAC_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_HAC;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case GEM_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_GEM;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case USB_CLK:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg &= ~MXC_CCM_MPDR1_USB_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_USBOTG;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case CSPI2_CLK:
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_CSPI2;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SDHC2_CLK:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR2);
		reg &= ~MXC_CCM_MPDR2_SDHC2DIS;
		__raw_writel(reg, MXC_CCM_MPDR2);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_SDHC2;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SDHC1_CLK:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR2);
		reg &= ~MXC_CCM_MPDR2_SDHC1DIS;
		__raw_writel(reg, MXC_CCM_MPDR2);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_SDHC1;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SSI2_BAUD:
#ifdef CONFIG_ARCH_MXC91321
		reg = __raw_readl(MXC_CCM_MPDR1);
		reg &= ~MXC_CCM_MPDR1_SSI2_DIS;
		__raw_writel(reg, MXC_CCM_MPDR1);
#endif
		reg = __raw_readl(MXC_CCM_MCGR1);
		reg |= MXC_CCM_MCGR1_SSI2;
		__raw_writel(reg, MXC_CCM_MCGR1);
		break;
	case SDMA_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg |= MXC_CCM_MCGR2_SDMA;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case RNG_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg |= MXC_CCM_MCGR2_RNG;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case KPP_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg |= MXC_CCM_MCGR2_KPP;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case MU_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg |= MXC_CCM_MCGR2_MU;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
	case SPBA_CLK:
		reg = __raw_readl(MXC_CCM_MCGR2);
		reg |= MXC_CCM_MCGR2_SPBA;
		__raw_writel(reg, MXC_CCM_MCGR2);
		break;
#ifdef CONFIG_MOT_WFN441
	case NFC_CLK:
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
