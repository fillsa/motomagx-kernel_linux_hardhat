/*
 * arch/arm/mach-pnx4008/clock.h
 *
 * Clock control driver for PNX4008 - header file
 *
 * Authors: Vitaly Wool, Dmitry Chigirev <source@mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ARCH_ARM_PNX4008_CLOCK_H
#define __ARCH_ARM_PNX4008_CLOCK_H

struct module;

#define PWRMAN_VA_BASE		IO_ADDRESS(PNX4008_PWRMAN_BASE)
#define HCLKDIVCTRL_REG		(PWRMAN_VA_BASE + 0x40)
#define PWRCTRL_REG		(PWRMAN_VA_BASE + 0x44)
#define PLLCTRL_REG		(PWRMAN_VA_BASE + 0x48)
#define OSC13CTRL_REG		(PWRMAN_VA_BASE + 0x4c)
#define SYSCLKCTRL_REG		(PWRMAN_VA_BASE + 0x50)
#define HCLKPLLCTRL_REG		(PWRMAN_VA_BASE + 0x58)
#define USBCTRL_REG		(PWRMAN_VA_BASE + 0x64)
#define SDRAMCLKCTRL_REG	(PWRMAN_VA_BASE + 0x68)
#define MSCTRL_REG		(PWRMAN_VA_BASE + 0x80)
#define BTCLKCTRL		(PWRMAN_VA_BASE + 0x84)
#define DUMCLKCTRL_REG		(PWRMAN_VA_BASE + 0x90)
#define I2CCLKCTRL_REG		(PWRMAN_VA_BASE + 0xac)
#define KEYCLKCTRL_REG		(PWRMAN_VA_BASE + 0xb0)
#define TSCLKCTRL_REG		(PWRMAN_VA_BASE + 0xb4)
#define PWMCLKCTRL_REG		(PWRMAN_VA_BASE + 0xb8)
#define SPICTRL_REG		(PWRMAN_VA_BASE + 0xc4)
#define FLASHCLKCTRL_REG	(PWRMAN_VA_BASE + 0xc8)
#define UART3CLK_REG		(PWRMAN_VA_BASE + 0xd0)
#define UARTCLKCTRL_REG		(PWRMAN_VA_BASE + 0xe4)
#define DMACLKCTRL_REG		(PWRMAN_VA_BASE + 0xe8)
#define AUTOCLK_CTRL		(PWRMAN_VA_BASE + 0xec)
#define JPEGCLKCTRL_REG		(PWRMAN_VA_BASE + 0xfc)

#define AUDIOCONFIG_VA_BASE	IO_ADDRESS(PNX4008_AUDIOCONFIG_BASE)
#define DSPPLLCTRL_REG		(AUDIOCONFIG_VA_BASE + 0x60)
#define DSPCLKCTRL_REG		(AUDIOCONFIG_VA_BASE + 0x64)
#define AUDIOCLKCTRL_REG	(AUDIOCONFIG_VA_BASE + 0x68)
#define AUDIOPLLCTRL_REG	(AUDIOCONFIG_VA_BASE + 0x6C)

#define USB_OTG_CLKCTRL_REG	IO_ADDRESS(PNX4008_USB_CONFIG_BASE + 0xff4)

#define VFP9CLKCTRL_REG		IO_ADDRESS(PNX4008_DEBUG_BASE)

struct clk {
	struct list_head node;
	struct module *owner;
	const char *name;
	struct clk *parent;
	struct clk *propagate_next;
	u32 rate;
	u32 user_rate;
	s8 usecount;
	u32 flags;
	u32 scale_reg;
	u8 enable_shift;
	u32 enable_reg;
	u8 enable_shift1;
	u32 enable_reg1;
	u32 parent_switch_reg;
	 u32(*round_rate) (struct clk *, u32);
	int (*set_rate) (struct clk *, u32);
	int (*set_parent) (struct clk * clk, struct clk * parent);
};

/* Flags */
#define RATE_PROPAGATES      (1<<0)
#define NEEDS_INITIALIZATION (1<<1)
#define PARENT_SET_RATE      (1<<2)
#define FIXED_RATE           (1<<3)

#define CLK_RATE_13MHZ 13000
#define CLK_RATE_1MHZ 1000
#define CLK_RATE_208MHZ 208000
#define CLK_RATE_48MHZ 48000
#define CLK_RATE_32KHZ 32

#endif
