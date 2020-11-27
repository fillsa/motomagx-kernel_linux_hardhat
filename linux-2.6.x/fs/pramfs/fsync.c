/*
 * FILE NAME fs/pramfs/fsync.c
 *
 * BRIEF DESCRIPTION
 *
 * fsync operation for directory and regular files.
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

/*
 *	File may be NULL when we are called. Perhaps we shouldn't
 *	even pass file to fsync ?
 */

int pram_sync_file(struct file * file, struct dentry *dentry, int datasync)
{
	// FIXME: check
	return 0;
}

