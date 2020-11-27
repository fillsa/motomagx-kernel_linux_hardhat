/*
 * Copyright 2004-2005 Freescale Semiconductor, Inc.
 * Copyright (C) 2006-2007 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 *
 * Date     Author    Comment
 * 10/2006  Motorola  Added support to panel_info for the HVGA display driver.
 * 04/2007  Motorola  Removed support for the HVGA driver
 */

/*
 * @file    mxcfb.h
 *
 * @brief Global header file for the MXC Frame buffer
 * 
 * @ingroup Framebuffer
 */
#ifndef _INCLUDE_MXCFB_H_
#define _INCLUDE_MXCFB_H_

#include <linux/fb.h>

/*
 * Bits per pixel defines
 */
#ifdef CONFIG_FB_MXC_16BPP
#define MXCFB_DEFUALT_BPP       16
#elif CONFIG_FB_MXC_24BPP
#define MXCFB_DEFUALT_BPP       24
#elif CONFIG_FB_MXC_32BPP
#define MXCFB_DEFUALT_BPP       32
#endif

#define MXCFB_MAIN_DEVICE       "/dev/fb/0"
#define MXCFB_OVERLAY_DEVICE    "/dev/fb/1"
#define MXCFB_CLI_DEVICE        "/dev/fb/2"

struct mxcfb_gbl_alpha {
	int enable;
	int alpha;
};

struct mxcfb_color_key {
	int enable;
	__u32 color_key;
};

#define MXCFB_SET_GBL_ALPHA     _IOW('M', 0, struct mxcfb_gbl_alpha)
#define MXCFB_SET_CLR_KEY       _IOW('M', 1, struct mxcfb_color_key)
#if !defined(__KERNEL__) || defined(CONFIG_MOT_WFN429)
#define MXCFB_WAIT_FOR_VSYNC    _IO('M', 2)
#else
#define MXCFB_WAIT_FOR_VSYNC	_IOW('F', 0x20, u_int32_t)
#endif
#define MXCFB_SET_BRIGHTNESS    _IOW('M', 3, __u8)

#ifdef __KERNEL__

#include "../drivers/mxc/ipu/ipu.h"

enum {
	MXCFB_REFRESH_OFF,
	MXCFB_REFRESH_AUTO,
	MXCFB_REFRESH_PARTIAL,
};

struct mxcfb_rect {
	u32 top;
	u32 left;
	u32 width;
	u32 height;
};

int mxcfb_set_refresh_mode(struct fb_info *fbi, int mode,
			   struct mxcfb_rect *update_region);

struct panel_info {
	char *name;
	ipu_panel_t type;
	uint32_t pixel_fmt;
	int width;
	int height;
	ipu_di_signal_cfg_t sig_pol;
	int vSyncWidth;
	int vStartWidth;
	int vEndWidth;
	int hSyncWidth;
	int hStartWidth;
	int hEndWidth;
};

/*!
 * Structure containing the MXC specific framebuffer information.
 */
struct mxcfb_info {
	int32_t open_count;
	bool enabled;
	int blank;
	uint32_t disp_num;
	ipu_channel_t ipu_ch;
	uint32_t ipu_ch_irq;
	uint32_t cur_pixel_fmt;
	uint32_t cur_ipu_buf;

	u32 pseudo_palette[16];

	struct semaphore flip_sem;
	spinlock_t fb_lock;

	int32_t cur_update_mode;
	unsigned long alloc_start_paddr;
	u32 alloc_size;
	uint32_t snoop_window_size;
};

#endif				/* __KERNEL__ */
#endif
