/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ---------------------------
 * 04/13/2006   Motorola  Changed /dev/mxc_ipu_pf permission to 666
 * 11/06/2006   Motorola  Fixed white lines in some MPEG4 video 
 *                        clips with fast motion 
 *
 */

/*!
 * @file mxc_pf.c
 *
 * @brief MXC IPU MPEG4/H.264 Post-filtering driver
 *
 * User-level API for IPU Hardware MPEG4/H.264 Post-filtering.
 *
 * @ingroup MXC_PF
 */

#include <linux/config.h>
#include <linux/pagemap.h>
#include <linux/module.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include "../drivers/mxc/ipu/ipu.h"
#include "mxc_pf.h"

#undef H264_CACHEABLE_BUFFERS

/*#define MXC_PF_DEBUG */
#ifdef MXC_PF_DEBUG

#define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " fmt, \
        __FILE__,__LINE__,__FUNCTION__ , ## args)
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#define FUNC_START	DPRINTK(" func start\n")
#define FUNC_END	DPRINTK(" func end\n")

#define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", \
	__FILE__, __LINE__, __FUNCTION__, err)
#else				/* MXC_PF_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* MXC_PF_DEBUG */

struct mxc_pf_data {
	pf_operation_t mode;
	u32 pf_enabled;
	u32 width;
	u32 height;
	u32 stride;
	uint32_t qp_size;
	unsigned long qp_paddr;
	pf_buf buf[PF_MAX_BUFFER_CNT];

	wait_queue_head_t pf_wait;
	volatile int done_flag;
	volatile int busy_flag;
	struct semaphore busy_lock;
};

static struct mxc_pf_data pf_data;
static u8 open_count = 0;
static struct class_simple *mxc_pf_class;

/*
 * Function definitions
 */

static irqreturn_t mxc_pf_irq_handler(int irq, void *dev_id,
				      struct pt_regs *regs)
{
	struct mxc_pf_data *pf = dev_id;
	pf->done_flag++;

	if ((pf->done_flag == 3) || (pf->mode == PF_MPEG4_DERING)) {
		pf->done_flag = 4;
		wake_up_interruptible(&pf->pf_wait);
	}
	return IRQ_HANDLED;
}

/*!
 * This function handles PF_IOCTL_INIT calls. It initializes the PF channels,
 * interrupt handlers, and channel buffers.
 *
 * @return      This function returns 0 on success or negative error code on
 *              error.
 */
static int mxc_pf_init(pf_init_params * pf_init)
{
	int err;
	ipu_channel_params_t params;
	u32 w;
	u32 stride;
	u32 h;
	u32 qp_size = 0;
	u32 qp_stride;

	FUNC_START;

	pf_data.mode = pf_init->pf_mode;
	w = pf_data.width = pf_init->width;
	h = pf_data.height = pf_init->height;
	stride = pf_data.stride = pf_init->stride;
	pf_data.qp_size = pf_init->qp_size;

	memset(&params, 0, sizeof(params));
	params.mem_pf_mem.operation = pf_data.mode;
	err = ipu_init_channel(MEM_PF_Y_MEM, &params);
	if (err < 0) {
		printk("mxc_ipu_pf: error initializing channel\n");
		goto err0;
	}

	err = ipu_init_channel_buffer(MEM_PF_Y_MEM, IPU_INPUT_BUFFER,
				      IPU_PIX_FMT_GENERIC, w, h, stride,
				      IPU_ROTATE_NONE, NULL, NULL);
	if (err < 0) {
		printk("mxc_ipu_pf: error initializing Y input buffer\n");
		goto err0;
	}

	err = ipu_init_channel_buffer(MEM_PF_Y_MEM, IPU_OUTPUT_BUFFER,
				      IPU_PIX_FMT_GENERIC, w, h, stride,
				      IPU_ROTATE_NONE, NULL, NULL);
	if (err < 0) {
		printk("mxc_ipu_pf: error initializing Y output buffer\n");
		goto err0;
	}

	w = w / 2;
	h = h / 2;
	stride = stride / 2;

	err = ipu_init_channel_buffer(MEM_PF_U_MEM, IPU_INPUT_BUFFER,
				      IPU_PIX_FMT_GENERIC, w, h, stride,
				      IPU_ROTATE_NONE, NULL, NULL);
	if (err < 0) {
		printk("mxc_ipu_pf: error initializing U input buffer\n");
		goto err0;
	}

	err = ipu_init_channel_buffer(MEM_PF_U_MEM, IPU_OUTPUT_BUFFER,
				      IPU_PIX_FMT_GENERIC, w, h, stride,
				      IPU_ROTATE_NONE, NULL, NULL);
	if (err < 0) {
		printk("mxc_ipu_pf: error initializing U output buffer\n");
		goto err0;
	}

	err = ipu_init_channel_buffer(MEM_PF_V_MEM, IPU_INPUT_BUFFER,
				      IPU_PIX_FMT_GENERIC, w, h, stride,
				      IPU_ROTATE_NONE, NULL, NULL);
	if (err < 0) {
		printk("mxc_ipu_pf: error initializing V input buffer\n");
		goto err0;
	}

	err = ipu_init_channel_buffer(MEM_PF_V_MEM, IPU_OUTPUT_BUFFER,
				      IPU_PIX_FMT_GENERIC, w, h, stride,
				      IPU_ROTATE_NONE, NULL, NULL);
	if (err < 0) {
		printk("mxc_ipu_pf: error initializing V output buffer\n");
		goto err0;
	}
	/*Setup Channel QF and BSC Params*/
	if (pf_data.mode == PF_H264_DEBLOCK) {
		w = ((pf_data.width + 15) / 16);
		h = (pf_data.height + 15) / 16;
		qp_stride = w;
		qp_size = 4 * qp_stride * h;
		DPRINTK("H264 QP width = %d, height = %d\n", w, h);
		err = ipu_init_channel_buffer(MEM_PF_Y_MEM,
					      IPU_SEC_INPUT_BUFFER,
					      IPU_PIX_FMT_GENERIC_32, w, h,
					      qp_stride, IPU_ROTATE_NONE, NULL,
					      NULL);
		if (err < 0) {
			DPRINTK("error initializing H264 QP buffer\n");
			goto err0;
		}
/*                w = (pf_data.width + 3) / 4; */
		w *= 4;
		h *= 4;
		qp_stride = w;
		err = ipu_init_channel_buffer(MEM_PF_U_MEM,
					      IPU_SEC_INPUT_BUFFER,
					      IPU_PIX_FMT_GENERIC, w, h,
					      qp_stride, IPU_ROTATE_NONE, NULL,
					      NULL);
		if (err < 0) {
			DPRINTK("error initializing H264 BSB buffer\n");
			goto err0;
		}
		qp_size += qp_stride * h;
	} else			/* MPEG4 mode */
	{
		w = (pf_data.width + 15) / 16;
		h = (pf_data.height + 15) / 16;
		qp_stride = (w + 3) & ~0x3UL;
		DPRINTK("MPEG4 QP width = %d, height = %d, stride = %d\n",
			w, h, qp_stride);
		err = ipu_init_channel_buffer(MEM_PF_Y_MEM,
					      IPU_SEC_INPUT_BUFFER,
					      IPU_PIX_FMT_GENERIC, w, h,
					      qp_stride, IPU_ROTATE_NONE, NULL,
					      NULL);
		if (err < 0) {
			DPRINTK("error initializing MPEG4 QP buffer\n");
			goto err0;
		}
		qp_size = qp_stride * h;
	}

	if (pf_data.qp_size > qp_size)
		qp_size = pf_data.qp_size;
	else
		pf_data.qp_size = qp_size;

	pf_data.qp_paddr = ipu_malloc(qp_size);
	if (!pf_data.qp_paddr)
		return -ENOMEM;

	pf_init->qp_paddr = pf_data.qp_paddr;
	pf_init->qp_size = pf_data.qp_size;

	FUNC_END;
	return 0;

      err0:
	return err;
}

