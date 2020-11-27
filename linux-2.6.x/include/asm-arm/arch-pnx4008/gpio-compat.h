/*
 *  include/asm-arm/arch-pnx4008/gpio-compat.h
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __PHILIPS_GPIO_COMPAT_H__
#define __PHILIPS_GPIO_COMPAT_H__

#include <asm/arch/gpio.h>

#ifdef __KERNEL__
/*
 *	Call this before manipulating an IO Pin.
 *	Returns GPIO_BUSY if pin is already registered, otherwise GPIO_OK.
 */
int GPIO_RegisterPin(unsigned short Key);

/*
 *	Opposite of register. Returns GPIO_FAIL if pin wasn't registered,
 *	GPIO_OK otherwise.
 */
int GPIO_UnregisterPin(unsigned short Key);

/*
 *	Returns pin status, 0 if 0, nonzero otherwise.
 */
unsigned long GPIO_ReadPin(unsigned short Key);

/* 
 *	Writes Value to output pin
 */
int GPIO_WritePin(unsigned short Key, int Value);

/* Set GPIO pin direction (Value=0 : set pin as input, 1= set pin as output) */
int GPIO_SetPinDirection(unsigned short Key, int Value);

/* Read GPIO pin direction: 0= pin used as input, 1= pin used as output*/
int GPIO_ReadPinDirection(unsigned short Key);


/* Set pin to muxed function (Value=1) or to GPIO (Value=0) */
int GPIO_SetPinMux(unsigned short Key, int Value);


/* Read pin mux function: 0= pin used as GPIO, 1= pin used for muxed function*/
int GPIO_ReadPinMux(unsigned short Key);
#endif /* __KERNEL__ */

#endif /* __PHILIPS_GPIO_COMPAT_H__ */
