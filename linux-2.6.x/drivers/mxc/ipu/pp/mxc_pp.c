/*
 * Copyright 2006 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*!
 * @file mxc_pp.c
 * 
 * @brief MXC IPU Post-processing driver 
 * 
 * User-level API for IPU Hardware Post-processing.
 *
 * @ingroup MXC_PP

               Modification     Tracking
Author                 Date         Description of Changes
----------------   ------------    -------------------------
Motorola            08/17/2006      File Created
 */

#include <linux/pagemap.h>
#include <linux/module.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include "../drivers/mxc/ipu/ipu.h"
#include "mxc_pp.h"


#undef MXC_PP_DEBUG
#ifdef MXC_PP_DEBUG
#include "../drivers/mxc/ipu/ipu_regs.h"
#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START	DPRINTK(" func start\n")
#  define FUNC_END	DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", __FILE__,__LINE__,__FUNCTION__ ,err)

#else /* MXC_PP_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif /* MXC_PP_DEBUG */

struct mxc_pp_data {
  pp_operation_t mode;
  u32 pp_enabled;
  uint16_t in_width;
  uint16_t in_height;
  uint32_t in_stride;
  uint32_t in_pixel_fmt;
  uint16_t in_comb_width;
  uint16_t in_comb_height;
  uint32_t in_comb_stride;
  uint32_t in_comb_pixel_fmt;
  uint32_t mid_stride;
  uint16_t out_width;
  uint16_t out_height;
  uint32_t out_stride;
  uint32_t out_pixel_fmt;
  ipu_rotate_mode_t rot;
  uint16_t ic_combine_en;
  pp_buf buf[PP_MAX_BUFFER_CNT];
  wait_queue_head_t pp_wait;
  volatile int done_flag;
  volatile int busy_flag;
  struct semaphore busy_lock;
};

static struct mxc_pp_data pp_data;
static u8 open_count = 0;


/*
 * Function definitions
 */

static irqreturn_t mxc_pp_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
  struct mxc_pp_data * pp = dev_id;

  if((pp->mode == PP_PP_ROT) && (pp->done_flag == 0)) {
    ipu_select_buffer(MEM_ROT_PP_MEM, IPU_INPUT_BUFFER, 0);
    pp->done_flag++ ;
  }
  else {
    pp->done_flag = 1;
    wake_up_interruptible(&pp->pp_wait);
  }
  return IRQ_HANDLED;
}

/*!
 * This function handles PP_IOCTL_INIT calls. It initializes the PP channels,
 * interrupt handlers, and channel buffers.
 * 
 * @return      This function returns 0 on success or negative error code on error
 */
static int mxc_pp_init(pp_init_params * pp_init)
{
  ipu_channel_params_t params;
  int err = 0;

  FUNC_START;

  memset(&params, 0, sizeof(params));
  pp_data.mode = pp_init->mode;
  pp_data.in_stride = pp_init->in_stride;
  pp_data.out_stride = pp_init->out_stride;
  if(pp_data.mode <= PP_PP_ROT) {
    pp_data.rot = (ipu_rotate_mode_t)pp_init->rot;
    if(pp_data.mode == PP_PP_ROT)
      pp_data.mid_stride = pp_init->mid_stride;
  }
  pp_data.in_width = pp_init->in_width;
  pp_data.in_height = pp_init->in_height;
  pp_data.in_pixel_fmt = pp_init->in_pixel_fmt;
  pp_data.out_width = pp_init->out_width;
  pp_data.out_height = pp_init->out_height;
  pp_data.out_pixel_fmt = pp_init->out_pixel_fmt;
  if(pp_data.mode >= PP_PP_ROT) {
    pp_data.ic_combine_en =  pp_init->ic_combine_en;
    params.mem_pp_mem.graphics_combine_en = (bool)pp_data.ic_combine_en;
  }
  if(pp_data.ic_combine_en != 0) {
    pp_data.in_comb_width = pp_init->in_comb_width;
    pp_data.in_comb_height = pp_init->in_comb_height;
    pp_data.in_comb_stride = pp_init->in_comb_stride;
    pp_data.in_comb_pixel_fmt = pp_init->in_comb_pixel_fmt;
  }

  /* ipu_unlink_channels(MEM_PP_MEM, MEM_ROT_PP_MEM); */

  if(pp_data.mode == PP_PP) {
    params.mem_pp_mem.in_width = pp_data.in_width;
    params.mem_pp_mem.in_height = pp_data.in_height;
    params.mem_pp_mem.in_pixel_fmt = pp_data.in_pixel_fmt;
    params.mem_pp_mem.out_width = pp_data.out_width;
    params.mem_pp_mem.out_height = pp_data.out_height;
    params.mem_pp_mem.out_pixel_fmt = pp_data.out_pixel_fmt;
    err = ipu_init_channel(MEM_PP_MEM, &params);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing MEM_PP_MEM channel\n");
      goto err0;
    }
    err = ipu_init_channel_buffer(MEM_PP_MEM, IPU_INPUT_BUFFER, pp_data.in_pixel_fmt,
                pp_data.in_width, pp_data.in_height, pp_data.in_stride, IPU_ROTATE_NONE, NULL, NULL);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing PP input buffer\n");
      goto err0;
    }

    err = ipu_init_channel_buffer(MEM_PP_MEM, IPU_OUTPUT_BUFFER, pp_data.out_pixel_fmt,
                pp_data.out_width, pp_data.out_height, pp_data.out_stride, IPU_ROTATE_NONE, NULL, NULL);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing PP output buffer\n");
      goto err0;
    }

    if(pp_data.ic_combine_en != 0) {
      err = ipu_init_channel_buffer(MEM_PP_MEM, IPU_SEC_INPUT_BUFFER, pp_data.in_comb_pixel_fmt,
                  pp_data.in_comb_width, pp_data.in_comb_height, pp_data.in_comb_stride,
                  IPU_ROTATE_NONE, NULL, NULL);
      if(err < 0) {
        printk("mxc_ipu_pp: error initializing PP second input buffer\n");
        goto err0;
      }
    }
  }
  else if(pp_data.mode == PP_ROT) {
    err = ipu_init_channel(MEM_ROT_PP_MEM, &params);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing MEM_ROT_PP_MEM channel\n");
      goto err0;
    }
    err = ipu_init_channel_buffer(MEM_ROT_PP_MEM, IPU_INPUT_BUFFER, pp_data.in_pixel_fmt,
                pp_data.in_width, pp_data.in_height, pp_data.in_stride, pp_data.rot, NULL, NULL);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing ROT input buffer\n");
      goto err0;
    }

    err = ipu_init_channel_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, pp_data.out_pixel_fmt,
                pp_data.out_width, pp_data.out_height, pp_data.out_stride, IPU_ROTATE_NONE, NULL, NULL);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing ROT output buffer\n");
      goto err0;
    }
  }
  else if(pp_data.mode == PP_PP_ROT) {
    err = ipu_init_channel(MEM_ROT_PP_MEM, &params);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing MEM_ROT_PP_MEM channel\n");
      goto err0;
    }
    params.mem_pp_mem.in_width = pp_data.in_width;
    params.mem_pp_mem.in_height = pp_data.in_height;
    params.mem_pp_mem.in_pixel_fmt = pp_data.in_pixel_fmt;
    if(pp_data.rot >= IPU_ROTATE_90_RIGHT) {
      params.mem_pp_mem.out_width = pp_data.out_height;
      params.mem_pp_mem.out_height = pp_data.out_width;
    }
    else {
      params.mem_pp_mem.out_width = pp_data.out_width;
      params.mem_pp_mem.out_height = pp_data.out_height;
    }
    params.mem_pp_mem.out_pixel_fmt = pp_data.out_pixel_fmt;
    err = ipu_init_channel(MEM_PP_MEM, &params);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing MEM_PP_MEM channel\n");
      goto err0;
    }

    /* err = ipu_link_channels(MEM_PP_MEM, MEM_ROT_PP_MEM); */

    if(err < 0) {
      printk("mxc_ipu_pp: error linking MEM_PP_MEM to MEM_ROT_PP_MEM\n");
      goto err0;
    }
    err = ipu_init_channel_buffer(MEM_PP_MEM, IPU_INPUT_BUFFER, pp_data.in_pixel_fmt,
                pp_data.in_width, pp_data.in_height, pp_data.in_stride, IPU_ROTATE_NONE, NULL, NULL);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing PP_PP_ROT input buffer\n");
      goto err0;
    }
    if(pp_data.rot >= IPU_ROTATE_90_RIGHT) {
      err = ipu_init_channel_buffer(MEM_PP_MEM, IPU_OUTPUT_BUFFER, pp_data.out_pixel_fmt,
                  pp_data.out_height, pp_data.out_width, pp_data.mid_stride, IPU_ROTATE_NONE, NULL, NULL);
    }
    else {
      err = ipu_init_channel_buffer(MEM_PP_MEM, IPU_OUTPUT_BUFFER, pp_data.out_pixel_fmt,
                  pp_data.out_width, pp_data.out_height, pp_data.out_stride, IPU_ROTATE_NONE, NULL, NULL);
    }
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing PP_PP_ROT middle output buffer\n");
      goto err0;
    }
    if(pp_data.ic_combine_en != 0) {
      err = ipu_init_channel_buffer(MEM_PP_MEM, IPU_SEC_INPUT_BUFFER, pp_data.in_comb_pixel_fmt,
                  pp_data.in_comb_width, pp_data.in_comb_height, pp_data.in_comb_stride,
                  IPU_ROTATE_NONE, NULL, NULL);
      if(err < 0) {
        printk("mxc_ipu_pp: error initializing PP second input buffer\n");
        goto err0;
      }
    }
    if(pp_data.rot >= IPU_ROTATE_90_RIGHT) {
      err = ipu_init_channel_buffer(MEM_ROT_PP_MEM, IPU_INPUT_BUFFER, pp_data.out_pixel_fmt,
                  pp_data.out_height, pp_data.out_width, pp_data.mid_stride, pp_data.rot, NULL, NULL);
    }
    else {
      err = ipu_init_channel_buffer(MEM_ROT_PP_MEM, IPU_INPUT_BUFFER, pp_data.out_pixel_fmt,
                  pp_data.out_width, pp_data.out_height, pp_data.out_stride, pp_data.rot, NULL, NULL);
    }
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing ROT input buffer\n");
      goto err0;
    }
    err = ipu_init_channel_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, pp_data.out_pixel_fmt,
                pp_data.out_width, pp_data.out_height, pp_data.out_stride, IPU_ROTATE_NONE, NULL, NULL);
    if(err < 0) {
      printk("mxc_ipu_pp: error initializing ROT output buffer\n");
      goto err0;
    }
  }

  FUNC_END;
  return 0;

