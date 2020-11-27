 /*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
  * @file spi-dev.c
  * @brief This file contains the implementation for the /dev interface.
  *
  * @ingroup SPI
  */

/*
 * Includes
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <asm/arch/clock.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/device.h>

#include "spi-internal.h"
#include "registers.h"

extern unsigned long spi_ipg_clk;
extern unsigned long spi_max_bit_rate;

static void mxc_null_release(struct device *dev);

/*
 * Dummy device needed due to make able other modules compiled into the kernel (like Atlas)
 * initialize their devices after SPI driver is initialized itself.
 */
static struct platform_device mxc_spi_pseudo_device = {
	.name = "mxc_spi_pseudo_device",
	.id = 0,
	.dev = {
		.release = mxc_null_release,
		},
};

static void mxc_null_release(struct device *dev)
{
/* Just due to make LDM happy */
}

/*!
 * Global variable which indicates which SPI is used
 */
extern bool spi_present[CONFIG_SPI_NB_MAX];

#ifdef SPI_TEST_CODE
/*!
 * SPI major
 */
static int major_spi[CONFIG_SPI_NB_MAX];

/*!
 * The handle on current device
 */
static void *cur_handle;

#endif				//SPI_TEST_CODE

/*!
 * SPI major
 */
static int nb_clients;

/*!
 * The current config
 */
spi_config *cur_config;

#ifdef SPI_TEST_CODE

struct task_struct *kspi1[5];
struct task_struct *kspi2[5];
struct task_struct *kspi3[5];

static int W1(void *args)
{
	static spi_config the_config;
	static void *spi_id;
	char ptr[4];
	int value = 0xE6E00001;
	long i;
	the_config.module_number = SPI2;
	the_config.priority = HIGH;
	the_config.master_mode = true;
	the_config.bit_rate = 4000000;
	the_config.bit_count = 32;
	the_config.active_high_polarity = true;
	the_config.active_high_ss_polarity = true;
	the_config.phase = false;
	the_config.ss_low_between_bursts = true;
	the_config.ss_asserted = SS_0;
	the_config.tx_delay = 0;
	spi_id = spi_get_device_id((spi_config *) & the_config);

	printk("START Thread 1\n");
	for (i = 0; i < 5000; i++) {
		ptr[0] = (value & 0xFF000000) >> 24;
		ptr[1] = (value & 0x00FF0000) >> 16;
		ptr[2] = (value & 0x0000FF00) >> 8;
		ptr[3] = (value & 0xFF);
		spi_send_frame(ptr, 4, spi_id);
		schedule();
	}
	printk("END Thread 1 prio high\n");
	return 0;
}

static int W2(void *args)
{
	static spi_config the_config2;
	static void *spi_id2;
	char ptr2[4];
	int value = 0xE6E00000;
	long i;
	the_config2.module_number = SPI2;
	the_config2.priority = MEDIUM;
	the_config2.master_mode = true;
	the_config2.bit_rate = 4000000;
	the_config2.bit_count = 32;
	the_config2.active_high_polarity = true;
	the_config2.active_high_ss_polarity = true;
	the_config2.phase = false;
	the_config2.ss_low_between_bursts = true;
	the_config2.ss_asserted = SS_0;
	the_config2.tx_delay = 0;
	spi_id2 = spi_get_device_id((spi_config *) & the_config2);

	printk("START Thread 2\n");
	for (i = 0; i < 5000; i++) {
		ptr2[0] = (value & 0xFF000000) >> 24;
		ptr2[1] = (value & 0x00FF0000) >> 16;
		ptr2[2] = (value & 0x0000FF00) >> 8;
		ptr2[3] = (value & 0xFF);
		schedule();
		spi_send_frame(ptr2, 4, spi_id2);
	}
	printk("END Thread 2 prio medium\n");
	return 0;
}

static int W3(void *args)
{
	static spi_config the_config3;
	static void *spi_id3;
	char ptr3[4];
	int value = 0xE6E00001;
	long i;
	the_config3.module_number = SPI2;
	the_config3.priority = LOW;
	the_config3.master_mode = true;
	the_config3.bit_rate = 4000000;
	the_config3.bit_count = 32;
	the_config3.active_high_polarity = true;
	the_config3.active_high_ss_polarity = true;
	the_config3.phase = false;
	the_config3.ss_low_between_bursts = true;
	the_config3.ss_asserted = SS_0;
	the_config3.tx_delay = 0;
	spi_id3 = spi_get_device_id((spi_config *) & the_config3);

	printk("START Thread 3\n");
	for (i = 0; i < 5000; i++) {
		ptr3[0] = (value & 0xFF000000) >> 24;
		ptr3[1] = (value & 0x00FF0000) >> 16;
		ptr3[2] = (value & 0x0000FF00) >> 8;
		ptr3[3] = (value & 0xFF);
		schedule();
		spi_send_frame(ptr3, 4, spi_id3);
	}
	printk("END Thread 3 prio low\n");
	return 0;
}

/*!
 * This function allows writing on a SPI device.
 *
 * @param        file        pointer on the file
 * @param        buf         pointer on the buffer
 * @param        count       size of the buffer
 * @param        ppos        offset in the buffer
 * @return       This function returns the number of written bytes.
 */
static ssize_t spi_write(struct file *file, const char *buf,
			 size_t count, loff_t * ppos)
{
	char *ptr = NULL;
	int result, ret = 0;

	result = 0;

	if (cur_handle != NULL) {
		ptr = kmalloc(count, GFP_KERNEL);
		if (ptr == NULL) {
			ret = -ENOMEM;
			goto out;
		}

		result = copy_from_user(ptr, buf, count);
		if (result) {
			ret = -EFAULT;
			goto out;
		}
		result = spi_send_frame(ptr, count, cur_handle);
		if (result) {
			ret = result;
			goto out;
		}
		result = copy_to_user((void *)buf, ptr, count);
		if (result) {
			ret = -EFAULT;
			goto out;
		}
	}

      out:
	if (ptr != NULL) {
		kfree(ptr);
	}

	return ret;
}

/*!
 * This function implements IOCTL controls on a SPI device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int spi_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	int length, i;
	int result = 0;
	char *ptr;
	spi_frame *ptr_frame;
	spi_config *ptr_config;
	void **ptr_handle;

	if (_IOC_TYPE(cmd) != SPI_IOCTL)
		return -ENOTTY;

	switch (cmd) {
	case SPI_CONFIG:
		if ((cur_config = kmalloc(sizeof(spi_config), GFP_KERNEL))
		    == NULL) {
			return (-ENOMEM);
		}
		ptr_config = (spi_config *) (arg);
		copy_from_user(cur_config, ptr_config, sizeof(spi_config));
		cur_handle = spi_get_device_id(cur_config);
		if (cur_handle == NULL) {
			return -EFAULT;
		}
		break;
	case SPI_FREE:
		kfree(cur_config);
		cur_config = NULL;
		break;
	case SPI_GET_CUR_HANDLE:
		ptr_handle = (void **)(arg);
		*ptr_handle = cur_handle;
		break;
	case SPI_WRITE:
		ptr_frame = (spi_frame *) (arg);
		length = ptr_frame->length;
		if ((ptr = kmalloc(length, GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}

		if (copy_from_user(ptr, ptr_frame->data, length)) {
			return -EFAULT;
		}
		result = spi_send_frame(ptr, length, ptr_frame->dev_handle);

		if (copy_to_user(ptr_frame->data, ptr, length)) {
			return -EFAULT;
		}
		DBG_PRINTK("spi : ioctl() SPI_WRITE result = %d\n", result);
		kfree(ptr);
		break;
	case SPI_LOOPBACK_MODE:
		if (arg == 0) {
			spi_loopback_mode(((spi_config *) cur_handle)->
					  module_number, 0);
		} else {
			spi_loopback_mode(((spi_config *) cur_handle)->
					  module_number, 1);
		}
		break;
	case SPI_MULTI_CLIENT_TEST:
		for (i = 0; i < 5; i++) {
			kspi1[i] = kthread_create(W1, NULL, "%s", "kspi0");
			kspi2[i] = kthread_create(W2, NULL, "%s", "kspi1");
			kspi3[i] = kthread_create(W3, NULL, "%s", "kspi2");

			wake_up_process(kspi3[i]);
			wake_up_process(kspi2[i]);
			wake_up_process(kspi1[i]);
		}
	default:
		DBG_PRINTK("spi : ioctl() unknown !\n");
		return -ENOMEM;
		break;
	}
	return result;
}

/*!
 * This function implements the open method on a SPI device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int spi_open(struct inode *inode, struct file *file)
{
	if (nb_clients > MAX_SPI_CLIENTS) {
		return -1;
	} else {
		nb_clients++;
		return 0;
	}
}

/*!
 * This function implements the release method on a SPI device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int spi_free(struct inode *inode, struct file *file)
{
	nb_clients--;
	return 0;
}

/*!
 * This structure defines file operations for a SPI device.
 */
