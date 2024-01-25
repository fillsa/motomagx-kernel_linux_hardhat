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
 * 26-Feb-2007  Motorola        Update for HWCFG tree.
 * 28-May-2007  Motorola        Added external prototypes for xPIXL remapping
 * 30-May-2007  Motorola        Added support for xPIXL PWM backlight
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
/**
 * The bootloader initializes the SDRAM IOMUX pad registers.
 */
#define IOMUX_PAD_SETTING_START  9 /* MBM initializes SDRAM pad registers */
#define IOMUX_PAD_SETTING_STOP  (IOMUX_PAD_SETTING_COUNT-1)
/**
 * The first SCM-A11 pad group is mapped to 0x0200 in the HWCFG schema.
 */
#define HWCFG_SCMA11_PAD_BASE   0x0200

/**
 * The number of entries in the hwcfg_pins array.
 */
#define HWCFG_IOMUX_PIN_COUNT   145


/**
 * The first SCM-A11 pin is mapped to 0x01B6 in the HWCFG schema.
 */
#define HWCFG_SCMA11_PIN_BASE   0x01B6


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
    
#if defined(CONFIG_MACH_XPIXL)
extern void __init pixl_gpio_signal_fixup(void);
extern void __init pixl_iomux_mux_fixup(void);
#endif

extern void __init scma11phone_gpio_init(void);

/*
 * PWM Registers for backlight brightness control.
 */
#define PWMCR               IO_ADDRESS(PWM_BASE_ADDR + 0x00)
#define PWMSAR              IO_ADDRESS(PWM_BASE_ADDR + 0x0c)
#define PWMPR               IO_ADDRESS(PWM_BASE_ADDR + 0x10)

#if defined(CONFIG_MACH_XPIXL)
// Settings used with PWM of backligt on xPIXL

// PWM enabled, PWM clock is 32k, clock source is 32K, active in wait and doze mode
#define ENABLE_PWM      0x01830001

// PWM disable, PWM clock is 32k, clock source is 32K, active in wait and doze mode
#define DISABLE_PWM     0x01830000

#define DUTY_CYCLE_0   0
#define DUTY_CYCLE_40  40
#define DUTY_CYCLE_45  45
#define DUTY_CYCLE_50  50
#define DUTY_CYCLE_55  55
#define DUTY_CYCLE_60  60
#define DUTY_CYCLE_65  65
#define DUTY_CYCLE_70  70
#define DUTY_CYCLE_75  75
#define DUTY_CYCLE_80  80
#define DUTY_CYCLE_85  85
#define DUTY_CYCLE_90  90
#define DUTY_CYCLE_95  95
#define DUTY_CYCLE_100 100

#else
#define DUTY_CYCLE_0   0x00000000
#define DUTY_CYCLE_50  0x00000007
#define DUTY_CYCLE_100 0x0000000F
#endif /* CONFIG_MACH_XPIXL */

#endif /* defined(CONFIG_ARCH_MXC91231) && defined(CONFIG_MOT_FEAT_GPIO_API) */

#endif /* __MOT_GPIO_SCMA11__H__ */
