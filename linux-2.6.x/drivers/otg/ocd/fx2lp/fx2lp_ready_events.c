/** *********************************************************************************
 ** *********************************************************************************
 ** fx2lp_ready_events.c - Cypress FX2LP HW specific library
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
 * Nov 21, 2006       Motorola, Inc.   Redesign
 * March 14, 2006       Motorola, Inc.  Handle IN short packets correctly - Add a reset ack event
 * Jul 13, 2007       Motorola, Inc.   Changes for open source compliance.
 * Aug 24, 2007       Motorola, Inc.   Changes for open source compliance.
*/

/***********************************************************************************
 *** SYSTEM SPECIFIC HEADERS
 ***********************************************************************************/

#include <otg/pcd-include.h>
#include <fx2lp_hardware.h>
#include <fx2lp_ready_events.h>
#include <linux/string.h>

extern wait_queue_head_t  response_wait_queue;

/* table of event handlers */
static const FX2LP_RDY_EV_INFO_T fx2lp_event_info[] = 
{
    FX2LP_RDY_INIT(EP2_SHORT,  &fx2lp_handle_ep_short,  FX2LP_EP2),
    FX2LP_RDY_INIT(EP6_SHORT,  &fx2lp_handle_ep_short,  FX2LP_EP6),
    FX2LP_RDY_INIT(ACK_SHORT_2,&fx2lp_handle_short_ack, 0),
    FX2LP_RDY_INIT(ACK_SHORT_6,&fx2lp_handle_short_ack, 2),
    FX2LP_RDY_INIT(WRITE_EP0,  &fx2lp_handle_write_ep0, 0),
    FX2LP_RDY_INIT(WRITE_EP1,  &fx2lp_handle_ep1_write, 0),
    FX2LP_RDY_INIT(EP1_CFG,    &fx2lp_handle_ep_config, FX2LP_EP1),
    FX2LP_RDY_INIT(EP2_CFG,    &fx2lp_handle_ep_config, FX2LP_EP2),
    FX2LP_RDY_INIT(EP6_CFG,    &fx2lp_handle_ep_config, FX2LP_EP6),
    FX2LP_RDY_INIT(ENUMERATE,  &fx2lp_handle_send_cmd,  FX2LP_PLUS_IN_ENUMERATE_CMD),
    FX2LP_RDY_INIT(SETUP_CPLT, &fx2lp_handle_setup_cplt,        0),
    FX2LP_RDY_INIT(STALL_EP0,  &fx2lp_handle_set_stall_ep0, FX2LP_EP0),
    FX2LP_RDY_INIT(STALL_EP1,  &fx2lp_handle_set_stall, FX2LP_EP1),
    FX2LP_RDY_INIT(STALL_EP2,  &fx2lp_handle_set_stall, FX2LP_EP2),
    FX2LP_RDY_INIT(STALL_EP6,  &fx2lp_handle_set_stall, FX2LP_EP6),
    FX2LP_RDY_INIT(SET_PTHRU,  &fx2lp_handle_send_cmd,  FX2LP_PLUS_IN_SETUPPASSTHROUGH_CMD),
    FX2LP_RDY_INIT(IFCONFIG,   &fx2lp_handle_ifconfig,  0),
    FX2LP_RDY_INIT(INTR_EN,    &fx2lp_handle_intr_en,   0),
    FX2LP_RDY_INIT(INTR_EN1,   &fx2lp_handle_intr_en,   1),
    FX2LP_RDY_INIT(BUS_ACTIVE, &fx2lp_handle_usb_activity_cmd, 1),
    FX2LP_RDY_INIT(BUS_INACTIVE, &fx2lp_handle_usb_activity_cmd, 0),
    FX2LP_RDY_INIT(READ_REG,   &fx2lp_handle_read_register, 0),
    FX2LP_RDY_INIT(STANDBY,    &fx2lp_handle_ifconfig,  FX2LP_STANDBY),
    FX2LP_RDY_INIT(USBRESET,   &fx2lp_handle_usb_reset,  0),
    FX2LP_RDY_INIT(USBRESET_ACK,   &fx2lp_handle_usb_reset_ack,  0),

    {0,0,0, NULL}
};

/* Maps from event number to index in fx2lp_event_info */
static const FX2LP_RDY_EV_INFO_T *fx2lp_event_index[FX2LP_RDY_NUM_EVENTS];

static void fx2lp_run_one_event(PFX2LP_PCD, const FX2LP_RDY_EV_INFO_T *);

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_EVENTS)

