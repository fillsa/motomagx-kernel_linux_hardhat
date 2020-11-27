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
 * @file pmic_light.c
 * @brief This is the main file of sc55112 Light driver.
 *
 * @ingroup PMIC_sc55112_LIGHT
 */

/*
 * Includes
 */
#include <asm/ioctl.h>
#include <linux/device.h>

#include "../core/sc55112_config.h"
#include "sc55112_light_regs.h"
#include "asm/arch/pmic_status.h"
#include "asm/arch/pmic_external.h"
#include "asm/arch/pmic_light.h"

static int pmic_light_major;

#define sc55112_LED_MAX_BACKLIGHT_CURRENT_LEVEL     7
#define sc55112_LED_MAX_BACKLIGHT_DUTY_CYCLE        0xF
#define sc55112_LED_MAX_BACKLIGHT_PERIOD            3

/*!
 * Number of users waiting in suspendq
 */
static int swait = 0;

/*!
 * To indicate whether any of the adc devices are suspending
 */
static int suspend_flag = 0;

/*!
 * The suspendq is used by blocking application calls
 */
static wait_queue_head_t suspendq;

/*
 * PMIC Light API
 */

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(pmic_bklit_master_enable);
EXPORT_SYMBOL(pmic_bklit_master_disable);
EXPORT_SYMBOL(pmic_bklit_set_current);
EXPORT_SYMBOL(pmic_bklit_get_current);
EXPORT_SYMBOL(pmic_bklit_set_dutycycle);
EXPORT_SYMBOL(pmic_bklit_get_dutycycle);
EXPORT_SYMBOL(pmic_bklit_set_cycle_time);
EXPORT_SYMBOL(pmic_bklit_get_cycle_time);
EXPORT_SYMBOL(pmic_bklit_set_mode);
EXPORT_SYMBOL(pmic_bklit_rampup);
EXPORT_SYMBOL(pmic_bklit_rampdown);
EXPORT_SYMBOL(pmic_bklit_enable_edge_slow);
EXPORT_SYMBOL(pmic_bklit_disable_edge_slow);
EXPORT_SYMBOL(pmic_bklit_set_strobemode);
EXPORT_SYMBOL(pmic_tcled_enable);
EXPORT_SYMBOL(pmic_tcled_disable);
EXPORT_SYMBOL(pmic_tcled_get_mode);
EXPORT_SYMBOL(pmic_tcled_ind_set_current);
EXPORT_SYMBOL(pmic_tcled_ind_get_current);
EXPORT_SYMBOL(pmic_tcled_ind_set_blink_pattern);
EXPORT_SYMBOL(pmic_tcled_ind_get_blink_pattern);
EXPORT_SYMBOL(pmic_tcled_fun_set_current);
EXPORT_SYMBOL(pmic_tcled_fun_get_current);
EXPORT_SYMBOL(pmic_tcled_fun_set_cycletime);
EXPORT_SYMBOL(pmic_tcled_fun_get_cycletime);
EXPORT_SYMBOL(pmic_tcled_fun_set_dutycycle);
EXPORT_SYMBOL(pmic_tcled_fun_get_dutycycle);
EXPORT_SYMBOL(pmic_tcled_fun_blendedramps);
EXPORT_SYMBOL(pmic_tcled_fun_sawramps);
EXPORT_SYMBOL(pmic_tcled_fun_blendedbowtie);
EXPORT_SYMBOL(pmic_tcled_fun_chasinglightspattern);
EXPORT_SYMBOL(pmic_tcled_fun_strobe);
EXPORT_SYMBOL(pmic_tcled_fun_rampup);
EXPORT_SYMBOL(pmic_tcled_fun_rampdown);
EXPORT_SYMBOL(pmic_tcled_fun_triode_on);
EXPORT_SYMBOL(pmic_tcled_fun_triode_off);
EXPORT_SYMBOL(pmic_tcled_enable_edge_slow);
EXPORT_SYMBOL(pmic_tcled_disable_edge_slow);
EXPORT_SYMBOL(pmic_tcled_enable_half_current);
EXPORT_SYMBOL(pmic_tcled_disable_half_current);
EXPORT_SYMBOL(pmic_tcled_enable_audio_modulation);
EXPORT_SYMBOL(pmic_tcled_disable_audio_modulation);

/*!
 * This function enables backlight.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_master_enable(void)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	val =
	    BITFVAL(sc55112_LED_BKLT1_LEDBLEN,
		    sc55112_LED_BKLT1_LEDBLEN_ENABLE);
	mask = BITFMASK(sc55112_LED_BKLT1_LEDBLEN);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function disables backlight.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_master_disable(void)
{
	unsigned int val, mask;

	val =
	    BITFVAL(sc55112_LED_BKLT1_LEDBLEN,
		    sc55112_LED_BKLT1_LEDBLEN_DISABLE);
	mask = BITFMASK(sc55112_LED_BKLT1_LEDBLEN);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function sets backlight current level.
 * In sc55112, LED1 and LED2 are designed for a nominal full scale current
 * of 84mA in 12mA steps.  The channels are not individually adjustable, hence
 * the channel parameter is ignored.
 *
 * @param        channel   Backlight channel (Ignored in sc55112 because the
 *                         channels are not individually adjustable)
 * @param        level     Backlight current level, as the following table.
 *                         @verbatim
                               level     current
                               ------    -----------
                                 0         0 mA
                                 1         12 mA
                                 2         24 mA
                                 3         36 mA
                                 4         48 mA
                                 5         60 mA
                                 6         72 mA
                                 7         84 mA
                            @endverbatim
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_current(t_bklit_channel channel, unsigned char level)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	/* check current level parameter */
	if (level > sc55112_LED_MAX_BACKLIGHT_CURRENT_LEVEL) {
		return PMIC_PARAMETER_ERROR;
	}

	val = BITFVAL(sc55112_LED_BKLT1_LEDI, level);
	mask = BITFMASK(sc55112_LED_BKLT1_LEDI);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function retrives backlight current level.
 * In sc55112, LED1 and LED2 are designed for a nominal full scale current
 * of 84mA in 12mA steps.  The channels are not individually adjustable, hence
 * the channel parameter is ignored.
 *
 * @param        channel   Backlight channel (Ignored in sc55112 because the
 *                         channels are not individually adjustable)
 * @param        level     Pointer to store backlight current level result.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_get_current(t_bklit_channel channel,
				   unsigned char *level)
{
	unsigned int val;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_BACKLIGHT_1, &val));

	*level = BITFEXT(val, sc55112_LED_BKLT1_LEDI);

	return PMIC_SUCCESS;
}

/*!
 * This function sets a backlight channel duty cycle.
 * LED perceived brightness for each zone may be individually set by setting
 * duty cycle. The default setting is for 0% duty cycle; this keeps all zone
 * drivers turned off even after the master enable command. Each LED current
 * sink can be turned on and adjusted for brightness with an independent 4 bit
 * word for a duty cycle ranging from 0% to 100% in approximately 6.7% steps.
 *
 * @param        channel   Backlight channel.
 * @param        dc        Backlight duty cycle, as the following table.
 *                         @verbatim
                                dc        Duty Cycle (% On-time over Cycle Time)
                               ------    ---------------------------------------
                                  0        0%
                                  1        6.7%
                                  2        13.3%
                                  3        20%
                                  4        26.7%
                                  5        33.3%
                                  6        40%
                                  7        46.7%
                                  8        53.3%
                                  9        60%
                                 10        66.7%
                                 11        73.3%
                                 12        80%
                                 13        86.7%
                                 14        93.3%
                                 15        100%
                             @endverbatim
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_dutycycle(t_bklit_channel channel, unsigned char dc)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	/* check current level parameter */
	if (dc > sc55112_LED_MAX_BACKLIGHT_DUTY_CYCLE) {
		return PMIC_PARAMETER_ERROR;
	}

	switch (channel) {
	case BACKLIGHT_LED1:
		val = BITFVAL(sc55112_LED_BKLT1_LED1DC, dc);
		mask = BITFMASK(sc55112_LED_BKLT1_LED1DC);
		break;

	case BACKLIGHT_LED2:
		val = BITFVAL(sc55112_LED_BKLT1_LED2DC, dc);
		mask = BITFMASK(sc55112_LED_BKLT1_LED2DC);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_1, val, mask));

	return PMIC_SUCCESS;

}

/*!
 * This function retrives a backlight channel duty cycle.
 *
 * @param        channel   Backlight channel.
 * @param        dc        Pointer to backlight duty cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_get_dutycycle(t_bklit_channel channel, unsigned char *dc)
{
	unsigned int val;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_BACKLIGHT_1, &val));

	switch (channel) {
	case BACKLIGHT_LED1:
		*dc = BITFEXT(val, sc55112_LED_BKLT1_LED1DC);
		break;

	case BACKLIGHT_LED2:
		*dc = BITFEXT(val, sc55112_LED_BKLT1_LED2DC);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function sets a backlight channel cycle time.
 * Cycle Time is defined as the period of a complete cycle of
 * Time_on + Time_off. The default Cycle Time is set to 0.01 seconds such that
 * the 100 Hz on-off cycling is averaged out by the eye to eliminate
 * flickering. Additionally, the Cycle Time can be programmed to intentionally
 * extend the period of on-off cycles for a visual pulsating or blinking effect.
 *
 * @param        period    Backlight cycle time, as the following table.
 *                         @verbatim
                                period      Cycle Time
                               --------    ------------
                                  0          0.01 seconds
                                  1          0.1 seconds
                                  2          0.5 seconds
                                  3          2 seconds
                             @endverbatim
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_cycle_time(unsigned char period)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	/* check cycle time parameter */
	if (period > sc55112_LED_MAX_BACKLIGHT_PERIOD) {
		return PMIC_PARAMETER_ERROR;
	}

	val = BITFVAL(sc55112_LED_BKLT1_BKLT1CYCLETIME, period);
	mask = BITFMASK(sc55112_LED_BKLT1_BKLT1CYCLETIME);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function retrives a backlight channel cycle time setting.
 *
 * @param        period    Pointer to save backlight cycle time setting result.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_get_cycle_time(unsigned char *period)
{
	unsigned int val;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_BACKLIGHT_1, &val));

	*period = BITFEXT(val, sc55112_LED_BKLT1_BKLT1CYCLETIME);

	return PMIC_SUCCESS;
}

