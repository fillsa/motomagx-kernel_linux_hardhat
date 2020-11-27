/*
 * arch/arm/mach-pnx4008/core.c
 *
 * PNX4008 core startup code
 *
 * Authors: Vitaly Wool, Dmitry Chigirev,
 * Grigory Tolstolytkin, Dmitry Pervushin <source@mvista.com>
 *
 * Based on reference code received from Philips:
 * Copyright (C) 2003 Philips Semiconductors
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/device.h>
#include <linux/kgdb.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/system.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <asm/arch/irq.h>
#include <asm/arch/pm.h>
#include <asm/arch/serial.h>

extern void pnx4008_uart_pm(struct uart_port *port,
			    unsigned int state, unsigned int oldstate);

static struct uart_port serial_ports[] = {
	{
	 .membase = (void *)IO_ADDRESS(PNX4008_UART5_BASE),
	 .irq = IIR5_INT,
	 .uartclk = BASE_BAUD * 16,
	 .regshift = 2,
	 .iotype = UPIO_MEM,
	 .flags = UPF_BUGGY_UART | UPF_SKIP_TEST,
	 .type = PORT_16550A,
	 .line = 0,
	 .fifosize = 64,
	 .lock = SPIN_LOCK_UNLOCKED,
	 },
	{
	 .membase = (void *)IO_ADDRESS(PNX4008_UART3_BASE),
	 .irq = IIR3_INT,
	 .uartclk = BASE_BAUD * 16,
	 .regshift = 2,
	 .iotype = UPIO_MEM,
	 .flags = UPF_BUGGY_UART | UPF_SKIP_TEST,
	 .type = PORT_16550A,
	 .line = 1,
	 .fifosize = 64,
	 .lock = SPIN_LOCK_UNLOCKED,
	 },
};

static struct platform_device i2c0_device = {
	.name = "pnx-i2c",
	.id = 0,
};

static struct platform_device i2c1_device = {
	.name = "pnx-i2c",
	.id = 1,
};

static struct platform_device i2c2_device = {
	.name = "pnx-i2c",
	.id = 2,
};

static struct platform_device nand_flash_device = {
	.name = "pnx4008-flash",
	.id = -1,
	.dev = {
		.coherent_dma_mask = 0xFFFFFFFF,
		},
};

static struct platform_device dsp_oss_device = {
	.name = "dsp-oss",
	.id = -1,
};

static struct platform_device sdum_device = {
	.name = "sdum",
	.id = 0,
	.dev = {
		.coherent_dma_mask = 0xffffffff,
		},
};

static struct platform_device rgbfb_device = {
	.name = "rgbfb",
	.id = 0,
	.dev = {
		.coherent_dma_mask = 0xffffffff,
		}
};

static struct platform_device watchdog_device = {
	.name = "watchdog",
	.id = -1,
};

static struct platform_device keypad_device = {
	.name = "keypad",
	.id = -1,
};

/* The dmamask must be set for OHCI to work */
static u64 ohci_dmamask = ~(u32) 0;

