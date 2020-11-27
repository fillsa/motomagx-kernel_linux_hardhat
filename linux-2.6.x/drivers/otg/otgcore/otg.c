/*
 * otg/otgcore/otg.c - OTG state machine
 * @(#) balden@seth2.belcarratech.com|otg/otgcore/otg.c|20051116205002|53384
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@lbelcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005, 2006, 2008 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 08/02/2006         Motorola         KW error 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 * 08/08/2008         Motorola         Commented out OTG debug from dmesg
 * 09/29/2008         Motorola         OSS compliance changes.                
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
 * @file otg/otgcore/otg.c
 * @brief OTG Core State Machine Event Processing
 *
 * The OTG State Machine receives "input" information passed to it
 * from the other drivers (pcd, tcd, hcd and ocd) that describe what
 * is happening.
 *
 * The State Machine uses the inputs to move from state to state. Each
 * state defines four things:
 *
 * 1. Reset - what inputs to set or reset on entry to the state.
 *
 * 2. Outputs - what output functions to call to set or reset.
 *
 * 3. Timeout - an optional timeout value to set
 *
 * 4. Tests - a series of tests that allow the state machine to move to
 * new states based on current or new inputs.
 *
 * @ingroup OTGCore
 */
#ifndef OTG_REGRESS

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <otg/otg-pcd.h>
#include <otg/otg-ocd.h>


struct hcd_ops *otg_hcd_ops;
struct ocd_ops *otg_ocd_ops;
struct pcd_ops *otg_pcd_ops2;
struct pcd_ops *otg_pcd_ops;
struct tcd_ops *otg_tcd_ops;
struct usbd_ops *otg_usbd_ops;

extern struct otg_instance otg_instance_private;
extern struct hcd_instance hcd_instance_private;
extern struct ocd_instance ocd_instance_private;
extern struct pcd_instance pcd_instance_private;
extern struct tcd_instance tcd_instance_private;

#if 0
/* otg_vbus - return vbus status
 *
 * XXX Should we default to set or reset if there is no vbus function
 */
int otg_vbus(struct otg_instance *otg)
{
        TRACE_MSG2(CORE, "otg_tcd_ops: %p vbus: %p", otg_tcd_ops, otg_tcd_ops ? otg_tcd_ops->vbus : NULL);
        return (otg_tcd_ops && otg_tcd_ops->vbus) ? otg_tcd_ops->vbus(otg) : 1;
}
#endif

#if defined (OTG_WINCE)
#define TRACE_SM_CURRENT(t, m, o) \
				
#else
#define TRACE_SM_CURRENT(t, m, o) \
                TRACE_MSG5(t, "SM_CURRENT: RESET: %08x SET: %08x %s (%s) %s",\
                                (u32)(~o->current_inputs), \
                                (u32)(o->current_inputs), \
                                o->current_outputs->name, \
                                o->previous_outputs->name, \
                                m \
                          );
#endif //OTG_WINCE
#define TRACE_SM_CHANGE(t, m, o) \
                TRACE_MSG5(t, "SM_CHANGE:  RESET: %08x SET: %08x %s (%s) %s",\
                                (u32)(~o->current_inputs), \
                                (u32)(o->current_inputs), \
                                o->current_outputs->name, \
                                o->previous_outputs->name, \
                                m \
                          );

#define TRACE_SM_NEW(t, m, o) \
                TRACE_MSG5(t, "SM_NEW:     RESET: %08x SET: %08x %s (%s) %s",\
                                (u32)(~o->current_inputs), \
                                (u32)(o->current_inputs), \
                                o->current_outputs->name, \
                                o->previous_outputs->name, \
                                m \
                          );


#define TRACE_SM_INPUTS(t, m, u, c) \
                TRACE_MSG4(t, "SM_INPUTS:  RESET: %08x SET: %08x CUR: %08x %s",\
                                (u32)(u >> 32), \
                                (u32)(u & 0xffffffff), \
                                c, \
                                m \
                                )

#endif



DECLARE_MUTEX (otg_sem);

/*!
 * Output change lookup table - this maps the current output and the desired
 * output to what should be done. This is just so that we don't redo changes,
 * if the output does not change we do not want to re-call the output function
 */
u8 otg_output_lookup[4][4] = {
      /*  NC       SET      RESET    PULSE, <--- current/  new     */
        { NC,      NC,      NC,      PULSE,  },         /* NC      */ 
        { SET,     NC,      SET,     PULSE,  },         /* SET     */ 
        { RESET,   RESET,   NC,      PULSE,  },         /* RESET   */ 
        { PULSE,   PULSE,   PULSE,   PULSE,  },         /* UNKNOWN */ 
};


