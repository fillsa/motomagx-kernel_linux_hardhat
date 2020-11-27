/* 
 * efuse_drv.c - E-Fuse Driver to support security APIs
 *
 *  Copyright 2006-2007 Motorola, Inc.
 *
 */

/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Initial revision.
 * 17-Jan-2007  Motorola        Fixed read order problem
 * 25-Apr-2007  Motorola        Added SCS1 interface.
 */


#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/config.h>
#include <linux/module.h>
#include <asm-generic/errno.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/efuse_api.h>
#include "efuse_drv.h"

/* Forward Declarations */

void clear_fuse_status(void);

int get_fuse_status(void);

static int efuse_probe(struct device *dev);

static int efuse_open(struct inode *inode, struct file *file);

static int efuse_release(struct inode *inode, struct file *file);

static int efuse_ioctl (struct inode *inode, struct file * file, unsigned int cmd, unsigned long arg);

#ifdef CONFIG_PM
static int efuse_suspend(struct device *dev, ulong state, ulong level);

static int efuse_resume(struct device *dev, ulong level);
#endif

/* Struct declarations */

static struct platform_device efuse_device = {
    .name = EFUSE_MOD_NAME,
    .id   = 0,
    .dev = {
        .release = (void *)efuse_release,
    },
};

static struct file_operations efuse_fops = {
    .owner   = THIS_MODULE,
    .open    = efuse_open,
    .release = efuse_release,
    .ioctl   = efuse_ioctl
};

static struct device_driver  mxc_efuse = {
    .name    = EFUSE_MOD_NAME,
    .bus     = &platform_bus_type,
    .probe   = efuse_probe,
#ifdef CONFIG_PM
    .suspend = (void *)efuse_suspend,
    .resume  = (void *)efuse_resume
#endif
};

/* The IIM control/status register */

static volatile iim_regs_t *iim_registers = (iim_regs_t *) IO_ADDRESS(IIM_BASE_ADDR);

static int dev_major; /* The major device number */

static struct semaphore stat_sem;

/* 
 * Retrieve the IIM status register value
 *
 * Note that although there is a semaphore which serializes the ARM access to the 
 * efuses, there is no synchronization between the ARM and the DSP.  The DSP code
 * currently accesses the fuses only at boot time, so there is no conflict.
 *
 */

int get_fuse_status(void)
{
    return (int)iim_registers->err;
}

/* Clear the IIM status register */
void clear_fuse_status(void)
{
    iim_registers->err = iim_registers->err;
    return;
}

#ifdef CONFIG_PM
static int efuse_suspend(struct device *dev, ulong state, ulong level)
{
    /* There is nothing to do here for this driver so just return 0 */
    return 0;
}

static int efuse_resume(struct device *dev, ulong level)
{
    /* There is nothing to do here for this driver so just return 0 */
    return 0;
}
#endif /* CONFIG_PM */

static int efuse_open(struct inode *inode, struct file *file)
{
#ifdef EFUSE_DEBUG
    printk("E-Fuse open called\n");
#endif
    return 0;
}

static int efuse_release(struct inode *inode, struct file *file)
{
#ifdef EFUSE_DEBUG
    printk("E-Fuse release called\n");
#endif
    return 0;
}

/* Read the e-fuse cache into the provided buffer */
efuse_status_t efuse_read (int bank, int offset, int len, 
                       unsigned char *buf)
{
    int i;
    int sret;
    efuse_status_t ret = EFUSE_SUCCESS;
    
    /* We start at the last byte of the field and read backwards */
    volatile unsigned char *fuses = (unsigned char *)
        IO_ADDRESS((IIM_BASE_ADDR + ((bank * MAX_EFUSE_OFFSET) + 
            EFUSE_BANK_0_OFFSET)  + offset) + ((len - 1) * 4));

#ifdef EFUSE_DEBUG
    printk("E-Fuse read bank= %x, offset= %x len= %d adx = %x\n", bank, offset, len, fuses);
#endif

    if((len < 1) || (len > MAX_EFUSE_READ_LEN))
    {
        return -EINVAL;
    }

    sret = down_interruptible(&stat_sem);
    if(sret)
    {
        return -EINTR;
    }

    /* Fuses are 32 bit aligned so read 1 of every 4 bytes */
    for (i = 0; i < len; i++)
    {
        clear_fuse_status(); /* Make sure we have a clear error status */
        buf[i] = *fuses;

	ret = get_fuse_status();

	if( ret != EFUSE_SUCCESS)
        {
            clear_fuse_status();
            up(&stat_sem);
            return ret;
        }
        fuses -= 4;
    }
    up(&stat_sem);
    return ret; 
}

/* Return the value stored in SCS1 */
efuse_status_t efuse_get_scs1 (unsigned char *buf)
{
    int sret;
    int ret;
    sret = down_interruptible(&stat_sem);
    if(sret)
    {
        return -EINTR;
    }
    clear_fuse_status(); /* Make sure we have a clear error status */
    *buf = (unsigned char)iim_registers->scs1;
    ret = get_fuse_status();
    clear_fuse_status();
    up(&stat_sem);
    return ret; 
}

static int efuse_probe(struct device *dev)
{
        return 0;
}

