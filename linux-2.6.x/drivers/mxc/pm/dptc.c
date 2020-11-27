/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*!
 * @file dptc.c
 *
 * @brief Driver for the Freescale Semiconductor MXC DPTC module.
 *
 * The DPTC driver
 * driver is designed as a character driver which interacts with the MXC DPTC
 * hardware. Upon initialization, the DPTC driver initializes the DPTC hardware
 * sets up driver nodes attaches to the DPTC interrupt and initializes internal
 * data structures. When the DPTC interrupt occurs the driver checks the cause
 * of the interrupt (lower voltage, increase voltage or emergency) and changes
 * the CPU voltage according to translation table that is loaded into the driver
 * (the voltage changes are done by calling some routines in the mc13783 driver).
 * The driver read method is used to read the currently loaded DPTC translation
 * table and the write method is used in-order to update the translation table.
 * Driver ioctls are used to change driver parameters and enable/disable the
 * DPTC operation.
 *
 * @ingroup PM
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <asm/semaphore.h>
#include <linux/jiffies.h>
#include <linux/device.h>
#include <asm/arch/sdma.h>
#include <asm/arch/clock.h>
#include "mc13783_power.h"

/*
 * Comment EVB_DEBUG to disable debug messages
 */
/* #define EVB_DEBUG 1 */

/*
 * Module header file
 */
#include "dptc.h"

#ifdef CONFIG_ARCH_MX3
#include "../../../arch/arm/mach-mx3/crm_regs.h"
#endif
#ifdef CONFIG_ARCH_MXC91331
#include "../../../arch/arm/mach-mxc91321/crm_regs.h"
#endif
#ifdef CONFIG_ARCH_MXC91321
#include "../../../arch/arm/mach-mxc91321/crm_regs.h"
#endif

/*!
 * The dvfs_dptc_params structure holds all the internal DPTC driver parameters
 * (current working point, current frequency, translation table and DPTC
 * log buffer).
 */
static dvfs_dptc_params_s *dvfs_dptc_params;

static struct work_struct dptc_work;

#ifndef CONFIG_MXC_DVFS_SDMA
static unsigned long ptvai;
#endif

/*!
 * Enable DPTC hardware
 */
static void dptc_enable_dptc(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DPTEN,
			   MXC_CCM_PMCR0_DPTEN);
}

/*!
 * Disable DPTC hardware
 */
static void dptc_disable_dptc(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DPTEN, 0);
}

/*!
 * Mask DPTC interrupt
 */
static void dptc_mask_dptc_int(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_PTVAIM,
			   MXC_CCM_PMCR0_PTVAIM);
}

/*!
 * Unmask DPTC interrupt
 */
static void dptc_unmask_dptc_int(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_PTVAIM, 0);
}

/*!
 * Read the PTVAI bits from the CCM
 *
 * @return  PTVAI bits value
 */
static unsigned long dptc_get_ptvai(void)
{
	return (mxc_ccm_get_reg(MXC_CCM_PMCR0) & MXC_CCM_PMCR0_PTVAI_MASK)
	    >> MXC_CCM_PMCR0_PTVAI_OFFSET;
}

/*!
 * Clear DCR bits of the CCM
 */
static void dptc_clear_dcr(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DCR, 0);
}

/*!
 * If value equal to PTVAI bits indicates working point decrease
 */
#define DPTC_DECREASE		(unsigned long)0x1

/*!
 * If value equal to PTVAI bits indicates working point increase
 */
#define DPTC_INCREASE		(unsigned long)0x2

/*!
 * If value equal to PTVAI bits indicates working point increase to maximum
 */
#define DPTC_EMERG		(unsigned long)0x3

#ifdef CONFIG_MXC_DVFS
#ifndef CONFIG_MXC_DVFS_SDMA
/*
 * MCU will get DPTC interrupt
 */
static void dptc_set_ptvis(void)
{
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_PTVIS,
			   MXC_CCM_PMCR0_PTVIS);
}
#endif
#endif

/*!
 * This function enables the DPTC reference circuits.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 * @param    rc_state  each high bit specifies which
 *                     reference circuite to enable
 * @return   0 on success, error code on failure
 */
int enable_ref_circuits(dvfs_dptc_params_s * params, unsigned char rc_state)
{
	int ret_val;

	if (params->dptc_is_active == FALSE) {
		params->rc_state = rc_state;

		if (rc_state & 0x1) {
			mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DRCE0,
					   MXC_CCM_PMCR0_DRCE0);
#ifdef EVB_DEBUG
			printk("Ref circuit 0 enabled\n");
#endif
		}
		if (rc_state & 0x2) {
			mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DRCE1,
					   MXC_CCM_PMCR0_DRCE1);
#ifdef EVB_DEBUG
			printk("Ref circuit 1 enabled\n");
#endif
		}
		if (rc_state & 0x4) {
			mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DRCE2,
					   MXC_CCM_PMCR0_DRCE2);
#ifdef EVB_DEBUG
			printk("Ref circuit 2 enabled\n");
#endif
		}
		if (rc_state & 0x8) {
			mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DRCE3,
					   MXC_CCM_PMCR0_DRCE3);
#ifdef EVB_DEBUG
			printk("Ref circuit 3 enabled\n");
#endif
		}

		ret_val = 0;
	} else {
		ret_val = -EINVAL;
	}

	return ret_val;
}

/*!
 * This function disables the DPTC reference circuits.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 * @param    rc_state  each high bit specifies which
 *                     reference circuite to disable
 * @return   0 on success, error code on failure
 */
int disable_ref_circuits(dvfs_dptc_params_s * params, unsigned char rc_state)
{
	int ret_val;

	if (params->dptc_is_active == FALSE) {
		params->rc_state &= ~rc_state;

		if (rc_state & 0x1) {
			mxc_ccm_modify_reg(MXC_CCM_PMCR0,
					   MXC_CCM_PMCR0_DRCE0, 0);
#ifdef EVB_DEBUG
			printk("Ref circuit 0 disabled\n");
#endif
		}
		if (rc_state & 0x2) {
			mxc_ccm_modify_reg(MXC_CCM_PMCR0,
					   MXC_CCM_PMCR0_DRCE1, 0);
#ifdef EVB_DEBUG
			printk("Ref circuit 1 disabled\n");
#endif
		}
		if (rc_state & 0x4) {
			mxc_ccm_modify_reg(MXC_CCM_PMCR0,
					   MXC_CCM_PMCR0_DRCE2, 0);
#ifdef EVB_DEBUG
			printk("Ref circuit 2 disabled\n");
#endif
		}
		if (rc_state & 0x8) {
			mxc_ccm_modify_reg(MXC_CCM_PMCR0,
					   MXC_CCM_PMCR0_DRCE3, 0);
#ifdef EVB_DEBUG
			printk("Ref circuit 3 disabled\n");
#endif
		}

		ret_val = 0;
	} else {
		ret_val = -EINVAL;
	}

	return ret_val;
}

static void dptc_workqueue_handler(void *arg)
{
	dvfs_dptc_params_s *params;

	params = (dvfs_dptc_params_s *) arg;

#ifdef EVB_DEBUG
	printk("In %s: PTVAI = %d\n", __FUNCTION__, dptc_get_ptvai());
	printk("PMCR0 = 0x%x ", mxc_ccm_get_reg(MXC_CCM_PMCR0));
	printk("DCVR0 = 0x%x ", mxc_ccm_get_reg(MXC_CCM_DCVR0));
	printk("DCVR1 = 0x%x ", mxc_ccm_get_reg(MXC_CCM_DCVR1));
	printk("DCVR2 = 0x%x ", mxc_ccm_get_reg(MXC_CCM_DCVR2));
	printk("DCVR3 = 0x%x ", mxc_ccm_get_reg(MXC_CCM_DCVR3));
	printk("PTVAI = 0x%x\n", ptvai);
#endif

#ifndef CONFIG_MXC_DVFS_SDMA
	switch (ptvai) {
		/* Chip working point has decreased, lower working point by one */
	case DPTC_DECREASE:
		set_dptc_wp(params, params->dvfs_dptc_tables_ptr->curr_wp + 1);
		break;

		/* Chip working point has increased, raise working point by one */
	case DPTC_INCREASE:
		set_dptc_wp(params, params->dvfs_dptc_tables_ptr->curr_wp - 1);
		break;

		/*
		 * Chip working point has increased dramatically,
		 * raise working point to maximum */
	case DPTC_EMERG:
		set_dptc_wp(params, params->dvfs_dptc_tables_ptr->curr_wp - 1);
		break;

		/* Unknown interrupt cause */
	default:
		break;
	}
#else
	set_dptc_wp(params, params->dvfs_dptc_tables_ptr->curr_wp);
	mxc_ccm_modify_reg(MXC_CCM_PMCR0, MXC_CCM_PMCR0_DPVV,
			   MXC_CCM_PMCR0_DPVV);
#endif

	/*
	 * If the DPTC module is still active, re-enable
	 * the DPTC hardware
	 */
	if (params->dptc_is_active) {
		dptc_enable_dptc();
		dptc_unmask_dptc_int();
	}
}

