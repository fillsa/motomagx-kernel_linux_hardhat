/*
 * otg/functions/network/basic2.c - Network Function Driver
 * @(#) balden@seth2.belcarratech.com|otg/functions/network/basic2-if.c|20051116204958|53276
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
 * @file otg/functions/network/basic2-if.c
 * @brief This file implements the required descriptors to implement
 * a basic network device with two interfaces.
 *
 * The BASIC2 network driver implements a very simple descriptor set.
 * Two interfaces with two BULK data endpoints and a optional
 * INTERRUPT endpoint.
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

#ifdef CONFIG_OTG_NETWORK_BASIC
/* USB BASIC Configuration ******************************************************************** */

/*! Data Alternate Interface Description List
 */
static struct usbd_alternate_description basic2_nodata_alternate_descriptions[] = {
        { 
                .iInterface = "Belcarra USBLAN - Basic2",
                .bInterfaceClass = LINEO_CLASS,
                .bInterfaceSubClass =  LINEO_SUBCLASS_BASIC_NET,
                .bInterfaceProtocol = 0,
                .endpoints =  1,
                .endpoint_index = cdc_int_endpoint_index,
        },
};

static struct usbd_alternate_description basic2_data_alternate_descriptions[] = {
        { 
                .iInterface = "Belcarra USBLAN - Basic2 Data",
                .bInterfaceClass = LINEO_CLASS,
                .bInterfaceSubClass =  LINEO_SUBCLASS_BASIC_NET,
                .bInterfaceProtocol = LINEO_BASIC_NET_CRC,
                .endpoints =  2,
                .endpoint_index = cdc_data_endpoint_index,
        },
};


/* BASIC Interface descriptions and descriptors
 */
/*! Interface Description List
 */
struct usbd_interface_description basic2_interfaces[] = {
        { 
                .alternates = sizeof (basic2_nodata_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = basic2_data_alternate_descriptions,
        },
        { 
                .alternates = sizeof (basic2_data_alternate_descriptions) / sizeof (struct usbd_alternate_description),
                .alternate_list = basic2_data_alternate_descriptions,
        },
};


/* ********************************************************************************************* */

/*! basic2_fd_function_enable - enable the function driver
 *
 * Called for usbd_function_enable() from usbd_register_device()
 */

int basic2_fd_function_enable (struct usbd_function_instance *function_instance)
{
        return net_fd_function_enable(function_instance, network_basic2);
}


/* ********************************************************************************************* */
/*! basic2_fd_function_ops - operations table for network function driver
 */
struct usbd_function_operations basic2_fd_function_ops = {
        .function_enable = basic2_fd_function_enable,
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
struct usbd_interface_driver basic2_interface_driver = {
        .driver.name = "net-basic2-if",
        .driver.fops = &basic2_fd_function_ops,
        .interfaces = sizeof (basic2_interfaces) / sizeof (struct usbd_interface_description),
        .interface_list = basic2_interfaces,
        .endpointsRequested = ENDPOINTS,
        .requestedEndpoints = net_fd_endpoint_requests,
        
        .bFunctionClass = USB_CLASS_COMM,
        .bFunctionSubClass = COMMUNICATIONS_ENCM_SUBCLASS,
        .bFunctionProtocol = COMMUNICATIONS_NO_PROTOCOL,
        .iFunction = "Belcarra USBLAN - Basic2",
};

/* ********************************************************************************************* */

/*!
 * @brief basic2_mod_init
 * @return none
 */
int basic2_mod_init (void)
{
        return usbd_register_interface_function (&basic2_interface_driver, "net-basic2-if", NULL);
}

void basic2_mod_exit(void)
{
        usbd_deregister_interface_function (&basic2_interface_driver);
}

#else                   /* CONFIG_OTG_NETWORK_BASIC */
/*!
 8 @brief basic2_mod_init
 * @return none
 */
int basic2_mod_init (void)
{
        return 0;
}

void basic2_mod_exit(void)
{
}
#endif                  /* CONFIG_OTG_NETWORK_BASIC */

