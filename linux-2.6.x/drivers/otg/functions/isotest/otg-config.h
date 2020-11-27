/*
 *
 * @(#) balden@seth2.belcarratech.com|otg/functions/isotest/otg-config.h|20051116204957|24651
 *
 *      Copyright (c) 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         dn Initial distribution
 * 10/18/2006         Motorola         ma sk pw Add Open Src Software language
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

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
 * tristate "  ISO Loop Test Function"
 * depends on OTG
 */
#define CONFIG_OTG_ISOTEST

/*
 * hex "VendorID (hex value)"
 * default "0x15ec"
 */
#define CONFIG_OTG_ISOTEST_VENDORID

/*
 * hex "ProductID (hex value)"
 * default "0xf004"
 */
#define CONFIG_OTG_ISOTEST_PRODUCTID

/*
 * hex "bcdDevice (binary-coded decimal)"
 * default "0x0100"
 */
#define CONFIG_OTG_ISOTEST_BCDDEVICE

/*
 * string "iManufacturer (string)"
 * default "Belcarra"
 */
#define CONFIG_OTG_ISOTEST_MANUFACTURER

/*
 * string "iProduct (string)"
 * default "ISO Loop Test Storage Class - Bulk Only"
 */
#define CONFIG_OTG_ISOTEST_PRODUCT_NAME

/*
 * string "ISO Loop Test Bulk Only iInterface (string)"
 * default "ISOTEST BO Data Intf"
 */
#define CONFIG_OTG_ISOTEST_INTF

/*
 * string "Data Interface iConfiguration (string)"
 * default "ISO Loop Test BO Configuration"
 */
#define CONFIG_OTG_ISOTEST_DESC


