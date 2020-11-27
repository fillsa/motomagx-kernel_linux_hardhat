/*
 * drivers/dpm/dpm-ui.c - userspace interface to Dynamic Power Management
 *
 * (c) 2003 MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express or
 * implied.
 */

#include <linux/dpm.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/time.h>

/* Common sysfs/proc support */

char *dpm_state_names[DPM_STATES] = DPM_STATE_NAMES;
char *dpm_param_names[DPM_PP_NBR] = DPM_PARAM_NAMES;

#define MAXTOKENS 80

static int
tokenizer(char **tbuf, const char *userbuf, ssize_t n, char **tokptrs,
	  int maxtoks)
{
	char *cp, *tok;
	char *whitespace = " \t\r\n";
	int ntoks = 0;

	if (n > MAXTOKENS)
		return -EINVAL;

	if (!(cp = kmalloc(n + 1, GFP_KERNEL)))
		return -ENOMEM;

	*tbuf = cp;
	memcpy(cp, userbuf, n);
	cp[n] = '\0';

	do {
		cp = cp + strspn(cp, whitespace);
		tok = strsep(&cp, whitespace);
		if ((*tok == '\0') || (ntoks == maxtoks))
			break;
		tokptrs[ntoks++] = tok;
	} while(cp);

	return ntoks;
}


/* SysFS Interface */

#define dpm_attr(_name,_prefix) \
static struct subsys_attribute _prefix##_attr = { \
        .attr   = {                             \
                .name = __stringify(_name),     \
                .mode = 0644,                   \
        },                                      \
        .show   = _prefix##_show,                 \
        .store  = _prefix##_store,                \
}


static void dpm_kobj_release(struct kobject *kobj)
{
	/*
	 * No sysfs/kobject state to release, DPM layer will handle the
	 * the containing object.
	 */

	return;
}

/*
 * Top-level control
 */

static ssize_t dpm_control_show(struct subsystem * subsys, char * buf)
{
	unsigned long flags;
	ssize_t len = 0;

	if (dpm_lock_interruptible())
		return -ERESTARTSYS;

	if (!dpm_enabled) {
		len += sprintf(buf, "disabled\n");
	} else {
		spin_lock_irqsave(&dpm_policy_lock, flags);
		len += sprintf(buf,"enabled %s %d %s %s %s\n",
			       dpm_active_policy->name,
			       dpm_active_state,
			       dpm_state_names[dpm_active_state],
			       dpm_classopt_name(dpm_active_policy,
						 dpm_active_state),
			       dpm_active_opt ? dpm_active_opt->name : "[none]");
		spin_unlock_irqrestore(&dpm_policy_lock, flags);
	}

	dpm_unlock();
	return len;
}

static ssize_t dpm_control_store(struct subsystem * subsys, const char * buf,
				 size_t n)
{
	int error = 0;

	if (strncmp(buf, "init", 4) == 0) {
		error = dynamicpower_init();
	} else if (strncmp(buf, "enable", 6) == 0) {
		error = dynamicpower_enable();
	} else if (strncmp(buf, "disable", 7) == 0) {
		error = dynamicpower_disable();
	} else if (strncmp(buf, "terminate", 9) == 0) {
		error = dynamicpower_terminate();
	} else
		error = -EINVAL;

        return error ? error : n;
}

dpm_attr(control,dpm_control);

static struct attribute * g[] = {
        &dpm_control_attr.attr,
        NULL,
};

static struct attribute_group dpm_attr_group = {
        .attrs = g,
};

decl_subsys(dpm, NULL, NULL);

/*
 * policy
 */

struct dpm_policy_attribute {
        struct attribute        attr;
        ssize_t (*show)(struct kobject * kobj, char * buf);
        ssize_t (*store)(struct kobject * kobj, const char * buf, size_t count);
};

#define to_policy(obj) container_of(obj,struct dpm_policy,kobj)
#define to_policy_attr(_attr) container_of(_attr,struct dpm_policy_attribute,attr)

static struct kobject dpm_policy_kobj = {
	.kset = &dpm_subsys.kset,
};

static ssize_t policy_control_show(struct subsystem * subsys, char * buf)
{
	ssize_t len = 0;
	struct list_head  * p;

	if (dpm_lock_interruptible())
		return -ERESTARTSYS;

	len += sprintf(buf + len, "policies: ");

	list_for_each(p, &dpm_policies) {
		len += sprintf(buf + len, "%s ",
			       ((struct dpm_policy *)
				list_entry(p, struct dpm_policy, list))->name);
	}

	len += sprintf(buf + len, "\n");
	dpm_unlock();
	return len;
}

static ssize_t policy_control_store(struct subsystem * subsys, const char * buf,
				   size_t n)
{
	int error = 0;
	char *tbuf = NULL;
	char *token[MAXTOKENS];
	int ntoks = tokenizer(&tbuf, buf, n, (char **) &token, MAXTOKENS);

	if (ntoks <= 0) {
		error = ntoks;
		goto out;
	}

	if (strcmp(token[0],"create") == 0) {
		error = dpm_create_policy(token[1], &token[2], ntoks - 2);
	} else if (strcmp(token[0],"set") == 0) {
		if (ntoks != 2)
			printk("dpm: policy set requires 1 policy name argument\n");
		else
			error = dpm_set_policy(token[1]);
	} else
		error = -EINVAL;

 out:
	if (tbuf)
		kfree(tbuf);
        return error ? error : n;
}

static ssize_t active_policy_show(struct subsystem * subsys, char * buf)
{
	unsigned long flags;
	ssize_t len = 0;

	if (dpm_lock_interruptible())
		return -ERESTARTSYS;

	if (!dpm_enabled || (dpm_active_state == DPM_NO_STATE)) {
		len += sprintf(buf + len, "[none]\n");
	} else {
		spin_lock_irqsave(&dpm_policy_lock, flags);
		len += sprintf(buf + len,"%s\n",
			       dpm_active_policy->name);
		spin_unlock_irqrestore(&dpm_policy_lock, flags);
	}

	dpm_unlock();
	return len;
}

static ssize_t active_policy_store(struct subsystem * subsys, const char * buf,
				   size_t n)
{
	int error = 0;
	char *tbuf = NULL;
	char *token[MAXTOKENS];
	int ntoks = tokenizer(&tbuf, buf, n, (char **) &token, MAXTOKENS);

	if (ntoks <= 0) {
		error = ntoks;
		goto out;
	}

	error = dpm_set_policy(token[0]);

 out:
	if (tbuf)
		kfree(tbuf);
        return error ? error : n;
}

dpm_attr(control,policy_control);
dpm_attr(active,active_policy);

#ifdef CONFIG_DPM_STATS
static ssize_t policy_stats_show(struct subsystem * subsys, char * buf)
{
	int len = 0;
	struct dpm_policy *policy;
	struct list_head *p;
	unsigned long long total_time;

	if (dpm_lock_interruptible())
		return -ERESTARTSYS;

	if (!dpm_enabled) {
		dpm_unlock();
		len += sprintf(buf + len, "DPM IS DISABLED\n");
		return len;
	}

	for (p = dpm_policies.next; p != &dpm_policies; p = p->next) {
		policy = list_entry(p, struct dpm_policy, list);
		len += sprintf(buf + len, "policy: %s", policy->name);
		total_time = policy->stats.total_time;
		if (policy == dpm_active_policy)
			total_time += (unsigned long long) dpm_time() -
				policy->stats.start_time;
		len += sprintf(buf + len, " ticks: %Lu times: %lu\n",
			       (unsigned long long) dpm_time_to_usec(total_time),
			       policy->stats.count);
	}

	dpm_unlock();
	return len;
}

static ssize_t policy_stats_store(struct subsystem * subsys, const char * buf,
				  size_t n)
{
	return n;
}

dpm_attr(stats, policy_stats);
#endif /* CONFIG_DPM_STATS */

static ssize_t a_policy_control_show(struct kobject * kobj, char * buf)
{
	struct dpm_policy *policy = to_policy(kobj);
	ssize_t len = 0;
	int i;

	len += sprintf(buf + len, "ops/classes: ");

	for (i = 0; i < DPM_STATES; i++)
		len += sprintf(buf + len, "%s ", dpm_classopt_name(policy,i));

	len += sprintf(buf + len, "\n");
	return len;
}

static ssize_t a_policy_control_store(struct kobject * kobj, const char * buf,
				      size_t n)
{
	struct dpm_policy *policy = to_policy(kobj);
	int error = 0;
	char *tbuf = NULL;
	char *token[MAXTOKENS];
	int ntoks = tokenizer(&tbuf, buf, n, (char **) &token, MAXTOKENS);

	if (ntoks <= 0) {
		error = ntoks;
		goto out;
	}

	if (strcmp(token[0],"destroy") == 0) {
		dpm_destroy_policy(policy->name);
	} else
		error = -EINVAL;

 out:
	if (tbuf)
		kfree(tbuf);
        return error ? error : n;
}

