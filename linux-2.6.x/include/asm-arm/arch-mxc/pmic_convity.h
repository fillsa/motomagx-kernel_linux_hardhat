/*
 * Copyright (c) 2005 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
 */

#ifndef __PMIC_CONVITY_H__
#define __PMIC_CONVITY_H__

/*!
 * @defgroup PMIC_sc55112_CONNECTIVITY sc55112 Connectivity Driver
 * @ingroup PMIC_sc55112_DRVRS
 */

/*!
 * @file pmic_convity.h
 * @brief External definitions for the PMIC Connectivity Client driver.
 *
 * The PMIC Connectivity driver and this API were developed to support the
 * external connectivity capabilities of several power management ICs that
 * are available from Freescale Semiconductor, Inc. In particular, both the
 * mc13783 and Phoenix sc55112 power management ICs are currently supported.
 * All of the common features of each power management IC are available as
 * well as all device-specific features. However, attempting to use a device-
 * specific feature on a platform on which it is not supported will simply
 * return an error status.
 *
 * The following operating modes, in terms of external connectivity, are
 * supported:
 *
 * @verbatim
       Operating Mode     mc13783   sc55112
       ---------------    -----   ----------
       USB (incl. OTG)     Yes       Yes
       RS-232              Yes       Yes
       CE-936              Yes       No

   @endverbatim
 *
 * The USB, including USB On-the-Go (OTG), and RS-232 operating modes are
 * very similar between the mc13783 and sc55112 power management ICs and
 * it is possible to configure and operate them in an identical manner.
 * However, the mc13783 hardware does support some additional USB and RS-232
 * configuration options as well as the CE-936 operating mode.
 *
 * For complete technical information concerning the Freescale power
 * management ICs, please refer to the following documents:
 *
 * [1] MC13783 Detailed Technical Specification (Level 3),
 *     Rev 1.3 - 04/09/17. Freescale Semiconductor, Inc.
 *
 * [2] iDEN Phoenix Platform Level 3 Detailed Technical Specification,
 *     Rev 2.2 11-03-2005. Freescale Semiconductor, Inc.
 *
 * For information about the USB and USB On-The-Go protocols, please refer
 * to the following documents:
 *
 * [3] Universal Serial Bus Specification, Revision 2.0, April 27, 2000,
 *     USB Implementers Forum, Inc.
 *
 * [4] On-The-Go Supplement to the USB 2.0 Specification, Revision 1.0a,
 *     June 24, 2003. USB Implementers Forum, Inc.
 *
 * @ingroup PMIC_sc55112_CONNECTIVITY
 */

#include "pmic_status.h"	/* Defines PMIC_STATUS return values. */

/***************************************************************************
 *                       TYPEDEFS AND ENUMERATIONS                         *
 ***************************************************************************/

/*!
 * @name General Setup and Configuration Typedefs and Enumerations
 * Typedefs and enumerations that are used for initial access to and
 * configuration of the PMIC Connectivity hardware.
 */
/*@{*/

#define DEBUG_CONVITY

/*!
 * @typedef PMIC_CONVITY_HANDLE
 * @brief Define typedef for a handle to the PMIC Connectivity hardware.
 *
 * Define a "handle" that is returned when the PMIC Connectivity hardware
 * is opened. This handle grants exclusive access to the PMIC Connectivity
 * hardware and must be used in all subsequent function calls. When access
 * to the PMIC Connectivity hardware is no longer required, then a close
 * operation must be done with this handle. The handle is no longer valid
 * if the close operation was successful.
 */
typedef long PMIC_CONVITY_HANDLE;

/*!
 * @enum PMIC_CONVITY_MODE
 * @brief Select the main Connectivity operating mode.
 *
 * Defines all possible PMIC Connectivity main operating modes. Only one of
 * these modes can be active at a time.
 */
typedef enum {
	USB,			/*!< Select USB mode (this is also the Reset/Default
				   mode).                                          */
	RS232,			/*!< Select RS-232 mode. for SC55112              */
	RS232_1,		/*!< Select RS-232_1 mode.                         */
	RS232_2,		/*!< Select RS-232_2 mode.                         */
	CEA936_MONO,		/*!< Select CE-936 Mono mode <b>(mc13783-only)</b>.    */
	CEA936_STEREO,		/*!< Select CE-936 Stereo mode <b>(mc13783-only)</b>.  */
	CEA936_TEST_RIGHT,	/*!< Select CE-936 Right Channel Test mode
				   <b>(mc13783-only)</b>.                            */
	CEA936_TEST_LEFT	/*!< Select CE-936 Left Channel Test mode
				   <b>(mc13783-only)</b>.                            */
} PMIC_CONVITY_MODE;

