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
 * @file mxc_v4l2_output.c
 *
 * @brief MXC V4L2 Video Output Driver
 *
 * Video4Linux2 Output Device using MXC IPU Post-processing functionality.
 *
 * @ingroup MXC_V4L2_OUTPUT
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/semaphore.h>
#include "asm/arch/board.h"
#include "mxc_v4l2_output.h"

#include "../drivers/video/mxc/mxcfb.h"

#define SDC_FG_FB_FORMAT        IPU_PIX_FMT_RGB565

struct v4l2_output mxc_outputs[2] = {
	{
	 .index = MXC_V4L2_OUT_2_SDC,
	 .name = "DISP3 Video Out",
	 .type = V4L2_OUTPUT_TYPE_ANALOG,	/* not really correct,
						   but no other choice */
	 .audioset = 0,
	 .modulator = 0,
	 .std = V4L2_STD_UNKNOWN},
	{
	 .index = MXC_V4L2_OUT_2_ADC,
	 .name = "DISPx Video Out",
	 .type = V4L2_OUTPUT_TYPE_ANALOG,	/* not really correct,
						   but no other choice */
	 .audioset = 0,
	 .modulator = 0,
	 .std = V4L2_STD_UNKNOWN}
};

#undef MXC_V4L2OUT_DEBUG
#ifdef MXC_V4L2OUT_DEBUG

#define DDPRINTK(fmt, args...)  printk(KERN_ERR"%s :: %d :: %s - " fmt, \
                                __FILE__,__LINE__,__FUNCTION__ , ## args)
#define DPRINTK(fmt, args...)   printk("%s: " fmt, __FUNCTION__ , ## args)

#define FUNC_START              DDPRINTK(" func start\n")
#define FUNC_END                DDPRINTK(" func end\n")

#define FUNC_ERR                printk(KERN_ERR"%s :: %d :: %s  err= %d \n", \
                                __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* MXC_V4L2OUT_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while (0)
#define DPRINTK(fmt, args...)   do {} while (0)

#define FUNC_START
#define FUNC_END

#endif				/* MXC_V4L2OUT_DEBUG */

static int video_nr = 16;
static spinlock_t g_lock = SPIN_LOCK_UNLOCKED;

/* debug counters */
uint32_t g_irq_cnt;
uint32_t g_buf_output_cnt;
uint32_t g_buf_q_cnt;
uint32_t g_buf_dq_cnt;

#define QUEUE_SIZE (MAX_FRAME_NUM + 1)
static __inline int queue_size(v4l_queue * q)
{
	if (q->tail >= q->head)
		return (q->tail - q->head);
	else
		return ((q->tail + QUEUE_SIZE) - q->head);
}

static __inline int queue_buf(v4l_queue * q, int idx)
{
	if (((q->tail + 1) % QUEUE_SIZE) == q->head)
		return -1;	/* queue full */
	q->list[q->tail] = idx;
	q->tail = (q->tail + 1) % QUEUE_SIZE;
	return 0;
}

static __inline int dequeue_buf(v4l_queue * q)
{
	int ret;
	if (q->tail == q->head)
		return -1;	/* queue empty */
	ret = q->list[q->head];
	q->head = (q->head + 1) % QUEUE_SIZE;
	return ret;
}

static __inline int peek_next_buf(v4l_queue * q)
{
	if (q->tail == q->head)
		return -1;	/* queue empty */
	return q->list[q->head];
}

/*!
 * Private function to allocate IPU buffers
 *
 * @param bufs          Output array of pointers to buffers allocated
 *
 * @param num_buf       Input number of buffers to allocate
 *
 * @param size          Input size for each buffer to allocate
 *
 * @return status  -0 Successfully allocated a buffer, -ENOBUFS failed.
 */
static int mxc_allocate_buffers(void *bufs[], int num_buf, int size)
{
	int i;

	for (i = 0; i < num_buf; i++) {
		bufs[i] = (void *)ipu_malloc(size);
		if (bufs[i] == 0) {
			DPRINTK("ipu_malloc failed.\n");
			return -ENOBUFS;
		}
		DPRINTK("allocated @ paddr=0x%08X, size=%d.\n",
			(u32) bufs[i], size);
	}

	return 0;
}

/*!
 * Private function to free IPU buffers
 *
 * @param bufs      void *
 *
 * @param num_buf   int
 *
 * @return status  0 success.
 */
static int mxc_free_buffers(void *bufs[], int num_buf)
{
	int i;

	for (i = 0; i < num_buf; i++) {
		if (bufs[i] != 0) {
			ipu_free((u32) bufs[i]);
			DPRINTK("freed @ paddr=0x%08X\n", (u32) bufs[i]);
			bufs[i] = NULL;
		}
	}
	return 0;
}

static void mxc_v4l2out_timer_handler(unsigned long arg)
{
	int index;
	unsigned long timeout;
	vout_data *vout = (vout_data *) arg;

	/* DPRINTK("timer handler:\n"); */

	/* If timer occurs before IPU h/w is ready, then set the state to
	   paused and the timer will be set again when next buffer is queued. */
	if (vout->ipu_buf[vout->next_rdy_ipu_buf] != -1) {
		DPRINTK("IPU buffer busy\n");
		vout->state = STATE_STREAM_PAUSED;
		return;
	}

	/* Dequeue buffer and pass to IPU */
	index = dequeue_buf(&vout->ready_q);
	if (index == -1) {	/* no buffers ready, should never occur */
		printk("mxc_v4l2out: timer - no queued buffers ready\n");
		return;
	}
	g_buf_dq_cnt++;
	vout->ipu_buf[vout->next_rdy_ipu_buf] = index;
	if (ipu_update_channel_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER,
				      vout->next_rdy_ipu_buf,
				      vout->queue_buf_paddr[index]) < 0) {
		DPRINTK("unable to update buffer %d address\n",
			vout->next_rdy_ipu_buf);
		return;
	}
	if (ipu_select_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER,
			      vout->next_rdy_ipu_buf) < 0) {
		DPRINTK("unable to set IPU buffer ready\n");
	}
	vout->next_rdy_ipu_buf = !vout->next_rdy_ipu_buf;

	/* Setup timer for next buffer */
	index = peek_next_buf(&vout->ready_q);
	if (index != -1) {
		timeout = timeval_to_jiffies(&vout->v4l2_bufs[index].timestamp);
		if (!timeout) {
			/* if timestamp is 0, then default to 30fps */
			timeout = vout->start_jiffies +
			    msecs_to_jiffies(vout->frame_count * 33);
		} else {	/* Adjust time from time of day to jiffies */
			timeout -= vout->start_tod_jiffies;
		}
		if (mod_timer(&vout->output_timer, timeout))
			DPRINTK("warning: timer was already set\n");

		vout->frame_count++;
	} else {
		vout->state = STATE_STREAM_PAUSED;
	}
}

