/*
 * otg/functions/generic/generic-cf.h
 * @(#) sl@whiskey.enposte.net|otg/functions/generic/generic.h|20051116230728|03475
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
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


/*
 * N.B. This list needs to be synchronized with both the default pre-defined
 * configuration conditional settings (see above) and the Kconfig list
 */
struct generic_config {
        struct usbd_composite_driver composite_driver;
        char *class_name;
        char *interface_names;
        char ** interface_list;
        struct usbd_configuration_description configuration_description;
        struct usbd_device_description device_description;
        int registered;
};

