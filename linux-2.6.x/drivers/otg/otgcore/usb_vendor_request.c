/*
 * Copyright 2005-2006 Motorola, Inc. All Rights Reserved
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 03/27/2006         Motorola         USB vendor specific command
 * 05/11/2006         Motorola         USB enumeration not working
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 *
 *  This program is licensed under a BSD license with the following terms:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 *  Neither the name of Motorola nor the names of its contributors may be
 *  used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* OS-specific #includes */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/ctype.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/utsname.h>
#include <linux/kmod.h>
#include <asm/uaccess.h>
#include <asm/system.h>

/* OTG specific */
#include <otg/otg-compat.h>
#include <otg/otg-module.h>
#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-utils.h>
#include <otg/otg-trace.h>
#include <public/otg-node.h>

#include <otg/hotplug.h>

#define USB_VD_SELECT_CFG       0x01    /* select configuration for next
                                           enumeration process */

static struct usb_hotplug_private usb_vendor_hotplug;

static int  usb_vendor_usb_config;

static char usbconfig[40];



static int usb_vendor_hotplug_callback(struct usb_hotplug_private * gen_priv)
{
   
   sprintf (usbconfig, "USBCONFIG=%d",usb_vendor_usb_config );

   TRACE_MSG1(USBD, "USB config value = %s",usbconfig);

   return call_hotplug(gen_priv,1,usbconfig);
}



int usb_vendor_request(struct usbd_device_request *request,
		               struct usbd_function_instance *function_instance)
{
   u8 bmRequestType = request->bmRequestType;
   u16 wValue = le16_to_cpu(request->wValue);
   u16 wIndex = le16_to_cpu(request->wIndex);
   u16 wLength = le16_to_cpu(request->wLength);

   TRACE_MSG4(USBD, "bRequest = %x wIndex = %x wLength = %x wValue = %x", 
				      request->bRequest,wIndex,wLength,wValue);
      
 
   // Verify that this is a USB Class request per CDC
   // specification or a vendor request.
   RETURN_EINVAL_UNLESS( request->bmRequestType &  USB_REQ_TYPE_VENDOR);
	

   // Determine the request direction and process accordingly
   switch (request->bmRequestType & (USB_REQ_DIRECTION_MASK | USB_REQ_TYPE_MASK))	
   { 

   case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_VENDOR:

	   switch (request->bRequest)
	   {

	   case USB_VD_SELECT_CFG:
		       
	       if ((wValue == 0) && (wLength == 0))
	       {
			   if( (wIndex == 0x01) || (wIndex == 0x0b) || (wIndex == 0x0d) ||
					   (wIndex == 0x0e) )
			   {
				   //printk("\nwIndex = %04x",wIndex);

                   usb_vendor_hotplug.function_instance = function_instance;

				   //agent script name usb_vendor.agent
                   usb_vendor_hotplug.dev_name = USB_VENDOR_AGENT;

				   hotplug_init(&usb_vendor_hotplug);

                   // hotplug_init should be called before assigning call back
                   usb_vendor_hotplug.hotplug_callback = usb_vendor_hotplug_callback;
 
				   usb_vendor_hotplug.hotplug_status = hotplug_detached;
				   usb_vendor_usb_config = wIndex;
				   usb_vendor_hotplug.flags 	= HOTPLUG_FUNC_ENABLED;

				   generic_os_hotplug(&usb_vendor_hotplug);

				   return 0;

				   
			   }
		   }

       return -EINVAL; 
				
	   default:
		   return -EINVAL;  
			   
	   }

   default:
	   return -EINVAL;  
   }

   return -EINVAL;
}
  
	 
    		 
					


		         
		         

