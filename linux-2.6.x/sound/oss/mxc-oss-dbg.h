/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 *
 * Copyright (C) 2005 Freescale Semiconductor, Inc. All rights reserved.
 *
 * History :
 *        04/06/29 : Creation
 *        05/03/03: TLSbo48139: Driver bring up on HW
 */

/*!
 * @file mxc-oss-dbg.h
 * @brief Definitions for help with debugging the MXC OSS sound driver.
 *
 * @ingroup SOUND_OSS_MXC_PMIC
 */

#ifndef _MXC_OSS_DBG_H
#define _MXC_OSS_DBG_H

/* Compilation flags - All disabled in normal mode. */

/*! All traces activated. */
/* #define DEBUG_ALL      */

/*! Enable most important traces. */
/* #define DEBUG                  */

/*! Adds some checks.          */
/* #define MORE_DEBUG_CHECKING */

/*! Enables workaround for hiZ rise-up latency. */
#define MXC_HIZ_WORKAROUND

/*
 * The flags below can be activated to add traces to dmesg
 * Allow only one at a time
 */
#ifdef DEBUG

#define DPRINTK(fmt, args...) 	printk("%s: " fmt, __FUNCTION__, ## args)
#define FUNC_START		do {} while(0)	/* No output. */
#define FUNC_END		do {} while(0)	/* No output. */
#define FUNC_ERR		DPRINTK("Operation Failed ! (%s)\n", \
                                      __FUNCTION__)
#else				/* DEBUG */

#ifdef DEBUG_ALL

#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__, ## args)
#define FUNC_START            DPRINTK(KERN_ERR"start\n")
#define FUNC_END              DPRINTK(KERN_ERR"end\n")
#define FUNC_ERR              DPRINTK("Operation Failed ! (%s)\n", \
                                      __FUNCTION__)

#else				/* DEBUG_ALL */

#define FUNC_START		do {} while(0)	/* No output. */
#define FUNC_END		do {} while(0)	/* No output. */
#define FUNC_ERR		do {} while(0)	/* No output. */
#define DPRINTK(fmt, args...)	do {} while(0)	/* No output. */

#endif				/* DEBUG_ALL */
#endif				/* DEBUG     */

#ifdef MORE_DEBUG_CHECKING
#define DBG_INFO(fmt, args...) printk("%s: " fmt, __FUNCTION__, ## args)
#define CHECK_IF_0(x, str) \
        do { \
                if ( (x)!=0 ) DBG_INFO("Error: %s=%i, should be 0\n", \
                                       str, x); \
        } while (0)
#else
#define DBG_INFO(fmt, args...)	do {} while(0)	/* No output. */
#define CHECK_IF_0(x, str)	do {} while(0)	/* No check.  */
#endif

#endif				/* _MXC_OSS_DBG_H */
