/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2007 Motorola, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * DATE          AUTHOR         COMMMENT
 * ----          ------         --------
 * 10/04/2006    Motorola       Added support for MU polling.
 *                              Added support for AP BP sleep negotiation.
 *                              Added ioctls for dynamic byte swapping.
 *                              Added ioctl for MU testing.
 *                              Fixed bug documented by WFN379.
 *                              Fixed bug documented by WFN403.
 *                              Fixed bug documented by WFN404.
 *                              Fixed bug documented by WFN349.
 * 12/01/2006    Motorola       Leave RX3 int enabled when suspend is called
 * 04/20/2007    Motorola       Add SDMA dump ioctl
 * 12/14/2007    Motorola       Add a new ioctl to dump 24 SDMA registers
 */

/*!
 * @file mxc_mu.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for the message transfer between MCU and DSP core.
 *
 * The Interfaces are with respect to the MCU core. Any driver on the MCU side
 * can transfer messages to the DSP side using the interfaces implemented here.
 *
 * @ingroup MU
 */

/*
 * Include Files
 */
#include <linux/types.h>
#include <linux/byteorder/swab.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/circ_buf.h>
#include <linux/device.h>

#ifdef CONFIG_MOT_WFN374
#include <linux/poll.h>
#endif

#include <asm/arch/mxc_mu.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include "mxc_mu_reg.h"

#ifdef CONFIG_MOT_FEAT_SDMA_DEBUG
#include <asm/arch/mxc_ipc.h>
#endif

#ifdef CONFIG_MOT_FEAT_PM
#include <linux/mpm.h>
#endif

#define DVR_VER "2.0"
static struct class_simple *mxc_mu_class;
static int mu_major;

/*
 * Used to calculate number of bytes in the read buffer
 */
#define GET_READBYTES(a) CIRC_CNT(mu_rbuf[a].cbuf.head, \
                                  mu_rbuf[a].cbuf.tail, MAX_BUF_SIZE)

/*
 * Used to calculate number of bytes in the write buffer
 */
#define GET_WRITEBYTES(a) CIRC_CNT(mu_wbuf[a].cbuf.head, \
                                   mu_wbuf[a].cbuf.tail, MAX_BUF_SIZE)

/*
 * Used to calculate empty space in the read buffer
 */
#define GET_READSPACE(a) CIRC_SPACE(mu_rbuf[a].cbuf.head, \
                                    mu_rbuf[a].cbuf.tail, MAX_BUF_SIZE)

/*
 * Used to calculate empty space in the write buffer
 */
#define GET_WRITESPACE(a) CIRC_SPACE(mu_wbuf[a].cbuf.head, \
                                     mu_wbuf[a].cbuf.tail, MAX_BUF_SIZE)

/*!
 * There are 4 callback functions, one for each receive
 * interrupt. When 'bind' function is called the callback
 * functions are stored in an array and retrieved in the
 * tasklets when a receive interrupt arrives
 */
static callbackfn rxcallback[NUM_MU_CHANNELS];

/*!
 * There are 4 callback functions, one for each transmit
 * interrupt. When 'bind' function is called the callback
 * functions are stored in an array and retrieved in the
 * tasklets when a transmit interrupt arrives
 */
static callbackfn txcallback[NUM_MU_CHANNELS];

/*!
 * There are 4 callback functions, one for each general purpose
 * interrupt. When 'bind' function is called the callback
 * functions are stored in an array and retrieved in the
 * tasklets when a general purpose interrupt arrives
 */
static callbackfn gpcallback[NUM_MU_CHANNELS];

/*!
 * Declaring lock for mutual exclusion
 */
static DEFINE_RAW_SPINLOCK(mu_lock);

/*!
 * Declaring lock to prevent access to
 * allocated registers
 */
static DEFINE_RAW_SPINLOCK(alloc_lock);

/*!
 * Number of users waiting in suspendq
 */
static int swait = 0;

/*!
 * To indicate whether any of the MU devices are suspending
 */
static int suspend_flag = 0;

#ifndef CONFIG_MOT_WFN374

/*!
 * Indicate block mode - 0 and non block mode - 1. Default is non-block mode
 */
static int blockmode[NUM_MU_CHANNELS] = { 1, 1, 1, 1 };

#endif

/*!
 * Device Kernel Buffers. Each channel has one read and write buffer.
 */
typedef struct {
	/*!
	 * Buffer used to store data that was read or the data that is
	 * to be written
	 */
	struct circ_buf cbuf;
#ifndef CONFIG_MOT_WFN374
	/*!
	 * to indicate number of users waiting on blocking mode of the device
	 */
	int pwait;
#endif


#ifdef CONFIG_MOT_WFN377

        /*!
         * Flags to control the behavior of the ring buffer
         */
        unsigned int flags;

#endif

} ring_buffer;

ring_buffer mu_rbuf[NUM_MU_CHANNELS], mu_wbuf[NUM_MU_CHANNELS];

/*!
 * reg_allocated[] stores channel handlers for the allocated registers.
 */
static int reg_allocated[NUM_MU_CHANNELS] = { -1, -1, -1, -1 };

static unsigned int wbits[NUM_MU_CHANNELS] = { AS_MUMSR_MTE0, AS_MUMSR_MTE1,
	AS_MUMSR_MTE2, AS_MUMSR_MTE3
};

static unsigned int rbits[NUM_MU_CHANNELS] = { AS_MUMSR_MRF0, AS_MUMSR_MRF1,
	AS_MUMSR_MRF2, AS_MUMSR_MRF3
};

static unsigned int read_en[NUM_MU_CHANNELS] = { AS_MUMCR_MRIE0, AS_MUMCR_MRIE1,
	AS_MUMCR_MRIE2, AS_MUMCR_MRIE3
};

static unsigned int write_en[NUM_MU_CHANNELS] =
    { AS_MUMCR_MTIE0, AS_MUMCR_MTIE1,
	AS_MUMCR_MTIE2, AS_MUMCR_MTIE3
};

static unsigned int genp_en[NUM_MU_CHANNELS] = { AS_MUMCR_MGIE0, AS_MUMCR_MGIE1,
	AS_MUMCR_MGIE2, AS_MUMCR_MGIE3
};

static unsigned int genp_pend[NUM_MU_CHANNELS] =
    { AS_MUMSR_MGIP0, AS_MUMSR_MGIP1,
	AS_MUMSR_MGIP2, AS_MUMSR_MGIP3
};

static unsigned int genp_req[NUM_MU_CHANNELS] =
    { AS_MUMCR_MGIR0, AS_MUMCR_MGIR1,
	AS_MUMCR_MGIR2, AS_MUMCR_MGIR3
};

static unsigned int read_reg[NUM_MU_CHANNELS] = { AS_MUMRR0, AS_MUMRR1,
	AS_MUMRR2, AS_MUMRR3
};

static unsigned int write_reg[NUM_MU_CHANNELS] = { AS_MUMTR0, AS_MUMTR1,
	AS_MUMTR2, AS_MUMTR3
};

/*!
 * The readq and waitq are used by blocking read/write
 */
static wait_queue_head_t readq, writeq, suspendq;

/*
 * Declaring Bottom-half handlers - one for GP, RX and TX interrupt lines
 */
static void mxc_mu_rxhandler(unsigned long someval);
static void mxc_mu_txhandler(unsigned long someval);
static void mxc_mu_gphandler(unsigned long someval);

#ifdef CONFIG_MOT_WFN379
static unsigned long save_status;
#endif

#ifdef CONFIG_MOT_FEAT_PM
/* normally, we want to report interrupts of interest */
/* unless we are explicitly told to ignore them       */
static unsigned int ioi_switch = 1;
#endif

