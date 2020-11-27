/*
 * Initially based on linux/include/asm-arm/arch-pxa/vst.h
 * Author: David Burrage
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __ASM_ARCH_VST_H__
#define __ASM_ARCH_VST_H__

/*
 * increment jiffies and fix-up GPT OCR1 to prevent double-counting
 * the elapsed time in the normal system timer
 */
#define vst_bump_jiffies_by(ref)					\
	do {								\
		jiffies_64 += (ref);					\
		unsigned int gpt_gptocr1 = __raw_readl(MXC_GPT_GPTOCR1);\
		gpt_gptocr1 += ((ref) * arch_cycles_per_jiffy);		\
		__raw_writel(gpt_gptocr1,MXC_GPT_GPTOCR1);		\
	} while(0)

#define _arch_defined_vst_jiffie_mask
static inline void vst_stop_jiffie_int(void)
{
	return;
}
static inline void vst_start_jiffie_int(void)
{
	return;
}

#endif				/* __ASM_ARCH_VST_H__ */
