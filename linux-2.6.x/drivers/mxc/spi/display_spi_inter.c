/*
 *  display_spi_inter.c - SPI Display Interface for inverted displays
 *
 *  Copyright (C) 2007 Motorola, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  This provides the SPI interface to a user space application that is used
 *  to send SPI commands to invert the main display on Paros.
 */

/*
 *  DATE         AUTHOR     COMMENT
 *  ----         ------     -------
 *  04/30/2007   Motorola   Initial version
 *  05/17/2007   Motorola   Added check for display_spi_interface property
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/spi_display.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/mothwcfg.h>

#include "spi.h"

#define DISPLAY_DEVICE_NAME "spi_display"
#define MOTHWCFG_PATH "/Aliases@0"

static atomic_t device_avail = ATOMIC_INIT (1);
static spi_config display_spi_inter_config;

static int display_spi_inter_ioctl(struct inode *inode, struct file *filp,
                                   unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct spi_command_frame *frame;
    int len;
    char *data_ptr;

    switch(cmd)
    {
        case SPI_COMMAND_WRITE:
            frame = (struct spi_command_frame*)arg;
            len = frame->len;

            if((len * 8) > display_spi_inter_config.bit_count)
            {
                printk(KERN_WARNING "display_spi_inter: invalid len: %d\n", len);
                return -EINVAL;
            }

            if((data_ptr = kmalloc(len, GFP_KERNEL)) == NULL)
                return -ENOMEM;

            if(copy_from_user(data_ptr, frame->data, len))
            {
                kfree(data_ptr);
                return -EFAULT;
            }

            ret = spi_send_frame(data_ptr, len, &display_spi_inter_config);

            kfree(data_ptr);
            break;
        default:
            printk(KERN_WARNING "display_spi_inter: ioctl() unknown\n");
            ret = -EINVAL;
            break;
    }

    return ret;
}

static int display_spi_inter_open(struct inode *inode, struct file *filp)
{
    if(!atomic_dec_and_test(&device_avail))
    {
        atomic_inc(&device_avail);
        return -EBUSY;
    }

    return 0;
}

static int display_spi_inter_release(struct inode *inode, struct file *filp)
{
    atomic_inc(&device_avail);

    return 0;
}

static struct file_operations display_device_fops = {
    .owner = THIS_MODULE,
    .open = display_spi_inter_open,
    .release = display_spi_inter_release,
    .ioctl = display_spi_inter_ioctl
};

static struct miscdevice display_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DISPLAY_DEVICE_NAME,
    .fops = &display_device_fops,
    .devfs_name = DISPLAY_DEVICE_NAME
};

static int __init display_spi_inter_init(void)
{
    MOTHWCFGNODE *node;
    int ret = 0;

    node = mothwcfg_get_node_by_path(MOTHWCFG_PATH);
    if(!node)
        return -ENODEV;

    if((ret = mothwcfg_read_prop(node, "display_spi_interface", NULL, 0)) < 0)
    {
        printk(KERN_ERR "Unable to read display_spi_interface property, error %d", ret);
        mothwcfg_put_node(node);
        return -ENODEV;
    }

    mothwcfg_put_node(node);
        
    printk("Loading Display SPI Interface\n");

    /* No reason to register with Power Management */
    
    ret = misc_register(&display_device);
    if(ret)
    {
        printk(KERN_ERR "Display SPI Interface device can't be loaded, ret=%d\n", ret);
        return ret;
    }

    display_spi_inter_config.module_number = SPI2;
    display_spi_inter_config.priority = HIGH;
    display_spi_inter_config.master_mode = 1;
    display_spi_inter_config.bit_rate = 4000000;
    display_spi_inter_config.bit_count = 24;
    display_spi_inter_config.active_high_polarity = 1;
    display_spi_inter_config.active_high_ss_polarity = 0;
    display_spi_inter_config.phase = 0;
    display_spi_inter_config.ss_low_between_bursts = 1;
    display_spi_inter_config.ss_asserted = SS_1;
    display_spi_inter_config.tx_delay = 0;

    return 0;
}

static void __exit display_spi_inter_exit(void)
{
    misc_deregister(&display_device);
}

module_init(display_spi_inter_init);
module_exit(display_spi_inter_exit);
