/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2008 Motorola, Inc.
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
 * 10/04/2006  Motorola  ifdef'd out unneeded code for PMIC handshake.
 * 12/25/2006  Motorola  Changed local_irq_disable/local_irq_enable to
 *                       local_irq_save/local_irq_restore.
 * 03/09/2007  Motorola  Added DVFS support
 * 04/04/2007  Motorola  Add code for DSM statistics gathering.
 * 05/15/2007  Motorola  Use AHB_FREQ instead of CLOCK_TICK_RATE when
 *                       determining AHB frequency.
 * 08/07/2007  Motorola  Remove code of clearing up DPE bit.
 * 10/31/2007  Motorola  Integrate the patch for removing WFI SW workaround.
 * 11/14/2007  Motorola  Add fix for errata issues.
 * 08/25/2008  Motorola  Force panic when neither DFSI or DFSP bits is correct
 *                       for freq switching.
 * 11/07/2008  Motorola  Disable PMICHE and set PMIC_HS_CNT to 80us as
 *                       a work around for WD2 apmd panic issue.
 */

/*!
 * @file mxc_pm.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for the CCM_MCU, DPLL and DSM modules.
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
#ifdef CONFIG_MOT_FEAT_PM_STATS
#include <linux/mpm.h>
#include <linux/rtc.h>
#endif
#include <asm/arch/timex.h>
#include <asm/arch/mxc_pm.h>
#include <asm/arch/system.h>
#include <asm/arch/clock.h>
#include <asm/arch/spba.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/mach/time.h>
#include <asm/arch/gpio.h>
#if CONFIG_MOT_FEAT_PM
#include <asm/arch/mxc_mu.h>
#include <asm/bootinfo.h>
#endif

#include "crm_regs.h"

/*
 * AVIC Lo Interrupt enable register for low-power modes
 */
#define LO_INTR_EN IO_ADDRESS(AVIC_BASE_ADDR + 0x14)

/*
 * MAX clock divider values
 */
#define MAX_PDF_3               0x00000010
#define MAX_PDF_4               0x00000018
#define MAX_PDF_5		0x00000020

/*
 * BRMM divider values
 */
#define MPDR0_BRMM_0            0x00000000
#define MPDR0_BRMM_1            0x00000001
#define MPDR0_BRMM_2            0x00000002
#define MPDR0_BRMM_3            0x00000003
#define MPDR0_BRMM_4            0x00000004

/*
 * MPDR0 Register Mask for Turbo and Normal mode switch
 */
#define TURBO_MASK              0x0000083F

/*
 * PMIC HS count values for max DVS speed setting
 */
#ifdef CONFIG_MOT_FEAT_PM
/*
 * Adds a 40us CKIH delay to allow POWER_RDY to go low after voltage change.
 */
/* #define PMCR1_PMIC_HS_CNT       0x420 */

/* 
 * As a workaround to avoid blocking at PMIC HS, PMICHE is disabled
 * Adjust 80us waiting for voltage change
 */
#define PMCR1_PMIC_HS_CNT       0x820
#else
#define PMIC_HS_CNT             0x204
#endif

#ifndef CONFIG_MOT_FEAT_PM
/*
 * Currently MXC91321 1.0 is unstable when AHB is 133MHz. So, clock
 * tick rate used is hardcoded to 100MHz assuming AHB clock is always
 * maintained at 100MHz. Therefor, the operating points will vary depending
 * on this #define
 */
#define AHB_CLOCK_100		   3
#endif

#ifdef AHB_CLOCK_100
/*!
 * Enumerations of operating points
 * Normal mode is 399MHz and Turbo mode is 500MHz
 * CORE_NORMAL_4 is PLL is 399MHz and BRMM is 4 when AHB is 100MHz
 */
typedef enum {
	CORE_NORMAL_4 = 100000000,
	CORE_NORMAL_3 = 133000000,
	CORE_NORMAL_2 = 160000000,
	CORE_NORMAL_1 = 200000000,
	CORE_NORMAL = 399000000,
	CORE_TURBO = 500000000,
	/*
	 * The following points are currently not implemented.
	 * These operating points are used only when
	 * the user really wants to lower frequency fast when
	 * working with TPLL otherwise it should not be used since
	 * these points operate at 1.6V and hence will not save power
	 */
	CORE_TURBO_4 = 100160000,	/* 100MHz at 1.6V */
	CORE_TURBO_3 = 136000000,
	CORE_TURBO_2 = 166000000,
	CORE_TURBO_1 = 214000000,
} dvfs_op_point_t;
#else
/*!
 * Enumerations of operating points
 */
