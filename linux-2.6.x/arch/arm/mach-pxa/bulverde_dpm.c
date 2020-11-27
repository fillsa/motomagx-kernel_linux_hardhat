/*
 * arch/arm/mach-pxa/bulverde_dpm.c  DPM support for Intel PXA27x (Bulverde)
 *
 * Author: <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Copyright (C) 2002, 2005 MontaVista Software <source@mvista.com>.
 *
 * Based on code by Matthew Locke, Dmitry Chigirev, and Bishop Brock.
 */

#include <linux/config.h>

#include <linux/dpm.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/cpufreq.h>

#include <asm/uaccess.h>

#include <asm/hardware.h>
#include <asm/arch/pxa-regs.h>
#include <asm/arch/dpm.h>

#include "bulverde_freq.h"

#undef DEBUG


static int saved_loops_per_jiffy = 0;
static int saved_cpu_freq = 0;

#define BULVERDE_DEFAULT_VOLTAGE 1400

#define DPM_FSCALER_NOP             0
#define DPM_FSCALER_CPUFREQ        (1 << 0)
#define DPM_FSCALER_SLEEP          (1 << 1)
#define DPM_FSCALER_STANDBY        (1 << 2)
#define DPM_FSCALER_DEEPSLEEP      (1 << 3)
#define DPM_FSCALER_WAKEUP         (1 << 4)
#define DPM_FSCALER_VOLTAGE        (1 << 5)
#define DPM_FSCALER_XPLLON          (1 << 6)
#define DPM_FSCALER_HALFTURBO_ON   (1 << 7)
#define DPM_FSCALER_HALFTURBO_OFF  (1 << 8)
#define DPM_FSCALER_TURBO_ON       (1 << 9)
#define DPM_FSCALER_TURBO_OFF      (1 << 10)

#define DPM_FSCALER_TURBO (DPM_FSCALER_TURBO_ON | DPM_FSCALER_TURBO_OFF)

#define DPM_FSCALER_ANY_SLEEPMODE     (DPM_FSCALER_SLEEP | \
                                   DPM_FSCALER_STANDBY | \
				   DPM_FSCALER_DEEPSLEEP)

#define CCCR_CPDIS_BIT_ON          (1 << 31)
#define CCCR_PPDIS_BIT_ON          (1 << 30)
#define CCCR_CPDIS_BIT_OFF         (0 << 31)
#define CCCR_PPDIS_BIT_OFF         (0 << 30)
#define CCCR_PLL_EARLY_EN_BIT_ON   (1 << 26)
#define CCSR_CPLL_LOCKED           (1 << 29)
#define CCSR_PPLL_LOCKED           (1 << 28)

/* CLKCFG
   | 31------------------------------------------- | 3 | 2  | 1 | 0 |
   | --------------------------------------------- | B | HT | F | T |
*/
#define CLKCFG_B_BIT               (1 << 3)
#define CLKCFG_HT_BIT              (1 << 2)
#define CLKCFG_F_BIT               (1 << 1)
#define CLKCFG_T_BIT               1

/* Initialize the machine-dependent operating point from a list of parameters,
   which has already been installed in the pp field of the operating point.
   Some of the parameters may be specified with a value of -1 to indicate a
   default value. */

#define	PLL_L_MAX	31
#define	PLL_N_MAX	8

/* The MIN for L is 2 in the Yellow Book tables, but L=1 really means
   13M mode, so L min includes 1 */
#define	PLL_L_MIN	1
#define	PLL_N_MIN	2

/* memory timing (MSC0,DTC,DRI) constants (see Blob and Intel BBU sources) */
#define XLLI_MSC0_13 0x11101110
#define XLLI_MSC0_19 0x11101110
#define XLLI_MSC0_26 0x11201120  /* 26 MHz setting */
#define XLLI_MSC0_32 0x11201120
#define XLLI_MSC0_39 0x11301130   /* 39 MHz setting */
#define XLLI_MSC0_45 0x11301130
#define XLLI_MSC0_52 0x11401140  /* @ 52 MHz setting */
#define XLLI_MSC0_58 0x11401140
#define XLLI_MSC0_65 0x11501150  /* @ 65 MHz setting */
#define XLLI_MSC0_68 0x11501150
#define XLLI_MSC0_71 0x11501150  /* @ 71.5 MHz setting */
#define XLLI_MSC0_74 0x11601160
#define XLLI_MSC0_78 0x12601260  /* @ 78 MHz setting */
#define XLLI_MSC0_81 0x12601260
#define XLLI_MSC0_84 0x12601260  /*  @ 84.5 MHz setting */
#define XLLI_MSC0_87 0x12701270
#define XLLI_MSC0_91 0x12701270  /* 91 MHz setting */
#define XLLI_MSC0_94 0x12701270  /* 94.2 MHz setting */
#define XLLI_MSC0_97 0x12701270  /* 97.5 MHz setting */
#define XLLI_MSC0_100 0x12801280 /* 100.7 MHz setting */
#define XLLI_MSC0_104 0x12801280 /* 104 MHz setting */
#define XLLI_MSC0_110 0x12901290
#define XLLI_MSC0_117 0x13901390 /* 117 MHz setting */
#define XLLI_MSC0_124 0x13A013A0
#define XLLI_MSC0_130 0x13A013A0 /* 130 MHz setting */
#define XLLI_MSC0_136 0x13B013B0
#define XLLI_MSC0_143 0x13B013B0
#define XLLI_MSC0_149 0x13C013C0
#define XLLI_MSC0_156 0x14C014C0
#define XLLI_MSC0_162 0x14C014C0
#define XLLI_MSC0_169 0x14C014C0
#define XLLI_MSC0_175 0x14C014C0
#define XLLI_MSC0_182 0x14C014C0
#define XLLI_MSC0_188 0x14C014C0
#define XLLI_MSC0_195 0x15C015C0
#define XLLI_MSC0_201 0x15D015D0
#define XLLI_MSC0_208 0x15D015D0

/* DTC settings depend on 16/32 bit SDRAM we have (32 is chosen) */
#define XLLI_DTC_13 0x00000000
#define XLLI_DTC_19 0x00000000
#define XLLI_DTC_26 0x00000000
#define XLLI_DTC_32 0x00000000
#define XLLI_DTC_39 0x00000000   
#define XLLI_DTC_45 0x00000000
#define XLLI_DTC_52 0x00000000
#define XLLI_DTC_58 0x01000100
#define XLLI_DTC_65 0x01000100
#define XLLI_DTC_68 0x01000100
#define XLLI_DTC_71 0x01000100
#define XLLI_DTC_74 0x01000100
#define XLLI_DTC_78 0x01000100
#define XLLI_DTC_81 0x01000100
#define XLLI_DTC_84 0x01000100
#define XLLI_DTC_87 0x01000100
#define XLLI_DTC_91 0x02000200
#define XLLI_DTC_94 0x02000200
#define XLLI_DTC_97 0x02000200
#define XLLI_DTC_100 0x02000200
#define XLLI_DTC_104 0x02000200
/* 110-208 MHz setting - SDCLK Halved*/ 
#define XLLI_DTC_110 0x01000100
#define XLLI_DTC_117 0x01000100
#define XLLI_DTC_124 0x01000100
#define XLLI_DTC_130 0x01000100   
#define XLLI_DTC_136 0x01000100
#define XLLI_DTC_143 0x01000100
#define XLLI_DTC_149 0x01000100   
#define XLLI_DTC_156 0x01000100
#define XLLI_DTC_162 0x01000100
#define XLLI_DTC_169 0x01000100
#define XLLI_DTC_175 0x01000100
/*  182-208 MHz setting - SDCLK Halved - Close to edge, so bump up */
#define XLLI_DTC_182 0x02000200
#define XLLI_DTC_188 0x02000200
#define XLLI_DTC_195 0x02000200
#define XLLI_DTC_201 0x02000200
#define XLLI_DTC_208 0x02000200

/*       Optimal values for DRI (refreash interval) settings for
 * various MemClk settings (MDREFR)
 */
#define XLLI_DRI_13 0x002
#define XLLI_DRI_19 0x003
#define XLLI_DRI_26 0x005
#define XLLI_DRI_32 0x006
#define XLLI_DRI_39 0x008
#define XLLI_DRI_45 0x00A
#define XLLI_DRI_52 0x00B
#define XLLI_DRI_58 0x00D
#define XLLI_DRI_65 0x00E
#define XLLI_DRI_68 0x00F
#define XLLI_DRI_71 0x010
#define XLLI_DRI_74 0x011
#define XLLI_DRI_78 0x012
#define XLLI_DRI_81 0x012
#define XLLI_DRI_84 0x013
#define XLLI_DRI_87 0x014
#define XLLI_DRI_91 0x015
#define XLLI_DRI_94 0x016
#define XLLI_DRI_97 0x016
#define XLLI_DRI_100 0x017
#define XLLI_DRI_104 0x018
#define XLLI_DRI_110 0x01A
#define XLLI_DRI_117 0x01B
#define XLLI_DRI_124 0x01D
#define XLLI_DRI_130 0x01E
#define XLLI_DRI_136 0x020
#define XLLI_DRI_143 0x021
#define XLLI_DRI_149 0x023
#define XLLI_DRI_156 0x025
#define XLLI_DRI_162 0x026
#define XLLI_DRI_169 0x028
#define XLLI_DRI_175 0x029
#define XLLI_DRI_182 0x02B
#define XLLI_DRI_188 0x02D
#define XLLI_DRI_195 0x02E
#define XLLI_DRI_201 0x030
#define XLLI_DRI_208 0x031



/* timings for memory controller set up (masked values) */
struct mem_timings{
	unsigned int msc0; /* for MSC0 */
	unsigned int dtc; /* for MDCNFG */
	unsigned int dri; /* for MDREFR */
};


static unsigned int dpm_fscaler_flags = 0;

void dpm_bulverde_fully_define_opt(struct dpm_md_opt *cur,
				   struct dpm_md_opt *new);
static int dpm_bulverde_fscale(struct dpm_opt *new);
static void mainstone_fscaler(struct dpm_regs *regs);

extern void bulverde_set_voltage(unsigned int mv);
extern void bulverde_prep_set_voltage(unsigned int mv);
extern int bulverde_vcs_init(void);
extern void bulverde_voltage_cleanup(void);

/*Linux 2.6 no longer enforces a certain sequence of SUSPEND and RESUME "
 *levels" in driver suspend/resume callbacks for all buses. The platform bus currently (2.6.5) uses the following sequence:
 *
 *suspend: 1. SUSPEND_DISABLE; 2. SUSPEND_SAVE_STATE; 3. SUSPEND_POWER_DOWN
 *
 *resume: 1. RESUME_POWER_ON; 2. RESUME_RESTORE_STATE; 3. RESUME_ENABLE
 *
 *Depending on the particulars of the platform, the suspend mode requested,
 *and the device/driver, all 3 levels may or may not be useful, and some may
 * be ignored if not useful. Note that SUSPEND_NOTIFY is still defined but not
 * used by the platform bus driver; if a 2-step "quiesce the device and then
 * power down" is needed then SUSPEND_NOTIFY can be replaced with 
 *SUSPEND_DISABLE and the actual power down can occur at SUSPEND_POWER_DOWN
 * at before. 
 */

static inline int p5d(char *buf, unsigned mhz)
{
	return sprintf(buf, "%5d", mhz);	/* Round */
}

static unsigned long
calculate_memclk(unsigned long cccr, unsigned long clkcfg)
{
	unsigned long M, memclk;
	u32 L;
	
	L = cccr & 0x1f;
	if (cccr & (1 << 25)) {
		if  (clkcfg & CLKCFG_B_BIT)
			memclk = (L*13);
		else
			memclk = (L*13)/2;
	}
	else {
		if (L <= 10) M = 1;
		else if (L <= 20) M = 2;
		else M = 4;

		memclk = (L*13)/M;
	}

	return memclk;
}

static unsigned long
calculate_new_memclk(struct dpm_regs *regs)
{
	return calculate_memclk(regs->cccr, regs->clkcfg);
}

static unsigned long
calculate_cur_memclk(void)
{
	unsigned long cccr = CCCR;
	return calculate_memclk(cccr, bulverde_read_clkcfg());
}

/* Returns optimal timings for memory controller
 * a - [A]
 * b - [B]
 * l - value of L
 */
static struct mem_timings get_optimal_mem_timings(int a, int b, int l){
	struct mem_timings ret = {
		.msc0 = 0,
		.dtc = 0,
		.dri = 0,
	};

