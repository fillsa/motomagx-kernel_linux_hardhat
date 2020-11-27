/*
 * otg/ocd/otg-pcd/tr-init-l24.c - Traditional Device Peripheral and OTG Controller Drivers Module Initialization
 * @(#) balden@seth2.belcarratech.com|otg/ocd/otglib/tr-init-l24.c|20051116205000|53690
 *
 *      Copyright (c) 2004-2005 Belcarra
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
 * 12/12/2005         Motorola         Initial distribution
 * 08/09/2006         Motorola         nfs over USB 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 *
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
 */
/*!
 * @file otg/ocd/otglib/tr-init-l24.c
 * @brief Traditional device driver init.
 *
 * TR OTG PCD/OCD/TCD Initialization
 *
 * This file initializes all of the low level hardware drivers for a traditional device.
 *
 * Traditional USB Devices must implement the following:
 *
 *      ocd_ops         operations from ocd driver
 *      pcd_ops         operations from pcd driver
 *      tcd_ops         operations from tcd driver
 *
 * The OTG Controller driver (ocd) implements any required OTG timers and if
 * necessary mediates access to and initializes the OTG and/or USB hardware.
 *
 * The Peripheral Controller Driver (pcd) implements the lowest layer of the
 * USBD stack to send and receive data from the USB actings as a USB
 * peripheral.
 *
 * The Transceiver Controller Driver (tcd) implements control of the OTG or
 * USB transceiver. At a minimum for a traditional device this should
 * implement VBUS sensing (sometimes known as cable detect) and where
 * possible control over the DP+ pullup resistor.
 *
 *
 * Notes
 *
 * 1. This is the linux 2.4 version.
 *
 * 2. This will optionally do an auto start with TR_INIT if the OCD_CAPBILITIES_AUTO
 * flag is set.
 *
 * TODO
 *
 * 1. hook up serial_number_str to pcd.c for use with bus interface layer.
 *
 * @ingroup OTGPCD
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-bus.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <otg/otg-pcd.h>
#include <otg/otg-ocd.h>

EMBED_LICENSE();

MOD_PARM (serial_number_str, "s");
MOD_PARM_DESC (serial_number_str, "Serial Number");
MOD_AUTHOR ("sl@belcarra.com, tbr@belcarra.com");
MOD_DESCRIPTION ("Traditional USB Device");

char *serial_number_str;

otg_tag_t OCD;
struct ocd_instance *ocd_instance;

otg_tag_t PCD;
struct pcd_instance *pcd_instance;

otg_tag_t TCD;
struct tcd_instance *tcd_instance;


/* ************************************************************************************* */

/* tr_ocd_start_timer
 * Fake - Set or reset timer to interrupt in number of uS (micro-seconds). 
 * This is only suitable for MN or TR firmware.
 */
int tr_ocd_start_timer(struct otg_instance *otg, int usec)
{
        otg_event(otg, TMOUT, OCD, "FAKE TMOUT");
        return 0;
}

/* ************************************************************************** */
/* tr_tcd_en_func - used to enable or disable transciever
 *
 */
void tr_tcd_en_func(struct otg_instance *otg, u8 flag)
{
        struct pcd_instance *pcd = (struct pcd_instance *) otg->pcd;
        struct usbd_bus_instance *bus = pcd->bus;

        //printk(KERN_INFO"%s: start\n", __FUNCTION__);
        TRACE_MSG0(TCD, "--");
        TRACE_MSG2(TCD, "pcd: %x bus: %x", pcd, pcd->bus);

        switch (flag) {
        case PULSE:
        case SET:
                TRACE_MSG0(TCD, "TR_TCD_EN SET");
                /* enable clock interrupt */
                otg_event_set_irq(tcd_instance->otg, 1, 1, B_SESS_VLD, TCD, "B_SESS_VLD");
                break;
        case RESET:
                TRACE_MSG0(TCD, "TR_TCD_EN RESET");
                /* disable clock interrupt */
                break;
        }
//        TRACE_MSG2(TCD, "ep0 endpoint: %x bEndpointAddress: %02x BBB", bus->ep0->endpoint_map_array->endpoint,
 //                       bus->ep0->endpoint_map_array->endpoint->bEndpointAddress);
}


/* ************************************************************************** */