#define POLICY_STATE_ATTR(index) \
static ssize_t policy_state ## index ## _show(struct kobject * kobj, \
					      char * buf) \
{ \
	ssize_t len = 0; \
	struct dpm_policy *policy = to_policy(kobj); \
	len += sprintf(buf + len, "%s\n", policy->classopt[index].opt ? policy->classopt[index].opt->name :policy->classopt[index].class->name ); \
	return len; \
} \
static ssize_t policy_state ## index ## _store(struct kobject * kobj, \
					       const char * buf, \
			      size_t n) \
{ \
	struct dpm_policy *policy = to_policy(kobj); \
	struct dpm_classopt old_classopt; \
	int ret; \
 \
	dpm_lock(); \
	old_classopt = policy->classopt[index]; \
	if ((ret = dpm_map_policy_state(policy,index,(char *)buf))) \
		policy->classopt[index] = old_classopt; \
	dpm_unlock(); \
	return ret ? -EINVAL : n; \
} \
static struct dpm_policy_attribute policy_state ## index ## _attr = { \
        .attr   = { \
                .mode = 0644, \
        }, \
        .show   = policy_state ## index ## _show, \
        .store  = policy_state ## index ## _store, \
}; \

#define MAX_POLICY_STATES 20
POLICY_STATE_ATTR(0);
POLICY_STATE_ATTR(1);
POLICY_STATE_ATTR(2);
POLICY_STATE_ATTR(3);
POLICY_STATE_ATTR(4);
POLICY_STATE_ATTR(5);
POLICY_STATE_ATTR(6);
POLICY_STATE_ATTR(7);
POLICY_STATE_ATTR(8);
POLICY_STATE_ATTR(9);
POLICY_STATE_ATTR(10);
POLICY_STATE_ATTR(11);
POLICY_STATE_ATTR(12);
POLICY_STATE_ATTR(13);
POLICY_STATE_ATTR(14);
POLICY_STATE_ATTR(15);
POLICY_STATE_ATTR(16);
POLICY_STATE_ATTR(17);
POLICY_STATE_ATTR(18);
POLICY_STATE_ATTR(19);

static struct dpm_policy_attribute *policy_state_attr[MAX_POLICY_STATES] = {
	&policy_state0_attr,
	&policy_state1_attr,
	&policy_state2_attr,
	&policy_state3_attr,
	&policy_state4_attr,
	&policy_state5_attr,
	&policy_state6_attr,
	&policy_state7_attr,
	&policy_state8_attr,
	&policy_state9_attr,
	&policy_state10_attr,
	&policy_state11_attr,
	&policy_state12_attr,
	&policy_state13_attr,
	&policy_state14_attr,
	&policy_state15_attr,
	&policy_state16_attr,
	&policy_state17_attr,
	&policy_state18_attr,
	&policy_state19_attr,
};

static ssize_t
policy_attr_show(struct kobject * kobj, struct attribute * attr, char * buf)
{
	struct dpm_policy_attribute * policy_attr = to_policy_attr(attr);
	ssize_t ret = 0;

	if (policy_attr->show)
		ret = policy_attr->show(kobj,buf);
	return ret;
}

static ssize_t
policy_attr_store(struct kobject * kobj, struct attribute * attr,
		  const char * buf, size_t count)
{
	struct dpm_policy_attribute * policy_attr = to_policy_attr(attr);
	ssize_t ret = 0;

	if (policy_attr->store)
		ret = policy_attr->store(kobj,buf,count);
	return ret;
}

static struct dpm_policy_attribute a_policy_control_attr = {
        .attr   = {
                .name = "control",
                .mode = 0644,
        },
        .show   = a_policy_control_show,
        .store  = a_policy_control_store,
};

static struct sysfs_ops policy_sysfs_ops = {
	.show	= policy_attr_show,
	.store	= policy_attr_store,
};

static struct attribute * policy_default_attrs[] = {
	&a_policy_control_attr.attr,
	NULL,
};

static struct kobj_type ktype_policy = {
	.release        = dpm_kobj_release,
	.sysfs_ops	= &policy_sysfs_ops,
	.default_attrs	= policy_default_attrs,
};

void dpm_sysfs_new_policy(struct dpm_policy *policy)
{
	int i;

	memset(&policy->kobj, 0, sizeof(struct kobject));
	policy->kobj.kset = &dpm_subsys.kset,
	kobject_set_name(&policy->kobj,policy->name);
	policy->kobj.parent = &dpm_policy_kobj;
	policy->kobj.ktype = &ktype_policy;
	kobject_register(&policy->kobj);

	for (i = 0; (i < DPM_STATES) && (i < MAX_POLICY_STATES); i++) {
		policy_state_attr[i]->attr.name = dpm_state_names[i];
		sysfs_create_file(&policy->kobj, &policy_state_attr[i]->attr);
	}

	return;
}