	if(a!=0 && b==0){
		switch(l){  
		case 2:
			ret.msc0 = XLLI_MSC0_13;
			ret.dtc = XLLI_DTC_13;
			ret.dri = XLLI_DRI_13;
			break;
		case 3:
			ret.msc0 = XLLI_MSC0_19;
			ret.dtc = XLLI_DTC_19;
			ret.dri = XLLI_DRI_19;
			break;
		case 4:
			ret.msc0 = XLLI_MSC0_26; 
			ret.dtc = XLLI_DTC_26;
			ret.dri = XLLI_DRI_26;
			break;
		case 5:
			ret.msc0 = XLLI_MSC0_32;
			ret.dtc = XLLI_DTC_32;
			ret.dri = XLLI_DRI_32;
			break;
		case 6:
			ret.msc0 = XLLI_MSC0_39; 
			ret.dtc = XLLI_DTC_39;
			ret.dri = XLLI_DRI_39;
			break;
		case 7:
			ret.msc0 = XLLI_MSC0_45;
			ret.dtc = XLLI_DTC_45;
			ret.dri = XLLI_DRI_45;
			break;
		case 8:
			ret.msc0 = XLLI_MSC0_52;
			ret.dtc = XLLI_DTC_52;
			ret.dri = XLLI_DRI_52;
			break;
		case 9:
			ret.msc0 = XLLI_MSC0_58;
			ret.dtc = XLLI_DTC_58;
			ret.dri = XLLI_DRI_58;
			break;
		case 10:
			ret.msc0 = XLLI_MSC0_65;
			ret.dtc = XLLI_DTC_65;
			ret.dri = XLLI_DRI_65;
			break;
			/*
			 *       L11 - L20 ARE THE SAME for A0Bx
			 */
		case 11: 
			ret.msc0 = XLLI_MSC0_71;
			ret.dtc = XLLI_DTC_71;
			ret.dri = XLLI_DRI_71;
			break;
		case 12:
			ret.msc0 = XLLI_MSC0_78;
			ret.dtc = XLLI_DTC_78;
			ret.dri = XLLI_DRI_78;
			break;
		case 13:
			ret.msc0 = XLLI_MSC0_84;
			ret.dtc = XLLI_DTC_84;
			ret.dri = XLLI_DRI_84;
			break;
		case 14:
			ret.msc0 = XLLI_MSC0_91;
			ret.dtc = XLLI_DTC_91;
			ret.dri = XLLI_DRI_91;
			break;
		case 15:
			ret.msc0 = XLLI_MSC0_97;
			ret.dtc = XLLI_DTC_97;
			ret.dri = XLLI_DRI_97;
			break;
		case 16:
			ret.msc0 = XLLI_MSC0_104;
			ret.dtc = XLLI_DTC_104;
			ret.dri = XLLI_DRI_104;
			break;
		case 17:
			ret.msc0 = XLLI_MSC0_110;
			ret.dtc = XLLI_DTC_110;
			ret.dri = XLLI_DRI_110;
			break;
		case 18:
			ret.msc0 = XLLI_MSC0_117;
			ret.dtc = XLLI_DTC_117;
			ret.dri = XLLI_DRI_117;
			break;
		case 19:
			ret.msc0 = XLLI_MSC0_124;
			ret.dtc = XLLI_DTC_124;
			ret.dri = XLLI_DRI_124;
			break;
		case 20:
			ret.msc0 = XLLI_MSC0_130;
			ret.dtc = XLLI_DTC_130;
			ret.dri = XLLI_DRI_130;
			break;
		case 21:
			ret.msc0 = XLLI_MSC0_136;
			ret.dtc = XLLI_DTC_136;
			ret.dri = XLLI_DRI_136;
			break;
		case 22:
			ret.msc0 = XLLI_MSC0_143;
			ret.dtc = XLLI_DTC_143;
			ret.dri = XLLI_DRI_143;
			break;
		case 23:
			ret.msc0 = XLLI_MSC0_149; 
			ret.dtc = XLLI_DTC_149;
			ret.dri = XLLI_DRI_149;
			break;
		case 24:
			ret.msc0 = XLLI_MSC0_156;
			ret.dtc = XLLI_DTC_156;
			ret.dri = XLLI_DRI_156;
			break;
		case 25:
			ret.msc0 = XLLI_MSC0_162;
			ret.dtc = XLLI_DTC_162;
			ret.dri = XLLI_DRI_162;
			break;
		case 26:
			ret.msc0 = XLLI_MSC0_169;
			ret.dtc = XLLI_DTC_169;
			ret.dri = XLLI_DRI_169;
			break;
		case 27:
			ret.msc0 = XLLI_MSC0_175;
			ret.dtc = XLLI_DTC_175;
			ret.dri = XLLI_DRI_175;
			break;
		case 28:
			ret.msc0 = XLLI_MSC0_182;
			ret.dtc = XLLI_DTC_182;
			ret.dri = XLLI_DRI_182;
			break;
		case 29:
			ret.msc0 = XLLI_MSC0_188;
			ret.dtc = XLLI_DTC_188;
			ret.dri = XLLI_DRI_188;
			break;
		case 30:
			ret.msc0 = XLLI_MSC0_195;
			ret.dtc = XLLI_DTC_195;
			ret.dri = XLLI_DRI_195;
			break;
		case 31:
			ret.msc0 = XLLI_MSC0_201;
			ret.dtc = XLLI_DTC_201;
			ret.dri = XLLI_DRI_201;
		}
	  
	}else if(a!=0 && b!=0){
	  switch(l){
		case 2:
			ret.msc0 = XLLI_MSC0_26;
			ret.dtc = XLLI_DTC_26;
			ret.dri = XLLI_DRI_26;
			break;
		case 3:
			ret.msc0 = XLLI_MSC0_39;
			ret.dtc = XLLI_DTC_39;
			ret.dri = XLLI_DRI_39;
			break;
		case 4:
			ret.msc0 = XLLI_MSC0_52;
			ret.dtc = XLLI_DTC_52;
			ret.dri = XLLI_DRI_52;
			break;
		case 5:
			ret.msc0 = XLLI_MSC0_65;
			ret.dtc = XLLI_DTC_65;
			ret.dri = XLLI_DRI_65;
			break;
		case 6:
			ret.msc0 = XLLI_MSC0_78;
			ret.dtc = XLLI_DTC_78;
			ret.dri = XLLI_DRI_78;
			break;
		case 7:
			ret.msc0 = XLLI_MSC0_91;
			ret.dtc = XLLI_DTC_91;
			ret.dri = XLLI_DRI_91;
			break;
		case 8:
			ret.msc0 = XLLI_MSC0_104;
			ret.dtc = XLLI_DTC_104;
			ret.dri = XLLI_DRI_104;
			break;
		case 9:
			ret.msc0 = XLLI_MSC0_117;
			ret.dtc = XLLI_DTC_117;
			ret.dri = XLLI_DRI_117;
			break;
		case 10:
			ret.msc0 = XLLI_MSC0_130;
			ret.dtc = XLLI_DTC_130;
			ret.dri = XLLI_DRI_130;
			break;
		case 11:
			ret.msc0 = XLLI_MSC0_143;
			ret.dtc = XLLI_DTC_143;
			ret.dri = XLLI_DRI_143;
			break;
		case 12:
			ret.msc0 = XLLI_MSC0_156;
			ret.dtc = XLLI_DTC_156;
			ret.dri = XLLI_DRI_156;
			break;
		case 13:
			ret.msc0 = XLLI_MSC0_169;
			ret.dtc = XLLI_DTC_169;
			ret.dri = XLLI_DRI_169;
			break;
		case 14:
			ret.msc0 = XLLI_MSC0_182;
			ret.dtc = XLLI_DTC_182;
			ret.dri = XLLI_DRI_182;
			break;
		case 15:
			ret.msc0 = XLLI_MSC0_195;
			ret.dtc = XLLI_DTC_195;
			ret.dri = XLLI_DRI_195;
			break;
		case 16:
			ret.msc0 = XLLI_MSC0_208;
			ret.dtc = XLLI_DTC_208;
			ret.dri = XLLI_DRI_208;
		}	 
	}else{
	  /* A0Bx */
		switch(l){
		case 2:
			ret.msc0 = XLLI_MSC0_26;
			ret.dtc = XLLI_DTC_26;
			ret.dri = XLLI_DRI_26;
			break;
		case 3:
			ret.msc0 = XLLI_MSC0_39;
			ret.dtc = XLLI_DTC_39;
			ret.dri = XLLI_DRI_39;
			break;
		case 4:
			ret.msc0 = XLLI_MSC0_52;
			ret.dtc = XLLI_DTC_52;
			ret.dri = XLLI_DRI_52;
			break;
		case 5:
			ret.msc0 = XLLI_MSC0_65;
			ret.dtc = XLLI_DTC_65;
			ret.dri = XLLI_DRI_65;
			break;
		case 6:
			ret.msc0 = XLLI_MSC0_78;
			ret.dtc = XLLI_DTC_78;
			ret.dri = XLLI_DRI_78;
			break;
		case 7:
			ret.msc0 = XLLI_MSC0_91;
			ret.dtc = XLLI_DTC_91;
			ret.dri = XLLI_DRI_91;
			break;
		case 8:
			ret.msc0 = XLLI_MSC0_104;
			ret.dtc = XLLI_DTC_104;
			ret.dri = XLLI_DRI_104;
			break;
		case 9:
			ret.msc0 = XLLI_MSC0_117;
			ret.dtc = XLLI_DTC_117;
			ret.dri = XLLI_DRI_117;
			break;
		case 10:
			ret.msc0 = XLLI_MSC0_130;
			ret.dtc = XLLI_DTC_130;
			ret.dri = XLLI_DRI_130;
			break;
		case 11:
			ret.msc0 = XLLI_MSC0_71;
			ret.dtc = XLLI_DTC_71;
			ret.dri = XLLI_DRI_71;
			break;
		case 12:
			ret.msc0 = XLLI_MSC0_78;
			ret.dtc = XLLI_DTC_78;
			ret.dri = XLLI_DRI_78;
			break;
		case 13:
			ret.msc0 = XLLI_MSC0_84;
			ret.dtc = XLLI_DTC_84;
			ret.dri = XLLI_DRI_84;
			break;
		case 14:
			ret.msc0 = XLLI_MSC0_91;
			ret.dtc = XLLI_DTC_91;
			ret.dri = XLLI_DRI_91;
			break;
		case 15:
			ret.msc0 = XLLI_MSC0_97;
			ret.dtc = XLLI_DTC_97;
			ret.dri = XLLI_DRI_97;
			break;
		case 16:
			ret.msc0 = XLLI_MSC0_104;
			ret.dtc = XLLI_DTC_104;
			ret.dri = XLLI_DRI_104;
			break;
		case 17:
			ret.msc0 = XLLI_MSC0_110;
			ret.dtc = XLLI_DTC_110;
			ret.dri = XLLI_DRI_110;
			break;
		case 18:
			ret.msc0 = XLLI_MSC0_117;
			ret.dtc = XLLI_DTC_117;
			ret.dri = XLLI_DRI_117;
			break;
		case 19:
			ret.msc0 = XLLI_MSC0_124;
			ret.dtc = XLLI_DTC_124;
			ret.dri = XLLI_DRI_124;
			break;
		case 20:
			ret.msc0 = XLLI_MSC0_130;
			ret.dtc = XLLI_DTC_130;
			ret.dri = XLLI_DRI_130;
			break;
		case 21:
			ret.msc0 = XLLI_MSC0_68;
			ret.dtc = XLLI_DTC_68;
			ret.dri = XLLI_DRI_68;
			break;
		case 22:
			ret.msc0 = XLLI_MSC0_71;
			ret.dtc = XLLI_DTC_71;
			ret.dri = XLLI_DRI_71;
			break;
		case 23:
			ret.msc0 = XLLI_MSC0_74;
			ret.dtc = XLLI_DTC_74;
			ret.dri = XLLI_DRI_74;
			break;
		case 24:
			ret.msc0 = XLLI_MSC0_78;
			ret.dtc = XLLI_DTC_78;
			ret.dri = XLLI_DRI_78;
			break;
		case 25:
			ret.msc0 = XLLI_MSC0_81;
			ret.dtc = XLLI_DTC_81;
			ret.dri = XLLI_DRI_81;
			break;
		case 26:
			ret.msc0 = XLLI_MSC0_84;
			ret.dtc = XLLI_DTC_84;
			ret.dri = XLLI_DRI_84;
			break;
		case 27:
			ret.msc0 = XLLI_MSC0_87;
			ret.dtc = XLLI_DTC_87;
			ret.dri = XLLI_DRI_87;
			break;
		case 28:
			ret.msc0 = XLLI_MSC0_91;
			ret.dtc = XLLI_DTC_91;
			ret.dri = XLLI_DRI_91;
			break;
		case 29:
			ret.msc0 = XLLI_MSC0_94;
			ret.dtc = XLLI_DTC_94;
			ret.dri = XLLI_DRI_94;
			break;
		case 30:
			ret.msc0 = XLLI_MSC0_97;
			ret.dtc = XLLI_DTC_97;
			ret.dri = XLLI_DRI_97;
			break;
		case 31:
			ret.msc0 = XLLI_MSC0_100;
			ret.dtc = XLLI_DTC_100;
			ret.dri = XLLI_DRI_100;
		}	 
	}

	return ret;
}

static void assign_optimal_mem_timings(
	unsigned int* msc0_reg,
	unsigned int* mdrefr_reg,
	unsigned int* mdcnfg_reg,
	int a, int b, int l
	)
{
	unsigned int msc0_reg_tmp = (*msc0_reg); 
	unsigned int mdrefr_reg_tmp = (*mdrefr_reg);
	unsigned int mdcnfg_reg_tmp = (*mdcnfg_reg);
	struct mem_timings timings = get_optimal_mem_timings(a,b,l);

	/* clear bits which are set by get_optimal_mem_timings*/
	msc0_reg_tmp &= ~(MSC0_RDF & MSC0_RDN & MSC0_RRR);
	mdrefr_reg_tmp &= ~(MDREFR_RFU & MDREFR_DRI);
	mdcnfg_reg_tmp &= ~(MDCNFG_DTC0 & MDCNFG_DTC2);

	/* prepare appropriate timings */
	msc0_reg_tmp |= timings.msc0;
	mdrefr_reg_tmp |= timings.dri;
	mdcnfg_reg_tmp |= timings.dtc;

	/* set timings (all bits one time) */
	(*msc0_reg) = msc0_reg_tmp;
	(*mdrefr_reg) = mdrefr_reg_tmp;
	(*mdcnfg_reg) = mdcnfg_reg_tmp;
}

