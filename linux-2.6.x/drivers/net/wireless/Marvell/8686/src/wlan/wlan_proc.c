/** @file wlan_proc.c
  * @brief This file contains functions for proc fin proc file.
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
/********************************************************
Change log:
	10/04/05: Add Doxygen format comments
	01/05/06: Add kernel 2.6.x support	
	
********************************************************/
#include 	"include.h"

/********************************************************
		Local Variables
********************************************************/

static char *szModes[] = {
    "Ad-hoc",
    "Managed",
    "Auto",
    "Unknown"
};

static char *szStates[] = {
    "Disconnected",
    "Connected"
};

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief proc read function
 *
 *  @param page	   pointer to buffer
 *  @param start   read data starting position
 *  @param offset  offset
 *  @param count   counter 
 *  @param eof     end of file flag
 *  @param data    data to output
 *  @return 	   number of output data
 */
static int
wlan_proc_read(char *page, char **start, off_t offset,
               int count, int *eof, void *data)
{
#ifdef CONFIG_PROC_FS
    int i;
    char *p = page;
    struct net_device *netdev = data;
    struct dev_mc_list *mcptr = netdev->mc_list;
    char fmt[64];
    wlan_private *priv = netdev->priv;
    wlan_adapter *Adapter = priv->adapter;
    ulong flags;
#endif

    if (offset != 0) {
        *eof = 1;
        goto exit;
    }

    get_version(Adapter, fmt, sizeof(fmt) - 1);

    p += sprintf(p, "driver_name = " "\"wlan\"\n");
    p += sprintf(p, "driver_version = %s", fmt);
    p += sprintf(p, "\nInterfaceName=\"%s\"\n", netdev->name);
    p += sprintf(p, "Mode=\"%s\"\n", szModes[Adapter->InfrastructureMode]);
    p += sprintf(p, "State=\"%s\"\n", szStates[Adapter->MediaConnectStatus]);
    p += sprintf(p, "MACAddress=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",
                 netdev->dev_addr[0], netdev->dev_addr[1],
                 netdev->dev_addr[2], netdev->dev_addr[3],
                 netdev->dev_addr[4], netdev->dev_addr[5]);

    p += sprintf(p, "MCCount=\"%d\"\n", netdev->mc_count);
    p += sprintf(p, "ESSID=\"%s\"\n", (u8 *) Adapter->CurBssParams.ssid.Ssid);
    p += sprintf(p, "Channel=\"%d\"\n", Adapter->CurBssParams.channel);
    p += sprintf(p, "region_code = \"%02x\"\n", (u32) Adapter->RegionCode);

    /*
     * Put out the multicast list 
     */
    for (i = 0; i < netdev->mc_count; i++) {
        p += sprintf(p,
                     "MCAddr[%d]=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",
                     i,
                     mcptr->dmi_addr[0], mcptr->dmi_addr[1],
                     mcptr->dmi_addr[2], mcptr->dmi_addr[3],
                     mcptr->dmi_addr[4], mcptr->dmi_addr[5]);

        mcptr = mcptr->next;
    }
    p += sprintf(p, "num_tx_bytes = %lu\n", priv->stats.tx_bytes);
    p += sprintf(p, "num_rx_bytes = %lu\n", priv->stats.rx_bytes);
    p += sprintf(p, "num_tx_pkts = %lu\n", priv->stats.tx_packets);
    p += sprintf(p, "num_rx_pkts = %lu\n", priv->stats.rx_packets);
    p += sprintf(p, "num_tx_pkts_dropped = %lu\n", priv->stats.tx_dropped);
    p += sprintf(p, "num_rx_pkts_dropped = %lu\n", priv->stats.rx_dropped);
    p += sprintf(p, "num_tx_pkts_err = %lu\n", priv->stats.tx_errors);
    p += sprintf(p, "num_rx_pkts_err = %lu\n", priv->stats.rx_errors);
    p += sprintf(p, "carrier %s\n",
                 ((netif_carrier_ok(priv->wlan_dev.netdev)) ? "on" : "off"));
    p += sprintf(p, "tx queue %s\n",
                 ((netif_queue_stopped(priv->wlan_dev.netdev)) ? "stopped" :
                  "started"));

    spin_lock_irqsave(&Adapter->QueueSpinLock, flags);
    if (Adapter->CurCmd) {
        HostCmd_DS_COMMAND *CmdPtr =
            (HostCmd_DS_COMMAND *) Adapter->CurCmd->BufVirtualAddr;
        p += sprintf(p, "CurCmd ID = 0x%x, 0x%x\n",
                     wlan_cpu_to_le16(CmdPtr->Command),
                     wlan_cpu_to_le16(*(u16 *) ((u8 *) CmdPtr + S_DS_GEN)));
    } else {
        p += sprintf(p, "CurCmd NULL\n");
    }
    spin_unlock_irqrestore(&Adapter->QueueSpinLock, flags);
#ifdef WPRM_DRV
    /*
     * Put out the Motorola Power Management parameters. 
     */
    p += sprintf(p, "Adapter->bWakeupDevRequired = %d.\n",
                 priv->adapter->bWakeupDevRequired);

    p += sprintf(p, "Adapter->PSState = %d.\n", priv->adapter->PSState);
    p += sprintf(p, "Adapter->PSMode = %d.\n", priv->adapter->PSMode);
    p += sprintf(p, "Adapter->NeedToWakeup = %d.\n",
                 priv->adapter->NeedToWakeup);

    p += sprintf(p, "WPrM Traffic Meter:\n");
    p += sprintf(p, "wprm_tm_ps_cmd_in_list = %d.\n", wprm_tm_ps_cmd_in_list);
    p += sprintf(p, "wprm_tm_ps_cmd_no = %d.\n", wprm_tm_ps_cmd_no);
    p += sprintf(p, "is_wprm_traffic_meter_timer_set =%d.\n",
                 is_wprm_traffic_meter_timer_set);

    p += sprintf(p, "UAPSD related:\n");
    p += sprintf(p, "Voice session Status = %d.\n", voicesession_status);
    p += sprintf(p, "Sleep Period  = %d.\n",
                 priv->adapter->sleep_period.period);
    p += sprintf(p, "CurBssParams.wmm_uapsd_enabled =%d.\n",
                 priv->adapter->CurBssParams.wmm_uapsd_enabled);
#endif

  exit:
    return (p - page);
}

