/*
 * linux/arch/arm/mach-pnx4008/rtc.c
 *
 * Real-Time Clock driver for PNX4008
 *
 * Authors: Vladimir Shebordayev, Semyon Podkorytov <source@mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/ioctl.h>

#include <asm/rtc.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/arch/pm.h>

#ifndef RTC_IRQF
#define RTC_IRQF 0x80
#endif

#ifndef RTC_PF
#define RTC_PF 0x40
#endif

#ifndef RTC_AF
#define RTC_AF 0x20
#endif

#ifndef RTC_UF
#define RTC_UF 0x10
#endif

#define RTC_VA_BASE	IO_ADDRESS(PNX4008_RTC_BASE)

#define RTC_UCOUNT	0x00
#define RTC_DCOUNT	0x04
#define RTC_MATCH0	0x08
#define RTC_MATCH1	0x0C
#define RTC_CTRL	0x10
#define RTC_INTSTAT	0x14
#define RTC_KEY		0x18
#define RTC_SRAM	0x80

#define RTC_CTRL_MATCH0_IE	0	/* Bits */
#define RTC_CTRL_MATCH1_IE	1
#define RTC_CTRL_MATCH0_ONSW	2
#define RTC_CTRL_MATCH1_ONSW	3
#define RTC_CTRL_STOP		6
#define RTC_INTSTAT_MATCH0 	0
#define RTC_INTSTAT_MATCH1 	1
#define RTC_INTSTAT_ONSW   	2
#define RTC_SRAM_ALARM_EVERY_DAY	0
#define RTC_SRAM_ALARM_IE		1

#define RTC_KEY_VALUE	0xB5C13F27

extern int (*set_rtc) (void);

static DEFINE_RAW_SPINLOCK(rtc_lock);

static inline u32 pnx4008_rtc_read(u32 reg)
{
	return __raw_readl(RTC_VA_BASE + reg);
}

static inline void pnx4008_rtc_write(u32 reg, u32 val)
{
	__raw_writel(val, RTC_VA_BASE + reg);
}

static inline int pnx4008_rtc_read_bit(u32 reg, int bit)
{
	return (pnx4008_rtc_read(reg) & (1UL << bit)) != 0;
}

static inline void pnx4008_rtc_write_bit(u32 reg, int bit, int enable)
{
	u32 val = pnx4008_rtc_read(reg);
	if (enable)
		pnx4008_rtc_write(reg, val | (1UL << bit));
	else
		pnx4008_rtc_write(reg, val & ~(1UL << bit));
}

static inline void pnx4008_rtc_ll_stop(void)
{
	pnx4008_rtc_write_bit(RTC_CTRL, RTC_CTRL_STOP, 1);
}

static inline void pnx4008_rtc_ll_run(void)
{
	pnx4008_rtc_write_bit(RTC_CTRL, RTC_CTRL_STOP, 0);
}

static inline void pnx4008_rtc_ll_clear_intstat(void)
{
	pnx4008_rtc_write_bit(RTC_INTSTAT, RTC_INTSTAT_MATCH1, 1);
}

static inline unsigned long pnx4008_rtc_ll_time(void)
{
	return pnx4008_rtc_read(RTC_UCOUNT);
}

static inline void pnx4008_rtc_ll_set_time(unsigned long time)
{
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	pnx4008_rtc_ll_stop();
	pnx4008_rtc_write(RTC_UCOUNT, time);
	pnx4008_rtc_ll_run();
	udelay(31);
	spin_unlock_irqrestore(&rtc_lock, flags);
}

static inline int pnx4008_rtc_ll_alarm_every_day(void)
{
	return pnx4008_rtc_read_bit(RTC_SRAM, RTC_SRAM_ALARM_EVERY_DAY);
}

static inline void pnx4008_rtc_ll_set_alarm_every_day(int enable)
{
	pnx4008_rtc_write_bit(RTC_SRAM, RTC_SRAM_ALARM_EVERY_DAY, enable);
}

static inline int pnx4008_rtc_ll_alarm_ie(void)
{
	return pnx4008_rtc_read_bit(RTC_CTRL, RTC_CTRL_MATCH1_IE);
}

static inline void pnx4008_rtc_ll_set_alarm_ie(int enable)
{
	pnx4008_rtc_write_bit(RTC_CTRL, RTC_CTRL_MATCH1_IE, enable);
	if (!enable)
		pnx4008_rtc_ll_clear_intstat();	/* ? */
}

static inline void pnx4008_rtc_ll_save_and_set_alarm_ie(int enable)
{
	pnx4008_rtc_write_bit(RTC_SRAM, RTC_SRAM_ALARM_IE, enable);
	pnx4008_rtc_ll_set_alarm_ie(enable);
}

static inline void pnx4008_rtc_ll_restore_alarm_ie(void)
{
	int enable = pnx4008_rtc_read_bit(RTC_SRAM, RTC_SRAM_ALARM_IE);
	pnx4008_rtc_ll_set_alarm_ie(enable);
}

static inline int pnx4008_rtc_ll_alarm_onsw(void)
{
	return pnx4008_rtc_read_bit(RTC_CTRL, RTC_CTRL_MATCH1_ONSW);
}

static inline void pnx4008_rtc_ll_set_alarm_onsw(int enable)
{
	pnx4008_rtc_write_bit(RTC_CTRL, RTC_CTRL_MATCH1_ONSW, enable);
	pnx4008_rtc_write(RTC_KEY, enable ? RTC_KEY_VALUE : 0);
}

static inline unsigned long pnx4008_rtc_ll_alarm_time(void)
{
	return pnx4008_rtc_read(RTC_MATCH1);
}