static void set_mdrefr_value(u32 new_mdrefr){
	unsigned long s, old_mdrefr, errata62;
	old_mdrefr = MDREFR;
	/* E62 (28007106.pdf): Memory controller may hang while clearing
	 * MDREFR[K1DB2] or MDREFR[K2DB2]
	 */
	errata62 =
		(((old_mdrefr & MDREFR_K1DB2) != 0) && ((new_mdrefr & MDREFR_K1DB2) == 0)) || 
		(((old_mdrefr & MDREFR_K2DB2) != 0) && ((new_mdrefr & MDREFR_K2DB2) == 0));
	  	 
	if(errata62){
		unsigned long oscr_0 = OSCR;
		unsigned long oscr_1 = oscr_0;
		/* Step 1 - disable interrupts */
		local_irq_save(s);
		/* Step 2 - leave KxDB2, but set MDREFR[DRI] (bits 0-11) to
		 *  0xFFF
		 */
		MDREFR = MDREFR | MDREFR_DRI;
		/* Step 3 - read MDREFR one time */
		MDREFR;
		/* Step 4 - wait 1.6167us
		 * (3.25MHz clock increments OSCR0 7 times)
		 */
		while(oscr_1-oscr_0 < 7){
			cpu_relax();
			oscr_1 = OSCR;
		}

	}

	/* Step 5 - clear K1DB1 and/or K2DB2, and set MDREFR[DRI] to
	 * proper value at the same time
	 */

	/*Set MDREFR as if no errata workaround is needed*/
	MDREFR = new_mdrefr;

	if(errata62){
		/* Step 6 - read MDREFR one time*/
		MDREFR;
		/* Step 7 - enable interrupts*/
		local_irq_restore(s);
	}
}

static void mainstone_scale_cpufreq(struct dpm_regs *regs)
{
	unsigned long new_memclk, cur_memclk;
	u32 new_mdrefr, cur_mdrefr, read_mdrefr;
	u32 new_msc0, new_mdcnfg;
	int set_mdrefr = 0, scaling_up = 0;
	int l, a, b;

	l = regs->cccr & CCCR_L_MASK;	/* Get L */
	b = (regs->clkcfg >> 3) & 0x1;
	a = (regs->cccr >> 25) & 0x1;	/* cccr[A]: bit 25 */

	cur_memclk = calculate_cur_memclk();
	new_memclk = calculate_new_memclk(regs);
	
	new_mdrefr = cur_mdrefr = MDREFR;
	new_msc0 = MSC0;
	new_mdcnfg = MDCNFG;
	
	if (new_memclk != cur_memclk) {
		/* SDCLK0,SDCLK1,SDCLK2 = MEMCLK - by default (<=52MHz) */
		new_mdrefr &= ~( MDREFR_K0DB2 | MDREFR_K0DB4 |
			MDREFR_K1DB2 | MDREFR_K2DB2 );

		if ((new_memclk > 52) && (new_memclk <= 104)) {
			/* SDCLK0 = MEMCLK/2, SDCLK1,SDCLK2 = MEMCLK */
			new_mdrefr |= MDREFR_K0DB2;
		}
		else if (new_memclk > 104){
			/* SDCLK0 = MEMCLK/4,  SDCLK1 and SDCLK2 = MEMCLK/2 */
			new_mdrefr |= (MDREFR_K0DB4 | MDREFR_K1DB2 | MDREFR_K2DB2);
		}

		/* clock increasing or decreasing? */
		if (new_memclk > cur_memclk) scaling_up = 1;
	}
	
	/* set MDREFR if necessary */
	if (new_mdrefr != cur_mdrefr){
		set_mdrefr = 1;
		/* also adjust timings  as long as we change MDREFR value */
		assign_optimal_mem_timings(
					   &new_msc0,
					   &new_mdrefr,
					   &new_mdcnfg,
					   a,b,l
					   );
	}

	/* if memclk is scaling up, set MDREFR before freq change
	 * (2800002.pdf:6.5.1.4)
	 */
	if (set_mdrefr && scaling_up) {
		MSC0 = new_msc0;
		set_mdrefr_value(new_mdrefr);
		MDCNFG = new_mdcnfg;
		read_mdrefr = MDREFR;
	}

	CCCR = regs->cccr;
	bulverde_set_freq(regs->clkcfg);

	/* if memclk is scaling down, set MDREFR after freq change
	 * (2800002.pdf:6.5.1.4)
	 */
	if (set_mdrefr && !scaling_up) {
		MSC0 = new_msc0;
		set_mdrefr_value(new_mdrefr);
		MDCNFG = new_mdcnfg;
		read_mdrefr = MDREFR;
	}
}

static void mainstone_scale_voltage(struct dpm_regs *regs)
{
	bulverde_set_voltage(regs->voltage);
}

static void mainstone_scale_voltage_coupled(struct dpm_regs *regs)
{
	bulverde_prep_set_voltage(regs->voltage);
}

static void calculate_lcd_freq(struct dpm_md_opt *opt)
{
	int k = 1;		/* lcd divisor */

	/* L is verified to be between PLL_L_MAX and PLL_L_MIN in
	   dpm_bulverde_init_opt() 
	 */
	if (opt->l == -1) {
		opt->lcd = -1;
		return;
	}

	if (opt->l > 16) {
		/* When L=17-31, K=4 */
		k = 4;
	} else if (opt->l > 7) {
		/* When L=8-16, K=2 */
		k = 2;
	}

	/* Else, when L=2-7, K=1 */

	opt->lcd = 13000 * opt->l / k;
}

static void calculate_reg_values(struct dpm_md_opt *opt)
{
	int f = 0;		/* frequency change bit */
	int turbo = 0;		/* turbo mode bit; depends on N value */

	opt->regs.voltage = opt->v;

/*
  CCCR
    | 31| 30|29-28| 27| 26| 25|24-11| 10| 9 | 8 | 7 |6-5  | 4 | 3 | 2 | 1 | 0 |
    | C | P |     | L | P |   |     |               |     |                   |
    | P | P |     | C | L |   |     |               |     |                   |
    | D | D |resrv| D | L | A |resrv|  2 * N        |resrv|    L              |
    | I | I |     | 2 | . |   |     |               |     |                   |
    | S | S |     | 6 | . |   |     |               |     |                   |
 
    A: Alternate setting for MEMC clock
       0 = MEM clock frequency as specified in YB's table 3-7
       1 = MEM clock frq = System Bus Frequency
 

  CLKCFG
    | 31------------------------------------------- | 3 | 2  | 1 | 0 |
    | --------------------------------------------- | B | HT | F | T |
 
    B = Fast-Bus Mode  0: System Bus is half of run-mode
                       1: System Bus is equal to run-mode
                       NOTE: only allowed when L <= 16
       
    HT = Half-Turbo    0: core frequency = run or turbo, depending on T bit
                       1: core frequency = turbo frequency / 2
                       NOTE: only allowed when 2N = 6 or 2N = 8
 
    F = Frequency change
                       0: No frequency change is performed
                       1: Do frequency-change 
 
    T = Turbo Mode     0: CPU operates at run Frequency
                       1: CPU operates at Turbo Frequency (when n2 > 2)
 
*/
	/* Set the CLKCFG with B, T, and HT */
	if (opt->b != -1 && opt->n != -1) {
		f = 1;

		/*When N2=2, Turbo Mode equals Run Mode, so it
		   does not really matter if this is >2 or >=2 
		 */
		if (opt->n > 2) {
			turbo = 0x1;
		}
		opt->regs.clkcfg = (opt->b << 3) + (f << 1) + turbo;
	} else {
		f = 0x1;
		opt->regs.clkcfg = (f << 1);
	}

	/*
	   What about when n2=0 ... it is not defined by the yellow
	   book 
	 */
	if (opt->n != -1) {
		/* N2 is 4 bits, L is 5 bits */
		opt->regs.cccr = ((opt->n & 0xF) << 7) + (opt->l & 0x1F);
	}

	if (opt->cccra > 0) {
		/* Turn on the CCCR[A] bit */
		opt->regs.cccr |= (1 << 25);
	}

	/* 13M Mode */
	if (opt->l == 1) {
	}

	if ( (opt->l > 1) && (opt->cpll_enabled == 0) ) {
	  	 printk(KERN_WARNING
		  "DPM: internal error if l>1 CPLL must be On\n");
	}
	if( (opt->cpll_enabled == 1) && (opt->ppll_enabled == 0) ){
	  	 printk(KERN_WARNING
		 "DPM: internal error CPLL=On PPLL=Off is NOT supported in hardware\n");
 	}
	if(opt->cpll_enabled == 0) {
	 	 opt->regs.cccr |= (CCCR_CPDIS_BIT_ON);
	}
	if(opt->ppll_enabled == 0) {
	 	 opt->regs.cccr |= (CCCR_PPDIS_BIT_ON);
	}

}