#define GEN_FATAL_ERROR_EVT	1
/** 
 *  @brief genevt proc write function
 *
 *  @param f	   file pointer
 *  @param buf     pointer to data buffer
 *  @param cnt     data number to write
 *  @param data    data to write
 *  @return 	   number of data
 */
static int
wlan_genevt_write(struct file *f, const char *buf, unsigned long cnt,
                  void *data)
{
    struct net_device *netdev = data;
    wlan_private *priv = netdev->priv;
    char databuf[10];
    int genevt;
    MODULE_GET;
    if (cnt > 10) {
        MODULE_PUT;
        return cnt;
    }
    if (copy_from_user(databuf, buf, cnt)) {
        MODULE_PUT;
        return 0;
    }
    genevt = string_to_number(databuf);
    switch (genevt) {
    case GEN_FATAL_ERROR_EVT:
        PRINTM(MSG, "proc: call wlan_fatal_error_handle\n");
        wlan_fatal_error_handle(priv);
        break;
    default:
        break;
    }
    MODULE_PUT;
    return cnt;
}

/********************************************************
		Global Functions
********************************************************/

/** 
 *  @brief create wlan proc file
 *
 *  @param priv	   pointer wlan_private
 *  @param dev     pointer net_device
 *  @return 	   N/A
 */
void
wlan_proc_entry(wlan_private * priv, struct net_device *dev)
{

#ifdef	CONFIG_PROC_FS
    struct proc_dir_entry *r;
    PRINTM(INFO, "Creating Proc Interface\n");

    if (!priv->proc_entry) {
        priv->proc_entry = proc_mkdir("wlan", proc_net);

        if (priv->proc_entry) {
            priv->proc_dev = create_proc_read_entry
                ("info", 0, priv->proc_entry, wlan_proc_read, dev);
            r = create_proc_entry("genevt", 0644, priv->proc_entry);
            if (r) {
                r->data = dev;
                r->write_proc = wlan_genevt_write;
                r->owner = THIS_MODULE;
            } else
                PRINTM(MSG, "Fail to create proc genevt\n");
        }
    }
#endif
}

/** 
 *  @brief remove proc file
 *
 *  @param priv	   pointer wlan_private
 *  @return 	   N/A
 */
void
wlan_proc_remove(wlan_private * priv)
{
#ifdef CONFIG_PROC_FS
    if (priv->proc_entry) {
        remove_proc_entry("info", priv->proc_entry);
        remove_proc_entry("genevt", priv->proc_entry);
    }
    remove_proc_entry("wlan", proc_net);
#endif
}
