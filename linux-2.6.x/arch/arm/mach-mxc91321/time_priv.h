/*
 *      Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 *      Copyright (C) 2007-2008 Motorola, Inc.
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 * Date         Author    Comment
 * ----------   --------  --------------------
 * 02/28/2007   Motorola  Added watchdog debug support
 * 03/12/2007   Motorola  Rewirte the check start code of wdog make it more readable
 * 11/13/2007   Motorola  Reset the time out and service period of the watchdog
 * 01/14/2008   Motorola  Changed the timeout of watchdog2.
 */

#ifndef __TIME_PRIV_H__
#define __TIME_PRIV_H__
/*!
 * @file time_priv.h
 * @brief This file contains OS tick and wdog timer register definitions.
 *
 * This file contains OS tick and wdog timer register definitions.
 *
 * @ingroup Timers
 */

#include <asm/hardware.h>

/*
 * EPIT Control register bit definitions
 */
#define EPITCR_CLKSRC_OFF		0x00000000
#define EPITCR_CLKSRC_HIGHFREQ		0x02000000
#define EPITCR_CLKSRC_32K		0x03000000

#define EPITCR_OM_DISCONNECTED		0x00000000
#define EPITCR_OM_TOGGLE		0x00400000
#define EPITCR_OM_CLEAR			0x00800000
#define EPITCR_OM_SET			0x00C00000

#define EPITCR_STOPEN_ENABLE		0x00200000
#define EPITCR_WAITEN_ENABLE		0x00080000
#define EPITCR_DBGEN_ENABLE		0x00040000
#define EPITCR_IOVW_ENABLE		0x00020000
#define EPITCR_SWR			0x00010000

#define EPITCR_PRESCALER_MASK		0x0000FFF0
#define EPITCR_PRESCALER_SHIFT		0x00000004

#define EPITCR_RLD			0x00000008
#define EPITCR_OCIEN			0x00000004
#define EPITCR_ENMOD			0x00000002
#define EPITCR_EN			0x00000001

/*!
 * GPT Control register bit definitions
 */
#define GPTCR_FO3			(1<<31)
#define GPTCR_FO2			(1<<30)
#define GPTCR_FO1			(1<<29)

#define GPTCR_OM3_SHIFT			26
#define GPTCR_OM3_MASK			(7<<GPTCR_OM3_SHIFT)
#define GPTCR_OM3_DISCONNECTED		(0<<GPTCR_OM3_SHIFT)
#define GPTCR_OM3_TOGGLE		(1<<GPTCR_OM3_SHIFT)
#define GPTCR_OM3_CLEAR			(2<<GPTCR_OM3_SHIFT)
#define GPTCR_OM3_SET			(3<<GPTCR_OM3_SHIFT)
#define GPTCR_OM3_GENERATE_LOW		(7<<GPTCR_OM3_SHIFT)

#define GPTCR_OM2_SHIFT			23
#define GPTCR_OM2_MASK			(7<<GPTCR_OM2_SHIFT)
#define GPTCR_OM2_DISCONNECTED		(0<<GPTCR_OM2_SHIFT)
#define GPTCR_OM2_TOGGLE		(1<<GPTCR_OM2_SHIFT)
#define GPTCR_OM2_CLEAR			(2<<GPTCR_OM2_SHIFT)
#define GPTCR_OM2_SET			(3<<GPTCR_OM2_SHIFT)
#define GPTCR_OM2_GENERATE_LOW		(7<<GPTCR_OM2_SHIFT)

#define GPTCR_OM1_SHIFT			20
#define GPTCR_OM1_MASK			(7<<GPTCR_OM1_SHIFT)
#define GPTCR_OM1_DISCONNECTED		(0<<GPTCR_OM1_SHIFT)
#define GPTCR_OM1_TOGGLE		(1<<GPTCR_OM1_SHIFT)
#define GPTCR_OM1_CLEAR			(2<<GPTCR_OM1_SHIFT)
#define GPTCR_OM1_SET			(3<<GPTCR_OM1_SHIFT)
#define GPTCR_OM1_GENERATE_LOW		(7<<GPTCR_OM1_SHIFT)

#define GPTCR_IM2_SHIFT			18
#define GPTCR_IM2_MASK			(3<<GPTCR_IM2_SHIFT)
#define GPTCR_IM2_CAPTURE_DISABLE	(0<<GPTCR_IM2_SHIFT)
#define GPTCR_IM2_CAPTURE_RISING	(1<<GPTCR_IM2_SHIFT)
#define GPTCR_IM2_CAPTURE_FALLING	(2<<GPTCR_IM2_SHIFT)
#define GPTCR_IM2_CAPTURE_BOTH		(3<<GPTCR_IM2_SHIFT)

#define GPTCR_IM1_SHIFT			16
#define GPTCR_IM1_MASK			(3<<GPTCR_IM1_SHIFT)
#define GPTCR_IM1_CAPTURE_DISABLE	(0<<GPTCR_IM1_SHIFT)
#define GPTCR_IM1_CAPTURE_RISING	(1<<GPTCR_IM1_SHIFT)
#define GPTCR_IM1_CAPTURE_FALLING	(2<<GPTCR_IM1_SHIFT)
#define GPTCR_IM1_CAPTURE_BOTH		(3<<GPTCR_IM1_SHIFT)

