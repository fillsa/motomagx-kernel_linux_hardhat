/*
 * Copyright (C) 2006-2007 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Motorola 2007-Apr-23 - Remove Power Control 1 initialization.
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Nov-16 - Initial Creation
 */

#ifndef __ARGONREF71_BASE_H__
#define __ARGONREF71_BASE_H__

 /*!
 * @file argonref71_base.h
 *
 * @ingroup poweric_core
 *
 * @brief The register values used in core.c
 *
 * This file contains all the register values used for base argonref71.
 */

#include <linux/power_ic.h>

/******************************************************************************
* Global Variables
******************************************************************************/

static const unsigned int tab_init_reg_argonref71_base[NUM_INIT_REGS][NUM_INIT_TABLE_COLUMNS] =
{
    {POWER_IC_REG_ATLAS_INT_MASK_0,        0x0FFFFFF},
    {POWER_IC_REG_ATLAS_INT_MASK_1,        0x0FFFFFF},
    {POWER_IC_REG_ATLAS_SEMAPHORE,         0x0000000},
    {POWER_IC_REG_ATLAS_ARB_PERIPH_AUDIO,  0x0001000},
    {POWER_IC_REG_ATLAS_ARB_SWITCHERS,     0x0000000},

    {POWER_IC_REG_ATLAS_ARB_REG_0,         0x0000000},
    {POWER_IC_REG_ATLAS_ARB_REG_1,         0x0000000},
    {POWER_IC_REG_ATLAS_PWR_CONTROL_0,     0x0CA2E83},
    {POWER_IC_REG_ATLAS_PWR_CONTROL_2,     0x0000000},

    {POWER_IC_REG_ATLAS_REGEN_ASSIGN,      0x0000000},
    {POWER_IC_REG_ATLAS_SWITCHERS_0,       0x000470E},
    {POWER_IC_REG_ATLAS_SWITCHERS_1,       0x000470E},
    {POWER_IC_REG_ATLAS_SWITCHERS_2,       0x0024924},
    {POWER_IC_REG_ATLAS_SWITCHERS_3,       0x0024924},

    {POWER_IC_REG_ATLAS_SWITCHERS_4,       0x002764D},
    {POWER_IC_REG_ATLAS_SWITCHERS_5,       0x03E360D},
    {POWER_IC_REG_ATLAS_REG_SET_0,         0x005168C},
    {POWER_IC_REG_ATLAS_REG_SET_1,         0x0000F68},
    {POWER_IC_REG_ATLAS_REG_MODE_0,        0x0E4927B},
    
    {POWER_IC_REG_ATLAS_REG_MODE_1,        0x007F239},
    {POWER_IC_REG_ATLAS_PWR_MISC,          0x0000000},
    {POWER_IC_REG_ATLAS_AUDIO_RX_0,        0x0003000},
    {POWER_IC_REG_ATLAS_AUDIO_RX_1,        0x000D35A},
    {POWER_IC_REG_ATLAS_AUDIO_TX,          0x0420008},

    {POWER_IC_REG_ATLAS_SSI_NETWORK,       0x0013060},
    {POWER_IC_REG_ATLAS_AUDIO_CODEC,       0x0180027},
    {POWER_IC_REG_ATLAS_AUDIO_STEREO_DAC,  0x00E0004},
    {POWER_IC_REG_ATLAS_ADC_0,             0x0008020},
    {POWER_IC_REG_ATLAS_CHARGER_0,         0x0080000},

    {POWER_IC_REG_ATLAS_USB_0,             0x0000060},
    {POWER_IC_REG_ATLAS_CHARGE_USB_1,      0x000000E}
};

#endif /* __ARGONREF71_BASE_H__ */
