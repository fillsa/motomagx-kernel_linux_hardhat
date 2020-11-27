/*
 * Copyright (C) 2006 Motorola, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-31 - One wire module support for ArgonLV
 * Motorola 2006-Jun-28 - Montavista header upmerge changes
 * Motorola 2006-Feb-06 - File Creation
 */

/*!
 * @file owire.c
 *
 * @brief This file contains functions for one wire bus driver on SCM-A11. 
 *
 * @ingroup poweric_brt
 */

#include <asm/arch/clock.h>
#include <asm/div64.h>
#include <asm/io.h>

#ifdef CONFIG_ARCH_MXC91231
#include <asm/arch/mxc91231.h>
#elif defined(CONFIG_ARCH_MXC91321)
#include <asm/arch/mxc91321.h>
#endif

#include <linux/delay.h>
#include <linux/types.h>
#include "owire.h"

/*!
 * @brief Bit mask for bit 0
 */
#define LSB_BIT_MASK       0x0001

/*!
 * @brief Read destination bit position
 */
#define RDST_BIT           3

/*!
 * @brief 1 million
 */
#define MILLION            1000000

/*!
 * @brief Reset Pulled and Presence Detect (RPP)
 */
#define CTRL_RPP           0x0080

/*!
 * @brief Presence Status (PST)
 */
#define CTRL_PST           0x0040

/*!
 * @brief Write 0 (WR0)
 */
#define CTRL_WR0           0x0020

/*!
 * @brief Write 1 or Read (WR1/RD)
 */
#define CTRL_WR1RD         0x0010

/*!
 * @brief Read Status (RDST)
 */
#define CTRL_RDST          0x0008

/*!
 * @brief Software Reset of Module (rst)
 */
#define RST_RESET          0x0001

/*!
 * @brief Mask for owire in mux
 */
#define MUX_OW_MASK        0x00FFFFFF

/*!
 * @brief Enable owire mux as io
 */
#define MUX_OW_IO          0x12000000

/*!
 * @brief Control register position
 */
#define REG_CTRL           IO_ADDRESS(OWIRE_BASE_ADDR + 0x00)

/*!
 * @brief Clock divider register position
 */
#define REG_CLKDIV         IO_ADDRESS(OWIRE_BASE_ADDR + 0x02)

/*!
 * @brief Reset register position
 */
#define REG_RST            IO_ADDRESS(OWIRE_BASE_ADDR + 0x04)

/*!
 * @brief Mux AP register position
 */

/*!@cond INTERNAL */
#if defined(CONFIG_ARCH_MXC91321)
#define MUX_OW_MASK                0xFFFF00FF /*! mask for owire in mux */
#define MUX_OW_IO                  0x00001200 /*! Enable owire mux as io */
#define REG_MUX_OW                 IO_ADDRESS(IOMUXC_BASE_ADDR + 0xDC)
#define MUX_PAD_CTL_BATT_LINE_EN   0x00080000 /*! Enable pad ctl loopback for batt line */
#define REG_MUX_PAD_CTL_BATT_LINE  IO_ADDRESS(IOMUXC_BASE_ADDR + 0x2C0)
#define read_ow_reg readb
#define write_ow_reg writeb

#elif defined(CONFIG_ARCH_MXC91231)
#define MUX_OW_MASK                0x00FFFFFF /*! mask for owire in mux */
#define MUX_OW_IO                  0x12000000 /*! Enable owire mux as io */
#define REG_MUX_OW                 IO_ADDRESS(IOMUX_AP_BASE_ADDR + 0x10)
#define read_ow_reg readw
#define write_ow_reg writew
#endif
/*!@endcond */
/*!
 * @brief owire_detect - detect one-wire device 
 *
 * This function implements a detection of one wire bus device.  
 *
 * @return 1 if owire device is detected
 */
