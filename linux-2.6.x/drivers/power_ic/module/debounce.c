/*
 * Copyright (C) 2005-2007 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
 *
 * Motorola 2007-Jan-25 - Add support for power management 
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Sep-19 - Replace headset_key_handler() and power_key_event() with generate_key_event()
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-May-17 - Fix unused function warning.
 * Motorola 2006-Apr-20 - Add 3.5mm headset support
 * Motorola 2005-Oct-15 - Finalized the software.
 * Motorola 2005-Jun-16 - Added ATLAS support
 * Motorola 2005-Feb-28 - Rewrote the software.
 */

/*!
 * @file debounce.c
 *
 * @ingroup poweric_debounce
 *
 * @brief This is the main file of the power IC debouncing routines
 *
 * This file handles the debouncing of various interrupts from the power IC.
 * The module is desgined to be as generic as possible, with a table that
 * defines the interrupts that are to be monitored (debounced) along with
 * a callback function to be called when a change to the signal has been
 * fully debounced.
 *
 * Currently, this file handles debouncing of the power key, barrel headset,
 * and barrel headset send/end key.  In the future, stereo headset detection
 * will need to be added as well.
 */

/*******************************************************************************
* Includes
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/keypad.h>
#include <linux/timer.h>
#include <linux/sched.h>

#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include <linux/moto_accy.h>

#include "../core/event.h"
#include "../core/os_independent.h"
#include "../core/thread.h"

/*******************************************************************************
* Macros and Constants
*******************************************************************************/

/*!
 * @brief Converts milliseconds to jiffies
 *
 *Accepts a parameter in units of milliseconds and defines value in jiffies
 */
#define TO_JIFFIES(msec) (1 + (((msec) * HZ) / 1000))
/*!
 * @brief Defines maximum number of events allowed in queue
 */
#define QUEUE_MAX_EVENTS 16
/*!
 * @brief Used as high-cutoff value for detection of headset type
 *
 *1.5 Volts
 */
#define HEADSET_DET_HIGH 0x29B
/*!
 * @brief Used as low-cutoff value for detection of headset type
 *
 *0.5 Volts
 */
#define HEADSET_DET_LOW  0x0DE

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define POWER_IC_EVENT_MB2I         POWER_IC_EVENT_ATLAS_MC2BI
#define POWER_IC_EVENT_ONOFFI       POWER_IC_EVENT_ATLAS_ONOFD1I
#define POWER_IC_EVENT_HEADSETI     POWER_IC_EVENT_ATLAS_HSDETI
#endif /* Doxygen skips over this */

/*******************************************************************************
* Type definitions
*******************************************************************************/

/*!
 * @brief Structure defining a power IC event to debounce
 */
typedef struct debounce_event_t
{
    POWER_IC_EVENT_T event;    /*!< power IC event to watch */

    int spi_bit_mask;          /*!< power IC SPI bit mask */

    int enabled;               /*!< 1 = event is enabled, 0 = disabled */

    int poll_period;           /*!< polling period (in milliseconds) */
    int poll_count;            /*!< number of times to poll */

    int polarity;              /*!< Active high (0) or low (1)*/

    void (*callback)(struct debounce_event_t *, int status); /*!< function to call when polling complete */
    void (*pre_debounce)(void);  /*!< function to call before debouncing begins */

    /* Entries after this point should not be initialized in debounce_events */

    int poll_current;          /*!< the current polling counter */
    int previous_status;       /*!< previous state of the event */

    struct timer_list timer;   /*!< timer used for this event */
} DEBOUNCE_EVENT_T;

/*!
 * @brief Defines debounce events for power key, barrel headset, and barrel headset send/end key
 */
enum
{
    DEBOUNCE_EVENT_POWER,
    DEBOUNCE_EVENT_HEADSET,
    DEBOUNCE_EVENT_MB2I
};

/*******************************************************************************
* Local Variables
*******************************************************************************/

/* doxygen is confused by the following definition since it looks like a function */
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/* Declare a wait queue used to signal arriving events to the handler thread */
static DECLARE_WAIT_QUEUE_HEAD(debounce_wait);

#endif

/*! Spin lock to prevent simultaneous access to the queue */
static spinlock_t queue_lock = SPIN_LOCK_UNLOCKED;

/*! Array location of the head of the queue */
static int queue_head = 0;

/*! Array location of the tail of the queue */
static int queue_tail = 0;

/*! The event queue array */
static POWER_IC_EVENT_T event_queue[QUEUE_MAX_EVENTS];

