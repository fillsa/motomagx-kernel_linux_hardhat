/*
 * Copyright © 2004-2008, Motorola, All Rights Reserved.
 *
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this list of conditions
 *   and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, this list of
 *   conditions and the following disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * - Neither the name of Motorola nor the names of its contributors may be used to endorse or
 *   promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Motorola 2008-Jan-29 - Added function to set morphing mode
 * Motorola 2007-Feb-13 - Add privacy LED brightness support
 * Motorola 2007-Jan-26 - Add privacy brightness control
 * Motorola 2007-Jan-17 - Add one more funlight apps
 * Motorola 2006-Nov-24 - Support funlights feature
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-24 - Support ambient light sensor
 * Motorola 2006-Jul-07 - Make it compatible to C++ source file
 * Motorola 2006-May-30 - Added new interface to get the backlight brightness
 * Motorola 2006-May-22 - Add LED enable control
 * Motorola 2006-May-09 - Move declaration of lights_init.
 * Motorola 2006-Apr-12 - Added new interface functions to control backlight brightness.
 * Motorola 2006-Apr-07 - Added new region for Keypad NAV backlight
 * Motorola 2006-Jan-12 - Expand FL regions to 16, Add in BT_LED region
 * Motorola 2005-May-17 - Add SOL LED control, keypad backlight control, and Wifi status
 * Motorola 2004-Dec-17 - Initial Creation
 */

#ifndef __LIGHTS_FUNLIGHTS_H__
#define __LIGHTS_FUNLIGHTS_H__

/*!
 * @file lights_funlights.h
 *
 * @brief Global symbol definitions for the fun lights interface.
 *
 * @ingroup poweric_lights
 */

