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
 * @file mxc_pm_mx3.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for the CRM_MCU and DPLL in mx3.
 *
 * @ingroup LPMD
 */

/*
 * Include Files
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <asm/hardware.h>
#include <asm/arch/system.h>
#include <asm/arch/clock.h>
#include <asm/arch/mxc_pm.h>
#include <asm/irq.h>
#include <asm/arch/dvfs_dptc_struct.h>
#include <asm/arch/dvfs.h>

#include "crm_regs.h"

/* Local defines */
#define FREQ_COMP_TOLERANCE      200	/* tolerance percentage times 100 */
#define MCU_PLL_MAX_FREQ   600000000	/* Maximum frequency MCU PLL clock */
#define MCU_PLL_MIN_FREQ   160000000	/* Minimum frequency MCU PLL clock */
#define NFC_MAX_FREQ        20000000	/* Maximum frequency NFC clock */
#define PRE_DIV_MIN_FREQ    10000000	/* Minimum Frequency after Predivider */

/* Define MXC_PM_DEBUG to enable debug messages */
#define MXC_PM_DEBUG

#ifdef MXC_PM_DEBUG
#define dprint(fmt, args...) printk("%s %d: " fmt,__FUNCTION__,__LINE__,##args)
#else
#define dprint(fmt, args...) do{}while (0)
#endif

#ifdef MXC_PM_DEBUG
static void print_frequencies(void)
{
	printk("MCUPLL       %9lu\n", mxc_pll_clock(MCUPLL));
	printk("SERIALPLL    %9lu\n", mxc_pll_clock(SERIALPLL));
	printk("USBPLL       %9lu\n", mxc_pll_clock(USBPLL));
	printk("\n");
	printk("CPU_CLK      %9lu\n", mxc_get_clocks(CPU_CLK));
	printk("AHB_CLK      %9lu\n", mxc_get_clocks(AHB_CLK));
	printk("IPG_CLK      %9lu\n", mxc_get_clocks(IPG_CLK));
	printk("NFC_CLK      %9lu\n", mxc_get_clocks(NFC_CLK));
	printk("IPU_CLK      %9lu\n", mxc_get_clocks(IPU_CLK));
	printk("USB_CLK      %9lu\n", mxc_get_clocks(USB_CLK));
	printk("CSI_BAUD     %9lu\n", mxc_get_clocks(CSI_BAUD));
	printk("UART1_BAUD   %9lu\n", mxc_get_clocks(UART1_BAUD));
	printk("SSI1_BAUD    %9lu\n", mxc_get_clocks(SSI1_BAUD));
	printk("SSI2_BAUD    %9lu\n", mxc_get_clocks(SSI2_BAUD));
	printk("FIRI_BAUD    %9lu\n", mxc_get_clocks(FIRI_BAUD));
	printk("MSTICK1_BAUD %9lu\n", mxc_get_clocks(MSTICK1_BAUD));
	printk("MSTICK2_BAUD %9lu\n", mxc_get_clocks(MSTICK2_BAUD));
	printk("\n");
}
#else
#define print_frequencies() do{}while (0)
#endif

/*!
 * Compare two frequences using allowable tolerance
 *
 * The MX3 PLL can generate many frequencies. This function
 * compares the generated frequency to the requested frequency
 * and determines it they are within and acceptable tolerance.
 *
 * @param   freq1  desired frequency
 * @param   freq2  generated frequency
 *
 * @return       Returns 0 is frequencies are within talerance
 *               and non-zero is they are not.
 */
static int freq_equal(unsigned long freq1, unsigned long freq2)
{
	if (freq1 > freq2) {
		return (freq1 - freq2) <= (freq1 / FREQ_COMP_TOLERANCE);
	}
	return (freq2 - freq1) <= (freq1 / FREQ_COMP_TOLERANCE);
}

/*!
 * Calculate new MCU clock dividers for the PDR0 regiser.
 *
 * @param   mcu_main_clk PLL output frequency (Hz)
 * @param   arm_freq     desired ARM frequency (Hz)
 * @param   max_freq     desired MAX frequency (Hz)
 * @param   ip_freq      desired IP frequency (Hz)
 * @param   mask         were to return PDR0 mask
 * @param   value        were to return PDR0 value
 *
 * @return             Returns 0 on success or
 *                     Returns non zero if error
 *                       PLL_LESS_ARM_ERR if pll frequency is less than
 *                       desired core frequency
 *                       FREQ_OUT_OF_RANGE if desided frequencies ar not
 *                       possible with the current mcu pll frequency.
 */
static int
cal_pdr0_value(unsigned long mcu_main_clk,
	       long arm_freq,
	       long max_freq,
	       long ip_freq, unsigned long *mask, unsigned long *value)
{
	unsigned long arm_div;	/* ARM core clock divider */
	unsigned long max_div;	/* MAX clock divider */
	unsigned long ipg_div;	/* IPG clock divider */
	unsigned long nfc_div;	/* NFC (Nand Flash Controller) clock divider */
	unsigned long hsp_div;	/* HSP clock divider */

	if (arm_freq > mcu_main_clk) {
		return -PLL_LESS_ARM_ERR;
	}

	arm_div = mcu_main_clk / arm_freq;
	if ((arm_div == 0) || !freq_equal(arm_freq, mcu_main_clk / arm_div)) {
		return FREQ_OUT_OF_RANGE;
	}
	max_div = mcu_main_clk / max_freq;
	if ((max_div == 0) || !freq_equal(max_freq, mcu_main_clk / max_div)) {
		return FREQ_OUT_OF_RANGE;
	}
	hsp_div = max_div;

	ipg_div = max_freq / ip_freq;
	if ((ipg_div == 0) || !freq_equal(ip_freq, max_freq / ipg_div)) {
		return FREQ_OUT_OF_RANGE;
	}

	nfc_div = ((max_freq - 1000000) / NFC_MAX_FREQ) + 1;

	/* All of the divider values have been calculated.
	 * Now change the hardware register. */

	*mask = MXC_CCM_PDR0_HSP_PODF_MASK |
	    MXC_CCM_PDR0_NFC_PODF_MASK |
	    MXC_CCM_PDR0_IPG_PODF_MASK |
	    MXC_CCM_PDR0_MAX_PODF_MASK | MXC_CCM_PDR0_MCU_PODF_MASK;

	*value = ((hsp_div - 1) << MXC_CCM_PDR0_HSP_PODF_OFFSET) |
	    ((nfc_div - 1) << MXC_CCM_PDR0_NFC_PODF_OFFSET) |
	    ((ipg_div - 1) << MXC_CCM_PDR0_IPG_PODF_OFFSET) |
	    ((max_div - 1) << MXC_CCM_PDR0_MAX_PODF_OFFSET) |
	    ((arm_div - 1) << MXC_CCM_PDR0_MCU_PODF_OFFSET);

	return 0;
}

/*!
 * Integer clock scaling
 *
 * Change main arm clock frequencies without changing the PLL.
 * The integer dividers are changed to produce the desired
 * frequencies. The number of valid frequency are limited and
 * are determined by the current MCU PLL frequency
 *
 * @param   arm_freq    desired ARM frequency (Hz)
 * @param   max_freq    desired MAX frequency (Hz)
 * @param   ip_freq     desired IP frequency (Hz)
 *
 * @return             Returns 0 on success or
 *                     Returns non zero if error
 *                       PLL_LESS_ARM_ERR if pll frequency is less than
 *                       desired core frequency
 *                       FREQ_OUT_OF_RANGE if desided frequencies ar not
 *                       possible with the current mcu pll frequency.
 */
int mxc_pm_intscale(long arm_freq, long max_freq, long ip_freq)
{
	unsigned long mcu_main_clk;	/* mcu clock domain main clock */
	unsigned long mask;
	unsigned long value;
	int ret_value;

	dprint("arm_freq=%ld, max_freq=%ld, ip_freq=%ld\n",
	       arm_freq, max_freq, ip_freq);
	print_frequencies();	/* debug */

	mcu_main_clk = mxc_pll_clock(MCUPLL);
	ret_value = cal_pdr0_value(mcu_main_clk, arm_freq, max_freq, ip_freq,
				   &mask, &value);

	if (ret_value == 0) {
		mxc_ccm_modify_reg(MXC_CCM_PDR0, mask, value);
	}

	print_frequencies();
	return ret_value;
}

