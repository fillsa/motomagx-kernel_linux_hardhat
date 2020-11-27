#
# Makefile for the kernel SSI device drivers.

#ifneq($(KERNELRELEASE))

obj-$(CONFIG_MXC_SSI) 		+= ssimod.o

ssimod-objs := ssi.o

#else

#default:
#	$(MAKE) -C /local/linux_tony/LINUX2.6/linux SUBDIRS=$(shell pwd) modules

#endif