/*!
 * @enum PMIC_CONVITY_EVENTS
 * @brief Identify the connectivity events that have been detected and should
 * be handled.
 *
 * Defines all possible PMIC Connectivity events. Multiple events may be
 * selected when defining a mask.
 */
typedef enum {
	USB_DETECT_4V4_RISE = 1,	/*!< Detected 4.4V rising edge.      */
	USB_DETECT_4V4_FALL = 2,	/*!< Detected 4.4V falling edge.     */
	USB_DETECT_2V0_RISE = 4,	/*!< Detected 2.0V rising edge.      */
	USB_DETECT_2V0_FALL = 8,	/*!< Detected 2.0V falling edge.     */
	USB_DETECT_0V8_RISE = 16,	/*!< Detected 0.8V rising edge.      */
	USB_DETECT_0V8_FALL = 32,	/*!< Detected 0.8V falling edge.     */
	USB_DETECT_MINI_A = 64,	/*!< Detected USB mini A plug.       */
	USB_DETECT_MINI_B = 128,	/*!< Detected USB mini B plug.       */
	USB_DETECT_NON_USB_ACCESSORY = 256,	/*!< Detected a non-USB connection
						   <b>(mc13783-only)</b>.            */
	USB_DETECT_FACTORY_MODE = 512	/*!< Detected a factory-mode
					   connection <b>(mc13783-only)</b>. */
} PMIC_CONVITY_EVENTS;

/*!
 * @typedef PMIC_CONVITY_CALLBACK
 * @brief Typedef for PMIC Connectivity event notification callback function.
 *
 * Define a typedef for the PMIC Connectivity event notification callback
 * function. The signalled events are passed to the function as the first
 * argument. The callback function should then process whatever events it
 * can and then return the set of unhandled events (if any).
 */
typedef void (*PMIC_CONVITY_CALLBACK) (const PMIC_CONVITY_EVENTS event);

/*@}*/

/*!
 * @name USB and USB On-The-Go Mode-specific Typedefs and Enumerations
 * Typedefs and enumerations that are used only for setting up and controlling
 * the USB and USB On-The-Go modes of operation.
 */
/*@{*/

/*!
 * @enum PMIC_CONVITY_USB_DEVICE_TYPE
 * @brief Select the USB device type (either A or B).
 *
 * Defines all possible USB device types. This must match the physical
 * connector being used.
 */
typedef enum {
	USB_A_DEVICE,
	USB_B_DEVICE
} PMIC_CONVITY_USB_DEVICE_TYPE;

/*!
 * @enum PMIC_CONVITY_USB_SPEED
 * @brief Select the USB transceiver operating speed.
 *
 * Defines all possible USB transceiver operating speeds. Only one
 * speed setting may be used at a time.
 */
typedef enum {
	USB_LOW_SPEED,		/*!< Select 1.5 Mbps.              */
	USB_FULL_SPEED,		/*!< Select 12 Mbps.               */
	USB_HIGH_SPEED		/*!< Select 480 Mbps <b>(currently
				   not supported)</b>.           */
} PMIC_CONVITY_USB_SPEED;

/*!
 * @enum PMIC_CONVITY_USB_MODE
 * @brief Select the USB transceiver operating mode.
 *
 * Defines all possible USB transceiver operating modes. Only one
 * mode may be used at a time. The selected mode, in combination with
 * the USB bus speed, determines the selection of pull-up and pull-down
 * resistors.
 */
typedef enum {
	USB_HOST,
	USB_PERIPHERAL
} PMIC_CONVITY_USB_MODE;

/*!
 * @enum PMIC_CONVITY_USB_POWER_IN
 * @brief Select the USB transceiver's power regulator input source.
 *
 * Defines all possible input power sources for the USB transceiver power
 * regulator. Only one power supply source may be selected at a time.
 */
