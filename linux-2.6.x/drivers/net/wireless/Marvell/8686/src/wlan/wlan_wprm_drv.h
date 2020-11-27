#ifndef _WLAN_WPRM_DRV_H_
#define _WLAN_WPRM_DRV_H_
/** @file wlan_wprm_drv.h
  *  
  * @brief This is header file for Power management APIs for WLAN driver
  * 
  */
/** 
  * @section copyright_sec Copyright
  *
  * (c) Copyright Motorola 2006 - 2008
  * This file is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * at your option) any later version.
  * This file is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * Revision History:
  * Author           Date            Description
  * Motorola         01-Sep-2006     Creation.
  * Motorola         09-Oct-2006     Change GPIO # for WLAN_HOST_WAKE_B (P3Bw, Marco)
  * Motorola         22-Nov-2006     Enable Host Wakeup on Multicast/Broadcast.
  * Motorola         29-Mar-2007     Implement chipset wake up by WLAN_CLIENT_WAKE_B
  * Motorola         31-May-2007     Add host sleep configuration when FW is in PS Mode at connection time.
  * Motorola         21-Mar-2008     Export datasession variable.
  */
  
/*---------------------------------- Includes --------------------------------*/
/*----------------------------------- Macros ---------------------------------*/
/* Return valuse */
#define WPRM_RETURN_SUCCESS  (0)
#define WPRM_RETURN_FAILURE  (-1)

/* Signal defines */
#define WPRM_SIGNAL_TRIGGER  (1)
#define WPRM_SIGNAL_RELEASE  (0)

/* Traffic Meter */
#define TRAFFIC_METER_MEASUREMENT_INTERVAL      (500)    /* in millisecond. Traffic meter granularity */
#define TXRX_TRESHOLD_EXIT_PS       (4)    /* Threshold to exit low power mode */
#define TXRX_TRESHOLD_ENTER_PS      (1)    /* Threshold to enter low power mode */
#define TIMESTAMP_SIZE              (2)    /* Timestamp tables size */
/* Traffic meter command number */
#define WPRM_TM_CMD_NONE            (0)
#define WPRM_TM_CMD_ENTER_PS        (1)
#define WPRM_TM_CMD_EXIT_PS         (2)
#define WPRM_TM_CMD_ENTER_UAPSD     (3)     /*Added by Zhang Yufei, 28/09/2005*/
#define WPRM_TM_CMD_CONFIG_HSCFG    (4)

/* hostsleepcfg params */
#define WPRM_HWUC_BROADCAST_MASK     (1 << 0)    /* bit0=1: broadcast data */
#define WPRM_HWUC_UNICAST_MASK       (1 << 1)    /* bit1=1: unicast data */
#define WPRM_HWUC_MAC_EVENTS_MASK    (1 << 2)    /* bit2=1: mac events */
#define WPRM_HWUC_MULTICAST_MASK     (1 << 3)    /* bit3=1: multicast data */
#define WPRM_HWUC_CANCEL_MASK        (0xffffffff)

#define WPRM_WLAN_HOST_WAKEB_GPIO_NO (1)         /* FW uses GPIO-1 to wakeup host */
#define WPRM_WLAN_HOST_WAKEUP_GAP    (20)        /* in ms. How far to trigger GPIO before sending an wakeup event */

/* Debug */
#ifdef WPRM_DRV_TRACING
#define WPRM_DRV_TRACING_PRINT()    printk("WPRM TRACING: %s, %s:%i\n", __FUNCTION__, \
						__FILE__, __LINE__)
#else
#define WPRM_DRV_TRACING_PRINT()
#endif


/*---------------------------------- Structures & Typedef --------------------*/

/* SESSIONTYPE structures */
typedef enum {
    ST_DATA_GENERAL = 0,
    ST_VOICE_20MS,
    ST_VOICE_40MS,
} WPRM_SESSION_TYPE_E;

typedef enum {
    SA_REGISTER = 0,
    SA_FREE,
} WPRM_SESSION_ACTION_E; 

typedef struct _WPRM_SESSION_SPEC_S {
    pid_t pid;
    int sid;
    int type;
    int act;
} WPRM_SESSION_SPEC_S; 

/*---------------------------------- Globals ---------------------------------*/
extern unsigned int wprm_tx_packet_cnt;                        /* TX packet counter */
extern unsigned int wprm_rx_packet_cnt;                        /* RX packet counter */
extern wlan_thread  tmThread;                                  /* Thread to traffic meter */
extern unsigned int wprm_tm_ps_cmd_no;                         /* Traffic meter command number */
extern BOOLEAN      wprm_tm_ps_cmd_in_list;                    /* is Traffic meter command in list */
extern BOOLEAN      is_wprm_traffic_meter_timer_set;           /* traffic meter timer is set or not */
extern unsigned int voicesession_status;
extern unsigned int datasession_status;

extern int wlan_set_power(struct net_device *dev, struct iw_request_info *info, \
                struct iw_param *vwrq, char *extra);           /* change wlan power mode */
		
extern __u32 gpio_wlan_hostwake_get_data(void);
extern void gpio_wlan_hostwake_clear_int(void);
extern void gpio_wlan_clientwake_set_data(__u32 data);


/*---------------------------------- APIs ------------------------------------*/
int wprm_configure_hscfg(void);
int wprm_wlan_host_wakeb_is_triggered(void);
int wprm_trigger_wlan_client_wakeb(int tr);
/* WLAN_HOST_WAKEB interrupt handler */
irqreturn_t wprm_wlan_host_wakeb_handler(int irq, void *dev_id, struct pt_regs *regs);
int wprm_traffic_meter_init(wlan_private * priv);
int wprm_traffic_meter_exit(wlan_private * priv);
int wprm_traffic_meter_start(wlan_private * priv);
void wprm_traffic_meter_timer_handler(void *FunctionContext);
int wprm_traffic_meter_service_thread(void *data);
void wprm_traffic_measurement(wlan_private *priv, wlan_adapter *Adapter, BOOLEAN isTx);
BOOLEAN wlan_wprm_tm_is_enabled(void);
int wlan_wprm_tm_enable(BOOLEAN tmEnable);
int wprm_session_type_action(WPRM_SESSION_SPEC_S *session);
#endif

