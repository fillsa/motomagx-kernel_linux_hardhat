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

ifndef _ckbuild_ckbuild
_ckbuild_ckbuild = 1

include $(STD_DIRPATH)/ckbuild/ckbuildvars.mk

modules: modules-prehook kbuild/modules modules-posthook

# this is a customisable hook for operations that need
# to be performed before ckbuild is invoked
modules-prehook::

# this is a customisable hook for operations that need
# to be performed after ckbuild is invoked
modules-posthook::

modules_install: kbuild/modules_install

# cleanup
clean:: 
	find .	\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
		-type f -print | xargs rm -f
	find .	\( -name .tmp_versions \) \
		-type d -print | xargs rm -rf

# this rule is used to pass the build to ckbuild's top-level makefile
kbuild/%:
	$(CKBUILD_INVOKE) $*

endif
