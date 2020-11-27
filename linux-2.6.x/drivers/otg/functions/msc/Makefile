# Function driver for a Random Mouse
# @(#) balden@seth2.belcarratech.com|otg/functions/msc/Makefile-l26|20051116204958|19042
#
# Copyright (c) 2004 Belcarra
#
# Copyright 2006 Motorola, Inc.
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 03/16/2006         Motorola         Initial distribution 
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

msc_if-objs	:= msc-fd.o crc.o msc-linux.o msc-bo.o msc-io-l24.o


obj-$(CONFIG_OTG_MSC) += msc_if.o

OTG=$(TOPDIR)/drivers/otg
ACMD=$(OTG)/functions/msc
OTGCORE_DIR=$(OTG)/otgcore
USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)
