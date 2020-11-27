/*
 * otg/ocd/otg-pcd/pcd-init-l24.c - OTG Peripheral Controller Driver Module Initialization
 * @(#) balden@seth2.belcarratech.com|otg/ocd/otglib/pcd-init-l24.c|20051116205000|42973
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
 * @file otg/ocd/otglib/pcd-init-l24.c
 * @brief PCD only driver init.
 *
 *
 * PCD Initialization
 *
 * This file initializes all of the low level hardware drivers for a PCD.
 *
 * The Peripheral Controller Driver (pcd) implements the lowest layer of the
 * USBD stack to send and receive data from the USB actings as a USB
 * peripheral.
 *
 * Notes
 *
 * 1. This is the linux 2.4 version.
 *
 * TODO
 *
 * 1. hook up serial_number_str to pcd.c for use with bus interface layer.
 *
 *
 * @ingroup OTGPCD
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/otg-module.h>
#include <otg/usbp-bus.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <otg/otg-pcd.h>

#ifdef MODULE
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,4,17)
MODULE_LICENSE ("GPL");
#endif
MODULE_PARM (serial_number_str, "s");
MODULE_PARM_DESC (serial_number_str, "Serial Number");
MODULE_AUTHOR ("sl@belcarra.com, tbr@belcarra.com");
MODULE_DESCRIPTION ("USB On-The-Go PCD");
#endif
char *serial_number_str;

otg_tag_t PCD;
struct pcd_instance *pcd_instance;

/* ************************************************************************************* */

/* pcd_modexit - module exit or init failure cleanup
 *
 * Specifically for each driver:
 *
 * 	call ops.mod_exit
 * 	reset instance address and  ops table address in state machine to NULL
 * 	invalidate tag
 */
static void pcd_modexit (void)
{
        struct otg_instance *otg = ocd_instance->otg;
        printk(KERN_INFO"%s\n", __FUNCTION__);
        if (otg)
                otg_exit(otg);
        if (pcd_ops.mod_exit) pcd_ops.mod_exit();
        pcd_instance = otg_set_pcd_ops(NULL);
        PCD = otg_trace_invalidate_tag(PCD);
}

/* pcd_modinit - linux module initialization
 *
 * This needs to initialize the ocd, pcd and tcd drivers.  
 *
 * Specifically for each driver:
 *
 * 	obtain tag
 * 	pass ops table address to state machine and get instance address
 * 	call ops.mod_init
 *
 * Note that we automatically provide a default tcd_init if
 * none is set.
 */
static int pcd_modinit (void)
{
        printk(KERN_INFO"%s\n", __FUNCTION__);

        #if !defined(OTG_C99)
        pcd_global_init();
        #endif /* !defined(OTG_C99) */

        PCD = otg_trace_obtain_tag();

        UNLESS(pcd_ops.pcd_init_func) pcd_ops.pcd_init_func = pcd_init_func;
        THROW_UNLESS(pcd_instance = otg_set_pcd_ops(&pcd_ops), error);
        THROW_IF((pcd_ops.mod_init) ? pcd_ops.mod_init() : 0, error);

        CATCH(error) {
                pcd_modexit();
                return -EINVAL;
        }
        return 0;
}
MOD_EXIT(pcd_modexit);
MOD_INIT(pcd_modinit);
