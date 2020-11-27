/*
 *  linux/kernel/vst.c
 *
*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 *  VST and IDLE code.    
 *
 *  2004            VST and IDLE code, by George Anzinger
 *                  Copyright (C)2004 by MontaVista Software.
 *                  Copyright 2004 Sony Corporation.
 *                  Copyright 2004 Matsushita Electric Industrial Co., Ltd.
 */
#include <linux/config.h>
#include <linux/kernel_stat.h>
#include <linux/vst.h>
#include <linux/sysctl.h>
#include <linux/ctype.h>
#include <linux/timer-list.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/cpu.h>
#include <asm/uaccess.h>
#include <asm/hardirq.h>
#include <linux/module.h>
#include <linux/interrupt.h>  /* for soft_irq() */
#include <asm/atomic.h>

#ifdef CONFIG_VST
raw_spinlock_t vst_int_disable_lock = RAW_SPIN_LOCK_UNLOCKED;

/* Information for Variable Scheduling Timeout's proc and sysctl interface. */
enum {
        VST_ENABLE = 1,
        VST_THRESHOLD = 2,
	VST_NO_TIMERS = 3,
	VST_SHORT_TIMERS = 4,
	VST_LONG_TIMERS = 5,
	VST_SUCCESS_EXIT = 6,
	VST_EXT_INTR = 7,
	VST_SKIPED_INTS = 8
};
int vst_enable = 0;                /* start disabled allows proper bring up */
int vst_successful_exit = 0;
int vst_external_intr_exit = 0;
int vst_skipped_interrupts = 0;
int vst_enable_min = 0;
int vst_jiffie_clock_disabled;
static int vst_enable_max = 1;
unsigned long vst_next_timer = 0;
unsigned long vst_start, vst_stop;
unsigned long vst_threshold = 1*HZ;
static unsigned long vst_threshold_min = 1;
atomic_t vst_no_timers = ATOMIC_INIT(0);
atomic_t vst_short_timers = ATOMIC_INIT(0);
atomic_t vst_long_timers = ATOMIC_INIT(0);
cpumask_t vst_cpus;

#ifdef  CONFIG_VST_STATS

#define TIMER_STATS_SIZE 10
raw_spinlock_t vst_stats_lock = RAW_SPIN_LOCK_UNLOCKED;
struct vst_stats {
	int fn_index;
	void (*fns[TIMER_STATS_SIZE])(unsigned long);
};

struct vst_stats vst_short_stats = {TIMER_STATS_SIZE - 1};
struct vst_stats vst_long_stats = {TIMER_STATS_SIZE - 1};

void (*vst_long_timer_fns[TIMER_STATS_SIZE])(unsigned long);

extern int proc_vst_doaddvec(ctl_table *table, int write, struct file *filp,
			 void *buffer, size_t *lenp, loff_t *ppos);
#define vst_stats(type, timer) do {					\
	struct vst_stats *p;						\
        spin_lock(&vst_stats_lock);					\
	p = &vst_ ## type ## _stats;					\
	int last = p->fn_index + 2;					\
	if (last >= TIMER_STATS_SIZE)					\
		last = 1;						\
	if (timer &&							\
	    ((p->fns[last] != timer->function ) ||			\
	    (p->fns[--last] != (void(*)(unsigned long))timer->data))) {	\
		p->fns[p->fn_index] = timer->function;			\
		p->fns[--p->fn_index] =					\
                     (void(*)(unsigned long))timer->data;		\
		if (!p->fn_index--)					\
			p->fn_index = TIMER_STATS_SIZE - 1;		\
	}								\
        spin_unlock(&vst_stats_lock);					\
}while(0)
#else
#define vst_stats(type, timer)
#endif
/*
 * It turns out that specifying this large max isn't very useful because
 * do_proc_doulongvec_minmax doesn't guard against overflow.  The value
 * the user gives is multiplied by 1000 (milliseconds per second) and
 * then divided by HZ to get ticks.  If the value the user gives is
 * greater than ULONG_MAX/1000, the multiply overflows and you end up
 * with a small final value which is then compared to the max.
 * so we check here and if it's too big return -EINVAL, if not
 * we call the proc_doulong routine.
 */

