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

#ifndef         __MC13783_RTC_H__
#define         __MC13783_RTC_H__

/*!
 * @defgroup MC13783_RTC mc13783 RTC Driver
 * @ingroup MC13783DRVRS
 */

/*!
 * @file mc13783_rtc.h
 * @brief This is the header of mc13783 rtc driver.
 *
 * @ingroup MC13783_RTC
 */

/*
 * Includes
 */
#include <asm/ioctl.h>

#define         MC13783_RTC_SET_TIME                   _IOWR('R',1, int)
#define         MC13783_RTC_GET_TIME                   _IOWR('R',2, int)
#define         MC13783_RTC_SET_ALARM		     _IOWR('R',3, int)
#define         MC13783_RTC_GET_ALARM		     _IOWR('R',4, int)
#define         MC13783_RTC_WAIT_ALARM		     _IOWR('R',5, int)
#define         MC13783_RTC_ALARM_REGISTER             _IOWR('R',6, int)
#define         MC13783_RTC_ALARM_UNREGISTER           _IOWR('R',7, int)

#ifdef __ALL_TRACES__
#define TRACEMSG_RTC(fmt,args...)  printk(fmt,##args)
#else				/* __ALL_TRACES__ */
#define TRACEMSG_RTC(fmt,args...)
#endif				/* __ALL_TRACES__ */

/*!
 * This enumeration define all rtc interrupt
 */
typedef enum {
	/*!
	 * Time of day alarm
	 */
	RTC_IT_ALARM,
	/*!
	 * 1 Hz timetick
	 */
	RTC_IT_1HZ,
	/*!
	 * RTC reset occurred
	 */
	RTC_IT_RST,
} t_rtc_int;

/*
 * RTC mc13783 API
 */

/* EXPORTED FUNCTIONS */

/*!
 * This function set the real time clock of mc13783
 *
 * @param        mc13783_time  	value of date and time
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_set_time(struct timeval *mc13783_time);

/*!
 * This function get the real time clock of mc13783
 *
 * @param        mc13783_time  	return value of date and time
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_get_time(struct timeval *mc13783_time);

/*!
 * This function set the real time clock alarm of mc13783
 *
 * @param        mc13783_time  	value of date and time
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_set_time_alarm(struct timeval *mc13783_time);

/*!
 * This function get the real time clock alarm of mc13783
 *
 * @param        mc13783_time  	return value of date and time
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_get_time_alarm(struct timeval *mc13783_time);

/*!
 * This function wait the Alarm event
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_wait_alarm(void);

/*!
 * This function is used to un/subscribe on rtc event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 * @param        sub      	define if Un/subscribe event.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_event(t_rtc_int event, void *callback, bool sub);

/*!
 * This function is used to subscribe on rtc event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_event_sub(t_rtc_int event, void *callback);

/*!
 * This function is used to unsubscribe on rtc event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_rtc_event_unsub(t_rtc_int event, void *callback);

/*!
 * This function is used to tell if mc13783 RTC has been correctly loaded.
 *
 * @return       This function returns 1 if RTC was successfully loaded
 * 		 0 otherwise.
 */
int mc13783_rtc_loaded(void);

#endif				/* __MC13783_RTC_H__ */
