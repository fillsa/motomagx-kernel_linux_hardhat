/*
 * Copyright 2006 Motorola, Inc.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the
 * Free Software Foundation, Inc.
 * 59 Temple Place, Suite 330
 * Boston, MA 02111-1307 USA
 *
 */
/*!
 * @defgroup MXC_PP Post Processing Driver
 */
/*!
 * @file mxc_pp.h
 * 
 * @brief MXC IPU Post-processing driver 
 * 
 * User-level API for IPU Hardware Post-processing.
 *
 * @ingroup MXC_PP

               Modification     Tracking
Author                 Date         Description of Changes
----------------   ------------    -------------------------
Motorola           08/17/2006       File Created
 */
#ifndef _INCLUDED_MXC_PP_H
#define _INCLUDED_MXC_PP_H


#define PP_MAX_BUFFER_CNT 4

/*!
 * Enumeration of Post-processing modes
 */
typedef enum {
  PP_DISABLE_ALL = 0,
  PP_ROT         = 1,
  PP_PP_ROT      = 2,
  PP_PP          = 3,
} pp_operation_t;

/*!
 * Structure for Post-processing initialization parameters.
 */
typedef struct {
  pp_operation_t mode;
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
  uint16_t rot;
  uint16_t ic_combine_en;
} pp_init_params;

/*!
 * Structure for Post Processing buffer request parameters.
 */
typedef struct {
  int count;              /*!< Number of buffers requested */
  __u32 req_size;
} pp_reqbufs_params;

/*!
 * Structure for Post Processing buffer request parameters.
 */
typedef struct {
  int index;
  int size;    /*!< Size of buffer allocated */
  __u32 addr;  /*!< Buffer address in driver memory. Set by QUERYBUF */
} pp_buf;

/*!
 * Structure for Post-processing start parameters.
 */
typedef struct {
  pp_buf in;       /*!< Input buffer address */
  pp_buf in_comb;  /*!< Second Input buffer address in case ic_combine_en = 1 */
  pp_buf mid;      /*!< Middle buffer address in case both PP and ROT are needed */
  pp_buf out;      /*!< Output buffer address */
  int wait;
} pp_start_params;

/*! @name User Client Ioctl Interface */
/*! @{ */

/*!
 * IOCTL to Initialize the Post-processing.
 */
#define PP_IOCTL_INIT           _IOW('F',0x0, pp_init_params)

/*!
 * IOCTL to Uninitialize the Post-processing.
 */
#define PP_IOCTL_UNINIT         _IO('F',0x1)

/*!
 * IOCTL to set the buffer mode and allocate buffers if driver allocated.
 */
#define PP_IOCTL_REQBUFS        _IOWR('F',0x2, pp_reqbufs_params)

/*!
 * IOCTL to set the buffer mode and allocate buffers if driver allocated.
 */
#define PP_IOCTL_QUERYBUF       _IOR('F',0x2, pp_buf)

/*!
 * IOCTL to start post-processing on a frame of data. This ioctl may block until
 * processing is done or return immediately.
 */
#define PP_IOCTL_START          _IOWR('F',0x3, pp_start_params)

/*!
 * IOCTL to wait for post-processing to complete.
 */
#define PP_IOCTL_WAIT           _IO('F',0x4)
/*! @} */

#endif /* _INCLUDED_MXC_PP_H */
