/* 
 * drivers/usb/host/ehci-arc.c
 *
 * Copyright 2005 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Note: this file is #included by ehci-hcd.c */

#define DRIVER_INFO DRIVER_VERSION " " DRIVER_DESC
MODULE_DESCRIPTION("ARC EHCI USB Host Controller Driver");

#include <asm/hardware.h>
#include <asm/arch/arc_otg.h>

#undef dbg
#if 0
#define dbg printk
#else
#define dbg(fmt, ...) do{}while(0)
#endif

void arc_otg_reset(struct ehci_hcd *ehci)
{
	struct device *dev = ehci_to_hcd(ehci)->self.controller;
	struct arc_usb_config *config =
	    (struct arc_usb_config *)dev->platform_data;

	dbg("%s  ehci->regs=0x%p\n", __FUNCTION__, ehci->regs);
	dbg("%s  config=0x%p  config->name=%s\n", __FUNCTION__,
	    config, config->name);

	writel(USBMODE_CM_HOST, config->usbmode);

	dbg("%s(): set %s xcvr=0x%x  usbmode=0x%x portsc1 at 0x%p\n",
	    __FUNCTION__, config->name, config->xcvr_type,
	    config->usbmode, &ehci->regs->port_status[0]);
}

static const struct hc_driver ehci_driver = {
	.description = hcd_name,
	.hcd_priv_size = sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq = ehci_irq,
	.flags = HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset = ehci_hc_reset,
	.start = ehci_start,
#ifdef	CONFIG_PM
	.suspend = ehci_suspend,
	.resume = ehci_resume,
#endif
	.stop = ehci_stop,

	/*
	 * memory lifecycle (except per-request)
	 */
	.hcd_alloc = ehci_hcd_alloc,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = ehci_urb_enqueue,
	.urb_dequeue = ehci_urb_dequeue,
	.endpoint_disable = ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number = ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data = ehci_hub_status_data,
	.hub_control = ehci_hub_control,
	.hub_suspend = ehci_hub_suspend,
	.hub_resume = ehci_hub_resume,
};

static int __init arc_ehci_probe(struct device *dev)
{
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	int retval;
	struct arc_usb_config *config;
	struct platform_device *pdev = to_platform_device(dev);

	config = (struct arc_usb_config *)dev->platform_data;
	dbg("\n%s  dev=0x%p  config=0x%p  name=%s\n",
	    __FUNCTION__, dev, config, config->name);

	if (pdev->resource[0].flags != 0
	    || pdev->resource[1].flags != IORESOURCE_IRQ) {
		return -ENODEV;
	}

	dbg("ehci_driver.description=%s  platform_device.name=%s\n",
	    ehci_driver.description, pdev->name);

	/*
	 * do platform specific init: check the clock, grab/config pins, etc.
	 */
	if (config->platform_init() != 0) {
		retval = -EINVAL;
		goto err1;
	}

	hcd = usb_create_hcd(&ehci_driver);
	if (!hcd) {
		retval = 0;
		goto err1;
	}

	ehci = hcd_to_ehci(hcd);
	dev_set_drvdata(dev, ehci);

	dbg("%s: hcd=0x%p  ehci=0x%p\n", __FUNCTION__, hcd, ehci);

	hcd->regs = (void *)pdev->resource[0].start;
	hcd->irq = pdev->resource[1].start;

	hcd->self.controller = dev;
	hcd->self.bus_name = dev->bus_id;
	hcd->product_desc = "Freescale ARC_OTG";

	retval = hcd_buffer_create(hcd);
	if (retval != 0) {
		dev_dbg(dev, "pool alloc failed\n");
		goto err2;
	}

	INIT_LIST_HEAD(&hcd->dev_list);

	retval =
	    request_irq(hcd->irq, usb_hcd_irq, SA_INTERRUPT, pdev->name, hcd);
	if (retval != 0)
		goto err2;

	retval = usb_register_bus(&hcd->self);
	if (retval < 0)
		goto err3;

	retval = ehci_hc_reset(hcd);

	if (retval < 0)
		goto err4;

	if ((retval = ehci_start(hcd)) < 0) {
		goto err4;
	}

	return 0;
      err4:
	usb_deregister_bus(&hcd->self);
      err3:
	free_irq(hcd->irq, hcd);
      err2:
	usb_put_hcd(hcd);
      err1:
	printk("init error, %d\n", retval);
	return retval;
}

