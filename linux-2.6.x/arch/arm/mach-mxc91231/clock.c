/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * Date        Author            Comment
 * ==========  ================  ========================
 * 12/05/2006  Motorola          Dynamic PLL lock/unlock
 * 12/07/2006  Motorola          Added etm_enable_trigger_clock.
 * 04/16/2007  Motorola          Implement USB / SSI clock splitting.
 *                               Enable PLL request / release logic.
 */

/*!
 * @file clock.c
 * @brief API for setting up and retrieving clocks.
 *
 * This file contains API for setting up and retrieving clocks.
 *
 * @ingroup CLOCKS
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <linux/delay.h>

#include "crm_regs.h"

#define MXC_PLL_REF_CLK                         26000000
#define NO_UPDATE                               -1

/*
 * Define reference clock inputs
 */
#define CKIH_CLK_FREQ               26000000

#ifdef CONFIG_MOT_WFN420

#include <asm/div64.h>

/*
 * SEXT does a sign extension on the given value. nbits is the number
 * of bits consumed by val, including the sign bit. nbits larger than
 * 31 doesn't make sense. This macro just shifts the bits up to the
 * top of the word and then back down. This causes the sign bit to
 * propagate when shifting back. Note that ANSI C characterizes this
 * behavior as implementation defined.
 */

#define SEXT(nbits,val)         ((((int)(val)) << (32-(nbits))) >> (32-(nbits)))

#endif /* CONFIG_MOT_WFN420 */

/*!
 * Spinlock to protect CRM register accesses
 */
static DEFINE_RAW_SPINLOCK(mxc_crm_lock);

static unsigned long pll_base[] = {
	(unsigned long)(IO_ADDRESS(PLL0_BASE_ADDR)),
	(unsigned long)(IO_ADDRESS(PLL1_BASE_ADDR)),
	(unsigned long)(IO_ADDRESS(PLL2_BASE_ADDR)),
	(unsigned long)(IO_ADDRESS(PLL3_BASE_ADDR)),
};

static unsigned long CRM_SMALL_DIV[] = { 2, 3, 4, 5, 6, 8, 10, 12 };

#ifdef CONFIG_MOT_FEAT_PM
static int pll_requests[NUM_MOT_PLLS] = { 1, 0, 0 };
#endif

/*!
 * This function is used to set the PLL pdf value into both dp_op registers
 *
 * @param pll_num   the PLL that you wish to modify
 * @param pdf       pre-division factor
 */
