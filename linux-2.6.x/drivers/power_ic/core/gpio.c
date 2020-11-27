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
 * Motorola 2008-Jan-29 - Removed non-xPIXL specific signal code.
 * Motorola 2008-Jan-11 - Cradle charger only supported on Marco
 * Motorola 2007-Feb-07 - Add support for cradle charger 
 * Motorola 2007-Jan-08 - Update copyright
 * Motorola 2006-Nov-21 - Fix undefined use of PM_EDIO
 * Motorola 2006-Nov-16 - Add conditional compile in int power_ic_gpio_event_read_priint()
 * Motorola 2006-Nov-10 - Add support for Marco.
 * Motorola 2006-Nov-09 - ReConfigure GPIO Keypad backlight
 * Motorola 2006-Nov-01 - Support LJ7.1 Reference Design
 * Motorola 2006-Oct-12 - Add support for Maxim LED controller chip.
 * Motorola 2006-Oct-06 - Update File
 * Motorola 2006-Sep-08 - Fix use of BOARDREV for Lido and Saipan.
 * Motorola 2006-Aug-15 - Add Elba support.
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-14 - Add Lido & Saipan Support
 * Motorola 2006-Jun-28 - Montavista header upmerges
 * Motorola 2006-Jun-19 - Fix montavista upmerge conditionals
 * Motorola 2006-Jun-14 - Modify keypad lights for HW revisions.
 * Motorola 2006-Apr-24 - Add 3.5mm headset support
 * Motorola 2006-Apr-04 - Confine all GPIO functions to this file
 */

 /*!
 * @file gpio.c
 *
 * @ingroup poweric_core
 *
 * @brief The main interface to GPIOs.
 *
 * This file contains all the configurations, setting, and receiving of all GPIOs
 */
#include <stdbool.h>

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/power_ic_kernel.h>
/* Must include before interrupt.h since interrupt.h depends on sched.h but is
 * not included in interrupt.h */
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/hardware.h>
#include <asm/irq.h>

#include <asm/boardrev.h>
#include <asm/mot-gpio.h>
#include <asm/arch/gpio.h>
#ifdef CONFIG_ARCH_MXC91321
#include <asm/arch/mxc91321_pins.h>
#elif defined(CONFIG_ARCH_MXC91231)
#include <asm/arch/mxc91231_pins.h>
#endif

#include "event.h"
#include "gpio.h"
#include "os_independent.h"

/******************************************************************************
* Local type definitions
******************************************************************************/

/******************************************************************************
* Local constants
******************************************************************************/
/*!
 * @name Main Display Lighting
 */
/*@{*/
#define LIGHTS_FL_MAIN_BKL_PIN               30
#define LIGHTS_FL_PWM_BKL_PIN                17
/*@}*/

/*!
 * @name EL Keypad Lighting
 */
/*@{*/
#define LIGHTS_FL_EL_BASE                    29
#define LIGHTS_FL_EL_SLIDER                  27
/*@}*/

/*!
 * @name Camera Flash
 */
/*@{*/
#define LIGHTS_FL_CAMERA_EN_P2               13
#define LIGHTS_FL_TORCH_FLASH                30
/*@}*/

/******************************************************************************
* Local variables
******************************************************************************/

/******************************************************************************
* Local function prototypes
******************************************************************************/

/******************************************************************************
* Global functions
******************************************************************************/
/*!
 * @brief Initializes the power IC event GPIO lines
 *
 * This function initializes the power IC event GPIOs.  This includes:
 *     - Configuring the GPIO interrupt lines
 *     - Registering the GPIO interrupt handlers
 *
 */
