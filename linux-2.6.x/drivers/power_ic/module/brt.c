/*
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * Motorola 2007-Sep-26 - Updated the copyright.
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Jul-31 - Fix some spelling errors in comments
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Feb-06 - File Creation
 */
 
 /*!
 * @file brt.c
 *
 * @ingroup poweric_brt
 * 
 * @brief BRT (Battery ROM Table) interface.
 *
 *
 */
#include <asm/errno.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <linux/power_ic.h>
#include <linux/string.h>
#include <linux/types.h>
#include <stdbool.h>
#include "brt.h"
#include "owire.h"

#include "../core/os_independent.h"

/*!
 * @name Local constants for BRT interface
 */
/*@{*/
#define BIT_ACCESS                       1         /*!< Used as paramter for 1-bit ROM access */
#define BYTE_ACCESS                      8         /*!< Used as paramter for 1-byte ROM access */
#define NUM_BATTERY_COMMAND              3         /*!< Total number of battery commands */

#define SHIFT_RIGHT_MSB_SET              0x80      /*!< Bit 7 (MSB) equals 1 */
#define SHIFT_RIGHT_MSB_CLEAR            0x00      /*!< Bit 7 (MSB) equals 0 */
#define SHIFT_RIGHT_LSB                  0x01      /*!< Bit 0 (LSB) equals 1 */

#define READ_DATA_COMMAND                0xC3      /*!< Reads data command from battery */
#define SEARCH_ROM_COMMAND               0xF0      /*!< Identifies ROM data */
#define DATA_ADDRESS_MSB                 0x00      /*!< Reads MSB of battery data */
#define DATA_ADDRESS_LSB                 0x00      /*!< Reads LSB of battery data */

#define ROM_PART_ID_INDEX                0         /*!< Index of ROM ID */
#define ROM_MANUFACURE_ID_MSB_INDEX      6         /*!< MSB index of manufacturer's ID */
#define ROM_MANUFACURE_ID_LSB_INDEX      5         /*!< LSB index of manufacturer's ID */ 

#define MEM_CRC_LENGTH                   1         /*!< Length of memory CRC check */
#define MEM_CRC_INDEX                    0         /*!< Index of memory CRC check */
#define MEM_BIT_COUNT                    64        /*!< Total number of memory bits */

#define BRT_DALLAS_2502_ID               0x89      /*!< ID of Dallas 2502 Serial EPROM */
#define BRT_MANUFACURE_ID_MSB            0x50      /*!< MSB of manufacturer's ID */
#define BRT_MANUFACURE_ID_LSB            0x00      /*!< LSB of manufacturer's ID */

#define MAX_NUM_RETRY_READ               4         /*!< Number of detection retry attempts */

#define BATT_AUX_SEL                     0x2F      /*!< Battery auxilary select */
#define MAIN_BATT_PRESENT                0         /*!< Battery present */

#define NUM_SERIAL_NUMBER_DATA           3         /*!< Serial Number Data Number */
#define NUM_SECURITY_EPROM_DATA          8         /*!< Security Eprom Data Number */

#define BRT_PAGE_SIZE                    0x20      /*!< Memory page size */
/*@}*/

/*!
 * @brief This structure defines the states of read procedure on battery rom
 */
enum
{
    BATT_DETECT = 0,
    BATT_IDENTIFY,
    BATT_SEND_COMMAND,
    BATT_RX_DATA,
    BATT_READ_DONE,
    BATT_RETRY_DETECT
};

/*!
 * @brief  This is the list of commands used to read battery data from one wire device
 */
const u8 battery_command[NUM_BATTERY_COMMAND] = {
    READ_DATA_COMMAND,
    DATA_ADDRESS_LSB, 
    DATA_ADDRESS_MSB };

/*! Local variables */
u8 battery_serial_numbers[NUM_SERIAL_NUMBER_DATA][NUM_SECURITY_EPROM_DATA];
static u8 battery_rom_unique_id[BRT_UID_SIZE];
static int battery_rom_device_type = BRT_NONE;
int conflict_bit_index;

/*!
 * @brief This macro performs 1-bit right rotation on byte x
 */ 
#define ror1(x) (x = ((x >> 1) | ((x & 1) << 7)))

/*!
 * @brief brt_read_bus() read one wire bus.
 *
 * This function reads data and/or CRC from the battery eprom on the 1 wire bus
 *
 * @param        *data     pointer to data.
 * @param        num_data  data length
 */
static void brt_read_bus(u8 *data, u8 num_data )
{
    int count;
    
    for (count=0; count<num_data; count++) 
    {
        data[count] = owire_read(BYTE_ACCESS);
    }
}

/*!
 * @brief brt_8bits_crc() compute crc.
 *
 * This function computes the 8bits CRC which will be compared with the CRC 
 * from the battery eprom.
 *
 * @param        *data_ptr pointer to the data
 * @param        num_data  data length
 *
 * @return result of crc
 */
u8 brt_8bits_crc(u8 *data_ptr, int num_data)
{
     u8 i;
     u8 scratch;
     u8 crc = 0;

     while (num_data > 0)
     {
          scratch = *data_ptr++;
          for (i = 0; i <= 7; i++)
          {
               if ((crc ^= (scratch & 1)) & 1)
               {
                    crc ^= 0x18;
               }
               ror1(crc);
               ror1(scratch);
          }
          num_data--;
     }
     return(crc);
}

/*!
 * @brief brt_shift_left64() 64 bit shift operation.
 *
 * This routine shifts byte left and bit right. The first
 * bit shifted in will end up as the right most bit in the
 * left most byte.  The last bit shifted in will end up in
 * the left most bit position of the right most byte.
 *
 * @param        serial_number_index    pointer to brt serial number index 
 * @param        bit
 */
static void brt_shift_left64(u8 serial_number_index, u8 bit)
{
    int index;
    int shifted_bit;

    for (index = 0; index < NUM_SECURITY_EPROM_DATA-1; index++)
    {
        if ((battery_serial_numbers[serial_number_index][index+1] & 
             SHIFT_RIGHT_LSB) == SHIFT_RIGHT_LSB)
        {
            shifted_bit = SHIFT_RIGHT_MSB_SET;
        }
        else
        {
            shifted_bit = SHIFT_RIGHT_MSB_CLEAR;
        }
        battery_serial_numbers[serial_number_index][index] = shifted_bit | 
            battery_serial_numbers[serial_number_index][index] >> 1;
    }
    
    battery_serial_numbers[serial_number_index][NUM_SECURITY_EPROM_DATA-1] = 
        (bit << 7) | battery_serial_numbers[serial_number_index][NUM_SECURITY_EPROM_DATA-1] >> 1;
}


/*!
 * @brief brt_detect() detect brt present.
 *
 * This function detects if battery rom presents.
 *
 * @return state of brt operation.
 */
u8 brt_detect(void)
{
    u8 next_state;

    if (owire_detect())
    {
        next_state = BATT_IDENTIFY;
    }
    else
    {
        next_state = BATT_RETRY_DETECT;
    }

    return(next_state);
}

/*!
 * @brief brt_identify() identify brt.
 *
 * This function identifies the battery rom data and determines if valid battery present.
 *
 * @param        serial_number_index_ptr    pointer to brt serial number index 
 * @param        multiple_devices_found_ptr pointer to the result of multiple device detection
 *
 * @return state of brt operation.
 */
