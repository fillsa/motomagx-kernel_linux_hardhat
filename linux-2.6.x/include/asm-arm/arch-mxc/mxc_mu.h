/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2007 Motorola, Inc.
 */

 /*
  * The code contained herein is licensed under the GNU Lesser General
  * Public License.  You may obtain a copy of the GNU Lesser General
  * Public License Version 2.1 or later at the following locations:
  *
  * http://www.opensource.org/licenses/lgpl-license.html
  * http://www.gnu.org/copyleft/lgpl.html
  */

/*
 * DATE          AUTHOR         COMMMENT
 * ----          ------         --------
 * 10/04/2006    Motorola       Fixed bug documented by WFN414.
 *                              Added ioctls for dynamic byte swapping.
 *                              Added #define for MU testing.
 *                              Added function prototype for AP BP sleep
 *                              negotiation.
 * 04/26/2007    Motorola       Adding sdma debug ioctl
 * 12/14/2007    Motorola       Add a new ioctl to dump 24 SDMA register
 */

#ifndef MU_AS_H
#define MU_AS_H

#if defined(__KERNEL__) && defined(CONFIG_MOT_WFN414)
#include <asm/hardware.h>


/*!
 * @defgroup MU Messaging Unit (MU) Driver
 */

/*!
 * @file mxc_mu.h
 *
 * @brief This file contains the MU chip level configuration details and
 * public API declarations.
 *
 * @ingroup MU
 */

/* MU configuration */

/*
 * This define specifies the transmitter interrupt numbers. If the transmitter
 * interrupts of MU are muxed, then we specify here a dummy value -1.
 */
#define MU_TX_INT0              INT_MU_TX_OR

/*
 * This define specifies the receiver interrupt numbers. If the receiver
 * interrupts of MU are muxed, then we specify here a dummy value -1.
 */
#define MU_RX_INT0              INT_MU_RX_OR

/*
 * This define specifies the general purpose interrupt numbers. If the general
 * purpose interrupts of MU are muxed, then we specify here a dummy value -1.
 */
#if MU_MUX_GN_INTS
#define MU_GN_INT0              INT_MU_GEN
#define MU_GN_INT1              -1
#define MU_GN_INT2              -1
#define MU_GN_INT3              -1
#else
#define MU_GN_INT0              INT_MU0
#define MU_GN_INT1              INT_MU1
#define MU_GN_INT2              INT_MU2
#define MU_GN_INT3              INT_MU3
#endif

/*!
 * Number of channels in MU indicated using NUM_MU_CHANNELS
 */
#define NUM_MU_CHANNELS          4

/*!
 * Channel transmit/receive size indicated using CH_SIZE
 */
#define CH_SIZE                  4
#endif /* defined(__KERNEL__) && defined(CONFIG_MOT_WFN414) */

/*!
 * Maximum Buffer Size is a sequence of 16, 32, 64, 128,256,...
 * ie., a value where only the MSB is 1. The value should be a
 * power of 2. It should be noted that the buffer is filled and
 * emptied as multiples of 4 bytes. Also, 4 bytes of the total allocated
 * space will not be used since empty space calculation always returns
 * available space minus 1 inorder to differentiate between full and empty
 * buffer.
 */
#define MAX_BUF_SIZE             1024
#define MU_MAJOR                 0

#ifdef CONFIG_MOT_WFN377

/*!
 * Buffer control flags.
 *
 * MUFLAG_SWAPREADBYTES - any data that is read will have its bytes swapped
 * MUFLAG_SWAPWRITEBYTES - any data that is written will have its bytes swapped
 */
#define MUFLAG_SWAPREADBYTES  (1<<0)
#define MUFLAG_SWAPWRITEBYTES (1<<1)

#endif

/*!
 * IOCTL commands
 */

/*!
 * SENDINT      : To issue general purpose interrupt to DSP
 */
#define SENDINT                  1

/*!
 * SENDNMI      : To issue NMI to DSP
 */
#define SENDNMI                  2

/*!
 * SENDDSPRESET : To issue DSP Hardware Reset
 */
#define SENDDSPRESET             3

/*!
 * SENDMURESET  : To issue MU Reset
 */
#define SENDMURESET              4

/*!
 * RXENABLE     : To enable receive interrupt
 */
#define RXENABLE                 5

/*!
 * TXENABLE     : To enable transmit interrupt
 */
#define TXENABLE                 6

/*!
 * GPENABLE     : To enable general purpose interrupt
 */
#define GPENABLE                 7

#ifdef CONFIG_MOT_WFN377

/*!
 * SWAPMUREADBYTES : Toggle the byte swapping on read data
 */
#define SWAPMUREADBYTES          8

/*!
 * SWAPMUWRITEBYTES : Toggle the byte swapping on written data
 */
#define SWAPMUWRITEBYTES         9

#endif

#if !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_SDMA_DEBUG)
#define MXC_MU_SDMA_DUMP_DEBUG  10


/*!
 * MXC_MU_SDMA_DUMP_REGISTER : Dump sdma registers when bp panic or suapi panic
 * appears
 */
#define MXC_MU_SDMA_DUMP_REGISTER 11

#endif

/*!
 * Error Indicators
 */
/*!
 * Error: NO_DATA: If no data is available in receive register or
 * if no transmit register is empty, this error is returned
 */
#define NO_DATA                  129

/*!
 * Error: NO_ACCESS: If user is accessing an incorrect register i.e., a register
 * not allocated to user, this error is returned
 */
#define NO_ACCESS                132


#ifdef CONFIG_MOT_WFN374

