/*
 * include/linux/dpm.h  DPM policy management
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
 * Copyright (C) 2002, International Business Machines Corporation
 * All Rights Reserved
 *
 * Robert Paulsen
 * IBM Linux Technology Center
 * rpaulsen@us.ibm.com
 * August, 2002
 *
 */

#ifndef __DPM_H__
#define __DPM_H__

#include <linux/config.h>
#include <linux/device.h>

#define DPM_NO_STATE   -1

#ifndef CONFIG_DPM

/* The above and following constants must always be defined for the
   benefit of the init task and system tasks, although they are
   otherwise ignored if DPM is not configured. */

#define DPM_TASK_STATE 0
#define dpm_set_os(task_state) do {} while (0);

#else /* CONFIG_DPM */

#include <asm/dpm.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/notifier.h>

/* max size of DPM names */
enum {DPM_NAME_SIZE=256};

#include <linux/dpm-trace.h>
#include <linux/list.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>

/* statistics */
struct dpm_stats {
        unsigned long count;
        unsigned long long total_time;
        unsigned long long start_time;
};

extern struct dpm_stats dpm_state_stats[DPM_STATES];

/* update statistics structures */
extern unsigned long long dpm_update_stats(struct dpm_stats *new,
					   struct dpm_stats *old);

typedef int dpm_state_t;
typedef int dpm_md_pp_t;

/* A table of processor-dependent routines, must be initialized by
   platform-dependent boot code.  None of the entries (that will actually be
   called) are allowed to be NULL if DPM is enabled. */

struct dpm_opt;

struct dpm_md {
	int	(*init_opt)(struct dpm_opt *opt);
	int	(*set_opt)(struct dpm_opt *cur, struct dpm_opt *new);
	int	(*get_opt)(struct dpm_opt *opt);
	int	(*check_constraint)(struct constraint_param *param,
				    struct dpm_opt *opt);
	void	(*idle)(void);
	void	(*startup)(void);
	void	(*cleanup)(void);
};


/*****************************************************************************
 * Search a list looking for a named entity.
 * A pointer to the found element is put in the variable named by the
 * "answer" argument (or it is set to zero if not found).
 * The structure's type name is given by the "element_type" argument.
 * The name being looked for is given by the "find_me" argument.
 * The name of the stand-alone list_head is given by the "list_name" argument.
 * Assumes the proper semaphore is held.
 * Assumes the structure's list_head is named "list".
 * Assumes the structure's name is in a field called "name"
 *****************************************************************************/
#define list_find(answer,find_me,list_name,element_type)        \
        do {                                                    \
                element_type            *elm;                   \
                struct list_head        *scan;                  \
                (answer)=0;                                     \
                for(scan=list_name.next;scan!=&list_name;       \
                                scan=scan->next) {              \
                        elm=list_entry(scan,element_type,list); \
                        if (strncmp((find_me),elm->name,        \
                                        DPM_NAME_SIZE)==0) {    \
                                (answer)=elm;                   \
                                break;                          \
                        }                                       \
                }                                               \
        } while(0)

/* internal representation of an operating point */

#define DPM_OP_FORCE	0x0001
#define DPM_OP_NOP	0x0002

struct dpm_opt {
	char			*name;          /* name */
	struct list_head	list;		/* all installed op points */
	dpm_md_pp_t             pp[DPM_PP_NBR]; /* initialization params */
	struct dpm_md_opt	md_opt;         /* machine dependent part */
	int			constrained;	/* is this opt constrained? */
	struct kobject		kobj;		/* kobject */
	struct dpm_stats        stats;          /* statistics */
	int			flags;
};

/* internal representation of a class of op points (to be mapped to an
 * operating state */
struct dpm_class {
	char			*name;          /* name */
	struct list_head	list;		/* all installed classes */
	unsigned		nops;		/* nbr ops in this class */
	struct dpm_opt		**ops;		/* the ops in this class */
	struct kobject		kobj;		/* kobject */
	struct dpm_stats        stats;          /* statistics */
};

/*
 * temporary support for policies to map operating points to either
 * operating pts or classes.  Only one field allowed to be set.
 */

struct dpm_classopt {
	struct dpm_opt		*opt;
	struct dpm_class	*class;
};

/* internal representation of an installed power policy */
struct dpm_policy {
	char			*name;          /* name */
	struct list_head	list;		/* all installed policies */
	struct dpm_classopt     classopt[DPM_STATES]; /* classes/op pts */
	struct kobject		kobj;		/* kobject */
	struct dpm_stats        stats;          /* statistics */
};

/*
 * internal use utility functions for use by DPM
 */

/* DPM semaphore locking. To simplify future expansion, don't 'down' _dpm_lock
   directly.  Also, _dpm_lock must be 'up'ed only by dpm_unlock(). */

extern struct semaphore _dpm_lock;

static inline void
dpm_lock(void)
{
        down(&_dpm_lock);
}

static inline int
dpm_lock_interruptible(void)
{
        if (down_interruptible(&_dpm_lock))
                return -ERESTARTSYS;
        return 0;
}

static inline int
dpm_trylock(void)
{
        if (down_trylock(&_dpm_lock))
                return -EBUSY;
        return 0;
}

void dpm_unlock(void);
void dpm_idle(void);

/* set operating state */
void dpm_set_os(dpm_state_t state);

/*
 * names of DPM stuff for userspace interfaces
 */

extern char *dpm_state_names[DPM_STATES];
extern char *dpm_param_names[DPM_PP_NBR];

/* initialize/terminate the DPM */
int dynamicpower_init(void);
int dynamicpower_terminate(void);

