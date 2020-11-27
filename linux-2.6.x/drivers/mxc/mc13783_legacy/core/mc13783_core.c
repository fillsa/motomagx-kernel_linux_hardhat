/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

/*!
 * @file mc13783_core.c
 * @brief This is the main file for the mc13783 driver.
 *
 * It contains devfs file system
 * and initialization of the driver.
 *
 * @ingroup MC13783
 */

#define __INIT_TAB_DEF

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/interrupt.h>
#include <asm/hardware.h>
#include <asm/arch/gpio.h>

#include "mc13783_register.h"
#include "mc13783_event.h"
#include "mc13783_external.h"

//#define DEBUG

#ifdef DEBUG
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

#define IT_MC13783_PORT   			0

static int mc13783_major;
static int mxc_mc13783_core_detected;
static int mxc_mc13783_ic_version;

extern void gpio_mc13783_active(void);
extern void gpio_mc13783_clear_int(void);

/*
 * Global variables
 */

DECLARE_WORK(mc13783_wq_task, (void *)mc13783_wq_handler, (unsigned long)0);

/*!
 * This function is called when mc13783 interrupt occurs on the processor.
 * It is the interrupt handler for the mc13783 module.
 *
 * @param        irq        the irq number
 * @param        dev_id     the pointer on the device
 * @param        regs       the interrupt parameters
 *
 * @return       The function returns IRQ_RETVAL(1) when handled.
 */
static irqreturn_t mc13783_irq_handler(int irq, void *dev_id,
				       struct pt_regs *regs)
{
	gpio_mc13783_clear_int();

	/* prepare a task */
	TRACEMSG(_K_D("* mc13783 irq handler top half *"));
	schedule_work(&mc13783_wq_task);
	return IRQ_RETVAL(1);
}

/*!
 * This function implements IOCTL controls on a MC13783 device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int mc13783_ioctl(struct inode *inode, struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	register_info reg_info;
#ifdef __TEST_CODE_ENABLE__
	type_event_notification event_sub;
#endif				/* __TEST_CODE_ENABLE__ */

	if (_IOC_TYPE(cmd) != 'P')
		return -ENOTTY;

	if (copy_from_user(&reg_info, (register_info *) arg,
			   sizeof(register_info))) {
		return -EFAULT;
	}

	switch (cmd) {
	case MC13783_READ_REG:
		TRACEMSG(_K_D("read reg"));
		CHECK_ERROR(mc13783_read_reg(0,
					     reg_info.reg,
					     &(reg_info.reg_value)));
		break;
	case MC13783_WRITE_REG:
		TRACEMSG(_K_D("write reg %d %x"), reg_info.reg,
			 reg_info.reg_value);
		CHECK_ERROR(mc13783_write_reg(0,
					      reg_info.reg,
					      &(reg_info.reg_value)));
		break;
#ifdef __TEST_CODE_ENABLE__
	case MC13783_SUBSCRIBE:
		TRACEMSG(_K_I("event subscribe"));
		get_event(reg_info.event, &event_sub);
		TRACEMSG(_K_I(" *** event sub %d"), event_sub.event);
		CHECK_ERROR(mc13783_event_subscribe(event_sub));
		TRACEMSG(_K_I("subscribe done"));
		break;
	case MC13783_UNSUBSCRIBE:
		TRACEMSG(_K_D("event unsubscribe"));
		get_event(reg_info.event, &event_sub);
		TRACEMSG(_K_I(" *** event unsub %d"), event_sub.event);
		CHECK_ERROR(mc13783_event_unsubscribe(event_sub));
		break;
#endif				/* __TEST_CODE_ENABLE__ */
	default:
		TRACEMSG(_K_D("%d unsupported ioctl command"), (int)cmd);
		return -EINVAL;
	}

	if (copy_to_user((register_info *) arg, &reg_info,
			 sizeof(register_info))) {
		return -EFAULT;
	}

	return ERROR_NONE;
}

/*!
 * This function implements the open method on a MC13783 device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int mc13783_open(struct inode *inode, struct file *file)
{
	TRACEMSG(_K_D("mc13783 : mc13783_open()"));
	return 0;
}

/*!
 * This function implements the release method on a MC13783 device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */

static int mc13783_free(struct inode *inode, struct file *file)
{
	TRACEMSG(_K_D("mc13783 : mc13783_free()"));
	return 0;
}

/*!
 * This structure defines file operations for a MC13783 device.
 */
