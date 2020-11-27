/** @file wlan_wprm_drv.c
  *  
  * @brief This file includes Power management APIs for WLAN driver
  * 
  */
/** 
  * @section copyright_sec Copyright
  *
  * Copyright (C) 2006-2008 Motorola, Inc.
  *
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
  * Motorola         13-Oct-2006     Change GPIO # for WLAN_HOST_WAKE_B (P3Bw, Marco) 
  * Motorola         22-Nov-2006     Enable Host Wakeup on Multicast/Broadcast.
  * Motorola         29-Mar-2007     Implement chipset wake up by WLAN_CLIENT_WAKE_B
  * Motorola         31-May-2007     Add host sleep configuration when FW is in PS Mode at connection time.
  * Motorola         04-Jun-2007     Improve log capability
  * Motorola         18-Jul-2007     Improve log capability phase 2
  * Motorola         17-Mar-2008     Change in sleeppd command based on Marvell recommandation.
  * Motorola         21-Mar-2008     Change debug level and check for trafic meter enable.
  * Motorola         15-May-2008     Add MPM support.
  * Motorola         04-Jul-2008     Update file for OSS compliance.
  */

/*---------------------------------- Includes --------------------------------*/
#include "include.h"
#include <asm/boardrev.h>
#include <asm/mot-gpio.h>

/*---------------------------------- Globals ---------------------------------*/
/* Traffic meter */
static BOOLEAN    wprm_tm_enabled = TRUE;      /* Traffic meter feature enabled or not */  
unsigned int      wprm_tx_packet_cnt = 0;       /* TX packet counter */
unsigned int      wprm_rx_packet_cnt = 0;       /* RX packet counter */
WLAN_DRV_TIMER    wprm_traffic_meter_timer;            /* Traffic meter timer */
BOOLEAN           is_wprm_traffic_meter_timer_set = FALSE;      /* is WPrM traffic meter timer set or not? */
spinlock_t        wprm_traffic_meter_timer_lock = SPIN_LOCK_UNLOCKED;       /* spin lock of access traffic_meter_timer */

unsigned int      wprm_tm_ps_cmd_no = WPRM_TM_CMD_NONE;            /* Traffic meter command number */
BOOLEAN           wprm_tm_ps_cmd_in_list = FALSE;                  /* is Traffic meter command in list */
spinlock_t        wprm_tm_ps_cmd_lock = SPIN_LOCK_UNLOCKED;        /* spin lock of ps_cmd_no */

wlan_thread       tmThread;                     /* Thread to traffic meter */

/* tx/rx traffic timestamp */
unsigned int wprm_tx_timestamp[TIMESTAMP_SIZE] = {0};
unsigned int wprm_tx_index = 0;
unsigned int wprm_rx_timestamp[TIMESTAMP_SIZE] = {0};
unsigned int wprm_rx_index = 0;

unsigned int voicesession_status = 0;
unsigned int datasession_status = 0;


/*--------------------------- Local functions ---------------------------------*/

/*
 * @brief
 *    Send Host sleep config cmd to FW, 
 * @return
 *    0     : succeed
 */
void wprm_send_hscfg_cmd(wlan_private * priv, BOOLEAN cancel)
{
    HostCmd_DS_802_11_HOST_SLEEP_CFG    HostSleepCfgParam;   
    
    
    if (cancel == TRUE)
    {
        /* Need to cancel Host sleep configuration for FW UAPSD*/
	/* assembly hostwakeupcfg command params */
        HostSleepCfgParam.conditions = WPRM_HWUC_CANCEL_MASK;
        HostSleepCfgParam.gpio = 0;
        HostSleepCfgParam.gap = 0;
        /* send hostwakeupcfg command to Cancel hostwakecfg */
        PrepareAndSendCommand(priv,
                HostCmd_CMD_802_11_HOST_SLEEP_CFG,
                0, HostCmd_OPTION_WAITFORRSP,
                0, &HostSleepCfgParam);
		
        PRINTM(MOTO_DEBUG, "WLAN driver: WPRM thread, ENTER_UAPSD: hostsleepcfg cmd sent to FW, condition=%d, gpio=%d, gap=%d\n", \
		       HostSleepCfgParam.conditions, HostSleepCfgParam.gpio, HostSleepCfgParam.gap);
    
    }
    else
    {
        /* Need to send Host sleep configuration for FW PS*/
        HostSleepCfgParam.conditions = WPRM_HWUC_BROADCAST_MASK | WPRM_HWUC_UNICAST_MASK | WPRM_HWUC_MULTICAST_MASK;
		
        /* P2B and P3A wingboards use GPIO 2 for WLAN_HOST_WAKE_B */
        if((boardrev() == BOARDREV_P3AW) || (boardrev() == BOARDREV_P2BW))
        {
            HostSleepCfgParam.gpio = 2;
        }
	else
	{
	    HostSleepCfgParam.gpio = WPRM_WLAN_HOST_WAKEB_GPIO_NO;
	}
        	
	HostSleepCfgParam.gap = WPRM_WLAN_HOST_WAKEUP_GAP;
        
	/* send hostwakeupcfg command to quiet SDIO bus */       
        PrepareAndSendCommand(priv,
                    HostCmd_CMD_802_11_HOST_SLEEP_CFG,
                    0, HostCmd_OPTION_WAITFORRSP,
                    0, &HostSleepCfgParam);
    
        PRINTM(MOTO_DEBUG, "WLAN driver: WPRM thread, CONFIG_HSCFG: hostsleepcfg cmd sent to FW, condition=%d, gpio=%d, gap=%d\n", \
		       HostSleepCfgParam.conditions, HostSleepCfgParam.gpio, HostSleepCfgParam.gap);
		       
    }
    return;
}		       
		     

/*---------------------------------- APIs ------------------------------------*/


/* 
 *  @brief Check WLAN_HOST_WAKEB is triggered or not
 *
 *  @return 	   1 : triggered, 0 : not triggered
 */
int wprm_wlan_host_wakeb_is_triggered(void)
{
    WPRM_DRV_TRACING_PRINT();

    /* triggered */
    if(!gpio_wlan_hostwake_get_data())
        return 1;
    /* not triggered */
    return 0;
}


/* wprm_trigger_wlan_client_wakeb --- Trigger/Release WLAN_CLIENT_WAKEB
 * Description:
 *    Allow to trigger WLAN_CLIENT_WAKEB to wake up WLAN chip or release WLAN_CLIENT_WAKEB (initial state).
 * Parameters:
 *    int tr: 
 *       WPRM_SIGNAL_TRIGGER (1)- trigger
 *       WPRM_SIGNAL_RELEASE (0)- release
 * Return Value:
 *    0 : success
 */
int wprm_trigger_wlan_client_wakeb(int tr)
{
    WPRM_DRV_TRACING_PRINT();

    if(tr == WPRM_SIGNAL_TRIGGER) {
        /* trigger HOST_WLAN_WAKEB */
	gpio_wlan_clientwake_set_data(GPIO_DATA_LOW);
    }
    else {
        /* Initialize HOST_WLAN_WAKEB */
        gpio_wlan_clientwake_set_data(GPIO_DATA_HIGH);
    }

    return 0;
}



/* 
 *  @brief 
 *     This callback is registered in wlan_add_card(), and works as WLAN_HOST_WAKEB 
 *     interrupt handler. When WLAN firmware wakes up Host via WLAN_HOST_WAKEB
 *     falling edge, this callback is called to turn on SDIO_CLK. So, firmware can send SDIO
 *     event to Host through SDIO bus.
 *
 *  @param irq     Triggered irq number
 *  @param dev_id  Pointer to wlan device
 *  @param pt_regs
 * 
 *  @return 	   0.
 */
irqreturn_t wprm_wlan_host_wakeb_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	wlan_private   *priv;

	WPRM_DRV_TRACING_PRINT();
	priv = (wlan_private *)dev_id;

#ifdef ENABLE_PM
    mpm_handle_ioi( );
    if ((wlan_mpm_advice_id > 0 ) && (wlan_mpm_active == FALSE)){
        wlan_mpm_active = TRUE;
        mpm_driver_advise(wlan_mpm_advice_id, MPM_ADVICE_DRIVER_IS_BUSY);
        PRINTM(MOTO_DEBUG, "Host wake up triggered, driver is BUSY sent to MPM\n");
    }
#endif

	if(sbi_set_bus_clock(priv, TRUE)) {
		PRINTM(MOTO_DEBUG, "wprm_wlan_host_wakeb_handler: turn on sdio clock error.\n");
	}

	gpio_wlan_hostwake_clear_int();

	WPRM_DRV_TRACING_PRINT();

	return 0;
}


/* 
 * @brief    Initialize traffic meter
 *    . init tx/rx packet counters, and
 *    . register traffic meter timer
 * 
 * @param priv   Wlan driver's priv data
 * 
 * @return 
 *    WPRM_RETURN_SUCCESS: succeed
 *    WPRM_RETURN_FAILURE: fail
 */
int wprm_traffic_meter_init(wlan_private * priv)
{

    WPRM_DRV_TRACING_PRINT();

    /* to avoid init again */
    if(is_wprm_traffic_meter_timer_set)
        return WPRM_RETURN_SUCCESS;

    /* init tx/rx packet counters */
    wprm_tx_packet_cnt = 0;       /* TX packet counter */
    wprm_rx_packet_cnt = 0;       /* RX packet counter */

    /* register traffic meter timer */
    InitializeTimer( &wprm_traffic_meter_timer, wprm_traffic_meter_timer_handler, priv);
    ModTimer( &wprm_traffic_meter_timer, TRAFFIC_METER_MEASUREMENT_INTERVAL);

    is_wprm_traffic_meter_timer_set = TRUE;

    return WPRM_RETURN_SUCCESS;
}


