/* @(#) balden@seth2.belcarratech.com|otg/ocd/mxc/hcd-test.c|20051116203403|17009
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

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <linux/usb.h>
#include <linux/delay.h>


#include <core/hcd.h>
#include <asm/arch/board.h>
#include <asm/memory.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/* ********************************************************************************************* */
/* ********************************************************************************************* */

#define MAX_HUB_PORTS 3

struct test_hcd {
        struct usb_hcd          hcd;
        struct device           *dev;

        u32                     root_hub_desc_a;
        u32                     root_hub_desc_b;
        u8                      bNbrPorts;
        u8                      wHubCharacteristics;
        u8                      bPwrOn2PwrGood;
        u8                      DeviceRemovable;
        u8                      PortPwrCtrlMask;
};

struct usbp_hub_descriptor {
        __u8  bDescLength;
        __u8  bDescriptorType;
        __u8  bNbrPorts;
        __u16 wHubCharacteristics;
        __u8  bPwrOn2PwrGood;
        __u8  bHubContrCurrent;
                /* add 1 bit for hub status change; round to bytes */
        __u8  DeviceRemovable;
        __u8  PortPwrCtrlMask;
} __attribute__ ((packed));



static inline struct test_hcd *hcd_to_mxc(struct usb_hcd *hcd)
{               
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return container_of(hcd, struct test_hcd, hcd);
}               


int test_hcd_urb_enqueue( struct usb_hcd *hcd, struct urb *urb, int mem_flags)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return 0;
}


int test_hcd_urb_dequeue(struct usb_hcd *hcd, struct urb *urb)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return -EINVAL;
}

void test_hcd_endpoint_disable(struct usb_hcd *hcd, struct hcd_dev *hdev, int epnum)
{
        //printk(KERN_INFO"%s:\n", __FUNCTION__); 
}

int test_rh_frame(struct usb_hcd *hcd)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return 0;
}

int test_rh_status_data(struct usb_hcd *hcd, char *buf)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        *buf = 0;
        return 0;
}

static void
test_rh_descriptor ( struct test_hcd *test_hcd, struct usbp_hub_descriptor *desc) 
{
        u16             temp = 0;

        desc->bDescLength = 9;
        desc->bDescriptorType = 0x29;

        desc->bNbrPorts = test_hcd->bNbrPorts;
        desc->wHubCharacteristics = test_hcd->wHubCharacteristics;
        desc->bPwrOn2PwrGood = test_hcd->bPwrOn2PwrGood;
        desc->bHubContrCurrent = 0;
        desc->DeviceRemovable = test_hcd->DeviceRemovable;
        desc->PortPwrCtrlMask = test_hcd->PortPwrCtrlMask;
}


int test_rh_control( struct usb_hcd  *hcd, u16 typeReq, u16 wValue, u16 wIndex, char *buf, u16 wLength)
{
        struct test_hcd  *test_hcd = hcd_to_mxc(hcd);

        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        switch (typeReq) {
        case ClearHubFeature:
                printk(KERN_INFO"%s: ClearHubFeature\n", __FUNCTION__); 
                return 0;

        case SetHubFeature:
                printk(KERN_INFO"%s: SetHubFeature\n", __FUNCTION__); 
                return 0;

        case SetPortFeature: {
                printk(KERN_INFO"%s: SetPortDescriptor\n", __FUNCTION__); 
                return 0;
        }
        case ClearPortFeature: {
                printk(KERN_INFO"%s: ClearPortDescriptor\n", __FUNCTION__); 
        }

        case GetHubDescriptor:
                printk(KERN_INFO"%s: GetHubDescriptor\n", __FUNCTION__); 
                test_rh_descriptor(test_hcd, (struct usbp_hub_descriptor *) buf);
                return 0;

        case GetHubStatus:
                printk(KERN_INFO"%s: GetHubStatus\n", __FUNCTION__); 
                *(__le32 *) buf = 0;
                return 0;

        case GetPortStatus:
                printk(KERN_INFO"%s: GetPortStatus\n", __FUNCTION__); 
                *(__le32 *) buf = 0; 
                return 0;

        default:
                printk(KERN_INFO"%s: Default\n", __FUNCTION__); 
                return -EPIPE;
        }
}