#ifdef __cplusplus
extern "C" {
#endif
/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/
#include <stdbool.h>
#include <stdarg.h>
#include <linux/morphing_mode.h>

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*!
 * @brief The maximum number of regions which can be controled by HAPI.
 */
#define LIGHTS_FL_MAX_REGIONS     16


/*!
 * @name Region Masks
 *
 * @brief Used in conjunction with LIGHTS_FL_Control_Set to set or remove control of regions.
 * Region 1 is bit 0 and region 8 is bit 7 as defined below.
 */
/*@{*/
#define LIGHTS_FL_REGION01_MSK    0x0001
#define LIGHTS_FL_REGION02_MSK    0x0002
#define LIGHTS_FL_REGION03_MSK    0x0004
#define LIGHTS_FL_REGION04_MSK    0x0008
#define LIGHTS_FL_REGION05_MSK    0x0010
#define LIGHTS_FL_REGION06_MSK    0x0020
#define LIGHTS_FL_REGION07_MSK    0x0040
#define LIGHTS_FL_REGION08_MSK    0x0080
#define LIGHTS_FL_REGION09_MSK    0x0100
#define LIGHTS_FL_REGION10_MSK    0x0200
#define LIGHTS_FL_REGION11_MSK    0x0400
#define LIGHTS_FL_REGION12_MSK    0x0800    
#define LIGHTS_FL_ALL_REGIONS_MSK 0xffff
/*@}*/

/*!
 * @name Colors
 *
 * @brief Define the basic colors which are supported by all of the tri colored LED drivers to be used
 * in conjunction with LIGHTS_FL_COLOR_T.
 */
/*@{*/
#define LIGHTS_FL_COLOR_BLACK   0x000000
#define LIGHTS_FL_COLOR_BLUE    0x0000ff
#define LIGHTS_FL_COLOR_GREEN   0x00ff00
#define LIGHTS_FL_COLOR_CYAN    0x00ffff
#define LIGHTS_FL_COLOR_RED     0xff0000
#define LIGHTS_FL_COLOR_MAGENTA 0xff00ff
#define LIGHTS_FL_COLOR_YELLOW  0xffff00
#define LIGHTS_FL_COLOR_WHITE   0xffffff
/*@}*/

/*!
 * @name Color Shift Values
 *
 * @brief Define the number of times a color value must be shifted in order to isolate only
 * the colors intensity.
 */
/*@{*/
#define LIGHTS_FL_COLOR_BLUE_SFT  0
#define LIGHTS_FL_COLOR_GREEN_SFT 8
#define LIGHTS_FL_COLOR_RED_SFT   16
/*@}*/

/*!
 * @name Fixed Regions
 *
 * @brief Define some fixed regions which are used by the apps to control backlights.  Please
 * note, not all of these are valid for all phones.
 */
/*@{*/
#define LIGHTS_FL_REGION_CAMERA_FLASH   1    
#define LIGHTS_FL_REGION_DISPLAY_BL     2
#define LIGHTS_FL_REGION_CLI_DISPLAY_BL 3
#define LIGHTS_FL_REGION_LOGO           4
#define LIGHTS_FL_REGION_KEYPAD_NAV_BL  5    
#define LIGHTS_FL_REGION_KEYPAD_NUM_BL  6
#define LIGHTS_FL_REGION_BT_LED         7
#define LIGHTS_FL_REGION_SOL_LED        8
#define LIGHTS_FL_REGION_PRIVACY_IND    9        
#define LIGHTS_FL_REGION_TRI_COLOR_1    10
#define LIGHTS_FL_REGION_TRI_COLOR_2    11
#define LIGHTS_FL_REGION_WIFI_STATUS    12



/*@}*/

/*!
 * @name Camera Flash Modes
 *
 * @brief  Define different modes for camera flash
 */
/*@{*/
#define LIGHTS_FL_CAMERA_TORCH   0x444444
#define LIGHTS_FL_CAMERA_REDEYE  0x888888
#define LIGHTS_FL_CAMERA_FLASH   0xffffff
/*@}*/

/*==================================================================================================
                                            MACROS
==================================================================================================*/

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*!
 * @name LED Controlling Apps
 *
 * @brief All of the apps which are allowed to control the LED's listed in increasing priority.
 */
typedef enum
{
   LIGHTS_FL_APP_CTL_DEFAULT = 0,
   LIGHTS_FL_APP_CTL_KJAVA,
   LIGHTS_FL_APP_CTL_FUN_LIGHTS,
   LIGHTS_FL_APP_CTL_ALERT_LIGHTS,
   LIGHTS_FL_APP_CTL_TST_CMDS,
   LIGHTS_FL_APP_CTL_END
} LIGHTS_FL_APP_CTL_T;

/*!
 * @name PRIVACY LED brightness table.
 * @brief Defines all the privacy LED brightness level
 */
typedef enum
{ 
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVELOFF =0,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL10,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL20,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL30,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL40,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL50,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL60,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL70,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL80,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL90,
    LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL100 = LIGHTS_FL_PRIVACY_BRIGHTNESS_LEVEL90,
    LIGHTS_FL_PRIVACY_NUM_BRIGHTNESS_LEVEL
 }LIGHTS_FL_PRIVACY_BRIGHTNESS_T;
 
/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/*!
 * @brief Define the type of fun light LED color
 *
 * The format of the type is to use the lower 24 bits
 * for color in the format rrrrrrrrggggggggbbbbbbbb where each position is a bit with r being the red
 * value, g being the green value and b being the blue value. If a region is not a
 * tri-color LED, then any non BLACK (0) value will result in the LED being turned on.
 *
 * Note:
 *   If the size of this typedef is made greater than the size of an unsigned int the
 *   code in LIGHTS_FL_vupdate will need to be updated.
 */
typedef unsigned int LIGHTS_FL_COLOR_T;

/*!
 * @brief Type used to define which region number will be affected for functions which take a region
 * value not a mask.
 *
 * Note: If the size of this typedef is made greater than the size of an unsigned int the
 * code in LIGHTS_FL_vupdate will need to be updated.
 */
typedef unsigned short int LIGHTS_FL_REGION_T;

/*!
 * @brief Define type which can be used to hold a bit mask with 1 bit per region.
 */
typedef unsigned short int LIGHTS_FL_REGION_MSK_T;

/*!
 * @brief Define the type of struct for the user space request to set the fun lights mode
 */
typedef struct
{
    LIGHTS_FL_APP_CTL_T nApp;
    LIGHTS_FL_REGION_MSK_T  nRegionMsk;
    LIGHTS_FL_REGION_MSK_T return_mask;
} LIGHTS_FL_SET_T;

/*!
 * @brief Define the type of struct for the user space request to update funlight
 */
typedef struct
{
    LIGHTS_FL_APP_CTL_T nApp;
    LIGHTS_FL_REGION_T nRegion;
    LIGHTS_FL_COLOR_T nColor;
    LIGHTS_FL_REGION_MSK_T return_mask;
} LIGHTS_FL_UPDATE_T;

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATION
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

extern LIGHTS_FL_REGION_MSK_T lights_fl_set_control 
(
   LIGHTS_FL_APP_CTL_T nApp,                
   LIGHTS_FL_REGION_MSK_T nRegionMask        
);

extern LIGHTS_FL_REGION_MSK_T lights_fl_vupdate  
(
    LIGHTS_FL_APP_CTL_T nApp,           
    unsigned char nRegions, 
    va_list pData                        
);

extern void lights_funlights_enable_led(void);

extern int lights_init(void);

#ifndef __KERNEL__
extern LIGHTS_FL_REGION_MSK_T lights_fl_update   
(
    LIGHTS_FL_APP_CTL_T nApp,            
    LIGHTS_FL_REGION_T nRegion,
    LIGHTS_FL_COLOR_T  nColor
);

void lights_fl_set_morphing_mode
(
    MORPHING_MODE_E mode
);

extern int lights_close(void);
#endif /* __KERNEL__ */

void lights_funlights_set_bl_pwm_brightness(unsigned char value);

char lights_funlights_get_bl_pwm_brightness(void);

#ifdef __cplusplus
}
#endif

#endif /* __LIGHTS_FUNLIGHTS_H__ */
