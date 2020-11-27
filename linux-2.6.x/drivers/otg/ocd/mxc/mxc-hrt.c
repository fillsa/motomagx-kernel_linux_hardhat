/*
 * otg/ocd/mxc/mxc-gptcr.c -- Freescale GPT timer 
 * @(#) balden@seth2.belcarratech.com|otg/ocd/mxc/mxc-hrt.c|20051116203403|56573
 *
 *      Copyright (c) 2005 Belcarra
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
 * 06/12/2006         Motorola         OTG name change
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
 */
/*!
 * @file otg/ocd/mxc/mxc-hrt.c
 * @brief Freecale GPT Timer implementation.
 *
 * The GPT2 timer is used for the OTG timer.
 *
 * The GPT3 timer is used as a free running counter for a source of ticks
 * for use with the trace facility.
 *
 * @ingroup FSOTG
 *
 */

#include <otg/pcd-include.h>

//#if defined (CONFIG_OTG_HRT)

#include <linux/pci.h>
#include <asm/arch/gpio.h>
#include <asm/arch/board.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/preempt.h>
#include <linux/timer.h>

/* Added for HRT support */
#include <linux/hrtime.h>
#include <asm/arch/hrtime.h>

#include <otghw/mxc-hardware.h>



/* ********************************************************************************************* */

struct timer_list hr_timer;




#if 0
static u64 mxc_hrt_divisor = 0;
static u64  mxc_hrt_multiplier = 0;
static int dev_id = 1;
static bool mxc_shared_int = 0;
#endif




/*!
 * mxc_hrt_ticks() - compute ticks from usecs
 * @param usecs
 * @return ticks
 */
u64 inline mxc_hrt_ticks(u32 usecs)
{
	return 0 ;
}

/*!
 * mxc_hrt_usecs() - compute usecs from ticks
 * @param usecs
 * @return usecs
 */
u64 inline mxc_hrt_usecs(u32 usecs)
{
	return 0;
}

/*!
 * mxc_hrt_trace_ticks() - get current ticks
 * 
 */
u64 mxc_hrt_trace_ticks (void)
{
	//ticks from jiffis is not enough we should use the arch cycle
	return (u64) get_arch_cycles(1);
}       

/*!
 * mxc_hrt_trace_elapsed() - return micro-seconds between two tick values
 *
 */
u64 mxc_hrt_trace_elapsed(u64 *t1, u64 *t2)
{
        u64 ticks = (*t1 > *t2) ? (*t1 - *t2) : (*t2 - *t1);
        return (arch_cycle_to_nsec (ticks)/1000);
}

/* ********************************************************************************************* */
//u32 mxc_hrt_timeout;
//u32 mxc_hrt_ticks_start;

u32 mxc_hr_usec_set;
u32 mxc_hr_jiffy_per_sec;

BOOL mxc_hr_active;


void mxc_hrt_callback (unsigned long arg)
{

        TRACE_MSG1(OCD, "checking active: %d", mxc_hr_active);
        RETURN_UNLESS(mxc_hr_active);
        mxc_hr_active = FALSE;
        TRACE_MSG1(OCD, "resetting active: %d", mxc_hr_active);

	if (mxc_hr_usec_set >= 1000000) {	//if requested period is in the range of 1 sec
	       	hr_timer.expires = jiffies + ((mxc_hr_usec_set/1000000)*mxc_hr_jiffy_per_sec);
		hr_timer.arch_cycle_expires = get_arch_cycles(jiffies);
	        hr_timer.function = mxc_hrt_callback;
	}
	else {
        	hr_timer.expires = jiffies;
	        hr_timer.arch_cycle_expires = get_arch_cycles(jiffies);
        	if (mxc_hr_usec_set < 100) {
	                TRACE_MSG1(OCD, "usec: %d set to minimum 100", mxc_hr_usec_set);
                	mxc_hr_usec_set = 100;
        	}
	        hr_timer.arch_cycle_expires += nsec_to_arch_cycle(mxc_hr_usec_set * 1000);
	        //hr_timer.arch_cycle_expires += nsec_to_arch_cycle(100 * 1000 * 1000);
        	while (hr_timer.arch_cycle_expires >= arch_cycles_per_jiffy)
	        {
        	        hr_timer.expires++;
                	hr_timer.arch_cycle_expires -= arch_cycles_per_jiffy;
	        }
	}	//end of else	

	TRACE_MSG3 (OCD, "usec: %d expires: %8u arch_cycle_expires: %8u", 
                        mxc_hr_usec_set, hr_timer.expires, hr_timer.arch_cycle_expires);
	otg_event(ocd_instance->otg, TMOUT, OCD, "TMOUT");


//        add_timer(&hr_timer);
}



