/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file dptc.h
 *
 * @brief MXC dptc header file.
 *
 * @ingroup PM
 */
#ifndef __DPTC_H__
#define __DPTC_H__

#include <asm/arch/dvfs_dptc_struct.h>

#ifdef CONFIG_MXC_DVFS
#include <asm/arch/dvfs.h>
#endif

/*!
 * DPTC proc file system entry name
 */
#define PROC_NODE_NAME	"dptc"

int __init init_dptc_controller(dvfs_dptc_params_s * params);

/*!
 * This function enables the DPTC module. this function updates the DPTC
 * thresholds, updates the PMIC, unmasks the DPTC interrupt and enables
 * the DPTC module
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return      0 if DPTC module was enabled else returns -EINVAL.
 */
int start_dptc(dvfs_dptc_params_s * params);
/*!
 * This function disables the DPTC module.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return      0 if DPTC module was disabled else returns -EINVAL.
 */
int stop_dptc(dvfs_dptc_params_s * params);
/*!
 * This function updates the drivers current working point index. This index is
 * used for access the current DTPC table entry and it corresponds to the
 * current CPU working point measured by the DPTC hardware.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 * @param    new_wp	New working point index value to be set.
 *
 */
void set_dptc_wp(dvfs_dptc_params_s * params, int new_wp);
/*!
 * This function updates the DPTC threshold registers.
 *
 * @param    dvfs_dptc_tables_ptr    pointer to the DPTC translation table.
 * @param    wp			current wp value.
 * @param    freq_index		translation table index of the current CPU
 *				frequency.
 *
 */
void update_dptc_thresholds(dvfs_dptc_tables_s * dptc_tables_ptr,
			    int wp, int freq_index);
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
			dptc_log_s * dptc_log, int wp, int freq_index);

/*!
 * This function updates the CPU voltage, produced by PMIC, by calling PMIC
 * driver functions.
 *
 * @param    dptc_tables_ptr    pointer to the DPTC translation table.
 * @param    wp			current wp value.
 */
void set_pmic_voltage(dvfs_dptc_tables_s * dptc_tables_ptr, int wp);

/*!
 * This function enables the DPTC reference circuits.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 * @param    rc_state  each high bit specifies which
 *                     reference circuite to enable.
 * @return   0 on success, error code on failure
 */
int enable_ref_circuits(dvfs_dptc_params_s * params, unsigned char rc_state);

/*!
 * This function disables the DPTC reference circuits.
  *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 * @param    rc_state  each high bit specifies which
 *                     reference circuite to disable
 * @return   0 on success, error code on failure
 */
int disable_ref_circuits(dvfs_dptc_params_s * params, unsigned char rc_state);

/*!
 * This function is the DPTC Interrupt handler.
 * This function wakes-up the dptc_workqueue_handler function that handles the
 * DPTC interrupt.
 */
void dptc_irq(void);

/*!
 * This function updates the drivers current frequency index.This index is
 * used for access the current DTPC table entry and it corresponds to the
 * current CPU frequency (each CPU frequency has a separate index number
 * according to the loaded DPTC table).
 *
 * @param    params      pointer to the DVFS & DPTC driver parameters structure.
 * @param    freq_index	 New frequency index value to be set.
 *
 * @return      0 if the frequency index was updated (the new index is a
 *		valid index and the DPTC module isn't active) else returns
 *              -EINVAL.
 *
 */
int set_dptc_curr_freq(dvfs_dptc_params_s * params, unsigned int freq_index);

#ifdef CONFIG_MXC_DVFS_SDMA
/*
 * DPTC SDMA callback.
 * Updates the PMIC voltage
 *
 * @param    params       pointer to the DVFS & DPTC driver parameters structure.
 */
void dptc_sdma_callback(dvfs_dptc_params_s * params);
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
int mxc_dptc_suspend(struct device *dev, u32 state, u32 level);

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
int mxc_dptc_resume(struct device *dev, u32 level);

#endif				/* __DPTC_H__ */
