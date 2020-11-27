/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

 /*!
  * @file ../dam/registers.h
  * @brief This is the brief documentation for this registers.h file.
  *
  * This header file contains DAM driver low level definition to access module registers.
  *
  * @ingroup DAM
  */

#ifndef REGISTERS_H_
#define REGISTERS_H_

#include <asm/hardware.h>

#define ModifyRegister32(a,b,c)	(c = ( ( (c)&(~(a)) ) | (b) ))

#if defined(CONFIG_ARCH_MX3) || \
    defined(CONFIG_ARCH_MXC91331) || \
    defined(CONFIG_ARCH_MXC91321) || \
    defined(CONFIG_ARCH_MXC91231) || \
    defined(CONFIG_ARCH_MXC91131)

#ifndef _reg_DAM_PTCR1
#define    _reg_DAM_PTCR1   (*((volatile unsigned long *) \
                            (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x00))))
#endif

#ifndef _reg_DAM_PDCR1
#define    _reg_DAM_PDCR1  (*((volatile unsigned long *) \
                           (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x04))))
#endif

#ifndef _reg_DAM_PTCR2
#define    _reg_DAM_PTCR2   (*((volatile unsigned long *) \
                            (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x08))))
#endif

#ifndef _reg_DAM_PDCR2
#define    _reg_DAM_PDCR2  (*((volatile unsigned long *) \
                           (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x0C))))
#endif

#ifndef _reg_DAM_PTCR3
#define    _reg_DAM_PTCR3   (*((volatile unsigned long *) \
                            (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x10))))
#endif

#ifndef _reg_DAM_PDCR3
#define    _reg_DAM_PDCR3  (*((volatile unsigned long *) \
                           (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x14))))
#endif

#ifndef _reg_DAM_PTCR4
#define    _reg_DAM_PTCR4   (*((volatile unsigned long *) \
                            (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x18))))
#endif

#ifndef _reg_DAM_PDCR4
#define    _reg_DAM_PDCR4  (*((volatile unsigned long *) \
                           (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x1C))))
#endif

#ifndef _reg_DAM_PTCR5
#define    _reg_DAM_PTCR5   (*((volatile unsigned long *) \
                            (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x20))))
#endif

#ifndef _reg_DAM_PDCR5
#define    _reg_DAM_PDCR5  (*((volatile unsigned long *) \
                           (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x24))))
#endif

#ifndef _reg_DAM_PTCR6
#define    _reg_DAM_PTCR6   (*((volatile unsigned long *) \
                            (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x28))))
#endif

#ifndef _reg_DAM_PDCR6
#define    _reg_DAM_PDCR6  (*((volatile unsigned long *) \
                           (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x2C))))
#endif

#ifndef _reg_DAM_PTCR7
#define    _reg_DAM_PTCR7   (*((volatile unsigned long *) \
                            (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x30))))
#endif

#ifndef _reg_DAM_PDCR7
#define    _reg_DAM_PDCR7  (*((volatile unsigned long *) \
                           (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x34))))
#endif

#ifndef _reg_DAM_CNMCR
#define    _reg_DAM_CNMCR   (*((volatile unsigned long *) \
                            (IO_ADDRESS(AUDMUX_BASE_ADDR + 0x38))))
#endif

#ifndef _reg_DAM_PTCR
#define    _reg_DAM_PTCR(a)   (*((volatile unsigned long *) \
                              (IO_ADDRESS(AUDMUX_BASE_ADDR + a*8))))
#endif

#ifndef _reg_DAM_PDCR
#define    _reg_DAM_PDCR(a)   (*((volatile unsigned long *) \
                              (IO_ADDRESS(AUDMUX_BASE_ADDR + 4 + a*8))))
#endif

/*!
 * PTCR Registers bit shift definitions
 */
#define dam_synchronous_mode_shift               11
#define dam_receive_clock_select_shift           12
#define dam_receive_clock_direction_shift        16
#define dam_receive_frame_sync_select_shift      17
#define dam_receive_frame_sync_direction_shift   21
#define dam_transmit_clock_select_shift          22
#define dam_transmit_clock_direction_shift       26
#define dam_transmit_frame_sync_select_shift     27
#define dam_transmit_frame_sync_direction_shift  31
#define dam_selection_mask                      0xF

/*!
 * HPDCR Register bit shift definitions
 */
#define dam_internal_network_mode_shift           0
#define dam_mode_shift                            8
#define dam_transmit_receive_switch_shift        12
#define dam_receive_data_select_shift            13

/*!
 * HPDCR Register bit masq definitions
 */
#define dam_mode_masq                          0x03
#define dam_internal_network_mode_mask         0xFF

/*!
 * CNMCR Register bit shift definitions
 */
#define dam_ce_bus_port_cntlow_shift              0
#define dam_ce_bus_port_cnthigh_shift             8
#define dam_ce_bus_port_clkpol_shift             16
#define dam_ce_bus_port_fspol_shift              17
#define dam_ce_bus_port_enable_shift             18

#endif

#endif
