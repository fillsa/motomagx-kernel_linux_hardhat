/*
 * drivers/char/watchdog/pnx4008_wdt.c
 *
 * Watchdog driver for PNX4008 board
 *
 * Author: Dmitry Chigirev <source@mvista.com>,
 * Based on sa1100 driver,
 * Copyright (C) 2000 Oleg Drokin <green@crimea.edu>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/ioport.h>
#include <linux/device.h>

#include <asm/hardware.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define MODULE_NAME "PNX4008-WDT"
#define PKMOD MODULE_NAME ": "

/* WatchDog Timer - Chapter 23 Page 207 */

#define DEFAULT_HEARTBEAT 60
#define MAX_HEARTBEAT     60

/*Watchdog timer register set definition*/
#define WDTIM_INT     IO_ADDRESS((PNX4008_WDOG_BASE + 0x0))
#define WDTIM_CTRL    IO_ADDRESS((PNX4008_WDOG_BASE + 0x4))
#define WDTIM_COUNTER IO_ADDRESS((PNX4008_WDOG_BASE + 0x8))
#define WDTIM_MCTRL   IO_ADDRESS((PNX4008_WDOG_BASE + 0xC))
#define WDTIM_MATCH0  IO_ADDRESS((PNX4008_WDOG_BASE + 0x10))
#define WDTIM_EMR     IO_ADDRESS((PNX4008_WDOG_BASE + 0x14))
#define WDTIM_PULSE   IO_ADDRESS((PNX4008_WDOG_BASE + 0x18))
#define WDTIM_RES     IO_ADDRESS((PNX4008_WDOG_BASE + 0x1C))

/* WDTIM_INT bit definitions */
#define MATCH_INT      1

/* WDTIM_CTRL bit definitions */
#define COUNT_ENAB     1
#define RESET_COUNT    (1<<1)
#define DEBUG_EN       (1<<2)

/* WDTIM_MCTRL bit definitions */
#define MR0_INT        1
#undef  RESET_COUNT0
#define RESET_COUNT0   (1<<2)
#define STOP_COUNT0    (1<<2)
#define M_RES1         (1<<3)
#define M_RES2         (1<<4)
#define RESFRC1        (1<<5)
#define RESFRC2        (1<<6)

/* WDTIM_EMR bit definitions */
#define EXT_MATCH0      1
#define MATCH_OUTPUT_HIGH (2<<4)	/*a MATCH_CTRL setting */

/* WDTIM_RES bit definitions */
#define WDOG_RESET      1	/* read only */

/* Watchdog Clock Control in PM register */
#define WATCHDOG_CLK_EN                      1

#define TIMCLK_CTRL_REG  IO_ADDRESS((PNX4008_PWRMAN_BASE + 0xBC))

#define wdt_clk_disable  __raw_writel((__raw_readl(TIMCLK_CTRL_REG) & ~WATCHDOG_CLK_EN), TIMCLK_CTRL_REG)
#define wdt_clk_enable   __raw_writel((__raw_readl(TIMCLK_CTRL_REG) | WATCHDOG_CLK_EN), TIMCLK_CTRL_REG)

#define WDOG_COUNTER_RATE 13000000	/*the counter clock is 13 MHz fixed */

#ifdef CONFIG_WATCHDOG_NOWAYOUT
static int nowayout = 1;
#else
static int nowayout = 0;
#endif
static int heartbeat = DEFAULT_HEARTBEAT;

static unsigned long wdt_status;
#define WDT_IN_USE        0
#define WDT_OK_TO_CLOSE   1
#define WDT_REGION_INITED 2
#define WDT_DEVICE_INITED 3

static unsigned long boot_status;

static void wdt_enable(void)
{
	wdt_clk_enable;
	__raw_writel(RESET_COUNT, WDTIM_CTRL);	/*stop counter, initiate counter reset */
	while (__raw_readl(WDTIM_COUNTER)) ;	/*wait for reset to complete. 100% guarantee event */
	__raw_writel(M_RES2 | STOP_COUNT0 | RESET_COUNT0, WDTIM_MCTRL);	/*internal and external reset, stop after that */
	__raw_writel(MATCH_OUTPUT_HIGH, WDTIM_EMR);	/*configure match output */
	__raw_writel(MATCH_INT, WDTIM_INT);	/*clear interrupt, just in case */
	__raw_writel(0xFFFF, WDTIM_PULSE);	/* the longest pulse period 65541/(13*10^6) seconds ~ 5 ms. */
	__raw_writel(heartbeat * WDOG_COUNTER_RATE, WDTIM_MATCH0);
	__raw_writel(COUNT_ENAB | DEBUG_EN, WDTIM_CTRL);	/*enable counter, stop when debugger active */
}

static void wdt_disable(void)
{
	__raw_writel(0, WDTIM_CTRL);	/*stop counter */
	wdt_clk_disable;
}

static int pnx4008_wdt_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(WDT_IN_USE, &wdt_status))
		return -EBUSY;

	clear_bit(WDT_OK_TO_CLOSE, &wdt_status);

	wdt_enable();

	return nonseekable_open(inode, file);
}

