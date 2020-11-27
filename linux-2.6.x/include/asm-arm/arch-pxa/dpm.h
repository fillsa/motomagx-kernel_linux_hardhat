/*
 * include/asm-arm/arch-pxa/dpm.h
 *
 * Bulverde-specific definitions for DPM.  If further PXA boards are
 * supported in the future, will split into board-specific files.
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
 * Copyright (C) 2002, MontaVista Software <source@mvista.com>
 *
 * Based on arch/ppc/platforms/ibm405lp_dpm.h by Bishop Brock.
 */

#ifndef __ASM_ARM_PXA_DPM_H__
#define __ASM_ARM_PXA_DPM_H__

/*
 * machine dependent operating state
 *
 * An operating state is a cpu execution state that has implications for power
 * management. The DPM will select operating points based largely on the
 * current operating state.
 *
 * DPM_STATES is the number of supported operating states. Valid operating
 * states are from 0 to DPM_STATES-1 but when setting an operating state the
 * kernel should only specify a state from the set of "base states" and should
 * do so by name.  During the context switch the new operating state is simply
 * extracted from current->dpm_state.
 *
 * task states:
 *
 * APIs that reference task states use the range -(DPM_TASK_STATE_LIMIT + 1)
 * through +DPM_TASK_STATE_LIMIT.  This value is added to DPM_TASK_STATE to
 * obtain the downward or upward adjusted task state value. The
 * -(DPM_TASK_STATE_LIMIT + 1) value is interpreted specially, and equates to
 * DPM_NO_STATE.
 *
 * Tasks inherit their task operating states across calls to
 * fork(). DPM_TASK_STATE is the default operating state for all tasks, and is
 * inherited from init.  Tasks can change (or have changed) their tasks states
 * using the DPM_SET_TASK_STATE variant of the sys_dpm() system call.  */

#define DPM_NO_STATE        -1

#define DPM_RELOCK_STATE     0
#define DPM_IDLE_TASK_STATE  1
#define DPM_IDLE_STATE       2
#define DPM_SLEEP_STATE      3
#define DPM_BASE_STATES      4

#define DPM_TASK_STATE_LIMIT 4
#define DPM_TASK_STATE       (DPM_BASE_STATES + DPM_TASK_STATE_LIMIT)
#define DPM_STATES           (DPM_TASK_STATE + DPM_TASK_STATE_LIMIT + 1)
#define DPM_TASK_STATES      (DPM_STATES - DPM_BASE_STATES)

/*
 *length of DPM_STATE_NAMES  is DPM_STATES,
 */
#define DPM_STATE_NAMES                  \
{ "relock", "idle-task", "idle", "sleep",\
  "task-4", "task-3", "task-2", "task-1",\
  "task",                                \
  "task+1", "task+2", "task+3", "task+4" \
}

/*------*/	 
/* MD operating point parameters (consecutive numbers, totally DPM_PP_NBR) */
#define DPM_MD_V                0  /* Voltage */
#define DPM_MD_PLL_L            1  /* L */
#define DPM_MD_PLL_N            2  /* N	*/
#define DPM_MD_PLL_B            3  /* B */
#define DPM_MD_HALF_TURBO       4  /* Cuts turbo mode in half */
#define DPM_MD_CCCRA            5  /* The A bit in the CCCR is
				      for MEMC clocks */
#define DPM_MD_CPLL_ON          6  /* Is the core PLL on? NOT yet implemented*/
#define DPM_MD_PPLL_ON          7  /* Is the periph PLL on? NOT yet implemented*/
#define DPM_MD_SLEEP_MODE       8  /* Sleep mode, from pm.h */
#define DPM_MD_PLL_LCD          9  /* calculated value */


enum
{
  CPUMODE_RUN,
  CPUMODE_IDLE,
  CPUMODE_STANDBY,
  CPUMODE_SLEEP,
  CPUMODE_RESERVED,
  CPUMODE_SENSE,
  CPUMODE_RESERVED2,
  CPUMODE_DEEPSLEEP,
};

#define PM_SUSPEND_CUSTOM	((__force suspend_state_t) 2)

/* this is the number of specifiable operating point parameters,
 * used by arch-independent DPM-core driver
 */
#define DPM_PP_NBR 10
#define DPM_PARAM_NAMES {"v","l","n","b","half_turbo","cccra","cpll-on", "ppll-on","sleep_mode", "lcd"};

#ifdef __KERNEL__
#ifndef __ASSEMBLER__

#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/hardware.h>
#include <asm/arch/pxa-regs.h>

#define dpm_time() (OSCR)
#define DPM_NSEC_PER_TICK 308 /* nanoseconds per tick */
#define dpm_time_to_usec(ticks) (((ticks) * DPM_NSEC_PER_TICK * 2 + 1) / (1000*2))

extern int cpu_mode_set(int mode);

struct dpm_regs {
	unsigned int	cccr;
	unsigned int	clkcfg;
	unsigned int	voltage;	/*This is not a register.*/
};   


/* Instances of this structure define valid Bulverde operating points for DPM.
   Voltages are represented in mV, and frequencies are represented in KHz. */ 

struct dpm_md_opt {
       /* Specified values */
        int v;         /* Target voltage in mV*/
        int l;         /* Run Mode to Oscillator ratio */
        int n;         /* Turbo-Mode to Run-Mode ratio */
        int b;         /* Fast Bus Mode */
        int half_turbo;/* Half Turbo bit */
        int cccra;     /* the 'A' bit of the CCCR register, 
			  alternate MEMC clock */
	int cpll_enabled; /* core PLL is ON?  (Bulverde >="C0" feature)*/
	int ppll_enabled; /* peripherial PLL is ON? (Bulverde >="C0" feature)*/

	int sleep_mode;
        /*Calculated values*/
	unsigned int lcd;	/*in KHz  */
	unsigned int lpj;	/*New value for loops_per_jiffy */
	unsigned int cpu;	/*CPU frequency in KHz */
	unsigned int turbo;     /* Turbo bit in clkcfg */

	struct dpm_regs regs;   /* Register values */
};

#if 0
/* Compares two instances of DPM settings, returns 0/1 */
static int dpm_md_opt_equals(struct dpm_md_opt *a, struct dpm_md_opt *b){
	int ret = 0;
	if(a->v == b->v &&
	   a->l == b->l &&
	   a->n == b->n &&
	   a->b == b->b &&
	   a->half_turbo == b->half_turbo &&
	   a->cccra == b->cccra &&
	   a->cpll_enabled == b->cpll_enabled &&
	   a->ppll_enabled == b->ppll_enabled       
	   a->sleep_mode == b->sleep_mode &&
	   ){
	  ret = 1;
	}
	return ret;
}

/* Debugging function */
static void print_dpm_md_opt(struct dpm_md_opt *a){
	printk("dpm_md: v=%d, l=%d, n=%d, b=%d,",a->v,a->l,a->n,a->b);
	printk("half_turbo=%d, cccra=%d,",a->half_turbo,a->cccra);
	printk("cpll_enabled=%d, ",a->cpll_enabled);
	printk("ppll_enabled=%d, sleep_mode=%d",a->ppll_enabled,
		a->sleep_mode);
}
#endif


#endif /* __ASSEMBLER__ */


#endif /* __KERNEL__ */
#endif /* __ASM_BULVERDE_DPM_H__ */

