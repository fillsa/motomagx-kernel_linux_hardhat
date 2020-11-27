/*
 *
 * @(#) balden@seth2.belcarratech.com|otg/ocd/zasevb/otg-config.h|20051116205001|02391
 *
 */
/*
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 06/12/2006         Motorola         OTG name change 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * tristate "Freescale ZAS Evaluation Board"
 * This implements On-The-Go USB Support for the ZAS EVB 
 */
#define CONFIG_OTG_ZASEVB

/*
 * bool 'Use the BaseBoard ISP1301 (S2.1 on S2.2 on)'
 * This will use the ISP1301 on the EVB Base Board.
 * Switch 2-1 should be on, switch 2-2 should be on.
 *
 * SW2.1 ON
 * SW2.2 ON
 */
#define CONFIG_OTG_ZASEVB_ISP1301


/*
 * bool 'Use the Altas Transceiver (check setting in help)'
 * This will use the Freescale MC13783 and the Freescale Connectivity driver. 
 * For BrassBoard works without any special setting
 *
 * For ZAS EVB with daughter MC13783 board, Switch 2-1 should be on, switch 2-2 
 * should be off on the ZAS EVB and switch 4-1 should be off, 4-2 should be on, 
 * 4-3 should be off, 4-4 should be on on MC13783 daughter board.
 *
 * SW2.1 ON
 * SW2.2 OFF
 */
#define CONFIG_OTG_ZASEVB_MC13783

/*
 * bool 'Use the RoadRunner Transceiver'
 * This will use the Freescale Roadrunner and the Freescale Connectivity driver. 
 */
#define CONFIG_OTG_ZASEVB_PMIC

/*
 * bool 'Differential Unidirectional (6 wire)'
 * Both the USBOTG HWMODE and Transceiver need to be configured
 * for the data transceiver connection. 
 *
 * This selects a Differential unidirectional 6 wire connection.
 * 
 * On MC13783 daughter board
 * SW4.1 = OFF
 * SW4.2 = OFF
 * SW4.3 = OFF
 * SW4.4 = ON
 *
 */
#define CONFIG_OTG_ZASEVB_DIFFERENTIAL_UNIDIRECTIONAL


/*
 * bool 'Differential Bidirectional (4 wire)'
 * Both the USBOTG HWMODE and Transceiver need to be configured
 * for the data transceiver connection. 
 *
 * This selects a Differential bidirectional 4 wire connection.
 */
#define CONFIG_OTG_ZASEVB_DIFFERENTIAL_BIDIRECTIONAL

/*
 * bool 'Singled Ended Unidirectional (6 wire)'
 * Both the USBOTG HWMODE and Transceiver need to be configured
 * for the data transceiver connection. 
 *
 * This selects a Single-Ended unidirectional 6 wire connection.
 */
#define CONFIG_OTG_ZASEVB_SINGLE_ENDED_UNIDIRECTIONAL

/*
 * bool 'Singled Ended Bidirectional (3 wire)'
 * Both the USBOTG HWMODE and Transceiver need to be configured
 * for the data transceiver connection.
 *
 * On MC13783 daughter board
 * SW4.1 = OFF
 * SW4.2 = OFF or ON (different from data sheet !!)
 * SW4.3 = OFF
 * SW4.4 = ON
 *
 * This selects a Single-Ended bidirectional 3 wire connection.
 */
#define CONFIG_OTG_ZASEVB_SINGLE_ENDED_BIDIRECTIONAL

/*
 * bool'Genral Purpose Timer'
 * Using the shared or non-shared general purpose timer
 */
#define CONFIG_OTG_GPTR

/*
 * bool 'High Resolution Timer'
 * Using high resolution timer, if exist
 */
#define CONFIG_OTG_HRT

/*
 * bool 
 * default OTG_ZASEVB
 */
#define CONFIG_OTG_PLATFORM_OTG OTG_ZASEVB

/*
 * bool 
 * default OTG_ZASEVB
 */
#define CONFIG_OTG_FREESCALE OTG_ZASEVB

/*
 * bool
 * default OTG_ZASEVB
 */
#define CONFIG_OTG_ISP1301 OTG_ZASEVB

/*
 * bool
 * default OTG_ZASEVB
 */
#define CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY OTG_ZASEVB

/*
 * bool
 * default
 */
#define CONFIG_OTG_ZASEVB_MC13783_POWERIC OTG_ZASEVB

