/*
 * otg/otgcore/usbp-fops.c - USB Function support
 * @(#) balden@seth2.belcarratech.com|otg/otgcore/usbp-fops.c|20051116205002|59412
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2007 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 02/01/2005         Motorola         Initial distribution
 * 07/14/2005         Motorola         IP adrres in String Desc 1
 * 04/26/2006         Motorola         Fix LANGID and power
 * 10/18/2006         Motorola         Add Open Src Software language
 * 11/28/2006         Motorola         dn Correct kmalloc call for interupt context
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 * 05/21/2007         Motorola         Changes for bmAttributes for self power. 
 * 08/24/2007         Motorola         Changes for Open Src compliance.
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
 * @file otg/otgcore/usbp-fops.c
 * @brief Function driver related functions.
 *
 * This implements the functions used to implement USB Function drivers.
 *
 * @ingroup USBP
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/otg-trace.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>

void urb_append(urb_link *hd, struct usbd_urb *urb);
struct usbd_function_driver *
usbd_find_function_internal(LIST_NODE *usbd_function_drivers, char *name);

extern struct semaphore usbd_bus_sem;

int usbd_strings_init(struct usbd_bus_instance *bus, struct usbd_function_instance *function_instance);
void usbd_strings_exit(struct usbd_bus_instance *bus);
void usbd_free_string_descriptor (struct usbd_bus_instance *bus, u8 index);

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Simple Function Registration:
 *	usbd_register_simple_function()
 *	usbd_deregister_simple_function()
 *
 */

LIST_NODE_INIT(usbd_simple_drivers);            // list of all registered configuration function modules

/*!
 * @brief usbd_register_simple_function() - register a usbd function driver
 *
 * USBD Simple Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param simple_driver
 * @param name
 * @param privdata
 * @return function instance
 *
 */
int
usbd_register_simple_function (struct usbd_simple_driver *simple_driver, char * name, void *privdata)
{
        TRACE_MSG1(USBD, "SIMPLE function: %s", name);
        DOWN(&usbd_bus_sem);
        simple_driver->driver.name = name;
        simple_driver->driver.privdata = privdata;
        simple_driver->driver.function_type = function_simple;
        LIST_ADD_TAIL (&simple_driver->driver.drivers, &usbd_simple_drivers);
        simple_driver->driver.flags |= FUNCTION_REGISTERED;
        UP(&usbd_bus_sem);
        return 0;
}

/*! 
 * @brief usbd_deregister_simple_function() - de-register a usbd function driver instance
 *
 * USBD Function drivers call this to de-register with the USBD Core layer.
 * @param simple_driver
 */
void usbd_deregister_simple_function (struct usbd_simple_driver *simple_driver)
{
        RETURN_UNLESS (simple_driver);
        TRACE_MSG1(USBD, "SIMPLE function: %s", simple_driver->driver.name);
        DOWN(&usbd_bus_sem);
        if (simple_driver->driver.flags & FUNCTION_ENABLED) {
                // XXX ERROR need to disable
        }
        if (simple_driver->driver.flags & FUNCTION_REGISTERED) {
                simple_driver->driver.flags &= ~FUNCTION_REGISTERED;
                LIST_DEL (simple_driver->driver.drivers);
        }
        UP(&usbd_bus_sem);
}

/*!
 * @brief usbp_find_simple_driver() - register a usbd function driver
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param name
 * @return function instance
 *
 */
struct usbd_simple_driver *
usbp_find_simple_driver (char *name)
{
        return (struct usbd_simple_driver *) usbd_find_function_internal(&usbd_simple_drivers, name);
}

OTG_EXPORT_SYMBOL(usbd_register_simple_function);
OTG_EXPORT_SYMBOL(usbd_deregister_simple_function);

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!     
 * Function Registration:
 *      usbd_register_class_function_function()
 *      usbd_register_interface_function()
 *      usbd_register_composite_function()
 *      usbd_deregister_class_function()
 *      usbd_deregister_interface_function()
 *      usbd_deregister_composite_function()
 */

LIST_NODE_INIT(usbd_interface_drivers);         // list of all registered interface function modules
LIST_NODE_INIT(usbd_class_drivers);             // list of all registered composite function modules
LIST_NODE_INIT(usbd_composite_drivers);             // list of all registered composite function modules

/*!
 * @brief usbd_register_interface_function() - register a usbd function driver
 *
 * USBD Interface Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *      
 * @param interface_driver
 * @param name
 * @param privdata
 * @return function instance
 *
 */
int
usbd_register_interface_function(struct usbd_interface_driver *interface_driver, char * name, void *privdata)
{
        TRACE_MSG1(USBD, "INTERFACE function: %s", name);
        DOWN(&usbd_bus_sem);
        interface_driver->driver.name = name;
        interface_driver->driver.privdata = privdata;
        interface_driver->driver.function_type = function_interface;
        LIST_ADD_TAIL (&interface_driver->driver.drivers, &usbd_interface_drivers);
        interface_driver->driver.flags |= FUNCTION_REGISTERED;
        UP(&usbd_bus_sem);
        return 0;
}

/*!
 * @brief usbd_register_class_function() - register a usbd function driver
 *
 * USBD composite Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param class_driver
 * @param name
 * @param privdata
 * @return function instance
 *
 */
int
usbd_register_class_function(struct usbd_class_driver *class_driver, char * name, void *privdata)
{
        TRACE_MSG1(USBD, "CLASS function: %s", name);
        DOWN(&usbd_bus_sem);
        class_driver->driver.name = name;
        class_driver->driver.privdata = privdata;
        class_driver->driver.function_type = function_class;
        LIST_ADD_TAIL (&class_driver->driver.drivers, &usbd_class_drivers);
        class_driver->driver.flags |= FUNCTION_REGISTERED;
        UP(&usbd_bus_sem);
        return 0;
}

/*!
 * usbd_register_composite_function() - register a usbd function driver
 *
 * USBD composite Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param composite_driver
 * @param name
 * @param class_name
 * @param interface_function_names
 * @param privdata
 * @return function instance
 *
 */
int
usbd_register_composite_function (struct usbd_composite_driver *composite_driver, char* name,
                char* class_name, char **interface_function_names, void *privdata)
{
        TRACE_MSG1(USBD, "COMPOSITE function: %s", name);
        DOWN(&usbd_bus_sem);
        composite_driver->driver.name = name;
        composite_driver->driver.privdata = privdata;
        composite_driver->driver.function_type = function_composite;
        composite_driver->class_name = class_name;
        composite_driver->interface_function_names = interface_function_names;
        LIST_ADD_TAIL (&composite_driver->driver.drivers, &usbd_composite_drivers);
        composite_driver->driver.flags |= FUNCTION_REGISTERED;
        UP(&usbd_bus_sem);
        return 0;
}

/*! 
 * usbd_deregister_interface_function() - de-register a usbd function driver instance
 *
 * USBD Function drivers call this to de-register with the USBD Core layer.
 * @param interface_driver
 */
void usbd_deregister_interface_function (struct usbd_interface_driver *interface_driver)
{
        RETURN_UNLESS (interface_driver);
        TRACE_MSG1(USBD, "INTERFACE function: %s", interface_driver->driver.name);
        DOWN(&usbd_bus_sem);
        if (interface_driver->driver.flags & FUNCTION_ENABLED) {
                // XXX ERROR need to disable
        }
        if (interface_driver->driver.flags & FUNCTION_REGISTERED) {
                interface_driver->driver.flags &= ~FUNCTION_REGISTERED;
                LIST_DEL (interface_driver->driver.drivers);
        }
        UP(&usbd_bus_sem);
}

/*! 
 * @brief usbd_deregister_class_function() - de-register a usbd function driver instance
 *
 * USBD Function drivers call this to de-register with the USBD Core layer.
 * @param class_driver
 */
void usbd_deregister_class_function (struct usbd_class_driver *class_driver)
{
        RETURN_UNLESS (class_driver);
        TRACE_MSG1(USBD, "CLASS function: %s", class_driver->driver.name);
        DOWN(&usbd_bus_sem);
        if (class_driver->driver.flags & FUNCTION_ENABLED) {
                // XXX ERROR need to disable
        }
        if (class_driver->driver.flags & FUNCTION_REGISTERED) {
                class_driver->driver.flags &= ~FUNCTION_REGISTERED;
                LIST_DEL (class_driver->driver.drivers);
        }
        UP(&usbd_bus_sem);
}

/*! 
 * @brief usbd_deregister_composite_function() - de-register a usbd function driver instance
 *
 * USBD Function drivers call this to de-register with the USBD Core layer.
 * @param composite_driver
 */
void usbd_deregister_composite_function (struct usbd_composite_driver *composite_driver)
{
        RETURN_UNLESS (composite_driver);
        TRACE_MSG1(USBD, "COMPOSITE function: %s", composite_driver->driver.name);
        DOWN(&usbd_bus_sem);
        if (composite_driver->driver.flags & FUNCTION_ENABLED) {
                // XXX ERROR need to disable
        }
        if (composite_driver->driver.flags & FUNCTION_REGISTERED) {
                composite_driver->driver.flags &= ~FUNCTION_REGISTERED;
                LIST_DEL (composite_driver->driver.drivers);
        }
        UP(&usbd_bus_sem);
}

/*!
 * @brief usbp_find_interface_driver() - register a usbd function driver
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param name
 * @return function instance
 *
 */
struct usbd_interface_driver *usbp_find_interface_driver(char *name)
{
        TRACE_MSG1(USBD, "%s", name ? name : "");
        return (struct usbd_interface_driver *)  usbd_find_function_internal(&usbd_interface_drivers, name);
}


/*!
 * @brief usbp_find_class_driver() - register a usbd function driver
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param name
 * @return function instance
 *
 */
struct usbd_class_driver *usbp_find_class_driver(char *name)
{
        TRACE_MSG1(USBD, "%s", name);
        return (struct usbd_class_driver *)  usbd_find_function_internal(&usbd_class_drivers, name);
}

