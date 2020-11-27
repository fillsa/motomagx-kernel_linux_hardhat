/*
 * otg/functions/acm/acm-l24-os.c
 * @(#) sl@belcarra.com|otg/functions/acm/acm-l26.c|20060403224909|25438
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2006 Motorola, Inc. 
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 04/25/2006         Motorola         Initial distribution
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
 * @file otg/functions/acm/acm-l26.c
 * @brief ACM Function Driver private defines
 *
 * This is the linux module wrapper for the tty-if function driver.
 *
 *                    TTY
 *                    Interface
 *    Upper           +----------+
 *    Edge            | tty-l26  |
 *    Implementation  +----------+
 *
 *
 *    Function        +----------+
 *    Descriptors     | tty-if   |
 *    Registration    +----------+
 *
 *
 *    Function        +----------+
 *    I/O             | acm      |
 *    Implementation  +----------+
 *
 *
 *    Module          +----------+
 *    Loading         | acm-l26  |      <-----
 *                    +----------+
 *
 * @ingroup TTYFunction
 */


//#include <otg/osversion.h>
#include <otg/otg-compat.h>
#include <otg/otg-module.h>

MOD_AUTHOR ("sl@belcarra.com");

MOD_DESCRIPTION ("Belcarra TTY Function");
EMBED_LICENSE();

#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/smp_lock.h>
#include <linux/slab.h>
#include <linux/serial.h>

#include <linux/devfs_fs_kernel.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>

#include <linux/capability.h>
#include <otg/otg-trace.h>
#include <public/otg-node.h>
#include "acm.h"
#include "tty.h"

MOD_PARM_INT (vendor_id, "Device Vendor ID", 0);
MOD_PARM_INT (product_id, "Device Product ID", 0);
MOD_PARM_INT (max_queued_urbs, "Maximum TX Queued Urbs", 0);

/*! acm_l26_modinit - module init
 *
 * This is called immediately after the module is loaded or during
 * the kernel driver initialization if linked into the kernel.
 *
 */
static int acm_l26_modinit (void)
{
        BOOL tty_l26 = FALSE, tty_if = FALSE;
        int minor_numbers=1;
        /* register tty  and usb interface function drivers
         */
        TTY = otg_trace_obtain_tag();
        THROW_UNLESS(tty_l26 = BOOLEAN(!tty_l26_init(ACM_DRIVER_PROCFS_NAME, ACM_TTY_MINORS)), error);
        THROW_UNLESS(tty_if = BOOLEAN(!tty_if_init()), error);

        CATCH(error) {
                if (tty_l26) tty_l26_exit();
                if (tty_if) tty_if_exit();
                otg_trace_invalidate_tag(TTY);
                return -EINVAL;
        }
        return 0;
}


/*! acm_l26_modexit - module cleanup
 *
 * This is called prior to the module being unloaded.
 */
static void acm_l26_modexit (void)
{
        /* de-register as tty  and usb drivers */
        tty_l26_exit();
        tty_if_exit();
        otg_trace_invalidate_tag(TTY);
}

#ifdef CONFIG_OTG_NFS
late_initcall (acm_l26_modinit);
#else
module_init (acm_l26_modinit);
#endif
module_exit (acm_l26_modexit);

