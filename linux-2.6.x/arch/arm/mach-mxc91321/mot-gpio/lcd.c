/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/lcd.c
 *
 * ArgonLV implementation of LCD GPIO signal control.
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
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 01-Nov-2006  Motorola        Initial revision.
 * 22-Nov-2006  Motorola        Test validity of DISP_RST_B
 * 02-Mar-2007  Motorola        Replace msleep with mdelay for atomic criteria.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/mot-gpio.h>

#include "mot-gpio-argonlv.h"
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
    /* Main Display Shut Down; set low to enable display */
    if(GPIO_SIGNAL_IS_VALID(GPIO_SIGNAL_SERDES_RESET_B)) {
        /*
         * Serializer Enable; set high to enable serializer.
         *
         * Pin USB1_VMOUT muxed to MCU GPIO Port Pin 28 at boot.
         */
        gpio_signal_set_data(GPIO_SIGNAL_SERDES_RESET_B, GPIO_DATA_HIGH);

        /* must wait 20us before waking up serializer */
        udelay(20);

        /*
         * Serializer Standby; set high to take wakeup serializer.
         * 
         * Pin USB1_TXENB muxed to MCU GPIO Port Pin 29 at boot.
         */
        gpio_signal_set_data(GPIO_SIGNAL_STBY, GPIO_DATA_HIGH);

        /* Delay between the SER_EN (SERDES STBY) and Data strobe
         * as per the SERDES FIN spec
         */
        udelay(50);

        /* Start pixel clock here */
        enable_ipu_pixel_clock();

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

        /* Pin GPIO14 muxed to MCU GPIO Port Pin 14 at boot. */
        gpio_signal_set_data(GPIO_SIGNAL_LCD_SD, GPIO_DATA_LOW);
    } else if(GPIO_SIGNAL_IS_VALID(GPIO_SIGNAL_DISP_RST_B)) {
        /* Pin USB1_VMOUT muxed to MCU GPIO Port Pin 28 at boot. */
        gpio_signal_set_data(GPIO_SIGNAL_DISP_RST_B, GPIO_DATA_LOW);
    }

    /*
     * Magic numbers from the table "PWM settings" in the IPU chapter
     * of the Bute ICD.
     */
    /* set pwm duty cycle to 100% (controls brightness) */
    writel(0x0000000F, PWMSAR);
    /* pwm period is 16 clock cycles */
    writel(0x0000000D, PWMPR);
    /* 
     * PWM enabled, PWM clock is 32k, Clock source is 32K, active in 
     * wait and doze modes. 
     */
    writel(0x01830001, PWMCR);
}


/*!
 * Setup GPIO for LCD to be inactive
 *
 */
void gpio_lcd_inactive(void)
{
    /* Main Display Shut Down; set high to disable display */
    if(GPIO_SIGNAL_IS_VALID(GPIO_SIGNAL_SERDES_RESET_B)) {
        /* Pin GPIO14 muxed to MCU GPIO Port Pin 14 at boot. */
        gpio_signal_set_data(GPIO_SIGNAL_LCD_SD, GPIO_DATA_HIGH);

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

        /* Turn Off the display clock */
        disable_ipu_pixel_clock();

        /*
         * Serializer Standby; set low to put serializer asleep.
         * 
         * Pin USB1_TXENB muxed to MCU GPIO Port Pin 29 at boot.
         */
        gpio_signal_set_data(GPIO_SIGNAL_STBY, GPIO_DATA_LOW);

        /* turn off serializer */
        gpio_signal_set_data(GPIO_SIGNAL_SERDES_RESET_B, GPIO_DATA_LOW);
    } else if(GPIO_SIGNAL_IS_VALID(GPIO_SIGNAL_DISP_RST_B)) {
        /* Pin USB1_VMOUT muxed to MCU GPIO Port Pin 28 at boot. */
        gpio_signal_set_data(GPIO_SIGNAL_DISP_RST_B, GPIO_DATA_HIGH);
    }
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_LCD */