/*!
 * @brief usbp_find_composite_driver() - register a usbd function driver
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * This will typically only be called within usbd_bus_sem protected area.
 *
 * @param name
 * @return function instance
 *
 */
struct usbd_composite_driver *usbp_find_composite_driver(char *name)
{
        TRACE_MSG1(USBD, "%s", name);
        return (struct usbd_composite_driver *) usbd_find_function_internal(&usbd_composite_drivers, name);
}


OTG_EXPORT_SYMBOL(usbd_register_interface_function);
OTG_EXPORT_SYMBOL(usbd_register_composite_function);
OTG_EXPORT_SYMBOL(usbd_register_class_function);
OTG_EXPORT_SYMBOL(usbd_deregister_interface_function);
OTG_EXPORT_SYMBOL(usbd_deregister_class_function);
OTG_EXPORT_SYMBOL(usbd_deregister_composite_function);


/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! 
 * @brief copy_descriptor() - copy data into buffer
 *
 * @param cp 
 * @param data
 * @return bLength
 */
static int copy_descriptor (u8 *cp, void *data)
{
        RETURN_ZERO_UNLESS (data);
        memcpy (cp, data, ((struct usbd_generic_descriptor *)data)->bLength);
        return ((struct usbd_generic_descriptor *)data)->bLength;
}       


/*! 
 * @brief usbp_alloc_simple_configuration_descriptor() - 
 *
 * @param simple_instance
 * @param cfg_size
 * @param hs
 * @return pointer to configuration descriptor
 */
void usbp_alloc_simple_configuration_descriptor(struct usbd_simple_instance *simple_instance, int cfg_size, int hs)
{
        struct usbd_simple_driver *simple_driver = (struct usbd_simple_driver *)
			 simple_instance->function.function_driver;

        struct usbd_configuration_description *configuration_description;
        struct usbd_configuration_descriptor *configuration_descriptor = NULL;


        //int cfg_size = sizeof(struct usbd_configuration_descriptor) + sizeof(struct usbd_otg_descriptor);
        u8 *cp = NULL;

        int i, j, k, l;

        configuration_description = simple_driver->configuration_description;

        configuration_descriptor = simple_instance->configuration_descriptor[hs];
        
        cfg_size = sizeof(struct usbd_configuration_descriptor) + sizeof(struct usbd_otg_descriptor);

        /* Compose the configuration descriptor
         * First copy the function driver configuration configuration descriptor.
         */
        cp = (u8 *)configuration_descriptor;
        cfg_size = sizeof(struct usbd_configuration_descriptor);
        configuration_descriptor->bLength = sizeof(struct usbd_configuration_descriptor);
        configuration_descriptor->bDescriptorType = USB_DT_CONFIGURATION;
        configuration_descriptor->bmAttributes = configuration_description->bmAttributes;
        configuration_descriptor->bMaxPower = configuration_description->bMaxPower;
        configuration_descriptor->iConfiguration = usbd_alloc_string (&simple_instance->function, 
                        configuration_description->iConfiguration);


        for (i = 0; i < simple_driver->interfaces; i++) {

                struct usbd_interface_description *interface_description = simple_driver->interface_list + i;

                TRACE_MSG3(USBD, "INTERFACE[%02d] interface_description: %x alternates: %d", i,
                                interface_description, interface_description->alternates);

                for (k = 0; k < interface_description->alternates; k++) {

                        struct usbd_alternate_description *alternate_description = interface_description->alternate_list + k;

                        struct usbd_interface_descriptor *interface_descriptor;

                        TRACE_MSG2(USBD, "ALTERNATE[%02d:%02d]", i, k);

                        /* copy alternate interface descriptor, update descriptor fields
                         */
                        interface_descriptor = (struct usbd_interface_descriptor *)(cp + cfg_size);

                        cfg_size += sizeof(struct usbd_interface_descriptor);

                        interface_descriptor->bLength = sizeof(struct usbd_interface_descriptor);
                        interface_descriptor->bDescriptorType = USB_DT_INTERFACE;
                        interface_descriptor->bInterfaceNumber = i;
                        interface_descriptor->bAlternateSetting = k;
                        interface_descriptor->bNumEndpoints = alternate_description->endpoints;

                        interface_descriptor->bInterfaceClass = alternate_description->bInterfaceClass;
                        interface_descriptor->bInterfaceSubClass = alternate_description->bInterfaceSubClass;
                        interface_descriptor->bInterfaceProtocol = alternate_description->bInterfaceProtocol;
                        // XXX
                        interface_descriptor->iInterface = usbd_alloc_string (&simple_instance->function, 
                                        alternate_description->iInterface);

                        TRACE_MSG6(USBD, "COPY  ALT[%02d:%02d] classes: %d endpoints: %d %s size: %d",
                                        i, k, alternate_description->classes, alternate_description->endpoints, 
                                        alternate_description->iInterface, cfg_size);

                        /* copy class descriptors 
                         */
                        for (l = 0; l < alternate_description->classes; l++) 
                                cfg_size += copy_descriptor(cp + cfg_size, *(alternate_description->class_list + l));
                        
                        /* copy endpoint descriptors, update descriptor fields
                         */
                        for (l = 0; l < alternate_description->endpoints; l++) {
                                int index = alternate_description->endpoint_index[l];
                                struct usbd_endpoint_map *endpoint_map = simple_instance->endpoint_map_array + index;
                                struct usbd_endpoint_descriptor *endpoint_descriptor = 
                                        (struct usbd_endpoint_descriptor *)(cp + cfg_size);
                  
                                cfg_size += sizeof(struct usbd_endpoint_descriptor);
                                endpoint_descriptor->bLength = sizeof(struct usbd_endpoint_descriptor);;
                                endpoint_descriptor->bDescriptorType = USB_DT_ENDPOINT;
                                endpoint_descriptor->bEndpointAddress = endpoint_map->bEndpointAddress[hs];
                                endpoint_descriptor->bmAttributes = endpoint_map->bmAttributes[hs] & 0x3;
                                endpoint_descriptor->wMaxPacketSize = cpu_to_le16(endpoint_map->wMaxPacketSize[hs]);
                                endpoint_descriptor->bInterval = endpoint_map->bInterval[hs];
                        }
                }
        }
        TRACE_MSG2(USBD, "cfg: %x size: %d", cp, cfg_size);

        /* XXX will need to correctly set bMaxPower and bmAttributes
         */
        configuration_descriptor->wTotalLength = cpu_to_le16(cfg_size);

        if( (configuration_description->bMaxPower <= 0 ) ||
				(configuration_description->bMaxPower >= 0xfa))
		{
            configuration_descriptor->bMaxPower =  0xfa;

			configuration_descriptor->bmAttributes = BMATTRIBUTE_RESERVED| BMATTRIBUTE_SELF_POWERED;
			
		}
		else if( ( configuration_description->bMaxPower <= 0x32) || 
				 ( ( configuration_description->bMaxPower > 0x32) &&
				   (configuration_description->bMaxPower <= 0xfa)))
		{
		   configuration_descriptor->bMaxPower = 0x32;
		   configuration_descriptor->bmAttributes = BMATTRIBUTE_RESERVED |
			                                        BMATTRIBUTE_SELF_POWERED | BMATTRIBUTE_REMOTE_WAKEUP;
		}
		   
        configuration_descriptor->bNumInterfaces = simple_driver->interfaces;
        configuration_descriptor->bConfigurationValue = 1;

        for (i = 0; i < cfg_size; i+= 8) {
                TRACE_MSG8(USBD, "%02x %02x %02x %02x %02x %02x %02x %02x",
                                cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3], 
                                cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                );
        }
        /*
        return configuration_descriptor;
        CATCH(error) {
                TRACE_MSG0(USBD, "FAILED");
                if (configuration_descriptor) LKFREE(configuration_descriptor);
                return NULL;
        }
        */
}

/*! 
 * @brief usbp_dealloc_simple_instance() - 
 *
 * @param simple_instance
 */
void usbp_dealloc_simple_instance(struct usbd_simple_instance *simple_instance)
{
    usbd_strings_exit(simple_instance->function.bus);
	LKFREE(simple_instance);
}

/*! 
 * @brief usbp_alloc_simple_instance() - 
 *
 * @param simple_driver
 * @param bus
 * @return usbd_simple_instance
 */
struct usbd_simple_instance *
usbp_alloc_simple_instance(struct usbd_simple_driver *simple_driver,
		struct usbd_bus_instance *bus)
{
        struct usbd_simple_instance *simple_instance = NULL;
	int i, k, l;
        int cfg_size = sizeof(struct usbd_configuration_descriptor) + sizeof(struct usbd_otg_descriptor);
        struct usbd_configuration_descriptor *configuration_descriptor[2] = { NULL, NULL, };
	struct usbd_endpoint_map *endpoint_map_array = NULL;
	u8 *altsettings = NULL;

        THROW_UNLESS((simple_instance = CKMALLOC(sizeof(struct usbd_simple_instance), GFP_KERNEL)), error);
        simple_instance->function.bus = bus;
        THROW_IF(usbd_strings_init(bus, &simple_instance->function), error);

        /* copy fields
         */
        simple_instance->function.function_driver = (struct usbd_function_driver *)simple_driver;
        simple_instance->function.function_type = function_simple;
        simple_instance->function.name = simple_driver->driver.name;
	simple_instance->function.bus = bus;

        /* 
         * Find and total descriptor sizes.
         */
        for (i = 0; i < simple_driver->interfaces; i++) {

                struct usbd_interface_description *interface_description = simple_driver->interface_list + i;

                for (k = 0; k < interface_description->alternates; k++) {

                        struct usbd_alternate_description *alternate_description = 
                                interface_description->alternate_list + k;

                        /* size interface descriptor
                         */
                        cfg_size += sizeof(struct usbd_interface_descriptor);;

                        TRACE_MSG6(USBD, "SIZE ALT[%02d:%02d] classes: %d endpoints: %d %s size: %d",
                                        i, k, alternate_description->classes,
                                        alternate_description->endpoints, alternate_description->iInterface, cfg_size);

                        /* size class descriptors
                         */
                        for (l = 0; l < alternate_description->classes; l++) {
                                struct usbd_generic_class_descriptor *class_descriptor =
                                        *(alternate_description->class_list + l);
                                cfg_size += class_descriptor->bLength;
                        }

                        cfg_size += alternate_description->endpoints * sizeof(struct usbd_endpoint_descriptor);

                        #if 0
                        /* size endpoint descriptors (in theory could be endpoints * sizeof())
                         */
                        for (l = 0; l < alternate_description->endpoints; l++) {
                                struct usbd_endpoint_descriptor *endpoint_descriptor =
                                        *(alternate_description->endpoint_list + l);
                                cfg_size += endpoint_descriptor->bLength;
                        }
                        #endif

                }
        }
        TRACE_MSG1(USBD, "cfg_size: %d", cfg_size);

        /* allocate configuration descriptor
         */
        /*
        THROW_UNLESS((configuration_descriptor = (struct usbd_configuration_descriptor *)
                                CKMALLOC (cfg_size + 4, GFP_KERNEL)), error);
        simple_instance->configuration_descriptor = configuration_descriptor;
        */
        #ifdef CONFIG_OTG_HIGH_SPEED
        for (i = 0; i < 2; i++) 
        #else /* CONFIG_OTG_HIGH_SPEED */
        for (i = 0; i < 1; i++) 
        #endif /* CONFIG_OTG_HIGH_SPEED */
        {
                THROW_UNLESS((configuration_descriptor[i] = (struct usbd_configuration_descriptor *)
                                        CKMALLOC (cfg_size + 4, GFP_KERNEL)), error);
                simple_instance->configuration_descriptor[i] = configuration_descriptor[i];
        }
        simple_instance->configuration_size = cfg_size;

        /* allocate endpoint map array and altsetting
         */
        THROW_UNLESS((endpoint_map_array = (struct usbd_endpoint_map *) CKMALLOC (sizeof(struct usbd_endpoint_map) * 
                                                simple_driver->endpointsRequested, GFP_KERNEL)), error); 
        simple_instance->endpoint_map_array = endpoint_map_array;

        THROW_UNLESS((altsettings = (u8 *)CKMALLOC(sizeof (u8) * simple_instance->interfaces, GFP_KERNEL)), error);
        simple_instance->altsettings = altsettings;

        TRACE_MSG1(USBD, "request endpoints: %x", simple_driver->endpointsRequested);

        /* request endpoints, allocate configuration descriptor, updating endpoint descriptors with correct
         * information from endpoint map allocated above and set the endpoints 
         */

        THROW_IF(bus->driver->bops->set_endpoints( bus, simple_driver->endpointsRequested,
                                simple_instance->endpoint_map_array), error);

        return simple_instance;

        CATCH(error) {
                TRACE_MSG0(USBD, "FAILED");
                for (i = 0; i < 2; i++) if (configuration_descriptor[i]) LKFREE(configuration_descriptor[i]);
                if (altsettings) LKFREE(altsettings);
                if (endpoint_map_array) LKFREE(endpoint_map_array);
                if (simple_instance) {
                        usbd_strings_exit(bus);
                        usbp_dealloc_simple_instance(simple_instance);
                }
                return NULL;
        }
}
/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*! 
 * @brief usbp_dealloc_composite_instance() - 
 *
 * @param composite_instance
 */
void usbp_dealloc_composite_instance(struct usbd_composite_instance *composite_instance)
{       
	int i;
        for (i = 0; i < 2; i++) 
                if (composite_instance->configuration_descriptor[i])
                        LKFREE(composite_instance->configuration_descriptor[i]);

        usbd_strings_exit(composite_instance->function.bus);
        if (composite_instance->interfaces_array) LKFREE(composite_instance->interfaces_array);
	if (composite_instance->endpoint_map_array) LKFREE(composite_instance->endpoint_map_array);
        if (composite_instance->requestedEndpoints) LKFREE(composite_instance->requestedEndpoints);
        if (composite_instance->interface_list) LKFREE(composite_instance->interface_list);

        LKFREE(composite_instance);
}       


/*! 
 * @brief usbp_alloc_composite_configuration_descriptor() - 
 *
 * @param composite_instance
 * @param cfg_size
 * @param hs
 * @return pointer to configuration descriptor
 */
void usbp_alloc_composite_configuration_descriptor(struct usbd_composite_instance *composite_instance, int cfg_size, int hs)
{
        struct usbd_composite_driver *composite_driver = (struct usbd_composite_driver*)
		composite_instance->function.function_driver;
        struct usbd_configuration_description *configuration_description = composite_driver->configuration_description;

        struct usbd_configuration_descriptor *configuration_descriptor = composite_instance->configuration_descriptor[hs];
        u8 *cp = NULL;

        int i, j, k, l, m;

        /* Compose the configuration descriptor
         * First copy the function driver configuration configuration descriptor.
         */
        cp = (u8 *)configuration_descriptor;
        
        TRACE_MSG3(USBD, "bDeviceClass: %02x descriptor[%d] %x", 
                        composite_driver->device_description->bDeviceClass, hs, 
                        composite_instance->configuration_descriptor[hs]); 

        cfg_size = sizeof(struct usbd_configuration_descriptor);
        configuration_descriptor->bLength = sizeof(struct usbd_configuration_descriptor);
        configuration_descriptor->bDescriptorType = USB_DT_CONFIGURATION;
        configuration_descriptor->bmAttributes = configuration_description->bmAttributes;
        configuration_descriptor->bMaxPower = configuration_description->bMaxPower;
        configuration_descriptor->iConfiguration = usbd_alloc_string (&composite_instance->function,
                        configuration_description->iConfiguration);

        for (i = 0; i < composite_instance->interface_functions; i++) {

                struct usbd_interface_instance *interface_instance = composite_instance->interfaces_array +i;
		struct usbd_interface_driver *interface_driver = (struct usbd_interface_driver *)
			interface_instance->function.function_driver;

                TRACE_MSG6(USBD, "INTERFACE[%02d] interfaces: %d endpoints: %d bFunctionClass: %d hs: %d %s", i, 
                                interface_driver->interfaces, interface_driver->endpointsRequested, 
                                interface_driver->bFunctionClass, hs,
                                interface_driver->driver.name);

                /* insert Interface Association Descriptor if required
                 */
                if ((USB_CLASS_MISC == composite_driver->device_description->bDeviceClass) /* && 
                                (interface_driver->interfaces > 1)*/ ) 
                {

                        struct usbd_interface_association_descriptor *iad =
                                (struct usbd_interface_association_descriptor *) (cp + cfg_size);

                        TRACE_MSG4(USBD, "      IAD[%02d] Class: %d SubClass: %d Protocol: %d", i, 
                                        interface_driver->bFunctionClass, interface_driver->bFunctionSubClass,
                                        interface_driver->bFunctionProtocol);

                        cfg_size += sizeof(struct usbd_interface_association_descriptor);
                        iad->bLength = sizeof(struct usbd_interface_association_descriptor);
                        iad->bDescriptorType = USB_DT_INTERFACE_ASSOCIATION;
                        iad->bFirstInterface = interface_instance->wIndex;
                        iad->bInterfaceCount = interface_driver->interfaces;
                        iad->bFunctionClass = interface_driver->bFunctionClass;
                        iad->bFunctionSubClass = interface_driver->bFunctionSubClass;
                        iad->bFunctionProtocol = interface_driver->bFunctionProtocol;
                        iad->iFunction = usbd_alloc_string (&composite_instance->function,
                                        interface_driver->iFunction);
                }

                /* iterate across interfaces and alternates for this for this function
                 * XXX this will need to be modified if interface functions are allowed
                 * to define multiple interfaces.
                 */

                for (j = 0; j < interface_driver->interfaces; j++) {

                        struct usbd_interface_description *interface_description = interface_driver->interface_list + j;

                        TRACE_MSG5(USBD, "INTERFACE[%02d:%02d] interface_description: %x alternates: %d wIndex: %d", i, j, 
                                        interface_description, interface_description->alternates, 
					interface_instance->wIndex);

                        for (k = 0; k < interface_description->alternates; k++) {

                                struct usbd_alternate_description *alternate_description = 
                                        interface_description->alternate_list + k;

                                struct usbd_interface_descriptor *interface_descriptor;

                                TRACE_MSG3(USBD, "ALTERNATE[%02d:%02d:%02d]", i, j, k);

                                /* copy alternate interface descriptor, update descriptor fields
                                 */
                                interface_descriptor = (struct usbd_interface_descriptor *)(cp + cfg_size);

                                cfg_size += sizeof(struct usbd_interface_descriptor);

                                //interface_descriptor->bInterfaceNumber = i;     // XXX see note above
                                interface_descriptor->bLength = sizeof(struct usbd_interface_descriptor);
                                interface_descriptor->bDescriptorType = USB_DT_INTERFACE;
                                interface_descriptor->bInterfaceNumber = interface_instance->wIndex + j;
                                interface_descriptor->bAlternateSetting = k;
                                interface_descriptor->bNumEndpoints = alternate_description->endpoints;

                                interface_descriptor->bInterfaceClass = alternate_description->bInterfaceClass;
                                interface_descriptor->bInterfaceSubClass = alternate_description->bInterfaceSubClass;
                                interface_descriptor->bInterfaceProtocol = alternate_description->bInterfaceProtocol;

                                interface_descriptor->iInterface = usbd_alloc_string (&composite_instance->function, 
                                                alternate_description->iInterface);

                                TRACE_MSG7(USBD, "COPY  ALT[%02d:%02d:%02d] classes: %d endpoints: %d %s size: %d", i, j, k, 
                                                alternate_description->classes, alternate_description->endpoints, 
                                                alternate_description->iInterface, cfg_size);

                                /* copy class descriptors 
                                 */
                                for (l = 0; l < alternate_description->classes; l++) {
                                        struct usbd_generic_class_descriptor *generic_descriptor = 
                                                (struct usbd_generic_class_descriptor *)(cp + cfg_size);

                                        cfg_size += copy_descriptor(cp + cfg_size, *(alternate_description->class_list + l));

                                        TRACE_MSG7(USBD, "COPY  CLS[%02d:%02d:%02d:%02d] type: %02x:%02x:%02x", 
                                                        i, j, k, l, 
                                                        generic_descriptor->bLength,
                                                        generic_descriptor->bDescriptorType,
                                                        generic_descriptor->bDescriptorSubtype
                                                        ); 

                                        /* update 
                                         *      call management functional descriptor
                                         *      union functional descriptor
                                         * and update interface numbers
                                         */
                                        CONTINUE_UNLESS (USB_DT_CLASS_SPECIFIC == generic_descriptor->bDescriptorType);
                                        switch (generic_descriptor->bDescriptorSubtype) {
                                        case USB_ST_CMF: {
                                                struct usbd_call_management_functional_descriptor *cmfd =
                                                        (struct usbd_call_management_functional_descriptor *) 
                                                        generic_descriptor;
                                                cmfd->bDataInterface += interface_instance->wIndex + j;
                                                TRACE_MSG5(USBD, "COPY  CLS[%02d:%02d:%02d:%02d] CMF bDataInterface %02x", 
                                                        i, j, k, l, cmfd->bDataInterface);
                                                break;
                                        }
                                        case USB_ST_UF: {
                                                struct usbd_union_functional_descriptor *ufd =
                                                        (struct usbd_union_functional_descriptor *) 
                                                        generic_descriptor;
                                                ufd->bMasterInterface = interface_instance->wIndex + j;
                                                TRACE_MSG6(USBD, 
                                                "COPY  CLS[%02d:%02d:%02d:%02d] UFD bMasterInterface %02x bSlaveInterface %02x", 
                                                                i, j, k, l, 
                                                                ufd->bMasterInterface,
                                                                ufd->bSlaveInterface[0]
                                                                );

                                                for (m = 0; m < interface_driver->interfaces - 1; m++) {


                                                        TRACE_MSG7(USBD, 
                                                        "COPY  CLS[%02d:%02d:%02d:%02d] UFD bSlaveInterface[%02x] %02x ->%02x",
                                                                        i, j, k, l, m, 
                                                                        ufd->bSlaveInterface[m],
                                                                        ufd->bSlaveInterface[m] + ufd->bMasterInterface
                                                                        );
                                                        ufd->bSlaveInterface[m] += ufd->bMasterInterface;
                                                }
                                                break;
                                        }
                                        default: break;
                                        }
                                }
                                
                                /* copy endpoint descriptors, update descriptor fields
                                 */
                                for (l = 0; l < alternate_description->endpoints; l++) {
                                        int index = alternate_description->endpoint_index[l];
                                        struct usbd_endpoint_map *endpoint_map = interface_instance->endpoint_map_array + index;
                                        struct usbd_endpoint_descriptor *endpoint_descriptor = 
                                                (struct usbd_endpoint_descriptor *)(cp + cfg_size);

                                        cfg_size += sizeof(struct usbd_endpoint_descriptor);
                                        endpoint_descriptor->bLength = sizeof(struct usbd_endpoint_descriptor);;
                                        endpoint_descriptor->bDescriptorType = USB_DT_ENDPOINT;
                                        endpoint_descriptor->bEndpointAddress = endpoint_map->bEndpointAddress[hs];
                                        endpoint_descriptor->bmAttributes = endpoint_map->bmAttributes[hs] & 0x3;
                                        endpoint_descriptor->wMaxPacketSize = cpu_to_le16(endpoint_map->wMaxPacketSize[hs]);
                                        endpoint_descriptor->bInterval = endpoint_map->bInterval[hs];
                                }
                        }
                }
        }

        #if defined(CONFIG_OTG_BDEVICE_WITH_SRP) || defined(CONFIG_OTG_DEVICE) 
        {
                struct usbd_otg_descriptor *otg_descriptor = (struct usbd_otg_descriptor *)(cp + cfg_size);
                otg_descriptor->bLength = sizeof(struct usbd_otg_descriptor);
                otg_descriptor->bDescriptorType = USB_DT_OTG;
                otg_descriptor->bmAttributes = USB_OTG_HNP_SUPPORTED | USB_OTG_SRP_SUPPORTED;
                cfg_size += sizeof(struct usbd_otg_descriptor);
        }
        #endif /* defined(CONFIG_OTG_BDEVICE_WITH_SRP) || defined(CONFIG_OTG_DEVICE) */
        /* XXX will need to correctly set bMaxPower and bmAttributes
         */
        configuration_descriptor->wTotalLength = cpu_to_le16(cfg_size);

		if( (configuration_description->bMaxPower <= 0 ) ||
				(configuration_description->bMaxPower >= 0xfa))
		{
            configuration_descriptor->bMaxPower =  0xfa;

			configuration_descriptor->bmAttributes = BMATTRIBUTE_RESERVED| BMATTRIBUTE_SELF_POWERED;
			
		}
		else if( ( configuration_description->bMaxPower <= 0x32) || 
				 ( ( configuration_description->bMaxPower > 0x32) &&
				   (configuration_description->bMaxPower <= 0xfa)))
		{
		   configuration_descriptor->bMaxPower = 0x32;
		   configuration_descriptor->bmAttributes = BMATTRIBUTE_RESERVED |
			                                        BMATTRIBUTE_SELF_POWERED | BMATTRIBUTE_REMOTE_WAKEUP;
		}

        configuration_descriptor->bNumInterfaces = composite_instance->interfaces;
        configuration_descriptor->bConfigurationValue = 1;

        TRACE_MSG2(USBD, "cfg: %x size: %d", cp, cfg_size);
        for (i = 0; i < cfg_size; i+= 8) {
                TRACE_MSG8(USBD, "%02x %02x %02x %02x %02x %02x %02x %02x",
                                cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3], 
                                cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                          );
        }
        /*
        return configuration_descriptor;
        CATCH(error) {
                TRACE_MSG0(USBD, "FAILED");
                if (configuration_descriptor) LKFREE(configuration_descriptor);
                return NULL;
        }
        */
}


/*! 
 * alloc_function_interface_function() - 
 *
 * @param interface_driver
 * @return pointer to interface instance
 */
struct usbd_interface_instance *
alloc_function_interface_function(struct usbd_interface_driver *interface_driver)
{
        struct usbd_interface_instance *function_instance = NULL;
        RETURN_NULL_UNLESS((function_instance = CKMALLOC(sizeof(struct usbd_interface_instance), GFP_KERNEL)));
        function_instance->function.function_type = function_interface;
        function_instance->function.name = interface_driver->driver.name;
        return function_instance;
}


/*! 
 * alloc_function_class_function() - 
 *
 * @param class_driver
 * @return pointer to class instance
 */
struct usbd_class_instance *
alloc_function_class_function(struct usbd_class_driver *class_driver)
{
        struct usbd_class_instance *function_instance = NULL;
        RETURN_NULL_UNLESS((function_instance = CKMALLOC(sizeof(struct usbd_class_instance), GFP_KERNEL)));
        function_instance->function.function_type = function_class;
        function_instance->function.name = class_driver->driver.name;
        return function_instance;
}

/*!
 * @brief usbp_alloc_composite_instance() - register a usbd function driver
 *
 * USBD Composite Function drivers call this to register with the USBD Core layer.
 *
 * This takes a list of interface functions (by name) to use. Each of these
 * must be found and the interface descriptor and endpoint request information
 * must be copied.
 *
 * @param composite_driver
 * @param bus
  *
 * @return function instance or  NULL on failure.
 *
 */
struct usbd_composite_instance *
usbp_alloc_composite_instance (struct usbd_composite_driver *composite_driver, struct usbd_bus_instance *bus)
{
        struct usbd_composite_instance *composite_instance = NULL;
        struct usbd_class_instance *class_instnace = NULL;
        struct usbd_class_driver *class_driver = NULL;
        struct usbd_interface_instance *interfaces_array = NULL;
        struct usbd_endpoint_request *requestedEndpoints = NULL;
        struct usbd_endpoint_map *endpoint_map_array = NULL;
        struct usbd_interface_description *interface_list = NULL;

        u32 interfaces_used;

	char ** interface_function_names = composite_driver->interface_function_names;

        int i = 0, j = 0, k, l;
        int endpoint_number, interface_number, function_number = 0;
        u8 *cp;

        int cfg_size = sizeof(struct usbd_configuration_descriptor) + sizeof(struct usbd_otg_descriptor);

	THROW_UNLESS((composite_instance = CKMALLOC(sizeof(struct usbd_composite_instance), GFP_KERNEL)), error);
        composite_instance->function.bus = bus;
        THROW_IF(usbd_strings_init(bus, &composite_instance->function), error);

        /* copy fields
         */
        composite_instance->function.function_driver = (struct usbd_function_driver *)composite_driver;
        composite_instance->function.function_type = function_composite;
        composite_instance->function.name = composite_driver->driver.name;
        composite_instance->function.bus = bus;

        /* count and list interfaces, then allocate interfaces_array.
         */
        for (endpoint_number = interface_number = function_number = 0; 
		*(interface_function_names + function_number); 
		function_number++) 
	{
		char *interface_name = *(interface_function_names + function_number); 
		char *cp;
                struct usbd_interface_driver *interface_driver;
                int interfaces;

		if ((cp = strchr(interface_name, '='))) 
			interface_name = cp + 1;
		

                TRACE_MSG1(USBD, "interface_name: \"%s\"", interface_name ? interface_name : "");

                THROW_UNLESS(interface_driver = usbp_find_interface_driver(interface_name), error);

                TRACE_MSG5(USBD, "INTERFACE[%02d] interfaces: %d endpoints: %d bFunctionClass: %d %s", 
                                endpoint_number, interface_driver->interfaces, 
                                interface_driver->endpointsRequested, interface_driver->bFunctionClass,
                                interface_driver->driver.name);

                interfaces = interface_driver->interfaces;

                TRACE_MSG6(USBD, "FUNCTION[%02d] interfaces: %d:%d endpoints: %d:%d %20s", 
				function_number, interface_driver->interfaces, interface_number, 
				interface_driver->endpointsRequested, endpoint_number, interface_name);

                interface_number += interface_driver->interfaces;
		endpoint_number += interface_driver->endpointsRequested;

                if ((USB_CLASS_MISC == composite_driver->device_description->bDeviceClass) /* && 
                                (interface_driver->interfaces > 1) */ ) 
                {
                        cfg_size += sizeof(struct usbd_interface_association_descriptor);
                        TRACE_MSG2(USBD, "FUNCTION[%02d] IAD size: %d", function_number, cfg_size);
                }

                /* iterate across interfaces for this function and their alternates to get descriptors sizes
                 */
                for (j = 0; j < interface_driver->interfaces; j++) {

                        struct usbd_interface_description *interface_description = interface_driver->interface_list + j;
			
			for (k = 0; k < interface_description->alternates; k++) {

                                struct usbd_alternate_description *alternate_description = 
                                        interface_description->alternate_list + k;

                                /* size interface descriptor
                                 */
                                cfg_size += sizeof(struct usbd_interface_descriptor);;

                                TRACE_MSG7(USBD, "SIZE ALT[%02d:%02d:%02d] classes: %d endpoints: %d %s size: %d",
                                                function_number, j, k, alternate_description->classes,
                                                alternate_description->endpoints, alternate_description->iInterface, cfg_size);

                                /* size class descriptors
                                 */
                                for (l = 0; l < alternate_description->classes; l++) {
                                        struct usbd_generic_class_descriptor *class_descriptor =
                                                *(alternate_description->class_list + l);
                                        cfg_size += class_descriptor->bLength;
                                }

                                cfg_size += alternate_description->endpoints * sizeof(struct usbd_endpoint_descriptor);

                                #if 0
                                /* size endpoint descriptors (in theory could be endpoints * sizeof())
                                 */
                                for (l = 0; l < alternate_description->endpoints; l++) {
                                        struct usbd_endpoint_descriptor *endpoint_descriptor =
                                                *(alternate_description->endpoint_list + l);
                                        cfg_size += endpoint_descriptor->bLength;
                                }
                                #endif
                        }
                }
        }

	TRACE_MSG2(USBD, "interfaces: %d endpoints: %d", interface_number, endpoint_number);

	/* Allocate and save information.
	 *
	 * Save endpoints, bNumInterfaces and configuration size, Allocate
	 * configuration descriptor, allocate enough memory for all
	 * interface instances plus one extra for class instance if
	 * required, allocate endpoint map array and endpoint requests.
	 */

	THROW_UNLESS((interfaces_array = (struct usbd_interface_instance *) 
				CKMALLOC (sizeof(struct usbd_interface_instance) *
                                        (function_number + (composite_driver->class_name ? 1 : 0)), GFP_KERNEL)), error);
        
	THROW_UNLESS((endpoint_map_array = (struct usbd_endpoint_map *) 
                                CKMALLOC (sizeof(struct usbd_endpoint_map) * endpoint_number, GFP_KERNEL)), error); 

        THROW_UNLESS((requestedEndpoints = (struct usbd_endpoint_request *) 
                                CKMALLOC (sizeof(struct usbd_endpoint_request) * endpoint_number, GFP_KERNEL)), error);

        THROW_UNLESS((interface_list = (struct usbd_interface_description *)
                                CKMALLOC (sizeof(struct usbd_interface_description) * interface_number, GFP_KERNEL)), error);


        #ifdef CONFIG_OTG_HIGH_SPEED
        for (i = 0; i < 2; i++) 
        #else /* CONFIG_OTG_HIGH_SPEED */
        for (i = 0; i < 1; i++) 
        #endif /* CONFIG_OTG_HIGH_SPEED */
        {
                THROW_UNLESS((composite_instance->configuration_descriptor[i] = 
                                        (struct usbd_configuration_descriptor *) CKMALLOC (cfg_size + 4, GFP_KERNEL)), error);
        }

        composite_instance->interfaces_array = interfaces_array;
	composite_instance->endpoint_map_array = endpoint_map_array;	
        composite_instance->requestedEndpoints = requestedEndpoints;
        composite_instance->interface_list = interface_list;

        composite_instance->configuration_size = cfg_size;			// total configuration size
        composite_instance->interface_functions = function_number;		// number of interface function drivers
        composite_instance->interfaces = interface_number;      		// number of interfaces
        composite_instance->endpointsRequested = endpoint_number;		// number of endpoints
        composite_instance->endpoints = endpoint_number;

	/* Setup interface_instance array.
	 *
	 * Find all interfaces and compose the interface list, count the
	 * endpoints and allocte the endpoint request array. Also find and
	 * total descriptor sizes.  Compose the endpoint request array and
	 * configuration descriptor, cannot request endpoints yet as the bus
	 * interface driver is not loaded, so we cannot compose
	 * configuration descriptor here.
         */
        for (interfaces_used = endpoint_number = interface_number = function_number = 0; 
			function_number < composite_instance->interface_functions; 
			function_number++) 
	{
		struct usbd_interface_instance *interface_instance = interfaces_array + function_number;
		char *interface_name = *(interface_function_names + function_number); 
		struct usbd_interface_driver *interface_driver;
		char *cp;
		int wIndex = 0;

		if ((cp = strchr(interface_name, '='))) {
			char *sp = interface_name;
			interface_name = cp + 1;
			for (; sp < cp; sp++) {
				BREAK_UNLESS(isdigit(*sp));
				wIndex = wIndex * 10 + *sp - '0';
			}
		}
		else {
			for (wIndex = 0; wIndex < 32; wIndex++) {
				BREAK_UNLESS(interfaces_used & (1 << wIndex));
			}
			THROW_IF(wIndex == 32, error);
		}
		
                THROW_UNLESS(interface_driver = usbp_find_interface_driver(interface_name), error);

                interface_instance->endpoint_map_array = composite_instance->endpoint_map_array + endpoint_number;
		interface_instance->function.function_driver = (struct usbd_function_driver *)interface_driver;
		interface_instance->function.function_type = function_interface;
		interface_instance->function.name = interface_driver->driver.name;
		interface_instance->function.bus = bus;
		interface_instance->wIndex = wIndex;
		interface_instance->lIndex = j;

		for (i = 0; i < interface_driver->interfaces; i++) {
			THROW_IF(interfaces_used & (1 << (wIndex + 1)), error);
			interfaces_used |= (1 << (wIndex + i));
		}

                TRACE_MSG4(USBD, "FUNCTION[%02d] interfaces: %d endpointsRequestged: %d wIndex: %d", function_number,
                                interface_driver->interfaces, interface_driver->endpointsRequested, wIndex);


		/* copy requested endpoint information
		 */
		for (k = 0; k < interface_driver->endpointsRequested; k++, endpoint_number++) {

			TRACE_MSG5(USBD, "REQUEST[%2d %d:%d %d:%d]", 
					function_number, j, interface_number, k, endpoint_number);

			TRACE_MSG3(USBD, "%x %x %d", requestedEndpoints + endpoint_number, 
					interface_driver->requestedEndpoints + j,
					sizeof(struct usbd_endpoint_request));

			TRACE_MSG5(USBD, "REQUEST[%2d:%2d:%2d] bm: %02x addr: %02x", function_number,
					k, endpoint_number, 
					interface_driver->requestedEndpoints[k].bmAttributes,
					interface_driver->requestedEndpoints[k].bEndpointAddress
				  );

			memcpy(requestedEndpoints + endpoint_number, interface_driver->requestedEndpoints + k,
					sizeof(struct usbd_endpoint_request));

			requestedEndpoints[endpoint_number].interface_num += wIndex;
		}

		interface_number += interface_driver->interfaces;
	}

	if (composite_driver->class_name) {

                struct usbd_class_instance *class_instance = (struct usbd_class_instance *)(interfaces_array + function_number);
		struct usbd_class_driver *class_driver;

                THROW_UNLESS(class_driver = usbp_find_class_driver(composite_driver->class_name), error);

		class_instance->function.function_driver = (struct usbd_function_driver *)composite_driver;
		class_instance->function.function_type = function_composite;
		class_instance->function.name = composite_driver->driver.name;
		class_instance->function.bus = bus;
	}

	return composite_instance;

        CATCH(error) {
		printk(KERN_INFO"%s: FAILED\n", __FUNCTION__);
                TRACE_MSG0(USBD, "FAILED");
                if (composite_instance) {
                        usbd_strings_exit(bus);
                        for (i = 0; i < 2; i++) if (composite_instance->configuration_descriptor[i]) 
                                LKFREE(composite_instance->configuration_descriptor[i]);
                }

                if (requestedEndpoints) LKFREE(requestedEndpoints);
                if (interfaces_array) LKFREE(interfaces_array);
                if (interface_list) LKFREE(interface_list);
                return NULL;
        }
}


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Function Registration:
 *	usbd_register_function_function()
 *	usbd_register_interface_function()
 *	usbd_register_composite_function()
 *	usbd_deregister_function()
 *	usbd_interface_deregister_function()
 *	usbd_class_deregister_function()
 *	usbd_composite_deregister_function()
 *	usbd_find_function()
 *	usbd_find_interface_function()
 *
 */

/*!
 * @brief usbd_find_function_internal() - register a usbd function driver
 *
 * USBD Function drivers call this to register with the USBD Core layer.
 * Return NULL on failure.
 *
 * @param usbd_function_drivers
 * @param name
 * @return function instance
 *
 */
struct usbd_function_driver *
usbd_find_function_internal(LIST_NODE *usbd_function_drivers, char *name)
{
        int len = name ? strlen(name) : 0;
        LIST_NODE *lhd = NULL;
        TRACE_MSG2(USBD, "len: %d \"%s\"", len, name ? name : "");
        LIST_FOR_EACH (lhd, usbd_function_drivers) {

		struct usbd_function_driver *function_driver = LIST_ENTRY (lhd, struct usbd_function_driver, drivers);

                CONTINUE_IF(len && strncmp(function_driver->name, name, len));

                TRACE_MSG1(USBD, "FOUND: %s", function_driver->name);

                return function_driver;
        }
        return NULL;
}


/*! 
 * usbd_function_name() - return name of nth function driver
 * @param n index to function name
 * @return pointer to name
 */
char * usbd_function_name(int n)
{
        struct usbd_function_driver *function_driver = NULL;
        LIST_NODE *lhd = NULL;

        TRACE_MSG1(USBD, "n: %d", n);
        LIST_FOR_EACH (lhd, &usbd_simple_drivers) {
                function_driver = LIST_ENTRY (lhd, struct usbd_function_driver, drivers);
                TRACE_MSG2(USBD, "simple name: [%d] %s", n, function_driver->name);
                CONTINUE_IF(n--);
                TRACE_MSG1(USBD, "simple: %s", function_driver->name);
                return (char *)function_driver->name;
        };

        LIST_FOR_EACH (lhd, &usbd_composite_drivers) {
                function_driver = LIST_ENTRY (lhd, struct usbd_function_driver, drivers);
                TRACE_MSG2(USBD, "composite name: [%d] %s", n, function_driver->name);
                CONTINUE_IF(n--);
                TRACE_MSG1(USBD, "composite: %s", function_driver->name);
                return (char *)function_driver->name;
        };

        return NULL;
}

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * String Descriptors:
 *	usbd_alloc_string_descriptor()
 *	usbd_free_string_descriptor()
 *	usbd_get_string_descriptor()
 */

#define LANGID_ENGLISH          "\011"
#define LANGID_US_ENGLISH       "\004"
#define LANGIDs  LANGID_US_ENGLISH LANGID_ENGLISH

/*!
 * usbd_alloc_string_zero() - allocate a string descriptor and return index number
 *
 * Find an empty slot in index string array, create a corresponding descriptor
 * and return the slot number.
 *
 * @param str string to allocate
 * @return the slot number
 */
static u8 usbd_alloc_string_zero (struct usbd_bus_instance *bus, char *str)
{
    u8 bLength;
    u16 *wData;
    struct usbd_langid_descriptor *langid = NULL;

    RETURN_ZERO_IF(bus->usb_strings[0] != NULL);

    TRACE_MSG2(USBD, "LANGID: %02x %02x", str[0], str[1]);

    bLength = 4;

    RETURN_ZERO_IF(!(langid = (struct usbd_langid_descriptor *) CKMALLOC (bLength, GFP_KERNEL)));

    langid->bLength = bLength;                              // set descriptor length
    langid->bDescriptorType = USB_DT_STRING;                // set descriptor type
    langid->bData[0] = str[1];
    langid->bData[1] = str[0];

    bus->usb_strings[0] = (struct usbd_string_descriptor *)langid;    // save in string index array
    return 0;
}

/*!
 * usbd_strings_init() - initialize usb strings pool
 */
int usbd_strings_init(struct usbd_bus_instance *bus, struct usbd_function_instance *function_instance)
{
        bus->usbd_maxstrings = 50; // XXX Need configuration parameter...

        RETURN_EINVAL_IF (!(bus->usb_strings = CKMALLOC (
                                        sizeof (struct usbd_string_descriptor *) * bus->usbd_maxstrings, 
                                        GFP_KERNEL)));

        #if defined(CONFIG_OTG_LANGID)
        {
                char langid_str[4];
                u16 langid;
                TRACE_MSG1(USBD, "LANGID: %04x", CONFIG_OTG_LANGID);
                langid = cpu_to_le16(CONFIG_OTG_LANGID);
                memset(langid_str, 0, sizeof(langid_str));
                langid_str[0] = langid & 0xff;
                langid_str[1] = (langid >> 8) & 0xff;
                if (usbd_alloc_string_zero(bus, langid_str)) {
                        LKFREE (function_instance->bus->usb_strings);
                        return -EINVAL;
                }
        }
        #else /* defined(CONFIG_OTG_LANGID) */

        if (usbd_alloc_string_zero(ubs, LANGIDs) != 0) {
                LKFREE (function_instance->bus->usb_strings);
                return -EINVAL;
        }
        #endif /* defined(CONFIG_OTG_LANGID) */
        return 0;
}

/*!
 * usbd_strings_exit() - de-initialize usb strings pool
 */
void usbd_strings_exit(struct usbd_bus_instance *bus)
{
        int i;
        RETURN_IF (!bus->usb_strings);
        for (i = 0; i < bus->usbd_maxstrings; i++)
                usbd_free_string_descriptor(bus, i);
        LKFREE (bus->usb_strings);
        bus->usb_strings = NULL;
}


/*! 
 * usbd_get_string_descriptor() - find and return a string descriptor
 *
 * Find an indexed string and return a pointer to a it.
 * @param function_instance
 * @param index index of string
 * @return pointer to string descriptor or NULL
 */
struct usbd_string_descriptor *usbd_get_string_descriptor (struct usbd_function_instance *function_instance, u8 index)
{
        struct usbd_string_descriptor *str_desc = NULL;
        struct usbd_composite_instance *composite_instance = NULL;
        struct usbd_composite_driver *composite_driver = NULL;
        int i;
        
        TRACE_MSG3(USBD, "usbd_get_string_descriptor for %0x, func=%s, driver=%s", index, function_instance->name, function_instance->function_driver->name);
        TRACE_MSG3(USBD, "usbd_get_string_descriptor function type=0x%x, simple=0x%x, composite=0x%x", function_instance->function_type, function_simple, function_composite);

        /* see if interface handle the string descriptor */
        if (function_composite == function_instance->function_type)
        {
                TRACE_MSG0(USBD, "usbd_get_string_desciptor function composite");

                composite_instance = (struct usbd_composite_instance *)function_instance;
                composite_driver = (struct usbd_composite_driver *)composite_instance->function.function_driver;

                for (i = 0; i < composite_instance->interface_functions; i++)
                {
                        struct usbd_interface_instance *interfaces_array = composite_instance->interfaces_array;
                        struct usbd_interface_instance *interface_instance = interfaces_array + i;
                        struct usbd_interface_driver *interface_driver;
                        CONTINUE_UNLESS(interface_instance);
                        TRACE_MSG1(USBD, "usbd_get_string_desciptor interface[%d]", i);
                        interface_driver = (struct usbd_interface_driver *)interface_instance->function.function_driver;
                        interface_instance->function.bus = function_instance->bus;
                        if (interface_driver->driver.fops->get_descriptor)
                        {
                            TRACE_MSG1(USBD, "Calling interface get_descriptor for 0x%x", index);
                            str_desc = interface_driver->driver.fops->get_descriptor
                                ((struct usbd_function_instance *)interface_instance, USB_DT_STRING, index);
                            
                            if (str_desc)
                            {
                                return str_desc;
                            }
                        }
                }
    }

        RETURN_NULL_IF (index >= function_instance->bus->usbd_maxstrings);
        return function_instance->bus->usb_strings[index];
}

/*! 
 * usbd_realloc_string() - allocate a string descriptor and return index number
 *
 * Find an empty slot in index string array, create a corresponding descriptor
 * and return the slot number.
 * @param function_instance
 * @param index
 * @param str
 * @return new index
 */
u8 usbd_realloc_string (struct usbd_function_instance *function_instance, u8 index, char *str)
{
        char *save = str;
        struct usbd_string_descriptor *string;
        u8 bLength;
        u16 *wData;

        RETURN_EINVAL_IF(!str || !strlen (str));

        if ((index < function_instance->bus->usbd_maxstrings) && (string = function_instance->bus->usb_strings[index])) {
                function_instance->bus->usb_strings[index] = NULL;
                LKFREE (string);
        }

        bLength = USBD_STRING_SIZEOF (struct usbd_string_descriptor) + 2 * strlen (str);

        RETURN_ENOMEM_IF(!(string = (struct usbd_string_descriptor *)CKMALLOC (bLength, GFP_ATOMIC)));

        string->bLength = bLength;
        string->bDescriptorType = USB_DT_STRING;

        for (wData = string->wData; *str;) 
                *wData++ = cpu_to_le16 ((u16) (*str++));

        // store in string index array
        function_instance->bus->usb_strings[index] = string;

        TRACE_MSG2(USBD, "STRING[%d] %s", index, save);
        return index;
}

/*!
 * @brief  strtest()
 *
 * @param string
 * @param str
 * @return non-zero if match
 */
int strtest(struct usbd_string_descriptor *string, char *str)
{
        int i;
        for (i = 0; i < string->bLength; i++ ) {
                CONTINUE_IF ( (*str == (string->wData[i] & 0xff)));
                return 1;
        }
        return 0;
}




/*! 
 * usbd_alloc_string() - allocate a string descriptor and return index number
 *
 * Find an empty slot in index string array, create a corresponding descriptor
 * and return the slot number.
 * @param function_instance
 * @param str
 * @return index
 *
 * XXX rename to usbd_alloc_string_descriptor
 */
