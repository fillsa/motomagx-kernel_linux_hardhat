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

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "mc13783_register.h"
#include "mc13783_event.h"
#include "mc13783_external.h"

EXPORT_SYMBOL(mc13783_read_reg);
EXPORT_SYMBOL(mc13783_write_reg);
EXPORT_SYMBOL(mc13783_write_reg_value);
EXPORT_SYMBOL(mc13783_set_reg_bit);
EXPORT_SYMBOL(mc13783_get_reg_bit);
EXPORT_SYMBOL(mc13783_set_reg_value);
EXPORT_SYMBOL(mc13783_get_reg_value);
EXPORT_SYMBOL(mc13783_event_subscribe);
EXPORT_SYMBOL(mc13783_event_unsubscribe);
EXPORT_SYMBOL(mc13783_event_init);
EXPORT_SYMBOL(mc13783_get_sense);

/*!
 * This function is called by mc13783 clients to read a register on mc13783.
 *
 * @param        priority   priority of access
 * @param        reg   	    number of register
 * @param        reg_value   return value of register
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_read_reg(t_prio priority, int reg, unsigned int *reg_value)
{
	return mc13783_reg_access(priority, true, reg, reg_value);
}

/*!
 * This function is called by mc13783 clients to write a register on mc13783.
 *
 * @param        priority   priority of access
 * @param        reg   	    number of register
 * @param        reg_value   New value of register
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_write_reg(t_prio priority, int reg, unsigned int *reg_value)
{
	return mc13783_reg_access(priority, false, reg, reg_value);
}

/*!
 * This function is called by mc13783 clients to write a register with immediate value on mc13783.
 *
 * @param        priority   priority of access
 * @param        reg   	    number of register.
 * @param        reg_value   New value of register.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_write_reg_value(t_prio priority, int reg, unsigned int reg_value)
{
	return mc13783_reg_access(priority, false, reg, &reg_value);
}

/*!
 * This function is called by mc13783 clients to set a bit in a register of mc13783.
 *
 * @param        priority   priority of access
 * @param        reg	    Register
 * @param        index	    Index in register of the bit
 * @param        value	    New value of the bit (true or false)
 *
 * @return       This function returns 0 if successful.
 */

int mc13783_set_reg_bit(t_prio priority, int reg, int index, int value)
{
	unsigned int reg_value = 0;
	unsigned int mask = 0;
	CHECK_ERROR(mc13783_reg_access(priority, true, reg, &reg_value));
	mask = (1 << index);
	reg_value = reg_value & ~(mask);
	reg_value = reg_value | mask * ((bool) value);
	return mc13783_reg_access(priority, false, reg, &reg_value);
}

/*!
 * This function is called by mc13783 clients to get a bit in a register of mc13783.
 *
 * @param        priority   priority of access
 * @param        reg	    Register
 * @param        index	    Index in register of the bit
 * @param        value	    return value of the bit (true or false)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_get_reg_bit(t_prio priority, int reg, int index, int *value)
{
	unsigned int reg_value = 0;
	unsigned int mask = 0;
	*value = false;
	CHECK_ERROR(mc13783_reg_access(priority, true, reg, &reg_value));
	mask = (1 << index);
	if (reg_value & mask) {
		*value = true;
	}
	return 0;
}

/*!
 * This function is called by mc13783 clients to set a value in a register of mc13783.
 *
 * @param        priority   priority of access
 * @param        reg	    Register
 * @param        index	    Index in register of the bit
 * @param        value	    New value of the bit (true or false)
 * @param        nb_bits    width of the value (in number of bits)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_set_reg_value(t_prio priority, int reg, int index, int value,
			  int nb_bits)
{
	unsigned int reg_value = 0;
	unsigned int mask = 0;
	mask = ((1 << nb_bits) - 1) << index;
	CHECK_ERROR(mc13783_reg_access(priority, true, reg, &reg_value));
	reg_value = reg_value & (~mask);
	reg_value = reg_value | (value << index);

	return mc13783_reg_access(priority, false, reg, &reg_value);
}

/*!
 * This function is called by mc13783 clients to get a value in a register of mc13783.
 *
 * @param        priority   priority of access
 * @param        reg	    Register
 * @param        index	    Index in register of the bit
 * @param        value	    New value of bits
 * @param        nb_bits    width of the value (in number of bits)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_get_reg_value(t_prio priority, int reg, int index, int *value,
			  int nb_bits)
{
	unsigned int reg_value = 0;
	unsigned int mask = 0;

	if ((reg > REG_NB) || (index > 25) || (nb_bits > 25)) {
		return ERROR_BAD_INPUT;
	}
	mask = ((1 << nb_bits) - 1) << index;
	CHECK_ERROR(mc13783_reg_access(priority, true, reg, &reg_value));
	reg_value = (reg_value & mask);
	*value = (reg_value >> index);
	return 0;
}

/*!
 * This function is called by mc13783 clients to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_event_subscribe(type_event_notification event_sub)
{
	return mc13783_event_subscribe_internal(event_sub);
}

/*!
 * This function is called by mc13783 clients to un-subscribe on an event.
 *
 * @param        event_unsub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_event_unsubscribe(type_event_notification event_unsub)
{
	return mc13783_event_unsubscribe_internal(event_unsub);
}

/*!
* This function is called to initialize an event.
*
* @param        event  structure of event.
*
* @return       This function returns 0 if successful.
*/
int mc13783_event_init(type_event_notification * event)
{
	event->event = 0;
	event->callback = NULL;
	event->callback_p = NULL;
	event->param = (void *)NO_PARAM;
	return 0;
}

/*!
* This function is called to read all sense bits of mc13783.
*
* @param        sense_bits  structure of all sense bits.
*
* @return       This function returns 0 if successful.
*/
int mc13783_get_sense(t_sense_bits * sense_bits)
{
	int sense_0 = 0, sense_1 = 0;

	sense_bits->sense_chgdets = false;
	sense_bits->sense_chgovs = false;
	sense_bits->sense_chgrevs = false;
	sense_bits->sense_chgshorts = false;
	sense_bits->sense_cccvs = false;
	sense_bits->sense_chgcurrs = false;
	sense_bits->sense_bpons = false;
	sense_bits->sense_lobatls = false;
	sense_bits->sense_lobaths = false;
	sense_bits->sense_usb4v4s = false;
	sense_bits->sense_usb2v0s = false;
	sense_bits->sense_usb0v8s = false;
	sense_bits->sense_id_floats = false;
	sense_bits->sense_id_gnds = false;
	sense_bits->sense_se1s = false;
	sense_bits->sense_ckdets = false;
	sense_bits->sense_mc2bs = false;
	sense_bits->sense_hsdets = false;
	sense_bits->sense_hsls = false;
	sense_bits->sense_alspths = false;
	sense_bits->sense_ahsshorts = false;
	sense_bits->sense_onofd1s = false;
	sense_bits->sense_onofd2s = false;
	sense_bits->sense_onofd3s = false;
	sense_bits->sense_pwrrdys = false;
	sense_bits->sense_thwarnhs = false;
	sense_bits->sense_thwarnls = false;
	sense_bits->sense_clks = false;

	CHECK_ERROR(mc13783_hs_read_reg(REG_INTERRUPT_SENSE_0, &sense_0));
	CHECK_ERROR(mc13783_hs_read_reg(REG_INTERRUPT_SENSE_1, &sense_1));

	if (sense_0 & 0x000040)
		sense_bits->sense_chgdets = true;
	if (sense_0 & 0x000080)
		sense_bits->sense_chgovs = true;
	if (sense_0 & 0x000100)
		sense_bits->sense_chgrevs = true;
	if (sense_0 & 0x000200)
		sense_bits->sense_chgshorts = true;
	if (sense_0 & 0x000400)
		sense_bits->sense_cccvs = true;
	if (sense_0 & 0x000800)
		sense_bits->sense_chgcurrs = true;
	if (sense_0 & 0x001000)
		sense_bits->sense_bpons = true;
	if (sense_0 & 0x002000)
		sense_bits->sense_lobatls = true;
	if (sense_0 & 0x004000)
		sense_bits->sense_lobaths = true;
	if (sense_0 & 0x010000)
		sense_bits->sense_usb4v4s = true;
	if (sense_0 & 0x020000)
		sense_bits->sense_usb2v0s = true;
	if (sense_0 & 0x040000)
		sense_bits->sense_usb0v8s = true;
	if (sense_0 & 0x080000)
		sense_bits->sense_id_floats = true;
	if (sense_0 & 0x100000)
		sense_bits->sense_id_gnds = true;
	if (sense_0 & 0x200000)
		sense_bits->sense_se1s = true;
	if (sense_0 & 0x400000)
		sense_bits->sense_ckdets = true;

	if (sense_1 & 0x000008)
		sense_bits->sense_onofd1s = true;
	if (sense_1 & 0x000010)
		sense_bits->sense_onofd2s = true;
	if (sense_1 & 0x000020)
		sense_bits->sense_onofd3s = true;
	if (sense_1 & 0x000800)
		sense_bits->sense_pwrrdys = true;
	if (sense_1 & 0x001000)
		sense_bits->sense_thwarnhs = true;
	if (sense_1 & 0x002000)
		sense_bits->sense_thwarnls = true;
	if (sense_1 & 0x004000)
		sense_bits->sense_clks = true;
	if (sense_1 & 0x020000)
		sense_bits->sense_mc2bs = true;
	if (sense_1 & 0x040000)
		sense_bits->sense_hsdets = true;
	if (sense_1 & 0x080000)
		sense_bits->sense_hsls = true;
	if (sense_1 & 0x100000)
		sense_bits->sense_alspths = true;
	if (sense_1 & 0x200000)
		sense_bits->sense_ahsshorts = true;

	return 0;
}
