/*
 *
 * Copyright (c) 2006-2007 Motorola, Inc, All Rights Reserved.
 *
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   *  Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   *  Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   *  Neither the name of Motorola nor the names of its contributors may
 *      be used to endorse or promote products derived from this software without
 *      specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 01/08/2006         Motorola         initial version
 * 02/26/2007         Motorola         Added PTP driver name
 * 01/09/2007         Motorola         Added PTP driver name(PictSync) 
 *
 * otg-node.h - OTG stack device names and nodes
 *
 */


#ifndef OTG_NODE_H /* To prevent inadvertently including a header twice */
#define OTG_NODE_H

#ifdef __cplusplus 
extern "C" {
#endif

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

#define ACM_DEVFS_NAME  "acm/"		/**< ACM device node DEVFS name if DEVFS enabled */
#define ACM_DRIVER_DEVFS_NAME "acm0"	/**< ACM TTY DEVFS name if DEVFS not enabled*/
#define ACM_DRIVER_PROCFS_NAME "tty_if" /**< ACM TTY PROCFS NAME */
#define ACM_TTY_MAJOR   166		/**< ACM device major number */
#define ACM_TTY_MINORS  1		/**< ACM device minor number */
#define ACM_TTY_MINOR_START 0		/**< ACM minor number start*/

#define NET_DEV_NAME 	"usbl0" 	/**< BLAN device name */ 
#define NET_PROCFS_ENTRY "network_ip"   /**< BLAN PROCFS ip adress */  
#define MSC_IO 		"msc_io"	/**< MSC proc interface */
#define MSC_DRIVER_NAME "msc-if"	/**< MSC driver name */

#define MTP_CTL_PROC_NAME "mtpctl"	/**< MTP proc interface */
#define MTP_DEV_NAME      "mtp"		/**< MTP device node */
#define PBG_DRIVER_NAME   "pbg-if"      /**< PBG driver name */
#define PTP_DRIVER_NAME   "ptp"      /**< PBG driver name */

#define OTG_MESSAGE	"otg_message"	/**< OTG state machine */

#define USB_FUNCTIONS   "usb-functions" /**< USB-FUNCTIONS proc entry */

#define ACM_AGENT	"acm_if"	/**< ACM Hotplug agent name*/
#define MSC_AGENT	"msc_if"	/**< MSC Hotplug agent name**/
#define MTP_AGENT	"mtp_if"	/**< MTP Hotplug agent name**/
#define PBG_AGENT	"pbg_if"	/**< PBG Hotplug agent name**/
#define PTP_AGENT	"ptp_if"	/**< PTP Hotplug agent name**/
#define USB_VENDOR_AGENT "usb_vendor" 	/**< USB Vendor Hotplug agent name**/

#ifdef __cplusplus
}
#endif

#endif /* OTG_NODE_H */
