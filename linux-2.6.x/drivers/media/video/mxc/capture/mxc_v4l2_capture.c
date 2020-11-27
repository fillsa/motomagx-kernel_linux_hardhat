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
 * @file mxc_v4l2_capture.c
 *
 * @brief Mxc Video For Linux 2 driver
 *
 * @ingroup MXC_V4L2_CAPTURE
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <asm/io.h>
#include <asm/semaphore.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/fb.h>

#include "mxc_v4l2_capture.h"
#include "asm/arch/board.h"
#include "../drivers/mxc/ipu/ipu.h"
#include "ipu_prp_sw.h"
#include "../drivers/video/mxc/mxcfb.h"

//#define MXC_V4L2_CAPTURE_DEBUG
#ifdef MXC_V4L2_CAPTURE_DEBUG

#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - "\
          fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START DPRINTK(" func start\n")
#  define FUNC_END DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", \
          __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* MXC_V4L2_CAPTURE_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* MXC_V4L2_CAPTURE_DEBUG */

static int csi_mclk_flag_backup;
static int video_nr = -1;
#define MXC_V4L2_CAPTURE_NUM_OUTPUTS        2
// Needed to changed after the framebuffer name been changed officially.
static struct v4l2_output mxc_capture_outputs[MXC_V4L2_CAPTURE_NUM_OUTPUTS] = {
	{
	 .index = 0,
	 .name = "DISP3",
	 .type = V4L2_OUTPUT_TYPE_ANALOG,
	 .audioset = 0,
	 .modulator = 0,
	 .std = V4L2_STD_UNKNOWN,
	 },
	{
	 .index = 1,
	 .name = "DISP0",
	 .type = V4L2_OUTPUT_TYPE_ANALOG,
	 .audioset = 0,
	 .modulator = 0,
	 .std = V4L2_STD_UNKNOWN,
	 }
};

/*!
 * Allocate frame buffers
 *
 * @param cam      Structure cam_data *
 *
 * @param count    int number of buffer need to allocated
 *
 * @return status  -0 Successfully allocated a buffer, -ENOBUFS	failed.
 */
static int mxc_allocate_frame_buf(cam_data * cam, int count)
{
	int i;

	if (count == 0)
		return 0;

	cam->frame[0].paddress =
	    ipu_malloc(PAGE_ALIGN(cam->v2f.fmt.pix.sizeimage) * count);
	if (cam->frame[0].paddress == 0) {
		printk("mxc_allocate_frame_buf ipu_malloc failed.\n");
		return -ENOBUFS;
	}

	cam->frame[0].buffer.flags = V4L2_BUF_FLAG_MAPPED;
	cam->frame[0].buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cam->frame[0].buffer.length = PAGE_ALIGN(cam->v2f.fmt.pix.sizeimage);
	cam->frame[0].buffer.memory = V4L2_MEMORY_MMAP;
	cam->frame[0].buffer.m.offset = cam->frame[0].paddress;

	for (i = 1; i < count; i++) {
		cam->frame[i].paddress = cam->frame[i - 1].paddress +
		    PAGE_ALIGN(cam->v2f.fmt.pix.sizeimage);
		cam->frame[i].buffer.flags = V4L2_BUF_FLAG_MAPPED;
		cam->frame[i].buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cam->frame[i].buffer.length =
		    PAGE_ALIGN(cam->v2f.fmt.pix.sizeimage);
		cam->frame[i].buffer.memory = V4L2_MEMORY_MMAP;
		cam->frame[i].buffer.m.offset = cam->frame[0].paddress +
		    i * PAGE_ALIGN(cam->v2f.fmt.pix.sizeimage);
	}

	cam->headFrame = cam->tailFrame = 0;

	return 0;
}

/*!
 * Free frame buffers
 *
 * @param cam      Structure cam_data *
 *
 * @return status  0 success.
 */
static int mxc_free_frame_buf(cam_data * cam)
{
	int i;

	if (cam->frame[0].paddress != 0)
		ipu_free(cam->frame[0].paddress);

	for (i = 0; i < FRAME_NUM; i++) {
		if (cam->frame[i].paddress != 0) {
			cam->frame[i].paddress = 0;
		}
	}

	cam->headFrame = cam->tailFrame = 0;
	return 0;
}

/*!
 * Free frame buffers status
 *
 * @param cam    Structure cam_data *
 *
 * @return none
 */
static void mxc_free_frames(cam_data * cam)
{
	int i;

	for (i = 0; i < FRAME_NUM; i++) {
		cam->frame[i].buffer.flags = V4L2_BUF_FLAG_MAPPED;
	}

	cam->enc_counter = 0;
	cam->skip_frame = 0;
	cam->headFrame = cam->tailFrame = 0;
}

/*!
 * Return the buffer status
 *
 * @param cam 	   Structure cam_data *
 * @param buf      Structure v4l2_buffer *
 *
 * @return status  0 success, EINVAL failed.
 */
static int mxc_v4l2_buffer_status(cam_data * cam, struct v4l2_buffer *buf)
{
	/* check range */
	if (buf->index < 0 || buf->index >= FRAME_NUM) {
		printk("mxc_v4l2_buffer_status buffers not allocated\n");
		return -EINVAL;
	}

	memcpy(buf, &(cam->frame[buf->index].buffer), sizeof(*buf));
	return 0;
}

/*!
 * start the encoder job
 *
 * @param cam      structure cam_data *
 *
 * @return status  0 Success
 */
static int mxc_streamon(cam_data * cam)
{
	int err = 0;

	if (!cam)
		return -EIO;

	if (cam->frame[0].paddress == 0) {
		printk("camera_start buffer not been queued yet!\n");
		return -EINVAL;
	}

	cam->capture_pid = current->pid;

	if (cam->enc_enable) {
		err = cam->enc_enable(cam);
		if (err != 0) {
			return err;
		}
	}

	cam->ping_pong_csi = 0;
	if (cam->enc_update_eba) {
		err = cam->enc_update_eba(cam->frame[cam->headFrame].paddress,
					  &cam->ping_pong_csi);
		err |=
		    cam->enc_update_eba(cam->frame[cam->headFrame + 1].paddress,
					&cam->ping_pong_csi);
	}

	return err;
}

/*!
 * Shut down the encoder job
 *
 * @param cam      structure cam_data *
 *
 * @return status  0 Success
 */
static int mxc_streamoff(cam_data * cam)
{
	int err = 0;

	if (!cam)
		return -EIO;

	if (cam->enc_disable) {
		err = cam->enc_disable(cam);
	}
	mxc_free_frames(cam);
	return err;
}

/*!
 * Valid whether the palette is supported
 *
 * @param palette V4L2_PIX_FMT_RGB565, V4L2_PIX_FMT_BGR24 or V4L2_PIX_FMT_BGR32
 *
 * @return 0 if failed
 */
