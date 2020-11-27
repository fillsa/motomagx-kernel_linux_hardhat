/*
 * Copyright (c) 2006 Motorola, Inc, All Rights Reserved.
 *
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   *  Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   *  Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   *  Neither the name of Motorola nor the names of its contributors may
 *      be used to endorse or promote products derived from this software without
 *      specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 03/28/2006       Motorola         MTP initial
 * 04/26/2006       Motorola         fix MTP ctl message header
 * 05/16/2006       Motorola         PictBridge support 
 * 09/18/2006       Motorola         Cancel sync from device to host
 * 11/30/2006       Motorola         fix shared recv_len 
 * 
 * mtp.h - MTP protocol library header
*/

#ifndef _MTP_H_
#define _MTP_H_

extern char *mtp_if_name;

extern otg_tag_t mtp_fd_trace_tag;
#define MTP mtp_fd_trace_tag

/*
 * TRACE to kernel log for debugging.
 * Comment out the next line before release.
 */
/*#define MTP_TRACE_PRINTK*/

#ifdef MTP_TRACE_PRINTK

#undef TRACE_MSG0
#undef TRACE_MSG1
#undef TRACE_MSG2
#undef TRACE_MSG3
#undef TRACE_MSG4
#undef TRACE_MSG5
#undef TRACE_MSG6
#undef TRACE_MSG7
#undef TRACE_MSG8

#define TRACE_MSG0(m, f)                                 printk(KERN_INFO "MTP: " f "\n")
#define TRACE_MSG1(m, f, a1)                             printk(KERN_INFO "MTP: " f "\n", a1)
#define TRACE_MSG2(m, f, a1, a2)                         printk(KERN_INFO "MTP: " f "\n", a1, a2)
#define TRACE_MSG3(m, f, a1, a2, a3)                     printk(KERN_INFO "MTP: " f "\n", a1, a2, a3)
#define TRACE_MSG4(m, f, a1, a2, a3, a4)                 printk(KERN_INFO "MTP: " f "\n", a1, a2, a3, a4)
#define TRACE_MSG5(m, f, a1, a2, a3, a4, a5)             printk(KERN_INFO "MTP: " f "\n", a1, a2, a3, a4, a5)
#define TRACE_MSG6(m, f, a1, a2, a3, a4, a5, a6)         printk(KERN_INFO "MTP: " f "\n", a1, a2, a3, a4, a5, a6)
#define TRACE_MSG7(m, f, a1, a2, a3, a4, a5, a6, a7)     printk(KERN_INFO "MTP: " f "\n", a1, a2, a3, a4, a5, a6, a7)
#define TRACE_MSG8(m, f, a1, a2, a3, a4, a5, a6, a7, a8) printk(KERN_INFO "MTP: " f "\n", a1, a2, a3, a4, a5, a6, a7, a8)

#endif

/*
 * Class Code
 */

#define MTP_CLASS_IMAGE_INTERFACE                  0x06
#define MTP_SUBCLASS_STILL_IMAGE_CAPTURE_DEVICE    0x01
#define MTP_PROTOCOL_CODE                          0x01

/*
 * endpoint and interface indexes
 */
#define BULK_OUT        0x00
#define BULK_IN         0x01
#define INT_IN          0x02
#define ENDPOINTS       0x03

/* Max size of data URB */
#define MTP_RECV_URB_MAX_SIZE 8192
#define MTP_SEND_URB_MAX_SIZE 8192

/*
 * The actual number of urbs allowed for data receiving and sending.
 * The max amount of data recv is MTP_RECV_URBS_MAX*MTP_RECV_URB_MAX_SIZE.
 * The max amount of data send is MTP_SEND_URBS_MAX*MTP_SEND_URB_MAX_SIZE.
 */
#define MTP_RECV_URBS_MAX 8
#define MTP_SEND_URBS_MAX 4

/*
 * URB EP0 URB max size.
 */
#define MTP_EP0_SEND_MAX_SIZE 512
#define MTP_EP0_RECV_MAX_SIZE 512

/* special value to identify cancel IO */
#define MTP_CANCEL_IO_MARKER    0x1FFFFFFF

/*
 * URB circular buffer
 * Queue like - we append to tail and remove from head.
 */
struct mtp_cubuf {
    int head;                 /* index of first ready one */
    int tail;                 /* index of next avaiable space; tail==head means empty */
    int size;                 /* number of URBS in buf */
    struct usbd_urb *urbs[0];
};

/*
 * MTP private data
 */
struct mtp_private {
    struct usbd_function_instance *function_instance;

    /* Is it high speed? */
    BOOL hs; 

    /* endpoint buf size */
    int outsize;
    int insize;

    /* URB transfer size */
    int writesize;                  
    int readsize;

    /* device request receive */
    unsigned char *req_recv_buf;
    unsigned char *req_recv_start;
    unsigned char *req_recv_end;
    wait_queue_head_t req_recv_wait;
    int req_ms_vendor_flag;

    /* device request send */
    int req_send_len;
    wait_queue_head_t req_send_wait;

    /* data receive */
    struct mtp_cubuf *recv_urbs;
    int recv_pos;                       /* point to the read position in the current urb */
    volatile int recv_len;                       /* total receiving data length of all urbs */
    wait_queue_head_t recv_wait;

    /* data send */
    int send_urbs_count;                /* number of urbs that mtp stack wants to send */
    int sent_urbs_count;                /* number of urbs that have been sent out by usb */
    wait_queue_head_t send_wait;
};

/*
 * cubuf functions
 */
struct mtp_cubuf *mtp_cubuf_create(int size);
void mtp_cubuf_destroy(struct mtp_cubuf *cu);
int mtp_cubuf_next(struct mtp_cubuf *cu, int pos);
bool mtp_cubuf_is_full(struct mtp_cubuf *cu);
int mtp_cubuf_append(struct mtp_cubuf *cu, struct usbd_urb *urb);
struct usbd_urb *mtp_cubuf_remove(struct mtp_cubuf *cu);
struct usbd_urb *mtp_cubuf_get(struct mtp_cubuf *cu);

#endif  /* _MTP_H_ */