typedef enum {
#ifdef CONFIG_MOT_FEAT_PM
	CORE_NORMAL_4 = 128000000,
	CORE_NORMAL_3 = 165000000,
	CORE_NORMAL_2 = 193000000,
	CORE_NORMAL_1 = 231000000,
	CORE_NORMAL = 385000000,
	CORE_TURBO = 514000000,
#else
	CORE_NORMAL_4 = 133000000,
	CORE_NORMAL_3 = 171000000,
	CORE_NORMAL_2 = 200000000,
	CORE_NORMAL_1 = 240000000,
	CORE_NORMAL = 399000000,
	CORE_TURBO = 532000000,
#endif
	/*
	 * The following points are currently not implemented.
	 * These operating points are used only when
	 * the user really wants to lower frequency fast when
	 * working with TPLL otherwise it should not be used since
	 * these points operate at 1.6V and hence will not save power
	 */
	CORE_TURBO_4 = 133160000,	/* 133MHz at 1.6V */
	CORE_TURBO_3 = 177000000,
	CORE_TURBO_2 = 212000000,
	CORE_TURBO_1 = 266000000,
} dvfs_op_point_t;
#endif

/*!
 * Enumerations of voltage levels
 */
typedef enum {
	LO_VOLT = 0,
	HI_VOLT = 1,
} dvs_t;

/*!
 * Enumerations of Turbo mode
 */
typedef enum {
	TPSEL_CLEAR = 0,
	TPSEL_SET = 1,
} tpsel_t;

/*!
 * Enumerations of DFSP switch status
 */
typedef enum {
	DFS_SWITCH = 0,
	DFS_NO_SWITCH = 1,
} dfs_switch_t;

/*
 * Timer Interrupt
 */
#define AVIC_TIMER_DIS		0xDFFFFFFF
#define AVIC_TIMER_EN		0x20000000

/*
 * COSR Register Mask for ARM, AHB, IPG clock
 */
#define COSR_REG_MASK		0xFFF803FF

/*
 * GPIO calls
 */
extern void gpio_pmhs_active(void);
extern void gpio_pmhs_inactive(void);

/*!
 * Declaring lock to prevent request to change core
 * frequency when previous transition is not finished
 */
static DEFINE_RAW_SPINLOCK(dvfs_lock);

#ifdef CONFIG_MOT_FEAT_PM
/*!
 * Perform software workaround of a chip errata that results in
 * an L1 instruction cache corruption.  This function must be
 * called with interrupts disabled and just before the call to
 * cpu_do_idle.
 *
 * @return  unsigned long     Saved value of MPDR0.
 */
unsigned long mxc_pm_l1_instr_cache_corruption_fix_before_wfi(void)
{
	unsigned long mpdr0;

        /*
         * There is an IC issue which causes a missing clock to part
         * of the ARM platform. This causes the ARM core to behave
         * erratically and could result in lockups or CPU exceptions.
         * The fix below changes the BRMM mux to set the core clock to
         * the AHB which will prevent the clock synchronization issue
         * which was the root cause of the missing clock.
         */

        /* Save MPDR0 */
        mpdr0 = __raw_readl(MXC_CCM_MPDR0);

        /* Set the BRMM bits to mux AHB to ARM core. */
        __raw_writel((mpdr0 & ~MXC_CCM_MPDR0_BRMM_MASK) | 
                    (MPDR0_BRMM_4 << MXC_CCM_MPDR0_BRMM_OFFSET),
                     MXC_CCM_MPDR0);

        /* 3 AHB clock delay for clock switch. */
        __raw_readl(MXC_CCM_MPDR0);

        /* 3 AHB clock delay for clock switch. */
        __raw_readl(MXC_CCM_MPDR0);

	return mpdr0;
}

/*!
 * Perform software workaround of a chip errata that results in
 * an L1 instruction cache corruption.  This function must be
 * called with interrupts disabled and just after the call to
 * cpu_do_idle.
 *
 * @param   mpdr0     Saved value of MPDR0.
 */
