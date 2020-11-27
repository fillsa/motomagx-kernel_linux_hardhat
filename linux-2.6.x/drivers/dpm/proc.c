/*
 * drivers/dpm/proc.c  Dynamic Power Management /proc
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
 * Bishop Brock
 * IBM Research, Austin Center for Low-Power Computing
 * bcbrock@us.ibm.com
 * September, 2002
 *
 */

#include <linux/dpm.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <asm/semaphore.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#define DEBUG
#ifdef DEBUG
#define DPRINT(args...) printk(KERN_CRIT args)
#else
#define DPRINT(args...) do {} while (0)
#endif

/****************************************************************************
 * /proc/driver/dpm interfaces
 *
 * NB: Some of these are borrowed from the 405LP, and may need to be made
 * machine independent.
 ****************************************************************************/

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * /proc/driver/dpm/cmd (Write-Only)
 *
 * Writing a string to this file is equivalent to issuing a DPM command.
 * Currently only one command per "write" is allowed, and there is a maximum on
 * the number of tokens that will be accepted (PAGE_SIZE / sizeof(char *)).
 * DPM can be initialized by a linewise copy of a configuration file to this
 * /proc file.
 *
 * DPM Control
 * -----------
 *
 * init          : dynamicpower_init()
 * enable        : dynamicpower_enable()
 * disable       : dynamicpower_disable()
 * terminate     : dynamicpower_terminate()
 *
 * Policy Control
 * --------------
 *
 * set_policy <policy>          : Set the policy by name
 * set_task_state <pid> <state> : Set the task state for a given pid, 0 = self
 *
 * Policy Creation
 * ---------------
 *
 * create_opt <name> <pp0> ... <ppn>
 *     Create a named operating point from DPM_PP_NBR paramaters.  All
 *     parameters must be  given. Parameter order and meaning are machine
 *     dependent.
 *
 * create_class <name> <opt0> [ ... <optn> ]
 *     Create a named class from 1 or more named operating points.  All
 *     operating points must be defined before the call.
 *
 * create_policy <name> <classopt0> [ ... <classoptn> ]
 *     Create a named policy from DPM_STATES classes or operating
 *     points.  All operating points must be defined before the call.
 *     The order is machine dependent.
 *
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static void
pwarn(char *command, int ntoks, char *requirement, int require)
{
	printk(KERN_WARNING "/proc/driver/dpm/cmd: "
	       "Command %s requires %s%d arguments - %d were given\n",
	       command, requirement, require - 1, ntoks - 1);
}

/*****************************************************************************
 * set a task state
 *****************************************************************************/

static int
dpm_set_task_state(pid_t pid, dpm_state_t task_state)
{
	struct task_struct *p;

	if (task_state == -(DPM_TASK_STATE_LIMIT + 1))
		task_state = DPM_NO_STATE;
	else if (abs(task_state) > DPM_TASK_STATE_LIMIT) {
		dpm_trace(DPM_TRACE_SET_TASK_STATE, pid, task_state, -EINVAL);
		return -EINVAL;
	} else
		task_state += DPM_TASK_STATE;

	read_lock(&tasklist_lock);

	if (pid == 0)
		p = current;
	else
		p = find_task_by_pid(pid);

	if (!p) {
		read_unlock(&tasklist_lock);
		dpm_trace(DPM_TRACE_SET_TASK_STATE, pid, task_state, -ENOENT);
		return -ENOENT;
	}

	p->dpm_state = task_state;
	read_unlock(&tasklist_lock);

	dpm_trace(DPM_TRACE_SET_TASK_STATE, pid, task_state, 0);

	if (pid == 0)
		dpm_set_os(p->dpm_state);


	return 0;
}


