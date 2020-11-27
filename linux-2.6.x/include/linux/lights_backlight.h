/*
 * Copyright © 2004-2006, Motorola, All Rights Reserved.
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
 * Motorola 2006-May-31 - Added new interface functions to provide backlight status.
 * Motorola 2006-Apr-12 - Added new interface functions to control backlight brightness.
 * Motorola 2005-Jan-26 - Add user space calls and enums
 * Motorola 2004-Dec-17 - Initial Creation
 */

#ifndef __LIGHTS_BACKLIGHTS_H__
#define __LIGHTS_BACKLIGHTS_H__

/*!
 * @file lights_backlight.h
 *
 * @brief Global symbols definitions for lights_backlights.c interface.
 *
 * @ingroup poweric_lights
 */
/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                            MACROS
==================================================================================================*/

/*==================================================================================================
                                             ENUMS
==================================================================================================*/
/*!
 * @brief The regions which can be used with the backlight functions.
 *
 * The following enumeration defines the regions which can be used with the backlight functions
 * defined within this file.  Please note LIGHTS_BACKLIGHT_ALL is not supported by all functions.
 * Please check the function documentation before attempting to use LIGHTS_BACKLIGHT_ALL.
 */
typedef enum
{
    LIGHTS_BACKLIGHT_KEYPAD = 0,
    LIGHTS_BACKLIGHT_DISPLAY,
    LIGHTS_BACKLIGHT_CLI_DISPLAY,
    LIGHTS_BACKLIGHT_NAV,
    LIGHTS_BACKLIGHT_ALL,
    LIGHTS_BACKLIGHT__NUM
} LIGHTS_BACKLIGHT_T;

/*!
 * @brief Defines the brightness levels which can be used with the backlight functions.
 *
 * Defines the brightness level which can be used with the backlight functions.  The brightness
 * can range from 0 to 100 and can be considered to be a percentage.  Not every value will change
 * the intensity of the backlight, since the physical hardware only supports small number of steps.
 * If a change is desired the step functions (eg. lights_backlightset_step()) must be used.
 */
typedef unsigned char LIGHTS_BACKLIGHT_BRIGHTNESS_T;

/*!
 * @brief Defines the brightness steps which can be used with the backlight functions.
 *
 * Defines the brightness steps which can be used with the backlight functions.   This is not a
 * percentage, but the a step which translates  directly to what the hardware can do.  This is
 * provided to allow the apps controlling the backlight control such that an actual change can be
 * seen when the step is changed, unlike the percentage values.
 */
typedef unsigned char LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T;

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/*!
 * @brief The structure is for the user space IOCTL calls which handle step and percentage functions.
 *
 * The following structure is used to pass information to the kernel from user space and vica verca.
 * The fields are used differently based on the IOCTL being used.  Please see the documentation for
 * the IOCTLS (located in power_ic.h) for the details on which are used for a specific IOCTL.
 */
typedef struct
{
    LIGHTS_BACKLIGHT_T bl_select;                /*!< @brief The backlight to update. */
    LIGHTS_BACKLIGHT_BRIGHTNESS_T bl_brightness; /*!< @brief The intensity to set with 0 being off and 100
                                                      being full intensity.  */
    LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T bl_step;  /*!< @brief The intensity to set with 0 being off and
                                                      the maximum being the number of steps the hardware
                                                      supports. */
} LIGHTS_BACKLIGHT_IOCTL_T;

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATION
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/
LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T lights_backlightget_steps
(
    LIGHTS_BACKLIGHT_T bl_select
);

void lights_backlightset_step
(
    LIGHTS_BACKLIGHT_T bl_select,
    LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T bl_step
);

LIGHTS_BACKLIGHT_BRIGHTNESS_T lights_backlights_step_to_percentage
(
    LIGHTS_BACKLIGHT_T bl_select,
    LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T bl_step
);

void lights_backlightset
(
    LIGHTS_BACKLIGHT_T bl_select,
    LIGHTS_BACKLIGHT_BRIGHTNESS_T bl_brightness
);

LIGHTS_BACKLIGHT_BRIGHTNESS_STEP_T lights_backlightget_steps_val
(
    LIGHTS_BACKLIGHT_T bl_select
);

LIGHTS_BACKLIGHT_BRIGHTNESS_T lights_backlightget_val
(
   LIGHTS_BACKLIGHT_T bl_select
);

#endif /* __LIGHTS_BACKLIGHTS_H__ */
