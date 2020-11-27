/*
 * otg/functions/acm/mcpc-l24-os.c
 * @(#) sl@belcarra.com|otg/functions/acm/mcpc-l26.c|20060403224909|51582
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @file otg/functions/acm/mcpc-l26.c
 * @brief ACM Function Driver private defines
 *
 * This is the linux module wrapper for the mcpc-if function driver.
 *
 *                    TTY
 *                    Interface
 *    Upper           +----------+
 *    Edge            | tty-l26  |
 *    Implementation  +----------+
 *
 *
 *    Function        +----------+
 *    Descriptors     | mcpc-if  |
 *    Registration    +----------+
 *
 *
 *    Function        +----------+
 *    I/O             | acm      |
 *    Implementation  +----------+
 *
 *
 *    Module          +----------+
 *    Loading         | mcpc-l26 |      <----
 *                    +----------+
 *
 *
 * @ingroup TTYFunction
 */


//#include <otg/osversion.h>
#include <otg/otg-compat.h>
#include <otg/otg-module.h>

MOD_AUTHOR ("sl@belcarra.com");

MOD_DESCRIPTION ("Belcarra MCPC Function");
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
#include "acm.h"
#include "tty.h"


MOD_PARM_INT (vendor_id, "Device Vendor ID", 0);
MOD_PARM_INT (product_id, "Device Product ID", 0);
MOD_PARM_INT (max_queued_urbs, "Maximum TX Queued Urbs", 0);
MOD_PARM_INT (max_queued_bytes, "Maximum TX Queued Bytes", 0);

/*! mcpc_l26_modinit - module init
 *
 * This is called immediately after the module is loaded or during
 * the kernel driver initialization if linked into the kernel.
 *
 */
STATIC int mcpc_l26_modinit (void)
{
        BOOL tty_l26 = FALSE, dun_if = FALSE, obex_if = FALSE, atcom_if = FALSE;

        TTY = otg_trace_obtain_tag();
        THROW_UNLESS (tty_l26 = BOOLEAN(!tty_l26_init("mcpc_if", 6)), error);
        THROW_UNLESS (tty_l26 = BOOLEAN(!dun_if_init()), error);
        THROW_UNLESS (tty_l26 = BOOLEAN(!obex_if_init()), error);
        THROW_UNLESS (tty_l26 = BOOLEAN(!atcom_if_init()), error);
        CATCH(error) {
                printk(KERN_ERR"%s: ERROR\n", __FUNCTION__);
                if (tty_l26) tty_l26_exit();
                if (dun_if) dun_if_exit();
                if (obex_if) obex_if_exit();
                if (atcom_if) atcom_if_exit();
                otg_trace_invalidate_tag(TTY);
                return -EINVAL;
        }
        return 0;
}


/*! mcpc_l26_modexit - module cleanup
 *
 * This is called prior to the module being unloaded.
 */
STATIC void mcpc_l26_modexit (void)
{
        /* de-register as tty  and usb drivers */
        tty_l26_exit();

        /* de-register as function driver via acm-fd */
        atcom_if_exit();
        obex_if_exit();
        dun_if_exit();
        otg_trace_invalidate_tag(TTY);
}

module_init (mcpc_l26_modinit);
module_exit (mcpc_l26_modexit);

