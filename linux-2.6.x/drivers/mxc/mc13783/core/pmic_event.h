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

#ifndef __MC13783_EVENT_H__
#define __MC13783_EVENT_H__

/*!
 * @file mc13783_event.h
 * @brief This is the header for event management.
 *
 * @ingroup MC13783
 */

#include <asm/arch/pmic_external.h>

/*!
 * These are the masks used when dealing
 * with mc13783 status registers. In mc13783
 * interrupt status bits are splited in
 * two registers.
 */
#define ISR_MASK_EVENT_0		0x00697fdf
#define ISR_MASK_EVENT_1		0x003efffb

struct mc13783_status_regs {
	unsigned long status0;
	unsigned long status1;
};

/*!
 * This function calls all callback of a specific event.
 *
 * @param        event   structure of event, it contains type of event
 */
void launch_all_callback_event(int sreg, type_event event);

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
int mc13783_event_subscribe_internal(type_event_notification * event_sub);

/*!
 * This function is used to un-subscribe on an event.
 *
 * @param        event_unsub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_event_unsubscribe_internal(type_event_notification * event_unsub);

#endif				/* __MC13783_EVENT_H__ */
