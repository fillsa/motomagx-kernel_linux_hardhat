/*
 * otg/otg/usbp-func.h - USB Device Function Driver Interface
 * @(#) balden@seth2.belcarratech.com|otg/otg/usbp-func.h|20051116205002|17742
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
 * @file otg/otg/usbp-func.h
 * @brief Function Driver related structures and definitions.
 *
 * This file contains the USB Function Driver and USB Interface Driver 
 * definitions.
 *
 * The USBOTG support allows for implementation of composite devices.
 *
 * From USB 2.0, C.f. 5.2.3:
 *
 *      A Device that has multiple interfaces controlled independantly of
 *      each other is referred to as a composite device. A composite device
 *      has only a single device.
 *
 * In this implemenation the function portion of the device is split into
 * two types of drivers:
 *
 *      - USB Interface Driver
 *      - USB Function Driver
 *
 *
 * USB Interface Driver
 *
 * An USB Interface Driver specifies the Interface related descriptors and
 * implements the data handling processes for each of the data endpoints
 * associated with the interface (or interfaces) required for the function.
 * It typically will also implement the upper edge interface to the OS.
 *
 * The USB Interface Driver will also handle non-standard device requests
 * and feature requests that are for the interface or an endpoint associated
 * with one of the interfaces associated with the driver.
 *
 * Each interface implemented by an USB Interface Driver will have a
 * interface instance that stores a pointer to the parent USB Function
 * Driver and has an array of pointers to endpoint instances for the data
 * endpoints associated with the interface.
 *
 *
 * USB Function Driver
 *
 * A USB Function Driver specifies the device and configuration descriptors.
 * A configuration descriptor is assembled from the interface descriptors
 * from the USB Interface Driver (or drivers) that it is making available to
 * the USB Host.
 *
 * The USB Function Driver handles non-standard device requests and feature
 * requests for the device.
 *
 * Each function driver has a function instance that maintains an array
 * of pointers to configuration instances and a pointer to the device
 * descriptor.
 *
 * Each configuration instance maintains an array of pointers to the 
 * interface instances for that configuration and a pointer to the 
 * complete configuration descriptor.
 *
 * @ingroup USBP
 */

/*!
 * This file contains the USB Device Core Common definitions. It also contains definitions for
 * functions that are commonly used by both Function Drivers and Bus Interface Drivers.
 *
 * Specifically:
 *
 *      o public functions exported by the USB Core layer 
 *
 *      o common structures and definitions 
 *
 */


/*!
 * USB Function Driver types
 */
typedef enum usbd_function_types {
        function_ep0,                // combined driver
        function_simple,                // combined driver
        function_interface,             // supports interface 
        function_class,                 // supports class
        function_composite,             // supports configuration
} usbd_function_types_t; 


/*!
 * USB Function Driver structures
 *
 * Descriptors:
 *   struct usbd_endpoint_description
 *   struct usbd_interface_description 
 *   struct usbd_configuration_description
 *
 * Driver description:
 *   struct usbd_function_driver
 *   struct usbd_function_operations
 *
 */

struct usbd_function_operations {

        int (*function_enable) (struct usbd_function_instance *);
        void (*function_disable) (struct usbd_function_instance *);

        /*
         * All of the following may be called from an interrupt context
         */
        void (*event_handler) (struct usbd_function_instance *, usbd_device_event_t, int);
        int (*device_request) (struct usbd_function_instance *, struct usbd_device_request *);

        // XXX should these return anything?
        int (*set_configuration) (struct usbd_function_instance *, int configuration);
        int (*set_interface) (struct usbd_function_instance *, int wIndex, int altsetting);
        int (*suspended) (struct usbd_function_instance *);
        int (*resumed) (struct usbd_function_instance *);
        int (*reset) (struct usbd_function_instance *);

        void * (*get_descriptor)(struct usbd_function_instance *, int descriptor_type, int descriptor_index);
        int * (*set_descriptor)(struct usbd_function_instance *, int descriptor_type, int descriptor_index, void *);

        void (*endpoint_cleared)(struct usbd_function_instance*, int wIndex);

};


/*!
 * function driver definitions
 */
struct xusbd_alternate_instance {
        struct usbd_interface_descriptor *interface_descriptor;
        int                             classes;
        struct usbd_generic_class_descriptor    **class_list;
        int                             endpoints;
};


/*!
 * usb device description structures
 */

struct usbd_alternate_description {
        //struct usbd_interface_descriptor *interface_descriptor;
        char                            *iInterface;
        u8                              bInterfaceClass;
        u8                              bInterfaceSubClass;
        u8                              bInterfaceProtocol;

        // list of CDC class descriptions for this alternate interface
        u8                              classes;
        struct usbd_generic_class_descriptor    **class_list;

        // list of endpoint descriptions for this alternate interface
        u8                              endpoints;

        // list of indexes into endpoint request map for each endpoint descriptor
        u8                              *endpoint_index;

                                                        // XXX This should be here
        u8                              new_endpointsRequested;
        struct usbd_endpoint_request    *new_requestedEndpoints;
};

typedef struct usbd_generic_class_descriptor usbd_class_descriptor_t;
typedef struct usbd_endpoint_descriptor      usbd_endpoint_descriptor_t;

struct usbd_interface_description {
        // list of alternate interface descriptions for this interface
        u8                              alternates;
        struct usbd_alternate_description *alternate_list;
};

struct usbd_configuration_description {
        char                            *iConfiguration;
        u8 bmAttributes;
        u8 bMaxPower;
};

struct usbd_device_description {
        //struct usbd_device_descriptor   *device_descriptor;
        //struct usbd_otg_descriptor      *otg_descriptor;

        u16                             idVendor;
        u16                             idProduct;
        u16                             bcdDevice;
        u8                              bDeviceClass;
        u8                              bDeviceSubClass;
        u8                              bDeviceProtocol;

        u8                              bMaxPacketSize0;

        u8                              *iManufacturer;
        u8                              *iProduct;
        u8                              *iSerialNumber;
};


struct usbd_interfaces_instance {
        u8                              alternates;
        struct usbd_alternate_instance  *alternates_instance_array;
};

/*! Function Driver data structure
 *
 * Function driver and its configuration descriptors. 
 *
 * This is passed to the usb-device layer when registering. It contains all
 * required information about the function driver for the usb-device layer
 * to use the function drivers configuration data and to configure this
 * function driver an active configuration.
 *
 * Note that each function driver registers itself on a speculative basis.
 * Whether a function driver is actually configured will depend on the USB
 * HOST selecting one of the function drivers configurations. 
 *
 * This may be done multiple times WRT to either a single bus interface
 * instance or WRT to multiple bus interface instances. In other words a
 * multiple configurations may be selected for a specific bus interface. Or
 * the same configuration may be selected for multiple bus interfaces. 
 *
 */
#define FUNCTION_REGISTERED             0x1                     // set in flags if function is registered
#define FUNCTION_ENABLED                0x2                     // set in flags if function is enabled
struct usbd_function_driver {
        LIST_NODE                       drivers;              // linked list (was drivers in usbd_function_driver)
        char                            *name;
        struct usbd_function_operations *fops;  // functions 
        usbd_function_types_t           function_type;
        u32                             flags;
        void                            *privdata;
};

/*! function configuration structure
 *
 * This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration 
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */
struct usbd_function_instance {
        char *                          name;
        struct usbd_bus_instance        *bus;
        usbd_function_types_t           function_type;
        struct usbd_function_driver     *function_driver;
        void                            *privdata;              // private data for the function

        //int                             usbd_maxstrings;
        //struct usbd_string_descriptor   **usb_strings;
};

/*! Function Driver data structure
 *
 * Function driver and its configuration descriptors. 
 *
 * This is passed to the usb-device layer when registering. It contains all
 * required information about the function driver for the usb-device layer
 * to use the function drivers configuration data and to configure this
 * function driver an active configuration.
 *
 * Note that each function driver registers itself on a speculative basis.
 * Whether a function driver is actually configured will depend on the USB
 * HOST selecting one of the function drivers configurations. 
 *
 * This may be done multiple times WRT to either a single bus interface
 * instance or WRT to multiple bus interface instances. In other words a
 * multiple configurations may be selected for a specific bus interface. Or
 * the same configuration may be selected for multiple bus interfaces. 
 *
 */
struct usbd_simple_driver {

        struct usbd_function_driver     driver;

        // device & configuration descriptions 
        struct usbd_device_description  *device_description;
        struct usbd_configuration_description *configuration_description;
        int                             bNumConfigurations;

        u8                              interfaces;         // XXX should be interfaces
        struct usbd_interface_description *interface_list;

        u8                              endpointsRequested;
        struct usbd_endpoint_request    *requestedEndpoints;

        // constructed descriptors
        //struct usbd_device_descriptor   *device_descriptor;

        u8                              iManufacturer;
        u8                              iProduct;
        u8                              iSerialNumber;
        

        //struct usbd_otg_descriptor       *otg_descriptor;
        //struct usbd_configuration_instance *configuration_instance_array;
       
};

/*! Simple configuration structure
 *
 * This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration 
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */
struct usbd_simple_instance {
                                                                // common to all types 
        struct usbd_function_instance   function;

                                                                // composite driver only
        int                             configuration;          // saved value from set configuration

        int                             interface_functions;    // number of names in interfaces_list
        int                             interfaces;             // accumulated total of all bNumInterfaces


        u8                              *altsettings;            // array[0..interfaces-1] of alternate settings 