void mxc_pll_pdf_set(enum plls pll_num, unsigned int pdf)
{
        volatile unsigned long dp_op;
        unsigned long pllbase, flags;

        pllbase = pll_base[pll_num];
        spin_lock_irqsave(&mxc_crm_lock, flags);
	dp_op = __raw_readl(pllbase + MXC_PLL_DP_OP);
        dp_op = (dp_op & (~MXC_PLL_DP_OP_PDF_MASK)) | pdf;
	__raw_writel(dp_op, pll_base + MXC_PLL_DP_OP);

	dp_op = __raw_readl(pllbase + MXC_PLL_DP_HFS_OP);
	dp_op = (dp_op & (~MXC_PLL_DP_OP_PDF_MASK)) | pdf;
	__raw_writel(dp_op, pll_base + MXC_PLL_DP_HFS_OP);
        spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is used to modify PLL control register.
 * @param  pll_num  the PLL that you wish to modify
 * @param  reg_val  value to write into the control register
 */
void mxc_pll_dpctl_set(enum plls pll_num, unsigned long reg_val)
{
        volatile unsigned long dp_ctl;
        unsigned long pllbase, flags;
        
        pllbase = pll_base[pll_num];
        spin_lock_irqsave(&mxc_crm_lock, flags);
	dp_ctl = __raw_readl(pllbase + MXC_PLL_DP_CTL);
        dp_ctl |= reg_val;
	__raw_writel(dp_ctl, pllbase + MXC_PLL_DP_CTL);
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
        volatile unsigned long acsr, pll_hfsm, dp_op;
        unsigned int dp_op_off, mfn_off, mfd_off, pllbase, flags;

	BUG_ON(pll_num > (sizeof(pll_base) / sizeof(pll_base[0])));

        pllbase = pll_base[pll_num];

        spin_lock_irqsave(&mxc_crm_lock, flags);
        /* switch to ap_ref_clk */
	acsr = __raw_readl(MXC_CRMAP_ACSR);
        acsr &= ~MXC_CRMAP_ACSR_ACS;
	__raw_writel(acsr, MXC_CRMAP_ACSR);

        /* adjust pll settings */
	pll_hfsm = __raw_readl(pllbase + MXC_PLL_DP_CTL) & MXC_PLL_DP_CTL_HFSM;
        if (pll_hfsm == MXC_PLL_DP_CTL_HFSM) {
                dp_op_off = MXC_PLL_DP_HFS_OP;
                mfd_off = MXC_PLL_DP_HFS_MFD;
                mfn_off = MXC_PLL_DP_HFS_MFN;
        } else {
                dp_op_off = MXC_PLL_DP_OP;
                mfd_off = MXC_PLL_DP_MFD;
                mfn_off = MXC_PLL_DP_MFN;
        }
                
        if ((mfi != NO_UPDATE) && (pdf != NO_UPDATE)) {
		__raw_writel(mfi << MXC_PLL_DP_OP_MFI_OFFSET | pdf,
                       pllbase + dp_op_off);
        } else {
		dp_op = __raw_readl(pllbase + dp_op_off);
                if (pdf != NO_UPDATE) {
                        dp_op = (dp_op & (~MXC_PLL_DP_OP_PDF_MASK)) | pdf;
                } else {
                        dp_op = (dp_op & (~MXC_PLL_DP_OP_MFI_MASK)) | 
                                (mfi << MXC_PLL_DP_OP_MFI_OFFSET);
                }
		__raw_writel(dp_op, pllbase + dp_op_off);
        }
        if (mfd != NO_UPDATE) {
		__raw_writel(mfd, pllbase + mfd_off);
        }
        if (mfn != NO_UPDATE) {
		__raw_writel(mfn, pllbase + mfn_off);
        }

        /* switch back to pll */
        acsr |= MXC_CRMAP_ACSR_ACS;
	__raw_writel(acsr, MXC_CRMAP_ACSR);
        spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is used to get PLL registers values used generate the clock
 * frequency.
 *
 * @param  pll_num  the PLL that you wish to access
 * @param  mfi      pointer that holds multiplication factor integer part
 * @param  pdf      pointer that holds pre-division factor
 * @param  mfd      pointer that holds multiplication factor denominator
 * @param  mfn      pointer that holds multiplication factor numerator
 */
void mxc_pll_get(enum plls pll_num, unsigned int *mfi, unsigned int *pdf,
                 unsigned int *mfd, unsigned int *mfn)
{
        volatile unsigned long pll_hfsm, pll_dp_op;
        unsigned long pllbase, flags;
                                                                                        
	BUG_ON(pll_num > (sizeof(pll_base) / sizeof(pll_base[0])));
                                                                                        
        pllbase = pll_base[pll_num];

        spin_lock_irqsave(&mxc_crm_lock, flags);

	pll_hfsm = __raw_readl(pllbase + MXC_PLL_DP_CTL) & MXC_PLL_DP_CTL_HFSM;

        if (pll_hfsm == MXC_PLL_DP_CTL_HFSM) {
		pll_dp_op = __raw_readl(pllbase + MXC_PLL_DP_HFS_OP);
                *pdf = pll_dp_op & MXC_PLL_DP_OP_PDF_MASK;
                *mfi = pll_dp_op >> MXC_PLL_DP_OP_MFI_OFFSET;
		*mfd = __raw_readl(pllbase + MXC_PLL_DP_HFS_MFD);
		*mfn = __raw_readl(pllbase + MXC_PLL_DP_HFS_MFN);
        } else {
		pll_dp_op = __raw_readl(pllbase + MXC_PLL_DP_OP);
                *pdf = pll_dp_op & MXC_PLL_DP_OP_PDF_MASK;
                *mfi = pll_dp_op >> MXC_PLL_DP_OP_MFI_OFFSET;
		*mfd = __raw_readl(pllbase + MXC_PLL_DP_MFD);
		*mfn = __raw_readl(pllbase + MXC_PLL_DP_MFN);
        }

        spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function returns the PLL output value in Hz based on pll.
 * @param       pll_num     PLL as defined in enum plls
 * @return      PLL value in Hz.
 */
unsigned long mxc_pll_clock(enum plls pll_num)
{
        signed long mfi, mfn, mfd, pdf, ref_clk, pll_out;
#ifdef CONFIG_MOT_WFN420
        signed long val1; 
        s64 val2;
#endif
        volatile unsigned long dp_op, dp_mfd, dp_mfn, pll_hfsm;
        unsigned long pllbase, flags;

	BUG_ON(pll_num > (sizeof(pll_base) / sizeof(pll_base[0])));

        pllbase = pll_base[pll_num];

        spin_lock_irqsave(&mxc_crm_lock, flags);

	pll_hfsm = __raw_readl(pllbase + MXC_PLL_DP_CTL) & MXC_PLL_DP_CTL_HFSM;
        if (pll_hfsm == 0) {
		dp_op = __raw_readl(pllbase + MXC_PLL_DP_OP);
		dp_mfd = __raw_readl(pllbase + MXC_PLL_DP_MFD);
		dp_mfn = __raw_readl(pllbase + MXC_PLL_DP_MFN);
        } else {
		dp_op = __raw_readl(pllbase + MXC_PLL_DP_HFS_OP);
		dp_mfd = __raw_readl(pllbase + MXC_PLL_DP_HFS_MFD);
		dp_mfn = __raw_readl(pllbase + MXC_PLL_DP_HFS_MFN);
	}
	pdf = (signed long)(dp_op & MXC_PLL_DP_OP_PDF_MASK);
	mfi = (signed long)((dp_op >> MXC_PLL_DP_OP_MFI_OFFSET) &
                              MXC_PLL_DP_OP_PDF_MASK);
	mfi = (mfi <= 5) ? 5 : mfi;
	mfd = (signed long)(dp_mfd & MXC_PLL_DP_MFD_MASK);
	mfn = (signed long)(dp_mfn & MXC_PLL_DP_MFN_MASK);
        mfn = (mfn <= 0x4000000) ? mfn : (mfn - 0x10000000);

        ref_clk = MXC_PLL_REF_CLK;

#ifdef CONFIG_MOT_WFN420

        mfn = SEXT(27,mfn);

        val1 = 2 * ref_clk * mfi;
        val2 = 2 * ref_clk * (s64)mfn;

        do_div(val2, mfd);            /* requires <asm/div64.h> */

        pll_out = (val1 + (signed long)val2) / (pdf + 1);

#else
        pll_out = (2 * ref_clk * mfi + ((2 * ref_clk / (mfd + 1)) * mfn)) /
                  (pdf + 1);
#endif

        spin_unlock_irqrestore(&mxc_crm_lock, flags);

        return pll_out;
}

/*!
 * This function returns the main clock dividers.
 * @param       clk     peripheral clock as defined in enum mxc_clocks
 *
 * @return      divider value
 */
unsigned long mxc_main_clock_divider(enum mxc_clocks clk)
{
        unsigned long div = 0;
        volatile unsigned long acdr, acder2;

	acdr = __raw_readl(MXC_CRMAP_ACDR);
	acder2 = __raw_readl(MXC_CRMAP_ACDER2);

        switch (clk) {
        case CPU_CLK:
                div = (acdr & MXC_CRMAP_ACDR_ARMDIV_MASK) >> 
                       MXC_CRMAP_ACDR_ARMDIV_OFFSET;
                div = (div > 7) ? 1 : (CRM_SMALL_DIV[div]);
                break;
        case AHB_CLK:
                div = (acdr & MXC_CRMAP_ACDR_AHBDIV_MASK) >> 
                       MXC_CRMAP_ACDR_AHBDIV_OFFSET;
                div = (div == 0) ? 16 : div;
                break;
        case IPG_CLK:
                div = acdr & MXC_CRMAP_ACDR_IPDIV_MASK;
                div = (div == 0) ? 16 : div;
                break;
        case NFC_CLK:
                div = ((acder2 & MXC_CRMAP_ACDER2_NFCDIV_MASK) >> 
                        MXC_CRMAP_ACDER2_NFCDIV_OFFSET) + 1;
                break;
        case USB_CLK:
                div = (acder2 & MXC_CRMAP_ACDER2_USBDIV_MASK) >> 
                       MXC_CRMAP_ACDER2_USBDIV_OFFSET;
                div = (div > 7) ? 1 : (CRM_SMALL_DIV[div]);
                break;
        default:
                printk("Wrong clock: %d\n", clk);
                break;
        }

        return div;
}

/*!
 * This function returns the peripheral clock dividers.
 * Note that for SSI divider, in order to maintain the accuracy, the returned
 * divider is doubled.
 *
 * @param       clk     peripheral clock as defined in enum mxc_clocks
 * @return      divider value
 */
unsigned long mxc_peri_clock_divider(enum mxc_clocks clk)
{
        unsigned long div = 0;
        volatile unsigned long apra, aprb, acder1, acder2;

	apra = __raw_readl(MXC_CRMAP_APRA);
	aprb = __raw_readl(MXC_CRMAP_APRB);
	acder1 = __raw_readl(MXC_CRMAP_ACDER1);
	acder2 = __raw_readl(MXC_CRMAP_ACDER2);

        switch (clk) {
        case UART1_BAUD:
        case UART2_BAUD:
                div = acder2 & MXC_CRMAP_ACDER2_BAUDDIV_MASK;
                div = (div > 7) ? 1 : (CRM_SMALL_DIV[div]);
                break;
        case UART3_BAUD:
                div = (apra & MXC_CRMAP_APRA_UART3DIV_MASK) >> 
                       MXC_CRMAP_APRA_UART3DIV_OFFSET;
                div = (div > 7) ? 1 : (CRM_SMALL_DIV[div]);
                break;
        case SSI1_BAUD:
                div = acder1 & MXC_CRMAP_ACDER1_SSI1DIV_MASK;
                /* Return twice the divider to avoid FP */
		div = (div == 0 || div == 1) ? (62 * 2) : div;
                break;
        case SSI2_BAUD:
                div = (acder1 & MXC_CRMAP_ACDER1_SSI2DIV_MASK) >> 
                       MXC_CRMAP_ACDER1_SSI2DIV_OFFSET;
                /* Return twice the divider to avoid FP */
		div = (div == 0 || div == 1) ? (62 * 2) : div;
                break;
        case CSI_BAUD:
                if (system_rev == CHIP_REV_1_0) {
                        div = (acder1 & MXC_CRMAP_ACDER1_CSIDIV_MASK) >>
                               MXC_CRMAP_ACDER1_CSIDIV_OFFSET;
                        div = (div > 7) ? 1 : (CRM_SMALL_DIV[div]);
                } else {
                        div = (acder1 & MXC_CRMAP_ACDER1_CSIDIV_MASK_PASS2) >>
                               MXC_CRMAP_ACDER1_CSIDIV_OFFSET;
                        /* Return twice the divider to avoid FP */
			div = (div == 0 || div == 1) ? (62 * 2) : div;
                }                
                break;
        case SDHC1_CLK:
                div = (aprb & MXC_CRMAP_APRB_SDHC1_DIV_MASK_PASS2) >> 
                       MXC_CRMAP_APRB_SDHC1_DIV_OFFSET_PASS2;
                div = (div > 7) ? 1 : (CRM_SMALL_DIV[div]);
                break;
        case SDHC2_CLK:
                div = (aprb & MXC_CRMAP_APRB_SDHC2_DIV_MASK_PASS2) >> 
                       MXC_CRMAP_APRB_SDHC2_DIV_OFFSET_PASS2;
                div = (div > 7) ? 1 : (CRM_SMALL_DIV[div]);
                break;
        default:
                printk("Wrong clock: %d\n", clk);
                break;
        }

        return div;
}

/*!
 * This function returns various reference clocks for MXC91231 Pass 1.
 *
 * @param       ap_unc_pat_ref     pointer to ap_uncorrected_pat_ref value
 *                                 on return
 * @param       ap_ref_x2          pointer to ap_ref_x2 value on return
 * @param       ap_ref             pointer to ap_ref value on return
 *
 * @return      clock value in Hz
 */
void mxc_get_ref_clk_pass1(unsigned long *ap_unc_pat_ref,
			   unsigned long *ap_ref_x2, unsigned long *ap_ref)
{
        unsigned long ap_pat_ref_div_1, ap_pat_ref_div_2, ap_isel;
        volatile unsigned long acsr, ascsr, adcr, acder2;

	acsr = __raw_readl(MXC_CRMAP_ACSR);
	ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	adcr = __raw_readl(MXC_CRMAP_ADCR);
	acder2 = __raw_readl(MXC_CRMAP_ACDER2);

        ap_isel = ascsr & MXC_CRMAP_ASCSR_APISEL;
	ap_pat_ref_div_1 =
	    ((ascsr >> MXC_CRMAP_ASCSR_AP_PATDIV1_OFFSET) & 0x1) + 1;
	ap_pat_ref_div_2 =
	    ((ascsr >> MXC_CRMAP_ASCSR_AP_PATDIV2_OFFSET) & 0x1) + 1;

        *ap_unc_pat_ref = MXC_PLL_REF_CLK * (ap_isel + 1);
	*ap_ref_x2 = (*ap_unc_pat_ref) / ap_pat_ref_div_1;
        *ap_ref = (*ap_ref_x2) / ap_pat_ref_div_2;
}

/*!
 * This function returns various reference clocks for MXC91231 Pass 2.
 *
 * @param       ap_unc_pat_ref     pointer to ap_uncorrected_pat_ref value
 *                                 on return
 * @param       ap_ref_x2          pointer to ap_ref_x2 value on return
 * @param       ap_ref             pointer to ap_ref value on return
 *
 * @return      clock value in Hz
 */
void mxc_get_ref_clk_pass2(unsigned long *ap_unc_pat_ref,
			   unsigned long *ap_ref_x2, unsigned long *ap_ref)
{
        unsigned long ap_pat_ref_div_2, ap_isel, acs, ads;
        volatile unsigned long acsr, ascsr, adcr, acder2;

	acsr = __raw_readl(MXC_CRMAP_ACSR);
	ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	adcr = __raw_readl(MXC_CRMAP_ADCR);
	acder2 = __raw_readl(MXC_CRMAP_ACDER2);

        ap_isel = ascsr & MXC_CRMAP_ASCSR_APISEL;
	ap_pat_ref_div_2 =
	    ((ascsr >> MXC_CRMAP_ASCSR_AP_PATDIV2_OFFSET) & 0x1) + 1;

        *ap_ref_x2 =  MXC_PLL_REF_CLK;
        ads = ((acsr >> MXC_CRMAP_ACSR_ADS_OFFSET_PASS2) & 0x1);
        acs = acsr & MXC_CRMAP_ACSR_ACS;

        if (acs == 0) {
                if (ads == 0) {
                        *ap_ref = MXC_PLL_REF_CLK / ap_pat_ref_div_2;               
                } else {
                        *ap_ref = MXC_PLL_REF_CLK * (ap_isel + 1);
                }
                *ap_unc_pat_ref = *ap_ref / 2;
        } else {
                *ap_ref = MXC_PLL_REF_CLK / ap_pat_ref_div_2;               
                *ap_unc_pat_ref = *ap_ref;
        }
}

/*!
 * This function returns various reference clocks for MXC91231.
 *
 * @param       ap_unc_pat_ref     pointer to ap_uncorrected_pat_ref value
 *                                 on return
 * @param       ap_ref_x2          pointer to ap_ref_x2 value on return
 * @param       ap_ref             pointer to ap_ref value on return
 *
 * @return      clock value in Hz
 */
void mxc_get_ref_clk(unsigned long *ap_unc_pat_ref, unsigned long *ap_ref_x2,
                     unsigned long *ap_ref)
{
        if (system_rev == CHIP_REV_1_0) {
                mxc_get_ref_clk_pass1(ap_unc_pat_ref, ap_ref_x2, ap_ref);
        } else {
                mxc_get_ref_clk_pass2(ap_unc_pat_ref, ap_ref_x2, ap_ref);
        }
}

/*!
 * This function returns the clock value in Hz for various MXC modules MXC91231 Pass 1.0.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
static unsigned long mxc_get_clocks_pass1(enum mxc_clocks clk)
{
        volatile unsigned long apra, aprb, acsr, ascsr, adcr, acder1, acder2; 
        unsigned long ap_unc_pat_ref, sel, ret_val = 0, apsel, 
                      ap_clk_pre_dfs, lfdf = 1, ap_ref_x2_clk, ap_ref_clk;

	apra = __raw_readl(MXC_CRMAP_APRA);
	aprb = __raw_readl(MXC_CRMAP_APRB);
	acsr = __raw_readl(MXC_CRMAP_ACSR);
	ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	adcr = __raw_readl(MXC_CRMAP_ADCR);
	acder1 = __raw_readl(MXC_CRMAP_ACDER1);
	acder2 = __raw_readl(MXC_CRMAP_ACDER2);

        mxc_get_ref_clk(&ap_unc_pat_ref, &ap_ref_x2_clk, &ap_ref_clk);

        if ((acsr & MXC_CRMAP_ACSR_ACS) == 0) {
                // inverted pat_ref is selected
                ap_clk_pre_dfs = ap_ref_clk;
        } else {
                // Now AP domain runs off the pll
                apsel = (ascsr & MXC_CRMAP_ASCSR_APSEL_MASK) >> 
                         MXC_CRMAP_ASCSR_APSEL_OFFSET;
                ap_clk_pre_dfs = mxc_pll_clock(apsel) / 
                                 mxc_main_clock_divider(CPU_CLK);
        }

        switch (clk) {
        case CPU_CLK:
                if ((adcr & MXC_CRMAP_ADCR_DIV_BYP) == 0) {
                        // DFS divider used
                        lfdf = mxc_get_lfdf_value();
                }
                ret_val = ap_clk_pre_dfs / lfdf;
                break;
        case AHB_CLK:
        case IPU_CLK:
                ret_val = ap_clk_pre_dfs / mxc_main_clock_divider(AHB_CLK);
                break;
        case NFC_CLK:
		if ((acder2 & MXC_CRMAP_ACDER2_NFCEN) == 0) {
                        printk("Warning: NFC clock is not enabled !!!\n");
                } else {
			ret_val =
			    ap_clk_pre_dfs / (mxc_main_clock_divider(AHB_CLK) *
                                                    mxc_main_clock_divider(NFC_CLK));
                }
                break;
        case USB_CLK:
		if ((acder2 & MXC_CRMAP_ACDER2_USBEN) == 0) {
                        printk("Warning: USB clock is not enabled !!!\n");
                } else {
                        if ((acsr & MXC_CRMAP_ACSR_ACS) == 0) {
                                // inverted pat_ref is selected
                                ret_val = ap_ref_clk / 8;
                        } else {
                                sel = (ascsr & MXC_CRMAP_ASCSR_USBSEL_MASK) >> 
                                       MXC_CRMAP_ASCSR_USBSEL_OFFSET;
                                ret_val = mxc_pll_clock(sel) /
                                          mxc_main_clock_divider(USB_CLK);
                        }
                }
                break;
        case UART1_BAUD:
                if ((apra & MXC_CRMAP_APRA_UART1EN) == 0) {
                        printk("Warning: UART 1 clock is not enabled !!!\n");
                } else {
                        ret_val = ap_unc_pat_ref /
                                  mxc_peri_clock_divider(UART1_BAUD);
                }
                break;
        case UART2_BAUD:
                if ((apra & MXC_CRMAP_APRA_UART2EN) == 0) {
                        printk("Warning: UART 2 clock is not enabled !!!\n");
                } else {
                        ret_val = ap_unc_pat_ref /
                                  mxc_peri_clock_divider(UART2_BAUD);
                }
                break;
        case UART3_BAUD:
                if ((apra & MXC_CRMAP_APRA_UART3EN) == 0) {
                        printk("Warning: UART 3 clock is not enabled !!!\n");
                } else {
                        ret_val = ap_unc_pat_ref /
                                  mxc_peri_clock_divider(UART3_BAUD);
                }
                break;
        case SDHC1_CLK:
                if ((aprb & MXC_CRMAP_APRB_SDHC1EN) == 0) {
                        printk("Warning: SDHC1 clock is not enabled !!!\n");
                } else {
                        ret_val = ap_unc_pat_ref /
                                  mxc_peri_clock_divider(UART2_BAUD);
                }
                break;
        case SDHC2_CLK:
                if ((aprb & MXC_CRMAP_APRB_SDHC2EN) == 0) {
                        printk("Warning: SDHC2 clock is not enabled !!!\n");
                } else {
                        ret_val = ap_unc_pat_ref /
                                  mxc_peri_clock_divider(UART2_BAUD);
                }
                break;
        case SSI1_BAUD:
                if ((acder1 & MXC_CRMAP_ACDER1_SSI1EN) == 0) {
                        printk("Warning: SSI 1 clock is not enabled !!!\n");
                } else {
                        if ((acsr & MXC_CRMAP_ACSR_ACS) == 0) {
                                // inverted pat_ref is selected
                                ret_val = ap_ref_clk / 8;
                        } else {
                                sel = (ascsr & MXC_CRMAP_ASCSR_SSI1SEL_MASK) >> 
                                       MXC_CRMAP_ASCSR_SSI1SEL_OFFSET;
                                /* Don't forget to double the numerator */
                                ret_val = (2 * mxc_pll_clock(sel)) /
                                          mxc_peri_clock_divider(SSI1_BAUD);
                        }
                }
                break;
        case SSI2_BAUD:
                if ((acder1 & MXC_CRMAP_ACDER1_SSI2EN) == 0) {
                        printk("Warning: SSI 2 clock is not enabled !!!\n");
                } else {
                        if ((acsr & MXC_CRMAP_ACSR_ACS) == 0) {
                                // inverted pat_ref is selected
                                ret_val = ap_ref_clk / 8;
                        } else {
                                sel = (ascsr & MXC_CRMAP_ASCSR_SSI2SEL_MASK) >> 
                                       MXC_CRMAP_ASCSR_SSI2SEL_OFFSET;
                                /* Don't forget to double the numerator */
                                ret_val = (2 * mxc_pll_clock(sel)) /
                                          mxc_peri_clock_divider(SSI2_BAUD);
                        }
                }
                break;
        case CSI_BAUD:
                if ((acder1 & MXC_CRMAP_ACDER1_CSIEN) == 0) {
                        printk("Warning: CSI clock is not enabled !!!\n");
                } else {
                        if ((acsr & MXC_CRMAP_ACSR_ACS) == 0) {
                                // inverted pat_ref is selected
                                ret_val = ap_ref_clk / 8;
                        } else {
                                sel = (ascsr & MXC_CRMAP_ASCSR_CSISEL_MASK) >> 
                                       MXC_CRMAP_ASCSR_CSISEL_OFFSET;
                                ret_val = mxc_pll_clock(sel) / 
                                          (mxc_peri_clock_divider(CSI_BAUD));
                        }
                }
                break;
        case I2C_CLK:
        case MQSPI_CKIH_CLK:
        case EL1T_NET_CLK:
                if ((acsr & MXC_CRMAP_ACSR_ACS) == 0) {
                        // inverted pat_ref is selected
                        ret_val = ap_ref_clk / 4;
                } else {
                        ret_val = ap_ref_clk;
                }
                break;
        case CSPI1_CLK:
        case CSPI2_CLK:
        case GPT_CLK:
        case RTC_CLK:
        case EPIT1_CLK:
        case EDIO_CLK:
        case WDOG_CLK:
        case WDOG2_CLK:
        case PWM_CLK:
        case SIM1_CLK:
        case HAC_CLK:
        case GEM_CLK:
        case SDMA_CLK:
        case RNG_CLK:
        case KPP_CLK:
        case MU_CLK:
        case RTIC_CLK:
        case SCC_CLK:
        case SPBA_CLK:
        case DSM_CLK:
        case MQSPI_IPG_CLK:
        case EL1T_IPG_CLK:
        case IPG_CLK:
                ret_val = ap_clk_pre_dfs / mxc_main_clock_divider(IPG_CLK);
                break;
        default:
                printk("This clock is not available\n"); 
                break;
        }

        return ret_val;
}

static unsigned long mxc_get_corrected_clk(void)
{
        volatile unsigned long cccr;
        unsigned long retval;
        unsigned int sel, div;

	cccr = __raw_readl(MXC_CRMCOM_CCCR);

        div = (cccr & MXC_CRMCOM_CCCR_CC_DIV_MASK) >> 
               MXC_CRMCOM_CCCR_CC_DIV_OFFSET;
        sel = (cccr & MXC_CRMCOM_CCCR_CC_SEL_MASK) >> 
               MXC_CRMCOM_CCCR_CC_SEL_OFFSET;
        retval = mxc_pll_clock(sel) / (div + 1);
        return retval;
}

static unsigned long mxc_get_usb_clk_pass2(void) 
{
        volatile unsigned long ascsr;
        unsigned long ret_val = 0;
        unsigned int sel;

	ascsr = __raw_readl(MXC_CRMAP_ASCSR);
        sel = (ascsr & MXC_CRMAP_ASCSR_USBSEL_MASK) >> 
               MXC_CRMAP_ASCSR_USBSEL_OFFSET;
        ret_val = mxc_pll_clock(sel) / mxc_main_clock_divider(USB_CLK);
        return ret_val;
}

static unsigned long mxc_get_peri_clock_pass2(unsigned int clk_sel)
{
        unsigned long clk_src;

        if (clk_sel == 0) {
                clk_src = MXC_PLL_REF_CLK; 
        } else if (clk_sel == 1) {
                clk_src = 2 * MXC_PLL_REF_CLK;
        } else {
                clk_src = mxc_get_usb_clk_pass2();
        }
        return clk_src;
}