/**
 * vst_doulongvec_ms_jiffies_minmax - read a vector of millisecond 
 * values with min/max values
 * @table: the sysctl table
 * @write: %TRUE if this is a write to the sysctl file
 * @filp: the file structure
 * @buffer: the user buffer
 * @lenp: the size of the user buffer
 *
 * Reads/writes up to table->maxlen/sizeof(unsigned long) unsigned long
 * values from/to the user buffer, treated as an ASCII string. The values
 * are treated as milliseconds, if they won't produce an overflow we
 * call the systctl routine.
 *
 * Returns 0 on success.
 */
#define MAX_NON_OVERFLOW_SIZE	42949670

static int vst_ms_to_jiffies(ctl_table *table, int write,
    struct file *filp, void *buffer, size_t *lenp, loff_t *ppos)
{
#define TMPBUFLEN 20
	long *i, val;
	int vleft, first=1, neg;
	size_t len, left;
	char buf[TMPBUFLEN], *p;
	
	if (!table->data || 
	    !table->maxlen || 
	    !*lenp || 
	    (filp->f_pos && !write)) {
		*lenp = 0;
		return 0;
	}
	
	i = (long *) table->data;
	vleft = table->maxlen / sizeof(long);
	left = *lenp;
	
	for (; left && vleft--; i++, first = 0) {
		if (write) {
			while (left) {
				char c;
				if(get_user(c, (char *) buffer))
					return -EFAULT;
				if (!isspace(c))
					break;
				left--;
				((char *) buffer)++;
			}
			if (!left)
				break;
			neg = 0;
			len = left;
			if (len > TMPBUFLEN-1)
				len = TMPBUFLEN-1;
			if (copy_from_user(buf, buffer, len))
				return -EFAULT;
			buf[len] = 0;
			p = buf;
			if (*p == '-' && left > 1) {
				neg = 1;
				left--, p++;
			}
			if (*p < '0' || *p > '9')
				break;
			val = simple_strtoul(p, &p, 0);
			if (val > MAX_NON_OVERFLOW_SIZE || val < 0) {
				return -EINVAL;
			}
		}
	}
	return proc_doulongvec_ms_jiffies_minmax(table, write, filp, 
						 buffer, lenp, ppos);
}

static unsigned long vst_threshold_max = MAX_TIMER_INTERVAL;
ctl_table vst_table[] = {
	{ .ctl_name = VST_ENABLE,
	  .procname = "enable",
	  .data = &vst_enable,
	  .maxlen = sizeof(vst_enable),
	  .mode = 0644,
	  .child = NULL,
	  .proc_handler = &proc_dointvec_minmax,
	  .strategy = &sysctl_intvec,
	  .de = NULL,
	  .extra1 = &vst_enable_min,
	  .extra2 = &vst_enable_max
	},
	{ .ctl_name = VST_THRESHOLD,
	  .procname = "threshold",
	  .data = &vst_threshold,
	  .maxlen = sizeof(vst_threshold),
	  .mode = 0644,
	  .child = NULL,
	  .proc_handler = &vst_ms_to_jiffies,
	  .strategy = &sysctl_intvec,
	  .de = NULL,
	  .extra1 = &vst_threshold_min,
	  .extra2 = &vst_threshold_max
	},
	{ .ctl_name = VST_NO_TIMERS,
	  .procname = "no_timers",
	  .data = &atomic_read(&vst_no_timers),
	  .maxlen = sizeof(vst_no_timers),
	  .mode = 0644,
	  .child = NULL,
	  .proc_handler = &proc_dointvec,
	  .strategy = &sysctl_intvec,
	  .de = NULL,
	  .extra1 = 0,
	  .extra2 = (void *)0x7fffffff,
	},
	{ .ctl_name = VST_SHORT_TIMERS,
	  .procname = "short_timers",
	  .data = &atomic_read(&vst_short_timers),
	  .maxlen = sizeof(vst_short_timers),
	  .mode = 0644,
	  .child = NULL,
	  .proc_handler = &proc_dointvec,
	  .strategy = &sysctl_intvec,
	  .de = NULL,
	},
	{ .ctl_name = VST_LONG_TIMERS,
	  .procname = "long_timers",
	  .data = &atomic_read(&vst_long_timers),
	  .maxlen = sizeof(vst_long_timers),
	  .mode = 0644,
	  .child = NULL,
	  .proc_handler = &proc_dointvec,
	  .strategy = &sysctl_intvec,
	  .de = NULL,
	},
#ifdef CONFIG_VST_STATS
	{ .ctl_name = VST_LONG_TIMERS,
	  .procname = "long_timer_fns",
	  .data = &vst_long_stats.fns,
	  .maxlen = sizeof(vst_long_stats.fns),
	  .mode = 0444,
	  .child = NULL,
	  .proc_handler = &proc_vst_doaddvec,
	  .strategy = NULL,
	  .de = NULL,
	  .extra1 = &vst_long_stats.fn_index,
	},
	{ .ctl_name = VST_LONG_TIMERS,
	  .procname = "short_timer_fns",
	  .data = &vst_short_stats.fns,
	  .maxlen = sizeof(vst_short_stats.fns),
	  .mode = 0444,
	  .child = NULL,
	  .proc_handler = &proc_vst_doaddvec,
	  .strategy = NULL,
	  .de = NULL,
	  .extra1 = &vst_short_stats.fn_index,
	},
#endif
        { .ctl_name = VST_SUCCESS_EXIT,
          .procname = "successful_vst_exit",
          .data = &vst_successful_exit,
          .maxlen = sizeof(vst_successful_exit),
          .mode = 0644,
          .child = NULL,
          .proc_handler = &proc_dointvec,
          .strategy = &sysctl_intvec,
          .de = NULL,
        },
        { .ctl_name = VST_EXT_INTR,
          .procname = "external_intr_exit",
          .data = &vst_external_intr_exit,
          .maxlen = sizeof(vst_external_intr_exit),
          .mode = 0644,
          .child = NULL,
          .proc_handler = &proc_dointvec,
          .strategy = &sysctl_intvec,
          .de = NULL,
        },
        { .ctl_name = VST_SKIPED_INTS,
          .procname = "skipped_interrupts",
          .data = &vst_skipped_interrupts,
          .maxlen = sizeof(vst_skipped_interrupts),
          .mode = 0644,
          .child = NULL,
          .proc_handler = &proc_dointvec,
          .strategy = &sysctl_intvec,
          .de = NULL,
        },
	{0}
};
#ifdef CONFIG_VST_STATS
/*
 * We use extra1 as the current index assuming the next value to be
 * filled is at that index.  We then output the newest values first.
 */
