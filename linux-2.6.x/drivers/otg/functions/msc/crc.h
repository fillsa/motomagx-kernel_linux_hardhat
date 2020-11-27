/*
 * otg/msc_fd/crc.c - crc table
 * @(#) balden@seth2.belcarratech.com|otg/functions/msc/crc.h|20051116204958|22014
 *
 *      Copyright (c) 2003-2004 Belcarra
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


extern unsigned int *msc_crc32_table;

#define CRC32_INIT   0xffffffff      // Initial FCS value
#define CRC32_GOOD   0xdebb20e3      // Good final FCS value

#define CRC32_POLY   0xedb88320      // Polynomial for table generation
        
#define COMPUTE_FCS(val, c) (((val) >> 8) ^ msc_crc32_table[((val) ^ (c)) & 0xff])

int make_crc_table(void);
void free_crc_table(void);
unsigned int crc32_compute(unsigned char *src, int len, unsigned int val);


