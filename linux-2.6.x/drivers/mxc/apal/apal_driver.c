 /*
 * Copyright (C) 2004,2006-2008 Motorola, Inc.
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
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ---------------------------
 * 06/28/2004   Motorola  File creation 
 * 12/11/2006   Motorola  Fixed bug, audio loss after wakeup from DSM
 * 01/30/2007   Motorola  Reset Audio IC at driver unload time
 * 02/06/2007   Motorola  Add poll system call
 * 04/09/2007   Motorola  Delay closing of SSI
 * 04/25/2007   Motorola  Add protection against SDMA driver bd_done bug
 * 05/14/2007   Motorola  Support querying data left(not played) in apal driver from user space
 * 05/15/2007   Motorola  Mute Power IC when switching clocks
 * 05/29/2007   Motorola  Add hack to avoid noise when stopping SDMA during playback
 * 06/06/2007   Motorola  Support 0 buffer size write 
 * 02/28/2008   Motorola  Supporting FM radio
 */

/*!
 * @file apal_driver.c
 *
 * @brief Audio Driver 
 *
 * This file contains all of the functions and data structures required to provide control to the
 * Audio Manager in user space to configure hardware for audio playback/capture and for voice calls.
 * This driver interfaces with the power IC driver, digital audio mux driver, SSI driver and the 
 * SDMA driver to setup paths for audio data flow.
 */


/*==================================================================================================
                                           INCLUDE FILES
==================================================================================================*/

#if defined(CONFIG_MODVERSIONS)
#define MODVERSIONS
#include <linux/modversions.h>
#endif

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/smp_lock.h>
#include <linux/module.h>
#include <linux/devfs_fs_kernel.h>      /* dev fs interface */
#include <asm/arch/spba.h>              /* SPBA configuration for SSI2 */
#include <asm/arch/clock.h>
#include <asm/uaccess.h>
#include "sdma.h"
#include "dam.h"
#include "ssi.h"
#include "registers.h"
#include "apal_driver.h"
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/power_ic_kernel.h>
#include <linux/poll.h>

/*==================================================================================================
                                             CONSTANTS
==================================================================================================*/
#define APAL_SUCCESS  0
#define APAL_FAILURE -1

/* DMA related macros */
#define TX_FIFO_EMPTY_WML  4
#define RX_FIFO_FULL_WML   4

#define APAL_SDMA_TIMEOUT  (HZ)  // HZ - 1sec, SDMA timeout - 1000msec

/*==================================================================================================
                                               MACROS
==================================================================================================*/

#ifdef APAL_DEBUG
#define APAL_DEBUG_LOG(args...)  printk (KERN_DEBUG "APAL_DRIVER:" args)
#else
#define APAL_DEBUG_LOG(args...)  
#endif

#define APAL_ERROR_LOG(args...)  printk (KERN_ERR "APAL_DRIVER: Error " args)

#define CATCH _err

#define TRY(a)  if((a) != 0)  goto CATCH;

/* To determine if this board is Pass1 or Pass2 */
#define SCMA11_PASS_1                (system_rev <  CHIP_REV_2_0)
#define SCMA11_PASS_2_OR_GREATER     (system_rev >= CHIP_REV_2_0)

#define APAL_NB_BLOCK_SDMA 10 
#define APAL_NB_WAIT_BLOCKS 4 

#define APAL_SDMA_8KMONO_SIZE       800

#define APAL_SWITCH_CLOCK_TO_13MHZ 0
#define APAL_SWITCH_CLOCK_TO_26MHZ 1

/*==================================================================================================
                                        FUNCTION PROTOTYPES
==================================================================================================*/
/* @TODO: Remove these once DAI related header file is available */
extern void gpio_dai_enable(void);
extern void gpio_dai_disable(void);
/* END TODO */

static int apal_open (struct inode *, struct file *);
static int apal_release (struct inode *, struct file *);
static int apal_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t apal_write (struct file *fp, const char* buf, size_t bytes, loff_t *nouse);
static ssize_t apal_read  (struct file *fp, char *buf, size_t bytes, loff_t *nouse);
static unsigned int apal_poll (struct file *file, poll_table *wait);
static int apal_suspend (struct device *dev, u32 state, u32 level);
static int apal_resume (struct device *dev, u32 level);
static int apal_probe (struct device *dev);

/*==================================================================================================
                                     STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
static struct file_operations apal_fops = {
    .owner   = THIS_MODULE,
    .open    = apal_open,
    .release = apal_release,   
    .ioctl   = apal_ioctl,
    .write   = apal_write,
    .read    = apal_read,
    .poll    = apal_poll,
};

static struct device_driver apal_device_driver = {
    .name    = "apal",
    .bus     = &platform_bus_type,
    .probe   = apal_probe,
    .suspend = apal_suspend,
    .resume  = apal_resume,
};

static struct platform_device apal_platform_device = {
    .name    = "apal",
    .id      = 0,
};

static struct 
{
    int major_num;
    BOOL port_4_in_use;           /* Port_4 is shared between CODEC and BT on the common bus */    
    POWER_IC_ST_DAC_SR_T stdac_sample_rate;
    POWER_IC_CODEC_SR_T codec_sample_rate;
    ssi_mod mod;
    BOOL stdac_in_use;
    BOOL codec_in_use;
    BOOL phone_call_on;
    BOOL prepare_for_dsm;
    BOOL fmradio_on;
}apal_driver_state;

static struct
{  
    APAL_BUFFER_STRUCT_T myTxBufStructSDMA[APAL_NB_BLOCK_SDMA];
    int write_channel;
    int next_buffer_to_fill;
    int buffer_read;
    int count;
    BOOL first_write;  
    BOOL wait_next;
    BOOL start_dma;
    BOOL wait_end;
    BOOL immediate_stop;
    BOOL pause;
    BOOL apal_tx_buf_flag;
}apal_tx_dma_config[2];

static struct
{  
    APAL_BUFFER_STRUCT_T myRxBufStructSDMA[APAL_NB_BLOCK_SDMA];
    int read_channel;
    int next_buffer_to_fill;
    int buffer_read;
    int count;
    BOOL first_read;  
    BOOL wait_next;
    BOOL wait_end;
    BOOL immediate_stop;
    BOOL apal_rx_buf_flag;
}apal_rx_dma_config;

typedef enum
{
    APAL_CODEC = 0,
    APAL_STDAC = 1
}APAL_STDAC_CODEC_ENUM_T;

typedef enum
{
    APAL_ATLAS_MASTER = 0,
    APAL_ATLAS_SLAVE  = 1
}APAL_MASTER_SLAVE_ENUM_T;

#ifdef APAL_DEBUG
/* Atlas configuration registers */
static struct 
{
    unsigned int audio_rx0;     /* AUDIO_RX_0 */
    unsigned int audio_rx1;     /* AUDIO_RX_1 */
    unsigned int audio_tx;       /* AUDIO_TX */
    unsigned int audio_ssi;      /* AUDIO_SSI */
    unsigned int audio_codec;    /* AUDIO_CODEC */
    unsigned int audio_stdac;    /* AUDIO_STDAC */
}atlas_config;
#endif

/*==================================================================================================
                                          LOCAL VARIABLES
==================================================================================================*/
/* Tx Wait queue */
static wait_queue_head_t apal_tx_wait_queue;
static spinlock_t apal_write_lock = SPIN_LOCK_UNLOCKED;
static int flags = 0;

/* Rx wait queue */
static wait_queue_head_t apal_rx_wait_queue;
static spinlock_t apal_read_lock = SPIN_LOCK_UNLOCKED;

/* DSM wait queue */
static wait_queue_head_t apal_dsm_wait_queue;

/* common local variables */
static int open_count = 0; 
static APAL_STDAC_CODEC_ENUM_T stdac_codec_in_use;

/* flag for control of the vaudio on/off */
static BOOL vaudio_on = FALSE;

/* current playback size per buffer */
static unsigned int current_size = 0;

static dma_channel_params tx_params, rx_params;

/*==================================================================================================
                                          GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                             LOCAL HARDWARE DEPENDENT FUNCTIONS 
==================================================================================================*/
/*=================================================================================================
FUNCTION: apal_audioic_power_off()

DESCRIPTION: 
    This function zeros out all Audio IC registers

ARGUMENTS PASSED:
    None

RETURN VALUE:
    APAL_SUCCESS - On Success
    APAL_FAILURE - On Failure

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
int
apal_audioic_power_off
(
    void
)
{
    unsigned int mask = 0xFFFFFF;
    unsigned int value = 0x0;
 
    TRY ( power_ic_audio_set_reg_mask_audio_rx_0 (mask, value | AUDIOIC_SET_HS_DET_EN) )
    TRY ( power_ic_audio_set_reg_mask_audio_rx_1 (mask, value) )
    TRY ( power_ic_audio_set_reg_mask_audio_tx   (mask, value) )
    TRY ( power_ic_audio_set_reg_mask_ssi_network(mask, value) )
    TRY ( power_ic_audio_set_reg_mask_audio_codec(mask, value) )
    TRY ( power_ic_audio_set_reg_mask_audio_stereo_dac (mask, value) )

    ssi_enable(SSI1, false);
    ssi_enable(SSI2, false);

    vaudio_on = FALSE;

    return APAL_SUCCESS;

CATCH:
    APAL_ERROR_LOG ("Operation failed inside %s\n", __FUNCTION__);
    return APAL_FAILURE;
}

/*=================================================================================================
FUNCTION: apal_dma_tx_interrupt_handler()

DESCRIPTION: 
    This function is the interrupt handler when writing data to SDMA. This is registered as a call
    back function with the SDMA.

ARGUMENTS PASSED:
    None

RETURN VALUE:
    None

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
void
apal_dma_tx_interrupt_handler
(
    void* arg
)
{
    dma_request_t req;
    BOOL buffer_read_incremented = FALSE;
/* print bd_done bits for debug purposes only when deemed necessary. */
#if 0
    printk ("tx_interrupt_handler called\n");

    {
        int i = 0, bd_done[APAL_NB_BLOCK_SDMA]; 
        int bd_done_all = 0;
        dma_request_t req1;
    
        for (i = 0; i < APAL_NB_BLOCK_SDMA; ++i) 
        {
            mxc_dma_get_config(apal_tx_dma_config[stdac_codec_in_use].write_channel, &req1, i);
            bd_done[i] = req1.bd_done;
            bd_done_all |= ((1 & req1.bd_done) << i);
            printk ("bd_done[%d] = %d, bd_done_all = %d\n", i, bd_done[i], bd_done_all);
        }
    }