void dpm_sysfs_destroy_policy(struct dpm_policy *policy)
{
	sysfs_remove_file(&policy->kobj, &policy_state_attr[0]->attr);
	kobject_unregister(&policy->kobj);
	return;
}

/*
 * class
 */

struct dpm_class_attribute {
        struct attribute        attr;
        ssize_t (*show)(struct kobject * kobj, char * buf);
        ssize_t (*store)(struct kobject * kobj, const char * buf, size_t count);
};

#define to_class(obj) container_of(obj,struct dpm_class,kobj)
#define to_class_attr(_attr) container_of(_attr,struct dpm_class_attribute,attr)

static ssize_t class_control_show(struct subsystem * subsys, char * buf)
{
	ssize_t len = 0;
	struct list_head  * p;

	len += sprintf(buf + len, "classes: ");

	list_for_each(p, &dpm_classes) {
		len += sprintf(buf + len, "%s ",
			       ((struct dpm_class *)
				list_entry(p, struct dpm_class, list))->name);
	}

	len += sprintf(buf + len, "\nactive: %s\n",
		       (dpm_enabled && dpm_active_class) ?
		       dpm_active_class->name : "[none]");
	return len;
}

static ssize_t class_control_store(struct subsystem * subsys, const char * buf,
				   size_t n)
{
	int error = 0;
	char *tbuf = NULL;
	char *token[MAXTOKENS];
	int ntoks = tokenizer(&tbuf, buf, n, (char **) &token, MAXTOKENS);

	if (ntoks <= 0) {
		error = ntoks;
		goto out;
	}

	if (strcmp(token[0],"create") == 0) {
		if (ntoks < 3)
			printk("dpm: class create requires 1 name and at least one operating point argument\n");
		else
			error = dpm_create_class(token[1], &token[2], ntoks-2);
	} else
		error = -EINVAL;

 out:
	if (tbuf)
		kfree(tbuf);
        return error ? error : n;
}

static struct kobject dpm_class_kobj = {
	.kset = &dpm_subsys.kset,
};

dpm_attr(control,class_control);

static ssize_t a_class_control_show(struct kobject * kobj, char * buf)
{
	ssize_t len = 0;
	struct dpm_class *class = to_class(kobj);
	int i;

	len += sprintf(buf + len, "ops: ");

	for (i = 0; i < class->nops; i++)
		len += sprintf(buf + len, "%s ", class->ops[i]->name);


	len += sprintf(buf + len, "\n");
	return len;
}

static ssize_t a_class_control_store(struct kobject * kobj, const char * buf,
				      size_t n)
{
	return n;
}

static ssize_t
class_attr_show(struct kobject * kobj, struct attribute * attr, char * buf)
{
	struct dpm_class_attribute * class_attr = to_class_attr(attr);
	ssize_t ret = 0;

	if (class_attr->show)
		ret = class_attr->show(kobj,buf);
	return ret;
}

static ssize_t
class_attr_store(struct kobject * kobj, struct attribute * attr,
		 const char * buf, size_t count)
{
	struct dpm_class_attribute * class_attr = to_class_attr(attr);
	ssize_t ret = 0;

	if (class_attr->store)
		ret = class_attr->store(kobj,buf,count);
	return ret;
}

static struct dpm_class_attribute a_class_control_attr = {
        .attr   = {
                .name = "control",
                .mode = 0644,
        },
        .show   = a_class_control_show,
        .store  = a_class_control_store,
};

static struct sysfs_ops class_sysfs_ops = {
	.show	= class_attr_show,
	.store	= class_attr_store,
};

static struct attribute * class_default_attrs[] = {
	&a_class_control_attr.attr,
	NULL,
};

static struct kobj_type ktype_class = {
	.release        = dpm_kobj_release,
	.sysfs_ops	= &class_sysfs_ops,
	.default_attrs	= class_default_attrs,
};

void dpm_sysfs_new_class(struct dpm_class *class)
{
	memset(&class->kobj, 0, sizeof(struct kobject));
	class->kobj.kset = &dpm_subsys.kset,
	kobject_set_name(&class->kobj,class->name);
	class->kobj.parent = &dpm_class_kobj;
	class->kobj.ktype = &ktype_class;
	kobject_register(&class->kobj);
	return;
}

void dpm_sysfs_destroy_class(struct dpm_class *class)
{
	kobject_unregister(&class->kobj);
	return;
}


/*
 * op
 */

struct dpm_op_attribute {
        struct attribute        attr;
        ssize_t (*show)(struct kobject * kobj, char * buf);
        ssize_t (*store)(struct kobject * kobj, const char * buf, size_t count);
};

