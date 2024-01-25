/*
 * linux/fs/fat/fatfs_syms.c
 *
 * Exported kernel symbols for the low-level FAT-based fs support.
 *
 */
/*
 *  Copyright (C) 2008 Motorola, Inc. 
 *
 * ChangeLog:
 * (mm-dd-yyyy)	Author		Comment
 * 08-12-2008   Motorola        Add misc init function
 */


#include <linux/module.h>
#include <linux/init.h>

#include <linux/mm.h>
#include <linux/msdos_fs.h>

EXPORT_SYMBOL(fat_new_dir);
EXPORT_SYMBOL(fat_date_unix2dos);
EXPORT_SYMBOL(fat__get_entry);
EXPORT_SYMBOL(fat_notify_change);
EXPORT_SYMBOL(fat_attach);
EXPORT_SYMBOL(fat_detach);
EXPORT_SYMBOL(fat_build_inode);
EXPORT_SYMBOL(fat_fill_super);
EXPORT_SYMBOL(fat_search_long);
EXPORT_SYMBOL(fat_scan);
EXPORT_SYMBOL(fat_add_entries);
EXPORT_SYMBOL(fat_dir_empty);

int __init fat_cache_init(void);
void __exit fat_cache_destroy(void);
int __init fat_init_inodecache(void);
void __exit fat_destroy_inodecache(void);
#if ! defined(MAGX_N_06_19_60I)
int __init fat_misc_init(void);
#endif
void __exit fat_misc_destroy(void);

static int __init init_fat_fs(void)
{
	int ret;

	ret = fat_cache_init();
	if (ret < 0)
		return ret;
	
#if ! defined(MAGX_N_06_19_60I)
	ret = fat_init_inodecache();
	if (ret < 0)
		return ret;

	return fat_misc_init();
#else
	return fat_init_inodecache();
#endif
}

static void __exit exit_fat_fs(void)
{
	fat_cache_destroy();
	fat_destroy_inodecache();
	fat_misc_destroy();
}

module_init(init_fat_fs)
module_exit(exit_fat_fs)
