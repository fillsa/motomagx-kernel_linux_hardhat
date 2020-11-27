/*
 * kpanic.h
 *
 * Copyright 2006 Motorola, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Date     Author    Comment
 * 12/2006  Motorola  Initial version. Moved out from arch/arm/kernel/kpanic.c
 */

#ifndef __LINUX_KPANIC_H__
#define __LINUX_KPANIC_H__



#define MAX_KPANIC_PAGE_SIZE 2048       /* max flash page size */

/* extern functions below are defined inside drivers/mtd/kpanic_mtd.c */
extern int kpanic_initialize (void);
extern int get_kpanic_partition_size (void);
extern int get_kpanic_partition_page_size (void);
extern int kpanic_erase (void);
extern int kpanic_write_page (loff_t, const u_char*);

#ifdef CONFIG_MOT_FEAT_MEMDUMP
#define MEMDUMP_MAGIC "MEMDUMP"
extern int memdumppart_initialize (void);
extern int get_memdump_partition_size (void);
extern int get_memdump_partition_page_size (void);
extern int memdump_erase (void);
extern int memdump_write_page (loff_t, const u_char*);
extern void v6_flush_kern_cache_all_l2(void);
extern void kick_wd(void);
#endif /* CONFIG_MOT_FEAT_MEMDUMP */

/* externs below declared and initialized inside printk.c */
extern char *log_buf;                   /* points to beginning of printk buffer */
extern int log_buf_len;                 /* length of the printk buffer */
extern unsigned long log_end;           /* offset to write data into the printk buffer;
                                           must be masked before using it */
extern unsigned long logged_chars;      /* number of characters written into the printk
                                           buffer; max value is log_buf_len */

#endif /* __LINUX_KPANIC_H__ */
