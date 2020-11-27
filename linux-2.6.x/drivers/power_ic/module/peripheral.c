/*
 * Copyright (C) 2004-2006 Motorola, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  
 * 02111-1307, USA
 * 
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Sep-25 - Removing unused header
 * Motorola 2006-Aug-11 - Add Linear Vibrator support
 * Motorola 2006-Aug-10 - Pico VSIM support
 * Motorola 2006-Aug-10 - Saipan VSIM support
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-11 - Remove vibrator level for SCM-A11
 * Motorola 2006-Jun-01 - Ascension cannot support 3.0V on the vibrator
 * Motorola 2006-May-15 - Conditionally compile cmd_num for tracemsg.
 * Motorola 2005-Sep-13 - Added Camera Support
 * Motorola 2005-Apr-28 - Provided support to change SIM voltage values
 * Motorola 2005-Mar-01 - Add WLAN support
 * Motorola 2004-Dec-06 - Design of the power IC peripheral interface.
 */

/*!
 * @file peripheral.c
 *
 * @brief This is the main file of the power IC peripheral interface.
 *
 * @ingroup poweric_periph
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/string.h>

#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>

#include "../core/os_independent.h"

/*******************************************************************************************
 * CONSTANTS
 ******************************************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/* The vibrator is turned on and off by V_VIB_EN in the AUX_VREG register. */
#define VIBRATOR_ONOFF_REG        POWER_IC_REG_ATLAS_REG_MODE_1
#define VIBRATOR_ONOFF_INDEX      11
#define VIBRATOR_ONOFF_NUM_BITS   1

/* The vibrator level is controlled by V_VIB_[0..1] in the AUX_VREG register. */
#define VIBRATOR_LEVEL_REG        POWER_IC_REG_ATLAS_REG_SET_1
#define VIBRATOR_LEVEL_INDEX      0
#define VIBRATOR_LEVEL_NUM_BITS   2

/* USB Cable Attached */
#define USB_CABLE_ATTACH          POWER_IC_EVENT_ATLAS_USBI

/* The USB pull-up is controlled by DP_1K5_PU in the Connectivity Control register of EOC */
#define USB_PU_REG                POWER_IC_REG_ATLAS_USB_0
#define USB_PU_INDEX              2
#define USB_PU_NUM_BITS           1

#if defined(CONFIG_MACH_SAIPAN) || defined(CONFIG_MACH_PICO)
/* SIM voltage is controlled by VMMC1 for Saipan */
#define SIM_REG_VMMC1_EN             POWER_IC_REG_ATLAS_REG_MODE_1
#define SIM_REG_VMMC1_EN_INDEX       18
#define SIM_REG_VMMC1_EN_NUM_BITS    1

#define SIM_REG_VMMC1_ESIM_EN        POWER_IC_REG_ATLAS_REGEN_ASSIGN
#define SIM_REG_VMMC1_ESIM_INDEX     22
#define SIM_REG_VMMC1_ESIM_NUM_BITS  1

#define SIM_REG_VMMC1_VOLT           POWER_IC_REG_ATLAS_REG_SET_1
#define SIM_REG_VMMC1_VOLT_INDEX     6
#define SIM_REG_VMMC1_VOLT_NUM_BITS  3
#define SIM_REG_VMMC1_1_8V           0x1
#define SIM_REG_VMMC1_3_0V           0x7


#else  /* Not Saipan */

/* SIM voltage is controlled by VSIM in the Regulator Setting 0 on SCM-A11 */
#define SIM_REG_SET_0_REG         POWER_IC_REG_ATLAS_REG_SET_0
#define SIM_REG_SET_0_VSIM_INDEX  14
#define SIM_REG_SET_0_VESIM_INDEX 15
#define SIM_REG_SET_0_NUM_BITS    1
#endif

/* The camera bits in order to turn on and off. */
#define CAMERA_ONOFF_REG        POWER_IC_REG_ATLAS_REG_MODE_1
#define CAMERA_ONOFF_INDEX      6
#define CAMERA_ONOFF_NUM_BITS   1

#endif

/*******************************************************************************************
 * TYPES
 ******************************************************************************************/
 
/*******************************************************************************************
* Local Variables
******************************************************************************************/
 
/*******************************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************************/

/*!
 * @brief This function is called by power IC clients to set the vibrator supply level.
 *
 * @param        level      The level that the vibrator should be set to, from 0 (low) to 3 (high).
 *
 * @return       This function returns 0 if successful.
 */
