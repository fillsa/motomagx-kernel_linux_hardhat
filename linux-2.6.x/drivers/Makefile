#
# Makefile for the Linux kernel device drivers.
#
# Copyright (C) 2005 - 2006 Motorola Inc.
#
# 15 Sep 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#

obj-$(CONFIG_PCI)		+= pci/
obj-$(CONFIG_PARISC)		+= parisc/
obj-y				+= video/
obj-$(CONFIG_ACPI_BOOT)		+= acpi/
# PnP must come after ACPI since it will eventually need to check if acpi
# was used and do nothing if so
obj-$(CONFIG_PNP)		+= pnp/

# char/ comes before serial/ etc so that the VT console is the boot-time
# default.
obj-y				+= char/

# i810fb and intelfb depend on char/agp/
obj-$(CONFIG_FB_I810)           += video/i810/
obj-$(CONFIG_FB_INTEL)          += video/intelfb/

# we also need input/serio early so serio bus is initialized by the time
# serial drivers start registering their serio ports
obj-$(CONFIG_SERIO)		+= input/serio/
obj-y				+= serial/
obj-$(CONFIG_PARPORT)		+= parport/
obj-y				+= base/ block/ misc/ net/ mpm/
obj-$(CONFIG_I2C)               += i2c/
obj-y				+= media/ ssi/
obj-$(CONFIG_NUBUS)		+= nubus/
obj-$(CONFIG_ATM)		+= atm/
obj-$(CONFIG_PPC_PMAC)		+= macintosh/
obj-$(CONFIG_IDE)		+= ide/
obj-$(CONFIG_FC4)		+= fc4/
obj-$(CONFIG_PROFILER)          += profiler/	        
obj-$(CONFIG_SCSI)		+= scsi/
obj-$(CONFIG_FUSION)		+= message/
obj-$(CONFIG_IEEE1394)		+= ieee1394/
obj-y				+= cdrom/
obj-$(CONFIG_MTD)		+= mtd/
obj-$(CONFIG_SPI)		+= spi/
obj-$(CONFIG_PCCARD)		+= pcmcia/
obj-$(CONFIG_DIO)		+= dio/
obj-$(CONFIG_SBUS)		+= sbus/
obj-$(CONFIG_ZORRO)		+= zorro/
obj-$(CONFIG_MAC)		+= macintosh/
obj-$(CONFIG_PARIDE) 		+= block/paride/
obj-$(CONFIG_TC)		+= tc/
obj-$(CONFIG_USB)		+= usb/
obj-$(CONFIG_USB_GADGET)	+= usb/gadget/
obj-$(CONFIG_INPUT)		+= input/
obj-$(CONFIG_GAMEPORT)		+= input/gameport/
obj-$(CONFIG_I2O)		+= message/
obj-$(CONFIG_W1)		+= w1/
obj-$(CONFIG_PHONE)		+= telephony/
obj-$(CONFIG_MD)		+= md/
obj-$(CONFIG_ISDN)		+= isdn/
obj-$(CONFIG_MCA)		+= mca/
obj-$(CONFIG_EISA)		+= eisa/
obj-$(CONFIG_CPU_FREQ)		+= cpufreq/
obj-$(CONFIG_ARCH_MXC)		+= mxc/
obj-$(CONFIG_OTG)		+= otg/
obj-$(CONFIG_DPM)		+= dpm/
obj-$(CONFIG_SIM_SUPPORT)       += sim/
obj-y				+= firmware/
obj-$(CONFIG_EVENT_BROKER)	+= evb/
obj-$(CONFIG_MOT_POWER_IC)      += power_ic/
obj-$(CONFIG_MMC)		+= mmc/
obj-$(CONFIG_APLOGGER)		+= aplogger/
obj-$(CONFIG_CAPTOUCH)          += keyv/
