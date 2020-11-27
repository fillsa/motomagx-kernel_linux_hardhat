/*
 * otg/function/msc/msc-linux.c
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Tony Tang <tt@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2008 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 07/06/2005         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 02/23/2007         Motorola         Changes for 32k per BIO size.
 * 04/10/2007         Motorola         Changes for free up the urb buffer.
 * 04/26/2007         Motorola         adjust capacity according to bdev.
 * 04/26/2007         Motorola         Changes for OTG_DISABLE_INTERRUPTS   
 * 05/25/2007         Motorola         Adjust bdev capacity
 * 11/23/2007	      Motorola	       Changes for wait for msc read write 
 * 08/08/2008         Motorola         Commented out OTG debug from dmesg
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
/*!
 * @file otg/functions/msc/msc-linux.c
 * @brief Mass Storage Driver private defines
 *
 *
 * This is a Mass Storage Class Function that uses the Bulk Only protocol.
 *
 * To use simply load with something like:
 *
 *      insmod msc_fd.o vendor_id=0xffff product_id=0xffff
 *
 * Notes:
 *
 * 1. Currently block I/O is done a page at a time. I have not determined if
 * it is possible to dispatch a multiple page generic request. It should at
 * least be possible to queue page requests. 
 *
 * 2. Currently for READ operations we have a maximum of one outstanding
 * read block I/O and send urb. These are allowed to overlap so that we can
 * continue to read data while sending data to the host.
 *
 * 3. Currently for WRITE operations we have a maximum of one outstanding
 * recv urb and one outstanding write block I/O. These are allowed to
 * overlap so that we can continue to receive data from the host while
 * waiting for writing to complete.
 *
 * 4. It would be possible to allow multiple writes to be pending, to the
 * limit of the page cache, if desired. 
 *
 * 5. It should be possible to allow multiple outstanding reads to be
 * pending, to the limit of the page cache, but this potentially could
 * require dealing with out of order completions of the reads. Essentially a
 * list of completed buffer heads would be required to hold any completed
 * buffer heads that cannot be sent prior to another, earlier request being
 * completed. 
 *
 * 6. Currently ioctl calls are available to start and stop device i/o. (wrong)
 *
 * 7. The driver can optionally generate trace messages showing each sectors 
 * CRC as read or written with LBA. These can be compared by user programs to
 * ensure that the correct data was read and/or written.
 *
 * 
 * TODO
 *
 * 1. error handling for block io, e.g. what if using with removable block
 * device (like CF card) and it is removed.
 *
 * 2. Currently we memcpy() data from between the urb buffer and buffer
 * head page. It should be possible to simply use the page buffer for the
 * urb. (If implemented, then one needs to change when the pending flags are changed)
 *
 * 3. Should we offer using fileio as an option? This would allow direct access
 * to a block device image stored in a normal file or direct access to (for example)
 * ram disks. It would require implementing a separate file I/O kernel thread to
 * do the actual I/O.
 *
 * 4. It may be interesting to support use of SCSI block device and pass the
 * scsi commands directly to that. This would allow vendor commands for real
 * devices to be passed through and executed with results being properly
 * returned to the host. [This is the intended design for the mass storage
 * specification.]
 *
 * 
 * TODO FIXME Bus Interface Notes
 *
 *
 * @ingroup MSCFunction
 */

/*------------------------------*/
/*     Included Files           */
/*------------------------------*/
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
#include <linux/fcntl.h>
#include <linux/bio.h>
#include <linux/buffer_head.h>

#define CONFIG_OTG_MSC_BLOCK_TRACE 1
#include <linux/blkdev.h>
#include <linux/kdev_t.h>
#include <public/otg-node.h>

#include "msc-scsi.h"
#include "msc.h"
#include "msc-fd.h"
#include "msc-io.h"


#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
#include "crc.h"
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */


/*------------------------------*/
/*     Module Parameters        */
/*------------------------------*/
EMBED_LICENSE();

MOD_PARM_INT (vendor_id, "Device Vendor ID", 0);
MOD_PARM_INT (product_id, "Device Product ID", 0);
MOD_PARM_INT (major, "Device Major", 0);
MOD_PARM_INT (minor, "Device Minor", 0);
MOD_PARM_INT (param_maxluns, "Number of LUNS", 1);


/*------------------------------*/
/*     Defines                  */
/*------------------------------*/
#define DEVICE_EJECTED          0x0001          // MEDIA_EJECTED
#define DEVICE_INSERTED         0x0002          // MEDIA_INSERT
#define DEVICE_OPEN             0x0004          // WR_PROTECT_OFF
#define DEVICE_WRITE_PROTECTED  0x0008          // WR_PROTECT_ON
#define DEVICE_CHANGE_ON        0x0010          // MEDIA_CHANGE_ON
#define DEVICE_PREVENT_REMOVAL  0x0020

#define DEF_NUMBER_OF_HEADS     0x10
#define DEF_SECTORS_PER_TRACK   0x20


