/*
 * otg/functions/generic/generic-cl.c
 * @(#) sl@whiskey.enposte.net|otg/functions/generic/generic-cl.c|20051116230343|27989
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2006 Motorola, Inc. 
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 03/16/2006         Motorola         Initial distribution
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
 * @file otg/functions/generic/generic-cl.c
 * @brief Generic Class Function Driver 
 *
 * This implements a generic NULL class function driver.
 *
 * @ingroup GenericFunction
 */

#include <linux/config.h>
#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#include "generic-cf.h"

/* Module Parameters ******************************************************** */
/* ! 
 * @name XXXXX MODULE Parameters
 */
/* ! @{ */

MOD_AUTHOR ("sl@belcarra.com"); 
EMBED_LICENSE();
MOD_DESCRIPTION ("Belcarra Generic NULL Class Function");

otg_tag_t GCLASS;

/* ! *} */

/* ********************************************************************************************* */
/*!     
 * generic_cl_device_request - called to process a request to endpoint or interface
 * @param function
 * @param request
 * @return non-zero for failure, will cause endpoint zero stall
 */
static int generic_cl_device_request (struct usbd_function_instance *function, struct usbd_device_request *request)
{
        TRACE_MSG5(GCLASS, "bmRequestType: %02x bRequest: %02x wValue: %04x wIndex: %04x wLength: %04x",
                        request->bmRequestType, request->bRequest, request->wValue,
                        request->wIndex, request->wLength);
        
	return -EINVAL;
}
/* ********************************************************************************************* */
#if !defined(OTG_C99)
/*! function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations generic_function_ops;

/*! generic_class_driver - USB Device Core function driver definition
 */
struct usbd_class_driver generic_class_driver;

#else /* defined(OTG_C99) */
/*! function_ops - operations table for the USB Device Core
 */
static struct usbd_function_operations generic_function_ops = {
        .device_request = generic_cl_device_request,           /*!< called for each received device request */
};

/*! generic_class_driver - USB Device Core function driver definition
 */
struct usbd_class_driver generic_class_driver = {
        .driver.name = "generic-class",                            /*!< driver name */
        .driver.fops = &generic_function_ops,                             /*!< operations table */
};
#endif /* defined(OTG_C99) */


/* USB Module init/exit ***************************************************** */

//#if OTG_EPILOGUE
/*! 
 * generic_cl_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded 
 * if compiled as a module. This function is never called if the 
 * driver is linked into the kernel.
 * @return none
 */
static void generic_cl_modexit (void)
{
        usbd_deregister_class_function (&generic_class_driver);
        otg_trace_invalidate_tag(GCLASS);
}

module_exit (generic_cl_modexit);
//#endif

/*! 
 * generic_cl_modinit() - module init
 *
 * This is called by the Linux kernel; either when the module is loaded
 * if compiled as a module, or during the system intialization if the 
 * driver is linked into the kernel.
 *
 * This function will parse module parameters if required and then register
 * the generic driver with the USB Device software.
 *
 */
static int generic_cl_modinit (void)
{
        #if !defined(OTG_C99)
        /*! function_ops - operations table for the USB Device Core
         */
        ZERO(generic_function_ops);
        generic_function_ops.device_request=generic_cl_device_request;          /*! called for each received device request */
	
        /*! class_driver - USB Device Core function driver definition
         */
        ZERO(generic_class_driver);
        generic_class_driver.driver.name = "generic-class";                            /*! driver name */
        generic_class_driver.driver.fops = &generic_function_ops;                             /*! operations table */
        #endif /* defined(OTG_C99) */
        
        GCLASS = otg_trace_obtain_tag();

        // register as usb function driver
	TRACE_MSG0(GCLASS, "REGISTER CLASS");
        THROW_IF (usbd_register_class_function (&generic_class_driver, "generic-class", NULL), error);

	TRACE_MSG0(GCLASS, "REGISTER FINISHED");

        CATCH(error) {
                generic_cl_modexit();
                return -EINVAL;
        }
        return 0;
}

#ifdef CONFIG_OTG_NFS
late_initcall (generic_cl_modinit);
#else
module_init (generic_cl_modinit);
#endif