#endif

    while (apal_tx_dma_config[stdac_codec_in_use].buffer_read
           != apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill)
    {
        mxc_dma_get_config(apal_tx_dma_config[stdac_codec_in_use].write_channel,
                           &req, apal_tx_dma_config[stdac_codec_in_use].buffer_read);

        APAL_DEBUG_LOG ("Itp:buf_read = %d, req.bd_done = %d, next_buf_to_fill = %d\n", 
                        apal_tx_dma_config[stdac_codec_in_use].buffer_read, req.bd_done, 
                        apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill);

        /* bd_done = 0 means that SDMA is done with that buffer, 
           bd_done = 1 means that SDMA is still working on that buffer */
        if (buffer_read_incremented && (req.bd_done == 1))
        {
            break;
        }

        if (req.bd_done == 0)
        {
            apal_tx_dma_config[stdac_codec_in_use].buffer_read =
                          (apal_tx_dma_config[stdac_codec_in_use].buffer_read+1)%APAL_NB_BLOCK_SDMA;
            buffer_read_incremented = TRUE;
        }
        else if ((req.bd_done == 1) && (!buffer_read_incremented))
        {
            apal_tx_dma_config[stdac_codec_in_use].buffer_read =
                          (apal_tx_dma_config[stdac_codec_in_use].buffer_read+1)%APAL_NB_BLOCK_SDMA;
        }
    }

    if((apal_tx_dma_config[stdac_codec_in_use].immediate_stop == TRUE) || 
       (apal_tx_dma_config[stdac_codec_in_use].buffer_read == 
        apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill))
    { 
        if(apal_tx_dma_config[stdac_codec_in_use].wait_end == TRUE)
        {
            /* Must stop SDMA first then stop SSI, otherwise there is 'pa' noise at the end */
#ifdef CONFIG_MOT_WFN409
            mxc_dma_reset (apal_tx_dma_config[stdac_codec_in_use].write_channel,APAL_NB_BLOCK_SDMA);
#endif /* CONFIG_MOT_WFN409 */
            mxc_dma_stop (apal_tx_dma_config[stdac_codec_in_use].write_channel);

            ssi_tx_flush_fifo (apal_driver_state.mod);

            /* reset static variables */
            apal_tx_dma_config[stdac_codec_in_use].wait_end = FALSE;
            apal_tx_dma_config[stdac_codec_in_use].first_write = FALSE;
            apal_tx_dma_config[stdac_codec_in_use].wait_next = FALSE;
            apal_tx_dma_config[stdac_codec_in_use].start_dma = FALSE;

            wake_up_interruptible (&apal_tx_wait_queue);
        } 
        else
        {
            APAL_ERROR_LOG("missed %s isr!\n",__FUNCTION__);
        }
    }
    else if(apal_tx_dma_config[stdac_codec_in_use].wait_next == TRUE)
    {
        wake_up_interruptible (&apal_tx_wait_queue);
        apal_tx_dma_config[stdac_codec_in_use].wait_next = FALSE;
    }

    return;
}

/*=================================================================================================
FUNCTION: apal_dma_rx_interrupt_handler()

DESCRIPTION: 
    This function is the interrupt handler when reading data from SDMA

ARGUMENTS PASSED:
    None

RETURN VALUE:
    None

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
void
apal_dma_rx_interrupt_handler
(
    void *arg
)
{
    apal_rx_dma_config.next_buffer_to_fill = 
                                      (apal_rx_dma_config.next_buffer_to_fill+1)%APAL_NB_BLOCK_SDMA;

    if((apal_rx_dma_config.immediate_stop == TRUE) || 
            (apal_rx_dma_config.buffer_read == apal_rx_dma_config.next_buffer_to_fill))
    {
        if(apal_rx_dma_config.wait_end == TRUE)
        {
            /* stop SDMA capture */
            ssi_receive_enable (apal_driver_state.mod, false);
            ssi_rx_fifo_enable(apal_driver_state.mod, ssi_fifo_0, false);
#ifdef CONFIG_MOT_WFN409
            mxc_dma_reset (apal_rx_dma_config.read_channel, APAL_NB_BLOCK_SDMA);
#endif /* CONFIG_MOT_WFN409 */
            mxc_dma_stop (apal_rx_dma_config.read_channel);

            /* reset static variables */
            apal_rx_dma_config.wait_end = FALSE;
            apal_rx_dma_config.first_read = FALSE;
            apal_rx_dma_config.wait_next = FALSE;
            apal_rx_dma_config.immediate_stop = FALSE;

            wake_up_interruptible (&apal_rx_wait_queue);
        }
        else
        {
            APAL_ERROR_LOG("missed %s isr!\n",__FUNCTION__);
        }
    }
    else if(apal_rx_dma_config.wait_next == TRUE)
    {
        apal_rx_dma_config.wait_next = FALSE;
        wake_up_interruptible (&apal_rx_wait_queue);
    }

    return;
}

/*=================================================================================================
FUNCTION: apal_setup_dma()

DESCRIPTION: 
    This function sets up DMA

ARGUMENTS PASSED:
    None

RETURN VALUE:
    None

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
int
apal_setup_dma
(
    APAL_STDAC_CODEC_ENUM_T stdac_codec
)
{
    int dma_channel = 0;

    if (stdac_codec == APAL_CODEC)
    {
        /* Allocate DMA Rx channel for SSI1 */
        TRY ( mxc_request_dma (&dma_channel, "APAL_CODEC SDMA") )
        apal_tx_dma_config[stdac_codec].write_channel = dma_channel;

        /* fifo 0, SSI1 */
        tx_params.per_address     = (SSI1_BASE_ADDR + 0); /*STX10*/
        tx_params.event_id        = DMA_REQ_SSI1_TX1;
        tx_params.peripheral_type = SSI;

        APAL_DEBUG_LOG("APAL_CODEC SDMA channel number %d\n", dma_channel);
    }
    else
    {
        /* Allocate DMA Tx channel for SSI2 */
        TRY ( mxc_request_dma (&dma_channel, "APAL_STDAC SDMA") )
        apal_tx_dma_config[stdac_codec].write_channel = dma_channel;

        /* fifo 0, SSI2 */
        tx_params.per_address     = (SSI2_BASE_ADDR + 0); /*STX20*/
        tx_params.event_id        = DMA_REQ_SSI2_TX1;
        tx_params.peripheral_type = SSI_SP;

        APAL_DEBUG_LOG("APAL_STDAC SDMA channel number %d\n", dma_channel);
    }

    tx_params.watermark_level = TX_FIFO_EMPTY_WML;
    tx_params.transfer_type   = emi_2_per;
    tx_params.callback        = apal_dma_tx_interrupt_handler;
    tx_params.arg             = (void*)&apal_tx_dma_config[stdac_codec];
    tx_params.bd_number       = APAL_NB_BLOCK_SDMA;
    tx_params.word_size       = TRANSFER_16BIT;

    /* Set the SDMA configuration for transmit */
    TRY ( mxc_dma_setup_channel(dma_channel, &tx_params) )

    /* If we are using the Codec then enable DMA for receive also */
    if (stdac_codec == APAL_CODEC)
    {
        /* Allocate DMA Rx channel only thru SSI1 */ 
        dma_channel = 0; /* request a new DMA channel for RX */
        TRY ( mxc_request_dma (&dma_channel, "APAL_RX SDMA") )
        apal_rx_dma_config.read_channel = dma_channel;

        rx_params.watermark_level = RX_FIFO_FULL_WML;
        rx_params.transfer_type   = per_2_emi;
        rx_params.callback        = apal_dma_rx_interrupt_handler;
        rx_params.arg             = (void*)&apal_rx_dma_config;
        rx_params.bd_number       = APAL_NB_BLOCK_SDMA;
        rx_params.word_size       = TRANSFER_16BIT;

        /* fifo 0, SSI1 */
        rx_params.per_address     = (SSI1_BASE_ADDR + 8); /*SRX10*/
        rx_params.event_id        = DMA_REQ_SSI1_RX1;
        rx_params.peripheral_type = SSI;

        /* Set the SDMA configuration for transmit */
        TRY ( mxc_dma_setup_channel(dma_channel, &rx_params) )

        APAL_DEBUG_LOG ("APAL_RX SDMA channel number %d\n", dma_channel);
    }

    return APAL_SUCCESS;

CATCH:
    APAL_ERROR_LOG ("Operation failed inside %s\n", __FUNCTION__);
    return APAL_FAILURE;
}

/*=================================================================================================
FUNCTION: apal_setup_dam()

DESCRIPTION: 
    This function sets up source and destination port on the AUDMUX

ARGUMENTS PASSED:
    port_a  - Internal AUDMUX port i.e. the ones connected to AP or BP
    port_b  - External AUDMUX port i.e. the ones connected to peripherals
    master_slave - Tell whether CODEC/STDAC is the master of the clock or not

RETURN VALUE:
    Always SUCCESS

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    Internal ports are port_1, port_2 & port_3. The first two connected to AP and the third to the
    BP.

==================================================================================================*/
int 
apal_setup_dam 
(
    dam_port port_a, 
    dam_port port_b,
    APAL_MASTER_SLAVE_ENUM_T master_slave
)
{
    APAL_DEBUG_LOG ("%s() called with port_a = %d port_b = %d master_slave = %d\n", __FUNCTION__, \
                    port_a, port_b, master_slave);

    dam_reset_register(port_a);
    dam_reset_register(port_b);

    dam_set_synchronous(port_a, true); /* Rx & Tx will use same clock, 4-wire mode*/
    dam_set_synchronous(port_b, true);

    if (port_a != port_7)
    {
        dam_select_RxD_source(port_a, port_b);   /* a<-b */
        dam_select_RxD_source(port_b, port_a);   /* a->b */
    }
    /* DAI needs to be treated differently */
    /* Port 3 (SAP) sends/receives data to/from CE Port7 (DAI box - TS1) and Port 4 (Atlas - TS0) */
    else 
    {
        dam_select_RxD_source(port_a, port_3);   /* 7<-3 */
        dam_select_RxD_source(port_4, port_3);   /* 4<-3 */
        dam_select_mode (port_3, CE_bus_network_mode);     /* put Port_3 in CE bus network mode */
    }

    if (master_slave == APAL_ATLAS_MASTER)
    {
        dam_select_TxClk_direction(port_a, signal_out); /* Internal ports don't supply clock */
        dam_select_TxFS_direction(port_a, signal_out);   
        dam_select_TxClk_source(port_a, false, port_b); /* port_b is the source of the Tx clk */
        dam_select_TxFS_source(port_a, false, port_b);  /* port_b is the source of the Tx FS */
        APAL_DEBUG_LOG ("port_b CLK, and FS setted\n");
    }
    else /* In Slave mode SSI is the clock master */
    {
        dam_select_TxClk_source(port_b, false, port_a);
        dam_select_TxFS_source(port_b, false, port_a);
        dam_select_TxClk_direction(port_b, signal_out); 
        dam_select_TxFS_direction(port_b, signal_out); 
    }
   
    return APAL_SUCCESS;
}