/*------------------------------*/
/*     Macros                  */
/*------------------------------*/


/*------------------------------*/
/*     Global Variables         */
/*------------------------------*/
DECLARE_MUTEX(msc_sem);

extern struct usbd_interface_driver msc_interface_driver;

int msc_maxluns = 0;
int msc_curluns = 0;
int msc_cmdlun  = 0;


/*------------------------------*/
/*     Function Prototypes      */
/*------------------------------*/
int msc_urb_sent (struct usbd_urb *tx_urb, int rc);

int msc_start_reading_block_data(struct usbd_function_instance *function);
static int  msc_block_read_finished(struct bio *bio,unsigned int bytes_done,int err);

int msc_start_receiving_data(struct usbd_function_instance *function);
static int msc_data_written(struct bio *bio, unsigned bytes_done,int err);

extern void msc_global_init(void);


/*------------------------------*/
/*     Function Definitions     */
/*------------------------------*/

/* Block Device ************************************************************* */

/*! msc_open_blockdev - open the block device specified in msc->major, msc->minor
 *
 * Sets appropriate fields to show current status of block device.
 *
 * XXX TODO - this needs to be tested against RO and absent devices.
 *
 */
void msc_open_blockdev(struct usbd_function_instance *function_instance, int lunNumber)
{
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + lunNumber;
        int rc;
        struct inode *inode; 

        
        down(&msc_sem);
        msc->block_dev_state = DEVICE_EJECTED;
        TRACE_MSG2(MSC, "OPEN BLOCKDEV: Major: %x Minor: %x", msc->major, msc->minor);


	// Ensure non-zero major number; then make the device node.
        THROW_UNLESS(msc->major, ejected);
        msc->dev = MKDEV(msc->major, msc->minor);
        THROW_UNLESS((msc->bdev = bdget(msc->dev)), ejected);


	// Open up the block device; 
        THROW_IF ((blkdev_get(msc->bdev, FMODE_READ | FMODE_WRITE, 0)), ejected);
        
	// Default state values
        msc->io_state = MSC_INACTIVE;
        msc->block_dev_state &= ~DEVICE_EJECTED;
        msc->block_dev_state |= DEVICE_INSERTED | DEVICE_CHANGE_ON;

        // Blockdev values.
	if (msc->bdev->bd_contains == msc->bdev)
		msc->capacity = msc->bdev->bd_disk->capacity - 1;
	else	
		msc->capacity = msc->bdev->bd_part->nr_sects - 1;

        msc->block_size = bdev_hardsect_size(msc->bdev);
        msc->max_blocks = PAGE_SIZE / msc->block_size;
	msc->write_pending = msc->read_pending = 0;

        TRACE_MSG1(MSC,"OPEN BLOCKDEV: opened block_dev_state: %x", msc->block_dev_state);


        /*
         * Note that capacity must return the LBA of the last addressable block
         * c.f. RBC 4.4, RBC 5.3 and notes below RBC Table 6
         *
         * The blk_size array contains the number of addressable blocks or N, 
         * capacity is therefore N-1.
         */

        // Neat information to know!
        printk(KERN_INFO"%s: bdev: %x\n", __FUNCTION__, msc->bdev);
        printk(KERN_INFO"%s: bd_part: %x\n", __FUNCTION__, msc->bdev->bd_part);
        printk(KERN_INFO"%s: bd_disk: %x\n", __FUNCTION__, msc->bdev->bd_disk);
        printk(KERN_INFO"%s: capacity: %d\n", __FUNCTION__, msc->bdev->bd_disk->capacity);

        TRACE_MSG2(MSC,"capacity: %x %d", msc->capacity, msc->capacity); 
        TRACE_MSG2(MSC,"block_size: %x %d", msc->block_size, msc->block_size); 
        TRACE_MSG2(MSC,"max_blocks: %x %d", msc->max_blocks, msc->max_blocks); 

        up(&msc_sem);
        return ;
        

        CATCH(ejected) {
                TRACE_MSG1(MSC,"OPEN BLOCKDEV: EJECTED block_dev_state: %x", msc->block_dev_state);
                printk(KERN_INFO"%s: Cannot get device %d %d\n", __FUNCTION__, msc->major, msc->minor); 
        
                up(&msc_sem);
                return ;
        }
}



/*! msc_close_blockdev - close the device for host
 */
void msc_close_blockdev(struct usbd_function_instance *function_instance, int lunNumber)
{
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + lunNumber;
        RETURN_IF(msc == NULL);
	RETURN_IF(msc->block_dev_state == DEVICE_EJECTED);


	/* Wait for read/write to finish in order to not unmount memory being used. */
	printk(KERN_INFO"%s: WRWR read_pending:%x,write_pending:%x\n", __FUNCTION__,msc->read_pending,msc->write_pending);
	msc->read_pending = 0; msc->write_pending = 0;
//	wait_event_interruptible(msc->rdwr_wq, ((msc->read_pending+msc->write_pending)==0));

        down(&msc_sem);
	msc->block_dev_state = DEVICE_EJECTED;

	if (msc->bdev)
            blkdev_put(msc->bdev);         /* Unregister the block device */

	msc->bdev = NULL;                  /* Make the block device null to ensure no usage. */
        up(&msc_sem);
}


