#ifndef LINUX_APLOGGER_HOOK_H
#define LINUX_APLOGGER_HOOK_H

/* aplogger_hook.h
 *
 * Copyright (C) 2006-2007 Motorola, Inc.
 *
 */                                                  

/*
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
 */

/*************************************************************************
 * Revision History
 *  #   Date        Author          Comment
 * === ==========  ============    ============================
 *   1 10/03/2006   Motorola        Initial version. Ported from the Motorola
 *                                  EzX logger to work under Linux 2.6.
 *   10 05/10/2007  Motorola        Enabled logging on authorized secure units
 *      08/18/2007  Motorola        Add comments.
 *************************************************************************/

#ifdef __KERNEL__

/* Constants for enabling datalogger on secure units */
#define SCS1_ENABLE_LOGGING   0x81
#define SCS1_DISABLE_LOGGING  0x80

/* Type definitions for fork and exec log function pointers */
typedef void(*logger_exec_log_ptr_t)(char *, char __user * __user *,
        char __user * __user *);
typedef void(*logger_fork_log_ptr_t)(struct task_struct *, pid_t);
        
extern volatile logger_exec_log_ptr_t logger_exec_log_ptr;
extern volatile logger_fork_log_ptr_t logger_fork_log_ptr;

extern struct rw_semaphore logger_exec_ptr_lock;
extern rwlock_t logger_fork_ptr_lock;

extern int logger_hooksysexecve(logger_exec_log_ptr_t);
extern int logger_unhooksysexecve(void);
extern int logger_hooksysfork(logger_fork_log_ptr_t);
extern int logger_unhooksysfork(void);

extern void * logger_hooksysopen(void *);
extern int logger_unhooksysopen(void *);
extern int logger_syscall_getall(void **, void **, void **, void **);

#endif //ifdef __KERNEL__
#endif //ifndef LINUX_APLOGGER_HOOK_H
