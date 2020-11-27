/*
 * otg/otg/usbp-hid.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/usbp-hid.h|20051116205002|19657
 *
 *      Copyright (c) 2004-2005 Belcarra
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef PRAGMAPACK
#pragma pack(push,1)
#endif
 
/*!
 * @file otg/otg/usbp-hid.h
 * @brief HID class descriptor structure definitions.
 *
 * @ingroup USBP
 */

/*!
 * @name HID requests
 */
 /*! @{ */
#define USB_REQ_GET_REPORT              0x01
#define USB_REQ_GET_IDLE                0x02
#define USB_REQ_GET_PROTOCOL            0x03
#define USB_REQ_SET_REPORT              0x09
#define USB_REQ_SET_IDLE                0x0A
#define USB_REQ_SET_PROTOCOL            0x0B
/*! @} */


/*!
 * @name HID - Class Descriptors
 * C.f. 7.1.1
 */
 /*! @{ */

#define HID             0x21
#define HID_REPORT      0x22
#define HID_PHYSICAL    0x23

/*! @} */

/*!
 * @name HID - Report Types
 * C.f. 7.1.1
 */
 /*! @{ */

#define HID_INPUT       0x01
#define HID_OUTPUT      0x02
#define HID_FEATURE     0x03

/*! @} */

/*!
 * @name HID Descriptor
 * C.f. E.8
 */
 /*! @{ */


PACKED0 struct PACKED1 hid_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u16 bcdHID;
        u8 bCountryCode;
        u8 bNumDescriptors;
        u8 bReportType;
        u16 wItemLength;
};

/*! @} */


#ifdef PRAGMAPACK
#pragma pack(pop)
#endif


