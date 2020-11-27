/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __MXC_AUDIO_MC13783_IOCTLS_H__
#define __MXC_AUDIO_MC13783_IOCTLS_H__

 /**/
#define SNDCTL_CLK_SET_MASTER                   	_SIOR('Z', 30, int)
#define SNDCTL_MC13783_READ_OUT_BALANCE      		_SIOR('Z', 8, int)
#define SNDCTL_MC13783_WRITE_OUT_BALANCE      		_SIOWR('Z', 9, int)
#define SNDCTL_MC13783_WRITE_OUT_ADDER			_SIOWR('Z', 10, int)
#define SNDCTL_MC13783_READ_OUT_ADDER			_SIOR('Z', 11, int)
#define SNDCTL_MC13783_WRITE_CODEC_FILTER			_SIOWR('Z', 12, int)
#define SNDCTL_MC13783_READ_CODEC_FILTER			_SIOR('Z', 13, int)
#define SNDCTL_DAM_SET_OUT_PORT				_SIOWR('Z', 14, int)
#endif				/* __MXC_AUDIO_MC13783_IOCTL_H__ */