static irqreturn_t mxc_v4l2out_pp_in_irq_handler(int irq, void *dev_id,
						 struct pt_regs *regs)
{
	int last_buf;
	vout_data *vout = dev_id;

	g_irq_cnt++;

/*
        printk("pp_irq %d, IPU_CUR_BUF=0x%08X, IPU_TASK_STAT=0x%08X, \
        IPU_INT_STAT5=0x%08X\n", vout->next_done_ipu_buf, *((u32*)0xD42C0010),
        *((u32*)0xD42C001C), *((u32*)0xD42C004C));
*/
	/* Process previous buffer */
	last_buf = vout->ipu_buf[vout->next_done_ipu_buf];
	if (last_buf != -1) {
		g_buf_output_cnt++;
		vout->v4l2_bufs[last_buf].flags = V4L2_BUF_FLAG_DONE;
		queue_buf(&vout->done_q, last_buf);
		vout->ipu_buf[vout->next_done_ipu_buf] = -1;
		wake_up_interruptible(&vout->v4l_bufq);
		/* printk("pp_irq: buf %d done\n", vout->next_done_ipu_buf); */
		vout->next_done_ipu_buf = !vout->next_done_ipu_buf;
	}

	if (vout->state == STATE_STREAM_STOPPING) {
		if ((vout->ipu_buf[0] == -1) && (vout->ipu_buf[1] == -1)) {
			vout->state = STATE_STREAM_OFF;
		}
	}
	return IRQ_HANDLED;
}

/*!
 * Start the output stream
 *
 * @param vout      structure vout_data *
 *
 * @return status  0 Success
 */
