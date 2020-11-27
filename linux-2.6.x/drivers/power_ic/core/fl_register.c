/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2004-2007 Motorola, Inc.
 * 
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Motorola 2007-Mar-21 - Initialize unused registers to 0xFF.
 * Motorola 2007-Feb-19 - Use HWCFG to determine which chip is present.
 * Motorola 2007-Jan-16 - Add support for MAX6946 LED controller chip.
 * Motorola 2006-Sep-25 - Add support for MAX7314 LED controller chip.
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2005-Jun-16 - Add Traces for debugging
 * Motorola 2004-Dec-06 - Design the Low Level interface to Funlight IC.
 */

/*!
 * @file fl_register.c
 * 
 * @ingroup poweric_core
 *
 * @brief Low-level interface to the Funlight registers
 *
 * Using the connection of the I2C bus to the processor, the Funlight Chip can be accessed.
 * In order for this to occur, a couple things must first happen. 
 * 
 *    - The I2C driver must be registered for the Funlight Chip to be used.
 *    - We must then wait for the I2C adaptor to be registered. 
 *    - An I2C client has to be registered, which will contain the driver and the adapter.
 *
 * Once all of this is complete, the I2C functions can be used to read, write and transfer data from
 * the Funlight Chip.
 *
 * This code was written using Revision 4 of the MAX7314 spec and Revision 2 of the MAX6946 spec.
 * The specs can be found online at http://www.maxim-ic.com
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/mothwcfg.h>

#include "os_independent.h"
#include "fl_register.h"

/******************************************************************************
* Constants
******************************************************************************/

/*!
 * @brief The size (in bytes) of the register address.
 */
#define FL_REG_ADDR_SIZE 1

/*!
 * @brief The size (in bytes) of the register value
 */
#define FL_REG_DATA_SIZE 1

/*!
 * @name I2C values for read and write
 */
/*! @{ */
#define FL_I2C_M_WR 0x00
#define FL_I2C_M_RD 0x01
/*! @} */

/******************************************************************************
* Macros
******************************************************************************/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define IS_CHIP_SUPPORTED() (fl_is_chip_present(FL_CHIP_MAX6946) || fl_is_chip_present(FL_CHIP_MAX7314))
#endif

/******************************************************************************
* Local function prototypes
******************************************************************************/

/*! @brief Function prototype to detach the client when finished with communication */
static int client_detach (struct i2c_client *client);

/*! @brief Function prototype to attach the client to begin communication */
static int adapter_attach (struct i2c_adapter *adap);

/******************************************************************************
* Local Variables
******************************************************************************/

/*! @brief The Funlight Chip present on the device. */
static int fl_chip_present = 0;

/*! @brief The I2C address of the Funlight Chip */
static int fl_i2c_address = 0;

/*! @brief The I2C client structure obtained for communication with the Funlight Chip */
static struct i2c_client *fl_i2c_client = NULL;

/*! @brief Creation of the I2C driver for Funlights */ 
static const struct i2c_driver driver =
{
    name:           "FunLight I2C Driver",
    flags:          I2C_DF_NOTIFY,
    attach_adapter: adapter_attach,
    detach_client:  client_detach,
};

/******************************************************************************
* Local Functions
******************************************************************************/

/*!
 * @brief Handles the addition of an I2C adapter capable of supporting the Funlight Chip
 *
 * This function is called by the I2C driver when an adapter has been added to the
 * system that can support communications with the Funlight Chip.  The function will
 * attempt to register a I2C client structure with the adapter so that communication
 * can start.  If the client is successfully registered, the Funlight initialization
 * register writes required at power-up will be executed.
 *
 * @param    adap   A pointer to I2C adapter 
 *
 * @return   This function returns 0 if successful
 */
