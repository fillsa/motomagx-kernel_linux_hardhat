/*
 *  linux/kernel/panic.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Copyright (C) 2006-2008 Motorola, Inc.
 *
 * Date         Author          Comment
 * 10/2006      Motorola        Added panic block dump support
 * 11/2006	Motorola	Including power_ic_kernel.h instead
 * 				of power_ic.h
 * 11/2006      Motorola        Changed timestamp to use secure clock
 * 11/2006      Motorola        Added memory dump support
 * 02/2007      Motorola        Fixing PC value in panic
 * 05/2007      Motorola        Emit build label in kernel panic text
 * 03/2008	Motorola	Add mem print log in panic
 * 03/2008	Motorola	Make memory log more flexable
 */

/*
 * This function is used through-out the kernel (including mm and fs)
 * to indicate a major problem.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/notifier.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/interrupt.h>
#include <linux/nmi.h>
#ifdef CONFIG_MOT_FEAT_KPANIC
#include <linux/ks_otp_api.h>
#ifdef CONFIG_MOT_FEAT_FB_PANIC_TEXT
#include <linux/motfb.h>
#endif /* CONFIG_MOT_FEAT_FB_PANIC_TEXT */
#ifdef CONFIG_MOT_POWER_IC_ATLAS
#include <linux/power_ic_kernel.h>
#endif /* CONFIG_MOT_POWER_IC_ATLAS */
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <asm/rtc.h>
#endif /* CONFIG_MOT_FEAT_KPANIC */

#define MOTO_BLD_FLAG "motobldlabel"

#ifdef CONFIG_MOT_FEAT_LOG_SCHEDULE_EVENTS
#include <linux/mem-log.h>
#endif /* CONFIG_MOT_FEAT_LOG_SCHEDULE_EVENTS */

#ifdef CONFIG_MOT_FEAT_KPANIC
extern int meminfo_read_proc(char *, char **, off_t, int, int *, void *);
extern void dump_kpanic(char *);
extern void kick_wd(void);
int kpanic_in_progress = 0;
#ifdef CONFIG_MOT_FEAT_MEMDUMP
extern void dump_rawmem(void);
static int memdump_enabled = 0;
extern char dumppart_name[64];
#endif /* CONFIG_MOT_FEAT_MEMDUMP */
#ifdef CONFIG_MOT_FEAT_FB_PANIC_TEXT
extern int fb_panic_text(struct fb_info * fbi, const char * panic_text,
                         long panic_len, int do_timer);
#endif /* CONFIG_MOT_FEAT_FB_PANIC_TEXT */
#endif /* CONFIG_MOT_FEAT_KPANIC */

int panic_timeout;
int panic_on_oops;
int tainted;

#ifdef CONFIG_MOT_FEAT_PRINT_PC_ON_PANIC
int aprdataprinted;
#endif

EXPORT_SYMBOL(panic_timeout);

struct notifier_block *panic_notifier_list;

EXPORT_SYMBOL(panic_notifier_list);

static int __init panic_setup(char *str)
{
	panic_timeout = simple_strtoul(str, NULL, 0);
	return 1;
}
__setup("panic=", panic_setup);

static long no_blink(long time)
{
	return 0;
}

/* Returns how long it waited in ms */
long (*panic_blink)(long time);
EXPORT_SYMBOL(panic_blink);

#ifdef CONFIG_MOT_FEAT_MEMDUMP
static int __init memdump_setup(char *str)
{
        char *s;

        if ((str != NULL) && (*str != '\0') && (s = strrchr(str, '/')) != 0) {
                strlcpy(dumppart_name, ++s, sizeof(dumppart_name));
                memdump_enabled = 1;
        }
        return 1;
}

__setup("memdump=", memdump_setup);
#endif /* CONFIG_MOT_FEAT_MEMDUMP */

#ifdef CONFIG_MOT_FEAT_FB_PANIC_TEXT
static char fb_panic_text_buf[PANIC_MAX_STR_LEN] = "AP Kernel Panic";

void set_fb_panic_text(const char * msg)
{
        strncpy(fb_panic_text_buf, msg, PANIC_MAX_STR_LEN);

        fb_panic_text_buf[PANIC_MAX_STR_LEN - 1] = '\0';
}

EXPORT_SYMBOL(set_fb_panic_text);
#endif /* CONFIG_MOT_FEAT_FB_PANIC_TEXT */

/**
 *	panic - halt the system
 *	@fmt: The text string to print
 *
 *	Display a message, then perform cleanups. Functions in the panic
 *	notifier list are called after the filesystem cache is flushed (when possible).
 *
 *	This function never returns.
 */
 