/*!
 * PLL clock scaling
 *
 * Change MCU PLL frequency and adjust derived clocks. Integer
 * dividers are used generate the derived clocks so changed to produce
 * the desired the valid frequencies are limited by the desired ARM
 * frequency.
 *
 * The clock source for the MCU is set to the MCU PLL.
 *
 * @param   arm_freq    desired ARM frequency (Hz)
 * @param   max_freq    desired MAX frequency (Hz)
 * @param   ip_freq     desired IP frequency (Hz)
 *
 * @return             Returns 0 on success or
 *                     Returns non zero if error
 *                       PLL_LESS_ARM_ERR if pll frequency is less than
 *                       desired core frequency
 *                       FREQ_OUT_OF_RANGE if desided frequencies ar not
 *                       possible with the current mcu pll frequency.
 */
int mxc_pm_pllscale(long arm_freq, long max_freq, long ip_freq)
{
	unsigned long ccmr;	/* clock control register */
	unsigned long prcs;	/* PLL reference clock select bits. */
	signed long ref_freq;	/* reference frequency */
	signed long pll_freq;	/* target pll frequency */
	signed long pd;		/* Pre-divider */
	signed long mfi;	/* Multiplication Factor (Integer part) */
	signed long mfn;	/* Multiplication Factor (Integer part) */
	signed long mfd;	/* Multiplication Factor (Denominator Part) */
	signed long old_pll;	/* old pll frequency */
	signed long tmp;
	unsigned long mask;
	unsigned long value;
	int ret_value;

	dprint("arm_freq=%ld, max_freq=%ld, ip_freq=%ld\n",
	       arm_freq, max_freq, ip_freq);
	print_frequencies();

	ccmr = __raw_readl(IO_ADDRESS(CCM_BASE_ADDR) + MXC_CCM_CCMR);
	prcs = (ccmr & MXC_CCM_CCMR_PRCS_MASK) >> MXC_CCM_CCMR_PRCS_OFFSET;
	if (prcs == 0x1) {
		ref_freq = mxc_get_clocks(CKIL_CLK) * 1024;
	} else {
		ref_freq = mxc_get_clocks(CKIH_CLK);
	}
	/*
	 * MCU PLL output frequency equation
	 *
	 *                           mfn
	 *                    mfi + -----
	 *                           mfd
	 *    ref_freq * 2 * ------------- =  pll_freq
	 *                        pd
	 *
	 *   mfi must be >= 5 and <= 15
	 *   mfd range 1 to 1024
	 *   mfn range -512 to 511
	 *   The absolute value of mfn/mfd must be smaller than 1.
	 *   pd range 1 to 16
	 *
	 */

	pd = 1;
	pll_freq = arm_freq;

	do {
		if ((pll_freq > MCU_PLL_MAX_FREQ) || (pll_freq / 8 > arm_freq)) {
			return FREQ_OUT_OF_RANGE;
		}
		if (pll_freq < MCU_PLL_MIN_FREQ) {
			ret_value = 111;
		} else {
			ret_value =
			    cal_pdr0_value(pll_freq, arm_freq, max_freq,
					   ip_freq, &mask, &value);
		}
		if (ret_value != 0) {
			pll_freq += arm_freq;
		}
	} while (ret_value != 0);

	while (((ref_freq / pd) * 10) > pll_freq) {
		pd++;
	}

	if ((ref_freq / pd) < PRE_DIV_MIN_FREQ) {
		return FREQ_OUT_OF_RANGE;
	}

	/* the ref_freq/2 in the following is to round up */
	mfi = (((pll_freq / 2) * pd) + (ref_freq / 2)) / ref_freq;
	if (mfi < 5 || mfi > 15) {
		return FREQ_OUT_OF_RANGE;
	}

	/* pick a mfd value that will work
	 * then solve for mfn */
	mfd = ref_freq / 50000;

	/*
	 *          pll_freq * pd * mfd
	 *   mfn = --------------------  -  (mfi * mfd)
	 *           2 * ref_freq
	 */
	/* the tmp/2 is for rounding */
	tmp = ref_freq / 10000;
	mfn =
	    ((((((pll_freq / 2) + (tmp / 2)) / tmp) * pd) * mfd) / 10000) -
	    (mfi * mfd);

	mfn = mfn & 0x3ff;

	old_pll = mxc_pll_clock(MCUPLL);
	if (pll_freq > old_pll) {
		/* if pll freq is increasing then change dividers first */
		mxc_ccm_modify_reg(MXC_CCM_PDR0, mask, value);
		mxc_pll_set(MCUPLL, mfi, pd - 1, mfd - 1, mfn);
	} else {
		/* if pll freq is decreasing then change pll first */
		mxc_pll_set(MCUPLL, mfi, pd - 1, mfd - 1, mfn);
		mxc_ccm_modify_reg(MXC_CCM_PDR0, mask, value);
	}
	print_frequencies();
	return 0;
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
	unsigned int lpm;
	int enable_flag;
	unsigned long reg;

	local_irq_disable();
	enable_flag = 0;

	switch (mode) {
	case DOZE_MODE:
		lpm = 1;
		break;

	case STOP_MODE:
		/* State Retention mode */
		lpm = 2;
		/* Disable timer interrupt */
		disable_irq(INT_GPT);
		enable_flag = 1;
		break;

	case DSM_MODE:
		/* Deep Sleep Mode */
		lpm = 3;
		/* Disable timer interrupt */
		disable_irq(INT_GPT);
		enable_flag = 1;
		/* Enabled Well Bias
		 * SBYCS = 0, MCU clock source is disabled*/
		mxc_ccm_modify_reg(MXC_CCM_CCMR,
				   MXC_CCM_CCMR_WBEN | MXC_CCM_CCMR_SBYCS,
				   MXC_CCM_CCMR_WBEN);
		break;

	default:
	case WAIT_MODE:
		/* Wait is the default mode used when idle. */
		lpm = 0;
		break;
	}
	reg = __raw_readl(MXC_CCM_CCMR);
	reg = (reg & (~MXC_CCM_CCMR_LPM_MASK)) | lpm << MXC_CCM_CCMR_LPM_OFFSET;
	__raw_writel(reg, MXC_CCM_CCMR);
	/* Executing CP15 (Wait-for-Interrupt) Instruction */
	/* wait for interrupt */
	__asm__ __volatile__("mcr	p15, 0, r1, c7, c0, 4\n"
			     "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"::);

	if (enable_flag) {
		/* Enable timer interrupt */
		enable_irq(INT_GPT);
	}
	local_irq_enable();
}

