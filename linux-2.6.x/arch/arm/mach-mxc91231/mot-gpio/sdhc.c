/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/sdhc.c
 *
 * SCM-A11 implementation of Motorola GPIO API for SD Host Controller support.
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
 * Date          Author      Comments
 * ===========   =========   ====================================================
 * 01-Jan-2006  Motorola        Initial version.
 * 27-Feb-2007  Motorola        Initial revision.
 * 28-Jun-2007  Motorola        Removed MARCO specific signal code in relation to xPIXL.
 * 20-Mar-2008   Motorola    Remove code not related to signals in Nevis produst
 * 23-June-2008  Motorola    Add code to check GPIO_SIGNAL_GND to detect Embedded TFlash of Nevis 
 * 03-Jul-2008  Motorola        OSS CV fix.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <asm/mot-gpio.h>
#include <asm/arch/clock.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API_SDHC)

#define NUM_SDHC_MODULE     2

static const enum gpio_signal sdhc_module_gpio_signal[NUM_SDHC_MODULE] = {
    GPIO_SIGNAL_SD1_DAT3,
    GPIO_SIGNAL_SD2_DAT3
};

static const enum iomux_pins sdhc_module_dat3_pin[NUM_SDHC_MODULE] = {
    SP_SD1_DAT3,
    SP_SD2_DAT3
};

/*!
 * Setup GPIO for SDHC to be active
 *
 * @param module SDHC module number
 */
void gpio_sdhc_active(int module) 
{
    /* IOMUX settings for SDHC pins are configured at boot. */

    switch(module) {
        case 0:
            break;

        case 1:
// ни на что не влияет, но у LJ7.1+ свой механизм определения GPIO_API_WLAN 
#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
            if(GPIO_SIGNAL_IS_VALID(GPIO_SIGNAL_WLAN_RESET))
#else
#if defined(CONFIG_MOT_FEAT_GPIO_API_WLAN)
#endif
            {
                /* SDHC slot 2 is the WLAN controller on scma11ref and Marco */
                gpio_signal_set_data(GPIO_SIGNAL_WLAN_PWR_DWN_B, GPIO_DATA_HIGH);
                gpio_signal_set_data(GPIO_SIGNAL_WLAN_RESET, GPIO_DATA_HIGH);
            }
#if ! defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
#endif /* CONFIG_MOT_FEAT_GPIO_API_WLAN */
#endif
            break;
    
        default:
            break;
    }
}


/*!
 * Setup GPIO for SDHC1 to be inactive
 *
 * @param module SDHC module number
 */
void gpio_sdhc_inactive(int module) 
{
    /* IOMUX settings for SDHC pins are configured at boot. */

    switch(module) {
        case 0:
            break;

        case 1:
// ни на что не влияет, но у LJ7.1+ свой механизм определения GPIO_API_WLAN 
#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
            if(GPIO_SIGNAL_IS_VALID(GPIO_SIGNAL_WLAN_RESET))
#else
#if defined(CONFIG_MOT_FEAT_GPIO_API_WLAN)
#endif
            {
                /* SDHC slot 2 is the WLAN controller on scma11ref and Marco */
                gpio_signal_set_data(GPIO_SIGNAL_WLAN_RESET, GPIO_DATA_LOW);
                gpio_signal_set_data(GPIO_SIGNAL_WLAN_PWR_DWN_B, GPIO_DATA_LOW);
            }
#if ! defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
#endif /* CONFIG_MOT_FEAT_GPIO_API_WLAN */
#endif

            break;
    
        default:
            break;
    }
}


/**
 * Probe for the card.
 *
 * @param   module  SDHC controller to check for card.
 *
 * @return  Zero if card is not present. Greater than zero if card is present.
 *          Less than zero if there is an error reading the card status.
 */