/*
 * Tasklets Declarations
 */
/*
 * Declaring tasklet to handle receive interrupts
 */
DECLARE_TASKLET(rxch0tasklet, (void *)mxc_mu_rxhandler, (unsigned long)0);
DECLARE_TASKLET(rxch1tasklet, (void *)mxc_mu_rxhandler, (unsigned long)1);
DECLARE_TASKLET(rxch2tasklet, (void *)mxc_mu_rxhandler, (unsigned long)2);
DECLARE_TASKLET(rxch3tasklet, (void *)mxc_mu_rxhandler, (unsigned long)3);

/*
 * Declaring tasklet to handle transmit interrupts
 */
DECLARE_TASKLET(txch0tasklet, (void *)mxc_mu_txhandler, (unsigned long)0);
DECLARE_TASKLET(txch1tasklet, (void *)mxc_mu_txhandler, (unsigned long)1);
DECLARE_TASKLET(txch2tasklet, (void *)mxc_mu_txhandler, (unsigned long)2);
DECLARE_TASKLET(txch3tasklet, (void *)mxc_mu_txhandler, (unsigned long)3);

/*
 * Declaring tasklet to handle general purpose interrupts
 */
DECLARE_TASKLET(gpch0tasklet, (void *)mxc_mu_gphandler, (unsigned long)0);
DECLARE_TASKLET(gpch1tasklet, (void *)mxc_mu_gphandler, (unsigned long)1);
DECLARE_TASKLET(gpch2tasklet, (void *)mxc_mu_gphandler, (unsigned long)2);
DECLARE_TASKLET(gpch3tasklet, (void *)mxc_mu_gphandler, (unsigned long)3);

/*!
 * Allocating Resources:
 * Allocating channels required to the caller if they are free.
 * 'reg_allocated' variable holds all the registers that have been allocated
 *
 * @param   chnum   The channel number required is passed
 *
 * @return          Returns unique channel handler associated with channel. This
 *                  channel handler is used to validate the user accessing the
 *                  channel.
 *                  Returns channel handler - on success
 *                  Returns negative value - on error
 *                     If invalid channel is passed, it returns -ENODEV error.
 *                     If no channel is available, returns -EBUSY.
 */
int mxc_mu_alloc_channel(int chnum)
{
	unsigned long flags;
	int ret_val = -EBUSY;

	if (chnum < 0 || chnum > 3) {
		return -ENODEV;
	}
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	/* Acquiring Lock */
	spin_lock_irqsave(&alloc_lock, flags);
	if (reg_allocated[chnum] == -1) {
		/*
		 * If channel requested is available, create
		 * channel handler and store in the array
		 */
		reg_allocated[chnum] = BASE_NUM + chnum;
		ret_val = BASE_NUM + chnum;
	}
	spin_unlock_irqrestore(&alloc_lock, flags);
	return ret_val;
}

/*!
 * Deallocating Resources:
 *
 * @param   chand   The channel handler associated with the channel
 *                  that is to be freed is passed.
 *
 * @return          Returns 0 on success and -1 if channel was not
 *                  already allocated or ENODEV error if invalid
 *                  channel is obtained from the channel handler
 */
int mxc_mu_dealloc_channel(int chand)
{
	unsigned long flags;
	int retval = -1;
	int chnum = chand - BASE_NUM;

	if (chnum < 0 || chnum > 3) {
		return -ENODEV;
	}
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	spin_lock_irqsave(&alloc_lock, flags);
	if (reg_allocated[chnum] == chand) {
		/* deallocating */
		reg_allocated[chnum] = -1;
		/* clearing callback arrays */
		rxcallback[chnum] = NULL;
		txcallback[chnum] = NULL;
		gpcallback[chnum] = NULL;
		retval = 0;
	}
	spin_unlock_irqrestore(&alloc_lock, flags);
	return retval;
}

/* NON BLOCKING READ/WRITE */

/*!
 * This function is called by other modules to read from one of the
 * receive registers on the MCU side.
 *
 * @param   chand         The channel handler associated with the channel
 *                        to be accessed is passed
 * @param   mu_msg        Buffer where the read data has to be stored
 *                        is passed by pointer as an argument
 *
 * @return                Returns 0 on success or
 *                        Returns NO_DATA error if there is no data to read or
 *                        Returns NO_ACCESS error if the incorrect channel
 *                        is accessed or ENODEV error if invalid
 *                        channel is obtained from the channel handler
 *                        Returns -1 if the buffer passed is not allocated
 */
int mxc_mu_mcuread(int chand, char *mu_msg)
{
	/* mu_msg should be 4 bytes long */
#ifdef CONFIG_MOT_WFN377
        unsigned int data;
#endif

	int chnum = chand - BASE_NUM;

	if (chnum < 0 || chnum > 3) {
		return -ENODEV;
	}
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if (mu_msg == NULL) {
		return -1;
	}
	if ((readl(AS_MUMSR) & rbits[chnum]) == 0) {
		return NO_DATA;
	}
	if (reg_allocated[chnum] == chand) {
#ifdef CONFIG_MOT_WFN377
            data = readl(read_reg[chnum]);

            if(mu_rbuf[chnum].flags&MUFLAG_SWAPREADBYTES)
                data = swab32(data);

            *(unsigned int *)mu_msg = data;
#else
            *(unsigned int *)mu_msg = readl(read_reg[chnum]);
#endif
	} else {
		return NO_ACCESS;
	}
	return 0;
}

/*!
 * This function is called by other modules to write to one of the
 * transmit registers on the MCU side
 *
 * @param   chand         The channel handler associated with the channel
 *                        to be accessed is passed
 * @param   mu_msg        Buffer where the write data has to be stored
 *                        is passed by pointer as an argument
 *
 * @return                Returns 0 on success or
 *                        Returns NO_DATA error if the register not empty or
 *                        Returns NO_ACCESS error if the incorrect channel
 *                        is accessed or ENODEV error if invalid
 *                        channel is obtained from the channel handler
 *                        Returns -1 if the buffer passed is not allocated
 */
int mxc_mu_mcuwrite(int chand, char *mu_msg)
{
#ifdef CONFIG_MOT_WFN377
        unsigned int data;
#endif

	int chnum = chand - BASE_NUM;

	if (chnum < 0 || chnum > 3) {
		return -ENODEV;
	}
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if (mu_msg == NULL) {
		return -1;
	}
	if ((readl(AS_MUMSR) & wbits[chnum]) == 0) {
		return NO_DATA;
	}
	/* Check whether the register is allocated */
	if (reg_allocated[chnum] == chand) {
#ifdef CONFIG_MOT_WFN377
                data = *(unsigned int*)mu_msg;

                if(mu_wbuf[chnum].flags&MUFLAG_SWAPWRITEBYTES)
                    data = swab32(data);

                writel(data, write_reg[chnum]);
#else
                writel(*(unsigned int*) mu_msg, write_reg[chnum]);
#endif
	} else {
		return NO_ACCESS;
	}
	return 0;
}

/*!
 * Tasklet used by the Interrupt service routine to handle the muxed
 * receive interrupts. When an interrupt occurs, control from the top main
 * interrupt service routine is transferred to this tasklet which sends the
 * channel number to the call back function
 *
 * @param   chnum  index value indicating channel number
 *
 */
static void mxc_mu_rxhandler(unsigned long chnum)
{
	if (rxcallback[chnum] != NULL) {
		rxcallback[chnum] (chnum);
	}
}

/*!
 * Tasklet used by the Interrupt service routine to handle the muxed
 * transmit interrupts. When an interrupt occurs, control from the top main
 * interrupt service routine is transferred to this tasklet which sends the
 * channel number to the call back function
 *
 * @param   chnum  index value indicating channel number
 *
 */