err0:
  return err;
}

/*!
 * This function handles PP_IOCTL_UNINIT calls. It uninitializes the PP channels
 * and interrupt handlers.
 * 
 * @return      This function returns 0 on success or negative error code on error
 */
static int mxc_pp_uninit(void)
{
  pp_data.pp_enabled = 0;

  ipu_disable_irq(IPU_IRQ_PP_ROT_OUT_EOF);
  ipu_disable_irq(IPU_IRQ_PP_OUT_EOF);

  ipu_disable_channel(MEM_PP_MEM, true);
  ipu_disable_channel(MEM_ROT_PP_MEM, true);
  ipu_uninit_channel(MEM_PP_MEM);
  ipu_uninit_channel(MEM_ROT_PP_MEM);
  /* ipu_unlink_channels(MEM_PP_MEM, MEM_ROT_PP_MEM); */

  return 0; 
}

/*!
 * This function handles PP_IOCTL_REQBUFS calls. It initializes the PP channels
 * and channel buffers.
 *
 * @param       reqbufs         Input/Output Structure containing buffer mode,
 *                              type, offset, and size. The offset and size of
 *                              the buffer are returned for PP_MEMORY_MMAP mode. 
 * 
 * @return      This function returns 0 on success or negative error code on error
 */
static int mxc_pp_reqbufs(pp_reqbufs_params * reqbufs)
{
  int i;
  int alloc_cnt = 0;
  int err = 0;
  pp_buf * buf = pp_data.buf;

  if(reqbufs->count > PP_MAX_BUFFER_CNT) {
    reqbufs->count = PP_MAX_BUFFER_CNT;
  }

  // Deallocate mmapped buffers
  if(reqbufs->count == 0) {
    for(i = 0; i < PP_MAX_BUFFER_CNT; i++) {
      if(buf[i].index != -1) {
        buf[i].index = -1;
        buf[i].size = 0;
        ipu_free(buf[i].addr);
      }
    }
    return 0;
  }

  for(i = 0; alloc_cnt < reqbufs->count; i++) {
    if(i >= PP_MAX_BUFFER_CNT)
      break;
    if(buf[i].index != -1) {
      continue;
    }
    buf[i].index = i;
    buf[i].size = reqbufs->req_size;
    buf[i].addr = (unsigned long)ipu_malloc(buf[i].size);
    if(buf[i].addr == 0) {
      DPRINTK("unable to allocate IPU buffers.\n");
      err = -ENOMEM;
      goto err0;
    }
    DPRINTK("Allocated buffer %d at paddr 0x%08X\n", i, buf[i].addr);
    alloc_cnt++;
  }
  if(alloc_cnt < reqbufs->count) {
    DPRINTK("Unable to allocate buffers.\n");
    err = -ENOMEM;
    goto err0;
  }

  return 0;
err0:
  return err;
}