static struct file_operations spi_fops = {
	/*!
	 * the owner
	 */
	.owner = THIS_MODULE,
	/*!
	 * the write operation
	 */
	.write = spi_write,
	/*!
	 * the ioctl operation
	 */
	.ioctl = spi_ioctl,
	/*!
	 * the open operation
	 */
	.open = spi_open,
	/*!
	 * the release operation
	 */
	.release = spi_free,
};

#endif				//SPI_TEST_CODE

/*!
 * This function implements the init function of the SPI device.
 * This function is called when the module is loaded.
 *
 * @return       This function returns 0.
 */
static int __init spi_init(void)
{
	printk("CSPI driver\n");

	platform_device_register(&mxc_spi_pseudo_device);

	spi_ipg_clk = mxc_get_clocks(IPG_CLK);
	spi_max_bit_rate = (spi_ipg_clk / 4);

#if defined(CONFIG_MXC_SPI_SELECT1) && defined(SPI_TEST_CODE)
	spi_present[SPI1] = true;

	DBG_PRINTK("spi : registering spi1 as a char device\n");
	major_spi[SPI1] = register_chrdev(0, "spi1", &spi_fops);

	DBG_PRINTK("spi : creating devfs entry for spi1 \n");
	devfs_mk_cdev(MKDEV(major_spi[SPI1], 0),
		      S_IFCHR | S_IRUGO | S_IWUSR, "spi1");

	if (major_spi[SPI1] < 0) {
		printk(KERN_WARNING "Unable to get a major for spi1\n");
		return major_spi[SPI1];
	}

	printk("SPI1 loaded\n");
#else
	spi_present[SPI1] = false;
#endif				//CONFIG_MXC_SPI_SELECT1

#if defined(CONFIG_MXC_SPI_SELECT2) && defined(SPI_TEST_CODE)
	spi_present[SPI2] = true;

	DBG_PRINTK("spi : registering spi2 as a char device\n");
	major_spi[SPI2] = register_chrdev(0, "spi2", &spi_fops);

	DBG_PRINTK("spi : creating devfs entry for spi2 \n");
	devfs_mk_cdev(MKDEV(major_spi[SPI2], 0),
		      S_IFCHR | S_IRUGO | S_IWUSR, "spi2");

	if (major_spi[SPI2] < 0) {
		printk(KERN_WARNING "Unable to get a major for spi2\n");
		return major_spi[SPI2];
	}

	printk("SPI2 loaded\n");
#else
	spi_present[SPI2] = false;
#endif				//CONFIG_MXC_SPI_SELECT2

#if defined(CONFIG_MXC_SPI_SELECT3) && defined(SPI_TEST_CODE)
	spi_present[SPI3] = true;

	DBG_PRINTK("spi : registering spi3 as a char device\n");
	major_spi[SPI3] = register_chrdev(0, "spi3", &spi_fops);

	DBG_PRINTK("spi : creating devfs entry for spi3 \n");
	devfs_mk_cdev(MKDEV(major_spi[SPI3], 0),
		      S_IFCHR | S_IRUGO | S_IWUSR, "spi3");

	if (major_spi[SPI3] < 0) {
		printk(KERN_WARNING "Unable to get a major for spi3\n");
		return major_spi[SPI3];
	}

	printk("SPI3 loaded\n");
#endif				//CONFIG_MXC_SPI_SELECT3

	nb_clients = 0;

	return spi_hw_init();
}

/*!
 * This function implements the exit function of the SPI device.
 * This function is called when the module is unloaded.
 *
 */
static void __exit spi_exit(void)
{
#ifdef SPI_TEST_CODE
	int spi;
	char spi_name[5];
#endif
	platform_device_unregister(&mxc_spi_pseudo_device);

#ifdef SPI_TEST_CODE

	for (spi = 0; spi < CONFIG_SPI_NB_MAX; spi++) {
		if (spi_present[spi]) {
			sprintf(spi_name, "spi%1d", spi + 1);
			devfs_remove(spi_name);
			unregister_chrdev(major_spi[spi], spi_name);
		}
	}

#endif				//SPI_TEST_CODE

	spi_core_exit();

	DBG_PRINTK("spi : successfully unloaded\n");
}

/*
 * Module entry points
 */

subsys_initcall(spi_init);
module_exit(spi_exit);

MODULE_DESCRIPTION("SPI char device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