/* 
 * @brief    De-init traffic meter 
 *    . delete traffic meter timer
 * 
 * @param priv   Wlan driver's priv data
 * 
 * @return 
 *    WPRM_RETURN_SUCCESS: succeed
 *    WPRM_RETURN_FAILURE: fail
 */
int wprm_traffic_meter_exit(wlan_private * priv)
{
    WPRM_DRV_TRACING_PRINT();

    /* to avoid exit twice */
    if(!is_wprm_traffic_meter_timer_set)
        return WPRM_RETURN_SUCCESS;

    /* delete traffic meter timer */
    CancelTimer(&wprm_traffic_meter_timer);
    FreeTimer(&wprm_traffic_meter_timer);

    is_wprm_traffic_meter_timer_set = FALSE;

    return WPRM_RETURN_SUCCESS;
}


/*
 * @brief
 *    Traffic meter timer handler. 
 *    In this handler, tx/rx packets in this time out period
 *     is accounted, and then make a decision about next PM state should be, then,
 *     issue a command in the queue, and wake up TM thread's waitQ. 
 * @param FunctionContext    Timer handler's private data 
 * @return None
 */
void wprm_traffic_meter_timer_handler(void *FunctionContext)
{

    /* get the adapter context */
    wlan_private *priv = (wlan_private *)FunctionContext;
    wlan_adapter *Adapter = priv->adapter;

    WPRM_DRV_TRACING_PRINT();
    if (wprm_tm_ps_cmd_in_list!=FALSE)
		goto wprm_tm_exit_reinstall;
    PRINTM(MOTO_DEBUG, "WLAN driver: WPRM: tx_cnt = %d. rx_cnt = %d.\n", wprm_tx_packet_cnt, wprm_rx_packet_cnt);
    if(datasession_status != 0 )
    {   /*Should Enter Active Mode*/
	if(Adapter->PSMode == Wlan802_11PowerModeCAM)
		goto wprm_tm_exit_reinstall;
	PRINTM(MOTO_MSG, "WLAN driver: WPRM: timer_handler(), Exit PS on data session.\n");
        wprm_tm_ps_cmd_no = WPRM_TM_CMD_EXIT_PS;
        wprm_tm_ps_cmd_in_list = TRUE;
        wake_up_interruptible(&tmThread.waitQ);
        PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to ENTER FULL POWER with data session set.\n");
        WPRM_DRV_TRACING_PRINT();
        goto wprm_tm_exit_reinstall;
    }


    else
    {
        /*Whether in UAPSD Mode*/
        if((Adapter->PSMode != Wlan802_11PowerModeCAM)&&(Adapter->sleep_period.period!=0))
        {
    	    if((voicesession_status != 0 ) /*&& (datasession_status == 0)*/)	
			    goto wprm_tm_exit_reinstall;	/*Keep in UAPSD Mode*/
	    if((wprm_tx_packet_cnt + wprm_rx_packet_cnt) >= TXRX_TRESHOLD_EXIT_PS) 
	    {	/*Should Enter Active Mode*/
	          PRINTM(MOTO_MSG, "WLAN driver: WPRM: timer_handler(), Exit PS on high traffic from UAPSD mode.\n");
		    wprm_tm_ps_cmd_no = WPRM_TM_CMD_EXIT_PS;
		   wprm_tm_ps_cmd_in_list = TRUE;
	          wake_up_interruptible(&tmThread.waitQ);
	          PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to ENTER FULL POWER.\n");

	          WPRM_DRV_TRACING_PRINT();
	          goto wprm_tm_exit_reinstall;
	    }
	    else		/*Should Enter PS Mode*/
	    {
	          PRINTM(MOTO_MSG, "WLAN driver: WPRM: timer_handler(), Enter PS on low traffic from UAPSD mode.\n");
		  wprm_tm_ps_cmd_no = WPRM_TM_CMD_ENTER_PS;
		  wprm_tm_ps_cmd_in_list = TRUE;
	          wake_up_interruptible(&tmThread.waitQ);
	          PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to ENTER ps.\n");

	          WPRM_DRV_TRACING_PRINT();
	          goto wprm_tm_exit_no_install;
	    }
        }
    }


    /* whether in power save mode */
    if((Adapter->PSMode != Wlan802_11PowerModeCAM)&&(Adapter->sleep_period.period==0)) 
    { 
        if(datasession_status != 0 )
        {       /*Should Enter Active Mode*/
              PRINTM(MOTO_MSG, "WLAN driver: WPRM: timer_handler(), Exit PS on data session from PS mode.\n");
              wprm_tm_ps_cmd_no = WPRM_TM_CMD_EXIT_PS;
              wprm_tm_ps_cmd_in_list = TRUE;
              wake_up_interruptible(&tmThread.waitQ);
              PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to ENTER FULL POWER.\n");
              WPRM_DRV_TRACING_PRINT();
              goto wprm_tm_exit_reinstall;
        }

	/*If voice session is set, then goto UAPSD mode*/
	else if(voicesession_status != 0 ) /*There is VOICE application running. go to UAPSD mode then*/			
	{
	    PRINTM(MOTO_MSG, "WLAN driver: WPRM: timer_handler(), Enter UAPSD on voice session from PS mode.\n");
	    wprm_tm_ps_cmd_no = WPRM_TM_CMD_ENTER_UAPSD;
	    wprm_tm_ps_cmd_in_list = TRUE;
	    wake_up_interruptible(&tmThread.waitQ);
	    PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to enter UAPSD.\n");

	     /* need to reinstall traffic meter timer */
	    WPRM_DRV_TRACING_PRINT();
	    goto wprm_tm_exit_reinstall;

	}
						

        if((wprm_tx_packet_cnt + wprm_rx_packet_cnt) >= TXRX_TRESHOLD_EXIT_PS) 
	{
            /* ENTER FULL POWER  mode */
            /* send command to traffic meter thread */
	    PRINTM(MOTO_MSG, "WLAN driver: WPRM: timer_handler(), Exit PS on high traffic from PS mode.\n");
            wprm_tm_ps_cmd_no = WPRM_TM_CMD_EXIT_PS;
            wprm_tm_ps_cmd_in_list = TRUE;
            wake_up_interruptible(&tmThread.waitQ);
            PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to exit ps.\n");

            WPRM_DRV_TRACING_PRINT();
            goto wprm_tm_exit_reinstall;
        }
        else {	
            /* should keep in power save mode */
            goto wprm_tm_exit_no_install;
        }
    }

    /* whether in active (full power) mode */
    if(Adapter->PSMode == Wlan802_11PowerModeCAM)
   {

        if(datasession_status != 0 )
        {       /*Should keep in  Active Mode*/
              PRINTM(INFO, "WPRM_TM: wprm_traffic_meter_handler(), datasession_status = %d.\n",datasession_status);
              PRINTM(INFO, "WPRM_TM: wprm_traffic_meter_handler(), keep in ACTIVE mode.\n");
              goto wprm_tm_exit_reinstall;
        }

        /*If voice session is set, then goto UAPSD mode*/
	else if(voicesession_status != 0 ) /*There is VOICE application running.*/
	{
	    PRINTM(MOTO_MSG, "WLAN driver: WPRM: timer_handler(), Enter UAPSD on voice session from FULL power.\n");
	    wprm_tm_ps_cmd_no = WPRM_TM_CMD_ENTER_UAPSD;
	    wprm_tm_ps_cmd_in_list = TRUE;
	    wake_up_interruptible(&tmThread.waitQ);
	    PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to enter UAPSD.\n");

	     /*need to reinstall traffic meter timer */
	    WPRM_DRV_TRACING_PRINT();
	    goto wprm_tm_exit_reinstall;

	}
	

        if((wprm_tx_packet_cnt + wprm_rx_packet_cnt) <= TXRX_TRESHOLD_ENTER_PS) 
	{
	    PRINTM(MOTO_MSG, "WLAN driver: WPRM: timer_handler(), Enter PS on low traffic from Full power.\n");
            /* enter power save mode */
            /* send command to traffic meter thread */
            wprm_tm_ps_cmd_no = WPRM_TM_CMD_ENTER_PS;
            wprm_tm_ps_cmd_in_list = TRUE;
            wake_up_interruptible(&tmThread.waitQ);
            PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to enter ps.\n");

            /* do not need to reinstall traffic meter timer */
            WPRM_DRV_TRACING_PRINT();
            goto wprm_tm_exit_no_install;
        }
        else {
            /* shoudl keep in active mode */
            goto wprm_tm_exit_reinstall;
        }
    }

    /* install traffic meter timer again */
wprm_tm_exit_reinstall:

    /* reset tx/rx packet counters */
    wprm_tx_packet_cnt = 0;       /* TX packet counter */
    wprm_rx_packet_cnt = 0;       /* RX packet counter */

    
    PRINTM(INFO, "WPRM_TM: before ModTimer()\n");
    ModTimer( &wprm_traffic_meter_timer, TRAFFIC_METER_MEASUREMENT_INTERVAL);
    PRINTM(INFO, "WPRM_TM: after ModTimer()\n");

    return;

    /* do not reinstall traffic meter timer */
wprm_tm_exit_no_install:
    /* reset tx/rx packet counters */
    wprm_tx_packet_cnt = 0;       /* TX packet counter */
    wprm_rx_packet_cnt = 0;       /* RX packet counter */

    /* delete timer */
    wprm_traffic_meter_exit(priv);

    return;
}