/*!
 * otg_new() - check OTG new inputs and select new state to be in
 *
 * This is used by the OTG event handler to see if the state should
 * change based on the current input values.
 *
 * @param otg pointer to the otg instance.
 * @return either the next OTG state or invalid_state if no change.
 */
static int otg_new(struct otg_instance *otg)
{
        int state = otg->state;
        struct otg_test *otg_test = otg_firmware_loaded ? otg_firmware_loaded->otg_tests : NULL;
        u64 input_mask = ((u64)otg->current_inputs) | ((u64)(~otg->current_inputs) << 32);   // get a copy of current inputs

        UNLESS(otg_test) return invalid_state;

        TRACE_MSG3(CORE, "OTG_NEW: %s inputs: %08x %08x", otg_get_state_name(state),
                        (u32)(input_mask>>32&0xffffffff), (u32)(input_mask&0xffffffff));

        /* iterate across the otg inputs table, for each entry that matches the current
         * state, test the input_mask to see if a state transition is required.
         */
        for (; otg_test->target != invalid_state; otg_test++) {

                u64 test1 = otg_test->test1;
                u64 test2 = otg_test->test2;
                u64 test3 = otg_test->test3;

                CONTINUE_UNLESS(otg_test->state == state);     // skip states that don't match


                TRACE_MSG7(CORE, "OTG_NEW: %s (%s, %d) test1: %08x %08x test2 %08x %08x",
                                otg_get_state_name(otg_test->state),
                                otg_get_state_name(otg_test->target),
                                otg_test->test,
                                (u32)(test1>>32&0xffffffff), (u32)(test1&0xffffffff),
                                (u32)(test2>>32&0xffffffff), (u32)(test2&0xffffffff)
                                );

                /* the otg inputs table has multiple masks that define between multiple tests. Each
                 * test is a simple OR to check if specific inputs are present.
                 */
                CONTINUE_UNLESS (
                                (!otg_test->test1 || ((test1 = otg_test->test1 & input_mask))) &&
                                (!otg_test->test2 || ((test2 = otg_test->test2 & input_mask))) &&
                                (!otg_test->test3 || ((test3 = otg_test->test3 & input_mask))) /*&&
                                (!otg_test->test4 || (otg_test->test4 & input_mask)) */ );

                TRACE_MSG7(CORE, "OTG_NEW: GOTO %s test1: %08x %08x test2 %08x %08x test3 %08x %08x",
                                otg_get_state_name(otg_test->target),
                                (u32)(test1>>32&0xffffffff), (u32)(test1&0xffffffff),
                                (u32)(test2>>32&0xffffffff), (u32)(test2&0xffffffff),
                                (u32)(test3>>32&0xffff), (u32)(test3&0xffff)
                                );

                return otg_test->target;
        }
        TRACE_MSG0(CORE, "OTG_NEW: finis");

        return invalid_state;
}

char mbuf[96];

/*!
 * otg_write_state_message_irq() -
 *
 * Send a message to the otg management application.
 *
 * @param msg
 * @param reset
 * @param inputs
 */
void otg_write_state_message_irq(char *msg, u64 reset, u32 inputs)
{
        sprintf(mbuf, "State: %s reset: %08x set: %08x inputs: %08x ", msg, (u32)(reset>>32), (u32)(reset&0xffffffff), inputs);
        mbuf[95] = '\0';
        otg_message(mbuf);
}

/*!
 * otg_write_output_message_irq() -
 *
 * Send a message to the otg management application.
 *
 * @param msg
 * @param val
 */
void otg_write_output_message_irq(char *msg, int val)
{
        strcpy(mbuf, "Output: ");
        strncat(mbuf, msg, 64 - strlen(mbuf));
        if (val == 2) strncat(mbuf, "/", 64 - strlen(mbuf));
        mbuf[95] = '\0';
        otg_message(mbuf);
}

/*!
 * otg_write_timer_message_irq() -
 *
 * Send a message to the otg management application.
 *
 * @param msg
 * @param val
 */
void otg_write_timer_message_irq(char *msg, int val)
{
        if (val < 10000)
                sprintf(mbuf, "Timer: %s %d uS %u", msg, val, otg_tmr_ticks());
        else if (val < 10000000)
                //sprintf(mbuf, " %d mS\n", ticks / 1000);
                //sprintf(mbuf, "Timer: %s %d mS %u", msg, val >> 10, otg_tmr_ticks());
                sprintf(mbuf, "Timer: %s %d mS %u", msg, val / 1000, otg_tmr_ticks());
        else
                //sprintf(mbuf, " %d S\n", ticks / 1000000);
                //sprintf(mbuf, "Timer: %s %d S %u", msg, val >> 20, otg_tmr_ticks());
                sprintf(mbuf, "Timer: %s %d S %u", msg, val / 1000000, otg_tmr_ticks());

        mbuf[95] = '\0';
        otg_message(mbuf);
}

