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
# 05/11/2008    Motorola        add repairfat dialog box

ID=$2
DEV_NODE=$1
FSCKPID=""
MEMRSS=""
TIMESLIP=60
ACTION_MMC=/ezxlocal/.action_mmc
SLOT=""
ACTION=""

if [ "$1" = "" ]
then
	echo "mmcfsck exit. parameter err"
	exit 1
else
	echo "mmcfsck: dev node is $DEV_NODE . Slot is $ID"
fi

if ps aux | grep dosfsck | grep "$DEV_NODE" 
then
	echo "mmcfsck: There is already dosfsck runing on $DEV_NODE. So exit normally"
	exit 0
fi

if [ -e "/dev/mmc/blk$ID" ];
then
	echo "mmcfsck: dev present"	
else
	echo "mmcfsck: dev is not present. So exit"
	exit 1
fi
	
dosfsck -awV "$DEV_NODE" &

FSCKPID=`ps aux | grep dosfsck | grep "$DEV_NODE" | awk '{ print $1 }'`
echo "mmcfsck: pid is $FSCKPID"

if [ "$FSCKPID" = "" ]
then
	echo "mmcfsck: PID is null. Maybe the process have already been finished"
	exit 0
fi

while ps aux | grep "$DEV_NODE" | grep "$FSCKPID" > /dev/null
do
	MEMRSS=`cat /proc/"$FSCKPID"/status | grep VmRSS | awk '{ print $2 }'`
	echo "MEMRSS is $MEMRSS"
	if [ "$MEMRSS" -gt 9000000 ]
	then
		echo "mmcfsck: memory too large. exit 1"
		exit 1
	fi
	
	TIMESLIP=$(($TIMESLIP - 1))
	sleep 1
	if [ "$TIMESLIP" -gt 0 ]
	then
		echo "mmcfsck: goon $TIMESLIP"
	else
		echo "mmcfsck: time out. exit 1"
		kill -9 "$FSCKPID"
		exit 1
	fi

	if [ -e $ACTION_MMC ];
	then
		ACTION=`cat $ACTION_MMC | awk '{ print $2 }'`
		SLOT=`cat $ACTION_MMC | awk '{ print $1 }'`
		echo "$ACTION_MMC: $SLOT $ACTION"

		if [ "$SLOT" = "$ID" ] && [ "$ACTION" = "cancel" ];
		then 
			echo "" > $ACTION_MMC
			echo "mmcfsck: cancel by manual"
			kill -9 "$FSCKPID"
			exit 1
		fi
	fi
done
echo "mmcfsck: exit normally"
exit 0