void mxc_pm_l1_instr_cache_corruption_fix_after_wfi(unsigned long mpdr0)
{
	/* Restore MPDR0 */
	__raw_writel(mpdr0, MXC_CCM_MPDR0);
}
#endif /* CONFIG_MOT_FEAT_PM */

/*!
 * To check status of DFSP, frequency switch
 *
 * @param   dfs_switch_t      DFS_SWITCH - switch between MPLL and
 *                            TPLL is in progress
 *                            DFS_NO_SWITCH - switch between MPLL and
 *                            TPLL is not in progress
 */
static dfs_switch_t mxc_pm_chk_dfsp(void)
{
	if ((__raw_readl(MXC_CCM_PMCR0) & (MXC_CCM_PMCR0_DFSP)) ==
	    MXC_CCM_PMCR0_DFSP) {
		return DFS_SWITCH;
	} else {
		return DFS_NO_SWITCH;
	}
}

/*!
 * To set Dynamic Voltage Scaling
 *
 */
static int mxc_pm_dvsenable(void)
{
	unsigned long pmcr;
#ifdef CONFIG_MOT_FEAT_PM
	unsigned long pmic_hs_cnt;
#endif

	if (mxc_pm_chk_dfsp() == DFS_NO_SWITCH) {
		/* Enable DVS, EMI handshake disable PMIC handshake and mask DFSI interrupt */
		pmcr = ((((__raw_readl(MXC_CCM_PMCR0) | (MXC_CCM_PMCR0_DVSE)) &
			  (~MXC_CCM_PMCR0_PMICHE)) | (MXC_CCM_PMCR0_EMIHE)) |
			(MXC_CCM_PMCR0_DFSIM));
		__raw_writel(pmcr, MXC_CCM_PMCR0);
#ifdef CONFIG_MOT_FEAT_PM
                pmic_hs_cnt = __raw_readl(MXC_CCM_PMCR1) & 
					~(MXC_CCM_PMCR1_PMIC_HS_CNT_MASK);
                pmic_hs_cnt |= PMCR1_PMIC_HS_CNT;

		__raw_writel(pmic_hs_cnt, MXC_CCM_PMCR1);
#endif

	} else {
		return ERR_DFSP_SWITCH;
	}
	return 0;
}

/*!
 * To check status of TPSEL
 *
 * @param   tpsel_t      TPSEL_SET - Turbo mode set
 *                       TPSEL_CLEAR - Turbo mode is clear
 */
static tpsel_t mxc_pm_chk_tpsel(void)
{
	if ((__raw_readl(MXC_CCM_MPDR0) & (MXC_CCM_MPDR0_TPSEL)) != 0) {
		return TPSEL_SET;
	} else {
		return TPSEL_CLEAR;
	}
}

/*!
 * Setting possible BRMM value
 *
 * @param   value      Possible values are 0,1,2,3.
 *
 */
static void mxc_pm_setbrmm(int value)
{
	mxc_ccm_modify_reg(MXC_CCM_MPDR0, MXC_CCM_MPDR0_BRMM_MASK, value);
}

/*!
 * To set Dynamic Voltage Scaling
 *
 */
