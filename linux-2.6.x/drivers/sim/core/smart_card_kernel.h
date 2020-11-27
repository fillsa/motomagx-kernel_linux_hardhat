/*
 * Copyright (C) 2005-2006 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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
 * Motorola 2006-Aug-31 - Remove some functionality from the driver
 * Motorola 2006-Aug-21 - Add support for all peripheral clock frequencies
 * Motorola 2006-Jul-25 - More MVL upmerge changes
 * Motorola 2006-Jul-14 - MVL upmerge
 * Motorola 2006-May-24 - Fix GPL issues
 * Motorola 2005-Oct-04 - Initial Creation
 */
 
#ifndef __SMART_CARD_KERNEL_H__
#define __SMART_CARD_KERNEL_H__

#ifdef __KERNEL__

#include <asm/hardware.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>

/************** STRUCTURES, ENUMS, AND TYPEDEFS *******************************/
typedef signed char INT8;
typedef unsigned char UINT8;
typedef signed short int INT16;
typedef unsigned short int UINT16;
typedef signed int INT32;
typedef unsigned int UINT32;
typedef unsigned char BOOL;
typedef unsigned char BOOLEAN;

typedef struct
{
    UINT32 port1_cntl;      /* 0x00 */
    UINT32 setup;           /* 0x04 */
    UINT32 port1_detect;    /* 0x08 */
    UINT32 port1_xmt_buf;   /* 0x0C */
    UINT32 port1_rcv_buf;   /* 0x10 */
    UINT32 port0_cntl;      /* 0x14 */
    UINT32 cntl;            /* 0x18 */
    UINT32 clk_prescaler;   /* 0x1C */
    UINT32 rcv_threshold;   /* 0x20 */
    UINT32 enable;          /* 0x24 */
    UINT32 xmt_status;      /* 0x28 */
    UINT32 rcv_status;      /* 0x2C */
    UINT32 int_mask;        /* 0x30 */
    UINT32 port0_xmt_buf;   /* 0x34 */
    UINT32 port0_rcv_buf;   /* 0x38 */
    UINT32 port0_detect;    /* 0x3C */
    UINT32 data_format;     /* 0x40 */
    UINT32 xmt_threshold;   /* 0x44 */
    UINT32 guard_cntl;      /* 0x48 */
    UINT32 od_config;       /* 0x4C */
    UINT32 reset_cntl;      /* 0x50 */
    UINT32 char_wait;       /* 0x54 */
    UINT32 gpcnt;           /* 0x58 */
    UINT32 divisor;         /* 0x5C */
    UINT32 bwt;             /* 0x60 */
    UINT32 bgt;             /* 0x64 */
    UINT32 bwt_h;           /* 0x68 */
    UINT32 xmt_fifo_stat;   /* 0x6C */
    UINT32 rcv_fifo_ctl;    /* 0x70 */
    UINT32 rcv_fifo_wptr;   /* 0x74 */
    UINT32 rcv_fifo_rptr;   /* 0x78 */
} SIM_MODULE_REGISTER_BANK;


/*******************************************************************************************
 * Constants
 *******************************************************************************************/

#if (defined(SIM_BASE_ADDR) && !defined(SIM1_BASE_ADDR))
# define SIM1_BASE_ADDR SIM_BASE_ADDR
#endif

#if (defined(INT_SIM_IPB) && !defined(INT_SIM_GENERAL))
# define INT_SIM_GENERAL INT_SIM_IPB 
#endif

#if (defined(SPBA_SIM) && !defined(SPBA_SIM1))
# define SPBA_SIM1  SPBA_SIM
#endif

#define SIM_DEV_NAME                       "sim"

#define SIM_REMOVAL                         1
#define SIM_INSERTION                       0

#define TRUE                                1
#define FALSE                               0

/* port1_cntl bits */
#define SAPD1                       0x0001
#define SVEN1                       0x0002
#define STEN1                       0x0004
#define SRST1                       0x0008
#define SCEN1                       0x0010
#define SCSP1                       0x0020
#define S3VOLT1                     0x0040
#define SFPD1                       0x0080

/* setup bits */
#define AMODE                       0x0001
#define SPS                         0x0002

/* port1_detect bits */
#define SDIM1                       0x0001
#define SDI1                        0x0002
#define SPDP1                       0x0004
#define SPDS1                       0x0008

/* port1_xmt_buf bits */
#define XMT_MASK1                   0x00FF

/* port1_rcv_buf bits */
#define RCV_MASK1                   0x00FF
#define PE1                         0x0100
#define FE1                         0x0200
#define CWT1                        0x0400

/* port0_cntl bits */
#define SAPD0                       0x0001
#define SVEN0                       0x0002
#define STEN0                       0x0004
#define SRST0                       0x0008
#define SCEN0                       0x0010
#define SCSP0                       0x0020
#define S3VOLT0                     0x0040
#define SFPD0                       0x0080