static char *evnames(FX2LP_RDY_EV_MASK_T mask)
{
    const FX2LP_RDY_EV_INFO_T *eip;
    static char evnamebuf[256];

    evnamebuf[0] = '\0';
    TRACE_MSG2(PCD,"evnamebuf = %s: mask = 0x%x",evnamebuf,mask);
    for (eip = fx2lp_event_info; eip->handler != NULL; ++eip)
    {
        if ((FX2LP_RDY_EV_N2M(eip->eventno) & mask) != 0)
        {
            strcat (evnamebuf, eip->evname);
            strcat (evnamebuf, " ");
        }
    }
    return (evnamebuf);
};
#endif

/*==========================================================================

FUNCTION:  fx2lp_run_one_event

DESCRIPTION:
    Internal routine to run a single event

ARGUMENTS PASSED:
    pfx2lp
    eip         Pointer to event information

RETURN VALUE:
    None

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    This assumes that the caller verifies that pfx2lp->is_cmd_rdy==TRUE.

==========================================================================*/
static void 
fx2lp_run_one_event(PFX2LP_PCD pfx2lp, const FX2LP_RDY_EV_INFO_T *eip)
{
    FX2LP_RDY_EV_MASK_T evmask = FX2LP_RDY_EV_N2M(eip->eventno);
    int ret;

    /* Don't start another command till this finishes */
    pfx2lp->is_cmd_rdy = FALSE;

   TRACE_MSG4(PCD,"\n%s(0x%x): %s %d", __FUNCTION__, pfx2lp,
        eip->evname, eip->arg);

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_EVENTS)
    printk ("\n%s(0x%x): %s %d", __FUNCTION__, pfx2lp,
        eip->evname, eip->arg);
#endif
    /* run handler, check if it wants to keep running */
    ret = (eip->handler)(pfx2lp, eip);

    if (ret > 0)
    {
        /* handler needs to run again after next "ready" intr */
        pfx2lp->curevent = eip;
    }
    else
    {
        if ((pfx2lp->wakeup_events & evmask) != 0)
        {
            /* someone wants a wakeup when this is done */
            pfx2lp->wakeup_events &= ~evmask;
            wake_up(&response_wait_queue);
        }

        if (ret < 0)
        {
            /* called function couldn't actually do anything */
            pfx2lp->is_cmd_rdy = TRUE;
        }
    }
}

/*==========================================================================

FUNCTION:  fx2lp_schedule_ready_event

DESCRIPTION:
    Task callable version of fx2lp_schedule_ready_event_irq()

ARGUMENTS PASSED:
    pfx2lp
    event

RETURN VALUE:
    None

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
void
fx2lp_schedule_ready_event(PFX2LP_PCD pfx2lp, FX2LP_RDY_EV_NUM_T event)
{
    unsigned long flags;

    local_irq_save (flags);

    fx2lp_schedule_ready_event_irq (pfx2lp, event);

    local_irq_restore (flags);
}

/*==========================================================================

FUNCTION:  fx2lp_schedule_ready_event_irq

DESCRIPTION:
    Schedule (or run) an FX2LP event

ARGUMENTS PASSED:
    pfx2lp
    event

RETURN VALUE:
    None

PRE-CONDITIONS:
    Called with interrupts disabled.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    If it's OK to do the command now, it will be done now.  Otherwise
    the event is added to the pending mask to be run on the next
    CMDRDY interrupt.

==========================================================================*/
void
fx2lp_schedule_ready_event_irq(PFX2LP_PCD pfx2lp, FX2LP_RDY_EV_NUM_T event)
{
    if ((event ==  FX2LP_RDY_EV_NO_EVENT) || (event == FX2LP_RDY_NUM_EVENTS))
    {

        TRACE_MSG1(PCD,"Invalid event: %d\n",event);
        return;
    }

    if (pfx2lp->is_cmd_rdy)
    {
        /* streamline case.  Since nothing is pending already, do this now */
        fx2lp_run_one_event (pfx2lp, fx2lp_event_index[event]);
    }
    else
    {
        pfx2lp->ready_events |= FX2LP_RDY_EV_N2M(event);

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_EVENTS)

        TRACE_MSG5(PCD,"\n%s(0x%x): %d 0x%x (%s)", __FUNCTION__, pfx2lp,
            pfx2lp->is_cmd_rdy,
            pfx2lp->ready_events,
            evnames(pfx2lp->ready_events));

        printk ("\n%s(0x%x): %d 0x%x (%s)", __FUNCTION__, pfx2lp,
            pfx2lp->is_cmd_rdy,
            pfx2lp->ready_events,
            evnames(pfx2lp->ready_events));
#endif
    }
}