        struct usbd_endpoint_map        *endpoint_map_array;  

        int                             configuration_size;
        struct usbd_configuration_descriptor  *configuration_descriptor[2];
        u8                              ConfigurationValue;     // current set configuration (zero is default)

};

/*! class configuration structure
 *
 * This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration 
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */
struct usbd_class_instance {
        struct usbd_function_instance   function;

};

struct usbd_class_driver {
        struct usbd_function_driver     driver;
};



/*! interface configuration structure
 *
 * This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration 
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */

struct usbd_interface_driver {
        struct usbd_function_driver     driver;

        u8                              interfaces;         
        struct usbd_interface_description *interface_list;

                                                        // XXX This should moved to interface description
        u8                              endpointsRequested;
        struct usbd_endpoint_request    *requestedEndpoints;

        u8 bFunctionClass;
        u8 bFunctionSubClass;
        u8 bFunctionProtocol;
        char *iFunction;
};

struct usbd_interface_instance {
        struct usbd_function_instance   function;

        int                             wIndex;                 // allocated interface number
        int                             lIndex;                 // logical interface number
        int                             altsetting;             

        int                             endpoints;
        struct usbd_endpoint_map        *endpoint_map_array;  
};

/*! Composite configuration structure
 *
 * This is allocated for each configured instance of a function driver.
 *
 * It stores pointers to the usbd_function_driver for the appropriate function,
 * and pointers to the USB HOST requested usbd_configuration_description and
 * usbd_interface_description.
 *
 * The privdata pointer may be used by the function driver to store private
 * per instance state information.
 *
 * The endpoint map will contain a list of all endpoints for the configuration 
 * driver, and only related endpoints for an interface driver.
 *
 * The interface driver array will be NULL for an interface driver.
 */
struct usbd_composite_driver {
        struct usbd_function_driver     driver;

        // device & configuration descriptions 
        struct usbd_device_description  *device_description;
        struct usbd_configuration_description *configuration_description;
        int                             bNumConfigurations;

        char                            **interface_function_names;      // list of interface function names
        char                            *class_name;
        char                            *windows_os;                    // windows 0xee string 

        u8                              iManufacturer;
        u8                              iProduct;
        u8                              iSerialNumber;

        //struct usbd_otg_descriptor       *otg_descriptor;             // XXX unused
       
};

struct usbd_composite_instance {
                                                                // common to all types 
        struct usbd_function_instance   function;
                                                                // composite driver only
        int                             configuration;          // saved value from set configuration

        int                             interface_functions;    // number of interface functions available
        int                             interfaces;             // accumulated total of all bNumInterfaces

        struct usbd_interface_description *class_list;
        struct usbd_class_instance      *class_instance;
        
        struct usbd_interface_instance   *interfaces_array;     // array of interface instances, one per interface function
        struct usbd_interface_description *interface_list;

        int                             endpoints;
        struct usbd_endpoint_map        *endpoint_map_array;  
        u8                              endpointsRequested;
        struct usbd_endpoint_request    *requestedEndpoints;

        int                             configuration_size;
        struct usbd_configuration_descriptor  *configuration_descriptor[2];
        u8                              ConfigurationValue;     // current set configuration (zero is default)
};
/*
union usbd_instance_union {
        struct usbd_function_instance *function_instance;
        struct usbd_simple_instance *simple_instance;
        struct usbd_composite_instance *composite_instance;
};
*/
/*!
 * @name Function Driver Registration
 *
 * Called by function drivers to register themselves when loaded
 * or de-register when unloading.
 * @{ 
 */
int usbd_register_simple_function (struct usbd_simple_driver *, char *, void *);

int usbd_register_interface_function (struct usbd_interface_driver *, char *, void *);
int usbd_register_class_function (struct usbd_class_driver *function_driver, char*, void *);
int usbd_register_composite_function (struct usbd_composite_driver *function_driver, char*, char*, char **, void *);

void usbd_deregister_simple_function (struct usbd_simple_driver *);
void usbd_deregister_interface_function (struct usbd_interface_driver *);
void usbd_deregister_class_function (struct usbd_class_driver *);
void usbd_deregister_composite_function (struct usbd_composite_driver *);

struct usbd_function_driver *usbd_find_function (char *name);
struct usbd_interface_driver *usbd_find_interface_function(char *name);
struct usbd_class_driver *usbd_find_class_function(char *name);
struct usbd_composite_driver *usbd_find_composite_function(char *name);

//struct usbd_function_instance * usbd_alloc_function (struct usbd_function_driver *, char *, void *);
//struct usbd_function_instance * usbd_alloc_interface_function (struct usbd_function_driver *, char *, void *);
//struct usbd_function_instance * usbd_alloc_class_function (struct usbd_function_driver *function_driver, char*, void *);
//struct usbd_function_instance * usbd_alloc_composite_function (struct usbd_function_driver *function_driver, char*, char*, 
//                char **, void *);

