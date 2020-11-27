# Function driver for a Random Mouse
# @(#) balden@seth2.belcarratech.com|otg/functions/mouse/Makefile-l26|20051116204958|03990
#
# Copyright (c) 2005,2008 Motorola, Inc.
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 12/12/2005         Motorola         dn Initial distribution 
# 01/11/2008         Motorola         Update Copyright for OSS compliance 
# 01/28/2008         Motorola         Update Copyright date range for OSS compliance 
#
# Copyright (c) 2004 Belcarra
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

mouse_fd-objs	:= mouse-fd.o 
mouse_cf-objs	:= mouse-cf.o 
mouse_if-objs	:= mouse-if.o 

ifeq ($(CONFIG_OTG_MOUSE_TRADITIONAL),y)
obj-$(CONFIG_OTG_MOUSE) += mouse_fd.o 
endif
ifeq ($(CONFIG_OTG_MOUSE_INTERFACE), y)
obj-$(CONFIG_OTG_MOUSE) += mouse_if.o 
endif
ifeq ($(CONFIG_OTG_MOUSE_COMPOSITE), y)
obj-$(CONFIG_OTG_MOUSE) += mouse_cf.o
endif

OTG=$(TOPDIR)/drivers/otg
MOUSED=$(OTG)/functions/mouse
OTGCORE_DIR=$(OTG)/otgcore
#USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(MOUSED) -I$(OTG) -Wno-unused -Wno-format  -I$(OTGCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(MOUSED) -I$(OTG) -Wno-unused -Wno-format  -I$(OTGCORE_DIR)
