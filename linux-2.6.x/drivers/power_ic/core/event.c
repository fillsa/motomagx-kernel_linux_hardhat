/*
 * Copyright (C) 2004, 2006 - 2007 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
 *
 * Motorola 2007-Mar-14 - Do not clear interrupt status bits if they are masked.
 * Motorola 2007-Jan-25 - Add support for power management
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Oct-06 - Update File
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jun-28 - Remove /asm/arch/gpio.h - unused
 * Motorola 2006-Jun-19 - Fix montavista upmerge conditionals
 * Motorola 2006-Apr-06 - Fix event support for EOC
 * Motorola 2006-Apr-05 - Add MPM support.
 * Motorola 2006-Apr-04 - Move all GPIO functionality to gpio.c
 * Motorola 2006-Feb-10 - Add ArgonLV support
 * Motorola 2006-Dec-10 - Finalize design of the event handler
 * Motorola 2004-Dec-06 - Redesign of the Power IC event handler 
 */

/*!
 * @file event.c
 *
 * @ingroup poweric_core
 *
 * @brief Power IC event (interrupt) handling
 *
 * This file contains all of the functions and data structures required to
 * implement the power IC event handling.  This includes:
 *
 *    - The actual interrupt handler
 *    - The bottom half interrupt handler
 *    - The public, kernel-level interface for event handling.
 *
 * The power IC has three registers related to interrupts: the interrupt status
 * register, the interrupt sense register, and the interrupt mask register.
 * The status register is a latched indication that the interrupt condition
 * occurred.  That is, when the condition that causes the interrupt occurs,
 * the interrupt status bit is set and can only be cleared by a write to the
 * interrupt status register.  For some interrupts, there is internal hardware
 * debouncing that will occcur before the interrupt status bit will be set.
 *
 * The interrupt sense register is used to indicate the current, unlatched state
 * of the event.  The bits in this register are read-only and can change at any
 * time.
 *
 * The interrupt mask register is used to indicate which interrupt status bits
 * can cause the interrupt line to the processor to be asserted.  Each zero bit
 * in the interrupt mask register indicates that when the corresponding interrupt
 * status bit is set, the external interrupt line will be asserted.  Note that
 * the interrupt mask bits do not prevent the interrupt status bits from being
 * set -- they only affect the external interrupt line.
 *
 * The way that interrupt handling is managed in this driver is through two levels
 * of interrupt handling.  There is a top-half interrupt handler that is called
 * whenever there is a rising edge on the external interrupt line going into the
 * processor.  This interrupt handler is executed when all other interrupts are
 * disabled, so it needs to run as quickly as possible.  The only thing that the
 * top-half interrupt handler does is to "schedule" the bottom-half interrupt
 * handler.
 *
 * The bottom-half interrupt handler is run in "bottom-half" or task context,
 * meaning that interrupts have been re-enabled and are allowed to interrupt the
 * bottom-half interrupt handler.  The bottom-half interrupt handler is responsible
 * for:
 *
 *    - Clearing the interrupt condition in the power IC (to allow the interrupt
 *      line to go back low.
 *    - Dispatching the interrupt event to registered callback functions.
 *
 */

#include <linux/list.h>
#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <linux/mpm.h>

#include "event.h"
#include "gpio.h"
#include "os_independent.h"
#include "thread.h"

#include "atlas_register.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
EXPORT_SYMBOL(power_ic_event_subscribe);
EXPORT_SYMBOL(power_ic_event_unsubscribe);
EXPORT_SYMBOL(power_ic_event_mask);
EXPORT_SYMBOL(power_ic_event_unmask);
EXPORT_SYMBOL(power_ic_event_clear);
EXPORT_SYMBOL(power_ic_event_sense_read);
#endif

/******************************************************************************
* Local type definitions
******************************************************************************/

/*!
 * @brief Define a type for implementing a list of callback functions.
 */
typedef struct
{
    struct list_head list; /*!< the list head for the list of callbacks */
    POWER_IC_EVENT_CALLBACK_T callback; /*!< the callback function pointer */
} POWER_IC_EVENT_CALLBACK_LIST_T;

/******************************************************************************
* Local constants
******************************************************************************/

/******************************************************************************
* Local variables
******************************************************************************/
/*! Define the event lists */
struct list_head power_ic_events[POWER_IC_EVENT_NUM_EVENTS];