//void usbd_dealloc_function (struct usbd_function_instance *);
//void usbd_dealloc_interface (struct usbd_function_instance *);
//void usbd_dealloc_class (struct usbd_function_instance *);

/*! @} */


/*!
 * @name String Descriptor database
 *
 * @{
 */

struct usbd_string_descriptor *usbd_get_string_descriptor (struct usbd_function_instance *, u8);
u8 usbd_realloc_string (struct usbd_function_instance *, u8, char *);
u8 usbd_alloc_string (struct usbd_function_instance *, char *);
//void usbd_free_string_descriptor(struct usbd_function_instance *, u8 );



//extern struct usbd_string_descriptor **usb_strings;
//void usbd_free_descriptor_strings (struct usbd_descriptor *);

/*! @} */

/*! 
 * @name LANGID's
 *
 * @{ 
 */
#define LANGID_ENGLISH          "\011" 
#define LANGID_US_ENGLISH       "\004"
#define LANGIDs  LANGID_US_ENGLISH LANGID_ENGLISH
/*! @} */


/*! 
 * @name Well Known string indices
 *
 * Used by function drivers to set certain strings
 * @{ 
 */
#define STRINDEX_LANGID     0
#define STRINDEX_IPADDR     1
#define STRINDEX_PRODUCT    2
/*! @} */




/*!
 * @name Device Information
 * 
 * @{
 */
BOOL usbd_high_speed(struct usbd_function_instance *);
int usbd_bmaxpower(struct usbd_function_instance *);
int usbd_endpoint_wMaxPacketSize(struct usbd_function_instance *, int, int);
int usbd_endpoint_zero_wMaxPacketSize(struct usbd_function_instance *, int);
int usbd_endpoint_bEndpointAddress(struct usbd_function_instance *, int, int);
int usbd_get_bMaxPower(struct usbd_function_instance *);
/*! @} */


/*!
 * @name Device Control
 * 
 * @{
 */
usbd_device_state_t usbd_get_device_state(struct usbd_function_instance *);
usbd_device_status_t usbd_get_device_status(struct usbd_function_instance *);
#if 0
int usbd_framenum(struct usbd_function_instance *);
u64 usbd_ticks(struct usbd_function_instance *);
u64 usbd_elapsed(struct usbd_function_instance *, u64 *, u64 *);
#endif
/*! @} */

/*!
 * @name Endpoint I/O
 * 
 * @{
 */

struct usbd_urb *usbd_alloc_urb (struct usbd_function_instance *, int, int length, int (*callback) (struct usbd_urb *, int));
struct usbd_urb *usbd_alloc_urb_ep0 (struct usbd_function_instance *, int length, int (*callback) (struct usbd_urb *, int));
void usbd_free_urb (struct usbd_urb *urb);
int usbd_start_in_urb (struct usbd_urb *urb);
int usbd_start_out_urb (struct usbd_urb *);
int usbd_cancel_urb(struct usbd_urb *);
int usbd_halt_endpoint (struct usbd_function_instance *function, int endpoint_index);
int usbd_endpoint_halted (struct usbd_function_instance *function, int endpoint);


/*! @} */


/*! 
 * @name endpoint information
 *
 * Used by function drivers to get specific endpoint information
 * @{ 
 */

int usbd_endpoint_transferSize(struct usbd_function_instance *, int, int);
int usbd_endpoint_interface(struct usbd_function_instance *, int);
int usbd_interface_AltSetting(struct usbd_function_instance *, int);
int usbd_ConfigurationValue(struct usbd_function_instance *);
void usbd_endpoint_update(struct usbd_function_instance *, int , struct usbd_endpoint_descriptor *, int);
/*! @} */

//int usbd_remote_wakeup_enabled(struct usbd_function_instance *);

/*! 
 * @name device information
 *
 * Used by function drivers to get device feature information
 * @{ 
 */
int usbd_otg_bmattributes(struct usbd_function_instance *);
/*! @} */

/*! 
 * @name usbd_feature_enabled - return non-zero if specified feature has been enabled
 * @{ 
 */
int usbd_feature_enabled(struct usbd_function_instance *function, int f);
/*! @} */

/* DEPRECATED */
/*! 
 * @name Access to function privdata (DEPRECATED)
 * @{ 
 */
void *usbd_function_get_privdata(struct usbd_function_instance *function);
void usbd_function_set_privdata(struct usbd_function_instance *function, void *privdata);
void usbd_flush_endpoint_index (struct usbd_function_instance *, int );
int usbd_get_descriptor (struct usbd_function_instance *function, u8 *buffer, int max, int descriptor_type, int index);
/*! @} */

