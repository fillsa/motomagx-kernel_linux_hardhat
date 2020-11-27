/*
 * otg/hcd/hc_xfer_rh.h - Generic transfer level USBOTG aware Host Controller Driver (HCD)
 * @(#) balden@seth2.belcarratech.com|otg/otg/hcd-rh.h|20051116205001|14204
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies
 *
 * By:
 *      Stuart Lynne <sl@belcara.com>,
 *      Bruce Balden <balden@belcara.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 */
/*
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
 * @file otg/otg/hcd-rh.h
 * @brief Implements Generic Root Hub function for Generic Host Controller Driver.
 *
 * @ingroup OTGHCD
 */
#ifndef HC_XFER_RH_H
#define HC_XFER_RH_H 1

#define XHC_RH_STRING_CONFIGURATION 1
#define XHC_RH_STRING_INTERFACE     2
#define XHC_RH_STRING_SERIAL        3
#define XHC_RH_STRING_PRODUCT       4
#define XHC_RH_STRING_MANUFACTURER  5

/*===========================================================================*
 * Functions provided by the generic virtual root hub.
 *===========================================================================*/

/*===========================================================================*
 * For the generic transfer aware framework.
 *===========================================================================*/

/*
 * hcd_rh_submit_urb()
 * Returns: values as for the generic submit_urb() function.
 */
extern int hcd_rh_submit_urb(struct bus_hcpriv *bus_hcpriv, struct urb *urb, int mem_flags);

extern int hcd_rh_unlink_urb(struct bus_hcpriv *bus_hcpriv, struct urb *urb);

extern void hcd_rh_get_ops(struct bus_hcpriv *bus_hcpriv);

extern int hcd_rh_init(struct bus_hcpriv *bus_hcpriv);

extern void hcd_rh_exit(struct bus_hcpriv *bus_hcpriv);

/*===========================================================================*
 * For the hardware specific root hub component.
 *===========================================================================*/

extern irqreturn_t hcd_rh_int_hndlr(int irq, void *dev_id, struct pt_regs *regs);

#endif