/*!
 * This function returns the clock value in Hz for various MXC modules MXC91231 Pass 2.0.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
static unsigned long mxc_get_clocks_pass2(enum mxc_clocks clk)
{
        volatile unsigned long apra, aprb, acsr, ascsr, adcr, acder1, acder2;
        unsigned long ap_unc_pat_ref, sel, ret_val = 0, apsel, clk_src,
                      ap_clk_pre_dfs, lfdf = 1, ap_ref_x2_clk, ap_ref_clk;
        unsigned int div = 0;        

	apra = __raw_readl(MXC_CRMAP_APRA);
	aprb = __raw_readl(MXC_CRMAP_APRB);
	acsr = __raw_readl(MXC_CRMAP_ACSR);
	ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	adcr = __raw_readl(MXC_CRMAP_ADCR);
	acder1 = __raw_readl(MXC_CRMAP_ACDER1);
	acder2 = __raw_readl(MXC_CRMAP_ACDER2);

        mxc_get_ref_clk(&ap_unc_pat_ref, &ap_ref_x2_clk, &ap_ref_clk);

        if ((acsr & MXC_CRMAP_ACSR_ACS) == 0) {
                // inverted pat_ref is selected
                ap_clk_pre_dfs = ap_ref_clk;
        } else {
                // Now AP domain runs off the pll
                apsel = (ascsr & MXC_CRMAP_ASCSR_APSEL_MASK) >> 
                         MXC_CRMAP_ASCSR_APSEL_OFFSET;
                ap_clk_pre_dfs = mxc_pll_clock(apsel) / 
                                 mxc_main_clock_divider(CPU_CLK);
        }

        switch (clk) {
        case CPU_CLK:
		if (((adcr & MXC_CRMAP_ADCR_DIV_BYP) == 0) &&
		    ((adcr & MXC_CRMAP_ADCR_VSTAT) != 0)) {
                        // DFS divider used
                        lfdf = mxc_get_lfdf_value();
                }
                ret_val = ap_clk_pre_dfs / lfdf;
                break;
        case AHB_CLK:
        case IPU_CLK:
                ret_val = ap_clk_pre_dfs / mxc_main_clock_divider(AHB_CLK);
                break;
        case NFC_CLK:
		if ((acder2 & MXC_CRMAP_ACDER2_NFCEN) == 0) {
                        printk("Warning: NFC clock is not enabled !!!\n");
                } else {
			ret_val =
			    ap_clk_pre_dfs / (mxc_main_clock_divider(AHB_CLK) *
                                                    mxc_main_clock_divider(NFC_CLK));
                }
                break;
        case USB_CLK:
		if ((acder2 & MXC_CRMAP_ACDER2_USBEN) == 0) {
                        printk("Warning: USB clock is not enabled !!!\n");
                } else {
                        mxc_get_usb_clk_pass2();
                }
                break;
        case UART1_BAUD: 
                if ((apra & MXC_CRMAP_APRA_UART1EN) == 0) {
                        printk("Warning: UART 1 clock is not enabled !!!\n");
                } else {
                        sel = (acder2 & MXC_CRMAP_ACDER2_BAUD_ISEL_MASK_PASS2) 
                               >> MXC_CRMAP_ACDER2_BAUD_ISEL_OFF_PASS2;
                        if (sel == 0) {
                                clk_src = MXC_PLL_REF_CLK; 
                        } else {
                                clk_src = 2 * MXC_PLL_REF_CLK;
                        }
			ret_val = clk_src / mxc_peri_clock_divider(UART1_BAUD);
                }
                break;
        case UART2_BAUD:
                if ((apra & MXC_CRMAP_APRA_UART2EN) == 0) {
                        printk("Warning: UART 2 clock is not enabled !!!\n");
                } else {
			sel =
			    (acder2 & MXC_CRMAP_ACDER2_BAUD_ISEL_MASK_PASS2) >>
                               MXC_CRMAP_ACDER2_BAUD_ISEL_OFF_PASS2;
                        if (sel == 0) {
                                clk_src = MXC_PLL_REF_CLK; 
                        } else {
                                clk_src = 2 * MXC_PLL_REF_CLK;
                        }
			ret_val = clk_src / mxc_peri_clock_divider(UART2_BAUD);
                }
                break;
        case UART3_BAUD:
                if ((apra & MXC_CRMAP_APRA_UART3EN) == 0) {
                        printk("Warning: UART 3 clock is not enabled !!!\n");
                } else {
			sel =
			    (acder2 & MXC_CRMAP_ACDER2_BAUD_ISEL_MASK_PASS2) >>
                               MXC_CRMAP_ACDER2_BAUD_ISEL_OFF_PASS2;
                        if (sel == 0) {
                                clk_src = MXC_PLL_REF_CLK; 
                        } else {
                                clk_src = 2 * MXC_PLL_REF_CLK;
                        }
			ret_val = clk_src / mxc_peri_clock_divider(UART3_BAUD);
                }
                break;
#ifdef CONFIG_MOT_WFN441
        case SIM1_CLK:
                if ((apra & MXC_CRMAP_APRA_SIMEN) == 0) {
                        printk("Warning: SIM clock is not enabled !!!\n");
                } else {
			sel =
			    (acder2 & MXC_CRMAP_ACDER2_BAUD_ISEL_MASK_PASS2) >>
                               MXC_CRMAP_ACDER2_BAUD_ISEL_OFF_PASS2;
                        if (sel == 0) {
                                ret_val = MXC_PLL_REF_CLK; 
                        } else {
                                ret_val = 2 * MXC_PLL_REF_CLK;
                        }
                }
                break;
#endif /* CONFIG_MOT_WFN441 */
        case SDHC1_CLK:
                if ((aprb & MXC_CRMAP_APRB_SDHC1EN) == 0) {
                        printk("Warning: SDHC1 clock is not enabled !!!\n");
                } else {
                        sel = (aprb & MXC_CRMAP_APRB_SDHC1_ISEL_MASK_PASS2) >>
                               MXC_CRMAP_APRB_SDHC1_ISEL_OFFSET_PASS2;
                        clk_src = mxc_get_peri_clock_pass2(sel);
			ret_val = clk_src / mxc_peri_clock_divider(SDHC1_CLK);
                }
                break;
        case SDHC2_CLK:
                if ((aprb & MXC_CRMAP_APRB_SDHC2EN) == 0) {
                        printk("Warning: SDHC2 clock is not enabled !!!\n");
                } else {
                        sel = (aprb & MXC_CRMAP_APRB_SDHC2_ISEL_MASK_PASS2) >>
                               MXC_CRMAP_APRB_SDHC2_ISEL_OFFSET_PASS2;
                        clk_src = mxc_get_peri_clock_pass2(sel);
			ret_val = clk_src / mxc_peri_clock_divider(SDHC2_CLK);
                }
                break;
        case SSI1_BAUD:
                if ((acder1 & MXC_CRMAP_ACDER1_SSI1EN) == 0) {
                        printk("Warning: SSI 1 clock is not enabled !!!\n");
                } else {
                        sel = (ascsr & MXC_CRMAP_ASCSR_SSI1SEL_MASK) >> 
                               MXC_CRMAP_ASCSR_SSI1SEL_OFFSET;
                        /* Don't forget to double the numerator */
                        ret_val = (2 * mxc_pll_clock(sel)) /
                                   mxc_peri_clock_divider(SSI1_BAUD);
                }
                break;
        case SSI2_BAUD:
                if ((acder1 & MXC_CRMAP_ACDER1_SSI2EN) == 0) {
                        printk("Warning: SSI 2 clock is not enabled !!!\n");
                } else {
                        sel = (ascsr & MXC_CRMAP_ASCSR_SSI2SEL_MASK) >> 
                               MXC_CRMAP_ASCSR_SSI2SEL_OFFSET;
                        /* Don't forget to double the numerator */
                        ret_val = (2 * mxc_pll_clock(sel)) /
                                   mxc_peri_clock_divider(SSI2_BAUD);
                }
                break;
        case CSI_BAUD:
                if ((acder1 & MXC_CRMAP_ACDER1_CSIEN_PASS2) == 0) {
                        printk("Warning: CSI clock is not enabled !!!\n");
                } else {
                        sel = (ascsr & MXC_CRMAP_ASCSR_CSISEL_MASK) >> 
                               MXC_CRMAP_ASCSR_CSISEL_OFFSET;
                        /* Don't forget to double the numerator */
                        ret_val = (2 * mxc_pll_clock(sel)) / 
                                   mxc_peri_clock_divider(CSI_BAUD);
                }
                break;
        case I2C_CLK:
                ret_val = ap_unc_pat_ref;
                break;
        case MQSPI_CKIH_CLK:
        case EL1T_NET_CLK:
                if ((ascsr & MXC_CRMAP_ASCSR_CRS) == 0) {
                        ret_val = ap_unc_pat_ref;
                } else {
			div = (acder2 & MXC_CRMAP_ACDER2_CRCT_CLK_DIV_MSK_PASS2)
			    >> MXC_CRMAP_ACDER2_CRCT_CLK_DIV_OFF_PASS2;
                        div = CRM_SMALL_DIV[div];
                        ret_val = mxc_get_corrected_clk() / div;     
                }
                break;
        case MQSPI_IPG_CLK:
                if ((apra & MXC_CRMAP_APRA_MQSPI_EN_PASS2) == 0) {
                        printk("Warning: MQSPI IPG clock is not enabled !!!\n");
                } else {
                        ret_val = mxc_get_corrected_clk();
                }
                break;
        case EL1T_IPG_CLK:
                if ((apra & MXC_CRMAP_APRA_EL1T_EN_PASS2) == 0) {
                        printk("Warning: EL1T IPG clock is not enabled !!!\n");
                } else {
                        ret_val = mxc_get_corrected_clk();
                }
                break;
        case SAHARA2_AHB_CLK:
                if ((apra & MXC_CRMAP_APRA_SAHARA_DIV2_CLK_EN_PASS2) == 0) {
			printk
			    ("Warning: SAHARA2 AHB clock is not enabled !!!\n");
		} else {
			ret_val =
			    ap_clk_pre_dfs / (2 *
					      mxc_main_clock_divider(AHB_CLK));
                }
                break;
        case CSPI1_CLK:
        case CSPI2_CLK:
        case GPT_CLK:
        case RTC_CLK:
        case EPIT1_CLK:
        case EDIO_CLK:
        case WDOG_CLK:
        case WDOG2_CLK:
        case PWM_CLK:
#ifndef CONFIG_MOT_WFN441
        case SIM1_CLK:
#endif
        case HAC_CLK:
        case GEM_CLK:
        case SDMA_CLK:
        case RNG_CLK:
        case KPP_CLK:
        case MU_CLK:
        case RTIC_CLK:
        case SCC_CLK:
        case SPBA_CLK:
        case DSM_CLK:
        case SAHARA2_IPG_CLK:
	case LPMC_CLK:
        case IPG_CLK:
                ret_val = ap_clk_pre_dfs / mxc_main_clock_divider(IPG_CLK);
                break;
	case CKIH_CLK:
		return CKIH_CLK_FREQ;
		break;
	default:
		ret_val = 0;
		break;
	}

	return ret_val;
}

/*!
 * This function returns the clock value in Hz for MXC91231.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks(enum mxc_clocks clk)
{
        if (system_rev == CHIP_REV_1_0) {
                return mxc_get_clocks_pass1(clk);
        } else {
                return mxc_get_clocks_pass2(clk);
        }
}

static unsigned long mxc_get_clocks_parent_pass1(enum mxc_clocks clk)
{
	volatile unsigned long acsr, ascsr, acder1;
	unsigned long sel, ret_val = 0;

	acsr = __raw_readl(MXC_CRMAP_ACSR);
	ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	acder1 = __raw_readl(MXC_CRMAP_ACDER1);

	switch (clk) {
	case CPU_CLK:
	case AHB_CLK:
	case IPU_CLK:
	case NFC_CLK:
	case USB_CLK:
#ifdef CONFIG_MOT_FEAT_PM
	case USB_AHB_CLK:
#endif
	case UART1_BAUD:
	case UART2_BAUD:
	case UART3_BAUD:
	case SDHC1_CLK:
	case SDHC2_CLK:
	case SSI1_BAUD:
	case SSI2_BAUD:
	case CSI_BAUD:
		if ((acder1 & MXC_CRMAP_ACDER1_CSIEN) == 0) {
			printk("Warning: CSI clock is not enabled !!!\n");
		} else {
			if ((acsr & MXC_CRMAP_ACSR_ACS) == 0) {
				printk
				    ("mxc_get_clocks_parent_pass1 Warning: fix me!!!\n");
			} else {
				sel = (ascsr & MXC_CRMAP_ASCSR_CSISEL_MASK) >>
				    MXC_CRMAP_ASCSR_CSISEL_OFFSET;
				ret_val = mxc_pll_clock(sel);
			}
		}
		break;
	case I2C_CLK:
	case MQSPI_CKIH_CLK:
	case EL1T_NET_CLK:
	case MQSPI_IPG_CLK:
	case EL1T_IPG_CLK:
	case SAHARA2_AHB_CLK:
	case CSPI1_CLK:
	case CSPI2_CLK:
	case GPT_CLK:
	case RTC_CLK:
	case EPIT1_CLK:
	case EDIO_CLK:
	case WDOG_CLK:
	case WDOG2_CLK:
	case PWM_CLK:
	case SIM1_CLK:
	case HAC_CLK:
	case GEM_CLK:
	case SDMA_CLK:
	case RNG_CLK:
	case KPP_CLK:
	case MU_CLK:
	case RTIC_CLK:
	case SCC_CLK:
	case SPBA_CLK:
	case DSM_CLK:
	case SAHARA2_IPG_CLK:
	case IPG_CLK:
#ifdef CONFIG_MOT_FEAT_PM
	case USB_IPG_CLK:
        case SSI1_IPG_CLK:
        case SSI2_IPG_CLK:
#endif
	default:
		break;
	}

	return ret_val;
}

static unsigned long mxc_get_clocks_parent_pass2(enum mxc_clocks clk)
{
	volatile unsigned long ascsr, acder1;
	unsigned long sel, ret_val = 0;

	ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	acder1 = __raw_readl(MXC_CRMAP_ACDER1);

	switch (clk) {
	case CPU_CLK:
	case AHB_CLK:
	case IPU_CLK:
	case NFC_CLK:
	case USB_CLK:
#ifdef CONFIG_MOT_FEAT_PM
        case USB_AHB_CLK:
#endif
	case UART1_BAUD:
	case UART2_BAUD:
	case UART3_BAUD:
	case SDHC1_CLK:
	case SDHC2_CLK:
	case SSI1_BAUD:
	case SSI2_BAUD:
	case CSI_BAUD:
		if ((acder1 & MXC_CRMAP_ACDER1_CSIEN_PASS2) == 0) {
			printk("Warning: CSI clock is not enabled !!!\n");
		} else {
			sel = (ascsr & MXC_CRMAP_ASCSR_CSISEL_MASK) >>
			    MXC_CRMAP_ASCSR_CSISEL_OFFSET;
			/* Don't forget to double the numerator */
			ret_val = mxc_pll_clock(sel);
		}
		break;
	case I2C_CLK:
	case MQSPI_CKIH_CLK:
	case EL1T_NET_CLK:
	case MQSPI_IPG_CLK:
	case EL1T_IPG_CLK:
	case SAHARA2_AHB_CLK:
	case CSPI1_CLK:
	case CSPI2_CLK:
	case GPT_CLK:
	case RTC_CLK:
	case EPIT1_CLK:
	case EDIO_CLK:
	case WDOG_CLK:
	case WDOG2_CLK:
	case PWM_CLK:
	case SIM1_CLK:
	case HAC_CLK:
	case GEM_CLK:
	case SDMA_CLK:
	case RNG_CLK:
	case KPP_CLK:
	case MU_CLK:
	case RTIC_CLK:
	case SCC_CLK:
	case SPBA_CLK:
	case DSM_CLK:
	case SAHARA2_IPG_CLK:
	case IPG_CLK:
#ifdef CONFIG_MOT_FEAT_PM
        case SSI1_IPG_CLK:
        case SSI2_IPG_CLK:
        case USB_IPG_CLK:
#endif
	default:
		break;
	}

	return ret_val;
}

/*!
 * This function returns the parent clock value in Hz for MXC91231.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks_parent(enum mxc_clocks clk)
{
	if (system_rev == CHIP_REV_1_0) {
		return mxc_get_clocks_parent_pass1(clk);
	} else {
		return mxc_get_clocks_parent_pass2(clk);
	}
}

/*!
 * This function sets the PLL source for a clock.
 *
 * @param clk     as defined in enum mxc_clocks
 * @param pll_num the PLL that you wish to use as source for this clock
 */
