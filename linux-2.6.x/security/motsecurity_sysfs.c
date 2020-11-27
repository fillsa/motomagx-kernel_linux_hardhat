/*
 * security/motsecurity_sysfs.c
 *
 * Sysfs interface to Motorola security features 
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
 *  10/2006  Motorola  Added attributes for secure mount features 
 *
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/module.h>

#define motsec_subsys_attr(_prefix, _name, _mode) \
static struct motsec_attribute _prefix##_attr = \
__ATTR(_name, _mode, _prefix##_show, _prefix##_store) 

extern int motsec_mounts_locked(void);
extern void motsec_lock_mounts(void);

extern int motsec_add_allowed_mount(char * dev, char * mount, char * fstype);
extern ssize_t motsec_get_allowed_mounts(char * buff, ssize_t size);

/**
 * Attribute structure to include show and store functions
 * for easy reference
 */

struct motsec_attribute {
    struct attribute attr;
    ssize_t(*show)  (struct kobject *, char *);
    ssize_t(*store) (struct kobject *, const char *, size_t);
};


/**
 * Generic store op for all attributes, calls the store function of the
 * passed attribute
 */

static ssize_t
motsec_attr_store(struct kobject *kobj, struct attribute *attr,
                  const char *buf, size_t len)
{
    struct motsec_attribute *attribute =
        container_of(attr, struct motsec_attribute, attr);
    return (attribute->store ? attribute->store(kobj, buf, len) : 0);
}

/**
 * Generic show op for all attributes, calls the show function of the
 * passed attribute
 */
static ssize_t
motsec_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct motsec_attribute *attribute =
        container_of(attr, struct motsec_attribute, attr);
    return (attribute->show ? attribute->show(kobj, buf) : 0);
}

/**
 * Function pointers for generic show and store ops
 */
struct sysfs_ops motsec_sysfs_ops = {
    .show  = motsec_attr_show,
    .store = motsec_attr_store,
};

/**
 * Ktype for security kobjects
 */
static struct kobj_type motsec_ktype = {
    .sysfs_ops = &motsec_sysfs_ops
};

/**
 * Declare the security subsystem: /sys/security
 */
decl_subsys(security, &motsec_ktype, NULL);


/**
 * mounts_locked security attribute
 *
 * store:
 * If 1 is recieved, only mounts previously read by the mounts_allowed
 * attribute are allowed to be mounted. All other inputs have no effect.
 *
 * If the mounts_locked flag has been set to 1, it cannot be 
 * reverted to 0
 *
 * show:
 * write current state of mounts_locked, 1 or 0
 *
 */

static ssize_t
motsec_mounts_locked_store(struct kobject *kobj, const char *buff, 
                           size_t count)
{
    /* Verify that we are receiving 1 */
    if(count != 2 || (buff[1] != '\n') 
                  || (buff[0] != '1')) {
        return -EINVAL;
    }
    /* Set the mounts_locked flag to 1 */
    motsec_lock_mounts();

    /* Return that we read all of the data */
    return count;
}

static ssize_t
motsec_mounts_locked_show(struct kobject *kobj, char *buff)
{
    return snprintf(buff, PAGE_SIZE, "%d\n", motsec_mounts_locked());
}

/* Create the mounts_locked attribute */
motsec_subsys_attr(motsec_mounts_locked, mounts_locked, 
                        (S_IRUGO | S_IWUSR));


/*
 * End of mounts_locked attribute functions
 */ 

/**
 * allowed_mounts security attribute
 *
 * store:
 * receive the mounts that will be allowed after mounts_locked is
 * set to true in the following format:
 * "device mount_point filesystem_type\n"
 *
 * If the mounts_locked flag has been set to 1, store will have no
 * effect
 *
 * show:
 * write current mounts that are allowed
 *
 */

static ssize_t
motsec_allowed_mounts_store(struct kobject *kobj, const char *buff, 
                            size_t count)
{
    int i, size, rc;
    char *pos;
    char * f_end[3];
    char * field[3];
    char * f_start[3] = { 0, 0, 0 };
    
    for(i = 0, pos = (char *)buff; 
        i < 3 && (size_t)(pos - buff) < count;
        pos++) {
        if (*pos == ' ' || *pos == '\n' || *pos == '\0') {
            if(!f_start[i]) {
                rc = -EINVAL;
                goto err;
            }
            else {
                f_end[i++] = pos;
                if (*pos == '\n') {
                    pos++;
                    break;
                }
            }
        }
        else {
            if (!f_start[i])
                f_start[i] = pos;
        }
    }
    /* Error if we did not receive 3 fields */
    if(i != 3) {
        rc = -EINVAL;
        goto err;
    }

    /* At this point we have good input, malloc and store the fields */
    for(i = 0; i < 3; i++) {
        size = f_end[i] - f_start[i];
        field[i] = kmalloc(size + 1, GFP_KERNEL);
        if(!field[i]) {
            rc = -ENOMEM;
            goto err_free;
        }
        memcpy(field[i], f_start[i], size);
        field[i][size] = '\0'; /* NULL terminate the string */
    }
    /* pass the data to add to the list of allowed mounts */
    rc = motsec_add_allowed_mount(field[0], field[1], field[2]);
    if(!rc)
        return pos - buff;
    
err_free:
    while(i)
        kfree(field[--i]);
err:
    if(rc == -EINVAL)
        printk(KERN_WARNING "Invalid mount format, could not "
                             "add allowed mount\n");
    return rc; 
}


static ssize_t
motsec_allowed_mounts_show(struct kobject *kobj, char *buff)
{
    return motsec_get_allowed_mounts(buff, PAGE_SIZE);
}

/* Create the mounts_locked attribute */
motsec_subsys_attr(motsec_allowed_mounts, allowed_mounts, 
                        (S_IRUGO | S_IWUSR));


/*
 * End of allowed_mounts attribute functions
 */ 



/* Init function for security sysfs */
int
motsec_sysfs_init(void)
{
    int rc = 0;
    /* Register security subsystem */
    if((rc = subsystem_register(&security_subsys))) {
        printk(KERN_WARNING "Error [%d] registering Motorola Security"
                            "subsystem.\n", rc);
        return rc;
    }
    /* Add mounts_locked files under security subsystem */
    if((rc = sysfs_create_file(&security_subsys.kset.kobj,
                               &motsec_mounts_locked_attr.attr))) {
        printk(KERN_WARNING "Error [%d] creating mounts_locked attribute.\n",
                            rc);
        return rc;
    }
    /* Add allowed_mounts files under security subsystem */
    if((rc = sysfs_create_file(&security_subsys.kset.kobj,
                               &motsec_allowed_mounts_attr.attr))) {
        printk(KERN_WARNING "Error [%d] creating allowed_mounts attribute.\n",
                            rc);
        return rc;
    }

    printk(KERN_INFO "Motorola security sysfs subsytem initialized.\n");

    return rc;
}

void
motsec_sysfs_exit(void) 
{
    sysfs_remove_file(&security_subsys.kset.kobj,
                      &motsec_mounts_locked_attr.attr);
    sysfs_remove_file(&security_subsys.kset.kobj,
                      &motsec_allowed_mounts_attr.attr);
    subsystem_unregister(&security_subsys);
}

module_init(motsec_sysfs_init);
module_exit(motsec_sysfs_exit);

MODULE_AUTHOR("Motorola, Inc.");
MODULE_DESCRIPTION("Motorola Security Module Sysfs Interface");
MODULE_LICENSE("GPL");