int power_ic_periph_set_vibrator_level(int level)
{
    return 0;
}

/*!
 * @brief This function is called by power IC clients turn the vibrator on or off.
 *
 * @param        on         Turns the vibrator on or off (POWER_IC_PERIPH_ON or POWER_IC_PERIPH_OFF)
 *
 * @return       This function returns 0 if successful.
 */
int power_ic_periph_set_vibrator_on(int on)
{
    int error = 0;
    
#ifdef CONFIG_LINEARVIBRATOR
    /* A pointer to a function to be called when the linear vibrator needs to be enabled */
    static int (*vib_enable)(const char *name) = NULL;
    /* A pointer to a function to be called when the linear vibrator needs to be disabled */
    static void (*vib_disable)(void) = NULL;

    if(on > 0)
    {
        vib_enable = (void *)module_kallsyms_lookup_name((const char*)"pwm_vibrator_enable");

        /* Enable the linear vibrator if the address of pwm_vibrator_enable was found */
        if(vib_enable != NULL)
        {
            error = (*vib_enable)(NULL);
        }
    }
    else
    {
        vib_disable = (void *)module_kallsyms_lookup_name((const char*)"pwm_vibrator_disable");
        
        /* Disable the linear vibrator if the address of pwm_vibrator_disable was found */
        if(vib_disable != NULL)
        {
            (*vib_disable)();
        }
    }

#else
    /* Any input above zero is on.. */
    if(on > 0)
    {
        on = 1;
    }
    else
    {
        on = 0;
    }
                              
    error = power_ic_set_reg_value(VIBRATOR_ONOFF_REG, VIBRATOR_ONOFF_INDEX, on, VIBRATOR_ONOFF_NUM_BITS);
#endif
    return error;
}

/*!
 * @brief Returns 1 if USB cable is currently connected
 *
 * This function determines whether a USB cable is connected or not.  Currently, this
 * is a very simple check that looks at the USB 3.4v sense bit in the power IC.
 * At some point, this function should not be required and the hotplug interface with
 * the USB driver will be the only indication of when a cable is inserted/removed.
 *
 * @return 1 if USB cable is connected, 0 if not connected
 */

int power_ic_periph_is_usb_cable_connected (void)
{
    return power_ic_event_sense_read (USB_CABLE_ATTACH);
}

/*!
 * @brief Returns 1 if the USB pull-up is enabled
 *
 * This function checks the power IC Connectivity Control register to determine
 * if the USB pull-up has been enabled (bit 2, DP_1K5_PU).  The function returns the
 * state of the bit (1 = enabled, 0 = not enabled).
 *
 * @return 1 if USB pull-up enabled, 0 if not
 */

int power_ic_periph_is_usb_pull_up_enabled (void)
{
    int value;

    /* Read the bit from the power IC */
    if (power_ic_get_reg_value (USB_PU_REG, USB_PU_INDEX, &value, USB_PU_NUM_BITS) != 0)
    {
        value = 0;
    }

    return value;
}

/*!
 * @brief This function is called by power IC clients to change the SIM voltage values.
 *
 * @param        sim_card_num   Used later for different types of SIM
 * @param        volt           Changes SIM voltage to 1.8 or 3.0 volts
 *
 * @return       This function returns 0 if successful.
 *
 * @note         If volt is 0, then SIM voltage will be 1.8 volts.  If volt is 1, then SIM voltage 
 *               is 3.0 volts
 */
