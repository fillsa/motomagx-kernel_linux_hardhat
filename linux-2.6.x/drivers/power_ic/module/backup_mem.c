/*
 * Copyright (C) 2005-2007 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 * 
 * Motorola 2007-Jun-19 - NAND secure boot time improvement 
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2005-Dec-06 - File Creation.
 */

/*!
 * @file backup_mem.c
 *
 * @ingroup poweric_core
 *
 * @brief The kernel-level interface to access backup memory registers
 *
 * The functions in this file implement the kernel-level interface for reading
 * and writing the backup memory registers.  The register access functions in 
 * external.c are accessed from this file in order to simplify reading/writing
 * to the actual registers.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include "../core/core.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
EXPORT_SYMBOL(power_ic_backup_memory_read);
EXPORT_SYMBOL(power_ic_backup_memory_write);
#endif


/******************************************************************************
* Constants
******************************************************************************/
/*!@cond INTERNAL */
/*!
 * @brief Backup Memory API definition
 *
 *Location A.
 */ 
#define  POWER_IC_BACKUP_MEMORY_A  POWER_IC_REG_ATLAS_MEMORY_A
/*!
 * @brief Backup Memory API definition
 *
 *Location B.
 */
#define  POWER_IC_BACKUP_MEMORY_B  POWER_IC_REG_ATLAS_MEMORY_B

/* Register bit access permission mask*/
#define  SET_BIT       0x01
#define  READ_BIT      0x02
#define  CLEAR_BIT     0x04

/*!@endcond */
 
/******************************************************************************
* Local Structures
******************************************************************************/
/*!
 * @brief Backup Memory API structure for lookup table.
 */
typedef struct
{
    POWER_IC_REGISTER_T      location;      /*!< Memory register element is stored in. */
    unsigned short           offset;        /*!< Element offset from LSB of register. */
    unsigned short           length;        /*!< Length of element in bits. */
    unsigned char            kernel;        /*!< Access permission for kernel-space. */
    unsigned char            user_space;    /*!< Access permission for user-space. */
} POWER_IC_BACKUP_MEMORY_LOOKUP_T;


/******************************************************************************
* Local Variables
******************************************************************************/

/* Lookup table for Backup Memory API based on Backup Memory ID. */
const POWER_IC_BACKUP_MEMORY_LOOKUP_T backup_memory_table[POWER_IC_BACKUP_MEMORY_ID_NUM_ELEMENTS] =
{  
    /* Memory Register         Offset  Length             Kernel                       User Space
                                                    Access Permission              Access Permission*/
    {POWER_IC_BACKUP_MEMORY_A,  0x00,   0x01,   SET_BIT|READ_BIT|CLEAR_BIT,   SET_BIT|READ_BIT|CLEAR_BIT},   /* POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_FLASH_MODE */
    {POWER_IC_BACKUP_MEMORY_A,  0x01,   0x01,   SET_BIT|READ_BIT|CLEAR_BIT,   SET_BIT|READ_BIT|CLEAR_BIT},   /* POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_PANIC */
    {POWER_IC_BACKUP_MEMORY_A,  0x02,   0x01,   SET_BIT|READ_BIT|CLEAR_BIT,   SET_BIT|READ_BIT|CLEAR_BIT},   /* POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_FOTA_MODE */
    {POWER_IC_BACKUP_MEMORY_A,  0x03,   0x01,   READ_BIT,                                       READ_BIT},   /* POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_FLASH_FAIL */
    {POWER_IC_BACKUP_MEMORY_A,  0x04,   0x01,   SET_BIT|READ_BIT,                               READ_BIT},   /* POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_SOFT_RESET */
    {POWER_IC_BACKUP_MEMORY_A,  0x05,   0x01,   READ_BIT,                                       READ_BIT},   /* POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_WDOG_RESET */
    {POWER_IC_BACKUP_MEMORY_A,  0x06,   0x01,   SET_BIT|READ_BIT,                               READ_BIT},   /* POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_ROOTFS_SEC_FAIL */
    {POWER_IC_BACKUP_MEMORY_A,  0x07,   0x01,   SET_BIT|READ_BIT,                               READ_BIT},   /* POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_LANG_PACK_SEC_FAIL */
};

/******************************************************************************
* Global Functions
******************************************************************************/

/*!
 * @brief Read the value of an element stored in backup memory.
 *
 * This function is used to read the value of an element stored in backup memory.  The read value is
 * passed back through the value parameter.
 *
 * @param id    Identifier for what element to read
 * @param value Pointer to the memory location in which to store the element's value
 * @param user  User identification (kernel-space or user-space)
 *
 * @return This function returns 0 if successful.
 */