/*!
 * otg_write_info_message_irq() -
 *
 * Send a message to the otg management application.
 *
 * @param msg
 */
void otg_write_info_message(char *msg)
{
        unsigned long flags;
        local_irq_save (flags);
        sprintf(mbuf, "Info: %s", msg);
        mbuf[95] = '\0';
        otg_message(mbuf);
        local_irq_restore (flags);
}

/*!
 * otg_write_reset_message_irq() -
 *
 * Send a message to the otg management application.
 *
 * @param msg
 * @param val
 */
void otg_write_reset_message(char *msg, int val)
{
        unsigned long flags;
        local_irq_save (flags);
        sprintf(mbuf, "State: %s reset: %x", msg, val);
        mbuf[95] = '\0';
        otg_message(mbuf);
        local_irq_restore (flags);
}

OTG_EXPORT_SYMBOL(otg_write_info_message);

char otg_change_names[4] = {
        '_', ' ', '/', '#',
};

/*!
 * otg_do_event_irq() - process OTG events and determine OTG outputs to reflect new state
 *
 * This function is passed a mask with changed input values. This
 * is passed to the otg_new() function to see if the state should
 * change. If it changes then the output functions required for
 * the new state are called.
 *
 * @param otg pointer to the otg instance.
 * @param inputs input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 * @return non-zero if state changed.
 */
