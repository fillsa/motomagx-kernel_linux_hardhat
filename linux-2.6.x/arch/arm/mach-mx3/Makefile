#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y			:= system.o iomux.o cpu.o mm.o time.o clock.o usb.o devices.o serial.o
obj-$(CONFIG_MACH_MX31ADS)	+= mx31ads.o mx31ads_gpio.o mxc_pm.o

# power management
obj-$(CONFIG_MXC_DVFS)  += dvfs.o
obj-$(CONFIG_PM) 		+= pm.o
obj-$(CONFIG_DPM)		+= dpm.o
