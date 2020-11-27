/*
 * otg/function/msc/msc.h - Mass Storage Class
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2007 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 05/11/2005         Motorola         Initial distribution 
 * 12/30/2005         Motorola         Added multi-LUN support 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 03/07/2007         Motorola         Added function to use get_free_pages.
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA
 *
 */
/*!
 * @defgroup MSCFunction Mass Storage 
 * @ingroup functiongroup
 */
/*!
 * @file otg/functions/msc/msc.h
 * @brief Mass Storage Driver private defines
 *
 *
 * @ingroup MSCFunction
 */

#ifndef MSC_H
#define MSC_H 1


/*------------------------------*/
/*     State Machines           */
/*------------------------------*/
typedef enum msc_command_state {
        MSC_READY,
        MSC_DATA_OUT_WRITE,
        MSC_DATA_OUT_WRITE_FINISHED,
        MSC_DATA_OUT_WRITE_ERROR,
        MSC_DATA_IN_READ,
        MSC_DATA_IN_READ_FINISHED,
        MSC_STATUS,
        MSC_QUERY,
	MSC_WAITFOR_RESET,
	MSC_WAITFOR_CLEAR,
	MSC_PHASE_ERROR,
        MSC_UNKNOWN
} msc_command_state_t;


typedef enum msc_io_state {
        MSC_INACTIVE                 = 0x0000,
        MSC_BLOCKIO_PENDING          = 0x0001,
        MSC_BLOCKIO_FINISHED         = 0x0002,
        MSC_RECV_PENDING             = 0x0004,
        MSC_RECV_FINISHED            = 0x0008,
        MSC_SEND_PENDING             = 0x0010
} msc_io_state_t;


typedef enum msc_card_state {
        DEVICE_EJECTED          = 0x0001,          // MEDIA_EJECTED
        DEVICE_INSERTED         = 0x0002,          // MEDIA_INSERT
        DEVICE_OPEN             = 0x0004,          // WR_PROTECT_OFF
        DEVICE_WRITE_PROTECTED  = 0x0008,          // WR_PROTECT_ON
        DEVICE_CHANGE_ON        = 0x0010,          // MEDIA_CHANGE_ON
        DEVICE_PREVENT_REMOVAL  = 0x0020
} msc_card_state_t;



/*------------------------------*/
/*     MSC Structure            */
/*------------------------------*/
struct msc_private {

        unsigned char           connected;              // non-zero if connected to host (configured) 
  
        struct usbd_urb       * rcv_urb_finished;       // Used to hold a pending urb (for write)
        struct bio            * rcv_bio_finished;       // Used to hold a pending bio (for read)

	unsigned int bytes_done;
	int err;

	u8                      read_pending;
	u8                      write_pending;
	u8                      unknown_cmd_ioctl_pending;
  	u8                      unknown_cmd_pending;


        msc_command_state_t     command_state;          // current command state
        msc_io_state_t          io_state;               // current IO state
	u8                      endpoint_state;         // Bulk in and Bulk out state bit 0 IN, bit 1 OUT. The rest are reserved

        COMMAND_BLOCK_WRAPPER   command;

        u32                     lba;                            // next lba to read/write from
        u32                     transfer_blocks;
        u32                     TransferLength_in_blocks;       // amount of transfer remaining
        u32                     TransferLength_in_bytes;        // amount of transfer remaining
        u32                     data_transferred_in_bytes;      // amount of data actually transferred

        int                     major;
        int                     minor;

        int                     close_pending;
        dev_t                   dev;
        struct gendisk          *disk;
        struct block_device     *bdev;
        u32                     block_size;
        u32                     capacity;                      
        u32                     max_blocks;                      


        wait_queue_head_t       rdwr_wq;                // This queue is awoken when done reading/writing
        wait_queue_head_t       ioctl_wq;               // This queue is awoken when ioctls pending ioclts need to be awoken.

        u32                     status;
        u32                     block_dev_state;
        u32                     sensedata;
        u32                     info;
        struct usbd_urb         *rcv_urb_bio_current;     // rcv urb for current bio (for write)


};


/*------------------------------*/
/*     Defines                  */
/*------------------------------*/
#define DEF_NUMBER_OF_HEADS     0x10
#define DEF_SECTORS_PER_TRACK   0x20

#define BULK_OUT        0x00
#define BULK_IN         0x01
#define ENDPOINTS       0x02

#define BULK_IN_HALTED    0x01
#define BULK_OUT_HALTED   0x02

#define MSC msc_fd_trace_tag


/*------------------------------*/
/*     External Variables       */
/*------------------------------*/
extern struct usbd_function_operations function_ops;
extern struct usbd_function_driver function_driver;
extern otg_tag_t msc_fd_trace_tag;

extern int msc_maxluns ;
extern int msc_curluns ;
extern int msc_cmdlun  ;

#endif
