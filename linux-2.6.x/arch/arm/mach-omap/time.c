/*
 * linux/arch/arm/mach-omap/time.c
 *
 * OMAP Timers
 *
 * Copyright (C) 2004 Nokia Corporation
 * Partial timer rewrite and additional VST timer support by
 * Tony Lindgen <tony@atomide.com> and
 * Tuukka Tikkanen <tuukka.tikkanen@elektrobit.com>
 *
 * MPU timer code based on the older MPU timer code for OMAP
 * Copyright (C) 2000 RidgeRun, Inc.
 * Author: Greg Lonnon <glonnon@ridgerun.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/kallsyms.h>

#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/leds.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/preempt.h>

/* Added for HRT support: */
#include <linux/time.h>
#include <linux/hrtime.h>
#include <linux/timex.h>
#include <asm/errno.h>
#include <asm/hardware.h>

struct sys_timer omap_timer;

#ifdef CONFIG_OMAP_MPU_TIMER

/*
 * ---------------------------------------------------------------------------
 * MPU timer
 * ---------------------------------------------------------------------------
 */
#define OMAP_MPU_TIMER1_BASE		(0xfffec500)
#define OMAP_MPU_TIMER2_BASE		(0xfffec600)
#define OMAP_MPU_TIMER3_BASE		(0xfffec700)
#define OMAP_MPU_TIMER_BASE		OMAP_MPU_TIMER1_BASE
#define OMAP_MPU_TIMER_OFFSET		0x100

#define MPU_TIMER_FREE			(1 << 6)
#define MPU_TIMER_CLOCK_ENABLE		(1 << 5)
#define MPU_TIMER_AR			(1 << 1)
#define MPU_TIMER_ST			(1 << 0)

/* cycles to nsec conversions taken from arch/i386/kernel/timers/timer_tsc.c,
 * converted to use kHz by Kevin Hilman */
/* convert from cycles(64bits) => nanoseconds (64bits)
 *  basic equation:
 *		ns = cycles / (freq / ns_per_sec)
 *		ns = cycles * (ns_per_sec / freq)
 *		ns = cycles * (10^9 / (cpu_khz * 10^3))
 *		ns = cycles * (10^6 / cpu_khz)
 *
 *	Then we use scaling math (suggested by george at mvista.com) to get:
 *		ns = cycles * (10^6 * SC / cpu_khz / SC
 *		ns = cycles * cyc2ns_scale / SC
 *
 *	And since SC is a constant power of two, we can convert the div
 *  into a shift.
 *			-johnstul at us.ibm.com "math is hard, lets go shopping!"
 */
static unsigned long cyc2ns_scale;
#define CYC2NS_SCALE_FACTOR 10 /* 2^10, carefully chosen */

static inline void set_cyc2ns_scale(unsigned long cpu_khz)
{
	cyc2ns_scale = (1000000 << CYC2NS_SCALE_FACTOR)/cpu_khz;
}

static inline unsigned long long __noinstrument cycles_2_ns(unsigned long long cyc)
{
	return (cyc * cyc2ns_scale) >> CYC2NS_SCALE_FACTOR;
}

/*
 * MPU_TICKS_PER_SEC must be an even number, otherwise machinecycles_to_usecs
 * will break. On P2, the timer count rate is 6.5 MHz after programming PTV
 * with 0. This divides the 13MHz input by 2, and is undocumented.
 */
#ifdef CONFIG_MACH_OMAP_PERSEUS2
/* REVISIT: This ifdef construct should be replaced by a query to clock
 * framework to see if timer base frequency is 12.0, 13.0 or 19.2 MHz.
 */
#define MPU_TICKS_PER_SEC		(13000000 / 2)
#else
#define MPU_TICKS_PER_SEC		(12000000 / 2)
#endif

#define MPU_TIMER_TICK_PERIOD		((MPU_TICKS_PER_SEC / HZ) - 1)

typedef struct {
	u32 cntl;			/* CNTL_TIMER, R/W */
	u32 load_tim;			/* LOAD_TIM,   W */
	u32 read_tim;			/* READ_TIM,   R */
} omap_mpu_timer_regs_t;

#define omap_mpu_timer_base(n)						\
((volatile omap_mpu_timer_regs_t*)IO_ADDRESS(OMAP_MPU_TIMER_BASE +	\
				 (n)*OMAP_MPU_TIMER_OFFSET))

