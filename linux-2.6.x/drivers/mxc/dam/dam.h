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
  * @defgroup DAM Digital Audio Multiplexer (AUDMUX) Driver
  */

 /*!
  * @file dam.h
  * @brief This is the brief documentation for this dam.h file.
  *
  * This header file contains DAM driver functions prototypes.
  *
  * @ingroup DAM
  */

#ifndef DAM_H_
#define DAM_H_

#include "dam_types.h"

/*!
 * Test purpose definition
 */
#define TEST_DAM 1

#ifdef TEST_DAM

#define DAM_IOCTL 0x55
#define DAM_CONFIG_SSI1_MC13783 _IOWR(DAM_IOCTL, 1, int)
#define DAM_CONFIG_SSI2_MC13783 _IOWR(DAM_IOCTL, 2, int)
#define DAM_CONFIG_SSI_NETWORK_MODE_MC13783 _IOWR(DAM_IOCTL, 3, int)
#endif

/*!
 * This function selects the operation mode of the port.
 *
 * @param        port              the DAM port to configure
 * @param        the_mode          the operation mode of the port
 * @return       This function returns the result of the operation
 *               (0 if successful, -1 otherwise).
 */
int dam_select_mode(dam_port port, dam_mode the_mode);

/*!
 * This function controls Receive clock signal direction for the port.
 *
 * @param        port              the DAM port to configure
 * @param        direction         the Rx clock signal direction
 */
void dam_select_RxClk_direction(dam_port port, signal_direction direction);

/*!
 * This function controls Receive clock signal source for the port.
 *
 * @param        p_config          the DAM port to configure
 * @param        from_RxClk        the signal comes from RxClk or TxClk of
 *                                 the source port
 * @param        p_source          the source port
 */
void dam_select_RxClk_source(dam_port p_config, int from_RxClk,
			     dam_port p_source);

/*!
 * This function selects the source port for the RxD data.
 *
 * @param        p_config          the DAM port to configure
 * @param        p_source          the source port
 */
void dam_select_RxD_source(dam_port p_config, dam_port p_source);

/*!
 * This function controls Receive Frame Sync signal direction for the port.
 *
 * @param        port              the DAM port to configure
 * @param        direction         the Rx Frame Sync signal direction
 */
void dam_select_RxFS_direction(dam_port port, signal_direction direction);

/*!
 * This function controls Receive Frame Sync signal source for the port.
 *
 * @param        p_config          the DAM port to configure
 * @param        from_RxFS         the signal comes from RxFS or TxFS of
 *                                 the source port
 * @param        p_source          the source port
 */
void dam_select_RxFS_source(dam_port p_config, int from_RxFS,
			    dam_port p_source);

/*!
 * This function controls Transmit clock signal direction for the port.
 *
 * @param        port              the DAM port to configure
 * @param        direction         the Tx clock signal direction
 */
void dam_select_TxClk_direction(dam_port port, signal_direction direction);

/*!
 * This function controls Transmit clock signal source for the port.
 *
 * @param        p_config          the DAM port to configure
 * @param        from_RxClk        the signal comes from RxClk or TxClk of
 *                                 the source port
 * @param        p_source          the source port
 */
void dam_select_TxClk_source(dam_port p_config, int from_RxClk,
			     dam_port p_source);

/*!
 * This function controls Transmit Frame Sync signal direction for the port.
 *
 * @param        port              the DAM port to configure
 * @param        direction         the Tx Frame Sync signal direction
 */
void dam_select_TxFS_direction(dam_port port, signal_direction direction);

/*!
 * This function controls Transmit Frame Sync signal source for the port.
 *
 * @param        p_config          the DAM port to configure
 * @param        from_RxFS         the signal comes from RxFS or TxFS of
 *                                 the source port
 * @param        p_source          the source port
 */
void dam_select_TxFS_source(dam_port p_config, int from_RxFS,
			    dam_port p_source);

/*!
 * This function sets a bit mask that selects the port from which of
 * the RxD signals are to be ANDed together for internal network mode.
 * Bit 6 represents RxD from Port7 and bit0 represents RxD from Port1.
 * 1 excludes RxDn from ANDing. 0 includes RxDn for ANDing.
 *
 * @param        port              the DAM port to configure
 * @param        bit_mask          the bit mask
 * @return       This function returns the result of the operation
 *               (0 if successful, -1 otherwise).
 */
int dam_set_internal_network_mode_mask(dam_port port, unsigned char bit_mask);

/*!
 * This function controls whether or not the port is in synchronous mode.
 * When the synchronous mode is selected, the receive and the transmit sections
 * use common clock and frame sync signals.
 * When the synchronous mode is not selected, separate clock and frame sync
 * signals are used for the transmit and the receive sections.
 * The defaut value is the synchronous mode selected.
 *
 * @param        port              the DAM port to configure
 * @param        synchronous       the state to assign
 */
void dam_set_synchronous(dam_port port, int synchronous);

/*!
 * This function swaps the transmit and receive signals from (Da-TxD, Db-RxD) to
 * (Da-RxD, Db-TxD).
 * This default signal configuration is Da-TxD, Db-RxD.
 *
 * @param        port              the DAM port to configure
 * @param        value             the switch state
 */
void dam_switch_Tx_Rx(dam_port port, int value);

/*!
 * This function resets the two registers of the selected port.
 *
 * @param        port              the DAM port to reset
 */
void dam_reset_register(dam_port port);

#endif