u8 usbd_alloc_string (struct usbd_function_instance *function_instance, char *str)
{
        char *save = str;
        int i, j;
        struct usbd_string_descriptor *string = NULL;
        u8 bLength;
        u16 *wData;

        TRACE_MSG2(USBD, "function_instance: %x str: %x", function_instance, str);

        RETURN_ZERO_IF(!str || !strlen (str));

        // find an empty string descriptor slot

        // find an empty string descriptor slot
        // "0" is consumed by the language ID, "1" is the optional IP address
        // and "2" is for the product string (as required by the PST group)
        for (i = 2; i < function_instance->bus->usbd_maxstrings; i++) {

                if (function_instance->bus->usb_strings[i]) {
                        UNLESS (strtest(function_instance->bus->usb_strings[i], str))
                                return i;
                        continue;
                }

                bLength = USBD_STRING_SIZEOF (struct usbd_string_descriptor) + 2 * strlen (str);

                RETURN_ZERO_IF(!(string = (struct usbd_string_descriptor *)CKMALLOC (bLength, GFP_KERNEL)));

                string->bLength = bLength;
                string->bDescriptorType = USB_DT_STRING;

                for (wData = string->wData; *str;) 
                        *wData++ = cpu_to_le16((u16) (*str++));

                // store in string index array
                function_instance->bus->usb_strings[i] = string;
                TRACE_MSG2(USBD, "STRING[%d] %s", i, save);
                return i;
        }
        return 0;
}


/*! 
 * usbd_free_string_descriptor() - deallocate a string descriptor
 *
 * Find and remove an allocated string.
 * @param bus
 * @param index
 *
 */
void usbd_free_string_descriptor (struct usbd_bus_instance *bus, u8 index)
{
        struct usbd_string_descriptor *string;

        if ((index < bus->usbd_maxstrings) && (string = bus->usb_strings[index])) {
                bus->usb_strings[index] = NULL;
                LKFREE (string);
        }
}

OTG_EXPORT_SYMBOL(usbd_realloc_string);
OTG_EXPORT_SYMBOL(usbd_alloc_string);
OTG_EXPORT_SYMBOL(usbd_free_string_descriptor);
OTG_EXPORT_SYMBOL(usbd_get_string_descriptor);
//OTG_EXPORT_SYMBOL(usbd_maxstrings);


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Device Information:
 *	usbd_high_speed()
 *	usbd_get_bMaxPower()
 *	usbd_endpoint_wMaxPacketsize()
 *	usbd_endpoint_wMaxPacketsize_ep0()
 *	usbd_endpoint_bEndpointAddress()
 *	
 */


/*!
 * usbd_high_speed() - return high speed status
 * @return true if operating at high speed
 */
BOOL usbd_high_speed(struct usbd_function_instance *function)
{
        // XXX TODO - per function modifications for composite devices
        return BOOLEAN(function->bus->high_speed);
}

/*!
 * usbd_get_bMaxPower() - process a received urb
 *
 * Used by a USB Bus interface driver to pass received device request to
 * the appropriate USB Function driver.
 *
 * @param function
 * @return non-zero if errror
 */
int usbd_get_bMaxPower(struct usbd_function_instance *function)
{
        struct usbd_bus_instance *bus = function->bus;
        return bus->driver->bMaxPower;
}

/*! usbd_endpoint_map()
 *
 * @param function
 * @return pointer to endpoint map
 */
struct usbd_endpoint_map *usbd_endpoint_map(struct usbd_function_instance *function)
{
        struct usbd_endpoint_map *endpoint_map = NULL;
        RETURN_NULL_UNLESS(function);
        switch(function->function_type) {
        case function_simple:
                RETURN_NULL_UNLESS((endpoint_map = ((struct usbd_simple_instance *)function)->endpoint_map_array));
                return endpoint_map;
        case function_composite:
                RETURN_NULL_UNLESS((endpoint_map = ((struct usbd_composite_instance *)function)->endpoint_map_array));
                return endpoint_map;
        case function_interface:
                RETURN_NULL_UNLESS((endpoint_map = ((struct usbd_interface_instance *)function)->endpoint_map_array));
                return endpoint_map;
        case function_ep0:
                RETURN_NULL_UNLESS((endpoint_map = ((struct usbd_interface_instance *)function)->endpoint_map_array));
                return endpoint_map;
        default:
                return NULL;
        }
}

/*!
 * usbd_endpoint_wMaxPacketSize() - get maximum packet size for endpoint
 * @param function
 * @param endpoint_index 
 * @param hs highspeed flag
 * @return endpoint size
 */
int usbd_endpoint_wMaxPacketSize(struct usbd_function_instance *function, int endpoint_index, int hs)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        TRACE_MSG4(USBD, "function: %x index: %d endpoint_map: %x wMaxPacketSize: %02x", 
                        function, endpoint_index, endpoint_map, endpoint_map[endpoint_index].wMaxPacketSize[hs]);
        return le16_to_cpu(endpoint_map[endpoint_index].wMaxPacketSize[hs]);
}

/*!
 * usbd_endpoint_zero_wMaxPacketSize() - get maximum packet size for endpoint zero
 * @param function
 * @param hs highspeed flag
 * @return endpoint size
 */
int usbd_endpoint_zero_wMaxPacketSize(struct usbd_function_instance *function, int hs)
{
        struct usbd_endpoint_map *endpoint_map;
        RETURN_ZERO_IF(!(endpoint_map = function->bus->ep0->endpoint_map_array));
        return le16_to_cpu(endpoint_map[0].wMaxPacketSize[hs]);
}

/*!
 * usbd_endpoint_bEndpointAddress() - get endpoint addrsess
 * @param function
 * @param endpoint_index
 * @param hs high speed flag
 * @return endpoint address
 */
int usbd_endpoint_bEndpointAddress(struct usbd_function_instance *function, int endpoint_index, int hs)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        TRACE_MSG3(USBD, "function: %x index: %d endpoint_map: %x", function, endpoint_index, endpoint_map);
        return endpoint_map[endpoint_index].bEndpointAddress[hs];
}

OTG_EXPORT_SYMBOL(usbd_endpoint_wMaxPacketSize);
OTG_EXPORT_SYMBOL(usbd_endpoint_zero_wMaxPacketSize);
OTG_EXPORT_SYMBOL(usbd_endpoint_bEndpointAddress);
OTG_EXPORT_SYMBOL(usbd_high_speed);

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Device Control:
 *
 *	XXX usbd_request_endpoints()
 *	XXX usbd_configure_endpoints()
 *
 *      usbd_get_device_state()
 *      usbd_get_device_status()
 *      usbd_framenum()
 *      usbd_ticks()
 *      usbd_elapsed()
 */

/*!
 * usbd_flush_endpoint_irq() - flush urbs from endpoint
 *
 * Iterate across the approrpiate tx or rcv list and cancel any outstanding urbs.
 *
 * @param endpoint flush urbs on this endpoint
 */
void usbd_flush_endpoint_irq (struct usbd_endpoint_instance *endpoint)
{
        struct usbd_urb *urb;

        while ((urb = endpoint->tx_urb))
                usbd_cancel_urb(urb);

        for (; (urb = usbd_first_urb_detached_irq (&endpoint->tx)); usbd_cancel_urb(urb));

        while ((urb = endpoint->rcv_urb))
                usbd_cancel_urb(urb);

        for (; (urb = usbd_first_urb_detached_irq (&endpoint->rdy)); usbd_cancel_urb(urb));
}

/*!
 * usbd_flush_endpoint() - flush urbs from endpoint
 *
 * Iterate across the approrpiate tx or rcv list and cancel any outstanding urbs.
 *
 * @param endpoint flush urbs on this endpoint
 */
void usbd_flush_endpoint (struct usbd_endpoint_instance *endpoint)
{
        unsigned long flags;
        local_irq_save (flags);
        usbd_flush_endpoint_irq(endpoint);
        local_irq_restore (flags);
}

/*!
 * usbd_flush_endpoint_address() - flush endpoint
 * @param function
 * @param endpoint_index
 */
void usbd_flush_endpoint_index (struct usbd_function_instance *function, int endpoint_index)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        struct usbd_endpoint_instance *endpoint;

        RETURN_UNLESS(function && endpoint_map);
        RETURN_IF(!(endpoint = endpoint_map[endpoint_index].endpoint));
        usbd_flush_endpoint (endpoint);
}

/*!
 * usbd_get_device_state() - return device state
 * @param function
 * @return current device state
 */
usbd_device_state_t usbd_get_device_state(struct usbd_function_instance *function)
{
        return (function && function->bus) ? function->bus->device_state : STATE_UNKNOWN;
}

/*!
 * usbd_get_device_status() - return device status
 * @param function
 * @return current device status
 */
usbd_device_status_t usbd_get_device_status(struct usbd_function_instance *function)
{
        return (function && function->bus) ?  function->bus->status : USBD_UNKNOWN;
}

#if 0
/*!
 * usbd_framenum() - return framenum
 * @param function
 * @return current framenum
 */
int usbd_framenum(struct usbd_function_instance * function)
{
        RETURN_ZERO_UNLESS(function);
        RETURN_ZERO_UNLESS(function->bus->driver->bops->framenum);
        return function->bus->driver->bops->framenum ();
}

/*!
 * usbd_ticks() - return ticks
 * @param function
 * @return current ticks
 */
u64 usbd_ticks(struct usbd_function_instance *function)
{
        RETURN_ZERO_UNLESS(function);
        RETURN_ZERO_UNLESS(function->bus->driver->bops->ticks);
        return function->bus->driver->bops->ticks ();
}

/*!
 * usbd_elapsed() - return elapsed
 * @param function
 * @param t1
 * @param t2
 * @return elapsed uSecs between t1 and t2
 */
u64 usbd_elapsed(struct usbd_function_instance *function, u64 *t1, u64 *t2)
{
        RETURN_ZERO_UNLESS(function);
        RETURN_ZERO_UNLESS(function->bus->driver->bops->elapsed);
        return function->bus->driver->bops->elapsed (t1, t2);
}
#endif

//OTG_EXPORT_SYMBOL(usbd_set_endpoint_halt);
//OTG_EXPORT_SYMBOL(usbd_endpoint_halted);
OTG_EXPORT_SYMBOL(usbd_get_device_state);
OTG_EXPORT_SYMBOL(usbd_get_device_status);
#if 0
OTG_EXPORT_SYMBOL(usbd_framenum);
OTG_EXPORT_SYMBOL(usbd_ticks);
OTG_EXPORT_SYMBOL(usbd_elapsed);
#endif
OTG_EXPORT_SYMBOL(usbd_flush_endpoint);
OTG_EXPORT_SYMBOL(usbd_flush_endpoint_index);

/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*!
 * Endpoint I/O:
 *	usbd_alloc_urb()
 *	usbd_start_in_urb()
 *	usbd_start_out_urb()
 *	usbd_cancel_urb()
 *
 */

/*!
 * usbd_alloc_urb() - allocate an URB appropriate for specified endpoint
 *
 * Allocate an urb structure. The usb device urb structure is used to
 * contain all data associated with a transfer, including a setup packet for
 * control transfers.
 * 
 * @param function
 * @param endpoint_index
 * @param length
 * @param callback
 * @return urb
 */
struct usbd_urb *usbd_alloc_urb (struct usbd_function_instance *function, int endpoint_index,
                int length, int (*callback) (struct usbd_urb *, int))
{
        struct usbd_urb *urb = NULL;
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        struct usbd_endpoint_instance *endpoint;
        unsigned long flags;
        int hs = function->bus->high_speed;

        RETURN_NULL_IF(!(endpoint = endpoint_map[endpoint_index].endpoint));
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG4(USBD, "function: %x index: %d endpoint_map: %x address: %02x", 
                        function, endpoint_index, endpoint_map, endpoint->new_bEndpointAddress[hs]);
        #endif
        local_irq_save(flags);
        THROW_IF (!(urb = (struct usbd_urb *)CKMALLOC (sizeof (struct usbd_urb), GFP_ATOMIC)), error); 
        urb->endpoint = endpoint;
        urb->bus = function->bus;
        urb->function_instance = function;
        urb->link.prev = urb->link.next = &urb->link;
        urb->callback = callback;
        urb->alloc_length = urb->actual_length = urb->buffer_length = 0;

        if (length) {

                urb->buffer_length = length;

                /* For receive we always overallocate to ensure that receiving another
                 * full sized packet when we are only expecting a short packet will
                 * not overflow the buffer. Endpoint zero alloc's don't specify direction
                 * so always overallocate.
                 */
                UNLESS (endpoint->new_bEndpointAddress[hs] && (endpoint->new_bEndpointAddress[hs] & USB_ENDPOINT_DIR_MASK))
                        length = ((length / endpoint->new_wMaxPacketSize[hs]) + 3) * endpoint->new_wMaxPacketSize[hs];

		THROW_IF(!(urb->buffer = (u8 *)CKMALLOC (length, GFP_ATOMIC)), error);
                urb->alloc_length = length;
        }


        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(USBD, "buffer_length: %d alloc_length: %d", urb->buffer_length, urb->alloc_length);
        #endif

        CATCH(error) {
                TRACE_MSG0(USBD,"ERROR Allocating URB");
                usbd_free_urb(urb);
                urb = NULL;
        }
        local_irq_restore(flags);
        return urb;
}

/*!
 * usbd_halt_endpoint() - 
 *
 * @param function function that owns endpoint
 * @param endpoint_index endpoint number
 * @return non-zero if endpoint is halted.
 */
int usbd_halt_endpoint (struct usbd_function_instance *function, int endpoint_index)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        struct usbd_endpoint_instance *endpoint;
        int hs = function->bus->high_speed;

        RETURN_ZERO_UNLESS(function && endpoint_map);
        RETURN_ZERO_IF(!(endpoint = endpoint_map[endpoint_index].endpoint));

        return function->bus->driver->bops->halt_endpoint (function->bus, endpoint->new_bEndpointAddress[hs], 1);
}


/*!
 * usbd_alloc_urb_ep0() - allocate an urb for endpoint zero
 * @param function
 * @param length
 * @param callback
 * @return urb
 */
struct usbd_urb *usbd_alloc_urb_ep0 (struct usbd_function_instance *function, int length, 
                int (*callback) (struct usbd_urb *, int))
{
        return usbd_alloc_urb((struct usbd_function_instance *)function->bus->ep0, 0, length, callback);
}

/*!
 * usbd_free_urb() - deallocate an URB and associated buffer
 *
 * Deallocate an urb structure and associated data.
 * @param urb
 */
void usbd_free_urb (struct usbd_urb *urb)
{
        RETURN_IF (!urb);
        if (urb->buffer) LKFREE ((void *) urb->buffer);
        LKFREE (urb);
}

int usbd_start_in_urb (struct usbd_urb *urb)
{
        int hs = urb->bus->high_speed;
        if (urb->endpoint->feature_setting & FEATURE(USB_ENDPOINT_HALT)) {
                TRACE_MSG0(USBD, "1. EAGAIN");
                urb->status = USBD_URB_STALLED;
                return -EAGAIN;
        }
        if (urb->endpoint->new_bEndpointAddress[hs] && (USBD_OK != urb->bus->status)) {
                TRACE_MSG0(USBD, "2. EAGAIN");
                urb->status = USBD_URB_NOT_READY;
                return -EAGAIN;
        }
        urb->status = USBD_URB_IN_QUEUE;
        urb_append (&(urb->endpoint->tx), urb);
        return urb->bus->driver->bops->start_endpoint_in(urb->bus, urb->endpoint);
}

/*!
 * usbd_start_out_urb() - recycle a received urb
 *
 * Used by a USB Function interface driver to recycle an urb.
 *
 * @param urb to process
 * @return non-zero if error
 */
int usbd_start_out_urb (struct usbd_urb *urb)
{
        int hs = urb->bus->high_speed;
        //TRACE_MSG2(USBD, "urb: %x bus: %x OK", urb, urb->bus);
        if (urb->endpoint->feature_setting & FEATURE(USB_ENDPOINT_HALT)) {
                urb->status = USBD_URB_STALLED;
		TRACE_MSG0(USBD, "USBD_URB_STALLED");
                return -EAGAIN;
        }
        if (urb->endpoint->new_bEndpointAddress[hs] && (USBD_OK != urb->bus->status)) {
                urb->status = USBD_URB_NOT_READY;
		//TRACE_MSG0(USBD, "USBD_URB_NOT_READY");
                return -EAGAIN;
        }

        urb->actual_length = 0;
        urb->status = USBD_URB_IN_QUEUE;
        urb_append (&(urb->endpoint->rdy), urb);
        return urb->bus->driver->bops->start_endpoint_out(urb->bus, urb->endpoint);
}

/*!
 * usbd_cancel_urb() - cancel an urb being sent
 *
 * @param urb to process
 * @return non-zero if error
 */
int usbd_cancel_urb (struct usbd_urb *urb)
{
        return urb->bus->driver->bops->cancel_urb_irq (urb);
}


/*!
 * usbd_endpoint_halted() - return halt status
 *
 * @param function function that owns endpoint
 * @param endpoint_index endpoint number
 * @return non-zero if endpoint is halted.
 */
int usbd_endpoint_halted (struct usbd_function_instance *function, int endpoint_index)
{
        struct usbd_endpoint_instance *endpoint;
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        RETURN_ZERO_UNLESS(function && endpoint_map);
        RETURN_ZERO_UNLESS((endpoint = endpoint_map[endpoint_index].endpoint));
        return function->bus->driver->bops->endpoint_halted (function->bus, 
                        endpoint->new_bEndpointAddress[function->bus->high_speed]);

}


OTG_EXPORT_SYMBOL(usbd_alloc_urb);
OTG_EXPORT_SYMBOL(usbd_alloc_urb_ep0);
OTG_EXPORT_SYMBOL(usbd_free_urb);

OTG_EXPORT_SYMBOL(usbd_start_in_urb);
OTG_EXPORT_SYMBOL(usbd_start_out_urb);
OTG_EXPORT_SYMBOL(usbd_cancel_urb);
OTG_EXPORT_SYMBOL(usbd_halt_endpoint);

/* ********************************************************************************************* */

/*!
 * usbd_function_get_privdata() - get private data pointer
 * @param function
 * @return void * pointer to private data
 */
void *usbd_function_get_privdata(struct usbd_function_instance *function)
{
        return(function->privdata);
}

/*!
 * usbd_function_set_privdata() - set private data structure in function
 * @param function
 * @param privdata
 */
void usbd_function_set_privdata(struct usbd_function_instance *function, void *privdata)
{
        function->privdata = privdata;
}

/*!
 * usbd_endpoint_transferSize() - get transferSize for endpoint
 * @param function
 * @param endpoint_index
 * @param hs highspeed flag
 * @return transfer size
 */
int usbd_endpoint_transferSize(struct usbd_function_instance *function, int endpoint_index, int hs)
{
        struct usbd_endpoint_map *endpoint_map = usbd_endpoint_map(function);
        RETURN_ZERO_UNLESS(function && endpoint_map);
        return endpoint_map[endpoint_index].transferSize[hs];
}

/*!
 * usbd_endpoint_update() - update endpoint address and size
 * @param function
 * @param endpoint_index
 * @param endpoint descriptor
 * @param hs high speed flag
 */
void usbd_endpoint_update(struct usbd_function_instance *function, int endpoint_index, 
                struct usbd_endpoint_descriptor *endpoint, int hs)
{
        endpoint->bEndpointAddress = usbd_endpoint_bEndpointAddress(function, endpoint_index, hs);
        endpoint->wMaxPacketSize = usbd_endpoint_wMaxPacketSize(function, endpoint_index, hs);
}

/*!
 * usbd_otg_bmattributes() - return attributes
 * @param function
 * @return endpoint attributes
 */
int usbd_otg_bmattributes(struct usbd_function_instance *function)
{
        // XXX TODO - per function modifications for composite devices
        return function->bus->otg_bmAttributes;
}

OTG_EXPORT_SYMBOL(usbd_function_get_privdata);
OTG_EXPORT_SYMBOL(usbd_function_set_privdata);
OTG_EXPORT_SYMBOL(usbd_endpoint_transferSize);
OTG_EXPORT_SYMBOL(usbd_otg_bmattributes);
OTG_EXPORT_SYMBOL(usbd_endpoint_update);