#define to_op(obj) container_of(obj,struct dpm_opt,kobj)
#define to_op_attr(_attr) container_of(_attr,struct dpm_op_attribute,attr)

static ssize_t op_control_show(struct subsystem * subsys, char * buf)
{
	unsigned long flags;
	ssize_t len = 0;

	if (dpm_lock_interruptible())
		return -ERESTARTSYS;

	len += sprintf(buf + len, "active: ");

	if (!dpm_enabled) {
		len += sprintf(buf + len, "[none]\n");
	} else {
		spin_lock_irqsave(&dpm_policy_lock, flags);
		len += sprintf(buf + len,"%s\n",
			       dpm_active_opt ? dpm_active_opt->name : "[none]");
		spin_unlock_irqrestore(&dpm_policy_lock, flags);
	}

	dpm_unlock();

	len += sprintf(buf + len, "params: %d\n", DPM_PP_NBR);
	return len;
}

static ssize_t op_control_store(struct subsystem * subsys, const char * buf,
				size_t n)
{
	int error = 0;
	char *tbuf = NULL;
	char *token[MAXTOKENS];
	int ntoks = tokenizer(&tbuf, buf, n, (char **) &token, MAXTOKENS);

	if (ntoks <= 0) {
		error = ntoks;
		goto out;
	}

	if ((strcmp(token[0],"create") == 0) && (ntoks >= 2)) {
		dpm_md_pp_t pp[DPM_PP_NBR];
		int i;

		for (i = 0; i < DPM_PP_NBR; i++) {
			if (i >= ntoks - 2)
				pp[i] = -1;
			else
				pp[i] = simple_strtol(token[i + 2],
						      NULL, 0);
		}

		error = dpm_create_opt(token[1], pp, DPM_PP_NBR);
	} else
		error = -EINVAL;

 out:
	if (tbuf)
		kfree(tbuf);
        return error ? error : n;

}

dpm_attr(control,op_control);

#ifdef CONFIG_DPM_STATS
static ssize_t op_stats_show(struct subsystem * subsys, char * buf)
{
	int len = 0;
	struct dpm_opt *opt;
	struct list_head *p;
	unsigned long long total_time;

	if (dpm_lock_interruptible())
		return -ERESTARTSYS;

	if (!dpm_enabled) {
		dpm_unlock();
		len += sprintf(buf + len, "DPM IS DISABLED\n");
		return len;
	}

	for (p = dpm_opts.next; p != &dpm_opts; p = p->next) {
		opt = list_entry(p, struct dpm_opt, list);
		len += sprintf(buf + len, "op: %s", opt->name);
		total_time = opt->stats.total_time;
		if (opt == dpm_active_opt)
			total_time += (unsigned long long) dpm_time() -
				opt->stats.start_time;
		len += sprintf(buf + len, " ticks: %Lu times: %lu\n",
			       (unsigned long long) dpm_time_to_usec(total_time),
			       opt->stats.count);
	}

	dpm_unlock();
	return len;
}

static ssize_t op_stats_store(struct subsystem * subsys, const char * buf,
			      size_t n)
{
	return n;
}

dpm_attr(stats, op_stats);
#endif /* CONFIG_DPM_STATS */


static struct kobject dpm_op_kobj = {
	.kset = &dpm_subsys.kset,
};

static ssize_t an_op_control_show(struct kobject * kobj, char * buf)
{
	ssize_t len = 0;
	// struct dpm_opt *opt = to_op(kobj);

	len += sprintf(buf + len, "\n");
	return len;
}

static ssize_t an_op_control_store(struct kobject * kobj, const char * buf,
				   size_t n)
{
	return n;
}

static struct dpm_op_attribute an_op_control_attr = {
        .attr   = {
                .name = "control",
                .mode = 0644,
        },
        .show   = an_op_control_show,
        .store  = an_op_control_store,
};

static ssize_t op_force_show(struct kobject * kobj, char * buf)
{
	ssize_t len = 0;
	struct dpm_opt *opt = to_op(kobj);

	len += sprintf(buf + len, "%d\n", opt->flags & DPM_OP_FORCE ? 1 : 0);
	return len;
}

static ssize_t op_force_store(struct kobject * kobj, const char * buf,
			      size_t n)
{
	struct dpm_opt *opt = to_op(kobj);

	opt->flags = (opt->flags & ~DPM_OP_FORCE) |
		(simple_strtol(buf, NULL, 0) ? DPM_OP_FORCE : 0);
	return n;
}

