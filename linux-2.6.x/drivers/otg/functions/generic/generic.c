/*
 * otg/functions/generic/generic-cf.c
 * @(#) sl@whiskey.enposte.net|otg/functions/generic/generic.c|20051116230343|51923
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2006-2008 Motorola, Inc. 
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 06/08/2006         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 10/02/2007         Motorola         Correct the Product String in USB Device Descriptor
 * 08/08/2008         Motorola         Commented out OTG debug from dmesg
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA
 *
 */
/*!
 * @file otg/functions/generic/generic-cf.c
 * @brief Generic Configuration Function Driver 
 *
 * This implements a generic composite function
 *
 *         - idVendor              16 bit word
 *         - idProduct             16 bit word
 *         - bcd_device            16 bit word
 *
 *         - bDeviceClass          byte
 *         - device_sub-class      byte
 *         - bcdDevice             byte
 *
 *         - vendor                string
 *         - iManufacturer         string
 *         - product               string
 *         - serial                string
 * 
 *         - power                 byte
 *         - remote_wakeup         bit
 * 
 *         - functions             string
 * 
 * 
 * The functions string would contain a list of names, the first would
 * be the composite function driver, the rest would be the interface
 * functions. For example:
 * 
 *         
 *         "cmouse-cf mouse-if"
 *         "mcpc-cf dun-if dial-if obex-if dlog-if"
 *
 * There are also a set of pre-defined configurations that
 * can be loaded singly or in toto.
 *
 * @ingroup GenericFunction
 */

#include <linux/config.h>
#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-cdc.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-utils.h>

#include "generic.h"

struct usbd_function_instance *composite_function; /*!< function instance for this module */

/* Module Parameters ******************************************************** */
/* ! 
 * @name XXXXX MODULE Parameters
 */
/* ! @{ */

MOD_AUTHOR ("sl@belcarra.com"); 
EMBED_LICENSE();
MOD_DESCRIPTION ("Generic Composite Function");


MOD_PARM_STR (driver_name, "Driver Name", NULL);

MOD_PARM_INT(idVendor, "Device Descriptor idVendor field", 0);
MOD_PARM_INT(idProduct, "Device Descriptor idProduct field", 0);
MOD_PARM_INT(bcddevice, "Device Descriptor bcdDevice field", 0);

MOD_PARM_INT (bDeviceClass, "Device Descriptor bDeviceClass field", 0);
MOD_PARM_INT (bDeviceSubClass, "Device Descriptor bDeviceSubClass field", 0);
MOD_PARM_INT (bDeviceProtocol, "Device Descriptor bDeviceProtocol field", 0);
MOD_PARM_INT (bcdDevice, "Device Descriptor bcdDevice field", 0);

MOD_PARM_STR (iConfiguration, "Configuration Descriptor iConfiguration field", NULL);
MOD_PARM_STR (iManufacturer, "Device Descriptor iManufacturer filed", NULL);
MOD_PARM_STR (iProduct, "Device Descriptor iProduct filed", NULL);
MOD_PARM_STR (iSerialNumber, "Device Descriptor iSerialNumber filed", NULL);

MOD_PARM_STR (class_name, "Class Function Name", NULL);
MOD_PARM_STR (interface_names, "Interface Function Names List", NULL);

MOD_PARM_STR (config_name, "Pre-defined Configuration", NULL);
MOD_PARM_BOOL (load_all, "Load all pre-defined Configurations", 1);

otg_tag_t GENERIC;


/* Setup the pre-defined configuration name from configuration
 * option if available.
 *
 * N.B. This list needs to be synchronized with both pre-defined
 * configurations (see below) and the Kconfig list
 */
extern char default_predefined_configuration[];

static char *generic_config_name(void){
	if(MODPARM(config_name) && strlen(MODPARM(config_name))) return MODPARM(config_name);
	return default_predefined_configuration;
}

static int preset_config_name(void){
	return (strlen(generic_config_name()) > 0);
}
	

/* ! *} */

