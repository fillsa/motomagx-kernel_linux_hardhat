/** *********************************************************************************
 ** *********************************************************************************
 ** fx2lp.c - Cypress FX2LP HW specific library
 **
 ** Cypress FX2LP HiSpeed USB Peripheral Controller driver
 **
 ** Portability:
 **     This file contains FX2LP H/W specific code. Not portable to other platforms
 **
 ** *********************************************************************************
 ** *********************************************************************************/

/************************************************************************************
 *** COMPILATION FLAGS
 *** -----------------_
 *** NOT_USED     - This code is commented for experimental purpose and is **NOT**
 ***                supposed to be defined in the makefile.
 ***
 ***********************************************************************************/

/*
 *  Copyright (c) 2006-2008, Motorola, All Rights Reserved.
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
 * Oct 30, 2006       Motorola, Inc.   Bus Speed Initialization issues
 * Nov  9, 2006       Motorola, Inc.   Disconnect / Setup issues
 * Nov 21, 2006       Motorola, Inc.   Redesign
 * Mar 30, 2006       Motorola, Inc.   Fix for Firmware not giving a READREGRESP.
 * May 21, 2007       Motorola, Inc.   Changes for not to pulldown D+ line in suspend.
 * Aug  8, 2008       Motorola, Inc.   Commented out OTG debug from dmesg
*/

/***********************************************************************************
 *** Compile this file ONLY if FX2LP_HSUSB flag is defined in the makefile
 ***********************************************************************************/
//#ifdef FX2LP_HSUSB  

/***********************************************************************************
 *** SYSTEM SPECIFIC HEADERS
 ***********************************************************************************/

#include <otg/pcd-include.h>
#include <fx2lp_hardware.h>
#include <fx2lp_ready_events.h>
#include <asm/preempt.h>
#include <asm/timex.h>
#include <linux/delay.h>

/***********************************************************************************
 *** GLOBAL DECLARATIONS
 ***********************************************************************************/

FX2LP_PCD     fx2lp_pcd;
DECLARE_WAIT_QUEUE_HEAD(response_wait_queue);

/* map from endpoint number to index in status */
const FX2LP_EPX fx2lp_epn_to_epx[FX2LP_MAX_EP_NUMBER + 1] =
{
    FX2LP_EP0,
    FX2LP_EP1,
    FX2LP_EP2,
    FX2LP_INVALID,      /* 3 inused */
    FX2LP_INVALID,      /* 4 unused */
    FX2LP_INVALID,      /* 5 unused */
    FX2LP_EP6
};

const EPX_INFO_T fx2lp_epx_info[FX2LP_NUM_EPS + 1] =
{
    /* stall event, phys ep num, ep index, config reg, packet length reg, EPxPF reg */
    { FX2LP_RDY_EV_N_STALL_EP0, FX2LP_RDY_EV_NO_EVENT, 
        FX2LP_RDY_EV_NO_EVENT,
        0, FX2LP_EP0, UNUSED_REGISTER_NUMBER, UNUSED_REGISTER_NUMBER, UNUSED_REGISTER_NUMBER},
    { FX2LP_RDY_EV_N_STALL_EP1, FX2LP_RDY_EV_NO_EVENT, 
        FX2LP_RDY_EV_NO_EVENT,
        1, FX2LP_EP1, FX2LP_REG_EP1INCFG, UNUSED_REGISTER_NUMBER, UNUSED_REGISTER_NUMBER },
    { FX2LP_RDY_EV_N_STALL_EP2, FX2LP_RDY_EV_N_EP2_SHORT,
        FX2LP_RDY_EV_N_ACK_SHORT_2,
        2, FX2LP_EP2, FX2LP_REG_EP2CFG, FX2LP_REG_EP2PKTLENH, FX2LP_REG_EP2PFH},
    { FX2LP_RDY_EV_N_STALL_EP6, FX2LP_RDY_EV_N_EP6_SHORT,
        FX2LP_RDY_EV_N_ACK_SHORT_6,
        6, FX2LP_EP6, FX2LP_REG_EP6CFG, FX2LP_REG_EP6PKTLENH, FX2LP_REG_EP6PFH},
    { FX2LP_RDY_EV_NO_EVENT, FX2LP_RDY_EV_NO_EVENT, 
        FX2LP_RDY_EV_NO_EVENT,
        0xFF, FX2LP_INVALID, UNUSED_REGISTER_NUMBER, UNUSED_REGISTER_NUMBER, UNUSED_REGISTER_NUMBER}
};