/*! msc_scsi_read_10 - process a read(10) command
 *
 * We have received a READ(10) CBW, if transfer length is non-zero
 * initiate a generic block i/o otherwise send a CSW.
 *
 * Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_read_10(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = ((struct msc_private *)(function_instance->privdata)) + msc_cmdlun;
        SCSI_READ_10_COMMAND *command = (SCSI_READ_10_COMMAND *)&msc->command.CBWCB;


        /* save the CBW information and setup for transmitting data read from the block device */
        msc->lba = be32_to_cpu(command->LogicalBlockAddress);
        msc->TransferLength_in_blocks = be16_to_cpu(command->TransferLength);
        msc->TransferLength_in_bytes = msc->TransferLength_in_blocks * msc->block_size;

        msc->command_state = MSC_DATA_IN_READ;
        msc->io_state = MSC_INACTIVE;
	msc->read_pending = ((msc->TransferLength_in_blocks)? 1 : 0);  // Protect?

        TRACE_MSG2(MSC,"LBA: 0x%x, TX Len (blocks): %d",msc->lba, command->TransferLength);


        /*
         * Start reading blocks to send or simply send the CSW if the host
         * didn't actually ask for a non-zero length.
         */
        return (msc->TransferLength_in_blocks) ?  msc_start_reading_block_data(function_instance) :
                msc_start_sending_csw(function_instance, USB_MSC_PASSED);
}


/*! msc_start_reading_block_data - start reading data
 *
 * Generate a generic block io request to read some data to transmit.
 *
 * This function is initially called by msc_scsi_read_10() but can also be called
 * by msc_urb_sent() if the amount of data requested was too large.
 *
 * The function msc_block_read_finished() will be called to actually send the data
 * to the host when the I/O request is complete.
 *
 */
int msc_start_reading_block_data(struct usbd_function_instance *function_instance)
{
        struct msc_private *msc = ((struct msc_private *)(function_instance->privdata)) + msc_cmdlun;
        int TransferLength_in_blocks = MIN(msc->max_blocks, msc->TransferLength_in_blocks);
        unsigned long flags;
        char *btemp_data;
        struct bio * read_bio = NULL;
        

        /* Error Checking */
        THROW_IF( (msc->block_dev_state != DEVICE_INSERTED), error);
        THROW_IF( (msc->lba >= msc->capacity), error);
	RETURN_ZERO_IF((TransferLength_in_blocks * msc->block_size) == 0);
        

        /* Allocate a bio structure and a page for the read. */
	THROW_IF( ((read_bio = bio_alloc(GFP_NOIO, 1)) == 0), error);
        THROW_IF( ((read_bio->bi_io_vec->bv_page = alloc_page(GFP_NOIO)) == NULL), error); 
        
        local_irq_save(flags);
        msc->io_state |= MSC_BLOCKIO_PENDING;
        local_irq_restore(flags);
        
        
        /*Set up the values for the read. */
	read_bio->bi_io_vec->bv_len = TransferLength_in_blocks * msc->block_size;;
	read_bio->bi_io_vec->bv_offset = 0;
	
        read_bio->bi_vcnt = 1;
	read_bio->bi_idx = 0;
	read_bio->bi_size = TransferLength_in_blocks * msc->block_size;
	read_bio->bi_bdev = msc->bdev;
	read_bio->bi_sector = msc->lba;
	read_bio->bi_private = function_instance;
	read_bio->bi_end_io = msc_block_read_finished;

        /* Zero out the memory to be read. */
	btemp_data=__bio_kmap_atomic(read_bio,0,KM_USER0);
        memset(btemp_data, 0x0, read_bio->bi_size);
	__bio_kunmap_atomic(btemp_data,KM_USER0);
	

        /* Submit Request */
        submit_bio(READ, read_bio);
        generic_unplug_device(bdev_get_queue(msc->bdev));

        return 0;



        CATCH(error){
                /* Set sense data depending on the error. */
                u32 sense_data = SCSI_SENSEKEY_UNRECOVERED_READ_ERROR;
                sense_data = ((msc->block_dev_state != DEVICE_INSERTED) ? SCSI_SENSEKEY_MEDIA_NOT_PRESENT : sense_data);
                sense_data = ((msc->lba >= msc->capacity) ? SCSI_SENSEKEY_BLOCK_ADDRESS_OUT_OF_RANGE : sense_data);


                /* Free bio struct if it was allocated.  */
                if (read_bio != NULL){
                        if ( read_bio->bi_io_vec->bv_page )
                            __free_page(read_bio->bi_io_vec->bv_page);

                        bio_put(read_bio);
                }


                /* Trace and return failed. */
                TRACE_MSG4( MSC, "Error! sense_data: 0x%x block_dev_state: 0x%x lba: %d capacity: %d", sense_data, msc->block_dev_state, msc->lba, msc->capacity);
                return msc_start_sending_csw_failed (function_instance, sense_data, msc->lba, USB_MSC_FAILED);

        }
}


