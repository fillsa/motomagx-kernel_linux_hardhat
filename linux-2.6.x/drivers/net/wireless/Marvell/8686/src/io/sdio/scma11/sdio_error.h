/** @file sdio_error.h
 *  @brief Contains the error code 
 * 
 * (c) Copyright � 2003-2006, Marvell International Ltd. 
 *
 * This software file (the "File") is distributed by Marvell International 
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 * (the "License").  You may use, redistribute and/or modify this File in 
 * accordance with the terms and conditions of the License, a copy of which 
 * is available along with the File in the gpl.txt file or by writing to 
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 * this warranty disclaimer.
 *
 */
#ifndef __SDIO_ERROR_H
#define __SDIO_ERROR_H

typedef enum _mmc_error
{
    /* controller errors */
    MMC_ERROR_GENERIC = -10000,
    MMC_ERROR_CRC_WRITE_ERROR = -10001,
    MMC_ERROR_CRC_READ_ERROR = -10002,
    MMC_ERROR_RES_CRC_ERROR = -10003,
    MMC_ERROR_READ_TIME_OUT = -10004,
    MMC_ERROR_TIME_OUT_RESPONSE = -10005,
    MMC_ERROR_INVAL = -10006,
    /* protocol errors reported in card status (R1 response) */
    MMC_ERROR_OUT_OF_RANGE = -10007,
    MMC_ERROR_ADDRESS_ERROR = -10008,
    MMC_ERROR_BLOCK_LEN_ERROR = -10009,
    MMC_ERROR_ERASE_SEQ_ERROR = -10010,
    MMC_ERROR_ERASE_PARAM = -10011,
    MMC_ERROR_WP_VIOLATION = -10012,
    MMC_ERROR_CARD_IS_LOCKED = -10013,
    MMC_ERROR_LOCK_UNLOCK_FAILED = -10014,
    MMC_ERROR_COM_CRC_ERROR = -10015,
    MMC_ERROR_ILLEGAL_COMMAND = -10016,
    MMC_ERROR_CARD_ECC_FAILED = -10017,
    MMC_ERROR_CC_ERROR = -10018,
    MMC_ERROR_ERROR = -10019,
    MMC_ERROR_UNDERRUN = -10020,
    MMC_ERROR_OVERRUN = -10021,
    MMC_ERROR_CID_CSD_OVERWRITE = -10022,
    MMC_ERROR_ERASE_RESET = -10025,
    /* protocol errors reported in R5 response */
    R5_COM_CRC_ERROR = -20000,
    R5_ILLEGAL_COMMAND = -20001,
    R5_ERROR = -20002,
    R5_FUNCTION_NUMBER = -20003,
    R5_OUT_OF_RANGE = -20004,
    /* protocol erros reported in R6 response */
    R6_COM_CRC_ERROR = -30000,
    R6_ILLEGAL_COMMAND = -30001,
    R6_ERROR = -30002,
    R6_AKE_SEQ_ERROR = -30003,
    SDIO_ERROR_R1_RESP = -55000,
    SDIO_ERROR_GENERIC = -40000,
    SDIO_ERROR_R5_RESP = -50000,
    SDIO_R6_ERROR = -60000
} mmc_error_t;

#define	MMC_CARD_STATUS_OUT_OF_RANGE 		(1<<31)
#define	MMC_CARD_STATUS_ADDRESS_ERROR 		(1<<30)
#define	MMC_CARD_STATUS_BLOCK_LEN_ERROR 	(1<<29)
#define	MMC_CARD_STATUS_ERASE_SEQ_ERROR 	(1<<28)
#define	MMC_CARD_STATUS_ERASE_PARAM 		(1<<27)
#define	MMC_CARD_STATUS_WP_VIOLATION 		(1<<26)
#define	MMC_CARD_STATUS_CARD_IS_LOCKED 		(1<<25)
#define	MMC_CARD_STATUS_LOCK_UNLOCK_FAILED 	(1<<24)
#define	MMC_CARD_STATUS_COM_CRC_ERROR 		(1<<23)
#define	MMC_CARD_STATUS_ILLEGAL_COMMAND 	(1<<22)
#define	MMC_CARD_STATUS_CARD_ECC_FAILED 	(1<<21)
#define	MMC_CARD_STATUS_CC_ERROR 		(1<<20)
#define	MMC_CARD_STATUS_ERROR 			(1<<19)
#define	MMC_CARD_STATUS_UNDERRUN 		(1<<18)
#define	MMC_CARD_STATUS_OVERRUN 		(1<<17)
#define	MMC_CARD_STATUS_CID_CSD_OVERWRITE 	(1<<16)
#define	MMC_CARD_STATUS_ERASE_RESET 		(1<<13)

#define MMC_ERROR( args... ) printk(__FUNCTION__); printk("(): " args );

/* SDIO R5 response flag bit definitions */

#define	SDIO_IO_RW_COM_CRC_ERROR		(1<<7)
#define	SDIO_IO_RW_ILLEGAL_COMMAND		(1<<6)
#define	SDIO_IO_RW_ERROR			(1<<3)
#define	SDIO_IO_RW_FUNCTION_NUMBER		(1<<1)
#define	SDIO_IO_RW_OUT_OF_RANGE 		(1<<0)

/* SDIO R6 response flag bit definitions */

#define	SD_R6_CARD_STATUS_COM_CRC_ERROR		(1<<15)
#define	SD_R6_CARD_STATUS_ILLEGAL_COMMAND 	(1<<14)
#define	SD_R6_CARD_STATUS_ERROR 		(1<<13)

#endif /* __SDIO_ERROR_H */