int dpm_bulverde_init_opt(struct dpm_opt *opt)
{
	int v = -1;
	int l = -1;
	int n2 = -1;
	int b = -1;
	int half_turbo = -1;
	int cccra = -1;
	int cpll_enabled = -1;
	int ppll_enabled = -1;
	int sleep_mode = -1;
	struct dpm_md_opt *md_opt = NULL;

	v = opt->pp[DPM_MD_V];
	l = opt->pp[DPM_MD_PLL_L];
	n2 = opt->pp[DPM_MD_PLL_N];	/* 2*N */
	b = opt->pp[DPM_MD_PLL_B];	/* Fast bus mode bit. */
	half_turbo = opt->pp[DPM_MD_HALF_TURBO];
	cccra = opt->pp[DPM_MD_CCCRA];	/* Alternate setting
					   for the MEM clock */
	cpll_enabled    = opt->pp[DPM_MD_CPLL_ON];
	ppll_enabled    = opt->pp[DPM_MD_PPLL_ON];
	sleep_mode = opt->pp[DPM_MD_SLEEP_MODE];

	md_opt = &opt->md_opt;

	/* Up-front error checking.  If we fail any of these, then the
	   whole operating point is suspect and therefore invalid. 
	 */

	/* Check if PLLs combination is valid due to not hang later*/
	if( (l > PLL_L_MIN) && ( cpll_enabled == 0 ) ){
		 printk(KERN_WARNING
			 "DPM: when l>0 (NOT 13M mode) CPLL must be On \n");
	  	 return -EINVAL;
	}
	if( (cpll_enabled>0) && (ppll_enabled==0) ){
		 printk(KERN_WARNING
			 "DPM: illegal combination CPLL=On PPLL=Off\n");
	  	 return -EINVAL;
	}

	/* Check if voltage is correct */
	if(v < -1){
		printk(KERN_WARNING
	       "DPM: incorrect voltage %d\n",
		       v);
		return -EINVAL;
	}

	if ((l != -1) && (n2 != -1)) {
		if (((l && n2) == 0) && (l || n2) != 0) {
			/* If one of L or N2 is 0, but they are not both 0 */
			printk(KERN_WARNING
			       "DPM: L/N (%d/%d) must both be 0 or both be non-zero\n",
			       l, n2);
			return -EINVAL;
		}

		/* Standard range checking */
		if (((l > 0) && (n2 > 0)) &&	/* Don't complain about 0, it means sleep */
		    ((l > PLL_L_MAX) ||
		     (n2 > PLL_N_MAX) || (l < PLL_L_MIN) || (n2 < PLL_N_MIN))) {
			/* Range checking */
			printk(KERN_WARNING
			       "DPM: L/N (%d/%d) out of range, L=1-31, N=2-8 \n",
			       l, n2);
			return -EINVAL;
		}

		/* If this is for 13M mode, do some more checking */
		if (l == PLL_L_MIN) {
			/*
			  NOTE: the Yellow Book does not require any
			  particular setting for N, but we think it really
			  should be 2
			*/
			if (n2 != 2) {
				printk(KERN_WARNING
				       "DPM: When L=1 (13M Mode), N must be 2 (%d)\n",
				       n2);
				return -EINVAL;
			}

			if ((cpll_enabled != 0) && (cpll_enabled != -1)) {
				printk(KERN_WARNING
					"DPM: When L=1 (13M Mode), CPLL must be OFF (%d)\n",
					cpll_enabled);
				return -EINVAL;
			}

			/* Page 3-32, section 3.5.7.5.2 of the Yellow Book
			   says, "Notes: Other bits in the CLKCFG can not be
			   changed while entering or exiting the 13M
			   mode. While in 13M mode, it is illegal to write to
			   CLKCFG's B, HT, or T bits"
			*/
			if ((b > 0) || (half_turbo > 0)) {
				printk(KERN_WARNING
				       "DPM: When L=1 (13M Mode), B (%d) and "
				       "Half-Turbo (%d) must be off\n", b, half_turbo);
				return -EINVAL;
			}
		}
	}

	if (half_turbo > 1) {
		printk(KERN_WARNING "DPM: Half-Turbo must be 0 or 1 (%d)\n",
		       half_turbo);
		return -EINVAL;
	}

	if (b > 1) {
		printk(KERN_WARNING
		       "DPM: Fast-Bus Mode (B) must be 0 or 1 (%d)\n", b);
		return -EINVAL;
	}

	/* 2800002.pdf 3.5.7.1 It is illegal to set B if CCCR[CPDIS] is set. */
	if( cpll_enabled==0 && b == 1){
		printk(KERN_WARNING
		       "DPM: fast bus (b=%d) must both be 0 if CPLL is Off\n",
		       b);
		return -EINVAL;
	}

	if (cccra > 1) {
		printk(KERN_WARNING
		       "DPM: CCCR[A] (alternate MEMC clock) must be 0 or 1 (%d)\n",
		       cccra);
		return -EINVAL;
	}

	/* This (when CCCR[A] is on and FastBus is on, L must be <=16)
	   is explicitly stated in text at the bottom of one of the
	   CPU frequency tables--the one where CCCR[A] is on */
	if ((b == 1) && (cccra == 1) && (l > 16)) {
		printk(KERN_WARNING
		       "DPM: when B=1 and CCCR[A]=1, L must be <= 16 (L is %d)\n",
		       l);
		return -EINVAL;
	}

	/* This one is not explicitly stated the Yellow Book as a bad
	   thing (as the previous restriction is), but according to
	   the CPU frequency tables, fast bus mode *cannot* be
	   supported, even when CCCR[A] is not 1.
	 */
	if ((b == 1) && (l > 16)) {
		printk(KERN_WARNING
		       "DPM: when B=1, L must be <= 16 (L is %d)\n", l);
		return -EINVAL;
	}

	if (n2 != -1) {
		if ((half_turbo == 1) && (n2 != 6) && (n2 != 8)) {
			printk(KERN_WARNING
			       "DPM: Half Turbo only allowed when N2 is 6 or 8\n"
			       "(N2 is %d)\n", n2);
			return -EINVAL;
		}
	}

	/* Check Sleep Mode versus modes from pm.h
	   NOTE: CPUMODE_SENSE is not implemented 
	 */
	if ((l == 0) && (n2 == 0) && (sleep_mode != -1) &&
	    (sleep_mode != CPUMODE_STANDBY) &&
	    (sleep_mode != CPUMODE_SLEEP) &&
	    (sleep_mode != CPUMODE_DEEPSLEEP)) {
		printk(KERN_WARNING
		       "DPM: Sleep Mode value %d is not allowed"
		       " (only %d, %d, or %d) l=%d n2=%d\n",
		       sleep_mode,
		       CPUMODE_STANDBY, CPUMODE_SLEEP, CPUMODE_DEEPSLEEP,
		       l, n2);
		return -EINVAL;
	}

	/* save the values for this operating point */
	md_opt->v = v;
	md_opt->l = l;
	md_opt->n = n2;
	md_opt->b = b;
	md_opt->cccra = cccra;
	md_opt->half_turbo = half_turbo;
	md_opt->cpll_enabled = cpll_enabled;
	md_opt->ppll_enabled = ppll_enabled;
	md_opt->sleep_mode = sleep_mode;
	calculate_lcd_freq(md_opt);

	if ((md_opt->l == -1) || (md_opt->n == -1)) {
		md_opt->cpu = -1;
	} else {
		/* shift 1 to divide by 2 because opt->n is 2*N */
		md_opt->cpu = (13000 * md_opt->l * md_opt->n) >> 1;
		if (md_opt->half_turbo == 1) {
			/* divide by 2 */
			md_opt->cpu = md_opt->cpu >> 1;
		}
	}

	return 0;
}

/* This routine computes the "forward" frequency scaler flags
 * for moving the system
 * from the current operating point to the new operating point.  The resulting
 * fscaler is applied to the registers of the new operating point. 
 */
