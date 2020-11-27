/** *********************************************************************************
 ** *********************************************************************************
 ** fx2lp_hardware.h - FX2LP specific macros, declarations
 **
 ** Cypress FX2LP HiSpeed USB Peripheral Controller driver
 **
 ** Portability:
 **     This file contains FX2LP H/W specific code. Not a portable to other platforms
 **
 ** *********************************************************************************
 ** *********************************************************************************/

/*
 *  Copyright (c) 2006-2008, Motorola, All Rights Reserved.
 *
 *  This program is licensed under a BSD license with the following terms:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 *  Neither the name of Motorola nor the names of its contributors may be
 *  used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * Mar  8, 2006       Motorola, Inc.   Initial create
 * Sep 21, 2006       Motorola, Inc.   Define a wait queue for testing HS tools 
 * Oct 30, 2006       Motorola, Inc.   Add new elements to structures
 * Jul 26, 2007       Motorola, Inc.   Changes for open source compliance.
 * Aug 24, 2007       Motorola, Inc.   Changes for open source compliance.
 * Nov 14, 2008       Motorola, Inc.   Clean the work queue before module exit.
*/

#if !defined(__FX2LP_HARDWARE_H__)
#define __FX2LP_HARDWARE_H__

#include<linux/workqueue.h>
#include <otg/otg-linux.h>
#include <asm/semaphore.h>
#include "fx2lp_ready_events.h"

#define FX2LP_DETAIL_EVENTS     0x01
#define FX2LP_DETAIL_DATAFLOW   0x02
#define FX2LP_DETAIL_REGISTERS  0x04
#define FX2LP_DETAIL_INIT       0x08
#define FX2LP_DETAIL_RWDATA     0x10
#define FX2LP_DETAIL_COPYSUMMARY 0x20
#define FX2LP_DETAIL_KPF (0)

//**************************
//*** This is a fix for ACM urb size issue for hispeed bulk out packets
//*** Once ACM fix is available, undefine this macro
//#define FX2LP_ACM_URB_FIX  

//-----------------------------------------------------------------------------------
//  GENERAL MACROS
//-----------------------------------------------------------------------------------
typedef unsigned char*                   pu8;
typedef unsigned short*                  pu16;
typedef unsigned long*                   pu32;
typedef void *                           HANDLE;
typedef void                             VOID;
typedef void *                           PVOID;
typedef int                              FX2LP_BOOL;
typedef int *                            PFX2LP_BOOL;

#define GPIO_HIGH     1
#define GPIO_LOW      0
#define __BIG_ENDIAN

#define PASS                             1
#define FAIL                             0
#define HIBYTE(x)                        ((x) >> 8)
#define LOBYTE(x)                        ((x) & 0xFF)
#define swab(x)                          (HIBYTE(x) | (LOBYTE(x) << 8))
#ifdef __BIG_ENDIAN
#define FX2LP_WORD_SWAP(x)               (x)
#else  
#define FX2LP_WORD_SWAP(x)               swab(x)
#endif /* _BYTE_ORDER == _BIG_ENDIAN */


//-----------------------------------------------------------------------------------
//  FX2LP SPECIFIC  GENERAL MACROS
//-----------------------------------------------------------------------------------
#define FX2LP_NAME                        "FX2LP"
#define FX2LP_MAX_ENDPOINTS               18 
#define FX2LP_EP0_PACKETSIZE              0x40

#define EP0_PACKET_SIZE                   0x40
#define EP1_PACKET_SIZE                   0x40
#define EP2_PACKET_SIZE_FS                0x40
#define EP6_PACKET_SIZE_FS                0x40
#define EP2_PACKET_SIZE_HS                0x200
#define EP6_PACKET_SIZE_HS                0x200

#define FX2LP_OUT_AS_BH         TRUE
#define FX2LP_IN_AS_BH          TRUE

//-----------------------------------------------------------------------------------
//  FX2LP SPECIFIC  DECLARATIONS
//-----------------------------------------------------------------------------------

static DECLARE_WAIT_QUEUE_HEAD(wkupfail_wait_queue);


typedef enum _DEVICE_EVENT {
        FX2LP_RESUMED= 0,
        FX2LP_SUSPENDED
} DEVICE_EVENT, *PDEVICE_EVENT;

