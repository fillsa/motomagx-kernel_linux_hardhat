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
 * @file sc55112_register.c
 * @brief This file contains register function of sc55112 driver.
 *
 * @ingroup PMIC_SC55112_CORE
 */

/*
 * Includes
 */
#include <linux/wait.h>
#include "sc55112_config.h"
#include "asm/arch/pmic_status.h"
#include "sc55112_register.h"
#include "../../spi/spi.h"

/*
 * For test only
 */
#define SC55112_DEBUG

#ifdef SC55112_DEBUG
#define STATIC
#else
#define STATIC    static
#endif
/*
 * End of For test only
 */

STATIC PMIC_STATUS register_access_wait(t_prio priority);
STATIC PMIC_STATUS unlock_client(void);
STATIC PMIC_STATUS sc55112_spi_send_frame(int num_reg, unsigned int *reg_value,
					  bool _rw);

static int pending_client_count[MAX_PRIO_QUEUE];
static wait_queue_head_t sc55112_reg_queue[MAX_PRIO_QUEUE];

static t_sc55112_status sc55112_status;
static spi_config the_config;
static void *spi_id;

/*!
 * This function initializes register access.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS sc55112_init_register_access(void)
{
	/* initialize all semaphore for all clients */
	int i = 0;

	for (i = 0; i < MAX_PRIO_QUEUE; i++) {
		pending_client_count[i] = 0;
		init_waitqueue_head(&(sc55112_reg_queue[i]));
	}

	sc55112_status = sc55112_FREE;

	/* initialize SPI */
	the_config.module_number = SPI1;
	the_config.priority = HIGH;
	the_config.master_mode = true;
	the_config.bit_rate = 4000000;
	the_config.bit_count = 32;
	the_config.active_high_polarity = true;
	the_config.active_high_ss_polarity = false;
	the_config.phase = false;
	the_config.ss_low_between_bursts = true;
	the_config.ss_asserted = SS_0;
	the_config.tx_delay = 0;
	spi_id = spi_get_device_id(&the_config);
	return PMIC_SUCCESS;
}

/*!
 * This function reads a register.
 *
 * @param        priority   priority of access
 * @param        reg        register.
 * @param        reg_value   register value.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS sc55112_read_reg(t_prio priority, int reg, unsigned int *reg_value)
{
	if (sc55112_status == sc55112_BUSY) {
		CHECK_ERROR(register_access_wait(priority));
	}

	TRACEMSG(_K_D("read reg = %d"), reg);
	CHECK_ERROR(sc55112_spi_send_frame(reg, reg_value, false));
	TRACEMSG(_K_D("write register done, read value = 0x%x"), *reg_value);
	CHECK_ERROR(unlock_client());

	return PMIC_SUCCESS;
}

/*!
 * This function writes a register.
 *
 * @param        priority   priority of access
 * @param        reg        register.
 * @param        reg_value  register value.
 * @param        reg_mask   bitmap mask indicating which bits to be modified.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS sc55112_write_reg(t_prio priority, int reg, unsigned int reg_value,
			      unsigned int reg_mask)
{
	unsigned int temp;

	if (sc55112_status == sc55112_BUSY) {
		CHECK_ERROR(register_access_wait(priority));
	}

	TRACEMSG(_K_D("write reg = %d"), reg);
	if ((reg_mask & PMIC_ALL_BITS) ^ PMIC_ALL_BITS) {
		CHECK_ERROR(sc55112_spi_send_frame(reg, &temp, false));
		temp &= ~reg_mask;
		temp |= (reg_value & reg_mask);
	} else {
		temp = reg_value;
	}

	CHECK_ERROR(sc55112_spi_send_frame(reg, &temp, true));

	TRACEMSG(_K_D("write register done"));
	CHECK_ERROR(unlock_client());

	return PMIC_SUCCESS;
}

/*!
 * This function sleeps for R/W access if the spi is busy.
 *
 * @param        priority   priority of access
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
STATIC PMIC_STATUS register_access_wait(t_prio priority)
{
	DEFINE_WAIT(wait);

	if (pending_client_count[priority] < MAX_CLIENTS) {
		TRACEMSG(_K_D("sleep register access prio %d ...."), priority);

		pending_client_count[priority]++;
		prepare_to_wait(&(sc55112_reg_queue[priority]), &wait,
				TASK_INTERRUPTIBLE);
		schedule();
		sc55112_status = sc55112_BUSY;
		finish_wait(&(sc55112_reg_queue[priority]), &wait);

		TRACEMSG(_K_D("wake up register access prio %d ...."),
			 priority);
	} else {
		return PMIC_CLIENT_NBOVERFLOW;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function unlocks on locked register access.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
STATIC PMIC_STATUS unlock_client()
{
	int i;
	bool queue_date = false;

	for (i = 0; i < MAX_PRIO_QUEUE; i++) {
		if (pending_client_count[i] > 0) {
			queue_date = true;
			TRACEMSG(_K_D("unlock register prio %d"), i);
			pending_client_count[i]--;
			wake_up_interruptible(&(sc55112_reg_queue[i]));
			break;
		}
	}

	if (queue_date == false) {
		TRACEMSG(_K_D("no pending client release SPI"));
		sc55112_status = sc55112_FREE;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function is used to send a frame on SPI bus.
 *
 * @param        num_reg    PMIC register number
 * @param        reg_value  Register value
 * @param        _rw       read or write operation
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
STATIC PMIC_STATUS sc55112_spi_send_frame(int num_reg, unsigned int *reg_value,
					  bool _rw)
{
	unsigned int send_val;
	unsigned int frame = 0;
	unsigned int frame_ret = 0;
	unsigned int result = 0;

	if (_rw == true) {
		frame |= 0x80000000;
		frame |= (*reg_value & 0x0ffffff);
	} else {
		frame = 0;
	}
	frame |= ((unsigned int)num_reg & 0x3f) << 0x19;

	TRACEMSG(_K_D("\t\tspi send frame : value=0x%.8x"), frame & 0xffffffff);

	send_val = (((frame & 0x000000ff) << 0x18) |
		    ((frame & 0x0000ff00) << 0x08) |
		    ((frame & 0x00ff0000) >> 0x08) |
		    ((frame & 0xff000000) >> 0x18));
	/* use this to launch SPI operation. */
	result = spi_send_frame((unsigned char *)(&send_val), 4, spi_id);

	frame_ret = (((send_val & 0x000000ff) << 0x18) |
		     ((send_val & 0x0000ff00) << 0x08) |
		     ((send_val & 0x00ff0000) >> 0x08) |
		     ((send_val & 0xff000000) >> 0x18));
	*reg_value = frame_ret & 0x00ffffff;

	TRACEMSG(_K_D("\t\tspi received frame : value=0x%.8x"),
		 frame_ret & 0xffffffff);

	return PMIC_SUCCESS;
};
