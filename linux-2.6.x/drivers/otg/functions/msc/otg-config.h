/*
 *
 * @(#) balden@seth2.belcarratech.com|otg/functions/msc/otg-config.h|20051116204958|37128
 *
 * Copyright (c) 2005-2007 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 07/17/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 03/07/2007         Motorola         Added 32K transfer rate option.
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
 * tristate "  Mass Storage Function"
 */
/* #define CONFIG_OTG_MSC */

/*
 * hex "VendorID (hex value)"
 * default "0x15ec"
 */
#define CONFIG_OTG_MSC_VENDORID 0x15ec

/*
 * hex "ProductID (hex value)"
 * default "0xf006"
 */
#define CONFIG_OTG_MSC_PRODUCTID 0xf006

/*
 * hex "bcdDevice (binary-coded decimal)"
 * default "0x0100"
 */
#define CONFIG_OTG_MSC_BCDDEVICE 0x0100

/*
 * string "iManufacturer (string)"
 * default "Motorola"
 */
#define CONFIG_OTG_MSC_MANUFACTURER "Motorola"

/*
 * string "iProduct (string)"
 * default "Mass Storage Class - Bulk Only"
 */
#define CONFIG_OTG_MSC_PRODUCT_NAME "Mass Storage Class - Bulk Only"

/*
 * string "MSC Bulk Only iInterface (string)"
 * default "MSC BO Data Intf"
 */
#define CONFIG_OTG_MSC_INTF "MSC BO Data Intf"

/*
 * string "Data Interface iConfiguration (string)"
 * default "MSC BO Configuration"
 */
#define CONFIG_OTG_MSC_DESC "MSC BO Configuratino"

/*
 * bool "  MSC Tracing"
 * default n
 */
#define CONFIG_OTG_MSC_REGISTER_TRACE
/*
 * hex "MSC Number of pages per write"
 * default "0x01"
 */
#define CONFIG_OTG_MSC_NUM_PAGES