typedef enum {

	USB_POWER_INTERNAL_BOOST,	/*!< Select internal power source
					   with boost.                  */

	USB_POWER_VBUS,		/*!< Select VBUS power source.    */

	USB_POWER_INTERNAL	/*!< Select internal power source
				   <b>(mc13783-only)</b>.         */
} PMIC_CONVITY_USB_POWER_IN;

/*!
 * @enum PMIC_CONVITY_USB_POWER_OUT
 * @brief Select the USB transceiver power regulator output voltage.
 *
 * Defines all possible output voltages for the USB transceiver power
 * regulator. Only one power output voltage level may be selected at
 * a time.
 */
typedef enum {
	USB_POWER_2V775,	/*!< Select 2.775V output voltage
				   <b>(mc13783-only)</b>.         */
	USB_POWER_3V3		/*!< Select 3.3V output voltage.  */
} PMIC_CONVITY_USB_POWER_OUT;

/*!
 * @enum PMIC_CONVITY_USB_TRANSCEIVER_MODE
 * @brief Select the USB transceiver operating mode.
 *
 * Defines all valid USB transceiver operating modes. Only one of the
 * following USB transceiver modes may be selected at a time.
 */
typedef enum {
	USB_TRANSCEIVER_OFF,	/*!< USB transceiver currently off
				   <b>(mc13783-only)</b>.            */
	USB_SINGLE_ENDED_UNIDIR,	/*!< Select Single-ended
					   unidirectional transmit mode.   */
	USB_SINGLE_ENDED_UNIDIR_TX,	/*!< Select Single-ended
					   unidirectional transmit mode.   */
	USB_SINGLE_ENDED_UNIDIR_RX,	/*!< Select Single-ended
					   unidirectional receive mode.    */
	USB_SINGLE_ENDED_BIDIR,	/*!< Select Single-ended
				   bidirectional transmit mode.    */
	USB_SINGLE_ENDED_LOW,	/*!< Select USB SE0 mode.            */
	USB_DIFFERENTIAL_UNIDIR_TX,	/*!< Select Differential
					   unidirectional transmit mode
					   <b>(mc13783-only)</b>.            */
	USB_DIFFERENTIAL_UNIDIR,	/*!< Select Differential
					   unidirectional transmit mode
					   <b>(mc13783-only)</b>.            */

	USB_DIFFERENTIAL_UNIDIR_RX,	/*!< Select Differential
					   unidirectional receive mode.    */
	USB_DIFFERENTIAL_BIDIR,	/*!< Select Differential
				   bidirectional transmit mode
				   <b>(mc13783-only)</b>             */
	USB_SUSPEND_ON,		/*!< Select Suspend mode.            */
	USB_SUSPEND_OFF,	/*!< Terminate Suspend mode.         */
	USB_OTG_SRP_DLP_START,	/*!< Start USB On-The-Go Session
				   Request Protocol using Data
				   Line Pulsing.                   */
	USB_OTG_SRP_DLP_STOP	/*!< Terminate USB On-The-Go Session
				   Request Protocol using Data
				   Line Pulsing.                   */
} PMIC_CONVITY_USB_TRANSCEIVER_MODE;

/*!
 * @enum PMIC_CONVITY_USB_OTG_CONFIG
 * @brief Select the USB On-The-Go configuration options.
 *
 * Defines all possible USB On-The-Go configuration options. Multiple
 * configuration options may be selected at the same time. However, only one
 * VBUS current limit may be selected at a time. Selecting more than one
 * VBUS current limit will result in undefined and implementation-dependent
 * behavior.
 */
