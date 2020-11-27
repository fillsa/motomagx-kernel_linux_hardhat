#
# Makefile for the MC13783 PMIC core drivers.
#
CFLAGS_pmic_external.o += -I$(TOPDIR)/drivers/mxc
CFLAGS_mc13783_core.o += -I$(TOPDIR)/drivers/mxc

obj-$(CONFIG_MXC_MC13783_PMIC) += mc13783-core-mod.o
mc13783-core-mod-objs := pmic_external.o pmic_event.o pmic_core.o
