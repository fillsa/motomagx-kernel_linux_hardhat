/*
 * lib/mothotplug_sysfs.c
 *
 * Sysfs interface to Motorola hotplug filtering 
 *
 * Copyright 2006 Motorola, Inc.
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
 *  12/2006  Motorola  Initial coding 
 *
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/module.h>

#define mothotplug_subsys_attr(_prefix, _name, _mode) \
static struct mothotplug_attribute _prefix##_attr = \
__ATTR(_name, _mode, _prefix##_show, _prefix##_store) 

extern int mothotplug_add_filters(const char * filters);
extern ssize_t mothotplug_get_filters(char * buff, ssize_t size);

/**
 * Attribute structure to include show and store functions
 * for easy reference
 */

struct mothotplug_attribute {
    struct attribute attr;
    ssize_t(*show)  (struct kobject *, char *);
    ssize_t(*store) (struct kobject *, const char *, size_t);
};


/**
 * Generic store op for all attributes, calls the store function of the
 * passed attribute
 */

static ssize_t
mothotplug_attr_store(struct kobject *kobj, struct attribute *attr,
					  const char *buf, size_t len)
{
    struct mothotplug_attribute *attribute =
        container_of(attr, struct mothotplug_attribute, attr);
    return (attribute->store ? attribute->store(kobj, buf, len) : 0);
}

/**
 * Generic show op for all attributes, calls the show function of the
 * passed attribute
 */
static ssize_t
mothotplug_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct mothotplug_attribute *attribute =
        container_of(attr, struct mothotplug_attribute, attr);
    return (attribute->show ? attribute->show(kobj, buf) : 0);
}

/**
 * Function pointers for generic show and store ops
 */
struct sysfs_ops mothotplug_sysfs_ops = {
    .show  = mothotplug_attr_show,
    .store = mothotplug_attr_store,
};

/**
 * Ktype for hotplug kobjects
 */
static struct kobj_type mothotplug_ktype = {
    .sysfs_ops = &mothotplug_sysfs_ops
};

/**
 * Declare the hotplug subsystem: /sys/hotplug
 */
decl_subsys(mothotplug, &mothotplug_ktype, NULL);


/**
 * filter_list hotplug attribute
 *
 * store:
 * receive the list of filters for the hotplug events that
 * will result in an exec of the hotplug script.
 * (usually /sbin/hotplug)
 * pass in the filters in the following format:
 * "filtername\nfiltername\nfiltername\n"
 *
 * this store function takes an arbitrary number of filter
 * names up to a kernel page in bytes.
 *
 * show:
 * write current filters that are in place
 *
 */

static ssize_t
mothotplug_filter_list_store(struct kobject *kobj, const char *buff, 
							 size_t count)
{
	if (count > PAGE_SIZE)
		return -EINVAL;

	return mothotplug_add_filters(buff);
}

static ssize_t
mothotplug_filter_list_show(struct kobject *kobj, char *buff)
{
    return mothotplug_get_filters(buff, PAGE_SIZE);
}

/* Create the filter_list attribute */
mothotplug_subsys_attr(mothotplug_filter_list, filter_list, 
					   (S_IRUGO | S_IWUSR));


/*
 * End of filter_list attribute functions
 */ 



/* Init function for hotplug filter sysfs */
int
mothotplug_sysfs_init(void)
{
    int rc = 0;
    /* Register hotplug subsystem */
    if((rc = subsystem_register(&mothotplug_subsys))) {
        printk(KERN_WARNING "Error [%d] registering Motorola hotplug filtering"
                            "subsystem.\n", rc);
        return rc;
    }
    /* Add filter list file under hotplug subsystem */
    if((rc = sysfs_create_file(&mothotplug_subsys.kset.kobj,
                               &mothotplug_filter_list_attr.attr))) {
        printk(KERN_WARNING "Error [%d] creating filter_list attribute.\n",rc);
        return rc;
    }

    printk(KERN_INFO "Motorola hotplug filtering sysfs subsytem initialized.\n");

    return rc;
}

void
mothotplug_sysfs_exit(void) 
{
    sysfs_remove_file(&mothotplug_subsys.kset.kobj,
                      &mothotplug_filter_list_attr.attr);
    subsystem_unregister(&mothotplug_subsys);
}

module_init(mothotplug_sysfs_init);
module_exit(mothotplug_sysfs_exit);

MODULE_AUTHOR("Motorola, Inc.");
MODULE_DESCRIPTION("Motorola Hotplug Filter Module Sysfs Interface");
MODULE_LICENSE("GPL");
