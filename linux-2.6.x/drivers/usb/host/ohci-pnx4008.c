/*
 * drivers/usb/host/ohci-pnx4008.c
 *
 * driver for Philips PNX4008 USB Host
 *
 * Author: Dmitry Chigirev <source@mvista.com>
 *
 * register initialization is based on code examples provided by Philips
 * Copyright (c) 2005 Koninklijke Philips Electronics N.V.
 *
 * NOTE: This driver does not have suspend/resume functionality
 * This driver is intended for engineering development purposes only
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/mach-types.h>

#include <asm/arch/platform.h>
#include <asm/arch/irqs.h>
#include <asm/arch/pm.h>
#include <asm/hardware/clock.h>

#include <linux/i2c.h>

#define USB_CTRL 	IO_ADDRESS(PNX4008_PWRMAN_BASE + 0x64)

/* USB_CTRL bit defines */
#define USB_SLAVE_HCLK_EN	(1 << 24)
#define USB_I2C_EN		(1 << 23)
#define USB_DEV_NEED_CLK_EN	(1 << 22)
#define USB_HOST_NEED_CLK_EN	(1 << 21)

#define USB_OTG_CLK_CTRL	IO_ADDRESS(PNX4008_USB_CONFIG_BASE+ 0xFF4)
#define USB_OTG_CLK_STAT	IO_ADDRESS(PNX4008_USB_CONFIG_BASE+ 0xFF8)

/* USB_OTG_CLK_CTRL bit defines */
#define AHB_M_CLOCK_ON		(1 << 4)
#define OTG_CLOCK_ON		(1 << 3)
#define I2C_CLOCK_ON		(1 << 2)
#define DEV_CLOCK_ON		(1 << 1)
#define HOST_CLOCK_ON		(1 << 0)

#define USB_OTG_STAT_CONTROL	IO_ADDRESS(PNX4008_USB_CONFIG_BASE + 0x110)

/* USB_OTG_STAT_CONTROL bit defines */
#define TRANSPARENT_I2C_EN	(1 << 7)
#define HOST_EN			(1 << 0)

/* ISP1301 USB transceiver I2C registers */
#define	ISP1301_MODE_CONTROL_1		0x04	/* u8 read, set, +1 clear */
#	define	MC1_SPEED_REG		(1 << 0)
#	define	MC1_SUSPEND_REG		(1 << 1)
#	define	MC1_DAT_SE0		(1 << 2)
#	define	MC1_TRANSPARENT		(1 << 3)
#	define	MC1_BDIS_ACON_EN	(1 << 4)
#	define	MC1_OE_INT_EN		(1 << 5)
#	define	MC1_UART_EN		(1 << 6)
#	define	MC1_MASK		0x7f
#define	ISP1301_MODE_CONTROL_2		0x12	/* u8 read, set, +1 clear */
#	define	MC2_GLOBAL_PWR_DN	(1 << 0)
#	define	MC2_SPD_SUSP_CTRL	(1 << 1)
#	define	MC2_BI_DI		(1 << 2)
#	define	MC2_TRANSP_BDIR0	(1 << 3)
#	define	MC2_TRANSP_BDIR1	(1 << 4)
#	define	MC2_AUDIO_EN		(1 << 5)
#	define	MC2_PSW_EN		(1 << 6)
#	define	MC2_EN2V7		(1 << 7)
#define	ISP1301_OTG_CONTROL_1		0x06	/* u8 read, set, +1 clear */
#	define	OTG1_DP_PULLUP		(1 << 0)
#	define	OTG1_DM_PULLUP		(1 << 1)
#	define	OTG1_DP_PULLDOWN	(1 << 2)
#	define	OTG1_DM_PULLDOWN	(1 << 3)
#	define	OTG1_ID_PULLDOWN	(1 << 4)
#	define	OTG1_VBUS_DRV		(1 << 5)
#	define	OTG1_VBUS_DISCHRG	(1 << 6)
#	define	OTG1_VBUS_CHRG		(1 << 7)
#define	ISP1301_OTG_STATUS		0x10	/* u8 readonly */
#	define	OTG_B_SESS_END		(1 << 6)
#	define	OTG_B_SESS_VLD		(1 << 7)

#define	ISP1301_INTERRUPT_SOURCE	0x08	/* u8 read */
#define	ISP1301_INTERRUPT_LATCH		0x0A	/* u8 read, set, +1 clear */

