#!/bin/sh
#
# Copyright (c) 2006-2007, 2008  Motorola Inc
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
# Date		Author		Comment
# 10/20/2006	Motorola	create
# 10/30/2007    Motorola        modify the umount script
# 11/28/2007    Motorola        upmerge from other CR
# 12/14/2007    Motorola        add change log
# 02/20/2008    Motorola        remove change sticky mode
#


ID=$1
AUTOCHK_MMC=/ezxlocal/.autofsck_mmc

umount_part (){

	mount_node="/dev/mmc/blk$1/part1"
	
	if [ "$1" -eq 0 ]
	then
		mount_node="/dev/mmca1"
	fi

	echo "try to umount ${mount_node}"
	retry=10
	while [ "$retry" -gt 0 ]
	do
		mount_path=`cat /etc/mtab | grep ${mount_node} | awk '{ print $2 }'`
		if [ "$mount_path" != "" ]
		then
			fuser -km ${mount_path}
			/bin/usleep $(($retry * 10))
			umount ${mount_path}
			ret=$?
			if [ $ret -eq 0 ]
			then
				echo "$retry umount ${mount_path} successful"
			else
				echo "$retry umount ${mount_path} fail"
			fi
		else
			echo "${mount_path} is finally umounted. rm autochk tag"
			rm -f $AUTOCHK_MMC"_"$ID
			return
		fi
		retry=$(($retry - 1))
	done
	echo "umount block device ${mount_node} failed"
}

if [ "$ID" = "" -o "$ID" = "stop" ];
then
	ID=0
	while [ "$ID" -lt 2 ]
	do
		umount_part $ID
		ID=$(($ID + 1))
	done
else
	umount_part $ID
fi
