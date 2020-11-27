/*
 * otg/function/msc/msc-io-l24.c - MSC IO
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@lbelcarra.com>, 
 *      Tony Tang <tt@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2007 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 05/11/2005         Motorola         Initial distribution 
 * 12/30/2005         Motorola         Updates and improvements 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 07/26/2007         Motorola         Changes for Open src compliance.
 * 08/24/2007         Motorola         Changes for Open src compliance.
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
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


#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/devfs_fs_kernel.h>

#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <public/otg-node.h>
#include "msc-scsi.h"
#include "msc.h"
#include "msc-fd.h"
#include "crc.h"
#include "msc-io.h"

extern void msc_open_blockdev (struct usbd_function_instance *function_instance, int lunNumber);
extern void msc_close_blockdev (struct usbd_function_instance *function_instance, int lunNumber);


#define DEVICE_MOUNTED   0x0001		//DEVICE_MOUNTED
#define DEVICE_UNMOUNTED 0x0002		//DEVICE_UNMOUNTED


extern struct usbd_function_instance *proc_function_instance;

/*! msc_io_ioctl_internal
 *
 * XXX Can we get function_instance passed in as arg??? XXX
 */
int msc_io_ioctl_internal(struct inode *inode, unsigned int cmd, unsigned long arg)
{
        struct usbd_function_instance *function_instance = proc_function_instance; // XXX
        struct msc_private *msc = function_instance ? function_instance->privdata : NULL;
        int i;
        int len;
        int flag;

        static char func_buf[32];
        struct otgmsc_mount mount;
	unsigned int usr_int;
	unsigned long flags;

        TRACE_MSG2(MSC, "cmd: 0x%x connect: %d", cmd, msc->connected);

        memset(&mount, 0, sizeof(mount));
        switch (cmd) {

                /* Mount - make the specified major/minof device available for use
                 * by the USB Host, this does not require an active connection.
                 */
	case OTGMSC_START:

		// Ensure you still have enough space to mount this device
		RETURN_EINVAL_UNLESS (msc_curluns + 1 <= msc_maxluns);
		// Move the pointer to the currect spot.
		msc += msc_curluns;

		TRACE_MSG1(MSC, "Mounting device %d", msc_curluns);
		RETURN_EINVAL_IF(copy_from_user(&mount, (void *)arg, _IOC_SIZE(cmd)));
                
		TRACE_MSG3(MSC, "major=%d minor=%d state=%d", mount.major, mount.minor, msc->block_dev_state);
		msc->major = mount.major;
		msc->minor = mount.minor;
		
		//msc->io_state = MSC_INACTIVE;
		inode->i_rdev=MKDEV(msc->major, msc->minor);
		msc_open_blockdev (function_instance, msc_curluns);
		msc_curluns++;    // Increment the number of LUNS
                RETURN_EAGAIN_UNLESS(msc->command_state == MSC_READY);

		mount.status = (msc->block_dev_state == DEVICE_INSERTED) ? DEVICE_MOUNTED : DEVICE_UNMOUNTED;

		TRACE_MSG1(MSC, "Device is mounted status: %d", mount.status);

                // XXX Need to copy result back to user space
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
		TRACE_MSG0(MSC,  "Device mounted");
		return 0; 

                /* Umount - make the currently mounted device unavailable to the USB Host,
                 * if there is pending block i/o block until it has finished.
                 * Note that if the driver is unloaded the waiting ioctl process 
                 * must be woken up and informed with error code.
                 */
	case OTGMSC_STOP:
                // Ensure we have mounted a device and then set the msc pointer appropriately.
		RETURN_EINVAL_IF ( msc_curluns == 0);
		msc = msc + (msc_curluns -1 );
		
		TRACE_MSG1(MSC,  "Unmounting device %d", (msc_curluns - 1));
		RETURN_EINVAL_IF (copy_from_user (&mount, (void *)arg, _IOC_SIZE(cmd)));
                RETURN_EAGAIN_UNLESS(msc->command_state == MSC_INACTIVE);

		// Set results.
		msc->major = mount.major;
		msc->minor = mount.minor;
		msc_close_blockdev(function_instance, msc_curluns - 1);
		mount.status = DEVICE_UNMOUNTED;
		msc_curluns--;   // Decrement the number of luns.

		// Copy results back to user
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
		TRACE_MSG0(MSC,  "Device unmounted");
		return 0;


                /* Status - return the current mount status.
                 */
        case OTGMSC_STATUS:
		TRACE_MSG0(MSC, "Mount status");
                RETURN_EINVAL_IF (copy_from_user (&mount, (void *)arg, _IOC_SIZE(cmd)));
		if (msc->block_dev_state == DEVICE_EJECTED)
			mount.status = DEVICE_UNMOUNTED;
		else
			mount.status = DEVICE_MOUNTED;

                // XXX Need to copy result back to user space
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
		return 0;
                
                /* Wait_Connect - if not already connected wait until connected,
                 * Note that if the driver is unloaded the waiting ioctl process 
                 * must be woken up and informed with error code.
                 */
        case OTGMSC_WAIT_CONNECT:

		// Wait for connect.
		TRACE_MSG1(MSC, "Wait for connect: connected: %d", msc->connected);
		wait_event_interruptible( msc->ioctl_wq, (msc->close_pending || msc->connected ));
		if (msc->close_pending) return -ECONNABORTED ;

		RETURN_EINVAL_IF (copy_from_user (&mount, (void *)arg, _IOC_SIZE(cmd)));
		RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
		TRACE_MSG0(MSC,  "Device connected");
		return 0;
                
                /* Wait_DisConnect - if not already disconnected wait until disconnected,
                 * Note that if the driver is unloaded the waiting ioctl process 
                 * must be woken up and informed with error code.
                 */
	case OTGMSC_WAIT_DISCONNECT:
	  
		// Wait for disconnect.
		TRACE_MSG1(MSC, "Wait for disconnect: connected: %d", msc->connected);
		wait_event_interruptible( msc->ioctl_wq, (msc->close_pending || !msc->connected ));
		if (msc->close_pending) return -ECONNABORTED ;

		// Inform user about disconnect
		RETURN_EINVAL_IF (copy_from_user (&mount, (void *)arg, _IOC_SIZE(cmd)));
		RETURN_EINVAL_IF (copy_to_user((void *)arg, &mount, sizeof(mount)));
		TRACE_MSG0(MSC,  "Device disconnected");
		return 0; 

		/* Get_Unkown_Cmd - if an unknown command is recieved, we have to deal with that,
		 *  therefore we send the command to user space to allow an app to handle.
		 */
	case OTGMSC_GET_UNKNOWN_CMD:
		TRACE_MSG0(MSC,  "Getting Command Unknown");

		// Get the command forwhich the user wants unknown commands.
		ZERO(usr_int);		
		RETURN_EINVAL_IF (copy_from_user (&usr_int, (void *)arg, 4));  // 4 bytes b/c that is the maxlun.
		RETURN_EINVAL_IF ( usr_int > (msc_curluns - 1) );
		msc += usr_int;
		msc->unknown_cmd_ioctl_pending = 1;
		// Wait for an unknown command to come on 
		wait_event_interruptible( msc->ioctl_wq, (msc->close_pending || msc->unknown_cmd_pending) );
		if (msc->close_pending) return -ECONNABORTED ;
		// Copy results back to user
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &(msc->command), sizeof(msc->command)));

		// Clean up, and wake up function calling us so it can send a CSW.
		msc->unknown_cmd_ioctl_pending = 0;
		msc->unknown_cmd_pending = 0;
		wake_up_interruptible(&msc->ioctl_wq);

		return 0;


        default:
		TRACE_MSG1(MSC,  "Unknown command: %x", cmd);
		TRACE_MSG1(MSC,  "OTGMSC_START: %x", OTGMSC_START);
		TRACE_MSG1(MSC,  "OTGMSC_WRITEPROTECT: %x", OTGMSC_WRITEPROTECT);
		TRACE_MSG1(MSC,  "OTGMSC_STOP: %x", OTGMSC_STOP);
		TRACE_MSG1(MSC,  "OTGMSC_STATUS: %x", OTGMSC_STATUS);
		TRACE_MSG1(MSC,  "OTGMSC_WAIT_CONNECT: %x", OTGMSC_WAIT_CONNECT);
		TRACE_MSG1(MSC,  "OTGMSC_WAIT_DISCONNECT: %x", OTGMSC_WAIT_DISCONNECT);
		TRACE_MSG1(MSC,  "OTGMSC_GET_UNKNOWN_CMD: %x", OTGMSC_GET_UNKNOWN_CMD);
                return -EINVAL;
        }
        return 0;
}



/*! msc_io_ioctl
 */
int msc_io_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
        struct usbd_function_instance *function_instance = proc_function_instance; // XXX
        struct msc_private *msc = function_instance ? function_instance->privdata : NULL;
        int i;
        int len;
        int flag;
	
        RETURN_EFAULT_IF(msc == NULL);
        TRACE_MSG6(MSC, "cmd: %08x arg: %08x type: %02d nr: %02d dir: %02d size: %02d", 
                        cmd, arg, _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_DIR(cmd), _IOC_SIZE(cmd));
			

	if (msc->close_pending) return -ECONNREFUSED;
	RETURN_EINVAL_UNLESS (_IOC_TYPE(cmd) == OTGMSC_MAGIC);
        RETURN_EINVAL_UNLESS (_IOC_NR(cmd) <= OTGMSC_MAXNR);
        RETURN_EFAULT_IF((_IOC_DIR(cmd) == _IOC_READ) && !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd)));
        RETURN_EFAULT_IF((_IOC_DIR(cmd) == _IOC_WRITE) && !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd)));

        return msc_io_ioctl_internal(inode, cmd, arg);
}



int msc_io_proc_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
        struct usbd_function_instance *function_instance = proc_function_instance; // XXX
        struct msc_private *msc = function_instance ? function_instance->privdata : NULL;
        return msc_io_ioctl(inode, filp, cmd, arg);
}


static struct file_operations msc_io_proc_switch_functions = {
        owner: THIS_MODULE,
        llseek:NULL,
        read:NULL,
        aio_read:NULL,
        write:NULL,
        aio_write:NULL,
        readdir:NULL,
        poll:NULL,
        ioctl: msc_io_proc_ioctl,
        unlocked_ioctl:NULL,
        compat_ioctl:NULL,
        mmap:NULL,
        open:NULL,
        flush:NULL,
        release:NULL,
        fsync:NULL,
        aio_fsync:NULL,
        fasync:NULL,
        lock:NULL,
        readv:NULL,
        writev:NULL,
        sendfile:NULL,
        sendpage:NULL,
        get_unmapped_area:NULL,
        check_flags:NULL,
        dir_notify:NULL,
        flock:NULL,
};


/* msc_io_init_l24 - initialize
 */
int msc_io_init_l24(struct usbd_function_instance *function_instance)
{
        struct proc_dir_entry *message = NULL;

        proc_function_instance = function_instance;

        THROW_IF (!(message = create_proc_entry (MSC_IO, 0666, 0)), error);
        THROW_IF (!(message = create_proc_entry (MSC_IO, 0666, 0)), error);
        message->proc_fops = &msc_io_proc_switch_functions;

        return 0;

        CATCH(error) {
                printk(KERN_ERR"%s: creating /proc/msc_io failed\n", __FUNCTION__);
                if (message)
                        remove_proc_entry(MSC_IO, NULL);
                return -EINVAL;
        }
}

/* msc_io_exit_l24 - exit
 */
void msc_io_exit_l24(struct usbd_function_instance * function_instance)
{
	struct msc_private *msc = function_instance ? function_instance->privdata : NULL;
	int count = 0;

	TRACE_MSG2( MSC, "Cleaning up io. msc is%s null.  There are %d LUNs.\n", ((msc == NULL)? "":" not"), msc_curluns);
	msc->close_pending = 1; 

	if ( msc != NULL)
	    for ( count = 0 ; count < msc_maxluns ; count ++, msc++)
		wake_up_interruptible(&msc->ioctl_wq);

        remove_proc_entry(MSC_IO, NULL);
        proc_function_instance = NULL;
}
