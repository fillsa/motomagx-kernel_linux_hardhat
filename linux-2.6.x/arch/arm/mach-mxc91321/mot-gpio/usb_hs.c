/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/usb_hs.c
 *
 * ArgonLV implementation of Motorola GPIO API for high-speed USB support.
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
 * 03-Jan-2007  Motorola        Add dual_mode and set_irqt functions.
 * 03-Jan-2007  Motorola        Change HS interrupt setup procedure.
 * 09-Jul-2007  Motorola        Added gpio_free_irq_dev work around.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <asm/mot-gpio.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API_USBHS)
/**
 * Enable high speed USB controller.
 *
 * @param   enable  Set high to enable controller or zero to put controller
 *                  into reset.
 */
void gpio_usb_hs_reset_set_data(__u32 enable)
{
    gpio_signal_set_data(GPIO_SIGNAL_USB_HS_RESET,
            enable ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
}


/**
 * Wakeup high speed USB controller.
 *
 * @param   wakeup  Set high to take high speed USB controller out of sleep;
 *                  set to zero to put USB controller to sleep.
 */
void gpio_usb_hs_wakeup_set_data(__u32 wakeup)
{
    gpio_signal_set_data(GPIO_SIGNAL_USB_HS_WAKEUP,
            wakeup ? GPIO_DATA_HIGH : GPIO_DATA_LOW);
}


/**
 * High speed USB switch.
 *
 * @param   swtch   Set to zero for USB connected to Atlas.
 */
void gpio_usb_hs_switch_set_data(__u32 swtch)
{
    /* stub on ArgonLV */
}


/**
 * Install an interrupt handler for the FLAGC signal.
 *
 * @param   handler     Function to be called when interrupt arives.
 * @param   irq_flags   Flags to pass to request_irq.
 * @param   devname     Name of device driver.
 * @param   dev_id      Device identifier to pass to request_irq.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_usb_hs_flagc_request_irq(gpio_irq_handler handler,
        unsigned long irq_flags, const char *devname, void *dev_id)
{
    /* FLAGC not defined on ArgonLV */
    return -ENODEV;
}


/**
 * Remove the interrupt handler for the FLAGC signal.
 *
 * @param   dev_id      Device identifier to pass to request_irq.
 */
void gpio_usb_hs_flagc_free_irq(void *dev_id)
{
    /* stub on ArgonLV */
}


/**
 * Clear pending FLAGC interrupt in GPIO controller.
 */
void gpio_usb_hs_flagc_clear_int(void)
{
    /* stub on ArgonLV */
}


/**
 * Get value of USB HS FLAGC signal.
 *
 * @return  Status of USB_HS_FLAGC signal.
 */
__u32 gpio_usb_hs_flagc_get_data(void)
{
    return GPIO_DATA_INVALID;
}


/**
 * Configure high speed USB interrupt.
 *
 * @param   handler     Function to be called when interrupt arives.
 * @param   irq_flags   Flags to pass to request_irq.
 * @param   devname     Name of device driver.
 * @param   dev_id      Device identifier to pass to request_irq.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_usb_hs_int_request_irq(gpio_irq_handler handler,
        unsigned long irq_flags, const char *devname, void *dev_id)
{
    gpio_signal_config(GPIO_SIGNAL_USB_HS_INT, GPIO_GDIR_INPUT,
            GPIO_INT_LOW_LEV);
    return gpio_signal_request_irq(GPIO_SIGNAL_USB_HS_INT,
            GPIO_HIGH_PRIO, handler, irq_flags, devname, dev_id);
}


/**
 * Remove the high speed USB interrupt handler.
 *
 * @param   dev_id      Device identifier passed to request_irq.
 *
 * @return  Zero on success.
 */
void gpio_usb_hs_int_free_irq(void *dev_id)
{
    gpio_signal_config(GPIO_SIGNAL_USB_HS_INT, GPIO_GDIR_INPUT,
            GPIO_INT_NONE);
    gpio_signal_free_irq(GPIO_SIGNAL_USB_HS_INT, GPIO_HIGH_PRIO, dev_id);
}


/**
 * Clear the high speed USB interrupt signal.
 */
void gpio_usb_hs_int_clear_int(void)
{
    gpio_signal_clear_int(GPIO_SIGNAL_USB_HS_INT);
}


/**
 * Get that status of the USB HS interrupt signal.
 */
__u32 gpio_usb_hs_int_get_data(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_USB_HS_INT);
}


/**
 * Configure pin GP_AP_B22 (signal USB_HS_DMA_REQ) for GPIO mode.
 */
void gpio_usb_hs_dma_req_config_gpio_mode(void)
{
    /* not implemented on ArgonLV */
}


/**
 * Configure pin GP_AP_B22 (signal USB_HS_DMA_REQ) for DMAREQ0 mode.
 */
void gpio_usb_hs_dma_req_config_sdma_mode(void)
{
    /* not implemented on ArgonLV */
}


/**
 * Allow SDMA signal from HS USB controller to be received by both the
 * SDMA controller and the GPIO controller on the Argon side.
 */
void gpio_usb_hs_dma_req_config_dual_mode(void)
{
    /* not implemented on ArgonLV */
}


/**
 * Install handler for USB_HS_DMA_REQ interrupt interrupt when in GPIO
 * mode.
 *
 * @param   handler     Function to be called when interrupt arives.
 * @param   irq_flags   Flags to pass to request_irq.
 * @param   devname     Name of device driver.
 * @param   dev_id      Device identifier to pass to request_irq.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_usb_hs_dma_req_request_irq(gpio_irq_handler handler,
        unsigned long irq_flags, const char *devname, void *dev_id)
{
    return gpio_signal_request_irq(GPIO_SIGNAL_USB_HS_DMA_REQ,
            GPIO_HIGH_PRIO, handler, irq_flags, devname, dev_id);
}


/**
 * Adjust the interrupt trigger level for the USB_HS_DMA_REQ interrupt when
 * in GPIO mode.
 *
 * @param   edge    Trigger level for interrupt.
 */
void gpio_usb_hs_dma_req_set_irq_type(gpio_edge_t edge)
{
    gpio_signal_config(GPIO_SIGNAL_USB_HS_DMA_REQ, GPIO_GDIR_INPUT,
            edge);
}


/**
 * Remove interrupt handler for USB_HS_DMA_REQ.
 *
 * @param   dev_id      Device identifier to pass to request_irq.
 */
void gpio_usb_hs_dma_req_free_irq(void *dev_id)
{
    gpio_signal_free_irq(GPIO_SIGNAL_USB_HS_DMA_REQ, GPIO_HIGH_PRIO, dev_id);
}


/**
 * Clear pending GPIO interrupt for USB_HS_DMA_REQ.
 */
void gpio_usb_hs_dma_req_clear_int(void)
{
    gpio_signal_clear_int(GPIO_SIGNAL_USB_HS_DMA_REQ);
}


/**
 * Get the current status of the USB_HS_DMA_REQ signal.
 */
__u32 gpio_usb_hs_dma_req_get_data(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_USB_HS_DMA_REQ);
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_USBHS */
