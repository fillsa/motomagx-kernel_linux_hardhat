/*
 * 2003 (c) MontaVista Software, Inc.  This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * Based on speedstep-centrino.c by Jeremy Fitzhardinge <jeremy@goop.org>
 */

#include <linux/config.h>
#include <linux/dpm.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/cpufeature.h>

/* Extract clock in kHz from PERF_CTL value */
static unsigned extract_clock(unsigned msr)
{
	msr = (msr >> 8) & 0xff;
	return msr * 100000;
}

/* Return the current CPU frequency in kHz */
static unsigned get_cur_freq(void)
{
	unsigned l, h;

	rdmsr(MSR_IA32_PERF_STATUS, l, h);
	return extract_clock(l);
}

static int
dpm_centrino_init_opt(struct dpm_opt *opt)
{
	int v		= opt->pp[DPM_MD_V];
	int cpu		= opt->pp[DPM_MD_CPU_FREQ];

	struct dpm_md_opt *md_opt = &opt->md_opt;

	md_opt->v = v;
	md_opt->cpu = cpu;
	return 0;
}

/* Fully determine the current machine-dependent operating point, and fill in a
   structure presented by the caller. This should only be called when the
   dpm_sem is held. This call can return an error if the system is currently at
   an operating point that could not be constructed by dpm_md_init_opt(). */

static int
dpm_centrino_get_opt(struct dpm_opt *opt)
{
	struct dpm_md_opt *md_opt = &opt->md_opt;

	md_opt->v = 100; /* TODO. */
	md_opt->cpu = get_cur_freq();
	return 0;
}

static int
dpm_centrino_set_opt(struct dpm_md_opt *md_opt)
{
	unsigned int msr = 0, oldmsr, h, mask = 0;

	if (md_opt->cpu != -1) {
		msr |= ((md_opt->cpu)/100) << 8;
		mask |= 0xff00;
	}

	if (md_opt->v != -1) {
		msr |= ((md_opt->v - 700) / 16);
		mask |= 0xff;
	}

	rdmsr(MSR_IA32_PERF_CTL, oldmsr, h);

	if (msr == (oldmsr & mask))
		return 0;

	/* all but 16 LSB are "reserved", so treat them with
	   care */
	oldmsr &= ~mask;
	msr &= mask;
	oldmsr |= msr;

	wrmsr(MSR_IA32_PERF_CTL, oldmsr, h);
	return 0;
}

static int dpm_centrino_startup(void)
{
	struct cpuinfo_x86 *cpu = cpu_data;
	unsigned l, h;

	if (!cpu_has(cpu, X86_FEATURE_EST))
		return -ENODEV;

	/* Check to see if Enhanced SpeedStep is enabled, and try to
	   enable it if not. */
	rdmsr(MSR_IA32_MISC_ENABLE, l, h);

	if (!(l & (1<<16))) {
		l |= (1<<16);
		wrmsr(MSR_IA32_MISC_ENABLE, l, h);

		/* check to see if it stuck */
		rdmsr(MSR_IA32_MISC_ENABLE, l, h);
		if (!(l & (1<<16))) {
			printk(KERN_INFO "DPM: Couldn't enable Enhanced SpeedStep\n");
			return -ENODEV;
		}
	}

	return 0;
}

int __init dpm_centrino_init(void)
{
	printk("Dynamic Power Management for Intel Centrino Enhanced SpeedStep.\n");

	dpm_bd.startup = dpm_centrino_startup;
	dpm_bd.init_opt = dpm_centrino_init_opt;
	dpm_bd.get_opt = dpm_centrino_get_opt;
	dpm_bd.set_opt = dpm_centrino_set_opt;
	return 0;
}

__initcall(dpm_centrino_init);
