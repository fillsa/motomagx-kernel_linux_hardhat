/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2005-2006 Motorola, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Motorola 2006-Oct-06 - Update File
 * Motorola 2006-Jun-22 - Move location of atlas_spi_inter.h
 * Motorola 2005-Feb-21 - Redesign of the register functions for Atlas driver. 
 */

/*!
 * @file atlas_register.c
 *
 * @brief This file contains register function of ATLAS driver. 
 *
 * @ingroup poweric_core
 */

#include <asm/arch/mc13783_spi_inter.h>
#include "os_independent.h"

/*!
 * @brief Reads a ATLAS register
 *
 * This function implements a read of a given ATLAS register.  
 *
 * @param reg          ATLAS register number
 * @param value_ptr    Pointer to which the read data should be stored
 *
 * @return 0 if successful
 */

int atlas_reg_read (int reg, unsigned int *value_ptr)
{ 
    return (spi_read_reg (reg, value_ptr));
}

/*!
 * @brief Writes a ATLAS register
 *
 * This function implements a write to a given ATLAS register.  
 *
 * @param reg          ATLAS register number
 * @param value        Value to write to the register
 *
 * @return 0 if successful
 */

int atlas_reg_write (int reg, unsigned int value)
{
    return (spi_write_reg (reg, &value));
}
