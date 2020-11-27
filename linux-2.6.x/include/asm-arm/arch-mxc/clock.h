/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2007 Motorola, Inc.
 *
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
 * 04/16/2007  Motorola          Additional clocks for USB and SSI splitting
 * 06/06/2007  Motorola          Added include of config.h
 */

#ifndef __MXC_CLOCK_H_

#define __MXC_CLOCK_H_

/*!
 * @defgroup CLOCKS Clocking Setup and Retrieving
 * @ingroup MSL
 */

/*!
 * @file clock.h
 *
 * @brief API for setting up and retrieving clocks.
 *
 * This file contains API for setting up and retrieving clocks.
 *
 * @ingroup CLOCKS
 */

/*
 * Include Files
 */

#include <linux/config.h>


#ifndef __ASSEMBLY__
/*!
 * Enumerations of plls and available clock sources
 */
enum plls {
	MCUPLL = 0,		/*!< MCU PLL */
	USBPLL = 2,		/*!< USB PLL */
	DSPPLL = 1,		/*!< DSP PLL */
	CDPLL = 3,		/*!< Corrected Clock PLL */
	CKIH = 4,		/*!< Main Network clock */
	CKIH_X2 = 5,		/*!< Doubled version of CKIH */
	USBCLK = 8,		/*!< USB Clock */
	TURBOPLL,		/*!< Turbo PLL */
	SERIALPLL,		/*!<  PLL */
};

#ifdef CONFIG_MOT_FEAT_PM
#define NUM_MOT_PLLS 3 /* We are not touching CDPLL in AP */
#endif
enum mxc_clk_out {
	CKOH,
	CKO,
	CKO2,
	CKO1,
};

/*!
 * Enumerations for MXC clocks
 */
enum mxc_clocks {
	CLK_NONE,
	CKIL_CLK,
	CKIH_CLK,
	CPU_CLK,
	AHB_CLK,
	IPG_CLK,
	NFC_CLK,
	USB_CLK,
#ifdef CONFIG_MOT_FEAT_PM
	USB_IPG_CLK,
	USB_AHB_CLK,
#endif
	UART1_BAUD,
	UART2_BAUD,
	UART3_BAUD,
	UART4_BAUD,
	UART5_BAUD,
	UART6_BAUD,
	SSI1_BAUD,
	SSI2_BAUD,
#ifdef CONFIG_MOT_FEAT_PM
	SSI1_IPG_CLK,
	SSI2_IPG_CLK,
#endif
	CSI_BAUD,
	FIRI_BAUD,
	I2C_CLK,
	I2C1_CLK = I2C_CLK,
	I2C2_CLK,
	I2C3_CLK,
	CSPI1_CLK,
	CSPI2_CLK,
	CSPI3_CLK,
	GPT_CLK,
	RTC_CLK,
	EPIT1_CLK,
	EPIT2_CLK,
	EDIO_CLK,
	WDOG_CLK,
	WDOG2_CLK,
	PWM_CLK,
	IPU_CLK,
	SIM1_CLK,
	SIM2_CLK,
	HAC_CLK,
	GEM_CLK,
	SDHC1_CLK,
	SDHC2_CLK,
	SDMA_CLK,
	RNG_CLK,
	KPP_CLK,
	MU_CLK,
	RTIC_CLK,
	SCC_CLK,
	SPBA_CLK,
	DSM_CLK,
	SAHARA2_IPG_CLK,
	SAHARA2_AHB_CLK,
	MQSPI_IPG_CLK,
	MQSPI_CKIH_CLK,
	EL1T_IPG_CLK,
	EL1T_NET_CLK,
	LPMC_CLK,
	MPEG4_CLK,
	OWIRE_CLK,
	MBX_CLK,
	MSTICK1_BAUD,
	MSTICK2_BAUD,
	ATA_CLK,
};

/*!
 * This function is used to modify PLL registers to generate the required
 * frequency.
 *
 * @param  pll      the PLL that you wish to modify
 * @param  mfi      multiplication factor integer part
 * @param  pdf      pre-division factor
 * @param  mfd      multiplication factor denominator
 * @param  mfn      multiplication factor numerator
 */
void mxc_pll_set(enum plls pll, unsigned int mfi, unsigned int pdf,
		 unsigned int mfd, unsigned int mfn);

/*!
 * This function is used to get PLL registers values used generate the clock
 * frequency.
 *
 * @param  pll      the PLL that you wish to access
 * @param  mfi      pointer that holds multiplication factor integer part
 * @param  pdf      pointer that holds pre-division factor
 * @param  mfd      pointer that holds multiplication factor denominator
 * @param  mfn      pointer that holds multiplication factor numerator
 */