static struct file_operations mc13783_fops = {
	.owner = THIS_MODULE,
	.ioctl = mc13783_ioctl,
	.open = mc13783_open,
	.release = mc13783_free,
};

/*!
 * This function tells whether mc13783 core driver
 * has been correctly detected or not. Correct detection
 * depends on version supported by Kernel (this is done at
 * compilation time) and version actually detected when requesting
 * the mc13783 board (when loading the mc13783 core driver. See mc13783_init
 * function for more information)
 *
 * @return       This function returns 1 if mc13783 version supported
 *		 by kernel and mc13783 version detected match,
 *		 0 otherwise.
 */
int mc13783_core_loaded(void)
{
	return mxc_mc13783_core_detected;
}

/*!
 * This function returns the mc13783 version. Version of IC is detected
 * at initialization time by reading a specific register in mc13783 IC.
 * See mc13783_init function for more information.
 *
 * @return       This function returns  version of mc13783 IC or -1 if mc13783 was
 *		 not detected properly.
 */
int mxc_mc13783_get_ic_version(void)
{
	return mxc_mc13783_ic_version;
}

/*!
 * This function implements the init function of the MC13783 device.
 * This function is called when the module is loaded.
 * It init the interrupt handler and write initialization value of mc13783.
 *
 * @return       This function returns 0.
 */
static int __init mc13783_init(void)
{
	int ret = 0, i = 0, rev_id = 0;
	int rev1 = 0, rev2 = 0, finid = 0;

	mxc_mc13783_ic_version = -1;

	mc13783_major = register_chrdev(0, "mc13783", &mc13783_fops);
	if (mc13783_major < 0) {
		printk(KERN_INFO "unable to get a major for mc13783");
		return mc13783_major;
	}

	devfs_mk_cdev(MKDEV(mc13783_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "mc13783");

	init_register_access_wait();

	/* init all register */
	DPRINTK("init all register value");

	for (i = 0; tab_init_reg[i][0] != -1; i++) {
		DPRINTK(("reg %d, value %x"), tab_init_reg[i][0],
			tab_init_reg[i][1]);
		mc13783_write_reg(0, tab_init_reg[i][0], &tab_init_reg[i][1]);
	}

	/* install our own mc13783 handler */
	mc13783_init_event_and_it_gpio();

	/* IOMux configuration */
	/* initialize GPIO for Interrupt of mc13783 */
	gpio_mc13783_active();

	mc13783_read_reg(0, REG_REVISION, &rev_id);

	rev1 = (rev_id & 0x018) >> 3;
	rev2 = (rev_id & 0x007);
	finid = (rev_id & 0x01E00) >> 9;

	mxc_mc13783_core_detected = 1;

	if (rev1 == 0) {
		printk(KERN_WARNING "mc13783 core: Access failed\n");
		mxc_mc13783_core_detected = 0;
	} else {
		printk(KERN_INFO "mc13783 Version: %d.%d\n", rev1, rev2);
		printk(KERN_INFO "mc13783 Final version: %X found\n", finid);
	}

	if (mxc_mc13783_core_detected == 0) {
		printk(KERN_WARNING "mc13783 core: Loading failed\n");

		devfs_remove("mc13783");
		unregister_chrdev(mc13783_major, "mc13783");
		return -1;
	}

	set_irq_type(MXC_PMIC_INT_LINE, IRQT_RISING);
	ret = request_irq(MXC_PMIC_INT_LINE, mc13783_irq_handler, 0, 0, 0);
	if (ret != 0) {
		printk(KERN_WARNING "gpio1: irq%d error.", MXC_PMIC_INT_LINE);
		return -1;
	}

	mxc_mc13783_ic_version = ((rev1 * 10) + rev2);

	printk(KERN_INFO "mc13783 core loaded. Current version is %d\n",
	       mxc_mc13783_ic_version);
	return 0;
}

/*!
 * This function implements the exit function of the MC13783 device.
 * This function is called when the module is unloaded.
 *
 */
static void __exit mc13783_exit(void)
{
	devfs_remove("mc13783");
	unregister_chrdev(mc13783_major, "mc13783");

	free_irq(MXC_PMIC_INT_LINE, 0);

	mxc_mc13783_core_detected = 0;
	mxc_mc13783_ic_version = -1;

	printk(KERN_INFO "mc13783 core unloaded\n");
}

subsys_initcall(mc13783_init);
module_exit(mc13783_exit);

EXPORT_SYMBOL(mxc_mc13783_get_ic_version);
EXPORT_SYMBOL(mc13783_core_loaded);

MODULE_DESCRIPTION("mc13783 device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
