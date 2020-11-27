# Copyright (C) 2005-2007 Motorola, Inc.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2 as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Motorola 2007-Jan-16 - Add support for MAX6946 LED controller chip.
# Motorola 2006-Nov-01 - Support LJ7.1 Reference Design
# Motorola 2006-Oct-06 - Update File
# Motorola 2006-Sep-26 - Add support for MAX7314 LED controller chip.
# Motorola 2006-Jun-19 - Fix montavista upmerge conditionals
# Motorola 2006-Apr-07 - Build gpio.o for all platforms
# Motorola 2006-Feb-10 - Add ArgonLV support
# Motorola 2005-Dec-06 - Minor modification to file names
# Motorola 2005-Mar-25 - Expanded power IC conditionals
# Motorola 2005-Feb-28 - File re-written from scratch

ifeq ($(CONFIG_ARCH_MXC91231),y)
    obj-$(CONFIG_MOT_POWER_IC) := \
        core.o \
        event.o \
        external.o  \
        fl_register.o \
        gpio.o \
        thread.o

    obj-$(CONFIG_MOT_POWER_IC_ATLAS) += \
        atlas_register.o
endif

ifeq ($(CONFIG_ARCH_MXC91321),y)
    obj-$(CONFIG_MOT_POWER_IC) := \
        core.o \
        event.o \
        external.o \
        fl_register.o \
        gpio.o \
        thread.o

    obj-$(CONFIG_MOT_POWER_IC_ATLAS) += \
        atlas_register.o
endif

fastdep:
