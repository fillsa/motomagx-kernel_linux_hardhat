/*
 * Generic RTC interface.
 * This version contains the part of the user interface to the Real Time Clock
 * service. It is used with both the legacy mc146818 and also  EFI
 * Struct rtc_time and first 12 ioctl by Paul Gortmaker, 1996 - separated out
 * from <linux/mc146818rtc.h> to this file for 2.4 kernels.
 * 
 * Copyright (C) 1999 Hewlett-Packard Co.
 * Copyright (C) 1999 Stephane Eranian <eranian@hpl.hp.com>
 *
 * Copyright 2006 Motorola, Inc.
 * 
 * Date     Author    Comment
 * 10/2006  Motorola  Added the user interface to the RTC Stopwatch driver
 * 11/2006  Motorola  Added kernel interface to RTC Stopwatch driver
 * 12/2006  Motorola  Fixed Power Management usage in RTC Stopwatch driver
 *
 */
#ifndef _LINUX_RTC_H_
#define _LINUX_RTC_H_

#ifdef __KERNEL__
#include <linux/interrupt.h>
#endif

/*
 * The struct used to pass data via the following ioctl. Similar to the
 * struct tm in <time.h>, but it needs to be here so that the kernel 
 * source is self contained, allowing cross-compiles, etc. etc.
 */

struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

/*
 * The struct used to pass data via the RTC Stopwatch ioctl.
 */
struct rtc_sw_time {
	unsigned int hours;
	unsigned int minutes;
	unsigned int seconds;
	unsigned int hundredths;
};

enum {
        PJ_NOT_RUNNING = 0,
        PJ_RUNNING = 1,
};

int rtc_sw_periodic_jobs_running(void);

/*
 * This data structure is inspired by the EFI (v0.92) wakeup
 * alarm API.
 */
struct rtc_wkalrm {
	unsigned char enabled;	/* 0 = alarm disable, 1 = alarm disabled */
	unsigned char pending;  /* 0 = alarm pending, 1 = alarm not pending */
	struct rtc_time time;	/* time the alarm is set to */
};

/*
 * Data structure to control PLL correction some better RTC feature
 * pll_value is used to get or set current value of correction,
 * the rest of the struct is used to query HW capabilities.
 * This is modeled after the RTC used in Q40/Q60 computers but
 * should be sufficiently flexible for other devices
 *
 * +ve pll_value means clock will run faster by
 *   pll_value*pll_posmult/pll_clock
 * -ve pll_value means clock will run slower by
 *   pll_value*pll_negmult/pll_clock
 */ 

struct rtc_pll_info {
	int pll_ctrl;       /* placeholder for fancier control */
	int pll_value;      /* get/set correction value */
	int pll_max;        /* max +ve (faster) adjustment value */
	int pll_min;        /* max -ve (slower) adjustment value */
	int pll_posmult;    /* factor for +ve correction */
	int pll_negmult;    /* factor for -ve correction */
	long pll_clock;     /* base PLL frequency */
};

/*
 * ioctl calls that are permitted to the /dev/rtc interface, if
 * any of the RTC drivers are enabled.
 */

#define RTC_AIE_ON	_IO('p', 0x01)	/* Alarm int. enable on		*/
#define RTC_AIE_OFF	_IO('p', 0x02)	/* ... off			*/
#define RTC_UIE_ON	_IO('p', 0x03)	/* Update int. enable on	*/
#define RTC_UIE_OFF	_IO('p', 0x04)	/* ... off			*/
#define RTC_PIE_ON	_IO('p', 0x05)	/* Periodic int. enable on	*/
#define RTC_PIE_OFF	_IO('p', 0x06)	/* ... off			*/
#define RTC_WIE_ON	_IO('p', 0x0f)  /* Watchdog int. enable on	*/
#define RTC_WIE_OFF	_IO('p', 0x10)  /* ... off			*/

#define RTC_ALM_SET	_IOW('p', 0x07, struct rtc_time) /* Set alarm time  */
#define RTC_ALM_READ	_IOR('p', 0x08, struct rtc_time) /* Read alarm time */
#define RTC_RD_TIME	_IOR('p', 0x09, struct rtc_time) /* Read RTC time   */
#define RTC_SET_TIME	_IOW('p', 0x0a, struct rtc_time) /* Set RTC time    */
#define RTC_IRQP_READ	_IOR('p', 0x0b, unsigned long)	 /* Read IRQ rate   */
#define RTC_IRQP_SET	_IOW('p', 0x0c, unsigned long)	 /* Set IRQ rate    */
#define RTC_EPOCH_READ	_IOR('p', 0x0d, unsigned long)	 /* Read epoch      */
#define RTC_EPOCH_SET	_IOW('p', 0x0e, unsigned long)	 /* Set epoch       */

#define RTC_WKALM_SET	_IOW('p', 0x0f, struct rtc_wkalrm)/* Set wakeup alarm*/
#define RTC_WKALM_RD	_IOR('p', 0x10, struct rtc_wkalrm)/* Get wakeup alarm*/

#define RTC_PLL_GET	_IOR('p', 0x11, struct rtc_pll_info)  /* Get PLL correction */
#define RTC_PLL_SET	_IOW('p', 0x12, struct rtc_pll_info)  /* Set PLL correction */

/*
 * ioctl calls that are permitted to the /dev/rtc_sw interface, if
 * the high res timers are enabled.
 */

