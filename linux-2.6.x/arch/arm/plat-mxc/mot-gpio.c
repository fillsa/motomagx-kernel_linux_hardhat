/*
 * mot-gpio.c - Motorola-specific GPIO API
 *
 * Copyright (C) 2006-2008 Motorola, Inc.
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
 * 04-Oct-2006  Motorola        Initial revision.
 * 30-Nov-2006  Motorola        remove exported gpio_sensor_active
 *                              and gpio_sensor_inactive interface
 * 06-Dec-2006  Motorola        Added etm_enable_trigger_clock export.
 * 02-Jan-2007  Motorola        Update high speed USB API.
 * 23-Jan-2007  Motorola        Added Bluetooth bt_power control API.
 * 23-Feb-2007  Motorola        Export DAI functions.
 * 13-Aug-2008  Motorola        GP_AP_C8 toggle workaround for 300uA BT power issue
 */

#include <linux/module.h>
#include <linux/config.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API)

#include <asm/arch/gpio.h>
#include <asm/mot-gpio.h>

/**
 * Configure a GPIO signal based on information from the GPIO settings
 * array.
 *
 * @param   index   Index into the init_gpio_settings array.
 * @param   out     Non-zero of GPIO signal should be configured for output.
 * @param   icr     Setting for GPIO signal interrupt control.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_config(enum gpio_signal index, bool out,
        enum gpio_int_cfg icr)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
        gpio_config(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no, out, icr);
        return 0;
    }

    gpio_tracemsg("Attempt to configure invalid GPIO signal: %d", index);
    return -ENODEV;
}


/**
 * Setup an interrupt handler for a GPIO signal based on information from
 * the GPIO settings array.
 *
 * @param   index       Index into the initial GPIO settings array.
 * @param   prio        Interrupt priority.
 * @param   handler     Handler for the interrupt.
 * @param   irq_flags   Interrupt flags.
 * @param   devname     Name of the device driver.
 * @param   dev_id      Unique ID for the device driver.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_request_irq(enum gpio_signal index,
        enum gpio_prio prio, gpio_irq_handler handler, __u32 irq_flags,
        const char *devname, void *dev_id)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
        return gpio_request_irq(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no, prio, handler, irq_flags,
                devname, dev_id);
    }

    gpio_tracemsg("Attempt to request interrupt for invalid GPIO signal: %d",
            index);
    return -ENODEV;
}


/**
 * Remove an interrupt handler for a GPIO signal based on information from
 * the GPIO settings array.
 *
 * @param   index       Index into the initial GPIO settings array.
 * @param   prio        Interrupt priority.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_free_irq(enum gpio_signal index,
        enum gpio_prio prio)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
        gpio_free_irq(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no, prio);
        return 0;
    }

    gpio_tracemsg(
            "Attempt to free interrupt request for invalid GPIO signal: %d",
            index);
    return -ENODEV;
}


/**
 * Clear a GPIO interrupt.
 *
 * @param   index       Index into the initial GPIO settings array.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_clear_int(enum gpio_signal index)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
        /* obsolete in in MVL 05/12
        gpio_clear_int(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no);
                */
        return 0;
    }

    gpio_tracemsg("Attempt to clear interrupt for invalid GPIO signal: %d",
            index);
    return -ENODEV;
}