/* (temporarily) disable the DPM */
int dynamicpower_disable(void);

/* re-enable the DPM */
int dynamicpower_enable(void);

/* suspend/resume DPM across a system shutdown */
int dynamicpm_suspend(void);
void dynamicpm_resume(void);

/* create operating point */
int dpm_create_opt(const char *name, const dpm_md_pp_t *pp, int npp);

/* create class of operating points */
int dpm_create_class(const char *name, char **op_names, unsigned nops);

/* create policy */
int dpm_create_policy(const char *name, char **opt_names, int nopts);
int dpm_map_policy_state(struct dpm_policy *policy, int state, char *classopt);

/* destroy policy */
int dpm_destroy_policy(const char *name);

/* activate a power policy */
int dpm_set_policy(const char *name);

/* get name of active power policy */
int dpm_get_policy(char *name);

/* set a raw operating state */
int dpm_set_op_state(const char *name);
int dpm_set_opt(struct dpm_opt *opt, unsigned flags);

/* choose unconstrained operating point from policy */
extern struct dpm_opt *dpm_choose_opt(struct dpm_policy *policy, int state);


/* constraints */
int dpm_check_constraints(struct dpm_opt *opt);
int dpm_default_check_constraint(struct constraint_param *param,
				 struct dpm_opt *opt);
int dpm_show_opconstraints(struct dpm_opt *opt, char * buf);

/* driver scale callbacks */
void dpm_driver_scale(int level, struct dpm_opt *newop);
void dpm_register_scale(struct notifier_block *nb, int level);
void dpm_unregister_scale(struct notifier_block *nb, int level);

/* utils */
extern void dpm_udelay(unsigned uS);
extern void dpm_udelay_from(u64 start, unsigned uS);
extern unsigned long dpm_compute_lpj(unsigned long ref, u_int div, u_int mult);

/*
 * sysfs interface
 */

extern void dpm_sysfs_new_policy(struct dpm_policy *policy);
extern void dpm_sysfs_destroy_policy(struct dpm_policy *policy);
extern void dpm_sysfs_new_class(struct dpm_class *class);
extern void dpm_sysfs_destroy_class(struct dpm_class *class);
extern void dpm_sysfs_new_op(struct dpm_opt *opt);
extern void dpm_sysfs_destroy_op(struct dpm_opt *opt);

extern int proc_pid_dpm_read(struct task_struct*,char*);


/*
 * global data for power management system
 */

/* curently installed policies, classes and operating points */
extern struct list_head		dpm_policies;
extern struct list_head		dpm_classes;
extern struct list_head		dpm_opts;
extern struct semaphore		dpm_policy_sem;
extern spinlock_t		dpm_policy_lock;

/* the currently active policy, class, state, point */
extern struct dpm_policy	*dpm_active_policy;
extern struct dpm_class		*dpm_active_class;
extern dpm_state_t		dpm_active_state;
extern struct dpm_opt		*dpm_active_opt;

/* is DPM initialized and enabled? */
extern int			dpm_initialized;
extern int			dpm_enabled;

extern inline void
dpm_quick_enter_state(int new_state)
{
#ifdef CONFIG_DPM_STATS
	dpm_update_stats(new_state != DPM_NO_STATE ?
			 &dpm_state_stats[new_state] : NULL,
			 dpm_active_state != DPM_NO_STATE ?
			 &dpm_state_stats[dpm_active_state] : NULL);
#endif

        dpm_active_state = new_state;
}

/* Flags for dpm_set_opt().  By default, dpm_set_op() is guaranteed not
   to block the caller, and will arrange to complete asynchronously if
   necessary.

   DPM_SYNC    The operating point is guaranteed to be set when the call
               returns. The call may block.

   DPM_UNLOCK  The caller requires dpm_md_set_opt() to unlock the DPM system
               once the operating point is set.
*/

#define DPM_SYNC      0x01
#define DPM_UNLOCK    0x02

/*
 * Common machine-dependent and board-dependent function wrappers.
 */

extern struct dpm_md dpm_md;

static inline void
dpm_md_startup(void)
{
        if (dpm_md.startup)
                dpm_md.startup();
}


static inline void
dpm_md_cleanup(void)
{
        if (dpm_md.cleanup)
                dpm_md.cleanup();
}


static inline void
dpm_md_idle(void)
{
        if (dpm_md.idle)
                dpm_md.idle();
}


/* Machine-dependent operating point creating/query/setting */


static inline int
dpm_md_init_opt(struct dpm_opt *opt)
{
        if (dpm_md.init_opt)
                return dpm_md.init_opt(opt);
        return 0;
}

static inline int
dpm_md_set_opt(struct dpm_opt *cur, struct dpm_opt *new)
{
        if (dpm_md.set_opt) {
                return dpm_md.set_opt(cur, new);
	}
        return 0;
}

static inline int
dpm_md_get_opt(struct dpm_opt *opt)
{
        if (dpm_md.get_opt)
                return dpm_md.get_opt(opt);
        return 0;
}

static inline int
dpm_md_check_constraint(struct constraint_param *param, struct dpm_opt *opt)
{
        return dpm_md.check_constraint ?
		dpm_md.check_constraint(param, opt) : 1;
}

/*
 * Helper functions
 */

static inline char *
dpm_classopt_name(struct dpm_policy *policy, int state)
{
	return policy->classopt[state].opt ?
		policy->classopt[state].opt->name :
		policy->classopt[state].class->name;
}

#endif /* CONFIG_DPM */
#endif /*__DPM_H__*/