/*!
 * This function handles PF_IOCTL_UNINIT calls. It uninitializes the PF
 * channels and interrupt handlers.
 *
 * @return      This function returns 0 on success or negative error code
 *              on error.
 */
static int mxc_pf_uninit(void)
{
	pf_data.pf_enabled = 0;
	ipu_disable_irq(IPU_IRQ_PF_Y_OUT_EOF);
	ipu_disable_irq(IPU_IRQ_PF_U_OUT_EOF);
	ipu_disable_irq(IPU_IRQ_PF_V_OUT_EOF);

	ipu_disable_channel(MEM_PF_Y_MEM, true);
	ipu_disable_channel(MEM_PF_U_MEM, true);
	ipu_disable_channel(MEM_PF_V_MEM, true);
	ipu_uninit_channel(MEM_PF_Y_MEM);
	ipu_uninit_channel(MEM_PF_U_MEM);
	ipu_uninit_channel(MEM_PF_V_MEM);

	if (pf_data.qp_paddr) {
		ipu_free(pf_data.qp_paddr);
	}

	return 0;
}

/*!
 * This function handles PF_IOCTL_REQBUFS calls. It initializes the PF channels
 * and channel buffers.
 *
 * @param       reqbufs         Input/Output Structure containing buffer mode,
 *                              type, offset, and size. The offset and size of
 *                              the buffer are returned for PF_MEMORY_MMAP mode.
 *
 * @return      This function returns 0 on success or negative error code
 *              on error.
 */
static int mxc_pf_reqbufs(pf_reqbufs_params * reqbufs)
{
	int err;
	uint32_t buf_size;
	int i;
	int alloc_cnt = 0;
	pf_buf *buf = pf_data.buf;
	if (reqbufs->count > PF_MAX_BUFFER_CNT) {
		reqbufs->count = PF_MAX_BUFFER_CNT;
	}
	/* Deallocate mmapped buffers */
	if (reqbufs->count == 0) {
		for (i = 0; i < PF_MAX_BUFFER_CNT; i++) {
			if (buf[i].index != -1) {
				buf[i].index = -1;
				buf[i].size = 0;
				ipu_free(buf[i].offset);
			}
		}
		return 0;
	}

	buf_size = (pf_data.stride * pf_data.height * 3) / 2;
	if (reqbufs->req_size > buf_size) {
		DPRINTK("using requested buffer size of %d\n");
		buf_size = reqbufs->req_size;
	} else {
		DPRINTK("using default buffer size of %d\n");
		reqbufs->req_size = buf_size;
	}

	for (i = 0; alloc_cnt < reqbufs->count; i++) {
		buf[i].index = i;
		buf[i].size = buf_size;
		buf[i].offset = (unsigned long)ipu_malloc(buf_size);
		if (buf[i].offset == 0) {
			DPRINTK("unable to allocate IPU buffers.\n");
			err = -ENOMEM;
			goto err0;
		}
		DPRINTK("Allocated buffer %d at paddr 0x%08X\n",
			i, buf[i].offset);

		alloc_cnt++;
	}

	return 0;
      err0:
	for (i = 0; i < alloc_cnt; i++) {
		buf[i].index = -1;
		buf[i].size = 0;
		ipu_free(buf[i].offset);
	}
	return err;
}

/*!
 * This function handles PF_IOCTL_START calls. It sets the PF channel buffers
 * addresses and starts the channels
 *
 * @return      This function returns 0 on success or negative error code on
 *              error.
 */