static void mxc_set_clocks_src(enum mxc_clocks clk, enum plls pll_num)
{
        volatile unsigned long reg;
        unsigned long flags;

        spin_lock_irqsave(&mxc_crm_lock, flags);

        switch (clk) {
        case CPU_CLK:
		reg = __raw_readl(MXC_CRMAP_ASCSR);
                reg = (reg & (~MXC_CRMAP_ASCSR_APSEL_MASK)) | 
                      (pll_num << MXC_CRMAP_ASCSR_APSEL_OFFSET); 
		__raw_writel(reg, MXC_CRMAP_ASCSR);
                break;
        case USB_CLK:
		reg = __raw_readl(MXC_CRMAP_ASCSR);
                reg = (reg & (~MXC_CRMAP_ASCSR_USBSEL_MASK)) | 
                      (pll_num << MXC_CRMAP_ASCSR_USBSEL_OFFSET); 
		__raw_writel(reg, MXC_CRMAP_ASCSR);
                break;
        case SSI1_BAUD:
		reg = __raw_readl(MXC_CRMAP_ASCSR);
                reg = (reg & (~MXC_CRMAP_ASCSR_SSI1SEL_MASK)) | 
                      (pll_num << MXC_CRMAP_ASCSR_SSI1SEL_OFFSET); 
		__raw_writel(reg, MXC_CRMAP_ASCSR);
                break;
        case SSI2_BAUD:
		reg = __raw_readl(MXC_CRMAP_ASCSR);
                reg = (reg & (~MXC_CRMAP_ASCSR_SSI2SEL_MASK)) | 
                      (pll_num << MXC_CRMAP_ASCSR_SSI2SEL_OFFSET); 
		__raw_writel(reg, MXC_CRMAP_ASCSR);
                break;
        case CSI_BAUD:
		reg = __raw_readl(MXC_CRMAP_ASCSR);
                reg = (reg & (~MXC_CRMAP_ASCSR_CSISEL_MASK)) | 
                      (pll_num << MXC_CRMAP_ASCSR_CSISEL_OFFSET); 
		__raw_writel(reg, MXC_CRMAP_ASCSR);
                break;
        case UART1_BAUD:
        case UART2_BAUD:
        case UART3_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
                reg = (reg & (~MXC_CRMAP_ACDER2_BAUD_ISEL_MASK_PASS2)) | 
                      ((pll_num - 4) << MXC_CRMAP_ACDER2_BAUD_ISEL_OFF_PASS2); 
		__raw_writel(reg, MXC_CRMAP_ACDER2);
                break;
        case SDHC1_CLK:
		reg = __raw_readl(MXC_CRMAP_APRB);
                reg = (reg & (~MXC_CRMAP_APRB_SDHC1_ISEL_MASK_PASS2)) | 
                      ((pll_num - 4) << MXC_CRMAP_APRB_SDHC1_ISEL_OFFSET_PASS2); 
		__raw_writel(reg, MXC_CRMAP_APRB);
                break;
        case SDHC2_CLK:
		reg = __raw_readl(MXC_CRMAP_APRB);
                reg = (reg & (~MXC_CRMAP_APRB_SDHC2_ISEL_MASK_PASS2)) | 
                      ((pll_num - 4) << MXC_CRMAP_APRB_SDHC2_ISEL_OFFSET_PASS2); 
		__raw_writel(reg, MXC_CRMAP_APRB);
                break;
        default:
                printk("This clock does not have ability to choose the PLL\n");
                break;
        }
        spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function sets the PLL source for a clock. Function checks to see differences 
 * between Pass 1.0 and Pass 2.0
 *
 * @param clk     as defined in enum mxc_clocks
 * @param pll_num the PLL that you wish to use as source for this clock
 */
void mxc_set_clocks_pll(enum mxc_clocks clk, enum plls pll_num)
{
        if (system_rev == CHIP_REV_1_0) {
                switch (clk) {
                case UART1_BAUD:
                case UART2_BAUD:
                case UART3_BAUD:
                case SDHC1_CLK:
                case SDHC2_CLK:
			printk
			    ("This clock does not have ability to choose the PLL\n");
                        return;
                default:
                        mxc_set_clocks_src(clk, pll_num);
                        break;
                }
        } else {
                mxc_set_clocks_src(clk, pll_num);
        }
}

/*!
 * This function sets the division factor for a clock.
 *
 * @param clk as defined in enum mxc_clocks
 * @param div the division factor to be used for the clock (For SSI & CSI, pass in
 *            2 times the expected division value to account for FP vals)
 */
static void mxc_set_clocks_div_common(enum mxc_clocks clk, unsigned int div)
{
        volatile unsigned long reg;
        unsigned long flags;
        unsigned int d;

        spin_lock_irqsave(&mxc_crm_lock, flags);

        switch (clk) {
        case AHB_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDR);
                d = (div == 16) ? 0 : div;
                reg = (reg & (~MXC_CRMAP_ACDR_AHBDIV_MASK)) |
                      (d << MXC_CRMAP_ACDR_AHBDIV_OFFSET);
		__raw_writel(reg, MXC_CRMAP_ACDR);
                break;
        case IPG_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDR);
                d = (div == 16) ? 0 : div;
                reg = (reg & (~MXC_CRMAP_ACDR_IPDIV_MASK)) |
                      (d << MXC_CRMAP_ACDR_IPDIV_OFFSET);
		__raw_writel(reg, MXC_CRMAP_ACDR);
                break;
        case NFC_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
                reg = (reg & (~MXC_CRMAP_ACDER2_NFCDIV_MASK)) |
                      ((div - 1) << MXC_CRMAP_ACDER2_NFCDIV_OFFSET);
		__raw_writel(reg, MXC_CRMAP_ACDER2);
                break;
        case CPU_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDR);
                d = (div <= 1) ? 15 : (div - 2);
                if (div > 6) {
                        d = d - ((div - 6) / 2);
                }
                reg = (reg & (~MXC_CRMAP_ACDR_ARMDIV_MASK)) |
                      (d << MXC_CRMAP_ACDR_ARMDIV_OFFSET);
		__raw_writel(reg, MXC_CRMAP_ACDR);
                break;
        case USB_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
                d = (div <= 1) ? 15 : (div - 2);
                if (div > 6) {
                        d = d - ((div - 6) / 2);
                }
                reg = (reg & (~MXC_CRMAP_ACDER2_USBDIV_MASK)) |
                      (d << MXC_CRMAP_ACDER2_USBDIV_OFFSET);
		__raw_writel(reg, MXC_CRMAP_ACDER2);
                break;
        case UART1_BAUD:
        case UART2_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
                d = (div <= 1) ? 15 : (div - 2);
                if (div > 6) {
                        d = d - ((div - 6) / 2);
                }
                reg = (reg & (~MXC_CRMAP_ACDER2_BAUDDIV_MASK)) | d;
		__raw_writel(reg, MXC_CRMAP_ACDER2);
                break;
        case UART3_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
                d = (div <= 1) ? 15 : (div - 2);
                if (div > 6) {
                        d = d - ((div - 6) / 2);
                }
                reg = (reg & (~MXC_CRMAP_APRA_UART3DIV_MASK)) |
                      (d << MXC_CRMAP_APRA_UART3DIV_OFFSET);
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case SSI1_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
                d = (div == 62 * 2) ? 0 : div;
                reg = (reg & (~MXC_CRMAP_ACDER1_SSI1DIV_MASK)) |
                      (d << MXC_CRMAP_ACDER1_SSI1DIV_OFFSET);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
                break;
        case SSI2_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
                d = (div == 62 * 2) ? 0 : div;
                reg = (reg & (~MXC_CRMAP_ACDER1_SSI2DIV_MASK)) |
                      (d << MXC_CRMAP_ACDER1_SSI2DIV_OFFSET);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
                break;
        case CSI_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
                if (system_rev == CHIP_REV_1_0) {
                        div = div / 2;
                        d = (div <= 1) ? 15 : (div - 2);
                        if (div > 6) {
                                d = d - ((div - 6) / 2);
                        }
                        reg = (reg & (~MXC_CRMAP_ACDER1_CSIDIV_MASK)) |
                              (d << MXC_CRMAP_ACDER1_CSIDIV_OFFSET);
                } else {
                        d = (div == 62 * 2) ? 0 : div;
                        reg = (reg & (~MXC_CRMAP_ACDER1_CSIDIV_MASK_PASS2)) |
                              (d << MXC_CRMAP_ACDER1_CSIDIV_OFFSET);
                }
		__raw_writel(reg, MXC_CRMAP_ACDER1);
                break;
        case MQSPI_CKIH_CLK:
        case EL1T_NET_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
                d = div - 2;
                if (div > 6) {
                        d = d - ((div - 6) / 2);
                }
                reg = (reg & (~MXC_CRMAP_ACDER2_CRCT_CLK_DIV_MSK_PASS2)) |
                      (d << MXC_CRMAP_ACDER2_CRCT_CLK_DIV_OFF_PASS2);
		__raw_writel(reg, MXC_CRMAP_ACDER2);
                break;
        case SDHC1_CLK:
		reg = __raw_readl(MXC_CRMAP_APRB);
                d = (div <= 1) ? 15 : (div - 2);
                if (div > 6) {
                        d = d - ((div - 6) / 2);
                }
                reg = (reg & (~MXC_CRMAP_APRB_SDHC1_DIV_MASK_PASS2)) |
                      (d << MXC_CRMAP_APRB_SDHC1_DIV_OFFSET_PASS2);
		__raw_writel(reg, MXC_CRMAP_APRB);
                break;
        case SDHC2_CLK:
		reg = __raw_readl(MXC_CRMAP_APRB);
                d = (div <= 1) ? 15 : (div - 2);
                if (div > 6) {
                        d = d - ((div - 6) / 2);
                }
                reg = (reg & (~MXC_CRMAP_APRB_SDHC2_DIV_MASK_PASS2)) |
                      (d << MXC_CRMAP_APRB_SDHC2_DIV_OFFSET_PASS2);
		__raw_writel(reg, MXC_CRMAP_APRB);
                break;
        default:
                break;
        }

        spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function sets the division factor for a clock. Function checks for differences
 * between MXC91231 Pass 1.0 and Pass 2.0
 *
 * @param clk as defined in enum mxc_clocks
 * @param div the division factor to be used for the clock (For SSI & CSI, pass in
 *            2 times the expected division value to account for FP vals)
 */
void mxc_set_clocks_div(enum mxc_clocks clk, unsigned int div)
{
        if (system_rev == CHIP_REV_1_0) {
                switch (clk) {
                case SDHC1_CLK:
                case SDHC2_CLK:
                case MQSPI_CKIH_CLK:
                case EL1T_NET_CLK:
			printk
			    ("This clock does not have ability to set the divider\n");
                        return;
                default:
                        mxc_set_clocks_div_common(clk, div);
                        break;
                }
        } else {
                mxc_set_clocks_div_common(clk, div);
        }
}

/*!
 * This function sets the digital frequency multiplier clock.
 *
 * @param       freq    Desired DFM output frequency in Hz
 *
 * @return              Actual DFM frequency in Hz
 */
unsigned long mxc_set_dfm_clock(unsigned int freq)
{
	u32 mult;
	u32 reg;
	int timeout;
	unsigned long act_freq = 0;

	if (system_rev == CHIP_REV_1_0) {
		return act_freq;	// Rev 1 doesn't support DFM
	}

	/* Stop the DFM */
	__raw_writel(MXC_CRMAP_ADFMR_DFM_PWR_DOWN, MXC_CRMAP_ADFMR);

	if (freq != 0) {
		/* Calculate multiplier value */
		mult = freq / 32768L;
		if (mult > 1023) {
			mult = 1023;
		} else if (mult < 128) {
			mult = 128;
		}

		reg = 0x10 << MXC_CRMAP_ADFMR_FC_OFFSET;	/* magic value! */
		reg |= mult << MXC_CRMAP_ADFMR_MF_OFFSET;
		__raw_writel(reg, MXC_CRMAP_ADFMR);

		/* Wait for clock to be ready */
		timeout = 10;
		while (((__raw_readl(MXC_CRMAP_ADFMR) &
			 MXC_CRMAP_ADFMR_DFM_CLK_READY) == 0) && timeout) {
			udelay(100);
			timeout--;
		}

		/* Calc actual frequency */
		act_freq = 32768L * mult;
	}

	return act_freq;
}

/*!
 * This function returns the DFS block divider - LFDF value
 *
 * @return      Low Voltage frequency Divider Factor value
 */
unsigned int mxc_get_lfdf_value(void)
{
	volatile unsigned long reg = __raw_readl(MXC_CRMAP_ADCR);
        unsigned long lfdf = (reg & MXC_CRMAP_ADCR_LFDF_MASK) >> 
                              MXC_CRMAP_ADCR_LFDF_OFFSET;
        unsigned int dfs_en = reg & MXC_CRMAP_ADCR_DFS_DIVEN; 
        unsigned int alt_pll = reg & MXC_CRMAP_ADCR_ALT_PLL;

        if ((dfs_en > 1) && (alt_pll == 0)) {
                if (lfdf == 0) {
                        lfdf = 1;
                } else if (lfdf == 1 || lfdf == 2) {
                        lfdf = 2 * lfdf;
                } else {
                        lfdf = 8;
                }
        } else {
                lfdf = 1;
        }
        return lfdf;
}

/*!
 * Configure clock output on CKO/CKOH pins
 *
 * @param   opt     The desired clock needed to measure. Possible
 *                  values are, CKOH_AP_SEL, CKOH_AHB_SEL or CKOH_IP_SEL
 *
 */
