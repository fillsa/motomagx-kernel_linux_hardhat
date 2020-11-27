/*
 * otg/ocd/otglib/otg-pci.c -- Generic PCI driver
 * @(#) balden@belcarra.com|otg/ocd/otglib/otg-pci.c|20051117000523|04070
 *
 *      Copyright (c) 2005 Belcarra
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
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
 * @file otg/ocd/otglib/otg-pci.c
 * @brief Generic PCI Driver.
 *
 * This supports using a single PCI device to implement various USB OTG
 * subsidiary drivers.
 *
 * The hardware driver is split into drivers for the following.
 *
 * Required:
 *
 *      1. pci
 *
 * One or more of the following:
 *
 *      2. ocd
 *      3. pcd
 *      4. hcd
 *      5. tcd
 *
 * The pci driver implements a standard pci driver structure which will
 * be used to register with the linux pci support. During the probe function
 * the otg_pci_probe() function should be called to create an otg_dev
 * structure for the device. This is used to keep track of all of the additional
 * drivers.
 *
 * Each of the optional drivers will use otg_register_driver() to let the
 * pci layer know what optional drivers are available.
 *
 */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <linux/usb.h>
#include <linux/delay.h>

#include <otg/otg-compat.h>



/*
#include <core/hcd.h>
#include <otg/usbp-hub.h>
#include <otg/otg-api.h>
#include <otg/otg-utils.h>
*/


#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>  

/* Other includes*/
#include <linux/pci.h>
#include <linux/poll.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/dma.h>

#include <otg/otg-compat.h>
#include <otg/otg-trace.h>
#include <otg/otg-pci.h>
#include <otg/otg-utils.h>


/* ********************************************************************************************* */
static struct otg_dev       *otg_devs;

/* ********************************************************************************************* */
/*!
 * otg_pci_isr() - interrupt service handler
 */
irqreturn_t otg_pci_isr(int irq, void *data, struct pt_regs *r) 
{
        struct pci_dev          *pci_dev = data;
        struct otg_dev          *otg_dev = pci_get_drvdata(pci_dev);
        int                     i;

        /* XXX spinlock */

        RETURN_IRQ_HANDLED_UNLESS(otg_dev);

        TRACE_MSG0(otg_dev->PCI, "---------------------------------------- Start"); 

        for (i = OTG_DRIVER_TCD; i < OTG_DRIVER_TYPES; i++) {
                struct otg_driver *otg_driver;
                otg_driver = otg_dev->otg_pci_driver->drivers[i];
                CONTINUE_UNLESS(otg_driver);
                TRACE_MSG2(otg_dev->PCI, "try %s %d", otg_driver->name, i);
                CONTINUE_UNLESS(otg_driver->isr);
                RETURN_IRQ_HANDLED_IF_IRQ_HANDLED (otg_driver->isr(otg_dev, data));
                TRACE_MSG2(otg_dev->PCI, "not handled by %s %d", otg_driver->name, i);
        }

        TRACE_MSG2(otg_dev->PCI, "try %s %d", otg_dev->otg_pci_driver->name, i);

        if (otg_dev->otg_pci_driver->isr) 
                RETURN_IRQ_HANDLED_IF_IRQ_HANDLED (otg_dev->otg_pci_driver->isr(otg_dev, data));
        

        /* XXX spinlock */
        TRACE_MSG0(otg_dev->PCI, "---------------------------------------- IRQ_NONE ----"); 
	return IRQ_NONE;
} 

/* ********************************************************************************************* */

/*!
 * otg_get_id() - find a free ID
 */
int otg_get_id(struct pci_dev *pci_dev)
{

        int i;
        UNLESS (otg_devs)
                return 0;

        for (i = 0; ; i++) {

                struct otg_dev *link;
                for (link = otg_devs; link; link = link->next) {
                        printk(KERN_INFO"%s: i: %d link: %x id: %d\n", __FUNCTION__, i, link, link->id); 
                        BREAK_IF (link->id == i);
                }
                CONTINUE_IF(link);
                printk(KERN_INFO"%s: i: %d\n", __FUNCTION__, i); 
                return i;
        }
}

/*!
 * otg_pci_free_dev
 */
void otg_pci_free_dev(struct pci_dev *pci_dev, struct otg_dev *otg_dev)
{
        int                     region;
        unsigned long           resource_start;
        unsigned long           resource_len;

        RETURN_UNLESS(otg_dev);

        for (region = 0; region < DEVICE_COUNT_RESOURCE; region++) {

                CONTINUE_UNLESS(otg_dev->pci_regions & (1 << region));

                if (otg_dev->regs[region]) iounmap(otg_dev->regs[region]);

                resource_start = pci_resource_start(pci_dev, region);
                resource_len = pci_resource_len(pci_dev, region);
                release_mem_region(resource_start, resource_len);
        }
}


/*!
 * otg_pci_probe() - otg pci probe function
 *
 * Get the standard PCI resources allocated.
 *
 */
int __devinit otg_pci_probe (struct pci_dev *pci_dev, const struct pci_device_id *id, struct otg_pci_driver *otg_pci_driver)
{

        struct otg_driver       *otg_driver;
        struct otg_dev          *otg_dev = NULL;
        int                     enabled = 0;
        int                     irq = 0;
        int                     region;

        u8                      latency, limit;


        /* allocate otg_dev structure and fill in standard fields */
        THROW_UNLESS((otg_dev = kmalloc(sizeof(struct otg_dev), SLAB_KERNEL)), error);

        memset(otg_dev, 0, sizeof(struct otg_dev));
        otg_dev->PCI = otg_trace_obtain_tag();


        //printk(KERN_INFO"%s: PCI %d\n", __FUNCTION__, otg_dev->PCI); 
        //TRACE_MSG0(otg_dev->PCI, "TEST");

        THROW_UNLESS((enabled = !pci_enable_device(pci_dev)), error);

        otg_dev->otg_pci_driver = otg_pci_driver;
        otg_dev->pci_regions = otg_pci_driver->pci_regions;
        pci_set_drvdata(pci_dev, otg_dev);
        printk(KERN_INFO"%s: pci_dev: %x otg_dev: %x drv_data: %x\n", __FUNCTION__, pci_dev, otg_dev, pci_get_drvdata(pci_dev)); 

        for (region = 0; region < DEVICE_COUNT_RESOURCE; region++) {
                unsigned long           resource_start;
                unsigned long           resource_len;
                TRACE_MSG5(otg_dev->PCI, "[%2d] flags: %08x start: %08x end: %08x len: %08x", region,
                                pci_resource_flags(pci_dev, region),
                                pci_resource_start(pci_dev, region),
                                pci_resource_end(pci_dev, region),
                                pci_resource_len(pci_dev, region)
                                ); 

                CONTINUE_UNLESS(otg_dev->pci_regions & (1 << region));

                resource_start = pci_resource_start(pci_dev, region);
                resource_len = pci_resource_len(pci_dev, region);

                TRACE_MSG5(otg_dev->PCI, "pci_dev: %x otg_dev: %x start: %lx len: %lx name: %s", 
                                pci_dev, otg_dev, resource_start, resource_len, otg_pci_driver->name); 

                THROW_UNLESS(request_mem_region(resource_start, resource_len, otg_pci_driver->name), error);
                THROW_UNLESS((otg_dev->regs[region] = ioremap_nocache(resource_start, resource_len)), error);
                TRACE_MSG2(otg_dev->PCI, "regs[%d] %x", region, otg_dev->regs[region]);
        }

        THROW_UNLESS((irq = !request_irq(pci_dev->irq, otg_pci_isr, SA_SHIRQ, otg_pci_driver->name, pci_dev)), error);
        TRACE_MSG1(otg_dev->PCI, "irq: %d", pci_dev->irq);

        /* bad pci latencies can contribute to overruns  - but where ?? */
        pci_read_config_byte (pci_dev, PCI_LATENCY_TIMER, &latency);
        pci_read_config_byte (pci_dev, PCI_MAX_LAT, &limit);
        TRACE_MSG2(otg_dev->PCI, "latency: %02x limit: %02x", latency, limit);

        if (latency && /* limit &&*/ (limit < latency)) {

                pci_write_config_byte (pci_dev, PCI_LATENCY_TIMER, limit);
                pci_read_config_byte (pci_dev, PCI_LATENCY_TIMER, &latency);
                TRACE_MSG2(otg_dev->PCI, "latency: %02x limit: %02x", latency, limit);
        }



	/* XXX lock? */
        otg_dev->id = otg_get_id(pci_dev);
        TRACE_MSG1(otg_dev->PCI, "id: %d", otg_dev->id); 
        if (otg_devs) {
                TRACE_MSG2(otg_dev->PCI, "otg_devs: %x new: %x", otg_devs, otg_dev); 
                otg_dev->next = otg_devs;
        }
        otg_devs = otg_dev;

        return 0;

        CATCH(error) {

                printk(KERN_INFO"%s: FAILED\n", __FUNCTION__); 
                pci_set_drvdata(pci_dev, NULL);

                if (irq) free_irq(pci_dev->irq, otg_dev);

                otg_pci_free_dev(pci_dev, otg_dev);

                if (otg_dev) kfree(otg_dev);

                if (enabled) pci_disable_device(pci_dev);
                
                return -EINVAL;
        }
} 