int get_timer_base(int nr)
{
	volatile omap_mpu_timer_regs_t* timer = omap_mpu_timer_base(nr);
	printk(" cntl 0x%x load_tim 0x%x read_tim 0x%x \n",
	    timer->cntl, timer->load_tim, timer->read_tim);
	return 0;
}

EXPORT_SYMBOL(get_timer_base);

unsigned long __noinstrument omap_mpu_timer_read(int nr)
{
	volatile omap_mpu_timer_regs_t* timer = omap_mpu_timer_base(nr);
	return timer->read_tim;
}
EXPORT_SYMBOL(omap_mpu_timer_read);

void omap_mpu_timer_start(int nr, unsigned long load_val)
{
	volatile omap_mpu_timer_regs_t* timer = omap_mpu_timer_base(nr);

	timer->cntl = MPU_TIMER_CLOCK_ENABLE;
	udelay(1);
	timer->load_tim = load_val;
        udelay(1);
	timer->cntl = (MPU_TIMER_CLOCK_ENABLE | MPU_TIMER_AR | MPU_TIMER_ST);
}
EXPORT_SYMBOL(omap_mpu_timer_start);

unsigned long __noinstrument omap_mpu_timer_ticks_to_usecs(unsigned long nr_ticks)
{
	unsigned long long nsec;

	nsec = cycles_2_ns((unsigned long long)nr_ticks);
	return (unsigned long)nsec / 1000;
}

/*
 * Last processed system timer interrupt
 */
static unsigned long omap_mpu_timer_last = 0;

#ifdef CONFIG_KFI
static int mpu_timer_started = 0;

unsigned long __noinstrument machinecycles_to_usecs(unsigned long nr_ticks)
{
	return omap_mpu_timer_ticks_to_usecs(nr_ticks);
}

unsigned long __noinstrument do_getmachinecycles(void)
{
	return (mpu_timer_started ? ~omap_mpu_timer_read(0) : 0);
}
#endif  /* CONFIG_KFI */

/*
 * Returns elapsed usecs since last system timer interrupt
 */
static unsigned long omap_mpu_timer_gettimeoffset(void)
{
	unsigned long now = 0 - omap_mpu_timer_read(0);
	unsigned long elapsed = now - omap_mpu_timer_last;

	return omap_mpu_timer_ticks_to_usecs(elapsed);
}

/*
 * Elapsed time between interrupts is calculated using timer0.
 * Latency during the interrupt is calculated using timer1.
 * Both timer0 and timer1 are counting at 6MHz (P2 6.5MHz).
 */
static irqreturn_t omap_mpu_timer_interrupt(int irq, void *dev_id,
					struct pt_regs *regs)
{
	unsigned long now, latency;

	write_seqlock(&xtime_lock);
	now = 0 - omap_mpu_timer_read(0);
	latency = MPU_TICKS_PER_SEC / HZ - omap_mpu_timer_read(1);
	omap_mpu_timer_last = now - latency;
	timer_tick(regs);
	write_sequnlock(&xtime_lock);

	return IRQ_HANDLED;
}

static struct irqaction omap_mpu_timer_irq = {
	.name		= "mpu timer",
	.flags		= SA_NODELAY | SA_INTERRUPT,
	.handler	= omap_mpu_timer_interrupt
};

static unsigned long omap_mpu_timer1_overflows;
static irqreturn_t omap_mpu_timer1_interrupt(int irq, void *dev_id,
					     struct pt_regs *regs)
{
	omap_mpu_timer1_overflows++;
	return IRQ_HANDLED;
}

static struct irqaction omap_mpu_timer1_irq = {
	.name		= "mpu timer1 overflow",
	.flags		= SA_NODELAY | SA_INTERRUPT,
	.handler	= omap_mpu_timer1_interrupt
};

static __init void omap_init_mpu_timer(void)
{
	set_cyc2ns_scale(MPU_TICKS_PER_SEC / 1000);
	omap_timer.offset = omap_mpu_timer_gettimeoffset;
	setup_irq(INT_TIMER1, &omap_mpu_timer1_irq);
	setup_irq(INT_TIMER2, &omap_mpu_timer_irq);
	omap_mpu_timer_start(0, 0xffffffff);
	omap_mpu_timer_start(1, MPU_TIMER_TICK_PERIOD);

#ifdef CONFIG_KFI
	mpu_timer_started = 1;
#endif
}

