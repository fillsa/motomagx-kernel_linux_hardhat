/*
 * drivers/i2c/i2c-pnx.c
 *
 * Provides i2c support for PNX010x/PNX4008.
 *
 * Author: Dennis Kovalev <dkovalev@ru.mvista.com>
 *
 * 2004-2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/i2c-pnx.h>
#include <asm/arch/i2c.h>

/* Define this for debug messages */
#if I2C_PNX_DEBUG
char *i2c_pnx_modestr[] = {
	"Transmit",
	"Receive",
	"No Data",
};
#endif

#if defined (CONFIG_I2C_PNX_SLAVE_SUPPORT)

static int slave_addr0 = 0x1d;	/* slave Address */
static int slave_mode0 = 0x00;	/* mode of operation. default is master mode */
static int slave_addr1 = 0x1d;	/* slave Address */
static int slave_mode1 = 0x00;	/* mode of operation. default is master mode */

int
i2c_slave_recv_data(char *buf, int len, void *data)
{
	int count = 0;
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) data;

	if ((!buf) || (!len)) {
		pr_debug ("%s(): check buffer!\n", __FUNCTION__);
		return -1;
	}

	while (alg_data->slave->rfl && len) {
		buf [count++] = alg_data->slave->fifo.rx;
		len--;
	}

	alg_data->slave->ctl |= scntrl_drsie | scntrl_daie;

	return count;
}

int
i2c_slave_send_data(char *buf, int len, void *data)
{
	int i;
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) data;

	if (!buf) {
		pr_debug ("%s(): check buffer!\n", __FUNCTION__);
		return -1;
	}

	if (len > I2C_BUFFER_SIZE) {
		pr_debug ("%s(): provided buffer too large!\n", __FUNCTION__);
		return -2;
	}

	if (alg_data->buf_index < 0) 
	{
		alg_data->slave->ctl |= scntrl_reset;
		while (alg_data->slave->ctl & scntrl_reset);
	}
	else if (alg_data->buf_index > 0) {
		pr_debug ("%s(): another operation is in progress: index %d!\n", __FUNCTION__, alg_data->buf_index);
		return -3;
	}

	/* Reverse copy buffer */
	for (i = 0; i < len; i++)
		alg_data->buffer[i] = buf[len-i-1];

	alg_data->buf_index = len - 1;

	while (!(alg_data->slave->sts & sstatus_tffs) && (alg_data->buf_index >= 0)) 
		/* push data until FIFO fills up */
		alg_data->slave->txs = 0x000000FF & alg_data->buffer[alg_data->buf_index--];

	/* Enable 'Slave TX FIFO not full' interrupt if TX FIFO is full */
	if (alg_data->slave->sts & sstatus_tffs) {
		alg_data->slave->ctl |= scntrl_tffsie;
		len = 0;
	}

	alg_data->slave->ctl |= scntrl_drsie | scntrl_daie;

	pr_debug ("%s(): S ->> M\n", __FUNCTION__);

	return len;
}
#endif

static inline void
i2c_pnx_arm_timer(struct timer_list *timer, void *data)
{
	del_timer_sync(timer);

	pr_debug("%s: Timer armed at %lu plus %u jiffies.\n",
		__FUNCTION__, jiffies, TIMEOUT );

	timer->expires = jiffies + TIMEOUT;
	timer->data = (unsigned long) data;

	add_timer(timer);
}

static inline int
i2c_pnx_bus_busy(i2c_regs *master)
{
	long timeout;

	timeout = TIMEOUT * 10000;
	if (timeout > 0 && (master->sts & mstatus_active)) {
		udelay(500);
		timeout -= 500;
	}

	return (timeout <= 0) ? 1 : 0;
}

/******************************************************
 * function: i2c_pnx_start
 *
 * description: Generate a START signal in the desired mode.
 * I2C is the master.
 * Return 0 if no error.
 ****************************************************/
static int
i2c_pnx_start(unsigned char slave_addr, struct i2c_adapter *adap)
{
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) adap->algo_data;

#if I2C_PNX010X_DEBUG
	pr_debug("%s(%d): %s() addr 0x%x mode %s\n", __FILE__,
		__LINE__, __FUNCTION__, slave_addr,
		i2c_pnx_modestr[alg_data->mif.mode]);
#endif

	/* Check for 7 bit slave addresses only */
	if (slave_addr & ~0x7f) {
		printk(KERN_ERR "%s: Invalid slave address %x. "
			"Only 7-bit addresses are supported\n",
			adap->name, slave_addr);
		return -EINVAL;
	}

	/* First, make sure bus is idle */
	if (i2c_pnx_bus_busy(alg_data->master)) {
		/* Somebody else is monopolising the bus */
		printk(KERN_ERR "%s: Bus busy. Slave addr = %02x, "
			"cntrl = %x, stat = %x\n",
			adap->name, slave_addr,
			alg_data->master->ctl,
			alg_data->master->sts);
		return -EBUSY;
	} else if (alg_data->master->sts & mstatus_afi) {
		/* Sorry, we lost the bus */
		printk(KERN_ERR "%s: Arbitration failure. "
			"Slave addr = %02x\n",
			adap->name, slave_addr);
			return -EIO;
	}

	/* OK, I2C is enabled and we have the bus. */
	/* Clear the current TDI and AFI status flags. */
	alg_data->master->sts |= mstatus_tdi | mstatus_afi;

	pr_debug ("%s(%d): %s() sending %#x\n", __FILE__, __LINE__,
		__FUNCTION__, (slave_addr << 1) | start_bit |
		(alg_data->mif.mode == rcv ? rw_bit : 0));

	/* Write the slave address, START bit and R/W bit */
	alg_data->master->fifo.tx = (slave_addr << 1) | start_bit |
		(alg_data->mif.mode == rcv ? rw_bit: 0);

	pr_debug ("%s(%d): %s() exit\n", __FILE__, __LINE__, __FUNCTION__);

	return 0;
}

/***********************************************************
 * function: i2c_pnx_stop
 *
 * description: Generate a STOP signal to terminate the master
 * transaction.
 * Always returns 0.
 **********************************************************/
static int
i2c_pnx_stop(i2c_pnx_algo_data_t *alg_data)
{
	pr_debug ("%s: %s() enter - stat = %04x.\n",
		__FILE__, __FUNCTION__, alg_data->master->sts);

	/* Write a STOP bit to TX FIFO */
	alg_data->master->fifo.tx = 0x000000ff | stop_bit;

	/* Wait until the STOP is seen. */
	while (alg_data->master->sts & mstatus_active);

	pr_debug ("%s: %s() exit - stat = %04x.\n",
		__FILE__, __FUNCTION__, alg_data->master->sts);

	return 0;
}

/****************************************************
 * function: i2c_pnx_master_xmit
 *
 * description: Master sends one byte of data to
 * slave target
 * return 0 if the byte transmitted.
 ***************************************************/
static int
i2c_pnx_master_xmit(i2c_pnx_algo_data_t *alg_data)
{
	u32 val;

	pr_debug ("%s(): Entered\n", __FUNCTION__);

	if (alg_data->mif.len > 0) {
		/* We still have somthing to talk about... */
		val = *alg_data->mif.buf++;
		val &= 0x000000ff;

		if (alg_data->mif.len == 1) {
			/* Last byte, issue a stop */
			val |= stop_bit;
		}

		alg_data->mif.len--;
		alg_data->master->fifo.tx = val;

		pr_debug ("%s: xmit %#x [%d]\n", __FUNCTION__, val, alg_data->mif.len + 1);

		if (alg_data->mif.len == 0) {
			/* Wait until the STOP is seen. */
			while (alg_data->master->sts & mstatus_active);

			/* Disable master interrupts */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_drmie);

			del_timer_sync(&alg_data->mif.timer);

			pr_debug("%s: Waking up xfer routine.\n", __FUNCTION__);

			complete(&alg_data->mif.complete);
		}
	} else /* For zero-sized transfers. */
		if (alg_data->mif.len == 0) {
			/* Isue a stop. */
			i2c_pnx_stop(alg_data);

			/* Disable master interrupts. */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_drmie);

			/* Stop timer. */
			del_timer_sync(&alg_data->mif.timer);
			pr_debug("%s: Waking up xfer routine after zero-xfer.\n", __FUNCTION__);

			complete(&alg_data->mif.complete);
		}

	pr_debug ("%s(): Exit\n", __FUNCTION__);

	return 0;
}

/***********************************************
 * function: i2c_pnx_master_rcv
 *
 * description: master reads one byte data
 * from slave source
 * return 0 if no error
 ***********************************************/
static int
i2c_pnx_master_rcv(i2c_pnx_algo_data_t *alg_data)
{
	unsigned int val = 0;

	pr_debug ("%s(): Entered\n", __FUNCTION__);

	/* Check, whether there is already data,
	 * or we didn't 'ask' for it yet.
	 */
	if (alg_data->master->sts & mstatus_rfe) {
		pr_debug ("%s(): Write dummy data to fill Rx-fifo...\n", __FUNCTION__);

		if (alg_data->mif.len == 1) {
			/* Last byte, do not acknowledge next rcv. */
			val |= stop_bit;

			/* Enable interrupt RFDAIE (= data in Rx-fifo.),
			 * and disable DRMIE (= need data for Tx.)
			 */
			alg_data->master->ctl |= mcntrl_rffie | mcntrl_daie;
			alg_data->master->ctl &= ~(mcntrl_drmie);
		}

		/* Now we'll 'ask' for data:
		 * For each byte we want to receive, we must
		 * write a (dummy) byte to the Tx-FIFO.
		 */
		alg_data->master->fifo.tx = val;

		return 0;
	}

	/* Handle data. */
	if (alg_data->mif.len > 0) {

		pr_debug ("%s(): Get data from Rx-fifo...\n", __FUNCTION__);

		val = alg_data->master->fifo.rx;
		*alg_data->mif.buf++ = (u8)(val & 0xff);
		pr_debug ("%s: rcv 0x%x [%d]\n", __FUNCTION__, val, alg_data->mif.len);

		alg_data->mif.len--;
		if (alg_data->mif.len == 0) {
			/* Wait until the STOP is seen. */
			while (alg_data->master->sts & mstatus_active);

			/* Disable master interrupts */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_rffie | mcntrl_drmie | mcntrl_daie);

			/* Kill timer. */
			del_timer_sync(&alg_data->mif.timer);
			pr_debug("%s: Waking up xfer routine after rcv.\n", __FUNCTION__);

			complete(&alg_data->mif.complete);
		}
	}

	pr_debug ("%s: Exit\n", __FUNCTION__);

	return 0;
}

