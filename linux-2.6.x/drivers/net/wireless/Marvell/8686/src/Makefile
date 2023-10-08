# File: Makefile
#
# (c) Copyright © 2003-2006, Marvell International Ltd. 
# (c) Copyright © 2006-2008, Motorola.
#
# This software file (the "File") is distributed by Marvell International 
# Ltd. under the terms of the GNU General Public License Version 2, June 1991 
# (the "License").  You may use, redistribute and/or modify this File in 
# accordance with the terms and conditions of the License, a copy of which 
# is available along with the File in the gpl.txt file or by writing to 
# the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
# 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
#
# THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
# IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
# ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
# this warranty disclaimer.

# Date         Author         Comment
# ==========   ===========    ==========================
# 28-Aug-2006  Motorola       WIFI driver :integrate Marvell 8686 09/06 release
# 01-Sep-2006  Motorola       Add Power Management support - WPRM_DRV
# 12-Oct-2006  Motorola       WIFI driver :integrate Marvell 8686 10/10 release (81043)
# 03-Nov-2006  Motorola       WIFI driver :integrate Marvell 8686 02/11 release (81043p26_26340p27)
# 13-Dec-2006  Motorola       WIFI driver :integrate Marvell 8686 12/12 release (81043p34_26340p37)
# 18-Jan-2006  Motorola       WIFI driver :integrate Marvell 8686 16/01 release (81044p3_26340p44)
# 02-Mar-2007  Motorola       WIFI driver :integrate Marvell 8686 01/03 release (81047p1_26340p54)
# 11-Apr-2007  Motorola       WIFI driver :integrate Marvell 8686 11/04 release (81047p5_26340p65)
# 30-May-2007  Motorola       WIFI driver :integrate Marvell 8686 30/05 release (81047p8_26340p68) 
# 10-Jul-2007  Motorola       WIFI driver :integrate Marvell 8686 09/07 release (81047p12_26340p72)
# 16-Oct-2007  Motorola       WIFI driver :integrate Marvell 8686 10/15 release (81048p0_26340p75)
# 09-Jun-2008  Motorola       WIFI driver :integrate Marvell 8686 06/01 pre release (81048p4_26340p77)
# 17-Jun-2008  Motorola       WIFI driver :integrate Marvell 8686 release (81048p5_26340p78)


MARVELL_SRC_DIR = drivers/net/wireless/Marvell/8686/src
GPL_DRIVERS_DIR        = /vobs/3gsm_sierra/gpl_drivers
LINUXJAVA_DRIVER_DIR   = /vobs/linuxjava/drivers_1

# Use SCMA11 official CC & LD
#CC=        $(CROSS)gcc
#LD=        $(CROSS)ld
BACKUP=     /root/backup
YMD=        `date +%Y%m%d%H%M`

#############################################################################
# Configuration Options
#############################################################################

# Debug Option
# DEBUG LEVEL n/1/2:
# n: NO DEBUG
# 1: Only PRINTM(MSG,...), PRINTM(FATAL,...), ...
# 2: All PRINTM()
CONFIG_DEBUG=1

# Proc in /proc/net/wlan
CONFIG_PROC_DEBUG=y

# Linux Power Management
CONFIG_ENABLE_PM=y

# Re-association in driver
CONFIG_REASSOCIATION=y

# Manufacturing firmware support
CONFIG_MFG_CMD_SUPPORT=y

# Moto Power Management 
CONFIG_WPRM_DRV=y

# Moto Platform
CONFIG_MOTO_PLATFORM=y

# Moto Debug
CONFIG_MOTO_DBG=y

#############################################################################
# Select Platform Tools
#############################################################################

MODEXT = ko
CFLAGS += -I$(MARVELL_SRC_DIR)/os/linux
CFLAGS += -I$(MARVELL_SRC_DIR)/wlan

	BUILD_ENV_SCMA11=/usr/local/arm/3.4.3/v6_vfp_le
	TOOLPATH_SCMA11=$(BUILD_ENV_SCMA11)/bin/
	SCMA11_DIR=/tftpboot/scma11/root

	INSTALLDIR=$(SCMA11_DIR)
	PLATFORM=scma11
	TOOLPATH=$(TOOLPATH_SCMA11)
	CROSS=$(TOOLPATH)/arm_v6_vfp_le-
	KERNELDIR=/usr/src/arm/linux-2.6.10-scma11

	#Enable MXC SDMA for SCMA11 platform
	CONFIG_MXC_MMC_DMA_ENABLE=y

	ifeq ($(CONFIG_MXC_MMC_DMA_ENABLE), y)
		CFLAGS += -DMXC_MMC_DMA_ENABLE
	endif

	CFLAGS += -I$(MARVELL_SRC_DIR)/if/if_sdio -I$(MARVELL_SRC_DIR)/io/sdio/$(PLATFORM)
	BINDIR = ../bin_sd8686

#############################################################################
# Compiler Flags
#############################################################################

	CFLAGS += -I$(KERNELDIR)/include
	CFLAGS += -Wno-deprecated-declarations

	CFLAGS += -DFPNUM='"104"'

