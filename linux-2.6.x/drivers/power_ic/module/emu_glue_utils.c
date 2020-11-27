/*
 * Copyright (C) 2006 - 2008 Motorola, Inc.
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
 * Motorola 2008-May-05 - Tuning pops and clicks for 3.5mm to EMU adapter
 * Motorola 2007-Jun-21 - Adjusting the hardware settle times for the
 *                        EMU headset
 * Motorola 2007-Jun-21 - Adding I/O control to get the state of
 *                        the power path.
 * Motorola 2007-Mar-22 - Switch power source for phone power accessories
 * Motorola 2007-Mar-15 - Check if the accessory needs to be configured
 *                        after unlocking hardware
 * Motorola 2007-Feb-07 - Add support for cradle charger
 * Motorola 2007-Jan-25 - Add support for power management
 * Motorola 2007-Jan-25 - Adding I/O control for FET configuration 
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Oct-12 - Fix Send End Key issue
 * Motorola 2006-Sep-06 - Update usb xcvr table setting
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-06 - Implement USB detection light driver
 * Motorola 2006-May-06 - Implement EMU Common Algorithm
 */

/*!
 * @file emu_glue_utils.c
 *
 * @ingroup poweric_emu
 *
 * @brief EMU GLUE utility functions
 *
 * This file defines the functions that are needed to assist the
 * EMU algorithm in User Space.
 *
 */

#include <stdbool.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include <linux/moto_accy.h>
#include <linux/poll.h>
#include <linux/delay.h>

#include "../core/gpio.h"
#include "audio.h"
#include "emu_glue_utils.h"
#include "emu_atlas.h"

#include "../core/event.h"
#include "../core/gpio.h"
#include "../core/os_independent.h"


/******************************************************************************
* Constants
******************************************************************************/
/*!
 * @brief Clear interrupt 1
 */
#define CLEAR_ACCY_INTERRUPT_1  POWER_IC_EVENT_ATLAS_USBI | POWER_IC_EVENT_ATLAS_IDI | POWER_IC_EVENT_ATLAS_SE1I

/*!
 * @name Sense Bits 
 *
 * Sense value reads indicate if interrupt status flags need to be cleared
 */
/*@{*/
#define EMU_SENSE_REVERSE_MODE          0x00000001
#define EMU_SENSE_CRADLE_DETECT         0x00000002
#define EMU_SENSE_CHGCURR               0x00000800
#define EMU_SENSE_DPLUS                 0x00008000
#define EMU_SENSE_VBUS_4V4              0x00010000
#define EMU_SENSE_VBUS_2V0              0x00020000
#define EMU_SENSE_VBUS_0V8              0x00040000
#define EMU_SENSE_ID_FLOAT              0x00080000
#define EMU_SENSE_ID_GROUND             0x00100000
#define EMU_SENSE_SE1                   0x00200000
#define EMU_SENSE_DMINUS                0x00800000
/*@}*/

/******************************************************************************
* Local Variables
******************************************************************************/
/*!
 * @brief Power IC poll wait queue
 */
DECLARE_WAIT_QUEUE_HEAD(emu_proc_event_int_wait_queue);
/*!
 * @brief EMU proc poll flag
 */
static bool emu_proc_event_int_flag;
/*!
 * @brief Variable containing the EMU proc events that occurs
 */
static char emu_proc_event_int;
/*!
 * @brief Mutex use to protect the emu_proc_event_int variable
 */
DECLARE_MUTEX(emu_proc_event_int_mutex);
/*!
 * @brief containing the emu proc open state
 */
/*!
 * @brief Mutex use to protect the emu_proc_opened variable
 */
DECLARE_MUTEX(emu_proc_opened_mutex);

/******************************************************************************
* Global variables
******************************************************************************/
/* HW locked variable */
bool power_ic_emu_hw_locked;

/* containing the emu proc open state */
bool emu_proc_opened;