static int adapter_attach (struct i2c_adapter *adap)
{
    int retval;

    tracemsg(_k_d("in adapter_attach"));

    /* Allocate memory for client structure */
    fl_i2c_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
    if (fl_i2c_client == NULL)
    {
        return -ENOMEM;
    }
    memset(fl_i2c_client, 0, sizeof(struct i2c_client));

    /* Fill in the required fields */
    fl_i2c_client->adapter = adap;
    fl_i2c_client->addr = fl_i2c_address;
    fl_i2c_client->driver = (struct i2c_driver *)&driver;
    sprintf(fl_i2c_client->name, "FunLight I2C Client");

    /* Register our client */
    retval = i2c_attach_client(fl_i2c_client);
    if (retval != 0)
    {
        /* Request failed, free the memory that we allocated */
        kfree(fl_i2c_client);
        fl_i2c_client = NULL;
    }
    /* else we are registered */
    else
    {
        tracemsg(_k_d("successfully attached our client"));

        /* Initialize Funlight registers now */
        switch (fl_chip_present)
        {
            case FL_CHIP_MAX6946:
                /* Set PWM stagger=1, run mode=1 */
                fl_reg_write(MAX6946_CONFIG_REG, 0x21);
                /* Set output current of P0-P3 to half and P4-P7 set to full. */
                fl_reg_write(MAX6946_OUTPUT_CURRENT_ISET70_REG, 0xF0);
                /* Set unused ports to 0xFF. */
                fl_reg_write(MAX6946_PORT_P3_OUTPUT_LEVEL_REG, 0xFF);
                fl_reg_write(MAX6946_PORT_P8_OUTPUT_LEVEL_REG, 0xFF);
                fl_reg_write(MAX6946_PORT_P9_OUTPUT_LEVEL_REG, 0xFF);
                fl_reg_write(MAX6946_PORTS_P8_P9_OUTPUT_LEVEL_REG, 0xFF);
                break;

            case FL_CHIP_MAX7314:
                /* Set P0, P1 and p2 to output ports. */
                fl_reg_write(MAX7314_PORTS_CONFIG_P7_P0_REG, 0xF8);
                /* Set P12, P13, P14 and P15 to output ports. */
                fl_reg_write(MAX7314_PORTS_CONFIG_P15_P8_REG, 0x0F);
                /* Set master intensity to 0xCF */
                fl_reg_write(MAX7314_MASTER_O16_INTESITY_REG, 0xCF);
                /* Set global control bits to 0x00 */
                fl_reg_write(MAX7314_CONFIG_REG, 0x00);
                break;

            default:
                /* This should not happen. */
                break;
        }
    }

    return retval;
}

/*!
 * @brief Handles the removal of the I2C adapter being used for the Funlight Chip
 *
 * This function call is an indication that our I2C client is being disconnected
 * because the I2C adapter has been removed.  Calling the i2c_detach_client() will
 * make sure that the client is destroyed properly.
 *
 * @param   client   Pointer to the I2C client that is being disconnected
 *
 * @return  This function returns 0 if successful
 */
static int client_detach(struct i2c_client *client)
{
    return i2c_detach_client(client);
}


/******************************************************************************
* Global Functions
******************************************************************************/

/*!
 * @brief Initializes communication with the Funlight Chip
 *
 * This function performs any initialization required for the Funlight Chip.  It
 * starts the process mentioned above for initiating I2C communication with the Funlight
 * Chip by registering our I2C driver.
 *
 * @return 0 if supported I2C funlight chip present, -ENODEV if unsupported or no funlight chip found.
 */
int __init fl_initialize(void)
{
    MOTHWCFGNODE *node;

    tracemsg(_k_d("in fl_initialize"));

    node = mothwcfg_get_node_by_path("/System@0/I2C@0/Lighting@0");
    if (node == NULL)
    {
        tracemsg(_k_d("No funlight chip found."));
        return -ENODEV;
    }

    if ((mothwcfg_read_prop(node, "type", (char *)&fl_chip_present, sizeof(fl_chip_present)) <= 0) ||
        (mothwcfg_read_prop(node, "i2c,address", (char *)&fl_i2c_address, sizeof(fl_i2c_address)) <= 0))
    {
        tracemsg(_k_d("Hardware properties could not be read."));

        mothwcfg_put_node(node);
        fl_chip_present = fl_i2c_address = 0;
        return -ENODEV;
    }

    mothwcfg_put_node(node);

    if (!IS_CHIP_SUPPORTED())
    {
        fl_chip_present = fl_i2c_address = 0;
        return -ENODEV;
    }

    /* Register our driver and return. */
    return i2c_add_driver((struct i2c_driver *)&driver);
}