static int mxc_pm_setturbo(dvfs_op_point_t dvfs_op_reqd)
{
	unsigned long mpdr = 0;

	/*
	 * For Turbo mode, TPSEL, MAX_PDF and BRMM should
	 * be set in one write. IPG_PDF is constant
	 */
	switch (dvfs_op_reqd) {

	case CORE_NORMAL:
		/*
		 * Core is at 399 and we need to move to 500 or 532MHz. So,
		 * Set TPSEL to support Turbo mode and
		 * reprogram MAX an IPG clock dividers
		 * to maintain frequency at 100 or 133MHz
		 */
		mpdr = __raw_readl(MXC_CCM_MPDR0) | (MXC_CCM_MPDR0_TPSEL);
		/*
		 * Set MAX clock divider to 4 or 5 for AHB at 133 or 100MHz
		 */
#ifdef CONFIG_MOT_FEAT_32KHZ_GPT
                if (AHB_FREQ > 100 * MEGA_HERTZ) {
#else
		if ((CLOCK_TICK_RATE / MEGA_HERTZ) > 10) {
#endif /* CONFIG_MOT_FEAT_32KHZ_GPT */
			mpdr = (mpdr & (~MXC_CCM_MPDR0_MAX_PDF_MASK)) |
			    (MAX_PDF_4);
		} else {
			mpdr = (mpdr & (~MXC_CCM_MPDR0_MAX_PDF_MASK)) |
			    (MAX_PDF_5);
		}
		break;

	case CORE_TURBO:
		/*
		 * Core is at 500 or 532 and we need to move to 399. So,
		 * Clear TPSEL to support Turbo mode and
		 * reprogram MAX an IPG clock dividers
		 * to maintain frequency at 100 or 133MHz
		 */
		mpdr = __raw_readl(MXC_CCM_MPDR0) & (~MXC_CCM_MPDR0_TPSEL);
		/*
		 * Set MAX clock divider to 3 or 4 for AHB at 133 or 100MHz
		 */
#ifdef CONFIG_MOT_FEAT_32KHZ_GPT
                if (AHB_FREQ > 100 * MEGA_HERTZ) {
#else
		if ((CLOCK_TICK_RATE / MEGA_HERTZ) > 10) {
#endif /* CONFIG_MOT_FEAT_32KHZ_GPT */
			mpdr = (mpdr & (~MXC_CCM_MPDR0_MAX_PDF_MASK)) |
			    (MAX_PDF_3);
		} else {
			mpdr = (mpdr & (~MXC_CCM_MPDR0_MAX_PDF_MASK)) |
			    (MAX_PDF_4);
		}
		break;

	default:
		/* Do nothing */
		break;

	}

	/* Set BRMM to 0 to indicate turbo mode */
	mpdr = (mpdr & (~MXC_CCM_MPDR0_BRMM_MASK)) | MPDR0_BRMM_0;
	if (mxc_pm_chk_dfsp() == DFS_NO_SWITCH) {
		mxc_ccm_modify_reg(MXC_CCM_MPDR0, TURBO_MASK, mpdr);
	} else {
		return ERR_DFSP_SWITCH;
	}
	return 0;
}

/*!
 * To change AP core frequency and/or voltage suitably
 *
 * @param   dvfs_op    The values are,
 *                     CORE_NORMAL - ARM desired to run @399MHz, LoV (1.2V)
 *                     CORE_NORMAL_4 - ARM desired to run @100MHz, LoV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @133MHz, LoV (1.2V)
 *                                     if AHB is 133MHz
 *                     CORE_NORMAL_3 - ARM desired to run @133MHz, LoV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @171MHz, LoV (1.2V)
 *                                     if AHB is 133MHz
 *                     CORE_NORMAL_2 - ARM desired to run @160MHz, LoV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @200MHz, LoV (1.2V)
 *                                     if AHB is 133MHz
 *                     CORE_NORMAL_1 - ARM desired to run @200MHz, LoV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @240MHz, LoV (1.2V)
 *                                     if AHB is 133MHz
 *                     CORE_TURBO    - ARM desired to run @500MHz, HiV (1.6V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @532MHz, HiV (1.6V)
 *                                     if AHB is 133MHz
 *                     CORE_TURBO_1  - ARM desired to run @214MHz, HiV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @266MHz, HiV (1.2V)
 *                                     if AHB is 133MHz
 *                                     The reason for supporting 214MHz is that this
 *                                     is the only operating point closer to 266MHz. Rest
 *                                     of the Hi Voltage but low frequency operating
 *                                     are not supported because they are not the best
 *                                     power saving option. But can be added upon request.
 *
 * @return             Returns -ERR_FREQ_INVALID if none of the above choice
 *                     is selected
 */
static int mxc_pm_chgfreq(dvfs_op_point_t dvfs_op)
{
	int retval = 0;
#ifdef CONFIG_MOT_FEAT_PM
        char buf[80];
        int ms = 0;
#endif

	switch (dvfs_op) {

	case CORE_NORMAL_4:
		/*
		 * Set Core to run at 399MHz and then set brmm to 4
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * CORE_NORMAL (399MHz) case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_NORMAL);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_4);

		break;

	case CORE_NORMAL_3:
		/*
		 * Set Core to run at 399MHz and then set brmm to 4
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * CORE_NORMAL (399MHz) case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_NORMAL);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_3);

		break;

	case CORE_NORMAL_2:
		/*
		 * Set Core to run at 399MHz and then set brmm to 4
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * CORE_NORMAL (399MHz) case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_NORMAL);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_2);

		break;

	case CORE_NORMAL_1:
		/*
		 * Set Core to run at 399MHz and then set brmm to 4
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * CORE_NORMAL (399MHz) case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_NORMAL);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_1);

		break;

	case CORE_TURBO_1:
		/*
		 * Set Core to run at 532MHz or 500MHz and then set brmm to 1
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_CLEAR) {
			/*
			 * CORE_TURBO case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_TURBO);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_1);

		break;

	case CORE_NORMAL:

		/* Check TPSEL bit status */

		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * If TPSEL is set then the PLL freq is 500 or 532MHz.
			 * So, switch from TPLL to MPLL should occur.
			 */
			/*
			 * Set Normal Mode
			 */
			retval = mxc_pm_setturbo(CORE_TURBO);
			if (retval == ERR_DFSP_SWITCH) {
				return -ERR_DFSP_SWITCH;
			}

			/*
			 * Check if MPLL is locked
			 */
			while ((__raw_readl(MXC_CCM_MCR) & MXC_CCM_MCR_MPL) !=
			       MXC_CCM_MCR_MPL) ;

			/*
			 * Switch from TPLL to MPLL will occur now
			 * Wait until switch is complete. Interrupt
			 * will occur as soon as the switch completes
			 * by setting DFSI
			 */
#ifdef CONFIG_MOT_FEAT_PM
                        /* Delay 5s for switch complete with DFSI setting and DFSP clear */
                        while (ms < 5000)
                        {
                        	if (((__raw_readl(MXC_CCM_PMCR0) >> 24) & 1) == 1)
                        		break;
                        	mdelay(1);
                        	ms++;
                        }
                          
                        /* if DFSI bit is not set and DFSP bit is not cleared
                         * which indicates switch is not complete, then dump register
                         * and panic.
                         */
                        if (ms >= 5000 && mxc_pm_chk_dfsp() == DFS_SWITCH)
                        {
                          /* Get the register context */
                        	snprintf(buf, sizeof(buf),\
                                         " MPDR0=0x%x\n PMCR0=0x%x\n PMCR1=0x%x\n MCR=0x%x\n powerup_reason = 0x%08x\n", \
                                         __raw_readl(MXC_CCM_MPDR0), \
                                         __raw_readl(MXC_CCM_PMCR0), \
                                         __raw_readl(MXC_CCM_PMCR1), \
                                         __raw_readl(MXC_CCM_MCR), \
                                         mot_powerup_reason());
                        	panic("Switch from TPLL to MPLL failed.\n Current registers context:\n%s", buf);
                        }
                        
                        /* Check the DFSI and DFSP bit after switching complete */
                        if (mxc_pm_chk_dfsp() == DFS_SWITCH)
                        	printk(KERN_WARNING "DFSP bit not clear when TPLL to MPLL.\n");
                        if (((__raw_readl(MXC_CCM_PMCR0) >> 24) & 1) != 1)
                        	printk(KERN_WARNING "DFSI bit not set when TPLL to MPLL.\n");
#else
                        while (((__raw_readl(MXC_CCM_PMCR0) >> 24) & 1) != 1) ;
#endif

			/*
			 * Clear DFSI interrupt
			 */
			__raw_writel((__raw_readl(MXC_CCM_PMCR0) |
				      MXC_CCM_PMCR0_DFSI), MXC_CCM_PMCR0);

#ifdef CONFIG_MOT_FEAT_PM
			/*
			 * Disable the now unused TPLL
			 */
                	__raw_writel((__raw_readl(MXC_CCM_MCR) & 
					~MXC_CCM_MCR_TPE), MXC_CCM_MCR);
#endif

		} else {
			/*
			 * This implies PLL is already locked at 399MHz and
			 * previous transitions had been Integer scaling from 399MHz
			 */
			mxc_pm_setbrmm(MPDR0_BRMM_0);
		}
		break;

	case CORE_TURBO:

		/* Check TPSEL bit status */

		if (mxc_pm_chk_tpsel() == TPSEL_CLEAR) {
			/*
			 * If TPSEL is clear then the PLL freq is 399MHz.
			 * So, switch from MPLL to TPLL should occur.
			 */
                
#ifdef CONFIG_MOT_FEAT_PM
			/*
			 * Enable the TPLL
			 */
                	__raw_writel((__raw_readl(MXC_CCM_MCR) | 
					MXC_CCM_MCR_TPE), MXC_CCM_MCR);
#endif

			/*
			 * Set Turbo Mode
			 */
			retval = mxc_pm_setturbo(CORE_NORMAL);
			if (retval == ERR_DFSP_SWITCH) {
				return -ERR_DFSP_SWITCH;
			}
			/*
			 * Check if TPLL is locked
			 */
			while ((__raw_readl(MXC_CCM_MCR) & MXC_CCM_MCR_TPL) !=
			       MXC_CCM_MCR_TPL) ;

			/*
			 * Switch from MPLL to TPLL will occur now
			 * Wait until switch is complete. Interrupt
			 * will occur as soon as the switch completes
			 * by setting DFSI
			 */
#ifdef CONFIG_MOT_FEAT_PM
                        /* Delay 5s for switch complete with DFSI setting and DFSP clear */
                        while (ms < 5000)
                        {
                        	if (((__raw_readl(MXC_CCM_PMCR0) >> 24) & 1) == 1)
                        		break;
                        	mdelay(1);
                        	ms++;
                        }

                        /* if DFSI bit is not set and DFSP bit is not cleared
                         * which indicates switch is not complete, then dump register
                         * and panic.
                         */
                        if (ms >= 5000 && mxc_pm_chk_dfsp() == DFS_SWITCH)
                        {
                          /* Get the register context */
                        	snprintf(buf, sizeof(buf),\
                                         " MPDR0=0x%x\n PMCR0=0x%x\n PMCR1=0x%x\n MCR=0x%x\n powerup_reason = 0x%08x\n", \
                                         __raw_readl(MXC_CCM_MPDR0), \
                                         __raw_readl(MXC_CCM_PMCR0), \
                                         __raw_readl(MXC_CCM_PMCR1), \
                                         __raw_readl(MXC_CCM_MCR), \
                                         mot_powerup_reason());
                        	panic("Switch from MPLL to TPLL failed.\n Current registers context:\n%s", buf);
                        }

                        /* Check the DFSI and DFSP bit after switching complete */
                        if (mxc_pm_chk_dfsp() == DFS_SWITCH)
                        	printk(KERN_WARNING "DFSP bit not clear when MPLL to TPLL.\n");
                        if (((__raw_readl(MXC_CCM_PMCR0) >> 24) & 1) != 1)
                        	printk(KERN_WARNING "DFSI bit not set when MPLL to TPLL.\n");
#else
                        while (((__raw_readl(MXC_CCM_PMCR0) >> 24) & 1) != 1) ;
#endif

			/*
			 * Clear DFSI interrupt
			 */
			__raw_writel((__raw_readl(MXC_CCM_PMCR0) |
				      MXC_CCM_PMCR0_DFSI), MXC_CCM_PMCR0);

		} else {
			/*
			 * This implies PLL is already locked at 532MHz and
			 * previous transitions had been Integer scaling from 532MHz
			 */
			mxc_pm_setbrmm(MPDR0_BRMM_0);
		}
		break;

	default:
		return -ERR_FREQ_INVALID;
		break;
	}
	return 0;
}

/*!
 * To change AP core frequency and/or voltage suitably
 *
 * @param   armfreq    The desired ARM frequency
 * @param   ahbfreq    The desired AHB frequency: constant value 133 MHz
 * @param   ipfreq     The desired IP frequency : constant value 66.5 MHz
 *
 * @return             Returns -ERR_FREQ_INVALID on failure
 *                     Returns 0 on success
 */
int mxc_pm_dvfs(unsigned long armfreq, long ahbfreq, long ipfreq)
{
	int ret_val = -1;
	unsigned long flags;

	/* Acquiring Lock */
	spin_lock_irqsave(&dvfs_lock, flags);

	/* Changing Frequency/Voltage */
	ret_val = mxc_pm_chgfreq(armfreq);

	/* Releasing lock */
	spin_unlock_irqrestore(&dvfs_lock, flags);

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
	int gpt_intr_dis_flag = 0;
#ifdef CONFIG_MOT_WFN478
        unsigned long flags;
#endif
#ifdef CONFIG_MOT_FEAT_PM
	unsigned long mpdr0;
#endif

#ifdef CONFIG_MOT_FEAT_PM_STATS
        u32 avic_nipndh;
        u32 avic_nipndl;
        u32 rtc_before = 0; 
        u32 rtc_after = 0;
	mpm_test_point_t exit_test_point = 0;
#endif

#ifndef CONFIG_MOT_FEAT_PM
	if ((__raw_readl(AVIC_VECTOR) & MXC_WFI_ENABLE) == 0) {
		/*
		 * If JTAG is connected then WFI is not enabled
		 * So return
		 */
		return;
	}
#endif

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
	if ((__raw_readl(AVIC_INTENABLEL) & AVIC_TIMER_DIS) != AVIC_TIMER_DIS) {
		__raw_writel((__raw_readl(AVIC_INTENABLEL) & AVIC_TIMER_DIS),
			     AVIC_INTENABLEL);
		gpt_intr_dis_flag = 1;
	}

	switch (mode) {
	case WAIT_MODE:
		/*
		 * Handled by kernel using arch_idle()
		 * There are no LPM bits for WAIT mode which
		 * are set by software
		 */
#ifdef CONFIG_MOT_FEAT_PM_STATS
                exit_test_point = MPM_TEST_WAIT_EXIT;
#endif
		break;
	case DOZE_MODE:
		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & AVIC_TIMER_DIS),
			     LO_INTR_EN);
		/* Writing '10' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_MASK)) |
		     (MXC_CCM_MCR_LPM_2));
		__raw_writel(crm_ctrl, MXC_CCM_MCR);
#ifdef CONFIG_MOT_FEAT_PM_STATS
                exit_test_point = MPM_TEST_DOZE_EXIT;
#endif
		break;
	case STOP_MODE:
		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & AVIC_TIMER_DIS),
			     LO_INTR_EN);
#ifndef CONFIG_MACH_ARGONLVPHONE
		__raw_writel((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_DPE)),
			     MXC_CCM_MCR);
#endif
		/* Writing '11' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_MASK)) |
		     (MXC_CCM_MCR_LPM_3));
		__raw_writel(crm_ctrl, MXC_CCM_MCR);
#ifdef CONFIG_MOT_FEAT_PM_STATS
                exit_test_point = MPM_TEST_STOP_EXIT;
#endif
		break;
	case DSM_MODE:
		__raw_writel((__raw_readl(MXC_CCM_MCR) | (MXC_CCM_MCR_WBEN)),
			     MXC_CCM_MCR);
		__raw_writel((__raw_readl(MXC_CCM_MCR) | (MXC_CCM_MCR_SBYOS)),
			     MXC_CCM_MCR);

#ifdef CONFIG_MOT_FEAT_PM
                /* Enable EPIT Interrupt */
                __raw_writel((__raw_readl(LO_INTR_EN) | (1<<28)), LO_INTR_EN);
#else
		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & AVIC_TIMER_DIS),
			     LO_INTR_EN);
#endif
#ifndef CONFIG_MACH_ARGONLVPHONE
		__raw_writel((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_DPE)),
			     MXC_CCM_MCR);
#endif
		/* Writing '11' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_MASK)) |
		     (MXC_CCM_MCR_LPM_3));
		__raw_writel(crm_ctrl, MXC_CCM_MCR);

#ifdef CONFIG_MOT_FEAT_PM_STATS
                MPM_REPORT_TEST_POINT(1, MPM_TEST_DSM_ENTER);
                exit_test_point = MPM_TEST_DSM_EXIT;
#endif
		break;
	}
	/* Executing CP15 (Wait-for-Interrupt) Instruction */

#ifdef CONFIG_MOT_FEAT_PM_STATS 
        /* Read the clock before we go to wait/stop/dsm. */
	rtc_before = rtc_sw_get_internal_ticks();
#endif /* CONFIG_MOT_FEAT_PM_STATS */

#ifdef CONFIG_MOT_FEAT_PM
        mpdr0 = mxc_pm_l1_instr_cache_corruption_fix_before_wfi();

	/*
	 * Call the MU function that informs the DSP that we're about
	 * to enter DSM mode.  We do this to work around a chip erratum
	 * that can cause the MCU CCM to lock up if the DSP exits DOZE
	 * or STOP mode at just the wrong time (~136ns window).
	 *
	 * Due to DSP timing requirements, this call must be as close
	 * to the WFI instruction as possible.
	 */
	mxc_mu_inform_dsp_dsm_mode();
