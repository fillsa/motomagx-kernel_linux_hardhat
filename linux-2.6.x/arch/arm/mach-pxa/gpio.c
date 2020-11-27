/*
 * arch/arm/mach-pxa/gpio.c
 *
 * Additional GPIO class driver support for PXA27x.
 *
 * Author: Todd Poynor <tpoynor@mvista.com>
 *
 * 2006 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>

#include <asm/arch/hardware.h>
#include <asm/arch/pxa-regs.h>


int gpio_can_wakeup(int gpio_nr)
{
	switch (gpio_nr) {
	case 113:
	case 53:
	case 40:
	case 38:
	case 36:
	case 35:
	case 31:
	case 15:
	case 14:
	case 13:
	case 12:
	case 11:
	case 10:
	case 9:
	case 4:
	case 3:
	case 1:
	case 0:
		return 1;
	default:
		return 0;
	}
}


int gpio_get_wakeup_enable(int gpio_nr)
{
	u32 pwer = PWER;

	if (gpio_nr == 35)
		return !!(pwer&(1 << 24));
	if (gpio_nr == 31)
		return !!(((pwer & 0x180000) >> 19) == 1);
	if (gpio_nr == 113)
		return !!(((pwer & 0x180000) >> 19) == 2);
	if (gpio_nr == 38)
		return !!(((pwer & 0x70000) >> 16) == 1);
	if (gpio_nr == 53)
		return !!(((pwer & 0x70000) >> 16) == 2);
	if (gpio_nr == 40)
		return !!(((pwer & 0x70000) >> 16) == 3);
	if (gpio_nr == 36)
		return !!(((pwer & 0x70000) >> 16) == 4);

	pwer &= 0x0000fe1b; /* keep 15-0, and clear reserved bits 8:5, 2 */
	return !!(pwer & (1 << gpio_nr));
}


void gpio_set_wakeup_enable(int gpio_nr, int should_wakeup)
{
	int val = !!should_wakeup;

	if (gpio_nr == 35)
		PWER = (PWER & ~(1 << 24)) | (val << 24);
	if (gpio_nr == 31) {
		if (!val && ((PWER & 0x180000) == 1))
			PWER = PWER & ~0x180000;
		if (val)
			PWER = (PWER & ~0x180000) | 1 << 19;
	}
	if (gpio_nr == 113) {
		if (!val && ((PWER & 0x180000) == 2))
			PWER = PWER & ~0x180000;
		if (val)
			PWER = (PWER & ~0x180000) | 2 << 19;
	}
	if (gpio_nr == 38) {
		if (!val && ((PWER & 0x70000) == 1))
			PWER = PWER & ~0x70000;
		if (val)
			PWER = (PWER & ~0x70000) | 1 << 16;
	}
	if (gpio_nr == 53) {
		if (!val && ((PWER & 0x70000) == 2))
			PWER = PWER & ~0x70000;
		if (val)
			PWER = (PWER & ~0x70000) |  2 << 16;
	}

	if (gpio_nr == 40) {
		if (!val && ((PWER & 0x70000) == 3))
			PWER = PWER & ~0x70000;
		if (val)
			PWER = (PWER & ~0x70000) | 3 << 16;
	}
	if (gpio_nr == 36) {
		if (!val && ((PWER & 0x70000) == 4))
			PWER = PWER & ~0x70000;
		if (val)
			PWER = (PWER & ~0x70000) | 4 << 16;
	}

	if (((gpio_nr >= 9) && (gpio_nr <= 15)) ||
	    (gpio_nr == 0) || (gpio_nr == 1) ||
	    (gpio_nr == 3) || (gpio_nr == 4))
		PWER = (PWER & ~(1 << gpio_nr)) | (val << gpio_nr);
}

int gpio_waker(void)
{
	u32 gpioed;
	int i;

	if ((gpioed = PEDR) == 0)
		return -1;

	if (gpioed & (1 << 24))
		return 35;
	if (gpioed & (1 << 20)) {
		u32 wemux3 = (PWER & 0x180000) >> 19;

		switch (wemux3) {
		case 1:
			return 31;
		case 2:
			return 113;
		}
	}
	if (gpioed & (1 << 17)) {
		u32 wemux2 = (PWER & 0x70000) >> 16;

		switch (wemux2) {
		case 1:
			return 38;
		case 2:
			return 53;
		case 3:
			return 40;
		case 4:
			return 36;
		}
	}


	gpioed &= 0xfffffe1b; /* clear reserved bits 8:5, 2 */

	for (i = 15; i >= 0; i--)
		if (gpioed & (1 << i))
			return i;

	return -1;
}

EXPORT_SYMBOL(gpio_waker);
EXPORT_SYMBOL(gpio_can_wakeup);
EXPORT_SYMBOL(gpio_get_wakeup_enable);
EXPORT_SYMBOL(gpio_set_wakeup_enable);
