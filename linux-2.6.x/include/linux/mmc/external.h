/*
 * Copyright © 2007, Motorola, All Rights Reserved.
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
 */
/*
 * Copyright (C) 2007-2008 Motorola, Inc
 * Date         Author          Comment
 * 01/26/2007	Motorola	Initial Creation
 * 09/23/2008	Motorola	Support shredding card
 */

#ifndef __MMC_EXTERNAL_H__
#define __MMC_EXTERNAL_H__

/* Below will be exported to external driver or user applications */
/* max 8 partitions per card */
#define MMC_SHIFT               3
#define MMC_BLK_DEV_NUM         243

#define MAX_MMC_CARD_NO 2
#define MMC_DEV_NODE_0 "/dev/mmc/blk0/disc"
#define MMC_DEV_NODE_1 "/dev/mmc/blk1/disc"

#define TYPE_MMC        0x00
#define TYPE_SDT        0x01
#define TYPE_DOC        0x02
#define TYPE_RAM        0x03
#define TYPE_NAND       0x04
#define TYPE_HCSD	0x05

#define MMC_REG_CID_LEN		16
#define MMC_REG_CSD_LEN 	16
#define MMC_REG_OCR_LEN 	4
#define MMC_REG_RCA_LEN 	2
#define MMC_REG_EXTCSD_LEN	512

#define IOCMMCGETCARDCID    _IOR(MMC_BLK_DEV_NUM, 0x0f02, char *)
#define IOCMMCGETCARDSTATUS _IOWR(MMC_BLK_DEV_NUM, 0x0f05, unsigned long *)
#define IOCMMC_STATUS_WRITE_PROTECTED    0x01
#define IOCMMC_STATUS_LOCKED             0x02
#define IOCMMC_STATUS_LOCK_FAILED        0x04
#define IOCMMC_STATUS_LOCK_NOT_SUPPORTED 0x08
#define IOCMMC_STATUS_MEDIA_CHANGED      0x10
#define IOCMMCGETCARDTYPE   _IOR(MMC_BLK_DEV_NUM, 0x0f06, unsigned long *)
#define IOCMMCGETSIZE       _IOR(MMC_BLK_DEV_NUM, 0x0f09, unsigned long *)
#define IOCMMCGETCARDCSD    _IOR(MMC_BLK_DEV_NUM, 0x0f0a, char *)
#define IOCMMCGETCARDOCR    _IOR(MMC_BLK_DEV_NUM, 0x0f0b, char *)
#define IOCMMCGETCARDDSR    _IOR(MMC_BLK_DEV_NUM, 0x0f0c, char *)
#define IOCMMCLOCKCARD	    	_IOR(MMC_BLK_DEV_NUM, 0x0f1a, unsigned long *)
#define IOCMMCUNLOCKCARD	_IOR(MMC_BLK_DEV_NUM, 0x0f1b, unsigned long *)
#define IOCMMCERASECARD		_IOR(MMC_BLK_DEV_NUM, 0x0f1c, unsigned long *)

#define MEGASIM_BOOTUP                _IO(MMC_BLK_DEV_NUM, 0x0e00)
#define MEGASIM_RESET_REQ             _IO(MMC_BLK_DEV_NUM, 0x0e01)

#endif /* __MMC_EXTERNAL_H__ */
