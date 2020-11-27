/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/mot-gpio-scma11.h
 *
 * Prototypes and definitions used for the SCM-A11 implementation of the
 * Motorola GPIO API.
 *
 * Copyright 2006-2007 Motorola, Inc.
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
 * 19-Oct-2006  Motorola        Initial revision.
 * 02-Jan-2007  Motorola        Added support for Lido P2.
 */

#ifndef __MOT_GPIO_SCMA11__H__
#define __MOT_GPIO_SCMA11__H__

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#if defined(CONFIG_ARCH_MXC91231) && defined(CONFIG_MOT_FEAT_GPIO_API)

#include "../iomux.h"

/**
 * Contains an IOMUX pad register and its desired setting.
 */
struct iomux_pad_setting {
    enum iopad_group grp;
    __u32 config;
};

#define IOMUX_PAD_SETTING_COUNT 28 /* highest index in array is 27 */
#define IOMUX_PAD_SETTING_START  9 /* MBM initializes SDRAM pad registers */
#define IOMUX_PAD_SETTING_STOP  (IOMUX_PAD_SETTING_COUNT-1)

extern struct iomux_pad_setting __initdata iomux_pad_register_settings[IOMUX_PAD_SETTING_COUNT];

#if defined(CONFIG_MACH_SCMA11REF)
extern void __init scma11ref_gpio_signal_fixup(void);
extern void __init scma11ref_iomux_mux_fixup(void);
#endif /* CONFIG_MACH_SCMA11REF */

#if defined(CONFIG_MACH_ELBA)
extern void __init elba_gpio_signal_fixup(void);
#endif /* CONFIG_MACH_ELBA */

#if defined(CONFIG_MACH_LIDO)
extern void __init lido_gpio_signal_fixup(void);
extern void __init lido_iomux_mux_fixup(void);
#endif /* CONFIG_MACH_LIDO */
    
extern void __init scma11_iomux_pad_init(void);
extern void __init scma11_iomux_mux_init(void);

extern void __init scma11phone_gpio_init(void);

#endif /* defined(CONFIG_ARCH_MXC91231) && defined(CONFIG_MOT_FEAT_GPIO_API) */

#endif /* __MOT_GPIO_SCMA11__H__ */