/* Pre-defined configurations *********************************************** */

struct generic_config generic_config = {
        .composite_driver.driver.name = "generic",
        .device_description.idVendor = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_VENDORID),
        .device_description.idProduct = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_PRODUCTID),
        .device_description.bcdDevice = __constant_cpu_to_le16(CONFIG_OTG_GENERIC_BCDDEVICE),
        .device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER,
        .device_description.iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME,
};

extern struct generic_config generic_configs[];

/* USB Module init/exit ***************************************************** */

/*! generic_cf_function_enable - called by USB Device Core to enable the driver
 * @param function The function instance for this driver to use.
 * @return non-zero if error.
 */
static int generic_cf_function_enable (struct usbd_function_instance *function)
{
        TRACE_MSG0(GENERIC,"--"); 
        // XXX MODULE LOCK HERE
        return 0;
}

/*! generic_cf_function_disable - called by the USB Device Core to disable the driver
 * @param function The function instance for this driver
 */
static void generic_cf_function_disable (struct usbd_function_instance *function)
{
        TRACE_MSG0(GENERIC,"--"); 
        // XXX MODULE UNLOCK HERE
}



/*! function_ops - operations table for the USB Device Core
 *  */     
static struct usbd_function_operations generic_function_ops = {
        .function_enable = generic_cf_function_enable,
        .function_disable = generic_cf_function_disable,
};



#if !defined(OTG_C99)

void generic_cf_global_init(void)
{
}

#else /* defined(OTG_C99) */

#endif /* defined(OTG_C99) */

/*! @} */ 

static int generic_count;

/* GENERIC ****************************************************************** */

/* USB Device Functions ***************************************************** */



/*!
 * generic_cf_register()
 *
 */
void generic_cf_register(struct generic_config *config, char *match)
{
        char *cp, *sp, *lp;
        int i;
        char **interface_list = NULL; 
        int interfaces = 0;

        //printk(KERN_INFO"%s: Driver: \"%s\" idVendor: %04x idProduct: %04x interface_names: \"%s\" match: \"%s\"\n", 
        //                __FUNCTION__,
        //                config->composite_driver.driver.name, MODPARM(idVendor), MODPARM(idProduct), config->interface_names, 
        //                match ? match : "");

        TRACE_MSG5(GENERIC, "Driver: \"%s\" idVendor: %04x idProduct: %04x interface_names: \"%s\" match: \"%s\"",
                        config->composite_driver.driver.name, MODPARM(idVendor), MODPARM(idProduct), config->interface_names, 
                        match ? match : "");


        RETURN_IF (match && strlen(match) && strcmp(match, config->composite_driver.driver.name));

        //printk(KERN_INFO"%s: MATCHED\n", __FUNCTION__); 

        /* decompose interface names to construct interface_list 
         */
        RETURN_UNLESS (config->interface_names && strlen(config->interface_names));

        /* count interface names and allocate _interface_names array
         */
        for (cp = sp = config->interface_names, interfaces = 0; cp && *cp; ) {
                for (; *cp && *cp == ':'; cp++);        // skip white space
                sp = cp;                                // save start of token
                for (; *cp && *cp != ':'; cp++);        // find end of token
                BREAK_IF (sp == cp);
                if (*cp) cp++;
                interfaces++;
        }

        THROW_UNLESS(interfaces, error);

        TRACE_MSG1(GENERIC, "interfaces: %d", interfaces);

        THROW_UNLESS((interface_list = (char **) CKMALLOC (sizeof (char *) * (interfaces + 1), GFP_KERNEL)), error);

        for (cp = sp = config->interface_names, interfaces = 0; cp && *cp; interfaces++) {
                for (; *cp && *cp == ':'; cp++);        // skip white space
                sp = cp;                                // save start of token
                for (; *cp && *cp != ':'; cp++);        // find end of token
                BREAK_IF (sp == cp);
                lp = cp;
                if (*cp) cp++;
                *lp = '\0';
                lp = CKMALLOC(strlen(sp), GFP_KERNEL);
                strcpy(lp, sp);
                interface_list[interfaces] = lp;

                TRACE_MSG3(GENERIC, "INTERFACE[%2d] %x \"%s\"", 
                                interfaces, interface_list[interfaces], interface_list[interfaces]);
        }

        config->composite_driver.device_description = &config->device_description;
        config->composite_driver.configuration_description = &config->configuration_description;
        config->composite_driver.driver.fops = &generic_function_ops;
        config->interface_list = interface_list;

        THROW_IF (usbd_register_composite_function (
                                &config->composite_driver,
                                config->composite_driver.driver.name,
                                config->class_name,
                                config->interface_list, NULL), error);

        config->registered++;

	TRACE_MSG0(GENERIC, "REGISTER FINISHED");

        CATCH(error) {
                otg_trace_invalidate_tag(GENERIC);
        }

}

