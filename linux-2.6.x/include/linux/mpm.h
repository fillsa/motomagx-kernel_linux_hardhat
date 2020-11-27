/*
 * Copyright 2006-2008 Motorola, Inc.
 *
 * This code is licensed under LGPL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA"
 */

/*
 * Revision History:
 *
 * Date        Author    Comment
 * 10/04/2006  Motorola  Initial version.
 * 12/04/2006  Motorola  Improved DSM time reporting for debug
 * 01/18/2007  Motorola  Tie DSM to IDLE and reduce IOI timeout to 10ms
 * 01/28/2007  Motorola  Add long IOI timeout API for SD Card
 * 02/06/2007  Motorola  Resolve race condition in DSM.
 * 04/12/2007  Motorola  Check in idle for busy drivers/modules.
 * 03/31/2008  Motorola  Add function declaration of mpm_set_awake_state.
 */

#ifndef LINUX_MPM_H
#define LINUX_MPM_H

/*
 * ioctl operations
 */
#include <linux/ioctl.h>
#include <asm/types.h>

#define MPM_MS_PER_SECOND       (1000)
#define MPM_MS_PER_MINUTE       (60 * MPM_MS_PER_SECOND)
#define MPM_MS_PER_HOUR         (60 * MPM_MS_PER_MINUTE)
#define MPM_MS_PER_DAY          (24 * MPM_MS_PER_HOUR)

#define MPM_FAILED_DEV_NAME_LEN  64
#define MPM_TRANS_ERR_CALLER_LEN 64
#define MPM_DRIVER_NAME_SIZE     64

typedef unsigned long mpm_op_t;

/*
 * kernel event definitions for our ezx platform
 */
typedef struct {
    unsigned short type;	/*	What type of events.	*/
    unsigned short kind;	/*	What kind, or sub-type of events.*/
    unsigned int info;
} mpm_event_t;

/*
 * MPM sleep interval struct.
 * Note:  This struct uses flexible array members; this requires that
 * int si[ ] be last in the member list for this struct.
 */
typedef struct {
    int cnt;
    int si[ ];
} mpm_sleepinterval_t;

/*
 * MAX_PMMODES is the number of supported power management modes.
 * This define tracks the processor modes that are defined elsewhere
 * (e.g., DSM_MODE).
 *
 * The PMMODE_ defines must start with 0 so they can be used as
 * indexes.  The processor modes do not.
 */
#define PMMODE_WAIT	        0
#define PMMODE_DOZE	        1
#define PMMODE_STOP	        2
#define PMMODE_DSM	        3
#define MAX_PMMODES             4

#define MAX_MODE_TIMES	        10      /* Max # of times that we save for each mode */
#define MAX_INT_LINES 	        64      /* Max # of interrupts sources for each mode */
#define MAX_DURATION_BUCKETS 	11      /* Max # of duration buckets */
#define MAX_TRANS_ERRORS        10      /* Max # of transition errors that we save */
#define MAX_IDLE_DELAYS_SAVED   10      /* Max # of busy driver idle delays */
                                        /*   that we save */

/*
 * The mpm_driver_info_t structure contains the information that the
 * static part of the MPM driver maintains about the drivers that
 * have registered with the MPM via mpm_register_with_mpm.
 */
typedef struct
{
    int             registered_count;       /* Count of registered drivers */
    int             busy_count;             /* Count of busy drivers */
    int             size_in_bits;           /* Size in bits of each busy */
                                            /*   and registered bitarray */
    unsigned long * registered_p;           /* Registered driver bitarray */
    unsigned long * busy_p;                 /* Busy driver bitarray */
    int             total_name_size;        /* Size in bytes of all driver */
					    /*   names, including terminating */
					    /*   NULLs */
    char **         name_arr;               /* Array of pointers to driver */
					    /*   names */
} mpm_driver_info_t;

/*
 * The mpm_driver_stats_t structure contains statistics about the
 * drivers.  This structure only exists if lpm stats have been enabled
 * (see LPM_STATS_ENABLED()).
 */