/* tr_modexit - This is used as module exit, and as cleanup if modinit fails.
 *
 * Specifically for each driver:
 *
 * 	call ops.mod_exit
 * 	reset instance address and  ops table address in state machine to NULL
 * 	invalidate tag
 */
static void tr_modexit (void)
{
        struct otg_instance *otg = ocd_instance->otg;
        printk(KERN_INFO"%s\n", __FUNCTION__);

        TRACE_MSG1(TCD, "otg: %x", otg);
        if (otg) {
                TRACE_MSG0(TCD, "Calling otg_exit()");
                otg_exit(otg);
                TRACE_MSG0(TCD, "back from otg_exit()");
        } 
        else
                TRACE_MSG0(TCD, "did not call otg_exit()");

#if 0
        if ((ocd_ops.capabilities & OCD_CAPABILITIES_TR) && (ocd_ops.capabilities & OCD_CAPABILITIES_AUTO)) {
                printk(KERN_INFO"%s calling otg_admin\n",__FUNCTION__);
                otg_admin("exit_all", "Traditional Device Auto TR_INIT");
        }
#endif

        if (tcd_ops.mod_exit) tcd_ops.mod_exit();
        if (ocd_ops.mod_exit) ocd_ops.mod_exit();
        if (pcd_ops.mod_exit) pcd_ops.mod_exit();

        tcd_instance = otg_set_tcd_ops(NULL);
        ocd_instance = otg_set_ocd_ops(NULL);
        pcd_instance = otg_set_pcd_ops(NULL);

        TCD = otg_trace_invalidate_tag(TCD);
        OCD = otg_trace_invalidate_tag(OCD);
        PCD = otg_trace_invalidate_tag(PCD);
        printk(KERN_INFO"%s finished\n", __FUNCTION__);
}


/* tr_modinit - linux module initialization
 *
 * This needs to initialize the ocd, pcd and tcd drivers.  
 *
 * Specifically for each driver:
 *
 * 	obtain tag
 * 	pass ops table address to state machine and get instance address
 * 	call ops.mod_init
 *
 */
static int tr_modinit (void)
{
        struct otg_instance *otg;
        printk(KERN_INFO"%s\n", __FUNCTION__);

        #if !defined(OTG_C99)
        pcd_global_init();
        #endif /* !defined(OTG_C99) */

        /* For each of ocd, pcd and tcd do:
         * 
         *      get tag
         *      update ops table with default functions where possible
         *      do otg_set_xxx_ops() as appropriate
         *      call xxx_ops.mod_init as appropriate
         */
        
        OCD = otg_trace_obtain_tag();
        PCD = otg_trace_obtain_tag();
        TCD = otg_trace_obtain_tag();

        UNLESS(ocd_ops.start_timer) ocd_ops.start_timer = tr_ocd_start_timer;
        UNLESS(pcd_ops.pcd_init_func) pcd_ops.pcd_init_func = pcd_init_func;
        UNLESS(pcd_ops.pcd_en_func) pcd_ops.pcd_en_func = pcd_en_func;
        UNLESS(tcd_ops.tcd_en_func) tcd_ops.tcd_en_func = tr_tcd_en_func;

        THROW_IF((ocd_ops.mod_init) ? ocd_ops.mod_init() : 0, error);
        THROW_IF((pcd_ops.mod_init) ? pcd_ops.mod_init() : 0, error);
        THROW_IF((tcd_ops.mod_init) ? tcd_ops.mod_init() : 0, error);

        THROW_UNLESS(ocd_instance = otg_set_ocd_ops(&ocd_ops), error);
        THROW_UNLESS(pcd_instance = otg_set_pcd_ops(&pcd_ops), error);
        THROW_UNLESS(tcd_instance = otg_set_tcd_ops(&tcd_ops), error);

        THROW_UNLESS(ocd_instance && (otg = ocd_instance->otg), error);

        CATCH(error) {
                tr_modexit();
                return -EINVAL;
        }
        /* Success!
         */
        otg_init(otg);
#if 0
        otg_start(otg, ocd_ops.capabilities & OCD_CAPABILITIES_TR);
#endif
        return 0;
}

#ifdef CONFIG_OTG_NFS
late_initcall (tr_modinit);
#else
module_init (tr_modinit);
#endif
MOD_EXIT (tr_modexit);

