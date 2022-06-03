#
# Copyright 2005 Motorola, Inc.
#
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

obj-y   	:= mpm_func.o

#for Motorola PM driver
#ifneq ($(FEAT_PM),0)
ifeq ($(CONFIG_MOT_FEAT_PM),y)

ifeq ($(DEBUG), true)
	CFLAGS += -DMPM_DEBUG
endif

MODULE_NAME = mpmdrv
obj-m   := $(MODULE_NAME).o
$(MODULE_NAME)-objs := mpm.o mpm_sysfs.o

endif