static int mxc_pf_start(pf_buf * in, pf_buf * out)
{
	int err;
	u32 y_in_paddr;
	u32 u_in_paddr;
	u32 v_in_paddr;
	u32 p1_in_paddr;
	u32 p2_in_paddr;
	u32 y_out_paddr;
	u32 u_out_paddr;
	u32 v_out_paddr;

	FUNC_START;

	/* H.264 requires output buffer equal to input */
	if (pf_data.mode == PF_H264_DEBLOCK)
		out = in;

	y_in_paddr = in->offset + in->y_offset;
	if (in->u_offset)
		u_in_paddr = in->offset + in->u_offset;
	else
		u_in_paddr = y_in_paddr + (pf_data.stride * pf_data.height);
	if (in->v_offset)
		v_in_paddr = in->offset + in->v_offset;
	else
		v_in_paddr = u_in_paddr + (pf_data.stride * pf_data.height) / 4;
	p1_in_paddr = pf_data.qp_paddr;
	if (pf_data.mode == PF_H264_DEBLOCK) {
		p2_in_paddr = pf_data.qp_paddr +
		    ((pf_data.width + 15) / 16) *
		    ((pf_data.height + 15) / 16) * 4;
	} else {
		p2_in_paddr = 0;
	}

	DPRINTK("y_in_paddr = 0x%08X\nu_in_paddr = 0x%08X\n"
		"v_in_paddr = 0x%08X\n"
		"qp_paddr = 0x%08X\nbsb_paddr = 0x%08X\n",
		y_in_paddr, u_in_paddr, v_in_paddr, p1_in_paddr, p2_in_paddr);

	y_out_paddr = out->offset + out->y_offset;
	if (out->u_offset)
		u_out_paddr = out->offset + out->u_offset;
	else
		u_out_paddr = y_out_paddr + (pf_data.stride * pf_data.height);
	if (out->v_offset)
		v_out_paddr = out->offset + out->v_offset;
	else
		v_out_paddr =
		    u_out_paddr + (pf_data.stride * pf_data.height) / 4;

	DPRINTK("y_out_paddr = 0x%08X\nu_out_paddr = 0x%08X\n"
		"v_out_paddr = 0x%08X\n",
		y_out_paddr, u_out_paddr, v_out_paddr);

	pf_data.done_flag = 0;

	ipu_enable_irq(IPU_IRQ_PF_Y_OUT_EOF);
	ipu_enable_irq(IPU_IRQ_PF_U_OUT_EOF);
	ipu_enable_irq(IPU_IRQ_PF_V_OUT_EOF);

	err = ipu_update_channel_buffer(MEM_PF_Y_MEM, IPU_INPUT_BUFFER, 0,
					(void *)y_in_paddr);
	if (err < 0) {
		printk("mxc_ipu_pf: error setting Y input buffer\n");
		goto err0;
	}

	err = ipu_update_channel_buffer(MEM_PF_Y_MEM, IPU_OUTPUT_BUFFER, 0,
					(void *)y_out_paddr);
	if (err < 0) {
		printk("mxc_ipu_pf: error setting Y output buffer\n");
		goto err0;
	}

	err = ipu_update_channel_buffer(MEM_PF_U_MEM, IPU_INPUT_BUFFER, 0,
					(void *)u_in_paddr);
	if (err < 0) {
		printk("mxc_ipu_pf: error setting U input buffer\n");
		goto err0;
	}

	err = ipu_update_channel_buffer(MEM_PF_U_MEM, IPU_OUTPUT_BUFFER, 0,
					(void *)u_out_paddr);
	if (err < 0) {
		printk("mxc_ipu_pf: error setting U output buffer\n");
		goto err0;
	}

	err = ipu_update_channel_buffer(MEM_PF_V_MEM, IPU_INPUT_BUFFER, 0,
					(void *)v_in_paddr);
	if (err < 0) {
		printk("mxc_ipu_pf: error setting V input buffer\n");
		goto err0;
	}

	err = ipu_update_channel_buffer(MEM_PF_V_MEM, IPU_OUTPUT_BUFFER, 0,
					(void *)v_out_paddr);
	if (err < 0) {
		printk("mxc_ipu_pf: error setting V output buffer\n");
		goto err0;
	}

	err = ipu_update_channel_buffer(MEM_PF_Y_MEM, IPU_SEC_INPUT_BUFFER, 0,
					(void *)p1_in_paddr);
	if (err < 0) {
		printk("mxc_ipu_pf: error setting QP buffer\n");
		goto err0;
	}

	if (pf_data.mode == PF_H264_DEBLOCK) {

		err = ipu_update_channel_buffer(MEM_PF_U_MEM,
						IPU_SEC_INPUT_BUFFER, 0,
						(void *)p2_in_paddr);
		if (err < 0) {
			printk("mxc_ipu_pf: error setting H264 BSB buffer\n");
			goto err0;
		}
		ipu_select_buffer(MEM_PF_U_MEM, IPU_SEC_INPUT_BUFFER, 0);
	}

	ipu_select_buffer(MEM_PF_Y_MEM, IPU_OUTPUT_BUFFER, 0);
	ipu_select_buffer(MEM_PF_U_MEM, IPU_OUTPUT_BUFFER, 0);
	ipu_select_buffer(MEM_PF_V_MEM, IPU_OUTPUT_BUFFER, 0);
	ipu_select_buffer(MEM_PF_U_MEM, IPU_INPUT_BUFFER, 0);
	ipu_select_buffer(MEM_PF_V_MEM, IPU_INPUT_BUFFER, 0);
	ipu_select_buffer(MEM_PF_Y_MEM, IPU_SEC_INPUT_BUFFER, 0);
	ipu_select_buffer(MEM_PF_Y_MEM, IPU_INPUT_BUFFER, 0);

	if (!pf_data.pf_enabled) {
		pf_data.pf_enabled = 1;
		ipu_enable_channel(MEM_PF_V_MEM);
		ipu_enable_channel(MEM_PF_U_MEM);
		ipu_enable_channel(MEM_PF_Y_MEM);
	}

	FUNC_END;
	return 0;
      err0:
	return err;
}

