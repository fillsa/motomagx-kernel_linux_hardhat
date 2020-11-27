/*
 *  drivers/mtd/kpanic_mtd.c
 *
 *	Copyright 2005 Motorola, Inc.
 *
 * This file implements the mechanism to erase and write to the kpanic 
 * flash partition. When CONFIG_MOT_FEAT_MEMDUMP_ON_PANIC is enabled,
 * it initializes the mass storage partition for mem dump.
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
 * 09-26-2005   Motorola  implemented CONFIG_MOT_FEAT_KPANIC feature.
 *              	  Whenever the kernel panics, the printk ring buffer 
 *              	  will be written out to a dedicated flash partition.
 *
 * 11-29-2006   Motorola  implemented CONFIG_MOT_FEAT_MEMDUMP feature.
 *                        Dump the memory content to the mass storage
 *                        partition on panic.
 */

#include <linux/mtd/mtd.h>

extern struct mtd_info *kpanic_partition;

#ifdef CONFIG_MOT_FEAT_MEMDUMP
extern struct mtd_info *memdump_partition;
#endif /* CONFIG_MOT_FEAT_MEMDUMP */

int kpanic_initialize (void) {
	
	if (!kpanic_partition) {
                printk(KERN_EMERG "Couldn't find the kpanic flash partition.\n");
                return 1;
        }
	return 0;
}

int get_kpanic_partition_size (void) {
	return kpanic_partition->size;
}

int get_kpanic_partition_page_size (void) {
	return kpanic_partition->oobblock;
}

int kpanic_erase (void) {

	struct erase_info erase;

	/* set up the erase structure */
	erase.mtd = kpanic_partition;
	erase.addr = 0;
	erase.len = kpanic_partition->size;
	erase.callback = NULL;

	/* erase the kpanic flash block partition */
	if (MTD_ERASE(kpanic_partition, &erase)) {
		printk(KERN_EMERG "Couldn't erase the kpanic flash partition.\n");
		return 1;
	}

	return 0;
}

int kpanic_write_page (loff_t to, const u_char *buf) {

	size_t retlen;

	if (MTD_WRITE(kpanic_partition, to, kpanic_partition->oobblock, &retlen, buf)) {
		printk(KERN_EMERG "Couldn't write to the kpanic flash partition.\n");
		return 1;
	}

	return 0;
}

#ifdef CONFIG_MOT_FEAT_MEMDUMP
int memdumppart_initialize (void) {
        if (!memdump_partition)
        {
                printk(KERN_EMERG "Couldn't find the memory dump flash partition.\n");
                return 1;
        }

        return 0;
}

int get_memdump_partition_size (void) {
        return memdump_partition->size;
}

int get_memdump_partition_page_size (void) {
        return memdump_partition->oobblock;
}

int memdump_erase (void) {

        struct erase_info erase;

        /* set up the erase structure */
        erase.mtd = memdump_partition;
        erase.addr = 0;
        erase.len = memdump_partition->size;
        erase.callback = NULL;

        /* erase the mass flash block partition */
        if (MTD_ERASE(memdump_partition, &erase)) {
                printk(KERN_EMERG "Couldn't erase the memory dump flash partition.\n");
                return 1;
        }

        return 0;
}

int memdump_write_page (loff_t to, const u_char *buf) {

        size_t retlen;

        if (MTD_WRITE(memdump_partition, to, memdump_partition->oobblock, &retlen, buf)) {
                printk(KERN_EMERG "Couldn't write to the memory dump flash partition.\n");
                return 1;
        }

        return 0;
}

#endif /* CONFIG_MOT_FEAT_MEMDUMP */