/*
 * Scheduler clock - returns current time in nanosec units.
 */
unsigned long long sched_clock(void)
{
	unsigned long ticks = 0 - omap_mpu_timer_read(0);
	unsigned long long ticks64;

	ticks64 = omap_mpu_timer1_overflows;
	ticks64 <<= 32;
	ticks64 |= ticks;

	return cycles_2_ns(ticks64);
}

#endif	/* CONFIG_OMAP_MPU_TIMER */

#ifdef CONFIG_OMAP_32K_TIMER

#ifdef CONFIG_ARCH_OMAP1510
#error OMAP 32KHz timer does not currently work on 1510!
#endif

/*
 * ---------------------------------------------------------------------------
 * 32KHz OS timer
 *
 * This currently works only on 16xx, as 1510 does not have the continuous
 * 32KHz synchronous timer. The 32KHz synchronous timer is used to keep track
 * of time in addition to the 32KHz OS timer. Using only the 32KHz OS timer
 * on 1510 would be possible, but the timer would not be as accurate as
 * with the 32KHz synchronized timer.
 * ---------------------------------------------------------------------------
 */
#define OMAP_32K_TIMER_BASE		0xfffb9000
#define OMAP_32K_TIMER_CR		0x08
#define OMAP_32K_TIMER_TVR		0x00
#define OMAP_32K_TIMER_TCR		0x04

#define OMAP_32K_TICKS_PER_HZ		(32768 / HZ)
#if (32768 % HZ) != 0
/* We cannot ignore modulo.
 * Potential error can be as high as several percent.
 */
#define OMAP_32K_TICK_MODULO		(32768 % HZ)
static unsigned modulo_count = 0; /* Counts 1/HZ units */
#endif

/*
 * TRM says 1 / HZ = ( TVR + 1) / 32768, so TRV = (32768 / HZ) - 1
 * so with HZ = 100, TVR = 327.68.
 */
#define OMAP_32K_TIMER_TICK_PERIOD	((32768 / HZ) - 1)
#define MAX_SKIP_JIFFIES		25
#define TIMER_32K_SYNCHRONIZED		0xfffbc410

#define JIFFIES_TO_HW_TICKS(nr_jiffies, clock_rate)			\
				(((nr_jiffies) * (clock_rate)) / HZ)

static spinlock_t spinlock_32k = SPIN_LOCK_UNLOCKED;

static inline void omap_32k_timer_write(int val, int reg)
{
	omap_writew(val, reg + OMAP_32K_TIMER_BASE);
}

static inline unsigned long omap_32k_timer_read(int reg)
{
	return omap_readl(reg + OMAP_32K_TIMER_BASE) & 0xffffff;
}

/*
 * The 32KHz synchronized timer is an additional timer on 16xx.
 * It is always running.
 */
static inline unsigned long omap_32k_sync_timer_read(void)
{
	return omap_readl(TIMER_32K_SYNCHRONIZED);
}

static inline void omap_32k_timer_start(unsigned long load_val)
{
	omap_32k_timer_write(load_val, OMAP_32K_TIMER_TVR);
	omap_32k_timer_write(0x0f, OMAP_32K_TIMER_CR);
}

static inline void omap_32k_timer_stop(void)
{
	omap_32k_timer_write(0x0, OMAP_32K_TIMER_CR);
}

/*
 * Rounds down to nearest usec
 */
static inline unsigned long omap_32k_ticks_to_usecs(unsigned long ticks_32k)
{
	return (ticks_32k * 5*5*5*5*5*5) >> 9;
}

static unsigned long omap_32k_last_tick = 0;

/*
 * Returns elapsed usecs since last 32k timer interrupt
 */
static unsigned long omap_32k_timer_gettimeoffset(void)
{
	unsigned long now = omap_32k_sync_timer_read();
	return omap_32k_ticks_to_usecs(now - omap_32k_last_tick);
}

/*
 * Timer interrupt for 32KHz timer. When VST is enabled, this
 * function is also called from other interrupts to remove latency
 * issues with VST. In the VST case, we need to lock with irqsave.
 */
