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
 * 03/28/2006        Motorola         MTP initial
 * 04/26/2006        Motorola         fix MTP ctl message header
 * 06/06/2006        Motorola         PictBridge support
 * 09/18/2006        Motorola         Cancel sync from device to host
 *
 * 11/30/2006        Motorola         fix shared recv_len
 * 02/12/2007        Motorola         fixed IOCTL reset functionality to clear URBs
 *
*/

/*!
 *
 * @file otg/functions/mtp/mtp-io.c
 * @brief MTP device
 *
 * This implements the MTP device.
 *
 * @ingroup MTPFunction
 *
 */


#include <otg/otg-compat.h>
#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <otg/otg-linux.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/random.h>
#include <linux/slab.h>

#include <linux/bio.h>
#include <linux/buffer_head.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <public/otg-node.h>
#include <otg/hotplug.h> 

#include "mtp.h"
#include "mtp-io.h"

int mtp_start_recv_urb(struct usbd_function_instance *function_instance);
int mtp_start_xmit (struct usbd_function_instance *function_instance, u8 *buffer, int len);
int mtp_send_intr (struct usbd_function_instance *function_instance, u8 *buffer, int len);
int mtp_start_xmit_ep0 (struct usbd_function_instance *function_instance, u8 *buffer, int len);
int mtp_dev_req_get(struct mtp_private *mtp, unsigned char *data, int data_len);

static struct cdev mtp_dev;
static dev_t mtp_devno;
static struct usbd_function_instance *mtp_dev_function_instance = NULL;
static struct usbd_function_instance *proc_function_instance = NULL;
static void mtp_empty_cubuf(struct mtp_private *mtp);