#define OP_SET	0
#define OP_AND	1
#define OP_OR	2
#define OP_MAX	3
#define OP_MIN	4

#define TMPBUFLEN 20
static int vst_do_proc_doaddvec(ctl_table *table, int write, struct file *filp,
		  void *buffer, size_t *lenp, loff_t *ppos)
{
	int  index, vleft, first=1;
	long *i, *maxi, *valp;
	size_t left, len;
	char buf[TMPBUFLEN], *p;
	
	if (write)
		return -EFAULT;

	if (!table->data || !table->maxlen || !*lenp || filp->f_pos) {
		*lenp = 0;
		return 0;
	}
	
	i = (long *) table->data;
	vleft = table->maxlen / sizeof(long);
	left = *lenp;
	index = *(int *)table->extra1 + 1;
	if (index >= vleft)
		index -= vleft;
	maxi = i + vleft;
	
	for (; left && (vleft > 0); i += 2, first=0) {
		vleft -= 2;
		p = buf;
		valp = i + index;
		if (valp >= maxi)
			valp -= table->maxlen / sizeof(long);
		if (!first) {
			*p++ = '\t';
		}
		sprintf(p, "%lx(%lx)", *(valp + 1), *(valp));
		*valp = 0;        /* here is where we zero the data */
		*(valp + 1) = 0;
		len = strlen(buf);
		if (len > left)
			len = left;
		if(copy_to_user(buffer, buf, len))
			return -EFAULT;
		left -= len;
		buffer += len;
	}

	if (!first && left) {
		if(put_user('\n', (char *) buffer))
			return -EFAULT;
		left--, buffer++;
	}
	*lenp -= left;
	*ppos += *lenp;
	return 0;
}
/**
 * proc_doaddvec - read a vector of addresses
 * @table: the sysctl table
 * @write: %TRUE if this is a write to the sysctl file (currently not supported)
 * @filp: the file structure
 * @buffer: the user buffer
 * @lenp: the size of the user buffer
 *
 * Reads up to table->maxlen/sizeof(unsigned int) integer
 * values to the user buffer, treated as an ASCII string. 
 * ---> At this point only read is supported. <----
 *
 * Returns 0 on success.
 */
