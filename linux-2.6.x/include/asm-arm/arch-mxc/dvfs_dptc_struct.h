/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file dvfs_dptc_struct.h
 *
 * @brief MXC dvfs & dptc structure definitions file.
 *
 * @ingroup PM
 */
#ifndef __DVFS_DPTC_STRUCT_H__
#define __DVFS_DPTC_STRUCT_H__

#include <asm/arch/pm_api.h>

/*!
 * Number of entries in the DPTC log buffer
 */
#define LOG_ENTRIES 1024

/*!
 * Log buffer Structure.\n
 * This structure records the DPTC changes. \n
 * This structure can be read by the user using the proc file system DPTC read entry.
 */
typedef struct {
	/*!
	 * Index to the head of the log buffer
	 */
	int head;

	/*!
	 * Index to the tail of the log buffer
	 */
	int tail;

	/*!
	 * Mutex to allow access to the log buffer
	 */
	struct semaphore mutex;

	/*!
	 * Array of log buffer entries
	 */
	dptc_log_entry_s entries[LOG_ENTRIES];
} dptc_log_s;

/*!
 * DPTC driver data structure.\n
 * Holds all driver parameters and data structures.
 */
typedef struct {
	/*!
	 * This variable holds the current frequency index
	 */
	int current_freq_index;

	/*!
	 * Boolean variable, if TRUE the DPTC module is enabled
	 * if FALSE the DPTC module is disabled
	 */
	int dptc_is_active;

#ifdef CONFIG_ARCH_MX3
	/*!
	 * Boolean variable, if TRUE the DVFS module is enabled
	 * if FALSE the DPTC module is disabled
	 */
	int dvfs_is_active;
#endif

	/*!
	 * Boolean variable, if TRUE the DPTC module is suspended
	 */
	int suspended;

	unsigned char rc_state;

	/*!
	 * Pointer to the DVFS & DPTC translation table
	 */
	dvfs_dptc_tables_s *dvfs_dptc_tables_ptr;

	/*!
	 * The DPTC log buffer
	 */
	dptc_log_s dptc_log_buffer;

	/*!
	 * The DVFS log buffer
	 */
	unsigned char *dvfs_log_buffer;

	/*!
	 * The DVFS log buffer physical address (for SDMA)
	 */
	dma_addr_t dvfs_log_buffer_phys;

#ifdef CONFIG_MXC_DVFS_SDMA
	/*!
	 * SDMA channel number
	 */
	int sdma_channel;

	/*!
	 * This holds the previous working point
	 */
	int prev_wp;

	/*!
	 * Wait entry for predictive DVFS
	 */
	wait_queue_head_t dvfs_pred_wait;
#endif

	/*!
	 * This holds the current DVFS mode
	 */
	unsigned int dvfs_mode;

	/*!
	 * Log buffer read pointer
	 */
	unsigned char *read_ptr;

	/*
	 * Number of characters in log buffer
	 */
	int chars_in_buffer;
} dvfs_dptc_params_s;

#ifdef CONFIG_ARCH_MX3
/*!
 * This struct contains the array with values of supported frequencies in Hz
 */
typedef struct {
	/*
	 * Number of supported states
	 */
	unsigned int num_of_states;
	/*!
	 * Array of frequencies
	 */
	unsigned int *freqs;
} dvfs_states_table;
#endif

/*
 * if not defined define TREU and FALSE values.
 */
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif				/* TRUE */

#endif
