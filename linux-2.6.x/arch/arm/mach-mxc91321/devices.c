/*
 * Author: MontaVista Software, Inc.
 *       <source@mvista.com>
 *
 * Based on the OMAP devices.c
 *
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2007 Motorola, Inc.
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  --------------
 * 10/06/2006   Motorola  Fix IPC issues
 * 03/06/2007   Motorola  Added FSL IPCv2 changes for WFN483
 * 06/25/2007   Motorola  Removed 2 scripts no longer used
 * 10/24/2007   Motorola  Integrated changes related to FSL SS13 patch
 *
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>

#include <asm/hardware.h>

#include <asm/arch/spba.h>
#include <asm/arch/sdma.h>

#ifdef CONFIG_ARCH_MXC91321
#include "sdma_script_code_pass2.h"
#else
#include "sdma_script_code.h"
#endif

#if 0
int board_device_enable(u32 device_id);
int board_device_disable(u32 device_id);

int mxc_device_enable(u32 device_id)
{
	int ret = 0;

	switch (device_id) {
	default:
		ret = board_device_enable(device_id);
	}

	return ret;
}

int mxc_device_disable(u32 device_id)
{
	int ret = 0;

	switch (device_id) {
	default:
		ret = board_device_disable(device_id);
	}
	return ret;
}

EXPORT_SYMBOL(mxc_device_enable);
EXPORT_SYMBOL(mxc_device_disable);
#endif

void mxc_sdma_get_script_info(sdma_script_start_addrs * sdma_script_addr)
{
        sdma_script_addr->mxc_sdma_app_2_mcu_addr = app_2_mcu_patched_ADDR;
	sdma_script_addr->mxc_sdma_ap_2_ap_addr = ap_2_ap_ADDR;
#ifdef CONFIG_MOT_WFN444
	sdma_script_addr->mxc_sdma_ap_2_bp_addr = ap_2_bp_patched_ADDR;
	sdma_script_addr->mxc_sdma_bp_2_ap_addr = bp_2_ap_patched_ADDR;
#else
	sdma_script_addr->mxc_sdma_ap_2_bp_addr = ap_2_bp_ADDR;
	sdma_script_addr->mxc_sdma_bp_2_ap_addr = bp_2_ap_ADDR;
#endif
        sdma_script_addr->mxc_sdma_mcu_2_app_addr = mcu_2_app_patched_ADDR;
        sdma_script_addr->mxc_sdma_mcu_2_shp_addr = mcu_2_shp_patched_ADDR;
        sdma_script_addr->mxc_sdma_shp_2_mcu_addr = shp_2_mcu_patched_ADDR;
        sdma_script_addr->mxc_sdma_start_addr = (unsigned short *)sdma_code;
        sdma_script_addr->mxc_sdma_uartsh_2_mcu_addr = uartsh_2_mcu_patched_ADDR;
        sdma_script_addr->mxc_sdma_uart_2_mcu_addr = uart_2_mcu_patched_ADDR;
#ifdef CONFIG_MOT_WFN483
#ifdef CONFIG_ARCH_MXC91321
	sdma_script_addr->mxc_sdma_utra_addr = sdma_utra_ADDR;
#else
	sdma_script_addr->mxc_sdma_utra_addr = -1;
#endif
#endif
	sdma_script_addr->mxc_sdma_ram_code_size = RAM_CODE_SIZE;
	sdma_script_addr->mxc_sdma_ram_code_start_addr = RAM_CODE_START_ADDR;
	sdma_script_addr->mxc_sdma_dptc_dvfs_addr = -1;
	sdma_script_addr->mxc_sdma_firi_2_mcu_addr = firi_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_firi_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_mshc_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_app_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_firi_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_shp_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_ata_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_firi_addr = mcu_2_firi_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_mshc_addr = -1;
	sdma_script_addr->mxc_sdma_ata_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_uartsh_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_shp_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_uart_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_app_2_per_addr = -1;
}

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

#if defined(CONFIG_MXC_WATCHDOG) || defined(CONFIG_MXC_WATCHDOG_MODULE)

static struct resource wdt_resources[] = {
	{
	 .start = WDOG1_BASE_ADDR,
	 .end = WDOG1_BASE_ADDR + 0x30,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = WDOG2_BASE_ADDR,
	 .end = WDOG2_BASE_ADDR + 0x30,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = INT_WDOG2,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device mxc_wdt_device = {
	.name = "mxc_wdt",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
	.num_resources = ARRAY_SIZE(wdt_resources),
	.resource = wdt_resources,
};

static void mxc_init_wdt(void)
{
	(void)platform_device_register(&mxc_wdt_device);
}
#else
static inline void mxc_init_wdt(void)
{
}
#endif

#if defined(CONFIG_MXC_IPU) || defined(CONFIG_MXC_IPU_MODULE)
static struct mxc_ipu_config mxc_ipu_data = {
	.rev = 1,
};

static struct resource ipu_resources[] = {
	{
	 .start = IPU_CTRL_BASE_ADDR,
	 .end = IPU_CTRL_BASE_ADDR + SZ_4K,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = INT_IPU,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device mxc_ipu_device = {
	.name = "mxc_ipu",
	.id = -1,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_ipu_data,
		},
	.num_resources = ARRAY_SIZE(ipu_resources),
	.resource = ipu_resources,
};

static void mxc_init_ipu(void)
{
	platform_device_register(&mxc_ipu_device);
}
#else
static inline void mxc_init_ipu(void)
{
}
#endif

/* MMC device data */

