/*
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *  Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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

#include <linux/config.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/serial_8250.h>
#include <linux/kgdb.h>
#include <linux/input.h>
#include <linux/nodemask.h>
#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/flash.h>
#endif

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/keypad.h>
#include <asm/arch/memory.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>

#include "crm_regs.h"
#include "iomux.h"
/*!
 * @file mx31ads.c
 *
 * @brief This file contains the board specific initialization routines.
 *
 * @ingroup System
 */

extern void mxc_map_io(void);
extern void mxc_init_irq(void);
extern void mxc_cpu_init(void) __init;
extern void mxc_gpio_init(void) __init;
extern void mx31ads_gpio_init(void) __init;
extern struct sys_timer mxc_timer;
extern void mxc_cpu_common_init(void);

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)

/* Keypad keycodes for the EVB 8x8
 * keypad.  POWER and PTT keys don't generate
 * any interrupts via this driver so they are
 * not support.  The SYM is defined as ESC since
 * no SYM code is available and the ON/OFF is
 * defined as POWER.  The numbers, # and * are defined as
 * KEY_KP of keypad and not keyboard codes. Change them
 * if you like
 */
static u16 keymapping[64] = {
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_F2, KEY_END, KEY_BACK,
	KEY_F1, KEY_SENDFILE, KEY_HOME, KEY_PROG1, KEY_VOLUMEUP, KEY_PROG2,
	KEY_PROG3, KEY_PROG4,
	KEY_KP3, KEY_KP2, KEY_KP1, KEY_KP4, KEY_VOLUMEDOWN, KEY_KP7, KEY_KP5,
	KEY_KP6,
	KEY_KP9, KEY_KPDOT, KEY_KP8, KEY_KP0, KEY_KPASTERISK, KEY_RECORD, KEY_Q,
	KEY_W,
	KEY_A, KEY_S, KEY_D, KEY_E, KEY_F, KEY_R, KEY_T, KEY_Y,
	KEY_TAB, KEY_ESC, KEY_CAPSLOCK, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_G,
	KEY_B, KEY_H, KEY_N, KEY_M, KEY_J, KEY_K, KEY_U, KEY_I,
	KEY_SPACE, KEY_POWER, KEY_DOT, KEY_ENTER, KEY_L, KEY_BACKSPACE, KEY_P,
	KEY_O,
};

static struct keypad_data evb_8_by_8_keypad = {
	.rowmax = 8,
	.colmax = 8,
	.irq = INT_KPP,
	.learning = 0,
	.delay = 2,
	.matrix = keymapping
};

/* mxc keypad driver */
static struct platform_device mxc_keypad_device = {
	.name = "mxc_keypad",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &evb_8_by_8_keypad,
		},
};

static void mxc_init_keypad(void)
{
	(void)platform_device_register(&mxc_keypad_device);
}
#else
static inline void mxc_init_keypad(void)
{
}
#endif

#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
/*!
 * The serial port definition structure. The fields contain:
 * {UART, CLK, PORT, IRQ, FLAGS}
 */
static struct plat_serial8250_port serial_platform_data[] = {
	{
	 .membase = (void __iomem *)(PBC_BASE_ADDRESS + PBC_SC16C652_UARTA),
	 .mapbase = (unsigned long)(CS4_BASE_ADDR + PBC_SC16C652_UARTA),
	 .irq = EXPIO_INT_XUART_INTA,
	 .uartclk = 14745600,
	 .regshift = 0,
	 .iotype = UPIO_MEM,
	 .flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
	 },
	{
	 .membase = (void __iomem *)(PBC_BASE_ADDRESS + PBC_SC16C652_UARTB),
	 .mapbase = (unsigned long)(CS4_BASE_ADDR + PBC_SC16C652_UARTB),
	 .irq = EXPIO_INT_XUART_INTB,
	 .uartclk = 14745600,
	 .regshift = 0,
	 .iotype = UPIO_MEM,
	 .flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
	 },
};

/*!
 * REVISIT: document me
 */
static struct platform_device serial_device = {
	.name = "serial8250",
	.id = 0,
	.dev = {
		.platform_data = serial_platform_data,
		},
};

/*!
 * REVISIT: document me
 */
static int __init mxc_init_extuart(void)
{
	return platform_device_register(&serial_device);
}
#else
static inline int mxc_init_extuart(void)
{
}
#endif
/* MTD NOR flash */

#if defined(CONFIG_MTD_MXC) || defined(CONFIG_MTD_MXC_MODULE)