/*==========================================================================

FUNCTION:  fx2lp_do_ready_evt_irq

DESCRIPTION:
    Run one pending event on CMDRDY interrupt

ARGUMENTS PASSED:
    pfx2lp

RETURN VALUE:
    None

PRE-CONDITIONS:
    pfx2lp->is_cmd_rdy is TRUE.

POST-CONDITIONS:
    Either a FX2LP operation that will cause a CMDRDY interrupt was
    done, or pfx2lp->is_cmd_rdy is TRUE.

IMPORTANT NOTES:
    Called only from interrupt handler.

==========================================================================*/
void
fx2lp_do_ready_evt_irq(PFX2LP_PCD pfx2lp)
{
    const FX2LP_RDY_EV_INFO_T *eip;
    FX2LP_RDY_EV_NUM_T evnum;

    if ((eip = pfx2lp->curevent) != NULL)
    {
        /* A previous handler still has work to do */
        pfx2lp->curevent = NULL;
        fx2lp_run_one_event (pfx2lp, eip);
        if (pfx2lp->is_cmd_rdy == FALSE)
        {
            return;
        }
    }

    do
    {
        FX2LP_RDY_EV_MASK_T mask = pfx2lp->ready_events;

        if (mask == 0)
        {
            /* nothing to run */
            return;
        }

        /* find one pending event */
        evnum = FX2LP_RDY_EV_M2N(pfx2lp->ready_events);
        eip = fx2lp_event_index[evnum];

        if( eip == NULL)
        printk("EIP IS NULL: evnum = %d\n",evnum);

        if ((pfx2lp->ready_events & FX2LP_RDY_EV_N2M(evnum)) == 0)
        {
            printk("\n\n fx2lp_do_ready_evt_irq Oops! %x and %d don't match",
                pfx2lp->ready_events, evnum);
            pfx2lp->ready_events = 0;
            return;
        }

        pfx2lp->ready_events &= ~FX2LP_RDY_EV_N2M(evnum);
        TRACE_MSG4(PCD,"\n%s(0x%x): %s %d", __FUNCTION__, pfx2lp,
            eip->evname, eip->arg);

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_EVENTS)
        printk ("\n%s(0x%x): %s %d", __FUNCTION__, pfx2lp,
            eip->evname, eip->arg);
#endif
        fx2lp_run_one_event (pfx2lp, eip);

    } while (pfx2lp->is_cmd_rdy);
}

/*==========================================================================

FUNCTION:  fx2lp_wait_event_completion

DESCRIPTION:
    Called from process to wait for an event to finish

ARGUMENTS PASSED:
    pfx2lp
    evnum       Event number

RETURN VALUE:
    None

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    Event is not pending.

IMPORTANT NOTES:
    None.

==========================================================================*/
void 
fx2lp_wait_event_completion(
    struct _FX2LP_PCD *pfx2lp, 
    FX2LP_RDY_EV_NUM_T evnum)
{
    unsigned long flags;
    FX2LP_RDY_EV_MASK_T evmask = FX2LP_RDY_EV_N2M(evnum);

    local_irq_save (flags);

    if ((pfx2lp->ready_events & evmask) != 0)
    {
        pfx2lp->wakeup_events |= evmask;
        local_irq_restore (flags);

        wait_event(response_wait_queue, 
            ((pfx2lp->ready_events & evmask) == 0));
    }
    else
    {
        local_irq_restore (flags);
    }
}

/*==========================================================================

FUNCTION:  fx2lp_ready_event_init

DESCRIPTION:
    One-time initialization of ready event tables.

ARGUMENTS PASSED:
    None

RETURN VALUE:
    None

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
void
fx2lp_ready_event_init(void)
{
    const FX2LP_RDY_EV_INFO_T *eip;

    for (eip = fx2lp_event_info; eip->handler != NULL; ++eip)
    {
        if ((eip->eventno ==  FX2LP_RDY_EV_NO_EVENT) || (eip->eventno == FX2LP_RDY_NUM_EVENTS))
        {

          TRACE_MSG1(PCD,"Invalid event: %d\n",eip->eventno);
          return;
        }
        fx2lp_event_index[eip->eventno] = eip;
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_EVENTS)
        printk ("\nEvent %2d: %s", eip->eventno, eip->evname);
#endif
    }
}

