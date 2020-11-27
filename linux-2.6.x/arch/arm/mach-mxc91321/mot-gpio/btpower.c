/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/btpower.c
 *
 * ArgonLV implementation of Bluetooth power control API.
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
 * 31-Jan-2007  Motorola        Add API to control BT_POWER.
 */

#include "mot-gpio-argonlv.h"

#if defined(CONFIG_MOT_FEAT_GPIO_API_BTPOWER)
/**
 * Install handler for BT_HOST_WAKE_B interrupt.
 *
 * @param   handler     Function to be called when interrupt arives.
 * @param   irq_flags   Flags to pass to request_irq.
 * @param   devname     Name of device driver.
 * @param   dev_id      Device identifier to pass to request_irq.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_bluetooth_hostwake_request_irq(gpio_irq_handler handler,
        unsigned long irq_flags, const char *devname, void *dev_id)
{
    int retval;

    retval = gpio_signal_request_irq(GPIO_SIGNAL_BT_HOST_WAKE_B,
            GPIO_HIGH_PRIO, handler, irq_flags, devname, dev_id);

    /* if request_irq was successful, configure the signal for interrupts  */
    if(retval == 0) {
        /* the clear_int function sets the proper trigger edge */
        gpio_bluetooth_hostwake_clear_int();
    }

    return retval;
}


/**
 * Remove handler for BT_HOST_WAKE_B interrupt.
 *
 * @param   dev_id      Device identifier to pass to free_irq.
 */
void gpio_bluetooth_hostwake_free_irq(void *dev_id)
{
    gpio_signal_config(GPIO_SIGNAL_BT_HOST_WAKE_B, GPIO_GDIR_INPUT,
            GPIO_INT_NONE);
    gpio_signal_free_irq(GPIO_SIGNAL_BT_HOST_WAKE_B, GPIO_HIGH_PRIO);
}


/**
 * Clear the BT_HOST_WAKE_B interrupt.
 *
 * ArgonLV GPIO doesn't support both edge triggered interrupts. So this
 * function adjusts the edge trigger of the interrupt based on its current
 * state.
 */
void gpio_bluetooth_hostwake_clear_int(void)
{
    gpio_signal_clear_int(GPIO_SIGNAL_BT_HOST_WAKE_B);
    
    /* Adjust the edge trigger for the interrupt. */
    gpio_signal_config(GPIO_SIGNAL_BT_HOST_WAKE_B, GPIO_GDIR_INPUT,
            gpio_bluetooth_hostwake_get_data() ? GPIO_INT_FALL_EDGE
            : GPIO_INT_RISE_EDGE);
}


/**
 * Get the current status of BT_HOST_WAKE_B.
 *
 * @return  Zero if signal is low; non-zero if signal is high.
 */
__u32 gpio_bluetooth_hostwake_get_data(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_BT_HOST_WAKE_B);
}


/**
 * Set the state of the BT_WAKE_B signal.
 *
 * @param   data    Desired state for the signal.
 */
void gpio_bluetooth_wake_set_data(__u32 data)
{
    gpio_signal_set_data(GPIO_SIGNAL_BT_WAKE_B, data);
}


/**
 * Get the current status of the USB_HS_DMA_REQ signal.
 */
__u32 gpio_bluetooth_wake_get_data(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_BT_WAKE_B);
}


/**
 * Set the current state of the BT_POWER signal. On SCM-A11 products this
 * also changes the IOMUX settings for the U1_TXT and U1_CTS_B pins. This
 * is done to reduce current drain when Bluetooth is inactive.
 *
 * On Argon, this is just a wrapper around gpio_signal_set_data.
 *
 * @param   data    Desired state for the signal.
 */
void gpio_bluetooth_power_set_data(__u32 data)
{
    gpio_signal_set_data(GPIO_SIGNAL_BT_POWER, data);
}


/**
 * Return the current state of the BT_POWER signal.
 *
 * @return  Zero if signal is low; non-zero if signal is high.
 */ 
__u32 gpio_bluetooth_power_get_data(void)
{
    return ( gpio_signal_get_data_check(GPIO_SIGNAL_BT_POWER) == 0 ? 0 : 1 );
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_BTPOWER */