/* Set RTC Stopwatch strict request */
#define RTC_SW_SETTIME		_IOW('q', 0x1, struct rtc_sw_time *)
/* Set RTC Stopwatch fuzzy request */
#define RTC_SW_SETTIME_FUZZ	_IOW('q', 0x2, struct rtc_sw_time *)
/* Delete RTC Stopwatch request */
#define RTC_SW_DELTIME		_IOW('q', 0x3, struct rtc_sw_time *)
/* RTC Stopwatch, App job done */
#define RTC_SW_JOB_DONE		_IO('q', 0x4)
/* RTC Stopwatch, App exit */
#define RTC_SW_APP_EXIT		_IO('q', 0x5)

/* RTC Stopwatch, Get milliseconds from boot, takes a pointer to a unsigned long that will be populated with this value*/
#define RTC_SW_MS_FROM_BOOT		_IOW('q', 0x6, unsigned long *)

#ifdef __KERNEL__

typedef struct rtc_task {
	void (*func)(void *private_data);
	void *private_data;
} rtc_task_t;

int rtc_register(rtc_task_t *task);
int rtc_unregister(rtc_task_t *task);
int rtc_control(rtc_task_t *t, unsigned int cmd, unsigned long arg);
void rtc_get_rtc_time(struct rtc_time *rtc_tm);

/*RTC Stop Watch Timer Functions*/

/*!
 * @return the number of milliseconds from boot.
 */
unsigned long rtc_sw_msfromboot(void);

/*!
 * This is used to get a value which represents a time used by the counter.
 * This number does not mean anything until it is converted into milliseconds with
 * rtc_sw_internal_to_ms.  The reason this lower level is exposed is for performance
 * reasons.  Converting this to ms requires a divide which on some architectures is
 * expensive, and some drivers may not want to do it in interrupt context.
 */
unsigned long rtc_sw_get_internal_ticks(void);

/*!
 * Convert the internal representation returned by  rtc_sw_get_internal_ticks to ms.
 */
unsigned long rtc_sw_internal_to_ms(unsigned long ticks);

/*!
 * This holds information about the callback function that is internal to
 * the rtc_sw implimentation.  this struct should not be used directly.
 */
struct rtc_sw_callback_data {
    struct tasklet_struct task;
    void (*orig_func)(unsigned long data);
    unsigned long orig_data;
};

/*!
 * Holds the rtc_sw_timer request.
 *
 * @note this should be treated like a black box by those using the rtc_sw_task API.
 */
struct rtc_sw_request {
    /* stored in ticks */
    unsigned long interval;       /* normal alarm interval */
    unsigned long remain;         /* remaining time in interval */    

    int type;                     /* strict, fuzzy, or kernel API timer */
    
    atomic_t transaction;         /* The count of the number of transactions made*/
    atomic_t cookie;              /* The value of transaction as of the last valid interrrupt*/
    unsigned int running;         /* Number of outstanding interrupts against this timer*/
    unsigned int pending;         /* Number of interrupts against this timer still needing to be finished*/
    unsigned int next_to_run;     /* This indicates that this is the next timer to go off*/
    
    
    /* Doubly Linked List of requests*/
    struct rtc_sw_request *prev;  
    struct rtc_sw_request *next;
    
    union
    {
        struct task_struct *process;  /* calling process */
        struct rtc_sw_callback_data task_data;   /* tasklet related information to be scheduled when timer goes off */
    } callback_data;
};

typedef struct rtc_sw_request rtc_sw_task_t;


/*!
 * Initialize the rtc_sw_task_t structure. This must be called before any other
 * rtc_sw APIs are called for ptask.  It may only be called one per ptask structure
 * unless rtc_sw_kill is called on that ptask first.
 *
 * @param ptask pointer to the uninitialised task structure.
 * @param func the function to be called when the timer expires.
 * @param data a user defined data value that will be passed to func when the timer expires.
 */
void rtc_sw_task_init(rtc_sw_task_t * ptask, void (*func)(unsigned long data), unsigned long data);

/*!
 * Stop a task that is running on any CPU.  The difference between this and rtc_sw_task_stop
 * is that this will block until the task is no longer in use anywhere in the system.
 * Because it blocks this may NOT be called from interrupt context.  Once task is
 * killed no rtc_sw API may be called with it until rtc_sw_task_init has been called on it
 * again.
 */
void rtc_sw_task_kill(rtc_sw_task_t * task);

/*!
 * Stop a running task. This may be called from interrupt context, but it is not
 * guaranteed that the memory associated with task may be deallocated.
 *
 * @param task a pointer to the rtc_sw task that should be stopped
 * @return the number of milliseconds left until this timer expires.  If it
 * has already been run a value of 0 will be returned.
 * 
 * @note this may be called from interrupt context.
 */
unsigned long rtc_sw_task_stop(rtc_sw_task_t * task);

/*!
 * @return the number of milliseconds left until this timer expires.
 *
 * @note this may be called from interrupt context.
 */
unsigned long rtc_sw_task_time_left(rtc_sw_task_t * task);

/*!
 * Schedule a task to be run offset ms from now
 *
 * @param offset the number of ms from now to run task.
 * @param task the task to be run.
 * 
 * @note this may be called from interrupt context.
 */
void rtc_sw_task_schedule(unsigned long offset, rtc_sw_task_t * task);

#endif /* __KERNEL__ */

#endif /* _LINUX_RTC_H_ */
