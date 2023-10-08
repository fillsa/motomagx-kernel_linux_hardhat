/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/lighting.c
 *
 * SCM-A11 implementation of Motorola GPIO API for Lighting.
 *
 * Copyright (C) 2006, 2008 Motorola, Inc.
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
 *
 * Date           Author        Comments
 * ============  ==========    ====================================================
 * 01-Jan-2006  Motorola        Intial version.
 * 28-Jun-2007  Motorola        Removed and renamed MARCO specific signal code in relation to xPIXL.
 * 10/16/2007   Motorola  Added support for SCMA11REF
 * 20-Mar-2008   Motorola      Remove code not related to signal in Nevis product 
 * 03-Jul-2008  Motorola        OSS CV fix.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_CAM_FLASH)
/**
 * Turn the camera flash on or off.
 *
 * @param   eanble  Non-zero enables the camera flash.
 */
void gpio_camera_flash_enable(int enable)
{
    gpio_signal_set_data(GPIO_SIGNAL_CAM_FLASH_EN,
            enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_CAM_FLASH */


#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_CAM_TORCH)
/**
 * Camera Torch Enable; set high to illuminate
 * 
 * @param   enable  Non-zero enables the camera torch.
 */
void gpio_camera_torch_enable(int enable)
{
#if ! defined(CONFIG_MACH_NEVIS) && ! defined(CONFIG_MACH_XPIXL)
    gpio_signal_set_data(GPIO_SIGNAL_CAM_TORCH_EN,
           enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
#endif
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_CAM_TORCH */


#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_EL)
/**
 * Turn the backlight for the number keys on or off.
 *
 * @param   enable  Set to non-zero to enable the backlight.
 */
void gpio_backlight_numbers_enable(int enable)
{
#if defined(CONFIG_MACH_SCMA11REF)
    if(GPIO_SIGNAL_IS_VALID(GPIO_SIGNAL_EL_NUM_EN)) {
        gpio_signal_set_data(GPIO_SIGNAL_EL_NUM_EN,
                enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
    } else {
#endif
#if ! defined(CONFIG_MACH_NEVIS) && ! defined(CONFIG_MACH_XPIXL)
       gpio_signal_set_data(GPIO_SIGNAL_EL_EN,
              enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
#endif
#if defined(CONFIG_MACH_SCMA11REF)
    }
#endif
}


/**
 * Turn the backlight for the navigation keys on or off.
 *
 * @param   enable  Set to non-zero to enable the backlight.
 */
void gpio_backlight_navigation_enable(int enable)
{
#if defined(CONFIG_MACH_SCMA11REF)
    if(GPIO_SIGNAL_IS_VALID(GPIO_SIGNAL_EL_NAV_EN)) {
        gpio_signal_set_data(GPIO_SIGNAL_EL_NAV_EN,
                enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
    } else {
#endif
#if ! defined(CONFIG_MACH_NEVIS) && ! defined(CONFIG_MACH_XPIXL)
        gpio_signal_set_data(GPIO_SIGNAL_EL_EN,
               enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
#endif
#if defined(CONFIG_MACH_SCMA11REF)
    }
#endif
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_EL */


#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
/**
 * Enable or disable the main display's backlight.
 *
 * @param   enable  Set high to enable the backlight; zero to disable it.
 */
void gpio_lcd_backlight_enable(bool enable)
{
    /* PWM_BKL (GP_AP_B17) is no longer connected to the backlight driver
     * on P1A and P1D wingboards. It is only connected to the ETM connectors.
     */
#ifdef CONFIG_MACH_XPIXL
	gpio_signal_set_data(GPIO_SIGNAL_LCD_BACKLIGHT,
			     enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
#else
    gpio_signal_set_data(GPIO_SIGNAL_PWM_BKL,
            enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
    gpio_signal_set_data(GPIO_SIGNAL_MAIN_BKL,
            enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
#endif
}

/**
 * Get status of lcd backlight gpio signals.
 *
 * @return  Status of the GPIO_SIGNAL_PWM_BKL and GPIO_SIGNAL_MAIN_BKL signals.
 */
int gpio_get_lcd_backlight(void)
{
#ifdef CONFIG_MACH_XPIXL
	return gpio_signal_get_data_check(GPIO_SIGNAL_LCD_BACKLIGHT);
#else
    return gpio_signal_get_data_check(GPIO_SIGNAL_MAIN_BKL);
#endif
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD */
