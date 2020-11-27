/*
 * otg/msc_fd/crc.c - crc table
 * @(#) balden@seth2.belcarratech.com|otg/functions/msc/crc.c|20051116204958|21012
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Chris Lynne <cl@belcarra.com>
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA
 *
 */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>


#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <otg/otg-compat.h>

//#include <usbd-chap9.h>
//#include <usbd-mem.h>
//#include <usbd.h>
//#include <usbd-func.h>

#include "crc.h"

unsigned int *msc_crc32_table;

/**
 * Generate the crc32 table
 *
 * return non-zero if malloc fails
 */
int make_crc_table(void)
{
        unsigned int n;
        if (msc_crc32_table) return 0;
        if (!(msc_crc32_table = (unsigned int *)ckmalloc(256*4, GFP_KERNEL))) return -EINVAL;
        for (n = 0; n < 256; n++) {
                int k;
                unsigned int c = n;
                for (k = 0; k < 8; k++) {
                        c =  (c & 1) ? (CRC32_POLY ^ (c >> 1)) : (c >> 1);
                }
                msc_crc32_table[n] = c;
        }
        return 0;
}

void free_crc_table(void)
{
        if (msc_crc32_table) {
                lkfree(msc_crc32_table);
                msc_crc32_table = NULL;
        }
}

unsigned int crc32_compute(unsigned char *src, int len, unsigned int val)
{
        for (; len-- > 0; val = COMPUTE_FCS (val, *src++));
        return val;
}

