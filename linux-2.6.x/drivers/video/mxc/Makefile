ifeq ($(CONFIG_MACH_NEVIS),y)
obj-$(CONFIG_FB_MXC_SANYO_QVGA_PANEL)   += mxcfb_gd2.o gd2_func.o
obj-$(CONFIG_FB_MXC_SHARP_QVGA_PANEL)   += mxcfb_gd2.o gd2_func.o
obj-$(CONFIG_FB_MXC_VGA_PANEL)          += mxcfb_gd2.o gd2_func.o
else
obj-$(CONFIG_FB_MXC_SANYO_QVGA_PANEL)   += mxcfb.o
obj-$(CONFIG_FB_MXC_SHARP_QVGA_PANEL)   += mxcfb.o
obj-$(CONFIG_FB_MXC_VGA_PANEL)          += mxcfb.o
endif
obj-$(CONFIG_FB_MXC_EPSON_PANEL)        += mxcfb_epson.o
obj-$(CONFIG_FB_MXC_EPSON_QVGA_PANEL)   += mxcfb_epson_qvga.o
obj-$(CONFIG_FB_MXC_TOSHIBA_QVGA_PANEL) += mxcfb_toshiba_qvga.o
obj-$(CONFIG_FB_MXC_SHARP_128_PANEL)    += mxcfb_sharp_128x128.o
ifeq ($(CONFIG_MACH_NEVIS),y)
obj-$(CONFIG_FB_MXC_TVOUT)              += mxcfb_gd2.o gd2_func.o fs453.o
else
obj-$(CONFIG_FB_MXC_TVOUT)              += mxcfb.o fs453.o
endif