/*!
 * This function is the DPTC Interrupt handler.
 * This function wakes-up the dptc_workqueue_handler function that handles the
 * DPTC interrupt.
 */
void dptc_irq(void)
{
#ifndef CONFIG_MXC_DVFS_SDMA
	ptvai = dptc_get_ptvai();

#ifdef EVB_DEBUG
	printk("Got DPTC interrupt on %d ", (int)jiffies);
	printk("ptvai = 0x%x (0x%x)!!!!!!!\n", ptvai,
	       mxc_ccm_get_reg(MXC_CCM_PMCR0));
#endif

	if (ptvai != 0) {
		dptc_mask_dptc_int();
		dptc_disable_dptc();

		schedule_delayed_work(&dptc_work, 0);
	}
#else
	schedule_delayed_work(&dptc_work, 0);
#endif
}

/*!
 * This function updates the CPU voltage, produced by mc13783, by calling mc13783
 * driver functions.
 *
 * @param    dvfs_dptc_tables_ptr    pointer to the DPTC translation table.
 * @param    wp			current wp value.
 *				frequency.
 *
 */
void set_pmic_voltage(dvfs_dptc_tables_s * dvfs_dptc_tables_ptr, int wp)
{
	/* Call mc13783 functions */
	struct t_switcher_conf sw1a_conf;
#ifdef CONFIG_MXC_DVFS
	struct t_switcher_conf sw1b_conf;
#endif

	/* Normal mode setting */
	mc13783_power_get_switcher(SW_SW1A, &sw1a_conf);
	sw1a_conf.sw_setting = dvfs_dptc_tables_ptr->wp[wp].pmic_values[0];

#ifdef CONFIG_MXC_DVFS
	mc13783_power_get_switcher(SW_SW1B, &sw1b_conf);

	sw1b_conf.sw_setting_dvs = dvfs_dptc_tables_ptr->wp[wp].pmic_values[2];
	sw1a_conf.sw_setting_dvs = dvfs_dptc_tables_ptr->wp[wp].pmic_values[1];
	sw1b_conf.sw_setting_standby =
	    dvfs_dptc_tables_ptr->wp[wp].pmic_values[3];
#endif

	mc13783_power_conf_switcher(SW_SW1A, &sw1a_conf);

#ifdef CONFIG_MXC_DVFS
	mc13783_power_conf_switcher(SW_SW1B, &sw1b_conf);
#endif

#ifdef CONFIG_ARCH_MX3
#ifdef EVB_DEBUG
	printk("DPVV = 0x%x (0x%x)\n",
	       mxc_ccm_get_reg(MXC_CCM_PMCR0) & MXC_CCM_PMCR0_DPVV,
	       mxc_ccm_get_reg(MXC_CCM_PMCR0));
#endif				/* EVB_DEBUG */
#endif				/* CONFIG_ARCH_MX3 */
}

/*!
 * This function enables the DPTC module. this function updates the DPTC
 * thresholds, updates the mc13783, unmasks the DPTC interrupt and enables
 * the DPTC module
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return      0 if DPTC module was enabled else returns -EINVAL.
 */
