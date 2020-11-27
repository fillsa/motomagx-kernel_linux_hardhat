/*
 * otg/otg/usbp-bus.h - USB Device Bus Interface Driver Interface
 * @(#) balden@seth2.belcarratech.com|otg/otg/usbp-bus.h|20051116205002|12293
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
 * @file otg/otg/usbp-bus.h
 * @brief Bus interface Defines for USB Device Core Layaer
 *
 * This file contains the USB Peripheral Driver definitions.
 *
 * This is the interface between the bottom of the USB Core and the top of
 * the Bus Interace Drivers and is comprised of:
 *
 *      o public functions exported by the USB Core layer 
 *
 *      o structures and functions passed by the Function Driver to the USB
 *      Core layer
 *
 * USB Peripheral Controller Drivers are structured such that the upper edge
 * implements interfaces to the USB Peripheral Core layer to provide low
 * level USB services and the lower edge interfaces to the actual USB
 * Hardware (typically called the USB Device Controller or UDC.)
 *
 * The peripheral controller layer primarily deals with endpoints.
 * There is one control endpoint and one or more data endpoints.
 *
 * The control endpoint is special, with received data processed with
 * the built in EP0 function. Which in turn will pass requests to 
 * interface functions as appropriate.
 *
 * Data endpoints are controlled by interface driver and have a
 * function callback specified by the interface driver to perform
 * whatever needs to be done when data is sent or received.
 *
 *
 * @ingroup USBP
 */

/*
struct usbd_endpoint_map; 
struct usbd_endpoint_instance;
struct usbd_bus_instance;
struct usbd_urb;
*/

/*!
 * USB Bus Interface Driver structures
 *
 * Driver description:
 * 
 *  struct usbd_bus_operations
 *  struct usbd_bus_driver
 *
 */

extern int usbd_maxstrings;


/*!
 * exported to otg
 */
struct usbd_ops {
        int (*admin) (char *);
        char * (*function_name) (int);
};



/*! Operations that the slave layer or function driver can use to interact
 * with the bus interface driver. 
 *
 * The send_urb() function is used by the usb-device endpoint 0 driver and
 * function drivers to submit data to be sent.
 *
 * The cancel_urb() function is used by the usb-device endpoint 0 driver and
 * function drivers to remove previously queued data to be sent.
 *
 * The endpoint_halted() function is used by the ep0 control function to
 * check if an endpoint is halted.
 *
 * The endpoint_feature() function is used by the ep0 control function to
 * set/reset device features on an endpoint.
 *
 * The device_event() function is used by the usb device core to tell the
 * bus interface driver about various events.
 */
struct usbd_bus_operations {
        //int (*xbus_serial_number) (struct usbd_bus_instance *, char *);

        int (*start_endpoint_in) (struct usbd_bus_instance *, struct usbd_endpoint_instance *);
        int (*start_endpoint_out) (struct usbd_bus_instance *, struct usbd_endpoint_instance *);
        int (*cancel_urb_irq) (struct usbd_urb *);
        //int (*device_feature) (struct usbd_bus_instance *, int, int);
        int (*endpoint_halted) (struct usbd_bus_instance *, int);
        int (*halt_endpoint) (struct usbd_bus_instance *, int, int);
        int (*set_address) (struct usbd_bus_instance *, int);
        int (*event_handler) (struct usbd_bus_instance *, usbd_device_event_t, int);
        int (*request_endpoints) (struct usbd_bus_instance *, struct usbd_endpoint_map *, 
                        int, struct usbd_endpoint_request *);
        int (*set_endpoints) (struct usbd_bus_instance *, int , struct usbd_endpoint_map *);
        int (*framenum) (void); 
        //u64 (*ticks) (void); 
        //u64 (*elapsed) (u64 *, u64 *); 
        //u64 (*interrupts) (void);
};

/*! Endpoint configuration
 *
 * Per endpoint configuration data. Used to track which actual physical
 * endpoints.
 *
 * The bus layer maintains an array of these to represent all physical
 * endpoints.
 *
 */
struct usbd_endpoint_instance {
        int                             interface_wIndex;       // which interface owns this endpoint
        int                             physical_endpoint;      // physical endpoint address - bus interface specific

        //u8                              old_bEndpointAddress;       // logical endpoint address 
        //u8                              old_bmAttributes;           // endpoint type
        //u16                             old_wMaxPacketSize;         // packet size for requested endpoint

        u8                              new_bEndpointAddress[2];    // logical endpoint address 
        int                             new_bmAttributes[2];        // endpoint type
        u16                             new_wMaxPacketSize[2];      // packet size for requested endpoint

        u32                             feature_setting;        // save set feature information

        // control
        int                             state;                  // available for use by bus interface driver

        // receive side
        struct urb_link                 rdy;                    // empty urbs ready to receive
        struct usbd_urb                 *rcv_urb;               // active urb
        u32                             rcv_transferSize;       // maximum transfer size from function driver
        //u32                             rcv_transferSize[2];    // maximum transfer size from function driver
        int                             rcv_error;              // current bulk-in has an error

        // transmit side
        struct urb_link                 tx;                     // urbs ready to transmit
        struct usbd_urb                 *tx_urb;                // active urb
        u32                             tx_transferSize;        // maximum transfer size from function driver
        //u32                             tx_transferSize[2];     // maximum transfer size from function driver

        u32                             sent;                   // data already sent
        u32                             last;                   // data sent in last packet XXX do we need this

