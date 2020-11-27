#ifndef _LINUX_IDLE_H
#define _LINUX_IDLE_H
/*
 * include/linux/idle.h
 *
 * Author: George Anzinger george@mvista.com>
 *
 * Copyright (C) 2004 MontaVista, Software, Inc. 
 * Copyright 2004 Sony Corporation.
 * Copyright 2004 Matsushita Electric Industrial Co., Ltd.
 * 
 * This file is licensed under  the terms of the GNU General Public 
 * License version 2. This program is licensed "as is" without any 
 * warranty of any kind, whether express or implied.
 *
 * The following pertains to the IDLE state which is defined as being
 * entered when the idle task is dispatched (on all cpus) and exited
 * when any other task is wakened.  The call back function will get, as
 * the first parameter, a flag indicating that the system is going into
 * the IDLE state (true or non-zero) or exiting the IDLE state (false or
 * zero).  The second parameter will be a pointer to the
 * IDLE_notify_list structure passed when the function was registered.
 * It is assumed that, for those functions that need more context, this
 * structure is part of a bigger structure which contains that context.
 */
#include <linux/list.h>
#include <linux/smp.h>
#include <linux/cpumask.h>

/*
 * Idle is entered by the system via a call to vst_setup() which first
 * does the idle thing and then sets up the vst sleep.  There is no
 * seperate code to enter idle, and thus no additional arch changes.
 */

struct IDLE_notify_list {
   struct list_head list;
   void (*function)(unsigned long, struct IDLE_notify_list*);
   int state;		     /* 1 called to go idle, 0 called to exit idle */
};
#define INIT_IDLE_NOTIFY_STRUCT(func) {.list.next = 0,		\
                                       .list.prev = 0,		\
                                       .function = func,	\
                                       .state = 0}

/*
 * For SMP systems all cpus must be idle to enter IDLE.  Here we define
 * a word to keep track of who is idle.
 */
extern cpumask_t idle_cpus;

#ifdef CONFIG_IDLE

extern void register_IDLE_function(struct IDLE_notify_list *list);
extern void unregister_IDLE_function(struct IDLE_notify_list *list);

static inline long in_IDLE(void)
{ 
   return (cpus_equal(idle_cpus, cpu_online_map));
}
static inline long __in_IDLE_this(int cpu)
{
	return cpu_isset(cpu, idle_cpus);
}


extern int try_to_enter_idle(void);
extern void exit_idle(void);
extern int idle_enable;

#else

#define register_IDLE_function(foo) do{}while(0)
#define unregister_IDLE_function(foo) do{}while(0)

#define try_to_enter_idle() (local_irq_disable(), 0)
#define exit_idle() do {} while(0)
#define __in_IDLE_this(x) 1

#endif /* CONFIG_IDLE */
#define in_IDLE_this_cpu() __in_IDLE_this(smp_processor_id())
#endif /* _LINUX_IDLE_H */