/******************************************************************************
* Local functions
******************************************************************************/
/*!
 * @brief Event handler for the power IC VBUS, SE1, and ID interrupts on SCMA11-Bute platforms
 *
 * This function is the event handler for the power IC VBUS,SE1 and ID interrupts.
 * When the events occur, the job of the interrupt handler is to ensure that
 * the event is reported to the User Space using the poll system call.  
 * This is accomplished through the following mechanism:
 *
 *   - Update the power_ic_event_int variable containing the type of events that
 *     occur before the User Space read it.
 *
 *   - Wake up the power_ic_poll function by updating the power_ic_event_int and
 *     waking up the power_ic_event_int_wait_queue wait queue. 
 *
 * @param event parameter indicating which even occurred
 *
 * @return 1, indicating that the event has been handled
 */
static int emu_glue_int_handler(POWER_IC_EVENT_T event)
{
    tracemsg(_k_d("EMU: emu_glue_int_handler: interrupt received for 0x%x event"),event);

    /* take mutex */
    down(&emu_proc_event_int_mutex);
    
    /* Update power_ic_event_int variable */
    switch (event)
    {
        case POWER_IC_EVENT_ATLAS_USBI: 
            emu_proc_event_int |= POWER_IC_EVENT_INT_VBUS;
            break; 
        case POWER_IC_EVENT_ATLAS_IDI:
            emu_proc_event_int |= POWER_IC_EVENT_INT_ID;
            break; 
        case POWER_IC_EVENT_ATLAS_SE1I:
            emu_proc_event_int |= POWER_IC_EVENT_INT_SE1;
            break;
        default:
            break;
    }
    
    /* Release mutex and wake up poll function */
    emu_proc_event_int_flag = true;
    up(&emu_proc_event_int_mutex);
    wake_up_interruptible(&emu_proc_event_int_wait_queue); 
    return 1;
}

/*!
 * @brief emu_glue_read_sense
 *
 * This function returns the EMU state information.  
 * The parameter to the function indicates if the interrupt status flags should be cleared 
 * after reading the value of the sense bits.  If the parameter is non zero, the interrupt 
 * status flags for the VBUS, ID, and SE1 interrupts should be cleared.  
 * If zero, the interrupt status flags should be left unchanged
 *
 * @param clear_int_flags see description
 *
 * @return unsigned long int representing the current state of the EMU state information
 */
unsigned long emu_glue_read_sense(unsigned long clear_int_flags)
{
    int value_read;
    unsigned long emu_glue_sense_read = 0;
   
    /* Read Atlas Sense 0 register bits */
    power_ic_get_reg_value(POWER_IC_REG_ATLAS_INT_SENSE_0, 0,
                           &(value_read), 24);

    /* Get all EMU sense bit states from the power ic */
    emu_glue_sense_read =  value_read & (EMU_SENSE_CHGCURR | EMU_SENSE_DPLUS | EMU_SENSE_DMINUS | \
                                         EMU_SENSE_VBUS_4V4 | EMU_SENSE_VBUS_2V0 | EMU_SENSE_VBUS_0V8 | \
                                         EMU_SENSE_ID_FLOAT | EMU_SENSE_ID_GROUND | EMU_SENSE_SE1 );

    /* OR in the state of the cradle detect line */
    emu_glue_sense_read |= (EMU_GET_CRADLE_DETECT() ? EMU_SENSE_CRADLE_DETECT : 0);
    
    /* Get accessory power setting */
    power_ic_get_reg_value(POWER_IC_REG_ATLAS_CHARGE_USB_1, 5,  &(value_read), 1);
    emu_glue_sense_read |=  value_read;

    /* Clear interrupt flags if requested */
    if (clear_int_flags != 0)
    {
        power_ic_set_reg_value(POWER_IC_REG_ATLAS_INT_STAT_0, 0,
                               CLEAR_ACCY_INTERRUPT_1, 24);
    }
    
    return emu_glue_sense_read;
}

/*!
 * @brief emu_glue_set_transceiver_params
 *
 * Thsi function set the EMU trasnceiver parameteres.
 *
 * @param trans_params
 *
 */
