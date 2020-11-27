/*
 * Copyright 2005 Motorola, Inc.
 *
 * This code is licensed under LGPL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA"
 */

/*
 * Revision History:
 *
 * Date        Author    Comment
 * 10/10/2005  Motorola  Initial version.
 */

#ifndef PROFILER_LOGGER_H
#define PROFILER_LOGGER_H

/* control channel command values */
typedef enum
{
	APP_START = 1,
	APP_START_ACK_SUCCESS,
        APP_START_ACK_FAILURE,
        APP_STOP,
	LOGGING_START,
	LOGGING_STOP,
	KERNEL_EXIT
} ctl_msg_type_t;


typedef struct data_channel_conf{
    unsigned int buf_size;
    unsigned int number_bufs;
    char filename[20];
}data_channel_conf_t;

#endif   /* PROFILER_LOGGER_H end */
