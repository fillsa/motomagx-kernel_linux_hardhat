#
# Makefile for the kernel SPI device drivers.

obj-$(CONFIG_MXC_SPI) 		+= spimod.o

spimod-objs := spi-core.o registers.o

ifeq ($(CONFIG_MOT_FEAT_GPIO_API_MC13783),y)
spimod-objs += mc13783_spi_inter.o
endif

obj-$(CONFIG_MOT_FEAT_INVERTED_SPI_DISPLAY) += display_spi_inter.o
