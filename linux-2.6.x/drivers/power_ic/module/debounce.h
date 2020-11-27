/*
 * Copyright (C) 2005-2006,2008 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Motorola 2008-Jun-16 - Protect BIASEN, BIASSPEED & MC2BEN bits during 3mm5 headset debouncing
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2005-Feb-28 - Rewrote the software.
 * Motorola 2005-May-25 - Finalized the software and comment clean up
 */

#ifndef __DEBOUNCE_H__
#define __DEBOUNCE_H__

/*!
 * @file debounce.h
 *
 * @ingroup poweric_debounce
 *
 * @brief Header containing internal driver definitions for the debounce module.
 */
#include <asm/semaphore.h>

/*!
 * @brief Global used to indicate 3mm5 headset is debouncing
 */
extern unsigned char power_ic_debouncing_3mm5_headset;

/*!
 * @brief Mutex needed to protect power IC during 3mm5 headset debouncing
 */
extern struct compat_semaphore power_ic_debounce_audio_mutex;

extern void power_ic_debounce_init (void);

#endif /* __DEBOUNCE_H__ */