/*!
 * This function handles PP_IOCTL_START calls. It sets the PP channel buffers
 * addresses and starts the channels
 *
 * @return      This function returns 0 on success or negative error code on 
 *              error.
 */
static int mxc_pp_start(pp_buf * in, pp_buf * in_comb, pp_buf * mid, pp_buf * out)
{
  int err = 0;

  FUNC_START;

  pp_data.done_flag = 0;

  if(pp_data.mode >= PP_PP_ROT)
    ipu_enable_irq(IPU_IRQ_PP_OUT_EOF);
  if(pp_data.mode <= PP_PP_ROT)
    ipu_enable_irq(IPU_IRQ_PP_ROT_OUT_EOF);

  if(pp_data.mode == PP_PP) {
    err = ipu_update_channel_buffer(MEM_PP_MEM, IPU_INPUT_BUFFER, 0, (void*)in->addr);
    if(err < 0) {
      printk("mxc_ipu_pp: error PP_PP input buffer\n");
      goto err0;
    }
    err = ipu_update_channel_buffer(MEM_PP_MEM, IPU_OUTPUT_BUFFER, 0, (void*)out->addr);
    if(err < 0) {
      printk("mxc_ipu_pp: error PP_PP output buffer\n");
      goto err0;
    }
    if(pp_data.ic_combine_en != 0) {
      err = ipu_update_channel_buffer(MEM_PP_MEM, IPU_SEC_INPUT_BUFFER, 0, (void*)in_comb->addr);
      if(err < 0) {
        printk("mxc_ipu_pp: error PP_PP second input buffer\n");
        goto err0;
      }
      ipu_select_buffer(MEM_PP_MEM, IPU_SEC_INPUT_BUFFER, 0);
    }
    ipu_select_buffer(MEM_PP_MEM, IPU_INPUT_BUFFER, 0);
    ipu_select_buffer(MEM_PP_MEM, IPU_OUTPUT_BUFFER, 0);
  }
  else if(pp_data.mode == PP_PP_ROT) {
    err = ipu_update_channel_buffer(MEM_PP_MEM, IPU_INPUT_BUFFER, 0, (void*)in->addr);
    if(err < 0) {
      printk("mxc_ipu_pp: error PP_PP_ROT input buffer\n");
      goto err0;
    }
    err = ipu_update_channel_buffer(MEM_PP_MEM, IPU_OUTPUT_BUFFER, 0, (void*)mid->addr);
    if(err < 0) {
      printk("mxc_ipu_pp: error setting PP_PP_ROT middle output buffer\n");
      goto err0;
    }
    err = ipu_update_channel_buffer(MEM_ROT_PP_MEM, IPU_INPUT_BUFFER, 0, (void*)mid->addr);
    if(err < 0) {
      printk("mxc_ipu_pp: error setting PP_PP_ROT middle input buffer\n");
      goto err0;
    }
    err = ipu_update_channel_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, 0, (void*)out->addr);
    if(err < 0) {
      printk("mxc_ipu_pp: error setting PP_PP_ROT output buffer\n");
      goto err0;
    }
    if(pp_data.ic_combine_en != 0) {
      err = ipu_update_channel_buffer(MEM_PP_MEM, IPU_SEC_INPUT_BUFFER, 0, (void*)in_comb->addr);
      if(err < 0) {
        printk("mxc_ipu_pp: error PP_PP_ROT second input buffer\n");
        goto err0;
      }
      ipu_select_buffer(MEM_PP_MEM, IPU_SEC_INPUT_BUFFER, 0);
    }
    ipu_select_buffer(MEM_PP_MEM, IPU_INPUT_BUFFER, 0);
    ipu_select_buffer(MEM_PP_MEM, IPU_OUTPUT_BUFFER, 0);
    ipu_select_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, 0);
  }
  else if(pp_data.mode == PP_ROT) {
    err = ipu_update_channel_buffer(MEM_ROT_PP_MEM, IPU_INPUT_BUFFER, 0, (void*)in->addr);
    if(err < 0) {
      printk("mxc_ipu_pp: error setting PP_ROT input buffer\n");
      goto err0;
    }
    err = ipu_update_channel_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, 0, (void*)out->addr);
    if(err < 0) {
      printk("mxc_ipu_pp: error setting PP_ROT output buffer\n");
      goto err0;
    }
    ipu_select_buffer(MEM_ROT_PP_MEM, IPU_INPUT_BUFFER, 0);
    ipu_select_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, 0);
  }

  if(!pp_data.pp_enabled) {
    pp_data.pp_enabled = 1;
    if(pp_data.mode == PP_PP)
      ipu_enable_channel(MEM_PP_MEM);
    else if(pp_data.mode == PP_PP_ROT) {
      ipu_enable_channel(MEM_PP_MEM);
      ipu_enable_channel(MEM_ROT_PP_MEM);
    }
    else if(pp_data.mode == PP_ROT) {
      ipu_enable_channel(MEM_ROT_PP_MEM);
    }
  }

  FUNC_END;
  return 0;
