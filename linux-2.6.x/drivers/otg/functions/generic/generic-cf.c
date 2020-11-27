/*
 * otg/functions/generic/belcarra-cf.c
 * @(#) sl@whiskey.enposte.net|otg/functions/generic/generic-cf.c|20051116230728|44098
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2006-2008 Motorola, Inc. 
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 04/26/2006         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 02/26/2007         Motorola         Changes for PTP.
 * 01/09/2007         Motorola         Changes for PTP(PictSync) 
 * 08/06/2008         Motorola         MaxPower =0xfa for PTP mode
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
/*!
 * @file otg/functions/generic/generic-cf.c
 * @brief Generic Configuration Function Driver 
 *
 * This implements a generic composite function
 *
 *         - idVendor              16 bit word
 *         - idProduct             16 bit word
 *         - bcd_device            16 bit word
 *
 *         - bDeviceClass          byte
 *         - device_sub-class      byte
 *         - bcdDevice             byte
 *
 *         - vendor                string
 *         - iManufacturer         string
 *         - product               string
 *         - serial                string
 * 
 *         - power                 byte
 *         - remote_wakeup         bit
 * 
 *         - functions             string
 * 
 * 
 * The functions string would contain a list of names, the first would
 * be the composite function driver, the rest would be the interface
 * functions. For example:
 * 
 *         
 *         "cmouse-cf mouse-if"
 *         "mcpc-cf dun-if dial-if obex-if dlog-if"
 *
 * There are also a set of pre-defined configurations that
 * can be loaded singly or in toto.
 *
 * @ingroup GenericFunction
 */

#include <linux/config.h>
#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-cdc.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-utils.h>

#include "generic.h"

/* Setup the pre-defined configuration name from configuration
 * option if available.
 *
 * N.B. This list needs to be synchronized with both pre-defined
 * configurations (see below) and the Kconfig list
 */
#ifdef CONFIG_OTG_GENERIC_CONFIG_MOUSE
static char default_predefined_configuration[] = "mouse";
#elif CONFIG_OTG_GENERIC_CONFIG_NET_BLAN
static char default_predefined_configuration[] = "net-blan";
#elif CONFIG_OTG_GENERIC_CONFIG_NET_CDC
static char default_predefined_configuration[] = "net-cdc";
#elif CONFIG_OTG_GENERIC_CONFIG_NET_SAFE
static char default_predefined_configuration[] = "net-safe";
#elif CONFIG_OTG_GENERIC_CONFIG_ACM_TTY
static char default_predefined_configuration[] = "acm-tty";
#elif CONFIG_OTG_GENERIC_CONFIG_MSC
static char default_predefined_configuration[] = "msc";
#elif CONFIG_OTG_GENERIC_CONFIG_MTP
static char default_predefined_configuration[] = "mtp";
#elif CONFIG_OTG_GENERIC_CONFIG_PBG
static char default_predefined_configuration[] = "pbg";
#elif CONFIG_OTG_GENERIC_CONFIG_PTP
static char default_predefined_configuration[] = "ptp";

#elif CONFIG_OTG_GENERIC_CONFIG_MOUSE2
static char default_predefined_configuration[] = "mouse2";
#elif CONFIG_OTG_GENERIC_CONFIG_MSC_MOUSE
static char default_predefined_configuration[] = "msc-mouse";
#elif CONFIG_OTG_GENERIC_CONFIG_MOUSE_BLAN
static char default_predefined_configuration[] = "mouse-blan";
#elif CONFIG_OTG_GENERIC_CONFIG_MSC_BLAN
static char default_predefined_configuration[] = "msc-blan";
#elif CONFIG_OTG_GENERIC_CONFIG_MSC_CDC
static char default_predefined_configuration[] = "msc-cdc";

#else
char default_predefined_configuration[] = CONFIG_OTG_GENERIC_CONFIG_NAME;
#endif


/* Pre-defined configurations *********************************************** */

