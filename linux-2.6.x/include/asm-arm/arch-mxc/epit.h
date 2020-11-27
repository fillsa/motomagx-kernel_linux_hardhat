/*
 *  epit.h - header file for accesses to the EPIT
 *
 *  Copyright 2006 Motorola, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Date     Author      Comment
 *  10/2006  Motorola    Initial version to support access to the EPIT  
 */
#ifndef __EPIT_H_
#define __EPIT_H_
#include <asm/arch/mxc.h>

#define _reg_EPIT_EPITCR       (volatile __u32 *)(MXC_EPIT_EPITCR)
#define _reg_EPIT_EPITSR       (volatile __u32 *)(MXC_EPIT_EPITSR)
#define _reg_EPIT_EPITCMPR     (volatile __u32 *)(MXC_EPIT_EPITCMPR)
#define _reg_EPIT_EPITCNR      (volatile __u32 *)(MXC_EPIT_EPITCNR)

#ifndef INT_EPIT 
#define INT_EPIT    INT_EPIT1
#endif

/*
 * EPIT Control register bit definitions
 */
#define EPITCR_CLKSRC_32K               0x03000000

#define EPITCR_OM_TOGGLE                0x00400000

#define EPITCR_STOPEN_ENABLE            0x00200000
#define EPITCR_WAITEN_ENABLE            0x00080000
#define EPITCR_DBGEN_ENABLE             0x00040000
#define EPITCR_DOZEN_ENABLE             0x00100000
#define EPITCR_SWR                      0x00010000

#define EPITCR_OCIEN                    0x00000004
#define EPITCR_ENMOD                    0x00000002
#define EPITCR_EN                       0x00000001

/* epit counter tick rate for 32K clock */
#define EPIT_COUNTER_TICK_RATE     32768

/* max epit counter value */
#define EPIT_MAX                   (0xffffffff)
#endif