static void mxc_mu_txhandler(unsigned long chnum)
{
	if (txcallback[chnum] != NULL) {
		txcallback[chnum] (chnum);
	}
}

/*!
 * Tasklet used by the Interrupt service routine to handle the general
 * purpose interrupts. When an interrupt occurs, control from the top main
 * interrupt service routine is transferred to this tasklet which sends the
 * channel number to the call back function
 *
 * @param   chnum  index value indicating channel number
 *
 */
static void mxc_mu_gphandler(unsigned long chnum)
{
	if (gpcallback[chnum] != NULL) {
		gpcallback[chnum] (chnum);
	}
}

/*!
 * Interrupt service routine registered to handle the individual general purpose
 * interrupts or muxed. Interrupts are cleared in ISR before scheduling tasklet
 *
 * @param   irq     the interrupt number
 * @param   dev_id  driver private data
 * @param   regs    holds a snapshot of the processor's context before the
 *                  processor entered the interrupt code
 *
 * @return          The function returns \b IRQ_RETVAL(1) if interrupt was
 *                  handled, returns \b IRQ_RETVAL(0) if the interrupt was
 *                  not handled.
 *                  \b IRQ_RETVAL is defined in \b include/linux/interrupt.h.
 */
static irqreturn_t mxc_mu_mcugphand(int irq, void *dev_id, struct pt_regs *regs)
{
	int handled = 0;
	unsigned int sreg1, sreg2, status = 0;

	sreg1 = readl(AS_MUMSR);
	sreg2 = readl(AS_MUMCR);
	/* When General Purpose Interrupt occurs */
	if ((sreg1 & AS_MUMSR_MGIP0) && (sreg2 & AS_MUMCR_MGIE0)) {
		status |= (AS_MUMSR_MGIP0);
		tasklet_schedule(&gpch0tasklet);
		handled = 1;
	}
	if ((sreg1 & AS_MUMSR_MGIP1) && (sreg2 & AS_MUMCR_MGIE1)) {
		status |= (AS_MUMSR_MGIP1);
		tasklet_schedule(&gpch1tasklet);
		handled = 1;
	}
	if ((sreg1 & AS_MUMSR_MGIP2) && (sreg2 & AS_MUMCR_MGIE2)) {
		status |= (AS_MUMSR_MGIP2);
		tasklet_schedule(&gpch2tasklet);
		handled = 1;
	}
	if ((sreg1 & AS_MUMSR_MGIP3) && (sreg2 & AS_MUMCR_MGIE3)) {
		status |= (AS_MUMSR_MGIP3);
		tasklet_schedule(&gpch3tasklet);
		handled = 1;
	}
	writel(status, AS_MUMSR);
	return IRQ_RETVAL(handled);
}

/*!
 * Interrupt service routine registered to handle the muxed receive
 * interrupts. Interrupts disabled inside ISR before scheduling tasklet
 *
 * @param   irq     the interrupt number
 * @param   dev_id  driver private data
 * @param   regs    holds a snapshot of the processor's context before the
 *                  processor entered the interrupt code
 *
 * @return          The function returns \b IRQ_RETVAL(1) if interrupt was
 *                  handled, returns \b IRQ_RETVAL(0) if the interrupt was
 *                  not handled.
 *                  \b IRQ_RETVAL is defined in \b include/linux/interrupt.h.
 */
static irqreturn_t mxc_mu_mcurxhand(int irq, void *dev_id, struct pt_regs *regs)
{
	int handled = 0;
	unsigned int sreg1, sreg2, control;

	sreg1 = readl(AS_MUMSR);
	sreg2 = readl(AS_MUMCR);
	control = sreg2;
	/* When Receive Interrupt occurs */
	if ((sreg1 & AS_MUMSR_MRF0) && (sreg2 & AS_MUMCR_MRIE0)) {
		control &= ~(AS_MUMCR_MRIE0);
		tasklet_schedule(&rxch0tasklet);
		handled = 1;
	}
	if ((sreg1 & AS_MUMSR_MRF1) && (sreg2 & AS_MUMCR_MRIE1)) {
		control &= ~(AS_MUMCR_MRIE1);
		tasklet_schedule(&rxch1tasklet);
		handled = 1;
	}
	if ((sreg1 & AS_MUMSR_MRF2) && (sreg2 & AS_MUMCR_MRIE2)) {
		control &= ~(AS_MUMCR_MRIE2);
		tasklet_schedule(&rxch2tasklet);
		handled = 1;
	}
	if ((sreg1 & AS_MUMSR_MRF3) && (sreg2 & AS_MUMCR_MRIE3)) {
		control &= ~(AS_MUMCR_MRIE3);
#ifdef CONFIG_MOT_FEAT_PM
                if (ioi_switch)
                    mpm_handle_ioi( );
#endif
		tasklet_schedule(&rxch3tasklet);
		handled = 1;
	}
	writel(control, AS_MUMCR);
	return IRQ_RETVAL(handled);
}

/*!
 * Interrupt service routine registered to handle the muxed transmit interrupts
 * Interrupts disabled inside ISR before scheduling tasklet
 *
 * @param   irq     the interrupt number
 * @param   dev_id  driver private data
 * @param   regs    holds a snapshot of the processor's context before the
 *                  processor entered the interrupt code
 *
 * @return          The function returns \b IRQ_RETVAL(1) if interrupt was
 *                  handled, returns \b IRQ_RETVAL(0) if the interrupt was
 *                  not handled.
 *                  \b IRQ_RETVAL is defined in \b include/linux/interrupt.h.
 */
static irqreturn_t mxc_mu_mcutxhand(int irq, void *dev_id, struct pt_regs *regs)
{
	int handled = 0;
	unsigned int sreg1, sreg2, control;

	sreg1 = readl(AS_MUMSR);
	sreg2 = readl(AS_MUMCR);
	control = sreg2;
	/* When Transmit Interrupt occurs */
	if ((sreg1 & AS_MUMSR_MTE0) && (sreg2 & AS_MUMCR_MTIE0)) {
		control &= ~(AS_MUMCR_MTIE0);
		tasklet_schedule(&txch0tasklet);
		handled = 1;
	}
	if ((sreg1 & AS_MUMSR_MTE1) && (sreg2 & AS_MUMCR_MTIE1)) {
		control &= ~(AS_MUMCR_MTIE1);
		tasklet_schedule(&txch1tasklet);
		handled = 1;
	}
	if ((sreg1 & AS_MUMSR_MTE2) && (sreg2 & AS_MUMCR_MTIE2)) {
		control &= ~(AS_MUMCR_MTIE2);
		tasklet_schedule(&txch2tasklet);
		handled = 1;
	}
	if ((sreg1 & AS_MUMSR_MTE3) && (sreg2 & AS_MUMCR_MTIE3)) {
		control &= ~(AS_MUMCR_MTIE3);
		tasklet_schedule(&txch3tasklet);
		handled = 1;
	}
	writel(control, AS_MUMCR);
	return IRQ_RETVAL(handled);
}

/*!
 * This function is used by other modules to issue DSP hardware reset.
 *
 */
int mxc_mu_dsp_reset(void)
{
	unsigned long flags;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	spin_lock_irqsave(&mu_lock, flags);
	writel((readl(AS_MUMCR) | AS_MUMCR_DHR), AS_MUMCR);
	spin_unlock_irqrestore(&mu_lock, flags);
	return 0;
}

/*!
 * This function is used by other modules to deassert DSP hardware reset
 */
