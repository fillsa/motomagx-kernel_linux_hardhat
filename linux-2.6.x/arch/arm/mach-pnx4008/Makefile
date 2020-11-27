#
# Makefile for the linux kernel.
#

obj-y			:= core.o irq.o time.o dma.o clock.o gpio.o serial.o rtc.o
obj-m			:=
obj-n			:=
obj-			:=

# Power Management
obj-$(CONFIG_PM) += pm.o sleep.o

# Dynamic power management
obj-$(CONFIG_DPM) += dpm.o

# Philips GPIO compatibility
obj-$(CONFIG_PNX4008_GPIO_COMPAT) += gpio-compat.o
