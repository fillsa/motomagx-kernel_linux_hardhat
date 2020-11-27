/*
 *  linux/drivers/mmc/mmc_sysfs.c
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  MMC sysfs/driver model support.
 */
/*
 * Copyright (C) 2006 - 2008 Motorola
 * Date         Author                  Comment
 * 2006-11-23	Motorola		Add sysfs and proc interface for megasim boot/halt.
 * 2007-11-05	Motorola		Add sysfs interface and call back for repair fat.
 * 2007-12-14   Motorola                Remove extra  copyright line
 * 2008-02-15   Motorola                redesign repair fat calback, run once when many fat panic
 * 2008-02-16   Motorola                Tracing FAT panic
 * 2008-06-25	Motorola		Fix 4bit issue
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/blkdev.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <linux/jiffies.h>
#include <linux/poll.h>

#include "mmc.h"

#define dev_to_mmc_card(d)	container_of(d, struct mmc_card, dev)
#define to_mmc_driver(d)	container_of(d, struct mmc_driver, drv)
#define cls_dev_to_mmc_host(d)	container_of(d, struct mmc_host, class_dev)

extern void register_repairfat_callback(void (*callback_fun)(struct super_block *sb));
extern void unregister_repairfat_callback(void);

static void mmc_release_card(struct device *dev)
{
	struct mmc_card *card = dev_to_mmc_card(dev);

	kfree(card);
}

/*
 * This currently matches any MMC driver to any MMC card - drivers
 * themselves make the decision whether to drive this card in their
 * probe method.  However, we force "bad" cards to fail.
 */
static int mmc_bus_match(struct device *dev, struct device_driver *drv)
{
	struct mmc_card *card = dev_to_mmc_card(dev);
	return !mmc_card_bad(card);
}

static int
mmc_bus_hotplug(struct device *dev, char **envp, int num_envp, char *buf,
		int buf_size)
{
	struct mmc_card *card = dev_to_mmc_card(dev);
	struct platform_device *pdev = to_platform_device(card->dev.parent);

	char ccc[13];
	int i = 0;

#define add_env(fmt,val)						\
	({								\
		int len, ret = -ENOMEM;					\
		if (i < num_envp) {					\
			envp[i++] = buf;				\
			len = snprintf(buf, buf_size, fmt, val) + 1;	\
			buf_size -= len;				\
			buf += len;					\
			if (buf_size >= 0)				\
				ret = 0;				\
		}							\
		ret;							\
	})

	for (i = 0; i < 12; i++)
		ccc[i] = card->csd.cmdclass & (1 << i) ? '1' : '0';
	ccc[12] = '\0';

	i = 0;
	add_env("MMC_CCC=%s", ccc);
	add_env("MMC_MANFID=%06x", card->cid.manfid);
	add_env("MMC_NAME=%s", mmc_card_name(card));
	add_env("MMC_OEMID=%04x", card->cid.oemid);
	add_env("SLOT=%d", pdev->id);

	return 0;
}

static int mmc_bus_suspend(struct device *dev, u32 state)
{
	struct mmc_driver *drv = to_mmc_driver(dev->driver);
	struct mmc_card *card = dev_to_mmc_card(dev);
	int ret = 0;

	if (dev->driver && drv->suspend)
		ret = drv->suspend(card, state);
	return ret;
}

static int mmc_bus_resume(struct device *dev)
{
	struct mmc_driver *drv = to_mmc_driver(dev->driver);
	struct mmc_card *card = dev_to_mmc_card(dev);
	int ret = 0;

	if (dev->driver && drv->resume)
		ret = drv->resume(card);
	return ret;
}

static struct bus_type mmc_bus_type = {
	.name		= "mmc",
	.match		= mmc_bus_match,
	.hotplug	= mmc_bus_hotplug,
	.suspend	= mmc_bus_suspend,
	.resume		= mmc_bus_resume,
};


static int mmc_drv_probe(struct device *dev)
{
	struct mmc_driver *drv = to_mmc_driver(dev->driver);
	struct mmc_card *card = dev_to_mmc_card(dev);

	return drv->probe(card);
}

static int mmc_drv_remove(struct device *dev)
{
	struct mmc_driver *drv = to_mmc_driver(dev->driver);
	struct mmc_card *card = dev_to_mmc_card(dev);

	drv->remove(card);

	return 0;
}


