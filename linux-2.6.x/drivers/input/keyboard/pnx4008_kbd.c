/*
 * drivers/input/keyboard/pnx4008_kbd.c
 *
 * driver for Philips PNX4008 Premo Keypad
 *
 * Author: Dmitry Chigirev <source@mvista.com>,
 * based on code examples by Mike James provided by Philips
 * Copyright (c) 2005 Koninklijke Philips Electronics N.V.
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <asm/arch/platform.h>
#include <linux/input.h>

#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/arch/pm.h>
#include <asm/arch/gpio.h>

/*Chapter 29 page 262 */

#define MODULE_NAME "PNX4008-KBD"
#define PKMOD MODULE_NAME ": "

#define KEYCLK_CTRL  IO_ADDRESS((PNX4008_PWRMAN_BASE + 0xB0))

#define KS_DEB        IO_ADDRESS((PNX4008_KEYSCAN_BASE + 0x0))
#define KS_STATE_COND IO_ADDRESS((PNX4008_KEYSCAN_BASE + 0x4))
#define KS_IRQ        IO_ADDRESS((PNX4008_KEYSCAN_BASE + 0x8))
#define KS_SCAN_CTL   IO_ADDRESS((PNX4008_KEYSCAN_BASE + 0xC))
#define KS_FAST_TST   IO_ADDRESS((PNX4008_KEYSCAN_BASE + 0x10))
#define KS_MATRIX_DIM IO_ADDRESS((PNX4008_KEYSCAN_BASE + 0x14))
#define KS_DATA(x)    IO_ADDRESS((PNX4008_KEYSCAN_BASE + 0x40 + ((x)<<2)))

static unsigned long pnx_keypad_status;
#define KEYPAD_REGION_INITED    0
#define KEYPAD_DEVICE_INITED    1
#define KEYPAD_IRQ_INITED       2
#define KEYPAD_GPIO2_MUX_INITED 3
#define KEYPAD_GPIO3_MUX_INITED 4

static struct input_dev pnx_keypad_dev;
#if 1
/* Philips PNX4008 Premo Layout */
#define KS_MATRIX_DIM_SIZE 6
static unsigned char pnx_keycode_table[KS_MATRIX_DIM_SIZE << 3] = {
	KEY_MENU, KEY_BACKSPACE, KEY_ENTER, KEY_RESERVED,	/* Row 0 col 0-3: Menu, Clr, OK, x */
	KEY_KPASTERISK, KEY_0, KEY_RESERVED, KEY_RESERVED,	/* Row 0 col 4-7: *, 0, x,x */

	KEY_C, KEY_D, KEY_SLASH, KEY_RESERVED,	/* Row 1 col 0-3: C,D,#,x */
	KEY_9, KEY_8, KEY_RESERVED, KEY_RESERVED,	/* Row 1 col 4-7: 9,8,x,x  */

	KEY_B, KEY_6, KEY_5, KEY_RESERVED,	/* Row 2 col 0-3: B,6,5,x */
	KEY_7, KEY_4, KEY_RESERVED, KEY_RESERVED,	/* Row 2 col 4-7: 7,4,x,x */

	KEY_A, KEY_3, KEY_2, KEY_RESERVED,	/* Row 3 col 0-3: A,3,2,x */
	KEY_RESERVED, KEY_1, KEY_RESERVED, KEY_RESERVED,	/* Row 3 col 4-7: x,1,x,x */

	KEY_RIGHT, KEY_DOWN, KEY_KPENTER, KEY_RESERVED,	/* Row 4 col 0-3: RIGHT,DOWN,Val,x */
	KEY_UP, KEY_LEFT, KEY_RESERVED, KEY_RESERVED,	/* Row 4 col 4-7: UP,LEFT,x,x */

	KEY_F3, KEY_F2, KEY_F1, KEY_RESERVED,	/* Row 5 col 0-3: F3, F2, F1, x */
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,	/* Row 5 col 4-7: x,x,x,x */
};
#else
/*test matrix*/
#define KS_MATRIX_DIM_SIZE 8
#define KS_USE_ROWS_6_7
static unsigned char pnx_keycode_table[KS_MATRIX_DIM_SIZE << 3] = {
	64, 1, 2, 3,		/* Row 0 col 0-3: x,x,x,x */
	4, 5, 6, 7,		/* Row 0 col 4-7: x,x,x,x */

	8, 9, 10, 11,		/* Row 1 col 0-3: x,x,x,x */
	12, 13, 14, 15,		/* Row 1 col 4-7: x,x,x,x */

	16, 17, 18, 19,		/* Row 2 col 0-3: x,x,x,x */
	20, 21, 22, 23,		/* Row 2 col 4-7: x,x,x,x */

	24, 25, 26, 27,		/* Row 3 col 0-3: x,x,x,x */
	28, 29, 30, 31,		/* Row 3 col 4-7: x,x,x,x */

	32, 33, 34, 35,		/* Row 4 col 0-3: x,x,x,x */
	36, 37, 38, 39,		/* Row 4 col 4-7: x,x,x,x */

	40, 41, 42, 43,		/* Row 5 col 0-3: x,x,x,x */
	44, 45, 46, 47,		/* Row 5 col 4-7: x,x,x,x */

	48, 49, 50, 51,		/* Row 6 col 0-3: x,x,x,x */
	52, 53, 54, 55,		/* Row 6 col 4-7: x,x,x,x */

	56, 57, 58, 59,		/* Row 7 col 0-3: x,x,x,x */
	60, 61, 62, 63,		/* Row 7 col 4-7: x,x,x,x */
};
#endif

