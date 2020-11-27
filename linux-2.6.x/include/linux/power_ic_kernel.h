/*
 * Copyright (C) 2006-2008 Motorola, Inc.
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
 * Motorola 2008-Feb-20 - Remove the public APIs power_ic_atod_current_and_batt_conversion() 
 * Motorola 2008-Feb-18 - Support for external audio amplifier
 * Motorola 2008-Jan-29 - Added function to set morphing mode.
 * Motorola 2007-Jul-05 - Remove external access functions which toggle atod channel.
 * Motorola 2007-May-08 - Add turbo indicator functions.
 * Motorola 2007-Mar-21 - Handle power cut.
 * Motorola 2007-Feb-14 - Removed old functions
 * Motorola 2007-Jan-08 - Update copyright
 * Motorola 2006-Nov-15 - Fix compiling problem for CONFIG_MOT_KEYPAD turned off
 * Motorola 2006-Nov-01 - Update keyv interfaces
 * Motorola 2006-Oct-11 - New Audio interfaces.
 * Motorola 2006-Oct-04 - Initial Creation
 */

#ifndef __POWER_IC_KERNEL_H__
#define __POWER_IC_KERNEL_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__

 /*!
 * @file power_ic_kernel.h
 *
 * @brief This file contains defines, types, enums, macros, functions, etc. for the kernel.
 *
 * @ingroup poweric_core
 *
 * This file contains the following information:
 * - Kernel-Space Interface:
 *   - @ref kernel_reg_macros          "Register Macros"
 *   - @ref kernel_enums               "Enumerations"
 *   - @ref kernel_types               "Types"
 *   - @ref kernel_funcs_accy          "Accessory Interface functions"
 *   - @ref kernel_funcs_atod          "AtoD functions"
 *   - @ref kernel_funcs_audio         "Audio functions"
 *   - @ref kernel_funcs_backup_memory "Backup Memory API functions"
 *   - @ref kernel_funcs_cap_touch     "Capacitive Touch Control functions"
 *   - @ref kernel_funcs_charger       "Charger Control functions"
 *   - @ref kernel_funcs_event         "Event functions"
 *   - @ref kernel_funcs_funlights     "Funlight Control functions"
 *   - @ref kernel_funcs_keypad        "Keypad Control functions"
 *   - @ref kernel_funcs_periph        "Peripheral functions"
 *   - @ref kernel_funcs_pwr_mgmt      "Power Management functions"
 *   - @ref kernel_funcs_reg           "Register Access functions"
 *   - @ref kernel_funcs_rtc           "RTC functions"
 *   - @ref kernel_funcs_touchscreen   "Touchscreen functions"
 *   - @ref kernel_funcs_usb_detect    "USB detection functions"
 */

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/

#include <asm/ptrace.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/lights_funlights.h>
#include <linux/moto_accy.h>
#include <linux/power_ic.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/version.h>

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                            MACROS
==================================================================================================*/

/*!
 * @section kernel Kernel-space Interface
 */
/*!
 * @anchor kernel_reg_macros
 * @name Register Macros
 *
 * The following macros are used to identify to which device a register belongs.
 */
/* @{ */
/*! @brief Returns 1 if given register is an Atlas register */
#define POWER_IC_REGISTER_IS_ATLAS(reg) \
        (((reg) >= POWER_IC_REG_ATLAS_FIRST_REG) && ((reg) <= POWER_IC_REG_ATLAS_LAST_REG))
/* @} End of register macros.  -------------------------------------------------------------------*/

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*!
 * @anchor kernel_types
 * @name Kernel-space Types
 *
 * These types are only for use by the kernel-space interfaces.
 */
/* @{ */
/*!
 * @brief The structure below is used to pass information to the functions which are responsible for
 * enabling the LEDs for different regions.
 */
typedef struct
{
    unsigned int nData;   /* Data which can be used by the control functions to change operation
                           * based on the region. */
    void *pGeneric;       /* Pointer to an implementation defined function or data pointer. */
} LIGHTS_FL_LED_CTL_T;

/*!
 * @brief Structure used to configure the regions for different products.
 */
typedef struct
{
    bool (*pFcn)(const LIGHTS_FL_LED_CTL_T *, LIGHTS_FL_COLOR_T);
    LIGHTS_FL_LED_CTL_T xCtlData;  /* See LIGHTS_FL_LED_CTL_T for a description. */
} LIGHTS_FL_REGION_CFG_T;

