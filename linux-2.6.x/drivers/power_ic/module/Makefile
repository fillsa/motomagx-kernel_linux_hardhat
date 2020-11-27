# Copyright (C) 2004-2006  Motorola, Inc.
#
# Makefile for the power IC interface modules
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Motorola 2006-Nov-01 - Support LJ7.1 Reference Design
# Motorola 2006-Oct-10 - Update File
# Motorola 2006-Sep-07 - Addition of Zeus support
# Motorola 2006-Aug-16 - Add TCMD support for BUTE
# Motorola 2006-Aug-04 - Turn off transflash power when card not in use
# Motorola 2006-Jul-31 - One wire support for ArgonLV	
# Motorola 2006-Jul-06 - Implement USB detection light driver 
# Motorola 2006-Jun-19 - Fix montavista upmerge conditionals
# Motorola 2006-May-06 - Implement EMU Common Algorithm
# Motorola 2005-Dec-06 - Addition of Bute support
# Motorola 2005-Apr-04 - Addition of Atlas support
# Motorola 2005-Feb-28 - Addition of Charger.c
# Motorola 2004-Dec-17 - Initial Creation

ifeq ($(CONFIG_ARCH_MXC91231),y)
    obj-$(CONFIG_MOT_POWER_IC_ATLAS) := \
       accy.o \
       atod.o \
       audio.o \
       backup_mem.o \
       brt.o \
       charger.o \
       debounce.o \
       emu_glue_utils.o \
       usb_detection.o \
       lights_funlights.o \
       lights_funlights_atlas.o \
       lights_backlight.o \
       owire.o \
       peripheral.o \
       power_management.o \
       rtc.o \
       tcmd_ioctl.o
endif

ifeq ($(CONFIG_ARCH_MXC91321),y)
    obj-$(CONFIG_MOT_POWER_IC_ATLAS) := \
       accy.o \
       atod.o \
       audio.o \
       backup_mem.o \
       brt.o \
       charger.o \
       debounce.o \
       emu_glue_utils.o \
       usb_detection.o \
       owire.o \
       peripheral.o \
       power_management.o \
       rtc.o \
       tcmd_ioctl.o
endif

ifeq ($(CONFIG_ARCH_MXC91131),y)
    obj-$(CONFIG_MOT_POWER_IC) := \
       power_management.o
endif

fastdep:
