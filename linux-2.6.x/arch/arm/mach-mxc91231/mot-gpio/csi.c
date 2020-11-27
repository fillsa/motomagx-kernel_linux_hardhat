/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/csi.c
 *
 * SCM-A11 implementation of Motorola GPIO API for camera sensor support.
 *
 * Copyright 2006 Motorola, Inc. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API_CSI)
/*!
 * Setup GPIO for sensor to be active
 */
void gpio_sensor_active(void)
{
    /* camera is taken out of reset at boot, but powered down */
    gpio_signal_set_data(GPIO_SIGNAL_CAM_PD, GPIO_DATA_LOW);
}


/**
 * Power down the camera.
 */
void gpio_sensor_inactive(void)
{
    gpio_signal_set_data(GPIO_SIGNAL_CAM_PD, GPIO_DATA_HIGH);
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_CSI */