static struct dpm_op_attribute op_force_attr = {
        .attr   = {
                .name = "force",
                .mode = 0644,
        },
        .show   = op_force_show,
        .store  = op_force_store,
};

#define OP_PARAM_ATTR(index) \
static ssize_t op_param ## index ## _show(struct kobject * kobj, char * buf) \
{ \
	ssize_t len = 0; \
	struct dpm_opt *opt = to_op(kobj); \
	len += sprintf(buf + len, "%d\n", opt->pp[index]); \
	return len; \
} \
static ssize_t op_param ## index ## _store(struct kobject * kobj, const char * buf, \
			      size_t n) \
{ \
	struct dpm_opt *opt = to_op(kobj); \
	int ret, oldval; \
 \
	oldval = opt->pp[index]; \
	opt->pp[index] = simple_strtol(buf, NULL, 0); \
	ret = dpm_md_init_opt(opt); \
	if (ret) \
		opt->pp[index] = oldval; \
	return ret ? ret : n; \
} \
static struct dpm_op_attribute op_param ## index ## _attr = { \
        .attr   = { \
                .mode = 0644, \
        }, \
        .show   = op_param ## index ## _show, \
        .store  = op_param ## index ## _store, \
}; \

#define MAX_OP_PARAMS 20
OP_PARAM_ATTR(0);
OP_PARAM_ATTR(1);
OP_PARAM_ATTR(2);
OP_PARAM_ATTR(3);
OP_PARAM_ATTR(4);
OP_PARAM_ATTR(5);
OP_PARAM_ATTR(6);
OP_PARAM_ATTR(7);
OP_PARAM_ATTR(8);
OP_PARAM_ATTR(9);
OP_PARAM_ATTR(10);
OP_PARAM_ATTR(11);
OP_PARAM_ATTR(12);
OP_PARAM_ATTR(13);
OP_PARAM_ATTR(14);
OP_PARAM_ATTR(15);
OP_PARAM_ATTR(16);
OP_PARAM_ATTR(17);
OP_PARAM_ATTR(18);
OP_PARAM_ATTR(19);

static struct dpm_op_attribute *op_param_attr[MAX_OP_PARAMS] = {
	&op_param0_attr,
	&op_param1_attr,
	&op_param2_attr,
	&op_param3_attr,
	&op_param4_attr,
	&op_param5_attr,
	&op_param6_attr,
	&op_param7_attr,
	&op_param8_attr,
	&op_param9_attr,
	&op_param10_attr,
	&op_param11_attr,
	&op_param12_attr,
	&op_param13_attr,
	&op_param14_attr,
	&op_param15_attr,
	&op_param16_attr,
	&op_param17_attr,
	&op_param18_attr,
	&op_param19_attr,
};

static ssize_t
op_attr_show(struct kobject * kobj, struct attribute * attr, char * buf)
{
	struct dpm_op_attribute * op_attr = to_op_attr(attr);
	ssize_t ret = 0;

	if (op_attr->show)
		ret = op_attr->show(kobj,buf);
	return ret;
}

static ssize_t
op_attr_store(struct kobject * kobj, struct attribute * attr,
	      const char * buf, size_t count)
{
	struct dpm_op_attribute * op_attr = to_op_attr(attr);
	ssize_t ret = 0;

	if (op_attr->store)
		ret = op_attr->store(kobj,buf,count);
	return ret;
}

static struct sysfs_ops op_sysfs_ops = {
	.show	= op_attr_show,
	.store	= op_attr_store,
};

static struct attribute * op_default_attrs[] = {
	&an_op_control_attr.attr,
	&op_force_attr.attr,
	NULL,
};

static struct kobj_type ktype_op = {
	.release        = dpm_kobj_release,
	.sysfs_ops	= &op_sysfs_ops,
	.default_attrs	= op_default_attrs,
};

void dpm_sysfs_new_op(struct dpm_opt *opt)
{
	int i;

	memset(&opt->kobj, 0, sizeof(struct kobject));
	opt->kobj.kset = &dpm_subsys.kset,
	kobject_set_name(&opt->kobj,opt->name);
	opt->kobj.parent = &dpm_op_kobj;
	opt->kobj.ktype = &ktype_op;
	kobject_register(&opt->kobj);

	for (i = 0; (i < DPM_PP_NBR) && (i < MAX_OP_PARAMS); i++) {
		op_param_attr[i]->attr.name = dpm_param_names[i];
		sysfs_create_file(&opt->kobj, &op_param_attr[i]->attr);
	}

	return;
}

void dpm_sysfs_destroy_op(struct dpm_opt *opt)
{
	sysfs_remove_file(&opt->kobj, &op_param_attr[0]->attr);
	kobject_unregister(&opt->kobj);
	return;
}