void emu_glue_set_transceiver_params(POWER_IC_EMU_GLUE_TRANSCEIVER_PARAMS_T trans_params)
{
    int vusb_settings  = (trans_params.vusb_input_source | \
                         ((trans_params.vusb_voltage == 3300 ? 1 : 0) << 2) | \
                         ((trans_params.vusb_en == 0 ? 0 : 1) << 3));
        
    /* Set VUSB settings in Charger USB 1 register */
    power_ic_set_reg_value(POWER_IC_REG_ATLAS_CHARGE_USB_1, 0, vusb_settings, 4);
    
    /* Set the state of the transceiver in USB 0 regsiter */
    power_ic_set_reg_value(POWER_IC_REG_ATLAS_USB_0, 12, (trans_params.xcvr_en == 0 ? 0 : 1), 1);   
}

/*!
 * @brief the open() handler for EMU the power IC proc device node - for SCMA11 and BUTE platform only.
 *
 * This function implements the open() system call on the EMU power IC proc device.  Currently,
 * this function just set the opened state .
 *
 * @param        inode       inode pointer
 * @param        file        file pointer
 *
 * @return 0
 */

static int emu_proc_open(struct inode *inode, struct file *file)
{
    /* take mutex */
    if(down_interruptible(&emu_proc_opened_mutex))
    {
        tracemsg(_k_d("process received signal while waiting for power ic opened mutex. Exiting."));
        return -EINTR;
    }

    /* Check if the proc is not already open */
    if (emu_proc_opened == true)
    {
        tracemsg(_k_d("/proc/emu control already opened.\n"));

         /* Release mutex */
        up(&emu_proc_opened_mutex);
        return -EBUSY;
    }
    /* Set the opened state */
    emu_proc_opened = true;

    /* MASK VBUS INT */
    power_ic_event_mask(EMU_INT_VBUS);

    /* Unsubsribe usb detection light driver handler on VBUS */
    power_ic_event_unsubscribe(EMU_INT_VBUS,usb_detection_int_handler);

    /* Subscribe to the EMU interrupt events */
    power_ic_event_subscribe(EMU_INT_VBUS, emu_glue_int_handler);
    power_ic_event_subscribe(EMU_INT_ID, emu_glue_int_handler);
    power_ic_event_subscribe(EMU_INT_SE1, emu_glue_int_handler);
    
    /* Release mutex */
    up(&emu_proc_opened_mutex);
    
    return 0;
}

/*!
 * @brief the close() handler for the power IC device node - for SCMA11 and BUTE platform only.
 *
 * This function implements the close() system call on the EMU power IC proc device. Currently,
 * this function just set the opened state .  
 *
 * @param        inode       inode pointer
 * @param        file        file pointer
 *
 * @return 0
 */

static int emu_proc_release(struct inode *inode, struct file *file)
{
    /* take mutex */
    if(down_interruptible(&emu_proc_opened_mutex))
    {
        tracemsg(_k_d("process received signal while waiting for power ic opened mutex. Exiting."));
        return -EINTR;
    }

    /* Reset the opened state */
    emu_proc_opened = false;

    /* Release mutex */
    up(&emu_proc_opened_mutex);

    return 0;
}


/*!
 * @brief the poll() handler for the EMU power IC proc device node - for SCMA11 and BUTE platform only.
 *
 * This function implements the poll() system call on the power IC device.
 *
 * @param        file        file pointer
 * @param        wait        poll table 
 *
 * @return 0
 */
unsigned int emu_proc_poll(struct file *file, poll_table *wait)
{
    unsigned int retval = 0;

    /* Add our wait queue to the poll table */
    poll_wait(file, &emu_proc_event_int_wait_queue, wait);

    /* If there are changes, indicate that there is data available to read */
    if (emu_proc_event_int_flag)
    {
       retval = (POLLIN | POLLRDNORM);
    }

    return retval;
}


/*!
 * @brief the read() handler for the EMU power IC proc device node  - for SCMA11 and BUTE platform only.
 *
 * This function implements the read() system call on the power IC device.
 *
 * @param        file        file pointer
 * @param        buf         data
 * @param        count       data size
 * @param        ppos        position unused.
 * 
 *
 * @return 0 if succesful
 */
