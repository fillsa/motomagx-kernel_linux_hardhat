/*
 * Copyright (C) 2005-2008 Motorola, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Motorola 2008-Nov-17 - Work around for msleep
 * Motorola 2008-Sep-25 - Fix the ramp up/down on nevis and disable adaptive boost in DSM
 * Motorola 2008-Jul-07 - Change keypad current level of Nevis to 6MA
 * Motorola 2008-Apr-29 - Support keypad light ramping up and down
 * Motorola 2008-Mar-28 - Fix the ABMODE bits
 * Motorola 2008-Mar-03 - Change brightness
 * Motorola 2008-Mar-11 - Added handling of MORPHING_MODE_PHONE_WITHOUT_REVIEW.
 * Motorola 2008-Feb-25 - Change brightness and made it possible to turn on/off keypad 
 *                                     and display backlight individually
 * Motorola 2008-Feb-18 - Support Nevis Lighting
 * Motorola 2008-Jan-29 - Add xPIXL morphing mode support
 * Motorola 2008-Jan-10 - Make the share LED working on keypad
 * Motorola 2007-Mar-30 - Privacy LED test command not working for MAX6946.
 * Motorola 2007-Mar-09 - Update LIGHTS_FL_region_ctl_tb for Marco
 * Motorola 2007-Feb-19 - Use HWCFG to determine which chip is present.
 * Motorola 2007-Jan-23 - Add privacy brightness support for MAX6946 LED
 * Motorola 2007-Jan-16 - Add support for MAX6946 LED controller chip.
 * Motorola 2006-Nov-10 - Add support for Marco.
 * Motorola 2006-Oct-19 - Add one more step for display
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Oct-02 - Add lighting support on Elba
 * Motorola 2006-Sep-26 - Add support for MAX7314 LED controller chip.
 * Motorola 2006-Sep-07 - Add supoort for Saipan keypad
 * Motorola 2006-Sep-01 - Correct back light brightness in Lido
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-26 - Support ambient light sensor
 * Motorola 2006-Jul-14 - Add Lido & Saipan Support
 * Motorola 2006-Jun-28 - Fix camera flash issue
 * Motorola 2006-May-22 - Add LED enable control
 * Motorola 2006-May-15 - Fix typecasting warning.
 * Motorola 2006-Apr-12 - Added new interface functions to control backlight brightness.
 * Motorola 2006-Apr-07 - Add new region for EL NAV Keypad
 * Motorola 2006-Apr-07 - Change camera torch GPIO for Ascension P2
 * Motorola 2006-Apr-04 - Move all GPIO functionality to gpio.c
 * Motorola 2006-Jan-12 - Add in Main Display, EL Keypad, and Camera functionality 
 *                        for Ascension
 * Motorola 2005-Nov-15 - Rewrote the software.
 * Motorola 2005-Jun-16 - Updated the region table for new hardware.
 */

/*!
 * @file lights_funlights_atlas.c
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
#include <linux/delay.h>
#include <asm/io.h>      

#include "emu.h"
#include "../core/fl_register.h"
#include "../core/gpio.h"
#include "../core/os_independent.h"
#include "../core/event.h"
/*******************************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************************/
static bool lights_fl_region_gpio(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nStep);
static bool lights_fl_region_cli_display(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nStep);
static bool lights_fl_region_tri_color(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nColor);
static bool lights_fl_region_tri_color_maxim(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nColor);
static bool lights_fl_region_tri_color_max6946(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nColor);
static bool lights_fl_region_tri_color_max7314(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nColor);
static bool lights_fl_region_sol_led(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nColor); 
static bool lights_fl_region_main_display(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nStep);
static bool lights_fl_region_main_display_maxim(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nStep);
static bool lights_fl_region_main_display_max6946(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nStep);
static bool lights_fl_region_main_display_max7314(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nStep);
static bool lights_fl_region_main_cli_display(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nStep);
#ifdef CONFIG_MACH_NEVIS
static bool lights_fl_region_keypad_nevis(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nStep);
#else
static bool lights_fl_region_keypad(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nStep);
#endif
#ifdef CONFIG_MACH_XPIXL
static int lights_fl_region_keypad_set_section(LIGHTS_FL_COLOR_T nStep);
static bool lights_fl_region_ledkp(const LIGHTS_FL_LED_CTL_T *pCtlData, LIGHTS_FL_COLOR_T nColor); 
static int lights_fl_config_atlas_led(POWER_IC_REGISTER_T reg, LIGHTS_FL_COLOR_T nStep);
#endif /* CONFIG_MACH_XPIXL */

/*******************************************************************************************
 * LOCAL CONSTANTS
 ******************************************************************************************/
/*!
 * @name Number of Morphing LED segments supported
 */
/*@{*/
enum
{
    LIGHTS_FL_LEDR1_NUMBER=0,
    LIGHTS_FL_LEDR2_NAVI,
    LIGHTS_FL_LEDR3_TRASH,
    LIGHTS_FL_LEDG1_SHARE,
    LIGHTS_FL_LEDG2_REVIEW, //	LIGHTS_FL_LEDG2_KODAK,
    LIGHTS_FL_LEDG3_NUMBER_A,
    LIGHTS_FL_LEDB2_CAPTURE_PLAYBACK,
    LIGHTS_FL_LEDB3_NUMBER_B,
    LIGHTS_FL_TOTAL_LED_SEGMENT
};
/*@}*/

/*!
 * @name Define Morphing Data Structure
 * Since MORPHING_MODE_E contains negative & unused values,
 * define MORPHING_MODE_E in struct results in Klocwork Error.
 * Must check for invalid morphing mode value before updating mode in struct!!!!
 */
/*@{*/
typedef struct
{
    int mode;
    bool fade_morphing_keys;
    bool is_keypad_leds_turned_on;
    LIGHTS_FL_COLOR_T nStep;
} MORPHING_DATA_T;

/*!
 * @name Define Morphing Segment structure
 */
/*@{*/
typedef struct
{
    unsigned int navi_on;
    unsigned int /*kodak_on*/ review_on ;
    unsigned int number_on;
    unsigned int toggle_on;
    unsigned int share_on;
    unsigned int trash_on;
} MORPHING_SEGMENT_T;

/*!@cond INTERNAL */
/*!
 * @name Number of Backlight Steps
 *
 * @brief Define the number of backlight steps which are supported for the different
 * backlights.  This value includes off, so the minimum value is 2, 1 for on 1 for off.
 */
/*@{*/
# define LIGHTS_NUM_KEYPAD_STEPS      2
# define LIGHTS_NUM_DISPLAY_STEPS     8
# define LIGHTS_NUM_CLI_DISPLAY_STEPS 2
# define LIGHTS_NUM_NAV_KEYPAD_STEPS  2
/*@}*/
/*!@endcond */

/* ATLAS Registers */

/*!
 * @name Register LED Control 0
 */
/*@{*/
#define LIGHTS_FL_CHRG_LED_EN                18
#define LIGHTS_FL_LEDEN                      0
/*@}*/

/*!
 * @name Register LED Control 2
 */
/*@{*/
#define LIGHTS_FL_MAIN_DISP_DC_MASK          0x001E00
#define LIGHTS_FL_MAIN_DISP_CUR_MASK         0x000007
#define LIGHTS_FL_CLI_DISP_DC_MASK           0x01E000
#define LIGHTS_FL_CLI_DISP_CUR_MASK          0x000038
#define LIGHTS_FL_KEYPAD_DC_MASK             0x1E01C0
#define LIGHTS_FL_XPIXL_PRIVACY_DC_MASK      0x1E0140
/*@}*/

/*!
 * @name Register LED Control 3
 */
/*@{*/
#define LIGHTS_FL_TRI_COLOR_RED_DC_INDEX     6
#define LIGHTS_FL_TRI_COLOR_RED_DC_MASK      0x0007C0
#define LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX   11
#define LIGHTS_FL_TRI_COLOR_GREEN_DC_MASK    0x00F800
#define LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX    16
#define LIGHTS_FL_TRI_COLOR_BLUE_DC_MASK     0x1F0000
#define LIGHTS_FL_TRI_COLOR_RED_CL_INDEX     0
#define LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX   2
#define LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX    4
#define LIGHTS_FL_CLI_DISP_CUR_SHFT          3
#define LIGHTS_FL_MAIN_DISP_DC_SHIFT         9
#define LIGHTS_FL_CLI_DISP_DC_SHIFT          13
/*@}*/

