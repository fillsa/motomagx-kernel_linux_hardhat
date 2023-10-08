/*
 * kernel/module_sysfs.c
 *
 * Sysfs interface to Motorola module hashing
 *
 * Copyright 2007 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  Date     Author    Comment
 *  3/2007  Motorola  Initial coding 
 *
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#undef DEBUGHASHES

#ifdef DEBUGHASHES
#define motmodule_hash_subsys_attr(_prefix, _name, _mode) \
static struct motmodule_hash_attribute _prefix##_attr = \
__ATTR(_name, _mode, _prefix##_show, _prefix##_store)
#else
#define motmodule_hash_subsys_attr(_prefix, _name, _mode) \
static struct motmodule_hash_attribute _prefix##_attr = \
__ATTR(_name, _mode, NULL, _prefix##_store)
#endif

extern int motmodule_hash_add_hashes(const char *hashes);
#ifdef DEBUGHASHES
extern int motmodule_hash_get_hashes(const char *buff, size_t buff_size);
#endif

/**
 * Attribute structure to include show and store functions
 * for easy reference
 */

struct motmodule_hash_attribute {
	struct attribute attr;
	ssize_t(*show) (struct kobject *, char *);
	ssize_t(*store) (struct kobject *, const char *, size_t);
};


/**
 * Generic store op for all attributes, calls the store function of the
 * passed attribute
 */

static ssize_t
motmodule_hash_attr_store(struct kobject *kobj, struct attribute *attr,
			  const char *buf, size_t len)
{
	struct motmodule_hash_attribute *attribute =
	    container_of(attr, struct motmodule_hash_attribute, attr);
	return (attribute->store ? attribute->store(kobj, buf, len) : 0);
}

#ifdef DEBUGHASHES
/**
 * Generic show op for all attributes, calls the show function of the
 * passed attribute
 */
static ssize_t
motmodule_hash_attr_show(struct kobject *kobj, struct attribute *attr,
			 char *buf)
{
	struct motmodule_hash_attribute *attribute =
	    container_of(attr, struct motmodule_hash_attribute, attr);
	return (attribute->show ? attribute->show(kobj, buf) : 0);
}
#endif

static int
motmodule_hash_hotplug_filter(struct kset *kset, struct kobject *kobj)
{
	return 0;
}

/**
 * Function pointers for generic show and store ops
 */
struct sysfs_ops motmodule_hash_sysfs_ops = {
#ifdef DEBUGHASHES
	.show = motmodule_hash_attr_show,
#else
	.show = NULL,
#endif
	.store = motmodule_hash_attr_store,
};

/**
 * Ktype for module_hash kobjects
 */
static struct kobj_type motmodule_hash_ktype = {
	.sysfs_ops = &motmodule_hash_sysfs_ops
};

struct kset_hotplug_ops motmodule_hash_hotplug_ops = {
	.filter = motmodule_hash_hotplug_filter,
	.name = NULL,
	.hotplug = NULL,
};

/**
 * Declare the module hash subsystem: /sys/motmodule_hash
 */
decl_subsys(motmodule_hash, &motmodule_hash_ktype,
	    &motmodule_hash_hotplug_ops);


/**
 * hash_list module attribute
 *
 * store:
 * receive the list of hashes for the modules
 * on the system.
 * pass in the hashes in the following format:
 * "hash hash hash\n"
 *
 * this store function takes an arbitrary number of hashes
 * up to a kernel page in bytes.
 *
 * show:
 * write current hashes that are in place
 *
 */

static ssize_t
motmodule_hash_hash_list_store(struct kobject *kobj, const char *buff,
			       size_t count)
{
	char *tmp;
	int len, i, buff_start, ret;

	if (count > PAGE_SIZE || count == 0)
		return -EINVAL;

	/* add two to the len if we don't have a trailing \n 
	   since we need a double null */
	len = count + (buff[count - 1] == '\n' ? 1 : 2);

	tmp = kmalloc(sizeof(char) * len, GFP_KERNEL);

	if (!tmp)
		return -ENOMEM;

	for (i = 0, buff_start = 0; i < count; ++i) {
		if (buff[i] == ' ' || buff[i] == '\n') {
			if (i - buff_start != 40) {
				/* hashes must be 40 characters */
				motmodule_hash_add_hashes(NULL);
				return -EINVAL;
			} else {
				tmp[i] = '\0';
				buff_start = i + 1;
			}
		} else
			tmp[i] = buff[i];
	}

	/* we are using a double-null to indicate the end of hashes */
	if (i > 0 && tmp[i - 1] != '\0')
		tmp[++i] = '\0';

	ret = motmodule_hash_add_hashes(tmp);

	if (ret != 0) {
		kfree(tmp);
	}
	return ret == 0 ? count : ret;
}

#ifdef DEBUGHASHES
static ssize_t
motmodule_hash_hash_list_show(struct kobject *kobj, char *buff)
{
	return motmodule_hash_get_hashes(buff, PAGE_SIZE);
}
#endif

/* Create the hash_list attribute */
motmodule_hash_subsys_attr(motmodule_hash_hash_list, hash_list,
#ifdef DEBUGHASHES
			   (S_IRUGO | S_IWUSR));
#else
			   S_IWUSR);
#endif


/*
 * End of hash_list attribute functions
 */



/* Init function for module hash sysfs */
int __init motmodule_hash_sysfs_init(void)
{
	int rc = 0;

	/* Register module hash subsystem */
	if ((rc = subsystem_register(&motmodule_hash_subsys))) {
		printk(KERN_WARNING
		       "Error [%d] registering Motorola module hash "
		       "subsystem.\n", rc);
		motmodule_hash_add_hashes(NULL);

		return rc;
	}
	/* Add hash list file under module subsystem */
	if ((rc = sysfs_create_file(&motmodule_hash_subsys.kset.kobj,
				    &motmodule_hash_hash_list_attr.
				    attr))) {
		printk(KERN_WARNING
		       "Error [%d] creating hash list attribute.\n", rc);
		motmodule_hash_add_hashes(NULL);

		return rc;
	}

	printk(KERN_INFO
	       "Motorola module hash sysfs subsytem initialized.\n");

	return rc;
}

void __exit motmodule_hash_sysfs_exit(void)
{
	sysfs_remove_file(&motmodule_hash_subsys.kset.kobj,
			  &motmodule_hash_hash_list_attr.attr);
	subsystem_unregister(&motmodule_hash_subsys);
}

module_init(motmodule_hash_sysfs_init);
module_exit(motmodule_hash_sysfs_exit);

MODULE_AUTHOR("Motorola, Inc.");
MODULE_DESCRIPTION("Motorola Module Hash Sysfs Interface");
MODULE_LICENSE("GPL");
