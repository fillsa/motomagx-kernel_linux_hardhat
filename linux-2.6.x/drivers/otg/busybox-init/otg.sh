#!/bin/sh

# Copyright (c) 2006 Motorola, Inc, All Rights Reserved.
#
#  This program is licensed under a BSD license with the following terms:
# 
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#   *  Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
#   *  Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#   *  Neither the name of Motorola nor the names of its contributors may
#      be used to endorse or promote products derived from this software without
#      specific prior written permission.
# 
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# 
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 08/09/2006         Motorola         Initial distribution
# 10/18/2006         Motorola         Add Open Src Software language
# 12/11/2006         Motorola         Changes for open src compliance review. 
#

# usb        This starts and stops the USB subsystem.
#
# description: The USB subsystem incorporates the kernel modules
#

PATH=/sbin:/bin:/usr/bin:/usr/sbin


# Source function library.
if [ -f /etc/initservices/services/init-functions ] ; then
	. /etc/initservices/services/init-functions
else
	exit 1  
fi

# Check that we are root
[ `/usr/bin/id -u` = 0 ] || exit 1

# Check that downloader exists - if any
# [ -f /sbin/dload ] || exit 1

subsystem="USBOTG"

try () {
	eval "$*" 
	rc="$?"
	if [ $rc -ne 0 ]; then
		echo "'$*' returned $rc"
	else
		echo "Done: '$*'"
	fi
}

start () {
	local opt
	echo "Starting ${subsystem}"
 
    apppath=/usr/local/bin
	# modpath=/lib/modules/2.6.10_dev/kernel/drivers/otg/

    # Start usb polling app
    #	try /usr/local/bin/usbstart & 
    	try /usr/local/bin/usbcontrol start net & 

	# install protocol modules
	# try /sbin/insmod ${modpath}/otgcore/otgcore.ko 
	# try /sbin/insmod ${modpath}/functions/network/network_if.ko 
	# try /sbin/insmod ${modpath}/functions/generic/generic_cf.ko 
	# try /sbin/insmod ${modpath}/ocd/brassboard/brassboard_tr.ko 

}

stop () {
	echo "Stopping ${subsystem}:"

}

restart () {
	stop
	start
}

# See how we were called.
case "$1" in
	start)
		start
	;;
	stop)
		stop
	;;
	restart)
		restart
	;;
	*)
		echo "Usage: $0 {start|stop|restart}"
	# exit 1
esac

# exit is logging user out at this point
# exit $?

