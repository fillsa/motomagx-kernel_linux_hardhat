#
# Makefile for the linux kernel.
#

# Common support
obj-y := irq.o dma_sdma.o spba.o
obj-$(CONFIG_ARCH_MX3) += gpio.o cpu_common.o
obj-$(CONFIG_ARCH_MXC91131) +=  gpio.o cpu_common.o
obj-$(CONFIG_ARCH_MXC91231) +=  gpio.o cpu_common.o
obj-$(CONFIG_ARCH_MXC91321) +=  gpio.o cpu_common.o
obj-$(CONFIG_ARCH_MXC91331) +=  gpio.o cpu_common.o
obj-$(CONFIG_MOT_FEAT_GPIO_API) +=	mot-gpio.o
obj-m :=
obj-n :=
obj-  :=

obj-$(CONFIG_MXC_DSP_BRINGUP) += dsp_bringup.o