int start_dptc(dvfs_dptc_params_s * params)
{
	int freq_index = 0;

	/* Check if DPTC module isn't already active */
	if (params->dptc_is_active == FALSE) {

		/*
		 * Set the DPTC thresholds and mc13783 voltage to
		 * correspond to the current working point and frequency.
		 */
		set_pmic_voltage(params->dvfs_dptc_tables_ptr,
				 params->dvfs_dptc_tables_ptr->curr_wp);

#ifdef CONFIG_MXC_DVFS
		freq_index = dvfs_get_dvsup();
#endif

		update_dptc_thresholds(params->dvfs_dptc_tables_ptr,
				       params->dvfs_dptc_tables_ptr->curr_wp,
				       freq_index);

		enable_ref_circuits(params, params->rc_state);
		disable_ref_circuits(params, ~params->rc_state);

		/* Mark DPCT module as active */
		params->dptc_is_active = TRUE;

		/* Enable the DPTC module and unmask the DPTC interrupt */
		dptc_enable_dptc();
		dptc_unmask_dptc_int();

		return 0;
	}

	/* DPTC module already active return error */
	return -EINVAL;
}

/*!
 * This function disables the DPTC module.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return      0 if DPTC module was disabled else returns -EINVAL.
 */
int stop_dptc(dvfs_dptc_params_s * params)
{
	/* Check if DPTC module isn't already disabled */
	if (params->dptc_is_active != FALSE) {

		/* Disable the DPTC module and mask the DPTC interrupt */
		dptc_disable_dptc();
		dptc_mask_dptc_int();

		/* Set working point 0 */
		set_dptc_wp(params, 0);

		/* Mark DPCT module as inactive */
		params->dptc_is_active = FALSE;

		return 0;
	}

	/* DPTC module already disabled, return error */
	return -EINVAL;
}

/*!
 * This function updates the drivers current working point index. This index is
 * used for access the current DTPC table entry and it corresponds to the
 * current CPU working point measured by the DPTC hardware.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 * @param    new_wp	New working point index value to be set.
 *
 */
void set_dptc_wp(dvfs_dptc_params_s * params, int new_wp)
{
	int freq_index = 0;

#ifdef CONFIG_MXC_DVFS
	freq_index = dvfs_get_dvsup();
#endif

	/*
	 * Check if new index is smaller than the maximal working point
	 * index in the DPTC translation table and larger that 0.
	 */
	if ((new_wp < params->dvfs_dptc_tables_ptr->wp_num)
	    && (new_wp >= 0)) {
		/* Set current working point index to new index */
		params->dvfs_dptc_tables_ptr->curr_wp = new_wp;
	}

	/*
	 * Check if new index is larger than the maximal working point index in
	 * the DPTC translation table.
	 */
	if (new_wp >= params->dvfs_dptc_tables_ptr->wp_num) {
		/*
		 * Set current working point index to maximal working point
		 * index in the DPTC translation table.
		 */
		params->dvfs_dptc_tables_ptr->curr_wp =
		    params->dvfs_dptc_tables_ptr->wp_num - 1;
	}

	/* Check if new index is smaller than 0. */
	if (new_wp < 0) {
		/* Set current working point index to 0 (minimal value) */
		params->dvfs_dptc_tables_ptr->curr_wp = 0;
	}
#ifndef CONFIG_MXC_DVFS_SDMA
	/* Update the DPTC hardware thresholds */
	update_dptc_thresholds(params->dvfs_dptc_tables_ptr,
			       params->dvfs_dptc_tables_ptr->curr_wp,
			       freq_index);
#else
	params->prev_wp = params->dvfs_dptc_tables_ptr->curr_wp;
#endif

	/* Update the mc13783 voltage */
	set_pmic_voltage(params->dvfs_dptc_tables_ptr,
			 params->dvfs_dptc_tables_ptr->curr_wp);

	/* Write DPTC changes in the DPTC log buffer */
	add_dptc_log_entry(params, &params->dptc_log_buffer,
			   params->dvfs_dptc_tables_ptr->curr_wp, freq_index);

#ifdef EVB_DEBUG
	printk("Current wp: %d\n", params->dvfs_dptc_tables_ptr->curr_wp);
#endif
}

/*!
 * This function updates the DPTC threshold registers.
 *
 * @param    dvfs_dptc_tables_ptr    pointer to the DPTC translation table.
 * @param    wp			current wp value.
 * @param    freq_index		translation table index of the current CPU
 *				frequency.
 *
 */
