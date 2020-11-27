/*
 * linux/include/asm-arm/arch-pxa/pcmcia.h
 *
 * Author:	MontaVista Software, Inc. <source@mvista.com>
 * Created:	Jan 17, 2006
 * Copyright:	MontaVista Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_PCMCIA_H
#define __ASM_ARCH_PCMCIA_H

#include <linux/config.h>

#ifdef CONFIG_MACH_MAINSTONE

#include <asm/arch/mainstone.h>

static inline int pxa2xx_board_pcmcia_event(void)
{
	return (MST_PCMCIA0 & MST_PCMCIA_nSTSCHG_BVD1) || 
	       (MST_PCMCIA1 & MST_PCMCIA_nSTSCHG_BVD1);
}

#else /* not CONFIG_MACH_MAINSTONE */

#define pxa2xx_board_pcmcia_event() (0)

#endif /* CONFIG_MACH_MAINSTONE */
#endif /* __ASM_ARCH_PCMCIA_H */
