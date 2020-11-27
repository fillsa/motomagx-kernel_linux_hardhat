/*
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * Motorola 2007-Jun-13 - Remove keyv_shutdown
 * Motorola 2006-May-12 - Initial Creation
 */


#ifndef __KEYV_REGISTER_H__
#define __KEYV_REGISTER_H__  

/*!
 * @file keyv_register.h
 * @brief This file contains prototypes of the KeyV driver available
 * in kernel space.
 *
 * @ingroup keyv
 */

/******************************************************************************
* Global variables
******************************************************************************/

/******************************************************************************
* Global Function Prototypes
******************************************************************************/
void keyv_initialize(void (*reg_init_fcn)(void));
int keyv_reg_read (unsigned int *reg_value);
int keyv_reg_write (unsigned int reg_value);

#endif  /* __KEYV_REGISTER_H__ */