static u8 brt_identify(u8 *serial_number_index_ptr, 
                       bool *multiple_devices_found_ptr)
{
    u8 next_state = BATT_SEND_COMMAND;
    u8 bit_index;
    u8 read_bit;
    u8 read_complement_bit;              
    u8 bit = 0;
    u8 temp_data;
    u8 family_code;
    u8 uid_lsb;
    u8 uid_msb;
    bool error_found = 0;

    owire_write(SEARCH_ROM_COMMAND, BYTE_ACCESS);

    bit_index = 0;
        
    while (( bit_index < MEM_BIT_COUNT) && (error_found == 0))
    {
        /* Read the next bit and its complement from the 1-wire bus. The bit read will be
         * in bit 0 of temp_data, and the complement will be in bit 1 of temp_data.
         */
        temp_data = owire_read(2);

        read_bit = temp_data & 0x01;
        read_complement_bit = (temp_data >> 1) & 0x01;

        /* Determine if all devices have fallen off the bus. */
        if ((read_bit == 1) && (read_complement_bit == 1))
        {
            next_state = BATT_RETRY_DETECT; 
            
            /* Cause loop to be aborted */
            error_found = 1;
        }
        
        /* Determine if there is a bit conflict. */
        else if ((read_bit == 0) && (read_complement_bit == 0))
        {
            /* Indicate that there are multiple devices on the bus. */
            *multiple_devices_found_ptr = 1;
           
            /* Assume there is an old conflict */
            bit = 1; /* Write a one */

            /* Determine if this is the first serial number found or if this is a new conflict */
            if ((*serial_number_index_ptr == 0) ||
                ((conflict_bit_index < bit_index) &&
                ((battery_serial_numbers[(*serial_number_index_ptr)-1][bit_index >> 3] & (1 << (bit_index & 0x07))) != 0)))
            {
                /* New conflict found so move conflict_bit_index to the current bit position. */
                bit = 0; /* Write a zero */
                conflict_bit_index = bit_index;
            }
        }
        else
        {
           bit = read_bit; /* Set bit to write as the 1st bit read */
        }

        /* Write the matching bit value. */
        owire_write(bit, BIT_ACCESS);

        /* Or in the new bit value. */
        brt_shift_left64(*serial_number_index_ptr, bit);

        /* Increment the bit index */
        bit_index++;
    }

    /* Add a check of the Family Code to determine if the battery
     * contains a Dallas 2502 Serial EPROM, which contains the battery
     * serial number.
     */
    family_code = ( battery_serial_numbers[*serial_number_index_ptr][ROM_PART_ID_INDEX] ) & 0xFF;
    if (family_code == BRT_DALLAS_2502_ID)
    {
        /* Check if it is a Motorola battery */
        uid_msb = (battery_serial_numbers[*serial_number_index_ptr][ROM_MANUFACURE_ID_MSB_INDEX]);
        uid_lsb = (battery_serial_numbers[*serial_number_index_ptr][ROM_MANUFACURE_ID_LSB_INDEX])&0xF0; 
        
        /* if battery is correct type and battery serial number is valid */
        if ((uid_lsb == BRT_MANUFACURE_ID_LSB) && (uid_msb == BRT_MANUFACURE_ID_MSB))
        {
            battery_rom_device_type = BRT_W_DATA;
        }
        else 
        {
            battery_rom_device_type = BRT_INVALID;
            next_state = BATT_RETRY_DETECT; 
        }
    }
    else/* Retry in case of an error */
    {
        battery_rom_device_type = BRT_INVALID;
        next_state = BATT_RETRY_DETECT;
    }

    return(next_state);
}

/*!
 * @brief brt_send_command() send commands to battery eprom
 *
 * This function sends commands to the battery eprom through
 * 1 wire bus, and checking CRC.
 *
 * @return state of brt operation.
 */
static u8 brt_send_command( void )
{
    u8 next_state = BATT_RETRY_DETECT;
    u8 current_command;
    u8 temp_battery_data[BRT_BYTE_SIZE+1];
    u8 command_count;
    u8 computed_crc;
    
    for (command_count=0; command_count<NUM_BATTERY_COMMAND; command_count++) 
    {
        current_command = battery_command[command_count];
        owire_write(current_command, BYTE_ACCESS);
    }

    /* Time to read CRC(1byte) for MEMORY FUNCTION COMMAND. This will be
     * compared with the computed CRC by the master.
     */
 
    /* Read the bus */
    brt_read_bus(&temp_battery_data[0], MEM_CRC_LENGTH);

    /* The CRC will be computed in the radio. */
    computed_crc = brt_8bits_crc( (u8 *)(battery_command), NUM_BATTERY_COMMAND);

    /* Check the CRC and ADDRESS. */
    if (temp_battery_data[MEM_CRC_INDEX] == computed_crc)
    {
        /* Ready to receive 32 bytes data */
        next_state = BATT_RX_DATA;
    }

    return(next_state);
}

/*!
 * @brief brt_rx_data() read brt data.
 *
 * This function reads the battery table from the serial eprom and determines
 * if this battery is the one that was requested.
 *
 * @param        *dest_addr                 returned data per request.
 * @param        serial_number_index_ptr    pointer to brt serial number index 
 * @param        multiple_devices_found     indication of multiple devices detected
 *
 * @return state of brt operation.
 */