#endif

	cpu_do_idle();

#ifdef CONFIG_MOT_FEAT_PM
	mxc_mu_inform_dsp_run_mode();
        mxc_pm_l1_instr_cache_corruption_fix_after_wfi(mpdr0);
#endif

#ifdef CONFIG_MOT_FEAT_PM_STATS
	rtc_after = rtc_sw_get_internal_ticks();

        /*      
	 * Retrieve the pending interrupts.  These are the
	 * interrupt(s) that woke us up.
         */
        avic_nipndh = __raw_readl(AVIC_NIPNDH);
        avic_nipndl = __raw_readl(AVIC_NIPNDL);
#endif

	/* Enable timer interrupt */
	if (gpt_intr_dis_flag == 1) {
		__raw_writel((__raw_readl(AVIC_INTENABLEL) | AVIC_TIMER_EN),
			     AVIC_INTENABLEL);
	}
#ifdef CONFIG_MOT_WFN478
        local_irq_restore(flags);
#else
	local_irq_enable();
#endif

#ifdef CONFIG_MOT_FEAT_PM_STATS
        MPM_REPORT_TEST_POINT(5, exit_test_point, rtc_before, rtc_after,
                              avic_nipndl, avic_nipndh);
#endif
}

/*
 * This API is not supported on MXC91321
 */