static int otg_do_event_irq(struct otg_instance *otg, u64 inputs, otg_tag_t tag, char *msg)
{
        u32 current_inputs = otg->current_inputs;
        int current_state = otg->state;
        u32 inputs_set = (u32)(inputs & 0xffffffff);           // get changed inputs that SET something
        u32 inputs_reset = (u32) (inputs >> 32);                // get changed inputs that RESET something
        int target;

        RETURN_ZERO_UNLESS(otg && inputs);;
        TRACE_SM_INPUTS(tag, msg, inputs, current_inputs);

        /* Special Overrides - These are special tests that satisfy specific injunctions from the
         * OTG Specification.
         */
        if (inputs_set & inputs_reset) {                // verify that there is no overlap between SET and RESET masks
                TRACE_MSG1(CORE, "OTG_EVENT: ERROR attempting to set and reset the same input: %08x", inputs_set & inputs_reset);
                return 0;
        }
        /* don't allow bus_req and bus_drop to be set at the same time
         */
        if ((inputs_set & (bus_req | bus_drop)) == (bus_req | bus_drop) ) {
                TRACE_MSG2(CORE, "OTG_EVENT: ERROR attempting to set both bus_req and bus_drop: %08x %08x",
                                inputs_set, (bus_req | bus_drop));
                return 0;
        }
        /* set bus_drop_ if bus_req, set bus_req_ if bus_drop
         */
        if (inputs_set & bus_req) {
                inputs |= bus_drop_;
                TRACE_MSG1(CORE, "OTG_EVENT: forcing bus_drop/: %08x", inputs_set);
        }
        if (inputs_set & bus_drop) {
                inputs |= bus_req_;
                TRACE_MSG1(CORE, "OTG_EVENT: forcing bus_req/: %08x", inputs_set);
        }

        otg->current_inputs &= ~inputs_reset;
        otg->current_inputs |= inputs_set;
        //TRACE_MSG3(CORE, "OTG_EVENT: reset: %08x set: %08x inputs: %08x", inputs_reset, inputs_set, otg->current_inputs);

        TRACE_SM_CHANGE(tag, msg, otg);

        RETURN_ZERO_IF(otg->active);

        otg->active++;

        /* Search the state input change table to see if we need to change to a new state.
         * This may take several iterations.
         */
        while (invalid_state != (target = otg_new(otg))) {

                struct otg_state *otg_state;
                int original_state = otg->state;
                u64 current_outputs;
                u64 output_results;
                u64 new_outputs;
                int i;

                /* if previous output started timer, then cancel timer before proceeding
                 */
                if ((otg_state = otg->current_outputs) && otg_state->tmout && otg->start_timer) {
                        TRACE_MSG0(CORE, "reseting timer");
                        otg->start_timer(otg, 0);
                }

                BREAK_UNLESS(otg_firmware_loaded);

                BREAK_UNLESS(target < otg_firmware_loaded->number_of_states);

                otg_state = otg_firmware_loaded->otg_states + target;

                BREAK_UNLESS(otg_state->name);

                otg->previous = otg->state;
                otg->state = target;
                otg->tickcount = otg_tmr_ticks();


                otg->previous_outputs= otg->current_outputs;
                //otg->current_outputs = NULL;


                /* A matching input table rule has been found, we are transitioning to a new
                 * state.  We need to find the new state entry in the output table.
                 * XXX this could be a table lookup instead of linear search.
                 */

                current_outputs = otg->outputs;
                new_outputs = otg_state->outputs;
                output_results = 0;

                TRACE_SM_NEW(tag, msg, otg);

                /* reset any inputs that the new state want's reset
                 */
                otg_do_event_irq(otg, otg_state->reset | TMOUT_, tag, otg_state->name);

                otg_write_state_message_irq(otg_state->name, otg_state->reset, otg->current_inputs);

#if 1
                switch (target) {                            /* C.f. 6.6.1.12 b_conn reset */
                case otg_disabled:
                        otg->current_inputs = 0;
                        otg->outputs = 0;
                        break;
                default:
                        //otg_event_irq(otg, not(b_conn), "a_host/ & a_suspend/: set b_conn/");
                        break;
                }
#endif
                #if 0
                TRACE_MSG5(CORE, "OTG_EVENT: OUTPUT MAX:%d OUTPUTS CURRENT: %08x %08x NEW: %08x %08x",
                                MAX_OUTPUTS,
                                (u32)(current_outputs >> 32),
                                (u32)(current_outputs & 0xffffffff),
                                (u32)(new_outputs >> 32),
                                (u32)(new_outputs & 0xffffffff)
                          );
                #endif

                /* Iterate across the outputs bitmask, calling the appropriate functions
                 * to make required changes. The current outputs are saved and we DO NOT
                 * make calls to change output values until they change again.
                 */
                for (i = 0; i < MAX_OUTPUTS; i++) {

                        u8 current_output = (u8)(current_outputs & 0x3);
                        u8 new_output = (u8)(new_outputs & 0x3);
                        u8 changed = otg_output_lookup[new_output][current_output];

                        output_results >>= 2;

                        switch (changed) {
                        case NC:
                                output_results |= ((current_outputs & 0x3) << (MAX_OUTPUTS * 2));
                                break;
                        case SET:
                        case RESET:
                                output_results |= (((u64)changed) << (MAX_OUTPUTS * 2));
                        case PULSE:


                                otg_write_output_message_irq(otg_output_names[i], changed);

                                //TRACE_MSG3(CORE, "OTG_EVENT: CHECKING OUTPUT %s %d %s",
                                //                otg_state->name, i, otg_output_names[i]);

                                BREAK_UNLESS(otg->otg_output_ops[i]); // Check if we have an output routine

                                TRACE_MSG7(CORE, "OTG_EVENT: CALLING OUTPUT %s %d %s%c cur: %02x new: %02x chng: %02x",
                                                otg_state->name, i, otg_output_names[i],
                                                otg_change_names[changed&0x3],
                                                current_output, new_output,
                                                changed);

                                otg->otg_output_ops[i](otg, changed); // Output changed, call function to do it

                                //TRACE_MSG2(CORE, "OTG_EVENT: OUTPUT FINISED %s %s", otg_state->name, otg_output_names[i]);
                                break;
                        }

                        current_outputs >>= 2;
                        new_outputs >>= 2;
                }

                output_results >>= 2;
                otg->outputs = output_results;
                otg->current_outputs = otg_state;

                //TRACE_MSG2(CORE, "OTG_EVENT: STATE: otg->current_outputs: %08x %08x",
                //                (u32)(otg->outputs & 0xffffffff), (u32)(otg->outputs >> 32));

                if (((otg->outputs & pcd_en_out_set) == pcd_en_out_set) &&
                                        ((otg->outputs & hcd_en_out_set) == hcd_en_out_set))
                {
                        TRACE_MSG0(CORE, "WARNING PCD_EN and HCD_EN both set");
                        printk(KERN_INFO "%s: WARNING PCD_EN and HCD_EN both set\n", __FUNCTION__);
                }

                /* start timer?
                 */
                CONTINUE_UNLESS(otg_state->tmout);
                //TRACE_MSG1(CORE, "setting timer: %d", otg_state->tmout);
                if (otg->start_timer) {
                        otg_write_timer_message_irq(otg_state->name, otg_state->tmout);
                        otg->start_timer(otg, otg_state->tmout);
                }
                else {
                        //TRACE_MSG0(CORE, "NO TIMER");
                        otg_do_event_irq(otg, TMOUT_, CORE, otg_state->name);
                }

        }
        TRACE_MSG0(CORE, "finishing");
        otg->active = 0;

        if (current_state == otg->state)
                TRACE_MSG0(CORE, "OTG_EVENT: NOT CHANGED");

        otg->active = 0;
        return 0;
}

/*!
 * otg_write_input_message_irq() -

 * @param inputs input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 */