void power_ic_gpio_config_event_int(void)
{
#if defined(CONFIG_ARCH_MXC91231)
    /* Configure mux */
    iomux_config_mux(AP_ED_INT5, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
    /* Configure and register the ATLAS interrupt */
    set_irq_type(PM_INT, IRQT_RISING);
    request_irq(PM_INT, power_ic_irq_handler, SA_INTERRUPT, "Atlas irq: SCM-A11", 0);
#elif defined(CONFIG_ARCH_MXC91321)
    /* Configure mux */
    iomux_config_mux(PIN_PM_INT, OUTPUTCONFIG_ALT2, INPUTCONFIG_ALT2);
    /* Configure and register the ATLAS interrupt */
    set_irq_type(PM_INT, IRQT_RISING);
    request_irq(PM_INT, power_ic_irq_handler, SA_INTERRUPT, "Atlas irq: BUTE", 0);
#endif
}

/*!
 * @brief Read the power IC event GPIO lines
 *
 * This function will read the primary interrupt GPIO line
 *
 * @return UINT32 value of the GPIO data
 *
 */
int power_ic_gpio_event_read_priint(unsigned int signo)
{
#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_MACH_ARGONLVREF)
    return(edio_get_data(PM_EDIO));
#else
    return 0;
#endif
}

/*!
 * @brief Configure the power IC EMU GPIO lines
 *
 * This function will configure the EMU GPIO lines for D+ and D-
 */
void power_ic_gpio_emu_config(void)
{
#ifdef CONFIG_ARCH_MXC91231
    iomux_config_mux(AP_GPIO_AP_C16, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC3);
    iomux_config_mux(AP_GPIO_AP_C17, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC3);
    gpio_config(GPIO_AP_C_PORT, 16, false, GPIO_INT_NONE);
    gpio_config(GPIO_AP_C_PORT, 17, false, GPIO_INT_NONE);

#ifdef CONFIG_MACH_MARCO
    /* Configure the cradle detect */
    iomux_config_mux(AP_ED_INT7, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
    gpio_config(GPIO_AP_C_PORT, 25, false, GPIO_INT_NONE);
#endif
    
#elif defined(CONFIG_ARCH_MXC91321)
    iomux_config_mux(PIN_USB_VPIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_GPIO);
    iomux_config_mux(PIN_USB_VMIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_GPIO);
    iomux_config_mux(PIN_GPIO4, OUTPUTCONFIG_FUNC, INPUTCONFIG_GPIO);
    iomux_config_mux(PIN_GPIO6, OUTPUTCONFIG_FUNC, INPUTCONFIG_GPIO);
    gpio_config(0, 4, false, GPIO_INT_NONE);  /* D+ for GPIO 4 (0 Base) */
    gpio_config(0, 6, false, GPIO_INT_NONE);  /* D- for GPIO 6 (0 Base) */
#endif
}

/*!
 * @brief Initializes the lighting
 *
 * The function performs any initialization required to get the lighting
 * for SCM-A11 running using GPIOs.
 */
void power_ic_gpio_lights_config(void)
{
#if defined(CONFIG_ARCH_MXC91231) && defined(CONFIG_MACH_SCMA11REF)
    iomux_config_mux(AP_IPU_D3_PS, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
    gpio_config(GPIO_AP_A_PORT, LIGHTS_FL_MAIN_BKL_PIN, true, GPIO_INT_NONE);
    gpio_set_data(GPIO_AP_A_PORT, LIGHTS_FL_MAIN_BKL_PIN, 1);
#elif defined(CONFIG_ARCH_MXC91231) && !(defined(CONFIG_MACH_ELBA)) && !(defined(CONFIG_MACH_XPIXL))
    /* EL Keypad GPIOs */
    iomux_config_mux(SP_SPI1_CLK, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
    gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_EL_BASE, 0);
    gpio_config(GPIO_SP_A_PORT, LIGHTS_FL_EL_BASE, true, GPIO_INT_NONE);

    iomux_config_mux(SP_SPI1_MISO, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
    gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_EL_SLIDER, 0);
    gpio_config(GPIO_SP_A_PORT, LIGHTS_FL_EL_SLIDER, true, GPIO_INT_NONE);

    /* Camera Flash GPIOs */
    iomux_config_mux(SP_SPI1_SS0, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
    gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_TORCH_FLASH, 0);
    gpio_config(GPIO_SP_A_PORT, LIGHTS_FL_TORCH_FLASH, true, GPIO_INT_NONE);

    iomux_config_mux(SP_UH2_RXDM, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
    gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_CAMERA_EN_P2, 0);
    gpio_config(GPIO_SP_A_PORT, LIGHTS_FL_CAMERA_EN_P2, true, GPIO_INT_NONE);
#endif /* SCM-A11 & ASCENSION */
}

/*!
 * @brief Set main display
 *
 *  This function will handle turning on and off the main display thru GPIO
 *
 * @param   nColor     The color to set the region to.
 */
void power_ic_gpio_lights_set_main_display(LIGHTS_FL_COLOR_T nColor)
{
#if defined(CONFIG_ARCH_MXC91231) && defined(CONFIG_MACH_SCMA11REF)
    gpio_set_data(GPIO_AP_A_PORT, LIGHTS_FL_MAIN_BKL_PIN, (nColor != 0));
#endif
}


/*!
 * @brief Update keypad base light
 *
 * This function will handle keypad base light update.
 *
 * @param  nColor       The color to set the region to.
 */
void power_ic_gpio_lights_set_keypad_base(LIGHTS_FL_COLOR_T nColor)
{
#if defined(CONFIG_ARCH_MXC91231)
    gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_EL_BASE, (nColor != 0));
#endif
}

/*!
 * @brief Update keypad slider light
 *
 * This function will handle keypad slider light update.
 *
 * @param  nColor       The color to set the region to.
 */
void power_ic_gpio_lights_set_keypad_slider(LIGHTS_FL_COLOR_T nColor)
{
#if defined(CONFIG_ARCH_MXC91231)
    gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_EL_SLIDER, (nColor != 0));
#endif
}

/*!
 * @brief Sets the Camera Flash and Torch Mode
 *
 * This function sets the camera flash and torch mode to the passed setting
 *
 * @param  nColor       The color to set the region to.
 */
void power_ic_gpio_lights_set_camera_flash(LIGHTS_FL_COLOR_T nColor)
{
#if defined(CONFIG_ARCH_MXC91231)
    if(nColor == LIGHTS_FL_CAMERA_FLASH )
    {
        /*Turn on the camera flash*/
        gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_CAMERA_EN_P2, 1);
        gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_TORCH_FLASH, 1);
    }
    else if (nColor == LIGHTS_FL_CAMERA_TORCH)
    {
        /*Turn on the camera torch mode*/
        gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_CAMERA_EN_P2, 1);
        gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_TORCH_FLASH, 0);
    }
    else
    {
        /*Turn off the camera flash*/
        gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_CAMERA_EN_P2, 0);
        gpio_set_data(GPIO_SP_A_PORT, LIGHTS_FL_TORCH_FLASH, 0);
    }
#endif
}
