/** @file if_sdio.c
 *  @brief This file contains SDIO IF (interface) module
 *  related functions.
 * 
 * (c) Copyright � 2003-2007, Marvell International Ltd. 
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
/****************************************************
Change log:
	10/14/05: add Doxygen format comments 
	01/05/06: add kernel 2.6.x support
	01/23/06: add fw downlaod
	06/06/06: add macro SD_BLOCK_SIZE_FW_DL for firmware download
		  add macro ALLOC_BUF_SIZE for cmd resp/Rx data skb buffer allocation
****************************************************/

#include	"if_sdio.h"

/* define SD block size for firmware download */
/* the minium block size must be 64 */
#define SD_BLOCK_SIZE_FW_DL	64

/* define SD block size for data Tx/Rx */
#define SD_BLOCK_SIZE		320     /* To minimize the overhead of ethernet frame
                                           with 1514 bytes, 320 bytes block size is used */

#define ALLOC_BUF_SIZE		(((MAX(MRVDRV_ETH_RX_PACKET_BUFFER_SIZE, \
					MRVDRV_SIZE_OF_CMD_BUFFER) + SDIO_HEADER_LEN \
					+ SD_BLOCK_SIZE - 1) / SD_BLOCK_SIZE) * SD_BLOCK_SIZE)

/* Max retry number of CMD53 write */
#define MAX_WRITE_IOMEM_RETRY	2

/* The macros below are hardware platform dependent.
   The definition should match the actual platform */
#define GPIO_PORT_NO
#define GPIO_PORT_INIT()
#ifdef WPRM_DRV
#define GPIO_PORT_TO_HIGH() wprm_trigger_wlan_client_wakeb(WPRM_SIGNAL_RELEASE)
#define GPIO_PORT_TO_LOW() wprm_trigger_wlan_client_wakeb(WPRM_SIGNAL_TRIGGER)
#else
#define GPIO_PORT_TO_HIGH()
#define GPIO_PORT_TO_LOW()
#endif /* WPRM_DRV */

/********************************************************
		Local Variables
********************************************************/

static wlan_private *pwlanpriv;
static wlan_private *(*wlan_add_callback) (void *dev_id);
static int (*wlan_remove_callback) (void *dev_id);

/********************************************************
		Global Variables
********************************************************/

mmc_notifier_rec_t if_sdio_notifier;
isr_notifier_fn_t isr_function;

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief This function adds the card
 *  
 *  @param card    A pointer to the card
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sbi_add_card(void *card)
{
    if (!wlan_add_callback)
        return WLAN_STATUS_FAILURE;

    pwlanpriv = wlan_add_callback(card);

    if (pwlanpriv)
        return WLAN_STATUS_SUCCESS;
    else
        return WLAN_STATUS_FAILURE;
}

/** 
 *  @brief This function removes the card
 *  
 *  @param card    A pointer to the card
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sbi_remove_card(void *card)
{
    if (!wlan_remove_callback)
        return WLAN_STATUS_FAILURE;

    pwlanpriv = NULL;
    return wlan_remove_callback(card);
}

/** 
 *  @brief This function reads scratch registers
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param dat	   A pointer to keep returned data
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
mv_sdio_read_scratch(wlan_private * priv, u16 * dat)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 scr0;
    u8 scr1;
    ret = sdio_read_ioreg(priv->wlan_dev.card, FN1, CARD_OCR_0_REG, &scr0);
    if (ret < 0)
        return WLAN_STATUS_FAILURE;

    ret = sdio_read_ioreg(priv->wlan_dev.card, FN1, CARD_OCR_1_REG, &scr1);
    PRINTM(INFO, "CARD_OCR_0_REG = 0x%x, CARD_OCR_1_REG = 0x%x\n", scr0,
           scr1);
    if (ret < 0)
        return WLAN_STATUS_FAILURE;

    *dat = (((u16) scr1) << 8) | scr0;

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function polls the card status register.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param bits    	the bit mask
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
mv_sdio_poll_card_status(wlan_private * priv, u8 bits)
{
    int tries;
    int rval;
    u8 cs;

    for (tries = 0; tries < MAX_POLL_TRIES; tries++) {
        rval = sdio_read_ioreg(priv->wlan_dev.card, FN1,
                               CARD_STATUS_REG, &cs);
        if (rval == 0 && (cs & bits) == bits) {
            return WLAN_STATUS_SUCCESS;
        }

        udelay(100);
    }

    PRINTM(WARN, "mv_sdio_poll_card_status: FAILED!:%d\n", rval);
    return WLAN_STATUS_FAILURE;
}

/** 
 *  @brief This function set the sdio bus width.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param mode    	1--1 bit mode, 4--4 bit mode
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
set_bus_width(wlan_private * priv, u8 mode)
{
    sdio_set_buswidth(((mmc_card_t) ((priv->wlan_dev).card))->ctrlr->card,
                      mode);
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function programs the firmware image.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param firmware 	A pointer to the buffer of firmware image
 *  @param firmwarelen 	the length of firmware image
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sbi_prog_firmware_image(wlan_private * priv,
                        const u8 * firmware, int firmwarelen)
{
    int ret = WLAN_STATUS_SUCCESS;
    u16 firmwarestat;
    u8 *fwbuf = priv->adapter->TmpTxBuf;
    int fwblknow;
    u32 tx_len;
#ifdef FW_DOWNLOAD_SPEED
    u32 tv1, tv2;
#endif

    ENTER();

    /* Check if the firmware is already downloaded */
    if ((ret = mv_sdio_read_scratch(priv, &firmwarestat)) < 0) {
        PRINTM(INFO, "read scratch returned <0\n");
        goto done;
    }

    if (firmwarestat == FIRMWARE_READY) {
        PRINTM(INFO, "FW already downloaded!\n");
        ret = WLAN_STATUS_SUCCESS;
        goto done;
    }

    PRINTM(INFO, "Downloading helper image (%d bytes), block size %d bytes\n",
           firmwarelen, SD_BLOCK_SIZE_FW_DL);

#ifdef FW_DOWNLOAD_SPEED
    tv1 = get_utimeofday();
#endif
    /* Perform firmware data transfer */
    tx_len =
        (FIRMWARE_TRANSFER_NBLOCK * SD_BLOCK_SIZE_FW_DL) - SDIO_HEADER_LEN;
    for (fwblknow = 0; fwblknow < firmwarelen; fwblknow += tx_len) {

        /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
        ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
        if (ret < 0) {
            PRINTM(FATAL, "FW download died @ %d\n", fwblknow);
            goto done;
        }

        /* Set blocksize to transfer - checking for last block */
        if (firmwarelen - fwblknow < tx_len)
            tx_len = firmwarelen - fwblknow;

        fwbuf[0] = ((tx_len & 0x000000ff) >> 0);        /* Little-endian */
        fwbuf[1] = ((tx_len & 0x0000ff00) >> 8);
        fwbuf[2] = ((tx_len & 0x00ff0000) >> 16);
        fwbuf[3] = ((tx_len & 0xff000000) >> 24);

        /* Copy payload to buffer */
        memcpy(&fwbuf[SDIO_HEADER_LEN], &firmware[fwblknow], tx_len);

        PRINTM(INFO, ".");

        /* Send data */
        ret = sdio_write_iomem(priv->wlan_dev.card, FN1,
                               priv->wlan_dev.ioport, BLOCK_MODE,
                               FIXED_ADDRESS, FIRMWARE_TRANSFER_NBLOCK,
                               SD_BLOCK_SIZE_FW_DL, fwbuf);

        if (ret < 0) {
            PRINTM(FATAL, "IO error: transferring block @ %d\n", fwblknow);
            goto done;
        }
    }

#ifdef FW_DOWNLOAD_SPEED
    tv2 = get_utimeofday();
    PRINTM(INFO, "helper: %ld.%03ld.%03ld ", tv1 / 1000000,
           (tv1 % 1000000) / 1000, tv1 % 1000);
    PRINTM(INFO, " -> %ld.%03ld.%03ld ", tv2 / 1000000,
           (tv2 % 1000000) / 1000, tv2 % 1000);
    tv2 -= tv1;
    PRINTM(INFO, " == %ld.%03ld.%03ld\n", tv2 / 1000000,
           (tv2 % 1000000) / 1000, tv2 % 1000);
#endif

    /* Write last EOF data */
    PRINTM(INFO, "\nTransferring EOF block\n");
    memset(fwbuf, 0x0, SD_BLOCK_SIZE_FW_DL);
    ret = sdio_write_iomem(priv->wlan_dev.card, FN1,
                           priv->wlan_dev.ioport, BLOCK_MODE,
                           FIXED_ADDRESS, 1, SD_BLOCK_SIZE_FW_DL, fwbuf);

    if (ret < 0) {
        PRINTM(FATAL, "IO error in writing EOF FW block\n");
        goto done;
    }

    ret = WLAN_STATUS_SUCCESS;

  done:
    return ret;
}

/** 
 *  @brief This function downloads firmware image to the card.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param firmware	A pointer to firmware image buffer
 *  @param firmwarelen	the length of firmware image
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sbi_download_wlan_fw_image(wlan_private * priv,
                           const u8 * firmware, int firmwarelen)
{
    u8 base0;
    u8 base1;
    int ret = WLAN_STATUS_SUCCESS;
    int offset;
    u8 *fwbuf = priv->adapter->TmpTxBuf;
    int timeout = 5000;
    u16 len;
    int txlen = 0;
    int tx_blocks = 0;
    int i = 0;
#ifdef FW_DOWNLOAD_SPEED
    u32 tv1, tv2;
#endif

    ENTER();

    PRINTM(INFO, "Downloading FW image (%d bytes)\n", firmwarelen);

#ifdef FW_DOWNLOAD_SPEED
    tv1 = get_utimeofday();
#endif

    /* Wait initially for the first non-zero value */
    do {
        if ((ret = sdio_read_ioreg(priv->wlan_dev.card, FN1,
                                   HOST_F1_RD_BASE_0, &base0)) < 0) {
            PRINTM(WARN, "Dev BASE0 register read failed:"
                   " base0=0x%04X(%d)\n", base0, base0);
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
        if ((ret = sdio_read_ioreg(priv->wlan_dev.card, FN1,
                                   HOST_F1_RD_BASE_1, &base1)) < 0) {
            PRINTM(WARN, "Dev BASE1 register read failed:"
                   " base1=0x%04X(%d)\n", base1, base1);
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
        len = (((u16) base1) << 8) | base0;
        mdelay(1);
    } while (!len && --timeout);

    if (!timeout) {
        PRINTM(MSG, "Helper downloading finished.\n");
        PRINTM(MSG, "Timeout for FW downloading!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
    ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
    if (ret < 0) {
        PRINTM(FATAL, "FW download died, helper not ready\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    len &= ~B_BIT_0;

    /* Perform firmware data transfer */
    for (offset = 0; offset < firmwarelen; offset += txlen) {
        txlen = len;

        /* Set blocksize to transfer - checking for last block */
        if (firmwarelen - offset < txlen) {
            txlen = firmwarelen - offset;
        }
        /* PRINTM(INFO, "fw: offset=%d, txlen = 0x%04X(%d)\n", 
           offset,txlen,txlen); */
        PRINTM(INFO, ".");

        /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
        ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
        if (ret < 0) {
            PRINTM(FATAL, "FW download with helper died @ %d\n", offset);
            goto done;
        }

        tx_blocks = (txlen + SD_BLOCK_SIZE_FW_DL - 1) / SD_BLOCK_SIZE_FW_DL;

        /* Copy payload to buffer */
        memcpy(fwbuf, &firmware[offset], txlen);

        /* Send data */
        ret = sdio_write_iomem(priv->wlan_dev.card, FN1,
                               priv->wlan_dev.ioport, BLOCK_MODE,
                               FIXED_ADDRESS, tx_blocks, SD_BLOCK_SIZE_FW_DL,
                               fwbuf);

        if (ret < 0) {
            PRINTM(ERROR, "FW download, write iomem (%d) failed: %d\n", i,
                   ret);
            if (sdio_write_ioreg
                (priv->wlan_dev.card, FN1, CONFIGURATION_REG, 0x04) < 0) {
                PRINTM(ERROR, "write ioreg failed (FN1 CFG)\n");
            }
        }

        /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
        ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
        if (ret < 0) {
            PRINTM(FATAL, "FW download with helper died @ %d\n", offset);
            goto done;
        }

        if ((ret = sdio_read_ioreg(priv->wlan_dev.card, FN1,
                                   HOST_F1_RD_BASE_0, &base0)) < 0) {
            PRINTM(WARN, "Dev BASE0 register read failed:"
                   " base0=0x%04X(%d)\n", base0, base0);
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
        if ((ret = sdio_read_ioreg(priv->wlan_dev.card, FN1,
                                   HOST_F1_RD_BASE_1, &base1)) < 0) {
            PRINTM(WARN, "Dev BASE1 register read failed:"
                   " base1=0x%04X(%d)\n", base1, base1);
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
        len = (((u16) base1) << 8) | base0;

        if (!len) {
            break;
        }

        if (len & B_BIT_0) {
            i++;
            if (i > MAX_WRITE_IOMEM_RETRY) {
                PRINTM(FATAL, "FW download failure, over max retry count\n");
                ret = WLAN_STATUS_FAILURE;
                goto done;
            }
            PRINTM(ERROR, "CRC error indicated by the helper:"
                   " len = 0x%04X, txlen = %d\n", len, txlen);
            len &= ~B_BIT_0;
            /* Setting this to 0 to resend from same offset */
            txlen = 0;
        } else
            i = 0;
    }
    PRINTM(INFO, "\nFW download over, size %d bytes\n", firmwarelen);

    ret = WLAN_STATUS_SUCCESS;
  done:
#ifdef FW_DOWNLOAD_SPEED
    tv2 = get_utimeofday();
    PRINTM(INFO, "FW: %ld.%03ld.%03ld ", tv1 / 1000000,
           (tv1 % 1000000) / 1000, tv1 % 1000);
    PRINTM(INFO, " -> %ld.%03ld.%03ld ", tv2 / 1000000,
           (tv2 % 1000000) / 1000, tv2 % 1000);
    tv2 -= tv1;
    PRINTM(INFO, " == %ld.%03ld.%03ld\n", tv2 / 1000000,
           (tv2 % 1000000) / 1000, tv2 % 1000);
#endif
    LEAVE();
    return ret;
}

/** 
 *  @brief This function reads data from the card.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param type	   	A pointer to keep type as data or command
 *  @param nb		A pointer to keep the data/cmd length retured in buffer
 *  @param payload 	A pointer to the data/cmd buffer
 *  @param nb	   	the length of data/cmd buffer
 *  @param npayload	the length of data/cmd buffer
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
mv_sdio_card_to_host(wlan_private * priv,
                     u32 * type, int *nb, u8 * payload, int npayload)
{
    int ret = WLAN_STATUS_SUCCESS;
    u16 buf_len = 0;
    int buf_block_len;
    int blksz;
    u32 *pevent;

    if (((mmc_card_t) ((priv->wlan_dev).card))->ctrlr->bus_width !=
        priv->adapter->sdiomode) {
        set_bus_width(priv, priv->adapter->sdiomode);
    }
    if (!payload) {
        PRINTM(WARN, "payload NULL pointer received!\n");
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }

    /* Read the length of data to be transferred */
    ret = mv_sdio_read_scratch(priv, &buf_len);
    if (ret < 0) {
        PRINTM(ERROR, "card_to_host, read scratch reg failed\n");
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }

    if (buf_len <= SDIO_HEADER_LEN || buf_len > npayload) {
        PRINTM(ERROR, "card_to_host, invalid packet length: %d\n", buf_len);
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }

    /* Allocate buffer */
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (buf_len + blksz - 1) / blksz;

    /* The host polls for the CARD_IO_READY bit */
    ret = mv_sdio_poll_card_status(priv, CARD_IO_READY);
    if (ret < 0) {
        PRINTM(ERROR, "card_to_host, poll status failed: %d\n", ret);
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }

    ret = sdio_read_iomem(priv->wlan_dev.card, FN1, priv->wlan_dev.ioport,
                          BLOCK_MODE, FIXED_ADDRESS, buf_block_len,
                          blksz, payload);

    if (ret < 0) {
        PRINTM(ERROR, "card_to_host, read iomem failed: %d\n", ret);
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }
    *nb = buf_len;

    DBG_HEXDUMP(IF_D, "SDIO Blk Rd", payload, blksz * buf_block_len);

    *type = (payload[2] | (payload[3] << 8));
    if (*type == MVSD_EVENT) {
        pevent = (u32 *) & payload[4];
        priv->adapter->EventCause = MVSD_EVENT | (((u16) (*pevent)) << 3);
    }

  exit:
    return ret;
}

/** 
 *  @brief This function enables the host interrupts mask
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   WLAN_STATUS_SUCCESS
 */
static int
enable_host_int_mask(wlan_private * priv, u8 mask)
{
    int ret = WLAN_STATUS_SUCCESS;
    mmc_card_t card = (mmc_card_t) priv->wlan_dev.card;

    /* Simply write the mask to the register */
    ret = sdio_write_ioreg(card, FN1, HOST_INT_MASK_REG, mask);

    if (ret < 0) {
        PRINTM(WARN, "ret = %d\n", ret);
        ret = WLAN_STATUS_FAILURE;
    }

    priv->adapter->HisRegCpy = 1;

    return ret;
}

/**  @brief This function disables the host interrupts mask.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
disable_host_int_mask(wlan_private * priv, u8 mask)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 host_int_mask;

    /* Read back the host_int_mask register */
    ret = sdio_read_ioreg(priv->wlan_dev.card, FN1, HOST_INT_MASK_REG,
                          &host_int_mask);
    if (ret < 0) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* Update with the mask and write back to the register */
    host_int_mask &= ~mask;
    ret = sdio_write_ioreg(priv->wlan_dev.card, FN1, HOST_INT_MASK_REG,
                           host_int_mask);
    if (ret < 0) {
        PRINTM(WARN, "Unable to diable the host interrupt!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

  done:
    return ret;
}

/********************************************************
		Global Functions
********************************************************/

int start_bus_clock(mmc_controller_t);
int stop_bus_clock_2(mmc_controller_t);

/** 
 *  @brief This function handles the interrupt.
 *  
 *  @param irq 	   The irq of device.
 *  @param dev_id  A pointer to net_device structure
 *  @param fp	   A pointer to pt_regs structure
 *  @return 	   n/a
 */
static IRQ_RET_TYPE
sbi_interrupt(int irq, void *dev_id, struct pt_regs *fp)
{
    struct net_device *dev = dev_id;
    wlan_private *priv;

    ENTER();

    priv = dev->priv;

    wlan_interrupt(dev);

    IRQ_RET;
}

/** 
 *  @brief This function registers the IF module in bus driver.
 *  
 *  @param add	   wlan driver's call back funtion for add card.
 *  @param remove  wlan driver's call back funtion for remove card.
 *  @param arg     not been used
 *  @return	   An int pointer that keeps returned value
 */
int *
sbi_register(wlan_notifier_fn_add add, wlan_notifier_fn_remove remove,
             void *arg)
{
    int *sdio_ret;

    ENTER();

    wlan_add_callback = add;
    wlan_remove_callback = remove;

    if_sdio_notifier.add = (mmc_notifier_fn_t) sbi_add_card;
    if_sdio_notifier.remove = (mmc_notifier_fn_t) sbi_remove_card;
    isr_function = sbi_interrupt;
    sdio_ret = sdio_register(MMC_REG_TYPE_USER, &if_sdio_notifier, 0);

    /* init GPIO PORT for wakeup purpose */
    GPIO_PORT_INIT();

    /* set default value */
    GPIO_PORT_TO_HIGH();

    LEAVE();

    return sdio_ret;
}

/** 
 *  @brief This function de-registers the IF module in bus driver.
 *  
 *  @return 	   n/a
 */
void
sbi_unregister(void)
{
    sdio_unregister(MMC_REG_TYPE_USER, &if_sdio_notifier);
}

/** 
 *  @brief This function reads the IO register.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param func	   funcion number
 *  @param reg	   register to be read
 *  @param dat	   A pointer to variable that keeps returned value
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_read_ioreg(wlan_private * priv, u8 func, u32 reg, u8 * dat)
{
    return sdio_read_ioreg(priv->wlan_dev.card, func, reg, dat);
}

/** 
 *  @brief This function writes the IO register.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param func	   funcion number
 *  @param reg	   register to be written
 *  @param dat	   the value to be written
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_write_ioreg(wlan_private * priv, u8 func, u32 reg, u8 dat)
{
    return sdio_write_ioreg(priv->wlan_dev.card, func, reg, dat);
}

/** 
 *  @brief This function checks the interrupt status and handle it accordingly.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param ireg    A pointer to variable that keeps returned value
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_get_int_status(wlan_private * priv, u8 * ireg)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 *cmdBuf;
    wlan_dev_t *wlan_dev = &priv->wlan_dev;
    struct sk_buff *skb;
    mmc_card_t card = (mmc_card_t) wlan_dev->card;

    if ((ret = sdio_read_ioreg(card, FN1, HOST_INTSTATUS_REG, ireg))) {
        PRINTM(WARN, "sdio_read_ioreg: read int status register failed\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    if (*ireg != 0) {
        /*
         * DN_LD_HOST_INT_STATUS and/or UP_LD_HOST_INT_STATUS
         * Clear the interrupt status register and re-enable the interrupt
         */
        PRINTM(INFO, "sdio_ireg = 0x%x\n", *ireg);
        if ((ret = sdio_write_ioreg(card, FN1, HOST_INTSTATUS_REG,
                                    ~(*ireg) & (DN_LD_HOST_INT_STATUS |
                                                UP_LD_HOST_INT_STATUS))) <
            0) {
            PRINTM(WARN,
                   "sdio_write_ioreg: clear int status register failed\n");
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
    }

    if (*ireg & DN_LD_HOST_INT_STATUS) {        /* tx_done INT */
        *ireg |= HIS_TxDnLdRdy;
        if (!priv->wlan_dev.dnld_sent) {        /* tx_done already received */
            PRINTM(INFO, "warning: tx_done already received:"
                   " dnld_sent=0x%x int status=0x%x\n",
                   priv->wlan_dev.dnld_sent, *ireg);
        } else {
            if (priv->wlan_dev.dnld_sent == DNLD_DATA_SENT)
                os_start_queue(priv);
            priv->wlan_dev.dnld_sent = DNLD_RES_RECEIVED;
        }
    }

    if (*ireg & UP_LD_HOST_INT_STATUS) {

        /* 
         * DMA read data is by block alignment,so we need alloc extra block
         * to avoid wrong memory access.
         */
        if (!(skb = dev_alloc_skb(ALLOC_BUF_SIZE))) {
            PRINTM(WARN, "No free skb\n");
            priv->stats.rx_dropped++;
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }

        /* 
         * Transfer data from card
         * skb->tail is passed as we are calling skb_put after we
         * are reading the data
         */
        if (mv_sdio_card_to_host(priv, &wlan_dev->upld_typ,
                                 (int *) &wlan_dev->upld_len, skb->tail,
                                 ALLOC_BUF_SIZE) < 0) {
            u8 cr = 0;

            PRINTM(ERROR, "Card to host failed: int status=0x%x\n", *ireg);
            if (sdio_read_ioreg(wlan_dev->card, FN1,
                                CONFIGURATION_REG, &cr) < 0)
                PRINTM(ERROR, "read ioreg failed (FN1 CFG)\n");

            PRINTM(INFO, "Config Reg val = %d\n", cr);
            if (sdio_write_ioreg(wlan_dev->card, FN1,
                                 CONFIGURATION_REG, (cr | 0x04)) < 0)
                PRINTM(ERROR, "write ioreg failed (FN1 CFG)\n");

            PRINTM(INFO, "write success\n");
            if (sdio_read_ioreg(wlan_dev->card, FN1,
                                CONFIGURATION_REG, &cr) < 0)
                PRINTM(ERROR, "read ioreg failed (FN1 CFG)\n");

            PRINTM(INFO, "Config reg val =%x\n", cr);
            ret = WLAN_STATUS_FAILURE;
            kfree_skb(skb);
            goto done;
        }

        if (wlan_dev->upld_typ == MVSD_DAT) {
            PRINTM(DATA, "Data <= FW\n");
            *ireg |= HIS_RxUpLdRdy;
            skb_put(skb, priv->wlan_dev.upld_len);
            skb_pull(skb, SDIO_HEADER_LEN);
            list_add_tail((struct list_head *) skb,
                          (struct list_head *) &priv->adapter->RxSkbQ);
        } else if (wlan_dev->upld_typ == MVSD_CMD) {
            *ireg &= ~(HIS_RxUpLdRdy);
            *ireg |= HIS_CmdUpLdRdy;

            /* take care of CurCmd = NULL case by reading the 
             * data to clear the interrupt */
            if (!priv->adapter->CurCmd) {
                cmdBuf = priv->wlan_dev.upld_buf;
                priv->adapter->HisRegCpy &= ~HIS_CmdUpLdRdy;
            } else {
                cmdBuf = priv->adapter->CurCmd->BufVirtualAddr;
            }

            priv->wlan_dev.upld_len -= SDIO_HEADER_LEN;
            memcpy(cmdBuf, skb->data + SDIO_HEADER_LEN,
                   MIN(MRVDRV_SIZE_OF_CMD_BUFFER, priv->wlan_dev.upld_len));
            kfree_skb(skb);
        } else if (wlan_dev->upld_typ == MVSD_EVENT) {
            *ireg |= HIS_CardEvent;
            kfree_skb(skb);
        }

        *ireg |= HIS_CmdDnLdRdy;
    }

    ret = WLAN_STATUS_SUCCESS;
  done:

    return ret;
}

/** 
 *  @brief This function is a dummy function.
 *  
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_card_to_host(wlan_private * priv, u32 type,
                 u32 * nb, u8 * payload, u16 npayload)
{
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function is a dummy function.
 *  
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_read_event_cause(wlan_private * priv)
{
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function reenables the host interrupts.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param bits    the bit mask
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_reenable_host_interrupt(wlan_private * priv, u8 bits)
{
    sdio_enable_SDIO_INT();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function disables the host interrupts.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_disable_host_int(wlan_private * priv)
{
    return disable_host_int_mask(priv, HIM_DISABLE);
}

/** 
 *  @brief This function enables the host interrupts.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_enable_host_int(wlan_private * priv)
{

    return enable_host_int_mask(priv, HIM_ENABLE);
}

/** 
 *  @brief This function de-registers the device.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_unregister_dev(wlan_private * priv)
{
    ENTER();

    if (priv->wlan_dev.card != NULL) {
        /* Release the SDIO IRQ */
        sdio_free_irq(priv->wlan_dev.card, priv->wlan_dev.netdev);
        PRINTM(WARN, "Making the sdio dev card as NULL\n");
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function registers the device.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_register_dev(wlan_private * priv)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 reg;
    mmc_card_t card = (mmc_card_t) priv->wlan_dev.card;

    ENTER();

    /* Initialize the private structure */
    strncpy(priv->wlan_dev.name, "sdio0", sizeof(priv->wlan_dev.name));
    priv->wlan_dev.ioport = 0;
    priv->wlan_dev.upld_rcv = 0;
    priv->wlan_dev.upld_typ = 0;
    priv->wlan_dev.upld_len = 0;

    /* Read the IO port */
    ret = sdio_read_ioreg(card, FN1, IO_PORT_0_REG, &reg);
    if (ret < 0)
        goto failed;
    else
        priv->wlan_dev.ioport |= reg;

    ret = sdio_read_ioreg(card, FN1, IO_PORT_1_REG, &reg);
    if (ret < 0)
        goto failed;
    else
        priv->wlan_dev.ioport |= (reg << 8);

    ret = sdio_read_ioreg(card, FN1, IO_PORT_2_REG, &reg);
    if (ret < 0)
        goto failed;
    else
        priv->wlan_dev.ioport |= (reg << 16);

    PRINTM(INFO, "SDIO FUNC1 IO port: 0x%x\n", priv->wlan_dev.ioport);

    /* Disable host interrupt first. */
    if ((ret = disable_host_int_mask(priv, 0xff)) < 0) {
        PRINTM(WARN, "Warning: unable to disable host interrupt!\n");
    }

    /* Request the SDIO IRQ */
    PRINTM(INFO, "Before request_irq Address is if==>%p\n", isr_function);
    ret = sdio_request_irq(priv->wlan_dev.card,
                           (handler_fn_t) isr_function, 0,
                           "sdio_irq", priv->wlan_dev.netdev);

    if (ret < 0) {
        PRINTM(FATAL, "Failed to request IRQ on SDIO bus (%d)\n", ret);
        goto failed;
    }

    PRINTM(INFO, "IrqLine: %d\n", card->ctrlr->tmpl->irq_line);
    priv->wlan_dev.netdev->irq = card->ctrlr->tmpl->irq_line;
    priv->adapter->irq = priv->wlan_dev.netdev->irq;
    priv->adapter->chip_rev = card->chiprev;
    priv->adapter->sdiomode =
        ((mmc_card_t) ((priv->wlan_dev).card))->ctrlr->bus_width;

    return WLAN_STATUS_SUCCESS;

  failed:
    priv->wlan_dev.card = NULL;

    return WLAN_STATUS_FAILURE;
}

/** 
 *  @brief This function sends data to the card.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param type	   data or command
 *  @param payload A pointer to the data/cmd buffer
 *  @param nb	   the length of data/cmd
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_host_to_card(wlan_private * priv, u8 type, u8 * payload, u16 nb)
{
    int ret = WLAN_STATUS_SUCCESS;
    int buf_block_len;
    int blksz;
    int i = 0;

    ENTER();

    priv->adapter->HisRegCpy = 0;

    /* Allocate buffer and copy payload */
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (nb + SDIO_HEADER_LEN + blksz - 1) / blksz;

    /* This is SDIO specific header
     *  length: byte[1][0], 
     *  type: byte[3][2] (MVSD_DAT = 0, MVSD_CMD = 1, MVSD_EVENT = 3) 
     */
    priv->adapter->TmpTxBuf[0] = (nb + SDIO_HEADER_LEN) & 0xff;
    priv->adapter->TmpTxBuf[1] = ((nb + SDIO_HEADER_LEN) >> 8) & 0xff;
    priv->adapter->TmpTxBuf[2] = type;
    priv->adapter->TmpTxBuf[3] = 0x0;

    if (payload != NULL &&
        (nb > 0 &&
         nb <= (sizeof(priv->adapter->TmpTxBuf) - SDIO_HEADER_LEN))) {
        if (type == MVMS_CMD)
            memcpy(&priv->adapter->TmpTxBuf[SDIO_HEADER_LEN], payload, nb);
    } else {
        PRINTM(WARN, "sbi_host_to_card(): Error: payload=%p, nb=%d\n",
               payload, nb);
    }

    /* The host polls for the CARD_IO_READY bit */
    ret = mv_sdio_poll_card_status(priv, CARD_IO_READY);
    if (ret < 0) {
        PRINTM(ERROR, "host_to_card, poll status failed: %d\n", ret);
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }

    if (((mmc_card_t) ((priv->wlan_dev).card))->ctrlr->bus_width !=
        priv->adapter->sdiomode) {
        set_bus_width(priv, priv->adapter->sdiomode);
    }

    if (type == MVSD_DAT)
        priv->wlan_dev.dnld_sent = DNLD_DATA_SENT;
    else
        priv->wlan_dev.dnld_sent = DNLD_CMD_SENT;

    do {
        /* Transfer data to card */
        ret =
            sdio_write_iomem(priv->wlan_dev.card, FN1, priv->wlan_dev.ioport,
                             BLOCK_MODE, FIXED_ADDRESS, buf_block_len, blksz,
                             priv->adapter->TmpTxBuf);
        if (ret < 0) {
            i++;

            PRINTM(ERROR, "host_to_card, write iomem (%d) failed: %d\n", i,
                   ret);
            if (sdio_write_ioreg
                (priv->wlan_dev.card, FN1, CONFIGURATION_REG, 0x04) < 0) {
                PRINTM(ERROR, "write ioreg failed (FN1 CFG)\n");
            }
            ret = WLAN_STATUS_FAILURE;
            if (i > MAX_WRITE_IOMEM_RETRY) {
                priv->wlan_dev.dnld_sent = DNLD_RES_RECEIVED;
                goto exit;
            }
        } else {
            DBG_HEXDUMP(IF_D, "SDIO Blk Wr", priv->adapter->TmpTxBuf,
                        blksz * buf_block_len);
        }
    } while (ret == WLAN_STATUS_FAILURE);

  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function reads CIS informaion.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_get_cis_info(wlan_private * priv)
{
    mmc_card_t card = (mmc_card_t) priv->wlan_dev.card;
    wlan_adapter *Adapter = priv->adapter;
    u8 tupledata[255];
    int i;
    u32 ret = WLAN_STATUS_SUCCESS;

    ENTER();

    /* Read the Tuple Data */
    for (i = 0; i < sizeof(tupledata); i++) {
        ret = sdio_read_ioreg(card, FN0, card->info.cisptr[FN0] + i,
                              &tupledata[i]);
        if (ret) {
            PRINTM(WARN, "get_cis_info error:%d\n", ret);
            return WLAN_STATUS_FAILURE;
        }
    }

    /* Copy the CIS Table to Adapter */
    memset(Adapter->CisInfoBuf, 0x0, sizeof(Adapter->CisInfoBuf));
    memcpy(Adapter->CisInfoBuf, tupledata, sizeof(tupledata));
    Adapter->CisInfoLen = sizeof(tupledata);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function probes the card.
 *  
 *  @param card_p  A pointer to the card
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_probe_card(void *card_p)
{
    mmc_card_t card = (mmc_card_t) card_p;
    int ret = WLAN_STATUS_SUCCESS;
    u8 bic;

    ENTER();

    if (!card) {
        ret = -ENODEV;          //WLAN_STATUS_FAILURE;
        goto done;
    }

    /* Check for MANFID */
    ret = sdio_get_vendor_id(card);

#define MARVELL_VENDOR_ID 0x02df
    if (ret == MARVELL_VENDOR_ID) {
        PRINTM(INFO, "Marvell SDIO card detected!\n");
    } else {
        PRINTM(MSG, "Ignoring a non-Marvell SDIO card...\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* read Revision Register to get the hw revision number */
    if (sdio_read_ioreg(card, FN1, CARD_REVISION_REG, &card->chiprev) < 0) {
        PRINTM(FATAL, "cannot read CARD_REVISION_REG\n");
    } else {
        PRINTM(INFO, "revsion=0x%x\n", card->chiprev);
        switch (card->chiprev) {
        default:
            card->block_size_512 = TRUE;
            card->async_int_mode = TRUE;

            /* enable async interrupt mode */
            ret = sdio_read_ioreg(card, FN0, BUS_INTERFACE_CONTROL_REG, &bic);
            if (ret < 0) {
                ret = WLAN_STATUS_FAILURE;
                goto done;
            }
            bic |= ASYNC_INT_MODE;
            ret = sdio_write_ioreg(card, FN0, BUS_INTERFACE_CONTROL_REG, bic);
            if (ret < 0) {
                ret = WLAN_STATUS_FAILURE;
                goto done;
            }
            break;
        }
    }

    ret = WLAN_STATUS_SUCCESS;
  done:

    LEAVE();
    return ret;
}

/** 
 *  @brief This function calls sbi_download_wlan_fw_image to download
 *  firmware image to the card.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_prog_firmware_w_helper(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    if (Adapter->fmimage != NULL) {
        return sbi_download_wlan_fw_image(priv,
                                          Adapter->fmimage,
                                          Adapter->fmimage_len);
    } else {
        PRINTM(MSG, "No external FW image\n");
        return WLAN_STATUS_FAILURE;
    }
}

/** 
 *  @brief This function programs helper image.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_prog_helper(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    if (Adapter->helper != NULL) {
        return sbi_prog_firmware_image(priv,
                                       Adapter->helper, Adapter->helper_len);
    } else {
        PRINTM(MSG, "No external helper image\n");
        return WLAN_STATUS_FAILURE;
    }
}

/** 
 *  @brief This function checks if the firmware is ready to accept
 *  command or not.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_verify_fw_download(wlan_private * priv)
{
    int ret = WLAN_STATUS_SUCCESS;
    u16 firmwarestat;
    int tries;

    /* Wait for firmware initialization event */
    for (tries = 0; tries < MAX_FIRMWARE_POLL_TRIES; tries++) {
        if ((ret = mv_sdio_read_scratch(priv, &firmwarestat)) < 0)
            continue;

        if (firmwarestat == FIRMWARE_READY) {
            ret = WLAN_STATUS_SUCCESS;
            break;
        } else {
            mdelay(10);
            ret = WLAN_STATUS_FAILURE;
        }
    }

    if (ret < 0) {
        PRINTM(MSG, "Timeout waiting for FW to become active\n");
        goto done;
    }

    ret = WLAN_STATUS_SUCCESS;
  done:
    return ret;
}

/** 
 *  @brief This function set bus clock on/off
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param option    TRUE--on , FALSE--off
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_set_bus_clock(wlan_private * priv, u8 option)
{

    if (option == TRUE)
        start_bus_clock(((mmc_card_t) ((priv->wlan_dev).card))->ctrlr);
    else
        stop_bus_clock_2(((mmc_card_t) ((priv->wlan_dev).card))->ctrlr);
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function makes firmware exiting from deep sleep.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_exit_deep_sleep(wlan_private * priv)
{
    int ret = WLAN_STATUS_SUCCESS;

    if (priv->adapter->fwWakeupMethod == WAKEUP_FW_THRU_GPIO) {
        GPIO_PORT_TO_LOW();
        sbi_set_bus_clock(priv, TRUE);
    } else
        ret = sdio_write_ioreg(priv->wlan_dev.card, FN1, CONFIGURATION_REG,
                               HOST_POWER_UP);

    return ret;
}

/** 
 *  @brief This function resets the setting of deep sleep.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_reset_deepsleep_wakeup(wlan_private * priv)
{

    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (priv->adapter->fwWakeupMethod == WAKEUP_FW_THRU_GPIO) {
        GPIO_PORT_TO_HIGH();
    } else
        ret =
            sdio_write_ioreg(priv->wlan_dev.card, FN1, CONFIGURATION_REG, 0);

    LEAVE();

    return ret;
}

#ifdef ENABLE_PM
/** 
 *  @brief This function suspends the device.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_suspend(wlan_private * priv)
{
    return sdio_suspend(priv->wlan_dev.card);
}

/** 
 *  @brief This function resumes the device from sleep
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
*/
int
sbi_resume(wlan_private * priv)
{
    return sdio_resume(priv->wlan_dev.card);
}
#endif

/** 
 *  @brief This function issues SDIO reset 
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
*/
int
sbi_reset(wlan_private * priv)
{
    return sdio_write_ioreg(priv->wlan_dev.card, FN0, IO_ABORT_REG, RES);
}