int test_rh_suspend(struct usb_hcd *hcd)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return 0;
}

int test_rh_resume(struct usb_hcd *hcd)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        return 0;
}


static struct hc_driver test_hc_driver = {
        .description =          "mxc-hcd",
        .flags =                HCD_USB11,
        .urb_enqueue =          test_hcd_urb_enqueue,
        .urb_dequeue =          test_hcd_urb_dequeue,
        .endpoint_disable =     test_hcd_endpoint_disable,
        .get_frame_number =     test_rh_frame,
        .hub_status_data =      test_rh_status_data,
        .hub_control =          test_rh_control,
        .hub_suspend =          test_rh_suspend,
        .hub_resume =           test_rh_resume,
};              
        
static void
test_hcd_stop(struct usb_hcd *hcd)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__); 
        del_timer_sync(&hcd->rh_timer);
}

static int __init_or_module
test_hcd_remove(struct device *dev)
{
        struct test_hcd          *test_hcd = NULL;

        printk(KERN_INFO"%s: \n", __FUNCTION__); 

        if((!(test_hcd = dev_get_drvdata(dev))))
		return -EINVAL;

        dev_set_drvdata(dev, NULL);

        printk(KERN_INFO"%s: usb_disconnect\n", __FUNCTION__); 
        usb_disconnect(&test_hcd->hcd.self.root_hub);

        printk(KERN_INFO"%s: usb_deregister_bus\n", __FUNCTION__); 
        usb_deregister_bus(&test_hcd->hcd.self);

        printk(KERN_INFO"%s: test_hcd_stop\n", __FUNCTION__); 
        test_hcd_stop(&test_hcd->hcd);

        printk(KERN_INFO"%s: kfree should free test_hcd: %x\n", __FUNCTION__, test_hcd); 
        kfree(test_hcd);
        printk(KERN_INFO"%s: finished\n", __FUNCTION__); 

        return 0;
}

static int
test_hcd_start(struct usb_hcd *hcd)
{
        struct usb_device       *udev = NULL;

        printk(KERN_INFO"%s:\n", __FUNCTION__);

        if (!(udev = usb_alloc_dev(NULL, &hcd->self, 0)))
		return -ENOMEM;

        udev->speed = USB_SPEED_FULL;
        hcd->state = USB_STATE_RUNNING;

        if (!hcd_register_root(udev, hcd)) {

                printk(KERN_INFO"%s: finished\n", __FUNCTION__); 
		return 0;
        }

	usb_put_dev(udev);
	test_hcd_stop(hcd);

	return -ENODEV;
}

void usb_hcd_release(struct usb_bus *bus);
static void test_hcd_release(struct device *dev)
{
        printk(KERN_INFO"%s: \n", __FUNCTION__);
        usb_hcd_release((struct usb_bus *) dev);
}