static irqreturn_t
i2c_pnx_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long stat;
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *)dev_id;

	if (alg_data->mode == slave) {
#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
		/* process slave-related events */
		pr_debug ("%s(): sstat = %x sctrl = %x\n", __FUNCTION__,
			alg_data->slave->sts, alg_data->slave->ctl);
		stat = alg_data->slave->sts;

		/* Now let's see what kind of event this is */
		if (stat & sstatus_afi) {
			/* We lost arbitration in the midst of a transfer */
			pr_debug ("%s(): Arbitration failed!\n", __FUNCTION__);
			alg_data->slave->sts |= sstatus_afi; /* clear AFI flag */
		}

		if (stat & sstatus_nai) {
			/* Master did not acknowledge our transfer */
			pr_debug ("%s(): Master did not acknowledge!\n", __FUNCTION__);
		}

		if ((!(stat & sstatus_tffs)) && (!(stat & sstatus_tfes))) {
			/* We can push some more data into slave TX FIFO */
			pr_debug ("%s(): filling up slave FIFO.\n", __FUNCTION__);
			if (!alg_data->buf_index)
				/* no more need in this interrupt */
				alg_data->slave->ctl &= ~scntrl_tffsie;

			alg_data->slave->txs = 0x000000FF
				& alg_data->buffer [alg_data->buf_index--];
		}

		if (!(stat & sstatus_rfe)) {
			/* RX FIFO contains data to process */
			pr_debug ("%s(): S <<- M\n", __FUNCTION__);
			if (alg_data->slave_recv_cb)
				alg_data->slave_recv_cb (alg_data);
			else
				pr_debug ("%s(): no slave receive callback installed!\n", __FUNCTION__);
		} else if (stat & sstatus_drsi) {
			/* I2C master requests data */
			/* request data from higher level code and send it */
			if (alg_data->slave_send_cb)
				alg_data->slave_send_cb (alg_data);
			else
				pr_debug ("%s(): no slave send callback installed!\n", __FUNCTION__);
		}
#endif
	} else {
		/* process master-related events */
#if I2C_PNX010X_DEBUG
		pr_debug ("%s(): mstat = %x mctrl = %x, mmode = %s\n", __FUNCTION__,
			alg_data->master->sts, alg_data->master->ctl,
			i2c_pnx_modestr[alg_data->mif.mode]);
#endif
		stat = alg_data->master->sts;

		/* Now let's see what kind of event this is */
		if (stat & mstatus_afi) {
			/* We lost arbitration in the midst of a transfer */
			alg_data->mif.ret = -EIO;

			/* Disable master interrupts. */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_drmie | mcntrl_rffie);

			/* Stop timer, to prevent timeout. */
			del_timer_sync(&alg_data->mif.timer);
			complete(&alg_data->mif.complete);
		} else if (stat & mstatus_nai) {
			/* Slave did not acknowledge, generate a STOP */
			pr_debug ("%s(): Slave did not acknowledge, generating a STOP.\n", __FUNCTION__);
			i2c_pnx_stop(alg_data);

			/* Disable master interrupts. */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_drmie | mcntrl_rffie);

			/* Our return value. */
			alg_data->mif.ret = -EFAULT;

			/* Stop timer, to prevent timeout. */
			del_timer_sync(&alg_data->mif.timer);
			complete(&alg_data->mif.complete);
		} else
			/* Two options:
			 * - Master Tx needs data.
			 * - There is data in the Rx-fifo
			 * The latter is only the case if we have requested for data,
			 * via a dummy write. (See 'i2c_pnx_master_rcv'.)
			 * We therefore check, as a sanity check, whether that interrupt
			 * has been enabled.
			 */
			if ((stat & mstatus_drmi) || !(stat & mstatus_rfe)) {
				if (alg_data->mif.mode == xmit) {
					i2c_pnx_master_xmit(alg_data);
				} else if (alg_data->mif.mode == rcv) {
					i2c_pnx_master_rcv(alg_data);
				}
			}

			/* Clear TDI and AFI bits by writing 1's in the respective position. */
			alg_data->master->sts |= mstatus_tdi | mstatus_afi;
	}

	pr_debug ("%s(): Exit, stat = %x ctrl = %x.\n",
		__FUNCTION__, alg_data->master->sts, alg_data->master->ctl);

	return IRQ_HANDLED;
}

static void
i2c_pnx_timeout(unsigned long data)
{
	i2c_pnx_algo_data_t* alg_data = (i2c_pnx_algo_data_t *)data;

	printk(KERN_ERR
		"Master timed out. stat = %04x, cntrl = %04x. Resetting master ...\n",
		alg_data->master->sts, alg_data->master->ctl);

	/* Reset master and disable interrupts */
	alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie | mcntrl_drmie | mcntrl_rffie);
	alg_data->master->ctl |= mcntrl_reset;
	while (alg_data->master->ctl & mcntrl_reset);

	alg_data->mif.ret = -EIO;

	complete(&alg_data->mif.complete);
}

