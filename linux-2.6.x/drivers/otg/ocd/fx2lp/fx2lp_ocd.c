/** *********************************************************************************
 ** *********************************************************************************
 ** fx2lp_ocd.c - Cypress FX2LP HW specific library
 **
 ** It does generic configuration and handles the all FX2LP interrupts.
 **
 ** Portability:
 **     This file contains FX2LP H/W specific code. 
 **     This also has some MXC specific code.
 **
 ** *********************************************************************************
 ** *********************************************************************************/

/************************************************************************************
 *** COMPILATION FLAGS
 *** -----------------_
 ***
 *** NOT_USED     - This code is commented for experimental purpose and is **NOT**
 ***                supposed to be defined in the makefile.
 ***
 ***********************************************************************************/
/*
 *  Copyright (c) 2006-2007, Motorola, All Rights Reserved.
 *
 *  This program is licensed under a BSD license with the following terms:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 *  Neither the name of Motorola nor the names of its contributors may be
 *  used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * Mar  8, 2006       Motorola, Inc.   Initial create
 * Jul 26, 2007       Motorola, Inc.   Changes for open src compliance.
 * Aug 24, 2007       Motorola, Inc.   Changes for open src compliance.
*/

/***********************************************************************************
 *** SYSTEM SPECIFIC HEADERS
 ***********************************************************************************/

#include <otg/pcd-include.h>
#include <fx2lp_hardware.h>
#include <otghw/mxc-hardware.h>
#include <asm/arch/gpio.h>
#include <fx2lp_ready_events.h>

/* **********************************************************************************
 * GLOBAL VARIABLES
 * **********************************************************************************/
int             mxc_transceiver_mode;
struct ocd_ops  ocd_ops;

/* **********************************************************************************
 * OCD SPECIFIC MACROS
 * **********************************************************************************/
#define TIMEOUT_VALUE 1000

/* **********************************************************************************
 * EXTERN FUNCTION DECLARATIONS
 * **********************************************************************************/
extern void         fx2lp_ocd_init         (struct otg_instance *otg, u8 flag);
extern int          fx2lp_ocd_mod_init     (void);
extern void         fx2lp_ocd_mod_exit     (void);
extern irqreturn_t  fx2lp_handle_pcd_isr   (int irq, void *dev_id, struct pt_regs *regs);
extern FX2LP_BOOL       fx2lp_wait_for_response(FX2LP_BOOL *flag);
extern FX2LP_BOOL       fx2lp_suspend_pcd (PFX2LP_PCD  pfx2lp);



#if defined(CONFIG_OTG_GPTR)
    extern int      mxc_gptcr_start_timer  (struct otg_instance *otg, int usec);
    extern u64      mxc_gptcr_trace_ticks  (void);
    extern u64      mxc_gptcr_trace_elapsed(u64 *t1, u64 *t2);
#endif /* defined(CONFIG_OTG_GPTR) */

#if defined(CONFIG_OTG_HRT)
    extern int      mxc_hrt_start_timer    (struct otg_instance *otg, int usec);
    extern u64      mxc_hrt_trace_ticks    (void);
    extern u64      mxc_hrt_trace_elapsed  (u64 *t1, u64 *t2);
#endif /* defined(CONFIG_OTG_HRT) */


/* **********************************************************************************
 * EXTERN VARIABLES
 * **********************************************************************************/
extern FX2LP_PCD fx2lp_pcd;

/* **********************************************************************************
 * GENERAL APIs
 * **********************************************************************************/

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         mxc_set_transceiver_mode
// 
// DESCRIPTION:      
// 
// ARGUMENTS PASSED:
//                   
//                   
// 
// RETURN VALUE:
//                   
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:  brassboard/tcd needs this function
//    None.
// 
//-----------------------------------------------------------------------------------

void 
mxc_set_transceiver_mode(int mode)
{   
    printk ("\n I am in (%s)", __FUNCTION__);
    mxc_transceiver_mode = mode;
    fs_andl(OTG_CORE_HWMODE, ~0xF0);
    fs_orl(OTG_CORE_HWMODE, (mode << 6) | (mode << 4));  // set to software hnp
    TRACE_MSG1(OCD, "hwmode: %08x", fs_rl(OTG_CORE_HWMODE));
    return;
} 

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         pcd_func_int_hndlr_isr
// 
// DESCRIPTION:      main pcd function interrupt handler
// 
// ARGUMENTS PASSED:
//                   
//                   
// 
// RETURN VALUE:
//                   
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------

