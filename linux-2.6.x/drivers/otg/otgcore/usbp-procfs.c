/*
 * otg/otgcore/usbp-procfs.c - USB Device Core Layer
 * @(#) balden@seth2.belcarratech.com|otg/otgcore/usbp-procfs.c|20051116205002|62654
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
 * @file otg/otgcore/usbp-procfs.c
 * @brief Implements /proc/usbd-functions, which displays descriptors for the current selected function.
 *
 *
 * @ingroup USBP
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#ifdef LINUX24
EXPORT_NO_SYMBOLS;
#endif

#include <otg/usbp-chap9.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-trace.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>
#include <otg/otg-pcd.h>
#include <public/otg-node.h>

//EMBED_USBD_INFO ("usbdprocfs 2.0-beta");

#define MAX_INTERFACES 2

extern struct usbd_bus_instance *usbd_bus_instance;
extern LIST_NODE usbd_simple_drivers;            // list of all registered configuration function modules
extern LIST_NODE usbd_interface_drivers;         // list of all registered interface function modules
extern LIST_NODE usbd_class_drivers;             // list of all registered composite function modules
extern LIST_NODE usbd_composite_drivers;         // list of all registered composite function modules

/*! 
 * list_function() - 
 */
int list_function(LIST_NODE *usbd_function_drivers, char *cp, char *msg)
{
        int total = sprintf(cp, "\n%s\n", msg);

        LIST_NODE *lhd = NULL;
        LIST_FOR_EACH (lhd, usbd_function_drivers) {
		struct usbd_function_driver *function_driver = LIST_ENTRY (lhd, struct usbd_function_driver, drivers);
                total += sprintf(cp + total, "\t%s\n", function_driver->name);
        }
        return total;
}


/* Proc Filesystem *************************************************************************** */
/* *
 * dohex
 *
 */
static void dohexdigit (char *cp, unsigned char val)
{
        if (val < 0xa) {
                *cp = val + '0';
        } else if ((val >= 0x0a) && (val <= 0x0f)) {
                *cp = val - 0x0a + 'a';
        }
}

/* *
 * dohex
 *
 */
static void dohexval (char *cp, unsigned char val)
{
        dohexdigit (cp++, val >> 4);
        dohexdigit (cp++, val & 0xf);
}

/* *
 * dump_descriptor
 */
static int dump_descriptor (char *buf, char *sp)
{
        int num;
        int len = 0;

        RETURN_ZERO_UNLESS(sp);

        num = *sp;

        while (sp && num--) {
                dohexval (buf, *sp++);
                buf += 2;
                *buf++ = ' ';
                len += 3;
        }
        len++;
        *buf = '\n';
        return len;
}

/* *
 * dump_string_descriptor
 */
static int dump_string_descriptor (char *buf, int i, char *name)
{
        int num;
        int k;
        int len = 0;

        struct usbd_string_descriptor *string_descriptor;


        RETURN_ZERO_UNLESS(i);

        string_descriptor = usbd_get_string_descriptor (usbd_bus_instance->function_instance, i);

        RETURN_ZERO_UNLESS (string_descriptor);

        len += sprintf((char *)buf+len, "  %-24s [%2d:%2d  ] ", name, i, string_descriptor->bLength);

        for (k = 0; k < (string_descriptor->bLength / 2) - 1; k++) {
                *(char *) (buf + len) = (char) string_descriptor->wData[k];
                len++;
        }
        len += sprintf ((char *) buf + len, "\n");

        return len;
}



/* dump_descriptors
 */
static int dump_device_descriptor(char *buf, char *sp)
{
        int total = 0;
        struct usbd_device_descriptor *device_descriptor = (struct usbd_device_descriptor *) sp;

        TRACE_MSG2(USBD, "buf: %x sp: %x", buf, sp);

        total = sprintf(buf + total,  "Device descriptor          [       ] ");
        total += dump_descriptor(buf + total, sp);

        total += sprintf(buf + total, "  bcdUSB                   [      2] %04x\n", device_descriptor->bcdUSB);
        total += sprintf(buf + total, "  bDevice[Class,Sub,Pro]   [      4] %02x %02x %02x\n", 
                        device_descriptor->bDeviceClass, device_descriptor->bDeviceSubClass, device_descriptor->bDeviceProtocol);
                       
        total += sprintf(buf + total, "  bMaxPacketSize0          [      7] %02x\n", device_descriptor->bMaxPacketSize0);

        total += sprintf(buf + total, "  idVendor                 [      8] %04x\n", device_descriptor->idVendor);
        total += sprintf(buf + total, "  idProduct                [     10] %04x\n", device_descriptor->idProduct);
        total += sprintf(buf + total, "  bcdDevice                [     12] %04x\n", device_descriptor->bcdDevice);

        total += sprintf(buf + total, "  bNumConfigurations       [     17] %02x\n", device_descriptor->bNumConfigurations);

        total += dump_string_descriptor(buf + total, device_descriptor->iManufacturer, "iManufacturer");
        total += dump_string_descriptor(buf + total, device_descriptor->iProduct, "iProduct");
        total += dump_string_descriptor(buf + total, device_descriptor->iSerialNumber, "iSerialNumber");

        total += sprintf(buf + total, "\n");
        return total;
}

/* dump_descriptors
 */
static int dump_config_descriptor(char *buf, char *sp)
{
        struct usbd_configuration_descriptor *config = (struct usbd_configuration_descriptor *) sp;
        struct usbd_endpoint_descriptor *endpoint;
        struct usbd_interface_descriptor *interface;
        struct usbd_interface_association_descriptor *iad;

        int wTotalLength = le16_to_cpu(config->wTotalLength);
        int bConfigurationValue = config->bConfigurationValue;
        int interface_num;
        int class_num;
        int endpoint_num;
        int total;

        TRACE_MSG3(USBD, "buf: %x sp: %x length: %d", buf, sp, wTotalLength);

        interface_num = class_num = endpoint_num = 0;

        for (total = 0; wTotalLength > 0; ) {
                BREAK_UNLESS(sp[0]);
                switch (sp[1]) {
                case USB_DT_CONFIGURATION:
                        interface_num = class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "Configuration descriptor   [%d      ] ", bConfigurationValue);
                        break;
                case USB_DT_DEVICE_QUALIFIER:
                        interface_num = class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "\nDevice qualifier           [%d      ] ", bConfigurationValue);
                        break;
                case USB_DT_OTHER_SPEED_CONFIGURATION:
                        interface_num = class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "Other Speed descriptor     [%d      ] ", bConfigurationValue);
                        break;
                case USB_DT_INTERFACE:
                        class_num = 0;
                        total += sprintf(buf + total, "\nInterface descriptor       [%d:%d:%d  ] ",
                                        bConfigurationValue, interface_num++, class_num);
                        break;
                case USB_DT_ENDPOINT:
                        class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "  Endpoint descriptor      [%d:%d:%d:%d] ",
                                        bConfigurationValue, interface_num - 1, class_num, ++endpoint_num);
                        break;
                case USB_DT_OTG:
                        class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "OTG descriptor             [%d      ] ", bConfigurationValue);
                        break;
                case USB_DT_INTERFACE_ASSOCIATION:
                        class_num = endpoint_num = 0;
                        total += sprintf(buf + total, "\nIAD descriptor             [%d:%d    ] ", 
                                        bConfigurationValue, interface_num);
                        break;
                default:
                        endpoint_num = 0;
                        total += sprintf(buf + total, "  Class descriptor         [%d:%d:%d  ] ",
                                        bConfigurationValue, interface_num-1  , ++class_num);
                        break;
                }
                total += dump_descriptor(buf + total, sp);
                switch (sp[1]) {
                case USB_DT_CONFIGURATION:
                        config = (struct usbd_configuration_descriptor *)sp;
                        total += sprintf(buf + total, "  bNumInterfaces           [      4] %02x\n", config->bNumInterfaces);
                        total += sprintf(buf + total, "  bConfigurationValue      [      5] %02x\n", config->bConfigurationValue);
                        total += dump_string_descriptor(buf + total, config->iConfiguration, "iConfiguration");
                        total += sprintf(buf + total, "  bmAttributes             [      7] %02x\n", config->bmAttributes);
                        total += sprintf(buf + total, "  bMaxPower                [      8] %02x\n", config->bMaxPower);
                        break;
                case USB_DT_DEVICE_QUALIFIER:
                        break;
                case USB_DT_OTHER_SPEED_CONFIGURATION:
                        break;
                case USB_DT_INTERFACE:
                        interface = (struct usbd_interface_descriptor *)sp;
                        total += sprintf(buf + total, "  bInterafaceNumber        [      2] %02x\n", interface->bInterfaceNumber);
                        total += sprintf(buf + total, "  bAlternateSetting        [      5] %02x\n", 
                                        interface->bAlternateSetting);
                        total += sprintf(buf + total, "  bNumEndpoints            [      5] %02x\n", interface->bNumEndpoints);
                        total += sprintf(buf + total, "  bInterface[Class,Sub,Pro][      5] %02x %02x %02x\n", 
                                        interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bInterfaceProtocol);
                        total += dump_string_descriptor(buf + total, interface->iInterface, "iInterface");
                        break;
                case USB_DT_ENDPOINT:
                        endpoint = (struct usbd_endpoint_descriptor *)sp;
                        break;
                case USB_DT_INTERFACE_ASSOCIATION:
                        iad = (struct usbd_interface_association_descriptor *)sp;
                        total += sprintf(buf + total, "  bFirstInterface          [      2] %02x\n", iad->bFirstInterface);
                        total += sprintf(buf + total, "  bInterfaceCount          [      3] %02x\n", iad->bInterfaceCount);
                        total += sprintf(buf + total, "  bFunction[Class,Sub,Pro] [      4] %02x %02x %02x\n", 
                                        iad->bFunctionClass, iad->bFunctionSubClass, iad->bFunctionProtocol);
                        total += dump_string_descriptor(buf + total, iad->iFunction, "iFunction");
                        break;
                default:
                        break;
                }
                wTotalLength -= sp[0];
                sp += sp[0];
        }
        total += sprintf(buf + total, "\n");
        return total;
}

/*! *
 * usbd_device_proc_read - implement proc file system read.
 * @param file
 * @param buf
 * @param count
 * @param pos
 *
 * Standard proc file system read function.
 *
 * We let upper layers iterate for us, *pos will indicate which device to return
 * statistics for.
 */
static ssize_t usbd_device_proc_read_functions (struct file *file, char *buf, size_t count, loff_t * pos)
{
        unsigned long page;
        int len = 0;
        int index;

        u8 config_descriptor[512];
        int config_size;

        //struct list_head *lhd;

        // get a page, max 4095 bytes of data...
//        if (!(page = GET_FREE_PAGE(GFP_KERNEL))) {

        if (!(page = GET_KERNEL_PAGE())) {
                return -ENOMEM;
        }

        len = 0;
        index = (*pos)++;

        switch(index) {

        case 0:
                len += sprintf ((char *) page + len, "usb-device list\n");
                break;
        
        case 1:
                if ( usbd_bus_instance && usbd_bus_instance->function_instance) {
                        int configuration = index;
                        struct usbd_function_instance *function_instance = usbd_bus_instance->function_instance;
                        //struct usbd_function_driver *function_driver;
                        int i;

                        //function_driver = function_instance->function_driver;

                        if ((config_size = usbd_get_descriptor(usbd_bus_instance->function_instance, config_descriptor, 
                                                        sizeof(config_descriptor),
                                                        USB_DT_DEVICE, 0)) > index) {
                                len += dump_device_descriptor((char *)page + len, config_descriptor );
                        }

                        if ((config_size = usbd_get_descriptor(usbd_bus_instance->function_instance, config_descriptor, 
                                                        sizeof(config_descriptor),
                                                        USB_DT_CONFIGURATION, 0)) > index) {
                                len += dump_config_descriptor((char *)page + len, config_descriptor );
                        }

                        #ifdef CONFIG_OTG_HIGH_SPEED
                        if ((config_size = usbd_get_descriptor(usbd_bus_instance->function_instance, config_descriptor, 
                                                        sizeof(config_descriptor),
                                                        USB_DT_DEVICE_QUALIFIER, 0)) > index) {
                                len += dump_config_descriptor((char *)page + len, config_descriptor );
                        }
                        if ((config_size = usbd_get_descriptor(usbd_bus_instance->function_instance, config_descriptor, 
                                                        sizeof(config_descriptor),
                                                        USB_DT_OTHER_SPEED_CONFIGURATION, 0)) > index) {
                                len += dump_config_descriptor((char *)page + len, config_descriptor );
                        }
                        #endif /* CONFIG_OTG_HIGH_SPEED */

                        for (i = 1; i < usbd_bus_instance->usbd_maxstrings; i++) {

                                struct usbd_string_descriptor *string_descriptor
                                        = usbd_get_string_descriptor (usbd_bus_instance->function_instance, i);
                                int k;

                                CONTINUE_UNLESS (string_descriptor);

                                len += sprintf((char *)page+len, "String                     [%2d:%2d  ] ", 
                                                i, string_descriptor->bLength);

                                // bLength = sizeof(struct usbd_string_descriptor) + 2*strlen(str)-2;

                                for (k = 0; k < (string_descriptor->bLength / 2) - 1; k++) {
                                        *(char *) (page + len) = (char) string_descriptor->wData[k];
                                        len++;
                                }
                                len += sprintf ((char *) page + len, "\n");
                        }
                }
                else 
                        len += list_function(&usbd_simple_drivers, (char *)page + len, "Not Enabled");

                break;
        
        case 2:
                len += list_function(&usbd_simple_drivers, (char *)page + len, "Simple Drivers");
                break;
        case 3:
                len += list_function(&usbd_composite_drivers, (char *)page + len, "Composite Drivers");
                break;
        case 4:
                len += list_function(&usbd_class_drivers, (char *)page + len, "Class Drivers");
                break;
        case 5:
                len += list_function(&usbd_interface_drivers, (char *)page + len, "Interface Drivers");
                break;
        }

        if (len > count) {
                len = -EINVAL;
        } 
        else if ((len > 0) && copy_to_user (buf, (char *) page, len)) {
                len = -EFAULT;
        }
        free_page (page);
        return len;
}

/* Module init ******************************************************************************* */

/*! 
 * usbd_device_proc_operations_functions - 
 */
static struct file_operations usbd_device_proc_operations_functions = {
        read:usbd_device_proc_read_functions,
};

/*! 
 * usbd_procfs_init () - 
 */
int usbd_procfs_init (void)
{
        struct proc_dir_entry *p;

        /* create proc filesystem entries
         */
        RETURN_ENOMEM_UNLESS ((p = create_proc_entry (USB_FUNCTIONS, 0, 0)));
        p->proc_fops = &usbd_device_proc_operations_functions;
        return 0;
}

/*! 
 * usbd_procfs_exit () - 
 */
void usbd_procfs_exit (void)
{
        // remove proc filesystem entry
        remove_proc_entry (USB_FUNCTIONS, NULL);
}


