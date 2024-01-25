/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006 - 2008 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_i2c.c
 *
 * @brief Driver for the Freescale Semiconductor MXC I2C buses.
 *
 * Based on i2c driver algorithm for PCF8584 adapters
 *
 * @ingroup MXCI2C
 */
/*
 *  Date     Author    Comment
 *  11/2006  Motorola  Added delay for stop on bus and fixed ack
 *                     not being checked on a bus read.
 *  12/2006  Motorola  Alter delay to read busy bit.
 *  03/2007  Motorola  Added fix to suspend and resume with interrupts
 *                     disabled. 
 *  04/2007  Motorola  Block suspend when I2C transaction is in progress and
 *                     remove clock gating on a transaction basis
 *  05/2007  Motorola  Added hardware configuration (device tree)
 *		       support for clock frequency divider.
 *  08/2007  Motorola  Clear control register after each transaction
 *  09/2007  Motorola  Add mpm advise calls
 *  10/2007  Motorola  Make sure have a STOP after all communication ceased
 *  03/2008  Motorola  Remove mpm and clock gating for elba.		
 *  03/2008  Motorola  Remove mpm advise calls due to ESD issues
 *  04/2008  Motorola  Add failure handler for arbitration lost issue. 
 *  07/2008  Motorola  Replace KERN_DEBUG with KERN_ERR in printk so that we can 
 *                     all I2C errors in AP log.
 *  09/2008  Motorola  Add a new interface mxc_cam_i2c_transfer().
 */
/*
 * Include Files
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/i2c.h>
#include <asm/arch/mxc_i2c.h>
#include <asm/arch/clock.h>
#ifdef CONFIG_MOT_FEAT_DEVICE_TREE
#include <asm/mothwcfg.h>
#endif
#include "mxc_i2c_reg.h"

#if defined(CONFIG_MOT_FEAT_PM) && !defined(CONFIG_MACH_ELBA)
#include <linux/mpm.h>
#endif

/*! for overriding the bus speed
   This flag complements the ones for i2c_msg.flags */
#define I2C_M_BUS_SPEED_OVERRIDE   0x1000 


#ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND
enum mxc_i2c_dev_status {
       MXC_I2C_IN_TRANSACTION = 1, /*transaction is ongoing*/
};
#endif

/*!
 * In case the MXC device has multiple I2C modules, this structure is used to
 * store information specific to each I2C module.
 */
typedef struct {
	/*!
	 * This structure is used to identify the physical i2c bus along with
	 * the access algorithms necessary to access it.
	 */
	struct i2c_adapter adap;

#ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND	
	unsigned long status; /* atomic bitops */
#endif

	/*!
	 * This waitqueue is used to wait for the data transfer to complete.
	 */
	wait_queue_head_t wq;

	/*!
	 * The base address of the I2C device.
	 */
	char *membase;

	/*!
	 * The interrupt number used by the I2C device.
	 */
	int irq;

	/*!
	 * The default clock divider value to be used.
	 */
	unsigned int clkdiv;

	/*!
	 * The current power state of the device
	 */
	bool low_power;
} mxc_i2c_device;

/*!
 * Boolean to indicate if data was transferred
 */
static bool transfer_done = false;

/*!
 * Boolean to indicate if we received an ACK for the data transmitted
 */
static bool tx_success = false;
static bool arbitration_lost = false;

/*!
 * This is an array where each element holds information about a I2C module,
 * like base address, interrupt number etc. This elements of this structure
 * are stored in the appropriate i2c_adapter structure during the module
 * initialization.
 */
static mxc_i2c_device mxc_i2c_devs[I2C_NR] = {
	[0] = {
	       .membase = (void *)IO_ADDRESS(I2C_BASE_ADDR),
	       .irq = INT_I2C,
	       .clkdiv = I2C1_FRQ_DIV,
	       },
#if I2C_NR > 1
	[1] = {
	       .membase = (void *)IO_ADDRESS(I2C2_BASE_ADDR),
	       .irq = INT_I2C2,
	       .clkdiv = I2C2_FRQ_DIV,
	       },
#if I2C_NR > 2
	[2] = {
	       .membase = (void *)IO_ADDRESS(I2C3_BASE_ADDR),
	       .irq = INT_I2C3,
	       .clkdiv = I2C3_FRQ_DIV,
	       }
#endif
#endif
};

#if defined(CONFIG_MOT_FEAT_PM) && !defined(CONFIG_MACH_ELBA)
static char *mpm_i2c_dev_names[I2C_NR] = {
        [0] = "mxc-i2c0",
#if I2C_NR > 1
        [1] = "mxc-i2c1",
#if I2C_NR > 2
        [2] = "mxc-i2c2",
#endif
#endif
};

static int mpm_i2c_dev_nums[I2C_NR];
#endif

extern void gpio_i2c_active(int i2c_num);
extern void gpio_i2c_inactive(int i2c_num);

/*!
 * Transmit a \b STOP signal to the slave device.
 *
 * @param   dev   the mxc i2c structure used to get to the right i2c device
 */
static void mxc_i2c_stop(mxc_i2c_device * dev)
{
	volatile unsigned int cr;
	unsigned int count=100;

	cr = readw(dev->membase + MXC_I2CR);
	cr &= ~(MXC_I2CR_MSTA | MXC_I2CR_MTX);
	writew(cr, dev->membase + MXC_I2CR);

#ifdef CONFIG_MOT_WFN461
	/* We need to wait for the stop to be transmitted because we might
	 * turn off the bus clock(SCL) once we exit this function. If that
	 * happens, the stop will never occur on the bus. */
	while ( (readw(dev->membase + MXC_I2SR) & MXC_I2SR_IBB) && --count) {
		udelay(1);
	}
	if (count == 0) {
		printk(KERN_ERR "I2C device timeout waiting for stop\n");
	}
#endif
}

/*!
 * Wait for the transmission of the data byte to complete. This function waits
 * till we get a signal from the interrupt service routine indicating completion
 * of the address cycle or we time out.
 *
 * @param   dev         the mxc i2c structure used to get to the right i2c device
 * @param   trans_flag  transfer flag
 *
 *
 * @return  The function returns 0 on success or -1 if an ack was not received
 */

#ifdef CONFIG_MOT_WFN465
static int mxc_i2c_wait_for_tc(mxc_i2c_device * dev, int trans_flag, int check_ack)
#else
static int mxc_i2c_wait_for_tc(mxc_i2c_device * dev, int trans_flag)
#endif
{
	int retry = 16;
	volatile unsigned int sr, cr;

	if (trans_flag & MXC_I2C_FLAG_POLLING) {
		/* I2C_POLLING_MODE */
		unsigned long x = jiffies + dev->adap.timeout * HZ;
		while (!(readw(dev->membase + MXC_I2SR) & MXC_I2SR_IIF)) {
			if (unlikely(time_after(jiffies, x))) {
				/* Poll timeout */
				retry = -ETIMEDOUT;
				break;
			}
			if (unlikely(signal_pending(current))) {
				/* Poll interrupted */
				retry = -ERESTARTSYS;
				break;
			}
			if (!irqs_disabled()) {
				schedule();
			}
		}
		sr = readw(dev->membase + MXC_I2SR);
		cr = readw(dev->membase + MXC_I2CR);
		writew(0, dev->membase + MXC_I2SR);

		tx_success = false;
		/* Check if RXAK is received in Transmit mode */
		if ((cr & MXC_I2CR_MTX) && (!(sr & MXC_I2SR_RXAK))) {
			tx_success = true;
		}
	} else {
		/* Interrupt mode */
		while (retry-- && !transfer_done) {
			wait_event_interruptible_timeout(dev->wq,
							 transfer_done,
							 dev->adap.timeout);
		}
		transfer_done = false;
	}
	if (retry <= 0) {
		/* Unable to send data */
		printk(KERN_ERR "Data not transmitted\n");
		return -EINTLOSTT;
#ifdef CONFIG_MOT_WFN465
	 } else if (arbitration_lost) {
                arbitration_lost = false;
                printk(KERN_DEBUG "Arbitration lost \n");
                return -EARBLOSTT;
	} else if (check_ack && !tx_success) {
		/* An ACK was not received for transmitted byte */
		printk(KERN_ERR "ACK not received \n");
		return -EACKLOSTT;
	}
#else
	} else if (arbitration_lost) {
                arbitration_lost = false;
                printk(KERN_DEBUG "Arbitration lost \n");
                return -EARBLOSTT;
	} else if (!(trans_flag & I2C_M_RD)) {
		if (!tx_success) {
			/* An ACK was not received for transmitted byte */
			printk(KERN_ERR "ACK not received \n");
			return -EACKLOSTT;
		}
	}
#endif

	return 0;
}

/*!
 * Transmit a \b START signal to the slave device.
 *
 * @param   dev   the mxc i2c structure used to get to the right i2c device
 * @param   *msg  pointer to a message structure that contains the slave
 *                address
 */
// static void mxc_i2c_start(mxc_i2c_device * dev, struct i2c_msg *msg)
static int mxc_i2c_start(mxc_i2c_device * dev, struct i2c_msg *msg)
{
	volatile unsigned int cr, sr;
	unsigned int addr_trans;
	int retry = 16;

	/* Set the frequency divider */
	writew(dev->clkdiv, dev->membase + MXC_IFDR);  
	/*
	 * Set the slave address and the requested transfer mode
	 * in the data register
	 */
	addr_trans = msg->addr << 1;
	if (msg->flags & I2C_M_RD) {
		addr_trans |= 0x01;
	}

	/* Set the Master bit */
	cr = readw(dev->membase + MXC_I2CR);
	cr |= MXC_I2CR_MSTA;
	writew(cr, dev->membase + MXC_I2CR);

	/* Wait till the Bus Busy bit is set */
	sr = readw(dev->membase + MXC_I2SR);
	while (retry-- && (!(sr & MXC_I2SR_IBB))) {
		udelay(3);
		sr = readw(dev->membase + MXC_I2SR);
	}
	if (retry <= 0) {
		printk(KERN_ERR "Could not grab Bus ownership,addr:0x%x\n",msg->addr);
		if(arbitration_lost) {
			arbitration_lost = false;
			return -EARBLOSTT;
		}
		return -ESTARTFAILT;
	}

	/* Set the Transmit bit */
	cr = readw(dev->membase + MXC_I2CR);
	cr |= MXC_I2CR_MTX;
	writew(cr, dev->membase + MXC_I2CR);

	writew(addr_trans, dev->membase + MXC_I2DR);
	return 0;
}

/*!
 * Transmit a \b REPEAT START to the slave device
 *
 * @param   dev   the mxc i2c structure used to get to the right i2c device
 * @param   *msg  pointer to a message structure that contains the slave
 *                address
 */
static void mxc_i2c_repstart(mxc_i2c_device * dev, struct i2c_msg *msg)
{
	volatile unsigned int cr;
	unsigned int addr_trans;

	/*
	 * Set the slave address and the requested transfer mode
	 * in the data register
	 */
	addr_trans = msg->addr << 1;
	if (msg->flags & I2C_M_RD) {
		addr_trans |= 0x01;
	}
	cr = readw(dev->membase + MXC_I2CR);
	cr |= MXC_I2CR_RSTA;
	writew(cr, dev->membase + MXC_I2CR);
	udelay(3);
	writew(addr_trans, dev->membase + MXC_I2DR);
}

/*!
 * Read the received data. The function waits till data is available or times
 * out. Generates a stop signal if this is the last message to be received.
 * Sends an ack for all the bytes received except the last byte.
 *
 * @param  dev       the mxc i2c structure used to get to the right i2c device
 * @param  *msg      pointer to a message structure that contains the slave
 *                   address and a pointer to the receive buffer
 * @param  last      indicates that this is the last message to be received
 * @param  addr_comp flag indicates that we just finished the address cycle
 *
 * @return  The function returns the number of bytes read or -1 on time out.
 */
static int mxc_i2c_readbytes(mxc_i2c_device * dev, struct i2c_msg *msg,
			     int last, int addr_comp)
{
	int i,rett;
	char *buf = msg->buf;
	int len = msg->len;
	volatile unsigned int cr;

	cr = readw(dev->membase + MXC_I2CR);
	/*
	 * Clear MTX to switch to receive mode.
	 */
	cr &= ~MXC_I2CR_MTX;
	/*
	 * Clear the TXAK bit to gen an ack when receiving only one byte.
	 */
	if (len == 1) {
		cr |= MXC_I2CR_TXAK;
	} else {
		cr &= ~MXC_I2CR_TXAK;
	}
	writew(cr, dev->membase + MXC_I2CR);
	/*
	 * Dummy read only at the end of an address cycle
	 */
	if (addr_comp > 0) {
		readw(dev->membase + MXC_I2DR);
	}

	for (i = 0; i < len; i++) {
		/*
		 * Wait for data transmission to complete
		 */
#if CONFIG_MOT_WFN465
		if (rett = mxc_i2c_wait_for_tc(dev, msg->flags, !(msg->flags & I2C_M_RD))) {
#else
		if (rett = mxc_i2c_wait_for_tc(dev, msg->flags)) {
#endif

#ifdef MENSHE_71_14_1AR || (! defined(CONFIG_MACH_ELBA) && ! defined(CONFIG_MACH_PIANOSA) && ! defined(CONFIG_MACH_KEYWEST)  && ! defined(CONFIG_MACH_PEARL) )
			mxc_i2c_stop(dev);
			return -1;
#else
			return rett;
#endif
		}
		/*
		 * Do not generate an ACK for the last byte
		 */
		if (i == (len - 2)) {
			cr = readw(dev->membase + MXC_I2CR);
			cr |= MXC_I2CR_TXAK;
			writew(cr, dev->membase + MXC_I2CR);
		} else if (i == (len - 1)) {
			if (last) {
				mxc_i2c_stop(dev);
			}
		}
		/* Read the data */
		*buf++ = readw(dev->membase + MXC_I2DR);
	}

	return i;
}

/*!
 * Write the data to the data register. Generates a stop signal if this is
 * the last message to be sent or if no ack was received for the data sent.
 *
 * @param   dev   the mxc i2c structure used to get to the right i2c device
 * @param   *msg  pointer to a message structure that contains the slave
 *                address and data to be sent
 * @param   last  indicates that this is the last message to be sent
 *
 * @return  The function returns the number of bytes written or -1 on time out
 *          or if no ack was received for the data that was sent.
 */
static int mxc_i2c_writebytes(mxc_i2c_device * dev, struct i2c_msg *msg,
			      int last)
{
	int i,rett;
	char *buf = msg->buf;
	int len = msg->len;
	volatile unsigned int cr;

	cr = readw(dev->membase + MXC_I2CR);
	/* Set MTX to switch to transmit mode */
	cr |= MXC_I2CR_MTX;
	writew(cr, dev->membase + MXC_I2CR);

	for (i = 0; i < len; i++) {
		/*
		 * Write the data
		 */
		writew(*buf++, dev->membase + MXC_I2DR);
#ifdef CONFIG_MOT_WFN465
		if (rett = mxc_i2c_wait_for_tc(dev, msg->flags, !(msg->flags & I2C_M_RD))) {
#else
		if (rett = mxc_i2c_wait_for_tc(dev, msg->flags)) {
#endif
#ifdef MENSHE_71_14_1AR || (! defined(CONFIG_MACH_ELBA) && ! defined(CONFIG_MACH_PIANOSA) && ! defined(CONFIG_MACH_KEYWEST) && ! defined(CONFIG_MACH_PEARL) )
			mxc_i2c_stop(dev);
			return -1;
#else
			return rett;
#endif
		}
        
	}
	if (last > 0) {
		mxc_i2c_stop(dev);
	}

	return i;
}

#if ! defined(CONFIG_MACH_PIANOSA)
#ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND
/*!
 * Function initializes the I2C the registers.
 *
 * @param   dev   the mxc i2c structure used to get to the right i2c device
 * @param   trans_flag  transfer flag
 */
static void mxc_i2c_module_en_regs(mxc_i2c_device * dev, int trans_flag)
{
	/* Clear the status register */
	writew(0x0, dev->membase + MXC_I2SR);
	/* Enable I2C and its interrupts */
	if (trans_flag & MXC_I2C_FLAG_POLLING) {
		writew(MXC_I2CR_IEN, dev->membase + MXC_I2CR);
	} else {
		writew(MXC_I2CR_IEN, dev->membase + MXC_I2CR);
		writew(MXC_I2CR_IEN | MXC_I2CR_IIEN, dev->membase + MXC_I2CR);
	}
}

/*!
 *  Enables the I2C clocks.
 *  
 *  @param   dev   the mxc i2c structure used to get to the right i2c device
 */
static void mxc_i2c_module_en_clks(mxc_i2c_device * dev)
{
        mxc_clks_enable(I2C_CLK);
        /* Set the frequency divider */
        writew(dev->clkdiv, dev->membase + MXC_IFDR);        
}
#else /* ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND*/
/*!
 * Function enables the I2C module and initializes the registers.
 *
 * @param   dev   the mxc i2c structure used to get to the right i2c device
 * @param   trans_flag  transfer flag
 */
static void mxc_i2c_module_en(mxc_i2c_device * dev, int trans_flag)
{
        mxc_clks_enable(I2C_CLK);
        /* Set the frequency divider */
        writew(dev->clkdiv, dev->membase + MXC_IFDR);
        /* Clear the status register */
        writew(0x0, dev->membase + MXC_I2SR);
        /* Enable I2C and its interrupts */
        if (trans_flag & MXC_I2C_FLAG_POLLING) {
                writew(MXC_I2CR_IEN, dev->membase + MXC_I2CR);
        } else {
                writew(MXC_I2CR_IEN, dev->membase + MXC_I2CR);
                writew(MXC_I2CR_IEN | MXC_I2CR_IIEN, dev->membase + MXC_I2CR);
        }
}
#endif/*END ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND*/
/*!
 * Disables the I2C module.
 *
 * @param   dev   the mxc i2c structure used to get to the right i2c device
 */
static void mxc_i2c_module_dis(mxc_i2c_device * dev)
{
	writew(0x0, dev->membase + MXC_I2CR);
	mxc_clks_disable(I2C_CLK);
}

#else /* CONFIG_MACH_ELBA */

/*!
 * Function enables the I2C module and initializes the registers.
 *
 * @param   dev   the mxc i2c structure used to get to the right i2c device
 * @param   trans_flag  transfer flag
 */
static void mxc_i2c_module_en(mxc_i2c_device * dev, int trans_flag)
{
#ifdef CONFIG_MOT_FEAT_PM
	if (mpm_driver_advise(mpm_i2c_dev_nums[dev->adap.id], MPM_ADVICE_DRIVER_IS_BUSY))
		printk(KERN_ERR "MPM advise call failed for mxc-i2c%d\n", dev->adap.id);
#endif
#ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND
	set_bit(MXC_I2C_IN_TRANSACTION, &(dev->status));
#endif

        mxc_clks_enable(I2C_CLK);
        /* Set the frequency divider */
        writew(dev->clkdiv, dev->membase + MXC_IFDR);
        /* Clear the status register */
        writew(0x0, dev->membase + MXC_I2SR);
        /* Enable I2C and its interrupts */
        if (trans_flag & MXC_I2C_FLAG_POLLING) {
                writew(MXC_I2CR_IEN, dev->membase + MXC_I2CR);
        } else {
                writew(MXC_I2CR_IEN, dev->membase + MXC_I2CR);
                writew(MXC_I2CR_IEN | MXC_I2CR_IIEN, dev->membase + MXC_I2CR);
        }
}

/*!
 * Disables the I2C module.
 *
 * @param   dev   the mxc i2c structure used to get to the right i2c device
 */
static void mxc_i2c_module_dis(mxc_i2c_device * dev)
{
	writew(0x0, dev->membase + MXC_I2CR);
	mxc_clks_disable(I2C_CLK);

#ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND      
	clear_bit(MXC_I2C_IN_TRANSACTION, &(dev->status));
#endif
#ifdef CONFIG_MOT_FEAT_PM
	if(mpm_driver_advise(mpm_i2c_dev_nums[dev->adap.id], MPM_ADVICE_DRIVER_IS_NOT_BUSY))
		printk(KERN_ERR "MPM advise call failed for mxc-i2c%d\n", dev->adap.id);
#endif
}

#endif/*END ifdef CONFIG_MACH_ELBA*/
/*!
 * The function is registered in the adapter structure. It is called when an MXC
 * driver wishes to transfer data to a device connected to the I2C device.
 *
 * @param   adap   adapter structure for the MXC i2c device
 * @param   msgs[] array of messages to be transferred to the device
 * @param   num    number of messages to be transferred to the device
 *
 * @return  The function returns the number of messages transferred,
 *          \b -EREMOTEIO on I2C failure and a 0 if the num argument is
 *          less than 0.
 */
static int mxc_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[],
			int num)
{
	int rett = 0;
	mxc_i2c_device *dev = (mxc_i2c_device *) (i2c_get_adapdata(adap));
	int i, ret = 0, addr_comp = 0;
	volatile unsigned int sr;

	if (dev->low_power) {
		printk(KERN_ERR "I2C Device in low power mode\n");
		return -EPWLOWT;
	}

	if (num < 1) {
		return 0;
	}
#if ! defined(CONFIG_MACH_PIANOSA) && defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
        set_bit(MXC_I2C_IN_TRANSACTION, &(dev->status));
	mxc_i2c_module_en_regs(dev, msgs[0].flags);
#else
	mxc_i2c_module_en(dev, msgs[0].flags);
#endif
	sr = readw(dev->membase + MXC_I2SR);
	/*
	 * Check bus state
	 */
	if (sr & MXC_I2SR_IBB) {
#if ! defined(CONFIG_MACH_PIANOSA) && defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
                clear_bit(MXC_I2C_IN_TRANSACTION, &(dev->status));
                writew(0x0, dev->membase + MXC_I2CR);
#else
		mxc_i2c_module_dis(dev);
#endif
		printk(KERN_ERR "Bus busy\n");
		return -EBUSBUSYT;
	}
	//gpio_i2c_active(dev->adap.id);
	transfer_done = false;
	tx_success = false;
	arbitration_lost = false;

	for (i = 0; i < num && ret >= 0; i++) {

		addr_comp = 0;
		/*
		 * Send the slave address and transfer direction in the
		 * address cycle
		 */
		 
		/* If we want to run at 100kbit/s we have to override default settings */ 
		if (msgs[i].flags & I2C_M_BUS_SPEED_OVERRIDE)
        {
		    dev->clkdiv = 0x0B;
        }
		else
        {
		    dev->clkdiv = I2C1_FRQ_DIV;
		}
		if (i == 0) {
			/*
			 * Send a start or repeat start signal
			 */
			if(rett = mxc_i2c_start(dev, &msgs[0])){
                                mxc_i2c_stop(dev);
#if defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
                                clear_bit(MXC_I2C_IN_TRANSACTION, &(dev->status));
                                writew(0x0, dev->membase + MXC_I2CR);
#else
                                mxc_i2c_module_dis(dev);
#endif
                                return rett;
			}
			/* Wait for the address cycle to complete */
#ifdef CONFIG_MOT_WFN465
			if (rett = mxc_i2c_wait_for_tc(dev, msgs[0].flags, 1)) {
#else
			if (rett = mxc_i2c_wait_for_tc(dev, msgs[0].flags)) {
#endif
				mxc_i2c_stop(dev);
				//gpio_i2c_inactive(dev->adap.id);
#if defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
                                clear_bit(MXC_I2C_IN_TRANSACTION, &(dev->status));
                                writew(0x0, dev->membase + MXC_I2CR);
#else
				mxc_i2c_module_dis(dev);
#endif

				return rett;
			}
			addr_comp = 1;
		} else {
			/*
			 * Generate repeat start only if required i.e the address
			 * changed or the transfer direction changed
			 */
			if ((msgs[i].addr != msgs[i - 1].addr) ||
			    ((msgs[i].flags & I2C_M_RD) !=
			     (msgs[i - 1].flags & I2C_M_RD))) {
				mxc_i2c_repstart(dev, &msgs[i]);
				/* Wait for the address cycle to complete */
#ifdef CONFIG_MOT_WFN465
				if (rett = mxc_i2c_wait_for_tc(dev, msgs[i].flags, 1)) {
#else
				if (rett = mxc_i2c_wait_for_tc(dev, msgs[i].flags)) {
#endif
					mxc_i2c_stop(dev);
					//gpio_i2c_inactive(dev->adap.id);
#if defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
                                        clear_bit(MXC_I2C_IN_TRANSACTION, &(dev->status));
                                        writew(0x0, dev->membase + MXC_I2CR);
#else
					mxc_i2c_module_dis(dev);
#endif
					return rett;
				}
				addr_comp = 1;
			}
		}

		/* Transfer the data */
		if (msgs[i].flags & I2C_M_RD) {
			/* Read the data */
			ret = mxc_i2c_readbytes(dev, &msgs[i], (i + 1 == num),
						addr_comp);
			if (ret < 0) {
				printk(KERN_ERR "mxc_i2c_readbytes: fail.\n");
				break;
			}
            
		} else {
			/* Write the data */
			ret = mxc_i2c_writebytes(dev, &msgs[i], (i + 1 == num));
			if (ret < 0) {
				printk(KERN_ERR "mxc_i2c_writebytes: fail.\n");
				break;
			}
		}
	}
	//gpio_i2c_inactive(dev->adap.id);
#if defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
	mxc_i2c_stop(dev);
        clear_bit(MXC_I2C_IN_TRANSACTION, &(dev->status));
        writew(0x0, dev->membase + MXC_I2CR);
#else
	mxc_i2c_module_dis(dev);
#endif
	/*
	 * Decrease by 1 as we do not want Start message to be included in
	 * the count
	 */
	return (ret < 0) ? ret : (i - 1);
}

/*!
 * Returns the i2c functionality supported by this driver.
 *
 * @param   adap adapter structure for this i2c device
 *
 * @return Returns the functionality that is supported.
 */
static u32 mxc_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_QUICK;
}

/*!
 * Stores the pointers for the i2c algorithm functions. The algorithm functions
 * is used by the i2c bus driver to talk to the i2c bus
 */
static struct i2c_algorithm mxc_i2c_algorithm = {
	.name = "MXC algorithm",
	.master_xfer = mxc_i2c_xfer,
	.functionality = mxc_i2c_func
};

/*!
 * Interrupt Service Routine. It signals to the process about the data transfer
 * completion. Also sets a flag if bus arbitration is lost.
 * @param   irq    the interrupt number
 * @param   dev_id driver private data
 * @param   regs   holds a snapshot of the processor's context before the
 *                 processor entered the interrupt code
 *
 * @return  The function returns \b IRQ_HANDLED.
 */
static irqreturn_t mxc_i2c_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	mxc_i2c_device *dev = dev_id;
	volatile unsigned int sr, cr;

	sr = readw(dev->membase + MXC_I2SR);
	cr = readw(dev->membase + MXC_I2CR);

	/*
	 * Clear the interrupt bit
	 */
	writew(0x0, dev->membase + MXC_I2SR);

	if (sr & MXC_I2SR_IAL) {
		/*In single master mode,arbitration lost is caused by one of i2c device malfunction or SCL and SDA are in wrong condition,need to wake up work queue,and return an error*/
		transfer_done = true;
		arbitration_lost = true;
		printk(KERN_ERR "Bus Arbitration lost\n");
                wake_up_interruptible(&dev->wq);
	} else {
		/* Interrupt due byte transfer completion */
		tx_success = false;
		/* Check if RXAK is received in Transmit mode */
		if ((cr & MXC_I2CR_MTX) && (!(sr & MXC_I2SR_RXAK))) {
			tx_success = true;
		}
		transfer_done = true;
		wake_up_interruptible(&dev->wq);
	}

	return IRQ_HANDLED;
}

/*!
 * This function is called to put the I2C adapter in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device structure used to give information on which I2C
 *                to suspend
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function returns 0 on success and -1 on failure.
 */
static int mxci2c_suspend(struct device *dev, u32 state, u32 level)
{
	mxc_i2c_device *mxcdev = dev_get_drvdata(dev);
	volatile unsigned int sr = 0;
#ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND
        if(test_bit(MXC_I2C_IN_TRANSACTION,  &(mxcdev->status))) {
                return -EBUSY;
        }
#endif

	if (mxcdev == NULL) {
		return -1;
	}

	if (!irqs_disabled()) {
		return -EAGAIN;
	}

	if (level == SUSPEND_DISABLE) {
		/* Prevent further calls to be processed */
		mxcdev->low_power = true;
		/* Wait till we finish the current transfer */
		sr = readw(mxcdev->membase + MXC_I2SR);
		while (sr & MXC_I2SR_IBB) {
			msleep(10);
			sr = readw(mxcdev->membase + MXC_I2SR);
		}
	} else if (level == SUSPEND_POWER_DOWN) {
		gpio_i2c_inactive(mxcdev->adap.id);
#if defined(CONFIG_MACH_ELBA) || !defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
                /* This call will always be made when we clear the TRANSACTION flag */
		mxc_i2c_module_dis(mxcdev);
#endif
	}
	return 0;
}

/*!
 * This function is called to bring the I2C adapter back from a low power state. Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device structure used to give information on which I2C
 *                to resume
 * @param   level the stage in device resumption process that we want the
 *                device to be put in
 *
 * @return  The function returns 0 on success and -1 on failure
 */
static int mxci2c_resume(struct device *dev, u32 level)
{
	mxc_i2c_device *mxcdev = dev_get_drvdata(dev);

	if (mxcdev == NULL) {
		return -1;
	}

	if (level == RESUME_ENABLE) {
		mxcdev->low_power = false;
	} else if (level == RESUME_POWER_ON) {
#if ! defined(CONFIG_MACH_PIANOSA) && defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
		mxc_i2c_module_en_clks(mxcdev);
#else
		/* This call will be made when we start another transaction */
		mxc_i2c_module_en(mxcdev, 0);
#endif
		gpio_i2c_active(mxcdev->adap.id);
	}
	return 0;
}

/*!
 * This function is called during the driver binding process.
 *
 * @param   dev   the device structure used to store device specific
 *                information that is used by the suspend, resume and remove
 *                functions
 *
 * @return  The function always returns 0.
 */
static int mxci2c_probe(struct device *dev)
{
	struct platform_device *platdev = to_platform_device(dev);
	int id = platdev->id;
	int i;

	for (i = 0; i < I2C_NR; i++) {
		/*
		 * Get the appropriate device structure for this I2C adapter
		 */
		if (mxc_i2c_devs[i].adap.id != id) {
			continue;
		}
		dev_set_drvdata(dev, &mxc_i2c_devs[i]);
	}
	return 0;
}

/*!
 * Dissociates the driver from the I2C device.
 *
 * @param   dev   the device structure used to give information on which I2C
 *                to remove
 *
 * @return  The function always returns 0.
 */
static int mxci2c_remove(struct device *dev)
{
#ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND
	mxc_i2c_device *mxcdev = dev_get_drvdata(dev);
#endif

	dev_set_drvdata(dev, NULL);
#if defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
	mxc_i2c_module_dis(mxcdev);
#endif
	
	return 0;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxci2c_driver = {
	.name = "mxci2c",
	.bus = &platform_bus_type,
	.probe = mxci2c_probe,
	.remove = mxci2c_remove,
	.suspend = mxci2c_suspend,
	.resume = mxci2c_resume,
};

/*! Device Definition for MXC I2C devices */
static struct platform_device mxci2c_devices[I2C_NR] = {
	[0] = {
	       .name = "mxci2c",
	       .id = 0,
	       },
#if I2C_NR > 1
	[1] = {
	       .name = "mxci2c",
	       .id = 1,
	       },
#if I2C_NR > 2
	[2] = {
	       .name = "mxci2c",
	       .id = 2,
	       }
#endif
#endif
};

#ifdef CONFIG_MOT_FEAT_DEVICE_TREE

/**
 * Path to node containing I2C clock divider
 */
static const char i2c_hwcfg_path[] __initdata = "/System@0/I2C@0";

#endif

/*!
 * Function requests the interrupts and registers the i2c adapter structures.
 *
 * @return The function returns 0 on success and a non-zero value on failure.
 */
static int __init mxc_i2c_init(void)
{
	int i, ret = 0, err = 0;
#ifdef CONFIG_MOT_FEAT_DEVICE_TREE
	unsigned int clkdiv = 0;
	MOTHWCFGNODE *node;
#endif

	printk(KERN_INFO "MXC I2C driver\n");

	/* Register the device driver structure. */
	ret = driver_register(&mxci2c_driver);
	if (ret != 0) {
		return ret;
	}

	for (i = 0; i < I2C_NR; i++) {
		/*
		 * Request the I2C interrupt
		 */
		ret = request_irq(mxc_i2c_devs[i].irq, mxc_i2c_handler,
				  0, "MXC I2C", &mxc_i2c_devs[i]);
		if (ret != 0) {
			printk(KERN_CRIT "mxc-i2c%d: failed to request i2c "
			       "interrupt\n", i);
			err = ret;
			continue;
		}

		init_waitqueue_head(&mxc_i2c_devs[i].wq);

		mxc_i2c_devs[i].low_power = false;
#ifdef CONFIG_MOT_FEAT_DEVICE_TREE
		/*
		 * Read the clock divider from hardware config tree
		 */
		if (node = mothwcfg_get_node_by_path(i2c_hwcfg_path)) {
			if (mothwcfg_read_prop(node, "clk_div", &clkdiv, sizeof(clkdiv))
			    == sizeof(clkdiv)) {
				mxc_i2c_devs[i].clkdiv = clkdiv;
			}
			mothwcfg_put_node(node);
		} else {
			/*
			 * If failed to get the I2C node, just use the
			 * default clock divider
			 */
			printk("%s:unable to read node \"%s\" from HWCFG\n",
			       __FUNCTION__, i2c_hwcfg_path);
		}
#endif
		/*
		 * Set the adapter information
		 */
		strcpy(mxc_i2c_devs[i].adap.name, MXC_ADAPTER_NAME);
		mxc_i2c_devs[i].adap.id = i;	/* Used by MXC I2C client */
		mxc_i2c_devs[i].adap.algo = &mxc_i2c_algorithm;
		mxc_i2c_devs[i].adap.timeout = 1;
#ifdef CONFIG_MOT_FEAT_I2C_BLK_SUSPEND
		mxc_i2c_devs[i].status = 0;	
#endif
		i2c_set_adapdata(&mxc_i2c_devs[i].adap, &mxc_i2c_devs[i]);

		if ((ret = i2c_add_adapter(&mxc_i2c_devs[i].adap)) != 0) {
			printk(KERN_CRIT "mxc-i2c%d: failed to register i2c "
			       "adapter\n", i);
			free_irq(mxc_i2c_devs[i].irq, &mxc_i2c_devs[i]);
			err = ret;
			continue;
		}
		/* Register the I2C device */
		ret = platform_device_register(&mxci2c_devices[i]);
		if (ret != 0) {
			printk(KERN_CRIT "mxc-i2c%d: failed to register i2c "
			       "platform device\n", i);
			free_irq(mxc_i2c_devs[i].irq, &mxc_i2c_devs[i]);
			err = ret;
		}
#if defined(CONFIG_MOT_FEAT_PM) && !defined(CONFIG_MACH_ELBA)
                mpm_i2c_dev_nums[i] = mpm_register_with_mpm(mpm_i2c_dev_names[i]);
                if(mpm_i2c_dev_nums[i] < 0) {
                        printk(KERN_ERR "mxc-i2c%d: failed to register with "
                               "mpm\n", i);
                        free_irq(mxc_i2c_devs[i].irq, &mxc_i2c_devs[i]);
                        err = mpm_i2c_dev_nums[i];
                }
#endif
#if ! defined(CONFIG_MACH_PIANOSA) && defined(CONFIG_MOT_FEAT_I2C_BLK_SUSPEND)
                mxc_i2c_module_en_clks(&mxc_i2c_devs[i]);
		mxc_i2c_module_en_regs(&mxc_i2c_devs[i], 0);
#endif
		/*
		 * Enable the I2C pins
		 */
		gpio_i2c_active(i);
	}

	return err;
}

/*!
 * This function is used to cleanup all resources before the driver exits.
 */
static void __exit mxc_i2c_exit(void)
{
	int i;

	for (i = 0; i < I2C_NR; i++) {
		gpio_i2c_inactive(i);
		free_irq(mxc_i2c_devs[i].irq, &mxc_i2c_devs[i]);
		i2c_del_adapter(&mxc_i2c_devs[i].adap);
		driver_unregister(&mxci2c_driver);
		platform_device_unregister(&mxci2c_devices[i]);
#if defined(CONFIG_MOT_FEAT_PM) && !defined(CONFIG_MACH_ELBA)
		mpm_unregister_with_mpm(mpm_i2c_dev_names[i]);
#endif
	}
}

#if defined(CONFIG_MOT_FEAT_RAW_I2C_API)
/*!
 * This function is called by mxc_i2c_raw_xfer() which is only used camera driver.
 * The camera driver might use higher I2c clock than other devices due to 
 * performance reuqirement. It is called when an camera driver wishes to transfer 
 * data to camera device.
 *
 * @param   adap   adapter structure for the MXC i2c device
 * @param   msgs[] array of messages to be transferred to the device
 * @param   num    number of messages to be transferred to the device
 * @param   clk    I2C clock rate in kHz
 *
 * @return  The function returns the number of messages transferred,
 *          \b -EREMOTEIO on I2C failure and a 0 if the num argument is
 *          less than 0.
 */
int mxc_cam_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg msgs[],
 			int num, int clk)
{
	mxc_i2c_device *dev = (mxc_i2c_device *) (i2c_get_adapdata(adap));
	int ret = 0;
	unsigned int orig_clkdiv = 0x0c;

	down(&adap->bus_lock);
	if (clk) {
		orig_clkdiv = dev->clkdiv;

		switch (clk) {
		case MXC_I2C_CLK_400K:
			dev->clkdiv = 0x01;
			break;
		case MXC_I2C_CLK_300K:
			dev->clkdiv = 0x27;
			break;
		case MXC_I2C_CLK_200K:
			dev->clkdiv = 0x2A;
			break;
		case MXC_I2C_CLK_180K:
			dev->clkdiv = 0x07;
			break;
		case MXC_I2C_CLK_90K:
			dev->clkdiv = 0x0c;
			break;
		default:
			dev->clkdiv = 0x0c;
			break;
		}
	}

	ret = mxc_i2c_xfer(adap, msgs, num);
	if (clk) {
		dev->clkdiv = orig_clkdiv;
	}
	up(&adap->bus_lock);

	return ret;
}
#endif

subsys_initcall(mxc_i2c_init);
module_exit(mxc_i2c_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC I2C driver");
MODULE_LICENSE("GPL");
