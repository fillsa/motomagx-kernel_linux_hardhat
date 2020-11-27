/*
 * @(#) balden@seth2.belcarratech.com|otg/otg-config-std.h|20051116203406|17913
 *
 */
/*
 * Copyright 2005-2006 Motorola, Inc.
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
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 03/08/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 */


/*
 * config OTG_USB_PERIPHERAL
 * bool "USB Peripheral (only)"
 * Compile as a standard USB Peripheral. 
 */
/* #define OTG_USB_PERIPHERAL */

/*
 * config OTG_USB_HOST
 * bool "USB Host (only)"
 * Compile as a standard USB Host. 
 */
/* #define  OTG_USB_HOST */

/*
 * config OTG_USB_PERIPHERAL_OR_HOST
 * bool "USB Peripheral or Host"
 * Compile as a standard USB Peripheral and Host. 
 * The transceiver driver must implement ID_GND to switch between
 * host and peripheral roles.
 */
/* #define OTG_USB_PERIPHERAL_OR_HOST */

/*
 * config OTG_BDEVICE_WITH_SRP
 * bool "SRP Capable B-Device (Only)"
 * Compile as a On-The-Go Peripheral-Only SRP capable device. This
 * is similiar to a Traditional USB Peripheral but enables
 * On-The-Go features such as SRP.
 */
/* #define OTG_BDEVICE_WITH_SRP */

/*
 * config OTG_DEVICE
 * bool "OTG Device - can act as A or B Device"
 * Implement full On-The-Go Device support for a platform that
 * supports implemenation of A and B Device.
 */
/* #define OTG_DEVICE */
        

/*
 * bool 'OTG Fast Tracing'
 * This option implements register trace to support 
 * driver debugging.
 */
/* #define  CONFIG_OTG_TRACE */


        
/*
 * bool 'Built-in Minimal USB Device' 
 * Compile in minimal USB Device only firmware.
 */
/* #define  CONFIG_OTG_FW_MN */

/*
 * bool 'Enable Auto-Start' 
 * Automatically start and enable minimal USB Device.
 */
/* #define  CONFIG_OTG_TR_AUTO */

/*
 * bool 'Disable C99 initializers'
 * If your compiler does not allow a structure to be initialized as .element_name=value 
 */
/* #define  CONFIG_OTG_NOC99 */

/*
 * boolean "USB Host - OTG Support"
 * The most notable feature of USB OTG is support for a
 * "Dual-Role" device, which can act as either a device
 * or a host.  The initial role choice can be changed
 * later, when two dual-role devices talk to each other.
 */
/* #define  CONFIG_USB_OTG */

