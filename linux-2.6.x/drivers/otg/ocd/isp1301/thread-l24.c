/*
 * otg/ocd/isp1301/thread-l24.c -- MX21ADS ISP1301 Transceiver Controller driver 
 * @(#) balden@seth2.belcarratech.com|otg/ocd/isp1301/thread-l24.c|20051116204959|43306
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@lbelcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open Src compliance.
 */
/*
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */
/*!
 * @file otg/ocd/isp1301/thread-l24.c
 * @brief Implement a kernel thread for checking ISP1301.
 *
 *
 * @ingroup ISP1301TCD
 */

#include <otg/pcd-include.h>
#include <otg/otg-module.h>
#include <linux/pci.h>
//#include <asm/arch/mx2.h>
#include <otghw/isp1301-hardware.h>
//#include "mx2.h"
//#include "otghw/mx2-hardware.h"


/* isp1301_bh
 */
extern void isp1301_bh(void *arg);

/* ********************************************************************************************* */
/* ********************************************************************************************* */

/* I2C Thread
 * The mx21 USBOTG module does not seem to be able to keep uptodate with the OTG Transceiver
 * registers. This kernel thread will iterated endlessly reading the appropriate registers.
 *
 * If this is successful we can assume that the USBOTG module will correctly monitor the appropriate
 * registers and allow us to follow them via the normal USBOTG interrupts and registers.
 *
 * Otherwise we can monitor here and issue the otg events directly.
 */
//#define ISP1301_THREAD_TIMEOUT_EN (10)
#define ISP1301_THREAD_TIMEOUT_EN (1*HZ)
#define ISP1301_THREAD_TIMEOUT_DIS (1*HZ)

int isp1301_terminate;
int mx2_pid;
wait_queue_head_t isp1301_wait;
static DECLARE_COMPLETION(mx2_exited);
int isp1301_en;

void (*isp1301_bh_proc)(void *);

/*! isp1301_thread - implement a kernel task to monitor isp1301 for changes
 */
int isp1301_thread(void *data)
{
        u8 int_src_saved = 0, otg_status_saved = 0, int_src, otg_status;
        TRACE_MSG0(TCD, "--");
	daemonize();
	reparent_to_init();
	sprintf(current->comm, "isp1301_thread");
                
        /* loop until told to terminate
         */
	while (!isp1301_terminate) {

                #if 0
                // XXX TESTING
                static int count = 0;
                count++;
                if (!(count % 5))
                        isp1301_dp_pullup_func(tcd_instance->otg, !(count % 10)? 1 : 2);
                #endif

                /* sleep for a while if not enabled
                 */
                if (!isp1301_en) {
                        TRACE_MSG1(TCD, "WAITING: %d", ISP1301_THREAD_TIMEOUT_DIS);
                        interruptible_sleep_on_timeout(&isp1301_wait, ISP1301_THREAD_TIMEOUT_DIS);
                        continue;
                }

                /* update 1301
                 */
                if (isp1301_bh_proc)
                        isp1301_bh_proc(NULL);
                
                //TRACE_MSG1(TCD, "SLEEPING: %d", ISP1301_THREAD_TIMEOUT_EN);
                interruptible_sleep_on_timeout(&isp1301_wait, ISP1301_THREAD_TIMEOUT_EN);
                //TRACE_MSG0(TCD, "RUNNING");
	}
	TRACE_MSG0(TCD, "Exiting");
	complete_and_exit(&mx2_exited, 0);
}

extern int isp1301_bh_first;
/*! isp1301_thread_wakeup - wakeup the isp1301 task
 */
void isp1301_thread_wakeup(int enabled, int first)
{
        TRACE_MSG0(TCD, "--");
        isp1301_bh_first |= first;
        isp1301_en = enabled;
	wake_up(&isp1301_wait);
}

/* ********************************************************************************************* */

/*! isp1301_thread_init - initial tcd setup
 * Allocate interrupts and setup hardware.
 */
int isp1301_thread_init (void (bh_proc)(void *))
{
	isp1301_terminate = 0;
        TRACE_MSG0(TCD, "--");
        isp1301_bh_proc = bh_proc;
	init_waitqueue_head(&isp1301_wait);
	THROW_IF((mx2_pid = kernel_thread(&isp1301_thread, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGHAND)) < 0, error);
        isp1301_thread_wakeup(0, 0);

        CATCH(error) {
                return -EINVAL;
        }
        return 0;
}

/*! isp1301_thread_exit - de-initialize
 */
void isp1301_thread_exit (wait_queue_head_t *wait_queue)
{
        TRACE_MSG0(TCD, "MX2_MOD_TCD_EXIT - terminating");
        isp1301_bh_proc = NULL;
	isp1301_terminate = 1;
        if (wait_queue)
                wake_up_interruptible(wait_queue);                              // wakeup waiting bottom half handler
        isp1301_thread_wakeup(0, 0);
	wait_for_completion(&mx2_exited);
        TRACE_MSG0(TCD, "MX2_MOD_TCD_EXIT - terminated");
}