struct generic_config generic_configs[] = {
        #if defined(CONFIG_OTG_ACM) ||  defined(CONFIG_OTG_ACM_MODULE)
        {
        .composite_driver.driver.name = "acm-tty",
        .interface_names = "tty-if",
        .configuration_description.iConfiguration = CONFIG_OTG_ACM_DESC,
        .device_description.bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
        .device_description.bDeviceSubClass = 2,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_ACM_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_ACM_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_ACM_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_ACM_PRODUCT_NAME,
        },
        #endif /* defined(CONFIG_OTG_ACM) */
        #if defined(CONFIG_OTG_NETWORK_BLAN) 
        {
        .composite_driver.driver.name = "net-blan",
        .interface_names = "net-blan-if",
        .configuration_description.iConfiguration = "net-blan",
        .device_description.bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
        .device_description.bDeviceSubClass = 2,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_NETWORK_PRODUCT_NAME,
        },
        #endif /* defined(CONFIG_OTG_NETWORK_BLAN) */
        #if defined(CONFIG_OTG_NETWORK_CDC) 
        {
        .composite_driver.driver.name = "net-cdc",
        .interface_names = "net-cdc-if",
        .configuration_description.iConfiguration = "net-cdc",
        .device_description.bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
        .device_description.bDeviceSubClass = 2,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_NETWORK_PRODUCT_NAME,
        },
        #endif /* defined(CONFIG_OTG_NETWORK_CDC) */
        #if defined(CONFIG_OTG_NETWORK_SAFE) 
        {
        .composite_driver.driver.name = "net-safe",
        .interface_names = "net-safe-if",
        .configuration_description.iConfiguration = "net-safe",
        .device_description.bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
        .device_description.bDeviceSubClass = 2,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_NETWORK_PRODUCT_NAME,
        },
        #endif /* defined(CONFIG_OTG_NETWORK_SAFE) */
        #if defined(CONFIG_OTG_NETWORK_BASIC) 
        {
        .composite_driver.driver.name = "net-basic",
        .interface_names = "net-basic-if",
        .configuration_description.iConfiguration = "net-basic",
        .device_description.bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_NETWORK_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_NETWORK_PRODUCT_NAME,
        },
        #endif /* defined(CONFIG_OTG_NETWORK_BASIC) */
        #if defined(CONFIG_OTG_MOUSE) || defined(CONFIG_OTG_MOUSE_MODULE)
        {
        .composite_driver.driver.name = "mouse",
        .interface_names = "mouse-if",
        .configuration_description.iConfiguration = "mouse",
        .device_description.bDeviceClass = 0,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MOUSE_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_MOUSE_PRODUCT_NAME,
        },
        #endif /* defined(CONFIG_OTG_MOUSE) */
        #if defined(CONFIG_OTG_MSC) || defined(CONFIG_OTG_MSC_MODULE) 
        {
        .composite_driver.driver.name = "msc",
        .interface_names = "msc-if",
        .configuration_description.iConfiguration = "msc",
        .device_description.bDeviceClass = 0,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_MSC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_MSC_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MSC_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_MSC_PRODUCT_NAME,
        },
        #endif /* defined(CONFIG_OTG_MSC)  */
        #if defined(CONFIG_OTG_MTP) || defined(CONFIG_OTG_MTP_MODULE) 
        {
        .composite_driver.driver.name = "mtp",
        .interface_names = "mtp-if",
        .configuration_description.iConfiguration = "mtp",
        .device_description.bDeviceClass = 0,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_MTP_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_MTP_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_MTP_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_MTP_PRODUCT_NAME,
        .configuration_description.bMaxPower = 0xfa, /* 500mA */
        },
        #endif /* defined(CONFIG_OTG_MTP)  */
        #if defined(CONFIG_OTG_PBG) || defined(CONFIG_OTG_PBG_MODULE) 
        {
        .composite_driver.driver.name = "pbg",
        .interface_names = "pbg-if",
        .configuration_description.iConfiguration = "pbg",
        .device_description.bDeviceClass = 0,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_PBG_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_PBG_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_PBG_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_PBG_PRODUCT_NAME,
        .configuration_description.bMaxPower = 0x32, /* 100mA */
        },
        #endif /* defined(CONFIG_OTG_PBG)  */
		#if defined(CONFIG_OTG_PTP) || defined(CONFIG_OTG_PTP_MODULE) 
        {
        .composite_driver.driver.name = "ptp",
        .interface_names = "pbg-if",
        .configuration_description.iConfiguration = "Motorola PTP Configuration",
        .device_description.bDeviceClass = 0,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_PBG_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_PBG_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_PBG_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_PBG_PRODUCT_NAME,
        .configuration_description.bMaxPower = 0xfa, /* 500mA */
        },
        #endif /* defined(CONFIG_OTG_PTP)  */