ssize_t emu_proc_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{   
    int retval = 0;
    /* If there isn't space in the buffer for the results, return a failure. Do not 
     * consider the process complete if this happens as the results are still available. */
    if(count < sizeof(emu_proc_event_int))
    {
        return -EFBIG;
    }
    
    /* take mutex */
    if(down_interruptible(&emu_proc_event_int_mutex))
    {
        tracemsg(_k_d("process received signal while waiting for power ic int event  mutex. Exiting."));
        return -EINTR;
    }
        
    /* Copy the results back to user-space. */
    if( !(copy_to_user((int *)buf, (void *)&emu_proc_event_int, sizeof(emu_proc_event_int))) )
    {
        /* Data was copied successfully. */
        retval = sizeof(emu_proc_event_int);
    }
    else
    {
        /* release mutex */
        up(&emu_proc_event_int_mutex);
        return -EFAULT;
    }
    
    /* Clear variable and release mutex */
    emu_proc_event_int_flag = false;
    emu_proc_event_int = 0;
    up(&emu_proc_event_int_mutex);

    return retval;
}

/*!
 * @brief the ioctl() handler for the EMU power IC proc device node.
 *
 * This function implements the ioctl() system call for the EMU power IC proc device node.
 * Based on the provided command, the function will handle the command.
 *
 * @param        inode       inode pointer
 * @param        file        file pointer
 * @param        cmd         the ioctl() command
 * @param        arg         the ioctl() argument
 *
 * @return 0 if successful
 */

