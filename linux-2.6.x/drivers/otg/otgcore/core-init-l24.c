/*
 * otg/otg/core-init-l24.c - OTG Peripheral Controller Driver Module Initialization
 * @(#) balden@seth2.belcarratech.com|otg/otgcore/core-init-l24.c|20051116205002|34325
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcara.com>, 
 *      Bruce Balden <balden@belcara.com>
 *
 * Copyright (c) 2005-2008 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 08/01/2005         Motorola         Initial distribution
 * 08/22/2005         Motorola         OS -PM sleep mode
 * 09/26/2005         Motorola         changed flag name from USB_INACTIVE to MPM_USB_INACTIVE
 *                                         added the USB bus to platform_bus_type.
 * 09/29/2005         Motorola         added a release function to avoid an unwanted printk in rmmod.
 * 10/07/2005         Motorola         changed flag name from MPM_USB_INACTIVE to PM_USB_INACTIVE.
 *                                         added a function pointer 'pm_iomux_config' to configure USB XCVR
 * 03/01/2006         Motorola         changed PM_USB_INACTIVE from bool to u8. Added checks for OS_SUSPENDED
 * 08/09/2006         Motorola         NFS over USB     
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 * 11/23/2007         Motorola         Changes for OSS compliance.
 * 08/08/2008         Motorola         Commented out OTG debug from dmesg
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
 * @file otg/otgcore/core-init-l24.c
 * @brief OTG Core Linux initialization.
 *
 * This file is the starting point for defining the Linux OTG Core 
 * driver. It references and starts all of the other components that
 * must be "linked" into the OTGCORE mdoule.
 *
 * @ingroup OTGCore
 */


#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <otg/otg-pcd.h>
#include <otg/otg-ocd.h>

EMBED_LICENSE(); // When supported by the OS, embed license information in the binary

#ifdef OTG_MALLOC_TEST
int otg_mallocs;
#endif /* OTG_MALLOC_TEST */

otg_tag_t CORE;
int usbd_procfs_init (void);
void usbd_procfs_exit (void);
struct workqueue_struct * otg_workqueue;
struct workqueue_struct * otg_wqrs;

/* Tables for OTG firmware
 */
#if defined(CONFIG_OTG_USB_PERIPHERAL) || defined (CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)
struct otg_firmware *otg_firmware_loaded = &otg_firmware_mn;
struct otg_firmware *otg_firmware_orig = &otg_firmware_mn;
#elif defined(CONFIG_OTG_BDEVICE_WITH_SRP) || defined(CONFIG_OTG_DEVICE) 
struct otg_firmware *otg_firmware_loaded = &otg_firmware_df;
struct otg_firmware *otg_firmware_orig = &otg_firmware_df;
#else /* error */
#abort "Missing USB or OTG configuration"
#endif


struct otg_firmware *otg_firmware_loading;
struct otg_instance otg_instance_private;

struct hcd_instance hcd_instance_private = { otg: &otg_instance_private, };
struct ocd_instance ocd_instance_private = { otg: &otg_instance_private, };
struct pcd_instance pcd_instance_private = { otg: &otg_instance_private, };
struct tcd_instance tcd_instance_private = { otg: &otg_instance_private, };

struct otg_instance otg_instance_private = {
        hcd: &hcd_instance_private,
        ocd: &ocd_instance_private,
        pcd: &pcd_instance_private,
        tcd: &tcd_instance_private,
};

struct otg_instance *otg_instance = &otg_instance_private;

int (*pm_iomux_config)(void);
u8 PM_USB_INACTIVE;

char * otg_get_state_name(int state)
{
        struct otg_state *otg_state;
        if (!otg_firmware_loaded || (state >= otg_firmware_loaded->number_of_states))
                return "UNKNOWN_STATE";

        otg_state = otg_firmware_loaded->otg_states + state;
        return otg_state->name;
}


/* ************************************************************************************* */



static int otg_match (struct device *dev, struct device_driver *drv)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        #if 0
        struct usb_interface *intf;
        struct usb_driver *usb_drv;
        const struct usb_device_id *id;

        /* check for generic driver, which we don't match any device with */
        if (drv == &usb_generic_driver)
                return 0;

        intf = to_usb_interface(dev);
        usb_drv = to_usb_driver(drv);

        id = usb_match_id (intf, usb_drv->id_table);
        if (id)
                return 1;

        #endif
        return 0;
}

static int otg_hotplug (struct device *dev, char **envp, int num_envp, char *buffer, int buffer_size)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return 0;
}

static int otg_suspend(struct device *dev, u32 state)
{       
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return 0;
}               
        
static int otg_resume(struct device *dev)
{               
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return 0;
}               


/***************************************************************************
The usb_suspend and usb_resume routines will be called by the OS PM.
usb_suspend checks if USB has already been suspended by its host. 
****************************************************************************/

static int usb_suspend(struct device *dev, u32 state, u32 level)
{
    if(PM_USB_INACTIVE & USB_SUSPENDED) 
    {   
       PM_USB_INACTIVE |= OS_SUSPENDED;
       TRACE_MSG0(CORE,"USB INACTIVE: Allowing OS to SUSPEND");
       return 0; //allow OS to go to DSM
    }
    //else reject the suspend
    TRACE_MSG0(CORE,"USB ACTIVE: OS not allowed to SUSPEND");
    return -EBUSY;
}
                      
