/*
 * Copyright (c) 2006-2008 Motorola, Inc, All Rights Reserved.
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
 * 03/28/2006        Motorola         MTP initial
 * 04/26/2006        Motorola         fix MTP ctl message header
 * 06/06/2006        Motorola         PictBridge support
 * 08/08/2006        Motorola         Move MTP descriptors to user space
 * 09/06/2006        Motorola         MTP HS fix
 * 09/18/2006        Motorola         Cancel sync from device to host
 * 01/23/2007        Motorola         EP0 Cancel sync from device to host
 * 04/25/2007        Motorola         Reading large data panic fix
 * 05/07/2007        Motorola         Added pbg-if function instance for COV's
 * 05/15/2007         Motorola         Don't schedule hotplug in reset if there is one scheduled.
 * 08/24/2007        Motorola         Changes for open source compliance.
 * 10/16/2007        Motorola         Add suspend and resume handling.
 * 08/08/2008        Motorola         Wake up suspend_wait in function disable.
 * 11/14/2008        Motorola         Clean the work queue before module exit.
 * 
 */

/*!
 * @file otg/functions/mtp/mtp-fd.c
 * @brief MTP Descriptor Set
 *
 * This is the generic portion of the MTP driver.
 *
 * @ingroup MTPFunction
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/smp_lock.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>
#include <otg/usbp-mcpc.h>

#include <otg/otg-trace.h>

#ifdef CONFIG_OTG_MTP_HOTPLUG
#include <otg/hotplug.h>
#endif

#include "mtp.h"
#include "mtp-io.h"

/* bVendorCode used for Microsoft OS Descriptor requests */ 
#define USB_MOD_VENDOR_CODE       0x1C 

#ifdef LINUX26
struct workqueue_struct * mtp_fd_workqueue = NULL; 

void mtp_fd_init(struct workqueue_struct * in_queue)
{
    mtp_fd_workqueue = in_queue;
}

void mtp_fd_exit(void)
{
    if(mtp_fd_workqueue != NULL)
    {
        flush_workqueue(mtp_fd_workqueue);
        destroy_workqueue(mtp_fd_workqueue);
        mtp_fd_workqueue = NULL;
    }
}

#endif
/* data for BH of device_request */
struct mtp_dev_req_bh_private {
    WORK_ITEM bh;
    struct usbd_function_instance * function_instance;
    unsigned char *data;
    int data_len;
    BOOL pending_data;
};

struct mtp_suspend_bh_private {
    WORK_ITEM bh;
};

static struct mtp_suspend_bh_private mtp_suspend_bh_pri;
static wait_queue_head_t suspend_wait ;

otg_tag_t mtp_fd_trace_tag;
char *mtp_if_name;

#ifdef CONFIG_OTG_MTP_HOTPLUG
static struct usb_hotplug_private mtp_gen_priv;
#endif

#define MTP_EXT_DESC_MAX_LEN    64
#define MTP_EXT_STR_MAX_LEN     32

static u8 mtp_extended_string_descriptor[MTP_EXT_STR_MAX_LEN];

int mtp_ext_id = 0;
char *mtp_ext_desc = NULL;

int mtp_ext_str_idx = 0;
char *mtp_ext_str_desc = NULL;

/* for ep0 scheduling */
static unsigned char* pending_ep0_data = NULL;
static int pending_ep0_data_len = 0;
static BOOL ready_for_ep0_data = FALSE;

/* ********************************************************************************************* */
/* ********************************************************************************************* */

int mtp_start_recv_urb(struct usbd_function_instance *function_instance);

int mtp_ctl_init(struct usbd_function_instance *function_instance);
void mtp_ctl_exit(void);

int mtp_dev_init(struct usbd_function_instance *function_instance);
void mtp_dev_exit(void);

void mtp_suspend_bh (void *data);

/*! mtp_cubuf_create - create a new cubuf
 *
 * @param int     size of urb the buffer can hold
 * Return pointer to new struct cubuf; or NULL if failed.
 */
struct mtp_cubuf *mtp_cubuf_create(int size)
{
    struct mtp_cubuf *cu = CKMALLOC(sizeof(struct mtp_cubuf) + size * sizeof(struct usbd_urb*), GFP_KERNEL);
    int i;

    if (cu != NULL)
    {
        cu->head = 0;
        cu->tail = 0;
        cu->size = size;

        /* we allocated the urbs array at the end of the structure */
        TRACE_MSG3(MTP, "sizeof=%d, cu=%x, cu->urbs=%x", sizeof(struct mtp_cubuf), cu, cu->urbs);
        for (i = 0; i < size; i++)
        {
            cu->urbs[i] = NULL;
        }
    }
        
    return cu;
}

/*! mtp_cubuf_destroy - destroy a cubuf
 *
 * @param cu     pointer to cubuf
 * No return value.
 */
void mtp_cubuf_destroy(struct mtp_cubuf *cu)
{
    LKFREE(cu);
}

/*! mtp_cubuf_next - next location in cubuf
 *
 * @param cu     pointer to cubuf
 * @param int    the location to check
 * Return next location of urb in cubuf; or -1 if failed.
 */
int mtp_cubuf_next(struct mtp_cubuf *cu, int pos)
{
    return ((pos + 1 == cu->size) ? 0 : pos + 1);
}

/*! mtp_cubuf_next - check if cubuf is full
 *
 * @param cu     pointer to cubuf
 * Return TRUE if cubuf is full; or FALSE otherwise.
 */
bool mtp_cubuf_is_full(struct mtp_cubuf *cu)
{
    return ((mtp_cubuf_next(cu, cu->tail)) == cu->head ? TRUE : FALSE);
}

/*! mtp_cubuf_append - append urb into a cubuf
 *
 * @param cu     pointer to cubuf
 * @param urb    the urb to put into cubuf
 * Return location of urb in cubuf on success; or -1 if failed.
 */
int mtp_cubuf_append(struct mtp_cubuf *cu, struct usbd_urb *urb)
{
    int loc = cu->tail;
    cu->urbs[loc] = urb;

    /* move tail ahead */
    cu->tail = mtp_cubuf_next(cu, loc);
    return loc;
}

/*! mtp_cubuf_remove - remove the first ready urb from a cubuf
 *
 * @param cu     pointer to cubuf
 * Return pointer to the urb removed; or NULL if failed.
 */
struct usbd_urb *mtp_cubuf_remove(struct mtp_cubuf *cu)
{
    int loc = cu->head;
    struct usbd_urb *urb = cu->urbs[loc];

    cu->urbs[loc] = NULL;

    /* move head ahead */
    cu->head = mtp_cubuf_next(cu, loc);
    return urb;
}

/*! mtp_cubuf_get - get the first ready urb in a cubuf but do not remove
 *
 * @param cu     pointer to cubuf
 * Return pointer to the first ready urb; or NULL if failed.
 */
struct usbd_urb *mtp_cubuf_get(struct mtp_cubuf *cu)
{
    int loc = cu->head;
    struct usbd_urb *urb = cu->urbs[loc];

    return urb;
}

/*! mtp_dev_req_buf_avail - device request circular buffer free space available
 *
 * @param mtp     pointer to mtp private data
 * Return length of available buf in device request circular buffer
 */
int mtp_dev_req_buf_avail(struct mtp_private *mtp)
{
    int return_value = 0; 
    unsigned char *local_buf_in_ptr; 
    unsigned char *local_buf_out_ptr; 
    unsigned long flags;

    RETURN_ZERO_UNLESS(mtp);
    RETURN_ZERO_UNLESS((mtp->req_recv_end != NULL && mtp->req_recv_start != NULL));
    
    local_irq_save(flags);
    local_buf_in_ptr = mtp->req_recv_end;
    local_buf_out_ptr = mtp->req_recv_start;
    local_irq_restore(flags);

    /* Calculate how many bytes of free space available in the circular buffer.   
       If the pointer is equal to the out pointer, no data is available.   
       Therefore the maximum number of bytes of data the buffer can hold is  
       local_buf_size-1.  The bytes immediately before the out pointer can not be used  
       to hold data, because in that case there is no way to distinguish between an  
       empty buffer or a full buffer  
    */ 
    if ( local_buf_out_ptr > local_buf_in_ptr ) 
    { 
        return_value = local_buf_out_ptr - local_buf_in_ptr -1; 
    } 
    else 
    { 
        return_value = MTP_EP0_RECV_MAX_SIZE -1 - (local_buf_in_ptr - local_buf_out_ptr); 
    }

    return return_value;
}

/*! mtp_dev_req_get - copy out of circular buffer
 *
 * @param mtp      pointer to mtp private data
 * @param buf      pointer to data buffer
 * @param buf_len  len of buffer
 * Return length of data copied
 */
int mtp_dev_req_get(struct mtp_private *mtp, unsigned char *buf, int buf_len)
{
    int data_avail;
    int bytes_to_copy;
    unsigned char *local_buf_out_ptr; 
    unsigned char *local_buf_start_addr; 
    unsigned char *local_buf_end_boundary; 
    int bytes_before_wraparound; 
    int bytes_after_wraparound;
    unsigned long flags;

    RETURN_ZERO_UNLESS(mtp);
    RETURN_ZERO_UNLESS((mtp->req_recv_end != NULL && mtp->req_recv_start != NULL));

    data_avail = MTP_EP0_RECV_MAX_SIZE -1 - mtp_dev_req_buf_avail(mtp);
    bytes_to_copy = MIN(data_avail, buf_len);

    if (bytes_to_copy == 0) 
    {
        TRACE_MSG0(MTP, "no data copy");
        return 0;
    }

    local_irq_save(flags);
    local_buf_out_ptr = mtp->req_recv_start;
    local_buf_start_addr = mtp->req_recv_buf;
    local_irq_restore(flags);
    
    local_buf_end_boundary = local_buf_start_addr + MTP_EP0_RECV_MAX_SIZE; 
 
    /* Decide how many bytes would fall before and after end of buffer boundary. */ 
    if ( bytes_to_copy < ( local_buf_end_boundary - local_buf_out_ptr ) ) 
    { 
        bytes_before_wraparound = bytes_to_copy;
        bytes_after_wraparound = 0; 
    } 
    else 
    { 
        bytes_before_wraparound = local_buf_end_boundary - local_buf_out_ptr; 
        bytes_after_wraparound = bytes_to_copy - bytes_before_wraparound; 
    } 
 
    /* The number of bytes before the end of the circular buffer are 
       now known - copy them from the circular buffer. */ 
    copy_to_user(buf, local_buf_out_ptr, bytes_before_wraparound); 

    /* If there are any remaining bytes to be copied, they must be copied at the start 
       of the circular buffer. */ 
    if( bytes_after_wraparound > 0) 
    { 
        copy_to_user(buf + bytes_before_wraparound, local_buf_start_addr, bytes_after_wraparound); 
        
        /* Update local position of in ptr from start of buffer. */ 
        local_buf_out_ptr = local_buf_start_addr + bytes_after_wraparound; 
    } 
    else /* Wraparound did not occur. Update local position of in ptr from present position. */ 
    { 
        local_buf_out_ptr += bytes_before_wraparound; 
        
        /* Check if the local_buf_ptr is pointing to the next bytes after 
           the last byte of the circular buffer, it needs to point back to 
           the beginning of teh circular buffer */ 
        if (local_buf_out_ptr == local_buf_end_boundary) 
        { 
            local_buf_out_ptr = local_buf_start_addr; 
        } 
    }

    /* Update the in pointer */ 
    mtp->req_recv_start = local_buf_out_ptr; 

    return bytes_to_copy;
}

