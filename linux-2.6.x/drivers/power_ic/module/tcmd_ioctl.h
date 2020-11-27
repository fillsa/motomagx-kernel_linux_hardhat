/*
 * Copyright (C) 2005-2007 Motorola, Inc.
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
 * Motorola 2007-Sep-26 - Updated the copyright.
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2005-Apr-29 - Design of the ioctl commands for TCMD.  
 */
 
#ifndef __TCMD_IOCTL_H__
#define __TCMD_IOCTL_H__

/*!
 * @file tcmd_ioctl.h
 * @brief This is the header of internal definitions for the power IC TCMD ioctl interface.
 * 
 * @ingroup poweric_tcmd_ioctl
 */

#include <linux/power_ic.h>

int tcmd_ioctl(unsigned int cmd, unsigned long arg);

#endif /* __TCMD_IOCTL_H__ */