#define GPTCR_SWR			(1<<15)
#define GPTCR_FRR			(1<<9)

#define GPTCR_CLKSRC_SHIFT		6
#define GPTCR_CLKSRC_MASK		(3<<GPTCR_CLKSRC_MASK)
#define GPTCR_CLKSRC_NOCLOCK		(0<<GPTCR_CLKSRC_SHIFT)
#define GPTCR_CLKSRC_HIGHFREQ		(2<<GPTCR_CLKSRC_SHIFT)
#define GPTCR_CLKSRC_CLKIN		(3<<GPTCR_CLKSRC_SHIFT)
#define GPTCR_CLKSRC_CLK32K		(7<<GPTCR_CLKSRC_SHIFT)

#define GPTCR_STOPEN			(1<<5)
#define GPTCR_DOZEN			(1<<4)
#define GPTCR_WAITEN			(1<<3)
#define GPTCR_DBGEN			(1<<2)

#define GPTCR_ENMOD			(1<<1)
#define GPTCR_ENABLE			(1<<0)

#define	GPTSR_OF1			(1<<0)
#define	GPTSR_OF2			(1<<1)
#define	GPTSR_OF3			(1<<2)
#define	GPTSR_IF1			(1<<3)
#define	GPTSR_IF2			(1<<4)
#define	GPTSR_ROV			(1<<5)

#define	GPTIR_OF1IE			GPTSR_OF1
#define	GPTIR_OF2IE			GPTSR_OF2
#define	GPTIR_OF3IE			GPTSR_OF3
#define	GPTIR_IF1IE			GPTSR_IF1
#define	GPTIR_IF2IE			GPTSR_IF2
#define	GPTIR_ROVIE			GPTSR_ROV

#ifndef	__noinstrument
#	define	__noinstrument
#endif

/*
 * This implements the OS tick timer to generate interrupts every 10ms
 */

#if !defined(HZ) || (HZ != 100)
#error HZ is not defined or not equal to 100
#endif

#ifdef CONFIG_MOT_FEAT_WDOG_CLEANUP
#else
#if 0				/* not enabled the two wdogs by default */
#define WDOG1_ENABLE		/* not defined by default */
#define WDOG2_ENABLE		/* not defined by default */
#endif
#endif /* CONFIG_MOT_FEAT_WDOG_CLEANUP */

#ifdef CONFIG_MOT_FEAT_DEBUG_WDOG
#define WDOG2_ENABLE
#endif /* CONFIG_MOT_FEAT_DEBUG_WDOG */

#ifdef CONFIG_MOT_FEAT_WDOG_CLEANUP || CONFIG_MOT_FEAT_DEBUG_WDOG
#define WDOG1_TIMEOUT           32000	/* WDOG1 timeout in ms */
#else
#define WDOG1_TIMEOUT           4000	/* WDOG1 timeout in ms */
#endif /* CONFIG_MOT_FEAT_WDOG_CLEANUP || CONFIG_MOT_FEAT_DEBUG_WDOG */


#if defined(CONFIG_MOT_FEAT_WDOG_CLEANUP) || defined (CONFIG_MOT_FEAT_DEBUG_WDOG)
#if (WDOG1_TIMEOUT < 1000)
#error WDOG1_TIMEOUT must be greater than 1000!
#endif
#define WDOG2_TIMEOUT           (WDOG1_TIMEOUT - 1000)     /* WDOG2 timeout in ms */
#if ((WDOG1_TIMEOUT/1000) < 20)
#define WDOG_SERVICE_PERIOD     (WDOG1_TIMEOUT / 2)    /* time interval in ms to service WDOGs */
#else
#define WDOG_SERVICE_PERIOD     10000   /* time interval in ms to service WDOGs */
#endif
#else
#define WDOG2_TIMEOUT           (WDOG1_TIMEOUT / 2)	/* WDOG2 timeout in ms */
#define WDOG_SERVICE_PERIOD     (WDOG2_TIMEOUT / 2)	/* time interval in ms to service WDOGs */

#if (WDOG1_TIMEOUT < WDOG2_TIMEOUT)
#error WDOG1_TIMEOUT must be greater than WDOG2_TIMEOUT!
#endif

#if (WDOG_SERVICE_PERIOD >= (WDOG2_TIMEOUT - 1000 / HZ))
#error WDOG_SERVICE_PERIOD is too large!
#endif

/* maximum timeout is 128s based on 2Hz clock */
#if ((WDOG1_TIMEOUT/1000) > 128)
#error WDOG time out has to be less than 128 seconds!
#endif
#endif/* CONFIG_MOT_FEAT_WDOG_CLEANUP || CONFIG_MOT_FEAT_DEBUG_WDOG */

#define WDOG_WT                 0x8	/* WDOG WT starting bit inside WCR */
#define WCR_WOE_BIT             (1 << 6)
#define WCR_WDA_BIT             (1 << 5)
#define WCR_SRS_BIT             (1 << 4)
#define WCR_WRE_BIT             (1 << 3)
#define WCR_WDE_BIT             (1 << 2)
#define WCR_WDBG_BIT            (1 << 1)
#define WCR_WDZST_BIT           (1 << 0)

#endif				/* __TIME_PRIV_H__ */