/*! mtp_dev_req_put - copy into circular buffer
 *
 * @param mtp      pointer to mtp private data
 * @param data     pointer to data
 * @param data_len len of date
 * Return length of data copied
 */
int mtp_dev_req_put(struct mtp_private *mtp, unsigned char *data, int data_len)
{
    int buf_avail;
    unsigned char *local_buf_in_ptr; 
    unsigned char *local_buf_start_addr; 
    unsigned char *local_buf_end_boundary; 
    int bytes_before_wraparound; 
    int bytes_after_wraparound; 
    unsigned long flags;

    RETURN_ZERO_UNLESS(mtp);
    RETURN_ZERO_UNLESS((mtp->req_recv_end != NULL && mtp->req_recv_start != NULL));

    buf_avail = mtp_dev_req_buf_avail(mtp);
    if ( buf_avail < data_len ) 
    {
        /* no point to copy incomplete req - this should never happen */
        TRACE_MSG2(MTP, "Not enough space to add dev req data, data_len=%d, buf_avail=%d", data_len, buf_avail);
        return 0;
    }

    local_irq_save(flags);
    local_buf_in_ptr = mtp->req_recv_end;
    local_buf_start_addr = mtp->req_recv_buf;
    local_irq_restore(flags);
    
    local_buf_end_boundary = local_buf_start_addr + MTP_EP0_RECV_MAX_SIZE; 
 
    /* Decide how many bytes would fall before and after end of buffer boundary. */ 
    if ( data_len < ( local_buf_end_boundary - local_buf_in_ptr ) ) 
    { 
        bytes_before_wraparound = data_len;
        bytes_after_wraparound = 0; 
    } 
    else 
    { 
        bytes_before_wraparound = local_buf_end_boundary - local_buf_in_ptr; 
        bytes_after_wraparound = data_len - bytes_before_wraparound; 
    } 
 
    /* The number of bytes that can fit before the end of the circular buffer are 
       now known - copy them into the circular buffer. */ 
    memcpy(local_buf_in_ptr, data, bytes_before_wraparound); 

    /* If there are any remaining bytes to be copied, they must be copied at the start 
       of the circular buffer. */ 
    if( bytes_after_wraparound > 0) 
    { 
        memcpy(local_buf_start_addr, data + bytes_before_wraparound, bytes_after_wraparound); 
        
        /* Update local position of in ptr from start of buffer. */ 
        local_buf_in_ptr = local_buf_start_addr + bytes_after_wraparound; 
    } 
    else /* Wraparound did not occur. Update local position of in ptr from present position. */ 
    { 
        local_buf_in_ptr += bytes_before_wraparound; 
        
        /* Check if the local_buf_ptr is pointing to the next bytes after 
           the last byte of the circular buffer, it needs to point back to 
           the beginning of teh circular buffer */ 
        if (local_buf_in_ptr == local_buf_end_boundary) 
        { 
            local_buf_in_ptr = local_buf_start_addr; 
        } 
    }

    /* Update the in pointer */ 
    mtp->req_recv_end = local_buf_in_ptr; 

    return data_len;
}

/*! mtp_recv_urb_ep0 - process a received urb on EP0
 *
 * @param urb     Pointer to the urb that has been received.
 * @param rc      Result code
 * Return non-zero if urb was not disposed of.
 */
static int mtp_recv_urb_ep0 (struct usbd_urb *urb, int rc)
{
    struct usbd_function_instance *function_instance = urb->function_privdata;
    struct mtp_private *mtp = function_instance->privdata;
    unsigned long flags;

    if (rc!=0) return rc;

    TRACE_MSG2(MTP, "RECV URB EP0 length=%d, ready_for_ep0_data=%d", urb->actual_length, ready_for_ep0_data);

    if (ready_for_ep0_data)
    {
        ready_for_ep0_data = FALSE;
        
        /* copy data to buffer */
        local_irq_save(flags);
        mtp_dev_req_put(mtp, urb->buffer, urb->actual_length);
        local_irq_restore(flags);

        /* data ready */
        wake_up_interruptible(&mtp->req_recv_wait);
    }
    else
    {
        /* data arrived before dev_req BH -- copy data to a tmp place */
        RETURN_ENOMEM_UNLESS ((pending_ep0_data = CKMALLOC(urb->actual_length, GFP_KERNEL)));
        memcpy(pending_ep0_data, urb->buffer, urb->actual_length);
        pending_ep0_data_len = urb->actual_length;
    }
    
    urb->function_privdata = NULL;
    usbd_free_urb(urb);
    return 0;
}

/*! mtp_sent_urb_ep0 - callback for completed EP0 xmit URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @param urb     Pointer to the urb that has been sent.
 * @param urb_rc  Result code from the send operation.
 *
 * @return non-zero for failure.
 */
int mtp_sent_urb_ep0(struct usbd_urb *urb, int urb_rc)
{
    struct usbd_function_instance *function_instance = urb->function_privdata;
    struct mtp_private *mtp = function_instance->privdata;
    
    TRACE_MSG3(MTP, "urb: %p urb_rc: %d length: %d", urb, urb_rc, urb->actual_length);

    urb->function_privdata = NULL;
    usbd_free_urb(urb);
    mtp->req_send_len = 0;

    /* wake up anyone waiting to write */
    wake_up_interruptible(&mtp->req_send_wait);
    TRACE_MSG0(MTP, "leaving mtp_sent_urb_ep0");
    return 0;
}