err0:
  return err;
}

/*!
 * Post-processing driver open function. This function implements the Linux
 * file_operations.open() API function. 
 *
 * @param       inode           struct inode *
 *
 * @param       filp            struct file * 
 *
 * @return      This function returns 0 on success or negative error code on 
 *              error.
 */
static int mxc_pp_open(struct inode *inode, struct file *filp)
{
  int i;

  FUNC_START;

  if(open_count++ == 0) {
    memset(&pp_data, 0, sizeof(pp_data));
    for(i = 0; i < PP_MAX_BUFFER_CNT; i++) {
      pp_data.buf[i].index = -1;
    }
    init_waitqueue_head(&pp_data.pp_wait);
    init_MUTEX(&pp_data.busy_lock);

    pp_data.busy_flag = 1;

    ipu_request_irq(IPU_IRQ_PP_ROT_OUT_EOF, mxc_pp_irq_handler, 0, "mxc_ipu_pp", &pp_data);
    ipu_request_irq(IPU_IRQ_PP_OUT_EOF, mxc_pp_irq_handler, 0, "mxc_ipu_pp", &pp_data);

    ipu_disable_irq(IPU_IRQ_PP_ROT_OUT_EOF);
    ipu_disable_irq(IPU_IRQ_PP_OUT_EOF);
  }

  FUNC_END;
  return 0;
}

/*!
 * Post-processing driver release function. This function implements the Linux
 * file_operations.release() API function. 
 *
 * @param       inode           struct inode *
 *
 * @param       filp            struct file * 
 *
 * @return      This function returns 0 on success or negative error code on 
 *              error.
 */
static int mxc_pp_release(struct inode *inode, struct file *filp)
{
  pp_reqbufs_params req_buf;

  FUNC_START;

  if(--open_count == 0) {
    mxc_pp_uninit();

    /* Free any allocated buffers */
    req_buf.count = 0;
    mxc_pp_reqbufs(&req_buf);

    ipu_free_irq(IPU_IRQ_PP_ROT_OUT_EOF, &pp_data);
    ipu_free_irq(IPU_IRQ_PP_OUT_EOF, &pp_data);
  }

  FUNC_END;
  return 0;       
}