int   busspeed_flag=0;

/***********************************************************************************
 *** EXTERNAL DECLARATIONS
 ***********************************************************************************/

extern FX2LP_BOOL fx2lp_stop_device(PFX2LP_PCD pfx2lp);
extern void inline fx2lp_normal_pcd_stop_out (PFX2LP_PCD pfx2lp);
static FX2LP_BOOL fx2lp_stall_ep0(PFX2LP_PCD pfx2lp);
extern void fx2lp_bus_suspend_bh(PFX2LP_PCD  pfx2lp);
extern void fx2lp_bus_resume_bh(PFX2LP_PCD  pfx2lp);

/* **********************************************************************************
 * GENERAL APIs
 * **********************************************************************************/


/*==========================================================================

FUNCTION:  init_fx2lp_epn

DESCRIPTION:
    Initialize endpoint status when we enmerate.

ARGUMENTS PASSED:
    pfx2lp

RETURN VALUE:
    None

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    is_used is cleared for all

IMPORTANT NOTES:
    None.

==========================================================================*/
void
init_fx2lp_epn(PFX2LP_PCD pfx2lp)
{
    u8    i;
    for (i=0; i<FX2LP_NUM_EPS; i++)
        pfx2lp->pep_status[i].is_used = FALSE;
}


/*==========================================================================

FUNCTION:  fx2lp_get_priority_based_epx

DESCRIPTION:
    Assign an endpoint for the given attributes.

ARGUMENTS PASSED:
    pfx2lp
    bm_attributes

RETURN VALUE:
    FX2LP_EPX   Indicator of which endpoint to use

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
FX2LP_EPX 
fx2lp_get_priority_based_epx(PFX2LP_PCD pfx2lp, u8 bm_attributes)
{
    FX2LP_EPX    epx = FX2LP_INVALID;
    PEP_STATUS   pep_status = pfx2lp->pep_status;
    
    switch (bm_attributes)
    {
        case USB_DIR_IN | USB_ENDPOINT_INTERRUPT:
        case USB_DIR_IN | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT:
            // 1 and only 1 is used for interrupt endpoints
            if (!pep_status[FX2LP_EP1].is_used) 
                epx = FX2LP_EP1;
            else
                printk("\n error : no interrupt IN endpoint is available...\n");
            break;

            
        case USB_DIR_IN | USB_ENDPOINT_BULK:
            // the priority is - 6,2:
            if (!pep_status[FX2LP_EP6].is_used) 
                epx = FX2LP_EP6;
            else if (!pep_status[FX2LP_EP2].is_used) 
                epx = FX2LP_EP2;
            else
                printk("\n error : no bulk IN endpoint is available...\n");
            break;

        case USB_DIR_OUT | USB_ENDPOINT_INTERRUPT:
        case USB_DIR_OUT | USB_ENDPOINT_INTERRUPT | USB_ENDPOINT_OPT:            
            // No need for OUT interrupts
                printk("\n error : no interrupt OUT endpoint is available...\n");
            break;

            
        case USB_DIR_OUT | USB_ENDPOINT_BULK:
            // the priority is - 2,6

            if (!pep_status[FX2LP_EP2].is_used) 
                epx = FX2LP_EP2;
            else if (!pep_status[FX2LP_EP6].is_used) 
                epx = FX2LP_EP6;
            else
                printk("\n error : no bulk OUT endpoint is available...\n");
	default :
	     break;
    }
    if (epx != FX2LP_INVALID)
    {
        pep_status[epx].is_used = TRUE;
        pep_status[epx].attributes = bm_attributes;
    }

    return epx;
}



//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_get_ep_status
// 
// DESCRIPTION:      Retrieve the endpoint status structure
// 
// ARGUMENTS PASSED:
//                   pfx2lp - fx2lp instance
//                   epx    - endpoint index
// 
// RETURN VALUE:
//                   endpoint instance if foound, NULL otherwise
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
PEP_STATUS
fx2lp_get_ep_status(PFX2LP_PCD pfx2lp, FX2LP_EPX epx)
{
    return (epx < FX2LP_NUM_EPS ? &pfx2lp->pep_status[epx] : NULL);
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_get_ep_config_reg
// 
// DESCRIPTION:      Retrieve the endpoint configuratin register
// 
// ARGUMENTS PASSED: epn    - endpoint number 
// 
// RETURN VALUE:     EP config register if found, 
//                   otherwise unsed register num
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
u8 
fx2lp_get_ep_config_reg(FX2LP_EPX epx)
{
    TRACE_MSG0(PCD,"--- Enter ---");
    return ((epx < FX2LP_NUM_EPS) ? 
            fx2lp_epx_info[epx].config_reg : 
            UNUSED_REGISTER_NUMBER);
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_get_ep_pktlen_reg
// 
// DESCRIPTION:      Retrieve the endpoint EPxPKTLENH register
// 
// ARGUMENTS PASSED: epn    - endpoint number 
// 
// RETURN VALUE:     EP pkt length register if found, 
//                   otherwise unsed register num
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
u8 
fx2lp_get_ep_pktlen_reg  (FX2LP_EPX epx)
{
    return ((epx < FX2LP_NUM_EPS) ? 
            fx2lp_epx_info[epx].pktlen_reg : 
            UNUSED_REGISTER_NUMBER);
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_get_ep_pf_reg
//  
// DESCRIPTION:      Retrieve the endpoint EPxPF register.
// 
// ARGUMENTS PASSED: epn    - endpoint number 
// 
// RETURN VALUE:     EP PF register if found, 
//                   otherwise unsed register num
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
u8 
fx2lp_get_ep_pf_reg (FX2LP_EPX epx)
{
    return ((epx < FX2LP_NUM_EPS) ? 
            fx2lp_epx_info[epx].ep_pf_reg : 
            UNUSED_REGISTER_NUMBER);
}

/* **********************************************************************************
 * ENDPOINT SPECIFIC APIs
 * **********************************************************************************/


