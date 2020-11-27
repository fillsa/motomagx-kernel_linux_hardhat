/*
 * otg/ocd/otg-tcd/tcd.c - OTG Transceiver Controller Driver
 * @(#) balden@seth2.belcarratech.com|otg/ocd/otglib/tcd.c|20051116205000|50788
 *
 *      Copyright (c) 2004-2005 Belcarra
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
 * @file otg/ocd/otglib/tcd.c
 * @brief TCD Support
 * Notes
 *
 * @ingroup OTGTCD
 */

#include <otg/otg-compat.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-bus.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <otg/otg-pcd.h>

extern struct tcd_ops tcd_ops;

/* ************************************************************************************* */


void tcd_cable_event_irq (struct otg_instance *otg, u8 connect)
{
        //TRACE_MSG1(TCD, "TCD_CABLE_EVENT: vbus: %d", tcd_vbus(otg));
        if (connect)
            otg_event(otg, VBUS_VLD, TCD, "CABLE");
        else
            otg_event(otg, VBUS_VLD_, TCD, "CABLE");
}
void tcd_cable_event (struct otg_instance *otg, u8 connect  )
{
        unsigned long flags;
        local_irq_save (flags);
        tcd_cable_event_irq (otg, connect);
        local_irq_restore (flags);
}


/* ************************************************************************************* */
#if 0
int tcd_vbus (struct otg_instance *otg)
{
        TRACE_MSG0(TCD, "OCD VBUS");
        return tcd_ops.vbus ? tcd_ops.vbus (otg) : 1;
}
#endif