typedef enum _USB_BUS_SPEED {
        BS_UNKNOWN_SPEED         = 0,
        BS_FULL_SPEED            = 0,
        BS_HIGH_SPEED            = 1
} USB_BUS_SPEED, *PUSB_BUS_SPEED;

typedef enum {
        FX2LP_EP0    = 0,
        FX2LP_EP1,    
        FX2LP_EP2,    
        FX2LP_EP6,
        FX2LP_NUM_EPS,
        FX2LP_INVALID = FX2LP_NUM_EPS
}FX2LP_EPX;

/* static (constant) information about a given endpoint */
typedef struct
{
    FX2LP_RDY_EV_NUM_T stallev; /* event to stall this one */
    FX2LP_RDY_EV_NUM_T short_packet_event; /* to send short packet (2,6) */
    FX2LP_RDY_EV_NUM_T shortack_event;     /* to ack short packet (2,6) */
    u8          phys_epn;       /* physical endpoint number */
    FX2LP_EPX   epx;            /* endpoint index */
    u8          config_reg;     /* config reg for this endpoint */
    u8          pktlen_reg;     /* packet length register (2,6) */
    u8          ep_pf_reg;      /* EPxPF regster (2,6) */
} EPX_INFO_T;

/* defines for mask to remember which config steps remain. */
#define FX2LP_DO_PKTLENH        0x01
#define FX2LP_DO_PKTLENL        0x02
#define FX2LP_DO_EPXPFH         0x04
#define FX2LP_DO_EPXPFL         0x08
#define FX2LP_DO_DATACOMMIT     0x10
#define FX2LP_DO_CONFIG         0x20
#define FX2LP_DO_INTRCFG        0x40

typedef struct _FX2LP_PCD FX2LP_PCD, *PFX2LP_PCD;

/* Pointers to functions that do actual in/out processing */
typedef void (FX2LP_OUT_DATA_FCN)(PFX2LP_PCD, struct usbd_endpoint_instance *);
typedef void (FX2LP_IN_DATA_FCN)(PFX2LP_PCD, struct usbd_endpoint_instance *);

/* 
 * information about pending OUT packets.
 * Most of this is needed for EPs 2 and 6, but EP0 can use
 * what fields it might need.
 */
typedef struct
{
    u16         outavail;       /* count of OUTNE received, not processed */
    short       shortpend;      /* if >=0, short packet notification has been
                                   received but not acknowledged */
    short       rcd_pktlen;     /* size of next packet to process */
} EP_OUTINFO_T;

typedef struct _EP_STATUS {
    struct usbd_endpoint_instance *endpoint_inst;
    struct WORK_STRUCT  start_rxtx;
    FX2LP_IN_DATA_FCN   *start_in_data;
    FX2LP_OUT_DATA_FCN  *do_out_data;
    EP_OUTINFO_T        outinfo;
    volatile pu16       pfifo_addr;
    u16                 pkt_size_assigned;
    u8                  ep_num;
    u8                  doconfig;
    u8                  bufsavail;
    FX2LP_BOOL          is_initialized;
    u8      ep_dir;

    u8                  cfg_reg_val;
    u8                  attributes;
    u8                  dmaflag;        /* DMA flag to write to SDMA */
    FX2LP_EPX           epx;
    FX2LP_BOOL          is_used;
    FX2LP_BOOL          sending_in_short;
    FX2LP_BOOL          bh_active;      /* true if BH is running */
    FX2LP_BOOL          send_zlp;      /* true if we need to send a zlp packet for a setup */
} EP_STATUS, *PEP_STATUS;

struct _FX2LP_PCD {
    FX2LP_BOOL          is_initialized;
    FX2LP_RDY_EV_MASK_T ready_events;   /* pending ready events */
    FX2LP_RDY_EV_MASK_T wakeup_events;  /* wakeup if one of these is done */
    const FX2LP_RDY_EV_INFO_T *curevent;      /* current ready pin owner */
#ifdef LINUX26
    struct workqueue_struct * fx2lp_bus_workqueue; /* workqueue for all fx2lp work_struct bottom half */
#endif
    struct WORK_STRUCT  bus_suspend_bh;
    struct WORK_STRUCT  bus_resume_bh;
    struct WORK_STRUCT  config_bus_speed_bh;
    struct semaphore    readreg_sem;    /* for task-level operations */

