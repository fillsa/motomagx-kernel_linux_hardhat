/*
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * Motorola 2007-Jul-03 - Prevent reads and writes from occurring at the same time
 * Motorola 2006-Dec-14 - Add in loop for reading and writing on error return
 * Motorola 2006-May-09 - Initial creation.
 */

/*!
 * @file keyv_register.c
 * 
 * @ingroup keyv
 *
 * @brief Low-level interface to the KeyV registers
 *
 * Using the connection of the I2C bus to the processor, the KeyV
 * can be accessed.  In order for this to occur, a couple things must first happen. 
 * 
 *    - The I2C driver must be registered for the KeyV Chip to use.  
 *    - We must then wait for the I2C adaptor to be registered. 
 *    - An I2C client has to be registered, which will contain the driver and the adapter.
 *
 * Once all of this is complete, the I2C functions can be used to read, write and transfer data from
 * the KeyV Chip.
 *
 */

#include <linux/keyv.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/semaphore.h>

#include "keyv_registers.h"

/******************************************************************************
* Constants
******************************************************************************/
#define KEYV_RETRIES       4

/*! The size (in bytes) of the register value */
#define KEYV_REG_DATA_SIZE 1

/*! The address of the KeyV part */
#define KEYV_ADDR  0x12
#define KEYV_ADDR_2 0x14
#define KEYV_ADDR_3 0x28
#define KEYV_ADDR_4 0x76

/*! Reset value for KeyV */
#define KEYV_RST 0x20

/*! The I2C client structure obtained for communication with the KeyV Chip */
static struct i2c_client *keyv_i2c_client = NULL;

/*! A pointer to a function to be called when communication between the I2C and KeyV Chip is available*/
static void (*keyv_reg_init)(void) = NULL;

/******************************************************************************
* Local Variables
******************************************************************************/
unsigned int rst_counter = 0;

/*! @brief Mutex used to prevent reads and writes from happening at the same time */
DECLARE_MUTEX(keyv_rw);

/******************************************************************************
* Local function prototypes
******************************************************************************/
/* Function prototype to detach the client when finished with communication */
static int client_detach(struct i2c_client *client);

/* Function prototype to attach the client to begin communication */
static int adapter_attach (struct i2c_adapter *adap);

/* Function prototype to write the reset to I2C */
static int keyv_reg_write_reset(void);

/******************************************************************************
* Local Functions
******************************************************************************/
/* Creation of the i2c_driver for KeyV */ 
static const struct i2c_driver driver = {
    name:           "KeyV I2C Driver",
    flags:          I2C_DF_NOTIFY,
    attach_adapter: adapter_attach,
    detach_client:  client_detach,
};

/*!
 * @brief Handles the addition of an I2C adapter capable of supporting the KeyV Chip
 *
 * This function is called by the I2C driver when an adapter has been added to the
 * system that can support communications with the KeyV Chip.  The function will
 * attempt to register a I2C client structure with the adapter so that communication
 * can start.  If the client is successfully registered, the KeyV initialization
 * function will be called to do any register writes required at power-up.
 *
 * @param    adap   A pointer to I2C adapter 
 *
 * @return   This function returns 0 if successful
 */
static int adapter_attach (struct i2c_adapter *adap)
{
    int retval;

    /* Allocate memory for client structure */
    keyv_i2c_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
    memset(keyv_i2c_client, 0, (sizeof(struct i2c_client)));
    
    if(keyv_i2c_client == NULL)
    {
        return -ENOMEM;
    }
    
    /* Fill in the required fields */
    keyv_i2c_client->adapter = adap;
    keyv_i2c_client->addr = KEYV_ADDR;
    keyv_i2c_client->driver = &driver;
    
    /* Register our client */
    retval = i2c_attach_client(keyv_i2c_client);
    
    if(retval != 0)
    {
        /* Request failed, free the memory that we allocated */
        kfree(keyv_i2c_client);
        keyv_i2c_client = NULL;
    }
    /* else we are registered */
    else
    {
        /* Initialize KeyV registers now */
        if(keyv_reg_init != NULL)
        {
            (*keyv_reg_init)();
        }
    }
    
    return retval;
}

/*!
 * @brief Handles the removal of the I2C adapter being used for the KeyV Chip
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

/*!
 * @brief Writes a reset to all KeyV I2C addresses
 *
 * This function implements a write of a reset command 
 * to all of the KeyV I2C addresses
 *
 * @return This function returns 0 if successful
 */
static int keyv_reg_write_reset(void)
{
    unsigned char value[KEYV_REG_DATA_SIZE];
    int retval = 0;
    int i = 0;
    
    /* Fail if we weren't able to initialize */
    if(keyv_i2c_client == NULL)
    {
        return -EINVAL;
    }

    /*Copy the data into a buffer for correct format */
    value[0] = KEYV_RST;
    
    /* Reset I2C address to address 2 and write reset */
    keyv_i2c_client->addr = KEYV_ADDR_2;
    retval = i2c_master_send(keyv_i2c_client, value, KEYV_REG_DATA_SIZE);
    printk("KEYV:  Reset to Address 0x%X returned a value of %d\n", KEYV_ADDR_2, retval);
    
    if(retval != 0)
    {
        /* Reset I2C address to address 3 and write reset */
        keyv_i2c_client->addr = KEYV_ADDR_3;
        retval = i2c_master_send(keyv_i2c_client, value, KEYV_REG_DATA_SIZE);
        printk("KEYV:  Reset to Address 0x%X returned a value of %d\n", KEYV_ADDR_3, retval);
        
        if(retval != 0)
        {
            /* Reset I2C address to address 4 and write reset */
            keyv_i2c_client->addr = KEYV_ADDR_4;
            retval = i2c_master_send(keyv_i2c_client, value, KEYV_REG_DATA_SIZE);
            printk("KEYV:  Reset to Address 0x%X returned a value of %d\n", KEYV_ADDR_4, retval);
        }
    }
    
    /* Reset I2C address back to address 1 */
    keyv_i2c_client->addr = KEYV_ADDR;
    
    /* Increment reset counter */
    rst_counter++;
    
    return (retval);
}

