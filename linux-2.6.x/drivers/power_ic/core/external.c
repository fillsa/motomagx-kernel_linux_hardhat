/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2004-2008 Motorola, Inc.
 * 
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Motorola 2008-May-15 - Do not store AHSREN and AHLREN in RAM
 * Motorola 2007-Jun-20 - Do not write when read for Power Gate SPI Enable bits.
 * Motorola 2007-Jun-19 - NAND secure boot time improvement 
 * Motorola 2007-Mar-14 - Do not read before write when writing to interrupt status regs.
 * Motorola 2007-Feb-19 - Update include
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Nov-07 - Fix Klocwork Warnings
 * Motorola 2006-Oct-31 - Add semaphore/mutex protection
 * Motorola 2006-Oct-06 - Update File
 * Motorola 2006-Sep-06 - Implement no_write_when_read masks.
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-11 - Make power_ic_set_reg_mask() smarter for Power Gate settings
 * Motorola 2005-Aug-24 - Finalize the interface.
 * Motorola 2004-Dec-06 - Rewrote the kernel-level interface to access power IC registers.
 */

/*!
 * @file external.c
 *
 * @ingroup poweric_core
 *
 * @brief The kernel-level interface to access power IC registers
 *
 * The functions in this file implement the kernel-level interface for reading
 * and writing power IC registers.  Low-level register access to the power IC is
 * controlled using disabled bottom halves.  The disabling of bottom halves is
 * still critical and will stay.  This will ensure that only one execution thread
 * will ever be accessing a power IC register at  any time.  While this is not so
 * important for the read functions, it becomes critical for the read-modify-write
 * functions.
 *
 * A RAM copy of the power IC registers is maintained.  This read functions will
 * always read the value of the register directly from the power IC instead of
 * using the RAM copy, but the RAM copy will be updated with whatever is read.
 * The write functions use the RAM copy to get the value of the bits in the
 * register that are not being explicitly changed.
 */

#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <asm/semaphore.h>

#include "atlas_register.h"
#include "core.h"
#include "event.h"
#include "os_independent.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
EXPORT_SYMBOL(power_ic_read_reg);
EXPORT_SYMBOL(power_ic_write_reg);
EXPORT_SYMBOL(power_ic_write_reg_value);
EXPORT_SYMBOL(power_ic_set_reg_bit);
EXPORT_SYMBOL(power_ic_set_reg_value);
EXPORT_SYMBOL(power_ic_get_reg_value);
EXPORT_SYMBOL(power_ic_set_reg_mask);
#endif

/******************************************************************************
* Constants
******************************************************************************/
/*!
 * @brief No need to read before writing to the register and no need to read from cache.
 */
#define NO_SPECIAL_READ          0x00  
/*!
 * @brief Must read before writing to the register.
 */
#define READ_BEFORE_WRITE        0x01  
/*!
 * @brief Read the register contents from cache instead of hardware.
 */
#define READ_FROM_CACHE          0x02
/*!
 * @brief Must read before writing to the register and must read from cache.
 */
#define READ_CACHE_BEFORE_WRITE  0x03

#define POWER_IC_REG_NUM_REGS POWER_IC_REG_NUM_REGS_ATLAS
#define POWER_GATE_INVERT     0x18000

/******************************************************************************
* Local Variables
******************************************************************************/

/*! @brief Mutex needed to protect accesses to the power IC's. */
static DECLARE_MUTEX(power_ic_access_mutex);

