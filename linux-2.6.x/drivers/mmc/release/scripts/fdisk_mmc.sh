#!/bin/sh
# Copyright (c) 2006,2007 Motorola Inc

# This program is free software; you can redistribute it
#!/bin/sh

# Copyright (c) 2007 , Motorola Inc

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

# Date          Author          Comment
# 07/20/2007    Motorola        create
#

busybox fdisk $1 <<End
o
n
p
1


t
6
w