int mxc_mu_dsp_deassert(void)
{
	unsigned long flags;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	spin_lock_irqsave(&mu_lock, flags);
	writel((readl(AS_MUMCR) & (~AS_MUMCR_DHR)), AS_MUMCR);
	spin_unlock_irqrestore(&mu_lock, flags);
	return 0;
}

/*!
 * This function is used by other modules to issue DSP Non-Maskable
 * Interrupt to the DSP
 */
int mxc_mu_dsp_nmi(void)
{
	unsigned long flags;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	spin_lock_irqsave(&mu_lock, flags);
	writel((readl(AS_MUMCR) | AS_MUMCR_DNMI), AS_MUMCR);
	spin_unlock_irqrestore(&mu_lock, flags);
	return 0;
}

/*!
 * This function is used by other modules to retrieve the DSP reset state.
 *
 * @return   Returns 1 if DSP in reset state and 0 otherwise
 */
int mxc_mu_dsp_reset_status(void)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if ((readl(AS_MUMSR) & AS_MUMSR_DRS) != 0) {
		/* 1 implies the DSP side of MU is in reset state */
		return 1;
	} else {
		/* 0 implies the DSP side of MU is not in reset state */
		return 0;
	}
}

/*!
 * This function is used by other modules to retrieve the DSP Power Mode.
 *
 * @return   Returns a value from which power mode of the DSP side of
 *           of MU unit can be inferred
 *           0 - Run mode,  1 - Wait mode
 *           2 - Stop mode, 3 - DSM mode
 */
unsigned int mxc_mu_dsp_pmode_status(void)
{
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	return (readl(AS_MUMSR) & (AS_MUMSR_DPM1 | AS_MUMSR_DPM0) >> 5);
}

#ifdef CONFIG_MOT_FEAT_PM
/*!
 * This function is used by an IPC module to switch the Interrupt-of-Interest 
 * behavior on and off for the MU channel used for the Power Management 
 * messages.  The switch is on, so interrupts will be reported, by default.
 *
 * @param   swtch     boolean on/off indicator
 *
 * @return   none
 */
void mxc_mu_set_ioi_response(unsigned int swtch)
{

  ioi_switch = swtch;

}
#endif

/*!
 * This function is used by other modules to reset the MU Unit. This would reset
 * both the MCU side and the DSP side
 *
 */
int mxc_mu_reset(void)
{
	unsigned long flags;

	if (suspend_flag == 1) {
		return -EBUSY;
	}
	spin_lock_irqsave(&mu_lock, flags);
	writel((readl(AS_MUMCR) | AS_MUMCR_MMUR), AS_MUMCR);
	spin_unlock_irqrestore(&mu_lock, flags);
	return 0;
}

/*!
 * This function is called by unbind function of this module and
 * can also be called from other modules to enable desired
 * receive Interrupt
 *
 * @param   chand    The channel handler associated with the channel
 *                   whose interrupt to be disabled is passed
 * @param   muoper   The value passed is TX, RX, GP
 *
 * @return           Returns 0 on success or ENODEV error if invalid
 *                   channel is obtained from the channel handler
 *                   Returns -1 if muoper is other than RX, TX or GP
 */
int mxc_mu_intdisable(int chand, enum mu_oper muoper)
{
	unsigned int status;
	unsigned long flags;
	int chnum = chand - BASE_NUM;

	if (chnum < 0 || chnum > 3) {
		return -ENODEV;
	}
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	spin_lock_irqsave(&mu_lock, flags);
	status = readl(AS_MUMCR);
	switch (muoper) {
	case RX:
		status &= ~(read_en[chnum]);
		break;
	case TX:
		status &= ~(write_en[chnum]);
		break;
	case GP:
		status &= ~(genp_en[chnum]);
		break;
	default:
		spin_unlock_irqrestore(&mu_lock, flags);
		return -1;
	}
	writel(status, AS_MUMCR);
	spin_unlock_irqrestore(&mu_lock, flags);
	return 0;
}

/*!
 * This function is called by other modules to unbind their
 * call back functions
 *
 * @param   chand    The channel handler associated with the channel
 *                   to be accessed is passed
 * @param   muoper   The value passed is TX, RX, GP
 *
 * @return           Returns 0 on success or ENODEV error if invalid
 *                   channel is obtained from the channel handler
 *                   Returns -1 if muoper is other than RX, TX or GP, or
 *                   if disabling interrupt failed
 */
int mxc_mu_unbind(int chand, enum mu_oper muoper)
{
	int result = 0;
	int chnum = chand - BASE_NUM;

	if (chnum < 0 || chnum > 3) {
		return -ENODEV;
	}
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if (reg_allocated[chnum] == chand) {
		if (mxc_mu_intdisable(chand, muoper) != 0) {
			return -1;
		}
		switch (muoper) {
		case RX:
			rxcallback[chnum] = NULL;
			break;
		case TX:
			txcallback[chnum] = NULL;
			break;
		case GP:
			gpcallback[chnum] = NULL;
			break;
		default:
			result = -1;
		}
	}
	return result;
}

/*!
 * This function is called by the user and other modules to enable desired
 * interrupt
 *
 * @param   chand     The channel handler associated with the channel
 *                    to be accessed is passed
 * @param    muoper   The value passed is TX, RX, GP
 *
 * @return           Returns 0 on success or ENODEV error if invalid
 *                   channel is obtained from the channel handler
 *                   Returns -1 if muoper is other than RX, TX or GP
 */
int mxc_mu_intenable(int chand, enum mu_oper muoper)
{
	unsigned long status = 0;
	unsigned long flags;
	int chnum = chand - BASE_NUM;

	if (chnum < 0 || chnum > 3) {
		return -ENODEV;
	}
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	spin_lock_irqsave(&mu_lock, flags);
	status = readl(AS_MUMCR);
	switch (muoper) {
	case RX:
		status |= (read_en[chnum]);
		break;
	case TX:
		status |= (write_en[chnum]);
		break;
	case GP:
		status |= (genp_en[chnum]);
		break;
	default:
		spin_unlock_irqrestore(&mu_lock, flags);
		return -1;
	}
	writel(status, AS_MUMCR);
	spin_unlock_irqrestore(&mu_lock, flags);
	return 0;
}

/*!
 * This function is called by other modules to bind their
 * call back functions.
 *
 * @param   chand         The channel handler associated with the channel
 *                        to be accessed is passed
 * @param   callback      the caller's callback function
 * @param   muoper        The value passed is TX, RX, GP
 *
 * @return                Returns 0 on success or ENODEV error if invalid
 *                        channel is obtained from the channel handler
 *                        Returns -1 if muoper is other than RX, TX or GP
 */
int mxc_mu_bind(int chand, callbackfn callback, enum mu_oper muoper)
{

	int chnum = chand - BASE_NUM;

	if (chnum < 0 || chnum > 3) {
		return -ENODEV;
	}
	if (suspend_flag == 1) {
		return -EBUSY;
	}
	if (reg_allocated[chnum] == chand) {
		switch (muoper) {
		case RX:
			rxcallback[chnum] = callback;
			break;
		case TX:
			txcallback[chnum] = callback;
			break;
		case GP:
			gpcallback[chnum] = callback;
			break;
		default:
			return -1;
		}
	}
	return 0;
}

/*
 * This function is used to copy data from kernel space buffer
 * to user space buffer
 */