        void                            *privdata;

};

/*! @name endpoint zero states
 * @{
 */
#define WAIT_FOR_SETUP          0
#define DATA_STATE_XMIT         1
#define DATA_STATE_NEED_ZLP     2
#define WAIT_FOR_OUT_STATUS     3
#define DATA_STATE_RECV         4
#define DATA_STATE_PENDING_XMIT 5
#define WAIT_FOR_IN_STATUS      6
/*! @} */

/*! Bus Interface data structure
 *
 * Keep track of specific bus interface. 
 *
 * This is passed to the usb-device layer when registering. It contains all
 * required information about each real bus interface found such that the
 * usb-device layer can create and maintain a usb-device structure.
 *
 * Note that bus interface registration is incumbent on finding specific
 * actual real bus interfaces. There will be a registration for each such
 * device found.
 *
 * The max_tx_endpoints and max_rx_endpoints are the maximum number of
 * possible endpoints that this bus interface can support. The default
 * endpoint 0 is not included in these counts.
 *
 */
struct usbd_bus_driver {
        char                            *name;
        u8                              max_endpoints;  // maximimum number of rx enpoints
        u8                              otg_bmAttributes;
        u8                              maxpacketsize;
        u8                              high_speed_capable;
        struct usbd_device_description   *device_description;
        struct usbd_bus_operations       *bops;
        void                            *privdata;
        u32                             capabilities;
        u8                              bMaxPower;
        u8                              ports;
};

/*!
 * Bus state
 *
 * enabled or disabled
 */
typedef enum usbd_bus_state {
        usbd_bus_state_unknown,
        usbd_bus_state_disabled,
        usbd_bus_state_enabled
} usbd_bus_state_t;


/*!
 * @name UDC Capabilities
 * @{
 */
#define HAVE_CABLE_IRQ                  0x0001
#define REMOTE_WAKEUP_SUPPORTED         0x0002
#define ROOT_HUB                        0x0004
/*! @} */

/*! Bus Interface configuration structure
 *
 * This is allocated for each configured instance of a bus interface driver.
 *
 * It contains a pointer to the appropriate bus interface driver.
 *
 * The privdata pointer may be used by the bus interface driver to store private
 * per instance state information.
 */
struct usbd_bus_instance {

        struct usbd_bus_driver          *driver;

        int                             endpoints;
        struct usbd_endpoint_instance   *endpoint_array;        // array of available configured endpoints

        struct urb_link                 finished;               // processed urbs

        usbd_device_status_t            status;                 // device status
        usbd_bus_state_t                bus_state;
        usbd_device_state_t             device_state;           // current USB Device state
        usbd_device_state_t             suspended_state;        // previous USB Device state

        struct usbd_interface_instance  *ep0;                   // ep0 configuration
        struct usbd_function_instance   *function_instance;

        u8                              high_speed;

        u32                             device_feature_settings;// save set feature information
        u8                              otg_bmAttributes;

        struct WORK_STRUCT              device_bh;              // runs as bottom half, equivalent to interrupt time

        void                            *privdata;              // private data for the bus interface
        char                            *arg;

        int                             usbd_maxstrings;
        struct usbd_string_descriptor   **usb_strings;
};


/*! bus driver registration
 *
 * Called by bus interface drivers to register themselves when loaded
 * or de-register when unloading.
 */
struct usbd_bus_instance *usbd_register_bus (struct usbd_bus_driver *, int);
void usbd_deregister_bus (struct usbd_bus_instance *);


/*! Enable/Disable Function
 *
 * Called by a bus interface driver to select and enable a specific function 
 * driver.
 */
int usbd_enable_function_irq (struct usbd_bus_instance *, char *, char *);
void usbd_disable_function (struct usbd_bus_instance *);


/*! 
 * usbd_configure_device is used by function drivers (usually the control endpoint)
 * to change the device configuration.
 *
 * usbd_device_event is used by bus interface drivers to tell the higher layers that
 * certain events have taken place.
 */
void usbd_bus_event_handler (struct usbd_bus_instance *, usbd_device_event_t, int);
void usbd_bus_event_handler_irq (struct usbd_bus_instance *, usbd_device_event_t, int);


/*!
 * usbd_device_request - process a received urb
 * @urb: pointer to an urb structure
 *
 * Used by a USB Bus interface driver to pass received data in a URB to the
 * appropriate USB Function driver.
 *
 * This function must return 0 for success and -EINVAL if the request
 * is to be stalled.
 *
 * Not that if the SETUP is Host to Device with a non-zero wLength then there
 * *MUST* be a valid receive urb queued OR the request must be stalled.
 */
int usbd_device_request (struct usbd_bus_instance*, struct usbd_device_request *);

/*!
 * Device I/O
 */
void usbd_urb_finished (struct usbd_urb *, int );
void usbd_urb_finished_irq (struct usbd_urb *, int );
struct usbd_urb *usbd_first_urb_detached_irq (urb_link * hd);
struct usbd_urb *usbd_first_urb_detached (urb_link * hd);
void urb_append(urb_link *hd, struct usbd_urb *urb);

/*! flush endpoint
 */
void usbd_flush_endpoint_irq (struct usbd_endpoint_instance *);
void usbd_flush_endpoint (struct usbd_endpoint_instance *);
int usbd_find_endpoint_index(struct usbd_bus_instance *bus, int bEndpointAddress);



