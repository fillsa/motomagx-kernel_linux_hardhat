/** @file sdio.c
 *  @brief Low level SDIO Driver
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

/*
 * Include Files
 */
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/blkdev.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/protocol.h>
#include <linux/delay.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <asm/delay.h>

#include "sdio.h"
#include "sdhc.h"
#ifdef MOTO_PLATFORM
#include "sdio_wprm_drv.h"
#endif

int sdioclock = 1;
MODULE_PARM(sdioclock, "l");
MODULE_PARM_DESC(sdioclock, "SDIO clock rate");

int bus_mode = 4;
MODULE_PARM(bus_mode, "l");
MODULE_PARM_DESC(bus_mode, "Bus Mode(4/1)");

/* Use the Kernel dynamic allocated major number, by default*/
int major_number = 0;
MODULE_PARM(major_number, "l");
MODULE_PARM_DESC(major_number, "SDIO major number");

#ifdef DEBUG_SDIO_LEVEL0
static void dump_status(int sts);
#endif

#define MODULE_GET			try_module_get(THIS_MODULE)
#define MODULE_PUT			module_put(THIS_MODULE)

#ifdef DEBUG_USEC
ulong rbuf[100];
ulong wbuf[100];
int rcnt = 0;
int wcnt = 0;
#endif

static struct resource sdio1_resources[] = {
    [0] = {
#ifdef MOTO_PLATFORM
           .start = MMC_SDHC2_BASE_ADDR,
           .end = MMC_SDHC2_BASE_ADDR + SZ_16K - 1,

#else
           .start = MMC_SDHC1_BASE_ADDR,
           .end = MMC_SDHC1_BASE_ADDR + SZ_16K - 1,
#endif
           .flags = IORESOURCE_MEM,
           },
    [1] = {
#ifdef MOTO_PLATFORM
           .start = INT_MMC_SDHC2,
           .end = INT_MMC_SDHC2,

#else
           .start = INT_MMC_SDHC1,
           .end = INT_MMC_SDHC1,
#endif
           .flags = IORESOURCE_IRQ,
           }
};

static struct mmc_host *mmc = 0;
/*!
 * Maxumum length of s/g list, only length of 1 is currently supported
 */
#define NR_SG   1

static sdio_ctrller *sdio_controller = 0;
struct mmc_data sdio_dma_data;
struct scatterlist sdio_dma_sg;
static IRQ_RET_TYPE(*sdioint_handler) (int, void *, struct pt_regs *);
static void *net_devid;
static void card_setup(sdio_ctrller * ctrller);
static void card_release(sdio_ctrller * ctrller);

#define mmc_detect_change(x) sdio_set_ios(0,0)
static void sdio_set_ios(struct mmc_host *mmc, struct mmc_ios *ios);

#ifndef MOTO_PLATFORM
extern void gpio_sdhc_active(int module);
extern void gpio_sdhc_inactive(int module);
#endif
#if 0
#ifdef MOTO_PLATFORM
extern int sdhc_intr_setup(int module, void *host, gpio_irq_handler handler);
#else
extern int sdhc_intr_setup(void *host, gpio_irq_handler handler);
#endif
extern int sdhc_intr_cleanup(void *host);
#ifdef MOTO_PLATFORM
extern void sdhc_intr_clear(int module, int flag);
#else
extern void sdhc_intr_clear(int *flag);
#endif
#endif
extern unsigned int sdhc_get_min_clock(enum mxc_clocks clk);
extern unsigned int sdhc_get_max_clock(enum mxc_clocks clk);
#ifdef MOTO_PLATFORM
extern int sdhc_find_card(int module);
#else
extern int sdhc_find_card(void);
#endif

#ifdef MXC_MMC_DMA_ENABLE
static void sdio_dma_irq(void *devid);
#endif

static inline void
WRITEL(unsigned long val, void *adr)
{
    _DBG("WRITEL(%s(%d)): ADDR=0x%08x.  VAL=0x%lx\n", __FUNCTION__, __LINE__,
         (int) adr, val);
    writel(val, adr);
}

static inline unsigned long
READL(void *adr)
{
    unsigned long val = readl(adr);
    _DBG("READL(%s(%d)): ADDR=0x%08x.  VAL=0x%lx\n", __FUNCTION__, __LINE__,
         (int) adr, val);
    return val;
}

#ifdef CONFIG_PROC_FS
static struct proc_dir_entry *sdio_proc_dir = NULL;
/** 
	This function writes the required card information to the proc memory
*/

int sdio_read_ioreg(mmc_card_t card, u8 func, u32 reg, u8 * dat);

/**
        This function saves some SDHC registers before going to sleep. 
*/
int
sdio_suspend(mmc_card_t card)
{
    /*sdio_ctrller *ctrller = card->ctrlr;

       SAVED_SDHC_CLK_RATE     = READL(ctrller->base + MMC_CLK_RATE);
       SAVED_SDHC_CMD_DAT_CONT = READL(ctrller->base + MMC_CMD_DAT_CONT);
       SAVED_SDHC_RES_TO       = READL(ctrller->base + MMC_RES_TO);
       SAVED_SDHC_READ_TO      = READL(ctrller->base + MMC_READ_TO);
       SAVED_SDHC_BLK_LEN      = READL(ctrller->base + MMC_BLK_LEN);
       SAVED_SDHC_NOB          = READL(ctrller->base + MMC_NOB);
       SAVED_SDHC_INT_CNTR     = READL(ctrller->base + MMC_INT_CNTR);
     */
    return 0;
}

EXPORT_SYMBOL(sdio_suspend);

/**
        This function restores some SDHC registers when waking up. 
*/
int
sdio_resume(mmc_card_t card)
{
/*
    sdio_ctrller *ctrller = card->ctrlr;
    
    WRITEL(SAVED_SDHC_CLK_RATE,    ctrller->base + MMC_CLK_RATE);
    WRITEL(SAVED_SDHC_CMD_DAT_CONT,ctrller->base + MMC_CMD_DAT_CONT);
    WRITEL(SAVED_SDHC_RES_TO,      ctrller->base + MMC_RES_TO);
    WRITEL(SAVED_SDHC_READ_TO,     ctrller->base + MMC_READ_TO);
    WRITEL(SAVED_SDHC_BLK_LEN,     ctrller->base + MMC_BLK_LEN);
    WRITEL(SAVED_SDHC_NOB,         ctrller->base + MMC_NOB);
    WRITEL(SAVED_SDHC_INT_CNTR,    ctrller->base + MMC_INT_CNTR);
  */
    return 0;
}

EXPORT_SYMBOL(sdio_resume);

static int
sdio_read_procmem(char *buf, char **start, off_t offset,
                  int count, int *eof, void *data)
{
    int len = 0;
    sdio_ctrller *ctrller = (sdio_ctrller *) data;

    u8 byte1, byte;

#ifdef DEBUG_USEC
    int i;
#endif

    int status = READL(ctrller->base + MMC_STATUS);
    int cntr = READL(ctrller->base + MMC_INT_CNTR);
    int clkrt = READL(ctrller->base + MMC_CLK_RATE);

    len += sprintf(buf + len, "INT_CNTR=%x\n", cntr);
    len += sprintf(buf + len, "Status=%x\n", status);
    len += sprintf(buf + len, "clk Rate=%x\n", clkrt);

    sdio_read_ioreg(ctrller->card, FN1, 0x20, &byte);
    len += sprintf(buf + len, "reg[0x20]=%x\n", byte);

    sdio_read_ioreg(ctrller->card, FN1, 0x05, &byte1);
    len += sprintf(buf + len, "reg[0x05]=%x\n", byte1);

    WRITEL(cntr, ctrller->base + MMC_INT_CNTR);
    wmb();

#ifdef DEBUG_SDIO_LEVEL0
    dump_status(status);
#endif

#ifdef DEBUG_USEC
    len += sprintf(buf + len, "rcnt=%d wcnt=%d\n", rcnt, wcnt);
    for (i = 0; i < 100; i++)
        len += sprintf(buf + len, "[%d]: %lu\n", i, rbuf[i]);

    len += sprintf(buf + len, "===============\n");
    for (i = 0; i < 100; i++)
        len += sprintf(buf + len, "[%d]: %lu\n", i, wbuf[i]);

#endif

    *eof = 1;
    return len;
}

static int
create_proc_entry_forsdio(sdio_ctrller * ctrller)
{
    if (!ctrller) {
        _ERROR("Bad ctrller\n");
        return -1;
    }

    create_proc_read_entry("SDIO", 0, NULL, sdio_read_procmem, ctrller);
    return 0;
}
#endif

/*!
 * This function sets the SDHC register to stop the clock and waits for the
 * clock stop indication.
 */