/*
 * @brief
 *    Measure wlan traffic, 
 *       . to decide to exit low power mode, and 
 *       . to start traffic meter timer
 *    If traffic meter timer has already been started, this routine does nothing.
 * @param priv   Wlan driver priv pointer
 * @param Adapter  Wlan driver Adapter pointer
 * @return None
 */
void wprm_traffic_measurement(wlan_private *priv, wlan_adapter *Adapter, BOOLEAN isTx)
{
    WPRM_DRV_TRACING_PRINT();

    /* check automatic traffic meter is enabled or not */
    if(wlan_wprm_tm_is_enabled() == FALSE) {
       return;
    }
    
    if((is_wprm_traffic_meter_timer_set == FALSE) /* ||
        (Adapter->PSMode != Wlan802_11PowerModeCAM) ||
        Adapter->bHostWakeupDevRequired */ ) {

        WPRM_DRV_TRACING_PRINT();

        if(isTx == TRUE) {

            /* update tx packets timestamp */
            wprm_tx_timestamp[wprm_tx_index] = xtime.tv_nsec/10000000+(xtime.tv_sec&0xffff)*100;
            PRINTM(INFO, "TX: %d, %d.\n", wprm_tx_index, wprm_tx_timestamp[wprm_tx_index]);
            wprm_tx_index = (wprm_tx_index + 1) % TIMESTAMP_SIZE;
        } 
        else {
            /* update rx packets timestamp */
            wprm_rx_timestamp[wprm_rx_index] = xtime.tv_nsec/10000000+(xtime.tv_sec&0xffff)*100;
            PRINTM(INFO, "RX: %d, %d.\n", wprm_rx_index, wprm_rx_timestamp[wprm_rx_index]);
            wprm_rx_index = (wprm_rx_index + 1) % TIMESTAMP_SIZE;
        }
        if(datasession_status != 0 )
        {       /*Should Enter Active Mode*/
	    PRINTM(MOTO_DEBUG, "WLAN driver: WPRM: wprm_traffic_measurement(), Exit PS on data session.\n");
            wprm_tm_ps_cmd_no = WPRM_TM_CMD_EXIT_PS;
            wprm_tm_ps_cmd_in_list = TRUE;
            wake_up_interruptible(&tmThread.waitQ);
            PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to ENTER FULL POWER.\n");
	    return;
        }
 
	/*If voice session is set, then goto UAPSD mode*/
	else if(voicesession_status != 0 ) /*There is VOICE application running.*/
	{
	    PRINTM(MOTO_MSG, "WLAN driver: WPRM: wprm_traffic_measurement(), Enter UAPSD on voice session.\n");
	    wprm_tm_ps_cmd_no = WPRM_TM_CMD_ENTER_UAPSD;
	    wprm_tm_ps_cmd_in_list = TRUE;
	    wake_up_interruptible(&tmThread.waitQ);
	    PRINTM(INFO, "WPRM_TM: after wake_up_interruptible() to enter UAPSD.\n");
	    return;
	}



        /* need to exit ps or not */
        if( /* tx packets measurement */
            ((wprm_tx_timestamp[(wprm_tx_index + TIMESTAMP_SIZE - 1) % TIMESTAMP_SIZE] - \
            wprm_tx_timestamp[(wprm_tx_index + TIMESTAMP_SIZE - 2) % TIMESTAMP_SIZE]) < \
            (TRAFFIC_METER_MEASUREMENT_INTERVAL /10)) && \
            /* rx packets measurement */ \
            ( (wprm_rx_timestamp[(wprm_rx_index + TIMESTAMP_SIZE - 1) % TIMESTAMP_SIZE] - \
            wprm_rx_timestamp[(wprm_rx_index + TIMESTAMP_SIZE - 2) % TIMESTAMP_SIZE]) < \
            (TRAFFIC_METER_MEASUREMENT_INTERVAL /10)) && \
            /* tx/rx correlation measurement */ \
            ( ((wprm_rx_timestamp[(wprm_rx_index + TIMESTAMP_SIZE - 1)  % TIMESTAMP_SIZE] > \
            wprm_tx_timestamp[(wprm_tx_index + TIMESTAMP_SIZE - 1) % TIMESTAMP_SIZE] ) ? \
            (wprm_rx_timestamp[(wprm_rx_index + TIMESTAMP_SIZE - 1) % TIMESTAMP_SIZE] - \
            wprm_tx_timestamp[(wprm_tx_index + TIMESTAMP_SIZE - 1) % TIMESTAMP_SIZE]) : \
            (wprm_tx_timestamp[(wprm_tx_index + TIMESTAMP_SIZE - 1) % TIMESTAMP_SIZE] - \
            wprm_rx_timestamp[(wprm_rx_index + TIMESTAMP_SIZE - 1) % TIMESTAMP_SIZE])) < \
            (TRAFFIC_METER_MEASUREMENT_INTERVAL /10)) ) {

            /* exit power save mode */
            /* send command to traffic meter thread */
            wprm_tm_ps_cmd_no = WPRM_TM_CMD_EXIT_PS;
            wprm_tm_ps_cmd_in_list = TRUE;
 
            wake_up_interruptible(&tmThread.waitQ);
            PRINTM(MOTO_MSG, "WLAN driver: WPRM: wprm_traffic_measurement(), Exit ps mode on TX/RX traffic high.\n");
        }
    }
   
    return;
}

/*
 * @brief
 *    Reconfigure Host sleep config for PS Mode, 
 * @return
 *    0     : succeed
 */
int wprm_configure_hscfg(void)
{

    /* check automatic traffic meter is enabled or not */
    if(wlan_wprm_tm_is_enabled() == FALSE) {
       return 0;
    }

    wprm_tm_ps_cmd_no = WPRM_TM_CMD_CONFIG_HSCFG;
    wprm_tm_ps_cmd_in_list = TRUE;
 
    wake_up_interruptible(&tmThread.waitQ);
    PRINTM(MOTO_DEBUG, "WLAN driver: WPRM: wprm_configure_hscfg(), to reconfigure hscfg.\n");

    return 0;
    
}


/* wprm_traffic_meter_service_thread --- service thread for meter feature
 * @brief
 *    In this thread, PM commands in queue are sent to driver's main service thread and got executed.
 * @param data  This thread
 * @return
 *    0     : succeed
 *    other : fail
 */
int wprm_traffic_meter_service_thread(void *data)
{
    wlan_thread  *thread = data;
    wlan_private *priv = thread->priv;
    wlan_adapter *Adapter = priv->adapter;
    wait_queue_t wait;
    struct iw_param iwrq;
    HostCmd_DS_802_11_SLEEP_PERIOD	sleeppd;

    WPRM_DRV_TRACING_PRINT();

    wlan_activate_thread(thread);
    init_waitqueue_entry(&wait, current);

    for(;;) {
        add_wait_queue(&thread->waitQ, &wait);
        OS_SET_THREAD_STATE(TASK_INTERRUPTIBLE);

        /* check any command need to be sent to wlan_main_service */
        if( !wprm_tm_ps_cmd_in_list ) { /* no command in list */
            schedule();
        }

        OS_SET_THREAD_STATE(TASK_RUNNING);
        remove_wait_queue(&thread->waitQ, &wait);
	
	/* To terminate thread */
        if (kthread_should_stop()) {
            break;
        }

        if(wlan_wprm_tm_is_enabled() == FALSE) {
            /* 
             * automatic traffic meter feature is disabled.
             * stop TM timer.
             */
            wprm_traffic_meter_exit(priv);
            PRINTM(INFO, "WPRM_TM_THREAD: command DISCARDED, while TM disabled.\n");
        } 
        else
        if( wprm_tm_ps_cmd_no == WPRM_TM_CMD_EXIT_PS ) {
            WPRM_DRV_TRACING_PRINT();

            /* assemble "iwconfig eth0 power off" command request */
            iwrq.value = 0;
            iwrq.fixed = 0;
            iwrq.disabled = 1;  /* exit from low power mode */
            iwrq.flags = 0;
            /* send exit IEEE power save command */
            wlan_set_power(priv->wlan_dev.netdev, NULL, &iwrq, NULL);

	    if(priv->adapter->sleep_period.period!=0 )
	    {
		 /*assembly sleeppd command*/
		 /*Added by Zhang, 28/09/2005,  To Exit periodic sleep*/
		 sleeppd.Action =  wlan_cpu_to_le16(HostCmd_ACT_GEN_SET);
		 sleeppd.Period = 0;

		 if ((priv->adapter->IsDeepSleep == TRUE))
		 {
		    PRINTM(MSG, "<1>():%s IOCTLS called when station is"
			    " in DeepSleep\n",__FUNCTION__);
		 }
		 else
		 {

		     PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SLEEP_PERIOD,
			    0, HostCmd_OPTION_WAITFORRSP,
			    0, (void *) &sleeppd);
		 }
	    }


            /* once enter active mode, start tm timer */
            if(priv->adapter->MediaConnectStatus != WlanMediaStateConnected)
            {
                wprm_tm_ps_cmd_no = WPRM_TM_CMD_NONE;
                wprm_tm_ps_cmd_in_list = FALSE;
		wprm_traffic_meter_exit(priv);
		PRINTM(MOTO_DEBUG, "WLAN driver: WPRM thread, EXIT_PS: link loss detected.\n");
                continue;
            }
            wprm_traffic_meter_init(priv);
            PRINTM(INFO, "WPRM_TM_THREAD: %s(), exit PS, and start TM timer.\n", __FUNCTION__);
        }
        else if( wprm_tm_ps_cmd_no == WPRM_TM_CMD_ENTER_PS ) {
            WPRM_DRV_TRACING_PRINT();
            if(priv->adapter->MediaConnectStatus != WlanMediaStateConnected)
            {
                wprm_tm_ps_cmd_no = WPRM_TM_CMD_NONE;
                wprm_tm_ps_cmd_in_list = FALSE;
		wprm_traffic_meter_exit(priv);
		PRINTM(MOTO_DEBUG, "WLAN driver: WPRM thread, ENTER_PS: link loss detected.\n");
                continue;
            }
		
	    if(priv->adapter->sleep_period.period!=0 )
	    {
		 /*assembly sleeppd command*/
		 sleeppd.Action =  wlan_cpu_to_le16(HostCmd_ACT_GEN_SET);
		 sleeppd.Period = 0;

                 if ((priv->adapter->IsDeepSleep == TRUE))
                 {
                    PRINTM(MSG, "<1>():%s IOCTLS called when station is"
                            " in DeepSleep\n",__FUNCTION__);
                 }
                 else
                 {
		     /* First exit PS */
		     /* assemble "iwconfig eth0 power off" command request */
                     iwrq.value = 0;
                     iwrq.fixed = 0;
                     iwrq.disabled = 1;  /* exit from low power mode */
                     iwrq.flags = 0;
                     /* send exit IEEE power save command */
                     wlan_set_power(priv->wlan_dev.netdev, NULL, &iwrq, NULL);
		     
		     /* Send the sleep pd command */
		     PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SLEEP_PERIOD,
			    0, HostCmd_OPTION_WAITFORRSP,
			    0, (void *) &sleeppd);
		 }

	    }
    
            if ((priv->adapter->IsDeepSleep == TRUE))
            {
                 PRINTM(MSG, "<1>():%s IOCTLS called when station is"
                                 " in DeepSleep\n",__FUNCTION__);
            }
            else
            {
	        /* Re configure Host sleep config for FW PS*/
          	wprm_send_hscfg_cmd(priv, FALSE);
	    }

            /* assembly enter IEEE power save command request */
            iwrq.value = 0;
            iwrq.fixed = 0;
            iwrq.disabled = 0;  /* enter low power mode */
            iwrq.flags = 0;
            /* send enter IEEE power save command */
            wlan_set_power(priv->wlan_dev.netdev, NULL, &iwrq, NULL);

            if(priv->adapter->MediaConnectStatus != WlanMediaStateConnected)
            {
                iwrq.disabled = 1;
                wlan_set_power(priv->wlan_dev.netdev, NULL, &iwrq, NULL);
                PRINTM(MOTO_DEBUG, "WLAN driver: WPRM thread, ENTER_PS: link loss detected - Exit PS.\n");
            }
            /* once enter PS mode, stop tm timer. After that, tm timer will only be triggered by 
             * traffic measurement or association 
             */
            wprm_traffic_meter_exit(priv);
            PRINTM(INFO, "WPRM_TM_THREAD: %s(), enter PS, and stop TM timer.\n", __FUNCTION__);
        }

	else if( wprm_tm_ps_cmd_no == WPRM_TM_CMD_ENTER_UAPSD)
	{
            WPRM_DRV_TRACING_PRINT();
            if(priv->adapter->MediaConnectStatus != WlanMediaStateConnected)
            {
                wprm_tm_ps_cmd_no = WPRM_TM_CMD_NONE;
                wprm_tm_ps_cmd_in_list = FALSE;
		wprm_traffic_meter_exit(priv);
		PRINTM(MOTO_DEBUG, "WLAN driver: WPRM thread, ENTER_UAPSD: link loss detected.\n");
                continue;
            }

            if ((priv->adapter->IsDeepSleep == TRUE))
            {
                 PRINTM(MSG, "<1>():%s IOCTLS called when station is"
                                 " in DeepSleep\n",__FUNCTION__);
            }
            else
            {
	        /* Cancel Host sleep config*/
	        wprm_send_hscfg_cmd(priv, TRUE);
	    }
	

	    /*assembly sleeppd command*/
	     sleeppd.Action =  wlan_cpu_to_le16(HostCmd_ACT_GEN_SET);
	     sleeppd.Period = 20;
   
            if ((priv->adapter->IsDeepSleep == TRUE))
            {
                 PRINTM(MSG, "<1>():%s IOCTLS called when station is"
                                 " in DeepSleep\n",__FUNCTION__);
            }
            else
            {
                /* First exit PS */
		/* assemble "iwconfig eth0 power off" command request */
                iwrq.value = 0;
                iwrq.fixed = 0;
                iwrq.disabled = 1;  /* exit from low power mode */
                iwrq.flags = 0;
                /* send exit IEEE power save command */
                wlan_set_power(priv->wlan_dev.netdev, NULL, &iwrq, NULL);
		     
                /* Send the sleep pd command */
	        PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SLEEP_PERIOD,
			0, HostCmd_OPTION_WAITFORRSP,
			0, (void *) &sleeppd);
    
	    }
    		
            /* assembly enter IEEE power save command request */
            iwrq.value = 0;
            iwrq.fixed = 0;
            iwrq.disabled = 0;  /* enter low power mode */
            iwrq.flags = 0;
            /* send enter IEEE power save command */
            wlan_set_power(priv->wlan_dev.netdev, NULL, &iwrq, NULL);

            /* once enter APSD mode, start tm timer.  */
            wprm_traffic_meter_init(priv);
            PRINTM(INFO, "WPRM_TM_THREAD: %s(), enter PS, and stop TM timer.\n", __FUNCTION__);
            if(priv->adapter->MediaConnectStatus != WlanMediaStateConnected)
            {
                sleeppd.Period = 0;
	
                if ((priv->adapter->IsDeepSleep == TRUE))
	        {
        	     PRINTM(MSG, "<1>():%s IOCTLS called when station is"
                             " in DeepSleep\n",__FUNCTION__);
	        }
        	else
	        {
		    PrepareAndSendCommand(priv, HostCmd_CMD_802_11_SLEEP_PERIOD,
                        0, HostCmd_OPTION_WAITFORRSP,
                        0, (void *) &sleeppd);
       
		}
     
                iwrq.disabled = 1;
                wlan_set_power(priv->wlan_dev.netdev, NULL, &iwrq, NULL);
                PRINTM(MOTO_DEBUG, "WLAN driver: WPRM thread, ENTER_UAPSD: link loss detected - Exit PS.\n");
           
		wprm_traffic_meter_exit(priv);
            }	
	}
	else if (wprm_tm_ps_cmd_no == WPRM_TM_CMD_CONFIG_HSCFG)
	{
	    /* if FW is already in PS Mode, need to reconfigure host sleep config */
	    if (Adapter->PSMode != Wlan802_11PowerModeCAM)
	    {
	        if (Adapter->sleep_period.period != 0)
	        {
	            /* Cancel Host sleep config for UAPSD*/
	            wprm_send_hscfg_cmd(priv, TRUE);
	        }
	        else
	        {
	            /* Re configure Host sleep config for FW PS*/
                    wprm_send_hscfg_cmd(priv, FALSE);
	        }
	    }
        }

        /* restore */
        wprm_tm_ps_cmd_no = WPRM_TM_CMD_NONE;
        wprm_tm_ps_cmd_in_list = FALSE;
    }
    /* exit */
    wlan_deactivate_thread(thread);

    WPRM_DRV_TRACING_PRINT();
    return 0;
}