/*! @brief RAM copies of all power IC registers */
static unsigned int power_ic_registers[POWER_IC_REG_NUM_REGS] =
{

    /* Atlas Registers */
    0x0000000,  /* Interrupt Status 0 */
    0x0000000,  /* Interrupt Mask 0 */
    0x0000000,  /* Interrupt Sense 0 */
    0x0000000,  /* Interrupt Status 1 */
    0x0000000,  /* Interrupt Mask 1 */
    0x0000000,  /* Interrupt Sense 1 */
    0x0000000,  /* Power Up Mode Sense */
    0x0000000,  /* Revision */
    0x0000000,  /* Semaphore */
    0x0000000,  /* Arbitration Peripheral Audio */
    0x0000000,  /* Arbitration Switchers */
    0x0000000,  /* Arbitration Regulators 0 */
    0x0000000,  /* Arbitration Regulators 1 */
    0x0000000,  /* Power Control 0 */
    0x0000000,  /* Power Control 1 */
    0x0000000,  /* Power Control 2 */
    0x0000000,  /* Regen Assignment */
    0x0000000,  /* Control Spare */
    0x0000000,  /* Memory A */
    0x0000000,  /* Memory B */
    0x0000000,  /* RTC Time */
    0x0000000,  /* RTC Alarm */
    0x0000000,  /* RTC Day */
    0x0000000,  /* RTC Day Alarm */
    0x0000000,  /* Switchers 0 */
    0x0000000,  /* Switchers 1 */
    0x0000000,  /* Switchers 2 */
    0x0000000,  /* Switchers 3 */ 
    0x0000000,  /* Switchers 4 */
    0x0000000,  /* Switchers 5 */
    0x0000000,  /* Regulator Setting 0 */
    0x0000000,  /* Regulator Setting 1 */
    0x0000000,  /* Regulator Mode 0 */
    0x0000000,  /* Regulator Mode 1 */
    0x0000000,  /* Power Miscellaneous */
    0x0000000,  /* Power Spare */
    0x0000000,  /* Audio Rx 0 */
    0x0000000,  /* Audio Rx 1 */
    0x0000000,  /* Audio Tx */
    0x0000000,  /* SSI Network */
    0x0000000,  /* Audio Codec */
    0x0000000,  /* Audio Stereo DAC */
    0x0000000,  /* Audio Spare */
    0x0000000,  /* ADC 0 */
    0x0000000,  /* ADC 1 */
    0x0000000,  /* ADC 2 */
    0x0000000,  /* ADC 3 */
    0x0000000,  /* ADC 4 */
    0x0000000,  /* Charger 0 */
    0x0000000,  /* USB 0 */
    0x0000000,  /* USB 1 */
    0x0000000,  /* LED Control 0 */
    0x0000000,  /* LED Control 1 */
    0x0000000,  /* LED Control 2 */
    0x0000000,  /* LED Control 3 */
    0x0000000,  /* LED Control 4 */
    0x0000000,  /* LED Control 5 */
    0x0000000,  /* Spare */
    0x0000000,  /* Trim 0 */
    0x0000000,  /* Trim 1 */
    0x0000000,  /* Test 0 */
    0x0000000,  /* Test 1*/
    0x0000000,  /* Test 2 */
    0x0000000   /* Test 3 */
};

/*!
 * @brief Masks indicating which bits are maintained in the RAM copy of each register
 *
 * A '1' bit indicates that the corresponding bit in the power IC register will
 * not be saved in the RAM copy of the register.  That is, all bits that can change
 * autonomously in the power IC should be set to '1's.
 */
static const unsigned int power_ic_register_no_write_masks[POWER_IC_REG_NUM_REGS] =
{
    /* Atlas Registers */
    0x0FFFFFF,  /* Interrupt Status 0 */
    0x0160020,  /* Interrupt Mask 0 */
    0x0FFFFFF,  /* Interrupt Sense 0 */
    0x0FFFFFF,  /* Interrupt Status 1 */
    0x0C10004,  /* Interrupt Mask 1 */
    0x0FFFFFF,  /* Interrupt Sense 1 */
    0x0FFFFFF,  /* Power Up Mode Sense */
    0x0FFFFFF,  /* Revision */
    0x0000000,  /* Semaphore */
    0x0F80000,  /* Arbitration Peripheral Audio */
    0x0FE0000,  /* Arbitration Switchers */
    0x0000000,  /* Arbitration Regulators 0 */
    0x0C03C00,  /* Arbitration Regulators 1 */
    0x0000000,  /* Power Control 0 */
    0x0E00000,  /* Power Control 1 */
    0x0FFF000,  /* Power Control 2 */
    0x000C000,  /* Regen Assignment */
    0x0FFFFFF,  /* Control Spare */
    0x0000000,  /* Memory A */
    0x0000000,  /* Memory B */
    0x0FFFFFF,  /* RTC Time */
    0x0000000,  /* RTC Alarm */
    0x0FFFFFF,  /* RTC Day */
    0x0000000,  /* RTC Day Alarm */
    0x0FC0000,  /* Switchers 0 */
    0x0FC0000,  /* Switchers 1 */
    0x0FC0000,  /* Switchers 2 */
    0x0FC0000,  /* Switchers 3 */ 
    0x0C00030,  /* Switchers 4 */
    0x0800030,  /* Switchers 5 */
    0x0F80003,  /* Regulator Setting 0 */
    0x0FFF000,  /* Regulator Setting 1 */
    0x0000000,  /* Regulator Mode 0 */
    0x0000000,  /* Regulator Mode 1 */
    0x0FE003F,  /* Power Miscellaneous */
    0x0FFFFFF,  /* Power Spare */
    0x0100600,  /* Audio Rx 0 */
    0x0C00000,  /* Audio Rx 1 */
    0x0000010,  /* Audio Tx */
    0x0E00003,  /* SSI Network */
    0x0E00000,  /* Audio Codec */
    0x0E06400,  /* Audio Stereo DAC */
    0x0FFFFFF,  /* Audio Spare */
    0x0780300,  /* ADC 0 */
    0x0000004,  /* ADC 1 */
    0x0FFFFFF,  /* ADC 2 */
    0x07F8000,  /* ADC 3 */
    0x0FFFFFF,  /* ADC 4 */
    0x0F05000,  /* Charger 0 */
    0x0000000,  /* USB 0 */
    0x0FFFF10,  /* USB 1 */
    0x0010400,  /* LED Control 0 */
    0x0780000,  /* LED Control 1 */
    0x0000000,  /* LED Control 2 */
    0x0000000,  /* LED Control 3 */
    0x0000000,  /* LED Control 4 */
    0x0000000,  /* LED Control 5 */
    0x0FFFFFF,  /* Spare */
    0x0FFFFFF,  /* Trim 0 */
    0x0FFFFFF,  /* Trim 1 */
    0x0FFFFFF,  /* Test 0 */
    0x0FFFFFF,  /* Test 1*/
    0x0FFFFFF,  /* Test 2 */
    0x0FFFFFF   /* Test 3 */
};

/*!
 * @brief Masks indicating which bits must not be changed from powerup.
 *
 * A '0' bit indicates that the corresponding bit in the power IC register will not be changed from
 * the value written during initialization.  A '1' bit indicates that the corresponding bit in the
 * power IC register may have new values written to it.
 *
 * When a register is changed in this table, it must be added to the product-specific init table
 * with a value of either an initialization value or the ATLAS hardware value.
 */
static const unsigned int power_ic_register_init_only[POWER_IC_REG_NUM_REGS] =
{
    /* Atlas Registers */
    0x0FFFFFF,  /* Interrupt Status 0 */
    0x0FFFFFF,  /* Interrupt Mask 0 */
    0x0FFFFFF,  /* Interrupt Sense 0 */
    0x0FFFFFF,  /* Interrupt Status 1 */
    0x0FFFFFF,  /* Interrupt Mask 1 */
    0x0FFFFFF,  /* Interrupt Sense 1 */
    0x0FFFFFF,  /* Power Up Mode Sense */
    0x0FFFFFF,  /* Revision */
    0x0FFFFFF,  /* Semaphore */
    0x0FFFFFF,  /* Arbitration Peripheral Audio */
    0x0FFFFFF,  /* Arbitration Switchers */
    0x0FFFFFF,  /* Arbitration Regulators 0 */
    0x0FFFFFF,  /* Arbitration Regulators 1 */
    0x0FFFFFF,  /* Power Control 0 */
    0x0FFFFFF,  /* Power Control 1 */
    0x0FFFFFF,  /* Power Control 2 */
    0x0FFFFFF,  /* Regen Assignment */
    0x0FFFFFF,  /* Control Spare */
    0x0FFFFFF,  /* Memory A */
    0x0FFFFFF,  /* Memory B */
    0x0FFFFFF,  /* RTC Time */
    0x0FFFFFF,  /* RTC Alarm */
    0x0FFFFFF,  /* RTC Day */
    0x0FFFFFF,  /* RTC Day Alarm */
    0x0FFFFFF,  /* Switchers 0 */
    0x0FFFFFF,  /* Switchers 1 */
    0x0FFFFFF,  /* Switchers 2 */
    0x0FFFFFF,  /* Switchers 3 */ 
    0x0FFFFFF,  /* Switchers 4 */
    0x0FFFFFF,  /* Switchers 5 */
    0x0FFFFFF,  /* Regulator Setting 0 */
    0x0FFFFFF,  /* Regulator Setting 1 */
    0x0B6DB6D,  /* Regulator Mode 0 */
    0x0B6DB6D,  /* Regulator Mode 1 */
    0x0FE7FFF,  /* Power Miscellaneous */
    0x0FFFFFF,  /* Power Spare */
    0x0FFFFFF,  /* Audio Rx 0 */
    0x0FFFFFF,  /* Audio Rx 1 */
    0x0FFFFFF,  /* Audio Tx */
    0x0FFFFFF,  /* SSI Network */
    0x0FFFFFF,  /* Audio Codec */
    0x0FFFFFF,  /* Audio Stereo DAC */
    0x0FFFFFF,  /* Audio Spare */
    0x0FFFFFF,  /* ADC 0 */
    0x0FFFFFF,  /* ADC 1 */
    0x0FFFFFF,  /* ADC 2 */
    0x0FFFFFF,  /* ADC 3 */
    0x0FFFFFF,  /* ADC 4 */
    0x0FFFFFF,  /* Charger 0 */
    0x0FFFFFF,  /* USB 0 */
    0x0FFFFFF,  /* USB 1 */
    0x0FFFFFF,  /* LED Control 0 */
    0x0FFFFFF,  /* LED Control 1 */
    0x0FFFFFF,  /* LED Control 2 */
    0x0FFFFFF,  /* LED Control 3 */
    0x0FFFFFF,  /* LED Control 4 */
    0x0FFFFFF,  /* LED Control 5 */
    0x0FFFFFF,  /* Spare */
    0x0FFFFFF,  /* Trim 0 */
    0x0FFFFFF,  /* Trim 1 */
    0x0FFFFFF,  /* Test 0 */
    0x0FFFFFF,  /* Test 1*/
    0x0FFFFFF,  /* Test 2 */
    0x0FFFFFF   /* Test 3 */
};

/*! @brief Table that defines which registers need to be read before written 
 *  
 *  A '1' in bit 0 will indicate that the register should be read before being written and
 *  a '1' in bit 1 will indicate that the register should always be read from cache instead
 *  of from the hardware. */
