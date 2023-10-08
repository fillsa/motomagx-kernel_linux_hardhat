/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2007 Motorola, Inc.
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

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 26-Apr-2007  Motorola        Fixed IOMUX pad bit width problem. (WFN491)
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/hardware.h>
#include "iomux.h"

#define PIN_TO_IOMUX_INDEX(pin) ((pin >> MUX_I) & ((1 << (MUX_F - MUX_I)) - 1))
#define PIN_TO_IOMUX_FIELD(pin) ((pin >> MUX_F) & ((1 << (PAD_I - MUX_F)) - 1))
#define PIN_TO_PAD_INDEX(pin) ((pin >> PAD_I) & ((1 << (PAD_F - PAD_I)) - 1))
#define PIN_TO_PAD_FIELD(pin) ((pin >> PAD_F) & ((1 << (MUX_RSVD - PAD_F)) - 1))

/*!
 * 8 bits for each MUX control field
 */
#define MUX_CTL_BIT_LEN         8

/*!
 * PAD control field
 */
#if defined(CONFIG_MOT_WFN491)
#define MUX_PAD_BIT_LEN         10
#else
#define MUX_PAD_BIT_LEN         9
#endif

/*!
 * IOMUX register (base) addresses
 */
enum iomux_reg_addr {
	IOMUXINT_OBS1 = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x000,	/*!< Observe interrupts 1 */
	IOMUXINT_OBS2 = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x004,	/*!< Observe interrupts 2 */
	IOMUXGPR = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x030,	/*!< General purpose */
	IOMUXSW_MUX_CTL = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x034,	/*!< MUX control actually starts here not 0xC */
	IOMUXSW_PAD_CTL = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x1E4	/*!< Pad control */
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
	volatile unsigned int *base =
	    (volatile unsigned int *)(IOMUXSW_MUX_CTL);
	u32 mux_index = PIN_TO_IOMUX_INDEX(pin);
	u32 mux_field = PIN_TO_IOMUX_FIELD(pin);

	spin_lock_irqsave(&gpio_mux_lock, gpio_mux_flags);
	base[mux_index] =
	    (base[mux_index] & (~(0xFF << (mux_field * MUX_CTL_BIT_LEN)))) |
	    (((out << 4) | in) << (mux_field * MUX_CTL_BIT_LEN));
	spin_unlock_irqrestore(&gpio_mux_lock, gpio_mux_flags);
}

/*!
 * This function configures the pad value for a IOMUX pin.
 *
 * @param  pin          a pin number as defined in \b enum \b #iomux_pins
 * @param  config       the ORed value of elements defined in \b enum \b #iomux_pad_config
 */
void iomux_config_pad(enum iomux_pins pin, __u32 config)
{
	unsigned long gpio_mux_flags;
	volatile unsigned int *base =
	    (volatile unsigned int *)(IOMUXSW_PAD_CTL);
	int pad_index = PIN_TO_PAD_INDEX(pin);
	int pad_field = PIN_TO_PAD_FIELD(pin);

	spin_lock_irqsave(&gpio_mux_lock, gpio_mux_flags);
#if defined(CONFIG_MOT_WFN491)
	base[pad_index] =
	    (base[pad_index] & (~(0x3FF << (pad_field * MUX_PAD_BIT_LEN)))) |
	    (config << (pad_field * MUX_PAD_BIT_LEN));
#else
	base[pad_index] =
	    (base[pad_index] & (~(0x1FF << (pad_field * MUX_PAD_BIT_LEN)))) |
	    (config << (pad_field * MUX_PAD_BIT_LEN));
#endif
	spin_unlock_irqrestore(&gpio_mux_lock, gpio_mux_flags);
}

/*!
 * This function enables/disables the general purpose function for a particular
 * signal.
 *
 * @param  gp   one signal as defined in \b enum \b #iomux_gp_func
 * @param  en   \b #true to enable; \b #false to disable
 */
void iomux_config_gpr(enum iomux_gp_func gp, bool en)
{
	unsigned long gpio_mux_flags;
	volatile unsigned int *base = (volatile unsigned int *)(IOMUXGPR);

	spin_lock_irqsave(&gpio_mux_lock, gpio_mux_flags);
	if (en) {
		*base |= gp;
	} else {
		*base &= ~gp;
	}
	spin_unlock_irqrestore(&gpio_mux_lock, gpio_mux_flags);
}

EXPORT_SYMBOL(iomux_config_mux);
EXPORT_SYMBOL(iomux_config_pad);
EXPORT_SYMBOL(iomux_config_gpr);
