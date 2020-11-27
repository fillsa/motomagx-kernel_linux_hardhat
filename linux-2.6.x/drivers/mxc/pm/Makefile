#
# Makefile for Power Managment devices.
#

# Module dependencies for the MXC driver
CFLAGS_dptc.o              = -I$(TOPDIR)/drivers/mxc/mc13783_legacy/module
CFLAGS_dvfs_dptc.o         = -I$(TOPDIR)/drivers/mxc/mc13783_legacy/module -I$(TOPDIR)/drivers/mxc/mc13783_legacy/core

obj-$(CONFIG_MXC_DPTC)		+= dptc.o

obj-$(CONFIG_MXC_DPTC)		+= dvfs_dptc.o

obj-$(CONFIG_MXC_DVFS)		+= dvfs_dptc.o