/*=================================================================================================
FUNCTION: apal_setup_codec()

DESCRIPTION: 
    This function sets up the codec for voice call and Mono audio using Atlas

ARGUMENTS PASSED:
    master_slave - CODEC is master of the clock or not

RETURN VALUE:
    APAL_SUCCESS - If CODEC was setup or reset successfully
    APAL_FAILURE - If CODEC could not be setup or reset successfully

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
int 
apal_setup_codec 
(
    APAL_MASTER_SLAVE_ENUM_T master_slave,
    APAL_AUDIO_ROUTE_ENUM_T  audio_route
)
{
    unsigned int mask = 0, value = 0;

    APAL_DEBUG_LOG ("%s() called with audio_route %d\n", __FUNCTION__, audio_route);

    mask |= AUDIOIC_SET_CDC_CLK2 | AUDIOIC_SET_CDC_CLK1 | AUDIOIC_SET_CDC_CLK0;

    /* If phone call is on then we need to use the corrected clock from BP which is 13Mhz */
    if (audio_route == APAL_AUDIO_ROUTE_PHONE) 
    { 
        /* value |= 0x0; , 000 = 13Mhz */
    }
    else /* For MM cases */
    {
        value |= AUDIOIC_SET_CDC_CLK2;  /* 100 = 26Mhz */
    }

    mask |= AUDIOIC_SET_CDC_SM;

    if (master_slave == APAL_ATLAS_MASTER)
    {
        /* Codec in Master mode */
    }
    else
    {
        /* Codec in Slave mode */ 
        value |= AUDIOIC_SET_CDC_SM;
    }

    mask |= AUDIOIC_SET_CDC_FS0 | AUDIOIC_SET_CDC_FS1;
    value |= AUDIOIC_SET_CDC_FS0;
      
    TRY ( power_ic_audio_set_reg_mask_audio_codec (mask, value))

#ifdef APAL_DEBUG
    TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_CODEC, &atlas_config.audio_codec) )
    TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_STEREO_DAC,&atlas_config.audio_stdac))
    TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_RX_1, &atlas_config.audio_rx1) )
    TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_RX_0, &atlas_config.audio_rx0) )
    TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_TX, &atlas_config.audio_tx) )
    TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_SSI_NETWORK, &atlas_config.audio_ssi) )

    APAL_DEBUG_LOG ("REG_CODEC = %#x\n REG_STDAC = %#x\n REG_RX_1 = %#x\n REG_RX_0 = %#x\n \
                     REG_TX = %#x\n REG_SSI_NETWORK = %#x\n", atlas_config.audio_codec, \
                     atlas_config.audio_stdac, atlas_config.audio_rx1, atlas_config.audio_rx0, \
                     atlas_config.audio_tx, atlas_config.audio_ssi);
#endif

    return APAL_SUCCESS;

CATCH:
    APAL_ERROR_LOG ("Operation failed inside %s\n", __FUNCTION__);
    return APAL_FAILURE;
}

/*=================================================================================================
FUNCTION: apal_setup_stdac()

DESCRIPTION: 
    This function sets up the STDAC

ARGUMENTS PASSED:
    master_slave - STDAC is the master of the clock or not

RETURN VALUE:
    APAL_SUCCESS - If STDAC was setup or reset successfully
    APAL_FAILURE - If STDAC could not be setup or reset successfully

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
int 
apal_setup_stdac
(
    APAL_MASTER_SLAVE_ENUM_T master_slave
)
{
    unsigned int mask = 0, value = 0;
    APAL_DEBUG_LOG ("%s() called\n", __FUNCTION__);

    mask |= AUDIOIC_SET_STDAC_CLK2 | AUDIOIC_SET_STDAC_CLK1 | AUDIOIC_SET_STDAC_CLK0;

    if (apal_driver_state.phone_call_on == TRUE)
    {
        APAL_DEBUG_LOG ("Phone call on so using 13Mhz clock for Stdac\n");
        /* value |= 0x0, 000 = 13Mhz */
    }
    else
    {
        APAL_DEBUG_LOG ("Using 26 Mhz clock for Stdac\n");
        value |= AUDIOIC_SET_STDAC_CLK2;
    }
 
    /* Choose SSI2 */
    mask |= AUDIOIC_SET_STDAC_SSI_SEL | AUDIOIC_SET_STDAC_FS0 | AUDIOIC_SET_STDAC_FS1;
    value |= AUDIOIC_SET_STDAC_SSI_SEL | AUDIOIC_SET_STDAC_FS0;

    mask |= AUDIOIC_SET_STDAC_SM;

    if (master_slave == APAL_ATLAS_MASTER)
    {
        /* STDAC in Master mode */
    }
    else
    {
        value |= AUDIOIC_SET_STDAC_SM;    /* STDAC in Slave mode */
    }

    TRY ( power_ic_audio_set_reg_mask_audio_stereo_dac (mask, value))

    /* STDCCLOTS[1:0] = 10, 4 time slots */
    mask = AUDIOIC_SET_ST_DAC_SLT_1 | AUDIOIC_SET_ST_DAC_SLT_0;
    value = AUDIOIC_SET_ST_DAC_SLT_1; /* 4 time slots */
 
    TRY ( power_ic_audio_set_reg_mask_ssi_network (mask, value) )  

    return APAL_SUCCESS;

CATCH:
    APAL_ERROR_LOG ("Operation failed inside %s\n", __FUNCTION__);
    return APAL_FAILURE;
}

/*=================================================================================================
FUNCTION: apal_setup_ssi()

DESCRIPTION: 
    This function sets up the SSI port

ARGUMENTS PASSED:
    stdac_codec - Whether to setup SSI for STDAC or CODEC
    master_codec - Whether CODEC/STDAC is the master of the clock or not

RETURN VALUE:
    Always APAL_SUCCESS

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
int
apal_setup_ssi
(
    APAL_STDAC_CODEC_ENUM_T stdac_codec,
    APAL_MASTER_SLAVE_ENUM_T master_slave
)
{
    unsigned char prescaler = 0;
    unsigned long ssi1_clock, ssi2_clock;
    ssi_mod mod;

    APAL_DEBUG_LOG ("%s() called with stdac_codec = %d\n", __FUNCTION__, stdac_codec);

    /*
     * SPBA configuration for SSI2 - SDMA and MCU are set 
     */
    spba_take_ownership(SPBA_SSI2, SPBA_MASTER_C|SPBA_MASTER_A);

    if (stdac_codec == APAL_STDAC)
    {
        apal_driver_state.mod = SSI2;
        mod = SSI2;

        /* enable IPG clock */
        mxc_clks_enable(SSI2_IPG_CLK);  

        /* NOTE: 8000 should be replace by the actual stdac sampling frequency being used. 8000 is 
                 just temporary. We don't use prescaler anyways because Atlas is always master */
        if (master_slave == APAL_ATLAS_SLAVE)
        {
            /* assign USBPLL to be used by SSI2 */
            mxc_set_clocks_pll(SSI2_BAUD, USBPLL);
    
            /*set divider to 2*/
            mxc_set_clocks_div(SSI2_BAUD, 2);

           /* enable clock*/
            mxc_clks_enable(SSI2_BAUD);

            ssi2_clock = mxc_get_clocks (SSI2_BAUD)/2;

            prescaler = (unsigned char)(ssi2_clock/ 
                                       (2 * 8000 *
                                        16 *  /* bits per sample */
                                        2));  /* number of channels */
        }

        /* disable ssi, first apal_read/apal_write will enable ssi and sdma */
        ssi_two_channel_mode(mod, false);
        /* Network mode config, enable Timeslot 0,1 */
        ssi_network_mode(mod, true);
        ssi_tx_fifo_enable(mod, ssi_fifo_1, false);
    }
    else /* CODEC */
    {
        apal_driver_state.mod = SSI1;
        mod = SSI1;

        /* enable IPG clock */
        mxc_clks_enable(SSI1_IPG_CLK);

        if (master_slave == APAL_ATLAS_SLAVE)
        {
            /* assign USBPLL to be used by SSI1 */
            mxc_set_clocks_pll(SSI1_BAUD, USBPLL);
	
            /*set divider to 2*/
            mxc_set_clocks_div(SSI1_BAUD, 2);

           /* enable clock*/
            mxc_clks_enable(SSI1_BAUD);

            ssi1_clock = mxc_get_clocks (SSI1_BAUD)/2;

            /* NOTE: 8000 should be replace by the actual codec sampling frequency being used. 8000
               is just temporary. We don't use prescaler anyways because Atlas is always master */
            prescaler = (unsigned char)(ssi1_clock/ 
                                       (2 * 8000 *
                                        16 *
                                        1)); 
        }

        /* disable ssi, first apal_read/apal_write will enable ssi and sdma */
        ssi_two_channel_mode(mod, false);
        ssi_network_mode(mod, false);
        ssi_tx_fifo_enable(mod, ssi_fifo_1, false);
    }
    
    /****** SCR (SSI Control Register) ******/
    /* Reset SSI */
    ssi_enable(mod, false);
    /* Start by disabling all interrupts */
    ssi_interrupt_disable(mod, 0xFFFFFFFF);
    /* Disable System clock */
    ssi_system_clock(mod, false);
    ssi_clock_idle_state(mod, true);
    /* configure other registers */
    ssi_i2s_mode(mod, i2s_normal);
    ssi_synchronous_mode(mod, true);
 
    /****** STCR/SRCR (SSI Transmit/Receive Configuration Register) *******/
    /* Configure the SSI module to transmit data word from bit position 0 or 23 in the Transmit 
       shift register */
    ssi_tx_bit0(mod, true);
    ssi_rx_bit0(mod, true);     
    
    /* SSI is the master of the clock in Slave mode */
    if (master_slave == APAL_ATLAS_SLAVE)
    {
        ssi_tx_frame_direction(mod, ssi_tx_rx_internally);
        ssi_tx_clock_direction(mod, ssi_tx_rx_internally); 
        ssi_rx_frame_direction(mod, ssi_tx_rx_internally);
        ssi_rx_clock_direction(mod, ssi_tx_rx_internally);
    }
    else /* In Master mode STDAC/CODEC is the master of the clock */
    {
        ssi_tx_frame_direction(mod, ssi_tx_rx_externally);
        ssi_tx_clock_direction(mod, ssi_tx_rx_externally);
        ssi_rx_frame_direction(mod, ssi_tx_rx_externally);
        ssi_rx_clock_direction(mod, ssi_tx_rx_externally);
    }
    
    /** STCR - Transmit configuration **/
    ssi_tx_early_frame_sync(mod, ssi_frame_sync_one_bit_before);

    ssi_tx_frame_sync_length(mod, ssi_frame_sync_one_bit);


    ssi_tx_frame_sync_active(mod, ssi_frame_sync_active_high);
    ssi_tx_clock_polarity(mod, ssi_clock_on_rising_edge);
    ssi_tx_shift_direction(mod, ssi_msb_first);

    /** SRCR - Receive configuration **/
    ssi_rx_early_frame_sync(mod, ssi_frame_sync_one_bit_before);
    ssi_rx_frame_sync_length(mod, ssi_frame_sync_one_bit);


    ssi_rx_frame_sync_active(mod, ssi_frame_sync_active_high);
    ssi_rx_clock_polarity(mod, ssi_clock_on_rising_edge);
    ssi_rx_shift_direction(mod, ssi_msb_first);
    
    /****** STCCR (SSI Transmit Clock Control Register) ******/
    ssi_tx_word_length(mod, ssi_16_bits);           /* WL */
    ssi_tx_clock_divide_by_two(mod, false);         /* DIV2 */
    /* In Normal mode, this is word transfer rate. In Network mode, its num of words per frame*/
    ssi_tx_frame_rate(mod, (unsigned char) 4);      /* DC */
    ssi_tx_prescaler_modulus(mod, (unsigned char ) prescaler);/* PM */
    ssi_tx_clock_prescaler(mod, false);             /* PSR */
    
    /****** SRCCR (SSI Receive Clock Control Register) ******/
    ssi_rx_word_length(mod, ssi_16_bits);
    ssi_rx_clock_divide_by_two(mod, false); 
    ssi_rx_frame_rate(mod, (unsigned char) 4);
    ssi_rx_prescaler_modulus(mod, (unsigned char) prescaler);
    ssi_rx_clock_prescaler(mod, false);

    /****** SFCSR (SSI FIFO Control/Status Register) ******/
    /* Empty threshold flag is set for transmit if there are >=4 empty slots in the Transmit FIFO */
    ssi_tx_fifo_empty_watermark(mod, ssi_fifo_0, (unsigned char) TX_FIFO_EMPTY_WML);
    ssi_tx_fifo_empty_watermark(mod, ssi_fifo_1, (unsigned char) TX_FIFO_EMPTY_WML);

    /* Full threshold flag is set for receive if there are 4 data words in the Receive FIFO */
    ssi_rx_fifo_full_watermark(mod, ssi_fifo_0, (unsigned char) 4);
    ssi_rx_fifo_full_watermark(mod, ssi_fifo_1, (unsigned char) 4);

    ssi_transmit_enable(mod, false);
    ssi_receive_enable(mod, false);

    ssi_interrupt_disable(mod, ssi_rx_dma_interrupt_enable);
    ssi_interrupt_disable(mod, ssi_tx_dma_interrupt_enable);

    ssi_enable(mod, false);
    ssi_enable(mod, true);
   
    /****** STMSK (SSI Transmit Time Slot Mask Register) *******/
    ssi_tx_mask_time_slot(mod, 0xFFFFFFFC); /* Use time slots 0, 1 only */

    ssi_interrupt_enable (apal_driver_state.mod, ssi_tx_dma_interrupt_enable);
    ssi_tx_fifo_enable(apal_driver_state.mod, ssi_fifo_0, true);
    ssi_transmit_enable (apal_driver_state.mod, true);

    return APAL_SUCCESS;
}

/*=================================================================================================
FUNCTION: apal_switch_stdac_clock()

DESCRIPTION: 
    This function switches STDAC input clock rate between 13 and 26 Mhz.

ARGUMENTS PASSED:
    switch_type - APAL_SWITCH_CLOCK_TO_13MHZ -> Switch clock to 13Mhz
                  APAL_SWITCH_CLOCK_TO_26MHZ -> Switch clock to 26Mhz

RETURN VALUE:
    None

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    NOTE: We are not doing clocking switching etc for BUTE platform. Need to handle BUTE case.

==================================================================================================*/
void
apal_switch_stdac_clock
(
    int switch_type
)
{
    unsigned int mask = 0, value = 0;

    APAL_DEBUG_LOG ("%s() called with switch_type = %d\n", __FUNCTION__, switch_type);

    mask = AUDIOIC_SET_STDAC_EN | AUDIOIC_SET_STDAC_RESET | AUDIOIC_SET_STDAC_CLK_EN;
    TRY ( power_ic_audio_set_reg_mask_audio_stereo_dac (mask, value) )

    value = mask;
    mask |= AUDIOIC_SET_STDAC_CLK2 | AUDIOIC_SET_STDAC_CLK1 | AUDIOIC_SET_STDAC_CLK0;
    
    if (switch_type == APAL_SWITCH_CLOCK_TO_13MHZ)
    {
        value |= 0;
    }
    else 
    {
        value |= AUDIOIC_SET_STDAC_CLK2;
    }

    TRY ( power_ic_audio_set_reg_mask_audio_stereo_dac (mask, value) )
    
    msleep (15);

    mask = value = AUDIOIC_SET_ADD_ST_DAC;
    TRY ( power_ic_audio_set_reg_mask_audio_rx_0 (mask, value) )

    return;

CATCH:
    APAL_ERROR_LOG ("Operation failed inside %s\n", __FUNCTION__);
}

/*==================================================================================================
                                          LOCAL FUNCTIONS (MAP to IOCTL's)
==================================================================================================*/
/*==================================================================================================
FUNCTION: apal_set_audio_route()

DESCRIPTION: 
    This function sets the requested audio route

ARGUMENTS PASSED:
    audio_route - Audio route to be set

RETURN VALUE:
    APAL_SUCCESS - If route set successfully
    APAL_FAILURE - If route could not be set

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    Common audio bus connects AUDMUX port_4, Atlas SSI1 and the Bluetooth module. port_4 can do 
    audio to CODEC or Bluetooth module but not to both at the same time. STDAC will always use 
    AUDMUX port_2 which is connected to SSI2 and port_5 which is connected to Atlas SSI2.

==================================================================================================*/
static int 
apal_set_audio_route
(
    APAL_AUDIO_ROUTE_ENUM_T audio_route
)
{
    volatile unsigned long reg;
    APAL_DEBUG_LOG ("%s() called with audio_route = %d\n", __FUNCTION__, audio_route);

    /* Set the input clock to ATLAS (CLIA) */
    {
#if defined(CONFIG_ARCH_MXC91231)
 
        reg = *((volatile unsigned long *)(IO_ADDRESS(ACR)));

        APAL_DEBUG_LOG ("Before setting, ACR = %08x\n", (unsigned int)reg);

        reg &= 0xFFFFFF8F;

        if (SCMA11_PASS_1)
	{
            /* Set ACR (AP CKO Register)*/
            reg |= 0x00000070;
            *((volatile unsigned long *)(IO_ADDRESS(ACR))) = reg;
	}
        else
        {
            APAL_DEBUG_LOG ("Setting reg |= 0x00000020\n");
            /* Set ACR (AP CKO Register) to select ap_ref_x2_clk (ap_ref_x2_clk is 26Mhz) */
            reg |= 0x00000020;
            (*((volatile unsigned long *)(IO_ADDRESS(ACR)))) = reg;
	}

        /* Set ACR (AP CKO Register)*/
        (*((volatile unsigned long *)(IO_ADDRESS(ACR)))) &= 0xFFFFFF7F;

        APAL_DEBUG_LOG("After setting,ACR= %08x\n",(unsigned int)(*((volatile unsigned long*)
                       (IO_ADDRESS(ACR)))));

        APAL_DEBUG_LOG("Before setting,CSCR=%08x\n",(unsigned int)(*((volatile unsigned long*)
                       (IO_ADDRESS(CSCR)))));

        /* If we are to go in a voice call then use corrected clock */
        if (audio_route == APAL_AUDIO_ROUTE_PHONE) 
        {
            unsigned int mask = 0, value = 0;
            APAL_DEBUG_LOG ("Using corrected clock for voice call\n");

            /* We need to switch Stdac freq from 26Mhz to 13Mhz because we are going in call so mute
               STDAC in case it was on */
            mask = AUDIOIC_SET_ADD_ST_DAC;
            TRY ( power_ic_audio_set_reg_mask_audio_rx_0 (mask, value) )

            /* Set CKO_SEL in CSCR (CRM System Control Register) */
            (*((volatile unsigned long *)(IO_ADDRESS(CSCR)))) &= 0xFFFDFFFF; /* clear bit 17 */

            /* If STDAC was in use reconfigure it */
            if (apal_driver_state.stdac_in_use == TRUE)
            {
                apal_switch_stdac_clock (APAL_SWITCH_CLOCK_TO_13MHZ);
            }

            apal_driver_state.phone_call_on = TRUE;
        }
        else 
        {
            if (apal_driver_state.phone_call_on == FALSE)
            {
                APAL_DEBUG_LOG ("Using CRM_AP clock for multimedia\n");
                /* Set CKO_SEL in CSCR (CRM System Control Register) */
                (*((volatile unsigned long *)(IO_ADDRESS(CSCR)))) |= 0x00020000; /* set bit 17 */
            }
        }

        APAL_DEBUG_LOG("After setting,CSCR= %08x\n",(unsigned int)(*((volatile unsigned long*)
                       (IO_ADDRESS(CSCR)))));

#elif defined(CONFIG_ARCH_MXC91331)
        /* Clear up first 7 bits of COSR register */
        (*((volatile unsigned long *)(IO_ADDRESS(COSR)))) &= 0xFFFFFF80;

        /* Enable and set CKO1 bits in COSR Register) */
        (*((volatile unsigned long *)(IO_ADDRESS(COSR)))) |= 0x00000041;
#endif 
    }

    switch (audio_route)
    {
        case APAL_AUDIO_ROUTE_PHONE: 
            {
                if (apal_driver_state.port_4_in_use == FALSE)
                {
                    /* Configure AUDMUX to use SAP and Port_4 */
                    TRY ( apal_setup_dam (port_3, port_4, APAL_ATLAS_MASTER) )
                    apal_driver_state.port_4_in_use = TRUE;
                }
                else
                {
                    APAL_DEBUG_LOG ("ROUTE_PHONE_ANALOG Port_4 already in use\n");
                }    
                /* Configure CODEC */
                TRY ( apal_setup_codec (APAL_ATLAS_MASTER, audio_route) )

                vaudio_on = TRUE;
            }
            break;

        case APAL_AUDIO_ROUTE_CODEC_AP:
            {
                if (apal_driver_state.port_4_in_use == FALSE)
	        {
                    /* Configure AUDMUX to use SSI1 and Port 4 */
		    TRY ( apal_setup_dam (port_1, port_4, APAL_ATLAS_MASTER) )
                    apal_driver_state.port_4_in_use = TRUE;
                }
                else
                {
                    APAL_ERROR_LOG ("AUDIO_ROUTE_CODEC cannot be set, Port_4 already in use\n");
                    return APAL_FAILURE;
                }

                /* Configure SSI1 */
                TRY ( apal_setup_ssi (APAL_CODEC, APAL_ATLAS_MASTER) )

                
                apal_driver_state.codec_in_use = TRUE;
                stdac_codec_in_use = APAL_CODEC;

                /* Configure CODEC */
                TRY ( apal_setup_codec (APAL_ATLAS_MASTER, audio_route) )

                vaudio_on = TRUE;
            }
            break;

        case APAL_AUDIO_ROUTE_STDAC:
            {
                /* Configure AUDMUX to use SSI2 and Port 5 */
                TRY ( apal_setup_dam (port_2, port_5, APAL_ATLAS_MASTER) )

                /* Configure SSI2  */
                TRY ( apal_setup_ssi (APAL_STDAC, APAL_ATLAS_MASTER) )

                stdac_codec_in_use = APAL_STDAC;

                /* Configure STDAC */
                TRY ( apal_setup_stdac (APAL_ATLAS_MASTER) )

                apal_driver_state.stdac_in_use = TRUE;

                vaudio_on = TRUE;
            }
            break;

        case APAL_AUDIO_ROUTE_FMRADIO:
                   {
                       /* Nothing to do for analog FM, just remember the status for DSM */
                       apal_driver_state.fmradio_on = TRUE;
                   }
                   break;

        default:
            APAL_ERROR_LOG ("%s() called with invalid audio_route = %d", __FUNCTION__, audio_route);
    }
    
    return APAL_SUCCESS;

CATCH:
    APAL_ERROR_LOG ("Operation failed inside %s\n", __FUNCTION__);
    return APAL_FAILURE;
}