/* Event table (defined later in the file) */
static DEBOUNCE_EVENT_T debounce_events[];

/* Stores the current Atlas audio bias setting */
static int audio_bias_settings = 0;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

/*******************************************************************************
* Local Functions
*******************************************************************************/

/*!
 * @brief Take an event and returns the event index for the debounce_events
 * table.
 *
 * @param event
 *
 * @returns index
 */
static int get_event_index(int event)
{
    int i = 0;
    
    while ((debounce_events[i].event != POWER_IC_EVENT_NUM_EVENTS) &&
           (debounce_events[i].event != event))
    {
        i++;
    }

    return i;
}

/*!
 * @brief the kernel thread responsible for debouncing
 *
 * This function implements the thread responsible for the majority of the
 * debounce handling.  The thread consists of an infinite loop that will
 * wait for interrupts and/or timer expirations.
 *
 * When an event arrives, it is placed into a queue and then the thread is
 * woken up.  When the thread wakes up, it takes events off of the queue and
 * handles them.  For each event handled, the current interrupt sense bit is
 * compared with the previous state of the sense bit.  If the sense bit stays
 * in the same state for X consecutive iterations (where X is defined on a
 * per-event basis), the callback function is called for the event.
 *
 * When not debouncing, the only thing that would cause the thread to start
 * running is an interrupt from the power IC.  While it is in the process or
 * debouncing an event, the event's interrupt is masked and a timer is used
 * to periodically poll the state of the sense bit.  Once debouncing is
 * complete, the interrupt is unmasked.
 *
 * In order to try to eliminate any interrupt race conditions, the interrupt
 * flag is always cleared whenever the interrupt sense is read.  Then, when
 * the interrupt is unmasked, the status is NOT cleared.  This will cause
 * the interrupt to fire immediately if the interrupt sense were to have
 * changed between the time it was last read and the time that the
 * interrupt was unmasked.
 *
 * @param unused An unused parameter
 *
 * @returns 0, but should never actually return
 */
static int debounce_thread (void *unused)
{
    POWER_IC_EVENT_T queued_event;
    int i;
    int status;

    /* Usual thread setup. */
    thread_common_setup("kdebounced");
    if(thread_set_realtime_priority(THREAD_PRIORITY_DEBOUNCE) != 0)
    {
        tracemsg(_k_d("Debounce thread - error setting thread priority."));
    }

    /*
     * Loop unless an abort siganl is received.  All signals, but the abort signals are
     * masked off in the common setup.  As a result only abort signals can be pending.
     */
    while(!signal_pending(current))
    {
        
        /* Iterate over the queue of pending events */
        while (queue_head != queue_tail)
        {
            /* Acquire the queue lock so that we can take an event off of the queue */
            spin_lock(&queue_lock);

            /* Grab the first entry and remove it from the queue */
            queued_event = event_queue[queue_head];
            queue_head = (queue_head + 1) % QUEUE_MAX_EVENTS;

            /* Unlock the queue */
            spin_unlock (&queue_lock);
        
            /* Locate the event entry in the debounce event table */
            i = get_event_index(queued_event);

            /* Only process the event if we found it in the table */
            if (debounce_events[i].event != POWER_IC_EVENT_NUM_EVENTS &&
                debounce_events[i].enabled)
            {
                /* Check if debouncing has begun */
                if ((debounce_events[i].poll_current == 0) && (debounce_events[i].pre_debounce != NULL))
                {
                    /* Since debouncing is just starting, call the pre_debounce function */
                    debounce_events[i].pre_debounce();
                }

                /* Read the sense bit from the power IC */
                power_ic_event_clear (debounce_events[i].event);
                status = power_ic_event_sense_read (debounce_events[i].event);

                /* If the status is the same as the previousstatus, increment the counter */
                if (status == debounce_events[i].previous_status &&
                    debounce_events[i].poll_current < debounce_events[i].poll_count)
                {
                    debounce_events[i].poll_current++;
                }

                /* Else, reset the counter and the previous status variable */
                else
                {
                    debounce_events[i].poll_current = 1;
                    debounce_events[i].previous_status = status;
                }

                /* Check to see if the polling counter has reached its limit */
                if (debounce_events[i].poll_current >= debounce_events[i].poll_count)
                {
                    /* Since we're done debouncing, call the callback function */
                    debounce_events[i].callback(&(debounce_events[i]), status);

                    /* Reset the variables */
                    debounce_events[i].poll_current = 0;
                    debounce_events[i].previous_status = -1;

                    /* Reenable the interrupt */
                    power_ic_event_unmask (debounce_events[i].event);

                    if (debounce_events[i].event >= POWER_IC_EVENT_ATLAS_SECOND_REG)
                    {
                        /* Clear the suspend bit in the suspend mask for interrupt 1 */
                        power_ic_pm_suspend_mask_tbl[POWER_IC_PM_INTERRUPT_1] &= ~(debounce_events[i].spi_bit_mask);
                    }
                    else
                    {
                        /* Clear the suspend bit in the suspend mask for interrupt 0 */
                        power_ic_pm_suspend_mask_tbl[POWER_IC_PM_INTERRUPT_0] &= ~(debounce_events[i].spi_bit_mask);
                    }
                }

                /* Else, need to keep polling, so start a timer */
                else
                {
                    /* Set the expiration time */
                    debounce_events[i].timer.expires =
                        jiffies + TO_JIFFIES(debounce_events[i].poll_period);

                    /* Add the timer to the list */
                    add_timer (&(debounce_events[i].timer));
                }
            }
        }
        
        /* Sleep if there are no more events waiting on the queue or a signal is received. */
        (void)wait_event_interruptible (debounce_wait, (queue_head != queue_tail));
    }

    return 0;
}