static inline int valid_mode(u32 palette)
{
	return ((palette == V4L2_PIX_FMT_RGB565) ||
		(palette == V4L2_PIX_FMT_BGR24) ||
		(palette == V4L2_PIX_FMT_RGB24) ||
		(palette == V4L2_PIX_FMT_BGR32) ||
		(palette == V4L2_PIX_FMT_RGB32) ||
		(palette == V4L2_PIX_FMT_YUV422P) ||
		(palette == V4L2_PIX_FMT_YUV420));
}

/*!
 * Valid and adjust the overlay window size, position
 *
 * @param cam      structure cam_data *
 * @param win      struct v4l2_window  *
 *
 * @return 0
 */
static int verify_preview(cam_data * cam, struct v4l2_window *win)
{
	int i = 0;
	int *width, *height;

	do {
		cam->overlay_fb = (struct fb_info *)registered_fb[i];
		if (cam->overlay_fb == NULL) {
			printk("verify_preview No matched.\n");
			return -1;
		}
		if (strncmp(cam->overlay_fb->fix.id,
			    mxc_capture_outputs[cam->output].name, 5) == 0) {
			break;
		}
	} while (++i < FB_MAX);

	/* 4 bytes alignment for both FG and BG */
	if (cam->overlay_fb->var.bits_per_pixel == 24) {
		win->w.left -= win->w.left % 4;
	} else if (cam->overlay_fb->var.bits_per_pixel == 16) {
		win->w.left -= win->w.left % 2;
	}

	if (win->w.width + win->w.left > cam->overlay_fb->var.xres)
		win->w.width = cam->overlay_fb->var.xres - win->w.left;
	if (win->w.height + win->w.top > cam->overlay_fb->var.yres)
		win->w.height = cam->overlay_fb->var.yres - win->w.top;

	/* stride line limitation */
	win->w.height -= win->w.height % 8;
	win->w.width -= win->w.width % 8;

	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		height = &win->w.width;
		width = &win->w.height;
	} else {
		width = &win->w.width;
		height = &win->w.height;
	}

	if ((cam->crop_bounds.width / *width > 8) ||
	    ((cam->crop_bounds.width / *width == 8) &&
	     (cam->crop_bounds.width % *width))) {
		*width = cam->crop_bounds.width / 8;
		if (*width % 8)
			*width += 8 - *width % 8;
		if (*width + win->w.left > cam->overlay_fb->var.xres) {
			printk("width exceed resize limit.\n");
			return -1;
		}
		printk("width exceed limit resize to %d.\n", *width);
	}

	if ((cam->crop_bounds.height / *height > 8) ||
	    ((cam->crop_bounds.height / *height == 8) &&
	     (cam->crop_bounds.height % *height))) {
		*height = cam->crop_bounds.height / 8;
		if (*height % 8)
			*height += 8 - *height % 8;
		if (*height + win->w.top > cam->overlay_fb->var.yres) {
			printk("height exceed resize limit.\n");
			return -1;
		}
		printk("height exceed limit resize to %d.\n", *height);
	}

	return 0;
}

/*!
 * start the viewfinder job
 *
 * @param cam      structure cam_data *
 *
 * @return status  0 Success
 */
static int start_preview(cam_data * cam)
{
	int err = 0;

#ifdef CONFIG_MXC_IPU_PRP_VF_SDC
	if (cam->output == 0) {
		if (cam->v4l2_fb.flags == V4L2_FBUF_FLAG_OVERLAY)
			err = prp_vf_sdc_select(cam);
		else if (cam->v4l2_fb.flags == V4L2_FBUF_FLAG_PRIMARY)
			err = prp_vf_sdc_select_bg(cam);
		if (err != 0)
			return err;

		cam->overlay_pid = current->pid;
		err = cam->vf_start_sdc(cam);
	}
#endif

#ifdef CONFIG_MXC_IPU_PRP_VF_ADC
	if (cam->output == 1) {
		err = prp_vf_adc_select(cam);
		if (err != 0)
			return err;

		cam->overlay_pid = current->pid;
		err = cam->vf_start_adc(cam);
	}
#endif

	return err;
}

/*!
 * shut down the viewfinder job
 *
 * @param cam      structure cam_data *
 *
 * @return status  0 Success
 */
static int stop_preview(cam_data * cam)
{
	int err = 0;

#ifdef CONFIG_MXC_IPU_PRP_VF_ADC
	if (cam->output == 1) {
		err = prp_vf_adc_deselect(cam);
	}
#endif

#ifdef CONFIG_MXC_IPU_PRP_VF_SDC
	if (cam->output == 0) {
		if (cam->v4l2_fb.flags == V4L2_FBUF_FLAG_OVERLAY)
			err = prp_vf_sdc_deselect(cam);
		else if (cam->v4l2_fb.flags == V4L2_FBUF_FLAG_PRIMARY)
			err = prp_vf_sdc_deselect_bg(cam);
	}
#endif

	return err;
}

/*!
 * V4L2 - mxc_v4l2_g_fmt function
 *
 * @param cam         structure cam_data *
 *
 * @param f           structure v4l2_format *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_v4l2_g_fmt(cam_data * cam, struct v4l2_format *f)
{
	int retval = 0;

	switch (f->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		f->fmt.pix.width = cam->v2f.fmt.pix.width;
		f->fmt.pix.height = cam->v2f.fmt.pix.height;
		f->fmt.pix.sizeimage = cam->v2f.fmt.pix.sizeimage;
		f->fmt.pix.pixelformat = cam->v2f.fmt.pix.pixelformat;
		f->fmt.pix.bytesperline = cam->v2f.fmt.pix.bytesperline;
		f->fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
		retval = 0;
		break;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
		f->fmt.win = cam->win;
		break;
	default:
		retval = -EINVAL;
	}
	return retval;
}

/*!
 * V4L2 - mxc_v4l2_s_fmt function
 *
 * @param cam         structure cam_data *
 *
 * @param f           structure v4l2_format *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_v4l2_s_fmt(cam_data * cam, struct v4l2_format *f)
{
	int retval = 0;
	int size = 0;
	int bytesperline = 0;
	int *width, *height;

	switch (f->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (!valid_mode(f->fmt.pix.pixelformat)) {
			printk("mxc_v4l2_s_fmt: format not supported\n");
			retval = -EINVAL;
		}

		if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
			height = &f->fmt.pix.width;
			width = &f->fmt.pix.height;
		} else {
			width = &f->fmt.pix.width;
			height = &f->fmt.pix.height;
		}

		/* stride line limitation */
		*width -= *width % 8;
		*height -= *height % 8;

		if ((cam->crop_bounds.width / *width > 8) ||
		    ((cam->crop_bounds.width / *width == 8) &&
		     (cam->crop_bounds.width % *width))) {
			*width = cam->crop_bounds.width / 8;
			if (*width % 8)
				*width += 8 - *width % 8;
			printk("width exceed limit resize to %d.\n", *width);
		}

		if ((cam->crop_bounds.height / *height > 8) ||
		    ((cam->crop_bounds.height / *height == 8) &&
		     (cam->crop_bounds.height % *height))) {
			*height = cam->crop_bounds.height / 8;
			if (*height % 8)
				*height += 8 - *height % 8;
			printk("height exceed limit resize to %d.\n", *height);
		}

		switch (f->fmt.pix.pixelformat) {
		case V4L2_PIX_FMT_RGB565:
			size = f->fmt.pix.width * f->fmt.pix.height * 2;
			bytesperline = f->fmt.pix.width * 2;
			break;
		case V4L2_PIX_FMT_BGR24:
			size = f->fmt.pix.width * f->fmt.pix.height * 3;
			bytesperline = f->fmt.pix.width * 3;
			break;
		case V4L2_PIX_FMT_RGB24:
			size = f->fmt.pix.width * f->fmt.pix.height * 3;
			bytesperline = f->fmt.pix.width * 3;
			break;
		case V4L2_PIX_FMT_BGR32:
			size = f->fmt.pix.width * f->fmt.pix.height * 4;
			bytesperline = f->fmt.pix.width * 4;
			break;
		case V4L2_PIX_FMT_RGB32:
			size = f->fmt.pix.width * f->fmt.pix.height * 4;
			bytesperline = f->fmt.pix.width * 4;
			break;
		case V4L2_PIX_FMT_YUV422P:
			size = f->fmt.pix.width * f->fmt.pix.height * 2;
			bytesperline = f->fmt.pix.width * 2;
			break;
		case V4L2_PIX_FMT_YUV420:
			size = f->fmt.pix.width * f->fmt.pix.height * 3 / 2;
			bytesperline = f->fmt.pix.width * 3 / 2;
			break;
		default:
			size = f->fmt.pix.width * f->fmt.pix.height * 4;
			bytesperline = f->fmt.pix.width * 4;
			printk("mxc_v4l2_s_fmt: default assume"
			       " to be YUV444 interleaved.\n");
			break;
		}

		if (f->fmt.pix.bytesperline < bytesperline) {
			f->fmt.pix.bytesperline = bytesperline;
		} else {
			bytesperline = f->fmt.pix.bytesperline;
		}

		if (f->fmt.pix.sizeimage > size) {
			printk("mxc_v4l2_s_fmt: sizeimage bigger than"
			       " needed.\n");
			size = f->fmt.pix.sizeimage;
		}
		f->fmt.pix.sizeimage = size;

		cam->v2f.fmt.pix.sizeimage = size;
		cam->v2f.fmt.pix.bytesperline = bytesperline;
		cam->v2f.fmt.pix.width = f->fmt.pix.width;
		cam->v2f.fmt.pix.height = f->fmt.pix.height;
		cam->v2f.fmt.pix.pixelformat = f->fmt.pix.pixelformat;
		retval = 0;
		break;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
		retval = verify_preview(cam, &f->fmt.win);
		cam->win = f->fmt.win;
		break;
	default:
		retval = -EINVAL;
	}
	return retval;
}

/*!
 * get control param
 *
 * @param cam         structure cam_data *
 *
 * @param c           structure v4l2_control *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_get_v42l_control(cam_data * cam, struct v4l2_control *c)
{
	int status = 0;

	switch (c->id) {
	case V4L2_CID_HFLIP:
		if (cam->rotation == IPU_ROTATE_HORIZ_FLIP)
			c->value = 1;
		break;
	case V4L2_CID_VFLIP:
		if (cam->rotation == IPU_ROTATE_VERT_FLIP)
			c->value = 1;
		break;
	case V4L2_CID_MXC_ROT:
		c->value = cam->rotation;
		break;
	case V4L2_CID_BRIGHTNESS:
		c->value = cam->bright;
		break;
	case V4L2_CID_HUE:
		c->value = cam->hue;
		break;
	case V4L2_CID_CONTRAST:
		c->value = cam->contrast;
		break;
	case V4L2_CID_SATURATION:
		c->value = cam->saturation;
		break;
	case V4L2_CID_RED_BALANCE:
		c->value = cam->red;
		break;
	case V4L2_CID_BLUE_BALANCE:
		c->value = cam->blue;
		break;
	default:
		status = -EINVAL;
	}
	return status;
}

/*!
 * V4L2 - set_control function
 *          V4L2_CID_PRIVATE_BASE is the extention for IPU preprocessing.
 *          0 for normal operation
 *          1 for vertical flip
 *          2 for horizontal flip
 *          3 for horizontal and vertical flip
 *          4 for 90 degree rotation
 * @param cam         structure cam_data *
 *
 * @param c           structure v4l2_control *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_set_v42l_control(cam_data * cam, struct v4l2_control *c)
{
	switch (c->id) {
	case V4L2_CID_HFLIP:
		if (c->value == 1) {
			if ((cam->rotation != IPU_ROTATE_VERT_FLIP) &&
			    (cam->rotation != IPU_ROTATE_180))
				cam->rotation = IPU_ROTATE_HORIZ_FLIP;
			else
				cam->rotation = IPU_ROTATE_180;
		} else {
			if (cam->rotation == IPU_ROTATE_HORIZ_FLIP)
				cam->rotation = IPU_ROTATE_NONE;
			if (cam->rotation == IPU_ROTATE_180)
				cam->rotation = IPU_ROTATE_VERT_FLIP;
		}
		break;
	case V4L2_CID_VFLIP:
		if (c->value == 1) {
			if ((cam->rotation != IPU_ROTATE_HORIZ_FLIP) &&
			    (cam->rotation != IPU_ROTATE_180))
				cam->rotation = IPU_ROTATE_VERT_FLIP;
			else
				cam->rotation = IPU_ROTATE_180;
		} else {
			if (cam->rotation == IPU_ROTATE_VERT_FLIP)
				cam->rotation = IPU_ROTATE_NONE;
			if (cam->rotation == IPU_ROTATE_180)
				cam->rotation = IPU_ROTATE_HORIZ_FLIP;
		}
		break;
	case V4L2_CID_MXC_ROT:
		cam->rotation = c->value;
		break;
	case V4L2_CID_BRIGHTNESS:
		cam->bright = c->value;
		ipu_csi_enable_mclk(CSI_MCLK_I2C, true, true);
		cam->cam_sensor->set_color(cam->bright, cam->saturation,
					   cam->red, cam->green, cam->blue);
		ipu_csi_enable_mclk(CSI_MCLK_I2C, false, false);
		break;
	case V4L2_CID_HUE:
		cam->hue = c->value;
	case V4L2_CID_CONTRAST:
		cam->contrast = c->value;
	case V4L2_CID_SATURATION:
		cam->saturation = c->value;
		ipu_csi_enable_mclk(CSI_MCLK_I2C, true, true);
		cam->cam_sensor->set_color(cam->bright, cam->saturation,
					   cam->red, cam->green, cam->blue);
		ipu_csi_enable_mclk(CSI_MCLK_I2C, false, false);
		break;
	case V4L2_CID_RED_BALANCE:
		cam->red = c->value;
		ipu_csi_enable_mclk(CSI_MCLK_I2C, true, true);
		cam->cam_sensor->set_color(cam->bright, cam->saturation,
					   cam->red, cam->green, cam->blue);
		ipu_csi_enable_mclk(CSI_MCLK_I2C, false, false);
		break;
	case V4L2_CID_BLUE_BALANCE:
		cam->blue = c->value;
		ipu_csi_enable_mclk(CSI_MCLK_I2C, true, true);
		cam->cam_sensor->set_color(cam->bright, cam->saturation,
					   cam->red, cam->green, cam->blue);
		ipu_csi_enable_mclk(CSI_MCLK_I2C, false, false);
		break;
	case V4L2_CID_MXC_FLASH:
		ipu_csi_flash_strobe(true);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/*!
 * V4L2 - mxc_v4l2_s_param function
 *
 * @param cam         structure cam_data *
 *
 * @param parm        structure v4l2_streamparm *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_v4l2_s_param(cam_data * cam, struct v4l2_streamparm *parm)
{
	sensor_interface *param;
	ipu_csi_signal_cfg_t csi_param;

	if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		printk("mxc_v4l2_s_param invalid type\n");
		return -EINVAL;
	}

	if (parm->parm.capture.timeperframe.denominator >
	    cam->standard.frameperiod.denominator) {
		printk("mxc_v4l2_s_param frame rate %d larger "
		       "than standard supported %d\n",
		       parm->parm.capture.timeperframe.denominator,
		       cam->standard.frameperiod.denominator);
		return -EINVAL;
	}

	cam->streamparm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;

	ipu_csi_enable_mclk(CSI_MCLK_I2C, true, true);
	param = cam->cam_sensor->config
	    (&parm->parm.capture.timeperframe.denominator,
	     parm->parm.capture.capturemode);
	ipu_csi_enable_mclk(CSI_MCLK_I2C, false, false);
	cam->streamparm.parm.capture.timeperframe =
	    parm->parm.capture.timeperframe;

	if ((parm->parm.capture.capturemode != 0) && 
		(parm->parm.capture.capturemode != V4L2_MODE_HIGHQUALITY)) {
		printk("mxc_v4l2_s_param frame un-supported capture mode\n");
		return -EINVAL;
	}

	if (parm->parm.capture.capturemode == 
		cam->streamparm.parm.capture.capturemode) {
		return 0;
	}

    /* resolution changed, so need to re-program the CSI */
	csi_param.sens_clksrc = 0;
	csi_param.clk_mode = param->clk_mode;
	csi_param.pixclk_pol = param->pixclk_pol;
	csi_param.data_width = param->data_width;
	csi_param.data_pol = param->data_pol;
	csi_param.ext_vsync = param->ext_vsync;
	csi_param.Vsync_pol = param->Vsync_pol;
	csi_param.Hsync_pol = param->Hsync_pol;
	ipu_csi_init_interface(param->width, param->height, param->pixel_fmt,
		csi_param);
	ipu_csi_set_window_size(param->width + 1, param->height + 1);

	if (parm->parm.capture.capturemode != V4L2_MODE_HIGHQUALITY) {
		cam->streamparm.parm.capture.capturemode = 0;
	}
	else {
		cam->streamparm.parm.capture.capturemode = V4L2_MODE_HIGHQUALITY;
		cam->streamparm.parm.capture.extendedmode =
		    parm->parm.capture.extendedmode;
		cam->streamparm.parm.capture.readbuffers = 1;
	}

	return 0;
}

/*!
 * Sync on a V4L buffer
 *
 * @param file        structure file *
 * @param frame       index of frame
 *
 * @return  status    0 success, EINVAL invalid frame number,
 *                    ETIME timeout, ERESTARTSYS interrupted by user
 */
static int mxc_v4l_sync(struct file *file, int frame)
{
	struct video_device *dev = video_devdata(file);
	cam_data *cam = dev->priv;

	/* check passed-in frame number */
	if (frame >= FRAME_NUM || frame < 0) {
		printk("mxc_v4l_sync() - frame %d is invalid\n", frame);
		return -EINVAL;
	}

	if (!wait_event_interruptible_timeout(cam->enc_queue,
					      cam->enc_counter != 0, 10 * HZ)) {
		printk("mxc_v4l_sync timeout enc_counter %x\n",
		       cam->enc_counter);
		return -ETIME;
	} else if (signal_pending(current)) {
		printk("mxc_v4l_sync() interrupt received\n");
		mxc_free_frames(cam);
		return -ERESTARTSYS;
	}
	cam->enc_counter--;
	//printk("mxc_v4l_sync() cam->enc_counter %x frame %xstate"
	//" %x %x %x %x\n",
	// cam->enc_counter, frame, cam->frame[0].state, cam->frame[1].state,
	// cam->frame[2].state, cam->frame[3].state);

	return 0;
}

/*!
 * V4L interface - open function
 *
 * @param inode        structure inode *
 * @param file         structure file *
 *
 * @return  status    0 success, ENODEV invalid device instance,
 *                    ENODEV timeout, ERESTARTSYS interrupted by user
 */
static int mxc_v4l_open(struct inode *inode, struct file *file)
{
	sensor_interface *param;
	ipu_csi_signal_cfg_t csi_param;
	struct video_device *dev = video_devdata(file);
	cam_data *cam = dev->priv;
	int err = 0;

	if (!cam) {
		printk("Internal error, cam_data not found!\n");
		return -ENODEV;
	}

	down(&cam->busy_lock);

	err = 0;
	if (signal_pending(current))
		goto oops;

	if (cam->open_count++ == 0) {
		wait_event_interruptible(cam->power_queue,
					 cam->low_power == false);

#ifdef CONFIG_MXC_IPU_PRP_ENC
		err = prp_enc_select(cam);
#endif

		cam->enc_counter = 0;
		cam->skip_frame = 0;

		ipu_csi_enable_mclk(CSI_MCLK_I2C, true, true);
		param = cam->cam_sensor->reset();
		csi_param.sens_clksrc = 0;
		csi_param.clk_mode = param->clk_mode;
		csi_param.pixclk_pol = param->pixclk_pol;
		csi_param.data_width = param->data_width;
		csi_param.data_pol = param->data_pol;
		csi_param.ext_vsync = param->ext_vsync;
		csi_param.Vsync_pol = param->Vsync_pol;
		csi_param.Hsync_pol = param->Hsync_pol;
		ipu_csi_init_interface(param->width, param->height,
				       param->pixel_fmt, csi_param);

		cam->cam_sensor->get_color(&cam->bright, &cam->saturation,
					   &cam->red, &cam->green, &cam->blue);
		ipu_csi_enable_mclk(CSI_MCLK_I2C, false, false);
	}

	file->private_data = dev;
      oops:
	up(&cam->busy_lock);
	return err;
}

/*!
 * V4L interface - close function
 *
 * @param inode    struct inode *
 * @param file     struct file *
 *
 * @return         0 success
 */
static int mxc_v4l_close(struct inode *inode, struct file *file)
{
	struct video_device *dev = video_devdata(file);
	int err = 0;
	cam_data *cam = dev->priv;

	/* for the case somebody hit the ctrl C */
	if (cam->overlay_pid == current->pid) {
		err = stop_preview(cam);
	}
	if (cam->capture_pid == current->pid) {
		err |= mxc_streamoff(cam);
		wake_up_interruptible(&cam->enc_queue);
	}

	if (--cam->open_count == 0) {
		wait_event_interruptible(cam->power_queue,
					 cam->low_power == false);
		printk("mxc_v4l_close: release resource\n");

#ifdef CONFIG_MXC_IPU_PRP_ENC
		err |= prp_enc_deselect(cam);
#endif
		mxc_free_frame_buf(cam);
		file->private_data = NULL;

		/* capture off */
		wake_up_interruptible(&cam->enc_queue);
		mxc_free_frames(cam);
		cam->enc_counter++;
	}
	//printk("state %x %x %x\n", cam->frame[0].state, cam->frame[1].state,
	//        cam->frame[2].state);
	return err;
}

#ifdef CONFIG_MXC_IPU_PRP_ENC
/*
 * V4L interface - read function
 *
 * @param file       struct file *
 * @param read buf   char *
 * @param count      size_t
 * @param ppos       structure loff_t *
 *
 * @return           bytes read
 */
static ssize_t
mxc_v4l_read(struct file *file, char *buf, size_t count, loff_t * ppos)
{
	int err = 0;
	int temp;
	u8 *v_address;
	struct video_device *dev = video_devdata(file);
	cam_data *cam = dev->priv;

	if (down_interruptible(&cam->busy_lock))
		return 0;

	/* freeze the viewfinder */
	if (cam->overlay_on == true)
		err = stop_preview(cam);

	cam->still_buf = (void *)
	    ipu_malloc(PAGE_ALIGN(cam->v2f.fmt.pix.sizeimage));
	if (!cam->still_buf) {
		printk("mxc_v4l_read failed at allocate still_buf\n");
		goto exit1;
	}

	v_address = (u8 *) ioremap((u32) cam->still_buf,
				   cam->v2f.fmt.pix.sizeimage);
	if (!v_address) {
		ipu_free((u32) cam->still_buf);
		goto exit1;
	}

	err = prp_still_select(cam);
	if (err != 0)
		goto exit;

	cam->still_counter = 0;
	err = cam->csi_start(cam);
	if (err != 0)
		goto exit;

	if (!wait_event_interruptible_timeout(cam->still_queue,
					      cam->still_counter != 0,
					      10 * HZ)) {
		printk("mxc_v4l_read timeout counter %x\n", cam->still_counter);
		goto exit;
	}

      exit:
	prp_still_deselect(cam);

	err = copy_to_user(buf, v_address, cam->v2f.fmt.pix.sizeimage);

	iounmap(v_address);
	ipu_free((u32) cam->still_buf);
	cam->still_buf = NULL;

      exit1:
	if (cam->overlay_on == true) {
		temp = cam->overlay_pid;
		err = start_preview(cam);
		cam->overlay_pid = temp;
	}

	up(&cam->busy_lock);
	if (err != 0)
		return err;

	return cam->v2f.fmt.pix.sizeimage;
}
#endif

/*!
 * V4L interface - ioctl function
 *
 * @param inode      struct inode *
 *
 * @param file       struct file *
 *
 * @param ioctlnr    unsigned int
 *
 * @param arg        void *
 *
 * @return           0 success, ENODEV for invalid device instance,
 *                   -1 for other errors.
 */
static int
mxc_v4l_do_ioctl(struct inode *inode, struct file *file,
		 unsigned int ioctlnr, void *arg)
{
	struct video_device *dev = video_devdata(file);
	cam_data *cam = dev->priv;
	int retval = 0;
	uint32_t lock_flags;

	if (!cam)
		return -ENODEV;

	wait_event_interruptible(cam->power_queue, cam->low_power == false);
	/* make this _really_ smp-safe */
	if (down_interruptible(&cam->busy_lock))
		return -EINTR;

	switch (ioctlnr) {
		/*!
		 * V4l2 VIDIOC_QUERYCAP ioctl
		 */
	case VIDIOC_QUERYCAP:{
			struct v4l2_capability *cap = arg;
			strcpy(cap->driver, "mxc_v4l2");
			cap->version = KERNEL_VERSION(0, 1, 11);
			cap->capabilities = V4L2_CAP_VIDEO_CAPTURE |
			    V4L2_CAP_VIDEO_OVERLAY | V4L2_CAP_STREAMING
			    | V4L2_CAP_READWRITE;
			retval = 0;
			break;
		}

		/*!
		 * V4l2 VIDIOC_G_FMT ioctl
		 */
	case VIDIOC_G_FMT:{
			struct v4l2_format *gf = arg;
			retval = mxc_v4l2_g_fmt(cam, gf);
			break;
		}

		/*!
		 * V4l2 VIDIOC_S_FMT ioctl
		 */
	case VIDIOC_S_FMT:{
			struct v4l2_format *sf = arg;
			retval = mxc_v4l2_s_fmt(cam, sf);
			break;
		}

		/*!
		 * V4l2 VIDIOC_REQBUFS ioctl
		 */
	case VIDIOC_REQBUFS:{
			struct v4l2_requestbuffers *req = arg;
			if (req->count > FRAME_NUM) {
				printk("VIDIOC_REQBUFS: not enough buffer\n");
				req->count = FRAME_NUM;
			}

			if (req->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
				printk("VIDIOC_REQBUFS: wrong buffer type\n");
				retval = -ENOMEM;
				break;
			}

			mxc_streamoff(cam);
			mxc_free_frame_buf(cam);

			retval = mxc_allocate_frame_buf(cam, req->count);
			break;
		}

		/*!
		 * V4l2 VIDIOC_QUERYBUF ioctl
		 */
	case VIDIOC_QUERYBUF:{
			struct v4l2_buffer *buf = arg;
			int index = buf->index;

			if ((buf->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
			    (buf->memory != V4L2_MEMORY_MMAP)) {
				printk("VIDIOC_QUERYBUFS: wrong buffer type\n");
				retval = -EINVAL;
				break;
			}

			memset(buf, 0, sizeof(buf));
			buf->index = index;

			down(&cam->param_lock);
			retval = mxc_v4l2_buffer_status(cam, buf);
			up(&cam->param_lock);
			break;
		}

		/*!
		 * V4l2 VIDIOC_QBUF ioctl
		 */
	case VIDIOC_QBUF:{
			struct v4l2_buffer *buf = arg;
			int index = buf->index;
			//printk("VIDIOC_QBUF: %d\n", buf->index);

			spin_lock_irqsave(&cam->int_lock, lock_flags);
			if ((cam->frame[index].buffer.flags & 0x7) ==
			    V4L2_BUF_FLAG_MAPPED) {
				cam->frame[index].buffer.flags |=
				    V4L2_BUF_FLAG_QUEUED;
				if (cam->skip_frame > 0) {
					cam->skip_frame--;
					retval =
					    cam->enc_update_eba(cam->
								frame[index].
								paddress,
								&cam->
								ping_pong_csi);
				}
			} else if (cam->frame[index].buffer.flags &
				   V4L2_BUF_FLAG_QUEUED) {
				printk("VIDIOC_QBUF: buffer already queued\n");
			} else if (cam->frame[index].buffer.flags &
				   V4L2_BUF_FLAG_DONE) {
				printk("VIDIOC_QBUF: buffer not clear.\n");
				cam->frame[index].buffer.flags &=
				    ~V4L2_BUF_FLAG_DONE;
				cam->frame[index].buffer.flags |=
				    V4L2_BUF_FLAG_QUEUED;
			}
			buf->flags = cam->frame[index].buffer.flags;
			spin_unlock_irqrestore(&cam->int_lock, lock_flags);
			break;
		}

		/*!
		 * V4l2 VIDIOC_DQBUF ioctl
		 */
	case VIDIOC_DQBUF:{
			struct v4l2_buffer *buf = arg;

			if (cam->tailFrame == FRAME_NUM)
				cam->tailFrame = 0;

			//printk("VIDIOC_DQBUF: %d\n", cam->tailFrame);

			retval = mxc_v4l_sync(file, cam->tailFrame);

			if (cam->frame[cam->tailFrame].buffer.flags &
			    V4L2_BUF_FLAG_DONE) {
				cam->frame[cam->tailFrame].buffer.flags &=
				    ~V4L2_BUF_FLAG_DONE;
			} else if (cam->frame[cam->tailFrame].buffer.flags &
				   V4L2_BUF_FLAG_QUEUED) {
				printk("VIDIOC_DQBUF: Buffer %d not filled.\n",
				       cam->tailFrame);
				cam->frame[cam->tailFrame].buffer.flags &=
				    ~V4L2_BUF_FLAG_QUEUED;
				retval = -EIO;
				break;
			} else
			    if ((cam->frame[cam->tailFrame].buffer.flags & 0x7)
				== V4L2_BUF_FLAG_MAPPED) {
				printk("VIDIOC_DQBUF: Buffer not queued.\n");
				retval = -EIO;
				break;
			}
			buf->bytesused = cam->v2f.fmt.pix.sizeimage;
			buf->index = cam->tailFrame++;
			buf->flags = cam->frame[buf->index].buffer.flags;
			break;
		}

		/*!
		 * V4l2 VIDIOC_STREAMON ioctl
		 */
	case VIDIOC_STREAMON:{
			retval = mxc_streamon(cam);
			cam->capture_on = true;
			break;
		}

		/*!
		 * V4l2 VIDIOC_STREAMOFF ioctl
		 */
	case VIDIOC_STREAMOFF:{
			retval = mxc_streamoff(cam);
			cam->capture_on = false;
			break;
		}

		/*!
		 * V4l2 VIDIOC_G_CTRL ioctl
		 */
	case VIDIOC_G_CTRL:{
			retval = mxc_get_v42l_control(cam, arg);
			break;
		}

		/*!
		 * V4l2 VIDIOC_S_CTRL ioctl
		 */
	case VIDIOC_S_CTRL:{
			retval = mxc_set_v42l_control(cam, arg);
			break;
		}

		/*!
		 * V4l2 VIDIOC_CROPCAP ioctl
		 */
	case VIDIOC_CROPCAP:{
			struct v4l2_cropcap *cap = arg;

			if (cap->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
			    cap->type != V4L2_BUF_TYPE_VIDEO_OVERLAY) {
				retval = -EINVAL;
				break;
			}
			cap->bounds = cam->crop_bounds;
			cap->defrect = cam->crop_defrect;
			break;
		}

		/*!
		 * V4l2 VIDIOC_G_CROP ioctl
		 */
	case VIDIOC_G_CROP:{
			struct v4l2_crop *crop = arg;

			if (crop->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
			    crop->type != V4L2_BUF_TYPE_VIDEO_OVERLAY) {
				retval = -EINVAL;
				break;
			}
			crop->c = cam->crop_current;
			break;
		}

		/*!
		 * V4l2 VIDIOC_S_CROP ioctl
		 */
	case VIDIOC_S_CROP:{
			struct v4l2_crop *crop = arg;
			struct v4l2_rect *b = &cam->crop_bounds;

			if (crop->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
			    crop->type != V4L2_BUF_TYPE_VIDEO_OVERLAY) {
				retval = -EINVAL;
				break;
			}

			crop->c.top = (crop->c.top < b->top) ? b->top
			    : crop->c.top;
			if (crop->c.top > b->top + b->height)
				crop->c.top = b->top + b->height - 1;
			if (crop->c.height > b->top + b->height - crop->c.top)
				crop->c.height =
				    b->top + b->height - crop->c.top;

			crop->c.left = (crop->c.left < b->left) ? b->left
			    : crop->c.left;
			if (crop->c.left > b->left + b->width)
				crop->c.left = b->left + b->width - 1;
			if (crop->c.width > b->left - crop->c.left + b->width)
				crop->c.width =
				    b->left - crop->c.left + b->width;

			crop->c.width -= crop->c.width % 8;
			crop->c.left -= crop->c.left % 4;
			cam->crop_current = crop->c;

			ipu_csi_set_window_size(cam->crop_current.width,
						cam->crop_current.height);
			ipu_csi_set_window_pos(cam->crop_current.left,
					       cam->crop_current.top);
			break;
		}

		/*!
		 * V4l2 VIDIOC_OVERLAY ioctl
		 */
	case VIDIOC_OVERLAY:{
			int *on = arg;
			if (*on) {
				cam->overlay_on = true;
				retval = start_preview(cam);
			}
			if (!*on) {
				retval = stop_preview(cam);
				cam->overlay_on = false;
			}
			break;
		}

		/*!
		 * V4l2 VIDIOC_G_FBUF ioctl
		 */
	case VIDIOC_G_FBUF:{
			struct v4l2_framebuffer *fb = arg;
			*fb = cam->v4l2_fb;
			fb->capability = V4L2_FBUF_CAP_EXTERNOVERLAY;
			break;
		}

		/*!
		 * V4l2 VIDIOC_S_FBUF ioctl
		 */
	case VIDIOC_S_FBUF:{
			struct v4l2_framebuffer *fb = arg;
			cam->v4l2_fb = *fb;
			break;
		}

	case VIDIOC_G_PARM:{
			struct v4l2_streamparm *parm = arg;
			if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
				printk("VIDIOC_G_PARM invalid type\n");
				retval = -EINVAL;
				break;
			}
			parm->parm.capture = cam->streamparm.parm.capture;
			break;
		}
	case VIDIOC_S_PARM:{
			struct v4l2_streamparm *parm = arg;
			retval = mxc_v4l2_s_param(cam, parm);
			break;
		}

		/* linux v4l2 bug, kernel c0485619 user c0405619 */
	case VIDIOC_ENUMSTD:{
			struct v4l2_standard *e = arg;
			*e = cam->standard;
			printk("VIDIOC_ENUMSTD call\n");
			retval = 0;
			break;
		}

	case VIDIOC_G_STD:{
			v4l2_std_id *e = arg;
			*e = cam->standard.id;
			break;
		}

	case VIDIOC_S_STD:{
			break;
		}

	case VIDIOC_ENUMOUTPUT:
		{
			struct v4l2_output *output = arg;

			if (output->index >= MXC_V4L2_CAPTURE_NUM_OUTPUTS) {
				retval = -EINVAL;
				break;
			}

			*output = mxc_capture_outputs[output->index];

			break;
		}
	case VIDIOC_G_OUTPUT:
		{
			int *p_output_num = arg;

			*p_output_num = cam->output;
			break;
		}
	case VIDIOC_S_OUTPUT:
		{
			int *p_output_num = arg;

			if (*p_output_num >= MXC_V4L2_CAPTURE_NUM_OUTPUTS) {
				retval = -EINVAL;
				break;
			}

			cam->output = *p_output_num;
			break;
		}

	case VIDIOC_ENUM_FMT:
	case VIDIOC_TRY_FMT:
	case VIDIOC_QUERYCTRL:
	case VIDIOC_ENUMINPUT:
	case VIDIOC_G_INPUT:
	case VIDIOC_S_INPUT:
	case VIDIOC_G_TUNER:
	case VIDIOC_S_TUNER:
	case VIDIOC_G_FREQUENCY:
	case VIDIOC_S_FREQUENCY:
	default:
		retval = -EINVAL;
		break;
	}

	up(&cam->busy_lock);
	return retval;
}

/*
 * V4L interface - ioctl function
 *
 * @return  None
 */
static int
mxc_v4l_ioctl(struct inode *inode, struct file *file,
	      unsigned int cmd, unsigned long arg)
{
	return video_usercopy(inode, file, cmd, arg, mxc_v4l_do_ioctl);
}

/*!
 * V4L interface - mmap function
 *
 * @param file        structure file *
 *
 * @param vma         structure vm_area_struct *
 *
 * @return status     0 Success, EINTR busy lock error, ENOBUFS remap_page error
 */
static int mxc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct video_device *dev = video_devdata(file);
	unsigned long size;
	int res = 0;
	cam_data *cam = dev->priv;

	DPRINTK("pgoff=0x%x, start=0x%x, end=0x%x\n",
		vma->vm_pgoff, vma->vm_start, vma->vm_end);

	/* make this _really_ smp-safe */
	if (down_interruptible(&cam->busy_lock))
		return -EINTR;

	size = vma->vm_end - vma->vm_start;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (remap_pfn_range(vma, vma->vm_start,
		vma->vm_pgoff, size, vma->vm_page_prot)) {
		printk("mxc_mmap: remap_pfn_range failed\n");
		res = -ENOBUFS;
		goto mxc_mmap_exit;
	}

	vma->vm_flags &= ~VM_IO; /* using shared anonymous pages */

mxc_mmap_exit:
	up(&cam->busy_lock);
	return res;
}

/*!
 * V4L interface - poll function
 *
 * @param file       structure file *
 *
 * @param wait       structure poll_table *
 *
 * @return  status   POLLIN | POLLRDNORM
 */
static unsigned int mxc_poll(struct file *file, poll_table * wait)
{
	struct video_device *dev = video_devdata(file);
	cam_data *cam = dev->priv;
	wait_queue_head_t *queue = NULL;
	int res = POLLIN | POLLRDNORM;

	if (down_interruptible(&cam->busy_lock))
		return -EINTR;

	queue = &cam->enc_queue;
	poll_wait(file, queue, wait);

	up(&cam->busy_lock);
	return res;
}

static struct
file_operations mxc_v4l_fops = {
	.owner = THIS_MODULE,
	.open = mxc_v4l_open,
	.release = mxc_v4l_close,
	.read = mxc_v4l_read,
	.ioctl = mxc_v4l_ioctl,
	.mmap = mxc_mmap,
	.poll = mxc_poll,
};

static struct video_device mxc_v4l_template = {
	.owner = THIS_MODULE,
	.name = "Mxc Camera",
	.type = 0,
	.type2 = VID_TYPE_CAPTURE,
	.hardware = 39,
	.fops = &mxc_v4l_fops,
	.release = video_device_release,
};

extern struct camera_sensor camera_sensor_if;

/*!
 * initialize cam_data structure
 *
 * @param cam      structure cam_data *
 *
 * @return status  0 Success
 */
static void init_camera_struct(cam_data * cam)
{
	int i;

	/* Default everything to 0 */
	memset(cam, 0, sizeof(cam_data));

	init_MUTEX(&cam->param_lock);
	init_MUTEX(&cam->busy_lock);

	cam->video_dev = video_device_alloc();
	if (cam->video_dev == NULL)
		return;

	*(cam->video_dev) = mxc_v4l_template;

	video_set_drvdata(cam->video_dev, cam);

	for (i = 0; i < FRAME_NUM; i++) {
		cam->frame[i].width = 0;
		cam->frame[i].height = 0;
		cam->frame[i].paddress = 0;
	}

	init_waitqueue_head(&cam->enc_queue);
	init_waitqueue_head(&cam->still_queue);

	cam->headFrame = cam->tailFrame = 0;

	/* setup cropping */
	cam->crop_bounds.left = 0;
	cam->crop_bounds.width = 640;
	cam->crop_bounds.top = 0;
	cam->crop_bounds.height = 480;
	cam->crop_current = cam->crop_defrect = cam->crop_bounds;
	ipu_csi_set_window_size(cam->crop_current.width,
				cam->crop_current.height);
	ipu_csi_set_window_pos(cam->crop_current.left, cam->crop_current.top);
    cam->streamparm.parm.capture.capturemode = 0;

	cam->standard.index = 0;
	cam->standard.id = V4L2_STD_UNKNOWN;
	cam->standard.frameperiod.denominator = 30;
	cam->standard.frameperiod.numerator = 1;
	cam->standard.framelines = 480;
	cam->streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cam->streamparm.parm.capture.timeperframe = cam->standard.frameperiod;
	cam->streamparm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	cam->overlay_on = false;
	cam->capture_on = false;
	cam->skip_frame = 0;
	cam->v4l2_fb.flags = V4L2_FBUF_FLAG_OVERLAY;

	cam->v2f.fmt.pix.sizeimage = 352 * 288 * 3 / 2;
	cam->v2f.fmt.pix.bytesperline = 288 * 3 / 2;
	cam->v2f.fmt.pix.width = 288;
	cam->v2f.fmt.pix.height = 352;
	cam->v2f.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	cam->win.w.width = 160;
	cam->win.w.height = 160;
	cam->win.w.left = 0;
	cam->win.w.top = 0;

	cam->cam_sensor = &camera_sensor_if;
}

/*!
 * mxc_v4l_register_camera
 *
 * @return  pointer to structure cam_data
 */
static cam_data *mxc_v4l_register_camera(void)
{
	cam_data *cam;

	if ((cam = kmalloc(sizeof(cam_data), GFP_KERNEL)) == NULL)
		return NULL;

	init_camera_struct(cam);

	/* register v4l device */
	if (video_register_device(cam->video_dev, VFL_TYPE_GRABBER, video_nr)
	    == -1) {
		kfree(cam);
		printk(KERN_DEBUG "video_register_device failed\n");
		return NULL;
	}

	return cam;
}

/*!
 * mxc_v4l_unregister_camera
 *
 * @param cam         structure cam_data *
 *
 * @return  none
 */
static void mxc_v4l_unregister_camera(cam_data * cam)
{
	printk("unregistering video\n");
	video_unregister_device(cam->video_dev);
	if (cam->open_count) {
		printk("camera open -- setting ops to NULL\n");
	}

	if (!cam->open_count) {
		printk("freeing camera\n");
		mxc_free_frame_buf(cam);
		kfree(cam);
	}
}

/*!
* Camera V4l2 callback function.
*
* @return status
*/
void camera_callback(u32 mask, void *dev)
{
	int temp_frame;

	cam_data *cam = (cam_data *) dev;
	if (cam == NULL)
		return;

	if (cam->frame[cam->headFrame].buffer.flags & V4L2_BUF_FLAG_QUEUED) {
		cam->frame[cam->headFrame].buffer.flags |= V4L2_BUF_FLAG_DONE;
		cam->frame[cam->headFrame].buffer.flags &=
		    ~V4L2_BUF_FLAG_QUEUED;

		temp_frame = cam->headFrame + 2;
		if (temp_frame >= FRAME_NUM) {
			temp_frame -= FRAME_NUM;
		}

		if (cam->frame[temp_frame].buffer.flags & V4L2_BUF_FLAG_QUEUED) {
			cam->enc_update_eba(cam->frame[temp_frame].paddress,
					    &cam->ping_pong_csi);
		} else {
			cam->skip_frame++;
		}

		if (++cam->headFrame == FRAME_NUM) {
			cam->headFrame = 0;
		}

		/* Wake up the queue */
		cam->enc_counter++;
		wake_up_interruptible(&cam->enc_queue);
	} else {
		printk("camera_callback :we are not supposed to be here.\n");
	}
}

/*!
 * camera_power function
 *    Turn Sensor power On/Off
 *
 * @param       cameraOn      true to turn camera on, otherwise shut down
 *
 * @return status
 */
static u8 camera_power(bool cameraOn)
{
	if (cameraOn == true) {
		ipu_csi_enable_mclk(csi_mclk_flag_backup, true, true);
	} else {
		csi_mclk_flag_backup = ipu_csi_read_mclk_flag();
		ipu_csi_enable_mclk(csi_mclk_flag_backup, false, false);
	}
	return 0;
}

/*!
 * This function is called to put the sensor in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device structure used to give information on which I2C
 *                to suspend
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function returns 0 on success and -1 on failure.
 */
static int mxc_v4l2_suspend(struct device *dev, u32 state, u32 level)
{
	cam_data *cam = dev_get_drvdata(dev);

	if (cam == NULL) {
		return -1;
	}

	if (level == SUSPEND_DISABLE) {
		cam->low_power = true;
	} else if (level == SUSPEND_POWER_DOWN) {
		if (cam->overlay_on == true)
			stop_preview(cam);
		if ((cam->capture_on == true) && cam->enc_disable) {
			cam->enc_disable(cam);
		}
		camera_power(false);
	}
	return 0;
}

/*!
 * This function is called to bring the sensor back from a low power state.Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device structure
 * @param   level the stage in device resumption process that we want the
 *                device to be put in
 *
 * @return  The function returns 0 on success and -1 on failure
 */
static int mxc_v4l2_resume(struct device *dev, u32 level)
{
	cam_data *cam = dev_get_drvdata(dev);

	if (cam == NULL) {
		return -1;
	}

	if (level == RESUME_ENABLE) {
		cam->low_power = false;
		wake_up_interruptible(&cam->power_queue);
	} else if (level == RESUME_POWER_ON) {
		if (cam->overlay_on == true)
			start_preview(cam);
		if (cam->capture_on == true)
			mxc_streamon(cam);
		camera_power(true);
	}
	return 0;
}

/*!
 * This function is called during the driver binding process.
 *
 * @param   dev   the device structure used to store device specific
 *                information that is used by the suspend, resume and remove
 *                functions
 *
 * @return  The function always returns 0.
 */
static int mxc_v4l2_probe(struct device *dev)
{
	cam_data *cam;
	cam = mxc_v4l_register_camera();
	if (cam == NULL) {
		printk("failed to mxc_v4l_register_camera\n");
		return -1;
	}

	dev_set_drvdata(dev, cam);

	init_waitqueue_head(&cam->power_queue);
	cam->int_lock = SPIN_LOCK_UNLOCKED;
	spin_lock_init(&cam->int_lock);

	return 0;
}

/*!
 * Dissociates the driver.
 *
 * @param   dev   the device structure
 *
 * @return  The function always returns 0.
 */
static int mxc_v4l2_remove(struct device *dev)
{
	cam_data *cam = dev_get_drvdata(dev);

	if (cam == NULL) {
		return -1;
	}

	mxc_v4l_unregister_camera(cam);

	dev_set_drvdata(dev, NULL);
	return 0;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxc_v4l2_driver = {
	.name = "mxc_v4l2",
	.bus = &platform_bus_type,
	.probe = mxc_v4l2_probe,
	.remove = mxc_v4l2_remove,
	.suspend = mxc_v4l2_suspend,
	.resume = mxc_v4l2_resume,
};

/*! Device Definition for Mt9v111 devices */
static struct platform_device mxc_v4l2_devices = {
	.name = "mxc_v4l2",
	.id = 0,
};

/*!
 * Entry point for the V4L2
 *
 * @return  Error code indicating success or failure
 */
int camera_init(void)
{
	u8 err = 0;
	FUNC_START;

	/* Register the device driver structure. */
	err = driver_register(&mxc_v4l2_driver);
	if (err != 0) {
		printk("camera_init: driver_register failed.\n");
		return err;
	}

	/* Register the I2C device */
	err = platform_device_register(&mxc_v4l2_devices);
	if (err != 0) {
		printk("camera_init: platform_device_register failed.\n");
		driver_unregister(&mxc_v4l2_driver);
	}

	FUNC_END;

	return err;
}

/*!
 * Exit and cleanup for the V4L2
 *
 */
void camera_exit(void)
{
	FUNC_START;

	driver_unregister(&mxc_v4l2_driver);
	platform_device_unregister(&mxc_v4l2_devices);

	FUNC_END;
}

/*!
 * mxc v4l2 init function
 *
 */
static __init int mxc_v4l2_init(void)
{
	camera_init();
	return 0;
}

/*!
 * mxc v4l2 cleanup function
 *
 */
static void __exit mxc_v4l2_clean(void)
{
	camera_exit();
}

module_init(mxc_v4l2_init);
module_exit(mxc_v4l2_clean);

/* Exported symbols for modules. */
EXPORT_SYMBOL(camera_callback);

MODULE_PARM(video_nr, "i");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("V4L2 capture driver for Mxc based cameras");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("video");
