/*
 * Copyright (C) 2005-2007 Motorola, Inc.
 */

/* 
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
 * Motorola 2007-May-22 - Set a flag for _all_tx_data_sent
 * Motorola 2007-Feb-28 - Add call to mpm_handle_ioi to the ISRs
 * Motorola 2007-Jan-09 - Correct the SIMPD debouncing thread
 * Motorola 2006-Aug-24 - Remove some functionality from the driver
 * Motorola 2006-Aug-21 - Add support for all peripheral clock frequencies
 * Motorola 2006-Jul-24 - More MVL upmerge changes
 * Motorola 2006-Jul-14 - MVL upmerge
 * Motorola 2006-Jun-07 - Provide prescalar adjustment for a doubled input clock
 * Motorola 2006-Apr-20 - Now set SP_SIM1_TRXD GPIO in init function
 * Motorola 2006-Apr-14 - Update baud rate selection
 * Motorola 2006-Mar-20 - Cleanup and Update for BUTE
 * Motorola 2006-Feb-02 - Remove reference clock control
 * Motorola 2006-Jan-30 - Added an IOCTL to retrieve the entire IC register array
 * Motorola 2006-Jan-27 - Added ETU counter roll-over handling to the ISR
 * Motorola 2005-Mar-18 - Initial creation.
 */

/*
 * This file includes all of the initialization and tear-down code for the SIM
 * low-level driver and the basic user-mode interface (open(), close(), ioctl(), etc.).
 */

#include <asm/bitops.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mot-gpio.h>
#include <asm/uaccess.h>         
#include <asm-arm/arch-mxc/gpio.h>
#include <asm-arm/arch-mxc/spba.h>
#include <asm-arm/arch-mxc/clock.h>
#include <linux/config.h>
#include <linux/delay.h>
#include <linux/devfs_fs_kernel.h>  
#include <linux/errno.h>
#include <linux/init.h>  
#include <linux/interrupt.h>
#include <linux/kernel.h> 
#include <linux/module.h>
#include <linux/mpm.h>
#include <linux/poll.h>
#include <linux/power_ic_kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/smart_card.h>

#include "smart_card_kernel.h"
#include "../../power_ic/core/os_independent.h"
/******************************************************************************
 * Global Variables
 *****************************************************************************/

/******************************************************************************
* Constants
******************************************************************************/
#define NO_OF_SIM_REGS     (sizeof(SIM_MODULE_REGISTER_BANK) / sizeof(UINT32))
#define REG_ACCESS_ARG_LENGTH 5
#define TO_JIFFIES(msec) (1 + (((msec) * HZ) / 1000))
#define SIMPD_POLL_INTERVAL_MS 5
#define SIMPD_POLL_INTERVAL_JIFFIES TO_JIFFIES(SIMPD_POLL_INTERVAL_MS)
#define SIMPD_POLL_COUNT    8


#define SIM_MODULE_EVENT_RX_A                   0x00000001
#define SIM_MODULE_EVENT_RX_B                   0x00000002
#define SIM_MODULE_EVENT_SIMPD_BOUNCE           0x00000004
#define SIM_MODULE_EVENT_SIMPD_INSERTION        0x00000008      
#define SIM_MODULE_EVENT_SIMPD_REMOVAL          0x00000010
#define SIM_MODULE_EVENT_PARITY_ERROR           0x00000020
#define SIM_MODULE_EVENT_BUFFER_INDEX           0x00000040
#define SIM_MODULE_EVENT_ERROR_FLAG             0x00000080
#define SIM_MODULE_EVENT_RX_FLAG                0x00000100

/******************************************************************************
 * Local Macros
 *****************************************************************************/


/******************************************************************************
* Local type definitions
******************************************************************************/
typedef enum {
    ISR_NO_ERROR =         0x00,
    ISR_FATAL_ERROR =      0x01,
    ISR_NACK_LIMIT_ERROR = 0x02
} SIM_MODULE_ISR_ERROR;

typedef struct
{
    UINT16               buffer_index;
    UINT16               rx_last_index;
    UINT16               tx_length;
    UINT8                error_flag;
    UINT8                buffer[SIM_MODULE_MAX_DATA];
}SIM_MODULE_BUFFER_DATA;

typedef enum {
    GPCNT_CLK_DISABLE,
    GPCNT_CLK_CARD,
    GPCNT_CLK_RECV,
    GPCNT_CLK_ETU
}SIM_MODULE_IC_GPCNT_CLK_SOURCE;

typedef enum
{
    SIM_MODULE_RX_MODE,
    SIM_MODULE_TX_MODE,
    SIM_MODULE_RESET_DETECT_MODE
} SIM_MODULE_INTERRUPT_MODE;

enum
{
    SIM_MODULE_1 = 0,
#if (defined(CONFIG_ARCH_MXC91321))
    SIM_MODULE_2,
#endif
    NUM_SIM_MODULES
};
/******************************************************************************
* Local function prototypes
******************************************************************************/
static irqreturn_t sim_module_int_data_1(int irq, void *dev_id, struct pt_regs *regs);
static irqreturn_t sim_module_int_irq_1(int irq, void *dev_id, struct pt_regs *regs);

static unsigned int sim_poll(struct file *file, poll_table *wait);

static void sim_module_cwt_rollover  (UINT8 reader_id);
static void sim_module_gpcnt_rollover(UINT8 reader_id);
static void sim_module_int_reset_detect (UINT8 reader_id);
static void sim_module_int_rx(UINT8 reader_id);
static void sim_module_init_rx_mode (UINT8 reader_id);
static void sim_module_int_tx(UINT8 reader_id);
static int  sim_module_debounce_simpd(void * unused);
static void sim_module_pd_debounce_timer_hdlr(unsigned long unused);
static void sim_module_poll_sim1(void);



/******************************************************************************
* Local variables
******************************************************************************/
static int sim_module_major;

static BOOLEAN sim_module_pd_debounced = 0;
static BOOLEAN sim_module_debouncing_simpd = 0;
static BOOLEAN sim_module_all_tx_data_sent = 0;

static UINT32  sim_module_remaining_cwtcnt[NUM_SIM_MODULES];
static UINT32  sim_module_remaining_gpcnt[NUM_SIM_MODULES];
static UINT32  sim_module_rx_event = 0;

static UINT8   sim_module_current_pd_state_sim1 = 1;
static UINT8   sim_module_nack_counter = 0;
static UINT8   sim_module_opens = 0;
static UINT8   sim_module_pd_event = 0;
static UINT8   sim_module_pd_event_debounced = 0;
static UINT8   sim_module_pd_timer_exp = 0;