struct mpm_driver_stats
{
    __u32           total_idle_delay_count; /* Number of times we delayed */
                                            /*   DSM because of busy dvrs */
    __u32           busy_count_indx;        /* Current index into busy_arr */
    unsigned long * busy_arr[MAX_IDLE_DELAYS_SAVED]; /* Array of pointers */
                                            /*   to bitarrays of most recent */
                                            /*   idle delays. */
    __u32           timestamp[MAX_IDLE_DELAYS_SAVED]; /* Timestamp of most */
					    /*   recent idle delays. */
};
typedef struct mpm_driver_stats mpm_driver_stats_t;

/*
 * The mpm_mode_info structure contains information about each supported
 * power management mode.
 */
struct mpm_mode_info
{
    __u32   count;			 /* # of times mode was entered */
    __u32   indx;			 /* indx into arrays for this mode */
    __u32   total_time;			 /* total time spent in this */
					 /* mode in millisecs */
    __u32   maxduration;	         /* max duration for this mode */
    __u32   minduration;	         /* min duration for this mode */
    __u32   duration[MAX_MODE_TIMES];	 /* most recent durations for */
					 /* this mode in millisecs */
    __u32   duration_bucket[MAX_DURATION_BUCKETS]; /* One bucket for each */
					 /* duration range. */
    __u32   avic_nipndl[MAX_MODE_TIMES]; /* pending intrs at the time */
					 /* this mode was exited */
    __u32   avic_nipndh[MAX_MODE_TIMES]; /* pending intrs at the time */
					 /* this mode was exited */
    __u32   interrupt_count[MAX_INT_LINES]; /* count of how many times */
					 /* each interrupt woke us up */
					 /* out of this mode */
    __u32   timestamp[MAX_MODE_TIMES];   /* This is the time when each */
					 /* duration ended for this mode. */
					 /* The time is in ms from boot. */
};
typedef struct mpm_mode_info mpm_mode_info_t;

/*
 * The mpm_transition_errors structure contains the information that
 * we keep whenever we detect a state machine transition error.
 */
struct mpm_transition_errors
{
    __u32   action;                      /* requested action at time of error */
    __u32   oldstate;                    /* current state at time of error */
    __u32   timestamp;                   /* timestamp (millisecs) at time of error */
    char    caller[MPM_TRANS_ERR_CALLER_LEN];
};
typedef struct mpm_transition_errors mpm_transition_errors_t;

/*
 * The mpm_stats structure contains all of the information that we
 * gather about low power modes.
 */
struct mpm_stats
{
    __u32   stats_enabled;              /* 0 => disabled, 1 => enabled */
    __u32   stat_start_time;            /* time (millisecs) when stats */
					/* started or reset */
    __u32   cur_time;                   /* current time (millisecs) */
                                        /* only used when sending to user space */
    
    /* counters for low power modes statistics */
    __u32   suspend_device_fail_count;
    __u32   suspend_cancel_count;
    __u32   suspend_delay_ioi_count;
    __u32   suspend_delay_pj_count;
    __u32   failed_dsm_count;
    __u32   state_transition_error_count;
    __u32   state_transition_error_indx;
    mpm_transition_errors_t transition_errors[MAX_TRANS_ERRORS];
    
    /* mode information for each power management mode */
    mpm_mode_info_t pmmode[MAX_PMMODES];

    __u32   total_suspend_time_interrupts_blocked;
    __u32   last_suspend_time_interrupts_blocked;
    __u32   total_suspend_time;
    __u32   last_suspend_time;

    __u32   device_suspend_start_time;
    __u32   device_suspend_done_time;
    __u32   interrupt_disable_time;
    __u32   interrupt_enable_time;

    char    failed_dev_name[MPM_FAILED_DEV_NAME_LEN];
    __u32   failed_dev_name_len;

    mpm_driver_stats_t driver_stats;
    mpm_driver_info_t  driver_info;
};
typedef struct mpm_stats mpm_stats_t;

