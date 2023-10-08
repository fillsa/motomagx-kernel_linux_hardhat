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
# Copyright (C) 2006-2007 Motorola, Inc.
#
# Motorola  2006-Nov-13 - Initial revision.
# Motorola  2007-Jan-25 - Both clearmake and cleargmake are now acceptable
#                         values for $(MAKE).
# Motorola  2007-Aug-27 - Fixed miniplatform build error.

ifndef _ckbuild_ckbuildvars
_ckbuild_ckbuildvars = 1

# extra flags
CKBUILD_EXTRA_CFLAGS =

# extra config
CKBUILD_EXTRA_CONFIG =

# kernel source directory
CKBUILD_KERNELSRC := $(shell grep ^KERNELSRC $(STD_DIRPATH)/linux_build/Makefile | sed -e 's/.*:= *\(.*\)/\1/')

# normal arguments
CKBUILD_NORMAL_ARGS = ARCH=$(ARCH) CROSS_COMPILE=$(patsubst %-gcc,%-,$(CC)) M=$(PWD) KBUILD_SRC=$(CKBUILD_KERNELSRC) MODLIB=$(KMODULES_DIR) CKBUILD_EXTRA_CFLAGS="$(CKBUILD_EXTRA_CFLAGS)" CKBUILD_EXTRA_CONFIG=$(CKBUILD_EXTRA_CONFIG)

# arguments which depend on the MAKE tool
#
# if we are using clearmake or cleargmake, we use a modified set of
# makefiles (ckbuild), otherwise we use the normal makefiles (kbuild)
ifeq ($(filter $(notdir $(MAKE)), clearmake cleargmake),)
CKBUILD_SWITCH_ARGS = -f $(CKBUILD_KERNELSRC)/Makefile
else
CKBUILD_SWITCH_ARGS = -f $(STD_DIRPATH)/ckbuild/Makefile CKBUILD_SRC=$(STD_DIRPATH)/ckbuild
endif

# extra arguments
CKBUILD_EXTRA_ARGS =

# how the toplevel makefile is invoked
#
# we change into the kernel build directory and invoke either the ckbuild
# or kbuild toplevel makefile, setting KBUILD_SRC.
#
# if we did not specify the makefile with -f, this is what would happen:
#
# 1. the build directory's toplevel makefile would change to the
#    source directory and invoke :
#    $(MAKE) O=output_dir
#
# 2. the source directory's toplevel makefile would change back
#    to the build directory and invoke :
#    $(MAKE) -f source_dir/Makefile KBUILD_SRC=source_dir
#
# we are if effect skipping two steps and retaining control over which
# toplevel makefile is used.
CKBUILD_INVOKE = cd $(STD_DIRPATH)/linux_build && $(MAKE) $(CKBUILD_SWITCH_ARGS) $(CKBUILD_NORMAL_ARGS) $(CKBUILD_EXTRA_ARGS)

endif
