#!/bin/sh
# Copyright (c) 2006 , Motorola Inc

# This program is free software; you can redistribute it
# and/or modify it under the terms of the GNU General
# Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at
# your option) any later version.  You should have
# received a copy of the GNU General Public License
# along with this program; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave,
# Cambridge, MA 02139, USA

# This Program is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of
# MERCHANTIBILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

#Date           Author          Comment
#10/20/06       Motorola        create
#

TFLASH_DEV_NODE=/dev/mmc/blk1/part1
TFLASH_MOUNT_PT=/mmc/mmca1

MOVINAND_DEV_NODE=/dev/mmc/blk0/part1
MOVINAND_MOUNT_PT=/mmc/movinand1
MOVINAND_MYSTUFF=/ezxlocal/download/mystuff

MOUNT_OPTIONS="-o uid=2000 -o gid=233 -o iocharset=utf8 -o shortname=mixed -o umask=002 -o noatime"

mount -t vfat $TFLASH_DEV_NODE $TFLASH_MOUNT_PT $MOUNT_OPTIONS

mount -t vfat $MOVINAND_DEV_NODE $MOVINAND_MOUNT_PT $MOUNT_OPTIONS

if [ $? -ne 0 ]; then
    echo "ERROR: Could not mount $MOVINAND_MOUNT_PT"
    /sbin/dosfsck -a -v -V $MOVINAND_DEV_NODE
    # Try to mount again
    mount -t vfat $MOVINAND_DEV_NODE $MOVINAND_MOUNT_PT $MOUNT_OPTIONS

    if [ $? -ne 0 ]; then
        echo "ERROR: Still unable to mount, reformating $MOVINAND_DEV_NODE"

	mkdosfs $MOVINAND_DEV_NODE

    	mount -t vfat $MOVINAND_DEV_NODE $MOVINAND_MOUNT_PT $MOUNT_OPTIONS

        if [ $? -ne 0 ]; then
            echo "ERROR: All attempts to mount $MOVINAND_DEV_NODE have failed"
            exit 1;
        fi
    fi
fi

mount -o bind $MOVINAND_MOUNT_PT $MOVINAND_MYSTUFF