static int mcu_copytouser(int count, int minor, char *userbuff)
{
	int bytes_available = 0, actual_bytes = 0, checkval = 0;

	while (1) {
		bytes_available = CIRC_CNT_TO_END(mu_rbuf[minor].cbuf.head,
						  mu_rbuf[minor].cbuf.tail,
						  MAX_BUF_SIZE);
		/*
		 * If the number of bytes already in the buffer is greater than
		 * count then read count from buffer else read what is in the buffer
		 */
		if (count < bytes_available) {
			bytes_available = count;
		}
		if (bytes_available <= 0) {
			break;
		}
		checkval = copy_to_user(userbuff,
					&mu_rbuf[minor].cbuf.buf[mu_rbuf[minor].
								 cbuf.tail],
					bytes_available);
		if (checkval) {
			break;
		}
		mu_rbuf[minor].cbuf.tail = (mu_rbuf[minor].cbuf.tail +
					    bytes_available) & (MAX_BUF_SIZE -
								1);

		userbuff += bytes_available;
		count -= bytes_available;
		actual_bytes += bytes_available;
	}
	/*
	 * Return the number of bytes copied
	 */
	return actual_bytes;
}

/*
 * This function is used to copy data from user space buffer
 * to kernel space buffer
 */
static int mcu_copyfromuser(int count, int minor, const char *userbuff)
{
	int space_available = 0, bytes_copied = 0, checkval = 0;

	while (1) {
		space_available = CIRC_SPACE_TO_END(mu_wbuf[minor].cbuf.head,
						    mu_wbuf[minor].cbuf.tail,
						    MAX_BUF_SIZE);

#ifdef CONFIG_MOT_WFN403
		space_available &= ~(CH_SIZE - 1);
#endif

		/*
		 * If the space available in the kernel space buffer
		 * is more than required i.e., count then number of bytes
		 * copied from the user space buffer is count else available empty
		 * space for transmission
		 */
		if (space_available >= count) {
			space_available = count;
		}
		if (space_available < CH_SIZE) {
			break;
		}
		checkval =
		    copy_from_user(&mu_wbuf[minor].cbuf.
				   buf[mu_wbuf[minor].cbuf.head], userbuff,
				   space_available);
		if (checkval) {
			break;
		}
		mu_wbuf[minor].cbuf.head =
		    (mu_wbuf[minor].cbuf.head +
		     space_available) & (MAX_BUF_SIZE - 1);

		userbuff += space_available;
		count -= space_available;
		bytes_copied += space_available;
	}

	return bytes_copied;
}

/*!
 * The read function is available to the user-space to perform
 * a read operation
 *
 *  @param  filp        Pointer to device file structure
 *  @param  userbuff    User buffer, where read data to be placed.
 *  @param  count       Size of the requested data transfer
 *  @param  offp        File position where the user is accessing.
 *
 *  @return             Returns number of bytes read or
 *                      Returns -EINVAL if the number of bytes
 *                      requested is not a multiple of channel
 *                      receive or transmit size or
 *                      Returns -EAGAIN for a non-block read when no data
 *                      is ready in the buffer or in the register or
 *                      Returns -EFAULT if copy_to_user failed
 */
static ssize_t mxc_mu_read(struct file *filp,
			   char *userbuff, size_t count, loff_t * offp)
{
	int total_bytestocopy = 0;
	struct inode *inode = filp->f_dentry->d_inode;
	unsigned int minor = MINOR(inode->i_rdev);

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	/*
	 * If the count value requested is not multiple of channel size then
	 * return error
	 */
	if (((count % CH_SIZE) != 0) || (count == 0)) {
		return -EINVAL;	/* error */
	}
	total_bytestocopy = mcu_copytouser(count, minor, userbuff);
	/*
	 * Enable the interrupt to read from the register
	 */
	mxc_mu_intenable(reg_allocated[minor], RX);
	if (total_bytestocopy != 0) {
		return total_bytestocopy;
	}
	/*
	 * If there is no data in buffer and number of bytes
	 * copied to user space buffer is 0, then block until at least
	 * 4 bytes are read incase of block read else, return immediately
	 * incase of nonblock read
	 */
	if (!(filp->f_flags & O_NONBLOCK)) {
		while (GET_READBYTES(minor) == 0) {
#ifndef CONFIG_MOT_WFN374
			mu_rbuf[minor].pwait++;
#endif
			/* Block */
			if (wait_event_interruptible(readq,
						     (GET_READBYTES(minor) !=
						      0))) {
				return -ERESTARTSYS;
			}
		}
	} else {
		/* Return error for Non-Block read */
		return -EAGAIN;
	}
	/*
	 * Some data is now available in the register,
	 * read the data and copy to the user
	 */
	return (mcu_copytouser(count, minor, userbuff));
}

/*!
 * The write function is available to the user-space to perform
 * a write operation
 *
 *  @param  filp        Pointer to device file structure
 *  @param  userbuff    User buffer, where read data to be placed.
 *  @param  count       Size of the requested data transfer
 *  @param  offp        File position where the user is accessing.
 *
 *  @return             Returns number of bytes read or
 *                      Returns -EINVAL if the number of bytes
 *                      requested is not a multiple of channel
 *                      receive or transmit size or
 *                      Returns -EAGAIN for a non-block read when no data
 *                      is ready in the buffer or in the register or
 *                      Returns -EFAULT if copy_to_user failed
 */
static ssize_t mxc_mu_write(struct file *filp, const char *userbuff, size_t
			    count, loff_t * offp)
{
	int totalbytes_copied = 0;
	struct inode *inode = filp->f_dentry->d_inode;
	unsigned int minor = MINOR(inode->i_rdev);

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	/*
	 * If the count value requested is not multiple of channel size then
	 * return error
	 */
	if (((count % CH_SIZE) != 0) || (count == 0)) {
		return -EINVAL;	/* error */
	}
	totalbytes_copied = mcu_copyfromuser(count, minor, userbuff);
	/* Enable the interrupt before return so that data is transmitted */
	mxc_mu_intenable(reg_allocated[minor], TX);
	/*
	 * If number of bytes copied to the kernel space buffer is greater than
	 * 0, return immediately
	 */
	if (totalbytes_copied != 0) {
		return totalbytes_copied;
	}
	/* If there is no space in buffer and number of bytes
	 * copied from user space buffer is 0, then block until at least
	 * 4 bytes are written incase of block write else, return
	 * immediately incase of nonblock write
	 */
	if (!(filp->f_flags & O_NONBLOCK)) {
		while (GET_WRITESPACE(minor) < CH_SIZE) {
#ifndef CONFIG_MOT_WFN374
			mu_wbuf[minor].pwait++;
#endif
			if (wait_event_interruptible(writeq,
						     (GET_WRITESPACE(minor) >=
						      CH_SIZE))) {
				return -ERESTARTSYS;
			}
		}
	} else {
		/* Return error incase of non-block write */
		return -EAGAIN;
	}
	/*
	 * Some space is now available since some
	 * data has been transmitted. So, copy data from
	 * the user space buffer to kernel space buffer,
	 */
	return (mcu_copyfromuser(count, minor, userbuff));
}

/*!
 * The write callback function is used by this module to perform any
 * write requests from user-space
 *
 *  @param  chnum        channel number whose register has to be accessed
 */