int power_ic_backup_memory_read_sec(POWER_IC_BACKUP_MEMORY_ID_T id,
                                    int* value,
                                    POWER_IC_USER_T user)
{
    int retval;

    /* Check if this is a valid backup memory element. */
    if (id < POWER_IC_BACKUP_MEMORY_ID_NUM_ELEMENTS)
    {
        /* Check if the user has permission to read the element. */
        if (((user == POWER_IC_USER_KERNEL) && (backup_memory_table[id].kernel & READ_BIT)) ||
            ((user == POWER_IC_USER_USER_SPACE) && (backup_memory_table[id].user_space & READ_BIT)))
        {
            retval = power_ic_get_reg_value(backup_memory_table[id].location,
                                            backup_memory_table[id].offset,
                                            value, backup_memory_table[id].length);
        }
        else
        {
            retval = -EPERM;
        }
    }
    /* Return invalid if ID is not recognized. */
    else
    {
        retval = -EINVAL;
    }

    return retval;
}

/*!
 * @brief Read the value of an element stored in backup memory.
 *
 * This function is used to read the value of an element stored in backup memory.  The read value is
 * passed back through the value parameter.
 *
 * @param id    Identifier for what element to read
 * @param value Pointer to the memory location in which to store the element's value
 *
 * @return This function returns 0 if successful.
 */
int power_ic_backup_memory_read(POWER_IC_BACKUP_MEMORY_ID_T id, int* value)
{
    return power_ic_backup_memory_read_sec(id, value, POWER_IC_USER_KERNEL);
}

/*!
 * @brief Store a value to a backup memory element.
 *
 * This function is used to store a value to a backup memory element.
 *
 * @param id    Identifier for what element to write
 * @param value Value to write to the element
 * @param user  User identification (kernel-space or user-space)
 *
 * @return This function returns 0 if successful.
 */
int power_ic_backup_memory_write_sec(POWER_IC_BACKUP_MEMORY_ID_T id,
                                     int value,
                                     POWER_IC_USER_T user)
{
    int retval;
    int old_value;
    int mask;

    /* Check if this is a valid backup memory element. */
    if (id < POWER_IC_BACKUP_MEMORY_ID_NUM_ELEMENTS)
    {
        /* Read the current state of the element. */
        retval = power_ic_read_reg(backup_memory_table[id].location, &old_value);

        /* Only continue if we read the element successfully (the read value is required for
         * identifying if the user has permission to write to the element).
         */
        if (retval == 0)
        {
            /* define the mask for this element. */
            mask = ((1 << backup_memory_table[id].length) - 1) << backup_memory_table[id].offset;

            /* Clear any extraneous bits in the value to write (bits beyond the length of the
             * element being written), and move the value to the correct position.
             */
            value <<= backup_memory_table[id].offset;
            value &= mask;

            /* Build the new value of the backup memory register that we expect to write. */
            value = (old_value & ~mask) | value;

            /* If the user has set permission, update the old value with the bits that they want to
             * be set.
             */
            if (((user == POWER_IC_USER_KERNEL) && (backup_memory_table[id].kernel & SET_BIT)) ||
                ((user == POWER_IC_USER_USER_SPACE) &&
                 (backup_memory_table[id].user_space & SET_BIT)))
            {
                old_value |= value;
            }

            /* If the user has clear permission, update the old value with the bits that they want
             * to be cleared.
             */
            if (((user == POWER_IC_USER_KERNEL) && (backup_memory_table[id].kernel & CLEAR_BIT)) ||
                ((user == POWER_IC_USER_USER_SPACE) &&
                 (backup_memory_table[id].user_space & CLEAR_BIT)))
            {
                old_value &= value;
            }

            /* If the user's permissions allow them to change the element's old value to the value
             * they want it to be set to, then go ahead and store the element's new value to backup
             * memory.
             */
            if (old_value == value)
            {
                retval = power_ic_set_reg_mask_sec(backup_memory_table[id].location,
                                                   mask,
                                                   value,
                                                   BACKUP_MEMORY_ACCESS_ALLOWED);
            }
            /* If user's permissions do not allow them to change the element's value to the value
             * specified, fail with a permission error.
             */
            else
            {
                retval = -EPERM;
            }
        }
    }
    /* Return invalid if ID is not recognized. */
    else
    {
        retval = -EINVAL;
    }

    return retval;
}

/*!
 * @brief Store a value to a backup memory element.
 *
 * This function is used to store a value to a backup memory element.
 *
 * @param id    Identifier for what element to write
 * @param value Value to write to the element
 *
 * @return This function returns 0 if successful.
 */
int power_ic_backup_memory_write(POWER_IC_BACKUP_MEMORY_ID_T id, int value)
{
    return power_ic_backup_memory_write_sec(id, value, POWER_IC_USER_KERNEL);
}
