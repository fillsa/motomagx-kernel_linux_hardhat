#
# Makefile for the kernel Digital Audio MUX (DAM) device driver.

#ifneq($(KERNELRELEASE))

obj-$(CONFIG_MXC_DAM) 		+= dammod.o

dammod-objs := dam.o

#else

#default:
#	$(MAKE) -C /local/linux_tony/LINUX2.6/linux SUBDIRS=$(shell pwd) modules

#endif


