/*
 * Copyright (C) 2008 Motorola, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  -------------------------------------
 * 03/11/2008   Motorola  Initial version
 * 
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/kallsyms.h>
#include <linux/rtc.h>
#include <linux/mem-log.h>

#define DBG_LOG_SIZE 256 

/* struct used to record different event
 * @when	- time tick
 * @pid		- current process pid 
 * @comm	- current process name
 * @prio	- current process priority
 */
struct dbg_log_struct{
    unsigned long when;
    pid_t pid;
    char comm[16];
    int prio;
};

struct dbg_log_struct dbg_log[DBG_LOG_SIZE];

static unsigned int dbg_log_at = 0;
static unsigned int stop_logging = 0;
static spinlock_t lock = SPIN_LOCK_UNLOCKED;

/* mem_log_event() used to record correspoding value depends on
 * different caller
 */
void mem_log_event(void) {
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);
    if(!stop_logging) {
        dbg_log[dbg_log_at].when = rtc_sw_msfromboot();
        dbg_log[dbg_log_at].pid = current->pid;
	dbg_log[dbg_log_at].prio = current->prio;
	memcpy(dbg_log[dbg_log_at].comm, current->comm, sizeof(current->comm));
	dbg_log_at++;
        if(dbg_log_at >= DBG_LOG_SIZE) {
            dbg_log_at = 0; 
        }
    }
    spin_unlock_irqrestore(&lock, flags);
}
EXPORT_SYMBOL(mem_log_event);

/*  print_single_log() prints single log according to differenet
 *  call
 */
static void print_single_log(unsigned int i) {

    printk(KERN_CRIT "%lu:%d:%s:%d\n", dbg_log[i].when, dbg_log[i].pid, dbg_log[i].comm, dbg_log[i].prio);
}

/* mem_print_log() used to print out event log in die() in traps.c
 */
void mem_print_log(void) {
    unsigned int i;
    unsigned long flags;
    spin_lock_irqsave(&lock, flags);

    printk(KERN_CRIT "Timestamp:PID:Comm:Prio\n");
    
    for(i = dbg_log_at; i < DBG_LOG_SIZE; i++) {
        print_single_log(i);
    }

    for(i = 0; i < dbg_log_at; i++) {
        print_single_log(i);
    }

    printk(KERN_CRIT "Current timestamp is: %lu\n", rtc_sw_msfromboot());
    
    spin_unlock_irqrestore(&lock, flags);
}
EXPORT_SYMBOL(mem_print_log);





