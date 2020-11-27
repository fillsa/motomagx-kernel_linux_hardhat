/* 
 * efuse_drv.h - E-Fuse driver types and definitions used internally.
 *
 * Copyright 2006-2007 Motorola, Inc.
 *
 */

/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Initial revision.
 * 18-Jan-2007  Motorola        Fixed read order problem.
 * 09-May-2007  Motorola        Added SCS register definitions.
 */


#ifndef  _EFUSE_DRV_H_
#define  _EFUSE_DRV_H_

/* IIM Status and Control definitions */

#define MAX_EFUSE_OFFSET	(0x400)  
#define MAX_EFUSE_READ_LEN	(MAX_EFUSE_OFFSET/4)  

/* Currently defining up to bank 5 */
#define MAX_EFUSE_BANK		(5)     

#define EFUSE_BANK_0_OFFSET	(0x0800)
#define EFUSE_BANK_1_OFFSET	(0x0C00)
#define EFUSE_BANK_2_OFFSET	(0x1000)
#define EFUSE_BANK_3_OFFSET	(0x1400)
#define EFUSE_BANK_4_OFFSET	(0x1800)
#define EFUSE_BANK_5_OFFSET	(0x1C00)

 /* IIM Data structures */

/* The data structure used for the IIM status/command access */
typedef struct iim_regs
{
    uint32_t  stat;
    uint32_t  statm;
    uint32_t  err;
    uint32_t  emask;
    uint32_t  fctl;
    uint32_t  ua;
    uint32_t  la;
    uint32_t  sdat;
    uint32_t  prev;
    uint32_t  srev;
    uint32_t  ppr;
    uint32_t  scs0;
    uint32_t  scs1;
} iim_regs_t;

#endif /*_EFUSE_DRV_H_ */
