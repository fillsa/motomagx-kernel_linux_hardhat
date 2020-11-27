/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2008 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ------------------------
 * 10/06/2006   Motorola  Power management support 
 * 11/10/2006   Motorola  Add PLL current drain optimization.
 * 12/14/2006   Motorola  Change DVFS frequency code.
 * 12/25/2006   Motorola  Changed local_irq_disable/local_irq_enable to
 *                        local_irq_save/local_irq_restore.
 * 01/08/2007   Motorola  Implemented MGPER fix for crash after WFI.
 * 01/16/2007   Motorola  Reset DSM state machine upon wakeup.
 * 01/19/2007   Motorola  Resolved issue with 133Mhz operating point
 *                        and its divider ratios.
 * 02/14/2007   Motorola  Add code for DSM statistics gathering.
 * 02/26/2007   Motorola  Make DSM change dividers in two steps for 133.
 * 02/15/2007   Motorola  Cache optimization changes
 * 04/30/2007   Motorola  Wait for end of dithering cycle before
 *                        changing PLL settings.
 * 05/12/2007   Motorola  Changed comments.
 * 06/05/2007   Motorola  Changed comments.
 * 07/31/2007   Motorola  Fix the problem that KernelPanic when inserted USB cable with PC.
 * 08/09/2007   Motorola  Add comments.
 * 25/09/2007   Motorola  Change max value of PPM for PLL dithering from 1000 to 2000.
 * 11/23/2007   Motorola  Add BT LED debug option processing
 * 01/08/2008   Motorola  Changed some debug information. 
 */



/*!
 * @file mxc_pm.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for the CRM_AP, DPLL and DSM modules.
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
#include <linux/rtc.h>
#endif
#include <asm/hardware.h>
#include <asm/arch/mxc_pm.h>
#include <asm/arch/clock.h>
#include <asm/arch/spba.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/mach/time.h>
#include <asm/arch/gpio.h>

#include "crm_regs.h"

#ifdef CONFIG_MOT_FEAT_PM_STATS
#include <linux/mpm.h>
#endif
#ifdef CONFIG_MOT_FEAT_PM
#include <asm/cacheflush.h>
#include <asm/div64.h>
#include <asm/bootinfo.h>
#endif /* CONFIG_MOT_FEAT_PM */

/*!
 * Declaring lock to prevent request to change core
 * frequency when previous transition is not finished
 */
static DEFINE_RAW_SPINLOCK(dvfs_lock);

/*
 * Test to see if this part is Pass 2 silicon.
 */
#ifndef CONFIG_MOT_FEAT_PM
#define MXC91231_PASS_1()                 (system_rev <  CHIP_REV_2_0)
#endif
#define MXC91231_PASS_2_OR_GREATER()      (system_rev >= CHIP_REV_2_0)

/*
 * Voltage Levels
 */
#define LO_VOLT                 0
#define HI_VOLT                 1

/*
 * MXC_CRMAP_AMCR ACK bypass signals
 */
#define AMCR_ACK_BYP            0x007B0000

/*
 * AVIC Lo Interrupt enable register for low-power modes
 */
#define LO_INTR_EN IO_ADDRESS(AVIC_BASE_ADDR + 0x14)

/*
 * Configuration Options
 */
/* Only one of the following options can be chosen */
#undef CONFIG_PLL_NOCLOCK	/* This option is broken in HW on MXC91231 P1 & P2 */

#ifdef CONFIG_MOT_FEAT_PM
#undef CONFIG_PLL_PATREF        /* This option is no longer supported */
#define CONFIG_ALT_PLL
#else
#define CONFIG_PLL_PATREF 1
#undef CONFIG_ALT_PLL		/* This option is currently not implemented */
#endif


/*
 * The following options used for DSM
 */
#define COUNT32_RESET           1
#define COUNT32_NOT_RESET       0
#define CONFIG_RESET_COUNT32    COUNT32_RESET

/* Software Workarounds for Hardware issues on MXC91231 P1 */
/* If SDRAMC issue exists then CRM issue will exist */
#define NO_SDRAMC_RATIO_ISSUE   0
#define CRM_ISSUE               1

#ifdef CONFIG_MOT_FEAT_PM
/*
 * Address offsets of the CRM_COM registers
 */
#define CRM_COM_CSCR_PPD1                       0x08000000
#endif /* CONFIG_MOT_FEAT_PM */

#ifndef CONFIG_MOT_FEAT_PM
/*
 * Address offsets of DPLL registers
 */
#define PLL0_DP_CTL                             (MXC_PLL0_BASE + MXC_PLL_DP_CTL)
#define PLL0_DP_CONFIG                          (MXC_PLL0_BASE + MXC_PLL_DP_CONFIG)
#define PLL0_DP_OP                              (MXC_PLL0_BASE + MXC_PLL_DP_OP)
#define PLL0_DP_MFD                             (MXC_PLL0_BASE + MXC_PLL_DP_MFD)
#define PLL0_DP_MFN                             (MXC_PLL0_BASE + MXC_PLL_DP_MFN)
#define PLL0_DP_HFSOP                           (MXC_PLL0_BASE + MXC_PLL_DP_HFS_OP)
#define PLL0_DP_HFSMFD                          (MXC_PLL0_BASE + MXC_PLL_DP_HFS_MFD)
#define PLL0_DP_HFSMFN                          (MXC_PLL0_BASE + MXC_PLL_DP_HFS_MFN)
#endif /* CONFIG_MOT_FEAT_PM */

#ifdef CONFIG_MOT_FEAT_PM
#define TOG_DIS                                 0x00020000
#define TOG_EN                                  0x00010000
#define TOG_SEL                                 0x80000000
#define TOGC_CNT_VALUE                          0x00000A28
#define TOGC_CNT_MASK                           0x0000FFFF

/*
 * De-sense feature: Max value of PPM for PLL dithering
 */
#define MAX_PLL_PPM_VALUE                       2000

/* ESDCTL MDDREN bit description used for automatic SDR / DDR detection */
#define ESDCTL_MISC                             IO_ADDRESS(ESDCTL_BASE_ADDR + 0x10)
#define ESDCTL_MISC_MDDREN                      0x00000004
#define mxc_pm_ddram_detected() \
	((__raw_readl(ESDCTL_MISC) & ESDCTL_MISC_MDDREN) == ESDCTL_MISC_MDDREN)

/* Bits used for aggressive clock gating */
#define APRA_ALL_CLK_EN         (MXC_CRMAP_APRA_SIMEN   | \
                                 MXC_CRMAP_APRA_UART1EN | \
                                 MXC_CRMAP_APRA_UART2EN | \
                                 MXC_CRMAP_APRA_UART3EN | \
                                 MXC_CRMAP_APRA_SAHARA_DIV2_CLK_EN_PASS2)

#define APRB_ALL_CLK_EN         (MXC_CRMAP_APRB_SDHC1EN | \
                                 MXC_CRMAP_APRB_SDHC2EN)

#define ACDER1_ALL_CLK_EN       (MXC_CRMAP_ACDER1_CSIEN_PASS2 | \
                                 MXC_CRMAP_ACDER1_SSI1EN | \
                                 MXC_CRMAP_ACDER1_SSI2EN)

#define ACDER2_ALL_CLK_EN       (MXC_CRMAP_ACDER2_USBEN | \
                                 MXC_CRMAP_ACDER2_NFCEN)

#endif /* CONFIG_MOT_FEAT_PM */


/*
 * Bits used for mask
 */
#define AP_DELAY_MASK                           0x0000ffff
#ifdef CONFIG_MOT_WFN421
#define AP_DELAY_SETTING                        0x00030000
#else
#define AP_DELAY_SETTING                        0x00090000
#endif /* CONFIG_MOT_WFN421 */
#define ACDR_MASK                               0xFFFFF000
#define AVIC_TIMER_INTR_DIS                     0xCFFFFFFF
#define AVIC_TIMER_INTR_EN                      0x30000000
#define ENABLE_WELL_BIAS                        0x1

/*
 * DPLL0 HFS register values
 */
#define DP_HFS_OP                               0x000000A0
#define DP_HFS_MFD                              0x0000000D
#define DP_HFS_MFN                              0x00000003

/*
 * Timer & Counter values for various DSM registers
 */
#define SLEEP_TIME                              0x00000002

/*
 * WARM PERIOD - 150 ckil cycles - approx~= 4ms
 */
/* Current drain optimization : crystal WARM period set to 92 ckil cycles - approx~= 2.8ms */
#ifdef CONFIG_MOT_FEAT_PM
#define CALC_WAKE_TIME                          0x0000005c
#else
#define CALC_WAKE_TIME                          0x00000096
#endif
/*
 * LOCK PERIOD - 120 usec needed for PLL relock
 */
#define CALC_LOCK_TIME                          0x00000004

/*
 * ARM:AHB:IPG Clock Ratios
 */
#ifndef CONFIG_MOT_FEAT_PM

#define ARM_AHB_IPG_RATIO_HI                    0x00000848
#define ARM_AHB_IPG_RATIO_LO                    0x00000836
#define ARM_AHB_IPG_RATIO_266                   0x00000824

#define ARM_AHB_IPG_CRMRATIO_532                0x00000048
#define ARM_AHB_IPG_CRMRATIO_399                0x00000036
#define ARM_AHB_IPG_CRMRATIO_266                0x00000024

#else  /* CONFIG_MOT_FEAT_PM */

/*
 * Note that the following ratio define names are based on the divider
 * ratio itself, not on the targeted operating point.
 */
#define ARM_AHB_IPG_RATIO_112                   0x00000812
#define ARM_AHB_IPG_RATIO_124                   0x00000824
#define ARM_AHB_IPG_RATIO_136                   0x00000836
#define ARM_AHB_IPG_RATIO_148                   0x00000848
#define ARM_AHB_IPG_RATIO_224                   0x00000024
#define ARM_AHB_IPG_RATIO_248                   0x00000048
#define ARM_AHB_IPG_RATIO_212                   0x00000012
#endif /* CONFIG_MOT_FEAT_PM */

/*
 * Define values used to access the L2 cache controller.
 *
 * Define the number of wait cycles required by the L2 cache for
 * various core frequencies.
 */
#ifdef CONFIG_MOT_FEAT_PM

#define L2CC_CONTROLREG 0x00000100
#define L2CC_AUXREG     0x00000104

#define L2CC_ENABLED 0x01

#define L2CC_LATENCY_MASK 0xFFFFFFC0

#define L2CC_LATENCY_133 0x00000009
#define L2CC_LATENCY_266 0x00000012
#define L2CC_LATENCY_399 0x0000001B
#define L2CC_LATENCY_532 0x00000024

#endif /* CONFIG_MOT_FEAT_PM */

/*!
 * Enumerations of scaling types - enabling or disabling DFS dividers
 */
typedef enum {
	DFS_ENABLE = 0,
	DFS_DISABLE,
} dvfs_scale_t;

#ifdef CONFIG_MOT_FEAT_PM
/*
 * Note that dvfs_op_point_t (defined here) must be kept in sync with
 * dvfs_op_point_index_t (defined in mxc_pm.h).
 */

/*!
 * Enumerations of operating points
 */
typedef enum {
	CORE_133 = CORE_FREQ_133,
	CORE_266 = CORE_FREQ_266,
	CORE_399 = CORE_FREQ_399,
	CORE_532 = CORE_FREQ_532,
} dvfs_op_point_t;
#else
/*!
 * Enumerations of operating points
 */
typedef enum {
        CORE_133 = 133,
        CORE_266 = 266,
        CORE_399 = 399,
        CORE_532 = 532,
} dvfs_op_point_t;
#endif

#ifdef CONFIG_MOT_FEAT_PM
/*
 * cur_dvfs_op_point_index is the current operating point index.
 */
