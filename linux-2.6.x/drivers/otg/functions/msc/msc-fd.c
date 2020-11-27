/*
 * otg/function/msc/msc.-fd.c
 *
 *      Copyright (c) 2003-2004 Belcarra
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 * 
 * Copyright (c) 2005-2008 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 08/04/2005         Motorola         Initial distribution 
 * 04/14/2006         Motorola         Updates and improvements 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 11/01/2006         Motorola         stop, suspend and resume handling
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 01/05/2007         Motorola         removed workaround for LastLogicalBlockAddress
 * 02/23/2007         Motorola         Modified to receive more data in usb urb allocation 
 * 03/07/2007         Motorola         Added function msc_start_recv_urb_for_write
 * 03/08/2007         Motorola         Close block device for all cases.
 * 04/26/2007         Motorola         Changes for MAC intel enumeration.
 * 04/26/2007         Motorola         Added callback function msc_recv_urb_for_write.
 * 05/15/2007         Motorola         Don't schedule hotplug in reset if there is one scheduled.
 * 05/21/2007         Motorola         Changes for start_stop command handler.
 * 08/28/2007         Motorola         Disable hotplug function when suspend and resume
 * 10/01/2007         Motorola         Added check for URB status.       
 * 01/09/2008         Motorola         Enabled hotplug
 * 06/05/2008         Motorola         Modify scsi inquiry and read_capacity cmd handler to 
 *                                     support linux based pc
 * 07/29/2008         Motorola         Ensure msc does start receiving the next urb until the
                                       previous one is received.This is to fix the panic when 
                                       connected to a MAC PC which sleeps and wakes up 
                                        
 *
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
 * @file otg/functions/msc/msc-fd.c
 * @brief Mass Storage Driver private defines
 *
 * This is a Mass Storage Class Function that uses the Bulk Only protocol.
 *
 *
 * Notes:
 *
 * 1. Currently we only support the Bulk Only model. Microsoft states that
 * further support for the mass storage driver will only be done for devices
 * that conform to the Bulk Only model.
 *
 * 2. Error handling should be done with STALL but using ZLP seems to also
 * work. ZLP is usually easier to implement (except possibly on the SA1100.)
 * We may need to make STALL an option if we find devices (perhaps SA1100)
 * that cannot reliaby send a ZLP on BULK-IN endpoint.
 *
 *
 * 3. WinXP will match the following:
 *
 *      USB: Class_08&SubClass_02&Prot_50
 *      USB: Class_08&SubClass_05&Prot_50
 *      USB: Class_08&SubClass_06&Prot_50
 *
 * SubClass 02 is MMC or SFF8020I
 * SubClass 05 is SFF or SFF8070I
 * SubClass 06 is SCSI
 *
 * From the Windows USB Storage FAQ:
 *
 *      RBC not supported
 *
 *      SubClass = 6 (SCSI)
 *              CDBs SHOULD NOT be padded to 12 bytes
 *              ModeSense/ModeSelect SHOULD NOT be translated from 1ah/15h to 5ah/55h
 *              should be used for FLASH
 *
 *      SubClass !=6
 *              CDBs SHOULD be padded to 12 bytes
 *              ModeSense/ModeSelect SHOULD be translated from 1ah/15h to 5ah/55h
 *
 * We are using the former, SubClass = 6, and implement the required SCSI operations.
 *
 * 
 * TODO
 *
 *
 * 
 * TODO FIXME Bus Interface Notes
 *
 * 1. The bus interface driver must correctly implement NAK'ing if not rcv_urb is
 * present (see au1x00.c or wmmx.c for examples.)
 *
 * 2. The bus interface driver must implement USBD_URB_SENDZLP flag (see au1x00.c 
 * for example.)
 *
 * @ingroup MSCFunction
 */


#include <otg/otg-compat.h>
#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/otg-trace.h>

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/random.h>
#include <linux/slab.h>


#include <linux/buffer_head.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <public/otg-node.h>

#include "msc-scsi.h"
#include "msc.h"
#include "msc-fd.h"
#include "crc.h"


#ifdef CONFIG_OTG_MSC_HOTPLUG

#include <otg/hotplug.h>
struct usb_hotplug_private gen_priv_msc;

#endif /* CONFIG_OTG_MSC_HOTPLUG */


/*------------------------------*/
/*     Global Variables         */
/*------------------------------*/
struct usbd_function_instance *proc_function_instance = NULL;
otg_tag_t msc_fd_trace_tag;
u8  msc_recv_urb_already;     //when there is a unfinished urb for recv, we should not issue it again in msc_endpoint_cleared(). 0- no recv urb, 1 - we have a urb already.
/*------------------------------*/
/*     Function Prototypes      */
/*------------------------------*/
int msc_urb_sent (struct usbd_urb *tx_urb, int rc);
static int msc_recv_urb (struct usbd_urb *urb, int rc);
static int msc_recv_urb_for_write (struct usbd_urb *urb, int rc);


int msc_io_init_l24(struct usbd_function_instance *function_instance);
void msc_io_exit_l24(struct usbd_function_instance *function_instance);

static void msc_close_all_blockdev (struct usbd_function_instance *function_instance);

/*------------------------------*/
/*     External Functions       */
/*------------------------------*/
extern void msc_close_blockdev (struct usbd_function_instance *function_instance, int lunNumber);
extern void msc_open_blockdev(struct usbd_function_instance *function_instance, int lunNumber);

extern int msc_scsi_read_10(struct usbd_function_instance *function_instance, char *name, int op);
extern int msc_in_read_10_urb_sent(struct usbd_urb *tx_urb);

extern int msc_scsi_write_10(struct usbd_function_instance *function_instance, char *name, int op);
extern void msc_recv_out_blocks(struct usbd_urb *rcv_urb);


/*------------------------------*/
/*     Function Definitions     */
/*------------------------------*/

/*! 
 * @brief set_sense_data - set sensedata in msc private struture
 *
 * This saves sense data. Sense data indicates what type of error
 * has occurred and will be returned to the host when a request sense
 * command is sent.

 * @param function_instance
 * @param sensedata
 * @param info
 * @return none
 */
void set_sense_data(struct usbd_function_instance *function_instance, u32 sensedata, u32 info)
{
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;
        TRACE_SENSE(sensedata, info);
        msc->sensedata = sensedata;
        msc->info = info;
}

/* Check blockdev ****************************************************************************** */

/*!
 * @brief msc_check_blockdev_name - check current status of the block device
 *
 * Check if the block device is operational, either generate a failed CSW 
 * or a ZLP if not ready.
 *
 * Returns non-zero if the block device is not available for I/O operations
 * and a failed CSW has already been sent.
 * @param function_instance
 * @param zlp
 * @param name
 * @return int
 */
int msc_check_blockdev_name(struct usbd_function_instance *function_instance, int zlp, char *name)
{
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
        TRACE_MSG1(MSC, "CHECK BLOCKDEV... (lun:%d)", msc_cmdlun);
        
        
        if (msc->block_dev_state & DEVICE_EJECTED) {
                TRACE_MSG0(MSC,"CHECK BLOCKDEV DEVICE_EJECTED");
                ((SENDZLP == zlp) ? msc_dispatch_query_urb_zlp : msc_start_sending_csw_failed)
                  (function_instance, SCSI_SENSEKEY_MEDIA_NOT_PRESENT, msc->lba, USB_MSC_FAILED);
                return -EINVAL;
        }
        
        if (msc->block_dev_state & DEVICE_CHANGE_ON) {
                msc->block_dev_state &= ~DEVICE_CHANGE_ON;
                TRACE_MSG0(MSC,"CHECK BLOCKDEV DEVICE_CHANGE_ON");
                ((SENDZLP == zlp) ? msc_dispatch_query_urb_zlp : msc_start_sending_csw_failed)
                  (function_instance, SCSI_SENSEKEY_NOT_READY_TO_READY_CHANGE, msc->lba, USB_MSC_FAILED);
                return -EINVAL;
        }
        TRACE_MSG0(MSC,"CHECK BLOCKDEV READY TO GO!");
        return 0;
}
      
/* Generic start recv urb and send csw ********************************************************* */

/*!
 * @brief msc_start_recv - queue a receive urb
 *
 * Ensure that size is a multiple of the endpoint packetsize.
 *
 * Returns non-zero if there is an error in the USB layer.
 * @param function_instance
 * @param size
 * @return int
 */