NORET_TYPE void panic(const char * fmt, ...)
{
	long i;
	static char buf[1024];
	static char *wdog = "Watchdog";
	va_list args;
#if defined(CONFIG_ARCH_S390)
        unsigned long caller = (unsigned long) __builtin_return_address(0);
#endif
#ifdef CONFIG_MOT_FEAT_KPANIC
	struct timeval power_ic_time;
	struct rtc_time rtc_timestamp;
	struct timespec uptime;
	/* setting the default values to be most restrictive */
	unsigned char security_mode = MOT_SECURITY_MODE_PRODUCTION;
	unsigned char production_state = LAUNCH_SHIP;
	unsigned char bound_signature_state = BS_DIS_DISABLED;
	int buf_len = 0;
	int label_len = 0;
	char * label_start, * label_end;
#endif /* CONFIG_MOT_FEAT_KPANIC */
				
	bust_spinlocks(1);
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	printk(KERN_EMERG "Kernel panic - not syncing: %s\n",buf);

	bust_spinlocks(0);
#ifdef CONFIG_SMP
	smp_send_stop();
#endif

	notifier_call_chain(&panic_notifier_list, 0, buf);

	if (!panic_blink)
		panic_blink = no_blink;

	if (panic_timeout > 0)
	{
		/*
	 	 * Delay timeout seconds before rebooting the machine. 
		 * We can't use the "normal" timers since we just panicked..
	 	 */
		printk(KERN_EMERG "Rebooting in %d seconds..",panic_timeout);
		for (i = 0; i < panic_timeout*1000; ) {
			touch_nmi_watchdog();
			i += panic_blink(i);
			mdelay(1);
			i++;
		}
		/*
		 *	Should we run the reboot notifier. For the moment Im
		 *	choosing not too. It might crash, be corrupt or do
		 *	more harm than good for other reasons.
		 */
		machine_restart(NULL);
	}
#ifdef __sparc__
	{
		extern int stop_a_enabled;
		/* Make sure the user can actually press L1-A */
		stop_a_enabled = 1;
		printk(KERN_EMERG "Press L1-A to return to the boot prom\n");
	}
#endif
#if defined(CONFIG_ARCH_S390)
        disabled_wait(caller);
#endif

#ifdef CONFIG_MOT_FEAT_KPANIC
	mot_otp_get_mode(&security_mode, NULL);
	mot_otp_get_productstate(&production_state, NULL);
	mot_otp_get_boundsignaturestate(&bound_signature_state, NULL);
	/* do not update the kpanic flash partition if a double panic occurs, i.e. 
	   a panic from within a panic context */
	if (!kpanic_in_progress) {
		kpanic_in_progress = 1;

		/* reuse the static buffer declared above */
		memset(buf, 0, sizeof(buf));
		label_start = strstr(saved_command_line, MOTO_BLD_FLAG);
		if (label_start != NULL)
		{
			label_end = strchr(label_start, ' ');
			if (label_end == NULL)
			{
				label_end = label_start + strlen(label_start);
			}
			label_len = min(label_end -  label_start, sizeof(buf) - 1);
			memcpy(buf, label_start, label_len);
		}else{
			label_len = strlen(MOTO_BLD_FLAG"=UNKNOWN");
			label_len = min(label_len, sizeof(buf) - 1);
			memcpy(buf, MOTO_BLD_FLAG"=UNKNOWN", label_len);
		}
		buf_len = label_len;
		buf_len += snprintf(&(buf[buf_len]), sizeof(buf)-buf_len, "\nKernel panic - not syncing: ");
		va_start(args, fmt);
		buf_len += vsnprintf(&(buf[buf_len]), sizeof(buf)-buf_len, fmt, args);
		va_end(args);
		power_ic_rtc_get_time(&power_ic_time);
		rtc_time_to_tm((unsigned long)power_ic_time.tv_sec, &rtc_timestamp);
		do_posix_clock_monotonic_gettime(&uptime);
		/* displays current time (in ISO 8601 format) and uptime (in seconds) */
		snprintf(&(buf[buf_len]), sizeof(buf)-buf_len, ", Current Time = "
			"%d-%02d-%02dT%02d:%02d:%02dZ, Uptime = %lu.%03lu seconds\n", 
			rtc_timestamp.tm_year + 1900, rtc_timestamp.tm_mon + 1, 
			rtc_timestamp.tm_mday, rtc_timestamp.tm_hour, rtc_timestamp.tm_min, 
			rtc_timestamp.tm_sec, (unsigned long)uptime.tv_sec, 
			(unsigned long)(uptime.tv_nsec/USEC_PER_SEC));
		printk(KERN_EMERG "%s", buf);

		/* dump the entire printk buffer to flash */
		if ((security_mode == MOT_SECURITY_MODE_ENGINEERING) ||
		    (security_mode == MOT_SECURITY_MODE_NO_SECURITY) ||
		    ((security_mode == MOT_SECURITY_MODE_PRODUCTION) &&
		     (production_state == PRE_ACCEPTANCE_ACCEPTANCE) &&
		     (bound_signature_state == BS_DIS_ENABLED))) {

#ifdef CONFIG_MOT_FEAT_PRINT_PC_ON_PANIC
			{
				void* panic_caller_location;

				panic_caller_location = __builtin_return_address(0);
				if (aprdataprinted == 0) {
					printk(KERN_ERR "[APR]PanicPC: %p\n", panic_caller_location);
					aprdataprinted = 1;
				}
			}
#endif

			/* displays current memory statistics; the return value is being ignored */
			meminfo_read_proc(buf, NULL, 0, 0, NULL, NULL);
#ifdef CONFIG_MOT_FEAT_MEMDUMP
                        if (memdump_enabled) {
                                printk(KERN_EMERG "Writing all of raw memory to [%s] \n", dumppart_name);
                                printk("Original preempt count: %x\n", preempt_count());
                                if (preempt_count() >= 0)
                                        preempt_count() = 0;
                                local_irq_disable();
                                dump_rawmem();
                        }
                        else
                                printk(KERN_EMERG "Memory dump disabled\n");
#endif /* CONFIG_MOT_FEAT_MEMDUMP */

#ifdef CONFIG_MOT_FEAT_LOG_SCHEDULE_EVENTS
			/* reuse the static buffer declared above */
			memset(buf, 0, sizeof(buf));
			va_start(args, fmt);
			vsnprintf(buf, sizeof(buf), fmt, args);
			va_end(args);

			/* print out the event log if panic triggered by wdog timeout */
			if (strstr(buf, wdog))
				mem_print_log();
#endif /* CONFIG_MOT_FEAT_LOG_SCHEDULE_EVENTS */
			
			/* dump the printk log buffer to flash */
			dump_kpanic(NULL);
		}
		/* only dump the timestamp and panic string to flash */
		else {
			dump_kpanic(buf);
		}
	}
#ifdef CONFIG_MOT_POWER_IC_ATLAS
	/* write the panic reason code */
	if (power_ic_backup_memory_write(POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_PANIC, 1) < 0) {
		printk(KERN_EMERG "Error: Could not write the panic reason code\n");
	}
#endif /* CONFIG_MOT_POWER_IC_ATLAS */
	if ((security_mode == MOT_SECURITY_MODE_ENGINEERING) || (security_mode == MOT_SECURITY_MODE_NO_SECURITY)) {
#ifdef CONFIG_MOT_FEAT_FB_PANIC_TEXT
		/* display panic text to the screen */
		fb_panic_text(NULL, fb_panic_text_buf,
                        strnlen(fb_panic_text_buf, PANIC_MAX_STR_LEN), false);
#endif /* CONFIG_MOT_FEAT_FB_PANIC_TEXT */
	}
	else {
		/* fast reboot the system */
		sys_reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL);
	}
