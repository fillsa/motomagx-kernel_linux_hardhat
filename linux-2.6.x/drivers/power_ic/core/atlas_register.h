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
 * Motorola 2005-Feb-21 - Redesign of the register functions for Atlas driver.
 */

#ifndef __ATLAS_REGISTER_H__
#define __ATLAS_REGISTER_H__

/*!
 * @file atlas_register.h
 *
 * @brief This file contains prototypes of ATLAS register access functions. 
 *
 * @ingroup poweric_core
 */ 

int atlas_reg_read (int reg, unsigned int *reg_value);
int atlas_reg_write (int reg, unsigned int reg_value);

#endif /* __ATLAS_REGISTER_H__ */