static dvfs_op_point_index_t cur_dvfs_op_point_index;
#endif /* CONFIG_MOT_FEAT_PM */

/*
 * Tree level clock gating
 */
static unsigned int gate_stop_wait[NUM_GATE_CTRL] =
    { MXC_ACGCR_ACG0_STOP_WAIT, MXC_ACGCR_ACG1_STOP_WAIT,
	MXC_ACGCR_ACG3_STOP_WAIT,
	MXC_ACGCR_ACG4_STOP_WAIT, MXC_ACGCR_ACG5_STOP_WAIT,
	MXC_ACGCR_ACG6_STOP_WAIT
};
static unsigned int gate_stop[NUM_GATE_CTRL] =
    { MXC_ACGCR_ACG0_STOP, MXC_ACGCR_ACG1_STOP, MXC_ACGCR_ACG3_STOP,
	MXC_ACGCR_ACG4_STOP, MXC_ACGCR_ACG5_STOP, MXC_ACGCR_ACG6_STOP
};
static unsigned int gate_run[NUM_GATE_CTRL] =
    { MXC_ACGCR_ACG0_RUN, MXC_ACGCR_ACG1_RUN, MXC_ACGCR_ACG3_RUN,
	MXC_ACGCR_ACG4_RUN, MXC_ACGCR_ACG5_RUN, MXC_ACGCR_ACG6_RUN
};

#ifdef CONFIG_MOT_FEAT_PM
static unsigned int pll_dp_ctlreg[NUM_PLL] = {PLL0_DP_CTL, PLL1_DP_CTL, PLL2_DP_CTL};
static unsigned int pll_dp_hfs_opreg[NUM_PLL] = {PLL0_DP_HFSOP, PLL1_DP_HFSOP, PLL2_DP_HFSOP};
static unsigned int pll_dp_hfs_mfnreg[NUM_PLL] = {PLL0_DP_HFSMFN, PLL1_DP_HFSMFN, PLL2_DP_HFSMFN};
static unsigned int pll_dp_hfs_mfdreg[NUM_PLL] = {PLL0_DP_HFSMFD, PLL1_DP_HFSMFD, PLL2_DP_HFSMFD};

static unsigned int pll_dp_config[NUM_PLL] = {PLL0_DP_CONFIG, PLL1_DP_CONFIG, PLL2_DP_CONFIG};
static unsigned int pll_dp_mfn_minus_reg[NUM_PLL] = {PLL0_DP_MFN_MINUS, PLL1_DP_MFN_MINUS, PLL2_DP_MFN_MINUS};
static unsigned int pll_dp_mfn_plus_reg[NUM_PLL] = {PLL0_DP_MFN_PLUS, PLL1_DP_MFN_PLUS, PLL2_DP_MFN_PLUS};
static unsigned int pll_dp_mfn_togc[NUM_PLL] = {PLL0_DP_MFN_TOGC, PLL1_DP_MFN_TOGC, PLL2_DP_MFN_TOGC};
static unsigned int pll_dp_destat[NUM_PLL] = {PLL0_DP_DESTAT, PLL1_DP_DESTAT, PLL2_DP_DESTAT};

static unsigned int l2cc_latency[NUM_DVFSOP_INDEXES] = {
                                                           L2CC_LATENCY_133, 
                                                           L2CC_LATENCY_266, 
                                                           L2CC_LATENCY_399, 
                                                           L2CC_LATENCY_532
                                                       };

/*
 * The opinfo structure contains the information required
 * to change operating points.  This information only applies
 * to PLL0 (MCUPLL).
 * The structure is initialized with configured values.
 * The remaining fields are calculated at run time.
 * Refer to the SCMA11 Detailed Technical Specification for additional
 * information on the fields that are initialized in this structure.
 */
static ap_pll_mfn_values_t opinfo[NUM_DVFSOP_INDEXES] = {
        /* 133Mhz */ {
                       ap_pll_dp_hfs_op:    0x00000050,
                       ap_pll_dp_hfs_mfn:   0x003B13B1,
                       ap_pll_dp_hfs_mfd:   0x01FFFFFE,
		       /*
			* We'd like the ratio to be 1:1:2, but the PLL
			* output clock must be at least 160Mhz, so we
			* choose 2:1:2 instead and set the PLL output
			* clock at 266Mhz.
			*/
		       divider_ratio:       ARM_AHB_IPG_RATIO_212,
                     },
        /* 266Mhz */ {
                       ap_pll_dp_hfs_op:    0x00000050,
                       ap_pll_dp_hfs_mfn:   0x003B13B1,
                       ap_pll_dp_hfs_mfd:   0x01FFFFFE,
		       divider_ratio:       ARM_AHB_IPG_RATIO_124,
                     },
        /* 399Mhz */ {
                       ap_pll_dp_hfs_op:    0x00000070,
                       ap_pll_dp_hfs_mfn:   0x01589D89,
                       ap_pll_dp_hfs_mfd:   0x01FFFFFE,
		       divider_ratio:       ARM_AHB_IPG_RATIO_136,
                     },
        /* 532Mhz */ {
                       ap_pll_dp_hfs_op:    0x000000A0,
                       ap_pll_dp_hfs_mfn:   0x00762762,
                       ap_pll_dp_hfs_mfd:   0x01FFFFFE,
		       divider_ratio:       ARM_AHB_IPG_RATIO_148,
                     },
};

/*
 * The ap_usb_pll_mfn_values structure contains information for the
 * USBPLL.
 */
static ap_pll_mfn_values_t ap_usb_pll_mfn_values = {
       ap_pll_dp_hfs_op:    0x00000082,
       ap_pll_dp_hfs_mfn:   0x009D89D8,
       ap_pll_dp_hfs_mfd:   0x01FFFFFE,
};

/* Prototypes */
static int mxc_pm_dither_pll_setup(enum plls pll_number,
                                   ap_pll_mfn_values_t *ap_pll_mfn_values);

int mxc_pm_dither_control(int enable, enum plls pll_number);

/*
 * SEXT does a sign extension on the given value. nbits is the number
 * of bits consumed by val, including the sign bit. nbits larger than
 * 31 doesn't make sense.  This macro just shifts the bits up to the
 * top of the word and then back down.  This causes the sign bit to
 * propagate when shifting back.  Note that ANSI C characterizes this
 * behavior as implementation defined.
 */
#define SEXT(nbits,val)            ((((int)(val)) << (32-(nbits))) >> (32-(nbits)))

/*
 * Convert 27 bits from MFNxxx registers ("false signed values")
 * to s32 absolute value.  Sign extend the value and then take
 * its absolute value.  
 */
#define DPLL_MFN_TO_ABSOLUTE(val)  abs(SEXT(27,val))

/*
 * Switching to PLL1 when the AP goes to DSM might cause some
 * instability, so we're providing a means to disable this behavior
 * using pll0_on_in_ap_dsm in the Linux command line.  Add
 * "pll0_on_in_ap_dsm=on" to the command line to disable the behavior.
 * pll0_on_in_ap_dsm is off by default.
 *
 * asm/setup.h is only needed for the __early_param code.
 */
#include <asm/setup.h>
static int pll0_on_in_ap_dsm = 0;

static void __init set_pll0_on_in_ap_dsm(char **p)
{
        if (memcmp(*p, "on", 2) == 0) {
                pll0_on_in_ap_dsm = 1;
                *p += 2;
        } else if (memcmp(*p, "off", 3) == 0) {
                pll0_on_in_ap_dsm = 0;
                *p += 3;
        }
}

__early_param("pll0_on_in_ap_dsm=", set_pll0_on_in_ap_dsm);

int pll0_off_in_ap_dsm(void)
{
        return (pll0_on_in_ap_dsm == 0);
}


/*
 * mxc_pm_chgfreq_enabled determines when frequency changes are allowed.
 */
static int mxc_pm_chgfreq_enabled(dvfs_op_point_t newop, dvfs_op_point_t curop)
{
	/*
	 * No frequency changes of any kind are allowed if the system
	 * contains DDRAM.
	 */
	if (mxc_pm_ddram_detected())
		return 0;

	return 1;
}
#endif /* CONFIG_MOT_FEAT_PM */

/*!
 * To check the status of the AP domain core voltage
 *
 * @return      HI_VOLT         Returns HI_VOLT indicating that core
 *                              is at Hi voltage (1.6V)
 * @return      LO_VOLT         Returns LO_VOLT indicating that core
 *                              is at Lo voltage (1.2V)
 */
static int mxc_pm_chkvoltage(void)
{
	/*
	 * Read the CRM_AP DFS register to get the current
	 * AP voltage
	 */
	if ((__raw_readl(MXC_CRMAP_ADCR) & MXC_CRMAP_ADCR_VSTAT) != 0) {
		return LO_VOLT;
	} else {
		return HI_VOLT;
	}
}

#ifndef CONFIG_MOT_FEAT_PM
/*!
 * To check the status of the DFS_DIV_EN bit
 *
 * @return      DFS_ENABLE      Returns DFS_ENABLE indicating this bit
 *                              is set - implies Integer scaling is
 *                              enabled using DFS dividers
 * @return      DFS_DISABLE     Returns DFS_DISABLE indicating this bit
 *                              is set - implies PLL scaling is used
 *
 */
static dvfs_scale_t mxc_pm_chkdfs(void)
{
	if ((__raw_readl(MXC_CRMAP_ADCR) & MXC_CRMAP_ADCR_DFS_DIVEN) != 0) {
		return DFS_ENABLE;
	} else {
		return DFS_DISABLE;
	}
}
#endif /* CONFIG_MOT_FEAT_PM */

/*!
 * To change the core voltage from Hi to Lo or viceversa
 *
 * @param       val       HI_VOLT - Indicating that core requires
 *                        Hi voltage (1.6V)
 *                        LO_VOLT - Indicating that core requires
 *                        Lo voltage (1.2V)
 *
 */
static void mxc_pm_chgvoltage(int val)
{

	unsigned long dsm_crm_ctrl;

	switch (val) {

	case LO_VOLT:

		/* Move to Lo Voltage */
		if ((__raw_readl(MXC_CRMAP_ADCR) & MXC_CRMAP_ADCR_TSTAT) == 0) {
			dsm_crm_ctrl =
			    __raw_readl(MXC_DSM_CRM_CONTROL) |
			    (MXC_DSM_CRM_CTRL_DVFS_VCTRL);
			__raw_writel(dsm_crm_ctrl, MXC_DSM_CRM_CONTROL);
		}
		break;

	case HI_VOLT:

		/* Setting ap_delay */
		__raw_writel(((__raw_readl(MXC_CRMAP_ADCR) & AP_DELAY_MASK) |
			      AP_DELAY_SETTING), MXC_CRMAP_ADCR);

		/* Move to Hi Voltage */
		dsm_crm_ctrl =
		    __raw_readl(MXC_DSM_CRM_CONTROL) &
		    (~MXC_DSM_CRM_CTRL_DVFS_VCTRL);
		__raw_writel(dsm_crm_ctrl, MXC_DSM_CRM_CONTROL);

		/* Wait on the TSTAT timer for voltage to stabilize */
		do {
			udelay(1);
		} while (__raw_readl(MXC_CRMAP_ADCR) & MXC_CRMAP_ADCR_TSTAT);
		break;

	default:
		break;
	}
}

/*!
 * To choose one of the scaling techniques either Integer scaling or
 * PLL scaling by setting or clearing DFS_DIV_EN bit
 *
 * @param      dfs     DFS_ENABLE - Enables Integer scaling -
 *                     to use DFS dividers by setting DFS_DIV_EN bit
 *
 *                     DFS_DISABLE - Enables PLL scaling -
 *                     to not use DFS dividers by clearing
 *                     DFS_DIV_EN bit and also setting one of the
 *                     PLL scaling methods,
 *                     PLL using pat_ref or
 *                     PLL using no_clock or - not supported as it is broken in HW
 *                     PLL using ALT_PLL -  now supported
 *
 */
