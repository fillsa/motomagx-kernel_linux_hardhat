#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y					:= devices.o iomux.o system.o cpu.o mm.o time.o clock.o serial.o
obj-$(CONFIG_MACH_I30030EVB)		+= i30030evb.o i30030evb_gpio.o
obj-$(CONFIG_MACH_I30030ADS)	    += i30030ads.o i30030ads_gpio.o
obj-$(CONFIG_MACH_MXC30030EVB)		+= mxc30030evb.o mxc30030evb_gpio.o
obj-$(CONFIG_MACH_MXC30030ADS)		+= mxc30030ads.o mxc30030ads_gpio.o
obj-$(CONFIG_MACH_ARGONLVPHONE)		+= argonlvphone.o

obj-$(CONFIG_MOT_FEAT_GPIO_API)		+= mot-gpio/

obj-$(CONFIG_ARCH_MXC91331)		+= mxc_pm_mxc91331.o
obj-$(CONFIG_ARCH_MXC91321)		+= mxc_pm.o
obj-$(CONFIG_ARCH_MXC91321)		+= pm.o

# Dynamic power management
obj-$(CONFIG_DPM)               += dpm.o
obj-$(CONFIG_MOT_FEAT_DEBUG_WDOG) += wdog2_fiq.o
