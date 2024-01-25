# ###########################################################################
# Makefile - generation of hardhat (kernel) component
# Copyright (C) 2006-2008 Motorola, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Changelog: 
#
# Date         Author          Comment
# ===========  ==============  ==============================================
# 09-Nov-2006  Motorola        Initial revision.
# 20-Nov-2006  Motorola        Added HW_RF_PYTHON
# 21-Nov-2006  Motorola        HW_MM_ACCEL_IPU check added
# 06-Dec-2006  Motorola        Added FEAT_ETM check.
# 15-Dec-2006  Motorola        Added memory dump support (DBG_MEMDUMP)
# 25-Dec-2006  Motorola        Export WLAN driver header file to application
# 07-Feb-2007  Motorola        Remove MMC/SD support
# 12-Jan-2007  Motorola        HW_PWR_KEY_LOCK and HW_OMEGA checks added
# 19-Jan-2007  Motorola        Added FEAT_VIRTUAL_KEYMAP
# 26-Jan-2007  Motorola        Added FM_RADIO check.
# 29-Jan-2007  Motorola        exported linux/mmc/external.h
# 09-Jan-2007  Motorola        Removed an incorrect export
# 12-Jan-2007  Motorola        HW_PWR_KEY_LOCK and HW_OMEGA checks added
# 19-Jan-2007  Motorola        Added FEAT_VIRTUAL_KEYMAP
# 23-Jan-2007  Motorola        Fixed kernel rebuild issue
# 26-Jan-2007  Motorola        Added FM_RADIO check.
# 29-Jan-2007  Motorola        exported linux/mmc/external.h
# 01-Feb-2007  Motorola        Added TEST_ALLOW_MODULES.
# 07-Feb-2007  Motorola        Export light_sensor.h.
# 07-Feb-2007  Motorola        Add HW_MAX6946 check
# 20-Feb-2007  Motorola        Added HW_USB_HS_ANTIOCH
# 02-Apr-2007  Motorola        Add TEST_COVERAGE.   
# 06-Apr-2007  Motorola        removed PowerIC exports
# 06-Apr-2007  Motorola	       Export mpm.h
# 11-Apr-2007  Motorola        TOOLPREFIX dump functionality
# 11-Apr-2007  Motorola        Export mtd/jffs-user.h & linux/jffs2.h
# 19-Apr-2007  Motorola        Updated ckbuild export for x86
# 25-Apr-2007  Motorola        Removed sierra kconfig hack
# 04-May-2007  Motorola        Export mxc_ipc.h
# 12-May-2007  Motorola	       Removed keypad.h
# 29-May-2007  Motorola        Removed sdhc_user.h and usr_blk_dev.h
# 05-Jun-2007  Motorola        Add FEAT_AV support
# 07-Jun-2007  Motorola        Added motfb.h and mxcfb.h
# 28-Jun-2007  Motorola        Removed some SiERRA variables
# 03-Aug-2007  Motorola	       Enable ptrace
# 04-Sep-2007  Motorola        Export ssi and dam header files
# 24-Oct-2007  Motorola        Add support for FEAT_MTD_SHA 
# 14-Nov-2007  Motorola        Export mpm.h for the previous incorrect merge.
# 23-Nov-2007  Motorola        Add BT LED debug option processing
# 04-Dec-2007  Motorola        Remove WLAN driver files.
# 15-Feb-2008  Motorola        Make if(n)eq compliant for both make and clearmake
# 30-Jun-2008  Motorola        Export inotify.h
# 29-Jan-2008  Motorola        Added header file morphing_mode.h to API_INCS
# 23-Apr-2008  Motorola        Add FEAT_EPSON_LTPS_DISPLAY check
# 04-Jun-2008  Motorola        Export dm500.h header file
# 27-Jun-2008  Motorola	       Export inotify.h on LJ7.4
# 30-Jue-2008  Motorola        Export inotify.h. 		
# 29-Jul-2008  Motorola        Add FEAT_32_BIT_DISPLAY check
# 01-Aug-2008  Motorola        Fix Wifi/Marvell Drivers compliance issues
# 10-Oct-2008  Motorola        Export inotify.h
# ###########################################################################

include $(BOOTSTRAP)


# ###########################################################################
# Variable Setup
# ###########################################################################
MARVELL_8686_SRC_DIR = $(COMPTOP)/linux-2.6.x/drivers/net/wireless/Marvell/8686/src
MARVELL_8686_WLANCONFIG_DIR = $(MARVELL_8686_SRC_DIR)/app/wlanconfig

