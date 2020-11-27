/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2005-2006  Motorola, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Motorola 2006-Oct-06 - Update File
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jan-18 - Update wait_event_timeout to be interruptible
 * Motorola 2005-Sep-01 - Implementation of wait event with timeout. 
 * Motorola 2005-Feb-28 - File re-written from scratch.
 */

#ifndef __OS_INDEPENDENT_H__
#define __OS_INDEPENDENT_H__

/*!
 * @file os_independent.h
 *
 * @brief Contains utilties for outputting information to the console
 *
 * @ingroup poweric_core
 */

#include <linux/errno.h>
#include <linux/time.h>
#include <linux/wait.h>

/*!@cond INTERNAL */
/*!
 * @name Trace Message Formatting & Options
 */
/*@{*/
/* Only compile trace message printing if build needs it. */
#ifdef CONFIG_MOT_POWER_IC_TRACEMSG
#define tracemsg(fmt,args...)  printk(fmt,##args)
#else
#define tracemsg(fmt,args...)
#endif

#define _a(a)           "Power IC: "a" \n"
#define _k_a(a) _a(a)
#define _k_d(a) KERN_DEBUG _a(a)
#define _k_i(a) KERN_INFO _a(a)
#define _k_w(a) KERN_WARNING _a(a)
/*@}*/

/*!
 * @name Wrappers for looking at timing.
 *
 * Gets turned on/off with tracemsgs to avoid
 * a bunch of conditional compiles everywhere..
 */
/*@{*/
#ifdef CONFIG_MOT_POWER_IC_TRACEMSG
#define dbg_gettimeofday(time)  do_gettimeofday((time))
#define dbg_timeval(time) struct timeval time
#else
#define dbg_gettimeofday(time)
#define dbg_timeval(time) 
#endif
/*@}*/
/*!@endcond */

#endif /* __OS_INDEPENDENT_H__ */
