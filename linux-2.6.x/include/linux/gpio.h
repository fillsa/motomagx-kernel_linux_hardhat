/*
 * include/linux/gpio.h
 *
 * Copyright (C) 2004 Robert Schwebel, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __GPIO_H
#define __GPIO_H

#include "asm/arch/gpio.h"

/* Values for policy */
#define GPIO_KERNEL     (1<<0)
#define GPIO_USER       (1<<1)
#define GPIO_INPUT      (1<<2)
#define GPIO_OUTPUT     (1<<3)

int request_gpio(unsigned int pin_nr, const char *owner,
		 unsigned char policy, unsigned char init_level);

void free_gpio(unsigned int pin_nr);

#endif
