/*
 *  linux/include/asm-arm/arch-pnx4008/uncompress.h
 *
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2005 MontaVista Software, Inc.
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

#define UART5_BASE 0x40090000

#define UART5_DR    (*(volatile unsigned char *) (UART5_BASE))
#define UART5_FR    (*(volatile unsigned char *) (UART5_BASE + 18))

/*
 * This does not append a newline
 */
static void putstr(const char *s)
{
	while (*s) {
		while (UART5_FR & (1 << 5))
			barrier();

		UART5_DR = *s;

		if (*s == '\n') {
			while (UART5_FR & (1 << 5))
				barrier();

			UART5_DR = '\r';
		}
		s++;
	}
	while (UART5_FR & (1 << 3)) ;
}

/*
 * nothing to do
 */
#define arch_decomp_setup()

#define arch_decomp_wdog()