void compute_fscaler_flags(struct dpm_md_opt *cur, struct dpm_md_opt *new)
{
	int current_n, ccsr;
	ccsr = CCSR;
	current_n = (ccsr & CCCR_N_MASK) >> 7;
	dpm_fscaler_flags = DPM_FSCALER_NOP;
	/* If new CPU is 0, that means sleep, we do NOT switch PLLs
	   if going to sleep.
	 */
	if (!new->cpu) {
		if (new->sleep_mode == CPUMODE_DEEPSLEEP) {
			dpm_fscaler_flags |= DPM_FSCALER_DEEPSLEEP;
		} else if (new->sleep_mode == CPUMODE_STANDBY) {
			dpm_fscaler_flags |= DPM_FSCALER_STANDBY;
		} else {
			dpm_fscaler_flags |= DPM_FSCALER_SLEEP;
		}
	} else {

		/* If exiting 13M mode, set the flag so we can do the extra
		work to get out before the frequency change 
		*/
		if( ((cur->cpll_enabled == 0) && (new->cpll_enabled ==1)) ||
			((cur->ppll_enabled == 0) && (new->ppll_enabled ==1)) ){
			dpm_fscaler_flags |= DPM_FSCALER_XPLLON;
		}

	}


	/* if CPU is *something*, it means we are not going to sleep */
	if ((new->cpu) &&
	    /* And something has indeed changed */
	    ((new->regs.cccr != cur->regs.cccr) ||
	     (new->regs.clkcfg != cur->regs.clkcfg))) {

		/* Find out if it is *just* a turbo bit change */
		if ((cur->l == new->l) &&
		    (cur->cccra == new->cccra) &&
		    (cur->b == new->b) &&
		    (cur->half_turbo == new->half_turbo)) {
			/* If the real, current N is a turbo freq and
			   the new N is not a turbo freq, then set
			   TURBO_OFF and do not change N 
			 */
			if ((cur->n > 1) && (new->n == 2)) {
				dpm_fscaler_flags |= DPM_FSCALER_TURBO_OFF;
			}
			/* Else if the current operating point's N is
			   not-turbo and the new N is the desired
			   destination N, then set TURBO_ON
			 */
			else if ((cur->n == 2) && (new->n == current_n)) {
				/* Desired N must be what is current
				   set in the CCCR/CCSR 
				 */
				dpm_fscaler_flags |= DPM_FSCALER_TURBO_ON;
			}
			/* Else, fall through to regular FCS     */
		}
		if (!(dpm_fscaler_flags & DPM_FSCALER_TURBO)) {
			/* It this is not a Turbo bit only change, it
			   must be a regular FCS  
			 */
			dpm_fscaler_flags |= DPM_FSCALER_CPUFREQ;
		}
		loops_per_jiffy = new->lpj;
	}

	if (new->half_turbo != cur->half_turbo) {
		loops_per_jiffy = new->lpj;

		if (new->half_turbo)
			dpm_fscaler_flags |= DPM_FSCALER_HALFTURBO_ON;
		else
			dpm_fscaler_flags |= DPM_FSCALER_HALFTURBO_OFF;
	}

	if (new->regs.voltage != cur->regs.voltage)
		dpm_fscaler_flags |= DPM_FSCALER_VOLTAGE;

}

int dpm_bulverde_set_opt(struct dpm_opt *cur0, struct dpm_opt *new)
{
	/*TODO - argument "cur0" is added in 2.6, see if it may be merged with
	   "cur"  we using here internally
	 */
	unsigned target_v;
	struct dpm_opt *cur;
	struct dpm_md_opt *md_cur, *md_new;

	cur = dpm_active_opt;

	pr_debug("dpm_default_set_opt: %s => %s\n", cur->name, new->name);

	md_cur = &cur->md_opt;
	md_new = &new->md_opt;

	/* fully define the new opt, if necessary, based on values
	   from the current opt
	 */
	dpm_bulverde_fully_define_opt(md_cur, md_new);
	target_v = md_new->v;

	/* In accordance with Yellow Book section 3.7.6.3, "Coupling
	   Voltage Change with Frequency Change", always set the
	   voltage first (setting the FVC bit in the PCFR) and then do
	   the frequency change
	 */
	return dpm_bulverde_fscale(new);
}

static int dpm_bulverde_fscale(struct dpm_opt *new)
{
	struct dpm_opt *cur = dpm_active_opt;
	struct dpm_md_opt *md_cur = &cur->md_opt;
	struct dpm_md_opt *md_new = &new->md_opt;

	compute_fscaler_flags(md_cur, md_new);

	mainstone_fscaler(&md_new->regs);

#ifdef CONFIG_DPM_STATS
	dpm_update_stats(&new->stats, &dpm_active_opt->stats);
#endif
	dpm_active_opt = new;
	mb();

	if (dpm_fscaler_flags & DPM_FSCALER_CPUFREQ) {
		if (md_new->cpu) {
			/* Normal change (not sleep), just compute. Always use
			   the "baseline" lpj and freq */
			dpm_driver_scale(SCALE_POSTCHANGE, new);
		} else {
			/* If sleeping, keep the old LPJ,
			 * so nothing to do here
			 */
		}
	}

	return 0;
}

void dpm_bulverde_fully_define_opt(struct dpm_md_opt *cur,
				   struct dpm_md_opt *new)
{
	if (new->v == -1)
		new->v = cur->v;
	if (new->l == -1)
		new->l = cur->l;
	if (new->n == -1)
		new->n = cur->n;
	if (new->b == -1)
		new->b = cur->b;
	if (new->half_turbo == -1)
		new->half_turbo = cur->half_turbo;
	if (new->cccra == -1)
		new->cccra = cur->cccra;
	if (new->cpll_enabled == -1)
		new->cpll_enabled = cur->cpll_enabled;
	if (new->ppll_enabled == -1)
		new->ppll_enabled = cur->ppll_enabled;
	if (new->sleep_mode == -1)
		new->sleep_mode = cur->sleep_mode;

#ifdef CONFIG_BULVERDE_B0
	/* for "B0"-revision PLLs have the same value */
	new->ppll_enabled = new->cpll_enabled;
#endif
		/*PXA27x manual ("Yellow book") 3.5.5 (Table 3-7) states that CPLL-"On" and
		 *PPLL-"Off"
		 *configuration is forbidden (all others seam to be OK for "B0")
		 *for "C0" boards we suppose that this configuration is also enabled.
		 *PXA27x manual ("Yellow book") also states at 3.5.7.1 (page 3-25)
		 *that "CCCR[PPDIS] and CCCR[CPDIS] must always be identical and
		 *changed together". "If PLLs are to be turned off using xPDIS then
		 *set xPDIS before frequency change and clear xPDIS after frequency
		 *change"
		 *
		 */

	if (new->n > 2) {
		new->turbo = 1;
		/* turbo mode: 13K * L * (N/2) 
		   Shift at the end to divide N by 2 for Turbo mode or
		   by 4 for Half-Turbo mode ) 
		 */
		new->cpu = (13000 * new->l * new->n) >>
		    ((new->half_turbo == 1) ? 2 : 1);
	} else {
		new->turbo = 0;
		/* run mode */
		new->cpu = 13000 * new->l;
	}
	/* lcd freq is derived from L */
	calculate_lcd_freq(new);
	calculate_reg_values(new);
	/* We want to keep a baseline loops_per_jiffy/cpu-freq ratio
	   to work off of for future calculations, especially when
	   emerging from sleep when there is no current cpu frequency
	   to calculate from (because cpu-freq of 0 means sleep). 
	 */
	if (!saved_loops_per_jiffy) {
		saved_loops_per_jiffy = loops_per_jiffy;
		saved_cpu_freq = cur->cpu;
	}
	/* now DPM core have
	 * a dedicated method for updating jiffies when frequency is changed
	 */
	if (new->cpu) {
		/* Normal change (not sleep), just compute. Always use
		   the "baseline" lpj and freq */
		new->lpj =
		    dpm_compute_lpj(saved_loops_per_jiffy, saved_cpu_freq,
				    new->cpu);
	} else {
		/* If sleeping, keep the old LPJ */
		new->lpj = loops_per_jiffy;
	}
}

#if 0
static void ppll_disable(void)
{
	unsigned int clkcfg = bulverde_read_clk_cfg();

	/*"Yellow Book" 3.5.7.1 (page 3-25) */

	/* Step 1 - set xPDIS bit */
	CCCR = CCCR | CCCR_PPDIS_BIT_ON;

	/* Step 2 - change freq
	   Section 3.5.7 of the Yellow Book says that the F
	   bit will be left on after a FCS, so we need to
	   explicitly clear it. But do not change the B bit 
	 */

	clkcfg |= (CLKCFG_F_BIT);
	bulverde_set_freq(clkcfg);
	udelay(100);

	/* Step 3 - clear xPDIS
	   this way CCCR= CCCR & ~CCCR_PPDIS_BIT_ON;
	 */

}

static void ppll_enable()
{
	int tmp_ccsr;
	int tmp_cccr = CCCR;
	unsigned int clkcfg = bulverde_read_clkcfg();

	clkcfg |= (CLKCFG_F_BIT);
	bulverde_set_freq(clkcfg);

}
#endif

