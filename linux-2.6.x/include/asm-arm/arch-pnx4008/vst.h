/*
 * include/asm-arm/arch-pnx4008/vst.h
 *
 * Author:	Scott Anderson <source@mvista.com>
 *
 * Overrides the default vst_bump_jiffies_by so that time keeping will
 * be correct with VST on.  Derived from include/asm-arm/arch-pxa/vst.h.
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __ASM_ARCH_VST_H__
#define __ASM_ARCH_VST_H__

/*
 * increment jiffies and fix-up MATCH0 to prevent double-counting the
 * elapsed time in the normal system timer
 */
#define vst_bump_jiffies_by(ref)                          \
        do {                                              \
                unsigned long match;                      \
                jiffies_64 += (ref);                      \
                match = __raw_readl(HSTIM_MATCH0);	  \
                match += ((ref) * arch_cycles_per_jiffy); \
                __raw_writel(match, HSTIM_MATCH0);    	  \
        } while(0)

#endif /* __ASM_ARCH_VST_H__ */

