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
 * @file ipu_prp_vf_adc.c
 *
 * @brief IPU Use case for PRP-VF
 *
 * @ingroup IPU
 */

#include "../drivers/media/video/mxc/capture/mxc_v4l2_capture.h"
#include "ipu_prp_sw.h"
#include "../drivers/video/mxc/mxcfb.h"
#include "../drivers/mxc/ipu/ipu.h"

//#define MXC_PRP_VF_ADC_DEBUG
#ifdef MXC_PRP_VF_ADC_DEBUG

#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " \
          fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START DPRINTK(" func start\n")
#  define FUNC_END DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n",\
          __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* MXC_PRP_VF_ADC_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* MXC_PRP_VF_ADC_DEBUG */

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
	ipu_channel_params_t params;
	u32 format = IPU_PIX_FMT_RGB565;
	u32 size = 2;
	int err = 0;

	FUNC_START;

	if (!cam) {
		DPRINTK("prpvf_start private is NULL\n");
		return -ENXIO;
	}

	if (cam->overlay_active == true) {
		DPRINTK("prpvf_start already start.\n");
		return 0;
	}

	mxcfb_set_refresh_mode(cam->overlay_fb, MXCFB_REFRESH_OFF, 0);

	memset(&vf, 0, sizeof(ipu_channel_params_t));
	ipu_csi_get_window_size(&vf.csi_prp_vf_adc.in_width,
				&vf.csi_prp_vf_adc.in_height);
	vf.csi_prp_vf_adc.in_pixel_fmt = IPU_PIX_FMT_YUYV;
	vf.csi_prp_vf_adc.out_width = cam->win.w.width;
	vf.csi_prp_vf_adc.out_height = cam->win.w.height;
	vf.csi_prp_vf_adc.graphics_combine_en = 0;
	vf.csi_prp_vf_adc.out_left = cam->win.w.left;

	// going to be removed when those offset taken cared by adc driver.
#ifdef CONFIG_FB_MXC_EPSON_QVGA_PANEL
	vf.csi_prp_vf_adc.out_left += 12;
#endif
#ifdef CONFIG_FB_MXC_EPSON_PANEL
	vf.csi_prp_vf_adc.out_left += 2;
#endif

	vf.csi_prp_vf_adc.out_top = cam->win.w.top;

	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		vf.csi_prp_vf_adc.out_width = cam->win.w.height;
		vf.csi_prp_vf_adc.out_height = cam->win.w.width;

		size = cam->win.w.width * cam->win.w.height * size;
		vf.csi_prp_vf_adc.out_pixel_fmt = format;
		ipu_uninit_channel(CSI_PRP_VF_MEM);
		err = ipu_init_channel(CSI_PRP_VF_MEM, &vf);
		if (err != 0)
			return err;
		ipu_csi_enable_mclk(CSI_MCLK_VF, true, true);
		if (cam->vf_bufs[0])
			ipu_free((u32) (cam->vf_bufs[0]));
		if (cam->vf_bufs[1])
			ipu_free((u32) (cam->vf_bufs[1]));
		cam->vf_bufs[0] = (void *)ipu_malloc(PAGE_ALIGN(size));
		if (cam->vf_bufs[0] == NULL) {
			DPRINTK("prpvf_start: Error to allocate vf buffer\n");
			return -ENOMEM;
		}
		cam->vf_bufs[1] = (void *)ipu_malloc(PAGE_ALIGN(size));
		if (cam->vf_bufs[1] == NULL) {
			ipu_free((u32) (cam->vf_bufs[0]));
			cam->vf_bufs[0] = NULL;
			DPRINTK("prpvf_start: Error to allocate vf buffer\n");
			return -ENOMEM;
		}
		err = ipu_init_channel_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
					      IPU_ROTATE_NONE,
					      cam->vf_bufs[0], cam->vf_bufs[1]);
		if (err != 0)
			return err;

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
			DPRINTK("prpvf_start: Error to allocate rot_vf_bufs\n");
			return -ENOMEM;
		}
		cam->rot_vf_bufs[1] = (void *)ipu_malloc(PAGE_ALIGN(size));
		if (cam->rot_vf_bufs[1] == NULL) {
			ipu_free((u32) (cam->vf_bufs[0]));
			ipu_free((u32) (cam->vf_bufs[1]));
			ipu_free((u32) (cam->rot_vf_bufs[0]));
			cam->rot_vf_bufs[0] = NULL;
			DPRINTK("prpvf_start: Error to allocate rot_vf_bufs\n");
			return -ENOMEM;
		}

		ipu_uninit_channel(MEM_ROT_VF_MEM);
		err = ipu_init_channel(MEM_ROT_VF_MEM, NULL);
		if (err != 0) {
			DPRINTK("prpvf_start :Error "
				"MEM_ROT_VF_MEM channel\n");
			return err;
		}

		err = ipu_init_channel_buffer(MEM_ROT_VF_MEM, IPU_INPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
					      cam->rotation, cam->vf_bufs[0],
					      cam->vf_bufs[1]);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"MEM_ROT_VF_MEM input buffer\n");
			return err;
		}

		err = ipu_init_channel_buffer(MEM_ROT_VF_MEM, IPU_OUTPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      IPU_ROTATE_NONE,
					      cam->rot_vf_bufs[0],
					      cam->rot_vf_bufs[1]);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"MEM_ROT_VF_MEM output buffer\n");
			return err;
		}

		err = ipu_link_channels(CSI_PRP_VF_MEM, MEM_ROT_VF_MEM);
		if (err < 0) {
			DPRINTK("prpvf_start: Error "
				"linking CSI_PRP_VF_MEM-MEM_ROT_VF_MEM\n");
			return err;
		}

		ipu_uninit_channel(ADC_SYS2);
		ipu_disable_channel(ADC_SYS2, false);

		params.adc_sys2.disp = DISP0;
		params.adc_sys2.ch_mode = WriteTemplateNonSeq;
		params.adc_sys2.out_left = cam->win.w.left;
		// going to be removed when those offset taken cared by adc driver.
#ifdef CONFIG_FB_MXC_EPSON_QVGA_PANEL
		params.adc_sys2.out_left += 12;
#endif
#ifdef CONFIG_FB_MXC_EPSON_PANEL
		params.adc_sys2.out_left += 2;
#endif
		params.adc_sys2.out_top = cam->win.w.top;
		err = ipu_init_channel(ADC_SYS2, &params);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"initializing ADC SYS1\n");
			return err;
		}

		err = ipu_init_channel_buffer(ADC_SYS2, IPU_INPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      IPU_ROTATE_NONE,
					      cam->rot_vf_bufs[0],
					      cam->rot_vf_bufs[1]);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"initializing ADC SYS1 buffer\n");
			return err;
		}

		err = ipu_link_channels(MEM_ROT_VF_MEM, ADC_SYS2);
		if (err < 0) {
			DPRINTK("prpvf_start: Error "
				"linking MEM_ROT_VF_MEM-ADC_SYS2\n");
			return err;
		}

		ipu_enable_channel(CSI_PRP_VF_MEM);
		ipu_enable_channel(MEM_ROT_VF_MEM);
		ipu_enable_channel(ADC_SYS2);

		ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 0);
		ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 1);
		//ipu_select_buffer(MEM_ROT_VF_MEM, IPU_INPUT_BUFFER, 0);
		//ipu_select_buffer(MEM_ROT_VF_MEM, IPU_INPUT_BUFFER, 1);
		ipu_select_buffer(MEM_ROT_VF_MEM, IPU_OUTPUT_BUFFER, 0);
		ipu_select_buffer(MEM_ROT_VF_MEM, IPU_OUTPUT_BUFFER, 1);
		//ipu_select_buffer(ADC_SYS2, IPU_INPUT_BUFFER, 0);
		//ipu_select_buffer(ADC_SYS2, IPU_INPUT_BUFFER, 1);
	}
#ifndef CONFIG_MXC_IPU_PRP_VF_SDC
	else if (cam->rotation == IPU_ROTATE_NONE) {
		vf.csi_prp_vf_adc.out_pixel_fmt = IPU_PIX_FMT_BGR32;
		ipu_uninit_channel(CSI_PRP_VF_ADC);
		err = ipu_init_channel(CSI_PRP_VF_ADC, &vf);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"initializing CSI_PRP_VF_ADC\n");
			return err;
		}
		ipu_csi_enable_mclk(CSI_MCLK_VF, true, true);
		err = ipu_init_channel_buffer(CSI_PRP_VF_ADC, IPU_OUTPUT_BUFFER,
					      format, cam->win.w.width,
					      cam->win.w.height,
					      cam->win.w.width, IPU_ROTATE_NONE,
					      NULL, NULL);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"initializing CSI_PRP_VF_MEM\n");
			return err;
		}
		ipu_enable_channel(CSI_PRP_VF_ADC);
	}
#endif
	else {
		size = cam->win.w.width * cam->win.w.height * size;
		vf.csi_prp_vf_adc.out_pixel_fmt = format;
		ipu_uninit_channel(CSI_PRP_VF_MEM);
		err = ipu_init_channel(CSI_PRP_VF_MEM, &vf);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"initializing CSI_PRP_VF_MEM\n");
			return err;
		}
		ipu_csi_enable_mclk(CSI_MCLK_VF, true, true);
		if (cam->vf_bufs[0])
			ipu_free((u32) (cam->vf_bufs[0]));
		if (cam->vf_bufs[1])
			ipu_free((u32) (cam->vf_bufs[1]));
		cam->vf_bufs[0] = (void *)ipu_malloc(PAGE_ALIGN(size));
		if (cam->vf_bufs[0] == NULL) {
			DPRINTK("prpvf_start: Error to allocate vf_bufs\n");
			return -ENOMEM;
		}
		cam->vf_bufs[1] = (void *)ipu_malloc(PAGE_ALIGN(size));
		if (cam->vf_bufs[1] == NULL) {
			ipu_free((u32) (cam->vf_bufs[0]));
			cam->vf_bufs[0] = NULL;
			DPRINTK("prpvf_start: Error to allocate vf_bufs\n");
			return -ENOMEM;
		}
		err = ipu_init_channel_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
					      cam->rotation,
					      cam->vf_bufs[0], cam->vf_bufs[1]);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"initializing CSI_PRP_VF_MEM\n");
			return err;
		}

		ipu_uninit_channel(ADC_SYS2);
		ipu_disable_channel(ADC_SYS2, false);

		params.adc_sys2.disp = DISP0;
		params.adc_sys2.ch_mode = WriteTemplateNonSeq;
		params.adc_sys2.out_left = cam->win.w.left;
		// going to be removed when those offset taken cared by adc driver.
#ifdef CONFIG_FB_MXC_EPSON_QVGA_PANEL
		params.adc_sys2.out_left += 12;
#endif
#ifdef CONFIG_FB_MXC_EPSON_PANEL
		params.adc_sys2.out_left += 2;
#endif
		params.adc_sys2.out_top = cam->win.w.top;
		err = ipu_init_channel(ADC_SYS2, &params);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"initializing ADC_SYS2\n");
			return err;
		}

		err = ipu_init_channel_buffer(ADC_SYS2, IPU_INPUT_BUFFER,
					      format,
					      vf.csi_prp_vf_mem.out_width,
					      vf.csi_prp_vf_mem.out_height,
					      vf.csi_prp_vf_mem.out_width,
					      IPU_ROTATE_NONE, cam->vf_bufs[0],
					      cam->vf_bufs[1]);
		if (err != 0) {
			DPRINTK("prpvf_start: Error "
				"initializing ADC SYS1 buffer\n");
			return err;
		}

		err = ipu_link_channels(CSI_PRP_VF_MEM, ADC_SYS2);
		if (err < 0) {
			DPRINTK("prpvf_start: Error "
				"linking MEM_ROT_VF_MEM-ADC_SYS2\n");
			return err;
		}

		ipu_enable_channel(CSI_PRP_VF_MEM);
		ipu_enable_channel(ADC_SYS2);

		ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 0);
		ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 1);
		//ipu_select_buffer(ADC_SYS2, IPU_INPUT_BUFFER, 0);
		//ipu_select_buffer(ADC_SYS2, IPU_INPUT_BUFFER, 1);
	}

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
	int err = 0;
	FUNC_START;

	if (cam->overlay_active == false)
		return 0;

	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		ipu_unlink_channels(CSI_PRP_VF_MEM, MEM_ROT_VF_MEM);
		ipu_unlink_channels(MEM_ROT_VF_MEM, ADC_SYS2);

		ipu_disable_channel(CSI_PRP_VF_MEM, true);
		ipu_disable_channel(MEM_ROT_VF_MEM, true);
		ipu_disable_channel(ADC_SYS2, true);

		ipu_uninit_channel(CSI_PRP_VF_MEM);
		ipu_uninit_channel(MEM_ROT_VF_MEM);
		ipu_uninit_channel(ADC_SYS2);

		ipu_csi_enable_mclk(CSI_MCLK_VF, false, false);
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
	}
#ifndef CONFIG_MXC_IPU_PRP_VF_SDC
	else if (cam->rotation == IPU_ROTATE_NONE) {
		ipu_uninit_channel(CSI_PRP_VF_ADC);
		ipu_disable_channel(CSI_PRP_VF_ADC, true);
		ipu_csi_enable_mclk(CSI_MCLK_VF, false, false);
	}
#endif
	else {
		ipu_unlink_channels(CSI_PRP_VF_MEM, ADC_SYS2);

		ipu_disable_channel(CSI_PRP_VF_MEM, true);
		ipu_disable_channel(ADC_SYS2, true);

		ipu_uninit_channel(CSI_PRP_VF_MEM);
		ipu_uninit_channel(ADC_SYS2);

		ipu_csi_enable_mclk(CSI_MCLK_VF, false, false);

		if (cam->vf_bufs[0]) {
			ipu_free((u32) (cam->vf_bufs[0]));
			cam->vf_bufs[0] = NULL;
		}
		if (cam->vf_bufs[1]) {
			ipu_free((u32) (cam->vf_bufs[1]));
			cam->vf_bufs[1] = NULL;
		}
	}

	cam->overlay_active = false;

	mxcfb_set_refresh_mode(cam->overlay_fb, MXCFB_REFRESH_PARTIAL, 0);
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
int prp_vf_adc_select(void *private)
{
	cam_data *cam;
	FUNC_START;
	if (private) {
		cam = (cam_data *) private;
		cam->vf_start_adc = prpvf_start;
		cam->vf_stop_adc = prpvf_stop;
		cam->overlay_active = false;
	} else {
		return -EIO;
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
int prp_vf_adc_deselect(void *private)
{
	cam_data *cam;
	int err = 0;
	FUNC_START;
	err = prpvf_stop(private);

	if (private) {
		cam = (cam_data *) private;
		cam->vf_start_adc = NULL;
		cam->vf_stop_adc = NULL;
	}
	FUNC_END;
	return err;
}

/*!
 * Init viewfinder task.
 *
 * @return  Error code indicating success or failure
 */
__init int prp_vf_adc_init(void)
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
void __exit prp_vf_adc_exit(void)
{
	FUNC_START;
	FUNC_END;
}

module_init(prp_vf_adc_init);
module_exit(prp_vf_adc_exit);

EXPORT_SYMBOL(prp_vf_adc_select);
EXPORT_SYMBOL(prp_vf_adc_deselect);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("IPU PRP VF Driver");
MODULE_LICENSE("GPL");
