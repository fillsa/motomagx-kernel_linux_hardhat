/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
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

#ifndef MC13783_REGISTER_H_
#define MC13783_REGISTER_H_

/*!
 * @file mc13783_register.h
 * @brief This file contains prototypes of register functions.
 *
 * @ingroup MC13783
 */

/*
 * Includes
 */
#include "mc13783_config.h"
#include "mc13783_external.h"

#define MAX_CLIENTS 100		/* max number of clients */
#define MAX_PRIO_QUEUE  8
#define MAX_HS_CLIENTS 20	/* max number of hight speed clients */

typedef enum {
	MC13783_FREE,
	MC13783_BUSY,
} t_mc13783_status;

/*!
 * This function initializes register access.
 *
 * @return       This function returns 0 if successful.
 */
int init_register_access_wait(void);

/*!
 * This function sleeps for R/W access if the SPI is busy.
 *
 * @param        priority   priority of access
 *
 * @return       This function returns 0 if successful.
 */
int register_access_wait(t_prio priority);

/*!
 * This function reads a register in high speed mode.
 *
 * @param        reg   	    register.
 * @param        reg_value   register value.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_hs_read_reg(int reg, unsigned int *reg_value);

/*!
 * This function writes a register in high speed mode.
 *
 * @param        reg   	    register.
 * @param        reg_value   register value.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_hs_write_reg(int reg, unsigned int *reg_value);

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
		       unsigned int *reg_value);

/*!
 * This function unlocks on locked register access.
 *
 * @return       This function returns 0 if successful.
 */
int unlock_client(void);

#endif				/* MC13783_REGISTER_H_ */