static void mxc_mu_writecb(int chnum)
{
	char *message;

	/*
	 * While there are more bytes to transfer and
	 * register is empty, continue transmitting
	 */
	while ((GET_WRITEBYTES(chnum) != 0) && (readl(AS_MUMSR) & wbits[chnum])) {
		/* Maximum of 4 bytes everytime */
		message = &mu_wbuf[chnum].cbuf.buf[mu_wbuf[chnum].cbuf.tail];
		mu_wbuf[chnum].cbuf.tail =
		    (mu_wbuf[chnum].cbuf.tail + CH_SIZE) & (MAX_BUF_SIZE - 1);
		mxc_mu_mcuwrite(reg_allocated[chnum], message);
		/* Emptying write buffer */
	}
	/*
	 * Enable the interrupt if more data has to be transmitted.
	 * Since interrupts are disabled inside ISR.
	 */
	if (GET_WRITEBYTES(chnum) != 0) {
		mxc_mu_intenable(reg_allocated[chnum], TX);
	}
	/* Wake up the sleeping process if the buffer gets emptied */
#ifndef CONFIG_MOT_WFN374
        if (
#ifdef CONFIG_MOT_WFN404
            GET_WRITEBYTES(chnum) == 0 ||
#endif
            blockmode[chnum] == 0 &&
            mu_wbuf[chnum].pwait > 0 &&
            GET_WRITESPACE(chnum) >= CH_SIZE
           ) {
#ifdef CONFIG_MOT_WFN404
                if(mu_wbuf[chnum].pwait > 0)
#endif
                    mu_wbuf[chnum].pwait--;

#else
        if (
#ifdef CONFIG_MOT_WFN404
            GET_WRITEBYTES(chnum) == 0 ||
#endif
            GET_WRITESPACE(chnum) >= CH_SIZE
           ) {
#endif
		/* Wake up */
		wake_up_interruptible(&writeq);
	}
}

/*!
 * The read callback function is used by this module to perform any
 * read requests from user-space
 *
 *  @param  chnum        channel number whose register has to be accessed
 */
static void mxc_mu_readcb(int chnum)
{
	char message[CH_SIZE];
	int index;

	/*
	 * While there more bytes can be read and if
	 * buffer is empty
	 */
	while ((GET_READSPACE(chnum) >= CH_SIZE)
	       && (readl(AS_MUMSR) & rbits[chnum])) {
		mxc_mu_mcuread(reg_allocated[chnum], message);
		index = mu_rbuf[chnum].cbuf.head;
		*(int *)(&(mu_rbuf[chnum].cbuf.buf[index])) = *(int *)message;
		mu_rbuf[chnum].cbuf.head =
		    (mu_rbuf[chnum].cbuf.head + CH_SIZE) & (MAX_BUF_SIZE - 1);
	}
	/*
	 * If no empty space in buffer to store the data that
	 * has been read, then disable the interrupt else
	 * enable the interrupt
	 */
	(GET_READSPACE(chnum) <
	 CH_SIZE) ? mxc_mu_intdisable(reg_allocated[chnum],
				      RX) :
mxc_mu_intenable(reg_allocated[chnum], RX);
	/* Wake up the sleeping process if data is available */
#ifndef CONFIG_MOT_WFN374
	if (blockmode[chnum] == 0 && mu_rbuf[chnum].pwait > 0 &&
	    GET_READBYTES(chnum) != 0) {
		mu_rbuf[chnum].pwait--;
#else
        if (GET_READBYTES(chnum) != 0) {
#endif
		/* Wake up */
		wake_up_interruptible(&readq);
	}
}

/*!
 * The general purpose callback function is used by this module to
 * perform any general purpose interrupt requests from user-space
 *
 *  @param  chnum        channel number whose register has to be accessed
 */
static void mxc_mu_genpurposecb(int chnum)
{
	writel((readl(AS_MUMSR) | (genp_pend[chnum])), AS_MUMSR);
}

/*!
 * This function is called when an ioctl call is made from user space.
 *
 * @param    inode    Pointer to device inode
 * @param    file     Pointer to device file structure
 * @param    cmd      Ioctl command
 * @param    arg      Ioctl argument
 *
 * @return    The function returns 0 on success and -EINVAL on
 *            failure when cmd is other than what is specified
 */
int mxc_mu_ioctl(struct inode *inode, struct file *file,
		 unsigned int cmd, unsigned long arg)
{
	struct inode *i_node = file->f_dentry->d_inode;
	unsigned int minor = MINOR(i_node->i_rdev);
	int i = 0;
#ifdef CONFIG_MOT_WFN349
        unsigned int status;
        unsigned long flags;
#endif
#ifdef CONFIG_MOT_FEAT_SDMA_DEBUG
	struct sdma_debug_registers sdma_regs;
#endif

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	switch (cmd) {
	case SENDINT:
#ifdef CONFIG_MOT_WFN349
                spin_lock_irqsave(&mu_lock, flags);
                status = readl(AS_MUMCR);
                status |= genp_req[minor];
                writel(status, AS_MUMCR);
                spin_unlock_irqrestore(&mu_lock, flags);
#else
		writel(genp_req[minor], AS_MUMCR);
#endif
		break;
	case SENDNMI:
		mxc_mu_dsp_nmi();
		break;
	case SENDDSPRESET:
		mxc_mu_dsp_reset();
		for (i = 0; i < 20; i++) ;
		mxc_mu_dsp_deassert();
		while (mxc_mu_dsp_reset_status() != 0) ;
		break;
	case SENDMURESET:
		mxc_mu_reset();
		break;
	case RXENABLE:
		mxc_mu_intenable(reg_allocated[minor], RX);
		break;
	case TXENABLE:
		mxc_mu_intenable(reg_allocated[minor], TX);
		break;
	case GPENABLE:
		mxc_mu_intenable(reg_allocated[minor], GP);
		break;

#ifdef CONFIG_MOT_WFN377
        case SWAPMUREADBYTES:
                mu_rbuf[minor].flags ^= MUFLAG_SWAPREADBYTES;
                break;
        case SWAPMUWRITEBYTES:
                mu_wbuf[minor].flags ^= MUFLAG_SWAPWRITEBYTES;
                break;
#endif

#ifdef CONFIG_MOT_WFN374

#ifdef CONFIG_MOT_FEAT_MU_TEST
        case MUTEST_INJECTDATA:
                if((GET_READSPACE(minor) >= CH_SIZE))
                {
                        i = mu_rbuf[minor].cbuf.head;

                        *(int*)(&(mu_rbuf[minor].cbuf.buf[i])) = arg;

                        mu_rbuf[minor].cbuf.head  = (mu_rbuf[minor].cbuf.head+CH_SIZE);
                        mu_rbuf[minor].cbuf.head &= (MAX_BUF_SIZE-1);
                }
                break;
#endif /* #ifdef CONFIG_MOT_FEAT_MU_TEST */

#endif /* #ifdef CONFIG_MOT_WFN374 */
#ifdef CONFIG_MOT_FEAT_SDMA_DEBUG
        case MXC_MU_SDMA_DUMP_DEBUG:
                mxc_sdma_debug_register_dump(&sdma_regs, SDMA_FULL_DUMP);
                copy_to_user((struct sdma_debug_registers *)arg, &sdma_regs, sizeof(struct sdma_debug_registers));    
                return 0;
 case MXC_MU_SDMA_DUMP_REGISTER:
                mxc_sdma_debug_register_dump(&sdma_regs, SDMA_REG_DUMP);
                copy_to_user((struct sdma_debug_registers *)arg, &sdma_regs, sizeof(struct sdma_debug_registers));
                return 0;
#endif
	default:
		return -EINVAL;
	}
	return 0;
}

/*!
 * This function is called to close the device. The MCU side of MU interrupts
 * are disabled. Any allocated buffers are freed
 *
 * @param    inode    Pointer to device inode
 * @param    filp     Pointer to device file structure
 *
 * @return    The function returns 0 on success or
 *            returns -ENODEV if incorrect channel is accessed
 *
 */
static int mxc_mu_close(struct inode *inode, struct file *filp)
{
	unsigned int minor = MINOR(inode->i_rdev);

#ifdef CONFIG_MOT_WFN404
        int          result;

        result = wait_event_interruptible(writeq, (GET_WRITEBYTES(minor) == 0));
        if(result)
            return -EINTR;
#endif

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	if (mxc_mu_dealloc_channel(reg_allocated[minor]) != 0) {
		return -ENODEV;
	}
	kfree(mu_rbuf[minor].cbuf.buf);
	return 0;
}

/*!
 * This function is called when the MU driver is opened. This function does
 * the initialization of the device.
 *
 * @param    inode    Pointer to device inode
 * @param    filp     Pointer to device file structure
 *
 * @return    The function returns 0 on success or a negative value if
 *            channel was not allocated or if kmalloc failed
 */
static int mxc_mu_open(struct inode *inode, struct file *filp)
{
	int chand;
	unsigned int minor = MINOR(inode->i_rdev);

	while (suspend_flag == 1) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, (suspend_flag == 0))) {
			return -ERESTARTSYS;
		}
	}
	chand = mxc_mu_alloc_channel(minor);
	if (chand < 0) {
		return chand;
	}
	mxc_mu_bind(reg_allocated[minor], &mxc_mu_readcb, RX);
	mxc_mu_bind(reg_allocated[minor], &mxc_mu_writecb, TX);
	mxc_mu_bind(reg_allocated[minor], &mxc_mu_genpurposecb, GP);
	mxc_mu_intdisable(reg_allocated[minor], TX);
	/*
	 * Read buffer from 0 to MAX_BUF_SIZE
	 * Write buffer from MAX_BUF_SIZE to 2*MAX_BUF_SIZE
	 */
	mu_rbuf[minor].cbuf.buf = kmalloc((2 * MAX_BUF_SIZE), GFP_KERNEL);
	mu_wbuf[minor].cbuf.buf = mu_rbuf[minor].cbuf.buf + MAX_BUF_SIZE;
	if (mu_rbuf[minor].cbuf.buf == NULL) {
		reg_allocated[minor] = -1;
		return -1;
	}
	mu_rbuf[minor].cbuf.tail = mu_rbuf[minor].cbuf.head = 0;
	mu_wbuf[minor].cbuf.tail = mu_wbuf[minor].cbuf.head = 0;

