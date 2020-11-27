/*
 *  linux/arch/arm/kernel/kpanic.c
 *
 *           Copyright Motorola 2006
 *
 * This file implements the mechanism to dump the printk buffer
 * to the dedicated kpanic flash partition.
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
 *  Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  --------------------
 * 10/06/2006   Motorola  Kernel panic changes
 * 11/29/2006   Motorola  Add support for memory dump
 *
 */

#include <linux/types.h>
#include <linux/string.h>
#include <linux/kpanic.h>

#ifdef CONFIG_MOT_FEAT_MEMDUMP
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sysctl.h>
#endif /* CONFIG_MOT_FEAT_MEMDUMP */

void dump_kpanic(char *str) {

	char *_log_buf = log_buf;
	int _log_buf_len = log_buf_len;
	unsigned long _log_end = log_end;
	unsigned long _logged_chars = logged_chars;
	char tmp_buf[MAX_KPANIC_PAGE_SIZE];
	loff_t flash_write_offset = 0;
	int quotient, remainder, i, j;
	int kpanic_page_size;
	int flag = 0;
	int orig_log_buf_len = _log_buf_len;
	int kpanic_partition_size;

	#define LOG_BUF_MASK (_log_buf_len-1)                    /* duplicated from printk.c */
	#define LOG_BUF(idx) (_log_buf[(idx) & LOG_BUF_MASK])    /* duplicated from printk.c */

	/* initialize the kpanic partition */
	if (kpanic_initialize() != 0) {
		return;
	}

	/* try to erase the kpanic partition and return if an error occurs */
	if (kpanic_erase() != 0)
		return;

	/* if str != NULL, try to write the passed in string to flash and return */
	if (str) {
		/* purposely ignoring the return value */
		kpanic_write_page(flash_write_offset, str);
		return;
	}

	/* get the size of the kpanic partition */
	kpanic_partition_size = get_kpanic_partition_size();

	/* truncate the printk buffer if it is larger than the kpanic partition */
	if (_log_buf_len > kpanic_partition_size) {
		if (_logged_chars > kpanic_partition_size) {
			_logged_chars = kpanic_partition_size;
			flag = 1;
		}
		_log_buf_len = kpanic_partition_size;
	}

	/* initialize the temporary buffer */
	memset(tmp_buf, 0, sizeof(tmp_buf));

	/* required for flash alignment */
	kpanic_page_size = get_kpanic_partition_page_size();

	/* the printk circular buffer has not yet wrapped around */
	if (_logged_chars < _log_buf_len) {
		
		quotient = _logged_chars / kpanic_page_size;
		remainder = _logged_chars % kpanic_page_size;

		for (i=0; i<quotient; i++) {
			if (kpanic_write_page(flash_write_offset, _log_buf))
				return;
			_log_buf += kpanic_page_size;
			flash_write_offset += kpanic_page_size;
		}

		if (remainder) {
                	for (i=0; i<remainder; i++)
                        	tmp_buf[i] = *_log_buf++;

			/* pad the buffer with 0xff, to signify un-used flash bytes */
                        while (i < kpanic_page_size) {
                        	tmp_buf[i] = 0xff;
                                i++;
                        }
			if (kpanic_write_page(flash_write_offset, tmp_buf))
				return;
                        /* no need to update flash_write_offset since we are done writing to the flash partition */
                }
	}
	/* once logged_chars equals log_buf_len, it is not incremented further */
	else {

		quotient = _log_buf_len / kpanic_page_size;

		/* 
		 * the number of messages inside the printk buffer (before truncating above) is equal to
		 * the size of the kpanic partition
		 */
		if (!flag) {
			for (i=0; i<quotient; i++) {
                        	for (j=0; j<kpanic_page_size; j++) {
                                	tmp_buf[j] = LOG_BUF(_log_end);
                                	_log_end++;
                        	}
				if (kpanic_write_page(flash_write_offset, tmp_buf))
					return;
                        	flash_write_offset += kpanic_page_size;
                	}
		}
		/* 
		 * since the number of messages inside the printk buffer is greater than the size of the kpanic
		 * partition, we will dump everything inside the printk buffer beginning with 
		 * (log_end - kpanic_partition_size) and going to (log_end - 1).
		 */
		else {

			/* will be writing pages to flash backwards */
			flash_write_offset = _log_buf_len - kpanic_page_size;
			_log_end--;

			/* needed b/c of macro expansions being used below */
                        _log_buf_len = orig_log_buf_len;

			for (i=0; i<quotient; i++) {
				for (j=kpanic_page_size-1; j>=0; j--) {
					tmp_buf[j] = LOG_BUF(_log_end);
					_log_end--;
				}
				if (kpanic_write_page(flash_write_offset, tmp_buf))
					return;
				flash_write_offset -= kpanic_page_size;
			}
		}
	}
}

#ifdef CONFIG_MOT_FEAT_MEMDUMP
void dump_rawmem(void) 
{
        unsigned long curpos = 0;
	unsigned long part_size, page_size, memdump_size;
        int pages_written = 0;


	memdump_size = num_physpages >> (20 - PAGE_SHIFT);

	/* initialize the kpanic partition */
	if (memdumppart_initialize() != 0) {
          printk(KERN_CRIT "Cannot initialize memory dump partition!\n");
		return;
	}

        printk(KERN_CRIT "Initialized memory dump partition\n");

	/* try to erase the kpanic partition and return if an error occurs */
	if (memdump_erase() != 0)
        {
          printk(KERN_CRIT "Cannot erase memory dump partition!\n");
		return;
        }

        printk(KERN_CRIT "Erased memory dump partition\n");

	/* flush the cache */
	v6_flush_kern_cache_all_l2();

	/* get the size of the kpanic partition */
	part_size = get_memdump_partition_size();
        printk(KERN_CRIT "memory dump partition size [%ld]\n", part_size);

	/* required for flash alignment */
	page_size = get_memdump_partition_page_size();
        printk(KERN_CRIT "memdump partition page size [%ld]\n", page_size);
	printk(KERN_CRIT "Dumping %lu MB of memory\n", memdump_size);

	memdump_size *= 1024 * 1024;

        while ((curpos < memdump_size) && (curpos < part_size))
        {
                volatile char *virt_curpos = (char *)ioremap(curpos + PHYS_OFFSET, PAGE_SIZE);
                int offset;

                if (! virt_curpos)
                {
                        printk(KERN_CRIT "Could not ioremap() [%lx]\n", curpos);
                        return;
                }

                for (offset = 0; offset < PAGE_SIZE; offset += page_size)
                {
		        kick_wd();


                        if (memdump_write_page((curpos + offset), (virt_curpos + offset)))
                        {
                               printk(KERN_CRIT "Cannot write to position %lx on memory dump partition!\n", (curpos + offset));
                               iounmap(virt_curpos);
                               return;
                        }

                        ++pages_written;
                }
                iounmap(virt_curpos);

                curpos += PAGE_SIZE;
        }

        printk(KERN_CRIT "Wrote %d pages\n", pages_written);

	/* Write the magic number at the last block.
	 * It writes whole mass_part_size bytes instead of sizeof(MEMDUMP_MAGIC).
	 * There is a very small risk of faulting, but we are in the process of panicing anyway...
	 */
	memdump_write_page(part_size - page_size, MEMDUMP_MAGIC);

}

int proc_cause_panic(ctl_table *a, int b, struct file * f,
	             void __user *p, size_t *s, loff_t *o)
{
	panic("test panic"); 

        return 0;
}

#endif /* CONFIG_MOT_FEAT_MEMDUMP */

