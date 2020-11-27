/*
 *  linux/arch/arm/mach-pnx4008/gpio-compat.c
 *
 *  PNX4008 GPIO compatibility interface
 *
 *  Copyright:	MontaVista Software Inc. (c) 2005
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/config.h>
#include <linux/module.h>

#include <asm/arch/gpio.h>
#include <asm/arch/gpio-compat.h>

int GPIO_RegisterPin(unsigned short Key)
{
	return pnx4008_gpio_register_pin(Key);
}
EXPORT_SYMBOL(GPIO_RegisterPin);

int GPIO_UnregisterPin(unsigned short Key)
{
	return pnx4008_gpio_unregister_pin(Key);
}
EXPORT_SYMBOL(GPIO_UnregisterPin);

unsigned long GPIO_ReadPin(unsigned short Key)
{
	return pnx4008_gpio_read_pin(Key);
}
EXPORT_SYMBOL(GPIO_ReadPin);

/* Write Value to output */
int GPIO_WritePin(unsigned short Key, int output)
{
	return pnx4008_gpio_write_pin(Key, output);
}
EXPORT_SYMBOL(GPIO_WritePin);

/* Value = 1 : Set GPIO pin as output */
/* Value = 0 : Set GPIO pin as input */
int GPIO_SetPinDirection(unsigned short Key, int output)
{
	return pnx4008_gpio_set_pin_direction(Key, output);
}
EXPORT_SYMBOL(GPIO_SetPinDirection);

/* Read GPIO pin direction: 0= pin used as input, 1= pin used as output*/
int GPIO_ReadPinDirection(unsigned short Key)
{
	return pnx4008_gpio_read_pin_direction(Key);
}
EXPORT_SYMBOL(GPIO_ReadPinDirection);

/* Value = 1 : Set pin to muxed function  */
/* Value = 0 : Set pin as GPIO */
int GPIO_SetPinMux(unsigned short Key, int output)
{
	return pnx4008_gpio_set_pin_mux(Key, output);
}
EXPORT_SYMBOL(GPIO_SetPinMux);

/* Read pin mux function: 0= pin used as GPIO, 1= pin used for muxed function*/
int GPIO_ReadPinMux(unsigned short Key)
{
	return pnx4008_gpio_read_pin_mux(Key);
}
EXPORT_SYMBOL(GPIO_ReadPinMux);
