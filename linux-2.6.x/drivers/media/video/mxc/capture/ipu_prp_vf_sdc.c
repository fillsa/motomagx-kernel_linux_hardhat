/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006 Motorola Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*
Revision History:
                    Modification    
Author                  Date        Description of Changes
-----------------   ------------    -------------------------
Motorola            07/12/2006      Viewfinder causing very long response
                                    delay to user input
 */
/*!
 * @file ipu_prp_vf_sdc.c
 *
 * @brief IPU Use case for PRP-VF
 *
 * @ingroup IPU
 */

#include "../drivers/media/video/mxc/capture/mxc_v4l2_capture.h"
#include "../drivers/mxc/ipu/ipu.h"
#include "ipu_prp_sw.h"

//#define MXC_PRP_VF_SDC_DEBUG
#ifdef MXC_PRP_VF_SDC_DEBUG

#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " \
          fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START DPRINTK(" func start\n")
#  define FUNC_END DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", \
          __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* MXC_PRP_VF_SDC_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* MXC_PRP_VF_SDC_DEBUG */

#ifdef CONFIG_VIDEO_MXC_V4L1

#include <linux/fb.h>
#include <linux/camera.h>

#define FB_DEV_INDEX     0
#define FB_OVL_DEV_INDEX 1


extern struct fb_info *registered_fb[FB_MAX];
extern V4l_VF_PARAM vfparam;

/*
 * This function is copied from /vobs/jem/hardhat/linux-2.6.x/drivers/video/mxc/mxcfb.c.
 * Given bits per pixel, it returns pixel color format used by the frame buffer device
 * In order to avoid making change in frame buffer device driver code, I copied the function
 * here instead of removing the "static" keyword from the original code. This may cause problems
 * if this function gets updated in the frame buffer device driver.
 */
static u32 bpp_to_pixfmt(int bpp)
{
        u32 pixfmt = 0;
        switch (bpp) {
        case 24:
#if defined(CONFIG_MOT_FEAT_IPU_RGB) && defined(CONFIG_FB_MXC_RGB)
                pixfmt = IPU_PIX_FMT_RGB24;
#elif defined(CONFIG_MOT_FEAT_IPU_BGRA6666)
                pixfmt = IPU_PIX_FMT_BGRA6666;
#else
                pixfmt = IPU_PIX_FMT_BGR24;
#endif
                break;
        case 32:
#if defined(CONFIG_MOT_FEAT_IPU_RGB) && defined(CONFIG_FB_MXC_RGB)
                pixfmt = IPU_PIX_FMT_RGB32;
#else
                pixfmt = IPU_PIX_FMT_BGR32;
#endif
                break;
        case 16:
#if defined(CONFIG_MOT_FEAT_IPU_RGB) && defined(CONFIG_FB_MXC_RGB) || \
	!defined(CONFIG_MOT_WFN271)
                pixfmt = IPU_PIX_FMT_RGB565;
#else
                pixfmt = IPU_PIX_FMT_BGR565;
#endif
                break;
        }
        return pixfmt;
}

#endif /* CONFIG_VIDEO_MXC_V4L1 */

/*
 * Function definitions
 */

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
	u32 format = IPU_PIX_FMT_RGB565;
	u32 size = 2;
	int err = 0;
#ifdef CONFIG_VIDEO_MXC_V4L1
#ifdef CONFIG_FB_MXC_OVERLAY
        u32 byteOffset;
        struct fb_info *fbi = registered_fb[FB_OVL_DEV_INDEX];
#else
        struct fb_info *fbi = registered_fb[FB_DEV_INDEX];
#endif /* CONFIG_FB_MXC_OVERLAY */
#endif /* CONFIG_VIDEO_MXC_V4L1 */

	FUNC_START;

	if (!cam) {
		DPRINTK("private is NULL\n");
		return -EIO;
	}

	if (cam->overlay_active == true) {
		DPRINTK("already started.\n");
		return 0;
	}

	memset(&vf, 0, sizeof(ipu_channel_params_t));
	ipu_csi_get_window_size(&vf.csi_prp_vf_mem.in_width,
				&vf.csi_prp_vf_mem.in_height);
	vf.csi_prp_vf_mem.in_pixel_fmt = IPU_PIX_FMT_YUYV;
#ifdef CONFIG_VIDEO_MXC_V4L1
        size = fbi->var.bits_per_pixel >> 3;
        format = bpp_to_pixfmt(fbi->var.bits_per_pixel);
#ifdef CONFIG_FB_MXC_OVERLAY
        byteOffset = (vfparam.yoffset * fbi->var.xres_virtual + vfparam.xoffset) * \
                     (fbi->var.bits_per_pixel >> 3);
#endif /* CONFIG_FB_MXC_OVERLAY */
        vf.csi_prp_vf_mem.out_width = vfparam.width;
        vf.csi_prp_vf_mem.out_height = vfparam.height;
        if(vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
          vf.csi_prp_vf_mem.out_width = vfparam.height;
          vf.csi_prp_vf_mem.out_height = vfparam.width;
	}
        size = vf.csi_prp_vf_mem.out_width * vf.csi_prp_vf_mem.out_height * size;
#else
	vf.csi_prp_vf_mem.out_width = cam->win.w.width;
	vf.csi_prp_vf_mem.out_height = cam->win.w.height;
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		vf.csi_prp_vf_mem.out_width = cam->win.w.height;
		vf.csi_prp_vf_mem.out_height = cam->win.w.width;
	}
	size = cam->win.w.width * cam->win.w.height * size;
#endif /* CONFIG_VIDEO_MXC_V4L1 */
	vf.csi_prp_vf_mem.out_pixel_fmt = format;

	ipu_uninit_channel(CSI_PRP_VF_MEM);
	err = ipu_init_channel(CSI_PRP_VF_MEM, &vf);
	if (err != 0)
		return err;

	ipu_csi_enable_mclk(CSI_MCLK_VF, true, true);

#if defined(CONFIG_VIDEO_MXC_V4L1) && defined(CONFIG_FB_MXC_OVERLAY)
        if(vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
#endif /* CONFIG_VIDEO_MXC_V4L1 && CONFIG_FB_MXC_OVERLAY */
	if (cam->vf_bufs[0])
		ipu_free((u32) (cam->vf_bufs[0]));
	if (cam->vf_bufs[1])
		ipu_free((u32) (cam->vf_bufs[1]));
	cam->vf_bufs[0] = (void *)ipu_malloc(PAGE_ALIGN(size));
	if (cam->vf_bufs[0] == NULL) {
		DPRINTK("Error to allocate vf buffer\n");
		return -ENOMEM;
	}
	cam->vf_bufs[1] = (void *)ipu_malloc(PAGE_ALIGN(size));
	if (cam->vf_bufs[1] == NULL) {
		ipu_free((u32) (cam->vf_bufs[0]));
		cam->vf_bufs[0] = NULL;
		DPRINTK("Error to allocate vf buffer\n");
		return -ENOMEM;
	}
#if defined(CONFIG_VIDEO_MXC_V4L1) && defined(CONFIG_FB_MXC_OVERLAY)
        }
        else {
          cam->vf_bufs[0] = (void *)(fbi->fix.smem_start + byteOffset);
          cam->vf_bufs[1] = (void *)(fbi->fix.smem_start + (fbi->fix.smem_len >> 1) + byteOffset);
        }
#endif /* CONFIG_FB_MXC_OVERLAY && CONFIG_FB_MXC_OVERLAY */
	DPRINTK("vf_bufs %x %x\n", cam->vf_bufs[0], cam->vf_bufs[1]);

#ifdef CONFIG_VIDEO_MXC_V4L1
        if (vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
#else
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
#endif /* CONFIG_VIDEO_MXC_V4L1 */
		err = ipu_init_channel_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
					      IPU_ROTATE_NONE, cam->vf_bufs[0],
					      cam->vf_bufs[1]);
 		if (err != 0) {
 			ipu_free((u32) (cam->vf_bufs[0]));
 			ipu_free((u32) (cam->vf_bufs[1]));
  			return err;
 		}

#if defined(CONFIG_VIDEO_MXC_V4L1) && defined(CONFIG_FB_MXC_OVERLAY)
                cam->rot_vf_bufs[0] = (void *)(fbi->fix.smem_start + byteOffset);
                cam->rot_vf_bufs[1] = (void *)(fbi->fix.smem_start + (fbi->fix.smem_len >> 1) + \
                                               byteOffset);
#else
		if (cam->rot_vf_bufs[0]) {
 			ipu_free((u32) (cam->rot_vf_bufs[0]));
		}
		if (cam->rot_vf_bufs[1]) {
 			ipu_free((u32) (cam->rot_vf_bufs[1]));
		}

 		cam->rot_vf_bufs[0] = (void *)ipu_malloc(PAGE_ALIGN(size));
		if (cam->rot_vf_bufs[0] == NULL) {
 			ipu_free((u32) (cam->vf_bufs[0]));
 			ipu_free((u32) (cam->vf_bufs[1]));
			DPRINTK("alloc rot_vf_bufs.\n");
			return -ENOMEM;
		}
 		cam->rot_vf_bufs[1] = (void *)ipu_malloc(PAGE_ALIGN(size));
		if (cam->rot_vf_bufs[1] == NULL) {
 			ipu_free((u32) (cam->vf_bufs[0]));
 			ipu_free((u32) (cam->vf_bufs[1]));
 			ipu_free((u32) (cam->rot_vf_bufs[0]));
			DPRINTK("alloc rot_vf_bufs.\n");
			return -ENOMEM;
		}
#endif /* CONFIG_VIDEO_MXC_V4L1 && CONFIG_FB_MXC_OVERLAY */
		DPRINTK("rot_vf_bufs %x %x\n", cam->rot_vf_bufs[0],
			cam->rot_vf_bufs[1]);

		ipu_uninit_channel(MEM_ROT_VF_MEM);
		err = ipu_init_channel(MEM_ROT_VF_MEM, NULL);
		if (err != 0) {
			DPRINTK("Error MEM_ROT_VF_MEM channel\n");
			goto out_3;
		}

		err = ipu_init_channel_buffer(MEM_ROT_VF_MEM, IPU_INPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
#ifdef CONFIG_VIDEO_MXC_V4L1
					      vfparam.rotation, cam->vf_bufs[0],
#else
					      cam->rotation, cam->vf_bufs[0],
#endif /* CONFIG_VIDEO_MXC_V4L1 */
					      cam->vf_bufs[1]);
		if (err != 0) {
			DPRINTK("Error MEM_ROT_VF_MEM input buffer\n");
			goto out_3;
		}

		err = ipu_init_channel_buffer(MEM_ROT_VF_MEM, IPU_OUTPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
#if defined(CONFIG_VIDEO_MXC_V4L1) && defined(CONFIG_FB_MXC_OVERLAY)
					      fbi->var.xres_virtual,
#else
					      vf.csi_prp_vf_mem.out_height,
#endif /* CONFIG_VIDEO_MXC_V4L1 && CONFIG_FB_MXC_OVERLAY */
					      IPU_ROTATE_NONE,
					      cam->rot_vf_bufs[0],
					      cam->rot_vf_bufs[1]);
		if (err != 0) {
			DPRINTK("Error MEM_ROT_VF_MEM output buffer\n");
			goto out_3;
		}

		err = ipu_link_channels(CSI_PRP_VF_MEM, MEM_ROT_VF_MEM);
		if (err < 0) {
			DPRINTK("Error link CSI_PRP_VF_MEM-MEM_ROT_VF_MEM\n");
			goto out_2;
		}

		ipu_uninit_channel(MEM_SDC_FG);
		err = ipu_init_channel(MEM_SDC_FG, NULL);
		if (err != 0)
			goto out_2;

#ifdef CONFIG_VIDEO_MXC_V4L1
#ifdef CONFIG_FB_MXC_OVERLAY
		ipu_sdc_set_window_pos(MEM_SDC_FG, 0, 0);
#else
		ipu_sdc_set_window_pos(MEM_SDC_FG, vfparam.xoffset, vfparam.yoffset);
#endif /* CONFIG_FB_MXC_OVERLAY */
#else
		ipu_sdc_set_window_pos(MEM_SDC_FG, cam->win.w.left,
				       cam->win.w.top);
#endif /* CONFIG_VIDEO_MXC_V4L1 */

		err = ipu_init_channel_buffer(MEM_SDC_FG, IPU_INPUT_BUFFER,
					      format,
#if defined(CONFIG_VIDEO_MXC_V4L1) && defined(CONFIG_FB_MXC_OVERLAY)
					      fbi->var.xres,
					      fbi->var.yres,
					      fbi->var.xres_virtual,
					      IPU_ROTATE_NONE,
					      (void *)fbi->fix.smem_start,
					      (void *)(fbi->fix.smem_start + (fbi->fix.smem_len >> 1)));
#else
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      IPU_ROTATE_NONE,
					      cam->rot_vf_bufs[0],
					      cam->rot_vf_bufs[1]);
#endif /* CONFIG_VIDEO_MXC_V4L1 && CONFIG_FB_MXC_OVERLAY */
		if (err != 0) {
			DPRINTK("Error initializing SDC FG buffer\n");
			goto out_1;
		}

		err = ipu_link_channels(MEM_ROT_VF_MEM, MEM_SDC_FG);
		if (err < 0) {
			DPRINTK("Error link MEM_ROT_VF_MEM-MEM_SDC_FG\n");
			goto out_1;
		}

		ipu_enable_channel(CSI_PRP_VF_MEM);
		ipu_enable_channel(MEM_ROT_VF_MEM);
		ipu_enable_channel(MEM_SDC_FG);

		ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 0);
		ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 1);
		//ipu_select_buffer(MEM_ROT_VF_MEM, IPU_INPUT_BUFFER, 0);
		//ipu_select_buffer(MEM_ROT_VF_MEM, IPU_INPUT_BUFFER, 1);
		ipu_select_buffer(MEM_ROT_VF_MEM, IPU_OUTPUT_BUFFER, 0);
		ipu_select_buffer(MEM_ROT_VF_MEM, IPU_OUTPUT_BUFFER, 1);
		//ipu_select_buffer(MEM_SDC_FG, IPU_INPUT_BUFFER, 0);
		//ipu_select_buffer(MEM_SDC_FG, IPU_INPUT_BUFFER, 1);
	} else {
		err = ipu_init_channel_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER,
#ifdef CONFIG_VIDEO_MXC_V4L1
					      format, vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
#ifdef CONFIG_FB_MXC_OVERLAY
					      fbi->var.xres_virtual, vfparam.rotation,
#else
					      vf.csi_prp_vf_mem.out_width, vfparam.rotation,
#endif /* CONFIG_FB_MXC_OVERLAY */
#else
					      format, cam->win.w.width,
					      cam->win.w.height,
					      cam->win.w.width, cam->rotation,
#endif /* CONFIG_VIDEO_MXC_V4L1 */
					      cam->vf_bufs[0], cam->vf_bufs[1]);
		if (err != 0) {
			DPRINTK("Error initializing CSI_PRP_VF_MEM\n");
			goto out_4;
		}

		ipu_uninit_channel(MEM_SDC_FG);
		err = ipu_init_channel(MEM_SDC_FG, NULL);
		if (err != 0)
			goto out_2;

#ifdef CONFIG_VIDEO_MXC_V4L1
#ifdef CONFIG_FB_MXC_OVERLAY
		ipu_sdc_set_window_pos(MEM_SDC_FG, 0, 0);
#else
		ipu_sdc_set_window_pos(MEM_SDC_FG, vfparam.xoffset, vfparam.yoffset);
#endif /* CONFIG_FB_MXC_OVERLAY */
#else
		ipu_sdc_set_window_pos(MEM_SDC_FG, cam->win.w.left,
				       cam->win.w.top);
#endif /* CONFIG_VIDEO_MXC_V4L1 */
		err = ipu_init_channel_buffer(MEM_SDC_FG,
					      IPU_INPUT_BUFFER, format,
#ifdef CONFIG_VIDEO_MXC_V4L1
#ifdef CONFIG_FB_MXC_OVERLAY
					      fbi->var.xres,
					      fbi->var.yres,
					      fbi->var.xres_virtual, IPU_ROTATE_NONE,
					      (void *)fbi->fix.smem_start,
					      (void *)(fbi->fix.smem_start + (fbi->fix.smem_len >> 1)));
#else
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width, IPU_ROTATE_NONE,
					      cam->vf_bufs[0], cam->vf_bufs[1]);
#endif /* CONFIG_FB_MXC_OVERLAY */
#else
					      cam->win.w.width,
					      cam->win.w.height,
					      cam->win.w.width, IPU_ROTATE_NONE,
					      cam->vf_bufs[0], cam->vf_bufs[1]);
#endif /* CONFIG_VIDEO_MXC_V4L1 */
		if (err != 0) {
			DPRINTK("Error initializing SDC FG buffer\n");
			goto out_1;
		}

		err = ipu_link_channels(CSI_PRP_VF_MEM, MEM_SDC_FG);
		if (err < 0) {
			DPRINTK("Error linking ipu channels\n");
			goto out_1;
		}

		ipu_enable_channel(CSI_PRP_VF_MEM);
		ipu_enable_channel(MEM_SDC_FG);

		ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 0);
		ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 1);
		//ipu_select_buffer(MEM_SDC_FG, IPU_INPUT_BUFFER, 0);
		//ipu_select_buffer(MEM_SDC_FG, IPU_INPUT_BUFFER, 1);
	}

	cam->overlay_active = true;
	FUNC_END;
	return err;

out_1:
	ipu_uninit_channel(MEM_SDC_FG);
out_2:
	ipu_uninit_channel(CSI_PRP_VF_MEM);
out_3:
#ifdef CONFIG_VIDEO_MXC_V4L1
	if (vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
#else
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
#endif /* CONFIG_VIDEO_MXC_V4L1 */
		ipu_uninit_channel(MEM_ROT_VF_MEM);
	}
out_4:
#ifdef CONFIG_VIDEO_MXC_V4L1
	if (vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
#else
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
#endif /* CONFIG_VIDEO_MXC_V4L1 */
#if !defined(CONFIG_VIDEO_MXC_V4L1) || !defined(CONFIG_FB_MXC_OVERLAY)
		if (cam->rot_vf_bufs[0]) {
			ipu_free( (u32)cam->rot_vf_bufs[0]);
			cam->rot_vf_bufs[0] = NULL;
		}
		if (cam->rot_vf_bufs[1]) {
			ipu_free( (u32)cam->rot_vf_bufs[1]);
			cam->rot_vf_bufs[1] = NULL;
		}
#endif /* !defined(CONFIG_VIDEO_MXC_V4L1) || !defined(CONFIG_FB_MXC_OVERLAY) */

#ifdef CONFIG_VIDEO_MXC_V4L1
		if (vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
#else
		if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
#endif /* CONFIG_VIDEO_MXC_V4L1 */
			ipu_disable_channel(MEM_ROT_VF_MEM, true);
		}

		if (cam->vf_bufs[0]) {
			ipu_free( (u32)cam->vf_bufs[0]);
			cam->vf_bufs[0] = NULL;
		}
		if (cam->vf_bufs[1]) {
			ipu_free( (u32)cam->vf_bufs[1]);
			cam->vf_bufs[1] = NULL;
		}
	}
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
	int err = 0;
	FUNC_START;

	if (cam->overlay_active == false)
		return 0;

#ifdef CONFIG_VIDEO_MXC_V4L1
	if(vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
#else
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
#endif /* CONFIG_VIDEO_MXC_V4L1 */
		ipu_unlink_channels(CSI_PRP_VF_MEM, MEM_ROT_VF_MEM);
		ipu_unlink_channels(MEM_ROT_VF_MEM, MEM_SDC_FG);
	} else {
		ipu_unlink_channels(CSI_PRP_VF_MEM, MEM_SDC_FG);
	}

	ipu_disable_channel(MEM_SDC_FG, true);
	ipu_disable_channel(CSI_PRP_VF_MEM, true);

	ipu_uninit_channel(CSI_PRP_VF_MEM);
#ifdef CONFIG_VIDEO_MXC_V4L1
	if(vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
#else
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
#endif /* CONFIG_VIDEO_MXC_V4L1 */
		ipu_uninit_channel(MEM_ROT_VF_MEM);
	}
	ipu_uninit_channel(MEM_SDC_FG);

	ipu_csi_enable_mclk(CSI_MCLK_VF, false, false);
#ifdef CONFIG_VIDEO_MXC_V4L1
	if(vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
#else
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
#endif /* CONFIG_VIDEO_MXC_V4L1 */
		ipu_disable_channel(MEM_ROT_VF_MEM, true);
	}

#if defined(CONFIG_VIDEO_MXC_V4L1) && defined(CONFIG_FB_MXC_OVERLAY)
	if(vfparam.rotation >= IPU_ROTATE_90_RIGHT) {
	  if (cam->vf_bufs[0]) {
	    ipu_free((u32)(cam->vf_bufs[0]));
	    cam->vf_bufs[0] = NULL; 
	  }  
	  if (cam->vf_bufs[1]) {
	    ipu_free((u32)(cam->vf_bufs[1]));
	    cam->vf_bufs[1] = NULL; 
	  }
	}
#else
	if (cam->vf_bufs[0]) {
		ipu_free((u32) (cam->vf_bufs[0]));
		cam->vf_bufs[0] = NULL;
	}
	if (cam->vf_bufs[1]) {
		ipu_free((u32) (cam->vf_bufs[1]));
		cam->vf_bufs[1] = NULL;
	}
	if (cam->rot_vf_bufs[0]) {
		ipu_free((u32) (cam->rot_vf_bufs[0]));
		cam->vf_bufs[0] = NULL;
	}
	if (cam->rot_vf_bufs[1]) {
		ipu_free((u32) (cam->rot_vf_bufs[1]));
		cam->vf_bufs[1] = NULL;
	}
#endif /* CONFIG_VIDEO_MXC_V4L1 */

	cam->overlay_active = false;
	FUNC_END;
	return err;
}

/*!
 * function to select PRP-VF as the working path
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 * @return  status
 */
int prp_vf_sdc_select(void *private)
{
	cam_data *cam;
	int err = 0;
	FUNC_START;
	if (private) {
		cam = (cam_data *) private;
		cam->vf_start_sdc = prpvf_start;
		cam->vf_stop_sdc = prpvf_stop;
		cam->overlay_active = false;
	} else
		err = -EIO;

	FUNC_END;
	return err;
}

/*!
 * function to de-select PRP-VF as the working path
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 * @return  int
 */
int prp_vf_sdc_deselect(void *private)
{
	cam_data *cam;
	int err = 0;
	FUNC_START;
	err = prpvf_stop(private);

	if (private) {
		cam = (cam_data *) private;
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
__init int prp_vf_sdc_init(void)
{
	u8 err = 0;
	FUNC_START;
	FUNC_END;
	return err;
}

/*!
 * Deinit viewfinder task.
 *
 * @return  Error code indicating success or failure
 */
void __exit prp_vf_sdc_exit(void)
{
	FUNC_START;
	FUNC_END;
}

module_init(prp_vf_sdc_init);
module_exit(prp_vf_sdc_exit);

EXPORT_SYMBOL(prp_vf_sdc_select);
EXPORT_SYMBOL(prp_vf_sdc_deselect);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("IPU PRP VF SDC Driver");
MODULE_LICENSE("GPL");