static int emu_proc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int ret_val = 0;
    
    POWER_IC_EMU_GLUE_READ_SENSE_T read_sense; 
    POWER_IC_EMU_GLUE_TRANSCEIVER_PARAMS_T trans_params;
    POWER_IC_FET_CONTROL_T fet_ctrl;

    /* prev lockout status */
    static bool prev_emu_hw_lockout_state = false;

    /* what devices were connected before lockout */
    static MOTO_ACCY_MASK_T connected_device_before_lock = 0;
    MOTO_ACCY_MASK_T connected_device;
    
    /* Get the actual command from the ioctl request. */
    unsigned int cmd_num = _IOC_NR(cmd);
    
    if ((cmd_num >= POWER_IC_IOC_CMD_EMU_GLUE_BASE) && (cmd_num <= POWER_IC_IOC_CMD_EMU_GLUE_LAST_CMD))
    { 
        tracemsg(_k_d("EMU GLUE control ioctl(), request 0x%X (cmd 0x%X)"),(int) cmd, _IOC_NR(cmd));

        /* Handle the request. */
        switch(cmd)
        {
            case POWER_IC_IOCTL_CMD_EMU_GLUE_READ_SENSE:
                /* Fetch the data passed from user space. */
                if(copy_from_user((void *)&read_sense, (void *)arg, sizeof(read_sense)) != 0)
                {
                    tracemsg(_k_d("error copying data from user space."));
                    ret_val = -EFAULT;
                }  
                else
                {
                    /* Read the sense and clear the interrupt if requested */
                    read_sense.sense = emu_glue_read_sense(read_sense.clear_int_flags);

                    /* Only the sense value read needs to be sent back to the caller. */
                    if(put_user(read_sense.sense,&(((POWER_IC_EMU_GLUE_READ_SENSE_T *)arg)->sense)) != 0)
                    {
                        tracemsg(_k_d("error copying read bits to user space."));
                        ret_val = -EFAULT;
                    }
                }
                break;
                
            case POWER_IC_IOCTL_CMD_EMU_GLUE_LOCKOUT_CHANGES:
                power_ic_emu_hw_locked = (bool)(arg == 0 ? 0 : 1);
                 
                if (power_ic_emu_hw_locked)
                {
                    /* Make sure the hardware is not already locked */
                    if (prev_emu_hw_lockout_state == false)
                    {
                        /* If the EMU hardware needs to be locked, keep track of
                           the currently connected accessory */
                        connected_device_before_lock = moto_accy_get_all_devices();
                    }
                }
                else
                {                   
                    /* If the EMU hardware was previously locked but is now unlocked the accessory may need
                       to be reconfigured. */
                    if (prev_emu_hw_lockout_state == true)
                    {
                        connected_device = moto_accy_get_all_devices();
                        
                        if (connected_device_before_lock == connected_device)
                        {
                            if ((ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_HEADSET_EMU_MONO)) ||
                                (ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_HEADSET_EMU_STEREO)))
                            {
                                audio_config.id_pull_down = 0;
                                audio_config.conn_mode = 0;
                                emu_util_set_emu_headset_mode(audio_config.headset_mode);
                            }
                            else if ((ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_CARKIT_MID)) ||
                                     (ACCY_BITMASK_ISSET(connected_device, MOTO_ACCY_TYPE_CARKIT_FAST)))
                            {
                                audio_config.headset_mode = MOTO_ACCY_HEADSET_MODE_NONE;
                                EMU_SET_EMU_CONN_MODE(audio_config.conn_mode);
                                EMU_SET_ID_PULL_DOWN(audio_config.id_pull_down);
                            }
                            else
                            {
                                audio_config.headset_mode = MOTO_ACCY_HEADSET_MODE_NONE;
                                audio_config.conn_mode = POWER_IC_EMU_CONN_MODE_USB;
                                audio_config.id_pull_down = 0;  
                            }
                        }
                        else
                        {
                            audio_config.headset_mode = MOTO_ACCY_HEADSET_MODE_NONE;
                            audio_config.conn_mode = POWER_IC_EMU_CONN_MODE_USB;
                            audio_config.id_pull_down = 0;  
                        }
                    }
                }

                prev_emu_hw_lockout_state = power_ic_emu_hw_locked;
                break;

            case POWER_IC_IOCTL_CMD_EMU_GLUE_GET_FET_CONTROL:
                if ((power_ic_get_reg_value(POWER_IC_REG_ATLAS_CHARGER_0, 10, (int *)&(fet_ctrl), 2)) != 0)
                {
                    ret_val = -EIO;
                }
                /* Only the sense value read needs to be sent back to the caller. */
                else if(put_user(fet_ctrl, (((int *)arg))) != 0)
                {
                    tracemsg(_k_d("error copying read bits to user space."));
                    ret_val = -EFAULT;
                }
                break;
                   
            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_FET_CONTROL:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_CHARGER_0, 10,
                                       arg, 2);
                break;
                
            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_VBUS_5K_PD:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_CHARGER_0, 19,
                                       (arg == 0 ? 0 : 1), 1);
                break;
         
            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_VBUS_70K_PD:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_USB_0, 6,
                                       (arg == 0 ? 0 : 1), 1);
                               
                break;

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_REVERSE_MODE:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_CHARGE_USB_1, 5,
                                   (arg == 0 ? 0 : 1), 1);
                break;

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_ID_PU:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_USB_0, 22,
                                       (arg == 0 ? 0 : 1), 1);
                break;            
         
            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_ID_PD:
                 power_ic_set_reg_value(POWER_IC_REG_ATLAS_USB_0, 20,
                                       (arg == 0 ? 0 : 1), 1);
                 break;

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_ID_STEREO_PU:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_CHARGE_USB_1, 8,
                                       (arg == 0 ? 0 : 1), 1);
                break;  

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_CONN_MODE:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_USB_0, 14,
                                       arg, 3);
                break;  

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_DPLUS_150K_PU:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_USB_0, 5,
                                       (arg == 0 ? 0 : 1), 1);            
                break;

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_DPLUS_1_5K_PU:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_USB_0, 2,
                                       (arg == 0 ? 0 : 1), 1); 
                break;
  
            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_LOW_SPEED_MODE:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_USB_0, 0,
                                       (arg == 0 ? 0 : 1), 1);    
                break;

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_USB_SUSPEND:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_USB_0, 1,
                                       (arg == 0 ? 0 : 1), 1);    
                break;

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_TRANSCEIVER_PARAMS:
                /* Fetch the data passed from user space. */
                if(copy_from_user((void *)&trans_params, (void *)arg, sizeof(trans_params)) != 0)
                {
                    tracemsg(_k_d("error copying data from user space."));
                    ret_val = -EFAULT;
                }
                else
                {
                    /* Call local function */
                    emu_glue_set_transceiver_params(trans_params);
                }
                break;

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_ID_INT_MASK:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_INT_MASK_0, 19,
                                       arg, 1);    
                break;  

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_VBUS_INT_MASK:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_INT_MASK_0, 16,
                                       arg, 1);    
                break; 

            case POWER_IC_IOCTL_CMD_EMU_GLUE_SET_SE1_INT_MASK:
                power_ic_set_reg_value(POWER_IC_REG_ATLAS_INT_MASK_0, 21,
                                       arg, 1);    
                break;

            default: /* This shouldn't be able to happen, but just in case... */
                tracemsg(_k_d("=> 0x%X unsupported emu proc ioctl command"), (int) cmd);
                ret_val = -ENOTTY;
                break;
        }
    }
    else /* The driver doesn't support this request. */
    {
        tracemsg(_k_d("0x%X unsupported ioctl command"), (int) cmd);
        ret_val = -ENOTTY;
    }
        
    return ret_val;
}

