/*
 * FILE NAME include/linux/oom.h
 *
 * BRIEF MODULE DESCRIPTION
 * header file for user directed  oom kill functions
 *
 * Author: David Singleton dsingleton@mvista.com MontaVista Software, Inc.
 *
 * 2001-2004 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef _LINUX_OOM_H
#define _LINUX_OOM_H

#define PROC_NAME_SIZE 80

typedef struct oom_procs {
        struct oom_procs *next;
	int len;
        char name[PROC_NAME_SIZE];
} oom_procs_t;

extern struct oom_procs oom_killable;
extern  struct oom_procs oom_dontkill;

extern int dontkill_read_proc(char *, char **, off_t, int, int *, void *);
extern int killable_read_proc(char *, char **, off_t, int, int *, void *);

extern ssize_t dontkill_write_proc(struct file *, const char *, size_t,
    loff_t *);
extern ssize_t killable_write_proc(struct file *, const char *, size_t,
    loff_t *);
#endif /* _LINUX_OOM_H */