void otg_write_input_message_irq(u64 inputs, otg_tag_t tag, char *msg)
{
        u32 reset = (u32)(inputs >> 32);
        u32 set = (u32) (inputs & 0xffffffff);

        char buf[64];
        //snprintf(buf, sizeof(buf), "Input: %08x %08x %s", *inputs >> 32, *inputs & 0xffffffff, msg ? msg : "NULL");
        //TRACE_MSG3(tag, "Inputs: %08x %08x %s", set, reset, msg ? msg : "NULL");
        #ifdef OTG_WINCE
        sprintf(buf, "Inputs: %08x %08x %s", reset, set, msg ? msg : "NULL");
        #else /* OTG_WINCE */
        snprintf(buf, sizeof(buf), "Inputs: %08x %08x %s", reset, set, msg ? msg : "NULL");
        #endif /* OTG_WINCE */
        buf[63] = '\0';
        otg_message(buf);
}


/*!
 * otg_event_input_message_irq() -
 * @param otg pointer to the otg instance.
 * @param inputs input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 * @return non-zero if state changed.
 */
int otg_event_irq(struct otg_instance *otg, u64 inputs, otg_tag_t tag, char *msg)
{
        //TRACE_SM_INPUTS(tag, msg, inputs, otg->current_inputs);
        //otg_write_input_message_irq((u32)(inputs >> 32), (u32) (inputs & 0xffffffff), tag, msg);
        otg_write_input_message_irq(inputs, tag, msg);
        return otg_do_event_irq(otg, inputs, tag, msg);
}

/*!
 * otg_event() -
 * @param otg pointer to the otg instance.
 * @param inputs input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 * @return non-zero if state changed.
 */
int otg_event(struct otg_instance *otg, u64 inputs, otg_tag_t tag, char *msg)
{
        int rc; 
        unsigned long flags;
        RETURN_ZERO_UNLESS(otg);
        local_irq_save (flags);
        rc = otg_event_irq(otg, inputs, tag, msg);
        local_irq_restore (flags);
        return rc;
}


/*!
 * otg_event_set_irq() -
 * @param otg pointer to the otg instance.
 * @param changed
 * @param flag
 * @param input input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 * @return non-zero if state changed.
 */
int otg_event_set_irq(struct otg_instance *otg, int changed, int flag, u32 input, otg_tag_t tag, char *msg)
{
        RETURN_ZERO_UNLESS(changed);
        TRACE_MSG4(tag, "%s: %08x changed: %d flag: %d", msg, input, changed, flag);
        return otg_event(otg, flag ? input : _NOT(input), tag, msg);
}


/*!
 * otg_serial_number() - set the device serial number
 *
 * @param otg pointer to the otg instance.
 * @param serial_number_str
 */
void otg_serial_number (struct otg_instance *otg, char *serial_number_str)
{
        int i;
        char *cp = serial_number_str;

        DOWN(&otg_sem);

        for (i = 0; cp && *cp && (i < (OTGADMIN_MAXSTR-1)); cp++) {
                CONTINUE_UNLESS (isxdigit(*cp));
                otg->serial_number[i++] = toupper(*cp);
        }
        otg->serial_number[i] = '\0';

        TRACE_MSG2(CORE, "serial_number_str: %s serial_number: %s", serial_number_str, otg->serial_number);

        UP(&otg_sem);
}

/*!
 * otg_init() - create and initialize otg instance
 *
 * Called to initialize the OTG state machine. This will cause the state
 * to change from the invalid_state to otg_disabled.
 *
 * If the device has the OCD_CAPABILITIES_AUTO flag set then
 * an initial enable_otg event is generated. This will cause
 * the state to move from otg_disabled to otg_enabled. The
 * requird drivers will be initialized by calling the appropriate
 * output functions:
 *
 *      - pcd_init_func
 *      - hcd_init_func
 *      - tcd_init_func
 *
 * @param otg pointer to the otg instance.
 */
void otg_init (struct otg_instance *otg)
{
        DOWN(&otg_sem);
        otg->outputs = tcd_init_out_ | pcd_init_out_ | hcd_init_out_;

        // XXX MODULE LOCK HERE
        
        /* This will move the state machine into the otg_disabled state
         */
        otg_event(otg, enable_otg, CORE, "enable_otg"); // XXX

        if (otg_ocd_ops->capabilities & OCD_CAPABILITIES_AUTO)
                otg_event(otg, AUTO, CORE, "AUTO"); // XXX

        UP(&otg_sem);
}


/*!
 * otg_exit()
 *
 * This is called by the driver that started the state machine to
 * cause it to exit. The state will move to otg_disabled.
 *
 * The appropriate output functions will be called to disable the
 * peripheral drivers.
 *
 * @param otg pointer to the otg instance.
 */
