/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 * (c) 2005 MontaVista Software, Inc.
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
 *
 * History:
 *     20051214: updated to platform device support
 *               borrowed many ideas from OMAP folks
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/ioport.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <asm/mach-types.h>
#include <asm/mach/flash.h>

#define DVR_VER "2.0"

#ifdef CONFIG_MTD_PARTITIONS
static const char *part_probes[] = { "RedBoot", "cmdlinepart", NULL };
#endif

struct mxcflash_info {
	struct mtd_partition *parts;
	struct mtd_info *mtd;
	struct map_info map;
};

/*!
 * @defgroup NOR_MTD NOR Flash MTD Driver
 */

/*!
 * @file mxc_nor.c
 *
 * @brief This file contains the MTD Mapping information on the MXC.
 *
 * @ingroup NOR_MTD
 */

static int __devinit mxcflash_probe(struct device *dev)
{
	int err, nr_parts = 0;
	struct mxcflash_info *info;
	struct platform_device *pdev = to_platform_device(dev);
	struct flash_platform_data *flash = pdev->dev.platform_data;
	struct resource *res = pdev->resource;
	unsigned long size = res->end - res->start + 1;

	info = kmalloc(sizeof(struct mxcflash_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	memset(info, 0, sizeof(struct mxcflash_info));

	if (!request_mem_region(res->start, size, "flash")) {
		err = -EBUSY;
		goto out_free_info;
	}
	info->map.virt = ioremap(res->start, size);
	if (!info->map.virt) {
		err = -ENOMEM;
		goto out_release_mem_region;
	}
	info->map.name = pdev->dev.bus_id;
	info->map.phys = res->start;
	info->map.size = size;
	info->map.bankwidth = flash->width;

	simple_map_init(&info->map);
	info->mtd = do_map_probe(flash->map_name, &info->map);
	if (!info->mtd) {
		err = -EIO;
		goto out_iounmap;
	}
	info->mtd->owner = THIS_MODULE;

#ifdef CONFIG_MTD_PARTITIONS
	nr_parts =
	    parse_mtd_partitions(info->mtd, part_probes, &info->parts, 0);
	if (nr_parts > 0) {
		add_mtd_partitions(info->mtd, info->parts, nr_parts);
	} else if (nr_parts < 0 && flash->parts) {
		add_mtd_partitions(info->mtd, flash->parts, flash->nr_parts);
	} else
#endif
	{
		printk(KERN_NOTICE "MXC flash: no partition info "
		       "available, registering whole flash\n");
		add_mtd_device(info->mtd);
	}

	dev_set_drvdata(&pdev->dev, info);
	return 0;

      out_iounmap:
	iounmap(info->map.virt);
      out_release_mem_region:
	release_mem_region(res->start, size);
      out_free_info:
	kfree(info);

	return err;
}

static int __devexit mxcflash_remove(struct device *dev)
{

	struct platform_device *pdev = to_platform_device(dev);
	struct mxcflash_info *info = dev_get_drvdata(&pdev->dev);
	struct flash_platform_data *flash = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, NULL);

	if (info) {
		if (info->parts) {
			del_mtd_partitions(info->mtd);
			kfree(info->parts);
		} else if (flash->parts)
			del_mtd_partitions(info->mtd);
		else
			del_mtd_device(info->mtd);

		map_destroy(info->mtd);
		release_mem_region(info->map.phys, info->map.size);
		iounmap((void __iomem *)info->map.virt);
		kfree(info);
	}
	return 0;
}

static struct device_driver mxcflash_driver = {
	.name = "mxc_nor_flash",
	.bus = &platform_bus_type,
	.probe = mxcflash_probe,
	.remove = __devexit_p(mxcflash_remove),
};

/*!
 * This is the module's entry function. It passes board specific
 * config details into the MTD physmap driver which then does the
 * real work for us. After this function runs, our job is done.
 *
 * @return  0 if successful; non-zero otherwise
 */
static int __init mxc_mtd_init(void)
{
	pr_info("MXC MTD nor Driver %s\n", DVR_VER);
	if (driver_register(&mxcflash_driver) != 0) {
		printk(KERN_ERR "Driver register failed for mxcflash_driver\n");
		return -ENODEV;
	}
	return 0;
}

/*!
 * This function is the module's exit function. It's empty because the
 * MTD physmap driver is doing the real work and our job was done after
 * mxc_mtd_init() runs.
 */
static void __exit mxc_mtd_exit(void)
{
	driver_unregister(&mxcflash_driver);
}

module_init(mxc_mtd_init);
module_exit(mxc_mtd_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MTD map and partitions for Freescale MXC boards");
MODULE_LICENSE("GPL");