/* Declare a wait queue used to signal arriving interrupts to the interrupt handler thread */
static DECLARE_WAIT_QUEUE_HEAD(interrupt_thread_wait);

/*! Flag to indicate pending interrupt conditions */ 
static int interrupt_flag;

/******************************************************************************
* Global variables
******************************************************************************/
int power_ic_pm_suspend_mask_tbl[POWER_IC_PM__NUM] =
{
    0, /* POWER_IC_PM_INTERRUPT_0 */
    0, /* POWER_IC_PM_INTERRUPT_1 */
    0, /* POWER_IC_PM_MISC_SLEEPS */
};

/******************************************************************************
* Local function prototypes
******************************************************************************/
static void power_ic_bh_handler(unsigned long unused);

/******************************************************************************
* Local functions
******************************************************************************/
/*!
 * @brief Executes the registered callback functions for a given event
 *
 * Iterates over the list of registered callback functions (saved in
 * #power_ic_events[]) and calls each of the callback functions.  The
 * return value from the callback indicates if the event was handled
 * by the callback and if additional callbacks should be called.  That
 * is, if a callback function returns a non-zero value, the event is
 * considered handled and no other callbacks are called for the event.
 *
 * The list of callbacks is implemented using the Linux standard
 * doubly-linked list implementation.
 *
 * @param       event    the event that occurred
 *
 * @return      nothing
 */

static void execute_handlers (POWER_IC_EVENT_T event)
{
    struct list_head *p;
    POWER_IC_EVENT_CALLBACK_LIST_T *t;

    list_for_each (p, &(power_ic_events[event]))
    {
        t = list_entry (p, POWER_IC_EVENT_CALLBACK_LIST_T, list);

        /* If the callback returns a non-zero value, it handled the event */
        if (t->callback(event) != 0)
        {
            break;
        }
    }
}

/*!
 * @brief Implements the kernel thread for power IC "bottom half" interrupt handling.
 *
 * The function that implements the kernel thread for interrupt handling.  This
 * is used to "simulate" a bottom half because the I2C driver only works in task
 * context, not in interrupt or bottom half context.  The function simply executes
 * the same bottom half function that would normally run in the bottom half tasklet.
 *
 * @param unused An unused parameter
 *
 * @return 0, but the function will only return upon receiving an abort signal.
 */

static int interrupt_thread_loop (void *unused)
{
    /* Usual thread setup. */
    thread_common_setup("keventd");
    if(thread_set_realtime_priority(THREAD_PRIORITY_EVENT) != 0)
    {
        tracemsg(_a("Event thread - error setting thread priority."));
    }

    /*
     * Loop unless an abort siganl is received.  All signals, but the abort signals are
     * masked off in the common setup.  As a result only abort signals can be pending.
     */
    while(!signal_pending(current))
    {
        /* Sleep if an interrupt isn't pending again */
        if (wait_event_interruptible (interrupt_thread_wait, interrupt_flag) < 0)
        {
            break;
        }

        /* Reset the interrupt flag */
        interrupt_flag = 0;

        /* Handle the interrupt */
        power_ic_bh_handler(0);
    }

    return 0;
}

/*!
 * @brief The bottom half of the power IC interrupt handler.
 *
 * This function implements the bottom half of the power IC interrupt handler.
 *
 * For interrupt handling, the function loops as long as the power IC continues to
 * assert its interrupt line to the processor.  In each iteration of the loop,
 * the interrupt status and interrupt mask registers are read from the ic to determine
 * the set of outstanding interrupts.  These interrupts are then "dispatched" to
 * the registered callback functions.  All of the pending interrupts are then
 * cleared and masked.
 *
 * @note This function normally runs in bottom-half context, meaning that interrupts
 * are enabled, but user processes are being preempted.  This needs to execute quickly
 * to allow control to return back to user-space as soon as possible, but it is not
 * absolutely time critical.
 *
 * @param unused An unused parameter
 *
 * @return nothing
 */