void otg_exit (struct otg_instance *otg)
{
        //TRACE_MSG0(CORE, "DOWN OTG_SEM");
        DOWN(&otg_sem);

        //TRACE_MSG0(CORE, "OTG_EXIT");
        //otg_event(otg, exit_all | not(a_bus_req) | a_bus_drop | not(b_bus_req), "tcd_otg_exit");

        otg_event(otg, enable_otg_, CORE, "enable_otg_");

        while (otg->state != otg_disabled) {
                // TRACE_MSG pointless here, module will be gon before we can look at it.
                #ifdef OTG_WINCE
                //DEBUGMSG(KERN_ERR"%s: waiting for state machine to disable\n", __FUNCTION__);
                #else /* OTG_WINCE */
//                printk(KERN_ERR"%s: waiting for state machine to disable\n", __FUNCTION__);
                #endif /* OTG_WINCE */
                SCHEDULE_TIMEOUT(10 * HZ);
        }

        //TRACE_MSG0(CORE, "UP OTG_SEM");
        UP(&otg_sem);

        // XXX MODULE UNLOCK HERE
}


OTG_EXPORT_SYMBOL(otg_event);
OTG_EXPORT_SYMBOL(otg_event_irq);
OTG_EXPORT_SYMBOL(otg_serial_number);
OTG_EXPORT_SYMBOL(otg_init);
OTG_EXPORT_SYMBOL(otg_exit);
OTG_EXPORT_SYMBOL(otg_event_set_irq);

/* ********************************************************************************************* */

/*!
 * otg_gen_init_func() -
 *
 * This is a default tcd_init_func. It will be used
 * if no other is provided.
 *
 * @param otg pointer to the otg instance.
 * @param flag set or reset
 */
void otg_gen_init_func (struct otg_instance *otg, u8 flag)
{
        printk(KERN_INFO"%s:Entered\n", __FUNCTION__);
        otg_event(otg, OCD_OK, CORE, "GENERIC OK");
}

/*!
 * otg_hcd_set_ops() -
 * @param hcd_ops - host operations table to use
 */
struct hcd_instance * otg_set_hcd_ops(struct hcd_ops *hcd_ops)
{
        otg_hcd_ops = hcd_ops;
        if (hcd_ops) {
                otg_instance_private.otg_output_ops[HCD_INIT_OUT] = otg_hcd_ops->hcd_init_func;
                otg_instance_private.otg_output_ops[HCD_EN_OUT] = otg_hcd_ops->hcd_en_func;
                otg_instance_private.otg_output_ops[HCD_RH_OUT] = otg_hcd_ops->hcd_rh_func;
                otg_instance_private.otg_output_ops[LOC_SOF_OUT] = otg_hcd_ops->loc_sof_func;
                otg_instance_private.otg_output_ops[LOC_SUSPEND_OUT] = otg_hcd_ops->loc_suspend_func;
                otg_instance_private.otg_output_ops[REMOTE_WAKEUP_EN_OUT] = otg_hcd_ops->remote_wakeup_en_func;
                otg_instance_private.otg_output_ops[HNP_EN_OUT] = otg_hcd_ops->hnp_en_func;
        }
        else
                otg_instance_private.otg_output_ops[HCD_INIT_OUT] =
                otg_instance_private.otg_output_ops[HCD_EN_OUT] =
                otg_instance_private.otg_output_ops[HCD_RH_OUT] =
                otg_instance_private.otg_output_ops[LOC_SOF_OUT] =
                otg_instance_private.otg_output_ops[LOC_SUSPEND_OUT] =
                otg_instance_private.otg_output_ops[REMOTE_WAKEUP_EN_OUT] =
                otg_instance_private.otg_output_ops[HNP_EN_OUT] = NULL;


        UNLESS (otg_instance_private.otg_output_ops[HCD_INIT_OUT])
                otg_instance_private.otg_output_ops[HCD_INIT_OUT] = otg_gen_init_func;

        return &hcd_instance_private;
}

/*!
 * otg_ocd_set_ops() -
 *
 * @param ocd_ops - ocd operations table to use
 */
struct ocd_instance * otg_set_ocd_ops(struct ocd_ops *ocd_ops)
{
        otg_ocd_ops = ocd_ops;
        if (ocd_ops) {
                otg_instance_private.otg_output_ops[OCD_INIT_OUT] = otg_ocd_ops->ocd_init_func;
                otg_instance_private.start_timer = ocd_ops->start_timer;
        }
        else {
                otg_instance_private.otg_output_ops[OCD_INIT_OUT] = NULL;
                otg_instance_private.start_timer = NULL;
        }

        UNLESS (otg_instance_private.otg_output_ops[OCD_INIT_OUT])
                otg_instance_private.otg_output_ops[OCD_INIT_OUT] = otg_gen_init_func;

        return &ocd_instance_private;
}

/*!
 * otg_pcd_set_ops() -
 * @param pcd_ops - pcd operations table to use
 */