typedef enum {
	USB_OTG_SE0CONN = 1,	/*!< Enable automatic
				   connection of a pull-up
				   resistor to VUSB when the
				   SE0 condition is detected. */
	USB_OTG_DLP_SRP = 2,	/*!< Enable use of the hardware
				   timer to control the
				   duration of the data line
				   pulse during the session
				   request protocol.          */
	USB_PULL_OVERRIDE = 4,	/*!< Enable automatic disconnect
				   of pull-up and pull-down
				   resistors when transmitter
				   is enabled.                */
	USB_VBUS_CURRENT_LIMIT_HIGH = 8,	/*!< Select current limit to 200mA
						   for VBUS regulator.        */
	USB_VBUS_CURRENT_LIMIT_LOW = 16,	/*!< Select low current limit
						   for VBUS regulator.        */
	USB_VBUS_CURRENT_LIMIT_LOW_10MS = 16,	/*!< Select low current limit
						   for VBUS regulator for
						   10 ms <b>(mc13783-only)</b>. */
	USB_VBUS_CURRENT_LIMIT_LOW_20MS = 32,	/*!< Select low current limit
						   for VBUS regulator for
						   20 ms <b>(mc13783-only)</b>. */
	USB_VBUS_CURRENT_LIMIT_LOW_30MS = 64,	/*!< Select low current limit
						   for VBUS regulator for
						   30 ms <b>(mc13783-only)</b>. */
	USB_VBUS_CURRENT_LIMIT_LOW_40MS = 128,	/*!< Select low current limit
						   for VBUS regulator for
						   40 ms <b>(mc13783-only)</b>. */
	USB_VBUS_CURRENT_LIMIT_LOW_50MS = 256,	/*!< Select low current limit
						   for VBUS regulator for
						   50 ms <b>(mc13783-only)</b>. */
	USB_VBUS_CURRENT_LIMIT_LOW_60MS = 512,	/*!< Select low current limit
						   for VBUS regulator for
						   60 ms <b>(mc13783-only)</b>. */
	USB_VBUS_PULLDOWN = 1024	/*!< Enable VBUS pull-down.     */
} PMIC_CONVITY_USB_OTG_CONFIG;

/*@}*/

/*!
 * @name RS-232 Mode-specific Typedefs and Enumerations
 * Typedefs and enumerations that are used only for setting up and controlling
 * the RS-232 mode of operation.
 */
/*@{*/

/*!
 * @enum PMIC_CONVITY_RS232_EXTERNAL
 * @brief Select the RS-232 transceiver external connections.
 *
 * Defines all valid RS-232 transceiver external RX/TX connection options.
 * Only one connection mode may be selected at a time.
 */
typedef enum {
	RS232_TX_UDM_RX_UDP,	/*!< Select RS-232 TX on UDM   */
	RS232_TX_UDP_RX_UDM,	/*!< Select RS-232 TX on UDP
				   <b>(sc55112-only)</b>. */
	RS232_TX_RX_EXTERNAL_DEFAULT	/*!< Use power on default.        */
} PMIC_CONVITY_RS232_EXTERNAL;

/*!
 * @enum PMIC_CONVITY_RS232_INTERNAL
 * @brief Select the RS-232 transceiver internal connections.
 *
 * Defines all valid RS-232 transceiver internal RX/TX connection options.
 * Only one connection mode can be selected at a time.
 */
typedef enum {
	RS232_TX_USE0VM_RX_UDATVP,	/*!< Select RS-232 TX from USE0VM
					   <b>(mc13783-only)</b>.         */
	RS232_TX_UDATVP_RX_URXVM,	/*!< Select RS-232 TX from UDATVP
					   <b>(mc13783-only)</b>.         */
	RS232_TX_UTXDI_RX_URXDO,	/*!< Select RS-232 TX from UTXDI
					   <b>(sc55112-only)</b>.    */
	RS232_TX_RX_INTERNAL_DEFAULT	/*!< Use power on default.     */
} PMIC_CONVITY_RS232_INTERNAL;

/*@}*/

/*!
 * @name CEA-936 Mode-specific Typedefs and Enumerations
 * Typedefs and enumerations that are used only for setting up and controlling
 * the CEA-936 mode of operation.
 */
/*@{*/

/*!
 * @enum PMIC_CONVITY_CEA936_EXIT_SIGNAL
 * @brief Select the CEA-936 mode exit signal.
 *
 * Defines all valid CEA-936 connection termination signals. Only one
 * termination signal can be selected at a time.
 */
typedef enum {
	CEA936_UID_NO_PULLDOWN,	/*!< No UID pull-down <b>(mc13783-only)</b>. */
	CEA936_UID_PULLDOWN_6MS,	/*!< UID pull-down for 6 ms (+/-2 ms)
					   <b>(mc13783-only)</b>.                  */
	CEA936_UID_PULLDOWN,	/*!< UID pulled down <b>(mc13783-only)</b>.  */
	CEA936_UDMPULSE		/*!< UDM pulsed <b>(mc13783-only)</b>.       */
} PMIC_CONVITY_CEA936_EXIT_SIGNAL;

/*@}*/

/***************************************************************************
 *                          PMIC API DEFINITIONS                           *
 ***************************************************************************/