/*! 
 * generic_cf_modinit() - module init
 *
 * This is called by the Linux kernel; either when the module is loaded
 * if compiled as a module, or during the system intialization if the 
 * driver is linked into the kernel.
 *
 * This function will parse module parameters if required and then register
 * the generic driver with the USB Device software.
 *
 */
static int generic_cf_modinit (void)
{
        int i;

        #if !defined(OTG_C99)
        //generic_cf_global_init();
        #endif /* defined(OTG_C99) */


        GENERIC = otg_trace_obtain_tag();

        i = MODPARM(idVendor);

        printk (KERN_INFO "Model ID is %s",MODPARM(iProduct));

#if 0
        TRACE_MSG4(GENERIC, "config_name: \"%s\" load_all: %d class_name: \"%s\" interface_names: \"%s\"",
                        MODPARM(config_name) ? MODPARM(config_name) : "", 
                        MODPARM(load_all), 
                        MODPARM(class_name) ? MODPARM(class_name) : "", 
                        MODPARM(interface_names) ? MODPARM(interface_names) : "");
#else

        TRACE_MSG5(GENERIC, "config_name: \"%s\" load_all: %d class_name: \"%s\" interface_names: \"%s\" Serial: \"%s\"",
                        generic_config_name(), 
                        MODPARM(load_all), 
                        MODPARM(class_name) ? MODPARM(class_name) : "", 
                        MODPARM(interface_names) ? MODPARM(interface_names) : "",
			MODPARM(iSerialNumber) ? MODPARM(iSerialNumber) : "");

#endif

        /* load config or configs
         */
        
        if (preset_config_name()  || MODPARM(load_all)) {

		if (preset_config_name()){
                        MODPARM(load_all) = 0;
		}

                printk (KERN_INFO "%s: config_name: \"%s\" load_all: %d\n", 
                                __FUNCTION__, generic_config_name() , MODPARM(load_all));

                /* search for named config
                 */
                for (i = 0; ; i++) {
                        struct generic_config *config = generic_configs + i;
                        BREAK_UNLESS(config->interface_names);
                        //printk(KERN_INFO"%s: checking[%d] \"%s\"\n", __FUNCTION__, i, config->composite_driver.driver.name);

			if (MODPARM(iSerialNumber) && strlen(MODPARM(iSerialNumber)) &&
                            /* For the moment, we will only use serial number for msc and mtp.  I suggest we come up with a more generic
                               way to determine if a function driver needs to use the serial number. (for instance another function member. */
                            ( ( !strcmp(config->composite_driver.driver.name, "mtp")) || !strcmp(config->composite_driver.driver.name, "msc")) ){
                              config->device_description.iSerialNumber = MODPARM(iSerialNumber);
			}

                        config->device_description.iProduct = MODPARM(iProduct);

                        generic_cf_register(config, generic_config_name());
                        //printk(KERN_INFO"%s: loaded  %s\n", __FUNCTION__, config->composite_driver.driver.name);
                }
        }
        else {
                struct generic_config *config = &generic_config;

                //printk (KERN_INFO "%s: idVendor: %04x idProduct: %04x\n", __FUNCTION__, MODPARM(idVendor), MODPARM(idProduct));
                //printk (KERN_INFO "%s: class_name: \"%s\" _interface_names: \"%s\"\n", 
                //                __FUNCTION__, MODPARM(class_name), MODPARM(interface_names));

                if (MODPARM(driver_name) && strlen(MODPARM(driver_name)))
                        config->composite_driver.driver.name = MODPARM(driver_name);

                if (MODPARM(class_name) && strlen(MODPARM(class_name)))
                        config->class_name = MODPARM(class_name);

                if (MODPARM(interface_names) && strlen(MODPARM(interface_names)))
                        config->interface_names = MODPARM(interface_names);

                if (MODPARM(iConfiguration) && strlen(MODPARM(iConfiguration)))
                        config->configuration_description.iConfiguration = MODPARM(iConfiguration);

                if (MODPARM(bDeviceClass))
                        config->device_description.bDeviceClass = MODPARM(bDeviceClass);

                if (MODPARM(bDeviceSubClass))
                        config->device_description.bDeviceSubClass = MODPARM(bDeviceSubClass);

                if (MODPARM(bDeviceProtocol))
                        config->device_description.bDeviceProtocol = MODPARM(bDeviceProtocol);

                if (MODPARM(idVendor)) 
                        config->device_description.idVendor = MODPARM(idVendor);
                else
                        config->device_description.idVendor = CONFIG_OTG_GENERIC_VENDORID;

                if (MODPARM(idProduct))
                        config->device_description.idProduct = MODPARM(idProduct);
                else
                        config->device_description.idProduct = CONFIG_OTG_GENERIC_PRODUCTID;

                if (MODPARM(bcdDevice))
                        config->device_description.bcdDevice = MODPARM(bcdDevice);
                else
                        config->device_description.bcdDevice = CONFIG_OTG_GENERIC_BCDDEVICE;

                if (MODPARM(iManufacturer) && strlen(MODPARM(iManufacturer)))
                        config->device_description.iManufacturer = MODPARM(iManufacturer);
                else
                        config->device_description.iManufacturer = CONFIG_OTG_GENERIC_MANUFACTURER;

                if (MODPARM(iProduct) && strlen(MODPARM(iProduct)))
                        config->device_description.iProduct = MODPARM(iProduct);
                else
                        config->device_description.iProduct = CONFIG_OTG_GENERIC_PRODUCT_NAME;

                if (MODPARM(iSerialNumber) && strlen(MODPARM(iSerialNumber))){
                        config->device_description.iSerialNumber = MODPARM(iSerialNumber);
		}
                if (MODPARM(interface_names))
                        config->interface_names = MODPARM(interface_names);

                generic_cf_register(config, NULL);
        }
        return 0;
}
#ifdef CONFIG_OTG_NFS
late_initcall (generic_cf_modinit);
#else
module_init (generic_cf_modinit);
#endif

#if OTG_EPILOGUE
/*! 
 * generic_cf_modexit() - module init
 *
 * This is called by the Linux kernel; when the module is being unloaded 
 * if compiled as a module. This function is never called if the 
 * driver is linked into the kernel.
 *
 * @param void
 * @return void
 */
static void generic_cf_modexit (void)
{
        int i;
        otg_trace_invalidate_tag(GENERIC);
        for (i = 0; ; i++) {
                struct generic_config *config = generic_configs + i;

                BREAK_UNLESS(config->interface_names);

                //printk(KERN_INFO"%s: %s registered: %d\n", 
                //                __FUNCTION__, config->composite_driver.driver.name, config->registered);

                CONTINUE_UNLESS(config->registered);

                usbd_deregister_composite_function (&config->composite_driver);

                if (config->interface_list)
                        LKFREE(config->interface_list);
        }

        if (generic_config.registered)
                usbd_deregister_composite_function (&generic_config.composite_driver);

        if (generic_config.interface_list) 
                LKFREE(generic_config.interface_list);
}

module_exit (generic_cf_modexit);
#endif

