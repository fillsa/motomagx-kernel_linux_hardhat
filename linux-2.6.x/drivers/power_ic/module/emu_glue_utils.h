/*
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * Motorola 2007-Sep-26 - Updated the copyright.
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-06 - Implement USB detection light driver 
 * Motorola 2006-May-06 - Implement EMU Common Algorithm
 */

#ifndef __EMU_GLUE_UTILS_H__
#define __EMU_GLUE_UTILS_H__


/*!
 * @file emu_glue_utils.h
 *
 * @brief This file contains defines, macros and prototypes for the EMU
 * glue utils .
 *
 * @ingroup poweric_emu
 */ 

#include <stdbool.h>
#include "../core/os_independent.h"

#include <linux/power_ic.h>
#include <linux/moto_accy.h>


/* HW locked variable */
extern bool power_ic_emu_hw_locked;

/* containing the emu proc open state */
extern bool emu_proc_opened;

/*!
 * @brief VUSB input sources
 */
typedef enum
{
    EMU_VREG_IN_VINBUS,
    EMU_VREG_IN_VBUS,
    EMU_VREG_IN_BPLUS,
    EMU_VREG_IN_VBUS2
} EMU_VREG_IN_T;

/*!
 * @brief VUSB output voltages
 */
typedef enum
{
    EMU_VREG_OUT_2_775V,
    EMU_VREG_OUT_3_3V
} EMU_VREG_OUT_T;

/*!
 * @brief Disables bus signal
 */
#define EMU_BUS_SIGNAL_STATE_DISABLED false
/*!
 * @brief Enables bus signal
 */
#define EMU_BUS_SIGNAL_STATE_ENABLED true

/************************** UTILITY FUNTION *************************************/
void emu_util_set_emu_headset_mode(MOTO_ACCY_HEADSET_MODE_T mode);

/**************************** XCVR FUNCTION *************************************/
void emu_configure_usb_xcvr(EMU_XCVR_T state);

/**************************** Init function *************************************/
int __init emu_glue_utils_init(void);

/**************************** usb detection **************************************/
int usb_detection_int_handler(POWER_IC_EVENT_T event);
int usb_detection_state_machine_thread(void* unused);


#endif /* __EMU_GLUE_UTILS_H__ */
