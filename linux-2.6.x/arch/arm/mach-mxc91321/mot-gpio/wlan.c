/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/wlan.c
 *
 * ArgonLV implementation of Motorola GPIO API for WiFi support.
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
 * 03-Nov-2006  Motorola        Initial revision.
 * 09-Jul-2007  Motorola        Added gpio_free_irq_dev work around.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API_WLAN)
/**
 * Set the state of the WLAN_RESET signal.
 *
 * @param   data    Desired state of the signal.
 */
void gpio_wlan_reset_set_data(__u32 data)
{
    gpio_signal_set_data(GPIO_SIGNAL_WLAN_RESET, data);
}


/**
 * Set the state of the WLAN_CLIENT_WAKE_B signal.
 *
 * @param   data    Desired state of the signal.
 */
void gpio_wlan_clientwake_set_data(__u32 data)
{
    gpio_signal_set_data(GPIO_SIGNAL_WLAN_CLIENT_WAKE_B, data);
}


/**
 * Get the current status of the WLAN_PWR_DWN_B signal.
 *
 * @return  Zero if WLAN_PWR_DWN_B is low; non-zero if it is high.
 */
__u32 gpio_wlan_powerdown_get_data(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_WLAN_PWR_DWN_B);
}


/**
 * Set the current status of the WLAN_PWR_DWN_B signal.
 *
 * @return  Zero if WLAN_PWR_DWN_B is low; non-zero if it is high.
 */
void gpio_wlan_powerdown_set_data(__u32 data)
{
    gpio_signal_set_data(GPIO_SIGNAL_WLAN_PWR_DWN_B, data);
}


/**
 * Install handler for WLAN_HOST_WAKE_B interrupt.
 *
 * @param   handler     Function to be called when interrupt arives.
 * @param   irq_flags   Flags to pass to request_irq.
 * @param   devname     Name of device driver.
 * @param   dev_id      Device identifier to pass to request_irq.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_wlan_hostwake_request_irq(gpio_irq_handler handler,
        unsigned long irq_flags, const char *devname, void *dev_id)
{
    int retval;
    
    retval = gpio_signal_request_irq(GPIO_SIGNAL_WLAN_HOST_WAKE_B,
            GPIO_HIGH_PRIO, handler, irq_flags, devname, dev_id);

    /* if request_irq was successful, configure the signal for interrupts  */
    if(retval == 0) {
        /* the clear_int function sets the proper trigger edge */
        gpio_wlan_hostwake_clear_int();
    }

    return retval;
}


/**
 * Remove handler for WLAN_HOST_WAKE_B interrupt.
 *
 * @param   dev_id      Device identifier to pass to free_irq.
 */
void gpio_wlan_hostwake_free_irq(void *dev_id)
{
    gpio_signal_config(GPIO_SIGNAL_WLAN_HOST_WAKE_B, GPIO_GDIR_INPUT,
            GPIO_INT_NONE);
    gpio_signal_free_irq(GPIO_SIGNAL_WLAN_HOST_WAKE_B, GPIO_HIGH_PRIO, dev_id);
}


/**
 * Clear the WLAN_HOST_WAKE_B interrupt.
 */
void gpio_wlan_hostwake_clear_int(void)
{
    gpio_signal_clear_int(GPIO_SIGNAL_WLAN_HOST_WAKE_B);
    
    /* Adjust the edge trigger for the interrupt. */
    gpio_signal_config(GPIO_SIGNAL_WLAN_HOST_WAKE_B, GPIO_GDIR_INPUT,
            gpio_wlan_hostwake_get_data() ? GPIO_INT_FALL_EDGE
            : GPIO_INT_RISE_EDGE);
}


/**
 * Get the current status of WLAN_HOST_WAKE_B.
 *
 * @return  Zero if signal is low; non-zero if signal is high.
 */
__u32 gpio_wlan_hostwake_get_data(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_WLAN_HOST_WAKE_B);
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_WLAN */
