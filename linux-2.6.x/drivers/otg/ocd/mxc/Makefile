#
# Makefile for the kernel USBD (device not host) drivers.
# @(#) balden@seth2.belcarratech.com|otg/ocd/mxc/Makefile-l26|20051116203403|58964
#
# Copyright (c) 2005 Belcarra Technologies Corp

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


#extra-y := mc13783.o mxc-hrt.o mxc-ocd.o mxc-pcd.o mxc-procfs.o mxc-l26.o mxc-hcd.o 
#obj-y := empty.o

OTG=$(TOPDIR)/drivers/otg
OTGCORE_DIR=$(OTG)/otgcore
USBDCORE_DIR=$(TOPDIR)/drivers/usb
MXCMC13783_LEGACYDIR=$(TOPDIR)/drivers/mxc/mc13783_legacy
PMICRRDIR=$(TOPDIR)/drivers/mxc/sc55112
EXTRA_CFLAGS += -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR) -I$(MXCMC13783_LEGACYDIR) -I$(PMICRRDIR)
EXTRA_CFLAGS_nostdinc += -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR) -I$(MXCMC13783_LEGACYDIR) -I$(PMICRRDIR)

# #########################################################################################################

obj-$(CONFIG_OTG_ZASEVB) += hcd_test.o
hcd_test-objs						:= hcd-test.o