int proc_vst_doaddvec(ctl_table *table, int write, struct file *filp,
		     void *buffer, size_t *lenp, loff_t *ppos)
{
    return vst_do_proc_doaddvec(table,write,filp,buffer,lenp,ppos);
}
#endif

/*
 * find_next_timer scans the timer list to find the next timer that will
 * happen.  There are four possible outcomes (listed from highest
 * to lowest precedence):
 *   0) The scan was interrupted by an interrupt or another cpu messing
 *      with the timer list.  Time to start over.  SCAN_INTERRUPTUS
 *      returned.
 *   1) No timers in the timer list:
 *	NO_NEXT_TIMER will be returned and *when is set to max interval.
 *   2) A timer is found that expires in less than threshold ticks:
 *	SHORT_NEXT_TIMER will be returned.
 *   3) Only timers >= threshold are found
 *	RETURNED_NEXT_TIMER is returned and *when is set to when the
 *	next timer will occur.
 *
 * vst_threshold and *when are specified as a relative number of ticks
 * instead of an absolute value for jiffies.
 *
 * find_next_timer is currently only used by the variable scheduling
 * timeout code in the idle loop to determine when the next timer event
 * should occur.  This call MUST be made with the interrupt system on
 * and no locks held.  On a successful exit (i.e. SCAN_INTERRUPTUS) the
 * return will be with the interrupt system off. The caller takes
 * different actions based on the return:
 *
 *   SCAN_INTERRUPTUS: Do any preamble stuff and try again.  I.e. it
 *                     should look like idle was entered all over again.
 *                     One possiblility is to jump to just after the
 *                     idle instruction.
 *
 *   NO_NEXT_TIMER:    disable the tick interrupt but don't set an interval
 *                     timer
 *
 *   SHORT_NEXT_TIMER: leave the tick interrupt enabled
 *
 *   RETURNED_NEXT_TIMER: disable the tick interrupt and set an interval
 *                     timer
 */