static void mxc_pm_config_scale(dvfs_scale_t dfs)
{
	unsigned long adcr;

	if (dfs == DFS_ENABLE) {
		/*
		 * Set DFS_DIV_EN bit and clear DIV_BYP bit to enable Integer scaling
		 * Also, clear the ALT_PLL bit
		 */
		adcr =
		    ((__raw_readl(MXC_CRMAP_ADCR) | MXC_CRMAP_ADCR_DFS_DIVEN) &
		     ~MXC_CRMAP_ADCR_DIV_BYP) & (~MXC_CRMAP_ADCR_ALT_PLL);
		__raw_writel(adcr, MXC_CRMAP_ADCR);
	} else if (dfs == DFS_DISABLE) {
		/* Disable using DFS divider to enable PLL scaling */

#if defined (CONFIG_PLL_PATREF)
		/*
		 * pat_ref clock is used while PLL is relocking. Set CLK_ON bit to
		 * use pat_ref. Also, set the DIV_BYP bit to bypass DFS divider
		 * and clear ALT_PLL bit. Clear DFS_DIV_EN bit to select PLL scaling
		 */
		adcr =
		    (((__raw_readl(MXC_CRMAP_ADCR) | MXC_CRMAP_ADCR_DIV_BYP) |
		      MXC_CRMAP_ADCR_CLK_ON) & (~MXC_CRMAP_ADCR_ALT_PLL)) &
		    (~MXC_CRMAP_ADCR_DFS_DIVEN);
		__raw_writel(adcr, MXC_CRMAP_ADCR);
#elif defined (CONFIG_PLL_NOCLOCK)
		/*
		 * no clock is used while PLL is relocking. Clear CLK_ON bit to
		 * not use pat_ref. Also, set the DIV_BYP bit to bypass DFS divider
		 * and clear ALT_PLL bit. Clear DFS_DIV_EN bit to select PLL scaling
		 */
		adcr =
		    (((__raw_readl(MXC_CRMAP_ADCR) | MXC_CRMAP_ADCR_DIV_BYP) &
		      (~MXC_CRMAP_ADCR_CLK_ON)) & (~MXC_CRMAP_ADCR_ALT_PLL))
		    & (~MXC_CRMAP_ADCR_DFS_DIVEN);
		__raw_writel(adcr, MXC_CRMAP_ADCR);
#elif defined (CONFIG_ALT_PLL)
		/*
		 * Clock source from an alternate PLL is used while
		 * PLL is relocking. Set ALT_PLL bit. Clear DFS_DIV_EN bit
		 * to select PLL scaling. Set DIV_BYP bit to bypass DFS divider.
		 * Clear CLK_ON to not use pat_ref
		 */
		adcr = ((__raw_readl(MXC_CRMAP_ADCR) | MXC_CRMAP_ADCR_ALT_PLL) &
			(~MXC_CRMAP_ADCR_DFS_DIVEN)) & (~MXC_CRMAP_ADCR_CLK_ON);
		__raw_writel(adcr, MXC_CRMAP_ADCR);
#else
#error You must choose one of the Scaling Options
#endif
	}
}

#ifndef CONFIG_MOT_FEAT_PM
/*!
 * Setting possible LFDF value for Integer Scaling
 *
 * @param   value      The divider value to be set for the DFS block.
 *                     Possible values are 0,2,4,8.
 *                     This occurs during Integer scaling
 *
 */
static void mxc_pm_setlfdf(int value)
{
	unsigned long adcr_val;

	adcr_val = (__raw_readl(MXC_CRMAP_ADCR) & (~MXC_CRMAP_ADCR_LFDF_MASK)) |
	    value;
	__raw_writel(adcr_val, MXC_CRMAP_ADCR);
}

/*!
 * Used on MXC91231 P1 to set MFI, MFN, MFD values in LFS PLL register.
 * Using this the output frequency whose value is calculated using,
 * 2 * REF_FREQ * (MF / PDF), where
 * REF_FREQ is 26 Mhz
 * MF = MFI + (MFN / MFD)
 * PDF is assumed to be 1
 *
 * @param   armfreq    The desired ARM frequency
 *
 * @return             Returns 0 on success or
 *                     Returns -1 on error
 */
static void mxc_pm_calc_mf_lfs(long armfreq)
{
	signed long dp_opval, mfn, mfd, mfi, ref_freq;
	int pdf;

	ref_freq = mxc_get_clocks(CKIH_CLK) / MEGA_HERTZ;
	/*
	 * Each PLL is  identified using base address and
	 * PLL0 is assumed to be selected for AP domain
	 * MF is  calculated here. PDF is set to 1 which is divide by 2
	 * by default by hardware
	 */
	pdf = (__raw_readl(PLL0_DP_OP)) & 0xF;
	/*
	 * The following steps calculate the Whole number and fractional
	 * part of the output frequency using integer arithmetic.
	 */
	mfi = (armfreq * (pdf + 1)) / (2 * ref_freq);
	mfn = armfreq - (2 * mfi * ref_freq);
	mfd = (2 * ref_freq);
	mfi = (mfi <= 5) ? 5 : mfi;
	mfn = (mfn <= 0x4000000) ? mfn : (mfn - 0x10000000);
	dp_opval = (mfi << 4) | (0x0000000F & __raw_readl(PLL0_DP_OP));
	__raw_writel(dp_opval, PLL0_DP_OP);
	__raw_writel(mfn, PLL0_DP_MFN);
	__raw_writel(mfd, PLL0_DP_MFD);

	/* Wait until PLL relocks */
	while ((__raw_readl(PLL0_DP_CTL) & MXC_PLL_DP_CTL_LRF) !=
	       MXC_PLL_DP_CTL_LRF) ;
}
#endif /* CONFIG_MOT_FEAT_PM */

#ifdef CONFIG_MOT_FEAT_PM
/*
 * dvfsop2index converts a dvfs_op_point_t to dvfs_op_point_index_t.
 */
static dvfs_op_point_index_t dvfsop2index (dvfs_op_point_t dvfs_op)
{
	dvfs_op_point_index_t dvfs_indx = INDX_INVAL;

	switch (dvfs_op) {
	case CORE_133:  dvfs_indx = INDX_133; break;
	case CORE_266:  dvfs_indx = INDX_266; break;
	case CORE_399:  dvfs_indx = INDX_399; break;
	case CORE_532:  dvfs_indx = INDX_532; break;
	}

	return dvfs_indx;
}

/*
 * mxc_pm_set_l2cc_latency is a function that sets the read/write
 * latency of the L2 cache controller.
 *
 * mxc_pm_set_l2cc_latency must be called with interrupts disabled.
 *
 * dvfs_op_index - requested operating point index.
 *
 * mxc_pm_set_l2cc_latency returns nothing.
 */
static void mxc_pm_set_l2cc_latency (dvfs_op_point_index_t dvfs_op_index)
{
    unsigned long control_reg;
    unsigned long auxilary_reg;

    flush_cache_all();
    __asm__("mcr p15, 0, r0, c7, c10, 4\n" : : : "r0"); /* Drain the write buffer */

    control_reg  = __raw_readl(L2CC_BASE_ADDR_VIRT+L2CC_CONTROLREG);
    control_reg &= ~L2CC_ENABLED;

    __raw_writel(control_reg, L2CC_BASE_ADDR_VIRT+L2CC_CONTROLREG);

    __asm__("mcr p15, 0, r0, c7, c10, 4\n" : : : "r0"); /* Drain the write buffer */

    auxilary_reg  = __raw_readl(L2CC_BASE_ADDR_VIRT+L2CC_AUXREG);
    auxilary_reg &= L2CC_LATENCY_MASK;
    auxilary_reg |= l2cc_latency[dvfs_op_index];

    __raw_writel(auxilary_reg, L2CC_BASE_ADDR_VIRT+L2CC_AUXREG);

    __asm__("mcr p15, 0, r0, c7, c10, 4\n" : : : "r0"); /* Drain the write buffer */

    control_reg = __raw_readl(L2CC_BASE_ADDR_VIRT+L2CC_CONTROLREG);
    control_reg |= L2CC_ENABLED;

    __raw_writel(control_reg, L2CC_BASE_ADDR_VIRT+L2CC_CONTROLREG);
}

/* mxc_gpt_udelay is a replace of udelay make use of GPT
 * 
 * usvalue - time want to delay. (us)
 *
 * Description - LATCH/10ms = ticks_need_to_wait/ (how long we will wait)ms
 */
static void mxc_gpt_udelay( unsigned long usvalue )
{
	unsigned long now_tick, ticks_to_wait, ticks_delta;

	now_tick = __raw_readl(MXC_GPT_GPTCNT);
	ticks_to_wait = ( usvalue * LATCH * HZ)/1000000;
	
	do{
		ticks_delta =  __raw_readl(MXC_GPT_GPTCNT) - now_tick;
	}while ( ticks_delta < ticks_to_wait );
	
}
	
/*
 * mxc_pm_chgfreq_common is a common function that does the real work
 * of changing frequencies.
 *
 * mxc_pm_chgfreq_common must be called with interrupts disabled.
 *
 * dvfs_op_index	- dvfs_op_index is the requested
 *                        operating point index.
 *
 * mxc_pm_chgfreq_common returns nothing.
 */
