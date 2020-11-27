/*
 * otg/otg-api.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-api.h|20051116205001|15413
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
/*
 * Doxygen group definitions - N.B. These much each be in
 * their own separate comment...
 */
/*!
 * @defgroup onthegogroup On-The-Go
 */
/*!
 * @defgroup functiongroup Function Drivers
 * @ingroup onthegogroup
 */
/*!
 * @defgroup tcdgroup Transceiver Controller Drivers
 * @ingroup OTGOCD
 */
/*!
 * @defgroup pcdgroup Peripheral Controller Drivers
 * @ingroup OTGOCD
 */
/*!
 * @defgroup libgroup Architecture Libraries
 * @ingroup OTGOCD
 */
/*!
 * @defgroup platformgroup Platform Drivers
 * @ingroup OTGOCD
 */


/*!
 * @defgroup OTGCore On-The-Go Core
 * @ingroup onthegogroup
 */

/*!
 * @file otg/otg/otg-api.h
 * @brief Core Defines for USB OTG Core Layaer
 *
 * @ingroup OTGCore
 */

/*!
 * @name OTGCORE OTG API Definitions
 * This contains the OTG API structures and definitions.
 * @{
 */
/*!
 * @defgroup MouseFunction Random Mouse
 * @ingroup functiongroup
 */

#include <otg/otg-fw.h>

#if defined(CONFIG_OTG_USB_PERIPHERAL) || defined (CONFIG_OTG_USB_HOST) || defined(CONFIG_OTG_USB_PERIPHERAL_OR_HOST)
#include <otg/otg-fw-mn.h>
#elif defined(CONFIG_OTG_BDEVICE_WITH_SRP) || defined(CONFIG_OTG_DEVICE) 
#include <otg/otg-fw-df.h>
#else /* error */
#abort "Missing USB or OTG configuration"
#endif


struct otg_instance;
typedef void (*otg_output_proc_t) (struct otg_instance *, u8);

extern struct otg_firmware *otg_firmware_loaded;
extern struct otg_firmware *otg_firmware_orig;
extern struct otg_firmware *otg_firmware_loading;


char * otg_get_state_name(int state);

/*!
 * @struct otg_instance
 *
 * This tracks the current OTG configuration and state
 */
struct otg_instance {

        u16                     state;                  /*!< current state      */
        int                     previous;               /*!< previous state     */

        int                     active;                 /*!< used as semaphore  */

        u32                     current_inputs;         /*!< input settings     */
        u64                     outputs;                /*!< output settings    */

        u64                     tickcount;              /*!< when we transitioned to current state      */
        u64                     bconn;                  /*!< when b_conn was set        */

        struct otg_state       *current_outputs;       /*!< output table entry for current state        */
        struct otg_state       *previous_outputs;      /*!< output table entry for current state        */

        otg_output_proc_t       otg_output_ops[MAX_OUTPUTS]; /*!< array of functions mapped to output numbers */

        int                     (*start_timer) (struct otg_instance *, int); /*!< OCD function to start timer */
        u64                     (*ticks) (void);        /*!< OCD function to return ticks */
        u64                     (*elapsed) ( u64 *, u64 *); /*!< OCD function to return elapsed ticks */

        void *                  hcd;                    /*!< pointer to hcd instance */
        void *                  ocd;                    /*!< pointer to ocd instance */
        void *                  pcd;                    /*!< pointer to pcd instance */
        void *                  tcd;                    /*!< pointer to tcd instance */

        char                    function_name[OTGADMIN_MAXSTR]; /*!< current function */
        char                    serial_number[OTGADMIN_MAXSTR]; /*!< current serial number */

        u32 interrupts;                                 /*!< track number of interrupts */
};

typedef int (*otg_event_t) (struct otg_instance *, int, char *);

struct otg_event_info {
        char *name;
        otg_event_t event;
};

typedef u16 (*framenum_t)(void);

/* 1 - used by external functions to pass in administrative commands as simple strings
 */
extern int otg_status(int, u32, u32, char *, int);
extern void otg_message(char *);
                                        
/* 2 - used internally by any OTG stack component to single event from interrupt context
 */
extern int otg_event_irq(struct otg_instance *, u64, otg_tag_t, char *);
                                        
/* 3 - used internally by any OTG stack component to single event from non-interrupt context,
 */
extern int otg_event(struct otg_instance *, u64, otg_tag_t, char *);

/* 5 - used by PCD/TCD drivers to signal various OTG Transceiver events
 */
int otg_event_set_irq(struct otg_instance *, int, int, u32, otg_tag_t, char *);


extern void otg_serial_number(struct otg_instance *otg, char *serial_number_str);
extern void otg_init (struct otg_instance *otg);
extern void otg_exit (struct otg_instance *otg);

/* message 
 */
#if defined(OTG_LINUX)
extern int otg_message_init_l24(struct otg_instance *);
extern void otg_message_exit_l24(void);
#endif /* defined(OTG_LINUX) */

#if defined(OTG_WINCE)
extern int otg_message_init_w42(struct otg_instance *);
extern void otg_message_exit_w42(void);
#endif /* defined(OTG_WINCE) */

extern int otg_message_init(struct otg_instance *);
extern void otg_message_exit(void);

extern int otg_write_message_irq(char *buf, int size);
extern int otg_write_message(char *buf, int size);
extern int otg_read_message(char *buf, int size);
extern int otg_data_queued(void);
extern unsigned int otg_message_block(void);
extern unsigned int otg_message_poll(struct file *, struct poll_table_struct *);


/* trace
 */
extern int otg_trace_init (void);
extern void otg_trace_exit (void);
#if defined(OTG_LINUX)
extern int otg_trace_init_l24(void);
extern void otg_trace_exit_l24(void);
#endif /* defined(OTG_LINUX) */

#if defined(OTG_WINCE)
extern int otg_trace_init_w42(void);
extern void otg_trace_exit_w42(void);
#endif /* defined(OTG_WINCE) */

extern int otgtrace_init (void);
extern void otgtrace_exit (void);
extern int otg_trace_proc_read (char *page, int count, int * pos);
extern int otg_trace_proc_write (const char *buf, int count, int * pos);

/* usbp
 */
extern int usbd_device_init (void);
extern void usbd_device_exit (void);


/*
 * otgcore/otg-mesg.c
 */

extern void otg_message(char *buf);
extern void otg_mesg_get_status_update(struct otg_status_update *status_update);
extern void otg_mesg_get_firmware_info(struct otg_firmware_info *firmware_info);
extern int otg_mesg_set_firmware_info(struct otg_firmware_info *firmware_info);
extern struct otg_instance * mesg_otg_instance;



/*
 * otgcore/otg.c
 */
extern char * otg_get_state_names(int i);

/*
 * ops
 */

extern struct hcd_ops *otg_hcd_ops;
extern struct ocd_ops *otg_ocd_ops;
extern struct pcd_ops *otg_pcd_ops;
extern struct tcd_ops *otg_tcd_ops;
extern struct usbd_ops *otg_usbd_ops;

#define CORE core_trace_tag
extern otg_tag_t CORE;

extern struct hcd_instance * otg_set_hcd_ops(struct hcd_ops *);
extern struct ocd_instance * otg_set_ocd_ops(struct ocd_ops *);
extern struct pcd_instance * otg_set_pcd_ops(struct pcd_ops *);
extern struct tcd_instance * otg_set_tcd_ops(struct tcd_ops *);
extern int otg_set_usbd_ops(struct usbd_ops *);

extern void otg_get_trace_info(otg_trace_t *p);
extern int otg_tmr_id_gnd(void);
extern u64 otg_tmr_ticks(void);
extern u64 otg_tmr_elapsed(u64 *t1, u64 *t2);
extern u16 otg_tmr_framenum(void);
extern u32 otg_tmr_interrupts(void);

extern void otg_write_info_message(char *msg);

/* @} */


