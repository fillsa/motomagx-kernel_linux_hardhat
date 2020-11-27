#
# Makefile for SCM-A11 implementation of Motorola GPIO API.
#
obj-y			:= iomux_init.o gpio_init.o base.o

obj-$(CONFIG_MACH_SCMA11REF)			+= gpio_scma11ref.o
obj-$(CONFIG_MACH_ASCENSION)			+= gpio_ascension.o
obj-$(CONFIG_MACH_LIDO)	        		+= gpio_lido.o
obj-$(CONFIG_MACH_SAIPAN)       		+= gpio_saipan.o
obj-$(CONFIG_MACH_ELBA)         		+= gpio_elba.o
obj-$(CONFIG_MACH_PICO)				+= gpio_pico.o
obj-$(CONFIG_MACH_MARCO)  			+= gpio_marco.o
obj-$(CONFIG_MACH_XPIXL)			    += gpio_xpixl.o
obj-$(CONFIG_MACH_NEVIS)  			+= gpio_nevis.o

obj-${CONFIG_MOT_FEAT_GPIO_API_BTPOWER} 	+= btpower.o
obj-${CONFIG_MOT_FEAT_GPIO_API_CSI}		+= csi.o
obj-${CONFIG_MOT_FEAT_GPIO_API_DAI}		+= dai.o
obj-${CONFIG_MOT_FEAT_GPIO_API_ETHERNET}	+= ethernet.o
obj-${CONFIG_MOT_FEAT_GPIO_API_ETM}		+= etm.o
obj-${CONFIG_MOT_FEAT_FLIP}			+= flip.o
obj-${CONFIG_MOT_FEAT_GPIO_API_LCD}		+= lcd.o
obj-${CONFIG_MOT_FEAT_GPIO_API_LIGHTING}	+= lighting.o
obj-${CONFIG_MOT_FEAT_GPIO_API_MC13783}		+= mc13783.o
obj-${CONFIG_MOT_FEAT_GPIO_API_SDHC}		+= sdhc.o
obj-${CONFIG_MOT_FEAT_GPIO_API_SERIALIZER}	+= serializer.o
obj-${CONFIG_MOT_FEAT_SLIDER}			+= slider.o
obj-${CONFIG_MOT_FEAT_GPIO_API_USBHS}		+= usb_hs.o
obj-${CONFIG_MOT_FEAT_GPIO_API_WLAN}		+= wlan.o
