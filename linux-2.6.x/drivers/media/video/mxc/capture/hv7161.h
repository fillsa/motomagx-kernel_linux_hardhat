/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file hv7161.h
 *
 * @brief HV7161 Camera Header file
 *
 * @ingroup Camera
 */

#ifndef HV7161_H_
#define HV7161_H_

#define HV7161_I2C_ADDRESS	0x11

typedef struct {
	u8 index;
	u16 width;
	u16 height;
} hv7161_image_format;

struct hv7161_reg {
	u8 reg;
	u8 val;
};

#endif				// HV7161_H_
