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
 * @file sc55112_event.c
 * @brief This file manage all event of sc55112 component.
 *
 * It contains subscribe and unsubscribe function and the bottom half of IT.
 *
 * @ingroup PMIC_sc55112_CORE
 */

/*
 * Includes
 */
#include <linux/spinlock.h>
#include "sc55112_config.h"
#include "sc55112_register.h"
#include "sc55112_event.h"
#include "asm/arch/gpio.h"

/* Create a spinlock that will be used to prevent race conditions when
 * possibly running in an interrupt context. Note that interrupts are
 * disabled when a spinlock is acquired. Therefore, we must minimize
 * the length of time that we hold onto a spinlock. We must also never
 * make calls to other drivers (e.g., the SPI and SDMA drivers) that
 * may call schedule() or sleep while holding a spinlock.
 */
static spinlock_t event_list_lock = SPIN_LOCK_UNLOCKED;

/* Create a mutex that will be used to prevent race conditions when
 * running in an interrupt context is not an issue. Interrupts remain
 * enabled when a mutex is acquired so we can make calls to other
 * drivers that may call schedule() or go to sleep (e.g., the SPI and
 * SDMA drivers) while holding a mutex.
 */
static DECLARE_MUTEX(mutex);

/* This is a pointer to the event handler array. It defines the currently
 * active set of events and user-defined callback functions.
 */
static notify_event *notify_event_tab[EVENT_NB];

/*!
 * This function is used to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event
 *                           and callback
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS sc55112_event_subscribe_internal(type_event_notification event_sub)
{
	notify_event *new_notify_event;
	notify_event *tail;

	TRACEMSG(_K_D("event subscribe\n"));

	/* Insert the new node to the end of the queue */
	new_notify_event =
	    (notify_event *) kmalloc(sizeof(notify_event), GFP_KERNEL);
	if (new_notify_event != NULL) {
		new_notify_event->callback = event_sub.callback;
		new_notify_event->param = event_sub.param;
		new_notify_event->next_notify_event = NULL;

		/* Start a critical section here to avoid a race condition with
		 * sc55112_event_unsubscribe_internal(). We must ensure that any
		 * overlapping calls to subscribe/unsubscribe the callback
		 * functions are handled sequentially to ensure a consistent
		 * hardware state.
		 */
		down_interruptible(&mutex);

		/* Also grab the spinlock since we need to avoid a race condition
		 * with the interrupt handler sc55112_wq_handler(). However, we will
		 * release the spinlock as soon as possible and just use the mutex
		 * if that is all that is required (i.e., once we've finished
		 * modifying the shared notify_event_tab[] data array).
		 */
		spin_lock(&event_list_lock);

		if (notify_event_tab[event_sub.event] == NULL) {
			notify_event_tab[event_sub.event] = new_notify_event;
			spin_unlock(&event_list_lock);

			/* Note that we cannot call unmask() while holding the
			 * spinlock (see the comment for the unmask() function
			 * below). However, we still do have the mutex to prevent
			 * a race condition with overlapping calls to
			 * sc55112_event_unsubscribe_internal().
			 */
			CHECK_ERROR(unmask(event_sub.event));
		} else {
			tail = notify_event_tab[event_sub.event];
			while (tail->next_notify_event != NULL) {
				tail = tail->next_notify_event;
			}
			tail->next_notify_event = new_notify_event;
			spin_unlock(&event_list_lock);
		}

		up(&mutex);

		/* test if event is not available */

		sc55112_wq_handler(NULL);

		return PMIC_SUCCESS;
	}

	return PMIC_ERROR;
}

/*!
 * This function is used to unsubscribe on an event.
 *
 * @param        event_unsub   structure of event, it contains type of event and callback
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS sc55112_event_unsubscribe_internal(type_event_notification
					       event_unsub)
{
	int maskIRQ = false;

	notify_event *pne1;
	notify_event *pne2;

	TRACEMSG(_K_D("event unsubscribe\n"));

	/* Start a critical section here to avoid a race condition
	 * with sc55112_event_subscribe_internal(). We must ensure that
	 * any overlapping calls to subscribe/unsubscribe the callback
	 * functions are handled sequentially to ensure a consistent
	 * hardware state.
	 */
	down_interruptible(&mutex);

	/* Also grab the spinlock since we need to avoid a race condition
	 * with the interrupt handler sc55112_wq_handler(). However, we will
	 * release the spinlock as soon as possible and just use the mutex
	 * if that is all that is required (i.e., once we've finished
	 * modifying the shared notify_event_tab[] data array).
	 */
	spin_lock(&event_list_lock);

	pne1 = notify_event_tab[event_unsub.event];
	pne2 = NULL;

	while (pne1 != NULL) {
		if (pne1->callback == event_unsub.callback) {
			if (pne2 != NULL) {
				pne2->next_notify_event =
				    pne1->next_notify_event;
			} else {
				/* Unsubscribed last callback for this event.
				 * We can also tell the sc55112 PMIC to
				 * mask the interrupt now.
				 */
				notify_event_tab[event_unsub.event] = NULL;
				maskIRQ = true;
			}
			break;
		}
		pne2 = pne1;
		pne1 = pne1->next_notify_event;
	}

	spin_unlock(&event_list_lock);

	if (pne1 == NULL) {
		TRACEMSG(_K_I("unsubsribe error (event not found)"));
		return PMIC_UNSUBSCRIBE_ERROR;
	}

	if (maskIRQ) {
		/* Note that we cannot call mask() until we have confirmed that
		 * the event was previously subscribed to. We also must not call
		 * mask() while holding the spinlock (see the comment for the
		 * mask() function below).
		 */
		CHECK_ERROR(mask(event_unsub.event));
	}

	up(&mutex);

	kfree(pne1);

	return PMIC_SUCCESS;
}

/*!
 * This function is the bottom half of the sc55112 Interrupt.
 * It checks the IT and launch client callback.
 *
 * WARNING: Be very careful about putting debugging statements in
 *          this function. Using printk() calls here may result in
 *          strange and usually bad behavior due to possible side
 *          effects while running in the context of an interrupt
 *          handler.
 */
void sc55112_wq_handler(void *code)
{
	unsigned int status = 0, j = 0;
	unsigned int mask_value = 0;

	/* read and clear the status bit */
	sc55112_read_reg(PRIO_CORE, REG_ISR, &status);
	status &= PMIC_INTS_TO_AP;
	sc55112_write_reg(PRIO_CORE, REG_ISR, status, status);

	if (status != 0) {
		for (j = 0; j < EVENT_NB; j++) {
			mask_value = 1 << j;
			if (mask_value & status) {
				launch_all_call_back_event((type_event) j);
			}
		}
	}
}

/*!
 * This function initializes events for sc55112 interrupt.
 *
 */
void sc55112_init_event(void)
{
	unsigned int status = 0;
	int i = 0;

	for (i = 0; i < EVENT_NB; i++) {
		notify_event_tab[i] = NULL;
	}

	/* Clear ISR */
	sc55112_read_reg(PRIO_CORE, REG_ISR, &status);
	status &= PMIC_INTS_TO_AP;
	sc55112_write_reg(PRIO_CORE, REG_ISR, status, status);

	spin_lock_init(&event_list_lock);
}

/*!
 * This function sets a bit in mask register of sc55112 to mask an event IT.
 *
 * Note that this function must not be called while in an atomic context
 * (e.g., while holding a spinlock) because sc55112_write_reg() will eventually
 * call the SPI driver which in turn will call schedule() because all SPI
 * bus transactions are asynchronous. This will result in "BUG: scheduling
 * while atomic ..." warning messages from the kernel.
 *
 * @param        event          structure of event, it contains type of event
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS unmask(type_event event)
{
	return sc55112_write_reg(PRIO_CORE, REG_IMR, 0, 1 << event);
}

/*!
 * This function clears a bit in mask register of sc55112 to enable an
 * event IT.
 *
 * Note that this function must not be called while in an atomic context
 * (e.g., while holding a spinlock) because sc55112_write_reg() will eventually
 * call the SPI driver which in turn will call schedule() because all SPI
 * bus transactions are asynchronous. This will result in "BUG: scheduling
 * while atomic ..." warning messages from the kernel.
 *
 * @param        event   structure of event, it contains type of event
 *
 * @return       This function returns PMIC_STATUS if successful.
 */
PMIC_STATUS mask(type_event event)
{
	return sc55112_write_reg(PRIO_CORE, REG_IMR, 1 << event, 1 << event);
}

/*!
 * This function calls all callback of a specific event.
 *
 * @param        event   structure of event, it contains type of event
 */
void launch_all_call_back_event(type_event event)
{
	notify_event *pne;

	spin_lock(&event_list_lock);

	pne = notify_event_tab[event];

	if (pne == NULL) {
		spin_unlock(&event_list_lock);
		TRACEMSG(_K_I("it sc55112 event %d detected but not "
			      "subscribed"), event);
	} else {
		/* call each callback function */
		while (pne != NULL) {
			if (pne->callback != NULL) {
				pne->callback(pne->param);
			}
			pne = pne->next_notify_event;
		}
		spin_unlock(&event_list_lock);
	}
}
