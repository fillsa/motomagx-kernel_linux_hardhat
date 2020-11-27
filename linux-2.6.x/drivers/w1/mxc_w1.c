/*
 * Copyright 2005 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @defgroup MXC_OWIRE MXC Driver for owire interface
 */

/*!
 * @file mxc_owire.c
 *
 * @brief Driver for the Freescale Semiconductor MXC owire interface.
 *
 *
 * @ingroup MXC_OWIRE
 */

/*!
 * Include Files
 */

#include <asm/atomic.h>
#include <asm/types.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/pci_ids.h>
#include <linux/pci.h>
#include <linux/timer.h>
#include <linux/config.h>
#include <linux/init.h>
#include <asm/hardware.h>
#include <asm/setup.h>

#include "w1.h"
#include "w1_int.h"
#include "w1_log.h"

/*
 * mxc function declarations
 */

static int __devinit mxc_w1_probe(struct device *pdev);
static int __devexit mxc_w1_remove(struct device *dev);

extern void gpio_owire_active(void);
extern void gpio_owire_inactive(void);

/*
 * MXC W1 Register offsets
 */
#define MXC_W1_CONTROL          0x0
#define MXC_W1_TIME_DIVIDER     0x02
#define MXC_W1_RESET            0x04

/* frequency divider for 1 MHz clock */
#ifdef CONFIG_ARCH_MXC91231
#define  MXC_W1_FRQ_DIV        0x19
#endif

#ifdef CONFIG_ARCH_MX3
#define  MXC_W1_FRQ_DIV        0x41
#endif

DEFINE_RAW_SPINLOCK(w1_lock);

/*!
 * This structure contains pointers to  callback functions.
 */
static struct device_driver mxc_w1_driver = {
	.name = "mxcw1",
	.bus = &platform_bus_type,
	.probe = mxc_w1_probe,
	.remove = mxc_w1_remove,
};

static struct platform_device mxc_w1_devices = {
	.name = "mxcw1",
	.id = 0
};

/*!
 * This structure is used to store
 * information specific to w1 module.
 */

struct mxc_w1_device {
	char *base_address;
	unsigned long found;
	unsigned int clkdiv;
	struct w1_bus_master *bus_master;
};

/*
 * one-wire function declarations
 */

static u8 mxc_w1_ds2_read_bit(unsigned long);
static void mxc_w1_ds2_write_bit(unsigned long, u8);
static u8 mxc_w1_ds2_reset_bus(unsigned long);

/*!
 * this is the low level routine to
 * read a bit on the One Wire interface
 * on the hardware.
 * @param data  the data field of the w1 device structure
 * @return the function returns the bit value read
 */
static u8 mxc_w1_ds2_read_bit(unsigned long data)
{

	volatile u16 reg_val;
	u8 ret;
	struct mxc_w1_device *dev = (struct mxc_w1_device *)data;

	__raw_writew((1 << 4), dev->base_address + MXC_W1_CONTROL);
	do {
		reg_val = __raw_readw(dev->base_address + MXC_W1_CONTROL);
	} while (0 != ((reg_val >> 4) & 0x1));

	reg_val = (((__raw_readw(dev->base_address + MXC_W1_CONTROL)) >> 3) & 0x1);
	ret = (u8) (reg_val);

	return ret;
}

/*
 * this is the low level routine to
 * reset the device on the One Wire interface
 * on the hardware
 * @param data  the data field of the w1 device structure
 * @return the function returns 0 when the reset pulse has
 *  been generated
 */
static u8 mxc_w1_ds2_reset_bus(unsigned long data)
{
	volatile u8 reg_val;
	u8 ret;
	struct mxc_w1_device *dev = (struct mxc_w1_device *)data;

	__raw_writew(0x80, (dev->base_address + MXC_W1_CONTROL));
	do {
		reg_val = __raw_readw(dev->base_address + MXC_W1_CONTROL);
	} while (((reg_val >> 7) & 0x1) != 0);

	ret = ((reg_val >> 7) & 0x1);

	return ret;
}

/*!
 * this is the low level routine to
 * write a bit on the One Wire interface
 * on the hardware
 * @param data  the data field of the w1 device structure
 * @param bit  the data bit to be written
 * @return the function returns void
 */
