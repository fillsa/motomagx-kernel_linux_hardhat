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
 *
 * Date         Author          Comment
 * ===========  ==============  ==============================================
 * 17-Mar-2006  Motorola        Added support for full-sized capture
 * 16-Jun-2006  Motorola        Use generic_32 data format during capture
 * 14-Aug-2006  Motorola        Generate dummy transfer from GEM_AP during
 *                              the 2MP image capture
 */

/*!
 * @file ipu_still.c
 *
 * @brief IPU Use case for still image capture
 *
 * @ingroup IPU
 */

#include <asm/semaphore.h>
#include "../drivers/media/video/mxc/capture/mxc_v4l2_capture.h"
#include "../drivers/mxc/ipu/ipu.h"
#include "ipu_prp_sw.h"

#ifdef CONFIG_MOT_FEAT_2MP_CAMERA_WRKARND
#include "capture_2mp_wrkarnd.h"
#endif

//#define MXC_PRP_STILL_DEBUG
#ifdef MXC_PRP_STILL_DEBUG

#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " \
          fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START DPRINTK(" func start\n")
#  define FUNC_END DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", \
          __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* MXC_PRP_STILL_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* MXC_PRP_STILL_DEBUG */

extern void v4l1_camera_callback(u32);

static int callback_flag;
/*
 * Function definitions
 */

/*!
 * CSI EOF callback function.
 *
 * @param irq       int irq line
 * @param dev_id    void * device id
 * @param regs      struct pt_regs *
 *
 * @return status   IRQ_HANDLED for handled
 */
static irqreturn_t
prp_csi_eof_callback(int irq, void *dev_id, struct pt_regs *regs)
{
	FUNC_START;
	if (callback_flag == 2) {

#ifdef CONFIG_MOT_FEAT_2MP_CAMERA_WRKARND
	  /* enable workarounds for still-image capture */
	  enable_fake_sdram();
	  start_gem();
	  start_rtic();
#endif

	  ipu_select_buffer(CSI_MEM, IPU_OUTPUT_BUFFER, 0);
	  ipu_enable_channel(CSI_MEM);
	}

	callback_flag++;
	FUNC_END;
	return IRQ_HANDLED;
}

/*!
 * CSI callback function.
 *
 * @param irq       int irq line
 * @param dev_id    void * device id
 * @param regs      struct pt_regs *
 *
 * @return status   IRQ_HANDLED for handled
 */
static irqreturn_t
prp_still_callback(int irq, void *dev_id, struct pt_regs *regs)
{
	cam_data *cam = (cam_data *) dev_id;
	FUNC_START;

#ifdef CONFIG_VIDEO_MXC_V4L1
	v4l1_camera_callback(irq);
#else
	cam->still_counter++;
	wake_up_interruptible(&cam->still_queue);
	//printk(" prp_idma7_eof_callback\n");
#endif

	FUNC_END;
	return IRQ_HANDLED;
}

/*!
 * start csi->mem task
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int prp_still_start(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err;
	FUNC_START;

	//ipu_uninit_channel(CSI_MEM);
	err = ipu_init_channel(CSI_MEM, NULL);
	if (err != 0)
		return err;
	ipu_csi_enable_mclk(CSI_MCLK_RAW, true, true);

#ifdef CONFIG_VIDEO_MXC_V4L1
	err = ipu_init_channel_buffer(CSI_MEM, IPU_OUTPUT_BUFFER, 
                IPU_PIX_FMT_GENERIC_32, cam->v2f.fmt.pix.width/2, 
                cam->v2f.fmt.pix.height, cam->v2f.fmt.pix.width/2, 
                IPU_ROTATE_NONE, cam->still_buf, NULL);
#else
	err = ipu_init_channel_buffer(CSI_MEM, IPU_OUTPUT_BUFFER,
				      IPU_PIX_FMT_YUYV, cam->v2f.fmt.pix.width,
				      cam->v2f.fmt.pix.height,
				      cam->v2f.fmt.pix.width, IPU_ROTATE_NONE,
				      cam->still_buf, NULL);
#endif
	if (err != 0)
		return err;

	err = ipu_request_irq(IPU_IRQ_SENSOR_OUT_EOF, prp_still_callback,
			      0, "Mxc Camera", cam);
	if (err != 0) {
		DPRINTK("Error registering irq.\n");
		return err;
	}
	callback_flag = 0;
	err = ipu_request_irq(IPU_IRQ_SENSOR_EOF, prp_csi_eof_callback,
			      0, "Mxc Camera", NULL);
	if (err != 0) {
		DPRINTK("Error IPU_IRQ_SENSOR_EOF \n");
		return err;
	}

	FUNC_END;
	return err;
}

/*!
 * stop csi->mem encoder task
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int prp_still_stop(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;
	FUNC_START;

	ipu_free_irq(IPU_IRQ_SENSOR_EOF, NULL);
	ipu_free_irq(IPU_IRQ_SENSOR_OUT_EOF, cam);

	ipu_uninit_channel(CSI_MEM);
	ipu_disable_channel(CSI_MEM, true);
	ipu_csi_enable_mclk(CSI_MCLK_RAW, false, false);

	FUNC_END;
	return err;
}

/*!
 * function to select CSI_MEM as the working path
 *
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
int prp_still_select(void *private)
{
	cam_data *cam = (cam_data *) private;

	FUNC_START;
	if (cam) {
		cam->csi_start = prp_still_start;
		cam->csi_stop = prp_still_stop;
	}

	FUNC_END;
	return 0;
}

/*!
 * function to de-select CSI_MEM as the working path
 *
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
int prp_still_deselect(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;
	FUNC_START;

	err = prp_still_stop(cam);

	if (cam) {
		cam->csi_start = NULL;
		cam->csi_stop = NULL;
	}

	FUNC_END;
	return err;
}

/*!
 * Init the Encorder channels
 *
 * @return  Error code indicating success or failure
 */
__init int prp_still_init(void)
{
	FUNC_START;
	FUNC_END;
	return 0;
}

/*!
 * Deinit the Encorder channels
 *
 */
void __exit prp_still_exit(void)
{
	FUNC_START;
	FUNC_END;
}

module_init(prp_still_init);
module_exit(prp_still_exit);

EXPORT_SYMBOL(prp_still_select);
EXPORT_SYMBOL(prp_still_deselect);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("IPU PRP STILL IMAGE Driver");
MODULE_LICENSE("GPL");