#define	ISP1301_INTERRUPT_FALLING	0x0C	/* u8 read, set, +1 clear */
#define	ISP1301_INTERRUPT_RISING	0x0E	/* u8 read, set, +1 clear */

/* same bitfields in all interrupt registers */
#	define	INTR_VBUS_VLD		(1 << 0)
#	define	INTR_SESS_VLD		(1 << 1)
#	define	INTR_DP_HI		(1 << 2)
#	define	INTR_ID_GND		(1 << 3)
#	define	INTR_DM_HI		(1 << 4)
#	define	INTR_ID_FLOAT		(1 << 5)
#	define	INTR_BDIS_ACON		(1 << 6)
#	define	INTR_CR_INT		(1 << 7)

#define ISP1301_I2C_ADDR 0x2C

#define ISP1301_I2C_MODE_CONTROL_1 0x4
#define ISP1301_I2C_MODE_CONTROL_2 0x12
#define ISP1301_I2C_OTG_CONTROL_1 0x6
#define ISP1301_I2C_OTG_CONTROL_2 0x10
#define ISP1301_I2C_INTERRUPT_SOURCE 0x8
#define ISP1301_I2C_INTERRUPT_LATCH 0xA
#define ISP1301_I2C_INTERRUPT_FALLING 0xC
#define ISP1301_I2C_INTERRUPT_RISING 0xE
#define ISP1301_I2C_REG_CLEAR_ADDR 1

struct i2c_driver isp1301_driver;
struct i2c_client *isp1301_i2c_client;

extern int usb_disabled(void);
extern int ocpi_enable(void);

static struct clk *usb_clk;
static unsigned long usb_init_status;
#define HC_STARTED        0
#define USB_IRQ_INITED    1
#define HC_BUFFER_INITED  2
#define HC_STRUCT_INITED  3
#define USB_CLOCKS_INITED 4
#define USB_I2C_INITED    5
#define REGION_INITED     6

static int usb_hcd_pnx4008_remove(struct device *dev);

static int isp1301_probe(struct i2c_adapter *adap);
static int isp1301_detach(struct i2c_client *client);
static int isp1301_command(struct i2c_client *client, unsigned int cmd,
			   void *arg);

static unsigned short normal_i2c[] =
    { ISP1301_I2C_ADDR, ISP1301_I2C_ADDR + 1, I2C_CLIENT_END };
static unsigned short dummy_i2c_addrlist[] = { I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	.normal_i2c = normal_i2c,
	.normal_i2c_range = dummy_i2c_addrlist,
	.probe = dummy_i2c_addrlist,
	.probe_range = dummy_i2c_addrlist,
	.ignore = dummy_i2c_addrlist,
	.ignore_range = dummy_i2c_addrlist,
	.force = dummy_i2c_addrlist
};

struct i2c_driver isp1301_driver = {
	.name = "isp1301",
	.id = I2C_DRIVERID_EXP0,	/* Fake Id */
	.class = I2C_CLASS_HWMON,
	.flags = I2C_DF_NOTIFY,
	.attach_adapter = isp1301_probe,
	.detach_client = isp1301_detach,
	.command = isp1301_command
};

static int isp1301_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;

	c = (struct i2c_client *)kmalloc(sizeof(*c), GFP_KERNEL);

	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(*c));

	strcpy(c->name, "isp1301");
	c->id = isp1301_driver.id;
	c->flags = 0;
	c->addr = addr;
	c->adapter = adap;
	c->driver = &isp1301_driver;

	isp1301_i2c_client = c;

	return i2c_attach_client(c);
}

static int isp1301_probe(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, isp1301_attach);
}

static int isp1301_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	kfree(isp1301_i2c_client);
	return 0;
}

/* No commands defined */
static int isp1301_command(struct i2c_client *client, unsigned int cmd,
			   void *arg)
{
	return 0;
}

static void i2c_write(u8 buf, u8 subaddr)
{
	char tmpbuf[2];

	tmpbuf[0] = subaddr;	/*register number */
	tmpbuf[1] = buf;	/*register data */
	i2c_master_send(isp1301_i2c_client, &tmpbuf[0], 2);
}

