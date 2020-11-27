/*
 *	TI OMAP Real Time Clock interface for Linux	
 *
 *	Copyright (C) 2003 MontaVista Software, Inc.
 *      Author: George G. Davis <gdavis@mvista.com> or <source@mvista.com>
 *
 *	Initially based on linux-2.4.20/drivers/char/rtc.c
 *	Copyright (C) 1996 Paul Gortmaker
 *
 *	This driver allows use of the real time clock (built into
 *	nearly all computers) from user space. It exports the /dev/rtc
 *	interface supporting various ioctl() and also the
 *	/proc/driver/rtc pseudo-file for status information.
 *
 *	The ioctls can be used to set the interrupt behaviour from the
 *	RTC via IRQs. Then the /dev/rtc	interface can be used to make
 *	use of RTC interrupts, be they time update or alarm based.
 *
 *	The /dev/rtc interface will block on reads until an interrupt
 *	has been received. If a RTC interrupt has already happened,
 *	it will output an unsigned long and then block. The output value
 *	contains the interrupt status in the low byte and the number of
 *	interrupts since the last read in the remaining high bytes. The 
 *	/dev/rtc interface can also be used with the select(2) call.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	Based on other minimal char device drivers, like Alan's
 *	watchdog, Ted's random, etc. etc.
 *
 * Change Log :
 *      v1.0    <gdavis@mvista.com> Initial version based on rtc.c v1.10e
 *              <ramakrishnan@india.ti.com> Added support for 2.6 kernel, 
 *                  - changed the return value of the interrupt handler
 */

#define RTC_VERSION		"1.0"

/*
 *	Note that *all* calls to CMOS_READ and CMOS_WRITE are done with
 *	interrupts disabled.
 *	REVISIT: Elaborate on OMAP1510 TRM 15uS BUSY access rule.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/rtc.h>

#include <linux/interrupt.h>
#include <linux/rtc.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include "omap-rtc.h"

extern raw_spinlock_t rtc_lock;


/* OMAP RTC register access macros: */

#define CMOS_READ(addr)		omap_readb(addr)
#define CMOS_WRITE(val, addr)	omap_writeb(val, addr)


/* Local BCD/BIN conversion macros: */
#ifdef BCD_TO_BIN
#undef BCD_TO_BIN
#endif
#define BCD_TO_BIN(val)	((val)=((val)&15) + ((val)>>4)*10)
 
#ifdef BIN_TO_BCD
#undef BIN_TO_BCD
#endif
#define BIN_TO_BCD(val)	((val)=(((val)/10)<<4) + (val)%10)


/*
 *	We sponge a minor off of the misc major. No need slurping
 *	up another valuable major dev number for this. If you add
 *	an ioctl, make sure you don't conflict with SPARC's RTC
 *	ioctls.
 */

static struct fasync_struct *rtc_async_queue;

static DECLARE_WAIT_QUEUE_HEAD(rtc_wait);

static ssize_t rtc_read(struct file *file, char *buf,
			size_t count, loff_t *ppos);

static int rtc_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg);

static unsigned int rtc_poll(struct file *file, poll_table *wait);

void get_rtc_time (struct rtc_time *rtc_tm);
static void get_rtc_alm_time (struct rtc_time *alm_tm);

static void set_rtc_irq_bit(unsigned char bit);
static void mask_rtc_irq_bit(unsigned char bit);

static inline unsigned char rtc_is_updating(void);

static int rtc_read_proc(char *page, char **start, off_t off,
                         int count, int *eof, void *data);

/*
 *	Bits in rtc_status. (7 bits of room for future expansion)
 */

#define RTC_IS_OPEN		0x01	/* means /dev/rtc is in use	*/

/*
 * REVISIT: fix this comment:
 * rtc_status is never changed by rtc_interrupt, and ioctl/open/close is
 * protected by the big kernel lock.
 */
static unsigned long rtc_status = 0;	/* bitmapped status byte.	*/
static unsigned long rtc_irq_data = 0;	/* our output to the world	*/

