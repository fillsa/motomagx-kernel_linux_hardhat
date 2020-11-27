/*
 * otg/functions/network/blan.c - Network Function Driver
 * @(#) balden@seth2.belcarratech.com|otg/functions/network/blan-if.c|20051116204958|54482
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
 * 12/12/2005         Motorola         Initial distribution 
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
 * @file otg/functions/network/blan-if.c
 * @brief This file implements the required descriptors to implement
 * a BLAN network device with a single interface.
 *
 * The BLAN network driver implements the BLAN protocol descriptors.
 *
 * The BLAN protocol is designed to support smart devices that want
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

#ifdef CONFIG_OTG_NETWORK_BLAN
/* USB BLAN  Configuration ******************************************************************** */

/*
 * BLAN Ethernet Configuration
 */

/* Communication Interface Class descriptors
 */
static struct usbd_class_header_function_descriptor blan_class_1 = {
        .bFunctionLength = 0x05, /* Length */
        .bDescriptorType =  USB_DT_CLASS_SPECIFIC, 
        .bDescriptorSubtype = USB_ST_HEADER, 
        .bcdCDC =  __constant_cpu_to_le16(0x0110) /*Version */
};
static struct usbd_class_mdlm_descriptor blan_class_2 = {
        .bFunctionLength = 0x15, 
        .bDescriptorType = USB_DT_CLASS_SPECIFIC, 
        .bDescriptorSubtype = USB_ST_MDLM, 
        .bcdVersion = __constant_cpu_to_le16(0x0100), 
        .bGUID =  {
            0x74, 0xf0, 0x3d, 0xbd, 0x1e, 0xc1, 0x44, 0x70,  /* bGUID */
            0xa3, 0x67, 0x71, 0x34, 0xc9, 0xf5, 0x54, 0x37,  /* bGUID */ },
};
static struct usbd_class_blan_descriptor blan_class_3 = { 
        .bFunctionLength = 0x07, 
        .bDescriptorType =  USB_DT_CLASS_SPECIFIC, 
        .bDescriptorSubtype =  USB_ST_MDLMD, 
        .bGuidDescriptorType =  0x01,
        .bmNetworkCapabilities =  0x00,
        .bmDataCapabilities =  0x00,
        .bPad =  0x00,
};
static struct usbd_class_ethernet_networking_descriptor blan_class_4 = { 
        .bFunctionLength = 0x0d, 
        .bDescriptorType = USB_DT_CLASS_SPECIFIC, 
        .bDescriptorSubtype = USB_ST_ENF, 
        .iMACAddress =  0x00, 
        .bmEthernetStatistics =  0x00, 
        .wMaxSegmentSize =  0x05ea, /* 1514 maximum frame size */
        .wNumberMCFilters =   0x00,
        .bNumberPowerFilters =  0x00 , 
};
static struct usbd_class_network_channel_descriptor blan_class_5 = {
        .bFunctionLength =  0x07,
        .bDescriptorType =  USB_DT_CLASS_SPECIFIC,
        .bDescriptorSubtype = USB_ST_NCT,
        .bEntityId =  0,
        .iName =  0,
        .bChannelIndex =  0,
        .bPhysicalInterface =  0,
};


static struct usbd_generic_class_descriptor *blan_comm_class_descriptors[] = {
        (struct usbd_generic_class_descriptor *) &blan_class_1, 
        (struct usbd_generic_class_descriptor *) &blan_class_2, 
        (struct usbd_generic_class_descriptor *) &blan_class_3, 
        (struct usbd_generic_class_descriptor *) &blan_class_4, 
        (struct usbd_generic_class_descriptor *) &blan_class_5, };

/*! Data Alternate Interface Description List
 */
static struct usbd_alternate_description blan_alternate_descriptions[] = {
        { 
                .iInterface =  "Belcarra USBLAN - MDLM/BLAN",
                .bInterfaceClass = COMMUNICATIONS_INTERFACE_CLASS, 
                .bInterfaceSubClass =  COMMUNICATIONS_MDLM_SUBCLASS, 
                .bInterfaceProtocol = COMMUNICATIONS_NO_PROTOCOL, 
                .classes = sizeof (blan_comm_class_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
                .class_list =  blan_comm_class_descriptors,
                .endpoints = ENDPOINTS,
                .endpoint_index =  net_fd_endpoint_index,
        },
};
/* Interface descriptions and descriptors
 */
/*! Interface Description List
 */
static struct usbd_interface_description blan_interfaces[] = {
        { 
                .alternates =  sizeof (blan_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list =  blan_alternate_descriptions, 
        },
};


/* ********************************************************************************************* */

/*! net_fd_function_enable - enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 */

int blan_fd_function_enable (struct usbd_function_instance *function_instance)
{
        struct usbd_class_network_channel_descriptor *channel = &blan_class_5 ;
#if 0
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,17)
        struct usbd_class_ethernet_networking_descriptor *ethernet = &blan_class_4;
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
        ethernet->iMACAddress = usbd_alloc_string(address_str);
#endif
        struct usbd_class_ethernet_networking_descriptor *ethernet = &blan_class_4;
        char address_str[20];
        sprintf(address_str, "%02x%02x%02x%02x%02x%02x", 0, 0, 0, 0, 0, 0);
        ethernet->iMACAddress = usbd_alloc_string(function_instance, address_str);
        channel->iName = usbd_alloc_string(function_instance, system_utsname.nodename);

        TRACE_MSG3(NTT, "name: %s strings index imac: %d name: %d", system_utsname.nodename, ethernet->iMACAddress, channel->iName);

        return net_fd_function_enable(function_instance, network_blan);
}


/* ********************************************************************************************* */
/*! blan_fd_function_ops - operations table for network function driver
 */
struct usbd_function_operations blan_fd_function_ops = {
        .function_enable = blan_fd_function_enable,
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
struct usbd_interface_driver blan_interface_driver = {
        .driver.name =  "net-blan-if",
        .driver.fops =  &blan_fd_function_ops,
        .interfaces = sizeof (blan_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = blan_interfaces, 
        .endpointsRequested =  ENDPOINTS,
        .requestedEndpoints =  net_fd_endpoint_requests,
        
        .bFunctionClass = USB_CLASS_COMM,
        .bFunctionSubClass = COMMUNICATIONS_ENCM_SUBCLASS,
        .bFunctionProtocol = COMMUNICATIONS_NO_PROTOCOL,
        .iFunction = "Belcarra USBLAN - MDLM/BLAN",
};

/* ********************************************************************************************* */

/*! blan_mod_init
 * @param function The function instance
 */
int blan_mod_init (void)
{
        struct usbd_class_ethernet_networking_descriptor *ethernet;
        struct usbd_class_network_channel_descriptor *channel;

        int len = 0;
        char buf[255];

        buf[0] = 0;

        //blan_alternate_descriptions[0].endpoints = Usb_network_private.have_interrupt ? 3 : 2;

#if 0
        // Update the iMACAddress field in the ethernet descriptor
        {
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
                TRACE_MSG0(NTT,"alloc mac string");
                //if ((ethernet = &blan_class_4)) 
                //        ethernet->iMACAddress = usbd_alloc_string(function_instance, address_str);
                TRACE_MSG0(NTT,"alloc mac string done");
        }
        TRACE_MSG0(NTT,"alloc channel string");
        //if ((channel = &blan_class_5)) 
        //        channel->iName = usbd_alloc_string(function_instance, system_utsname.nodename);
        TRACE_MSG0(NTT,"alloc channel string done");
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_PADBYTES
        blan_class_3.bPad = CONFIG_OTG_NETWORK_BLAN_PADBYTES;
        len += sprintf(buf + len, "PADBYTES: %02x ", blan_class_3.bPad);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_PADBEFORE
        blan_class_3.bmDataCapabilities |= BMDATA_PADBEFORE;
        len += sprintf(buf + len, "PADBEFORE: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_PADAFTER
        blan_class_3.bmDataCapabilities |= BMDATA_PADAFTER;
        len += sprintf(buf + len, "PADAFTER: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_CRC
        blan_class_3.bmDataCapabilities |= BMDATA_CRC;
        len += sprintf(buf + len, "CRC: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_FERMAT
        blan_class_3.bmDataCapabilities |= BMDATA_FERMAT;
        len += sprintf(buf + len, "FERMAT: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_HOSTNAME
        blan_class_3.bmDataCapabilities |= BMDATA_HOSTNAME;
        len += sprintf(buf + len, "HOSTNAME: %02x ", blan_class_3.bmDataCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_NONBRIDGED
        blan_class_3.bmNetworkCapabilities |= BMNETWORK_NONBRIDGED;
        len += sprintf(buf + len, "NONBRIDGE: %02x ", blan_class_3.bmNetworkCapabilities);
#endif
#ifdef CONFIG_OTG_NETWORK_BLAN_DATA_NOTIFY_OK
        blan_class_3.bmNetworkCapabilities |= BMNETWORK_DATA_NOTIFY_OK;
        len += sprintf(buf + len, "DATA NOTIFY: %02x ", blan_class_3.bmNetworkCapabilities);
#endif
        if (strlen(buf))
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, buf);

        return usbd_register_interface_function (&blan_interface_driver, "net-blan-if", NULL);
}

/*!
 * @brief blan_mod_exit
 * @return none 
 */
void blan_mod_exit(void)
{
        usbd_deregister_interface_function (&blan_interface_driver);
}

#else                  /* CONFIG_OTG_NETWORK_BLAN */
/*!
 * @brief  blan_mod_init
 * @return int
 */
int blan_mod_init (void)
{
        return 0;
}
/*!
 * @brief blan_mod_exit
 * @return none
 */
void blan_mod_exit(void)
{
}
#endif                  /* CONFIG_OTG_NETWORK_BLAN */