static void mxc_pm_chgfreq_common(dvfs_op_point_index_t dvfs_op_index)
{
        unsigned long ascsr;

	if (dvfs_op_index == cur_dvfs_op_point_index)
		return;

	if ((dvfs_op_index < INDX_FIRST) || (dvfs_op_index >= NUM_DVFSOP_INDEXES))
		return;

	/* Request USBPLL.  When this returns, the USBPLL is started and locked. */
	mxc_pll_request_pll(USBPLL);

	/*
         * If we are switching to a frequency that requires a longer latency,
         * make the switch now so when the frequency change is complete the cache 
         * still behaves correctly.
         */
	if(l2cc_latency[cur_dvfs_op_point_index] < l2cc_latency[dvfs_op_index])
		mxc_pm_set_l2cc_latency(dvfs_op_index);

	/*
	 * ARM:AHB:IPG divider ratio = 1:4:8
	 *
	 * We are about to switch to PLL2 (USBPLL).  We set the PLL
	 * dividers to 1:4:8 because it satisfies the requirements
	 * that SDRAM is <= 133Mhz and the SDRAM:IP frequency is 2:1
	 * for all PLL0 frequencies that we use (266/399/532) *and*
	 * the PLL2 frequency (144Mhz).  If we were running at 133Mhz,
	 * we'll slow down the SDRAM and IP clocks pretty well, but
	 * we're not running on PLL2 for very long, so it's OK.
	 */
	if (cur_dvfs_op_point_index == INDX_133) {
		/*
		 * If we get here, then we are currently operating at
		 * the 133 operating point.
		 *
		 * Note that all operating points except 133 use a
		 * CPU divider ratio of 1, so the switch can be done
		 * in one step.
		 *
		 * Freescale recommends that returning from a divider
		 * ratio of 2:1:2 (for operating point 133) should be
		 * done in two steps.  The first step is 2:4:8.
		 *
		 * We delay after we write to ensure that the dividers
		 * are propagated in the correct order.  We are
		 * running now at 2:1:2.  We move to 2:4:8 which
		 * changes only the AHB/IPG dividers.  Then we make
		 * the final change which changes the CPU divider.
		 *
		 * Failure to accomplish the divider ratio switch
		 * in two steps results in system instability.
		 */
		__raw_writel(ARM_AHB_IPG_RATIO_248, MXC_CRMAP_ACDR);
		udelay(1);
	}
	__raw_writel(ARM_AHB_IPG_RATIO_148, MXC_CRMAP_ACDR);

	/* Switch AP domain to PLL2 (USBPLL) */
	ascsr = ((__raw_readl(MXC_CRMAP_ASCSR) & ~ASCSR_APSEL_0) | ASCSR_APSEL_1);
        __raw_writel(ascsr, MXC_CRMAP_ASCSR);

	/*
	 * If the change in frequency allows us to move to a faster latency, make
	 * the switch after the change in frequency. This allows the cache up to 
	 * the frequency change to work correctly.
	 */
	if(l2cc_latency[cur_dvfs_op_point_index] > l2cc_latency[dvfs_op_index])
                mxc_pm_set_l2cc_latency(dvfs_op_index);

	mxc_pll_release_pll(MCUPLL);

	/* Change voltage if necessary. */
	if (dvfs_op_index == INDX_532) {
		mxc_pm_chgvoltage(HI_VOLT);
	} else {
		if (mxc_pm_chkvoltage() == HI_VOLT)
			mxc_pm_chgvoltage(LO_VOLT);
	}
	/* Setup dithering of AP Core Normal PLL, if dithering enabled */
	if ((__raw_readl(pll_dp_mfn_togc[MCUPLL]) & TOG_DIS) == 0)
	{
	/*	mxc_pm_dither_pll_setup(MCUPLL, &opinfo[dvfs_op_index]); */

		/* Disable dithering */
		mxc_pm_dither_control(0, MCUPLL);

		/* Disable MCUPLL */
		__raw_writel(__raw_readl(pll_dp_ctlreg[MCUPLL]) & ~MXC_PLL_DP_CTL_UPEN, pll_dp_ctlreg[MCUPLL]);


		/* Setup PLL registers */
		__raw_writel(opinfo[dvfs_op_index].ap_pll_dp_hfs_mfnminus, pll_dp_mfn_minus_reg[MCUPLL]);
		__raw_writel(opinfo[dvfs_op_index].ap_pll_dp_hfs_mfnplus, pll_dp_mfn_plus_reg[MCUPLL]);

		/* Add a delay longer than a TOG_COUNTER period to make sure dither cycle eventually running is finishe
		 * Toggle clock reference is CKIH 26 MHz. 
		 * TOG_COUNTER = 0xA28 = 0d2600 for SCM-A11 platform. 2600 * 1 / 26E^10 = 100 us.
		 * Take in addition a 10 us margin 
		 */
		mxc_gpt_udelay(110);
		
		/* Generate a load_req */
		__raw_writel(__raw_readl(pll_dp_config[MCUPLL]) | DP_LDREQ, pll_dp_config[MCUPLL]);
								
		/* Re-enable MCUPLL */
		__raw_writel(__raw_readl(pll_dp_ctlreg[MCUPLL]) | MXC_PLL_DP_CTL_UPEN, pll_dp_ctlreg[MCUPLL]);

		/* Enable dithering, only if ppm value != 0 */
		if (opinfo[dvfs_op_index].ap_pll_ppm_value != 0)
			mxc_pm_dither_control(1, MCUPLL);
		
	}


	/* Load the correct frequency values into the HFS registers. */
	__raw_writel(opinfo[dvfs_op_index].ap_pll_dp_hfs_op,  pll_dp_hfs_opreg[MCUPLL]);
	__raw_writel(opinfo[dvfs_op_index].ap_pll_dp_hfs_mfd, pll_dp_hfs_mfdreg[MCUPLL]);
	__raw_writel(opinfo[dvfs_op_index].ap_pll_dp_hfs_mfn, pll_dp_hfs_mfnreg[MCUPLL]);

	
	/* Request MCUPLL.  When this returns, the MCUPLL is started and locked. */
	mxc_pll_request_pll(MCUPLL);
	
	/* Switch AP domain back on PLL0 */
	ascsr = (ascsr & ~MXC_CRMAP_ASCSR_APSEL_MASK);
	__raw_writel(ascsr, MXC_CRMAP_ASCSR);

	mxc_pll_release_pll(USBPLL);
	
	if (dvfs_op_index == INDX_133) {
		/*
		 * If we get here, then we are switching to the 133
		 * operating point.
		 *
		 * Freescale recommends that going to a divider ratio
		 * of 2:1:2 (for operating point 133) should be done
		 * in two steps.  The first step is 2:4:8.
		 *
		 * Note that all operating points except 133 use a
		 * CPU divider ratio of 1, so the switch can be done
		 * in one step.
		 *
		 * We delay after we write to ensure that the dividers
		 * are propagated in the correct order.  We are
		 * running now at 1:4:8.  We move to 2:4:8 which
		 * changes only the CPU divider.  Then we make the
		 * final change which changes the AHB/IPG dividers.
		 *
		 * Failure to accomplish the divider ratio switch
		 * in two steps results in system instability.
		 */
		__raw_writel(ARM_AHB_IPG_RATIO_248, MXC_CRMAP_ACDR);
		udelay(1);
	}

	/* Set the ARM:AHB:IPG divider ratio. */
	__raw_writel(opinfo[dvfs_op_index].divider_ratio, MXC_CRMAP_ACDR);

	cur_dvfs_op_point_index = dvfs_op_index;
}
#endif /* CONFIG_MOT_FEAT_PM */

/*!
 * To change AP core frequency and/or voltage suitably
 *
 * @param   dvfs_op    The values are,
 *                     CORE_133 - ARM desired to run @133MHz, LoV (1.2V)
 *                     CORE_266 - ARM desired to run @266MHz, LoV (1.2V)
 *                     CORE_399 - ARM desired to run @399MHz, LoV (1.2V)
 *                     CORE_532 - ARM desired to run @133MHz, HiV (1.6V)
 *                     The table or sequence os as follows where dividers
 *                     ratio includes, LFDF:ARM:AHB:IPG.
 *                          x ==> LFDF or DFS dividers bypassed
 *                               Pass 1           Pass 2
 *                            PLL   Dividers    PLL   Dividers
 *                     133 =  266   x:2:2:4     532   4:1:1:2
 *                     266 =  266   x:1:2:4     532   2:1:2:4
 *                     399 =  399   x:1:3:6     399   x:1:3:6
 *                     532 =  532   x:2:2:4     532   x:1:4:8  (on P1, Core is @266Mhz)
 *
 * @return             Returns -ERR_FREQ_INVALID if none of the above choice
 *                     is selected
 */
static int mxc_pm_chgfreq(dvfs_op_point_t dvfs_op)
{
#ifdef CONFIG_MOT_FEAT_PM
	dvfs_op_point_index_t dvfs_op_index;
#else
	int voltage, dfs_div;
#endif

#ifdef CONFIG_MOT_FEAT_PM
	dvfs_op_index = dvfsop2index(dvfs_op);
	if ((dvfs_op_index < INDX_FIRST) || (dvfs_op_index >= NUM_DVFSOP_INDEXES))
		return -ERR_FREQ_INVALID;

	/*
	 * If we can't change frequencies, then just return.
	 * mxc_pm_chgfreq_enabled() defines when we can change
	 * frequencies and when we can't.
	 */
	if (!mxc_pm_chgfreq_enabled(dvfs_op_index, cur_dvfs_op_point_index))
		return -MXC_PM_DVFS_DISABLED;

	mxc_pm_chgfreq_common(dvfs_op_index);

	return 0;

#else  /* CONFIG_MOT_FEAT_PM */

	/* Check if AP is in HIGH or in LOW voltage */
	voltage = mxc_pm_chkvoltage();

	/* Check if DFS_DIV is ENABLED or DISABLED */
	dfs_div = mxc_pm_chkdfs();

	switch (dvfs_op) {

	case CORE_133:
		/* INTEGER SCALING */
		if (CRM_ISSUE && MXC91231_PASS_1()) {
			/*
			 * Due to HW issue with LFDF dividers, to get 133MHz, PLL is
			 * relocked to 266MHz and ARM:AHB:IPG dividers are set to
			 * 2:2:4
			 */
			mxc_pm_chgfreq(CORE_266);

			/* setting ratio 2:2:4 */
			__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) & ACDR_MASK)
				      | ARM_AHB_IPG_CRMRATIO_266),
				     MXC_CRMAP_ACDR);

			return 0;
		}

		/*
		 * There are two conditions,
		 * 1. You have to be at Hi voltage to set DFS dividers
		 * 2. If not at Hi voltage, then move to Hi voltage and
		 * then set the dividers
		 */
		if (voltage == LO_VOLT) {
			/*
			 * This implies that we are at 399MHz or 266MHz. So, first change
			 * to 532MHz
			 */
			/* Raise Voltage */
			mxc_pm_chgfreq(CORE_532);
		}

		/* Configure scaling type to Integer scaling - to use DFS dividers */
		mxc_pm_config_scale(DFS_ENABLE);

		/* Set LFDF to LFDF_4 so that LFDF:ARM:AHB:IPG is 4:1:1:2 */
		mxc_pm_setlfdf(MXC_CRMAP_ADCR_LFDF_4);

		/* Reduce Voltage */
		mxc_pm_chgvoltage(LO_VOLT);

		break;

	case CORE_266:
		/* INTEGER SCALING */
		if (CRM_ISSUE && MXC91231_PASS_1()) {
			/*
			 * Due to HW issue with LFDF dividers, to get 266MHz, PLL is
			 * relocked to 266MHz and ARM:AHB:IPG dividers are set to
			 * 1:2:4
			 */
			mxc_pm_calc_mf_lfs(266);
			if (voltage == HI_VOLT) {
				mxc_pm_chgvoltage(LO_VOLT);
			}

			/* Change ARM:AHB:IPG divider ratio as 1:2:4 */
			__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) & ACDR_MASK)
				      | ARM_AHB_IPG_RATIO_266), MXC_CRMAP_ACDR);
			return 0;
		}

		/*
		 * There are two conditions,
		 * 1. You have to be at Hi voltage to set DFS dividers
		 * 2. If not at Hi voltage, then move to Hi voltage and
		 * then set the dividers
		 */
		if (voltage == LO_VOLT) {
			/* First change frequency to 532 */
			mxc_pm_chgfreq(CORE_532);
		}

		/* Configure scaling type to Integer scaling - to use DFS dividers */
		mxc_pm_config_scale(DFS_ENABLE);

		/* Set LFDF to LFDF_2 so that LFDF:ARM:AHB:IPG is 2:1:2:4 */
		mxc_pm_setlfdf(MXC_CRMAP_ADCR_LFDF_2);

		/* Reduce Voltage */
		mxc_pm_chgvoltage(LO_VOLT);

		break;

	case CORE_399:
		/* PLL RELOCKING */
		/*
		 * if DFS_DIV_EN = 0 and LOW voltage implies core is running at
		 * 399MHz only on MXC91231 P2. And, 399 or 133 or 266
		 * on MXC91231 P1 due to workaround for HW issues and LFS PLL
		 * registers may be at 266MHz
		 */
		if ((dfs_div == DFS_DISABLE) && (voltage == LO_VOLT)) {

			/* CRM workaround on MXC91231 P1 */
			if (CRM_ISSUE && MXC91231_PASS_1()) {
				/*
				 * This implies either the core frequency is
				 * is already at 133MHz or 266MHz or 399MHz and
				 * the ratio will be at 1:3:6
				 */
				__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) &
					       ACDR_MASK) |
					      ARM_AHB_IPG_RATIO_LO),
					     MXC_CRMAP_ACDR);
				mxc_pm_calc_mf_lfs(399);
			}
			return 0;
		} else {
			if (voltage == LO_VOLT) {
				/*
				 * Raise voltage to HIGH voltage as the core is at
				 * either 133MHz or 266MHz using Integer scaling which.
				 * implies DFS_DIV_EN bit is set. So, to clear DFS_DIV_EN bit
				 * move to Hi voltage as this bit cannot be changed at Lo voltage
				 */
				mxc_pm_chgfreq(CORE_532);
			}

			/* Now core is at Hi voltage */
                        /* Configure scaling type ALT_PLL and clear DFS_DIV_EN bit */
			mxc_pm_config_scale(DFS_DISABLE);

			/* CRM workaround on MXC91231 P1 */
			if (CRM_ISSUE && MXC91231_PASS_1()) {
				/*
				 * Set ARM divider to 2. So that ARM:AHB:IPG
				 * will be 2:4:8
				 */
				__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) &
					       ACDR_MASK) |
					      ARM_AHB_IPG_CRMRATIO_532),
					     MXC_CRMAP_ACDR);
			}


			/* Now, decrease the voltage */
			mxc_pm_chgvoltage(LO_VOLT);

			/*
			 * Wait until PLL relocks as DFS_DIV_EN bit is cleared and
			 * voltage has been decreased
			 */
			while ((__raw_readl(PLL0_DP_CTL) & MXC_PLL_DP_CTL_LRF)
			       != MXC_PLL_DP_CTL_LRF) ;
			/* CRM workaround on MXC91231 P1 */
			if (CRM_ISSUE && MXC91231_PASS_1()) {
				/*
				 * Reprogram PLL as LFS registers may be
				 * programmed to 266MHz
				 */
				mxc_pm_calc_mf_lfs(399);
				/*
				 * Set ARM divider to 2. So that ARM:AHB:IPG
				 * will be 2:3:6
				 */
				__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) &
					       ACDR_MASK) |
					      ARM_AHB_IPG_CRMRATIO_399),
					     MXC_CRMAP_ACDR);
			}

			/* Change ARM:AHB:IPG divider ratio as 1:3:6 */
			__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) & ACDR_MASK)
				      | ARM_AHB_IPG_RATIO_LO), MXC_CRMAP_ACDR);
		}

		break;

	case CORE_532:
		/* if DFS_DIV_EN = 1 and LOW voltage: 133 or 266 -> 532 */
		if ((dfs_div == DFS_ENABLE) && (voltage == LO_VOLT)) {
			/* This implies core is at 133 or 266 on P2 */
			/* CRM workaround on MXC91231 P1 */
			if (CRM_ISSUE && MXC91231_PASS_1()) {
				/*
				 * Set ARM divider to 2. So that ARM:AHB:IPG
				 * will be 2:2:4
				 */
				__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) &
					       ACDR_MASK) |
					      ARM_AHB_IPG_CRMRATIO_266),
					     MXC_CRMAP_ACDR);
			}

			mxc_pm_chgvoltage(HI_VOLT);

		} else if ((dfs_div == DFS_DISABLE) && (voltage == LO_VOLT)) {
			/*
			 * This implies core is at 399 on P2 and 133 or 266 or 399
			 * on P1
			 */

			/* CRM workaround on MXC91231 P1 */
			if (CRM_ISSUE && MXC91231_PASS_1()) {
				/*
				 * Now set the divider ratio to 2:2:4 suitable for
				 * 532MHz
				 */
				__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) &
					       ACDR_MASK) |
					      ARM_AHB_IPG_CRMRATIO_266),
					     MXC_CRMAP_ACDR);
			} else {
				/* Change ARM:AHB:IPG divider ratio as 1:4:8 */
				__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) &
					       ACDR_MASK) |
					      ARM_AHB_IPG_RATIO_HI),
					     MXC_CRMAP_ACDR);
			}
			/* Now, raise the core voltage */
			mxc_pm_chgvoltage(HI_VOLT);

			/* Configure the scaling type - not use DFS dividers */
			mxc_pm_config_scale(DFS_DISABLE);

			/*
			 * Wait until PLL relocks as DFS_DIV_EN bit is cleared and
			 * voltage has been increased
			 */
			while ((__raw_readl(PLL0_DP_CTL) & MXC_PLL_DP_CTL_LRF)
			       != MXC_PLL_DP_CTL_LRF) ;

		} else if (voltage == HI_VOLT) {
			/* do nothing */
		}

		/* CRM workaround */
		if ((CRM_ISSUE && MXC91231_PASS_1()) &&
		    (NO_SDRAMC_RATIO_ISSUE == 1)) {
			/* Change ARM:AHB:IPG divider ratio as 1:4:8 */
			__raw_writel(((__raw_readl(MXC_CRMAP_ACDR) & ACDR_MASK)
				      | ARM_AHB_IPG_RATIO_HI), MXC_CRMAP_ACDR);
		}

		break;

	default:
		return -ERR_FREQ_INVALID;
		break;
	}
	return 0;
