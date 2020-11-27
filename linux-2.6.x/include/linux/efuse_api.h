/* 
 * efuse_api.h - E-Fuse driver external definitions and types
 *
 * Copyright (C) 2005-2007 Motorola, Inc.
 *
 */

/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 10-Oct-2005  Motorola        Initial revision.
 * 05-Sept-2005 Motorola        Modified header.
 * 04-Oct-2006  Motorola        Modified header.
 * 24-Apr-2007  Motorola        Added SCS1 access.
 * 18-Aug-2007  Motorola        Add comments.
 */

#ifndef  _EFUSE_API_H_
#define  _EFUSE_API_H_

#include <asm/ioctl.h>

/* Defines */

#define EFUSE_MAGIC		'b'

#define EFUSE_MOD_NAME		"efuse"

#define EFUSE_DEVICE_NAME	"/dev/"EFUSE_MOD_NAME

/* Enumerations and Data Structures */

/* The returned error codes */
typedef enum efuse_ioctl_status 
{
    EFUSE_SUCCESS		= 0x00,
    EFUSE_PARITY_ERR		= 0x02,
    EFUSE_SENSE_ERR		= 0x04,
    EFUSE_WLRE_ERR		= 0x08,
    EFUSE_READ_PROTECT_ERR	= 0x10,
    EFUSE_OVERRIDE_PROTECT_ERR	= 0x20,
    EFUSE_WRITE_PROTECT_ERR	= 0x40,
    EFUSE_PROGRAM_ERR		= 0x80
} efuse_status_t;

/* The data structure passed in and out of the ioctl call */
typedef struct efuse_data
{
    int bank;
    int offset;
    int length;
    efuse_status_t status;
    unsigned char *buf;
} efuse_data_t;

typedef struct scs_data
{
    efuse_status_t status;
    unsigned char *buf;
} scs_data_t;

typedef enum
{
    EFUSE_READ  = _IOR(EFUSE_MAGIC,1,struct efuse_data),
    GET_SCS1    = _IOR(EFUSE_MAGIC,2,struct scs_data)
} efuse_ioctl_operation_t;

#endif /* _EFUSE_API_H_ */