int msc_start_recv_urb(struct usbd_function_instance *function_instance, int size)
{
        struct usbd_urb *rcv_urb = NULL;
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata);
        int wMaxPacketSize = usbd_endpoint_wMaxPacketSize(function_instance, BULK_OUT, usbd_high_speed(function_instance));
        int i = 0;
        u32 recv_urb_flag;
        
        RETURN_ZERO_IF(msc_recv_urb_already);

        local_irq_save(recv_urb_flag);
        msc_recv_urb_already=1;
        local_irq_restore(recv_urb_flag);
        TRACE_MSG0(MSC,"enter msc_start_recv_urb");
        if ((size % wMaxPacketSize)) 
            size = ((size + wMaxPacketSize) / wMaxPacketSize) * wMaxPacketSize;
        
        RETURN_EINVAL_UNLESS((rcv_urb = usbd_alloc_urb (function_instance, BULK_OUT, 0, msc_recv_urb)));
        
        if(!(rcv_urb->buffer = (u8 *)CKMALLOC (size, GFP_ATOMIC)) )
        {
            TRACE_MSG0(MSC,"START RECV URB ERROR - CAN'T ALLOCATE URB BUFFER"); 
            usbd_free_urb(rcv_urb);
            return -EINVAL;
        }
        
        rcv_urb->buffer_length = size;
        rcv_urb->alloc_length = size;
        
        rcv_urb->function_privdata = function_instance;
        
        // Loop through all LUNs since we don't know which one is reciving and urb. ??? This look wrong -SR
        for (i = 0; i < msc_maxluns; i++, msc++)
            msc->rcv_urb_finished = NULL;
        
        RETURN_ZERO_UNLESS(usbd_start_out_urb(rcv_urb));
        TRACE_MSG0(MSC,"START RECV URB ERROR"); 
        usbd_free_urb(rcv_urb);
        return -EINVAL;
}

/*!
 * @brief msc_start_recv - queue a receive urb
 *
 * Ensure that size is a multiple of the endpoint packetsize.
 *
 * Returns non-zero if there is an error in the USB layer.
 * @param function_instance
 * @param size
 * @return int
 */
int msc_start_recv_urb_for_write(struct usbd_function_instance *function_instance, int size)
{
        struct usbd_urb *rcv_urb = NULL;
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata);
        int wMaxPacketSize = usbd_endpoint_wMaxPacketSize(function_instance, BULK_OUT, usbd_high_speed(function_instance));
        int i = 0;
        TRACE_MSG0(MSC,"enter msc_start_recv_urb_for_write");
        
        if ((size % wMaxPacketSize)) 
            size = ((size + wMaxPacketSize) / wMaxPacketSize) * wMaxPacketSize;
        
        RETURN_EINVAL_UNLESS((rcv_urb = usbd_alloc_urb (function_instance, BULK_OUT, 0x0, msc_recv_urb_for_write)));

        RETURN_EINVAL_UNLESS((rcv_urb->buffer = (u8 *)__get_free_pages(GFP_KERNEL,get_order(size))));
        rcv_urb->buffer_length = size;
        rcv_urb->alloc_length = (1 << (get_order(size))) * PAGE_SIZE ;
        rcv_urb->actual_length = 0;
        
        TRACE_MSG2(MSC,"Allocationg buffer rcv_urb->buffer: 0x%x, rcv_urb: 0x%x",rcv_urb->buffer,rcv_urb);
        TRACE_MSG2(MSC,"rcv_urb->buffer_length: 0x%x, rcv_urb->alloc_length: 0x%x",rcv_urb->buffer_length,rcv_urb->alloc_length);

        rcv_urb->function_privdata = function_instance;
        
        // Loop through all LUNs since we don't know which one is reciving and urb. ??? This look wrong -SR
        for (i = 0; i < msc_maxluns; i++, msc++)
            msc->rcv_urb_finished = NULL;
        
        RETURN_ZERO_UNLESS(usbd_start_out_urb(rcv_urb));
        TRACE_MSG0(MSC,"START RECV URB ERROR");
        
        free_pages((unsigned long)(rcv_urb->buffer),get_order(size));
        rcv_urb->buffer = NULL;
        usbd_free_urb(rcv_urb);
        return -EINVAL;
}


/*!
 * @brief  msc_start_sending_csw - start sending a new data or csw urb
 *
 * Generate a CSW (Command Status Wrapper) to send to the the host.
 *
 * Returns non-zero if there is an error in the USB layer.
 * @param function_instance
 * @param status
 * @return int
 */
int msc_start_sending_csw(struct usbd_function_instance *function_instance, u8 status)
{
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;
        COMMAND_STATUS_WRAPPER *csw;
        int length = sizeof(COMMAND_STATUS_WRAPPER);
        struct usbd_urb *tx_urb;
        
        TRACE_MSG1(MSC, "Sending csw with LUN %d", msc_cmdlun);
        

        msc->command_state = MSC_STATUS;
        
        RETURN_EINVAL_UNLESS((tx_urb = usbd_alloc_urb (function_instance, BULK_IN, length, msc_urb_sent)));
        
        tx_urb->actual_length = length;
        tx_urb->function_privdata = function_instance;
  
        // fill in CSW and queue the urb
        csw = (COMMAND_STATUS_WRAPPER *) tx_urb->buffer;
        csw->dCSWSignature = CSW_SIGNATURE;
        csw->dCSWTag = msc->command.dCBWTag;
        csw->dCSWDataResidue = msc->command.dCBWDataTransferLength - msc->data_transferred_in_bytes;
        csw->bCSWStatus = status;
        
        TRACE_MSG2(MSC,"START SENDING CSW status: %02x data residue: %d", status, csw->dCSWDataResidue); 
        
        RETURN_ZERO_UNLESS(usbd_start_in_urb (tx_urb));
        TRACE_MSG0(MSC,"START SENDING CSW FAILED");
        usbd_free_urb (tx_urb);
        return -EINVAL;
}


/*!
 * @brief msc_start_sending_csw_failed - starting sending a CSW showing failure
 *
 * Sets sensedata and generates a CSW with status set to USB_MSC_FAILED.
 *
 * Returns non-zero if there is an error in the USB layer.
 * @param function_instance
 * @param sensedata
 * @param info
 * @param status
 * @return int
 */
int msc_start_sending_csw_failed(struct usbd_function_instance *function_instance, u32 sensedata, u32 info, int status)
{
        TRACE_MSG2(MSC, "sensedata: %x status: %x", sensedata, status);
        set_sense_data(function_instance, sensedata, info);
        return msc_start_sending_csw(function_instance, status);
}



/*!
 * @brief msc_alloc_urb - allocate an urb for returning a query
 * @param function_instance
 * @param length
 * @return pointer to a urb, Returns NULL if there is an error in the USB layer.
 
*/
struct usbd_urb * msc_alloc_urb(struct usbd_function_instance *function_instance, int length)
{
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
        struct usbd_urb *urb;
        
        THROW_IF(!(urb = usbd_alloc_urb (function_instance, BULK_IN, length, msc_urb_sent)), error);
        return urb;
        CATCH(error) {
                msc->command_state = MSC_READY;
                return NULL;
        }
}


/*!
 * @brief  msc_dispatch_query_urb - dispatch an urb containing query data
 *
 * @param function_instance
 * @param urb
 * @param length
 * @param status
 * @return int Returns non-zero if there is an error in the USB layer.
 */
int msc_dispatch_query_urb(struct usbd_function_instance *function_instance, struct usbd_urb *urb, int length, int status)
{
        int rc;
        unsigned long flags;
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;
        
        TRACE_MSG2(MSC,"DISPATCH URB len: %d, lun: %d", length, msc_cmdlun);
        
        // save information in msc and urb
        //
        urb->function_privdata = function_instance;
        urb->actual_length = msc->TransferLength_in_bytes = length;
        msc->data_transferred_in_bytes = length;
        
        // dispatch urb
        local_irq_save(flags);
        if ((rc = usbd_start_in_urb (urb))) {
                
                TRACE_MSG0(MSC,"DISPATCH URB FAILED");
                usbd_free_urb (urb);

                // stall?
                msc->command_state = MSC_READY;
                local_irq_restore(flags);
                return -EINVAL;
        }
        
        msc->command_state = MSC_QUERY;
        msc->status = status;
        local_irq_restore(flags);
        return 0;
}

