/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

 /*!
  * @file dam_types.h
  * @brief This is the brief documentation for this dam_types.h file.
  *
  * This header file contains Digital Audio Multiplexer types.
  *
  * @ingroup DAM
  */

#ifndef DAM_TYPES_H_
#define DAM_TYPES_H_

/*!
 * This enumeration describes the Digital Audio Multiplexer mode.
 */
typedef enum {

	/*!
	 * Normal mode
	 */
	normal_mode = 0,

	/*!
	 * Internal network mode
	 */
	internal_network_mode = 1,

	/*!
	 * CE bus network mode
	 */
	CE_bus_network_mode = 2
} dam_mode;

/*!
 * This enumeration describes the port.
 */
typedef enum {

	/*!
	 * The port 1
	 */
	port_1 = 0,

	/*!
	 * The port 2
	 */
	port_2 = 1,

	/*!
	 * The port 3
	 */
	port_3 = 2,

	/*!
	 * The port 4
	 */
	port_4 = 3,

	/*!
	 * The port 5
	 */
	port_5 = 4,

	/*!
	 * The port 6
	 */
	port_6 = 5,

	/*!
	 * The port 7
	 */
	port_7 = 6
} dam_port;

/*!
 * This enumeration describes the bit mask for each port.
 */
typedef enum {

	/*!
	 * Port 1 bit mask
	 */
	port_1_mask = 0x01,

	/*!
	 * Port 2 bit mask
	 */
	port_2_mask = 0x02,

	/*!
	 * Port 3 bit mask
	 */
	port_3_mask = 0x04,

	/*!
	 * Port 4 bit mask
	 */
	port_4_mask = 0x08,

	/*!
	 * Port 5 bit mask
	 */
	port_5_mask = 0x10,

	/*!
	 * Port 6 bit mask
	 */
	port_6_mask = 0x20,

	/*!
	 * Port 7 bit mask
	 */
	port_7_mask = 0x40,
} dam_port_mask;

/*!
 * This enumeration describes the signal direction.
 */
typedef enum {

	/*!
	 * Signal In
	 */
	signal_in = 0,

	/*!
	 * Signal Out
	 */
	signal_out = 1
} signal_direction;

#endif
