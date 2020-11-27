/*
 * otg/functions/network/net-os.h
 * @(#) balden@seth2.belcarratech.com|otg/functions/network/net-os.h|20051116204958|63266
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
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

/*!
 * @file otg/functions/network/net-os.h
 * @brief Structures and function definitions required for implementation
 * of the OS specific upper edge (network interface) for the Network
 * Function Driver.
 *
 * A USB network driver is composed of two pieces:
 *    1) An OS specific piece that handles creating and operating
 *       a network device for the given OS.
 *    2) A USB specific piece that interfaces either with the host
 *       usbcore layer, or with the device usbdcore layer.
 *
 * If the USB piece interfaces with the host usbcore layer you get
 * a network class driver.  If the USB piece interfaces with the usbdcore
 * layer you get a network function driver.
 *
 * This file describes the functions exported by the various net-*-os.c
 * files (implementing (1)) for use in net-fd.c (2).
 *
 *
 * @ingroup NetworkFunction
 */

#ifndef NET_OS_H
#define NET_OS_H 1

/*
 * net_os_mutex_enter - enter mutex region
 */
extern void net_os_mutex_enter(struct usbd_function_instance *);

/*
 * net_os_mutex_exit - exit mutex region
 */
extern void net_os_mutex_exit(struct usbd_function_instance *);

/*
 * net_os_send_notification_later - schedule a callback to the USB part to send
 *                                  an INT notification.
 */
extern void net_os_send_notification_later(struct usbd_function_instance *);

/*
 * net_os_xmit_done - called from USB part when a transmit completes, good or bad.
 *                    tx_rc is SEND_FINISHED_ERROR, SEND_CANCELLED or...
 *                    buff_ctx is the pointer passed to fd_ops->start_xmit().
 * Return non-zero only if network does not exist, ow 0.
 */
extern int net_os_xmit_done(struct usbd_function_instance *function, void *buff_ctx, int tx_rc);

/*
 *
 */
extern void *net_os_alloc_buffer(struct usbd_function_instance *, u8 **cp, int n);
extern void net_os_dealloc_buffer(struct usbd_function_instance *function_instance, void *data);

/*
 * net_os_recv_buffer - forward a received URB, or clean up after a bad one.
 *      buff_ctx == NULL --> just accumulate stats (count 1 bad buff)
 *      crc_bad != 0     --> count a crc error and free buff_ctx/skb
 *      trim             --> amount to trim from valid buffer (i.e. shorten
 *                           from amount requested in net_os_alloc_buff(..... n).
 *                    This will be called from interrupt context.
 */
extern int net_os_recv_buffer(struct usbd_function_instance *, void *buff_ctx, int crc_bad, int trim);

/*
 * net_os_config - schedule a network hotplug event if the OS supports hotplug.
 */
extern void net_os_config(struct usbd_function_instance *);

/*
 * net_os_hotplug - schedule a network hotplug event if the OS supports hotplug.
 */
extern void net_os_hotplug(struct usbd_function_instance *);

/*
 *
 */
extern void net_os_enable(struct usbd_function_instance *function);

/*
 *
 */
extern void net_os_disable(struct usbd_function_instance *function);

/*
 *
 */
extern void net_os_carrier_on(struct usbd_function_instance *);

/*
 *
 */
extern void net_os_carrier_off(struct usbd_function_instance *);

#endif
