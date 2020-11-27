#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y			:= devices.o iomux.o system.o cpu.o mm.o time.o clock.o serial.o
obj-$(CONFIG_MACH_MXC91131EVB)	+= mxc91131evb.o mxc_pm.o

# Power management
obj-$(CONFIG_PM) 		+= pm.o
obj-$(CONFIG_DPM)       += dpm.o