/*!
 * @name Register LED Control tri-color mask
 */
/*@{*/
#define LIGHTS_FL_TRI_COLOR_DC_MASK          0x1FFFC0
#define LIGHTS_FL_TRI_COLOR_NUM_STEP         32
/*@}*/

/*!
 * @name Register LED Control 5
 */
/*@{*/
#define LIGHTS_FL_DUTY_CYCLE_SHIFT           3
/*@}*/

/*!
 * @bits GPO1EN and GPO1STBY in register PWR MISC 
 */
/*@{*/
#define LIGHTS_SENSOR_ENABLE_MASK            0x0000C0
/*@}*/

/*!
 * @name Defines for morphing LED ON/OFF settings
 */
/*@{*/
#define SET_INACTIVE                                    0x000000
#define SET_ACTIVE                                      0xFFFFFF
/*@}*/

/*!
 * @name Bitmask to change morphing leds
 */
/*@{*/
#define LIGHTS_FL_KEYPAD_C1_MASK                        0x03FFDB
#define LIGHTS_FL_KEYPAD_C3_MASK                        0x00FFCF
#define LIGHTS_FL_KEYPAD_C4_MASK                        0x1FFFFF
#define LIGHTS_FL_KEYPAD_C5_MASK                        0x1FFFFF
/*@}*/

/*!
 * @name Bits to ramp up/down given leds
 */
/*@{*/
#define LIGHTS_FL_LEDR1_NUMBER_RAMPUP                   0x000001
#define LIGHTS_FL_LEDR1_NUMBER_RAMPDOWN                 0x000008
#define LIGHTS_FL_LEDR2_NAVI_RAMPUP                     0x000040
#define LIGHTS_FL_LEDR2_NAVI_RAMPDOWN                   0x000200
#define LIGHTS_FL_LEDR3_TRASH_RAMPUP                    0x001000
#define LIGHTS_FL_LEDR3_TRASH_RAMPDOWN                  0x008000
#define LIGHTS_FL_LEDG1_SHARE_RAMPUP                    0x000002
#define LIGHTS_FL_LEDG1_SHARE_RAMPDOWN                  0x000010
//#define LIGHTS_FL_LEDG2_KODAK_RAMPUP                    0x000080
//#define LIGHTS_FL_LEDG2_KODAK_RAMPDOWN                  0x000400
#define LIGHTS_FL_LEDG2_REVIEW_RAMPUP                   0x000080
#define LIGHTS_FL_LEDG2_REVIEW_RAMPDOWN                 0x000400
#define LIGHTS_FL_LEDG3_NUMBER_A_RAMPUP                 0x002000
#define LIGHTS_FL_LEDG3_NUMBER_A_RAMPDOWN               0x010000
#define LIGHTS_FL_LEDB2_CAPTURE_PLAYBACK_RAMPUP         0x000100
#define LIGHTS_FL_LEDB2_CAPTURE_PLAYBACK_RAMPDOWN       0x000800
#define LIGHTS_FL_LEDB3_NUMBER_B_RAMPUP                 0x004000
#define LIGHTS_FL_LEDB3_NUMBER_B_RAMPDOWN               0x020000
/*@}*/

/* TCMD Test Modes */
#define TCMD_TEST_MODE_OFF 0
#define TCMD_TEST_MODE_ON  1

/* Defines the brightness of main and cli display.*/
const static struct
{
    unsigned char duty_cycle;
    unsigned char current_lev;
} atlas_brightness_tb[LIGHTS_NUM_DISPLAY_STEPS] =
#if defined(CONFIG_MACH_ELBA)       
{
    {0x00, 0x00},
    {0x02, 0x03},
    {0x06, 0x03},
    {0x0B, 0x03},
    {0x09, 0x05},
    {0x0A, 0x06},
    {0x0C, 0x06},
    {0x0F, 0x06}
};
#else
{
    {0x00,0x00}, /* Off   */
    {0x0F,0x01}, /* 3 mA  */
    {0x0F,0x02}, /* 6 mA  */
    {0x0F,0x03}, /* 9 mA  */
    {0x0F,0x04}, /* 12 mA */
    {0x0F,0x05}, /* 15 mA */
    {0x0F,0x06}, /* 18 mA */
    {0x0F,0x07}  /* 21 mA */
};
#endif

/*!
 * @name Maxim MAX7314 defines
 */
/*@{*/
#define MAX7314_BLUETOOTH_MASK  0x01
#define MAX7314_PRIVACY_MASK    0x02
#define MAX7314_CHARGING_MASK   0x04
/*@}*/

#ifdef CONFIG_MOT_FEAT_GPIO_API_LCD
/* PWMSAR is defined in ../../../arch/arm/mach-mxc91231/mot-gpio/mot-gpio-scma11.h
/* and used in xPIXL baseline only.  */
#define PWMSAR              IO_ADDRESS(PWM_BASE_ADDR + 0x0c)
#endif 

#ifdef CONFIG_MACH_NEVIS
/*!
 * @name Adaptive boost defines
 */
/*@{*/
#define ATLAS_BOOST_ENABLE_INDEX    10
#define ATLAS_BOOST_NUM_BITS        6
#define ATLAS_BOOST_FULL_ON         0x1C
#define ATLAS_BOOST_PARTIAL_ON      0x19
#define ATLAS_BOOST_OFF           0
/*@}*/

/*!
 * @name Nevis Keypad defines
 */
/*@{*/
#define NEVIS_KEYPAD_DC_CL  0x1E010
#define NEVIS_KEYPAD_RAMP_BOOST_INDEX 0
#define NEVIS_KEYPAD_RAMP_BOOST_NUM_BITS 16
#define NEVIS_KEYPAD_RAMP_BOOST_UP    0x7405
#define NEVIS_KEYPAD_RAMP_BOOST_RAMP_UP_CLEAR  0x007401
#define NEVIS_KEYPAD_RAMP_BOOST_RAMP_DOWN      0x007421
#define NEVIS_KEYPAD_RAMPING_NUM_BITS 3
#define NEVIS_KEYPAD_RAMPING_UP_INDEX 1
#define NEVIS_KEYPAD_RAMPING_DOWN_INDEX  4
#define NEVIS_KEYPAD_RAMPING_ENABLE   0x02
/*@}*/

/*!
 * @name OFF value for duty cycle and current level
 */
#define DC_CL_OFF  0x000010
#endif

/*******************************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************************/
/*
 * Table to determine the number of brightness steps supported by hardware.  See
 * lights_backlights.h for more details.
 */
const uint8_t lights_bl_num_steps_tb[LIGHTS_BACKLIGHT_ALL] =
{
    LIGHTS_NUM_KEYPAD_STEPS,      /* LIGHTS_BACKLIGHT_KEYPAD      */
    LIGHTS_NUM_DISPLAY_STEPS,     /* LIGHTS_BACKLIGHT_DISPLAY     */
    LIGHTS_NUM_CLI_DISPLAY_STEPS, /* LIGHTS_BACKLIGHT_CLI_DISPLAY */
    LIGHTS_NUM_NAV_KEYPAD_STEPS   /* LIGHTS_BACKLIGHT_NAV         */
};

/*
 * Table to determine the percentage step size for backlight brightness. See lights_backlights.h
 * for more details.
 */
const uint8_t lights_bl_percentage_steps[LIGHTS_BACKLIGHT_ALL] =
{
    100/(LIGHTS_NUM_KEYPAD_STEPS-1),      /* LIGHTS_BACKLIGHT_KEYPAD      */
    100/(LIGHTS_NUM_DISPLAY_STEPS-1),     /* LIGHTS_BACKLIGHT_DISPLAY     */
    100/(LIGHTS_NUM_CLI_DISPLAY_STEPS-1), /* LIGHTS_BACKLIGHT_CLI_DISPLAY */
    100/(LIGHTS_NUM_NAV_KEYPAD_STEPS-1)   /* LIGHTS_BACKLIGHT_NAV         */
};

