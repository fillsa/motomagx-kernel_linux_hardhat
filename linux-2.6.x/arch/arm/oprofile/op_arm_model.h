/**
 * @file op_arm_model.h
 * interface to ARM machine specific operations
 *
 * @remark Copyright 2004 Oprofile Authors
 * @remark Read the file COPYING
 *
 * @author Zwane Mwaikambo
 */

/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 */
/* 
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 * 
 * http://www.opensource.org/licenses/gpl-license.html 
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file op_arm_model.h
 * 
 * @ingroup MXC_Oprofile
 */



#ifndef OP_ARM_MODEL_H
#define OP_ARM_MODEL_H

struct op_arm_model_spec {
	int (*init)(void);
	unsigned int num_counters;
	int (*setup_ctrs)(void);
	int (*start)(void);
	void (*stop)(void);
	char *name;
};

extern struct op_arm_model_spec op_arm_spec;

extern int pmu_init(struct oprofile_operations **ops, struct op_arm_model_spec *spec);
extern void pmu_exit(void);
#endif /* OP_ARM_MODEL_H */
