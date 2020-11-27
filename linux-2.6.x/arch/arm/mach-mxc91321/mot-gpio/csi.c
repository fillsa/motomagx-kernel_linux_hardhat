/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/csi.c
 *
 * ArgonLV implementation of Camera Sensor Interface GPIO control.
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

#include <linux/config.h>
#include <linux/module.h>

#if defined(CONFIG_MOT_FEAT_BRDREV)
#include <asm/boardrev.h>
#endif

#include "mot-gpio-argonlv.h"

#if defined(CONFIG_MOT_FEAT_GPIO_API_CSI)
/*!
 * Setup GPIO for Camera sensor to be active
 */
void gpio_sensor_active(void)
{
    /*
     * Camera Reset, Active Low
     *
     * Pin GPIO23 muxed to MCU GPIO Port Pin 18 at boot.
     */
    gpio_signal_set_data(GPIO_SIGNAL_CAM_RST_B, GPIO_DATA_HIGH);

#ifdef CONFIG_MXC_IPU_CAMERA_MI2010SOC /* a.k.a. MT9D111 */
    /*
     * Camera Power Down (2MP Imager), Active High
     *
     * Pin GPIO15 muxed to MCU GPIO Port Pin 15 at boot.
     */
    gpio_signal_set_data(GPIO_SIGNAL_CAM_EXT_PWRDN, GPIO_DATA_LOW);
#endif /* CONFIG_MXC_IPU_CAMERA_MI2010SOC */

#if defined(CONFIG_MOT_FEAT_BRDREV) && defined(CONFIG_MACH_BUTEREF)
    if((boardrev() < BOARDREV_P4A) || (boardrev() == BOARDREV_UNKNOWN)) {
#endif /* CONFIG_MOT_FEAT_BRDREV && CONFIG_MACH_BUTEREF */

#if defined(CONFIG_MXC_IPU_CAMERA_MT9M111) \
    || defined(CONFIG_MXC_IPU_CAMERA_OV2640) \
    || defined(CONFIG_MXC_IPU_CAMERA_MI2020SOC)
        /*
         * Camera Power Down (1.3MP Imager), Active High
         *
         * Pin GPIO16 muxed to MCU GPIO Port Pin 16 at boot.
         */
        gpio_signal_set_data(GPIO_SIGNAL_CAM_INT_PWRDN, GPIO_DATA_LOW);
#endif

#if defined(CONFIG_MOT_FEAT_BRDREV) && defined(CONFIG_MACH_BUTEREF)
    }
#endif /* CONFIG_MOT_FEAT_BRDREV && CONFIG_MACH_BUTEREF */
}


/*!
 * Setup GPIO for camera sensor to be inactive
 */
void gpio_sensor_inactive(void)
{
    /* GPIO_CAM_RST_B -- GPIO_Shared_3 -- Camera Reset, Active Low */
    gpio_signal_set_data(GPIO_SIGNAL_CAM_RST_B, GPIO_DATA_LOW);

#ifdef CONFIG_MXC_IPU_CAMERA_MI2010SOC /* a.k.a. MT9D111 */
    /* CSI_CS0 -- Camera Power Down (2MP Imager), Active High */
    gpio_signal_set_data(GPIO_SIGNAL_CAM_EXT_PWRDN, GPIO_DATA_HIGH);
#endif /* CONFIG_MXC_IPU_CAMERA_MI2010SOC */

#if defined(CONFIG_MOT_FEAT_BRDREV) && defined(CONFIG_MACH_BUTEREF)
    if((boardrev() < BOARDREV_P4A) || (boardrev() == BOARDREV_UNKNOWN)) {
#endif /* CONFIG_MOT_FEAT_BRDREV && CONFIG_MACH_BUTEREF */

#if defined(CONFIG_MXC_IPU_CAMERA_MT9M111) \
    || defined(CONFIG_MXC_IPU_CAMERA_OV2640) \
    || defined(CONFIG_MXC_IPU_CAMERA_MI2020SOC)
        /* CSI_CS1 -- Camera Power Down (1.3MP Imager), Active High */
        gpio_signal_set_data(GPIO_SIGNAL_CAM_INT_PWRDN, GPIO_DATA_HIGH);
#endif

#if defined(CONFIG_MOT_FEAT_BRDREV) && defined(CONFIG_MACH_BUTEREF)
    }
#endif /* CONFIG_MOT_FEAT_BRDREV && CONFIG_MACH_BUTEREF */
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_CSI */
