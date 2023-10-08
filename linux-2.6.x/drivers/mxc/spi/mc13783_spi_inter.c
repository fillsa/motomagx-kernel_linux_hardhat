/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Copyright 2006 Motorola, Inc.
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
 * Date     Author    Comment
 * 10/2006  Motorola  Added support for power_ic drivers
 *                    Added SPI timing work-arounds for hardware issues
 */

/*!
 * @file mc13783_spi_inter.c
 * @brief This file contains interface with the SPI driver. 
 *
 * @ingroup MC13783
 */

/*
 * Includes
 */

#include <linux/module.h>
#include <asm/arch/mc13783_spi_inter.h>
#include "../mc13783_legacy/core/mc13783_config.h"
#include "spi.h"

static spi_config the_config;
static void *spi_id;

extern int gpio_mc13783_get_spi(void);
extern int gpio_mc13783_get_ss(void);

/*!
 * This function configures the SPI access.
 *
 * @return       This function returns 0 if successful.
 */
int spi_init(void) 
{
        TRACEMSG(_K_D("\t\tspi configuration"));

        /* Initialize the semaphore */
	the_config.module_number = gpio_mc13783_get_spi();

        the_config.priority = HIGH;
	the_config.master_mode = 1;
#ifdef CONFIG_MOT_FEAT_ATLAS_TIMING_ISSUES
        the_config.bit_rate = 4200000;
#else
        the_config.bit_rate = 4000000;
#endif
        the_config.bit_count = 32;
	the_config.active_high_polarity = 1;
	the_config.active_high_ss_polarity = 1;
	the_config.phase = 0;
	the_config.ss_low_between_bursts = 1;
	the_config.ss_asserted = gpio_mc13783_get_ss();
        the_config.tx_delay = 0;
	spi_id = spi_get_device_id((spi_config *) & the_config);

        return ERROR_NONE;
};

EXPORT_SYMBOL(spi_init);

/*!
 * This function is used to write on a mc13783 register.
 *
 * @param        num_reg   mc13783 register number
 * @param        reg_value  Register value
 *
 * @return       This function returns 0 if successful.
 */
int spi_write_reg(int num_reg, unsigned int *reg_value)
{ 
	CHECK_ERROR(spi_send_frame_to_spi(num_reg, reg_value, 1));
	return ERROR_NONE;
};

EXPORT_SYMBOL(spi_write_reg);

/*!
 * This function is used to read on a mc13783 register.
 *
 * @param        num_reg   mc13783 register number
 * @param        reg_value  Register value
 *
 * @return       This function returns 0 if successful.
 */
int spi_read_reg(int num_reg, unsigned int *reg_value)
{
	CHECK_ERROR(spi_send_frame_to_spi(num_reg, reg_value, 0));
        return ERROR_NONE;
};

EXPORT_SYMBOL(spi_read_reg);

/*!
 * This function is used to send a frame on SPI bus.
 *
 * @param        num_reg    mc13783 register number
 * @param        reg_value  Register value
 * @param        rw         read or write operation
 *
 * @return       This function returns 0 if successful, -1 otherwise.
 */
int spi_send_frame_to_spi(int num_reg, unsigned int *reg_value, int rw)
{
	unsigned int frame_ret = 0;
	unsigned int result = 0;
	unsigned int frame = 0;
        unsigned int send_val;	

	if (rw == 1) {
                frame |= 0x80000000;
        } else {
                frame = 0;
        }

        frame |= (*reg_value & 0x0ffffff);
	frame |= ((unsigned int)num_reg & 0x3f) << 0x19;
        
	TRACEMSG(_K_D("\t\tspi send frame : value=0x%.8x"), frame & 0xffffffff);
        
	send_val = (((frame & 0x000000ff) << 0x18) |
		    ((frame & 0x0000ff00) << 0x08) |
		    ((frame & 0x00ff0000) >> 0x08) |
		    ((frame & 0xff000000) >> 0x18));

	result = mxc_spi_is_active(the_config.module_number);
	if (result == 0) {
		return -1;
	}

	/* use this to launch SPI operation. */
	result = spi_send_frame((unsigned char *)(&send_val),
				(unsigned long)4, spi_id);

	frame_ret = (((send_val & 0x000000ff) << 0x18) |
		     ((send_val & 0x0000ff00) << 0x08) |
		     ((send_val & 0x00ff0000) >> 0x08) |
		     ((send_val & 0xff000000) >> 0x18));

        *reg_value = frame_ret & 0x00ffffff;
        
	TRACEMSG(_K_D
		 ("\t\tspi received frame : value=0x%.8x",
		  frame_ret & 0xffffffff));

	return ERROR_NONE;
};
