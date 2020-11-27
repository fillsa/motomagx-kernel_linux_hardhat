/*
 * otg/ocd/mxc/mxc-gptcr.c -- Freescale GPT timer 
 * @(#) balden@seth2.belcarratech.com|otg/ocd/mxc/mxc-gptcr.c|20051116203403|37224
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
 *
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
 * @file otg/ocd/mxc/mxc-gptcr.c
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

#if defined (CONFIG_OTG_GPTR)

#include <linux/pci.h>
#include <asm/arch/gpio.h>
#include <asm/arch/board.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/preempt.h>


#include <otghw/mxc-hardware.h>




/* ********************************************************************************************* */


static u64 mxc_gptcr_divisor = 0;
static u64  mxc_gptcr_multiplier = 0;
static int dev_id = 1;
static bool mxc_shared_int = 0;

/*!
 * mxc_gptcr_ticks() - compute ticks from usecs
 * @param usecs
 * @return ticks
 */
u64 inline mxc_gptcr_ticks(u32 usecs)
{
        return (((u64)usecs) * ((u64)mxc_gptcr_divisor)) / ((u64)mxc_gptcr_multiplier); 
}

/*!
 * mxc_gptcr_usecs() - compute usecs from ticks
 * @param usecs
 * @return usecs
 */
u64 inline mxc_gptcr_usecs(u32 usecs)
{
        return (((u64)usecs) * ((u64)mxc_gptcr_multiplier)) / ((u64)mxc_gptcr_divisor); 
}

/*!
 * mxc_gptcr_trace_ticks() - get current ticks
 * GPT3 is setup as free running clock at 266 Mhz - 1/266 = .0037
 */
u64 mxc_gptcr_trace_ticks (void)
{       
        return (u64) *_reg_GPT_GPTCNT;
}       

/*!
 * mxc_gptcr_trace_elapsed() - return micro-seconds between two tick values
 *
 * GPT3 is setup as free running clock at 266 Mhz - 1/266 = .003759
 *
 * MPLL = 266, FCLK = 266, BCLK = 88
 *
 */
u64 mxc_gptcr_trace_elapsed(u64 *t1, u64 *t2)
{
        u64 ticks = (*t1 > *t2) ? (*t1 - *t2) : (*t2 - *t1);
//        ticks = (ticks * mxc_gptcr_multiplier) / mxc_gptcr_divisor; // .3776
	if (TICKS_PER_USEC != 0)
		ticks /= TICKS_PER_USEC;
	else
		ticks = 0;
        return ticks;
}

/* ********************************************************************************************* */
//u32 mxc_gptcr_timeout;
//u32 mxc_gptcr_ticks_start;

u32 mxc_gptcr_usec_set;
u32 mxc_gptcr_ticks_set;
u32 mxc_gptcr_match_set;
u32 mxc_gptcr_active;

/*!
 * mxc_gptcr_start_timer() - start a timer for otg state machine
 * Set or reset timer to interrupt in number of uS (micro-seconds). 
 *
 * XXX There may be a floor or minimum that can be effectively set.
 * XXX We have seen an occasional problem with US(25) for discharge for example.
 *
 * @param otg
 * @param usec
 */
int mxc_gptcr_start_timer(struct otg_instance *otg, int usec)
{
        u32 ticks;
        u32 match;
        unsigned long flags;
        local_irq_save (flags);

        //TRACE_MSG2(OCD, "usec: %d CNT: %08x", usec, *_reg_GPT_GPTCNT);

        if (usec && (usec < 100)) {
                usec = 100;
                TRACE_MSG2(OCD, "usec: %d CNT: %08x", usec, *_reg_GPT_GPTCNT);
        }

        /*
         * Disable Channel 3 compare.
         */
        *_reg_GPT_GPTCR &= ~(0x7 << 26);

        mxc_gptcr_usec_set = usec;
        if (usec) {

                mxc_gptcr_ticks_set = ticks = (u32 ) mxc_gptcr_ticks(usec);
                mxc_gptcr_match_set = 0;
                mxc_gptcr_active = 0;


                /*
                 * Compute and set match register
                 */
                mxc_gptcr_match_set = match = *_reg_GPT_GPTCNT + ticks;
                *_reg_GPT_GPTOCR3 = match;
                mxc_gptcr_active = 1;

                TRACE_MSG6(OCD, "cnt: %08x match: %08x GPTCNT: %08x GPTCR: %08x GPTSR: %08x GPTOCR3: %08x\n", 
                                mxc_gptcr_ticks_set, mxc_gptcr_match_set, 
                                *_reg_GPT_GPTCNT, *_reg_GPT_GPTCR, *_reg_GPT_GPTSR, *_reg_GPT_GPTOCR3);
                /* 
                 * Enable interrupt 
                 */
                *_reg_GPT_GPTIR |= (0x01 << 2);

        }

        local_irq_restore (flags);
        return 0;
}

