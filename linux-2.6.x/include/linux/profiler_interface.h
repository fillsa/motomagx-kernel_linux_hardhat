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
 * 06/14/2005  Motorola  Initial version.
 * 10/18/2005  Motorola  Added logging feature
 */

#ifndef PROFILER_INTERFACE_H
#define PROFILER_INTERFACE_H

#include <linux/ioctl.h>

typedef struct profiler_threshold {
   unsigned short high_threshold;
   unsigned short low_threshold;
} profiler_threshold_t;

typedef struct profiler_interval {
   unsigned short upper_bound_interval;
   unsigned short lower_bound_interval;
} profiler_interval_t;

typedef struct profiler_event {
   unsigned short type;
   unsigned short info;
} profiler_event_t;


/* event types */
#define PROFILER_DEC_FREQ 0
#define PROFILER_INC_FREQ 1 

#define PROFILER_DEVICE_NAME "profiler"
#define PROFILER_DEVICE "/dev/"PROFILER_DEVICE_NAME

#define PROFILER_IOC_MAGIC  'K'
#define PROFILER_IOC_SET_PROF_INTERVAL _IOW(PROFILER_IOC_MAGIC,0,profiler_interval_t)
#define PROFILER_IOC_GET_PROF_INTERVAL _IOR(PROFILER_IOC_MAGIC,1,profiler_interval_t)
#define PROFILER_IOC_SET_THRESHOLD _IOW(PROFILER_IOC_MAGIC,2,profiler_threshold_t)
#define PROFILER_IOC_GET_THRESHOLD _IOR(PROFILER_IOC_MAGIC,3,profiler_threshold_t)
#define PROFILER_IOC_CLEAR_EVENTQ _IO(PROFILER_IOC_MAGIC,4)

#define PROFILER_IOC_SET_EVENT _IOW(PROFILER_IOC_MAGIC,5,profiler_event_t)

#endif   /* PROFILER_INTERFACE_H end */