/**
 *	mmc_register_driver - register a media driver
 *	@drv: MMC media driver
 */
int mmc_register_driver(struct mmc_driver *drv)
{
	drv->drv.bus = &mmc_bus_type;
	drv->drv.probe = mmc_drv_probe;
	drv->drv.remove = mmc_drv_remove;
	return driver_register(&drv->drv);
}

EXPORT_SYMBOL(mmc_register_driver);

/**
 *	mmc_unregister_driver - unregister a media driver
 *	@drv: MMC media driver
 */
void mmc_unregister_driver(struct mmc_driver *drv)
{
	drv->drv.bus = &mmc_bus_type;
	driver_unregister(&drv->drv);
}

EXPORT_SYMBOL(mmc_unregister_driver);


#define MMC_ATTR(name, fmt, args...)					\
static ssize_t mmc_dev_show_##name (struct device *dev, char *buf)	\
{									\
	struct mmc_card *card = dev_to_mmc_card(dev);			\
	return sprintf(buf, fmt, args);					\
}									\
static DEVICE_ATTR(name, S_IRUGO, mmc_dev_show_##name, NULL)

MMC_ATTR(cid, "%08x%08x%08x%08x\n", card->raw_cid[0], card->raw_cid[1],
	card->raw_cid[2], card->raw_cid[3]);
MMC_ATTR(csd, "%08x%08x%08x%08x\n", card->raw_csd[0], card->raw_csd[1],
	card->raw_csd[2], card->raw_csd[3]);
MMC_ATTR(scr, "%08x%08x\n", card->raw_scr[0], card->raw_scr[1]);
MMC_ATTR(date, "%02d/%04d\n", card->cid.month, card->cid.year);
MMC_ATTR(fwrev, "0x%x\n", card->cid.fwrev);
MMC_ATTR(hwrev, "0x%x\n", card->cid.hwrev);
MMC_ATTR(manfid, "0x%06x\n", card->cid.manfid);
MMC_ATTR(name, "%s\n", card->cid.prod_name);
MMC_ATTR(oemid, "0x%04x\n", card->cid.oemid);
MMC_ATTR(serial, "0x%08x\n", card->cid.serial);

static struct device_attribute *mmc_dev_attributes[] = {
	&dev_attr_cid,
	&dev_attr_csd,
	&dev_attr_scr,
	&dev_attr_date,
	&dev_attr_fwrev,
	&dev_attr_hwrev,
	&dev_attr_manfid,
	&dev_attr_name,
	&dev_attr_oemid,
	&dev_attr_serial,
};

/*
 * Internal function.  Initialise a MMC card structure.
 */
void mmc_init_card(struct mmc_card *card, struct mmc_host *host)
{
	memset(card, 0, sizeof(struct mmc_card));
	card->host = host;
	device_initialize(&card->dev);
	card->dev.parent = card->host->dev;
	card->dev.bus = &mmc_bus_type;
	card->dev.release = mmc_release_card;
}

/*
 * Internal function.  Register a new MMC card with the driver model.
 */
int mmc_register_card(struct mmc_card *card)
{
	int ret, i;

	snprintf(card->dev.bus_id, sizeof(card->dev.bus_id),
		 "%s:%04x", mmc_hostname(card->host), card->rca);

	ret = device_add(&card->dev);
	if (ret == 0)
		for (i = 0; i < ARRAY_SIZE(mmc_dev_attributes); i++)
			device_create_file(&card->dev, mmc_dev_attributes[i]);

	return ret;
}

/*
 * Internal function.  Unregister a new MMC card with the
 * driver model, and (eventually) free it.
 */
void mmc_remove_card(struct mmc_card *card)
{
	if (mmc_card_present(card))
		device_del(&card->dev);

	put_device(&card->dev);
}


static void mmc_host_classdev_release(struct class_device *dev)
{
	struct mmc_host *host = cls_dev_to_mmc_host(dev);
	kfree(host);
}

DECLARE_COMPLETION(fatpanic_completion);
//static struct completion fatpanic_completion;
static char device_name[BDEVNAME_SIZE];
static volatile int panic_script_running=0;
spinlock_t panic_script_lock=SPIN_LOCK_UNLOCKED;

static void call_repairfat(char *dev_name)
{
#define NUM_ENVP 32
	int i=0,ret;
	char *argv[3];
	char **envp=NULL;
	char script_path[]="/etc/repairfat.sh";

	spin_lock(&panic_script_lock);
	if( !panic_script_running )
	{
		panic_script_running = 1;
	}
	else
	{
		spin_unlock(&panic_script_lock);
		return;
	}

	spin_unlock(&panic_script_lock);
	
	envp = kmalloc(NUM_ENVP * sizeof (char *), GFP_KERNEL);
       	if (!envp)
               	return;
       	memset (envp, 0x00, NUM_ENVP * sizeof (char *));

	argv[0]= script_path;
	argv[1]= dev_name;
	argv[2]= NULL;
	
        envp [i++] = "HOME=/";
	envp [i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
	//envp [i++] = 
	printk("mxclay: begin to call usermodehelper %s %s \n", argv[0], argv[1]);
	ret = call_usermodehelper(argv[0], argv, envp, 0 );
	printk( "%s: call repairfat.sh returned %d \n", __FUNCTION__,  ret);

	spin_lock(&panic_script_lock);
	panic_script_running=0;
	spin_unlock(&panic_script_lock);
	kfree(envp);
}

static void wakeup_mmc_repairfat(struct super_block *sb)
{
	int part = 0;
	struct block_device *panic_bdev = sb->s_bdev;

	unregister_repairfat_callback();//LYN when repaired, re-register the callback, avoid too many invoked

	printk("%s: mxclay repairfat \n", __FUNCTION__);

	if(panic_bdev && panic_bdev->bd_disk && 
			!(strncmp("mmc", panic_bdev->bd_disk->devfs_name, 3))) {
		printk("wmnbmh:repairfat %s\n", panic_bdev->bd_disk->devfs_name);
		part = MINOR(panic_bdev->bd_dev) - panic_bdev->bd_disk->first_minor;
		snprintf(device_name, BDEVNAME_SIZE, "/dev/%s/part%d", 
				panic_bdev->bd_disk->devfs_name, part);	
		printk("wmnbmh:repairfat device= %s\n", device_name);

		call_repairfat(device_name);	
			
		complete(&fatpanic_completion);
	}
}

static ssize_t mmc_repairfs_show(struct class_device *dev, char *buf)
{
	INIT_COMPLETION(fatpanic_completion);
	wait_for_completion(&fatpanic_completion);
	printk("%s: mxclay %s\n", __FUNCTION__,device_name);

	if (!copy_to_user(buf, device_name, sizeof(device_name)))
		return 0;

	return -1; /* error */
}

static ssize_t mmc_repairfs_store(struct class_device *dev, const char *buf, size_t size)
{
	char devname[BDEVNAME_SIZE];
	/* for furture use */
	if( memcmp(buf, "panic_",6) == 0 )
	{
		memset(devname,0,BDEVNAME_SIZE);
		memcpy(devname,	buf+6, min(size-6,(size_t)(BDEVNAME_SIZE-1)));
		devname[size-6]=0;
		printk("%s: szq devname=%s\n",__FUNCTION__,devname);
		call_repairfat(devname);
	}
	return size;
}

static struct class_device_attribute attr_repairfs[] = {
	__ATTR(repairfs, S_IRUGO | S_IWUGO, mmc_repairfs_show, mmc_repairfs_store),
	__ATTR_NULL
};


#ifdef CONFIG_MOT_FEAT_MEGASIM
#define PROC_MMC_MEGASIM_NAME         "megasim"

extern int megasim_restart_mmc(unsigned int flag);
static struct proc_dir_entry *proc_megasim_file;

/* containing the /proc/mmc/megasim open state */
bool megasim_proc_opened;

/*
 * Mutex use to protect the megasim_proc_opened variable
 */
DECLARE_MUTEX(megasim_proc_opened_mutex);

static int megasim_proc_open(struct inode *inode, struct file *file)
{
        /* take mutex */
        if(down_interruptible(&megasim_proc_opened_mutex))
        {
                /* process received signal while waiting for /proc/mmc/megasim opened mutex. Exiting. */
                return -EINTR;
        }

        /* Check if the proc is not already open */
        if (megasim_proc_opened == true)
        {
                /* /proc/mmc/megasim control already opened. */

                /* Release mutex */
                up(&megasim_proc_opened_mutex);
                return -EBUSY;
        }

        /* Set the opened state */
        megasim_proc_opened = true;

        /* Release mutex */
        up(&megasim_proc_opened_mutex);

        return 0;
}

static int megasim_proc_release(struct inode *inode, struct file *file)
{
        /* take mutex */
        if(down_interruptible(&megasim_proc_opened_mutex))
        {
                /* process received signal while waiting for megasim opened mutex. Exiting. */
                return -EINTR;
        }

        /* Reset the opened state */
        megasim_proc_opened = false;

        /* Release mutex */
        up(&megasim_proc_opened_mutex);

        return 0;
}

ssize_t megasim_proc_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{  
        return 0;
}

unsigned int megasim_proc_poll(struct file *file, poll_table *wait)
{
    unsigned int retval = 0;

    retval = (POLLIN | POLLRDNORM);

    return retval;
}

static int megasim_proc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
        /* Handle the request. */
        switch(cmd)
        {
                case MEGASIM_BOOTUP:
			ret = megasim_restart_mmc(1);
                    	break;
    
                case MEGASIM_RESET_REQ:
			ret = megasim_restart_mmc(0);
                    	break;
    
                default: /* This shouldn't be able to happen, but just in case... */
                    printk("PICO_MEGASIM: unsupported command=%d\n", cmd);
                    /* unsupported /proc/mmc/megasim ioctl command */
                    return -ENOTTY;
        }

        return ret;
}


/* This structure defines the file operations for the /proc/mmc/megasim device */
static struct file_operations megasim_proc_fops =
{
    .open    = megasim_proc_open,
    .ioctl   = megasim_proc_ioctl,
    .release = megasim_proc_release,
    .poll    = megasim_proc_poll,
    .read    = megasim_proc_read,
};



static ssize_t mmc_megasim_show(struct class_device *dev, char *buf)
{
	/* for furture use */
	return 0;
}

static ssize_t mmc_megasim_store(struct class_device *dev, const char *buf, size_t size)
{
	int ret = 0;
	if (memcmp(buf, "boot", 4) == 0) {
		ret = megasim_restart_mmc(1);	
	} else if (memcmp(buf, "halt", 4) == 0) {
		ret = megasim_restart_mmc(0);	
	} else {
		printk("megasim reset:illegal command\n");	
		ret = -EINVAL;
	}

	if (ret) {
		return ret;
	}

	return size;
}
#endif /* CONFIG_MOT_FEAT_MEGASIM */


/*
 *  LYN repairfat callback enable/disable interface
 */
static ssize_t repairfat_proc_read(struct file *file, char *buf, size_t count, loff_t * pos)
{
        int len = 0;
        int index;
	char page[128];
	

        len = 0;
        index = (*pos)++;

        switch(index) {

        case 0:
                len += sprintf ((char *) page + len, "repairfat \n");
                break;
	case 1:
		len += sprintf ((char *) page + len, " 1 enable fat panic repair, 0 disable \n");
		
	}

	if ((len > 0) && copy_to_user (buf, (char *) page, len)) {
                len = -EFAULT;
        }
        return len;
}

static ssize_t repairfat_proc_write (struct file *file, const char *buf, size_t count, loff_t * pos)
{
        char c;
	printk("%s %c %c ",__FUNCTION__, buf[0],buf[1]);

	c = buf[0];
	switch(c) {
	case '0':
		unregister_repairfat_callback();
		break;
	case '1':
		register_repairfat_callback(wakeup_mmc_repairfat);
		break;
	default:
		printk("%s get wrong cmd:%c\n",__FUNCTION__, c);
	}

        return count;
}


/* This structure defines the file operations for the /proc/mmc/megasim device */
static struct file_operations repairfat_proc_fops =
{
    .read    = repairfat_proc_read,
    .write   = repairfat_proc_write,	
};


#define PROC_MMC_DIR_NAME             "mmc"
static struct proc_dir_entry *proc_mmc_dir, *proc_repairfat_file;


/*
 * Init proc file system for megasim feature.
 * And this interface is a bidirectional interface for the communication between kernel mmc driver and TAPI driver.
 * We support read and write operations:
 * -- read  : indicate whether theare is any data operations on MegaSIM card. (It is useless by now.) 
 * -- write : 
 *    MEGASIM_BOOTUP    : start the MegaSIM MMC part initialization and return 0.
 *    MEGASIM_RESET_REQ : Return 0 while there are no data operations on MegaSIM card. And unregister the MegaSIM card MMC driver. The subsequent data operations on MegaSIM card will be refused.
 */
static int mmc_proc_init(void)
{
        int rv = 0;

        /* create directory */
        proc_mmc_dir = proc_mkdir(PROC_MMC_DIR_NAME, NULL);
        if(proc_mmc_dir == NULL) {
                rv = -ENOMEM;
                goto out;
        }
#ifdef CONFIG_MOT_FEAT_MEGASIM
        /*
         * create megasim file using callback functions
         */
        proc_megasim_file = create_proc_entry(PROC_MMC_MEGASIM_NAME, 0644, proc_mmc_dir);
        if(proc_megasim_file == NULL) {
                rv = -ENOMEM;
                goto no_megasim;
        }
#endif
	proc_repairfat_file = create_proc_entry("repairfat", 0644, proc_mmc_dir);	
        if(proc_repairfat_file == NULL)	{
		rv = -ENOMEM;
		goto no_megasim;
	}

        /* Set the proc fops */
#ifdef CONFIG_MOT_FEAT_MEGASIM
        proc_megasim_file ->proc_fops = (struct file_operations *)&megasim_proc_fops;
#endif
	proc_repairfat_file->proc_fops = (struct file_operations *)&repairfat_proc_fops;

        return 0;
no_megasim:
        remove_proc_entry(PROC_MMC_DIR_NAME, NULL);
out:
        return rv;
}

static void mmc_proc_exit(void)
{
#ifdef CONFIG_MOT_FEAT_MEGASIM
        remove_proc_entry(PROC_MMC_MEGASIM_NAME, proc_mmc_dir);
#endif
        remove_proc_entry(PROC_MMC_DIR_NAME, NULL);
}





static struct class_device_attribute attr_class_sysfs[] = {
#ifdef CONFIG_MOT_FEAT_MEGASIM
	__ATTR(detect, S_IRUGO | S_IWUGO, mmc_megasim_show, mmc_megasim_store),
#endif
	__ATTR(repairfs, S_IRUGO | S_IWUGO, mmc_repairfs_show, mmc_repairfs_store),
	__ATTR_NULL
};

static struct class mmc_host_class = {
	.name		= "mmc_host",
	.class_dev_attrs = attr_class_sysfs,
	.release	= mmc_host_classdev_release,
};

/*
 * Internal function. Allocate a new MMC host.
 */
struct mmc_host *mmc_alloc_host_sysfs(int extra, struct device *dev)
{
	struct mmc_host *host;

	host = kmalloc(sizeof(struct mmc_host) + extra, GFP_KERNEL);
	if (host) {
		memset(host, 0, sizeof(struct mmc_host) + extra);

		host->dev = dev;
		host->class_dev.dev = host->dev;
		host->class_dev.class = &mmc_host_class;
		class_device_initialize(&host->class_dev);
	}

	return host;
}

/*
 * Internal function. Register a new MMC host with the MMC class.
 */
int mmc_add_host_sysfs(struct mmc_host *host)
{
	static unsigned int host_num;

	snprintf(host->host_name, sizeof(host->host_name),
		 "mmc%d", host_num++);

	strlcpy(host->class_dev.class_id, host->host_name, BUS_ID_SIZE);
	return class_device_add(&host->class_dev);
}

/*
 * Internal function. Unregister a MMC host with the MMC class.
 */
void mmc_remove_host_sysfs(struct mmc_host *host)
{
	class_device_del(&host->class_dev);
}

/*
 * Internal function. Free a MMC host.
 */
void mmc_free_host_sysfs(struct mmc_host *host)
{
	class_device_put(&host->class_dev);
}

static int __init mmc_init(void)
{
	int ret = bus_register(&mmc_bus_type);
	if (ret == 0) {
		ret = class_register(&mmc_host_class);
		if (ret)
			bus_unregister(&mmc_bus_type);
	}

	register_repairfat_callback(wakeup_mmc_repairfat);

        if (ret == 0) {
		ret = mmc_proc_init();
		if (ret) {
			class_unregister(&mmc_host_class);	
			bus_unregister(&mmc_bus_type);
		}
	}
	return ret;
}

static void __exit mmc_exit(void)
{
	unregister_repairfat_callback();
	class_unregister(&mmc_host_class);
	bus_unregister(&mmc_bus_type);

        mmc_proc_exit();
}

module_init(mmc_init);
module_exit(mmc_exit);