/*!
 * @name General Setup and Configuration APIs
 * Functions for general setup and configuration of the PMIC Connectivity
 * hardware.
 */
/*@{*/

/*!
 * @brief Request exclusive access to the PMIC Connectivity hardware.
 *
 * Attempt to open and gain exclusive access to the PMIC Connectivity
 * hardware. An initial operating mode (e.g., USB or RS-232) must also
 * be specified.
 *
 * If the open request is successful, then a numeric handle is returned
 * and this handle must be used in all subsequent function calls. The
 * same handle must also be used in the close call when use of the PMIC
 * connectivity hardware is no longer required.
 *
 * The open request will fail if another thread has already obtained the
 * device handle and has not yet called pmic_convity_close() with it.
 *
 * @param[out]  handle          Device handle to be used for subsequent PMIC
 *                              Connectivity API calls.
 * @param[in]   mode            Initial connectivity operating mode.
 *
 * @retval      PMIC_SUCCESS    If the open request was successful
 * @retval      PMIC_ERROR      If the connectivity hardware cannot be opened.
 */
PMIC_STATUS pmic_convity_open(PMIC_CONVITY_HANDLE * const handle,
			      const PMIC_CONVITY_MODE mode);

/*!
 * @brief Terminate further access to the PMIC Connectivity hardware.
 *
 * Terminate further access to the PMIC Connectivity hardware. This also
 * allows another thread to successfully call pmic_convity_open() to gain
 * access.
 *
 * @param[in]   handle          Device handle from open() call.
 *
 * @retval      PMIC_SUCCESS         If the close request was successful.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_convity_close(const PMIC_CONVITY_HANDLE handle);

/*!
 * @brief Set the PMIC Connectivity main operating mode.
 *
 * Change the current operating mode of the PMIC Connectivity hardware.
 * The available connectivity operating modes are hardware-dependent and
 * consists of one or more of the following: USB (including USB On-the-Go),
 * RS-232, and CEA-936. Requesting an operating mode that is not supported
 * by the PMIC hardware will return PMIC_NOT_SUPPORTED.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   mode            Desired operating mode.
 *
 * @retval      PMIC_SUCCESS         If the requested mode was successfully set.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If the PMIC hardware does not support
 *                                   the desired operating mode.
 */
PMIC_STATUS pmic_convity_set_mode(const PMIC_CONVITY_HANDLE handle,
				  const PMIC_CONVITY_MODE mode);

/*!
 * @brief Get the current PMIC Connectivity main operating mode.
 *
 * Get the current operating mode for the PMIC Connectivity hardware.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[out]  mode            The current PMIC Connectivity operating mode.
 *
 * @retval      PMIC_SUCCESS         If the requested mode was successfully set.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_convity_get_mode(const PMIC_CONVITY_HANDLE handle,
				  PMIC_CONVITY_MODE * const mode);

/*!
 * @brief Reset the Connectivity hardware to it's power on state.
 *
 * Restore all registers to the initial power-on/reset state.
 *
 * @param[in]   handle          Device handle from open() call.
 *
 * @retval      PMIC_SUCCESS         If the reset was successful.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_convity_reset(const PMIC_CONVITY_HANDLE handle);

/*!
 * @brief Set the Connectivity callback function.
 *
 * Register a callback function that will be used to signal PMIC Connectivity
 * events. For example, the USB subsystem should register a callback function
 * in order to be notified of device connect/disconnect events. Note, however,
 * that non-USB events may also be signalled depending upon the PMIC hardware
 * capabilities. Therefore, the callback function must be able to properly
 * handle all of the possible events if support for non-USB peripherals is
 * also to be included.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   func            A pointer to the callback function.
 * @param[in]   eventMask       A mask selecting events to be notified.
 *
 * @retval      PMIC_SUCCESS         If the callback was successfully registered.
 * @retval      PMIC_PARAMETER_ERROR If the handle or the eventMask is invalid.
 */
PMIC_STATUS pmic_convity_set_callback(const PMIC_CONVITY_HANDLE handle,
				      const PMIC_CONVITY_CALLBACK func,
				      const PMIC_CONVITY_EVENTS eventMask);