    struct pcd_instance *pcd;
    EP_STATUS           pep_status[FX2LP_NUM_EPS];
    volatile u16*       pintr_addr;   // check - is this be u32* or u16*
    volatile u16*       pcmd_in_addr;
    volatile u16*       pcmd_out_addr;
    u16                 wout_read_reg_resp_cmd;
    u16                 eintr_en;     // extern interrupts enabled
    u8                  ifconfig;
    FX2LP_BOOL          ep0_partial;

    DEVICE_EVENT        device_state;
    FX2LP_BOOL          is_cmd_rdy;
    FX2LP_BOOL          is_read_response_received;
    FX2LP_BOOL          is_fx2lp_ready;
    FX2LP_BOOL          is_pcd_mod_init;

    u8                  register_to_read;
};

#define PFX2LP_PCD(P) ((P)->pcd)
#define PCD_PFX2LP(P) ((PFX2LP_PCD)(P)->privdata)

//-----------------------------------------------------------------------------------
//  FX2LP SPECIFIC REGISTER OFFSET
//-----------------------------------------------------------------------------------
#define FX2LP_MAX_COMMAND_LENGTH    0x20
#define UNUSED_REGISTER_NUMBER      0xFF

//#define FX2LP_IS_HIGH_SPEED()   ((fx2lp_pcd.bus_speed == BS_HIGH_SPEED )? 1 : 0)


typedef enum FX2LP_REGS_tag
{
    // General Configuration
    FX2LP_REG_IFCONFIG        =   0x01,    // Interface configuration        
    FX2LP_REG_FLAGSAB         =   0x02,    // FIFO FLAGA and FLAGB Assignments
    FX2LP_REG_FLAGSCD         =   0x03,    // FIFO FLAGC and FLAGD Assignments
    FX2LP_REG_POLAR           =   0x04,    // FIFO Polarities
    FX2LP_REG_REVID           =   0x05,    // Chip Revision

    // Endpoint Configuration
    FX2LP_REG_EP2CFG          =   0x06,    // EP2 configuration
    // FX2LP_REG_EP1OUTCFG    =   0x07,    // RESERVED
    FX2LP_REG_EP6CFG          =   0x08,    // EP6 configuration
    FX2LP_REG_EP1INCFG        =   0x09,    // EP1-IN configuration

    FX2LP_REG_EP2PKTLENH      =   0x0A,    // Endpoint 2 Pkt Length H
    FX2LP_REG_EP2PKTLENL      =   0x0B,    // Endpoint 2 Pkt Length L 
    // FX2LP_REG_EP4PKTLENH   =   0x0C,    // RESERVED
    // FX2LP_REG_EP4PKTLENL   =   0x0D,
    FX2LP_REG_EP6PKTLENH      =   0x0E,    // Endpoint 6 Pkt Length H
    FX2LP_REG_EP6PKTLENL      =   0x0F,    // Endpoint 6 Pkt Length L
    // FX2LP_REG_EP8PKTLENH   =   0x10,
    // FX2LP_REG_EP8PKTLENL   =   0x11,

    FX2LP_REG_EP2PFH          =   0x12,    // Endpoint 2 Programmable Flag H
    FX2LP_REG_EP2PFL          =   0x13,    // Endpoint 2 Programmable Flag L

    FX2LP_REG_EP4PFH          =   0x14,
    FX2LP_REG_EP4PFL          =   0x15,
    FX2LP_REG_EP6PFH          =   0x16,
    FX2LP_REG_EP6PFL          =   0x17,
    FX2LP_REG_EP8PFH          =   0x18,
    FX2LP_REG_EP8PFL          =   0x19,

    // FX2LP_REG_EP2ISOINPKTS =   0x1A,
        
    FX2LP_REG_EP1OUTEN        =   0x1B,    // Enable=1: Enable  ;  Enable=0: Disable ; Mode=1: Interrupt ;   Mode=0: Bulk
    FX2LP_REG_EP4OUTEN        =   0x1C,    // Enable=1: Enable  ;  Enable=0: Disable ; Mode=1: Interrupt ;   Mode=0: Bulk
    FX2LP_REG_EP8INEN         =   0x1D,    // Enable=1: Enable  ;  Enable=0: Disable ; Mode=1: Interrupt ;   Mode=0: Bulk

    // FLAGS
    FX2LP_REG_EP2FLAGS        =   0x1E,    // Endpoint 2 FIFO Flags
    FX2LP_REG_EP6FLAGS        =   0x1F,    // Endpoint 6 FIFO Flags
    FX2LP_REG_EPINPKTEND      =   0x20,    // Force Packet End / Flush FIFOs

    // USB Configuration
    FX2LP_REG_USBFRAMEH       =   0x2A,    // USB Frame Count H
    FX2LP_REG_USBFRAMEL       =   0x2B,    // USB Frame Count L
    FX2LP_REG_MICROFRAME      =   0x2C,    // Microframe Count 0-7
    FX2LP_REG_FNADDR          =   0x2D,    // USB Function Address

    // Interrupts
    FX2LP_REG_INTENABLE       =   0x2E,    // Interrupt Enable
    FX2LP_REG_INTENABLE1      =   0x2F,
    // FX2LP_REG_DESC_RAM     =   0x30,

    // Endpoint 0
    // FX2LP_REG_EP0BUF       =   0x31,
       FX2LP_REG_SETUP        =   0x32,
    // FX2LP_REG_EP0BC        =   0x33

    // Un-Indexed Registers in XDATA Space
    FX2LP_REG_FIFOPINPOLAR    =   0xE609,   // FIFO Interface Pins Polarity
    FX2LP_REG_TOGCTL          =   0xE683    // Data Toggle Control

} FX2LP_REGS;

