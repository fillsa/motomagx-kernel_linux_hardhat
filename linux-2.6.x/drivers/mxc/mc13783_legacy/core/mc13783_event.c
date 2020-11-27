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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/slab.h>

#include <asm/arch/gpio.h>
#include <asm/uaccess.h>

#include "mc13783_config.h"
#include "mc13783_register.h"
#include "mc13783_event.h"

static struct semaphore sem_event;

static int mask_bit_event_0[ISR_NB_BITS] = {
	EVENT_ADCDONEI,
	EVENT_ADCBISDONEI,
	EVENT_TSI,
	EVENT_WHIGHI,
	EVENT_WLOWI,
	-1,
	EVENT_CHGDETI,
	EVENT_CHGOVI,
	EVENT_CHGREVI,
	EVENT_CHGSHORTI,
	EVENT_CCCVI,
	EVENT_CHRGCURRI,
	EVENT_BPONI,
	EVENT_LOBATLI,
	EVENT_LOBATHI,
	-1,
	EVENT_USBI,
	-1,
	-1,
	EVENT_IDI,
	-1,
	EVENT_SE1I,
	EVENT_CKDETI,
	-1,
};

static int mask_bit_event_1[ISR_NB_BITS] = {
	EVENT_E1HZI,
	EVENT_TODAI,
	-1,
	EVENT_ONOFD1I,
	EVENT_ONOFD2I,
	EVENT_ONOFD3I,
	EVENT_SYSRSTI,
	EVENT_RTCRSTI,
	EVENT_PCI,
	EVENT_WARMI,
	EVENT_MEMHLDI,
	EVENT_PWRRDYI,
	EVENT_THWARNLI,
	EVENT_THWARNHI,
	EVENT_CLKI,
	EVENT_SEMAFI,
	-1,
	EVENT_MC2BI,
	EVENT_HSDETI,
	EVENT_HSLI,
	EVENT_ALSPTHI,
	EVENT_AHSSHORTI,
	-1,
	-1,
};

static int mc13783_mask_0;
static int mc13783_mask_1;
static int mask_done_0[ISR_NB_BITS];
static int mask_done_1[ISR_NB_BITS];
static notify_event *notify_event_tab[EVENT_NB];

int unmask(type_event event);
int mask(type_event event);
void launch_all_call_back_event(type_event event);

/*!
 * This function is used to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_event_subscribe_internal(type_event_notification event_sub)
{
	notify_event *new_notify_event;
	void *pmem = kmalloc(sizeof(notify_event), GFP_KERNEL);	/*check? */

	TRACEMSG(_K_D("event subscribe\n"));

	down_interruptible(&sem_event);
	new_notify_event = notify_event_tab[event_sub.event];

	if (new_notify_event == NULL) {
		notify_event_tab[event_sub.event] = (notify_event *) pmem;
		new_notify_event = notify_event_tab[event_sub.event];
		new_notify_event->callback = NULL;
		new_notify_event->callback_p = NULL;
		new_notify_event->param = (void *)NO_PARAM;
	} else {
		while (new_notify_event->next_notify_event != NULL) {
			new_notify_event = (notify_event *)
			    new_notify_event->next_notify_event;
		}
		new_notify_event->next_notify_event = (notify_event *) pmem;
		new_notify_event = (notify_event *)
		    new_notify_event->next_notify_event;
		new_notify_event->callback = NULL;
		new_notify_event->callback_p = NULL;
		new_notify_event->param = (void *)NO_PARAM;
	}
	if (event_sub.param != (void *)NO_PARAM) {
		new_notify_event->callback_p = event_sub.callback_p;
		new_notify_event->param = event_sub.param;
	} else {
		new_notify_event->callback = event_sub.callback;
	}
	new_notify_event->next_notify_event = NULL;
	up(&sem_event);

	CHECK_ERROR(unmask(event_sub.event));

	/* test if event is not available */
	mc13783_wq_handler(NULL);
	return 0;
}

/*!
 * This function is used to unsubscribe on an event.
 *
 * @param        event_unsub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_event_unsubscribe_internal(type_event_notification event_unsub)
{
	notify_event *del_notify_event;
	notify_event **notify_event_var;
	notify_event *notify_event_var2;

	down_interruptible(&sem_event);

	TRACEMSG(_K_D("event unsubscribe\n"));

	notify_event_var = &notify_event_tab[event_unsub.event];
	notify_event_var2 = *notify_event_var;

	if (*notify_event_var == NULL) {
		TRACEMSG(_K_I("unsubsribe error (event not found)"));
		return ERROR_UNSUBSCRIBE;
	}
	if (event_unsub.param == (void *)NO_PARAM) {
		while ((notify_event_var2)->callback != event_unsub.callback) {
			if (notify_event_var2->next_notify_event != NULL) {
				(notify_event_var) = (notify_event **)
				    & (notify_event_var2->next_notify_event);
				(notify_event_var2) = (notify_event *)
				    (notify_event_var2)->next_notify_event;
			} else {
				TRACEMSG(_K_I("Error (callback not found)"));
				return ERROR_UNSUBSCRIBE;
			}
		}
	} else {
		while ((notify_event_var2)->callback_p !=
		       event_unsub.callback_p) {
			if (notify_event_var2->next_notify_event != NULL) {
				(notify_event_var) = (notify_event **)
				    & (notify_event_var2->next_notify_event);
				(notify_event_var2) = (notify_event *)
				    (notify_event_var2)->next_notify_event;
			} else {
				TRACEMSG(_K_I("Error (callback not found)"));
				return ERROR_UNSUBSCRIBE;
			}
		}
	}
	(*notify_event_var) = (notify_event *)
	    (notify_event_var2)->next_notify_event;
	del_notify_event = (notify_event_var2);
	kfree(del_notify_event);
	del_notify_event = NULL;
	up(&sem_event);

	CHECK_ERROR(mask(event_unsub.event));
	return 0;
}

/*!
 * This function is the bottom half of the mc13783 Interrupt.
 * It checks the IT and launch client callback.
 *
 */
void mc13783_wq_handler(void *code)
{
	unsigned int status = 0, j = 0;
	unsigned int reg_value = 0;
	unsigned int mask_value = 0;

	TRACEMSG(_K_I("mc13783 IT bottom half"));
	TRACEMSG(_K_D("Test first it regs"));

	/* read and clear the status bit */
	status = ISR_MASK_EVENT_IT_0;
	mc13783_hs_write_reg(REG_INTERRUPT_STATUS_0, &status);

	/* reset unused bits */
	reg_value &= ISR_MASK_EVENT_IT_0;

	reg_value = status & ~mc13783_mask_0;

	if (reg_value != 0) {
		for (j = 0; j < ISR_NB_BITS; j++) {
			mask_value = 1 << j;
			if (mask_value & reg_value) {
				launch_all_call_back_event((type_event)
							   mask_bit_event_0[j]);
			}
		}
	}

	TRACEMSG(_K_D("Test second IT regs"));

	/* read and clear the status bit */
	status = ISR_MASK_EVENT_IT_1;
	mc13783_hs_write_reg(REG_INTERRUPT_STATUS_1, &status);

	reg_value = status & ~mc13783_mask_1;
	/* reset unused bits */
	reg_value &= ISR_MASK_EVENT_IT_1;

	if (reg_value != 0) {
		for (j = 0; j < ISR_NB_BITS; j++) {
			mask_value = 1 << j;
			if (mask_value & reg_value) {
				launch_all_call_back_event((type_event)
							   mask_bit_event_1[j]);
			}
		}
	}

}

/*!
 * This function initializes the GPIO of MX3 for mc13783 interrupt.
 *
 */
void mc13783_init_event_and_it_gpio(void)
{
	int i = 0;
	for (i = 0; i < EVENT_NB; i++) {
		notify_event_tab[i] = NULL;
	}
	mc13783_mask_0 = MASK_USED_BITS;
	mc13783_mask_1 = MASK_USED_BITS;
	for (i = 0; i < ISR_NB_BITS; i++) {
		mask_done_0[i] = 0;
		mask_done_1[i] = 0;
	}

	sema_init(&sem_event, 1);
}

/*!
 * This function sets a bit in mask register of mc13783 to mask an event IT.
 *
 * @param        event          structure of event, it contains type of event
 *
 * @return       This function returns 0 if successful.
 */
int unmask(type_event event)
{
	int bit_index = 0, j = 0, k = 0, *mc13783_mask;
	unsigned int mask_value = 0, mask = 0;
	unsigned int mask_reg = 0, status_reg = 0;
	int event_test = 0, *mask_done;

	if (event < EVENT_E1HZI) {
		mask_reg = REG_INTERRUPT_MASK_0;
		status_reg = REG_INTERRUPT_STATUS_0;
		mc13783_mask = &mc13783_mask_0;
	} else {
		mask_reg = REG_INTERRUPT_MASK_1;
		status_reg = REG_INTERRUPT_STATUS_1;
		mc13783_mask = &mc13783_mask_1;
	}

	for (j = 0; j < ISR_NB_BITS; j++) {
		k = 1 << j;
		if (event < EVENT_E1HZI) {
			event_test = mask_bit_event_0[j];
		} else {
			event_test = mask_bit_event_1[j];
		}
		if (event_test == event) {
			mask = k;
			bit_index = j + 1;
		}
	}

	if (event < EVENT_E1HZI) {
		mask_done = &mask_done_0[bit_index];
	} else {
		mask_done = &mask_done_1[bit_index];
	}

	if (*mask_done == 0) {
		/* Unmask MASK bit */
		CHECK_ERROR(mc13783_hs_read_reg(mask_reg, &mask_value));
		mask_value &= ~mask;
		*mc13783_mask = mask_value;
		CHECK_ERROR(mc13783_hs_write_reg(mask_reg, &mask_value));

	}
	(*mask_done)++;
	TRACEMSG(_K_D("IT Number : %d"), (*mask_done));
	return 0;
}

/*!
 * This function clears a bit in mask register of mc13783 to enable an event IT.
 *
 * @param        event   structure of event, it contains type of event
 *
 * @return       This function returns 0 if successful.
 */
int mask(type_event event)
{
	int bit_index = 0, j = 0, k = 0, *mc13783_mask;
	unsigned int mask_value = 0, mask = 0;
	unsigned int mask_reg = 0;
	int event_test, *mask_done;

	if (event < EVENT_E1HZI) {
		mask_reg = REG_INTERRUPT_MASK_0;
		mc13783_mask = &mc13783_mask_0;
	} else {
		mask_reg = REG_INTERRUPT_MASK_1;
		mc13783_mask = &mc13783_mask_1;
	}

	for (j = 0; j < ISR_NB_BITS; j++) {
		k = 1 << j;
		if (event < EVENT_E1HZI) {
			event_test = mask_bit_event_0[j];
		} else {
			event_test = mask_bit_event_1[j];
		}
		if (event_test == event) {
			mask = k;
			bit_index = j + 1;
		}
	}

	if (event < EVENT_E1HZI) {
		mask_done = &mask_done_0[bit_index];
	} else {
		mask_done = &mask_done_1[bit_index];
	}

	(*mask_done)--;
	TRACEMSG(_K_D("IT Number : %d"), (*mask_done));
	if (*mask_done == 0) {
		CHECK_ERROR(mc13783_hs_read_reg(mask_reg, &mask_value));
		mask_value |= mask;
		*mc13783_mask = mask_value;
		CHECK_ERROR(mc13783_hs_write_reg(mask_reg, &mask_value));
	}
	return 0;
}

/*!
 * This function calls all callback of a specific event.
 *
 * @param        event   structure of event, it contains type of event
 */
void launch_all_call_back_event(type_event event)
{
	notify_event *notify_event_var;
	void (*cur_callback) (void);
	void (*cur_callback_p) (void *param);
	void *param;

	down_interruptible(&sem_event);
	if (notify_event_tab[event] != NULL) {
		notify_event_var = notify_event_tab[event];
		do {
			if (notify_event_var->callback != NULL) {
				TRACEMSG(_K_D("Callback without param"));
				cur_callback = notify_event_var->callback;
				up(&sem_event);
				cur_callback();
				down_interruptible(&sem_event);
			} else if (notify_event_var->callback_p != NULL) {
				TRACEMSG(_K_D("Callback with param"));
				cur_callback_p = notify_event_var->callback_p;
				param = notify_event_var->param;
				up(&sem_event);
				cur_callback_p(param);
				down_interruptible(&sem_event);
			}
			TRACEMSG(_K_D("Callback done"));
			notify_event_var = (notify_event *)
			    notify_event_var->next_notify_event;
		} while (notify_event_var != NULL);
	} else {
		TRACEMSG(_K_I
			 ("it mc13783 event %d detected but not subscribed"),
			 event);
	}
	up(&sem_event);
}

/*!
 * In test mode, this function is called when mc13783 ADC Done interrupt occurs.
 */

/* these callback functions are used for test only */

#ifdef __TEST_CODE_ENABLE__

static void callbackfn(void *event)
{
	TRACEMSG_ALL_TIME(KERN_INFO "\n**********************************\n");
	TRACEMSG_ALL_TIME(KERN_INFO "*** IT MC13783 CALLBACK FUNCTION 1\n");
	TRACEMSG_ALL_TIME(KERN_INFO "*** Event Nb : %d \n", (int)event);
	TRACEMSG_ALL_TIME(KERN_INFO "**********************************\n");
}

static void callbackfn_2(void *event)
{
	TRACEMSG_ALL_TIME(KERN_INFO "\n**********************************\n");
	TRACEMSG_ALL_TIME(KERN_INFO "*** IT MC13783 CALLBACK FUNCTION 2\n");
	TRACEMSG_ALL_TIME(KERN_INFO "*** Event Nb : %d \n", (int)event);
	TRACEMSG_ALL_TIME(KERN_INFO "**********************************\n");
}

/*!
 * In test mode, this function is used to initialize 'TypeEventNotification' structure with the correct event.
 *
 * @param        event      event passed by the test application
 * @param        event_sub   event structure with IT and callback
 *
 */
void get_event(unsigned int event, type_event_notification * event_sub)
{
	unsigned int reg_val = 0;
	mc13783_event_init(event_sub);
	event_sub->event = event;
	event_sub->callback_p = callbackfn;
	event_sub->param = (void *)event;
	if (event == EVENT_TSI) {
		event_sub->callback_p = callbackfn_2;
		reg_val = 0x001c00;
		mc13783_write_reg(0, REG_ADC_0, &reg_val);
	}
}

#endif				/* __TEST_CODE_ENABLE__ */