/*!
 * @brief Deregisters the existing Connectivity callback function.
 *
 * Deregister the callback function that was previously registered by calling
 * pmic_convity_set_callback().
 *
 * @param[in]   handle          Device handle from open() call.
 *
 * @retval      PMIC_SUCCESS         If the callback was successfully deregistered.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_convity_clear_callback(const PMIC_CONVITY_HANDLE handle);

/*!
 * @brief Get the current Connectivity callback function settings.
 *
 * Get the current callback function and event mask.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[out]  func            The current callback function.
 * @param[out]  eventMask       The current event selection mask.
 *
 * @retval      PMIC_SUCCESS         If the callback information was successfully
 *                                   retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_convity_get_callback(const PMIC_CONVITY_HANDLE handle,
				      PMIC_CONVITY_CALLBACK * const func,
				      PMIC_CONVITY_EVENTS * const eventMask);

/*@}*/

/***************************************************************************/

/*!
 * @name USB and USB On-The-Go APIs
 * USB Connectivity mode-specific configuration and setup functions.
 */
/*@{*/

/*!
 * @brief Set the USB transceiver's operating speed.
 *
 * Set the USB transceiver speed. Both mc13783 and sc55112 have support
 * for low speed (1.5 Mbps) and full speed (12 Mbps) modes. However, there
 * is currently no support for the high speed (480 Mbps) mode.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   speed           The desired USB transceiver speed.
 * @param[in]   mode            The USB transceiver mode (host/peripheral).
 *
 * @retval      PMIC_SUCCESS         If the transceiver speed was successfully
 *                                   set.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If the high speed (480 Mbps) mode is
 *                                   requested.
 */
PMIC_STATUS pmic_convity_usb_set_speed(const PMIC_CONVITY_HANDLE handle,
				       const PMIC_CONVITY_USB_SPEED speed,
				       const PMIC_CONVITY_USB_MODE mode);

/*!
 * @brief Get the USB transceiver's operating speed.
 *
 * Get the USB transceiver speed.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[out]  speed           The current USB transceiver speed.
 * @param[out]  mode            The current USB transceiver mode.
 *
 * @retval      PMIC_SUCCESS         If the transceiver speed was successfully
 *                                   set.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 *                                   obtained
 */
PMIC_STATUS pmic_convity_usb_get_speed(const PMIC_CONVITY_HANDLE handle,
				       PMIC_CONVITY_USB_SPEED * const speed,
				       PMIC_CONVITY_USB_MODE * const mode);

/*!
 * @brief Set the USB transceiver's power supply configuration.
 *
 * Set the USB transceiver's power supply configuration.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   pwrin           USB transceiver regulator input power source.
 * @param[in]   pwrout          USB transceiver regulator output power level.
 *
 * @retval      PMIC_SUCCESS         If the USB transceiver's power supply
 *                                   configuration was successfully set.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If the PMIC hardware does not support
 *                                   the desired configuration.
 */
PMIC_STATUS pmic_convity_usb_set_power_source(const PMIC_CONVITY_HANDLE handle,
					      const PMIC_CONVITY_USB_POWER_IN
					      pwrin,
					      const PMIC_CONVITY_USB_POWER_OUT
					      pwrout);

/*!
 * @brief Get the USB transceiver's power supply configuration.
 *
 * Get the USB transceiver's current power supply configuration.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[out]  pwrin           USB transceiver regulator input power source
 * @param[out]  pwrout          USB transceiver regulator output power level
 *
 * @retval      PMIC_SUCCESS         If the USB transceiver's power supply
 *                                   configuration was successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_convity_usb_get_power_source(const PMIC_CONVITY_HANDLE handle,
					      PMIC_CONVITY_USB_POWER_IN *
					      const pwrin,
					      PMIC_CONVITY_USB_POWER_OUT *
					      const pwrout);

/*!
 * @brief Set the current USB transceiver operating mode.
 *
 * Set the USB transceiver's operating mode.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   mode            Desired operating mode.
 *
 * @retval      PMIC_SUCCESS    If the USB transceiver's operating mode
 *                              was successfully configured.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If the desired USB transceiver mode is
 *                                   not supported by the PMIC hardware.
 */
PMIC_STATUS pmic_convity_usb_set_xcvr(const PMIC_CONVITY_HANDLE handle,
				      const PMIC_CONVITY_USB_TRANSCEIVER_MODE
				      mode);