/*!
 * @brief adds an event to the event queue
 *
 * This function adds an event to the event queue for the debouncing thread.
 * The ordering of the events in the queue doesn't really matter too much
 * since all of the pending events will be handled by the thread during one
 * iteration.
 *
 * This function checks to see if the queue is full by checking to see if
 * incrementing the tail pointer will make the tail equal to the head
 * pointer.  When the tail and head are equal, it means that the queue
 * is empty, so if incrementing the tail would make it equal to the head,
 * it means that the queue is full and new events cannot be added.
 *
 * The queue is currently large enough so that it is impossible for the
 * queue to fill up.  If in the future many additional events are added
 * into the event table, the queue size may need to be increased.
 *
 * @param event power IC event to add
 */
static void add_event (POWER_IC_EVENT_T event)
{
    int next_event = (queue_tail + 1) % QUEUE_MAX_EVENTS;

    /* Acquire the lock protecting the thread queue */
    spin_lock (&queue_lock);

    /* Add the event to the queue if it isn't already full */
    if (next_event != queue_head)
    {
        /* Store the event into the queue */
        event_queue[queue_tail] = event;

        /* Update the tail pointer */
        queue_tail = next_event;
    }

    /* Release the lock */
    spin_unlock (&queue_lock);

    /* Wake up the thread if thread is sleeping */
    wake_up (&debounce_wait);
}

/*!
 * @brief the event handler for all of the power IC events being debounced
 *
 * This is the power IC event handler for the events registered by the debouncing
 * code.  The function simply adds the event to the queue (which will cause the
 * thread to be woken up to process the event).
 *
 * @param event power IC that occurred
 *
 * @return 1 to indicate that the event has been handled
 */
static int debounce_interrupt_handler (POWER_IC_EVENT_T event)
{
    /* Add the event to the queue */
    add_event(event);

    return 1;
}

/*!
 * @brief the handler for a debounce timer expiration
 *
 * This is the timer event handler for the events registered by the debouncing
 * code.  The function simply adds the event to the queue (which will cause th
 * thread to be woken up to process the timer expiration).  Timers are not
 * cyclic, so when the timer expires, it must be restarted by hand before
 * it will "fire" again.  This is handled by the debouncing thread.
 *
 * @param i entry in the event table that this timer expiration is for
 */
static void debounce_timer_handler (unsigned long i)
{
    /* Add the event to the queue */
    add_event(debounce_events[i].event);
}

/*!
 * @brief callback function to indicate power key status
 *
 * This function is called when the status of the power key changes.  The
 * function will report the new state of the power key to the keypad handling
 * code for furture processing.
 *
 * @param event pointer to the location in the event table
 * @param status current status of the power key
 *
 * @note The filtering of the power key when the flip is closed is done in 
 *       drivers/char/mot_keyb.c so if the keycode is changed in this function,
 *       it must also be changed in mot_keyb.c
 */
