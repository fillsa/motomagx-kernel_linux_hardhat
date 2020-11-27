/*
 * FILE NAME ktools/memmap.c
 *
 * BRIEF MODULE DESCRIPTION
 *
 * Author: David Singleton dsingleton@mvista.com MontaVista Software, Inc.
 *
 * 2001-2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/namespace.h>
#include <linux/mm.h>
#include <linux/kallsyms.h>
#include <linux/proc_fs.h>
#include <linux/mount.h>
#include <linux/swap.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#if defined(CONFIG_KMOD)
#include <linux/kmod.h>
#endif
extern int show_physical_memory(char *, char **, off_t, int, int *, void *);

#define VERSION 1.1
#define MODULE_VERS "1.1"
#define MODULE_NAME "memmap"
#define PREFIX "MEM ACC"
#define DPRINT(args...) if (mem_acc_debug) printk(PREFIX args)
#define BITS_PER_INT 	32
#define ONE_LINE	65

int lowwater = 0;

static int memmap_read_proc(char *page, char **start, off_t off,
    int count, int *eof, void *data)
{
	int ret = 0;

	ret = show_physical_memory(page, start, off, count, eof, data);
	return ret;
}

static struct proc_dir_entry *entry;

int __init
init_memmap(void)
{
	extern int lowwater;
	/*
	 * get the amount of free pages right here, as this
	 * should be the lowest amount of allocated pages
	 * the system ever sees.
	 */
	lowwater = num_physpages - nr_free_pages();

	/* create memmap using convenience function */
	if ((entry = create_proc_read_entry("memmap", 0444, NULL,
	    memmap_read_proc, NULL)) == NULL) {
		return -ENOMEM;
	}

	/*
	 * proc entries are created for us by fs/proc/base, even if we
	 * are a module
	 */
	
	/* everything OK */
	printk(KERN_INFO "%s %s initialised\n", MODULE_NAME, MODULE_VERS);
	return 0;
}


void cleanup_memmap(void)
{
	printk(KERN_INFO "%s %s removed\n",
	       MODULE_NAME, MODULE_VERS);
}


module_init(init_memmap);
module_exit(cleanup_memmap);

MODULE_AUTHOR("David Singleton <dsingleton@mvista.com>");
MODULE_DESCRIPTION("memory accounting");
MODULE_LICENSE("GPL");