void mxc_set_clock_output(enum mxc_clk_out output, enum mxc_clocks clk, int div)
{
	unsigned long flags;
	const int div_value[] = { 8, 8, 0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 7 };
	u32 reg = __raw_readl(MXC_CRMAP_ACR);

	spin_lock_irqsave(&mxc_crm_lock, flags);

	/* Select AP CKOH */
	if (output == CKOH) {
		reg &= ~(MXC_CRMAP_ACR_CKOHS_HIGH |
			 MXC_CRMAP_ACR_CKOHS_MASK |
			 MXC_CRMAP_ACR_CKOHDIV_MASK | MXC_CRMAP_ACR_CKOHD);
		__raw_writel(__raw_readl(MXC_CRMCOM_CSCR) |
			     MXC_CRMCOM_CSCR_CKOHSEL, MXC_CRMCOM_CSCR);
	} else {
		reg &= ~(MXC_CRMAP_ACR_CKOS_HIGH |
			 MXC_CRMAP_ACR_CKOS_MASK | MXC_CRMAP_ACR_CKOD);
		__raw_writel(__raw_readl(MXC_CRMCOM_CSCR) |
			     MXC_CRMCOM_CSCR_CKOSEL, MXC_CRMCOM_CSCR);
	}

	switch (clk) {
	case CLK_NONE:
		if (output == CKOH) {
			reg |= MXC_CRMAP_ACR_CKOHD;
		} else {
			reg |= MXC_CRMAP_ACR_CKOD;
		}
	case CPU_CLK:
		/* To select AP clock */
		if (output == CKOH) {
			reg = 0x00001000 |
			    div_value[div] << MXC_CRMAP_ACR_CKOHDIV_OFFSET;
		}
		break;
	case AHB_CLK:
		/* To select AHB clock */
		if (output == CKOH) {
			reg = 0x00002000 |
			    div_value[div] << MXC_CRMAP_ACR_CKOHDIV_OFFSET;
		}
		break;

	case IPG_CLK:
		/* To select IP clock */
		if (output == CKOH) {
			reg = 0x00003000 |
			    div_value[div] << MXC_CRMAP_ACR_CKOHDIV_OFFSET;
		}
		break;
	default:
		break;
	}

	__raw_writel(reg, MXC_CRMAP_ACR);
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is called to gate off the individual module clocks
 *
 * @param clks     as defined in enum mxc_clocks
 */
void mxc_clks_disable(enum mxc_clocks clks) 
{
        unsigned long flags;
        volatile unsigned long reg;
        
        spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (clks) {
        case UART1_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg &= ~MXC_CRMAP_APRA_UART1EN;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case UART2_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg &= ~MXC_CRMAP_APRA_UART2EN;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case UART3_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg &= ~MXC_CRMAP_APRA_UART3EN;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
#ifdef CONFIG_MOT_FEAT_PM
        case SSI1_IPG_CLK:
#else
        case SSI1_BAUD:
#endif
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg &= ~MXC_CRMAP_AMLPMRA_MLPMA4_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
#ifdef CONFIG_MOT_FEAT_PM
                break;
        case SSI1_BAUD:
#endif
		reg = __raw_readl(MXC_CRMAP_ACDER1);
                reg &= ~MXC_CRMAP_ACDER1_SSI1EN;
		__raw_writel(reg, MXC_CRMAP_ACDER1);
                break;
#ifdef CONFIG_MOT_FEAT_PM
        case SSI2_IPG_CLK:
#else
        case SSI2_BAUD:
#endif
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg &= ~MXC_CRMAP_AMLPMRE1_MLPME3_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
#ifdef CONFIG_MOT_FEAT_PM
                break;
        case SSI2_BAUD:
#endif
		reg = __raw_readl(MXC_CRMAP_ACDER1);
                reg &= ~MXC_CRMAP_ACDER1_SSI2EN;
		__raw_writel(reg, MXC_CRMAP_ACDER1);
                break; 
        case CSI_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
                if (system_rev == CHIP_REV_1_0) {
                        reg &= ~MXC_CRMAP_ACDER1_CSIEN;
                } else {
                        reg &= ~MXC_CRMAP_ACDER1_CSIEN_PASS2;
                }
		__raw_writel(reg, MXC_CRMAP_ACDER1);
                break;
        case NFC_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
                reg &= ~MXC_CRMAP_ACDER2_NFCEN;
		__raw_writel(reg, MXC_CRMAP_ACDER2);
                break;
        case I2C_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg &= ~MXC_CRMAP_AMLPMRA_MLPMA6_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
                break;
        case MU_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg &= ~MXC_CRMAP_AMLPMRA_MLPMA2_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
                break;
        case CSPI1_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE2);
                reg &= ~MXC_CRMAP_AMLPMRE2_MLPME0_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE2);
                break;
        case CSPI2_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg &= ~MXC_CRMAP_AMLPMRE1_MLPME6_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
                break;
#ifdef CONFIG_MOT_FEAT_PM
        case USB_IPG_CLK:
#else
        case USB_CLK:
#endif
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg &= ~MXC_CRMAP_AMLPMRE1_MLPME7_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
#ifdef CONFIG_MOT_FEAT_PM
                break;
        case USB_AHB_CLK:
#endif
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg &= ~MXC_CRMAP_AMLPMRG_MLPMG5_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
#ifdef CONFIG_MOT_FEAT_PM
                break;
        case USB_CLK:
#endif
		reg = __raw_readl(MXC_CRMAP_ACDER2);
                reg &= ~MXC_CRMAP_ACDER2_USBEN;
		__raw_writel(reg, MXC_CRMAP_ACDER2);
                break;
        case SDHC1_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg &= ~MXC_CRMAP_AMLPMRE1_MLPME4_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
		reg = __raw_readl(MXC_CRMAP_APRB);
                reg &= ~MXC_CRMAP_APRB_SDHC1EN;
		__raw_writel(reg, MXC_CRMAP_APRB);
                break;
        case SDHC2_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg &= ~MXC_CRMAP_AMLPMRE1_MLPME5_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
		reg = __raw_readl(MXC_CRMAP_APRB);
                reg &= ~MXC_CRMAP_APRB_SDHC2EN;
		__raw_writel(reg, MXC_CRMAP_APRB);
                break;
        case SPBA_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg &= ~MXC_CRMAP_AMLPMRC_MLPMC0_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case SDMA_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg &= ~MXC_CRMAP_AMLPMRC_MLPMC2_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg &= ~MXC_CRMAP_AMLPMRG_MLPMG2_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case EDIO_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg &= ~MXC_CRMAP_AMLPMRC_MLPMC3_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case RTC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg &= ~MXC_CRMAP_AMLPMRC_MLPMC1_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case EPIT1_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg &= ~MXC_CRMAP_AMLPMRC_MLPMC5_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case GPT_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg &= ~MXC_CRMAP_AMLPMRC_MLPMC4_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case PWM_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg &= ~MXC_CRMAP_AMLPMRC_MLPMC7_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case WDOG_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRD);
                reg &= ~MXC_CRMAP_AMLPMRD_MLPMD7_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRD);
                break;
        case RNG_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRD);
                reg &= ~MXC_CRMAP_AMLPMRD_MLPMD0_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRD);
                break;
        case SCC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRD);
		/* SCC Clock can turn off only in STOP and Wait Modes */
		reg = (reg & (~MXC_CRMAP_AMLPMRD_MLPMD2_MASK)) |
		    (0x1 << MXC_CRMAP_AMLPMRD_MLPMD2_OFFSET);
		__raw_writel(reg, MXC_CRMAP_AMLPMRD);
		break;
	case IPU_CLK:
		if (system_rev == CHIP_REV_1_0) {
			reg = __raw_readl(MXC_CRMAP_AMLPMRC);
			reg &= ~MXC_CRMAP_AMLPMRC_MLPMC9_MASK;
			__raw_writel(reg, MXC_CRMAP_AMLPMRC);
		}
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg &= ~MXC_CRMAP_AMLPMRG_MLPMG3_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
	case LPMC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
		reg &= ~MXC_CRMAP_AMLPMRC_MLPMC9_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
		break;
	case KPP_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg &= ~MXC_CRMAP_AMLPMRA_MLPMA3_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
                break;
        case RTIC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg &= ~MXC_CRMAP_AMLPMRG_MLPMG0_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case GEM_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg &= ~MXC_CRMAP_AMLPMRG_MLPMG1_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case HAC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg &= ~MXC_CRMAP_AMLPMRG_MLPMG7_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case DSM_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg &= ~MXC_CRMAP_AMLPMRA_MLPMA1_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
                break;
        case SIM1_CLK:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg &= ~MXC_CRMAP_APRA_SIMEN;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case WDOG2_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRD);
                reg &= ~MXC_CRMAP_AMLPMRD_MLPMD3_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRD);
                break;
        case SAHARA2_AHB_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg &= ~MXC_CRMAP_AMLPMRG_MLPMG7_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case SAHARA2_IPG_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg &= ~MXC_CRMAP_AMLPMRE1_MLPME2_MASK_PASS2;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
                break;
        case MQSPI_IPG_CLK:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg &= ~MXC_CRMAP_APRA_MQSPI_EN_PASS2;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case EL1T_IPG_CLK:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg &= ~MXC_CRMAP_APRA_EL1T_EN_PASS2;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case CPU_CLK:
        case IPG_CLK:
        case AHB_CLK:
        case MQSPI_CKIH_CLK:
        case EL1T_NET_CLK:
	default:
                break;
        }

        spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function is called to enable the individual module clocks
 *
 * @param clks     as defined in enum mxc_clocks
 */
void mxc_clks_enable(enum mxc_clocks clks)
{
        unsigned long flags;
        volatile unsigned long reg;

        spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (clks) {
        case UART1_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg |= MXC_CRMAP_APRA_UART1EN;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case UART2_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg |= MXC_CRMAP_APRA_UART2EN;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case UART3_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg |= MXC_CRMAP_APRA_UART3EN;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
#ifdef CONFIG_MOT_FEAT_PM
        case SSI1_IPG_CLK:
#else
        case SSI1_BAUD:
#endif
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg |= MXC_CRMAP_AMLPMRA_MLPMA4_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
#ifdef CONFIG_MOT_FEAT_PM
                break;
        case SSI1_BAUD:
#endif
		reg = __raw_readl(MXC_CRMAP_ACDER1);
                reg |= MXC_CRMAP_ACDER1_SSI1EN;
		__raw_writel(reg, MXC_CRMAP_ACDER1);
                break;
#ifdef CONFIG_MOT_FEAT_PM
        case SSI2_IPG_CLK:
#else
        case SSI2_BAUD:
#endif
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg |= MXC_CRMAP_AMLPMRE1_MLPME3_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
#ifdef CONFIG_MOT_FEAT_PM
                break;
        case SSI2_BAUD:
#endif
		reg = __raw_readl(MXC_CRMAP_ACDER1);
                reg |= MXC_CRMAP_ACDER1_SSI2EN;
		__raw_writel(reg, MXC_CRMAP_ACDER1);
                break; 
        case CSI_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
                if (system_rev == CHIP_REV_1_0) {
                        reg |= MXC_CRMAP_ACDER1_CSIEN;
                } else {
                        reg |= MXC_CRMAP_ACDER1_CSIEN_PASS2;
                }
		__raw_writel(reg, MXC_CRMAP_ACDER1);
                break;
        case NFC_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
                reg |= MXC_CRMAP_ACDER2_NFCEN;
		__raw_writel(reg, MXC_CRMAP_ACDER2);
                break;
        case I2C_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg |= MXC_CRMAP_AMLPMRA_MLPMA6_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
                break;
        case MU_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg |= MXC_CRMAP_AMLPMRA_MLPMA2_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
                break;
        case CSPI1_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE2);
                reg |= MXC_CRMAP_AMLPMRE2_MLPME0_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE2);
                break;
        case CSPI2_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg |= MXC_CRMAP_AMLPMRE1_MLPME6_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
                break;
