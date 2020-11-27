/** @file sdhc.h
 *  @brief Definitions for SD Controller's register
 * 
 * (c) Copyright © 2003-2006, Marvell International Ltd. 
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
#ifndef _SDHC_REG_
#define _SDHC_REG_

#define	MMC_CLKRT_FREQ_9_75MHZ		(1 << 0)
#define	MMC_CLKRT_FREQ_19_5MHZ		(0 << 0)

#define	MMC_RES_TO_MAX 			(0x007fUL)

#define	CMD(x)				(x)

#define SDHC_NR                         1

#define SDHC_MMC_WML                    16
#define SDHC_SD_WML                     64
#define DRIVER_NAME                     "MXC-SDIO"
#define SDHC_MEM_SIZE                   16384
#define SDHC_REV_NO                     0x400
#define READ_TO_VALUE                   0x2db4

/* Address offsets of the SDHC registers */
#define MMC_STR_STP_CLK                 0x00    /* Clock Control Reg */
#define MMC_STATUS                      0x04    /* Status Reg */
#define MMC_CLK_RATE                    0x08    /* Clock Rate Reg */
#define MMC_CMD_DAT_CONT                0x0C    /* Command and Data Control Reg */
#define MMC_RES_TO                      0x10    /* Response Time-out Reg */
#define MMC_READ_TO                     0x14    /* Read Time-out Reg */
#define MMC_BLK_LEN                     0x18    /* Block Length Reg */
#define MMC_NOB                         0x1C    /* Number of Blocks Reg */
#define MMC_REV_NO                      0x20    /* Revision Number Reg */
#define MMC_INT_CNTR                    0x24    /* Interrupt Control Reg */
#define MMC_CMD                         0x28    /* Command Number Reg */
#define MMC_ARG                         0x2C    /* Command Argument Reg */
#define MMC_RES_FIFO                    0x34    /* Command Response Reg */
#define MMC_BUFFER_ACCESS               0x38    /* Data Buffer Access Reg */
#define MMC_REM_NOB                     0x40    /* Remaining NOB Reg */
#define MMC_REM_BLK_SIZE                0x44    /* Remaining Block Size Reg */

/* Bit definitions for STR_STP_CLK */
#define STR_STP_CLK_RESET               (1<<3)
#define STR_STP_CLK_START_CLK           ((1<<1)|(1<<15)|(1<<14))
#define STR_STP_CLK_STOP_CLK            ((1<<0)|(1<<15)|(1<<14))

#define	MMC_CMDAT_DATA_EN		(0x0001UL << 3)
#define	MMC_CMDAT_BLOCK			(0x0000UL << 4)
#define	MMC_CMDAT_WR_RD			(0x0001UL << 3)
#define	MMC_CMDAT_READ			(0x0000UL << 3)
#define	MMC_CMDAT_INIT			(0x0001UL << 7)
#define	MMC_CMDAT_SD_4DAT		(1 << 9)
#define	MMC_CMDAT_SD_1DAT		(0x0000UL << 8)

#define	MMC_CMDAT_R1			(0x0001UL)
#define	MMC_CMDAT_R2			(0x0002UL)
#define	MMC_CMDAT_R3            	(0x0003UL)

#define CISTPL_MANFID                   0x20
#define CISTPL_END                      0xff

/* Bit definitions for STATUS */
#define STATUS_CARD_INSERTION           (1<<31)
#define STATUS_CARD_REMOVAL             (1<<30)
#define STATUS_YBUF_EMPTY               (1<<29)
#define STATUS_XBUF_EMPTY               (1<<28)
#define STATUS_YBUF_FULL                (1<<27)
#define STATUS_XBUF_FULL                (1<<26)
#define STATUS_BUF_UND_RUN              (1<<25)
#define STATUS_BUF_OVFL                 (1<<24)
#define STATUS_SDIO_INT_ACTIVE          (1<<14)
#define STATUS_END_CMD_RESP             (1<<13)
#define STATUS_WRITE_OP_DONE            (1<<12)
#define STATUS_READ_OP_DONE             (1<<11)
#define STATUS_WR_CRC_ERROR_CODE_MASK   (3<<10)
#define STATUS_CARD_BUS_CLK_RUN         (1<<8)
#define STATUS_BUF_READ_RDY             (1<<7)
#define STATUS_BUF_WRITE_RDY            (1<<6)
#define STATUS_RESP_CRC_ERR             (1<<5)
#define STATUS_READ_CRC_ERR             (1<<3)
#define STATUS_WRITE_CRC_ERR            (1<<2)
#define STATUS_TIME_OUT_RESP            (1<<1)
#define STATUS_TIME_OUT_READ            (1<<0)
#define STATUS_ERR_MASK                 0x3f

#define MMC_STAT_ERRORS ( STATUS_RESP_CRC_ERR |STATUS_READ_CRC_ERR \
	|STATUS_WRITE_CRC_ERR | STATUS_TIME_OUT_RESP | STATUS_TIME_OUT_READ )

/* Clock rate definitions */
#define CLK_RATE_PRESCALER(x)           ((x) & 0xF)
#define CLK_RATE_CLK_DIVIDER(x)         (((x) & 0xF) << 4)

/* Bit definitions for CMD_DAT_CONT */
#define CMD_DAT_CONT_CMD_RESP_LONG_OFF  (1<<12)
#define CMD_DAT_CONT_STOP_READWAIT      (1<<11)
#define CMD_DAT_CONT_START_READWAIT     (1<<10)
#define CMD_DAT_CONT_BUS_WIDTH_1        (0<<8)
#define CMD_DAT_CONT_BUS_WIDTH_4        (2<<8)
#define CMD_DAT_CONT_INIT               (1<<7)
#define CMD_DAT_CONT_WRITE              (1<<4)
#define CMD_DAT_CONT_DATA_ENABLE        (1<<3)
#define CMD_DAT_CONT_RESPONSE_FORMAT_R1 (1)
#define CMD_DAT_CONT_RESPONSE_FORMAT_R2 (2)
#define CMD_DAT_CONT_RESPONSE_FORMAT_R3 (3)
#define CMD_DAT_CONT_RESPONSE_FORMAT_R4 (4)
#define CMD_DAT_CONT_RESPONSE_FORMAT_R5 (5)
#define CMD_DAT_CONT_RESPONSE_FORMAT_R6 (6)

/* Bit definitions for INT_CNTR */
#define INT_CNTR_SDIO_INT_WKP_EN        (1<<18)
#define INT_CNTR_CARD_INSERTION_WKP_EN  (1<<17)
#define INT_CNTR_CARD_REMOVAL_WKP_EN    (1<<16)
#define INT_CNTR_CARD_INSERTION_EN      (1<<15)
#define INT_CNTR_CARD_REMOVAL_EN        (1<<14)
#define INT_CNTR_SDIO_IRQ_EN            (1<<13)
#define INT_CNTR_DAT0_EN                (1<<12)
#define INT_CNTR_BUF_READ_EN            (1<<4)
#define INT_CNTR_BUF_WRITE_EN           (1<<3)
#define INT_CNTR_END_CMD_RES            (1<<2)
#define INT_CNTR_WRITE_OP_DONE          (1<<1)
#define INT_CNTR_READ_OP_DONE           (1<<0)

#ifdef MOTO_PLATFORM
#define SDHC1_MODULE 0
#define SDHC2_MODULE 1
#endif

#endif /* _SDHC_REG_H */
