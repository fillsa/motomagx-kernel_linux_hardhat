/*
 * Copyright (C) 2005,2007 Motorola Inc.
 */

/* ChangeLog:
 * (mm-dd-yyyy) Author    Comment
 * 02-25-2005   Motorola  Integrate Freescale MXC Drop
 * 08-20-2007   Motorola  Support DM500
 */
 
#ifndef _LINUX_CDEV_H
#define _LINUX_CDEV_H
#ifdef __KERNEL__

#include <linux/kobject.h>
#include <linux/kdev_t.h>
#include <linux/list.h>

struct file_operations;
struct inode;
struct module;

struct cdev {
	struct kobject kobj;
	struct module *owner;
	struct file_operations *ops;
	struct list_head list;
	dev_t dev;
	unsigned int count;
};

void cdev_init(struct cdev *, struct file_operations *);

struct cdev *cdev_alloc(void);

void cdev_put(struct cdev *p);

int cdev_add(struct cdev *, dev_t, unsigned);

void cdev_del(struct cdev *);

void cd_forget(struct inode *);

#endif
#endif
