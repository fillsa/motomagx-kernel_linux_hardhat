#
# Copyright (c) 2005 Belcarra
# @(#) balden@belcarra.com|otg/ocd/otglib/Makefile-l26|20051117000523|36225
#
# Copyright 2005-2006 Motorola, Inc.
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 12/12/2005         Motorola         Initial distribution
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
#


#obj-y  := empty.o

OTG=$(TOPDIR)/drivers/otg
USBCORE=$(TOPDIR)/drivers/usb
OTGCORE_DIR=$(OTG)/otgcore
USBDCORE_DIR=$(TOPDIR)/drivers/usb
EXTRA_CFLAGS += -I$(OTG) -Wno-unused -Wno-format  -I$(USBCORE) -I$(USBDCORE_DIR) -I$(OTGCORE_DIR) -I$(MXCATLASDIR)
EXTRA_CFLAGS_nostdinc += -I$(OTG) -Wno-unused -Wno-format  -I$(USBCORE) -I$(USBDCORE_DIR) -I$(OTGCORE_DIR) -I$(MXCATLASDIR)
