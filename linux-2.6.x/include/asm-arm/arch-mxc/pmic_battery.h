/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

 /*
  * The code contained herein is licensed under the GNU Lesser General
  * Public License.  You may obtain a copy of the GNU Lesser General
  * Public License Version 2.1 or later at the following locations:
  *
  * http://www.opensource.org/licenses/lgpl-license.html
  * http://www.gnu.org/copyleft/lgpl.html
  */

#ifndef _PMIC_BATTERY_H
#define _PMIC_BATTERY_H

/*!
 * @defgroup PMIC_BATTERY PMIC Battery Driver
 * @ingroup PMIC_DRVRS
 */

/*!
 * @file pmic_battery.h
 * @brief This is the header of PMIC Battery driver.
 *
 * @ingroup PMIC_BATTERY
 */

#include <asm/ioctl.h>

/*!
 * @name IOCTL user space interface
 */

/*! @{ */
/*!
 * Enable and disable charger.
 * Argument type: pointer to t_charger_setting
 */
#define PMIC_BATT_CHARGER_CONTROL              _IOW('p', 0xc0, int)
/*!
 * Set charger configuration.
 * Argument type: pointer to t_charger_setting
 */
#define PMIC_BATT_SET_CHARGER                  _IOW('p', 0xc1, int)
/*!
 * Get charger configuration.
 * Argument type: pointer to t_charger_setting
 */
#define PMIC_BATT_GET_CHARGER                  _IOR('p', 0xc2, int)
/*!
 * Get charger current.
 * Argument type: pointer to t_charger_setting
 */
#define PMIC_BATT_GET_CHARGER_CURRENT          _IOR('p', 0xc3, int)
/*!
 * Set EOL control
 * Argument type: pointer to t_eol_setting
 */
#define PMIC_BATT_EOL_CONTROL                   _IOW('p', 0xc4, int)
/*!
 * Enable and disable charging LED.
 * Argument type: bool
 */
#define PMIC_BATT_LED_CONTROL                   _IOW('p', 0xc5, int)
/*!
 * Enable and disable reverse supply.
 * Argument type: bool
 */
#define PMIC_BATT_REV_SUPP_CONTROL              _IOW('p', 0xc6, int)
/*!
 * Enable and disable unregulated charging mode.
 * Argument type: bool
 */
#define PMIC_BATT_UNREG_CONTROL                 _IOW('p', 0xc7, int)
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * Set the output controls.
 * Argument type: t_control
 */
#define PMIC_BATT_SET_OUT_CONTROL                _IOW('p', 0xc8, int)
/*!
 * Set the over voltage threshold.
 * Argument type: int
 */
#define PMIC_BATT_SET_THRESHOLD	                 _IOW('p', 0xc9, int)
/*! @} */
#endif
/*!
 * This enumeration defines battery chargers.
 */
typedef enum {
	BATT_MAIN_CHGR = 0,	/*!< Main battery charger */
	BATT_CELL_CHGR,		/*!< Cell battery charger */
	BATT_TRCKLE_CHGR	/*!< Trickle charger (only available on mc13783) */
} t_batt_charger;
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This enumeration defines the bp threshold.
 */
typedef enum {
	BATT_BP_0 = 0,		/*!< LOBATL UVDET + 0.2 */
	BATT_BP_1,		/*!< LOBATL UVDET + 0.3 */
	BATT_BP_2,		/*!< LOBATL UVDET + 0.4 */
	BATT_BP_3		/*!< LOBATL UVDET + 0.5 */
} t_bp_threshold;

/*!
 * This enumeration of all types of output controls
 */
typedef enum {
	/*!
	 * controlled hardware
	 */
	CONTROL_HARDWARE = 0,
	/*!
	 * BPFET is driven low, BATTFET is driven high
	 */
	CONTROL_BPFET_LOW,
	/*!
	 * BPFET is driven high, BATTFET is driven low
	 */
	CONTROL_BPFET_HIGH,
} t_control;

/*!
 * This enumeration define all battery interrupt
 */
typedef enum {
	/*!
	 * Charge detection interrupt
	 */
	BAT_IT_CHG_DET,
	/*!
	 * Charge over voltage detection it
	 */
	BAT_IT_CHG_OVERVOLT,
	/*!
	 * Charge path reverse current it
	 */
	BAT_IT_CHG_REVERSE,
	/*!
	 * Charge path short circuitin revers supply mode it
	 */
	BAT_IT_CHG_SHORT_CIRCUIT,
	/*!
	 * Charger has switched its mode (CC to CV or CV to CC)
	 */
	BAT_IT_CCCV,
	/*!
	 * Charge current has dropped below its threshold
	 */
	BAT_IT_BELOW_THRESHOLD,
} t_batt_event;
#endif
/*!
 * This structure is used for the following battery changer control
 * IOCTLs:
 * - PMIC_BATT_CHARGER_CONTROL
 * - PMIC_BATT_SET_CHARGER
 * - PMIC_BATT_GET_CHARGER
 */
