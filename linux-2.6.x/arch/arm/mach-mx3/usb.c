/*
 * arch/arm/mach-mx3/usb.c -- platform level USB initialization
 *
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/usb_otg.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/mach-types.h>

#include <asm/arch/board.h>
#include <asm/arch/arc_otg.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>	/* for clock, pll stuff */

#define PBC3_CLEAR	(PBC_BASE_ADDRESS + PBC_BCTRL3_CLEAR)
#define PBC3_SET	(PBC_BASE_ADDRESS + PBC_BCTRL3_SET)

#undef DEBUG
#ifdef DEBUG
#define dbg(fmt, args...)	printk("%s: " fmt "\n", __FUNCTION__, ## args)
#else
#define dbg(fmt, args...)	do{} while(0)
#endif

extern void gpio_usbh1_active(void);
extern void gpio_usbh1_inactive(void);
extern void gpio_usbh2_active(void);
extern void gpio_usbh2_inactive(void);
extern void gpio_usbotg_hs_active(void);
extern void gpio_usbotg_hs_inactive(void);
extern void gpio_usbotg_fs_active(void);
extern void gpio_usbotg_fs_inactive(void);

#if defined(CONFIG_USB_EHCI_ARC)
/* The dmamask must be set for EHCI to work */
static u64 ehci_dmamask = ~(u32) 0;
#endif

#if defined(CONFIG_USB_EHCI_ARC) || defined(CONFIG_USB_GADGET_ARC)
static void usb_release(struct device *dev)
{
	/* normally not freed */
}

/*
 * make sure USB_CLK is running at 60 MHz +/- 1000 Hz
 */
static int check_usbclk(void)
{
	unsigned long clk;

	clk = mxc_get_clocks(USB_CLK);
	if ((clk < 59999000) || (clk > 60001000)) {
		printk(KERN_ERR "USB_CLK=%lu, should be 60MHz\n", clk);
		return -1;
	}
	return 0;
}
#endif				/* CONFIG_USB_EHCI_ARC  || CONFIG_USB_GADGET_ARC */

#if defined(CONFIG_USB_EHCI_ARC_OTGHS) || defined(CONFIG_USB_GADGET_ARC_OTGHS)
static void otg_hs_set_xcvr(void)
{
	UOG_PORTSC1 &= ~PORTSC_PTS_MASK;	/* set ULPI xcvr */
	UOG_PORTSC1 |= PORTSC_PTS_ULPI;
}

static int otg_hs_init(void)
{
	dbg("grab OTG-HS pins");
	if (check_usbclk() != 0)
		return -EINVAL;

	gpio_usbotg_hs_active();	/* grab our pins */

	__raw_writew(PBC_BCTRL3_OTG_HS_EN, PBC3_CLEAR);	/* enable  OTG/HS */
	__raw_writew(PBC_BCTRL3_OTG_FS_EN, PBC3_SET);	/* disable OTG/FS */
	__raw_writew(PBC_BCTRL3_HSH_SEL, PBC3_SET);

	otg_hs_set_xcvr();	/* set transciever type */

	USBCTRL &= ~UCTRL_BPE;	/* disable bypass mode */
	USBCTRL |= UCTRL_OUIE |	/* ULPI intr enable */
	    UCTRL_OWIE |	/* OTG wakeup intr enable */
	    UCTRL_OPM;		/* power mask */

	return 0;
}

static void otg_hs_uninit(void)
{
	dbg();

	__raw_writew(PBC_BCTRL3_OTG_HS_EN, PBC3_SET);	/* disable  OTG/HS */
	__raw_writew(PBC_BCTRL3_HSH_SEL, PBC3_CLEAR);

	gpio_usbotg_hs_inactive();	/* release our pins */
}

#if defined(CONFIG_USB_EHCI_ARC_OTGHS)
static struct arc_usb_config otg_hs_config = {
	.name = "OTG_HS",
	.platform_init = otg_hs_init,
	.platform_uninit = otg_hs_uninit,
	.xcvr_type = PORTSC_PTS_ULPI,
	.usbmode = (u32) & UOG_USBMODE,
};