#endif /* CONFIG_MOT_FEAT_PM */
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
	dvfs_op_point_t dvfs_op_point = -1;
	unsigned long flags;

	/* Acquiring Lock */
	spin_lock_irqsave(&dvfs_lock, flags);

	switch (armfreq / MEGA_HERTZ) {

	case CORE_133:
		dvfs_op_point = CORE_133;
		break;

	case CORE_266:
		dvfs_op_point = CORE_266;
		break;

	case CORE_399:
		dvfs_op_point = CORE_399;
		break;

	case CORE_532:
		dvfs_op_point = CORE_532;
		break;

	default:
		/* Releasing lock */
		spin_unlock_irqrestore(&dvfs_lock, flags);
		return -ERR_FREQ_INVALID;
		break;
	}

	ret_val = mxc_pm_chgfreq(dvfs_op_point);

	/* Releasing lock */
	spin_unlock_irqrestore(&dvfs_lock, flags);

	return ret_val;
}

/*!
 * Implementing steps required to transition to low-power modes
 *
 * @param   mode    The desired low-power mode. Possible values are,
 *                  WAIT_MODE, STOP_MODE or DSM_MODE
 *
 */
void mxc_pm_lowpower(int mode)
{
	unsigned int crm_ctrl, ctrl_slp;
#ifdef CONFIG_MOT_FEAT_PM
        unsigned int ascsr, pll0_dp_ctlreg, pll1_dp_ctlreg;
	unsigned int arm_ahb_ip_ratio = 0;
	unsigned int apra = 0;
	unsigned int aprb = 0;
	unsigned int acder1 = 0;
	unsigned int acder2 = 0;
	unsigned int crmcom_cscr = 0;
#endif /* CONFIG_MOT_FEAT_PM */
#ifdef CONFIG_MOT_FEAT_PM_STATS
        u32 avic_nipndh;
        u32 avic_nipndl;
        u32 rtc_before = 0; 
        u32 rtc_after = 0;
	mpm_test_point_t exit_test_point = 0;
#endif 

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
	if ((mode != WAIT_MODE) && (mode != STOP_MODE) && (mode != DSM_MODE)) {
		return;
	}

#ifdef CONFIG_MOT_WFN478
	local_irq_save(flags);
#else
        local_irq_disable();
#endif

	switch (mode) {
	case WAIT_MODE:
		/* Writing '01' to CRM_lpmd bits */
		crm_ctrl =
		    (__raw_readl(MXC_DSM_CRM_CONTROL) &
		     ~(MXC_DSM_CRM_CTRL_LPMD1)) | (MXC_DSM_CRM_CTRL_LPMD0);
		__raw_writel(crm_ctrl, MXC_DSM_CRM_CONTROL);
#ifdef CONFIG_MOT_FEAT_PM_STATS
                exit_test_point = MPM_TEST_WAIT_EXIT;
#endif
		break;

	case STOP_MODE:
		/*
		 * High-Level Power Management should ensure all devices are
		 * turned-off before entering STOP mode. This will ensure
		 * that all acknowledgements (MXC_CRMAP_AMCR) are bypassed else STOP mode
		 * will not be entered
		 */

		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & AVIC_TIMER_INTR_DIS),
			     LO_INTR_EN);

		/* Writing '00' to CRM_lpmd bits */
		crm_ctrl =
		    (__raw_readl(MXC_DSM_CRM_CONTROL) &
		     ~(MXC_DSM_CRM_CTRL_LPMD1)) & (~MXC_DSM_CRM_CTRL_LPMD0);
		__raw_writel(crm_ctrl, MXC_DSM_CRM_CONTROL);
#ifdef CONFIG_MOT_FEAT_PM_STATS
                exit_test_point = MPM_TEST_STOP_EXIT;
#endif
		break;

	case DSM_MODE:
		/*
		 * High-Level Power Management should ensure all devices are
		 * turned-off before entering DSM mode. This will ensure
		 * that all acknowledgements (MXC_CRMAP_AMCR) are bypassed else DSM mode
		 * will not be entered
		 */

#ifdef CONFIG_MOT_FEAT_PM
		/*
		 * Store RUN mode settings and apply aggressive clock
		 * gating for STOP mode.
		 */
		apra = __raw_readl(MXC_CRMAP_APRA);
		aprb = __raw_readl(MXC_CRMAP_APRB);
		acder1 = __raw_readl(MXC_CRMAP_ACDER1);
		acder2 = __raw_readl(MXC_CRMAP_ACDER2);

		__raw_writel((apra & ~APRA_ALL_CLK_EN), MXC_CRMAP_APRA);
		__raw_writel((aprb & ~APRB_ALL_CLK_EN), MXC_CRMAP_APRB);
		__raw_writel((acder1 & ~ACDER1_ALL_CLK_EN), MXC_CRMAP_ACDER1);
		__raw_writel((acder2 & ~ACDER2_ALL_CLK_EN), MXC_CRMAP_ACDER2);
#endif /* CONFIG_MOT_FEAT_PM */

		/* Enable WellBias */
		__raw_writel((__raw_readl(MXC_CRMAP_AMCR) | ENABLE_WELL_BIAS),
			     MXC_CRMAP_AMCR);

#ifdef CONFIG_MOT_FEAT_ACTIVE_AND_LOW_POWER_INTERVAL_TIMERS
                /* Enable EPIT Interrupt */
		__raw_writel((__raw_readl(LO_INTR_EN) | (1<<28)), LO_INTR_EN);
		/* Disable GPT interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & 0xDFFFFFFF), LO_INTR_EN);
#else
		/* Disable Timer interrupt source */
		__raw_writel((__raw_readl(LO_INTR_EN) & AVIC_TIMER_INTR_DIS),
			     LO_INTR_EN);
#endif

