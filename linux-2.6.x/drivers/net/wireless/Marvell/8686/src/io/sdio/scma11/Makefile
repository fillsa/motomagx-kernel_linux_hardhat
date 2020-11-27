# File: Makefile
#
# (c) Copyright © 2003-2006, Marvell International Ltd. 
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

# debugging?
# DEBUG LEVEL n/0/1:
# 0: MSG
# 1: ENTER/LEAVE, MSG
CONFIG_DEBUG=n

ifeq ($(CONFIG_DEBUG),0)
CFLAGS += -DDEBUG_SDIO_LEVEL0 
endif

ifeq ($(CONFIG_DEBUG),1)
CFLAGS += -DDEBUG_SDIO_LEVEL0 -DDEBUG_SDIO_LEVEL1 -DDEBUG_USEC
endif

all:		sdio.o
		@echo "Finished making SDIO Driver"

build:		all
		@if [ ! -d $(BINDIR) ]; then \
			mkdir $(BINDIR); \
		fi
clean:
		rm -f *.o *~ core

