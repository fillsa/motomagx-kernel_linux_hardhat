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
 * @file mxc_rnga.h
 *
 * @brief MXC RNGA header file.
 *
 * @ingroup MXC_Security
 */

#ifndef _MXC_RNGA_H_
#define _MXC_RNGA_H_

#include <asm/hardware.h>
#include <asm/arch/mxc_security_api.h>
#include <linux/config.h>

#ifdef CONFIG_MXC_RNGA_TEST_DEBUG
#include <linux/module.h>
#endif				/* CONFIG_MXC_RNGA_TEST_DEBUG */

#ifdef CONFIG_MXC_RNGA_TEST_DEBUG
#define RNGA_DEBUG(fmt, args...) printk(fmt,## args)
#else
#define RNGA_DEBUG(fmt, args...)
#endif				/* CONFIG_MXC_RNGA_TEST_DEBUG */

/*
 * Configures Hi-assurance, Interrupt, Clears interrupt, goes to power down
 * mode and starts the random number generation
 */
#define   RNGA_CONTROL          IO_ADDRESS(RNGA_BASE_ADDR + 0x0000)
/* Gives the various status of the RNGA module */
#define   RNGA_STATUS           IO_ADDRESS(RNGA_BASE_ADDR + 0x0004)
/* Has the initial seed used for the generation of the random numbers */
#define   RNGA_ENTROPY          IO_ADDRESS(RNGA_BASE_ADDR + 0x0008)
/* Has the random number output */
#define   RNGA_FIFO             IO_ADDRESS(RNGA_BASE_ADDR + 0x000C)
/* Sets the mode of operation of the RNGA module */
#define   RNGA_MODE             IO_ADDRESS(RNGA_BASE_ADDR + 0x0010)
/* Used to configure for the verification mode */
#define   RNGA_VRFY_CTRL        IO_ADDRESS(RNGA_BASE_ADDR + 0x0014)
/* Used to control the oscillator counter */
#define   RNGA_OSC_CTRL_CNT     IO_ADDRESS(RNGA_BASE_ADDR + 0x0018)
/* Used to count the number of oscillator pulses */
#define   RNGA_OSC1_CNT         IO_ADDRESS(RNGA_BASE_ADDR + 0x001C)
/* Used to count the number of oscillator pulses */
#define   RNGA_OSC2_CNT         IO_ADDRESS(RNGA_BASE_ADDR + 0x0020)
/* Gives the status of the oscillators */
#define   RNGA_OSC_STATUS       IO_ADDRESS(RNGA_BASE_ADDR + 0x0024)

/*!
 * GO bit in RNGA Control register
 */
#define         RNGA_GO                 1

/*!
 * Start the random number generation
 */
#define         RNGA_START_RAND         1

/*!
 * Clear RNGA interrupt
 */
#define         RNGA_CLR_INTR           (1 << 3)

/*!
 * Set the RNGA to verification mode
 */
#define         RNGA_VERIFICATION       1

/*!
 * Set the RNGA shift clocks to off
 */
#define         RNGA_SHIFT_CLOCK_OFF    1

/*!
 * Force the shift clock to be driven by system clock
 */
#define         RNGA_FORCE_SYS_CLK      (1 << 1)

/*!
 * Reset the RNGA shift register
 */
#define         RNGA_RESET_SHIFT_REG    (1 << 2)

/*!
 * Maximum oscillator counter that the counter can accept.
 */
#define         RNGA_MAX_OSC_COUNT      0x3FFF

/*!
 * Read from an RNGA register
 */
#define         RNGA_READ               0

/*!
 * Read from an RNGA register
 */
#define         RNGA_WRITE              1

/*!
 * Generate random numbers
 */
#define         RNGA_GEN_RAND           1

/*!
 * This parameter used to Place RNGA in low power mode
 */
#define         RNGA_SLEEP              (1 << 4)

/*!
 * This parameter used to check for Osc.dead bit in RNGA status register.
 */
#define         RNGA_OSCDEAD            (1 << 31)

/*!
 * This parameter is used to get the offset for output FIFO Level.
 */
#define         RNGA_STATUS_FIFO_LVL_OFFSET       8

/*!
 * This parameter is used to mask Output FIFO Level.
 */
#define         RNGA_FIFO_LVL_MASK      0xff

/*!
 * This parameter is used to get offet for Maximum FIFO Size value.
 */
#define         RNGA_STATUS_FIFO_SIZE_OFFSET          16

/*!
 * This parameter is used to mask maximum FIFO Size value.
 *
 */ 
#define         RNGA_FIFO_SIZE_MASK      0xffff

/*!
 * This parameter is used to get oscillator #1 status.
 */
#define         RNGA_OSC1        1

/*!
 * This parameter is used to get oscillator #2 status.
 */
#define         RNGA_OSC2        2

#endif				/* _MXC_RNGA_H_ */