/*!
 * The type of the callback functions used for events.  Function takes the
 * event number as its parameter and returns 0 if the event is NOT handled
 * by the function (and the next callback in the chain should be called) or
 * non-zero if the event is handled.
 */
typedef int (*POWER_IC_EVENT_CALLBACK_T)(POWER_IC_EVENT_T);

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATION
==================================================================================================*/
/*!
 * @brief Table used to control the different regions.
 *
 * The table is set up to easily allow different products to conditionally select different control
 * functions based on available hardware.  If flex control of the regions is necessary, it should
 * be done in the functions which are listed in this table.
 *
 * @note This is internal to the kernel driver and should never be used outside of the lights code.
 */
extern const LIGHTS_FL_REGION_CFG_T LIGHTS_FL_region_ctl_tb[LIGHTS_FL_MAX_REGIONS];

/*!
 * @brief Table to determine the number of brightness steps supported by hardware.
 *
 * Table which is indexed by the backlight select field to obtain the number of steps for
 * a specific backlight.  LIGHTS_BACKLIGHT_ALL cannot be used with this table.
 *
 * @note This is internal to the kernel driver and should never be used outside of the lights code.
 */
extern const uint8_t lights_bl_num_steps_tb[];

/*!
 * @brief Table to determine the percentage step size for backlight brightness.
 *
 * Table which is indexed by the backlight select field to obtain the percentage step size
 * for a specific backlight.  LIGHTS_BACKLIGHT_ALL cannot be used with this table.
 *
 * @note This is internal to the kernel driver and should never be used outside of the lights code.
 */
extern const uint8_t lights_bl_percentage_steps[];

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/*!
 * @anchor kernel_funcs_accy
 * @name Kernel-space Accessory Interface functions
 *
 * These functions are exported by the driver to allow other kernel-space code to control
 * various accessory operations. For more information, see the documentation for the
 * accy.c file.
 */
/* @{ */
void moto_accy_init (void);
void moto_accy_destroy (void);
void moto_accy_notify_insert (MOTO_ACCY_TYPE_T accy);
void moto_accy_notify_remove (MOTO_ACCY_TYPE_T accy);
MOTO_ACCY_MASK_T moto_accy_get_all_devices (void);
void moto_accy_set_accessory_power (MOTO_ACCY_TYPE_T device, int on_off);
/* @} End of kernel accessory functions --------------------------------------------------------- */

/*!
 * @anchor kernel_funcs_atod
 * @name Kernel-space AtoD functions
 *
 * These functions are exported by the driver to allow other kernel-space code to
 * perform AtoD conversions.  For more information, see the documentation for the
 * atod.c file.
 */
/* @{ */
int power_ic_atod_single_channel(POWER_IC_ATOD_CHANNEL_T channel, int * result);
int power_ic_atod_general_conversion(POWER_IC_ATOD_RESULT_GENERAL_CONVERSION_T * result);
/*Remove 2008-Feb-20 int power_ic_atod_current_and_batt_conversion(POWER_IC_ATOD_TIMING_T timing,
                                              int timeout_secs,
                                              POWER_IC_ATOD_CURR_POLARITY_T polarity,
                                              int * batt_result, int * curr_result);
*/
int power_ic_atod_raw_conversion(POWER_IC_ATOD_CHANNEL_T channel, int * samples, int * length);
/* @} End of kernel atod functions -------------------------------------------------------------- */

/*!
 * @anchor kernel_funcs_audio
 * @name Kernel-space Audio functions
 *
 * These functions are exported by the driver to allow other kernel-space code to
 * perform audio registers read/write.  For more information, see the documentation for the
 * audio.c file.
 */
/* @{ */
int power_ic_audio_conn_mode_set(POWER_IC_EMU_CONN_MODE_T conn_mode);
int power_ic_audio_ceramic_speaker_en(bool en_val);
int power_ic_audio_set_reg_mask_audio_rx_0(unsigned int mask, unsigned int value);
int power_ic_audio_set_reg_mask_audio_rx_1(unsigned int mask, unsigned int value);
int power_ic_audio_set_reg_mask_audio_tx(unsigned int mask, unsigned int value);
int power_ic_audio_set_reg_mask_ssi_network(unsigned int mask, unsigned int value);
int power_ic_audio_set_reg_mask_audio_codec(unsigned int mask, unsigned int value);
int power_ic_audio_set_reg_mask_audio_stereo_dac(unsigned int mask, unsigned int value);
//#if defined(CONFIG_MACH_PICO) || defined(CONFIG_MACH_XPIXL)|| defined(CONFIG_MACH_NEVIS) 
int power_ic_audio_ext_audio_amp_en(unsigned char en_val);
//#endif

/* @} End of kernel audio functions ------------------------------------------------------------- */

/*!
 * @anchor kernel_funcs_backup_memory
 * @name Kernel-space Backup Memory API functions
 *
 * These functions are exported by the driver to allow other kernel-space code to
 * have access to the Backup Memory registers.  For more information, see the
 * documentation for the backup_mem.c file.
 */
/* @{ */
int power_ic_backup_memory_read(POWER_IC_BACKUP_MEMORY_ID_T id, int* value);
int power_ic_backup_memory_write(POWER_IC_BACKUP_MEMORY_ID_T id, int value);
/* @} End of kernel Backup Memory API ------------------------------------------------------------*/

/*!
 * @anchor kernel_funcs_cap_touch
 * @name Kernel-space KeyV Control functions
 *
 * These functions are exported by the driver to allow other kernel-space code to control
 * KeyV keys.  For more information, see the documentation for the
 * keyv.c file.
 */
/* @{ */
int keyv_prox_sensor_en(unsigned int en);
int keyv_init(void);
void keyv_exit(void);
int keyv_reset(void);
int keyv_key_en(unsigned char nKeys, ...);
/* @} End of kernel capacitive touch functions -------------------------------------------------- */

/*!
 * @anchor kernel_funcs_charger
 * @name Kernel-space Charger Control functions
 *
 * These functions are exported by the driver to allow other kernel-space code to control
 * various charging capabilities.  For more information, see the documentation for the
 * charger.c file.
 */
/* @{ */
int power_ic_charger_set_charge_voltage(int charge_voltage);
int power_ic_charger_set_charge_current(int charge_current);
int power_ic_charger_set_trickle_current(int charge_current);
int power_ic_charger_set_power_path(POWER_IC_CHARGER_POWER_PATH_T path);
int power_ic_charger_get_overvoltage(int * overvoltage);
int power_ic_charger_reset_overvoltage(void);
void power_ic_charger_set_usb_state(POWER_IC_CHARGER_CHRG_STATE_T usb_state);
POWER_IC_CHARGER_CHRG_STATE_T power_ic_charger_get_usb_state(void);
int power_ic_charger_set_power_cut(int pwr_cut);
int power_ic_charger_get_power_cut(int * pwr_cut);
/* @} End of kernel charging functions ---------------------------------------------------------- */

/*!
 * @anchor kernel_funcs_event
 * @name Kernel-space Event Handling functions
 *
 * These functions are exported by the driver to allow other kernel-space code to
 * subscribe to and manage power IC events.  For more information, see the
 * documentation for the event.c file.
 */
/* @{ */
int power_ic_event_subscribe (POWER_IC_EVENT_T event, POWER_IC_EVENT_CALLBACK_T callback);
int power_ic_event_unsubscribe (POWER_IC_EVENT_T event, POWER_IC_EVENT_CALLBACK_T callback);
int power_ic_event_unmask (POWER_IC_EVENT_T event);
int power_ic_event_mask (POWER_IC_EVENT_T event);
int power_ic_event_clear (POWER_IC_EVENT_T event);
int power_ic_event_sense_read (POWER_IC_EVENT_T event);
irqreturn_t power_ic_irq_handler (int irq, void *dev_id, struct pt_regs *regs);
/* @} End of kernel event functions --------------------------------------------------------------*/

/*!
 * @anchor kernel_funcs_funlights
 * @name Kernel-space Funlights Control functions
 *
 * These functions are exported by the driver to allow other kernel-space code to control
 * the funlights. For more information, see the documentation for the
 * lights_funlights.c file.
 */
/* @{ */
#if defined(CONFIG_MACH_PICO) || defined(CONFIG_MACH_XPIXL)|| defined(CONFIG_MACH_NEVIS) 
extern bool lights_funlights_set_morphing_mode(MORPHING_MODE_E modeId);
#endif
extern int lights_funlights_enable_light_sensor(int light_enable);
extern int lights_funlights_light_sensor_atod(void);
extern LIGHTS_FL_REGION_MSK_T lights_fl_update   
(
    LIGHTS_FL_APP_CTL_T nApp,            
    unsigned char nRegions,
    ...
);
extern int lights_ioctl(unsigned int cmd, unsigned long arg);
extern LIGHTS_FL_COLOR_T lights_fl_get_region_color(LIGHTS_FL_APP_CTL_T nApp,
                                                    LIGHTS_FL_REGION_T nRegion);
