/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/hardware.h>
#include "iomux.h"

#define PIN_TO_IOMUX_INDEX(pin) ((pin >> MUX_I) & ((1 << (MUX_F - MUX_I)) - 1))
#define PIN_TO_IOMUX_FIELD(pin) ((pin >> MUX_F) & ((1 << (PAD_I - MUX_F)) - 1))

/*!
 * 8 bits for each MUX control field
 */
#define MUX_CTL_BIT_LEN         8

/*!
 * PAD control field
 */
#define MUX_PAD_BIT_LEN         16

/*!
 * IOMUX register (base) addresses
 */
enum iomux_reg_addr {
	IOMUXSW_AP_CTL = IO_ADDRESS(IOMUX_AP_BASE_ADDR) + 0x000,	/*!< MUX control for AP side */
	IOMUXSW_SP_CTL = IO_ADDRESS(IOMUX_COM_BASE_ADDR) + 0x000,	/*!< MUX control for SP side */
	IOMUXSW_PAD_CTL = IO_ADDRESS(IOMUX_COM_BASE_ADDR) + 0x200,	/*!< Pad control */
	IOMUXINT_OBS1 = IO_ADDRESS(IOMUX_AP_BASE_ADDR) + 0x600,	/*!< Observe interrupts 0-3 */
	IOMUXINT_OBS2 = IO_ADDRESS(IOMUX_AP_BASE_ADDR) + 0x604,	/*!< Observe interrupt 4 */
};

static DEFINE_RAW_SPINLOCK(gpio_mux_lock);

/*!
 * This function is used to configure a pin through the IOMUX module.
 *
 * @param  pin          a pin number as defined in \b enum \b iomux_pins
 * @param  out          an output function as defined in \b enum \b iomux_output_config
 * @param  in           an input function as defined in \b enum \b iomux_input_config
 */
void iomux_config_mux(enum iomux_pins pin, enum iomux_output_config out,
		      enum iomux_input_config in)
{
	unsigned long gpio_mux_flags;
	unsigned int *base;
	u32 mux_index = PIN_TO_IOMUX_INDEX(pin);
	u32 mux_field = PIN_TO_IOMUX_FIELD(pin);
	int mux_reg;

	if (mux_index <= PIN_TO_IOMUX_INDEX(AP_MAX_PIN)) {
		base = (unsigned int *)(IOMUXSW_AP_CTL);
	} else {
		base = (unsigned int *)(IOMUXSW_SP_CTL);
		mux_index &= 0x7F;
	}

	spin_lock_irqsave(&gpio_mux_lock, gpio_mux_flags);
	mux_reg = __raw_readl(base + mux_index);
	mux_reg &= ~(0xFF << (mux_field * MUX_CTL_BIT_LEN));
	mux_reg |= ((out << 4) | in) << (mux_field * MUX_CTL_BIT_LEN);
	__raw_writel(mux_reg, base + mux_index);
	spin_unlock_irqrestore(&gpio_mux_lock, gpio_mux_flags);
}

/*!
 * This function returns the pad value for a IOMUX group.
 *
 * @param  grp          a group number as defined in \b enum \b #iopad_group
 *
 * @return A group pad value
 */
__u16 iomux_get_pad(enum iopad_group grp)
{
	volatile unsigned int *base =
	    (volatile unsigned int *)(IOMUXSW_PAD_CTL);
	int pad_index = grp / 2;
	int pad_field = grp % 2;

	return (__u16) (base[pad_index] >> (pad_field * MUX_PAD_BIT_LEN));
}

/*!
 * This function configures the pad value for a IOMUX pin.
 *
 * @param  grp          a pin number as defined in \b enum \b #iopad_group
 * @param  config       the ORed value of elements defined in \b enum \b #iomux_pad_config
 */
void iomux_set_pad(enum iopad_group grp, __u32 config)
{
	unsigned long gpio_mux_flags;
	volatile unsigned int *base =
	    (volatile unsigned int *)(IOMUXSW_PAD_CTL);
	int pad_index = grp / 2;
	int pad_field = grp % 2;

	spin_lock_irqsave(&gpio_mux_lock, gpio_mux_flags);
	base[pad_index] =
	    (base[pad_index] & (~(0xFFFF << (pad_field * MUX_PAD_BIT_LEN)))) |
	    (config << (pad_field * MUX_PAD_BIT_LEN));
	spin_unlock_irqrestore(&gpio_mux_lock, gpio_mux_flags);
}

EXPORT_SYMBOL(iomux_config_mux);
EXPORT_SYMBOL(iomux_get_pad);
EXPORT_SYMBOL(iomux_set_pad);
