/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @defgroup MXC_V4L2_CAPTURE MXC V4L2 Video Capture Driver
 */
/*!
 * @file mxc_v4l2_capture.h
 *
 * @brief mxc V4L2 capture device API  Header file
 *
 * It include all the defines for frame operations, also three structure defines
 * use case ops structure, common v4l2 driver structure and frame structure.
 *
 * @ingroup MXC_V4L2_CAPTURE
 */
#ifndef MXC_V4L2_CAPTURE_H
#define MXC_V4L2_CAPTURE_H

#include <asm/uaccess.h>
#include <linux/videodev.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include "../drivers/mxc/ipu/ipu.h"

#define FRAME_NUM 3

#define V4L2_CID_MXC_ROT           (V4L2_CID_PRIVATE_BASE+0)
#define V4L2_CID_MXC_FLASH         (V4L2_CID_PRIVATE_BASE+1)

/*!
 * v4l2 frame structure.
 */
struct mxc_v4l_frame {
	u32 paddress;
	int count;
	int width;
	int height;

	struct v4l2_buffer buffer;
};

typedef struct {
	u8 clk_mode;
	u8 ext_vsync;
	u8 Vsync_pol;
	u8 Hsync_pol;
	u8 pixclk_pol;
	u8 data_pol;
	u8 data_width;
	u16 width;
	u16 height;
	u32 pixel_fmt;
	u32 mclk;
} sensor_interface;

/* Sensor control function */
struct camera_sensor {
	void (*set_color) (int bright, int saturation, int red, int green,
			   int blue);
	void (*get_color) (int *bright, int *saturation, int *red, int *green,
			   int *blue);
	sensor_interface *(*config) (int *frame_rate, int high_quality);
	sensor_interface *(*reset) (void);
};

/*!
 * common v4l2 driver structure.
 */
typedef struct _cam_data {
	struct video_device *video_dev;

	/* semaphore guard against SMP multithreading */
	struct semaphore busy_lock;

	int open_count;

	/* params lock for this camera */
	struct semaphore param_lock;

	/* Encorder */
	int headFrame;
	int tailFrame;
	int ping_pong_csi;
	spinlock_t int_lock;
	struct mxc_v4l_frame frame[FRAME_NUM];
	int skip_frame;
	wait_queue_head_t enc_queue;
	int enc_counter;
	void *rot_enc_bufs[2];
	enum v4l2_buf_type type;

	/* still image capture */
	wait_queue_head_t still_queue;
	int still_counter;
	void *still_buf;

	/* overlay */
	struct v4l2_window win;
	struct v4l2_framebuffer v4l2_fb;
	void *vf_bufs[2];
	void *rot_vf_bufs[2];
	bool overlay_active;
	int output;
	struct fb_info *overlay_fb;

	/* v4l2 format */
	struct v4l2_format v2f;
	ipu_rotate_mode_t rotation;

	/* V4l2 control bit */
	int bright;
	int hue;
	int contrast;
	int saturation;
	int red;
	int green;
	int blue;

	/* standart */
	struct v4l2_streamparm streamparm;
	struct v4l2_standard standard;

	/* crop */
	struct v4l2_rect crop_bounds;
	struct v4l2_rect crop_defrect;
	struct v4l2_rect crop_current;

	int (*enc_update_eba) (u32 eba, int *bufferNum);
	int (*enc_enable) (void *private);
	int (*enc_disable) (void *private);
	int (*vf_start_adc) (void *private);
	int (*vf_stop_adc) (void *private);
	int (*vf_start_sdc) (void *private);
	int (*vf_stop_sdc) (void *private);
	int (*csi_start) (void *private);
	int (*csi_stop) (void *private);

	/* misc status flag */
	bool overlay_on;
	bool capture_on;
	int overlay_pid;
	int capture_pid;
	bool low_power;
	wait_queue_head_t power_queue;

	/* camera sensor interface */
	struct camera_sensor *cam_sensor;
} cam_data;

void camera_callback(u32 mask, void *dev);
void set_mclk_rate(uint32_t * p_mclk_freq);
#endif				/* MXC_V4L2_CAPTURE_H */