/*!
 * @brief msc_dispatch_query_urb_zlp - send a ZLP
 *
 * @param function_instance
 * @param sensedata
 * @param info
 * @param status
 * @return int Returns non-zero if there is an error in the USB layer.
 */
int msc_dispatch_query_urb_zlp(struct usbd_function_instance *function_instance, u32 sensedata, u32 info, int status)
{
        struct usbd_urb *urb;
        TRACE_MSG2(MSC, "sensedata: %x status: %x", sensedata, status);
        RETURN_EINVAL_IF(!(urb = msc_alloc_urb(function_instance, 1))); 
        set_sense_data(function_instance, sensedata, info);
        urb->flags |= USBD_URB_SENDZLP;
        return msc_dispatch_query_urb(function_instance, urb, 0, status);
}



/*******************************************************
 * FUNCTION:  msc_scsi_vendorspecfic
 *
 * DESCRIPTION:
 *   This function will be called when the driver is being told to
 *   a vendor specific scsi command.  This is called by the msc_recv_command
 *   function.
 *
 *   When called, this function will copy the contents of the command
 *   into a buffer and then wake up any ioctl waiting to recieve the data.
 *
 * ARGUMENTS:
 *   usbd_function_instance * : This is a pointer function instance of the driver
 *   char *                   : This is a pointer to the name of this function
 *   int op                   : This integer shows the opcode of the command
 *
 * RETURN VALUE:
 *   Integer : The sucess of sending a csw
 *
 ********************************************************/
static int msc_scsi_vendorspecific(struct usbd_function_instance *function_instance, char *name, int op) 
{
      struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;

      TRACE_MSG2(MSC, "Recieved a vendor specific command: cmd_pend: %d ioctl_pend:%d", msc->unknown_cmd_pending, msc->unknown_cmd_ioctl_pending);
      if ( msc->unknown_cmd_ioctl_pending) {
	    msc->unknown_cmd_pending = 1;
	    wake_up_interruptible(&msc->ioctl_wq);
	    wait_event_interruptible( msc->ioctl_wq, (msc->unknown_cmd_pending == 0));
	    TRACE_MSG0(MSC, "awake!");

	    return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
      }
     
      return msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_INVALID_COMMAND, msc->lba, USB_MSC_FAILED);
}      


/*!
 * @brief msc_scsi_inquiry - process an inquiry
 *
 * Used by:
 *      win2k
 *
 * @param function_instance
 * @param name 
 * @param op
 * @return int Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_inquiry(struct usbd_function_instance *function_instance, char *name, int op) 
{
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
        SCSI_INQUIRY_COMMAND * command = (SCSI_INQUIRY_COMMAND *)&msc->command.CBWCB;
        SCSI_INQUIRY_DATA *data;
        struct usbd_urb *urb;
        int length = sizeof(SCSI_INQUIRY_DATA);
        int returnlength = MIN ( (int)(msc->command.dCBWDataTransferLength), length );
        
        /*
         * C.f. SPC2 7.3 INQUIRY command
         * C.f. Table 46 - Standard INQUIRY data format
         *
         * C.f. Table 47 - Peripheral Qualifier
         *
         * 000b The specified peripheral device type is currently connected to this
         *      logical unit.....
         * 001b The device server is capable of of supporting the peripheral device
         *      type on this logical unit. However, the physical device is not currently 
         *      connected to this logical unit.....
         * 010b Reserved
         * 011b The device server is not capable of supporting a physical device on
         *      this logical unit....
         *      
         */

        TRACE_MSG5(MSC,"INQUIRY EnableVPD: %02x LogicalUnitNumber: %02x PageCode: %02x AllocLen: %02x, cmdlun: %d",
                   command->EnableVPD, command->LogicalUnitNumber, command->PageCode, command->AllocationLength, msc_cmdlun);

        // XXX THROW_IF(msc->command_state != MSC_READY, error);
        
        RETURN_EINVAL_IF(!(urb = msc_alloc_urb(function_instance, length)));
        
        data = (SCSI_INQUIRY_DATA *)urb->buffer;
        // Always set PeripheralQaulifier to 0 to support linux based pc.
        data->PeripheralQaulifier = 0;
        TRACE_MSG1(MSC,"%d",data->PeripheralQaulifier);
        data->PeripheralDeviceType = 0x00;
        data->RMB = 0x1;
        data->ANSIVersion = 0x04;
        data->ResponseDataFormat = 0x1;
        data->AdditionalLength = 0x1f;
        data->ProductRevisionLevel[0] = 0x31;      //Product Rev:1.01.
        data->ProductRevisionLevel[1] = 0x2E; 
        data->ProductRevisionLevel[2] = 0x30;
        data->ProductRevisionLevel[3] = 0x31;

        strncpy(data->VendorInformation, CONFIG_OTG_MSC_MANUFACTURER, strlen(CONFIG_OTG_MSC_MANUFACTURER));
        strncpy(data->ProductIdentification, CONFIG_OTG_MSC_PRODUCT_NAME, strlen(CONFIG_OTG_MSC_PRODUCT_NAME));
        
        return msc_dispatch_query_urb(function_instance, urb, returnlength, USB_MSC_PASSED);
}


/*!
 * @brief msc_scsi_read_format_capacity - process a query
 *
 * Used by:
 *      win2k
 *
 * @param function_instance
 * @param name
 * @param op
 * @return int Returns non-zero if there is an error in the USB layer.
*/
int msc_scsi_read_format_capacity(struct usbd_function_instance *function_instance, char *name, int op) 
{
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;
        SCSI_READ_FORMAT_CAPACITY_DATA *data;
        struct usbd_urb *urb;
        int length = sizeof(SCSI_READ_FORMAT_CAPACITY_DATA);
        
        u32 block_num = msc->capacity;
        u32 block_len;
        
        TRACE_MSG2(MSC, "cmdlun = %d, capacity: %d", msc_cmdlun, msc->capacity);
        

        RETURN_EINVAL_IF(!(urb = msc_alloc_urb(function_instance, length))); 
        
        data = (SCSI_READ_FORMAT_CAPACITY_DATA *) urb->buffer;
        
        data->CapacityListHeader.CapacityListLength = sizeof(data->CurrentMaximumCapacityDescriptor);
        
        data->CurrentMaximumCapacityDescriptor.NumberofBlocks = block_num;
        data->CurrentMaximumCapacityDescriptor.DescriptorCode = 0x03;
        memcpy(data->CurrentMaximumCapacityDescriptor.BlockLength + 1, &block_len, sizeof(block_len));
        
        return msc_dispatch_query_urb(function_instance, urb, length, USB_MSC_PASSED);
}


/*!
 * @brief msc_read_capacity - process a read_capacity command
 *
 * Used by:
 *      win2k
 *
 * @param function_instance
 * @param name
 * @param op
 * @return int Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_read_capacity(struct usbd_function_instance *function_instance, char *name, int op) 
{
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
        SCSI_READ_CAPACITY_COMMAND *command = (SCSI_READ_CAPACITY_COMMAND *)&msc->command.CBWCB;
        SCSI_READ_CAPACITY_DATA *data;
        struct usbd_urb *urb;
        int length = 8;
        u32 lba;

        /* C.f. RBC 5.3 */
        lba = be32_to_cpu(command->LogicalBlockAddress);
        TRACE_MSG3(MSC,"lba (cmd): %d, lun: %d, capacity: 0x%x", lba, msc_cmdlun, msc->capacity);
        

        /* Sanity Checks */
        THROW_IF( ((command->PMI > 1) || (!command->PMI && lba)), error);
        THROW_IF( ((!msc->connected) || ( msc->capacity - msc->block_size <= 0)), error);
        RETURN_EINVAL_IF(!(urb = msc_alloc_urb(function_instance, length)));


        /* Place information into the urb about to b sent out. */
        data = (SCSI_READ_CAPACITY_DATA *) urb->buffer;

        /*change from capacity to (capacity-1) since LBA starts from 0 and ends at (capacity-1)*/  
        data->LastLogicalBlockAddress = cpu_to_be32(msc->capacity-1);
        data->BlockLengthInBytes = cpu_to_be32(msc->block_size);
        

        /* Send Urb */
        TRACE_MSG2(MSC,"lba (urb): %x block_size: %x", be32_to_cpu(data->LastLogicalBlockAddress), be32_to_cpu(data->BlockLengthInBytes));
        return msc_dispatch_query_urb(function_instance, urb, length, USB_MSC_PASSED);



        CATCH (error) {
                u32 sense_data = SCSI_SENSEKEY_INVALID_FIELD_IN_CDB;
                sense_data = (((!msc->connected) || ( msc->capacity - msc->block_size <= 0))? SCSI_SENSEKEY_MEDIA_NOT_PRESENT : sense_data);
                
                TRACE_MSG4(MSC, "Connected: %d, sense data: 0x%x, PMI: 0x%x, LBA (cmd): 0x%x", msc->connected, sense_data, command->PMI, lba);
                return msc_start_sending_csw_failed (function_instance, sense_data, lba, USB_MSC_FAILED);
        }
}


/*!
 * @brief msc_scsi_request_sense - process a request_sense command
 *
 * @param function_instance
 * @param name
 * @param op
 * @return int Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_request_sense(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
        SCSI_REQUEST_SENSE_COMMAND*    command = (SCSI_REQUEST_SENSE_COMMAND *)&msc->command.CBWCB;
        SCSI_REQUEST_SENSE_DATA *data; 
        struct usbd_urb *urb;
        int length = sizeof(SCSI_REQUEST_SENSE_DATA);       

        /* C.f. SPC2 7.20 REQUEST SENSE command */
        
        // alloc urb
        RETURN_EINVAL_IF(!(urb = msc_alloc_urb(function_instance, length)));

        data = (SCSI_REQUEST_SENSE_DATA *) urb->buffer;
        memset(data, 0x0, length);

        data->ErrorCode = SCSI_ERROR_CURRENT;
        data->SenseKey = msc->sensedata >> 16;
        data->AdditionalSenseLength = 0xa; /* XXX is this needed */
        data->AdditionalSenseCode = msc->sensedata >> 8;
        data->AdditionalSenseCodeQualifier = msc->sensedata;
        data->Valid = 1;
        
        set_sense_data(function_instance, SCSI_SENSEKEY_NO_SENSE, 0);
        
        return msc_dispatch_query_urb(function_instance, urb, length, USB_MSC_PASSED);
}


/*******************************************************
 * FUNCTION: msc_scsi_mode_sense_6
 *
 * DESCRIPTION:
 *   This function handles a mode sense(6) command.  It returns
 *   a header followed by no block descriptors or pages.
 *
 *   The mode sense can return more information that I am currenlty
 *   returning.  See SCSI-2 spec 8.3.3 Table 90 for more information.
 *
 * ARGUMENTS:
 *   function_instance : The function_instance of the mass storage driver
 *   name              : A character pointer to the name of this function
 *   op                : An integer with the op-code for this SCSI command.
 *
 * RETURN VALUE:
 *   int:  0  success
 *   int: <0  failure
 *
 ********************************************************/
int msc_scsi_mode_sense_6(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
        SCSI_MODE_SENSE_6_COMMAND *command = (SCSI_MODE_SENSE_6_COMMAND *)&msc->command.CBWCB;
        MODE_6_PARAMETER_HEADER *data;
        int length = sizeof(MODE_6_PARAMETER_HEADER);
        struct usbd_urb *urb;
        

        /* Error Checking */
        TRACE_MSG3(MSC,"dbd: 0x%x PageControl: 0x%x PageCode: 0x%x", command->DBD, command->PageCode, command->PageControl);
        THROW_IF( (command->PageControl != SCSI_MODESENSE_PAGE_CONTROL_CURRENT), error);


        /* Initialize URB */
        RETURN_EINVAL_IF(!(urb = msc_alloc_urb(function_instance, length)));
        data = (MODE_6_PARAMETER_HEADER *) urb->buffer;
        memset(data, 0x0, length);
        
        
        /* Set data for our device */
        data->ModeDataLength = sizeof(data) - sizeof(data->ModeDataLength);
        data->MediumTypeCode = SCSI_MODESENSE_MEDIUM_TYPE_DEFAULT;
        data->DPOFUA = 0;
        data->WriteProtect = 0;
        data->BlockDescriptorLength = 0;
        
        
        return msc_dispatch_query_urb(function_instance, urb, length, USB_MSC_PASSED);

        CATCH(error){
              TRACE_MSG0(MSC,"Mode Sense Error");
              return msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_INVALID_FIELD_IN_CDB, msc->lba, USB_MSC_FAILED);
        }
}


/*******************************************************
 * FUNCTION: msc_scsi_mode_sense_10
 *
 * DESCRIPTION:
 *   This function handles a mode sense(10) command.  It returns
 *   a header followed by no block descriptors or pages.
 *
 *   The mode sense can return more information that I am currenlty
 *   returning.  See SCSI-2 spec 8.3.3 Table 90 for more information.
 *
 * ARGUMENTS:
 *   function_instance : The function_instance of the mass storage driver
 *   name              : A character pointer to the name of this function
 *   op                : An integer with the op-code for this SCSI command.
 *
 * RETURN VALUE:
 *   int:  0  success
 *   int: <0  failure
 *
 ********************************************************/
int msc_scsi_mode_sense_10(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
        SCSI_MODE_SENSE_10_COMMAND *command = (SCSI_MODE_SENSE_10_COMMAND *)&msc->command.CBWCB;
        MODE_10_PARAMETER_HEADER *data;
        int length = sizeof(MODE_10_PARAMETER_HEADER);
        struct usbd_urb *urb;
        

        /* Error Checking */
        TRACE_MSG3(MSC,"dbd: 0x%x PageControl: 0x%x PageCode: 0x%x", command->DBD, command->PageCode, command->PageControl);
        THROW_IF( (command->PageControl != SCSI_MODESENSE_PAGE_CONTROL_CURRENT), error);


        /* Initialize URB */
        RETURN_EINVAL_IF(!(urb = msc_alloc_urb(function_instance, length)));
        data = (MODE_10_PARAMETER_HEADER *) urb->buffer;
        memset(data, 0x0, length);
        
        
        /* Set data for our device */
        data->ModeDataLength = sizeof(data) - sizeof(data->ModeDataLength);
        data->MediumTypeCode = SCSI_MODESENSE_MEDIUM_TYPE_DEFAULT;
        data->DPOFUA = 0;
        data->WriteProtect = 0;
        data->BlockDescriptorLength = 0;
        
        
        return msc_dispatch_query_urb(function_instance, urb, length, USB_MSC_PASSED);

        CATCH(error){
              TRACE_MSG0(MSC,"Mode Sense Error");
              return msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_INVALID_FIELD_IN_CDB, msc->lba, USB_MSC_FAILED);
        }

}



/*! msc_scsi_test_unit_ready - process a request_sense command
 *
 * Used by:
 *      win2k
 *
 * Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_test_unit_ready(struct usbd_function_instance *function_instance, char *name, int op)
{
        TRACE_MSG0(MSC, "Test Unit Ready");
        return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
}


/*******************************************************
 * FUNCTION: msc_scsi_prevent_allow
 *
 * DESCRIPTION:
 *   This function is called when a prevent/allow media removal command is given.
 *   Since there is nothing our control can do, we return sucess.
 *
 * ARGUMENTS:
 *   function_instance : The function_instance of the mass storage driver
 *   name              : A character pointer to the name of this function
 *   op                : An integer with the op-code for this SCSI command.
 *
 * RETURN VALUE:
 *   int:  0  success
 *   int: <0  failure
 *
 ********************************************************/
int msc_scsi_prevent_allow(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun; 
        SCSI_PREVENT_ALLOW_MEDIA_REMOVAL_COMMAND* command = (SCSI_PREVENT_ALLOW_MEDIA_REMOVAL_COMMAND*)&msc->command.CBWCB;
        
        TRACE_MSG2(MSC, "%s: prevent = %d", name, command->Prevent);
        return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
}


/*! msc_scsi_start_stop - process a start_stop command
 *
 * C.f. RBC 5.4 and 5.4.2
 *
 * Used by:
 *      win2k and Mac OS X
 *
 * Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_start_stop(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun; 
        SCSI_START_STOP_COMMAND* command = (SCSI_START_STOP_COMMAND*)&msc->command.CBWCB;
        
        TRACE_MSG4(MSC,"START STOP: Immed: %d Power: %x LoEj: %d Start: %d", 
                   command->IMMED, command->PowerConditions, command->LoEj, command->Start);
        /*
         * C.f. 5.4 
         *
         * IMMED - if set return status immediately after command validation, otherwise
         * return status as soon operation is completed.
         *
         * C.f. 5.4.1 Table 8 POWER CONDITIONS
         *
         * 0 - M - no change in power condition
         * 1 - M - place device in active condition
         * 2 - M - place device in idle condition
         * 3 - M - place device in Standby condition
         * 4 -   - reserved
         * 5 - M - place device in Sleep condition
         * 6 -   - reserved
         * 7 - 0 - Device Control
         *
         * C.f. 5.4.2 Table 9 START STOP control bit definitions
         *
         * Power        Load/Eject      Start   
         * 1-7          x               x               LoEj and Start Ignored
         * 0            0               0               Stop the medium
         * 0            0               1               Make the medium ready
         * 0            1               0               Unload the medium
         * 0            1               1               Load the medium
         *
         * NOTE: Currently we are handling only last two cases.
         *
         */
        
       if (command->Start && command->LoEj) 
            return msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_INVALID_FIELD_IN_CDB, msc->lba, USB_MSC_FAILED);
        

        /* change the state of the lun */
        if (command->LoEj && command->Start)
        { 
            /* clear EJECTED state */
	    msc->block_dev_state &= ~DEVICE_EJECTED;

#ifdef CONFIG_OTG_MSC_HOTPLUG
            /* Signal to the outside world that the msc is back */
           gen_priv_msc.flags  = HOTPLUG_FUNC_ENABLED;
            generic_os_hotplug(&gen_priv_msc);
#endif
        } 
        else if (command->LoEj && !(command->Start))
        {
            /* close this open block device - sets device ejected */ 
            msc_close_blockdev(function_instance, msc_cmdlun);

            /* one less logical unit, was that the last one? */
            msc_curluns--; 
            if (msc_curluns == 0)
            { 
#ifdef CONFIG_OTG_MSC_HOTPLUG
                /* Remove signal to the outside world that msc is no longer running */
                gen_priv_msc.flags  = ~HOTPLUG_FUNC_ENABLED; 
                generic_os_hotplug(&gen_priv_msc);
#endif
            }
        } 

        return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
}


/*! msc_scsi_verify - process a verify command
 *
 * Used by:
 *      win2k
 *
 * Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_verify(struct usbd_function_instance *function_instance, char *name, int op)
{
        /*
         * C.f. RBC 5.7 VERIFY command
         */
        // XXX This actually should use the read_10 function and when
        // finished reading simply send the following
        return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
}


/*! msc_scsi_mode_select - process a select command
 *
 * Returns non-zero if there is an error in the USB layer.
 */
int msc_scsi_mode_select(struct usbd_function_instance *function_instance, char *name, int op)
{
        /*
         * C.f. SPC2 7.6 MODE SELECT(6) command
         */
        
        // if less than correct amount of data return USB_MSC_PHASE_ERROR - see MV
        //
        return msc_start_sending_csw(function_instance, USB_MSC_PASSED);
}


/*! msc_cmd_unknown - process an unknown command
 *
 * Returns non-zero if there is an error in the USB layer.
 */
int msc_cmd_unknown(struct usbd_function_instance *function_instance, char *name, int op)
{
        struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;
        TRACE_MSG3(MSC, "Unknown command %s (%d) on lun %d", name, op, msc_cmdlun);
        return msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_INVALID_COMMAND, msc->lba, USB_MSC_FAILED);
}


struct rbc_dispatch {
  u8      op;
  char    *name;
  int     (*rbc_command) (struct usbd_function_instance *, char *, int op);
  int     device_check;
};


/*! Command cross reference 
 *
 * This is the list of commands observed from each host OS. It is necessarily
 * incomplete in that we not have reached some condition necessary to have
 * other commands used.
 *                                      Win2k   WinXP
 * SCSI_TEST_UNIT_READY                 yes     yes
 * SCSI_READ_10                         yes     yes
 * SCSI_WRITE_10                        yes     yes
 * SCSI_READ_CAPACITY                   yes     yes
 * SCSI_VERIFY                          yes
 * SCSI_INQUIRY                         yes     yes
 * SCSI_MODE_SENSE                      yes
 * SCSI_READ_FORMAT_CAPACITY            yes     yes
 * SCSI_REQUEST_SENSE                   
 * SCSI_PREVENT_ALLOW_MEDIA_REMOVAL    
 * SCSI_START_STOP                      
 * SCSI_MODE_SELECT                     
 * SCSI_FORMAT_UNIT                     
 *
 */

struct rbc_dispatch rbc_dispatch_table[] = {
  { SCSI_TEST_UNIT_READY,         "SCSI_TEST_UNIT_READY",         msc_scsi_test_unit_ready,               NOZLP },    
  { SCSI_READ_10,                 "SCSI_READ(10)",                msc_scsi_read_10,                       SENDZLP },
  { SCSI_WRITE_10,                "SCSI_WRITE(10)",               msc_scsi_write_10,                      NOZLP },
  { SCSI_READ_CAPACITY,           "SCSI_READ_CAPACITY",           msc_scsi_read_capacity,                 SENDZLP },
  { SCSI_VERIFY,                  "SCSI_VERIFY",                  msc_scsi_verify,                        NOCHK },
  { SCSI_INQUIRY,                 "SCSI_INQUIRY",                 msc_scsi_inquiry,                       NOCHK },
  { SCSI_MODE_SENSE_6,            "SCSI_MODE_SENSE(6)",           msc_scsi_mode_sense_6,                  NOCHK },
  { SCSI_READ_FORMAT_CAPACITY,    "SCSI_READ_FORMAT_CAPACITY",    msc_scsi_read_format_capacity,          SENDZLP },
  { SCSI_REQUEST_SENSE,           "SCSI_REQUEST_SENSE",           msc_scsi_request_sense,                 NOCHK },
  { SCSI_PREVENT_ALLOW_MEDIA_REMOVAL, "SCSI_PREVENT_ALLOW_MEDIA_REMOVAL", msc_scsi_prevent_allow,         NOZLP }, 
  { SCSI_START_STOP,              "SCSI_START_STOP",              msc_scsi_start_stop,                    NOZLP },
  { SCSI_MODE_SELECT_6,           "SCSI_MODE_SELECT(6)",          msc_scsi_mode_select,                   NOCHK },
  { SCSI_FORMAT_UNIT,             "SCSI_FORMAT_UNIT",             msc_cmd_unknown,                        NOCHK },

  { SCSI_READ_6,                  "SCSI_READ(6)",                 msc_cmd_unknown,                        NOCHK },
  { SCSI_WRITE_6,                 "SCSI_WRITE(6)",                msc_cmd_unknown,                        NOCHK },
  { SCSI_RESERVE,                 "SCSI_RESERVE",                 msc_cmd_unknown,                        NOCHK },
  { SCSI_RELEASE,                 "SCSI_RELEASE",                 msc_cmd_unknown,                        NOCHK },
  { SCSI_SEND_DIAGNOSTIC,         "SCSI_SEND_DIAGNOSTIC",         msc_cmd_unknown,                        NOCHK },
  { SCSI_SYNCHRONIZE_CACHE,       "SCSI_SYNCHRONIZE_CACHE",       msc_cmd_unknown,                        NOCHK },
  { SCSI_MODE_SENSE_10,           "SCSI_MODE_SENSE(10)",          msc_scsi_mode_sense_10,                 NOCHK },
  { SCSI_REZERO_UNIT,             "SCSI_REZERO_UNIT",             msc_cmd_unknown,                        NOCHK },
  { SCSI_SEEK_10,                 "SCSI_SEEK(10)",                msc_cmd_unknown,                        NOCHK },
  { SCSI_WRITE_AND_VERIFY,        "SCSI_WRITE_AND_VERIFY",        msc_cmd_unknown,                        NOCHK },
  { SCSI_WRITE_12,                "SCSI_WRITE(12)",               msc_cmd_unknown,                        NOCHK },
  { SCSI_READ_12,                 "SCSI_READ(12)",                msc_cmd_unknown,                        NOCHK },

  { 0xff,                         "SCSI_UNKNOWN",                 msc_cmd_unknown,                        NOCHK },
};


/*! msc_recv_command - process a new CBW
 *
 * Return non-zero if urb was not disposed of.
 */