static struct resource otg_hs_resources[] = {
	{
	 .start = (u32) & UOG_CAPLENGTH,
	 .flags = 0,
	 },
	{
	 .start = INT_USB3,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device otg_hs_device = {
	.name = "ehci_hcd_otg_hs",
	.id = -1,
	.dev = {
		.release = usb_release,
		.dma_mask = &ehci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data = &otg_hs_config,
		},
	.num_resources = ARRAY_SIZE(otg_hs_resources),
	.resource = otg_hs_resources,
};
#endif				/* CONFIG_USB_EHCI_ARC_OTGHS */
#endif				/* CONFIG_USB_EHCI_ARC_OTGHS || CONFIG_USB_GADGET_ARC_OTGHS */

#ifdef CONFIG_USB_EHCI_ARC_OTGFS
static void otg_fs_set_xcvr(void)
{
	UOG_PORTSC1 &= ~PORTSC_PTS_MASK;	/* set serial xcvr */
	UOG_PORTSC1 |= PORTSC_PTS_SERIAL;
}

static int otg_fs_init(void)
{
	dbg("grab OTG-FS pins");

	if (check_usbclk() != 0)
		return -EINVAL;

	gpio_usbotg_fs_active();	/* grab our pins */

	__raw_writew(PBC_BCTRL3_OTG_FS_SEL, PBC3_SET);
	__raw_writew(PBC_BCTRL3_OTG_FS_EN, PBC3_CLEAR);	/* enable OTG/FS */
	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_CLEAR);	/* enable OTG VBUS */

	otg_fs_set_xcvr();	/* set transceiver type */

	USBCTRL &= ~(UCTRL_OSIC_MASK | UCTRL_BPE);	/* disable bypass mode */
	USBCTRL |= UCTRL_OSIC_SU6 |	/* single-ended/unidirectional */
	    UCTRL_OWIE | UCTRL_OPM;	/* power mask */

	return 0;
}

static void otg_fs_uninit(void)
{
	dbg();

	__raw_writew(PBC_BCTRL3_OTG_FS_SEL, PBC3_CLEAR);	/* OTG_FS_SEL */
	__raw_writew(PBC_BCTRL3_OTG_FS_EN, PBC3_SET);	/* disable OTG/FS */
	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_SET);	/* disable OTG VBUS */

	gpio_usbotg_fs_inactive();	/* release our pins */
}
#endif				/* CONFIG_USB_EHCI_ARC_OTGFS */

/* Host 1 */
#ifdef CONFIG_USB_EHCI_ARC_H1
static void usbh1_set_xcvr(void)
{
	UH1_PORTSC1 &= ~PORTSC_PTS_MASK;
	UH1_PORTSC1 |= PORTSC_PTS_SERIAL;
}

static int usbh1_init(void)
{
	dbg("grab H1 pins");

	if (check_usbclk() != 0)
		return -EINVAL;

	gpio_usbh1_active();
	usbh1_set_xcvr();

	__raw_writew(PBC_BCTRL3_FSH_EN, PBC3_CLEAR);	/* enable FSH */
	__raw_writew(PBC_BCTRL3_FSH_SEL, PBC3_SET);	/* Group B */
	__raw_writew(PBC_BCTRL3_FSH_MOD, PBC3_CLEAR);	/* single ended */
	__raw_writew(PBC_BCTRL3_FSH_VBUS_EN, PBC3_CLEAR);	/* enable FSH VBUS */

	USBCTRL &= ~(UCTRL_H1SIC_MASK | UCTRL_BPE);	/* disable bypass mode */
	USBCTRL |= UCTRL_H1SIC_SU6 |	/* single-ended / unidirectional */
	    UCTRL_H1WIE | UCTRL_H1DT |	/* disable H1 TLL */
	    UCTRL_H1PM;		/* power mask */

	return 0;
}

static void usbh1_uninit(void)
{
	dbg();

	__raw_writew(PBC_BCTRL3_FSH_EN, PBC3_SET);	/* disable FSH */
	__raw_writew(PBC_BCTRL3_FSH_VBUS_EN, PBC3_SET);	/* disable FSH VBUS */

	gpio_usbh1_inactive();	/* release our pins */
}

