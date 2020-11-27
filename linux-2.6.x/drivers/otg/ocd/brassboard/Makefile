#
# Copyright (c) 2005 Belcarra
# @(#) balden@seth2.belcarratech.com|otg/ocd/brassboard/Makefile-l26|20051116204959|10333
#
# Copyright 2006 Motorola, Inc.
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 08/18/2006         Motorola         Initial distribution 
# 10/18/2006         Motorola         Add Open Src Software language
# 12/11/2006         Motorola         Changes for Open src compliance.
#
# This Program is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of
# MERCHANTIBILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# This program is free software; you can redistribute it
# and/or modify it under the terms of the GNU General
# Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at
# your option) any later version.  You should have
# received a copy of the GNU General Public License
# along with this program; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave,
# Cambridge, MA 02139, USA

OTG=$(TOPDIR)/drivers/otg
FX2LPDIR=$(OTG)/ocd/fx2lp
OTGCORE_DIR=$(OTG)/otgcore
MXCMC13783_LEGACYDIR=$(TOPDIR)/drivers/mxc/mc13783_legacy/
USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR) -I$(MXCMC13783_LEGACYDIR) -DFX2LP_NODMA -DFX2LP_HSUSB -I$(FX2LPDIR)
EXTRA_CFLAGS_nostdinc += -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR) -I$(MXCMC13783_LEGACYDIR)

# Link rules for multi-part drivers.

obj-$(CONFIG_OTG_BRASSBOARD) += brassboard_tr.o

brassboard_tr-objs := brassboard-tr.o
brassboard_tr-objs += ../mxc/mxc-gpio.o
brassboard_tr-objs += ../otglib/pcd.o
#ifeq ($(CONFIG_OTG_HRT),y)
brassboard_tr-objs += ../mxc/mxc-hrt.o 
#endif
brassboard_tr-objs += brassboard-mc13783-sierra.o
brassboard_tr-objs += ../mxc/mc13783-poweric.o 
brassboard_tr-objs += ../mxc/mxc-ocd.o
brassboard_tr-objs += ../mxc/mxc-pcd.o