void update_dptc_thresholds(dvfs_dptc_tables_s * dvfs_dptc_tables_ptr,
			    int wp, int freq_index)
{
	dcvr_state *dcvr;

#ifdef CONFIG_MXC_DVFS_SDMA

	dcvr_state **dcvr_arr;
	dcvr_state *curr_freq_dcvr;

	dcvr_arr = dvfs_dptc_tables_ptr->dcvr;
	dcvr_arr = sdma_phys_to_virt((unsigned long)dcvr_arr);
	curr_freq_dcvr = dcvr_arr[freq_index];
	curr_freq_dcvr = sdma_phys_to_virt((unsigned long)curr_freq_dcvr);
	dcvr = &curr_freq_dcvr[wp];
#else
	/* Calculate current table entry offset in the DPTC translation table */
	dcvr = &dvfs_dptc_tables_ptr->dcvr[freq_index][wp];
#endif
	/* Update DPTC threshold registers */
	mxc_ccm_modify_reg(MXC_CCM_DCVR0, 0xffffffff, dcvr->dcvr_reg[0].AsInt);
	mxc_ccm_modify_reg(MXC_CCM_DCVR1, 0xffffffff, dcvr->dcvr_reg[1].AsInt);
	mxc_ccm_modify_reg(MXC_CCM_DCVR2, 0xffffffff, dcvr->dcvr_reg[2].AsInt);
	mxc_ccm_modify_reg(MXC_CCM_DCVR3, 0xffffffff, dcvr->dcvr_reg[3].AsInt);
}

/*!
 * This function increments a log buffer index (head or tail)
 * by the value of val.
 *
 * @param    index	pointer to the DPTC log buffer index that
 *			we wish to change.
 * @param    val	the value in which the index should be incremented.
 *
 */
static void inc_log_index(int *index, int val)
{
	*index = (*index + val) % LOG_ENTRIES;
}

/*!
 * This function returns the number of entries in the DPTC log buffer.
 *
 * @param    dptc_log	pointer to the DPTC log buffer structure.
 *
 * @return   number of log buffer entries.
 *
 */
static int get_entry_count(dptc_log_s * dptc_log)
{
	return ((dptc_log->head - dptc_log->tail + LOG_ENTRIES) % LOG_ENTRIES);
}

/*!
 * This function is used by the proc file system to read the DPTC log buffer.
 * Each time the DPTC proc file system file is read this function is called
 * and returns the data written in the log buffer.
 *
 * @param    buf	pointer to the buffer the data should be written to.
 * @param    start	pointer to the pointer where the new data is
 *                      written to.
 *			procedure should update the start pointer to point to
 *			where in the buffer the data was written.
 * @param    offset	current offset in the DPTC proc file.
 * @param    count	number of bytes to read.
 * @param    eof	pointer to eof flag. should be set to 1 when
 *                      reaching eof.
 * @param    data	driver specific data pointer.
 *
 * @return   number byte read from the log buffer.
 *
 */
