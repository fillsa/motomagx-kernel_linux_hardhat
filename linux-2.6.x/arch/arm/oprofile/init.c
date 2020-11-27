/**
 * @file init.c
 *
 * @remark Copyright 2004 Oprofile Authors
 * @remark Read the file COPYING
 *
 * @author Zwane Mwaikambo
 */

#include <linux/oprofile.h>
#include <linux/init.h>
#include <linux/errno.h>
#include "op_arm_model.h"

int __init oprofile_arch_init(struct oprofile_operations **ops)
{
	int ret = -ENODEV;

#if defined(CONFIG_CPU_XSCALE) || defined(CONFIG_CPU_V6)
	ret = pmu_init(ops, &op_arm_spec);
#endif
	return ret;
}

void oprofile_arch_exit(void)
{
#if defined(CONFIG_CPU_XSCALE) || defined(CONFIG_CPU_V6)
	pmu_exit();
#endif
}