//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_stall_ep0
// 
// DESCRIPTION:      stall the endpoint ZERO
// 
// ARGUMENTS PASSED: pfx2lp     - fx2lp instance
// 
// RETURN VALUE:     PASS, if everything goes well
//                   FAIL, if internal write operation fails or otherwise
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
static FX2LP_BOOL
fx2lp_stall_ep0(PFX2LP_PCD pfx2lp)
{
    TRACE_MSG0(PCD,"Enter fx2lp_stall_ep0\n"); 

    fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_STALL_EP0);
    return PASS;
}



//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_read_reg
//  
// DESCRIPTION:      Read a value from one of FX2LP's internal registers
//
// ARGUMENTS PASSED: pfx2lp  - fx2lp instance
//                   reg_addr- register to read from
//                   pdata   - to store the read value
//
// RETURN VALUE:     PASS, if write is successful
//                   FAIL, if ready pin is not high, or if no response
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//                   Ref 3.7.7.3 SX+ Emulation SW manual
//       This really should only be called from process context.
//       Even bottom half is potentially problematic, since this sleeps
//       waiting for an interrupt.  The sleep time should be short,
//       but still this should only be done in bh in unusual circumstances.
// 
//-----------------------------------------------------------------------------------
FX2LP_BOOL 
fx2lp_read_reg(PFX2LP_PCD  pfx2lp, u8 reg_addr, pu8 pdata)
{
    if (down_interruptible (&pfx2lp->readreg_sem)) /* mediate among tasks */
    {
        return (FAIL);
    }

    /* It's our turn. Record which register we need. */
    pfx2lp->register_to_read = reg_addr;
    pfx2lp->is_read_response_received = FALSE;

    /* Start the process to read the register */

    fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_READ_REG);

    /* wait for the register value */
    wait_event(response_wait_queue,
                        (pfx2lp->is_read_response_received != FALSE));

    /* return value to caller */
            *pdata = (u8) (pfx2lp->wout_read_reg_resp_cmd  & 0xFF);

    up (&pfx2lp->readreg_sem);         /* allow someone else to run */

    return PASS;
}