/******************************************************************************
* Global Functions
******************************************************************************/

/*!
 * @brief Initializes communication with the KeyV Chip
 *
 * This function performs any initialization required for the KeyV Chip.  It
 * starts the process mentioned above for initiating I2C communication with the KeyV
 * Chip by registering our I2C driver.  The rest of the process is handled
 * through the callback function adapter_attach().
 *
 * @param reg_init_fcn  Pointer to a function to call when communications are available
 */
void __init keyv_initialize(void (*reg_init_fcn)(void))
{
    /* Save the register initialization function for later */
    keyv_reg_init = reg_init_fcn;
    
    /* Register our driver */
    i2c_add_driver((struct i2c_driver *)&driver);    
}

/*!
 * @brief Reads a specific KeyV Chip Register
 *
 * This function implements a read of a given KeyV register.  The read is done using i2c_transfer
 * which is a set of transactions.  The only transaction will be an I2C read which will read
 * the contents of the register of the KeyV Chip.  
 *
 * @param reg_value  Pointer to which the read data should be stored
 *
 * @return This function returns 0 if successful
 */
int keyv_reg_read (unsigned int *reg_value)
{
    unsigned char value[KEYV_REG_DATA_SIZE];
    int retval;
    int i = 0;
    
    struct i2c_msg msgs[KEYV_REG_DATA_SIZE] = 
    {
	    { keyv_i2c_client->addr, I2C_M_RD, KEYV_REG_DATA_SIZE, value }
    };
    
    /* Fail if we weren't able to initialize (yet) */
    if (keyv_i2c_client == NULL)
    {
        return -EINVAL;
    }

    /* Prevent writes from occurring */
    down(&keyv_rw);
    
    retval = i2c_transfer(keyv_i2c_client->adapter,msgs,KEYV_REG_DATA_SIZE);
    
    if(retval < 0)
    {
        /* Wait 10ms before next transfer */
        msleep(10);
        printk("KEYV:  Read failed with an error code of %d\n", retval);
             
        /* If an error was received then the part is probably not ready and the read was
         * unacknowledged.  In this case try to re-read the register up to 4 times
         * and if the read is still unsuccessful at that point, stop trying */
         for(i = 0; ((i < KEYV_RETRIES) && 
             ((retval = i2c_transfer(keyv_i2c_client->adapter,msgs,KEYV_REG_DATA_SIZE)) < 0)); i++)
         {
             /* Wait 10ms before next transfer */
             msleep(10);
             printk("KEYV:  Read failed with an error code of %d\n", retval);
         }
    }
    
    /* If the IC wont respond, reset */
    if((retval < 0) && (i == KEYV_RETRIES)) 
    {
        i = keyv_reg_write_reset();
        printk("KEYV:  Read failed 5 consecutive times so reset...return value is %d\n", i);
    }
    else
    {
        *reg_value = *(msgs[0].buf);
        retval = 0;
    }
    
    udelay(60);
    
    /* Allow writes to occur */
    up(&keyv_rw);
    
    return retval;
}

/*!
 * @brief Writes a value to a specified KeyV register
 *
 * This function implements a write to a specified KeyV register.  The write is accomplished
 * by sending the new contents to the i2c function i2c_master_send.
 *
 * @param reg_value  Register value to write
 *
 * @return This function returns 0 if successful
 */
int keyv_reg_write(unsigned int reg_value)
{
    unsigned char value[KEYV_REG_DATA_SIZE];
    int retval = 0;
    int i = 0;

    /* Fail if we weren't able to initialize */
    if(keyv_i2c_client == NULL)
    {
        return -EINVAL;
    }
    
    /* Prevent reads from occurring */
    down(&keyv_rw);

    /*Copy the data into a buffer for correct format */
    value[0] = reg_value;
    
    /* Write the data to the KeyV */
    retval = i2c_master_send(keyv_i2c_client, value, KEYV_REG_DATA_SIZE);
    
    if(retval < 0)
    {
        /* Wait 10ms before next transfer */
        msleep(10);
        printk("KEYV:  Write failed with an error code of %d\n", retval);
             
        /* If an error was received then the part is probably not ready and the write was
         * unacknowledged.  In this case try to re-write the register up to 4 times
         * and if the write is still unsuccessful at that point, stop trying */
         for(i = 0; ((i < KEYV_RETRIES) && 
             ((retval = i2c_master_send(keyv_i2c_client, value, KEYV_REG_DATA_SIZE)) < 0)); i++)
         {
             /* Wait 10ms before next transfer */
             msleep(10);
             printk("KEYV:  Write failed with an error code of %d\n", retval);
         }
    }
    
    /* If the IC wont respond, reset */
    if((retval < 0) && (i == KEYV_RETRIES)) 
    {
        i = keyv_reg_write_reset();
        printk("KEYV:  Write failed 5 consecutive times, so reset...return value is %d\n", i);
    }
    
    udelay(60);
    
    /* Allow reads to occur */
    up(&keyv_rw);
    
    return (retval);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
EXPORT_SYMBOL(keyv_reg_read);
EXPORT_SYMBOL(keyv_reg_write);
#endif
