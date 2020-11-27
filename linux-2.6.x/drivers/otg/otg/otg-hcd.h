/*
 * otg/otg-hcd.h - OTG Host Controller Driver
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-hcd.h|20051116205001|23709
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
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 *
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
 *
 */
/*!
 * @defgroup OTGHCD Host Controller Driver Support
 * @ingroup onthegogroup
 */
/*!
 * @file otg/otg/otg-hcd.h
 * @brief Defines common to On-The-Go Host Controller Support
 *
 * This file defines the hcd_ops and hcd_instance structures.
 *
 * The hcd_ops structure contains all of the output functions that will
 * be called as required by the OTG event handler when changing states.
 *
 * The hcd_instance structure is used to maintain the global data
 * required by the host controller drivers.
 *
 * @ingroup OTGHCD
 */

/*!
 * @name HCD Host Controller Driver
 * @{
 */
struct hcd_instance;

/*!
 * @struct hcd_ops
 * The pcd_ops structure contains pointers to all of the functions implemented for the
 * linked in driver. Having them in this structure allows us to easily determine what
 * functions are available without resorting to ugly compile time macros or ifdefs
 *
 * There is only one instance of this, defined in the device specific lower layer.
 */
struct hcd_ops {
        
        /* Driver Initialization - by degrees
         */
        int (*mod_init) (void);                         /*!< HCD Module Initialization */
        void (*mod_exit) (void);                        /*!< HCD Module Exit */


        /* mandatory */
        int max_ports;                                  /*!< maximum number of ports available */
        u32 capabilities;                               /*!< UDC Capabilities - see usbd-bus.h for details */
        char *name;                                     /*!< name of controller */


        otg_output_proc_t       hcd_init_func;          /*!< OTG calls to initialize or de-initialize the HCD */
        otg_output_proc_t       hcd_en_func;            /*!< OTG calls to enable or disable the HCD */
        otg_output_proc_t       hcd_rh_func;            /*!< OTG calls to enable HCD Root Hub */
        otg_output_proc_t       loc_sof_func;           /*!< OTG calls to into a_host or b_host state - attempt to use port */
        otg_output_proc_t       loc_suspend_func;       /*!< OTG calls to suspend bus */
        otg_output_proc_t       remote_wakeup_en_func;  /*!< OTG calls to issue SET FEATURE REMOTE WAKEUP */
        otg_output_proc_t       hnp_en_func;            /*!< OTG calls to issues SET FEATURE B_HNP_ENABLE */

        framenum_t		framenum;               /*!< OTG calls to get current USB Frame number */
};

/*!
 * @struct hcd_instance
 */
struct hcd_instance {
        struct otg_instance *otg;                       /*!< pointer to OTG Instance */
        void * privdata;                                /*!< pointer to private data for PCD */
        struct WORK_STRUCT bh;                          /*!< work structure for bottom half handler */
        int active;
};

#define HCD hcd_trace_tag
extern otg_tag_t HCD;
extern struct hcd_ops hcd_ops;
extern struct hcd_instance *hcd_instance;
#if !defined(OTG_C99)
extern void fs_hcd_global_init(void);
#endif /* !defined(OTG_C99) */
extern void hcd_init_func(struct otg_instance *, u8 );
extern void hcd_en_func(struct otg_instance *, u8 );

/* @} */