/*!
 * @brief Check if a specific funlight chip is present
 *
 * @note  For the return of this function to be valid, only call this function after a successful
 *        fl_initialize call.
 *
 * @param chip  The chip to check if present.
 *
 * @return True if chip is present, false if not.
 */
bool fl_is_chip_present(int chip)
{
    return (fl_chip_present == chip);
}

/*!
 * @brief Reads a specific Funlight Chip Register
 *
 * This function implements a read of a given Funlight register.  The read is done using i2c_transfer
 * which is a set of transactions.  The first transaction will be an I2C write of the register
 * number that is going to next be read.  The second transaction will be an I2C read which will read
 * the contents of the register previously written from the Funlight Chip.  
 *
 * Originally the read was going to be done using the I2C commands i2c_master_send() to write and then 
 * i2c_master_recv() to read, but this was found to be incorrect.  These two transactions both send STOP
 * bits, whereas the funlight timing needs a RESTART in between the write and read transactions.  The
 * i2c_transfer is what gives us this functionality.
 *
 * @param reg        Funlight register number
 * @param reg_value  Pointer to which the read data should be stored
 *
 * @return This function returns 0 if successful
 */
int fl_reg_read (int reg, unsigned int *reg_value)
{
    unsigned char reg_num = reg;
    unsigned char value[FL_REG_DATA_SIZE];
    int retval;
    
    struct i2c_msg msgs[2] = 
    {
        { 0, FL_I2C_M_WR, FL_REG_ADDR_SIZE, &reg_num },
        { 0, FL_I2C_M_RD, FL_REG_DATA_SIZE, value    }
    };

    tracemsg(_k_d("in fl_reg_read"));

    /* Fail if we weren't able to initialize (yet) */
    if (fl_i2c_client == NULL)
    {
        tracemsg(_k_d("fl_reg_read: not initialized?"));
        return -EINVAL;
    }

    msgs[0].addr = msgs[1].addr = fl_i2c_client->addr;
    tracemsg(_k_d("Writing register number %2d to the funlight"), reg_num);

    retval = i2c_transfer(fl_i2c_client->adapter,msgs,2);
    if (retval >= 0) 
    {
       tracemsg(_k_d("fl_reg_read: i2c_transfer successful, reading register %2d with 0x%02x"),reg_num,
       *(msgs[1].buf));
       *reg_value = *(msgs[1].buf);
       retval = 0;
    }
    
    return retval;
}

 /*!
 * @brief Writes a value to a specified Funlight register
 *
 * This function implements a write to a specified Funlight register.  The write is accomplished
 * by sending the register number and the new contents to the i2c function i2c_master_send.
 *
 * @param reg        Funlight register number
 * @param reg_value  Register value to write
 *
 * @return This function returns 0 if successful
 */
int fl_reg_write(int reg, unsigned int reg_value)
{
    unsigned char value[FL_REG_ADDR_SIZE + FL_REG_DATA_SIZE];
    int retval;

    tracemsg(_k_d("in fl_reg_write"));

    /* Fail if we weren't able to initialize */
    if(fl_i2c_client == NULL)
    {
        tracemsg(_k_d("fl_reg_write: not initialized?"));
        return -EINVAL;
    }

    /*Copy the data into a buffer for correct format */
    value[0] = reg;
    value[1] = reg_value;

    /* Write the data to the FL */
    retval = i2c_master_send(fl_i2c_client, value, FL_REG_ADDR_SIZE + FL_REG_DATA_SIZE);

    if(retval < 0)
    {
        tracemsg(_k_d("fl_reg_write: i2c_master_send failed: %d"), retval);
    }
    else
    {
        retval = 0;
    }

    return retval;
}