static u8 brt_rx_data (u8 *dest_addr, 
                       u8 *serial_number_index_ptr, 
                       bool multiple_devices_found)
{
    u8 next_state = BATT_RETRY_DETECT;
    u8 computed_crc;
    u8 temp_battery_data[BRT_BYTE_SIZE+1];
    u8 temp_battery_data_index = 0;
    bool crc_success = 0;
    
    while(temp_battery_data_index < BRT_BYTE_SIZE)
    {
        /* Read data from the bus, including CRC. */
        brt_read_bus(&temp_battery_data[temp_battery_data_index], BRT_PAGE_SIZE+1);
        
        /* Compute CRC by the radio. */
        computed_crc = brt_8bits_crc(&temp_battery_data[temp_battery_data_index], BRT_PAGE_SIZE);
        
        /* Check the CRC */
        if (computed_crc == temp_battery_data[temp_battery_data_index + BRT_PAGE_SIZE])
        {
            crc_success = 1;
        }
        else
        {
            /* CRC failure try again */
            crc_success = 0;
            break;
        }
        temp_battery_data_index += BRT_PAGE_SIZE;
    }
    if(crc_success == 1)
    {
        /* Determine if this device is the battery */
        if (temp_battery_data[BATT_AUX_SEL] == MAIN_BATT_PRESENT)
        {
            /* Copy the temporary buffer to the battery ram table. */
            memcpy(dest_addr, &temp_battery_data[0], BRT_BYTE_SIZE);
            
            /* Copy the unique id of the battery that was just read */
            memcpy(battery_rom_unique_id, 
                   &battery_serial_numbers[*serial_number_index_ptr][0],
                   BRT_UID_SIZE);
            
            next_state = BATT_READ_DONE; /* Set state to battery read done. */ 
        }
        else /* Data read was not from requested battery */
        {
            /* Increment the serial number index. */
            *serial_number_index_ptr = *serial_number_index_ptr + 1;
            
            /* If multiple devices were detected on the bus, then check the next device for the data. */
            if ((multiple_devices_found) && (*serial_number_index_ptr < NUM_SERIAL_NUMBER_DATA)) 
            {
                /* Return to the battery identify state */
                next_state = BATT_IDENTIFY;
                
                /* Reset the battery interface. */
                owire_detect();
            }
            else /* Only one device and not the one to be read, so exit with read failure */
            {
                battery_rom_device_type = BRT_NONE;
                next_state = BATT_READ_DONE;
            }   
        }
    }
    return(next_state);
}

/*!
 * @brief brt_retry_detect() re-try battery rom detection
 *
 * This function attempts to retry a detection of battery rom on one wire bus
 *
 * @param        *retry_count_ptr           pointer to the counter for re-detection.
 * @param        serial_number_index_ptr    pointer to brt serial number index 
 * @param        multiple_devices_found     indication of multiple devices detected
 *
 * @return state of brt operation.
 */
static u8 brt_retry_detect(u8* retry_count_ptr, 
                           u8* serial_number_index_ptr, 
                           bool multiple_devices_found)
{
    /* BATT_DETECT is default state for next_state. */
    u8 next_state = BATT_DETECT;
    
    (*retry_count_ptr)++;
    
    if (*retry_count_ptr >= MAX_NUM_RETRY_READ)
    {
        /* Increment the serial number index. */                    
        (*serial_number_index_ptr)++;
        
        /* If more than 1 device was found on the bus and if all devices have not been checked then
         * read the next device.
         */
        if ((multiple_devices_found) && (*serial_number_index_ptr < NUM_SERIAL_NUMBER_DATA)) 
        {
            /* Clear retry counter to check next device. */
            *retry_count_ptr = 0;
            
            /* Reset the battery interface. */
            owire_detect();
            
            /* Return to the battery identify state. */
            next_state = BATT_IDENTIFY;
        }
    }
    else
    {
        /* If more than one device was found on the bus, first do retries
         * on the current device.
         */
        if (multiple_devices_found)
        {
            /* Reset the battery interface. */
            owire_detect();
            next_state = BATT_IDENTIFY;
        }
    }
    return(next_state);
}


