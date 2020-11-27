/*
 *  linux/arch/arm/mm/cache-l210.c
 *
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *  Copyright (C) 2005-2006 Freescale Semiconductor, Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/cacheflush.h>
#include <asm/hardware.h>

/* L210 related defines */
#define L2_LINE_SIZE		32
#define L2_ENABLE_BIT		0x1
#define L2_ID_REG		(L2CC_BASE_ADDR_VIRT + 0x0)
#define L2_TYPE_REG		(L2CC_BASE_ADDR_VIRT + 0x4)
#define L2_CTL_REG		(L2CC_BASE_ADDR_VIRT + 0x100)
#define L2_AUX_REG		(L2CC_BASE_ADDR_VIRT + 0x104)
#define L2_SYNC_REG		(L2CC_BASE_ADDR_VIRT + 0x730)
#define L2_INV_LINE_REG		(L2CC_BASE_ADDR_VIRT + 0x770)
#define L2_INV_WAY_REG		(L2CC_BASE_ADDR_VIRT + 0x77C)
#define L2_CLEAN_LINE_REG	(L2CC_BASE_ADDR_VIRT + 0x7B0)
#define L2_CLEAN_WAY_REG	(L2CC_BASE_ADDR_VIRT + 0x7BC)
#define L2_CLEAN_INV_LINE_REG	(L2CC_BASE_ADDR_VIRT + 0x7F0)
#define L2_CLEAN_INV_WAY_REG	(L2CC_BASE_ADDR_VIRT + 0x7FC)
#define L2_DEBUG_CTL_REG	(L2CC_BASE_ADDR_VIRT + 0xF40)

#define IS_L2_ENABLED()		((__raw_readl(L2_CTL_REG) & L2_ENABLE_BIT) != 0)
#define L2_ENABLE()		__raw_writel(L2_ENABLE_BIT, L2_CTL_REG)
#define L2_DISABLE()		__raw_writel(0, L2_CTL_REG)
#define L2_CLEAN_LINE(x)	__raw_writel(x, L2_CLEAN_LINE_REG)
#define L2_CLEAN_INV_LINE(x)	__raw_writel(x, L2_CLEAN_INV_LINE_REG)
#define L2_INV_LINE(x)		__raw_writel(x, L2_INV_LINE_REG)
#define L2_CLEAN_INV_WAY(x)	__raw_writel(x, L2_CLEAN_INV_WAY_REG)
#define IS_L2_ALL_CLEANED()	(__raw_readl(L2_CLEAN_INV_WAY_REG) == 0)

/*!
 * Invalidate the L2 data cache within the specified region; we will
 * be performing a DMA operation in this region and we want to
 * purge old data in the cache.
 *
 * @param start   physical start address of region
 * @param end     physical end address of region
 */
void dmac_l2_inv_range(dma_addr_t start, dma_addr_t end)
{
	if (!IS_L2_ENABLED())
		return;

	if ((start & (L2_LINE_SIZE - 1)) != 0) {
		start &= ~(L2_LINE_SIZE - 1);
		L2_CLEAN_LINE(start);
	}
	if ((end & (L2_LINE_SIZE - 1)) != 0) {
		end &= ~(L2_LINE_SIZE - 1);
		L2_CLEAN_INV_LINE(end);
	}
	while (start < end) {
		L2_INV_LINE(start);
		start += L2_LINE_SIZE;
	}
}

/*!
 * Clean the L2 data cache within the specified region
 *
 * @param start   physical start address of region
 * @param end     physical end address of region
 */
void dmac_l2_clean_range(dma_addr_t start, dma_addr_t end)
{
	if (!IS_L2_ENABLED())
		return;

	if ((start & (L2_LINE_SIZE - 1)) != 0) {
		start &= ~(L2_LINE_SIZE - 1);
	}
	while (start < end) {
		L2_CLEAN_LINE(start);
		start += L2_LINE_SIZE;
	}
}

/*!
 * Flush (clean/invalidate) the L2 data cache within the specified region
 *
 * @param start   physical start address of region
 * @param end     physical end address of region
 */
void dmac_l2_flush_range(dma_addr_t start, dma_addr_t end)
{
	if (!IS_L2_ENABLED())
		return;

	if ((start & (L2_LINE_SIZE - 1)) != 0) {
		start &= ~(L2_LINE_SIZE - 1);
	}
	while (start < end) {
		L2_CLEAN_INV_LINE(start);
		start += L2_LINE_SIZE;
	}
}

/*!
 * Flush (clean and invalidate) the whole L2 cache
 */
void l2_flush_all(void)
{
	if (!IS_L2_ENABLED())
		return;

	L2_CLEAN_INV_WAY(0xFF);
	while (!IS_L2_ALL_CLEANED()) {
	}
}

/*!
 * Disable the L2 cache
 */