static ssize_t
pnx4008_wdt_write(struct file *file, const char *data, size_t len,
		  loff_t * ppos)
{
	/*  Can't seek (pwrite) on this device  */
	if (ppos != &file->f_pos)
		return -ESPIPE;

	if (len) {
		if (!nowayout) {
			size_t i;

			clear_bit(WDT_OK_TO_CLOSE, &wdt_status);

			for (i = 0; i != len; i++) {
				char c;

				if (get_user(c, data + i))
					return -EFAULT;
				if (c == 'V')
					set_bit(WDT_OK_TO_CLOSE, &wdt_status);
			}
		}
		wdt_enable();
	}

	return len;
}

static struct watchdog_info ident = {
	.options = WDIOF_CARDRESET | WDIOF_MAGICCLOSE |
	    WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
	.identity = "PNX4008 Watchdog",
};

static int
pnx4008_wdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		  unsigned long arg)
{
	int ret = -ENOIOCTLCMD;
	int time;

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		ret = copy_to_user((struct watchdog_info *)arg, &ident,
				   sizeof(ident)) ? -EFAULT : 0;
		break;

	case WDIOC_GETSTATUS:
		ret = put_user(0, (int *)arg);
		break;

	case WDIOC_GETBOOTSTATUS:
		ret = put_user(boot_status, (int *)arg);
		break;

	case WDIOC_SETTIMEOUT:
		ret = get_user(time, (int *)arg);
		if (ret)
			break;

		if (time <= 0 || time > MAX_HEARTBEAT) {
			ret = -EINVAL;
			break;
		}

		heartbeat = time;
		wdt_enable();
		/* Fall through */

	case WDIOC_GETTIMEOUT:
		ret = put_user(heartbeat, (int *)arg);
		break;

	case WDIOC_KEEPALIVE:
		wdt_enable();
		ret = 0;
		break;
	}
	return ret;
}

static int pnx4008_wdt_release(struct inode *inode, struct file *file)
{
	if (test_bit(WDT_OK_TO_CLOSE, &wdt_status)) {
		wdt_disable();
	} else {
		printk(KERN_CRIT "WATCHDOG: Device closed unexpectdly - "
		       "timer will not stop\n");
	}

	clear_bit(WDT_IN_USE, &wdt_status);
	clear_bit(WDT_OK_TO_CLOSE, &wdt_status);

	return 0;
}

static struct file_operations pnx4008_wdt_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.write = pnx4008_wdt_write,
	.ioctl = pnx4008_wdt_ioctl,
	.open = pnx4008_wdt_open,
	.release = pnx4008_wdt_release,
};

static struct miscdevice pnx4008_wdt_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &pnx4008_wdt_fops,
};

static int pnx4008_wdt_remove(struct device *dev);

static int pnx4008_wdt_probe(struct device *dev)
{
	int ret;

	if (heartbeat < 1 || heartbeat > MAX_HEARTBEAT)
		heartbeat = DEFAULT_HEARTBEAT;

	printk(KERN_INFO PKMOD "PNX4008 Watchdog Timer: heartbeat %d sec\n",
	       heartbeat);

	if (!request_region(PNX4008_WDOG_BASE, 0x1000, MODULE_NAME)) {
		printk(KERN_ERR PKMOD
		       "watchdog registers are already in use\n");
		pnx4008_wdt_remove(dev);
		return -EBUSY;
	}
	set_bit(WDT_REGION_INITED, &wdt_status);

	ret = misc_register(&pnx4008_wdt_miscdev);
	if (ret < 0) {
		printk(KERN_ERR PKMOD "cannot register misc device\n");
		pnx4008_wdt_remove(dev);
		return ret;
	}

	wdt_clk_enable;
	boot_status = (__raw_readl(WDTIM_RES) & WDOG_RESET) ?
	    WDIOF_CARDRESET : 0;
	wdt_disable();		/*disable for now */
	set_bit(WDT_DEVICE_INITED, &wdt_status);

	return ret;
}

static int pnx4008_wdt_remove(struct device *dev)
{
	if (test_bit(WDT_DEVICE_INITED, &wdt_status)) {
		misc_deregister(&pnx4008_wdt_miscdev);
		clear_bit(WDT_DEVICE_INITED, &wdt_status);
	}
	if (test_bit(WDT_REGION_INITED, &wdt_status)) {
		release_region(PNX4008_WDOG_BASE, 0x1000);
		clear_bit(WDT_REGION_INITED, &wdt_status);
	}
	return 0;
}

static struct device_driver pnx4008_wdt_device_driver = {
	.name = "watchdog",
	.bus = &platform_bus_type,
	.probe = pnx4008_wdt_probe,
	.remove = pnx4008_wdt_remove,
};

static int __init pnx4008_wdt_init(void)
{
	return driver_register(&pnx4008_wdt_device_driver);
}

static void __exit pnx4008_wdt_exit(void)
{
	return driver_unregister(&pnx4008_wdt_device_driver);
}

module_init(pnx4008_wdt_init);
module_exit(pnx4008_wdt_exit);

MODULE_AUTHOR("MontaVista Software, Inc. <source@mvista.com>");
MODULE_DESCRIPTION("PNX4008 Watchdog Driver");

module_param(heartbeat, int, 0);
MODULE_PARM_DESC(heartbeat,
		 "Watchdog heartbeat period in seconds from 1 to "
		 __MODULE_STRING(MAX_HEARTBEAT) ", default "
		 __MODULE_STRING(DEFAULT_HEARTBEAT));

module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout,
		 "Set to 1 to keep watchdog running after device release");

MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