/*!
 * @brief brt_data_read() read battery rom data.
 *
 * This function performs a read operation on battery rom
 *
 * @param        *dest_addr   pointer to the received battery rom data
 *
 * @return the status of battery rom.
 */
unsigned int brt_data_read(unsigned char *dest_addr)
{ 
    u8 retry_count = 0;
    u8 state = BATT_DETECT;
    u8 serial_number_index;
    static bool multiple_devices_found = 0;

    battery_rom_device_type = BRT_NONE;
    
    /* Clear unique id */
    memset(battery_rom_unique_id, 0xFF, BRT_UID_SIZE);
    
    while ( (retry_count < MAX_NUM_RETRY_READ) && (state != BATT_READ_DONE) )
    {
        switch(state)
        {
            case BATT_DETECT:
                serial_number_index = 0;
                conflict_bit_index = 0;
                state = brt_detect();
                break;
                
            case BATT_IDENTIFY:
                state = brt_identify(&serial_number_index, &multiple_devices_found);
                break;
                
            case BATT_SEND_COMMAND:
                state = brt_send_command();
                break;
                
            case BATT_RX_DATA:
                state = brt_rx_data(dest_addr, &serial_number_index, multiple_devices_found);
                break;
                
            case BATT_RETRY_DETECT:
            default: 
                state = brt_retry_detect(&retry_count, &serial_number_index, multiple_devices_found);
                break;
        }
    }
    return(battery_rom_device_type);
}


/*!
 * @brief brt_uid_read() read battery unique id
 *
 * This function gets the battery unique id number. 
 * This function should be called after brt_data_read being called
 *
 * @param        *dest_addr   returned uid data per request.
 *
 * @return the status of battery rom
 */
unsigned int brt_uid_read(unsigned char *dest_addr)
{
    /* Copy the unique id of the battery that was just read */
    memcpy(dest_addr, battery_rom_unique_id, BRT_UID_SIZE);
    return(battery_rom_device_type);
}

/*!
 * @brief brt_ioctl - handler for the BRT interface.
 *
 * This function is the ioctl() interface handler for all BRT operations. It is not called
 * directly through an ioctl() call on the power IC device, but is executed from the core ioctl
 * handler for all ioctl requests in the range for BRT operations.
 *
 * @param        cmd       ioctl() request received.
 * @param        arg       typically points to structure giving further details of data.
 *
 * @return 0 if successful.
 */
int brt_ioctl(unsigned int cmd, unsigned long arg)
{
    int temp[sizeof(POWER_IC_BRT_DATA_REQUEST_T)];

    /* Handle the request. */
    switch(cmd)
    {
        case POWER_IC_IOCTL_CMD_BRT_DATA_READ:
            tracemsg(_k_d("=> Read BRT data"));
            
            ((POWER_IC_BRT_DATA_REQUEST_T *)temp)->brt_type = 
                brt_data_read(((POWER_IC_BRT_DATA_REQUEST_T *)temp)->brt_data);
            
            /* Return the read result back to the user. */
            if(copy_to_user( (void *) arg, (void *) temp, 
                             sizeof(POWER_IC_BRT_DATA_REQUEST_T)) != 0)
            {
                tracemsg(_k_d("   error copying brt data results to user space."));
                return -EFAULT;
            }     

            break;
            
        case POWER_IC_IOCTL_CMD_BRT_UID_READ:
            tracemsg(_k_d("=> Read BRT UID"));
            
            ((POWER_IC_BRT_UID_REQUEST_T *)temp)->brt_type =
                brt_uid_read(((POWER_IC_BRT_UID_REQUEST_T *)temp)->brt_uid);
            
            /* Return the read result back to the user. */
            if(copy_to_user( (void *) arg, (void *) temp, 
                             sizeof(POWER_IC_BRT_UID_REQUEST_T)) != 0)
            {
                tracemsg(_k_d("   error copying brt uid results to user space."));
                return -EFAULT;
            }

            break;
        default: /* This shouldn't be able to happen, but just in case... */
            tracemsg(_k_d("=> 0x%X unsupported BRT ioctl command"), (int) cmd);
            return -ENOTTY;
            break;
            
    }
    return 0;
}