/* **********************************************************************************
 * EP0 SETUP / ENUMERATION APIs
 * **********************************************************************************/

FX2LP_BOOL
fx2lp_send_setup_complete_cmd(PFX2LP_PCD pfx2lp)
{
    fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_SETUP_CPLT);
    return PASS;
}


/*==========================================================================

FUNCTION:  fx2lp_read_setup_pkt_irq

DESCRIPTION:
    Gets and handles a setup packet from FX2LP

ARGUMENTS PASSED:
    pfx2lp

RETURN VALUE:
    None

PRE-CONDITIONS:
    Interrupt context

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    This still has the kludge to set bus speed the first time we see a
    setup packet.

==========================================================================*/
void
fx2lp_read_setup_pkt_irq(
    PFX2LP_PCD pfx2lp)
{
    struct pcd_instance *pcd    = pfx2lp->pcd;
    struct usbd_device_request request;
    volatile u16        *pfifo_addr;
    u16                 temp;
    pfifo_addr = pfx2lp->pcmd_out_addr;
    temp = FX2LP_WORD_SWAP(*pfifo_addr);

    /* check if this is really what we're looking for */
    if ((temp & FX2LP_PLUS_SETUP_PKT_CMD) == 0)
    {
        return;
    }

    /* hardly worth setting up a loop */
    ((u16 *)&request)[0] = FX2LP_WORD_SWAP(*pfifo_addr);
    ((u16 *)&request)[1] = FX2LP_WORD_SWAP(*pfifo_addr);
    ((u16 *)&request)[2] = FX2LP_WORD_SWAP(*pfifo_addr);
    ((u16 *)&request)[3] = FX2LP_WORD_SWAP(*pfifo_addr);

#if (FX2LP_DETAIL_KPF & (FX2LP_DETAIL_DATAFLOW|FX2LP_DETAI_INIT))
    printk ("\n  Setup: RT %02x, R %02x, V %04x, I %04X, L %04X",
            request.bmRequestType,
            request.bRequest,
            request.wValue,
            request.wIndex,
            request.wLength);
#endif

    if (pcd_recv_setup_irq(pcd, &request)) 
    { 
    //printk("\n^^^^^^^^^^^^^^%s:%d\n",__FUNCTION__,__LINE__);
        TRACE_MSG0(PCD, "pcd_ep0: STALLING");
        fx2lp_stall_ep0(pfx2lp);
    }
    else if ((request.wLength == 0)
     && (request.bmRequestType & USB_REQ_DIRECTION_MASK) == 
                        USB_REQ_HOST2DEVICE)
    {
        /* we need to ACK the setup packet */
        TRACE_MSG0(PCD,"Scheduling setup complete");
        pfx2lp->pep_status[FX2LP_EP0].send_zlp = TRUE;  
        fx2lp_schedule_ready_event (pfx2lp, FX2LP_RDY_EV_N_WRITE_EP0);
        fx2lp_schedule_ready_event (pfx2lp, FX2LP_RDY_EV_N_SETUP_CPLT);
    }
}


/* **************************************************************************
 * TX/RX APIs
 * **************************************************************************/


/* **************************************************************************
 * INITIALIZATION APIs
 * **************************************************************************/

