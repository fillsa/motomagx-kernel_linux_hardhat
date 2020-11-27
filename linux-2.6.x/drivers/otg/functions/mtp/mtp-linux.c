/*
 * Copyright (c) 2006-2008 Motorola, Inc, All Rights Reserved.
 *
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   *  Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   *  Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   *  Neither the name of Motorola nor the names of its contributors may
 *      be used to endorse or promote products derived from this software without
 *      specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 03/28/2006        Motorola      MTP initial
 * 04/26/2006        Motorola      fix MTP ctl message header
 * 06/06/2006        Motorola      PictBridge support
 * 08/08/2006        Motorola      Move MTP descriptors to user space
 * 09/06/2006        Motorola      MTP HS fix
 * 11/14/2008        Motorola      Clean the work queue before module exit.
 * 
 *
 * 
 */

/*!
 * @file otg/functions/mtp/mtp-linux.c
 * @brief MTP module
 *
 * This is a MTP Class Function
 *
 * To use simply load with something like:
 *
 *      insmod mtp_fd.o vendor_id=0xffff product_id=0xffff
 *
 * @ingroup MTPFunction
 */


#include <otg/otg-compat.h>
#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <public/otg-node.h>
#include <otg/otg-linux.h>

#include "mtp.h"

/*!
 * MTP
 *
 * Endpoint, Class, Interface, Configuration and Device descriptors/descriptions
 */

/*! Endpoint Index Lists
 */
static u8 mtp_endpoint_index[] = { BULK_OUT, BULK_IN, INT_IN, };

/*! Endpoint Request List
 */
static struct usbd_endpoint_request mtp_endpoint_requests[ENDPOINTS+1] = {
        { BULK_OUT, 1, 0, 0, USB_DIR_OUT | USB_ENDPOINT_BULK, MTP_RECV_URB_MAX_SIZE, MTP_RECV_URB_MAX_SIZE, 0, },
        { BULK_IN,  1, 0, 0, USB_DIR_IN  | USB_ENDPOINT_BULK, MTP_RECV_URB_MAX_SIZE, MTP_RECV_URB_MAX_SIZE, 0, },
        { INT_IN,   1, 0, 0, USB_DIR_IN  | USB_ENDPOINT_INTERRUPT, 64, 64, 10, },
        { 0, },
};

/*! Comm alternate descriptions 
 */
static struct usbd_alternate_description mtp_alternate_descriptions[] = {
    {
        .iInterface = "MTP",
        .bInterfaceClass = MTP_CLASS_IMAGE_INTERFACE,
        .bInterfaceSubClass =  MTP_SUBCLASS_STILL_IMAGE_CAPTURE_DEVICE,
        .bInterfaceProtocol = MTP_PROTOCOL_CODE,
        .endpoints = sizeof (mtp_endpoint_index) / sizeof(u8),
        .endpoint_index =  mtp_endpoint_index,
    },
};

/*! Interfaces
 */
struct usbd_interface_description mtp_interfaces[] = {
    { alternates:sizeof (mtp_alternate_descriptions) / sizeof (struct usbd_alternate_description),
      alternate_list:mtp_alternate_descriptions,},
};

/* ********************************************************************************************* */
/* ********************************************************************************************* */

extern struct usbd_function_operations mtp_function_ops;

/*! function_driver 
 */
struct usbd_interface_driver mtp_interface_driver = {
    .driver.name = "mtp-if",
    .driver.fops = &mtp_function_ops,
    .interfaces = sizeof (mtp_interfaces) / sizeof (struct usbd_interface_description),
    .interface_list = mtp_interfaces,
    .endpointsRequested =  ENDPOINTS,
    .requestedEndpoints =  mtp_endpoint_requests,
};

/* Module Parameters ************************************************************************* */

MOD_PARM_INT (vendor_id, "Device Vendor ID", 0);
MOD_PARM_INT (product_id, "Device Product ID", 0);
MOD_PARM_INT (major, "Device Major", 0);
MOD_PARM_INT (minor, "Device Minor", 0);
MOD_PARM_INT (ext_id, "ID of Vendor Extended Descriptor", 0);
MOD_PARM_STR (ext_desc, "Vendor Extended Descriptor", NULL);
MOD_PARM_INT (ext_str_idx, "Extended String Index", 0);
MOD_PARM_STR (ext_str_desc, "Extended String Descriptor", NULL);

extern int mtp_ext_id;
extern char *mtp_ext_desc;
extern int mtp_ext_str_idx;
extern char *mtp_ext_str_desc;

/* USB Module init/exit ***************************************************** */

#ifdef LINUX26
struct workqueue_struct * mtp_if_workqueue = NULL;
#endif

/*! mtp_modinit - module init
 *
 */
static int mtp_modinit (void)
{
    int rc;

    printk (KERN_INFO "%s vendor_id: %04x product_id: %04x major: %d minor: %d\n", __FUNCTION__, 
            MODPARM(vendor_id), MODPARM(product_id), MODPARM(major), MODPARM(minor));

#ifdef LINUX26
    mtp_if_workqueue = create_singlethread_workqueue("mtp-if");
    mtp_fd_init(mtp_if_workqueue);
#endif

    MTP = otg_trace_obtain_tag();
    mtp_if_name = MTP_AGENT;

    mtp_ext_id = MODPARM(ext_id);
    mtp_ext_desc = MODPARM(ext_desc);

    mtp_ext_str_idx = MODPARM(ext_str_idx);
    mtp_ext_str_desc = MODPARM(ext_str_desc);

    /* register as usb function driver */
    RETURN_EINVAL_IF(usbd_register_interface_function (&mtp_interface_driver, "mtp-if", NULL));

    CATCH(error) {
        otg_trace_invalidate_tag(MTP);
        return -EINVAL;
    }

    return 0;
}

/*! mtp_modexit - module cleanup
 */
static void mtp_modexit (void)
{
    printk (KERN_INFO "%s\n", __FUNCTION__);

#ifdef LINUX26
    mtp_fd_exit();
    mtp_if_workqueue = NULL;
#endif

    usbd_deregister_interface_function (&mtp_interface_driver);

    otg_trace_invalidate_tag(MTP);
}

MODULE_LICENSE("Dual BSD/GPL");
module_init (mtp_modinit);
module_exit (mtp_modexit);