#define EP2_CFG_DEFAULT_VALUE    (0xA2)     // OUT Endpoint, BULK, Double Buffering (2x512byte)
#define EP6_CFG_DEFAULT_VALUE    (0xE2)     // IN Endpoint, BULK, Double Buffering
#define EP1IN_CFG_DEFAULT_VALUE  (0xB0)     // IN Endpoint, Interrupt  

//-----------------------------------------------------------------------------------
//  FX2LP SPECIFIC REGISTER DEFINITIONS
//  
//  D7      D6      D5      D4      D3      D2      D1      D0
//  80      40      20      10      8       4       2       1
//
//  Macros to get and set flags in FX2LP registers (Ref FX2LP+ Emulation Firmware)
//-----------------------------------------------------------------------------------

//*** IFCONFIG REGISTER
#define FX2LP_IFCLKSRC            0x80
#define FX2LP_3048MHZ             0x40
#define FX2LP_IFCLKOE             0x20
#define FX2LP_IFCLKPOL            0x10
#define FX2LP_ASYNC               0x08
#define FX2LP_STANDBY             0x04
#define FX2LP_FLAGDCS             0x02
#define FX2LP_DISCON              0x01

//*** FLAGSAB/FLAGSCD REGISTER
#define FX2LP_FLAGS_DEFAULT       0x0
// 1-3 are reserved
#define FX2LP_FLAGS_EP2_PF        0x4
#define FX2LP_FLAGS_EP4_PF        0x5
#define FX2LP_FLAGS_EP6_PF        0x6
#define FX2LP_FLAGS_EP8_PF        0x7
#define FX2LP_FLAGS_EP2_EF        0x8
#define FX2LP_FLAGS_EP4_EF        0x9
#define FX2LP_FLAGS_EP6_EF        0xA
#define FX2LP_FLAGS_EP8_EF        0xB
#define FX2LP_FLAGS_EP2_FF        0xC
#define FX2LP_FLAGS_EP4_FF        0xD
#define FX2LP_FLAGS_EP6_FF        0xE
#define FX2LP_FLAGS_EP8_FF        0xF
#define FX2LP_FLAGS_CMDOUT_PF     0x5
#define FX2LP_FLAGS_CMDIN_PF      0x7
#define FX2LP_FLAGS_CMDOUT_EF     0x9
#define FX2LP_FLAGS_CMDIN_EF      0xB
#define FX2LP_FLAGS_CMDOUT_FF     0xD
#define FX2LP_FLAGS_CMDIN_FF      0xF

//*** POLAR REGISTER
#define FX2LP_WUPOL               0x80
#define FX2LP_PKTEND              0x20
#define FX2LP_SLOE                0x10
#define FX2LP_SLRD                0x08
#define FX2LP_SLWR                0x04
#define FX2LP_EF                  0x02
#define FX2LP_FF                  0x01


//*** EPxCFG REGISTER
#define FX2LP_EP_VALID_FLAG       0x80
#define FX2LP_EP_DIRECTION_IN     0x40
#define FX2LP_EP_SIZE_1024_FLAG   0x08
#define FX2LP_EP_STALL_FLAG       0x04
#define FX2LP_EP_BUF_DOUBLE       0x02


//*** EP2(6)PKTLENH 
#define FX2LP_FLAG_ZEROLEN        0x20
#define FX2LP_FLAG_WORDWIDE       0x10

//*** EP2(6)PFH
#define FX2LP_FLAG_DECIS          0x80   

//*** EP2 (6) FLAGS Register - Give's current state of the EPx PF, FF, EF
#define FX2LP_EPFLAG_PF           0x04
#define FX2LP_EPFLAG_EF           0x02
#define FX2LP_EPFLAG_FF           0x01

//*** INPKTEND/FLUSH REGISTER
#define FX2LP_FLAG_FLUSH_FIFO6    (0x04<<4)
#define FX2LP_FLAG_FLUSH_FIFO2    (0x01<<4)

#define FX2LP_FLAG_PKTEND_EP6     0x06
#define FX2LP_FLAG_PKTEND_EP2     0x02

#define FX2LP_EPxPF               0x4
#define FX2LP_EPxEF               0x2
#define FX2LP_EPxFF               0x1

//-----------------------------------------------------------------------------------
// FIFO MEMORY MAP ON EXTERNAL CPU
//-----------------------------------------------------------------------------------
#ifdef FX2LP_ARGON_LV
    #define FX2LP_FIFO_BASE_ADDR  0xB4000000
#else // SCMA11 - default
    #define FX2LP_FIFO_BASE_ADDR  0xB6000000
#endif

#define FX2LP_FIFO2_ADDR          (FX2LP_FIFO_BASE_ADDR + 0x00000000)
#define FX2LP_CMDOUT_ADDR         (FX2LP_FIFO_BASE_ADDR + 0x00008000)
#define FX2LP_FIFO6_ADDR          (FX2LP_FIFO_BASE_ADDR + 0x00010000)
#define FX2LP_CMDIN_ADDR          (FX2LP_FIFO_BASE_ADDR + 0x00018000)
#define FX2LP_INT_ADDR            (FX2LP_FIFO_BASE_ADDR + 0x00020000)
#define FX2LP_CMDDONE_ADDR        (FX2LP_FIFO_BASE_ADDR + 0x00020000)

//-----------------------------------------------------------------------------------
// INTERRUPT LINES
//-----------------------------------------------------------------------------------
#define FX2LP_INTR_FUNC           0x0 //TBD
#define FX2LP_INTR_DMAREQ         0x0 //TBD

//-----------------------------------------------------------------------------------
// FX2LP PLUSE COMMAND ID   
// 3.7.12 Command list
//-----------------------------------------------------------------------------------
#define FX2LP_PLUS_SETUP_PKT_CMD                   0x01
#define FX2LP_PLUS_OUT_PHASE_CMD                   0x02
#define FX2LP_PLUS_IN_PHASE_CMD                    0x03
#define FX2LP_PLUS_READ_REG_CMD                    0x04
#define FX2LP_PLUS_SETUP_CPLT_CMD                  0x06
#define FX2LP_PLUS_READ_REG_RESP_CMD               0x40
#define FX2LP_PLUS_WRITE_REG_WITH_RESP_CMD         0x05
#define FX2LP_PLUS_WRITE_REG_CMD                   0x40
#define FX2LP_PLUS_WRITE_REG_RESP_CMD              0x07
#define FX2LP_PLUS_WRITE_DESC_CMD                  0x08
#define FX2LP_PLUS_PROGRAM_EEPROM_CMD              0x09
#define FX2LP_PLUS_IN_SHORT_PKT_EP2_CMD            0x80
#define FX2LP_PLUS_IN_SHORT_PKT_EP6_CMD            0xC0
#define FX2LP_PLUS_IN_SHORT_PKT_ACK_CMD            0x0A
#define FX2LP_PLUS_OUT_SHORT_PKT_EP2_RCVD_CMD      0x80
#define FX2LP_PLUS_OUT_SHORT_PKT_EP6_RCVD_CMD      0xC0
#define FX2LP_PLUS_OUT_SHORT_PKT_ACK_CMD           0x0C
#define FX2LP_PLUS_OUT_DATA_EP1_CMD                0x0D
#define FX2LP_PLUS_EP1INBUF_AVAIL_CMD              0x0E
#define FX2LP_PLUS_IN_DATA_EP1_CMD                 0x0F
#define CMD_MASK                                   0xC0