//-----------------------------------------------------------------------------------
// 
// FUNCTION:        fx2lp_pcd_init
// 
// DESCRIPTION:     Initialize the FX2LP-PCD instance
// 
// ARGUMENTS PASSED:pcd - pcd instance
// 
// RETURN VALUE:    PASS, if everything goes well
//                  FAIL, otherwise
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
FX2LP_BOOL
fx2lp_pcd_init (void)
{    
    PFX2LP_PCD     pfx2lp = &fx2lp_pcd;
    u32            ep_num;
    PEP_STATUS     peps;
    u8             zz;
    FX2LP_EPX      epx;
    unsigned long flags;

    TRACE_MSG0(PCD,"Enter FX2LP-PCD_Init\n");
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("\n GRRRRRR.... Entered (%s) (0x%x, 0x%x)\n",__FUNCTION__, pfx2lp, pcd_instance);
#endif
    if (pfx2lp->is_pcd_mod_init == TRUE)
    { 
      pfx2lp->is_pcd_mod_init = FALSE;
      TRACE_MSG0(PCD,"Initialize ready event handler\n");
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
      printk("\n Initialize ready event handler in : %s:%p\n",__FUNCTION__,__LINE__);
#endif
      fx2lp_ready_event_init();
    }
    
    pfx2lp->is_cmd_rdy = TRUE;

    printk("Entered %s:%p\n",__FUNCTION__,__LINE__);
    if (pcd_instance != NULL)
    {
        TRACE_MSG0(PCD,"fx2lp_pcd_init... crosslink pfx2lp and pcd");
        pfx2lp->pcd = pcd_instance;
        pcd_instance->privdata = (void *)pfx2lp;
    }



    // save pcd with fx2lp
    pfx2lp->device_state           = FX2LP_RESUMED;
    pfx2lp->is_read_response_received  = FALSE;
    pfx2lp->pep_status[FX2LP_EP0].outinfo.outavail = 0;
    pfx2lp->register_to_read = 0;

       // printk("In function: %s:%p",__FUNCTION__,__LINE__);

#ifdef SOME_RECE_CONDITION
    pfx2lp->pintr_addr            = ioremap_nocache(FX2LP_INT_ADDR, 16);
    pfx2lp->pcmd_in_addr          = ioremap_nocache(FX2LP_CMDIN_ADDR, 16);
    pfx2lp->pcmd_out_addr         = ioremap_nocache(FX2LP_CMDOUT_ADDR, 16);
    if(!pfx2lp->pintr_addr || !pfx2lp->pcmd_in_addr || !pfx2lp->pcmd_out_addr)
        return FAIL;
#endif
    pfx2lp->is_fx2lp_ready        = FALSE;
   //printk("In function: %s:%p",__FUNCTION__,__LINE__);

    //initialize the out short pkt counters

    // Initialize peps items
    for(epx=0; epx < FX2LP_NUM_EPS; epx ++)
    { 
        peps = &pfx2lp->pep_status[epx];

        peps->ep_num          = fx2lp_epx_info[epx].phys_epn;
        peps->is_initialized  = FALSE;
        peps->cfg_reg_val     = 0;
        peps->pfifo_addr      = NULL;
        peps->epx             = epx;
        peps->doconfig        = 0;
        peps->send_zlp = FALSE;
        
    }

    //printk("In function: %s:%p",__FUNCTION__,__LINE__);
    if(!(pfx2lp->pep_status[FX2LP_EP2].pfifo_addr = 
                    ioremap_nocache(FX2LP_FIFO2_ADDR, 16)))
        return FAIL;
    if(!(pfx2lp->pep_status[FX2LP_EP6].pfifo_addr = 
                    ioremap_nocache(FX2LP_FIFO6_ADDR, 16)))
        return FAIL;

    pfx2lp->ready_events = 0;
    pfx2lp->wakeup_events = 0;
    pfx2lp->curevent = NULL;
    pfx2lp->eintr_en = 0;
    pfx2lp->ifconfig = 0;
    PREPARE_WORK_ITEM(pfx2lp->bus_suspend_bh, fx2lp_bus_suspend_bh, pfx2lp);
    PREPARE_WORK_ITEM(pfx2lp->bus_resume_bh, fx2lp_bus_resume_bh, pfx2lp);
    init_MUTEX(&pfx2lp->readreg_sem);
    pfx2lp->ep0_partial = FALSE;
    pfx2lp->is_initialized = TRUE;
#if 0
    j = 1;
    }