/*
 * state
 */


static ssize_t state_control_show(struct subsystem * subsys, char * buf)
{
	ssize_t len = 0;
	int i;

	len += sprintf(buf + len, "states: ");

	for (i = 0; i < DPM_STATES; i++) {
		len += sprintf(buf + len, "%s ", dpm_state_names[i]);
	}

	len += sprintf(buf + len, "\ntask-states: min=%s norm=%s max=%s\n",
		       dpm_state_names[DPM_TASK_STATE - DPM_TASK_STATE_LIMIT],
		       dpm_state_names[DPM_TASK_STATE],
		       dpm_state_names[DPM_TASK_STATE + DPM_TASK_STATE_LIMIT]);

	return len;
}

static ssize_t state_control_store(struct subsystem * subsys, const char * buf,
				   size_t n)
{
	return -EINVAL;
}

static ssize_t active_state_show(struct subsystem * subsys, char * buf)
{
	unsigned long flags;
	ssize_t len = 0;

	if (dpm_lock_interruptible())
		return -ERESTARTSYS;

	if (!dpm_enabled || (dpm_active_state == DPM_NO_STATE)) {
		len += sprintf(buf + len, "[none]\n");
	} else {
		spin_lock_irqsave(&dpm_policy_lock, flags);
		len += sprintf(buf + len,"%s\n",
			       dpm_state_names[dpm_active_state]);
		spin_unlock_irqrestore(&dpm_policy_lock, flags);
	}

	dpm_unlock();
	return len;
}

static ssize_t active_state_store(struct subsystem * subsys, const char * buf,
				  size_t n)
{
	int error = 0;
	char *tbuf = NULL;
	char *token[MAXTOKENS];
	int ntoks = tokenizer(&tbuf, buf, n, (char **) &token, MAXTOKENS);

	if (ntoks <= 0) {
		error = ntoks;
		goto out;
	}

	error = dpm_set_op_state(token[0]);

 out:
	if (tbuf)
		kfree(tbuf);
        return error ? error : n;
}

#ifdef CONFIG_DPM_STATS
static ssize_t state_stats_show(struct subsystem * subsys, char * buf)
{
	unsigned long flags;
	ssize_t len = 0;
	int i;

	spin_lock_irqsave(&dpm_policy_lock, flags);

	for (i = 0; i < DPM_STATES; i++) {
		unsigned long long total_time = dpm_state_stats[i].total_time;

		if (i == dpm_active_state)
			total_time += (unsigned long long) dpm_time() -
				dpm_state_stats[i].start_time;

		len += sprintf(buf + len, "state: %s", dpm_state_names[i]);
                len += sprintf(buf + len, " ticks: %Lu",
			       (unsigned long long) dpm_time_to_usec(total_time));
		len += sprintf(buf + len, " times: %lu\n",
			       dpm_state_stats[i].count);
	}

	spin_unlock_irqrestore(&dpm_policy_lock, flags);
	return len;
}

static ssize_t state_stats_store(struct subsystem * subsys, const char * buf,
				 size_t n)
{
        return n;
}
#endif /* CONFIG_DPM_STATS */

static struct kobject dpm_state_kobj = {
	.kset = &dpm_subsys.kset,
};

dpm_attr(control, state_control);
dpm_attr(active, active_state);
#ifdef CONFIG_DPM_STATS
dpm_attr(stats, state_stats);
#endif

struct astate {
	int index;
	struct kobject kobj;
};

struct astate_attribute {
        struct attribute        attr;
        ssize_t (*show)(struct kobject * kobj, char * buf);
        ssize_t (*store)(struct kobject * kobj, const char * buf, size_t count);
};

#define to_astate(obj) container_of(obj,struct astate,kobj)
#define to_astate_attr(_attr) container_of(_attr,struct astate_attribute,attr)

static ssize_t
astate_attr_show(struct kobject * kobj, struct attribute * attr, char * buf)
{
	struct astate_attribute * astate_attr = to_astate_attr(attr);
	ssize_t ret = 0;

	if (astate_attr->show)
		ret = astate_attr->show(kobj,buf);
	return ret;
}

static ssize_t
astate_attr_store(struct kobject * kobj, struct attribute * attr,
		  const char * buf, size_t count)
{
	struct astate_attribute * astate_attr = to_astate_attr(attr);
	ssize_t ret = 0;

	if (astate_attr->store)
		ret = astate_attr->store(kobj,buf,count);
	return ret;
}

