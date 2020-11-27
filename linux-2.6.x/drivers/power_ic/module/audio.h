/*
 * Copyright (C) 2005-2008 Motorola, Inc.
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
 * Motorola 2008-May-15 - Add variable to keep track of AHS bits
 * Motorola 2007-Mar-14 - Convert AUDIO_CONFIG enum to bit masks
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-May-06 - Implement EMU Common Algorithm
 * Motorola 2005-Feb-28 - Rewrote the software.
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__

/*!
 * @file audio.h
 *
 * @ingroup poweric_audio
 *
 * @brief This is the header of internal definitions the power IC audio interface.
 *
 */

#include <linux/power_ic.h>
#include  <linux/moto_accy.h>

/*!
 * @brief Structure to keep trace of audio configuration when atlas is locked by EMU machine.
 */
typedef struct {
    MOTO_ACCY_HEADSET_MODE_T headset_mode;
    POWER_IC_EMU_CONN_MODE_T conn_mode;
    int id_pull_down;
} AUDIO_CONFIG_T;

/*!@cond INTERNAL */
/*! Defines the bit positions of the AHS bits in Atlas */
#define POWER_IC_AUDIO_RX0_AHS 0x000600
/*!@endcond */

/*!
 * @brief Global used to keep track of audio configuration.
 */
extern AUDIO_CONFIG_T audio_config;

/*!
 * @brief Global used to keep track of AHS bits.
 */
extern unsigned int power_ic_audio_set_reg_rx0_data;

int audio_ioctl(unsigned int cmd, unsigned long arg);

#endif /* __AUDIO_H__ */