/*
 * Table of control functions for region LED's.  See lights_funlights.h for more details.
 */
const LIGHTS_FL_REGION_CFG_T LIGHTS_FL_region_ctl_tb[LIGHTS_FL_MAX_REGIONS] =
{
#if defined(CONFIG_MACH_ASCENSION)
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_camera_flash}}, /* Camera Flash */
    {(void*)lights_fl_region_main_display,         {0, NULL}}, /* Display Backlight */
    {(void*)lights_fl_region_cli_display,          {0, NULL}}, /* CLI Display Backlight */
    { NULL,                                        {0, NULL}}, /* Motorola Logo */
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_keypad_slider}},  /* Navigation Keypad Backlight */
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_keypad_base}}, /* Keypad Backlight */
    {(void*)lights_fl_region_tri_color,            {/*1*/3, NULL}}, /* Bluetooth Status LED */
    {(void*)lights_fl_region_sol_led,              {0, NULL}}, /* SOL */
    { NULL,                                        {0, NULL}}, /* Privacy LED */
    {(void*)lights_fl_region_tri_color,            {1, NULL}}, /* Tri Color LED #1 */
    {(void*)lights_fl_region_tri_color,            {2, NULL}}, /* Tri Color LED #2 */
    { NULL,                                        {0, NULL}}, /* WiFi Status LED */

#elif defined(CONFIG_MACH_XPIXL)
    {NULL,                                         {0, NULL}}, /* Camera Flash */
    {(void*)lights_fl_region_main_display,         {0, NULL}}, /* Display Backlight */
    {NULL,                                         {0, NULL}}, /* CLI Display Backlight */
    {NULL,                                         {0, NULL}}, /* Motorola Logo */
    {NULL,                                         {0, NULL}}, /* Navigation Keypad Backlight */
    {(void*)lights_fl_region_keypad,               {0, NULL}}, /* Keypad Backlight */
    {(void*)lights_fl_region_tri_color,            {3, NULL}}, /* Bluetooth Status LED */
    {(void*)lights_fl_region_sol_led,              {0, NULL}}, /* SOL */
    {(void*)lights_fl_region_ledkp,                {0, NULL}}, /* Privacy LED */
    {NULL,                                         {0, NULL}}, /* Tri Color LED #1 */
    {NULL,                                         {0, NULL}}, /* Tri Color LED #2 */
    {NULL,                                         {0, NULL}}, /* WiFi Status LED */

#elif defined(CONFIG_MACH_MARCO)
    {NULL,                                         {0, NULL}}, /* Camera Flash */
    {(void*)lights_fl_region_main_display,         {0, NULL}}, /* Display Backlight */
    {(void*)lights_fl_region_cli_display,          {0, NULL}}, /* CLI Display Backlight */
    {NULL,                                         {0, NULL}}, /* Motorola Logo */
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_keypad_base}},  /* Navigation Keypad Backlight */
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_keypad_base}}, /* Keypad Backlight */
    {(void*)lights_fl_region_tri_color,            {/*1*/3, NULL}}, /* Bluetooth Status LED */
    {(void*)lights_fl_region_sol_led,              {0, NULL}}, /* SOL */
    {NULL,                                         {/*0*/1, NULL}}, /* Privacy LED */
    {NULL,                                         {/*1*/0, NULL}}, /* Tri Color LED #1 */
    {NULL,                                         {/*2*/0, NULL}}, /* Tri Color LED #2 */
    {NULL,                                         {0, NULL}}, /* WiFi Status LED */

#elif defined(CONFIG_MACH_NEVIS)
    {NULL,                                         {0, NULL}}, /* Camera Flash */
    {(void*)lights_fl_region_main_display,         {0, NULL}}, /* Display Backlight */
    {NULL,                                         {0, NULL}}, /* CLI Display Backlight */
    {NULL,                                         {0, NULL}}, /* Motorola Logo */
    {NULL,                                         {0, NULL}}, /* Navigation Keypad Backlight */
    {(void*)lights_fl_region_keypad_nevis,         {0, NULL}}, /* Keypad Backlight */
    {(void*)lights_fl_region_tri_color,            {3, NULL}}, /* Bluetooth Status LED */
    {(void*)lights_fl_region_tri_color,            {2, NULL}}, /* SOL */
    {NULL,                                         {1, NULL}}, /* Privacy LED */
    {NULL,                                         {0, NULL}}, /* Tri Color LED #1 */
    {NULL,                                         {0, NULL}}, /* Tri Color LED #2 */
    {NULL,                                         {0, NULL}}, /* WiFi Status LED */

#elif defined(CONFIG_MACH_LIDO)
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_camera_flash}}, /* Camera Flash */
    {(void*)lights_fl_region_main_cli_display,     {0, NULL}}, /* Display Backlight */
    {(void*)lights_fl_region_main_cli_display,     {0, NULL}}, /* CLI Display Backlight */
    { NULL,                                        {0, NULL}}, /* Motorola Logo */
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_keypad_slider}},  /* Navigation Keypad Backlight */
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_keypad_base}}, /* Keypad Backlight */
    {(void*)lights_fl_region_tri_color,            {/*1*/3, NULL}}, /* Bluetooth Status LED */
    {(void*)lights_fl_region_sol_led,              {0, NULL}},  /* SOL */
    {NULL,                                         {0, NULL}}, /* Privacy LED */
    {(void*)lights_fl_region_tri_color,            {1, NULL}}, /* Tri Color LED #1 */
    {(void*)lights_fl_region_tri_color,            {2, NULL}}, /* Tri Color LED #2 */
    {NULL,                                         {0, NULL}}, /* WiFi Status LED */    

#elif defined(CONFIG_MACH_SAIPAN)
    {NULL,                                         {0, NULL}}, /* Camera Flash */
    {(void*)lights_fl_region_main_display,         {0, NULL}}, /* Display Backlight */
    {(void*)lights_fl_region_cli_display,          {0, NULL}}, /* CLI Display Backlight */
    {NULL,                                         {0, NULL}}, /* Motorola Logo */
    {NULL,                                         {0, NULL}}, /* Navigation Keypad Backlight */
    {(void*)lights_fl_region_keypad,               {0, NULL}}, /* Keypad Backlight */
    {(void*)lights_fl_region_tri_color,            {/*1*/3, NULL}}, /* Bluetooth Status LED */
    {(void*)lights_fl_region_sol_led,              {0, NULL}}, /* SOL */
    {NULL,                                         {0, NULL}}, /* Privacy LED */
    {(void*)lights_fl_region_tri_color,            {1, NULL}}, /* Tri Color LED #1 */
    {(void*)lights_fl_region_tri_color,            {2, NULL}}, /* Tri Color LED #2 */
    {NULL,                                         {0, NULL}}, /* WiFi Status LED */

#elif defined(CONFIG_MACH_ELBA)
    {NULL,                                         {0, NULL}}, /* Camera Flash */
    {(void*)lights_fl_region_main_cli_display,     {0, NULL}}, /* Display Backlight */
    {NULL,                                         {0, NULL}}, /* CLI Display Backlight */
    {NULL,                                         {0, NULL}}, /* Motorola Logo */
    {NULL,                                         {0, NULL}}, /* Navigation Keypad Backlight */
    {NULL,                                         {0, NULL}}, /* Keypad Backlight */
    {NULL,                                         {/*1*/0, NULL}}, /* Bluetooth Status LED */
    {NULL,                                         {0, NULL}}, /* SOL */
    {NULL,                                         {0, NULL}}, /* Privacy LED */
    {NULL,                                         {0, NULL}}, /* Tri Color LED #1 */
    {NULL,                                         {0, NULL}}, /* Tri Color LED #2 */      
    {NULL,                                         {0, NULL}}, /* WiFi Status LED */   

