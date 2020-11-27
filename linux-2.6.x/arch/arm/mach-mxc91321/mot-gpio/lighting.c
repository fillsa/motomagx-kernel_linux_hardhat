/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/lighting.c
 *
 * ArgonLV implementation of lighting GPIO signal control.
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
 */

#include "mot-gpio-argonlv.h"

#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
/**
 * Turn on or off the main display backlight.
 *
 * @param enable True to turn on the backlight; false to turn it off.
 */
void gpio_lcd_backlight_enable(bool enable)
{
    /* 
     * Main Display Backlight -- set high to enable backlight.
     * 
     * Pin USB1_VPIN muxed to MCU GPIO Port Pin 30 at boot.
     */
    gpio_signal_set_data(GPIO_SIGNAL_LCD_BACKLIGHT,
            enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);    
}


/**
 * Get status of lcd backlight gpio signals.
 *
 * @return  Status of the LCD_Backlight signal.
 */
int gpio_get_lcd_backlight(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_LCD_BACKLIGHT);
}


/**
 *  Set the display backlight intensity.
 *
 *  @param  value   Intensity level; range 0-100
 */
void pwm_set_lcd_bkl_brightness(int value)
{
    int dutycycle = DUTY_CYCLE_0;
    if (value > 0)
    {
        dutycycle = (value <= 50) ? DUTY_CYCLE_50 : DUTY_CYCLE_100;
    }
    /* set pwm duty cycle (controls brightness) */
    writel(dutycycle, PWMSAR);
}


/**
 * Get the display backlight intensity. 
 *
 * @return Backlight intensity, a value between 0 and 100. 
 */
int pwm_get_lcd_bkl_brightness(void)
{
    int dutycycle, value = 0;
    dutycycle = readl(PWMSAR);
    if (dutycycle > DUTY_CYCLE_0)
    {
        value = (dutycycle <=  DUTY_CYCLE_50) ? 50 : 100;
    }
    return value;
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD */
