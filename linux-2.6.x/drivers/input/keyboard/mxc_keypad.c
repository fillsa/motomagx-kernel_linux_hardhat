/*
 * driver/input/keypad/mcx_keypad.c
 *
 * Keypad driver for Freescale MXC development boards
 * supports the zeus evb 8X8 keypad
 *
 * Author: Armin Kuster <someone@mvista.com>
 *
 * Initially based on drivers/char/mxc_keyb.c
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 * Linux input API plumbing borrowed from drivers/input/keyboard/mstone_keypad.c
 * and drivers/input/keyboard/omap-keypad.c
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/config.h>

#ifdef CONFIG_INPUT_EVBUG
#define DEBUG 1
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>

#include <asm/mach-types.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/mach/keypad.h>	/* includes input.h */

#include "mxc_keypad.h"
#define DVR_VER "2.0"

static void mxc_keypad_tasklet(unsigned long);
static void mxc_keypad_timer(unsigned long);

static struct input_dev *mxc_keypad_input;
static u16 keypad_state[MAXCOL];

static struct timer_list keypad_timer;
DECLARE_TASKLET_DISABLED(keypad_tasklet, mxc_keypad_tasklet, 0);

static struct platform_device *mxc_keyboard_dev;

static u16 *keymap;
static u16 maxrow, maxcol;
static u16 learning;
static u16 kp_delay = 8; /* default */

/*
 * Clear the KPKD status flag (write 1 to it) and synchronizer chain.
 * Set KDIE control bit, clear KRIE control bit (avoid false release
 * events.
 */
static void mxc_reset_interrupts(void)
{
	u16 reg_val;

	reg_val = __raw_readw(KPSR);
	reg_val |= (KBD_STAT_KPKD | KBD_STAT_KDIE);
	reg_val &= ~(KBD_STAT_KRIE | KBD_STAT_KRSS);
	__raw_writew(reg_val, KPSR);
}

/* Enables the keypad GPIO pins.
 */
static void mxc_keypad_enable(void)
{

	/* IOMUX configuration for keypad */
	gpio_keypad_active();
	mxc_reset_interrupts();
}

/*
 * Clear the KPKD status flag (write 1 to it) and synchronizer chain.
 * Set KDIE control bit, clear KRIE control bit (avoid false release
 * events. Disable the keypad GPIO pins.
 */
static void mxc_keypad_disable(void)
{

	__raw_writew(0x00, KPCR);
	__raw_writew(0x00, KPDR);
	__raw_writew(0x00, KDDR);

	mxc_reset_interrupts();
	gpio_keypad_inactive();
}

/*
 * Stop scanning and wait for interrupt.
 * Enable press interrupt and disable release interrupt.
 */
static void mxc_keywait_setup(void)
{
	u16 reg_val;

	reg_val = __raw_readw(KPSR);
	reg_val &= ~KBD_STAT_KRIE;
	reg_val |= (KBD_STAT_KPKR | KBD_STAT_KPKD | KBD_STAT_KDIE);
	__raw_writew(reg_val, KPSR);
}

/*
 * The configures the keypad
 */
static void mxc_keypad_config(void)
{
	u16 reg_val;

	/* Enable number of rows in keypad (KPCR[7:0])
	 * Configure keypad columns as open-drain (KPCR[15:8])
	 */
	reg_val = __raw_readw(KPCR);
	reg_val |= (((1 << maxcol) - 1) << 8) | ((1 << maxrow) - 1);
	__raw_writew(reg_val, KPCR);

	/* Set output as open drain KPDR[15:8]  -> 0's */
	reg_val = __raw_readw(KPDR);
	reg_val &= 0x00ff;
	__raw_writew(reg_val, KPDR);

	reg_val = __raw_readw(KDDR);

	/* set [15:8] as ouput -> 1's  */
	reg_val |= 0xff00;

	/* and [7:0] as inputs -> 0's */
	reg_val &= 0xff00;
	__raw_writew(reg_val, KDDR);

	mxc_keypad_enable();
}

/* Reference section 42.5.1
 * 1. Disable both(depress and release) keypad interrupts
 * 2. Write 1's to KPDR[15:8] setting column data to 1's
 * 3. Configure columns as totem pole outputs(for quick discharging of keypad
 * capacitance)
 * 4. Configure columns as open-drain
 * 5. Write a single column to 0, others to 1.
 * 6. Sample row inputs and save data. Multiple key presses can be detected on a single
 * column.
 * 7. Repeat steps 2 - 6 for remaining columns.
 * 8. Return all columns to 0 in preparation for standby mode.
 * 9. Clear KPKD and KPKR status bit(s) by writing to a "1",
 * set the KPKR synchronizer chain by writing "1" to KRSS register,
 * clear the KPKD synchronizer chain by writing "1" to KDSC register
 * 10. Re-enable the appropriate keypad interrupt(s);
 * KDIE to detect a key hold condition,
 * or KRIE to detect a key release event
 */
static void mxc_keypad_scan(u16 * state)
{

	u16 reg_val;
	int col, row;

	for (col = 0; col < maxcol; col++) {
		/* init state array */
		state[col] = 0xFF00;

		/* Write 1.s to KPDR[15:8] setting column data to 1's */
		reg_val = __raw_readw(KPDR);
		reg_val |= 0xff00;
		__raw_writew(reg_val, KPDR);

		udelay(kp_delay);

		/*
		 * Configure columns as totem pole outputs(for quick
		 * discharging of keypad capacitance) by writing 1's
		 * to the output pins
		 */
		reg_val = __raw_readw(KPCR);
		reg_val &= 0x00ff;
		__raw_writew(reg_val, KPCR);

		/* Configure columns as open-drain by
		 * writing 0's to the output pins
		 */
		reg_val |= 0xff00;
		__raw_writew(reg_val, KPCR);

		/*
		 * Write a single column to 0, others to 1.
		 * Col bit starts at 8th bit in KPDR
		 */

		reg_val = __raw_readw(KPDR);
		reg_val &= ~(1 << (8 + col));
		__raw_writew(reg_val, KPDR);

		/* Delay added to avoid propagating the 0 from column to row
		 * when scanning. */

		udelay(kp_delay);

		/* read row inputs and save data. Multiple key presses
		 * can be detected on a single column.
		 */
		reg_val = __raw_readw(KPDR);
		for (row = 0; row < maxrow; row++) {
			if ((reg_val & (0x1 << row)) == 0) {
				state[col] =
				    ((~(1 << (8 + col)) & 0xFF00) | (1 << row));
			}
		}
	}

	/* Prep for standby mode */
	__raw_writew(0x00, KPDR);

}

static void mxc_keypad_timer(unsigned long data)
{
	tasklet_schedule(&keypad_tasklet);
}

static void mxc_keypad_tasklet(unsigned long data)
{
	u16 new_state[8], changed, key_down = 0;
	int col, row;
	int spurious = 0;

	mxc_keypad_scan(new_state);

	/* check for changes and send those */
	for (col = 0; col < maxcol; col++) {
		changed = new_state[col] ^ keypad_state[col];
		key_down |= new_state[col];
		if (changed == 0)
			continue;
		for (row = 0; row < maxrow; row++) {
			if (!(changed & (1 << row)))
				continue;
			if (learning) {
				pr_info
				    ("mxc-keypad: key: %d -> %d-%d %s reg:%0x\n",
				     keymap[row * 8 + col], col, row,
				     (new_state[col] & (1 << row)) ? "pressed" :
				     "released",
				     ((~(1 << (8 + col)) & 0xff00) |
				      (1 << row)));
			} else {

				input_report_key(mxc_keypad_input,
						 keymap[row * 8 + col],
						 new_state[col] & (1 << row));
				input_sync(mxc_keypad_input);
			}
		}

	}

	memcpy(keypad_state, new_state, sizeof(keypad_state));

	if (key_down) {
		int delay = HZ / 20;
		/* some key is pressed - keep irq disabled and use timer
		 * to poll the keypad
		 */
		if (spurious)
			delay = 2 * HZ;
		mod_timer(&keypad_timer, jiffies + delay);
	} else {
		/* enable interrupts */
		mxc_keywait_setup();
	}
}

/*
 * This function is the keypad Interrupt handler.
 * This function checks for keypad status register (KPSR) for key press
 * and key release interrupt. If key press interrupt has occurred, then the key
 * press and key release interrupt in the KPSR are disabled. If the key
 * release interrupt has occurred, then key release interrupt is disabled and
 * key press interrupt is enabled in the KPSR.
 */
static irqreturn_t mxc_keypad_interrupt(int irq, void *ptr,
					struct pt_regs *regs)
{
	u16 reg_val;

	reg_val = __raw_readw(KPSR);

	/* Check if it is key press interrupt */
	if (reg_val & KBD_STAT_KPKD) {
		/*
		 * Disable key press(KDIE status bit) interrupt
		 * clear  key release(KRIE status bit) interrupt
		 */
		reg_val &= ~(KBD_STAT_KRIE | KBD_STAT_KDIE);
		__raw_writew(reg_val, KPSR);
	} else if (reg_val & KBD_STAT_KPKR) {
		/* Check if it is key release interrupt */
		/*
		 * set key press(KDIE status bit) interrupt.
		 * clear key release(KRIE status bit) interrupt.
		 */
		reg_val |= KBD_STAT_KDIE;
		reg_val &= ~KBD_STAT_KRIE;
		__raw_writew(reg_val, KPSR);
	} else {
		/* spurious interrupt */
		return IRQ_NONE;
	}

	/*
	 * Check if any keys are pressed, if so start polling.
	 */
	tasklet_schedule(&keypad_tasklet);

	return IRQ_HANDLED;
}

static int mxc_keypad_open(struct input_dev *dev)
{

	if (dev->private++ == 0) {
		mxc_keypad_config();
		/* this initializes the keypad_state
		 * for later comparisons
		 */
		mxc_keypad_scan(keypad_state);
	}

	return 0;
}

static void mxc_keypad_close(struct input_dev *dev)
{
	if (--dev->private == 0) {
		mxc_keypad_disable();
	}
}

static int __devinit mxc_keypad_probe(struct device *dev)
{
	int i, ret;
	struct keypad_data *k = dev->platform_data;

	u32 irq = k->irq;
	keymap = k->matrix;
	maxrow = k->rowmax;
	maxcol = k->colmax;
	learning = k->learning;
	kp_delay = k->delay;

	if (keymap == NULL) {
		pr_info(" No keypad map defined for %p!!\n", keymap);
		return -ENODEV;
	}

	mxc_keypad_input = kmalloc(sizeof(*mxc_keypad_input), GFP_KERNEL);
	if (!mxc_keypad_input){
		return -ENOMEM;
	}
	memset(mxc_keypad_input, 0, sizeof(*mxc_keypad_input));

	init_timer(&keypad_timer);
	keypad_timer.function = mxc_keypad_timer;

	/* get the irq and init timer */
	tasklet_enable(&keypad_tasklet);

	/* get the irq and init timer */

	ret =
	    request_irq(irq, mxc_keypad_interrupt, 0, "keypad",
			mxc_keypad_input);
	if (ret < 0) {
		kfree(mxc_keypad_input);
		tasklet_disable(&keypad_tasklet);
		del_timer_sync(&keypad_timer);
		return ret;
	}

	mxc_keypad_input->name = "mxc_keypad";
	mxc_keypad_input->id.bustype = BUS_HOST;
	mxc_keypad_input->open = mxc_keypad_open;
	mxc_keypad_input->close = mxc_keypad_close;

	set_bit(EV_KEY, mxc_keypad_input->evbit);
	set_bit(EV_REL, mxc_keypad_input->evbit);

	/* setup input device */
	for (i = 0; i < (maxrow * maxcol); i++)
		set_bit(keymap[i], mxc_keypad_input->keybit);

	input_register_device(mxc_keypad_input);

	pr_info("MXC Keypad Driver initialized \n");
	return 0;
}

static int __devexit mxc_keypad_remove(struct device *dev)
{
	input_unregister_device(mxc_keypad_input);
	free_irq(INT_KPP, mxc_keypad_input);
	kfree(mxc_keypad_input);
	tasklet_disable(&keypad_tasklet);
	del_timer_sync(&keypad_timer);

	pr_info("MXC Keypad Driver removed\n");
	return 0;
}

/* 
 * Set keypad interrupts when suspending since the key depress
 * interrupt may be used as a wakeup source
 */
static int mxc_keypad_suspend(struct device *dev, u32 state, u32 level)
{
	int ret = 0;
	switch (level) {
	case SUSPEND_POWER_DOWN:
	case SUSPEND_DISABLE:
		pr_info("Suspending\n");
		mxc_reset_interrupts();
		break;
	}
	return ret;
}

static int mxc_keypad_resume(struct device *dev, u32 level)
{
	int ret = 0;
	switch (level) {
	case RESUME_POWER_ON:
	case RESUME_ENABLE:
		/* put your code here */
		pr_info("Resuming\n");
		break;
	}
	return ret;
}

static struct device_driver mxc_keypad_driver = {
	.name = "mxc_keypad",
	.bus = &platform_bus_type,
	.probe = mxc_keypad_probe,
	.remove = mxc_keypad_remove,
	.suspend = mxc_keypad_suspend,
	.resume = mxc_keypad_resume,
};

static int __init mxc_keypad_init(void)
{
	int ret = 0;

	pr_info("MXC Keypad Driver %s\n", DVR_VER);
	if (driver_register(&mxc_keypad_driver) != 0) {
		printk(KERN_ERR
		       "Driver register failed for mxc_keypad_driver\n");
		return -ENODEV;
	}

	mxc_keyboard_dev =
	    platform_device_register_simple("keypad", 0, NULL, 0);
	if (IS_ERR(mxc_keyboard_dev)) {
		ret = PTR_ERR(mxc_keyboard_dev);
		driver_unregister(&mxc_keypad_driver);
	}

	return ret;
}

static void __exit mxc_keypad_exit(void)
{
	struct platform_device *key_dev = mxc_keyboard_dev;

	platform_device_unregister(key_dev);
	driver_unregister(&mxc_keypad_driver);
}

module_init(mxc_keypad_init);
module_exit(mxc_keypad_exit);

MODULE_AUTHOR("Armin Kuster");
MODULE_DESCRIPTION("Freescale MXC keypad driver");
MODULE_LICENSE("GPL");