/*
 *	If this driver ever becomes modularised, it will be really nice
 *	to make the epoch retain its value across module reload...
 */

static unsigned long epoch = 1900;	/* year corresponding to 0x00	*/

static const unsigned char days_in_mo[] = 
{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*
 *	A very tiny interrupt handler. It runs with SA_INTERRUPT set.
 */

static irqreturn_t rtc_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	/*
	 *	Either an alarm interrupt or update complete interrupt.
	 *	We store the status in the low byte and the number of
	 *	interrupts received since the last read in the remainder
	 *	of rtc_irq_data.
	 */

	spin_lock (&rtc_lock);

	rtc_irq_data += 0x100;
	rtc_irq_data &= ~0xff;
	rtc_irq_data |= CMOS_READ(OMAP_RTC_STATUS_REG);

	if (rtc_irq_data & OMAP_RTC_STATUS_ALARM)
		CMOS_WRITE(OMAP_RTC_STATUS_ALARM, OMAP_RTC_STATUS_REG);

	spin_unlock (&rtc_lock);

	/* Now do the rest of the actions */
	wake_up_interruptible(&rtc_wait);	

	kill_fasync (&rtc_async_queue, SIGIO, POLL_IN);
	return IRQ_HANDLED;
}

/*
 *	Now all the various file operations that we export.
 */

static ssize_t rtc_read(struct file *file, char *buf,
			size_t count, loff_t *ppos)
{
	DECLARE_WAITQUEUE(wait, current);
	unsigned long data;
	ssize_t retval;
	
	if (count < sizeof(unsigned long))
		return -EINVAL;

	add_wait_queue(&rtc_wait, &wait);
	set_current_state(TASK_INTERRUPTIBLE);

	for (;;) {
		spin_lock_irq (&rtc_lock);
		data = rtc_irq_data;
		if (data != 0) {
			rtc_irq_data = 0;
			break;
		}
		spin_unlock_irq (&rtc_lock);

		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto out;
		}
		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			goto out;
		}
		schedule();
	}

	spin_unlock_irq (&rtc_lock);
	retval = put_user(data, (unsigned long *)buf); 
	if (!retval)
		retval = sizeof(unsigned long); 
 out:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&rtc_wait, &wait);

	return retval;
}

