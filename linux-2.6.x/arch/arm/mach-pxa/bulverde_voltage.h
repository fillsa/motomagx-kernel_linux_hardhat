#ifndef BULVERDE_VOLTAGE_H
#define BULVERDE_VOLTAGE_H

/*
 * linux/include/linux/bulverde_voltage.h
 *
 * Bulverde-specific support for changing CPU voltages via DPM
 *
 * Author: <source@mvista.com>
 *
 * 2003 (c) MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 */

#include <linux/config.h>
#include <linux/notifier.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

#define BLVD_MAX_VOL_C5        1500	/*  in mV.  */
#define BLVD_MIN_VOL_C5        899	/*  in Mv.  */
#define BLVD_MAX_VOL_C0        1400	/*  in mV.  */
#define BLVD_MIN_VOL_C0        850	/*  in Mv.  */

unsigned int mv2DAC(unsigned int mv);
void vm_setvoltage(unsigned int);
unsigned int bulverde_validate_voltage(unsigned int mv);

void bulverde_set_voltage(unsigned int mv);
void bulverde_prep_set_voltage(unsigned int mv);
int bulverde_vcs_init(void);

#endif