#ifdef CONFIG_MOT_FEAT_PM
		/*** Current Drain Improvement ***/
		if (!mxc_pm_ddram_detected() && pll0_off_in_ap_dsm()) {
			/*** ONLY IF THE MEMORY IS NOT A DDRAM ***/

			/* Switch AP domain to DSP PLL (PLL1) */

			/*
			 * Set PPD1 bit in CRM_COM CSCR to prevent
			 * PLL1 off if BP only is in DSM.
			 */
			crmcom_cscr = __raw_readl(MXC_CRMCOM_CSCR);
			__raw_writel((crmcom_cscr | CRM_COM_CSCR_PPD1), MXC_CRMCOM_CSCR);

			/*
			 * Re-enable and Restart PLL1.
			 * We don't use the mxc_pll_request_pll
			 * service because we don't know the value of
			 * the counter.  We have to ensure that we
			 * restart and relock the PLL, so we just do
			 * it.
			 */
			if ((__raw_readl(pll_dp_ctlreg[DSPPLL]) & MXC_PLL_DP_CTL_LRF) != MXC_PLL_DP_CTL_LRF)
			{
				pll1_dp_ctlreg = ((__raw_readl(pll_dp_ctlreg[DSPPLL]) |
					MXC_PLL_DP_CTL_RST) | MXC_PLL_DP_CTL_UPEN);
				__raw_writel(pll1_dp_ctlreg, pll_dp_ctlreg[DSPPLL]);
			}

			/* Wait until PLL1 is locked */
			while((__raw_readl(pll_dp_ctlreg[DSPPLL]) & MXC_PLL_DP_CTL_LRF) != MXC_PLL_DP_CTL_LRF);

			/* Switch AP domain to PLL1. */
			ascsr = ((__raw_readl(MXC_CRMAP_ASCSR) & ~ASCSR_APSEL_1) | ASCSR_APSEL_0);
			__raw_writel(ascsr, MXC_CRMAP_ASCSR);
			udelay(1);

			/* Unlock PLL0 */
			pll0_dp_ctlreg = ((__raw_readl(pll_dp_ctlreg[MCUPLL]) &
				   ~MXC_PLL_DP_CTL_RST) & ~MXC_PLL_DP_CTL_UPEN);
			__raw_writel(pll0_dp_ctlreg, pll_dp_ctlreg[MCUPLL]);

			/*
			 * Force ARM:AHB:IP ratio to have
			 * SDRAM clock = PLL1 / 2
			 * Save the existing ratio so we can restore
			 * it after we wake up.
			 *
			 * We are about to switch to PLL1 (DSPPLL)
			 * while we are in DSM so we can turn off
			 * PLL0 (MCUPLL).  We set the PLL dividers to
			 * 1:2:4 because it satisfies the requirements
			 * that SDRAM is <= 133Mhz and the SDRAM:IP
			 * frequency is 2:1 for all PLL0 frequencies
			 * that we use (266/399/532) *and* the PLL1
			 * frequency (156Mhz).
			 */
			arm_ahb_ip_ratio = __raw_readl(MXC_CRMAP_ACDR);

			if (cur_dvfs_op_point_index == INDX_133) {
				/*
				 * If we get here, then we are currently operating at
				 * the 133 operating point.
				 *
				 * Note that all operating points except 133 use a
				 * CPU divider ratio of 1, so the switch can be done
				 * in one step.
				 *
				 * Freescale recommends that moving from a divider
				 * ratio of 2:1:2 (for operating point 133) should be
				 * done in two steps.  The first step is 2:2:4.
				 *
				 * We delay after we write to ensure that the dividers
				 * are propagated in the correct order.  We are
				 * running now at 2:1:2.  We move to 2:2:4 which
				 * changes only the AHB/IPG dividers.  Then we make
				 * the final change which changes the CPU divider.
				 *
				 * Failure to accomplish the divider ratio switch
				 * in two steps results in system instability.
				 */
				__raw_writel(ARM_AHB_IPG_RATIO_224, MXC_CRMAP_ACDR);
				udelay(1);
			}
			__raw_writel(ARM_AHB_IPG_RATIO_124, MXC_CRMAP_ACDR);

			/*
			 * Restore the CRM_COM CSCR.  This deasserts
			 * the PPD1 bit in CRM_COM CSCR to allow PLL1
			 * off if BP only is in DSM.
			 */
			__raw_writel(crmcom_cscr, MXC_CRMCOM_CSCR);
		}

	        /*
	         * Force external interrupts to be delayed 5 CKIL clock cycles
	         * while in DSM DEEPSLEEP1 state.
	         */
	        __raw_writel(MXC_DSM_MGPER_EN_MGFX | MXC_DSM_MGPER_PER(5),
                        MXC_DSM_MGPER);


		/* Reset counter block - Start go_to_sleep transition with COUNT32 = 0 */
		__raw_writel((__raw_readl(MXC_DSM_CONTROL1) | MXC_CONTROL1_RST_CNT32),
								MXC_DSM_CONTROL1);
#endif /* CONFIG_MOT_FEAT_PM */

		/* Enable DSM */
		__raw_writel((__raw_readl(MXC_DSM_CONTROL0) |
			      MXC_DSM_CONTROL0_MSTR_EN), MXC_DSM_CONTROL0);

		/* Enable SLEEP and WAKE UP DISABLE */
		ctrl_slp =
		    ((__raw_readl(MXC_DSM_CONTROL1) | MXC_DSM_CONTROL1_SLEEP) |
		     MXC_DSM_CONTROL1_WAKEUP_DISABLE);
		__raw_writel(ctrl_slp, MXC_DSM_CONTROL1);

#ifndef CONFIG_MOT_FEAT_PM
		/* COUNT32 must not be reset between two DSM state machine transitions */
		if (CONFIG_RESET_COUNT32) {
			/* Set RST_COUNT32_EN bit */
			__raw_writel((__raw_readl(MXC_DSM_CONTROL1) |
				      MXC_DSM_CONTROL1_RST_CNT32_EN),
				     MXC_DSM_CONTROL1);
		}
#endif

		/* Set STBY_COMMIT_EN bit */
		__raw_writel((__raw_readl(MXC_DSM_CONTROL0) |
			      MXC_DSM_CONTROL0_STBY_COMMIT_EN),
			     MXC_DSM_CONTROL0);

#ifndef CONFIG_MOT_FEAT_PM
		/* L1T is owned by BP => AP does not need to restart it */
		/* AP DSM state machine looping between LOCKED and WAIT4SLEEP states */
		/* Set RESTART bit */
		__raw_writel((__raw_readl(MXC_DSM_CONTROL0) |
			      MXC_DSM_CONTROL0_RESTART), MXC_DSM_CONTROL0);
#endif

		/* Setting SLEEP time */
		__raw_writel(SLEEP_TIME, MXC_DSM_SLEEP_TIME);

		/* Setting WARMPER and LOCKPER */
		__raw_writel(CALC_WAKE_TIME, MXC_DSM_WARM_PER);
		__raw_writel(CALC_LOCK_TIME, MXC_DSM_LOCK_PER);

		__raw_writel((__raw_readl(MXC_DSM_CTREN) | MXC_DSM_CTREN_CNT32),
			     MXC_DSM_CTREN);

		/* Writing '00' to CRM_lpmd bits */
		crm_ctrl =
		    (__raw_readl(MXC_DSM_CRM_CONTROL) &
		     (~MXC_DSM_CRM_CTRL_LPMD1)) & (~MXC_DSM_CRM_CTRL_LPMD0);
		__raw_writel(crm_ctrl, MXC_DSM_CRM_CONTROL);

#ifdef CONFIG_MOT_FEAT_PM_STATS
                MPM_REPORT_TEST_POINT(1, MPM_TEST_DSM_ENTER);
                exit_test_point = MPM_TEST_DSM_EXIT;
#endif
		break;
	}
#ifdef CONFIG_MOT_FEAT_PM_STATS 
        /* Read the clock before we go to wait/stop/dsm. */
	rtc_before = rtc_sw_get_internal_ticks();
#endif /* CONFIG_MOT_FEAT_PM_STATS */

	/* Executing CP15 (Wait-for-Interrupt) Instruction */
        cpu_do_idle();

#ifdef CONFIG_MOT_FEAT_PM_STATS
	rtc_after = rtc_sw_get_internal_ticks();

        /*      
	 * Retrieve the pending interrupts.  These are the
	 * interrupt(s) that woke us up.
         */
        avic_nipndh = __raw_readl(AVIC_NIPNDH);
        avic_nipndl = __raw_readl(AVIC_NIPNDL);
#endif           


#ifdef CONFIG_MOT_FEAT_PM
	switch (mode) {
	case WAIT_MODE:
	case STOP_MODE:
		break;

	case DSM_MODE:
                /* Reset DSM controller upon wakeup. */

                /* Clear sleep bit */
                __raw_writel(__raw_readl(MXC_DSM_CONTROL1)
                        & (~MXC_DSM_CONTROL1_SLEEP), MXC_DSM_CONTROL1);

                /* Reset Counter Block and State Machine */
                __raw_writel(__raw_readl(MXC_DSM_CONTROL1)
                        | MXC_DSM_CONTROL1_CB_RST | MXC_DSM_CONTROL1_SM_RST,
                        MXC_DSM_CONTROL1);

                /* Wait for CB and SM reset bits to clear */
                while((__raw_readl(MXC_DSM_CONTROL1)
                        & (MXC_DSM_CONTROL1_CB_RST | MXC_DSM_CONTROL1_SM_RST))
                        != 0x00) {
                    /* spin */
                }

                /* Disable MGPER */
	        __raw_writel(__raw_readl(MXC_DSM_MGPER)
                        & (~MXC_DSM_MGPER_EN_MGFX), MXC_DSM_MGPER);

                /* End reset of DSM controller. */

		/*** Current Drain Improvement ***/
		/*** ONLY IF THE MEMORY IS NOT A DDRAM ***/
		if (!mxc_pm_ddram_detected() && pll0_off_in_ap_dsm()) {
			/* Restore ARM:AHB:IP ratio */
			if (cur_dvfs_op_point_index == INDX_133) {
				/*
				 * If we get here, then we are switching to the 133
				 * operating point.
				 *
				 * Freescale recommends that going to a divider ratio
				 * of 2:1:2 (for operating point 133) should be done
				 * in two steps.  The first step is 2:2:4.
				 *
				 * Note that all operating points except 133 use a
				 * CPU divider ratio of 1, so the switch can be done
				 * in one step.
				 *
				 * We delay after we write to ensure that the dividers
				 * are propagated in the correct order.  We are
				 * running now at 1:2:4.  We move to 2:2:4 which
				 * changes only the CPU divider.  Then we make the
				 * final change which changes the AHB/IPG dividers.
				 *
				 * Failure to accomplish the divider ratio switch
				 * in two steps results in system instability.
				 */
				__raw_writel(ARM_AHB_IPG_RATIO_224, MXC_CRMAP_ACDR);
				udelay(1);
			}
			__raw_writel(arm_ahb_ip_ratio, MXC_CRMAP_ACDR);


			/*
			 * Re-enable and Restart PLL0.
			 * We don't use the mxc_pll_request_pll
			 * service because we don't know the value of
			 * the counter.  We have to ensure that we
			 * restart and relock the PLL, so we just do
			 * it.
			 */
			pll0_dp_ctlreg = ((__raw_readl(pll_dp_ctlreg[MCUPLL]) |
					MXC_PLL_DP_CTL_RST) | MXC_PLL_DP_CTL_UPEN);
			__raw_writel(pll0_dp_ctlreg, pll_dp_ctlreg[MCUPLL]);

			/* Wait until PLL relocks */
			while((__raw_readl(pll_dp_ctlreg[MCUPLL]) &
					MXC_PLL_DP_CTL_LRF) != MXC_PLL_DP_CTL_LRF);

			/* Switch AP domain back on PLL0 */
			ascsr = ((__raw_readl(MXC_CRMAP_ASCSR) & ~ASCSR_APSEL_0) &
								~ASCSR_APSEL_1);
			__raw_writel(ascsr, MXC_CRMAP_ASCSR);

			/*
			 * Deassert PPD1 bit in CRM_COM CSCR to allow PLL1 off
			 * if BP only is in DSM
			 */
			__raw_writel((__raw_readl(MXC_CRMCOM_CSCR) &
						~CRM_COM_CSCR_PPD1), MXC_CRMCOM_CSCR);
		}

		/* Restore RUN mode clock gating */
		__raw_writel(apra, MXC_CRMAP_APRA);
		__raw_writel(aprb, MXC_CRMAP_APRB);
		__raw_writel(acder1, MXC_CRMAP_ACDER1);
		__raw_writel(acder2, MXC_CRMAP_ACDER2);
		break;
	}
	/*** Current Drain Improvement - END ***/
#endif

	/* Enable timer interrupt */
	__raw_writel((__raw_readl(LO_INTR_EN) | AVIC_TIMER_INTR_EN),
		     LO_INTR_EN);

#ifdef CONFIG_MOT_WFN478
	local_irq_restore(flags);
