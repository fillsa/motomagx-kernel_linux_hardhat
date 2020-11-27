/*
 * otg/functions/msc/msc-io.h - Mass Storage Class
 * @(#) balden@seth2.belcarratech.com|otg/functions/msc/msc-io.h|20051116204958|31159
 *
 *      Copyright (c) 2003-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 *  Copyright (c) 2005-2006, Motorola, All Rights Reserved.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 04/18/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 *
 *  This program is licensed under a BSD license with the following terms:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 *  Neither the name of Motorola nor the names of its contributors may be
 *  used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*!
 * @file otg/functions/msc/msc-io.h
 * @brief Mass Storage Driver private defines
 *
 * OTGMSC_START                 - make specified major/minor device available to USB Host
 * OTGMSC_WRITEPROTECT          - set or reset write protect flag
 *
 * OTGMSC_STOP                  - remove access to current device, block until pending I/O finished
 * OTGMSC_STATUS                - remove current USB connection status (connected or disconnected)
 * OTGMSC_WAIT_CONNECT          - wait until device is connected (may return immediately if already connected)
 * OTGMSC_WAIT_DISCONNECT       - wait until device is disconnected (may return immediately if already disconnected)
 *
 * @ingroup MSCFunction
 */

#ifndef MSC_IO_H
#define MSC_IO_H 1

#include "msc-scsi.h"

struct otgmsc_mount {
        int     major;
        int     minor;
        int     lun;
        int     writeprotect;
        int     result;
        int     status;
};

#define OTGMSC_MAGIC    'M'
#define OTGMSC_MAXNR	10

#define OTGMSC_START            _IOWR(OTGMSC_MAGIC, 1, struct otgmsc_mount)
#define OTGMSC_WRITEPROTECT     _IOWR(OTGMSC_MAGIC, 2, struct otgmsc_mount)
#define OTGMSC_GET_UNKNOWN_CMD  _IOWR(OTGMSC_MAGIC, 3, COMMAND_BLOCK_WRAPPER)

#define OTGMSC_STOP             _IOR(OTGMSC_MAGIC, 1, struct otgmsc_mount)
#define OTGMSC_STATUS           _IOR(OTGMSC_MAGIC, 2, struct otgmsc_mount)
#define OTGMSC_WAIT_CONNECT     _IOR(OTGMSC_MAGIC, 3, struct otgmsc_mount)
#define OTGMSC_WAIT_DISCONNECT  _IOR(OTGMSC_MAGIC, 4, struct otgmsc_mount)



#define MSC_CONNECTED           0x01
#define MSC_DISCONNECTED        0x02
#define MSC_WRITEPROTECTED      0x04


#endif /* MSC_IO_H */

