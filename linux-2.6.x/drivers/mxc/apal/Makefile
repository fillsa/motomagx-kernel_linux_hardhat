# Copyright (C) 2005-2006  Motorola, Inc.
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  
# 02111-1307, USA
#

#
# Makefile for the kernel APAL (Audio Platform Abstraction Layer) driver.
#

ifneq ($(KBUILD_SRC),)
LINUX_OS	= $(KBUILD_SRC)
else
LINUX_OS	=
endif

LINUX_INCLUDE	= $(LINUX_OS)/include

ifeq ($(CONFIG_ARCH_MXC91231), y)
LINUX_ASM	= $(LINUX_INCLUDE)/asm-arm/arch-mxc
endif

ifeq ($(CONFIG_ARCH_MXC91331), y)
LINUX_ASM	= $(LINUX_INCLUDE)/asm-arm/arch-mxc
endif

ifeq ($(CONFIG_ARCH_MXC91321), y)
LINUX_ASM	= $(LINUX_INCLUDE)/asm-arm/arch-mxc
endif

OS_DRIVERS_PATH	= $(LINUX_OS)/drivers/mxc

APAL_KERNEL_INCLUDE	= -I$(LINUX_ASM) \
			  -I$(OS_DRIVERS_PATH)/atlas/module \
			  -I$(OS_DRIVERS_PATH)/atlas/core   \
			  -I$(OS_DRIVERS_PATH)/ssi  \
			  -I$(OS_DRIVERS_PATH)/dam \
			  -I$(OS_DRIVERS_PATH)/sdma \
			  -I$(LINUX_INCLUDE)

EXTRA_CFLAGS += $(APAL_KERNEL_INCLUDE)

#ifneq($(KERNELRELEASE))

obj-$(CONFIG_APAL) += apal_driver.o
