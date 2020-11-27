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

#ifndef SC55112_EVENT_H__
#define SC55112_EVENT_H__

/*!
 * @file sc55112_event.h
 * @brief This is the header for event management.
 *
 * @ingroup PMIC_SC55112_CORE
 */

#include "asm/arch/gpio.h"
#include "asm/arch/pmic_status.h"

/*
 * Event handlers
 */

/*!
 * @struct notify_event
 *
 * Define function pointer for notification of callback events.
 */
typedef struct {
	void (*callback) (void *param);	/* call back call when an event occurs */
	void *param;
	void *next_notify_event;
} notify_event;

#define NO_PARAM -1

/*!
 * This function is the bottom half of the sc55112 Interrupt.
 * It checks the IT and launch client callback.
 *
 */
void sc55112_wq_handler(void *code);

/*!
 * This function initializes the events for sc55112 interrupts.
 *
 */
void sc55112_init_event(void);

/*!
 * This function sets a bit in mask register of sc55112 to mask an event IT.
 *
 * @param        event          structure of event, it contains type of event
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS unmask(type_event event);

/*!
 * This function clears a bit in mask register of sc55112 to enable an event IT.
 *
 * @param        event   structure of event, it contains type of event
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS mask(type_event event);

/*!
 * This function calls all callback of a specific event.
 *
 * @param        event   structure of event, it contains type of event
 */
void launch_all_call_back_event(type_event event);

/*!
 * This function is used to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS sc55112_event_subscribe_internal(type_event_notification event_sub);

/*!
 * This function is used to un-subscribe on an event.
 *
 * @param        event_unsub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS sc55112_event_unsubscribe_internal(type_event_notification
					       event_unsub);

#endif				/* __EVENT_H__ */