static void xpll_on(struct dpm_regs *regs, int fscaler_flags)
{
	int tmp_cccr, tmp_ccsr;
	int new_cpllon=0, new_ppllon=0, cur_cpllon=0;
	int  cur_ppllon=0, start_cpll=0, start_ppll=0;


	tmp_ccsr = CCSR;

	if( (regs->cccr & CCCR_CPDIS_BIT_ON) == 0) new_cpllon=1;
	if( (regs->cccr & CCCR_PPDIS_BIT_ON) == 0) new_ppllon=1;
	if(  ((tmp_ccsr >> 31) & 0x1) == 0  ) cur_cpllon=1;
	if(  ((tmp_ccsr >> 30) & 0x1) == 0  ) cur_ppllon=1;

	if( (new_cpllon == 1) && (cur_cpllon == 0) ){
	  start_cpll=1;
	}
	if( (new_ppllon == 1) && (cur_ppllon == 0) ){
	  start_ppll=1;
 	}

	if (!(fscaler_flags & DPM_FSCALER_XPLLON)){
		return;
	}
	if( (start_cpll == 0) && (start_ppll == 0) ){
	  return;
	}
	/* NOTE: the Yellow Book says that exiting 13M mode requires a
	   PLL relock, which takes at least 120uS, so the book suggests
	   the OS could use a timer to keep busy until it is time to
	   check the CCSR bits which must happen before changing the
	   frequency back.

	   For now, we'll just loop.
	 */

	/* From Yellow Book, page 3-31, section 3.5.7.5 13M Mode

	   Exiting 13M Mode:

	   1. Remain in 13M mode, but early enable the PLL via
	   CCCR[CPDIS, PPDIS]=11, and CCCR[PLL_EARLY_EN]=1. Doing
	   so will allow the PLL to be started early.

	   2. Read CCCR and compare to make sure that the data was
	   correctly written.

	   3. Check to see if CCS[CPLOCK] and CCSR[PPLOCK] bits are
	   both set. Once these bits are both high, the PLLs are
	   locked and you may move on.

	   4. Note that the CPU is still in 13M mode, but the PLLs are
	   started.

	   5. Exit from 13M mode by writing CCCR[CPDIS, PPDIS]=00, but
	   maintain CCCR[PLL_EARLY_EN]=1. This bit will be cleared
	   by the imminent frequency change.
	 */

	/* Step 1 */
	tmp_cccr = CCCR;
	if(start_cpll)tmp_cccr |= CCCR_CPDIS_BIT_ON;
	if(start_ppll)tmp_cccr |= CCCR_PPDIS_BIT_ON;
	tmp_cccr |= CCCR_PLL_EARLY_EN_BIT_ON;

	CCCR = tmp_cccr;

	/* Step 2 */
	tmp_cccr = CCCR;

	if ( (tmp_cccr & CCCR_PLL_EARLY_EN_BIT_ON) != CCCR_PLL_EARLY_EN_BIT_ON) {
		printk(KERN_WARNING
		       "DPM: Warning: PLL_EARLY_EN is NOT on\n");
 	}
	if (  (start_cpll==1) &&
	      ((tmp_cccr & CCCR_CPDIS_BIT_ON) != CCCR_CPDIS_BIT_ON)  ) {
		printk(KERN_WARNING
		       "DPM: Warning: CPDIS is NOT on\n");
	}
	if (  (start_ppll==1) &&
	      (tmp_cccr & CCCR_PPDIS_BIT_ON) != CCCR_PPDIS_BIT_ON  ) {
		printk(KERN_WARNING
		       "DPM: Warning: PPDIS is NOT on\n");
	}

	/* Step 3 */
	{
		/* Note: the point of this is to "wait" for the lock
		   bits to be set; the Yellow Book says this may take
		   a while, but observation indicates that it is
		   instantaneous 
		 */

		long volatile int i = 0;

		int cpll_complete=1;
		int ppll_complete=1;
		if(start_cpll==1) cpll_complete=0;
		if(start_ppll==1) ppll_complete=0;

		/*loop  arbitrary big value to prevent  looping forever */
		for (i = 0; i < 999999; i++) {
			tmp_ccsr = CCSR;

			if( (tmp_ccsr & CCSR_CPLL_LOCKED) ==  CCSR_CPLL_LOCKED ){
			  /*CPLL locked*/
			  cpll_complete=1;
			}
			if( (tmp_ccsr & CCSR_PPLL_LOCKED ) == CCSR_PPLL_LOCKED ){
			  /*PPLL locked*/
			  ppll_complete=1;
			}
			if( (cpll_complete == 1) && (ppll_complete == 1) ){
			  break;
 			}



		}
	}

	/* Step 4: NOP */

	/* Step 5 
	   Clear the PLL disable bits - do NOT do it here. 
	 */


	/* But leave EARLY_EN on; it will be cleared by the frequency change */
	regs->cccr |= CCCR_PLL_EARLY_EN_BIT_ON;
	/* Do not set it now 
	   Step 6: Now go continue on with frequency change 
	   We do this step later as if voltage is too low,
	   we must ensure that it rised up  before entereng to higher
	   freq mode or simultaniously 
	 */
}

static void mainstone_fscaler(struct dpm_regs *regs)
{
	unsigned int cccr, clkcfg = 0;
	unsigned long s;

	/* If no flags are set, don't waste time here, just return */
	if (dpm_fscaler_flags == DPM_FSCALER_NOP)
		return;

	if (! (dpm_fscaler_flags & DPM_FSCALER_ANY_SLEEPMODE))
		local_irq_save(s);

	/* If exiting 13M mode (turn on  PLL(s) ), do some extra work
	   before changing the CPU frequency or voltage.
	   We may turn on a combination of PLLs supported by hardware
	   only. Otherwise xpll_on(...) hang the system.
	 */
	if(dpm_fscaler_flags & DPM_FSCALER_XPLLON)
		xpll_on(regs, dpm_fscaler_flags);

	/* if not sleeping, and have a voltage change 
	   note that SLEEPMODE will handle voltage itself 
	 */
	if (((dpm_fscaler_flags & DPM_FSCALER_ANY_SLEEPMODE) == 0) &&
	    (dpm_fscaler_flags & DPM_FSCALER_VOLTAGE)) {
		if (dpm_fscaler_flags & DPM_FSCALER_CPUFREQ) {
			/* coupled voltage & freq change */
			mainstone_scale_voltage_coupled(regs);
		} else {
			/* Scale CPU voltage un-coupled with freq */
			mainstone_scale_voltage(regs);
		}
	}

	if (dpm_fscaler_flags & DPM_FSCALER_CPUFREQ)	/* Scale CPU freq */
		mainstone_scale_cpufreq(regs);

	if ((dpm_fscaler_flags & DPM_FSCALER_VOLTAGE) &&
	    (dpm_fscaler_flags & DPM_FSCALER_CPUFREQ))
		PCFR &= ~PCFR_FVC;

	if (dpm_fscaler_flags & DPM_FSCALER_TURBO) {

		clkcfg = bulverde_read_clkcfg();

		/* Section 3.5.7 of the Yellow Book says that the F
		   bit will be left on after a FCS, so we need to
		   explicitly clear it. But do not change the B bit 
		 */
		clkcfg &= ~(CLKCFG_F_BIT);

		if (dpm_fscaler_flags & DPM_FSCALER_TURBO_ON) {
			clkcfg = clkcfg | (CLKCFG_T_BIT);
		} else {
			clkcfg = clkcfg & ~(CLKCFG_T_BIT);
		}

		/* enable */
		bulverde_set_freq(clkcfg);
	}

	if ((dpm_fscaler_flags & DPM_FSCALER_HALFTURBO_ON) ||
	    (dpm_fscaler_flags & DPM_FSCALER_HALFTURBO_OFF)) {
		if ((dpm_fscaler_flags & DPM_FSCALER_CPUFREQ) ||
		    (dpm_fscaler_flags & DPM_FSCALER_VOLTAGE)) {

			/*
			   From the Yellow Book, p 3-106:

			   "Any two writes to CLKCFG or PWRMODE
			   registers must be separated by siz 13-MHz
			   cycles.  This requirement is achieved by
			   doing the write to the CLKCFG or POWERMODE
			   reigster, performing a read of CCCR, and
			   then comparing the value in the CLKCFG or
			   POWERMODE register to the written value
			   until it matches."

			   Since the setting of half turbo is a
			   separate write to CLKCFG, we need to adhere
			   to this requirement.
			 */
			cccr = CCCR;
			clkcfg = bulverde_read_clkcfg();
			while (clkcfg != regs->clkcfg)
				clkcfg = bulverde_read_clkcfg();
		}

		if (clkcfg == 0)
			clkcfg = regs->clkcfg;
		/* Turn off f-bit.

		   According to the Yellow Book, page 3-23, "If only
		   HT is set, F is clear, and B is not altered, then
		   the core PLL is not stopped." 
		 */
		clkcfg = clkcfg & ~(CLKCFG_F_BIT);
		/* set half turbo bit */
		if (dpm_fscaler_flags & DPM_FSCALER_HALFTURBO_ON) {
			clkcfg = clkcfg | (CLKCFG_HT_BIT);
		} else {
			clkcfg = clkcfg & ~(CLKCFG_HT_BIT);
		}

		/* enable */
		bulverde_set_freq(clkcfg);
	}

	/* Devices only need to scale on a core frequency
	   change. Half-Turbo changes are separate from the regular
	   frequency changes, so Half-Turbo changes do not need to
	   trigger a device recalculation.

	   NOTE: turbo-mode-only changes could someday also be
	   optimized like Half-Turbo (to not trigger a device
	   recalc). 
	 */

	if (dpm_fscaler_flags & DPM_FSCALER_ANY_SLEEPMODE) {
#ifdef CONFIG_PM
		/* NOTE: voltage needs i2c, so be sure to change
		   voltage BEFORE* calling device_suspend
		 */

		if (dpm_fscaler_flags & DPM_FSCALER_VOLTAGE) {
			/* Scale CPU voltage un-coupled with freq */
			mainstone_scale_voltage(regs);
		}

		if (dpm_fscaler_flags & DPM_FSCALER_SLEEP) {
			pm_suspend(PM_SUSPEND_MEM);
		} else if (dpm_fscaler_flags & DPM_FSCALER_STANDBY) {
			pm_suspend(PM_SUSPEND_STANDBY);
		} else if (dpm_fscaler_flags & DPM_FSCALER_DEEPSLEEP) {
			pm_suspend(PM_SUSPEND_CUSTOM);
		}

		/* Here when we wake up. */

		/* Power on devices again */
		/*   Recursive call to switch back to to task state. */
		dpm_set_os(DPM_TASK_STATE);
#else
	printk(KERN_WARNING
		"DPM: Warinng - can't sleep with CONFIG_PM undefined\n");
#endif /*CONFIG_PM*/
	} else {
		local_irq_restore(s);
	}
}

