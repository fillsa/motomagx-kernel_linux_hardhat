/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/btpower.c
 *
 * SCM-A11 implementation of Motorola GPIO API for Bluetooth Power Management.
 *
 * Copyright (C) 2007-2008 Motorola, Inc.
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
 * 26-Jan-2007  Motorola        Initial revision.
 * 13-Aug-2008  Motorola	GP_AP_C8(BT_RESET_B) toggle workaround for 300uA BT power issue
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API_BTPOWER)

//#if defined(CONFIG_MACH_NEVIS)
#include <linux/delay.h>

#define BT_POWER_TOGGLE_DELAY 200 /* jiffies * 10ms */
#define BT_POWER_TOGGLE_HIGH 1 /* ms */

struct timer_list bt_power_timer;

/**
 * Add a timer to toggle GP_AP_C8 to remove 300uA extra current drain
 */
void bt_power_timer_fn(unsigned long high)
{
	gpio_signal_set_data(GPIO_SIGNAL_BT_POWER, GPIO_DATA_HIGH);
	mdelay(high);
	gpio_signal_set_data(GPIO_SIGNAL_BT_POWER, GPIO_DATA_LOW);
}

void gpio_bluetooth_power_fixup(void)
{
	init_timer(&bt_power_timer);
	bt_power_timer.data = BT_POWER_TOGGLE_HIGH;
	bt_power_timer.function = bt_power_timer_fn;
	bt_power_timer.expires = jiffies + BT_POWER_TOGGLE_DELAY;
	add_timer(&bt_power_timer);
}
//#endif /* CONFIG_MACH_NEVIS */

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
    set_irq_type(INT_EXT_INT1, IRQT_BOTHEDGE);
    return request_irq(INT_EXT_INT1, handler, irq_flags, devname, dev_id);    
}


/**
 * Remove handler for BT_HOST_WAKE_B interrupt.
 *
 * @param   dev_id      Device identifier to pass to free_irq.
 */
void gpio_bluetooth_hostwake_free_irq(void *dev_id)
{
    free_irq(INT_EXT_INT1, dev_id);
}


/**
 * Clear the BT_HOST_WAKE_B interrupt.
 */
void gpio_bluetooth_hostwake_clear_int(void)
{
    /* NO OP */
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
 * @param   data    Desired state for the signal.
 */
void gpio_bluetooth_power_set_data(__u32 data)
{
    if(data == 1) {
        /* enable bluetooth */
        iomux_config_mux(AP_U1_TXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE);
        iomux_config_mux(AP_U1_CTS_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE);
    }

    gpio_signal_set_data(GPIO_SIGNAL_BT_POWER, data);

    if(data == 0) {
        /* disable bluetooth */
        iomux_config_mux(AP_U1_TXD, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
        iomux_config_mux(AP_U1_CTS_B, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
    }
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