static int
i2c_pnx_ioctl(struct i2c_adapter *adap, unsigned int cmd, unsigned long arg)
{
	printk( KERN_ERR "%s: IOCTL-functionality not implemented!\n", adap->name);
	return 0;
}

/*
 * Generic transfer entrypoint
 */
static int
i2c_pnx_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct i2c_msg *pmsg;
	int rc = 0, completed = 0, i;
	i2c_pnx_algo_data_t * alg_data = (i2c_pnx_algo_data_t *) adap->algo_data;

	pr_debug ("%s: Entered\n", __FUNCTION__);

	/* Give an error if we're suspended. */
	if (adap->dev.parent->power.power_state)
		return -EBUSY;
		
	down(&alg_data->mif.sem);

	/* If the bus is still active, or there is already
	 * data in one of the fifo's, there is something wrong.
	 * Repair this by a reset ...
	 */
	if ((alg_data->master->sts & mstatus_active)) {
		alg_data->master->ctl |= mcntrl_reset;
		while (alg_data->master->ctl & mcntrl_reset);
	} else if (!(alg_data->master->sts & mstatus_rfe) ||
		!(alg_data->master->sts & mstatus_tfe)) {
		printk(KERN_ERR
			"%s: FIFO's contain already data before xfer. Reset it ...\n",
			adap->name);

		alg_data->master->ctl |= mcntrl_reset;
		while (alg_data->master->ctl & mcntrl_reset);
	}
 
	/* Process transactions in a loop. */
	for (i = 0; rc >= 0 && i < num;) {
		u8 addr;

		pmsg = &msgs[i++];
		addr = pmsg->addr;

		if (pmsg->flags & I2C_M_TEN) {
			printk(KERN_ERR
				"%s: 10 bits addr not supported !\n", adap->name);
			rc = -EINVAL;
			break;
		}
 
		/* Check address for sanity. */
		if (alg_data->slave && addr == alg_data->slave->adr) {
			printk("%s: An attempt was made to access our own slave address! :-)\n",
				adap->name);
			rc = -EINVAL;
			break;
		}

		/* Set up address and r/w bit */
		if (pmsg->flags & I2C_M_REV_DIR_ADDR) {
			addr ^= 1;
		}

		alg_data->mif.buf = pmsg->buf;
		alg_data->mif.len = pmsg->len;
		alg_data->mif.mode = (pmsg->flags & I2C_M_RD) ? rcv : xmit;
		alg_data->mif.ret = 0;

#if I2C_PNX010X_DEBUG
		pr_debug("%s: %s mode, %d bytes\n", __FUNCTION__,
			i2c_pnx_modestr[alg_data->mif.mode], alg_data->mif.len);
#endif

		/* Arm timer */
		i2c_pnx_arm_timer(&alg_data->mif.timer, (void *) alg_data);

		/* initialize the completion var */	
		init_completion(&alg_data->mif.complete);
		
		/* Enable master interrupt */
		alg_data->master->ctl = mcntrl_afie | mcntrl_naie | mcntrl_drmie;

		/* Put start-code and slave-address on the bus. */
		rc = i2c_pnx_start(addr, adap);
		if (0 != rc) {
			break;
		}
 
		pr_debug("%s(%d): stat = %04x, cntrl = %04x.\n",
			__FUNCTION__, __LINE__, alg_data->master->sts, alg_data->master->ctl);

		/* Wait for completion */
		wait_for_completion(&alg_data->mif.complete);

		if (!(rc = alg_data->mif.ret))
			completed++;
		pr_debug("%s: Return code = %d.\n", __FUNCTION__, rc);

		/* Clear TDI and AFI bits in case they are set. */
		if (alg_data->master->sts & mstatus_tdi) {
			printk("%s: TDI still set ... clearing now.\n", adap->name);
			alg_data->master->sts |= mstatus_tdi;
		}
		if (alg_data->master->sts & mstatus_afi) {
			printk("%s: AFI still set ... clearing now.\n", adap->name);
			alg_data->master->sts |= mstatus_afi;
		}
	}

	/* If the bus is still active, reset it to prevent
	 * corruption of future transactions.
	 */
	if ((alg_data->master->sts & mstatus_active)) {
		printk("%s: Bus is still active after xfer. Reset it ...\n",
			adap->name);
		alg_data->master->ctl |= mcntrl_reset;
		while (alg_data->master->ctl & mcntrl_reset);
	} else
		/* If there is data in the fifo's after transfer,
		 * flush fifo's by reset.
		 */
		if (!(alg_data->master->sts & mstatus_rfe) ||
			!(alg_data->master->sts & mstatus_tfe)) {
			alg_data->master->ctl |= mcntrl_reset;
			while (alg_data->master->ctl & mcntrl_reset);
		} else if ((alg_data->master->sts & mstatus_nai)) {
			alg_data->master->ctl |= mcntrl_reset;
			while (alg_data->master->ctl & mcntrl_reset);
		}

	/* Cleanup to be sure ... */
	alg_data->mif.buf = NULL;
	alg_data->mif.len = 0;

	/* Release sem */
	up(&alg_data->mif.sem);
	pr_debug ("%s: Exit\n", __FUNCTION__);

	if (completed != num) {
		return ((rc<0) ? rc : -EREMOTEIO);
	}
	return num ;
}