struct pcd_instance * otg_set_pcd_ops(struct pcd_ops *pcd_ops)
{
        otg_pcd_ops = pcd_ops;
        if (pcd_ops) {
                otg_instance_private.otg_output_ops[PCD_INIT_OUT] = otg_pcd_ops->pcd_init_func;
                otg_instance_private.otg_output_ops[PCD_EN_OUT] = otg_pcd_ops->pcd_en_func;
                otg_instance_private.otg_output_ops[REMOTE_WAKEUP_OUT] = otg_pcd_ops->remote_wakeup_func;
        }
        else {
                otg_instance_private.otg_output_ops[PCD_INIT_OUT] =
                otg_instance_private.otg_output_ops[PCD_EN_OUT] =
                otg_instance_private.otg_output_ops[REMOTE_WAKEUP_OUT] = NULL;
	}

        UNLESS (otg_instance_private.otg_output_ops[PCD_INIT_OUT])
                otg_instance_private.otg_output_ops[PCD_INIT_OUT] = otg_gen_init_func;

        return &pcd_instance_private;
}

/*!
 * otg_tcd_set_ops() -
 * @param tcd_ops - tcd operations table to use
 */
struct tcd_instance * otg_set_tcd_ops(struct tcd_ops *tcd_ops)
{
        otg_tcd_ops = tcd_ops;
        if (tcd_ops) {
                otg_instance_private.otg_output_ops[TCD_INIT_OUT] = otg_tcd_ops->tcd_init_func;
                otg_instance_private.otg_output_ops[TCD_EN_OUT] = otg_tcd_ops->tcd_en_func;
                otg_instance_private.otg_output_ops[CHRG_VBUS_OUT] = otg_tcd_ops->chrg_vbus_func;
                otg_instance_private.otg_output_ops[DRV_VBUS_OUT] = otg_tcd_ops->drv_vbus_func;
                otg_instance_private.otg_output_ops[DISCHRG_VBUS_OUT] = otg_tcd_ops->dischrg_vbus_func;
                otg_instance_private.otg_output_ops[DP_PULLUP_OUT] = otg_tcd_ops->dp_pullup_func;
                otg_instance_private.otg_output_ops[DM_PULLUP_OUT] = otg_tcd_ops->dm_pullup_func;
                otg_instance_private.otg_output_ops[DP_PULLDOWN_OUT] = otg_tcd_ops->dp_pulldown_func;
                otg_instance_private.otg_output_ops[DM_PULLDOWN_OUT] = otg_tcd_ops->dm_pulldown_func;
                //otg_instance_private.otg_output_ops[PERIPHERAL_HOST] = otg_tcd_ops->peripheral_host_func;
                otg_instance_private.otg_output_ops[CLR_OVERCURRENT_OUT] = otg_tcd_ops->overcurrent_func;
                otg_instance_private.otg_output_ops[DM_DET_OUT] = otg_tcd_ops->dm_det_func;
                otg_instance_private.otg_output_ops[DP_DET_OUT] = otg_tcd_ops->dp_det_func;
                otg_instance_private.otg_output_ops[CR_DET_OUT] = otg_tcd_ops->cr_det_func;
                otg_instance_private.otg_output_ops[AUDIO_OUT] = otg_tcd_ops->audio_func;
                otg_instance_private.otg_output_ops[CHARGE_PUMP_OUT] = otg_tcd_ops->charge_pump_func;
                otg_instance_private.otg_output_ops[BDIS_ACON_OUT] = otg_tcd_ops->bdis_acon_func;
                //otg_instance_private.otg_output_ops[MX21_VBUS_DRAIN] = otg_tcd_ops->mx21_vbus_drain_func;
                otg_instance_private.otg_output_ops[ID_PULLDOWN_OUT] = otg_tcd_ops->id_pulldown_func;
                otg_instance_private.otg_output_ops[UART_OUT] = otg_tcd_ops->uart_func;
                otg_instance_private.otg_output_ops[MONO_OUT] = otg_tcd_ops->mono_func;
        }
        else  {
                otg_instance_private.otg_output_ops[TCD_INIT_OUT] = 
                otg_instance_private.otg_output_ops[TCD_EN_OUT] = 
                otg_instance_private.otg_output_ops[CHRG_VBUS_OUT] = 
                otg_instance_private.otg_output_ops[DRV_VBUS_OUT] = 
                otg_instance_private.otg_output_ops[DISCHRG_VBUS_OUT] = 
                otg_instance_private.otg_output_ops[DP_PULLUP_OUT] = 
                otg_instance_private.otg_output_ops[DM_PULLUP_OUT] = 
                otg_instance_private.otg_output_ops[DP_PULLDOWN_OUT] = 
                otg_instance_private.otg_output_ops[DM_PULLDOWN_OUT] = 
                //otg_instance_private.otg_output_ops[PERIPHERAL_HOST] = 
                otg_instance_private.otg_output_ops[CLR_OVERCURRENT_OUT] =
                otg_instance_private.otg_output_ops[DM_DET_OUT] =
                otg_instance_private.otg_output_ops[DP_DET_OUT] =
                otg_instance_private.otg_output_ops[CR_DET_OUT] = 
                otg_instance_private.otg_output_ops[AUDIO_OUT] =
                otg_instance_private.otg_output_ops[CHARGE_PUMP_OUT] =
                otg_instance_private.otg_output_ops[BDIS_ACON_OUT] =
                //otg_instance_private.otg_output_ops[MX21_VBUS_DRAIN] = 
                otg_instance_private.otg_output_ops[ID_PULLDOWN_OUT] =
                otg_instance_private.otg_output_ops[UART_OUT] =
                otg_instance_private.otg_output_ops[MONO_OUT] = NULL;
	}