typedef struct {
	/*!
	 * Charger
	 */
	t_batt_charger chgr;
	/*!
	 * Turn on charger
	 */
	bool on;
	/*!
	 * Charging voltage
	 */
	unsigned char c_voltage;
	/*!
	 * Charging current
	 */
	unsigned char c_current;
} t_charger_setting;
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This structure is used for EOL setting IOCTL PMIC_BATT_EOL_CONTROL
 */
typedef struct {
	/*!
	 * Enable EOL comparator
	 */
	bool enable;
	/*!
	 * c_voltage threshold - Used on SC55112
	 */
	unsigned char threshold;
	/*!
	 * bp threshold - Used on mc13783
	 */
	t_bp_threshold typical;
} t_eol_setting;
#endif
#ifdef CONFIG_MXC_PMIC_SC55112
/*!
 * This structure is used for EOL setting IOCTL PMIC_BATT_EOL_CONTROL
 */
typedef struct {
	/*!
	 * Enable EOL comparator
	 */
	bool enable;
	/*!
	 * c_voltage threshold
	 */
	unsigned char threshold;
} t_eol_setting;
#endif

/* EXPORTED FUNCTIONS */
#ifdef __KERNEL__

/*!
 * This function is used to start charging a battery. For different charger,
 * different c_voltage and current range are supported. \n
 *
 *
 * @param      chgr        Charger as defined in \b t_batt_charger.
 * @param      c_voltage     Charging voltage.
 * @param      c_current     Charging current.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_enable_charger(t_batt_charger chgr,
				     unsigned char c_voltage,
				     unsigned char c_current);

/*!
 * This function turns off a charger.
 *
 * @param      chgr        Charger as defined in \b t_batt_charger.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_disable_charger(t_batt_charger chgr);

/*!
 * This function is used to change the charger setting.
 *
 * @param      chgr        Charger as defined in \b t_batt_charger.
 * @param      c_voltage   Charging voltage.
 * @param      c_current   Charging current.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_charger(t_batt_charger chgr,
				  unsigned char c_voltage,
				  unsigned char c_current);

/*!
 * This function is used to retrieve the charger setting.
 *
 * @param      chgr        Charger as defined in \b t_batt_charger.
 * @param      c_voltage   Output parameter for charging c_voltage setting.
 * @param      c_current   Output parameter for charging current setting.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_get_charger_setting(t_batt_charger chgr,
					  unsigned char *c_voltage,
					  unsigned char *c_current);

/*!
 * This function is retrieves the main battery charging current.
 *
 * @param      c_current     Output parameter for charging current setting.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_batt_get_charge_current(unsigned short *c_current);
#else
PMIC_STATUS pmic_batt_get_charge_current(unsigned char *c_current);
#endif
/*!
 * This function enables End-of-Life comparator.
 *
 * @param      threshold  End-of-Life threshold.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_enable_eol(unsigned char threshold);
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function enables End-of-Life comparator.
 *
 * @param      typical  Falling Edge Threshold threshold.
 *                      @verbatim
                        BPDET   UVDET   LOBATL
                        ____    _____   ___________
                        0       2.6     UVDET + 0.2
                        1       2.6     UVDET + 0.3
                        2       2.6     UVDET + 0.4
                        3       2.6     UVDET + 0.5
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_bp_enable_eol(t_bp_threshold typical);
#endif
/*!
 * This function disables End-of-Life comparator.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_disable_eol(void);
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function sets the output controls.
 * It sets the FETOVRD and FETCTRL bits of mc13783
 *
 * @param        control        type of control.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_batt_set_out_control(t_control control);

/*!
 * This function sets over voltage threshold.
 *
 * @param        threshold      value of over voltage threshold.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_batt_set_threshold(int threshold);
#endif
/*!
 * This function controls charge LED.
 *
 * @param      on   If on is true, LED will be turned on,
 *                  or otherwise, LED will be turned off.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_led_control(bool on);

/*!
 * This function sets reverse supply mode. This function is not applicable
 * to sc55112.
 *
 * @param      enable     If enable is true, reverse supply mode is enable,
 *                        or otherwise, reverse supply mode is disabled.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_reverse_supply(bool enable);

/*!
 * This function sets unregulated charging mode on main battery. This
 * function is not applicable to sc55112.
 *
 * @param      enable     If enable is true, unregulated charging mode is
 *                        enable, or otherwise, disabled.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_unregulated(bool enable);
#ifdef CONFIG_MXC_MC13783_PMIC

/*!
 * This function sets a 5K pull down at CHRGRAW.
 * To be used in the dual path charging configuration.
 *
 * @param      enable     If enable is true, 5k pull down is
 *                        enable, or otherwise, disabled.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_5k_pull(bool enable);

/*!
 * This function is used to subscribe on battery event IT.
 *
 * @param        event          type of event.
 * @param        callback       event callback function.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_batt_event_subscribe(t_batt_event event, void *callback);

/*!
 * This function is used to un subscribe on battery event IT.
 *
 * @param        event          type of event.
 * @param        callback       event callback function.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_batt_event_unsubscribe(t_batt_event event, void *callback);
#endif
#endif				/* __KERNEL__ */

#endif				/* _PMIC_BATTERY_H */
