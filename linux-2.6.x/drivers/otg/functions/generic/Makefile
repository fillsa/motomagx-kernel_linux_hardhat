# Function driver for the Generic configuration 
#
# @(#) sl@whiskey.enposte.net|otg/functions/generic/Makefile|20051116230343|44880
# Copyright (c) 2004-2005 Belcarra
#
# Copyright (c) 2005-2006 Motorola, Inc. 
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 12/12/2005         Motorola         Initial distribution
# 04/20/2006         Motorola         Corrected some typo's and got rid of 
#                                       belcarra-cf references 
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

#belcarra_cf-objs	:= generic.o belcarra-cf.o
generic_cf-objs	:= generic.o generic-cf.o
generic_cl-objs	:= generic-cl.o

obj-$(CONFIG_OTG_GENERIC) += generic_cf.o
#obj-$(CONFIG_OTG_GENERIC) += belcarra_cf.o
#obj-$(CONFIG_OTG_GENERIC) += generic_cl.o

OTG=$(TOPDIR)/drivers/otg
ACMD=$(OTG)/functions/mouse
OTGCORE_DIR=$(OTG)/otgcore
USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)
