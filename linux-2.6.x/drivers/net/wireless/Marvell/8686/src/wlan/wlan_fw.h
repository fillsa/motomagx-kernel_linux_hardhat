/** @file wlan_fw.h
 *  @brief This header file contains FW interface related definitions.
 *
 * (c) Copyright © 2003-2006, Marvell International Ltd. 
 * 
 * This software file (the "File") is distributed by Marvell International 
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 * (the "License").  You may use, redistribute and/or modify this File in 
 * accordance with the terms and conditions of the License, a copy of which 
 * is available along with the File in the gpl.txt file or by writing to 
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 * this warranty disclaimer.
 *
 */
/*************************************************************
Change log:
	09/26/05: add Doxygen format comments 
 ************************************************************/

#ifndef _WLAN_FW_H_
#define _WLAN_FW_H_

#ifndef DEV_NAME_LEN
#define DEV_NAME_LEN            32
#endif

#define MAXKEYLEN           13

/* The number of times to try when waiting for downloaded firmware to 
 become active. (polling the scratch register). */

#define MAX_FIRMWARE_POLL_TRIES     100

#define FIRMWARE_TRANSFER_BLOCK_SIZE    1536

/** function prototypes */
int wlan_init_fw(wlan_private * priv);
int wlan_disable_host_int(wlan_private * priv, u8 reg);
int wlan_enable_host_int(wlan_private * priv, u8 mask);
int wlan_free_cmd_buffers(wlan_private * priv);

#endif /* _WLAN_FW_H_ */