static inline void pnx4008_rtc_ll_set_alarm_time(unsigned long time)
{
	pnx4008_rtc_write(RTC_MATCH1, time);
	pnx4008_rtc_ll_restore_alarm_ie();
}

static void pnx4008_rtc_update_alarm_time(void)
{
	u32 m = 24 * 60 * 60;
	u32 atime = pnx4008_rtc_ll_alarm_time();
	u32 ctime = pnx4008_rtc_ll_time();
	if (atime <= ctime) {
		atime = ctime + m - (ctime - atime) % m;
		pnx4008_rtc_ll_set_alarm_time(atime);
	}
}

static void pnx4008_rtc_read_time(struct rtc_time *tm)
{
	rtc_time_to_tm(pnx4008_rtc_ll_time(), tm);
}

static int pnx4008_rtc_set_time(struct rtc_time *tm)
{
	int ret;
	unsigned long time;

	ret = rtc_tm_to_time(tm, &time);
	if (ret)
		return ret;

	pnx4008_rtc_ll_set_time(time);

	return 0;
}

static void pnx4008_rtc_read_alarm(struct rtc_wkalrm *alrm)
{
	unsigned long time;

	if (pnx4008_rtc_ll_alarm_every_day())
		pnx4008_rtc_update_alarm_time();
	time = pnx4008_rtc_ll_alarm_time();
	rtc_time_to_tm(time, &alrm->time);
	alrm->enabled = pnx4008_rtc_ll_alarm_onsw();
	alrm->pending = time > pnx4008_rtc_ll_time();
}

static int pnx4008_rtc_set_alarm(struct rtc_wkalrm *alrm)
{
	struct rtc_time atm, ctm;
	unsigned long atime;
	int ret;

	if (rtc_periodic_alarm(&alrm->time)) {
		pnx4008_rtc_read_time(&ctm);
		rtc_next_alarm_time(&atm, &ctm, &alrm->time);
		ret = rtc_tm_to_time(&atm, &atime);
		if (ret)
			return ret;
		pnx4008_rtc_ll_set_alarm_onsw(0);
		pnx4008_rtc_ll_set_alarm_every_day(1);
		pnx4008_rtc_ll_set_alarm_time(atime);
		/* Here an alarm interrupt may happen */
		pnx4008_rtc_update_alarm_time();
	} else {
		ret = rtc_tm_to_time(&alrm->time, &atime);
		if (ret)
			return ret;
		pnx4008_rtc_ll_set_alarm_onsw(alrm->enabled);
		pnx4008_rtc_ll_set_alarm_every_day(0);
		pnx4008_rtc_ll_set_alarm_time(atime);
	}

	return 0;
}

static irqreturn_t pnx4008_rtc_interrupt(int irq, void *dev_id,
					 struct pt_regs *regs)
{
	if (pnx4008_rtc_ll_alarm_every_day())
		pnx4008_rtc_update_alarm_time();
	else
		pnx4008_rtc_ll_set_alarm_ie(0);

	pnx4008_rtc_ll_clear_intstat();

	rtc_update(1, RTC_AF);

	return IRQ_HANDLED;
}

static int pnx4008_rtc_ioctl(unsigned int type, unsigned long arg)
{
	switch (type) {
	case RTC_AIE_ON:
		if (pnx4008_rtc_ll_alarm_every_day())
			pnx4008_rtc_update_alarm_time();
		pnx4008_rtc_ll_save_and_set_alarm_ie(1);
		return 0;
	case RTC_AIE_OFF:
		pnx4008_rtc_ll_save_and_set_alarm_ie(0);
		return 0;
	default:
		return -ENOTTY;
	}
}

static struct rtc_ops pnx4008_rtc_ops = {
	.owner = THIS_MODULE,
	.ioctl = pnx4008_rtc_ioctl,
	.read_time = pnx4008_rtc_read_time,
	.set_time = pnx4008_rtc_set_time,
	.read_alarm = pnx4008_rtc_read_alarm,
	.set_alarm = pnx4008_rtc_set_alarm,
};

static int pnx4008_set_rtc(void)
{
	pnx4008_rtc_ll_set_time(xtime.tv_sec);
	return 1;
}

#ifdef CONFIG_PREEMPT_HARDIRQS
#define RTC_INT_FLAGS (0)
#else
#define RTC_INT_FLAGS (SA_INTERRUPT)
#endif

static int __init pnx4008_rtc_init(void)
{
	int ret;

	ret = request_irq(RTC_INT, pnx4008_rtc_interrupt, RTC_INT_FLAGS,
			  "rtc", NULL);
	if (ret) {
		printk(KERN_ERR "PNX4008 RTC: failed to request IRQ\n");
		ret = -EBUSY;
		goto error;
	}

	ret = register_rtc(&pnx4008_rtc_ops);
	if (ret) {
		printk(KERN_ERR "PNX4008 RTC: failed to register \n");
		goto err_reg;
	}

	xtime.tv_sec = pnx4008_rtc_read(RTC_UCOUNT);
	set_rtc = (int (*)(void))pnx4008_set_rtc;

	if (pnx4008_rtc_ll_alarm_every_day())
		pnx4008_rtc_update_alarm_time();
	pnx4008_rtc_ll_restore_alarm_ie();

	/*setup wakeup interrupt */
	start_int_set_rising_edge(SE_RTC_INT);
	start_int_ack(SE_RTC_INT);
	start_int_umask(SE_RTC_INT);

	printk(KERN_INFO "PNX4008 RTC Driver\n");
	return 0;
      err_reg:
	free_irq(RTC_INT, NULL);
      error:
	return ret;
}

__initcall(pnx4008_rtc_init);