#endif
    printk("Leaving Init: In function: %s:%p\n",__FUNCTION__,__LINE__);
    TRACE_MSG0(PCD,"Leaving FX2LP-PCD_Init\n");
//    local_irq_restore(flags);
    return PASS;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:        fx2lp_setup_dev
// 
// DESCRIPTION:     initializes the fx2lp chip with h/w specific info
// 
// ARGUMENTS PASSED: pfx2lp - fx2lp instance
// 
// RETURN VALUE:     PASS, if everything goes well
//                   FAIL, otherwise
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
//-----------------------------------------------------------------------------------
FX2LP_BOOL fx2lp_setup_dev (PFX2LP_PCD pfx2lp)
{
    volatile u32   temp;
    u8              reg_val;
    u32             ii = 0;
    FX2LP_BOOL            is_timeout = FALSE;

    TRACE_MSG0(PCD,"enter fx2lp_init\n");
    if(!pfx2lp)
    {
        printk("\n (%s:%d) >>>>>>>>>>>> pfx2lp is NULL . ",__FUNCTION__,__LINE__);
        return FAIL;
    }
    //Enable BUS activity wakeup. This is temp placed here. Need to move to proper place ater checking for RDY INT.
    /* wait for ready, then enable bus activity */
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("\n (%s:%d) >>>>>>>>>>>> BUS_ACTIVE . ",__FUNCTION__,__LINE__);
#endif
    fx2lp_schedule_ready_event (pfx2lp, FX2LP_RDY_EV_N_BUS_ACTIVE);

    return PASS;

}

/* **************************************************************************
 * REQ-RESPONSE CMD APIs
 * **************************************************************************/


/* **************************************************************************
 * MISC APIs
 * **************************************************************************/
 
//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_suspend_pcd
// 
// DESCRIPTION:      Suspend the FX2LP chip for power saving
// 
// ARGUMENTS PASSED: pfx2lp   - fx2lp instance
// 
// RETURN VALUE:     Pass or Fail.
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

FX2LP_BOOL 
fx2lp_suspend_pcd (PFX2LP_PCD  pfx2lp)
{
    u8 reg_val;
    printk("\n +++++++++++++++++++++++++++++++++ I am in ---- (%s:%d)  ",__FUNCTION__,__LINE__);
// Firmware does not give a read register response that causes a stickiness where the phone re-enumerates
// in same mode even after a mode switch. So, remove the read and write the same value. 
    //if(!fx2lp_read_reg(pfx2lp, FX2LP_REG_IFCONFIG, &pfx2lp->ifconfig))
    //    return FAIL;
    
    // This value is for suspending the chip but do not pull down the D+ line. We should pull down D+ 
    // line only if we want to disconnect.
    pfx2lp->ifconfig = 0xcc;

// Suspend the fx2lp to save power
    fx2lp_schedule_ready_event (pfx2lp, FX2LP_RDY_EV_N_STANDBY);
    return PASS;
}