static int rtc_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		     unsigned long arg)
{
	struct rtc_time wtime; 

	switch (cmd) {
	case RTC_AIE_OFF:	/* Mask alarm int. enab. bit	*/
	{
		mask_rtc_irq_bit(OMAP_RTC_INTERRUPTS_IT_ALARM);
		return 0;
	}
	case RTC_AIE_ON:	/* Allow alarm interrupts.	*/
	{
		set_rtc_irq_bit(OMAP_RTC_INTERRUPTS_IT_ALARM);
		return 0;
	}
	case RTC_UIE_OFF:	/* Mask ints from RTC updates.	*/
	{
		mask_rtc_irq_bit(OMAP_RTC_INTERRUPTS_IT_TIMER);
		return 0;
	}
	case RTC_UIE_ON:	/* Allow ints for RTC updates.	*/
	{
		set_rtc_irq_bit(OMAP_RTC_INTERRUPTS_IT_TIMER);
		return 0;
	}
	case RTC_ALM_READ:	/* Read the present alarm time */
	{
		/*
		 * This returns a struct rtc_time. Reading >= 0xc0
		 * means "don't care" or "match all". Only the tm_hour,
		 * tm_min, and tm_sec values are filled in.
		 */
		memset(&wtime, 0, sizeof(struct rtc_time));
		get_rtc_alm_time(&wtime);
		break; 
	}
	case RTC_ALM_SET:	/* Store a time into the alarm */
	{
		struct rtc_time alm_tm;
		unsigned char mon, day, hrs, min, sec, leap_yr;
		unsigned int yrs;

		if (copy_from_user(&alm_tm, (struct rtc_time*)arg,
				   sizeof(struct rtc_time)))
			return -EFAULT;

		yrs = alm_tm.tm_year + 1900;
		mon = alm_tm.tm_mon + 1;
		day = alm_tm.tm_mday;
		hrs = alm_tm.tm_hour;
		min = alm_tm.tm_min;
		sec = alm_tm.tm_sec;

		if (yrs < 1970)
			return -EINVAL;

		leap_yr = ((!(yrs % 4) && (yrs % 100)) || !(yrs % 400));

		if ((mon > 12) || (day == 0))
			return -EINVAL;

		if (day > (days_in_mo[mon] + ((mon == 2) && leap_yr)))
			return -EINVAL;
			
		if ((hrs >= 24) || (min >= 60) || (sec >= 60))
			return -EINVAL;

		if ((yrs -= epoch) > 255)    /* They are unsigned */
			return -EINVAL;

		if (yrs > 169) {
			return -EINVAL;
		}

		if (yrs >= 100)
			yrs -= 100;

		BIN_TO_BCD(sec);
		BIN_TO_BCD(min);
		BIN_TO_BCD(hrs);
		BIN_TO_BCD(day);
		BIN_TO_BCD(mon);
		BIN_TO_BCD(yrs);

		spin_lock_irq(&rtc_lock);
		CMOS_WRITE(yrs, OMAP_RTC_ALARM_YEARS_REG);
		CMOS_WRITE(mon, OMAP_RTC_ALARM_MONTHS_REG);
		CMOS_WRITE(day, OMAP_RTC_ALARM_DAYS_REG);
		CMOS_WRITE(hrs, OMAP_RTC_ALARM_HOURS_REG);
		CMOS_WRITE(min, OMAP_RTC_ALARM_MINUTES_REG);
		CMOS_WRITE(sec, OMAP_RTC_ALARM_SECONDS_REG);
		spin_unlock_irq(&rtc_lock);

		return 0;
	}
	case RTC_RD_TIME:	/* Read the time/date from RTC	*/
	{
		memset(&wtime, 0, sizeof(struct rtc_time));
		get_rtc_time(&wtime);
		break;
	}
	case RTC_SET_TIME:	/* Set the RTC */
	{
		struct rtc_time rtc_tm;
		unsigned char mon, day, hrs, min, sec, leap_yr;
		unsigned char save_control;
		unsigned int yrs;

		if (!capable(CAP_SYS_TIME))
			return -EACCES;

		if (copy_from_user(&rtc_tm, (struct rtc_time*)arg,
				   sizeof(struct rtc_time)))
			return -EFAULT;

		yrs = rtc_tm.tm_year + 1900;
		mon = rtc_tm.tm_mon + 1;   /* tm_mon starts at zero */
		day = rtc_tm.tm_mday;
		hrs = rtc_tm.tm_hour;
		min = rtc_tm.tm_min;
		sec = rtc_tm.tm_sec;

		if (yrs < 1970)
			return -EINVAL;

		leap_yr = ((!(yrs % 4) && (yrs % 100)) || !(yrs % 400));

		if ((mon > 12) || (day == 0))
			return -EINVAL;

		if (day > (days_in_mo[mon] + ((mon == 2) && leap_yr)))
			return -EINVAL;

		if ((hrs >= 24) || (min >= 60) || (sec >= 60))
			return -EINVAL;

		if ((yrs -= epoch) > 255)    /* They are unsigned */
			return -EINVAL;

		if (yrs > 169) {
			return -EINVAL;
		}

		if (yrs >= 100)
			yrs -= 100;

		BIN_TO_BCD(sec);
		BIN_TO_BCD(min);
		BIN_TO_BCD(hrs);
		BIN_TO_BCD(day);
		BIN_TO_BCD(mon);
		BIN_TO_BCD(yrs);

		spin_lock_irq(&rtc_lock);
		save_control = CMOS_READ(OMAP_RTC_CTRL_REG);
		CMOS_WRITE((save_control & ~OMAP_RTC_CTRL_STOP),
			   OMAP_RTC_CTRL_REG);
		CMOS_WRITE(yrs, OMAP_RTC_YEARS_REG);
		CMOS_WRITE(mon, OMAP_RTC_MONTHS_REG);
		CMOS_WRITE(day, OMAP_RTC_DAYS_REG);
		CMOS_WRITE(hrs, OMAP_RTC_HOURS_REG);
		CMOS_WRITE(min, OMAP_RTC_MINUTES_REG);
		CMOS_WRITE(sec, OMAP_RTC_SECONDS_REG);
		CMOS_WRITE((save_control | OMAP_RTC_CTRL_STOP),
			   OMAP_RTC_CTRL_REG);
		spin_unlock_irq(&rtc_lock);

		return 0;
	}
	case RTC_EPOCH_READ:	/* Read the epoch.	*/
	{
		return put_user (epoch, (unsigned long *)arg);
	}
	case RTC_EPOCH_SET:	/* Set the epoch.	*/
	{
		/* 
		 * There were no RTC clocks before 1900.
		 */
		if (arg < 1900)
			return -EINVAL;

		if (!capable(CAP_SYS_TIME))
			return -EACCES;

		epoch = arg;
		return 0;
	}
	default:
#if	!defined(CONFIG_ARCH_OMAP)
		return -ENOTTY;
#else
		return -EINVAL;
#endif
	}
	return copy_to_user((void *)arg, &wtime, sizeof wtime) ? -EFAULT : 0;
}

