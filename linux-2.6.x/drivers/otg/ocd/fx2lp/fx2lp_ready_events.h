/** *********************************************************************************
 ** *********************************************************************************
 ** fx2lp_ready_events.h - FX2LP specific macros, declarations
 **
 ** Cypress FX2LP HiSpeed USB Peripheral Controller driver
 **
 ** Portability:
 **     This file contains FX2LP H/W specific code. Not a portable to other platforms
 **
 ** *********************************************************************************
 ** *********************************************************************************/

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
 * Nov 21, 2006       Motorola, Inc.   Initial Creation
 * March 14, 2006     Motorola, Inc.  Handle IN short packets correctly - Add a reset ack event
 * Jul 13, 2007       Motorola, Inc.   Changes for open source compliance.
 * Aug 24, 2007       Motorola, Inc.   Changes for open source compliance.

*/

#ifndef __FX2LP_READY_EVENTS_H
#define __FX2LP_READY_EVENTS_H
/*
 * Information on how to process an FX2LP readypin event
 */
typedef unsigned int FX2LP_RDY_EV_MASK_T;
struct _FX2LP_PCD;

/*
 * Ready events
 *
 * Note: These are defined in order of priority.  If several events
 * are pending, they'll be done in the order defined here.
 */
typedef enum
{
    FX2LP_RDY_EV_N_EP2_SHORT,   /* commit EP2 short packet */
    FX2LP_RDY_EV_N_EP6_SHORT,   /* commit EP6 short packet */
    FX2LP_RDY_EV_N_ACK_SHORT_2, /* ack short packet notification, ep2 */
    FX2LP_RDY_EV_N_ACK_SHORT_6, /* ack short packet notification, ep6 */
    FX2LP_RDY_EV_N_WRITE_EP0,   /* write next EP0 block */
    FX2LP_RDY_EV_N_WRITE_EP1,   /* write EP1 packet */
    FX2LP_RDY_EV_N_EP1_CFG,     /* config EP1 */
    FX2LP_RDY_EV_N_EP2_CFG,     /* config EP2 */
    FX2LP_RDY_EV_N_EP6_CFG,     /* config EP6 */
    FX2LP_RDY_EV_N_SETUP_CPLT,  /* send setup complete */

    FX2LP_RDY_EV_N_STALL_EP0,   /* stall EP0 */
    FX2LP_RDY_EV_N_STALL_EP1,   /* stall EP1 */
    FX2LP_RDY_EV_N_STALL_EP2,   /* stall EP2 */
    FX2LP_RDY_EV_N_STALL_EP6,   /* stall EP6 */

    FX2LP_RDY_EV_N_INTR_EN,     /* set interrupt enable */
    FX2LP_RDY_EV_N_INTR_EN1,    /* set interrupt enable1 */
    FX2LP_RDY_EV_N_IFCONFIG,    /* write FX2LP_REG_IFCONFIG */
    FX2LP_RDY_EV_N_ENUMERATE,   /* tell chip to enumerate */
    FX2LP_RDY_EV_N_SET_PTHRU,   /* tell chip to give us setup packets */
    FX2LP_RDY_EV_N_BUS_ACTIVE,  /* send wakeup bus activity */
    FX2LP_RDY_EV_N_BUS_INACTIVE, /* send suspend bus activity */

    FX2LP_RDY_EV_N_READ_REG,    /* read register */
    FX2LP_RDY_EV_N_STANDBY,     /* go into standby mode */
    FX2LP_RDY_EV_N_USBRESET,   /* usb reset */
    FX2LP_RDY_EV_N_USBRESET_ACK,   /* usb reset ack */

    //FX2LP_RDY_EV_N_STOP2,     /* stop EP2 */
    //FX2LP_RDY_EV_N_STOP6      /* stop EP6 */
    FX2LP_RDY_NUM_EVENTS,       /* ALWAYS LAST - Number of events defined */
    FX2LP_RDY_EV_NO_EVENT = -1  /* Indication of not an event */
} FX2LP_RDY_EV_NUM_T;

typedef struct FX2LP_RDY_EV_INFO_S FX2LP_RDY_EV_INFO_T;

typedef int (FX2LP_RDY_EV_HANDLER_T) (struct _FX2LP_PCD *,
                        const FX2LP_RDY_EV_INFO_T *);

/* structure defines how each event will be handled */
struct FX2LP_RDY_EV_INFO_S
{
    FX2LP_RDY_EV_NUM_T     eventno;     /* event number */
    FX2LP_RDY_EV_HANDLER_T *handler;    /* routine to handle event */
    u32                    arg;         /* event-specific arg */
    char                   *evname;     /* debug */
};

#define FX2LP_RDY_INIT(E,H,A)   \
    {FX2LP_RDY_EV_N_##E, H, A, #E}


#define fx2lp_ready_event_pending(P,M) (((P)->ready_events & (M)) != 0)

#define FX2LP_RDY_EV_M2N(M) (ffs(M)-1)
#define FX2LP_RDY_EV_N2M(N) ((FX2LP_RDY_EV_MASK_T)1 << (N))

extern void fx2lp_ready_event_init(void);

extern void fx2lp_schedule_ready_event(struct _FX2LP_PCD *, FX2LP_RDY_EV_NUM_T);
extern void fx2lp_schedule_ready_event_irq(struct _FX2LP_PCD *, FX2LP_RDY_EV_NUM_T);
extern void fx2lp_wait_event_completion(struct _FX2LP_PCD *, FX2LP_RDY_EV_NUM_T);
extern void fx2lp_do_ready_evt_irq(struct _FX2LP_PCD *);


/* routines that handle events */
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_ep_short;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_setup_cplt;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_write_ep0;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_commit_short;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_ep1_write;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_ep_config;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_send_cmd;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_set_stall;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_set_stall_ep0;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_ifconfig;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_intr_en;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_usb_activity_cmd;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_read_register;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_short_ack;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_usb_reset;
extern FX2LP_RDY_EV_HANDLER_T fx2lp_handle_usb_reset_ack;
#endif /* __FX2LP_READY_EVENTS_H */
