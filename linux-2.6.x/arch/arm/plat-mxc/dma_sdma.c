/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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

/*
 * DATE          AUTHOR         COMMMENT
 * ----          ------         --------
 * 10/04/2006    Motorola       Add mxc_dma_reset support
 */

/*!
 * @file dma.c
 * @brief This file contains functions for Smart DMA  API
 *
 * SDMA (Smart DMA) is used for transferring data between MCU and peripherals
 *
 * @ingroup SDMA
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <asm/dma.h>
#include <asm/mach/dma.h>
#include <linux/interrupt.h>
#include <asm/arch/hardware.h>

#include <asm/semaphore.h>
#include <linux/spinlock.h>

#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#ifdef CONFIG_MXC_SDMA_API

static int bd_index[MAX_DMA_CHANNELS];

/*!
 * Request dma channel. Standard DMA API handler.
 *
 * @param   channel           channel number
 * @param   dma               dma structure
 */
static int dma_api_request(dmach_t channel, dma_t * dma)
{
	int res;

	if (channel == 0) {
		res = -EBUSY;
	} else {
		res = mxc_request_dma(&channel, "DMA_API");
		if (res == 0) {
			bd_index[channel] = 0;
		}
	}
	return res;
}

/*!
 * Free dma channel. Standard DMA API handler.
 *
 * @param   channel           channel number
 * @param   dma               dma structure
 */
static void dma_api_free(dmach_t channel, dma_t * dma)
{
	mxc_free_dma(channel);
}

/*!
 * Enable dma channel. Standard DMA API handler.
 *
 * @param   channel           channel number
 * @param   dma               dma structure
 */
static void dma_api_enable(dmach_t channel, dma_t * dma)
{
	dma_request_t r_params;

	mxc_dma_get_config(channel, &r_params, bd_index[channel]);
	r_params.count = dma->buf.length;

	if (dma->dma_mode == DMA_MODE_READ) {
		r_params.destAddr = dma->buf.__address;
	} else if (dma->dma_mode == DMA_MODE_WRITE) {
		r_params.sourceAddr = dma->buf.__address;
	}

	mxc_dma_set_config(channel, &r_params, bd_index[channel]);
	bd_index[channel] = (bd_index[channel] + 1) % MAX_BD_NUMBER;

	mxc_dma_start(channel);
}

/*!
 * Disable dma channel. Standard DMA API handler.
 *
 * @param   channel           channel number
 * @param   dma               dma structure
 */
static void dma_api_disable(dmach_t channel, dma_t * dma)
{
	mxc_dma_stop(channel);
}

/*!
 * This structure contains the pointers to the control functions that are
 * invoked by the standard dma api driver.
 * The structure is passed to dma.c file during initialization.
 */
struct dma_ops dma_operations = {
	.request = dma_api_request,
	.free = dma_api_free,
	.enable = dma_api_enable,
	.disable = dma_api_disable,
	.type = "sdma",
};

/*!
 * Initializes dma structure with dma_operations
 *
 * @param   dma           dma structure
 */
void __init arch_dma_init(dma_t * dma)
{
	int i;

	for (i = 0; i < MAX_DMA_CHANNELS; i++) {
		dma[i].d_ops = &dma_operations;
	}
}

module_init(sdma_init);

#else
int mxc_request_dma(int *channel, const char *devicename)
{
	return -ENODEV;
}

int mxc_dma_setup_channel(int channel, dma_channel_params * p)
{
	return -ENODEV;
}

int mxc_dma_set_config(int channel, dma_request_t * p, int bd_index)
{
	return -ENODEV;
}

int mxc_dma_get_config(int channel, dma_request_t * p, int bd_index)
{
	return -ENODEV;
}

int mxc_dma_start(int channel)
{
	return -ENODEV;
}

#ifdef CONFIG_MOT_WFN409
int mxc_dma_reset(int channel, int buffer_number)
{
        return -ENODEV;
}
#endif /* CONFIG_MOT_WFN409 */

int mxc_dma_stop(int channel)
{
	return -ENODEV;
}

void mxc_free_dma(int channel)
{
}

void mxc_dma_set_callback(int channel, dma_callback_t callback, void *arg)
{
}

void *sdma_malloc(size_t size)
{
	return 0;
}

void sdma_free(void *buf)
{
}

void *sdma_phys_to_virt(unsigned long buf)
{
	return 0;
}

unsigned long sdma_virt_to_phys(void *buf)
{
	return 0;
}

EXPORT_SYMBOL(mxc_request_dma);
EXPORT_SYMBOL(mxc_dma_setup_channel);
EXPORT_SYMBOL(mxc_dma_set_config);
EXPORT_SYMBOL(mxc_dma_get_config);
EXPORT_SYMBOL(mxc_dma_start);
#ifdef CONFIG_MOT_WFN409
EXPORT_SYMBOL(mxc_dma_reset);
#endif /* CONFIG_MOT_WFN409 */
EXPORT_SYMBOL(mxc_dma_stop);
EXPORT_SYMBOL(mxc_free_dma);
EXPORT_SYMBOL(mxc_dma_set_callback);
EXPORT_SYMBOL(sdma_malloc);
EXPORT_SYMBOL(sdma_free);
EXPORT_SYMBOL(sdma_phys_to_virt);
EXPORT_SYMBOL(sdma_virt_to_phys);

#endif

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC Linux SDMA API");
MODULE_LICENSE("GPL");
