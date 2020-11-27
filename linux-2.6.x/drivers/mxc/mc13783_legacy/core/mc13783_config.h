/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

#ifndef __MC13783_CONFIG_H__
#define __MC13783_CONFIG_H__

 /*!
  * @file mc13783_config.h
  * @brief This file contains configuration define used by mc13783 protocol.
  *
  * @ingroup MC13783
  */

/*
 * Includes
 */
#include <linux/kernel.h>

#include "mc13783_external.h"

#define CHECK_ERROR(a) do { \
        int ret = (a); \
        if(ret != ERROR_NONE) \
return ret; \
} while (0);

/* NULL pointer */
#if !defined(NULL)
#define NULL    ((void *) 0)
#endif

#ifdef __ALL_TRACES__
#define TRACEMSG(fmt,args...) printk(fmt,##args)
#else				/* __ALL_TRACES__ */
#define TRACEMSG(fmt,args...)
#endif				/* __ALL_TRACES__ */

#define TRACEMSG_ALL_TIME(fmt,args...)  printk(fmt,##args)

enum {
	/* interrupt status 0 */
	ISR_MASK_ADCDONE = 0X0000001,
	ISR_MASK_ADCBISDONE = 0X0000002,
	ISR_MASK_TSI = 0X0000004,
	ISR_MASK_WHIGHI = 0X0000008,
	ISR_MASK_WLOWI = 0X0000010,
	ISR_MASK_CHGDETI = 0X0000040,
	ISR_MASK_CHGOVI = 0X0000080,
	ISR_MASK_CHGREVI = 0X0000100,
	ISR_MASK_CHGSHORTI = 0X0000200,
	ISR_MASK_CCCVI = 0X0000400,
	ISR_MASK_CHGCURRI = 0X0000800,
	ISR_MASK_BPONI = 0X0001000,
	ISR_MASK_LOBATLI = 0X0002000,
	ISR_MASK_LOBATHI = 0X0004000,
	ISR_MASK_USBI = 0X0010000,
	ISR_MASK_IDI = 0X0080000,
	ISR_MASK_SE1I = 0X0200000,
	ISR_MASK_CKDETI = 0X0400000,

	/* interrupt status 1 */
	ISR_MASK_1HZI = 0X0000001,
	ISR_MASK_TODAI = 0X0000002,
	ISR_MASK_ONOFD1I = 0X0000008,
	ISR_MASK_ONOFD2I = 0X0000010,
	ISR_MASK_ONOFD3I = 0X0000020,
	ISR_MASK_SYSRSTI = 0X0000040,
	ISR_MASK_RTCRSTI = 0X0000080,
	ISR_MASK_PCI = 0X0000100,
	ISR_MASK_WARMI = 0X0000200,
	ISR_MASK_MEMHLDI = 0X0000400,
	ISR_MASK_PWRRDYI = 0X0000800,
	ISR_MASK_THWARNLI = 0X0001000,
	ISR_MASK_THWARNHI = 0X0002000,
	ISR_MASK_CLKI = 0X0004000,
	ISR_MASK_SEMAFI = 0X0008000,
	ISR_MASK_MC2BI = 0X0020000,
	ISR_MASK_HSDETI = 0X0040000,
	ISR_MASK_HSLI = 0X0080000,
	ISR_MASK_ALSPTHI = 0X0100000,
	ISR_MASK_AHSSHORTI = 0X0200000,

};

#define	 ISR_MASK_EVENT_IT_0  	        (ISR_MASK_ADCDONE	       |\
                                        ISR_MASK_ADCBISDONE            |\
                                        ISR_MASK_TSI                   |\
                                        ISR_MASK_WHIGHI                |\
                                        ISR_MASK_WLOWI                 |\
                                        ISR_MASK_CHGDETI               |\
                                        ISR_MASK_CHGOVI                |\
                                        ISR_MASK_CHGREVI               |\
                                        ISR_MASK_CHGSHORTI             |\
                                        ISR_MASK_CCCVI                 |\
                                        ISR_MASK_CHGCURRI              |\
                                        ISR_MASK_BPONI                 |\
                                        ISR_MASK_LOBATLI               |\
                                        ISR_MASK_LOBATHI               |\
                                        ISR_MASK_USBI                  |\
                                        ISR_MASK_IDI                   |\
                                        ISR_MASK_SE1I                  |\
                                        ISR_MASK_CKDETI                 )

