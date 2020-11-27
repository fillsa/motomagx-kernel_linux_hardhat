/*
 * 2003 (c) MontaVista Software, Inc.  This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>
#include <linux/dpm.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include <asm/page.h>
#include <asm/uaccess.h>

struct dpm_bd dpm_bd;

static int
dpm_x86_init_opt(struct dpm_opt *opt)
{
	return dpm_bd.init_opt ? dpm_bd.init_opt(opt) : -1;
}

/* Fully determine the current machine-dependent operating point, and fill in a
   structure presented by the caller. This should only be called when the
   dpm_sem is held. This call can return an error if the system is currently at
   an operating point that could not be constructed by dpm_md_init_opt(). */

static unsigned long loops_per_jiffy_ref = 0;

static int
dpm_x86_get_opt(struct dpm_opt *opt)
{
	return dpm_bd.get_opt ? dpm_bd.get_opt(opt) : -1;
}

int
dpm_x86_set_opt(struct dpm_opt *cur, struct dpm_opt *new)
{
	struct cpuinfo_x86 *cpu = cpu_data;

	if (! new->md_opt.cpu) {
#ifdef CONFIG_PM
		pm_suspend(PM_SUSPEND_STANDBY);

	 	/* Here when we wake up.  Recursive call to switch back to
		 * to task state.
		 */

		dpm_set_os(DPM_TASK_STATE);
#endif
		return 0;
	}

	if (dpm_bd.set_opt){
	 	dpm_bd.set_opt(&new->md_opt);

	}else{
		return -1;
	}

       if (cur->md_opt.cpu && new->md_opt.cpu){
		loops_per_jiffy_ref = cpu->loops_per_jiffy;
                cpu->loops_per_jiffy =
			dpm_compute_lpj(loops_per_jiffy_ref ,
                                                  cur->md_opt.cpu,
                                                  new->md_opt.cpu);

		loops_per_jiffy = cpu->loops_per_jiffy;
		if (cpu_khz)
			cpu_khz = dpm_compute_lpj(cpu_khz,
                                                  cur->md_opt.cpu,
                                                  new->md_opt.cpu);
	}
	return 0;
}

/*
 * idle loop
 */

static void (*orig_idle)(void);

void dpm_x86_idle(void)
{
	extern void default_idle(void);

	if (orig_idle)
		orig_idle();
	else
		default_idle();
}

/****************************************************************************
 * Initialization/Exit
 ****************************************************************************/

void
dpm_x86_startup(void)
{
	orig_idle = pm_idle;
	pm_idle = dpm_idle;

	if (dpm_bd.startup)
		dpm_bd.startup();
}

void
dpm_x86_cleanup(void)
{
	pm_idle = orig_idle;

	if (dpm_bd.cleanup)
		dpm_bd.cleanup();
}

int __init
dpm_x86_init(void)
{
	printk("Dynamic Power Management for x86.\n");

	dpm_md.init_opt		= dpm_x86_init_opt;
	dpm_md.set_opt		= dpm_x86_set_opt;
	dpm_md.get_opt		= dpm_x86_get_opt;
	dpm_md.idle		= dpm_x86_idle;
	dpm_md.startup		= dpm_x86_startup;
	dpm_md.cleanup		= dpm_x86_cleanup;
	return 0;
}
__initcall(dpm_x86_init);