#if defined(CONFIG_MMC_MXC) || defined(CONFIG_MMC_MXC_MODULE)
/*!
 * Resource definition for the SDHC1
 */
static struct resource mxcsdhc1_resources[] = {
	[0] = {
	       .start = MMC_SDHC1_BASE_ADDR,
	       .end = MMC_SDHC1_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_MMC_SDHC1,
	       .end = INT_MMC_SDHC1,
	       .flags = IORESOURCE_IRQ,
	       }
};

/*!
 * Resource definition for the SDHC2
 */
static struct resource mxcsdhc2_resources[] = {
	[0] = {
	       .start = MMC_SDHC2_BASE_ADDR,
	       .end = MMC_SDHC2_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_MMC_SDHC2,
	       .end = INT_MMC_SDHC2,
	       .flags = IORESOURCE_IRQ,
	       }
};

/*! Device Definition for MXC SDHC1 */
static struct platform_device mxcsdhc1_device = {
	.name = "mxcmci",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
	.num_resources = ARRAY_SIZE(mxcsdhc1_resources),
	.resource = mxcsdhc1_resources,
};

/*! Device Definition for MXC SDHC2 */
static struct platform_device mxcsdhc2_device = {
	.name = "mxcmci",
	.id = 1,
	.dev = {
		.release = mxc_nop_release,
		},
	.num_resources = ARRAY_SIZE(mxcsdhc2_resources),
	.resource = mxcsdhc2_resources,
};

static inline void mxc_init_mmc(void)
{
	spba_take_ownership(SPBA_SDHC1, SPBA_MASTER_A | SPBA_MASTER_C);
	(void)platform_device_register(&mxcsdhc1_device);
	spba_take_ownership(SPBA_SDHC2, SPBA_MASTER_A | SPBA_MASTER_C);
	(void)platform_device_register(&mxcsdhc2_device);
}
#else
static inline void mxc_init_mmc(void)
{
}
#endif

struct mxc_gpio_port mxc_gpio_ports[GPIO_PORT_NUM] = {
	{
	 .num = 0,
	 .base = IO_ADDRESS(GPIO1_BASE_ADDR),
	 .irq = INT_GPIO1,
	 .virtual_irq_start = MXC_GPIO_BASE,
	 },
	{
	 .num = 1,
	 .base = IO_ADDRESS(GPIO2_BASE_ADDR),
	 .irq = INT_GPIO2,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN,
	 },
};

/* MU device data */

#if defined(CONFIG_MXC_MU) || defined(CONFIG_MXC_MU_MODULE)
#include <asm/arch/mxc_mu.h>

static struct resource mxc_mu_resources[] = {
        [0] = {
                .start  = INT_MU_RX_OR,
                .flags  = IORESOURCE_IRQ,
        },
        [1] = {
                .start  = INT_MU_TX_OR,
                .flags  = IORESOURCE_IRQ,
        },
        [2] = {
                .start  = MU_GN_INT0,
                .flags  = IORESOURCE_IRQ,
        },
};


/*!
 * This is platform device structure for adding MU
 */
static struct platform_device mxc_mu_device = {
        .name           = "mxc_mu",
        .id             = 0,
        .dev = {
                .release        = mxc_nop_release,
        },
        .num_resources  = ARRAY_SIZE(mxc_mu_resources),
        .resource       = mxc_mu_resources,
};

static inline void mxc_init_mu(void)
{
       (void)platform_device_register(&mxc_mu_device);
}
#else
static inline void mxc_init_mu(void)
{
}
#endif

static int __init mxc_init_devices(void)
{
	mxc_init_wdt();
	mxc_init_ipu();
	mxc_init_mmc();
	mxc_init_mu();

	// TODO: MOVE ME
	spba_take_ownership(SPBA_CSPI2, SPBA_MASTER_A);
	/* SPBA configuration for SSI2 - SDMA and MCU are set */
	spba_take_ownership(SPBA_SSI2, SPBA_MASTER_C | SPBA_MASTER_A);
	return 0;
}

arch_initcall(mxc_init_devices);
