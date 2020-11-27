/*
 * otg/otg-pcd.h - OTG Peripheral Controller Driver
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-pcd.h|20051116205001|32084
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
 * 09/26/2005         Motorola         Change flag name from USB_INACTIVE to MPM_USB_INACTIVE 
 * 10/21/2005         Motorola         Change flag name from MPM_USB_INACTIVE to PM_USB_INACTIVE
 * 03/01/2006         Motorola         Change PM_USB_INACTIVE from bool to u8. 
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
 * @defgroup OTGPCD Peripheral Controller Driver Support (Common)
 * @ingroup OTGOCD
 */
/*!
 * @file otg/otg/otg-pcd.h
 * @brief Defines common to On-The-Go Peripheral Controller Support
 *
 * This file defines the pcd_ops and pcd_instance structures.
 *
 * The pcd_ops structure contains all of the output functions that will
 * be called as required by the OTG event handler when changing states.
 *
 * The pcd_instance structure is used to maintain the global data
 * required by the peripheral controller drivers.
 *
 * @ingroup OTGPCD
 */

/*!
 * @name PCD Peripheral Controller Driver
 * @{
 */

/*!
 * @struct pcd_ops
 *
 * The pcd_ops structure contains pointers to all of the functions implemented for the
 * linked in driver. Having them in this structure allows us to easily determine what
 * functions are available without resorting to ugly compile time macros or ifdefs
 *
 * There is only one instance of this, defined in the device specific lower layer.
 */
struct pcd_ops {

        int (*mod_init) (void);                         /*!< PCD Module Initialization */
        void (*mod_exit) (void);                        /*!< PCD Module Exit */
       
        otg_output_proc_t       pcd_init_func;          /*!< OTG calls to initialize or de-initialize the PCD */
        otg_output_proc_t       pcd_en_func;            /*!< OTG calls to enable or disable the PCD */
        otg_output_proc_t       remote_wakeup_func;     /*!< OTG calls to have PCD perform remote wakeup */

        framenum_t		framenum;               /*!< OTG calls to get current USB Frame number */
};

#define PCD pcd_trace_tag
extern otg_tag_t PCD;
extern struct pcd_ops pcd_ops;
extern struct pcd_instance *pcd_instance;

/*!
 * @struct pcd_instance
 */
struct pcd_instance {
        struct otg_instance *otg;                       /*!< pointer to OTG Instance */
        struct usbd_bus_instance *bus;                  /*!< pointer to usb bus instance */
        void * privdata;                                /*!< pointer to private data for PCD */
        int pcd_exiting;                                /*!< non-zero if OTG is unloading */
        struct WORK_STRUCT bh;                          /*!< work structure for bottom half handler */
        int active;
};


#if !defined(OTG_C99)
extern void pcd_global_init(void);
#endif /* !defined(OTG_C99) */
extern void pcd_init_func(struct otg_instance *, u8 );
extern void pcd_en_func(struct otg_instance *, u8 );


/* Motorola: Added */
extern u8 PM_USB_INACTIVE;
#define OS_SUSPENDED 0x10
#define USB_SUSPENDED 0x01
/* END Motorola: Added */

/* @} */