#define FX2LP_PLUS_IN_PHASE_COMMAND(LEN) \
    (FX2LP_PLUS_IN_PHASE_CMD | (((u16)(LEN)) << 8))

    // Sent to start get EP2 OUT data; master checks PF flag prior to reading each packet
#define FX2LP_PLUS_OUT_DATACOMMIT_CMD              0x14
    // Sent once or twice for each packet depending on received packet length
#define FX2LP_PLUS_OUT_EP1DATA_CMD                 0x15 
    // Sent to start get EP4 OUT data sequence
#define FX2LP_PLUS_OUT_EP4DATA_CMD                 0x16  
    // Sent to terminate get EP4 OUT data sequence
#define FX2LP_PLUS_OUT_EP4DATADONE_CMD             0x17   
    // Sent to start EP8 IN data sequence
#define FX2LP_PLUS_IN_EP8DATA_CMD                  0x18    
    // Enables Pass Through of all Standard Requests. Command should be sent after 
    // receive READY interrupt
#define FX2LP_PLUS_IN_SETUPPASSTHROUGH_CMD         0x19    
    // Master controlled enumeration
#define FX2LP_PLUS_IN_ENUMERATE_CMD                0x1A 

    // These extended endpoints must be initialized using one of the commands EP1OUTEN, 
    // EP4OUTEN, EP8INEN. Each of these commands take two params; enable and mode. 
    // enable - enable or disable an endpoint
    // mode   - interrupt or bulk 
    // 
#define FX2LP_PLUS_IN_EP1OUTEN_CMD                 0x1B
#define FX2LP_PLUS_IN_EP4OUTEN_CMD                 0x1C
#define FX2LP_PLUS_IN_EP8INEN_CMD                  0x1D
#define FX2LP_PLUS_IN_CLEAR_DATA_TOGGLE_AND_STALL_CMD    0x1E
#define FX2LP_PLUS_IN_WAKEUP_USB_ACTIVITY_CMD      0x1F   // need to add TBD
#define FX2LP_PLUS_IN_OUT_SHORT_PKT_MSG_ACK_CMD    0x20
#define FX2LP_PLUS_IN_SET_STALL_CMD                0x21
#define FX2LP_PLUS_IN_OUT_PKTCOUNT_NOTIFY_CMD      0x22
#define FX2LP_PLUS_IN_OUT_PKTCOUNT_CMD             0x23
#define FX2LP_PLUS_IN_BYPASS_OUT_SHORT_PKT_ACK_CMD 0x24
#define FX2LP_PLUS_IN_USBRESET_ACK_CMD             0x25

//-----------------------------------------------------------------------------------
//*** INTENABLE - Interrupt Enable Register
//-----------------------------------------------------------------------------------
// INTMODE ? Interrupt Mode (8 or 16-bit status)
// 0: Use Standard Flag definitions for each bit in lower status byte.  Upper byte not used.
// 1: Use Extended Flag definitions for INTCODE[2:0] in upper status byte.  Use PL[9:0] in 
//    upper and  lower bytes.
//
// INTCODE[2:0] ? Interrupt Code
// 000 ? EP1OUTPKTAVAIL: indicates EP1OUT packet is available and length
// 001 ? EP1OUTRDY: indicates EP1OUT data ready at [001]
// 010 ? EP4NAK: indicates host is attempting to sent OUT packet
// 011 ? EP4PKTAVAIL: indicates EP4 OUT packet is available and length
// 100 ? EP8INRDY: indicates that EP8 is ready for an IN packet
// 101 ? EP0AVAIL: indicates that SETUP or EP0 data is available and length
// 110 - OUTNE - indicates EP2 OUT or EP6 OUT is not-empty. The endpoint is indicated in EP[3:0]
// 111 - INNF - indicates EP2 IN or EP6 IN is not-full. The endpoint is indicated in EP[3:0]
//
// New Standard Interrupt 
// CMDSTATRDY: indicates that read data is available in the command status buffer (
// same indication as FLAGC at [001]). An enable bit is also added to INTENABLE register 
// to allow this interrupt to be turned off.  It is default off for backward compatibility.
//
// Interrupt Mode - 0: Standard; 1: Extended
//-----------------------------------------------------------------------------------