static int read_log(char *buf, char **start, off_t offset, int count,
		    int *eof, void *data)
{
	int entries_to_read;
	int num_of_entries;
	int entries_to_end_of_buffer, entries_left;
	void *entry_ptr;
	char *buf_ptr;
	dvfs_dptc_params_s *params;

	params = (dvfs_dptc_params_s *) data;

	/* Calculate number of log entries to read */
	entries_to_read = count / sizeof(dptc_log_entry_s);
	/* Get the number of current log buffer entries */
	num_of_entries = get_entry_count(&params->dptc_log_buffer);

	/*
	 * If number of entries to read is larger that the number of entries
	 * in the log buffer set number of entries to read to number of
	 * entries in the log buffer and set eof flag to 1
	 */
	if (num_of_entries < entries_to_read) {
		entries_to_read = num_of_entries;
		*eof = 1;
	}

	/*
	 * Down the log buffer mutex to exclude others from reading and
	 * writing to the log buffer.
	 */
	down_interruptible(&params->dptc_log_buffer.mutex);

	if (num_of_entries == 0 && offset == 0) {
		inc_log_index(&params->dptc_log_buffer.tail, -1);
		num_of_entries++;
		entries_to_read++;
	}

	/* get the pointer of the last (oldest) entry in the log buffer */
	entry_ptr = (void *)&params->dptc_log_buffer.
	    entries[params->dptc_log_buffer.tail];

	/* Check if tail index wraps during current read */
	if ((params->dptc_log_buffer.tail + entries_to_read) < LOG_ENTRIES) {
		/* No tail wrap around copy data from log buffer to buf */
		memcpy(buf, entry_ptr,
		       (entries_to_read * sizeof(dptc_log_entry_s)));
	} else {
		/*
		 * Tail wrap around.
		 * First copy data from current position until end of buffer,
		 * after that copy the rest from start of the log buffer.
		 */
		entries_to_end_of_buffer = LOG_ENTRIES -
		    params->dptc_log_buffer.tail;
		memcpy(buf, entry_ptr,
		       (entries_to_end_of_buffer * sizeof(dptc_log_entry_s)));

		entry_ptr = (void *)&params->dptc_log_buffer.entries[0];
		buf_ptr = buf +
		    (entries_to_end_of_buffer * sizeof(dptc_log_entry_s));
		entries_left = entries_to_read - entries_to_end_of_buffer;
		memcpy(buf_ptr, entry_ptr,
		       (entries_left * sizeof(dptc_log_entry_s)));
	}

	/* Increment the tail index by the number of entries read */
	inc_log_index(&params->dptc_log_buffer.tail, entries_to_read);

	/* Up the log buffer mutex to allow access to the log buffer */
	up(&params->dptc_log_buffer.mutex);

	/* set start of data to point to buf */
	*start = buf;

	return (entries_to_read * sizeof(dptc_log_entry_s));
}

/*!
 * This function initializes the DPTC log buffer.
 *
 * @param    params	pointer to \b dvfs_dptc_params_s.
 * @param    dptc_log	pointer to the DPTC log buffer structure.
 *
 */
void init_dptc_log(dvfs_dptc_params_s * params, dptc_log_s * dptc_log)
{
	dptc_log->tail = 0;
	dptc_log->head = 0;

	/* initialize log buffer mutex used for accessing the log buffer */
	sema_init(&dptc_log->mutex, 1);

	/* add the first log buffer entry */
	add_dptc_log_entry(params, dptc_log,
			   params->dvfs_dptc_tables_ptr->curr_wp,
			   params->current_freq_index);
}

/*!
 * This function adds a new entry to the DPTC log buffer.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 * @param    dptc_log		pointer to the DPTC log buffer structure.
 * @param    wp			value of the working point index written
 *				to the log buffer.
 * @param    freq_index		value of the frequency index written to
 *				the log buffer.
 *
 * @return   number of log buffer entries.
 *
 */
void add_dptc_log_entry(dvfs_dptc_params_s * params,
			dptc_log_s * dptc_log, int wp, int freq_index)
{
	/*
	 * Down the log buffer mutex to exclude others from reading and
	 * writing to the log buffer.
	 */
	down_interruptible(&dptc_log->mutex);

	/* Write values to log buffer */
	dptc_log->entries[dptc_log->head].jiffies = jiffies;
	dptc_log->entries[dptc_log->head].wp = wp;

	dptc_log->entries[dptc_log->head].voltage = wp;

#ifdef CONFIG_MXC_DVFS
	freq_index = dptc_get_ptvai();
#endif

	dptc_log->entries[dptc_log->head].freq = freq_index;

	/* Increment the head index by 1 */
	inc_log_index(&dptc_log->head, 1);
	/* If head index reaches the tail increment the tail by 1 */
	if (dptc_log->head == dptc_log->tail) {
		inc_log_index(&dptc_log->tail, 1);
	}

	/* Up the log buffer mutex to allow access to the log buffer */
	up(&dptc_log->mutex);
}

/*!
 * This function initializes the DPTC hardware
 *
 * @param    params    pointer to the DPTC driver parameters structure.
 *
 */
