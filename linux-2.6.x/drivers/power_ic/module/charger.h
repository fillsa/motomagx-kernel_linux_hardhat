/*
 * Copyright (C) 2005-2006 Motorola, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-May-17 - USB Charge State Macro Removal
 * Motorola 2006-Apr-11 - USB Charge State Addition
 * Motorola 2005-Feb-28 - File Creation
 */

#ifndef __CHARGER_H__
#define __CHARGER_H__

/*!
 * @file charger.h
 * @brief This is the header of internal definitions for the power IC charger control interface.
 *
 * @ingroup poweric_charger
 */
 
#include <linux/power_ic.h>

/**************************************************************************************
 * Internal Global driver macros
 **************************************************************************************/

/**************************************************************************************
 * Internal Global driver variables
 **************************************************************************************/

/**************************************************************************************
 * Internal Global driver functions
 **************************************************************************************/
int charger_ioctl(unsigned int cmd, unsigned long arg);

#endif /* __CHARGER_H__ */