static void isp1301_configure(void)
{
	/* PNX4008 only supports DAT_SE0 USB mode */
	/* PNX4008 R2A requires setting the MAX603 to output 3.6V */
	/* Power up externel charge-pump */

	i2c_write(MC1_DAT_SE0 | MC1_SPEED_REG, ISP1301_I2C_MODE_CONTROL_1);
	i2c_write(~(MC1_DAT_SE0 | MC1_SPEED_REG),
		  ISP1301_I2C_MODE_CONTROL_1 | ISP1301_I2C_REG_CLEAR_ADDR);
	i2c_write(MC2_BI_DI | MC2_PSW_EN | MC2_SPD_SUSP_CTRL,
		  ISP1301_I2C_MODE_CONTROL_2);
	i2c_write(~(MC2_BI_DI | MC2_PSW_EN | MC2_SPD_SUSP_CTRL),
		  ISP1301_I2C_MODE_CONTROL_2 | ISP1301_I2C_REG_CLEAR_ADDR);
	i2c_write(OTG1_DM_PULLDOWN | OTG1_DP_PULLDOWN,
		  ISP1301_I2C_OTG_CONTROL_1);
	i2c_write(~(OTG1_DM_PULLDOWN | OTG1_DP_PULLDOWN),
		  ISP1301_I2C_OTG_CONTROL_1 | ISP1301_I2C_REG_CLEAR_ADDR);
	i2c_write(0xFF,
		  ISP1301_I2C_INTERRUPT_LATCH | ISP1301_I2C_REG_CLEAR_ADDR);
	i2c_write(0xFF,
		  ISP1301_I2C_INTERRUPT_FALLING | ISP1301_I2C_REG_CLEAR_ADDR);
	i2c_write(0xFF,
		  ISP1301_I2C_INTERRUPT_RISING | ISP1301_I2C_REG_CLEAR_ADDR);

}

static inline void isp1301_vbus_on(void)
{
	i2c_write(OTG1_VBUS_DRV, ISP1301_I2C_OTG_CONTROL_1);
}

static inline void isp1301_vbus_off(void)
{
	i2c_write(OTG1_VBUS_DRV,
		  ISP1301_I2C_OTG_CONTROL_1 | ISP1301_I2C_REG_CLEAR_ADDR);
}

static void pnx4008_start_hc(void)
{
	unsigned long tmp;
	tmp = __raw_readl(USB_OTG_STAT_CONTROL);
	tmp |= HOST_EN;
	__raw_writel(tmp, USB_OTG_STAT_CONTROL);
	isp1301_vbus_on();
}

static void pnx4008_stop_hc(void)
{
	unsigned long tmp;
	isp1301_vbus_off();
	tmp = __raw_readl(USB_OTG_STAT_CONTROL);
	tmp &= ~HOST_EN;
	__raw_writel(tmp, USB_OTG_STAT_CONTROL);
}

static int __devinit ohci_pnx4008_start(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int ret;

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run(ohci)) < 0) {
		err("can't start %s", ohci->hcd.self.bus_name);
		ohci_stop(hcd);
		return ret;
	}
	return 0;
}

static const struct hc_driver ohci_pnx4008_hc_driver = {
	.description = hcd_name,

	/*
	 * generic hardware linkage
	 */
	.irq = ohci_irq,
	.flags = HCD_USB11,

	/*
	 * basic lifecycle operations
	 */
	.start = ohci_pnx4008_start,
	.stop = ohci_stop,

	/*
	 * memory lifecycle (except per-request)
	 */
	.hcd_alloc = ohci_hcd_alloc,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = ohci_urb_enqueue,
	.urb_dequeue = ohci_urb_dequeue,
	.endpoint_disable = ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number = ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data = ohci_hub_status_data,
	.hub_control = ohci_hub_control,

	.start_port_reset = ohci_start_port_reset,
};

#define USB_CLOCK_MASK (AHB_M_CLOCK_ON| OTG_CLOCK_ON | HOST_CLOCK_ON | I2C_CLOCK_ON)