static u32
i2c_pnx_func(struct i2c_adapter * adapter)
{
	return	I2C_FUNC_I2C |
		I2C_FUNC_SMBUS_WRITE_BYTE_DATA | I2C_FUNC_SMBUS_READ_BYTE_DATA |
		I2C_FUNC_SMBUS_WRITE_WORD_DATA | I2C_FUNC_SMBUS_READ_WORD_DATA |
		I2C_FUNC_SMBUS_QUICK ;
}

/* For now, we only handle combined mode (smbus) */
static struct i2c_algorithm pnx_algorithm = {
	name:		CHIP_NAME,
	id:		I2C_ALGO_SMBUS,
	master_xfer:	i2c_pnx_xfer,
	functionality:	i2c_pnx_func,
	algo_control:	i2c_pnx_ioctl,
};


#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
int
i2c_slave_set_recv_cb(void (*callback)(void *), unsigned int adap)
{
  i2c_pnx_algo_data_t *alg_data;
  alg_data = (i2c_pnx_algo_data_t *) pnx_i2c_adapters[adap]->algo_data;


	alg_data->slave_recv_cb = callback;
	pr_debug ("%s: callback installed\n", __FUNCTION__);

	return 0;
}

int
i2c_slave_set_send_cb(void (*callback)(void *), unsigned int adap)
{
  i2c_pnx_algo_data_t *alg_data;
  alg_data = (i2c_pnx_algo_data_t *) pnx_i2c_adapters[adap]->algo_data;

	alg_data->slave_send_cb = callback;
	pr_debug ("%s: callback installed\n", __FUNCTION__);

	return 0;
}

int
i2c_slave_push_data(char* buf, int len, int adap)
{
	i2c_pnx_algo_data_t *alg_data;

	alg_data =
		(i2c_pnx_algo_data_t *) pnx_i2c_adapters[adap]->algo_data;

	/* flush the fifo */
	alg_data->slave->ctl |= mcntrl_reset;
	while (alg_data->slave->ctl & mcntrl_reset);

	/* send the data through */
	len = i2c_slave_send_data( buf, len, alg_data);

	return len;
}
#endif