int __init init_dptc_controller(dvfs_dptc_params_s * params)
{
	INIT_WORK(&dptc_work, dptc_workqueue_handler, params);

	if (create_proc_read_entry(PROC_NODE_NAME, 0,
				   NULL, read_log, params) == NULL) {
		/*
		 * Error creating proc file system entry.
		 * Exit and return error code
		 */
		printk(KERN_ERR "DPTC: Unable create proc entry");
		return -EFAULT;
	}

	/* Initialize the DPTC log buffer */
	init_dptc_log(params, &params->dptc_log_buffer);

	set_dptc_curr_freq(params, 0);
	set_dptc_wp(params, 0);

	/* By default all reference circuits are enabled */
	params->rc_state = DPTC_REF_CIRCUITS_STATUS;

	dptc_clear_dcr();
	/* Disable DPTC hardware and mask DPTC interrupt */
	dptc_disable_dptc();
	dptc_mask_dptc_int();

#ifdef CONFIG_MXC_DVFS
#ifndef CONFIG_MXC_DVFS_SDMA
	dptc_set_ptvis();
#endif
#endif
	dvfs_dptc_params = params;

	printk(KERN_INFO "DPTC controller initialized\n");

	return 0;
}

/*!
 * This function updates the drivers current frequency index.This index is
 * used for access the current DTPC table entry and it corresponds to the
 * current CPU frequency (each CPU frequency has a separate index number
 * according to the loaded DPTC table).
 *
 * @param    params       pointer to the DVFS & DPTC driver parameters structure.
 * @param    freq_index	  New frequency index value to be set.
 *
 * @return      0 if the frequency index was updated (the new index is a
 *		valid index and the DPTC module isn't active) else returns
 *              -EINVAL.
 *
 */
int set_dptc_curr_freq(dvfs_dptc_params_s * params, unsigned int freq_index)
{
	/*
	 * Check if the new index value is a valid frequency index (smaller
	 * than the maximal index in the DPTC table) and if the DPTC module
	 * is disabled.
	 */
	if ((freq_index < params->dvfs_dptc_tables_ptr->dvfs_state_num)
	    && (params->dptc_is_active == FALSE)) {
		/*
		 * Index is valid and DPTC module is
		 * disabled -> change frequency index.
		 */
		params->current_freq_index = freq_index;
		add_dptc_log_entry(params, &params->dptc_log_buffer,
				   params->dvfs_dptc_tables_ptr->curr_wp,
				   params->current_freq_index);

		return 0;
	}

	/* Invalid index or DPTC module is active -> return error */
	return -EINVAL;
}

#ifdef CONFIG_MXC_DVFS_SDMA
/*
 * DPTC SDMA callback.
 * Updates the mc13783 voltage
 *
 * @param    params       pointer to the DVFS & DPTC driver parameters structure.
 */
void dptc_sdma_callback(dvfs_dptc_params_s * params)
{
	printk("In %s: params->dvfs_dptc_tables_ptr->curr_wp = %d\n",
	       __FUNCTION__, params->dvfs_dptc_tables_ptr->curr_wp);
}
#endif

/*!
 * This function is called to put the DPTC in a low power state.
 *
 * @param   dev   the device structure used to give information on which
 *                device to suspend (not relevant for DPTC)
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
int mxc_dptc_suspend(struct device *dev, u32 state, u32 level)
{
	switch (level) {
	case SUSPEND_POWER_DOWN:
#ifdef CONFIG_MXC_DPTC
		if (dvfs_dptc_params->dptc_is_active) {
			dptc_disable_dptc();
			set_dptc_wp(dvfs_dptc_params,
				    dvfs_dptc_params->
				    dvfs_dptc_tables_ptr->curr_wp - 1);
		}
#endif

		break;
	case SUSPEND_SAVE_STATE:
		/* Nothing to be done for DPTC */
		break;
	case SUSPEND_DISABLE:
		dvfs_dptc_params->suspended = 1;
		break;
	}
	return 0;
}

/*!
 * This function is called to resume the DPTC from a low power state.
 *
 * @param   dev   the device structure used to give information on which
 *                device to suspend (not relevant for DPTC)
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
int mxc_dptc_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_ENABLE:
		dvfs_dptc_params->suspended = 0;
		break;
	case RESUME_RESTORE_STATE:
		/* Nothing to be done for DPTC */
		break;
	case RESUME_POWER_ON:
#ifdef CONFIG_MXC_DPTC
		if (dvfs_dptc_params->dptc_is_active) {
			dptc_enable_dptc();
		}
#endif
		break;
	}
	return 0;
}