#endif /* CONFIG_MOT_FEAT_KPANIC */

	local_irq_enable();
	for (i = 0;;) {
#ifdef CONFIG_MOT_FEAT_KPANIC
		if (i==0) preempt_disable();
		/* tickle the watchdog timer */
		kick_wd();
#endif /* CONFIG_MOT_FEAT_KPANIC */
		i += panic_blink(i);
		mdelay(1);
		i++;
	}
}

EXPORT_SYMBOL(panic);

/**
 *	print_tainted - return a string to represent the kernel taint state.
 *
 *  'P' - Proprietary module has been loaded.
 *  'F' - Module has been forcibly loaded.
 *  'S' - SMP with CPUs not designed for SMP.
 *  'R' - User forced a module unload.
 *  'M' - Machine had a machine check experience.
 *  'B' - System has hit bad_page.
 *
 *	The string is overwritten by the next call to print_taint().
 */
 
const char *print_tainted(void)
{
	static char buf[20];
	if (tainted) {
		snprintf(buf, sizeof(buf), "Tainted: %c%c%c%c%c%c",
			tainted & TAINT_PROPRIETARY_MODULE ? 'P' : 'G',
			tainted & TAINT_FORCED_MODULE ? 'F' : ' ',
			tainted & TAINT_UNSAFE_SMP ? 'S' : ' ',
			tainted & TAINT_FORCED_RMMOD ? 'R' : ' ',
 			tainted & TAINT_MACHINE_CHECK ? 'M' : ' ',
			tainted & TAINT_BAD_PAGE ? 'B' : ' ');
	}
	else
		snprintf(buf, sizeof(buf), "Not tainted");
	return(buf);
}

void add_taint(unsigned flag)
{
	tainted |= flag;
}
EXPORT_SYMBOL(add_taint);
