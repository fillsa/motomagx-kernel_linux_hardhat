/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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

#include <linux/config.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/arch/clock.h>

#include "crm_regs.h"

/*
 * Define reference clock inputs
 */
#define CKIL_CLK_FREQ               32768
#define CKIH_CLK_FREQ               16800000
#define DIGRF_CLK_FREQ              26000000

/*!
 * Spinlock to protect CRM register accesses
 */
static DEFINE_RAW_SPINLOCK(mxc_crm_lock);

//static volatile unsigned char *g_pcrm_ap = (volatile unsigned char *)
//    (IO_ADDRESS(CRM_AP_BASE_ADDR));

static unsigned char *g_pdpll[] = {
	(unsigned char *)(IO_ADDRESS(ADPLL_BASE_ADDR)),
	(unsigned char *)(IO_ADDRESS(BDPLL_BASE_ADDR)),
	(unsigned char *)(IO_ADDRESS(UDPLL_BASE_ADDR)),
};

enum clk_div {
	ARM_DIV = 0,
	USB_DIV = 0,
	BAUD_DIV = 0,
	UART3_DIV = 0,
	CKOH_DIV = 0,
	SDHC1_DIV = 0,
	SDHC2_DIV = 0,
	AHB_DIV = 1,
	IP_DIV = 1,
	NFC_DIV = 2,
};

enum clk_sel {
	ADPLL_SEL = 0,
	BDPLL_SEL = 1,
	UDPLL_SEL = 2,
	MRCG_SEL = 3,
};

enum mrcg_clk {
	AP_MRCG = 0,
	CS_MRCG = 0,
	SSI2_MRCG = 1,
	SSI1_MRCG = 2,
	USB_MRCG = 2,
	FIRI_MRCG = 2,
	SDHC_MRCG = 2,
};

/* Table for 4-bit CRM dividers:
 *      g_CLK_DIV[0]:  ARM_DIV, USB_DIV, BAUD_DIV, CKOHDIV, UART3_DIV, SDHC2_DIV, SDCH1_DIV
 *      g_CLK_DIV[1]:  AHB_DIV, IP_DIV
 *      g_CLK_DIV[2]:  NFC_DIV
 */