static int efuse_ioctl (struct inode *inode, struct file * file, 
                        unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned char  *buf = NULL;
    efuse_data_t *edata = NULL;
    efuse_data_t *usr_edata = NULL;
    scs_data_t *sdata = NULL;
    scs_data_t *usr_sdata = NULL;

    switch(cmd)
    {
        case EFUSE_READ:
            edata = (efuse_data_t *)kmalloc(sizeof(efuse_data_t), GFP_KERNEL);

            if(!edata)
            {
                return -ENOMEM;
            }
            /* Copy the data passed from the user */
            if(copy_from_user((void *)edata, (void *)arg, sizeof(efuse_data_t)))
            {
                kfree(edata);
                return -EFAULT;
            }
        
            buf = (unsigned char *)kmalloc(edata->length, GFP_KERNEL);
            if(!buf)
            {
                kfree(edata);
                return -ENOMEM;
            }

  
#ifdef EFUSE_DEBUG
            printk("E-Fuse ioctl cmd = %d,\n", cmd);
#endif
            if ((edata->offset < 0) || 
                ((edata->offset + edata->length) >= MAX_EFUSE_OFFSET))
            {
                kfree(buf);
                kfree(edata);
                return -EINVAL;
            }
            if ((edata->bank < 0) || (edata->bank >= MAX_EFUSE_BANK))
            {
                kfree(buf);
                kfree(edata);
                return -EINVAL;
            }

            ret = efuse_read(edata->bank, edata->offset, edata->length, 
                             buf);

            edata->status = ret;

            /* Copy the e-fuse data back to the user */
            usr_edata = (efuse_data_t *)arg;

            if(copy_to_user(edata->buf, buf, edata->length))
            {
                ret = -EFAULT;
            }
	    else if(put_user(ret, &(usr_edata->status)))
            {
                ret = -EFAULT;
            }


            kfree(buf);
            kfree(edata);
            break;

	case GET_SCS1:
            sdata = (scs_data_t *)kmalloc(sizeof(scs_data_t), GFP_KERNEL);
 
            if(!sdata)
            {
                return -ENOMEM;
            }
            /* Copy the data passed from the user */
            if(copy_from_user((void *)sdata, (void *)arg, sizeof(scs_data_t)))
            {
                kfree(sdata);
                return -EFAULT;
            }
        
            buf = (unsigned char *)kmalloc(sizeof(buf), GFP_KERNEL);
            if(!buf)
            {
                kfree(sdata);
                return -ENOMEM;
            }

  
#ifdef EFUSE_DEBUG
            printk("E-Fuse ioctl cmd = %d,\n", cmd);
#endif
	    ret = efuse_get_scs1((unsigned char *)buf);

            sdata->status = ret;
            usr_sdata = (scs_data_t *)arg;

            /* Copy the SCS1 data back to the user */
            if(copy_to_user(sdata->buf, buf, sizeof(buf)))
            {
                ret = -EFAULT;
            }
            else if(put_user(ret, &(usr_sdata->status)))
            {
                ret = -EFAULT;
            }
            
            kfree(buf);
            kfree(sdata);
            break;

        default:
#ifdef EFUSE_DEBUG
            printk("E-Fuse driver: ioctl: unknown cmd=%d,\n", cmd);
#endif
            return -EINVAL;
    }
    return ret;
}

static int __init efuse_init(void)
{
    int ret;

#ifdef EFUSE_DEBUG
    printk("E-Fuse Driver init entry\n");
#endif

    /* Register the driver and obtain the major number */
    dev_major = register_chrdev(0, EFUSE_MOD_NAME, &efuse_fops);

    if (dev_major < 0)
    {
        printk(KERN_WARNING "Registering the efuse device failed\n");
        return dev_major;
    }

    /* Create the device node */
    ret = devfs_mk_cdev(MKDEV(dev_major, 0), 
                  S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH , EFUSE_MOD_NAME);

    if (ret != 0)
    {
        unregister_chrdev(dev_major, EFUSE_MOD_NAME);
        printk(KERN_WARNING "Creating the efuse device failed\n");
        return ret;
    }

    /* Power handling registration */
    ret = driver_register(&mxc_efuse);

    if (ret != 0)
    {
        printk(KERN_WARNING "Registering the efuse driver failed\n");
        devfs_remove(EFUSE_MOD_NAME);
        unregister_chrdev(dev_major, EFUSE_MOD_NAME);
        return ret;
    }

    ret = platform_device_register(&efuse_device);

    if (ret != 0)
    {
        printk(KERN_WARNING "Registering the efuse device failed\n");
        driver_unregister(&mxc_efuse);
        devfs_remove(EFUSE_MOD_NAME);
        unregister_chrdev(dev_major, EFUSE_MOD_NAME);
        return ret;
    }

    sema_init(&stat_sem,1);

    return 0;
}

/* 
 * efuse_cleanup is called when the module is removed from the kernel.
 * It unregisters the e-fuse driver and frees any allocated memory
 */

static void __exit efuse_cleanup(void)
{
#ifdef EFUSE_DEBUG
    printk("E-Fuse Driver cleanup\n");
#endif

    driver_unregister(&mxc_efuse);
    devfs_remove(EFUSE_MOD_NAME);
    unregister_chrdev(dev_major, EFUSE_MOD_NAME);
    platform_device_unregister(&efuse_device);
 
}

module_init(efuse_init);
module_exit(efuse_cleanup);

EXPORT_SYMBOL(efuse_read);
EXPORT_SYMBOL(efuse_get_scs1);

MODULE_AUTHOR ("Motorola");
MODULE_DESCRIPTION ("Motorola E-Fuse driver");
MODULE_LICENSE ("GPL");