/*! mtp_start_xmit_ep0 - start sending a buffer over EP0
 *
 * @param function_instance The function instance for this driver to use.
 * @param buffer The data to send.
 * @param len The data length.
 * @return: non-zero if there is an error in the USB layer.
 */
int mtp_start_xmit_ep0 (struct usbd_function_instance *function_instance, u8 *buffer, int len)
{
    struct mtp_private *mtp = function_instance->privdata;
    struct usbd_urb *urb = NULL;
    int rc;

    TRACE_MSG1(MTP,"enter mtp_start_xmit_ep0, len=%d", len);

    if (len > MTP_EP0_SEND_MAX_SIZE)
    {
        TRACE_MSG2(MTP,"len=%d oversize, truncate to %d", len, MTP_EP0_SEND_MAX_SIZE);
        len = MTP_EP0_SEND_MAX_SIZE;
    }
    
    RETURN_EINVAL_UNLESS((urb = usbd_alloc_urb_ep0(function_instance, len, mtp_sent_urb_ep0)));
    urb->function_privdata = function_instance;
    copy_from_user(urb->buffer, buffer, len);
    urb->actual_length = len;

    TRACE_MSG1(MTP, "start sending urb len=%d", len);
    if ((rc = usbd_start_in_urb (urb))) {
        TRACE_MSG1(MTP, "FAILED: %d", rc);
        urb->function_privdata = NULL;
        usbd_free_urb (urb);
        return rc;
    }
    
    TRACE_MSG0(MTP, "leaving mtp_start_xmit_ep0");
    return 0;
}


/*! mtp_recv_urb - process a received urb
 *
 * Return non-zero if urb was not disposed of.
 */
static int mtp_recv_urb (struct usbd_urb *urb, int rc)
{
    struct usbd_function_instance *function_instance = urb->function_privdata;
    struct mtp_private *mtp = function_instance->privdata;
    struct mtp_cubuf *cu = mtp->recv_urbs;

    TRACE_MSG2(MTP, "RECV URB urb=0x%x, rc: %d", urb, rc);
    if (rc!=0) return rc;

    TRACE_MSG1(MTP, "RECV URB length: %d", urb->actual_length);
    TRACE_MSG1(MTP, "mtp->recv_len: 0x%x", mtp->recv_len);
    
    if ( mtp->recv_len == MTP_CANCEL_IO_MARKER)
    {
        TRACE_MSG0(MTP, "RECV URB meets cancel IO");
        mtp->recv_len = 0;
    }

    mtp_cubuf_append(cu, urb);
    mtp->recv_len += urb->actual_length;
        
    /* data ready */
    TRACE_MSG1(MTP, "wake up waiting, recv_len=%d", mtp->recv_len);
    wake_up_interruptible(&mtp->recv_wait);

    /* start another one */
    TRACE_MSG1(MTP, "start another one? cubuf full is %d", mtp_cubuf_is_full(mtp->recv_urbs));
    if (!mtp_cubuf_is_full(mtp->recv_urbs))
    {
        TRACE_MSG0(MTP, "starting another one");
        return mtp_start_recv_urb(function_instance);
    }

    return 0;
}

/*!
 * @brief mtp_start_recv - queue a receive urb
 *
 * Returns non-zero if there is an error in the USB layer.
 * @param function_instance
 * @return int
 */
int mtp_start_recv_urb(struct usbd_function_instance *function_instance)
{
    struct usbd_urb *urb = NULL;
    struct mtp_private *mtp = function_instance->privdata;
    int count = mtp->readsize;
    struct mtp_cubuf *cu = mtp->recv_urbs;

    TRACE_MSG1(MTP,"enter mtp_start_recv_urb, cubuf full is %d", mtp_cubuf_is_full(cu));
    
    if (mtp_cubuf_is_full(cu))
    {
        /* no more room in urb buf; will start later by upper layer after data is read */
        TRACE_MSG0(MTP,"no more recv urbs");
        return 0;
    }
    
    RETURN_EINVAL_UNLESS((urb = usbd_alloc_urb (function_instance, BULK_OUT, count, mtp_recv_urb)));
    urb->function_privdata = function_instance;
    if (usbd_start_out_urb(urb) == 0)
    {
        TRACE_MSG0(MTP,"START RECV URB success");
        return 0;
    }

    TRACE_MSG0(MTP,"START RECV URB ERROR");
    urb->function_privdata = NULL;
    usbd_free_urb(urb);
    return -EINVAL;
}

/*! mtp_sent_urb - callback for completed BULK xmit URB
 *
 * Handles notification that an urb has been sent (successfully or otherwise).
 *
 * @param urb     Pointer to the urb that has been sent.
 * @param urb_rc  Result code from the send operation.
 *
 * @return non-zero for failure.
 */
int mtp_sent_urb(struct usbd_urb *urb, int urb_rc)
{
    struct usbd_function_instance *function_instance = urb->function_privdata;
    struct mtp_private *mtp = function_instance->privdata;
    
    TRACE_MSG3(MTP, "urb: %p urb_rc: %d length: %d", urb, urb_rc, urb->actual_length);

    urb->function_privdata = NULL;
    usbd_free_urb(urb);

    mtp->sent_urbs_count++;
    TRACE_MSG1(MTP, "in mtp_send_urb mtp->sent_urbs_count = %d.", mtp->sent_urbs_count);
    TRACE_MSG0(MTP, "wake up anyone waiting to write");
    /* wake up anyone waiting to write */
    wake_up_interruptible(&mtp->send_wait);
    
    TRACE_MSG0(MTP, "leaving mtp_sent_urb");
    return 0;
}
/*! mtp_start_xmit - start sending a buffer
 *
 * @param function_instance The function instance for this driver to use.
 * @param buffer The data to send.
 * @param len The data length.
 * @return: non-zero if there is an error in the USB layer.
 */
int mtp_start_xmit (struct usbd_function_instance *function_instance, u8 *buffer, int len)
{
    struct mtp_private *mtp = function_instance->privdata;
    struct usbd_urb *urb = NULL;
    int rc;
	int flags;

    TRACE_MSG1(MTP,"enter mtp_start_xmit, len=%d", len);

    /* we send all data in one urb - check if it fits */
    if (len > mtp->writesize)
    {
        TRACE_MSG0(MTP,"mtp_start_xmit send buf too large");
        return -ENOMEM;
    }

    if (mtp->send_urbs_count - mtp->sent_urbs_count>= MTP_SEND_URBS_MAX)
    {
        /* Caller needs to make sure has room and block wait if it does. So this should never happen. */
        TRACE_MSG0(MTP,"mtp_start_xmit send buf full");
        return -ENOMEM;
    }
    
    RETURN_EINVAL_UNLESS((urb = usbd_alloc_urb (function_instance, BULK_IN, len+1, mtp_sent_urb)));
    urb->function_privdata = function_instance;

    if (len == 0)
    {
        urb->flags |= USBD_URB_SENDZLP;
    }
    else
    {
        copy_from_user(urb->buffer, buffer, len);
    }
    
    urb->actual_length = len;

    TRACE_MSG1(MTP, "start sending urb len=%d", len);
	/*Need to lock the interrupts so that the URBs can be appended
	to the buffer*/
	local_irq_save (flags);
    if ((rc = usbd_start_in_urb (urb))) {
        TRACE_MSG1(MTP, "FAILED: %d", rc);
        urb->function_privdata = NULL;
        usbd_free_urb (urb);
		local_irq_restore (flags);
        return rc;
    }
    local_irq_restore (flags);
    TRACE_MSG0(MTP, "mtp_start_xmit appending cu");
    mtp->send_urbs_count++;
    
    TRACE_MSG0(MTP, "leaving mtp_start_xmit");
    return 0;
}

/*! mtp_send_intr - send an interrupt
 *
 * @param function_instance The function instance for this driver to use.
 * @param buffer The data to send.
 * @param len The data length.
 * @return: non-zero if there is an error in the USB layer.
 */
int mtp_send_intr (struct usbd_function_instance *function_instance, u8 *buffer, int len)
{
    struct mtp_private *mtp = function_instance->privdata;
    struct usbd_urb *urb = NULL;
    int count;
    int rc;

    TRACE_MSG1(MTP,"enter mtp_send_intr, len=%d", len);

    /* truncate data larger then allowed intr size */
    count = MIN(mtp->writesize, len);

    /* Callback functio is NULL - usbd will free the urb */
    RETURN_EINVAL_UNLESS((urb = usbd_alloc_urb (function_instance, INT_IN, count, NULL)));
    urb->function_privdata = function_instance;
    memcpy (urb->buffer, buffer, count);
    
    urb->actual_length = count;

    TRACE_MSG1(MTP, "start sending urb count=%d", count);
    if ((rc = usbd_start_in_urb (urb))) {
        TRACE_MSG1(MTP, "FAILED: %d", rc);
        urb->function_privdata = NULL;
        usbd_free_urb (urb);
        return rc;
    }
    
    TRACE_MSG0(MTP, "leaving mtp_send_intr");
    return 0;
}

/*! 
 * @brief mtp_function_enable - called by USB Device Core to enable the driver
 * @param function_instance The function instance for this driver to use.
 * @return non-zero if error.
 */
static int mtp_function_enable (struct usbd_function_instance *function_instance)
{
    struct mtp_private *mtp = NULL;
    int ctl_init_ret = -EINVAL;
    int dev_init_ret = -EINVAL;

    TRACE_MSG0(MTP, "ENABLED");
    RETURN_EINVAL_UNLESS((mtp = CKMALLOC(sizeof(struct mtp_private), GFP_KERNEL)));

    function_instance->privdata = mtp;
    mtp->function_instance = function_instance;

    THROW_IF(!(mtp->recv_urbs = mtp_cubuf_create(MTP_RECV_URBS_MAX + 1)), error);
    
    mtp->recv_pos = 0;
    mtp->recv_len = 0;
    init_waitqueue_head(&mtp->recv_wait);

    mtp->send_urbs_count = 0;
    mtp->sent_urbs_count = 0 ;
    init_waitqueue_head(&mtp->send_wait);

    mtp->req_ms_vendor_flag = 0;
    mtp->req_recv_buf = NULL;
    mtp->req_recv_start = mtp->req_recv_buf;
    mtp->req_recv_end = mtp->req_recv_buf;
    init_waitqueue_head(&mtp->req_recv_wait);

    mtp->req_send_len = 0;
    init_waitqueue_head(&mtp->req_send_wait);
    init_waitqueue_head(&suspend_wait);
    
    mtp->hs = 0;

    /* hotplug to start MTP app */
#ifdef CONFIG_OTG_MTP_HOTPLUG
    TRACE_MSG0(MTP, "hotplug init");
    mtp_gen_priv.function_instance = function_instance; 
    mtp_gen_priv.dev_name = mtp_if_name; 
    hotplug_init(&mtp_gen_priv);
#endif

    PREPARE_WORK_ITEM(mtp_suspend_bh_pri.bh, mtp_suspend_bh, &mtp_suspend_bh_pri);

    /* init /dev/mtp and /proc/mtpctl */    
    THROW_IF((ctl_init_ret = mtp_ctl_init(function_instance)), error);
    THROW_IF((dev_init_ret = mtp_dev_init(function_instance)), error);

    CATCH (error) {
        if (ctl_init_ret == 0)
        {
            mtp_ctl_exit();
        }

        if (mtp->recv_urbs)
        {
            mtp_cubuf_destroy(mtp->recv_urbs);
        }

        function_instance->privdata = NULL;
        LKFREE(mtp);
        return -EINVAL;
    }

    return 0;
}

/*! 
 * @brief mtp_function_disable - called by USB Device Core to disable the driver
 * @param function_instance The function instance for this driver to use.
o * @return non-zero if error.
 */
static void mtp_function_disable (struct usbd_function_instance *function_instance)
{
    struct mtp_private *mtp = function_instance->privdata;
    
    TRACE_MSG0(MTP, "DISABLED");
    RETURN_UNLESS(mtp);

    // Wake up mtp_suspend_bh waiting on suspend_wait 
    wake_up(&suspend_wait);
    
    /* hotplug stop */
#ifdef CONFIG_OTG_MTP_HOTPLUG
    while (PENDING_WORK_ITEM(mtp_gen_priv.hotplug_bh))
    { 
        TRACE_MSG0(MTP, "waiting for hotplug bh");
        schedule_timeout(10 * HZ); 
    }
#endif
    
    mtp_dev_exit();
    mtp_ctl_exit();

    mtp_cubuf_destroy(mtp->recv_urbs);
    mtp->recv_urbs = NULL;

    if (mtp->req_recv_buf != NULL)
    {
        LKFREE(mtp->req_recv_buf);
        mtp->req_recv_buf = NULL;
        mtp->req_recv_start = mtp->req_recv_buf;
        mtp->req_recv_end = mtp->req_recv_buf;
    }
    
    function_instance->privdata = NULL;
    LKFREE(mtp);
}

/*! 
 * @brief mtp_set_configuration - called to indicate set configuration
 * @param function_instance
 * @param configuration
 * @return int 
 */             
static int mtp_set_configuration (struct usbd_function_instance *function_instance, int configuration)
{               
    struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
    struct mtp_private *mtp = function_instance->privdata;
    
    TRACE_MSG0(MTP, "CONFIGURED");

#ifdef CONFIG_OTG_MTP_HOTPLUG
    TRACE_MSG0(MTP, "hotplug start");
    mtp_gen_priv.flags  = HOTPLUG_FUNC_ENABLED;
    generic_os_hotplug(&mtp_gen_priv); 
#endif
    
    /* endpoint buf size */
    mtp->hs = usbd_high_speed((struct usbd_function_instance *) function_instance);
    mtp->outsize = usbd_endpoint_wMaxPacketSize(function_instance, BULK_OUT, mtp->hs); 
    mtp->insize = usbd_endpoint_wMaxPacketSize(function_instance, BULK_IN, mtp->hs);
    TRACE_MSG3(MTP, "hs=%d, outsize: %d insize: %d", mtp->hs, mtp->outsize, mtp->insize); 

    /* URB transfer size */
    mtp->readsize = MTP_RECV_URB_MAX_SIZE;
    mtp->writesize = MTP_SEND_URB_MAX_SIZE;
    TRACE_MSG2(MTP, "readsize: %d writesize: %d", mtp->readsize, mtp->writesize); 

    mtp_start_recv_urb(function_instance);

    wake_up(&suspend_wait);

    return 0;
}       

/*!  
 * @brief mtp_reset - called to indicate reset
 * @param function_instance 
 * @return int  
 */ 
static int mtp_reset (struct usbd_function_instance *function_instance) 
{ 
    struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance; 
    struct mtp_private *mtp = function_instance->privdata; 
    unsigned long flags;
    
    TRACE_MSG0(MTP, "RESET"); 
    
#ifdef CONFIG_OTG_MTP_HOTPLUG
     local_irq_save(flags);
     if (mtp_gen_priv.flags != ~HOTPLUG_FUNC_ENABLED)
     {
       mtp_gen_priv.flags = ~HOTPLUG_FUNC_ENABLED;
       generic_os_hotplug(&mtp_gen_priv);
     }
     else
     {
       TRACE_MSG0(MTP,"mtp disable is ongoing: do not restart hotplug event\n");
     }
     local_irq_restore(flags);
#endif
    
    return 0; 
} 

/*!
 * @brief mtp_suspended - called to indicate suspend
 * @param function_instance
 * @return int
 */
static int mtp_suspended (struct usbd_function_instance *function_instance)
{
    SET_WORK_ARG(mtp_suspend_bh_pri.bh, &mtp_suspend_bh_pri);
#ifdef LINUX26
    queue_work(mtp_fd_workqueue, &(mtp_suspend_bh_pri.bh));
#else
    SCHEDULE_WORK(mtp_suspend_bh_pri.bh);
#endif
    return 0;
}

/*!
 * @brief mtp_suspend_bh - handling suspend
 * @param  data
 * @return void
 */
void mtp_suspend_bh (void *data)
{
    struct mtp_suspend_bh_private *  mtp_suspend_bh_pri = (struct mtp_suspend_bh_private *) data;
    RETURN_UNLESS(mtp_suspend_bh_pri);

    interruptible_sleep_on_timeout(&suspend_wait, 2*HZ);
}

/*!
 * @brief mtp_resumed - called to indicate resume
 * @param function_instance
 * @return int
 */
static int mtp_resumed(struct usbd_function_instance *function_instance)
{
    return 0;
}


/*!  
 * @brief mtp_device_request_bh - BH for handling device request
 * @param arg
 * @return void
 */ 
void mtp_device_request_bh (void *arg)
{
    struct mtp_dev_req_bh_private *bh_priv = arg;
    struct mtp_private *mtp;

    TRACE_MSG1(MTP, "mtp_device_request_bh: arg=%x", arg);

    mtp= bh_priv->function_instance->privdata;
    RETURN_UNLESS(mtp);
    
    TRACE_MSG3(MTP, "mtp_device_request_bh: mtp=%x, data_len=%d, pending_data=%d", mtp, bh_priv->data_len, bh_priv->pending_data);

    mtp_dev_req_put(mtp, (unsigned char*) bh_priv->data, bh_priv->data_len);

    if (bh_priv->pending_data == FALSE)
    {
        /* no data to receive for the req, pass it to the upper layer now.
         * otherwise the recv data will wake up upper layer.
         */
        wake_up_interruptible(&mtp->req_recv_wait);
    }
    else
    {
        TRACE_MSG1(MTP, "pending_ep0_data_len=%x", pending_ep0_data_len);
        if (pending_ep0_data)
        {
            /* ep0 data arrived before the BH for dev_req */
            mtp_dev_req_put(mtp, pending_ep0_data, pending_ep0_data_len);
            LKFREE(pending_ep0_data);
            pending_ep0_data = NULL;
            pending_ep0_data_len = 0;
            
            wake_up_interruptible(&mtp->req_recv_wait);
        }
        else
        {
            ready_for_ep0_data = TRUE;
        }
    }

    TRACE_MSG0(MTP, "going to free bh_priv->data");
    LKFREE(bh_priv->data);

    TRACE_MSG0(MTP, "going to free bh_priv");
    LKFREE(bh_priv);

    TRACE_MSG0(MTP, "mtp_device_request_bh return");
}

/*!  
 * @brief mtp_str2descriptor - convert string to descriptor
 * @param buf
 * @param buf_len
 * @param str
 * @return int  
 */ 
static void *mtp_str2descriptor(u8 *buf, int buf_len, const char *str)
{
    int ext_len = strlen(str)/2;
    u8 val;
    int i;
    int j;

    if (ext_len > buf_len)
    {
        ext_len = buf_len;
    }
        
    TRACE_MSG2(MTP, "mtp_str2descriptor, ext_len=%d, str=%s", ext_len, str);

    for (i = 0; i < ext_len; i++)
    {
        val = 0;
        for (j = 2*i; j <= 2*i+1; j++)
        {
            val <<= 4;
            val += (str[j] >= 'A') ? str[j] - 'A' + 10 : str[j] - '0';
        }
        
        buf[i] = val;
        /* TRACE_MSG2(MTP, "mtp_str2descriptor, desc[%d]=0x%x", i, buf[i]); */
    }

    return buf;
}

/*!  
 * @brief mtp_device_request - called for device request
 * @param function_instance 
 * @return int  
 */ 
static int mtp_device_request (struct usbd_function_instance *function_instance, struct usbd_device_request *request)
{
    struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance; 
    struct mtp_private *mtp = function_instance->privdata; 
    struct usbd_urb *urb;
    int count;
    int buf_avail;
    u8 bRequest = request->bRequest;
    u8 bmRequestType = request->bmRequestType;
    u16 wValue = le16_to_cpu(request->wValue);
    u16 wIndex = le16_to_cpu(request->wIndex);
    u16 wLength = le16_to_cpu(request->wLength);
    int length;
    struct mtp_ctl_msg_header msg;
    unsigned char* msg_data = NULL;
    struct mtp_dev_req_bh_private *bh_priv = NULL;
    BOOL has_data = FALSE;
    u8 mtp_extended_config_descriptor[MTP_EXT_DESC_MAX_LEN];
   
    TRACE_MSG6(MTP, "MTP RECV SETUP bmRequestType:%02x bRequest:%02x wValue:%04x wIndex:%04x wLength:%04x, interface=%s",
               bmRequestType, bRequest, wValue, wIndex, wLength, function_instance->name);

    /* verify that this is a usb class request or a vendor request. */
    RETURN_ZERO_IF (!(request->bmRequestType & (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_VENDOR)));


    /* Handle req for MS Extended configuration descriptor 
	This is called when a Vendor COV is called*/
    if ((request->bmRequestType & USB_REQ_TYPE_MASK)==USB_REQ_TYPE_VENDOR && ((!strcmp(function_instance->name, "mtp-if")) 
        || (!strcmp(function_instance->name, "pbg-if")))) 
    { 
        if (request->bRequest==USB_MOD_VENDOR_CODE && request->wIndex == mtp_ext_id && mtp_ext_desc) 
        { 
            TRACE_MSG1(MTP, "device request vendor req=%d", request->wIndex);
            mtp_str2descriptor(mtp_extended_config_descriptor, MTP_EXT_DESC_MAX_LEN, mtp_ext_desc);
            
            /* Get extended configuration descriptor */
            if (request->wLength<mtp_extended_config_descriptor[0]) 
                length=request->wLength; 
            else 
                length=mtp_extended_config_descriptor[0];
            
            urb = usbd_alloc_urb_ep0(function_instance, length, NULL); 
            if (!urb) 
            { 
                TRACE_MSG0(MTP, "leave mtp_device_request, usbd_alloc_urb_ep0 fail");
                return -EINVAL; 
            }
            
            TRACE_MSG1(MTP, "EP0 size=%d", usbd_endpoint_zero_wMaxPacketSize(function_instance, (int) mtp->hs));
            if ((0 == length) || !(length % usbd_endpoint_zero_wMaxPacketSize(function_instance, (int) mtp->hs))) 
            { 
                urb->flags = USBD_URB_SENDZLP; 
            } 
            memcpy(urb->buffer, mtp_extended_config_descriptor, length); 
            urb->actual_length = length; 
            if (usbd_start_in_urb(urb)) 
            { 
                usbd_free_urb(urb);                       
                TRACE_MSG0(MTP, "LEAVE ... usbd_send_urb fail"); 
                return -EINVAL; 
            }

            /* we've served MTP vendor req */
            mtp->req_ms_vendor_flag = 1;
            
            TRACE_MSG0(MTP, "LEAVE ... send vendor request success"); 
            return 0; 
        } 
        else  
        { 
            TRACE_MSG0(MTP, "LEAVE ... Unknown vendor-specific request."); 
            return -EINVAL; 
        } 
    }
        
    /* alloc memory if it is the first use. */ 
    if (mtp->req_recv_buf == NULL)
    {
        RETURN_ENOMEM_UNLESS((mtp->req_recv_buf = CKMALLOC(MTP_EP0_RECV_MAX_SIZE, GFP_KERNEL)));
        mtp->req_recv_start = mtp->req_recv_buf;
        mtp->req_recv_end = mtp->req_recv_buf;
    }

    /* prepare msg that needs to pass along to the upper layer */
    count = MTP_CTL_MSG_HEADER_SIZE + sizeof(struct usbd_device_request);

    /* if it is from host to device, also need space for data, if any */
    if ((request->bmRequestType & USB_REQ_DIRECTION_MASK) == USB_REQ_HOST2DEVICE && wLength > 0)
    {
        count += wLength;
        has_data = TRUE;
    }

    buf_avail = mtp_dev_req_buf_avail(mtp);
    
    TRACE_MSG2(MTP, "buf avail=%d, count=%d", buf_avail, count); 
    if (count > buf_avail)
    {
        TRACE_MSG0(MTP, "LEAVE ... device request overflow"); 
        return -ENOMEM;
    }

    RETURN_ENOMEM_UNLESS ((msg_data = CKMALLOC(MTP_CTL_MSG_HEADER_SIZE + sizeof(struct usbd_device_request), GFP_KERNEL)));
    bh_priv = CKMALLOC(sizeof(struct mtp_dev_req_bh_private), GFP_KERNEL);
    if (bh_priv == NULL)
    {
        LKFREE(msg_data);
        return -ENOMEM;
    }

    msg.msg_id = MTP_CTL_CLASS_REQ;
    msg.msg_len =  count;
    memcpy(msg_data, &msg, MTP_CTL_MSG_HEADER_SIZE);
    memcpy(msg_data + MTP_CTL_MSG_HEADER_SIZE, (unsigned char*) request, sizeof(struct usbd_device_request));

    TRACE_MSG0(MTP, "dev req schedule work");
    bh_priv->function_instance = function_instance;
    bh_priv->data = msg_data;
    bh_priv->data_len = MTP_CTL_MSG_HEADER_SIZE + sizeof(struct usbd_device_request);
    bh_priv->pending_data = has_data;
    
    /* The host could send multiple dev reqs one after another. The dev req is handled in intr and recv data in BH.
     * So to get the dev_req->data->dev_req in the right sequence, we need to process dev req in BH too.
     */
    PREPARE_WORK_ITEM(bh_priv->bh, mtp_device_request_bh, bh_priv);
    SET_WORK_ARG(bh_priv->bh, bh_priv);
#ifdef LINUX26
    queue_work(mtp_fd_workqueue, &(bh_priv->bh));
#else
    SCHEDULE_WORK(bh_priv->bh);
#endif

    TRACE_MSG0(MTP, "dev req work scheduled");

    /* if it is from host to device, also need to read data if wLength > 0 */
    if (has_data)
    {
        /* read data that follows the request */
        TRACE_MSG0(MTP,"START RECV EP0 data");
        RETURN_ENOMEM_UNLESS((urb = usbd_alloc_urb_ep0(function_instance, wLength, mtp_recv_urb_ep0)));
        urb->function_privdata = function_instance;
        RETURN_ZERO_UNLESS(usbd_start_out_urb  (urb));
        TRACE_MSG0(MTP,"START RECV URB EP0 ERROR");
        urb->function_privdata = NULL;
        usbd_free_urb(urb);
        return -EINVAL;
    }

    return 0;
} 

/*!  
 * @brief mtp_get_descriptor - called for descriptor request
 * @param function_instance 
 * @param descriptor_type
 * @param descriptor_index
 * @return void*
 */ 
static void *mtp_get_descriptor(struct usbd_function_instance *function_instance, int descriptor_type, int descriptor_index)
{
    int ext_len;
    u8 *str_desc = NULL;

    TRACE_MSG2(MTP, "mtp_get_descriptor type=0x%x, index=0x%x", descriptor_type, descriptor_index);
    RETURN_NULL_UNLESS(descriptor_type == USB_DT_STRING);
    
    if (mtp_ext_str_idx == descriptor_index && mtp_ext_str_desc && strlen(mtp_ext_str_desc))
    {
        str_desc = mtp_extended_string_descriptor;
        mtp_str2descriptor(str_desc, MTP_EXT_STR_MAX_LEN, mtp_ext_str_desc);
    }
    
    return str_desc;
}

/*! mtp_function_ops 
 */
struct usbd_function_operations mtp_function_ops = {
    .function_enable = mtp_function_enable,
    .function_disable = mtp_function_disable,
    .set_configuration = mtp_set_configuration,
    .reset = mtp_reset,
    .device_request = mtp_device_request,
    .get_descriptor = mtp_get_descriptor,
    .suspended = mtp_suspended,
    .resumed = mtp_resumed,
};