void msc_recv_command(struct usbd_urb *urb, struct usbd_function_instance *function_instance)
{
        struct msc_private *msc = function_instance->privdata; // not incremenented, b/c command_lun unknown.
        COMMAND_BLOCK_WRAPPER *command = (COMMAND_BLOCK_WRAPPER *)urb->buffer;
        u8 op = command->CBWCB[0];
        struct rbc_dispatch *dispatch;
        msc_cmdlun = command->bCBWLUN;
        msc += msc_cmdlun;
        
        if (msc_cmdlun != command->bCBWLUN)
        {
                TRACE_MSG0(MSC, "Alert! CBWLun != command's LUN!");
                printk("Alert! CBWLun != Command's LUN!");
        }
        
        /*
         * c.f. section 6.2 - Valid and Meaningful CBW
         * c.f. section 6.2.1 - Valid CBW
         *
         * The CBW was received after the device had sent a CSW or after a
         * reset XXX check that we only set MSC_READY after reset or sending
         * CSW.
         *
         * The CBW is 31 (1Fh) bytes in length and the bCBWSignature is
         * equal to 43425355h.
         */
        TRACE_MSG1(MSC,"urb->actual_length:%d", urb->actual_length); 
        THROW_IF(31 != urb->actual_length, error);
        THROW_IF(CBW_SIGNATURE != le32_to_cpu(command->dCBWSignature), error);
        
        /*
         * c.f. section 6.2.2 - Meaningful CBW
         *
         * no reserved bits are set
         * the bCBWLUN contains a valid LUN supported by the device
         * both bCBWCBlength and the content of the CBWCB are in accordance with bInterfaceSubClass
         */
        TRACE_MSG2(MSC, "msc_cmdlun set to %d, comand=0x%x", msc_cmdlun, op);
        // XXX checklun etc
        
        /* 
         * Success 
         */
        memcpy(&msc->command, command, sizeof(COMMAND_BLOCK_WRAPPER));
        msc->data_transferred_in_bytes = msc->TransferLength_in_blocks = msc->TransferLength_in_bytes = 0;
        
        TRACE_TAG(command->dCBWTag, urb->framenum);
        usbd_free_urb(urb);
        urb = NULL;
        
        /*
         * Search using the opcode to find the dispatch function to use and
         * call it.
         */
        for (dispatch = rbc_dispatch_table; dispatch->op != 0xff; dispatch++) {
                CONTINUE_UNLESS ((dispatch->op == op));
                
                TRACE_CBW(dispatch->name, dispatch->op, dispatch->device_check);
                TRACE_RECV(&(command->CBWCB[1]));
                
                /* Depending on the command we may need to check if the device is available
                 * and either fail or send a ZLP if it is not
                 */
                if (dispatch->device_check) 
                    RETURN_IF (msc_check_blockdev_name(function_instance, dispatch->device_check, dispatch->name));  
                
                /* Call the specific function that implements the specified command
                 */
                if (dispatch->rbc_command(function_instance, dispatch->name, op)) 
                    TRACE_MSG0(MSC,"COMMAND ERROR");
                return;
        }
        
        // If command wasn't on the known list, treat it as a vendor specific.
        if (msc_scsi_vendorspecific(function_instance, "Vendor Specific", op))
            TRACE_MSG0(MSC, "Vendor Specific Command Error!");
        return;
        
        
        CATCH(error) {
                TRACE_MSG1(MSC,"Error: urb->actual_length: %d", ((urb)?urb->actual_length:0));
                TRACE_MSG1(MSC,"--le32_to_cpu(command->dCBWSignature):%08X", le32_to_cpu(command->dCBWSignature));
        
                if (urb)
                    usbd_free_urb(urb);
                
                /* XXX which of these do we stall?
                 */
                
                TRACE_MSG0(MSC,"sos88 halt endpoint");
                usbd_halt_endpoint(function_instance, BULK_IN);
                usbd_halt_endpoint(function_instance, BULK_OUT);
                msc->endpoint_state=3;
                msc->command_state=MSC_WAITFOR_RESET;
        }
}


/* Sent Function - process a sent urb ********************************************************** */

/*! msc_urb_sent - called to indicate URB transmit finished
 * @param tx_urb: pointer to struct usbd_urb
 * @param rc: result
 *
 * This is called when an urb is sent. Depending on current state
 * it may:
 *
 *      - continue sending data
 *      - send a CSW
 *      - start a recv for a CBW
 *
 * This is called from BOTTOM HALF context.
 *
 * @return non-zero if urb was not disposed of.
 */
int msc_urb_sent (struct usbd_urb *tx_urb, int rc)
{
      struct usbd_function_instance *function_instance = tx_urb->function_privdata;
      struct msc_private *msc = ((struct msc_private *)function_instance->privdata) + msc_cmdlun;
      

      RETURN_EINVAL_IF(!(function_instance = tx_urb->function_instance));
      RETURN_EINVAL_IF(usbd_get_device_status(function_instance) == USBD_CLOSING);
      RETURN_EINVAL_IF(usbd_get_device_state(function_instance) != STATE_CONFIGURED);
      
      TRACE_MSG1(MSC, "Sending URB on LUN %d", msc_cmdlun);
      
      switch (msc->command_state) {
	  case MSC_DATA_IN_READ:
	  case MSC_DATA_IN_READ_FINISHED:
		TRACE_MSG0(MSC,"URB SENT READ");
		return msc_in_read_10_urb_sent(tx_urb);
		
	  case MSC_QUERY:
		// finished, send CSW
		TRACE_MSG0(MSC,"URB SENT QUERY");
		msc_start_sending_csw(function_instance, msc->status);
		break;
		
	  case MSC_STATUS:
	  default:
		// sent a CSW need to receive the next CBW
		TRACE_MSG0(MSC,"URB SENT STATUS");
		msc->command_state = MSC_READY;
		msc_start_recv_urb(function_instance, sizeof(COMMAND_BLOCK_WRAPPER));
		break;
      }
      usbd_free_urb (tx_urb);
      return 0;
}


/* Receive Function - receiving an urb ********************************************************* */

/*! msc_recv_urb - process a received urb
 *
 * Return non-zero if urb was not disposed of.
 */
static int msc_recv_urb (struct usbd_urb *rcv_urb, int rc)
{
      struct usbd_function_instance *function_instance = rcv_urb->function_privdata;
      struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;
      u32 recv_urb_flag;
      //we clear recv_urb flag here, since if we got call here, it should be from an recv urb
      local_irq_save(recv_urb_flag);
      msc_recv_urb_already=0;
      local_irq_restore(recv_urb_flag);
      RETURN_EINVAL_IF(!msc->connected);
      TRACE_MSG2(MSC, "RECV URB length: %d state: %d", rcv_urb->actual_length, msc->command_state);
      if(rc!=0) return -EINVAL; //XXX if urb callback with error, now do nothing
  
      switch(msc->command_state) {

	  // ready to start a new transaction
	  case MSC_READY:
		msc_recv_command(rcv_urb, function_instance);
		return 0;
		
		// we think we are receiving data
	  case MSC_DATA_OUT_WRITE:
	  case MSC_DATA_OUT_WRITE_FINISHED:
          case MSC_DATA_OUT_WRITE_ERROR:
                TRACE_MSG0(MSC, "Writing");
		msc_recv_out_blocks(rcv_urb);
		return 0;
		
		// we think we are sending data
	  case MSC_DATA_IN_READ:
	  case MSC_DATA_IN_READ_FINISHED:
                TRACE_MSG0(MSC, "Read error");
		msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_INVALID_COMMAND, msc->lba, USB_MSC_FAILED);
		break;

		// we think we are sending status
	  case MSC_STATUS:
		msc_start_sending_csw_failed (function_instance, SCSI_SENSEKEY_INVALID_COMMAND, msc->lba, USB_MSC_FAILED);
		break;

	  case MSC_WAITFOR_RESET:
		usbd_halt_endpoint(function_instance, BULK_IN);
		usbd_halt_endpoint(function_instance, BULK_OUT);
		msc->endpoint_state=3;
		break;
		
		// we don't think
	  case MSC_UNKNOWN:
	  default:
		TRACE_MSG0(MSC,"RECV URB ERROR");
		TRACE_MSG0(MSC,"sos 89 halt points");
		usbd_halt_endpoint(function_instance, BULK_IN);
		usbd_halt_endpoint(function_instance, BULK_OUT);
		msc->endpoint_state=3;
		msc->command_state=MSC_WAITFOR_RESET;
      }
      // let caller dispose of urb
      return -EINVAL;
}

/* Receive Function - receiving an urb ********************************************************* */

/*! msc_recv_urb_for_write - process a received urb_for_write
 *
 * Return non-zero if urb was not disposed of.
 */