/*!
 * Changes MCU frequencies using dvfs.
 *
 * @param       armfreq       desired ARM frequency in Hz
 * @param       ahbfreq       desired AHB frequency in Hz
 * @param       ipfreq        desired IP frequency in Hz
 *
 * @return             Returns 0 on success, non-zero on error
 */
int mxc_pm_dvfs(unsigned long armfreq, long ahbfreq, long ipfreq)
{
	int ret_value;
	int i;

	if (ahbfreq != 133000000) {
		return FREQ_OUT_OF_RANGE;
	}
	if (ipfreq != 66500000) {
		return FREQ_OUT_OF_RANGE;
	}
	ret_value = FREQ_OUT_OF_RANGE;
	for (i = 0; i < dvfs_states_tbl->num_of_states; i++) {
		if (dvfs_states_tbl->freqs[i] == armfreq) {
			ret_value = dvfs_set_state(i);
			break;
		}
	}

	return ret_value;
}

/*!
 * This function is used to load the module.
 *
 * @return   Returns an Integer on success
 */
static int __init mxc_pm_init_module(void)
{
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
EXPORT_SYMBOL(mxc_pm_lowpower);
EXPORT_SYMBOL(mxc_pm_dvfs);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MX3 Low-level Power Management Driver");
MODULE_LICENSE("GPL");