#define MAX_LOOP_COUNT 10
#define INDEX(N) (base->timer_jiffies >> (TVR_BITS + N * TVN_BITS))
#define INCINDEX(N) ((base->timer_jiffies & ((1 << (TVR_BITS + N * TVN_BITS)) - 1)) ? 1 : 0)
static int latency_test(int *loop_count, tvec_base_t *base)
{
	if (++*loop_count >= MAX_LOOP_COUNT) {
		int visit_count = base->visit_count;
		spin_unlock(&base->lock);
		local_irq_enable();
		*loop_count = 0;
		cpu_relax();
		local_irq_disable();
		if (!spin_trylock(&base->lock))
			goto ex;
		if (visit_count != base->visit_count) {
			spin_unlock(&base->lock);
		ex:		
			local_irq_enable();
			return 1;
		}
	}
	return 0;
}
#if 0 /* code to make sure we really found the right timer */
static void check_list(struct list_head *head, unsigned long exp)
{
	struct timer_list * timer;

	list_for_each_entry(timer, head, entry) {
		if (time_before(timer->expires, exp)) {
			BREAKPOINT();
		}
	}
	return;
}
static void check_whole_list(tvec_base_t *base, unsigned long exp)
{
	int k,kk;
	tvec_t *varray[4];
	struct timer_list  *tmp_timer;
	struct list_head *head;

	varray[0] = &base->tv2;
	varray[1] = &base->tv3;
	varray[2] = &base->tv4;
	varray[3] = &base->tv5;

	for (k = 0; k < TVR_SIZE; k++) {
		head = base->tv1.vec + k;
		if (!list_empty(head)) {
			tmp_timer = list_entry(head->next, 
					       struct timer_list, entry);
			if (time_before(tmp_timer->expires, exp)) {
				BREAKPOINT();
			}
		}
	}
	for (kk = 0; kk < 4; kk++) {
		for (k = 0; k < TVN_SIZE; k++) {
			head = varray[kk]->vec + k;
			check_list(head, exp);
		}
	}
}
#endif /* debug code */
static int find_next_timer(unsigned long *when)
{
	unsigned long threshold_jiffies = jiffies + vst_threshold;
	tvec_base_t *base;
	struct list_head *list;
	struct timer_list *timer = 0, *tmp_timer;
	unsigned long expires;
	tvec_t *varray[4];
	int i = -1, j, end, index = 0, loop_count = 0;
	unsigned long tjiffies;

	local_irq_disable();
	base = &__get_cpu_var(tvec_bases);
	/* 
	 * If there is something in the timer queue, well, it just
	 * should not be. Well, rcu seems to get stuck.  Give it a
	 * break.
	 */
	if (!spin_trylock(&base->lock)) {
	not_now:
		if (local_softirq_pending())
			do_softirq();
		local_irq_enable();
		return SCAN_INTERRUPTUS;
	}
	tjiffies = base->timer_jiffies;
	if (base->timer_jiffies != (jiffies + 1)) {
	run_l_timers:
		run_local_timers();
		spin_unlock(&base->lock);
		goto not_now;
	}
	if (hrtimer_is_pending()) {
		spin_unlock(&base->lock);
		return SHORT_NEXT_TIMER;
	}

	*when = expires = base->timer_jiffies + (LONG_MAX >> 1);
	list = 0;

	varray[0] = &base->tv2;
	varray[1] = &base->tv3;
	varray[2] = &base->tv4;
	varray[3] = &base->tv5;
	/*

	 * So many ways to get this wrong...  The way the cascade works
	 * it is possible to have timers that expire at the same time in
	 * two lists (or even in more than two lists).  (It depends on
	 * when they came into the list WRT now.)  For each of the
	 * arrays, timers found from the current mark to the end of that
	 * array will be unique.  However, timers found from the
	 * beginning of the array to the current mark may have other,
	 * earlier timers in the marked entry of the next array.  This
	 * is true for each array except, of course, the last one.  We,
	 * therefor set up to scan the next array's marked list as part
	 * of the reset to the zero'th entry on the current one.  Each
	 * list in tv1 only contains timers that expire at the same
	 * time.  That means we don't need to shoot all the lists, just
	 * look for the first non-empty list and then look at its head.

	 */
	/* Look for timer events in tv1. */

	end = j = base->timer_jiffies & TVR_MASK;
	index = (INDEX(0)+ (end ? 1 : 0)) & TVN_MASK;
	do {
		struct list_head *head;
		head = base->tv1.vec + j;
		if (!j) {
			list = base->tv2.vec + index;
		}
		if (latency_test(&loop_count, base)) 
			return SCAN_INTERRUPTUS;

		if (!list_empty(head)) {
			timer = list_entry(head->next, typeof(*timer), entry);

			expires = timer->expires;
			goto found;
		}
		j = (j + 1) & TVR_MASK;
	} while (j != end);
		
	/* Check tv2-tv5. */
	for (i = 0; i < 4; i++) {
		end = j = index;
		list = 0;
		index = (INDEX(i + 1) + INCINDEX(i+1)) & TVN_MASK;
		do {
			if (unlikely(!j && (i < 3))) {
				list = varray[i + 1]->vec + index;
			}	
			if (list_empty(varray[i]->vec + j)) {
				j = (j + 1) & TVN_MASK;
				continue;
			}
			list_for_each_entry(tmp_timer, varray[i]->vec + j, 
					    entry) {
				if (latency_test(&loop_count, base)) 
					return SCAN_INTERRUPTUS;

				if (time_before(tmp_timer->expires, expires)) {
					expires = tmp_timer->expires;
					timer = tmp_timer;
				}
			}
			goto found;

		} while (j != end);
	}
	spin_unlock(&base->lock);
	return NO_NEXT_TIMER;
found:
	if (list) {
		/*
		 * The search wrapped. We need to look at the next list
		 * from next tv element that would cascade into tv element
		 * where we found the timer element. This too can cascade...
		 */
		do {
			list_for_each_entry(tmp_timer, list, entry) {
				if (latency_test(&loop_count, base)) 
					return SCAN_INTERRUPTUS;

				if (time_before(tmp_timer->expires, expires)) {
					expires = tmp_timer->expires;
					timer = tmp_timer;
				}
			}
			list = 0;
			if (!index && (++i < 3)) {
				index = (INDEX(i + 1) + 1) & TVN_MASK;
				list = varray[i + 1]->vec + index;
			}	
		} while (list);
	}
	/*
	 * We have a timer, figure out what the return is.
	 * But first, make sure the timer list is up to date.
	 */
	if ((jiffies + 1) != base->timer_jiffies) 
		goto run_l_timers;

	/* check_whole_list(base, expires); debug code */
	*when = expires;
	if (time_after(threshold_jiffies, expires)) {
		vst_stats(short, timer);
		spin_unlock(&base->lock);
		return SHORT_NEXT_TIMER;
	}
	vst_stats(long, timer);
	spin_unlock(&base->lock);
	return RETURNED_NEXT_TIMER;
}

