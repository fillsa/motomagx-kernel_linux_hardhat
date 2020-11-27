/*
 * otg/functions/network/net-fd.h - Network Function Driver
 * @(#) balden@seth2.belcarratech.com|otg/functions/network/net-fd.h|20051116204958|60657
 *
 *      Copyright (c) 2002-2004 Belcarra
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
 * @file otg/functions/network/net-fd.h
 * @brief These are the functions exported by the USB specific parts
 * (i.e. net-fd.c and net-cl.c) for use in the OS specific
 * layers (e.g. net-l24-os.c).
 *
 *
 * @ingroup NetworkFunction
 */
#ifndef NET_FD_H
#define NET_FD_H 1

#if 0
struct net_usb_services {
        int (*initialize_usb_part)(struct usb_network_private *npd, char *info_str);
        void (*terminate_usb_part)(struct usb_network_private *npd);
        void (*send_int_notification)(struct usb_network_private *npd, int connected, int data);
        int (*start_xmit)(struct usb_network_private *npd, __u8 *buff, int len, void *buff_ctx);
};

extern struct net_usb_services net_fd_usb_ops;
#endif

#endif
