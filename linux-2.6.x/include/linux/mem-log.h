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
 * ----------   --------  ----------------------
 * 03/11/2008   Motorola  Initial version
 */

/*
 * memory buffer logging
 */
#ifndef _LINUX_MEMLOG_H
#define _LINUX_MEMLOG_H

/*
 * Event type
 * 
 * If you update this update the output code as well
 */
enum log_type {
    MEM_LOG_UNKNOWN,
    MEM_LOG_IRQ,
    MEM_LOG_SIRQ,
    MEM_LOG_TASKLET,
    MEM_LOG_HI_TASKLET,
    MEM_LOG_TIMER,
    MEM_LOG_LTT_START,
    MEM_LOG_LTT_DONE,
    MEM_LOG_LTT_RESERVED,
    MEM_LOG_LTT_ICP,
    MEM_LOG_SCHED_ICP,
    MEM_LOG_FOUND_IT
};

/* mem_log_event() used to record correspoding value depends on
 * different caller
 */
extern void mem_log_event(void);

/* mem_print_log() used to print out event log in die() in traps.c
 */
extern void mem_print_log(void);

#endif
