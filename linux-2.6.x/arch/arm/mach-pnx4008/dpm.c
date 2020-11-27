/*
 * arch/arm/mach-pnx4008/dpm.c
 *
 * DPM driver for PNX4008
 *
 * Authors: Vitaly Wool, Dmitry Chigirev <source@mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>
#include <linux/dpm.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/cpufreq.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/dpm.h>
#include <asm/hardware/clock.h>

static struct clk *vfp9_clk;
static struct clk *hclk_clk;
static struct clk *m2_clk;
static struct clk *pll4_clk;

int dpm_pnx4008_set_opt(struct dpm_opt *cur, struct dpm_opt *new)
{
	unsigned long flags;

	if (new->md_opt.sleep_mode != PM_SUSPEND_ON) {
#ifdef CONFIG_PM
		pm_suspend(new->md_opt.sleep_mode);
		/* Here when we wake up. */
		/* Recursive call to switch back to task state. */
		dpm_set_os(DPM_TASK_STATE);
#else
		printk(KERN_WARNING
		       "DPM: Warning - can't sleep with CONFIG_PM undefined\n");
#endif				/* CONFIG_PM */
		return 0;
	}

	local_irq_save(flags);

	if (new->md_opt.cpu != cur->md_opt.cpu ||
	    new->md_opt.bus_div != cur->md_opt.bus_div) {
		clk_set_rate(hclk_clk, new->md_opt.bus_div);
		clk_set_rate(pll4_clk, new->md_opt.cpu);
	}

	if (new->md_opt.vfp9 != cur->md_opt.vfp9)
		clk_set_rate(vfp9_clk, new->md_opt.vfp9);
	if (new->md_opt.m2 != cur->md_opt.m2)
		clk_set_rate(m2_clk, new->md_opt.m2);

	if (cur->md_opt.cpu != new->md_opt.cpu && cur->md_opt.cpu
	    && new->md_opt.cpu)
		loops_per_jiffy =
		    dpm_compute_lpj(loops_per_jiffy, cur->md_opt.cpu,
				    new->md_opt.cpu);
	local_irq_restore(flags);

	return 0;
}

/* Fully determine the current machine-dependent operating point, and fill in a
   structure presented by the caller. This should only be called when the
   dpm_sem is held. This call can return an error if the system is currently at
   an operating point that could not be constructed by dpm_md_init_opt(). */

int dpm_pnx4008_get_opt(struct dpm_opt *opt)
{
	struct dpm_md_opt *md_opt = &opt->md_opt;

	md_opt->cpu = clk_get_rate(pll4_clk);
	md_opt->vfp9 = clk_get_rate(vfp9_clk);
	md_opt->bus_div = clk_get_rate(hclk_clk);
	md_opt->m2 = clk_get_rate(m2_clk);

	md_opt->sleep_mode = PM_SUSPEND_ON;

	return 0;
}

/****************************************************************************
 *  DPM Idle Handler
 ****************************************************************************/

static void (*orig_idle) (void);

void dpm_pnx4008_idle(void)
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

extern void (*pm_idle) (void);

static void dpm_pnx4008_startup(void)
{
	if (pm_idle != dpm_idle) {
		orig_idle = pm_idle;
		pm_idle = dpm_idle;
	}
}

static void dpm_pnx4008_cleanup(void)
{
	pm_idle = orig_idle;
}

