# Function driver for a CDC ACM OTG Device
# @(#) sl@belcarra.com|otg/functions/acm/Makefile-l26|20060403224909|46820
#
# Copyright (c) 2004 Belcarra Technologies Corp
# Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
#
# Copyright (c) 2006 Motorola, Inc. 
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 04/26/2006         Motorola         Initial distribution
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

tty_if-objs	:= acm-l26.o  acm.o tty-l26-os.o tty-if.o

obj-$(CONFIG_OTG_ACM) += tty_if.o

OTG=$(TOPDIR)/drivers/otg
ACMD=$(OTG)/functions/acm
USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR)