#elif defined(CONFIG_MACH_PICO)
    {NULL,                                         {0, NULL}}, /* Camera Flash */
    {(void*)lights_fl_region_main_display_maxim,   {0, NULL}}, /* Display Backlight */
    {NULL,                                         {0, NULL}}, /* CLI Display Backlight */
    {NULL,                                         {0, NULL}}, /* Motorola Logo */
    {NULL,                                         {0, NULL}}, /* Navigation Keypad Backlight */
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_keypad_base}}, /* Keypad Backlight */
    {(void*)lights_fl_region_tri_color_maxim,      {3, NULL}}, /* Bluetooth Status LED */
    {(void*)lights_fl_region_tri_color_maxim,      {2, NULL}}, /* SOL */
    {(void*)lights_fl_region_tri_color_maxim,      {1, NULL}}, /* Privacy LED */
    {NULL,                                         {0, NULL}}, /* Tri Color LED #1 */
    {NULL,                                         {0, NULL}}, /* Tri Color LED #2 */
    {NULL,                                         {0, NULL}}, /* WiFi Status LED */

#else
    {NULL,                                         {0, NULL}}, /* Camera Flash */
    {(void*)lights_fl_region_gpio,                 {0, power_ic_gpio_lights_set_main_display}}, /* Display Backlight */
    {(void*)lights_fl_region_cli_display,          {0, NULL}}, /* CLI Display Backlight */
    {NULL,                                         {0, NULL}}, /* Motorola Logo */
    {(void*)lights_fl_region_keypad,               {0, NULL}}, /* Navigation Keypad Backlight */
    {(void*)lights_fl_region_keypad,               {0, NULL}}, /* Keypad Backlight */
    {(void*)lights_fl_region_tri_color,            {/*1*/3, NULL}}, /* Bluetooth Status LED */
    {(void*)lights_fl_region_sol_led,              {0, NULL}}, /* SOL */
    {NULL,                                         {0, NULL}}, /* Privacy LED */
    {(void*)lights_fl_region_tri_color,            {1, NULL}}, /* Tri Color LED #1 */
    {(void*)lights_fl_region_tri_color,            {2, NULL}}, /* Tri Color LED #2 */
    { NULL,                                        {0, NULL}}, /* WiFi Status LED */   
#endif
};

/*
 * Morphing Data variables
 */
static MORPHING_DATA_T morping_data = {MORPHING_MODE_PHONE, true, false, 0};

/* TCMD test mode flag */
static bool tcmd_test_mode = TCMD_TEST_MODE_OFF;

/*
 * xPIXL pwm brightness table, convert nStep to pwm(%)
 */
const unsigned char bl_pwm_brightness_tb[LIGHTS_NUM_DISPLAY_STEPS] =
{
    /* PWM is not handled by ATLAS for xPIXL. Thus the code is located here to minimize effort */
    /* PWM duty cycle = 100 is used, i.e. it corresponds to a procentage (number 100 = 100%)   */ 
    /* See settings for PWM in ./jem/hardhat/linux-2.6.x/arch/arm/mach-mxc91231/mot-gpio/mot-gpio-scma11.h */
    0, 8, 20, 30, 45, 60, 75, 100 
};

/*
 * Defines the ON/OFF settings of morphing LEDs
 */
static const MORPHING_SEGMENT_T led_onoff_tb[MORPHING_MODE_NUM] =
{
    /*  navi           kodak(review)         number        toggle        share        trash   */
    {SET_INACTIVE, SET_INACTIVE, SET_INACTIVE, SET_INACTIVE, SET_INACTIVE, SET_INACTIVE}, /* standby  */
    {SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_INACTIVE, SET_INACTIVE, SET_INACTIVE}, /* navi     */
    {SET_ACTIVE  , SET_ACTIVE  , SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_INACTIVE}, /* phone    */	
    {SET_ACTIVE  , SET_INACTIVE, SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_INACTIVE}, /* phone w/o review    */
    {SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE}, /* capture  */
    {SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_ACTIVE  , SET_ACTIVE  , SET_ACTIVE  }, /* playback */
    {SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_ACTIVE  , SET_INACTIVE, SET_ACTIVE  }, /* playback w/o share  */
    {SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_ACTIVE  , SET_ACTIVE  , SET_INACTIVE}, /* playback w/o trash  */
    {SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_INACTIVE, SET_ACTIVE  , SET_ACTIVE  }, /* playback w/o toggle */
    {SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_INACTIVE, SET_ACTIVE  , SET_INACTIVE}, /* share    */
    {SET_ACTIVE  , SET_INACTIVE, SET_INACTIVE, SET_INACTIVE, SET_INACTIVE, SET_ACTIVE  }  /* trash    */
};

/*
 * Defines the brightness of morphing LEDs
 */
const int led_bl_tb[LIGHTS_FL_TOTAL_LED_SEGMENT][LIGHTS_NUM_DISPLAY_STEPS] =
{
    { /* LIGHTS_FL_LEDR1_NUMBER */
        {(0x00 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x03 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x02 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x03 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x06 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x03 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x09 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x03 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x0D << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x03 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x12 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x03 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x18 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x03 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x1F << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x03 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)}
    },
    { /* LIGHTS_FL_LEDR2_NAVI */
        {(0x00 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x02 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x02 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x02 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x06 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x02 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x09 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x02 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x0D << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x02 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x12 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x02 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x18 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x02 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x1F << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x02 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)}
    },
    { /* LIGHTS_FL_LEDR3_TRASH */
        {(0x00 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x02 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x06 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x09 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x0D << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x12 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x18 << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)},
        {(0x1F << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_RED_CL_INDEX)}
    },
    { /* LIGHTS_FL_LEDG1_SHARE */
        {(0x00 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x02 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x06 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x09 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x0D << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x12 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x18 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x1F << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)}
    },
    { /* LIGHTS_FL_LEDG2_REVIEW(LIGHTS_FL_LEDG2_KODAK) */
        {(0x00 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x02 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x06 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x09 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x0D << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x12 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x18 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x1F << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)}
    },
    { /* LIGHTS_FL_LEDG3_NUMBER_A */
        {(0x00 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x02 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x06 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x09 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x0D << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x12 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x18 << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)},
        {(0x1F << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_GREEN_CL_INDEX)}
    },
    { /* LIGHTS_FL_LEDB2_CAPTURE_PLAYBACK */
        {(0x00 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x02 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x06 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x09 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x0D << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x12 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x18 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x1F << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x01 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)}
    },
    { /* LIGHTS_FL_LEDB3_NUMBER_B */
        {(0x00 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x02 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x06 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x09 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x0D << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x12 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x18 << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)},
        {(0x1F << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX) | (0x00 << LIGHTS_FL_TRI_COLOR_BLUE_CL_INDEX)}
    }
};

/*******************************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************************/
/*!
 * @brief Converts a color to a step
 *
 * @param color       The color of the region.
 * @param num_steps   Total number of brightness steps for light.
 *
 * @note Only non-zero RGB colors will be used when calculating the step.
 *
 * @return Brightness step [0, num_steps)
 */
static unsigned char lights_fl_color_to_step(LIGHTS_FL_COLOR_T color, unsigned int num_steps)
{
    unsigned char step = 0;
    unsigned char total_non_zero_colors = 0;
    unsigned int temp = 0;

    /* Verify number of steps is valid.  Also, only need to do something if color not black. */
    if ((num_steps > 1) && (color != LIGHTS_FL_COLOR_BLACK))
    {
        /* Average the non-zero RGB colors. */
        if (color & LIGHTS_FL_COLOR_RED)
        {
            total_non_zero_colors++;
            temp += (color & LIGHTS_FL_COLOR_RED) >> LIGHTS_FL_COLOR_RED_SFT;
        }
        if (color & LIGHTS_FL_COLOR_GREEN)
        {
            total_non_zero_colors++;
            temp += (color & LIGHTS_FL_COLOR_GREEN) >> LIGHTS_FL_COLOR_GREEN_SFT;
        }
        if (color & LIGHTS_FL_COLOR_BLUE)
        {
            total_non_zero_colors++;
            temp += (color & LIGHTS_FL_COLOR_BLUE) >> LIGHTS_FL_COLOR_BLUE_SFT;
        }
        temp = temp/total_non_zero_colors;

        /* Calculate step number.  The 255 in the calculation is the max color value. */
        step = (temp + (255/(num_steps-1)) - 1) / (255/(num_steps-1));

        if (step >= num_steps) 
        {
            step = num_steps - 1;
        }
    }

    return step;
}
/*!
 * @brief Update an LED connected to a gpio
 *
 * This function will turn on or off an LED which is connected to gpio.  The control data
 * must include the gpio function to call.
 *
 * @param  pCtlData     Pointer to the control data for the region. 
 * @param  nColor       The color to set the region to.  In this case 0 is off and non
 *                      0 is on.
 *
 * @return Always returns 0.
 */