static struct timer_list sim_module_pd_debounce_timer;

static wait_queue_head_t sim_module_debounce_wait = __WAIT_QUEUE_HEAD_INITIALIZER(sim_module_debounce_wait);
static wait_queue_head_t sim_module_wait = __WAIT_QUEUE_HEAD_INITIALIZER(sim_module_wait);

static spinlock_t sim_module_lock = SPIN_LOCK_UNLOCKED;

static SIM_MODULE_INTERRUPT_MODE sim_module_interrupt_mode = SIM_MODULE_RX_MODE;
static SIM_MODULE_BUFFER_DATA sim_module_card_data[NUM_SIM_MODULES];

static void (* sim_module_interrupt_ptr) (UINT8 reader_id) = sim_module_int_rx;

static volatile  UINT32 * sim_module_base[NUM_SIM_MODULES] =
{
    ((volatile UINT32 *)IO_ADDRESS(SIM1_BASE_ADDR))
#if (defined(CONFIG_ARCH_MXC91321))
    ,((volatile UINT32 *)IO_ADDRESS(SIM2_BASE_ADDR))
#endif
};

static volatile SIM_MODULE_REGISTER_BANK *sim_registers[NUM_SIM_MODULES] =
{
    ((volatile SIM_MODULE_REGISTER_BANK *)IO_ADDRESS(SIM1_BASE_ADDR))
#if (defined(CONFIG_ARCH_MXC91321))
     ,   ((volatile SIM_MODULE_REGISTER_BANK *)IO_ADDRESS(SIM2_BASE_ADDR))
#endif
};
/******************************************************************************
* Local Functions
******************************************************************************/

/* DESCRIPTION:
       The IOCTL handler for the SIM device
 
   INPUTS:
       inode       inode pointer
       file        file pointer
       cmd         the ioctl() command
       arg         the ioctl() argument

   OUTPUTS:
       Returns 0 if successful.

   IMPORTANT NOTES:
       None.   
*/

static int sim_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int status = 0;
    /*
      args_kernel[0] = module #
      args_kernel[1] = read register pointer
      args_kernel[2] = write register pointer
      args_kernel[3] = data (for register write) and userspace pointer (for register read)
      args_kernel[4] = 
    */
    UINT32 args_kernel[REG_ACCESS_ARG_LENGTH] = {0, 0, 0, 0, 0};
    volatile UINT32 * sim_module_register_ptr = NULL;
    UINT32 temp = 0;
    /*
      These IO control requests must only be used by the protocol layer in user space.
      Any deviation from this rule WILL cause problems.
    */
    
    switch(cmd)
    {
        case SIM_IOCTL_CONFIGURE_GPIO:
            if ((UINT8)arg == SIM_MODULE_1)
            {
#if (defined(CONFIG_ARCH_MXC91321))
                iomux_config_mux(PIN_SIM1_RST0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
                iomux_config_mux(PIN_SIM1_DATA0_RX_TX, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
                iomux_config_mux(PIN_SIM1_SVEN0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
                iomux_config_mux(PIN_SIM1_CLK0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
                iomux_config_mux(PIN_SIM1_SIMPD0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
#else
                iomux_config_mux(SP_SIM1_TRXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
                iomux_config_mux(SP_SIM1_RST_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
                iomux_config_mux(SP_SIM1_SVEN, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
                iomux_config_mux(SP_SIM1_CLK, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
                iomux_config_mux(SP_SIM1_PD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
#endif
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;
            }
            break;
        case SIM_IOCTL_SET_CWTCNT:
            copy_from_user(args_kernel, (UINT32 *) arg, (sizeof(UINT32) * 2));
            
            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                sim_module_remaining_cwtcnt[args_kernel[0]] = args_kernel[1];
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;
            }
            break;
        case SIM_IOCTL_SET_GPCNT:
            copy_from_user(args_kernel, (UINT32 *) arg, (sizeof(UINT32) * 2));
            
            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                sim_module_remaining_gpcnt[args_kernel[0]] = args_kernel[1];
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;
            }
            break;
        case SIM_IOCTL_GET_GPCNT:
            copy_from_user(args_kernel, (UINT32 *) arg, (sizeof(UINT32) * 2));

            temp = 0;
            
            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                temp = sim_module_remaining_gpcnt[args_kernel[0]];
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;
            }

            if(status != -EFAULT)
            {
                if(copy_to_user((UINT32 *)args_kernel[1], &temp, sizeof(UINT32)))
                {
                    tracemsg("Warning: failed to copy data to user-space while retrieving the remaining SIM GPCNT\n");
                    status = -EFAULT;
                }
            }
            break;
        case SIM_IOCTL_GET_CWTCNT:
            copy_from_user(args_kernel, (UINT32 *) arg, (sizeof(UINT32) * 2));

            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                temp = sim_module_remaining_cwtcnt[args_kernel[0]];
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;
            }

            if(status != -EFAULT)
            {
                if(copy_to_user((UINT32 *)args_kernel[1], &temp, sizeof(UINT32)))
                {
                    tracemsg("Warning: failed to copy data to user-space while retrieving remaining SIM CWTCNT\n");
                    status = -EFAULT;
                }
            }
            break;
        case SIM_IOCTL_READ_DATA_BUFFER:
            copy_from_user(args_kernel, (UINT32 *) arg, (sizeof(UINT32) * 4));

            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                tracemsg("buffer_index(read data buffer): %X offset: %X rx_length: %X\n", sim_module_card_data[args_kernel[0]].buffer_index, args_kernel[2], args_kernel[3]);

                if(copy_to_user((UINT8 *)args_kernel[1], (&(sim_module_card_data[args_kernel[0]].buffer[args_kernel[2]])), args_kernel[3]))
                {
                    tracemsg("Warning: failed to copy data to user-space while retrieving SIM card reader data for module 2\n");
                    status = -EFAULT;
                }
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;
            }
            break;
        case SIM_IOCTL_UPDATE_DATA_BUFFER:
            copy_from_user(args_kernel, (UINT32 *) arg, (sizeof(UINT32) * 3));

            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                copy_from_user(sim_module_card_data[args_kernel[0]].buffer, (UINT8 *)args_kernel[1], args_kernel[2]);
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;
            }
            break;
        case SIM_IOCTL_INTERRUPT_INIT:
            copy_from_user(args_kernel, (UINT32 *) arg, (sizeof(UINT32) * 3));
            
            if( (UINT8) args_kernel[0] == SIM_MODULE_1)
            {
                request_irq(INT_SIM_DATA, sim_module_int_data_1 , SA_INTERRUPT, SIM_DEV_NAME, 0);
                request_irq(INT_SIM_GENERAL, sim_module_int_irq_1 , SA_INTERRUPT, SIM_DEV_NAME, 0);
            }
            else 
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;
            }
            break;
        case SIM_IOCTL_INTERRUPT_MODE:
            tracemsg("Set SIM interrupt mode -> arg: %X\n", (UINT32)arg);
            switch((UINT32)arg)
            {
                case SIM_MODULE_RX_MODE:
                    sim_module_interrupt_ptr = sim_module_int_rx;
                    sim_module_interrupt_mode = SIM_MODULE_RX_MODE;
                    break;
                case SIM_MODULE_TX_MODE:
                    sim_module_interrupt_ptr = sim_module_int_tx;
                    sim_module_interrupt_mode = SIM_MODULE_TX_MODE;
                    break;
                case SIM_MODULE_RESET_DETECT_MODE:
                    sim_module_interrupt_ptr = sim_module_int_reset_detect;
                    sim_module_interrupt_mode = SIM_MODULE_RX_MODE;
                    break;
                default:
                    break;
            }
            break;
        case SIM_IOCTL_WRITE_SIM_REG_DATA:

            copy_from_user(args_kernel, (UINT32 *) arg, (REG_ACCESS_ARG_LENGTH * sizeof(UINT32)));

            tracemsg("Write SIM module register data -> reader ID: %X egister: %X data: %X\n", args_kernel[0], args_kernel[2], args_kernel[3]);

            if (args_kernel[2] <= sizeof(SIM_MODULE_REGISTER_BANK)) 
            {
                if (args_kernel[0] < NUM_SIM_MODULES)
                {
                    sim_module_register_ptr = (UINT32 *) ((UINT32)sim_module_base[args_kernel[0]] + (UINT32)args_kernel[2]);
                }
                else
                {
                    tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                    status = -EFAULT;
                }

                if (sim_module_register_ptr != NULL)
                {
                    *sim_module_register_ptr  = args_kernel[3];

                    /* if writing to the transmit FIFO increment the buffer index */
                    if(args_kernel[2] == 0x00000034)
                    {
                        sim_module_card_data[args_kernel[0]].buffer_index++;
                        tracemsg("Output Char: %X\n", args_kernel[3]);
                    }
                }
            }
            else
            {
                tracemsg("Warning: Invalid register address in the SIM driver.\n");
                status = -EFAULT; 
            }
            break;
        case SIM_IOCTL_READ_SIM_REG_DATA:

            copy_from_user(args_kernel, (UINT32 *) arg, (REG_ACCESS_ARG_LENGTH * sizeof(UINT32)));
            tracemsg("Read SIM register data -> Reader ID: %X, register: %X \n", args_kernel[0], args_kernel[1]);
            if (args_kernel[1] <= sizeof(SIM_MODULE_REGISTER_BANK)) 
            {
                if (args_kernel[0] < NUM_SIM_MODULES)
                {
                    sim_module_register_ptr = (UINT32 *) ((UINT32)sim_module_base[args_kernel[0]] + (UINT32)args_kernel[1]);
                }
                else
                {
                    tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                    status = -EFAULT;
                }

                if(sim_module_register_ptr != NULL)
                {
                    args_kernel[3] = *sim_module_register_ptr;
                    
                    if(copy_to_user((UINT32 *)arg, args_kernel, sizeof(UINT32) * REG_ACCESS_ARG_LENGTH))
                    {
                        tracemsg("Warning: failed to copy data to user-space while reading SIM module register data.\n");
                        status = -EFAULT;
                    }
                }
            }
            else
            {
                tracemsg("Warning: Invalid register address in the SIM driver.\n");
                status = -EFAULT; 
            }
            break;
        case SIM_IOCTL_WRITE_SIM_REG_BIT:
            copy_from_user(args_kernel, (UINT32 *) arg, (REG_ACCESS_ARG_LENGTH * sizeof(UINT32)));
            
            tracemsg("Write SIM register bits -> reader ID: %X register: %X bits: %X mask: %X\n",
                     args_kernel[0], args_kernel[2], args_kernel[3], args_kernel[1]);
            
            if (args_kernel[2] <= sizeof(SIM_MODULE_REGISTER_BANK)) 
            {
                if (args_kernel[0] < NUM_SIM_MODULES)
                {
                    sim_module_register_ptr = (UINT32 *) ((UINT32)sim_module_base[args_kernel[0]] + (UINT32)args_kernel[2]);
                }
                else
                {
                    tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                    status = -EFAULT;
                }

                if (sim_module_register_ptr != NULL)
                {
                    temp = *(sim_module_register_ptr);
                    temp = (temp & ~(args_kernel[1])) | (args_kernel[3]);
                    *(sim_module_register_ptr) = temp;
                    
                    /* if writing to the transmit FIFO increment the buffer index */
                    if(args_kernel[2] == 0x00000034)
                    {
                        sim_module_card_data[args_kernel[0]].buffer_index++;
                        tracemsg("Output Char: %X\n", args_kernel[3]);
                    }
                }
            }
            else
            {
                tracemsg("Warning: Invalid register address in the SIM driver.\n");
                status = -EFAULT; 
            }
            break;    
        case SIM_IOCTL_READ_PERIPHERAL_CLOCK_FREQ:
            copy_from_user(args_kernel, (UINT32 *) arg, sizeof(UINT32));
            if (args_kernel[0] == SIM_MODULE_1)
            {
                args_kernel[0] = mxc_get_clocks(SIM1_CLK);
            }
#ifdef CONFIG_ARCH_MXC91321
            else if (args_kernel[0] == SIM_MODULE_2)
            {
                args_kernel[0] = mxc_get_clocks(SIM2_CLK);
            }
#endif
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;       
            }
            if (status != -EFAULT)
            {
                if(copy_to_user((UINT32 *)arg, args_kernel, sizeof(UINT32)))
                {
                    tracemsg("Warning: failed to copy data to user-space while returning the SIM peripheral clock frequency.\n");
                    status = -EFAULT;
                }
            }
            break;
        case SIM_IOCTL_READ_RX_EVENT:
            if(copy_to_user((UINT32 *)arg, &sim_module_rx_event, sizeof(UINT32)))
            {
                tracemsg("Warning: failed to copy data to user-space while reading SIM RX event type.\n");
                status = -EFAULT;
            }
            sim_module_rx_event = 0;
            break;    
        case SIM_IOCTL_READ_SIMPD_EVENT:
            tracemsg("A SIMPD event occured\n");
            
            if(put_user(sim_module_pd_event_debounced, (UINT8 *)arg) != 0)
            {
                tracemsg("Warning: failed to copy data to user-space while reading SIM PD event.\n");
                status = -EFAULT;
            }
            sim_module_pd_event_debounced = 0;
            break;
        case SIM_IOCTL_SET_VOLTAGE_LEVEL:
            copy_from_user(args_kernel, (UINT32 *) arg, (sizeof(UINT32) * 2));
            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                power_ic_periph_set_sim_voltage(args_kernel[0], args_kernel[1]);
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;
            }
            break;
        case SIM_IOCTL_READ_CARD_READER_DATA:
            copy_from_user(args_kernel, (UINT32 *) arg, sizeof(UINT32));

            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                args_kernel[1] = sim_module_card_data[args_kernel[0]].buffer_index;
                args_kernel[2] = sim_module_card_data[args_kernel[0]].rx_last_index;
                args_kernel[3] = sim_module_card_data[args_kernel[0]].tx_length;
                args_kernel[4] = 0;
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT; 
            }
            
            tracemsg("read card reader data -> buffer_index: %X rx_last_index: %X tx_length: %X error_flag: %X\n",
                     args_kernel[1], args_kernel[2], args_kernel[3], args_kernel[4]);
            
            if (arg != 0)
            {
                if (copy_to_user((UINT32 *)arg, args_kernel, sizeof(UINT32) * REG_ACCESS_ARG_LENGTH))
                {
                    tracemsg("Warning: failed to copy data to user-space during card reader data retrieval\n");
                    status = -EFAULT;
                }
            }
            else
            {
                tracemsg("Warning: null pointer passed into the SIM driver for card reader data retrieval\n");
                status = -EFAULT;
            }
            break;
        case SIM_IOCTL_RESET_CARD_READER_DATA:
            copy_from_user(args_kernel, (UINT32 *)arg, (sizeof(UINT32) * REG_ACCESS_ARG_LENGTH));

            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                sim_module_card_data[args_kernel[0]].buffer_index  = args_kernel[1];
                sim_module_card_data[args_kernel[0]].rx_last_index = args_kernel[2];
                sim_module_card_data[args_kernel[0]].tx_length     = args_kernel[3];
                sim_module_card_data[args_kernel[0]].error_flag    = args_kernel[4];
            }       
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;                 
            }
            
            tracemsg("reset card reader data -> buffer_index: %X rx_last_index: %X tx_length: %X error_flag: %X\n",
                     args_kernel[1], args_kernel[2], args_kernel[3], args_kernel[4]);
            break;
        case SIM_IOCTL_UPDATE_RX_LAST_INDEX:
            copy_from_user(args_kernel, (UINT32 *)arg, (sizeof(UINT32) * 2));

            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                sim_module_card_data[args_kernel[0]].rx_last_index = args_kernel[1];
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;       
            }
            break;
        case SIM_IOCTL_UPDATE_BUFFER_INDEX:
            copy_from_user(args_kernel, (UINT32 *)arg, (sizeof(UINT32) * 2));

            if (args_kernel[0] < NUM_SIM_MODULES)
            {
                sim_module_card_data[args_kernel[0]].buffer_index = args_kernel[1];
            }
            else
            {
                tracemsg("Warning: Invalid reader ID in SIM driver request.\n");
                status = -EFAULT;       
            }
            break;
        case SIM_IOCTL_ALL_TX_DATA_SENT:
            if (arg != 0)
            {
                if (copy_to_user((BOOLEAN *)arg, &sim_module_all_tx_data_sent, sizeof(BOOLEAN)))
                {
                    tracemsg("Warning: failed to copy data to user-space during card reader data retrieval\n");
                    status = -EFAULT;
                }

            }
            else
            {
                tracemsg("Warning: null pointer passed in for SIM_ALL_TX_DATA_SENT IOCTL.\n");
                status = -EFAULT;
            }
            break;
        case SIM_IOCTL_RESET_ALL_TX_DATA_SENT:
            sim_module_all_tx_data_sent = FALSE;
            break;
            
        default:
            tracemsg("Warning: Invalid request sent to the SIM driver.\n");
            status = -ENOTTY;
            break;
    }
    
    return status;
}