void l2_disable(void)
{
	L2_DISABLE();
}

/*!
 * Enable the L2 cache
 */
void l2_enable(void)
{
	L2_ENABLE();
}

/*
 * Here are the L2 Cache options from the command line. By default the
 * L2 is on with L2 in write through mode.
 *
 *       "off" -- off
 *       "wt" --  write through (default)
 *       "wb" -- write back
 */

#define L210_MODE_DISABLED	0
#define L210_MODE_WRITE_THROUGH	1
#define L210_MODE_WRITE_BACK	2

static int l210_mode_updated __initdata;
static int l210_mode __initdata = L210_MODE_WRITE_THROUGH;
static char *l210_mode_info[] __initdata = {
		"OFF",
		"WT",
		"WB",
	};

static void __init l2_cmd_policy(char **p)
{
	if (memcmp(*p, "off", 3) == 0) {
		l210_mode = L210_MODE_DISABLED;
		*p += 3;
	} else if (memcmp(*p, "wt", 2) == 0) {
		l210_mode = L210_MODE_WRITE_THROUGH;
		*p += 2;
	} if (memcmp(*p, "wb", 2) == 0) {
		l210_mode = L210_MODE_WRITE_BACK;
		*p += 2;
	}

	l210_mode_updated = 1;
}

__early_param("L2cache=", l2_cmd_policy);

/*!
 * Initialize and enable L2 cache based on boot command line
 */
static int __init l2_init(void)
{
	if (!l210_mode_updated) {
		/* L2 cache policy is not enforced via command line */
		if ((__raw_readl(L2_ID_REG) & 0x3F) != 0x2) {
			l210_mode = L210_MODE_WRITE_BACK;
		}
	}

	printk("L2 cache: %s\n", l210_mode_info[l210_mode]);

	switch (l210_mode) {
	case L210_MODE_WRITE_THROUGH:
		/* Do nothing for default config */
		break;
	case L210_MODE_WRITE_BACK:
		__raw_writel(0, L2_DEBUG_CTL_REG);
		break;
	case L210_MODE_DISABLED:
		l2_flush_all();
		l2_disable();
		break;
	}
	return 0;
}

arch_initcall(l2_init);

extern void v6_flush_kern_cache_all(void);

/*!
 * Flush both L1 and L2 caches
 */
void v6_flush_kern_cache_all_l2(void)
{
	v6_flush_kern_cache_all();
	l2_flush_all();
}

EXPORT_SYMBOL(dmac_l2_inv_range);
EXPORT_SYMBOL(dmac_l2_clean_range);
EXPORT_SYMBOL(dmac_l2_flush_range);
EXPORT_SYMBOL(l2_flush_all);
EXPORT_SYMBOL(v6_flush_kern_cache_all_l2);

#ifdef CONFIG_PROC_FS
static int l2cache_read_proc(char *page, char **start, off_t off,
			     int count, int *eof, void *data)
{
	char *p = page;
	int len;
	u32 size, way, waysize, linelen;
	u32 l2_type = __raw_readl(L2_TYPE_REG);
	u32 l2_aux = __raw_readl(L2_AUX_REG);

	waysize = (l2_aux >> 17) & 0x7;
	if (waysize == 0 || waysize > 5) {
		waysize = 0;
	} else {
		waysize = (1 << (waysize - 1)) * 16384;
	}

	way = (l2_aux >> 13) & 0xF;
	if (way > 8)
		way = 0;

	linelen = 32;
	size = way * waysize;

	p += sprintf(p, "L2:\t\t\t%s\n", (IS_L2_ENABLED()? "ON" : "OFF"));
	p += sprintf(p, "L2 Cache type:\t\t%s\n",
		     (__raw_readl(L2_DEBUG_CTL_REG) == 0 ?
		      "write-back" : "write-through"));
	p += sprintf(p, "L2 Cache lockdown:\t%s\n",
		     (((l2_type >> 25) & 0xF) == 0xE ? "format C" : "unknown"));
	p += sprintf(p, "L2 Cache format:\t%s\n", "unified");
	p += sprintf(p, "L2 Cache size:\t\t%d\n", size);
	p += sprintf(p, "L2 Cache ways:\t\t%d\n", way);
	p += sprintf(p, "L2 Cache way size:\t%d\n", waysize);
	p += sprintf(p, "L2 Cache line length:\t%d\n", linelen);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

static int __init setup_l2cache_proc_entry(void)
{
	struct proc_dir_entry *res;

	res = create_proc_read_entry("cpu/l2cache", 0, NULL,
				     l2cache_read_proc, NULL);
	if (!res) {
		printk(KERN_ERR "Failed to create proc/cpu/l2cache\n");
		return -ENOMEM;
	}
	return 0;
}

late_initcall(setup_l2cache_proc_entry);
#endif
