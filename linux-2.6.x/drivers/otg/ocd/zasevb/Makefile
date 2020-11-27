#
# @(#) balden@seth2.belcarratech.com|otg/ocd/zasevb/Makefile-l26|20051116205001|01111
# Copyright (c) 2005 Belcarra
# Copyright 2005-2006 Motorola, Inc.
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 12/12/2005         Motorola         Initial distribution 
# 06/12/2006         Motorola         OTG name change 
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
OTGCORE_DIR=$(OTG)/otgcore
USBCORE=$(TOPDIR)/drivers/usb
MXCMC13783_LEGACYDIR=$(TOPDIR)/drivers/mxc/mc13783_legacy
EXTRA_CFLAGS += -I$(OTG) -Wno-unused -Wno-format           -I$(USBCORE) -I$(OTGCORE_DIR) -I$(MXCMC13783_LEGACYDIR)
EXTRA_CFLAGS_nostdinc += -I$(OTG) -Wno-unused -Wno-format  -I$(USBCORE) -I$(OTGCORE_DIR) -I$(MXCMC13783_LEGACYDIR)

# #########################################################################################################

#
# USB Configurations - peripheral, host or both
#
# Produce zasevb_usb module.
#

ifeq ($(CONFIG_OTG_USB_PERIPHERAL),y)
obj-$(CONFIG_OTG_ZASEVB) += zasevb_usb.o
endif
			
ifeq ($(CONFIG_OTG_USB_HOST),y)
obj-$(CONFIG_OTG_ZASEVB) += zasevb_usb.o
endif
			
ifeq ($(CONFIG_OTG_USB_PERIPHERAL_OR_HOST),y)
obj-$(CONFIG_OTG_ZASEVB) += zasevb_usb.o
endif

zasevb_usb-objs						:= zasevb-l26.o
zasevb_usb-objs						+= ../mxc/mxc-ocd.o 
zasevb_usb-objs						+= ../mxc/mxc-procfs.o 
zasevb_usb-objs						+= ../mxc/mxc-gpio.o

zasevb_usb-$(CONFIG_OTG_USB_PERIPHERAL)         	+= ../otglib/pcd.o ../mxc/mxc-pcd.o
zasevb_usb-$(CONFIG_OTG_USB_HOST)                       += ../otglib/pcd.o ../mxc/mxc-pcd.o
zasevb_usb-$(CONFIG_OTG_USB_HOST)                       += ../mxc/mxc-hcd.o ../mxc/mxc-l26.o
zasevb_usb-$(CONFIG_OTG_USB_PERIPHERAL_OR_HOST) 	+= ../otglib/pcd.o ../mxc/mxc-pcd.o
zasevb_usb-$(CONFIG_OTG_USB_PERIPHERAL_OR_HOST) 	+= ../mxc/mxc-hcd.o ../mxc/mxc-l26.o

zasevb_usb-$(CONFIG_OTG_GPTR)				+= ../mxc/mxc-gptcr.o
zasevb_usb-$(CONFIG_OTG_HRT)				+= ../mxc/mxc-hrt.o

zasevb_usb-${CONFIG_OTG_ZASEVB_ISP1301}  		+= zasevb-isp1301.o ../isp1301/isp1301.o \
					   		../isp1301/isp1301-procfs.o ../otglib/i2c-l26.o

zasevb_usb-${CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY} 	+= zasevb-mc13783.o ../mxc/mxc-mc13783.o
zasevb_usb-${CONFIG_ZASEVB_MC13783_POWERIC}		+= zasevb-poweric.o
zasevb_usb-$(CONFIG_OTG_ZASEVB_PMIC)			+= zasevb-sc55112.o ../mxc/mxc-sc55112.o
			
# #########################################################################################################

#
# OTG Configurations - peripheral with SRP or full OTG Device
#
# Produce zasevb_otg module.
#

ifeq ($(CONFIG_OTG_BDEVICE_WITH_SRP),y)
obj-$(CONFIG_OTG_ZASEVB) += zasevb_otg.o
endif
			
ifeq ($(CONFIG_OTG_DEVICE),y)
obj-$(CONFIG_OTG_ZASEVB) += zasevb_otg.o
endif

zasevb_otg-objs						:= zasevb-l26.o
zasevb_otg-objs						+= ../mxc/mxc-ocd.o 
zasevb_otg-objs						+= ../mxc/mxc-procfs.o 

zasevb_otg-$(CONFIG_OTG_BDEVICE_WITH_SRP)		+= ../otglib/pcd.o ../mxc/mxc-pcd.o
zasevb_otg-$(CONFIG_OTG_DEVICE)				+= ../otglib/pcd.o ../mxc/mxc-pcd.o

zasevb_otg-$(CONFIG_OTG_DEVICE)				+= ../mxc/mxc-hcd.o ../mxc/mxc-l26.o
zasevb_otg-$(CONFIG_OTG_GPTR)				+= ../mxc/mxc-gptcr.o
zasevb_otg-$(CONFIG_OTG_HRT)				+= ../mxc/mxc-hrt.o


zasevb_otg-${CONFIG_OTG_ZASEVB_ISP1301}  		+= zasevb-isp1301.o ../isp1301/isp1301.o \
					   		../isp1301/isp1301-procfs.o ../otglib/i2c-l26.o


zasevb_otg-${CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY} 	+= zasevb-mc13783.o ../mxc/mxc-mc13783.o
zasevb_otg-${CONFIG_ZASEVB_MC13783_POWERIC}		+= zasevb-poweric.o
zasevb_otg-$(CONFIG_OTG_ZASEVB_PMIC)			+= zasevb-sc55112.o ../mxc/mxc-sc55112.o

			

