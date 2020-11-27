/*
 * Copyright (C) 2006 - 2007 Motorola, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Motorola 2007-Mar-28 - Switch power source for phone power accessories
 * Motorola 2007-Feb-08 - Add support for China charger 
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Nov-07 - Fix Klocwork Warnings
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Sep-27 - Add charger connection status function
 * Motorola 2006-Jul-06 - Implement USB detection light driver
 */

/*!
 * @file usb_detection.c
 *
 * @ingroup poweric_emu
 *
 * @brief EMU USB detection light driver.
 *
 * This file defines the functions that are needed to assist the
 * USB detection light driver.
 *
 * This driver is used to detect USB cable insertion/removal at power up when User Space
 * is not yet available and in kernel stand alone mode.
 *
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include <linux/moto_accy.h>

#include "emu_glue_utils.h"
#include "emu_atlas.h"
#include "../core/thread.h"
#include "../core/os_independent.h"


/******************************************************************************
* Constants
******************************************************************************/
/*! driver states */ 
typedef enum
{
    USB_DISCONNECTED_STATE,
    USB_DEBOUNCING_STATE,
    USB_POLLING_REMOVAL_STATE
} USB_DETECTION_STATE_T;

/*! Structure defining format of state table entries */
typedef struct
{
    /*! state handler */
    USB_DETECTION_STATE_T (*handler)(void);

    /*! polling time */
    int polling_time;
} STATE_TABLE_ENTRY_T;

#define USB_DEBOUNCED 3
#define USB2V0_MASK   0x0020000
#define IDF_MASK      0x0080000
#define IDG_MASK      0x0100000
#define SE1_MASK      0x0200000

#define VBUSEN_MASK   0x0000020

/******************************************************************************
* Local Variables
******************************************************************************/
/*! Wait event flag */
static bool usb_detection_event_flag = false;

/*! Wait queue for interrupt/thread communication */
DECLARE_WAIT_QUEUE_HEAD(usb_detection_thread_wq);

/*! Holds the current state */
static USB_DETECTION_STATE_T state = USB_DEBOUNCING_STATE;

/*! Holds the cable attached */
static MOTO_ACCY_TYPE_T cable_attached = MOTO_ACCY_TYPE_NONE;

/*! debounce variables */
static int prev_vbus_state = 0;
static int debounce_counter = 0;

/******************************************************************************
* Global variables
******************************************************************************/

/******************************************************************************
* Local functions
******************************************************************************/

/*! 
 * @brief Configure the EMU hardware for detection 
 *
 * Configure the EMU hardware for kernel detection
 */
static void configure_emu_for_detection(void)
{
    /* Disable reverse mode */
    EMU_SET_REVERSE_MODE(0);
    
    /* Make sure that we are using the ID pull up */
    EMU_SET_ID_PULL_UP_CONTROL(0);

    /* Enable the 150K D+ pull up */
    EMU_SET_DPLUS_150K_PULL_UP(1);

    /* Disable the ID pull down */
    EMU_SET_ID_PULL_DOWN(0);

    /* Set the conn mode */
    EMU_SET_EMU_CONN_MODE(POWER_IC_EMU_CONN_MODE_USB);

    /* Reset headset send/end key registers.*/
    EMU_SET_HS_SEND_END_REGS(0);

    /* Disable the headset pull up */
    EMU_SET_HEADSET_PULL_UP(0);

    /* Setup the transceiver for detection */
    emu_configure_usb_xcvr(EMU_XCVR_OFF);

    /* Disable the USB pull up */
    power_ic_periph_set_usb_pull_up(0);
}

/*! 
 * @brief Configure Atlas for the USB cable 
 *
 * Configure Atlas power IC for a USB cable.
 */
static void configure_usb_cable(void)
{
    /* disable the ID pull down */
    EMU_SET_ID_PULL_DOWN(0);

    /* disable the 150K D+ pull up */
    EMU_SET_DPLUS_150K_PULL_UP(0);

    /* disable the 5K VBUS pull down */
    EMU_SET_VBUS_5K_PULL_DOWN(0);

    /* Set the USB transceiver to USB HOST */
    emu_configure_usb_xcvr(EMU_XCVR_USB_HOST);

    /* Set the CONN mode to USB */
    EMU_SET_EMU_CONN_MODE(POWER_IC_EMU_CONN_MODE_USB);
}


/*!
 * @brief handler for the USB_DEBOUNCING_STATE state  
 *
 * This function debounce the VBUS state. 
 * Once VBUS is debounced the function checks if an insertion or removal is detected.
 * 
 * Insertion :
 *  - Detect if an USB or factory cable is atatched
 *  - Configure the Atlas, update the accessory driver
 *  - Go to the next state (Polling for USB cable, Stopped for factory cable)   
 *
 * For a removal
 *  - remove the accessory if one has been inserted before
 *  - reset atlas
 *  - go in wait for interrupt state
 */
