#
# Makefile for the FX2LP Bus Driver 
#
#  Copyright (c) 2006-2007, Motorola, All Rights Reserved.
#  This program is licensed under a BSD license with the following terms:
# 
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are 
#   met:
# 
#   Redistributions of source code must retain the above copyright notice, 
#   this list of conditions and the following disclaimer.
# 
#   Redistributions in binary form must reproduce the above copyright notice, 
#   this list of conditions and the following disclaimer in the documentation 
#   and/or other materials provided with the distribution.
# 
#   Neither the name of Motorola nor the names of its contributors may be 
#   used to endorse or promote products derived from this software without 
#   specific prior written permission.
# 
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
#   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE 
#   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
#   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#  Changelog:
#  Date               Author           Comment
#  -----------------------------------------------------------------------------
#  Mar  8, 2006       Motorola, Inc.   Initial create
#  Sep 21, 2006       Motorola, Inc.   call hotplug script if FX2LP WAKEUP not 
#                                      connected
#  Oct 30, 2006       Motorola, Inc.   Bus Speed initialization issues
#  Nov 21, 2006       Motorola, Inc.   Redesign
#  Jul 26, 2007       Motorola, Inc.   Changes for open source compiance.
#  Aug 24, 2007       Motorola, Inc.   Changes for open source compiance.
#


OTG=$(TOPDIR)/drivers/otg
OTGCORE_DIR=$(OTG)/otgcore
FX2LPDIR=$(OTG)/ocd/fx2lp
USBDCORE_DIR=$(TOPDIR)/drivers/usb
MXCATLASDIR=$(TOPDIR)/drivers/mxc/atlas
PMICRRDIR=$(TOPDIR)/drivers/mxc/roadrunner
EXTRA_CFLAGS += -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR) -I$(MXCATLASDIR) -I$(PMICRRDIR) -DFX2LP_NODMA -DFX2LP_HSUSB -I$(FX2LPDIR)
EXTRA_CFLAGS_nostdinc += -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR) -I$(MXCATLASDIR) -I$(PMICRRDIR)

ifeq ($(CONFIG_MOT_FEAT_FX2LP_I2C), y)
    ifeq ($(CONFIG_OTG_HIGH_SPEED), y)
        obj-$(CONFIG_OTG_BRASSBOARD) += brassboard_tr_fx2lp.o
    endif
endif


ifeq ($(CONFIG_MOT_FEAT_FX2LP_I2C), y)
    ifeq ($(CONFIG_OTG_HIGH_SPEED), y)
        brassboard_tr_fx2lp-objs := ../brassboard/brassboard-tr.o
        brassboard_tr_fx2lp-objs += ../otglib/pcd.o
        brassboard_tr_fx2lp-objs += ../mxc/mxc-hrt.o 
        brassboard_tr_fx2lp-objs += fx2lp_lib.o
        brassboard_tr_fx2lp-objs += fx2lp_ocd.o
        brassboard_tr_fx2lp-objs += fx2lp_pcd.o
        brassboard_tr_fx2lp-objs += fx2lp_tcd.o
        brassboard_tr_fx2lp-objs += fx2lp_ready_events.o
    endif
endif