/*
 * mpmstats_req_t is the structure that contains the data that
 * the caller must provide for the MPM_STATS_IOC_GET_MPMSTATS
 * ioctl.  The caller must first get the size with the
 * MPM_STATS_IOC_GET_MPMSTATS_SIZE ioctl and provide a buffer of at
 * least that size.
 */
typedef struct {
    mpm_stats_t *addr;	/* User address where stats are written */
    int          size;  /* Size of the provided memory */
} mpmstats_req_t;

/*
 * MPM desense values struct.
 * This structure is used by user space programs to provide the
 * desense information to the MPM driver.
 */
#define MPM_DESENSE_INFO_VERSION_1		1
#define MPM_DESENSE_INFO_VERSION_LATEST		MPM_DESENSE_INFO_VERSION_1
typedef struct {
    int version;				/* struct version */
    unsigned short ap_core_normal_pll_ppm;	/* PPM desense for normal mode */
    unsigned short ap_core_turbo_pll_ppm;	/* PPM desense for turbo mode */
    unsigned short ap_usb_pll_ppm;		/* PPM desense for USB */
} mpm_desense_info_t;

#define MPM_DEVICE_NAME         "mpm"
#define MPM_STATS_DEVICE_NAME   "mpm_stats"

/* device name string used to open the MPM device */
#define MPM_DEV_NAME            "/dev/"MPM_DEVICE_NAME
#define MPM_STATS_DEV_NAME      "/dev/"MPM_STATS_DEVICE_NAME

/* event types */
#define	MPM_EVENT_DEVICE	0x2		/*	Device event.	*/

/* event kinds */ 

#define EVENT_DEV_ACCS		0x0
#define EVENT_DEV_BT		0x1
#define EVENT_DEV_TS		0x2
#define EVENT_DEV_KEY		0x3
#define EVENT_DEV_RTC		0x4
#define EVENT_DEV_FLIP		0x5
#define EVENT_DEV_ICL		0x6
#define EVENT_DEV_USB	        0x7
#define EVENT_DEV_SLIDER	0x8

/* event info */

#define DEV_OFF			0x0
#define DEV_ON			0x1

#define DEV_DETACH              0x0
#define DEV_ATTACH              0x1

typedef unsigned long mpm_phoneattr_t;

/*
 * These defined attributes are used within mpm driver's ioctl, 
 * which provides information about the phone.  
 */
#define MPMA_FLIP              0x00000001
#define MPMA_SLIDER            0x00000002
#define MPMA_CANDYBAR          0x00000004
#define MPMA_MASK              ~(MPMA_FLIP | MPMA_SLIDER | MPMA_CANDYBAR)


/*
 * some new io control commands
 */
#define MPM_IOC_GET_NUMOP           _IOR('A',  4, int)
#define MPM_IOC_GETALL_OP           _IOR('A',  5, mpm_op_t)
#define MPM_IOC_SET_OP              _IOW('A',  6, mpm_op_t)
#define MPM_IOC_GET_NUMSLPMODES     _IOR('A',  7, int)
#define MPM_IOC_SET_SLEEP_INTERVALS _IOW('A',  8, mpm_sleepinterval_t)
#define MPM_IOC_AWAKE_REQ	    _IO ('A',  9)
#define MPM_IOC_GETCUR_OP           _IOR('A', 10, mpm_op_t)
#define MPM_IOC_REFLASH		    _IO ('A', 12)
#define MPM_IOC_INSERT_DEV_EVENT    _IOW('A', 13, mpm_event_t)
#define MPM_IOC_PRINT_OPSTAT        _IO ('A', 14)
#define MPM_IOC_START_OPSTAT        _IO ('A', 15)
#define MPM_IOC_STOP_OPSTAT         _IO ('A', 16)
#define MPM_IOC_INITIATE_SLEEP      _IO ('A', 17)
#define MPM_IOC_GET_PHONEATTR       _IOR('A', 18, mpm_phoneattr_t)
#define MPM_IOC_CONFIG_DESENSE      _IOR('A', 19, mpm_desense_info_t)

/*
 * ioctls for the mpm_stats device.
 */