/*==================================================================================================
FUNCTION: apal_clear_audio_route()

DESCRIPTION: 
    This function clears the requested audio route

ARGUMENTS PASSED:
    audio_route - Audio route to be cleared

RETURN VALUE:
    APAL_SUCCESS - If route cleared successfully
    APAL_FAILURE - If route could not be cleared

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static int 
apal_clear_audio_route
(
    APAL_AUDIO_ROUTE_ENUM_T audio_route
)
{
    APAL_DEBUG_LOG ("%s() called with audio_route = %d\n", __FUNCTION__, audio_route);

    switch (audio_route)
    {
        case APAL_AUDIO_ROUTE_PHONE: 
            apal_driver_state.port_4_in_use = FALSE;
            dam_reset_register(port_3);
	    dam_reset_register(port_4);
            apal_driver_state.phone_call_on = FALSE;
            break;

        case APAL_AUDIO_ROUTE_CODEC_AP: 
            apal_driver_state.port_4_in_use = FALSE;
            dam_reset_register(port_1);
	    dam_reset_register(port_4);
            apal_driver_state.codec_in_use = FALSE;
            ssi_tx_fifo_enable(apal_driver_state.mod, ssi_fifo_0, false);
            ssi_tx_flush_fifo (apal_driver_state.mod);
    	    ssi_transmit_enable (apal_driver_state.mod, false);
            mxc_clks_disable(SSI1_IPG_CLK);
            mxc_clks_disable(SSI1_BAUD);
            break;

        case APAL_AUDIO_ROUTE_STDAC:
            dam_reset_register(port_2);
            dam_reset_register(port_5);
            apal_driver_state.stdac_in_use = FALSE;
            ssi_tx_fifo_enable(apal_driver_state.mod, ssi_fifo_0, false);
            ssi_tx_flush_fifo (apal_driver_state.mod);
    	    ssi_transmit_enable (apal_driver_state.mod, false);
            mxc_clks_disable(SSI2_IPG_CLK);
            mxc_clks_disable(SSI2_BAUD);
            break;

        case APAL_AUDIO_ROUTE_FMRADIO:
            /* Nothing to do for analog FM, just remember the status for DSM */
            apal_driver_state.fmradio_on = FALSE;
            break;

        default:
            APAL_ERROR_LOG ("%s() called with invalid audio_route = %d", __FUNCTION__, audio_route);
    }

    /* @TODO: Need to handle clocking switching etc for BUTE */
#if defined(CONFIG_ARCH_MXC91231)
    if (audio_route == APAL_AUDIO_ROUTE_PHONE)
    {
        APAL_DEBUG_LOG ("Phone audio route is closing\n");

        if (apal_driver_state.stdac_in_use == TRUE)
	{
            unsigned int mask = 0, value = 0;
            APAL_DEBUG_LOG ("Stdac is in use so we need to switch from 13 Mhz to 26 Mhz\n");

            mask = AUDIOIC_SET_ADD_ST_DAC;
            TRY ( power_ic_audio_set_reg_mask_audio_rx_0 (mask, value) )

            /* Set CKO_SEL in CSCR (CRM System Control Register) */
            (*((volatile unsigned long *)(IO_ADDRESS(CSCR)))) |= 0x00020000; /* set bit 17 */

            apal_switch_stdac_clock (APAL_SWITCH_CLOCK_TO_26MHZ);
        }
    }
#endif

    return APAL_SUCCESS;

CATCH:
    APAL_ERROR_LOG ("Operation failed inside %s\n", __FUNCTION__);
    return APAL_FAILURE;
}

/*==================================================================================================
FUNCTION: apal_sdma_stop_rw()

DESCRIPTION: 
    This function performs SDMA STOP/ABORT   

ARGUMENTS PASSED:
    void

RETURN VALUE:
    void

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static void
apal_sdma_stop_rw
(
    void
)
{
    APAL_DEBUG_LOG ("%s()\n", __FUNCTION__);

    spin_lock_irqsave(&apal_write_lock, flags);

    /* check if we need to stop SDMA transmit */
    if(apal_tx_dma_config[stdac_codec_in_use].first_write == TRUE)
    {
        int prev_buf_read = 0;
        APAL_DEBUG_LOG("stop playback\n");

        if(apal_tx_dma_config[stdac_codec_in_use].buffer_read != 
                 apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill)
        {
            if (apal_tx_dma_config[stdac_codec_in_use].start_dma == FALSE)
            {
                apal_tx_dma_config[stdac_codec_in_use].start_dma = TRUE;
                mxc_dma_start (apal_tx_dma_config[stdac_codec_in_use].write_channel);
            }

            /* play all driver buffer */
            prev_buf_read = apal_tx_dma_config[stdac_codec_in_use].buffer_read;

            spin_unlock_irqrestore(&apal_write_lock, flags);

            wait_event_interruptible_timeout (apal_tx_wait_queue, 
                   (apal_tx_dma_config[stdac_codec_in_use].start_dma == FALSE), APAL_SDMA_TIMEOUT);

            spin_lock_irqsave(&apal_write_lock, flags);
        }

        apal_tx_dma_config[stdac_codec_in_use].first_write = FALSE;
    }

    spin_unlock_irqrestore(&apal_write_lock, flags);
    /* check if we need to stop SDMA receive */
    if(apal_rx_dma_config.first_read == TRUE)
    {
        APAL_DEBUG_LOG("stop record\n");
        apal_rx_dma_config.immediate_stop = TRUE;
        wait_event_interruptible_timeout (apal_rx_wait_queue, 
                (apal_rx_dma_config.wait_end == FALSE), APAL_SDMA_TIMEOUT);
        apal_rx_dma_config.first_read = FALSE;
    }
}

/*==================================================================================================
FUNCTION: apal_stop()

DESCRIPTION: 
    This function stops playback or recording based on their session flag

ARGUMENTS PASSED:
    None

RETURN VALUE:
    APAL_SUCCESS

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None
	
==================================================================================================*/
static int 
apal_stop
(
    void
)
{
    APAL_DEBUG_LOG ("%s() called\n", __FUNCTION__);
    spin_lock_irqsave(&apal_write_lock, flags);

    if(apal_tx_dma_config[stdac_codec_in_use].first_write == TRUE)
    {
        apal_tx_dma_config[stdac_codec_in_use].immediate_stop = TRUE;
#ifdef CONFIG_MOT_WFN409
        mxc_dma_reset (apal_tx_dma_config[stdac_codec_in_use].write_channel,APAL_NB_BLOCK_SDMA);
#endif /* CONFIG_MOT_WFN409 */
        mxc_dma_stop (apal_tx_dma_config[stdac_codec_in_use].write_channel);

        /*@TODO: This is a hack to avoid the noise happening due to SDMA playing out old buffers 
                 even after a stop and reset */
        if (stdac_codec_in_use == APAL_STDAC)
        {
            /* fifo 0, SSI2 */
            tx_params.per_address     = (SSI2_BASE_ADDR + 0); /*STX20*/
            tx_params.event_id        = DMA_REQ_SSI2_TX1;
            tx_params.peripheral_type = SSI_SP;

            tx_params.watermark_level = TX_FIFO_EMPTY_WML;
            tx_params.transfer_type   = emi_2_per;
            tx_params.callback        = apal_dma_tx_interrupt_handler;
            tx_params.arg             = (void*)&apal_tx_dma_config[stdac_codec_in_use];
            tx_params.bd_number       = APAL_NB_BLOCK_SDMA;
            tx_params.word_size       = TRANSFER_16BIT;

            spin_unlock_irqrestore(&apal_write_lock, flags);
            mxc_dma_setup_channel(apal_tx_dma_config[stdac_codec_in_use].write_channel, &tx_params);
            spin_lock_irqsave(&apal_write_lock, flags);
        }
        /* @TODO: End Hack */

        if (stdac_codec_in_use == APAL_CODEC)
        {
            ssi_tx_flush_fifo (SSI1);
        }
	else
        {
	    ssi_tx_flush_fifo (SSI2);
        }

        /* reset static variables */
        apal_tx_dma_config[stdac_codec_in_use].wait_end = FALSE;
        apal_tx_dma_config[stdac_codec_in_use].first_write = FALSE;
        apal_tx_dma_config[stdac_codec_in_use].wait_next = FALSE;
        apal_tx_dma_config[stdac_codec_in_use].start_dma = FALSE;
    }

    if(apal_rx_dma_config.first_read == TRUE)
    {
        apal_rx_dma_config.immediate_stop = TRUE;
        apal_sdma_stop_rw();
    }

    spin_unlock_irqrestore(&apal_write_lock, flags);
    return APAL_SUCCESS;
}

/*==================================================================================================
FUNCTION: apal_pause()

DESCRIPTION: 
    This function do a pause while playing

ARGUMENTS PASSED:
    stop_go - 0 pause
              1 play

RETURN VALUE:
    APAL_SUCCESS

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None
	
==================================================================================================*/
static int 
apal_pause
(
    int stop_go
)
{
    APAL_DEBUG_LOG ("%s() called\n", __FUNCTION__);

    if(apal_tx_dma_config[stdac_codec_in_use].first_write == TRUE)
    {
        if (stop_go == 0) {
            ssi_transmit_enable(stdac_codec_in_use, false);
            apal_tx_dma_config[stdac_codec_in_use].pause = TRUE;
        } else {
            ssi_transmit_enable(stdac_codec_in_use, true);
            apal_tx_dma_config[stdac_codec_in_use].pause = FALSE;
        }
    }
    return APAL_SUCCESS;  
}

