/*
 * Copyright (C) 2005-2006, 2008 Motorola, Inc.
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
 * Motorola 2008-Jan-10 - Fix the share keypad LED
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Oct-05 - Fix display backlight in keylock mode
 * Motorola 2006-Sep-11 - Fix backlight update issue for Lido
 * Motorola 2006-Sep-06 - Track lighting update status
 * Motorola 2006-May-31 - Added new interface functions to provide backlight status.
 * Motorola 2006-Apr-12 - Added new interface functions to control backlight brightness.
 * Motorola 2006-Apr-07 - Add support for NAV Keypad backlight
 * Motorola 2005-Feb-28 - Rewrote the software.
 */

/*!
 * @file lights_backlight.c
 *
 * @brief Backlight setting
 *
 *  This is the main file of the backlight interface which conrols the backlight setting.
 *
 * @ingroup poweric_lights
 */

#include <linux/power_ic_kernel.h>
#include <linux/lights_backlight.h>
#include <linux/lights_funlights.h>
#include <linux/module.h>
/*******************************************************************************************
                                           CONSTANTS
 ******************************************************************************************/
const LIGHTS_FL_REGION_MSK_T bl_to_region[LIGHTS_BACKLIGHT_ALL] =
    {
        LIGHTS_FL_REGION_KEYPAD_NUM_BL,  /* LIGHTS_BACKLIGHT_KEYPAD      */
        LIGHTS_FL_REGION_DISPLAY_BL,     /* LIGHTS_BACKLIGHT_DISPLAY     */
        LIGHTS_FL_REGION_CLI_DISPLAY_BL, /* LIGHTS_BACKLIGHT_CLI_DISPLAY */
        LIGHTS_FL_REGION_KEYPAD_NAV_BL   /* LIGHTS_BACKLIGHT_NAV         */
    };
/********************************************************************************************
                                 STRUCTURES AND OTHER TYPEDEFS
*******************************************************************************************/
static LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T  lights_backlight_update_status[LIGHTS_BACKLIGHT_ALL] =
{
    0,  /* LIGHTS_BACKLIGHT_KEYPAD */
    0,  /* LIGHTS_BACKLIGHT_DISPLAY */
    0,  /* LIGHTS_BACKLIGHT_CLI_DISPLAY */
    0   /* LIGHTS_BACKLIGHT_NAV  */
};

/*******************************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************************/
/*!
 * @brief  Updates the stored backlight brightness step for each region.
 * This function will update the backlight brightness step of all the light regions.
 *
 * @param bl_select      Indicates which backlights to enable/disable.
 * @param bl_step        Level of brightness.
 * @return               Level of brightness used to update the lights 
 */
static LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T lights_backlight_status_update( LIGHTS_BACKLIGHT_T bl_select,  
                                                                          LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T bl_step)
{
    unsigned char i;

    if (bl_select >= LIGHTS_BACKLIGHT_ALL)
    {
        /* Update state of all backlights */
        for(i = 0; i < LIGHTS_BACKLIGHT_ALL; i++)
        {
            lights_backlight_update_status[i] = bl_step;
        }
    }
    else
    {
        /* Updated the state of the selected backlight */
        lights_backlight_update_status[bl_select] = bl_step;
    }
    
    /* The shared backlight can only be disabled
       if both the cli and display backlight are disabled */
#if defined(CONFIG_MACH_LIDO)
    if (bl_select == LIGHTS_BACKLIGHT_DISPLAY)
    { 
        if(bl_step !=0)
        {
            lights_backlight_update_status[LIGHTS_BACKLIGHT_CLI_DISPLAY] = bl_step;
            /* Update light color queue for CLI. It makes sure that CLI does
               not turning off main display when lights_fl_set_control is
               called by LIGHTS_FL_APP_CTL_DEFAULT. */
            lights_fl_update(LIGHTS_FL_APP_CTL_DEFAULT, 1,LIGHTS_FL_REGION_CLI_DISPLAY_BL, bl_step);
        }
        
        else
        {
            return (lights_backlight_update_status[LIGHTS_BACKLIGHT_CLI_DISPLAY]);
        }
    }
    else if (bl_select == LIGHTS_BACKLIGHT_CLI_DISPLAY)
    {
        if(bl_step !=0)
        {
            lights_backlight_update_status[LIGHTS_BACKLIGHT_DISPLAY] = bl_step;
            /*Update the light color queue for main display.It makes sure that
              main display does not turning off CLI display when
              lights_fl_set_control is called by LIGHTS_FL_APP_CTL_DEFAULT.*/
            lights_fl_update(LIGHTS_FL_APP_CTL_DEFAULT, 1,LIGHTS_FL_REGION_DISPLAY_BL,bl_step);
        }
        else
        {
            return(lights_backlight_update_status[LIGHTS_BACKLIGHT_DISPLAY]);
        }
    }
#elif defined (CONFIG_MACH_MARCO)
    if (bl_select == LIGHTS_BACKLIGHT_KEYPAD)
    { 
        if(bl_step !=0)
        {
            lights_backlight_update_status[LIGHTS_BACKLIGHT_NAV] = bl_step;
            /* Update light color queue for NAV keypad. It makes sure that NAV keypad does
               not turning off number keypad when lights_fl_set_control is
               called by LIGHTS_FL_APP_CTL_DEFAULT. */
            lights_fl_update(LIGHTS_FL_APP_CTL_DEFAULT, 1,LIGHTS_FL_REGION_KEYPAD_NAV_BL, bl_step);
        }
        
        else
        {
            return (lights_backlight_update_status[LIGHTS_BACKLIGHT_NAV]);
        }
    }
    else if (bl_select == LIGHTS_BACKLIGHT_NAV)
    {
        if(bl_step !=0)
        {
            lights_backlight_update_status[LIGHTS_BACKLIGHT_KEYPAD] = bl_step;
            /*Update the light color queue for number keypad.It makes sure that
              navigation keypad does not turning off number keypad when
              lights_fl_set_control is called by LIGHTS_FL_APP_CTL_DEFAULT.*/
            lights_fl_update(LIGHTS_FL_APP_CTL_DEFAULT, 1,LIGHTS_FL_REGION_KEYPAD_NUM_BL,bl_step);
        }
        else
        {
            return(lights_backlight_update_status[LIGHTS_BACKLIGHT_KEYPAD]);
        }
    }
#endif 
    return bl_step;
} 

/*******************************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************************/
/*!
 * @brief  Set the backlight to specific step level as defined by the hardware.
 *
 * This function will set the backlight intensity to the step provided. 
 *
 * @param bl_select      Indicates which backlights to enable/disable.
 * @param bl_step        Level of brightness.
 */
void lights_backlightset_step
(
    LIGHTS_BACKLIGHT_T bl_select,  
    LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T bl_step
)
{
    
    /* Update bl_enable according to shared LED state */
    bl_step = lights_backlight_status_update(bl_select, bl_step);
   
    /*
     * For the backlights the color is used to pass the intensity in steps supported
     * by the hardware.
     */
    if (bl_select >= LIGHTS_BACKLIGHT_ALL)
    {
        lights_fl_update(LIGHTS_FL_APP_CTL_DEFAULT, 4,
                         LIGHTS_FL_REGION_KEYPAD_NUM_BL,
                         bl_step,
                         LIGHTS_FL_REGION_KEYPAD_NAV_BL,
                         bl_step,
                         LIGHTS_FL_REGION_CLI_DISPLAY_BL,
                         bl_step,
                         LIGHTS_FL_REGION_DISPLAY_BL,
                         bl_step);
    }
    else
    {
        lights_fl_update(LIGHTS_FL_APP_CTL_DEFAULT, 1, bl_to_region[bl_select], bl_step);
    }
}

/*!
 * @brief  Set the backlight intensity.
 *
 * This function activates/deactivates (with intensity for the main and cli displays)
 * the selected backlight(s).  The brightness will be in a range of 0 to 100 with 0 being
 * off and 100 being full intensity.
 *
 * @param bl_select      Indicates which backlights to enable/disable.
 * @param bl_brightness  Levels of brightness from 0 to 100 with 0 being off.
 */
void lights_backlightset(LIGHTS_BACKLIGHT_T bl_select, LIGHTS_BACKLIGHT_BRIGHTNESS_T bl_brightness)
{
    LIGHTS_BACKLIGHT_T i;
    LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T bl_step;
  
    if (bl_select >= LIGHTS_BACKLIGHT_ALL)
    {
        i = LIGHTS_BACKLIGHT_NAV;
        do
        {
            bl_step = (bl_brightness + lights_bl_percentage_steps[i] -1)/lights_bl_percentage_steps[i];
            if(bl_step >= lights_bl_num_steps_tb[i])
            {
                bl_step = lights_bl_num_steps_tb[i] -1;
            }
            lights_backlightset_step(i, bl_step);
        } while(i--);
    }
    else
    {
        bl_step =
            (bl_brightness + lights_bl_percentage_steps[bl_select] - 1)/lights_bl_percentage_steps[bl_select];
        
        if(bl_step >= lights_bl_num_steps_tb[bl_select]) 
        {
            bl_step = lights_bl_num_steps_tb[bl_select] -1;
        }
        lights_backlightset_step(bl_select, bl_step);
    }
}

/*!
 * @brief  Return the number of brightness steps supported for a backlight
 *
 * This function will return the number of brightness steps implemented in the hardware
 * driver for a type of backlight. The number of steps will include the value for off.
 *
 * @param bl_select      Indicates which backlights to enable/disable (LIGHTS_BACKLIGHT_ALL
 *                       cannot be used with this function).
 * @return               The number of steps supported.
 */
LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T lights_backlightget_steps
(
    LIGHTS_BACKLIGHT_T bl_select
)
{
    if (bl_select < LIGHTS_BACKLIGHT_ALL)
    {
        return lights_bl_num_steps_tb[bl_select];
    }
    return 0;
}

/*!
 * @brief  Convert a step to a percentage based on the current hardware.
 *
 * This function will convert a step to a percentage suitable for use by
 * lights_backlightset().
 *
 * @param bl_select      Indicates which backlights to run the conversion on
 *                       (LIGHTS_BACKLIGHT_ALL cannot be used with this function).
 * @param bl_step        The brightness step to convert to a percentage.
 * @return               The percentage equivalent to bl_step.
 */
LIGHTS_BACKLIGHT_BRIGHTNESS_T lights_backlights_step_to_percentage
(
    LIGHTS_BACKLIGHT_T bl_select,
    LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T bl_step
)
{
    LIGHTS_BACKLIGHT_BRIGHTNESS_T tmp = 0;

    if (bl_select < LIGHTS_BACKLIGHT_ALL)
    {
        tmp = lights_bl_percentage_steps[bl_select]*bl_step;
        if (tmp > 100)
        {
            tmp = 100;
        }
    }
    return tmp;
}

/*!
 * @brief  Return the current backlight step of the request region.
 *
 * @param bl_select      Indicates which backlights to run the conversion on
 *                       (LIGHTS_BACKLIGHT_ALL cannot be used with this function).
 * @return               The step of the request region.
 */

LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T lights_backlightget_steps_val
(
    LIGHTS_BACKLIGHT_T bl_select
)
{
    if (bl_select < LIGHTS_BACKLIGHT_ALL)
    { 
        return ((LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T)
                lights_fl_get_region_color(LIGHTS_FL_APP_CTL_DEFAULT,bl_to_region[bl_select]));
    }
    return 0;
}

/*!
 * @brief  Return the current backlight percentage of the request region.
 *
 * @param bl_select      Indicates which backlights to run the conversion on
 *                       (LIGHTS_BACKLIGHT_ALL cannot be used with this function).
 * @return               The backlight brightness percentage of the request region.
 */
LIGHTS_BACKLIGHT_BRIGHTNESS_T lights_backlightget_val
(
   LIGHTS_BACKLIGHT_T bl_select
)
{
     if (bl_select < LIGHTS_BACKLIGHT_ALL)
     {  
         return lights_backlights_step_to_percentage(bl_select, (LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T)
                                                     lights_fl_get_region_color(LIGHTS_FL_APP_CTL_DEFAULT,
                                                                                bl_to_region[bl_select]));
     }
     return 0;
}
/*==================================================================================================
                                     EXPORTED SYMBOLS
==================================================================================================*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
EXPORT_SYMBOL(lights_backlightset_step);
EXPORT_SYMBOL(lights_backlightset);
EXPORT_SYMBOL(lights_backlightget_steps);
EXPORT_SYMBOL(lights_backlights_step_to_percentage);
EXPORT_SYMBOL(lights_backlightget_steps_val);
EXPORT_SYMBOL(lights_backlightget_val);
#endif   
 
