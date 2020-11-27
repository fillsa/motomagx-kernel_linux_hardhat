/*
 * otg/ocd/otglib/l26-ocd.c -- Generic PCI driver
 * @(#) balden@seth2.belcarratech.com|otg/ocd/otglib/l26-ocd.c|20051116205000|40554
 *
 *      Copyright (c) 2005 Belcarra
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
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
 */
/*!
 * @file otg/ocd/otglib/l26-ocd.c
 * @brief Generic L26 OCD Driver.
 *
 * Provides ticks via gettimeofday().
 *
 * Start timer is a dummy...
 *
 */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <linux/usb.h>
#include <linux/delay.h>

#include <otg/otg-compat.h>


/*
#include <core/hcd.h>
#include <otg/usbp-hub.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-utils.h>
*/


#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>  

/* Other includes*/
#include <linux/pci.h>
#include <linux/poll.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/dma.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-ocd.h>

/* ********************************************************************************************* */

/*!
 * l26_ocd_init() - used to initialize/enable or disable the tcd driver
 * @param otg
 * @param flag
 */
static void l26_ocd_init(struct otg_instance *otg, u8 flag)
{
        struct ocd_instance *ocd = otg->ocd;

        TRACE_MSG0(ocd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(ocd->TAG, "FS_OCD_EN SET");
                //l26_init();
                otg_event(otg, TCD_OK | ID_FLOAT, otg->ocd->TAG, "OCD_OK");
                break;
        case RESET:
                //l26_exit();
                TRACE_MSG0(ocd->TAG, "FS_OCD_EN RESET");
                otg_event(otg, TCD_OK | ID_FLOAT, otg->ocd->TAG, "OCD_OK");
                break;
        }
}



/* ********************************************************************************************* */

/*!     
 * l26_ocd_mod_init() - initial tcd setup
 * Allocate interrupts and setup hardware.
 */
static int l26_ocd_mod_init (void)
{       
        return 0;
}       

/*!
 * l26_ocd_mod_exit() - de-initialize
 */
static void l26_ocd_mod_exit (void)
{       
}



/* ********************************************************************************************* */
extern void l26_ocd_init (struct otg_instance *otg, u8 flag);
extern int l26_ocd_mod_init (void);
extern void l26_ocd_mod_exit (void);

/*!
 * l26_ocd_start_timer() - start a timer for otg state machine
 * Set or reset timer to interrupt in number of uS (micro-seconds). 
 *
 * @param otg
 * @param usec
 */
int l26_ocd_start_timer(struct otg_instance *otg, int usec)
{
	otg_event(otg, TMOUT, otg->ocd->TAG, "l26 Timer");
        return 0;
}

/* l26_ocd_ticks - get current ticks
 */ 
u64 l26_ocd_ticks (void)
{       
        struct timeval tv;
        do_gettimeofday(&tv);
        return tv.tv_sec * 1000000 + tv.tv_usec;
}

/* l26_ocd_elapsed - return micro-seconds between two tick values
 */ 
u64 l26_ocd_elapsed(u64 *t1, u64 *t2)
{       
        return (((*t1 > *t2) ? (*t1 - *t2) : (*t2 - *t1)));
}


struct ocd_ops l26_ocd_ops = {
        #if defined(CONFIG_OTG_TR_AUTO)
        .capabilities = OCD_CAPABILITIES_TR | OCD_CAPABILITIES_AUTO,
        #else
        .capabilities = OCD_CAPABILITIES_TR,
        #endif
        .ocd_init_func = l26_ocd_init,
        .mod_init = l26_ocd_mod_init,
        .mod_exit = l26_ocd_mod_exit,
        .start_timer = l26_ocd_start_timer,
        .ticks = l26_ocd_ticks,
        .elapsed = l26_ocd_elapsed,
};      


/* ********************************************************************************************* */