static void mxc_w1_ds2_write_bit(unsigned long data, u8 bit)
{

	volatile u8 reg_val;
	struct mxc_w1_device *dev = (struct mxc_w1_device *)data;

	if (0 == bit) {
		__raw_writew((1 << 5), (dev->base_address + MXC_W1_CONTROL));

		do {
			reg_val = __raw_readw(dev->base_address + MXC_W1_CONTROL);
		} while (0 != ((reg_val >> 5) & 0x1));
	}

	else {
		__raw_writew((1 << 4), dev->base_address + MXC_W1_CONTROL);
		do {
			reg_val = __raw_readw(dev->base_address + MXC_W1_CONTROL);
		} while (0 != ((reg_val >> 4) & 0x1));
	}
	return;
}

/*!
 * this routine sets the One Wire clock
 * to a value of 1 Mhz, as required by
 * hardware.
 * @param   dev   the device structure for w1
 * @return  The function returns void
 */
static void mxc_w1_hw_init(struct mxc_w1_device *dev)
{

	/* set the timer divider clock to divide by 65 */
	/* as the clock to the One Wire is at 66.5MHz */
	__raw_writew(dev->clkdiv, dev->base_address + MXC_W1_TIME_DIVIDER);
	return;
}

/*!
 * this is the probe routine for the One Wire driver.
 * It is called during the driver initilaization.
 * @param   pdev   the platform device structure for w1
 * @return The function returns 0 on success
 * and a non-zero value on failure
 *
 */
static int __devinit mxc_w1_probe(struct device *pdev)
{

	struct platform_device *platdev = to_platform_device(pdev);
	struct mxc_w1_device *dev;
	int id = platdev->id;
	int err = 0;

	if (mxc_w1_devices.id != id) {
		err = 1;
		return err;
	}

	dev = kmalloc(sizeof(struct mxc_w1_device) +
		      sizeof(struct w1_bus_master), GFP_KERNEL);
	if (!dev) {
		return -ENOMEM;
	}

	memset(dev, 0,
	       sizeof(struct mxc_w1_device) + sizeof(struct w1_bus_master));
	dev->bus_master = (struct w1_bus_master *)(dev + 1);
	dev->found = 1;
	dev->clkdiv = MXC_W1_FRQ_DIV;
	dev->base_address = (void *)IO_ADDRESS(OWIRE_BASE_ADDR);

	mxc_w1_hw_init(dev);
	dev->bus_master->data = (unsigned long)dev;
	dev->bus_master->read_bit = &mxc_w1_ds2_read_bit;
	dev->bus_master->write_bit = &mxc_w1_ds2_write_bit;
	dev->bus_master->reset_bus = &mxc_w1_ds2_reset_bus;

	err = w1_add_master_device(dev->bus_master);

	if (err)
		goto err_out_free_device;

	dev_set_drvdata(pdev, dev);
	return 0;

      err_out_free_device:

	kfree(dev);
	return err;
}

/*
 * disassociate the w1 device from the driver
 * @param   dev   the device structure for w1
 * @return  The function returns void
 */
static int mxc_w1_remove(struct device *dev)
{
	struct mxc_w1_device *pdev = dev_get_drvdata(dev);

	if (pdev->found) {
		w1_remove_master_device(pdev->bus_master);
	}
	dev_set_drvdata(dev, NULL);

	return 0;
}

/*
 * initialize the driver
 * @return The function returns 0 on success
 * and a non-zero value on failure
 */

static int __init mxc_w1_init(void)
{
	int ret;

	printk(KERN_INFO "Serial: MXC OWire driver\n");

	gpio_owire_active();

	ret = driver_register(&mxc_w1_driver);
	if (0 != ret) {
		return ret;
	}

	ret = platform_device_register(&mxc_w1_devices);
	if (0 != ret) {
		return ret;
	}

	return ret;
}

/*
 * cleanup before the driver exits
 */
static void mxc_w1_exit(void)
{
	gpio_owire_inactive();
	driver_unregister(&mxc_w1_driver);
	platform_device_unregister(&mxc_w1_devices);
}

module_init(mxc_w1_init);
module_exit(mxc_w1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freescale Semiconductors Inc");
MODULE_DESCRIPTION("Driver for One-Wire on MXC");
