#
# Makefile for the sc55112 PMIC core driver.
#

obj-$(CONFIG_MXC_PMIC_SC55112) += pmic-core-mod.o
pmic-core-mod-objs := sc55112_core.o sc55112_register.o sc55112_event.o pmic_external.o

ifeq ($(CONFIG_MXC_PMIC_SC55112_FIXARB),y)
    pmic-core-mod-objs += sc55112_fixarb.o
endif
