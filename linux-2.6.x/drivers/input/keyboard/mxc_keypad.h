/*
 * driver/input/keypad/mcx_keypad.h
 *
 * Keypad driver for Freescale MXC development boards
 * supports the zeus evb 8X8 keypad
 *
 * Author: Armin Kuster <someone@mvista.com>
 *
 * Initially based on drivers/char/mxc_keyb.h
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef _MXC_KEYB_H
#define _MXC_KEYB_H

#define KPCR    IO_ADDRESS(KPP_BASE_ADDR + 0x00)
#define KPSR    IO_ADDRESS(KPP_BASE_ADDR + 0x02)
#define KDDR    IO_ADDRESS(KPP_BASE_ADDR + 0x04)
#define KPDR    IO_ADDRESS(KPP_BASE_ADDR + 0x06)

#define KBD_STAT_KPKD        0x01
#define KBD_STAT_KPKR        0x02
#define KBD_STAT_KDSC        0x04
#define KBD_STAT_KRSS        0x08
#define KBD_STAT_KDIE        0x100
#define KBD_STAT_KRIE        0x200
#define KBD_STAT_KPPEN       0x400

/* these will need to be changed
 * when unique names are given to
 * them
 */
void gpio_keypad_active(void);
void gpio_keypad_inactive(void);

#endif				/* _MXC_KEYB_H */
