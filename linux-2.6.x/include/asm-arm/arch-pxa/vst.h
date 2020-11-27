/*
 * linux/include/asm-arm/arch-pxa/vst.h
 *
 * Author:	David Burrage
 * Created:	June 17, 2005
 * Copyright:	MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_VST_H__
#define __ASM_ARCH_VST_H__

/*
 * increment jiffies and fix-up OSMR0 to prevent double-counting the
 * elapsed time in the normal system timer
 */
#define vst_bump_jiffies_by(ref)                          \
        do {                                              \
                jiffies_64 += (ref);                      \
                OSMR0 += ((ref) * arch_cycles_per_jiffy); \
        } while(0)

#endif /* __ASM_ARCH_VST_H__ */