static int i2c_pnx_controller_suspend(struct device *dev, u32 state, u32 level)
{

	return i2c_pnx_suspend(dev, state, level);
}

static int i2c_pnx_controller_resume(struct device *dev, u32 level)
{
	return i2c_pnx_resume(dev, level);
}

static int i2c_pnx_probe(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	unsigned long tmp;
	int ret;
	i2c_pnx_algo_data_t * alg_data;
	int freq_mhz;
#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
	int *m_params_mode[] = {&slave_mode0, &slave_mode1};
	int *m_params_address[] = {&slave_addr0, &slave_addr1};
#endif
	
	freq_mhz = calculate_input_freq(dev);
	
	if (pdev->id > ARRAY_SIZE(pnx_i2c_adapters)) {
		printk(KERN_ERR "I2C device %d is not defined.\n", pdev->id);
		return -ENODEV;
	}

	dev_set_drvdata(dev, pnx_i2c_adapters[pdev->id]);

	alg_data = (i2c_pnx_algo_data_t *) pnx_i2c_adapters[pdev->id]->algo_data;

#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
	if (*m_params_mode[pdev->id]) 
		alg_data->mode = slave;
	else 
		alg_data->mode = master;
	alg_data->slave_addr = *m_params_address[pdev->id];
#endif

	/* Internal Data structures for Master */
	if (alg_data->mode == master) {
		memset(&alg_data->mif, 0, sizeof(alg_data->mif));
		init_MUTEX(&alg_data->mif.sem);
		init_timer(&alg_data->mif.timer);
		alg_data->mif.timer.function = i2c_pnx_timeout;
		alg_data->mif.timer.data = (unsigned long) alg_data;
	} else {
		/* Init I2C buffer */
		memset (&alg_data->buffer, 0, I2C_BUFFER_SIZE);
		alg_data->buf_index = 0;
	}

	/* Register I/O resource */
	if (!request_region(alg_data->base, I2C_BLOCK_SIZE, "i2c-pnx")) {
		printk(KERN_ERR
		       "I/O region 0x%08x for I2C already in use.\n",
		       alg_data->base);
		return -ENODEV;
	} 

	set_clock_run(dev);

	/* Clock Divisor High This value is the number of system clocks
	 * the serial clock (SCL) will be high. n is defined at compile
	 * time based on the system clock (PCLK) and minimum serial frequency.
	 * For example, if the system clock period is 50 ns and the maximum
	 * desired serial period is 10000 ns (100 kHz), then CLKHI would be
	 * set to 0.5*(f_sys/f_i2c)-2=0.5*(20e6/100e3)-2=98. The actual value
	 * programmed into CLKHI will vary from this slightly due to
	 * variations in the output pad s rise and fall times as well as
	 * the deglitching filter length. In this example, n = 7, since
	 * eight bits are needed to hold the clock divider count.
	 */

	if (alg_data->mode == master) {
		tmp = ((freq_mhz * 1000) / I2C_PNX010X_SPEED_KHZ)/2 - 2;
		alg_data->master->ckh = tmp;
		alg_data->master->ckl = tmp;
	}
	else if (alg_data->slave_addr != 0xff) {
		alg_data->slave->adr = alg_data->slave_addr & 0x7f;
		if (alg_data->slave->adr != (alg_data->slave_addr & 0x7f))
			printk(KERN_ERR "%s: "
			       "Failed to program the slave to address %u.\n",
			       pnx_i2c_adapters[pdev->id]->name, (alg_data->slave_addr & 0x7f));
	}
	/* Master/Slave interrupts init */
	if (alg_data->mode == slave)
		alg_data->slave->ctl = scntrl_drsie | scntrl_daie;
	else
		alg_data->master->ctl = 0;

	/* Reset the I2C Master/Slave. The reset bit is self clearing. */
	if (alg_data->mode == slave) {
		alg_data->slave->ctl |= scntrl_reset;
		while ((alg_data->slave->ctl & scntrl_reset));
	} else {
		alg_data->master->ctl |= mcntrl_reset;
		while ((alg_data->master->ctl & mcntrl_reset));
	}

	if (alg_data->mode != slave) {
		/* initialize the completion var */	
		init_completion(&alg_data->mif.complete);
	}

	ret = request_irq(alg_data->irq, i2c_pnx_interrupt, 0,
			  alg_data->mode == slave? CHIP_NAME" Slave":CHIP_NAME" Master",
			  alg_data);
	if (ret) {
		set_clock_stop(dev);
		release_region(alg_data->base, I2C_BLOCK_SIZE);
		return ret;
	}

	/* Register this adapter with the I2C subsystem */
	if (alg_data->mode == master) {
		pnx_i2c_adapters[pdev->id]->dev.parent = dev;
		ret = i2c_add_adapter(pnx_i2c_adapters[pdev->id]);
		if (ret < 0) {
			printk(KERN_INFO "I2C: Failed to add bus\n");
			free_irq(alg_data->irq, alg_data);
			set_clock_stop(dev);
			release_region(alg_data->base, I2C_BLOCK_SIZE);
			return ret;
		}
	}

	printk(KERN_INFO "%s: %s at %#8x, irq %d.\n",
	       pnx_i2c_adapters[pdev->id]->name, alg_data->mode == slave?"Slave":"Master",
	       alg_data->base, alg_data->irq);

	return 0;
}

