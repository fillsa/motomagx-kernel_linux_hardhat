# Function driver for a Random Mouse
# @(#) balden@seth2.belcarratech.com|otg/functions/isotest/Makefile-l26|20051116204957|14953
#
# Copyright (c) 2004 Belcarra
#
# Copyright (c) 2005-2006 Motorola, Inc.
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 12/12/2005         Motorola         dn Initial distribution
# 10/18/2006         Motorola         ma sk pw Add Open Src Software language
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

isotest_if-objs	:= iso.o test.o fermat.o


obj-$(CONFIG_OTG_ISOTEST) += isotest_if.o

OTG=$(TOPDIR)/drivers/otg
ACMD=$(OTG)/functions/isotest
OTGCORE_DIR=$(OTG)/otgcore
USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)
