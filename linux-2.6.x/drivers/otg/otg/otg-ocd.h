/*
 * otg/otg-ocd.h - OTG Controller Driver
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-ocd.h|20051116205001|29178
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
 * @defgroup OTGOCD OTG Controller Driver Support
 * @ingroup onthegogroup
 */
/*!
 * @file otg/otg/otg-ocd.h
 * @brief Defines common to On-The-Go OTG Controller Support
 *
 * This file defines the ocd_ops and ocd_instance structures.
 *
 * The ocd_ops structure contains all of the output functions that will
 * be called as required by the OTG event handler when changing states.
 *
 * The ocd_instance structure is used to maintain the global data
 * required by the OTG controller drivers.
 *
 * @ingroup OTGOCD
 */

/*!
 * @name OCD OTG Controller Driver
 * @{
 */
//struct ocd_instance;

struct ocd_instance {
        struct otg_instance *otg;
        void * privdata;
};


typedef int (*otg_timer_callback_proc_t) (void *);

#define OCD_CAPABILITIES_DR          1 << 0
#define OCD_CAPABILITIES_PO          1 << 1
#define OCD_CAPABILITIES_TR          1 << 2
#define OCD_CAPABILITIES_HOST        1 << 3

#define OCD_CAPABILITIES_AUTO        1 << 4


/*!
 * @struct ocd_ops
 * The ocd_ops structure contains pointers to all of the functions implemented for the
 * linked in driver. Having them in this structure allows us to easily determine what
 * functions are available without resorting to ugly compile time macros or ifdefs
 *
 * There is only one instance of this, defined in the device specific lower layer.
 */
struct ocd_ops {

        u32 capabilities;                               /* OCD Capabilities */

        /* Driver Initialization - by degrees
         */
        int (*mod_init) (void);                         /*!< OCD Module Initialization */
        void (*mod_exit) (void);                        /*!< OCD Module Exit */

        otg_output_proc_t       ocd_init_func;          /*!< OTG calls to initialize or de-initialize the OCD */

        int (*start_timer) (struct otg_instance *, int);/*!< called by OTG to start timer */
        u64 (*ticks) (void);            /*!< called by OTG to  fetch current ticks, typically micro-seconds when available */
        u64 (*elapsed) ( u64 *, u64 *);                 /*!< called by OTG to get micro-seconds elapsed between two ticks */
        //u32 interrupts;                                 /*!< called by OTG to get number of interrupts */
};


#if 0
struct ocd_instance {
        struct otg_instance *otg;
        void * privdata;
};
#endif

#ifndef OTG_APPLICATION

#define OCD ocd_trace_tag
extern otg_tag_t OCD;
extern struct ocd_ops ocd_ops;
extern struct ocd_instance *ocd_instance;
#endif
/* @} */
