/*
 * FILE NAME fs/pramfs/ioctl.c
 *
 * BRIEF DESCRIPTION
 *
 * Ioctl method for directory and regular files.
 *
 * Author: Steve Longerbeam <stevel@mvista.com, or source@mvista.com>
 *
 * Copyright 2003 Sony Corporation
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 2003-2004 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <linux/fs.h>
#include <linux/pram_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>


int pram_ioctl (struct inode * inode, struct file * filp, unsigned int cmd,
		unsigned long arg)
{
	// FIXME: need any special ioctl's?
	return 0;
}