static int msc_recv_urb_for_write (struct usbd_urb *rcv_urb, int rc)
{
    struct usbd_function_instance *function_instance = rcv_urb->function_privdata;
    struct msc_private *msc = ((struct msc_private *) function_instance->privdata) + msc_cmdlun;

    // Check if device is connected or not. If not, then clean up the buffer of current urb's
    // buffer and then return invalid. Also check if the URB is cancelled. 
    if(!msc->connected || (rc == USBD_URB_CANCELLED))
    {
        if( rcv_urb != NULL &&  rcv_urb->buffer != NULL)
        {
            free_pages(rcv_urb->buffer,get_order(rcv_urb->alloc_length));
            rcv_urb->buffer = NULL;
        }
    }

    return msc_recv_urb (rcv_urb,rc);
}

/* USB Device Functions ************************************************************************ */

/*! msc_device_request - called to indicate urb has been received
 *
 * This function is called when a SETUP packet has been received that
 * should be handled by the function driver. It will not be called to
 * process the standard chapter nine defined requests.
 *
 * Return non-zero for failure.
 */
int msc_device_request(struct usbd_function_instance *function_instance, struct usbd_device_request *request)
{
      struct msc_private *msc = function_instance->privdata;
      struct usbd_urb *urb;
      int count;
      
      u8 bRequest = request->bRequest;
      u8 bmRequestType = request->bmRequestType;
      u16 wValue = le16_to_cpu(request->wValue);
      u16 wIndex = le16_to_cpu(request->wIndex);
      u16 wLength = le16_to_cpu(request->wLength);
      
      TRACE_MSG5(MSC, "MSC RECV SETUP bmRequestType:%02x bRequest:%02x wValue:%04x wIndex:%04x wLength:%04x",
		 bmRequestType, bRequest, wValue, wIndex, wLength);
      
      // verify that this is a usb class request per cdc-acm specification or a vendor request.
      RETURN_ZERO_IF (!(request->bmRequestType & (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_VENDOR)));
      
      // determine the request direction and process accordingly
      switch (request->bmRequestType & (USB_REQ_DIRECTION_MASK | USB_REQ_TYPE_MASK)) {
	  
	  case USB_REQ_HOST2DEVICE | USB_REQ_TYPE_CLASS:
		switch (request->bRequest) {
		    case MSC_BULKONLY_RESET:
			  TRACE_MSG0(MSC, "MSC_BULKONLY_RESET");
			  RETURN_EINVAL_IF((wValue !=0) | (wIndex !=0) | ( wLength !=0));
			  // set msc back to reset state here
			  //.....
			  for (count = 0; count < msc_maxluns; count++, msc++)
			  {
			      msc->command_state = MSC_WAITFOR_CLEAR;
			      msc->endpoint_state=3;
			  }
			  //////////////////////////
			  return 0;
		    default:
			  TRACE_MSG0(MSC, "UNKNOWN H2D");
			  return -EINVAL;
		
		}

	  case USB_REQ_DEVICE2HOST | USB_REQ_TYPE_CLASS:
		switch (request->bRequest) {
		    case MSC_BULKONLY_GETMAXLUN: 
			  RETURN_EINVAL_IF((wValue !=0) | (wIndex !=0) | ( wLength !=1));
			  RETURN_EINVAL_IF(!(urb = usbd_alloc_urb_ep0(function_instance, 1, NULL)));
			  urb->buffer[0] = msc_maxluns - 1;
			  TRACE_MSG1(MSC, "maxlun's buffer[0] = %d", urb->buffer[0]);
			  urb->actual_length = 1;
			  RETURN_ZERO_IF(!usbd_start_in_urb(urb));
			  TRACE_MSG0(MSC, "URB failed while trying to send max luns.");
			  usbd_free_urb(urb);
			  return -EINVAL;
		    default:
			  TRACE_MSG0(MSC, "UNKNOWN D2H");
			  return -EINVAL;
		}
	  default: 
		TRACE_MSG0(MSC,"unknown MSC device request");
		return -EINVAL;
		break;
      }
      return -EINVAL;
}


/*! msc_function_enable - this is called by the USBD core layer 
 *
 * This is called to initialize the function when a bus interface driver
 * is loaded.
 */
static int msc_function_enable (struct usbd_function_instance *function_instance)
{
      struct msc_private *msc = NULL;
      int count = 0;
      unsigned long flags;
      
      RETURN_EINVAL_UNLESS((msc = CKMALLOC((msc_maxluns * sizeof(struct msc_private)), GFP_KERNEL)));
      
      // XXX MODULE LOCK HERE
      msc_recv_urb_already = 0;   
      
      function_instance->privdata = msc;
      
      // XXX TODO need to verify that serial number is minimum of 12 
      
      local_irq_save(flags);
      for (count = 0; count < msc_maxluns; count ++, msc++)
      {
	  init_waitqueue_head(&msc->rdwr_wq);
	  init_waitqueue_head(&msc->ioctl_wq);
	  
	  msc->block_dev_state = DEVICE_EJECTED;
	  msc->command_state = MSC_READY;
	  msc->io_state = MSC_INACTIVE;
	  msc->command_state = MSC_READY;
	  msc->unknown_cmd_pending = 0;
	  msc->unknown_cmd_ioctl_pending=0;
	  msc->close_pending = 0;  
	  msc_io_init_l24(function_instance);
      }

      local_irq_restore(flags);

      // If hotplug is enabled, then we need to init the script!
#ifdef CONFIG_OTG_MSC_HOTPLUG
      gen_priv_msc.function_instance = function_instance;
      gen_priv_msc.dev_name = MSC_AGENT;
      hotplug_init(&gen_priv_msc);
#endif

      return 0;
}


/*! msc_function_disable - this is called by the USBD core layer 
 *
 * This is called to close the function when a bus interface driver
 * is unloaded.
 */
static void msc_function_disable (struct usbd_function_instance *function_instance)
{               
      struct msc_private *msc = function_instance->privdata;  // Correct, to be freed.

      TRACE_MSG0(MSC,"FUNCTION EXIT");
      
      // destroy control io interface
      msc_io_exit_l24(function_instance);
      
      LKFREE(msc);          // Free memory used by the msc device
      
      // Set the private data to NULL.
      function_instance->privdata = NULL;


#ifdef CONFIG_OTG_MSC_HOTPLUG
      while (PENDING_WORK_ITEM(gen_priv_msc.hotplug_bh)) {
	  printk(KERN_ERR"%s: waiting for MSC hotplug bh\n", __FUNCTION__);
	  schedule_timeout(10 * HZ);
  }
#endif

      // XXX MODULE UNLOCK HERE
}


/*! 
 * @brief msc_set_configuration - called to indicate urb has been received
 * @param function_instance
 * @param configuration
 * @return int 
 */             
int msc_set_configuration (struct usbd_function_instance *function_instance, int configuration)
{               
      struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
      struct msc_private *msc = function_instance->privdata;  // correct, set later in function
      int i = 0;
      
      TRACE_MSG0(MSC, "CONFIGURED");
      
      // XXX Need to differentiate between non-zero, zero and non-zero done twice
      
      TRACE_MSG0(MSC,"EVENT CONFIGURED");
      for (i = 0; i < msc_maxluns; i++, msc++)
      {
	  msc->endpoint_state=0;
	  msc->connected = 1;
	  msc->command_state = MSC_READY;

	  // If there are any ioctls waiting for connect.
	  wake_up_interruptible(&msc->ioctl_wq);
      }
      msc_start_recv_urb(function_instance, sizeof(COMMAND_BLOCK_WRAPPER));
      
      
      msc = (struct msc_private *)function_instance->privdata;
      for (i =0 ; i < msc_curluns; i++, msc++)
	  wake_up_interruptible(&msc->ioctl_wq);
      
#ifdef CONFIG_OTG_MSC_HOTPLUG
      gen_priv_msc.flags  = HOTPLUG_FUNC_ENABLED;
      generic_os_hotplug(&gen_priv_msc);
#endif
      return 0;
}       


/*! 
 * @brief msc_reset - called to indicate urb has been received
 * @param function_instance
 * @return int
 */
