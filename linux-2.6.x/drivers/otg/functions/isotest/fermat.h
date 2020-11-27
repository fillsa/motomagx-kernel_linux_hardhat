/*
 * otg/network_fd/fermat.h - Network Function Driver
 * @(#) balden@seth2.belcarratech.com|otg/functions/isotest/fermat.h|20051116204957|16917
 *
 *      Copyright (c) 2003-2004 Belcarra
 *
 * By: 
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         dn Initial distribution
 * 10/18/2006         Motorola         ma sk pw Add Open Src Software language
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

#ifndef ISO_FERMAT_DEFINED
#define ISO_FERMAT_DEFINED 1
typedef unsigned char BYTE;
typedef struct fermat {
        int length;
        BYTE power[256];
} FERMAT;

void fermat_init(void);
void fermat_encode(BYTE *data, int length);
void fermat_decode(BYTE *data, int length);
#endif