/*!
 * mxc_hrt_start_timer() - start a timer for otg state machine
 * Set or reset timer to interrupt in number of uS (micro-seconds). 
 *
 * XXX There may be a floor or minimum that can be effectively set.
 * XXX We have seen an occasional problem with US(25) for discharge for example.
 *
 * @param otg
 * @param usec
 */
int mxc_hrt_start_timer(struct otg_instance *otg, int usec)
{

        TRACE_MSG1(OCD, "usec: %d", usec);

	mxc_hr_usec_set = usec;
	//TRACE_MSG1 (OCD, "usec: %d", usec);
        
        mxc_hr_active = FALSE;
        TRACE_MSG1(OCD, "resetting active: %d", mxc_hr_active);

	del_timer(&hr_timer);
        RETURN_ZERO_UNLESS(usec);

        mxc_hr_active = TRUE;
        TRACE_MSG1(OCD, "setting active: %d", mxc_hr_active);

	if (mxc_hr_usec_set >= 1000000) {
                hr_timer.expires = jiffies + ((mxc_hr_usec_set/1000000)*mxc_hr_jiffy_per_sec);
                hr_timer.arch_cycle_expires = get_arch_cycles(jiffies);
                TRACE_MSG4 (OCD, "usec: %u jiffies: %8u expires: %8u arch_cycle_expires: %8u LONG", 
                                usec, jiffies, hr_timer.expires, hr_timer.arch_cycle_expires);
	}
	else {
	        hr_timer.expires = jiffies;
        	hr_timer.arch_cycle_expires = get_arch_cycles(jiffies);

        	if (mxc_hr_usec_set < 100) {
	                TRACE_MSG1(OCD, "usec: %d set to minimum 100", mxc_hr_usec_set);
                	mxc_hr_usec_set = 100;
        	}
	        hr_timer.arch_cycle_expires += nsec_to_arch_cycle(mxc_hr_usec_set * 1000);

                TRACE_MSG2(OCD, "arch_cycle_expires: %d arch_cycles_per_jiffy: %d", 
                                hr_timer.arch_cycle_expires, arch_cycles_per_jiffy);

        	while (hr_timer.arch_cycle_expires >= arch_cycles_per_jiffy) {
        	        hr_timer.expires++;
                	hr_timer.arch_cycle_expires -= arch_cycles_per_jiffy;
	        }
                TRACE_MSG4 (OCD, "usec: %u jiffies: %8u expires: %8u arch_cycle_expires: %8u SHORT", 
                                usec, jiffies, hr_timer.expires, hr_timer.arch_cycle_expires);
	}
	

	add_timer(&hr_timer);
        return 0;
}

#if 0
/*!
 * mxc_hrt_timer_int_hndlr() - timer interrupt
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t mxc_hrt_timer_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{

	return 0;
}
#endif

/* ********************************************************************************************* */
extern void fs_ocd_init(struct otg_instance *otg, u8 flag);



/*!
 * mxc_hrt_ocd_mod_init() - initial tcd setup
 * Allocate interrupts and setup hardware.
 */
int mxc_hrt_mod_init (void)
{
int res = 0;

#if 0	//test timer for 10 sec
        init_timer (&hr_timer);
        hr_timer.expires = jiffies + 630;
        hr_timer.function = mxc_hrt_callback;
	add_timer(&hr_timer);				
#endif
	
#if 1
	init_timer (&hr_timer);
        hr_timer.expires = jiffies + 10;
        hr_timer.function = mxc_hrt_callback;
	hr_timer.arch_cycle_expires = get_arch_cycles(jiffies);
	hr_timer.arch_cycle_expires += nsec_to_arch_cycle(100 * 1000 * 1000);
        while (hr_timer.arch_cycle_expires >= arch_cycles_per_jiffy)
        {
                hr_timer.expires++;
                hr_timer.arch_cycle_expires -= arch_cycles_per_jiffy;
        }
//	add_timer(&hr_timer);
#endif

	res = nsec_to_arch_cycle(100000);
	//printk(KERN_INFO"arch cycles for 100usec:  %8X\n", res);
	res = arch_cycles_per_jiffy;
	mxc_hr_jiffy_per_sec = (nsec_to_arch_cycle(1000000000)/arch_cycles_per_jiffy);

	TRACE_MSG4(OCD, "arch cycles per jiffy: %8u Number of jiffy for 1 sec is %8u resolution: %8d nsec/cycle: %8u\n", 
                        arch_cycles_per_jiffy, 
                        mxc_hr_jiffy_per_sec, 
                        hr_time_resolution,
                        nsec_to_arch_cycle(1)
                        );
	
        CATCH(error) {
                return -EINVAL;
        }
        return 0;
}

/*!
 * mxc_hrt_mod_exit() - de-initialize
 */
void mxc_hrt_mod_exit (void)
{

	del_timer (&hr_timer);

}


//#endif	//CONFIG_OTG_HRT



