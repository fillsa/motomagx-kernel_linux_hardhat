/*
 * FILE NAME ktools/kfistatic.c
 *
 * BRIEF MODULE DESCRIPTION
 *
 * Author: David Singleton dsingleton@mvista.com MontaVista Software, Inc.
 *
 * 2001-2004 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/types.h>
#include <linux/kfi.h>

extern void start_kernel(void);

extern void to_userspace(void);

static kfi_entry_t run0_log[MAX_RUN_LOG_ENTRIES];

kfi_run_t kfi_run0 = {
	0, 0,
	{ TRIGGER_FUNC_ENTRY, { func_addr: (void*)start_kernel } },
	{ TRIGGER_FUNC_ENTRY, { func_addr: (void*)to_userspace } },
	{ 500, 0, 0, 0, NULL, 0, {0} },
	run0_log, MAX_RUN_LOG_ENTRIES, 0, 0, NULL,
};

const int kfi_num_runs = 1;
kfi_run_t* kfi_first_run = &kfi_run0;
kfi_run_t* kfi_last_run = &kfi_run0;