/*!
 * @brief Get the current USB transceiver operating mode.
 *
 * Get the USB transceiver's current operating mode.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[out]  mode            Current operating mode.
 *
 * @retval      PMIC_SUCCESS         If the USB transceiver's operating mode
 *                                   was successfully retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_convity_usb_get_xcvr(const PMIC_CONVITY_HANDLE handle,
				      PMIC_CONVITY_USB_TRANSCEIVER_MODE *
				      const mode);

/*!
 * @brief Set the current USB On-The-Go data line pulse duration (ms).
 *
 * Set the Data Line Pulse duration (in milliseconds) for the USB OTG
 * Session Request Protocol.
 *
 * Note that for mc13783 the duration is fixed at 7.5 ms and calling this
 * function will simply return PMIC_NOT_SUPPORTED.
 *
 * For sc55112, valid values are in the range of 0-7. A value of 0
 * selects a pulse duration that is manually controlled by calling
 * pmic_convity_usb_otg_set_config() with USB_OTG_DLP_SRP_SET and
 * USB_OTG_DLP_SRP_CLEAR. Values between 1-7 selects a pulse duration
 * from 4-10 ms (the hardware always adds an additional 3 ms).
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   duration        The data line pulse duration (ms).
 *
 * @retval      PMIC_SUCCESS         If the pulse duration was successfully set.
 * @retval      PMIC_PARAMETER_ERROR If the handle or the data line pulse
 *                                   duration is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If the desired data line pulse duration
 *                                   is not supported by the PMIC hardware.
 */
PMIC_STATUS pmic_convity_usb_otg_set_dlp_duration(const PMIC_CONVITY_HANDLE
						  handle,
						  const unsigned int duration);

/*!
 * @brief Get the current USB On-The-Go data line pulse duration (ms).
 *
 * Get the current Data Line Pulse duration (in milliseconds) for the USB
 * OTG Session Request Protocol.
 *
 * Note that the Data Line Pulse duration is fixed at 7.5 ms for the mc13783
 * PMIC. Therefore, calling this function while using the mc13783 PMIC will
 * simply return PMIC_NOT_SUPPORTED.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[out]  duration        The data line pulse duration (ms).
 *
 * @retval      PMIC_SUCCESS         If the pulse duration was successfully
 *                                   obtained.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If called using the mc13783 PMIC.
 */
PMIC_STATUS pmic_convity_usb_otg_get_dlp_duration(const PMIC_CONVITY_HANDLE
						  handle,
						  unsigned int *const duration);

/*!
 * @brief Start the USB OTG Host Negotiation Protocol (HNP) process.
 *
 * This function must be called during the start of the HNP process to
 * properly reconfigure the pull-up resistor on the D+ line for both
 * the USB A and B devices.
 *
 * @param[in]   handle          device handle from open() call
 * @param[in]   deviceType      the USB device type (either A or B)
 *
 * @return      PMIC_SUCCESS    if the HNP was successfully started
 */
PMIC_STATUS pmic_convity_usb_otg_begin_hnp(const PMIC_CONVITY_HANDLE handle,
					   const PMIC_CONVITY_USB_DEVICE_TYPE
					   deviceType);

/*!
 * @brief Complete the USB OTG Host Negotiation Protocol (HNP) process.
 *
 * This function must be called during the end of the HNP process to
 * properly reconfigure the pull-up resistor on the D+ line for both
 * the USB A and B devices.
 *
 * @param[in]   handle          device handle from open() call
 * @param[in]   deviceType      the USB device type (either A or B)
 *
 * @return      PMIC_SUCCESS    if the HNP was successfully ended
 */
PMIC_STATUS pmic_convity_usb_otg_end_hnp(const PMIC_CONVITY_HANDLE handle,
					 const PMIC_CONVITY_USB_DEVICE_TYPE
					 deviceType);

/*!
 * @brief Set the current USB On-The-Go configuration.
 *
 * Set the USB On-The-Go (OTG) configuration. Multiple configuration settings
 * may be OR'd together in a single call. However, selecting conflicting
 * settings (e.g., multiple VBUS current limits) will result in undefined
 * behavior.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   cfg             Desired USB OTG configuration.
 *
 * @retval      PMIC_SUCCESS         If the OTG configuration was successfully
 *                                   set.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If the desired USB OTG configuration is
 *                                   not supported by the PMIC hardware.
 */