#define	 ISR_MASK_EVENT_IT_1            (ISR_MASK_1HZI	               |\
                                        ISR_MASK_TODAI                 |\
                                        ISR_MASK_ONOFD1I               |\
                                        ISR_MASK_ONOFD2I               |\
                                        ISR_MASK_ONOFD3I               |\
                                        ISR_MASK_SYSRSTI               |\
                                        ISR_MASK_RTCRSTI               |\
                                        ISR_MASK_PCI                   |\
                                        ISR_MASK_WARMI                 |\
                                        ISR_MASK_MEMHLDI               |\
                                        ISR_MASK_PWRRDYI               |\
                                        ISR_MASK_THWARNLI              |\
                                        ISR_MASK_THWARNHI              |\
                                        ISR_MASK_CLKI                  |\
                                        ISR_MASK_SEMAFI                |\
                                        ISR_MASK_MC2BI                 |\
                                        ISR_MASK_HSDETI                |\
                                        ISR_MASK_HSLI                  |\
                                        ISR_MASK_ALSPTHI               |\
                                        ISR_MASK_AHSSHORTI              )

#define MASK_USED_BITS   0x00FFFFFF
#define ISR_NB_BITS 24

typedef struct timer_list osi_timer;

/*
#define __ALL_TRACES__
*/

#define __TEST_CODE_ENABLE__

#define	_A(a)		"mc13783 : "a" \n"
#define _K_A(a)         _A(a)
#define _K_D(a)         KERN_DEBUG _A(a)
#define _K_I(a)         KERN_INFO _A(a)
#define _K_W(a)         KERN_WARNING _A(a)

#ifdef          __INIT_TAB_DEF

/*!
 * This structure contains initialized value of register.
 */
static unsigned int tab_init_reg[][2] = {
	{REG_INTERRUPT_MASK_0, MASK_USED_BITS},	/*clear all IT mask    */
	{REG_INTERRUPT_STATUS_0, MASK_USED_BITS},	/*clear all IT status  */
	{REG_INTERRUPT_MASK_1, MASK_USED_BITS},	/*clear all IT mask 1  */
	{REG_INTERRUPT_STATUS_1, MASK_USED_BITS},	/*clear all IT status 1 */
	{-1, -1},
};
#endif

/*!
 * This enumeration define all error code used in mc13783 drivers.
 */

enum error_code {
	/*!
	 * none error
	 */
	ERROR_NONE = 0,
	/*!
	 * error process in use
	 */
	ERROR_INUSE = -1,
	/*!
	 * error in SPI initialize driver
	 */
	ERROR_SPIINIT = -2,
	/*!
	 * error in initialize section
	 */
	ERROR_INIT = -3,
	/*!
	 * error number of client overflow
	 */
	ERROR_CLIENT_NBOVERFLOW = -4,
	/*!
	 * error in malloc function
	 */
	ERROR_MALLOC = -5,
	/*!
	 * error in un-subscribe event
	 */
	ERROR_UNSUBSCRIBE = -6,
	/*!
	 * error event occur and not subscribed
	 */
	ERROR_EVENT_NOT_SUBSCRIBED = -7,
	/*!
	 * error bad call back
	 */
	ERROR_EVENT_CALL_BACK = -8,
	/*!
	 * error input arg is wrong.
	 */
	ERROR_BAD_INPUT = -9,
};

#endif				/* __MC13783_CONFIG_H__ */
