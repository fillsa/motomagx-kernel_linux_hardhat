/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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

/*!
 * @file mc13783_power.c
 * @brief This is the main file of mc13783 Power driver.
 *
 * The mc13783 Power driver has been developped to support on chip signals
 * and detector outputs.
 * The principle elements of this interface are:
 *
 * @verbatim
       Name          Type of sig  Function
       -------       -----------  --------------------------------
       ON1B          Input pin    Connection for a power on/of button
       ON2B          Input pin    Connection for an accessory power
                                  on/off button
       ON3B          Input pin    Connection for a third power on/off button
       WDI           Input pin    Watchdog input has to be kept high by the
                                  processor to keep mc13783 active
       RESETB        Output pin   Reset Bar output (active low) to the
                                  application. Open drain output.
       RESETBMCU     Output pin   Reset Bar output (active low) to the
                                  processor core. Open drain output.
       PWRFAIL       Output pin   Power fail signal indicates if an
                                  undervoltage condition occurs
       LOBATB        Output pin   Low battery detection signal, goes low
                                  when a low BP condition occurs
       USEROFF       Input pin    Signal from processor to confirm user off
                                  mode after a power fail
       STANDBYPRI    Input pin    Signal from primary processor to put mc13783
                                  in a low power mode
       STANBYPRIINV  SPI bit      If set then STANDBYPRI is interpreted as
                                  active low
       STANDBYSEC    Input pin    Signal from secundary processor to put mc13783
                                  in a low power mode
       STANDBYSECINV SPI bit      If set then STANDBYSEC is interpreted as
                                  active low
       UVDET         Detector     Under voltage detector output
       LOBATDET      Detector     Low battery detector output
       RTCUVDET      Detector     RTC under voltage detector output
       RTCLOBATDET   Detector     RTC low battery detector output
       CHGDET        Detector     Charger presence detector output
       USBDET        Detector     USB presence detector output
       BPONI         SPI bit      BP Turn on threshold interrupt
       LOBATHI       SPI bit      Low battery (BP) warning
       LOBATLI       SPI bit      Low battery (BP) turn off
       BPDET[1:0]    SPI bits     BP detection thresholds setting
       USEROFFSPI    SPI bit      Initiates a transition to memory hold mode
                                  or user off mode
       USEROFFPC     SPI bit      Allows to transition to user off during
                                  power cut with USEROFF low
       USEROFFCLK    SPI bit      Keeps the CLK32KMCU active during user off
                                  modes
       CLK32KMCUEN   SPI bit      Enables the CLK32KMCU clock output, defaults
                                  to 1
       VBKUP1EN      SPI bit      Enables VBKUP1 in startup modes, on and user
                                  off wait modes
       VBKUP1AUTO    SPI bit      Enables VBKUP1 in user off and memory hold
                                  modes
       VBKUP2EN      SPI bit      Enables VBKUP2 in startup modes, on and
                                  user off wait modes
       VBKUP2AUTOMH  SPI bit      Enables VBKUP2 in memory hold modes
       VBKUP2AUTOUO  SPI bit      Enables VBKUP2 in user off modes
       WARMEN        SPI bit      Enables for a transition to user off mode
       WARMI         SPI bit      Indicates that the application powered up
                                  from user off mode
       MEMHLDI       SPI bit      Indicates that the application powered up
                                  from memory hold mode
       PCEN          SPI bit      Enables power cut support
       PCI           SPI bit      Indicates that a power cut has occurred
       ON1BDBNC[1:0] SPI bits     Debounce time on ON1B pin
       ON2BDBNC[1:0] SPI bits     Debounce time on ON2B pin
       ON3BDBNC[1:0] SPI bits     Debounce time on ON3B pin
       ON1BRSTEN     SPI bit      System reset enable for ON1B pin
       ON2BRSTEN     SPI bit      System reset enable for ON2B pin
       ON3BRSTEN     SPI bit      System reset enable for ON3B pin
       SYSRSTI       SPI bit      System reset interrupt
       RESTARTEN     SPI bit      Allows for restart after a system reset
   @endverbatim
 *
 *
 * @ingroup MC13783_POWER
 */

#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "../core/mc13783_config.h"
#include "../core/mc13783_event.h"
#include "../core/mc13783_external.h"

#include "mc13783_power.h"
#include "mc13783_power_defs.h"

#define MC13783_LOAD_ERROR_MSG		\
"mc13783 card was not correctly detected. Stop loading mc13783 Power driver\n"

static int mc13783_power_major;
static int mc13783_power_detected = 0;

/*
 * Audio mc13783 API
 */

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(mc13783_power_get_power_mode_sense);
EXPORT_SYMBOL(mc13783_power_set_regen_assig);
EXPORT_SYMBOL(mc13783_power_get_regen_assig);
EXPORT_SYMBOL(mc13783_power_set_regen_inv);
EXPORT_SYMBOL(mc13783_power_get_regen_inv);
EXPORT_SYMBOL(mc13783_power_conf_switcher);
EXPORT_SYMBOL(mc13783_power_get_switcher);
EXPORT_SYMBOL(mc13783_power_regu_en);
EXPORT_SYMBOL(mc13783_power_get_regu_state);
EXPORT_SYMBOL(mc13783_power_conf_regu);
EXPORT_SYMBOL(mc13783_power_get_regu_conf);
EXPORT_SYMBOL(mc13783_power_set_regu);
EXPORT_SYMBOL(mc13783_power_get_regu);
EXPORT_SYMBOL(mc13783_power_cut_en);
EXPORT_SYMBOL(mc13783_power_get_power_cut);
EXPORT_SYMBOL(mc13783_power_warm_start_en);
EXPORT_SYMBOL(mc13783_power_get_warm_start);
EXPORT_SYMBOL(mc13783_power_user_off_en);
EXPORT_SYMBOL(mc13783_power_get_user_off_en);
EXPORT_SYMBOL(mc13783_power_32k_en);
EXPORT_SYMBOL(mc13783_power_get_32k_state);
EXPORT_SYMBOL(mc13783_power_cut_conf);
EXPORT_SYMBOL(mc13783_power_cut_get_conf);
EXPORT_SYMBOL(mc13783_power_vbkup_conf);
EXPORT_SYMBOL(mc13783_power_get_vbkup_conf);
EXPORT_SYMBOL(mc13783_power_set_bp_threshold);
EXPORT_SYMBOL(mc13783_power_get_bp_threshold);
EXPORT_SYMBOL(mc13783_power_set_eol_function);
EXPORT_SYMBOL(mc13783_power_get_eol_function);
EXPORT_SYMBOL(mc13783_power_set_coincell_voltage);
EXPORT_SYMBOL(mc13783_power_get_coincell_voltage);
EXPORT_SYMBOL(mc13783_power_coincell_charger_en);
EXPORT_SYMBOL(mc13783_power_get_coincell_charger_en);
EXPORT_SYMBOL(mc13783_power_set_auto_reset_en);
EXPORT_SYMBOL(mc13783_power_get_auto_reset_en);
EXPORT_SYMBOL(mc13783_power_set_conf_button);
EXPORT_SYMBOL(mc13783_power_get_conf_button);
EXPORT_SYMBOL(mc13783_power_set_conf_pll);
EXPORT_SYMBOL(mc13783_power_get_conf_pll);
EXPORT_SYMBOL(mc13783_power_set_sw3);
EXPORT_SYMBOL(mc13783_power_get_sw3);
EXPORT_SYMBOL(mc13783_power_event_sub);
EXPORT_SYMBOL(mc13783_power_event_unsub);
EXPORT_SYMBOL(mc13783_power_vbkup2_auto_en);
EXPORT_SYMBOL(mc13783_power_get_vbkup2_auto_state);
EXPORT_SYMBOL(mc13783_power_bat_det_en);
EXPORT_SYMBOL(mc13783_power_get_bat_det_state);
EXPORT_SYMBOL(mc13783_power_esim_v_en);
EXPORT_SYMBOL(mc13783_power_gets_esim_v_state);
EXPORT_SYMBOL(mc13783_power_vib_pin_en);
EXPORT_SYMBOL(mc13783_power_gets_vib_pin_state);
EXPORT_SYMBOL(mc13783_power_loaded);

/*
 * Internal function
 */
int mc13783_power_event(t_pwr_int event, void *callback, bool sub);

/*!
 * This function is called to put the power in a low power state.
 * Switching off the platform cannot be decided by
 * the power module. It has to be handled by the
 * client application.
 *
 * @param   dev   The device structure used to give information on which power
 *                device (0 through 3 channels) to suspend
 * @param   state The power state the device is entering
 * @param   level The stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mc13783_power_suspend(struct device *dev, u32 state, u32 level)
{
	switch (level) {
	case SUSPEND_DISABLE:
		break;
	case SUSPEND_SAVE_STATE:
		/* TBD */
		break;
	case SUSPEND_POWER_DOWN:
		/* Turn off clock */
		break;
	}
	return 0;
};

/*!
 * This function is called to resume the power from a low power state.
 *
 * @param   dev   The device structure used to give information on which power
 *                device (0 through 3 channels) to suspend
 * @param   level The stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mc13783_power_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_POWER_ON:
		/* Turn on clock */
		break;
	case RESUME_RESTORE_STATE:
		/* TBD */
		break;
	case RESUME_ENABLE:
		break;
	}
	return 0;
};

static struct device_driver mc13783_power_driver_ldm = {
	.name = "mc13783_power",
	.bus = &platform_bus_type,
	.suspend = mc13783_power_suspend,
	.resume = mc13783_power_resume,
};

static struct platform_device mc13783_power_ldm = {
	.name = "mc13783_power",
	.id = 1,
};

/*
 * Power mc13783 API
 */

/*!
 * This function returns power up sense value.
 *
 * @param        p_up_sense  	Value of power up sense
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_power_mode_sense(struct t_p_up_sense *p_up_sense)
{
	unsigned int reg_value = 0;
	CHECK_ERROR(mc13783_read_reg(PRIO_PWM, REG_POWER_UP_MODE_SENSE,
				     &reg_value));
	p_up_sense->state_ictest = (STATE_ICTEST_MASK & reg_value);
	p_up_sense->state_clksel = ((STATE_CLKSEL_MASK & reg_value)
				    >> STATE_CLKSEL_BIT);
	p_up_sense->state_pums1 = ((STATE_PUMS1_MASK & reg_value)
				   >> STATE_PUMS1_BITS);
	p_up_sense->state_pums2 = ((STATE_PUMS2_MASK & reg_value)
				   >> STATE_PUMS2_BITS);
	p_up_sense->state_pums3 = ((STATE_PUMS3_MASK & reg_value)
				   >> STATE_PUMS3_BITS);
	p_up_sense->state_chrgmode0 = ((STATE_CHRGM1_MASK & reg_value)
				       >> STATE_CHRGM1_BITS);
	p_up_sense->state_chrgmode1 = ((STATE_CHRGM2_MASK & reg_value)
				       >> STATE_CHRGM2_BITS);
	p_up_sense->state_umod = ((STATE_UMOD_MASK & reg_value)
				  >> STATE_UMOD_BITS);
	p_up_sense->state_usben = ((STATE_USBEN_MASK & reg_value)
				   >> STATE_USBEN_BIT);
	p_up_sense->state_sw_1a1b_joined = ((STATE_SW1A_J_B_MASK & reg_value)
					    >> STATE_SW1A_J_B_BIT);
	p_up_sense->state_sw_2a2b_joined = ((STATE_SW2A_J_B_MASK & reg_value)
					    >> STATE_SW2A_J_B_BIT);
	return ERROR_NONE;
}

/*!
 * This function configures the Regen assignment for all regulator.
 * The REGEN pin allows to enable or disable one or more regulators and
 * switchers of choice. The REGEN function can be used in two ways.
 * It can be used as a regulator enable pin like with SIMEN
 * where the SPI programming is static and the REGEN pin is dynamic.
 * It can also be used in a static fashion where REGEN is maintained
 * high while the regulators get enabled and disabled dynamically
 * via SPI. In that case REGEN functions as a master enable.
 *
 * @param        regu     	Type of regulator
 * @param        en_dis  	If true, the regulator is enabled by regen.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_regen_assig(t_regulator regu, bool en_dis)
{
	if ((REGULATOR_REGEN_BIT[regu]) != -1) {
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_REGEN_ASSIGNMENT,
						REGULATOR_REGEN_BIT[regu],
						en_dis));
	} else {
		return ERROR_BAD_INPUT;
	}
	return ERROR_NONE;
}

/*!
 * This function gets the Regen assignment for all regulator
 *
 * @param        regu     	Type of regulator
 * @param        en_dis  	Return value, if true :
 *                              the regulator is enabled by regen.
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regen_assig(t_regulator regu, bool * en_dis)
{
	if ((REGULATOR_REGEN_BIT[regu]) != -1) {
		CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_REGEN_ASSIGNMENT,
						REGULATOR_REGEN_BIT[regu],
						en_dis));
	} else {
		return ERROR_BAD_INPUT;
	}
	return ERROR_NONE;
}

/*!
 * This function sets the Regen polarity.
 * The polarity of the REGEN pin is programmable with the REGENINV bit,
 * when set to "0" it is active high, when set to a "1" it is active low.
 *
 * @param        en_dis  	If true regen is inverted.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_regen_inv(bool en_dis)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_REGEN_ASSIGNMENT,
					BIT_REGEN, en_dis));
	return ERROR_NONE;
}

/*!
 * This function gets the Regen polarity.
 *
 * @param        en_dis  	If true regen is inverted.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regen_inv(bool * en_dis)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_REGEN_ASSIGNMENT,
					BIT_REGEN, en_dis));
	return ERROR_NONE;
}

/*!
 * This function configures 4 buck switcher units: SW1A, SW1B, SW2A
 * and SW2B.
 * The configuration consists in selecting the output voltage, the dvs voltage,
 * the standby voltage, the operation mode, the standby operation mode, the
 * dvs speed, the panic mode and the softstart.
 *
 * @param        sw  	 Define type of switcher.
 * @param        sw_conf Define new configuration.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_conf_switcher(t_switcher sw, struct t_switcher_conf *sw_conf)
{
	/* check variable */
	int error = 0, offset = 0, o_b = 0;
	int conf_sw = 0, conf_2 = 0;

	if (sw_conf->sw_setting > (MASK_SET >> BIT_SET)) {
		error = true;
	}
	if (sw_conf->sw_setting_dvs > (MASK_SET_DVS >> BIT_SET_DVS)) {
		error = true;
	}
	if (sw_conf->sw_setting_standby > (MASK_SET_SDBY >> BIT_SET_SDBY)) {
		error = true;
	}
	if (sw_conf->sw_op_mode > (MASK_OP_MODE >> BIT_OP_MODE)) {
		error = true;
	}
	if (sw_conf->sw_op_mode_standby > (MASK_OP_SDBY >> BIT_OP_SDBY)) {
		error = true;
	}
	if (sw_conf->sw_dvs_speed > (MASK_SPD_DVS >> BIT_SPD_DVS)) {
		error = true;
	}
	if (sw_conf->sw_panic_mode > (MASK_PANIC >> BIT_PANIC)) {
		error = true;
	}
	if (sw_conf->sw_softstart > (MASK_SOFTSTART >> BIT_SOFTSTART)) {
		error = true;
	}
	if (sw > SW_SW2B) {
		error = true;
	}
	if (error) {
		return ERROR_BAD_INPUT;
	}

	conf_sw = sw_conf->sw_setting << BIT_SET |
	    sw_conf->sw_setting_dvs << BIT_SET_DVS |
	    sw_conf->sw_setting_standby << BIT_SET_SDBY;

	conf_2 = sw_conf->sw_op_mode << BIT_OP_MODE |
	    sw_conf->sw_op_mode_standby << BIT_OP_SDBY |
	    sw_conf->sw_dvs_speed << BIT_SPD_DVS |
	    sw_conf->sw_panic_mode << BIT_PANIC |
	    sw_conf->sw_softstart << BIT_SOFTSTART;

	offset = sw;
	CHECK_ERROR(mc13783_set_reg_value(PRIO_PWM, REG_SWITCHERS_0 + offset,
					  BITS_SW, (int)conf_sw, LONG_SW));
	offset = 0;
	if ((sw == SW_SW2A) || (sw == SW_SW2B)) {
		offset = 1;
	}
	o_b = 0;
	if ((sw == SW_SW1B) || (sw == SW_SW2B)) {
		o_b = LONG_CONF_SW + 1;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_PWM, REG_SWITCHERS_4 + offset,
					  BITS_CONF_SW + o_b, (int)conf_2,
					  LONG_CONF_SW));
	return ERROR_NONE;
}

/*!
 * This function returns configuration of one switcher.
 *
 * @param        sw  	 Define type of switcher.
 * @param        sw_conf Return value of configuration.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_switcher(t_switcher sw, struct t_switcher_conf *sw_conf)
{
	/* check variable */
	int offset = 0, o_b = 0;
	int conf_sw = 0, conf_2 = 0;

	offset = sw;
	CHECK_ERROR(mc13783_get_reg_value(PRIO_PWM, REG_SWITCHERS_0 + offset,
					  BITS_SW, (int *)&conf_sw, LONG_SW));
	offset = 0;
	if ((sw == SW_SW2A) || (sw == SW_SW2B)) {
		offset = 1;
	}
	o_b = 0;
	if ((sw == SW_SW1B) || (sw == SW_SW2B)) {
		o_b = LONG_CONF_SW + 1;
	}
	CHECK_ERROR(mc13783_get_reg_value(PRIO_PWM, REG_SWITCHERS_4 + offset,
					  BITS_CONF_SW + o_b, (int *)&conf_2,
					  LONG_CONF_SW));

	sw_conf->sw_setting = ((MASK_SET & conf_sw) >> BIT_SET);

	sw_conf->sw_setting_dvs = ((MASK_SET_DVS & conf_sw) >> BIT_SET_DVS);
	sw_conf->sw_setting_standby = ((MASK_SET_SDBY & conf_sw)
				       >> BIT_SET_SDBY);
	sw_conf->sw_op_mode = ((MASK_OP_MODE & conf_2) >> BIT_OP_MODE);
	sw_conf->sw_op_mode_standby = ((MASK_OP_SDBY & conf_2) >> BIT_OP_SDBY);
	sw_conf->sw_dvs_speed = ((MASK_SPD_DVS & conf_2) >> BIT_SPD_DVS);
	sw_conf->sw_panic_mode = ((MASK_PANIC & conf_2) >> BIT_PANIC);
	sw_conf->sw_softstart = ((MASK_SOFTSTART & conf_2) >> BIT_SOFTSTART);
	return ERROR_NONE;
}

/*!
 * This function enables or disables a regulator.
 *
 * @param        regu           Define the regulator.
 * @param        en_dis  	If true, enable the regulator.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_regu_en(t_regulator regu, bool en_dis)
{
	int reg = 0, regu_index = 0, en_bit_val = 0;

	en_bit_val = REGULATOR_EN_BIT[regu];
	if (en_bit_val >= 200) {
		reg = REG_POWER_MISCELLANEOUS;
		regu_index = en_bit_val - 200;
	} else if (en_bit_val >= 100) {
		reg = REG_REGULATOR_MODE_1;
		regu_index = en_bit_val - 100;
	} else {
		reg = REG_REGULATOR_MODE_0;
		regu_index = regu;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, reg, regu_index, en_dis));
	return ERROR_NONE;
}

/*!
 * This function returns state of regulator.
 *
 * @param        regu           Define the regulator.
 * @param        en_dis  	If true, regulator is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regu_state(t_regulator regu, bool * en_dis)
{
	int reg = 0, regu_index = 0, en_bit_val = 0;

	en_bit_val = REGULATOR_EN_BIT[regu];
	if (en_bit_val >= 200) {
		reg = REG_POWER_MISCELLANEOUS;
		regu_index = en_bit_val - 200;
	} else if (en_bit_val >= 100) {
		reg = REG_REGULATOR_MODE_1;
		regu_index = en_bit_val - 100;
	} else {
		reg = REG_REGULATOR_MODE_0;
		regu_index = regu;
	}
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, reg, regu_index, en_dis));
	return ERROR_NONE;
}

/*!
 * This function configures a regulator.
 *
 * @param        regu           Define the regulator.
 * @param        stby  	        If true, the regulator is controlled by standby.
 * @param        mode  	        Configure the regulator operating mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_conf_regu(t_regulator regu, bool stby, bool mode)
{
	int reg1 = 0, regu_index_sdby = 0, stby_bit_val = 0;
	int reg2 = 0, regu_index_mode = 0, mode_bit_val = 0;

	stby_bit_val = REGULATOR_SDBY_BIT[regu];

	if (stby_bit_val >= 200) {
		reg1 = REG_POWER_MISCELLANEOUS;
		regu_index_sdby = stby_bit_val - 200;
	} else if (stby_bit_val >= 100) {
		reg1 = REG_REGULATOR_MODE_1;
		regu_index_sdby = stby_bit_val - 100;
	} else {
		reg1 = REG_REGULATOR_MODE_0;
		regu_index_sdby = regu;
	}

	mode_bit_val = REGULATOR_OP_MODE_BIT[regu];
	if (mode_bit_val != -1) {
		if (mode_bit_val >= 200) {
			reg2 = REG_POWER_MISCELLANEOUS;
			regu_index_mode = mode_bit_val - 200;
		} else if (stby_bit_val >= 100) {
			reg2 = REG_REGULATOR_MODE_1;
			regu_index_mode = mode_bit_val - 100;
		} else {
			reg2 = REG_REGULATOR_MODE_0;
			regu_index_mode = regu;
		}
		if (reg1 != reg2) {
			return ERROR_BAD_INPUT;
		}
		CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, reg1, regu_index_mode,
						mode));
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, reg1, regu_index_sdby, stby));
	return ERROR_NONE;
}

/*!
 * This function gets configuration of one regulator.
 *
 * @param        regu           Define the regulator.
 * @param        stby  	        If true, the regulator is controlled by standby.
 * @param        mode  	        Return the regulator operating mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regu_conf(t_regulator regu, bool * stby, bool * mode)
{
	int reg1 = 0, regu_index_sdby = 0, stby_bit_val = 0;
	int reg2 = 0, regu_index_mode = 0, mode_bit_val = 0;
	*mode = 0;

	stby_bit_val = REGULATOR_SDBY_BIT[regu];

	if (stby_bit_val >= 200) {
		reg1 = REG_POWER_MISCELLANEOUS;
		regu_index_sdby = stby_bit_val - 200;
	} else if (stby_bit_val >= 100) {
		reg1 = REG_REGULATOR_MODE_1;
		regu_index_sdby = stby_bit_val - 100;
	} else {
		reg1 = REG_REGULATOR_MODE_0;
		regu_index_sdby = regu;
	}

	mode_bit_val = REGULATOR_OP_MODE_BIT[regu];
	if (mode_bit_val != -1) {
		if (mode_bit_val >= 200) {
			reg2 = REG_POWER_MISCELLANEOUS;
			regu_index_mode = mode_bit_val - 200;
		} else if (stby_bit_val >= 100) {
			reg2 = REG_REGULATOR_MODE_1;
			regu_index_mode = mode_bit_val - 100;
		} else {
			reg2 = REG_REGULATOR_MODE_0;
			regu_index_mode = regu;
		}
		if (reg1 != reg2) {
			return ERROR_BAD_INPUT;
		}
		CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, reg1, regu_index_mode,
						mode));
	}

	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, reg1, regu_index_sdby, stby));
	return ERROR_NONE;
}

/*!
 * This function sets value of regulator output voltage.
 *
 * @param        regu           Define the regulator.
 * @param        setting        Define the regulator setting.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_regu(t_regulator regu, int setting)
{
	int reg = 0, size_set = 0, bit_index = 0;
	int regu_val_tab = 0, max_val = 0;

	regu_val_tab = REGULATOR_SETTING_BITS[regu];
	if (regu_val_tab < 0) {
		return ERROR_BAD_INPUT;
	}
	reg = REG_REGULATOR_SETTING_0 + (regu_val_tab / 10000);
	if ((regu_val_tab / 10000) == 1) {
		regu_val_tab -= 10000;
	}
	bit_index = (regu_val_tab / 100);
	size_set = regu_val_tab & 0x0F;
	max_val = (1 << size_set) - 1;
	if (setting > max_val) {
		return ERROR_BAD_INPUT;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_PWM, reg, bit_index, setting,
					  size_set));
	return ERROR_NONE;
}

/*!
 * This function gets value of regulator output voltage.
 *
 * @param        regu           Define the regulator.
 * @param        setting        Return the regulator setting.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regu(t_regulator regu, int *setting)
{
	int reg = 0, size_set = 0, bit_index = 0;
	int regu_val_tab = 0;

	regu_val_tab = REGULATOR_SETTING_BITS[regu];
	if (regu_val_tab < 0) {
		return ERROR_BAD_INPUT;
	}
	reg = REG_REGULATOR_SETTING_0 + (regu_val_tab / 10000);
	if ((regu_val_tab / 10000) == 1) {
		regu_val_tab -= 10000;
	}
	bit_index = (regu_val_tab / 100);
	size_set = regu_val_tab & 0x0F;
	CHECK_ERROR(mc13783_get_reg_value(PRIO_PWM, reg, bit_index,
					  (int *)setting, size_set));
	return ERROR_NONE;
}

/*!
 * This function enables power cut mode.
 * A power cut is defined as a momentary loss of power.
 * In the power cut modes, the backup cell voltage is indirectly monitored
 * by a comparator sensing the on chip VRTC supply.
 *
 * @param        en           If true, power cut enable.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_cut_en(bool en)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_POWER_CUT, en));
	return ERROR_NONE;
}

/*!
 * This function gets power cut mode.
 *
 * @param        en           If true, power cut is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_power_cut(bool * en)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_POWER_CUT, en));
	return ERROR_NONE;
}

/*!
 * This function enables warm start mode.
 * When User Off Wait expired, the Wait mode is exited for User Off mode
 * or Memory Hold mode depending on warm starts being enabled.
 *
 * @param        en           If true, warm start enable.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_warm_start_en(bool en)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_WARM_EN, en));
	return ERROR_NONE;
}

/*!
 * This function gets warm start mode.
 *
 * @param        en           If true, warm start is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_warm_start(bool * en)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_WARM_EN, en));
	return ERROR_NONE;
}

/*!
 * This function send SPI command for entering user off modes.
 *
 * @param        en           If true, entering user off modes
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_user_off_en(bool en)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_USER_OFF, en));
	return ERROR_NONE;
}

/*!
 * This function returns value of user off modes SPI command.
 *
 * @param        en           If true, user off modes is started
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_user_off_en(bool * en)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_USER_OFF, en));
	return ERROR_NONE;
}

/*!
 * This function enables CLK32KMCU clock.
 *
 * @param        en           If true, enable CLK32K
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_32k_en(bool en)
{
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_PWM, REG_POWER_CONTROL_0, BIT_32K_EN, en));
	return ERROR_NONE;
}

/*!
 * This function gets state of CLK32KMCU clock.
 *
 * @param        en           If true, CLK32K is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_32k_state(bool * en)
{
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_PWM, REG_POWER_CONTROL_0, BIT_32K_EN, en));
	return ERROR_NONE;
}

/*!
 * This function enables automatically VBKUP2 in the memory hold modes.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, enable VBKUP2AUTOMH
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_vbkup2_auto_en(bool en)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_VBKUP2AUTOMH, en));
	return ERROR_NONE;
}

/*!
 * This function gets state of automatically VBKUP2.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, VBKUP2AUTOMH is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_vbkup2_auto_state(bool * en)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_VBKUP2AUTOMH, en));
	return ERROR_NONE;
}

/*!
 * This function enables battery detect function.
 * Battery detect comparator will compare the voltage at ADIN5 with BATTDET.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, enable BATTDETEN
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_bat_det_en(bool en)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_BATTDETEN, en));
	return ERROR_NONE;
}

/*!
 * This function gets state of battery detect function.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, BATTDETEN is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_bat_det_state(bool * en)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_BATTDETEN, en));
	return ERROR_NONE;
}

/*!
 * This function enables esim and mmc control voltage.
 * VESIM supplies the eSIM card.
 * The MMC card can be either a hot swap MMC or SD card or an extension module.
 * Only on mc13783 2.0 or higher
 *
 * @param        vesim          If true, enable VESIMESIMEN
 * @param        vmmc1          If true, enable VMMC1ESIMEN
 * @param        vmmc2          If true, enable VMMC2ESIMEN
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_esim_v_en(bool vesim, bool vmmc1, bool vmmc2)
{
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_PWM, REG_REGEN_ASSIGNMENT, BIT_VESIM, vesim));
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_PWM, REG_REGEN_ASSIGNMENT, BIT_VMMC1, vmmc1));
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_PWM, REG_REGEN_ASSIGNMENT, BIT_VMMC2, vmmc2));
	return ERROR_NONE;
}

/*!
 * This function gets esim control voltage values.
 * Only on mc13783 2.0 or higher
 *
 * @param        vesim          If true, enable VESIMESIMEN
 * @param        vmmc1          If true, enable VMMC1ESIMEN
 * @param        vmmc2          If true, enable VMMC2ESIMEN
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_gets_esim_v_state(bool * vesim, bool * vmmc1, bool * vmmc2)
{
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_PWM, REG_REGEN_ASSIGNMENT, BIT_VESIM, vesim));
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_PWM, REG_REGEN_ASSIGNMENT, BIT_VMMC1, vmmc1));
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_PWM, REG_REGEN_ASSIGNMENT, BIT_VMMC2, vmmc2));
	return ERROR_NONE;
}

/*!
 * This function enables control of the regulator for the vibrator motor (VVIB)
 * by VIBEN pin.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, enable VVIB control by VIBEN
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_vib_pin_en(bool en)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_MISCELLANEOUS,
					BIT_VIBPINCTRL, en));
	return ERROR_NONE;
}

/*!
 * This function gets state of control of VVIB by VIBEN pin.
 * Only on mc13783 2.0 or higher
 * @param        en           If true, VIBPINCTRL is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_gets_vib_pin_state(bool * en)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_MISCELLANEOUS,
					BIT_VIBPINCTRL, en));
	return ERROR_NONE;
}

/*!
 * This function configures power cut mode.
 *
 * @param        pc  	Define configuration of power cut mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_cut_conf(struct t_power_cut_conf *pc)
{
	int val_max = 0;

	val_max = (1 << LONG_TIMER) - 1;
	if (pc->pc_timer > val_max) {
		return ERROR_BAD_INPUT;
	}
	val_max = (1 << LONG_CUT_CUNT) - 1;
	if (pc->pc_counter > val_max) {
		return ERROR_BAD_INPUT;
	}
	val_max = (1 << LONG_MAX_PW_CUT) - 1;
	if (pc->pc_max_nb_pc > val_max) {
		return ERROR_BAD_INPUT;
	}
	val_max = (1 << LONG_EXT_PW_CUT) - 1;
	if (pc->pc_ext_timer > val_max) {
		return ERROR_BAD_INPUT;
	}

	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_PC_CNT_EN, pc->pc_counter_en));
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_AUTO_OFF_EN, pc->pc_auto_user_off));
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_KEEP_32K_UOFF,
					pc->pc_user_off_32k_en));
	CHECK_ERROR(mc13783_set_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_1, BITS_TIMER, pc->pc_timer,
		     LONG_TIMER));
	CHECK_ERROR(mc13783_set_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_1, BITS_CUT_CUNT,
		     pc->pc_counter, LONG_CUT_CUNT));
	CHECK_ERROR(mc13783_set_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_1, BITS_MAX_PW_CUT,
		     pc->pc_max_nb_pc, LONG_MAX_PW_CUT));
	CHECK_ERROR(mc13783_set_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_1, BITS_EXT_PW_CUT,
		     pc->pc_ext_timer, LONG_EXT_PW_CUT));
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_PWM, REG_POWER_CONTROL_1, BIT_TIMER_INF,
		     pc->pc_ext_timer_inf));
	return ERROR_NONE;
}

/*!
 * This function gets configuration of power cut mode.
 *
 * @param        pc  	Return configuration of power cut mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_cut_get_conf(struct t_power_cut_conf *pc)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_PC_CNT_EN, &(pc->pc_counter_en)));
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_AUTO_OFF_EN,
					&(pc->pc_auto_user_off)));
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_PWM, REG_POWER_CONTROL_0, BIT_KEEP_32K_UOFF,
		     &(pc->pc_user_off_32k_en)));
	CHECK_ERROR(mc13783_get_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_1, BITS_TIMER, &(pc->pc_timer),
		     LONG_TIMER));
	CHECK_ERROR(mc13783_get_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_1, BITS_CUT_CUNT,
		     &(pc->pc_counter), LONG_CUT_CUNT));
	CHECK_ERROR(mc13783_get_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_1, BITS_MAX_PW_CUT,
		     &(pc->pc_max_nb_pc), LONG_MAX_PW_CUT));
	CHECK_ERROR(mc13783_get_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_1, BITS_EXT_PW_CUT,
		     &(pc->pc_ext_timer), LONG_EXT_PW_CUT));
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_PWM, REG_POWER_CONTROL_1, BIT_TIMER_INF,
		     &(pc->pc_ext_timer_inf)));
	return ERROR_NONE;
}

/*!
 * This function configures v-backup during power cut.
 * Configuration parameters are : enable or not VBKUP; set or nor automatic
 * enabling of VBKUP in the memory hold and user off modes; set VBKUP voltage.
 * VBKUP1 switch is used to supply memory circuit.
 * VBKUP2 switch is used to supply processor cores circuit.
 *
 * @param        vbkup  	Type of VBLUP (1 or2).
 * @param        vbkup_conf  	Define configuration of VBLUP.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_vbkup_conf(t_vbkup vbkup, struct t_vbkup_conf *vbkup_conf)
{
	int offset = 0;
	int max_val = 0;
	max_val = (1 << LONG_VBLUP_VOLT) - 1;
	if (vbkup_conf->vbkup_voltage > max_val) {
		return ERROR_BAD_INPUT;
	}

	if (vbkup == VBKUP_2) {
		offset = VBLUP2_OFFSET;
	}

	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_VBLUP_EN + offset,
					vbkup_conf->vbkup_en));
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_PWM, REG_POWER_CONTROL_0, BIT_VBLUP_AUTO_EN + offset,
		     vbkup_conf->vbkup_auto_en));
	CHECK_ERROR(mc13783_set_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_0, BITS_VBLUP_VOLT + offset,
		     vbkup_conf->vbkup_voltage, LONG_VBLUP_VOLT));
	return ERROR_NONE;
}

/*!
 * This function gets configuration of v-backup.
 *
 * @param        vbkup  	Type of VBLUP (1 or2).
 * @param        vbkup_conf  	Return configuration of VBLUP.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_vbkup_conf(t_vbkup vbkup, struct t_vbkup_conf *vbkup_conf)
{
	int offset = 0;
	if (vbkup == VBKUP_2) {
		offset = VBLUP2_OFFSET;
	}
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_VBLUP_EN + offset,
					&(vbkup_conf->vbkup_en)));
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_PWM, REG_POWER_CONTROL_0, BIT_VBLUP_AUTO_EN + offset,
		     &(vbkup_conf->vbkup_auto_en)));
	CHECK_ERROR(mc13783_get_reg_value
		    (PRIO_PWM, REG_POWER_CONTROL_0, BITS_VBLUP_VOLT + offset,
		     &(vbkup_conf->vbkup_voltage), LONG_VBLUP_VOLT));
	return ERROR_NONE;
}

/*!
 * This function sets BP detection threshold.
 * @verbatim
          BPDET       UVDET  LOBATL        LOBATH          BPON
          ---------   -----  ------------  --------------  ------------
            0         2.6    UVDET + 0.2   UVDET + 0.4     3.2
            1         2.6    UVDET + 0.3   UVDET + 0.5     3.2
            2         2.6    UVDET + 0.4   UVDET + 0.7     3.2
            3         2.6    UVDET + 0.5   UVDET + 0.8     3.2
   @endverbatim
 *
 * @param        threshold           Define threshold
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_bp_threshold(int threshold)
{
	CHECK_ERROR(mc13783_set_reg_value(PRIO_PWM, REG_POWER_CONTROL_0,
					  BITS_BP_THRESHOLD, threshold,
					  LONG_BP_THRESHOLD));
	return ERROR_NONE;
}

/*!
 * This function gets BP detection threshold.
 *
 * @param        threshold           Return threshold
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_bp_threshold(int *threshold)
{
	CHECK_ERROR(mc13783_get_reg_value(PRIO_PWM, REG_POWER_CONTROL_0,
					  BITS_BP_THRESHOLD, (int *)threshold,
					  LONG_BP_THRESHOLD));
	return ERROR_NONE;
}

/*!
 * This function selects EOL function instead of LOBAT.
 * If the EOL function is set, the LOBATL threshold is not monitored but
 * the drop out on the VRF1, VRF2 and VRFREF regulators.
 *
 * @param        eol          If true, selects EOL function
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_eol_function(bool eol)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_EOL_FCT, eol));
	return ERROR_NONE;
}

/*!
 * This function gets EOL function instead of LOBAT.
 *
 * @param        eol          If true, selects EOL function
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_eol_function(bool * eol)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_EOL_FCT, eol));
	return ERROR_NONE;
}

/*!
 * This function sets coincell charger voltage setting.
 * @verbatim
                VCOIN[2:0]      V
                --------------  --------
                0               2.50
                1               2.70
                2               2.80
                3               2.90
                4               3.00
                5               3.10
                6               3.20
                7               3.30
   @endverbatim
 *
 * @param        voltage         Value of voltage setting
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_coincell_voltage(int voltage)
{
	int max_val = 0;
	max_val = (1 << LONG_CHARGE_VOLT) - 1;
	if (voltage > max_val) {
		return ERROR_BAD_INPUT;
	}
	CHECK_ERROR(mc13783_set_reg_value(PRIO_PWM, REG_POWER_CONTROL_0,
					  BITS_CHARGE_VOLT, voltage,
					  LONG_CHARGE_VOLT));
	return ERROR_NONE;
}

/*!
 * This function gets coincell charger voltage setting.
 *
 * @param        voltage         Return value of voltage setting
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_coincell_voltage(int *voltage)
{
	CHECK_ERROR(mc13783_get_reg_value(PRIO_PWM, REG_POWER_CONTROL_0,
					  BITS_CHARGE_VOLT, (int *)voltage,
					  LONG_CHARGE_VOLT));
	return ERROR_NONE;
}

/*!
 * This function enables coincell charger.
 *
 * @param        en         If true, enable the coincell charger
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_coincell_charger_en(bool en)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_CHARGER_EN, en));
	return ERROR_NONE;
}

/*!
 * This function gets state of coincell charger.
 *
 * @param        en         If true, the coincell charger is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_coincell_charger_en(bool * en)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_0,
					BIT_CHARGER_EN, (int *)en));
	return ERROR_NONE;
}

/*!
 * This function enables auto reset after a system reset.
 *
 * @param        en         If true, the auto reset is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_auto_reset_en(bool en)
{
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_2,
					BIT_AUTO_RESTART, en));
	return ERROR_NONE;
}

/*!
 * This function gets auto reset configuration.
 *
 * @param        en         If true, the auto reset is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_auto_reset_en(bool * en)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_2,
					BIT_AUTO_RESTART, (int *)en));
	return ERROR_NONE;
}

/*!
 * This function configures a system reset on a button.
 *
 * @param       bt         Type of button.
 * @param       sys_rst    If true, enable the system reset on this button
 * @param       deb_time   Sets the debounce time on this button pin
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_conf_button(t_button bt, bool sys_rst, int deb_time)
{
	int max_val = 0;
	max_val = (1 << LONG_DEB_TIME_ON) - 1;
	if (deb_time > max_val) {
		return ERROR_BAD_INPUT;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_POWER_CONTROL_2,
					BIT_RESET_ON + bt, sys_rst));

	CHECK_ERROR(mc13783_set_reg_value(PRIO_PWM, REG_POWER_CONTROL_2,
					  BITS_DEB_TIME_ON +
					  bt * LONG_DEB_TIME_ON, deb_time,
					  LONG_DEB_TIME_ON));
	return ERROR_NONE;
}

/*!
 * This function gets configuration of a button.
 *
 * @param       bt         Type of button.
 * @param       sys_rst    If true, the system reset is enabled on this button
 * @param       deb_time   Gets the debounce time on this button pin
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_conf_button(t_button bt, bool * sys_rst, int *deb_time)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_POWER_CONTROL_2,
					BIT_RESET_ON + bt, (int *)sys_rst));

	CHECK_ERROR(mc13783_get_reg_value(PRIO_PWM, REG_POWER_CONTROL_2,
					  BITS_DEB_TIME_ON +
					  bt * LONG_DEB_TIME_ON,
					  (int *)deb_time, LONG_DEB_TIME_ON));
	return ERROR_NONE;
}

/*!
 * This function configures switcher PLL.
 * PLL configuration consists in enabling or not PLL in power off mode and
 * to select the internal multiplication factor.
 *
 * @param       pll_conf   Configuration of PLL.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_conf_pll(struct t_pll_conf *pll_conf)
{
	int max_val = 0;
	max_val = (1 << LONG_PLL_MULTI) - 1;
	if (pll_conf->pll_factor > max_val) {
		return ERROR_BAD_INPUT;
	}

	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_SWITCHERS_4, BITS_PLL_EN,
					pll_conf->pll_en));
	CHECK_ERROR(mc13783_set_reg_value
		    (PRIO_PWM, REG_SWITCHERS_4, BITS_PLL_MULTI,
		     pll_conf->pll_factor, LONG_PLL_MULTI));
	return ERROR_NONE;
}

/*!
 * This function gets configuration of switcher PLL.
 *
 * @param       pll_conf   Configuration of PLL.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_conf_pll(struct t_pll_conf *pll_conf)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_SWITCHERS_4, BITS_PLL_EN,
					(int *)&(pll_conf->pll_en)));
	CHECK_ERROR(mc13783_get_reg_value
		    (PRIO_PWM, REG_SWITCHERS_4, BITS_PLL_MULTI,
		     (int *)&(pll_conf->pll_factor), LONG_PLL_MULTI));
	return ERROR_NONE;
}

/*!
 * This function configures SW3 switcher.
 * SW3 supplies the backlights, and the regulators for the USB.
 * SW3 configuration consists in enabling or not the switch, selecting the
 * voltage, enabling or not control by standby pin and enabling or not
 * low power mode.
 *
 * @param       sw3_conf   Configuration of sw3 switcher.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_sw3(struct t_sw3_conf *sw3_conf)
{
	int max_val = 0;
	max_val = (1 << LONG_SW3_SET) - 1;
	if (sw3_conf->sw3_setting > max_val) {
		return ERROR_BAD_INPUT;
	}
	CHECK_ERROR(mc13783_set_reg_bit(PRIO_PWM, REG_SWITCHERS_5, BIT_SW3_EN,
					sw3_conf->sw3_en));
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_PWM, REG_SWITCHERS_5, BIT_SW3_CTR_DBY,
		     sw3_conf->sw3_ctr_stby));
	CHECK_ERROR(mc13783_set_reg_bit
		    (PRIO_PWM, REG_SWITCHERS_5, BIT_SW3_MODE,
		     sw3_conf->sw3_op_mode));
	CHECK_ERROR(mc13783_set_reg_value
		    (PRIO_PWM, REG_SWITCHERS_5, BITS_SW3_SET,
		     sw3_conf->sw3_setting, LONG_SW3_SET));
	return ERROR_NONE;
}

/*!
 * This function gets configuration of SW3 switcher.
 *
 * @param       sw3_conf   Configuration of sw3 switcher.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_sw3(struct t_sw3_conf *sw3_conf)
{
	CHECK_ERROR(mc13783_get_reg_bit(PRIO_PWM, REG_SWITCHERS_5, BIT_SW3_EN,
					(int *)&(sw3_conf->sw3_en)));
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_PWM, REG_SWITCHERS_5, BIT_SW3_CTR_DBY,
		     (int *)&(sw3_conf->sw3_ctr_stby)));
	CHECK_ERROR(mc13783_get_reg_bit
		    (PRIO_PWM, REG_SWITCHERS_5, BIT_SW3_MODE,
		     (int *)&(sw3_conf->sw3_op_mode)));
	CHECK_ERROR(mc13783_get_reg_value
		    (PRIO_PWM, REG_SWITCHERS_5, BITS_SW3_SET,
		     (int *)&(sw3_conf->sw3_setting), LONG_SW3_SET));
	return ERROR_NONE;
}

/*!
 * This function is used to un/subscribe on power event IT.
 *
 * @param        event  	Type of event.
 * @param        callback  	Event callback function.
 * @param        sub      	Define if Un/subscribe event.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_event(t_pwr_int event, void *callback, bool sub)
{
	type_event_notification power_event;

	mc13783_event_init(&power_event);
	power_event.callback = callback;
	switch (event) {
	case PWR_IT_BPONI:
		power_event.event = EVENT_BPONI;
		break;
	case PWR_IT_LOBATLI:
		power_event.event = EVENT_LOBATLI;
		break;
	case PWR_IT_LOBATHI:
		power_event.event = EVENT_LOBATHI;
		break;
	case PWR_IT_ONOFD1I:
		power_event.event = EVENT_ONOFD1I;
		break;
	case PWR_IT_ONOFD2I:
		power_event.event = EVENT_ONOFD2I;
		break;
	case PWR_IT_ONOFD3I:
		power_event.event = EVENT_ONOFD3I;
		break;
	case PWR_IT_SYSRSTI:
		power_event.event = EVENT_SYSRSTI;
		break;
	case PWR_IT_PWRRDYI:
		power_event.event = EVENT_PWRRDYI;
		break;
	case PWR_IT_PCI:
		power_event.event = EVENT_PCI;
		break;
	case PWR_IT_WARMI:
		power_event.event = EVENT_WARMI;
		break;
	default:
		return ERROR_BAD_INPUT;
		break;
	}
	if (sub == true) {
		CHECK_ERROR(mc13783_event_subscribe(power_event));
	} else {
		CHECK_ERROR(mc13783_event_unsubscribe(power_event));
	}
	return ERROR_NONE;
}

/*!
 * This function is used to subscribe on power event IT.
 *
 * @param        event  	Type of event.
 * @param        callback  	Event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_event_sub(t_pwr_int event, void *callback)
{
	return mc13783_power_event(event, callback, true);
}

/*!
 * This function is used to un subscribe on power event IT.
 *
 * @param        event  	Type of event.
 * @param        callback  	Event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_event_unsub(t_pwr_int event, void *callback)
{
	return mc13783_power_event(event, callback, false);
}

#ifdef __TEST_CODE_ENABLE__

int mc13783_power_check_power(void)
{
	int i = 0;
	int val = 0, val2 = 0;
	struct t_p_up_sense var_p_up_sense;
	struct t_power_cut_conf var_pc, var2_pc;

	CHECK_ERROR(mc13783_power_get_power_mode_sense(&var_p_up_sense));

	CHECK_ERROR(mc13783_power_get_power_cut(&val));
	CHECK_ERROR(mc13783_power_get_warm_start(&val));
	CHECK_ERROR(mc13783_power_get_user_off_en(&val));

	var_pc.pc_counter_en = false;
	var_pc.pc_auto_user_off = false;
	var_pc.pc_user_off_32k_en = false;
	var_pc.pc_timer = 5;
	var_pc.pc_counter = 4;
	var_pc.pc_max_nb_pc = 2;
	var_pc.pc_ext_timer = 3;
	var_pc.pc_ext_timer_inf = false;
	CHECK_ERROR(mc13783_power_cut_conf(&var_pc));

	CHECK_ERROR(mc13783_power_cut_conf(&var_pc));
	CHECK_ERROR(mc13783_power_cut_get_conf(&var2_pc));

	if (var_pc.pc_auto_user_off != var2_pc.pc_auto_user_off) {
		TRACEMSG_POWER(_K_I("Error in pc_auto_user_off"));
	}
	if (var_pc.pc_user_off_32k_en != var2_pc.pc_user_off_32k_en) {
		TRACEMSG_POWER(_K_I("Error in pc_user_off_32k_en"));
	}
	if (var_pc.pc_timer != var2_pc.pc_timer) {
		TRACEMSG_POWER(_K_I("Error in pc_timer"));
	}
	if (var_pc.pc_counter != var2_pc.pc_counter) {
		TRACEMSG_POWER(_K_I("Error in pc_counter"));
	}
	if (var_pc.pc_counter_en != var2_pc.pc_counter_en) {
		TRACEMSG_POWER(_K_I("Error in pc_counter_en"));
	}
	if (var_pc.pc_max_nb_pc != var2_pc.pc_max_nb_pc) {
		TRACEMSG_POWER(_K_I("Error in pc_max_nb_pc"));
	}
	if (var_pc.pc_ext_timer != var2_pc.pc_ext_timer) {
		TRACEMSG_POWER(_K_I("Error in pc_ext_timer"));
	}
	if (var_pc.pc_ext_timer_inf != var2_pc.pc_ext_timer_inf) {
		TRACEMSG_POWER(_K_I("Error in pc_ext_timer_inf"));
	}

	CHECK_ERROR(mc13783_power_set_bp_threshold(2));
	CHECK_ERROR(mc13783_power_get_bp_threshold(&val));
	if (val != 2) {
		TRACEMSG_POWER(_K_I("Error in threshold conf"));
	}
	CHECK_ERROR(mc13783_power_set_bp_threshold(0));

	CHECK_ERROR(mc13783_power_set_eol_function(false));
	CHECK_ERROR(mc13783_power_get_eol_function(&val));
	if (val != false) {
		TRACEMSG_POWER(_K_I("Error in EOL conf"));
	}

	CHECK_ERROR(mc13783_power_set_coincell_voltage(5));
	CHECK_ERROR(mc13783_power_get_coincell_voltage(&val));
	if (val != 5) {
		TRACEMSG_POWER(_K_I("Error in coincell volt conf"));
	}
	CHECK_ERROR(mc13783_power_set_coincell_voltage(0));

	CHECK_ERROR(mc13783_power_coincell_charger_en(false));
	CHECK_ERROR(mc13783_power_get_coincell_charger_en(&val));
	if (val != false) {
		TRACEMSG_POWER(_K_I("Error in charger en volt conf"));
	}

	CHECK_ERROR(mc13783_power_set_auto_reset_en(true));
	CHECK_ERROR(mc13783_power_get_auto_reset_en(&val));
	if (val != true) {
		TRACEMSG_POWER(_K_I("Error in auto reset conf"));
	}
	CHECK_ERROR(mc13783_power_set_auto_reset_en(false));

	for (i = 0; i < BT_ON3B; i++) {
		CHECK_ERROR(mc13783_power_set_conf_button(i, true, 2));
		CHECK_ERROR(mc13783_power_get_conf_button(i, &val, &val2));
		if ((val != true) || (val2 != 2)) {
			TRACEMSG_POWER(_K_I("Error in button conf"));
		}
		CHECK_ERROR(mc13783_power_set_conf_button(i, false, 0));
	}

	CHECK_ERROR(mc13783_power_vbkup2_auto_en(true));
	CHECK_ERROR(mc13783_power_get_vbkup2_auto_state(&val));
	if (val != true) {
		TRACEMSG_POWER(_K_I("Error in auto vbkup conf"));
	}
	CHECK_ERROR(mc13783_power_vbkup2_auto_en(false));

	CHECK_ERROR(mc13783_power_bat_det_en(true));
	CHECK_ERROR(mc13783_power_get_bat_det_state(&val));
	if (val != true) {
		TRACEMSG_POWER(_K_I("Error in bat detect conf"));
	}
	CHECK_ERROR(mc13783_power_bat_det_en(false));

	return ERROR_NONE;
}

int mc13783_power_check_switcher(void)
{
	/* switcher can't be tested on hardware */
	return ERROR_NONE;
}

int mc13783_power_check_regulator(void)
{
	int i = 0;
	int val = 0, val2 = 0;

	for (i = 0; i < REGU_NUMBER; i++) {
		if (((i == REGU_VAUDIO) ||
		     (i == REGU_VCAM) ||
		     (i == REGU_VVIB) ||
		     (i == REGU_VMMC1) || (i == REGU_VMMC2))) {
			val = 0;
			TRACEMSG_POWER(_K_D("TEST Regu conf %d"), i);
			if (!((i == REGU_VSIM) ||
			      (i == REGU_VESIM) || (i == REGU_VVIB))) {

				CHECK_ERROR(mc13783_power_set_regen_assig
					    (i, true));
				CHECK_ERROR(mc13783_power_get_regen_assig
					    (i, &val));
				if (val != true) {
					TRACEMSG_POWER(_K_I
						       ("Error in Regen conf %d"),
						       i);
				}
				CHECK_ERROR(mc13783_power_set_regen_assig
					    (i, false));
			}

			CHECK_ERROR(mc13783_power_regu_en(i, true));
			CHECK_ERROR(mc13783_power_get_regu_state(i, &val));
			if (val != true) {
				TRACEMSG_POWER(_K_I("Error in Regu en %d"), i);
			}
			CHECK_ERROR(mc13783_power_regu_en(i, false));

			if (!((i == REGU_VVIB) ||
			      (i == REGU_VRFBG) ||
			      (i == REGU_GPO1) ||
			      (i == REGU_GPO2) ||
			      (i == REGU_GPO3) || (i == REGU_GPO4))) {
				CHECK_ERROR(mc13783_power_conf_regu
					    (i, true, true));
				CHECK_ERROR(mc13783_power_get_regu_conf
					    (i, &val, &val2));
				if ((val != true) || (val2 != true)) {
					TRACEMSG_POWER(_K_I
						       ("Error in Regu conf %d"),
						       i);
				}
				CHECK_ERROR(mc13783_power_conf_regu
					    (i, false, false));
			}
			if (!((i == REGU_VAUDIO) ||
			      (i == REGU_VIOHI) ||
			      (i == REGU_VRFBG) ||
			      (i == REGU_GPO1) ||
			      (i == REGU_GPO2) ||
			      (i == REGU_GPO3) || (i == REGU_GPO4))) {

				CHECK_ERROR(mc13783_power_set_regu(i, 1));
				CHECK_ERROR(mc13783_power_get_regu(i, &val));
				if (val != 1) {
					TRACEMSG_POWER(_K_I
						       ("Error in regu setting %d"),
						       i);
				}
				CHECK_ERROR(mc13783_power_set_regu(i, 0));
			}
		}
	}

	/* test Vibrator */

	CHECK_ERROR(mc13783_power_regu_en(REGU_VVIB, true));
	CHECK_ERROR(mc13783_power_get_regu_state(REGU_VVIB, &val));
	if (val != true) {
		TRACEMSG_POWER(_K_I("Error in Regu en %d"), i);
	}

	for (i = 0; i <= 3; i++) {
		CHECK_ERROR(mc13783_power_set_regu(REGU_VVIB, i));
		CHECK_ERROR(mc13783_power_get_regu(REGU_VVIB, &val));
		if (val != 1) {
			TRACEMSG_POWER(_K_I("Error in regu setting %d"), i);
		}
		msleep(3000);
	}
	CHECK_ERROR(mc13783_power_set_regu(REGU_VVIB, 0));
	CHECK_ERROR(mc13783_power_regu_en(REGU_VVIB, false));

	CHECK_ERROR(mc13783_power_set_regen_inv(false));
	CHECK_ERROR(mc13783_power_get_regen_inv(&val));
	if (val != true) {
		TRACEMSG_POWER(_K_I("Error in Regen conf"));
	}
	return ERROR_NONE;
}

#endif				/* __TEST_CODE_ENABLE__ */

/*!
 * This function implements IOCTL controls on a mc13783 power device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int mc13783_power_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg)
{

	if (_IOC_TYPE(cmd) != 'P')
		return -ENOTTY;

	switch (cmd) {
	case MC13783_POWER_INIT:
		TRACEMSG_POWER(_K_D("Init Power"));
		CHECK_ERROR(0);
		break;
#ifdef __TEST_CODE_ENABLE__
	case MC13783_POWER_FULL_CHECK:
		TRACEMSG_POWER(_K_D("Check Power"));
		CHECK_ERROR(mc13783_power_check_power());
		CHECK_ERROR(mc13783_power_check_switcher());
		CHECK_ERROR(mc13783_power_check_regulator());
		break;
	case MC13783_POWER_CONTROL_CHECK:
		TRACEMSG_POWER(_K_D("Check Power"));
		CHECK_ERROR(mc13783_power_check_power());
		break;
	case MC13783_SWITCHER_CHECK:
		TRACEMSG_POWER(_K_D("Check Switcher"));
		CHECK_ERROR(mc13783_power_check_switcher());
		break;
	case MC13783_REGULATOR_CHECK:
		TRACEMSG_POWER(_K_D("Check Regulator"));
		CHECK_ERROR(mc13783_power_check_regulator());
		break;
#endif				/* __TEST_CODE_ENABLE__ */
	default:
		TRACEMSG_POWER(_K_D("%d unsupported ioctl command"), (int)cmd);
		return -EINVAL;
	}
	return ERROR_NONE;
}

/*!
 * This function implements the open method on a mc13783 power device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_power_open(struct inode *inode, struct file *file)
{
	return ERROR_NONE;
}

/*!
 * This function implements the release method on a mc13783 power device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_power_release(struct inode *inode, struct file *file)
{
	return ERROR_NONE;
}

static struct file_operations mc13783_power_fops = {
	.owner = THIS_MODULE,
	.ioctl = mc13783_power_ioctl,
	.open = mc13783_power_open,
	.release = mc13783_power_release,
};

int mc13783_power_loaded(void)
{
	return mc13783_power_detected;
}

static int __init mc13783_power_init(void)
{
	int ret = 0;

	if (mc13783_core_loaded() == 0) {
		printk(KERN_INFO MC13783_LOAD_ERROR_MSG);
		return -1;
	}

	mc13783_power_major = register_chrdev(0, "mc13783_power",
					      &mc13783_power_fops);
	if (mc13783_power_major < 0) {
		TRACEMSG_POWER(_K_D("Unable to get a major for mc13783_power"));
		return -1;
	}

	devfs_mk_cdev(MKDEV(mc13783_power_major, 0),
		      S_IFCHR | S_IRUGO | S_IWUSR, "mc13783_power");

	ret = driver_register(&mc13783_power_driver_ldm);
	if (ret == 0) {
		ret = platform_device_register(&mc13783_power_ldm);
		if (ret != 0) {
			driver_unregister(&mc13783_power_driver_ldm);
		} else {
			mc13783_power_detected = 1;
			printk(KERN_INFO "mc13783 Power loaded\n");
		}
	}

	return ret;
}

static void __exit mc13783_power_exit(void)
{
	devfs_remove("mc13783_power");

	driver_unregister(&mc13783_power_driver_ldm);
	platform_device_unregister(&mc13783_power_ldm);
	unregister_chrdev(mc13783_power_major, "mc13783_power");

	printk(KERN_INFO "mc13783_power : successfully unloaded");
}

/*
 * Module entry points
 */

subsys_initcall(mc13783_power_init);
module_exit(mc13783_power_exit);

MODULE_DESCRIPTION("mc13783_power driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