static unsigned int g_CLK_DIV[3][16] = {
	{2, 3, 4, 5, 6, 8, 10, 12, 1, 1, 1, 1, 1, 1, 1, 1},
	{16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
};

#define BFMASK(bit) (((1U << (bit ## _BFW)) - 1) << (bit ## _BFS))
#define BFVALUE(bit, val) ((val) << (bit ## _BFS))
#define BFEXTRACT(var, bit) ((var & BFMASK(bit)) >> (bit ## _BFS))
#define BFCLEAR(var, bit) (var &= (~(BFMASK(bit))))
#define BFINSERT(var, bit, val) (BFCLEAR(var, bit)); (var |= BFVALUE(bit, val))

static unsigned long mxc_get_ahb_div(void)
{
	unsigned long acdr = __raw_readl(MXC_CRMAP_ACDR);

	return g_CLK_DIV[AHB_DIV][BFEXTRACT(acdr, ACDR_AHB_DIV)];
}

static unsigned long mxc_get_ip_div(void)
{
	unsigned long acdr = __raw_readl(MXC_CRMAP_ACDR);

	return g_CLK_DIV[IP_DIV][BFEXTRACT(acdr, ACDR_IP_DIV)];
}

static unsigned long mxc_get_nfc_div(void)
{
	unsigned long acder2 = __raw_readl(MXC_CRMAP_ACDER2);

	return mxc_get_ahb_div() *
	    g_CLK_DIV[NFC_DIV][BFEXTRACT(acder2, ACDER2_NFC_DIV)];
}

/*!
 * This function returns the PLL output value in Hz based on pll.
 * @param       pll     PLL as defined in enum plls
 * @return      PLL value in Hz.
 */
unsigned long mxc_pll_clock(enum plls pll)
{
	unsigned char *pdpll = g_pdpll[pll];
	signed long mfi, mfn, mfd, pdf, ref_clk, pll_out;
	unsigned long dp_ctl, dp_op, dp_mfd, dp_mfn;

	dp_ctl = __raw_readl(pdpll + DPLL_DP_CTL);

	if (BFEXTRACT(dp_ctl, DP_CTL_HFSM) == DP_CTL_HFSM_NORMAL_MODE) {
		dp_op = __raw_readl(pdpll + DPLL_DP_OP);
		dp_mfd = __raw_readl(pdpll + DPLL_DP_MFD);
		dp_mfn = __raw_readl(pdpll + DPLL_DP_MFN);
	} else {
		dp_op = __raw_readl(pdpll + DPLL_DP_HFS_OP);
		dp_mfd = __raw_readl(pdpll + DPLL_DP_HFS_MFD);
		dp_mfn = __raw_readl(pdpll + DPLL_DP_HFS_MFN);
	}

	pdf = (signed long)BFEXTRACT(dp_op, DP_OP_PDF);
	mfi = (signed long)BFEXTRACT(dp_op, DP_OP_MFI);
	mfi = (mfi <= 5) ? 5 : mfi;
	mfd = (signed long)(dp_mfd & BFMASK(DP_MFD_MFD));
	mfn = (signed long)(dp_mfn & BFMASK(DP_MFN_MFN));
	mfn = (mfn <= 0x4000000) ? mfn : (mfn - 0x10000000);

	switch (BFEXTRACT(dp_ctl, DP_CTL_REF_CLK_SEL)) {

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

	pll_out = (2 * ref_clk * mfi + ((2 * ref_clk / (mfd + 1)) * mfn)) /
	    (pdf + 1);

	return pll_out;
}

static unsigned long mxc_get_mrcg_clk(enum mrcg_clk mrcg)
{
	/* TODO:  need to implement MRCG clocking */
	return 0;
}

static unsigned long mxc_get_high_speed_clk(unsigned int sel,
					    enum mrcg_clk mrcg)
{
	unsigned long clk = 0;

	switch (sel) {

	case ADPLL_SEL:
		clk = mxc_pll_clock(MCUPLL);
		break;

	case UDPLL_SEL:
		clk = mxc_pll_clock(USBPLL);
		break;

	case BDPLL_SEL:
		clk = mxc_pll_clock(DSPPLL);
		break;

	case MRCG_SEL:
		clk = mxc_get_mrcg_clk(mrcg);
		break;
	}

	return clk;
}

static unsigned long mxc_get_ungated_ap_clk(void)
{
	unsigned long clk = 0;
	unsigned long acsr, ascsr, adcr, acdr;

	acsr = __raw_readl(MXC_CRMAP_ACSR);
	ascsr = __raw_readl(MXC_CRMAP_ASCSR);

	/* If ARM core clock is ref_core_clk */
	if (BFEXTRACT(acsr, ACSR_ACS) == ACSR_ACS_REF_CORE_CLK) {

		/* If ap_ref_clk path selected */
		if (BFEXTRACT(ascsr, ACSR_ADS) == ACSR_ADS_NO_DOUBLER) {

			if (BFEXTRACT(ascsr, ASCSR_CRS) ==
			    ASCSR_CRS_UNCORRECTED) {
				clk = CKIH_CLK_FREQ;
			} else {
				if (BFEXTRACT(ascsr, ASCSR_AP_ISEL) ==
				    ASCSR_AP_ISEL_CKIH) {
					clk = CKIH_CLK_FREQ;
				} else {
					clk = DIGRF_CLK_FREQ;
				}
			}
			if (BFEXTRACT(ascsr, ASCSR_AP_PAT_REF_DIV) ==
			    ASCSR_AP_PAT_REF_DIV_2) {
				clk >>= 1;
			}
		}

		/* Else, relf_clkx2 path selected */
		else {
			if (BFEXTRACT(ascsr, ASCSR_AP_ISEL) ==
			    ASCSR_AP_ISEL_CKIH) {
				clk = (CKIH_CLK_FREQ * 2);
			} else {
				clk = DIGRF_CLK_FREQ;
			}
		}
	}

	/* Else, PLL devived path selected */
	else {
		clk =
		    mxc_get_high_speed_clk(BFEXTRACT(ascsr, ASCSR_APSEL),
					   AP_MRCG);

		adcr = __raw_readl(MXC_CRMAP_ADCR);
		acdr = __raw_readl(MXC_CRMAP_ACDR);

		/*
		 * If DFS is active, that is if:
		 * We're at the low voltage (VSTAT=1) and DIV_EN = 1 and DFS_DIV_BYP = 0
		 */
		if ((BFEXTRACT(adcr, ADCR_DFS_DIV_EN) ==
		     ADCR_DFS_DIV_EN_INT_DIV)
		    && (BFEXTRACT(adcr, ADCR_VSTAT) == ADCR_VSTAT_LOW_VOLTAGE)
		    && (BFEXTRACT(adcr, ADCR_DIV_BYP) == ADCR_DIV_BYP_USED)) {
			/* Use DFS integer divide (1, 2, 4, 8) from LFDF bits */
			clk /= mxc_get_lfdf_value();
		}

		/* Apply ARM divide factor (ARM_DIV) */
		clk /= g_CLK_DIV[ARM_DIV][BFEXTRACT(acdr, ACDR_ARM_DIV)];

	}

	return clk;
}

static unsigned long mxc_get_usb_clk(void)
{
	unsigned long clk = 0;
	unsigned long ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	unsigned long acder2 = __raw_readl(MXC_CRMAP_ACDER2);

	if (BFEXTRACT(acder2, ACDER2_USB_EN)) {
		clk =
		    mxc_get_high_speed_clk(BFEXTRACT(ascsr, ASCSR_USBSEL),
					   USB_MRCG) /
		    g_CLK_DIV[USB_DIV][BFEXTRACT(acder2, ACDER2_USB_DIV)];
	}

	return clk;
}

static unsigned long mxc_get_ap_baudclk(unsigned int baud_isel)
{
	unsigned long clk = 0;

	switch (baud_isel) {

	case ACDER2_BAUD_ISEL_CKIH:
		clk = CKIH_CLK_FREQ;
		break;

	case ACDER2_BAUD_ISEL_CKIH_X2:
		clk = (CKIH_CLK_FREQ * 2);
		break;

	case ACDER2_BAUD_ISEL_DIGRF:
		clk = DIGRF_CLK_FREQ;
		break;

	case ACDER2_BAUD_ISEL_DIGRF_X2:
		clk = (DIGRF_CLK_FREQ * 2);
		break;
	}

	return clk;
}

static unsigned long mxc_get_sdhc_baudclk(unsigned int sdhc_isel)
{
	unsigned long clk = 0;

	switch (sdhc_isel) {

	case APRB_SDHC_ISEL_CKIH:
		clk = CKIH_CLK_FREQ;
		break;

	case APRB_SDHC_ISEL_CKIH_X2:
		clk = (CKIH_CLK_FREQ * 2);
		break;

	case APRB_SDHC_ISEL_DIGRF:
		clk = DIGRF_CLK_FREQ;
		break;

	case APRB_SDHC_ISEL_DIGRF_X2:
		clk = (DIGRF_CLK_FREQ * 2);
		break;

	case APRB_SDHC_ISEL_USB_CLK:
		clk = mxc_get_usb_clk();
		break;

	case APRB_SDHC_ISEL_MRCG2_CLK:
		clk = mxc_get_mrcg_clk(SDHC_MRCG);
		break;
	}

	return clk;
}
static unsigned long mxc_get_ap_perclk(void)
{
	unsigned long clk = 0;
	unsigned long acder2 = __raw_readl(MXC_CRMAP_ACDER2);

	clk = mxc_get_ap_baudclk(BFEXTRACT(acder2, ACDER2_BAUD_ISEL));

	return clk / g_CLK_DIV[BAUD_DIV][BFEXTRACT(acder2, ACDER2_BAUD_DIV)];
}

static unsigned long mxc_get_ap_uart3_perclk(void)
{
	unsigned long clk = 0;
	unsigned long acder2 = __raw_readl(MXC_CRMAP_ACDER2);
	unsigned long apra = __raw_readl(MXC_CRMAP_APRA);

	clk = mxc_get_ap_baudclk(BFEXTRACT(acder2, ACDER2_BAUD_ISEL));

	return clk / g_CLK_DIV[UART3_DIV][BFEXTRACT(apra, APRA_UART3_DIV)];
}

static unsigned long mxc_get_ssi1_clk(void)
{
	unsigned long clk = 0;
	unsigned long div;
	unsigned long ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	unsigned long acder1 = __raw_readl(MXC_CRMAP_ACDER1);

	if (BFEXTRACT(acder1, ACDER1_SSI1_EN)) {

		clk =
		    mxc_get_high_speed_clk(BFEXTRACT(ascsr, ASCSR_SSI1SEL),
					   SSI1_MRCG);
		div = BFEXTRACT(acder1, ACDER1_SSI1_DIV);

		/* Handle special cases for divider */
		if (div <= 1) {
			clk /= 62;
		} else {
			/* Use non-fractional divider (2x) to avoid floating point */
			clk <<= 1;
			clk /= div;
		}
	}

	return clk;
}

static unsigned long mxc_get_ssi2_clk(void)
{
	unsigned long clk = 0;
	unsigned long div;
	unsigned long ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	unsigned long acder1 = __raw_readl(MXC_CRMAP_ACDER1);

	if (BFEXTRACT(acder1, ACDER1_SSI2_EN)) {

		clk =
		    mxc_get_high_speed_clk(BFEXTRACT(ascsr, ASCSR_SSI2SEL),
					   SSI2_MRCG);
		div = BFEXTRACT(acder1, ACDER1_SSI2_DIV);

		/* Handle special cases for divider */
		if (div <= 1) {
			clk /= 62;
		} else {
			/* Use non-fractional divider (2x) to avoid floating point */
			clk <<= 1;
			clk /= div;
		}
	}

	return clk;
}

static unsigned long mxc_get_cs_clk(void)
{
	unsigned long clk = 0;
	unsigned long div;
	unsigned long ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	unsigned long acder1 = __raw_readl(MXC_CRMAP_ACDER1);

	if (BFEXTRACT(acder1, ACDER1_CS_EN)) {

		clk =
		    mxc_get_high_speed_clk(BFEXTRACT(ascsr, ASCSR_CSSEL),
					   CS_MRCG);
		div = BFEXTRACT(acder1, ACDER1_CS_DIV);

		/* Handle special cases for divider */
		if (div <= 1) {
			clk /= 62;
		} else {
			/* Use non-fractional divider (2x) to avoid floating point */
			clk <<= 1;
			clk /= div;
		}
	}

	return clk;
}

static unsigned long mxc_get_cs_clk_parent(void)
{
	unsigned long clk = 0;
	unsigned long ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	unsigned long acder1 = __raw_readl(MXC_CRMAP_ACDER1);

	if (BFEXTRACT(acder1, ACDER1_CS_EN)) {

		clk =
		    mxc_get_high_speed_clk(BFEXTRACT(ascsr, ASCSR_CSSEL),
					   CS_MRCG);
	}

	return clk;
}

static unsigned long mxc_get_firi_clk(void)
{
	unsigned long clk = 0;
	unsigned long div;
	unsigned long ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	unsigned long acder1 = __raw_readl(MXC_CRMAP_ACDER1);

	if (BFEXTRACT(acder1, ACDER1_FIRI_EN)) {
		clk =
		    mxc_get_high_speed_clk(BFEXTRACT(ascsr, ASCSR_FIRISEL),
					   FIRI_MRCG);
		div = BFEXTRACT(acder1, ACDER1_FIRI_DIV);
		clk = clk / (div + 1);
	}

	return clk;
}

static unsigned long mxc_get_ap_sdhc1_perclk(void)
{
	unsigned long clk = 0;
	unsigned long aprb = __raw_readl(MXC_CRMAP_APRB);

	clk = mxc_get_sdhc_baudclk(BFEXTRACT(aprb, APRB_SDHC1_ISEL));

	return clk / g_CLK_DIV[SDHC1_DIV][BFEXTRACT(aprb, APRB_SDHC1_DIV)];
}

static unsigned long mxc_get_ap_sdhc2_perclk(void)
{
	unsigned long clk = 0;
	unsigned long aprb = __raw_readl(MXC_CRMAP_APRB);

	clk = mxc_get_sdhc_baudclk(BFEXTRACT(aprb, APRB_SDHC2_ISEL));

	return clk / g_CLK_DIV[SDHC2_DIV][BFEXTRACT(aprb, APRB_SDHC2_DIV)];
}

/*!
 * This function returns the main clock value in Hz.
 *
 * @param       clk     as defined in enum main_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks(enum mxc_clocks clk)
{
	unsigned long mxc_clk = 0;

	switch (clk) {

	case CPU_CLK:
		mxc_clk = mxc_get_ungated_ap_clk();
		break;

	case AHB_CLK:
		mxc_clk = mxc_get_ungated_ap_clk() / mxc_get_ahb_div();
		break;

	case IPU_CLK:
		mxc_clk = mxc_get_ungated_ap_clk() / mxc_get_ahb_div();
		break;

	case NFC_CLK:
		mxc_clk = mxc_get_ungated_ap_clk() / mxc_get_nfc_div();
		break;

	case USB_CLK:
		mxc_clk = mxc_get_usb_clk();
		break;

	case UART1_BAUD:
	case UART2_BAUD:
	case SIM1_CLK:
		mxc_clk = mxc_get_ap_perclk();
		break;

	case UART3_BAUD:
		mxc_clk = mxc_get_ap_uart3_perclk();
		break;

	case SSI1_BAUD:
		mxc_clk = mxc_get_ssi1_clk();
		break;

	case SSI2_BAUD:
		mxc_clk = mxc_get_ssi2_clk();
		break;

	case CSI_BAUD:
		mxc_clk = mxc_get_cs_clk();
		break;

	case FIRI_BAUD:
		mxc_clk = mxc_get_firi_clk();
		break;

	case SDHC1_CLK:
		mxc_clk = mxc_get_ap_sdhc1_perclk();
		break;

	case SDHC2_CLK:
		mxc_clk = mxc_get_ap_sdhc2_perclk();
		break;

//    case CKIH_CLK:
//        mxc_clk = CKIH_CLK_FREQ;
//        break;

//    case CKIL_CLK:
	case WDOG_CLK:
		mxc_clk = CKIL_CLK_FREQ;
		break;

//    case DIGRF_CLK:
//        mxc_clk = DIGRF_CLK_FREQ;
//        break;

		/* All other cases will return IPG_CLK */
	default:
		mxc_clk = mxc_get_ungated_ap_clk() / mxc_get_ip_div();
		break;

	}

	return mxc_clk;
}