/* Fully determine the current machine-dependent operating point, and fill in a
   structure presented by the caller. This should only be called when the
   dpm_sem is held. This call can return an error if the system is currently at
   an operating point that could not be constructed by dpm_md_init_opt(). */

void bulverde_get_current_info(struct dpm_md_opt *opt)
{
	unsigned int tmp_cccr;
	unsigned int cpdis;
	unsigned int ppdis;

	/* You should read CCSR to see what's up...but there is no A
	   bit in the CCSR, so we'll grab it from the CCCR.
	 */
	tmp_cccr = CCCR;
	opt->cccra = (tmp_cccr >> 25) & 0x1;	/* cccr[A]: bit 25 */

	/* NOTE: the current voltage is not obtained, but will be left
	   as 0 in the opt which will mean no voltage change at all 
	 */

	opt->regs.cccr = CCSR;

	opt->l = opt->regs.cccr & CCCR_L_MASK;	/* Get L */
	opt->n = (opt->regs.cccr & CCCR_N_MASK) >> 7;	/* Get 2N */

	/* This should never really be less than 2 */
	if (opt->n < 2) {
		opt->n = 2;
	}

	opt->regs.clkcfg = bulverde_read_clkcfg();
	opt->b = (opt->regs.clkcfg >> 3) & 0x1;	/* Fast Bus (b): bit 3 */
	opt->turbo = opt->regs.clkcfg & 0x1;	/* Turbo is bit 1 */
	opt->half_turbo = (opt->regs.clkcfg >> 2) & 0x1;	/* HalfTurbo: bit 2 */

	calculate_lcd_freq(opt);

	/* are any of the PLLs is on? */
	cpdis = ((opt->regs.cccr >> 31) & 0x1);
	ppdis = ((opt->regs.cccr >> 30) & 0x1);
	/* Newer revisions still require that if CPLL is On
	   then PPLL must also be On.
	 */
	if ((cpdis == 0) && (ppdis != 0)) {
	  /* CPLL=On PPLL=Off is NOT supported with hardware.
	    NOTE:"B0"-revision has even more restrictive requirments
	    to PLLs
	  */
 		printk("DPM: cpdis and ppdis are not in sync!\n");
 	}

	opt->cpll_enabled = (cpdis == 0);
	opt->ppll_enabled = (ppdis == 0);

	/* Shift 1 to divide by 2 (because opt->n is really 2*N */
	if (opt->turbo) {
		opt->cpu = (13000 * opt->l * opt->n) >> 1;
	} else {
		/* turbo bit is off, so skip N multiplier (no matter
		   what N really is) and use Run frequency (13K * L)
		 */
		opt->cpu = 13000 * opt->l;
	}
}

int dpm_bulverde_get_opt(struct dpm_opt *opt)
{
	struct dpm_md_opt *md_opt = &opt->md_opt;
	bulverde_get_current_info(md_opt);

	return 0;
}

static int dpm_proc_print_opt(char *buf, struct dpm_opt *opt)
{
	int len = 0;
	struct dpm_md_opt *md_opt = &opt->md_opt;

	len += sprintf(buf + len, "%12s ", opt->name);
	len += p5d(buf + len, md_opt->v);
	len += p5d(buf + len, (md_opt->cpu) / 1000);
	len += p5d(buf + len, md_opt->l);
	len += p5d(buf + len, md_opt->n);
	len += p5d(buf + len, md_opt->cccra);
	len += p5d(buf + len, md_opt->b);
	len += p5d(buf + len, md_opt->half_turbo);
        len += p5d(buf + len, md_opt->cpll_enabled);
        len += p5d(buf + len, md_opt->ppll_enabled);
	len += p5d(buf + len, md_opt->sleep_mode);
	len += p5d(buf + len, (md_opt->lcd) / 1000);
	len += sprintf(buf + len, "\n");
	return len;
}

int
read_proc_dpm_md_opts(char *page, char **start, off_t offset,
		      int count, int *eof, void *data)
{
	int len = 0;
	int limit = offset + count;
	struct dpm_opt *opt;
	struct list_head *opt_list;

	/* FIXME: For now we assume that the complete table,
	 * formatted, fits within one page */
	if (offset >= PAGE_SIZE)
		return 0;
	if (dpm_lock_interruptible())
		return -ERESTARTSYS;
	if (!dpm_initialized)
		len += sprintf(page + len, "DPM is not initialized\n");
	else if (!dpm_enabled)
		len += sprintf(page + len, "DPM is disabled\n");
	else {
		len += sprintf(page + len,
			       "The active DPM policy is \"%s\"\n",
			       dpm_active_policy->name);
		len += sprintf(page + len,
			       "The current operating point is \"%s\"\n",
			       dpm_active_opt->name);
	}

	if (dpm_initialized) {
		len += sprintf(page + len,
			       "Table of all defined operating points, "
			       "frequencies in MHz:\n");

		len += sprintf(page + len,
			       "        Name  Vol   CPU    L    N    A    B   HT  PLL Sleep LCD\n");

		list_for_each(opt_list, &dpm_opts) {
			opt = list_entry(opt_list, struct dpm_opt, list);
			if (len >= PAGE_SIZE)
				BUG();
			if (len >= limit)
				break;
			len += dpm_proc_print_opt(page + len, opt);
		}
	}
	dpm_unlock();
	*eof = 1;
	if (offset >= len)
		return 0;
	*start = page + offset;
	return min(count, len - (int)offset);
}

/*
 *
 * /proc/driver/dpm/md/cmd (Write-only)
 *
 *  This is a catch-all, simple command processor for the Bulverde DPM
 *  implementation. These commands are for experimentation and development
 *  _only_, and may leave the system in an unstable state.
 *
 *  No commands defined now.
 *
 */

int
write_proc_dpm_md_cmd(struct file *file, const char *buffer,
		      unsigned long count, void *data)
{
	char *buf, *tok, *s;
	char *whitespace = " \t\r\n";
	int ret = 0;
	if (current->uid != 0)
		return -EACCES;
	if (count == 0)
		return 0;
	if (!(buf = kmalloc(count + 1, GFP_KERNEL)))
		return -ENOMEM;
	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EFAULT;
	}
	buf[count] = '\0';
	s = buf + strspn(buf, whitespace);
	tok = strsep(&s, whitespace);
	if (strcmp(tok, "define-me") == 0) {
		;
	} else {
		ret = -EINVAL;
	}
	kfree(buf);
	if (ret == 0)
		return count;
	else
		return ret;
}

/****************************************************************************
 *  DPM Idle Handler
 ****************************************************************************/

static void (*orig_idle) (void);

void dpm_bulverde_idle(void)
{
	extern void default_idle(void);

	if (orig_idle)
		orig_idle();
	else
		default_idle();
}

/****************************************************************************
 * Initialization/Exit
 ****************************************************************************/

extern void (*pm_idle) (void);

void dpm_bulverde_startup(void)
{
	orig_idle = pm_idle;
	pm_idle = dpm_idle;
}

void dpm_bulverde_cleanup(void)
{
	pm_idle = orig_idle;
}

int __init dpm_bulverde_init(void)
{
	printk("Mainstone bulverde Dynamic Power Management\n");
	/* NOTE: none of dpm_md member function pointers  (that will actually be
	   called) are allowed to be NULL if DPM is enabled
	   (see linux/dpm.h for details)
	 */
	dpm_md.init_opt = dpm_bulverde_init_opt;
	dpm_md.set_opt = dpm_bulverde_set_opt;
	dpm_md.get_opt = dpm_bulverde_get_opt;
	dpm_md.check_constraint = dpm_default_check_constraint;
	dpm_md.idle = dpm_bulverde_idle;
	dpm_md.startup = dpm_bulverde_startup;
	dpm_md.cleanup = dpm_bulverde_cleanup;

	bulverde_clk_init();
	bulverde_vcs_init();

	return 0;
}

void __exit dpm_bulverde_exit(void){
	bulverde_freq_cleanup();
	bulverde_voltage_cleanup();	
}

__initcall(dpm_bulverde_init);
__exitcall(dpm_bulverde_exit);
