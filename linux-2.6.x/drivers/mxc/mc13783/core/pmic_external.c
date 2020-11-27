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
 * @file mc13783_external.c
 * @brief This file contains all external functions of mc13783 driver.
 *
 * @ingroup MC13783
 */

/*
 * Includes
 */
#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_status.h>

#include <linux/config.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include "pmic_config.h"
#include "pmic_event.h"

extern int send_frame_to_spi(int, unsigned int *, bool);

/*!
 * This function returns the PMIC version in system.
 *
 * @return       This function returns PMIC version.
 */
t_pmic_version pmic_get_version(void)
{
	return PMIC_MC13783;
}

/*!
 * This function is called by PMIC clients to read a register on PMIC.
 *
 * @param        priority   priority of access
 * @param        reg        number of register
 * @param        reg_value   return value of register
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_read_reg(t_prio priority, int reg, unsigned int *reg_value,
			  unsigned int reg_mask)
{
	unsigned int temp = 0;

	TRACEMSG(_K_D("read reg = %d"), reg);

	send_frame_to_spi(reg, &temp, false);
	*reg_value = (temp & reg_mask);

	TRACEMSG(_K_D("write register done, read value = 0x%x"), *reg_value);
	return PMIC_SUCCESS;
}

/*!
 * This function is called by PMIC clients to write a register on MC13783.
 *
 * @param        priority   priority of access
 * @param        reg        number of register
 * @param        reg_value  New value of register
 * @param        reg_mask   Bitmap mask indicating which bits to modify
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_write_reg(t_prio priority, int reg, unsigned int reg_value,
			   unsigned int reg_mask)
{
	unsigned int temp = 0;

	TRACEMSG(_K_D("write reg = %d"), reg);

	send_frame_to_spi(reg, &temp, false);
	temp = (temp & (~reg_mask)) | reg_value;
	send_frame_to_spi(reg, &temp, true);

	TRACEMSG(_K_D("write register done"));
	return PMIC_SUCCESS;
}

/*!
 * This function is called by mc13783 clients to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_event_subscribe(type_event_notification * event_sub)
{
	return mc13783_event_subscribe_internal(event_sub);
}

/*!
 * This function is called by mc13783 clients to un-subscribe on an event.
 *
 * @param        event_unsub   structure of event, it contains type of event and callback
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_event_unsubscribe(type_event_notification * event_unsub)
{
	return mc13783_event_unsubscribe_internal(event_unsub);
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
	event->param = (void *)NULL;

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
	return false;
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
	int sense_0 = 0;
	int sense_1 = 0;

	memset(sensor_bits, 0, sizeof(t_sensor_bits));

	pmic_read_reg(0, REG_INTERRUPT_SENSE_0, &sense_0, 0xffffff);
	pmic_read_reg(0, REG_INTERRUPT_SENSE_1, &sense_1, 0xffffff);

	if (sense_0 & 0x000040)
		sensor_bits->sense_chgdets = true;
	if (sense_0 & 0x000080)
		sensor_bits->sense_chgovs = true;
	if (sense_0 & 0x000100)
		sensor_bits->sense_chgrevs = true;
	if (sense_0 & 0x000200)
		sensor_bits->sense_chgshorts = true;
	if (sense_0 & 0x000400)
		sensor_bits->sense_cccvs = true;
	if (sense_0 & 0x000800)
		sensor_bits->sense_chgcurrs = true;
	if (sense_0 & 0x001000)
		sensor_bits->sense_bpons = true;
	if (sense_0 & 0x002000)
		sensor_bits->sense_lobatls = true;
	if (sense_0 & 0x004000)
		sensor_bits->sense_lobaths = true;
	if (sense_0 & 0x010000)
		sensor_bits->sense_usb4v4s = true;
	if (sense_0 & 0x020000)
		sensor_bits->sense_usb2v0s = true;
	if (sense_0 & 0x040000)
		sensor_bits->sense_usb0v8s = true;
	if (sense_0 & 0x080000)
		sensor_bits->sense_id_floats = true;
	if (sense_0 & 0x100000)
		sensor_bits->sense_id_gnds = true;
	if (sense_0 & 0x200000)
		sensor_bits->sense_se1s = true;
	if (sense_0 & 0x400000)
		sensor_bits->sense_ckdets = true;

	if (sense_1 & 0x000008)
		sensor_bits->sense_onofd1s = true;
	if (sense_1 & 0x000010)
		sensor_bits->sense_onofd2s = true;
	if (sense_1 & 0x000020)
		sensor_bits->sense_onofd3s = true;
	if (sense_1 & 0x000800)
		sensor_bits->sense_pwrrdys = true;
	if (sense_1 & 0x001000)
		sensor_bits->sense_thwarnhs = true;
	if (sense_1 & 0x002000)
		sensor_bits->sense_thwarnls = true;
	if (sense_1 & 0x004000)
		sensor_bits->sense_clks = true;
	if (sense_1 & 0x020000)
		sensor_bits->sense_mc2bs = true;
	if (sense_1 & 0x040000)
		sensor_bits->sense_hsdets = true;
	if (sense_1 & 0x080000)
		sensor_bits->sense_hsls = true;
	if (sense_1 & 0x100000)
		sensor_bits->sense_alspths = true;
	if (sense_1 & 0x200000)
		sensor_bits->sense_ahsshorts = true;

	return 0;

	return PMIC_SUCCESS;
}

EXPORT_SYMBOL(pmic_get_version);
EXPORT_SYMBOL(pmic_read_reg);
EXPORT_SYMBOL(pmic_write_reg);
EXPORT_SYMBOL(pmic_event_subscribe);
EXPORT_SYMBOL(pmic_event_unsubscribe);
EXPORT_SYMBOL(pmic_event_init);
EXPORT_SYMBOL(pmic_check_sensor);
EXPORT_SYMBOL(pmic_get_sensors);
