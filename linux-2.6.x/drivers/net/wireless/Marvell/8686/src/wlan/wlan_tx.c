/** @file wlan_tx.c
  * @brief This file contains the handling of TX in wlan
  * driver.
  *    
  *  (c) Copyright © 2003-2006, Marvell International Ltd.  
  *   
  *  This software file (the "File") is distributed by Marvell International 
  *  Ltd. under the terms of the GNU General Public License Version 2, June 1991 
  *  (the "License").  You may use, redistribute and/or modify this File in 
  *  accordance with the terms and conditions of the License, a copy of which 
  *  is available along with the File in the gpl.txt file or by writing to 
  *  the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
  *  02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
  *
  *  THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
  *  ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
  *  this warranty disclaimer.
  *
  */
/********************************************************
Change log:
	09/28/05: Add Doxygen format comments
	12/13/05: Add Proprietary periodic sleep support
	01/05/06: Add kernel 2.6.x support	
	04/06/06: Add TSPEC, queue metrics, and MSDU expiry support
********************************************************/

#include	"include.h"

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/
static int
timeval_diff_in_ms(const struct timeval *pTv1, const struct timeval *pTv2)
{
    int diff_ms;

    diff_ms = (pTv1->tv_sec - pTv2->tv_sec) * 1000;
    diff_ms += (pTv1->tv_usec - pTv2->tv_usec) / 1000;

    return diff_ms;
}

/** 
 *  @brief This function processes a single packet and sends
 *  to IF layer
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param skb     A pointer to skb which includes TX packet
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
SendSinglePacket(wlan_private * priv, struct sk_buff *skb)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    TxPD LocalTxPD;
    TxPD *pLocalTxPD = &LocalTxPD;
    u8 *ptr = priv->adapter->TmpTxBuf;
    struct timeval in_tv;
    struct timeval out_tv;
    int queue_delay;

    ENTER();

    if (!skb->len || (skb->len > MRVDRV_ETH_TX_PACKET_BUFFER_SIZE)) {
        PRINTM(ERROR, "Tx Error: Bad skb length %d : %d\n",
               skb->len, MRVDRV_ETH_TX_PACKET_BUFFER_SIZE);
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    memset(pLocalTxPD, 0, sizeof(TxPD));

    pLocalTxPD->TxPacketLength = skb->len;

    if (Adapter->wmm.enabled) {
        /* 
         * original skb->priority has been overwritten 
         * by wmm_map_and_add_skb()
         */
        pLocalTxPD->Priority = (u8) skb->priority;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)
        skb_get_timestamp(skb, &in_tv);
#else
        memcpy(&in_tv, &skb->stamp, sizeof(in_tv));
#endif
        do_gettimeofday(&out_tv);

        /* Queue delay is passed as a uint8 in units of 2ms (ms shifted by 1).
         *   Min value (other than 0) is therefore 2ms, max is 510ms.
         */
        queue_delay = timeval_diff_in_ms(&out_tv, &in_tv) >> 1;

        /* Pass max value if queue_delay is beyond the uint8 range */
        pLocalTxPD->PktDelay_2ms = MIN(queue_delay, 0xFF);

        PRINTM(INFO, "WMM: Pkt Delay: %d ms\n",
               pLocalTxPD->PktDelay_2ms << 1);
    }
    if (Adapter->PSState != PS_STATE_FULL_POWER) {
        if (TRUE == CheckLastPacketIndication(priv)) {
            Adapter->TxLockFlag = TRUE;
            pLocalTxPD->PowerMgmt = MRVDRV_TxPD_POWER_MGMT_LAST_PACKET;
        }
    }
    /* offset of actual data */
    pLocalTxPD->TxPacketLocation = sizeof(TxPD);

    /* TxCtrl set by user or default */
    pLocalTxPD->TxControl = Adapter->PktTxCtrl;

    memcpy((u8 *) pLocalTxPD->TxDestAddr, skb->data, MRVDRV_ETH_ADDR_LEN);

    ptr += SDIO_HEADER_LEN;
    memcpy(ptr, pLocalTxPD, sizeof(TxPD));

    ptr += sizeof(TxPD);

    memcpy(ptr, skb->data, skb->len);

    ret = sbi_host_to_card(priv, MVMS_DAT,
                           priv->adapter->TmpTxBuf, skb->len + sizeof(TxPD));

    if (ret) {
        PRINTM(ERROR, "Tx Error: sbi_host_to_card failed: 0x%X\n", ret);
        Adapter->dbg.num_tx_host_to_card_failure++;
        goto done;
    }

    PRINTM(DATA, "Data => FW\n");
    DBG_HEXDUMP(DAT_D, "Tx", ptr - sizeof(TxPD),
                MIN(skb->len + sizeof(TxPD), MAX_DATA_DUMP_LEN));

  done:
    if (!ret) {
        priv->stats.tx_packets++;
        priv->stats.tx_bytes += skb->len;
    } else {
        priv->stats.tx_dropped++;
        priv->stats.tx_errors++;
        os_start_queue(priv);
    }

    /* need to be freed in all cases */
    os_free_tx_packet(priv);

    LEAVE();
    return ret;
}

