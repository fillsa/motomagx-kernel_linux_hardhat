# Makefile for camera driver for H2/H3

omapCamera-objs :=  camera_core.o omap16xxcam.o sensor_ov9640.o  

obj-$(CONFIG_VIDEO_OMAP_CAMERA) += omapCamera.o

EXTRA_CFLAGS = -I$(src)/..
