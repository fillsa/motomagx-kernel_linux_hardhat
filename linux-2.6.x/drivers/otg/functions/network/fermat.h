/*
 * otg/functions/network/fermat.h - Network Function Driver
 * @(#) balden@seth2.belcarratech.com|otg/functions/network/fermat.h|20051116204958|58075
 *
 *      Copyright (c) 2003-2004 Belcarra
 *
 * By: 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 06/08/2005         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
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
/*!
 * @file otg/functions/network/fermat.h
 * @brief Fermat related data structures.
 *
 *
 * @ingroup NetworkFunction
 */

#ifndef FERMAT_DEFINED
#define FERMAT_DEFINED 1
typedef unsigned char BYTE;
typedef struct fermat {
        int length;
        BYTE power[256];
} FERMAT;

void fermat_init(void);
void fermat_encode(BYTE *data, int length);
void fermat_decode(BYTE *data, int length);
#endif


