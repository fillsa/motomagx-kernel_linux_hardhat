/*
 * otg/otg-tcd.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-tcd.h|20051116205002|00667
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 08/01/2005         Motorola         Initial distribution
 * 10/07/2005         Motorola         declared the function pointer 'pm_iomux_config' 
 * 06/12/2006         Motorola         OTG name change
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 */
/*
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

/*!
 * @defgroup OTGTCD Transceiver Controller Driver Support
 * @ingroup OTGOCD
 */
/*!
 * @file otg/otg/otg-tcd.h
 * @brief Defines common to On-The-Go Transceiver Controller Support
 *
 * This file defines the tcd_ops and tcd_instance structures.
 *
 * The tcd_ops structure contains all of the output functions that will
 * be called as required by the OTG event handler when changing states.
 *
 * The tcd_instance structure is used to maintain the global data
 * required by the transceiver controller drivers.
 *
 *
 * @ingroup OTGTCD
 */

/*!
 * @name TCD Transceiver Controller Driver
 * @{
 */


/*!
 * @struct tcd_ops
 * The pcd_ops structure contains pointers to all of the functions implemented for the
 * linked in driver. Having them in this structure allows us to easily determine what
 * functions are available without resorting to ugly compile time macros or ifdefs
 *
 * There is only one instance of this, defined in the device specific lower layer.
 */
struct tcd_ops {

        /* Driver Initialization - by degrees
         */
        int (*mod_init) (void);                         /*!< TCD Module Initialization */
        void (*mod_exit) (void);                        /*!< TCD Module Exit */

        u64 initial_state;
        void * privdata;


        /* 3. called for usbd_vbus() if defined */
        //int (*vbus) (struct otg_instance *);            /* return non-zero if the Vbus valid */
        //int (*id) (struct otg_instance *);              /* return non-zero if the ID valid */

        /* 4. called by otg event to control outputs
         */

        otg_output_proc_t       tcd_init_func;          /*!< OTG calls to initialize or de-initialize the HCD */
        otg_output_proc_t       tcd_en_func;            /*!< OTG calls to enable or disable the TCD */
        otg_output_proc_t       chrg_vbus_func;         /*!< OTG calls to enable or disable charging Vbus */
        otg_output_proc_t       drv_vbus_func;          /*!< OTG calls to enable or disable Vbus */
        otg_output_proc_t       dischrg_vbus_func;      /*!< OTG calls to enable or disable discharging Vbus */
        otg_output_proc_t       dp_pullup_func;         /*!< OTG calls to enable or disable D+ pullup (aka loc_conn) */
        otg_output_proc_t       dm_pullup_func;         /*!< OTG calls to enable or disable D- pullup (aka loc_carkit) */
        otg_output_proc_t       dp_pulldown_func;       /*!< OTG calls to enable or disable D+ pulldown (aka loc_conn) */
        otg_output_proc_t       dm_pulldown_func;       /*!< OTG calls to enable or disable D- pulldown (aka loc_carkit) */
        otg_output_proc_t       peripheral_host_func;   /*!< OTG calls to enable or disable D- pulldown (aka loc_carkit) */
        otg_output_proc_t       overcurrent_func;       /*!< OTG calls to clear overcurrent indication */
        otg_output_proc_t       dm_det_func;            /*!< OTG calls to enable or disable D+ pullup detection */
        otg_output_proc_t       dp_det_func;            /*!< OTG calls to enable or disable D- pullup detection */
        otg_output_proc_t       cr_det_func;            /*!< OTG calls to enable or disable D+ CRINT detection */
        otg_output_proc_t       charge_pump_func;       /*!< OTG calls to enable or disable external charge pump */
        otg_output_proc_t       bdis_acon_func;         /*!< OTG calls to enable or disable the BDIS ACON feature */
        otg_output_proc_t       mx21_vbus_drain_func;   /*!< OTG calls to enable or disable the mx21 vbus drain feature */
        otg_output_proc_t       id_pulldown_func;       /*!< OTG calls to enable or disable ID pulldown to ground */
        otg_output_proc_t       audio_func;             /*!< OTG calls to enable or disable audio function */
        otg_output_proc_t       uart_func;              /*!< OTG calls to enable or disable uart function */
        otg_output_proc_t       mono_func;              /*!< OTG calls to enable or disable mono function */
};

/*!
 * @struct hcd_instance
 */
struct tcd_instance {
        struct otg_instance *otg;                       /*!< pointer to OTG Instance */
        struct WORK_STRUCT bh;                          /*!< work structure for bottom half handler */

        BOOL vbus;
        BOOL id;

        void *privdata;
};


extern struct trace_ops trace_ops;
extern struct tcd_ops tcd_ops;


extern struct tcd_instance *tcd_instance;
extern int tcd_vbus (struct otg_instance *otg);

/* pcd_cable_event[_irq] - these can be called in a traditional PCD implementation 
 * to set b_sess_vld appropriately when vbus is sensed (pcd must implement ocd_ops.vbus)
 */
extern void tcd_cable_event_irq (struct otg_instance *otg, u8 connect);

extern void tcd_cable_event (struct otg_instance *otg, u8 connect);

//extern void tcd_init(struct otg_instance *, u8 );

#define TCD tcd_trace_tag
extern otg_tag_t TCD;

extern int mxc_iomux_gpio_mc13783_reset (void);
extern int mxc_iomux_gpio_mc13783_set (int);

extern int (*pm_iomux_config)(void);
/* @} */