#define INT_B_INTMODE            15
#define INT_M_INTMODE            (1 << INT_B_INTMODE)

#ifdef FX2LP_FIRMWARE_VER_16
//*** Extended interrupts : INCODE[2:0]
#define INT_B_EXTND_MASK         0x1C00
#define INT_B_PKTLEN             0x03FF

#define INT_B_EXNTD              10
#define INT_M_EP1OUTPKTAVAIL     (0x0 << INT_B_EXNTD)
#define INT_M_EP1OUTRDY          (0x1 << INT_B_EXNTD)
#define INT_M_EP4ACK             (0x2 << INT_B_EXNTD)
#define INT_M_EP4PKTAVAIL        (0x3 << INT_B_EXNTD)
#define INT_M_EP8INRDY           (0x4 << INT_B_EXNTD)
#define INT_M_CMDRDY             (0x5 << INT_B_EXNTD) 
#define INT_M_OUTNE              (0x6 << INT_B_EXNTD)  // OUT buffer changed from nonEmpty to empty
#define INT_M_INNF               (0x7 << INT_B_EXNTD)  // IN buffer changed from nonEmpty to empty
#else  //////////////////////////////////////////////////////////
//*** Extended interrupts : INCODE[3:0]
#define INT_B_EXTND_MASK         0x3C00
#define INT_B_PKTLEN             0x03FF

#define INT_B_EXNTD              10

#define INT_M_EP1OUTPKTAVAIL     (0x0 << INT_B_EXNTD)
#define INT_M_EP2SHORTPKTAVAIL   (0x1 << INT_B_EXNTD)
#define INT_M_EP6SHORTPKTAVAIL   (0x2 << INT_B_EXNTD)
#define INT_M_EP4PKTAVAIL        (0x3 << INT_B_EXNTD)
#define INT_M_EP8INRDY           (0x4 << INT_B_EXNTD)
#define INT_M_CMDRDY             (0x5 << INT_B_EXNTD) 
#define INT_M_OUTNE              (0x6 << INT_B_EXNTD)  // OUT buffer changed from nonEmpty to empty
#define INT_M_INNF               (0x7 << INT_B_EXNTD)  // IN buffer changed from nonEmpty to empty
#define INT_M_WRITEREGRESP       (0x8 << INT_B_EXNTD) 
#define INT_M_EP1INBUFAVAIL      (0x9 << INT_B_EXNTD)
#define INT_M_INSHORTPKTACK      (0xA << INT_B_EXNTD)
#define INT_M_SUSPEND            (0xB << INT_B_EXNTD)
#define INT_M_RESUME             (0xC << INT_B_EXNTD)
#define INT_M_HS                 (0xE << INT_B_EXNTD)
#define INT_M_READREGRESP        (0x10 << INT_B_EXNTD)
#endif //////////////////////////////////////////////////////////

//*** Standard interrupts
#define INT_B_SETUP              7
#define INT_M_SETUP              (1 << INT_B_SETUP)
#define INT_B_EP0BUF             6
#define INT_M_EP0BUF             (1 << INT_B_EP0BUF)
#define INT_B_FLAGS              5
#define INT_M_FLAGS              (1 << INT_B_FLAGS)
#define INT_B_USBRESET           4 
#define INT_M_USBRESET           (1 << INT_B_USBRESET)
#define INT_B_CMDSTATRDY         3 
#define INT_M_CMDSTATRDY         (1 << INT_B_CMDSTATRDY)
#define INT_B_ENUMOK             2
#define INT_M_ENUMOK             (1 << INT_B_ENUMOK)
#define INT_B_BUSACTIVITY        1
#define INT_M_BUSACTIVITY        (1 << INT_B_BUSACTIVITY)
#define INT_B_READY              0
#define INT_M_READY              (1 << INT_B_READY)

