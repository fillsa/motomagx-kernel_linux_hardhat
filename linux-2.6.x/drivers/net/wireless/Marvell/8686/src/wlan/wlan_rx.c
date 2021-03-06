/** @file wlan_rx.c
  * @brief This file contains the handling of RX in wlan
  * driver.
  * 
  * (c) Copyright ? 2003-2006, Marvell International Ltd.  
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
	09/28/05: Add Doxygen format comments
	12/09/05: ADD Sliding window SNR/NF Average Calculation support
	
********************************************************/

#include	"include.h"

/********************************************************
		Local Variables
********************************************************/

typedef struct
{
    u8 dest_addr[6];
    u8 src_addr[6];
    u16 h803_len;

} __ATTRIB_PACK__ Eth803Hdr_t;

typedef struct
{
    u8 llc_dsap;
    u8 llc_ssap;
    u8 llc_ctrl;
    u8 snap_oui[3];
    u16 snap_type;

} __ATTRIB_PACK__ Rfc1042Hdr_t;

typedef struct
{
    RxPD rx_pd;
    Eth803Hdr_t eth803_hdr;
    Rfc1042Hdr_t rfc1042_hdr;

} __ATTRIB_PACK__ RxPacketHdr_t;

typedef struct
{
    u8 dest_addr[6];
    u8 src_addr[6];
    u16 ethertype;

} __ATTRIB_PACK__ EthII_Hdr_t;

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief This function computes the AvgSNR .
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   AvgSNR
 */
static u8
wlan_getAvgSNR(wlan_private * priv)
{
    u8 i;
    u16 temp = 0;
    wlan_adapter *Adapter = priv->adapter;
    if (Adapter->numSNRNF == 0)
        return 0;
    for (i = 0; i < Adapter->numSNRNF; i++)
        temp += Adapter->rawSNR[i];
    return (u8) (temp / Adapter->numSNRNF);

}

/** 
 *  @brief This function computes the AvgNF
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   AvgNF
 */
static u8
wlan_getAvgNF(wlan_private * priv)
{
    u8 i;
    u16 temp = 0;
    wlan_adapter *Adapter = priv->adapter;
    if (Adapter->numSNRNF == 0)
        return 0;
    for (i = 0; i < Adapter->numSNRNF; i++)
        temp += Adapter->rawNF[i];
    return (u8) (temp / Adapter->numSNRNF);

}

/** 
 *  @brief This function save the raw SNR/NF to our internel buffer
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param pRxPD   A pointer to RxPD structure of received packet
 *  @return 	   n/a
 */
static void
wlan_save_rawSNRNF(wlan_private * priv, RxPD * pRxPD)
{
    wlan_adapter *Adapter = priv->adapter;
    if (Adapter->numSNRNF < Adapter->data_avg_factor)
        Adapter->numSNRNF++;
    Adapter->rawSNR[Adapter->nextSNRNF] = pRxPD->SNR;
    Adapter->rawNF[Adapter->nextSNRNF] = pRxPD->NF;
    Adapter->nextSNRNF++;
    if (Adapter->nextSNRNF >= Adapter->data_avg_factor)
        Adapter->nextSNRNF = 0;
    return;
}

#define DATA_RSSI_LOW_BIT		0x01
#define DATA_SNR_LOW_BIT		0x02
#define DATA_RSSI_HIGH_BIT		0x04
#define DATA_SNR_HIGH_BIT		0x08
/** 
 *  @brief This function computes the RSSI in received packet.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   n/a
 */