#ifdef CONFIG_MOT_WFN377
        mu_rbuf[minor].flags = 0;
        mu_wbuf[minor].flags = 0;
#endif

#ifndef CONFIG_MOT_WFN374
	mu_rbuf[minor].pwait = mu_rbuf[minor].pwait = 0;
	if (filp->f_flags & O_NONBLOCK) {
		blockmode[minor] = 1;
	} else {
		blockmode[minor] = 0;
	}
#endif

	mxc_mu_intenable(reg_allocated[minor], RX);
	return 0;
}

/*!
 * This function is called to put the MU in a low power state.
 *
 * @param   dev   the device structure used to give information on which MU
 *                device (0 through 3 channels) to suspend
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_mu_suspend(struct device *dev, u32 state, u32 level)
{
	int mu_id, status;
	struct platform_device *platdev = to_platform_device(dev);
#ifdef CONFIG_MOT_WFN349
        unsigned long flags;
#endif

	mu_id = platdev->id;
	switch (level) {
	case SUSPEND_DISABLE:
		suspend_flag = 1;
#ifdef CONFIG_MOT_WFN379

#ifdef CONFIG_MOT_WFN349
                spin_lock_irqsave(&mu_lock, flags);
#endif
                status = readl(AS_MUMCR);
                save_status = status;
                /* Leave MU RX 3 interrupt enabled so that the interrupt can
                   be used as a wakeup source out of low power modes. */
                status &= ~((AS_MUMCR_MRIE0|AS_MUMCR_MRIE1|AS_MUMCR_MRIE2) |
                            (AS_MUMCR_MTIE0|AS_MUMCR_MTIE1|AS_MUMCR_MTIE2|AS_MUMCR_MTIE3) | 
                            (AS_MUMCR_MGIE0|AS_MUMCR_MGIE1|AS_MUMCR_MGIE2|AS_MUMCR_MGIE3));
                writel(status, AS_MUMCR);
#ifdef CONFIG_MOT_WFN349
                spin_unlock_irqrestore(&mu_lock, flags);
#endif



#else



                while (mu_id < 4) {
#ifdef CONFIG_MOT_WFN349
                        spin_lock_irqsave(&mu_lock, flags);
#endif
			status = readl(AS_MUMCR);
			status &= ~(read_en[mu_id]);
			status &= ~(write_en[mu_id]);
			status &= ~(genp_en[mu_id]);
			writel(status, AS_MUMCR);
#ifdef CONFIG_MOT_WFN349
                        spin_unlock_irqrestore(&mu_lock, flags);
#endif
			mu_id++;
		}
#endif

		break;
	case SUSPEND_SAVE_STATE:
		/* TBD */
		break;
	case SUSPEND_POWER_DOWN:
                /* leave the clock on so AP can be awakened by BP */
#ifndef CONFIG_MOT_FEAT_PM
		/* Turn off clock */
		mxc_clks_disable(MU_CLK);
#endif
		break;
	}
	return 0;
}

/*!
 * This function is called to resume the MU from a low power state.
 *
 * @param   dev   the device structure used to give information on which MU
 *                device (0 through 3 channels) to suspend
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_mu_resume(struct device *dev, u32 level)
{
        int mu_id;
#ifndef CONFIG_MOT_WFN379
        int status;
#endif
        struct platform_device *platdev = to_platform_device(dev);
#ifdef CONFIG_MOT_WFN349
        unsigned long flags;
#endif

	mu_id = platdev->id;
	switch (level) {
	case RESUME_POWER_ON:
                /* clock was left on so AP could be awakened by BP */
#ifndef CONFIG_MOT_FEAT_PM
                /* Turn on clock */
                mxc_clks_enable(MU_CLK);
#endif
		break;
	case RESUME_RESTORE_STATE:
		/* TBD */
		break;
	case RESUME_ENABLE:
#ifndef CONFIG_MOT_WFN349
                suspend_flag = 0;
#endif

#ifdef CONFIG_MOT_WFN379

#ifdef CONFIG_MOT_WFN349
                spin_lock_irqsave(&mu_lock, flags);
#endif
                writel(save_status, AS_MUMCR);
#ifdef CONFIG_MOT_WFN349
                suspend_flag = 0;
                spin_unlock_irqrestore(&mu_lock, flags);
#endif

                while (swait > 0) {
                        swait--;
                        wake_up_interruptible(&suspendq);
                }



#else



                while (mu_id < 4) {
#ifdef CONFIG_MOT_WFN349
                        spin_lock_irqsave(&mu_lock, flags);
#endif
			status = readl(AS_MUMCR);
			status |= (read_en[mu_id]);
			status |= (write_en[mu_id]);
			status |= (genp_en[mu_id]);
			writel(status, AS_MUMCR);
#ifdef CONFIG_MOT_WFN349
                        spin_unlock_irqrestore(&mu_lock, flags);
#endif
			mu_id++;
		}
#ifdef CONFIG_MOT_WFN349
                suspend_flag = 0;
#endif
		while (swait > 0) {
			swait--;
			wake_up_interruptible(&suspendq);
		}
#endif
		break;
	}
	return 0;
}

#ifdef CONFIG_MOT_WFN374

/*!
 * This function is called to check if any data is available. This is called
 * by the kernel poll functionality.
 *
 * @param    file      Pointer to device file structure
 * @param    wait      Poll table structure is used within the kernel to
 *                     implement the poll and select system calls; it is
 *                     declared in linux/poll.h.
 *
 * @result      Bit mask events POLLIN|POLLRDNORM if data is available in
 *              MU internal circular buffer. If write space is available
                POLLOUT|POLLWRNORM is returned.
 */
