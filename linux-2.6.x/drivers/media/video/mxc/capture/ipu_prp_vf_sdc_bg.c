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
 * @file ipu_prp_vf_sdc_bg.c
 *
 * @brief IPU Use case for PRP-VF back-ground
 *
 * @ingroup IPU
 */
#include <linux/fb.h>
#include "../drivers/media/video/mxc/capture/mxc_v4l2_capture.h"
#include "../drivers/mxc/ipu/ipu.h"
#include "ipu_prp_sw.h"

//#define MXC_PRP_VF_SDC_BG_DEBUG
#ifdef MXC_PRP_VF_SDC_BG_DEBUG

#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " \
          fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START DPRINTK(" func start\n")
#  define FUNC_END DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", \
          __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* MXC_PRP_VF_SDC_BG_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* MXC_PRP_VF_SDC_BG_DEBUG */

static int buffer_num = 0;
static int buffer_ready = 0;

/*
 * Function definitions
 */

/*!
 * SDC V-Sync callback function.
 *
 * @param irq       int irq line
 * @param dev_id    void * device id
 * @param regs      struct pt_regs *
 *
 * @return status   IRQ_HANDLED for handled
 */
static irqreturn_t
prpvf_sdc_vsync_callback(int irq, void *dev_id, struct pt_regs *regs)
{
	FUNC_START;

	DPRINTK("buffer_ready %d buffer_num %d\n", buffer_ready, buffer_num);
	if (buffer_ready > 0) {
		ipu_select_buffer(MEM_ROT_VF_MEM, IPU_OUTPUT_BUFFER, 0);
		buffer_ready--;
	}

	FUNC_END;
	return IRQ_HANDLED;
}

/*!
 * VF EOF callback function.
 *
 * @param irq       int irq line
 * @param dev_id    void * device id
 * @param regs      struct pt_regs *
 *
 * @return status   IRQ_HANDLED for handled
 */
static irqreturn_t
prpvf_vf_eof_callback(int irq, void *dev_id, struct pt_regs *regs)
{
	FUNC_START;
	DPRINTK("buffer_ready %d buffer_num %d\n", buffer_ready, buffer_num);

	ipu_select_buffer(MEM_ROT_VF_MEM, IPU_INPUT_BUFFER, buffer_num);

	buffer_num = (buffer_num == 0) ? 1 : 0;

	ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, buffer_num);
	buffer_ready++;
	FUNC_END;
	return IRQ_HANDLED;
}

/*!
 * prpvf_start - start the vf task
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 */
static int prpvf_start(void *private)
{
	cam_data *cam = (cam_data *) private;
	ipu_channel_params_t vf;
	u32 format;
	u32 offset;
	u32 size = 3;
	int err = 0;

	FUNC_START;

	if (!cam) {
		DPRINTK("private is NULL\n");
		return -EIO;
	}

	if (cam->overlay_active == true) {
		DPRINTK("already start.\n");
		return 0;
	}

	format = cam->v4l2_fb.fmt.pixelformat;
	if (cam->v4l2_fb.fmt.pixelformat == IPU_PIX_FMT_BGR24) {
		size = 3;
		DPRINTK("BGR24\n");
	} else if (cam->v4l2_fb.fmt.pixelformat == IPU_PIX_FMT_RGB565) {
		size = 2;
		DPRINTK("RGB565\n");
	} else if (cam->v4l2_fb.fmt.pixelformat == IPU_PIX_FMT_BGR32) {
		size = 4;
		DPRINTK("BGR32\n");
	} else {
		DPRINTK("unsupported fix format from the framebuffer.\n");
	}

	offset = cam->v4l2_fb.fmt.bytesperline * cam->win.w.top +
	    size * cam->win.w.left;

	if (cam->v4l2_fb.base == 0) {
		offset += MXCIPU_MEM_ADDRESS;
	} else {
		offset += (u32) cam->v4l2_fb.base;
	}

	memset(&vf, 0, sizeof(ipu_channel_params_t));
	ipu_csi_get_window_size(&vf.csi_prp_vf_mem.in_width,
				&vf.csi_prp_vf_mem.in_height);
	vf.csi_prp_vf_mem.in_pixel_fmt = IPU_PIX_FMT_YUYV;
	vf.csi_prp_vf_mem.out_width = cam->win.w.width;
	vf.csi_prp_vf_mem.out_height = cam->win.w.height;
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		vf.csi_prp_vf_mem.out_width = cam->win.w.height;
		vf.csi_prp_vf_mem.out_height = cam->win.w.width;
	}
	vf.csi_prp_vf_mem.out_pixel_fmt = format;
	size = cam->win.w.width * cam->win.w.height * size;

	ipu_uninit_channel(CSI_PRP_VF_MEM);
	err = ipu_init_channel(CSI_PRP_VF_MEM, &vf);
	if (err != 0)
		return err;
	ipu_csi_enable_mclk(CSI_MCLK_VF, true, true);

	if (cam->vf_bufs[0])
		ipu_free((u32) (cam->vf_bufs[0]));
	cam->vf_bufs[0] = (void *)ipu_malloc(PAGE_ALIGN(size));
	if (cam->vf_bufs[0] == NULL) {
		DPRINTK("Error to allocate vf buffer\n");
		return -ENOMEM;
	}
	if (cam->vf_bufs[1])
		ipu_free((u32) (cam->vf_bufs[1]));
	cam->vf_bufs[1] = (void *)ipu_malloc(PAGE_ALIGN(size));
	if (cam->vf_bufs[1] == NULL) {
		ipu_free((u32) (cam->vf_bufs[0]));
		cam->vf_bufs[0] = NULL;
		DPRINTK("Error to allocate vf buffer\n");
		return -ENOMEM;
	}

	err = ipu_init_channel_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER,
				      format, vf.csi_prp_vf_mem.out_width,
				      vf.csi_prp_vf_mem.out_height,
				      vf.csi_prp_vf_mem.out_width,
				      IPU_ROTATE_NONE, cam->vf_bufs[0],
				      cam->vf_bufs[1]);
	if (err != 0) {
		DPRINTK("Error initializing CSI_PRP_VF_MEM\n");
		return err;
	}

	ipu_uninit_channel(MEM_ROT_VF_MEM);
	err = ipu_init_channel(MEM_ROT_VF_MEM, NULL);
	if (err != 0) {
		DPRINTK("Error MEM_ROT_VF_MEM channel\n");
		return err;
	}

	err = ipu_init_channel_buffer(MEM_ROT_VF_MEM, IPU_INPUT_BUFFER,
				      format, vf.csi_prp_vf_mem.out_width,
				      vf.csi_prp_vf_mem.out_height,
				      vf.csi_prp_vf_mem.out_width,
				      cam->rotation, cam->vf_bufs[0],
				      cam->vf_bufs[1]);
	if (err != 0) {
		DPRINTK("Error MEM_ROT_VF_MEM input buffer\n");
		return err;
	}

	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		err = ipu_init_channel_buffer(MEM_ROT_VF_MEM, IPU_OUTPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
					      cam->overlay_fb->var.xres,
					      IPU_ROTATE_NONE, (void *)offset,
					      NULL);

		if (err != 0) {
			DPRINTK("Error MEM_ROT_VF_MEM output buffer\n");
			return err;
		}
	} else {
		err = ipu_init_channel_buffer(MEM_ROT_VF_MEM, IPU_OUTPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      cam->overlay_fb->var.xres,
					      IPU_ROTATE_NONE, (void *)offset,
					      NULL);
		if (err != 0) {
			DPRINTK("Error MEM_ROT_VF_MEM output buffer\n");
			return err;
		}
	}

	err = ipu_request_irq(IPU_IRQ_PRP_VF_OUT_EOF, prpvf_vf_eof_callback,
			      0, "Mxc Camera", cam);
	if (err != 0) {
		DPRINTK("Error registering IPU_IRQ_PRP_VF_OUT_EOF irq.\n");
		return err;
	}

	err = ipu_request_irq(IPU_IRQ_SDC_BG_OUT_EOF, prpvf_sdc_vsync_callback,
			      0, "Mxc Camera", NULL);
	if (err != 0) {
		DPRINTK("Error registering IPU_IRQ_SDC_BG_OUT_EOF irq.\n");
		return err;
	}

	ipu_enable_channel(CSI_PRP_VF_MEM);
	ipu_enable_channel(MEM_ROT_VF_MEM);

	buffer_num = 0;
	buffer_ready = 0;
	ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 0);

	cam->overlay_active = true;
	FUNC_END;
	return err;
}

/*!
 * prpvf_stop - stop the vf task
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 */
static int prpvf_stop(void *private)
{
	cam_data *cam = (cam_data *) private;
	FUNC_START;

	if (cam->overlay_active == false)
		return 0;

	ipu_free_irq(IPU_IRQ_SDC_BG_OUT_EOF, NULL);
	ipu_free_irq(IPU_IRQ_PRP_VF_OUT_EOF, cam);

	ipu_disable_channel(CSI_PRP_VF_MEM, true);
	ipu_disable_channel(MEM_ROT_VF_MEM, true);
	ipu_uninit_channel(CSI_PRP_VF_MEM);
	ipu_uninit_channel(MEM_ROT_VF_MEM);
	ipu_csi_enable_mclk(CSI_MCLK_VF, false, false);

	if (cam->vf_bufs[0]) {
		ipu_free((u32) (cam->vf_bufs[0]));
		cam->vf_bufs[0] = NULL;
	}
	if (cam->vf_bufs[1]) {
		ipu_free((u32) (cam->vf_bufs[1]));
		cam->vf_bufs[1] = NULL;
	}

	buffer_num = 0;
	buffer_ready = 0;
	cam->overlay_active = false;
	FUNC_END;
	return 0;
}

/*!
 * function to select PRP-VF as the working path
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 * @return  status
 */
int prp_vf_sdc_select_bg(void *private)
{
	cam_data *cam = (cam_data *) private;

	FUNC_START;
	if (cam) {
		cam->vf_start_sdc = prpvf_start;
		cam->vf_stop_sdc = prpvf_stop;
		cam->overlay_active = false;
	}

	FUNC_END;
	return 0;
}

/*!
 * function to de-select PRP-VF as the working path
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 * @return  status
 */
int prp_vf_sdc_deselect_bg(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;
	FUNC_START;
	err = prpvf_stop(private);

	if (cam) {
		cam->vf_start_sdc = NULL;
		cam->vf_stop_sdc = NULL;
	}
	FUNC_END;
	return err;
}

/*!
 * Init viewfinder task.
 *
 * @return  Error code indicating success or failure
 */
__init int prp_vf_sdc_init_bg(void)
{
	FUNC_START;
	FUNC_END;
	return 0;
}

/*!
 * Deinit viewfinder task.
 *
 * @return  Error code indicating success or failure
 */
void __exit prp_vf_sdc_exit_bg(void)
{
	FUNC_START;
	FUNC_END;
}

module_init(prp_vf_sdc_init_bg);
module_exit(prp_vf_sdc_exit_bg);

EXPORT_SYMBOL(prp_vf_sdc_select_bg);
EXPORT_SYMBOL(prp_vf_sdc_deselect_bg);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("IPU PRP VF SDC Backgroud Driver");
MODULE_LICENSE("GPL");