/*!
 * Post Filter driver open function. This function implements the Linux
 * file_operations.open() API function.
 *
 * @param       inode           struct inode *
 *
 * @param       filp            struct file *
 *
 * @return      This function returns 0 on success or negative error code on
 *              error.
 */
static int mxc_pf_open(struct inode *inode, struct file *filp)
{
	int i;
	FUNC_START;

	if (open_count++ == 0) {
		memset(&pf_data, 0, sizeof(pf_data));
		for (i = 0; i < PF_MAX_BUFFER_CNT; i++) {
			pf_data.buf[i].index = -1;
		}
		init_waitqueue_head(&pf_data.pf_wait);
		init_MUTEX(&pf_data.busy_lock);

		pf_data.busy_flag = 1;

		ipu_request_irq(IPU_IRQ_PF_Y_OUT_EOF, mxc_pf_irq_handler,
				0, "mxc_ipu_pf", &pf_data);

		ipu_request_irq(IPU_IRQ_PF_U_OUT_EOF, mxc_pf_irq_handler,
				0, "mxc_ipu_pf", &pf_data);

		ipu_request_irq(IPU_IRQ_PF_V_OUT_EOF, mxc_pf_irq_handler,
				0, "mxc_ipu_pf", &pf_data);

		ipu_disable_irq(IPU_IRQ_PF_Y_OUT_EOF);
		ipu_disable_irq(IPU_IRQ_PF_U_OUT_EOF);
		ipu_disable_irq(IPU_IRQ_PF_V_OUT_EOF);
	}
	FUNC_END;
	return 0;
}

/*!
 * Post Filter driver release function. This function implements the Linux
 * file_operations.release() API function.
 *
 * @param       inode           struct inode *
 *
 * @param       filp            struct file *
 *
 * @return      This function returns 0 on success or negative error code on
 *              error.
 */
static int mxc_pf_release(struct inode *inode, struct file *filp)
{
	pf_reqbufs_params req_buf;
	FUNC_START;
	if (--open_count == 0) {
		mxc_pf_uninit();

		/* Free any allocated buffers */
		req_buf.count = 0;
		mxc_pf_reqbufs(&req_buf);

		ipu_free_irq(IPU_IRQ_PF_V_OUT_EOF, &pf_data);
		ipu_free_irq(IPU_IRQ_PF_U_OUT_EOF, &pf_data);
		ipu_free_irq(IPU_IRQ_PF_Y_OUT_EOF, &pf_data);

	}
	FUNC_END;
	return 0;
}

