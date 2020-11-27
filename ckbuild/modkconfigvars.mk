#!/usr/bin/make -f
#
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Copyright 2006 Motorola
#
# Motorola  2006-Nov-13 - Initial revision.

ifndef _ckbuild_modkconfigvars
_ckbuild_modkconfigvars = 1

include $(STD_DIRPATH)/ckbuild/ckbuildvars.mk

# full path to the modkconfig.pl script
MODKCONFIG = $(STD_DIRPATH)/ckbuild/scripts/modkconfig.pl

# the modkconfig spec for the current module
MODKCONFIG_FILE ?= ModKconfig

# the defconfig for the current module
MODKCONFIG_DEFCONFIG ?= arch/$(ARCH)/defconfig

# how modkconfig.pl is invoked
MODKCONFIG_INVOKE = $(MODKCONFIG) -a$(ARCH) -k$(CKBUILD_KERNELSRC) -o$(STD_DIRPATH)/linux_build -f $(MODKCONFIG_FILE)

# the full path to the partial config header for the current module
MODKCONFIG_AUTOCONF_H := $(shell $(MODKCONFIG_INVOKE) path autoconf_header)

# the full path to the full config header for the current module
MODKCONFIG_CONFIG_H := $(shell $(MODKCONFIG_INVOKE) path config_header)

# the full path to the .config for the current module
MODKCONFIG_DOTCONFIG := $(shell $(MODKCONFIG_INVOKE) path config)

# relative path to the .config for the current module
MODKCONFIG_DOTCONFIG_REL = $(patsubst $(CURDIR)/%,%,$(MODKCONFIG_DOTCONFIG))

endif