static const unsigned char read_before_write[POWER_IC_REG_NUM_REGS] =
{
    /* Atlas Registers */
    NO_SPECIAL_READ,     /* Interrupt Status 0 */
    READ_BEFORE_WRITE,   /* Interrupt Mask 0 */
    READ_BEFORE_WRITE,   /* Interrupt Sense 0 */
    NO_SPECIAL_READ,     /* Interrupt Status 1 */
    READ_BEFORE_WRITE,   /* Interrupt Mask 1 */
    READ_BEFORE_WRITE,   /* Interrupt Sense 1 */
    READ_BEFORE_WRITE,   /* Power Up Mode Sense */
    READ_BEFORE_WRITE,   /* Revision */
    READ_BEFORE_WRITE,   /* Semaphore */
    READ_BEFORE_WRITE,   /* Arbitration Peripheral Audio */
    READ_BEFORE_WRITE,   /* Arbitration Switchers */
    READ_BEFORE_WRITE,   /* Arbitration Regulators 0 */
    READ_BEFORE_WRITE,   /* Arbitration Regulators 1 */
    READ_BEFORE_WRITE,   /* Power Control 0 */
    READ_BEFORE_WRITE,   /* Power Control 1 */
    READ_BEFORE_WRITE,   /* Power Control 2 */
    READ_BEFORE_WRITE,   /* Regen Assignment */
    READ_BEFORE_WRITE,   /* Control Spare */
    READ_BEFORE_WRITE,   /* Memory A */
    READ_BEFORE_WRITE,   /* Memory B */
    READ_BEFORE_WRITE,   /* RTC Time */
    READ_BEFORE_WRITE,   /* RTC Alarm */
    READ_BEFORE_WRITE,   /* RTC Day */
    READ_BEFORE_WRITE,   /* RTC Day Alarm */
    READ_BEFORE_WRITE,   /* Switchers 0 */
    READ_BEFORE_WRITE,   /* Switchers 1 */
    READ_BEFORE_WRITE,   /* Switchers 2 */
    READ_BEFORE_WRITE,   /* Switchers 3 */ 
    READ_BEFORE_WRITE,   /* Switchers 4 */
    READ_BEFORE_WRITE,   /* Switchers 5 */
    READ_BEFORE_WRITE,   /* Regulator Setting 0 */
    READ_BEFORE_WRITE,   /* Regulator Setting 1 */
    READ_BEFORE_WRITE,   /* Regulator Mode 0 */
    READ_BEFORE_WRITE,   /* Regulator Mode 1 */
    READ_BEFORE_WRITE,   /* Power Miscellaneous */
    READ_BEFORE_WRITE,   /* Power Spare */
    READ_BEFORE_WRITE,   /* Audio Rx 0 */
    READ_BEFORE_WRITE,   /* Audio Rx 1 */
    READ_BEFORE_WRITE,   /* Audio Tx */
    READ_BEFORE_WRITE,   /* SSI Network */
    READ_BEFORE_WRITE,   /* Audio Codec */
    READ_BEFORE_WRITE,   /* Audio Stereo DAC */
    READ_BEFORE_WRITE,   /* Audio Spare */
    READ_BEFORE_WRITE,   /* ADC 0 */
    READ_BEFORE_WRITE,   /* ADC 1 */
    READ_BEFORE_WRITE,   /* ADC 2 */
    READ_BEFORE_WRITE,   /* ADC 3 */
    READ_BEFORE_WRITE,   /* ADC 4 */
    READ_BEFORE_WRITE,   /* Charger 0 */
    READ_BEFORE_WRITE,   /* USB 0 */
    READ_BEFORE_WRITE,   /* USB 1 */
    READ_BEFORE_WRITE,   /* LED Control 0 */
    READ_BEFORE_WRITE,   /* LED Control 1 */
    READ_BEFORE_WRITE,   /* LED Control 2 */
    READ_BEFORE_WRITE,   /* LED Control 3 */
    READ_BEFORE_WRITE,   /* LED Control 4 */
    READ_BEFORE_WRITE,   /* LED Control 5 */
    READ_BEFORE_WRITE,   /* Spare */
    READ_BEFORE_WRITE,   /* Trim 0 */
    READ_BEFORE_WRITE,   /* Trim 1 */
    READ_BEFORE_WRITE,   /* Test 0 */
    READ_BEFORE_WRITE,   /* Test 1*/
    READ_BEFORE_WRITE,   /* Test 2 */
    READ_BEFORE_WRITE    /* Test 3 */
};

/******************************************************************************
* Local Functions
******************************************************************************/

/*!
 * @brief Read an entire register from the power IC
 *
 * This function is used to read an entire register from the power IC.  This read
 * function will always read the value of the register directly from the power IC
 * instead of using the RAM copy, but the RAM copy will be updated with whatever
 * is read.
 *
 * @param        reg        register number
 * @param        reg_value  location to store the register value
 *
 * @return 0 if successful
 */ 