/*!
 * Post-processing driver ioctl function. This function implements the Linux
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
static int mxc_pp_ioctl(struct inode *inode, struct file *filp, 
             unsigned int cmd, unsigned long arg)
{
  int retval = 0;

  switch(cmd) {
    case PP_IOCTL_INIT: {
      pp_init_params pp_init;

      DPRINTK("PP_IOCTL_INIT\n");
      if(copy_from_user(&pp_init, (void *)arg, _IOC_SIZE(cmd))) {
        retval = -EFAULT;
        break;
      }

      retval = mxc_pp_init(&pp_init);
      if(retval < 0) break;

      pp_data.busy_flag = 0;
      break;
    }
    case PP_IOCTL_UNINIT: {
      DPRINTK("PP_IOCTL_UNINIT\n");
      retval = mxc_pp_uninit();                        
      break;
    }
    case PP_IOCTL_REQBUFS: {
      pp_reqbufs_params reqbufs;
      DPRINTK("PP_IOCTL_REQBUFS\n");

      if(copy_from_user(&reqbufs, (void *)arg, _IOC_SIZE(cmd))) {
        retval = -EFAULT;
        break;
      }

      retval = mxc_pp_reqbufs(&reqbufs);
      break;
    }
    case PP_IOCTL_QUERYBUF: {
      pp_buf buf;
      DPRINTK("PP_IOCTL_QUERYBUF\n");

      if(copy_from_user(&buf, (void *)arg, _IOC_SIZE(cmd))) {
        retval = -EFAULT;
        break;
      }

      if((buf.index < 0) || 
         (buf.index >= PP_MAX_BUFFER_CNT) ||
         (pp_data.buf[buf.index].index != buf.index)) {
        retval = -EINVAL;
        break;
      }

      // Return size of memory allocated
      if(copy_to_user((void *)arg, &pp_data.buf[buf.index], _IOC_SIZE(cmd))) {
        retval = -EFAULT;
        break;
      }
                        
      break;
    }
    case PP_IOCTL_START: {
      int index;
      pp_start_params start_params;
      DPRINTK("PP_IOCTL_START\n");

      if(pp_data.busy_flag) {
        retval = -EBUSY;
        break;
      }
      pp_data.busy_flag = 1;

      if(copy_from_user(&start_params, (void *)arg, _IOC_SIZE(cmd))) {
        retval = -EFAULT;
        break;
      }

      index = start_params.in.index;
      if((index >= 0) && (index < PP_MAX_BUFFER_CNT)) {
        if(pp_data.buf[index].addr != start_params.in.addr) {
          retval = -EINVAL;
          break;
        }
      }

      index = start_params.out.index;
      if((index >= 0) && (index < PP_MAX_BUFFER_CNT)) {
        if(pp_data.buf[index].addr != start_params.out.addr) {
          retval = -EINVAL;
          break;
        }
      }

      if(pp_data.mode >= PP_PP_ROT) {
        if(pp_data.ic_combine_en != 0) {
          index = start_params.in_comb.index;
          if(pp_data.buf[index].addr != start_params.in_comb.addr) {
            retval = -EINVAL;
            break;
          }
	}
        if(pp_data.mode == PP_PP_ROT) {
          index = start_params.mid.index;
          if(pp_data.buf[index].addr != start_params.mid.addr) {
            retval = -EINVAL;
            break;
	  }
	}
      }

      //Update buffers in DMA Channels
      if((retval = mxc_pp_start(&start_params.in, &start_params.in_comb,
                                &start_params.mid, &start_params.out)) < 0)
        break;

      DPRINTK("PP_IOCTL_START - processing started\n");

      if(!start_params.wait)
        break;

      DPRINTK("PP_IOCTL_START - waiting for completion\n");
      DPRINTK("ipu_conf = 0x%08X, ic_conf = 0x%08X, buf0_rdy = 0x%08X, \
               int_ctrl_1 = 0x%08X, int_stat_1 = 0x%08X\n", \
              readl(IPU_CONF), readl(IC_CONF), readl(IPU_CHA_BUF0_RDY), \
              readl(IPU_INT_CTRL_1), readl(IPU_INT_STAT_1));
      /* Fall thru to wait */
    }
    case PP_IOCTL_WAIT: {
      if(!wait_event_interruptible_timeout(pp_data.pp_wait, pp_data.done_flag == 1, 2 * HZ)) {
        retval = -ETIME;
        break;
      }
      else if(signal_pending(current)) {
        DPRINTK("PP_IOCTL_WAIT: interrupt received\n");
        retval = -ERESTARTSYS;
        break;
      }

      pp_data.busy_flag = 0;

      DPRINTK("PP_IOCTL_WAIT - processing finished\n");
      break;
    }
    default:
      printk(" ipu_pp_ioctl not supported ioctls\n");
      retval = -1;
  }

  if(retval < 0) DPRINTK("return = %d\n", retval);
  return retval;
}