/* 
 * @brief
 *    check wlan traffic meter feature is enabled or not
 * @param None
 * @return
 *    TRUE  : enabled
 *    FALSE : disabled
 */
BOOLEAN wlan_wprm_tm_is_enabled()
{
    return wprm_tm_enabled;
}


/* 
 * @brief
 *    enable/disable wlan traffic meter feature
 * @param tmEnable:
 *        TRUE : enable TM
 *        FALSE: disable TM
 * @return 1
 */
int wlan_wprm_tm_enable(BOOLEAN tmEnable)
{
    wprm_tm_enabled = tmEnable;
    return 1;
}


/* 
 * @brief
 *    register/free a session type. 
 *    . According to different session types currnetly registered, WPrM chooses different PM policy
 * @param session : pid, sid, type, and action
 * @return 0
 *    
 */
int wprm_session_type_action(WPRM_SESSION_SPEC_S *session)
{

    /* debug */
    PRINTM(MOTO_DEBUG, "WLAN driver: WPRM: before new session: data=%d, voice=%d\n", \
           datasession_status, voicesession_status);
	   
    PRINTM(MOTO_MSG, "WLAN driver: WPRM: new session: pid=0x%x, sid=%d, type=%d, action=%d.\n", \
           session->pid, session->sid, session->type, session->act);
	   
	   
	switch (session->type){
		
	case ST_DATA_GENERAL:

		if (SA_REGISTER == session->act)
			datasession_status = 1;
		else
			datasession_status = 0;
		break;
		
	case ST_VOICE_20MS:

		if (SA_REGISTER == session->act)
			voicesession_status = 1;
		else
			voicesession_status = 0;
		break;
	case ST_VOICE_40MS:

		if (SA_REGISTER == session->act)
			voicesession_status = 1;
		else
			voicesession_status = 0;
	
		break;
	}
	
    PRINTM(MOTO_DEBUG, "WLAN driver: WPRM: after new session: data=%d, voice=%d\n", \
           datasession_status, voicesession_status);
	   		
    return 0;
}