/* DESCRIPTION:
       The open() handler for the SIM device.
 
   INPUTS:
       inode       inode pointer
       file        file pointer

   OUTPUTS:
       Returns 0 if successful.

   IMPORTANT NOTES:
       None.   
*/
static int sim_open(struct inode *inode, struct file *file) 
{
    spin_lock(&sim_module_lock);

    /* only allow 1 open */
    if (sim_module_opens > 0)
    {
        spin_unlock(&sim_module_lock);

        return -ENODEV;
    }
    
    sim_module_opens++;
    spin_unlock(&sim_module_lock);
    
    tracemsg("sim : sim_open()\n");
    
    spba_take_ownership(SPBA_SIM1, SPBA_MASTER_A);
#if (defined(CONFIG_ARCH_MXC91321))
    spba_take_ownership(SPBA_SIM2, SPBA_MASTER_A);
#endif  
    return 0;
}

/* DESCRIPTION:
       The close() handler for the SIM device
 
   INPUTS:
       inode       inode pointer
       file        file pointer

   OUTPUTS:
       Returns 0 if successful.

   IMPORTANT NOTES:
       None.   
*/
static int sim_free(struct inode *inode, struct file *file)
{
        tracemsg("sim : sim_free()\n");
        sim_module_opens--;
        return 0;
}

/*This structure defines the file operations for the SIM device */
static struct file_operations sim_fops =
{
    .owner =    THIS_MODULE,
    .ioctl =    sim_ioctl,
    .open =     sim_open,
    .release =  sim_free,
    .poll =     sim_poll,
};