/*! msc_block_read_finished - called by generic request 
 *
 * Called when block i/o read is complete, send the data to the host if possible.
 *
 * The function msc_urb_sent() will be called when the data is sent to
 * either send additional data or the CSW as appropriate.
 *
 * If more data is required then call msc_start_reading_block_data() to
 * issue another block i/o to get more data.
 *
 * These means that there can be two outstanding actions when this
 * function completes:
 *
 *      1. a transmit urb may be pending, sending the most recently
 *         read data to the host
 *      2. another block i/o may be pending to read additional data
 *
 * This leads to a race condition, if the block i/o finished before the urb
 * transmit, then we must simply exit. The msc_in_read_10_urb_sent()
 * function will ensure that this is restarted.
 */

static int  msc_block_read_finished(struct bio *bio,unsigned int bytes_done,int err)
{
        struct usbd_function_instance *function_instance = bio->bi_private;
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;
        int TransferLength_in_blocks;
        struct usbd_urb *tx_urb;
        int rc;
	int i;
	char *btemp_data;
        unsigned long flags;

        
	if(bio->bi_size || NULL == msc) 
            return 1;
	

        TransferLength_in_blocks = bytes_done / msc->block_size;
        TRACE_MSG2(MSC," bio BLOCK READ FINISHED size: %x - TransferLength_in_blocks: %d", bytes_done,TransferLength_in_blocks); 
        
        /*
         * Race condition here, if we have not finished sending the
         * previous tx_urb then we want to simply remmber the parameters ,exit and let
         * msc_in_read_10_urb_sent() call us again.
         *
         * Ensure that we do not reset BLOCKIO flags if SEND PENDING and
         * that we do reset BLOCKIO if not SEND PENDING
         */
         
         local_irq_save(flags);


         if (msc->io_state & MSC_SEND_PENDING) {
                TRACE_MSG2(MSC,"Send Penind, rcv_bio: 0x%x, 0x%x", ((unsigned int) msc->rcv_bio_finished), msc->io_state);
                msc->io_state |= MSC_BLOCKIO_FINISHED;       /* This flag actuall represents pending READING block io */
                msc->rcv_bio_finished = bio;
                msc->bytes_done = bytes_done;
                msc->err = err;

                local_irq_restore(flags);
                return 0 ;
        }
        msc->io_state &= ~(MSC_BLOCKIO_PENDING | MSC_BLOCKIO_FINISHED); 
        local_irq_restore(flags);

        

        /* Verify that the I/O completed, and allocate urb */
        THROW_IF (err, error);
        THROW_IF(!(tx_urb = usbd_alloc_urb (function_instance, BULK_IN, bytes_done, msc_urb_sent)), error);


        /* Setup urb and memory. */
        tx_urb->function_privdata = function_instance;
        tx_urb->actual_length = tx_urb->buffer_length = bytes_done;
	
	btemp_data=__bio_kmap_atomic(bio, 0, KM_USER0);
        memcpy(tx_urb->buffer, btemp_data, bytes_done);
	__bio_kunmap_atomic(btemp_data,KM_USER0);


#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
	TRACE_MSG0(MSC,"sos CONFIG_OTG_MSC_BLOCK_TRACE");
        for (i = 0; i < bytes_done / msc->block_size; i++) {
                u32 crc = crc32_compute(btemp_data + (i * msc->block_size), msc->block_size, CRC32_INIT);
                TRACE_RLBA(bio->bi_sector + i, crc);
        }
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */
	
        local_irq_save(flags);

        msc->io_state |= MSC_SEND_PENDING;
        msc->TransferLength_in_bytes -= bytes_done;
        msc->TransferLength_in_blocks -= TransferLength_in_blocks; 
        msc->lba += TransferLength_in_blocks;


        /* If true, then we have queued all the data for this read request. */
        if ( !(msc->TransferLength_in_blocks) ) {
                TRACE_MSG0(MSC,"Read IO Finished");
                msc->command_state = MSC_DATA_IN_READ_FINISHED;

                msc->read_pending=0;
                wake_up_interruptible(&msc->rdwr_wq);
        }

        local_irq_restore(flags);


        /* dispatch urb - msc_urb_sent() will be called when urb is finished  */        
        if ((rc = usbd_start_in_urb (tx_urb))) {
                TRACE_MSG0(MSC,"BLOCK READ FINISHED FAILED"); 
                usbd_free_urb (tx_urb);
                THROW(error);
        }

        /* AFTER sending the urb, start gathering more data if aren't finished */
        if ( ! (msc->command_state == MSC_DATA_IN_READ_FINISHED)){
                TRACE_MSG0(MSC,"Read IO Not Finished, requesting more data");
                msc_start_reading_block_data(function_instance);
        }


        /* Free memory used for the read. */
        __free_page(bio->bi_io_vec->bv_page);
        bio_put(bio);

	return 0;


        CATCH(error) {
                __free_page(bio->bi_io_vec->bv_page);
                bio_put(bio);
                
                TRACE_MSG1(MSC, "Read Error Value: %d" , err);
                msc->read_pending=0;
                wake_up_interruptible(&msc->rdwr_wq);

                return msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_UNRECOVERED_READ_ERROR, msc->lba, USB_MSC_FAILED);

        }

}