static bool lights_fl_region_gpio
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    POWER_IC_UNUSED LIGHTS_FL_COLOR_T nColor
)    
{
    tracemsg(_k_d("=>Update gpio light to color %d"), nColor);

    if (pCtlData->pGeneric != NULL)
    {
        (*(void (*)(LIGHTS_FL_COLOR_T))(pCtlData->pGeneric))(nColor);
    }
    return true;
}

/*!
 * @brief Set main display
 *
 * Function to handle a request for the main display backlight on devices which support variable
 * backlight intensity and have display backlights controlled by either a GPIO or a power ic.
 * The backlight intensity is set in lights_led_backlights.c and converted to a hardware
 * value in this routine.
 *
 * @param   pCtlData   Pointer to the control data for the region.
 * @param   nStep      The brightness step to set the region to.
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_main_display
(
    POWER_IC_UNUSED const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nStep
)
{
    int error = 0;
    
    tracemsg(_k_d("=>Update the main display with step %d"), nStep);
    
    if (nStep >= LIGHTS_NUM_DISPLAY_STEPS)
    {
        nStep = LIGHTS_NUM_DISPLAY_STEPS-1;
    }

#ifdef CONFIG_MACH_XPIXL
    lights_funlights_set_bl_pwm_brightness(bl_pwm_brightness_tb[nStep]);
	if ((MORPHING_MODE_NUM > morping_data.mode) && (tcmd_test_mode == TCMD_TEST_MODE_OFF))
    {
        error = lights_fl_region_keypad_set_section(nStep);
    }

    tcmd_test_mode = TCMD_TEST_MODE_OFF; // Make sure TCMD test mode is disabled again
#else
#ifndef CONFIG_MACH_NEVIS
    if(nStep)
    {

        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_CLI_DISP_DC_MASK | LIGHTS_FL_CLI_DISP_CUR_MASK,
                                      0);
    }
#endif   
    error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                   LIGHTS_FL_MAIN_DISP_DC_MASK | LIGHTS_FL_MAIN_DISP_CUR_MASK,
                                   (atlas_brightness_tb[nStep].duty_cycle <<LIGHTS_FL_MAIN_DISP_DC_SHIFT)|
                                   atlas_brightness_tb[nStep].current_lev );
#ifdef CONFIG_MACH_NEVIS
    if(nStep == 0)
    {
  
        /*Disable the adaptive boost*/
        error |= power_ic_set_reg_value(POWER_IC_REG_ATLAS_LED_CONTROL_0, ATLAS_BOOST_ENABLE_INDEX, ATLAS_BOOST_OFF, ATLAS_BOOST_NUM_BITS );
    }
#endif
    
#endif /* CONFIG_MACH_XPIXL */
    return (error != 0);
}    

/*!
 * @brief Determine which Maxim chip is present and call appropriate display function
 *
 * @param   pCtlData   Pointer to the control data for the region.
 * @param   nStep      The brightness step to set the region to.
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_main_display_maxim
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nStep
)
{
    if (fl_is_chip_present(FL_CHIP_MAX6946))
    {
        return lights_fl_region_main_display_max6946(pCtlData, nStep);
    }
    else if (fl_is_chip_present(FL_CHIP_MAX7314))
    {
        return lights_fl_region_main_display_max7314(pCtlData, nStep);
    }

    /* This should not happen. */
    return false;
}

/*!
 * @brief Set main display using Maxim MAX6946 LED controller chip
 *
 * Function to handle a request for the main display backlight on devices which support variable
 * backlight intensity and have display backlights controlled by the Maxim MAX6946 LED controller chip.
 * The backlight intensity is set in lights_led_backlights.c and converted to a hardware
 * value in this routine.
 *
 * @param   pCtlData   Pointer to the control data for the region.
 * @param   nStep      The brightness step to set the region to.
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_main_display_max6946
(
    POWER_IC_UNUSED const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nStep
)
{
    const unsigned int fl_brightness_tb[LIGHTS_NUM_DISPLAY_STEPS] =
    {
        0xFF, /* High impedance: LED off */
        0x1D, /* 29 PWM duty cycle */
        0x29, /* 41 PWM duty cycle */
        0x3A, /* 58 PWM duty cycle */
        0x51, /* 81 PWM duty cycle */
        0x73, /* 115 PWM duty cycle */
        0xA3, /* 163 PWM duty cycle */
        0xE6  /* 230 PWM duty cycle */
    };
  
    tracemsg(_k_d("=>Update the main display with step %d"), nStep);
    
    if (nStep >= LIGHTS_NUM_DISPLAY_STEPS)
    {
        nStep = LIGHTS_NUM_DISPLAY_STEPS-1;
    }

    return (fl_reg_write(MAX6946_PORTS_P4_P7_OUTPUT_LEVEL_REG, fl_brightness_tb[nStep]) != 0);
}

/*!
 * @brief Set main display using Maxim MAX7314 LED controller chip
 *
 * Function to handle a request for the main display backlight on devices which support variable
 * backlight intensity and have display backlights controlled by the Maxim MAX7314 LED controller chip.
 * The backlight intensity is set in lights_led_backlights.c and converted to a hardware
 * value in this routine.
 *
 * @param   pCtlData   Pointer to the control data for the region.
 * @param   nStep      The brightness step to set the region to.
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_main_display_max7314
(
    POWER_IC_UNUSED const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nStep
)
{
    int error = 0;

    const unsigned int fl_brightness_tb[LIGHTS_NUM_DISPLAY_STEPS] =
    {
        0x00, /* duty cycle 0/16 */
        0x11, /* duty cycle 1/16 */
        0x22, /* duty cycle 2/16 */
        0x33, /* duty cycle 3/16 */
        0x55, /* duty cycle 5/16 */
        0x77, /* duty cycle 7/16 */
        0x88, /* duty cycle 8/16 */
        0x99  /* duty cycle 9/16 */
    };
  
    tracemsg(_k_d("=>Update the main display with step %d"), nStep);
    
    if (nStep >= LIGHTS_NUM_DISPLAY_STEPS)
    {
        nStep = LIGHTS_NUM_DISPLAY_STEPS-1;
    }

    if (nStep)
    {
        /* Set P12, P13, P14 and P15 to output ports. */
        error = fl_reg_write(MAX7314_PORTS_CONFIG_P15_P8_REG, 0x0F);
        /* Set blink phase 0 reg to low. */
        error |= fl_reg_write(MAX7314_BLINK_PHASE_0_OUTPUT_P15_P8_REG, 0x0F);
        /*
         * Set brightness for main display backlights. Done after setting the pins to output
         * because writes to input port registers are ignored.
         */
        error |= fl_reg_write(MAX7314_OUTPUT_INTENSITY_P13_P12_REG, fl_brightness_tb[nStep]);
        error |= fl_reg_write(MAX7314_OUTPUT_INTENSITY_P15_P14_REG, fl_brightness_tb[nStep]);
    }
    else
    {
        /* Set off brightness for main display backlights. */
        error = fl_reg_write(MAX7314_OUTPUT_INTENSITY_P13_P12_REG, fl_brightness_tb[nStep]);
        error |= fl_reg_write(MAX7314_OUTPUT_INTENSITY_P15_P14_REG, fl_brightness_tb[nStep]);

        /* Set P12, P13, P14 and P15 to input ports, this is to turn off main display backlights. */
        error |= fl_reg_write(MAX7314_PORTS_CONFIG_P15_P8_REG, 0xFF);
    } 
    return (error != 0);
}