static void power_ic_bh_handler(unsigned long unused)
{
    unsigned int isr0;
    POWER_IC_UNUSED unsigned int isr1;
    unsigned int imr0;
    POWER_IC_UNUSED unsigned int imr1;
    unsigned int enabled_ints0;
    POWER_IC_UNUSED unsigned int enabled_ints1;
    unsigned int int_vec;

    /* Loop while power IC continues to assert its interrupt line */
    while (power_ic_gpio_event_read_priint(PM_INT))
    {
        /* Read the interrupt status and mask registers */
        power_ic_read_reg (POWER_IC_REG_ATLAS_INT_STAT_0, &isr0);
        power_ic_read_reg (POWER_IC_REG_ATLAS_INT_MASK_0, &imr0);
        /* Clear the interrupt status bits of the unmasked interrupts only */
        power_ic_write_reg_value (POWER_IC_REG_ATLAS_INT_STAT_0, (isr0 & ~imr0));
        
        /* Mask all interrupts that we are about to service */
        power_ic_write_reg_value (POWER_IC_REG_ATLAS_INT_MASK_0, imr0 | isr0);

        
        /* Read the interrupt status and mask registers */
        power_ic_read_reg (POWER_IC_REG_ATLAS_INT_STAT_1, &isr1);
        power_ic_read_reg (POWER_IC_REG_ATLAS_INT_MASK_1, &imr1);
        
        /* Clear the interrupt status bits of the unmasked interrupts only */
        power_ic_write_reg_value (POWER_IC_REG_ATLAS_INT_STAT_1, (isr1 & ~imr1));
        
        /* Mask all interrupts that we are about to service */
        power_ic_write_reg_value (POWER_IC_REG_ATLAS_INT_MASK_1, imr1 | isr1);

        
        /* Get the set of enabled interrupt bits */
        enabled_ints0 = isr0 & ~imr0;
        enabled_ints1 = isr1 & ~imr1;

        /* Enable the pm suspend bit for the interrupt received */
        power_ic_pm_suspend_mask_tbl[POWER_IC_PM_INTERRUPT_1] |=
            (enabled_ints1 & (POWER_IC_ONOFF_MASK | POWER_IC_MB2_MASK | POWER_IC_HSDET_MASK));
            
        /* Loop to handle the interrupts */
        while (enabled_ints0 != 0)
        {
            /* Find the interrupt number of the highest priority interrupt */
            int_vec = ffs(enabled_ints0) - 1;

            /* Run the handlers for the event */
            execute_handlers(int_vec + POWER_IC_EVENT_ATLAS_FIRST_REG);

            /* Clear the bit for the interrupt we just serviced */
            enabled_ints0 &= ~(1 << int_vec);
        }
        /* Loop to handle the interrupts for Register 1 */
        while (enabled_ints1 != 0)
        {
            /* Find the interrupt number of the highest priority interrupt */
            int_vec = ffs(enabled_ints1) - 1;

            /* Run the handlers for the event for Interrupt Status Register 1 */
            execute_handlers(int_vec + POWER_IC_EVENT_ATLAS_SECOND_REG);

            /* Clear the bit for the interrupt we just serviced */
            enabled_ints1 &= ~(1 << int_vec);
        }
    }

}

/******************************************************************************
* Global functions
******************************************************************************/

/*!
 * @brief Interrupt handler for power IC interrupts
 *
 * This is the interrupt handler for the power IC interrupts.
 * The purpose of the interrupt handler is to schedule the bottom half
 * interrupt handler.  Due to problems with the I2C driver failing
 * to operate in interrupt (or bottom half) context, the bottom half of
 * the interrupt is implemented as a kernel thread.  The interrupt handler
 * wakes up the thread (which is waiting on a wait queue).
 *
 * @note This function runs with context switching and all other interrupts
 * disabled, so it needs to run as fast as possible.
 *
 * @param        irq        the irq number
 * @param        dev_id     the pointer on the device
 * @param        regs       the interrupt parameters
 *
 * @return       The function returns IRQ_RETVAL(1) when handled.
 */
irqreturn_t power_ic_irq_handler (int irq, void *dev_id, struct pt_regs *regs)
{
    /* Inform power management about interrupt of interest */
    mpm_handle_ioi();
    /* Set the interrupt flag to prevent the thread from sleeping */
    interrupt_flag = 1;

    /* Wake up the "bottom half" interrupt handler thread */
    wake_up(&interrupt_thread_wait);

    return IRQ_RETVAL(1);
}

/*!
 * @brief Initializes the power IC event handling
 *
 * This function initializes the power IC event handling.  This includes:
 *     - Initializing the doubly-linked lists in the #power_ic_events array
 *     - Configuring the GPIO interrupt lines
 *     - Registering the GPIO interrupt handlers
 *
 * This function is also responsible for creating the kernel thread that will be
 * use as a temporary replacement for the bottom half interrupt tasklet due to
 * issues with the I2C driver.
 *
 * @return nothing
 */