static struct resource ohci_resources[] = {
	{
	 .start = IO_ADDRESS(PNX4008_USB_CONFIG_BASE),
	 .end = IO_ADDRESS(PNX4008_USB_CONFIG_BASE + 0x100),
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = USB_HOST_INT,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device ohci_device = {
	.name = "usb-ohci",
	.id = -1,
	.dev = {
		.dma_mask = &ohci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		},
	.num_resources = ARRAY_SIZE(ohci_resources),
	.resource = ohci_resources,
};

struct resource spipnx_0_resources[] = {
	{
	 .start = PNX4008_SPI1_BASE,
	 .end = PNX4008_SPI1_BASE + SZ_4K,
	 .flags = IORESOURCE_MEM,
	 }, {
	     .start = 11 /* SPI1_DMA_PERIPHERAL_ID */ ,
	     .flags = IORESOURCE_DMA,
	     }, {
		 .start = SPI1_INT,
		 .flags = IORESOURCE_IRQ,
		 }, {
		     .flags = 0,
		     }
};

struct resource spipnx_1_resources[] = {
	{
	 .start = PNX4008_SPI2_BASE,
	 .end = PNX4008_SPI2_BASE + SZ_4K,
	 .flags = IORESOURCE_MEM,
	 }, {
	     .start = 12 /* SPI2_DMA_PERIPHERAL_ID */ ,
	     .flags = IORESOURCE_DMA,
	     }, {
		 .start = SPI2_INT,
		 .flags = IORESOURCE_IRQ,
		 }, {
		     .flags = 0,
		     }
};

static struct platform_device spipnx_0 = {
	.name = "spipnx",
	.id = 0,
	.num_resources = ARRAY_SIZE(spipnx_0_resources),
	.resource = spipnx_0_resources,
	.dev = {
		.coherent_dma_mask = 0xFFFFFFFF,
		},
};

static struct platform_device spipnx_1 = {
	.name = "spipnx",
	.id = 1,
	.num_resources = ARRAY_SIZE(spipnx_1_resources),
	.resource = spipnx_1_resources,
	.dev = {
		.coherent_dma_mask = 0xFFFFFFFF,
		},
};

static struct platform_device *devices[] __initdata = {
	&i2c0_device,
	&i2c1_device,
	&i2c2_device,
	&ohci_device,
	&nand_flash_device,
	&dsp_oss_device,
	&sdum_device,
	&rgbfb_device,
	&watchdog_device,
	&keypad_device,
	&spipnx_0,
	&spipnx_1,
};

static int __init pnx4008_early_serial_init(void)
{
#ifdef CONFIG_SERIAL_8250
	early_serial_setup(&serial_ports[0]);
	early_serial_setup(&serial_ports[1]);
#endif		/* CONFIG_SERIAL_8250 */
#ifdef CONFIG_KGDB_8250
	kgdb8250_add_port(0, &serial_ports[0]);
	kgdb8250_add_port(1, &serial_ports[1]);
#endif		/* CONFIG_KGDB_8250 */

	return 0;
}

extern void pnx4008_uart_init(void);

static void __init PNX4008_init(void)
{
	/*disable all START interrupt sources,
	   and clear all START interrupt flags */
	__raw_writel(0, START_INT_ER_REG(SE_PIN_BASE_INT));
	__raw_writel(0, START_INT_ER_REG(SE_INT_BASE_INT));
	__raw_writel(0xffffffff, START_INT_RSR_REG(SE_PIN_BASE_INT));
	__raw_writel(0xffffffff, START_INT_RSR_REG(SE_INT_BASE_INT));

	/* Switch on the UART clocks */
	pnx4008_uart_init();

	platform_add_devices(devices, ARRAY_SIZE(devices));
}

static struct plat_serial8250_port platform_serial_ports[] = {
	{
	 .membase = (void *)IO_ADDRESS(PNX4008_UART5_BASE),
	 .irq = IIR5_INT,
	 .uartclk = BASE_BAUD * 16,
	 .regshift = 2,
	 .iotype = UPIO_MEM,
	 .flags =
	 UPF_BOOT_AUTOCONF | UPF_BUGGY_UART | UPF_SKIP_TEST | UPF_SPD_VHI,
	 .pm = pnx4008_uart_pm,
	 },
	{
	 .membase = (void *)IO_ADDRESS(PNX4008_UART3_BASE),
	 .irq = IIR3_INT,
	 .uartclk = BASE_BAUD * 16,
	 .regshift = 2,
	 .iotype = UPIO_MEM,
	 .flags =
	 UPF_BOOT_AUTOCONF | UPF_BUGGY_UART | UPF_SKIP_TEST | UPF_SPD_VHI,
	 .pm = pnx4008_uart_pm,
	 },
	{}
};

static struct platform_device serial_device = {
	.name = "serial8250",
	.id = 0,
	.dev = {
		.platform_data = &platform_serial_ports,
		},
};

int __init pnx4008_late_serial_init(void)
{
	pnx4008_uart_init();
	platform_device_register(&serial_device);
	return 0;
}

late_initcall(pnx4008_late_serial_init);

/**********************
 * IO description map *
 **********************/

static struct map_desc PNX4008_io_desc[] __initdata = {
	{IO_ADDRESS(PNX4008_IRAM_BASE), PNX4008_IRAM_BASE, SZ_64K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_SDRAM_CFG_BASE), PNX4008_SDRAM_CFG_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_USB_CONFIG_BASE), PNX4008_USB_CONFIG_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_AHB2FAB_BASE), PNX4008_AHB2FAB_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_DMA_CONFIG_BASE), PNX4008_DMA_CONFIG_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_PWRMAN_BASE), PNX4008_PWRMAN_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_INTCTRLMIC_BASE), PNX4008_INTCTRLMIC_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_INTCTRLSIC1_BASE), PNX4008_INTCTRLSIC1_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_INTCTRLSIC2_BASE), PNX4008_INTCTRLSIC2_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_HSUART1_BASE), PNX4008_HSUART1_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_HSUART2_BASE), PNX4008_HSUART2_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_HSUART7_BASE), PNX4008_HSUART7_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_RTC_BASE), PNX4008_RTC_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_PIO_BASE), PNX4008_PIO_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_MSTIMER_BASE), PNX4008_MSTIMER_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_HSTIMER_BASE), PNX4008_HSTIMER_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_WDOG_BASE), PNX4008_WDOG_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_DEBUG_BASE), PNX4008_DEBUG_BASE, SZ_8K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_TOUCH1_BASE), PNX4008_TOUCH1_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_KEYSCAN_BASE), PNX4008_KEYSCAN_BASE, SZ_8K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_UARTCTRL_BASE), PNX4008_UARTCTRL_BASE, SZ_8K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_PWM_BASE), PNX4008_PWM_BASE, SZ_8K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_UART3_BASE), PNX4008_UART3_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_UART4_BASE), PNX4008_UART4_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_UART5_BASE), PNX4008_UART5_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_UART6_BASE), PNX4008_UART6_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_I2C1_BASE), PNX4008_I2C1_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_I2C2_BASE), PNX4008_I2C2_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_MAGICGATE_BASE), PNX4008_MAGICGATE_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_DUMCONF_BASE), PNX4008_DUMCONF_BASE, SZ_16K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_DUM_MAINCFG_BASE), PNX4008_DUM_MAINCFG_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_DSP_BASE), PNX4008_DSP_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_PROFCOUNTER_BASE), PNX4008_PROFCOUNTER_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_CRYPTO_BASE), PNX4008_CRYPTO_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_CAMIFCONF_BASE), PNX4008_CAMIFCONF_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_YUV2RGB_BASE), PNX4008_YUV2RGB_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_AUDIOCONFIG_BASE), PNX4008_AUDIOCONFIG_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_NDF_FLASH_BASE), PNX4008_NDF_FLASH_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_MLC_FLASH_BASE), PNX4008_MLC_FLASH_BASE, SZ_4K, MT_DEVICE},
	{IO_ADDRESS(PNX4008_FLASH_DATA), PNX4008_FLASH_DATA, SZ_4K, MT_DEVICE},
};

void __init PNX4008_map_io(void)
{
	iotable_init(PNX4008_io_desc, ARRAY_SIZE(PNX4008_io_desc));
}

static void __init
PNX4008_fixup(struct machine_desc *desc, struct tag *tags,
	      char **cmdline, struct meminfo *mi)
{
	/* prevent kgdb breakage in case of kgdbwait */
	pnx4008_early_serial_init();
}

extern struct sys_timer PNX4008_timer;

MACHINE_START(PNX4008, "Philips PNX4008")
    MAINTAINER("MontaVista Software")
    BOOT_MEM(0x80000000, 0x40000000, 0xf4000000)
    BOOT_PARAMS(0x80000100)
    FIXUP(PNX4008_fixup)
    MAPIO(PNX4008_map_io)
    INITIRQ(PNX4008_init_irq)
    INIT_MACHINE(PNX4008_init)
    .timer = &PNX4008_timer, MACHINE_END
