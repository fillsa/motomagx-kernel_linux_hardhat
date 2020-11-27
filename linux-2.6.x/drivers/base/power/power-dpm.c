/*
 * power-dpm.c -- Dynamic Power Management LDM power hooks
 *
 * (c) 2003 MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express or
 * implied.
 */

#include <linux/device.h>
#include <linux/pm.h>
#include <linux/dpm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include "power.h"

#ifdef CONFIG_HOTPLUG
/*
 * power hotplug events
 */

#define BUFFER_SIZE	1024	/* should be enough memory for the env */
#define NUM_ENVP	32	/* number of env pointers */
static unsigned long sequence_num;
static spinlock_t sequence_lock = SPIN_LOCK_UNLOCKED;

void power_event(char *eventstr)
{
	char *argv [3];
	char **envp = NULL;
	char *buffer = NULL;
	char *scratch;
	int i = 0;
	int retval;
	unsigned long seq;

	if (!hotplug_path[0])
		return;

	envp = kmalloc(NUM_ENVP * sizeof (char *), GFP_KERNEL);
	if (!envp)
		return;
	memset (envp, 0x00, NUM_ENVP * sizeof (char *));

	buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (!buffer)
		goto exit;

	argv [0] = hotplug_path;
	argv [1] = "power";
	argv [2] = 0;

	/* minimal command environment */
	envp [i++] = "HOME=/";
	envp [i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";

	scratch = buffer;

	envp [i++] = scratch;
	scratch += sprintf(scratch, "ACTION=event") + 1;

	spin_lock(&sequence_lock);
	seq = sequence_num++;
	spin_unlock(&sequence_lock);

	envp [i++] = scratch;
	scratch += sprintf(scratch, "SEQNUM=%ld", seq) + 1;
	envp [i++] = scratch;
	scratch += sprintf(scratch, "EVENT=%s", eventstr) + 1;

	pr_debug ("%s: %s %s %s %s %s %s %s\n", __FUNCTION__, argv[0], argv[1],
		  envp[0], envp[1], envp[2], envp[3], envp[4]);
	retval = call_usermodehelper (argv[0], argv, envp, 0);
	if (retval)
		pr_debug ("%s - call_usermodehelper returned %d\n",
			  __FUNCTION__, retval);

exit:
	kfree(buffer);
	kfree(envp);
	return;
}

void device_power_event(struct device * dev, char *eventstr)
{
	char *argv [3];
	char **envp = NULL;
	char *buffer = NULL;
	char *scratch;
	int i = 0;
	int retval;
	unsigned long seq;

	if (!hotplug_path[0])
		return;

	envp = kmalloc(NUM_ENVP * sizeof (char *), GFP_KERNEL);
	if (!envp)
		return;
	memset (envp, 0x00, NUM_ENVP * sizeof (char *));

	buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (!buffer)
		goto exit;

	argv [0] = hotplug_path;
	argv [1] = "power";
	argv [2] = 0;

	/* minimal command environment */
	envp [i++] = "HOME=/";
	envp [i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";

	scratch = buffer;

	envp [i++] = scratch;
	scratch += sprintf(scratch, "ACTION=device-event") + 1;

	spin_lock(&sequence_lock);
	seq = sequence_num++;
	spin_unlock(&sequence_lock);

	envp [i++] = scratch;
	scratch += sprintf(scratch, "SEQNUM=%ld", seq) + 1;
	envp [i++] = scratch;
	scratch += sprintf(scratch, "DEVICE=%s", dev->bus_id) + 1;
	envp [i++] = scratch;
	scratch += sprintf(scratch, "EVENT=%s", eventstr) + 1;

	pr_debug ("%s: %s %s %s %s %s %s %s\n", __FUNCTION__, argv[0], argv[1],
		  envp[0], envp[1], envp[2], envp[3], envp[4]);
	retval = call_usermodehelper (argv[0], argv, envp, 0);
	if (retval)
		pr_debug ("%s - call_usermodehelper returned %d\n",
			  __FUNCTION__, retval);

exit:
	kfree(buffer);
	kfree(envp);
	return;
}
#else /* !defined(CONFIG_HOTPLUG) */
void power_event(char *eventstr)
{
}
void device_power_event(struct device * dev, char *eventstr)
{
}
#endif

EXPORT_SYMBOL(power_event);
EXPORT_SYMBOL(device_power_event);

/*
 * Device constraints
 */

#ifdef CONFIG_DPM
LIST_HEAD(dpm_constraints);
DECLARE_MUTEX(dpm_constraints_sem);

void assert_constraints(struct constraints *constraints)
{
	if (! constraints || constraints->asserted)
		return;

	down(&dpm_constraints_sem);
	constraints->asserted = 1;
	list_add_tail(&constraints->entry, &dpm_constraints);
	up(&dpm_constraints_sem);

	/* DPM-PM-TODO: Check against DPM state. */

}


void deassert_constraints(struct constraints *constraints)
{
	if (! constraints || ! constraints->asserted)
		return;

	down(&dpm_constraints_sem);
	constraints->asserted = 0;
	list_del_init(&constraints->entry);
	up(&dpm_constraints_sem);
}


EXPORT_SYMBOL(assert_constraints);
EXPORT_SYMBOL(deassert_constraints);

static ssize_t
#if 1 /* Mobilinux 4.x */
constraints_show(struct device * dev,
#else /* Mobilinux 4.x */
constraints_show(struct device * dev, struct device_attribute *attr,
#endif /* Mobilinux 4.x */
		 char * buf)
{
	int i, cnt = 0;

	if (dev->constraints) {
		for (i = 0; i < dev->constraints->count; i++) {
			cnt += sprintf(buf + cnt,"%s: min=%d max=%d\n",
				       dpm_param_names[dev->constraints->param[i].id],
				       dev->constraints->param[i].min,
				       dev->constraints->param[i].max);
		}

		cnt += sprintf(buf + cnt,"asserted=%s violations=%d\n",
			       dev->constraints->asserted ?
			       "yes" : "no", dev->constraints->violations);
	} else {
		cnt += sprintf(buf + cnt,"none\n");
	}

	return cnt;
}

static ssize_t
#if 1 /* Mobilinux 4.x */
constraints_store(struct device * dev,
#else /* Mobilinux 4.x */
constraints_store(struct device * dev, struct device_attribute *attr,
#endif /* Mobilinux 4.x */
		  const char * buf, size_t count)
{
	int num_args, paramid, min, max;
	int cidx;
	const char *cp, *paramname;
	int paramnamelen;
	int provisional = 0;
	int ret = 0;

	if (!dev->constraints) {
		if (! (dev->constraints = kmalloc(sizeof(struct constraints),
						  GFP_KERNEL)))
			return -EINVAL;

		memset(dev->constraints, 0,
		       sizeof(struct constraints));
		provisional = 1;
	}

	cp = buf;
	while((cp - buf < count) && *cp && (*cp == ' '))
		cp++;

	paramname = cp;

	while((cp - buf < count) && *cp && (*cp != ' '))
		cp++;

	paramnamelen = cp - paramname;
	num_args = sscanf(cp, "%d %d", &min, &max);

	if (num_args != 2) {
		printk("DPM: Need 2 integer parameters for constraint min/max.\n");
		ret = -EINVAL;
		goto out;
	}

	for (paramid = 0; paramid < DPM_PP_NBR; paramid++) {
		if (strncmp(paramname, dpm_param_names[paramid], paramnamelen) == 0)
			break;
	}

	if (paramid >= DPM_PP_NBR) {
		printk("DPM: Unknown power parameter name in device constraints\n");
		ret = -EINVAL;
		goto out;
	}

	for (cidx = 0; cidx < dev->constraints->count; cidx++)
		/*
		 * If the new range overlaps an existing range,
		 * modify the existing one.
		 */

		if ((dev->constraints->param[cidx].id == paramid) &&
		    ((max == -1) || 
		     (max >= dev->constraints->param[cidx].min)) &&
		    ((min == -1) ||
		     (min <= dev->constraints->param[cidx].max)))
			break;

	if (cidx >= DPM_CONSTRAINT_PARAMS_MAX) {
		ret = -EINVAL;
		goto out;
	}

	/* Error if max is less than min */
	if (max < min) {
		printk("DPM: Max value of the constraint should not be less than min\n");
		ret = -EINVAL;
		goto out;
	}

	dev->constraints->param[cidx].id = paramid;
	dev->constraints->param[cidx].max = max;
	dev->constraints->param[cidx].min = min;

	if (cidx == dev->constraints->count)
		dev->constraints->count++;

	/* New constraints should start off with same state as power
	   state */
#if 1 /* Mobilinux 4.x */
	if (provisional && (dev->power.power_state == PM_SUSPEND_ON))
#else /* Mobilinux 4.x */
	if (provisional && (dev->power.power_state.event == PM_EVENT_ON))
#endif /* Mobilinux 4.x */
		assert_constraints(dev->constraints);

out:

	if (provisional && (ret < 0)) {
		kfree(dev->constraints);
		dev->constraints = NULL;
	}

	return ret < 0 ? ret : count;
}

DEVICE_ATTR(constraints,S_IWUSR | S_IRUGO,
            constraints_show,constraints_store);

#else /* CONFIG_DPM */
void assert_constraints(struct constraints *constraints)
{
}

void deassert_constraints(struct constraints *constraints)
{
}
#endif /* CONFIG_DPM */

#ifdef CONFIG_DPM

/*
 * Driver scale callbacks
 */

static struct notifier_block *dpm_scale_notifier_list[SCALE_MAX];
static DECLARE_MUTEX(dpm_scale_sem);

/* This function may be called by the platform frequency scaler before
   or after a frequency change, in order to let drivers adjust any
   clocks or calculations for the new frequency. */

void dpm_driver_scale(int level, struct dpm_opt *newop)
{
	if (down_trylock(&dpm_scale_sem))
		return;

	notifier_call_chain(&dpm_scale_notifier_list[level], level, newop);
	up(&dpm_scale_sem);
}

void dpm_register_scale(struct notifier_block *nb, int level)
{
	down(&dpm_scale_sem);
	notifier_chain_register(&dpm_scale_notifier_list[level], nb);
	up(&dpm_scale_sem);
}

void dpm_unregister_scale(struct notifier_block *nb, int level)
{
	down(&dpm_scale_sem);
	notifier_chain_unregister(&dpm_scale_notifier_list[level], nb);
	up(&dpm_scale_sem);
}

EXPORT_SYMBOL(dpm_driver_scale);
EXPORT_SYMBOL(dpm_register_scale);
EXPORT_SYMBOL(dpm_unregister_scale);

int dpm_constraint_rejects = 0;


int
dpm_default_check_constraint(struct constraint_param *param,
			     struct dpm_opt *opt)
{
	return (opt->pp[param->id] == -1) ||
		((param->min == -1 || opt->pp[param->id] >= param->min) &&
		 (param->max == -1 || opt->pp[param->id] <= param->max));
}

static int
dpm_check_a_constraint(struct constraints *constraints, struct dpm_opt *opt)
{
	int i;
	int failid = -1;
	int ppconstraint[DPM_PP_NBR];


	if (! constraints || !constraints->asserted)
		return 1;

	/*
	 * ppconstraint[ppid] == 0  means power param has not been checked
	 *                          for a constraint
	 *                    == -1 means power param has matched a constraint
	 *                     > 0  means constraint #n-1 mismatched
	 *
	 * failid == pp id of (a) failed constraint
	 */

	memset(ppconstraint, 0, sizeof(ppconstraint));

	for (i = 0; i < constraints->count; i++) {
		struct constraint_param *param = &constraints->param[i];

		if (! dpm_md_check_constraint(param, opt)) {
			if (ppconstraint[param->id] == 0) {
				failid = param->id;
				ppconstraint[failid] = i+1;
			}
		} else
			ppconstraint[param->id] = -1;
	}

	if ((failid >= 0) && (ppconstraint[failid] > 0)) {
#ifdef CONFIG_DPM_TRACE
		struct constraint_param *param =
			&constraints->param[ppconstraint[failid]-1];

		dpm_trace(DPM_TRACE_CONSTRAINT_ASSERTED,
			  param->id, param->min, param->max,
			  opt);
#endif
		return 0;
	}

	return 1;
}

int dpm_check_constraints(struct dpm_opt *opt)
{
	struct list_head * entry;
	int valid = 1;

	list_for_each(entry,&dpm_constraints) {
		struct constraints *constraints =
			list_entry(entry, struct constraints, entry);
		if (!dpm_check_a_constraint(constraints, opt)) {
			constraints->violations++;
			dpm_constraint_rejects++;
			valid = 0;
		}
	}

	return valid;
}

EXPORT_SYMBOL(dpm_default_check_constraint);
EXPORT_SYMBOL(dpm_check_constraints);

int dpm_show_opconstraints(struct dpm_opt *opt, char * buf)
{
#ifdef CONFIG_PM
	struct list_head * entry;
	int len = 0;

	list_for_each_prev(entry,&dpm_active) {
		struct device * dev = to_device(entry);

		if (!dpm_check_a_constraint(dev->constraints, opt)) {
			len += sprintf(buf + len, "%s/%s\n", dev->bus->name,
				       dev->bus_id);
		}
	}

	return len;
#else /* CONFIG_PM */
	return 0;
#endif /* CONFIG_PM */
}

void dpm_force_off_constrainers(struct dpm_opt *opt)
{
#ifdef CONFIG_PM
	struct list_head * entry;

	list_for_each_prev(entry,&dpm_active) {
		struct device * dev = to_device(entry);

		if (!dpm_check_a_constraint(dev->constraints, opt)) {
			suspend_device(dev, 3);
		}
	}
#endif
}

EXPORT_SYMBOL(dpm_force_off_constrainers);
#endif /* CONFIG_DPM */

