/*
 * Copyright (C) 2007 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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
 * Motorola 2007-Aug-10 - NAND secure boot time improvement 
 * Motorola 2007-Jun-22 - Initial creation.
 */

#ifndef __CORE_H__
#define __CORE_H__

 /*!
 * @file core.h
 *
 * @ingroup poweric_core
 *
 * @brief The main internal header for the power_ic driver.
 *
 * This file contains all global declarations that are internal to the power_ic driver.
 */
 
 #include <linux/power_ic.h>

/*==================================================================================================
                                             ENUMS
==================================================================================================*/
/*! Permission to access backup memory. */
typedef enum
{
    BACKUP_MEMORY_ACCESS_NOT_ALLOWED,
    BACKUP_MEMORY_ACCESS_ALLOWED
} POWER_IC_BACKUP_MEMORY_ACCESS_T;

/*! Users for accessing backup memory. */
typedef enum
{
    POWER_IC_USER_USER_SPACE,
    POWER_IC_USER_KERNEL
} POWER_IC_USER_T;

/*==================================================================================================
                                       GLOBAL VARIABLES
==================================================================================================*/

extern unsigned long int reg_init_tbl[POWER_IC_REG_NUM_REGS_ATLAS];

/*==================================================================================================
                                      FUNCTION PROTOTYPES
==================================================================================================*/

extern int power_ic_backup_memory_read_sec(POWER_IC_BACKUP_MEMORY_ID_T id,
                                           int* value,
                                           POWER_IC_USER_T user);

extern int power_ic_backup_memory_write_sec(POWER_IC_BACKUP_MEMORY_ID_T id,
                                            int value,
                                            POWER_IC_USER_T user);

extern int power_ic_write_reg_sec(POWER_IC_REGISTER_T reg,
                                  unsigned int *reg_value,
                                  POWER_IC_BACKUP_MEMORY_ACCESS_T permission);

extern int power_ic_set_reg_mask_sec(POWER_IC_REGISTER_T reg,
                                     int mask,
                                     int value,
                                     POWER_IC_BACKUP_MEMORY_ACCESS_T permission);

#endif /* __CORE_H__ */
