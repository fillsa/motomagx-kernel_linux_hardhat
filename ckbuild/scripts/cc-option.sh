#!/bin/sh
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
# Copyright 2006 Motorola
# 
# Motorola  2006-Nov-13 - Initial revision.

# This script is based on the $(cc-option) function defined in the
# Linux kernel's toplevel Makefile.

CC=$1
CFLAGS=$2
OPT1=$3
OPT2=$4

if [ -z "$OPT1" ]; then
    echo "Usage : $0 CC CFLAGS OPT1 OPT2"
    exit 1
fi

if $CC $CFLAGS $OPT1 -S -o /dev/null -xc /dev/null > /dev/null 2>&1; then 
    echo "$OPT1";
else
    echo "$OPT2";
fi
