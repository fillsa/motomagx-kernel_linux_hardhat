/* aplogger_hook - Hooks to allow manipulation of various system calls
 *
 * Copyright (C) 2006, 2007 Motorola, Inc.
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
 *  Revision History
 *  #   Date        Author          Comment
 * === ==========  ============    ============================
 *   1 10/03/2006   Motorola        Initial version. Based on EzX logger but
 *                                  ported to work under the 2.6 Linux kernel.
 *   2 05/10/2007   Motorola        Enabled logging on authorized secure units.
 *
 *************************************************************************/

#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/interrupt.h>
#include <asm/unistd.h>
#include <linux/module.h>
#include <linux/aplogger_hook.h>
#include <linux/spinlock.h>

#ifdef CONFIG_MOT_FEAT_OTP_EFUSE
#include <linux/ks_otp_api.h>
#endif /* CONFIG_MOT_FEAT_OTP_EFUSE */

extern void * sys_call_table[];

spinlock_t logger_syscalltable_lock = SPIN_LOCK_UNLOCKED;

volatile logger_exec_log_ptr_t logger_exec_log_ptr = NULL;
volatile logger_fork_log_ptr_t logger_fork_log_ptr = NULL;
DECLARE_RWSEM(logger_exec_ptr_lock);
rwlock_t logger_fork_ptr_lock = RW_LOCK_UNLOCKED;

#ifdef CONFIG_MOT_FEAT_OTP_EFUSE
/* Security variables */
static volatile unsigned char logger_mot_security_status = 0;
static spinlock_t logger_mot_security_lock = SPIN_LOCK_UNLOCKED;

#define LOGGER_MOT_SECURITY_SET             1
#define LOGGER_MOT_SECURITY_PASSED          2
#define LOGGER_MOT_SECURITY_ERRORED         4
#endif /* CONFIG_MOT_FEAT_OTP_EFUSE */

/* Debug defines */
#define LOGGER_HOOK_DEBUG   0

/*
 * pass_mot_security_check()
 *
 * Description: This performs a check on the security mode in which we're
 * operating in. If we are in engineering or no security mode we are ok. If we
 * are in production mode, then don't allow anything to happen.
 *
 * Return: 1 for continue running, 0 for stop execution
 */

static inline int pass_mot_security_check()
{
#ifdef CONFIG_MOT_FEAT_OTP_EFUSE
    int logger_mot_security_err = 0;
    unsigned char logger_mot_security_mode = MOT_SECURITY_MODE_PRODUCTION;
    unsigned char logger_production_state = LAUNCH_SHIP;
    unsigned char logger_scs1_value = SCS1_DISABLE_LOGGING;

    /* Multi-thread protection */
    spin_lock(&logger_mot_security_lock);
    /* Check if the mode has been set */
    if (!(logger_mot_security_status & LOGGER_MOT_SECURITY_SET))
    {

retry_mode:
        /* Security has not been checked, so check it */
        mot_otp_get_mode(&logger_mot_security_mode, &logger_mot_security_err);
        if (logger_mot_security_err != 0)
        {
            /* If we were interrupted try again */
            if (logger_mot_security_err == -EINTR)
            {
                logger_mot_security_err = 0;
                goto retry_mode;
            }
            /* Otherwise fail */
            else
                logger_mot_security_status |= LOGGER_MOT_SECURITY_ERRORED;
        }
        
        /* Update whether the check passed or failed */
        if (logger_mot_security_mode == MOT_SECURITY_MODE_ENGINEERING ||
                logger_mot_security_mode == MOT_SECURITY_MODE_NO_SECURITY)
            logger_mot_security_status |= LOGGER_MOT_SECURITY_PASSED;
        else {

retry_product_state:
            mot_otp_get_productstate(&logger_production_state, &logger_mot_security_err);
            if (logger_mot_security_err != 0)
            {
                if (logger_mot_security_err == -EINTR)
                {
                    logger_mot_security_err = 0;
                    goto retry_product_state;
                }
                else
                    logger_mot_security_status |= LOGGER_MOT_SECURITY_ERRORED;
            }

            /* Update whether the check passed or failed */
            if (logger_production_state == PRE_ACCEPTANCE_ACCEPTANCE)
                logger_mot_security_status |= LOGGER_MOT_SECURITY_PASSED;
            else {

retry_scs1:
                 mot_otp_get_scs1(&logger_scs1_value, &logger_mot_security_err);
                 if (logger_mot_security_err != 0)
                 {
                     if (logger_mot_security_err == -EINTR)
                     {
                         logger_mot_security_err = 0;
                         goto retry_scs1;
                     }
                     else
                        logger_mot_security_status |= LOGGER_MOT_SECURITY_ERRORED;
                 }

                 /* Update whether the check passed or failed */
                 if ((logger_scs1_value & SCS1_ENABLE_LOGGING) == SCS1_ENABLE_LOGGING)
                     logger_mot_security_status |= LOGGER_MOT_SECURITY_PASSED;
            } /* SCS1 check */
        } /* product state check */

        /* We've done a sucessful check, update the status */
        logger_mot_security_status |= LOGGER_MOT_SECURITY_SET;
    }

    /* If we passed, return one */
    if (logger_mot_security_status & LOGGER_MOT_SECURITY_PASSED)
    {
        spin_unlock(&logger_mot_security_lock);
        return 1;
    }

    /* If we failed security check, return zero (below the #endif) */
    spin_unlock(&logger_mot_security_lock);
#endif /* CONFIG_MOT_FEAT_OTP_EFUSE */

    /* If OTP security is not compiled in currently, always fail */
    return 0;
}

/* 
 * logger_syscall_get
 * This function retrieves the address of the system call passed in
 * 
 * Return: pointer to system call function; NULL on fail
 * 
 * @call_in = system call id to retreive
 */
static void * logger_syscall_get(int call_in)
{
    if (!pass_mot_security_check())
        return NULL;
    
    if (call_in > __NR_SYSCALL_BASE + NR_syscalls || call_in < __NR_SYSCALL_BASE){
        return NULL; /* if syscall entry is invalid */
    }
    else {
        return sys_call_table[call_in - __NR_SYSCALL_BASE];
    }
}


/*
 * logger_syscall_set
 * This function replaces the system call supplied in the table, with the
 * address of the call supplied.
 *
 * Return: The original address of the call; NULL on fail
 * 
 * @call_in = system call to replace;
 * @newcall = address of function to use to replace system call
 */
static void * logger_syscall_set(int call_in, void * newcall)
{
    void * orig_addr = NULL;
    unsigned long flags = 0;

    if (!pass_mot_security_check())
        return NULL;
    
    if (newcall == NULL)
        return NULL; /* if address is invalid */

    else { /* replace the call */
        orig_addr = logger_syscall_get(call_in);
        if (orig_addr == NULL)
            return orig_addr;
        spin_lock_irqsave(&logger_syscalltable_lock, flags);
        sys_call_table[call_in - __NR_SYSCALL_BASE] = newcall;
        spin_unlock_irqrestore(&logger_syscalltable_lock, flags);
        return orig_addr;
    }
}


/*
 * logger_hooksysexecve
 *
 * This function sets the kernel pointer to the applicable execve logging
 * function within the AP logger.
 * 
 * Return: (int) 0 on success, -1 on fail
 * 
 * @new_log_ptr = address of the new aplogger execve logging function
 */
int logger_hooksysexecve(logger_exec_log_ptr_t new_log_ptr)
{
    if (!pass_mot_security_check())
        return -1;
    
    if (new_log_ptr == NULL)
    {
        printk("APLOGGER: Warning! Tried to pass in NULL exec log pointer\n");
        return -1;
    }
    
    if (logger_exec_log_ptr != NULL && logger_exec_log_ptr != new_log_ptr)
        printk("APLOGGER: Warning! Stomping on old exec log pointer\n");
    
    down_write(&logger_exec_ptr_lock);
    logger_exec_log_ptr = new_log_ptr;
    up_write(&logger_exec_ptr_lock);
    return 0;
}


/*
 * logger_unhooksysexecve
 *
 * This function resets the kernel pointer for execve logging to NULL.
 *
 * Return: (int) 0 on success, -1 on fail
 */
int logger_unhooksysexecve()
{
    if (!pass_mot_security_check())
        return -1;
    
#if LOGGER_HOOK_DEBUG
    if (logger_exec_log_ptr == NULL)
        printk("APLOGGER: Warning! Unhooking a non-hooked exec pointer\n");
#endif /* LOGGER_HOOK_DEBUG */
    down_write(&logger_exec_ptr_lock);
    logger_exec_log_ptr = NULL;
    up_write(&logger_exec_ptr_lock);
    return 0;
}


/*
 * logger_hooksysfork
 *
 * This function sets the pointer to the AP logger fork logging function.
 *
 * Return: (int) 0 on sucess, -1 on fail
 * 
 * @new_log_ptr = AP logger address of fork logging function
 */
int logger_hooksysfork(logger_fork_log_ptr_t new_log_ptr)
{
    unsigned long irq_flags = 0;
    
    if (!pass_mot_security_check())
        return -1;
    
    if (new_log_ptr == NULL)
    {
        printk("APLOGGER: Warning! Tried to pass in NULL fork log pointer\n");
        return -1;
    }
    
    if (logger_fork_log_ptr != NULL && logger_fork_log_ptr != new_log_ptr)
        printk("APLOGGER: Warning! Stomping on old fork log pointer\n");

    write_lock_irqsave(&logger_fork_ptr_lock, irq_flags);
    logger_fork_log_ptr = new_log_ptr;
    write_unlock_irqrestore(&logger_fork_ptr_lock, irq_flags);
    return 0;
}


/*
 * logger_unhooksysfork
 *
 * This function resets the function pointer for fork logging to NULL.
 * 
 * Return: (int) 0 on sucess; -1 on fail
 */
int logger_unhooksysfork()
{
    unsigned long irq_flags = 0;
    
    if (!pass_mot_security_check())
        return -1;
    
#if LOGGER_HOOK_DEBUG
    if (logger_fork_log_ptr == NULL)
        printk("APLOGGER: Warning! Unhooking a non-hooked fork pointer\n");
#endif /* LOGGER_HOOK_DEBUG */
    write_lock_irqsave(&logger_fork_ptr_lock, irq_flags);
    logger_fork_log_ptr = NULL;
    write_unlock_irqrestore(&logger_fork_ptr_lock, irq_flags);
    return 0;
}


/*
 * logger_hooksysopen
 * This function sets the open call to be the passed function pointer.
 * It returns the address of the original open
 *
 * Return: address of old open; NULL on failure
 * 
 * @new_open = location of the new open function
 */
void * logger_hooksysopen(void * new_open)
{
    void * old_open = NULL;
    
    if (!pass_mot_security_check())
        return NULL;
    
    old_open = logger_syscall_set(__NR_open, new_open);
    return old_open;
}


/*
 * logger_unhooksysopen
 * This function resets open to its original call,
 * and disregards returning the address of the custom call
 *
 * Return: (int) 0 on success, -1 on fail
 * 
 * @orig_open = address of the original open call
 */
int logger_unhooksysopen(void * orig_open)
{
    if (!pass_mot_security_check())
        return -1;
    
    logger_syscall_set(__NR_open, orig_open);
    return 0;
}


/*
 * logger_syscall_getall
 * This function gets poll, fcntl, fsync, and lseek, and set them in the
 * provided pointers
 *
 * @poll_ptr: pointer to poll
 * @fcntl_ptr: pointer to fcntl
 * @fsync_ptr: pointer to fsync
 * @lseek_prt: pointer to lseek
 */
int logger_syscall_getall(void ** poll_ptr, void ** fcntl_ptr,
            void ** fsync_ptr, void ** lseek_ptr)
{
    if (!pass_mot_security_check())
        return -1;
    
    *poll_ptr = (long (*)(struct pollfd * ufds, unsigned int nfds,
                long timeout))logger_syscall_get(__NR_poll);
    if (*poll_ptr == NULL)
        return -1;

    *fcntl_ptr = (long(*)(unsigned int fd, unsigned int cmd,
                unsigned long arg))logger_syscall_get(__NR_fcntl);
    if (*fcntl_ptr == NULL)
        return -1;
    
    *fsync_ptr = (int(*)(unsigned int fd))logger_syscall_get(__NR_fsync);
    if (*fsync_ptr == NULL)
        return -1;
    
    *lseek_ptr = (long(*)(unsigned int fd, off_t offset,
        unsigned int origin))logger_syscall_get(__NR_lseek);
    if (*lseek_ptr == NULL)
        return -1;
    
    return 0;
}

EXPORT_SYMBOL(logger_hooksysexecve);
EXPORT_SYMBOL(logger_unhooksysexecve);
EXPORT_SYMBOL(logger_hooksysfork);
EXPORT_SYMBOL(logger_unhooksysfork);
EXPORT_SYMBOL(logger_hooksysopen);
EXPORT_SYMBOL(logger_unhooksysopen);
EXPORT_SYMBOL(logger_syscall_getall);
