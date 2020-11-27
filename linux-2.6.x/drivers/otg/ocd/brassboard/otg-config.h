/*
 * @(#) balden@seth2.belcarratech.com|otg/ocd/brassboard/otg-config.h|20051116204959|18048
 *
 * Copyright 2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 07/31/2006         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA
 *
 */

/*
 * tristate "Motorola Brassboard"
 * This implements On-The-Go USB Support for the SCMA11REF.
 * Ensure that P901 jumper is set to support USB.
 */
#define CONFIG_OTG_BRASSBOARD

/*
 * prompt "Select OTG Transceiver"	
 * bool 'BaseBoard ISP1104 (NOT IMPLEMENTED)'
 * This will use the Brassboard ISP1104 (ensure that DNP
 * resistors are populated and MC13783 DNP resistors are
 * not populated.)
 */
#define CONFIG_OTG_BRASSBOARD_ISP1104

/*
 * bool 'Altas Transceiver (Connectivity Driver)'
 * This will use the Freescale MC13783 chip and Freescale
 * connectivity driver (ensure that the board is properly
 * configured for MC13783 and not the ISP1104.)
 */
#define CONFIG_OTG_BRASSBOARD_MC13783


/*
 * bool 'Differential Unidirectional (6 wire)'
 * Both the USBOTG and Transceiver need to be configured
 * for the data transceiver connection. 
 * This selects a Differential Unidirectional 6 wire
 * connection.
 */
#define CONFIG_OTG_BRASSBOARD_DIFFERENTIAL_UNIDIRECTIONAL


/*
 * bool 'Differential Bidirectional (4 wire, STILL TESTING)'
 * Both the USBOTG and Transceiver need to be configured
 * for the data transceiver connection. 
 * This selects a Differential Bidirectional 4 wire
 * connection.
 */
#define CONFIG_OTG_BRASSBOARD_DIFFERENTIAL_BIDIRECTIONAL

/*
 * bool 'Singled Ended Unidirectional (6 wire, STILL TESTING)'
 * Both the USBOTG and Transceiver need to be configured
 * for the data transceiver connection. 
 * This selects a Single Ended Unidirectional 6 wire
 * connection.
 */
#define CONFIG_OTG_BRASSBOARD_SINGLE_ENDED_UNIDIRECTIONAL

/*
 * bool 'Singled Ended Bidirectional (3 wire)'
 * Both the USBOTG and Transceiver need to be configured
 * for the data transceiver connection. 
 * This selects a Single Ended Bidirectional 3 wire
 * connection.
 */
#define CONFIG_OTG_BRASSBOARD_SINGLE_ENDED_BIDIRECTIONAL


/*
 * bool 
 * default OTG_BRASSBOARD
 */
#define CONFIG_OTG_PLATFORM_OTG OTG_BRASSBOARD

/*
 * bool 
 * default OTG_BRASSBOARD
 */
#define CONFIG_OTG_FREESCALE OTG_BRASSBOARD


/*
 * bool
 * default OTG_BRASSBOARD
 */
#define CONFIG_OTG_ISP1104 OTG_BRASSBOARD

/*
 * bool
 * default OTG_BRASSBOARD
 */
#define CONFIG_OTG_BRASSBOARD_MC13783_CONNECTIVITY OTG_BRASSBOARD

/*
 * bool
 * default OTG_BRASSBOARD
 */
#define CONFIG_OTG_BRASSBOARD_MC13783_POWERIC OTG_BRASSBOARD