static void mtp_empty_cubuf(struct mtp_private *mtp)
{
    struct mtp_cubuf *cu;
    cu = mtp->recv_urbs;
	if(cu == NULL)
	{
		TRACE_MSG0(MTP, "No bufffers available to remove");
		return;
	}
    TRACE_MSG0(MTP, "Entering mtp_empty_cubuf");
    while (mtp->recv_len > 0)
    {
        struct usbd_urb *urb = mtp_cubuf_get(cu);
		if(urb != NULL)
		{
             TRACE_MSG3(MTP, "mtp->recv_len=%d, mtp->recv_pos = %d, urb->actual_length = %d", mtp->recv_len,  mtp->recv_pos, urb->actual_length );
             usbd_free_urb(urb);
             mtp_cubuf_remove(cu);
             mtp->recv_len -= (urb->actual_length - mtp->recv_pos);
             /*Need to make sure we are at the beginning of the URB*/
			 mtp->recv_pos = 0 ;

		}
		else
		{
			TRACE_MSG0(MTP, "URB was NULL");
			break;
		}
    }
}
static ssize_t mtp_dev_read (struct file *file, char *buf, size_t count, loff_t * pos)
{
    struct mtp_private *mtp = NULL;
    int bytes_read = 0;
    bool start_next = FALSE;
    struct mtp_cubuf *cu;
    unsigned long flags;
    bool end_trans = FALSE;

    TRACE_MSG1(MTP, "mtp_dev_read, count=%d", count);
    
    if (mtp_dev_function_instance == NULL)
    {
        /* Error: function driver not ready */
        TRACE_MSG0(MTP, "function instance NULL");
        return -EINVAL;
    }

    mtp = mtp_dev_function_instance->privdata;
    if (mtp == NULL)
    {
        /* Error: function driver not ready */
        TRACE_MSG0(MTP, "function instance privdata NULL");
        return -EINVAL;
    }

    TRACE_MSG1(MTP, "recv_len=0x%x", mtp->recv_len);
    if (mtp->recv_len == MTP_CANCEL_IO_MARKER)
    { 
        TRACE_MSG0(MTP, "dev read meets cancel IO");
        mtp->recv_len = 0;
    }

    if (mtp->recv_len == 0)
    {
        /* no data yet, block wait for data */
        TRACE_MSG0(MTP, "dev read SLEEPING");
        wait_event_interruptible(mtp->recv_wait, (mtp->recv_len > 0));
        TRACE_MSG1(MTP, "dev read AWAKE, recv_len=%d", mtp->recv_len);

        /* if we were woke up by CancelIO or device reset */
        if (mtp->recv_len == MTP_CANCEL_IO_MARKER)
        {
            mtp->recv_len = 0;
            TRACE_MSG0(MTP, "read interrupted");
            return -EINTR;
        }
    }

    /* copy data out */
    cu = mtp->recv_urbs;
    while (mtp->recv_len > 0 && bytes_read < count && !end_trans)
    {
        struct usbd_urb *urb = mtp_cubuf_get(cu);
        int len;

        TRACE_MSG2(MTP, "got urb %x, actual_length=%d", urb, urb->actual_length);

        if (urb->actual_length < mtp->readsize)
        {
            /* End of a transfer if not a full size urb. We return after the read. This only happens when host sends two small
             * transfers (code followed by data).
             * It host sends us ZLP, it is also handled here.
             */
            end_trans = TRUE;
        }
        
        len = MIN(count - bytes_read, urb->actual_length - mtp->recv_pos);        
        TRACE_MSG4(MTP, "copy to user bytes_read=%d, recv_pos=%d, len=%d, recv_len=%d", bytes_read, mtp->recv_pos, len, mtp->recv_len);
        copy_to_user(buf + bytes_read, urb->buffer + mtp->recv_pos, len);
        TRACE_MSG0(MTP, "done copy to user");
        bytes_read += len;

        local_irq_save(flags);
        mtp->recv_len -= len;
        local_irq_restore(flags);
        
        if (len == urb->actual_length - mtp->recv_pos)
        {
            TRACE_MSG0(MTP, "done with this urb");
            /* all data in this urb read by user */
            urb->function_privdata = NULL;
            usbd_free_urb(urb);

            /* if the circular buf was full, we also need to start another recv */
            local_irq_save(flags);
            if (mtp_cubuf_is_full(cu))
            {
                start_next = TRUE;
            }
            
            mtp_cubuf_remove(cu);
            mtp->recv_pos = 0;

            if (start_next)
            {
                TRACE_MSG0(MTP, "mtp_dev_read start recv");                
                start_next = FALSE;
                mtp_start_recv_urb(mtp_dev_function_instance);
            }    

            local_irq_restore(flags);
        }
        else
        {
            TRACE_MSG0(MTP, "NOT done with this urb");
            /* more pointer in the urb for the next read */
            mtp->recv_pos += len;
        }

        /* if we have no more data but read wants more, we block wait for more data to come */
        if (mtp->recv_len == 0 && bytes_read < count)
        {
            /* no data yet, block wait for data */
            TRACE_MSG0(MTP, "dev read SLEEPING again");
            wait_event_interruptible(mtp->recv_wait, (mtp->recv_len > 0));
            TRACE_MSG1(MTP, "dev read AWAKE again, recv_len=%d", mtp->recv_len);

            /* if we were woke up by CancelIO or device reset */
            if (mtp->recv_len == MTP_CANCEL_IO_MARKER)
            {
                mtp->recv_len = 0;
                TRACE_MSG0(MTP, "read interrupted");
                return -EINTR;
            }
        }
    }    

    TRACE_MSG1(MTP, "mtp_dev_read read %d", bytes_read);
    return bytes_read;
}

static ssize_t mtp_dev_write (struct file *file, const char *buf, size_t count, loff_t * pos)
{
    struct mtp_private *mtp = NULL;

    TRACE_MSG1(MTP, "mtp_dev_write count=%d", count);

    if (mtp_dev_function_instance == NULL)
    {
        /* Error: function driver not ready */
        TRACE_MSG0(MTP, "function instance NULL");
        return -EINVAL;
    }

    mtp = mtp_dev_function_instance->privdata;
    if (mtp == NULL)
    {
        /* Error: missing privdata not ready */
        TRACE_MSG0(MTP, "privdata NULL");
        return -EINVAL;
    }

    if (count > mtp->writesize)
    {
        /* Error: write size too large */
        TRACE_MSG0(MTP, "write too large");
        return -ENOMEM;
    }
    
    TRACE_MSG1(MTP, "mtp_dev_write send_urbs_count=%d", mtp->send_urbs_count);
    TRACE_MSG1(MTP, "mtp_dev_write sent_urbs_count=%d", mtp->sent_urbs_count);
    if ((mtp->send_urbs_count - mtp->sent_urbs_count) >= MTP_SEND_URBS_MAX)
    {
        /* all urbs used, block wait */
        TRACE_MSG0(MTP, "dev write SLEEPING");
        wait_event_interruptible(mtp->send_wait, ((mtp->send_urbs_count - mtp->sent_urbs_count) < MTP_SEND_URBS_MAX));
        TRACE_MSG0(MTP, "dev write AWAKE");
        if (mtp->recv_len == MTP_CANCEL_IO_MARKER)
        {
            TRACE_MSG0(MTP, "dev write meets cancel IO");
            mtp->recv_len = 0 ;
            return -EINTR ;
        }
    }
    
    mtp_start_xmit(mtp_dev_function_instance, (u8 *)buf, count);    
    return count;
}

static int mtp_dev_fsync(struct file *file, struct dentry *dentry, int datasync)
{
    struct mtp_private *mtp = NULL;

    TRACE_MSG1(MTP, "mtp_dev_fsync file=%x", file);

    if (file == NULL)
    {
        return -EINVAL;
    }

    if (mtp_dev_function_instance == NULL)
    {
        /* Error: function driver not ready */
        TRACE_MSG0(MTP, "function instance NULL");
        return -EINVAL;
    }

    mtp = mtp_dev_function_instance->privdata;
    if (mtp == NULL)
    {
        /* Error: missing privdata not ready */
        TRACE_MSG0(MTP, "privdata NULL");
        return -EINVAL;
    }

    TRACE_MSG1(MTP, "mtp->send_urbs_count = %d", mtp->send_urbs_count);
    while ((mtp->send_urbs_count - mtp->sent_urbs_count) > 0)
    {
        /* send not done yet, block wait */
        TRACE_MSG0(MTP, "dev fsync SLEEPING");
        interruptible_sleep_on_timeout(&mtp->send_wait,1);
        TRACE_MSG0(MTP, "dev fsync AWAKE");
    }

    return 0;
}

static int mtp_dev_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    struct mtp_private *mtp = NULL;
    struct mtp_event_data event;
    int len;
    int clen;
    
    TRACE_MSG1(MTP, "mtp_dev_ioctl file=%x", file);

    if (file == NULL || mtp_dev_function_instance == NULL)
    {
        /* Error: function driver not ready */
        TRACE_MSG2(MTP, "function driver not ready: file=0x%x, function instance 0x%x", file, mtp_dev_function_instance);
        return -EINVAL;
    }

    mtp = mtp_dev_function_instance->privdata;
    if (mtp == NULL)
    {
        /* Error: missing privdata not ready */
        TRACE_MSG0(MTP, "privdata NULL");
        return -EINVAL;
    }

    switch (cmd)
    {
        case MTP_IOC_EVENT:
            TRACE_MSG1(MTP, "copy from user %d", MIN(_IOC_SIZE(cmd), MTP_EVENT_SIZE));
            RETURN_EINVAL_IF(copy_from_user(event.data, (void *)arg, MIN(_IOC_SIZE(cmd), MTP_EVENT_SIZE)));

            /* length is in little endian */
            memcpy(&len, event.data, sizeof(len));
            TRACE_MSG1(MTP, "len=%d", len);
            clen = le32_to_cpu(len);
            TRACE_MSG1(MTP, "cpu len=%d", clen);

            /* send event through interrupt in */
            mtp_send_intr(mtp_dev_function_instance, event.data, clen);
            break;

        case MTP_IOC_SEND_ZLP:
            /* fsync before ZLP */
            mtp_dev_fsync(file, NULL, 0);
            mtp_start_xmit(mtp_dev_function_instance, NULL, 0);
            break;

        case MTP_IOC_GET_EP_SIZE_IN:
            /* get endpoint buffer size for bulk in */
            RETURN_EINVAL_IF (copy_to_user((void *)arg, &mtp->insize, sizeof(int)));
            break;

        case MTP_IOC_GET_VENDOR_FLAG:
            /* get flag whether vender request has served */
            RETURN_EINVAL_IF (copy_to_user((void *)arg, &mtp->req_ms_vendor_flag, sizeof(int)));
            break;

        case MTP_IOC_CANCEL_IO:
        case MTP_IOC_DEVICE_RESET:
            TRACE_MSG1(MTP, "cancel IO or reset, recv_len=%d", mtp->recv_len);
            /*If we recieve a Reset IOCTL clear all pending URBs*/
            if(cmd == MTP_IOC_DEVICE_RESET)
            {
	            TRACE_MSG0(MTP, "Flushing URBs due to Reset IOCTL\n");
	            mtp_empty_cubuf(mtp);
            }
			
			mtp->recv_len = MTP_CANCEL_IO_MARKER;
            
			TRACE_MSG0(MTP, "before cancel_urb for BULK_IN.\n");
            usbd_flush_endpoint_index(mtp_dev_function_instance, BULK_IN) ;

            TRACE_MSG0(MTP, "before cancel_urb for BULK_OUT.\n");
            usbd_flush_endpoint_index(mtp_dev_function_instance, BULK_OUT) ;
                        
            TRACE_MSG1(MTP, "in cancel mtp->sent_urbs_count = %d.",mtp->sent_urbs_count);
            TRACE_MSG1(MTP, "in cancel mtp->send_urbs_count = %d.",mtp->send_urbs_count);

            /* we've cancelled the recv urb, start new one */
            mtp_start_recv_urb(mtp_dev_function_instance);

            wake_up_interruptible(&mtp->recv_wait);
            break;
    }
    
    return 0;
}

static unsigned int mtp_dev_poll(struct file *file, poll_table *wait)
{
    struct mtp_private *mtp = NULL;
    struct mtp_event_data event;
    int mask = 0;

    TRACE_MSG2(MTP, "mtp_dev_ioctl file=%x, wait=%x", file, wait);

    if (file == NULL)
    {
        TRACE_MSG0(MTP, "file NULL");
        return mask;
    }

    if (mtp_dev_function_instance == NULL)
    {
        /* Error: function driver not ready */
        TRACE_MSG0(MTP, "function instance NULL");
        return mask;
    }

    mtp = mtp_dev_function_instance->privdata;
    if (mtp == NULL)
    {
        /* Error: missing privdata not ready */
        TRACE_MSG0(MTP, "privdata NULL");
        return mask;
    }

    if (wait != NULL)
    {
        poll_wait(file, &mtp->send_wait, wait);
        poll_wait(file, &mtp->recv_wait, wait);
    }

    if (mtp->recv_len > 0)
    {
        TRACE_MSG0(MTP, "poll readable");
        mask |= POLLIN | POLLRDNORM;  /* readable */
    }
    
    if (mtp->send_urbs_count - mtp->sent_urbs_count < MTP_SEND_URBS_MAX)
    {
        TRACE_MSG0(MTP, "poll writable");
        mask |= POLLOUT | POLLWRNORM; /* writable */
    }

    TRACE_MSG1(MTP, "return mask=%d", mask);
    return mask;
}

static struct file_operations mtp_dev_switch_functions = {
        read:mtp_dev_read,
        write:mtp_dev_write,
        fsync:mtp_dev_fsync,
        ioctl:mtp_dev_ioctl,
        poll:mtp_dev_poll,
};

/*
 * mtp_dev_init - initialize
 */
int mtp_dev_init(struct usbd_function_instance *function_instance)
{
    int result;
    
    TRACE_MSG0(MTP, "mtp_dev_init");
    
    mtp_dev_function_instance = function_instance;

    /* allocate device number */
    result= alloc_chrdev_region(&mtp_devno, 0, 1, MTP_DEV_NAME);
    if (result < 0)
    {
        TRACE_MSG0(MTP, "allocating device number failed");
        return -EINVAL;
    }

    /* add new device */
    TRACE_MSG0(MTP, "cdev_init");
    cdev_init(&mtp_dev, &mtp_dev_switch_functions);
    mtp_dev.owner = THIS_MODULE;
    mtp_dev.ops = &mtp_dev_switch_functions;
    TRACE_MSG0(MTP, "cdev_add");
    result = cdev_add(&mtp_dev, mtp_devno, 1);
    if (result)
    {
        TRACE_MSG0(MTP, "cdev_add failed");
        unregister_chrdev_region(mtp_devno, 1);
        return -EINVAL;
    }

    TRACE_MSG0(MTP, "mtp_dev_init success");
    return 0;
}

/* mtp_dev_exit - exit
 */
void mtp_dev_exit(void)
{
    TRACE_MSG0(MTP, "mtp_dev_exit");

    cdev_del(&mtp_dev);
    unregister_chrdev_region(mtp_devno, 1);
    mtp_dev_function_instance = NULL;
}


/* /proc/mtpctl ***************************************************** */

static ssize_t mtp_ctl_read (struct file *file, char *buf, size_t count, loff_t * pos)
{
    struct mtp_private *mtp = NULL;
    int bytes_read = 0;

    TRACE_MSG1(MTP, "count=%d", count);

    if (proc_function_instance == NULL)
    {
        /* Error: function driver not ready */
        TRACE_MSG0(MTP, "function instance NULL");
        return -EINVAL;
    }

    mtp = proc_function_instance->privdata;
    if (mtp->req_recv_start == mtp->req_recv_end)
    {
        /* no data yet, block wait for data */
        TRACE_MSG0(MTP, "ctl read SLEEPING");
        interruptible_sleep_on(&mtp->req_recv_wait);
        TRACE_MSG0(MTP, "ctl write AWAKE");
    }

    bytes_read = mtp_dev_req_get(mtp, buf, count);
    TRACE_MSG1(MTP, "read %d", bytes_read);

    return bytes_read;
}

static ssize_t mtp_ctl_write (struct file *file, const char *buf, size_t count, loff_t * pos)
{
    struct mtp_private *mtp = NULL;
    struct mtp_ctl_msg_header msg;

    TRACE_MSG1(MTP, "count=%d", count);

    if (count > MTP_EP0_SEND_MAX_SIZE || count < MTP_CTL_MSG_HEADER_SIZE)
    {
        /* Error: write size too large or too small */
        TRACE_MSG0(MTP, "write size invalid");
        return -ENOMEM;
    }
    
    if (proc_function_instance == NULL)
    {
        /* Error: function driver not ready */
        TRACE_MSG0(MTP, "function instance NULL");
        return -EINVAL;
    }

    mtp = proc_function_instance->privdata;
    if (mtp == NULL)
    {
        /* Error: missing privdata not ready */
        TRACE_MSG0(MTP, "privdata NULL");
        return -EINVAL;
    }

    /* msg info */
    copy_from_user(&msg, buf, MTP_CTL_MSG_HEADER_SIZE);
    TRACE_MSG2(MTP, "ctl write msg len = %d, msg id = %d", msg.msg_len, msg.msg_id);
    if (msg.msg_id != MTP_CTL_CLASS_REPLY)
    {
        /* unsupported */
        TRACE_MSG1(MTP, "unsupported ctl msg id %d", msg.msg_id);
        return -EINVAL;
    }
    
    TRACE_MSG1(MTP, "mtp->req_send_len = %d", mtp->req_send_len);
    if (mtp->req_send_len != 0)
    {
        /* previous send not done yet, block wait */
        TRACE_MSG0(MTP, "ctl write SLEEPING");
        interruptible_sleep_on(&mtp->req_send_wait);
        TRACE_MSG0(MTP, "ctl write AWAKE");
    }

    /* sending the data */
    mtp->req_send_len = count - MTP_CTL_MSG_HEADER_SIZE;
    mtp_start_xmit_ep0(proc_function_instance, (u8 *)buf + MTP_CTL_MSG_HEADER_SIZE, mtp->req_send_len);
    return count;
}

static struct file_operations mtp_ctl_switch_functions = {
        read:mtp_ctl_read,
        write:mtp_ctl_write,
};

/* mtp_ctl_init - initialize
 */
int mtp_ctl_init(struct usbd_function_instance *function_instance)
{
    struct proc_dir_entry *message = NULL;
    
    TRACE_MSG0(MTP, "mtp_ctl_init");
    
    THROW_IF (!(message = create_proc_entry (MTP_CTL_PROC_NAME, 0666, 0)), error);
    message->proc_fops = &mtp_ctl_switch_functions;
    CATCH(error) {
        printk(KERN_ERR "%s: creating /proc/mtpctl failed\n", __FUNCTION__);
        if (message)
            remove_proc_entry(MTP_CTL_PROC_NAME, NULL);
        return -EINVAL;
    }
    
    proc_function_instance = function_instance;
    return 0;
}


/* mtp_ctl_exit - exit
 */
void mtp_ctl_exit(void)
{
    TRACE_MSG0(MTP, "mtp_ctl_exit");
    remove_proc_entry(MTP_CTL_PROC_NAME, NULL);
    proc_function_instance = NULL;
}