static int __init_or_module arc_ehci_remove(struct device *dev)
{
	struct ehci_hcd *ehci = dev_get_drvdata(dev);
	struct usb_hcd *hcd = ehci_to_hcd(ehci);
	struct arc_usb_config *config;

	config = (struct arc_usb_config *)dev->platform_data;

	dbg("%s  dev=0x%p\n", __FUNCTION__, dev);

	if (HCD_IS_RUNNING(hcd->state))
		hcd->state = USB_STATE_QUIESCING;

	usb_disconnect(&hcd->self.root_hub);

	hcd->driver->stop(hcd);

	hcd_buffer_destroy(hcd);

	usb_deregister_bus(&hcd->self);

	free_irq(hcd->irq, hcd);

	usb_put_hcd(hcd);

	/*
	 * do platform specific un-initialization:
	 * release iomux pins, etc.
	 */
	config->platform_uninit();

	return 0;
}

#define	arc_ehci_suspend	NULL
#define	arc_ehci_resume	NULL

#if defined(CONFIG_USB_EHCI_ARC_H1)
static struct device_driver arc_ehcih1_driver = {
	.name = (char *)"ehci_hcd_h1",
	.bus = &platform_bus_type,

	.probe = arc_ehci_probe,
	.remove = arc_ehci_remove,

	.suspend = arc_ehci_suspend,
	.resume = arc_ehci_resume,
};
#endif				/* CONFIG_USB_EHCI_ARC_H1 */

#if defined(CONFIG_USB_EHCI_ARC_H2)
static struct device_driver arc_ehcih2_driver = {
	.name = (char *)"ehci_hcd_h2",
	.bus = &platform_bus_type,

	.probe = arc_ehci_probe,
	.remove = arc_ehci_remove,

	.suspend = arc_ehci_suspend,
	.resume = arc_ehci_resume,
};
#endif				/* CONFIG_USB_EHCI_ARC_H2 */

#if defined(CONFIG_USB_EHCI_ARC_OTGHS)
static struct device_driver arc_ehci_otg_hs_driver = {
	.name = (char *)"ehci_hcd_otg_hs",
	.bus = &platform_bus_type,

	.probe = arc_ehci_probe,
	.remove = arc_ehci_remove,

	.suspend = arc_ehci_suspend,
	.resume = arc_ehci_resume,
};
#endif				/* CONFIG_USB_EHCI_ARC_OTGHS */

/*-------------------------------------------------------------------------*/

static int __init arc_ehci_init(void)
{
	int err;
	if (usb_disabled())
		return -ENODEV;

#if defined(CONFIG_USB_EHCI_ARC_H1)
	err = driver_register(&arc_ehcih1_driver);
	if (err)
		return err;
#endif				/* CONFIG_USB_EHCI_ARC_H1 */

#if defined(CONFIG_USB_EHCI_ARC_H2)
	err = driver_register(&arc_ehcih2_driver);
	if (err)
		return err;
#endif				/* CONFIG_USB_EHCI_ARC_H2 */

#if defined(CONFIG_USB_EHCI_ARC_OTGHS)
	err = driver_register(&arc_ehci_otg_hs_driver);
	if (err)
		return err;
#endif				/* CONFIG_USB_EHCI_ARC_OTGHS */

	return err;
}

static void __exit arc_ehci_exit(void)
{
	dbg("driver_unregister %s, %s\n", hcd_name, DRIVER_VERSION);

#if defined(CONFIG_USB_EHCI_ARC_H1)
	driver_unregister(&arc_ehcih1_driver);
#endif

#if defined(CONFIG_USB_EHCI_ARC_H2)
	driver_unregister(&arc_ehcih2_driver);
#endif

#if defined(CONFIG_USB_EHCI_ARC_OTGHS)
	driver_unregister(&arc_ehci_otg_hs_driver);
#endif

}

MODULE_DESCRIPTION(DRIVER_INFO);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
module_init(arc_ehci_init);
module_exit(arc_ehci_exit);
