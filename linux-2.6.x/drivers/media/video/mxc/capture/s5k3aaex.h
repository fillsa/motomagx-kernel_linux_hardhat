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
 * @file s5k3aaex.h
 *
 * @brief S5K3AAEX Camera Header file
 *
 * It include all the defines for bitmaps operations, also two main structure
 * one for IFP interface structure, other for sensor core registers.
 *
 * @ingroup Camera
 */

#ifndef S5K3AAEX_H_
#define S5K3AAEX_H_

/*! I2C Slave Address */
#define S5K3AAEX_I2C_ADDRESS	0x2d

enum {
	S5K3AAEX_OutputResolution_SXGA = 0,
	S5K3AAEX_OutputResolution_VGA = 0x1c,
	S5K3AAEX_OutputResolution_QVGA = 0x14,
	S5K3AAEX_OutputResolution_QQVGA = 0x16,
	S5K3AAEX_OutputResolution_CIF = 0x10,
	S5K3AAEX_OutputResolution_QCIF = 0x12,
};

enum {
	S5K3AAEX_WINWIDTH = 0x500,
	S5K3AAEX_WINHEIGHT = 0x400,
	S5K3AAEX_ROWSTART = 14,
	S5K3AAEX_COLSTART = 14,

	S5K3AAEX_HORZBLANK_DEFAULT = 142,
	S5K3AAEX_VERTBLANK_DEFAULT = 101,
};

typedef struct {
	u8 index;
	u8 imageFormat;
	u16 width;
	u16 height;
} s5k3aaex_image_format;

/*!
 * s5k3aaex ARM command Register page 0 structure.
 */
typedef struct {
	u8 addressSelect;
	u8 imageFormat;
	u8 functionOnOff;
	u8 mainClock;
} s5k3aaex_page0;

/*!
 * s5k3aaex IFP Register structure.
 */
typedef struct {
	u8 addressSelect;
	u16 wrp;
	u16 wcp;
	u16 wrd;
	u16 wcw;
	u16 vblank;
	u16 hblank;
} s5k3aaex_page2;

/*!
 * s5k3aaex Config structure
 */
typedef struct {
	s5k3aaex_page0 *page0;
	s5k3aaex_page2 *page2;
} s5k3aaex_conf;

#endif				// S5K3AAEX_H_
