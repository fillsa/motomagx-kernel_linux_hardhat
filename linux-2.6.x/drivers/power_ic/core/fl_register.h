/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2004, 2006-2007 Motorola, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Motorola 2007-Feb-19 - Use HWCFG to determine which chip is present.
 * Motorola 2007-Jan-16 - Add support for MAX6946 LED controller chip.
 * Motorola 2006-Sep-26 - Add support for MAX7314 LED controller chip.
 * Motorola 2004-Dec-06 - Design the Low Level interface to Funlight IC.
 */

#ifndef __FL_REGISTER_H__
#define __FL_REGISTER_H__

/*!
 * @file fl_register.h
 * @brief This file contains prototypes of Funlight driver available in kernel space.
 *
 * @ingroup poweric_core
 */

#include <stdbool.h>

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*!
 * @name Definition of Maxim MAX6946 Register Addresses
 */
/*@{*/
#define MAX6946_PORT_P0_OUTPUT_LEVEL_REG            0x00
#define MAX6946_PORT_P1_OUTPUT_LEVEL_REG            0x01
#define MAX6946_PORT_P2_OUTPUT_LEVEL_REG            0x02
#define MAX6946_PORT_P3_OUTPUT_LEVEL_REG            0x03
#define MAX6946_PORT_P4_OUTPUT_LEVEL_REG            0x04
#define MAX6946_PORT_P5_OUTPUT_LEVEL_REG            0x05
#define MAX6946_PORT_P6_OUTPUT_LEVEL_REG            0x06
#define MAX6946_PORT_P7_OUTPUT_LEVEL_REG            0x07
#define MAX6946_PORT_P8_OUTPUT_LEVEL_REG            0x08
#define MAX6946_PORT_P9_OUTPUT_LEVEL_REG            0x09
#define MAX6946_PORTS_P0_P9_OUTPUT_LEVEL_REG        0x0A
#define MAX6946_PORTS_P0_P3_OUTPUT_LEVEL_REG        0x0B
#define MAX6946_PORTS_P4_P7_OUTPUT_LEVEL_REG        0x0C
#define MAX6946_PORTS_P8_P9_OUTPUT_LEVEL_REG        0x0D
#define MAX6946_CONFIG_REG                          0x10
#define MAX6946_OUTPUT_CURRENT_ISET70_REG           0x13
#define MAX6946_OUTPUT_CURRENT_ISET98_REG           0x14
#define MAX6946_GLOBAL_CURRENT_REG                  0x15
/*@}*/

/*!
 * @name Definition of Maxim MAX7314 Register Addresses
 */
/*@{*/
#define MAX7314_BLINK_PHASE_0_OUTPUT_P7_P0_REG      0x02
#define MAX7314_BLINK_PHASE_0_OUTPUT_P15_P8_REG     0x03
#define MAX7314_PORTS_CONFIG_P7_P0_REG              0x06
#define MAX7314_PORTS_CONFIG_P15_P8_REG             0x07
#define MAX7314_BLINK_PHASE_1_OUTPUT_P7_P0_REG      0x0A
#define MAX7314_BLINK_PHASE_1_OUTPUT_P15_P8_REG     0x0B
#define MAX7314_MASTER_O16_INTESITY_REG             0x0E
#define MAX7314_CONFIG_REG                          0x0F
#define MAX7314_OUTPUT_INTENSITY_P1_P0_REG          0x10
#define MAX7314_OUTPUT_INTENSITY_P3_P2_REG          0x11
#define MAX7314_OUTPUT_INTENSITY_P5_P4_REG          0x12
#define MAX7314_OUTPUT_INTENSITY_P7_P6_REG          0x13
#define MAX7314_OUTPUT_INTENSITY_P9_P8_REG          0x14
#define MAX7314_OUTPUT_INTENSITY_P11_P10_REG        0x15
#define MAX7314_OUTPUT_INTENSITY_P13_P12_REG        0x16
#define MAX7314_OUTPUT_INTENSITY_P15_P14_REG        0x17
/*@}*/

/*!
 * @name The funlight chips supported by this driver.
 */
/*@{*/
#define FL_CHIP_MAX6946                       0x00080000
#define FL_CHIP_MAX7314                       0x00080001
/*@}*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

int fl_initialize(void);
bool fl_is_chip_present(int chip);
int fl_reg_read (int reg, unsigned int *reg_value);
int fl_reg_write (int reg, unsigned int reg_value);

#endif  /* __FL_REGISTER_H__ */
