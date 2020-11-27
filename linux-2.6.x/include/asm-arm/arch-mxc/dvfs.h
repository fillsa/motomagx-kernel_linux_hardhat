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
 * @file dvfs.h
 *
 * @brief MXC dvfs header file.
 *
 * @ingroup PM
 */
#ifndef __DVFS_H__
#define __DVFS_H__

#include "dvfs_dptc_struct.h"

/*!
 * If value equal to FSVAI bits indicates working point decrease
 */
#define DVFS_DECREASE		(unsigned long)0x2

/*!
 * If value equal to FSVAI bits indicates working point increase
 */
#define DVFS_INCREASE		(unsigned long)0x1

/*!
 * If value equal to FSVAI bits indicates working point increase to maximum
 */
#define DVFS_EMERG		(unsigned long)0x3

/*!
 * This function is called for module initialization.
 * It initializes the driver data structures and sets up the DVFS hardware.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return   0 to indicate success else returns a negative number.
 *
 */
int __init init_dvfs_controller(dvfs_dptc_params_s * params);

/*!
 * This function enables the DVFS module. this function updates the DVFS
 * thresholds, updates the PMIC, unmasks the DVFS interrupt and enables
 * the DVFS module
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return      0 if DVFS module was enabled else returns -EINVAL.
 */
int start_dvfs(dvfs_dptc_params_s * params);

/*!
 * This function disables the DVFS module.
 *
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 *
 * @return      0 if DVFS module was enabled else returns -EINVAL.
 */
int stop_dvfs(dvfs_dptc_params_s * params);

/*!
 * This function turns on/off SW general purpose bits.
 * The argument's 4 LSBs represent the status of the bits.
 *
 * @param   arg  status of the SW general purpose bits
 *
 * @return 0 on success
 */
int set_sw_gp(unsigned char arg);

/*!
 * This function sets DVFS to monitor WFI signal
 *
 * @param   arg  0 - turn WFI off, 1 - turn WFI on
 * @return  0 on success, error code on fail
 */
int set_wfi(unsigned char arg);

#ifndef CONFIG_MXC_DVFS_SDMA
/*!
 * This function is the DVFS Interrupt handler.
 * @param    params    pointer to the DVFS & DPTC driver parameters structure.
 */
void dvfs_irq(dvfs_dptc_params_s * params);
#endif

/*!
 * This function changes the frequency if DVFS HW is disabled.
 * It gets id of the required state supported by DVFS table and
 * updates CCM.
 *
 * @param    dvfs_state_id  id of the DVFS state.
 * @return   0 on success, error code on fail
 */
int dvfs_set_state(int dvfs_state_id);

/*
 * Update tables of frequencies for DPM usage
 *
 * @param    dvfs_dptc_tables_ptr    pointer to the DVFS &
 *                                   DPTC translation table.
 */
void dvfs_update_freqs_table(dvfs_dptc_tables_s * dvfs_dptc_tables_ptr);

/*!
 * Read the DVSUP bits from the CCM
 */
unsigned long dvfs_get_dvsup(void);

#ifdef CONFIG_ARCH_MX3
/*!
 * This contains the array with values of supported frequencies in Hz.
 * The structure is used by DPM.
 */
extern dvfs_states_table *dvfs_states_tbl;
#endif

/*!
 * This define DVFS log buffer sample size (in bits)
 */
#define DVFS_LB_SAMPLE_SIZE 4

/*!
 * This define DVFS log buffer samples
 */
#define DVFS_LB_SIZE 1600

/*!
 * This define DVFS SDMA buffer descriptors number
 */
#define DVFS_LB_SDMA_BD 10

/*!
 * This defines default DVFS down threshold
 */
#define DVFS_DNTHR  18

/*!
 * This defines default DVFS up threshold
 */
#define DVFS_UPTHR  30

/*!
 * This defines default DVFS panic threshold
 */
#define DVFS_PNCTHR 63

#endif