static int read_reg(POWER_IC_REGISTER_T reg, unsigned int *reg_value) 
{
    unsigned int value;
    int retval = -EINVAL;
     
    if(!POWER_IC_REGISTER_IS_ATLAS(reg))
    {
        printk("POWER_IC: Atlas Register is invalid\n");
        return -EINVAL;
    }
       
    /* Check to see if we should read directly from hardware or from cache */
    if((read_before_write[reg] & READ_FROM_CACHE) == READ_FROM_CACHE)
    {
        value = power_ic_registers[reg];
        retval = 0;
    }
    else
    {
        /* Read the register from ATLAS */
        retval = atlas_reg_read (reg - POWER_IC_REG_ATLAS_FIRST_REG, &value);
    }

    /* If the read was successful, store the value in the RAM copy */
    if (retval == 0)
    {
        if (reg_value != NULL)
        {
            *reg_value = value;
        }
        
        value &= ~power_ic_register_no_write_masks[reg];
        power_ic_registers[reg] |= value & ~power_ic_register_init_only[reg];
    }
    
    return retval;
}

/******************************************************************************
* Global Functions
******************************************************************************/

/*!
 * @brief Read an entire register from the power IC
 *
 * This function is the wrapper to the function used to read an entire register
 * from the power IC.
 *
 * @param        reg        register number
 * @param        reg_value  location to store the register value
 *
 * @return 0 if successful
 */ 

int power_ic_read_reg(POWER_IC_REGISTER_T reg, unsigned int *reg_value) 
{
    int ret_val = 0;
    
    if(down_interruptible(&power_ic_access_mutex) != 0)
    {
        tracemsg(_k_d("process received signal while waiting for postponable mutex. Exiting."));
        return -EINTR;
    }

    ret_val = read_reg(reg, reg_value);
    
    up(&power_ic_access_mutex);

    return ret_val;
}

/*!
 * @brief Write an entire register to the power IC
 *
 * This function is used to write an entire register to the power IC.  Since
 * the entire register is being written, the RAM copy of the register is not
 * read in this function, but the RAM copy will be updated with the value that
 * is written to the register.
 *
 * @param        reg         register number
 * @param        reg_value   pointer to the new register value
 * @param        permission  permission to access to Backup Memory registers
 *
 * @return 0 if successful
 */ 

int power_ic_write_reg_sec(POWER_IC_REGISTER_T reg,
                           unsigned int *reg_value,
                           POWER_IC_BACKUP_MEMORY_ACCESS_T permission) 
{
    int retval = -EINVAL;
    unsigned int regv;
    
    if(POWER_IC_REGISTER_IS_ATLAS(reg) && (reg_value != NULL))
    {
        regv = *reg_value;

        if(down_interruptible(&power_ic_access_mutex) == 0)
        {
            /* Check if we have permission to access backup memory. */
            if ((permission == BACKUP_MEMORY_ACCESS_NOT_ALLOWED) &&
                ((reg == POWER_IC_REG_ATLAS_MEMORY_A) || (reg == POWER_IC_REG_ATLAS_MEMORY_B)))
            {
                retval = -EPERM;
            }
            else
            {
                /* For bits that must not be changed from init, set the init values before
                 * writing.
                 */
                if (power_ic_register_init_only[reg] != 0x0FFFFFF)
                {
                    regv &= power_ic_register_init_only[reg];
                    regv |= (reg_init_tbl[reg] & ~(power_ic_register_init_only[reg]));
                }

                /* Write the register to ATLAS if it is an ATLAS register */
                retval = atlas_reg_write (reg - POWER_IC_REG_ATLAS_FIRST_REG, regv);

                /* If the write was successful, save the new register contents */
                if (retval == 0)
                {
                    power_ic_registers[reg] = regv & ~power_ic_register_no_write_masks[reg];
                }
            }

            up(&power_ic_access_mutex);
        }
        else
        {
            tracemsg(_k_d("process received signal while waiting for postponable mutex. Exiting."));
            retval = -EINTR;
        }
    }
    else
    {
        printk("POWER_IC: register number or value is invalid: reg=%d, reg_value=%d\n",
               reg,
               (unsigned int) reg_value);
        retval = -EINVAL;
    }

    return retval;
}