/*!
 * @brief Set CLI display
 *
 * Function to handle a request for the cli display backlight on devices which support variable
 * backlight intensity and have CLI display backlights controlled by either a GPIO port or a power
 * IC.
 *
 * @param  pCtlData     Pointer to the control data for the region. 
 * @param  nStep        The brightness step to set the region to. 
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_cli_display
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nStep
)   
{   
    int error = 0;
    
    tracemsg(_k_d("=>Update the cli display with step %d"), nStep);
    if (nStep >= LIGHTS_NUM_CLI_DISPLAY_STEPS)
    {
        nStep = LIGHTS_NUM_CLI_DISPLAY_STEPS-1;
    }
    if(nStep)
    {
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_MAIN_DISP_DC_MASK|LIGHTS_FL_MAIN_DISP_CUR_MASK,
                                      0);
        error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                       LIGHTS_FL_CLI_DISP_DC_MASK | LIGHTS_FL_CLI_DISP_CUR_MASK,
                                       LIGHTS_FL_CLI_DISP_DC_MASK | LIGHTS_FL_CLI_DISP_CUR_MASK);
    }
    else
    {
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_CLI_DISP_DC_MASK | LIGHTS_FL_CLI_DISP_CUR_MASK,
                                      0);
    }
    return (error != 0);
}

/*!
 * @brief Set main display and cli display
 *
 * Function to handle a request for main display or CLI display backlight on devices which support
 * variable backlight intensity and have main display and CLI display backlights controlled by either
 * a GPIO or a power ic.
 *
 * @param   pCtlData   Pointer to the control data for the region.
 * @param   nStep      The brightness step to set the region to.
 *
 * @return     true  region updated
 *             false  region not updated
 */
static bool lights_fl_region_main_cli_display
(
    POWER_IC_UNUSED const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nStep
)
{
    int error = 0;
    
   
    tracemsg(_k_d("=>Update the main and cli display with step %d"), nStep);
    
    if (nStep >= LIGHTS_NUM_DISPLAY_STEPS)
    {
        nStep = LIGHTS_NUM_DISPLAY_STEPS-1;
    }
    
    error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                   LIGHTS_FL_MAIN_DISP_DC_MASK |
                                   LIGHTS_FL_CLI_DISP_DC_MASK |
                                   LIGHTS_FL_MAIN_DISP_CUR_MASK|
                                   LIGHTS_FL_CLI_DISP_CUR_MASK,
                                   (atlas_brightness_tb[nStep].duty_cycle << LIGHTS_FL_MAIN_DISP_DC_SHIFT) |
                                   (atlas_brightness_tb[nStep].duty_cycle << LIGHTS_FL_CLI_DISP_DC_SHIFT) |
                                   atlas_brightness_tb[nStep].current_lev |
                                   (atlas_brightness_tb[nStep].current_lev << LIGHTS_FL_CLI_DISP_CUR_SHFT));
    
    return (error != 0);
}

#if defined(CONFIG_MACH_NEVIS)
/*!
 * @brief Update keypad backlight for Nevis
 *
 * Turn on or off the keypad backlight for Nevis. Nevis is using Auxilary display in
 * Atlas for the keypad light.
 *
 * @param  pCtlData    Pointer to the control data for the region. 
 * @param  nStep       The brightness step to set the region to. 
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_keypad_nevis
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nStep
)    
{
    int error;
  
    tracemsg(_k_d("=>Update the keypad backlight with step %d"), nStep);

    if(nStep)
    {
       
        /*Clear the ramping down bit.*/
        error = power_ic_set_reg_value(POWER_IC_REG_ATLAS_LED_CONTROL_0,NEVIS_KEYPAD_RAMPING_DOWN_INDEX,
                                       0, NEVIS_KEYPAD_RAMPING_NUM_BITS);

        /*Turn on the keypad light*/
        error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_CLI_DISP_DC_MASK| LIGHTS_FL_CLI_DISP_CUR_MASK,
                                      NEVIS_KEYPAD_DC_CL);
        
        /*Enable the ramp up and enable adaptive boost*/
         error |= power_ic_set_reg_value(POWER_IC_REG_ATLAS_LED_CONTROL_0, NEVIS_KEYPAD_RAMP_BOOST_INDEX, 
                                         NEVIS_KEYPAD_RAMP_BOOST_UP, NEVIS_KEYPAD_RAMP_BOOST_NUM_BITS);
  
    }
    else
    {     
        /*clear the ramp up bit*/
        error |=power_ic_write_reg_value(POWER_IC_REG_ATLAS_LED_CONTROL_0, NEVIS_KEYPAD_RAMP_BOOST_RAMP_UP_CLEAR);
        
        /*Enable the ramp down*/
        error |=power_ic_write_reg_value(POWER_IC_REG_ATLAS_LED_CONTROL_0,NEVIS_KEYPAD_RAMP_BOOST_RAMP_DOWN);
        
        power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] |= POWER_IC_LIGHTS_SLEEP;
        
        msleep(200);

        power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] &= ~(POWER_IC_LIGHTS_SLEEP);
        
        error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_CLI_DISP_DC_MASK| LIGHTS_FL_CLI_DISP_CUR_MASK,
                                      DC_CL_OFF);
        power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] |= POWER_IC_LIGHTS_SLEEP;
        msleep(500);
        power_ic_pm_suspend_mask_tbl[POWER_IC_PM_MISC_SLEEPS] &= ~(POWER_IC_LIGHTS_SLEEP);
        
        error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_CLI_DISP_CUR_MASK,0);
        /* Set the ABMODE and BOOST to make sure the boost stay on*/
        error = power_ic_set_reg_value(POWER_IC_REG_ATLAS_LED_CONTROL_0,
                                       ATLAS_BOOST_ENABLE_INDEX, ATLAS_BOOST_PARTIAL_ON, ATLAS_BOOST_NUM_BITS );

    }
    return (error != 0);
}
#else

/*!
 * @brief Update keypad backlight
 *
 * Turn on or off the keypad backlight when it is connected to the power IC.
 *
 * @param  pCtlData    Pointer to the control data for the region. 
 * @param  nStep       The brightness step to set the region to. 
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_keypad
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nStep
)    
{
    int error = 0;
   
    tracemsg(_k_d("=>Update the keypad backlight with step %d"), nStep);
    if (nStep >= LIGHTS_NUM_KEYPAD_STEPS)
    {
        nStep = LIGHTS_NUM_KEYPAD_STEPS-1;
    }

#ifdef CONFIG_MACH_XPIXL
    morping_data.mode = MORPHING_MODE_PHONE;
    error = lights_fl_region_keypad_set_section(nStep);
#else
    if(nStep)
    {
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_KEYPAD_DC_MASK,
                                      LIGHTS_FL_KEYPAD_DC_MASK);
    }
    else
    {
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_KEYPAD_DC_MASK,
                                      0);
    }
#endif
    return (error != 0);
}

#endif

#if defined(CONFIG_MACH_XPIXL)
/*!
 * @brief Update keypad morphing lighting
 *
 * Turn on or off the different sections of the morphing backlight when it is connected to the power IC.
 *
 * @param  nStep       The brightness step to set the region to. 
 *
 * @return     0 if successful
 */