#else
        local_irq_enable();
#endif

#if defined(CONFIG_MOT_DSM_INDICATOR) || defined(CONFIG_MOT_TURBO_INDICATOR)  
        printk("MPM: restore from sleep,exit_test_point=%d, rtc_before = 0x%x, rtc_after= 0x%x, pending_interrupt=0x%x,0x%x\n",
                     exit_test_point, rtc_before, rtc_after, avic_nipndl, avic_nipndh);

#endif


#ifdef CONFIG_MOT_FEAT_PM_STATS
        MPM_REPORT_TEST_POINT(5, exit_test_point, rtc_before, rtc_after,
                              avic_nipndl, avic_nipndh);
#endif
}

/*!
 * Enables acknowledgement from module when entering stop or DSM mode.
 *
 * @param   ack     The desired module acknowledgement to enable.
 *
 */
void mxc_pm_lp_ack_enable(int ack)
{
	/* Clear selected bypass and enable the acknowledgement */
	__raw_writel(__raw_readl(MXC_CRMAP_AMCR) & ~(ack << 16),
		     MXC_CRMAP_AMCR);
}

/*!
 * Disables acknowledgement from module when entering stop or DSM mode.
 *
 * @param   ack     The desired module acknowledgement to disable.
 *
 */
void mxc_pm_lp_ack_disable(int ack)
{
	/* Set selected bypass and disable the acknowledgement */
	__raw_writel(__raw_readl(MXC_CRMAP_AMCR) | (ack << 16), MXC_CRMAP_AMCR);
}

/*!
 * Implementing Level 1 CRM Gate Control. Level 2 gate control
 * is provided at module level using LPMD registers
 *
 * @param   group   The desired clock gate control register bits.
 *                  Possible values are 0 through 6
 * @param   opt     The desired option requesting clock to run during stop
 *                  and wait modes or just during the stop mode. Possible
 *                  values are GATE_STOP_WAIT and GATE_STOP.
 *
 */
void mxc_pm_clockgate(int group, int opt)
{
	if (opt == GATE_STOP_WAIT) {
		__raw_writel(((__raw_readl(MXC_CRMAP_ACGCR) &
			       (~gate_run[group])) | (gate_stop_wait[group])),
			     MXC_CRMAP_ACGCR);
	} else if (opt == GATE_STOP) {
		__raw_writel(((__raw_readl(MXC_CRMAP_ACGCR) &
			       (~gate_run[group])) | (gate_stop[group])),
			     MXC_CRMAP_ACGCR);
	}
}

/*
 * This API is not supported on MXC91231
 */
int mxc_pm_intscale(long armfreq, long ahbfreq, long ipfreq)
{
	return -MXC_PM_API_NOT_SUPPORTED;
}

/*
 * This API is not supported on MXC91231
 */
int mxc_pm_pllscale(long armfreq, long ahbfreq, long ipfreq)
{
	return -MXC_PM_API_NOT_SUPPORTED;
}

#ifndef CONFIG_MOT_FEAT_PM
/*!
 * Programming MCUPLL HFS register after lvs and HFSM signals are
 * synchronised
 */
static void mxc_pm_config_hfs(void)
{
	/*
	 * When core is running at 399MHz and HFSM bit is
	 * synchronised with lvs signal, then HFS register can
	 * configured to 532MHz
	 */
	__raw_writel(DP_HFS_OP, PLL0_DP_HFSOP);
	__raw_writel(DP_HFS_MFN, PLL0_DP_HFSMFN);
	__raw_writel(DP_HFS_MFD, PLL0_DP_HFSMFD);
}

/*!
 * This function is used to synchronise HFSM bit and the LVS signal.
 * They are out of sync by default. So, moving to hi voltage
 * using Int DVFS fixes this
 *
 */
static void mxc_pm_hfsm_sync(void)
{
	/*
	 * HFSM bit gets synchronised with LVS after the initial low to high
	 * Integer DVFS
	 */
	unsigned long adcr_val;

	/* Set up to do integer DVFS low to High */
	adcr_val =
	    ((__raw_readl(MXC_CRMAP_ADCR) & (~MXC_CRMAP_ADCR_ALT_PLL)) &
	     (~MXC_CRMAP_ADCR_DIV_BYP)) | (MXC_CRMAP_ADCR_DFS_DIVEN);
	__raw_writel(adcr_val, MXC_CRMAP_ADCR);

	/*
	 * Clear the DVFS_BYP bypass bit to allow voltage change.
	 */
	__raw_writel((__raw_readl(MXC_DSM_CRM_CONTROL) &
		      (~MXC_DSM_CRM_CTRL_DVFS_BYP)), MXC_DSM_CRM_CONTROL);

	/* Change voltage status */
	mxc_pm_chgvoltage(HI_VOLT);

	/* Reset to PLL Relock DVFS at Hi voltage */
	adcr_val =
	    (((__raw_readl(MXC_CRMAP_ADCR) | (MXC_CRMAP_ADCR_DIV_BYP)) &
	      (~MXC_CRMAP_ADCR_DFS_DIVEN)) | (MXC_CRMAP_ADCR_CLK_ON)) &
	    (~MXC_CRMAP_ADCR_ALT_PLL);
	__raw_writel(adcr_val, MXC_CRMAP_ADCR);

	/* Resetting LFDF to divide by 1 */
	mxc_pm_setlfdf(MXC_CRMAP_ADCR_LFDF_0);

	/*
	 * Now HFSM and VSTAT are synchronised.
	 * So, moving back to low voltage using PLL scaling
	 */
	mxc_pm_chgvoltage(LO_VOLT);

	/*
	 * Configure HFS settings for 532MHz
	 */
	mxc_pm_config_hfs();

}
#endif /* !CONFIG_MOT_FEAT_PM */

#ifdef CONFIG_MOT_FEAT_PM

/*
 *  PLLs dithering
 */

/*!
 * Dithering enable/disable of the AP and USB PLLs
 * SCM-A11: only ap_core_normal_pll_ppm and ap_usb_pll_ppm are concerned.
 * Must be called only if a non 0 ppm value has been setup via mxc_pm_dither_setup
 *
 * @param   enable    0: disable, 1: enable
 * @param   pll_number of the type ap_pll_number_t: PLL to enable/disable
 *
 * @return 0 OK, -1 wrong PLL number
 */
int mxc_pm_dither_control(int enable, enum plls pll_number)
{
        int togc_val;

        if ((pll_number != MCUPLL) && (pll_number != USBPLL))
                return -1;

        if (enable) {
                togc_val = __raw_readl(pll_dp_mfn_togc[pll_number]) & ~TOG_DIS;
                __raw_writel(togc_val, pll_dp_mfn_togc[pll_number]);
        } else {
                togc_val = __raw_readl(pll_dp_mfn_togc[pll_number]) | TOG_DIS;
                __raw_writel(togc_val, pll_dp_mfn_togc[pll_number]);
        }

        return 0;
}


/*!
 * Dithering: AP PLL dithering setup sequence
 * SCM-A11: only ap_core_normal_pll_ppm and ap_usb_pll_ppm are concerned.
 *
 * @param pll_number of type enum ap_pll_number_t
 * @param pll_mfn_values of type enum ap_pll_mfn_values_t* for storage of register values
 *
 * @return 0 OK, -1 wrong pll_number
 */
static int mxc_pm_dither_pll_setup(enum plls pll_number,
                                   ap_pll_mfn_values_t *ap_pll_mfn_values)
{
        int reg_val;

        if ((pll_number != MCUPLL) && (pll_number != USBPLL))
                return -1;

        /* Disable dithering */
        mxc_pm_dither_control(0, pll_number);

        /* Wait for end of the dithering cycle. */
        while (__raw_readl(pll_dp_destat[pll_number]) & TOG_SEL);
	
        /* Setup PLL registers */
	__raw_writel(ap_pll_mfn_values->ap_pll_dp_hfs_mfnminus,
	       pll_dp_mfn_minus_reg[pll_number]);
	__raw_writel(ap_pll_mfn_values->ap_pll_dp_hfs_mfnplus,
	       pll_dp_mfn_plus_reg[pll_number]);

        /* Write toggle rate: TOG_CNT of TOGC register */
        reg_val = __raw_readl(pll_dp_mfn_togc[pll_number]) & ~TOGC_CNT_MASK;
        reg_val |= TOGC_CNT_VALUE;
        __raw_writel(reg_val, pll_dp_mfn_togc[pll_number]);
                
        /* Generate a load_req */
        reg_val = __raw_readl(pll_dp_config[pll_number]) | DP_LDREQ;
        __raw_writel(reg_val, pll_dp_config[pll_number]);

        /* Enable dithering, only if ppm value != 0 */
        if (ap_pll_mfn_values->ap_pll_ppm_value != 0)
                mxc_pm_dither_control(1, pll_number);

        return 0;
}


/*!
 * Dithering: PLL dithering parameters calculation
 *            This function guarantees that if the calculations
 *            fail in any way (fcn return value is -1) then the
 *            pll_mfn_values structure is not modified.
 * SCM-A11: only ap_core_normal_pll_ppm and ap_usb_pll_ppm are calculated.
 *
 * @param pll_ppm_value
 * @param pll_number of type enum plls
 * @param pll_mfn_values of type ap_pll_mfn_values_t* for storage of register values
 *
 * @return 0 OK, -1 calculated parameters out of range
 */
static int mxc_pm_dither_calc_mfnplusminus(u16 pll_ppm_value,
                                           enum plls pll_number,
                                           ap_pll_mfn_values_t *pll_mfn_values) 
{
        u32 mfi_hfs, mfd_hfs;
        s32 mfn_hfs;
        s32 prod_hfs;
        s64 var1_hfs;
        s64 var2_hfs;
        int ap_pll_hfs_mfnplus, ap_pll_hfs_mfnminus;

        /* Compute MFNPLUS and MFNMINUS from registers value for HFS mode */
        /* Use the MFI, MFN, and MFD values from the provided structure. */
        mfi_hfs = (pll_mfn_values->ap_pll_dp_hfs_op >> 4) & 0xF;
        mfd_hfs = pll_mfn_values->ap_pll_dp_hfs_mfd;
        mfn_hfs = pll_mfn_values->ap_pll_dp_hfs_mfn;

        /* Store the MFI*MFD product value */
        prod_hfs = (s32)(mfi_hfs) * (s32)(mfd_hfs);
   
        /* Compute MFNPLUS and MFNMINUS from MFN value for Normal Mode */
        var1_hfs = (s64)(prod_hfs + mfn_hfs) * (s64)(1000000 + (pll_ppm_value));
        var2_hfs = var1_hfs;

        /* MFN_PLUS  <=== (MFI*MFD + MFN)*(1+EpsilonPlus/1000000) - MFI*MFD */
        do_div(var1_hfs, 1000000);
        ap_pll_hfs_mfnplus = (s32)var1_hfs - prod_hfs;

        /*
	 * MFN_MINUS <=== (MFI*MFD + MFN)*(1-EpsilonMinus/1000000) - MFI*MFD
	 * We could do the calculation above, but the calculation below is
	 * faster and equivalent.
	 */
	ap_pll_hfs_mfnminus = (mfn_hfs * 2) - ap_pll_hfs_mfnplus;


        /* Verify that MFN values are not out of bounds. */
        if ( (DPLL_MFN_TO_ABSOLUTE(ap_pll_hfs_mfnplus)  > mfd_hfs) ||
             (DPLL_MFN_TO_ABSOLUTE(ap_pll_hfs_mfnminus) > mfd_hfs) )
        {
                return -1;
        }

        /* Only store the values if they are properly validated. */
        pll_mfn_values->ap_pll_ppm_value = pll_ppm_value;
        pll_mfn_values->ap_pll_dp_hfs_mfnplus = ap_pll_hfs_mfnplus;
        pll_mfn_values->ap_pll_dp_hfs_mfnminus = ap_pll_hfs_mfnminus;

        return 0;
}