/*!
 * otg_pci_remove() - pci remove function
 */ 
void __devexit otg_pci_remove (struct pci_dev *pci_dev)
{
        struct otg_dev *otg_dev = pci_get_drvdata(pci_dev);

        printk(KERN_INFO"%s:\n", __FUNCTION__); 

        pci_set_drvdata(pci_dev, NULL);

        /* call subsidiary drivers remove functions */


        if (otg_dev) {
                
		if (otg_devs == otg_dev) {
			otg_devs = otg_dev->next;
		}
		else {
			struct otg_dev *link;
			for (link = otg_devs; link; link = otg_dev->next) {
				if (link->next == otg_dev) {
					link->next = otg_dev->next;
					break;
				}
			}
		}

                free_irq(pci_dev->irq, otg_dev);

                otg_pci_free_dev(pci_dev, otg_dev);

                otg_trace_invalidate_tag(otg_dev->PCI);
                kfree(otg_dev);
        }
        pci_disable_device(pci_dev);
} 


/* ********************************************************************************************* */

/*!
 * otg_register_driver() - sub-driver registration function
 */
int otg_register_driver(struct otg_pci_driver *otg_pci_driver, struct otg_driver *otg_driver)
{
	struct otg_dev *otg_dev;

	printk(KERN_INFO"%s: probe: %x\n", __FUNCTION__, otg_driver->probe); 

        otg_pci_driver->drivers[otg_driver->id] = otg_driver;

	if (otg_driver->probe) {
		for (otg_dev = otg_devs; otg_dev; otg_dev = otg_dev->next) {

			if (otg_driver->probe(otg_dev, otg_driver->id)) {
				otg_dev->otg_pci_driver->drivers[otg_driver->id] = NULL;
				return -EINVAL;
			}
#if 1
                        return 0;
#endif
		}
	}
	return 0;
}

/*!
 * otg_unregister_driver() - sub-driver unregistration function
 */
void otg_unregister_driver(struct otg_pci_driver *otg_pci_driver, struct otg_driver *otg_driver)
{
	struct otg_dev *otg_dev;

	printk(KERN_INFO"%s: probe: %x\n", __FUNCTION__, otg_driver->probe); 

	if (otg_driver->probe) {
		for (otg_dev = otg_devs; otg_dev; otg_dev = otg_dev->next) {
			otg_driver->remove(otg_dev, otg_driver->id);
#if 1
                        return;
#endif
		}
	}

        otg_pci_driver->drivers[otg_driver->id] = NULL;
}


void otg_pci_set_drvdata(struct otg_dev *otg_dev, void *data)
{
        otg_dev->drvdata = data;
}

void * otg_pci_get_drvdata(struct otg_dev *otg_dev)
{
        return otg_dev->drvdata;
}

/* ********************************************************************************************* */

/* ********************************************************************************************* */



//EXPORT_SYMBOL(otg_pci_register_driver);
//EXPORT_SYMBOL(otg_pci_unregister_driver);
OTG_EXPORT_SYMBOL(otg_pci_probe);
OTG_EXPORT_SYMBOL(otg_pci_remove);


OTG_EXPORT_SYMBOL(otg_register_driver);
OTG_EXPORT_SYMBOL(otg_unregister_driver);

OTG_EXPORT_SYMBOL(otg_pci_set_drvdata);
OTG_EXPORT_SYMBOL(otg_pci_get_drvdata);