/*!
 * Post-processing driver mmap function. This function implements the Linux
 * file_operations.mmap() API function for mapping driver buffers to user space. 
 *
 * @param       file            struct file * 
 *
 * @param       vma             structure vm_area_struct *
 *
 * @return status     0 Success, EINTR busy lock error, ENOBUFS remap_page error. 
 */
static int
mxc_pp_mmap(struct file *file, struct vm_area_struct *vma)
{
  unsigned long start = vma->vm_start;
  unsigned long size  = vma->vm_end - vma->vm_start;
  int res = 0;

  FUNC_START;

  /* make this _really_ smp-safe */
  if(down_interruptible(&pp_data.busy_lock))
    return -EINTR;

  if(remap_pfn_range(vma, start, vma->vm_pgoff, size, pgprot_noncached(vma->vm_page_prot))) {
    DPRINTK("remap_pfn_range failed\n");
    res = -ENOBUFS;
    goto mmap_exit;
  }

  DPRINTK("phys addr 0x%08X mmapped to virt addr 0x%08X, size = 0x%08X\n",
           vma->vm_pgoff << PAGE_SHIFT, start, size);

mmap_exit:
  up(&pp_data.busy_lock);

  FUNC_END;

  return res;
}

/*!
 * Post-processing driver poll function. This function implements the Linux
 * file_operations.poll() API function. 
 *
 * @param file       structure file *
 *
 * @param wait       structure poll_table * 
 *
 * @return  status   POLLIN | POLLRDNORM
 */
static unsigned int
mxc_pp_poll(struct file *file, poll_table *wait)
{
  wait_queue_head_t *queue = NULL;
  int res = POLLIN | POLLRDNORM;

  FUNC_START;

  if(down_interruptible(&pp_data.busy_lock))
    return -EINTR;

  queue = &pp_data.pp_wait;
  poll_wait(file, queue, wait);

  up(&pp_data.busy_lock);

  FUNC_END;
  return res;
}

/*!
 * File operation structure functions pointers.
 */
static struct file_operations mxc_pp_fops = {
  .owner   = THIS_MODULE,
  .open    = mxc_pp_open,
  .release = mxc_pp_release,
  .ioctl   = mxc_pp_ioctl,
  .poll    = mxc_pp_poll,
  .mmap    = mxc_pp_mmap,
};

static int mxc_pp_major = 0;

/*!
 * Post-processing driver module initialization function.
 */
int mxc_pp_dev_init(void)
{
  FUNC_START;

  mxc_pp_major = register_chrdev(0, "mxc_ipu_pp", &mxc_pp_fops);

  if(mxc_pp_major < 0) {
    printk(KERN_INFO "Unable to get a major for mxc_ipu_pp");
    return mxc_pp_major;
  }

  devfs_mk_cdev(MKDEV(mxc_pp_major, 0), S_IFCHR | S_IRUGO | S_IWUGO, "mxc_ipu_pp");

  printk(KERN_INFO "IPU Post-processing loading\n");

  FUNC_END;

  return 0;
}

/*!
 * Post Processing driver module exit function.
 */
static void mxc_pp_exit(void)
{
  FUNC_START;

  if(mxc_pp_major > 0) {
    devfs_remove("mxc_ipu_pp");
    unregister_chrdev(mxc_pp_major, "mxc_ipu_pp");
  }

  FUNC_END;

  printk(KERN_INFO "IPU Post-processing: successfully unloaded\n");
}

module_init(mxc_pp_dev_init);
module_exit(mxc_pp_exit);

MODULE_AUTHOR("Motorola, Inc.");
MODULE_DESCRIPTION("MXC IPU Post Processing Driver");
MODULE_LICENSE("GPL");