#ifdef CONFIG_MOT_FEAT_PM
        case USB_IPG_CLK:
#else
        case USB_CLK:
#endif
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg |= MXC_CRMAP_AMLPMRE1_MLPME7_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
#ifdef CONFIG_MOT_FEAT_PM
                break;
        case USB_AHB_CLK:
#endif
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg |= MXC_CRMAP_AMLPMRG_MLPMG5_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
#ifdef CONFIG_MOT_FEAT_PM
                break;
        case USB_CLK:
#endif
		reg = __raw_readl(MXC_CRMAP_ACDER2);
                reg |= MXC_CRMAP_ACDER2_USBEN;
		__raw_writel(reg, MXC_CRMAP_ACDER2);
                break;
        case SDHC1_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg |= MXC_CRMAP_AMLPMRE1_MLPME4_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
		reg = __raw_readl(MXC_CRMAP_APRB);
                reg |= MXC_CRMAP_APRB_SDHC1EN;
		__raw_writel(reg, MXC_CRMAP_APRB);
                break;
        case SDHC2_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg |= MXC_CRMAP_AMLPMRE1_MLPME5_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
		reg = __raw_readl(MXC_CRMAP_APRB);
                reg |= MXC_CRMAP_APRB_SDHC2EN;
		__raw_writel(reg, MXC_CRMAP_APRB);
                break;
        case SPBA_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg |= MXC_CRMAP_AMLPMRC_MLPMC0_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case SDMA_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg |= MXC_CRMAP_AMLPMRC_MLPMC2_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg |= MXC_CRMAP_AMLPMRG_MLPMG2_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case EDIO_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg |= MXC_CRMAP_AMLPMRC_MLPMC3_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case RTC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg |= MXC_CRMAP_AMLPMRC_MLPMC1_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case EPIT1_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg |= MXC_CRMAP_AMLPMRC_MLPMC5_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case GPT_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg |= MXC_CRMAP_AMLPMRC_MLPMC4_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case PWM_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                reg |= MXC_CRMAP_AMLPMRC_MLPMC7_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                break;
        case WDOG_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRD);
                reg |= MXC_CRMAP_AMLPMRD_MLPMD7_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRD);
                break;
        case RNG_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRD);
                reg |= MXC_CRMAP_AMLPMRD_MLPMD0_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRD);
                break;
        case SCC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRD);
                reg |= MXC_CRMAP_AMLPMRD_MLPMD2_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRD);
                break;
        case IPU_CLK:
                if (system_rev == CHIP_REV_1_0) {
			reg = __raw_readl(MXC_CRMAP_AMLPMRC);
                        reg |= MXC_CRMAP_AMLPMRC_MLPMC9_MASK;
			__raw_writel(reg, MXC_CRMAP_AMLPMRC);
                }
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg |= MXC_CRMAP_AMLPMRG_MLPMG3_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
	case LPMC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRC);
		reg |= MXC_CRMAP_AMLPMRC_MLPMC9_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRC);
		break;
	case KPP_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg |= MXC_CRMAP_AMLPMRA_MLPMA3_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
                break;
        case RTIC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg |= MXC_CRMAP_AMLPMRG_MLPMG0_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case GEM_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg |= MXC_CRMAP_AMLPMRG_MLPMG1_MASK; 
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case HAC_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg |= MXC_CRMAP_AMLPMRG_MLPMG7_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case DSM_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRA);
                reg |= MXC_CRMAP_AMLPMRA_MLPMA1_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRA);
                break;
        case SIM1_CLK:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg |= MXC_CRMAP_APRA_SIMEN;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case WDOG2_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRD);
                reg |= MXC_CRMAP_AMLPMRD_MLPMD3_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRD);
                break;
        case SAHARA2_AHB_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
                reg |= MXC_CRMAP_AMLPMRG_MLPMG7_MASK;
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
                break;
        case SAHARA2_IPG_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRE1);
                reg |= MXC_CRMAP_AMLPMRE1_MLPME2_MASK_PASS2;
		__raw_writel(reg, MXC_CRMAP_AMLPMRE1);
                break;
        case MQSPI_IPG_CLK:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg |= MXC_CRMAP_APRA_MQSPI_EN_PASS2;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case EL1T_IPG_CLK:
		reg = __raw_readl(MXC_CRMAP_APRA);
                reg |= MXC_CRMAP_APRA_EL1T_EN_PASS2;
		__raw_writel(reg, MXC_CRMAP_APRA);
                break;
        case CPU_CLK:
        case IPG_CLK:
        case AHB_CLK:
        case MQSPI_CKIH_CLK:
        case EL1T_NET_CLK:
	default:
                break;
        }
        spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

#if defined(CONFIG_MOT_FEAT_GPIO_API_ETM)
/**
 * Adjust clock settings so that ETM trigger clock behaves properly.
 */
void etm_enable_trigger_clock(void)
{
    unsigned long flags;
    volatile unsigned long reg;

    spin_lock_irqsave(&mxc_crm_lock, flags);

    reg = __raw_readl(MXC_CRMAP_AGPR);
    reg = (reg & (~MXC_CRMAP_AGPR_IPUPAD_MASK)) |
          (0x7 << MXC_CRMAP_AGPR_IPUPAD_OFFSET);
    __raw_writel(reg, MXC_CRMAP_AGPR);

    spin_unlock_irqrestore(&mxc_crm_lock, flags);
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_ETM */

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
	unsigned long flags;
	unsigned long pllreg;

	/* Ensure we have a legal value. */
	if (pllnum >= NUM_MOT_PLLS)
		return -1;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (pllnum) {
	case MCUPLL: /* PLL0 */
		pllreg = PLL0_DP_CTL;
		break;

	case DSPPLL: /* PLL1 */
		pllreg = PLL1_DP_CTL;
		break;

	case USBPLL: /* PLL2 */
		pllreg = PLL2_DP_CTL;
		break;

	default: /* should have already checked above */
		spin_unlock_irqrestore(&mxc_crm_lock, flags);
		return -1;
	}

	/*
	 * Normally, we want to restart and relock the PLL only
	 * when it is currently unused, but when we request the
	 * MCUPLL, we always want to restart and relock it.  We have
	 * issues (the USB connection drops every so often) when we
	 * restart and relock the USBPLL when it is in use.
	 */
	if ((pllnum == MCUPLL) || (pll_requests[pllnum] == 0)) {
		__raw_writel(__raw_readl(pllreg) | MXC_PLL_DP_CTL_RST | MXC_PLL_DP_CTL_UPEN, pllreg);
		while ((__raw_readl(pllreg) & MXC_PLL_DP_CTL_LRF) != MXC_PLL_DP_CTL_LRF);
	}

	pll_requests[pllnum]++;

	spin_unlock_irqrestore(&mxc_crm_lock, flags);
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
	unsigned long flags;
	unsigned long pllreg;

	/* Ensure we have a legal value. */
	if (pllnum >= NUM_MOT_PLLS)
		return -1;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	pll_requests[pllnum]--;

	/* In the initialization cases, we may be running with PLL already
	 * locked, but the pll_requests[] are initialized to zero, so we
	 * want to go ahead and disable the PLL, even though it looks like
	 * we should not have to.
	 */
	if (pll_requests[pllnum] <= 0) {
		switch (pllnum) {
		case MCUPLL: /* PLL0 */
			pllreg = PLL0_DP_CTL;
			break;

		case DSPPLL: /* PLL1 */
			pllreg = PLL1_DP_CTL;
			break;

		case USBPLL: /* PLL2 */
			pllreg = PLL2_DP_CTL;
			break;

		default: /* should have already checked above */
			spin_unlock_irqrestore(&mxc_crm_lock, flags);
			return -1;
		}

		__raw_writel(__raw_readl(pllreg) & ~MXC_PLL_DP_CTL_RST & ~MXC_PLL_DP_CTL_UPEN, pllreg);
	}

	if (pll_requests[pllnum] < 0)
	{
		pll_requests[pllnum] = 0;
		spin_unlock_irqrestore(&mxc_crm_lock, flags);
		return -2;
	}

	spin_unlock_irqrestore(&mxc_crm_lock, flags);
	return 0;
}
#endif /* CONFIG_MOT_FEAT_PM */

EXPORT_SYMBOL(mxc_pll_set);
EXPORT_SYMBOL(mxc_pll_get);
EXPORT_SYMBOL(mxc_pll_clock);
EXPORT_SYMBOL(mxc_get_clocks);
EXPORT_SYMBOL(mxc_get_clocks_parent);
EXPORT_SYMBOL(mxc_set_clocks_pll);
EXPORT_SYMBOL(mxc_set_clocks_div);
EXPORT_SYMBOL(mxc_get_ref_clk);
EXPORT_SYMBOL(mxc_main_clock_divider);
EXPORT_SYMBOL(mxc_peri_clock_divider);
EXPORT_SYMBOL(mxc_set_dfm_clock);
EXPORT_SYMBOL(mxc_get_lfdf_value);
EXPORT_SYMBOL(mxc_set_clock_output);
EXPORT_SYMBOL(mxc_clks_disable);
EXPORT_SYMBOL(mxc_clks_enable);
#ifdef CONFIG_MOT_FEAT_PM
EXPORT_SYMBOL(mxc_pll_request_pll);
EXPORT_SYMBOL(mxc_pll_release_pll);
#endif /* CONFIG_MOT_FEAT_PM */
