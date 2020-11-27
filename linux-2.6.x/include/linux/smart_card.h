/*
 * Copyright (C) 2005-2007 Motorola, Inc.
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
 * Motorola 2007-May-23 - Add new IOCTLs
 * Motorola 2007-Feb-27 - Update license
 * Motorola 2007-Jan-08 - Update copyright
 * Motorola 2006-Aug-31 - Remove some functionality from the driver
 * Motorola 2006-Aug-21 - Add support for all peripheral clock frequencies
 * Motorola 2006-Jul-25 - More MVL upmerge changes
 * Motorola 2006-Jul-14 - MVL upmerge
 * Motorola 2006-May-24 - Fix GPL issues
 * Motorola 2005-Oct-04 - Initial Creation
 */
 
#ifndef __SMART_CARD_H__
#define __SMART_CARD_H__

#include <linux/ioctl.h>
/*******************************************************************************************
 * Ioctl commands
 *
 *    These are the ioctl commands for the SIM interface module driver. These will not be used directly
 *    from user-space, but are used to construct the request parameters used in ioctl() 
 *    calls to the driver. 
 ******************************************************************************************/

#define SIM_IOC_CMD_CORE_CONFIGURE_GPIO                (0x00)
#define SIM_IOC_CMD_CORE_GET_CWTCNT                    (0x01)
#define SIM_IOC_CMD_CORE_GET_GPCNT                     (0x02)
#define SIM_IOC_CMD_CORE_INTERRUPT_INIT                (0x03)
#define SIM_IOC_CMD_CORE_INTERRUPT_MODE                (0x04)
#define SIM_IOC_CMD_CORE_READ_CARD_READER_DATA         (0x05)
#define SIM_IOC_CMD_CORE_READ_DATA_BUFFER              (0x06)
#define SIM_IOC_CMD_CORE_READ_PERIPHERAL_CLOCK_FREQ    (0x07)
#define SIM_IOC_CMD_CORE_READ_RX_EVENT                 (0x08)
#define SIM_IOC_CMD_CORE_READ_SIMPD_EVENT              (0x09)
#define SIM_IOC_CMD_CORE_READ_SIM_REG_DATA             (0x0A)
#define SIM_IOC_CMD_CORE_RESET_CARD_READER_DATA        (0x0B)
#define SIM_IOC_CMD_CORE_SET_CWTCNT                    (0x0C)
#define SIM_IOC_CMD_CORE_SET_GPCNT                     (0x0D)
#define SIM_IOC_CMD_CORE_SET_VOLTAGE_LEVEL             (0x0E)
#define SIM_IOC_CMD_CORE_UPDATE_DATA_BUFFER            (0x0F)
#define SIM_IOC_CMD_CORE_UPDATE_BUFFER_INDEX           (0x10)
#define SIM_IOC_CMD_CORE_UPDATE_RX_LAST_INDEX          (0x11)
#define SIM_IOC_CMD_CORE_WRITE_SIM_REG_BIT             (0x12)
#define SIM_IOC_CMD_CORE_WRITE_SIM_REG_DATA            (0x13)
#define SIM_IOC_CMD_CORE_ALL_TX_DATA_SENT              (0x14)
#define SIM_IOC_CMD_CORE_RESET_ALL_TX_DATA_SENT        (0x15)
#define SIM_IOC_CMD_CORE_LAST_CMD                       SIM_IOC_CMD_CORE_RESET_ALL_TX_DATA_SENT

/*******************************************************************************************
 * Ioctl requests
 *
 *    These are the requests that can be passed to ioctl() to request operations on the
 *    SIM interface module driver.
 ******************************************************************************************/
#define SIM_IOCTL_CONFIGURE_GPIO \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_CONFIGURE_GPIO, UINT8)
#define SIM_IOCTL_GET_CWTCNT \
        _IOR(SIM_NUM, SIM_IOC_CMD_CORE_GET_CWTCNT, UINT32 *)
#define SIM_IOCTL_GET_GPCNT \
        _IOR(SIM_NUM, SIM_IOC_CMD_CORE_GET_GPCNT, UINT32 *)
#define SIM_IOCTL_INTERRUPT_INIT \
        _IOW(SIM_NUM,  SIM_IOC_CMD_CORE_INTERRUPT_INIT, UINT32 *)
#define SIM_IOCTL_INTERRUPT_MODE \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_INTERRUPT_MODE, UINT8)
#define SIM_IOCTL_READ_CARD_READER_DATA \
        _IOR(SIM_NUM, SIM_IOC_CMD_CORE_READ_CARD_READER_DATA, UINT32 *)
#define SIM_IOCTL_READ_DATA_BUFFER \
        _IOR(SIM_NUM, SIM_IOC_CMD_CORE_READ_DATA_BUFFER, UINT32 *)
#define SIM_IOCTL_READ_PERIPHERAL_CLOCK_FREQ \
        _IOR(SIM_NUM, SIM_IOC_CMD_CORE_READ_PERIPHERAL_CLOCK_FREQ, UINT32 *)
#define SIM_IOCTL_READ_RX_EVENT \
        _IOR(SIM_NUM, SIM_IOC_CMD_CORE_READ_RX_EVENT, UINT32 *)
#define SIM_IOCTL_READ_SIMPD_EVENT \
        _IOR(SIM_NUM, SIM_IOC_CMD_CORE_READ_SIMPD_EVENT, UINT32 *)
#define SIM_IOCTL_READ_SIM_REG_DATA \
        _IOR(SIM_NUM, SIM_IOC_CMD_CORE_READ_SIM_REG_DATA, UINT32 *)
#define SIM_IOCTL_RESET_CARD_READER_DATA \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_RESET_CARD_READER_DATA, UINT32 *)
#define SIM_IOCTL_SET_CWTCNT \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_SET_CWTCNT, UINT32 *)
#define SIM_IOCTL_SET_GPCNT \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_SET_GPCNT, UINT32 *)
#define SIM_IOCTL_SET_VOLTAGE_LEVEL \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_SET_VOLTAGE_LEVEL, UINT32 *)
#define SIM_IOCTL_UPDATE_DATA_BUFFER \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_UPDATE_DATA_BUFFER, UINT8 *)
#define SIM_IOCTL_UPDATE_BUFFER_INDEX \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_UPDATE_BUFFER_INDEX, UINT32 *)
#define SIM_IOCTL_UPDATE_RX_LAST_INDEX \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_UPDATE_RX_LAST_INDEX, UINT32 *)
#define SIM_IOCTL_WRITE_SIM_REG_BIT \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_WRITE_SIM_REG_BIT, UINT32 *)
#define SIM_IOCTL_WRITE_SIM_REG_DATA \
        _IOW(SIM_NUM, SIM_IOC_CMD_CORE_WRITE_SIM_REG_DATA, UINT32 *)
#define SIM_IOCTL_ALL_TX_DATA_SENT \
       _IOR(SIM_NUM, SIM_IOC_CMD_CORE_ALL_TX_DATA_SENT, BOOLEAN *)
#define SIM_IOCTL_RESET_ALL_TX_DATA_SENT \
       _IO(SIM_NUM, SIM_IOC_CMD_CORE_RESET_ALL_TX_DATA_SENT)

#endif /* __SMART_CARD_H__ */