extern int lights_enter_turbo_mode(void);
extern int lights_exit_turbo_mode(void);
/* @} End of kernel funlights functions ---------------------------------------------------------- */

/*!
 * @anchor kernel_funcs_keypad
 * @name Kernel-space Keypad Control functions
 *
 * These functions are exported by the driver to allow other kernel-space code to control
 * the keypad.
 */
/* @{ */
/* called to register a key event notification callback */
extern void* register_keyevent_callback (void (*)(unsigned short, void*), void*);

/* called to unregister a key event notification callback */
extern void unregister_keyevent_callback (void*);

#if !defined(CONFIG_MOT_KEYPAD)
/* dummy macros for LJ CONFIG_MOT_KEYPAD turned off */
#define generate_key_event(keycode, up_down)
#else
/* called when a key event needs to be generated via software */
extern int generate_key_event(unsigned short keycode, unsigned short up_down);
#endif
/* @} End of kernel keypad functions -------------------------------------------------------- */

/*!
 * @anchor kernel_funcs_periph
 * @name Kernel-space Peripheral functions
 *
 * These functions are exported by the driver to allow other kernel-space code to
 * control power IC-related peripherals.  For more information, see the documentation
 * for the peripheral.c file.
 */
/* @{ */
int power_ic_periph_set_vibrator_on(int on);
int power_ic_periph_is_usb_cable_connected (void);
int power_ic_periph_is_usb_pull_up_enabled (void);
void power_ic_periph_set_usb_pull_up (int on);
int power_ic_periph_set_sim_voltage(unsigned char sim_card_num, POWER_IC_SIM_VOLTAGE_T volt);
int power_ic_periph_set_camera_on(int on);
/* @} End of kernel peripheral functions -------------------------------------------------------- */

/*!
 * @anchor kernel_funcs_pwr_mgmt
 * @name Kernel-space Power Management functions
 *
 * These functions are exported by the driver to allow other kernel-space code to
 * perform power management functions. For more information, see the documentation
 * for the power_management.c file.
 */
/* @{ */
int power_ic_set_transflash_voltage(unsigned int millivolts);
/* @} End of power management functions.  --------------------------------------------------------*/

/*!
 * @anchor kernel_funcs_reg
 * @name Kernel-space Register Access functions
 *
 * These functions are exported by the driver to allow other kernel-space code to
 * have register-level access to the power IC.  For more information, see the
 * documentation for the external.c file.
 */
/* @{ */
int power_ic_read_reg(POWER_IC_REGISTER_T reg, unsigned int *reg_value);
int power_ic_write_reg(POWER_IC_REGISTER_T reg, unsigned int *reg_value);
int power_ic_write_reg_value(POWER_IC_REGISTER_T reg, unsigned int reg_value);
int power_ic_set_reg_value(POWER_IC_REGISTER_T reg, int index, int value, int nb_bits);
int power_ic_get_reg_value(POWER_IC_REGISTER_T reg, int index, int *value, int nb_bits);
int power_ic_set_reg_mask(POWER_IC_REGISTER_T reg, int mask, int value);
int power_ic_set_reg_bit(POWER_IC_REGISTER_T reg, int index, int value);
/* @} End of kernel register access functions ----------------------------------------------------*/

/*!
 * @anchor kernel_funcs_rtc
 * @name Kernel-space RTC functions
 *
 * These functions are exported by the driver to allow other kernel-space code to
 * access power IC real-time clock information.  For more information, see the
 * documentation for the rtc.c file.
 */
/* @{ */
int power_ic_get_num_power_cuts(int * data);
int power_ic_rtc_set_time(struct timeval * power_ic_time);
int power_ic_rtc_get_time(struct timeval * power_ic_time);
int power_ic_rtc_set_time_alarm(struct timeval * power_ic_time);
int power_ic_rtc_get_time_alarm(struct timeval * power_ic_time);
/* @} End of kernel rtc functions --------------------------------------------------------------- */

/*!
 * @anchor kernel_funcs_usb_detect
 * @name Kernel-space USB detection functions
 *
 * These functions are exported by the driver to allow other kernel-space code to
 * perform USB detection functions. For more information, see the documentation
 * for the usb_detection.c file.
 */
 
/* @{ */
int power_ic_is_charger_attached(void);
/* @} End of usb detection functions.  ------------------------------------------------------------*/

#endif /* KERNEL */

#ifdef __cplusplus
}
#endif

#endif /* __POWER_IC_KERNEL_H__ */

