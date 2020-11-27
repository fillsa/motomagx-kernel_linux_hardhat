/*
 * include/asm-arm/arch-omap/dpm.h
 * DPM for TI OMAP
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

#ifndef __ASM_OMAP_DPM_H__
#define __ASM_OMAP_DPM_H__

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

#define DPM_IDLE_TASK_STATE  0
#define DPM_IDLE_STATE       1
#define DPM_SLEEP_STATE      2
#define DPM_BASE_STATES      3

#define DPM_TASK_STATE_LIMIT 4
#define DPM_TASK_STATE       (DPM_BASE_STATES + DPM_TASK_STATE_LIMIT)
#define DPM_STATES           (DPM_TASK_STATE + DPM_TASK_STATE_LIMIT + 1)
#define DPM_TASK_STATES      (DPM_STATES - DPM_BASE_STATES)

#define DPM_STATE_NAMES                  \
{ "idle-task", "idle", "sleep",          \
  "task-4", "task-3", "task-2", "task-1",\
  "task",                                \
  "task+1", "task+2", "task+3", "task+4" \
}

#define DPM_PARAM_NAMES				\
{ "v", "dpll-mult", "dpll-div", "arm-div",	\
  "tc-div", "per-div", "dsp-div", "dspmmu-div",	\
  "lcd-div" }

/* MD operating point parameters */
#define DPM_MD_V		0  /* voltage */
#define DPM_MD_DPLL_MULT	1  /* DPLL_CTL PLL MULT */
#define DPM_MD_DPLL_DIV		2  /* DPLL_CTL PLL divisor */
#define DPM_MD_ARM_DIV		3  /* ARM_CKCTL ARMDIV divisor */
#define DPM_MD_TC_DIV		4  /* ARM_CKCTL TCDIV divisor */
#define DPM_MD_PER_DIV		5  /* Peripheral divisor */
#define DPM_MD_DSP_DIV		6  /* ARM_CKCTL DSPDIV divisor **/
#define DPM_MD_DSPMMU_DIV	7  /**/
#define DPM_MD_LCD_DIV		8  /**/

#define DPM_PP_NBR 9

extern unsigned long omap_mpu_timer_read(int timer);
extern unsigned long omap_mpu_timer_ticks_to_usecs(unsigned long nr_ticks);

#define dpm_time() ~omap_mpu_timer_read(0)
#define dpm_time_to_usec(ticks) omap_mpu_timer_ticks_to_usecs(ticks)

#ifndef __ASSEMBLER__

#include <linux/types.h>

/* The register values only include the bits manipulated by the DPM
   system - other bits that also happen to reside in these registers are
   not represented here. */

struct dpm_regs {
	u16 arm_msk; /* Clock Divider mask */
	u16 arm_ctl; /* Clock Divider register */
	u16 dpll_ctl; /* DPLL 1 Multiplier and Divider Register */
};

/* Instances of this structure define valid Innovator operating points for DPM.
   Voltages are represented in mV, and frequencies are represented in KHz. */

struct dpm_md_opt {
        unsigned int v;         /* Target voltage in mV */
	unsigned int dpll;	/* in KHz */
	unsigned int cpu;	/* CPU frequency in KHz */
	unsigned int tc;	/* in KHz */
	unsigned int per;	/* in KHz */
	unsigned int dsp;	/* in KHz */
	unsigned int dspmmu;	/* in KHz */
	unsigned int lcd;	/* in KHz */
	struct dpm_regs regs;   /* Register values */
};

#endif /* __ASSEMBLER__ */
#endif /* __ASM_OMAP_DPM_H__ */