static void
sdio_stop_clock(sdio_ctrller * host)
{
    int wait_cnt;

    _ENTER();
    disable_irq(host->irq_line);
    while (1) {
        /* Clear both STOP_CLK and START_CLK bits of STR_STP_CLK 
           register first, and then write '1' to STOP_CLK */
        WRITEL(0, host->base + MMC_STR_STP_CLK);
        wmb();
        WRITEL(STR_STP_CLK_STOP_CLK, host->base + MMC_STR_STP_CLK);
        wait_cnt = 0;
        while ((readl(host->base + MMC_STATUS) &
                STATUS_CARD_BUS_CLK_RUN) && (wait_cnt++ < 100));

        if (!(READL(host->base + MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN))
            break;
    }
    enable_irq(host->irq_line);
    _LEAVE();
}

int
stop_bus_clock_2(sdio_ctrller * host)
{
    _ENTER();
    sdio_stop_clock(host);
    _LEAVE();
    return 0;
}

#define CMD_WAIT_CNT 100
/*
 * This function sets the SDHC register to start the clock and waits for the
 * clock start indication. When the clock starts SDHC module starts processing
 * the command in CMD Register with arguments in ARG Register.
 */
static void
sdio_start_clock(sdio_ctrller * host)
{
    int wait_cnt;
    _ENTER();
    disable_irq(host->irq_line);
    while (1) {
        WRITEL(STR_STP_CLK_START_CLK, host->base + MMC_STR_STP_CLK);
        wait_cnt = 0;
        while (!(readl(host->base + MMC_STATUS) &
                 STATUS_CARD_BUS_CLK_RUN) && (wait_cnt++ < CMD_WAIT_CNT)) {
            /* Do Nothing */
        }
        if (READL(host->base + MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN) {
            break;
        }
    }
    enable_irq(host->irq_line);
    _LEAVE();
}

int
start_bus_clock(sdio_ctrller * ctrller)
{
    sdio_start_clock(ctrller);
    return 0;
}

/*!
 * This function resets the SDHC host.
 *
 * @param host  Pointer to MMC/SD  host structure
 */
static void
sdio_softreset(sdio_ctrller * host)
{
    _ENTER();
    /* reset sequence */
    WRITEL(0x8, host->base + MMC_STR_STP_CLK);
    WRITEL(0x9, host->base + MMC_STR_STP_CLK);
    WRITEL(0x1, host->base + MMC_STR_STP_CLK);
    WRITEL(0x1, host->base + MMC_STR_STP_CLK);
    WRITEL(0x1, host->base + MMC_STR_STP_CLK);
    WRITEL(0x1, host->base + MMC_STR_STP_CLK);
    WRITEL(0x1, host->base + MMC_STR_STP_CLK);
    WRITEL(0x1, host->base + MMC_STR_STP_CLK);
    WRITEL(0x1, host->base + MMC_STR_STP_CLK);
    WRITEL(0x1, host->base + MMC_STR_STP_CLK);
    WRITEL(0x3f, host->base + MMC_CLK_RATE);
    WRITEL(0xff, host->base + MMC_RES_TO);
    WRITEL(0xffff, host->base + MMC_READ_TO);
    WRITEL(512, host->base + MMC_BLK_LEN);
    WRITEL(1, host->base + MMC_NOB);
    _LEAVE();
}

/*!
 * This function is called to setup SDHC register for data transfer.
 * The function allocates DMA buffers, configures the SDMA channel.
 * Start the SDMA channel to transfer data. When DMA is not enabled this
 * function set ups only Number of Block and Block Length registers.
 *
 * @param host  Pointer to MMC/SD host structure
 * @param data  Pointer to MMC/SD data structure
 */
static void
sdio_setup_data(sdio_ctrller * host, struct mmc_data *data)
{
    unsigned int nob = data->blocks;

#ifdef MXC_MMC_DMA_ENABLE
    dma_request_t sdma_request;
    dma_channel_params params;
#endif

    _ENTER();

    if (data->flags & MMC_DATA_STREAM) {
        nob = 0xffff;
    }

    host->data = data;

    WRITEL(nob, host->base + MMC_NOB);
    WRITEL(1 << data->blksz_bits, host->base + MMC_BLK_LEN);

    host->dma_size = data->blocks << data->blksz_bits;
    _DBG("%s:Request bytes to transfer:%d\n", DRIVER_NAME, host->dma_size);
#ifdef MXC_MMC_DMA_ENABLE
    if (data->flags & MMC_DATA_READ) {
        host->dma_dir = DMA_FROM_DEVICE;
    } else {
        host->dma_dir = DMA_TO_DEVICE;
    }
    host->dma_len = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
                               host->dma_dir);
    if (data->flags & MMC_DATA_READ) {
        sdma_request.destAddr = (__u8 *) sg_dma_address(&data->sg[0]);
        params.transfer_type = per_2_emi;
    } else {
        sdma_request.sourceAddr = (__u8 *) sg_dma_address(&data->sg[0]);
        params.transfer_type = emi_2_per;
    }

    /*
     * Setup DMA Channel parameters
     */
    params.watermark_level = SDHC_MMC_WML;
    params.peripheral_type = MMC;
    params.per_address = host->res->start + MMC_BUFFER_ACCESS;
    params.event_id = host->event_id;
    params.bd_number = 1 /* host->dma_size / 256 */ ;
    params.word_size = TRANSFER_32BIT;
    params.callback = sdio_dma_irq;
    params.arg = host;
    mxc_dma_setup_channel(host->dma, &params);

    sdma_request.count = host->dma_size;
    mxc_dma_set_config(host->dma, &sdma_request, 0);

    /* mxc_dma_start(host->dma); */
#endif
    _LEAVE();
}

/*!
 * This function is called by \b mxcmmc_request() function to setup the SDHC
 * register to issue command. This function disables the card insertion and
 * removal detection interrupt.
 *
 * @param host  Pointer to MMC/SD host structure
 * @param cmd   Pointer to MMC/SD command structure
 * @param cmdat Value to store in Command and Data Control Register
 */
static void
sdio_start_cmd(sdio_ctrller * host, struct mmc_command *cmd,
               unsigned int cmdat)
{
    WARN_ON(host->cmd != NULL);
    host->cmd = cmd;
    switch (cmd->flags & (MMC_RSP_MASK | MMC_RSP_CRC)) {
    case MMC_RSP_SHORT | MMC_RSP_CRC:
        cmdat |= CMD_DAT_CONT_RESPONSE_FORMAT_R1;
        break;
    case MMC_RSP_SHORT:
        cmdat |= CMD_DAT_CONT_RESPONSE_FORMAT_R3;
        break;
    case MMC_RSP_LONG | MMC_RSP_CRC:
        cmdat |= CMD_DAT_CONT_RESPONSE_FORMAT_R2;
        break;
    default:
        /* No Response required */
        break;
    }

    if (cmd->opcode == MMC_GO_IDLE_STATE) {
        cmdat |= CMD_DAT_CONT_INIT;     /* This command needs init */
    }

    WRITEL(cmd->opcode, host->base + MMC_CMD);
    WRITEL(cmd->arg, host->base + MMC_ARG);

    WRITEL(cmdat, host->base + MMC_CMD_DAT_CONT);

    sdio_start_clock(host);
}

#if 0
/*!
 * GPIO interrupt service routine registered to handle the SDHC interrupts.
 * This interrupt routine handles card insertion and card removal interrupts. 
 *
 * @param   irq    the interrupt number
 * @param   devid  driver private data
 * @param   regs   holds a snapshot of the processor's context before the
 *                 processor entered the interrupt code
 *
 * @return  The function returns \b IRQ_RETVAL(1) 
 */
static irqreturn_t
sdio_gpio_irq(int irq, void *devid, struct pt_regs *regs)
{
    static int flag = 0;

    _DBG("%s: Card inserted/Removed \n", DRIVER_NAME);
#ifdef MOTO_PLATFORM
    sdhc_intr_clear(SDHC2_MODULE, &flag);
#else
    sdhc_intr_clear(&flag);
#endif
    mdelay(50);
    mmc_detect_change(0);
    return IRQ_RETVAL(1);
}
#endif

#ifdef DEBUG_SDIO_LEVEL0
static void
dump_status(int sts)
{
    unsigned int bitset;

    printk("status: ");
    while (sts) {
        /* Find the next bit set */
        bitset = sts & ~(sts - 1);
        switch (bitset) {
        case STATUS_CARD_INSERTION:
            printk("CARD_INSERTION|");
            break;
        case STATUS_CARD_REMOVAL:
            printk("CARD_REMOVAL |");
            break;
        case STATUS_YBUF_EMPTY:
            printk("YBUF_EMPTY |");
            break;
        case STATUS_XBUF_EMPTY:
            printk("XBUF_EMPTY |");
            break;
        case STATUS_YBUF_FULL:
            printk("YBUF_FULL |");
            break;
        case STATUS_XBUF_FULL:
            printk("XBUF_FULL |");
            break;
        case STATUS_BUF_UND_RUN:
            printk("BUF_UND_RUN |");
            break;
        case STATUS_BUF_OVFL:
            printk("BUF_OVFL |");
            break;
        case STATUS_READ_OP_DONE:
            printk("READ_OP_DONE |");
            break;
        case STATUS_WR_CRC_ERROR_CODE_MASK:
            printk("WR_CRC_ERROR_CODE |");
            break;
        case STATUS_READ_CRC_ERR:
            printk("READ_CRC_ERR |");
            break;
        case STATUS_WRITE_CRC_ERR:
            printk("WRITE_CRC_ERR |");
            break;
        case STATUS_SDIO_INT_ACTIVE:
            printk("SDIO_INT_ACTIVE |");
            break;
        case STATUS_END_CMD_RESP:
            printk("END_CMD_RESP |");
            break;
        case STATUS_WRITE_OP_DONE:
            printk("WRITE_OP_DONE |");
            break;
        case STATUS_CARD_BUS_CLK_RUN:
            printk("CARD_BUS_CLK_RUN |");
            break;
        case STATUS_BUF_READ_RDY:
            printk("BUF_READ_RDY |");
            break;
        case STATUS_BUF_WRITE_RDY:
            printk("BUF_WRITE_RDY |");
            break;
        case STATUS_RESP_CRC_ERR:
            printk("RESP_CRC_ERR |");
            break;
        case STATUS_TIME_OUT_RESP:
            printk("TIME_OUT_RESP |");
            break;
        case STATUS_TIME_OUT_READ:
            printk("TIME_OUT_READ |");
            break;
        default:
            printk("Ivalid Status Register value");
            break;
        }
        sts &= ~bitset;
    }
    printk("\n");
}
#endif

/*!
 * Interrupt service routine registered to handle the SDHC interrupts.
 * This interrupt routine handles end of command, card insertion and
 * card removal interrupts. If the interrupt is card insertion or removal then
 * inform the MMC/SD core driver to detect the change in physical connections.
 * If the command is END_CMD_RESP read the Response FIFO. If DMA is not enabled
 * and data transfer is associated with the command then read or write the data
 * from or to the BUFFER_ACCESS FIFO.
 *
 * @param   irq    the interrupt number
 * @param   devid  driver private data
 * @param   regs   holds a snapshot of the processor's context before the
 *                 processor entered the interrupt code
 *
 * @return  The function returns \b IRQ_RETVAL(1) if interrupt was handled,
 *          returns \b IRQ_RETVAL(0) if the interrupt was not handled.
 */
static irqreturn_t
sdio_irq(int irq, void *devid, struct pt_regs *regs)
{
    sdio_ctrller *host = devid;
    unsigned int status = 0;

    _ENTER();
    if (host == NULL) {
        _ERROR("host==NULL\n");
        return IRQ_RETVAL(1);
    }

    status = READL(host->base + MMC_STATUS);

    _DBG("sdio_irq: status=%x\n", status);

    if (status & STATUS_END_CMD_RESP) {
        WRITEL(STATUS_END_CMD_RESP, host->base + MMC_STATUS);
        wmb();
        complete(&host->completion);
    }

    if ((status & STATUS_SDIO_INT_ACTIVE) ||
        ((status & 0x0d000080) == 0x0d000080)) {
        int mask = READL(host->base + MMC_INT_CNTR);
        WRITEL(STATUS_SDIO_INT_ACTIVE, host->base + MMC_STATUS);
        wmb();

        _DBG("SDIO_INT\n");

        if (sdioint_handler && (mask & INT_CNTR_SDIO_IRQ_EN)) {
            mask &= ~(INT_CNTR_SDIO_IRQ_EN);
            WRITEL(mask, host->base + MMC_INT_CNTR);
            wmb();
            sdioint_handler(irq, net_devid, regs);
        }
    }

    _LEAVE();
    return IRQ_RETVAL(1);
}

/*!
 * This function is called by MMC/SD Bus Protocol driver to issue a MMC
 * and SD commands to the SDHC.
 *
 * @param  mmc  Pointer to MMC/SD host structure
 * @param  req  Pointer to MMC/SD command request structure
 */
static void
sdio_request(struct mmc_host *mmc, struct mmc_request *req)
{
    sdio_ctrller *host = mmc_priv(mmc);
    /* Holds the value of Command and Data Control Register */
    unsigned long cmdat;

    _ENTER();
    WARN_ON(host->req != NULL);

    host->req = req;
#ifdef CONFIG_MMC_DEBUG
    dump_cmd(req->cmd);
#endif
    cmdat = 0;
    if (req->data) {
        sdio_setup_data(host, req->data);

        cmdat |= CMD_DAT_CONT_DATA_ENABLE;

        if (req->data->flags & MMC_DATA_WRITE) {
            cmdat |= CMD_DAT_CONT_WRITE;
        }
        if (req->data->flags & MMC_DATA_STREAM) {
            _ERROR("MXC MMC does not support stream mode\n");
        }
    }
    sdio_start_cmd(host, req->cmd, cmdat);
    _LEAVE();
}

/*!
 * This function is called by MMC/SD Bus Protocol driver to change the clock
 * speed of MMC or SD card
 *
 * @param mmc Pointer to MMC/SD host structure
 * @param ios Pointer to MMC/SD I/O type structure
 */
static void
sdio_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{

    _ENTER();
    if (!sdio_controller) {
        _DBG("sdio_controller is NULL\n");
        _LEAVE();
        return;
    }

    sdio_stop_clock(sdio_controller);
    WRITEL(sdioclock, sdio_controller->base + MMC_CLK_RATE);
    sdio_start_clock(sdio_controller);

    _LEAVE();
    return;
}

/*!
 * MMC/SD host operations structure.
 * These functions are registered with MMC/SD Bus protocol driver.
 */
static struct mmc_host_ops sdio_ops = {
    .request = sdio_request,
    .set_ios = sdio_set_ios
};

#ifdef MXC_MMC_DMA_ENABLE
/*!
 * This function is called by SDMA Interrupt Service Routine to indicate
 * requested DMA transfer is completed.
 *
 * @param   devid  pointer to device specific structure
 */
static void
sdio_dma_irq(void *devid)
{
    sdio_ctrller *host = devid;
    struct mmc_data *data = host->data;
    u32 status;
    ulong nob, blk_size, i, blk_len;
    /* This variable holds DMA configuration parameters */
    dma_request_t sdma_request;

    _ENTER();
    _DBG("Got DMA_IRQ!\n");

    complete(&host->completion);
    return;

    mxc_dma_stop(host->dma);

    mxc_dma_get_config(host->dma, &sdma_request, 0);

    if (sdma_request.bd_done == 1) {
        _ERROR("Re-transmit...\n");
        mxc_dma_start(host->dma);
        return;
    }

    _DBG("%s: Transfered bytes:%d\n", DRIVER_NAME, sdma_request.count);
    nob = READL(host->base + MMC_REM_NOB);
    blk_size = READL(host->base + MMC_REM_BLK_SIZE);
    blk_len = READL(host->base + MMC_BLK_LEN);
    _DBG("%s: REM_NOB:%lu REM_BLK_SIZE:%lu\n", DRIVER_NAME, nob, blk_size);

    i = 0;
    do {
        status = READL(host->base + MMC_STATUS);
        if (i++ > 10000)
            break;

    } while (!(status & (STATUS_READ_OP_DONE | STATUS_WRITE_OP_DONE)));

    if (i > 10000)
        _ERROR("%s: i=%lu\n", __FUNCTION__, i);

    _DBG("DMA STATUS: %x\n", status);

#ifdef DEBUG_SDIO_LEVEL0
    dump_status(status);
#endif
    if (status & (STATUS_READ_OP_DONE | STATUS_WRITE_OP_DONE)) {
        _DBG("%s:READ/WRITE OPERATION DONE\n", DRIVER_NAME);
        /* check for time out and CRC errors */
        status = READL(host->base + MMC_STATUS);
        if (status & STATUS_READ_OP_DONE) {
            if (status & STATUS_TIME_OUT_READ) {
                _DBG("%s: Read time out occurred\n", DRIVER_NAME);
                data->error = MMC_ERR_TIMEOUT;
                WRITEL(STATUS_TIME_OUT_READ, host->base + MMC_STATUS);
            } else if (status & STATUS_READ_CRC_ERR) {
                _DBG("%s: Read CRC error occurred\n", DRIVER_NAME);
                data->error = MMC_ERR_BADCRC;
                WRITEL(STATUS_READ_CRC_ERR, host->base + MMC_STATUS);
            }
            WRITEL(STATUS_READ_OP_DONE, host->base + MMC_STATUS);
        }

        /* check for CRC errors */
        if (status & STATUS_WRITE_OP_DONE) {
            if (status & STATUS_WRITE_CRC_ERR) {
                _DBG("%s: Write CRC error occurred\n", DRIVER_NAME);
                data->error = MMC_ERR_BADCRC;
                WRITEL(STATUS_WRITE_CRC_ERR, host->base + MMC_STATUS);
            }
            WRITEL(STATUS_WRITE_OP_DONE, host->base + MMC_STATUS);
        }
    }

    complete(&host->completion);

    _LEAVE();
}
#endif

/*!
 * Get ownership of this SDHC shared peripheral. Master is decided based on
 * whether we need DMA transfer or not.
 */
static int
sdio_getownership(void)
{
#ifdef MOTO_PLATFORM
    return spba_take_ownership(SPBA_SDHC2, SPBA_MASTER_A | SPBA_MASTER_C);
#else
    return spba_take_ownership(SPBA_SDHC1, SPBA_MASTER_A | SPBA_MASTER_C);
#endif
}

/*!
 * Release ownership of this SDHC shared peripheral. Master is decided 
 * based on whether we need DMA transfer or not.
 */
static int
sdio_relownership(void)
{
#ifdef MOTO_PLATFORM
    return spba_rel_ownership(SPBA_SDHC2, SPBA_MASTER_A | SPBA_MASTER_C);
#else
    return spba_rel_ownership(SPBA_SDHC1, SPBA_MASTER_A | SPBA_MASTER_C);
#endif
}

/*!
 * sdio_probe:
 */
static int
sdio_probe(sdio_ctrller ** ctrller_ptr)
{
    struct _sdio_host *host = 0;
    struct resource *memr, *irqr;
    int ret = 0, irq = NO_IRQ;

#ifdef MXC_MMC_DMA_ENABLE
    int err;
#endif

    _ENTER();
    *ctrller_ptr = 0;

    memr = &sdio1_resources[0];

    irqr = &sdio1_resources[1];

    irq = irqr->start;

    if (!memr || irq == NO_IRQ) {
        return -ENXIO;
    }

    memr = request_mem_region(memr->start, SDHC_MEM_SIZE, DRIVER_NAME);
    if (!memr) {
        return -EBUSY;
    }

    mmc =
        kmalloc(sizeof(struct _sdio_host) + sizeof(struct mmc_host),
                GFP_ATOMIC);

    if (!mmc) {
        ret = -ENOMEM;
        goto out;
    }

    memset(mmc, 0, sizeof(struct _sdio_host) + sizeof(struct mmc_host));
    INIT_LIST_HEAD(&mmc->cards);

    mmc->ops = &sdio_ops;
    mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;
    mmc->max_phys_segs = NR_SG;

    host = mmc_priv(mmc);
    host->mmc = mmc;
    host->dma = -1;

#ifdef MOTO_PLATFORM
    host->event_id = DMA_REQ_SDHC2;
    host->clock_id = SDHC2_CLK;
#else
    host->event_id = DMA_REQ_SDHC1;
    host->clock_id = SDHC1_CLK;
#endif

    mmc->f_min = sdhc_get_min_clock(host->clock_id);
    mmc->f_max = sdhc_get_max_clock(host->clock_id);

    _DBG("mmc->f_min=0x%08x.  mmc->f_max=0x%08x\n", mmc->f_min, mmc->f_max);
    _DBG("SDHC:%d clock:%lu\n", 0, mxc_get_clocks(host->clock_id));

    spin_lock_init(&host->lock);
    init_completion(&host->completion);
    host->res = memr;
    host->irq = irq;

    host->base = ioremap(memr->start, SDHC_MEM_SIZE);

    _DBG("host->base=%p\n", host->base);
    _DBG("memr->start=%lx\n", memr->start);
#ifdef MOTO_PLATFORM
    _DBG(" MMC_SDHC2_BASE_ADDR= %x\n", MMC_SDHC2_BASE_ADDR);
#else
    _DBG(" MMC_SDHC1_BASE_ADDR= %x\n", MMC_SDHC1_BASE_ADDR);
#endif

    if (!host->base) {
        ret = -ENOMEM;
        goto out;
    }

    /* Get the ownership */
    ret = sdio_getownership();
    if (ret != 0) {
        _ERROR("%s: Failed to get SPBA ownership\n", DRIVER_NAME);
        return ret;
    }
#ifdef MOTO_PLATFORM
    /* Power up the Wlan chipset according to Power up sequence */
    sdio_wprm_wlan_power_on();
#else
    gpio_sdhc_active(0);
#endif
#if 0
#ifdef MOTO_PLATFORM
    ret = sdhc_intr_setup(SDHC2_MODULE, host, sdio_gpio_irq);
#else
    ret = sdhc_intr_setup(host, sdio_gpio_irq);
#endif
    if (ret != 0) {
        goto out;
    }
#endif
    sdio_softreset(host);

    if (READL(host->base + MMC_REV_NO) != SDHC_REV_NO) {
        _DBG("%s: wrong rev.no. 0x%08x. aborting.\n",
             DRIVER_NAME, MMC_REV_NO);
        goto out;
    }
    WRITEL(READ_TO_VALUE, host->base + MMC_READ_TO);

#ifdef MXC_MMC_DMA_ENABLE
    host->dma = 0;
    if ((err = mxc_request_dma(&host->dma, DRIVER_NAME)) < 0) {
        _ERROR("%s: %s:err = %d\n", __FUNCTION__, DRIVER_NAME, err);
        goto out;
    } else
        printk("mxc_request_dma write OK. dma chan=%d\n", host->dma);

#endif

    ret = request_irq(host->irq, sdio_irq, 0, DRIVER_NAME, host);
    if (ret) {
        _ERROR("request IRQ Fail. RET=%d\n", ret);
        goto out;
    }
    host->irq_line = host->irq;

    mmc_detect_change(host->mmc);

#ifdef MOTO_PLATFORM
    ret = sdhc_find_card(SDHC2_MODULE);
    /* Workaround : return value should be greater than zero if a card is present.
       Waiting for sdhc_find_card to be correctly implemented to remove this workaround. */
    if (ret >= 0) {
#else
    ret = sdhc_find_card();
    if (!ret) {
#endif

        _DBG("Card Present");
        mdelay(50);
        *ctrller_ptr = host;
        return 0;
    }

  out:
    _ERROR("%s: Error in initializing....", DRIVER_NAME);
    if (host) {
#ifdef MXC_MMC_DMA_ENABLE
        if (host->dma >= 0) {
            mxc_free_dma(host->dma);
        }
#endif
        if (host->base) {
            iounmap(host->base);
        }
    }
    release_resource(memr);

    _LEAVE();
    return ret;
}

/*
 * sdio_remove:
 */

static int
sdio_remove(sdio_ctrller * ctrller)
{
    int ret = 0;

    struct _sdio_host *host = sdio_controller;

    _ENTER();

    ret = sdio_relownership();
    if (ret != 0) {
        _ERROR("%s: Failed to release SPBA ownership\n", DRIVER_NAME);
        return ret;
    }
#if 0
    sdhc_intr_cleanup(host);
#endif
#ifdef MOTO_PLATFORM
    /* Power down the Wlan chipset */
    sdio_wprm_wlan_power_off();
#else
    gpio_sdhc_inactive(0);
#endif
    free_irq(host->irq, host);

#ifdef MXC_MMC_DMA_ENABLE
    mxc_free_dma(host->dma);
#endif

    iounmap(host->base);
    release_resource(host->res);

    _LEAVE();
    return 0;
}

/** 
	This function initiailizes the semaphore 

	\param pointer to controller structure
*/

static void
init_semaphores(sdio_ctrller * ctrller)
{
    init_MUTEX(&ctrller->io_sem);
    return;
}

static void *
register_sdiohost(void *ops, int extra)
{
    sdio_ctrller *ctrller;
    mmc_card_t card;

    _ENTER();

    /* - Initialize the SDIO Controller 
     * Allocate memory to the card structure
     * Request for a DMA channel
     * Request for the IRQ
     */

    memset(&sdio_dma_data, 0, sizeof(sdio_dma_data));
    memset(&sdio_dma_sg, 0, sizeof(sdio_dma_sg));

    if (sdio_probe(&ctrller) < 0) {
        _ERROR("Failed to probe host controller\n");
        goto out;
    }

    if (!ctrller) {
        _ERROR("sdio_probe failed bailing out ...\n");
        goto out;
    }

    init_semaphores(ctrller);

    if (!(card = kmalloc(sizeof(mmc_card_rec), GFP_ATOMIC))) {
        _ERROR("Cannot allocate memory for card!\n");
        goto out;
    }

    card->ctrlr = ctrller;      /* Back Reference */
    ctrller->card = card;
    ctrller->tmpl = kmalloc(sizeof(dummy_tmpl), GFP_ATOMIC);

    if (!ctrller->tmpl) {
        _ERROR("Kmalloc failed bailing out ...\n");
        goto out;
    }

    ctrller->tmpl->irq_line = ctrller->irq;
    return ctrller;

  out:
    if (ctrller) {
        if (ctrller->card)
            kfree(ctrller->card);

        if (ctrller->tmpl)
            kfree(ctrller->tmpl);

        kfree(ctrller);
    }
    return NULL;
}

/**
	Registers the WLAN driver and makes a call back function to the WLAN 
	driver's entry point

	\param pointer to the function passed by the wlan layer, pointer to the 
	controller structure
*/

static void *
register_sdiouser(mmc_notifier_rec_t * ptr, sdio_ctrller * ctrller)
{
    _ENTER();

    /* Increment the count */
    if (MODULE_GET == 0)
        return NULL;

    if (!ptr) {
        _ERROR("null func pter\n");
        goto exit;
    }

    if (!ptr->add) {
        _ERROR("No Add function pointers\n");
        goto exit;
    }

    if (!ctrller) {
        _ERROR("Null ctrller\n");
        goto exit;

    }

    _DBG("Value of the sdio_ctrller = %p\n", ctrller);
    _DBG("Value of card = %p\n", ctrller->card);
    if (ptr->add(ctrller->card))
        goto exit;

    /* Initialize the DMA channel .. By now we know that it is the 
     * Marvell SD Card */
    _DBG("returning the func pter value =%p\n", ptr);

    _LEAVE();
    return ptr;
  exit:
    _ERROR("register sdio user error!\n");
    _LEAVE();
    return 0;
}

/** 
	This function unregisters the WLAN driver

	\param Function pointer passed by the WLAN driver
*/

static int
unregister_sdiouser(mmc_notifier_rec_t * ptr)
{
    _ENTER();

        /** Decrement the count so that the Wlan interface can be 
	 * detached
	 */
    MODULE_PUT;

    if (!ptr) {
        _ERROR("Null pointer passed...\n");
        goto exit;
    }

    if (!ptr->remove) {
        _ERROR("No Remove function pointers\n");
        goto exit;
    }

    ptr->remove(sdio_controller->card);

    _LEAVE();
    return 0;
  exit:
    _ERROR("unregister_sdiouser recieved null function pointer\n");
    return -1;
}

sdio_operations sops = {
  name:"SDIO DRIVER",
};

/** 
	Dummy function holders RFU
*/

int
sdio_open(struct inode *in, struct file *file)
{
    if (MODULE_GET == 0)
        return -1;

    return 0;
}

int
sdio_release(struct inode *in, struct file *file)
{
    _ENTER();

    MODULE_PUT;

    unregister_chrdev(major_number, "sdio");

    _LEAVE();
    return 0;
}

ssize_t
sdio_write(struct file * file, const char *buf, size_t size, loff_t * ptr)
{
    return 0;
}

ssize_t
sdio_read(struct file * file, char *buf, size_t size, loff_t * ptr)
{
    return 0;
}

int
sdio_ioctl(struct inode *in, struct file *file, uint val, ulong val1)
{
    return 0;
}

struct file_operations sdio_fops = {
  owner:THIS_MODULE,
  open:sdio_open,
  release:sdio_release,
  read:sdio_read,
  write:sdio_write,
  ioctl:sdio_ioctl,
  llseek:NULL,
};

/** 
	This function registers the card as HOST/USER(WLAN)

	\param type of the card, function pointer
	\return returns pointer to the type requested for
*/
void *
sdio_register(int type, void *ops, int memory)
{
    void *ret = NULL;

    _ENTER();

    switch (type) {

    case MMC_TYPE_HOST:
        ret = register_sdiohost(ops, memory);

        if (!ret) {
            _ERROR("sdio_register failed\n");
            return NULL;
        }

        _DBG("register_sdio_controller val of ret = %p\n", ret);
        break;

    case MMC_REG_TYPE_USER:
        /* There can be multiple regestrations
         * Take care of serializing this
         */
        ret = register_sdiouser(ops, sdio_controller);
        break;

    default:
        break;
    }

    _LEAVE();
    return ret;
}

/** 
	This function is a hook to the WLAN driver to unregister
	
	\param type to be registered, function pointer
*/

int
sdio_unregister(int type, void *ops)
{
    _ENTER();
    switch (type) {
    case MMC_TYPE_HOST:
        break;
    case MMC_REG_TYPE_USER:
        unregister_sdiouser(ops);
        break;

    default:
        break;
    }
    _LEAVE();
    return 0;
}

/** 
	This function acquires the semaphore and makes sure there is 
	serializes the data

	\param pointer to sdio_controller structure
	\returns 0 on success else negative value given by the OS is returned
*/

static int
acquire_io(sdio_ctrller * ctrller)
{
    int ret = -1;

    _ENTER();
    if (!ctrller) {
        _ERROR("Bad ctrller\n");
        _LEAVE();
        return ret;
    }

    down(&ctrller->io_sem);

    _LEAVE();
    return 0;
}

/** 
	This function release the semaphore and makes sure there is 
	serializes the data

	\param pointer to sdio_controller structure
	\returns 0 on success 
*/

static int
release_io(sdio_ctrller * ctrller)
{
    int ret = -1;

    _ENTER();
    if (!ctrller) {
        _ERROR("Bad ctrller\n");
        _LEAVE();
        return ret;
    }

    up(&ctrller->io_sem);

    _LEAVE();
    return 0;
}

/** 
	This function sets the state of the contrller to the desired state
	
	\param pointer to controller structure, and the seired state
*/

static int
set_state(sdio_ctrller * ctrller, int state)
{

    _ENTER();
    if (state == SDIO_FSM_IDLE) {
        _DBG("%s: enable SDIO_INT\n", __FUNCTION__);
        disable_irq(ctrller->irq_line);

        WRITEL(INT_CNTR_SDIO_IRQ_EN, ctrller->base + MMC_INT_CNTR);
        wmb();
        enable_irq(ctrller->irq_line);
    }

    ctrller->state = state;

    _LEAVE();
    return 0;
}

/**
	This function checks for the error status for the response R5
	This is the response for CMD52 or CMD53
		
	\param controller pointer to sdio_ctrller structure
	\returns 0 on success else negative value is returned
*/

static int
check_for_err_response_mmc_r5(sdio_ctrller * ctrller)
{
    u32 err;
    int ret = SDIO_ERROR_R5_RESP;

    _ENTER();

    /* The response R5 contains the error code in 23-31 bits
     */
    err = ctrller->mmc_res[3];

    err = err << 8;

    err |= ctrller->mmc_res[4];

    if (!(err & 0xff))
        return 0;

    _LEAVE();
    return ret;
}

/** 
	This function checks for the error status for the response R6
  	This is the response for CMD3

	\param controller pointer to sdio_ctrller structure
	\returns 0 on success else negative value is returned
*/

static int
check_for_err_response_mmc_r6(sdio_ctrller * ctrller)
{
    u16 err = 0;
    int ret = SDIO_R6_ERROR;

    _ENTER();
    /*SDIO Spec : The bits 13-15 indicates the error for the 
     * response r6 (CMD3 response)
     */
    err |= ctrller->mmc_res[2] << 8;
    err |= ctrller->mmc_res[1] << 0;

    if (!(err & 0xe0))
        ret = 0;

    _LEAVE();
    return ret;
}

static int
check_for_err_response_mmc_r1(sdio_ctrller * ctrller)
{
    u32 status;
    int ret = SDIO_ERROR_R1_RESP;

    _ENTER();
    /* SDIO Spec : Response to Command 7 (in R1) contains the 
     * error code from bits 13-31 */
    status = ctrller->mmc_res[4];
    status = status << 24;
    status |= ctrller->mmc_res[3];
    status = status << 16;
    status |= ctrller->mmc_res[2];
    status = status << 8;
    status |= status << 0;

    _DBG("status4=0x%08x\n", status);
    if (!(status & 0xffffe000))
        ret = 0;

    _LEAVE();
    return ret;
}

/** 
	This function interprets the command response as described in the
	SDIO spec

	\param controller pointer to sdio_ctrller structure
	\returns 0 on success else negative value is returned
*/

static int
interpret_cmd_response(sdio_ctrller * ctrller, mmc_response format)
{
    int nwords, ret;

    _ENTER();
    _DBG("format=0x%08x\n", format);

    if (ctrller->io_reset) {
        ret = 0;
        goto suc_exit;
    }

    switch (format) {
    case MMC_NORESPONSE:
        nwords = -1;
        break;
    case MMC_R1:
        nwords = 3;
        break;
    case MMC_R2:
        nwords = 8;
        break;
    case MMC_R3:
        nwords = 3;
        break;
    case MMC_R4:
        nwords = 3;
        break;
    case MMC_R5:
        nwords = 3;
        break;
    case MMC_R6:
        nwords = 3;
        break;
    default:
        nwords = -1;
        break;
    }

    ret = nwords;

    if (nwords > 0) {
        register int i;
        int ibase;
        u16 resp;
        for (i = nwords - 1; i >= 0; i--) {
            // resp = MMC_RES;
            resp = READL(ctrller->base + MMC_RES_FIFO);
            ibase = i << 1;

            ctrller->mmc_res[ibase] = (u8) resp;
            resp = resp >> 8;
            ctrller->mmc_res[ibase + 1] = resp & 0x00ff;
            _DBG("*** ctrller->mmc_res[ibase + 1]=0x%08x\n",
                 ctrller->mmc_res[ibase + 1]);
            --ret;
        }

        if (format == MMC_R1)
            ret = check_for_err_response_mmc_r1(ctrller);

        if (ret < 0) {
            _ERROR("Error in MMC_R1 Response ..\n");
            goto exit;
        }
    }
    _DBG("format=0x%08x\n", format);

    if (format == MMC_R5)
        ret = check_for_err_response_mmc_r5(ctrller);

    else if (format == MMC_R6)
        ret = check_for_err_response_mmc_r6(ctrller);

    if (ret < 0) {
        _ERROR("Error: In the MMC Response\n");

        goto exit;
    }

  suc_exit:
    /* No errors , Lets set the state to End Cmd */
    set_state(ctrller, SDIO_FSM_END_CMD);
    _LEAVE();
    return ret;

  exit:
    _ERROR("Error in response of MMC_R5 Or MMC_R6\n");
    _LEAVE();
    return ret;
}

/** 
	This function checks the error code that is returned by the
 	MMC_STAT. This is useful to locate the problems in the 
 	card / controller
	
	\param controller pointer to sdio_ctrller structure
	\returns 0 on success else negative value is returned
*/

static int
check_for_errors(sdio_ctrller * ctrller, u32 status)
{
    u16 tmp;
    int ret = MMC_ERROR_GENERIC;

    _ENTER();
    /* Check for the good condition first! If there are no errors
     * return back, else check the state of the mmc_stat to
     * know the cause of the error
     */
    tmp = (status & MMC_STAT_ERRORS);

    _DBG("tmp success ? 0x%08x\n", tmp);

    if (!tmp || ctrller->io_reset)
        ret = 0;
    else {
        WRITEL(tmp, ctrller->base + MMC_STATUS);

        _ERROR("Error status = %#x\n", status);
        if (status & STATUS_RESP_CRC_ERR) {
            _ERROR("RESP_CRC_ERR\n");
        }
        if (status & STATUS_TIME_OUT_RESP) {
            _ERROR("TIME_OUT_RESP\n");
        }
        if (status & STATUS_TIME_OUT_READ) {
            _ERROR("TIME_OUT_READ\n");
        }
        if (status & STATUS_WRITE_CRC_ERR) {
            _ERROR("WRITE_CRC_ERR\n");
        }
        if (status & STATUS_READ_CRC_ERR) {
            _ERROR("READ_CRC_ERR\n");
        }
    }

    _LEAVE();
    return ret;
}

/**
	This function waits for the interrupt that has been set previously
	If this function returns success , indicates that the interrupt
 	that was awaited is done

	\param controller pointer to sdio_ctrller structure
	\returns 0 on success else negative value is returned
*/

static int
sdio_wait_for_interrupt(sdio_ctrller * ctrller, int status)
{
    int ret = 0;

    _ENTER();

    ret = xchg(&ctrller->busy, 1);

    if (!ret) {
        _ERROR("Err: %s ret =%d\n", __FUNCTION__, ret);
        goto done;
    }

    _DBG("Waiting for the interrupt to occur\n");

    sdio_start_clock(ctrller);

    wait_for_completion(&ctrller->completion);

  done:
    xchg(&ctrller->busy, 0);
    _LEAVE();
    return ret;
}

/** 
	This function sets/Masks the Required flag and 
	indicates the initialization is done

	\param controller pointer to sdio_ctrller structure, mask 
	\returns 0 on success else negative value is returned
*/

static int
sdio_initialize_ireg(sdio_ctrller * ctrller, int mask)
{
    int ret = 0;

    _ENTER();

    ret = xchg(&ctrller->busy, 1);

    _DBG("xchg success value = %d\n", ret);

    if (ret) {
        _ERROR("Error initialize ireg = %d\n", ret);
        goto error;
    }
#if IRQ_DEBUG
    ctrller->irqcnt = 1;
#endif

    init_completion(&ctrller->completion);

    disable_irq(ctrller->irq_line);
    WRITEL(mask, ctrller->base + MMC_INT_CNTR);
    wmb();
    enable_irq(ctrller->irq_line);

    _LEAVE();
    return 0;

  error:
    _ERROR("error in sdio_initialize_ireg\n");
    _LEAVE();
    return -1;
}

/*
	This function sets the interrupt flag in the appropriate registers
 	and waits for the interrupt to occur

	\param controller pointer to sdio_ctrller structure, response_t format, 
	and the flag
	\returns 0 on success else negative value is returned
*/

static mmc_error_t
send_command_poll(sdio_ctrller * ctrller, mmc_response format, int flag)
{
    int ret;
    int time_out = 50;
    unsigned int status = 0;

    _ENTER();

    /* For CMD53 read, it's not necessary to restart the clock */
    status = READL(ctrller->base + MMC_CMD_DAT_CONT);
    if (((CMD_DAT_CONT_WRITE & status) != 0) ||
        ((CMD_DAT_CONT_DATA_ENABLE & status) == 0)) {
        sdio_start_clock(ctrller);
    }

    while (time_out > 0) {
        status = READL(ctrller->base + MMC_STATUS);
        if ((STATUS_END_CMD_RESP & status) != 0)
            break;
        time_out--;
        udelay(1);
    }

    WRITEL(STATUS_END_CMD_RESP, ctrller->base + MMC_STATUS);

    /* -Ideally the response should be located on 
     * MMC_RES, check for the ERRORS first and 
     * read the response from the card 
     */
    ret = check_for_errors(ctrller, status);
    if (ret < 0) {
        _ERROR("Response error %d\n", ret);
        goto exit;
    }

    ret = interpret_cmd_response(ctrller, format);

    if (ret < 0) {
        _ERROR("Error in interpret cmd_response\n");
        goto exit;
    }

  exit:
    _LEAVE();
    return ret;
}

static mmc_error_t
send_command(sdio_ctrller * ctrller, mmc_response format, int flag)
{
    int ret;
    u32 status;

    _ENTER();

    sdio_initialize_ireg(ctrller, INT_CNTR_END_CMD_RES);

    sdio_wait_for_interrupt(ctrller, INT_CNTR_END_CMD_RES);

    /* -Ideally the response should be located on 
     * MMC_RES, check for the ERRORS first and 
     * read the response from the card 
     */
    status = READL(ctrller->base + MMC_STATUS);
    ret = check_for_errors(ctrller, status);
    if (ret < 0) {
        _ERROR("Response error %d\n", ret);
        goto exit;
    }

    ret = interpret_cmd_response(ctrller, format);

    if (ret < 0) {
        _ERROR("Error in interpret cmd_response\n");
        goto exit;
    }

  exit:
    _LEAVE();
    return ret;
}

/** 
	This function is used to either read or write a byte of data
	(CMD52) , fills up the CMD52 and sends it out on the cmd line

	\param pointer to sdio_controller structure , and contents of ioreg
	\returns 0 on success else negative value given by the OS is returned
*/

static int
rw_ioreg(sdio_ctrller * ctrller, ioreg_t * ioreg)
{
    int ret;
    u16 argh = 0, argl = 0;

    _ENTER();

    _DBG("C52:%c reg:%x\n", (ioreg->read_or_write == SDIO_READ) ? 'R' : 'W',
         ioreg->reg_addr);

#ifdef DEBUG_USEC
    _DBG("C52 Time=%lu\n", get_usecond());
#endif

    ret = acquire_io(ctrller);
    ctrller->num_ofcmd52++;

    if (ret < 0) {
        _ERROR("acquire io failed\n");
        goto exit;
    }

    sdio_stop_clock(ctrller);
    /* SDIO Spec: CMD52 is 48 bit long. 
       -----------------------------------------------------------
       S|D|CMDIND|R|FUN|F|S|REGISTER  ADDRESS|S|WRITEBIT|CRC    7|
       -----------------------------------------------------------
       The Command and the Command index will be filled by the SDIO Controller
       (48 - 16)
       So fill up argh (16 bits) argl (16 bits) with 
       R/W flag (1)
       FUNC NUMBER (3)
       RAW FLAG (1)
       STUFF (1)
       REG ADDR (17)
       and the Write data value or a Null if read
     */

    argh =
        (ioreg->read_or_write ? (1 << 15) : 0) |
        (ioreg->function_num << 12) |
        (ioreg->read_or_write == 2 ? (1 << 11) : 0) |
        ((ioreg->reg_addr & 0x0001ff80) >> 7);

    argl =
        ((ioreg->reg_addr & 0x0000007f) << 9) |
        (ioreg->read_or_write ? ioreg->dat : 0);

    WRITEL(CMD(52), ctrller->base + MMC_CMD);
    WRITEL(argh << 16 | argl, ctrller->base + MMC_ARG);

    wmb();

#define HOST_INTSTATUS_REG 0x05
#define CARD_RESET 8

    WRITEL(0x201, ctrller->base + MMC_CMD_DAT_CONT);

    if ((ioreg->reg_addr == IO_ABORT_REG) && ioreg->read_or_write &&
        (ioreg->function_num == FN0) &&
        ((ioreg->dat & CARD_RESET) == CARD_RESET)) {
        int mask = READL(ctrller->base + MMC_INT_CNTR);
        _DBG("Turning on INT because of IO_ABORT\n");
        mask |= INT_CNTR_SDIO_IRQ_EN;
        WRITEL(mask, ctrller->base + MMC_INT_CNTR);
        ctrller->io_reset = 1;
    }

    ret = send_command(ctrller, MMC_R5, 0);

    if (ret < 0) {
        _ERROR("send_command failed\n");
        goto exit;
    }

    ioreg->dat = MMC_RESPONSE(ctrller, 1);

    _DBG("ioreg->dat = %x\n", ioreg->dat);
    _DBG("-----C52E\n");

  exit:
    disable_irq(ctrller->irq_line);
    WRITEL(INT_CNTR_SDIO_IRQ_EN, sdio_controller->base + MMC_INT_CNTR);
    enable_irq(ctrller->irq_line);

    release_io(ctrller);
    _LEAVE();
    return ret;
}

/*!
 * This function is used to initialize the MMC/SD driver module. The function
 * registers the power management callback functions with the kernel and also
 * registers the MMC/SD callback functions with the core MMC/SD driver.
 *
 * @return  The function returns 0 on success and a non-zero value on failure.
 */
static int __init
sdio_init(void)
{
    int ret = 0, fail = 0;

    _ENTER();

    sdio_controller = sdio_register(MMC_TYPE_HOST, &sops, 0);

    if (!sdio_controller) {
        _ERROR("SDIO Registration failed\n");
        ret = -1;
        goto exit;
    }

    if ((fail = register_chrdev(major_number, "sdio", &sdio_fops)) < 0) {
        _ERROR("register_chrdev with major %d failed with error %d\n",
               major_number, fail);
        _ERROR("Unable to register the sdio driver as a char dev\n");
        ret = -1;
        goto exit;
    }
#ifdef CONFIG_PROC_FS
    sdio_proc_dir = proc_mkdir("sdio", NULL);
#endif

    if (fail) {
        major_number = fail;
        printk("<1>SDIO device registered.\n");
        printk("<1>To access, use: \"mknod /dev/sdio c %d 0\"\n",
               major_number);
    }

    card_setup(sdio_controller);

  exit:
    _LEAVE();
    return ret;
}

/*!
 * This function is used to cleanup all resources before the driver exits.
 */
static void __exit
sdio_exit(void)
{
    ulong flags;

    _ENTER();

    unregister_chrdev(major_number, "sdio");

    sdio_softreset(sdio_controller);
    card_release(sdio_controller);

    spin_lock_irqsave(&sdio_controller->lock, flags);

    sdio_remove(sdio_controller);

    spin_unlock_irqrestore(&sdio_controller->lock, flags);

    if (sdio_controller->card)
        kfree(sdio_controller->card);

    if (sdio_controller->tmpl)
        kfree(sdio_controller->tmpl);
    if (sdio_controller->mmc)
        kfree(sdio_controller->mmc);

#ifdef CONFIG_PROC_FS
    if (sdio_proc_dir)
        remove_proc_entry("sdio", NULL);
    remove_proc_entry("SDIO", NULL);
#endif
    _LEAVE();
    return;
}

/** 
	This function is used to check the upper bound for the functions 
	CMD52

	\param pointer to sdio_controller structure , and the contents of ioreg 
	passed by the wlan layer
	\returns 0 on success else negative value is returned
*/

static int
check_for_valid_ioreg(sdio_ctrller * ctrller, ioreg_t * ioreg)
{
    _ENTER();

    if (!ioreg) {
        _ERROR("Bad ioreg\n");
        return -EINVAL;
    }

    if (ioreg->function_num > 7 || ioreg->function_num >
        ctrller->card_capability.num_of_io_funcs) {
        return -EINVAL;
    }

    _LEAVE();
    return 0;
}

/** 
	This function is the hook to the wlan driver to read a byte of data 
	from the card

	\param pointer to the card structure, function num and register address
	value to be read 
	\returns 0 on success negative value on error
*/

int
sdio_read_ioreg(mmc_card_t card, u8 func, u32 reg, u8 * dat)
{
    int ret = -1;
    ioreg_t ioreg;
    sdio_ctrller *ctrller = card->ctrlr;

    _ENTER();
    /* SDIO Spec: Command 52 needs 
     * R/W Flag if 0 this will be read data if 1 write
     * Function number: Number of the function within I/O 
     * Register Address: This is the address of the  register
     * write data: This bit is set to 0 for read
     * CRC7: 7 bits CRC
     */
    if (!ctrller) {
        _ERROR("Bad ctrller recieved sdio_read_ioreg\n");
        goto exit;
    }

    ioreg.read_or_write = SDIO_READ;
    ioreg.function_num = func;
    ioreg.reg_addr = reg;
    ioreg.dat = 0x00;     /** Will be filled by the card */

    ret = check_for_valid_ioreg(ctrller, &ioreg);

    if (ret < 0) {
        _ERROR("Wrong parameters for rw_ioreg\n");
        goto exit;
    }

    ret = rw_ioreg(ctrller, &ioreg);

    if (ret < 0) {
        _ERROR("sdio_read_ioreg failed, fn=%d, reg=%#x\n", func, reg);
    } else
        *dat = ioreg.dat;

  exit:
    _LEAVE();
    return ret;
}

/** 
	This function is the hook to the wlan driver to write a byte of data

	\param pointer to the card structure, function number, register address
	and the data to be written to the card
	\return returns 0 on success , negate value on failure
*/

int
sdio_write_ioreg(mmc_card_t card, u8 func, u32 reg, u8 dat)
{
    int ret;
    sdio_ctrller *ctrller;
    ioreg_t ioreg;

    _ENTER();

    if (!card) {
        _ERROR("Bad Card\n");
        return -1;
    }
    ctrller = card->ctrlr;

    /* SDIO Spec: Command 52 needs 
     * R/W Flag if 0 this will be read data if 1 write
     * Function number: Number of the function within I/O 
     * Register Address: This is the address of the byte of data inside of 
     * the write data: This bit is set to 0 for read
     * CRC7: 7 bits CRC
     */

    ioreg.read_or_write = SDIO_WRITE;
    ioreg.function_num = func;
    ioreg.reg_addr = reg;
    ioreg.dat = dat;     /** Will be filled by the card */

    ret = check_for_valid_ioreg(ctrller, &ioreg);

    if (ret < 0) {
        _ERROR("Wrong parameters for rw_ioreg\n");
        goto exit;
    }

    ret = rw_ioreg(ctrller, &ioreg);

    if (ret < 0) {
        _ERROR("sdio_write_ioreg failed, fn=%d, reg=%#x\n", func, reg);
    }

  exit:
    _LEAVE();
    return ret;
}

/* 
	This function sets up the DMA channel for the DMA and reads the 
	specified amount of data (blocks) from the SDIO card to host

	\param pointer to sdio controller structure and number of bytes to be 
	read
	\return returns byte count to be read or negative on error
*/

static int
trigger_dma_read(sdio_ctrller * ctrller, int byte_cnt)
{
    struct mmc_data *data = &sdio_dma_data;
    ssize_t ret = -EIO;
#ifdef MXC_MMC_DMA_ENABLE
    dma_request_t sdma_request;
    dma_channel_params params;
    dma_addr_t sdma_handle;
#else
    unsigned long *buf;
    int bytecnt = byte_cnt;
#endif
    unsigned int status;
    int i = 0;

    _ENTER();

//      disable_irq( ctrller->irq_line );
    if (!byte_cnt) {
        _ERROR("Count value is zero erring out trigger_dma_read\n");
        goto error;
    }

    set_state(ctrller, SDIO_FSM_BUFFER_IN_TRANSIT);

    _DBG("trigger_dma_read: CNTR=%lx\n", READL(ctrller->base + MMC_INT_CNTR));

#ifdef MXC_MMC_DMA_ENABLE

    memset(&sdma_request, 0, sizeof(dma_request_t));
    memset(&params, 0, sizeof(dma_channel_params));

    ctrller->data = data;

    ctrller->dma_size = byte_cnt;

    ctrller->dma_dir = DMA_FROM_DEVICE;

    /* * Setup DMA Channel parameters */
    params.transfer_type = per_2_emi;

    params.watermark_level = (ctrller->bus_width == 0x04) ? 64 : 16;
    params.peripheral_type = MMC;
    params.per_address = ctrller->res->start + MMC_BUFFER_ACCESS;
#ifdef MOTO_PLATFORM
    params.event_id = DMA_REQ_SDHC2;    //ctrller->event_id;
#else
    params.event_id = DMA_REQ_SDHC1;    //ctrller->event_id;
#endif
    params.bd_number = 1 /* host->dma_size / 256 */ ;
    params.word_size = TRANSFER_32BIT;
    params.callback = sdio_dma_irq;
    params.arg = ctrller;

    if (mxc_dma_setup_channel(ctrller->dma, &params)) {
        _ERROR("%s: mxc_dma_setup_channel ERR\n", __FUNCTION__);
        goto error;
    }

    sdma_handle = dma_map_single(mmc_dev(ctrller->mmc),
                                 (char *) (data->sg[0].dma_address), byte_cnt,
                                 DMA_FROM_DEVICE);

    sdma_request.destAddr = (__u8 *) sdma_handle;
    sdma_request.count = byte_cnt;

    if (mxc_dma_set_config(ctrller->dma, &sdma_request, 0)) {
        _ERROR("%s: mxc_dma_set_config ERR\n", __FUNCTION__);
        goto error;
    }

    init_completion(&ctrller->completion);

    if (mxc_dma_start(ctrller->dma)) {
        _ERROR("%s: mxc_dma_start ERR\n", __FUNCTION__);
        goto error;
    }

    wait_for_completion(&ctrller->completion);

    dma_unmap_single(mmc_dev(ctrller->mmc), (dma_addr_t) sdma_handle,
                     byte_cnt, DMA_TO_DEVICE);

    status = READL(ctrller->base + MMC_STATUS);
    while (!(status & STATUS_READ_OP_DONE)) {
        udelay(10);
        status = READL(ctrller->base + MMC_STATUS);
        i++;
        if (i > 100) {
            _ERROR("%s: over 100\n", __FUNCTION__);
            break;
        }
    }
    WRITEL(STATUS_READ_OP_DONE, ctrller->base + MMC_STATUS);

    ret = check_for_errors(ctrller, status);
    if (ret < 0) {
        _ERROR("%s: DMA read ERR\n", __FUNCTION__);
        goto error;
    }
#else

    buf = (unsigned long *) (sg_dma_address(&data->sg[0]));
    _DBG("A:\n");

    while (bytecnt > 0) {       // Dumb polling for now!
        status = READL(ctrller->base + MMC_STATUS);

        _DBG("B:status=%x bytecnt=%d byte_cnt=%d\n", status, bytecnt,
             byte_cnt);

        if (((status & (STATUS_YBUF_FULL | STATUS_XBUF_FULL)) != 0) &&
            ((status & STATUS_BUF_READ_RDY) != 0)) {
            {
                int fifo_size = 16;

                if (ctrller->bus_width == 0x01)
                    fifo_size = 4;
                if (ctrller->bus_width == 0x04)
                    fifo_size = 16;

                for (i = fifo_size; i > 0; i--) {
                    *buf++ = READL(ctrller->base + MMC_BUFFER_ACCESS);
                    bytecnt -= 4;
                }
            }
        }
    }
    _DBG("C:\n");

    status = READL(ctrller->base + MMC_STATUS);
    while (!(status & STATUS_READ_OP_DONE)) {
        udelay(10);
        status = READL(ctrller->base + MMC_STATUS);
    }
    WRITEL(STATUS_READ_OP_DONE, ctrller->base + MMC_STATUS);
    _DBG("D:\n");

#endif
    ret = byte_cnt;
  error:
//      xchg(&ctrller->busy, 0);
//      enable_irq( ctrller->irq_line );
    _LEAVE();
    return ret;
}

/** 
	This function sets up the DMA channel for the DMA and writes the 
	specified amount of data (blocks) from the hodst to SDIO card 
	
	\param pointer to sdio controller structure and number of bytes to be 
	read
	\return returns byte count to be read or negative on error
*/

int
trigger_dma_write(sdio_ctrller * ctrller, int cnt)
{
    struct mmc_data *data = &sdio_dma_data;
    ssize_t ret = -EIO;
#ifdef MXC_MMC_DMA_ENABLE
    dma_request_t sdma_request;
    dma_channel_params params;
    dma_addr_t sdma_handle;
#else
    unsigned long *buf;
    int bytecnt = cnt;
    int fifo_size = 16;
#endif
    unsigned int status;
    int i = 0;

    _ENTER();

//      disable_irq( ctrller->irq_line );
    if (!cnt) {
        _ERROR("Count value is zero erring out trigger_dma_write\n");
        goto error;
    }

    set_state(ctrller, SDIO_FSM_BUFFER_IN_TRANSIT);

#ifdef MXC_MMC_DMA_ENABLE

    /* DMA transfer setup */
    ctrller->data = data;

    ctrller->dma_size = cnt;

    _DBG("%s:Request bytes to transfer:%d\n", DRIVER_NAME, ctrller->dma_size);

    ctrller->dma_dir = DMA_TO_DEVICE;

    sdma_handle =
        dma_map_single(mmc_dev(ctrller->mmc),
                       (char *) (data->sg[0].dma_address), cnt,
                       DMA_TO_DEVICE);
    sdma_request.sourceAddr = (__u8 *) sdma_handle;

    /* * Setup DMA Channel parameters */
    params.transfer_type = emi_2_per;
    params.watermark_level = (ctrller->bus_width == 0x04) ? 64 : 16;
    params.peripheral_type = MMC;
    params.per_address = ctrller->res->start + MMC_BUFFER_ACCESS;
#ifdef MOTO_PLATFORM
    params.event_id = DMA_REQ_SDHC2;    //ctrller->event_id;
#else
    params.event_id = DMA_REQ_SDHC1;    //ctrller->event_id;
#endif
    params.bd_number = 1 /* host->dma_size / 256 */ ;
    params.word_size = TRANSFER_32BIT;
    params.callback = sdio_dma_irq;
    params.arg = ctrller;

    if (mxc_dma_setup_channel(ctrller->dma, &params)) {
        _ERROR("%s: mxc_dma_setup_channel ERR\n", __FUNCTION__);
        goto error;
    }

    sdma_request.count = ctrller->dma_size;

    if (mxc_dma_set_config(ctrller->dma, &sdma_request, 0)) {
        _ERROR("%s: mxc_dma_set_config ERR\n", __FUNCTION__);
        goto error;
    }

    disable_irq(ctrller->irq_line);
    ret = send_command_poll(ctrller, MMC_R5, 0);
    enable_irq(ctrller->irq_line);
    if (ret < 0) {
        _ERROR("%s: send_command_poll ERR\n", __FUNCTION__);
        goto error;
    }

    init_completion(&ctrller->completion);

    if (mxc_dma_start(ctrller->dma)) {
        _ERROR("%s: mxc_dma_start ERR\n", __FUNCTION__);
        goto error;
    }

    wait_for_completion(&ctrller->completion);

    dma_unmap_single(mmc_dev(ctrller->mmc), (dma_addr_t) sdma_handle, cnt,
                     DMA_TO_DEVICE);

    status = READL(ctrller->base + MMC_STATUS);
    while (!(status & STATUS_WRITE_OP_DONE)) {
        udelay(10);
        status = READL(ctrller->base + MMC_STATUS);
        if (i > 100) {
            _ERROR("%s: over 100\n", __FUNCTION__);
            break;
        }
    }
    WRITEL(STATUS_WRITE_OP_DONE, ctrller->base + MMC_STATUS);

    ret = check_for_errors(ctrller, status);
    if (ret < 0) {
        _ERROR("%s: DMA write ERR\n", __FUNCTION__);
        goto error;
    }
#else

    send_command_poll(ctrller, MMC_R5, 0);

    buf = (unsigned long *) (sg_dma_address(&data->sg[0]));
    _DBG("buf=0x%p  cnt=%d\n", buf, cnt);

    if (ctrller->bus_width == 0x01)
        fifo_size = 4;
    if (ctrller->bus_width == 0x04)
        fifo_size = 16;

    while (bytecnt > 0) {
        status = READL(ctrller->base + MMC_STATUS);
        if (((status & (STATUS_XBUF_EMPTY | STATUS_YBUF_EMPTY)) != 0) &&
            ((status & STATUS_BUF_WRITE_RDY) != 0)) {
            for (i = fifo_size; i > 0; i--) {
                WRITEL(*buf++, ctrller->base + MMC_BUFFER_ACCESS);
                bytecnt -= 4;
            }
        }
    }

    status = READL(ctrller->base + MMC_STATUS);
    while (!(status & STATUS_WRITE_OP_DONE)) {
        udelay(10);
        status = READL(ctrller->base + MMC_STATUS);
    }
    WRITEL(STATUS_WRITE_OP_DONE, ctrller->base + MMC_STATUS);

#endif
//      xchg(&ctrller->busy, 0);        
    ret = cnt;

  error:
//      enable_irq( ctrller->irq_line );
    _LEAVE();
    return ret;
}

/** This function is used to read / write block of data from the card
 */

static int
write_blksz(sdio_ctrller * ctrller, iorw_extended_t * io_rw)
{
    int ret;
    ioreg_t ioreg;

    _ENTER();
    if (io_rw->blkmode && ctrller->card_capability.
        fnblksz[io_rw->func_num] != io_rw->blk_size) {
        /* Write low byte */
        _DBG("looks like a odd size printing the values"
             "blkmode=0x%x blksz=%d fnblksz =%d\n",
             io_rw->blkmode, io_rw->blk_size,
             ctrller->card_capability.fnblksz[io_rw->func_num]);

        ioreg.read_or_write = SDIO_WRITE;
        ioreg.function_num = FN0;
        ioreg.reg_addr = FN_BLOCK_SIZE_0_REG(io_rw->func_num);
        ioreg.dat = (u8) (io_rw->blk_size & 0x00ff);
        if (rw_ioreg(ctrller, &ioreg) < 0) {
            _ERROR("rw_ioreg failed rw_iomem\n");
            ret = -ENXIO;
            goto err_down;
        }
        /* Write high byte */
        ioreg.read_or_write = SDIO_WRITE;
        ioreg.function_num = FN0;
        ioreg.reg_addr = FN_BLOCK_SIZE_1_REG(io_rw->func_num);
        ioreg.dat = (u8) (io_rw->blk_size >> 8);
        if (rw_ioreg(ctrller, &ioreg) < 0) {
            _ERROR("rw_ioreg failed rw_iomem 1\n");
            ret = -ENXIO;
            goto err_down;
        }

        ctrller->card_capability.fnblksz[io_rw->func_num] = io_rw->blk_size;
    }
    _LEAVE();
    return 0;

  err_down:
    _ERROR("rw_iomem failed\n");
    _LEAVE();
    return -1;
}

int
complete_io(sdio_ctrller * ctrller, int rw_flag)
{
    int ret = MMC_ERROR_GENERIC;

    _ENTER();

    if (set_state(ctrller, SDIO_FSM_IDLE)) {
        _ERROR("<1>__set_state failed -complete_io\n");
        goto error;
    }
    ret = 0;
  error:
    _LEAVE();
    return ret;
}

/** 
	This function fills up the CMD53
	\param pointer to the controller structure and parameters for the CMD53
	\return returns 0 on success or a negative value on error
*/

int
cmd53_handle(sdio_ctrller * ctrller, iorw_extended_t * io_rw)
{
    u32 cmdat_temp;
    int ret = -ENODEV, ret1 = 0;
    u16 argh = 0UL, argl = 0UL;
    ssize_t bytecnt;
    /* CMD53 */

    _ENTER();

    /* SDIO Spec: 
       R/W flag (1) 
       Function Number (3)
       Block Mode (1)
       OP Code (1) (Multi byte read / write from fixed location or 
       from the fixed location      
       Register Address (17) 
       Byte Count (9)
       Command and the CRC will be taken care by the controller, so 
       fill up (48-16) bits
     */
    _DBG("C53:%c\n", (io_rw->rw_flag == 0) ? 'R' : 'W');

#ifdef DEBUG_USEC
    _DBG("C53 Time=%lu\n", get_usecond());
#endif
    sdio_stop_clock(ctrller);

    argh =
        (io_rw->rw_flag << 15) |
        (io_rw->func_num << 12) |
        (io_rw->blkmode << 11) |
        (io_rw->op_code << 10) | ((io_rw->reg_addr & 0x0001ff80) >> 7);

    argl = ((io_rw->reg_addr & 0x0000007f) << 9) | (io_rw->byte_cnt & 0x1ff);

    WRITEL(CMD(53), ctrller->base + MMC_CMD);
    WRITEL(argh << 16 | argl, ctrller->base + MMC_ARG);

    cmdat_temp =
        CMD_DAT_CONT_RESPONSE_FORMAT_R1 | CMD_DAT_CONT_DATA_ENABLE |
        (io_rw->rw_flag ? CMD_DAT_CONT_WRITE : 0);

    cmdat_temp |= CMD_DAT_CONT_INIT;

    if (ctrller->bus_width == 0x04)
        cmdat_temp |= CMD_DAT_CONT_BUS_WIDTH_4;

    else if (ctrller->bus_width == 0x01)
        cmdat_temp |= CMD_DAT_CONT_BUS_WIDTH_1;

    else
        cmdat_temp |= CMD_DAT_CONT_BUS_WIDTH_4;

    if (io_rw->blkmode) {
        WRITEL(io_rw->blk_size, ctrller->base + MMC_BLK_LEN);
        WRITEL(io_rw->byte_cnt, ctrller->base + MMC_NOB);
    } else {
        WRITEL(io_rw->byte_cnt, ctrller->base + MMC_BLK_LEN);
        WRITEL(1, ctrller->base + MMC_NOB);
    }

    // cmdat_temp |= MMC_CMDAT_DMA_EN;
    wmb();

    WRITEL(cmdat_temp, ctrller->base + MMC_CMD_DAT_CONT);

    ctrller->num_ofcmd53++;

    _DBG("CMD53(0x%04x%04x)\n", argh, argl);

    if (io_rw->rw_flag == 0) {  //53 read
        disable_irq(ctrller->irq_line);
        ret = send_command_poll(ctrller, MMC_R5, 0);
        enable_irq(ctrller->irq_line);

        if (ret < 0) {
            _ERROR("53 read cmd fail\n");
            ret1 = ret;
            goto error;
        }
    }

    _DBG("io_rw->blk_size=0x%08x.  io_rw->byte_cnt=%d\n", io_rw->blk_size,
         io_rw->byte_cnt);

    if (io_rw->blkmode)
        bytecnt = io_rw->blk_size * io_rw->byte_cnt;
    else
        bytecnt = io_rw->byte_cnt * 1;

    _DBG("bytecnt=%d\n", bytecnt);

    disable_irq(ctrller->irq_line);
    WRITEL(0, ctrller->base + MMC_INT_CNTR);
    enable_irq(ctrller->irq_line);

    sdio_dma_data.blocks = io_rw->byte_cnt;
    sdio_dma_sg.dma_address = (dma_addr_t) io_rw->buf;
    sdio_dma_sg.length = io_rw->blk_size * io_rw->byte_cnt;
    sdio_dma_data.sg = &sdio_dma_sg;
    sdio_dma_data.sg_len = 1;

    /* Start transferring data on DAT line */
    _DBG("Byte count = %d\n", bytecnt);
    while (bytecnt > 0) {
        if (io_rw->rw_flag == 0) {
            /* READ */
            _DBG("read_buffer of %d\n", bytecnt);
#ifdef DEBUG_USEC
            {
                ulong t = get_usecond();
                _DBG("[1] READ: %lu\n", t = get_usecond());
#endif

                _DBG("C53R\n");
                if ((ret = trigger_dma_read(ctrller, bytecnt)) <= 0) {
                    _ERROR("<1>HWAC Read buffer error\n");
                    ret1 = MMC_ERROR_GENERIC;
                    goto error;
                }
#ifdef DEBUG_USEC
                _DBG("[2] READ: %lu\n", get_usecond() - t);
                rbuf[rcnt++] = get_usecond() - t;
                rcnt %= 100;
            }
#endif

        } else {
            /* WRITE */
#ifdef DEBUG_USEC
            {
                ulong t = get_usecond();
                _DBG("[1] WRT: %lu\n", t = get_usecond());
                _DBG("C53W\n");
#endif
                if ((ret = trigger_dma_write(ctrller, bytecnt)) <= 0) {
                    _ERROR("HWAC Write buffer error\n");
                    ret1 = MMC_ERROR_GENERIC;
                    goto error;
                }
#ifdef DEBUG_USEC
                _DBG("[2] WRT: %lu\n", get_usecond() - t);
                wbuf[wcnt++] = get_usecond() - t;
                wcnt %= 100;
            }
#endif

        }
        io_rw->buf += ret;
        bytecnt -= ret;
        _DBG("bytecnt=%d\n", bytecnt);
    }
    if (set_state(ctrller, SDIO_FSM_END_IO)) {
        _ERROR("<1>set_state failed rw_iomem\n");
        goto error;
    }
    if ((ret = complete_io(ctrller, io_rw->rw_flag))) {
        _ERROR("<1>complete_io failed rw_iomem\n");
        goto error;
    }
    _DBG("C53E\n");
    _LEAVE();
    return 0;

  error:
    if (set_state(ctrller, SDIO_FSM_END_IO)) {
        _ERROR("<1>set_state failed rw_iomem\n");
    }

    if ((ret = complete_io(ctrller, io_rw->rw_flag))) {
        _ERROR("complete_io failed in cmd53_handle\n");
    }
    _LEAVE();
    return ret1;
}

/** This function checks the upper bound for the CMD53 read or write
 */

static int
check_iomem_args(sdio_ctrller * ctrller, iorw_extended_t * io_rw)
{
    int ret = 0;
    _ENTER();

    if (!ctrller) {
        _ERROR("Null ctrller\n");
        _LEAVE();
        return -ENODEV;
    }

    /* Check function number range */
    if (io_rw->func_num > ctrller->card_capability.num_of_io_funcs) {
        ret = -ERANGE;
    }

    /* Check register range */
    if ((io_rw->reg_addr & 0xfffe0000) != 0x0) {
        ret = -ERANGE;
    }

    /* Check cnt range */
    if (io_rw->byte_cnt == 0 || (io_rw->byte_cnt & 0xfffffe00) != 0x0) {
        ret = -ERANGE;
    }

    /* Check blksz range */
    if (io_rw->blkmode && (io_rw->blk_size == 0 || io_rw->blk_size > 0x0800)) {
        ret = -ERANGE;
    }

    /* Check null-pointer */
    if (io_rw->buf == 0x0) {
        ret = -EINVAL;
    }

    _LEAVE();
    return ret;
}

/** 
	This function is a hook to the wlan driver to read a block of data
	(CMD53)

	\param pointer to card structure, function number, register address, 
	block mode, block size count and pointer to the buffer
	\return returns 0 on success negative value on error
*/

int
sdio_read_iomem(mmc_card_t card, u8 func, u32 reg, u8 blockmode,
                u8 opcode, ssize_t cnt, ssize_t blksz, u8 * dat)
{

    int ret;
#ifdef DEBUG_USEC
    ulong time;
#endif
    sdio_ctrller *ctrller = card->ctrlr;
    iorw_extended_t io_rw;

    _ENTER();

    io_rw.rw_flag = IOMEM_READ;
    io_rw.func_num = func;
    io_rw.reg_addr = reg;
    io_rw.blkmode = blockmode;
    io_rw.op_code = opcode;
    io_rw.byte_cnt = cnt;
    io_rw.blk_size = blksz;
    io_rw.buf = dat;
    {
        ssize_t _blksz = blksz;
        sdio_dma_data.blksz_bits = -1;
        for (; _blksz; sdio_dma_data.blksz_bits++, _blksz = _blksz >> 1);
    }

    sdio_dma_data.blocks = cnt;
    sdio_dma_sg.dma_address = (dma_addr_t) dat;
    sdio_dma_sg.length = blksz * cnt;
    sdio_dma_data.sg = &sdio_dma_sg;
    sdio_dma_data.sg_len = 1;

    _DBG("sdio_read_iomem blksize = %d count = %d jiffies = %lu\n",
         blksz, cnt, jiffies);
    _DBG("Values are io_rw.rw_flag = %x io_rw.func_num =%x"
         "io_rw.reg_addr=%x io_rw.blkmode =%x io_rw.op_code =%x"
         "io_rw.byte_cnt =%xio_rw.blk_size =%x io_rw.buf =%p\n",
         io_rw.rw_flag, io_rw.func_num, io_rw.reg_addr,
         io_rw.blkmode, io_rw.op_code, io_rw.byte_cnt,
         io_rw.blk_size, io_rw.buf);

#ifdef DEBUG_USEC
    time = get_usecond();
    _DBG("sdio_read_iomem: time in = %lu\n", time);
#endif
    ret = check_iomem_args(ctrller, &io_rw);

    if (ret < 0) {
        _ERROR("Wrong parameters passed to sdio_write_iomem\n");
        goto exit;
    }

    acquire_io(ctrller);

    /* Perform the actual CMD53 Write now */
    ret = cmd53_handle(ctrller, &io_rw);

    release_io(ctrller);

    if (ret < 0)
        _ERROR("rw_iomem error CMD53 write fails\n");

    _DBG("leave sdio_read_iomem jiffies = %lu\n", jiffies);

  exit:
#ifdef DEBUG_USEC
    time = get_usecond();
    _DBG("sdio_read_iomem: time out = %lu\n", time);
#endif
    _LEAVE();
    return ret;
}

/** 
	This function is a hook to the WLAN driver to write a block of data
	on SDIO card

	\param pointer to card structure, function number, register address, 
	type of	transfer, incrimental / fixed address, block size and pointer 
	to the buffer to be written
*/

int
sdio_write_iomem(mmc_card_t card, u8 func, u32 reg, u8 blockmode,
                 u8 opcode, ssize_t cnt, ssize_t blksz, u8 * dat)
{
    int ret;

#ifdef DEBUG_USEC
    ulong time;
#endif
    sdio_ctrller *ctrller = card->ctrlr;
    iorw_extended_t io_rw;

    _ENTER();

    io_rw.rw_flag = IOMEM_WRITE;
    io_rw.func_num = func;
    io_rw.reg_addr = reg;
    io_rw.blkmode = blockmode;
    io_rw.op_code = opcode;
    io_rw.byte_cnt = cnt;
    io_rw.blk_size = blksz;
    io_rw.buf = dat;

    {
        ssize_t _blksz = blksz;
        sdio_dma_data.blksz_bits = -1;
        for (; _blksz; sdio_dma_data.blksz_bits++, _blksz = _blksz >> 1);
    }

    sdio_dma_data.blocks = cnt;
    // sdio_dma_sg.dma_address = (dma_addr_t)dat;
    sdio_dma_sg.length = blksz * cnt;
    sdio_dma_sg.page = virt_to_page(dat);
    sdio_dma_sg.offset = offset_in_page(dat);
    sdio_dma_data.sg = &sdio_dma_sg;
    sdio_dma_data.sg_len = 1;

    _DBG("sdio_write_iomem\n");
    _DBG("CMD 53 write values rw_flag = %x func_num = %x"
         "reg_addr = %x, blkmode = %x opcode = %x count = %x"
         "blk size = %x buf_addr = %p", io_rw.rw_flag,
         io_rw.func_num, io_rw.reg_addr, io_rw.blkmode,
         io_rw.op_code, io_rw.byte_cnt, io_rw.blk_size, io_rw.buf);

#ifdef DEBUG_USEC
    time = get_usecond();
    _DBG("sdio_write_iomem: time in = %lu\n", time);
#endif
    ret = check_iomem_args(ctrller, &io_rw);

    if (ret < 0) {
        _ERROR("Wrong parameters passed to sdio_write_iomem\n");
        goto exit;
    }
    ret = write_blksz(ctrller, &io_rw);

    if (ret < 0) {
        _ERROR("rw_iomem error CMD53 write fails\n");
        goto exit;
    }

    /* Perform the actual CMD53 Write now */

    acquire_io(ctrller);
    ret = cmd53_handle(ctrller, &io_rw);

    release_io(ctrller);
    if (ret < 0)
        _ERROR("rw_iomem error CMD53 write fails\n");

    _DBG("leave: sdio_write_iomem jiffies = %lu\n", jiffies);

  exit:
#ifdef DEBUG_USEC
    time = get_usecond();
    _DBG("sdio_write_iomem: time out = %lu\n", time);
#endif
    _LEAVE();
    return ret;
}

/** 
	This function is a hook to the WLAN driver to request the interrupts
	(for Card interrupt)on SDIO card
*/

int
sdio_request_irq(mmc_card_t card,
                 IRQ_RET_TYPE(*handler) (int, void *, struct pt_regs *),
                 unsigned long irq_flags, const char *devname, void *dev_id)
{
    int ret, func;
    u8 io_enable_reg, int_enable_reg;
    sdio_ctrller *ctrller;

    _ENTER();

    if (!card) {
        _ERROR("Passed a Null card exiting out\n");
        ret = -ENODEV;
        goto done;
    }
    ctrller = card->ctrlr;

    net_devid = dev_id;
    mmc_dev(ctrller->mmc) = dev_id;

    sdioint_handler = handler;

    _DBG("address of dev_id = %p sdioint_handler =%p\n",
         net_devid, sdioint_handler);
    if (!ctrller->card_int_ready)
        ctrller->card_int_ready = YES;

    io_enable_reg = 0x0;

    int_enable_reg = IENM;

    for (func = 1; func <= 7 && func <=
         ctrller->card_capability.num_of_io_funcs; func++) {

        io_enable_reg |= IOE(func);
        int_enable_reg |= IEN(func);
    }

        /** Enable function IOs on card */
    sdio_write_ioreg(card, FN0, IO_ENABLE_REG, io_enable_reg);
        /** Enable function interrupts on card */
    sdio_write_ioreg(card, FN0, INT_ENABLE_REG, int_enable_reg);

//      WRITEL(INT_CNTR_SDIO_IRQ_EN | INT_CNTR_SDIO_INT_WKP_EN,  ctrller->base + MMC_INT_CNTR);
    WRITEL(INT_CNTR_SDIO_IRQ_EN, ctrller->base + MMC_INT_CNTR);

    wmb();

    ret = 0;

  done:
    _LEAVE();
    return ret;
}

/** 
	This function is a hook to the WLAN driver to release the interrupts
	requested for
*/

int
sdio_free_irq(mmc_card_t card, void *dev_id)
{
    int ret;
    sdio_ctrller *ctrller = NULL;

    _ENTER();

    dev_id = net_devid;
    net_devid = 0;

    if (!card) {
        ret = -ENODEV;
        goto done;
    }
    ctrller = card->ctrlr;

        /** Disable function IOs on the card */
    sdio_write_ioreg(card, FN0, IO_ENABLE_REG, 0x0);

        /** Disable function interrupts on the function */
    sdio_write_ioreg(card, FN0, INT_ENABLE_REG, 0x0);

        /** Release IRQ from OS */

    WRITEL(0, ctrller->base + MMC_INT_CNTR);
    sdioint_handler = NULL;

  done:
    _LEAVE();
    return 0;
}

/** 
	This function uses CMD52 to read CIS memory
*/

int
sdio_read_cis_ioreg(mmc_card_t card, u8 func, u32 reg, u8 * dat)
{
    int ret = 0;
    u8 tmp = 0;

    ret = sdio_read_ioreg(card, func, reg, dat);
    mdelay(10);
    if (ret < 0)
        return ret;

    ret = sdio_read_ioreg(card, func, reg & 0xff, &tmp);
    mdelay(10);
    return ret;
}

/** 
	This function reads the Manufacturing ID and the Vendor ID

	\param pointer to controller structure and function numer
*/

static int
read_manfid(sdio_ctrller * ctrller, int func)
{
    int offset = 0;
    u32 manfid, card_id;
    int ret = 0;
    u8 tuple, link, datah, datal;

    _ENTER();
    do {
        ret = sdio_read_cis_ioreg(ctrller->card, func,
                                  ctrller->card_capability.cisptr[func]
                                  + offset, &tuple);

        if (ret < 0)
            return ret;

        if (tuple == CISTPL_MANFID) {
            offset += 2;

            ret = sdio_read_cis_ioreg(ctrller->card, func,
                                      ctrller->card_capability.cisptr[func]
                                      + offset, &datal);
            if (ret < 0)
                return ret;

            offset++;

            ret = sdio_read_cis_ioreg(ctrller->card, func,
                                      ctrller->card_capability.cisptr[func]
                                      + offset, &datah);
            if (ret < 0)
                return ret;

            manfid = datal | datah << 8;
            ctrller->manf_id = manfid;

            offset++;

            ret = sdio_read_cis_ioreg(ctrller->card, func,
                                      ctrller->card_capability.cisptr[func]
                                      + offset, &datal);
            if (ret < 0)
                return ret;

            offset++;

            ret = sdio_read_cis_ioreg(ctrller->card, func,
                                      ctrller->card_capability.cisptr[func]
                                      + offset, &datah);
            if (ret < 0)
                return ret;

            card_id = datal | datah << 8;

            ctrller->dev_id = card_id;

            _DBG("Card id = 0%2x manfid = %x\n", card_id, manfid);

            return manfid;
        }

        ret = sdio_read_cis_ioreg(ctrller->card, func,
                                  ctrller->card_capability.cisptr[func] +
                                  offset + 1, &link);
        if (ret < 0)
            return ret;

        offset += link + 2;

    } while (tuple != CISTPL_END);

    _LEAVE();
    return -EINVAL;
}

int
sdio_get_vendor_id(mmc_card_t card)
{
    int ret;

    _ENTER();
    ret = card->ctrlr->manf_id;
    _LEAVE();

    return ret;
}

/* Dummy holders for driver integration */

int
sdio_clear_imask(sdio_ctrller * ctrller)
{
    _ENTER();
    _LEAVE();
    return 0;
}

void
sdio_enable_SDIO_INT(void)
{
    _ENTER();
    _LEAVE();
    return;
}

/*============================================================================*/

/** 
	This function checks for the card capability, like if the memory 
 	is present , Number of I/O functions etc. This is the response interpret
  	of CMD5 
	
	\param controller pointer to sdio_ctrller structure
	\returns 0 on success else negative value is returned
*/

static int
check_for_card_capability(sdio_ctrller * ctrller)
{
    card_capability capability;

    _ENTER();

    /* SDIO Spec: bit 9-12 represents Number Of I/O Functions
     * 12-13 Memory present
     * 16-40 OCR 
     * If the I/O functions < 1 error
     */

    /* OCR */
    capability.ocr = 0x0;
    capability.ocr |= (MMC_RESPONSE(ctrller, 3) << 16);
    _DBG("%x\n", capability.ocr);
    capability.ocr |= (MMC_RESPONSE(ctrller, 2) << 8);
    _DBG("%x\n", capability.ocr);
    capability.ocr |= (MMC_RESPONSE(ctrller, 1) << 0);
    _DBG("%x\n", capability.ocr);

    ctrller->card_capability.ocr = capability.ocr;

    /* NUM OF IO FUNCS */
    capability.num_of_io_funcs = ((MMC_RESPONSE(ctrller, 4)
                                   & 0x70) >> 4);

    ctrller->card_capability.num_of_io_funcs = capability.num_of_io_funcs;

    _DBG("IO funcs %x\n", capability.num_of_io_funcs);

    if (capability.num_of_io_funcs < 1) {
        _ERROR("Card Doesnot support the I/O\n");
        return -1;
    }

    /* MEMORY? */
    capability.memory_yes = ((MMC_RESPONSE(ctrller, 4)
                              & 0x08) ? 1 : 0);
    _DBG("Memory present  %x\n", capability.memory_yes);
    ctrller->card_capability.memory_yes = capability.memory_yes;

    _LEAVE();
    return 0;
}

static int
send_iosend_op_cmd(sdio_ctrller * ctrller)
{
    int ret = -1;
    u16 argl, argh;

    _ENTER();

    /* After Reset MMC Card must be initialized by sending 
     * 80 Clocks on MMCLK signal.
     */

    WRITEL(sdioclock, ctrller->base + MMC_CLK_RATE);

    WRITEL(0xff, ctrller->base + MMC_RES_TO);

    sdio_stop_clock(ctrller);

    /* -SDIO Spec : CMD5 is used to inquire about the voltage range
     * needed by the I/O card. The normal response to this command is
     * R4 . The host determines the card's configuration based on the
     * data contained in the R4
     *      ------------------------------------------------------------
     *      | S|D|RESERV|C|I/O|M|STB|I/O..................OCR|RESERVE|E|    
     *      ------------------------------------------------------------
     *      Refer to SD Spec.
     */

    argh = argl = 0x0;
    WRITEL(CMD(5), ctrller->base + MMC_CMD);
    WRITEL(argh << 16 | argl, ctrller->base + MMC_ARG);

    wmb();

    WRITEL(CMD_DAT_CONT_INIT | MMC_CMDAT_R1,
           ctrller->base + MMC_CMD_DAT_CONT);

    _DBG("SDIO init 1  CMD5(0x%04x%04x)\n", argh, argl);

    ret = send_command(ctrller, MMC_R4, 0);

    if (!ret) {
        /* Fill up the High byte to argh from the reponse 
         *  recieved from the CMD5
         */
        argh = (MMC_RESPONSE(ctrller, 4) << 8) | MMC_RESPONSE(ctrller, 3);
        argh &= 0x00ff;

        /* Fill up the low byte to argl from the reponse 
         *  recieved from the CMD5
         */
        argl = (MMC_RESPONSE(ctrller, 2) << 8) | MMC_RESPONSE(ctrller, 1);
    }

    if (!argh && !argl) {
        /* If the value of argh and argl are NULL set the OCR to
         *  max value
         */

        /* Set to full limit */

        argh = 0x00ff;
        argl = 0xff00;
        _DBG("Setting the card to full voltage limit\n");
    }

    /* SDIO Spec: Send the CMD5 again to set the OCR values */

    _DBG("SDIO init 2 stop_bus_clock \n");
    sdio_stop_clock(ctrller);

    WRITEL(CMD(5), ctrller->base + MMC_CMD);
    WRITEL(argh << 16 | argl, ctrller->base + MMC_ARG);
    WRITEL(MMC_CMDAT_R1, ctrller->base + MMC_CMD_DAT_CONT);

    _DBG("SDIO init 3  CMD5(0x%04x%04x)\n", argh, argl);
    ret = send_command(ctrller, MMC_R4, 0);
    ret = check_for_card_capability(ctrller);

    if (ret < 0) {
        _ERROR("Unable to get the OCR Value\n");
    }

    _LEAVE();
    return ret;
}

/** 
	This function sends the CMD7 to the card
	\param pointer to sdio_controller structure
	\returns 0 on success else negative value is returned
*/

static int
send_select_card_cmd(sdio_ctrller * ctrller)
{
    int ret;

    _ENTER();
    /* This function sends the CMD 7 and intrprets the
     * response to verify if the response is OK
     */

    /* Take the argh from the CMD3 response and pass it to 
     * CMD7
     */

    sdio_stop_clock(ctrller);

    WRITEL(CMD(7), ctrller->base + MMC_CMD);
    WRITEL(ctrller->card_capability.rca << 16 | 0x0, ctrller->base + MMC_ARG);
    WRITEL(MMC_CMDAT_R1, ctrller->base + MMC_CMD_DAT_CONT);

    _DBG("SDIO init 5  CMD7(0x%04x%04x)\n", ctrller->card_capability.rca, 0);

    ret = send_command(ctrller, MMC_R1, 0);

    _LEAVE();
    return ret;
}

/** 
	This function sends the CMD3 and gets the relative RCA

	\param pointer to sdio_controller structure
	\returns 0 on success else negative value given by the OS is returned
*/

static int
send_relative_addr_cmd(sdio_ctrller * ctrller)
{
    int ret = -1;
    card_capability card_capb;

    _ENTER();

    /* This function sends the CMD 3 and intrprets the
     * response to verify if the response is OK
     */

    sdio_stop_clock(ctrller);

    WRITEL(CMD(3), ctrller->base + MMC_CMD);
    WRITEL(0x0, ctrller->base + MMC_ARG);
    WRITEL(MMC_CMDAT_R1, ctrller->base + MMC_CMD_DAT_CONT);

    _DBG("Command 3 ....\n");
    _DBG("SDIO init 4  CMD3(0x%04x%04x)\n", 0, 0);

    ret = send_command(ctrller, MMC_R6, 0);

    if (ret)
        _DBG("Problem in CMD 3\n");

    card_capb.rca = ((MMC_RESPONSE(ctrller, 4) << 8) |
                     MMC_RESPONSE(ctrller, 3));
    ctrller->card_capability.rca = card_capb.rca;

    _DBG("CMD 3 card capability = %x\n", card_capb.rca);

    _LEAVE();
    return ret;
}

/**
	This function gets the CIS adress present on the SDIO card

	\param pointer to controller structure
	\return returns 0 on success, negative value on error
*/

static int
get_cisptr_address(sdio_ctrller * ctrller)
{
    int ret = 0, fn;
    u8 dat;

    _ENTER();

    for (fn = 0; fn <= ctrller->card_capability.num_of_io_funcs; fn++) {

        ret = sdio_read_ioreg(ctrller->card, FN0,
                              FN_CIS_POINTER_0_REG(fn), &dat);
        if (ret < 0)
            break;

        ctrller->card_capability.cisptr[fn] = (((u32) dat) << 0);

        _DBG("dat = %x\n", dat);

        ret = sdio_read_ioreg(ctrller->card, FN0,
                              FN_CIS_POINTER_1_REG(fn), &dat);
        if (ret < 0)
            break;

        ctrller->card_capability.cisptr[fn] |= (((u32) dat) << 8);

        _DBG("dat = %x\n", dat);

        ret = sdio_read_ioreg(ctrller->card, FN0,
                              FN_CIS_POINTER_2_REG(fn), &dat);
        if (ret < 0)
            break;

        ctrller->card_capability.cisptr[fn] |= (((u32) dat) << 16);

        _DBG("dat = %x\n", dat);

        _DBG("Card CIS Addr = %x fn = %d\n",
             ctrller->card_capability.cisptr[fn], fn);
    }

    _LEAVE();
    return ret;
}

/** 
	This function is the hook to the wlan driver to set the bus width
 	either to 1 bit mode or to 4 bit mode

	\param pointer to the card structure and the mode (1bit / 4bit)
	\return returns 0 on success negative value on error
*/

int
sdio_set_buswidth(mmc_card_t card, int mode)
{
    _ENTER();
    if ((mode != 1) && (mode != 4))
        mode = 4;
    //ECSI bit should be turn on too.
    switch (mode) {
    case SDIO_BUSWIDTH_1_BIT:
        sdio_write_ioreg(card, FN0,
                         BUS_INTERFACE_CONTROL_REG,
                         ECSI_BIT | BUSWIDTH_1_BIT);

        card->ctrlr->bus_width = SDIO_BUSWIDTH_1_BIT;
        _DBG("\n Marvell SDIO: Bus width is " "set to 1 bit mode\n");
        break;

    case SDIO_BUSWIDTH_4_BIT:
        sdio_write_ioreg(card, FN0,
                         BUS_INTERFACE_CONTROL_REG,
                         ECSI_BIT | BUSWIDTH_4_BIT);

        card->ctrlr->bus_width = SDIO_BUSWIDTH_4_BIT;

        _DBG("\n Marvell SDIO: Bus width is " "set to 4 bit mode\n");

        break;
    default:
        _ERROR("Not supported Mode\n");
        break;
    }

    _LEAVE();
    return 0;
}

static void
card_setup(sdio_ctrller * ctrller)
{
    int ret;
    /*
     * 1. Initialize the card 
     * 2. Read the Vendor ID
     * 3. Read the supported functionaleties in the card
     * 4. Pass these parameters to the user space
     */

    _ENTER();

    ret = send_iosend_op_cmd(ctrller);

    if (ret < 0) {
        _ERROR("Unable to get the command 5 response\n");
        goto cleanup;
    }

    /*TODO SDIO Spec: Continuosly send CMD3 and CMD7 until card 
     * is put in to standby mode 
     */

    /* CMD 3 */
    ret = send_relative_addr_cmd(ctrller);

    if (ret < 0) {
        _ERROR("Unable to get the Response for CMD3\n");
        goto cleanup;
    }

    /* CMD 7 */
    ret = send_select_card_cmd(ctrller);

    if (ret < 0) {
        _ERROR("Unable to get the Response for CMD 7\n");
        goto cleanup;
    }

    /* Send the correct rca and set the proper state */

    // ctrller->saved_mmc_clkrt = 0x01;

    _DBG("Setting the card to 10Mhz\n");

    ret = send_select_card_cmd(ctrller);

    if (ret < 0) {
        _ERROR("Unable to get the Response for CMD 7\n");
        goto cleanup;
    }

    init_semaphores(ctrller);

    /* Now read the regesters CMD52 */

    /* Get the function block size
     * Get the CIS pointer base
     * Parse the tuple
     * Get the Vendor ID compare with Marvell ID
     * End of Ctrller initialization
     */

    /* Now the card is initialized. Set the clk to 10/20 Mhz */

    if (ret < 0) {
        _ERROR("get_ioblk_size failed\n");
        goto cleanup;
    }

    sdio_set_buswidth(ctrller->card, bus_mode);
    _DBG("<1> Bus_mode=%d\n", bus_mode);

    ret = get_cisptr_address(ctrller);

    if (ret < 0) {
        _ERROR("get_cisptr_address failed\n");
        goto cleanup;
    }

    ret = read_manfid(ctrller, FN0);

    if (ret < 0) {
        _ERROR("Unable to read the manf or vendor id\n");
        goto cleanup;
    }

    _DBG("Manf id   = %x\n", ctrller->manf_id);
    _DBG("Device id = %x\n", ctrller->dev_id);

    if (ctrller->manf_id == MRVL_MANFID) {
        _DBG("Found the MARVELL SDIO (0x%x) card\n", ret);
        _DBG("Marvell SDIO Driver insertion Successful\n");
    }

    /* Backup card capability info for upper driver to use */
    memcpy(&ctrller->card->info, &ctrller->card_capability,
           sizeof(card_capability));

    /* Stop the clock and put it to IDLE state */
    sdio_stop_clock(ctrller);

    set_state(ctrller, SDIO_FSM_IDLE);

#ifdef CONFIG_PROC_FS
    create_proc_entry_forsdio(ctrller);
#endif

  cleanup:

    _LEAVE();
    return;
}

static void
card_release(sdio_ctrller * ctrller)
{
    _ENTER();
    sdio_write_ioreg(ctrller->card, FN0, 0x06, 0x08);
    _LEAVE();
}

module_init(sdio_init);
module_exit(sdio_exit);

EXPORT_SYMBOL(sdio_read_ioreg);
EXPORT_SYMBOL(sdio_write_ioreg);
EXPORT_SYMBOL(sdio_read_iomem);
EXPORT_SYMBOL(sdio_write_iomem);
EXPORT_SYMBOL(sdio_request_irq);
EXPORT_SYMBOL(sdio_free_irq);
EXPORT_SYMBOL(start_bus_clock);
EXPORT_SYMBOL(stop_bus_clock_2);
EXPORT_SYMBOL(sdio_get_vendor_id);
EXPORT_SYMBOL(sdio_register);
EXPORT_SYMBOL(sdio_unregister);
EXPORT_SYMBOL(sdio_clear_imask);
EXPORT_SYMBOL(sdio_enable_SDIO_INT);
EXPORT_SYMBOL(sdio_set_buswidth);

MODULE_DESCRIPTION("SDIO Interface Driver");
MODULE_AUTHOR("Marvell Semiconductor");
MODULE_LICENSE("GPL");
