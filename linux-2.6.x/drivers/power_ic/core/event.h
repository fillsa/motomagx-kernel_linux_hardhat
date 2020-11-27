/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2004-2008 Motorola, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Motorola 2008-Nov-17 - Add new sleep mask for lighting
 * Motorola 2007-Jun-21 - Adding new sleep mask
 * Motorola 2007-Jan-25 - Add support for power management
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Oct-06 - Update File
 * Motorola 2006_Apr-20 - Remove IRQ defines
 * Motorola 2005-Mar-15 - Fix Doxygen documentation
 * Motorola 2004-Dec-06 - Redesign of the Power IC event handler 
 */

#ifndef __EVENT_H__
#define __EVENT_H__

/*!
 * @file event.h
 *
 * @brief This file contains prototypes for power IC core-internal event handling functions.
 *
 * @ingroup poweric_core
 */ 

#include <linux/power_ic.h>

#define POWER_IC_EVENT_NUM_EVENTS POWER_IC_EVENT_NUM_EVENTS_ATLAS

/* Enum used for power management control */
typedef enum
{
    POWER_IC_PM_INTERRUPT_0,
    POWER_IC_PM_INTERRUPT_1,
    POWER_IC_PM_MISC_SLEEPS,
 
    POWER_IC_PM__NUM
} POWER_IC_PM_COMPONENT_T;

extern int power_ic_pm_suspend_mask_tbl[];

/* Atlas SPI interrupt 1 masks used for power management */
#define POWER_IC_ONOFF_MASK   0x00000008
#define POWER_IC_MB2_MASK     0x00020000
#define POWER_IC_HSDET_MASK   0x00040000

/* Misc sleep masks used for the power management */
#define POWER_IC_EMU_REV_MODE_SLEEP      0x00000001
#define POWER_IC_EMU_DMINUS_MONO_SLEEP   0x00000002
#define POWER_IC_EMU_DMINUS_STEREO_SLEEP 0x00000004
#define POWER_IC_LIGHTS_SLEEP            0x00000008

void power_ic_event_initialize (void);
#endif /* __EVENT_H__ */