/*! This structure defines the file operations for the EMU proc device */
static struct file_operations emu_proc_fops =
{
    .open = emu_proc_open,
    .ioctl = emu_proc_ioctl,
    .release = emu_proc_release,
    .poll = emu_proc_poll,
    .read = emu_proc_read,
};


/******************************************************************************
* Global Functions
******************************************************************************/

/*!
 * @brief configure_usb_xcvr
 *
 * This utility function is used to configure the transceiver, which includes
 * enabling/disabling the transceiver, setting the VUSB input source, setting the
 * VUSB output voltage and setting the state of the VUSB regulator
 *
 * @param state - what state the transceiver should be set to
 */
void emu_configure_usb_xcvr(EMU_XCVR_T state)
{
    static const struct
    {
        bool usbsuspend;
        bool usbxcvren;
        bool vusben;
        EMU_VREG_IN_T input_source;
        EMU_VREG_OUT_T voltage;
    } vusb_states[EMU_XCVR__NUM] =
    {
        /* EMU_XCVR_OFF */          { true,  false, true,  EMU_VREG_IN_VINBUS, EMU_VREG_OUT_3_3V   },
        
        /* EMU_XCVR_PPD_DETECT */   { true,  false, true,  EMU_VREG_IN_VINBUS, EMU_VREG_OUT_3_3V   },

        /* EMU_XCVR_SPD_DETECT */   { true,  false, true,  EMU_VREG_IN_VBUS,   EMU_VREG_OUT_3_3V   },

        /* EMU_XCVR_PPD */          { true,  false, true,  EMU_VREG_IN_VINBUS, EMU_VREG_OUT_3_3V   },

        /* EMU_XCVR_PPD_AUDIO */    { true,  true,  true,  EMU_VREG_IN_VINBUS, EMU_VREG_OUT_3_3V   },

        /* EMU_XCVR_SPD */          { true,  false, true,  EMU_VREG_IN_VBUS,   EMU_VREG_OUT_3_3V   },

        /* EMU_XCVR_SPD_AUDIO */    { true,  false, true,  EMU_VREG_IN_VBUS,   EMU_VREG_OUT_3_3V   },

        /* EMU_XCVR_USB_HOST */     { false, true,  true,  EMU_VREG_IN_VINBUS, EMU_VREG_OUT_3_3V   },

        /* EMU_XCVR_FACTORY_MODE */ { false, true,  true,  EMU_VREG_IN_VINBUS, EMU_VREG_OUT_3_3V   },
    };

    /* If the state is out of range, assume the off state */
    if (state >= EMU_XCVR__NUM)
    {
        state = EMU_XCVR_OFF;
    }

    /* Set the USB suspend bit */
    EMU_SET_USB_SUSPEND(vusb_states[state].usbsuspend);
    
    /* Set the VUSB input source */
    EMU_SET_VUSB_INPUT_SOURCE(vusb_states[state].input_source);
    
    /* Set VUSB output voltage */
    EMU_SET_VUSB_OUTPUT_VOLTAGE(vusb_states[state].voltage);
    
    /* Set the state of the VUSB regulator */
    EMU_SET_VUSB_REGULATOR_STATE(vusb_states[state].vusben);
    
    /* Set the state of the transceiver */
    EMU_SET_TRANSCEIVER_STATE(vusb_states[state].usbxcvren);
}


/*!
 * @brief This utility function sets the audio mode in Atlas and stereo emu headset
 *        pull up.
 *
 * @param mode - The headset mode
 *
 */