static int
write_proc_dpm_cmd (struct file *file, const char *buffer,
		    unsigned long count, void *data)
{
	char *buf, *tok, **tokptrs;
	char *whitespace = " \t\r\n";
	int ret = 0, ntoks;

	if (current->uid != 0)
		return -EACCES;
	if (count == 0)
		return 0;
	if (!(buf = kmalloc(count + 1, GFP_KERNEL)))
		return -ENOMEM;
	if (copy_from_user(buf, buffer, count)) {
		ret = -EFAULT;
		goto out0;
	}

	buf[count] = '\0';

	if (!(tokptrs = (char **)__get_free_page(GFP_KERNEL))) {
		ret = -ENOMEM;
		goto out1;
	}

	ret = -EINVAL;
	ntoks = 0;
	do {
		buf = buf + strspn(buf, whitespace);
		tok = strsep(&buf, whitespace);
		if (*tok == '\0') {
			if (ntoks == 0) {
				ret = 0;
				goto out1;
			} else
				break;
		}
		if (ntoks == (PAGE_SIZE / sizeof(char **)))
			goto out1;
		tokptrs[ntoks++] = tok;
	} while(buf);

	if (ntoks == 1) {
		if (strcmp(tokptrs[0], "init") == 0) {
			ret = dynamicpower_init();
		} else if (strcmp(tokptrs[0], "enable") == 0) {
			ret = dynamicpower_enable();
		} else if (strcmp(tokptrs[0], "disable") == 0) {
			ret = dynamicpower_disable();
		} else if (strcmp(tokptrs[0], "terminate") == 0) {
			ret = dynamicpower_terminate();
		}
	} else if (ntoks == 2) {
		if (strcmp(tokptrs[0], "set_policy") == 0)
			ret = dpm_set_policy(tokptrs[1]);
		else if (strcmp(tokptrs[0], "set_state") == 0)
			ret = dpm_set_op_state(tokptrs[1]);
	} else {
		if (strcmp(tokptrs[0], "set_task_state") == 0) {
			if (ntoks != 3)
				pwarn("set_task_state", ntoks, "", 3);
			else
				ret = dpm_set_task_state(simple_strtol(tokptrs[1],
								       NULL, 0),
							 simple_strtol(tokptrs[2],
								       NULL, 0));
		} else if (strcmp(tokptrs[0], "create_opt") == 0) {
			if (ntoks != DPM_PP_NBR + 2)
				pwarn("create_opt", ntoks,
				      "", DPM_PP_NBR + 2);
			else {
				dpm_md_pp_t pp[DPM_PP_NBR];
				int i;

				for (i = 0; i < DPM_PP_NBR; i++)
					pp[i] = simple_strtol(tokptrs[i + 2],
							      NULL, 0);
				ret = dpm_create_opt(tokptrs[1], pp, DPM_PP_NBR);
			}

		} else if (strcmp(tokptrs[0], "create_class") == 0) {
			if (ntoks < 3)
				pwarn("create_class", ntoks, ">= ", 3);
			else
				ret = dpm_create_class(tokptrs[1], &tokptrs[2],
						       ntoks - 2);

		} else if (strcmp(tokptrs[0], "create_policy") == 0) {
			if (ntoks != (DPM_STATES + 2))
				pwarn("create_policy", ntoks, "",
				      DPM_STATES + 2);
			else
				ret = dpm_create_policy(tokptrs[1],
							&tokptrs[2], ntoks-2);
		}
	}
out1:
	free_page((unsigned long)tokptrs);
out0:
	kfree(buf);
	if (ret == 0)
		return count;
	else
		return ret;
}

#ifdef CONFIG_DPM_STATS

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * /proc/driver/dpm/stats (Read-Only)
 *
 * Reading this file produces the following line for each defined operating
 * state:
 *
 * state_name total_time count opt_name
 *
 * Where:
 *
 * state_name = The operating state name.
 * total_time = The 64-bit number of microseconds spent in this
 *              operating state.
 * count      = The 64-bit number of times this operating state was entered.
 * opt_name   = The name of the operating point currently assigned to this
 *              operating state.
 *
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static int
sprintf_u64(char *buf, int fill, char *s, u64 ul)
{
	int len = 0;
	u32 u, l;

	u = (u32)((ul >> 32) & 0xffffffffU);
	l = (u32)(ul & 0xffffffffU);

	len += sprintf(buf + len, s);
	if (fill)
		len += sprintf(buf + len, "0x%08x%08x", u, l);
	else {
		if (u)
			len += sprintf(buf + len, "0x%x%x", u, l);
		else
			len += sprintf(buf + len, "0x%x", l);
	}
	return len;
}

