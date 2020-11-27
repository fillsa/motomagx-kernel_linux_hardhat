#
# Network Function Driver
# @(#) balden@seth2.belcarratech.com|otg/functions/network/Makefile-l26|20051116204958|49152
#
# Copyright (C) 2002-2004 Belcarra Technologies Corp
#
# Copyright 2005-2006 Motorola, Inc.
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 06/08/2005         Motorola         Initial distribution 
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


network_if-objs	:= net-l24-os.o net-fd.o blan-if.o basic-if.o basic2-if.o cdc-if.o safe-if.o fermat.o

obj-$(CONFIG_OTG_NETWORK) += network_if.o

OTG=$(TOPDIR)/drivers/otg
NETWORKD=$(OTG)/functions/network
OTGCORE_DIR=$(OTG)/otgcore
USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(NETWORKD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(NETWORKD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)



# Link rules for multi-part drivers.

#network_fd.o: $(network_fd-objs)
#	$(LD) -r -o $@ $(network_fd-objs)

