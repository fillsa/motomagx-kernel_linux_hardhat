# File: app/Makefile
#
# (c) Copyright © 2003-2006, Marvell International Ltd. 
# (c) Copyright © 2006-2007, Motorola.
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
#
# Date         Author         Comment
# ==========   ===========    ==========================
# 30-Oct-2006  Motorola       Build of wlanconfig.
# 18-Jan-2006  Motorola       WIFI driver :integrate Marvell 8686 16/01 release (81044p3_26340p44)
# 02-Mar-2007  Motorola       WIFI driver :integrate Marvell 8686 01/03 release (81047p1_26340p54)
# 11-Apr-2007  Motorola       WIFI driver :integrate Marvell 8686 11/04 release (81047p5_26340p65)
# 30-May-2007  Motorola       WIFI driver :integrate Marvell 8686 30/05 release (81047p8_26340p68)
# 10-Jul-2007  Motorola       WIFI driver :integrate Marvell 8686 09/07 release (81047p12_26340p72)
# 16-Oct-2007  Motorola       WIFI driver :integrate Marvell 8686 10/15 release (81048p0_26340p75)




COMPILERTOOLPATH=/vobs/jem/cee4_lsp/mobilinux/devkit/arm/v6_vfp_le/bin
COMPILERTOOLPREFIX=arm_v6_vfp_le-
CC        = $(COMPILERTOOLPATH)/$(COMPILERTOOLPREFIX)gcc

PATH_TO_TOP = ../..
INSTALLPATH := $(PATH_TO_TOP)

CFLAGS += -I$(PATH_TO_TOP)/os/linux
CFLAGS +=  -I$(PATH_TO_TOP)/wlan
CFLAGS += -DMOTO_PLATFORM

#
# List of application executables to create
#
TARGETS := wlanconfig

#
# Make target rules
#

# All rule compiles list of TARGETS using builtin program target from src rule
all : $(TARGETS)

# Update any needed TARGETS and then copy to the install path
build install: $(TARGETS)
	@cp -f $< $(INSTALLPATH)

clean:
	@rm -f $(TARGETS)

distclean: clean
	@rm -f *~ core