/*
 *	We enforce only one user at a time here with the open/close.
 *	Also clear the previous interrupt data on an open, and clean
 *	up things on a close.
 */

/* We use rtc_lock to protect against concurrent opens. So the BKL is not
 * needed here. Or anywhere else in this driver. */
static int rtc_open(struct inode *inode, struct file *file)
{
	spin_lock_irq (&rtc_lock);

	if(rtc_status & RTC_IS_OPEN)
		goto out_busy;

	rtc_status |= RTC_IS_OPEN;

	rtc_irq_data = 0;
	spin_unlock_irq (&rtc_lock);
	return 0;

out_busy:
	spin_unlock_irq (&rtc_lock);
	return -EBUSY;
}

static int rtc_fasync (int fd, struct file *filp, int on)

{
	return fasync_helper (fd, filp, on, &rtc_async_queue);
}

static int rtc_release(struct inode *inode, struct file *file)
{
	unsigned char tmp;

	/*
	 * Turn off all interrupts once the device is no longer
	 * in use, and clear the data.
	 */

	spin_lock_irq(&rtc_lock);
	tmp = CMOS_READ(OMAP_RTC_INTERRUPTS_REG);
	tmp &=  ~OMAP_RTC_INTERRUPTS_IT_ALARM;
	tmp &=  ~OMAP_RTC_INTERRUPTS_IT_TIMER;
	CMOS_WRITE(tmp, OMAP_RTC_INTERRUPTS_REG);
	spin_unlock_irq(&rtc_lock);

	if (file->f_flags & FASYNC) {
		rtc_fasync (-1, file, 0);
	}

	spin_lock_irq (&rtc_lock);
	rtc_irq_data = 0;
	spin_unlock_irq (&rtc_lock);

	/* No need for locking -- nobody else can do anything until this rmw
	 * is committed, and we don't implement timer support in omap-rtc.
	 */
	rtc_status &= ~RTC_IS_OPEN;
	return 0;
}

/* Called without the kernel lock - fine */
static unsigned int rtc_poll(struct file *file, poll_table *wait)
{
	unsigned long l;

	poll_wait(file, &rtc_wait, wait);

	spin_lock_irq (&rtc_lock);
	l = rtc_irq_data;
	spin_unlock_irq (&rtc_lock);

	if (l != 0)
		return POLLIN | POLLRDNORM;
	return 0;
}

/*
 *	The various file operations we support.
 */

