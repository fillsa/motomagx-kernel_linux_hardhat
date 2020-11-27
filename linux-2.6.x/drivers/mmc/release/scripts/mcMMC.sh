#!/bin/sh
##
# mcMMC.sh
#
#  Copyright (C) 2007 Motorola, Inc.
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
# ChangeLog:
# (mm-dd-yyyy)  Author    Comment
# 09-23-2008   Motorola   creat for shredding cards
#

if /etc/initservices/services/umountMMCSD.sh
then
	echo "umount ok"
else
	echo "umount fail and exit 2"
	exit 2
fi

/sbin/iommcerase 0
echo "iommcerase done"

/etc/fdisk_mmc.sh

for count in 1 2 3 4 5 6 7 8 9
do
  echo "Loop count = $count"
  if [ -b /dev/mmc/blk0/part1 ]
  then
    echo "/dev/mmc/blk0/part1 does exist. Exit the loop"
    break;
  else
    echo "/dev/mmc/blk0/part1 does NOT exist..."
    echo "Run . /etc/fdisk_mmc.sh again"
    . /etc/fdisk_mmc.sh
  fi
done
  
if [ -b /dev/mmc/blk0/part1 ]
then
  echo "fdisk done" 
else
  echo "There is an issue for fdisk, exit with error code 5"
  exit 5
fi

mkdosfs -F 16 /dev/mmca1
echo "format done"

if /etc/initservices/services/mountMMCSD.sh
then
	echo "mount successfully"
else
	echo "mount fail.exit 1"
	exit 1
fi
echo "erase done! exit 0"
exit 0