extern void v6_flush_kern_cache_all_l2(void);

/*!
 * Post Filter driver ioctl function. This function implements the Linux
 * file_operations.ioctl() API function.
 *
 * @param       inode           struct inode *
 *
 * @param       filp            struct file *
 *
 * @param       cmd             IOCTL command to handle
 *
 * @param       arg             Pointer to arguments for IOCTL
 *
 * @return      This function returns 0 on success or negative error code on
 *              error.
 */
static int mxc_pf_ioctl(struct inode *inode, struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	switch (cmd) {
	case PF_IOCTL_INIT:
		{
			pf_init_params pf_init;

			DPRINTK("PF_IOCTL_INIT\n");
			if (copy_from_user(&pf_init, (void *)arg,
					   _IOC_SIZE(cmd))) {
				retval = -EFAULT;
				break;
			}

			retval = mxc_pf_init(&pf_init);
			if (retval < 0)
				break;
			pf_init.qp_paddr = pf_data.qp_paddr;
			pf_init.qp_size = pf_data.qp_size;

			/* Return size of memory allocated */
			if (copy_to_user((void *)arg, &pf_init, _IOC_SIZE(cmd))) {
				retval = -EFAULT;
				break;
			}

			pf_data.busy_flag = 0;
			break;
		}
	case PF_IOCTL_UNINIT:
		DPRINTK("PF_IOCTL_UNINIT\n");
		retval = mxc_pf_uninit();
		break;
	case PF_IOCTL_REQBUFS:
		{
			pf_reqbufs_params reqbufs;
			DPRINTK("PF_IOCTL_REQBUFS\n");

			if (copy_from_user
			    (&reqbufs, (void *)arg, _IOC_SIZE(cmd))) {
				retval = -EFAULT;
				break;
			}

			retval = mxc_pf_reqbufs(&reqbufs);

			/* Return size of memory allocated */
			if (copy_to_user((void *)arg, &reqbufs, _IOC_SIZE(cmd))) {
				retval = -EFAULT;
				break;
			}

			break;
		}
	case PF_IOCTL_QUERYBUF:
		{
			pf_buf buf;
			DPRINTK("PF_IOCTL_QUERYBUF\n");

			if (copy_from_user(&buf, (void *)arg, _IOC_SIZE(cmd))) {
				retval = -EFAULT;
				break;
			}

			if ((buf.index < 0) ||
			    (buf.index >= PF_MAX_BUFFER_CNT) ||
			    (pf_data.buf[buf.index].index != buf.index)) {
				retval = -EINVAL;
				break;
			}
			/* Return size of memory allocated */
			if (copy_to_user((void *)arg, &pf_data.buf[buf.index],
					 _IOC_SIZE(cmd))) {
				retval = -EFAULT;
				break;
			}

			break;
		}
	case PF_IOCTL_START:
		{
			int index;
			pf_start_params start_params;
			DPRINTK("PF_IOCTL_START\n");

			if (pf_data.busy_flag) {
				retval = -EBUSY;
				break;
			}

			if (copy_from_user(&start_params, (void *)arg,
					   _IOC_SIZE(cmd))) {
				retval = -EFAULT;
				break;
			}
			if (start_params.h264_pause_row >=
			    ((pf_data.height + 15) / 16)) {
				retval = -EINVAL;
				break;
			}

			pf_data.busy_flag = 1;

			index = start_params.in.index;
			if ((index >= 0) && (index < PF_MAX_BUFFER_CNT)) {
				if (pf_data.buf[index].offset !=
				    start_params.in.offset) {
					retval = -EINVAL;
					break;
				}
			}

			index = start_params.out.index;
			if ((index >= 0) && (index < PF_MAX_BUFFER_CNT)) {
				if (pf_data.buf[index].offset !=
				    start_params.out.offset) {
					retval = -EINVAL;
					break;
				}
			}

			ipu_pf_set_pause_row(start_params.h264_pause_row);

			/*Update y, u, v buffers in DMA Channels */
			if ((retval =
			     mxc_pf_start(&start_params.in, &start_params.out))
			    < 0) {
				break;
			}

			DPRINTK("PF_IOCTL_START - processing started\n");

			if (!start_params.wait) {
				break;
			}

			DPRINTK("PF_IOCTL_START - waiting for completion\n");

			/* Fall thru to wait */
		}
	case PF_IOCTL_WAIT:
		{
			if (!wait_event_interruptible_timeout(pf_data.pf_wait,
							      pf_data.
							      done_flag == 4,
							      1 * HZ)) {
				DPRINTK
				    ("PF_IOCTL_WAIT: timeout, done_flag = %d\n",
				     pf_data.done_flag);
				retval = -ETIME;
				break;
			} else if (signal_pending(current)) {
				DPRINTK("PF_IOCTL_WAIT: interrupt received\n");
				retval = -ERESTARTSYS;
				break;
			}
#ifdef H264_CACHEABLE_BUFFERS
			v6_flush_kern_cache_all_l2();
#endif
			pf_data.busy_flag = 0;

			DPRINTK("PF_IOCTL_WAIT - processing finished\n");
			break;
		}
	case PF_IOCTL_RESUME:
		{
			int pause_row;
			DPRINTK("PF_IOCTL_RESUME\n");

			if (pf_data.busy_flag == 0) {
				retval = -EFAULT;
				break;
			}

			if (copy_from_user(&pause_row, (void *)arg,
					   _IOC_SIZE(cmd))) {
				retval = -EFAULT;
				break;
			}

			if (pause_row >= ((pf_data.height + 15) / 16)) {
				retval = -EINVAL;
				break;
			}

			ipu_pf_set_pause_row(pause_row);
			break;
		}

	default:
		printk(" ipu_pf_ioctl not supported ioctls\n");
		retval = -1;
	}

	if (retval < 0)
		DPRINTK("return = %d\n", retval);
	return retval;
}