bool owire_detect(void)
{
    u8 reg_val;

    reg_val = read_ow_reg(REG_CTRL);
    reg_val |= CTRL_RPP;
    /* Set the RPP bit */
    write_ow_reg(reg_val, REG_CTRL);

    /* Wait until the RPP bit returns low */
    while((bool)(read_ow_reg(REG_CTRL) & CTRL_RPP) != 0)
    {
        /* Wait */
    }

    /* Return the state of the PST BIT */
    reg_val = read_ow_reg(REG_CTRL) & CTRL_PST;

    return ((bool)reg_val);
}

/*!
 * @brief owire_read - read one-wire device 
 *
 * This function implements a read function from one wire bus device.  
 *
 * @param number_of_bits number of bits to read
 *
 * @return read result
 */
u8 owire_read(u8 number_of_bits)
{
    u8 index;
    u8 read_value = 0;
    u8 reg_val;

    for (index=0; index<number_of_bits; index++)
    {
        /* Initiate the read operation */
        reg_val = read_ow_reg(REG_CTRL);
        reg_val |= CTRL_WR1RD;
        write_ow_reg(reg_val, REG_CTRL);
        /* Wait until WR1/RD bit returns low */
        while((bool)(read_ow_reg(REG_CTRL) & CTRL_WR1RD) != 0)
        {
            /* Wait */
        }
        /* Place read bit into the current bit position of the pattern being read */
        read_value |= ((read_ow_reg(REG_CTRL) & CTRL_RDST)>>RDST_BIT)<<index;
    }

    return (read_value);
}

/*!
 * @brief owire_write - write to one-wire device 
 *
 * This function implements a write function to one wire bus device.  
 *
 * @param value          value to write
 * @param number_of_bits number of bits to write
 */
void owire_write(u8 value, u8 number_of_bits)
{
    u8 index;
    u8 reg_val;
   
    for (index=0; index<number_of_bits; index++)
    {
        reg_val = read_ow_reg(REG_CTRL);
        /* if current bit is a one then set WR1/RD else set WR0 */
        (bool)((value >> index) & LSB_BIT_MASK) ? (reg_val |= CTRL_WR1RD) : (reg_val |= CTRL_WR0);
        write_ow_reg(reg_val, REG_CTRL);
        /* Wait until the the WR0 bit or the WR1/RD bit return low */
        while (((bool)(read_ow_reg(REG_CTRL) & CTRL_WR0) != 0) || 
                ((bool)(read_ow_reg(REG_CTRL) & CTRL_WR1RD) != 0))
        {
            /* Wait */
        }
    }      
}

/*!
 * @brief owire_init - initial one-wire device 
 *
 * This function implements the initial function on one wire module.
 */
void owire_init(void)
{
    u8 reg_val;
    u32 rem; 
    u32 mux;
    
    /* Read clock used by owire module, in scma11, i2c and owire uses same clock */
    u32 clk = mxc_get_clocks(I2C_CLK);

    /* Determine the value for clock divider register,
       SCMA11 uses clock/(divider+1) to generate 1MHz internal clock */
    rem = do_div(clk, MILLION);
    if (rem < (MILLION/2)){
        clk--;
    }

    /* Set mux as io */
    mux = readl(REG_MUX_OW);
    mux &= MUX_OW_MASK;
    mux |= MUX_OW_IO;
    writel(mux, REG_MUX_OW);

#if defined(CONFIG_ARCH_MXC91321)
    /* Enable loopback for software pad control */
    mux = readl(REG_MUX_PAD_CTL_BATT_LINE);
    mux |= MUX_PAD_CTL_BATT_LINE_EN;
    writel(mux, REG_MUX_PAD_CTL_BATT_LINE);
#endif

    /* Set bit 0 to put 1 wire module in reset */
    reg_val = read_ow_reg(REG_RST);
    reg_val |= RST_RESET;
    write_ow_reg(reg_val, REG_RST);

    /* Wait for 700us, defined by scma11 spec */
    udelay(700);

    /* Clear reset bit to pull 1 wire module out of reset */
    reg_val &= ~RST_RESET;
    write_ow_reg(reg_val, REG_RST);
 
    /* Set up the one-wire clock divider */
    write_ow_reg((u16)clk, REG_CLKDIV);
}

