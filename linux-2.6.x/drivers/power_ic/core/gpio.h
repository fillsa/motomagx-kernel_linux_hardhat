/*
 * Copyright (C) 2006-2008 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Motorola 2008-Jan-11 - Cradle charger only supported on Marco
 * Motorola 2007-Feb-22 - Removed transflash driver
 * Motorola 2007-Feb-19 - Define EMU_GET_CRADLE_DETECT for Argon products
 * Motorola 2007-Feb-07 - Add support for cradle charger
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Nov-01 - Support LJ7.1 Reference Design
 * Motorola 2006-Oct-06 - Update File
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jun-28 - Montavista header upmerge fixes
 * Motorola 2006-Jun-19 - Fix montavista upmerge conditionals
 * Motorola 2006-Apr-04 - Confine all GPIO functions to this file
 */

#ifndef __GPIO_H__
#define __GPIO_H__

 /*!
 * @file gpio.h
 *
 * @ingroup poweric_core
 *
 * @brief The main interface to GPIOs.
 *
 * This file contains all the function declarations for GPIOs
 */

#include <linux/lights_funlights.h>

#include <asm/mot-gpio.h>
#ifdef CONFIG_ARCH_MXC91321
#include <asm/arch/mxc91321.h>
#include <asm/arch/mxc91321_pins.h>
#elif defined(CONFIG_ARCH_MXC91231)
#include <asm/arch/mxc91231.h>
#include <asm/arch/mxc91231_pins.h>
#endif

/******************************************************************************
* Local constants
******************************************************************************/
/*!
 * @brief Used to prevent unused variable warnings during compilation
 */
#define POWER_IC_UNUSED __attribute((unused))

/*!
 * @brief define GPIO for stereo emu headset
 */
#define GPIO99_ST_HST  99

#if defined(CONFIG_ARCH_MXC91231)
#define PM_INT      INT_EXT_INT5
#define PM_EDIO     ED_INT5
#elif defined(CONFIG_ARCH_MXC91321)
#define PM_INT      INT_EXT_INT1
#define PM_EDIO     ED_INT1
#endif /* MXC91321 */

/******************************************************************************
* Macros
******************************************************************************/
/* Define GPIOs specific to SCMA11 */
#if defined(CONFIG_ARCH_MXC91231)
#define EMU_GET_D_PLUS() \
({\
    ((gpio_get_data(GPIO_AP_C_PORT, 16)) ? \
        EMU_BUS_SIGNAL_STATE_ENABLED : EMU_BUS_SIGNAL_STATE_DISABLED); \
})

#define EMU_GET_D_MINUS() \
({\
    ((gpio_get_data(GPIO_AP_C_PORT, 17)) ? \
      EMU_BUS_SIGNAL_STATE_ENABLED : EMU_BUS_SIGNAL_STATE_DISABLED); \
})

/* Define GPIOs specific to BUTE */
#elif defined(CONFIG_ARCH_MXC91321)
#define EMU_GET_D_PLUS() \
({\
    ((gpio_get_data(0, 4)) ? \
        EMU_BUS_SIGNAL_STATE_ENABLED : EMU_BUS_SIGNAL_STATE_DISABLED); \
})

#define EMU_GET_D_MINUS() \
({\
    ((gpio_get_data(0, 6)) ? \
        EMU_BUS_SIGNAL_STATE_ENABLED : EMU_BUS_SIGNAL_STATE_DISABLED); \
})
#endif

#if defined(CONFIG_MACH_MARCO)
/* The cradle detect is active low */
#define EMU_GET_CRADLE_DETECT() \
    ((edio_get_data(ED_INT7)) ? 0 : 1)

/* The cradle is not supported on other products. */
#else
#define EMU_GET_CRADLE_DETECT() 0
#endif

/******************************************************************************
* Function prototypes
******************************************************************************/
void power_ic_gpio_config_event_int(void);
int power_ic_gpio_event_read_priint(unsigned int signo);
void power_ic_gpio_emu_config(void);
void power_ic_gpio_lights_config(void);
void power_ic_gpio_lights_set_main_display(LIGHTS_FL_COLOR_T nColor);
void power_ic_gpio_lights_set_keypad_base(LIGHTS_FL_COLOR_T nColor);
void power_ic_gpio_lights_set_keypad_slider(LIGHTS_FL_COLOR_T nColor);
void power_ic_gpio_lights_set_camera_flash(LIGHTS_FL_COLOR_T nColor);

#endif /* __GPIO_H__ */