/*!
 * Post Filter driver mmap function. This function implements the Linux
 * file_operations.mmap() API function for mapping driver buffers to user space.
 *
 * @param       file            struct file *
 *
 * @param       vma             structure vm_area_struct *
 *
 * @return      0 Success, EINTR busy lock error,
 *                      ENOBUFS remap_page error.
 */
static int mxc_pf_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	pgprot_t prot;
	int res = 0;

	FUNC_START;

	/* make this _really_ smp-safe */
	if (down_interruptible(&pf_data.busy_lock))
		return -EINTR;

#ifndef H264_CACHEABLE_BUFFERS
	if (pf_data.mode == PF_H264_DEBLOCK) {
#if defined(CONFIG_MOT_FEAT_NO_PAGE_SHARED)
		prot = pgprot_writecombine(vma->vm_page_prot);
#else
		prot = pgprot_writecombine(PAGE_SHARED);
#endif /* defined(CONFIG_MOT_FEAT_NO_PAGE_SHARED) */
	} else
#endif
	{
#if defined(CONFIG_MOT_FEAT_NO_PAGE_SHARED)
		prot = pgprot_writecombine(vma->vm_page_prot);
#else
		prot = __pgprot(pgprot_val(PAGE_SHARED) & ~L_PTE_BUFFERABLE);
#endif /* defined(CONFIG_MOT_FEAT_NO_PAGE_SHARED) */
	}
	if (remap_pfn_range(vma, start, vma->vm_pgoff, size, prot)) {
		DPRINTK("remap_pfn_range failed\n");
		res = -ENOBUFS;
		goto mmap_exit;
	}

	DPRINTK("phy addr 0x%08X mmapped to virt addr 0x%08X, size = 0x%08X\n",
		vma->vm_pgoff << PAGE_SHIFT, start, size);

      mmap_exit:
	up(&pf_data.busy_lock);
	FUNC_END;
	return res;
}

