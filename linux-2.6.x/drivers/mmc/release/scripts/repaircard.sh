#!/bin/sh
#
# Copyright (c) 2008 Motorola Inc
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
# 04/10/2008    Motorola        create


mount_disk="/dev/mmc/blk$1/disc"
mount_node="/dev/mmc/blk$1/part1"
SLOT=$1
ACTION=$2
ACTION_MMC=/ezxlocal/.action_mmc

domkdosfs (){
	if [ `cat /sys/block/mmcblk$1/size` -gt 4000000 ];
	then
		mkdosfs -F 32 $2
	else
		mkdosfs -F 16 $2
	fi
}

echo "" > $ACTION_MMC
if [ "$ACTION" = "" ];
then
	echo "repaircard: ACTION is $2 null, nothing to do"
	exit 1

else
	case "$ACTION" in
		cancel)
			echo "repaircard: $SLOT cancel"
			echo "$SLOT $ACTION" > $ACTION_MMC
		;;
		refsck)
			echo "repaircard: $SLOT refsck"
			/etc/mmcfsck.sh ${mount_node} $SLOT
			ret=$?
			if [ $ret -eq 0 ]
			then 
				echo "repaircard: mmcfsck successfully"
				/etc/initservices/services/mountMMCSD.sh $SLOT
				ret=$?
				if [ $ret -eq 0 ]
				then 
					echo "repaircard: mount successfully"
				else
					echo "repaircard: mount fail after fsck"
					exit 1
				fi
			else
				echo "repaircard: mmcfsck fail"
				exit 1
			fi
		;;
		refdisk)
			/etc/fdisk_mmc.sh ${mount_disk}
		;;
		remkfs)
			domkdosfs $SLOT ${mount_node}
		;;
		*)
			echo "repaircard: wrong param"
			exit 1
		;;
	esac
fi
echo "repaircard: end"
exit 0