int sdhc_find_card(int module)
{
    int data = -ENODEV;

#if defined(CONFIG_MOT_FEAT_SD1_TF_DET)
    if(module == 0) {
#if ! defined(CONFIG_MACH_NEVIS) // || ! defined(CONFIG_MACH_XPIXL)
        /* TF_DET is low if card is present */
        data = gpio_signal_get_data_check(GPIO_SIGNAL_TF_DET);
#else
	/*GND is low if card is present*/
	data = gpio_signal_get_data_check(GPIO_SIGNAL_GND);
#endif
    } else {
#endif
        if(module < NUM_SDHC_MODULE) {
        /* Remarked this section code waiting for DAT3 detection OK */
        #if 0	
            /* temporarily change IOMUX setting so DAT3 can be used as GPIO */
            iomux_config_mux(sdhc_module_dat3_pin[module], OUTPUTCONFIG_DEFAULT,
                    INPUTCONFIG_DEFAULT);
            udelay(125);

            /* read the DAT3 line */
            data = (
                    gpio_signal_get_data_check(sdhc_module_gpio_signal[module]) 
                    == GPIO_DATA_HIGH ? 1 : 0 );

            /* change IOMUX back */
            iomux_config_mux(sdhc_module_dat3_pin[module], OUTPUTCONFIG_FUNC1,
                    INPUTCONFIG_FUNC1);
         #else
             data = 0;       
         #endif
        }
#if defined(CONFIG_MOT_FEAT_SD1_TF_DET)
    }
#endif

    return data;
}


/*!
 * Setup the IOMUX/GPIO for SDHC1 SD1_DET.
 * 
 * @param   module      SDHC controller to probe.
 * @param   host        Pointer to MMC/SD host structure.
 * @param   handler     GPIO ISR function pointer for the GPIO signal.
 *
 * @return  Zero if interrupt registers successfully; non-zero otherwise.
 **/
int sdhc_intr_setup(int module, void *host, gpio_irq_handler handler)
{
    int ret = -ENODEV;

#if defined(CONFIG_MOT_FEAT_SD1_TF_DET)
    if(module == 0) {
        set_irq_type(INT_EXT_INT4, IRQT_BOTHEDGE);
        ret = request_irq(INT_EXT_INT4, handler,
                SA_INTERRUPT | SA_SAMPLE_RANDOM, "mmc card detect",
                (void *)host);
        
        if (ret) {
            printk(KERN_ERR
                    "MMC Unable to initialize card detect interrupt.\n");
        }
    }
#else
    /* 
     * For non SD1_TF_DET detection products, we use DAT3 as detecting pin.
     * Reserved it for DAT3 function.
     */
    ret = 0;
#endif
    
    return ret;
}


/*!
 * Clear the IOMUX/GPIO for SDHC1 SD1_DET.
 * 
 * @param flag Flag represents whether the card is inserted/removed.
 *             Using this sensitive level of GPIO signal is changed.
 *
 **/
void sdhc_intr_clear(int module, int flag)
{
    /* reserved interface */
}


/*!
* Clear the IOMUX/GPIO for SDHC1 SD1_DET.
*/
void sdhc_intr_destroy(int module, void *host)
{
#if defined(CONFIG_MOT_FEAT_SD1_TF_DET)
    if(module == 0) {
        free_irq(INT_EXT_INT4, host);
    }
#endif
}


/**
 * Find the minimum clock for SDHC.
 *
 * @param clk SDHC module number.
 * @return Returns the minimum SDHC clock.
 */
unsigned int sdhc_get_min_clock(enum mxc_clocks clk)
{
    return (mxc_get_clocks(clk) / 9) / 32 ;
}


/**
 * Find the maximum clock for SDHC.
 * @param clk SDHC module number.
 * @return Returns the maximum SDHC clock.
 */
unsigned int sdhc_get_max_clock(enum mxc_clocks clk)
{
    return mxc_get_clocks(clk) / 2;
}

#endif /*CONFIG_MOT_FEAT_GPIO_API_SDHC */
