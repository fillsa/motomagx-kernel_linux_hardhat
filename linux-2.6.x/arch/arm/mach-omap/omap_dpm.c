/*
 * arch/arm/mach-omap/omap_dpm.c  DPM support for TI OMAP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Copyright (C) 2002, 2004 MontaVista Software <source@mvista.com>.
 *
 * Based on code by Matthew Locke, Dmitry Chigirev, and Bishop Brock.
 */

#include <linux/config.h>
#include <linux/dpm.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/err.h>

#include <asm/hardirq.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/hardware/clock.h>
#include <asm/arch/board.h>
#include <asm/arch/mux.h>
#include <asm/arch/pm.h>
#include <asm/mach-types.h>

/* ARM_CKCTL bit shifts */
#define CKCTL_PERDIV_OFFSET	0
#define CKCTL_LCDDIV_OFFSET	2
#define CKCTL_ARMDIV_OFFSET	4
#define CKCTL_DSPDIV_OFFSET	6
#define CKCTL_TCDIV_OFFSET	8
#define CKCTL_DSPMMUDIV_OFFSET	10

#define CK_DPLL_MASK                  0x0fe0

#if defined(CONFIG_MACH_OMAP_H2)
#define V_CORE_HIGH			1500
#define V_CORE_LOW			1300
#define DVS_SUPPORT
#elif defined(CONFIG_MACH_OMAP_H3)
#define V_CORE_HIGH			1300
#define V_CORE_LOW			1050
#define DVS_SUPPORT
#else
#define V_CORE_HIGH			1500
#endif

#define ULPD_MIN_MAX_REG (1 << 11)
#define ULPD_DVS_ENABLE  (1 << 10)
#define ULPD_LOW_PWR_REQ (1 << 1)
#if 1 /* Mobilinux 4.x */
#define ULPD_LOW_PWR_EN (1 << 0)
#endif /* Mobilinux 4.x */
#define LOW_POWER (ULPD_MIN_MAX_REG | ULPD_DVS_ENABLE | ULPD_LOW_PWR_REQ | \
		   ULPD_LOW_PWR_EN)

static void omap_scale_divisors(struct dpm_regs *regs)
{
	omap_writew((omap_readw(ARM_CKCTL) & ~regs->arm_msk) | regs->arm_ctl, ARM_CKCTL);
}

static void omap_scale_dpll(struct dpm_regs *regs)
{
	int i;

	omap_writew((omap_readw(DPLL_CTL) & ~CK_DPLL_MASK) | regs->dpll_ctl,
		    DPLL_CTL);

	for (i = 0; i < 0x1FF; i++)
		nop();

	/*
	 * Wait for PLL relock.
	 */

	while ((omap_readw(DPLL_CTL) & 0x1) == 0);
}

static void
omap_scale_v(struct dpm_md_opt *opt)
{
#ifdef DVS_SUPPORT
	if (opt->v == V_CORE_LOW)
		omap_writew(omap_readw(ULPD_POWER_CTRL) | LOW_POWER,
			    ULPD_POWER_CTRL);

	else if (opt->v == V_CORE_HIGH)
		omap_writew(omap_readw(ULPD_POWER_CTRL) & ~LOW_POWER,
			    ULPD_POWER_CTRL);
#endif
}

static unsigned int
voltage_get(void)
{
	unsigned int v;

#ifdef DVS_SUPPORT
	if (omap_readw(ULPD_POWER_CTRL) & (1 << 1))
		v = V_CORE_LOW;
	else
#endif
		v = V_CORE_HIGH;

	return v;
}

int
dpm_omap_set_opt(struct dpm_opt *cur, struct dpm_opt *new)
{
	unsigned long flags;

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

	local_irq_save(flags);

	if (new->md_opt.regs.dpll_ctl <
	    (cur->md_opt.regs.dpll_ctl & CK_DPLL_MASK))
		omap_scale_dpll(&new->md_opt.regs);

	if (new->md_opt.regs.arm_ctl !=
	    (cur->md_opt.regs.arm_ctl & new->md_opt.regs.arm_msk))
		omap_scale_divisors(&new->md_opt.regs);

	if (new->md_opt.regs.dpll_ctl >
	    (cur->md_opt.regs.dpll_ctl & CK_DPLL_MASK))
		omap_scale_dpll(&new->md_opt.regs);

	if ((new->md_opt.v != -1) && (new->md_opt.v != cur->md_opt.v))
		omap_scale_v(&new->md_opt);

	if (cur->md_opt.cpu && new->md_opt.cpu)
		loops_per_jiffy = dpm_compute_lpj(loops_per_jiffy,
						  cur->md_opt.cpu,
						  new->md_opt.cpu);
	local_irq_restore(flags);
	return 0;
}



/* Initialize the machine-dependent operating point from a list of parameters,
   which has already been installed in the pp field of the operating point.
   Some of the parameters may be specified with a value of -1 to indicate a
   default value. */

#define DPLL_DIV_MAX 4
#define DPLL_MULT_MAX 31

int divider_encode(int n)
{
	int count = 0;
        if (n == 0 || n == -1)
		return n;

	if (n > 8) 	/* no divider greater than 8 is valid */
		n = 8;
	while(n) {
		n = n >> 1;
		count++;
	}
	return (count -1);
}

/*
 * FIXME: Use the new scheme.
 */
#define MHz	1000000

static long get_dpll_rate(void)
{
	struct clk *clk;
	long ret;

	clk = clk_get(0, "ck_dpll1");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_get_rate(clk);
	clk_put(clk);
	return ret /MHz;
}
static long get_ckref_rate(void)
{
	struct clk *clk;
	long ret;

	clk = clk_get(0, "ck_ref");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_get_rate(clk);
	clk_put(clk);
	return ret /MHz;
}
static long get_tc_rate(void)
{
	struct clk *clk;
	long ret;

	clk = clk_get(0, "tc_ck");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_get_rate(clk);
	clk_put(clk);
	return ret /MHz;
}

static long get_mpuper_rate(void)
{
	struct clk *clk;
	long ret;

	clk = clk_get(0, "mpuper_ck");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_get_rate(clk);
	clk_put(clk);
	return ret /MHz;
}

static long get_dsp_rate(void)
{
	struct clk *clk;
	long ret;

	clk = clk_get(0, "dsp_ck");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_get_rate(clk);
	clk_put(clk);
	return ret /MHz;
}

static long get_dspmmu_rate(void)
{
	struct clk *clk;
	long ret;

	clk = clk_get(0, "dspmmu_ck");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_get_rate(clk);
	clk_put(clk);
	return ret /MHz;
}

static long get_lcd_rate(void)
{
	struct clk *clk;
	long ret;

	clk = clk_get(0, "lcd_ck");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_get_rate(clk);
	clk_put(clk);
	return ret /MHz;
}

static long get_arm_rate(void)
{
	struct clk *clk;
	long ret;

	clk = clk_get(0, "arm_ck");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_get_rate(clk);
	clk_put(clk);
	return ret /MHz;;
}


int
dpm_omap_init_opt(struct dpm_opt *opt)
{
	static int divider_decode[] = {1,2,4,8};
	int v		= opt->pp[DPM_MD_V];
	int pll_mult	= opt->pp[DPM_MD_DPLL_MULT];
	int pll_div	= opt->pp[DPM_MD_DPLL_DIV];
	int arm_div	= opt->pp[DPM_MD_ARM_DIV];
	int tc_div	= opt->pp[DPM_MD_TC_DIV];
	int per_div	= opt->pp[DPM_MD_PER_DIV];
	int dsp_div	= opt->pp[DPM_MD_DSP_DIV];
	int dspmmu_div	= opt->pp[DPM_MD_DSPMMU_DIV];
	int lcd_div	= opt->pp[DPM_MD_LCD_DIV];

	struct dpm_md_opt *md_opt = &opt->md_opt;

	/* Let's do some upfront error checking.  If we fail any of these, then the
	 * whole operating point is suspect and therefore invalid.
	 */

	if ((pll_mult > DPLL_MULT_MAX) ||
	    (pll_div > DPLL_DIV_MAX)) {

		printk(KERN_WARNING "dpm_md_init_opt: "
				 "PLL_MUL/PLL_DIV specification "
				 "(%d/%d) out of range for op named %s\n",
				 pll_mult, pll_div, opt->name );
		return -EINVAL;
	}

#ifdef DVS_SUPPORT
/* make sure the operating point follows the syncronous scalable mode rules */

	if ((v != -1) && ((v != V_CORE_LOW) && (v != V_CORE_HIGH))) {
		printk(KERN_WARNING "dpm_md_init_opt: "
		       "Core voltage can not be other than %dmV or %dmV\n"
		       "Core voltage %d out of range"
		       " for opt named %s\n", V_CORE_LOW, V_CORE_HIGH, v, opt->name);
		return -EINVAL;
	}
#endif

	if ((arm_div > 0) && (tc_div > 0) && (arm_div > tc_div)) {
		printk(KERN_WARNING "dpm_md_init_opt: "
			 "CPU frequency can not be lower than TC frequency"
			 "CPU div %u and/or TC div %u out of range"
			 " for opt named %s\n",
			 arm_div, tc_div, opt->name );
		return -EINVAL;
	}

	if ((dspmmu_div > 0) && (tc_div > 0) && (dspmmu_div > tc_div)) {
		printk(KERN_WARNING "dpm_md_init_opt: "
			 "DSP MMU frequency can not be lower than TC frequency"
			 "DSP MMU div %u and/or TC div %u out of range"
			 " for opt named %s\n",
			 dspmmu_div, tc_div, opt->name );
		return -EINVAL;
	}

	if ((dsp_div > 0) && (dspmmu_div > 0) && (dspmmu_div > dsp_div)) {
		printk(KERN_WARNING "dpm_md_init_opt: "
			 "DSP MMU frequency can not be lower than DSP frequency"
			 "DSP MMU div %u and/or DSP div %u out of range"
			 " for opt named %s\n",
			 dspmmu_div, dsp_div, opt->name );
		return -EINVAL;
	}

	/* 0 is a special case placeholder for dpll idle */
	if (pll_mult == 0 || pll_div == 0) {
		md_opt->regs.dpll_ctl = 0xffff;
		md_opt->dpll = get_dpll_rate();
	}
	else if ((pll_mult > 0) && (pll_div > 0)) {
		md_opt->dpll = (get_ckref_rate() * pll_mult) / pll_div;
		md_opt->regs.dpll_ctl = (((pll_mult << 2) |
					  (pll_div -1)) << 5);
	}

 	/* the encode/decode will weed out invalid dividers */
	if (arm_div == 0) 		/* zero means sleep for cpu */
		md_opt->cpu = 0;
	else if (arm_div > 0) {
		arm_div	= divider_encode(arm_div);
		md_opt->cpu = md_opt->dpll / divider_decode[arm_div];
		md_opt->regs.arm_ctl |= (arm_div << CKCTL_ARMDIV_OFFSET);
		md_opt->regs.arm_msk |= (3 << CKCTL_ARMDIV_OFFSET);
	}

	/*  Ignore 0 and -1 */
	if (tc_div > 0) {
		tc_div	= divider_encode(tc_div);
		md_opt->tc = md_opt->dpll / divider_decode[tc_div];
		md_opt->regs.arm_ctl |= (tc_div << CKCTL_TCDIV_OFFSET);
		md_opt->regs.arm_msk |= (3 << CKCTL_TCDIV_OFFSET);
	}

	if (per_div > 0) {
		per_div	= divider_encode(per_div);
		md_opt->per = md_opt->dpll / divider_decode[per_div];
		md_opt->regs.arm_ctl |= (per_div << CKCTL_PERDIV_OFFSET);
		md_opt->regs.arm_msk |= (3 << CKCTL_PERDIV_OFFSET);
	}

	if (dsp_div > 0) {
		dsp_div	= divider_encode(dsp_div);
		md_opt->dsp = md_opt->dpll / divider_decode[dsp_div];
		md_opt->regs.arm_ctl |= (dsp_div << CKCTL_DSPDIV_OFFSET);
		md_opt->regs.arm_msk |= (3 << CKCTL_DSPDIV_OFFSET);
	}

	if (dspmmu_div > 0) {
		dspmmu_div= divider_encode(dspmmu_div);
		md_opt->dspmmu = md_opt->dpll / divider_decode[dspmmu_div];
		md_opt->regs.arm_ctl |= (dspmmu_div <<CKCTL_DSPMMUDIV_OFFSET);
		md_opt->regs.arm_msk |= (3 << CKCTL_DSPMMUDIV_OFFSET);
	}

	if (lcd_div > 0) {
		lcd_div	= divider_encode(lcd_div);
		md_opt->lcd = md_opt->dpll / divider_decode[lcd_div];
		md_opt->regs.arm_ctl |= (lcd_div << CKCTL_LCDDIV_OFFSET);
		md_opt->regs.arm_msk |= (3 << CKCTL_LCDDIV_OFFSET);
	}

	md_opt->v = v;
	return 0;
}

