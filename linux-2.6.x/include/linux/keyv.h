/*
 * Copyright © 2006-2007, Motorola, All Rights Reserved.
 *
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this list of conditions
 *   and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, this list of
 *   conditions and the following disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * - Neither the name of Motorola nor the names of its contributors may be used to endorse or
 *   promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Motorola 2007-Mar-26 - Handle multi-key pressing
 * Motorola 2006-Oct-10 - Add New Key support
 * Motorola 2006-May-23 - Initial Creation
 */

#ifndef __KEYV_H__
#define __KEYV_H__

#include <stdbool.h>

/******************************************************************************
* Constants
******************************************************************************/
/*! The major number of the KeyV driver. */
#define KEYV_MAJOR_NUM 224

/*! The name of the device in /dev. */
#define KEYV_DEV_NAME "keyv"

/******************************************************************************
* Typedefs/Enumerations
******************************************************************************/
/*! KeyV Keys */
typedef enum
{
    KEYV_KEY_3,
    KEYV_KEY_0,
    KEYV_KEY_1,
    KEYV_KEY_2, 

    KEYV_KEY_MAX,
} KEYV_KEY_T;

/*
 * Define the type of struct for the user space request to set the KeyV key states
 */
typedef struct
{
    KEYV_KEY_T key;     
    bool  en;
} KEYV_KEY_SET_T;

/******************************************************************************
* IOCTLs
******************************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS
/* @{ */

/* Base of all the ioctl() commands that will be handled by the KeyV driver. */
#define KEYV_IOC_CMD_BASE          0x00
/* Last of the range of ioctl() commands reserved for the driver core. */
#define KEYV_IOC_CMD_LAST_CMD      (KEYV_CMD_BASE + 0x0F)

/* @} End of ioctl range constants. -------------------------------------------------------------- */

/*!
 * @brief Resets KeyV
 *
 * This command resets the KeyV part
 *
 * @note int is unused
 */
#define KEYV_IOCTL_RESET \
        _IOW(KEYV_MAJOR_NUM, (KEYV_IOC_CMD_BASE + 0x00), int)
        
/*!
 * @brief Enables/Disables KeyV keys
 *
 * This command enables and disables the KeyV keys
 *
 * This IOCTL takes a structure which will define which key should be enabled/disabled
 */
#define KEYV_IOCTL_KEY_EN \
        _IOW(KEYV_MAJOR_NUM, (KEYV_IOC_CMD_BASE + 0x01), KEYV_KEY_SET_T *)


/*!
 * @brief Reads KeyV register
 *
 * This command reads the KeyV register
 *
 * The returned value is stored in the variable location passed
 */
#define KEYV_IOCTL_READ_REG \
        _IOR(KEYV_MAJOR_NUM, (KEYV_IOC_CMD_BASE + 0x02), unsigned int *)

/*!
 * @brief Writes the KeyV register
 *
 * This command writes the KeyV register
 *
 * The value written is passed in by the user
 */
#define KEYV_IOCTL_WRITE_REG \
        _IOW(KEYV_MAJOR_NUM, (KEYV_IOC_CMD_BASE + 0x03), unsigned int)

/*!
 * @brief Enables/Disables Proximity Sensor
 *
 * This command enables and disables the proximity sensor
 *
 * A 1 for enabling and a 0 for disabling
 */
#define KEYV_IOCTL_PROX_SENSOR_EN \
        _IOW(KEYV_MAJOR_NUM, (KEYV_IOC_CMD_BASE + 0x04), unsigned int)

#endif /* Doxygen */

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Function Prototypes
******************************************************************************/

#endif /* __KEYV_H__ */
