/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @defgroup MXC_Oprofile ARM11 Driver for Oprofile
 */

/*!
 * @file op_model_arm11.c
 *
 *Based on the op_model_xscale.c driver by author Zwane Mwaikambo
 *
 * @ingroup MXC_Oprofile
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/oprofile.h>
#include <linux/interrupt.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/system.h>

#include "op_counter.h"
#include "op_arm_model.h"

/*!
 * defines used in ARM11 performance unit
 */
#define PMU_ENABLE       0x001	/* Enable counters */
#define PMN_RESET        0x002	/* Reset event counters */
#define CCNT_RESET       0x004	/* Reset clock counter */
#define PMU_RESET        (CCNT_RESET | PMN_RESET)
#define PMU_CNT64        0x008	/* Make CCNT count every 64th cycle */

#define PMU_FLAG_CR0     0x080
#define PMU_FLAG_CR1     0x100
#define PMU_FLAG_CC      0x200

/*!
 * Different types of events that can be counted by the ARM11 PMU
 * as used by Oprofile userspace.
 */
#define EVT_ICACHE_MISS                  0x00
#define EVT_STALL_INSTR                  0x01
#define EVT_DATA_STALL                   0x02
#define EVT_ITLB_MISS                    0x03
#define EVT_DTLB_MISS                    0x04
#define EVT_BRANCH                       0x05
#define EVT_BRANCH_MISS                  0x06
#define EVT_INSTRUCTION                  0x07
#define EVT_DCACHE_FULL_STALL_CONTIG     0x09
#define EVT_DCACHE_ACCESS                0x0A
#define EVT_DCACHE_MISS                  0x0B
#define EVT_DCACE_WRITE_BACK             0x0C
#define EVT_PC_CHANGED                   0x0D
#define EVT_TLB_MISS                     0x0F
#define EVT_BCU_REQUEST                  0x10
#define EVT_BCU_FULL                     0x11
#define EVT_BCU_DRAIN                    0x12
#define EVT_ETMEXTOT0                    0x20
#define EVT_ETMEXTOT1                    0x21
/* EVT_CCNT is not hardware defined */
#define EVT_CCNT                         0xFE
#define EVT_INCREMENT                    0xFF
#define EVT_UNUSED                       0x100

struct pmu_counter {
	volatile unsigned long ovf;
	unsigned long reset_counter;
};

enum { CCNT, PMN0, PMN1, MAX_COUNTERS };

static struct pmu_counter results[MAX_COUNTERS];

struct pmu_type {
	int id;
	char *name;
	int num_counters;
	unsigned int int_enable;
	unsigned int cnt_ovf[MAX_COUNTERS];
	unsigned int int_mask[MAX_COUNTERS];
};

static struct pmu_type pmu_parms[] = {
	{
	 .id = 0,
	 .name = "arm/arm11",
	 .num_counters = 3,
	 .int_mask = {[PMN0] = 0x10,[PMN1] = 0x20,
		      [CCNT] = 0x40},
	 .cnt_ovf = {[CCNT] = 0x400,[PMN0] = 0x100,
		     [PMN1] = 0x200},
	 },
};

static struct pmu_type *pmu;

/*!
 * function is used to write the control register for the ARM11 performance counters
 */
static void write_pmnc(u32 val)
{
	pr_debug("PMC value written is %#08x\n", val);
	__asm__ __volatile__("mcr p15, 0, %0, c15, c12, 0"::"r"(val));
}

/*!
 * function is used to read the control register for the ARM11 performance counters
 */
static u32 read_pmnc(void)
{
	u32 val;
	pr_debug("In function %s\n", __FUNCTION__);
	__asm__ __volatile__("mrc p15, 0, %0, c15, c12, 0":"=r"(val));
	pr_debug("PMC value read is %#08x\n", val);
	return val;
}

/*!
 * function is used to read the ARM11 performance counters
 */
static u32 read_counter(int counter)
{
	u32 val = 0;
	pr_debug("In function %s\n", __FUNCTION__);

	switch (counter) {
	case CCNT:
	      __asm__ __volatile__("mrc p15, 0, %0, c15, c12, 1":"=r"(val));
		break;
	case PMN0:
	      __asm__ __volatile__("mrc p15, 0, %0, c15, c12, 2":"=r"(val));
		break;
	case PMN1:
	      __asm__ __volatile__("mrc p15, 0, %0, c15, c12, 3":"=r"(val));
		break;
	}

	pr_debug("counter %d value read is %#08x\n", counter, val);
	return val;
}

/*!
 * function is used to write to the ARM11 performance counters
 */
static void write_counter(int counter, u32 val)
{
	pr_debug("counter %d value written is %#08x\n", counter, val);

	switch (counter) {
	case CCNT:
	      __asm__ __volatile__("mcr p15, 0, %0, c15, c12, 1": :"r"(val));
		break;
	case PMN0:
	      __asm__ __volatile__("mcr p15, 0, %0, c15, c12, 2": :"r"(val));
		break;
	case PMN1:
	      __asm__ __volatile__("mcr p15, 0, %0, c15, c12, 3": :"r"(val));
		break;
	}
}