/*!
 * This function returns the parent clock value in Hz.
 *
 * @param       clk     as defined in enum main_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks_parent(enum mxc_clocks clk)
{
	unsigned long mxc_clk = 0;

	switch (clk) {

	case CSI_BAUD:
		mxc_clk = mxc_get_cs_clk_parent();
		break;
	default:
		break;
	}

	return mxc_clk;
}

/*!
 * This function returns the divider value for a clock.
 *
 * @param       clk as defined in enum mxc_clocks
 *
 * @return      divider value
 */
unsigned long mxc_get_clocks_div(enum mxc_clocks clk)
{
	unsigned long acdr, acder2, div = 0;

	switch (clk) {

	case CPU_CLK:
		acdr = __raw_readl(MXC_CRMAP_ACDR);
		div = g_CLK_DIV[ARM_DIV][BFEXTRACT(acdr, ACDR_ARM_DIV)];
		break;

	case AHB_CLK:
		div = mxc_get_ahb_div();
		break;

	case IPG_CLK:
		div = mxc_get_ip_div();
		break;

	case NFC_CLK:
		div = mxc_get_nfc_div();
		break;

	case USB_CLK:
		acder2 = __raw_readl(MXC_CRMAP_ACDER2);
		div = g_CLK_DIV[USB_DIV][BFEXTRACT(acder2, ACDER2_USB_DIV)];
		break;

	default:
		break;

	}

	return div;
}