/**
 * Set a GPIO signal's data register based on information from the initial
 * GPIO settings array.
 *
 * @param   index   Index into the initial GPIO settings array.
 * @param   data    New value for the data register.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_set_data(enum gpio_signal index, __u32 data)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
        gpio_set_data(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no,
                data);
        return 0;
    }
    
    gpio_tracemsg("Attempt to set data for invalid signal: %d", index);
    return -ENODEV;
}


/**
 * Get a GPIO signal's state based on information from the initial
 * GPIO settings array.
 *
 * @param   index   [in] Index into the initial GPIO settings array.
 * @param   data    [out] Value of the data bit for the GPIO signal.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_get_data(enum gpio_signal index, __u32 *data)
{
    if(data==NULL) {
        return -EFAULT;
    }

    if(GPIO_SIGNAL_IS_VALID(index)) {
        *data = gpio_get_data(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no);
        return 0;
    }

    gpio_tracemsg("Attempt to read data for invalid signal: %d", index);
    return -ENODEV;
}

/**
 * This is like gpio_signal_get_data but will print an error message
 * if index isn't a valid signal for the particular hardware
 * configuration (printk is called with the KERN_ERR message level). 
 * The caller will not be notified of the failure, so the return value
 * is potentially invalid.
 *
 * Only call this function if you know what you are doing.
 *
 * @param   index   [in] Index into the initial GPIO settings array.
 *
 * @return  Value of the data bit for the GPIO signal.
 */
__u32 gpio_signal_get_data_check(enum gpio_signal index)
{
    __u32 data;
    int ret;

    if((ret = gpio_signal_get_data(index, &data)) != 0) {
        printk(KERN_ERR "Encountered error %d reading GPIO signal %d!\n",
                ret, index);
    }

    return data;
}


#if defined(CONFIG_MOT_FEAT_GPIO_API_EDIO)
/**
 * Protect edio_set_data.
 */
static DEFINE_RAW_SPINLOCK(edio_lock);

/**
 * This function sets a EDIO signal value.
 *
 * @param  sig_no       EDIO signal (0 based) as defined in \b #edio_sig_no
 * @param  data         value to be set (only 0 or 1 is valid)
 */
void edio_set_data(enum edio_sig_no sig_no, __u32 data)
{
    unsigned long edio_flags;
    unsigned short rval;
    
    MXC_ERR_CHK((data > 1) || (sig_no > EDIO_MAX_SIG_NO));
    
    spin_lock_irqsave(&edio_lock, edio_flags);

    /* EDIO_EPDR is EDIO Edge Port Data Register */
    rval = __raw_readw(EDIO_EPDR);

    /* clear the bit */
    rval &= ~(1 << sig_no); 

    /* if data is set to a non-zero value, set the appropriate bit */
    if(data) {
        rval |= (1 << sig_no);
    }
    
    __raw_writew(rval, EDIO_EPDR);

    spin_unlock_irqrestore(&edio_lock, edio_flags);
}


/**
 * This function returns the value of the EDIO signal.
 *
 * @param  sig_no       EDIO signal (0 based) as defined in \b #edio_sig_no
 *
 * @return Value of the EDIO signal (either 0 or 1)
 */