/*!
 * mxc_gptcr_timer_int_hndlr() - timer interrupt
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t mxc_gptcr_timer_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{               
        u32 gptsr = *_reg_GPT_GPTSR;


	*_reg_GPT_GPTIR &= ~(0x04);

        if (gptsr & (0x01 << 2)){
        	TRACE_MSG7(OCD, "cnt: %08x match: %08x gptsr: %08x GPTCNT: %08x GPTCR: %08x GPTSR: %08x GPTOCR3: %08x\n",
		                       mxc_gptcr_ticks_set, mxc_gptcr_match_set, gptsr,
		                        *_reg_GPT_GPTCNT, *_reg_GPT_GPTCR, *_reg_GPT_GPTSR, *_reg_GPT_GPTOCR3);

		*_reg_GPT_GPTCR &= ~(0x7 << 26);
		*_reg_GPT_GPTOCR3 = 0;
                TRACE_MSG0(OCD, "OCM3");
                TRACE_MSG1(OCD, "active: %x", mxc_gptcr_active);
                if (mxc_gptcr_active) {
                        TRACE_MSG0(OCD, "calling otg_event"); 
                        mxc_gptcr_active = 0;
                        otg_event(ocd_instance->otg, TMOUT, OCD, "TMOUT");
                }
                else {
                        TRACE_MSG0(OCD, "skipping otg_event"); 
                }
		*_reg_GPT_GPTSR |= 0x4;
		return IRQ_HANDLED;
	}

	if (!mxc_shared_int)
		*_reg_GPT_GPTIR |= 0x4;
		
	return IRQ_NONE;
}


/* ********************************************************************************************* */
extern void fs_ocd_init(struct otg_instance *otg, u8 flag);



/*!
 * mxc_gptcr_ocd_mod_init() - initial tcd setup
 * Allocate interrupts and setup hardware.
 */
int mxc_gptcr_mod_init (int divisor, int multiplier)
{
        int timer;

	if (*_reg_GPT_GPTCR & 0x1)
		mxc_shared_int = 1;
	else
		mxc_shared_int = 0;
	
        if (mxc_shared_int){
		printk (KERN_INFO"Using shared interrupt for timer 3 \n");
		timer = request_irq (INT_GPT, mxc_gptcr_timer_int_hndlr, SA_INTERRUPT | SA_SHIRQ, 
                                UDC_NAME " OTG TIMER", (int *) &dev_id);
	}
	else{
		printk (KERN_INFO"OTG TIMER is the only ISR for timer 3 \n");
		timer = request_irq (INT_GPT, mxc_gptcr_timer_int_hndlr, SA_INTERRUPT | SA_SHIRQ, 
                                UDC_NAME " OTG TIMER", (int *) &dev_id);
	        /* disable and reset GPT
		*/
		*_reg_GPT_GPTCR &= ~0x00000001;
		*_reg_GPT_GPTCR |= (1<<15);
		while ((*_reg_GPT_GPTCR & (1<<15)) != 0) ;
		*_reg_GPT_GPTPR = 0;
		*_reg_GPT_GPTCR = (0x1 << 9) | (0x2 << 6);
		*_reg_GPT_GPTCR |= 0x1;      
	}

	TRACE_MSG0(OCD, "1. Interrupts requested");

        THROW_IF(timer, error);

        mxc_gptcr_divisor = divisor;
        mxc_gptcr_multiplier = multiplier;

	return 0;

        CATCH(error) {
                return -EINVAL;
        }
        return 0;
}

/*!
 * mxc_gptcr_mod_exit() - de-initialize
 */
void mxc_gptcr_mod_exit (void)
{
        mxc_gptcr_start_timer(NULL, 0);
        free_irq (INT_GPT, (int *) &dev_id);
}


#endif	//CONFIG_OTG_GPTR