static int lights_fl_region_keypad_set_section
(
    LIGHTS_FL_COLOR_T nStep
)    
{
    int error = 0;
    
    tracemsg(_k_d("=>Update the keypad morphing mode with mode %d and nStep %d"), morping_data.mode, nStep);
    morping_data.nStep = nStep;
    
    if (nStep > 0)
    {
        morping_data.is_keypad_leds_turned_on = true;
    }
    else
    {
        morping_data.is_keypad_leds_turned_on = false;
    }
    
    if (morping_data.fade_morphing_keys && (nStep > 0))
    {
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_0,
                                      LIGHTS_FL_LEDEN,
                                      1);
        error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_1,
                                      LIGHTS_FL_KEYPAD_C1_MASK,
                                      (led_onoff_tb[morping_data.mode].number_on & LIGHTS_FL_LEDR1_NUMBER_RAMPUP) | 
                                      (led_onoff_tb[morping_data.mode].navi_on & LIGHTS_FL_LEDR2_NAVI_RAMPUP) | 
                                      (led_onoff_tb[morping_data.mode].trash_on & LIGHTS_FL_LEDR3_TRASH_RAMPUP) | 
                                      (led_onoff_tb[morping_data.mode].share_on & LIGHTS_FL_LEDG1_SHARE_RAMPUP) |
//                                      (led_onoff_tb[morping_data.mode].kodak_on & LIGHTS_FL_LEDG2_KODAK_RAMPUP) | 
                                      (led_onoff_tb[morping_data.mode].review_on & LIGHTS_FL_LEDG2_REVIEW_RAMPUP) |
                                      (led_onoff_tb[morping_data.mode].number_on & LIGHTS_FL_LEDG3_NUMBER_A_RAMPUP) | 
                                      (led_onoff_tb[morping_data.mode].toggle_on & LIGHTS_FL_LEDB2_CAPTURE_PLAYBACK_RAMPUP) |
                                      (led_onoff_tb[morping_data.mode].number_on & LIGHTS_FL_LEDB3_NUMBER_B_RAMPUP)); 
    }
    else if (morping_data.fade_morphing_keys)
    {
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_1,
                                      LIGHTS_FL_KEYPAD_C1_MASK,
                                      LIGHTS_FL_LEDR1_NUMBER_RAMPDOWN |
                                      LIGHTS_FL_LEDR2_NAVI_RAMPDOWN |
                                      LIGHTS_FL_LEDR3_TRASH_RAMPDOWN |
                                      LIGHTS_FL_LEDG1_SHARE_RAMPDOWN |
//                                      LIGHTS_FL_LEDG2_KODAK_RAMPDOWN |
									  LIGHTS_FL_LEDG2_REVIEW_RAMPDOWN |
                                      LIGHTS_FL_LEDG3_NUMBER_A_RAMPDOWN |
                                      LIGHTS_FL_LEDB2_CAPTURE_PLAYBACK_RAMPDOWN |
                                      LIGHTS_FL_LEDB3_NUMBER_B_RAMPDOWN);
        udelay(110);
    } 
    else
    {
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_1,
                                      LIGHTS_FL_KEYPAD_C1_MASK,
                                      SET_INACTIVE);
    }
    error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_3,
                                  LIGHTS_FL_KEYPAD_C3_MASK,
                                  ((led_onoff_tb[morping_data.mode].number_on ? led_bl_tb[LIGHTS_FL_LEDR1_NUMBER][nStep] : 0) |
                                  (led_onoff_tb[morping_data.mode].share_on ? led_bl_tb[LIGHTS_FL_LEDG1_SHARE][nStep] : 0)));

    error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_4,
                                  LIGHTS_FL_KEYPAD_C4_MASK,
                                  ((led_onoff_tb[morping_data.mode].navi_on ? led_bl_tb[LIGHTS_FL_LEDR2_NAVI][nStep] : 0) |
                                  (led_onoff_tb[morping_data.mode]./*kodak_on*/review_on ? led_bl_tb[/*LIGHTS_FL_LEDG2_KODAK*/LIGHTS_FL_LEDG2_REVIEW][nStep] : 0) |
                                  (led_onoff_tb[morping_data.mode].toggle_on ? led_bl_tb[LIGHTS_FL_LEDB2_CAPTURE_PLAYBACK][nStep] : 0)));

    error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_5,
                                  LIGHTS_FL_KEYPAD_C5_MASK,
                                  ((led_onoff_tb[morping_data.mode].trash_on ? led_bl_tb[LIGHTS_FL_LEDR3_TRASH][nStep]: 0) |
                                  (led_onoff_tb[morping_data.mode].number_on ? (led_bl_tb[LIGHTS_FL_LEDG3_NUMBER_A][nStep] |
                                  led_bl_tb[LIGHTS_FL_LEDB3_NUMBER_B][nStep]) : 0)));
    return (error != 0);    
}

/*!
 * @brief Enable or disable bottom switch of xPIXL Privacy LED (LED-KP pin).
 *
 * Enables or disables LED-KP based on the nColor parameter.  When nColor is 0
 * the led is turned off when it is non 0 it is turned on.
 *
 * @param  pCtlData     Pointer to the control data for the region. 
 * @param  nColor       The color to set the region to.
 *
 * @return true  When the region is update.
 *         false When the region was not updated due to a communication or hardware error.
 */
static bool lights_fl_region_ledkp
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nColor       
)
{
    int error;
    tracemsg(_k_d("=>Update the LED_KP pin with color %d"), nColor);

    if(nColor)
    {
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_XPIXL_PRIVACY_DC_MASK,
                                      LIGHTS_FL_XPIXL_PRIVACY_DC_MASK);
    }
    else
    {
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_2,
                                      LIGHTS_FL_XPIXL_PRIVACY_DC_MASK,
                                      0);
    }
    return (error != 0);    
}
#endif /* CONFIG_MACH_XPIXL */

/*!
 * @brief Update a tri colored LED
 *
 * Function to handle the enabling tri color leds which have the regions combined.
 *
 * @param  pCtlData     Pointer to the control data for the region. 
 * @param  nColor       The color to set the region to. 
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_tri_color 
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nColor       
)
{
    
    int error = 0;
    unsigned char step;
 
    tracemsg(_k_d("=>Update the tricolor display with ndata %d and color %d"),pCtlData->nData, nColor);
    
    step = lights_fl_color_to_step(nColor, LIGHTS_FL_TRI_COLOR_NUM_STEP);
   
    switch (pCtlData->nData)
    {
        /*Privacy*/
        case 1:
            error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_3,
                                          LIGHTS_FL_TRI_COLOR_RED_DC_MASK,
                                          (step << LIGHTS_FL_TRI_COLOR_RED_DC_INDEX)); 
            break;
        /*Charging*/   
        case 2:
           
           error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_3,
                                         LIGHTS_FL_TRI_COLOR_GREEN_DC_MASK,
                                         (step << LIGHTS_FL_TRI_COLOR_GREEN_DC_INDEX));
           break;
        /*Bluetooth*/
        case 3:
            error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_LED_CONTROL_3,
                                         LIGHTS_FL_TRI_COLOR_BLUE_DC_MASK,
                                         (step << LIGHTS_FL_TRI_COLOR_BLUE_DC_INDEX));
           break;
        default:
            break;
    }
            
    return (error != 0);
}

/*!
 * @brief Determine which Maxim chip is present and call appropriate tri color function
 *
 * @param   pCtlData   Pointer to the control data for the region.
 * @param   nStep      The brightness step to set the region to.
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_tri_color_maxim
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nColor
)
{
    if (fl_is_chip_present(FL_CHIP_MAX6946))
    {
        return lights_fl_region_tri_color_max6946(pCtlData, nColor);
    }
    else if (fl_is_chip_present(FL_CHIP_MAX7314))
    {
        return lights_fl_region_tri_color_max7314(pCtlData, nColor);
    }

    /* This should not happen. */
    return false;
}

/*!
 * @brief Update a tri colored LED using Maxim MAX6946 LED controller chip
 *
 * Function to handle the enabling tri color leds which have the regions combined.
 *
 * @param  pCtlData     Pointer to the control data for the region. 
 * @param  nColor       The color to set the region to. 
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_tri_color_max6946
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nColor       
)
{
    int error = 0;
    const unsigned int privacy_brightness_tbl[LIGHTS_FL_PRIVACY_NUM_BRIGHTNESS_LEVEL]=
    {
        0xFF,  /*LED OFF*/
        0x08,
        0x0B,
        0x11,
        0x18,
        0x23,
        0x31,
        0x46,
        0x62,
        0x92  /*Highest brightness.*/
    };
         
    tracemsg(_k_d("=>Update the tricolor led %d, color=%d"), pCtlData->nData, nColor);

    switch (pCtlData->nData)
    {
        /* Privacy */
        case 1:
            if (nColor >= LIGHTS_FL_PRIVACY_NUM_BRIGHTNESS_LEVEL)
            {
                nColor = LIGHTS_FL_PRIVACY_NUM_BRIGHTNESS_LEVEL-1;
            }
            error = fl_reg_write(MAX6946_PORT_P1_OUTPUT_LEVEL_REG, privacy_brightness_tbl[nColor]);
            break;

        /* Charging */
        case 2:
            error = fl_reg_write(MAX6946_PORT_P2_OUTPUT_LEVEL_REG, ((nColor > 0) ? 0x92 : 0xFF));
            break;

        /* Bluetooth */
        case 3:
            error = fl_reg_write(MAX6946_PORT_P0_OUTPUT_LEVEL_REG, ((nColor > 0) ? 0x92 : 0xFF));
            break;

        default:
            /* This should not happen. */
            break;
    }

    return (error != 0);
}