/*

DESCRIPTION:
    This function is the interrupt handler SIM receive data.

INPUTS:
    UINT8 reader_id: the card reader ID of the interrupt to handle

OUTPUT:
    None

IMPORTANT NOTES:
    None

*/
static void sim_module_int_rx (UINT8 reader_id)
{
    UINT16              read_data;
    UINT16              bytes_remaining;
    UINT16              rx_threshold;

    /* default the RX threshold */
    rx_threshold = 1;

    /* mask the RX FIFO full interrupt */
    sim_registers[reader_id]->int_mask |= RIM;

    /* while there is data in the FIFO ... */
    while ((sim_registers[reader_id]->rcv_status & RFD) == RFD)
    {
        /* read a character from the FIFO */
        read_data = sim_registers[reader_id]->port0_rcv_buf & (FE0 | PE0 | RCV_MASK0);

        /* if the byte was received without a parity error ... */
        if ((read_data & PE0) == 0)
        {
            /* copy the byte into the card data buffer */
            sim_module_card_data[reader_id].buffer
                [sim_module_card_data[reader_id].buffer_index++] = (read_data & RCV_MASK0);

            /* reset the NACK counter */
            sim_module_nack_counter = 0;

            
        }
        /* else (a parity error occurred) ... */
        else
        {
            /* IMPORTANT NOTE: THIS MUST BE COMPILED IN WHEN T=1 SUPPORT IS REQUIRED.
               FOR T=1, THE BYTE NEEDS TO BE STORED EVEN IF A PARITY ERROR OCCURED.
               ALSO, NEED A METHOD TO RETREIVE PROTOCOL TYPE AND ATR STATUS FROM
               USERSPACE.
            */
#if 0
            /* copy the byte into the card data buffer */
            sim_module_card_data[reader_id].buffer
                [sim_module_card_data[reader_id].buffer_index++] = (read_data & RCV_MASK0);

            sim_module_rx_event |= SIM_MODULE_EVENT_PARITY_ERROR;
            wake_up_interruptible(&sim_module_wait);
            continue;
#endif
        }
        
    }

    /* if there is still additional data to receive */
    if ((sim_module_card_data[reader_id].rx_last_index >
         sim_module_card_data[reader_id].buffer_index))
    {
        /* determine the amount of data remaining */
        bytes_remaining = (sim_module_card_data[reader_id].rx_last_index -
            sim_module_card_data[reader_id].buffer_index);

        /* if the bytes remaining exceed the FIFO size ... */
        if (bytes_remaining >= SIM_MODULE_RX_FIFO_SIZE)
        {
            /* set the threshold level to the maximum FIFO size */
            rx_threshold = SIM_MODULE_RX_FIFO_SIZE;
        }

        /* else (the bytes remaining will fit in the FIFO) ... */
        else
        {
            /* set the threshold level to the remaining data size */
            rx_threshold = bytes_remaining;
        }
    }

    /* set the RX FIFO threshold */
    sim_registers[reader_id]->rcv_threshold = ((sim_registers[reader_id]->rcv_threshold & ~RDT_MASK) | (RDT_MASK & rx_threshold));

    /* unmask the RX FIFO full interrupt */
    sim_registers[reader_id]->int_mask &= ~RIM;

    /* notify user space that data was received */
    sim_module_rx_event |= SIM_MODULE_EVENT_RX_A;
    wake_up_interruptible(&sim_module_wait);  
    
    return;
}

/*

DESCRIPTION:
    This function handles the SIM TX interrupt.  In cases where the TX data exceeds the TX FIFO
    length, it refills the TX FIFO on TX threshold interrupts.  On TX complete, it sets up GPCNT and
    switches the system into receive mode.

INPUTS:
    UINT8 reader_id: the card reader ID of the interrupt to handle

OUTPUT:
    None

IMPORTANT NOTES:
    None

*/
static void sim_module_int_tx (UINT8 reader_id)
{
    BOOL   unmask_threshold;
    UINT8  tx_index;
    UINT16 bytes_remaining;

    /* if the early transmit complete interrupt has fired ... */
    if (((sim_registers[reader_id]->int_mask & ETCIM) == 0) &&
        ((sim_registers[reader_id]->xmt_status & ETC) == ETC))
    {
        /* setup GPCNT */
        sim_registers[reader_id]->cntl &= ~GPCNT_CLK_SEL_MASK;
        sim_registers[reader_id]->cntl |= GPCNT_CLK_ETU << GPCNT_CLK_SEL_SHIFT;
        sim_registers[reader_id]->int_mask &= ~GPCNTM;

        /* clear the early transmit complete interrupt */
        sim_registers[reader_id]->xmt_status = ETC;
        sim_registers[reader_id]->int_mask |= ETCIM;

        /* reset the data buffer index */
        sim_module_card_data[reader_id].buffer_index = 0;

        /* setup the hardware for receive */
        sim_module_init_rx_mode (reader_id);

        sim_module_all_tx_data_sent = TRUE;

    }

    /* if the TX FIFO threshold interrupt has fired ... */
    if (((sim_registers[reader_id]->int_mask & TDTFM) == 0) &&
        ((sim_registers[reader_id]->xmt_status & TDTF) == TDTF))
    {
        /* determine remaining number of bytes to transmit */
        bytes_remaining = sim_module_card_data[reader_id].tx_length - 
            sim_module_card_data[reader_id].buffer_index;

        /* disable the TX FIFO threshold interrupt */
        sim_registers[reader_id]->int_mask |= TDTFM;
      
        /* if the data to be transmitted exceeds the free TX FIFO size ...*/
        if (bytes_remaining > SIM_MODULE_TX_FIFO_FREE)
        {
            /* only transmit the free TX FIFO size */
            bytes_remaining = SIM_MODULE_TX_FIFO_FREE;

            /* indicate the TX FIFO threshold interrupt should be unmasked */
            unmask_threshold = TRUE;
        }
        else
        {
            /* clear the TX threshold level */
            sim_registers[reader_id]->xmt_threshold &= ~TDT_MASK;

            /* indicate the early transmit complete interrupt should be unmasked */
            unmask_threshold = FALSE;
        }
    
        /* for each byte of data to transmit ... */
        for (tx_index = 0; tx_index < bytes_remaining; tx_index++)
        {
            /* write the character to the TX FIFO */
            sim_registers[reader_id]->port0_xmt_buf = sim_module_card_data[reader_id].
                buffer[sim_module_card_data[reader_id].buffer_index++];

        }

        if(bytes_remaining > 0)
        {
            sim_registers[reader_id]->xmt_status = (TC | ETC);
        }

        /* clear the transmit FIFO threshold interrupt */
        sim_registers[reader_id]->xmt_status = TDTF;

        /* if unmasking the TX FIFO threshold interrupt ... */
        if (unmask_threshold == TRUE)
        {
            /* unmask the TX FIFO threshold interrupt */
            sim_registers[reader_id]->int_mask &= ~TDTFM;
        }

        /* else (unmasking the early transmit complete interrupt) ... */
        else
        {
            /* unmask the TX FIFO threshold interrupt */
            sim_registers[reader_id]->int_mask &= ~ETCIM;
        }

        sim_module_rx_event |= SIM_MODULE_EVENT_BUFFER_INDEX;
        wake_up_interruptible(&sim_module_wait);        
    }
    
    
    return;
}

/*

DESCRIPTION:
    This function is the interrupt handler for the SIM Reset Detect Mode.  This function only reads
    data and copies it into the receive buffer without checking for parity errors.  The purpose is
    to advance the buffer_index variable for the detection of an incoming ATR message which
    indicates that the SIM card has reset.  An ATR message is the only thing which should be
    received (and then only on SIM card resets) when this interrupt handler is active.

INPUTS:
    UINT8 reader_id: the card reader ID of the interrupt to handle

OUTPUT:
    None

IMPORTANT NOTES:
    None

*/
static void sim_module_int_reset_detect (UINT8 reader_id)
{
    /* mask the RX FIFO full interrupt */
    sim_registers[reader_id]->int_mask |= RIM;

    /* while there is data in the FIFO ... */
    while ((sim_registers[reader_id]->rcv_status & RFD) == RFD)
    {
        /* copy the character into the data buffer */
        sim_module_card_data[reader_id].buffer[sim_module_card_data[reader_id].buffer_index++]
            = sim_registers[reader_id]->port0_rcv_buf & RCV_MASK0;

    }

    sim_module_rx_event |= SIM_MODULE_EVENT_BUFFER_INDEX;
    wake_up_interruptible(&sim_module_wait);

    return;
}


/*

DESCRIPTION:
    This function is the interrupt service routine for the SIM data interrupt for the smart card
    interface module 1.

INPUTS:
    None

OUTPUT:
    None

IMPORTANT NOTES:
    None

*/
static irqreturn_t sim_module_int_data_1 (int irq, void *dev_id, struct pt_regs *regs)
{
    /* if the receiver FIFO full, early TX complete, or TX FIFO full interrupts have fired ... */
    if ((((sim_registers[SIM_MODULE_1]->int_mask & RIM) == 0) &&
         ((sim_registers[SIM_MODULE_1]->rcv_status & RDRF) == RDRF)) ||
        ((sim_module_interrupt_mode == SIM_MODULE_TX_MODE) &&
         (((sim_registers[SIM_MODULE_1]->xmt_status & ETC) == ETC) ||
          ((sim_registers[SIM_MODULE_1]->xmt_status & TDTF) == TDTF))))
    {
        mpm_handle_ioi();
        /* execute the interrupt handler */
        (*sim_module_interrupt_ptr) (SIM_MODULE_1);
    }

    return(IRQ_RETVAL(1));
}

/*

DESCRIPTION:
    This function is the interrupt service routine that handles non-data interrupts of smart card
    interface module 1.

INPUTS:
    None

OUTPUT:
    None

IMPORTANT NOTES:
    None

*/
static irqreturn_t sim_module_int_irq_1 (int irq, void *dev_id, struct pt_regs *regs)
{
    mpm_handle_ioi();

    /* if the PD interrupt fired ... */
    if  (((sim_registers[SIM_MODULE_1]->port0_detect & SDIM0) == 0) &&
        ((sim_registers[SIM_MODULE_1]->port0_detect & SDI0) == SDI0))
    {
        /* disable the PD interrupt */
        sim_registers[SIM_MODULE_1]->port0_detect |= SDIM0;
        
        /* clear the PD event */
        sim_registers[SIM_MODULE_1]->port0_detect |= SDI0;

        if( sim_module_debouncing_simpd == 0 )
        {
            /* Wake up the debouncing thread */
            sim_module_pd_event = 1;
            wake_up(&sim_module_debounce_wait);   
        }
    }

    /* else (some other interrupt has fired) ... */
    else
    {
        /* if the TNACK threshold interrupt fired ... */
        if (((sim_registers[SIM_MODULE_1]->int_mask & XTM) == 0) &&
            ((sim_registers[SIM_MODULE_1]->xmt_status & XTE) == XTE))
        {
            /* clear the TNACK event */
            sim_registers[SIM_MODULE_1]->xmt_status = XTE;

            /* indicate a fatal error */
            sim_module_card_data[SIM_MODULE_1].error_flag = ISR_FATAL_ERROR;

            sim_module_rx_event |= SIM_MODULE_EVENT_ERROR_FLAG;
            wake_up_interruptible(&sim_module_wait); 
        }
        
        /* if the overrun error interrupt fired ... */
        if (((sim_registers[SIM_MODULE_1]->int_mask & OIM) == 0) &&
            ((sim_registers[SIM_MODULE_1]->rcv_status & OEF) == OEF))
        {
            /* clear the overrun event */
            sim_registers[SIM_MODULE_1]->rcv_status = OEF;
        }

        /* if the GPCNT interrupt fired ... */
        if (((sim_registers[SIM_MODULE_1]->int_mask & GPCNTM) == 0) &&
            ((sim_registers[SIM_MODULE_1]->xmt_status & GPCNT) == GPCNT))
        {
            if (sim_module_remaining_gpcnt[SIM_MODULE_1] != 0)
            {
                sim_module_gpcnt_rollover (SIM_MODULE_1);
            }
            else
            {
                /* disable the GPCNT interrupt */
                sim_registers[SIM_MODULE_1]->int_mask |= GPCNTM;

                /* notify user space */
                sim_module_rx_event |= SIM_MODULE_EVENT_RX_A;
                wake_up_interruptible(&sim_module_wait);
            }
        }

        /* if the CWT interrupt fired ... */
        if (((sim_registers[SIM_MODULE_1]->int_mask & CWTM) == 0) &&
            ((sim_registers[SIM_MODULE_1]->rcv_status & CWT) == CWT))
        {
            if (sim_module_remaining_cwtcnt[SIM_MODULE_1] != 0)                
            {
                /* It is not a real CWT timeout (CWT requires more than 16 bit) */
                sim_module_cwt_rollover(SIM_MODULE_1);
            }
            else
            {
                /* disable the CWT interrupt */
                sim_registers[SIM_MODULE_1]->int_mask |= CWTM;

                /* notify user space */
                sim_module_rx_event |= SIM_MODULE_EVENT_RX_A;
                wake_up_interruptible(&sim_module_wait);
            }
        }
    }

    return(IRQ_RETVAL(1));
}

/*



DESCRIPTION:
    This routine sets up the character wait timer following expiration of the timer when additional
    time remains due to the original timer value exceeding the maximum that may be programmed into
    the hardware.

INPUTS:
    UINT8 - reader ID

OUTPUT:
    None

IMPORTANT NOTES:
    None

*/
static void sim_module_cwt_rollover (UINT8 reader_id)
{
    UINT32 time; /* the CWT value to be programmed */

    /* if the reader ID is valid ... */
    if (reader_id < NUM_SIM_MODULES)
    {
        /* if the remaining still exceeds the maximum that can be programmed ... */
        if (sim_module_remaining_cwtcnt[reader_id] > MAX_CWTCNT_VALUE)
        {
            /* program the maximum value and decrement the maximum value from the time remaining */
            time = MAX_CWTCNT_VALUE;
            sim_module_remaining_cwtcnt[reader_id] -= MAX_CWTCNT_VALUE;
        }

        /* else (the remaining time can be directly programmed) ... */
        else
        {
            /* program the actual value and set the time remaining to 0 */
            time = sim_module_remaining_cwtcnt[reader_id];
            sim_module_remaining_cwtcnt[reader_id] = 0;
        }

        /* setup the CWT timer and interrupt */
        sim_registers[reader_id]->char_wait = time;
        sim_registers[reader_id]->rcv_status = CWT;
    }                           

    return;
}

/*

DESCRIPTION:
    This routine sets up the general purpose timer following expiration of the timer when additional
    time remains due to the original timer value exceeding the maximum that may be programmed into
    the hardware.

INPUTS:
    UINT8           reader_id: the card reader ID for which to setup GPCNT
    SIM_MODULE_IC_GPCNT_CLK_SOURCE source:    the source of the GPCNT timer

OUTPUT:
    None

IMPORTANT NOTES:
    None

*/
static void sim_module_gpcnt_rollover (UINT8 reader_id)
{
    UINT32 reg_source; /* register value to set the clock source */
    UINT32 time;       /* the GPCNT value to be programmed       */

    /* if the reader ID is valid ... */
    if (reader_id < NUM_SIM_MODULES)
    {
        /* if the remaining still exceeds the maximum that can be programmed ... */
        if (sim_module_remaining_gpcnt[reader_id] > MAX_GPCNT_VALUE)
        {
            /* program the maximum value and decrement the maximum value from the time remaining */
            time = MAX_GPCNT_VALUE;
            sim_module_remaining_gpcnt[reader_id] -= MAX_GPCNT_VALUE;
        }

        /* else (the remaining time can be directly programmed) ... */
        else
        {
            /* program the actual value and set the time remaining to 0 */
            time = sim_module_remaining_gpcnt[reader_id];
            sim_module_remaining_gpcnt[reader_id] = 0;
        }

        /* save the current clock source */
        reg_source = (sim_registers[reader_id]->cntl & GPCNT_CLK_SEL_MASK);

        /* setup the GPCNT timer and interrupt */
        sim_registers[reader_id]->gpcnt = time;
        sim_registers[reader_id]->xmt_status = GPCNT;
        sim_registers[reader_id]->cntl &= ~GPCNT_CLK_SEL_MASK;
        sim_registers[reader_id]->cntl |= (GPCNT_CLK_SEL_MASK & reg_source);
        sim_registers[reader_id]->int_mask &= ~GPCNTM;
    }

    return;
}

/*

DESCRIPTION:
    This routine sets up the hardware and interrupt system to receive data.

INPUTS:
    UINT8 card_reader_id: card reader ID for which to setup

OUTPUT:
    None

IMPORTANT NOTES:
    None
*/
static void sim_module_init_rx_mode (UINT8 reader_id)
{
    /* if the reader ID is valid ... */
    if (reader_id < NUM_SIM_MODULES)
    {
        /* indicate the card is in the non-ATR receive mode */
        sim_module_interrupt_mode = SIM_MODULE_RX_MODE;
        sim_module_interrupt_ptr = sim_module_int_rx;

        /* enable parity and overrun NACK'ing */
        sim_registers[reader_id]->cntl |= (ANACK | ONACK);

        /* disable 11 ETUs per character mode */
        sim_registers[reader_id]->guard_cntl &= ~RCVR11;
        
        /* mask the transmit complete and TNACK error interrupts */
        sim_registers[reader_id]->int_mask |= (TCIM | XTM);

        /* disable initial character mode */
        sim_registers[reader_id]->cntl &= ~ICM;

        /* set the RX FIFO threshold to 1 */
        sim_registers[reader_id]->rcv_threshold = ((sim_registers[reader_id]->rcv_threshold & ~RDT_MASK) | (RDT_MASK & 1));

        /* enable the receiver */
        sim_registers[reader_id]->enable |= RCV_EN;

        /* unmask the RX full interrupt */
        sim_registers[reader_id]->int_mask &= ~RIM;

    }

    return;
}
/******************************************************************************
* Local functions
******************************************************************************/

/* DESCRIPTION:
       This function is the debouncing thread for SIMPD.
 
   INPUTS:
       None.  

   OUTPUTS:
       Returns the status of SIMPD - 0 SIM removed
                                     1 SIM inserted

   IMPORTANT NOTES:
       None.
*/
static int sim_module_debounce_simpd(void * unused)
{
    int error = 0;
    /* Give the thread a higher priority */
    struct sched_param thread_sched_params = {.sched_priority = (MAX_RT_PRIO - 4)};

    sim_module_pd_event = 0;
    
    lock_kernel();
    daemonize("ksim_module_debounce_simpd\n");
    reparent_to_init();
    siginitsetinv(&current->blocked, sigmask(SIGKILL)|sigmask(SIGINT)|sigmask(SIGTERM));
    unlock_kernel();
    
    error = sched_setscheduler(current, SCHED_FIFO, &thread_sched_params);

    /* Initialize the debounce timer */
    init_timer(&sim_module_pd_debounce_timer);
    sim_module_pd_debounce_timer.data = 0;
    sim_module_pd_debounce_timer.function = sim_module_pd_debounce_timer_hdlr;

    
    if(error == 0)
    {
        /* Allow signals to be detected */
        while (!signal_pending(current))
        {
                        
            /* Sleep until a SIM PD status change needs to be debounced */
            (void)wait_event_interruptible (sim_module_debounce_wait, (sim_module_pd_event == 1));
            
            if( sim_module_pd_event == 1 )
            {
                sim_module_debouncing_simpd = 1;
            
                /* Loop while debouncing SIMPD */
                while (sim_module_pd_debounced == 0)
                {
                    sim_module_poll_sim1();

                    if( sim_module_pd_debounced == 0)
                    {
                        sim_module_pd_debounce_timer.expires = jiffies + SIMPD_POLL_INTERVAL_JIFFIES;
                        add_timer(&sim_module_pd_debounce_timer);

                        /* Sleep until the timer expires then start the next iteration */
                        (void)wait_event_interruptible(sim_module_debounce_wait, (sim_module_pd_timer_exp == 1));
                    }
                }

                /* Initialize variables before the next debounce*/
                sim_module_pd_event = 0;
                sim_module_pd_debounced = 0;
                sim_module_pd_timer_exp = 0;
                sim_module_debouncing_simpd = 0;
            }
        } 
    }
    else
    {
        tracemsg("Error setting thread priority.\n");
    }
    
    del_timer(&sim_module_pd_debounce_timer);
    
    return error;
}


/* DESCRIPTION:
       This function will poll sim pd for sim 1.
 
   INPUTS:
       None.  

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       
*/
static void sim_module_poll_sim1(void)
{
    static UINT8 num_of_matching_sim1_pd_samples = 0;
    static BOOL previous_sim1_pd_sample = 0;
    static BOOL previous_sim1_state = 0;
    BOOL sim1_pd_sample;

    /* If we are entering the debounce routine for the first time, and a sim is 
       currently attached, contact bounce or a removal has been detected.
       Save the previous state of the sim.
    */
    if(num_of_matching_sim1_pd_samples == 0)
    {
        if (sim_module_current_pd_state_sim1 == 1)
        {
            previous_sim1_state = 1;
        }
    }

    /* Read the state of sim1 pd */
    sim1_pd_sample = !((sim_registers[SIM_MODULE_1]->port0_detect & SPDP0) == SPDP0);

    /* If the sample taken is the same as the previous sample, then increment the 
       matching sample counter.
    */
    if(sim1_pd_sample == previous_sim1_pd_sample)
    {
        num_of_matching_sim1_pd_samples++;
    }
    /* Otherwise the sample is different, so set the previous sample equal to the 
       current sample, and set the matching sample counter to one.
    */
    else
    {
       previous_sim1_pd_sample = sim1_pd_sample;
       num_of_matching_sim1_pd_samples = 1;
    }

    /* If there are eight identical samples, sim pd is debounced */
    if(num_of_matching_sim1_pd_samples == SIMPD_POLL_COUNT)
    {
        if(previous_sim1_state == sim1_pd_sample)
        {
            sim_module_rx_event |= SIM_MODULE_EVENT_SIMPD_BOUNCE;
            wake_up(&sim_module_wait);

        }
        else
        {
            sim_module_current_pd_state_sim1 = sim1_pd_sample;
            sim_registers[SIM_MODULE_1]->port0_detect &= ~SDIM0;
            
            if(sim1_pd_sample == 1)
            {
                sim_module_pd_event_debounced = SIM_MODULE_EVENT_SIMPD_INSERTION;
                sim_registers[SIM_MODULE_1]->port0_detect |= SPDS0;
            }
            else
            {
                sim_module_pd_event_debounced = SIM_MODULE_EVENT_SIMPD_REMOVAL;
                sim_registers[SIM_MODULE_1]->port0_detect &= ~SPDS0;
            }
            
            wake_up(&sim_module_wait);
        }
        
        /* Set the matching sample counter to zero for the next debounce and disable 
           polling. Also save the sim state and enable the sim interrupt.
        */
        num_of_matching_sim1_pd_samples = 0;
        previous_sim1_state = sim1_pd_sample;

        sim_module_pd_debounced = 1;
    }

    return;
}

/* DESCRIPTION:
       This function handles timer expiration.
 
   INPUTS:
       unsigned long unused: There is only 1 event that needs debouncing so the parameter is unused..  

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       None.   
*/
static void sim_module_pd_debounce_timer_hdlr(unsigned long unused)
{
    sim_module_pd_timer_exp = 1;
    wake_up(&sim_module_debounce_wait);
    
    return;
}

/* DESCRIPTION:
       The SIM intialization function.
 
   INPUTS:
       None.

   OUTPUTS:
       Returns the SIM major number.

   IMPORTANT NOTES:
       None.   
*/
int __init sim_init(void)
{
    
    sim_module_major = register_chrdev(0,SIM_DEV_NAME, &sim_fops);
    
    devfs_mk_cdev(MKDEV(sim_module_major,0), S_IFCHR | S_IRUGO | S_IWUSR, SIM_DEV_NAME);

    /* Initialize the SIM interface module. */
    if(sim_module_major < 0)
    {
       tracemsg("Unable to get a major for the SIM driver.\n");

    }

    kernel_thread (sim_module_debounce_simpd, NULL, 0);

    return sim_module_major;
}



/* DESCRIPTION:
       The SIM device cleanup function
 
   INPUTS:
       None. 

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       None.   
*/
static void __exit sim_exit(void)
{
    spba_rel_ownership(SPBA_SIM1, SPBA_MASTER_A);
#if (defined(CONFIG_ARCH_MXC91321))
    spba_rel_ownership(SPBA_SIM2, SPBA_MASTER_A);
#endif
    unregister_chrdev(sim_module_major, SIM_DEV_NAME);
    tracemsg("SIM driver successfully unloaded\n");
}

/* DESCRIPTION:
       The poll() handler for the SIM driver
 
   INPUTS:
       file        file pointer
       wait        poll table for this poll()

   OUTPUTS:
       Returns 0 if no data to read or POLLIN|POLLRDNORM if data available.

   IMPORTANT NOTES:
       None.   
*/
static unsigned int sim_poll(struct file *file, poll_table *wait)
{
    unsigned int retval = 0;

    /* Add our wait queue to the poll table */
    poll_wait(file, &sim_module_wait, wait);

    if(sim_module_rx_event != 0)
    {
        retval = POLLIN | POLLRDNORM;
    }
    
    return retval;
}

/*
 * Module entry points
 */
module_init(sim_init);
module_exit(sim_exit);

MODULE_DESCRIPTION("SIM driver");
MODULE_AUTHOR("Motorola");
MODULE_LICENSE("GPL");