static void power_key_debounced (DEBOUNCE_EVENT_T *event, int status)
{
    /* Status is reverse-polarity: 1 = not pressed */
    if (status != debounce_events[DEBOUNCE_EVENT_POWER].polarity)
    {
        tracemsg(_k_d("power_key_debounced: key is released"));
        generate_key_event(KEYPAD_HANGUP, KEYUP);
    }
    else
    {
        tracemsg(_k_d("power_key_debounced: key is pressed"));
        generate_key_event(KEYPAD_HANGUP, KEYDOWN);
    }   
}

/*!
 * @brief Configures Atlas for 3.5mm headset detection
 *
 * This function is called before the HSDET sense bit is debounced. This function
 * will configure Atlas to allow detection of the headset with mic and headset
 * without mic. The time spent debouncing the HSDET sense bit is also used as
 * delay to allow the MC2B line to settle after this configuration.
 */
static void headset_3mm5_pre_debounce (void)
{   
    /* Mask the interrupt */
    power_ic_event_mask(POWER_IC_EVENT_MB2I);

    /* Store the current audio bias settings */
    power_ic_get_reg_value(POWER_IC_REG_ATLAS_AUDIO_RX_0, 1, &audio_bias_settings, 2);
     
    /* Set BIASEN and BIASSPEED bit in Atlas */
    power_ic_set_reg_bit(POWER_IC_REG_ATLAS_AUDIO_RX_0, 1, 1);
    power_ic_set_reg_bit(POWER_IC_REG_ATLAS_AUDIO_RX_0, 2, 1);
        
    /* Set MC2BDETEN */
    power_ic_set_reg_bit(POWER_IC_REG_ATLAS_AUDIO_TX, 3, 1);

    /* Set MC2BEN */
    power_ic_set_reg_bit(POWER_IC_REG_ATLAS_AUDIO_TX, 1, 1);
}

/*!
 * @brief callback function to indicate headset status
 *
 * This function is called when the status of the headset changes.  The
 * function will report the new state of the accessory to the accessory driver
 * code.  The accessory driver will handle notifying any interested applications
 * of the event.
 *
 * The function also handles enabling and disabling of the headset send/end key
 * interrupt.  In order to prevent problems with spurious send/end key interrupts,
 * the send/end key interrupt (MB2) is not enabled until the headset has been
 * inserted and fully debounced.  Likewise, when the headset is removed, the
 * send/end key interrupt will be disabled.
 *
 * @param event pointer to the location in the event table
 * @param status current status of the headset
 */
static void headset_3mm5_debounced(DEBOUNCE_EVENT_T *event, int status)
{
    int i = get_event_index(POWER_IC_EVENT_MB2I); /* Get the MB2 event index (send/end key) in the table */

    /* Headset not attached */
    if (status == debounce_events[DEBOUNCE_EVENT_HEADSET].polarity)
    {
        /* Disable the send/end key event in the table */
        debounce_events[i].enabled = 0;

        /* Set MC2B debouncing time to 30mS */
        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_AUDIO_TX, 2, 0);
            
        /* Disable MC2BDETEN */
        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_AUDIO_TX, 3, 0);
            
        /* Notify the applications that the headset has been removed */
        moto_accy_notify_remove(MOTO_ACCY_TYPE_3MM5_HEADSET_STEREO_MIC);
        moto_accy_notify_remove(MOTO_ACCY_TYPE_3MM5_HEADSET_STEREO);

        tracemsg(_k_d("headset_3mm5_debounced: stereo headset is removed\n"));
    }

    /* Else, headset is attached */
    else
    {
        /* Headset with mic */
        if (power_ic_event_sense_read(POWER_IC_EVENT_MB2I))
        {
            /* Set MC2B debouncing time to 100mS */
            power_ic_set_reg_bit(POWER_IC_REG_ATLAS_AUDIO_TX, 2, 1);
    
            /* Enable the send/end key event in the table */
            debounce_events[i].enabled = 1;

            /* Reset the send/end key debounce variables so that debouncing starts from the beginning */
            debounce_events[i].poll_current = 0;
            debounce_events[i].previous_status = -1;
            
            /* Notify the applications that the stereo headset with mic is attached */
            moto_accy_notify_insert(MOTO_ACCY_TYPE_3MM5_HEADSET_STEREO_MIC);
            
            tracemsg(_k_d("headset_3mm5_debounced: stereo headset with mic is inserted\n"));
            
            /* Clear and unmask the send/end key interrupt */
            power_ic_event_clear(POWER_IC_EVENT_MB2I);
            power_ic_event_unmask(POWER_IC_EVENT_MB2I);
        }
        /* Headset without mic */
        else
        {
            /* Disable the event in the table */
            debounce_events[i].enabled = 0;

            /* Disable MC2BDETEN */
            power_ic_set_reg_bit(POWER_IC_REG_ATLAS_AUDIO_TX, 3, 0);
            
            /* Notify the applications that the stereo headset without mic is attached */
            moto_accy_notify_insert(MOTO_ACCY_TYPE_3MM5_HEADSET_STEREO);
            
            tracemsg(_k_d("headset_3mm5_debounced: stereo headset without mic is inserted\n"));
        }
    }

    /* Set the audio bias to it previous value */
    power_ic_set_reg_value(POWER_IC_REG_ATLAS_AUDIO_RX_0, 1, audio_bias_settings, 2);

    /* Disable MC2BEN */
    power_ic_set_reg_bit(POWER_IC_REG_ATLAS_AUDIO_TX, 1, 0);
}