void emu_util_set_emu_headset_mode(MOTO_ACCY_HEADSET_MODE_T mode)
{
    switch(mode)
    {   
        case MOTO_ACCY_HEADSET_MODE_NONE:
            EMU_SET_REVERSE_MODE(false);

            /* Don't allow power management to suspend the phone while waiting for VBUS to settle. */
            power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] |= POWER_IC_EMU_REV_MODE_SLEEP;
            
            /* mdelay must be used here because the delay cannot be allowed to be longer than 20 ms. */
            mdelay(12);
            
            power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] &= ~(POWER_IC_EMU_REV_MODE_SLEEP);
            
            EMU_SET_EMU_CONN_MODE(POWER_IC_EMU_CONN_MODE_USB);
            EMU_SET_HEADSET_PULL_UP(0);
            /*Following is the work around for the emu headset send/end key*/
            EMU_SET_HS_SEND_END_REGS(1);
            EMU_SET_VUSB_INPUT_SOURCE(EMU_VREG_IN_VINBUS);
            break;
        
        case MOTO_ACCY_HEADSET_MODE_MONO:
            EMU_SET_VUSB_INPUT_SOURCE(EMU_VREG_IN_BPLUS);
            EMU_SET_HS_SEND_END_REGS(0);
            EMU_SET_EMU_CONN_MODE(POWER_IC_EMU_CONN_MODE_MONO_AUDIO);
            EMU_SET_HEADSET_PULL_UP(0);
            
            /* Don't allow power management to suspend the phone while waiting for D- to settle */
            power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] |= POWER_IC_EMU_DMINUS_MONO_SLEEP;
            msleep(10);
            power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] &= ~(POWER_IC_EMU_DMINUS_MONO_SLEEP);
            
            EMU_SET_REVERSE_MODE(true);
            break;
            
        case MOTO_ACCY_HEADSET_MODE_STEREO:
            EMU_SET_VUSB_INPUT_SOURCE(EMU_VREG_IN_BPLUS);
            EMU_SET_HS_SEND_END_REGS(0);
            EMU_SET_EMU_CONN_MODE(POWER_IC_EMU_CONN_MODE_STEREO_AUDIO);
            EMU_SET_HEADSET_PULL_UP(1);
            
            /* Don't allow power management to suspend the phone while waiting for D- to settle */
            power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] |= POWER_IC_EMU_DMINUS_STEREO_SLEEP;
            msleep(10);
            power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] &= ~(POWER_IC_EMU_DMINUS_STEREO_SLEEP);
            
            EMU_SET_REVERSE_MODE(true);
            break;
        
        default:
            tracemsg(_k_d("EMU: Audio Mode %d not supported"), mode);
    }
}


/*!
 * @brief Initializes the EMU Glue utils
 *
 * The function performs the initialization of the EMU glue utils variables and register handler
 * to the Power IC EMU related interrupts
 */
int __init emu_glue_utils_init(void)
{
    struct proc_dir_entry * emu_proc;

    tracemsg(_k_d("EMU: emu_glue_utils_init: initializing kernel EMU glue utils"));

    /* configure D+/D- lines and cradle detect line */
    power_ic_gpio_emu_config();

    /* Create emu proc entry */
    emu_proc = create_proc_entry("emu", S_IRUGO | S_IWUGO, NULL);
    if (emu_proc == NULL)
    {
        tracemsg(_k_d(KERN_ERR "Unable to create EMU proc entry in /proc.\n"));
        return -ENOMEM;
    }

    /* Set the proc fops */
    emu_proc->proc_fops = (struct file_operations *)&emu_proc_fops;
 
    /* Initialize variables */ 
    emu_proc_event_int = 0;
    emu_proc_event_int_flag = false;
    power_ic_emu_hw_locked = false;
    emu_proc_opened = false;
    audio_config.headset_mode = MOTO_ACCY_HEADSET_MODE_NONE;
    audio_config.conn_mode = POWER_IC_EMU_CONN_MODE_USB;
    audio_config.id_pull_down = 0;
    
    /* register the light usb driver int */
    power_ic_event_subscribe(EMU_INT_VBUS,usb_detection_int_handler);

    tracemsg(_k_d("USB DET: Start light driver thread"));

    /* Create USB detection thread */
    kernel_thread(usb_detection_state_machine_thread, NULL, 0);

    /* Init ok */
    return 0;
}
