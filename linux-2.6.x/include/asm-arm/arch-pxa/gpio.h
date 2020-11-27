/*
 * linux/include/asm-arm/arch-pxa/gpio.h
 *
 * Copyright (C) 2004 Robert Schwebel, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ARM_PXA_GPIO_H
#define __ARM_PXA_GPIO_H

#include <linux/kernel.h>
#include <asm/arch/hardware.h>

#define DEBUG 1

static inline void gpio_set_pin(int gpio_nr)
{
	GPSR(gpio_nr) |= GPIO_bit(gpio_nr);
	return;
}

static inline void gpio_clear_pin(int gpio_nr)
{
	GPCR(gpio_nr) |= GPIO_bit(gpio_nr);
	return;
}

static inline void gpio_dir_input(int gpio_nr)
{
	GPDR(gpio_nr) &= ~GPIO_bit(gpio_nr);
	return;
}

static inline void gpio_dir_output(int gpio_nr)
{
	GPDR(gpio_nr) |= GPIO_bit(gpio_nr);
	return;
}

static inline int gpio_get_pin(int gpio_nr)
{
	return GPLR(gpio_nr) & GPIO_bit(gpio_nr) ? 1:0;
}


extern int gpio_waker(void);
extern int gpio_can_wakeup(int gpio_nr);
extern int gpio_get_wakeup_enable(int gpio_nr);
extern void gpio_set_wakeup_enable(int gpio_nr, int should_wakeup);

#endif
