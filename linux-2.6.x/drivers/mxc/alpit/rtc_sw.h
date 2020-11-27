/*
 * rtc_sw.h - Header file for Active and Low Power Interval Timers
 *
 * Copyright 2006 Motorola, Inc.
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

/*
 * DATE          AUTHOR         COMMMENT
 * ----          ------         --------
 * 10/04/2006    Motorola       Initial version
 */

#ifndef RTC_SW_H_
#define RTC_SW_H_

#ifdef RTC_SW_DEBUG
#define RTC_SW_DPRINTK(args...)    printk(args)
#else
#define RTC_SW_DPRINTK(args...)
#endif

#define RTC_SW                     "rtc_sw"

#define FREE_REQUEST(req_ptr)      kfree(req_ptr)

#define HUNDREDTHS_IN_HOUR         360000
#define HOURS_TO_HUNDREDTHS(hr)    (hr * HUNDREDTHS_IN_HOUR)
#define HUNDREDTHS_IN_MINUTE       6000
#define MINUTES_TO_HUNDREDTHS(min) (min * HUNDREDTHS_IN_MINUTE)
#define HUNDREDTHS_IN_SECOND       100
#define SECONDS_TO_HUNDREDTHS(sec) (sec * HUNDREDTHS_IN_SECOND)

#endif /* RTC_SW_H_ */