static unsigned char pnx_keypad_last_scan[KS_MATRIX_DIM_SIZE];

/*
 * Keyboard interrupt handler
 */
static irqreturn_t pnx_keypad_interrupt(int irq, void *data,
					struct pt_regs *regs)
{
	struct input_dev *dev = (struct input_dev *)data;
	int row, col;
	unsigned char scan_diff_bits, scan_bits, scancode, keycode;

	/* run along the status bits */
	for (col = 0; col < KS_MATRIX_DIM_SIZE; col++) {
		scan_bits = __raw_readl(KS_DATA(col));
		scan_diff_bits = scan_bits ^ pnx_keypad_last_scan[col];
		if (scan_diff_bits) {
			unsigned char mask = 1;
			for (row = 0; row < KS_MATRIX_DIM_SIZE; row++) {
				if (scan_diff_bits & mask) {
					scancode = (col << 3) + row;
					/* maximum value of column is 8 for this IP block, */
					/* 6 in pnx4008 hardware so a multiply by 8 will */
					/* always be adequate */
					keycode = pnx_keycode_table[scancode];	/* lookup keypress */
					if (scan_bits & mask) {
						input_report_key(dev, keycode, 1);	/* key pressed */
					} else {
						input_report_key(dev, keycode, 0);	/* key released */
					}
				}
				mask <<= 1;
			}
			pnx_keypad_last_scan[col] = scan_bits;
		}
	}
	input_sync(dev);
	__raw_writel(1, KS_IRQ);	/* clear the interrupt status in device, start scan */
	return IRQ_HANDLED;

}

/* turning on and off the keyboard clock */
static void inline pnx4008_kbd_clock_control(int on_off)
{
	__raw_writel(on_off ? 1 : 0, KEYCLK_CTRL);
}

static inline void pnx_keypad_reg_init(void)
{
	/* initialise keypad registers */
	__raw_writel(10, KS_SCAN_CTL);	/* time between each keypad scan - about 10ms */
	__raw_writel(2, KS_DEB);	/* debounce completes after 2 equal matrix values are read */
	__raw_writel(2, KS_FAST_TST);	/* 2 <=> 32kHz scanning master clk */
	__raw_writel(KS_MATRIX_DIM_SIZE, KS_MATRIX_DIM);	/* set matrix size */
	__raw_writel(1, KS_IRQ);	/* clear the interrupt status in device, start scan */
}

static inline void pnx_keypad_dev_init(struct input_dev *dev)
{
	int i;
	init_input_dev(dev);

	dev->name = MODULE_NAME;
	dev->evbit[0] = BIT(EV_KEY);

	dev->keycode = pnx_keycode_table;
	dev->keycodesize = sizeof(unsigned char);
	dev->keycodemax = ARRAY_SIZE(pnx_keycode_table);

	dev->id.bustype = BUS_HOST;
	dev->id.vendor = 0x0001;
	dev->id.product = 0x0001;
	dev->id.version = 0x0100;

	/* setup scancodes for which there is a keycode */
	for (i = 0; i < ARRAY_SIZE(pnx_keycode_table); i++) {
		if (pnx_keycode_table[i]) {
			set_bit(pnx_keycode_table[i], dev->keybit);
		}
	}
}

#ifdef CONFIG_PM
static int pnx_keypad_suspend(struct device *dev, u32 state, u32 level)
{
	switch (level) {
	case SUSPEND_DISABLE:
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		disable_irq(KEY_IRQ);
		break;
	}
	return 0;
}

static int pnx_keypad_resume(struct device *dev, u32 level)
{
	switch (level) {
	case RESUME_POWER_ON:
		enable_irq(KEY_IRQ);
		break;
	case RESUME_RESTORE_STATE:
		break;
	case RESUME_ENABLE:
		break;
	}
	return 0;
}
#endif

static int pnx_keypad_remove(struct device *dev);