/*!
 * Dithering setup: pass the PLLs ppm values. This function calculates
 *   and stores the MFN+- values locally for the AP PLL,
 *   sets up the USB PLL dithering, and enables the dithering.
 * SCM-A11: only ap_core_normal_pll_ppm and ap_usb_pll_ppm PLLs are used.
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
        unsigned long flags;
        int rc = 0;
	dvfs_op_point_t dvfs_op;
	dvfs_op_point_index_t dvfs_op_index;

        if (ap_core_normal_pll_ppm > MAX_PLL_PPM_VALUE)
                return -1;

        if (ap_usb_pll_ppm > MAX_PLL_PPM_VALUE)
                return -1;

	dvfs_op = mxc_get_clocks(CPU_CLK) / MEGA_HERTZ;

        /* Acquiring Lock */
        spin_lock_irqsave(&dvfs_lock, flags);

	for (dvfs_op_index=INDX_FIRST; (rc==0) && (dvfs_op_index<NUM_DVFSOP_INDEXES); dvfs_op_index++)
	{
		/* AP normal PLL dithering setup: calc, storage, setup, enable */
		/* Calc AP PLL MFN+- values for HFS mode */
		rc = mxc_pm_dither_calc_mfnplusminus(ap_core_normal_pll_ppm,
						     MCUPLL,
						     &opinfo[dvfs_op_index]);

		/* Setup AP PLL registers: MFN+-, toggle rate, Enable bit if ppm!=0 */
		if ((rc == 0) && (dvfsop2index(dvfs_op) == dvfs_op_index)) {
			rc = mxc_pm_dither_pll_setup(MCUPLL,
						     &opinfo[dvfs_op_index]);
		}
        }

        /* AP USB PLL dithering setup: calc, storage, setup, enable */
        if (rc == 0) {
                /* Calc USB PLL MFN+- values for HFS mode */
                rc = mxc_pm_dither_calc_mfnplusminus(ap_usb_pll_ppm,
                                                     USBPLL,
                                                     &ap_usb_pll_mfn_values);

                /* Setup USB PLL registers: MFN+-, toggle rate, Enable bit if ppm!=0 */
                if (rc == 0) {
                        rc = mxc_pm_dither_pll_setup(USBPLL,
                                                     &ap_usb_pll_mfn_values);
                }
        }

        /* Releasing lock */
        spin_unlock_irqrestore(&dvfs_lock, flags);

        return rc;
}


/*!
 * Dithering report: return dithering information for all AP PLLs
 * The dithering information includes the PPM and MFN+- values for
 * HFS and LFS.
 * SCM-A11: only AP core normal and AP USB PLLs are used.
 *          The information for AP core turbo is just zeroed.
 *
 * @param   pllvals    ptr to dithering info for all AP PLLs
 */
void mxc_pm_dither_report(ap_all_plls_mfn_values_t *pllvals)
{
        unsigned long flags;
	dvfs_op_point_index_t dvfs_op_index;

        /* Acquiring Lock */
        spin_lock_irqsave(&dvfs_lock, flags);

        /*
         * Copy the values from the local structs to the one provided by 
         * the caller.
         * SCM-A11 doesn't use the turbo mode values, so just return
         * zeroes.
         */
	for (dvfs_op_index=INDX_FIRST; dvfs_op_index<NUM_DVFSOP_INDEXES; dvfs_op_index++)
	{
		pllvals->ap_core_normal_pll[dvfs_op_index] = opinfo[dvfs_op_index];
	}
        memset(&pllvals->ap_core_turbo_pll, 0, sizeof(pllvals->ap_core_turbo_pll));
        pllvals->ap_usb_pll = ap_usb_pll_mfn_values;

        /* Releasing lock */
        spin_unlock_irqrestore(&dvfs_lock, flags);
}
#endif /* CONFIG_MOT_FEAT_PM */


/*!
 * This function is used to load the module.
 *
 * @return   Returns an Integer on success
 */
static int __init mxc_pm_init_module(void)
{
#ifdef CONFIG_MOT_FEAT_PM
        unsigned long adcr;
	int bootloader_used_lfs = 0;
#endif

#ifndef CONFIG_MOT_FEAT_PM
	/*
	 * If HFSM bit in the PLL Control register is 1
	 * but VSTAT bit is high indicating a Low voltage
	 * then HFSM and lvs signal should be synchronised
	 */
	if (((__raw_readl(PLL0_DP_CTL) & MXC_PLL_DP_CTL_HFSM) != 0) &&
	    (mxc_pm_chkvoltage() == LO_VOLT)) {
		mxc_pm_hfsm_sync();
	}
#endif

#ifdef CONFIG_MOT_FEAT_PM
        if (mxc_pm_ddram_detected()) {
                printk("mxc_pm_init: memory is DDRAM: DVFS is not allowed\n");
        } else {
		/*** ONLY IF THE MEMORY IS NOT A DDRAM ***/
                printk("mxc_pm_init: memory is not DDRAM: DVFS is allowed\n");

                /* Change the PLL mode to go to HFS mode using the PLL relock with CLK_ON method */
                /* Configure PLL for 399 MHz in HFS mode at low voltage */

		if ((__raw_readl(pll_dp_ctlreg[MCUPLL]) & DP_HFSM) == 0) {
			/*
			 * This is the old bootloader.  We know that
			 * it's booting at 399.  Set up for that.
			 * PLL0 is at 399 MHz in LFS mode and the core
			 * is at low voltage.
			 */
			bootloader_used_lfs = 1;

			/* Load the 399Mhz frequency values into the HFS registers. */
			__raw_writel(opinfo[INDX_399].ap_pll_dp_hfs_op,  pll_dp_hfs_opreg[MCUPLL]);
			__raw_writel(opinfo[INDX_399].ap_pll_dp_hfs_mfd, pll_dp_hfs_mfdreg[MCUPLL]);
			__raw_writel(opinfo[INDX_399].ap_pll_dp_hfs_mfn, pll_dp_hfs_mfnreg[MCUPLL]);

			/* PLL relock using Pat_ref_clk during relock to switch to HFS mode */
			adcr = (((__raw_readl(MXC_CRMAP_ADCR) | MXC_CRMAP_ADCR_DIV_BYP) | MXC_CRMAP_ADCR_CLK_ON) &
					~MXC_CRMAP_ADCR_ALT_PLL) & ~MXC_CRMAP_ADCR_DFS_DIVEN;
			__raw_writel(adcr, MXC_CRMAP_ADCR);

			/* Now, raise the core voltage */
			mxc_pm_chgvoltage(HI_VOLT);

			/* Wait until PLL relocks */
			while((__raw_readl(pll_dp_ctlreg[MCUPLL]) & MXC_PLL_DP_CTL_LRF) != MXC_PLL_DP_CTL_LRF);

			/*
			 * PLL0 is at 399 MHz in HFS mode, core is at high voltage
			 * Voltage should now be lowered
			 */
			/* Select ALT_PLL method */
			adcr = ((__raw_readl(MXC_CRMAP_ADCR) | MXC_CRMAP_ADCR_ALT_PLL) & (~MXC_CRMAP_ADCR_DFS_DIVEN)) &
				(~MXC_CRMAP_ADCR_CLK_ON);
			__raw_writel(adcr, MXC_CRMAP_ADCR);
			/* Now, lower the core voltage */
			mxc_pm_chgvoltage(LO_VOLT);

			cur_dvfs_op_point_index = INDX_399;

			/*
			 * PLL0 is at 399 MHz in HFS mode, core is at low voltage
			 */
		}
        }

	/*
	 * If the bootloader is an older one that uses LFS mode, then
	 * we have already set cur_dvfs_op_point_index.  Otherwise, we
	 * need to do it here.
	 */
	if (bootloader_used_lfs == 0) {
		switch (mot_boot_frequency()) {
		case BOOT_FREQUENCY_532:
			cur_dvfs_op_point_index = INDX_532;
			break;
		case BOOT_FREQUENCY_399:
		default:
			cur_dvfs_op_point_index = INDX_399;
			break;
		}
	}

	/*
	 * We never use integer scaling since we always use PLL
	 * scaling with the ALT_PLL method so we disable
	 * integer scaling once here and leave it off.
	 */
	mxc_pm_config_scale(DFS_DISABLE);

	/*
	 * Normally during PLL relock DVFS, the system transitions to run
	 * the ARM core off of ckin (26 MHz), making AHB 13Mhz and IP
	 * 6.5 Mhz.  This IP clock is too slow.
	 * To preserve 13 MHz on pat_ref, we run the ARM core off of
	 * ckihX2.
	 */
	if (MXC91231_PASS_2_OR_GREATER()) {
		/*
		 * Set ASCSR:AP_ISEL to get ckih_x2_ck
		 * Set ACSR:ADS to select the clock doubler path.  Note that the
		 * ADS bit was introduced in Pass 2 silicon.
		 */
#ifdef CONFIG_MACH_MXC27530EVB
		__raw_writel(__raw_readl(MXC_CRMAP_ASCSR) | (long)
			     MXC_CRMAP_ASCSR_APISEL, MXC_CRMAP_ASCSR);
#endif
		__raw_writel(__raw_readl(MXC_CRMAP_ACSR) | (long)
			     MXC_CRMAP_ACSR_ADS_PASS2, MXC_CRMAP_ACSR);
	}
#else  /* CONFIG_MOT_FEAT_PM */

#ifdef CONFIG_MACH_MXC27530EVB
	/*
	 * Normally during PLL relock DVFS, the system transitions to run
	 * the ARM core off of ckin (26 MHz), making AHB 13Mhz and IP
	 * 6.5 Mhz.  This IP clock is too slow.
	 * To preserve 13 MHz on pat_ref, we run the ARM core off of
	 * ckihX2.
	 */
	if (MXC91231_PASS_2_OR_GREATER()) {
		/*
		 * Set ASCSR:AP_ISEL to get ckih_x2_ck
		 * Set ACSR:ADS to select the clock doubler path.  Note that the
		 * ADS bit was introduced in Pass 2 silicon.
		 */
		__raw_writel(__raw_readl(MXC_CRMAP_ASCSR) | (long)
			     MXC_CRMAP_ASCSR_APISEL, MXC_CRMAP_ASCSR);
		__raw_writel(__raw_readl(MXC_CRMAP_ACSR) | (long)
			     MXC_CRMAP_ACSR_ADS_PASS2, MXC_CRMAP_ACSR);
	}
#endif

#endif

	/* Bypass all acknowledgements except MAX */
	__raw_writel((__raw_readl(MXC_CRMAP_AMCR) | AMCR_ACK_BYP),
		     MXC_CRMAP_AMCR);

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

EXPORT_SYMBOL(mxc_pm_dvfs);
EXPORT_SYMBOL(mxc_pm_clockgate);
EXPORT_SYMBOL(mxc_pm_lowpower);
EXPORT_SYMBOL(mxc_pm_lp_ack_enable);
EXPORT_SYMBOL(mxc_pm_lp_ack_disable);
EXPORT_SYMBOL(mxc_pm_pllscale);
EXPORT_SYMBOL(mxc_pm_intscale);

#ifdef CONFIG_MOT_FEAT_PM
/* De-sense */
EXPORT_SYMBOL(mxc_pm_dither_control);
EXPORT_SYMBOL(mxc_pm_dither_setup);
EXPORT_SYMBOL(mxc_pm_dither_report);
#endif /* CONFIG_MOT_FEAT_PM */

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC91231 Low-level PM Driver");
MODULE_LICENSE("GPL");