        #if defined(CONFIG_OTG_MOUSE) || defined(CONFIG_OTG_MOUSE_MODULE)
        {
        .composite_driver.driver.name = "mouse2",
        .interface_names = "mouse-if:mouse-if",
        .configuration_description.iConfiguration = "mouse2",
        .device_description.bDeviceClass = 0,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc001),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "Dual Mouse Demo",
        },
        {
        .composite_driver.driver.name = "mouse2-iad",
        .interface_names = "mouse-if:mouse-if",
        .configuration_description.iConfiguration = "mouse2-iad",
        .device_description.bDeviceClass = USB_CLASS_MISC,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc101),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "Dual Mouse Demo",
        },
        #endif /* defined(CONFIG_OTG_MOUSE) */



        #if (defined(CONFIG_OTG_MSC_MODULE) || defined(CONFIG_OTG_MSC)) && \
                (defined(CONFIG_OTG_MOUSE_MODULE) || defined(CONFIG_OTG_MOUSE))
        {
        .composite_driver.driver.name = "msc-mouse",
        .interface_names = "msc-if:mouse-if",
        .configuration_description.iConfiguration = "msc-mouse",
        .device_description.bDeviceClass = 0,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc002),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "MSC-Mouse Demo",
        },
        {
        .composite_driver.driver.name = "msc-mouse-iad",
        .interface_names = "msc-if:mouse-if",
        .configuration_description.iConfiguration = "msc-mouse-iad",
        .device_description.bDeviceClass = USB_CLASS_MISC,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc102),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "MSC-Mouse Demo",
        },
        #endif /* defined(CONFIG_OTG_MSC) && defined(CONFIG_OTG_MOUSE)) */



        #if (defined(CONFIG_OTG_MOUSE_MODULE) || defined(CONFIG_OTG_MOUSE)) && defined(CONFIG_OTG_NETWORK_BLAN)
        {
        .composite_driver.driver.name = "mouse-blan",
        .interface_names = "mouse-if:net-blan-if",
        .configuration_description.iConfiguration = "mouse-blan",
        .device_description.bDeviceClass = 0,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc003),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "MOUSE-BLAN Demo",
        },
        {
        .composite_driver.driver.name = "mouse-blan-iad",
        .interface_names = "mouse-if:net-blan-if",
        .configuration_description.iConfiguration = "mouse-blan-iad",
        .device_description.bDeviceClass = USB_CLASS_MISC,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc103),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "MOUSE-BLAN Demo",
        },
        #endif /* defined(CONFIG_OTG_MOUSE) && defined(CONFIG_OTG_NETWORK_BLAN) */

       /*
        #if (defined(CONFIG_OTG_MSC_MODULE) || defined(CONFIG_OTG_MSC))  && defined(CONFIG_OTG_NETWORK_BLAN)
        {
        .composite_driver.driver.name = "msc-blan",
        .interface_names = "msc-if:net-blan-if",
        .configuration_description.iConfiguration = "msc-blan",
        .device_description.bDeviceClass = 0,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc004),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "MSC-BLAN Demo",
        },
        {
        .composite_driver.driver.name = "msc-blan-iad",
        .interface_names = "msc-if:net-blan-if",
        .configuration_description.iConfiguration = "msc-blan-iad",
        .device_description.bDeviceClass = USB_CLASS_MISC,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc104),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "MSC-BLAN Demo",
        },
        #endif  defined(CONFIG_OTG_MSC) && defined(CONFIG_OTG_NETWORK_BLAN) */


        #if (defined(CONFIG_OTG_MSC_MODULE) || defined(CONFIG_OTG_MSC)) && defined(CONFIG_OTG_NETWORK_CDC)
        {
        .composite_driver.driver.name = "msc-cdc",
        .interface_names = "msc-if:net-cdc-if",
        .configuration_description.iConfiguration = "msc-cdc",
        .device_description.bDeviceClass = 0xff,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc005),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "MSC-CDC Demo",
        },
        {
        .composite_driver.driver.name = "msc-cdc-iad",
        .interface_names = "msc-if:net-cdc-if",
        .configuration_description.iConfiguration = "msc-cdc-iad",
        .device_description.bDeviceClass = USB_CLASS_MISC,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc105),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "MSC-CDC Demo",
        },
        #endif /* defined(CONFIG_OTG_MSC) && defined(CONFIG_OTG_NETWORK_BLAN) */


        #if (defined(CONFIG_OTG_ACM_MODULE) || defined(CONFIG_OTG_ACM)) && defined(CONFIG_OTG_NETWORK_BLAN)
        {
        .composite_driver.driver.name = "tty-blan-motorola",
        .interface_names = "tty-if:net-blan-if",
        .configuration_description.iConfiguration = CONFIG_OTG_ACM_NETWORK_BLAN_DESC,
        .device_description.bDeviceClass = 0xff,
        .device_description.bDeviceSubClass = 0xff,
        .device_description.bDeviceProtocol = 0xff,   
        // .device_description.bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
        // .device_description.bDeviceSubClass = 0x00,
        // .device_description.bDeviceProtocol = 0x00,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_ACM_NETWORK_PRODUCTID),
        // .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "Motorola TTY-MDLM/BLAN Network Composite Demo (Class = 0xff)",
        },
        #endif
		
        /*
        {
        .composite_driver.driver.name = "acm-mcci",
        .interface_names = "tty-if",
        .configuration_description.iConfiguration = "acm-mcci",
        .device_description.bDeviceClass = COMMUNICATIONS_DEVICE_CLASS,
        .device_description.bDeviceSubClass = 0x00,
        .device_description.bDeviceProtocol = 0x00,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0x6430),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "Motorola ACM MCCI ",
        },
        {
        .composite_driver.driver.name = "tty-blan-iad",
        .interface_names = "tty-if:net-blan-if",
        .configuration_description.iConfiguration = "tty-blan-iad",
        .device_description.bDeviceClass = USB_CLASS_MISC,
        .device_description.bDeviceSubClass = 0,
        .device_description.bDeviceProtocol = 0,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
    	.device_description.idProduct = __constant_cpu_to_le16(0xc106),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),        
	.device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "Belcarra TTY-MDLM/BLAN Network Composite Demo (IAD)",
        },
        {
        .composite_driver.driver.name = "blan-tty",
        .interface_names = "net-blan-if:tty-if",
        .configuration_description.iConfiguration = "blan-tty",
        .device_description.bDeviceClass = 0x00,
        .device_description.bDeviceSubClass = 0x00,
        .device_description.bDeviceProtocol = 0x00,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc007),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "Belcarra MDLM/BLAN-TTY Network Composite Demo (Class = 0x00)",
        },
        {
        .composite_driver.driver.name = "blan-tty-vendor",
        .interface_names = "net-blan-if:tty-if",
        .configuration_description.iConfiguration = "blan-tty",
        .device_description.bDeviceClass = 0xff,
        .device_description.bDeviceSubClass = 0xff,
        .device_description.bDeviceProtocol = 0xff,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc007),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "Belcarra MDLM/BLAN-TTY Network Composite Demo (Class = 0xff)",
        },
        {
        .composite_driver.driver.name = "blan-tty-iad",
        .interface_names = "net-blan-if:tty-if",
        .configuration_description.iConfiguration = "blan-tty-iad",
        .device_description.bDeviceClass = USB_CLASS_MISC,
        .device_description.bDeviceSubClass = 0xff,
        .device_description.bDeviceProtocol = 0xff,
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(0xc107),
        .device_description.bcdDevice = __constant_cpu_to_le16(0x0001),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = "Belcarra MDLM/BLAN-TTY Network Composite Demo (IAD)",
        },
        #endif  defined(CONFIG_OTG_ACM) */

        {  },
};