/*!
 * @brief callback function to indicate headset send/end key status
 *
 * This function is called when the state of the headset send/end key changes.
 * The function will report the new state of the send/end key to the keypad 
 * handling code for furture processing.
 * 
 * @param event pointer to the location in the event table
 * @param status current status of the headset
 */
static void headset_key_debounced (DEBOUNCE_EVENT_T *event, int status)
{
    int hsdet_status;
    
    if (status != debounce_events[DEBOUNCE_EVENT_MB2I].polarity)
    {
        tracemsg(_k_d("headset_key_debounced: headset send/end key is released\n"));
        generate_key_event(KEYPAD_HEADSET, KEYUP);
    }
    else
    {
        power_ic_get_reg_value(POWER_IC_REG_ATLAS_INT_STAT_1, 18, &hsdet_status, 1);

        /* Generate send/end key press only if HSDET sense is one and HSDET status
           has not been generated */
        if ((hsdet_status == 0) && (power_ic_event_sense_read(POWER_IC_EVENT_HEADSETI)))
        {

            tracemsg(_k_d("headset_key_debounced: headset send/end is pressed\n"));
        generate_key_event(KEYPAD_HEADSET, KEYDOWN);
    }
}
}

/*******************************************************************************
* Global Functions
*******************************************************************************/
/*! Event table */
static DEBOUNCE_EVENT_T debounce_events[] =
{
    /* event                   SPI mask            enabled time count polarity  callback                pre-debounce */
    { POWER_IC_EVENT_ONOFFI,   POWER_IC_ONOFF_MASK,      1,  10,   3,        0, power_key_debounced,    NULL                      },
    { POWER_IC_EVENT_HEADSETI, POWER_IC_HSDET_MASK,      1,  75,  10,        0, headset_3mm5_debounced, headset_3mm5_pre_debounce },
    { POWER_IC_EVENT_MB2I,     POWER_IC_MB2_MASK,        0,  15,  10,        0, headset_key_debounced,  NULL                      },
    /* End of table -- insert new entries before this one */
    { POWER_IC_EVENT_NUM_EVENTS,                 0,      0,   0,   0,        0,                  NULL,  NULL                      }
};

/*!
 * @brief function to initialize the power IC debouncing thread
 *
 * This function does the initialization required to start the power IC
 * debouncing thread.  During the initialization, an event for each of the
 * enabled events from the event table is added into the queue to be
 * processed by the debouncing thread.  This is done to allow for power-up
 * determination of the state of each of the events (headset status, etc.)
 * rather than just having to wait for an interrupt from the power IC before
 * the state of the event can be determined.
 */
void power_ic_debounce_init (void)
{
    int i = 0;

    /* Loop through the set of registered events */
    while (debounce_events[i].event < POWER_IC_EVENT_NUM_EVENTS)
    {
        /* Reset the variables for this event */
        debounce_events[i].poll_current = 0;
        debounce_events[i].previous_status = -1;

        /* Initialize the timer data for this entry */
        init_timer(&(debounce_events[i].timer));
        debounce_events[i].timer.data = i;
        debounce_events[i].timer.function = debounce_timer_handler;

        /* Register an event handler */
        power_ic_event_subscribe (debounce_events[i].event, debounce_interrupt_handler);

        /* If the entry is enabled, force an event for this entry, just to get things started */
        if (debounce_events[i].enabled)
        {
            add_event(debounce_events[i].event);
        }

        /* Move to the next event in the table */
        i++;
    }

    /* Start the debouncer kernel thread */
    kernel_thread (debounce_thread, NULL, 0);
}
