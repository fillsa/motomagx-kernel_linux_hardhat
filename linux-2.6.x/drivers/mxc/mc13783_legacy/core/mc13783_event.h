/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

#ifndef MC13783_EVENT_H_
#define MC13783_EVENT_H_

/*!
 * @file mc13783_event.h
 * @brief This is the header for event management.
 *
 * @ingroup MC13783
 */

#include <asm/arch/gpio.h>
#include "mc13783_external.h"

/*
 * Event handlers
 */

typedef struct {
	void (*callback) (void);	/* call back call when an event occurs */
	void (*callback_p) (void *param);	/* call back with parameter */
	void *param;
	void *next_notify_event;
} notify_event;

#define NO_PARAM -1

/*!
 * This function is the bottom half of the mc13783 Interrupt.
 * It checks the IT and launch client callback.
 *
 */
void mc13783_wq_handler(void *code);

/*!
 * This function initializes the GPIO of MX3 for mc13783 interrupt.
 *
 */
void mc13783_init_event_and_it_gpio(void);

/*!
 * This function is used to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_event_subscribe_internal(type_event_notification event_sub);

/*!
 * This function is used to un-subscribe on an event.
 *
 * @param        event_unsub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_event_unsubscribe_internal(type_event_notification event_unsub);

/*!
 * In test mode, this function is used to initialize 'TypeEventNotification' structure with the correct event.
 *
 * @param        event      event passed by the test application
 * @param        event_sub   event structure with IT and callback
 *
 */
void get_event(unsigned int event, type_event_notification * event_sub);

#endif				/* MC13783_EVENT_H_ */