static int usb_hcd_pnx4008_probe(struct device *dev)
{
	struct usb_hcd *hcd = 0;
	struct ohci_hcd *ohci;
	const struct hc_driver *driver = &ohci_pnx4008_hc_driver;
	struct platform_device *pdev = to_platform_device(dev);

	int ret = 0;

#ifndef MODULE
	DECLARE_WAIT_QUEUE_HEAD(usb_sleep);
#endif

	printk(KERN_DEBUG "%s: " DRIVER_INFO " (pnx4008)\n", hcd_name);
	if (usb_disabled()) {
		err("USB is disabled");
		return -ENODEV;
	}

	if (pdev->num_resources != 2
	    || pdev->resource[0].flags != IORESOURCE_MEM
	    || pdev->resource[1].flags != IORESOURCE_IRQ) {
		err("Invalid resource configuration");
		return -ENODEV;
	}

	if (!request_region(PNX4008_USB_CONFIG_BASE, 0x100, "usb-ohci")) {
		err("USB registers are already in use");
		return -EBUSY;
	}

	set_bit(REGION_INITED, &usb_init_status);

	/* Enable AHB slave - USB clock, needed for further USB clock control,
	 * Also configure transceiver type
	 */

	__raw_writel(USB_SLAVE_HCLK_EN | (1 << 19), USB_CTRL);

	/* Enable USB-I2C clock */
	__raw_writel(I2C_CLOCK_ON, USB_OTG_CLK_CTRL);

	/*wait for I2C clock */
	while (!(__raw_readl(USB_OTG_CLK_STAT) & I2C_CLOCK_ON)) ;

	ret = i2c_add_driver(&isp1301_driver);
	if (ret < 0) {
		err("failed to connect I2C to ISP1301 USB Transceiver");
		usb_hcd_pnx4008_remove(dev);
		return ret;
	}

	isp1301_configure();

	set_bit(USB_I2C_INITED, &usb_init_status);

	/* Enable USB PLL */

	usb_clk = clk_get(0, "ck_pll5");
	if (IS_ERR(usb_clk)) {
		err("failed to acquire USB PLL");
		usb_hcd_pnx4008_remove(dev);
		return PTR_ERR(usb_clk);
	}

	ret = clk_set_rate(usb_clk, 48000);
	if (ret < 0) {
		err("failed to set USB clock rate");
		clk_put(usb_clk);
		usb_hcd_pnx4008_remove(dev);
		return ret;
	}

	ret = clk_use(usb_clk);
	if (ret < 0) {

		err("failed to enable USB clock");
		clk_put(usb_clk);
		usb_hcd_pnx4008_remove(dev);
		return ret;
	}

	__raw_writel(__raw_readl(USB_CTRL) | USB_HOST_NEED_CLK_EN, USB_CTRL);

	/* Set to enable all needed USB clocks */
	__raw_writel(USB_CLOCK_MASK, USB_OTG_CLK_CTRL);

	while ((__raw_readl(USB_OTG_CLK_STAT) & USB_CLOCK_MASK) !=
	       USB_CLOCK_MASK) ;

	/* Set all USB bits in the Start Enable register */
	start_int_set_falling_edge(SE_USB_OTG_ATX_INT_N);
	start_int_ack(SE_USB_OTG_ATX_INT_N);
	start_int_umask(SE_USB_OTG_ATX_INT_N);

	start_int_set_rising_edge(SE_USB_OTG_TIMER_INT);
	start_int_ack(SE_USB_OTG_TIMER_INT);
	start_int_umask(SE_USB_OTG_TIMER_INT);

	start_int_set_rising_edge(SE_USB_I2C_INT);
	start_int_ack(SE_USB_I2C_INT);
	start_int_umask(SE_USB_I2C_INT);

	start_int_set_rising_edge(SE_USB_INT);
	start_int_ack(SE_USB_INT);
	start_int_umask(SE_USB_INT);

	start_int_set_rising_edge(SE_USB_NEED_CLK_INT);
	start_int_ack(SE_USB_NEED_CLK_INT);
	start_int_umask(SE_USB_NEED_CLK_INT);

	start_int_set_rising_edge(SE_USB_AHB_NEED_CLK_INT);
	start_int_ack(SE_USB_AHB_NEED_CLK_INT);
	start_int_umask(SE_USB_AHB_NEED_CLK_INT);

	set_bit(USB_CLOCKS_INITED, &usb_init_status);

	hcd = driver->hcd_alloc();
	if (hcd == NULL) {
		err("Failed to allocate HC structure");
		usb_hcd_pnx4008_remove(dev);
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, hcd);
	ohci = hcd_to_ohci(hcd);

	hcd->driver = (struct hc_driver *)driver;
	hcd->description = driver->description;
	hcd->irq = pdev->resource[1].start;
	hcd->regs = (void *)pdev->resource[0].start;
	hcd->self.controller = &pdev->dev;

	set_bit(HC_STRUCT_INITED, &usb_init_status);

	ret = hcd_buffer_create(hcd);
	if (ret < 0) {
		err("Failed to allocate HC buffer");
		usb_hcd_pnx4008_remove(dev);
		return ret;
	}

	set_bit(HC_BUFFER_INITED, &usb_init_status);
#ifdef CONFIG_PREEMPT_HARDIRQS
	ret = request_irq(hcd->irq, usb_hcd_irq, 0, hcd->description, hcd);
#else
	ret =
	    request_irq(hcd->irq, usb_hcd_irq, SA_INTERRUPT, hcd->description,
			hcd);
#endif
	if (ret < 0) {
		err("Failed to request IRQ");
		usb_hcd_pnx4008_remove(dev);
		return ret;
	}
	set_bit(USB_IRQ_INITED, &usb_init_status);

	dev_info(&pdev->dev, "at 0x%p, irq %d\n", hcd->regs, hcd->irq);
	usb_bus_init(&hcd->self);
	hcd->self.op = &usb_hcd_operations;
	hcd->self.release = &usb_hcd_release;
	hcd->self.hcpriv = (void *)hcd;
	hcd->self.bus_name = "pnx4008";	//pdev->dev.bus_id;
	hcd->product_desc = "pnx4008 OHCI";
	INIT_LIST_HEAD(&hcd->dev_list);

	usb_register_bus(&hcd->self);

	pnx4008_start_hc();

	if ((ret = driver->start(hcd)) < 0) {
		err("Failed to start HC");
		usb_hcd_pnx4008_remove(dev);
		return ret;
	}
	set_bit(HC_STARTED, &usb_init_status);

#ifndef MODULE
	/* this is a hack to let a usb network adapter initialize before
	   the kernel attempts mounting root NFS */
	sleep_on_timeout(&usb_sleep, HZ * 2);
#endif

	return 0;
}

