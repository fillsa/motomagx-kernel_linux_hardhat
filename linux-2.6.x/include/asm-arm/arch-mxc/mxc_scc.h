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
 * @file mxc_scc.h
 *
 * @brief This is intended to be the file which contains all of code or changes
 * needed to port the driver.
 *
 * @ingroup MXCSCC
 */

#ifndef MXC_SCC_H

#define MXC_SCC_H

#include <asm/hardware.h>

/*!
 * Expected to come from platform header files.
 * This symbol must be the address of the SCC
 */
#define SCC_BASE        SCC_BASE_ADDR

/*!
 *  This must be the interrupt line number of the SCM interrupt.
 */
#define INT_SCM         INT_SCC_SCM

/*!
 *  if #USE_SMN_INTERRUPT is defined, this must be the interrupt line number of
 *  the SMN interrupt.
 */
#define INT_SMN         INT_SCC_SMN

/*!
 * Make sure that this flag is defined if compiling for a VIRTIO model (of
 * MXC91331 1.0f1 or before, perhaps others).  This will avoid some difficulties
 * with some versions of the model.
 */
// #define VIRTIO

#endif