static int show_opconstrains(int state, char *buf)
{
	struct dpm_opt *opt;
	int len = 0;

	if (dpm_active_policy->classopt[state].opt) {
		opt = dpm_active_policy->classopt[state].opt;

		len += dpm_show_opconstraints(opt, buf);
	}
	else {
		int i;

		for (i = 0;
		     i < dpm_active_policy->classopt[state].class->nops; i++) {
			len += dpm_show_opconstraints(
				dpm_active_policy->classopt[state].class->ops[i], buf);
		}
	}

	return len;
}
static ssize_t astate_constraints_show(struct kobject * kobj, char * buf)
{
	struct astate *astate = to_astate(kobj);
	ssize_t len = 0;

	if (dpm_enabled && dpm_active_policy)
		len = show_opconstrains(astate->index, buf);

	return len;
}

static ssize_t astate_constraints_store(struct kobject * kobj,
					const char * buf, size_t n)
{
	return n;
}

static struct astate_attribute astate_constraints_attr = {
        .attr   = {
                .name = "constraints",
                .mode = 0644,
        },
        .show   = astate_constraints_show,
        .store  = astate_constraints_store,
};

static struct sysfs_ops astate_sysfs_ops = {
	.show	= astate_attr_show,
	.store	= astate_attr_store,
};

static struct attribute * astate_default_attrs[] = {
	&astate_constraints_attr.attr,
	NULL,
};

static struct kobj_type ktype_astate = {
	.release        = dpm_kobj_release,
	.sysfs_ops	= &astate_sysfs_ops,
	.default_attrs	= astate_default_attrs,
};

static struct astate astate[DPM_STATES];

/*
 * Init
 */

static int __init dpm_sysfs_init(void)
{
        int error, i;

	error = subsystem_register(&dpm_subsys);
        if (!error)
                error = sysfs_create_group(&dpm_subsys.kset.kobj,&dpm_attr_group);
	if (!error) {
		kobject_set_name(&dpm_policy_kobj, "policy");
		kobject_register(&dpm_policy_kobj);
		sysfs_create_file(&dpm_policy_kobj, &policy_control_attr.attr);
		sysfs_create_file(&dpm_policy_kobj, &active_policy_attr.attr);
#ifdef CONFIG_DPM_STATS
		sysfs_create_file(&dpm_policy_kobj, &policy_stats_attr.attr);
#endif
		kobject_set_name(&dpm_class_kobj, "class");
		kobject_register(&dpm_class_kobj);
		sysfs_create_file(&dpm_class_kobj, &class_control_attr.attr);
		kobject_set_name(&dpm_op_kobj, "op");
		kobject_register(&dpm_op_kobj);
		sysfs_create_file(&dpm_op_kobj, &op_control_attr.attr);
#ifdef CONFIG_DPM_STATS
		sysfs_create_file(&dpm_op_kobj, &op_stats_attr.attr);
#endif
		kobject_set_name(&dpm_state_kobj, "state");
		kobject_register(&dpm_state_kobj);
		sysfs_create_file(&dpm_state_kobj, &state_control_attr.attr);
		sysfs_create_file(&dpm_state_kobj, &active_state_attr.attr);
#ifdef CONFIG_DPM_STATS
		sysfs_create_file(&dpm_state_kobj, &state_stats_attr.attr);
#endif

		for (i = 0; i < DPM_STATES; i++) {
			astate[i].index = i;
			astate[i].kobj.kset = &dpm_subsys.kset;
			kobject_set_name(&astate[i].kobj,dpm_state_names[i]);
			astate[i].kobj.parent = &dpm_state_kobj;
			astate[i].kobj.ktype = &ktype_astate;
			kobject_register(&astate[i].kobj);
		}
	}

        return error;
}

__initcall(dpm_sysfs_init);

/* /proc interface */

int dpm_set_task_state_by_name(struct task_struct *task, char *buf, ssize_t n)
{
	int task_state;
	int ret = 0;
	char *tbuf = NULL;
	char *token[MAXTOKENS];
	int ntoks = tokenizer(&tbuf, buf, n, (char **) &token, MAXTOKENS);

	if (ntoks <= 0) {
		ret = ntoks;
		goto out;
	}

	for (task_state = DPM_TASK_STATE - DPM_TASK_STATE_LIMIT;
	     task_state <= DPM_TASK_STATE + DPM_TASK_STATE_LIMIT;
	     task_state++)
		if (strcmp(token[0], dpm_state_names[task_state]) == 0) {
			task->dpm_state = task_state;

			if (task == current)
				dpm_set_os(task_state);

			ret = 0;
			break;
		}

out:
	if (tbuf)
		kfree(tbuf);

	return ret;
}