#ifdef CONFIG_MOT_FEAT_MU_TEST

/*!
 * MUTEST_INJECTDATA     : Inject data to be read so we don't need a BP to test
 */
#define MUTEST_INJECTDATA 5000

#endif /* #ifdef CONFIG_MOT_FEAT_MU_TEST */

#endif /* #ifdef CONFIG_MOT_WFN374 */

#if defined(__KERNEL__) && defined(CONFIG_MOT_WFN414)
/*!
 * enum mu_oper
 * This datatype is to indicate whether the operation
 * related to receive, transmit or general purpose
 */
enum mu_oper {
	TX,
	RX,
	GP,
};

/*!
 * To declare array of function pointers of type callbackfn
 */
typedef void (*callbackfn) (int chnum);

/*!
 * This function is called by other modules to bind their
 * call back functions.
 *
 * @param   chand         The channel handler associated with the channel
 *                        to be accessed is passed
 * @param   callback      the caller's callback function
 * @param   muoper        The value passed is TX, RX, GP
 *
 * @return                Returns 0 on success or REG_INVALID error if invalid
 *                        channel is obtained from the channel handler
 *                        Returns -1 if muoper is other than RX, TX or GP
 */
int mxc_mu_bind(int chand, callbackfn callback, enum mu_oper muoper);

/*!
 * This function is called by other modules to unbind their
 * call back functions
 *
 * @param   chand    The channel handler associated with the channel
 *                   to be accessed is passed
 * @param   muoper   The value passed is TX, RX, GP
 *
 * @return           Returns 0 on success or REG_INVALID error if invalid
 *                   channel is obtained from the channel handler
 *                   Returns -1 if muoper is other than RX, TX or GP, or
 *                   if disabling interrupt failed
 */
int mxc_mu_unbind(int chand, enum mu_oper muoper);

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
 *                  If invalid channel is passed, it returns REG_INVALID error.
 *                  If no channel is available, returns error.
 *                  Returns channel handler - on success
 *                  Returns -1 - on error
 */
int mxc_mu_alloc_channel(int chnum);

/*!
 * Deallocating Resources:
 *
 * @param   chand   The channel handler associated with the channel
 *                  that is to be freed is passed.
 *
 * @return          Returns 0 on success and -1 if channel was not
 *                  already allocated or REG_INVALID error if invalid
 *                  channel is obtained from the channel handler
 */
int mxc_mu_dealloc_channel(int chand);

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
 *                        is accessed or REG_INVALID error if invalid
 *                        channel is obtained from the channel handler
 *                        Returns -1 if the buffer passed is not allocated
 */
int mxc_mu_mcuread(int chand, char *mu_msg);

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
 *                        is accessed or REG_INVALID error if invalid
 *                        channel is obtained from the channel handler
 *                        Returns -1 if the buffer passed is not allocated
 */
int mxc_mu_mcuwrite(int chand, char *mu_msg);

/*!
 * This function is called by the user and other modules to enable desired
 * interrupt
 *
 * @param   chand     The channel handler associated with the channel
 *                    to be accessed is passed
 * @param    muoper   The value passed is TX, RX, GP
 *
 * @return           Returns 0 on success or REG_INVALID error if invalid
 *                   channel is obtained from the channel handler
 *                   Returns -1 if muoper is other than RX, TX or GP
 */
int mxc_mu_intenable(int chand, enum mu_oper muoper);

/*!
 * This function is called by unbind function of this module and
 * can also be called from other modules to enable desired
 * receive Interrupt
 *
 * @param   chand    The channel handler associated with the channel
 *                   whose interrupt to be disabled is passed
 * @param   muoper   The value passed is TX, RX, GP
 *
 * @return           Returns 0 on success or REG_INVALID error if invalid
 *                   channel is obtained from the channel handler
 *                   Returns -1 if muoper is other than RX, TX or GP
 */
int mxc_mu_intdisable(int chand, enum mu_oper muoper);

/*!
 * This function is used by other modules to reset the MU Unit. This would reset
 * both the MCU side and the DSP side
 */
int mxc_mu_reset(void);

/*!
 * This function is used by other modules to issue DSP hardware reset.
 */
int mxc_mu_dsp_reset(void);

/*!
 * This function is used by other modules to deassert DSP hardware reset.
 */
int mxc_mu_dsp_deassert(void);

/*!
 * This function is used by other modules to issue DSP Non-Maskable
 * Interrupt to the DSP
 */
int mxc_mu_dsp_nmi(void);

/*!
 * This function is used by other modules to retrieve the DSP reset state.
 *
 * @return   Returns 1 if DSP in reset state and 0 otherwise
 */
int mxc_mu_dsp_reset_status(void);

/*!
 * This function is used by other modules to retrieve the DSP Power Mode.
 *
 * @return   Returns a value from which power mode of the DSP side of
 *           of MU unit can be inferred
 *           0 - Run mode,  1 - Wait mode
 *           2 - Stop mode, 3 - DSM mode
 */
unsigned int mxc_mu_dsp_pmode_status(void);

#ifdef CONFIG_MOT_FEAT_PM
/*!
 * This function is used by an IPC module to switch the Interrupt-of-Interest
 * behavior on and off for the MU channel used for the Power Management
 * messages.
 *
 * @param   swtch     boolean on/off indicator
 *
 * @return   none
 */
void mxc_mu_set_ioi_response(unsigned int swtch);
#endif
#endif /* defined(__KERNEL__) && defined(CONFIG_MOT_WFN414) */

#endif				/* MU_AS_H */