static void
wlan_check_subscribe_event(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int temp;
    if (Adapter->subevent.EventsBitmap == 0)
        return;
    if ((Adapter->subevent.EventsBitmap & DATA_RSSI_LOW_BIT) ||
        (Adapter->subevent.EventsBitmap & DATA_RSSI_HIGH_BIT)) {
        temp =
            -CAL_RSSI(Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE,
                      Adapter->NF[TYPE_RXPD][TYPE_AVG] / AVG_SCALE);
        if (Adapter->subevent.EventsBitmap & DATA_RSSI_LOW_BIT) {
            if (temp > Adapter->subevent.Rssi_low.value) {
                if (!Adapter->subevent.Rssi_low.Freq)
                    Adapter->subevent.EventsBitmap &= ~DATA_RSSI_LOW_BIT;
                if (Adapter->subevent.Rssi_low.Freq > 1) {
                    Adapter->subevent.Rssi_low.Freq--;
                    if (Adapter->subevent.Rssi_low.Freq == 1)
                        Adapter->subevent.Rssi_low.Freq = 0;
                }
                send_iwevcustom_event(priv, CUS_EVT_DATA_RSSI_LOW);
            }
        }
        if (Adapter->subevent.EventsBitmap & DATA_RSSI_HIGH_BIT) {
            if (temp < Adapter->subevent.Rssi_high.value) {
                if (!Adapter->subevent.Rssi_high.Freq)
                    Adapter->subevent.EventsBitmap &= ~DATA_RSSI_HIGH_BIT;
                if (Adapter->subevent.Rssi_high.Freq > 1) {
                    Adapter->subevent.Rssi_high.Freq--;
                    if (Adapter->subevent.Rssi_high.Freq == 1)
                        Adapter->subevent.Rssi_high.Freq = 0;
                }
                send_iwevcustom_event(priv, CUS_EVT_DATA_RSSI_HIGH);
            }
        }
    }
    if ((Adapter->subevent.EventsBitmap & DATA_SNR_LOW_BIT) ||
        (Adapter->subevent.EventsBitmap & DATA_SNR_HIGH_BIT)) {
        temp = Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE;
        if (Adapter->subevent.EventsBitmap & DATA_SNR_LOW_BIT) {
            if (temp < Adapter->subevent.Snr_low.value) {
                if (!Adapter->subevent.Snr_low.Freq)
                    Adapter->subevent.EventsBitmap &= ~DATA_SNR_LOW_BIT;
                if (Adapter->subevent.Snr_low.Freq > 1) {
                    Adapter->subevent.Snr_low.Freq--;
                    if (Adapter->subevent.Snr_low.Freq == 1)
                        Adapter->subevent.Snr_low.Freq = 0;
                }
                send_iwevcustom_event(priv, CUS_EVT_DATA_SNR_LOW);
            }
        }
        if (Adapter->subevent.EventsBitmap & DATA_SNR_HIGH_BIT) {
            if (temp > Adapter->subevent.Snr_high.value) {
                if (!Adapter->subevent.Snr_high.Freq)
                    Adapter->subevent.EventsBitmap &= ~DATA_SNR_HIGH_BIT;
                if (Adapter->subevent.Snr_high.Freq > 1) {
                    Adapter->subevent.Snr_high.Freq--;
                    if (Adapter->subevent.Snr_high.Freq == 1)
                        Adapter->subevent.Snr_high.Freq = 0;
                }
                send_iwevcustom_event(priv, CUS_EVT_DATA_SNR_HIGH);
            }
        }
    }
    return;
}

/** 
 *  @brief This function computes the RSSI in received packet.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param pRxPD   A pointer to RxPD structure of received packet
 *  @return 	   n/a
 */
static void
wlan_compute_rssi(wlan_private * priv, RxPD * pRxPD)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    PRINTM(INFO, "RxPD: SNR = %d, NF = %d\n", pRxPD->SNR, pRxPD->NF);

    Adapter->SNR[TYPE_RXPD][TYPE_NOAVG] = pRxPD->SNR;
    Adapter->NF[TYPE_RXPD][TYPE_NOAVG] = pRxPD->NF;
    wlan_save_rawSNRNF(priv, pRxPD);

    Adapter->RxPDAge = os_time_get();
    Adapter->RxPDRate = pRxPD->RxRate;

    Adapter->SNR[TYPE_RXPD][TYPE_AVG] = wlan_getAvgSNR(priv) * AVG_SCALE;
    Adapter->NF[TYPE_RXPD][TYPE_AVG] = wlan_getAvgNF(priv) * AVG_SCALE;
    PRINTM(INFO, "SNR-avg = %d, NF-avg = %d\n",
           Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE,
           Adapter->NF[TYPE_RXPD][TYPE_AVG] / AVG_SCALE);

    Adapter->RSSI[TYPE_RXPD][TYPE_NOAVG] =
        CAL_RSSI(Adapter->SNR[TYPE_RXPD][TYPE_NOAVG],
                 Adapter->NF[TYPE_RXPD][TYPE_NOAVG]);

    Adapter->RSSI[TYPE_RXPD][TYPE_AVG] =
        CAL_RSSI(Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE,
                 Adapter->NF[TYPE_RXPD][TYPE_AVG] / AVG_SCALE);
    wlan_check_subscribe_event(priv);
    LEAVE();
}

/********************************************************
		Global functions
********************************************************/

/**
 *  @brief This function processes received packet and forwards it
 *  to kernel/upper layer
 *  
 *  @param priv    A pointer to wlan_private
 *  @param skb     A pointer to skb which includes the received packet
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
ProcessRxedPacket(wlan_private * priv, struct sk_buff *skb)
{
#ifdef WPRM_DRV
    wlan_adapter *Adapter = priv->adapter;
#endif
    int ret = WLAN_STATUS_SUCCESS;

    RxPacketHdr_t *pRxPkt;
    RxPD *pRxPD;

    int hdrChop;
    EthII_Hdr_t *pEthHdr;
    u32 u32SkbLen = skb->len;

    const u8 rfc1042_eth_hdr[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

    ENTER();

    pRxPkt = (RxPacketHdr_t *) skb->data;
    pRxPD = &pRxPkt->rx_pd;

    DBG_HEXDUMP(DAT_D, "Rx", skb->data, MIN(skb->len, MAX_DATA_DUMP_LEN));

    if (skb->len < (ETH_HLEN + 8 + sizeof(RxPD))) {
        PRINTM(ERROR, "RX Error: FRAME RECEIVED WITH BAD LENGTH\n");
        priv->stats.rx_length_errors++;
        ret = WLAN_STATUS_SUCCESS;
        kfree_skb(skb);
        goto done;
    }

    PRINTM(INFO, "RX Data: skb->len - sizeof(RxPd) = %d - %d = %d\n",
           skb->len, sizeof(RxPD), skb->len - sizeof(RxPD));

    HEXDUMP("RX Data: Dest", pRxPkt->eth803_hdr.dest_addr,
            sizeof(pRxPkt->eth803_hdr.dest_addr));
    HEXDUMP("RX Data: Src", pRxPkt->eth803_hdr.src_addr,
            sizeof(pRxPkt->eth803_hdr.src_addr));

    if (memcmp(&pRxPkt->rfc1042_hdr,
               rfc1042_eth_hdr, sizeof(rfc1042_eth_hdr)) == 0) {
        /* 
         *  Replace the 803 header and rfc1042 header (llc/snap) with an 
         *    EthernetII header, keep the src/dst and snap_type (ethertype)
         *
         *  The firmware only passes up SNAP frames converting
         *    all RX Data from 802.11 to 802.2/LLC/SNAP frames.
         *
         *  To create the Ethernet II, just move the src, dst address right
         *    before the snap_type.
         */
        pEthHdr = (EthII_Hdr_t *)
            ((u8 *) & pRxPkt->eth803_hdr
             + sizeof(pRxPkt->eth803_hdr) + sizeof(pRxPkt->rfc1042_hdr)
             - sizeof(pRxPkt->eth803_hdr.dest_addr)
             - sizeof(pRxPkt->eth803_hdr.src_addr)
             - sizeof(pRxPkt->rfc1042_hdr.snap_type));

        memcpy(pEthHdr->src_addr, pRxPkt->eth803_hdr.src_addr,
               sizeof(pEthHdr->src_addr));
        memcpy(pEthHdr->dest_addr, pRxPkt->eth803_hdr.dest_addr,
               sizeof(pEthHdr->dest_addr));

        /* Chop off the RxPD + the excess memory from the 802.2/llc/snap header
         *   that was removed 
         */
        hdrChop = (u8 *) pEthHdr - (u8 *) pRxPkt;
    } else {
        HEXDUMP("RX Data: LLC/SNAP",
                (u8 *) & pRxPkt->rfc1042_hdr, sizeof(pRxPkt->rfc1042_hdr));

        /* Chop off the RxPD */
        hdrChop = (u8 *) & pRxPkt->eth803_hdr - (u8 *) pRxPkt;
    }

    /* Chop off the leading header bytes so the skb points to the start of 
     *   either the reconstructed EthII frame or the 802.2/llc/snap frame
     */
    skb_pull(skb, hdrChop);

    u32SkbLen = skb->len;
    wlan_compute_rssi(priv, pRxPD);

    if (os_upload_rx_packet(priv, skb)) {
        PRINTM(ERROR, "RX Error: os_upload_rx_packet" " returns failure\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
    priv->stats.rx_bytes += u32SkbLen;
    priv->stats.rx_packets++;

    PRINTM(DATA, "Data => kernel\n");
#ifdef WPRM_DRV
    WPRM_DRV_TRACING_PRINT();
    /* increase traffic meter rx counter 
       and call measurement function to see 
       if we need to change FW power mode. */
    wprm_rx_packet_cnt++;
    wprm_traffic_measurement(priv, Adapter, FALSE);
#endif
    ret = WLAN_STATUS_SUCCESS;
  done:
    LEAVE();

    return (ret);
}
