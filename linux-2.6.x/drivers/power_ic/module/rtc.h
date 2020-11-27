/*
 * Copyright (C) 2005-2006 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2005-Feb-28 - Rewrote the software
 */

#ifndef __RTC_H__
#define __RTC_H__

/*!
 * @file rtc.h
 *
 * @brief This is the header of internal definitions the power IC RTC interface.
 *
 * @ingroup poweric_rtc
 */

/*==================================================================================================
     Header Name: rtc.h

     General Description: RTC Driver

==================================================================================================*/

/*====================================================================================================
                                         INCLUDE FILES
==================================================================================================*/

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                            MACROS
==================================================================================================*/

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATION
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/
int rtc_ioctl(unsigned int cmd, unsigned long arg);

#endif /* __RTC_H__ */