static struct file_operations rtc_fops = {
	owner:		THIS_MODULE,
	llseek:		no_llseek,
	read:		rtc_read,
	poll:		rtc_poll,
	ioctl:		rtc_ioctl,
	open:		rtc_open,
	release:	rtc_release,
	fasync:		rtc_fasync,
};

static struct miscdevice rtc_dev=
{
	RTC_MINOR,
	"rtc",
	&rtc_fops
};

static int __init rtc_init(void)
{
	if (!request_region(OMAP_RTC_VIRT_BASE, OMAP_RTC_SIZE,
			    rtc_dev.name)) {
		printk(KERN_ERR "%s: RTC I/O port %d is not free.\n",
		       rtc_dev.name, OMAP_RTC_VIRT_BASE);
		return -EIO;
	}

	if (CMOS_READ(OMAP_RTC_STATUS_REG) & OMAP_RTC_STATUS_POWER_UP) {
		printk(KERN_WARNING "%s: RTC power up reset detected.\n",
		       rtc_dev.name);
		/* Clear OMAP_RTC_STATUS_POWER_UP */
		CMOS_WRITE(OMAP_RTC_STATUS_POWER_UP, OMAP_RTC_STATUS_REG);
	}

	if (CMOS_READ(OMAP_RTC_STATUS_REG) & OMAP_RTC_STATUS_ALARM) {
		printk(KERN_WARNING "%s: Clearing RTC ALARM interrupt.\n",
		       rtc_dev.name);
		/* Clear OMAP_RTC_STATUS_ALARM */
		CMOS_WRITE(OMAP_RTC_STATUS_ALARM, OMAP_RTC_STATUS_REG);
	}

	if (!(CMOS_READ(OMAP_RTC_CTRL_REG) & OMAP_RTC_CTRL_STOP)) {
		printk(KERN_INFO "%s: Enabling RTC.\n", rtc_dev.name);
		CMOS_WRITE(OMAP_RTC_CTRL_STOP, OMAP_RTC_CTRL_REG);
	}

#ifdef CONFIG_PREEMPT_HARDIRQS
	if (request_irq(INT_RTC_TIMER, rtc_interrupt, 0,
#else
	if (request_irq(INT_RTC_TIMER, rtc_interrupt, SA_INTERRUPT,
#endif
			rtc_dev.name, NULL)) {
		printk(KERN_ERR "%s: RTC timer interrupt IRQ%d is not free.\n",
		       rtc_dev.name, INT_RTC_TIMER);
		release_region(OMAP_RTC_VIRT_BASE, OMAP_RTC_SIZE);
		return -EIO;
	}

#ifdef CONFIG_PREEMPT_HARDIRQS
	if (request_irq(INT_RTC_ALARM, rtc_interrupt, 0,
#else
	if (request_irq(INT_RTC_ALARM, rtc_interrupt, SA_INTERRUPT,
#endif
	                "omap-rtc alarm", NULL)) {
		printk(KERN_ERR "%s: RTC alarm interrupt IRQ%d is not free.\n",
		       rtc_dev.name, INT_RTC_ALARM);
		release_region(OMAP_RTC_VIRT_BASE, OMAP_RTC_SIZE);
		return -EIO;
	}

	misc_register(&rtc_dev);
	create_proc_read_entry ("driver/rtc", 0, 0, rtc_read_proc, NULL);

	printk(KERN_INFO "Real Time Clock Driver v" RTC_VERSION "\n");

	return 0;
}

static void __exit rtc_exit (void)
{
	free_irq (INT_RTC_TIMER, NULL);
	free_irq (INT_RTC_ALARM, NULL);

	remove_proc_entry ("driver/rtc", NULL);
	misc_deregister(&rtc_dev);

	release_region (OMAP_RTC_VIRT_BASE, OMAP_RTC_SIZE);
}

/*
 *	Info exported via "/proc/driver/rtc".
 */

static int rtc_proc_output (char *buf)
{
#define YN(value) ((value) ? "yes" : "no")
#define NY(value) ((value) ? "no" : "yes")
	char *p;
	struct rtc_time tm;

	p = buf;

	get_rtc_time(&tm);

	/*
	 * There is no way to tell if the luser has the RTC set for local
	 * time or for Universal Standard Time (GMT). Probably local though.
	 */
	p += sprintf(p,
		     "rtc_time\t: %02d:%02d:%02d\n"
		     "rtc_date\t: %04d-%02d-%02d\n"
	 	     "rtc_epoch\t: %04lu\n",
		     tm.tm_hour, tm.tm_min, tm.tm_sec,
		     tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, epoch);

	get_rtc_alm_time(&tm);

	/*
	 * We implicitly assume 24hr mode here. Alarm values >= 0xc0 will
	 * match any value for that particular field. Values that are
	 * greater than a valid time, but less than 0xc0 shouldn't appear.
	 */
	p += sprintf(p,
		     "alarm_time\t: %02d:%02d:%02d\n"
		     "alarm_date\t: %04d-%02d-%02d\n",
		     tm.tm_hour, tm.tm_min, tm.tm_sec,
		     tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

	p += sprintf(p,
		     "BCD\t\t: %s\n"
		     "24hr\t\t: %s\n"
		     "alarm_IRQ\t: %s\n"
		     "update_IRQ\t: %s\n"
		     "update_rate\t: %ud\n",
		     YN(1),
		     YN(1),
		     YN(CMOS_READ(OMAP_RTC_INTERRUPTS_REG) &
			OMAP_RTC_INTERRUPTS_IT_ALARM),
		     YN(CMOS_READ(OMAP_RTC_INTERRUPTS_REG) &
			OMAP_RTC_INTERRUPTS_IT_TIMER),
		     CMOS_READ(OMAP_RTC_INTERRUPTS_REG) & 3 /* REVISIT */);

	return  p - buf;
#undef YN
#undef NY
}

static int rtc_read_proc(char *page, char **start, off_t off,
                         int count, int *eof, void *data)
{
        int len = rtc_proc_output (page);
        if (len <= off+count) *eof = 1;
        *start = page + off;
        len -= off;
        if (len>count) len = count;
        if (len<0) len = 0;
        return len;
}

/*
 * Returns true if a clock update is in progress
 */
/* FIXME shouldn't this be above rtc_init to make it fully inlined? */
static inline unsigned char rtc_is_updating(void)
{
	unsigned char uip;

	spin_lock_irq(&rtc_lock);
	uip = (CMOS_READ(OMAP_RTC_STATUS_REG) & OMAP_RTC_STATUS_BUSY);
	spin_unlock_irq(&rtc_lock);
	return uip;
}

void get_rtc_time(struct rtc_time *rtc_tm)
{
	unsigned char ctrl;

	/* REVISIT: Fix this comment!!!
	 * read RTC once any update in progress is done. The update
	 * can take just over 2ms. We wait 10 to 20ms. There is no need to
	 * to poll-wait (up to 1s - eeccch) for the falling edge of OMAP_RTC_STATUS_BUSY.
	 * If you need to know *exactly* when a second has started, enable
	 * periodic update complete interrupts, (via ioctl) and then 
	 * immediately read /dev/rtc which will block until you get the IRQ.
	 * Once the read clears, read the RTC time (again via ioctl). Easy.
	 */

#if	0 /* REVISIT: This need to do as the TRM says. */
	unsigned long uip_watchdog = jiffies;
	if (rtc_is_updating() != 0)
		while (jiffies - uip_watchdog < 2*HZ/100) {
			barrier();
			cpu_relax();
		}
#endif

	/*
	 * Only the values that we read from the RTC are set. We leave
	 * tm_wday, tm_yday and tm_isdst untouched. Even though the
	 * RTC has RTC_DAY_OF_WEEK, we ignore it, as it is only updated
	 * by the RTC when initially set to a non-zero value.
	 */
	spin_lock_irq(&rtc_lock);
	rtc_tm->tm_sec = CMOS_READ(OMAP_RTC_SECONDS_REG);
	rtc_tm->tm_min = CMOS_READ(OMAP_RTC_MINUTES_REG);
	rtc_tm->tm_hour = CMOS_READ(OMAP_RTC_HOURS_REG);
	rtc_tm->tm_mday = CMOS_READ(OMAP_RTC_DAYS_REG);
	rtc_tm->tm_mon = CMOS_READ(OMAP_RTC_MONTHS_REG);
	rtc_tm->tm_year = CMOS_READ(OMAP_RTC_YEARS_REG);
	ctrl = CMOS_READ(OMAP_RTC_CTRL_REG);
	spin_unlock_irq(&rtc_lock);

	BCD_TO_BIN(rtc_tm->tm_sec);
	BCD_TO_BIN(rtc_tm->tm_min);
	BCD_TO_BIN(rtc_tm->tm_hour);
	BCD_TO_BIN(rtc_tm->tm_mday);
	BCD_TO_BIN(rtc_tm->tm_mon);
	BCD_TO_BIN(rtc_tm->tm_year);

	/*
	 * Account for differences between how the RTC uses the values
	 * and how they are defined in a struct rtc_time;
	 */
	if ((rtc_tm->tm_year += (epoch - 1900)) <= 69)
		rtc_tm->tm_year += 100;

	rtc_tm->tm_mon--;
}
EXPORT_SYMBOL(get_rtc_time);

static void get_rtc_alm_time(struct rtc_time *alm_tm)
{
	unsigned char ctrl;

	spin_lock_irq(&rtc_lock);
	alm_tm->tm_sec = CMOS_READ(OMAP_RTC_ALARM_SECONDS_REG);
	alm_tm->tm_min = CMOS_READ(OMAP_RTC_ALARM_MINUTES_REG);
	alm_tm->tm_hour = CMOS_READ(OMAP_RTC_ALARM_HOURS_REG);
	alm_tm->tm_mday = CMOS_READ(OMAP_RTC_ALARM_DAYS_REG);
	alm_tm->tm_mon = CMOS_READ(OMAP_RTC_ALARM_MONTHS_REG);
	alm_tm->tm_year = CMOS_READ(OMAP_RTC_ALARM_YEARS_REG);
	ctrl = CMOS_READ(OMAP_RTC_CTRL_REG);
	spin_unlock_irq(&rtc_lock);

	BCD_TO_BIN(alm_tm->tm_sec);
	BCD_TO_BIN(alm_tm->tm_min);
	BCD_TO_BIN(alm_tm->tm_hour);
	BCD_TO_BIN(alm_tm->tm_mday);
	BCD_TO_BIN(alm_tm->tm_mon);
	BCD_TO_BIN(alm_tm->tm_year);

	/*
	 * Account for differences between how the RTC uses the values
	 * and how they are defined in a struct rtc_time;
	 */
	if ((alm_tm->tm_year += (epoch - 1900)) <= 69)
		alm_tm->tm_year += 100;

	alm_tm->tm_mon--;
}

/*
 * Used to disable/enable UIE and AIE interrupts.
 */

static void mask_rtc_irq_bit(unsigned char bit)
{
	unsigned char val;

	spin_lock_irq(&rtc_lock);
	val = CMOS_READ(OMAP_RTC_INTERRUPTS_REG);
	val &=  ~bit;
	CMOS_WRITE(val, OMAP_RTC_INTERRUPTS_REG);
	rtc_irq_data = 0;
	spin_unlock_irq(&rtc_lock);
}

static void set_rtc_irq_bit(unsigned char bit)
{
	unsigned char val;

	spin_lock_irq(&rtc_lock);
	val = CMOS_READ(OMAP_RTC_INTERRUPTS_REG);
	val |= bit;
	CMOS_WRITE(val, OMAP_RTC_INTERRUPTS_REG);
	rtc_irq_data = 0;
	spin_unlock_irq(&rtc_lock);
}

MODULE_AUTHOR("George G. Davis");
MODULE_LICENSE("GPL");

module_init(rtc_init);
module_exit(rtc_exit);