int _do_vst_setup(void)
{
	if (!vst_enable)
		return 0;

	switch (find_next_timer(&vst_next_timer)) {
	case SCAN_INTERRUPTUS:
		return 1;

	case SHORT_NEXT_TIMER:
		/*
		 * Next timer is less than threshold; leave the periodic
		 * tick interrupt enabled and don't set up a
		 * non-periodic timer.
		 */
		atomic_inc(&vst_short_timers);
		return 0;
	case NO_NEXT_TIMER:
		/*
		 * No timers in lists; If we didn't have to worry about
		 * timer overflow, we could disable the periodic tick
		 * interrupt and don't bother with a non-periodic timer.
		 * We would then wait for an interrupt to wake us up.
		 * However, that is not the case.  vst_next_timer has
		 * been set to the maximum interval, so just handle it
		 * the same as RETURNED_NEXT_TIMER.
		 */
		atomic_inc(&vst_no_timers);
		break;
	case RETURNED_NEXT_TIMER:
		/*
		 * vst_next_timer contains when next timer will expire;
		 * disable the periodic tick interrupt and set a
		 * non-periodic timer.
		 */
		atomic_inc(&vst_long_timers);
	}
	vst_sleep_till(vst_next_timer);
	return 0;
}

#endif /* CONFIG_VST */
#ifdef CONFIG_IDLE
#include <linux/idle.h>
enum {
	IDLE_ENABLE = 1,
	IDLE_ATTEMPTS,
	IDLE_SUCCESS
};
int idle_enable = 0;
static int idle_attempts;
static int idle_success;
long zip;
long one = 1;
long maxlong = 0x7fffffff;

ctl_table idle_table[] = {
	{ .ctl_name = IDLE_ENABLE,
	  .procname = "enable",
	  .data = &idle_enable,
	  .maxlen = sizeof(idle_enable),
	  .mode = 0644,
	  .child = NULL,
	  .proc_handler = &proc_dointvec_minmax,
	  .strategy = &sysctl_intvec,
	  .de = NULL,
	  .extra1 = &zip,
	  .extra2 = &one
	},
	{ .ctl_name = IDLE_ATTEMPTS,
	  .procname = "idle_attempts",
	  .data = &idle_attempts,
	  .maxlen = sizeof(idle_attempts),
	  .mode = 0644,
	  .child = NULL,
	  .proc_handler = &proc_dointvec,
	  .strategy = &sysctl_intvec,
	  .de = NULL,
	  .extra1 = &zip,
	  .extra2 = &maxlong
	},
	{ .ctl_name = IDLE_SUCCESS,
	  .procname = "idle_success",
	  .data = &idle_success,
	  .maxlen = sizeof(idle_success),
	  .mode = 0644,
	  .child = NULL,
	  .proc_handler = &proc_dointvec,
	  .strategy = &sysctl_intvec,
	  .de = NULL,
	  .extra1 = &zip,
	  .extra2 = &maxlong,
	},
	{0}
};

static LIST_HEAD(IDLE_notify_list); 
int IDLE_list_modified;
cpumask_t idle_cpus;
EXPORT_SYMBOL(idle_cpus);
raw_spinlock_t idle_list_lock = RAW_SPIN_LOCK_UNLOCKED;

/*
 * Possibly entering IDLE.  Actually only enter IDLE if all cpus are
 * idle.  This is a bit tricky.  We want to stop if ever there is work
 * to do.  We come up for air after each call back.  If we are not still
 * in IDLE, we return failure. If, for some reason, the call back list
 * is modified, we want to restart the scan as we can no longer trust
 * the pointers.  This is a bit more paranoid than the
 * list_for_each_safe as we assume that ANY entry can be removed.
 *
 * return 0 on success, 1 if we need to go round again.  Note, we return
 * in this case so any prior work can be redone.  If this is the first
 * thing in the idle task, it should just call again.  If the system has
 * preemption turned off, a call to schedule may be in order.
 *
 * This code attempts to be a model "idle entry" citizen.  This means
 * that, if ever it finds that it should start over, it returns a falure
 * rather than starting over.  This is to allow any prior code that acts
 * the same way to be restarted at the same time.  We also provide an
 * "all is idle" and a "this cpu is idle" function so that a simple test
 * at the end of the idle set up may be done.
 *
 *
 * On the sucess return we return with local_irq_disable() in effect.
 */