KERNEL_VERSION = 2.6
LINUXROOT = $(COMPTOP)/linux-$(KERNEL_VERSION).x

PRODUCT_DIR := $(BUILDTOP)/$(ARCH)/$(PROC_TYPE)/$(PRODUCT)
PRODUCT_ROOTFS := $(PRODUCT_DIR)/fs

LINUXBUILD = $(PRODUCT_DIR)/linux_build

LINUXBUILD_KERNEL = $(PRODUCT_DIR)/kernel/linux_build

API_INCS = $(COMPTOP)/linux-2.6.x/include//linux/moto_accy.h \
	$(COMPTOP)/linux-2.6.x/include//linux/camera.h \
	$(COMPTOP)/linux-2.6.x/include//linux/videodev.h \
	$(COMPTOP)/linux-2.6.x/include//linux/lights_funlights.h \
	$(COMPTOP)/linux-2.6.x/include//linux/light_sensor.h \
	$(COMPTOP)/linux-2.6.x/include//linux/power_ic.h \
	$(COMPTOP)/linux-2.6.x/include//linux/lights_backlight.h \
	$(COMPTOP)/linux-2.6.x/include//linux/keypad.h \
	$(COMPTOP)/linux-2.6.x/include//linux/keyv.h \
	$(COMPTOP)/linux-2.6.x/include//linux/mmc/external.h \
	$(COMPTOP)/linux-2.6.x/include//linux/sahara/sahara.h \
	$(COMPTOP)/linux-2.6.x/include//linux/sahara/fsl_shw.h \
	$(COMPTOP)/linux-2.6.x/include//linux/sahara/fsl_platform.h \
	$(COMPTOP)/linux-2.6.x/include//linux/sahara/sah_kernel.h \
	$(COMPTOP)/linux-2.6.x/include//linux/sahara/sf_util.h \
	$(COMPTOP)/linux-2.6.x/include//linux/sahara/adaptor.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/apal/apal_driver.h \
        $(COMPTOP)/linux-2.6.x/include/asm-arm/arch-mxc//linux/mxc_mu.h \
        $(COMPTOP)/linux-2.6.x/include/asm-arm/arch-mxc//linux/mxc_ipc.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/ipu/ipu.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/ipu/ipu_regs.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/ipu/ipu_prv.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/ipu/ipu_alloc.h \
	$(COMPTOP)/linux-2.6.x/include//asm-arm/setup.h \
	$(COMPTOP)/linux-2.6.x/drivers/net/wireless/Marvell/8686/src/wlan/hostcmd.h \
	$(COMPTOP)/linux-2.6.x/drivers/net/wireless/Marvell/8686/src/wlan/wlan_types.h \
	$(COMPTOP)/linux-2.6.x/drivers/net/wireless/Marvell/8686/src/wlan/wlan_wext.h \
	$(COMPTOP)/linux-2.6.x/drivers/media/video/mxc/capture/capture_2mp_wrkarnd.h \
	$(COMPTOP)/linux-2.6.x/include//linux/spi_display.h \
	$(COMPTOP)/linux-2.6.x/include//mtd/jffs2-user.h \
	$(COMPTOP)/linux-2.6.x/include//linux/jffs2.h \
	$(COMPTOP)/linux-2.6.x/include//linux/mpm.h    \
	$(COMPTOP)/linux-2.6.x/include//linux/motfb.h \
	$(COMPTOP)/linux-2.6.x/include//linux/mxcfb.h \
	$(COMPTOP)/linux-2.6.x/include//linux/morphing_mode.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/ssi/ssi.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/ssi/ssi_types.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/dam/dam.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/dam/dam_types.h \
	$(COMPTOP)/linux-2.6.x/drivers/mxc/spi/dm500.h \
	$(COMPTOP)/linux-2.6.x/include//linux/inotify.h

# don't build kernel for x86
ifneq ($(HW_ARCH),i686)

API_DIRS = $(LINUXBUILD) $(COMPTOP)/kernel_include $(COMPTOP)/ckbuild

DOTCONFIG = $(LINUXBUILD)/.config

# Parse the full path and prefix for the cross compiler prefix
TOOLPREFIX = $(patsubst %-gcc,%-,$(CC))
TOOLPREFIX_DUMP = $(LINUXBUILD)/toolprefix.mk

