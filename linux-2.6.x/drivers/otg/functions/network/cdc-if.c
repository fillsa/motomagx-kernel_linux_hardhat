/*
 * otg/functions/network/cdc.c - Network Function Driver
 * @(#) balden@seth2.belcarratech.com|otg/functions/network/cdc-if.c|20051116204958|55843
 *
 *      Copyright (c) 2002-2003, 2004 Belcarra
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
 * @file otg/functions/network/cdc-if.c
 * @brief This file implements the required descriptors to implement
 * a basic network device with two interfaces.
 *
 * The CDC network driver implements the CDC Network Configuration.  Two
 * interfaces with two BULK data endpoints and an INTERRUPT endpoint.
 *
 * @ingroup NetworkFunction
 */


#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/utsname.h>
#include <linux/netdevice.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>

#include "network.h"

#define NTT network_fd_trace_tag

#ifdef CONFIG_OTG_NETWORK_CDC
/* USB CDC Configuration ******************************************************************** */


static struct usbd_class_header_function_descriptor cdc_class_1 = {
    bFunctionLength:	0x05,
    bDescriptorType:    USB_DT_CLASS_SPECIFIC,
    bDescriptorSubtype: USB_ST_HEADER,
    bcdCDC:             __constant_cpu_to_le16(0x0110),
};


static struct usbd_class_ethernet_networking_descriptor cdc_class_2 = {
    bFunctionLength:    0x0D,
    bDescriptorType:    USB_DT_CLASS_SPECIFIC,
    bDescriptorSubtype: USB_ST_ENF,
    iMACAddress:        0x00,
    bmEthernetStatistics: 0x00,
    wMaxSegmentSize:    __constant_cpu_to_le16(0x05ea),
    wNumberMCFilters:   0,
    bNumberPowerFilters: 0,
};

static struct usbd_class_union_function_descriptor cdc_class_3 = 
{
     bFunctionLength:    0x05,
     bDescriptorType:    USB_DT_CLASS_SPECIFIC,
     bDescriptorSubtype: USB_ST_UF,
     bMasterInterface:   0x00,
     bSlaveInterface:	 { 1 },                         // This will be updated 
      
};

static usbd_class_descriptor_t *cdc_comm_class_descriptors[] = { 
        (struct usbd_generic_class_descriptor *) &cdc_class_1, 
        (struct usbd_generic_class_descriptor *) &cdc_class_2, 
        (struct usbd_generic_class_descriptor *) &cdc_class_3, 
};



/*! Data Alternate Interface Description List
 */
static struct usbd_alternate_description cdc_comm_alternate_descriptions[] = {
        { 
                .iInterface = "Belcarra USBLAN - MDLM/BLAN",
                .bInterfaceClass = USB_CLASS_COMM,   
                .bInterfaceSubClass =  COMMUNICATIONS_ENCM_SUBCLASS,
                .bInterfaceProtocol = 0,
                .classes = sizeof (cdc_comm_class_descriptors) / sizeof (struct usbd_generic_class_descriptor *),
                .class_list =  cdc_comm_class_descriptors,
                .endpoints =  1,
                .endpoint_index = cdc_int_endpoint_index,
        },
};

static struct usbd_alternate_description cdc_data_alternate_descriptions[] = {
        { 
                .iInterface = "Belcarra USBLAN - MDLM/BLAN No Data",
                .bInterfaceClass = USB_CLASS_DATA,      // XXX
                .bInterfaceSubClass =  0,               // XXX
                .bInterfaceProtocol = 0,                // XXX
        },
        { 
                .iInterface = "Belcarra USBLAN - MDLM/BLAN Data",
                .bInterfaceClass = USB_CLASS_DATA,      // XXX
                .bInterfaceSubClass =  0x0,             // XXX
                .bInterfaceProtocol = 0x0,              // XXX
                .endpoints =  2,
                .endpoint_index = cdc_data_endpoint_index,
        },
};


/* CDC Interface descriptions and descriptors
 */
/*! Interface Description List
 */
struct usbd_interface_description cdc_interfaces[] = {
        { 
                .alternates = sizeof (cdc_comm_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = cdc_comm_alternate_descriptions,
        },
        { 
                .alternates = sizeof (cdc_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = cdc_data_alternate_descriptions,
        },
};


/* ********************************************************************************************* */

/*! cdc_fd_function_enable - enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 */

int cdc_fd_function_enable (struct usbd_function_instance *function_instance)
{
        return net_fd_function_enable(function_instance, network_cdc);
}


/* ********************************************************************************************* */
/*! cdc_fd_function_ops - operations table for network function driver
 */
struct usbd_function_operations cdc_fd_function_ops = {
        .function_enable = cdc_fd_function_enable,
        .function_disable = net_fd_function_disable,

        .device_request = net_fd_device_request,
        .endpoint_cleared = net_fd_endpoint_cleared,

        .endpoint_cleared = net_fd_endpoint_cleared,
        .set_configuration = net_fd_set_configuration,
        .set_interface = net_fd_set_interface,
        .reset = net_fd_reset,
        .suspended = net_fd_suspended,
        .resumed = net_fd_resumed,
};


/*! function driver description
 */
struct usbd_interface_driver cdc_interface_driver = {
        .driver.name = "net-cdc-if",
        .driver.fops = &cdc_fd_function_ops,
        .interfaces = sizeof (cdc_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = cdc_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = net_fd_endpoint_requests,

        .bFunctionClass = USB_CLASS_COMM,
        .bFunctionSubClass = COMMUNICATIONS_ENCM_SUBCLASS,
        .bFunctionProtocol = COMMUNICATIONS_NO_PROTOCOL,
        .iFunction = "Belcarra USBLAN - CDC/ECM",
};

/* ********************************************************************************************* */

/*!
 * @brief cdc_mod_init
 * @return int
 */
int cdc_mod_init (void)
{
        return usbd_register_interface_function (&cdc_interface_driver, "net-cdc-if", NULL);
}

void cdc_mod_exit(void)
{
        usbd_deregister_interface_function (&cdc_interface_driver);
}

#else                   /* CONFIG_OTG_NETWORK_CDC */
/*!
 * @brief cdc_mod_init
 * @return int
 */
int cdc_mod_init (void)
{
        return 0;
}

void cdc_mod_exit(void)
{
}
#endif                  /* CONFIG_OTG_NETWORK_CDC */