static int mxc_v4l2out_streamon(vout_data * vout)
{
	ipu_channel_params_t params;
	int pp_in_buf[2];
	u16 out_width;
	u16 out_height;
	ipu_channel_t display_input_ch = MEM_PP_MEM;
	bool use_direct_adc = false;

	if (!vout)
		return -EINVAL;

	if (queue_size(&vout->ready_q) < 2) {
		DPRINTK("2 buffers not been queued yet!\n");
		return -EINVAL;
	}

	out_width = vout->crop_current.width;
	out_height = vout->crop_current.height;

	vout->next_done_ipu_buf = vout->next_rdy_ipu_buf = 0;
	vout->ipu_buf[0] = pp_in_buf[0] = dequeue_buf(&vout->ready_q);
	vout->ipu_buf[1] = pp_in_buf[1] = dequeue_buf(&vout->ready_q);

	/* Init Display Channel */
#ifdef CONFIG_FB_MXC_ASYNC_PANEL
	if (vout->cur_disp_output != DISP3) {
		int fbnum = vout->output_fb_num[vout->cur_disp_output];
		mxcfb_set_refresh_mode(registered_fb[fbnum],
				       MXCFB_REFRESH_OFF, 0);
		if (vout->rotate < IPU_ROTATE_90_RIGHT) {
			DPRINTK("Using PP direct to ADC channel\n");
			use_direct_adc = true;
			vout->display_ch = MEM_PP_ADC;
			vout->post_proc_ch = MEM_PP_ADC;

			memset(&params, 0, sizeof(params));
			params.mem_pp_adc.in_width = vout->v2f.fmt.pix.width;
			params.mem_pp_adc.in_height = vout->v2f.fmt.pix.height;
			params.mem_pp_adc.in_pixel_fmt =
			    vout->v2f.fmt.pix.pixelformat;
			params.mem_pp_adc.out_width = out_width;
			params.mem_pp_adc.out_height = out_height;
			params.mem_pp_adc.out_pixel_fmt = SDC_FG_FB_FORMAT;
#ifdef CONFIG_FB_MXC_EPSON_PANEL
			params.mem_pp_adc.out_left = 2 +
			    vout->crop_current.left;
#else
			params.mem_pp_adc.out_left = 12 +
			    vout->crop_current.left;
#endif
			params.mem_pp_adc.out_top = vout->crop_current.top;
			if (ipu_init_channel(vout->post_proc_ch, &params) != 0) {
				DPRINTK(KERN_ERR
					"Error initializing PP chan\n");
				return -EINVAL;
			}

			if (ipu_init_channel_buffer(vout->post_proc_ch,
						    IPU_INPUT_BUFFER,
						    params.mem_pp_adc.
						    in_pixel_fmt,
						    params.mem_pp_adc.in_width,
						    params.mem_pp_adc.in_height,
						    params.mem_pp_adc.in_width,
						    vout->rotate,
						    vout->
						    queue_buf_paddr[pp_in_buf
								    [0]],
						    vout->
						    queue_buf_paddr[pp_in_buf
								    [1]]) !=
			    0) {
				DPRINTK(KERN_ERR "Error initializing PP "
					"in buf\n");
				return -EINVAL;
			}
			if (ipu_init_channel_buffer(vout->post_proc_ch,
						    IPU_OUTPUT_BUFFER,
						    params.mem_pp_adc.
						    out_pixel_fmt, out_width,
						    out_height, out_width,
						    vout->rotate, NULL,
						    NULL) != 0) {
				DPRINTK(KERN_ERR "Error initializing PP "
					"output buffer\n");
				return -EINVAL;
			}

		} else {
			DPRINTK("Using ADC SYS2 channel\n");
			vout->display_ch = ADC_SYS2;
			vout->post_proc_ch = MEM_PP_MEM;
			memset(&params, 0, sizeof(params));
			params.adc_sys2.disp = vout->cur_disp_output;
			params.adc_sys2.ch_mode = WriteTemplateNonSeq;
#ifdef CONFIG_FB_MXC_EPSON_PANEL
			params.adc_sys2.out_left = 2 + vout->crop_current.left;
#else
			params.adc_sys2.out_left = 12 + vout->crop_current.left;
#endif
			params.adc_sys2.out_top = vout->crop_current.top;
			if (ipu_init_channel(ADC_SYS2, &params) < 0)
				return -EINVAL;

			if (ipu_init_channel_buffer(vout->display_ch,
						    IPU_INPUT_BUFFER,
						    SDC_FG_FB_FORMAT,
						    out_width, out_height,
						    out_width,
						    IPU_ROTATE_NONE,
						    vout->display_bufs[0],
						    vout->display_bufs[1]) !=
			    0) {
				DPRINTK(KERN_ERR "Error initializing SDC FG "
					"buffer\n");
				return -EINVAL;
			}
		}
	} else
#endif
	{			/* Use SDC */
		DPRINTK("Using SDC channel\n");
		vout->display_ch = MEM_SDC_FG;
		vout->post_proc_ch = MEM_PP_MEM;
		ipu_init_channel(MEM_SDC_FG, NULL);
		ipu_sdc_set_window_pos(MEM_SDC_FG, vout->crop_current.left,
				       vout->crop_current.top);
		if (ipu_init_channel_buffer(vout->display_ch, IPU_INPUT_BUFFER,
					    SDC_FG_FB_FORMAT,
					    out_width, out_height, out_width,
					    IPU_ROTATE_NONE,
					    vout->display_bufs[0],
					    vout->display_bufs[1]) != 0) {
			DPRINTK(KERN_ERR "Error initializing SDC FG buffer\n");
			return -EINVAL;
		}
	}

	/* Init PP */
	if (use_direct_adc == false) {
		if (vout->rotate >= IPU_ROTATE_90_RIGHT) {
			out_width = vout->crop_current.height;
			out_height = vout->crop_current.width;
		}
		memset(&params, 0, sizeof(params));
		params.mem_pp_mem.in_width = vout->v2f.fmt.pix.width;
		params.mem_pp_mem.in_height = vout->v2f.fmt.pix.height;
		params.mem_pp_mem.in_pixel_fmt = vout->v2f.fmt.pix.pixelformat;
		params.mem_pp_mem.out_width = out_width;
		params.mem_pp_mem.out_height = out_height;
		params.mem_pp_mem.out_pixel_fmt = SDC_FG_FB_FORMAT;
		if (ipu_init_channel(vout->post_proc_ch, &params) != 0) {
			DPRINTK(KERN_ERR "Error initializing PP channel\n");
			return -EINVAL;
		}

		if (ipu_init_channel_buffer(vout->post_proc_ch,
					    IPU_INPUT_BUFFER,
					    params.mem_pp_mem.in_pixel_fmt,
					    params.mem_pp_mem.in_width,
					    params.mem_pp_mem.in_height,
					    params.mem_pp_mem.in_width,
					    IPU_ROTATE_NONE,
					    vout->queue_buf_paddr[pp_in_buf[0]],
					    vout->
					    queue_buf_paddr[pp_in_buf[1]]) !=
		    0) {
			DPRINTK(KERN_ERR
				"Error initializing PP input buffer\n");
			return -EINVAL;
		}

		if (vout->rotate >= IPU_ROTATE_90_RIGHT) {
			if (vout->rot_pp_bufs[0]) {
				mxc_free_buffers(vout->rot_pp_bufs, 2);
			}
			if (mxc_allocate_buffers(vout->rot_pp_bufs, 2,
						 vout->sdc_fg_buf_size) < 0) {
				return -ENOBUFS;
			}

			if (ipu_init_channel_buffer(vout->post_proc_ch,
						    IPU_OUTPUT_BUFFER,
						    params.mem_pp_mem.
						    out_pixel_fmt, out_width,
						    out_height, out_width,
						    IPU_ROTATE_NONE,
						    vout->rot_pp_bufs[0],
						    vout->rot_pp_bufs[1]) !=
			    0) {
				DPRINTK(KERN_ERR "Error initializing PP "
					"output buffer\n");
				return -EINVAL;
			}

			if (ipu_init_channel(MEM_ROT_PP_MEM, NULL) != 0) {
				DPRINTK(KERN_ERR "Error initializing PP ROT "
					"channel\n");
				return -EINVAL;
			}

			if (ipu_init_channel_buffer(MEM_ROT_PP_MEM,
						    IPU_INPUT_BUFFER,
						    params.mem_pp_mem.
						    out_pixel_fmt, out_width,
						    out_height, out_width,
						    vout->rotate,
						    vout->rot_pp_bufs[0],
						    vout->rot_pp_bufs[1]) !=
			    0) {
				DPRINTK(KERN_ERR "Error initializing PP ROT "
					"input buffer\n");
				return -EINVAL;
			}

			/* swap width and height */
			out_width = vout->crop_current.width;
			out_height = vout->crop_current.height;

			if (ipu_init_channel_buffer(MEM_ROT_PP_MEM,
						    IPU_OUTPUT_BUFFER,
						    params.mem_pp_mem.
						    out_pixel_fmt, out_width,
						    out_height, out_width,
						    IPU_ROTATE_NONE,
						    vout->display_bufs[0],
						    vout->display_bufs[1]) !=
			    0) {
				DPRINTK(KERN_ERR "Error initializing PP "
					"output buffer\n");
				return -EINVAL;
			}

			if (ipu_link_channels(vout->post_proc_ch,
					      MEM_ROT_PP_MEM) < 0) {
				return -EINVAL;
			}
			ipu_select_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, 0);
			ipu_select_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, 1);

			ipu_enable_channel(MEM_ROT_PP_MEM);

			display_input_ch = MEM_ROT_PP_MEM;
		} else {
			if (ipu_init_channel_buffer(vout->post_proc_ch,
						    IPU_OUTPUT_BUFFER,
						    params.mem_pp_mem.
						    out_pixel_fmt, out_width,
						    out_height, out_width,
						    vout->rotate,
						    vout->display_bufs[0],
						    vout->display_bufs[1]) !=
			    0) {
				DPRINTK(KERN_ERR "Error initializing PP "
					"output buffer\n");
				return -EINVAL;
			}
		}
		if (ipu_link_channels(display_input_ch, vout->display_ch) < 0) {
			DPRINTK(KERN_ERR "Error linking ipu channels\n");
			return -EINVAL;
		}
	}

	vout->state = STATE_STREAM_PAUSED;

	ipu_select_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER, 0);
	ipu_select_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER, 1);

	if (use_direct_adc == false) {
		ipu_select_buffer(vout->post_proc_ch, IPU_OUTPUT_BUFFER, 0);
		ipu_select_buffer(vout->post_proc_ch, IPU_OUTPUT_BUFFER, 1);

		ipu_enable_channel(vout->post_proc_ch);
		ipu_enable_channel(vout->display_ch);
	} else {
		ipu_enable_channel(vout->post_proc_ch);
	}

	return 0;
}

/*!
 * Shut down the voutera
 *
 * @param vout      structure vout_data *
 *
 * @return status  0 Success
 */
static int mxc_v4l2out_streamoff(vout_data * vout)
{
	int retval = 0;
	u32 lockflag = 0;

	if (!vout)
		return -EINVAL;

	if (vout->state == STATE_STREAM_OFF) {
		return 0;
	}

	spin_lock_irqsave(&g_lock, lockflag);

	del_timer(&vout->output_timer);

	if (vout->state == STATE_STREAM_ON) {
		vout->state = STATE_STREAM_STOPPING;
	}

	ipu_disable_irq(IPU_IRQ_PP_IN_EOF);

	spin_unlock_irqrestore(&g_lock, lockflag);

	if (vout->post_proc_ch == MEM_PP_MEM) {	/* SDC or ADC with Rotation */
		ipu_disable_channel(MEM_PP_MEM, true);
		if (vout->rotate >= IPU_ROTATE_90_RIGHT) {
			ipu_disable_channel(MEM_ROT_PP_MEM, true);
			ipu_unlink_channels(MEM_PP_MEM, MEM_ROT_PP_MEM);
			ipu_unlink_channels(MEM_ROT_PP_MEM, vout->display_ch);
		} else {
			ipu_unlink_channels(MEM_PP_MEM, vout->display_ch);
		}
		ipu_disable_channel(vout->display_ch, true);
		ipu_uninit_channel(vout->display_ch);
		ipu_uninit_channel(MEM_PP_MEM);
		ipu_uninit_channel(MEM_ROT_PP_MEM);
	} else {		/* ADC Direct */
		ipu_disable_channel(MEM_PP_ADC, true);
		ipu_uninit_channel(MEM_PP_ADC);
	}
	vout->ready_q.head = vout->ready_q.tail = 0;

	vout->state = STATE_STREAM_OFF;

#ifdef CONFIG_FB_MXC_ASYNC_PANEL
	if (vout->cur_disp_output != DISP3) {
		mxcfb_set_refresh_mode(registered_fb
				       [vout->
					output_fb_num[vout->cur_disp_output]],
				       MXCFB_REFRESH_PARTIAL, 0);
	}
#endif

	return retval;
}

/*
 * Valid whether the palette is supported
 *
 * @param palette  V4L2_PIX_FMT_RGB565, V4L2_PIX_FMT_BGR24 or V4L2_PIX_FMT_BGR32
 *
 * @return 1 if supported, 0 if failed
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

/*
 * Returns bits per pixel for given pixel format
 *
 * @param pixelformat  V4L2_PIX_FMT_RGB565, V4L2_PIX_FMT_BGR24 or V4L2_PIX_FMT_BGR32
 *
 * @return bits per pixel of pixelformat
 */
static u32 fmt_to_bpp(u32 pixelformat)
{
	u32 bpp;

	switch (pixelformat) {
	case V4L2_PIX_FMT_RGB565:
		bpp = 16;
		break;
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB24:
		bpp = 24;
		break;
	case V4L2_PIX_FMT_BGR32:
	case V4L2_PIX_FMT_RGB32:
		bpp = 32;
		break;
	default:
		bpp = 8;
		break;
	}
	return bpp;
}

/*
 * V4L2 - Handles VIDIOC_G_FMT Ioctl
 *
 * @param vout         structure vout_data *
 *
 * @param v4l2_format structure v4l2_format *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_v4l2out_g_fmt(vout_data * vout, struct v4l2_format *f)
{
	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		return -EINVAL;
	}
	*f = vout->v2f;
	return 0;
}

/*
 * V4L2 - Handles VIDIOC_S_FMT Ioctl
 *
 * @param vout         structure vout_data *
 *
 * @param v4l2_format structure v4l2_format *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_v4l2out_s_fmt(vout_data * vout, struct v4l2_format *f)
{
	int retval = 0;
	u32 size = 0;
	u32 bytesperline;

	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		retval = -EINVAL;
		goto err0;
	}
	if (!valid_mode(f->fmt.pix.pixelformat)) {
		DPRINTK("pixel format not supported\n");
		retval = -EINVAL;
		goto err0;
	}

	bytesperline = (f->fmt.pix.width * fmt_to_bpp(f->fmt.pix.pixelformat)) /
	    8;
	if (f->fmt.pix.bytesperline < bytesperline) {
		f->fmt.pix.bytesperline = bytesperline;
	} else {
		bytesperline = f->fmt.pix.bytesperline;
	}

	switch (f->fmt.pix.pixelformat) {
	case V4L2_PIX_FMT_YUV422P:
		/* byteperline for YUV planar formats is for
		   Y plane only */
		size = bytesperline * f->fmt.pix.height * 2;
		break;
	case V4L2_PIX_FMT_YUV420:
		size = (bytesperline * f->fmt.pix.height * 3) / 2;
		break;
	default:
		size = bytesperline * f->fmt.pix.height;
		break;
	}

	/* Return the actual size of the image to the app */
	f->fmt.pix.sizeimage = size;

	vout->v2f.fmt.pix.sizeimage = size;
	vout->v2f.fmt.pix.width = f->fmt.pix.width;
	vout->v2f.fmt.pix.height = f->fmt.pix.height;
	vout->v2f.fmt.pix.pixelformat = f->fmt.pix.pixelformat;
	vout->v2f.fmt.pix.bytesperline = f->fmt.pix.bytesperline;

	retval = 0;
      err0:
	return retval;
}

/*
 * V4L2 - Handles VIDIOC_G_CTRL Ioctl
 *
 * @param vout         structure vout_data *
 *
 * @param c           structure v4l2_control *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_get_v42lout_control(vout_data * vout, struct v4l2_control *c)
{
	switch (c->id) {
	case V4L2_CID_HFLIP:
		return (vout->rotate & IPU_ROTATE_HORIZ_FLIP) ? 1 : 0;
	case V4L2_CID_VFLIP:
		return (vout->rotate & IPU_ROTATE_VERT_FLIP) ? 1 : 0;
	case (V4L2_CID_PRIVATE_BASE + 1):
		return vout->rotate;
	default:
		return -EINVAL;
	}
}

/*
 * V4L2 - Handles VIDIOC_S_CTRL Ioctl
 *
 * @param vout         structure vout_data *
 *
 * @param c           structure v4l2_control *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_set_v42lout_control(vout_data * vout, struct v4l2_control *c)
{
	switch (c->id) {
	case V4L2_CID_HFLIP:
		vout->rotate |= c->value ? IPU_ROTATE_HORIZ_FLIP :
		    IPU_ROTATE_NONE;
		break;
	case V4L2_CID_VFLIP:
		vout->rotate |= c->value ? IPU_ROTATE_VERT_FLIP :
		    IPU_ROTATE_NONE;
		break;
	case V4L2_CID_MXC_ROTATE:
		vout->rotate = c->value;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/*!
 * V4L2 interface - open function
 *
 * @param inode        structure inode *
 *
 * @param file         structure file *
 *
 * @return  status    0 success, ENODEV invalid device instance,
 *                    ENODEV timeout, ERESTARTSYS interrupted by user
 */
static int mxc_v4l2out_open(struct inode *inode, struct file *file)
{
	struct video_device *dev = video_devdata(file);
	vout_data *vout = video_get_drvdata(dev);
	int err;

	FUNC_START;

	if (!vout) {
		DPRINTK("Internal error, vout_data not found!\n");
		return -ENODEV;
	}

	down(&vout->busy_lock);

	err = -EINTR;
	if (signal_pending(current))
		goto oops;

	if (vout->open_count++ == 0) {
		ipu_request_irq(IPU_IRQ_PP_IN_EOF,
				mxc_v4l2out_pp_in_irq_handler,
				0, dev->name, vout);

		init_waitqueue_head(&vout->v4l_bufq);

		init_timer(&vout->output_timer);
		vout->output_timer.function = mxc_v4l2out_timer_handler;
		vout->output_timer.data = (unsigned long)vout;

		vout->state = STATE_STREAM_OFF;
		g_irq_cnt = g_buf_output_cnt = g_buf_q_cnt = g_buf_dq_cnt = 0;

	}

	file->private_data = dev;

	up(&vout->busy_lock);

	FUNC_END;
	return 0;

      oops:
	up(&vout->busy_lock);
	return err;
}

/*!
 * V4L2 interface - close function
 *
 * @param inode    struct inode *
 *
 * @param file     struct file *
 *
 * @return         0 success
 */
static int mxc_v4l2out_close(struct inode *inode, struct file *file)
{
	struct video_device *dev = file->private_data;
	vout_data *vout = video_get_drvdata(dev);

	if (--vout->open_count == 0) {
		DPRINTK("release resource\n");

		if (vout->state != STATE_STREAM_OFF)
			mxc_v4l2out_streamoff(vout);

/*
                printk("g_irq_cnt = %d, g_buf_output_cnt = %d, " \
                        "g_buf_q_cnt = %d, g_buf_dq_cnt = %d\n",
                        g_irq_cnt, g_buf_output_cnt, g_buf_q_cnt, g_buf_dq_cnt);
*/

		ipu_free_irq(IPU_IRQ_PP_IN_EOF, vout);

		file->private_data = NULL;

		mxc_free_buffers(vout->queue_buf_paddr, vout->buffer_cnt);
		vout->buffer_cnt = 0;
		mxc_free_buffers(vout->display_bufs, 2);
		mxc_free_buffers(vout->rot_pp_bufs, 2);

		/* capture off */
		wake_up_interruptible(&vout->v4l_bufq);
	}

	return 0;
}

/*!
 * V4L2 interface - ioctl function
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
mxc_v4l2out_do_ioctl(struct inode *inode, struct file *file,
		     unsigned int ioctlnr, void *arg)
{
	struct video_device *dev = file->private_data;
	vout_data *vout = video_get_drvdata(dev);
	int retval = 0;
	int i = 0;

	if (!vout)
		return -ENODEV;

	/* make this _really_ smp-safe */
	if (down_interruptible(&vout->busy_lock))
		return -EINTR;

	switch (ioctlnr) {
	case VIDIOC_QUERYCAP:
		{
			struct v4l2_capability *cap = arg;
			strcpy(cap->driver, "mxc_v4l2_output");
			cap->version = 0;
			cap->capabilities =
			    V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
			retval = 0;
			break;
		}
	case VIDIOC_G_FMT:
		{
			struct v4l2_format *gf = arg;
			retval = mxc_v4l2out_g_fmt(vout, gf);
			break;
		}
	case VIDIOC_S_FMT:
		{
			struct v4l2_format *sf = arg;
			if (vout->state != STATE_STREAM_OFF) {
				retval = -EBUSY;
				break;
			}
			retval = mxc_v4l2out_s_fmt(vout, sf);
			break;
		}
	case VIDIOC_REQBUFS:
		{
			struct v4l2_requestbuffers *req = arg;
			if ((req->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) ||
			    (req->memory != V4L2_MEMORY_MMAP)) {
				DPRINTK
				    ("VIDIOC_REQBUFS: incorrect buffer type\n");
				retval = -EINVAL;
				break;
			}

			if (req->count == 0) {
				mxc_v4l2out_streamoff(vout);
				if (vout->queue_buf_paddr[0] != 0) {
					mxc_free_buffers(vout->queue_buf_paddr,
							 vout->buffer_cnt);
					DPRINTK
					    ("VIDIOC_REQBUFS: freed buffers\n");
				}
				vout->buffer_cnt = 0;
				break;
			}

			if (vout->queue_buf_paddr[0] != 0) {
				DPRINTK
				    ("VIDIOC_REQBUFS: Cannot allocate buffers\n");
				retval = -EBUSY;
				break;
			}

			if (req->count < MIN_FRAME_NUM) {
				req->count = MIN_FRAME_NUM;
			} else if (req->count > MAX_FRAME_NUM) {
				req->count = MAX_FRAME_NUM;
			}
			vout->buffer_cnt = req->count;

			retval = mxc_allocate_buffers(vout->queue_buf_paddr,
						      vout->buffer_cnt,
						      PAGE_ALIGN(vout->v2f.fmt.
								 pix.
								 sizeimage));
			if (retval < 0)
				break;

			/* Init buffer queues */
			vout->done_q.head = 0;
			vout->done_q.tail = 0;
			vout->ready_q.head = 0;
			vout->ready_q.tail = 0;

			for (i = 0; i < vout->buffer_cnt; i++) {
				memset(&(vout->v4l2_bufs[i]), 0,
				       sizeof(vout->v4l2_bufs[i]));
				vout->v4l2_bufs[i].flags = 0;
				vout->v4l2_bufs[i].memory = V4L2_MEMORY_MMAP;
				vout->v4l2_bufs[i].index = i;
				vout->v4l2_bufs[i].type =
				    V4L2_BUF_TYPE_VIDEO_OUTPUT;
				vout->v4l2_bufs[i].length =
				    PAGE_ALIGN(vout->v2f.fmt.pix.sizeimage);
				vout->v4l2_bufs[i].m.offset =
				    (unsigned long)vout->queue_buf_paddr[i];
				vout->v4l2_bufs[i].timestamp.tv_sec = 0;
				vout->v4l2_bufs[i].timestamp.tv_usec = 0;
			}
			break;
		}
	case VIDIOC_QUERYBUF:
		{
			struct v4l2_buffer *buf = arg;
			u32 type = buf->type;
			int index = buf->index;

			if ((type != V4L2_BUF_TYPE_VIDEO_OUTPUT) ||
			    (index >= vout->buffer_cnt)) {
				DPRINTK
				    ("VIDIOC_QUERYBUFS: incorrect buffer type\n");
				retval = -EINVAL;
				break;
			}
			down(&vout->param_lock);
			memcpy(buf, &(vout->v4l2_bufs[index]), sizeof(*buf));
			up(&vout->param_lock);
			break;
		}
	case VIDIOC_QBUF:
		{
			struct v4l2_buffer *buf = arg;
			int index = buf->index;
			u32 lock_flags;

			if ((buf->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) ||
			    (index >= vout->buffer_cnt) || (buf->flags != 0)) {
				retval = -EINVAL;
				break;
			}

			DPRINTK("VIDIOC_QBUF: %d\n", buf->index);

			spin_lock_irqsave(&g_lock, lock_flags);

			memcpy(&(vout->v4l2_bufs[index]), buf, sizeof(*buf));
			vout->v4l2_bufs[index].flags |= V4L2_BUF_FLAG_QUEUED;

			g_buf_q_cnt++;
			queue_buf(&vout->ready_q, index);
			if (vout->state == STATE_STREAM_PAUSED) {
				unsigned long timeout =
				    timeval_to_jiffies(&vout->v4l2_bufs[index].
						       timestamp);
				if (!timeout) {
					/* if timestamp is 0, then default to 30fps */
					timeout = vout->start_jiffies +
					    msecs_to_jiffies(vout->frame_count *
							     33);
				} else {	/* Adjust time from time of day to jiffies */
					timeout -= vout->start_tod_jiffies;
				}
				vout->output_timer.expires = timeout;
				DPRINTK("QBUF: frame #%u timeout @ %u jiffies, "
					"current = %u\n",
					vout->frame_count, timeout, jiffies);
				add_timer(&vout->output_timer);
				vout->state = STATE_STREAM_ON;
				vout->frame_count++;
			}

			spin_unlock_irqrestore(&g_lock, lock_flags);
			break;
		}
	case VIDIOC_DQBUF:
		{
			struct v4l2_buffer *buf = arg;
			int idx;

/*                DPRINTK("VIDIOC_DQBUF: q size = %d\n",
                        queue_size(&vout->done_q));
*/
			if ((queue_size(&vout->done_q) == 0) &&
			    (file->f_flags & O_NONBLOCK)) {
				retval = -EAGAIN;
				break;
			}

			if (!wait_event_interruptible_timeout(vout->v4l_bufq,
							      queue_size(&vout->
									 done_q)
							      != 0, 2 * HZ)) {
				printk("VIDIOC_DQBUF: timeout\n");
				retval = -ETIME;
				break;
			} else if (signal_pending(current)) {
				printk("VIDIOC_DQBUF: interrupt received\n");
				vout->state = STATE_STREAM_STOPPING;
				retval = -ERESTARTSYS;
				break;
			}
			idx = dequeue_buf(&vout->done_q);
			if (idx == -1) {	/* No frame free */
				printk
				    ("VIDIOC_DQBUF: no free buffers, returning\n");
				retval = -EAGAIN;
				break;
			}
			if ((vout->v4l2_bufs[idx].flags & V4L2_BUF_FLAG_DONE) ==
			    0)
				printk
				    ("VIDIOC_DQBUF: buffer in done q, but not "
				     "flagged as done\n");

			vout->v4l2_bufs[idx].flags = 0;
			memcpy(buf, &(vout->v4l2_bufs[idx]), sizeof(*buf));
			DPRINTK("VIDIOC_DQBUF: %d\n", buf->index);
			break;
		}
	case VIDIOC_STREAMON:
		{
			struct timeval t;
			do_gettimeofday(&t);
			vout->start_tod_jiffies =
			    timeval_to_jiffies(&t) - jiffies;
			vout->frame_count = 2;
			vout->start_jiffies = jiffies;
			DPRINTK("VIDIOC_STREAMON: start time = %u jiffies, "
				"tod adjustment = %u\n",
				vout->start_jiffies, vout->start_tod_jiffies);

			retval = mxc_v4l2out_streamon(vout);
			break;
		}
	case VIDIOC_STREAMOFF:
		{
			retval = mxc_v4l2out_streamoff(vout);
			break;
		}
	case VIDIOC_G_CTRL:
		{
			retval = mxc_get_v42lout_control(vout, arg);
			break;
		}
	case VIDIOC_S_CTRL:
		{
			retval = mxc_set_v42lout_control(vout, arg);
			break;
		}
	case VIDIOC_CROPCAP:
		{
			struct v4l2_cropcap *cap = arg;

			if (cap->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
				retval = -EINVAL;
				break;
			}
			cap->bounds = vout->crop_bounds[vout->cur_disp_output];
			cap->defrect = vout->crop_bounds[vout->cur_disp_output];
			retval = 0;
			break;
		}
	case VIDIOC_G_CROP:
		{
			struct v4l2_crop *crop = arg;

			if (crop->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
				retval = -EINVAL;
				break;
			}
			crop->c = vout->crop_current;
			break;
		}
	case VIDIOC_S_CROP:
		{
			struct v4l2_crop *crop = arg;
			struct v4l2_rect *b =
			    &(vout->crop_bounds[vout->cur_disp_output]);

			if (crop->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
				retval = -EINVAL;
				break;
			}
			if (crop->c.height < 0) {
				retval = -EINVAL;
				break;
			}
			if (crop->c.width < 0) {
				retval = -EINVAL;
				break;
			}

			if (crop->c.top < b->top)
				crop->c.top = b->top;
			if (crop->c.top > b->top + b->height)
				crop->c.top = b->top + b->height;
			if (crop->c.height > b->top - crop->c.top + b->height)
				crop->c.height =
				    b->top - crop->c.top + b->height;

			if (crop->c.left < b->left)
				crop->c.top = b->left;
			if (crop->c.left > b->left + b->width)
				crop->c.top = b->left + b->width;
			if (crop->c.width > b->left - crop->c.left + b->width)
				crop->c.width =
				    b->left - crop->c.left + b->width;

			/* stride line limitation */
			crop->c.height -= crop->c.height % 8;
			crop->c.width -= crop->c.width % 8;

			vout->crop_current = crop->c;

			vout->sdc_fg_buf_size = vout->crop_current.width *
			    vout->crop_current.height;
			vout->sdc_fg_buf_size *=
			    fmt_to_bpp(SDC_FG_FB_FORMAT) / 8;

			/* Free previously allocated buffer */
			if (vout->display_bufs[0] != NULL) {
				mxc_free_buffers(vout->display_bufs, 2);
			}
			if ((retval = mxc_allocate_buffers(vout->display_bufs,
							   2,
							   vout->
							   sdc_fg_buf_size)) <
			    0) {
				DPRINTK("unable to allocate SDC FG buffers\n");
				retval = -ENOMEM;
				break;
			}
			break;
		}
	case VIDIOC_ENUMOUTPUT:
		{
			struct v4l2_output *output = arg;

			if ((output->index >= 4) ||
			    (vout->output_enabled[output->index] == false)) {
				retval = -EINVAL;
				break;
			}

			if (output->index < 3) {
				*output = mxc_outputs[MXC_V4L2_OUT_2_ADC];
				output->name[4] = '0' + output->index;
			} else {
				*output = mxc_outputs[MXC_V4L2_OUT_2_SDC];
			}
			break;
		}
	case VIDIOC_G_OUTPUT:
		{
			int *p_output_num = arg;

			*p_output_num = vout->cur_disp_output;
			break;
		}
	case VIDIOC_S_OUTPUT:
		{
			int *p_output_num = arg;

			if ((*p_output_num >= 4) ||
			    (vout->output_enabled[*p_output_num] == false)) {
				retval = -EINVAL;
				break;
			}

			if (vout->state != STATE_STREAM_OFF) {
				retval = -EBUSY;
				break;
			}

			vout->cur_disp_output = *p_output_num;
			break;
		}
	case VIDIOC_ENUM_FMT:
	case VIDIOC_TRY_FMT:
	case VIDIOC_QUERYCTRL:
	case VIDIOC_G_PARM:
	case VIDIOC_ENUMSTD:
	case VIDIOC_G_STD:
	case VIDIOC_S_STD:
	case VIDIOC_G_TUNER:
	case VIDIOC_S_TUNER:
	case VIDIOC_G_FREQUENCY:
	case VIDIOC_S_FREQUENCY:
	default:
		retval = -EINVAL;
		break;
	}

	up(&vout->busy_lock);
	return retval;
}

/*
 * V4L2 interface - ioctl function
 *
 * @return  None
 */
static int
mxc_v4l2out_ioctl(struct inode *inode, struct file *file,
		  unsigned int cmd, unsigned long arg)
{
	return video_usercopy(inode, file, cmd, arg, mxc_v4l2out_do_ioctl);
}

/*!
 * V4L2 interface - mmap function
 *
 * @param file          structure file *
 *
 * @param vma           structure vm_area_struct *
 *
 * @return status       0 Success, EINTR busy lock error,
 *                      ENOBUFS remap_page error
 */
static int mxc_v4l2out_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct video_device *dev = file->private_data;
	unsigned long size;
	int res = 0;
	vout_data *vout = video_get_drvdata(dev);

	DPRINTK("pgoff=0x%x, start=0x%x, end=0x%x\n",
		vma->vm_pgoff, vma->vm_start,  vma->vm_end);

	/* make this _really_ smp-safe */
	if (down_interruptible(&vout->busy_lock))
		return -EINTR;

	size = vma->vm_end - vma->vm_start;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (remap_pfn_range(vma, vma->vm_start,
		vma->vm_pgoff, size, vma->vm_page_prot)) {
		printk("mxc_mmap(V4L)i - remap_pfn_range failed\n");
		res = -ENOBUFS;
		goto mxc_mmap_exit;
	}

	vma->vm_flags &= ~VM_IO; /* using shared anonymous pages */

mxc_mmap_exit:
	up(&vout->busy_lock);
	return res;
}

/*!
 * V4L2 interface - poll function
 *
 * @param file       structure file *
 *
 * @param wait       structure poll_table *
 *
 * @return  status   POLLIN | POLLRDNORM
 */
static unsigned int mxc_v4l2out_poll(struct file *file, poll_table * wait)
{
	struct video_device *dev = file->private_data;
	vout_data *vout = video_get_drvdata(dev);

	wait_queue_head_t *queue = NULL;
	int res = POLLIN | POLLRDNORM;

	if (down_interruptible(&vout->busy_lock))
		return -EINTR;

	queue = &vout->v4l_bufq;
	poll_wait(file, queue, wait);

	up(&vout->busy_lock);
	return res;
}

static struct
file_operations mxc_v4l2out_fops = {
	.owner = THIS_MODULE,
	.open = mxc_v4l2out_open,
	.release = mxc_v4l2out_close,
	.ioctl = mxc_v4l2out_ioctl,
	.mmap = mxc_v4l2out_mmap,
	.poll = mxc_v4l2out_poll,
};

static struct video_device mxc_v4l2out_template = {
	.owner = THIS_MODULE,
	.name = "MXC Video Output",
	.type = 0,
	.type2 = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING,
	.hardware = 39,
	.fops = &mxc_v4l2out_fops,
	.release = video_device_release,
};

/*!
 * Probe routine for the framebuffer driver. It is called during the
 * driver binding process.      The following functions are performed in
 * this routine: Framebuffer initialization, Memory allocation and
 * mapping, Framebuffer registration, IPU initialization.
 *
 * @return      Appropriate error code to the kernel common code
 */
static int mxc_v4l2out_probe(struct device *dev)
{
	int i;
	vout_data *vout;

	FUNC_START;
	/*
	 * Allocate sufficient memory for the fb structure
	 */
	vout = kmalloc(sizeof(vout_data), GFP_KERNEL);

	if (!vout)
		return 0;

	memset(vout, 0, sizeof(vout_data));

	vout->video_dev = video_device_alloc();
	if (vout->video_dev == NULL)
		return -1;
	vout->video_dev->dev = dev;
	vout->video_dev->minor = -1;

	*(vout->video_dev) = mxc_v4l2out_template;

	/* register v4l device */
	if (video_register_device(vout->video_dev,
				  VFL_TYPE_GRABBER, video_nr) == -1) {
		printk(KERN_DEBUG "video_register_device failed\n");
		return 0;
	}
	printk(KERN_INFO "mxc_v4l2out: registered device video%d\n",
	       vout->video_dev->minor & 0x1f);

	video_set_drvdata(vout->video_dev, vout);

	init_MUTEX(&vout->param_lock);
	init_MUTEX(&vout->busy_lock);

	/* setup outputs and cropping */
	vout->cur_disp_output = -1;
	for (i = 0; i < num_registered_fb; i++) {
		char *idstr = registered_fb[i]->fix.id;
		if (strncmp(idstr, "DISP", 4) == 0) {
			int disp_num = idstr[4] - '0';
			if ((disp_num == 3) &&
			    (strncmp(idstr, "DISP3 BG", 8) != 0)) {
				continue;
			}
			vout->crop_bounds[disp_num].left = 0;
			vout->crop_bounds[disp_num].top = 0;
			vout->crop_bounds[disp_num].width =
			    registered_fb[i]->var.xres;
			vout->crop_bounds[disp_num].height =
			    registered_fb[i]->var.yres;
			vout->output_enabled[disp_num] = true;
			vout->output_fb_num[disp_num] = i;
			if (vout->cur_disp_output == -1)
				vout->cur_disp_output = disp_num;
		}

	}
	vout->crop_current = vout->crop_bounds[vout->cur_disp_output];

	FUNC_END;
	return 0;
}

static int mxc_v4l2out_remove(struct device *dev)
{
	vout_data *vout = dev_get_drvdata(dev);

	FUNC_START;

	if (vout->video_dev) {
		if (-1 != vout->video_dev->minor)
			video_unregister_device(vout->video_dev);
		else
			video_device_release(vout->video_dev);
		vout->video_dev = NULL;
	}

	dev_set_drvdata(dev, NULL);

	kfree(vout);

	FUNC_END;
	return 0;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxc_v4l2out_driver = {
	.name = "MXC Video Output",
	.bus = &platform_bus_type,
	.probe = mxc_v4l2out_probe,
	.remove = mxc_v4l2out_remove,
};

static struct platform_device mxc_v4l2out_device = {
	.name = "MXC Video Output",
	.id = 0,
};

/*!
 * mxc v4l2 init function
 *
 */
static int mxc_v4l2out_init(void)
{
	u8 err = 0;
	FUNC_START;

	err = driver_register(&mxc_v4l2out_driver);
	if (err == 0) {
		platform_device_register(&mxc_v4l2out_device);
	}
	FUNC_END;
	return err;
}

/*!
 * mxc v4l2 cleanup function
 *
 */
static void mxc_v4l2out_clean(void)
{
	FUNC_START;

}

module_init(mxc_v4l2out_init);
module_exit(mxc_v4l2out_clean);

MODULE_PARM(video_nr, "i");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("V4L2-driver for MXC video output");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("video");