int try_to_enter_idle()
{
	long cpu;
	struct list_head *pos;
	struct IDLE_notify_list *entry;

	local_irq_disable();

	spin_lock(&idle_list_lock);
	cpu = smp_processor_id();
 	cpu_set(cpu, idle_cpus);
	/*
	 * Only go into IDLE if all cpus are idle.
	 *
         * Note we do the idle_enable test here AFTER the idle_cpus set
         * up so that the caller does not have to test "idle_enable" to
         * see if the cpu_idle is valid.  I.e. it is always valid, even
         * when idle_enable is false.
	 */
	if (!in_IDLE() || !idle_enable) {
		spin_unlock(&idle_list_lock);
		return 0;
	}
	idle_attempts++;
enter_list_restart:
 	list_for_each(pos, &IDLE_notify_list) {
		IDLE_list_modified = 0;
		entry = list_entry(pos, struct IDLE_notify_list, list);
		if (!entry->state){
			/*
			 * Do the state change first as the callee
			 * just may remove himself from the
			 * list and we don't want to modify anything
			 * that is not in the list.
			 */
			entry->state = 1;
			entry->function(1, entry);
		} else {
			continue;
		}
		spin_unlock_irq(&idle_list_lock);
		spin_lock_irq(&idle_list_lock);
		if (!in_IDLE() || need_resched()) {
			spin_unlock_irq(&idle_list_lock);
			return 1;
		}
		if (IDLE_list_modified)
			goto enter_list_restart;
	}
	idle_success++;
	spin_unlock(&idle_list_lock);
	return 0;
}
/*
 * Called from schedule() with interrupts off.  Keep it that way.
 */
void exit_idle()
{
	struct IDLE_notify_list *entry;
	struct list_head *pos;
	long in_IDLE_flag;

	spin_lock(&idle_list_lock);
	in_IDLE_flag = in_IDLE();
	cpu_clear(smp_processor_id(), idle_cpus);
	if (in_IDLE_flag){

	exit_idle_restart:
		IDLE_list_modified = 0;
		list_for_each(pos, &IDLE_notify_list) {
			entry = list_entry(pos, struct IDLE_notify_list, list);
			if (entry->state){
				/*
				 * Do the state change first as the callee
				 * just may remove himself from the
				 * list and we don't want to modify anything
				 * that is not in the list.
				 */
				entry->state = 0;
				entry->function(0, entry);
			} else {
				continue;
			}
			if (IDLE_list_modified)
				goto exit_idle_restart;
		}
	}
	spin_unlock(&idle_list_lock);
	
}
void register_IDLE_function(struct IDLE_notify_list *list)
{
	unsigned long flags;
	BUG_ON(list->list.next);
	spin_lock_irqsave(&idle_list_lock, flags);
	list_add_tail(&list->list, &IDLE_notify_list);
	IDLE_list_modified++;
	spin_unlock_irqrestore(&idle_list_lock, flags);
}
void unregister_IDLE_function(struct IDLE_notify_list *list)
{
	unsigned long flags;
	spin_lock_irqsave(&idle_list_lock, flags);
	list_del(&list->list);
	IDLE_list_modified++;
	spin_unlock_irqrestore(&idle_list_lock, flags);
}
EXPORT_SYMBOL(register_IDLE_function);
EXPORT_SYMBOL(unregister_IDLE_function);

#endif     /* CONFIG_IDLE */

#if defined(CONFIG_IDLE) && ! defined(CONFIG_VST)
int do_vst_setup(void)
{
	return try_to_enter_idle();
}
#elif ! defined(CONFIG_IDLE) &&  defined(CONFIG_VST)
int  do_vst_setup(void)
{
	return _do_vst_setup();
}
#else
/*
 * If both IDLE and VST we require a full idle system prior to 
 * even considering vst.
 */
int do_vst_setup(void)
{
	if (!try_to_enter_idle()) {
		local_irq_enable();
		if (!_do_vst_setup()){
			if (in_IDLE() || !idle_enable) {
				return 0;
			}
		}
	}
	return 1;

}
#endif /* defined(CONFIG_IDLE) || defined(CONFIG_VST) */
