/*
 * Copyright 2005 Motorola, Inc.
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
 * @defgroup NOR_MTD BB NOR Flash MTD Driver
 */

/*!
 * @file butebb_flash.c
 *
 * @brief This file contains the MTD Mapping information on the BB.
 *
 * @ingroup NOR_MTD
 */

#ifndef CONFIG_MACH_ARGONPLUSREF
#error This is for BUTE architecture only
#endif

/*! 
 * The error code for MTD probe failure
 */
#define MTD_PROBE_ERR           -55

/*!
 * The PA of the NOR flash memory base on the BB board at CS0
 */
#define BB_FLASH_BASE_PA       0xA0000000
/*!
 * The size in bytes for the NOR flash memory on the BB board
 */
#define BB_FLASH_SIZE          0x02000000

/*!
 * BB NOR flash memory information
 */
static struct map_info bb_map = {
        .name      =    "nor0",
        .size      =    BB_FLASH_SIZE,
        .phys      =    BB_FLASH_BASE_PA,
        .bankwidth =    2,
};

/*!
 * This table contains the static partition information for the BB
 * NOR flash. It is only used by add_mtd_partitions() for static partition
 * registration to the upper MTD driver. See include/linux/mtd/partitions.h
 * for definition of the mtd_partition structure.
 */
static struct mtd_partition bb_part[] =
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
 * and used throughout the rest of the bb_mtd_init() function.
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
static int __init bb_mtd_init(void)
{
        int ret = 0;

        if (!request_mem_region(BB_FLASH_BASE_PA, 
                                bb_map.size, "flash")) {
                printk(KERN_NOTICE "request_mem_region() failed - busy1)\n");
                return -EBUSY;

        }
        
        bb_map.virt = (unsigned long)ioremap(BB_FLASH_BASE_PA, 
                                                    bb_map.size);
        
        if (!bb_map.virt) {
                printk("Failed to ioremap %s\n", bb_map.name);
                goto out_err;
        }
        
        simple_map_init(&bb_map);

        printk(KERN_NOTICE "Probing %s at PA 0x%08lx (%d-bit)\n",
               bb_map.name, bb_map.phys, 
               bb_map.bankwidth * 8);

        /*
         * Now let's probe for the actual flash.  Do it here since
         * specific machine settings might have been set above.
         */
        flash_mtd = do_map_probe("cfi_probe", &bb_map);
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
                ret = add_mtd_partitions(flash_mtd, bb_part, 
                                         ARRAY_SIZE(bb_part));
        }

        if (ret) {
                printk(KERN_ERR "mtd partition failed: %d\n", ret);
                BUG();
        }

        return 0;

out_err:
        if (bb_map.map_priv_2 != -1) {
                iounmap((void *)bb_map.map_priv_1);
                release_mem_region(bb_map.map_priv_2, bb_map.size);
        }
        printk("\n bb_mtd_init() error\n");
        return ret;
}

#if 0
/****************** MTD over SDRAM implementation ********************/
/* 
 * Note: No doxygen comments on purpose. Mainly for initianl chip bringup.
 */
#define BB_SDRAM_MTD_BASE_PA   0x91600000      /* start from 22MB */
#define BB_SDRAM_MTD_SIZE      0x00800000      /* 8MB */

/* BB MTD over SDRAM information */
static struct map_info bb_map_sdram = {
        .name      =    "Freescale BB SDRAM",
        .size      =    BB_SDRAM_MTD_SIZE,
        .phys      =    BB_SDRAM_MTD_BASE_PA,
        .bankwidth =    2,
};

static struct mtd_partition bb_part_sdram[] =
{
        {       .name       =   "root (cramfs)",
                .size       =   8 * 1024 * 1024,
                .offset     =   0x00000000,
                .mask_flags =   MTD_WRITEABLE  /* force read-only */
        },
};

static struct mtd_partition *parsed_parts_sdram = NULL;
static struct mtd_info *sdram_mtd = NULL;
static int nr_parsed_parts_sdram = 0;

static int __init bb_mtd_init_sdram(void)
{
        int ret = 0;

        if (!request_mem_region(BB_SDRAM_MTD_BASE_PA, 
                                bb_map_sdram.size, "sdram")) {
                printk(KERN_NOTICE "request_mem_region() failed - busy2)\n");
                return -EBUSY;
        }
        
        bb_map_sdram.virt = (unsigned long)ioremap(BB_SDRAM_MTD_BASE_PA, 
                                                    bb_map_sdram.size);
        if (!bb_map_sdram.virt) {
                printk("Failed to ioremap SDRAM %s\n", bb_map_sdram.name);
                goto out_err;
        }
        
        simple_map_init(&bb_map_sdram);

        printk(KERN_NOTICE "Probing %s at PA 0x%08lx (%d-bit)\n",
               bb_map_sdram.name, bb_map_sdram.phys, 
               bb_map_sdram.bankwidth * 8);

        /*
         * Now let's probe for the actual flash.  Do it here since
         * specific machine settings might have been set above.
         */
        sdram_mtd = do_map_probe("map_ram", &bb_map_sdram);
        if (!sdram_mtd) {
                printk("bb do_map_probe() failed for SDRAM\n");
                ret = MTD_PROBE_ERR;
                goto out_err;
        }

        sdram_mtd->owner = THIS_MODULE;

        ret = parse_mtd_partitions(sdram_mtd, probes, 
                                   &parsed_parts_sdram, 0);
        if (ret > 0)
                nr_parsed_parts_sdram = ret;

        if (nr_parsed_parts_sdram) {
                printk(KERN_NOTICE "Using SDRAM dynamic partition\n");
                ret = add_mtd_partitions(sdram_mtd, parsed_parts_sdram, 
                                         nr_parsed_parts_sdram);
        } else {
                printk(KERN_NOTICE "Using SDRAM static partition\n");
                ret = add_mtd_partitions(sdram_mtd, bb_part_sdram, 
                                         ARRAY_SIZE(bb_part_sdram));
        }

        if (ret) {
                printk(KERN_ERR "mtd SDRAM partition failed: %d\n", ret);
                BUG();
        }

        return 0;

out_err:
        if (bb_map_sdram.map_priv_2 != -1) {
                iounmap((void *)bb_map_sdram.map_priv_1);
                release_mem_region(bb_map_sdram.map_priv_2, 
                                   bb_map_sdram.size);
        }
        printk("bb_mtd_init_sdram() error \n");

        return ret;
}
/****************** End of MTD over SDRAM implementation *******************/

#endif

/*!
 * This is the module's entry function and is called when the kernel starts.
 *
 * @return  0 if successful; non-zero otherwise
 */
static int __init bb_init_maps(void)
{
#if 0
	if (bb_mtd_init() == 0 || bb_mtd_init_sdram() == 0) {
#endif
	if (bb_mtd_init() == 0) {
                return 0;
	}else {
                BUG_ON(1);
        }
        return -1;

}
/*!
 * This function is the module's exit function. It is called when the module is 
 * unloaded. Since MTD is statically linked into the kernel, this function 
 * is never called.
 */
static void __exit bb_mtd_cleanup(void)
{
        if (flash_mtd) {
                del_mtd_partitions(flash_mtd);
                map_destroy(flash_mtd);
                iounmap((void *)bb_map.virt);
                if (parsed_parts)
                        kfree(parsed_parts);
        }
#if 0
        if (sdram_mtd) {
                del_mtd_partitions(sdram_mtd);
                map_destroy(sdram_mtd);
                iounmap((void *)bb_map_sdram.virt);
                if (parsed_parts_sdram)
                        kfree(parsed_parts_sdram);
        }
#endif
}

module_init(bb_init_maps);
module_exit(bb_mtd_cleanup);

MODULE_AUTHOR("Freescale");
MODULE_DESCRIPTION("BB MTD map driver");
MODULE_LICENSE("GPL");