static unsigned int mxc_mu_poll(struct file *file, poll_table *wait)
{
    unsigned int mask;
    unsigned int minor;

    minor = MINOR(file->f_dentry->d_inode->i_rdev);
    mask  = 0;

    poll_wait(file, &writeq, wait);
    poll_wait(file, &readq, wait);

    if(GET_READBYTES(minor))
        mask |= POLLIN|POLLRDNORM;
    if(GET_WRITESPACE(minor))
        mask |= POLLOUT|POLLWRNORM;

    return mask;
}

#endif

static struct file_operations mu_fops = {
	.owner = THIS_MODULE,
	.read = mxc_mu_read,
	.write = mxc_mu_write,
	.ioctl = mxc_mu_ioctl,
	.open = mxc_mu_open,
	.release = mxc_mu_close,

#ifdef CONFIG_MOT_WFN374
        .poll      = mxc_mu_poll,
#endif
};

/*!
 * This function is called while loading the module to initiate message
 * transfers. This function initializes all the registers.
 */
static void mxc_mu_load_mod(void)
{
	/* Initializing wait queues */
	init_waitqueue_head(&readq);
	init_waitqueue_head(&writeq);
	init_waitqueue_head(&suspendq);
	/* Setting the status and control registers to default values */
	writel(0xffff, AS_MUMSR);
	writel(0, AS_MUMCR);
}

/*!
 * This function is called while unloading driver.
 * This resets all the registers and disables
 * interrupts.
 */
static void mxc_mu_unload_mod(void)
{
	/* setting the status and control registers to default values */
	writel(0xffff, AS_MUMSR);
	writel(0, AS_MUMCR);
}

/*!
 * This function is used to load the module and all the interrupt lines
 * are requested.
 *
 * @return   Returns an Integer on success
 */
static int __init mxc_mu_probe(struct device *dev)
{
	/* Initializing resources allocated */
	int rx_irq, tx_irq, irq;
	int max_res;
	int i;
	struct class_device *temp_class;

	struct platform_device *pdev = to_platform_device(dev);

	max_res = pdev->num_resources;
	rx_irq = platform_get_irq(pdev, 0);
	tx_irq = platform_get_irq(pdev, 1);
	if (rx_irq == NO_IRQ || tx_irq == NO_IRQ)
		return -ENXIO;

	if ((mu_major = register_chrdev(0, "mxc_mu", &mu_fops)) < 0) {
		printk(KERN_NOTICE
		       "Can't allocate major number for MU Devices.\n");
		return -EAGAIN;
	}

	mxc_mu_class = class_simple_create(THIS_MODULE, "mxc_mu");

	if (IS_ERR(mxc_mu_class)) {
		printk(KERN_ERR "Error creating mu class.\n");
		unregister_chrdev(mu_major, "mxc_mu");
		return PTR_ERR(mxc_mu_class);
	}

	devfs_mk_dir("mxc_mu");
	mxc_mu_load_mod();

	for (i = 0; i <= 3; i++) {
		temp_class =
		    class_simple_device_add(mxc_mu_class, MKDEV(mu_major, i),
					    NULL, "mxc_mu%u", i);

		if (IS_ERR(temp_class))
			goto err_out1;

		if (devfs_mk_cdev
		    (MKDEV(mu_major, i), S_IFCHR | S_IRUSR | S_IWUSR,
		     "mxc_mu/%u", i))
			goto err_out1;

	}

	if (request_irq(rx_irq, mxc_mu_mcurxhand, 0, "mxc_mu_rx", NULL) != 0)
		goto err_out1;

	if (request_irq(tx_irq, mxc_mu_mcutxhand, 0, "mxc_mu_tx", NULL) != 0)
		goto err_out2;

	for (i = 2; i < max_res; i++) {
		irq = platform_get_irq(pdev, i);
		if (irq == NO_IRQ)
			goto err_out3;

		if (request_irq(irq, mxc_mu_mcugphand, 0, "mxc_mu_mcug", NULL)
		    != 0) {
			printk(KERN_ERR "mxc_mu: request_irq for %d failed\n",
			       irq);
			goto err_out3;
		}
	}

	return 0;

      err_out3:
	for (--i; i >= 2; i--) {
		irq = platform_get_irq(pdev, i);
		free_irq(irq, NULL);
	}
	free_irq(tx_irq, NULL);

      err_out2:
	free_irq(rx_irq, NULL);

      err_out1:
	for (i = 0; i <= 3; i++) {
		class_simple_device_remove(MKDEV(mu_major, i));
		devfs_remove("mxc_mu%u", i);
	}
	mxc_mu_unload_mod();
	devfs_remove("mxc_mu");
	class_simple_destroy(mxc_mu_class);
	unregister_chrdev(mu_major, "mxc_mu");

	return -1;
}

/*!
 * This function is used to unload the module and all interrupt lines are freed
 */
static void __exit mxc_mu_remove(struct device *dev)
{
	int irq, max_res, i;
	struct platform_device *pdev = to_platform_device(dev);
	max_res = pdev->num_resources;

	/* Freeing the resources allocated */
	mxc_mu_unload_mod();
	for (i = 0; i < max_res; i++) {
		irq = platform_get_irq(pdev, i);
		free_irq(irq, NULL);
	}
	for (i = 0; i <= 3; i++) {
		class_simple_device_remove(MKDEV(mu_major, i));
		devfs_remove("mxc_mu%u", i);
	}
	devfs_remove("mxc_mu");
	class_simple_destroy(mxc_mu_class);
	unregister_chrdev(mu_major, "mxc_mu");

}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxc_mu_driver = {
	.name = "mxc_mu",
	.bus = &platform_bus_type,
	.probe = mxc_mu_probe,
	.remove = __exit_p(mxc_mu_remove),
	.suspend = mxc_mu_suspend,
	.resume = mxc_mu_resume,
};

/*
 * Main initialization routine
 */
static int __init mxc_mu_init(void)
{
	/* Register the device driver structure. */
	pr_info("MXC MU Driver %s\n", DVR_VER);
	if (driver_register(&mxc_mu_driver) != 0) {
		printk(KERN_ERR "Driver register failed for mxc_mu_driver\n");
		return -ENODEV;
	}
	return 0;
}

/*
 * Clean up routine
 */
static void __exit mxc_mu_cleanup(void)
{
	/* Unregister the device structure */
	driver_unregister(&mxc_mu_driver);
	printk("MU Driver Module Unloaded\n");
}

module_init(mxc_mu_init);
module_exit(mxc_mu_cleanup);

EXPORT_SYMBOL(mxc_mu_bind);
EXPORT_SYMBOL(mxc_mu_unbind);
EXPORT_SYMBOL(mxc_mu_alloc_channel);
EXPORT_SYMBOL(mxc_mu_dealloc_channel);
EXPORT_SYMBOL(mxc_mu_mcuread);
EXPORT_SYMBOL(mxc_mu_mcuwrite);
EXPORT_SYMBOL(mxc_mu_intenable);
EXPORT_SYMBOL(mxc_mu_intdisable);
EXPORT_SYMBOL(mxc_mu_reset);
EXPORT_SYMBOL(mxc_mu_dsp_reset);
EXPORT_SYMBOL(mxc_mu_dsp_deassert);
EXPORT_SYMBOL(mxc_mu_dsp_nmi);
EXPORT_SYMBOL(mxc_mu_dsp_reset_status);
EXPORT_SYMBOL(mxc_mu_dsp_pmode_status);
#ifdef CONFIG_MOT_FEAT_PM
EXPORT_SYMBOL(mxc_mu_set_ioi_response);
#endif

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Messaging Unit Driver");
MODULE_LICENSE("GPL");
