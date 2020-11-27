/*
 * otg/functions/network/safe.c - Network Function Driver
 * @(#) balden@seth2.belcarratech.com|otg/functions/network/safe-if.c|20051116204958|01186
 *
 *      Copyright (c) 2002-2005, 2004 Belcarra
 *
 * By: 
 *      Chris Lynne <cl@belcarra.com>
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 06/08/2005         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 *                                                                                                          
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
 * @file otg/functions/network/safe-if.c
 * @brief This file implements the required descriptors to implement
 * a SAFE network device with a single interface.
 *
 * The SAFE network driver implements the SAFE protocol descriptors.
 *
 * The SAFE protocol is designed to support smart devices that want
 * to create a virtual network between them host and other similiar
 * devices.
 *
 * @ingroup NetworkFunction
 */


#include <otg/otg-compat.h>
#include <otg/otg-module.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/utsname.h>
#include <linux/netdevice.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>

#include "network.h"

#define NTT network_fd_trace_tag

#ifdef CONFIG_OTG_NETWORK_SAFE
/* USB SAFE  Configuration ******************************************************************** */

/*
 * Safe Ethernet Configuration
 */

static  struct usbd_class_header_function_descriptor safe_class_1 = { 
     bFunctionLength:0x05, 
     bDescriptorType: USB_DT_CLASS_SPECIFIC, 
     bDescriptorSubtype: USB_ST_HEADER, 
     bcdCDC: __constant_cpu_to_le16(0x0110), //0x10, 0x01, 
};

static struct usbd_class_mdlm_descriptor safe_class_2 = { 
     bFunctionLength: 0x15, 
     bDescriptorType: USB_DT_CLASS_SPECIFIC, 
     bDescriptorSubtype: USB_ST_MDLM, 
     bcdVersion: __constant_cpu_to_le16( 0x0100), //0x00, 0x01, 
     bGUID:  {
                0x5d, 0x34, 0xcf, 0x66, 0x11, 0x18, 0x11, 0xd6,  /* bGUID */
                0xa2, 0x1a, 0x00, 0x01, 0x02, 0xca, 0x9a, 0x7f,  /* bGUID */ },
};

static struct usbd_class_safe_descriptor safe_class_3 = { 
     bFunctionLength: 0x06, 
     bDescriptorType: USB_DT_CLASS_SPECIFIC, 
     bDescriptorSubtype: USB_ST_MDLMD, 
     bGuidDescriptorType: 0x00, 
     bmNetworkCapabilities: 0x00, 
     bmDataCapabilities: 0x00,    /* bDetailData */ 
};

static struct usbd_class_ethernet_networking_descriptor safe_class_4 = { 
     bFunctionLength: 0x0d, 
     bDescriptorType: USB_DT_CLASS_SPECIFIC, 
     bDescriptorSubtype: USB_ST_ENF,
     iMACAddress: 0x0,
     bmEthernetStatistics: __constant_cpu_to_le32(0x00), 
     wMaxSegmentSize: __constant_cpu_to_le16(0x5ea),
     wNumberMCFilters: __constant_cpu_to_le16(0x00),
     bNumberPowerFilters: 0,
};


static struct usbd_generic_class_descriptor  *safe_comm_class_descriptors[] = { 
        (struct usbd_generic_class_descriptor *) &safe_class_1, 
        (struct usbd_generic_class_descriptor *) &safe_class_2, 
        (struct usbd_generic_class_descriptor *) &safe_class_3, 
        (struct usbd_generic_class_descriptor *) &safe_class_4, };


/*! Data Alternate Interface Description List
 */
static struct usbd_alternate_description safe_alternate_descriptions[] = {
        { 
                .iInterface = "Belcarra USBLAN - MDLM/SAFE",
                .bInterfaceClass = COMMUNICATIONS_INTERFACE_CLASS, 
                .bInterfaceSubClass =  COMMUNICATIONS_MDLM_SUBCLASS, 
                .bInterfaceProtocol = COMMUNICATIONS_NO_PROTOCOL, 
                .classes = sizeof (safe_comm_class_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
                .class_list =  safe_comm_class_descriptors,
                .endpoints = ENDPOINTS,
                .endpoint_index =  net_fd_endpoint_index,
        },
};
/* Interface descriptions and descriptors
 */
/*! Interface Description List
 */
static struct usbd_interface_description safe_interfaces[] = {
        { 
                .alternates =  sizeof (safe_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list =  safe_alternate_descriptions, 
        },
};


/* ********************************************************************************************* */

/*! net_fd_function_enable - enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 */

int safe_fd_function_enable (struct usbd_function_instance *function_instance)
{
        struct usbd_class_ethernet_networking_descriptor *ethernet = &safe_class_4;
        //struct usbd_class_network_channel_descriptor *channel = &safe_class_5 ;

#if 0
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,17)
        char address_str[14];
        snprintf(address_str, 13, "%02x%02x%02x%02x%02x%02x",
                        local_dev_addr[0], local_dev_addr[1], local_dev_addr[2], 
                        local_dev_addr[3], local_dev_addr[4], local_dev_addr[5]);
#else
        char address_str[20];
        sprintf(address_str, "%02x%02x%02x%02x%02x%02x",
                        local_dev_addr[0], local_dev_addr[1], local_dev_addr[2], 
                        local_dev_addr[3], local_dev_addr[4], local_dev_addr[5]);
#endif
#endif
        //ethernet->iMACAddress = usbd_alloc_string(address_str);
        //channel->iName = usbd_alloc_string(system_utsname.nodename);

        return net_fd_function_enable(function_instance, network_safe);
}


/* ********************************************************************************************* */
/*! safe_fd_function_ops - operations table for network function driver
 */
struct usbd_function_operations safe_fd_function_ops = {
        .function_enable = safe_fd_function_enable,
        .function_disable = net_fd_function_disable,

        .device_request = net_fd_device_request,
        .endpoint_cleared = net_fd_endpoint_cleared,

        .set_configuration = net_fd_set_configuration,
        .set_interface = net_fd_set_interface,
        .reset = net_fd_reset,
        .suspended = net_fd_suspended,
        .resumed = net_fd_resumed,
};

/*! function driver description
 */
struct usbd_interface_driver safe_interface_driver = {
        .driver.name =  "net-safe-if",
        .driver.fops =  &safe_fd_function_ops,
        .interfaces = sizeof (safe_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = safe_interfaces, 
        .endpointsRequested =  ENDPOINTS,
        .requestedEndpoints =  net_fd_endpoint_requests,
        
        .bFunctionClass = USB_CLASS_COMM,
        .bFunctionSubClass = COMMUNICATIONS_ENCM_SUBCLASS,
        .bFunctionProtocol = COMMUNICATIONS_NO_PROTOCOL,
        .iFunction = "Belcarra USBLAN - MDLM/SAFE",
};

/* ********************************************************************************************* */

/*! safe_mod_init
 * @param function The function instance
 */
int safe_mod_init (void)
{
        struct usbd_class_ethernet_networking_descriptor *ethernet;
        struct usbd_class_network_channel_descriptor *channel;

        int len = 0;
        char buf[255];

        buf[0] = 0;

#if 0
        // Update the iMACAddress field in the ethernet descriptor
        {
                char address_str[14];
                snprintf(address_str, 13, "%02x%02x%02x%02x%02x%02x",
                                remote_dev_addr[0], remote_dev_addr[1], remote_dev_addr[2], 
                                remote_dev_addr[3], remote_dev_addr[4], remote_dev_addr[5]);
                if ((ethernet = &safe_class_4)) {
                        if (ethernet->iMACAddress) {
                                usbd_free_string_descriptor(ethernet->iMACAddress);
                        }
                        ethernet->iMACAddress = usbd_alloc_string(function_instance, address_str);
                }
        }
#endif

#ifdef CONFIG_OTG_NETWORK_SAFE_PADBEFORE
        safe_class_3.bmDataCapabilities |= BMDATA_PADBEFORE;
        len += sprintf(buf + len, "PADBEFORE: %02x ", safe_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_SAFE_CRC
        safe_class_3.bmDataCapabilities |= BMDATA_CRC;
        len += sprintf(buf + len, "CRC: %02x ", safe_class_3.bmDataCapabilities);
#endif
#ifndef CONFIG_OTG_NETWORK_SAFE_BRIDGED
        safe_class_3.bmNetworkCapabilities |= BMNETWORK_NONBRIDGED;
        len += sprintf(buf + len, "NONBRIDGE: %02x ", safe_class_3.bmNetworkCapabilities);
#endif
        if (strlen(buf))
                TRACE_MSG1(NTT,"%s", buf);

        return usbd_register_interface_function (&safe_interface_driver, "net-safe-if", NULL);
}

/*! safe_mod_exit
 */
void safe_mod_exit(void)
{
        usbd_deregister_interface_function (&safe_interface_driver);
}

#else                  /* CONFIG_OTG_NETWORK_SAFE */
/*!
 * @brief safe_mod_init
 * @return none
 */
int safe_mod_init (void)
{
	return 0;
}
/*! safe_mod_exit
 */
void safe_mod_exit(void)
{
}
#endif                  /* CONFIG_OTG_NETWORK_SAFE */
