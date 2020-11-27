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
 * @file pmic_external.c
 * @brief This file contains all external functions of sc55112 driver.
 *
 * @ingroup PMIC_sc55112_CORE
 */

/*
 * Includes
 */

#include "sc55112_config.h"
#include "sc55112_register.h"
#include "sc55112_event.h"
#include "asm/arch/pmic_status.h"
#include "asm/arch/pmic_external.h"

EXPORT_SYMBOL(pmic_get_version);
EXPORT_SYMBOL(pmic_read_reg);
EXPORT_SYMBOL(pmic_write_reg);
EXPORT_SYMBOL(pmic_event_subscribe);
EXPORT_SYMBOL(pmic_event_unsubscribe);
EXPORT_SYMBOL(pmic_event_init);
EXPORT_SYMBOL(pmic_check_sensor);
EXPORT_SYMBOL(pmic_get_sensors);

static int sensor_offset[] = { 6, 7, 8, 9, 10, 11, 12, 14, 16, 17, 18, 20, 21 };

/*!
 * This function returns the PMIC version in system.
 *
 * @return       This function returns PMIC version.
 */
t_pmic_version pmic_get_version(void)
{
	return PMIC_sc55112;
}

/*!
 * This function is called by PMIC clients to read a register on sc55112.
 *
 * @param        priority   priority of access
 * @param        reg        number of register
 * @param        reg_value  return value of register
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_read_reg(t_prio priority, int reg, unsigned int *reg_value)
{
	return sc55112_read_reg(priority, reg, reg_value);
}

/*!
 * This function is called by PMIC clients to write a register on sc55112.
 *
 * @param        priority   priority of access
 * @param        reg        number of register
 * @param        reg_value  new value of register
 * @param        reg_mask   bitmap mask indicating which bits to modify
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_write_reg(t_prio priority, int reg, unsigned int reg_value,
			   unsigned int reg_mask)
{
	return sc55112_write_reg(priority, reg, reg_value, reg_mask);
}

/*!
 * This function is called by PMIC clients to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_event_subscribe(type_event_notification event_sub)
{
	return sc55112_event_subscribe_internal(event_sub);
}

/*!
 * This function is called by PMIC clients to un-subscribe on an event.
 *
 * @param        event_unsub   structure of event, it contains type of event and callback
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_event_unsubscribe(type_event_notification event_unsub)
{
	return sc55112_event_unsubscribe_internal(event_unsub);
}

/*!
* This function is called to initialize an event.
*
* @param        event  structure of event.
*
* @return       This function returns PMIC_SUCCESS if successful.
*/
PMIC_STATUS pmic_event_init(type_event_notification * event)
{
	event->event = 0;
	event->callback = NULL;
	event->param = (void *)NO_PARAM;
	return PMIC_SUCCESS;
}

/*!
* This function is called to read all sensor bits of PMIC.
*
* @param        sensor    Sensor to be checked.
*
* @return       This function returns true if the sensor bit is high;
*               or returns false if the sensor bit is low.
*/
bool pmic_check_sensor(t_sensor sensor)
{
	int rrstat = 0;

	CHECK_ERROR(pmic_read_reg(PRIO_CORE, REG_PSTAT, &rrstat));

	return (rrstat & (1 << sensor_offset[sensor])) ? true : false;
}

/*!
* This function checks one sensor of PMIC.
*
* @param        sensor_bits  structure of all sensor bits.
*
* @return       This function returns PMIC_SUCCESS if successful.
*/
PMIC_STATUS pmic_get_sensors(t_sensor_bits * sensor_bits)
{
	int rrstat = 0;

	CHECK_ERROR(pmic_read_reg(PRIO_CORE, REG_PSTAT, &rrstat));

	sensor_bits->usbdet_44v = (rrstat & (1 << 6)) ? true : false;
	sensor_bits->onoffsns = (rrstat & (1 << 7)) ? true : false;
	sensor_bits->onoffsns2 = (rrstat & (1 << 8)) ? true : false;
	sensor_bits->usbdet_08v = (rrstat & (1 << 9)) ? true : false;
	sensor_bits->mobsnsb = (rrstat & (1 << 10)) ? true : false;
	sensor_bits->pttsns = (rrstat & (1 << 11)) ? true : false;
	sensor_bits->a1sns = (rrstat & (1 << 12)) ? true : false;
	sensor_bits->usbdet_20v = (rrstat & (1 << 14)) ? true : false;
	sensor_bits->eol_stat = (rrstat & (1 << 16)) ? true : false;
	sensor_bits->clk_stat = (rrstat & (1 << 17)) ? true : false;
	sensor_bits->sys_rst = (rrstat & (1 << 18)) ? true : false;
	sensor_bits->warm_sys_rst = (rrstat & (1 << 20)) ? true : false;
	sensor_bits->batt_det_in_sns = (rrstat & (1 << 21)) ? true : false;

	return PMIC_SUCCESS;
}
