/*
 *
 * @(#) balden@seth2.belcarratech.com|otg/functions/generic/otg-config.h|20051116204957|11013
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *
 *      Copyright (c) 2006-2007 Motorola, Inc. 
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 04/26/2006         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 02/26/2007         Motorola         Add PTP config define
 * 01/09/2007         Motorola         Added PTP config define(PictSync) 
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA
 *
 */

/*
 * tristate "  Generic Function"
 * This acts as a generic function, allowing selection of class
 * and interface function drivers to create a specific configuration.
 */
#define CONFIG_OTG_GENERIC

/*
 * hex "VendorID (hex value)"
 * default "0x15ec"
 */
#define CONFIG_OTG_GENERIC_VENDORID

/*
 * hex "ProductID (hex value)"
 * default "0xf010"
 */
#define CONFIG_OTG_GENERIC_PRODUCTID

/*
 * hex "bcdDevice (binary-coded decimal)"
 * default "0x0100"
 */
#define CONFIG_OTG_GENERIC_BCDDEVICE

/*
 * string "iManufacturer (string)"
 * default "Belcarra"
 */
#define CONFIG_OTG_GENERIC_MANUFACTURER

/*
 * string "iProduct (string)"
 * default "Generic Composite"
 */
#define CONFIG_OTG_GENERIC_PRODUCT_NAME

/*
 * bool 'none'
 * No pre-defined selection, use the OTG_GENERIC_CONFIG_NAME instead.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NONE

/*
 * bool 'mouse'
 * Configure the mouse driver in a single function non-composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MOUSE

/*
 * bool 'net-blan'
 * Configure the network driver in a single function non-composite BLAN configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NET_BLAN

/*
 * bool 'net-cdc'
 * Configure the network driver in a single function non-composite CDC configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NET_CDC

/* 
 * bool 'net-safe'
 * Configure the network driver in a single function non-composite SAFE configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NET_SAFE

/* 
 * bool 'acm-tty'
 * Configure the acm driver in a single function non-composite TTY configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_ACM_TTY

/*
 * bool 'msc'
 * Configure the msc driver in a single function non-composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MSC

/*
 * bool 'mtp'
 * Configure the mtp driver in a single function non-composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MTP

/*
 * bool 'pbg'
 * Configure the pbg driver in a single function non-composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_PBG

/*
 * bool 'ptp'
 * Configure the ptp driver in a single function non-composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_PTP

/*
 * bool 'mouse2'
 * Configure the mouse driver in a demostration, two function composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MOUSE2

/*
 * bool 'msc-mouse'
 * Configure the msc and mouse driver in a demostration, two function composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MSC_MOUSE

/*
 * bool 'msc-blan'
 * Configure the msc and network-blan driver in a demostration, two function composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MSC_BLAN

/*
 * bool 'msc-cdc'
 * Configure the msc and network-cdc driver in a demostration, two function composite configuration.
 */
#define CONFIG_OTG_GENERIC_CONFIG_MSC_CDC




/*
 * string "Composite Configuration (string)"
 * default ""
 * Name of predefined configuration to be enabled, note that
 * if this is not defined (empty string) then the first available
 * configuration will be used (assuming that the required interface
 * function drivers are available.
 */
#define CONFIG_OTG_GENERIC_CONFIG_NAME

