/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/mot-gpio-argonlv.h
 *
 * Prototypes and definitions used for the Argon implementation of the
 * Motorola GPIO API.
 *
 * Copyright 2006 Motorola, Inc.
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
 * 01-Nov-2006  Motorola        Initial revision.
 * 26-Nov-2006  Motorola        Added support for GPU GPIO signal control.
 */

#ifndef __MOT_GPIO_ARGONLV__H__
#define __MOT_GPIO_ARGONLV__H__

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#if defined(CONFIG_ARCH_MXC91321) && defined(CONFIG_MOT_FEAT_GPIO_API)

#include "../iomux.h"

/**
 * Description of a pin's IOMUX mux control register settings.
 */
struct iomux_initialization {
    enum iomux_pins             pin;
    enum iomux_output_config    out;
    enum iomux_input_config     in;
};

/**
 * Array of arrays containing mux register settings for ArgonLV pins.
 */
extern struct iomux_initialization * __initdata initial_iomux_settings[];

extern void __init argonlvphone_gpio_init(void);

extern void __init argonlv_iomux_pad_init(void);
extern void __init argonlv_iomux_mux_init(void);

#if defined(CONFIG_MACH_BUTEREF)
extern void __init buteref_gpio_signal_fixup(void);
#elif defined(CONFIG_MOT_FEAT_GPIO_API_GPU)
extern void __init gpu_gpio_signal_fixup(void);
#endif

/*
 * PWM Registers for backlight brightness control.
 */
#define PWMCR               IO_ADDRESS(PWM_BASE_ADDR + 0x00)
#define PWMSAR              IO_ADDRESS(PWM_BASE_ADDR + 0x0c)
#define PWMPR               IO_ADDRESS(PWM_BASE_ADDR + 0x10)

#define DUTY_CYCLE_0   0x00000000
#define DUTY_CYCLE_50  0x00000007
#define DUTY_CYCLE_100 0x0000000F

#endif /* defined(CONFIG_MACH_MXC91321) && defined(CONFIG_MOT_FEAT_GPIO_API) */

#endif /* __MOT_GPIO_ARGONLV__H__ */