static USB_DETECTION_STATE_T usb_detection_debouncing_handler(void)
{   
    int sense;
    int vbus_state;

    power_ic_read_reg(POWER_IC_REG_ATLAS_INT_SENSE_0, &sense);

    vbus_state = ((sense & USB2V0_MASK) == USB2V0_MASK);
        
    /* Check if VBUS is debounced */
    if (vbus_state == prev_vbus_state)
    {        
        debounce_counter++;
    }
    else
    {
        debounce_counter = 0;
        prev_vbus_state = vbus_state;
    }

    tracemsg(_k_d("USB DET: Debouncing state %x prev state %x counter %d"), vbus_state, prev_vbus_state, debounce_counter);

    /* vbus_state is debounced */
    if (debounce_counter >= USB_DEBOUNCED)
    {
        debounce_counter = 0;
        
        /* Insertion */
        if (vbus_state)
        {
            /* Check if we have a factory cable */
            if ((sense & (IDF_MASK | IDG_MASK | SE1_MASK)) == (IDF_MASK | IDG_MASK))
            {               
                /* hold the cable detected */
                cable_attached = MOTO_ACCY_TYPE_CABLE_FACTORY;

                /* configure Atlas */
                configure_usb_cable();

                /* Update accessory driver */
                moto_accy_notify_insert(MOTO_ACCY_TYPE_CABLE_FACTORY);

                tracemsg(_k_d("USB DET: attach MOTO_ACCY_TYPE_CABLE_FACTORY %x"),cable_attached);

                /* Do not umask the VBUS INT and 
                   Go in USB_DISCONNECTED_STATE state as we do not poll the factory cable removal */
                return USB_DISCONNECTED_STATE;
                
            }
            /* Check if we have a USB cable */
            else if ((sense & (IDF_MASK | IDG_MASK | SE1_MASK)) == IDF_MASK)
            {
                /* hold the cable detected */
                cable_attached = MOTO_ACCY_TYPE_CABLE_USB;

                /* configure Atlas */
                configure_usb_cable();

                /* Update accessory driver */
                moto_accy_notify_insert(MOTO_ACCY_TYPE_CABLE_USB);

                tracemsg(_k_d("USB DET: attach MOTO_ACCY_TYPE_CABLE_USB %x"),cable_attached);
                   
                /* Go in USB_POLLING_REMOVAL_STATE state */
                return USB_POLLING_REMOVAL_STATE;
            }
            /* Unsupported cable */
            else
            {
                /* Unmask and clear VBUS */
                power_ic_event_clear(EMU_INT_VBUS);
                power_ic_event_unmask(EMU_INT_VBUS);

                /* Go in DISCONNECTED state and wait for other VBUS INT */
                return USB_DISCONNECTED_STATE;
            }            
        }
        /* Removal */
        else
        {
            /* Reset the Atlas */
            configure_emu_for_detection();

            if (cable_attached != MOTO_ACCY_TYPE_NONE)
            {
                tracemsg(_k_d("USB DET: remove %x \n"),cable_attached);
                
                /* Update accessory driver */
                moto_accy_notify_remove(cable_attached);

                cable_attached = MOTO_ACCY_TYPE_NONE; 
            }

            /* Unmask and clear VBUS */
            power_ic_event_clear(EMU_INT_VBUS);
            power_ic_event_unmask(EMU_INT_VBUS);

            /* Go in DISCONNECTED state */
            return USB_DISCONNECTED_STATE;
        }
    }
    
    /* Continue to debouncing */
    return USB_DEBOUNCING_STATE;
}

/*!
 * @brief handler for the USB_POLLING_REMOVAL_STATE state  
 *
 * This function poll the VBUS state to detect USB cable removal.
 *
 */
static USB_DETECTION_STATE_T usb_detection_removal_polling_handler(void)
{
    int sense;
    
    power_ic_read_reg(POWER_IC_REG_ATLAS_INT_SENSE_0, &sense);
    
    tracemsg(_k_d("USB DET: polling removal"));

    if ((sense & USB2V0_MASK) == USB2V0_MASK)
    {
        /* Continue to poll removal */
        return USB_POLLING_REMOVAL_STATE;
    }
    else
    {
        /* Removal detected . Start debouncing */
        return USB_DEBOUNCING_STATE;
    }
}

/******************************************************************************
* State table
******************************************************************************/

/*! USB detection state table */
static const STATE_TABLE_ENTRY_T state_table[] = 
{
    /* State                          handler                                polling time*/
    /* USB_DISCONNECTED_STATE     */ {NULL,                                  0},
    /* USB_DEBOUNCING_STATE       */ {usb_detection_debouncing_handler,      (100 * HZ) / 1000},
    /* USB_POLLING_REMOVAL_STATE  */ {usb_detection_removal_polling_handler, (500 * HZ) / 1000},
};


