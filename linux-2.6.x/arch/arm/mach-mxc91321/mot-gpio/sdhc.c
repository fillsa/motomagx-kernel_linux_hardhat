/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/sdhc.c
 *
 * ArgonLV implementation of SD GPIO signal control.
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
 * 01-Nov-2006  Motorola        Initial revision.
 * 09-Jul-2007  Motorola        Added gpio_free_irq_dev work around.
 * 08-Aug-2007  Motorola        Modified function sdhc_get_max_clock.
 */

#include <linux/errno.h>

#include "mot-gpio-argonlv.h"

#if defined(CONFIG_MOT_FEAT_GPIO_API_SDHC)
/*!
 * Setup IOMUX for Secure Digital Host Controller to be active.
 *
 * @param module SDHC module number
 */
void gpio_sdhc_active(int module) 
{
}


/*!
 * Setup GPIO for Secure Digital Host Controller to be inactive
 *
 * @param module SDHC module number
 */
void gpio_sdhc_inactive(int module) 
{
}


/*!
 * Setup the IOMUX/GPIO for SDHC1 SD1_DET.
 * 
 * @param  module SDHC module number
 * @param  host Pointer to MMC/SD host structure.
 * @param  handler      GPIO ISR function pointer for the GPIO signal.
 * @return The function returns 0 on success and -1 on failure.
 */
int sdhc_intr_setup(int module, void *host, gpio_irq_handler handler) 
{
    if(module == 0) {
        /*
         * SD Card Detect (Host 1); Low=Card Present
         *
         * Pin GPIO17 muxed to MCU GPIO Port Pin 17 at boot.
         */
        gpio_signal_config(GPIO_SIGNAL_SD1_DET_B, GPIO_GDIR_INPUT,
            GPIO_INT_FALL_EDGE);
        return gpio_signal_request_irq(GPIO_SIGNAL_SD1_DET_B,
                GPIO_HIGH_PRIO, handler, 0, "MXCMMC", host);
    }
    return -ENODEV;
}


/*!
 * Free the interrupt request for SD1_DET.
 *
 * @param  host Pointer to MMC/SD host structure.
 */
void sdhc_intr_destroy(int module, void *host)
{
    if(module == 0) {
        gpio_signal_free_irq(GPIO_SIGNAL_SD1_DET_B, GPIO_HIGH_PRIO, host);
    }
}


/*!
 * Clear the GPIO interrupt for SDHC1 SD1_DET.
 *
 * @param flag Flag represents whether the card is inserted/removed.
 *             Using this sensitive level of GPIO signal is changed.
 */
void sdhc_intr_clear(int module, int flag) 
{
    if(module == 0) {
        /* Actual interrupt is cleared by MXC interrupt handler. */
        if(flag) {
            gpio_signal_config(GPIO_SIGNAL_SD1_DET_B, GPIO_GDIR_INPUT,
                    GPIO_INT_FALL_EDGE);
        } else {
            gpio_signal_config(GPIO_SIGNAL_SD1_DET_B, GPIO_GDIR_INPUT,
                    GPIO_INT_RISE_EDGE);
        }
    }
}


/**
 * Find the minimum clock for SDHC.
 *
 * @param clk SDHC module number.
 * @return Returns the minimum SDHC clock.
 */
unsigned int sdhc_get_min_clock(enum mxc_clocks clk)
{
    return (mxc_get_clocks(SDHC1_CLK) / 8) / 32;
}


/**
 * Find the maximum clock for SDHC.
 * @param clk SDHC module number.
 * @return Returns the maximum SDHC clock.
 */
unsigned int sdhc_get_max_clock(enum mxc_clocks clk)
{
    return mxc_get_clocks(SDHC1_CLK) / 2;
}


/**
 * Probe for the card. Low=card present; high=card absent.
 */
int sdhc_find_card(int module)
{
    if(module == 0) {
        return gpio_signal_get_data_check(GPIO_SIGNAL_SD1_DET_B);
    } else {
        return -ENODEV;
    }
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_SDHC */
