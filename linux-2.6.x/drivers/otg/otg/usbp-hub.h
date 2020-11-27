/*
 * otg/otg/usbp-hub.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/usbp-hub.h|20051116205002|21527
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
/*!
 * @\addtogroup USBP
 */


/*!
 * @name Hub requests
 */
 /*! @{ */
#define USB_REQ_CLEAR_TT_BUFFER         0x08
#define USB_REQ_RESET_TT                0x09
#define USB_REQ_GET_TT_STATE            0x0A
#define USB_REQ_STOP_TT                 0x0B
/*! @} */

/*!
 * @name Hub Recipients
 */
 /*! @{ */
#define USB_REQ_RECIPIENT_HUB           0x00
#define USB_REQ_RECIPIENT_PORT          0x03
/*! @} */

/*!
 * @name Hub Feature Selectors C.f. Table 11-17
 *
 *              
 */
 /*! @{ */
#if !defined(C_HUB_LOCAL_POWER)
#define C_HUB_LOCAL_POWER  0x0
#endif
#if !defined(C_HUB_OVER_CURRENT)
#define C_HUB_OVER_CURRENT  0x1
#endif

#define PORT_CONNECTION         0               // 0x01
#define PORT_ENABLE             1               // 0x02
#define PORT_SUSPEND            2               // 0x04
#define PORT_OVER_CURRENT       3               // 0x08
#define PORT_RESET              4               // 0x10

#define PORT_POWER              8               // 0x100
#define PORT_LOW_SPEED          9               // 0x200
#define PORT_HIGH_SPEED         10              // 0x400

#define C_PORT_CONNECTION       16
#define C_PORT_ENABLE           17
#define C_PORT_SUSPEND          18
#define C_PORT_OVER_CURRENT     19
#define C_PORT_RESET            20

#define PORT_TEST               21
#define PORT_INDICATOR          22

/*! @} */

#ifdef PRAGMAPACK
#pragma pack(push,1)
#endif



/*!
 * @name Hub Descriptor C.f. 11.23.2.1 Table 11-13
 * N.B. This assumes 1-8 ports, DeviceRemovable and PortPwrCtrlMask
 * must be sized for the number of if ports > 8.
 */

PACKED0 struct PACKED1 hub_descriptor {
 u8 bDescLength;
 u8 bDescriptorType;
 u8 bNbrPorts;
 u16 wHubCharacteristics;
 u8 bPwrOn2PwrGood;
 u8 bHubContrCurrent;
 u8 DeviceRemovable;
 u8 PortPwrCtrlMask;
};

 /*! @{ */


#define HUB_GANGED_POWER  0x00
#define HUB_INDIVIDUAL_POWER  0x01
#define HUB_COMPOUND_DEVICE  0x04
#define HUB_GLOBAL_OVERCURRENT  0x00
#define HUB_INDIVIDUAL_OVERCURRENT 0x08
#define HUB_NO_OVERCURRENT  0x10
#define HUB_TT_8   0x00
#define HUB_TT_16   0x20
#define HUB_TT_24   0x40
#define HUB_TT_32   0x60
#define HUB_INDICATORS_SUPPORTED 0x80
/*! @} */


#ifdef PRAGMAPACK
#pragma pack(pop)
#endif