static int usb_hcd_pnx4008_remove(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	if (test_bit(HC_STARTED, &usb_init_status)) {
		hcd->state = USB_STATE_QUIESCING;
		usb_disconnect(&hcd->self.root_hub);
		hcd->driver->stop(hcd);
		hcd->state = USB_STATE_HALT;
		pnx4008_stop_hc();
		usb_deregister_bus(&hcd->self);
		clear_bit(HC_STARTED, &usb_init_status);
	}

	if (test_bit(USB_IRQ_INITED, &usb_init_status)) {
		free_irq(hcd->irq, hcd);
		clear_bit(USB_IRQ_INITED, &usb_init_status);
	}

	if (test_bit(HC_BUFFER_INITED, &usb_init_status)) {
		hcd_buffer_destroy(hcd);
		clear_bit(HC_BUFFER_INITED, &usb_init_status);
	}

	if (test_bit(HC_STRUCT_INITED, &usb_init_status)) {
		kfree(hcd);
		dev_set_drvdata(dev, NULL);
		clear_bit(HC_STRUCT_INITED, &usb_init_status);
	}

	if (test_bit(USB_CLOCKS_INITED, &usb_init_status)) {
		start_int_mask(SE_USB_OTG_ATX_INT_N);
		start_int_mask(SE_USB_OTG_TIMER_INT);
		start_int_mask(SE_USB_I2C_INT);
		start_int_mask(SE_USB_INT);
		start_int_mask(SE_USB_NEED_CLK_INT);
		start_int_mask(SE_USB_AHB_NEED_CLK_INT);
		clk_unuse(usb_clk);
		clk_put(usb_clk);
		clear_bit(USB_CLOCKS_INITED, &usb_init_status);
	}

	if (test_bit(USB_I2C_INITED, &usb_init_status)) {
		i2c_del_driver(&isp1301_driver);
		__raw_writel(0, USB_OTG_CLK_CTRL);
		__raw_writel((1 << 19), USB_CTRL);
	}

	if (test_bit(REGION_INITED, &usb_init_status)) {
		release_region(PNX4008_USB_CONFIG_BASE, 0x100);
		clear_bit(REGION_INITED, &usb_init_status);
	}

	return 0;
}

static struct device_driver usb_hcd_pnx4008_driver = {
	.name = "usb-ohci",
	.bus = &platform_bus_type,
	.probe = usb_hcd_pnx4008_probe,
	.remove = usb_hcd_pnx4008_remove,
};

static int __init usb_hcd_pnx4008_init(void)
{
	return driver_register(&usb_hcd_pnx4008_driver);
}

static void __exit usb_hcd_pnx4008_cleanup(void)
{
	return driver_unregister(&usb_hcd_pnx4008_driver);
}

module_init(usb_hcd_pnx4008_init);
module_exit(usb_hcd_pnx4008_cleanup);