/*==================================================================================================
                                        FILE OPERATION FUNCTIONS
==================================================================================================*/
/*=================================================================================================
FUNCTION: apal_open()

DESCRIPTION: 
    This function is called when an open system call is made on /dev/apal

ARGUMENTS PASSED:
    inode - Kernel representation for disk file
    file  - opened by kernel on file open (i.e./dev/apal) and passed by kernel to every function 
           (functions in the file_operations structure) that operates on the file 

RETURN VALUE:
    APAL_SUCCESS - Returns SUCCESS on the first attempt to open /dev/apal
    APAL_FAILURE - All subsequent attempts will return FAILURE

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static int 
apal_open 
(
    struct inode* inode,
    struct file* file
)
{
    int ret_val = APAL_FAILURE;
    APAL_DEBUG_LOG ("%s() called\n", __FUNCTION__);

    if (open_count == 1)
    {
       return APAL_FAILURE;
    }

    /* Increment Usage Count */
    open_count++;

    apal_driver_state.port_4_in_use     = FALSE;
    apal_driver_state.stdac_sample_rate = POWER_IC_ST_DAC_SR_8000;
    apal_driver_state.codec_sample_rate = POWER_IC_CODEC_SR_8000;
    apal_driver_state.stdac_in_use      = FALSE;
    apal_driver_state.codec_in_use      = FALSE;
    apal_driver_state.phone_call_on     = FALSE;
    apal_driver_state.prepare_for_dsm   = FALSE;
    apal_driver_state.fmradio_on        = FALSE;

    init_waitqueue_head (&apal_tx_wait_queue);
    init_waitqueue_head (&apal_rx_wait_queue);
    init_waitqueue_head (&apal_dsm_wait_queue);

    /* Reset all Audio registers */
    ret_val = apal_audioic_power_off();

    /* reset the vaudio_on flag */
    vaudio_on = FALSE;
    return ret_val;
}

/*=================================================================================================
FUNCTION: apal_release()

DESCRIPTION: 
    This function is called when an close system call is made on /dev/apal

ARGUMENTS PASSED:
    inode - Kernel representation for disk file
    file  - opened by kernel on file open (i.e./dev/apal) and passed by kernel to every function 
           (functions in the file_operations structure) that operates on the file 

RETURN VALUE:
    Always SUCCESS 

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static int 
apal_release 
(
    struct inode* inode,
    struct file* file
)
{
    int i, j;
    APAL_DEBUG_LOG ("%s() called\n", __FUNCTION__);

    /* Decrement Usage Count */
    open_count--;
    if (open_count < 0)
    {
      open_count = 0;
    }

    ssi_enable(SSI1, false);
    ssi_enable(SSI2, false);

    dam_reset_register(port_1);
    dam_reset_register(port_2);
    dam_reset_register(port_3);
    dam_reset_register(port_4);
    dam_reset_register(port_5);

    /* TODO: clear up DMA */

#if defined(CONFIG_ARCH_MXC91231)
    /* Disable CKO in ACR (AP CKO Register) */
    (*((volatile unsigned long *)(IO_ADDRESS(ACR)))) |= 0x00000080;

#elif defined(CONFIG_ARCH_MXC91331)
    /* Disable CKO1 in COSR register for BUTE) */
    (*((volatile unsigned long *)(IO_ADDRESS(COSR)))) &= 0xFFFFFFBF;
#endif

    for(j=0; j < 2; j++)
    {
        if(apal_tx_dma_config[j].apal_tx_buf_flag == TRUE)
        {
            for(i=0; i<APAL_NB_BLOCK_SDMA; i++)
            { 
                kfree(apal_tx_dma_config[j].myTxBufStructSDMA[i].pbuf);
                apal_tx_dma_config[j].myTxBufStructSDMA[i].buf_size = 0;
            }
            apal_tx_dma_config[j].apal_tx_buf_flag = FALSE;
        }
    }
    if(apal_rx_dma_config.apal_rx_buf_flag == TRUE)
    {
        for(i=0; i<APAL_NB_BLOCK_SDMA; i++)
        { 
            kfree(apal_rx_dma_config.myRxBufStructSDMA[i].pbuf);
            apal_rx_dma_config.myRxBufStructSDMA[i].buf_size = 0;
        }
        apal_rx_dma_config.apal_rx_buf_flag = FALSE;
    }

    /* Reset Power IC */
    apal_audioic_power_off ();
    
    return APAL_SUCCESS;
}

/*=================================================================================================
FUNCTION: apal_poll()

DESCRIPTION: 
    This function is called when a poll system call is made on /dev/apal

ARGUMENTS PASSED:
    file        file pointer
    wait        poll table for this poll() 

RETURN VALUE:
    mask of POLLIN|POLLRDNORM

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static unsigned int 
apal_poll
(
    struct file *file, 
    poll_table *wait
)
{
    unsigned int ret_val = 0;

    APAL_DEBUG_LOG ("%s() called\n", __FUNCTION__);

    /* Add our wait queue to the poll table */
    poll_wait (file, &apal_dsm_wait_queue, wait);
 
    if (apal_driver_state.prepare_for_dsm == TRUE)
    {
        APAL_DEBUG_LOG ("prepare_for_dsm = TRUE\n");
        ret_val = (POLLIN | POLLRDNORM);
        apal_driver_state.prepare_for_dsm = FALSE;
    }
  
    return ret_val;
}

/*=================================================================================================
FUNCTION: apal_ioctl()

DESCRIPTION: 
    This function is called when an ioctl system call is made on /dev/apal

ARGUMENTS PASSED:
    inode - Kernel representation for disk file
    file  - opened by kernel on file open (i.e./dev/apal) and passed by kernel to every function 
           (functions in the file_operations structure) that operates on the file
    cmd   - IOCTL command sent from the user space
    arg   - Any argument sent with the ioctl command eg: audio_route, output_device, gain etc 

RETURN VALUE:
    APAL_SUCCESS - If the ioctl command succeeded
    APAL_FAILURE - If the ioctl command failed

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static int 
apal_ioctl 
(
    struct inode *inode, 
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int enable, stop_go;
    int ret_val = APAL_FAILURE;
    int unplayed_buffer_number = 0;
    int unplayed_bytes = 0;	 


    APAL_AUDIO_ROUTE_ENUM_T audio_route = APAL_AUDIO_ROUTE_NONE;

    APAL_DEBUG_LOG ("%s() called with cmd %d\n", __FUNCTION__, cmd);

    switch (cmd)
    {
        case APAL_IOCTL_PAUSE:
            TRY ( get_user (stop_go, (int*)arg) )	  
	    ret_val = apal_pause (stop_go);
            break;

        case APAL_IOCTL_STOP:
	    ret_val = apal_stop ();
            break;

        case APAL_IOCTL_SET_AUDIO_ROUTE:
            TRY ( get_user (audio_route, (int*)arg) )	  
	    ret_val = apal_set_audio_route (audio_route);
            break;

        case APAL_IOCTL_CLEAR_AUDIO_ROUTE:
            TRY ( get_user (audio_route, (int*)arg) )
	    ret_val = apal_clear_audio_route (audio_route);

#ifdef APAL_DEBUG            
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_CODEC, &atlas_config.audio_codec) )
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_STEREO_DAC,&atlas_config.audio_stdac))
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_RX_1, &atlas_config.audio_rx1) )
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_RX_0, &atlas_config.audio_rx0) )
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_TX, &atlas_config.audio_tx) )
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_SSI_NETWORK, &atlas_config.audio_ssi) )

            APAL_DEBUG_LOG ("REG_CODEC = %#x\n REG_STDAC = %#x\n REG_RX_1 = %#x\n REG_RX_0 = %#x\n \
                     REG_TX = %#x\n REG_SSI_NETWORK = %#x\n", atlas_config.audio_codec, \
                     atlas_config.audio_stdac, atlas_config.audio_rx1, atlas_config.audio_rx0, \
                     atlas_config.audio_tx, atlas_config.audio_ssi);
#endif
            break;

        case APAL_IOCTL_SET_DAI:
            TRY ( get_user (enable, (int*)arg) )

            /* DAI needs to treated differently. Here we connect port_3 with port_7 for data and
               port_4 needs to be connected to port_7 for Bitclk and Fsync */
            if (enable == TRUE) 
            {
                /* DAI also needs Port_7 to be connected to Port_4 */
		TRY ( apal_setup_dam (port_7, port_4, APAL_ATLAS_MASTER) ) 

                /* Do IOMUX setting needed for DAI */
                gpio_dai_enable();
            }
            else /* Go back in normal voice call mode */
            {
                dam_reset_register(port_7);
                gpio_dai_disable();
            }
           
            ret_val = APAL_SUCCESS;
            break;
      
        case APAL_IOCTL_AUDIO_POWER_OFF:
            ret_val = apal_audioic_power_off();
            
#if defined(CONFIG_ARCH_MXC91231)
            /* Disable CKO in ACR (AP CKO Register) */
            (*((volatile unsigned long *)(IO_ADDRESS(ACR)))) |= 0x00000080;
            
#elif defined(CONFIG_ARCH_MXC91331)
            /* Disable CKO1 in COSR register for BUTE */
            (*((volatile unsigned long *)(IO_ADDRESS(COSR)))) &= 0xFFFFFFBF;
#endif 

#ifdef APAL_DEBUG
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_CODEC, &atlas_config.audio_codec) )
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_STEREO_DAC,&atlas_config.audio_stdac))
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_RX_1, &atlas_config.audio_rx1) )
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_RX_0, &atlas_config.audio_rx0) )
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_AUDIO_TX, &atlas_config.audio_tx) )
            TRY ( power_ic_read_reg (POWER_IC_REG_ATLAS_SSI_NETWORK, &atlas_config.audio_ssi) )

            APAL_DEBUG_LOG ("REG_CODEC = %#x\n REG_STDAC = %#x\n REG_RX_1 = %#x\n REG_RX_0 = %#x\n \
                         REG_TX = %#x\n REG_SSI_NETWORK = %#x\n", atlas_config.audio_codec, \
                         atlas_config.audio_stdac, atlas_config.audio_rx1, atlas_config.audio_rx0, \
                         atlas_config.audio_tx, atlas_config.audio_ssi);
#endif
            break;

        case APAL_IOCTL_STOP_SDMA:
            apal_sdma_stop_rw();
            ret_val = APAL_SUCCESS;
            break;

	case APAL_GET_LEFT_WRITTEN_BYTES_IN_KERNEL:
	    unplayed_buffer_number = (apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill + APAL_NB_BLOCK_SDMA -
					apal_tx_dma_config[stdac_codec_in_use].buffer_read)%APAL_NB_BLOCK_SDMA;
	    unplayed_bytes = unplayed_buffer_number * current_size;
	    copy_to_user((int*)arg, &unplayed_bytes, sizeof(int));
	    ret_val = APAL_SUCCESS;
	    break;

        default:
            APAL_ERROR_LOG ("%s() received invalid ioctl = %d", __FUNCTION__, cmd);
    }
 
    return ret_val;

CATCH:
    APAL_ERROR_LOG ("Operation failed inside %s\n", __FUNCTION__);
    return APAL_FAILURE;
}

