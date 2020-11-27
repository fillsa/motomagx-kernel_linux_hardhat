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

#ifndef         _MC13783_CONNECTIVITY_DEFS_H
#define         _MC13783_CONNECTIVITY_DEFS_H

/*
 * USB 0
 */
#define         BIT_FSENB               0
#define         BIT_USBSUSPEND          1
#define         BIT_USBPU               2
#define         BIT_UDPPD               3
#define         BIT_UDMPD               4
#define         BIT_DP150KPU            5
#define         BIT_VBUSPDENB           6
#define         BITS_CURRENT_LIMIT      7
#define         LONG_CURRENT_LIMIT      3
#define         BIT_DLPSRP              10
#define         BIT_SE0CONN             11
#define         BIT_USBXCVREN           12
#define         BIT_PULLOVER            13
#define         BITS_INTERFACE_MODE     14
#define         LONG_INTERFACE_MODE     3
#define         BIT_DATSE0              17
#define         BIT_BIDIR               18
#define         BIT_USBCNTRL            19
#define         BIT_IDPD                20
#define         BIT_IDPULSE             21
#define         BIT_IDPUCNTRL           22
#define         BIT_DMPULSE             23

/*
 * USB 1
 */
#define         BITS_VUSB_IN            0
#define         LONG_VUSB_IN            2
#define         BIT_VUSB                2
#define         BIT_VUSBEN              3
#define         BIT_VBUSEN              5

#define         BIT_RSPOL               6
#define         BIT_RSTRI               7

#define         MODE_USB                0
#define         MODE_RS232_1            1
#define         MODE_RS232_2            2
#define         MODE_AUDIO_MONO         4
#define         MODE_AUDIO_ST           5

#endif				/*  _MC13783_CONNECTIVITY_DEFS_H */
