/*
 * Copyright 2005-2006 Motorola, Inc.
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
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
 *
 * ChangeLog:
 * (mm-dd-yyyy) Author    Comment
 * 06-27-2005   Motorola  commented out MTD over SDRAM implementation.
 * 11-09-2006   Motorola  Changed ArgonLV-related definitions
 * 11-17-2006   Motorola  bute_init_maps no longer panics on probe failure
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <asm/hardware.h>
#include <asm/io.h>

/*!
 * @defgroup NOR_MTD BUTE NOR Flash MTD Driver
 */

/*!
 * @file buteref_flash.c
 *
 * @brief This file contains the MTD Mapping information on the BB.
 *
 * @ingroup NOR_MTD
 */

#ifndef CONFIG_MACH_ARGONLVPHONE
#error This is for ArgonLV architecture only
#endif

/*! 
 * The error code for MTD probe failure
 */
#define MTD_PROBE_ERR           -55

/*!
 * The PA of the NOR flash memory base on the BB board at CS0
 */
#define LV_FLASH_BASE_PA       0xA0000000
/*!
 * The size in bytes for the NOR flash memory on the BB board
 */
#define LV_FLASH_SIZE          0x02000000

/*!
 * BB NOR flash memory information
 */
static struct map_info bute_map = {
        .name      =    "nor0",
        .size      =    LV_FLASH_SIZE,
        .phys      =    LV_FLASH_BASE_PA,
        .bankwidth =    2,
};

/*!
 * This table contains the static partition information for the BB
 * NOR flash. It is only used by add_mtd_partitions() for static partition
 * registration to the upper MTD driver. See include/linux/mtd/partitions.h
 * for definition of the mtd_partition structure.
 */
static struct mtd_partition bute_part[] =
{
        {
                .name       =   "Bootloader",
                .size       =   1* 1024 * 1024,
                .offset     =   0x00000000,
                .mask_flags =   MTD_WRITEABLE  /* force read-only */
        },
        {
                .name       =   "Kernel",
                .size       =   3* 1024 * 1024,
                .offset     =   MTDPART_OFS_APPEND,
                .mask_flags =   MTD_WRITEABLE  /* force read-only */
        },
        {       .name       =   "rootfs",
                .size       =   18 * 1024 * 1024,
                .offset     =   MTDPART_OFS_APPEND,
                .mask_flags =   MTD_WRITEABLE  /* force read-only */
        },
        {
                .name       =   "userfs",
                .size       =   4 * 1024 *1024,
                .offset     =   MTDPART_OFS_APPEND,
        },
        {
                .name       =   "baseband",
                .size       =   4 * 1024 *1024,
                .offset     =   MTDPART_OFS_APPEND,
        },
};

/*!
 * This is used by parse_mtd_partitions() in order to obtain
 * the dynamic partition information for the NOR flash. If the dynamic
 * partition is valid, the static partition table won't be used. Instead,
 * parsed_parts is used by add_mtd_partitions() for registration to the
 * upper MTD driver.
 */
struct mtd_partition *parsed_parts = NULL;

/*!
 * This variable is used to specify the number of dynamic MTD partitions for
 * the NOR flash.
 */
static int nr_parsed_parts = 0;

/*!
 * This is the pointer to struct mtd_info returned by do_map_probe()
 * and used throughout the rest of the bute_mtd_init() function.
 */
static struct mtd_info *flash_mtd = NULL;

/*!
 * This array is used by parse_mtd_partitions() for dynamic partition parsing.
 * The proper partition parsing functions will be called in the order as they
 * appear in this array. For the BB port, the Redboot partition parsing
 * function is called first and then the command line parsing function. If a
 * valid partition table is found by any of these dynamic partition parsing
 * functions, the static partition table won't be used.
 */
const char *probes[] = {"RedBoot", "cmdlinepart", NULL};

/*!
 * This function is used to search for valid MTD partition information and 
 * pass it to the upper MTD driver.
 *
 * @return  0 if successful; non-zero otherwise
 */
static int __init bute_mtd_init(void)
{
        int ret = 0;

        if (!request_mem_region(LV_FLASH_BASE_PA, 
                                bute_map.size, "flash")) {
                printk(KERN_NOTICE "request_mem_region() failed - busy1)\n");
                return -EBUSY;

        }
        
        bute_map.virt = (unsigned long)ioremap(LV_FLASH_BASE_PA, 
                                                    bute_map.size);
        
        if (!bute_map.virt) {
                printk("Failed to ioremap %s\n", bute_map.name);
                goto out_err;
        }
        
        simple_map_init(&bute_map);

        printk(KERN_NOTICE "Probing %s at PA 0x%08lx (%d-bit)\n",
               bute_map.name, bute_map.phys, 
               bute_map.bankwidth * 8);

        /*
         * Now let's probe for the actual flash.  Do it here since
         * specific machine settings might have been set above.
         */
        flash_mtd = do_map_probe("cfi_probe", &bute_map);
        if (!flash_mtd) {
                printk("bb do_map_probe() failed\n");
                ret = MTD_PROBE_ERR;
                goto out_err;
        }
        flash_mtd->owner = THIS_MODULE;

        ret = parse_mtd_partitions(flash_mtd, probes, &parsed_parts, 0);
        if (ret > 0)
                nr_parsed_parts = ret;

        if (nr_parsed_parts) {
                printk(KERN_NOTICE "Using dynamic partition definition\n");
                ret = add_mtd_partitions(flash_mtd, parsed_parts, 
                                         nr_parsed_parts);
        } else {
                printk(KERN_NOTICE "Using static partition definition\n");
                ret = add_mtd_partitions(flash_mtd, bute_part, 
                                         ARRAY_SIZE(bute_part));
        }

        if (ret) {
                printk(KERN_ERR "mtd partition failed: %d\n", ret);
                BUG();
        }

        return 0;

out_err:
        if (bute_map.map_priv_2 != -1) {
                iounmap((void *)bute_map.map_priv_1);
                release_mem_region(bute_map.map_priv_2, bute_map.size);
        }
        printk("\n bute_mtd_init() error\n");
        return ret;
}


/*!
 * This is the module's entry function and is called when the kernel starts.
 *
 * @return  0 if successful; non-zero otherwise
 */
static int __init bute_init_maps(void)
{
        int ret = bute_mtd_init();
	if (ret == 0) {
                return 0;
	} else if (ret != MTD_PROBE_ERR) {
                BUG_ON(1);
        }
        return -1;
}
/*!
 * This function is the module's exit function. It is called when the module is 
 * unloaded. Since MTD is statically linked into the kernel, this function 
 * is never called.
 */
static void __exit bute_mtd_cleanup(void)
{
        if (flash_mtd) {
                del_mtd_partitions(flash_mtd);
                map_destroy(flash_mtd);
                iounmap((void *)bute_map.virt);
                if (parsed_parts)
                        kfree(parsed_parts);
        }
}

module_init(bute_init_maps);
module_exit(bute_mtd_cleanup);

MODULE_AUTHOR("Motorola");
MODULE_DESCRIPTION("BUTELV MTD map driver");
MODULE_LICENSE("GPL");
