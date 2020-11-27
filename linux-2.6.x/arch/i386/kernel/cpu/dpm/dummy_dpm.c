/*
 * 2005 (c) MontaVista Software, Inc.  This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 */

#include <linux/config.h>
#include <linux/dpm.h>
#include <linux/errno.h>
#include <linux/init.h>

static int
dpm_dummy_init_opt(struct dpm_opt *opt)
{
	return 0;
}

static int
dpm_dummy_get_opt(struct dpm_opt *opt)
{
	return 0;
}

static int
dpm_dummy_set_opt(struct dpm_md_opt *md_opt)
{
	return 0;
}

static int dpm_dummy_startup(void)
{
	return 0;
}

int __init dpm_dummy_init(void)
{
	dpm_bd.startup = dpm_dummy_startup;
	dpm_bd.init_opt = dpm_dummy_init_opt;
	dpm_bd.get_opt = dpm_dummy_get_opt;
	dpm_bd.set_opt = dpm_dummy_set_opt;
	return 0;
}

__initcall(dpm_dummy_init);
