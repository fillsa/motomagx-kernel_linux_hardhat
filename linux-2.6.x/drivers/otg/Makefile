#
# Belcarra OTG - On-The-Go 
# @(#) balden@seth2.belcarratech.com|otg/.Makefiles/Makefile-mxc|20051116204957|04657
#
# Copyright (c) 2004-2005 Belcarra Technologies Corp
#
#                      Copyright (c) 2006 Motorola, Inc. 
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 08/09/2006         Motorola         Initial distribution
# 10/18/2006         Motorola         Add Open Src Software language
# 12/12/2006         Motorola         Changes for Open Src compliance.
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

obj-y 		+= 
obj-$(CONFIG_OTG) += otgcore/ 

#
# Belcarra OTG - On-The-Go 
#
# Copyright (c) 2004 Belcarra Technologies Corp

EXTRA_CFLAGS += -Wno-format -Wall
# Function Drivers
obj-$(CONFIG_OTG_GENERIC) += functions/generic/
obj-$(CONFIG_OTG_ACM) += functions/acm/
obj-$(CONFIG_OTG_MSC) += functions/msc/
# obj-$(CONFIG_OTG_MOUSE) += functions/mouse/
obj-$(CONFIG_OTG_NETWORK) += functions/network/
# obj-$(CONFIG_OTG_NETWORK) += functions/isotest/
obj-$(CONFIG_OTG_MTP) += functions/mtp/

ifeq ($(CONFIG_OTG),m)
    ifeq ($(CONFIG_MOT_FEAT_FX2LP_I2C), y)
    ifeq ($(CONFIG_OTG_HIGH_SPEED),y) 
    obj-m += ocd/fx2lp/
    endif
    endif
obj-m += ocd/mxc/
obj-m += ocd/isp1301/
obj-m += ocd/otglib/
obj-m += ocd/brassboard/
# obj-m += ocd/zasevb/
endif

ifeq ($(CONFIG_OTG),y)
obj-y += ocd/brassboard/
    ifeq ($(CONFIG_MOT_FEAT_FX2LP_I2C), y)
    ifeq ($(CONFIG_OTG_HIGH_SPEED),y) 
      obj-y += ocd/fx2lp/
    endif
    endif
# obj-y += ocd/zasevb/
endif