# ###########################################################################
# DYNAMIC GENERATION OF KERNEL CONFIGURATION FILE (defconfig)
#
# For more information, see:
#   /vobs/jem/hardhat/linux-2.6.x/arch/arm/configs/motorola_ljap/README.txt
# ###########################################################################

TARGET_DEFCONFIG    := ${PRODUCT_DIR}/${PRODUCT}_defconfig
DEFCONFIGSRC        := ${LINUXROOT}/arch/${ARCH}/configs
LJAPDEFCONFIGSRC    := ${DEFCONFIGSRC}/motorola_ljap

# build list of files to be concatenated together to make the linux defconfig
PRODUCT_SPECIFIC_DEFCONFIGS := \
    ${DEFCONFIGSRC}/motorola_ljap_defconfig \
    ${LJAPDEFCONFIGSRC}/product-family/${PRODUCT_FAMILY}.config \
    ${LJAPDEFCONFIGSRC}/product/${PRODUCT}.config \
    ${DEFCONFIGSRC}/${PRODUCT}_defconfig

# If HW_MM_ACCEL_IPU is not set, we need GPU support.
ifeq ($(HW_MM_ACCEL_IPU),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/display_gpu.config \
        ${LJAPDEFCONFIGSRC}/feature/display_gpu.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/display_gpu.${PRODUCT}_config 
endif

# set proper form factor
PRODUCT_SPECIFIC_DEFCONFIGS += \
    ${LJAPDEFCONFIGSRC}/feature/formfactor_${FEAT_FORM_FACTOR}.config \
    ${LJAPDEFCONFIGSRC}/feature/formfactor_${FEAT_FORM_FACTOR}.${PRODUCT_FAMILY}_config \
    ${LJAPDEFCONFIGSRC}/feature/formfactor_${FEAT_FORM_FACTOR}.${PRODUCT}_config

# Build the small page NAND YAFFS module or the Large Page NAND YAFFS module.
PRODUCT_SPECIFIC_DEFCONFIGS += \
  ${LJAPDEFCONFIGSRC}/feature/nand_${MEM_MAP_PAGE_SIZE}.config

# Support STM 90nm 256MB NAND
ifneq ($(HW_NAND_STM90NM),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/nand_stm90nm.config \
        ${LJAPDEFCONFIGSRC}/feature/nand_stm90nm.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/nand_stm90nm.${PRODUCT}_config 
endif 

# High Speed USB using FX2LP
ifneq ($(HW_USB_HS_FX2LP),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/usb_hs_fx2lp.config \
        ${LJAPDEFCONFIGSRC}/feature/usb_hs_fx2lp.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/usb_hs_fx2lp.${PRODUCT}_config 
endif 

# High Speed USB using Antioch
ifneq ($(HW_USB_HS_ANTIOCH),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/usb_hs_antioch.config \
        ${LJAPDEFCONFIGSRC}/feature/usb_hs_antioch.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/usb_hs_antioch.${PRODUCT}_config 
endif 

# Multi-Media Card support
ifneq ($(HW_MMC),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/mmc_sdhc.config \
        ${LJAPDEFCONFIGSRC}/feature/mmc_sdhc.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/mmc_sdhc.${PRODUCT}_config 
endif 

# Internal SD Card Slot
ifneq ($(HW_MMC_INTERNAL),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/mmc_internal.config \
        ${LJAPDEFCONFIGSRC}/feature/mmc_internal.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/mmc_internal.${PRODUCT}_config 
endif

# External SD Card Slot
ifneq ($(HW_MMC_EXTERNAL),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/mmc_external.config \
        ${LJAPDEFCONFIGSRC}/feature/mmc_external.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/mmc_external.${PRODUCT}_config 
endif

# Megasim SD Card
ifneq ($(HW_MMC_MEGASIM),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/mmc_megasim.config \
        ${LJAPDEFCONFIGSRC}/feature/mmc_megasim.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/mmc_megasim.${PRODUCT}_config 
endif

# CS89x0 Ethernet Support
ifneq ($(HW_ETHERNET),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/ethernet.config \
        ${LJAPDEFCONFIGSRC}/feature/ethernet.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/ethernet.${PRODUCT}_config 
endif 

# compile USBOTG drivers statically to support NFS-over-USB
# appears after HW_ETHERNET because this disables CONFIG_NET_ETHERNET
ifeq ($(FEAT_USBOTG),static)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/usbotgstatic.config \
	${LJAPDEFCONFIGSRC}/feature/usbotgstatic.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/usbotgstatic.${PRODUCT}_config
endif

ifeq ($(FEAT_CONSOLE),usb)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/usbconsole.config \
        ${LJAPDEFCONFIGSRC}/feature/usbconsole.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/usbconsole.${PRODUCT}_config
endif

# allow MXC internal UART3 to be used as a serial console
ifeq ($(FEAT_CONSOLE),serial)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/serialconsole.config \
	${LJAPDEFCONFIGSRC}/feature/serialconsole.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/serialconsole.${PRODUCT}_config
endif

ifeq ($(DBG_LTT),1) 
        PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/ltt.config 
endif 

ifeq ($(TEST_I2CDEV),1)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/i2cdev.config \
	${LJAPDEFCONFIGSRC}/feature/i2cdev.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/i2cdev.${PRODUCT}_config
endif

ifeq ($(DBG_TURBO_INDICATOR),1)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/turbo.config \
        ${LJAPDEFCONFIGSRC}/feature/turbo.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/turbo.${PRODUCT}_config
endif

ifeq ($(DBG_DSM_INDICATOR),1)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/dsm.config \
        ${LJAPDEFCONFIGSRC}/feature/dsm.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/dsm.${PRODUCT}_config
endif

ifeq ($(DBG_PM_LED),1)
	PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/pm_led.onyx_config
endif

ifeq ($(TEST_ALLOW_MODULES),1)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/allowmodules.config
endif
 
ifeq ($(DBG_OPROFILE),1) 
        PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/oprofile.config 
endif 
 
ifeq ($(DBG_MEMDUMP),1)
	PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/memdump.config
endif


# SiERRA: Enable support for Python RF module
ifneq ($(HW_RF_PYTHON),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/rf_python.config \
	${LJAPDEFCONFIGSRC}/feature/rf_python.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/rf_python.${PRODUCT}_config
endif
 
# SiERRA: Linear Vibrator
ifneq ($(HW_LINEARVIBRATOR),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/linearvibrator.config \
        ${LJAPDEFCONFIGSRC}/feature/linearvibrator.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/linearvibrator.${PRODUCT}_config
endif

# SiERRA: Capacitive Touch
ifneq ($(HW_CAPTOUCH),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/captouch.config \
        ${LJAPDEFCONFIGSRC}/feature/captouch.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/captouch.${PRODUCT}_config
endif

# SiERRA: Ceramic Speaker
ifneq ($(HW_CERAMICSPEAKER),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/ceramicspeaker.config \
        ${LJAPDEFCONFIGSRC}/feature/ceramicspeaker.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/ceramicspeaker.${PRODUCT}_config
endif

# OS: ptrace 
ifeq ($(FEAT_PTRACE),1)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/ptrace.config \
	${LJAPDEFCONFIGSRC}/feature/ptrace.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/ptrace.${PRODUCT}_config
endif

# Security: antivirus
ifneq ($(FEAT_AV),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/av.config \
	${LJAPDEFCONFIGSRC}/feature/av.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/av.${PRODUCT}_config
 endif

# SiERRA: Proximity Sensor
ifneq ($(HW_PROXSENSOR),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/proxsensor.config \
	${LJAPDEFCONFIGSRC}/feature/proxsensor.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/proxsensor.${PRODUCT}_config
endif

# SiERRA: MAX6946 support
ifneq ($(HW_MAX6946),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/max6946.config \
        ${LJAPDEFCONFIGSRC}/feature/max6946.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/max6946.${PRODUCT}_config 
endif

# SiERRA: MAX7314 support
ifneq ($(HW_MAX7314),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/max7314.config \
        ${LJAPDEFCONFIGSRC}/feature/max7314.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/max7314.${PRODUCT}_config 
endif

# SiERRA: Omega Wheel
ifneq ($(HW_OMEGA),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/omega.config \
        ${LJAPDEFCONFIGSRC}/feature/omega.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/omega.${PRODUCT}_config 
endif

# SiERRA: Power/Keylock
ifneq ($(HW_PWR_KEY_LOCK),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/pwr_key_lock.config \
        ${LJAPDEFCONFIGSRC}/feature/pwr_key_lock.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/pwr_key_lock.${PRODUCT}_config 
endif


# SiERRA: FM radio
ifneq ($(HW_FM_RADIO),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/fm_radio.config \
        ${LJAPDEFCONFIGSRC}/feature/fm_radio.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/fm_radio.${PRODUCT}_config 
endif

# MME: Landscape Main Display
ifneq ($(HW_LCD_LANDSCAPE_MODE),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/landscape.config \
        ${LJAPDEFCONFIGSRC}/feature/landscape.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/landscape.${PRODUCT}_config 
endif 

# PM: Desense Support
ifneq ($(HW_PM_DESENSE),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/desense.config \
        ${LJAPDEFCONFIGSRC}/feature/desense.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/desense.${PRODUCT}_config 
endif 

# Marvell WiFi Support
ifneq ($(FEAT_WIFI_DRIVERS),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/wifi_drivers.config \
        ${LJAPDEFCONFIGSRC}/feature/wifi_drivers.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/wifi_drivers.${PRODUCT}_config 
endif 

# HVGA Display Support
ifneq ($(HW_HVGA_DISPLAY),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/hvga_display.config \
        ${LJAPDEFCONFIGSRC}/feature/hvga_display.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/hvga_display.${PRODUCT}_config 
endif 

# 'Common' Camera Support
ifneq ($(FEAT_COMMON_CAMERA),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/common_camera.config \
        ${LJAPDEFCONFIGSRC}/feature/common_camera.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/common_camera.${PRODUCT}_config 
endif 

# Detect transflash card insertion and removal via external interrupt
ifneq ($(HW_SDHC_1_TF_DET),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/tf_det.config \
        ${LJAPDEFCONFIGSRC}/feature/tf_det.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/tf_det.${PRODUCT}_config 
endif 

# Enable RAW I2C api for camera common driver
ifeq ($(FEAT_CAMERA_MISC_DRIVER),1)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
        ${LJAPDEFCONFIGSRC}/feature/raw_i2c_api.config \
        ${LJAPDEFCONFIGSRC}/feature/raw_i2c_api.${PRODUCT_FAMILY}_config \
        ${LJAPDEFCONFIGSRC}/feature/raw_i2c_api.${PRODUCT}_config 
endif 

# Enable ETM IOMUX configuration
ifneq ($(FEAT_ETM),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/etm.config \
	${LJAPDEFCONFIGSRC}/feature/etm.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/etm.${PRODUCT}_config
endif

# Enable Sierra Virtual Keymap driver
ifneq ($(FEAT_VIRTUAL_KEYMAP),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/virtualkeymap.config \
	${LJAPDEFCONFIGSRC}/feature/virtualkeymap.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/virtualkeymap.${PRODUCT}_config
endif

# Enable epson display driver
ifneq ($(FEAT_EPSON_LTPS_DISPLAY),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
    ${LJAPDEFCONFIGSRC}/feature/display_epson.config
endif
                
# Enable 32bit Display
ifneq ($(FEAT_32_BIT_DISPLAY),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
    ${LJAPDEFCONFIGSRC}/feature/display_32bit.config
endif

# Enable MTD SHA functionality
ifneq ($(FEAT_MTD_SHA),0)
    PRODUCT_SPECIFIC_DEFCONFIGS += \
	${LJAPDEFCONFIGSRC}/feature/mtd_sha.config \
	${LJAPDEFCONFIGSRC}/feature/mtd_sha.${PRODUCT_FAMILY}_config \
	${LJAPDEFCONFIGSRC}/feature/mtd_sha.${PRODUCT}_config
endif

# filter out any non-existent files
PRODUCT_SPECIFIC_DEFCONFIGS := \
    $(strip $(foreach FILE, $(PRODUCT_SPECIFIC_DEFCONFIGS), \
	$(wildcard $(FILE))))

# Allow user to specify arbitrary files to be concatenated from the command
# line. Do this after filtering for non-existent files so that an error
# will be returned if a specified file doesn't exist.
ifneq ($(TEST_LNXOS_DEFCONFIGS),)
    PRODUCT_SPECIFIC_DEFCONFIGS += ${strip ${TEST_LNXOS_DEFCONFIGS}}
endif

# Option to enable or disable gcov
ifeq ($(TEST_COVERAGE),1)
	PRODUCT_SPECIFIC_DEFCONFIGS += ${LJAPDEFCONFIGSRC}/feature/coverage.config
endif

# generate a defconfig file
${TARGET_DEFCONFIG}: ${PRODUCT_SPECIFIC_DEFCONFIGS}
	# generate an error message if no config files are defined
ifeq ($(PRODUCT_SPECIFIC_DEFCONFIGS),)
	@echo "No defconfig file defined for PRODUCT=$(PRODUCT))"
	false
endif
	mkdir -p $(PRODUCT_DIR)
	@( perl -le 'print "# This file was automatically generated from:\n#\t" . join("\n#\t", @ARGV) . "\n"' $(PRODUCT_SPECIFIC_DEFCONFIGS) && cat $(PRODUCT_SPECIFIC_DEFCONFIGS) ) > $@ || ( rm -f $@ && false )


# ###########################################################################
# Kernel Build Targets
# ###########################################################################

.PHONY: distclean kernel modules kernel_clean

# don't build anything in parallel, needed for kernel_clean and $(DOTCONFIG)
.NOTPARALLEL: 

# kernel clean must come first, but cannot be a dependency
api_build: kernel_clean sierraOmegaWheelKconfig $(DOTCONFIG)

impl: modules

# avoid partial winkins of .config when kernel is not properly cleaned
ifneq (_$(NO_KERNEL_CLEAN),_yes)
.NO_CONFIG_REC: kernel modules
else
.NO_CONFIG_REC: kernel modules $(DOTCONFIG)
endif

$(DOTCONFIG): $(TARGET_DEFCONFIG) 
ifneq (_$(NO_KERNEL_CLEAN),_yes)
	# clean out config area to prevent partial builds from winking in
	rm -rf $(LINUXBUILD)
endif
	mkdir -p $(LINUXBUILD)
	cd $(LINUXROOT) && env -u MAKECMDGOALS make MAKE=make ARCH=$(ARCH) CROSS_COMPILE=$(TOOLPREFIX) O=$(@D) MOT_KBUILD_DEFCONFIG=$(TARGET_DEFCONFIG) defconfig modules_prepare
	# Build Environment sends the regular, non-metric-collecting compiler,
	# so we can skim off the the toolprefix for kernel and module builds.
	# For other components building modules, we need to save off this value
	# and bring it into the generated Makefile
	@echo "TOOLPREFIX=$(TOOLPREFIX)" > $(TOOLPREFIX_DUMP)

kernel: $(DOTCONFIG)
	# Kernel must make a copy of the config area to prevent the api step from rebuilding unnecessarily
ifneq (_$(NO_KERNEL_CLEAN),_yes)
	# Clean out kernel build area for safety
	rm -rf $(LINUXBUILD_KERNEL)
endif
	mkdir -p $(LINUXBUILD_KERNEL)
	cp -pR  $(LINUXBUILD) $(dir $(LINUXBUILD_KERNEL)) 
	# JOBS tells gnumake to use the j option.
	cd $(LINUXBUILD_KERNEL) && $(MAKE) JOBS=-j4 ARCH=$(ARCH) zImage

modules: kernel wlanconfig
	mkdir -p $(PRODUCT_ROOTFS)
	# Build the modules in the kernel area, so that the LINUXBUILD area is not contaminated.
	cd $(LINUXBUILD_KERNEL) && $(MAKE) ARCH=$(ARCH) INSTALL_MOD_PATH=${PRODUCT_ROOTFS} modules modules_install

wlanconfig:
	$(CC) -I$(MARVELL_8686_SRC_DIR)/wlan -I$(MARVELL_8686_SRC_DIR)/os/linux -DMOTO_PLATFORM  -o $(MARVELL_8686_WLANCONFIG_DIR)/wlanconfig $(MARVELL_8686_WLANCONFIG_DIR)/wlanconfig.c


distclean: clean

sierraOmegaWheelKconfig:
# Hack source in /vobs/3gsm_sierra/gpl_drivers/Kconfig if the virtual keypad is turned on
	mkdir -p $(LINUXBUILD)/drivers
ifneq ($(FEAT_VIRTUAL_KEYMAP),0)
	@echo 'source "/vobs/3gsm_sierra/gpl_drivers/Kconfig"' > $(LINUXBUILD)/drivers/sierraOmegaWheelKconfig
else
	@echo "" > $(LINUXBUILD)/drivers/sierraOmegaWheelKconfig
endif

else
# rules for PRODUCT_CONF=x86

.PHONY: api_build impl

api_build: $(PROPFILES)

impl:

endif