/*!
 * This function sets backlight operation mode. There are two modes of
 * operations: current control and triode mode.
 * The Duty Cycle/Cycle Time control is retained in Triode Mode. Audio
 * coupling is not available in Triode Mode.
 *
 * @param        channel   Backlight channel.
 * @param        mode      Backlight operation mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_mode(t_bklit_channel channel, t_bklit_mode mode)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case BACKLIGHT_LED1:
		if (mode == BACKLIGHT_CURRENT_CTRL_MODE) {
			val = BITFVAL(sc55112_LED_BKLT2_LED1TRIODE,
				      sc55112_LED_BKLT2_LED1TRIODE_DISABLE);
		} else {
			val = BITFVAL(sc55112_LED_BKLT2_LED1TRIODE,
				      sc55112_LED_BKLT2_LED1TRIODE_ENABLE);
		}

		mask = BITFMASK(sc55112_LED_BKLT2_LED1TRIODE);
		break;

	case BACKLIGHT_LED2:
		if (mode == BACKLIGHT_CURRENT_CTRL_MODE) {
			val = BITFVAL(sc55112_LED_BKLT2_LED2TRIODE,
				      sc55112_LED_BKLT2_LED2TRIODE_DISABLE);
		} else {
			val = BITFVAL(sc55112_LED_BKLT2_LED2TRIODE,
				      sc55112_LED_BKLT2_LED2TRIODE_ENABLE);
		}
		mask = BITFMASK(sc55112_LED_BKLT2_LED2TRIODE);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function starts backlight brightness ramp up function; ramp time is
 * fixed at 0.5 seconds.
 *
 * @param        channel   Backlight channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_rampup(t_bklit_channel channel)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case BACKLIGHT_LED1:
		val =
		    BITFVAL(sc55112_LED_BKLT1_LED1RAMPUP,
			    sc55112_LED_BKLT1_LEDRAMPUP_ENABLE);
		mask = BITFMASK(sc55112_LED_BKLT1_LED1RAMPUP);
		break;

	case BACKLIGHT_LED2:
		val =
		    BITFVAL(sc55112_LED_BKLT1_LED2RAMPUP,
			    sc55112_LED_BKLT1_LEDRAMPUP_ENABLE);
		mask = BITFMASK(sc55112_LED_BKLT1_LED2RAMPUP);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function starts backlight brightness ramp down function; ramp time is
 * fixed at 0.5 seconds.
 *
 * @param        channel   Backlight channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_rampdown(t_bklit_channel channel)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case BACKLIGHT_LED1:
		val =
		    BITFVAL(sc55112_LED_BKLT1_LED1RAMPDOWN,
			    sc55112_LED_BKLT1_LEDRAMPDOWN_ENABLE);
		mask = BITFMASK(sc55112_LED_BKLT1_LED1RAMPDOWN);
		break;

	case BACKLIGHT_LED2:
		val =
		    BITFVAL(sc55112_LED_BKLT1_LED2RAMPDOWN,
			    sc55112_LED_BKLT1_LEDRAMPDOWN_ENABLE);
		mask = BITFMASK(sc55112_LED_BKLT1_LED2RAMPDOWN);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function enables backlight analog edge slowing mode. Analog Edge
 * Slowing slows down the transient edges to reduce the chance of coupling LED
 * modulation activity into other circuits. Rise and fall times will be targeted
 * for approximately 50usec.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_enable_edge_slow(void)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	val =
	    BITFVAL(sc55112_LED_BKLT2_SLEWLIMEN,
		    sc55112_LED_BKLT2_SLEWLIMEN_ENABLE);
	mask = BITFMASK(sc55112_LED_BKLT2_SLEWLIMEN);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function disables backlight analog edge slowing mode. The backlight
 * drivers will default to an “Instant On” mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_disable_edge_slow(void)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	val =
	    BITFVAL(sc55112_LED_BKLT2_SLEWLIMEN,
		    sc55112_LED_BKLT2_SLEWLIMEN_DISABLE);
	mask = BITFMASK(sc55112_LED_BKLT2_SLEWLIMEN);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function sets backlight Strobe Light Pulsing mode.
 *
 * @param        channel   Backlight channel.
 * @param        mode      Strobe Light Pulsing mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_strobemode(t_bklit_channel channel,
				      t_bklit_strobe_mode mode)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	val = 0;

	switch (channel) {
	case BACKLIGHT_LED1:
		if (mode == BACKLIGHT_STROBE_FAST) {
			val = BITFVAL(sc55112_LED_BKLT2_LED1STROBEFAST,
				      sc55112_LED_BKLT2_LEDSTROBEFAST_ENABLE);
		} else if (mode == BACKLIGHT_STROBE_SLOW) {
			val = BITFVAL(sc55112_LED_BKLT2_LED1STROBESLOW,
				      sc55112_LED_BKLT2_LEDSTROBESLOW_ENABLE);
		}

		mask = BITFMASK(sc55112_LED_BKLT2_LED1STROBEFAST) |
		    BITFMASK(sc55112_LED_BKLT2_LED1STROBESLOW);
		break;

	case BACKLIGHT_LED2:
		if (mode == BACKLIGHT_STROBE_FAST) {
			val = BITFVAL(sc55112_LED_BKLT2_LED2STROBEFAST,
				      sc55112_LED_BKLT2_LEDSTROBEFAST_ENABLE);
		} else if (mode == BACKLIGHT_STROBE_SLOW) {
			val = BITFVAL(sc55112_LED_BKLT2_LED2STROBESLOW,
				      sc55112_LED_BKLT2_LEDSTROBESLOW_ENABLE);
		}

		mask = BITFMASK(sc55112_LED_BKLT2_LED2STROBEFAST) |
		    BITFMASK(sc55112_LED_BKLT2_LED2STROBESLOW);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function enables tri-color LED.
 *
 * @param        mode      Tri-color LED operation mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_enable(t_tcled_mode mode)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (mode) {
	case TCLED_IND_MODE:
		val =
		    BITFVAL(sc55112_LED_BKLT2_FLRGB,
			    sc55112_LED_BKLT2_FLRGB_RGINDICATOR);
		break;

	case TCLED_FUN_MODE:
		val =
		    BITFVAL(sc55112_LED_BKLT2_FLRGB,
			    sc55112_LED_BKLT2_FLRGB_FUNLIGHT);
		break;
	}
	mask = BITFMASK(sc55112_LED_BKLT2_FLRGB);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_2, val, mask));

	val = BITFVAL(sc55112_LED_TC2_LED_TC_EN, sc55112_LED_TC2_LED_TC_ENABLE);
	mask = BITFMASK(sc55112_LED_TC2_LED_TC_EN);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	switch (mode) {
	case TCLED_IND_MODE:
		val =
		    BITFVAL(sc55112_LED_TC1_LEDR_EN,
			    sc55112_LED_TC1_LED_ENABLE) |
		    BITFVAL(sc55112_LED_TC1_LEDG_EN,
			    sc55112_LED_TC1_LED_ENABLE);
		mask =
		    BITFMASK(sc55112_LED_TC1_LEDR_EN) |
		    BITFMASK(sc55112_LED_TC1_LEDG_EN);
		break;

	case TCLED_FUN_MODE:
		val =
		    BITFVAL(sc55112_LED_TC1_LEDR_EN,
			    sc55112_LED_TC1_LED_ENABLE) |
		    BITFVAL(sc55112_LED_TC1_LEDG_EN,
			    sc55112_LED_TC1_LED_ENABLE) |
		    BITFVAL(sc55112_LED_TC1_LEDB_EN,
			    sc55112_LED_TC1_LED_ENABLE);
		mask =
		    BITFMASK(sc55112_LED_TC1_LEDR_EN) |
		    BITFMASK(sc55112_LED_TC1_LEDG_EN) |
		    BITFMASK(sc55112_LED_TC1_LEDB_EN);
		break;
	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function disables tri-color LED.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_disable(void)
{
	unsigned int val, mask;

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, 0, 0xFFFFFF));

	val = BITFVAL(sc55112_LED_TC1_LEDR_EN, sc55112_LED_TC1_LED_DISABLE) |
	    BITFVAL(sc55112_LED_TC1_LEDG_EN, sc55112_LED_TC1_LED_DISABLE) |
	    BITFVAL(sc55112_LED_TC1_LEDB_EN, sc55112_LED_TC1_LED_DISABLE);
	mask = BITFMASK(sc55112_LED_TC1_LEDR_EN) |
	    BITFMASK(sc55112_LED_TC1_LEDG_EN) |
	    BITFMASK(sc55112_LED_TC1_LEDB_EN);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function retrives tri-color LED operation mode.
 *
 * @param        mode      Pointer to Tri-color LED operation mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_get_mode(t_tcled_mode * mode)
{
	unsigned int val;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_BACKLIGHT_2, &val));

	if (BITFEXT(val, sc55112_LED_BKLT2_FLRGB) ==
	    sc55112_LED_BKLT2_FLRGB_RGINDICATOR) {
		*mode = TCLED_IND_MODE;
	} else {
		*mode = TCLED_FUN_MODE;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function sets a tri-color LED channel current level in indicator mode.
 *
 * @param        channel      Tri-color LED channel.
 * @param        level        Current level.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_ind_set_current(t_ind_channel channel,
				       t_tcled_cur_level level)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case TCLED_IND_RED:
		val = BITFVAL(sc55112_LED_TC1_LEDR_I, level);
		mask = BITFMASK(sc55112_LED_TC1_LEDR_I);
		break;

	case TCLED_IND_GREEN:
		val = BITFVAL(sc55112_LED_TC1_LEDG_I, level);
		mask = BITFMASK(sc55112_LED_TC1_LEDG_I);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function retrives a tri-color LED channel current level in indicator mode.
 *
 * @param        channel      Tri-color LED channel.
 * @param        level        Pointer to current level.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_ind_get_current(t_ind_channel channel,
				       t_tcled_cur_level * level)
{
	unsigned int val;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_TC_CONTROL1, &val));

	switch (channel) {
	case TCLED_IND_RED:
		*level = BITFEXT(val, sc55112_LED_TC1_LEDR_I);
		break;

	case TCLED_IND_GREEN:
		*level = BITFEXT(val, sc55112_LED_TC1_LEDG_I);
		break;

	default:
		return PMIC_PARAMETER_ERROR;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function sets a tri-color LED channel blinking pattern in indication
 * mode.
 *
 * @param        channel      Tri-color LED channel.
 * @param        pattern      Blinking pattern.
 * @param        skip         If trure, skip a cycle after each cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_ind_set_blink_pattern(t_ind_channel channel,
					     t_tcled_ind_blink_pattern pattern,
					     bool skip)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case TCLED_IND_RED:
		val = BITFVAL(sc55112_LED_TC1_LEDR_CTRL, pattern);
		mask = BITFMASK(sc55112_LED_TC1_LEDR_CTRL);
		break;

	case TCLED_IND_GREEN:
		val = BITFVAL(sc55112_LED_TC1_LEDG_CTRL, pattern);
		mask = BITFMASK(sc55112_LED_TC1_LEDG_CTRL);
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL1, val, mask));

	if (skip != false) {
		val =
		    BITFVAL(sc55112_LED_BKLT2_SKIP,
			    sc55112_LED_BKLT2_SKIP_SKIP);
	} else {
		val =
		    BITFVAL(sc55112_LED_BKLT2_SKIP,
			    sc55112_LED_BKLT2_SKIP_NOSKIP);
	}

	mask = BITFMASK(sc55112_LED_BKLT2_SKIP);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function retrives a tri-color LED channel blinking pattern in
 * indication mode.
 *
 * @param        channel      Tri-color LED channel.
 * @param        pattern      Pointer to Blinking pattern.
 * @param        skip         Pointer to a boolean varible indicating if skip
 *                            a cycle after each cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_ind_get_blink_pattern(t_ind_channel channel,
					     t_tcled_ind_blink_pattern *
					     pattern, bool * skip)
{
	unsigned int val;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_TC_CONTROL1, &val));

	switch (channel) {
	case TCLED_IND_RED:
		*pattern = BITFEXT(val, sc55112_LED_TC1_LEDR_CTRL);
		break;

	case TCLED_IND_GREEN:
		*pattern = BITFEXT(val, sc55112_LED_TC1_LEDG_CTRL);
		break;
	}

	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_BACKLIGHT_2, &val));

	if (BITFEXT(val, sc55112_LED_BKLT2_SKIP) == sc55112_LED_BKLT2_SKIP_SKIP) {
		*skip = true;
	} else {
		*skip = false;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function sets a tri-color LED channel current level in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        level        Current level.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_set_current(t_funlight_bank bank,
				       t_funlight_channel channel,
				       t_tcled_cur_level level)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case TCLED_FUN_CHANNEL1:
		val = BITFVAL(sc55112_LED_TC1_LEDR_I, level);
		mask = BITFMASK(sc55112_LED_TC1_LEDR_I);
		break;

	case TCLED_FUN_CHANNEL2:
		val = BITFVAL(sc55112_LED_TC1_LEDG_I, level);
		mask = BITFMASK(sc55112_LED_TC1_LEDG_I);
		break;

	case TCLED_FUN_CHANNEL3:
		val = BITFVAL(sc55112_LED_TC1_LEDB_I, level);
		mask = BITFMASK(sc55112_LED_TC1_LEDB_I);
		break;
	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function retrives a tri-color LED channel current level in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        level        Pointer to current level.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_get_current(t_funlight_bank bank,
				       t_funlight_channel channel,
				       t_tcled_cur_level * level)
{
	unsigned int val;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_TC_CONTROL1, &val));

	switch (channel) {
	case TCLED_FUN_CHANNEL1:
		*level = BITFEXT(val, sc55112_LED_TC1_LEDR_I);
		break;

	case TCLED_FUN_CHANNEL2:
		*level = BITFEXT(val, sc55112_LED_TC1_LEDG_I);
		break;

	case TCLED_FUN_CHANNEL3:
		*level = BITFEXT(val, sc55112_LED_TC1_LEDB_I);
		break;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function sets tri-color LED cycle time in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        ct           Cycle time.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_set_cycletime(t_funlight_bank bank,
					 t_tcled_fun_cycle_time ct)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	val = BITFVAL(sc55112_LED_TC2_CYCLE_TIME, ct);
	mask = BITFMASK(sc55112_LED_TC2_CYCLE_TIME);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function retrives tri-color LED cycle time in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        ct           Pointer to cycle time.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_get_cycletime(t_funlight_bank bank,
					 t_tcled_fun_cycle_time * ct)
{
	unsigned int val;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_TC_CONTROL2, &val));

	*ct = BITFEXT(val, sc55112_LED_TC2_CYCLE_TIME);

	return PMIC_SUCCESS;
}

/*!
 * This function sets a tri-color LED channel duty cycle in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        dc           Duty cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_set_dutycycle(t_funlight_bank bank,
					 t_funlight_channel channel,
					 unsigned char dc)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case TCLED_FUN_CHANNEL1:
		val = BITFVAL(sc55112_LED_TC1_LEDR_CTRL, dc);
		mask = BITFMASK(sc55112_LED_TC1_LEDR_CTRL);
		break;

	case TCLED_FUN_CHANNEL2:
		val = BITFVAL(sc55112_LED_TC1_LEDG_CTRL, dc);
		mask = BITFMASK(sc55112_LED_TC1_LEDG_CTRL);
		break;

	case TCLED_FUN_CHANNEL3:
		val = BITFVAL(sc55112_LED_TC1_LEDB_CTRL, dc);
		mask = BITFMASK(sc55112_LED_TC1_LEDB_CTRL);
		break;
	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL1, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function retrives a tri-color LED channel duty cycle in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        dc           Pointer to duty cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_get_dutycycle(t_funlight_bank bank,
					 t_funlight_channel channel,
					 unsigned char *dc)
{
	unsigned int val;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	CHECK_ERROR(pmic_read_reg(PRIO_LIGHT, REG_TC_CONTROL1, &val));

	switch (channel) {
	case TCLED_FUN_CHANNEL1:
		*dc = BITFEXT(val, sc55112_LED_TC1_LEDR_CTRL);
		break;

	case TCLED_FUN_CHANNEL2:
		*dc = BITFEXT(val, sc55112_LED_TC1_LEDG_CTRL);
		break;

	case TCLED_FUN_CHANNEL3:
		*dc = BITFEXT(val, sc55112_LED_TC1_LEDB_CTRL);
		break;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function initiates Blended Ramp fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_blendedramps(t_funlight_bank bank,
					t_tcled_fun_speed speed)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (speed) {
	case TC_OFF:
		val = BITFVAL(sc55112_LED_TC2_BLENDED_RAMPS_SLOW,
			      sc55112_LED_TC2_BLENDED_RAMPS_SLOW_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_BLENDED_RAMPS_FAST,
			    sc55112_LED_TC2_BLENDED_RAMPS_FAST_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_SAW_RAMPS_SLOW,
			    sc55112_LED_TC2_SAW_RAMPS_SLOW_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_SAW_RAMPS_FAST,
			    sc55112_LED_TC2_SAW_RAMPS_FAST_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_BLENDED_BOWTIE_SLOW,
			    sc55112_LED_TC2_BLENDED_BOWTIE_SLOW_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_BLENDED_BOWTIE_FAST,
			    sc55112_LED_TC2_BLENDED_BOWTIE_FAST_DISABLE);

		mask = BITFMASK(sc55112_LED_TC2_BLENDED_RAMPS_SLOW) |
		    BITFMASK(sc55112_LED_TC2_BLENDED_RAMPS_FAST) |
		    BITFMASK(sc55112_LED_TC2_SAW_RAMPS_SLOW) |
		    BITFMASK(sc55112_LED_TC2_SAW_RAMPS_FAST) |
		    BITFMASK(sc55112_LED_TC2_BLENDED_BOWTIE_SLOW) |
		    BITFMASK(sc55112_LED_TC2_BLENDED_BOWTIE_FAST);

		break;

	case TC_SLOW:
		val = BITFVAL(sc55112_LED_TC2_BLENDED_RAMPS_SLOW,
			      sc55112_LED_TC2_BLENDED_RAMPS_SLOW_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_BLENDED_RAMPS_SLOW);
		break;

	case TC_FAST:
		val = BITFVAL(sc55112_LED_TC2_BLENDED_RAMPS_FAST,
			      sc55112_LED_TC2_BLENDED_RAMPS_FAST_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_BLENDED_RAMPS_FAST);
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function initiates Saw Ramp fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_sawramps(t_funlight_bank bank,
				    t_tcled_fun_speed speed)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (speed) {
	case TC_OFF:
		val = BITFVAL(sc55112_LED_TC2_BLENDED_RAMPS_SLOW,
			      sc55112_LED_TC2_BLENDED_RAMPS_SLOW_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_BLENDED_RAMPS_FAST,
			    sc55112_LED_TC2_BLENDED_RAMPS_FAST_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_SAW_RAMPS_SLOW,
			    sc55112_LED_TC2_SAW_RAMPS_SLOW_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_SAW_RAMPS_FAST,
			    sc55112_LED_TC2_SAW_RAMPS_FAST_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_BLENDED_BOWTIE_SLOW,
			    sc55112_LED_TC2_BLENDED_BOWTIE_SLOW_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_BLENDED_BOWTIE_FAST,
			    sc55112_LED_TC2_BLENDED_BOWTIE_FAST_DISABLE);

		mask = BITFMASK(sc55112_LED_TC2_BLENDED_RAMPS_SLOW) |
		    BITFMASK(sc55112_LED_TC2_BLENDED_RAMPS_FAST) |
		    BITFMASK(sc55112_LED_TC2_SAW_RAMPS_SLOW) |
		    BITFMASK(sc55112_LED_TC2_SAW_RAMPS_FAST) |
		    BITFMASK(sc55112_LED_TC2_BLENDED_BOWTIE_SLOW) |
		    BITFMASK(sc55112_LED_TC2_BLENDED_BOWTIE_FAST);

		break;

	case TC_SLOW:
		val = BITFVAL(sc55112_LED_TC2_SAW_RAMPS_SLOW,
			      sc55112_LED_TC2_SAW_RAMPS_SLOW_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_SAW_RAMPS_SLOW);
		break;

	case TC_FAST:
		val = BITFVAL(sc55112_LED_TC2_SAW_RAMPS_FAST,
			      sc55112_LED_TC2_SAW_RAMPS_FAST_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_SAW_RAMPS_FAST);
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function initiates Blended Bowtie fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_blendedbowtie(t_funlight_bank bank,
					 t_tcled_fun_speed speed)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (speed) {
	case TC_OFF:
		val = BITFVAL(sc55112_LED_TC2_BLENDED_RAMPS_SLOW,
			      sc55112_LED_TC2_BLENDED_RAMPS_SLOW_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_BLENDED_RAMPS_FAST,
			    sc55112_LED_TC2_BLENDED_RAMPS_FAST_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_SAW_RAMPS_SLOW,
			    sc55112_LED_TC2_SAW_RAMPS_SLOW_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_SAW_RAMPS_FAST,
			    sc55112_LED_TC2_SAW_RAMPS_FAST_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_BLENDED_BOWTIE_SLOW,
			    sc55112_LED_TC2_BLENDED_BOWTIE_SLOW_DISABLE) |
		    BITFVAL(sc55112_LED_TC2_BLENDED_BOWTIE_FAST,
			    sc55112_LED_TC2_BLENDED_BOWTIE_FAST_DISABLE);

		mask = BITFMASK(sc55112_LED_TC2_BLENDED_RAMPS_SLOW) |
		    BITFMASK(sc55112_LED_TC2_BLENDED_RAMPS_FAST) |
		    BITFMASK(sc55112_LED_TC2_SAW_RAMPS_SLOW) |
		    BITFMASK(sc55112_LED_TC2_SAW_RAMPS_FAST) |
		    BITFMASK(sc55112_LED_TC2_BLENDED_BOWTIE_SLOW) |
		    BITFMASK(sc55112_LED_TC2_BLENDED_BOWTIE_FAST);

		break;

	case TC_SLOW:
		val = BITFVAL(sc55112_LED_TC2_BLENDED_BOWTIE_SLOW,
			      sc55112_LED_TC2_BLENDED_BOWTIE_SLOW_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_BLENDED_BOWTIE_SLOW);
		break;

	case TC_FAST:
		val = BITFVAL(sc55112_LED_TC2_BLENDED_BOWTIE_FAST,
			      sc55112_LED_TC2_BLENDED_BOWTIE_FAST_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_BLENDED_BOWTIE_FAST);
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function initiates Chasing Lights fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        pattern      Chasing light pattern mode.
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_chasinglightspattern(t_funlight_bank bank,
						t_chaselight_pattern pattern,
						t_tcled_fun_speed speed)
{
	return PMIC_NOT_SUPPORTED;
}

/*!
 * This function initiates Strobe Mode fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_strobe(t_funlight_bank bank,
				  t_funlight_channel channel,
				  t_tcled_fun_strobe_speed speed)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case TCLED_FUN_CHANNEL1:
		switch (speed) {
		case TC_STROBE_OFF:
			val =
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDR_SLOW,
				    sc55112_LED_TC2_STROBE_LED_SLOW_DISABLE) |
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDR_FAST,
				    sc55112_LED_TC2_STROBE_LED_FAST_DISABLE) |
			    BITFVAL(sc55112_LED_TC2_LEDR_RAMP_DOWN,
				    sc55112_LED_TC2_LED_RAMP_DOWN_DISABLE) |
			    BITFVAL(sc55112_LED_TC2_LEDR_RAMP_UP,
				    sc55112_LED_TC2_LED_RAMP_UP_DISABLE);
			mask =
			    BITFMASK(sc55112_LED_TC2_STROBE_LEDR_SLOW) |
			    BITFMASK(sc55112_LED_TC2_STROBE_LEDR_FAST) |
			    BITFMASK(sc55112_LED_TC2_LEDR_RAMP_DOWN) |
			    BITFMASK(sc55112_LED_TC2_LEDR_RAMP_UP);
			break;

		case TC_STROBE_SLOW:
			val =
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDR_SLOW,
				    sc55112_LED_TC2_STROBE_LED_SLOW_ENABLE);
			mask = BITFMASK(sc55112_LED_TC2_STROBE_LEDR_SLOW);
			break;

		case TC_STROBE_FAST:
			val =
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDR_FAST,
				    sc55112_LED_TC2_STROBE_LED_FAST_ENABLE);
			mask = BITFMASK(sc55112_LED_TC2_STROBE_LEDR_FAST);
			break;
		}
		break;

	case TCLED_FUN_CHANNEL2:
		switch (speed) {
		case TC_STROBE_OFF:
			val =
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDG_SLOW,
				    sc55112_LED_TC2_STROBE_LED_SLOW_DISABLE) |
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDG_FAST,
				    sc55112_LED_TC2_STROBE_LED_FAST_DISABLE) |
			    BITFVAL(sc55112_LED_TC2_LEDG_RAMP_DOWN,
				    sc55112_LED_TC2_LED_RAMP_DOWN_DISABLE) |
			    BITFVAL(sc55112_LED_TC2_LEDG_RAMP_UP,
				    sc55112_LED_TC2_LED_RAMP_UP_DISABLE);
			mask =
			    BITFMASK(sc55112_LED_TC2_STROBE_LEDG_SLOW) |
			    BITFMASK(sc55112_LED_TC2_STROBE_LEDG_FAST) |
			    BITFMASK(sc55112_LED_TC2_LEDG_RAMP_DOWN) |
			    BITFMASK(sc55112_LED_TC2_LEDG_RAMP_UP);
			break;

		case TC_STROBE_SLOW:
			val =
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDG_SLOW,
				    sc55112_LED_TC2_STROBE_LED_SLOW_ENABLE);
			mask = BITFMASK(sc55112_LED_TC2_STROBE_LEDG_SLOW);
			break;

		case TC_STROBE_FAST:
			val =
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDG_FAST,
				    sc55112_LED_TC2_STROBE_LED_FAST_ENABLE);
			mask = BITFMASK(sc55112_LED_TC2_STROBE_LEDG_FAST);
			break;
		}
		break;

	case TCLED_FUN_CHANNEL3:
		switch (speed) {
		case TC_STROBE_OFF:
			val =
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDB_SLOW,
				    sc55112_LED_TC2_STROBE_LED_SLOW_DISABLE) |
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDB_FAST,
				    sc55112_LED_TC2_STROBE_LED_FAST_DISABLE) |
			    BITFVAL(sc55112_LED_TC2_LEDB_RAMP_DOWN,
				    sc55112_LED_TC2_LED_RAMP_DOWN_DISABLE) |
			    BITFVAL(sc55112_LED_TC2_LEDB_RAMP_UP,
				    sc55112_LED_TC2_LED_RAMP_UP_DISABLE);
			mask =
			    BITFMASK(sc55112_LED_TC2_STROBE_LEDB_SLOW) |
			    BITFMASK(sc55112_LED_TC2_STROBE_LEDB_FAST) |
			    BITFMASK(sc55112_LED_TC2_LEDB_RAMP_DOWN) |
			    BITFMASK(sc55112_LED_TC2_LEDB_RAMP_UP);
			break;

		case TC_STROBE_SLOW:
			val =
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDB_SLOW,
				    sc55112_LED_TC2_STROBE_LED_SLOW_ENABLE);
			mask = BITFMASK(sc55112_LED_TC2_STROBE_LEDB_SLOW);
			break;

		case TC_STROBE_FAST:
			val =
			    BITFVAL(sc55112_LED_TC2_STROBE_LEDB_FAST,
				    sc55112_LED_TC2_STROBE_LED_FAST_ENABLE);
			mask = BITFMASK(sc55112_LED_TC2_STROBE_LEDB_FAST);
			break;
		}
		break;

	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function initiates Tri-color LED brightness Ramp Up function; Ramp time
 * is fixed at 1 second.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_rampup(t_funlight_bank bank,
				  t_funlight_channel channel)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case TCLED_FUN_CHANNEL1:
		val =
		    BITFVAL(sc55112_LED_TC2_LEDR_RAMP_UP,
			    sc55112_LED_TC2_LED_RAMP_UP_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_LEDR_RAMP_UP);
		break;

	case TCLED_FUN_CHANNEL2:
		val =
		    BITFVAL(sc55112_LED_TC2_LEDG_RAMP_UP,
			    sc55112_LED_TC2_LED_RAMP_UP_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_LEDG_RAMP_UP);
		break;

	case TCLED_FUN_CHANNEL3:
		val =
		    BITFVAL(sc55112_LED_TC2_LEDB_RAMP_UP,
			    sc55112_LED_TC2_LED_RAMP_UP_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_LEDB_RAMP_UP);
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function initiates Tri-color LED brightness Ramp Down function; Ramp
 * time is fixed at 1 second.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_rampdown(t_funlight_bank bank,
				    t_funlight_channel channel)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case TCLED_FUN_CHANNEL1:
		val =
		    BITFVAL(sc55112_LED_TC2_LEDR_RAMP_DOWN,
			    sc55112_LED_TC2_LED_RAMP_DOWN_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_LEDR_RAMP_DOWN);
		break;

	case TCLED_FUN_CHANNEL2:
		val =
		    BITFVAL(sc55112_LED_TC2_LEDG_RAMP_DOWN,
			    sc55112_LED_TC2_LED_RAMP_DOWN_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_LEDG_RAMP_DOWN);
		break;

	case TCLED_FUN_CHANNEL3:
		val =
		    BITFVAL(sc55112_LED_TC2_LEDB_RAMP_DOWN,
			    sc55112_LED_TC2_LED_RAMP_DOWN_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_LEDB_RAMP_DOWN);
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function enables a Tri-color channel triode mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_triode_on(t_funlight_bank bank,
				     t_funlight_channel channel)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case TCLED_FUN_CHANNEL1:
		val =
		    BITFVAL(sc55112_LED_TC2_LED_TC2_TRIODE1,
			    sc55112_LED_TC2_LED_TC2_TRIODE_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_LED_TC2_TRIODE1);
		break;

	case TCLED_FUN_CHANNEL2:
		val =
		    BITFVAL(sc55112_LED_TC2_LED_TC2_TRIODE2,
			    sc55112_LED_TC2_LED_TC2_TRIODE_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_LED_TC2_TRIODE2);
		break;

	case TCLED_FUN_CHANNEL3:
		val =
		    BITFVAL(sc55112_LED_TC2_LED_TC2_TRIODE3,
			    sc55112_LED_TC2_LED_TC2_TRIODE_ENABLE);
		mask = BITFMASK(sc55112_LED_TC2_LED_TC2_TRIODE3);
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function disables a Tri-color LED channel triode mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_triode_off(t_funlight_bank bank,
				      t_funlight_channel channel)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case TCLED_FUN_CHANNEL1:
		val =
		    BITFVAL(sc55112_LED_TC2_LED_TC2_TRIODE1,
			    sc55112_LED_TC2_LED_TC2_TRIODE_DISABLE);
		mask = BITFMASK(sc55112_LED_TC2_LED_TC2_TRIODE1);
		break;

	case TCLED_FUN_CHANNEL2:
		val =
		    BITFVAL(sc55112_LED_TC2_LED_TC2_TRIODE2,
			    sc55112_LED_TC2_LED_TC2_TRIODE_DISABLE);
		mask = BITFMASK(sc55112_LED_TC2_LED_TC2_TRIODE2);
		break;

	case TCLED_FUN_CHANNEL3:
		val =
		    BITFVAL(sc55112_LED_TC2_LED_TC2_TRIODE3,
			    sc55112_LED_TC2_LED_TC2_TRIODE_DISABLE);
		mask = BITFMASK(sc55112_LED_TC2_LED_TC2_TRIODE3);
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_TC_CONTROL2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function enables Tri-color LED edge slowing, this function does not
 * apply to sc55112.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_enable_edge_slow(void)
{
	return PMIC_NOT_SUPPORTED;
}

/*!
 * This function disables Tri-color LED edge slowing, this function does not
 * apply to sc55112.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_disable_edge_slow(void)
{
	return PMIC_NOT_SUPPORTED;
}

/*!
 * This function enables Tri-color LED half current mode, this function does
 * not apply to sc55112.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_enable_half_current(void)
{
	return PMIC_NOT_SUPPORTED;
}

/*!
 * This function disables Tri-color LED half current mode, this function does
 * not apply to sc55112.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_disable_half_current(void)
{
	return PMIC_NOT_SUPPORTED;
}

/*!
 * This function enables backlight or Tri-color LED audio modulation.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_enable_audio_modulation(t_led_channel channel,
					       t_aud_path path,
					       t_aud_gain gain, bool lpf_bypass)
{
	unsigned int val = 0, mask = 0;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	switch (channel) {
	case AUDIO_LED1:
		val =
		    BITFVAL(sc55112_LED_BKLT2_AUDLED1,
			    sc55112_LED_BKLT2_AUDLED_ENABLE);
		mask = BITFMASK(sc55112_LED_BKLT2_AUDLED1);
		break;

	case AUDIO_LED2:
		val =
		    BITFVAL(sc55112_LED_BKLT2_AUDLED2,
			    sc55112_LED_BKLT2_AUDLED_ENABLE);
		mask = BITFMASK(sc55112_LED_BKLT2_AUDLED2);
		break;

	case AUDIO_LEDR:
		val =
		    BITFVAL(sc55112_LED_BKLT2_AUDLEDR,
			    sc55112_LED_BKLT2_AUDLED_ENABLE);
		mask = BITFMASK(sc55112_LED_BKLT2_AUDLEDR);
		break;

	case AUDIO_LEDG:
		val =
		    BITFVAL(sc55112_LED_BKLT2_AUDLEDG,
			    sc55112_LED_BKLT2_AUDLED_ENABLE);
		mask = BITFMASK(sc55112_LED_BKLT2_AUDLEDG);
		break;

	case AUDIO_LEDB:
		val =
		    BITFVAL(sc55112_LED_BKLT2_AUDLEDB,
			    sc55112_LED_BKLT2_AUDLED_ENABLE);
		mask = BITFMASK(sc55112_LED_BKLT2_AUDLEDB);
		break;
	}

	val |= BITFVAL(sc55112_LED_BKLT2_AUDGAIN, gain);
	mask |= BITFMASK(sc55112_LED_BKLT2_AUDGAIN);

	val |= BITFVAL(sc55112_LED_BKLT2_AUDPATHSEL, path);
	mask |= BITFMASK(sc55112_LED_BKLT2_AUDPATHSEL);

	if (lpf_bypass != false) {
		val |=
		    BITFVAL(sc55112_LED_BKLT2_AUDLPFBYP,
			    sc55112_LED_BKLT2_AUDLPFBYP_LPF);
	} else {
		val |=
		    BITFVAL(sc55112_LED_BKLT2_AUDLPFBYP,
			    sc55112_LED_BKLT2_AUDLPFBYP_BYPASS);
	}
	mask |= BITFMASK(sc55112_LED_BKLT2_AUDLPFBYP);

	val |=
	    BITFVAL(sc55112_LED_BKLT2_AUDMODEN,
		    sc55112_LED_BKLT2_AUDMODEN_ENABLE);
	mask |= BITFMASK(sc55112_LED_BKLT2_AUDMODEN);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_2, val, mask));

	return PMIC_SUCCESS;

}

/*!
 * This function disables backlight or Tri-color LED audio modulation.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_disable_audio_modulation(void)
{
	unsigned int val, mask;

	if (suspend_flag == 1) {
		return PMIC_ERROR;
	}
	val =
	    BITFVAL(sc55112_LED_BKLT2_AUDMODEN,
		    sc55112_LED_BKLT2_AUDMODEN_DISABLE) |
	    BITFVAL(sc55112_LED_BKLT2_AUDLED1,
		    sc55112_LED_BKLT2_AUDLED_DISABLE) |
	    BITFVAL(sc55112_LED_BKLT2_AUDLED2,
		    sc55112_LED_BKLT2_AUDLED_DISABLE) |
	    BITFVAL(sc55112_LED_BKLT2_AUDLEDR,
		    sc55112_LED_BKLT2_AUDLED_DISABLE) |
	    BITFVAL(sc55112_LED_BKLT2_AUDLEDG,
		    sc55112_LED_BKLT2_AUDLED_DISABLE) |
	    BITFVAL(sc55112_LED_BKLT2_AUDLEDB,
		    sc55112_LED_BKLT2_AUDLED_DISABLE);

	mask = BITFMASK(sc55112_LED_BKLT2_AUDMODEN) |
	    BITFMASK(sc55112_LED_BKLT2_AUDLED1) |
	    BITFMASK(sc55112_LED_BKLT2_AUDLED2) |
	    BITFMASK(sc55112_LED_BKLT2_AUDLEDR) |
	    BITFMASK(sc55112_LED_BKLT2_AUDLEDG) |
	    BITFMASK(sc55112_LED_BKLT2_AUDLEDB);

	CHECK_ERROR(pmic_write_reg(PRIO_LIGHT, REG_BACKLIGHT_2, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function implements IOCTL controls on a PMIC Light device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int pmic_light_ioctl(struct inode *inode, struct file *file,
			    unsigned int cmd, unsigned long arg)
{
	t_bklit_setting_param *bklit_setting;
	t_fun_param *fun_param;
	unsigned int val;

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	if (_IOC_TYPE(cmd) != 'p')
		return -ENOTTY;

	switch (cmd) {
	case PMIC_BKLIT_ENABLE:
		pmic_bklit_master_enable();
		break;

	case PMIC_BKLIT_DISABLE:
		pmic_bklit_master_disable();
		break;

	case PMIC_SET_BKLIT:
		if ((bklit_setting =
		     kmalloc(sizeof(t_bklit_setting_param), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(bklit_setting, (t_bklit_setting_param *) arg,
				   sizeof(t_bklit_setting_param))) {
			kfree(bklit_setting);
			return -EFAULT;
		}

		CHECK_ERROR_KFREE(pmic_bklit_set_mode
				  (bklit_setting->channel, bklit_setting->mode),
				  (kfree(bklit_setting)));

		CHECK_ERROR_KFREE(pmic_bklit_set_strobemode
				  (bklit_setting->channel,
				   bklit_setting->strobe),
				  (kfree(bklit_setting)));

		CHECK_ERROR_KFREE(pmic_bklit_set_current
				  (bklit_setting->channel,
				   bklit_setting->current_level),
				  (kfree(bklit_setting)));

		CHECK_ERROR_KFREE(pmic_bklit_set_dutycycle
				  (bklit_setting->channel,
				   bklit_setting->duty_cycle),
				  (kfree(bklit_setting)));

		CHECK_ERROR_KFREE(pmic_bklit_set_cycle_time
				  (bklit_setting->cycle_time),
				  (kfree(bklit_setting)));

		if (bklit_setting->edge_slow != false) {
			CHECK_ERROR_KFREE(pmic_bklit_enable_edge_slow(),
					  (kfree(bklit_setting)));

		} else {
			CHECK_ERROR_KFREE(pmic_bklit_disable_edge_slow(),
					  (kfree(bklit_setting)));

		}
		kfree(bklit_setting);
		break;

	case PMIC_GET_BKLIT:
		if ((bklit_setting =
		     kmalloc(sizeof(t_bklit_setting_param), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}

		if (copy_from_user(bklit_setting, (t_bklit_setting_param *) arg,
				   sizeof(t_bklit_setting_param))) {
			kfree(bklit_setting);
			return -EFAULT;
		}

		CHECK_ERROR_KFREE(pmic_read_reg
				  (PRIO_LIGHT, REG_BACKLIGHT_1, &val),
				  (kfree(bklit_setting)));

		bklit_setting->current_level =
		    BITFEXT(val, sc55112_LED_BKLT1_LEDI);
		bklit_setting->cycle_time =
		    BITFEXT(val, sc55112_LED_BKLT1_BKLT1CYCLETIME);

		switch (bklit_setting->channel) {
		case BACKLIGHT_LED1:
			bklit_setting->duty_cycle =
			    BITFEXT(val, sc55112_LED_BKLT1_LED1DC);
			break;

		case BACKLIGHT_LED2:
			bklit_setting->duty_cycle =
			    BITFEXT(val, sc55112_LED_BKLT1_LED2DC);
			break;
		}

		CHECK_ERROR_KFREE(pmic_read_reg
				  (PRIO_LIGHT, REG_BACKLIGHT_2, &val),
				  (kfree(bklit_setting)));

		switch (bklit_setting->channel) {
		case BACKLIGHT_LED1:
			if (BITFEXT(val, sc55112_LED_BKLT2_LED1TRIODE) ==
			    sc55112_LED_BKLT2_LED1TRIODE_DISABLE) {
				bklit_setting->mode =
				    BACKLIGHT_CURRENT_CTRL_MODE;
			} else {
				bklit_setting->mode = BACKLIGHT_TRIODE_MODE;
			}

			if (BITFEXT(val, sc55112_LED_BKLT2_LED1STROBEFAST) ==
			    sc55112_LED_BKLT2_LEDSTROBEFAST_ENABLE) {
				bklit_setting->strobe = BACKLIGHT_STROBE_FAST;
			} else
			    if (BITFEXT(val, sc55112_LED_BKLT2_LED1STROBESLOW)
				== sc55112_LED_BKLT2_LEDSTROBESLOW_ENABLE) {
				bklit_setting->strobe = BACKLIGHT_STROBE_SLOW;
			} else {
				bklit_setting->strobe = BACKLIGHT_STROBE_NONE;
			}

			break;

		case BACKLIGHT_LED2:
			if (BITFEXT(val, sc55112_LED_BKLT2_LED2TRIODE) ==
			    sc55112_LED_BKLT2_LED2TRIODE_DISABLE) {
				bklit_setting->mode =
				    BACKLIGHT_CURRENT_CTRL_MODE;
			} else {
				bklit_setting->mode = BACKLIGHT_TRIODE_MODE;
			}

			if (BITFEXT(val, sc55112_LED_BKLT2_LED2STROBEFAST) ==
			    sc55112_LED_BKLT2_LEDSTROBEFAST_ENABLE) {
				bklit_setting->strobe = BACKLIGHT_STROBE_FAST;
			} else
			    if (BITFEXT(val, sc55112_LED_BKLT2_LED2STROBESLOW)
				== sc55112_LED_BKLT2_LEDSTROBESLOW_ENABLE) {
				bklit_setting->strobe = BACKLIGHT_STROBE_SLOW;
			} else {
				bklit_setting->strobe = BACKLIGHT_STROBE_NONE;
			}

			break;
		}

		if (BITFEXT(val, sc55112_LED_BKLT2_SLEWLIMEN) ==
		    sc55112_LED_BKLT2_SLEWLIMEN_ENABLE) {
			bklit_setting->edge_slow = true;
		} else {
			bklit_setting->edge_slow = false;
		}

		if (copy_to_user((t_bklit_setting_param *) arg, bklit_setting,
				 sizeof(t_bklit_setting_param))) {
			kfree(bklit_setting);
			return -EFAULT;
		}

	case PMIC_RAMPUP_BKLIT:
		CHECK_ERROR(pmic_bklit_rampup((t_bklit_channel) arg));
		break;

	case PMIC_RAMPDOWN_BKLIT:
		CHECK_ERROR(pmic_bklit_rampdown((t_bklit_channel) arg));
		break;

	case PMIC_TCLED_ENABLE:
		CHECK_ERROR(pmic_tcled_enable((t_tcled_mode) arg));
		break;

	case PMIC_TCLED_DISABLE:
		CHECK_ERROR(pmic_tcled_disable());
		break;

	case PMIC_TCLED_PATTERN:
		if ((fun_param =
		     kmalloc(sizeof(t_fun_param), GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user
		    (fun_param, (t_fun_param *) arg, sizeof(t_fun_param))) {
			kfree(fun_param);
			return -EFAULT;
		}

		switch (fun_param->pattern) {
		case BLENDED_RAMPS_SLOW:
			CHECK_ERROR_KFREE(pmic_tcled_fun_blendedramps
					  (fun_param->bank, TC_SLOW),
					  (kfree(fun_param)));
			break;

		case BLENDED_RAMPS_FAST:
			CHECK_ERROR_KFREE(pmic_tcled_fun_blendedramps
					  (fun_param->bank, TC_FAST),
					  (kfree(fun_param)));
			break;

		case SAW_RAMPS_SLOW:
			CHECK_ERROR_KFREE(pmic_tcled_fun_sawramps
					  (fun_param->bank, TC_SLOW),
					  (kfree(fun_param)));
			break;

		case SAW_RAMPS_FAST:
			CHECK_ERROR_KFREE(pmic_tcled_fun_sawramps
					  (fun_param->bank, TC_FAST),
					  (kfree(fun_param)));
			break;

		case BLENDED_BOWTIE_SLOW:
			CHECK_ERROR_KFREE(pmic_tcled_fun_blendedbowtie
					  (fun_param->bank, TC_SLOW),
					  (kfree(fun_param)));
			break;

		case BLENDED_BOWTIE_FAST:
			CHECK_ERROR_KFREE(pmic_tcled_fun_blendedbowtie
					  (fun_param->bank, TC_FAST),
					  (kfree(fun_param)));
			break;

		case STROBE_SLOW:
			CHECK_ERROR_KFREE(pmic_tcled_fun_strobe
					  (fun_param->bank, fun_param->channel,
					   TC_STROBE_SLOW), (kfree(fun_param)));
			break;

		case STROBE_FAST:
			CHECK_ERROR_KFREE(pmic_tcled_fun_strobe
					  (fun_param->bank, fun_param->channel,
					   TC_STROBE_SLOW), (kfree(fun_param)));
			break;

		case CHASING_LIGHT_RGB_SLOW:
			CHECK_ERROR_KFREE(pmic_tcled_fun_chasinglightspattern
					  (fun_param->bank, RGB, TC_SLOW),
					  (kfree(fun_param)));
			break;

		case CHASING_LIGHT_RGB_FAST:
			CHECK_ERROR_KFREE(pmic_tcled_fun_chasinglightspattern
					  (fun_param->bank, RGB, TC_FAST),
					  (kfree(fun_param)));
			break;

		case CHASING_LIGHT_BGR_SLOW:
			CHECK_ERROR_KFREE(pmic_tcled_fun_chasinglightspattern
					  (fun_param->bank, BGR, TC_SLOW),
					  (kfree(fun_param)));
			break;

		case CHASING_LIGHT_BGR_FAST:
			CHECK_ERROR_KFREE(pmic_tcled_fun_chasinglightspattern
					  (fun_param->bank, BGR, TC_FAST),
					  (kfree(fun_param)));
			break;
		}

		break;

	default:
		return -EINVAL;
		break;
	}
	return 0;
}

/*!
 * This function implements the open method on a PMIC Light device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_light_open(struct inode *inode, struct file *file)
{
	return 0;
}

/*!
 * This function implements the release method on a PMIC Light device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_light_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations pmic_light_fops = {
	.owner = THIS_MODULE,
	.ioctl = pmic_light_ioctl,
	.open = pmic_light_open,
	.release = pmic_light_release,
};

/*!
 * This is the suspend of power management for the sc55112 Light API.
 *
 * @param        dev            the device
 * @param        state          the state
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int pmic_light_suspend(struct device *dev, u32 state, u32 level)
{
	switch (level) {
	case SUSPEND_DISABLE:
		suspend_flag = 1;
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		/* switch off backlight */
		CHECK_ERROR(pmic_bklit_master_disable());
		/* switch off leds */
		CHECK_ERROR(pmic_tcled_disable());
		break;
	}

	return 0;
};

/*!
 * This is the resume of power management for the sc55112 adc API.
 * It suports RESTORE state.
 *
 * @param        dev            the device
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int pmic_light_resume(struct device *dev, u32 level)
{
	t_tcled_mode mode = TCLED_IND_MODE;

	switch (level) {
	case RESUME_POWER_ON:
		break;
	case RESUME_RESTORE_STATE:
		break;
	case RESUME_ENABLE:
		suspend_flag = 0;
		/* Enable Backlight */
		CHECK_ERROR(pmic_bklit_master_enable());

		/* Enable Tri-color LED */
		CHECK_ERROR(pmic_tcled_enable(mode));
		mode = TCLED_FUN_MODE;
		CHECK_ERROR(pmic_tcled_enable(mode));
		while (swait > 0) {
			swait--;
			wake_up_interruptible(&suspendq);
		}
		break;
	}
	return 0;
};

static struct device_driver pmic_light_driver_ldm = {
	.name = "PMIC_LIGHT",
	.bus = &platform_bus_type,
	.probe = NULL,
	.remove = NULL,
	.suspend = pmic_light_suspend,
	.resume = pmic_light_resume,
};

static struct platform_device pmic_light_ldm = {
	.name = "PMIC_LIGHT",
	.id = 1,
};

/*
 * Initialization and Exit
 */
static int __init pmic_light_init(void)
{
	int ret = 0;
	pmic_light_major = register_chrdev(0, "pmic_light", &pmic_light_fops);

	if (pmic_light_major < 0) {
		/* TRACEMSG_ADC(_K_D("Unable to get a major for pmic_light")); */
		return pmic_light_major;
	}

	devfs_mk_cdev(MKDEV(pmic_light_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "pmic_light");

	ret = driver_register(&pmic_light_driver_ldm);
	if (ret == 0) {

		ret = platform_device_register(&pmic_light_ldm);

		if (ret != 0) {
			driver_unregister(&pmic_light_driver_ldm);
		}

		printk(KERN_INFO "PMIC Light loaded\n");

	}
	return 0;
}

static void __exit pmic_light_exit(void)
{
	devfs_remove("pmic_light");
	unregister_chrdev(pmic_light_major, "pmic_light");
	driver_unregister(&pmic_light_driver_ldm);
	platform_device_unregister(&pmic_light_ldm);
	printk(KERN_INFO "PMIC Light unloaded\n");
}

/*
 * Module entry points
 */

subsys_initcall(pmic_light_init);
module_exit(pmic_light_exit);

MODULE_DESCRIPTION("PMIC Light device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