//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_bus_suspend_bh
// 
// DESCRIPTION:      Handle the bus suspend interrupt in the BH
// 
// ARGUMENTS PASSED: pfx2lp   - fx2lp instance
// 
// RETURN VALUE:     None.
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
fx2lp_bus_suspend_bh(PFX2LP_PCD  pfx2lp)
{
    struct pcd_instance *pcd = PFX2LP_PCD(pfx2lp); 

    printk("\n +++++++++++++++++++++++++++++++++ SUSPENDING ---- (%s:%d)  ",__FUNCTION__,__LINE__);
    if(!pcd)
    {
        printk("\npfx2lp->pcd is not initialised in (%s) this is wrong.. So not proceeding... ");
        return;
    }

	if(pfx2lp->device_state != FX2LP_SUSPENDED)
	{
        pfx2lp->device_state = FX2LP_SUSPENDED;
// put the chip in suspend mode
        fx2lp_suspend_pcd(pfx2lp);
        usbd_bus_event_handler_irq (pcd->bus, DEVICE_BUS_INACTIVE, 0);
	}
	else
	{
		printk("\n +++++++++++++++++++++++++++++++++ Already Suspended ---- (%s:%d)  ",__FUNCTION__,__LINE__);
	}
		
}



//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_bus_resume_bh
// 
// DESCRIPTION:      Handle the bus resume interrupt
// 
// ARGUMENTS PASSED: pfx2lp   - fx2lp instance
// 
// RETURN VALUE:     None.
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
fx2lp_bus_resume_bh(PFX2LP_PCD  pfx2lp)
{
    struct pcd_instance *pcd = PFX2LP_PCD(pfx2lp); 

    printk("\n +++++++++++++++++++++++++++++++++ RESUMING ---- (%s:%d)  ",__FUNCTION__,__LINE__);
	if(pfx2lp->device_state != FX2LP_RESUMED)
	{
        pfx2lp->device_state = FX2LP_RESUMED;
        usbd_bus_event_handler_irq (pcd->bus, DEVICE_BUS_ACTIVITY, 0);
    }
	else
	{
		printk("\n +++++++++++++++++++++++++++++++++ Already Active ---- (%s:%d)  ",__FUNCTION__,__LINE__);
	}
}



//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_disconnect_device
// 
// DESCRIPTION:      disconnect to the FX2LP device
// 
// ARGUMENTS PASSED: pfx2lp   - fx2lp instance
// 
// RETURN VALUE:     None.
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:  call this as part of the cleanup (TBD)
//    None.
// 
//-----------------------------------------------------------------------------------

FX2LP_BOOL
fx2lp_disconnect_device (PFX2LP_PCD  pfx2lp)
{

    TRACE_MSG0(PCD,"fx2lp_disconnect_device\n");
    // Disable BUS activity so that the noise doesnt try to wake the part up.
    // Send 0 with the cmd to disable
    fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_BUS_INACTIVE);
    fx2lp_wait_event_completion(pfx2lp, FX2LP_RDY_EV_N_BUS_INACTIVE);

    // pull down D+
//       if(!fx2lp_read_reg(pfx2lp, FX2LP_REG_IFCONFIG, &pfx2lp->ifconfig))
 //      {
  //       return FAIL;
   //    }
     msleep(100);

     pfx2lp->ifconfig = 0xcd;

    fx2lp_schedule_ready_event (pfx2lp, FX2LP_RDY_EV_N_IFCONFIG);

    msleep(100);

    if(!fx2lp_stop_device(pfx2lp))
        return FAIL;
    
    return PASS;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_enable_interrupts
// 
// DESCRIPTION:      enable FX2LP specifc interrupts
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

BOOL
fx2lp_enable_interrupts (PFX2LP_PCD pfx2lp)
{
    static int first = 1;
    pfx2lp->eintr_en |= INT_M_HS_NOTIFY;
    TRACE_MSG1(PCD,"Enter - pfx2lp->eintr_en = %x",pfx2lp->eintr_en);
    
    // Enable the interrupts
    if(first)
    {
      fx2lp_schedule_ready_event (pfx2lp, FX2LP_RDY_EV_N_INTR_EN1);
      fx2lp_schedule_ready_event (pfx2lp, FX2LP_RDY_EV_N_INTR_EN);
      first = 0;
    }

    return PASS;
}

/***********************************************************************************
 *** Compile this file ONLY if FX2LP_HSUSB is defined.
 ***********************************************************************************/

//#endif /* FX2LP_HSUSB */





