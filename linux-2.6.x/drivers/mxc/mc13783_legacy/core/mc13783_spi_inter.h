/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Date     Author    Comment
 * 10/2006  Motorola  Added support for power_ic drivers
 *                    Added SPI timing work-arounds for hardware issues
 */

#ifndef __SPI_INTERFACE_H__
#define __SPI_INTERFACE_H__

/*!
 * @file mc13783_spi_inter.h
 * @brief This file contains prototypes of SPI interface.
 *
 * @ingroup MC13783
 */

#define bool int

/*!
 * This function configures the SPI access.
 *
 * @return       This function returns 0 if successful.
 */
int spi_init(void);

/*!
 * This function is used to write on a mc13783 register.
 *
 * @param        num_reg   mc13783 register number
 * @param        reg_value  Register value
 *
 * @return       This function returns 0 if successful.
 */
int spi_write_reg(int num_reg, unsigned int *reg_value);

/*!
 * This function is used to read on a mc13783 register.
 *
 * @param        num_reg   mc13783 register number
 * @param        reg_value  Register value
 *
 * @return       This function returns 0 if successful.
 */
int spi_read_reg(int num_reg, unsigned int *reg_value);

/*!
 * This function is used to send a frame on SPI bus.
 *
 * @param        num_reg    mc13783 register number
 * @param        reg_value  Register value
 * @param        _rw       read or write operation
 *
 * @return       This function returns 0 if successful.
 */
int spi_send_frame_to_spi(int num_reg, unsigned int *reg_value, int rw);

#endif				/* __SPI_INTERFACE_H__ */