void power_ic_event_initialize (void)
{
    int i;
    
    /* Initialize the lists for power IC events */
    for (i = 0; i < POWER_IC_EVENT_NUM_EVENTS; i++)
    {
        INIT_LIST_HEAD(&(power_ic_events[i]));
    }

    /* Start our kernel thread */
    kernel_thread(interrupt_thread_loop, NULL, 0);

    /* Configure and register event interrupts */
    power_ic_gpio_config_event_int();
}

/*!
 * @brief Registers a callback function to be called when an event occurs
 *
 * This function is used to subscribe to an event.  The callback function is
 * stored in a dynamically-allocated structure and then added to a doubly-linked
 * list implemented using the standard Linux doubly-linked list implementation.
 * Multiple callbacks may be added for each event.  Duplicated callback function
 * checking is not implemented.
 *
 * @param        event     the event type
 * @param        callback  the function to call when event occurrs
 *
 * @return 0 when successful or -ENOMEM if memory allocation failed
 */

int power_ic_event_subscribe (POWER_IC_EVENT_T event, POWER_IC_EVENT_CALLBACK_T callback)
{
    POWER_IC_EVENT_CALLBACK_LIST_T *temp;

    /* Verify that the event number is not out of range */
    if (event >= POWER_IC_EVENT_NUM_EVENTS)
    {
        return -EINVAL;
    }

    /* Create a new linked list entry */
    temp = kmalloc(sizeof(POWER_IC_EVENT_CALLBACK_LIST_T), GFP_KERNEL);
    if (temp == NULL)
    {
        return -ENOMEM;
    }

    /* Initialize the fields of the list */
    temp->callback = callback;
    INIT_LIST_HEAD(&temp->list);

    /* Add the entry to the list for the requested event */
    list_add (&temp->list, &(power_ic_events[event]));

    return 0;
}

/*!
 * @brief Unregisters a callback function for a given event
 *
 * The function iterates over the list of callback functions registered for the
 * given event.  If one is found matching the provided callback function pointer,
 * the callback is removed from the list.  If the callback function is included
 * in the list more than once, all instances are removed.  Memory allocated for
 * the list entry is also freed when the entry is removed.
 *
 * @param        event     the event type
 * @param        callback  the callback function to unregister
 *
 * @return always returns 0
 */

int power_ic_event_unsubscribe (POWER_IC_EVENT_T event, POWER_IC_EVENT_CALLBACK_T callback)
{
    struct list_head *p;
    struct list_head *n;
    POWER_IC_EVENT_CALLBACK_LIST_T *t;

    /* Verify that the event number is not out of range */
    if (event >= POWER_IC_EVENT_NUM_EVENTS)
    {
        return -EINVAL;
    }

    /* Find the entry in the list */
    list_for_each_safe (p, n, &(power_ic_events[event]))
    {
        t = list_entry (p, POWER_IC_EVENT_CALLBACK_LIST_T, list);
        if (t->callback == callback)
        {
            /* Remove the entry from the list and free the memory */
            list_del(p);
            kfree(t);
        }
    }

    return 0;
}

/*!
 * @brief Unmasks (enables) an event.
 *
 * This function clears the interrupt mask bit for the given event.  This will
 * allow interrupts to be generated for the event when the event occurs.  Once
 * the event is unmasked, the event callback could be called at any time (including
 * before this function returns back to the caller).
 *
 * @note If the interrupt is pending and masked, when it is unmasked, the callback
 * will be called immediately.
 *
 * @pre While not specifically required, a callback function should generally be
 * registered before unmasking an event.
 *
 * @param        event   the event type to unmask
 *
 * @return 0 on success
 */

int power_ic_event_unmask (POWER_IC_EVENT_T event)
{
    /* Verify that the event number is not out of range */
    if (event >= POWER_IC_EVENT_NUM_EVENTS)
    {
        return -EINVAL;
    }
    
    else if (event >= POWER_IC_EVENT_ATLAS_SECOND_REG)
    {
        /* Set the interrupt mask bit to 0 to unmask the interrupt -- From Interrupt Mask 1 */
        return power_ic_set_reg_bit (
            POWER_IC_REG_ATLAS_INT_MASK_1,
            event - POWER_IC_EVENT_ATLAS_SECOND_REG,
            0);
    }
    else
    {
        /* Set the interrupt mask bit to 0 to unmask the interrupt -- From Interrupt Mask 0 */
        return power_ic_set_reg_bit (
            POWER_IC_REG_ATLAS_INT_MASK_0,
            event - POWER_IC_EVENT_ATLAS_FIRST_REG,
            0);
    }
}

