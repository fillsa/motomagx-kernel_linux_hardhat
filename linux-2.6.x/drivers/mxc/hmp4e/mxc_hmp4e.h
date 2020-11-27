/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/* 
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 * 
 * http://www.opensource.org/licenses/gpl-license.html 
 * http://www.gnu.org/copyleft/gpl.html
 */

/* 
 * Encoder device driver (kernel module headers)
 *
 * Copyright (C) 2005  Hantro Products Oy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _HMP4ENC_H_
#define _HMP4ENC_H_
#include <linux/ioctl.h>	/* needed for the _IOW etc stuff used later */

/*
 * Macros to help debugging
 */

#undef PDEBUG			/* undef it, just in case */
#ifdef HMP4E_DEBUG
#  ifdef __KERNEL__
    /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_INFO "hmp4e: " fmt, ## args)
#  else
    /* This one for user space */
#    define PDEBUG(fmt, args...) printf(__FILE__ ":%d: " fmt, __LINE__ , ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)	/* not debugging: nothing */
#endif

/*
 * Ioctl definitions
 */

/* Use 'k' as magic number */
#define HMP4E_IOC_MAGIC  'k'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */
#define HMP4E_IOCGBUFBUSADDRESS _IOR(HMP4E_IOC_MAGIC,  1, unsigned long *)
#define HMP4E_IOCGBUFSIZE       _IOR(HMP4E_IOC_MAGIC,  2, unsigned int *)
#define HMP4E_IOCGHWOFFSET      _IOR(HMP4E_IOC_MAGIC,  3, unsigned long *)
#define HMP4E_IOCGHWIOSIZE      _IOR(HMP4E_IOC_MAGIC,  4, unsigned int *)
#define HMP4E_IOC_CLI           _IO(HMP4E_IOC_MAGIC,  5)
#define HMP4E_IOC_STI           _IO(HMP4E_IOC_MAGIC,  6)

/* ... more to come */
#define HMP4E_IOCHARDRESET _IO(HMP4E_IOC_MAGIC, 15)	/* debugging tool */
#define HMP4E_IOC_MAXNR 15

#define mem_map_reserve SetPageReserved
#define mem_map_unreserve ClearPageReserved

//#define CCM_MCGR1_REG   (*((volatile unsigned long*)(IO_ADDRESS(CRM_MCU_BASE_ADDR+0x20))))
#define CCM_MCGR1_REG  IO_ADDRESS(CRM_MCU_BASE_ADDR+0x20)
#define MCGR1_MPEG4_CLK_EN 0x0000000C

#endif				/* !_HMP4ENC_H_ */
