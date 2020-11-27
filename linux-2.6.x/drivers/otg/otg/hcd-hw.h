/*
 * otg/hcd/hc_xfer_hw.h - Generic transfer level USBOTG aware Host Controller Driver (HCD)
 * @(#) balden@seth2.belcarratech.com|otg/otg/hcd-hw.h|20051116205001|11801
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
 * @file otg/otg/hcd-hw.h
 * @brief Implements Generic Host Controller Driver
 *
 * @ingroup OTGHCD
 */
#ifndef HC_XFER_HW_H
#define HC_XFER_HW_H 1

/*===========================================================================*
 * Functions provided by the hardware specific component (whatever HW it is).
 * Each HW specific component provides these functions.
 *===========================================================================*/

/*===========================================================================*
 * For the generic transfer aware framework.
 *===========================================================================*/

/*
 * hcd_hw_max_active_transfers()
 * Returns: the (constant) maximum number of concurrently active transfers the HW will handle.
 */
extern int hcd_hw_max_active_transfers(struct bus_hcpriv *bus_hcpriv);

/*
 * hcd_hw_start_transfer()
 * Returns: tid >= 0  when the transfer starts - this is an index in [0..hcd_hw_max_active_transfers]
 *          -1        when the transfer is invalid,
 *          -2        when there are no resources for the transfer currently available.
 */
extern int hcd_hw_start_transfer(struct bus_hcpriv *bus_hcpriv,
                                 int len,       // length of data region
                                 void *data,    // virtual address of data region
                                 int toggle,    // toggle value to start the xfer with
                                 int maxps,     // max packet size of endpoint
                                 int slow,      // 1 ==> slow speed, 0 ==> full speed  QQSV verify: (((urb->pipe) >> 26) & 1)
                                 int endpoint,  // endpoint number
                                 int address,   // USB device address
                                 int pid,       // {PID_OUT, PID_IN, PID_SETUP}
                                 int format,    // PIPE_{CONTROL,BULK,INTERRUPT,ISOCHRONOUS}
                                 u32 other);    // Values depending on format

/*
 * hcd_hw_unlink_urb()
 * Called in irq_lock, calls back to xhc_transfer_complete().
 * Returns: 0     when transfer is unlinked, non-zero otherwise.
 */
extern int hcd_hw_unlink_urb(struct bus_hcpriv *bus_hcpriv, int tid); // tid is value returned by hcd_hw_start_transfer()

extern u32 hcd_hw_frame_number(struct bus_hcpriv *bus_hcpriv);

extern void hcd_hw_enable_interrupts(struct bus_hcpriv *bus_hcpriv);
extern void hcd_hw_disable_interrupts(struct bus_hcpriv *bus_hcpriv);

extern void hcd_hw_get_ops(struct bus_hcpriv *bus_hcpriv);

extern int hcd_hw_init(struct bus_hcpriv *bus_hcpriv);

extern void hcd_hw_exit(struct bus_hcpriv *bus_hcpriv);

/*===========================================================================*
 * For the virtual root hub.
 *===========================================================================*/

extern int hcd_hw_rh_num_ports(struct bus_hcpriv *bus_hcpriv);
extern u8 hcd_hw_rh_otg_capable_mask(struct bus_hcpriv *bus_hcpriv);

extern char *hcd_hw_rh_string(struct bus_hcpriv *bus_hcpriv, int strno);
extern u16 hcd_hw_rh_idVendor(struct bus_hcpriv *bus_hcpriv);
extern u16 hcd_hw_rh_idProduct(struct bus_hcpriv *bus_hcpriv);
extern u16 hcd_hw_rh_bcdDevice(struct bus_hcpriv *bus_hcpriv);
extern u8 hcd_hw_rh_cfg_bmAttributes(struct bus_hcpriv *bus_hcpriv);
extern u8 hcd_hw_rh_cfg_MaxPower(struct bus_hcpriv *bus_hcpriv);
extern u16 hcd_hw_rh_hub_attributes(struct bus_hcpriv *bus_hcpriv);
extern u8 hcd_hw_rh_power_delay(struct bus_hcpriv *bus_hcpriv);
extern u8 hcd_hw_rh_hub_contr_current(struct bus_hcpriv *bus_hcpriv);
extern u8 hcd_hw_rh_DeviceRemovable(struct bus_hcpriv *bus_hcpriv);
extern u8 hcd_hw_rh_PortPwrCtrlMask(struct bus_hcpriv *bus_hcpriv);
extern u32 hcd_hw_rh_get_hub_change_status(struct bus_hcpriv *bus_hcpriv);
extern void hcd_hw_rh_hub_feature(struct bus_hcpriv *bus_hcpriv, int feat_selector, int set_flag);
extern void hcd_hw_hcd_en_func(struct otg_instance *oi, u8 on);
/*
 * Note: in the following functions, portnum is 0-origin.
 * (I.e., valid range is [0..(hcd_hw_rh_num_ports-1)].)
 */
extern u32 hcd_hw_rh_get_port_change_status(struct bus_hcpriv *bus_hcpriv, int portnum);
extern void hcd_hw_rh_port_feature(struct bus_hcpriv *bus_hcpriv, u16 wValue, u16 wIndex, int set_flag);


#endif
