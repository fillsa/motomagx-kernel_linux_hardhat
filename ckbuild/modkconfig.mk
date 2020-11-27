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

ifndef _ckbuild_modkconfig
_ckbuild_modkconfig = 1

include $(STD_DIRPATH)/ckbuild/ckbuild.mk
include $(STD_DIRPATH)/ckbuild/modkconfigvars.mk

# include the .config file when building modules
CKBUILD_EXTRA_CONFIG = $(MODKCONFIG_DOTCONFIG)

# define some phony targets for configuration
# NOTE : these targets cannot be built using clearmake,
# as they require user interaction
gconfig menuconfig oldconfig silentoldconfig xconfig:
	$(MODKCONFIG_INVOKE) $@

# define how the .config file is generated
$(MODKCONFIG_DOTCONFIG_REL):
	$(MODKCONFIG_INVOKE) defconfig $(MODKCONFIG_DEFCONFIG)

# generate the .config file before building modules
modules-prehook:: $(MODKCONFIG_DOTCONFIG_REL) 

# cleanup
clean::
	rm -f $(MODKCONFIG_DOTCONFIG) $(MODKCONFIG_AUTOCONF_H) $(MODKCONFIG_CONFIG_H)

# phony targets
.PHONY: gconfig menuconfig oldconfig silentoldconfig xconfig

endif
