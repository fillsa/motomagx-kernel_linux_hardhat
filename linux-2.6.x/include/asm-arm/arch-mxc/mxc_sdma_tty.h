/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

 /*
  * The code contained herein is licensed under the GNU Lesser General
  * Public License.  You may obtain a copy of the GNU Lesser General
  * Public License Version 2.1 or later at the following locations:
  *
  * http://www.opensource.org/licenses/lgpl-license.html
  * http://www.gnu.org/copyleft/lgpl.html
  */

/*!
 * @file mxc_sdma_tty.h
 *
 * @brief This file contains the SDMA TTY public declarations.
 *
 * SDMA TTY driver is used for moving data between MCU and DSP using line discipline API.
 *
 * @ingroup SDMA
 */

#ifndef MXC_SDMA_TTY_H
#define MXC_SDMA_TTY_H

#include <linux/ioctl.h>

/*!
 * This defines Change Loopback Mode ioctl
 */
#define TIOCLPBACK _IOR('s', 0, unsigned char)

/*!
 * This defines default loopback mode
 */
#define DEFAULT_LOOPBACK_MODE 0

/*!
 * This defines a number of bi-directional driver instances
 */
#define IPC_NB_CH_BIDIR  2

/*!
 * This defines a number of mono directional channels from DSP to MCU  driver instances
 */
#define IPC_NB_CH_DSPMCU 4

#endif