PMIC_STATUS pmic_convity_usb_otg_set_config(const PMIC_CONVITY_HANDLE handle,
					    const PMIC_CONVITY_USB_OTG_CONFIG
					    cfg);

/*!
 * @brief Clear the current USB On-The-Go configuration.
 *
 * Clears the USB On-The-Go (OTG) configuration. Multiple configuration settings
 * may be OR'd together in a single call. However, selecting conflicting
 * settings (e.g., multiple VBUS current limits) will result in undefined
 * behavior.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   cfg             USB OTG configuration settings to be cleared.
 *
 * @retval      PMIC_SUCCESS         If the OTG configuration was successfully
 *                                   cleared.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If the desired USB OTG configuration is
 *                                   not supported by the PMIC hardware.
 */
PMIC_STATUS pmic_convity_usb_otg_clear_config(const PMIC_CONVITY_HANDLE handle,
					      const PMIC_CONVITY_USB_OTG_CONFIG
					      cfg);

/*!
 * @brief Get the current USB On-The-Go configuration.
 *
 * Get the current USB On-The-Go (OTG) configuration.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[out]  cfg             The current USB OTG configuration.
 *
 * @retval      PMIC_SUCCESS         If the OTG configuration was successfully
 *                                   retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_convity_usb_otg_get_config(const PMIC_CONVITY_HANDLE handle,
					    PMIC_CONVITY_USB_OTG_CONFIG *
					    const cfg);

/*@}*/

/***************************************************************************/

/*!
 * @name RS-232 APIs
 * RS-232 Connectivity mode-specific configuration and setup functions.
 */
/*@{*/

/*!
 * @brief Set the current RS-232 operating configuration.
 *
 * Set the connectivity interface to the selected RS-232 operating mode.
 * Note that the RS-232 operating mode will be automatically overridden
 * if the USB_EN is asserted at any time (e.g., when a USB device is
 * attached).
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   cfgInternal     RS-232 transceiver internal connections.
 * @param[in]   cfgExternal     RS-232 transceiver external connections.
 *
 * @retval      PMIC_SUCCESS         If the requested RS-232 mode was set.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If the desired RS-232 configuration is
 *                                   not supported by the PMIC hardware.
 */
PMIC_STATUS pmic_convity_rs232_set_config(const PMIC_CONVITY_HANDLE handle,
					  const PMIC_CONVITY_RS232_INTERNAL
					  cfgInternal,
					  const PMIC_CONVITY_RS232_EXTERNAL
					  cfgExternal);

/*!
 * @brief Get the current RS-232 operating configuration.
 *
 * Get the connectivity interface's current RS-232 operating mode.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[out]  cfgInternal     RS-232 transceiver internal connections.
 * @param[out]  cfgExternal     RS-232 transceiver external connections.
 *
 * @retval      PMIC_SUCCESS         If the requested RS-232 mode was retrieved.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 */
PMIC_STATUS pmic_convity_rs232_get_config(const PMIC_CONVITY_HANDLE handle,
					  PMIC_CONVITY_RS232_INTERNAL *
					  const cfgInternal,
					  PMIC_CONVITY_RS232_EXTERNAL *
					  const cfgExternal);

/***************************************************************************/

/*@}*/

/*!
 * @name CE-936 APIs
 * CE-936 Connectivity mode-specific configuration and setup functions.
 */
/*@{*/

/*!
 * @brief Send a signal to exit CEA-936 mode.
 *
 * Signal the attached device to exit the current CEA-936 operating mode.
 * Returns an error if the current operating mode is not CEA-936.
 *
 * @param[in]   handle          Device handle from open() call.
 * @param[in]   signal          Type of exit signal to be sent.
 *
 * @retval      PMIC_SUCCESS         If the CEA-936 exit mode signal was sent.
 * @retval      PMIC_PARAMETER_ERROR If the handle is invalid.
 * @retval      PMIC_NOT_SUPPORTED   If the desired CEA-936 exit mode signal
 *                                   is not supported by the PMIC hardware.
 */
PMIC_STATUS pmic_convity_cea936_exit_signal(const PMIC_CONVITY_HANDLE handle,
					    const
					    PMIC_CONVITY_CEA936_EXIT_SIGNAL
					    signal);

/*@}*/

#endif				/* __PMIC_CONVITY_H__ */
