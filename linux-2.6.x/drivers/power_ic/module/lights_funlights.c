/*
 * Copyright (C) 2004-2008 Motorola, Inc.
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
 * Motorola 2008-Jan-29 - Added IOCTL entry for setting morphing mode
 * Motorola 2007-May-11 - Remove fl_setcontrol from lights_init
 * Motorola 2007-May-08 - Add turbo indicator functions.
 * Motorola 2007-Feb-19 - Move fl_initialize call to lights_init
 * Motorola 2006-Nov-15 - Remove LED register initialization.
 * Motorola 2006-Nov-07 - Fix Klocwork Warnings
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Sep-25 - Move function calls from power_ic_init to lights_init
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-24 - Support ambient light sensor
 * Motorola 2006-Jul-03 - Add boundary check for lights_fl_set_control
 * Motorola 2006-May-31 - Added new interface functions to provide backlight status.
 * Motorola 2006-Apr-12 - Added new interface functions to control backlight brightness.
 * Motorola 2006-Apr-07 - Change camera torch GPIO for Ascension P2
 * Motorola 2006-Apr-04 - Move all GPIO functionality to gpio.c
 * Motorola 2006-Feb-22 - Fix GPIO data sets for Ascension
 * Motorola 2006-Jan-12 - Add in lights_init()
 * Motorola 2005-Nov-22 - Simplified the interface functions for the funlight driver
 * Motorola 2004-Dec-17 - Design of the interface functions for the funlight driver
 */


/*!
 * @file lights_funlights.c
 *
 * @ingroup poweric_lights
 *
 * @brief Funlight module
 *
 *  In this file, there are interface functions between the funlights driver and outside world.
 *  These functions implement a cached priority based method of controlling fun lights regions.
 *  Upon powerup the regions are assigned to the default app, which can be looked at as the old functionality.
 *  This allows the keypad and display backlights to operate as they do today, when they are not in use by fun
 *  lights or KJAVA.  If a new application needs to be added it must be done in the corresponding
 *  header file.
 */
#include <stdbool.h>
#include <linux/kernel.h>
#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include <linux/lights_funlights.h>
#include <linux/lights_backlight.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <asm/uaccess.h>

#include "../core/fl_register.h"
#include "../core/gpio.h"
#include "../core/os_independent.h"

/*******************************************************************************************
 * LOCAL MACROS
 ******************************************************************************************/
/*!
 * @brief Converts a fun light region to mask
 */
#define LIGHTS_FL_NREGION_TO_MASK(nRegion)  (1 << nRegion)

/*******************************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************************/
/*
 * An array which holds the current status of an LED for a region and priority.  To
 * access an LED use LIGHTS_FL_color_queue[priority][region].  The value which is contained
 * in the element must be of  LIGHTS_FL_COLOR_T, however the array is not defined as such
 * to same memory.
 */
static LIGHTS_FL_COLOR_T LIGHTS_FL_color_queue[LIGHTS_FL_APP_CTL_END][LIGHTS_FL_MAX_REGIONS];

/*
 * An array which is used to control the priorities of the regions.  Each region can have an
 * independent control, thus each region has a bit in the array.  When the bit is clear, the
 * region is inactive, when set it is active.
 */
static LIGHTS_FL_REGION_MSK_T LIGHTS_FL_region_priorities[LIGHTS_FL_APP_CTL_END];

/*******************************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************************/

/*******************************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************************/
/*!
  * @brief Read application priority
  *       Returns the number of the highest priority app which has control the region requested.
  *
  * @param  nRegion  Region ID
  *
  * @return     The application priority
  */

static  LIGHTS_FL_APP_CTL_T lights_fl_get_priority_app 
(
    LIGHTS_FL_REGION_T nRegion  
)
{
    LIGHTS_FL_APP_CTL_T i;
    LIGHTS_FL_REGION_MSK_T nRegionMask;

    nRegionMask = LIGHTS_FL_NREGION_TO_MASK(nRegion);
    i = LIGHTS_FL_APP_CTL_END-1;
    while ((i > 0) && (!(LIGHTS_FL_region_priorities[i] & nRegionMask)))
    {
        i--;
    }
    return (i);
}

/*==================================================================================================
                                       GLOBAL FUNCTIONS
==================================================================================================*/
/*!
 * @brief Initializes the lighting
 *
 * The function performs any initialization required to get the lighting
 * for SCM-A11 running.  
 */
