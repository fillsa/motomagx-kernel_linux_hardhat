/*
 * include/asm-i386/dpm.h        Platform-dependent DPM defines for x86
 *
 * 2003 (c) MontaVista Software, Inc.  This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __ASM_DPM_H__
#define __ASM_DPM_H__

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
#define DPM_BASE_STATES      2

#define DPM_TASK_STATE_LIMIT 4
#define DPM_TASK_STATE       (DPM_BASE_STATES + DPM_TASK_STATE_LIMIT)
#define DPM_STATES           (DPM_TASK_STATE + DPM_TASK_STATE_LIMIT + 1)
#define DPM_TASK_STATES      (DPM_STATES - DPM_BASE_STATES)

#define DPM_STATE_NAMES                  \
{ "idle-task", "idle",\
  "task-4", "task-3", "task-2", "task-1",\
  "task",                                \
  "task+1", "task+2", "task+3", "task+4" \
}

#ifdef CONFIG_DPM_CENTRINO
#include <asm/dpm-centrino.h>
#else
#include <asm/dpm-generic.h>
#endif

#ifndef __ASSEMBLER__

#include <linux/types.h>

/* Board-dependent routines. */

struct dpm_opt;

struct dpm_bd {
	int (*startup)(void);				/* startup */
	void (*cleanup)(void);				/* terminate */
	int (*init_opt)(struct dpm_opt *opt);		/* init an opt */
	int (*get_opt)(struct dpm_opt *opt);		/* get current opt */
	int (*set_opt)(struct dpm_md_opt *md_opt);	/* set opt */
};

extern struct dpm_bd dpm_bd;

#endif /* __ASSEMBLER__ */
#endif /* __ASM_DPM_H__ */
