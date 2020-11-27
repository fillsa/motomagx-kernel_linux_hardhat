#
# Makefile for ArgonLV implementation of Motorola GPIO API.
#
obj-y				:= iomux_init.o gpio_init.o base.o

ifdef CONFIG_MACH_BUTEREF
# buteref defines both CONFIG_MACH_BUTEREF and CONFIG_MACH_ARGONLVREF
obj-$(CONFIG_MACH_ARGONLVREF)	+= gpio_buteref.o iomux_buteref.o
else
obj-$(CONFIG_MACH_ARGONLVREF)	+= gpio_argonlvref.o iomux_argonlvref.o
endif

obj-$(CONFIG_MACH_KASSOS)	+= gpio_argonlvref.o iomux_argonlvref.o
obj-$(CONFIG_MACH_KEYWEST)	+= gpio_argonlvref.o iomux_argonlvref.o
obj-$(CONFIG_MACH_PAROS)	+= gpio_argonlvref.o iomux_argonlvref.o

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
obj-${CONFIG_MOT_FEAT_GPIO_API_USBHS}		+= usb_hs.o
obj-${CONFIG_MOT_FEAT_GPIO_API_WLAN}		+= wlan.o