/*!
 * @brief Write an entire register to the power IC
 *
 * This function is used to write an entire register to the power IC.  Since
 * the entire register is being written, the RAM copy of the register is not
 * read in this function, but the RAM copy will be updated with the value that
 * is written to the register.
 *
 * This function differs from power_ic_write_reg() in that it takes the new
 * register value directly instead of expecting a pointer to the new value.
 * The function is simply implemented as a call to power_ic_write_reg().
 *
 * @param        reg        register number
 * @param        reg_value  new register value
 *
 * @return 0 if successful
 */ 

int power_ic_write_reg_value(POWER_IC_REGISTER_T reg, unsigned int reg_value)
{
    return power_ic_write_reg (reg, &reg_value);
}

/*!
 * @brief Set the value of a single bit in a power IC register
 *
 * This function is used to set an individual bit in a power IC register.  The
 * function is implemented simply as a call to power_ic_set_reg_value() with
 * the number of bits parameter set to 1.
 *
 * @param        reg        register number
 * @param        index      bit index to set (0 = least-significant bit)
 * @param        value      new bit value
 *
 * @return 0 if successful
 */ 
 
int power_ic_set_reg_bit(POWER_IC_REGISTER_T reg, int index, int value) 
{
    return power_ic_set_reg_value (reg, index, value ? 1 : 0, 1);
}

/*!
 * @brief Set the value of a range of bits in a power IC register
 *
 * This function is used to set a range of bits in a power IC register.  The
 * function is implemented as a call to power_ic_set_reg_mask() by converting
 * the input parameters index and nb_bits into a bitmask.
 *
 * @param        reg        register number
 * @param        index      starting bit index (0 = least-significant bit)
 * @param        value      new value
 * @param        nb_bits    number of bits to set
 *
 * @return 0 if successful
 */ 
int power_ic_set_reg_value(POWER_IC_REGISTER_T reg, int index, int value, int nb_bits) 
{
    return power_ic_set_reg_mask (reg, (((1 << nb_bits) - 1) << index), value << index);
}

/*!
 * @brief Read the value of a range of bits in a power IC register
 *
 * This function is used to read a contiguous range of bits in a power IC register.
 * The function is implemented by calling power_ic_read_reg() which reads the
 * entire register contents and then masking and shifting the returned register
 * value to match the input parameters.
 *
 * @param        reg        register number
 * @param        index      starting bit index (0 = least-significant bit)
 * @param        value      location to store the read value
 * @param        nb_bits    number of bits to read
 *
 * @return 0 if successful
 */ 
int power_ic_get_reg_value(POWER_IC_REGISTER_T reg, int index, int *value, int nb_bits) 
{
    unsigned int reg_value;
    int retval;

    /* Read the whole register first */
    retval = power_ic_read_reg(reg, &reg_value);

    if (retval == 0 && value != NULL)
    {
        *value = (reg_value >> index) & ((1 << nb_bits) - 1);
    }

    return retval;
}

/*!
 * @brief Set the value of a set of (possibly) non-contiguous bits in a power IC register
 *
 * This function is used to set a possibly non-contiguous set of bits in a power IC
 * register.  This function performs a read-modify-write operation on a power IC register.
 * In the case of those registers that must be physically read before being written (as
 * indicated in the #read_before_write array), the register is read directly from the
 * power IC.  Otherwise, the RAM copy of the register is used as the starting point for
 * the modification.  Using the passed in mask parameter, the set of bits are first
 * cleared and then the new bits are OR'ed into the register.  This new register value
 * is then written out to the power IC and then saved into the RAM copy.
 *
 * This function is the basis for a number of other functions, namely power_ic_set_reg_bit()
 * and power_ic_set_reg_value().  Since all three functions perform nearly the same operation,
 * the implementation was combined into this single generic function which the other two
 * call.
 *
 * @param        reg          register number
 * @param        mask       bitmask indicating which bits are to be modified
 * @param        value        new values for modified bits  
 * @param        permission   permission to access to Backup Memory registers  
 *
 * @return 0 if successful
 */
