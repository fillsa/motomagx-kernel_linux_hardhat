/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/lcd.c
 *
 * SCM-A11 implementation of Motorola GPIO API for controlling main
 * display.
 *
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * Revision History:
 * Date         Author          Comment
 * ===========  ==============  ==============================================
 * 03/07/2006   Motorola        Initial version.
 * 03/10/2007   Motorola        Replace msleep with mdelay.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/mot-gpio.h>

#ifdef CONFIG_MOT_FEAT_KPANIC
extern int kpanic_in_progress;
#endif

#if defined(CONFIG_MOT_FEAT_GPIO_API_LCD)
extern void disable_ipu_pixel_clock(void);
extern void enable_ipu_pixel_clock(void);

/*!
 * Setup GPIO for LCD to be active
 *
 */
void gpio_lcd_active(void)
{
    /* Reset the LCD. */
#ifndef CONFIG_MACH_PICO
    /* Do not reset PICO lcd, will result in display inversion */
    gpio_signal_set_data(GPIO_SIGNAL_DISP_RST_B, GPIO_DATA_HIGH);

    /* Delay between LCD reset and LCD standby. */
    udelay(20);
#endif
    /* Enable the serializer. Has no effect on boards where GPIO_SIGNAL_SER_EN
     * is marked invalid. */
    gpio_signal_set_data(GPIO_SIGNAL_SER_EN, GPIO_DATA_HIGH);

    /* delay before enabling pixel clock */
    udelay(50);
#ifdef CONFIG_MACH_ELBA /*When LCD comes to active state from inactive state restore pin polarity*/
    change_polarity_ipu_sdc_signals(0, 0, 0);
    udelay(50);
#endif    

#ifndef CONFIG_MACH_PICO
    /* Start pixel clock here */
    enable_ipu_pixel_clock();
#else
    udelay(10);/*delay 50+10 between enabling serialiser and SDC module*/
    ipu_enable_sdc();/*turn on dotclk, hsynch, vsynch*/
#endif

    /* tclk-sd : time between MCLK - onto falling edge of SD */
#ifdef CONFIG_MOT_FEAT_KPANIC
    if (kpanic_in_progress) {
	mdelay(1);
    } else {
        msleep(1);
    }
#else 
    msleep(1);
#endif
    /* Enable the LCD. */
    //gpio_signal_set_data(GPIO_SIGNAL_DISP_SD, GPIO_DATA_LOW);

}


/*!
 * Setup GPIO for LCD to be inactive
 *
 */
void gpio_lcd_inactive(void)
{
    /* shut down the display */
    //gpio_signal_set_data(GPIO_SIGNAL_DISP_SD, GPIO_DATA_HIGH);

    /* tsd-off : Rising edge of SD to Display off*/
#ifdef CONFIG_MOT_FEAT_KPANIC
    if (kpanic_in_progress) {
        mdelay(70);
    } else {
        msleep(70);
    }
#else 
    msleep(70);
#endif

#ifndef CONFIG_MACH_PICO
    /* Do not turn off for PICO, needed for sec display access */
    /* Turn Off the display clock */
    disable_ipu_pixel_clock();
#else
    ipu_disable_sdc();/*turn off dotclk, hsynch, vsynch*/
#endif
#ifdef CONFIG_MACH_ELBA /*When LCD is shutdown vysnch, hsynch and dotclock output should be low*/
    change_polarity_ipu_sdc_signals(1, 1, 1);
#endif

    /* Disable the serializer. Has no effect on boards where GPIO_SIGNAL_SER_EN
     * is marked invalid. */
    gpio_signal_set_data(GPIO_SIGNAL_SER_EN, GPIO_DATA_LOW);

#ifndef CONFIG_MACH_PICO
    /* Do not reset PICO lcd. Will result in display inversion */
    /* Place display in reset. */
    gpio_signal_set_data(GPIO_SIGNAL_DISP_RST_B, GPIO_DATA_LOW);
#endif
}

/*!
 * Setup GPIO for the serializer RESET 
 *
 * @param   asserted    If set to "assert", serializer will be held in reset.
 */
void gpio_lcd_display_reset(enum gpio_signal_assertion asserted)
{
    /* GPIO_SIGNAL_DISP_RST_B is serializer reset */
    gpio_signal_set_data(GPIO_SIGNAL_DISP_RST_B,
            (asserted == ASSERT_GPIO_SIGNAL) ? GPIO_DATA_LOW : GPIO_DATA_HIGH);
}
void gpio_lcd_serializer_reset(enum gpio_signal_assertion asserted)
{
    /* GPIO_SIGNAL_SER_RST_B is serializer reset */
    gpio_signal_set_data(GPIO_SIGNAL_SER_RST_B,
            (asserted == ASSERT_GPIO_SIGNAL) ? GPIO_DATA_LOW : GPIO_DATA_HIGH);
	/* GPIO_SIGNAL_DISP_RST_B is display reset */
    gpio_signal_set_data(GPIO_SIGNAL_DISP_RST_B,
            (asserted == ASSERT_GPIO_SIGNAL) ? GPIO_DATA_LOW : GPIO_DATA_HIGH);
}

/*!
 * Setup GPIO for serializer STBY
 *
 * @param   asserted    If set to "assert", serializer will be ins standby.
 */
void gpio_lcd_serializer_stby(enum gpio_signal_assertion asserted)
{
    /* 
     * Pull the serializer out of STBY. Must occur at least 
     * 20us after calling gpio_lcd_serializer_reset
     */
    gpio_signal_set_data(GPIO_SIGNAL_SER_EN,
            (asserted == ASSERT_GPIO_SIGNAL) ? GPIO_DATA_LOW : GPIO_DATA_HIGH);
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_LCD */