int lights_init(void)
{
    /* Configure all GPIOs needed for lighting */
    power_ic_gpio_lights_config();

    /* Initialize the funlights driver */
    fl_initialize();

    return 0;
}

/*!
 * @brief Enable the turbo mode light
 *
 * If turbo mode indication is enabled, this function will enable the bluetooth LED.
 *
 * @return 0 on success, -EPERM if turbo mode indicator is disabled.
 */
int lights_enter_turbo_mode(void)
{
    int retval = -EPERM;

#ifdef CONFIG_MOT_TURBO_INDICATOR
    if (LIGHTS_FL_region_ctl_tb[LIGHTS_FL_REGION_BT_LED-1].pFcn != NULL)
    {
        (*LIGHTS_FL_region_ctl_tb[LIGHTS_FL_REGION_BT_LED-1].pFcn)(
            &LIGHTS_FL_region_ctl_tb[LIGHTS_FL_REGION_BT_LED-1].xCtlData,
            LIGHTS_FL_COLOR_BLUE);
        retval = 0;
    }
#endif

    return retval;
}

/*!
 * @brief Disable the turbo mode light
 *
 * If turbo mode indication is enabled, this function will disable the bluetooth LED.
 *
 * @return 0 on success, -EPERM if turbo mode indicator is disabled.
 */
int lights_exit_turbo_mode(void)
{
    int retval = -EPERM;

#ifdef CONFIG_MOT_TURBO_INDICATOR
    if (LIGHTS_FL_region_ctl_tb[LIGHTS_FL_REGION_BT_LED-1].pFcn != NULL)
    {
        (*LIGHTS_FL_region_ctl_tb[LIGHTS_FL_REGION_BT_LED-1].pFcn)(
            &LIGHTS_FL_region_ctl_tb[LIGHTS_FL_REGION_BT_LED-1].xCtlData,
            LIGHTS_FL_COLOR_BLACK);
        retval = 0;
    }
#endif

    return retval;
}

/*!
  * @brief Turn on/off fun light
  *
  * Enables the regions specified in the region mask for control by the app which is passed in nApp.
  * When bits in the mask are clear access is returned to lower priority apps.  Please note an app
  * will only be granted access when a lower priority task currently has access.  If a higher priority
  * task currently has control the requesting app will be granted access once all higher priority
  * tasks have released control.  If a higher priority task has control the request for access
  * will be queued, if this is not desired the function must be called again with the request bit
  * cleared.
  *
  * @param  nApp         The app will take or release control of the regions in the mask 
  * @param  nRegionMsk   The regions which are set in the mask.
  *
  * @return  Returns a bit mask with all regions for which access was granted set. 
  */

LIGHTS_FL_REGION_MSK_T lights_fl_set_control  
(
   LIGHTS_FL_APP_CTL_T nApp,         
   LIGHTS_FL_REGION_MSK_T nRegionMsk 
)
{
    LIGHTS_FL_REGION_T i;
    LIGHTS_FL_REGION_MSK_T nReturn;
    LIGHTS_FL_REGION_MSK_T nRegionChangedMsk;   /* Mask in which the bits are set for regions which have changed
                                                   state are set. */
    LIGHTS_FL_APP_CTL_T nPriorityApp;

    nReturn = 0;
    if (nApp >= LIGHTS_FL_APP_CTL_END)
    {
        tracemsg(_k_d("lights_fl_set_control Error:  Invalid App."));
        return 0;
    }
    nRegionChangedMsk = LIGHTS_FL_region_priorities[nApp] ^ nRegionMsk;
    LIGHTS_FL_region_priorities[nApp] = nRegionMsk;
    
    /*
     * Update the LED's from the cache.  These may be for this app or a lower priority
     * app which now has control.
     */
    for (i = 0; i < LIGHTS_FL_MAX_REGIONS; i++)
    {
        nPriorityApp = lights_fl_get_priority_app(i);
        
        /*
         * Update the LEDs for all apps which have the same or lower priority.
         */
        if (nPriorityApp <= nApp)
        {
            /*
             * Only set the bits in the return value which are valid for the app for which
             * the call was made.
             */
            if (nPriorityApp == nApp)
            {
                nReturn |= nRegionMsk & LIGHTS_FL_NREGION_TO_MASK(i);
            }
            /*
             * Only update the LED if the priority actually changed.
             */
            if (nRegionChangedMsk & LIGHTS_FL_NREGION_TO_MASK(i))
            {
                /* If the region is active request an update to the current status of the LED. */
#ifdef CONFIG_MOT_TURBO_INDICATOR
                if ((LIGHTS_FL_region_ctl_tb[i].pFcn != NULL) && (i != (LIGHTS_FL_REGION_BT_LED-1)))
#else
                if (LIGHTS_FL_region_ctl_tb[i].pFcn != NULL)
#endif
                {
                    (void)(*LIGHTS_FL_region_ctl_tb[i].pFcn)(&LIGHTS_FL_region_ctl_tb[i].xCtlData,
                                                             LIGHTS_FL_color_queue[nPriorityApp][i]);
                }
            }
        }
    }
    return (nReturn);
}


/*!
  * @brief Update fun lights
  *
  * This function takes a va_list instead of a prameter list but is functionally equivalent to
  * LIGHTS_FL_update.
  *
  * @param  nApp     The app enumeration which is requesting the LED update 
  * @param  nRegions The number of region data pairs which follow. 
  * @param  pData    Parameter pairs which include region followed by color. 
  *
  * @return Returns a bit mask with the bits corresponding to
  *         those regions which were updated set.  If a request was
  *         queued, due to a higher priority app having access the
  *         bit is clear.
  */

LIGHTS_FL_REGION_MSK_T lights_fl_vupdate  
(
    LIGHTS_FL_APP_CTL_T nApp,
    unsigned char nRegions,           
    va_list pData             
)
{
    LIGHTS_FL_REGION_T nRegion;
    LIGHTS_FL_COLOR_T nColor;
    LIGHTS_FL_REGION_MSK_T nReturn;

    nReturn = 0;
   
    if (nApp < LIGHTS_FL_APP_CTL_END)
    {
        while (nRegions > 0)
        {
            /*
             * Store the current value in the queue.  Please note, when variable parameter lists are
             * used argument promotion occurs.  If the size of the type is less than an int it will
             * be promoted to an int.  For this reason some special handling must be done to guarantee
             * the parameters are handled correctly.
             */
            nRegion = (LIGHTS_FL_REGION_T)va_arg(pData, unsigned int);
            nColor = (LIGHTS_FL_COLOR_T)va_arg(pData, unsigned int);

#ifdef CONFIG_MOT_TURBO_INDICATOR
            if ((nRegion > 0) && (nRegion <= LIGHTS_FL_MAX_REGIONS) && (nRegion != LIGHTS_FL_REGION_BT_LED))
#else
            if ((nRegion > 0) && (nRegion <= LIGHTS_FL_MAX_REGIONS))
#endif
            {
                /* The first region number is 1, however this corresponds to array element 0. */
                LIGHTS_FL_color_queue[nApp][--nRegion] = nColor;
                /*
                 * If the region is active activate the LED.
                 */
                if ((lights_fl_get_priority_app(nRegion) == nApp) &&
                    (LIGHTS_FL_region_ctl_tb[nRegion].pFcn != NULL))
                {
                    if ((*LIGHTS_FL_region_ctl_tb[nRegion].pFcn)(&LIGHTS_FL_region_ctl_tb[nRegion].xCtlData,
                                                                 nColor))
                    {
                        nReturn |= LIGHTS_FL_NREGION_TO_MASK(nRegion);
                    }
                }
            }
            nRegions--;
        }
    }
    lights_funlights_enable_led();
    return (nReturn);
}
/*!
  * @brief Update fun light
  *
  * Assigns colors to regions for the app which is passed in. If the app is not currently active
  * the color assignment is cached but will not be active. In the case the LED is not active
  * the corresponding bit in the return mask will be clear.
  *
  * @param  nApp     The app enumeration which is requesting the LED update 
  * @param  nRegions The number of region data pairs which follow. 
  * @param  ...      Parameter pairs which include region followed by color. 
  *
  * @return Returns a bit mask with the bits corresponding to
  *         those regions which were updated set.  If a request was
  *         queued, due to a higher priority app having access the
  *         bit is clear.
  */

LIGHTS_FL_REGION_MSK_T lights_fl_update    
(
    LIGHTS_FL_APP_CTL_T nApp,       
    unsigned char nRegions,                
    ...
    /* LIGHTS_FL_REGION_T nRegion,     Optional Input - The region which the following color is for.  */
    /* LIGHTS_FL_COLOR_TYPE nColor,    Optional Input - The color to set the preceeding region to.    */
                                    /* Repeat the two inputs above as needed. */
)
{
    va_list pArgs;
    LIGHTS_FL_REGION_MSK_T nReturn;

    va_start(pArgs, nRegions);
    nReturn = lights_fl_vupdate(nApp, nRegions, pArgs);
    va_end(pArgs);

    return (nReturn);
}


/*!
 * @brief  Return the current light color for the requested region.
 *
 * @param nRegion      Indicates which light region to run the conversion on.
 *         
 * @return             The light color of the request region.
 */

LIGHTS_FL_COLOR_T lights_fl_get_region_color
(
    LIGHTS_FL_APP_CTL_T nApp,
    LIGHTS_FL_REGION_T nRegion
)
{
    if (nApp >= LIGHTS_FL_APP_CTL_END)
    {
        tracemsg(_k_d("lights_fl_get_region_color Error:  Invalid App."));
        return 0;
    }
    
    return LIGHTS_FL_color_queue[nApp][nRegion];
}

/*!
 * @brief Lights module ioctl function
 *
 * This function implements IOCTL controls on a Power_Ic lights module.
 * @param  cmd   the command
 * @param  arg   the parameter
 *
 * @return returns 0 if successful.
 */