/*initialize the device*/
static int pnx_keypad_probe(struct device *dev)
{
	int tmp;

	printk(KERN_INFO PKMOD "PNX4008 Keypad Driver\n");
	/* printk("%s %s\n", __TIME__, __DATE__); */

	if (!request_region(PNX4008_KEYSCAN_BASE, 0x1000, MODULE_NAME)) {
		printk(KERN_ERR PKMOD
		       "KS (keypad) registers are already in use\n");
		pnx_keypad_remove(dev);
		return -EBUSY;
	}
	set_bit(KEYPAD_REGION_INITED, &pnx_keypad_status);

#ifdef KS_USE_ROWS_6_7
	tmp = pnx4008_gpio_register_pin(GPIO_02);
	if (tmp < 0) {
		printk(KERN_ERR PKMOD "failed to register GPIO2\n");
		pnx_keypad_remove(dev);
		return tmp;
	}
	pnx4008_gpio_set_pin_mux(GPIO_02, 1);	/*set to MUX function */
	set_bit(KEYPAD_GPIO2_MUX_INITED, &pnx_keypad_status);

	tmp = pnx4008_gpio_register_pin(GPIO_03);
	if (tmp < 0) {
		printk(KERN_ERR PKMOD "failed to register GPIO3\n");
		pnx_keypad_remove(dev);
		return tmp;
	}
	pnx4008_gpio_set_pin_mux(GPIO_03, 1);	/*set to MUX function */
	set_bit(KEYPAD_GPIO3_MUX_INITED, &pnx_keypad_status);
#endif

	pnx_keypad_dev_init(&pnx_keypad_dev);	/*initialize device fields */
	input_register_device(&pnx_keypad_dev);
	pnx4008_kbd_clock_control(1);	/*enable clock */
	pnx_keypad_reg_init();	/*setup registers */

	/*setup wakeup interrupt */
	start_int_set_rising_edge(SE_KEY_IRQ);
	start_int_ack(SE_KEY_IRQ);
	start_int_umask(SE_KEY_IRQ);

	set_bit(KEYPAD_DEVICE_INITED, &pnx_keypad_status);

	/*interrupt may be triggered even if keypad clock is disabled */
	tmp = request_irq(KEY_IRQ, pnx_keypad_interrupt,
			  0, MODULE_NAME, &pnx_keypad_dev);
	if (tmp < 0) {
		printk(KERN_ERR PKMOD "failed to register irq\n");
		pnx_keypad_remove(dev);
		return tmp;
	}
	set_bit(KEYPAD_IRQ_INITED, &pnx_keypad_status);
	return 0;
}

/*deinitialize the device*/
static int pnx_keypad_remove(struct device *dev)
{
	if (test_bit(KEYPAD_IRQ_INITED, &pnx_keypad_status)) {
		free_irq(KEY_IRQ, &pnx_keypad_dev);
		clear_bit(KEYPAD_IRQ_INITED, &pnx_keypad_status);
	}

	if (test_bit(KEYPAD_DEVICE_INITED, &pnx_keypad_status)) {
		start_int_mask(SE_KEY_IRQ);
		pnx4008_kbd_clock_control(0);
		input_unregister_device(&pnx_keypad_dev);
		clear_bit(KEYPAD_DEVICE_INITED, &pnx_keypad_status);
	}
#ifdef KS_USE_ROWS_6_7
	if (test_bit(KEYPAD_GPIO3_MUX_INITED, &pnx_keypad_status)) {
		pnx4008_gpio_unregister_pin(GPIO_03);
		clear_bit(KEYPAD_GPIO3_MUX_INITED, &pnx_keypad_status);
	}

	if (test_bit(KEYPAD_GPIO2_MUX_INITED, &pnx_keypad_status)) {
		pnx4008_gpio_unregister_pin(GPIO_02);
		clear_bit(KEYPAD_GPIO2_MUX_INITED, &pnx_keypad_status);
	}
#endif

	if (test_bit(KEYPAD_REGION_INITED, &pnx_keypad_status)) {
		release_region(PNX4008_KEYSCAN_BASE, 0x1000);
		clear_bit(KEYPAD_REGION_INITED, &pnx_keypad_status);
	}
	return 0;
}

MODULE_AUTHOR("MontaVista Software, Inc. <source@mvista.com>");
MODULE_DESCRIPTION("PNX4008 Keypad Driver");
MODULE_LICENSE("GPL");

static struct device_driver pnx_keypad_device_driver = {
	.name = "keypad",
	.bus = &platform_bus_type,
	.probe = pnx_keypad_probe,
	.remove = pnx_keypad_remove,
#ifdef CONFIG_PM
	.suspend = pnx_keypad_suspend,
	.resume = pnx_keypad_resume,
#endif
};

static int __init pnx_keypad_init(void)
{
	return driver_register(&pnx_keypad_device_driver);
}

static void __exit pnx_keypad_exit(void)
{
	return driver_unregister(&pnx_keypad_device_driver);
}

module_init(pnx_keypad_init);
module_exit(pnx_keypad_exit);