/*!
 * This function sets the PLL source for a clock.
 *
 * @param clk     as defined in enum mxc_clocks
 * @param pll_num the PLL that you wish to use as source for this clock
 */
void mxc_set_clocks_pll(enum mxc_clocks clk, enum plls pll_num)
{
	unsigned long ascsr, flags;

	spin_lock_irqsave(&mxc_crm_lock, flags);
	ascsr = __raw_readl(MXC_CRMAP_ASCSR);
	switch (clk) {
	case CPU_CLK:
		BFINSERT(ascsr, ASCSR_APSEL, pll_num);
		break;
	case USB_CLK:
		BFINSERT(ascsr, ASCSR_USBSEL, pll_num);
		break;
	case SSI1_BAUD:
		BFINSERT(ascsr, ASCSR_SSI1SEL, pll_num);
		break;
	case SSI2_BAUD:
		BFINSERT(ascsr, ASCSR_SSI2SEL, pll_num);
		break;
	case CSI_BAUD:
		BFINSERT(ascsr, ASCSR_CSSEL, pll_num);
		break;
	case FIRI_BAUD:
		BFINSERT(ascsr, ASCSR_FIRISEL, pll_num);
		break;
	default:
		printk("This clock does not have ability to choose the PLL\n");
		break;
	}
	__raw_writel(ascsr, MXC_CRMAP_ASCSR);
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function sets the divider value for a clock.
 *
 * @param clk as defined in enum mxc_clocks
 * @param div the division factor to be used for the clock (For SSI & CSI, pass
 *            in 2 times the expected division value to account for FP vals)
 */
void mxc_set_clocks_div(enum mxc_clocks clk, unsigned int div)
{
	unsigned long reg, flags;
	unsigned int d;

	spin_lock_irqsave(&mxc_crm_lock, flags);

	switch (clk) {
	case AHB_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDR);
		d = (div == 16) ? 0 : div;
		BFINSERT(reg, ACDR_AHB_DIV, d);
		__raw_writel(reg, MXC_CRMAP_ACDR);
		break;
	case IPG_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDR);
		d = (div == 16) ? 0 : div;
		BFINSERT(reg, ACDR_IP_DIV, d);
		__raw_writel(reg, MXC_CRMAP_ACDR);
		break;
	case NFC_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
		BFINSERT(reg, ACDER2_NFC_DIV, (div - 1));
		__raw_writel(reg, MXC_CRMAP_ACDER2);
		break;
	case CPU_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDR);
		d = (div <= 1) ? 15 : (div - 2);
		if (div > 6) {
			d = d - ((div - 6) / 2);
		}
		BFINSERT(reg, ACDR_ARM_DIV, d);
		__raw_writel(reg, MXC_CRMAP_ACDR);
		break;
	case USB_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
		d = (div <= 1) ? 15 : (div - 2);
		if (div > 6) {
			d = d - ((div - 6) / 2);
		}
		BFINSERT(reg, ACDER2_USB_DIV, d);
		__raw_writel(reg, MXC_CRMAP_ACDER2);
		break;
	case UART1_BAUD:
	case UART2_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
		d = (div <= 1) ? 15 : (div - 2);
		if (div > 6) {
			d = d - ((div - 6) / 2);
		}
		BFINSERT(reg, ACDER2_BAUD_DIV, d);
		__raw_writel(reg, MXC_CRMAP_ACDER2);
		break;
	case UART3_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
		d = (div <= 1) ? 15 : (div - 2);
		if (div > 6) {
			d = d - ((div - 6) / 2);
		}
		BFINSERT(reg, APRA_UART3_DIV, d);
		__raw_writel(reg, MXC_CRMAP_APRA);
		break;
	case SSI1_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		d = (div == 62 * 2) ? 0 : div;
		BFINSERT(reg, ACDER1_SSI1_DIV, d);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;
	case SSI2_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		d = (div == 62 * 2) ? 0 : div;
		BFINSERT(reg, ACDER1_SSI2_DIV, d);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;
	case CSI_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		d = (div == 62 * 2) ? 0 : div;
		BFINSERT(reg, ACDER1_CS_DIV, d);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;
	case FIRI_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		BFINSERT(reg, ACDER1_FIRI_DIV, (div - 1));
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;
	default:
		break;
	}

	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * This function returns the DFS block divider - LFDF value
 */
unsigned int mxc_get_lfdf_value(void)
{
	volatile unsigned long adcr = __raw_readl(MXC_CRMAP_ADCR);

	return (1 << BFEXTRACT(adcr, ADCR_LFDF));

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
		reg &= ~(ACR_CKOHD | ACR_CKOHS_MASK | ACR_CKOHDIV_MASK);
		__raw_writel(__raw_readl(MXC_CRMCOM_CSCR) | CSCR_CKOHSEL,
			     MXC_CRMCOM_CSCR);
	} else {
		reg &= ~(ACR_CKOD | ACR_CKOS_MASK);
		__raw_writel(__raw_readl(MXC_CRMCOM_CSCR) | CSCR_CKOSEL,
			     MXC_CRMCOM_CSCR);
	}

	switch (clk) {
	case CLK_NONE:
		if (output == CKOH) {
			reg |= ACR_CKOHD;
		} else {
			reg |= ACR_CKOD;
		}
		break;

	case CPU_CLK:
		/* To select AP clock */
		if (output == CKOH) {
			reg = (ACR_CKOHS_AP_CLK << ACR_CKOHS_OFFSET) |
			    (div_value[div] << ACR_CKOHDIV_OFFSET);
		}
		break;

	case AHB_CLK:
		/* To select AHB clock */
		if (output == CKOH) {
			reg = (ACR_CKOHS_AP_AHB_CLK << ACR_CKOHS_OFFSET) |
			    (div_value[div] << ACR_CKOHDIV_OFFSET);
		}
		break;

	case IPG_CLK:
		/* To select IP clock */
		if (output == CKOH) {
			reg = (ACR_CKOHS_AP_PCLK << ACR_CKOHS_OFFSET) |
			    (div_value[div] << ACR_CKOHDIV_OFFSET);
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
		BFINSERT(reg, APRA_UART1_EN, 0);
		__raw_writel(reg, MXC_CRMAP_APRA);
		break;

	case UART2_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
		BFINSERT(reg, APRA_UART2_EN, 0);
		__raw_writel(reg, MXC_CRMAP_APRA);
		break;

	case UART3_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
		BFINSERT(reg, APRA_UART3_EN, 0);
		__raw_writel(reg, MXC_CRMAP_APRA);
		break;

	case SSI1_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		BFINSERT(reg, ACDER1_SSI1_EN, 0);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;

	case SSI2_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		BFINSERT(reg, ACDER1_SSI2_EN, 0);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;

	case CSI_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		BFINSERT(reg, ACDER1_CS_EN, 0);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;

	case NFC_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
		BFINSERT(reg, ACDER2_NFC_EN, 0);
		__raw_writel(reg, MXC_CRMAP_ACDER2);
		break;

	case USB_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
		BFINSERT(reg, ACDER2_USB_EN, 0);
		__raw_writel(reg, MXC_CRMAP_ACDER2);
		break;

	case SDHC1_CLK:
		reg = __raw_readl(MXC_CRMAP_APRB);
		BFINSERT(reg, APRB_SDHC1_EN, 0);
		__raw_writel(reg, MXC_CRMAP_APRB);
		break;

	case SDHC2_CLK:
		reg = __raw_readl(MXC_CRMAP_APRB);
		BFINSERT(reg, APRB_SDHC2_EN, 0);
		__raw_writel(reg, MXC_CRMAP_APRB);
		break;

	case SIM1_CLK:
		reg = __raw_readl(MXC_CRMAP_APRA);
		BFINSERT(reg, APRA_SIM_EN, 0);
		__raw_writel(reg, MXC_CRMAP_APRA);
		break;

	default:
		break;

	}
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
	return;
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

	case IPU_CLK:
		reg = __raw_readl(MXC_CRMAP_AMLPMRG);
		BFINSERT(reg, AMLPMR_MLPM2, AMLPMR_MLPM_ALWAYS_ON);
		__raw_writel(reg, MXC_CRMAP_AMLPMRG);
		break;

	case UART1_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
		BFINSERT(reg, APRA_UART1_EN, 1);
		__raw_writel(reg, MXC_CRMAP_APRA);
		break;

	case UART2_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
		BFINSERT(reg, APRA_UART2_EN, 1);
		__raw_writel(reg, MXC_CRMAP_APRA);
		break;

	case UART3_BAUD:
		reg = __raw_readl(MXC_CRMAP_APRA);
		BFINSERT(reg, APRA_UART3_EN, 1);
		__raw_writel(reg, MXC_CRMAP_APRA);
		break;

	case SSI1_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		BFINSERT(reg, ACDER1_SSI1_EN, 1);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;

	case SSI2_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		BFINSERT(reg, ACDER1_SSI2_EN, 1);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;

	case CSI_BAUD:
		reg = __raw_readl(MXC_CRMAP_ACDER1);
		BFINSERT(reg, ACDER1_CS_EN, 1);
		__raw_writel(reg, MXC_CRMAP_ACDER1);
		break;

	case NFC_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
		BFINSERT(reg, ACDER2_NFC_EN, 1);
		__raw_writel(reg, MXC_CRMAP_ACDER2);
		break;

	case USB_CLK:
		reg = __raw_readl(MXC_CRMAP_ACDER2);
		BFINSERT(reg, ACDER2_USB_EN, 1);
		__raw_writel(reg, MXC_CRMAP_ACDER2);
		break;

	case SDHC1_CLK:
		reg = __raw_readl(MXC_CRMAP_APRB);
		BFINSERT(reg, APRB_SDHC1_EN, 1);
		__raw_writel(reg, MXC_CRMAP_APRB);
		break;

	case SDHC2_CLK:
		reg = __raw_readl(MXC_CRMAP_APRB);
		BFINSERT(reg, APRB_SDHC2_EN, 1);
		__raw_writel(reg, MXC_CRMAP_APRB);
		break;

	case SIM1_CLK:
		reg = __raw_readl(MXC_CRMAP_APRA);
		BFINSERT(reg, APRA_SIM_EN, 1);
		__raw_writel(reg, MXC_CRMAP_APRA);
		break;

	default:
		break;

	}
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
	return;
}

EXPORT_SYMBOL(mxc_pll_clock);
EXPORT_SYMBOL(mxc_get_clocks);
EXPORT_SYMBOL(mxc_get_clocks_parent);
EXPORT_SYMBOL(mxc_get_clocks_div);
EXPORT_SYMBOL(mxc_set_clocks_pll);
EXPORT_SYMBOL(mxc_set_clocks_div);
EXPORT_SYMBOL(mxc_clks_disable);
EXPORT_SYMBOL(mxc_clks_enable);
EXPORT_SYMBOL(mxc_set_clock_output);
