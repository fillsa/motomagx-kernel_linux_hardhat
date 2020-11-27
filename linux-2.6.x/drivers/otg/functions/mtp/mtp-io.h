/*
 *
 * Copyright (c) 2006 Motorola, Inc, All Rights Reserved.
 *
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   *  Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   *  Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   *  Neither the name of Motorola nor the names of its contributors may
 *      be used to endorse or promote products derived from this software without
 *      specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 03/28/2006         Motorola         MTP initial
 * 04/26/2006         Motorola         Fix MTP ctl message header
 * 10/18/2006         Motorola         Add Open Src Software language
 *
 *
 * mtp-io.h - MTP device library header
 *
 */

#ifndef _MTP_IO_H_
#define _MTP_IO_H_

/* mtpctl related */
#define MTP_CTL_CLASS_REQ    1
#define MTP_CTL_CLASS_REPLY  2

struct mtp_ctl_msg_header {
    int msg_len;
    int msg_id;
};

#define MTP_CTL_MSG_HEADER_SIZE   (sizeof(struct mtp_ctl_msg_header))

/* ioctl related */
#define MTP_EVENT_SIZE   28
struct mtp_event_data {
    unsigned char data[MTP_EVENT_SIZE];
};

#define MTP_IOC_MAGIC    'm'
#define MTP_IOC_MAXNR    10

#define MTP_IOC_EVENT            _IOW(MTP_IOC_MAGIC, 1, struct mtp_event_data)
#define MTP_IOC_SEND_ZLP         _IO(MTP_IOC_MAGIC, 2)
#define MTP_IOC_GET_EP_SIZE_IN   _IOR(MTP_IOC_MAGIC, 3, int)
#define MTP_IOC_GET_VENDOR_FLAG  _IOR(MTP_IOC_MAGIC, 4, int)
#define MTP_IOC_CANCEL_IO        _IO(MTP_IOC_MAGIC, 5)
#define MTP_IOC_DEVICE_RESET     _IO(MTP_IOC_MAGIC, 6)

#endif  /* _MTP_IO_H_ */