int dpm_pnx4008_init_opt(struct dpm_opt *opt)
{
	int pll_freq = opt->pp[DPM_MD_PLL_FREQ];
	int bus_div = opt->pp[DPM_MD_BUS_DIV];
	int vfp9_ena = opt->pp[DPM_MD_VFP9_ENA];
	int m2_ena = opt->pp[DPM_MD_M2_ENA];
	int sleep_mode = opt->pp[DPM_MD_SLEEP_MODE];

	struct dpm_md_opt *md_opt = &opt->md_opt;

	/*
	 * Let's do some upfront error checking.  If we fail any
	 * of these, then the whole operating point is suspect
	 * and therefore invalid.
	 */

	if ((pll_freq == -1 && bus_div != -1)
	    || (pll_freq != -1 && bus_div == -1)) {
		printk(KERN_WARNING "dpm_md_init_opt: "
		       "PLL_FREQ and BUS_DIV parameters must be either both enabled "
		       "or both disabled for op named %s\n", opt->name);
		return -EINVAL;
	}

	if (pll_freq != -1
	    && (pll_freq < 0 || pll_freq > 208000
		|| pll_freq % (bus_div * 13000))) {
		printk(KERN_WARNING "dpm_md_init_opt: " "PLL_FREQ parameter "
		       "(%d) out of range for op named %s\n", pll_freq,
		       opt->name);
		return -EINVAL;
	}

	if (bus_div != -1
	    &&
	    (!((bus_div == 1 && pll_freq != 208000) || bus_div == 2
	       || bus_div == 4))) {
		printk(KERN_WARNING "dpm_md_init_opt: " "BUS_DIV parameter "
		       "(%d) out of range for op named %s\n", bus_div,
		       opt->name);
		return -EINVAL;
	}

	if (pll_freq == -1)
		md_opt->cpu = clk_get_rate(pll4_clk);
	else
		md_opt->cpu = pll_freq;

	if (bus_div == -1)
		md_opt->bus_div = clk_get_rate(hclk_clk);
	else
		md_opt->bus_div = bus_div;

	switch (vfp9_ena) {
	case 0:
	case 1:
		md_opt->vfp9 = vfp9_ena;
		break;
	case -1:
		md_opt->vfp9 = clk_get_rate(vfp9_clk);
		break;

	default:
		printk(KERN_WARNING "dpm_md_init_opt: "
		       "VFP9_ENA parameter "
		       "(%d) out of range for op named %s\n",
		       vfp9_ena, opt->name);
		return -EINVAL;
	}

	switch (m2_ena) {
	case 0:
	case 1:
		md_opt->m2 = m2_ena;
		break;
	case -1:
		md_opt->m2 = clk_get_rate(m2_clk);
		break;

	default:
		printk(KERN_WARNING "dpm_md_init_opt: "
		       "M2_ENA parameter "
		       "(%d) out of range for op named %s\n",
		       m2_ena, opt->name);
		return -EINVAL;
	}

	if (sleep_mode > 0)
		md_opt->sleep_mode = sleep_mode;

	printk(KERN_INFO
	       "DPM: new op: name %s, CPU freq: %d, BUS div: %d, VFP Ena: %d, M2 Ena: %d, Slp: %d\n",
	       opt->name, pll_freq, bus_div, vfp9_ena, m2_ena, sleep_mode);

	return 0;
}

int __init dpm_pnx4008_init(void)
{
	printk("PNX4008 Dynamic Power Management\n");

	vfp9_clk = clk_get(0, "vfp9_ck");
	if (IS_ERR(vfp9_clk)) {
		printk(KERN_ERR
		       "DPM Error: cannot acquire VFP9 clock control\n");
		return PTR_ERR(vfp9_clk);
	}

	hclk_clk = clk_get(0, "hclk_ck");
	if (IS_ERR(hclk_clk)) {
		printk(KERN_ERR
		       "DPM Error: cannot acquire BUSCLK/HCLK clock control\n");
		clk_put(vfp9_clk);
		return PTR_ERR(hclk_clk);
	}

	m2_clk = clk_get(0, "m2hclk_ck");
	if (IS_ERR(m2_clk)) {
		printk(KERN_ERR
		       "DPM Error: cannot acquire AHB Matrix2 clock control\n");
		clk_put(vfp9_clk);
		clk_put(hclk_clk);
		return PTR_ERR(m2_clk);
	}

	pll4_clk = clk_get(0, "ck_pll4");
	if (IS_ERR(pll4_clk)) {
		printk(KERN_ERR
		       "DPM Error: cannot acquire ARM(PLL4) clock control\n");
		clk_put(m2_clk);
		clk_put(vfp9_clk);
		clk_put(hclk_clk);
		return PTR_ERR(pll4_clk);
	}

	dpm_md.init_opt = dpm_pnx4008_init_opt;
	dpm_md.set_opt = dpm_pnx4008_set_opt;
	dpm_md.get_opt = dpm_pnx4008_get_opt;
	dpm_md.check_constraint = dpm_default_check_constraint;
	dpm_md.idle = dpm_pnx4008_idle;
	dpm_md.startup = dpm_pnx4008_startup;
	dpm_md.cleanup = dpm_pnx4008_cleanup;

	return 0;
}

__initcall(dpm_pnx4008_init);