/* Fully determine the current machine-dependent operating point, and fill in a
   structure presented by the caller. This should only be called when the
   dpm_sem is held. This call can return an error if the system is currently at
   an operating point that could not be constructed by dpm_md_init_opt(). */

int
dpm_omap_get_opt(struct dpm_opt *opt)
{
	struct dpm_md_opt *md_opt = &opt->md_opt;

	md_opt->v = voltage_get();
	md_opt->dpll = get_dpll_rate();
	md_opt->cpu = get_arm_rate();
	md_opt->tc = get_tc_rate();
	md_opt->per = get_mpuper_rate();
	md_opt->dsp = get_dsp_rate();
	md_opt->dspmmu = get_dspmmu_rate();
	md_opt->lcd = get_lcd_rate();
	md_opt->regs.dpll_ctl = omap_readw(DPLL_CTL);
	md_opt->regs.arm_ctl = omap_readw(ARM_CKCTL);

	return 0;
}


/****************************************************************************
 *  DPM Idle Handler
 ****************************************************************************/

static void (*orig_idle)(void);

void dpm_omap_idle(void)
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

extern void (*pm_idle)(void);

static void
dpm_omap_startup(void)
{
	if (pm_idle != dpm_idle) {
		orig_idle = pm_idle;
		pm_idle = dpm_idle;
	}
}

static void
dpm_omap_cleanup(void)
{
	pm_idle = orig_idle;
}

int __init
dpm_omap_init(void)
{
	printk("TI OMAP Dynamic Power Management.\n");
	dpm_md.init_opt		= dpm_omap_init_opt;
	dpm_md.set_opt		= dpm_omap_set_opt;
	dpm_md.get_opt		= dpm_omap_get_opt;
	dpm_md.check_constraint	= dpm_default_check_constraint;
	dpm_md.idle		= dpm_omap_idle;
	dpm_md.startup		= dpm_omap_startup;
	dpm_md.cleanup		= dpm_omap_cleanup;

	/* MUX select pin T20 to LOW_PWR */
	omap_cfg_reg(T20_1610_LOW_PWR);
	return 0;
}

__initcall(dpm_omap_init);

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