/*! msc_in_read_10_urb_sent - called by msc_urb_sent when state is MSC_DATA_IN_READ
 *
 * This will process a read_10 urb that has been sent. Specifically if any previous
 * read_10 block I/O has finished we recall the msc_block_read_finished() function
 * to transmit another read_10 urb. 
 *
 * If there is no other pending read_10 to do we create and send a CSW.
 *
 * If there is more I/O to do or there is already an outstanding I/O we simply
 * return after freeing the URB.
 *
 * Return non-zero if urb was not disposed of.
 */
int msc_in_read_10_urb_sent(struct usbd_urb *tx_urb)
{
        struct usbd_function_instance *function_instance = tx_urb->function_privdata;
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;
        unsigned long flags;

        TRACE_MSG0(MSC,"URB SENT DATA IN");

        /*
         * Potential race condition here, we may need to restart blockio.
         */
        local_irq_save(flags);

        msc->io_state &= ~MSC_SEND_PENDING;
        msc->data_transferred_in_bytes += tx_urb->actual_length;

        TRACE_MSG1(MSC,"Data transferred: %d", msc->data_transferred_in_bytes);


        if (!tx_urb->actual_length) 
                msc->TransferLength_in_blocks = 0;

        /* XXX We should be checking urb status */
        usbd_free_urb (tx_urb);


        /* Check to see if we need to send CSW. */
        if (msc->command_state == MSC_DATA_IN_READ_FINISHED) {
                TRACE_MSG0(MSC,"Read finished, send CSW");
                local_irq_restore(flags);
                return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
        }


        /* Check to see if there is a block read needs to be finished. */
        if (MSC_BLOCKIO_FINISHED & msc->io_state) {   /* This flag represents pending READ block io. */
                TRACE_MSG0(MSC,"Finishing a pending bio.");
                local_irq_restore(flags);

	 	msc_block_read_finished(msc->rcv_bio_finished, msc->bytes_done, msc->err);
                return 0;
        }
        local_irq_restore(flags);
        return 0;
}




/*! msc_scsi_write_10 - process a write command
 *
 * Call either msc_start_receiving_data() or msc_start_sending_csw() as appropriate.
 * Normally msc_start_receiving_data() is called and it will use msc_start_recv_urb() 
 * to setup a receive urb of the appropriate size. 
 *
 * Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_write_10(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;
        SCSI_WRITE_10_COMMAND *command = (SCSI_WRITE_10_COMMAND *)&msc->command.CBWCB;


        /* save the CBW and setup for receiving data to be written to the block device */
        msc->lba = be32_to_cpu(command->LogicalBlockAddress);
        msc->TransferLength_in_blocks = be16_to_cpu(command->TransferLength);
        msc->TransferLength_in_bytes = msc->TransferLength_in_blocks * msc->block_size;


        msc->command_state = MSC_DATA_OUT_WRITE;
        msc->io_state = MSC_INACTIVE;
	msc->write_pending = 1;


        TRACE_MSG2(MSC,"RECV WRITE lba: %x, lun: %d", msc->lba, msc_cmdlun); 
        TRACE_MSG1(MSC,"RECV WRITE blocks: %d", msc->TransferLength_in_blocks); 

        return (msc->TransferLength_in_blocks) ?
                msc_start_receiving_data(function_instance) :
                msc_start_sending_csw(function_instance, USB_MSC_PASSED);
}
 

/*! msc_start_receiving_data - called to initiate an urb to receive WRITE(10) data
 *
 * Initiate a receive urb to receive upto PAGE_SIZE WRITE(10) data. The
 * msc_recv_urb() function will call msc_recv_out_blocks() to actually
 * write the data. 
 *
 * This is called from msc_scsi_write_10() to initiate the WRITE(10) and
 * called from msc_data_written() to start the next page sized receive
 * urb.
 */
int msc_start_receiving_data(struct usbd_function_instance *function_instance)
{
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;

        /* Calculating the length is most of the work we do. */
        int TransferLength_in_blocks = MIN( CONFIG_OTG_MSC_NUM_PAGES * msc->max_blocks, msc->TransferLength_in_blocks);

        TRACE_MSG3(MSC,"lba: 0x%x, blocks: %d, bytes: %d", msc->lba, msc->TransferLength_in_blocks, (TransferLength_in_blocks * msc->block_size)); 


        THROW_IF( !((msc->command_state == MSC_DATA_OUT_WRITE) || (msc->command_state == MSC_DATA_OUT_WRITE_ERROR)), error);
        THROW_IF(!msc->TransferLength_in_blocks, error);

        msc->io_state |= MSC_RECV_PENDING;
        return msc_start_recv_urb_for_write(function_instance, TransferLength_in_blocks * msc->block_size);


        CATCH(error) {
                TRACE_MSG0(MSC,"START RECEIVING DATA ERROR");
                return -EINVAL;
        }
}


/*! msc_recv_out_blocks - process received WRITE(10) data by writing to block device
 *
 * We get here indirectly from msc_recv_urb() when state is MSC_DATA_OUT_WRITE.
 *
 * Dealloc the urb and call Initiate the generic request to write the
 * received data. The b_end_io function msc_data_written() will send the
 * CSW.
 *
 */
void msc_recv_out_blocks(struct usbd_urb *rcv_urb)
{
        struct usbd_function_instance *function_instance = rcv_urb->function_privdata;
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
        int TransferLength_in_bytes = rcv_urb->actual_length;
        int TransferLength_in_blocks = TransferLength_in_bytes / msc->block_size;
        u32 writeBlockAddress = 0;
        int i;
	char *btemp_data;
        struct bio * write_bio = NULL;
        unsigned long flags;


	TRACE_MSG4(MSC,"bytes: %d, lun: %d, iostate: 0x%x, TXed (bytes) %d", TransferLength_in_bytes, msc_cmdlun, msc->io_state, msc->data_transferred_in_bytes);
        TRACE_MSG3(MSC,"lba: 0x%x, left: %d, blocks current: %d", msc->lba, msc->TransferLength_in_blocks, TransferLength_in_blocks); 

        /*
         * Race condition here, we may get to here before the previous block
         * write completed. If so just exit and the msc_data_written()
         * function will recall us.
         */
        local_irq_save(flags);
        if (msc->io_state & MSC_BLOCKIO_PENDING){
                TRACE_MSG0(MSC,"RECV OUT  BLOCKIO PENDING");
                msc->io_state |= MSC_RECV_FINISHED;
                msc->rcv_urb_finished = rcv_urb;

                local_irq_restore(flags);
                return;
        }
        msc->io_state &= ~(MSC_RECV_PENDING |MSC_RECV_FINISHED);
        local_irq_restore(flags);


        /* decrement counters and increment address */
        writeBlockAddress = msc->lba;
        msc->lba += TransferLength_in_blocks;
        msc->TransferLength_in_bytes -= TransferLength_in_bytes;
        msc->TransferLength_in_blocks -= TransferLength_in_blocks;  // This is the problem line.


#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        for (i = 0; i < TransferLength_in_blocks; i++) {
                u32 crc = crc32_compute(rcv_urb->buffer + (i * msc->block_size), msc->block_size, CRC32_INIT);
                TRACE_SLBA(writeBlockAddress + i, crc);
        }
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */
        

        /* XXX additional sanity check required here - verify that the transfer
         * size agrees with what we where expecting, specifically I think
         * we need to verify that we have received a multiple of the block
         * size and either a full bulk transfer (so more will come) or
         * the actual exact amount that the host said it would send.
         * An error should generate a CSW with a USB_MSC_PHASE_ERROR
         */

        /* Sanity Checks */ 
        THROW_IF( (msc->command_state == MSC_DATA_OUT_WRITE_ERROR), error);
        THROW_IF( (msc->block_dev_state != DEVICE_INSERTED), error);
        THROW_IF( (writeBlockAddress >= msc->capacity), error);
        THROW_IF( (msc->TransferLength_in_blocks < 0), error);
	THROW_IF( !(TransferLength_in_bytes), error);

        int numleft = TransferLength_in_bytes;
        int num_bv = (TransferLength_in_bytes + PAGE_SIZE - 1) >> PAGE_SHIFT;
        char* cp = rcv_urb->buffer;
        int count = 0;

        msc->rcv_urb_bio_current = rcv_urb;

        THROW_IF( ((write_bio = bio_alloc(GFP_NOIO, num_bv)) == NULL), error);
        write_bio->bi_vcnt = num_bv;
	write_bio->bi_idx = 0;
        write_bio->bi_size = TransferLength_in_bytes;
	write_bio->bi_bdev = msc->bdev;
        write_bio->bi_sector = writeBlockAddress;
        write_bio->bi_private = function_instance;
        write_bio->bi_end_io = msc_data_written;

        for( count=0; count < num_bv; count++) 
        {
            int len;
            struct bio_vec *bv;
            bv = bio_iovec_idx(write_bio, count);
            THROW_IF( (( bv->bv_page = virt_to_page((void *)cp) ) == NULL), error);
            len= (numleft > PAGE_SIZE) ? PAGE_SIZE : numleft;
            bv->bv_len = len;
            bv->bv_offset = 0;

            numleft -= len;
            cp += len;
        }


        /* Get last usefull information and free the urb */
        msc->data_transferred_in_bytes += rcv_urb->actual_length;

        local_irq_save(flags);
	msc->io_state |= MSC_BLOCKIO_PENDING;
        local_irq_restore(flags);

        if (msc->TransferLength_in_blocks)
            msc_start_receiving_data(function_instance);


        /* Submit bio to the Block layer and return. */
        submit_bio(WRITE, write_bio);
        generic_unplug_device(bdev_get_queue(msc->bdev));
        return;


        
        CATCH(error) {
                /* Set sense data depending on the error. */
                msc->sensedata = ((msc->block_dev_state != DEVICE_INSERTED) ? SCSI_SENSEKEY_MEDIA_NOT_PRESENT : msc->sensedata);
                msc->sensedata = ((writeBlockAddress >= msc->capacity) ? SCSI_SENSEKEY_BLOCK_ADDRESS_OUT_OF_RANGE : msc->sensedata);
                msc->sensedata = ((msc->TransferLength_in_blocks < 0) ? SCSI_SENSEKEY_BLOCK_ADDRESS_OUT_OF_RANGE : msc->sensedata);
                /* Not sure why this line is wrong, but I should fix it at some point.  Doesn't enumerate well when not fixed.
                   What it is suppose to do is set a default value for the sense data if nothing else has been sensed. 
                   msc->sensedata = ((msc->sensedata == SCSI_SENSEKEY_NO_SENSE) ? SCSI_SENSEKEY_WRITE_ERROR : msc->sensedata); */

                TRACE_MSG3(MSC, "ERROR: Sense data: 0x%x, WBA: 0x%x, Capacity: 0x%x", msc->sensedata, writeBlockAddress, msc->capacity);
                TRACE_MSG3(MSC, "     : State: %d, TXLen (blks): %d, blk_dev_state: %d", msc->command_state, msc->TransferLength_in_blocks, msc->block_dev_state);

                /* Free bio struct if it was allocated.  */
                if (write_bio != NULL){
                    /* Free Page from bio structure */
                    for( i=0; i < write_bio->bi_vcnt ; i++ )
                    {
                        if (write_bio->bi_io_vec[i].bv_page)
                            __free_page(write_bio->bi_io_vec[i].bv_page);
                    }

                    bio_put(write_bio);
                }

                /* Free urb is necessairy */
                if (rcv_urb != NULL)
                {
                    /* We already freed up the buffer above. So, make it to NULL and then
                       Free up the urb
                    */
                    rcv_urb->buffer = NULL;
                    usbd_free_urb(rcv_urb);
                }


                /* Send CSW if there is a failure, and set the command state otherwise */
                if ( msc->TransferLength_in_blocks ){
                        msc->command_state = MSC_DATA_OUT_WRITE_ERROR;
                        msc_start_receiving_data(function_instance);
                }
                else {
                        msc_start_sending_csw_failed (function_instance, msc->sensedata, msc->lba, USB_MSC_FAILED);
                }

                return;
        }
}


/*! msc_data_written - called by generic request 
 *
 * Called when current block i/o read is finished, restart block i/o or send the CSW.
 *
 */
static int msc_data_written(struct bio *bio, unsigned bytes_done,int err)
{
        struct usbd_function_instance *function_instance = bio->bi_private;
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
        unsigned long flags;
        u8 io_state;
        int i =0;

        struct usbd_urb *rcv_urb = NULL;


        rcv_urb = msc->rcv_urb_bio_current;
        free_pages((unsigned long)rcv_urb->buffer,get_order(bytes_done));
        rcv_urb->buffer = NULL;
        usbd_free_urb(rcv_urb);



        /* Check current status of TransferLength - if non-zero then we need
         * to queue another urb to receive more data. */
        if (!msc->TransferLength_in_blocks) 
                msc->command_state = MSC_DATA_OUT_WRITE_FINISHED;
	
        if (bio->bi_size)
		return 1;


        bio_put(bio);


	TRACE_MSG5(MSC,"DATA WRITTEN bytes_done: 0x%x err: 0x%x state: 0x%x io: 0x%x, lba: 0x%x", 
                        bytes_done, err, msc->command_state, msc->io_state, msc->lba); 

        
        local_irq_save(flags);
        io_state = msc->io_state &= ~MSC_BLOCKIO_PENDING;
        local_irq_restore(flags);


        /* verify that the I/O completed */
        if (err) {
                TRACE_MSG3(MSC,"FAILURE: XFLen (blks): %d, rcv_urb: 0x%x, state: %d", msc->TransferLength_in_blocks, msc->rcv_urb_finished, msc->command_state); 

                if ( msc->TransferLength_in_blocks ){
                        
                        /* Check error and set sense data accordingly */
                        err = ((err < 0) ? err * -1 : err);
                        msc->sensedata = ((err == ENODEV)? SCSI_SENSEKEY_MEDIA_NOT_PRESENT : msc->sensedata);
                        msc->sensedata = ((err == ENOMEM)? SCSI_SENSEKEY_MEDIA_NOT_PRESENT : msc->sensedata);
                        msc->sensedata = ((err == EINVAL)?  SCSI_SENSEKEY_BLOCK_ADDRESS_OUT_OF_RANGE : msc->sensedata);
                        msc->sensedata = ((msc->sensedata == SCSI_SENSEKEY_NO_SENSE) ? SCSI_SENSEKEY_WRITE_ERROR : msc->sensedata);

                        /* set state machine */
                        msc->command_state = MSC_DATA_OUT_WRITE_ERROR;

                        if (msc->rcv_urb_finished) {
                                struct usbd_urb *rcv_urb = msc->rcv_urb_finished;
                                msc->rcv_urb_finished = NULL;
                                msc_recv_out_blocks(rcv_urb);
                        }
                }
                else {
                        msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_WRITE_ERROR, msc->lba, USB_MSC_FAILED);
                }

                return 0;
        }

        /*
         * If there was a rcv_urb that was not processed then we need
         * to process it now. This is done by simply restarting msc_recv_out()
         */
        if ((io_state & MSC_RECV_FINISHED) && msc->rcv_urb_finished) {
                struct usbd_urb *rcv_urb = msc->rcv_urb_finished;
                msc->rcv_urb_finished = NULL;
                TRACE_MSG0(MSC,"DATA WRITTEN RECV FINISHED");
                msc_recv_out_blocks(rcv_urb);
        }


        /*
         * DATA_IN mode and no more data to write
         */
        if (MSC_DATA_OUT_WRITE_FINISHED == msc->command_state) {
                TRACE_MSG0(MSC,"DATA WRITTEN send CSW");
		msc->write_pending=0;
		wake_up_interruptible(&msc->rdwr_wq);
		msc_start_sending_csw(function_instance, USB_MSC_PASSED);
        }
	return 0;
}
 

/*******************************************************
 * FUNCTION: msc_modinit
 *
 * DESCRIPTION:
 *   This function is ran at when the module is first installed
 *   in the kernel, either when insmod'd or when installed (static)
 *
 *   Sets up default values for some variables.  Most init-like
 *   functionality is taken care of in msc_function_enable().
 *
 * ARGUMENTS:
 *   None
 *
 * RETURN VALUE:
 *   int:  0  sucess
 *   int: <0  failure
 *
 ********************************************************/
static int msc_modinit (void)
{
        int rc;

	msc_maxluns = MODPARM(param_maxluns);
	msc_curluns = 0;

	THROW_IF(msc_maxluns < 1, error);
	THROW_IF(msc_maxluns > 8, error);

        printk(KERN_INFO "Copyright (c) 2004 Belcarra Technologies; www.belcarra.com; sl@belcarra.com\n");
        printk (KERN_INFO "%s vendor_id: %04x product_id: %04x major: %d minor: %d maxlun: %d\n", __FUNCTION__, 
                        MODPARM(vendor_id), MODPARM(product_id), MODPARM(major), MODPARM(minor), msc_maxluns);

#ifndef OTG_C99
#warning "C99"
	msc_global_init();
#endif
        MSC = otg_trace_obtain_tag();


#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        make_crc_table();
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */


        /* register as usb function driver */
        RETURN_EINVAL_IF(usbd_register_interface_function (&msc_interface_driver, MSC_DRIVER_NAME, NULL));


        TRACE_MSG3(MSC,"PAGE_SHIFT: %x PAGE_SIZE: %x %d", PAGE_SHIFT, PAGE_SIZE, PAGE_SIZE); 
        return 0; 


        CATCH(error) {
	        ;
		TRACE_MSG0(MSC, "Error on msc mod init.");
                otg_trace_invalidate_tag(MSC);
                return -EINVAL;
        }
}


/*******************************************************
 * FUNCTION: msc_modexit
 *
 * DESCRIPTION:
 *   This function is ran at when the module is removed.
 *
 *   Unregisters interface and trace tag. More clean up
 *   functionality is taken care of in msc_function_disable().
 *
 * ARGUMENTS:
 *   None
 *
 * RETURN VALUE:
 *   int:  0  sucess
 *   int: <0  failure
 *
 ********************************************************/
static void msc_modexit (void)
{

        usbd_deregister_interface_function (&msc_interface_driver);

#ifdef CONFIG_OTG_MSC_BLOCK_TRACE
        free_crc_table();
#endif /* CONFIG_OTG_MSC_BLOCK_TRACE */

        otg_trace_invalidate_tag(MSC);
}



/*------------------------------*/
/*     Static Check             */
/*------------------------------*/
#ifdef USB_STATIC
device_initcall (msc_modinit);
#else
module_init (msc_modinit);
#endif
module_exit (msc_modexit);