static irqreturn_t omap_32k_timer_interrupt(int irq, void *dev_id,
					    struct pt_regs *regs)
{
	unsigned long flags;
	unsigned long now;

	write_seqlock_irqsave(&xtime_lock, flags);
	now = omap_32k_sync_timer_read();

	while (now - omap_32k_last_tick >= OMAP_32K_TICKS_PER_HZ) {
#ifdef OMAP_32K_TICK_MODULO
		/* Modulo addition may put omap_32k_last_tick ahead of now
		 * and cause unwanted repetition of the while loop.
		 */
		if (unlikely(now - omap_32k_last_tick == ~0))
			break;

		modulo_count += OMAP_32K_TICK_MODULO;
		if (modulo_count > HZ) {
			++omap_32k_last_tick;
			modulo_count -= HZ;
		}
#endif
		omap_32k_last_tick += OMAP_32K_TICKS_PER_HZ;
		timer_tick(regs);
	}

	/* Restart timer so we don't drift off due to modulo or VST.
	 * By default we program the next timer to be continuous to avoid
	 * latencies during high system load. During VST operation the
	 * continuous timer can be overridden from pm_idle to be longer.
	 */
	omap_32k_timer_start(omap_32k_last_tick + OMAP_32K_TICKS_PER_HZ - now);
	write_sequnlock_irqrestore(&xtime_lock, flags);

	return IRQ_HANDLED;
}

#ifdef CONFIG_NO_IDLE_HZ

static int vst_enabled = 0;

/*
 * Programs the next timer interrupt needed. Called from pm_idle.
 */
void omap_32k_timer_next_vst_interrupt(void)
{
	unsigned long next;

	if (!vst_enabled)
		return;

	next = next_timer_interrupt() - jiffies;

	if (next > MAX_SKIP_JIFFIES)
		next = MAX_SKIP_JIFFIES;

	/*
	 * We can keep the timer continuous, no need to set it to
	 * run in one-shot mode. When using VST, the timer will get
	 * reprogrammed again after the next interrupt.
	 */
	omap_32k_timer_start(JIFFIES_TO_HW_TICKS(next, 32768) + 1);
}

static struct irqaction omap_32k_timer_irq;
extern struct timer_update_handler timer_update;

static void omap_32k_timer_enable_vst(void)
{
	vst_enabled = 1;
	timer_update.skip = INT_OS_TIMER;
	timer_update.function = omap_32k_timer_interrupt;
	omap_32k_timer_next_vst_interrupt();
}

static void omap_32k_timer_disable_vst(void)
{
	vst_enabled = 0;
	timer_update.function = 0;
	omap_32k_timer_start(OMAP_32K_TIMER_TICK_PERIOD);
}

#endif	/* CONFIG_NO_IDLE_HZ */

static struct irqaction omap_32k_timer_irq = {
	.name		= "32KHz timer",
	.flags		= SA_NODELAY | SA_INTERRUPT,
	.handler	= omap_32k_timer_interrupt
};

static __init void omap_init_32k_timer(void)
{
	/* We cannot turn on VST until after calibrate delay for bogomips */
	setup_irq(INT_OS_TIMER, &omap_32k_timer_irq);
	omap_timer.offset  = omap_32k_timer_gettimeoffset;
	omap_32k_last_tick = omap_32k_sync_timer_read();
	omap_32k_timer_start(OMAP_32K_TIMER_TICK_PERIOD);
}

#ifdef CONFIG_NO_IDLE_HZ

/*
 * ---------------------------------------------------------------------------
 * Timer VST control via sysfs. Currently only for 32KHz timer.
 * ---------------------------------------------------------------------------
 */
static ssize_t show_vst(struct sys_device *dev, char *buf)
{
	return sprintf(buf, "%i\n", vst_enabled);
}

static ssize_t set_vst(struct sys_device *dev, const char *buf, size_t count)
{
	unsigned long flags;
	unsigned int tmp = simple_strtoul(buf, NULL, 2);

	if (tmp > 1)
		tmp = 1;

	spin_lock_irqsave(&spinlock_32k, flags);
	vst_enabled = tmp;

	if (vst_enabled) {
		omap_32k_timer_enable_vst();
	} else {
		omap_32k_timer_disable_vst();
	}

	spin_unlock_irqrestore(&spinlock_32k, flags);

	return count;
}