static struct arc_usb_config usbh1_config = {
	.name = "USBH1",
	.platform_init = usbh1_init,
	.platform_uninit = usbh1_uninit,
	.xcvr_type = PORTSC_PTS_SERIAL,
	.usbmode = (u32) & UH1_USBMODE,
};

/*
 *    Don't use IORESOURCE_MEM for the registers, 
 *    because that causes an ioremap(), and the USB
 *    registers are already statically mapped.
 */
static struct resource usbh1_resources[] = {
	{
	 .start = (u32) & UH1_CAPLENGTH,
	 .flags = 0,
	 },
	{
	 .start = INT_USB1,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device usbh1_device = {
	.name = "ehci_hcd_h1",
	.id = -1,
	.dev = {
		.release = usb_release,
		.dma_mask = &ehci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data = &usbh1_config,
		},
	.num_resources = ARRAY_SIZE(usbh1_resources),
	.resource = usbh1_resources,
};
#endif				/* CONFIG_USB_EHCI_ARC_H1 */

#ifdef CONFIG_USB_EHCI_ARC_H2
static void usbh2_set_xcvr(void)
{
	UH2_PORTSC1 &= ~PORTSC_PTS_MASK;	/* set ULPI xcvr */
	UH2_PORTSC1 |= PORTSC_PTS_ULPI;
}

static int usbh2_init(void)
{
	dbg("grab H2 pins");
	if (check_usbclk() != 0)
		return -EINVAL;

	/* abort the init if NAND card is present */
	if ((__raw_readw(PBC_BASE_ADDRESS + PBC_BSTAT1) & PBC_BSTAT1_NF_DET) ==
	    0) {
		printk(KERN_ERR "USBH2 port not configured: "
		       "pin conflict with NAND flash.\n");
		return -EINVAL;
	}

	gpio_usbh2_active();	/* grab our pins */

	__raw_writew(PBC_BCTRL3_HSH_SEL, PBC3_SET);	/* enable HSH select */
	__raw_writew(PBC_BCTRL3_HSH_EN, PBC3_CLEAR);	/* enable HSH */

	usbh2_set_xcvr();	/* set transceiver type */

	USBCTRL &= ~(UCTRL_H2SIC_MASK | UCTRL_BPE);	/* disable bypass mode */
	USBCTRL |= UCTRL_H2WIE |	/* wakeup intr enable */
	    UCTRL_H2UIE |	/* ULPI intr enable */
	    UCTRL_H2DT |	/* disable H2 TLL */
	    UCTRL_H2PM;		/* power mask */

	return 0;
}

static void usbh2_uninit(void)
{
	dbg();

	__raw_writew(PBC_BCTRL3_HSH_SEL, PBC3_CLEAR);	/* disable HSH select */
	__raw_writew(PBC_BCTRL3_HSH_EN, PBC3_SET);	/* disable HSH */

	gpio_usbh2_inactive();	/* release our pins */
}

static struct arc_usb_config usbh2_config = {
	.name = "USBH2",
	.platform_init = usbh2_init,
	.platform_uninit = usbh2_uninit,
	.xcvr_type = PORTSC_PTS_ULPI,
	.usbmode = (u32) & UH2_USBMODE,
};

/*
 *    Don't use IORESOURCE_MEM for the registers, 
 *    because that causes an ioremap(), and the USB
 *    registers are already statically mapped.
 */
static struct resource usbh2_resources[] = {
	{
	 .start = (u32) & UH2_CAPLENGTH,
	 .flags = 0,
	 },
	{
	 .start = INT_USB2,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device usbh2_device = {
	.name = "ehci_hcd_h2",
	.id = -1,
	.dev = {
		.release = usb_release,
		.dma_mask = &ehci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data = &usbh2_config,
		},
	.num_resources = ARRAY_SIZE(usbh2_resources),
	.resource = usbh2_resources,
};
#endif				/* CONFIG_USB_EHCI_ARC_H2 */

#if 0				/* DDD later for this */
static void isp1301_release(struct device *dev)
{
	/* normally not freed */
}

static struct resource isp1301_resources[] = {
	{
	 .start = 4,		/* FIXME */
	 .flags = IORESOURCE_IRQ,
	 },
};

static u64 isp1301_dmamask = ~(u32) 0;
static struct platform_device isp1301_device = {
	.name = "mx31_isp1301",
	.id = -1,
	.dev = {
		.release = isp1301_release,
		.dma_mask = &isp1301_dmamask,
		.coherent_dma_mask = 0xffffffff,
		},
	.num_resources = ARRAY_SIZE(isp1301_resources),
	.resource = isp1301_resources,
};
#endif				/* 0 */

#ifdef CONFIG_USB_GADGET_ARC
static struct arc_usb_config udc_hs_config = {
	.name = "udc_otg_hs",
	.platform_init = otg_hs_init,
	.platform_uninit = otg_hs_uninit,
	.xcvr_type = PORTSC_PTS_ULPI,
	.usbmode = (u32) & UH2_USBMODE,
};

static struct resource udc_resources[] = {
	{
	 .start = 0,
	 .flags = 0,
	 },
	{
	 .start = INT_USB3,
	 .flags = IORESOURCE_IRQ,
	 },
};

static u64 udc_dmamask = ~(u32) 0;
static struct platform_device udc_device = {
	.name = "arc_udc",
	.id = -1,
	.dev = {
		.release = usb_release,
		.dma_mask = &udc_dmamask,
		.coherent_dma_mask = 0xffffffff,
#if   defined CONFIG_USB_GADGET_ARC_OTGHS
		.platform_data = &udc_hs_config,
#elif defined CONFIG_USB_GADGET_ARC_OTGFS
		.platform_data = &udc_fs_config,
#else
#error "No OTG port configured."
#endif
		},
	.num_resources = ARRAY_SIZE(udc_resources),
	.resource = udc_resources,
};
#endif				/* CONFIG_USB_GADGET_ARC */

static int __init mx3_usb_init(void)
{
	int rc;

	rc = 0;
	dbg();

#if 0				/* DDD: later */
	rc = platform_device_register(&isp1301_device);
	if (rc) {
		pr_debug("can't register isp1301 dvd, %d\n", status);
	} else {
		dbg("isp1301: platform_device_register succeeded.");
		dbg("isp1301_device=0x%p  resources=0x%p.",
		    &isp1301_device, isp1301_device.resource);
	}
#endif

#ifdef CONFIG_USB_EHCI_ARC_H1
	rc = platform_device_register(&usbh1_device);
	if (rc) {
		pr_debug("can't register USBH1 Host, %d\n", status);
	} else {
		printk(KERN_INFO "usb: USBH1 registered\n");
		dbg("usbh1_device=0x%p  dev=0x%p  resources=0x%p  config=0x%p",
		    &usbh1_device, &usbh1_device.dev,
		    usbh1_device.resource, usbh1_device.dev.platform_data);
	}
#endif

#ifdef CONFIG_USB_EHCI_ARC_H2
	rc = platform_device_register(&usbh2_device);
	if (rc) {
		pr_debug("can't register USBH2 Host, %d\n", status);
	} else {
		printk(KERN_INFO "usb: USBH2 registered\n");
		dbg("usbh2_device=0x%p  dev=0x%p  resources=0x%p.",
		    &usbh2_device, &usbh2_device.dev, usbh2_device.resource);
	}
#endif

#ifdef CONFIG_USB_EHCI_ARC_OTGHS
	rc = platform_device_register(&otg_hs_device);
	if (rc) {
		pr_debug("can't register OTG HS Host, %d\n", status);
	} else {
		printk(KERN_INFO "usb: OTG_HS Host registered\n");
		dbg("otg_hs_device at 0x%p  dev=0x%p  resources=0x%p.",
		    &otg_hs_device, &otg_hs_device.dev, otg_hs_device.resource);
	}
#endif

#ifdef CONFIG_USB_GADGET_ARC
	rc = platform_device_register(&udc_device);
	if (rc)
		pr_debug("can't register OTG Gadget, %d\n", status);
	else
		printk(KERN_INFO "usb: OTG Gadget registered\n");
#endif
	return 0;
}

subsys_initcall(mx3_usb_init);