void mxc_pll_get(enum plls pll, unsigned int *mfi, unsigned int *pdf,
		 unsigned int *mfd, unsigned int *mfn);

/*!
 * This function returns the PLL output value in Hz based on pll.
 *
 * @param       pll     PLL as defined in enum plls
 *
 * @return      PLL value in Hz.
 */
unsigned long mxc_pll_clock(enum plls pll);

/*!
 * This function returns the clock value in Hz for various MXC modules.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks(enum mxc_clocks clk);

/*!
 * This function returns the parent clock value in Hz for various MXC modules.
 *
 * @param       clk     as defined in enum mxc_clocks
 *
 * @return      clock value in Hz
 */
unsigned long mxc_get_clocks_parent(enum mxc_clocks clk);

/*!
 * This function sets the PLL source for a clock.
 *
 * @param clk     as defined in enum mxc_clocks
 * @param pll_num the PLL that you wish to use as source for this clock
 */
void mxc_set_clocks_pll(enum mxc_clocks clk, enum plls pll_num);

/*!
 * This function sets the division factor for a clock.
 *
 * @param clk as defined in enum mxc_clocks
 * @param div the division factor to be used for the clock (For SSI, pass in
 *            2 times the expected division value to account for FP vals)
 */
void mxc_set_clocks_div(enum mxc_clocks clk, unsigned int div);

/*!
 * This function returns the peripheral clock dividers.
 * Note that for SSI divider, in order to maintain the accuracy, the returned
 * divider is doubled.
 *
 * @param       clk     peripheral clock as defined in enum mxc_clocks
 *
 * @return      divider value
 */
unsigned long mxc_peri_clock_divider(enum mxc_clocks clk);

/*!
 * This function returns the main clock dividers.
 *
 * @param       clk     peripheral clock as defined in enum mxc_clocks
 *
 * @return      divider value
 */
unsigned long mxc_main_clock_divider(enum mxc_clocks clk);

/*!
 * This function sets the digital frequency multiplier clock.
 *
 * @param       freq    Desired DFM output frequency in Hz
 *
 * @return      Actual DFM frequency in Hz
 */
unsigned long mxc_set_dfm_clock(unsigned int freq);

/*!
 * This function returns the DFS block divider - LFDF value
 *
 * @return      Low Voltage frequency Divider Factor value
 */
unsigned int mxc_get_lfdf_value(void);

/*!
 * This function is called to gate off the individual module clocks
 *
 * @param clks     as defined in enum mxc_clocks
 */
void mxc_clks_disable(enum mxc_clocks clks);

/*!
 * This function is called to enable the individual module clocks
 *
 * @param clks     as defined in enum mxc_clocks
 */
void mxc_clks_enable(enum mxc_clocks clks);

/*!
 * This function is called to read the contents of a CCM register
 *
 * @param reg_offset the CCM register that will read
 *
 * @return the register contents
 */
volatile unsigned long mxc_ccm_get_reg(unsigned int reg_offset);

/*!
 * This function is called to modify the contents of a CCM register
 *
 * @param reg_offset the CCM register that will read
 * @param mask       the mask to be used to clear the bits that are to be modified
 * @param data       the data that should be written to the register
 */
void mxc_ccm_modify_reg(unsigned int reg_offset, unsigned int mask,
			unsigned int data);

/*!
 * Configure clock output on CKO1/CKO2 pins
 *
 * @param   opt     The desired clock needed to measure. Possible
 *                  values are, CKOH_AP_SEL, CKOH_AHB_SEL or CKOH_IP_SEL
 *
 */
void mxc_set_clock_output(enum mxc_clk_out output, enum mxc_clocks clk,
			  int div);

/*!
 * This function returns the divider value for a clock.
 *
 * @param       clk as defined in enum mxc_clocks
 *
 * @return      divider value
 */
unsigned long mxc_get_clocks_div(enum mxc_clocks clk);

#ifdef CONFIG_MOT_FEAT_PM
/*!
 * This function requests that a pll be locked and available.
 *
 * @param       pllnum as defined in enum plls
 *
 * @return      0 on success, -1 on invalid PLL number
 */
int mxc_pll_request_pll(enum plls pllnum);

/*!
 * This function releases a previous request for a pll
 *
 * @param       pllnum as defined in enum plls
 *
 * @return      0 on success, -1 on invalid PLL number, -2 if already unlocked
 */
int mxc_pll_release_pll(enum plls pllnum);
#endif /* CONFIG_MOT_FEAT_PM */

#endif				/* __ASSEMBLY__ */
#endif				/* __MXC_CLOCK_H_ */