/*!
 * function is used to check the status of the ARM11 performance counters
 */
static int arm11_setup_ctrs(void)
{
	u32 pmnc;
	int i;

	for (i = CCNT; i < MAX_COUNTERS; i++) {
		if (counter_config[i].enabled)
			continue;

		counter_config[i].event = EVT_UNUSED;
	}

	pmnc =
	    (counter_config[PMN1].event << 20) | (counter_config[PMN0].
						  event << 12);
	pr_debug("arm11_setup_ctrs: pmnc: %#08x\n", pmnc);
	write_pmnc(pmnc);

	for (i = CCNT; i < MAX_COUNTERS; i++) {
		if (counter_config[i].event == EVT_UNUSED) {
			counter_config[i].event = 0;
			pmu->int_enable &= ~pmu->int_mask[i];
			pr_debug
			    ("arm11_setup_ctrs: The counter event is %d for counter%d\n",
			     counter_config[i].event, i);
			continue;
		}

		results[i].reset_counter = counter_config[i].count;
		write_counter(i, -(u32) counter_config[i].count);
		pmu->int_enable |= pmu->int_mask[i];
		pr_debug
		    ("arm11_setup_ctrs: The values of int mask and enables are %x, %x\n",
		     pmu->int_mask[i], pmu->int_enable);

		pr_debug("arm11_setup_ctrs: counter%d %#08x from %#08lx\n", i,
			 read_counter(i), counter_config[i].count);
	}

	return 0;
}

/*!
 * function is used to check the status of the ARM11 performance counters
 */
static void inline arm11_check_ctrs(void)
{
	int i;
	u32 pmnc = read_pmnc();

	for (i = CCNT; i <= PMN1; i++) {
		if (!(pmu->int_mask[i] & pmu->int_enable)) {
			pr_debug
			    ("arm11_check_ctrs: The values of int mask and enables are %x, %x",
			     pmu->int_mask[i], pmu->int_enable);
			continue;
		}

		if (pmnc & pmu->cnt_ovf[i]) {
			pr_debug("arm11_check_ctrs: overflow detected\n");
			results[i].ovf++;
		}
	}

	pmnc &= ~(PMU_ENABLE | pmu->cnt_ovf[PMN0] | pmu->cnt_ovf[PMN1] |
		  pmu->cnt_ovf[CCNT]);
	write_pmnc(pmnc);
}

/*!
 * function is the interrupt service handler for the ARM11 performance counters
 */
static irqreturn_t arm11_pmu_interrupt(int irq, void *arg, struct pt_regs *regs)
{
	unsigned long pc = profile_pc(regs);
	int i, is_kernel = !user_mode(regs);
	u32 pmnc;

	arm11_check_ctrs();

	for (i = CCNT; i < MAX_COUNTERS; i++) {
		if (!results[i].ovf)
			continue;

		pr_debug("arm11_pmu_interrupt: writing to file\n");
		write_counter(i, -(u32) results[i].reset_counter);
		oprofile_add_sample(pc, is_kernel, i, smp_processor_id());
		results[i].ovf--;
	}

	pmnc = read_pmnc() | PMU_ENABLE;
	write_pmnc(pmnc);

	return IRQ_HANDLED;
}

/*!
 * function used to start the ARM11 performance counters
 */
static void arm11_pmu_stop(void)
{
	u32 pmnc = read_pmnc();

	pmnc &= ~PMU_ENABLE;
	write_pmnc(pmnc);

	free_irq(ARM11_PMU_IRQ, results);
}

/*!
 * function used to start the ARM11 performance counters
 */
static int arm11_pmu_start(void)
{
	int ret;
	u32 pmnc = read_pmnc();

	ret = request_irq(ARM11_PMU_IRQ, arm11_pmu_interrupt, SA_INTERRUPT,
			  "ARM11 PMU", (void *)results);
	pr_debug("requested IRQ\n");

	if (ret < 0) {
		printk(KERN_ERR
		       "oprofile: unable to request IRQ%d for ARM11 PMU\n",
		       ARM11_PMU_IRQ);
		return ret;
	}

	pmnc |= pmu->int_enable;
	pmnc |= PMU_ENABLE;

	write_pmnc(pmnc);
	pr_debug("arm11_pmu_start: pmnc: %#08x mask: %08x\n", pmnc,
		 pmu->int_enable);
	return 0;
}

/*!
 * function detect the ARM11 performance counters
 */
static int arm11_detect_pmu(void)
{
	int ret = 0;
	u32 id;

	id = (read_cpuid(CPUID_ID) >> 0x10) & 0xF;

	switch (id) {
	case 7:
		pmu = &pmu_parms[0];
		break;
	default:
		ret = -ENODEV;
		break;
	}

	if (!ret) {
		op_arm_spec.name = pmu->name;
		op_arm_spec.num_counters = pmu->num_counters;
		pr_debug("arm11_detect_pmu: detected %s PMU\n", pmu->name);
	}

	return ret;
}

struct op_arm_model_spec op_arm_spec = {
	.init = arm11_detect_pmu,
	.setup_ctrs = arm11_setup_ctrs,
	.start = arm11_pmu_start,
	.stop = arm11_pmu_stop,
};
