/*
 * otg/otgcore/otg-trace-l24.c
 * @(#) balden@seth2.belcarratech.com|otg/otgcore/otg-trace-l24.c|20051116205002|48604
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 */
/*
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
/*!
 * @file otg/otgcore/otg-trace-l24.c
 * @brief OTG Core Linux Debug Trace Facility
 *
 * This allows access to the OTG Core Trace via /proc/trace_otg. Each time
 * this file is opened and read it will contain a snapshot of the most
 * recent trace messages. 
 *
 * Reading the trace resets it, the next read will open a new snapshot.
 *
 * N.B. There is no protection from multiple reads.
 *
 * N.B. This is for debugging and is not normally enabled for production software.
 *
 * @ingroup OTGCore
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>
#include <linux/vmalloc.h>
#include <stdarg.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#if defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE)

DECLARE_MUTEX(otg_trace_sem_l24);


/* *    
 * otg_trace_proc_read_l24 - implement proc file system read.
 * @file        
 * @buf         
 * @count
 * @pos 
 *      
 * Standard proc file system read function.
 */         
static ssize_t otg_trace_proc_read_l24 (struct file *file, char *buf, size_t count, loff_t * pos)
{                                  
        unsigned long page;
        int len = 0;
        int index;
        int oindex;
        int rc;

        u64 ticks = 0;

        unsigned char *cp;
        int skip = 0;

        otg_trace_t px;
        otg_trace_t ox;
        otg_trace_t vx;
        otg_trace_t *o, *p;

        RETURN_ENOMEM_IF (!(page = GET_KERNEL_PAGE()));

        _MOD_INC_USE_COUNT;

        // Lock out tag changes while reading
        DOWN(&otg_trace_sem_l24);
        len = otg_trace_proc_read((char *)page, count, (int *)pos);

        if (len > count) 
                len = -EINVAL;
        
        else if (len > 0 && copy_to_user (buf, (char *) page, len)) 
                len = -EFAULT;
        
        //free_page (page);
        UP(&otg_trace_sem_l24);
        free_page (page);
        _MOD_DEC_USE_COUNT;
        return len;
}

#define MAX_TRACE_CMD_LEN  64
/* *
 * otg_trace_proc_write_l24 - implement proc file system write.
 * @file
 * @buf
 * @count
 * @pos
 *
 * Proc file system write function.
 */
static ssize_t otg_trace_proc_write_l24 (struct file *file, const char *buf, size_t count, loff_t * pos)
{
        char command[MAX_TRACE_CMD_LEN+1];
        size_t l = MIN(count, MAX_TRACE_CMD_LEN);

        RETURN_ZERO_UNLESS(count);
        RETURN_EINVAL_IF (copy_from_user (command, buf, l));


        RETURN_EINVAL_IF(copy_from_user (command, buf, l));

        return otg_trace_proc_write(command, l, (int *)pos);
#if 0
        else {
                n -= l;
                while (n > 0) {
                        if (copy_from_user (&c, buf + (count - n), 1)) {
                                count = -EFAULT;
                                break;
                        }
                        n -= 1;
                }
                // Terminate command[]
                if (l > 0 && command[l-1] == '\n') {
                        l -= 1;
                }
                command[l] = 0;
        }
#endif
}

/* Module init ************************************************************** */



static struct file_operations otg_trace_proc_operations_functions = {
        read: otg_trace_proc_read_l24,
        write: otg_trace_proc_write_l24,
};

/*! otg_trace_init_l24
 *
 * Return non-zero if not successful.
 */
int otg_trace_init_l24 (void)
{
        struct proc_dir_entry *p;

        // create proc filesystem entries
        if ((p = create_proc_entry ("trace_otg", 0, 0)) == NULL) 
                printk(KERN_INFO"%s PROC FS failed\n", "trace_otg");
        else 
                p->proc_fops = &otg_trace_proc_operations_functions;

        return 0;
}

/*! otg_trace_exit_l24 - remove procfs entry, free trace data space.
 */
void otg_trace_exit_l24 (void)
{
        otg_trace_t *p;
        unsigned long flags;
        remove_proc_entry ("trace_otg", NULL);
}

#else /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */
int otg_trace_init_l24 (void) {return 0;}
void otg_trace_exit_l24 (void) {}
#endif /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */

