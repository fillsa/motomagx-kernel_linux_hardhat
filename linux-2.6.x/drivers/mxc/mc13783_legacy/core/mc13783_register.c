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
 * @file mc13783_register.c
 * @brief This file contains register function of mc13783 driver.
 *
 * @ingroup MC13783
 */

/*
 * Includes
 */
#include "mc13783_register.h"
#include "mc13783_spi_inter.h"
#include "mc13783_config.h"

/*!
 * This function initializes register access.
 *
 * @return       This function returns 0 if successful.
 */
int init_register_access_wait(void)
{
	CHECK_ERROR(spi_init());
	return ERROR_NONE;
}

/*!
 * This function reads a register in high speed mode.
 *
 * @param        reg   	    register.
 * @param        reg_value   register value.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_hs_read_reg(int reg, unsigned int *reg_value)
{
	/* hight speed read  register, use by mc13783 protocol driver */
	return mc13783_reg_access(0, 1, reg, reg_value);
}

/*!
 * This function writes a register in high speed mode.
 *
 * @param        reg   	    register.
 * @param        reg_value   register value.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_hs_write_reg(int reg, unsigned int *reg_value)
{
	/* hight speed write register, use by mc13783 protocol driver */
	return mc13783_reg_access(0, 0, reg, reg_value);
}

/*!
 * This function R/W register on mc13783.
 *
 * @param        priority   priority of access
 * @param        rw   	    Select read or write operation
 * @param        reg   	    the register
 * @param        reg_value   the register value
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_reg_access(t_prio priority, bool rw, int reg,
		       unsigned int *reg_value)
{
	if (rw == 1) {
		TRACEMSG(_K_D("read reg = %d"), reg);
		CHECK_ERROR(spi_read_reg(reg, reg_value));
		TRACEMSG(_K_D("read value = 0x%x"), *reg_value);
	} else {
		TRACEMSG(_K_D("write reg = %d, value 0x%x"), reg, *reg_value);
		CHECK_ERROR(spi_write_reg(reg, reg_value));
	}
	TRACEMSG(_K_D("r/w register done"));
	return ERROR_NONE;
}