/*!
 * @brief Masks (disables) an event.
 *
 * This function sets the interrupt mask bit for the given event.  This will
 * prevent interrupts from being generated for the event when it occurrs.
 *
 * @param        event   the event type to mask
 *
 * @return 0 on success
 */

int power_ic_event_mask (POWER_IC_EVENT_T event) 
{
    /* Verify that the event number is not out of range */
    if (event >= POWER_IC_EVENT_NUM_EVENTS)
    {
        return -EINVAL;
    }

    else if (event >= POWER_IC_EVENT_ATLAS_SECOND_REG)
    {
        /* Set the interrupt mask bit to 1 to mask the interrupt -- from Interrupt Mask 1 */
        return power_ic_set_reg_bit (
            POWER_IC_REG_ATLAS_INT_MASK_1,
            event - POWER_IC_EVENT_ATLAS_SECOND_REG,
            1);
    }
    else
    {
        /* Set the interrupt mask bit to 1 to mask the interrupt -- from Interrupt Mask 0*/
        return power_ic_set_reg_bit (
            POWER_IC_REG_ATLAS_INT_MASK_0,
            event - POWER_IC_EVENT_ATLAS_FIRST_REG,
            1);
    }
}

/*!
 * @brief Clears an event (interrupt) flag in the power IC.
 *
 * This function clears the interrupt status flag in the power IC for the given
 * event.  This function would generally be called before an event is unmasked to
 * ensure that any previously pending event doesn't cause the callback to
 * called immediately.
 *
 * @note When the event ocurrs, the event flag is automatically cleared after
 * all of the callback functions have been executed.
 *
 * @param        event   the event type to clear.
 *
 * @return 0 on success
 */

int power_ic_event_clear (POWER_IC_EVENT_T event) 
{
    /* Verify that the event number is not out of range */
    if (event >= POWER_IC_EVENT_NUM_EVENTS)
    {
        return -EINVAL;
    }

    else if (event >= POWER_IC_EVENT_ATLAS_SECOND_REG)
    {
        /* Set the interrupt status bit to 1 to clear the flag -- From Interrupt Status 1 */
        return power_ic_set_reg_bit (
            POWER_IC_REG_ATLAS_INT_STAT_1,
            event - POWER_IC_EVENT_ATLAS_SECOND_REG,
            1);
    }
    else 
    {
        /* Set the interrupt status bit to 1 to clear the flag -- From Interrupt Status 0 */
        return power_ic_set_reg_bit (
            POWER_IC_REG_ATLAS_INT_STAT_0,
            event - POWER_IC_EVENT_ATLAS_FIRST_REG,
            1);
    }
}

/*!
 * @brief Reads an interrupt sense flag from the power IC.
 *
 * This function reads the interrupt sense flag in the power IC for the given
 * event.
 *
 * @param        event   the event type to read
 *
 * @return the state of the interrupt sense or <0 on failure.
 */

int power_ic_event_sense_read (POWER_IC_EVENT_T event)
{
    int retval;
    int value;

    /* Verify that the event number is not out of range */
    if (event >= POWER_IC_EVENT_NUM_EVENTS)
    {
        return -EINVAL;
    }

    if (event >= POWER_IC_EVENT_ATLAS_SECOND_REG)
    {
        /* Read the interrupt sense 1 bit from register 4 */
        retval = power_ic_get_reg_value (
            POWER_IC_REG_ATLAS_INT_SENSE_1,
            event - POWER_IC_EVENT_ATLAS_SECOND_REG,
            &value,
            1);
    }
    else
    {
        /* Read the interrupt sense 0 bit from register 2 */
        retval = power_ic_get_reg_value (
            POWER_IC_REG_ATLAS_INT_SENSE_0,
            event - POWER_IC_EVENT_ATLAS_FIRST_REG,
            &value,
            1);
    } 

    /* Return the error or the value of the interrupt sense bit */
    return (retval < 0) ? retval : value;
}