static int usb_resume(struct device *dev, u32 level)
{
    TRACE_MSG0(CORE,"USB RESUME CALLED");
                                   
/* This function is called to ensure that the XCVR iomuxes are 
 * configured to the right settings after a low power mode. 
 * This is becuase it is seen that the iomuxes are deconfigured 
 * after the OS goes to low power mode.
 */
   
   PM_USB_INACTIVE &= ~OS_SUSPENDED;
   if(pm_iomux_config) 
   {
      TRACE_MSG0(CORE,"TCD MOD INIT CALLED");
      pm_iomux_config();
   }   
   else
   {
      TRACE_MSG0(CORE,"FUNC TCD MOD NOT CALLED");
   }
                                      
   TRACE_MSG0(CORE,"USB RESUME DONE!!");
   return 0; //Allow OS to resume safely
}

static void otg_release(struct device *dev)
{
}

struct bus_type otg_bus_type = {
        .name =         "otg",
        .match =        otg_match,
        .hotplug =      otg_hotplug,
        .suspend =      otg_suspend,
        .resume =       otg_resume,
};

static int __init_or_module
otg_remove(struct device *dev)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return 0;
}


static int __init
otg_probe(struct device *dev)
{               
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return 0;
}

static struct device_driver otg_driver = {

        .owner =        THIS_MODULE,
        .name =         "otg",
        .bus =          &platform_bus_type,
//      .bus =          &otg_bus_type,
        .probe =        otg_probe,
        .remove =       otg_remove,
        .suspend =      usb_suspend,
        .resume =       usb_resume,
//        .scale =        NULL,
};
            
            
/**********************************************************************
  adding platform_device so that OS can call functions from its list
***********************************************************************/
static struct platform_device otg_device = {
       .name =     "otg",
       .id =       0,
       .dev = {
              .release = otg_release,
       },
};

OTG_EXPORT_SYMBOL(otg_bus_type);
OTG_EXPORT_SYMBOL(pm_iomux_config);



/* ************************************************************************************* */

/* otg_modinit - linux module initialization
 */
static int otg_modinit (void)
{
        int bus_registered = 0;
        int driver_registered = 0;
        int message_init = 0;
        int message_init_l24 = 0;
        int trace_init = 0;
        int trace_init_l24 = 0;
        int platform_registered = 0;

        PM_USB_INACTIVE = ~(USB_SUSPENDED | OS_SUSPENDED);
        pm_iomux_config = NULL;

        printk(KERN_INFO"%s: \n", __FUNCTION__);
	otg_workqueue = create_workqueue("otg_work");
	//printk("workqueue otg_work is created otg_workqueue value is 0x%x.\n", otg_workqueue);
	otg_wqrs = create_workqueue("otg_wqrs");

        THROW_IF((bus_registered = bus_register(&otg_bus_type)), error);
        THROW_IF((driver_registered = driver_register(&otg_driver)), error);
        THROW_IF((platform_registered = platform_device_register(&otg_device)),error);


        trace_init = otg_trace_init();
        trace_init_l24 = otg_trace_init_l24();

        CORE = otg_trace_obtain_tag();

        otg_instance_private.function_name[0] = '\0';

        TRACE_MSG0(CORE,"--");
        otg_instance->current_outputs = otg_firmware_loaded->otg_states;
        otg_instance->previous_outputs = otg_firmware_loaded->otg_states;

        /* initialize otg-mesg and usbp 
         */
        THROW_IF((message_init = otg_message_init(otg_instance)), error);
        THROW_IF((message_init_l24 = otg_message_init_l24(otg_instance)), error);
        THROW_IF(usbd_device_init(), error);
        THROW_IF(usbd_procfs_init (), error);

        otg_set_ocd_ops(NULL);
        otg_set_tcd_ops(NULL);
        otg_set_hcd_ops(NULL);
        otg_set_pcd_ops(NULL);

        CATCH(error) {
                CORE = otg_trace_invalidate_tag(CORE);
                if (trace_init_l24 )otg_trace_exit_l24();
                if (trace_init) otg_trace_exit();
                if (message_init_l24 )otg_message_exit_l24();
                if (message_init) otg_message_exit();
                if (driver_registered) driver_unregister(&otg_driver);
                if (bus_registered) bus_unregister(&otg_bus_type);
                return -EINVAL;
        }
        return 0;
}
#ifdef CONFIG_OTG_NFS
late_initcall (otg_modinit);
#else
module_init (otg_modinit);
#endif


#if OTG_EPILOGUE  /* Set nonzero in <otg-module.h> when -DMODULE is in force */
/* otg_modexit - This is *only* used for drivers compiled and used as a module.
 */
static void otg_modexit (void)
{
        printk(KERN_INFO"%s: \n", __FUNCTION__);

        usbd_procfs_exit ();
	usbd_device_exit();
        otg_message_exit_l24();

        CORE = otg_trace_invalidate_tag(CORE);
	otg_trace_exit_l24();
	otg_trace_exit();

        if (otg_firmware_loading) {
                lkfree(otg_firmware_loading->otg_states);
                lkfree(otg_firmware_loading->otg_tests);
                lkfree(otg_firmware_loading);
        }
        if (otg_firmware_loaded && (otg_firmware_loaded != otg_firmware_orig)) {
                lkfree(otg_firmware_loaded->otg_states);
                lkfree(otg_firmware_loaded->otg_tests);
                lkfree(otg_firmware_loaded);
        }

        platform_device_unregister(&otg_device);
        driver_unregister(&otg_driver);
        bus_unregister(&otg_bus_type);
	destroy_workqueue(otg_workqueue);
	destroy_workqueue(otg_wqrs);

}
#endif

MOD_EXIT(otg_modexit);
OTG_EXPORT_SYMBOL(otg_get_state_name);
OTG_EXPORT_SYMBOL(otg_workqueue);
OTG_EXPORT_SYMBOL(otg_wqrs);
#ifdef OTG_MALLOC_TEST
OTG_EXPORT_SYMBOL(otg_mallocs);
#endif /* OTG_MALLOC_TEST */