static struct mtd_partition mxc_nor_partitions[] = {
	{
	 .name = "Bootloader",
	 .size = 512 * 1024,
	 .offset = 0x00000000,
	 .mask_flags = MTD_WRITEABLE	/* force read-only */
	 },
	{
	 .name = "Kernel",
	 .size = 2 * 1024 * 1024,
	 .offset = MTDPART_OFS_APPEND,
	 .mask_flags = 0},
	{
	 .name = "userfs",
	 .size = 14 * 1024 * 1024,
	 .offset = MTDPART_OFS_APPEND,
	 .mask_flags = 0},
	{
	 .name = "rootfs",
	 .size = 12 * 1024 * 1024,
	 .offset = MTDPART_OFS_APPEND,
	 .mask_flags = MTD_WRITEABLE},
	{
	 .name = "FIS directory",
	 .size = 12 * 1024,
	 .offset = 0x01FE0000,
	 .mask_flags = MTD_WRITEABLE	/* force read-only */
	 },
	{
	 .name = "Redboot config",
	 .size = MTDPART_SIZ_FULL,
	 .offset = 0x01FFF000,
	 .mask_flags = MTD_WRITEABLE	/* force read-only */
	 },
};

static struct flash_platform_data mxc_flash_data = {
	.map_name = "cfi_probe",
	.width = 2,
	.parts = mxc_nor_partitions,
	.nr_parts = ARRAY_SIZE(mxc_nor_partitions),
};

static struct resource mxc_flash_resource = {
	.start = 0xa0000000,
	.end = 0xa0000000 + 0x02000000 - 1,
	.flags = IORESOURCE_MEM,

};

static struct platform_device mxc_nor_mtd_device = {
	.name = "mxc_nor_flash",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_flash_data,
		},
	.num_resources = 1,
	.resource = &mxc_flash_resource,
};

static void mxc_init_nor_mtd(void)
{
	(void)platform_device_register(&mxc_nor_mtd_device);
}
#else
static void mxc_init_nor_mtd(void)
{
}
#endif

/* MTD NAND flash */

#if defined(CONFIG_MTD_NAND_MXC) || defined(CONFIG_MTD_NAND_MXC_MODULE)

static struct mtd_partition mxc_nand_partitions[4] = {
	{
	 .name = "IPL-SPL",
	 .offset = 0,
	 .size = 0x4000},
	{
	 .name = "nand.kernel",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 4 * 1024 * 1024},
	{
	 .name = "nand.rootfs",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 20 * 1024 * 1024},
	{
	 .name = "nand.userfs",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL},
};

static struct flash_platform_data mxc_nand_data = {
	.parts = mxc_nand_partitions,
	.nr_parts = ARRAY_SIZE(mxc_nand_partitions),
	.width = 1,
};

static struct platform_device mxc_nand_mtd_device = {
	.name = "mxc_nand_flash",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_nand_data,
		},
};

static void mxc_init_nand_mtd(void)
{
	if (__raw_readl(MXC_CCM_RCSR) & MXC_CCM_RCSR_NF16B) {
		mxc_nand_data.width = 2;
	}
	(void)platform_device_register(&mxc_nand_mtd_device);
}
#else
static inline void mxc_init_nand_mtd(void)
{
}
#endif

static void mxc_expio_irq_handler(u32 irq, struct irqdesc *desc,
				  struct pt_regs *regs)
{
	u32 imr_val;
	u32 int_valid;
	u32 expio_irq;

	desc->chip->ack(irq);

	imr_val = __raw_readw(PBC_INTMASK_SET_REG);
	int_valid = __raw_readw(PBC_INTSTATUS_REG) & imr_val;

	if (unlikely(!int_valid)) {
		printk(KERN_ERR "\nEXPIO: Spurious interrupt:0x%0x\n\n",
		       int_valid);
		BUG();		/* oops */
	}

	expio_irq = MXC_EXP_IO_BASE;
	for (; int_valid != 0; int_valid >>= 1, expio_irq++) {
		struct irqdesc *d;
		if ((int_valid & 1) == 0)
			continue;
		d = irq_desc + expio_irq;
		if (unlikely(!(d->handle))) {
			printk(KERN_ERR "\nEXPIO irq: %d unhandeled\n",
			       expio_irq);
			BUG();	/* oops */
		}
		d->handle(expio_irq, d, regs);
	}
	desc->chip->unmask(irq);
}

/*
 * Disable an expio pin's interrupt by setting the bit in the imr.
 * @param irq		an expio virtual irq number
 */
static void expio_mask_irq(u32 irq)
{
	u32 expio = MXC_IRQ_TO_EXPIO(irq);
	/* mask the interrupt */
	__raw_writew(1 << expio, PBC_INTMASK_CLEAR_REG);
}

/*
 * Acknowledge an expanded io pin's interrupt by clearing the bit in the isr.
 * @param irq		an expanded io virtual irq number
 */
static void expio_ack_irq(u32 irq)
{
	u32 expio = MXC_IRQ_TO_EXPIO(irq);
	/* clear the interrupt status */
	__raw_writew(1 << expio, PBC_INTSTATUS_REG);
	/* mask the interrupt */
	expio_mask_irq(irq);
}