ifeq ($(CONFIG_DEBUG),1)
	CFLAGS += -DDEBUG_LEVEL1
endif

ifeq ($(CONFIG_DEBUG),2)
	CFLAGS += -DDEBUG_LEVEL1
	CFLAGS += -DDEBUG_LEVEL2
	DBG=	-dbg
endif

ifeq ($(CONFIG_PROC_DEBUG),y)
	CFLAGS += -DPROC_DEBUG
	export CONFIG_PROC_DEBUG
endif

ifeq ($(CONFIG_ENABLE_PM),y)
	CFLAGS += -DENABLE_PM
endif

ifeq ($(CONFIG_REASSOCIATION),y)
	CFLAGS += -DREASSOCIATION
endif

ifeq ($(CONFIG_MFG_CMD_SUPPORT),y)
	CFLAGS += -DMFG_CMD_SUPPORT
endif

ifeq ($(CONFIG_WPRM_DRV),y)
	CFLAGS += -DWPRM_DRV
endif

ifeq ($(CONFIG_MOTO_PLATFORM),y)
	CFLAGS += -DMOTO_PLATFORM
endif

ifeq ($(CONFIG_MOTO_DBG),y)
	CFLAGS += -DMOTO_DBG
endif

#############################################################################
# Make Targets
#############################################################################

ifneq ($(KERNELRELEASE),)

WLANOBJS =	wlan/wlan_main.o wlan/wlan_fw.o wlan/wlan_wext.o \
		wlan/wlan_rx.o wlan/wlan_tx.o \
		wlan/wlan_cmd.o wlan/wlan_cmdresp.o \
		wlan/wlan_proc.o wlan/wlan_scan.o wlan/wlan_join.o

WLANOBJS += wlan/wlan_wmm.o
WLANOBJS += wlan/wlan_11d.o
WLANOBJS += wlan/wlan_procfw.o

ifeq ($(CONFIG_PROC_DEBUG), y)
WLANOBJS += wlan/wlan_debug.o
endif
ifeq ($(CONFIG_WPRM_DRV),y)
WLANOBJS += wlan/wlan_wprm_drv.o
endif

IFOBJS := if/if_sdio/if_sdio.o
obj-m := sd8xxx.o
sd8xxx-objs := $(WLANOBJS) $(IFOBJS)
IOOBJS := io/sdio/$(PLATFORM)/sdio.o
ifeq ($(CONFIG_MOTO_PLATFORM),y)
IOOBJS += io/sdio/$(PLATFORM)/sdio_wprm_drv.o
endif
obj-m += sdio.o
sdio-objs := $(IOOBJS)

# Otherwise we were called directly from the command line; invoke the kernel build system.
else

default:
	$(MAKE) -C $(KERNELDIR) M=$(MARVELL_SRC_DIR) modules

endif

###############################################################

export		CC LD CFLAGS KERNELDIR

.PHONY: app/wlanconfig wlan clean distclean

	@echo "Finished Making Marvell Wlan Linux Driver"

wlan app/wlanconfig:
	$(MAKE) -C $@

echo:
	@echo "Please make sure the following are done before building:"
	@echo "	1. Modify release_version.h to set correct version number"
	@echo "	2. Updates the README files"

build:		echo default

	@if [ ! -d $(BINDIR) ]; then \
		mkdir $(BINDIR); \
	fi 
	cp -f sd8xxx.$(MODEXT) $(BINDIR)/sd8686$(DBG).$(MODEXT)
	cp -f sdio.$(MODEXT) $(BINDIR)

	cp -f README $(BINDIR)
	$(MAKE) -C app/wlanconfig $@ INSTALLDIR=$(BINDIR)
	cp -r config $(BINDIR)

clean:
	-find . -name "*.o" -exec rm {} \;
	-find . -name "*.ko" -exec rm {} \;
	-find . -name ".*.cmd" -exec rm {} \;
	-find . -name "*.mod.c" -exec rm {} \;
	-rm -rf .tmp_versions
	$(MAKE) -C app/wlanconfig $@

install: default

	cp -f sdio.$(MODEXT) $(INSTALLDIR)
	$(MAKE) -C app/wlanconfig $@ INSTALLDIR=$(INSTALLDIR)
	cp -f sd8xxx.$(MODEXT) $(INSTALLDIR)/sd8686$(DBG).$(MODEXT)
	echo "sd8686 Driver Installed"

distclean:
	-find . -name "*.o" -exec rm {} \;
	-find . -name "*.orig" -exec rm {} \;
	-find . -name "*.swp" -exec rm {} \;
	-find . -name "*.*~" -exec rm {} \;
	-find . -name "*~" -exec rm {} \;
	-find . -name "*.d" -exec rm {} \;
	-find . -name "*.a" -exec rm {} \;
	-find . -name "tags" -exec rm {} \;
	-find . -name ".*" -exec rm -rf 2> /dev/null \;
	-find . -name "*.ko" -exec rm {} \;
	-find . -name ".*.cmd" -exec rm {} \;
	-find . -name "*.mod.c" -exec rm {} \;
	-rm -rf .tmp_versions
	$(MAKE) -C app/wlanconfig $@

# End of file
