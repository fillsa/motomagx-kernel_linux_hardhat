#!/bin/sh
#
# Copyright (c) 2007-2008 Motorola Inc
#
# This program is free software; you can redistribute it
# and/or modify it under the terms of the GNU General
# Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at
# your option) any later version.  You should have
# received a copy of the GNU General Public License
# along with this program; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave,
# Cambridge, MA 02139, USA
#
# This Program is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of
# MERCHANTIBILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# Date          Author          Comment
# 12/21/2007    Motorola        create
# 02/15/2008    Motorola        umount then dosfsck
# 02/16/2008    Motorola        Tracing FAT panic
# 05/04/2008	Motorola	pop up dialog
# 02/25/2008    Motorola        if finding filesystem panic then umount sd card,this is temporary solution,wait better solutio                                n in future release
# 12/16/2008    Motorola        Repair FAT on SD card on slot 0 instead of slot 1. Do nothing on slot 1 - should never be called

ACTION_MMC=/ezxlocal/.action_mmc

exec < /dev/null
#test -t 1 || exec > /dev/null

test -t 1 || exec > /dev/null
test -t 2 || exec 2>&1

#set env
. /etc/initservices/services/ezxenv.sh
echo "reparifat: repair $1"

if [ $# -ne 1 ]; then
        echo "Error parameter count"
        exit 1;
fi

if [ $1 = "/dev/mmc/blk0/part1" -o $1 = "/dev/mmca1" ];
then
	dev="/dev/mmca1"
	mp="/mmc/mmca1"
	slot="0"
elif [ $1 = "/dev/mmc/blk1/part1" ]
then
	dev="/dev/mmc/blk1/part1"
	mp="/mmc/megasim1"
	slot="1"
else
	echo "repairfat.sh parameter error"
	exit 1;
fi


if [ `ls $1` ];
then
	echo "repairfat: try to dosfsck"
	/etc/initservices/services/umountMMCSD.sh $slot
      if [ "$slot" -eq "0" ];then
          echo "slot0"
          
	export SLOT=$slot
	export ACTION="refsck"
	echo "$SLOT $ACTION" > $ACTION_MMC
	echo "call to mmchotplug $SLOT $ACTION"
	exec /sbin/mmchotplug $SLOT $ACTION

      else
        echo "slot1 - do nothing"

#		export SLOT=$slot
#		export ACTION="remove"
#		/etc/hotplug/mmc.agent
      fi
	echo "1" > /proc/mmc/repairfat
	echo "repairfat: end of dosfsck"
fi

