/*
 * otg/ocd/otg-tcd/tcd-init-l24.c - OTG Transceiver Controller Driver Module Initialization
 * @(#) balden@seth2.belcarratech.com|otg/ocd/otglib/tcd-init-l24.c|20051116205000|49360
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
 * 08/09/2006         Motorola         nfs over USB 
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
 */
/*!
 * @file otg/ocd/otglib/tcd-init-l24.c
 * @brief PCD only driver init.
 *
 * OTG TCD Initialization
 *
 * This file initializes the low level hardware drivers.
 *
 * This is the linux 2.4 version.
 *
 * @ingroup OTGTCD
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


#ifdef MODULE
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,4,17)
MODULE_LICENSE ("GPL");
#endif
MODULE_AUTHOR ("sl@belcarra.com, tbr@belcarra.com");
MODULE_DESCRIPTION ("USB On-The-Go TCD");
#endif

otg_tag_t TCD;

extern struct tcd_ops tcd_ops;
struct tcd_instance *tcd_instance;

/* ************************************************************************************* */

/* tcd_modexit - This is *only* used for drivers compiled and used as a module.
 */
static void tcd_modexit (void)
{
        struct otg_instance *otg = tcd_instance->otg;
        //printk(KERN_INFO"%s\n", __FUNCTION__);

        if (otg)
                otg_exit(otg);

        if (tcd_ops.mod_exit) tcd_ops.mod_exit();
        tcd_instance = otg_set_tcd_ops(NULL);
        otg_trace_invalidate_tag(TCD);
}

/* otg_modinit - linux module initialization
 *
 * This needs to initialize the hcd, pcd and tcd drivers. This includes tcd and possibly hcd
 * for some architectures.
 *
 */
static int tcd_modinit (void)
{
        TCD = otg_trace_obtain_tag();
        THROW_UNLESS(tcd_instance = otg_set_tcd_ops(&tcd_ops), error);
        THROW_IF((tcd_ops.mod_init) ? tcd_ops.mod_init() : 0, error);
        //printk(KERN_INFO"%s finish\n", __FUNCTION__);

        TRACE_MSG0(TCD, "--");

        CATCH(error) {
                tcd_modexit();
                return -EINVAL;
        }
        return 0;
}
#ifdef CONFIG_OTG_NFS
late_initcall (tcd_modinit);
#else
module_init (tcd_modinit);
#endif
module_exit (tcd_modexit);