/* cntl bits */
#define ICM                         0x0002
#define ANACK                       0x0004
#define ONACK                       0x0008
#define SAMPLE12                    0x0010
#define BAUD_SEL_MASK               0x01D0
#define BAUD_SEL_SHIFT              6
#define GPCNT_CLK_SEL_MASK          0x0600
#define GPCNT_CLK_SEL_SHIFT         9
#define CWTEN                       0x0800
#define LRCEN                       0x1000
#define CRCEN                       0x2000
#define XMT_CRC_LRC                 0x4000
#define BWTEN                       0x8000

/* clk_prescaler bits */
#define CLK_PRESCALER_MASK          0x00FF

/* rcv_threshold bits */
#define RDT_MASK                    0x01FF
#define RTH_MASK                    0x1E00

/* sim_enable bits */
#define RCV_EN                      0x0001
#define XMT_EN                      0x0002

/* xmt_status bits */
#define XTE                         0x0001
#define TFE                         0x0008
#define ETC                         0x0010
#define TC                          0x0020
#define TFO                         0x0040
#define TDTF                        0x0080
#define GPCNT                       0x0100

/* rcv_status bits */
#define OEF                         0x0001
#define RFD                         0x0010
#define RDRF                        0x0020
#define LRCOK                       0x0040
#define CRCOK                       0x0080
#define CWT                         0x0100
#define RTE                         0x0200
#define BWT                         0x0400
#define BGT                         0x0800

/* int_mask bits */
#define RIM                         0x0001
#define TCIM                        0x0002
#define OIM                         0x0004
#define ETCIM                       0x0008
#define TFEIM                       0x0010
#define XTM                         0x0020
#define TFOM                        0x0040
#define TDTFM                       0x0080
#define GPCNTM                      0x0100
#define CWTM                        0x0200
#define RTM                         0x0400
#define BWTM                        0x0800
#define BGTM                        0x1000
#define SIM_INT_MASK_ALL            0x1FFF

/* port0_xmt_buf bits */
#define XMT_MASK0                   0x00FF

/* port0_rcv_buf bits */
#define RCV_MASK0                   0x00FF
#define PE0                         0x0100
#define FE0                         0x0200
#define CWT0                        0x0400

/* port0_detect bits */
#define SDIM0                       0x0001
#define SDI0                        0x0002
#define SPDP0                       0x0004
#define SPDS0                       0x0008

/* data_format bits */
#define IC                          0x0001

/* xmt_threshold bits */
#define TDT_MASK                    0x000F
#define XTH_MASK                    0x00F0
#define XTH_SHIFT                   4

/* guard_cntl bits */
#define GETU_MASK                   0x00FF
#define RCVR11                      0x0100

/* od_config bits */
#define OD_P0                       0x0001
#define OD_P1                       0x0002

/* reset_cntl bits */
#define FLUSH_RCV                   0x0001
#define FLUSH_XMT                   0x0002
#define SOFT_RST                    0x0004
#define KILL_CLOCK                  0x0008
#define DOZE                        0x0010
#define STOP                        0x0020
#define DEBUG                       0x0040

/* char_wait bits */
#define CWT_MASK                    0xFFFF

/* gpcnt bits */
#define GPCNT_MASK                  0xFFFF

/* divisor bits */
#define DIVISOR_MASK                0x007F

/* bwt bits */
#define BWT_MASK                    0xFFFF

/* bgt bits */
#define BGT_MASK                    0xFFFF

/* bwt_h bits */
#define BWT_H_MASK                  0xFFFF

/* xmt_fifo_stat bits */
#define XMT_RPTR_MASK               0x000F
#define XMT_WPTR_MASK               0x00F0
#define XMT_CNT_MASK                0x0F00

/* rcv_fifo_cnt bits */
#define RCV_CNT_MASK                0x01FF

/* rcv_fifo_wptr bits */
#define RCV_WPTR_MASK               0x01FF

/* rcv_fifo_rptr bits */
#define RCV_RPTR_MASK               0x01FF
#define MAX_CWTCNT_VALUE            65535
#define MAX_GPCNT_VALUE             65535
#define NACK_MAX                    4

/* Threshold value for the Transmit FIFO at which the TDTF bit in the   */
/* XMT_STATUS register will be set.                                     */
#define TDT_VALUE                   6

#define SIM_MODULE_RX_FIFO_SIZE         259
#define SIM_MODULE_TX_FIFO_SIZE               15
#define SIM_MODULE_CWT_OFFSET                 1
#define SIM_MODULE_TX_FIFO_FREE           (SIM_MODULE_TX_FIFO_SIZE - TDT_VALUE)
#define SIM_MODULE_MAX_NULL_PROCEDURE_BYTES   500
#define SIM_MODULE_MAX_DATA                  (260 + SIM_MODULE_MAX_NULL_PROCEDURE_BYTES)

#endif

#define SIM_NUM                       230

#endif /* __SMART_CARD_KERNEL_H__ */
