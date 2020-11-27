/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

#ifndef         __MC13783_RTC_DEFS_H__
#define         __MC13783_RTC_DEFS_H__

/*!
 * @defgroup PMIC_RTC mc13783 RTC Driver
 * @ingroup PMICDRVRS
 */

/*!
 * @file pmic_rtc_defs.h
 * @brief This is the internal header of PMIC RTC driver.
 *
 * @ingroup PMIC_RTC
 */

#ifdef __ALL_TRACES__
#define TRACEMSG_RTC(fmt,args...)  printk(fmt,##args)
#else				/* __ALL_TRACES__ */
#define TRACEMSG_RTC(fmt,args...)
#endif				/* __ALL_TRACES__ */

/*
 * RTC Time
 */
#define MC13783_RTCTIME_TIME_LSH			0
#define MC13783_RTCTIME_TIME_WID			17

/*
 * RTC Alarm
 */
#define MC13783_RTCALARM_TIME_LSH			0
#define MC13783_RTCALARM_TIME_WID			17

/*
 * RTC Day
 */
#define MC13783_RTCDAY_DAY_LSH			0
#define MC13783_RTCDAY_DAY_WID			15

/*
 * RTC Day alarm
 */
#define MC13783_RTCALARM_DAY_LSH			0
#define MC13783_RTCALARM_DAY_WID			15

#endif				/* __MC13783_RTC_DEFS_H__ */