/*
 * Enable a expio pin's interrupt by clearing the bit in the imr.
 * @param irq		a expio virtual irq number
 */
static void expio_unmask_irq(u32 irq)
{
	u32 expio = MXC_IRQ_TO_EXPIO(irq);
	/* unmask the interrupt */
	__raw_writew(1 << expio, PBC_INTMASK_SET_REG);
}

static struct irqchip expio_irq_chip = {
	.ack = expio_ack_irq,
	.mask = expio_mask_irq,
	.unmask = expio_unmask_irq,
};

static int initialized = 0;

static int __init _mxc_expio_init(void)
{
	int i;

	initialized = 1;

	printk(KERN_INFO "MX31ADS EXPIO(CPLD) hardware\n");
	/*
	 * Configure INT line as GPIO input
	 */
	mxc_request_iomux(MX31_PIN_GPIO1_4, OUTPUTCONFIG_GPIO,
			  INPUTCONFIG_GPIO);
	mxc_set_gpio_direction(MX31_PIN_GPIO1_4, 1);

	/* disable the interrupt and clear the status */
	__raw_writew(0xFFFF, PBC_INTMASK_CLEAR_REG);
	__raw_writew(0xFFFF, PBC_INTSTATUS_REG);
	for (i = MXC_EXP_IO_BASE; i < (MXC_EXP_IO_BASE + MXC_MAX_EXP_IO_LINES);
	     i++) {
		set_irq_chip(i, &expio_irq_chip);
		set_irq_handler(i, do_level_IRQ);
		set_irq_flags(i, IRQF_VALID);
	}
	set_irq_type(EXPIO_PARENT_INT, IRQT_HIGH);
	set_irq_chained_handler(EXPIO_PARENT_INT, mxc_expio_irq_handler);

	return 0;
}

/*
 * This may get called early from board specific init
 */
int mxc_expio_init(void)
{
	if (!initialized)
		return _mxc_expio_init();
	else
		return 0;
}

/*!
 * Board specific fixup function. It is called by \b setup_arch() in
 * setup.c file very early on during kernel starts. It allows the user to
 * statically fill in the proper values for the passed-in parameters. None of
 * the parameters is used currently.
 *
 * @param  desc         pointer to \b struct \b machine_desc
 * @param  tags         pointer to \b struct \b tag
 * @param  cmdline      pointer to the command line
 * @param  mi           pointer to \b struct \b meminfo
 */
static void __init fixup_mxc_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{
	struct tag *t;
#ifdef CONFIG_KGDB_8250
	int i;
	for (i = 0;
	     i <
	     (sizeof(serial_platform_data) / sizeof(serial_platform_data[0]));
	     i += 1)
		kgdb8250_add_platform_port(i, &serial_platform_data[i]);
#endif

	mxc_cpu_init();
	for_each_tag(t, tags) {
		if (t->hdr.tag == ATAG_MEM) {
			t->u.mem.size = MEM_SIZE;
			break;
		}
	}
	if (t->hdr.size == 0) {
		printk("%s: no mem tag found\n", __FUNCTION__);
	}
#ifdef CONFIG_DISCONTIGMEM
	do {
		int nid;
		mi->nr_banks = MXC_NUMNODES;
		for (nid = 0; nid < mi->nr_banks; nid++) {
			SET_NODE(mi, nid);
		}
	} while (0);
#endif
}

/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
	mxc_cpu_common_init();

	/* Enable 26 mhz clock on CKO1 for MC13783 audio */
	mxc_ccm_modify_reg(MXC_CCM_COSR, 0x00000fff, 0x00000208);

	mxc_gpio_init();
        mx31ads_gpio_init();
	mxc_expio_init();
	mxc_init_keypad();
	mxc_init_extuart();
	mxc_init_nor_mtd();
	mxc_init_nand_mtd();
}

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_MX31ADS data structure.
 */
/* *INDENT-OFF* */
MACHINE_START(MX31ADS, "Freescale MX31ADS")
    MAINTAINER("Freescale Semiconductor, Inc.")
#ifdef CONFIG_SERIAL_8250_CONSOLE
    /*       physical memory    physical IO        virtual IO     */
    BOOT_MEM(PHYS_OFFSET_ASM, CS4_BASE_ADDR, CS4_BASE_ADDR_VIRT)
#else
    /*       physical memory    physical IO        virtual IO     */
    BOOT_MEM(PHYS_OFFSET_ASM, AIPS1_BASE_ADDR, AIPS1_BASE_ADDR_VIRT)
#endif
    BOOT_PARAMS(PHYS_OFFSET_ASM + 0x100)
    FIXUP(fixup_mxc_board)
    MAPIO(mxc_map_io)
    INITIRQ(mxc_init_irq)
    INIT_MACHINE(mxc_board_init)
    .timer = &mxc_timer,
MACHINE_END