/******************************************************************************
* Global Functions
******************************************************************************/
/*!
 * @brief Event handler for the power IC VBUS interrupt on SCMA11-Bute platforms
 * for the USB detection light driver.
 *
 * This function is the event handler for the power IC and ID interrupts for the 
 * USB detection light driver.
 *
 * The handler sets the state machine state to USB DEBOUCING and wakes up the thread.
 *
 * @param event parameter indicating which even occurred
 *
 * @return 1, indicating that the event has been handled
 */
int usb_detection_int_handler(POWER_IC_EVENT_T event)
{
    tracemsg(_k_d("USB DET: usb_detection_int_handler: interrupt received"));
    
    /* Set the flag indicating that an event occurred */
    usb_detection_event_flag = true;

    /* Set the state */
    state = USB_DEBOUNCING_STATE;

    /* Mask the interrupt during the debouncing */
    power_ic_event_mask(EMU_INT_VBUS);

    /* Wake up the thread if it is sleeping */
    wake_up(&usb_detection_thread_wq);

    /* Indicate that the event has been handled */
    return 1;
}

/*!
 * @brief USB detection light driver state machine thread
 *
 * This function implements the main USB detection state machine
 * thread loop.  The state machine itself is implemented as a simple loop.
 * In each iteration of the loop, a state handler function is called.
 * 
 * Following the return of the handler function two action can be taken:
 * - wait for an interrupt
 * - wait for a timeout time.
 *
 * @param unused An unused parameter
 *
 * @return 0.

 */
int usb_detection_state_machine_thread(void* unused)
{
    /* Usual thread setup. */
    thread_common_setup("kusbdetd");

    /* Take the EMU thread priority */
    if(thread_set_realtime_priority(THREAD_PRIORITY_EMU) != 0)
    {
        tracemsg(_a("USB DET: thread - error setting thread priority."));
    }

    /* Configure the EMU hardware */
    configure_emu_for_detection();
    
    /*
     * Loop unless an abort signal is received or the EMU proc is opened.  
     * All signals, but the abort signals are
     * masked off in the common setup.  As a result only abort signals can be pending.
     */
    while((emu_proc_opened == false) && (!signal_pending(current)))
    {
        /* Call the state handler */
        state = state_table[state].handler();

        /* Wait for interrupts */
        if (state ==  USB_DISCONNECTED_STATE)
        {
            /* Wait for the next event if we haven't already received it */
            wait_event_interruptible(usb_detection_thread_wq, usb_detection_event_flag);
            
            /* Reset the event flag */
            usb_detection_event_flag = false;

        }
        /* Wait for timeout */
        else
        {
            /* Sleep for polling interval, or until an abort signal is received.  */
            wait_event_interruptible_timeout(usb_detection_thread_wq, usb_detection_event_flag,  state_table[state].polling_time);

            /* Reset the event flag */
            usb_detection_event_flag = false;
        }

    }

    tracemsg(_k_d("USB DET: thread stopped "));
    
    /* If the proc is opened, set the state to USB_DISCONNECTED_STATE */
    if (emu_proc_opened == true)
    {
        state = USB_DISCONNECTED_STATE;
    }
    return 0;
}

/*!
 * @brief Checks whether a valid charger is connected.
 *
 * This function will check the EMU related Power IC sense bits to
 * determine if a charger is inserted.  This is done instead of just
 * checking what accessories are known to be attached because this
 * function is called on power down and a charger may be inserted or
 * removed after user space is shut down.
 *
 * @return   0 when no charger attached<BR>
 *           1 if charger is attached.<BR>
 *           -1 on error.
 */
int power_ic_is_charger_attached(void)
{
    int ret;
    int sense0;
    int charger1;

    /* Store the current setting of the accessory power supply */
    ret = power_ic_read_reg(POWER_IC_REG_ATLAS_CHARGE_USB_1, &charger1);

    configure_emu_for_detection();

    /* Allow the EMU hardware to settle */
    mdelay(5);

    ret |= power_ic_read_reg(POWER_IC_REG_ATLAS_INT_SENSE_0, &sense0);

    if (ret == 0)
    {
        /* Check if a charger is connected. Accessory power is disabled
           in this function but it takes 200mS for the VBUS voltage to drop
           after disabling the SPI bit. This is why the accessory power is
           checked. */
        if (((sense0 & (USB2V0_MASK | SE1_MASK)) == (USB2V0_MASK | SE1_MASK)) &&
            ((charger1 & VBUSEN_MASK) == 0))
        {
            tracemsg(_k_d("USB DET: is_charger_attached: YES"));
            return 1;
        }
    }
    else
    {
        printk("USB DET: is_charger_attached: Error reading sense register!\n");
        return -1;
    }

    /* Indicate that no charger is attached */
    tracemsg(_k_d("USB DET: is_charger_attached: NO"));
    return 0;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
EXPORT_SYMBOL(power_ic_is_charger_attached);
#endif
