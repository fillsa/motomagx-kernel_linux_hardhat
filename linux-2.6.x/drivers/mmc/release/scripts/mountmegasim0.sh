#!/bin/sh
#
# Copyright (c) 2006, 2007, 2008, Motorola Inc
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
# 10/30/2007	Motorola	modify the mount script
# 11/28/2007    Motorola        Add new .system support
# 01/11/2008    Motorola        upgrade mount support
# 02/20/2008    Motorola        remove sticky mode
# 05/11/2008    Motorola        add repairfat dialogbox 
#

MMC_PROC=/dev/mmc
MOUNT_OPTIONS="-o uid=2000 -o gid=233 -o iocharset=utf8 -o shortname=mixed -o umask=002 -o noatime"
ID=$1
ACTION=""
MOUNT_PATH=""
MOUNT_NODE=""
AUTOCHK_MMC=/ezxlocal/.autofsck_mmc
ACTION_MMC=/ezxlocal/.action_mmc
START_MMC=""

pre_creat_system_files () {
	if [ ! -d "$MOUNT_PATH/.system" ]
	then
		mkdir "$MOUNT_PATH/.system"
		if [ $? -eq 0 ]; then
			chmod 755 "$MOUNT_PATH/.system"
			chown ezx:ezx "$MOUNT_PATH/.system"
		else
			echo "Unable to create the $MOUNT_PATH/.system"
		fi
	fi
}

domount (){
	echo "mount try to $MOUNT_NODE $MOUNT_PATH"

	if cat /etc/mtab | grep $MOUNT_NODE > /dev/null
	then
		echo "$MOUNT_NODE is mounted already"
		doumount $MOUNT_NODE
	else
		echo "$MOUNT_NODE is ready to be mounted"
	fi
	
        mount -t vfat $MOUNT_NODE $MOUNT_PATH $MOUNT_OPTIONS
	ret=$?
	if [ $ret -ne 0 ];
	then
		echo "mount $MOUNT_NODE failed"
	else
		pre_creat_system_files
		echo "mount $MOUNT_NODE successfully"
	fi
}

dofsck (){
	ACTION="refsck"
	echo "$ID $ACTION" > $ACTION_MMC
	echo "$ID need $ACTION"
}

dofdisk (){
	ACTION="refdisk"
	echo "$ID $ACTION" > $ACTION_MMC
	echo "$ID need $ACTION"
}

domkfs (){
	ACTION="remkfs"
	echo "$ID $ACTION" > $ACTION_MMC
	echo "$ID need $ACTION"
}

doremount (){
	ACTION="remount"
	echo "$ID $ACTION" > $ACTION_MMC
	echo "$ID need $ACTION"
}

domkdosfs (){
	if [ cat /sys/block/mmcblk$ID/size -gt 4000000 ];
	then
		mkdosfs -F 32 $mount_node
	else
		mkdosfs -F 16 $mount_node
	fi
}

doumount (){
	echo "try to umount $MOUNT_NODE"
	retry=10
	while [ "$retry" -gt 0 ]
	do
		if cat /etc/mtab | grep "$MOUNT_NODE" > /dev/null
		then
			echo "retry $retry still mounted"
			fuser -mk $MOUNT_NODE
			/bin/usleep $(($retry * 10))
			umount $MOUNT_NODE
			ret=$?
			if [ $ret -eq 0 ]
			then
				echo "umount $MOUNT_NODE successfully"
			fi
		else
			echo "$MOUNT_NODE is finally umounted"
			return 
		fi
		retry=$(($retry - 1))
	done

	if cat /etc/mtab | grep "$MOUNT_NODE" > /dev/null
	then 
		echo "umount failed"
	fi
}

mount_part (){
	
	mount_name=""
	mount_path=""
	mount_disk="/dev/mmc/blk$1/disc"
	mount_node="/dev/mmc/blk$1/part1"

	if [ "$1" = 0 ];
	then
		mount_name=external0
	else
		mount_name=megasim
	fi
	echo "slot $1 $mount_name"
	
	if [ "$mount_name" != "" ];
	then
		case "$mount_name" in
			external0)
				mount_disk="/dev/mmca"
				mount_node="/dev/mmca1"
				mount_path="/mmc/mmca1"
			;;
			external1)
				mount_path="/mmc/mmcb1"
			;;
			internal)
				mount_path="/mmc/movinand1"
			;;
			megasim)
				mount_path="/mmc/megasim1"
			;;
			*)
			;;
		esac

		MOUNT_NODE=$mount_node
		MOUNT_PATH=$mount_path
		if [ "$mount_path" = "" ];
		then
			echo "mount: wrong type of host $1"
			return
		fi

		if busybox fdisk -l ${mount_disk} | grep ${mount_node} > /dev/null
	        then
			echo "$mount_disk is present"
			if [ -e $AUTOCHK_MMC"_"$ID ]; then
				echo "try to dosfsck since "$AUTOCHK_MMC"_"$ID" is present"
				dofsck $MOUNT_NODE
			fi
		else
			dofdisk $mount_node
		fi

		domount $MOUNT_NODE $MOUNT_PATH

		if mount | grep $mount_node | grep 'ro'  > /dev/null
		then
			echo "$mount_node is mounted as read-only"
			doumount $MOUNT_NODE
			dofsck $MOUNT_NODE
		else
			echo "$mount_node ~~"
		fi
		
		rm -f $mount_path/FSCK[0-9][0-9][0-9][0-9].REC
		
		if [ "$mount_name" = "internal" ];
		then
			mount_bind="/ezxlocal/download/mystuff"
			mount -t vfat -o bind $mount_path $mount_bind
			ret=$?
			if [ $ret -ne 0 ];
			then
				echo "mount bind ${mount_bind} failed"
			else
				echo "mount bind ${mount_bind} successfully"
			fi
		fi

		if cat /etc/mtab | grep $mount_node > /dev/null
		then
			touch $AUTOCHK_MMC"_"$ID &
		fi
	else
		echo "mount: not found name of host $1"
	fi
}

adjustbootup (){
	date
	if [ "$1" = "start" ];
	then
		rm -f $AUTOCHK_MMC"_0"
		START_MMC="start"
		echo "adjustbootup $START_MMC"
	fi
}


edjustbootup (){
	if [ "$START_MMC" = "start" ];
	then
	START_MMC=""
	echo "edjustbootup $START_MMC"
	fi
	date
}


echo "Enter MMC/SD mount script..."
if [ "$ID" = "start" -o "$ID" = "" ];
then
	adjustbootup $ID
	rm -rf $ACTION_MMC
	touch $ACTION_MMC
	ID=0
	while [ "$ID" -lt 2 ]
	do
		if [ -e "${MMC_PROC}/blk$ID" ];
		then
		if [ "$START_MMC" = "start" ];then
			mount_part $ID &
		else
			mount_part $ID
		fi	
		else
			echo "Host ${ID} was not registered"
		fi
		ID=$(($ID + 1))
	done
	edjustbootup
else
	if [ -e "${MMC_PROC}/blk$ID" ];
	then
		mount_part $ID
	else
		echo "Host ${ID} was not registered any mmc card"
	fi
fi