int lights_ioctl(unsigned int cmd, unsigned long arg)
{
    union
    {
        LIGHTS_BACKLIGHT_IOCTL_T backlight_ioctl;
        LIGHTS_FL_SET_T  funlights_set;
        LIGHTS_FL_UPDATE_T funlights_update;
        int light_sensor_atod;
    } u;
    int ret_val = 0;
    
    switch(cmd)
    {
        case POWER_IC_IOCTL_LIGHTS_BACKLIGHTS_SET:
            if(copy_from_user((void*)&u.backlight_ioctl, (void *)arg, sizeof(u.backlight_ioctl)))
            {
                ret_val = -EFAULT;
            }
            else
            {
                lights_backlightset(u.backlight_ioctl.bl_select,
                                    u.backlight_ioctl.bl_brightness);
            }
            break;
            
        case POWER_IC_IOCTL_LIGHTS_FL_SET_CONTROL:
            if(copy_from_user((void*)&u.funlights_set,(void *)arg, sizeof(u.funlights_set)))
            {
                
                ret_val = -EFAULT;
            }
            else
            {
                u.funlights_set.return_mask = lights_fl_set_control(u.funlights_set.nApp, u.funlights_set.nRegionMsk);
                
                 /* Copy the data back to user space */
                ret_val = copy_to_user ((void *)arg, (void *)&u.funlights_set, sizeof(u.funlights_set));

                /* If the copy failed, return an error */
                if (ret_val != 0)
                {
                    ret_val = -EFAULT;
                }
            }
            break;
            
        case POWER_IC_IOCTL_LIGHTS_FL_UPDATE:
            if(copy_from_user((void*)&u.funlights_update,(void *)arg, sizeof(u.funlights_update)))
            {
                
                ret_val = -EFAULT;
            }
            else
            {
                u.funlights_update.return_mask =
                    lights_fl_update(u.funlights_update.nApp, 1, u.funlights_update.nRegion, u.funlights_update.nColor);

                /* Copy the data back to user space */
                ret_val = copy_to_user ((void *)arg, (void *)&u.funlights_update, sizeof(u.funlights_update));
                /* If the copy failed, return an error */
                if (ret_val != 0)
                {
                    ret_val = -EFAULT;
                }
            }
            break;

        case POWER_IC_IOCTL_LIGHTS_BACKLIGHTS_STEP_SET:
            if(copy_from_user((void*)&u.backlight_ioctl, (void *)arg, sizeof(u.backlight_ioctl)))
            {
                ret_val = -EFAULT;
            }
            else
            {
                lights_backlightset_step(u.backlight_ioctl.bl_select, u.backlight_ioctl.bl_step);
            }
            break;

        case POWER_IC_IOCTL_LIGHTS_BACKLIGHT_GET_STEPS:
            if (copy_from_user((void*)&u.backlight_ioctl, (void *)arg, sizeof(u.backlight_ioctl)))
            {
                ret_val = -EFAULT;
            }
            else
            {
                u.backlight_ioctl.bl_step = lights_backlightget_steps(u.backlight_ioctl.bl_select);

                /* Copy the data back to user space */
                ret_val = copy_to_user ((void *)arg, (void *)&u.backlight_ioctl, sizeof(u.backlight_ioctl));
                /* If the copy failed, return an error */
                if (ret_val != 0)
                {
                    ret_val = -EFAULT;
                }
            }
            break;

        case POWER_IC_IOCTL_LIGHTS_BACKLIGHT_STEPS_TO_PERCENT:
            if (copy_from_user((void*)&u.backlight_ioctl, (void *)arg, sizeof(u.backlight_ioctl)))
            {
                ret_val = -EFAULT;
            }
            else
            {
                u.backlight_ioctl.bl_brightness =
                    lights_backlights_step_to_percentage(u.backlight_ioctl.bl_select,
                                                         u.backlight_ioctl.bl_step);
                /* Copy the data back to user space */
                ret_val = copy_to_user ((void *)arg, (void *)&u.backlight_ioctl, sizeof(u.backlight_ioctl));
                /* If the copy failed, return an error */
                if (ret_val != 0)
                {
                    ret_val = -EFAULT;
                }
            }
            break;

       case POWER_IC_IOCTL_LIGHTS_BACKLIGHT_GET_STEPS_VAL:
           if (copy_from_user((void*)&u.backlight_ioctl, (void *)arg, sizeof(u.backlight_ioctl)))
           {
               ret_val = -EFAULT;
           }
           else
           {
               u.backlight_ioctl.bl_step = lights_backlightget_steps_val(u.backlight_ioctl.bl_select);
               /* Copy the data back to user space */
               ret_val = 
                   copy_to_user ((void *)arg, (void *)&u.backlight_ioctl, sizeof(u.backlight_ioctl));
               /* If the copy failed, return an error */
               if (ret_val != 0)
               {
                   ret_val = -EFAULT;
               }
           }
           break;

       case POWER_IC_IOCTL_LIGHTS_BACKLIGHT_GET_VAL:
           if (copy_from_user((void*)&u.backlight_ioctl, (void *)arg, sizeof(u.backlight_ioctl)))
           {
               ret_val = -EFAULT;
           }
           else
           {
               u.backlight_ioctl.bl_brightness = lights_backlightget_val(u.backlight_ioctl.bl_select);
               /* Copy the data back to user space */
               ret_val = 
                   copy_to_user ((void *)arg, (void *)&u.backlight_ioctl, sizeof(u.backlight_ioctl));
               /* If the copy failed, return an error */
               if (ret_val != 0)
               {
                   ret_val = -EFAULT;
               }
           }
           break;

        case POWER_IC_IOCTL_LIGHTS_LIGHT_SENSOR_START:
            
            /*Enable light sensor.*/
            ret_val = lights_funlights_enable_light_sensor(1);
            break;
            
        case POWER_IC_IOCTL_LIGHTS_LIGHT_SENSOR_GET_LUMA:
            
            u.light_sensor_atod = lights_funlights_light_sensor_atod();
            
            /* Copy the data back to user space */
            ret_val = copy_to_user ((void *)arg, (void *)&u.light_sensor_atod, sizeof(u.light_sensor_atod));
            /* If the copy failed, return an error */
            if (ret_val != 0)
            {
                ret_val = -EFAULT;
            }
            break;
        case POWER_IC_IOCTL_LIGHTS_LIGHT_SENSOR_STOP:
            
            ret_val = lights_funlights_enable_light_sensor(0);
            break;
	    
        case POWER_IC_IOCTL_LIGHTS_SET_MORPHING_MODE:
#ifdef CONFIG_MACH_XPIXL
            ret_val = lights_funlights_set_morphing_mode((MORPHING_MODE_E)arg);
#endif	   
            break;
	    
        default:
            tracemsg(_k_d("0x%X unsupported ioctl command"), (int) cmd);
            ret_val = -ENOTTY;
            break;
    }
    return ret_val;
}
            
/*==================================================================================================
                                     EXPORTED SYMBOLS
==================================================================================================*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS            
EXPORT_SYMBOL(lights_fl_set_control);
EXPORT_SYMBOL(lights_fl_vupdate);
EXPORT_SYMBOL(lights_fl_update);
EXPORT_SYMBOL(lights_ioctl);
EXPORT_SYMBOL(lights_fl_get_region_color);
EXPORT_SYMBOL(lights_enter_turbo_mode);
EXPORT_SYMBOL(lights_exit_turbo_mode);
#endif