static int i2c_pnx_remove(struct device *dev)
{
	struct i2c_adapter *adap= (struct i2c_adapter *)dev_get_drvdata(dev);
	i2c_pnx_algo_data_t * alg_data =
		(i2c_pnx_algo_data_t *) adap->algo_data;

	/* Reset the I2C controller. The reset bit is self clearing. */
	if (alg_data->mode == slave) {
		alg_data->slave->ctl |= scntrl_reset;
		while ((alg_data->slave->ctl & scntrl_reset));
	} else {
		alg_data->master->ctl |= mcntrl_reset;
		while ((alg_data->master->ctl & mcntrl_reset));
	}

	free_irq(alg_data->irq, alg_data );
	if (alg_data->mode == master) {
		i2c_del_adapter(adap);
	}
	set_clock_stop(dev);
	release_region(alg_data->base, I2C_BLOCK_SIZE);

	return 0;
}

static struct device_driver i2c_pnx_driver = {
	.name		= "pnx-i2c",
	.bus		= &platform_bus_type,
	.probe		= i2c_pnx_probe,
	.remove		= i2c_pnx_remove,
	.suspend        = i2c_pnx_controller_suspend,
	.resume         = i2c_pnx_controller_resume,
};

static int __init i2c_adap_pnx_init(void)
{
	return driver_register(&i2c_pnx_driver);
}

static void i2c_adap_pnx_exit(void)
{
	return driver_unregister(&i2c_pnx_driver);
}

/* The MODULE_* macros resolve to nothing if MODULES is not defined
 * when this file is compiled.
 */
MODULE_AUTHOR("Dennis Kovalev <dkovalev@ru.mvista.com>");
MODULE_DESCRIPTION("I2C driver for PNX bus");
MODULE_LICENSE("GPL");

MODULE_PARM(slave_addr0, "i");
MODULE_PARM(slave_mode0, "i");
MODULE_PARM(slave_addr1, "i");
MODULE_PARM(slave_mode1, "i");
module_init(i2c_adap_pnx_init);
module_exit(i2c_adap_pnx_exit);

#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
EXPORT_SYMBOL(i2c_slave_recv_data);
EXPORT_SYMBOL(i2c_slave_send_data);
EXPORT_SYMBOL(i2c_slave_set_send_cb);
EXPORT_SYMBOL(i2c_slave_set_recv_cb);
EXPORT_SYMBOL(i2c_slave_push_data);
#endif
