/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __MXC_RTC_H__
#define __MXC_RTC_H__

#include <asm/hardware.h>
#include <asm/arch/clock.h>
#define RTC_INPUT_CLK_32768HZ
/*
#define RTC_INPUT_CLK_32000HZ
#define RTC_INPUT_CLK_32768HZ
*/

#if defined(RTC_INPUT_CLK_32768HZ)
#define RTC_INPUT_CLK   (0x00 << 5)
#else
#if defined(RTC_INPUT_CLK_32000HZ)
#define RTC_INPUT_CLK   (0x01 << 5)
#else				/* 38.4kHz */
#define RTC_INPUT_CLK   (0x02 << 5)
#endif
#endif

#define RTC_SW_BIT      (1 << 0)
#define RTC_ALM_BIT     (1 << 2)
#define RTC_1HZ_BIT     (1 << 4)
#define RTC_2HZ_BIT     (1 << 7)
#define RTC_SAM0_BIT    (1 << 8)
#define RTC_SAM1_BIT    (1 << 9)
#define RTC_SAM2_BIT    (1 << 10)
#define RTC_SAM3_BIT    (1 << 11)
#define RTC_SAM4_BIT    (1 << 12)
#define RTC_SAM5_BIT    (1 << 13)
#define RTC_SAM6_BIT    (1 << 14)
#define RTC_SAM7_BIT    (1 << 15)
#define PIT_ALL_ON      (RTC_2HZ_BIT | RTC_SAM0_BIT | RTC_SAM1_BIT | \
                         RTC_SAM2_BIT | RTC_SAM3_BIT | RTC_SAM4_BIT | \
			 RTC_SAM5_BIT | RTC_SAM6_BIT | RTC_SAM7_BIT)

#define RTC_ENABLE_BIT	(1 << 7)

#ifndef RTC_INPUT_CLK_32768HZ
#error The PIE code has to be modified to support different input clock!
#endif

#define MAX_PIE_NUM     9
#define MAX_PIE_FREQ	512
const u32 PIE_BIT_DEF[MAX_PIE_NUM][2] = {
	{2, RTC_2HZ_BIT},
	{4, RTC_SAM0_BIT},
	{8, RTC_SAM1_BIT},
	{16, RTC_SAM2_BIT},
	{32, RTC_SAM3_BIT},
	{64, RTC_SAM4_BIT},
	{128, RTC_SAM5_BIT},
	{256, RTC_SAM6_BIT},
	{MAX_PIE_FREQ, RTC_SAM7_BIT},
};

/* Those are the bits from a classic RTC we want to mimic */
#define RTC_IRQF                0x80	/* any of the following 3 is active */
#define RTC_PF                  0x40	/* Periodic interrupt */
#define RTC_AF                  0x20	/* Alarm interrupt */
#define RTC_UF                  0x10	/* Update interrupt for 1Hz RTC */

#define MXC_RTC_TIME            0
#define MXC_RTC_ALARM           1

#define _reg_RTC_HOURMIN   ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x00)))	/*  32bit rtc hour/min counter reg */
#define _reg_RTC_SECOND    ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x04)))	/*  32bit rtc seconds counter reg */
#define _reg_RTC_ALRM_HM   ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x08)))	/*  32bit rtc alarm hour/min reg */
#define _reg_RTC_ALRM_SEC  ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x0C)))	/*  32bit rtc alarm seconds reg */
#define _reg_RTC_RTCCTL    ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x10)))	/*  32bit rtc control reg */
#define _reg_RTC_RTCISR    ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x14)))	/*  32bit rtc interrupt status reg */
#define _reg_RTC_RTCIENR   ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x18)))	/*  32bit rtc interrupt enable reg */
#define _reg_RTC_STPWCH    ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x1C)))	/*  32bit rtc stopwatch min reg */
#define _reg_RTC_DAYR      ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x20)))	/*  32bit rtc days counter reg */
#define _reg_RTC_DAYALARM  ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x24)))	/*  32bit rtc day alarm reg */
#define _reg_RTC_TEST1     ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x28)))	/*  32bit rtc test reg 1 */
#define _reg_RTC_TEST2     ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x2C)))	/*  32bit rtc test reg 2 */
#define _reg_RTC_TEST3     ((volatile u32 *)(IO_ADDRESS(RTC_BASE_ADDR + 0x30)))	/*  32bit rtc test reg 3 */

#endif				/* __MXC_RTC_H__ */