//*** Extended interrupts 1
#define INT_B_EP2OUTNE           0
#define INT_M_EP2OUTNE_E         (1 << INT_B_EP2OUTNE)
#define INT_B_EP6OUTNE           1
#define INT_M_EP6OUTNE_E         (1 << INT_B_EP6OUTNE)
#define INT_B_EP2INNF            2
#define INT_M_EP2INNF_E          (1 << INT_B_EP2INNF)
#define INT_B_EP6INNF            3
#define INT_M_EP6INNF_E          (1 << INT_B_EP6INNF)
#define INT_B_HS_NOTIFY          4
#define INT_M_HS_NOTIFY          (1 << INT_B_HS_NOTIFY)

#define FX2LP_EP1IN_EOP_TOKEN   0x8000
#define FX2LP_EP1IN_EOP_CMD(L)  (FX2LP_EP1IN_EOP_TOKEN | (((u16)(L))<<8))

/* Defines for interrupt source register (after any necessary swap */
#define INT_M_EPNO              0xF     /* EP generating interrupt */


// Mapping between EP number and EP index
#define FX2LP_MAX_EP_NUMBER     6
extern const FX2LP_EPX fx2lp_epn_to_epx[FX2LP_MAX_EP_NUMBER + 1];
extern const EPX_INFO_T fx2lp_epx_info[FX2LP_NUM_EPS + 1];
#define epn_to_fx2lp_epx(N) \
    (((N) > FX2LP_MAX_EP_NUMBER) ? FX2LP_INVALID : fx2lp_epn_to_epx[N])

// Common function prototupes
extern PEP_STATUS fx2lp_get_ep_status (PFX2LP_PCD, FX2LP_EPX);

extern FX2LP_BOOL fx2lp_read_reg(PFX2LP_PCD  pfx2lp, u8 reg_addr, pu8 pdata);
extern FX2LP_BOOL fx2lp_disconnect_device (PFX2LP_PCD  pfx2lp);
extern FX2LP_BOOL fx2lp_pcd_init (void);
extern void init_fx2lp_epn(PFX2LP_PCD pfx2lp);
extern void fx2lp_read_setup_pkt_irq(PFX2LP_PCD);
extern void fx2lp_req_short_packet (PFX2LP_PCD pfx2lp, PEP_STATUS peps);
extern FX2LP_EPX  fx2lp_get_priority_based_epx(PFX2LP_PCD, u8 bm_attributes);
extern FX2LP_BOOL fx2lp_setup_dev (PFX2LP_PCD pfx2lp);
extern FX2LP_BOOL fx2lp_suspend_pcd (PFX2LP_PCD  pfx2lp);
extern FX2LP_BOOL fx2lp_enable_interrupts (PFX2LP_PCD pfx2lp);

/* ============== HAPI stuff ============== */
/* HAPI should have provided prototypes for these. I'm tired of warnings */
typedef irqreturn_t (hapi_intr_hdlr) (int, void *, struct pt_regs *);
extern int gpio_usb_hs_int_request_irq (hapi_intr_hdlr *, int, const char *, void *);
extern void gpio_usb_hs_wakeup_set_data(__u32);
extern void gpio_usb_hs_switch_set_data(__u32);
extern void gpio_usb_hs_int_free_irq (void *);
extern void gpio_usb_hs_reset_set_data(__u32 enable);

/* ============== End HAPI stuff ============== */

#define fx2lp_gen_intr(P,V) (*((P)->pintr_addr) = (V))
#define _fx2lp_write_reg_worsp_irq(P,ADDR,DATA) \
    (((*(P)->pcmd_in_addr) = ((ADDR) | FX2LP_PLUS_WRITE_REG_CMD \
        | ((u16)(DATA) << 8))),fx2lp_gen_intr(P,0))

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_REGISTERS)
#define fx2lp_write_reg_worsp_irq(P,ADDR,DATA) \
    (printk("\n%s: Write reg %x = %x", __FUNCTION__, ADDR, DATA), \
                _fx2lp_write_reg_worsp_irq(P,ADDR,DATA))
#else
#define fx2lp_write_reg_worsp_irq(P,ADDR,DATA) \
    _fx2lp_write_reg_worsp_irq(P,ADDR,DATA)
#endif

#endif /* !defined(__FX2LP_HARDWARE_H__) */