int msc_reset (struct usbd_function_instance *function_instance)
{
      struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
      struct msc_private *msc = function_instance ? function_instance->privdata : NULL;
      int connected = msc ? msc->connected : 0;
      int i = 0;
      unsigned long flags;
      
      TRACE_MSG1(MSC,"msc: %x", (int)msc);
      
      if (msc == NULL) return 0;
      
      TRACE_MSG1(MSC,"EVENT RESET: connected %d", msc->connected);

      for (i = 0; i < msc_curluns; i++, msc++)
      {
              msc->connected = 0;
              msc->close_pending = 1;
              // Wake up any ioctls waiting for a command. (WAIT DISCONNECT)
              wake_up_interruptible(&msc->ioctl_wq);
              RETURN_ZERO_UNLESS(connected);
              // XXX we should have a semaphore to protect this 
              if(msc->rcv_urb_finished == NULL) {
                  TRACE_MSG0(MSC,"recv_urb is NULL");
                  continue;
              }
              usbd_free_urb (msc->rcv_urb_finished);
              msc->rcv_urb_finished = NULL;
              msc->io_state=0;
      }
      
      /* close down any open block devices */
      msc_close_all_blockdev(function_instance);

#ifdef CONFIG_OTG_MSC_HOTPLUG
      local_irq_save(flags);
      if (gen_priv_msc.flags != ~HOTPLUG_FUNC_ENABLED)
      {
        gen_priv_msc.flags = ~HOTPLUG_FUNC_ENABLED;
        generic_os_hotplug(&gen_priv_msc);
      }
      else
      {
        TRACE_MSG0(MSC,"msc disable is ongoing: do not restart hotplug event\n");
      }
      local_irq_restore(flags);
#endif
      
      return 0;
}

/*******************************************************
 * FUNCTION:  msc_suspended
 *
 * DESCRIPTION:
 *   This function will be called when the driver is being told, by the 
 *   host to suspend.  This function will close all the block devices
 *   and then use hotplug to signal that the msc is no longer being
 *   used.
 *
 * ARGUMENTS:
 *   usbd_function_instance * : This is a pointer function instance of the driver
 *
 * RETURN VALUE:
 *   Integer :  0 on sucess.
 *
 ********************************************************/
int msc_suspended (struct usbd_function_instance *function_instance)
{
      struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
      struct msc_private *msc = function_instance ? function_instance->privdata : NULL;
      int connected = msc ? msc->connected : 0;
      int i = 0;

      TRACE_MSG1(MSC,"msc: %x", (int)msc);

      if (msc == NULL) return 0;

      TRACE_MSG0(MSC, "Suspending");
     
      for (i = 0; i < msc_curluns; i++, msc++)
      {
              msc->connected = 0;
              msc->close_pending = 1;
              // Wake up any ioctls waiting for a command. (WAIT DISCONNECT)
              wake_up_interruptible(&msc->ioctl_wq);
              RETURN_ZERO_UNLESS(connected);
              // XXX we should have a semaphore to protect this
              if(msc->rcv_urb_finished == NULL) {
                  TRACE_MSG0(MSC,"recv_urb is NULL");
                  continue;
              }
	      free_pages(msc->rcv_urb_finished->buffer, get_order(msc->rcv_urb_finished->alloc_length));
	      msc->rcv_urb_finished->buffer = NULL;
              usbd_free_urb (msc->rcv_urb_finished);
              msc->rcv_urb_finished = NULL;
              msc->io_state = MSC_INACTIVE;
      }
 
      /* close down any open block devices */
      msc_close_all_blockdev(function_instance);

#ifdef CONFIG_OTG_MSC_HOTPLUG
      /* Remove signal to the outside world that msc is no longer running */
      gen_priv_msc.flags  = ~HOTPLUG_FUNC_ENABLED; 
      generic_os_hotplug(&gen_priv_msc);
#endif

      TRACE_MSG0(MSC, "Suspended");

      return 0;
}

/*******************************************************
 * FUNCTION:  msc_resumed
 *
 * DESCRIPTION:
 *   This function will be called when the driver is being told, by the 
 *   host, to resume. This function uses hotplug to signal to the outside
 *   world that the msc is back.
 *
 * ARGUMENTS:
 *   usbd_function_instance * : This is a pointer function instance of the driver
 *
 * RETURN VALUE:
 *   Integer :  0 on sucess.
 *
 ********************************************************/
int msc_resumed (struct usbd_function_instance *function_instance)
{
      struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
      struct msc_private *msc = function_instance->privdata;  // correct, set later in function
      int i = 0;

      TRACE_MSG0(MSC, "RESUMED");

      for (i = 0; i < msc_maxluns; i++, msc++)
      {
          msc->endpoint_state=0;
          msc->connected = 1;
          msc->command_state = MSC_READY;
          msc->close_pending = 0;

          // If there are any ioctls waiting for connect.
          wake_up_interruptible(&msc->ioctl_wq);
      }
      msc_start_recv_urb(function_instance, sizeof(COMMAND_BLOCK_WRAPPER));

#ifdef CONFIG_OTG_MSC_HOTPLUG
      /* Signal to the outside world that the msc is back */
      gen_priv_msc.flags  = HOTPLUG_FUNC_ENABLED;
      generic_os_hotplug(&gen_priv_msc);
#endif

      TRACE_MSG0(MSC, "Resume");

      return 0;
}

/*******************************************************
 * FUNCTION:  msc_close_all_blockdev
 *
 * DESCRIPTION:
 *   This function will close all the open block devices
 *
 * ARGUMENTS:
 *   usbd_function_instance * : This is a pointer function instance of the driver
 *
 * RETURN VALUE:
 *   Integer :  0 on sucess.
 *
 ********************************************************/
static void msc_close_all_blockdev (struct usbd_function_instance *function_instance)
{
      int i;

      /* flush io and free page buffers */
      for (i = 0; i < msc_curluns; i++)
	  msc_close_blockdev(function_instance, i);

      msc_curluns = 0;      /* Reset the number of current LUNS. */
}

/*! 
 * @brief msc_endpoint_cleared- called to indicate endpoint has been cleared
 * @param function_instance
 * @param bEndpointAddress
 * @return none
 */
static void msc_endpoint_cleared (struct usbd_function_instance *function_instance, int bEndpointAddress)
{
      struct usbd_interface_instance *interface_instance = (struct usbd_interface_instance *)function_instance;
      struct msc_private *msc = ((struct msc_private *)function_instance->privdata); 
      int i = 0;
      u32 recv_urb_flag,all_ep_state=0;
      u8 recv_urb_alr;
      TRACE_MSG1(MSC,"bEndpointAddress: %02x", bEndpointAddress);
 
     
      for (i = 0; i < msc_maxluns; i++, msc++)
      {
            TRACE_MSG2(MSC, "lun %d is in state 0x%x", i, msc->command_state);

            switch (msc->command_state) {
                  
                  case MSC_WAITFOR_RESET:
                       usbd_halt_endpoint(function_instance, BULK_IN);
                       usbd_halt_endpoint(function_instance, BULK_OUT);
                       msc->endpoint_state=BULK_IN_HALTED | BULK_OUT_HALTED;
                       break;
                       
                  case MSC_WAITFOR_CLEAR:
                       if(bEndpointAddress & 0x80) {
                             msc->endpoint_state &= ~BULK_IN_HALTED;     
                             TRACE_MSG0(MSC,"Bulk IN cleared");
                       }
                       else { 
                             msc->endpoint_state &= ~BULK_OUT_HALTED;
                             TRACE_MSG0(MSC,"Bulk OUT cleared");
                       }
                       
                       if(!msc->endpoint_state){
                             msc->command_state = MSC_READY;
                             all_ep_state=1; //prepare recv urb when either msc is ready
                             TRACE_MSG0(MSC,"Ready to receive a new CBW");
                       } 
                       break;
                       
                  default:
                       break;
            }
      }
    
      local_irq_save(recv_urb_flag);   
      recv_urb_alr=msc_recv_urb_already;
      local_irq_restore(recv_urb_flag); 
      if(!recv_urb_alr && all_ep_state)// we only issue this recv urb when there is no any pending recv urb already.
          msc_start_recv_urb(function_instance, sizeof(COMMAND_BLOCK_WRAPPER));
      
}




/* ********************************************************************************************* */
struct usbd_function_operations msc_function_ops = {
  device_request: msc_device_request,
  function_enable: msc_function_enable,
  function_disable: msc_function_disable,
  set_configuration: msc_set_configuration,
  reset: msc_reset,
  suspended: msc_suspended,
  resumed: msc_resumed,
  endpoint_cleared: msc_endpoint_cleared,
};

/* ********************************************************************************************* */

