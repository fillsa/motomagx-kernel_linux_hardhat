/*
 *  dm500.c - SPI Driver for Texas Instruments DM500 ISP chip
 *
 *  Copyright (C) 2007-2008 Motorola, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  DM500 ISP chip connects to CSPI1 interface on Kassos P2 HW.
 *  This provides the SPI interface to a user space application that is used
 *  to download program from Argon to DM500 during bootup and to transfer encoded/
 *  decoded frame between Argon and DM500 chip during video playback/record afterwards
 *  on Kassos P2 HW.
 */

/*
 *  DATE         AUTHOR     COMMENT
 *  ----         ------     -------
 *  05/30/2007   Motorola   Initial version
 *  11/15/2007   Motorola   Add Pearl support
 *  12/06/2007   Motorola   Set CSPI SS according to product 
 *  01/30/2008   Motorola   Add IOCTL to set SPI clock  
 *  06/02/2008   Motorola   Add CSPI clock gating
 */

#include <linux/config.h>
#include "dm500.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <asm/arch/clock.h>

/*==================================================================================================
                                     MEMORY MAPPED PERIPHERALS
==================================================================================================*/

static volatile struct
{
   u32 rxdata;         // Receive Data Register
   u32 txdata;         // Transmit Data Register
   u32 conreg;         // Control Register
   u32 intreg;         // Interrupt Control Register
   u32 dmareg;         // DMA Control Register
   u32 statreg;        // Status Register
   u32 periodreg;      // Sample Period Control
   u32 testreg;        // Test Control Register
#ifdef CONFIG_MACH_PEARL 
} *const cspi = (void *)IO_ADDRESS(CSPI2_BASE_ADDR);
#else
} *const cspi = (void *)IO_ADDRESS(CSPI1_BASE_ADDR);
#endif

/*==================================================================================================
                                          LOCAL CONSTANTS
==================================================================================================*/

/* CSPI CONREG (Control Register) bit definitions */
#define SSPOL    0x00000080  // Slave Select Polarity (0 = active low, 1 = active high)
#define SSCTL    0x00000040  // Slave Select Waveform Control
#define PHA      0x00000020  // Clock/Data Phase
#define POL      0x00000010  // Clock Polarity (0 = idle low, 1 = idle high)
#define SMC      0x00000008  // Start Mode Control
#define XCH      0x00000004  // Exchange Bit
#define MODE     0x00000002  // Mode Select (0 = Slave, 1 = Master)
#define EN       0x00000001  // Enable Bit

/* CSPI INTREG (Interrupt Control Register) bit definitions */
#define TCEN     0x00000080  // Transfer Completed  Interrupt Enable
#define ROEN     0x00000040  // RXFIFO Overflow     Interrupt Enable
#define RFEN     0x00000020  // RXFIFO Full         Interrupt Enable
#define RHEN     0x00000010  // RXFIFO Half Full    Interrupt Enable
#define RREN     0x00000008  // RXFIFO Ready        Interrupt Enable
#define TFEN     0x00000004  // TXFIFO Full         Interrupt Enable
#define THEN     0x00000002  // TXFIFO Half Empty   Interrupt Enable
#define TEEN     0x00000001  // TXFIFO Empty        Interrupt Enable

/* CSPI STATREG (Status Register) bit definitions */
#define TC       0x00000080  // Transfer Completed
#define RO       0x00000040  // RXFIFO Overflow
#define RF       0x00000020  // RXFIFO Full
#define RH       0x00000010  // RXFIFO Half Full
#define RR       0x00000008  // RXFIFO Ready
#define TF       0x00000004  // TXFIFO Full
#define TH       0x00000002  // TXFIFO Half Empty
#define TE       0x00000001  // TXFIFO Empty

/*==================================================================================================
                                          LOCAL TYPEDEFS
==================================================================================================*/

typedef enum
{
   AIM_READ,
   AIM_WRITE
} AIM_TYPE;  // ARM Internal Memory transfer type

typedef struct
{
   u8  full_burst_bits;       // bits transferred on SPI bus during a full burst
   u8  full_burst_bytes;      // bytes of data transferred during a full burst
   u8  full_burst_words;      // words written to FIFO during a full burst
   u8  partial_burst_bits;    // bits transferred on SPI bus during a partial burst
   u8  partial_burst_bytes;   // bytes of data transferred during a partial burst
   u8  partial_burst_words;   // words written to FIFO during a partial burst
   u32 full_bursts;           // number of full bursts that need to be transferred
   u32 *buffer;               // list of words that we transfer to/from FIFOs
   u32 *read_ptr;             // buffer slot where we will read next word for TXFIFO
   u32 *write_ptr;            // buffer slot where we will write next word from RXFIFO
   u8  words_queued;          // words written to CSPI minus words read from CSPI
   u8  transaction_complete;  // flag that indicates SPI transaction is complete
} spi_desc;  // SPI transfer descriptor

/*==================================================================================================
                                     LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

static void aim_depopulate_buffer(spi_desc *desc, u8 *kbuf, u32 len);
static spi_desc *aim_generate_descriptor(u32 len);
static void aim_populate_buffer(spi_desc *desc, u8 *kbuf, u32 len, u32 base, AIM_TYPE type);
static void burst_full(void);
static void burst_partial(void);
static irqreturn_t cspi_isr(int irq, void *dev_id, struct pt_regs *regs);
static void dm500_exit(void);
static int  dm500_init(void);
static int  dm500_ioctl(struct inode *inode, struct file *filp,
                        unsigned int cmd, unsigned long arg);
static int  dm500_open(struct inode *inode, struct file *filp);
static int  dm500_release(struct inode *inode, struct file *filp);
static void fifo_read(u32 *ptr, u8 num_words);
static void fifo_write(u32 *ptr, u8 num_words);
static void handle_rxfifo_ready(void);
static void handle_spurious(void);
static void handle_transfer_completed(void);
static void handle_txfifo_empty(void);
static void msg_depopulate_buffer(spi_desc *desc, u8 *kbuf, u32 len);
static spi_desc *msg_generate_descriptor(u32 len);
static void msg_populate_buffer(spi_desc *desc, u8 *kbuf, u32 len);
static int  perform_transfer(spi_desc *desc);
static int  spi_transfer_aim(u8 *kbuf, u32 len, u32 base, AIM_TYPE type);
static int  spi_transfer_msg(u8 *kbuf, u32 len);

/*==================================================================================================
                                          LOCAL VARIABLES
==================================================================================================*/

static dev_t devno;
static struct cdev cdev;
/* in dm299 
static struct file_operations dmcam_fops = {
	.read    = dmcam_read,
	.write   = dmcam_write,
	.open    = dmcam_open,
	.release = dmcam_release,
	.ioctl   = dmcam_ioctl,
};*/
static struct file_operations fops =
{
   .open    = dm500_open,
   .release = dm500_release,
   .read    = NULL,
   .write   = NULL,
   .ioctl   = dm500_ioctl,
};

static DECLARE_WAIT_QUEUE_HEAD(spi_wait);
static void (*handler)(void) = handle_spurious;
static spi_desc *spid;  // pointer used by interrupt handlers to access descriptor
static int spiclk_divider = 1;
// 0: 	16MHz spi clk for app firware download and mesg transfer;
// 1:	8 MHz spi clk for dm500 bootloader download(default); 


/*==================================================================================================
                                          LOCAL FUNCTIONS
==================================================================================================*/
/*==================================================================================================
  
FUNCTION: aim_depopulate_buffer

DESCRIPTION:
   This function depopulates the buffer that contains the words we read out of the
   RXFIFO.  The padding, R/W bits, and address bits will be stripped out and the
   remaining byte stream will be written to the output buffer.

ARGUMENTS PASSED:
   desc - pointer to descriptor
   kbuf - output buffer
   len  - length of buffer

RETURN VALUE:

==================================================================================================*/
static void aim_depopulate_buffer(spi_desc *desc, u8 *kbuf, u32 len)
{
   u32 bytes_left = len;
   u8 *sptr = (u8 *)desc->buffer;
   u8 *kptr = kbuf;
   u32 data_bytes, empty_bytes, i;

   while (bytes_left)
   {
      if (bytes_left >= desc->full_burst_bytes)
      {
         data_bytes = desc->full_burst_bytes;
         empty_bytes = 0;
      }
      else
      {
         data_bytes = bytes_left;
         empty_bytes = (desc->partial_burst_words * 32 - desc->partial_burst_bits) / 8;
      }

      sptr += empty_bytes + 3;  // 3 bytes for R/W bit and address bits

      for (i = 0; i < data_bytes; i++) { *kptr++ = *sptr++; }

      bytes_left -= data_bytes;
   }
}

/*==================================================================================================
  
FUNCTION: aim_generate_descriptor

DESCRIPTION:
   This function generates a SPI transaction descriptor.  It allocates memory for it and
   initializes the fields.

ARGUMENTS PASSED:
   len - the length of the transfer this descriptor will specify

RETURN VALUE:
   desc - pointer to the descriptor (NULL if we could not allocate one)

==================================================================================================*/
static spi_desc *aim_generate_descriptor(u32 len)
{
   spi_desc *desc;
   u32 total_words;

   desc = kmalloc(sizeof(spi_desc), GFP_KERNEL);
   if (desc == NULL) { return NULL; }

   desc->full_burst_words = 7;
   desc->full_burst_bytes = desc->full_burst_words * 4 - 3;
   desc->full_burst_bits = 1 + 16 + desc->full_burst_bytes * 8;
   desc->full_bursts = len / desc->full_burst_bytes;

   desc->partial_burst_bytes = len - desc->full_bursts * desc->full_burst_bytes;

   if (desc->partial_burst_bytes)
   {
      desc->partial_burst_bits = 1 + 16 + desc->partial_burst_bytes * 8;
      desc->partial_burst_words = (desc->partial_burst_bits + 31) / 32;
   }
   else
   {
      desc->partial_burst_bits = 0;
      desc->partial_burst_words = 0;
   }

   total_words = desc->full_bursts * desc->full_burst_words + desc->partial_burst_words;

   desc->buffer = kmalloc(total_words * 4, GFP_KERNEL);
   if (desc->buffer == NULL) { kfree(desc); return NULL; }

   desc->write_ptr = desc->read_ptr = desc->buffer;
   desc->words_queued = 0;
   desc->transaction_complete = 0;

   return(desc);
}

/*==================================================================================================

FUNCTION: aim_populate_buffer

DESCRIPTION:
   This function populates the buffer that contains the words we will write to the
   TXFIFO.  We will read the raw byte stream from the input buffer and insert the
   necessary padding, R/W bits, and address bits.

ARGUMENTS PASSED:
   desc - pointer to descriptor
   kbuf - input buffer
   len  - length of buffer
   base - base address of AIM we will be reading or writing
   type - AIM_READ or AIM_WRITE

RETURN VALUE:

==================================================================================================*/
static void aim_populate_buffer(spi_desc *desc, u8 *kbuf, u32 len, u32 base, AIM_TYPE type)
{
   u32 bytes_left = len;
   u32 aim_addr = base;
   u8 *sptr = (u8 *)desc->buffer;
   u8 *kptr = kbuf;
   u32 data_bytes, empty_bytes, i;

   while (bytes_left)
   {
      if (bytes_left >= desc->full_burst_bytes)
      {
         data_bytes = desc->full_burst_bytes;
         empty_bytes = 0;
      }
      else
      {
         data_bytes = bytes_left;
         empty_bytes = (desc->partial_burst_words * 32 - desc->partial_burst_bits) / 8;
      }

      for (i = 0; i < empty_bytes; i++) { *sptr++ = 0x00; }

      *sptr++ = (type == AIM_READ) ? (0x01) : (0x00);  // 7 empty bits followed by R/W bit
      *sptr++ = aim_addr >> 8;                         // upper 8 bits of AIM address
      *sptr++ = aim_addr;                              // lower 8 bits of AIM address

      if (type == AIM_WRITE)
      {
         for (i = 0; i < data_bytes; i++) { *sptr++ = *kptr++; }
      }
      else  // (type == AIM_READ)
      {
         for (i = 0; i < data_bytes; i++) { *sptr++ = 0x00; }
      }

      aim_addr += data_bytes;
      bytes_left -= data_bytes;
   }
}

/*==================================================================================================
  
FUNCTION: burst_full

DESCRIPTION:
   Initiate a full burst (28 bytes in length)

ARGUMENTS PASSED:

RETURN VALUE:

==================================================================================================*/
static void burst_full(void)
{
   fifo_write(spid->read_ptr, spid->full_burst_words);
   spid->read_ptr     += spid->full_burst_words;
   spid->words_queued += spid->full_burst_words;
   spid->full_bursts--;
   handler = handle_txfifo_empty;
   cspi->intreg = TEEN;
}

/*==================================================================================================
  
FUNCTION: burst_partial

DESCRIPTION:
   Initiate a partial burst (1 to 27 bytes in length)

ARGUMENTS PASSED:

RETURN VALUE:

==================================================================================================*/
static void burst_partial(void)
{
#ifdef CONFIG_MACH_PEARL
   //CSPI2_SS_2 for pearl dm500
   cspi->conreg = ((spid->partial_burst_bits - 1) << 20) | (spiclk_divider << 16) | (2 << 12) |
                   PHA | POL | SMC | MODE | EN;
#else
   //CSPI1_SS_1 for kassos dm500
   cspi->conreg = ((spid->partial_burst_bits - 1) << 20) | (1 << 16) | (1 << 12) |
                   PHA | POL | SMC | MODE | EN;
#endif
   cspi->statreg = TC;  // write one to clear

   fifo_write(spid->read_ptr, spid->partial_burst_words);
   spid->read_ptr     += spid->partial_burst_words;
   spid->words_queued += spid->partial_burst_words;
   handler = handle_transfer_completed;
   cspi->intreg = TCEN;
}

/*==================================================================================================
  
FUNCTION: cspi_isr

DESCRIPTION:
   Configurable Serial Peripheral Interface Interrupt Service Routine.

ARGUMENTS PASSED:
   irq    - IRQ line that triggered the interrupt
   dev_id - device specific pointer
   regs   - contents of CPU registers at time of interrupt

RETURN VALUE:
   IRQ_HANDLED - indicates to the interrupt subsystem that the IRQ was handled successfully

==================================================================================================*/
static irqreturn_t cspi_isr(int irq, void *dev_id, struct pt_regs *regs)
{
   volatile register u32 dummy;  // dummy variable for debug tracing purposes

   dummy = cspi->statreg;
   dummy = cspi->testreg;

   cspi->intreg = 0;  // disable interrupt source
   handler();
   return IRQ_HANDLED;
}

/*==================================================================================================
  
FUNCTION: dm500_exit         

DESCRIPTION:
   Module exit point

ARGUMENTS PASSED:

RETURN VALUE:

==================================================================================================*/
static void __exit dm500_exit(void)
{
   cdev_del(&cdev);
   unregister_chrdev_region(devno, 1);
#ifdef CONFIG_MACH_PEARL
   free_irq(INT_CSPI2, 0);
   cspi->conreg &= (~EN);
   mxc_clks_disable(CSPI2_CLK);

#else
   free_irq(INT_CSPI1, 0);
#endif
   
}
module_exit(dm500_exit);

/*==================================================================================================
  
FUNCTION: dm500_init         

DESCRIPTION:
   Module entry point

ARGUMENTS PASSED:

RETURN VALUE:
   err - nonzero value indicates an error

==================================================================================================*/
static int __init dm500_init(void)
{
   int err;

   cdev_init(&cdev, &fops);

   // Put CSPI CLK and SS in proper idle state
#ifdef CONFIG_MACH_PEARL
   mxc_clks_enable(CSPI2_CLK);
   cspi->conreg = (spiclk_divider << 16) | (2 << 12) | PHA | POL | SMC | MODE | EN;
#else
   cspi->conreg = (1 << 16) | (1 << 12) | PHA | POL | SMC | MODE | EN;
#endif
   // Make sure there are no interrupt sources enabled at CSPI
   cspi->intreg = 0;

   // Install handler and enable interrupt source at AVIC
#ifdef CONFIG_MACH_PEARL
   err = request_irq(INT_CSPI2, cspi_isr, 0, "DM500 SPI handler", 0);
#else
   err = request_irq(INT_CSPI1, cspi_isr, 0, "DM500 SPI handler", 0);
#endif
   if (err) { goto fail_irq; }

   err = alloc_chrdev_region(&devno, 0, 1, "dm500");
   if (err) { goto fail_devno; }

   err = cdev_add(&cdev, devno, 1);
   if (err) { goto fail_cdev; }

   err = devfs_mk_cdev(devno, S_IFCHR | S_IRUGO | S_IWUGO, "dm500");
   if (err) { goto fail_devfs; }

#ifdef CONFIG_MACH_PEARL
   cspi->conreg &= (~EN);
   mxc_clks_disable(CSPI2_CLK);
#endif

   return(0); // success

fail_devfs:
   cdev_del(&cdev);
fail_cdev:
   unregister_chrdev_region(devno, 1);
fail_devno:
#ifdef CONFIG_MACH_PEARL
   free_irq(INT_CSPI2, 0);
#else
   free_irq(INT_CSPI1, 0);
#endif
fail_irq:
#ifdef CONFIG_MACH_PEARL
   cspi->conreg &= (~EN);	
   mxc_clks_disable(CSPI2_CLK);
#endif

   return(err);
}
module_init(dm500_init);

/*==================================================================================================
  
FUNCTION: dm500_ioctl         

DESCRIPTION:
   ioctl() handler for the dm500 device

ARGUMENTS PASSED:
   inode - pointer to file system node
   filp  - pointer to the open file descriptor
   cmd   - command parameter passed to ioctl() system call
   arg   - argument parameter passed to ioctl() system call

RETURN VALUE:
   error - nonzero value indicates an error

==================================================================================================*/
//in dm399 static int dmcam_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
static int dm500_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
   u8 *kbuf = NULL;
   int error = 0;
   DM500_BD bd;
   DM500_SPICLKMODE spiclk;

   switch (cmd)
   {
      case DM500_SET_SPICLK:  // set spi clock

	 if (copy_from_user(&spiclk, (DM500_SPICLKMODE *)arg, sizeof(spiclk)) != 0)  { error = -EFAULT; break; }
	 if (spiclk == SPICLK_8MHz)
		spiclk_divider = 1;
         else if (spiclk == SPICLK_16MHz)
		spiclk_divider = 0;
         else {
		error = -EINVAL;
		printk("Error:invalid spi clk freq.\n");
              }
         break;

      case DM500_READ_AIM:  // read ARM Internal Memory

	 if (spiclk_divider != 1){spiclk_divider = 1; printk("SPI clock in ARM_BOOT mode must be less than 10MHz \n");}
		
         if (copy_from_user(&bd, (DM500_BD *)arg, sizeof(bd)) != 0)  { error = -EFAULT; break; }
         if ((kbuf = kmalloc(bd.len, GFP_KERNEL)) == NULL)           { error = -ENOMEM; break; }
         if ((error = spi_transfer_aim(kbuf, bd.len, bd.addr, AIM_READ)))             { break; }
         if (copy_to_user(bd.ubuf, kbuf, bd.len) != 0)               { error = -EFAULT; break; }
         break;

      case DM500_WRITE_AIM:  // write ARM Internal Memory

	 if (spiclk_divider != 1){spiclk_divider = 1; printk("SPI clock in ARM_BOOT mode must be less than 10MHz \n");}

         if (copy_from_user(&bd, (DM500_BD *)arg, sizeof(bd)) != 0)  { error = -EFAULT; break; }
         if ((kbuf = kmalloc(bd.len, GFP_KERNEL)) == 0)              { error = -ENOMEM; break; }
         if (copy_from_user(kbuf, bd.ubuf, bd.len) != 0)             { error = -EFAULT; break; }
         if ((error = spi_transfer_aim(kbuf, bd.len, bd.addr, AIM_WRITE)))            { break; }
         break;

      case DM500_EXCHANGE:  // transmit and receive (full duplex) a DM500 SPI message
        
         if (copy_from_user(&bd, (DM500_BD *)arg, sizeof(bd)) != 0)  { error = -EFAULT; break; }
         if ((kbuf = kmalloc(bd.len, GFP_KERNEL)) == 0)              { error = -ENOMEM; break; }
         if (copy_from_user(kbuf, bd.ubuf, bd.len) != 0)             { error = -EFAULT; break; }
         if ((error = spi_transfer_msg(kbuf, bd.len)))                                { break; }
         if (copy_to_user(bd.ubuf, kbuf, bd.len) != 0)               { error = -EFAULT; break; }
         break;

      default:
         error = -EINVAL;
         break;
   }

   if (kbuf) { kfree(kbuf); }
   return(error);
}

/*==================================================================================================
  
FUNCTION: dm500_open         

DESCRIPTION:
   open() handler for the dm500 device

ARGUMENTS PASSED:
   inode - pointer to file system node
   filp  - pointer to the open file descriptor

RETURN VALUE:
   error - nonzero value indicates an error

==================================================================================================*/
static int dm500_open(struct inode *inode, struct file *filp)
{

#ifdef CONFIG_MACH_PEARL
   mxc_clks_enable(CSPI2_CLK);
#endif
   
   return(0); // success

}

/*==================================================================================================
  
FUNCTION: dm500_release         

DESCRIPTION:
   close() handler for the dm500 device

ARGUMENTS PASSED:
   inode - pointer to file system node
   filp  - pointer to the open file descriptor

RETURN VALUE:
   error - nonzero value indicates an error

==================================================================================================*/
static int dm500_release(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_MACH_PEARL
   cspi->conreg &= (~EN);
   mxc_clks_disable(CSPI2_CLK);
#endif

   return(0);
}

/*==================================================================================================
  
FUNCTION: fifo_read

DESCRIPTION:
   Read words from RXFIFO

ARGUMENTS PASSED:
   ptr       - pointer to output buffer
   num_words - number of words to retrieve from RXFIFO

RETURN VALUE:

==================================================================================================*/
static void fifo_read(u32 *ptr, u8 num_words)
{
   u32 word;
   u8 i;

   for (i = 0; i < num_words; i++)
   {
      word = cspi->rxdata;
      asm("rev %0,%0" : "+r" (word));  // reverse byte order
      ptr[i] = word;
   }
}

/*==================================================================================================
  
FUNCTION: fifo_write

DESCRIPTION:
   Write words to TXFIFO

ARGUMENTS PASSED:
   ptr       - pointer to input buffer
   num_words - number of words to place in TXFIFO

RETURN VALUE:

==================================================================================================*/
static void fifo_write(u32 *ptr, u8 num_words)
{
   u32 flags;
   u32 word;
   u8 i;

   // If the TXFIFO empties during a burst the CSPI module will stop the clock and
   // wait for new data.  When the new data is written to the TXFIFO the CSPI will
   // restart the clock and resume the transfer.  However, due to a Freescale bug,
   // sometimes a bit will be dropped when this happens.  Therefore, we must prevent
   // the possibility that the TXFIFO empties in the middle of a burst.  We do this
   // by choosing burst lengths that are no larger than the size of the FIFO and we
   // disable all interrupts (normal and fast) while we fill the FIFO.

   local_save_flags(flags);
   local_irq_disable();
   local_fiq_disable();

   for (i = 0; i < num_words; i++)
   {
      word = ptr[i];
      asm("rev %0,%0" : "+r" (word));  // reverse byte order
      cspi->txdata = word;
   }

   local_irq_restore(flags);
}

/*==================================================================================================
  
FUNCTION: handle_rxfifo_ready

DESCRIPTION:
   CSPI interrupt handler for RR (RXFIFO Ready).
   We only wait on this event during the last full burst.

ARGUMENTS PASSED:

RETURN VALUE:

==================================================================================================*/
static void handle_rxfifo_ready(void)
{
   fifo_read(spid->write_ptr, 1);
   spid->write_ptr    += 1;
   spid->words_queued -= 1;

   if (spid->partial_burst_words)
   {
      burst_partial();
   }
   else
   {
      spid->transaction_complete = 1;
      wake_up_interruptible(&spi_wait);
   }
}

/*==================================================================================================
  
FUNCTION: handle_spurious

DESCRIPTION:
   CSPI interrupt handler when no interrupts are expected

ARGUMENTS PASSED:

RETURN VALUE:

==================================================================================================*/
static void handle_spurious(void)
{
   return;  // TODO: We should invoke some kind of error reporting mechanism
}

/*==================================================================================================
  
FUNCTION: handle_transfer_completed

DESCRIPTION:
   CSPI interrupt handler for TC (Transfer Completed).
   We only wait on this event during a partial burst.

ARGUMENTS PASSED:

RETURN VALUE:

==================================================================================================*/
static void handle_transfer_completed(void)
{
   fifo_read(spid->write_ptr, spid->partial_burst_words);
   spid->write_ptr    += spid->partial_burst_words;
   spid->words_queued -= spid->partial_burst_words;
   spid->transaction_complete = 1;
   wake_up_interruptible(&spi_wait);
}

/*==================================================================================================
  
FUNCTION: handle_txfifo_empty

DESCRIPTION:
   CSPI interrupt handler for TE (TXFIFO Empty).
   We only wait on this event during a full burst.

ARGUMENTS PASSED:

RETURN VALUE:

==================================================================================================*/
static void handle_txfifo_empty(void)
{
   u8 words_completed = spid->words_queued - 1;  // last word may still be in progress

   fifo_read(spid->write_ptr, words_completed);
   spid->write_ptr    += words_completed;
   spid->words_queued -= words_completed;

   if (spid->full_bursts)
   {
      burst_full();
   }
   else  // wait for the last word to finish
   {
      handler = handle_rxfifo_ready;
      cspi->intreg = RREN;
   }
}

/*==================================================================================================
  
FUNCTION: msg_depopulate_buffer

DESCRIPTION:
   This function depopulates the buffer that contains the words we read out of the
   RXFIFO.  Any padding bytes will be stripped out and the remaining byte stream will
   be written to the output buffer.

ARGUMENTS PASSED:
   desc - pointer to descriptor
   kbuf - output buffer
   len  - length of buffer

RETURN VALUE:

==================================================================================================*/
static void msg_depopulate_buffer(spi_desc *desc, u8 *kbuf, u32 len)
{
   u32 bytes_left = len;
   u8 *sptr = (u8 *)desc->buffer;
   u8 *kptr = kbuf;
   u32 data_bytes, empty_bytes, i;

   while (bytes_left)
   {
      if (bytes_left >= desc->full_burst_bytes)
      {
         data_bytes = desc->full_burst_bytes;
         empty_bytes = 0;
      }
      else
      {
         data_bytes = bytes_left;
         empty_bytes = (desc->partial_burst_words * 32 - desc->partial_burst_bits) / 8;
      }

      sptr += empty_bytes;

      for (i = 0; i < data_bytes; i++) { *kptr++ = *sptr++; }

      bytes_left -= data_bytes;
   }
}

/*==================================================================================================
  
FUNCTION: msg_generate_descriptor

DESCRIPTION:
   This function generates a SPI transaction descriptor.  It allocates memory for it and
   initializes the fields.

ARGUMENTS PASSED:
   len - the length of the transfer this descriptor will specify

RETURN VALUE:
   desc - pointer to the descriptor (NULL if we could not allocate one)

==================================================================================================*/
static spi_desc *msg_generate_descriptor(u32 len)
{
   spi_desc *desc;
   u32 total_words;

   desc = kmalloc(sizeof(spi_desc), GFP_KERNEL);
   if (desc == NULL) { return NULL; }

   desc->full_burst_words = 7;
   desc->full_burst_bytes = desc->full_burst_words * 4;
   desc->full_burst_bits = desc->full_burst_bytes * 8;
   desc->full_bursts = len / desc->full_burst_bytes;

   desc->partial_burst_bytes = len - desc->full_bursts * desc->full_burst_bytes;

   if (desc->partial_burst_bytes)
   {
      desc->partial_burst_bits = desc->partial_burst_bytes * 8;
      desc->partial_burst_words = (desc->partial_burst_bits + 31) / 32;
   }
   else
   {
      desc->partial_burst_bits = 0;
      desc->partial_burst_words = 0;
   }

   total_words = desc->full_bursts * desc->full_burst_words + desc->partial_burst_words;

   desc->buffer = kmalloc(total_words * 4, GFP_KERNEL);
   if (desc->buffer == NULL) { kfree(desc); return NULL; }

   desc->write_ptr = desc->read_ptr = desc->buffer;
   desc->words_queued = 0;
   desc->transaction_complete = 0;

   return(desc);
}

/*==================================================================================================
  
FUNCTION: msg_populate_buffer

DESCRIPTION:
   This function populates the buffer that contains the words we will write to the
   TXFIFO.  We will read the raw byte stream from the input buffer and insert any
   padding bytes if necessary.

ARGUMENTS PASSED:
   desc - pointer to descriptor
   kbuf - input buffer
   len  - length of buffer

RETURN VALUE:

==================================================================================================*/
static void msg_populate_buffer(spi_desc *desc, u8 *kbuf, u32 len)
{
   u32 bytes_left = len;
   u8 *sptr = (u8 *)desc->buffer;
   u8 *kptr = kbuf;
   u32 data_bytes, empty_bytes, i;

   while (bytes_left)
   {
      if (bytes_left >= desc->full_burst_bytes)
      {
         data_bytes = desc->full_burst_bytes;
         empty_bytes = 0;
      }
      else
      {
         data_bytes = bytes_left;
         empty_bytes = (desc->partial_burst_words * 32 - desc->partial_burst_bits) / 8;
      }

      for (i = 0; i < empty_bytes; i++) { *sptr++ = 0x00;    }
      for (i = 0; i < data_bytes;  i++) { *sptr++ = *kptr++; }

      bytes_left -= data_bytes;
   }
}

/*==================================================================================================
  
FUNCTION: perform_transfer

DESCRIPTION:
   This function performs the SPI transfer specified in the descriptor.  It begins the
   transfer and then blocks in a wait queue.  The interrupt handlers perform the rest
   of the transfer and wake us up when it's done.

ARGUMENTS PASSED:
   desc - pointer to descriptor

RETURN VALUE:
   error - negative value indicates an error

==================================================================================================*/
static int perform_transfer(spi_desc *desc)
{
   long result;

   spid = desc;  // save pointer so interrupt handlers can access descriptor
   
#ifdef CONFIG_MACH_PEARL
   cspi->conreg = ((spid->full_burst_bits - 1) << 20) | (spiclk_divider << 16) | (2 << 12) |
                   PHA | POL | SMC | MODE | EN;
#else
   cspi->conreg = ((spid->full_burst_bits - 1) << 20) | (1 << 16) | (1 << 12) |
                   PHA | POL | SMC | MODE | EN;
#endif
   if (spid->full_bursts) { burst_full();    }
   else                   { burst_partial(); }

   // TODO: We are currently using a 1 second timeout.  This will need to be reevaluated.
   result = wait_event_interruptible_timeout(spi_wait, spid->transaction_complete, HZ);

   if (result == 0)             { return(-EIO);   }  // timeout occurred
   if (result == -ERESTARTSYS)  { return(-EINTR); }  // interrupted by signal
   if (result < 0)              { return(result); }  // unknown error

   return(0);  // transfer completed successfully
}

/*==================================================================================================
  
FUNCTION: spi_transfer_aim

DESCRIPTION:
   This function performs a SPI transaction that reads or writes to AIM (ARM Internal Memory)
   on the DM500.  It uses a special SPI protocol that is only valid during the bootstrap
   phase.

ARGUMENTS PASSED:
   kbuf - pointer to kernel buffer
   len  - length of kernel buffer
   base - address of AIM to read/write
   type - AIM_READ or AIM_WRITE

RETURN VALUE:
   error - nonzero value indicates an error

==================================================================================================*/
static int spi_transfer_aim(u8 *kbuf, u32 len, u32 base, AIM_TYPE type)
{
   spi_desc *desc;
   int error;

   if (len == 0) { return 0; }  // nothing to transfer

   desc = aim_generate_descriptor(len);
   if (desc == NULL) { return -ENOMEM; }  // out of memory

   aim_populate_buffer(desc, kbuf, len, base, type);

   error = perform_transfer(desc);
   if ((error == 0) && (type == AIM_READ)) { aim_depopulate_buffer(desc, kbuf, len); }

   kfree(desc->buffer);
   kfree(desc);
   return(error);
}

/*==================================================================================================
  
FUNCTION: spi_transfer_msg         

DESCRIPTION:
   This function performs a SPI transaction that transmits a stream of bytes in both
   directions.  The transaction may be split up into multiple bursts.

ARGUMENTS PASSED:
   kbuf - pointer to kernel buffer (used for input and output)
   len  - length of kernel buffer

RETURN VALUE:
   error - nonzero value indicates an error

==================================================================================================*/
static int spi_transfer_msg(u8 *kbuf, u32 len)
{
   spi_desc *desc;
   int error;

   if (len == 0) { return 0; }  // nothing to transfer

   desc = msg_generate_descriptor(len);
   if (desc == NULL) { return -ENOMEM; }  // out of memory

   msg_populate_buffer(desc, kbuf, len);

   error = perform_transfer(desc);
   if (error == 0) { msg_depopulate_buffer(desc, kbuf, len); }

   kfree(desc->buffer);
   kfree(desc);
   return(error);
}