int mxc_pm_intscale(long armfreq, long ahbfreq, long ipfreq)
{
	return -MXC_PM_API_NOT_SUPPORTED;
}

/*
 * This API is not supported on MXC91321
 */
int mxc_pm_pllscale(long armfreq, long ahbfreq, long ipfreq)
{
	return -MXC_PM_API_NOT_SUPPORTED;
}

/*!
 * This function is used to load the module.
 *
 * @return   Returns an Integer on success
 */
static int __init mxc_pm_init_module(void)
{
	int retval;

	retval =
	    request_irq(INT_CCM_MCU_DVFS, NULL, 0, "mxc_pm_mxc91321", NULL);

	/*
	 * Set DVSE bit to enable Dynamic Voltage Scaling
	 */
	retval = mxc_pm_dvsenable();
	if (retval == ERR_DFSP_SWITCH) {
		return -1;
	}

	/*
	 * IOMUX configuration for GPIO9 and GPIO20
	 */
#ifndef CONFIG_MOT_FEAT_PM
	gpio_pmhs_active();
#endif

#ifdef CONFIG_MOT_FEAT_PM
	/* Debug code until the MBM initializes the TPLL to 514 MHz */
	__raw_writel(0x00194817, MXC_CCM_TPCTL);
#endif

	printk("Low-Level PM Driver module loaded\n");
	return 0;
}

/*!
 * This function is used to unload the module
 */
static void __exit mxc_pm_cleanup_module(void)
{
#ifndef CONFIG_MOT_FEAT_PM
	gpio_pmhs_inactive();
#endif
	printk("Low-Level PM Driver module Unloaded\n");
}

module_init(mxc_pm_init_module);
module_exit(mxc_pm_cleanup_module);

EXPORT_SYMBOL(mxc_pm_dvfs);
EXPORT_SYMBOL(mxc_pm_lowpower);
EXPORT_SYMBOL(mxc_pm_pllscale);
EXPORT_SYMBOL(mxc_pm_intscale);

#ifdef CONFIG_MOT_FEAT_PM
EXPORT_SYMBOL(mxc_pm_l1_instr_cache_corruption_fix_before_wfi);
EXPORT_SYMBOL(mxc_pm_l1_instr_cache_corruption_fix_after_wfi);
#endif

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC91321 Low-level PM Driver");
MODULE_LICENSE("GPL");