/*=================================================================================================
FUNCTION: apal_write()

DESCRIPTION: 
    This function is called when a write system call is made on /dev/apal

ARGUMENTS PASSED:
    inode     - Kernel representation for disk file
    bufStruct - Pointer to the buffer with data
    size      - Number of bytes to write
    nouse     - Unused

RETURN VALUE:
    size      - Number of bytes written
    

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static ssize_t
apal_write
(
    struct file *inode,
    const char  *bufStruct,
    size_t      size,
    loff_t      *nouse
)
{
    APAL_BUFFER_STRUCT_T * myBufStruct;
    dma_request_t sdma_request;
    int bufferDescIndex;
    
    APAL_DEBUG_LOG ("%s() called with size = %d and codec_in_use = %d\n", __FUNCTION__, size, 
                    stdac_codec_in_use); 

    if(size <= 0)
    {
	return size;
    }

    spin_lock_irqsave(&apal_write_lock, flags);

    /* update current playback size */
    current_size = size;
	
    if(apal_tx_dma_config[stdac_codec_in_use].apal_tx_buf_flag == FALSE)
    {
        APAL_DEBUG_LOG("Allocated new buffer for STDAC\n");
        /* ignore the size and allocate buffer */
        for(bufferDescIndex=0; bufferDescIndex<APAL_NB_BLOCK_SDMA; bufferDescIndex++)
        {
            apal_tx_dma_config[stdac_codec_in_use].myTxBufStructSDMA[bufferDescIndex].pbuf = 
                (unsigned char *) kmalloc(size,GFP_KERNEL);
            apal_tx_dma_config[stdac_codec_in_use].myTxBufStructSDMA[bufferDescIndex].buf_size=size;
        }
        apal_tx_dma_config[stdac_codec_in_use].count            = size;
        apal_tx_dma_config[stdac_codec_in_use].first_write      = FALSE;
        apal_tx_dma_config[stdac_codec_in_use].apal_tx_buf_flag = TRUE;
    }
    else if(apal_tx_dma_config[stdac_codec_in_use].count != size)
    {
        if(stdac_codec_in_use == APAL_CODEC)
        {
            APAL_DEBUG_LOG("This should not print, But!!!\n");
            /* TODO: Need to avoid this case at APAL interface or AM level */
        }

        if(apal_tx_dma_config[stdac_codec_in_use].start_dma == TRUE)
        {
            /* stop sdma */
            APAL_DEBUG_LOG("WARNING: buf size changed after SDMA start!!!");
            apal_tx_dma_config[stdac_codec_in_use].immediate_stop = TRUE;
#ifdef CONFIG_MOT_WFN409
            mxc_dma_reset (apal_tx_dma_config[stdac_codec_in_use].write_channel,APAL_NB_BLOCK_SDMA);
#endif /* CONFIG_MOT_WFN409 */
            mxc_dma_stop (apal_tx_dma_config[stdac_codec_in_use].write_channel);

            /* reset static variables */
            apal_tx_dma_config[stdac_codec_in_use].start_dma = FALSE;
        }

        APAL_DEBUG_LOG("buff size changed in middle, prv size = %d\n", 
                        apal_tx_dma_config[stdac_codec_in_use].count);
        for(bufferDescIndex=0; bufferDescIndex<APAL_NB_BLOCK_SDMA; bufferDescIndex++)
        {
            /* free old buffer and allocate new */
            kfree(apal_tx_dma_config[stdac_codec_in_use].myTxBufStructSDMA[bufferDescIndex].pbuf);
            apal_tx_dma_config[stdac_codec_in_use].myTxBufStructSDMA[bufferDescIndex].pbuf = 
                (unsigned char *) kmalloc(size,GFP_KERNEL);
            apal_tx_dma_config[stdac_codec_in_use].myTxBufStructSDMA[bufferDescIndex].buf_size = 
                size;
        }
        apal_tx_dma_config[stdac_codec_in_use].count            = size;
        apal_tx_dma_config[stdac_codec_in_use].first_write      = FALSE;
    }

    /* first time or SDMA data starvation */
    if (apal_tx_dma_config[stdac_codec_in_use].first_write == FALSE) 
    {    
        apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill = 
                                                 apal_tx_dma_config[stdac_codec_in_use].buffer_read;
        apal_tx_dma_config[stdac_codec_in_use].wait_next = FALSE;  /* don't wait for interrupt */
        apal_tx_dma_config[stdac_codec_in_use].start_dma = FALSE;
        apal_tx_dma_config[stdac_codec_in_use].wait_end = TRUE;
    	apal_tx_dma_config[stdac_codec_in_use].first_write = TRUE;
        apal_tx_dma_config[stdac_codec_in_use].immediate_stop = FALSE;
        apal_tx_dma_config[stdac_codec_in_use].pause = FALSE;
        apal_tx_dma_config[stdac_codec_in_use].count = size; 
    }

    memset(&sdma_request, 0, sizeof(sdma_request));

    bufferDescIndex = apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill;

    myBufStruct = &apal_tx_dma_config[stdac_codec_in_use].myTxBufStructSDMA[bufferDescIndex];
    copy_from_user (myBufStruct->pbuf, bufStruct, size); /* get the new data to write */

    spin_unlock_irqrestore(&apal_write_lock, flags);
    wait_event_interruptible_timeout (apal_tx_wait_queue, 
            (apal_tx_dma_config[stdac_codec_in_use].wait_next == FALSE), APAL_SDMA_TIMEOUT); 
    spin_lock_irqsave(&apal_write_lock, flags); 
    /* APAL_DEBUG_LOG ("after wait_event_interruptible_timeout()\n"); */

    if ((apal_tx_dma_config[stdac_codec_in_use].immediate_stop == FALSE) &&
        (apal_tx_dma_config[stdac_codec_in_use].first_write == TRUE) && 
        (apal_tx_dma_config[stdac_codec_in_use].wait_next == FALSE))
    {
        apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill = 
            (apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill+1)%APAL_NB_BLOCK_SDMA;

        if ((apal_tx_dma_config[stdac_codec_in_use].start_dma == TRUE) && 
            ((apal_tx_dma_config[stdac_codec_in_use].buffer_read+(APAL_NB_BLOCK_SDMA-1)) % 
              APAL_NB_BLOCK_SDMA) == apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill)
        {
            /* put this write in wait_event */
            apal_tx_dma_config[stdac_codec_in_use].wait_next = TRUE; 
        }

        /* SDMA ISSUE: count have to be equal to size of buf defined */
        if (size < apal_tx_dma_config[stdac_codec_in_use].count)
        {
            memset((char *)myBufStruct->pbuf + size, 0, 
                    apal_tx_dma_config[stdac_codec_in_use].count - size);
        }

        sdma_request.sourceAddr = (char*)sdma_virt_to_phys(myBufStruct->pbuf);
        sdma_request.count	= myBufStruct->buf_size;
        sdma_request.bd_cont    = 1;

    	/* Flush the cache before starting the write.
    	   Note that performance may be better if DMA_TO_DEVICE is used but
    	   that change will require taking care of possible cache line crossings */
    	consistent_sync(myBufStruct->pbuf, sdma_request.count, DMA_BIDIRECTIONAL);

    	/* configure buffer descriptor to chain transfers at SDMA level */
    	mxc_dma_set_config (apal_tx_dma_config[stdac_codec_in_use].write_channel, &sdma_request, 
                            bufferDescIndex);

        if ((apal_tx_dma_config[stdac_codec_in_use].start_dma == FALSE) &&  
             /* first time, fill data before SDMA request */
           ((((apal_tx_dma_config[stdac_codec_in_use].next_buffer_to_fill - 
                apal_tx_dma_config[stdac_codec_in_use].buffer_read) + 
                APAL_NB_BLOCK_SDMA)%APAL_NB_BLOCK_SDMA) > APAL_NB_WAIT_BLOCKS))
        {
            APAL_DEBUG_LOG("start SDMA\n");
            apal_tx_dma_config[stdac_codec_in_use].start_dma = TRUE;
            mxc_dma_start (apal_tx_dma_config[stdac_codec_in_use].write_channel);
        }
    }

    spin_unlock_irqrestore(&apal_write_lock, flags);
    return size;
}


/*=================================================================================================
FUNCTION: apal_read()

DESCRIPTION: 
    This function is called when a read system call is made on /dev/apal

ARGUMENTS PASSED:
    inode       - Kernel representation for disk file
    bufStruct   - Buffer in which read samples should be returned
    size        - NUmber of bytes to read
    nouse       - unused 

RETURN VALUE:
    size - Number of bytes read

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static ssize_t
apal_read
(
    struct file *inode,
    char  *bufStruct,
    size_t      size,
    loff_t      *nouse
)
{
    APAL_BUFFER_STRUCT_T * myBufStruct;
    dma_request_t sdma_request;
    int bufferDescIndex;
    int flags;

    APAL_DEBUG_LOG ("%s() called\n", __FUNCTION__);

    if(size <= 0)
    {
       return size;
    }
	 
    if (apal_rx_dma_config.first_read == FALSE) /* first time or SDMA data starvation */
    {
        apal_rx_dma_config.buffer_read = apal_rx_dma_config.next_buffer_to_fill;
        apal_rx_dma_config.wait_next = TRUE;  /* wait for interrupt */
        apal_rx_dma_config.wait_end = TRUE;   /* stop sdma if isr starve for data */
    	apal_rx_dma_config.first_read = TRUE;
        apal_rx_dma_config.immediate_stop = FALSE;

        if(apal_rx_dma_config.apal_rx_buf_flag == FALSE)
        {
            for (bufferDescIndex=0; bufferDescIndex<APAL_NB_BLOCK_SDMA; bufferDescIndex++)
            {
                apal_rx_dma_config.myRxBufStructSDMA[bufferDescIndex].pbuf = 
                                                        (unsigned char *) kmalloc(size,GFP_KERNEL);
                apal_rx_dma_config.myRxBufStructSDMA[bufferDescIndex].buf_size = size;
            }

            apal_rx_dma_config.apal_rx_buf_flag = TRUE;
        }
        else
            APAL_DEBUG_LOG("reuse old buffer req. size = %d\n", size);

        for (bufferDescIndex=0; bufferDescIndex<APAL_NB_BLOCK_SDMA; bufferDescIndex++)
        { 
            myBufStruct = &apal_rx_dma_config.myRxBufStructSDMA[bufferDescIndex];

            /* configure SDMA */
            memset(&sdma_request, 0, sizeof(sdma_request));
    	    sdma_request.destAddr = (char*)sdma_virt_to_phys(myBufStruct->pbuf);
    	    sdma_request.count	= size;
    	    sdma_request.bd_cont = 1;
    	    // flush the cache before starting the write
    	    // note that performance may be better if DMA_TO_DEVICE is used but
    	    // that change will require taking care of possible cache line crossings
    	    consistent_sync(myBufStruct->pbuf, sdma_request.count, DMA_BIDIRECTIONAL);

    	    /* configure buffer descriptor to chain transfers at SDMA level */
    	    mxc_dma_set_config (apal_rx_dma_config.read_channel, &sdma_request, bufferDescIndex);
        }

        /* Start DMA */
    	mxc_dma_start (apal_rx_dma_config.read_channel);
    	ssi_interrupt_enable (apal_driver_state.mod, ssi_rx_dma_interrupt_enable);
    	ssi_rx_fifo_enable(apal_driver_state.mod, ssi_fifo_0, true);
    	ssi_receive_enable (apal_driver_state.mod, true);
    }

    /* get the read buffer intex before updated by ISR */
    bufferDescIndex = apal_rx_dma_config.buffer_read;
    myBufStruct = &apal_rx_dma_config.myRxBufStructSDMA[bufferDescIndex];

    wait_event_interruptible_timeout (apal_rx_wait_queue, 
            (apal_rx_dma_config.wait_next == FALSE), APAL_SDMA_TIMEOUT); 
    APAL_DEBUG_LOG ("after wait_event_interruptible_timeout()\n");
    
    if(apal_rx_dma_config.first_read == TRUE)
    {
        spin_lock_irqsave(&apal_read_lock, flags); 
        apal_rx_dma_config.buffer_read = (apal_rx_dma_config.buffer_read+1)%APAL_NB_BLOCK_SDMA;
        if (apal_rx_dma_config.buffer_read == apal_rx_dma_config.next_buffer_to_fill)
        {
            /* put the next read in waitQ, reading faster than interrupt */
            apal_rx_dma_config.wait_next = TRUE; 
        }
        spin_unlock_irqrestore(&apal_read_lock, flags);

        /* Get the data */
        copy_to_user (bufStruct, myBufStruct->pbuf, size);
        
        if (apal_rx_dma_config.immediate_stop == FALSE)
        {
            memset(&sdma_request, 0, sizeof(sdma_request));
            sdma_request.destAddr = (char*)sdma_virt_to_phys(myBufStruct->pbuf);
            sdma_request.count	= myBufStruct->buf_size;
            sdma_request.bd_cont = 1;

            // flush the cache before starting the write
            // note that performance may be better if DMA_TO_DEVICE is used but
            // that change will require taking care of possible cache line crossings
            consistent_sync(myBufStruct->pbuf, sdma_request.count, DMA_BIDIRECTIONAL);

            /* configure buffer descriptor to chain transfers at SDMA level */
            mxc_dma_set_config (apal_rx_dma_config.read_channel, &sdma_request, bufferDescIndex);
        }
    }
        
    return size;
}

