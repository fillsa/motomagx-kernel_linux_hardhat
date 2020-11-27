/*
 * include/asm-arm/arch-pnx4008/dpm.h
 *
 * DPM driver for PNX4008 - header file
 *
 * Authors: Vitaly Wool, Dmitry Chigirev <source@mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */


#ifndef __ASM_ARM_PNX4008_DPM_H__
#define __ASM_ARM_PNX4008_DPM_H__

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

#define DPM_IDLE_TASK_STATE  0
#define DPM_IDLE_STATE       1
#define DPM_SLEEP_STATE      2
#define DPM_BASE_STATES      3

#define DPM_TASK_STATE_LIMIT 4
#define DPM_TASK_STATE       (DPM_BASE_STATES + DPM_TASK_STATE_LIMIT)
#define DPM_STATES           (DPM_TASK_STATE + DPM_TASK_STATE_LIMIT + 1)
#define DPM_TASK_STATES      (DPM_STATES - DPM_BASE_STATES)

/*
 *length of DPM_STATE_NAMES  is DPM_STATES,
 */
#define DPM_STATE_NAMES                  \
{ "idle-task", "idle", "sleep",          \
  "task-4", "task-3", "task-2", "task-1",\
  "task",                                \
  "task+1", "task+2", "task+3", "task+4" \
}

/*------*/
/* this is the number of specifiable operating point parameters,
 * used by arch-independent DPM-core driver
 */
#define DPM_PP_NBR 6
#define DPM_PARAM_NAMES { \
	"m-volts", "pll-freq", "bus-div", "vfp9-ena", "m2-ena", "sleep_mode" \
};

/* MD operating point parameters */
#define DPM_MD_V		0
#define DPM_MD_PLL_FREQ		1
#define DPM_MD_BUS_DIV		2
#define DPM_MD_VFP9_ENA		3
#define DPM_MD_M2_ENA		4
#define DPM_MD_SLEEP_MODE	5

#ifdef __KERNEL__
#ifndef __ASSEMBLER__

#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/hardware.h>

typedef __u64 dpm_md_count_t;
typedef __u64 dpm_md_time_t;

/* Time in microseconds for dpm stats*/
#define dpm_md_time() (get_cycles() * CLOCK_TICK_RATE / 1000000)

/* Instances of this structure define valid PNX4008 operating points for DPM.
   Voltages are represented in mV, and frequencies are represented in KHz. */

struct dpm_md_opt {
       /* Specified values */
	int cpu;		/* CPU frequency in KHz*/
	int bus_div;		/* PLL-to-Busclk divisor (1, 2 or 4)*/
	int vfp9;		/* VFP9 clk on/off */
	int m2;			/* Matrix2 HCLK on/off */
	int sleep_mode;		/* interface to pm_suspend */
};

#endif /* __ASSEMBLER__ */

#endif /* __KERNEL__ */
#endif /* __ASM_ARM_PNX4008_DPM_H__ */