int power_ic_periph_set_sim_voltage(unsigned char sim_card_num, POWER_IC_SIM_VOLTAGE_T volt)
{
    int error = 0;

    if(sim_card_num == 0)
    {
#if defined(CONFIG_MACH_SAIPAN) || defined(CONFIG_MACH_PICO)
        /* VSIM is controlled by VMMC1 for Saipan.  Either set to 1.8V or 3.0V depending on volt */
        error = power_ic_set_reg_value(SIM_REG_VMMC1_EN, SIM_REG_VMMC1_EN_INDEX, 1, SIM_REG_VMMC1_EN_NUM_BITS);
        error |= power_ic_set_reg_value(SIM_REG_VMMC1_ESIM_EN, SIM_REG_VMMC1_ESIM_INDEX, 1, SIM_REG_VMMC1_ESIM_NUM_BITS);
        
        if(volt == 0)
        {
            error |= power_ic_set_reg_value(SIM_REG_VMMC1_VOLT, SIM_REG_VMMC1_VOLT_INDEX, 
                                            SIM_REG_VMMC1_1_8V, SIM_REG_VMMC1_VOLT_NUM_BITS);
        }
        else
        {
            error |= power_ic_set_reg_value(SIM_REG_VMMC1_VOLT, SIM_REG_VMMC1_VOLT_INDEX, 
                                            SIM_REG_VMMC1_3_0V,SIM_REG_VMMC1_VOLT_NUM_BITS);
        }
    }
#else
        error |= power_ic_set_reg_value(SIM_REG_SET_0_REG, SIM_REG_SET_0_VSIM_INDEX, volt,
                                        SIM_REG_SET_0_NUM_BITS);
    }
    else
    {  
        error |= power_ic_set_reg_value(SIM_REG_SET_0_REG, SIM_REG_SET_0_VESIM_INDEX, volt,
                                        SIM_REG_SET_0_NUM_BITS);
    }      
#endif
    return error;
}

/*!
 * @brief Set the state of the USB pull-up
 *
 * This function sets the state of the USB pull-up in the power IC Connectivity
 * Control register.
 *
 * @param on  0 to turn the pull-up off, anything else turns it on
 */

void power_ic_periph_set_usb_pull_up (int on)
{
    /* Set the USB pull-up bit in the power IC register */
    (void)power_ic_set_reg_bit (USB_PU_REG, USB_PU_INDEX, (on) ? 1 : 0);
}

/*!
 * @brief This function is called by power IC clients turn the camera on or off.
 *
 * @param        on         Turns the camera on or off (POWER_IC_PERIPH_ON or POWER_IC_PERIPH_OFF)
 *
 * @return       This function returns 0 if successful.
 */
int power_ic_periph_set_camera_on(int on)
{
    /* Any input above zero is on.. */
    if(on > 0)
    {
        on = 1;
    }
    else
    {
        on = 0;
    }
    
    return power_ic_set_reg_value(CAMERA_ONOFF_REG, CAMERA_ONOFF_INDEX, on,
                                  CAMERA_ONOFF_NUM_BITS);
}

/*!
 * @brief This function is the ioctl() interface handler for all peripheral operations.
 *
 * It is not called directly through an ioctl() call on the power IC device, but is executed from the core ioctl
 * handler for all ioctl requests in the range for peripherals.
 *
 * @param        cmd         the ioctl() command
 * @param        arg         the ioctl() argument
 *
 * @return       This function returns 0 if successful.
 */
int periph_ioctl(unsigned int cmd, unsigned long arg)
{
    int retval = 0;
    int data = (int) arg;
    
    /* Get the actual command from the ioctl request. */
#ifdef CONFIG_MOT_POWER_IC_TRACEMSG
    unsigned int cmd_num = _IOC_NR(cmd);
#endif

    tracemsg(_k_d("peripheral ioctl(), request 0x%X (cmd %d)"),(int) cmd, (int)cmd_num);
    
    /* Handle the request. */
    switch(cmd)
    {
        case POWER_IC_IOCTL_PERIPH_SET_VIBRATOR_LEVEL:
            retval = power_ic_periph_set_vibrator_level(data);
            break;

        case POWER_IC_IOCTL_PERIPH_SET_VIBRATOR_ON:
            retval = power_ic_periph_set_vibrator_on(data);
            break;

        case POWER_IC_IOCTL_PERIPH_SET_CAMERA_ON:
            retval = power_ic_periph_set_camera_on(data);
            break;

        default: /* This shouldn't be able to happen, but just in case... */
            tracemsg(_k_d("=> 0x%X unsupported peripheral ioctl command"), (int) cmd);
            retval = -EINVAL;
            break;
    }

    return retval;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
EXPORT_SYMBOL(power_ic_periph_set_vibrator_level);
EXPORT_SYMBOL(power_ic_periph_set_vibrator_on);
EXPORT_SYMBOL(power_ic_periph_is_usb_cable_connected);
EXPORT_SYMBOL(power_ic_periph_is_usb_pull_up_enabled);
EXPORT_SYMBOL(power_ic_periph_set_usb_pull_up);
EXPORT_SYMBOL(power_ic_periph_set_sim_voltage);
EXPORT_SYMBOL(power_ic_periph_set_camera_on);
#endif
