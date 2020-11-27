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
 * @file pmic_power.c
 * @brief This is the main file of sc55112 Power Control driver.
 *
 * @ingroup PMIC_sc55112_POWER
 */

/*
 * Includes
 */
#include <asm/ioctl.h>
#include <linux/device.h>

#include "../core/sc55112_config.h"
#include "sc55112_power_regs.h"
#include <asm/arch/pmic_status.h>
#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_power.h>

static int pmic_power_major;
static int mmc_voltage = 0;

/*
 * PMIC Power Control API
 */

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(pmic_power_off);
EXPORT_SYMBOL(pmic_power_set_pc_config);
EXPORT_SYMBOL(pmic_power_get_pc_config);
EXPORT_SYMBOL(pmic_power_regulator_on);
EXPORT_SYMBOL(pmic_power_regulator_off);
EXPORT_SYMBOL(pmic_power_regulator_set_voltage);
EXPORT_SYMBOL(pmic_power_regulator_get_voltage);
EXPORT_SYMBOL(pmic_power_regulator_set_config);
EXPORT_SYMBOL(pmic_power_regulator_get_config);

/*!
 * This is the suspend of power management for the sc55112 Power Control
 * API module.
 *
 * @param        dev            the device
 * @param        state          the state
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int pmic_power_suspend(struct device *dev, u32 state, u32 level)
{
	/* not supported */
	return -1;
};

/*!
 * This is the resume of power management for the sc55112  adc API.
 * It suports RESTORE state.
 *
 * @param        dev            the device
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int pmic_power_resume(struct device *dev, u32 level)
{
	/* not supported */
	return -1;
};

static struct device_driver pmic_power_driver_ldm = {
	.name = "PMIC_POWER",
	.bus = &platform_bus_type,
	.probe = NULL,
	.remove = NULL,
	.suspend = pmic_power_suspend,
	.resume = pmic_power_resume,
};

/*!
 * This function sets user power off in power control register and thus powers
 * off the phone.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_power_off(void)
{
	return pmic_write_reg(PRIO_PWM, REG_PWRCTRL,
			      BITFVAL(sc55112_PWRCTRL_USER_OFF_SPI,
				      sc55112_PWRCTRL_USER_OFF_SPI_ENABLE),
			      BITFMASK(sc55112_PWRCTRL_USER_OFF_SPI));
}

/*!
 * This function sets the power control configuration.
 *
 * @param        pc_config   power control configuration.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_power_set_pc_config(t_pc_config * pc_config)
{
	unsigned int pwrctrl_val = 0;
	unsigned int rtc_day_val = 0, rtc_day_mask = 0;

	if (pc_config == NULL) {
		return PMIC_PARAMETER_ERROR;
	}

	pwrctrl_val = 0;

	if (pc_config->pc_enable != false) {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_PCEN, sc55112_PWRCTRL_PCEN_ENABLE);
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_PCT, pc_config->pc_timer);
	} else {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_PCEN, sc55112_PWRCTRL_PCEN_DISABLE);
	}

	if (pc_config->pc_count_enable != false) {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_PC_COUNT_EN,
			    sc55112_PWRCTRL_PC_COUNT_EN_ENABLE);
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_PC_COUNT, pc_config->pc_count);
		rtc_day_val =
		    BITFVAL(sc55112_RTC_DAY_PC_MAX_CNT,
			    pc_config->pc_max_count);
		rtc_day_mask = BITFMASK(sc55112_RTC_DAY_PC_MAX_CNT);
	} else {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_PC_COUNT_EN,
			    sc55112_PWRCTRL_PC_COUNT_EN_DISABLE);
	}

	if (pc_config->warm_enable != false) {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_WARM_EN,
			    sc55112_PWRCTRL_WARM_EN_ENABLE);
	} else {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_WARM_EN,
			    sc55112_PWRCTRL_WARM_EN_DISABLE);
	}

	if (pc_config->clk_32k_enable != false) {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_32OUT_EN,
			    sc55112_PWRCTRL_32OUT_EN_ENABLE);
	} else {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_32OUT_EN,
			    sc55112_PWRCTRL_32OUT_EN_DISABLE);
	}

	pwrctrl_val |= BITFVAL(sc55112_PWRCTRL_VHOLD, pc_config->vhold_voltage);

	if (pc_config->clk_32k_enable != false) {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_32OUT_EN,
			    sc55112_PWRCTRL_32OUT_EN_ENABLE);
	} else {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_32OUT_EN,
			    sc55112_PWRCTRL_32OUT_EN_DISABLE);
	}

	pwrctrl_val |= BITFVAL(sc55112_PWRCTRL_VHOLD, pc_config->vhold_voltage);

	if (pc_config->mem_allon != false) {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_MEM_ALLON,
			    sc55112_PWRCTRL_MEM_ALLON_ENABLE);
	} else {
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_MEM_ALLON,
			    sc55112_PWRCTRL_MEM_ALLON_DISABLE);
		pwrctrl_val |=
		    BITFVAL(sc55112_PWRCTRL_MEM_TMR, pc_config->mem_timer);
	}

	CHECK_ERROR(pmic_write_reg
		    (PRIO_PWM, REG_PWRCTRL, pwrctrl_val, 0xFFFFFF));

	if (pc_config->pc_count_enable != false) {
		CHECK_ERROR(pmic_write_reg
			    (PRIO_PWM, REG_RTC_DAY, rtc_day_val, rtc_day_mask));
	}

	return PMIC_SUCCESS;
}

/*!
 * This function retrives the power control configuration.
 *
 * @param        pc_config   pointer to power control configuration.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_power_get_pc_config(t_pc_config * pc_config)
{
	unsigned int pwrctrl_val = 0, rtc_day_val = 0;

	if (pc_config == NULL) {
		return PMIC_PARAMETER_ERROR;
	}

	CHECK_ERROR(pmic_read_reg(PRIO_PWM, REG_PWRCTRL, &pwrctrl_val));

	if (BITFEXT(pwrctrl_val, sc55112_PWRCTRL_PCEN) ==
	    sc55112_PWRCTRL_PCEN_ENABLE) {
		pc_config->pc_enable = true;
		pc_config->pc_timer = BITFEXT(pwrctrl_val, sc55112_PWRCTRL_PCT);

	} else {
		pc_config->pc_enable = false;
		pc_config->pc_timer = 0;
	}

	if (BITFEXT(pwrctrl_val, sc55112_PWRCTRL_PC_COUNT_EN) ==
	    sc55112_PWRCTRL_PCEN_ENABLE) {
		pc_config->pc_count_enable = true;
		pc_config->pc_count =
		    BITFEXT(pwrctrl_val, sc55112_PWRCTRL_PC_COUNT);
		CHECK_ERROR(pmic_read_reg(PRIO_PWM, REG_RTC_DAY, &rtc_day_val));
		pc_config->pc_max_count =
		    BITFEXT(rtc_day_val, sc55112_RTC_DAY_PC_MAX_CNT);
	} else {
		pc_config->pc_count_enable = false;
		pc_config->pc_count = 0;
		pc_config->pc_max_count = 0;
	}

	if (BITFEXT(pwrctrl_val, sc55112_PWRCTRL_WARM_EN) ==
	    sc55112_PWRCTRL_WARM_EN_ENABLE) {
		pc_config->warm_enable = true;
	} else {
		pc_config->warm_enable = false;
	}

	if (BITFEXT(pwrctrl_val, sc55112_PWRCTRL_32OUT_EN) ==
	    sc55112_PWRCTRL_32OUT_EN_ENABLE) {
		pc_config->clk_32k_enable = false;
	} else {
		pc_config->clk_32k_enable = false;
	}

	pc_config->vhold_voltage = BITFEXT(pwrctrl_val, sc55112_PWRCTRL_VHOLD);

	if (BITFEXT(pwrctrl_val, sc55112_PWRCTRL_32OUT_EN) ==
	    sc55112_PWRCTRL_32OUT_EN_ENABLE) {
		pc_config->clk_32k_enable = false;
	} else {
		pc_config->clk_32k_enable = false;
	}

	pwrctrl_val |= BITFVAL(sc55112_PWRCTRL_VHOLD, pc_config->vhold_voltage);

	if (BITFEXT(pwrctrl_val, sc55112_PWRCTRL_MEM_ALLON) ==
	    sc55112_PWRCTRL_MEM_ALLON_ENABLE) {
		pc_config->mem_allon = true;
		pc_config->mem_timer = 0;
	} else {
		pc_config->mem_allon = false;
		pc_config->mem_timer =
		    BITFEXT(pwrctrl_val, sc55112_PWRCTRL_MEM_TMR);
	}

	return PMIC_SUCCESS;
}

/*!
 * This function turns on a regulator.
 *
 * @param        regulator    The regulator to be truned on.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_power_regulator_on(t_pmic_regulator regulator)
{
	unsigned int reg_val = 0, reg_mask = 0;
	bool is_sw = false;

	switch (regulator) {
	case SW1:
		reg_val =
		    BITFVAL(sc55112_SWCTRL_SW1_EN,
			    sc55112_SWCTRL_SW1_EN_ENABLE);
		reg_mask = BITFMASK(sc55112_SWCTRL_SW1_EN);
		is_sw = true;
		break;

	case SW2:
		reg_val =
		    BITFVAL(sc55112_SWCTRL_SW2_EN,
			    sc55112_SWCTRL_SW2_EN_ENABLE);
		reg_mask = BITFMASK(sc55112_SWCTRL_SW2_EN);
		is_sw = true;
		break;

	case SW3:
		reg_val =
		    BITFVAL(sc55112_SWCTRL_SW3_EN,
			    sc55112_SWCTRL_SW3_EN_ENABLE);
		reg_mask = BITFMASK(sc55112_SWCTRL_SW3_EN);
		is_sw = true;
		break;

	case V1:
		reg_val =
		    BITFVAL(sc55112_VREG_V1_EN, sc55112_VREG_V1_EN_ENABLE);
		reg_mask = BITFMASK(sc55112_VREG_V1_EN);
		break;

	case V2:
		reg_val =
		    BITFVAL(sc55112_VREG_V2_EN, sc55112_VREG_V2_EN_ENABLE);
		reg_mask = BITFMASK(sc55112_VREG_V2_EN);
		break;

	case V3:
		reg_val =
		    BITFVAL(sc55112_VREG_V3_EN, sc55112_VREG_V3_EN_ENABLE);
		reg_mask = BITFMASK(sc55112_VREG_V3_EN);
		break;

	case V4:
		reg_val =
		    BITFVAL(sc55112_VREG_V4_EN, sc55112_VREG_V4_EN_ENABLE);
		reg_mask = BITFMASK(sc55112_VREG_V4_EN);
		break;

	case VSIM:
		reg_val =
		    BITFVAL(sc55112_VREG_VSIM_EN, sc55112_VREG_VSIM_EN_ENABLE);
		reg_mask = BITFMASK(sc55112_VREG_VSIM_EN);
		break;

	case V_VIB:
		reg_val =
		    BITFVAL(sc55112_VREG_V_VIB_EN,
			    sc55112_VREG_V_VIB_EN_ENABLE);
		reg_mask = BITFMASK(sc55112_VREG_V_VIB_EN);
		break;

	case VMMC:
		reg_val = BITFVAL(sc55112_VREG_VMMC, mmc_voltage);
		reg_mask = BITFMASK(sc55112_VREG_VMMC);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	if (is_sw == true) {
		CHECK_ERROR(pmic_write_reg
			    (PRIO_PWM, REG_SWCTRL, reg_val, reg_mask));
	} else {
		CHECK_ERROR(pmic_write_reg
			    (PRIO_PWM, REG_VREG, reg_val, reg_mask));
	}

	return PMIC_SUCCESS;
}

/*!
 * This function turns off a regulator.
 *
 * @param        regulator    The regulator to be truned off.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_power_regulator_off(t_pmic_regulator regulator)
{
	unsigned int reg_val = 0, reg_mask = 0;
	bool is_sw;

	is_sw = false;

	switch (regulator) {
	case SW1:
		reg_val =
		    BITFVAL(sc55112_SWCTRL_SW1_EN,
			    sc55112_SWCTRL_SW1_EN_DISABLE);
		reg_mask = BITFMASK(sc55112_SWCTRL_SW1_EN);
		is_sw = true;
		break;

	case SW2:
		reg_val =
		    BITFVAL(sc55112_SWCTRL_SW2_EN,
			    sc55112_SWCTRL_SW2_EN_DISABLE);
		reg_mask = BITFMASK(sc55112_SWCTRL_SW2_EN);
		is_sw = true;
		break;

	case SW3:
		reg_val =
		    BITFVAL(sc55112_SWCTRL_SW3_EN,
			    sc55112_SWCTRL_SW3_EN_DISABLE);
		reg_mask = BITFMASK(sc55112_SWCTRL_SW3_EN);
		is_sw = true;
		break;

	case V1:
		reg_val =
		    BITFVAL(sc55112_VREG_V1_EN, sc55112_VREG_V1_EN_DISABLE);
		reg_mask = BITFMASK(sc55112_VREG_V1_EN);
		break;

	case V2:
		reg_val =
		    BITFVAL(sc55112_VREG_V2_EN, sc55112_VREG_V2_EN_DISABLE);
		reg_mask = BITFMASK(sc55112_VREG_V2_EN);
		break;

	case V3:
		reg_val =
		    BITFVAL(sc55112_VREG_V3_EN, sc55112_VREG_V3_EN_DISABLE);
		reg_mask = BITFMASK(sc55112_VREG_V3_EN);
		break;

	case V4:
		reg_val =
		    BITFVAL(sc55112_VREG_V4_EN, sc55112_VREG_V4_EN_DISABLE);
		reg_mask = BITFMASK(sc55112_VREG_V4_EN);
		break;

	case VSIM:
		reg_val =
		    BITFVAL(sc55112_VREG_VSIM_EN, sc55112_VREG_VSIM_EN_DISABLE);
		reg_mask = BITFMASK(sc55112_VREG_VSIM_EN);
		break;

	case V_VIB:
		reg_val =
		    BITFVAL(sc55112_VREG_V_VIB_EN,
			    sc55112_VREG_V_VIB_EN_DISABLE);
		reg_mask = BITFMASK(sc55112_VREG_V_VIB_EN);
		break;

	case VMMC:
		reg_val = BITFVAL(sc55112_VREG_VMMC, sc55112_VREG_VMMC_OFF);
		reg_mask = BITFMASK(sc55112_VREG_VMMC);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	if (is_sw == true) {
		CHECK_ERROR(pmic_write_reg
			    (PRIO_PWM, REG_SWCTRL, reg_val, reg_mask));
	} else {
		CHECK_ERROR(pmic_write_reg
			    (PRIO_PWM, REG_VREG, reg_val, reg_mask));
	}

	return PMIC_SUCCESS;
}

/*!
 * This function sets the regulator output voltage.
 *
 * @param        regulator    The regulator to be truned off.
 * @param        voltage      The regulator output voltage.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_power_regulator_set_voltage(t_pmic_regulator regulator,
					     t_regulator_voltage voltage)
{
	unsigned int reg_val = 0, reg_mask = 0;
	int is_sw;

	is_sw = false;

	switch (regulator) {
	case SW1:
		if ((voltage.sw1 < SW1_1V) || (voltage.sw1 > SW1_1_875V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val = BITFVAL(sc55112_SWCTRL_SW1, voltage.sw1);
		reg_mask = BITFMASK(sc55112_SWCTRL_SW1);
		is_sw = true;
		break;

	case SW2:
		if ((voltage.sw2 < SW2_1V) || (voltage.sw2 > SW2_1_875V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val = BITFVAL(sc55112_SWCTRL_SW2, voltage.sw2);
		reg_mask = BITFMASK(sc55112_SWCTRL_SW2);
		is_sw = true;
		break;

	case SW3:
		if ((voltage.sw3 < SW3_5_1V) || (voltage.sw3 > SW3_5_6V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val = BITFVAL(sc55112_SWCTRL_SW3, voltage.sw3);
		reg_mask = BITFMASK(sc55112_SWCTRL_SW3);
		is_sw = true;
		break;

	case V1:
		if ((voltage.v1 < V1_2_775V) || (voltage.v1 > V1_2_475V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val = BITFVAL(sc55112_VREG_V1, voltage.v1);
		reg_mask = BITFMASK(sc55112_VREG_V1);
		break;

	case V2:
		if (voltage.v2 != V2_2_775V) {
			return PMIC_PARAMETER_ERROR;
		} else {
			return PMIC_SUCCESS;
		}
		break;

	case V3:
		if ((voltage.v3 < V3_1_875V) || (voltage.v3 > V3_2_775V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val = BITFVAL(sc55112_VREG_V3, voltage.v3);
		reg_mask = BITFMASK(sc55112_VREG_V3);
		break;

	case V4:
		if (voltage.v4 != V4_2_775V) {
			return PMIC_PARAMETER_ERROR;
		} else {
			return PMIC_SUCCESS;
		}
		break;

	case VSIM:
		if ((voltage.vsim < VSIM_1_8V) || (voltage.v3 > VSIM_3V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val = BITFVAL(sc55112_VREG_VSIM, voltage.vsim);
		reg_mask = BITFMASK(sc55112_VREG_VSIM);
		break;

	case V_VIB:
		if ((voltage.v_vib < V_VIB_2V) || (voltage.v_vib > V_VIB_3V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val = BITFVAL(sc55112_VREG_V_VIB, voltage.v_vib);
		reg_mask = BITFMASK(sc55112_VREG_V_VIB);
		break;

	case VMMC:
		if ((voltage.vmmc < VMMC_OFF) || (voltage.vmmc > VMMC_3_4V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val = BITFVAL(sc55112_VREG_VMMC, voltage.vmmc);
		reg_mask = BITFMASK(sc55112_VREG_VMMC);
		mmc_voltage = voltage.vmmc;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	if (is_sw == true) {
		CHECK_ERROR(pmic_write_reg
			    (PRIO_PWM, REG_SWCTRL, reg_val, reg_mask));
	} else {
		CHECK_ERROR(pmic_write_reg
			    (PRIO_PWM, REG_VREG, reg_val, reg_mask));
	}

	return PMIC_SUCCESS;
}

/*!
 * This function retrives the regulator output voltage.
 *
 * @param        regulator    The regulator to be truned off.
 * @param        voltage      Pointer to regulator output voltage.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_power_regulator_get_voltage(t_pmic_regulator regulator,
					     t_regulator_voltage * voltage)
{
	unsigned int reg_val = 0;

	if ((regulator == SW1) || (regulator == SW2) || (regulator == SW3)) {
		CHECK_ERROR(pmic_read_reg(PRIO_PWM, REG_SWCTRL, &reg_val));
	} else {
		CHECK_ERROR(pmic_read_reg(PRIO_PWM, REG_VREG, &reg_val));
	}

	switch (regulator) {
	case SW1:
		voltage->sw1 = BITFEXT(reg_val, sc55112_SWCTRL_SW1);
		break;

	case SW2:
		voltage->sw2 = BITFEXT(reg_val, sc55112_SWCTRL_SW2);
		break;

	case SW3:
		voltage->sw3 = BITFEXT(reg_val, sc55112_SWCTRL_SW3);
		break;

	case V1:
		voltage->v1 = BITFEXT(reg_val, sc55112_VREG_V1);
		break;

	case V2:
		voltage->v2 = V2_2_775V;
		break;

	case V3:
		voltage->v3 = BITFEXT(reg_val, sc55112_VREG_V3);
		break;

	case V4:
		voltage->v4 = V4_2_775V;
		break;

	case VSIM:
		voltage->vsim = BITFEXT(reg_val, sc55112_VREG_VSIM);
		break;

	case V_VIB:
		voltage->v_vib = BITFEXT(reg_val, sc55112_VREG_V_VIB);
		break;

	case VMMC:
		voltage->vmmc = BITFEXT(reg_val, sc55112_VREG_VMMC);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function sets the regulator configuration.
 *
 * @param        regulator    The regulator to be truned off.
 * @param        config       The regulator output configuration.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_power_regulator_set_config(t_pmic_regulator regulator,
					    t_regulator_config * config)
{
	unsigned int reg_val = 0, reg_mask = 0;
	int is_sw = false;

	switch (regulator) {
	case SW1:
		is_sw = true;

		if (config->mode == SYNC_RECT) {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW1_MODE,
				    sc55112_SWCTRL_SW1_MODE_SYNC_RECT_EN);
		} else {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW1_MODE,
				    sc55112_SWCTRL_SW1_MODE_PULSE_SKIP_EN);
		}
		reg_mask |= BITFMASK(sc55112_SWCTRL_SW1_MODE);

		if ((config->voltage.sw1 < SW1_1V)
		    || (config->voltage.sw1 > SW1_1_875V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |= BITFVAL(sc55112_SWCTRL_SW1, config->voltage.sw1);
		reg_mask |= BITFMASK(sc55112_SWCTRL_SW1);

		if ((config->voltage_lvs.sw1 < SW1_1V)
		    || (config->voltage_lvs.sw1 > SW1_1_875V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |=
		    BITFVAL(sc55112_SWCTRL_SW1_LVS, config->voltage_lvs.sw1);
		reg_mask |= BITFMASK(sc55112_SWCTRL_SW1_LVS);

		if ((config->voltage_stby.sw1 < SW1_1V)
		    || (config->voltage_stby.sw1 > SW1_1_875V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |=
		    BITFVAL(sc55112_SWCTRL_SW1_STBY, config->voltage_stby.sw1);
		reg_mask |= BITFMASK(sc55112_SWCTRL_SW1_STBY);

		if (config->lp_mode == LOW_POWER_DISABLED) {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW1_STBY_LVS_EN,
				    sc55112_SWCTRL_SW1_STBY_LVS_EN_DISABLE);
			reg_mask |= BITFMASK(sc55112_SWCTRL_SW1_STBY_LVS_EN);
		} else if (config->lp_mode == LOW_POWER_CTRL_BY_PIN) {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW1_STBY_LVS_EN,
				    sc55112_SWCTRL_SW1_STBY_LVS_EN_ENABLE);
			reg_mask |= BITFMASK(sc55112_SWCTRL_SW1_STBY_LVS_EN);
		}

		break;

	case SW2:
		is_sw = true;

		if (config->mode == SYNC_RECT) {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW2_MODE,
				    sc55112_SWCTRL_SW2_MODE_SYNC_RECT_EN);
		} else {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW2_MODE,
				    sc55112_SWCTRL_SW2_MODE_PULSE_SKIP_EN);
		}
		reg_mask |= BITFMASK(sc55112_SWCTRL_SW2_MODE);

		if ((config->voltage.sw2 < SW2_1V)
		    || (config->voltage.sw2 > SW2_1_875V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |= BITFVAL(sc55112_SWCTRL_SW2, config->voltage.sw2);
		reg_mask |= BITFMASK(sc55112_SWCTRL_SW2);

		if ((config->voltage_lvs.sw2 < SW2_1V)
		    || (config->voltage_lvs.sw2 > SW2_1_875V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |=
		    BITFVAL(sc55112_SWCTRL_SW2_LVS, config->voltage_lvs.sw2);
		reg_mask |= BITFMASK(sc55112_SWCTRL_SW2_LVS);

		if (config->lp_mode == LOW_POWER_DISABLED) {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW2_DFVS_EN,
				    sc55112_SWCTRL_SW2_DFVS_EN_DISABLE);
			reg_mask |= BITFMASK(sc55112_SWCTRL_SW2_DFVS_EN);
		} else if (config->lp_mode == LOW_POWER_CTRL_BY_PIN) {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW2_DFVS_EN,
				    sc55112_SWCTRL_SW2_DFVS_EN_ENABLE);
			reg_mask |= BITFMASK(sc55112_SWCTRL_SW2_DFVS_EN);
		}
		break;

	case SW3:
		is_sw = true;

		if ((config->voltage.sw3 < SW3_5_1V)
		    || (config->voltage.sw3 > SW3_5_6V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |= BITFVAL(sc55112_SWCTRL_SW3, config->voltage.sw3);
		reg_mask |= BITFMASK(sc55112_SWCTRL_SW3);

		if (config->lp_mode == LOW_POWER_DISABLED) {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW3_STBY_EN,
				    sc55112_SWCTRL_SW3_STBY_EN_DISABLE);
			reg_mask |= BITFMASK(sc55112_SWCTRL_SW3_STBY_EN);
		} else if (config->lp_mode == LOW_POWER_CTRL_BY_PIN) {
			reg_val |=
			    BITFVAL(sc55112_SWCTRL_SW3_STBY_EN,
				    sc55112_SWCTRL_SW3_STBY_EN_ENABLE);
			reg_mask |= BITFMASK(sc55112_SWCTRL_SW3_STBY_EN);
		}
		break;

	case V1:
		if ((config->voltage.v1 < V1_2_775V)
		    || (config->voltage.v1 > V1_2_475V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |= BITFVAL(sc55112_VREG_V1, config->voltage.v1);
		reg_mask |= BITFMASK(sc55112_VREG_V1);

		if (config->lp_mode == LOW_POWER) {
			reg_val |=
			    BITFVAL(sc55112_VREG_V1_LP,
				    sc55112_VREG_V1_LP_ENABLE);
			reg_mask |= BITFMASK(sc55112_VREG_V1_LP);
		} else if (config->lp_mode == LOW_POWER_CTRL_BY_PIN) {
			reg_val |=
			    BITFVAL(sc55112_VREG_V1_LP, sc55112_VREG_V1_LP_CTL);
			reg_mask |= BITFMASK(sc55112_VREG_V1_LP);
			reg_val |=
			    BITFVAL(sc55112_VREG_V1_STBY,
				    sc55112_VREG_V1_STBY_LOW_POWER);
			reg_mask |= BITFMASK(sc55112_VREG_V1_STBY);
		} else {
			reg_val |=
			    BITFVAL(sc55112_VREG_V1_LP, sc55112_VREG_V1_LP_CTL);
			reg_mask |= BITFMASK(sc55112_VREG_V1_LP);
			reg_val |=
			    BITFVAL(sc55112_VREG_V1_STBY,
				    sc55112_VREG_V1_STBY_NO_INTERACTION);
			reg_mask |= BITFMASK(sc55112_VREG_V1_STBY);
		}
		break;

	case V2:
		if (config->lp_mode == LOW_POWER) {
			reg_val |=
			    BITFVAL(sc55112_VREG_V2_LP,
				    sc55112_VREG_V2_LP_ENABLE);
			reg_mask |= BITFMASK(sc55112_VREG_V2_LP);
		} else if (config->lp_mode == LOW_POWER_CTRL_BY_PIN) {
			reg_val |=
			    BITFVAL(sc55112_VREG_V2_LP, sc55112_VREG_V2_LP_CTL);
			reg_mask |= BITFMASK(sc55112_VREG_V2_LP);
			reg_val |=
			    BITFVAL(sc55112_VREG_V2_STBY,
				    sc55112_VREG_V2_STBY_LOW_POWER);
			reg_mask |= BITFMASK(sc55112_VREG_V2_STBY);
		} else {
			reg_val |=
			    BITFVAL(sc55112_VREG_V2_LP, sc55112_VREG_V2_LP_CTL);
			reg_mask |= BITFMASK(sc55112_VREG_V2_LP);
			reg_val |=
			    BITFVAL(sc55112_VREG_V2_STBY,
				    sc55112_VREG_V2_STBY_NO_INTERACTION);
			reg_mask |= BITFMASK(sc55112_VREG_V2_STBY);
		}

		break;

	case V3:
		if ((config->voltage.v3 < V3_1_875V)
		    || (config->voltage.v3 > V3_2_775V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |= BITFVAL(sc55112_VREG_V3, config->voltage.v3);
		reg_mask |= BITFMASK(sc55112_VREG_V3);

		if (config->lp_mode == LOW_POWER) {
			reg_val |=
			    BITFVAL(sc55112_VREG_V3_LP,
				    sc55112_VREG_V3_LP_ENABLE);
			reg_mask |= BITFMASK(sc55112_VREG_V3_LP);
		} else if (config->lp_mode == LOW_POWER_CTRL_BY_PIN) {
			reg_val |=
			    BITFVAL(sc55112_VREG_V3_LP, sc55112_VREG_V3_LP_CTL);
			reg_mask |= BITFMASK(sc55112_VREG_V3_LP);
			reg_val |=
			    BITFVAL(sc55112_VREG_V3_STBY,
				    sc55112_VREG_V3_STBY_LOW_POWER);
			reg_mask |= BITFMASK(sc55112_VREG_V3_STBY);
		} else {
			reg_val |=
			    BITFVAL(sc55112_VREG_V3_LP, sc55112_VREG_V3_LP_CTL);
			reg_mask |= BITFMASK(sc55112_VREG_V3_LP);
			reg_val |=
			    BITFVAL(sc55112_VREG_V3_STBY,
				    sc55112_VREG_V3_STBY_NO_INTERACTION);
			reg_mask |= BITFMASK(sc55112_VREG_V3_STBY);
		}
		break;

	case V4:
		if (config->voltage.v4 != V4_2_775V) {
			return PMIC_PARAMETER_ERROR;
		}

		if (config->lp_mode == LOW_POWER_DISABLED) {
			reg_val |=
			    BITFVAL(sc55112_VREG_V4_STBY,
				    sc55112_VREG_V4_STBY_NO_INTERACTION);
			reg_mask |= BITFMASK(sc55112_VREG_V4_STBY);
		} else if (config->lp_mode == LOW_POWER_CTRL_BY_PIN) {
			reg_val |=
			    BITFVAL(sc55112_VREG_V4_STBY,
				    sc55112_VREG_V4_STBY_LOW_POWER);
			reg_mask |= BITFMASK(sc55112_VREG_V4_STBY);
		}
		break;

	case VSIM:
		if ((config->voltage.vsim < VSIM_1_8V)
		    || (config->voltage.v3 > VSIM_3V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |= BITFVAL(sc55112_VREG_VSIM, config->voltage.vsim);
		reg_mask |= BITFMASK(sc55112_VREG_VSIM);
		break;

	case V_VIB:
		if ((config->voltage.v_vib < V_VIB_2V)
		    || (config->voltage.v_vib > V_VIB_3V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |= BITFVAL(sc55112_VREG_V_VIB, config->voltage.v_vib);
		reg_mask = BITFMASK(sc55112_VREG_V_VIB);
		break;

	case VMMC:
		if ((config->voltage.vmmc < VMMC_OFF)
		    || (config->voltage.vmmc > VMMC_3_4V)) {
			return PMIC_PARAMETER_ERROR;
		}
		reg_val |= BITFVAL(sc55112_VREG_VMMC, config->voltage.vmmc);
		reg_mask |= BITFMASK(sc55112_VREG_VMMC);
		mmc_voltage = config->voltage.vmmc;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	if (is_sw == true) {
		CHECK_ERROR(pmic_write_reg
			    (PRIO_PWM, REG_SWCTRL, reg_val, reg_mask));
	} else {
		CHECK_ERROR(pmic_write_reg
			    (PRIO_PWM, REG_VREG, reg_val, reg_mask));
	}

	return PMIC_SUCCESS;
}

/*!
 * This function retrives the regulator output configuration.
 *
 * @param        regulator    The regulator to be truned off.
 * @param        config       Pointer to regulator configuration.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_power_regulator_get_config(t_pmic_regulator regulator,
					    t_regulator_config * config)
{
	unsigned int reg_val = 0;

	if ((regulator == SW1) || (regulator == SW2) || (regulator == SW3)) {
		CHECK_ERROR(pmic_read_reg(PRIO_PWM, REG_SWCTRL, &reg_val));
	} else {
		CHECK_ERROR(pmic_read_reg(PRIO_PWM, REG_VREG, &reg_val));
	}

	config->mode = 0;
	config->voltage.sw1 = 0;
	config->voltage_lvs.sw1 = 0;
	config->voltage_stby.sw1 = 0;
	config->lp_mode = 0;

	switch (regulator) {
	case SW1:
		if (BITFEXT(reg_val, sc55112_SWCTRL_SW1_MODE) ==
		    sc55112_SWCTRL_SW1_MODE_SYNC_RECT_EN) {
			config->mode = SYNC_RECT;
		} else {
			config->mode = PULSE_SKIP;
		}

		config->voltage.sw1 = BITFEXT(reg_val, sc55112_SWCTRL_SW1);
		config->voltage_lvs.sw1 =
		    BITFEXT(reg_val, sc55112_SWCTRL_SW1_LVS);
		config->voltage_stby.sw1 =
		    BITFEXT(reg_val, sc55112_SWCTRL_SW1_STBY);

		if (BITFEXT(reg_val, sc55112_SWCTRL_SW1_STBY_LVS_EN) ==
		    sc55112_SWCTRL_SW1_STBY_LVS_EN_ENABLE) {
			config->lp_mode = LOW_POWER_CTRL_BY_PIN;
		} else {
			config->lp_mode = LOW_POWER_DISABLED;
		}
		break;

	case SW2:
		if (BITFEXT(reg_val, sc55112_SWCTRL_SW2_MODE) ==
		    sc55112_SWCTRL_SW2_MODE_SYNC_RECT_EN) {
			config->mode = SYNC_RECT;
		} else {
			config->mode = PULSE_SKIP;
		}

		config->voltage.sw2 = BITFEXT(reg_val, sc55112_SWCTRL_SW2);
		config->voltage_lvs.sw2 =
		    BITFEXT(reg_val, sc55112_SWCTRL_SW2_LVS);

		if (BITFEXT(reg_val, sc55112_SWCTRL_SW2_DFVS_EN) ==
		    sc55112_SWCTRL_SW2_DFVS_EN_ENABLE) {
			config->lp_mode = LOW_POWER_CTRL_BY_PIN;
		} else {
			config->lp_mode = LOW_POWER_DISABLED;
		}
		break;

	case SW3:
		config->voltage.sw3 = BITFEXT(reg_val, sc55112_SWCTRL_SW3);

		if (BITFEXT(reg_val, sc55112_SWCTRL_SW3_STBY_EN) ==
		    sc55112_SWCTRL_SW3_STBY_EN_ENABLE) {
			config->lp_mode = LOW_POWER_CTRL_BY_PIN;
		} else {
			config->lp_mode = LOW_POWER_DISABLED;
		}
		break;

	case V1:
		config->voltage.v1 = BITFEXT(reg_val, sc55112_VREG_V1);

		if (BITFEXT(reg_val, sc55112_VREG_V1_LP) ==
		    sc55112_VREG_V1_LP_ENABLE) {
			config->lp_mode = LOW_POWER;
		} else if (BITFEXT(reg_val, sc55112_VREG_V1_STBY) ==
			   sc55112_VREG_V1_STBY_LOW_POWER) {
			config->lp_mode = LOW_POWER_CTRL_BY_PIN;
		} else {
			config->lp_mode = LOW_POWER_DISABLED;
		}
		break;

	case V2:
		config->voltage.v2 = V2_2_775V;

		if (BITFEXT(reg_val, sc55112_VREG_V2_LP) ==
		    sc55112_VREG_V2_LP_ENABLE) {
			config->lp_mode = LOW_POWER;
		} else if (BITFEXT(reg_val, sc55112_VREG_V2_STBY) ==
			   sc55112_VREG_V2_STBY_LOW_POWER) {
			config->lp_mode = LOW_POWER_CTRL_BY_PIN;
		} else {
			config->lp_mode = LOW_POWER_DISABLED;
		}
		break;

	case V3:
		config->voltage.v3 = BITFEXT(reg_val, sc55112_VREG_V3);

		if (BITFEXT(reg_val, sc55112_VREG_V3_LP) ==
		    sc55112_VREG_V3_LP_ENABLE) {
			config->lp_mode = LOW_POWER;
		} else if (BITFEXT(reg_val, sc55112_VREG_V3_STBY) ==
			   sc55112_VREG_V3_STBY_LOW_POWER) {
			config->lp_mode = LOW_POWER_CTRL_BY_PIN;
		} else {
			config->lp_mode = LOW_POWER_DISABLED;
		}
		break;

	case V4:
		config->voltage.v4 = V4_2_775V;

		if (BITFEXT(reg_val, sc55112_VREG_V3_LP) ==
		    sc55112_VREG_V3_LP_ENABLE) {
			config->lp_mode = LOW_POWER;
		} else if (BITFEXT(reg_val, sc55112_VREG_V3_STBY) ==
			   sc55112_VREG_V3_STBY_LOW_POWER) {
			config->lp_mode = LOW_POWER_CTRL_BY_PIN;
		} else {
			config->lp_mode = LOW_POWER_DISABLED;
		}
		break;

	case VSIM:
		config->voltage.vsim = BITFEXT(reg_val, sc55112_VREG_VSIM);
		break;

	case V_VIB:
		config->voltage.v_vib = BITFEXT(reg_val, sc55112_VREG_V_VIB);
		break;

	case VMMC:
		config->voltage.vmmc = BITFEXT(reg_val, sc55112_VREG_VMMC);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function implements IOCTL controls on a PMIC Power Control device
 * Driver.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int pmic_power_ioctl(struct inode *inode, struct file *file,
			    unsigned int cmd, unsigned long arg)
{
	t_regulator_cfg_param *cfg_param;
	PMIC_STATUS ret;

	if (_IOC_TYPE(cmd) != 'p')
		return -ENOTTY;

	switch (cmd) {
	case PMIC_REGULATOR_ON:
		CHECK_ERROR(pmic_power_regulator_on((t_pmic_regulator) arg));
		break;

	case PMIC_REGULATOR_OFF:
		CHECK_ERROR(pmic_power_regulator_off((t_pmic_regulator) arg));
		break;

	case PMIC_REGULATOR_SET_CONFIG:
		if ((cfg_param =
		     kmalloc(sizeof(t_regulator_cfg_param), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(cfg_param, (t_regulator_cfg_param *) arg,
				   sizeof(t_regulator_cfg_param))) {
			kfree(cfg_param);
			return -EFAULT;
		}

		ret =
		    pmic_power_regulator_set_config(cfg_param->regulator,
						    &cfg_param->cfg);
		if (ret != PMIC_SUCCESS) {
			kfree(cfg_param);
			return ret;
		}
		kfree(cfg_param);
		break;

	case PMIC_REGULATOR_GET_CONFIG:
		if ((cfg_param =
		     kmalloc(sizeof(t_regulator_cfg_param), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(cfg_param, (t_regulator_cfg_param *) arg,
				   sizeof(t_regulator_cfg_param))) {
			kfree(cfg_param);
			return -EFAULT;
		}

		ret =
		    pmic_power_regulator_get_config(cfg_param->regulator,
						    &cfg_param->cfg);
		if (ret != PMIC_SUCCESS) {
			kfree(cfg_param);
			return ret;
		}

		if (copy_to_user((t_regulator_cfg_param *) arg, cfg_param,
				 sizeof(t_regulator_cfg_param))) {
			kfree(cfg_param);
			return -EFAULT;
		}
		kfree(cfg_param);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/*!
 * This function implements the open method on a PMIC power device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_power_open(struct inode *inode, struct file *file)
{
	return 0;
}

/*!
 * This function implements the release method on a PMIC power device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_power_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct platform_device pmic_power_ldm = {
	.name = "PMIC_POWER",
	.id = 1,
};

static struct file_operations pmic_power_fops = {
	.owner = THIS_MODULE,
	.ioctl = pmic_power_ioctl,
	.open = pmic_power_open,
	.release = pmic_power_release,
};

/*
 * Initialization and Exit
 */
static int __init pmic_power_init(void)
{
	pmic_power_major = register_chrdev(0, "pmic_power", &pmic_power_fops);

	if (pmic_power_major < 0) {
		TRACEMSG(_K_D("Unable to get a major for pmic_power"));
		return pmic_power_major;
	}

	devfs_mk_cdev(MKDEV(pmic_power_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "pmic_power");

	driver_register(&pmic_power_driver_ldm);

	platform_device_register(&pmic_power_ldm);

	printk(KERN_INFO "PMIC Power Control loaded\n");
	return 0;
}

static void __exit pmic_power_exit(void)
{
	devfs_remove("pmic_power");
	unregister_chrdev(pmic_power_major, "pmic_power");
	driver_unregister(&pmic_power_driver_ldm);
	platform_device_unregister(&pmic_power_ldm);
	printk(KERN_INFO "PMIC Power Control unloaded\n");
}

/*
 * Module entry points
 */

subsys_initcall(pmic_power_init);
module_exit(pmic_power_exit);

MODULE_DESCRIPTION("PMIC Power Control device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
