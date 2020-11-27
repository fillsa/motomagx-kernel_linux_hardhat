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
 * @file ipu_lpmc.c
 *
 * @brief This file contains the IPU API functions for the LPMC module.
 *
 * @ingroup IPU
 */

#include <linux/module.h>
#include "ipu.h"

#ifdef CONFIG_ARCH_MXC91231

#include "ipu_prv.h"
#include "asm-arm/io.h"
#include <asm/arch/clock.h>
#include <asm/arch/mxc_pm.h>

#define LPMC_CNTL	IO_ADDRESS(LPMC_BASE_ADDR + 0)
#define LPMC_DATA	IO_ADDRESS(LPMC_BASE_ADDR + 4)
#define LPMC_STAT	IO_ADDRESS(LPMC_BASE_ADDR + 8)

#define LPMC_CNTL_LOW_PWR_CLK           0x00000002
#define LPMC_CNTL_LPMC_EN               0x00000001
#define LPMC_CNTL_RMP_END_PTR_OFFSET    16
#define LPMC_CNTL_LPMP_END_PTR_OFFSET   24

/*!
 * This function initializes low-power self-refresh mode.
 *
 * @param       dfm_freq        Frequency for DFM. This should be 4x the
 *                              pixel clock.
 *
 * @param       save_list       List of IPU register accesses for saving normal
 *                              mode.
 *
 * @param       save_count      Number of IPU register accesses for normal mode.
 *
 * @param       lpm_list        List of IPU register accesses for setting up
 *                              low power, self-refresh mode.
 *
 * @param       lpm_count       Number of IPU register accesses for
 *                              low power, self-refresh mode.
 *
 * @return      This function returns 0 on success or negative error code on fail
 */
int ipu_lpmc_init(u32 dfm_freq,
		  ipu_lpmc_reg_t * save_list,
		  u32 save_count, ipu_lpmc_reg_t * lpm_list, u32 lpm_count)
{
	u32 reg, lpmc_cntl;
	u32 lpmc_state, previous_lpmc_state;

	FUNC_START;

	mxc_set_dfm_clock(dfm_freq);
	mxc_clks_enable(LPMC_CLK);

#ifdef CONFIG_DPM
	mxc_pm_lp_ack_enable(MXC_PM_LOWPWR_ACK_IPU);
#endif
	lpmc_cntl = (save_count) << LPMC_CNTL_RMP_END_PTR_OFFSET;
	lpmc_cntl |= (lpm_count + save_count) << LPMC_CNTL_LPMP_END_PTR_OFFSET;
	__raw_writel(lpmc_cntl, LPMC_CNTL);

	/* Dummy read of IPU_CONF is needed for some reason	*/
	__raw_writel(0, LPMC_DATA);
	__raw_writel(0, LPMC_DATA);

	/* Setup LPMC normal mode register save template */
	for (; save_count > 0; save_count--) {
		reg = (~IPU_LPMC_REG_READ & save_list->reg) >> 1;
		if ((save_list->reg & IPU_LPMC_REG_READ) == 0)
			reg |= 0x1;	/* Write register */
		__raw_writel(reg, LPMC_DATA);
		__raw_writel(save_list->value, LPMC_DATA);
		DPRINTK("Save reg - reg 0x%08X = 0x%08X\n", reg,
			save_list->value);
		save_list++;
	}

	/* Setup LPMC low power mode register settings */
	for (; lpm_count > 0; lpm_count--) {
		reg = (~IPU_LPMC_REG_READ & lpm_list->reg) >> 1;
		if ((lpm_list->reg & IPU_LPMC_REG_READ) == 0)
			reg |= 0x1;	/* Write register */
		__raw_writel(reg, LPMC_DATA);
		__raw_writel(lpm_list->value, LPMC_DATA);
		DPRINTK("lpm reg - reg 0x%08X = 0x%08X\n", reg,
			lpm_list->value);
		lpm_list++;
	}

	lpmc_state = previous_lpmc_state = __raw_readl(LPMC_STAT) >> 16;
	DPRINTK("LPMC off - LPMC_STAT = 0x%X\n", lpmc_state);

	lpmc_cntl |= LPMC_CNTL_LOW_PWR_CLK | LPMC_CNTL_LPMC_EN;
	__raw_writel(lpmc_cntl, LPMC_CNTL);	/* EXIT LPMC Mode */
	DPRINTK("LPMC_CNTL = 0x%08X\n", lpmc_cntl);

	while (lpmc_state != 0xC) {
		lpmc_state = __raw_readl(LPMC_STAT) >> 16;
		if (previous_lpmc_state != lpmc_state) {
			DPRINTK("LPMC_STAT = 0x%08X\n", lpmc_state);
			previous_lpmc_state = lpmc_state;
		}
	}

	FUNC_END;
	return 0;
}

/*!
 * This function uninitializes low-power self-refresh mode and returns
 * to normal operation.
 *
 */
void ipu_lpmc_uninit(void)
{
	u32 reg;
	u32 lpmc_state, previous_lpmc_state;
	u32 lock_flags;

	FUNC_START;

	spin_lock_irqsave(&ipu_lock, lock_flags);

#ifdef CONFIG_DPM
	mxc_pm_lp_ack_disable(MXC_PM_LOWPWR_ACK_IPU);
#endif
	reg = __raw_readl(LPMC_CNTL);
	reg &= ~LPMC_CNTL_LOW_PWR_CLK;
	__raw_writel(reg, LPMC_CNTL);	/* EXIT LPMC Mode */

	spin_unlock_irqrestore(&ipu_lock, lock_flags);

	lpmc_state = previous_lpmc_state = __raw_readl(LPMC_STAT) >> 16;
	while (lpmc_state != 0) {
		lpmc_state = __raw_readl(LPMC_STAT) >> 16;
		if (previous_lpmc_state != lpmc_state) {
			DPRINTK("LPMC_STAT = 0x%08X\n", lpmc_state);
			previous_lpmc_state = lpmc_state;
		}
	}

	__raw_writel(0, LPMC_CNTL);	/* Disable LPMC	*/
	mxc_clks_disable(LPMC_CLK);
	mxc_set_dfm_clock(0);

	FUNC_END;
}

#else				/* CONFIG_ARCH_MXC91231 */
int ipu_lpmc_init(u32 dfm_freq,
		  ipu_lpmc_reg_t * save_list,
		  u32 save_count, ipu_lpmc_reg_t * lpm_list, u32 lpm_count)
{
	return 0;
}

void ipu_lpmc_uninit(void)
{
}

#endif

EXPORT_SYMBOL(ipu_lpmc_init);
EXPORT_SYMBOL(ipu_lpmc_uninit);