__u32 edio_get_data(enum edio_sig_no sig_no)
{
    MXC_ERR_CHK(sig_no > EDIO_MAX_SIG_NO);
    return (__raw_readw(EDIO_EPDR) >> sig_no) & 1;
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_EDIO */


/*
 * Export initial_gpio_settings array for GPIODev.
 */
EXPORT_SYMBOL(initial_gpio_settings);


/*
 * Export common GPIO API functions.
 */
EXPORT_SYMBOL(gpio_signal_config);
EXPORT_SYMBOL(gpio_signal_request_irq);
EXPORT_SYMBOL(gpio_signal_free_irq);
EXPORT_SYMBOL(gpio_signal_clear_int);
EXPORT_SYMBOL(gpio_signal_set_data);
EXPORT_SYMBOL(gpio_signal_get_data);
EXPORT_SYMBOL(gpio_signal_get_data_check);


/*
 * Export EDIO API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_EDIO)
EXPORT_SYMBOL(edio_set_data);
EXPORT_SYMBOL(edio_get_data);
#endif


/*
 * Export Digital Audio Interface functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_DAI)
EXPORT_SYMBOL(gpio_dai_enable);
EXPORT_SYMBOL(gpio_dai_disable);
#endif

/*
 * Export SDHC API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_SDHC)
EXPORT_SYMBOL(gpio_sdhc_active);
EXPORT_SYMBOL(gpio_sdhc_inactive);
EXPORT_SYMBOL(sdhc_intr_setup);
EXPORT_SYMBOL(sdhc_intr_destroy);
EXPORT_SYMBOL(sdhc_intr_clear);
EXPORT_SYMBOL(sdhc_get_min_clock);
EXPORT_SYMBOL(sdhc_get_max_clock);
EXPORT_SYMBOL(sdhc_find_card);
#endif /* CONFIG_MOT_FEAT_GPIO_API_SDHC */


/*
 * Export USB HS GPIO API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_USBHS)
EXPORT_SYMBOL(gpio_usb_hs_reset_set_data);
EXPORT_SYMBOL(gpio_usb_hs_wakeup_set_data);
EXPORT_SYMBOL(gpio_usb_hs_switch_set_data);
EXPORT_SYMBOL(gpio_usb_hs_flagc_request_irq);
EXPORT_SYMBOL(gpio_usb_hs_flagc_free_irq);
EXPORT_SYMBOL(gpio_usb_hs_flagc_clear_int);
EXPORT_SYMBOL(gpio_usb_hs_flagc_get_data);
EXPORT_SYMBOL(gpio_usb_hs_int_request_irq);
EXPORT_SYMBOL(gpio_usb_hs_int_free_irq);
EXPORT_SYMBOL(gpio_usb_hs_int_clear_int);
EXPORT_SYMBOL(gpio_usb_hs_int_get_data);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_config_gpio_mode);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_config_sdma_mode);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_config_dual_mode);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_request_irq);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_set_irq_type);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_free_irq);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_clear_int);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_get_data);
#endif /* CONFIG_MOT_FEAT_GPIO_API_USBHS */


/*
 * Export Bluetooth power management API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_BTPOWER)
EXPORT_SYMBOL(gpio_bluetooth_hostwake_request_irq);
EXPORT_SYMBOL(gpio_bluetooth_hostwake_free_irq);
EXPORT_SYMBOL(gpio_bluetooth_hostwake_clear_int);
EXPORT_SYMBOL(gpio_bluetooth_hostwake_get_data);
EXPORT_SYMBOL(gpio_bluetooth_wake_set_data);
EXPORT_SYMBOL(gpio_bluetooth_wake_get_data);
EXPORT_SYMBOL(gpio_bluetooth_power_set_data);
EXPORT_SYMBOL(gpio_bluetooth_power_get_data);
#if defined(CONFIG_MACH_NEVIS)
EXPORT_SYMBOL(gpio_bluetooth_power_fixup);
#endif /* CONFIG_MACH_NEVIS */
#endif /* CONFIG_MOT_FEAT_GPIO_API_BTPOWER */


/*
 * Export WLAN power management API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_WLAN)
EXPORT_SYMBOL(gpio_wlan_reset_set_data);
EXPORT_SYMBOL(gpio_wlan_clientwake_set_data);
EXPORT_SYMBOL(gpio_wlan_powerdown_get_data);
EXPORT_SYMBOL(gpio_wlan_powerdown_set_data);
EXPORT_SYMBOL(gpio_wlan_hostwake_request_irq);
EXPORT_SYMBOL(gpio_wlan_hostwake_free_irq);
EXPORT_SYMBOL(gpio_wlan_hostwake_clear_int);
EXPORT_SYMBOL(gpio_wlan_hostwake_get_data);
#endif /* CONFIG_MOT_FEAT_GPIO_API_WLAN */

/*
 * Export ETM-related functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_ETM)
EXPORT_SYMBOL(etm_iomux_config);
EXPORT_SYMBOL(etm_enable_trigger_clock);
#endif /* CONFIG_MOT_FEAT_GPIO_API_ETM */

#endif /* CONFIG_MOT_FEAT_GPIO_API */

