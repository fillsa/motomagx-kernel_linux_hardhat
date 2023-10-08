#ifndef LINUX_MPMDRV_H
#define LINUX_MPMDRV_H

/* mpmdrv.h 
 *                               
 * Copyright 2006-2008 Motorola, Inc.
 *                             
 */                                                  

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * Date        Author            Comment
 * ==========  ================  ========================
 * 10/04/2006  Motorola          Initial version.
 * 01/18/2007  Motorola          Tie DSM to IDLE and reduce IOI timeout to 10ms
 * 02/14/2007  Motorola          Resolved race condition in DSM code.
 * 08/03/2007  Motorola          Move declaration of mpm set awake to kernel mpm.h
 * 03/31/2008  Motorola          Move declaration of mpm_set_awake_state to kernel mpm.h inlj63
 * 10/25/2007  Motorola          Improved periodic job state collection for debug.
 */


#ifdef __KERNEL__

#include <asm/arch/mxc_pm.h>

#define COMM_SIZE 16
#define MAX_BUCKETS 10

typedef struct {
    mpm_op_t cpu_freq;
    struct timeval start_time;
    struct timeval end_time;
} mpm_opstat_t;

struct mpm_opst_list;

typedef struct mpm_opst_list{
    struct mpm_opst_list *next;
    mpm_opstat_t opst;
} mpm_opst_list_t;

#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST)
typedef struct {
    unsigned long cur_time;
    unsigned long minduration;
    unsigned long maxduration;
    unsigned long averageduration[MAX_BUCKETS];
    int durationcount[MAX_BUCKETS];
    int wakeuptimes[MAX_BUCKETS];
    int count;
    int waiting;
} mpm_pjstat_t;

struct mpm_pjist_list;

typedef struct mpm_pjist_list{
    struct mpm_pjist_list *next;
    unsigned long orig_func;
    mpm_pjstat_t pjstat;
} mpm_pjist_list_t;

struct mpm_pjcst_list;

typedef struct mpm_pjcst_list{
    struct mpm_pjcst_list *next;
    int pid;
    char process_comm[COMM_SIZE];
    mpm_pjstat_t pjstat;
} mpm_pjcst_list_t;
#endif


#define ssec(n)      ((n)->opst.start_time.tv_sec)
#define susec(n)     ((n)->opst.start_time.tv_usec)
#define esec(n)      ((n)->opst.end_time.tv_sec)
#define eusec(n)     ((n)->opst.end_time.tv_usec)


/* define AHB and IP freq as constants */
#define MPM_AHB_FREQ          AHB_FREQ 
#define MPM_IP_FREQ           IPG_FREQ 

/* Index values of the items in the sleep modes array */
enum {
    MPM_STOP,
    MPM_DSM,
};

/* Low power mode states */
enum 
{
    LPM_STATE_UNDEFINED=1,
    LPM_STATE_AWAKE,
    LPM_STATE_WAITING_FOR_SLEEP_TIMER,
    LPM_STATE_WAITING_FOR_IDLE,
    LPM_STATE_INITIATING_SLEEP,
    LPM_STATE_SLEEPING,
    LPM_STATE_WAKING_UP,
    LPM_STATE_WAITING_FOR_PJ_COMPLETION,
};

/* Low power mode actions for DSM state machine */
enum 
{
    LPM_ACTION_AWAKE_REQUEST=1,
    LPM_ACTION_SLEEP_REQUEST,
    LPM_ACTION_SYSTEM_IDLE,
    LPM_ACTION_SUSPEND_TIMER_EXPIRES,
    LPM_ACTION_RESUME_FROM_SLEEP,
    LPM_ACTION_HANDLE_IOI,
    LPM_ACTION_HANDLE_LONG_IOI,
    LPM_ACTION_SUSPEND_SUCCEEDED,
    LPM_ACTION_SUSPEND_FAILED,
    LPM_ACTION_READY_TO_SLEEP_IN_WQ,
    LPM_ACTION_PERIODIC_JOBS_DONE,
};

extern void mpm_set_initiate_sleep_state(void);
extern void mpm_get_last_desense_request(mpm_desense_info_t *, int *);


#endif /* __KERNEL__ */

#endif	/* LINUX_MPMDRV_H */
