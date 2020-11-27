#
# Belcarra OTG - On-The-Go 
# @(#) balden@seth2.belcarratech.com|otg/otgcore/Makefile-l26|20051116205002|30939
#
# Copyright (c) 2004 Belcarra Technologies Corp

# Copyright 2005-2006 Motorola, Inc.
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 12/12/2005         Motorola         Initial distribution
# 08/16/2006         Motorola         NFS Crash issue
# 03/16/2006         Motorola         vendor specific changes
# 10/18/2006         Motorola         Add Open Src Software language
# 12/12/2006         Motorola         Changes for Open Src compliance.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

OTG=$(TOPDIR)/drivers/otg
EXTRA_CFLAGS +=          -Wno-missing-prototypes -Wno-unused -Wno-format -I$(OTG) 
EXTRA_CFLAGS_nostdinc += -Wno-missing-prototypes -Wno-unused -Wno-format -I$(OTG) 

otgcore-objs		:= core-init-l24.o otg.o \
			otg-trace.o otg-trace-l24.o \
			otg-mesg.o otg-mesg-l24.o \
			otg-fw.o usbp-bops.o usbp-fops.o usbp-procfs.o

ifeq ($(CONFIG_OTG_GENERIC_HOTPLUG),y)
otgcore-objs		+= usb_vendor_request.o hotplug.o
endif

ifeq ($(CONFIG_OTG_USB_PERIPHERAL),y)
otgcore-objs		+= otg-fw-mn.o
endif

ifeq ($(CONFIG_OTG_USB_HOST),y)
otgcore-objs		+= otg-fw-mn.o
endif
                        
ifeq ($(CONFIG_OTG_USB_PERIPHERAL_OR_HOST),y)
otgcore-objs		+= otg-fw-mn.o
endif

ifeq ($(CONFIG_OTG_BDEVICE_WITH_SRP),y)
otgcore-objs		+= otg-fw-df.o
endif

ifeq ($(CONFIG_OTG_DEVICE),y)
otgcore-objs		+= otg-fw-df.o
endif


#ifeq ("$(CONFIG_OTG_TRACE)", "y")
#otgcore-objs		+= usbp-procfs.o
#endif

#usbprocfs-objs		:= usbp-procfs.o

obj-$(CONFIG_OTG)	+= otgcore.o 
#obj-$(CONFIG_OTG_PROCFSM)       += usbprocfs.o

#otgtrace-objs		:= otg-trace.o
#obj-$(CONFIG_OTG_TRACE)	+= otgtrace.o 