/*!
 * @brief Update a tri colored LED using Maxim MAX7314 LED controller chip
 *
 * Function to handle the enabling tri color leds which have the regions combined.
 *
 * @param  pCtlData     Pointer to the control data for the region. 
 * @param  nColor       The color to set the region to. 
 *
 * @return     true  region updated
 *             false region not updated
 */
static bool lights_fl_region_tri_color_max7314 
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nColor       
)
{
    static uint8_t reg_value = 0xFF;
    int error;
    
    tracemsg(_k_d("=>Update the tricolor display %d"), nColor);

    switch (pCtlData->nData)
    {
        /* Privacy */
        case 1:
            if (nColor > 0)
            {
                reg_value &= ~(MAX7314_PRIVACY_MASK);
            }
            else
            {
                reg_value |= MAX7314_PRIVACY_MASK;
            }
            break;

        /* Charging */
        case 2:
            if (nColor > 0)
            {
                reg_value &= ~(MAX7314_CHARGING_MASK);
            }
            else
            {
                reg_value |= MAX7314_CHARGING_MASK;
            }
            break;

        /* Bluetooth */
        case 3:
            if (nColor > 0)
            {
                reg_value &= ~(MAX7314_BLUETOOTH_MASK);
            }
            else
            {
                reg_value |= MAX7314_BLUETOOTH_MASK;
            }
            break;

        default:
            /* This should not happen. */
            break;
    }

    error = fl_reg_write(MAX7314_BLINK_PHASE_0_OUTPUT_P7_P0_REG, reg_value);
    return (error != 0);
}

/*!
 * @brief Enable or disable the sol LED.
 *
 * Enables or disables the sol LED based on the nColor parameter.  When nColor is 0
 * the led is turned off when it is non 0 it is turned on.
 *
 * @param  pCtlData     Pointer to the control data for the region. 
 * @param  nColor       The color to set the region to.
 *
 * @return true  When the region is update.
 *         false When the region was not updated due to a communication or hardware error.
 */
static bool lights_fl_region_sol_led
(
    const LIGHTS_FL_LED_CTL_T *pCtlData, 
    LIGHTS_FL_COLOR_T nColor       
)
{
    int error;
    tracemsg(_k_d("=>Update the sol led with color %d"), nColor);
    
    error = power_ic_set_reg_bit(POWER_IC_REG_ATLAS_CHARGER_0,LIGHTS_FL_CHRG_LED_EN,(nColor != 0));
        
    return (error != 0);
}

/*******************************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************************/
/*!
 * @brief Enable or disable the LED EN bit.
 *
 *  The LEDEN bit in Atlas is the master control bit for all the LED's. 
 *  leden will be set to FALSE if none of the LED's are enabled. Otherwise, 
 *  leden will be set to TRUE if one of more of the LED's are enabled.
 * @param  none
 *
 * @return none
 */
void lights_funlights_enable_led(void)
{
    unsigned int reg_value2;
    unsigned int reg_value3;
    unsigned int reg_value4;
    unsigned int reg_value5;
    int value;
    bool leden;
    /* Read out LED CONTROL 2 ,3,4,5 register value. */
    if((power_ic_read_reg(POWER_IC_REG_ATLAS_LED_CONTROL_2,&reg_value2))||
       (power_ic_read_reg(POWER_IC_REG_ATLAS_LED_CONTROL_3,&reg_value3))||
       (power_ic_read_reg(POWER_IC_REG_ATLAS_LED_CONTROL_4,&reg_value4))||
       (power_ic_read_reg(POWER_IC_REG_ATLAS_LED_CONTROL_5,&reg_value5)))
    {
        return;
    }
    leden = ((reg_value2 & (LIGHTS_FL_MAIN_DISP_DC_MASK |
                            LIGHTS_FL_MAIN_DISP_CUR_MASK |
                            LIGHTS_FL_CLI_DISP_DC_MASK |
                            LIGHTS_FL_KEYPAD_DC_MASK)) ||
             (reg_value3 & LIGHTS_FL_TRI_COLOR_DC_MASK)||
             (reg_value4 & LIGHTS_FL_TRI_COLOR_DC_MASK)||
             (reg_value5 & LIGHTS_FL_TRI_COLOR_DC_MASK));

    if(power_ic_get_reg_value(POWER_IC_REG_ATLAS_LED_CONTROL_0,LIGHTS_FL_LEDEN,&value,1))
    {
        return;
    }
    tracemsg(_k_d("The funlight register0 value is: %d"), value);
    /* If the state of the LEDs/Funlights is not equal to the state of the LEDEN bit
       in Atlas, then set the LEDEN bit to the state of the LEDs/Funlights */
    if(leden != value)
    {
        tracemsg(_k_d("=>Update the leden bit,leden= %d,"),leden);
        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_LED_CONTROL_0,LIGHTS_FL_LEDEN, leden ? 1 : 0);
    }
   
}

/*!
 * @brief Enable or disable the ambinet light sensor.
 *
 * The GPO1EN and GPO1STBY bits in register PWR MISC control the ambient light sensor
 * circuit. Both bits set enables the light sensor. Both bits cleared disables the sensor.
 *
 * @param  0 is disable the ambient light circuit
 *         Non 0 is enable the ambient light circuit
 *
 * @return 0 for normal case
 */
int lights_funlights_enable_light_sensor(int light_enable)
{
    tracemsg(_k_d("Start to enable the light sensor circuit"));
    return(power_ic_set_reg_mask(POWER_IC_REG_ATLAS_PWR_MISC,
                                  LIGHTS_SENSOR_ENABLE_MASK,
                                 (light_enable ? LIGHTS_SENSOR_ENABLE_MASK : 0)));
           
}

/*!
 * @brief Read ambient light sensor Atod value
 *
 * This function will read and return the light sensor atod value
 *
 * @return light sensor atod value
 * @note The circuit must be enabled by calling lights_funlights_enable_light_sensor
 *       before the value can be read.
 */       
int lights_funlights_light_sensor_atod(void)
{
    int light_sensor;
   
    if(power_ic_atod_single_channel(POWER_IC_ATOD_CHANNEL_AMBIENT_LIGHT,&light_sensor))
    {
        light_sensor = 0;
    }
    tracemsg(_k_d("light sensor atod reading value is %d"), light_sensor );
    return light_sensor;
}

#ifdef CONFIG_MACH_XPIXL
/*!
 * @brief Set the morphing mode
 *
 * This function will set the morphing mode
 *
 * @param mode    The morphing mode
 *
 * @return 0 for normal cases
 */       
bool lights_funlights_set_morphing_mode(MORPHING_MODE_E modeId)
{
    int error = 0;
    tracemsg(_k_d("set the morphing mode %d"), modeId);

    if (modeId == MORPHING_MODE_TEST)
    {
        tcmd_test_mode = TCMD_TEST_MODE_ON;
    }
    else if ((MORPHING_MODE_NUM > modeId) && (modeId > MORPHING_MODE_KEEP_CURRENT))
    {
        morping_data.mode = modeId;
        if (morping_data.is_keypad_leds_turned_on)
        {
            error = lights_fl_region_keypad_set_section(morping_data.nStep);
        }
    }
    else
    {
        error = -EINVAL;
    }
    return (error);
}
#endif /* CONFIG_MACH_XPIXL */

#ifdef CONFIG_MOT_FEAT_GPIO_API_LCD
/**
 *  Set the display backlight intensity.
 *
 *  @param  value   Intensity level; range 0-100
*/
void lights_funlights_set_bl_pwm_brightness(unsigned char value)
{
    /* Set PWM duty cycle to value */
    writel(value, PWMSAR);

    /* Sleep to make sure that PWM FIFO is ready for next write - avoids AP kernel panic */
    msleep(50);
}

/** * Get the display backlight intensity.
 *
 * @return Backlight intensity, a value between 0 and 100.
 */
char lights_funlights_get_bl_pwm_brightness(void)
{
    int value = 0;

    value = readl(PWMSAR);

    return (unsigned char)value;
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_LCD */