/********************************************************
		Global functions
********************************************************/

/** 
 *  @brief This function checks the conditions and sends packet to IF
 *  layer if everything is ok.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   n/a
 */
void
wlan_process_tx(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    OS_INTERRUPT_SAVE_AREA;

    ENTER();

    if (priv->wlan_dev.dnld_sent) {
        PRINTM(MSG, "TX Error: dnld_sent = %d, not sending\n",
               priv->wlan_dev.dnld_sent);
        goto done;
    }

    SendSinglePacket(priv, Adapter->CurrentTxSkb);

    OS_INT_DISABLE;
    priv->adapter->HisRegCpy &= ~HIS_TxDnLdRdy;
    OS_INT_RESTORE;

  done:
    LEAVE();
}

/** 
 *  @brief This function queues the packet received from
 *  kernel/upper layer and wake up the main thread to handle it.
 *  
 *  @param priv    A pointer to wlan_private structure
  * @param skb     A pointer to skb which includes TX packet
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_tx_packet(wlan_private * priv, struct sk_buff *skb)
{
    ulong flags;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    HEXDUMP("TX Data", skb->data, MIN(skb->len, 100));

    spin_lock_irqsave(&Adapter->CurrentTxLock, flags);

    if (Adapter->wmm.enabled) {
        wmm_map_and_add_skb(priv, skb);
#ifdef WPRM_DRV
        WPRM_DRV_TRACING_PRINT();
        /* increase traffic meter tx counter 
           and call measurement function to see 
           if we need to change FW power mode. */
        wprm_tx_packet_cnt++;
        wprm_traffic_measurement(priv, Adapter, TRUE);
#endif

        wake_up_interruptible(&priv->MainThread.waitQ);
    } else {
        Adapter->TxSkbNum++;
        list_add_tail((struct list_head *) skb,
                      (struct list_head *) &priv->adapter->TxSkbQ);
#ifdef WPRM_DRV
        WPRM_DRV_TRACING_PRINT();
        /* increase traffic meter tx counter 
           and call measurement function to see 
           if we need to change FW power mode. */
        wprm_tx_packet_cnt++;
        wprm_traffic_measurement(priv, Adapter, TRUE);
#endif

        wake_up_interruptible(&priv->MainThread.waitQ);
    }
    spin_unlock_irqrestore(&Adapter->CurrentTxLock, flags);

    LEAVE();

    return ret;
}

/** 
 *  @brief This function tells firmware to send a NULL data packet.
 *  
 *  @param priv     A pointer to wlan_private structure
 *  @param pwr_mgmt indicate if power management bit should be 0 or 1
 *  @return 	    n/a
 */
int
SendNullPacket(wlan_private * priv, u8 pwr_mgmt)
{
    wlan_adapter *Adapter = priv->adapter;
    TxPD txpd = { 0 };
    int ret = WLAN_STATUS_SUCCESS;
    u8 *ptr = priv->adapter->TmpTxBuf;

    ENTER();

    if (priv->adapter->SurpriseRemoved == TRUE) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    if (priv->adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    memset(&txpd, 0, sizeof(TxPD));

    txpd.TxControl = Adapter->PktTxCtrl;
    txpd.PowerMgmt = pwr_mgmt;
    txpd.TxPacketLocation = sizeof(TxPD);

    ptr += SDIO_HEADER_LEN;
    memcpy(ptr, &txpd, sizeof(TxPD));

    ret = sbi_host_to_card(priv, MVMS_DAT,
                           priv->adapter->TmpTxBuf, sizeof(TxPD));

    if (ret != 0) {
        PRINTM(ERROR, "TX Error: SendNullPacket failed!\n");
        Adapter->dbg.num_tx_host_to_card_failure++;
        goto done;
    }
    PRINTM(DATA, "Null data => FW\n");
    DBG_HEXDUMP(DAT_D, "Tx", ptr, sizeof(TxPD));

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function check if we need send last packet indication.
 *  
 *  @param priv     A pointer to wlan_private structure
 *
 *  @return 	   TRUE or FALSE
 */
BOOLEAN
CheckLastPacketIndication(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    BOOLEAN ret = FALSE;
    BOOLEAN prop_ps = TRUE;

    ENTER();

    if (Adapter->sleep_period.period == 0 || Adapter->gen_null_pkg == FALSE     /* for UPSD certification tests */
        )
        goto done;
    if (Adapter->wmm.enabled) {
        if (wmm_lists_empty(priv)) {
            if (((Adapter->CurBssParams.wmm_uapsd_enabled == TRUE) &&
                 (Adapter->wmm.qosinfo != 0)) || prop_ps)
                ret = TRUE;
        }
        goto done;
    }
    if (!Adapter->TxSkbNum)
        ret = TRUE;
  done:

    LEAVE();
    return ret;
}