        UNLESS (otg_instance_private.otg_output_ops[TCD_INIT_OUT])
                otg_instance_private.otg_output_ops[TCD_INIT_OUT] = otg_gen_init_func;

        return &tcd_instance_private;
}

/*!
 * otg_set_usbd_ops() -
 * @param usbd_ops
 */
int otg_set_usbd_ops(struct usbd_ops *usbd_ops)
{
        otg_usbd_ops = usbd_ops;
        return 0;
}


/*!
 * otg_get_trace_info()
 */
void otg_get_trace_info(otg_trace_t *p)
{
        p->id_gnd = otg_instance_private.current_inputs & ID_GND ? 1 : 0;
        p->hcd_pcd = 0;

        if (hcd_instance_private.active) p->hcd_pcd |= 0x1;
        if (pcd_instance_private.active) p->hcd_pcd |= 0x2;

        p->va.s.ticks = (otg_ocd_ops && otg_ocd_ops->ticks) ? otg_ocd_ops->ticks () : 0;
        //p->va.s.interrupts = (otg_ocd_ops && otg_ocd_ops->interrupts) ? otg_ocd_ops->interrupts : 0;
        p->va.s.interrupts = otg_instance_private.interrupts;
        p->va.s.h_framenum = ((otg_hcd_ops && otg_hcd_ops->framenum) ? otg_hcd_ops->framenum() : 0);
        p->va.s.p_framenum = ((otg_pcd_ops && otg_pcd_ops->framenum) ? otg_pcd_ops->framenum() : 0);
        p->in_interrupt = in_interrupt();
}

/*!
 * otg_tmr_ticks() -
 * @return number of ticks.
 */
u64 otg_tmr_ticks(void)
{
        return (otg_ocd_ops && otg_ocd_ops->ticks) ? otg_ocd_ops->ticks () : 0;
}

/*!
 * otg_tmr_elapsed() -
 * @param t1
 * @param t2
 * @return number of uSecs between t1 and t2 ticks.
 */
u64 otg_tmr_elapsed(u64 *t1, u64 *t2)
{
	return (otg_ocd_ops && otg_ocd_ops->elapsed) ? otg_ocd_ops->elapsed (t1, t2) : 0;
}

/*!
 * otg_tmr_framenum() -
 * @return the current USB frame number.
 */
u16 otg_tmr_framenum(void)
{
        return 
                otg_instance_private.current_inputs & ID_GND ? 
                ((otg_hcd_ops && otg_hcd_ops->framenum) ? otg_hcd_ops->framenum() : 0) :
                        ((otg_pcd_ops && otg_pcd_ops->framenum) ? otg_pcd_ops->framenum() : 0);
}

/*!
 * otg_tmr_interrupts() -
 * @return the current number of interrupts.
 */
u32 otg_tmr_interrupts(void)
{
	//return (otg_ocd_ops && otg_ocd_ops->interrupts) ? otg_ocd_ops->interrupts : 0;
	return otg_instance_private.interrupts;
}


OTG_EXPORT_SYMBOL(otg_hcd_ops);
OTG_EXPORT_SYMBOL(otg_ocd_ops);
OTG_EXPORT_SYMBOL(otg_pcd_ops);
OTG_EXPORT_SYMBOL(otg_tcd_ops);
OTG_EXPORT_SYMBOL(otg_set_hcd_ops);
OTG_EXPORT_SYMBOL(otg_set_ocd_ops);
OTG_EXPORT_SYMBOL(otg_set_pcd_ops);
OTG_EXPORT_SYMBOL(otg_set_tcd_ops);
OTG_EXPORT_SYMBOL(otg_set_usbd_ops);
OTG_EXPORT_SYMBOL(otg_tmr_ticks);
OTG_EXPORT_SYMBOL(otg_tmr_elapsed);
OTG_EXPORT_SYMBOL(otg_write_input_message_irq);


#if defined(OTG_WINCE)
#else /* defined(OTG_WINCE) */
#endif /* defined(OTG_WINCE) */