static SYSDEV_ATTR(vst, 0644, show_vst, set_vst);

static int __init omap_timer_ctrl_init(void)
{

#if defined(CONFIG_NO_IDLE_HZ_ENABLED)
	/* Turn on VST only after calibrating delay for correct bogomips */
	omap_32k_timer_enable_vst();
#endif

	sysdev_create_file(&system_timer->dev, &attr_vst);

	return 0;
}

device_initcall(omap_timer_ctrl_init);

#endif	/* CONFIG_NO_IDLE_HZ */
#endif	/* CONFIG_OMAP_32K_TIMER */

/*
 * ---------------------------------------------------------------------------
 * Timer initialization
 * ---------------------------------------------------------------------------
 */
void __init omap_timer_init(void)
{
#if defined(CONFIG_OMAP_MPU_TIMER)
	omap_init_mpu_timer();
#elif defined(CONFIG_OMAP_32K_TIMER)
	omap_init_32k_timer();
#else
#error No system timer selected in Kconfig!
#endif
}

struct sys_timer omap_timer = {
	.init		= omap_timer_init,
	.offset		= NULL,		/* Initialized later */
};

#ifdef CONFIG_HIGH_RES_TIMERS
/*
 * arch/arm/mach-omap/hrtime.c
 *
 * High-Res Timer Implementation for TI OMAP boards
 *
 * Author: MontaVista Software, Inc. <source@mvista.com>
 *
 * 2003-2004 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

int
schedule_hr_timer_int(unsigned long ref_jiffies, int ref_cycles)
{
	unsigned long temp_cycles, jiffies_f = jiffies;
	volatile omap_mpu_timer_regs_t *subhz_timer = omap_mpu_timer_base(2);

	BUG_ON(ref_cycles < 0);

	/*
	 * Get offset from last jiffy
	 */
	temp_cycles = (ref_jiffies - jiffies_f) * arch_cycles_per_jiffy +
	    ref_cycles - get_arch_cycles(jiffies_f);

	if ((long) (ref_jiffies - jiffies_f) <= 0 && (long) temp_cycles < 0)
		return -ETIME;

	subhz_timer->cntl = MPUTIM_CLOCK_ENABLE |
	    (MPUTIM_PTV_VALUE << MPUTIM_PTV_BIT);
	subhz_timer->load_tim = temp_cycles;
	subhz_timer->cntl = MPUTIM_CLOCK_ENABLE | MPUTIM_ST |
	    (MPUTIM_PTV_VALUE << MPUTIM_PTV_BIT);

	return 0;
}

int
get_arch_cycles(unsigned long ref_jiffies)
{
	extern unsigned long do_getmachinecycles(void);
	int ret;
	unsigned temp_jiffies;
	unsigned diff_jiffies;

	do {
		/* snapshot jiffies */
		temp_jiffies = jiffies;
		barrier();

		/* calculate cycles since the current jiffy */
		ret = 0 - omap_mpu_timer_read(0) - omap_mpu_timer_last;

		/* compensate for ref_jiffies in the past */
		if (unlikely(diff_jiffies = jiffies - ref_jiffies))
			ret += diff_jiffies * arch_cycles_per_jiffy;

		barrier();
		/* repeat if we didn't have a consistent view of the world */
	} while (unlikely(temp_jiffies != jiffies));

	return ret;
}

int
schedule_nonperiodic_timer_int(unsigned long count)
{
	return schedule_hr_timer_int(jiffies + count, 0);
}

static irqreturn_t
hr_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	volatile omap_mpu_timer_regs_t *subhz_timer = omap_mpu_timer_base(2);
	subhz_timer->cntl = 0;
	do_hr_timer_int();

	return IRQ_HANDLED;
}

static struct irqaction hr_timer_irq = {
	.name		= "high-res timer",
	.handler	= hr_timer_interrupt,
	.flags		= SA_NODELAY | SA_INTERRUPT
};

static int
hr_timer_init(void)
{
	int ret;
	volatile omap_mpu_timer_regs_t *subhz_timer = omap_mpu_timer_base(2);

	subhz_timer->cntl = 0;
	ret = setup_irq(INT_TIMER3, &hr_timer_irq);

	return ret;
}

__initcall(hr_timer_init);
#endif	/* CONFIG_HIGH_RES_TIMERS */