#define MPM_STATS_IOC_GET_MPMSTATS       _IOR('B',  1, mpmstats_req_t)
#define MPM_STATS_IOC_GET_MPMSTATS_SIZE  _IOR('B',  2, int)

#ifdef __KERNEL__

#ifdef MPM_DEBUG
#define MPM_DPRINTK(format, args...)	printk(format, ##args)
#else
#define MPM_DPRINTK(format, args...)
#endif

/* Round up x to the next multiple of y. */
#define roundup(x, y) ((((x)+((y)-1))/(y))*(y))

/*
 * MPM callback function types
 */
typedef int (*mpm_callback_t)(void);

#ifdef CONFIG_MOT_FEAT_PM_STATS
/*
 * Test point event type
 */
typedef unsigned long mpm_test_point_t;

/*
 * Test point event definitions
 */
enum {
    MPM_TEST_DEVICE_SUSPEND_START,
    MPM_TEST_DEVICE_SUSPEND_DONE,
    MPM_TEST_DEVICE_SUSPEND_INTERRUPTS_DISABLED_START,
    MPM_TEST_DEVICE_SUSPEND_INTERRUPTS_DISABLED_DONE,
    MPM_TEST_DEVICE_SUSPEND_FAILED,
    MPM_TEST_SUSPEND_CANCELLED_BY_USER_ACTIVITY,
    MPM_TEST_SUSPEND_DELAYED_BY_IOI,
    MPM_TEST_SUSPEND_DELAYED_BY_PJ,
    MPM_TEST_DSM_ENTER,
    MPM_TEST_WAIT_EXIT,
    MPM_TEST_STOP_EXIT,
    MPM_TEST_DSM_EXIT,
    MPM_TEST_SLEEP_ATTEMPT_FAILED,
    MPM_TEST_STATE_TRANSITION_ERROR,
    MPM_TEST_SUSPEND_DELAYED_BY_BUSY_DRIVERS,
};

/*
 * Test point callback function types
 */
typedef void(*mpm_test_callback_t)(int i, ...);

#ifdef CONFIG_ARCH_MXC
/*
 * cpumode2indx converts cpu modes (e.g. DSM_MODE) to
 * a linear value that can be used as an indx in the
 * pmmode array in the mpm_stats structure.
 * indx2cpumode converts in the other direction.
 */
#define cpumode2indx(mode)      ((mode)-111)
#define indx2cpumode(indx)      ((indx)+111)
#endif /* CONFIG_ARCH_MXC */


extern mpm_stats_t *mpmsp;
extern spinlock_t mpmsplk;

extern mpm_driver_info_t *mpmdip;
extern spinlock_t mpm_driver_info_lock;

/*
 * Macro to control collection of low power mode statistics.  If low
 * power mode statistics are enabled, then we have allocated the mpmsp
 * global variable.  Otherwise, it's NULL.
 */
#define LPM_STATS_ENABLED() (mpmsp != NULL)

/*
 * Gating mechanism for collection of lpm stats
 */