/*!
 * Post Filter driver poll function. This function implements the Linux
 * file_operations.poll() API function.
 *
 * @param file       structure file *
 *
 * @param wait       structure poll_table *
 *
 * @return  status   POLLIN | POLLRDNORM
 */
static unsigned int mxc_pf_poll(struct file *file, poll_table * wait)
{
	wait_queue_head_t *queue = NULL;
	int res = POLLIN | POLLRDNORM;

	FUNC_START;

	if (down_interruptible(&pf_data.busy_lock))
		return -EINTR;

	queue = &pf_data.pf_wait;
	poll_wait(file, queue, wait);

	up(&pf_data.busy_lock);

	FUNC_END;
	return res;
}

/*!
 * File operation structure functions pointers.
 */
static struct file_operations mxc_pf_fops = {
	.owner = THIS_MODULE,
	.open = mxc_pf_open,
	.release = mxc_pf_release,
	.ioctl = mxc_pf_ioctl,
	.poll = mxc_pf_poll,
	.mmap = mxc_pf_mmap,
};

static int mxc_pf_major = 0;

/*!
 * Post Filter driver module initialization function.
 */
int mxc_pf_dev_init(void)
{
	struct class_device *temp_class;

	FUNC_START;

	mxc_pf_major = register_chrdev(0, "mxc_ipu_pf", &mxc_pf_fops);

	if (mxc_pf_major < 0) {
		printk(KERN_INFO "Unable to get a major for mxc_ipu_pf");
		return mxc_pf_major;
	}

	mxc_pf_class = class_simple_create(THIS_MODULE, "mxc_ipu_pf");
	if (IS_ERR(mxc_pf_class)) {
		printk(KERN_ERR "Error creating mxc_ipu_pf class.\n");
		devfs_remove("mxc_ipu_pf");
		class_simple_device_remove(MKDEV(mxc_pf_major, 0));
		unregister_chrdev(mxc_pf_major, "mxc_ipu_pf");
		return PTR_ERR(mxc_pf_class);
	}

	temp_class =
	    class_simple_device_add(mxc_pf_class, MKDEV(mxc_pf_major, 0), NULL,
				    "mxc_ipu_pf");
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "Error creating mxc_ipu_pf class device.\n");
		devfs_remove("mxc_ipu_pf");
		class_simple_device_remove(MKDEV(mxc_pf_major, 0));
		class_simple_destroy(mxc_pf_class);
		unregister_chrdev(mxc_pf_major, "mxc_ipu_pf");
		return -1;
	}

#if defined(CONFIG_MOT_FEAT_IPU_PF_PERM666)
        devfs_mk_cdev(MKDEV(mxc_pf_major, 0), S_IFCHR | S_IRUGO | S_IWUGO, 
                      "mxc_ipu_pf");
#else
	devfs_mk_cdev(MKDEV(mxc_pf_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "mxc_ipu_pf");
#endif /* defined(CONFIG_MOT_FEAT_IPU_PF_PERM666) */

	printk(KERN_INFO "IPU Post-filter loading\n");

	FUNC_END;

	return 0;
}

/*!
 * Post Filter driver module exit function.
 */
static void mxc_pf_exit(void)
{
	FUNC_START;

	if (mxc_pf_major > 0) {
		devfs_remove("mxc_ipu_pf");
		class_simple_device_remove(MKDEV(mxc_pf_major, 0));
		class_simple_destroy(mxc_pf_class);
		unregister_chrdev(mxc_pf_major, "mxc_ipu_pf");
	}

	FUNC_END;

	printk(KERN_INFO "IPU Post-filter: successfully unloaded\n");
}

module_init(mxc_pf_dev_init);
module_exit(mxc_pf_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC MPEG4/H.264 Postfilter Driver");
MODULE_LICENSE("GPL");