int power_ic_set_reg_mask_sec(POWER_IC_REGISTER_T reg,
                              int mask,
                              int value,
                              POWER_IC_BACKUP_MEMORY_ACCESS_T permission)
{
    int retval = -EINVAL;
    unsigned int old_value;
    
    if(!POWER_IC_REGISTER_IS_ATLAS(reg))
    {
        printk("POWER_IC: Atlas Register is invalid\n");
        return -EINVAL;
    }
    
    if(down_interruptible(&power_ic_access_mutex) != 0)
    {
        tracemsg(_k_d("process received signal while waiting for postponable mutex. Exiting."));
        return -EINTR;
    }

    /* Check if we have permission to access backup memory. */
    if ((permission == BACKUP_MEMORY_ACCESS_NOT_ALLOWED) &&
        ((reg == POWER_IC_REG_ATLAS_MEMORY_A) || (reg == POWER_IC_REG_ATLAS_MEMORY_B)))
    {
        retval = -EPERM;
    }
    else
    {
        /* If the register needs to be read before written, read it now */
        if ((read_before_write[reg] & READ_BEFORE_WRITE) &&
            (power_ic_register_init_only[reg] == 0x0FFFFFF))
        {
            /*
            * Read the register, but discard the value.  We're only interested in
            * updating the RAM copy right now. The function read_reg is called
            * directly since the mutex has already be taken.
            */
            read_reg(reg, &old_value);
        }
        else
        {
            /* Get the RAM copy of the register */
            old_value = power_ic_registers[reg];
        }

        /* Clear the bits in question */
        old_value &= ~mask;

        /* Set the new value of the bits */
        old_value |= value & mask;

        /* For bits that must not be changed from init, set the init values before writing. */
        if (power_ic_register_init_only[reg] != 0x0FFFFFF)
        {
            old_value &= power_ic_register_init_only[reg];
            old_value |= (reg_init_tbl[reg] & ~(power_ic_register_init_only[reg]));
        }
        
        retval = atlas_reg_write (reg - POWER_IC_REG_ATLAS_FIRST_REG, old_value);
    
        /* If the write was successful, save the new register contents */
        if (retval == 0)
        {
            power_ic_registers[reg] = old_value & ~power_ic_register_no_write_masks[reg];
        }
    }
    
    up(&power_ic_access_mutex);

    return retval;
}

/*!
 * @brief Write an entire register to the power IC
 *
 * This function is used to write an entire register to the power IC.  Since
 * the entire register is being written, the RAM copy of the register is not
 * read in this function, but the RAM copy will be updated with the value that
 *
 * @param        reg_value   pointer to the new register value
 * @return 0 if successful
 */ 
int power_ic_write_reg(POWER_IC_REGISTER_T reg, unsigned int *reg_value) 
{
    return power_ic_write_reg_sec(reg, reg_value, BACKUP_MEMORY_ACCESS_NOT_ALLOWED);
}

/*!
 * @brief Set the value of a set of (possibly) non-contiguous bits in a power IC register
 *
 * This function is used to set a possibly non-contiguous set of bits in a power IC
 * register.  This function performs a read-modify-write operation on a power IC register.
 * In the case of those registers that must be physically read before being written (as
 * indicated in the #read_before_write array), the register is read directly from the
 * power IC.  Otherwise, the RAM copy of the register is used as the starting point for
 * the modification.  Using the passed in mask parameter, the set of bits are first
 * cleared and then the new bits are OR'ed into the register.  This new register value
 * is then written out to the power IC and then saved into the RAM copy.
 *
 * This function is the basis for a number of other functions, namely power_ic_set_reg_bit()
 * and power_ic_set_reg_value().  Since all three functions perform nearly the same operation,
 * the implementation was combined into this single generic function which the other two
 * call.
 *
 * @param        reg          register number
 * @param        mask         bitmask indicating which bits are to be modified
 * @param        value        new values for modified bits  
 *
 * @return 0 if successful
 */
int power_ic_set_reg_mask(POWER_IC_REGISTER_T reg, int mask, int value)
{
    return power_ic_set_reg_mask_sec(reg, mask, value, BACKUP_MEMORY_ACCESS_NOT_ALLOWED);
}