irqreturn_t 
pcd_func_int_hndlr_isr (int irq, void *dev_id, struct pt_regs *regs)
{
    //pcd_instance->otg->interrupts++;
    return fx2lp_handle_pcd_isr(irq, dev_id, regs);
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         mxc_ocd_global_init
// 
// DESCRIPTION:      
// 
// ARGUMENTS PASSED:
//                   
//                   
// 
// RETURN VALUE:
//                   
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------

#if !defined(OTG_C99)
struct ocd_ops ocd_ops;
void fx2lp_ocd_global_init(void)
{
        ZERO(ocd_ops);
        #if defined(CONFIG_OTG_TR_AUTO)
        ocd_ops.capabilities  = OCD_CAPABILITIES_TR | OCD_CAPABILITIES_AUTO;
        #else 
        ocd_ops.capabilities  = OCD_CAPABILITIES_TR; 
        #endif
        ocd_ops.ocd_init_func = fx2lp_ocd_init;
        ocd_ops.mod_init      = fx2lp_ocd_mod_init;
        ocd_ops.mod_exit      = fx2lp_ocd_mod_exit;
#if defined(CONFIG_OTG_GPTR)
        ocd_ops.start_timer   = mxc_gptcr_start_timer;
        ocd_ops.ticks         = mxc_gptcr_trace_ticks;
        ocd_ops.elapsed       = mxc_gptcr_trace_elapsed;
#endif /* defined(CONFIG_OTG_GPTR) */
#if defined(CONFIG_OTG_HRT)
        ocd_ops.start_timer   = mxc_hrt_start_timer;
        ocd_ops.ticks         = mxc_hrt_trace_ticks;
        ocd_ops.elapsed       = mxc_hrt_trace_elapsed;
#endif /* defined(CONFIG_OTG_HRT) */
}
#else /* !defined(OTG_C99) */

struct ocd_ops ocd_ops = {
        #if defined(CONFIG_OTG_TR_AUTO)
        .capabilities  = OCD_CAPABILITIES_TR | OCD_CAPABILITIES_AUTO,
        #else 
        .capabilities  = OCD_CAPABILITIES_TR, 
        #endif
        .ocd_init_func = fx2lp_ocd_init,
        .mod_init      = fx2lp_ocd_mod_init,
        .mod_exit      = fx2lp_ocd_mod_exit,
#if defined(CONFIG_OTG_GPTR)
        .start_timer   = mxc_gptcr_start_timer,
        .ticks         = mxc_gptcr_trace_ticks,
        .elapsed       = mxc_gptcr_trace_elapsed,
#endif /* defined(CONFIG_OTG_GPTR) */
#if defined(CONFIG_OTG_HRT)
        .start_timer   = mxc_hrt_start_timer,
        .ticks         = mxc_hrt_trace_ticks,
        .elapsed       = mxc_hrt_trace_elapsed,
#endif /* defined(CONFIG_OTG_HRT) */
};
#endif /* !defined(OTG_C99) */
//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_ocd_init
// 
// DESCRIPTION:      used to initialize/enable or disable the tcd driver
// 
// ARGUMENTS PASSED:
//                   
//                   
// 
// RETURN VALUE:
//                   
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
void 
fx2lp_ocd_init(struct otg_instance *otg, u8 flag)
{

    switch (flag) {
    case SET:
        TRACE_MSG0(OCD, "FS_OCD_EN SET");
        otg_event(otg, TCD_OK | ID_FLOAT, OCD, "OCD_OK");
        break;
    case RESET:
        TRACE_MSG0(OCD, "FS_OCD_EN RESET");
        otg_event(otg, TCD_OK | ID_FLOAT, OCD, "OCD_OK");
        break;
    default :
        break;
    }

    return ;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_ocd_mod_exit
// 
// DESCRIPTION:      de-initialize
// 
// ARGUMENTS PASSED:
//                   
//                   
// 
// RETURN VALUE:
//                   
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------

void 
fx2lp_ocd_mod_exit (void)
{
    PFX2LP_PCD pfx2lp = &fx2lp_pcd;
    
    printk("\n I am in (%s).. ", __FUNCTION__);
    return;
}



//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_ocd_mod_init
// 
// DESCRIPTION:      Allocate interrupts and setup hardware
// 
// ARGUMENTS PASSED:
//                   
//                   
// 
// RETURN VALUE:
//                   
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
//-----------------------------------------------------------------------------------


int 
fx2lp_ocd_mod_init (void)
{
    u32 *scm_controoler;
    PFX2LP_PCD pfx2lp = &fx2lp_pcd;

    printk ("\n I am in (%s)", __FUNCTION__);
    
    pfx2lp->pintr_addr            = ioremap_nocache(FX2LP_INT_ADDR, 16);
    pfx2lp->pcmd_in_addr          = ioremap_nocache(FX2LP_CMDIN_ADDR, 16);
    pfx2lp->pcmd_out_addr         = ioremap_nocache(FX2LP_CMDOUT_ADDR, 16);
    if(!pfx2lp->pintr_addr || !pfx2lp->pcmd_in_addr || !pfx2lp->pcmd_out_addr)
        return FAIL;

    // connect high speed USB to Atlas.
    gpio_usb_hs_switch_set_data (GPIO_HIGH);   //*****IMB TBD *****


    scm_controoler = ioremap_nocache (0xB8002050, 0x80);
    RETURN_EINVAL_UNLESS(scm_controoler);


    return 0;
}