static int __init
test_hcd_probe(struct device *dev)
{               
        struct platform_device  *platform_device = NULL;
        struct test_hcd          *test_hcd = NULL;
        int                     i;

        printk(KERN_INFO"%s:\n", __FUNCTION__); 

        dev->release = test_hcd_release;

        if (!(test_hcd = kmalloc(sizeof (struct test_hcd), GFP_KERNEL))) {
                printk(KERN_INFO"%s: kmalloc failed\n", __FUNCTION__); 
		return -ENOMEM;
        }

        printk(KERN_INFO"%s: test_hcd: %x\n", __FUNCTION__, test_hcd); 

	memset(test_hcd, sizeof(struct test_hcd), 0);

        platform_device = container_of(dev, struct platform_device, dev);
        dev_set_drvdata(dev, test_hcd);

        usb_bus_init(&test_hcd->hcd.self);
        test_hcd->hcd.self.controller = dev;
        test_hcd->hcd.self.bus_name = dev->bus_id;
        test_hcd->hcd.self.op = &usb_hcd_operations;
        test_hcd->hcd.self.hcpriv = test_hcd;

        test_hcd->hcd.self.is_b_host = 1;
        test_hcd->hcd.self.otg_port = 1;
        test_hcd->root_hub_desc_a = 0x01000103;
        test_hcd->root_hub_desc_b = 0x00070000;

        test_hcd->bNbrPorts = test_hcd->root_hub_desc_a & 0xff;
        test_hcd->wHubCharacteristics = (test_hcd->root_hub_desc_a >> 8) & 0xff;
        test_hcd->bPwrOn2PwrGood = (test_hcd->root_hub_desc_a >> 24) & 0xff;
        test_hcd->DeviceRemovable = test_hcd->root_hub_desc_b & 0xff;
        test_hcd->PortPwrCtrlMask = (test_hcd->root_hub_desc_b >> 16) & 0xff;


        INIT_LIST_HEAD(&test_hcd->hcd.dev_list);
        test_hcd->hcd.self.release = &usb_hcd_release;

        test_hcd->hcd.description = test_hc_driver.description;
        test_hcd->hcd.product_desc = "TEST HCD";

        init_timer(&test_hcd->hcd.rh_timer);
        test_hcd->hcd.driver = &test_hc_driver;
        test_hcd->hcd.irq = -1;
        test_hcd->hcd.state = USB_STATE_HALT;


	if (!usb_register_bus(&test_hcd->hcd.self) && !test_hcd_start(&test_hcd->hcd)) {

                printk(KERN_INFO"%s: usb_register_bus ok\n", __FUNCTION__); 
		return 0;
        }

        printk(KERN_INFO"%s: usb_register_bus failed\n", __FUNCTION__); 
        test_hcd_stop(&test_hcd->hcd);
        kfree(test_hcd);
	return -ENODEV;
}

static int
test_hcd_suspend(struct device *dev, u32 state, u32 phase)
{       
	printk(KERN_INFO"%s:\n", __FUNCTION__); 
	return -EINVAL;
}               

static int
test_hcd_resume(struct device *dev, u32 phase)
{               
	printk(KERN_INFO"%s:\n", __FUNCTION__); 
	return -EINVAL;
}               

static struct device_driver test_hcd_driver = {

	.name =         "hcd-test",
	.bus =          &platform_bus_type,

	.probe =        test_hcd_probe,
	.remove =       test_hcd_remove,

	.suspend =      test_hcd_suspend,
	.resume =       test_hcd_resume,

};

static struct platform_device test_hcd_platform = {
	.name       = "hcd-test",
	.id         = 1,
};

int test_hcd_mod_init_l26(void)
{
	u32 hwmode;
	int driver_registered = 0;
	int platform_registered = 0;
	struct proc_dir_entry *pde = NULL;

	printk(KERN_INFO"%s: TEST-HCD.C)\n", __FUNCTION__); 

	if (usb_disabled()) {
		printk(KERN_INFO"%s: usb_disabled\n", __FUNCTION__); 
		return -ENOMEM;
	}

	if ((driver_registered = driver_register(&test_hcd_driver))) {

		printk(KERN_INFO"%s: driver_register failed\n", __FUNCTION__); 
		return -EINVAL;
	}

	if ((platform_registered = platform_registered = platform_device_register(&test_hcd_platform))) {

		printk(KERN_INFO"%s: platform_device_register failed\n", __FUNCTION__); 
		return -EINVAL;
	}

        return 0;
}

#ifdef MODULE
void test_hcd_mod_exit_l26(void)
{
        printk(KERN_INFO"%s: \n", __FUNCTION__); 

        printk(KERN_INFO"%s: driver_unregister\n", __FUNCTION__); 
        driver_unregister(&test_hcd_driver);

        printk(KERN_INFO"%s: platform_device_unregister \n", __FUNCTION__); 
        platform_device_unregister(&test_hcd_platform);

        printk(KERN_INFO"%s: finished\n", __FUNCTION__); 
		
}
#endif
#ifdef CONFIG_OTG_NFS
late_initcall (test_hcd_mod_init_l26);
#else
module_init (test_hcd_mod_init_l26);
#endif
module_exit (test_hcd_mod_exit_l26);
MODULE_LICENSE ("GPL");

