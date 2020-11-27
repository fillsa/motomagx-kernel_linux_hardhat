/*
 * otg/ocd/otglib/pci.h -- Generic PCI driver
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-pci.h|20051116205001|33958
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
 * @file otg/otg/otg-pci.h
 * @brief Generic PCI Driver.
 *
 * This is the base PCI driver for supporting the PCI devices.
 *
 * It will probe the hardware and support registration for the sub-device
 * drivers (ocd, hcd and pcd).
 *
 * otg_pci_register_driver() and otg_pci_unregister_driver() are used
 * to register the base PCI hardware driver.
 *
 * otg_register_driver() and otg_unregister_driver() are used to register
 * the ocd, pcd, hcd, tcd 
 *
 */

#define OTG_DRIVER_OCD  0
#define OTG_DRIVER_PCD  1
#define OTG_DRIVER_HCD  2
#define OTG_DRIVER_TCD  3
#define OTG_DRIVER_TYPES 4


struct otg_dev;


struct otg_driver {
        char                    *name;
        int                     id;


        irqreturn_t             (*isr)(struct otg_dev *, void *);

        int (*probe)(struct otg_dev *dev, int id);     
        void (*remove)(struct otg_dev *dev, int id);    

#ifdef CONFIG_PM
        void (*suspend)(struct otg_dev *dev, u32 state, int id);       /* Device suspended */
        void (*resume)(struct otg_dev *dev, int id);        /* Device woken up */
#endif /* CONFIG_PM */

};

struct otg_drivers {

        int                     pci_region;
        char                    *name;
        irqreturn_t             (*isr)(struct otg_dev *, void *);

        /* sub-drivers supporting various OTG functions from the hardware */
        struct otg_driver       *drivers[OTG_DRIVER_TYPES];
};


struct otg_dev {
        /* base hardware PCI driver and interrupt handler */
        struct pci_dev          *pci_dev;

        struct otg_drivers      *otg_drivers;

        struct otg_dev          *next;

        spinlock_t              lock;


        /* PCI mapping information */
        unsigned long           resource_start;
        unsigned long           resource_len;

        void                    *base;
        void                    *regs;

        int                     id;

        /* OTG Driver data */
        void                    *drvdata;
};



int __devinit otg_pci_probe (struct pci_dev *pci_dev, const struct pci_device_id *id, struct otg_drivers *otg_drivers);

void __devexit otg_pci_remove (struct pci_dev *pci_dev);

//extern int otg_pci_register_driver(struct pci_driver *);
//extern void otg_pci_unregister_driver(struct pci_driver *);

extern int otg_register_driver(struct otg_drivers*, struct otg_driver *);
extern void otg_unregister_driver(struct otg_drivers*, struct otg_driver *);

extern void otg_pci_set_drvdata(struct otg_dev *, void *);
extern void * otg_pci_get_drvdata(struct otg_dev *);


/* ********************************************************************************************** */

static void inline otg_writel(struct otg_dev *dev, u32 data, u32 reg)
{
        writel(data, dev->regs + reg);
}


static u32 inline otg_readl(struct otg_dev *dev, u32 reg)
{
        return readl(dev->regs + reg);
}


static u16 inline otg_readw(struct otg_dev *dev, u32 reg)
{
        return readw(dev->regs + reg);
}

static void inline otg_writew(struct otg_dev *dev, u16 data, u32 reg)
{
        writew(data, dev->regs + reg);
}

static u8 inline otg_readb(struct otg_dev *dev, u32 reg)
{
        return readb(dev->regs + reg);
}

static void inline otg_writeb(struct otg_dev *dev, u8 data, u32 reg)
{
        writeb(data, dev->regs + reg);
}


/* End of FILE */
