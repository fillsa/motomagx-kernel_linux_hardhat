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

#ifndef SC55112_REGISTER_H__
#define SC55112_REGISTER_H__

/*!
 * @file sc55112_register.h
 * @brief This file contains prototypes of register functions.
 *
 * @ingroup PMIC_SC55112_CORE
 */

/*
 * Includes
 */
#include "sc55112_config.h"
#include <asm/arch/pmic_status.h>
#include <asm/arch/pmic_external.h>

#define MAX_CLIENTS     100	/* max number of clients */
#define MAX_PRIO_QUEUE  8

typedef enum {
	sc55112_FREE,
	sc55112_BUSY,
} t_sc55112_status;

PMIC_STATUS init_register_access(void);

/*!
 * This function initializes register access.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS sc55112_init_register_access(void);

/*!
 * This function reads a register in high speed mode.
 *
 * @param        priority   priority of access
 * @param        reg        register.
 * @param        reg_value   register value.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS sc55112_read_reg(t_prio priority, int reg, unsigned int *reg_value);

/*!
 * This function writes a register in high speed mode.
 *
 * @param        priority   priority of access
 * @param        reg        register.
 * @param        reg_value  register value.
 * @param        reg_mask   bitmap mask indicating which bits to be modified.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS sc55112_write_reg(t_prio priority, int reg, unsigned int reg_value,
			      unsigned int reg_mask);

#endif				/* _SC55112_REGISTER_H__ */
