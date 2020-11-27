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

/*!
 * @file mc13783_event.c
 * @brief This file manage all event of mc13783 component.
 *
 * It contains subscribe and unsubscribe function and the bottom half of IT.
 *
 * @ingroup MC13783
 */

/*
 * Includes
 */
#include <asm/arch/gpio.h>
#include <asm/arch/pmic_status.h>

#include "pmic_config.h"
#include "pmic_event.h"
#include "component_spec.h"

#define EVENT_MASK_0			0x697fdf
#define EVENT_MASK_1			0x3efffb
#define EVENT_MAX			48

static type_event_notification notify_event_tab[EVENT_MAX];
static struct semaphore sem_event;

extern int send_frame_to_spi(int, unsigned int *, bool);

/*!
 * This function sets a bit in mask register of mc13783 to unmask an event IT.
 *
 * @param        event          structure of event, it contains type of event
 *
 * @return       This function returns 0 if successful.
 */
int unmask(type_event event)
{
	unsigned int event_mask = 0;
	unsigned int mask_reg = 0;
	unsigned int event_bit = 0;

	if (event < EVENT_E1HZI) {
		mask_reg = REG_INTERRUPT_MASK_0;
		event_mask = EVENT_MASK_0;
	} else {
		event -= 24;
		mask_reg = REG_INTERRUPT_MASK_1;
		event_mask = EVENT_MASK_1;
	}

	event_bit = (1 << event);

	if ((event_bit & event_mask) == 0) {
		TRACEMSG(_K_I("Error: unmasking a reserved/unused event"));
		return PMIC_ERROR;
	}

	pmic_write_reg(PRIO_CORE, mask_reg, 0, event_bit);

	TRACEMSG(_K_D("IT Number : %d"), event);
	return 0;
}

/*!
 * This function sets a bit in mask register of mc13783 to disable an event IT.
 *
 * @param        event   structure of event, it contains type of event
 *
 * @return       This function returns 0 if successful.
 */
int mask(type_event event)
{
	unsigned int event_mask = 0;
	unsigned int mask_reg = 0;
	unsigned int event_bit = 0;
	unsigned int tmp = 0;

	tmp = event;

	if (tmp < EVENT_E1HZI) {
		mask_reg = REG_INTERRUPT_MASK_0;
		event_mask = EVENT_MASK_0;
	} else {
		tmp -= 24;
		mask_reg = REG_INTERRUPT_MASK_1;
		event_mask = EVENT_MASK_1;
	}

	event_bit = (1 << tmp);

	if ((event_bit & event_mask) == 0) {
		TRACEMSG(_K_I("Error: masking a reserved/unused event"));
		return PMIC_ERROR;
	}

	pmic_write_reg(PRIO_CORE, mask_reg, event_bit, event_bit);

	TRACEMSG(_K_D("IT Number : %d"), event);
	return 0;
}

/*!
 * This function is used to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS mc13783_event_subscribe_internal(type_event_notification *
					     event_sub)
{
	type_event_notification *entry;
	type_event_notification *cur;

	TRACEMSG(_K_D("event subscribe\n"));

	down_interruptible(&sem_event);
	event_sub->next = NULL;
	entry = &notify_event_tab[event_sub->event];

	if (entry->next == NULL) {
		entry->next = (void *)event_sub;
	} else {
		cur = (type_event_notification *) entry->next;

		while (cur->next != NULL) {
			cur = (type_event_notification *) cur->next;
		}

		cur->next = (void *)event_sub;
	}

	unmask(event_sub->event);
	up(&sem_event);
	return PMIC_SUCCESS;
}

/*!
 * This function is used to unsubscribe on an event.
 *
 * @param        event_unsub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS mc13783_event_unsubscribe_internal(type_event_notification *
					       event_unsub)
{
	type_event_notification *entry = NULL;
	type_event_notification *prev = NULL;
	type_event_notification *cur = NULL;

	TRACEMSG(_K_D("event unsubscribe\n"));

	down_interruptible(&sem_event);

	/*get list head */
	entry = &notify_event_tab[event_unsub->event];

	/*get first element of the list if any */
	cur = entry->next;
	prev = entry;

	while (cur != NULL) {
		if (cur != event_unsub) {
			prev = cur;
			cur = cur->next;
		} else {
			prev->next = cur->next;
			cur->next = NULL;
			break;
		}
	}

	if (entry->next == NULL) {
		mask(event_unsub->event);
	}

	up(&sem_event);
	return PMIC_SUCCESS;
}

/*
 not finished...
inline unsigned int get_interrupt_index(unsigned int bitmask)
{
	asm("rsbs   r1, r0, #0x0");
        asm("and    r1, r1, r0");
        asm("clzcc  r1, r1");
        asm("rsc    r0, r1, #0x20");
}
*/

/*!
 *  This function initializes events for mc13783 interrupt.
 *
 */
inline void mc13783_init_event_and_it_gpio(void)
{
	int i;

	for (i = 0; i < EVENT_MAX; i++) {
		notify_event_tab[i].event = -1;
		notify_event_tab[i].callback = NULL;
		notify_event_tab[i].param = NULL;
		notify_event_tab[i].next = NULL;
	}

	sema_init(&sem_event, 1);
}

/*!
 * This function calls all callback of a specific event.
 *
 * @param        event   structure of event, it contains type of event
 */
void launch_all_callback_event(int sreg, type_event event)
{
	type_event_notification *entry = NULL;
	type_event_notification *cur = NULL;
	int index = 0;

	TRACEMSG(_K_I("Event detected %d\n"), index);

	index = (sreg == 0) ? event : (event + 24);
	down_interruptible(&sem_event);

	entry = &notify_event_tab[index];
	if (entry->next == NULL) {
		TRACEMSG(_K_I("mc13783 event %d detected. \
      				No clients subscribed"), index);
		goto error_out;
	}

	cur = (type_event_notification *) entry->next;

	/* call each callback function */
	while (cur != NULL) {
		if (cur->callback != NULL) {
			cur->callback(cur->param);
		}

		cur = (type_event_notification *) cur->next;
	}

      error_out:
	up(&sem_event);
}