#define MPM_REPORT_TEST_POINT(num_args, args...) \
            do { \
                if (LPM_STATS_ENABLED()) \
                    mpm_report_test_point(num_args, ##args); \
            } while(0)

#else

#define LPM_STATS_ENABLED() (0)
#define MPM_REPORT_TEST_POINT(num_args, args...)

#endif   /* CONFIG_MOT_FEAT_PM_STATS */

/*
 * The following macros are useful for calculating the field sizes,
 * offsets into, and addresses of the dynamic portion of the
 * mpm_stats_t.  The kernel keeps its information and statistics
 * in scattered structures that it allocates individually.  The
 * MPM_STATS_IOC_GET_MPMSTATS ioctl returns the mpm_stats_t
 * structure immediately followed by the dynamically-sized data
 * for the bitarrays and other information.  The following
 * macros provide the offsets and sizes of the dynamic data
 * that is returned via ioctl.
 *
 * The sizeof_* macros return the size in bytes of the requested data.
 *
 * The offsetof_* macros return the number of bytes after the
 * beginning of the mpm_stats_t structure where the data is found.
 *
 * The k_*_addr macros return a properly-casted pointer to the
 * requested data based on how the kernel stores the data.
 * The u_*_addr macros return a properly-casted pointer to the
 * requested data based on how the data is presented to the user
 * via ioctl.
 */
#define mpm_bitarray_size_in_bytes(size_in_bits) \
            (((size_t)(size_in_bits)) / (BITS_PER_LONG / sizeof(unsigned long)))

#define sizeof_registered_bitarray(size_in_bits) \
            ((size_t)(mpm_bitarray_size_in_bytes(size_in_bits)))
#define sizeof_busy_bitarray(size_in_bits) \
            ((size_t)(mpm_bitarray_size_in_bytes(size_in_bits)))
#define sizeof_name_ptr_array(size_in_bits) \
            ((size_t)((size_in_bits) * sizeof(char *)))
#define sizeof_name_strings(total_name_size) \
            ((size_t)(roundup((total_name_size), sizeof(unsigned long))))
#define sizeof_busy_arr_bitarrays(size_in_bits) \
            ((size_t)(mpm_bitarray_size_in_bytes(size_in_bits) * \
		      MAX_IDLE_DELAYS_SAVED))

#define offsetof_registered_bitarray(size_in_bits) \
            ((size_t)0)
#define offsetof_busy_bitarray(size_in_bits) \
            ((size_t)(offsetof_registered_bitarray(size_in_bits) + \
                      sizeof_registered_bitarray(size_in_bits)))
#define offsetof_name_ptr_array(size_in_bits) \
            ((size_t)(offsetof_busy_bitarray(size_in_bits) + \
                      sizeof_busy_bitarray(size_in_bits)))
#define offsetof_name_strings(size_in_bits) \
            ((size_t)(offsetof_name_ptr_array(size_in_bits) + \
                      sizeof_name_ptr_array(size_in_bits)))
#define offsetof_busy_arr_bitarrays(size_in_bits,total_name_size) \
            ((size_t)(offsetof_name_strings(size_in_bits) + \
                      sizeof_name_strings(total_name_size)))

/*
 * The following macros provide the address of a given piece of data.
 * There are two flavors of addresses: k_* and u_*.  The u_* macros
 * provide the address given the address of the beginning of the
 * mpm_stats_t structure.  The k_* macros provide the address without
 * any initial offset.
 */
#define registered_bitarray_addr(sp,bias,size_in_bits) \
            ((unsigned long *) \
             (((char *)(sp)) + (bias) + \
                       offsetof_registered_bitarray(size_in_bits)))
#define k_registered_bitarray_addr(sp,size_in_bits) \
            registered_bitarray_addr((sp),0,(size_in_bits))
#define u_registered_bitarray_addr(sp,size_in_bits) \
            registered_bitarray_addr((sp),sizeof(mpm_stats_t),(size_in_bits))

#define busy_bitarray_addr(sp,bias,size_in_bits) \
            ((unsigned long *) \
             (((char *)(sp)) + (bias) + \
		       offsetof_busy_bitarray(size_in_bits)))
#define k_busy_bitarray_addr(sp,size_in_bits) \
            busy_bitarray_addr((sp),0,(size_in_bits))
#define u_busy_bitarray_addr(sp,size_in_bits) \
            busy_bitarray_addr((sp),sizeof(mpm_stats_t),(size_in_bits))

#define name_ptr_array_addr(sp,bias,size_in_bits) \
            ((char **) \
             (((char *)(sp)) + (bias) + \
		       offsetof_name_ptr_array(size_in_bits)))
#define k_name_ptr_array_addr(sp,size_in_bits) \
            name_ptr_array_addr((sp),0,(size_in_bits))
#define u_name_ptr_array_addr(sp,size_in_bits) \
            name_ptr_array_addr((sp),sizeof(mpm_stats_t),(size_in_bits))

#define name_strings_addr(sp,bias,size_in_bits) \
            ((char *) \
             (((char *)(sp)) + (bias) + \
		       offsetof_name_strings(size_in_bits)))
#define u_name_strings_addr(sp,size_in_bits) \
            name_strings_addr((sp),sizeof(mpm_stats_t),(size_in_bits))

#define busy_arr_bitarrays_addr(sp,bias,size_in_bits,total_name_size) \
            ((unsigned long *) \
             (((char *)(sp)) + (bias) + \
		       offsetof_busy_arr_bitarrays((size_in_bits), \
                                                   (total_name_size))))
#define u_busy_arr_bitarrays_addr(sp,size_in_bits,total_name_size) \
            busy_arr_bitarrays_addr((sp),sizeof(mpm_stats_t), \
				    (size_in_bits), (total_name_size))


/* Struct to pass the callback functions between the static 
 * and dynamic portions of mpm driver.
 */
struct mpm_callback_fns
{
    mpm_callback_t periodic_jobs_done;
    mpm_callback_t periodic_jobs_started;
    mpm_callback_t ready_to_sleep;
    mpm_callback_t handle_ioi;
    mpm_callback_t handle_long_ioi;
    mpm_callback_t set_awake_state;
    mpm_callback_t start_sleep;
    mpm_callback_t resume_from_sleep;
#ifdef CONFIG_MOT_FEAT_PM_STATS
    mpm_test_callback_t report_test_point;
#endif
};

extern int mpm_queue_empty(void);
extern void mpm_get_queued_event(mpm_event_t *);
extern wait_queue_head_t mpm_wq;
extern void mpm_event_notify(short type, short kind, int info);
extern mpm_callback_t mpm_periodic_jobs_done;
extern mpm_callback_t mpm_periodic_jobs_started;
extern mpm_callback_t mpm_set_awake_state;
extern mpm_callback_t mpm_ready_to_sleep;
extern mpm_callback_t mpm_handle_ioi;
extern mpm_callback_t mpm_handle_long_ioi;
extern mpm_callback_t mpm_start_sleep;
extern mpm_callback_t mpm_resume_from_sleep;
extern int mpm_getnum_op(void);
extern int mpm_set_op (mpm_op_t arg);
extern mpm_op_t mpm_getcur_op(void);
extern const mpm_op_t* const mpm_getall_op(void);
extern void mpm_print_opstat(char *buf, int buflen);
extern void mpm_start_opstat(void);
extern void mpm_stop_opstat(void);
extern void mpm_callback_register(struct mpm_callback_fns *);
extern void mpm_callback_deregister(void);
extern int  mpm_register_with_mpm(const char *name);
extern void mpm_unregister_with_mpm(int driver_num);
extern int  mpm_driver_advise(int driver_num, int advice);
extern int  mpm_num_busy_drivers(void);
extern int  mpm_get_extra_size(void);
#ifdef CONFIG_MOT_FEAT_PM_DESENSE
extern int mpm_panic_with_invalid_desense(void);
#endif
#ifdef CONFIG_MOT_FEAT_PM_STATS
extern mpm_test_callback_t mpm_report_test_point;
extern void mpm_lpm_stat_ctl(int);
extern void mpm_reset_lpm_stats(void);
#endif

/* Return values from mpm_ready_to_sleep function.
 */
#define MPM_GO_TO_SLEEP 1     /* Indicates that the system can go into low power mode */ 
#define MPM_DO_NOT_SLEEP 0    /* Indicates system should not go into low power mode */

/*
 * MPM advice requests
 *
 * Each driver that registers can indicate to the MPM when it
 * is busy using the following advices.
 *
 * These #defines are used with mpm_driver_advise.
 */
#define MPM_ADVICE_DRIVER_IS_NOT_BUSY  0
#define MPM_ADVICE_DRIVER_IS_BUSY      1

/*
 * Maximum number of events that can be stored in
 *  Queue are 20
 */
#define MPM_MAX_EVENTS		  21

typedef struct {
    int		event_head;
    int		event_tail;
    mpm_event_t	events[MPM_MAX_EVENTS];
} mpm_devq_t;


#endif /* __KERNEL__ */

#endif	/* LINUX_MPM_H */