/*==================================================================================================
                                        REGISTER/ UNREGISTER FUNCTIONS
==================================================================================================*/
/*=================================================================================================
FUNCTION: apal_init()

DESCRIPTION: 
    This function is called when a module is installed by the kernel either externally or internally

ARGUMENTS PASSED:
    None

RETURN VALUE:
    APAL_SUCCESS - Returns SUCCESS if the character device(driver) (/dev/apal) is successfully 
                   registered with the kernel
    APAL_FAILURE - If the APAL device (driver) could not be registered

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static int __init 
apal_init
(
    void
)
{
    int bufferDescIndex = 0;
    
    /* register device with kernel and get major number */
    apal_driver_state.major_num = register_chrdev(0,"apal", &apal_fops);
        
    if (apal_driver_state.major_num < 0)
    {
        APAL_ERROR_LOG ("Unable to register APAL driver with the kernel");
        return APAL_FAILURE;   
    }

    APAL_DEBUG_LOG ("creating devfs entry for APAL \n");
    devfs_mk_cdev (MKDEV (apal_driver_state.major_num, 0), S_IFCHR | /*S_IRUGO |*/ S_IRUSR | 
                   S_IWUSR, "apal" );

    APAL_DEBUG_LOG ("APAL driver registered with the kernel\n");
    
#if defined(CONFIG_ARCH_MXC91231)
    /* Disable CKO in ACR (AP CKO Register) */
    (*((volatile unsigned long *)(IO_ADDRESS(ACR)))) |= 0x00000080;

#elif defined(CONFIG_ARCH_MXC91331)
    /* Disable CKO1 in COSR register for BUTE */
    (*((volatile unsigned long *)(IO_ADDRESS(COSR)))) &= 0xFFFFFFBF;
#endif 

    /* Configure DMA */
    apal_setup_dma (APAL_CODEC);
    apal_setup_dma (APAL_STDAC);

    apal_tx_dma_config[APAL_CODEC].next_buffer_to_fill = 0;
    apal_tx_dma_config[APAL_CODEC].buffer_read = 0;
    apal_tx_dma_config[APAL_CODEC].first_write = FALSE;
    apal_tx_dma_config[APAL_STDAC].next_buffer_to_fill = 0;
    apal_tx_dma_config[APAL_STDAC].buffer_read = 0;
    apal_tx_dma_config[APAL_STDAC].first_write = FALSE;
    apal_rx_dma_config.next_buffer_to_fill = 0;
    apal_rx_dma_config.buffer_read = 0;
    apal_rx_dma_config.first_read = FALSE;

    /* allocate default codec Tx buffer size - 8kHz mono */
    for(bufferDescIndex=0; bufferDescIndex<APAL_NB_BLOCK_SDMA; bufferDescIndex++)
    {
        apal_tx_dma_config[APAL_CODEC].myTxBufStructSDMA[bufferDescIndex].pbuf = 
            (unsigned char *) kmalloc(APAL_SDMA_8KMONO_SIZE,GFP_KERNEL);
        apal_tx_dma_config[APAL_CODEC].myTxBufStructSDMA[bufferDescIndex].buf_size = 
            APAL_SDMA_8KMONO_SIZE;
    }

    /* allocate default codec Rx buffer size - 8kHz mono */
    // NOTE: not combined this two loop because, the malloc might interleave the buffers
    for(bufferDescIndex=0; bufferDescIndex<APAL_NB_BLOCK_SDMA; bufferDescIndex++)
    {
        apal_rx_dma_config.myRxBufStructSDMA[bufferDescIndex].pbuf = 
            (unsigned char *) kmalloc(APAL_SDMA_8KMONO_SIZE,GFP_KERNEL);
        apal_rx_dma_config.myRxBufStructSDMA[bufferDescIndex].buf_size = APAL_SDMA_8KMONO_SIZE;
    }

    apal_tx_dma_config[APAL_CODEC].count = APAL_SDMA_8KMONO_SIZE;
    apal_tx_dma_config[APAL_CODEC].apal_tx_buf_flag = TRUE;
    apal_rx_dma_config.apal_rx_buf_flag = TRUE;

    /* allocate stdac bufer in first write */
    apal_tx_dma_config[APAL_STDAC].count = 0;
    apal_tx_dma_config[APAL_STDAC].apal_tx_buf_flag = FALSE;

    /* Register driver for Power Management purposes */
    {
        int ret;

        ret = driver_register (&apal_device_driver);
        
        if (ret == 0) 
        {
            ret = platform_device_register (&apal_platform_device);
        }

        return ret;
    }
}

/*=================================================================================================
FUNCTION: apal_exit()

DESCRIPTION: 
    This function is called when a module is removed by rmmod command

ARGUMENTS PASSED:
    None

RETURN VALUE:
    None

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static void __exit  
apal_exit 
(
    void
)
{
    int ret_val ;

    mxc_free_dma (apal_tx_dma_config[APAL_STDAC].write_channel);  /* Free DMA channel */
    mxc_free_dma (apal_tx_dma_config[APAL_CODEC].write_channel);  /* Free DMA channel */
    mxc_free_dma (apal_rx_dma_config.read_channel);  /* Free DMA channel */

    devfs_remove("apal");
    ret_val = unregister_chrdev (apal_driver_state.major_num, "apal");
 
    platform_device_unregister (&apal_platform_device);
    driver_unregister (&apal_device_driver);

    if (ret_val == -EINVAL)
    {
        APAL_ERROR_LOG ("Unable to unregister APAL driver with the kernel");
        return;
    }

    APAL_DEBUG_LOG ("APAL driver unregistered with the kernel\n");
    return;     
}

/*=================================================================================================
FUNCTION: apal_suspend()

DESCRIPTION: 
    This function is called by the power management module requesting for permission to go in DSM

ARGUMENTS PASSED:
    dev - the device structure used to store device specific information that is used by the suspend
          ,resume and remove functions
    state - the power state the device is entering.
    level - the stage in device resumption process that we want the device to be put in.

RETURN VALUE:
    Negative - if Audio activity is going on or if audio power off has not happened yet
    0 - If no activity in progress and Audio chip has been powered off

PRE-CONDITIONS
    If the PM driver is calling suspend function then we can assume that all the applications have
    okayed the phone to go in DSM.

POST-CONDITIONS
    None

IMPORTANT NOTES:
    Note the following audio related conditions when the phone can go in DSM:
    1) Phone can go in DSM even when a voice call is on.
    2) Phone can go in DSM when FM radio playback is on. This is TODO.
    3) Phone can go in DSM when music playback is paused.

==================================================================================================*/
static int 
apal_suspend
(
    struct device *dev, 
    u32 state, 
    u32 level
)
{
    APAL_DEBUG_LOG ("%s() called\n", __FUNCTION__);

    if (vaudio_on == FALSE)
    {
        /* All audio activity is already stopped and Power IC has been switched off */
    }
    else
    {
        /* Phone call case */ 
        if (apal_driver_state.phone_call_on == TRUE)
        {
            /* Don't do anything. Power IC is already setup for voice call */
        }
        else if (apal_driver_state.fmradio_on == TRUE)
        {
            /* Don't do anything. Power IC is already setup for FM radio */
             APAL_DEBUG_LOG ("apal_driver_state.fmradio_on == TRUE\n");
        }
        else if (apal_driver_state.stdac_in_use || apal_driver_state.codec_in_use)
        {
            return -EBUSY;
        }
        else
        {
            /* There is no active audio going on but the audio timer hasn't expired so vaudio_on is
               not set. Don't wait for the timer reset Power IC and go in DSM */
            apal_audioic_power_off();
            apal_driver_state.prepare_for_dsm = TRUE;
            wake_up_interruptible (&apal_dsm_wait_queue);
        }
    }

    return 0;
}

/*=================================================================================================
FUNCTION: apal_resume()

DESCRIPTION: 
    This function handles resume callback message from power management 

ARGUMENTS PASSED:
    dev - the device structure used to store device specific information that is used by the suspend
          ,resume and remove functions
    level - the stage in device resumption process that we want the device to be put in

RETURN VALUE:
    Always return 0

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static int 
apal_resume
(
    struct device *dev, 
    u32 level
)
{
    APAL_DEBUG_LOG ("%s() called\n", __FUNCTION__);
    return 0;
}

/*==================================================================================================
FUNCTION: apal_probe()

DESCRIPTION: 
    This function is callback function called from the power management module

ARGUMENTS PASSED:
    dev - the device structure used to store device specific information that is used by the suspend
          ,resume and remove functions

RETURN VALUE:
    Always return 0

PRE-CONDITIONS
    None

POST-CONDITIONS
    None

IMPORTANT NOTES:
    None

==================================================================================================*/
static int 
apal_probe
(
    struct device *dev
)
{
    return 0;
}

/*================================================================================================*/

module_init(apal_init);
module_exit(apal_exit);

MODULE_DESCRIPTION("APAL (Audio Platform Abstraction Layer) driver");
MODULE_AUTHOR("Motorola, Inc.");
MODULE_LICENSE("GPL");
