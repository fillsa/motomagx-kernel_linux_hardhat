/*
 * Copyright (C) 2004, 2006 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2004-Dec-17 - Rewrote software for a less generic interface
 */

#ifndef __ATOD_H__
#define __ATOD_H__

/*!
 * @file atod.h
 * @brief This is the header of internal definitions for the power IC AtoD interface.
 *
 * @ingroup poweric_atod
 */

#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/types.h>

int atod_init(void);
int atod_ioctl(unsigned int cmd, unsigned long arg);
unsigned int atod_poll(struct file *file, poll_table *wait);
ssize_t atod_read(struct file *file, char *buf, size_t count, loff_t *ppos);


#endif /* __ATOD_H__ */