/*****************************************************************************
 * get statistics for all operating states
 *****************************************************************************/

int
dpm_get_os_stats(struct dpm_stats *stats)
{
	unsigned long flags;

	spin_lock_irqsave(&dpm_policy_lock, flags);
	memcpy(stats, dpm_state_stats, DPM_STATES * sizeof (struct dpm_stats));
	stats[dpm_active_state].total_time +=
		dpm_time() - stats[dpm_active_state].start_time;
	spin_unlock_irqrestore(&dpm_policy_lock, flags);
	return 0;
}

static int
read_proc_dpm_stats(char *page, char **start, off_t offset,
		    int count, int *eof, void *data)
{
	int i, len = 0;
	struct dpm_stats stats[DPM_STATES];

	if (!dpm_enabled) {
		len += sprintf(page + len, "DPM IS DISABLED\n");
		*eof = 1;
		return len;
	}

	dpm_get_os_stats(stats);

	for (i = 0; i < DPM_STATES; i++) {
		len += sprintf(page + len, "%20s", dpm_state_names[i]);
                len += sprintf_u64(page + len, 1, " ",
				   (u64)stats[i].total_time);
		len += sprintf_u64(page + len, 1, " ", (u64)stats[i].count);
		len += sprintf(page + len, " %s\n",
			       dpm_classopt_name(dpm_active_policy,i));
	}

	*eof = 1;
	return len;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * /proc/driver/dpm/opt_stats (Read-Only)
 *
 * Reading this file produces the following line for each defined operating
 * point:
 *
 * name total_time count
 *
 * Where:
 *
 * name       = The operating point name.
 * total_time = The 64-bit number of microseconds spent in this
 *              operating state.
 * count      = The 64-bit number of times this operating point was entered.
 *
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static int
read_proc_dpm_opt_stats(char *page, char **start, off_t offset,
			int count, int *eof, void *data)
{
	int len = 0;
	struct dpm_opt *opt;
	struct list_head *p;
	unsigned long long total_time;

	if (dpm_lock_interruptible())
		return -ERESTARTSYS;

	if (!dpm_enabled) {
		dpm_unlock();
		len += sprintf(page + len, "DPM IS DISABLED\n");
		*eof = 1;
		return len;
	}

	for (p = dpm_opts.next; p != &dpm_opts; p = p->next) {
		opt = list_entry(p, struct dpm_opt, list);
		len += sprintf(page + len, "%s", opt->name);
		total_time = opt->stats.total_time;
		if (opt == dpm_active_opt)
			total_time += dpm_time() - opt->stats.start_time;
		len += sprintf_u64(page + len, 0, " ", opt->stats.total_time);
		len += sprintf_u64(page + len, 0, " ", opt->stats.count);
		len += sprintf(page + len, "\n");
	}

	dpm_unlock();
	*eof = 1;
	return len;
}
#endif /* CONFIG_DPM_STATS */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * /proc/driver/dpm/state (Read-Only)
 *
 * Reading this file produces the following:
 *
 * policy_name os os_name os_opt_name opt_name hz
 *
 * Where:
 *
 * policy_name = The name of the current policy
 * os          = The curret operating state index
 * os_name     = The current operating state name
 * os_opt_name = The name of the implied operating point for the policy and
 *               state.
 * opt_name    = The name of the actual operating point; may be different if
 *               the operating state and operating point are out of sync.
 * hz          = The frequency of the statistics timer
 *
 * If DPM is disabled the line will appear as:
 *
 * N/A -1 N/A N/A N/A <hz>
 *
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static int
read_proc_dpm_state(char *page, char **start, off_t offset,
		    int count, int *eof, void *data)
{
	unsigned long flags;

	int len = 0;

	if (dpm_lock_interruptible())
		return -ERESTARTSYS;

	if (!dpm_enabled) {
		len += sprintf(page + len, "N/A -1 N/A N/A N/A N/A\n");
	} else {

		spin_lock_irqsave(&dpm_policy_lock, flags);
		len += sprintf(page + len,"%s %d %s %s %s\n",
			       dpm_active_policy->name,
			       dpm_active_state,
			       dpm_state_names[dpm_active_state],
			       dpm_classopt_name(dpm_active_policy,
						 dpm_active_state),
			       dpm_active_opt ? dpm_active_opt->name : "none");
		spin_unlock_irqrestore(&dpm_policy_lock, flags);
	}

	dpm_unlock();
	*eof = 1;
	return len;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * /proc/driver/dpm/debug (Read-Only)
 *
 * Whatever it needs to be
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifdef DEBUG
static int
read_proc_dpm_debug(char *page, char **start, off_t offset,
		    int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page + len, "No DEBUG info\n");
	*eof = 1;
	return len;
}
#endif /* DEBUG */

static struct proc_dir_entry *proc_dpm;
static struct proc_dir_entry *proc_dpm_cmd;
static struct proc_dir_entry *proc_dpm_state;

#ifdef CONFIG_DPM_STATS
static struct proc_dir_entry *proc_dpm_stats;
static struct proc_dir_entry *proc_dpm_opt_stats;
#endif

#ifdef DEBUG
static struct proc_dir_entry *proc_dpm_debug;
#endif

#ifdef CONFIG_DPM_TRACE
static struct proc_dir_entry *proc_dpm_trace;
#endif

static int __init
dpm_proc_init(void)
{
	proc_dpm = proc_mkdir("driver/dpm", NULL);

	if (proc_dpm) {

		proc_dpm_cmd =
			create_proc_entry("cmd",
					  S_IWUSR,
					  proc_dpm);
		if (proc_dpm_cmd)
			proc_dpm_cmd->write_proc = write_proc_dpm_cmd;

		proc_dpm_state =
			create_proc_read_entry("state",
					       S_IRUGO,
					       proc_dpm,
					       read_proc_dpm_state,
					       NULL);
#ifdef CONFIG_DPM_STATS
		proc_dpm_stats =
			create_proc_read_entry("stats",
					       S_IRUGO,
					       proc_dpm,
					       read_proc_dpm_stats,
					       NULL);
		proc_dpm_opt_stats =
			create_proc_read_entry("opt_stats",
					       S_IRUGO,
					       proc_dpm,
					       read_proc_dpm_opt_stats,
					       NULL);

#endif /* CONFIG_DPM_STATS */

#ifdef DEBUG
		proc_dpm_debug =
			create_proc_read_entry("debug",
					       S_IRUGO,
					       proc_dpm,
					       read_proc_dpm_debug,
					       NULL);
#endif

#ifdef CONFIG_DPM_TRACE
		proc_dpm_trace =
			create_proc_read_entry("trace",
					       S_IWUSR | S_IRUGO,
					       proc_dpm,
					       read_proc_dpm_trace,
					       NULL);
		if (proc_dpm_trace)
			proc_dpm_trace->write_proc = write_proc_dpm_trace;
#endif
	} else {
	  printk(KERN_ERR "Attempt to create /proc/driver/dpm failed\n");

	}
	return 0;
}

static void __exit
dpm_proc_exit(void)
{
	if (proc_dpm_cmd) {
		remove_proc_entry("cmd", proc_dpm);
		proc_dpm_cmd = NULL;
	}

	if (proc_dpm_state) {
		remove_proc_entry("state", proc_dpm);
		proc_dpm_state = NULL;
	}

#ifdef CONFIG_DPM_STATS
	if (proc_dpm_stats) {
		remove_proc_entry("stats", proc_dpm);
		proc_dpm_stats = NULL;
	}

	if (proc_dpm_opt_stats) {
		remove_proc_entry("opt_stats", proc_dpm);
		proc_dpm_opt_stats = NULL;
	}
#endif /* CONFIG_DPM_STATS */

#ifdef DEBUG
	if (proc_dpm_debug) {
		remove_proc_entry("debug", proc_dpm);
		proc_dpm_debug = NULL;
	}
#endif

#ifdef CONFIG_DPM_TRACE
	if (proc_dpm_trace) {
		remove_proc_entry("trace", proc_dpm);
		proc_dpm_trace = NULL;
	}
#endif

	remove_proc_entry("driver/dpm", NULL);
}



module_init(dpm_proc_init);
module_exit(dpm_proc_exit);

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */

