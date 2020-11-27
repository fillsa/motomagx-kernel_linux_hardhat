/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2005-2006 Motorola Inc.
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
Author                 Date        Description of Changes
----------------   ------------    -------------------------
Motorola            08/29/2005     Modifications to facilitate
                                   usage by EZX camera driver.

Motorola            09/12/2005     Upmerge to Montavista release

Motorola            10/04/2005     Make changes associated with
                                   viewfinder hang:
                                   a. disable double buffer mode
                                   b. set NSB bit in DMA channel
                                   c. clear out BUF0_RDY bit
                                      for channel 0 after disabling
                                      channel
Motorola            10/18/2005     Remove the clearing of status
                                   bits: DMAIC0_EOF, CSI_NF, and
                                   CSI_EOF
Motorola            01/04/2006     Upmerge to MV 12/19/05 release
Motorola            06/09/2006     Upmerge to MV 05/12/2006
 */

/*!
 * @file ipu_prp_enc.c
 *
 * @brief IPU Use case for PRP-ENC
 *
 * @ingroup IPU
 */

#include "../drivers/media/video/mxc/capture/mxc_v4l2_capture.h"
#include "../drivers/mxc/ipu/ipu.h"
#include "ipu_prp_sw.h"
#include "../drivers/mxc/ipu/ipu_regs.h"

//#define MXC_PRP_ENC_DEBUG
#ifdef MXC_PRP_ENC_DEBUG

#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " \
          fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START DPRINTK(" func start\n")
#  define FUNC_END DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n",\
           __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* MXC_PRP_ENC_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* MXC_PRP_ENC_DEBUG */

static ipu_rotate_mode_t grotation = IPU_ROTATE_NONE;


#ifdef CONFIG_VIDEO_MXC_V4L1
extern void v4l1_camera_callback(u32 mask);
extern ipu_csi_signal_cfg_t param;
extern uint32_t p_mclk_freq;
#endif

/*
 * Function definitions
 */

/*!
 * IPU ENC callback function.
 *
 * @param irq       int irq line
 * @param dev_id    void * device id
 * @param regs      struct pt_regs *
 *
 * @return status   IRQ_HANDLED for handled
 */
static irqreturn_t prp_enc_callback(int irq, void *dev_id, struct pt_regs *regs)
{
	FUNC_START;

#ifdef CONFIG_VIDEO_MXC_V4L1
	v4l1_camera_callback(irq);
#else
	camera_callback(irq, dev_id);
#endif

	FUNC_END;

	return IRQ_HANDLED;
}

/*!
 * PrpENC enable channel setup function
 *
 * @param cam       struct cam_data * mxc capture instance
 *
 * @return  status
 */
#ifdef CONFIG_VIDEO_MXC_V4L1
int 
#else
static int 
#endif
prp_enc_setup(cam_data * cam)
{
	ipu_channel_params_t enc;
	int err = 0;
	void *dummy = (void *)0xdeadbeaf;

	FUNC_START;

	if (!cam) {
		DPRINTK("cam private is NULL\n");
		return -ENXIO;
	}

	ipu_csi_get_window_size(&enc.csi_prp_enc_mem.in_width,
				&enc.csi_prp_enc_mem.in_height);
	enc.csi_prp_enc_mem.in_pixel_fmt = IPU_PIX_FMT_YUYV;
	enc.csi_prp_enc_mem.out_width = cam->v2f.fmt.pix.width;
	enc.csi_prp_enc_mem.out_height = cam->v2f.fmt.pix.height;
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		enc.csi_prp_enc_mem.out_width = cam->v2f.fmt.pix.height;
		enc.csi_prp_enc_mem.out_height = cam->v2f.fmt.pix.width;
	}
	if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_YUV420P;
		DPRINTK("YUV420\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV422P) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_YVU422P;
		DPRINTK("YUV422P\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_BGR24) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_BGR24;
		DPRINTK("BGR24\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_RGB24;
		DPRINTK("RGB24\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_RGB565;
		DPRINTK("RGB565\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_BGR32) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_BGR32;
		DPRINTK("BGR32\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB32) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_RGB32;
		DPRINTK("RGB32\n");
	} else {
		DPRINTK("format not supported\n");
		return -EINVAL;
	}

	ipu_uninit_channel(CSI_PRP_ENC_MEM);
	err = ipu_init_channel(CSI_PRP_ENC_MEM, &enc);
	if (err != 0) {
		DPRINTK("ipu_init_channel %d\n", err);
		return err;
	}
	ipu_csi_enable_mclk(CSI_MCLK_ENC, true, true);

	grotation = cam->rotation;
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		if (cam->rot_enc_bufs[0]) {
			ipu_free((u32) (cam->rot_enc_bufs[0]));
		}
		if (cam->rot_enc_bufs[1]) {
			ipu_free((u32) (cam->rot_enc_bufs[1]));
		}
		cam->rot_enc_bufs[0] = (void *)
		    ipu_malloc(PAGE_ALIGN(cam->v2f.fmt.pix.sizeimage));
		if (!cam->rot_enc_bufs[0]) {
			DPRINTK("alloc enc_bufs0\n");
			return -ENOMEM;
		}
		cam->rot_enc_bufs[1] = (void *)
		    ipu_malloc(PAGE_ALIGN(cam->v2f.fmt.pix.sizeimage));
		if (!cam->rot_enc_bufs[1]) {
			ipu_free((u32) (cam->rot_enc_bufs[0]));
			cam->rot_enc_bufs[0] = NULL;
			DPRINTK("alloc enc_bufs1\n");
			return -ENOMEM;
		}

		err = ipu_init_channel_buffer(CSI_PRP_ENC_MEM,
					      IPU_OUTPUT_BUFFER,
					      enc.csi_prp_enc_mem.out_pixel_fmt,
					      enc.csi_prp_enc_mem.out_width,
					      enc.csi_prp_enc_mem.out_height,
					      enc.csi_prp_enc_mem.out_width,
					      IPU_ROTATE_NONE,
					      cam->rot_enc_bufs[0],
					      cam->rot_enc_bufs[1]);
		if (err != 0) {
			DPRINTK("CSI_PRP_ENC_MEM err\n");
			return err;
		}

		ipu_uninit_channel(MEM_ROT_ENC_MEM);
		err = ipu_init_channel(MEM_ROT_ENC_MEM, NULL);
		if (err != 0) {
			DPRINTK("MEM_ROT_ENC_MEM channel err\n");
			return err;
		}

		err = ipu_init_channel_buffer(MEM_ROT_ENC_MEM, IPU_INPUT_BUFFER,
					      enc.csi_prp_enc_mem.out_pixel_fmt,
					      enc.csi_prp_enc_mem.out_width,
					      enc.csi_prp_enc_mem.out_height,
					      enc.csi_prp_enc_mem.out_width,
					      cam->rotation,
					      cam->rot_enc_bufs[0],
					      cam->rot_enc_bufs[1]);
		if (err != 0) {
			DPRINTK("Error MEM_ROT_ENC_MEM input buffer\n");
			return err;
		}

		err =
		    ipu_init_channel_buffer(MEM_ROT_ENC_MEM, IPU_OUTPUT_BUFFER,
					    enc.csi_prp_enc_mem.out_pixel_fmt,
					    enc.csi_prp_enc_mem.out_height,
					    enc.csi_prp_enc_mem.out_width,
					    enc.csi_prp_enc_mem.out_height,
					    IPU_ROTATE_NONE, dummy, dummy);
		if (err != 0) {
			DPRINTK("Error MEM_ROT_ENC_MEM output buffer\n");
			return err;
		}

		err = ipu_link_channels(CSI_PRP_ENC_MEM, MEM_ROT_ENC_MEM);
		if (err < 0) {
			DPRINTK("Err link CSI_PRP_ENC_MEM-MEM_ROT_ENC_MEM\n");
			return err;
		}

		err = ipu_enable_channel(CSI_PRP_ENC_MEM);
		if (err < 0) {
			DPRINTK("Err ipu_enable_channel CSI_PRP_ENC_MEM\n");
			return err;
		}
		err = ipu_enable_channel(MEM_ROT_ENC_MEM);
		if (err < 0) {
			DPRINTK("Err ipu_enable_channel MEM_ROT_ENC_MEM\n");
			return err;
		}

		ipu_select_buffer(CSI_PRP_ENC_MEM, IPU_OUTPUT_BUFFER, 0);
		ipu_select_buffer(CSI_PRP_ENC_MEM, IPU_OUTPUT_BUFFER, 1);
	} else {
#ifdef CONFIG_VIDEO_MXC_V4L1
	  /* NOTE: do not use double buffer for SCM-A11 camera for now */
	  /* TODO: need to remove this limitation */
		err = 
		    ipu_init_channel_buffer(CSI_PRP_ENC_MEM, IPU_OUTPUT_BUFFER, 
					    enc.csi_prp_enc_mem.out_pixel_fmt, 
					    enc.csi_prp_enc_mem.out_width, 
					    enc.csi_prp_enc_mem.out_height,	
					    enc.csi_prp_enc_mem.out_width, 
					    cam->rotation, dummy, NULL);
#else
		err =
		    ipu_init_channel_buffer(CSI_PRP_ENC_MEM, IPU_OUTPUT_BUFFER,
					    enc.csi_prp_enc_mem.out_pixel_fmt,
					    enc.csi_prp_enc_mem.out_width,
					    enc.csi_prp_enc_mem.out_height,
					    enc.csi_prp_enc_mem.out_width,
					    cam->rotation, dummy, dummy);
#endif
		if (err != 0) {
			DPRINTK("Error CSI_PRP_ENC_MEM output buffer\n");
			return err;
		}
		err = ipu_enable_channel(CSI_PRP_ENC_MEM);
		if (err < 0) {
			DPRINTK("Err ipu_enable_channel CSI_PRP_ENC_MEM\n");
			return err;
		}
	}

#ifdef CONFIG_VIDEO_MXC_V4L1
	/* NOTE: overwrite default settings for u_offset and v_offset */
	/* TODO: currently using fixed values, need a more flexible API */
	volatile unsigned long *ipu_addr = (unsigned long *)(0xd42c0000+0x20);
	volatile unsigned long *ipu_data = (unsigned long *)(0xd42c0000+0x24);

	/* NOTE: 127--word3-95--word2-63--word1-31--------0 */
	/* NOTE:      104--vbo--79 78--ubo--53              */
	/* NOTE: ubo = bits 78-53, vbo = bits 104-79 */

	/* NOTE: read 3 32-bit words from IPU channel parameter memory */
	*ipu_addr = 0x00010001;
	unsigned long word1 = *ipu_data;
	unsigned long word2 = *ipu_data;
	unsigned long word3 = *ipu_data;
        
	/* NOTE: desired ubo and vbo */
	unsigned long ubo = PAGE_ALIGN(enc.csi_prp_enc_mem.out_width * \
				       enc.csi_prp_enc_mem.out_height);

	unsigned long vbo = ubo + PAGE_ALIGN(enc.csi_prp_enc_mem.out_width *\
					     enc.csi_prp_enc_mem.out_height / 2);

	/* NOTE: divide ubo and vbo into 2 parts because they cross
             32-bit boundaries */
	unsigned long ubo_low = ubo & ((1<<(64-53))-1); /* NOTE: 11 bits */
	unsigned long ubo_high = (ubo >> (64-53)) & 0x00007fff; /* NOTE: 15 bits */

	unsigned long vbo_low = vbo & ((1<<(96-79))-1); /* NOTE: 17 bits */
	unsigned long vbo_high = (vbo >> (96-79)) & 0x000001ff; /* NOTE: 9 bits */

	/* NOTE: save ubo and vbo into relevant portions of the 3 32-bit words */
	word1 = (word1 & 0x001fffff) | (ubo_low << 21);
	word2 = (vbo_low << 15) | ubo_high;
	word3 = (word3 & 0xfffffe00) | vbo_high;

	/* NOTE: write to IPU channel parameter memory */
	*ipu_addr = 0x00010001;
	*ipu_data = word1 | 0x00004000; /* NOTE: spec says need to set NSB bit */
	*ipu_data = word2;
	*ipu_data = word3;
#endif

	FUNC_END;
	return err;
}

/*!
 * function to update physical buffer address for encorder IDMA channel
 *
 * @param eba         u32 physical buffer address for encorder IDMA channel
 * @param buffer_num  int buffer 0 or buffer 1
 *
 * @return  status
 */
static int prp_enc_eba_update(u32 eba, int *buffer_num)
{
	int err = 0;
	FUNC_START;

	DPRINTK("eba %x\n", eba);
	if (grotation >= IPU_ROTATE_90_RIGHT) {
		err = ipu_update_channel_buffer(MEM_ROT_ENC_MEM,
						IPU_OUTPUT_BUFFER, *buffer_num,
						(u8 *) (eba));
	} else {
		err = ipu_update_channel_buffer(CSI_PRP_ENC_MEM,
						IPU_OUTPUT_BUFFER, *buffer_num,
						(u8 *) (eba));
	}
	if (err != 0) {
		DPRINTK("err %d buffer_num %d\n", err, *buffer_num);
		return err;
	}

	if (grotation >= IPU_ROTATE_90_RIGHT) {
		ipu_select_buffer(MEM_ROT_ENC_MEM, IPU_OUTPUT_BUFFER,
				  *buffer_num);
	} else {
		ipu_select_buffer(CSI_PRP_ENC_MEM, IPU_OUTPUT_BUFFER,
				  *buffer_num);
	}

#ifdef CONFIG_VIDEO_MXC_V4L1
	/* NOTE: do not use double buffer mode for SCM-A11 camera for now */
	/* TODO: need to remove this limitation */
#else
	*buffer_num = (*buffer_num == 0) ? 1 : 0;
#endif
	FUNC_END;
	return 0;
}

/*!
 * Enable encoder task
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int prp_enc_enabling_tasks(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;
	FUNC_START;

	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		err = ipu_request_irq(IPU_IRQ_PRP_ENC_ROT_OUT_EOF,
				      prp_enc_callback, 0, "Mxc Camera", cam);
	} else {
		err = ipu_request_irq(IPU_IRQ_PRP_ENC_OUT_EOF,
				      prp_enc_callback, 0, "Mxc Camera", cam);
	}
	if (err != 0) {
		DPRINTK("Error registering rot irq\n");
		return err;
	}

#ifdef CONFIG_VIDEO_MXC_V4L1
	/* NOTE: call these functions inside ci_set_image_format in mxc_v4l1.c */
	/* TODO: investigate why calling these functions here causes occasional hangs */
#else
	err = prp_enc_setup(cam);
	if (err != 0) {
		DPRINTK("prp_enc_setup %d\n", err);
		return err;
	}
#endif

	FUNC_END;
	return err;
}

/*!
 * Disable encoder task
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  int
 */
static int prp_enc_disabling_tasks(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;
#ifdef CONFIG_VIDEO_MXC_V4L1
	unsigned long reg;
#endif
	FUNC_START;

	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		ipu_free_irq(IPU_IRQ_PRP_ENC_ROT_OUT_EOF, cam);
	} else {
		ipu_free_irq(IPU_IRQ_PRP_ENC_OUT_EOF, cam);
	}

	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		ipu_unlink_channels(CSI_PRP_ENC_MEM, MEM_ROT_ENC_MEM);
	}

	err = ipu_disable_channel(CSI_PRP_ENC_MEM, true);
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		err |= ipu_disable_channel(MEM_ROT_ENC_MEM, true);
	}

	ipu_uninit_channel(CSI_PRP_ENC_MEM);
	if (cam->rotation >= IPU_ROTATE_90_RIGHT) {
		ipu_uninit_channel(MEM_ROT_ENC_MEM);
	}

	ipu_csi_enable_mclk(CSI_MCLK_ENC, false, false);

#ifdef CONFIG_VIDEO_MXC_V4L1
	/* NOTE: forcibly clear BUF0_RDY */
	/* TODO: investigate why this bit is never cleared,
	         causing wait-loop to timeout in ipu_disable_channel */
	/* NOTE: assuming no double-buffering */
	reg = readl(IPU_CHA_BUF0_RDY);
	reg &= ~(0x00000001);
	writel(reg, IPU_CHA_BUF0_RDY);
#endif

	FUNC_END;
	return err;
}

/*!
 * function to select PRP-ENC as the working path
 *
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  int
 */
int prp_enc_select(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;

	FUNC_START;
	if (cam) {
		cam->enc_update_eba = prp_enc_eba_update;
		cam->enc_enable = prp_enc_enabling_tasks;
		cam->enc_disable = prp_enc_disabling_tasks;
	} else {
		err = -EIO;
	}

	FUNC_END;
	return err;
}

/*!
 * function to de-select PRP-ENC as the working path
 *
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  int
 */
int prp_enc_deselect(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;
	FUNC_START;

	err = prp_enc_disabling_tasks(cam);

	if (cam) {
		cam->enc_update_eba = NULL;
		cam->enc_enable = NULL;
		cam->enc_disable = NULL;
		if (cam->rot_enc_bufs[0]) {
			ipu_free((u32) (cam->rot_enc_bufs[0]));
			cam->rot_enc_bufs[0] = NULL;
		}
		if (cam->rot_enc_bufs[1]) {
			ipu_free((u32) (cam->rot_enc_bufs[1]));
			cam->rot_enc_bufs[1] = NULL;
		}
	}

	FUNC_END;
	return err;
}

/*!
 * Init the Encorder channels
 *
 * @return  Error code indicating success or failure
 */
__init int prp_enc_init(void)
{
	FUNC_START;

	FUNC_END;
	return 0;
}

/*!
 * Deinit the Encorder channels
 *
 */
void __exit prp_enc_exit(void)
{
	FUNC_START;

	FUNC_END;
}

module_init(prp_enc_init);
module_exit(prp_enc_exit);

EXPORT_SYMBOL(prp_enc_select);
EXPORT_SYMBOL(prp_enc_deselect);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("IPU PRP ENC Driver");
MODULE_LICENSE("GPL");
