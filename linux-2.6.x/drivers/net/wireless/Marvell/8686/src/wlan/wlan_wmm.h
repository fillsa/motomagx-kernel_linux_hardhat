/** @file wlan_wmm.h
 * @brief This file contains related macros, enum, and struct
 * of wmm functionalities
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
/****************************************************
Change log:
    09/26/05: add Doxygen format comments 
    04/06/06: Add TSPEC, queue metrics, and MSDU expiry support
****************************************************/

#ifndef __WLAN_WMM_H
#define __WLAN_WMM_H

#define WMM_IE_LENGTH                 0x0009
#define WMM_PARA_IE_LENGTH            0x0018
#define WMM_QOS_INFO_OFFSET           0x08
#define WMM_QOS_INFO_UAPSD_BIT        0x80
#define WMM_OUISUBTYPE_IE             0x00
#define WMM_OUISUBTYPE_PARA           0x01
#define WMM_TXOP_LIMIT_UNITS_SHIFT    5

#define WMM_CONFIG_CHANGE_INDICATION  "WMM_CONFIG_CHANGE.indication"

/** Size of a TSPEC.  Used to allocate necessary buffer space in commands */
#define WMM_TSPEC_SIZE              63

/** Extra IE bytes allocated in messages for appended IEs after a TSPEC */
#define WMM_ADDTS_EXTRA_IE_BYTES    256

/** Extra TLV bytes allocated in messages for configuring WMM Queues */
#define WMM_QUEUE_CONFIG_EXTRA_TLV_BYTES 64

#ifdef __KERNEL__
/** struct of WMM DESC */
typedef struct
{
    u8 required;
    u8 enabled;
    u8 fw_notify;
    struct sk_buff TxSkbQ[MAX_AC_QUEUES];
    u8 Para_IE[WMM_PARA_IE_LENGTH];
    u8 qosinfo;
    WMM_AC_STATUS acStatus[MAX_AC_QUEUES];
} __ATTRIB_PACK__ WMM_DESC;

extern void wmm_map_and_add_skb(wlan_private * priv, struct sk_buff *);
#endif

extern int sendWMMStatusChangeCmd(wlan_private * priv);
extern int wmm_lists_empty(wlan_private * priv);
extern void wmm_cleanup_queues(wlan_private * priv);
extern void wmm_process_tx(wlan_private * priv);

extern u32 wlan_wmm_process_association_req(wlan_private * priv,
                                            u8 ** ppAssocBuf,
                                            WMM_PARAMETER_IE * pWmmIE);

/* 
 *  Functions used in the cmd handling routine
 */
extern int wlan_cmd_wmm_ack_policy(wlan_private * priv,
                                   HostCmd_DS_COMMAND * cmd, void *InfoBuf);
extern int wlan_cmd_wmm_get_status(wlan_private * priv,
                                   HostCmd_DS_COMMAND * cmd, void *InfoBuf);
extern int wlan_cmd_wmm_addts_req(wlan_private * priv,
                                  HostCmd_DS_COMMAND * cmd, void *InfoBuf);
extern int wlan_cmd_wmm_delts_req(wlan_private * priv,
                                  HostCmd_DS_COMMAND * cmd, void *InfoBuf);
extern int wlan_cmd_wmm_queue_config(wlan_private * priv,
                                     HostCmd_DS_COMMAND * cmd, void *InfoBuf);
extern int wlan_cmd_wmm_queue_stats(wlan_private * priv,
                                    HostCmd_DS_COMMAND * cmd, void *InfoBuf);

/* 
 *  Functions used in the cmdresp handling routine
 */
extern int wlan_cmdresp_wmm_ack_policy(wlan_private * priv,
                                       const HostCmd_DS_COMMAND * resp);
extern int wlan_cmdresp_wmm_get_status(wlan_private * priv,
                                       const HostCmd_DS_COMMAND * resp);
extern int wlan_cmdresp_wmm_addts_req(wlan_private * priv,
                                      const HostCmd_DS_COMMAND * resp);
extern int wlan_cmdresp_wmm_delts_req(wlan_private * priv,
                                      const HostCmd_DS_COMMAND * resp);
extern int wlan_cmdresp_wmm_queue_config(wlan_private * priv,
                                         const HostCmd_DS_COMMAND * resp);
extern int wlan_cmdresp_wmm_queue_stats(wlan_private * priv,
                                        const HostCmd_DS_COMMAND * resp);

/* 
 * IOCTLs 
 */
extern int wlan_wmm_ack_policy_ioctl(wlan_private * priv, struct ifreq *req);
extern int wlan_wmm_para_ie_ioctl(wlan_private * priv, struct ifreq *req);
extern int wlan_wmm_enable_ioctl(wlan_private * priv, struct iwreq *wrq);
extern int wlan_wmm_queue_status_ioctl(wlan_private * priv,
                                       struct iwreq *wrq);

extern int wlan_wmm_addts_req_ioctl(wlan_private * priv, struct iwreq *wrq);
extern int wlan_wmm_delts_req_ioctl(wlan_private * priv, struct iwreq *wrq);
extern int wlan_wmm_queue_config_ioctl(wlan_private * priv,
                                       struct iwreq *wrq);
extern int wlan_wmm_queue_stats_ioctl(wlan_private * priv, struct iwreq *wrq);
#endif /* __WLAN_WMM_H */
