/*
 *  linux/drivers/mmc/imxmmc.c - Motorola i.MX MMCI driver
 *
 *  Copyright (C) 2004 Sascha Hauer, Pengutronix <sascha@saschahauer.de>
 *
 *  derived from pxamci.c by Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_mmc.c
 *
 * @brief Driver for the Freescale Semiconductor MXC SDHC modules.
 *
 * This driver code is based on imxmmc.c, by Sascha Hauer,
 * Pengutronix <sascha@saschahauer.de>. This driver supports both Secure Digital
 * Host Controller modules (SDHC1 and SDHC2) of MXC. SDHC is also referred as
 * MMC/SD controller. This code is not tested for SD cards.
 *
 * @ingroup MMC_SD
 */

/*
 * Copyright (C) 2006-2008 Motorola
 * Date         Author                  Comment
 * 11/23/2006   Motorola		Add megasim_restart_mmc for sysfs and ioctl interface.
 * 03/08/2007   Motorola		Add two interfaces for turning on/off sdhc clock.
 * 04/06/2007   Motorola                change power up sequence for 4G SD cards
 * 07/04/2007   Motorola                Add support for MC/MR
 * 07/12/2007   Motorola   		Add time out in stop clock
 * 09/07/2007	Motorola		Change mmc clock to 12MHz from 24MHz
 * 09/28/2007   Motorola		Recover SDHC error
 * 12/04/2007	Motorola		Modify power setting place
 * 01/11/2008   Motorola                Modify stop clock
 * 02/20/2008   Motorola                Modify plugin debounce checking with umount case
 * 04/28/2008   Motorola                Change sd clock rate back to 12Mhz from 16Hz for desense issue
 * 09/23/2008   Motorola                Add shredding support
 * 11/03/2008	Motorola		Modify start/stop clock loop jiff
 */


/*
 * Include Files
 */
#include <linux/config.h>
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
#include <linux/timer.h>
#include <linux/power_ic_kernel.h>
#include <linux/mpm.h>
#include <linux/sched.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>

#ifdef CONFIG_MOT_FEAT_GPIO_API_SDHC
#include <asm/mot-gpio.h>
#endif

#include <asm/arch/clock.h>

#include "mmc.h"
#include "mxc_mmc.h"

/*
 * We want to debounce card insertions.  So we need to sample the
 * card insert gpio as being stable for a count of 10 periods
 * of 50 mSec each before considering the card to be inserted.
 */
#define MXC_MMC_DEBOUNCE_HAPPY_COUNT 10
#define MXC_MMC_DEBOUNCE_POLL_DELAY_MSEC 50

/*
 * This define is used to test the driver without using SDMA
 */

/*!
 * Maxumum length of s/g list, only length of 1 is currently supported
*/
#define NR_SG   128

#define MXC_MMC_DMA_ENABLE

static unsigned int card_selected[2] = {0,0};
static unsigned slot_id = 0;
module_param(slot_id, uint, 0);
MODULE_PARM_DESC(slot_id, "MMC slot to use (default is 0)");



#ifdef CONFIG_MMC_DEBUG
static void dump_cmd(struct mmc_command *cmd)
{
	DBG(2,"%s: CMD: opcode: %2d ", DRIVER_NAME, cmd->opcode);
	DBG(2,"arg: 0x%08x ", cmd->arg);
	DBG(2,"flags: 0x%08x", cmd->flags);
	DBG(2,"\n");
}

static void dump_status(const char *func, int sts)
{
	unsigned int bitset;
	DBG(3,"%s:status: ", func);
	while (sts) {
		/* Find the next bit set */
		bitset = sts & ~(sts - 1);
		switch (bitset) {
		case STATUS_CARD_INSERTION:
			DBG(3,"CARD_INSERTION|");
			break;
		case STATUS_CARD_REMOVAL:
			DBG(3,"CARD_REMOVAL |");
			break;
		case STATUS_YBUF_EMPTY:
			DBG(3,"YBUF_EMPTY |");
			break;
		case STATUS_XBUF_EMPTY:
			DBG(3,"XBUF_EMPTY |");
			break;
		case STATUS_YBUF_FULL:
			DBG(3,"YBUF_FULL |");
			break;
		case STATUS_XBUF_FULL:
			DBG(3,"XBUF_FULL |");
			break;
		case STATUS_BUF_UND_RUN:
			DBG(3,"BUF_UND_RUN |");
			break;
		case STATUS_BUF_OVFL:
			DBG(3,"BUF_OVFL |");
			break;
		case STATUS_READ_OP_DONE:
			DBG(3,"READ_OP_DONE |");
			break;
		case STATUS_WR_CRC_ERROR_CODE_MASK:
			DBG(3,"WR_CRC_ERROR_CODE |");
			break;
		case STATUS_READ_CRC_ERR:
			DBG(3,"READ_CRC_ERR |");
			break;
		case STATUS_WRITE_CRC_ERR:
			DBG(3,"WRITE_CRC_ERR |");
			break;
		case STATUS_SDIO_INT_ACTIVE:
			DBG(3,"SDIO_INT_ACTIVE |");
			break;
		case STATUS_END_CMD_RESP:
			DBG(3,"END_CMD_RESP |");
			break;
		case STATUS_WRITE_OP_DONE:
			DBG(3,"WRITE_OP_DONE |");
			break;
		case STATUS_CARD_BUS_CLK_RUN:
			DBG(3,"CARD_BUS_CLK_RUN |");
			break;
		case STATUS_BUF_READ_RDY:
			DBG(3,"BUF_READ_RDY |");
			break;
		case STATUS_BUF_WRITE_RDY:
			DBG(3,"BUF_WRITE_RDY |");
			break;
		case STATUS_RESP_CRC_ERR:
			DBG(3,"RESP_CRC_ERR |");
			break;
		case STATUS_TIME_OUT_RESP:
			DBG(3,"TIME_OUT_RESP |");
			break;
		case STATUS_TIME_OUT_READ:
			DBG(3,"TIME_OUT_READ |");
			break;
		default:
			DBG(3,"Ivalid Status Register value0x%x\n", bitset);
			break;
		}
		sts &= ~bitset;
	}
	DBG(3,"\n");
}

#endif

/*!
 * This structure is a way for the low level driver to define their own
 * \b mmc_host structure. This structure includes the core \b mmc_host
 * structure that is provided by Linux MMC/SD Bus protocol driver as an
 * element and has other elements that are specifically required by this
 * low-level driver.
 */
struct mxcmci_host {
	/*!
	 * The mmc structure holds all the information about the device
	 * structure, current SDHC io bus settings, the current OCR setting,
	 * devices attached to this host, and so on.
	 */
	struct mmc_host *mmc;

	/*!
	 * This variable is used for locking the host data structure from
	 * multiple access.
	 */
	spinlock_t lock;

	/*!
	 * Resource structure, which will maintain base addresses and IRQs.
	 */
	struct resource *res;

	/*!
	 * Base address of SDHC, used in readl and writel.
	 */
	void *base;

	/*!
	 * SDHC IRQ number.
	 */
	int irq;

	/*!
	 * Clock id to hold ipg_perclk.
	 */
	enum mxc_clocks clock_id;
	/*!
	 * DMA Event number.
	 */
	int event_id;

	/*!
	 * DMA channel number.
	 */
	int dma;

	/*!
	 * Holds the Interrupt Control Register value.
	 */
	unsigned int icntr;

	/*!
	 * Pointer to hold MMC/SD request.
	 */
	struct mmc_request *req;

	/*!
	 * Pointer to hold MMC/SD command.
	 */
	struct mmc_command *cmd;

	/*!
	 * Pointer to hold MMC/SD data.
	 */
	struct mmc_data *data;

	/*!
	 * Holds the number of bytes to transfer using SDMA.
	 */
	unsigned int dma_size;

	/*!
	 * Value to store in Command and Data Control Register
	 * - currently unused
	 */
	unsigned int cmdat;

	/*!
	 * Power mode - currently unused
	 */
	unsigned int power_mode;

	/*!
	 * DMA buffer for transfers
	 */
	char *dma_buffer;
	
	/*!
	 * DMA address for transfers
	 */
	dma_addr_t dma_addr;

	/*!
	 * Length of the dma list
	 */
	unsigned int dma_len;
	#define DMA_BUF_LEN 65536

	/*!
	 * Holds the direction of data transfer.
	 */
	unsigned int dma_dir;

	/*!
	 * Work queue to handle long delays that would otherwise
	 * be in the gpio irq handler.
	 */
	struct work_struct switch_work;

	/*!
	 * Work queue to handle deletion of MMC card node
	 * in the system.
	 */
	struct work_struct card_del_work;

	/*!
	 * Id for MMC block.  MXC91231 has 2 MMC blocks .
	 */
	unsigned int id;

	/*!
	 * Note whether this driver has been suspended.
	 */
	unsigned int mxc_mmc_suspend_flag;

	/*!
	 * Counter for debouncing card insertions.
	 */
	unsigned int insert_debounce_counter;
	
	/*!
	 * Counter to be sure we catch card removal.
	 */
	unsigned int remove_debounce_counter;

	/*!
	 * Timer for debouncing card insertions.
	 */
	struct timer_list debounce_timer;
	struct timer_list cmd_timeout_timer;

	/*
	 * SDHC error flag, if the SDHC enter a error
  	 * status, then set this flag.
	 */
	int sdhc_err;
};

#ifdef CONFIG_MMC_DEBUG
static void dump_sdhc(struct mxcmci_host *host)
{
	DBG(1,"mxclay: SDHC register \n");
	DBG(1,"CLK     0x%08x \n", (__raw_readl(host->base + MMC_STR_STP_CLK )));
	DBG(1,"STATUS  0x%08x \n", (__raw_readl(host->base + MMC_STATUS)));
	DBG(1,"CLTRATE 0x%08x \n", (__raw_readl(host->base + MMC_CLK_RATE)));
	DBG(1,"CMDCONT 0x%08x \n", (__raw_readl(host->base + MMC_CMD_DAT_CONT)));
	DBG(1,"RES_TO  0x%08x \n", (__raw_readl(host->base + MMC_RES_TO)));
	DBG(1,"READ_TO 0x%08x \n", (__raw_readl(host->base + MMC_READ_TO)));
	DBG(1,"BLK_LEN 0x%08x \n", (__raw_readl(host->base + MMC_BLK_LEN)));
	DBG(1,"NOB     0x%08x \n", (__raw_readl(host->base + MMC_NOB)));
	DBG(1,"REV_NO  0x%08x \n", (__raw_readl(host->base + MMC_REV_NO)));
	DBG(1,"INT_CNTR0x%08x \n", (__raw_readl(host->base + MMC_INT_CNTR)));
	DBG(1,"CMD     0x%08x \n", (__raw_readl(host->base + MMC_CMD)));
	DBG(1,"ARG     0x%08x \n", (__raw_readl(host->base + MMC_ARG)));
	DBG(1,"RESFIFO 0x%08x \n", (__raw_readl(host->base + MMC_RES_FIFO)));
	DBG(1,"BUFACC  0x%08x \n", (__raw_readl(host->base + MMC_BUFFER_ACCESS)));
	DBG(1,"REMNOB  0x%08x \n", (__raw_readl(host->base + MMC_REM_NOB)));
	DBG(1,"REMBSZ  0x%08x \n", (__raw_readl(host->base + MMC_REM_BLK_SIZE)));

}
#endif

void (*mmc_turn_off_clock)(struct mmc_host *host);
void (*mmc_turn_on_clock)(struct mmc_host *host);
extern unsigned int mmc_blk_get_refcount(int module);


struct work_struct del_dead;
void mxc_remove_dead_card(struct mxcmci_host *host)
{
        struct list_head *l, *n;
	struct mmc_host * mmc = host->mmc;
        host->sdhc_err = 2;

        list_for_each_safe(l, n, &mmc->cards) {
                struct mmc_card *card = mmc_list_to_card(l);

                mmc_card_set_dead(card);

                /*
                 * If this card is dead, destroy it.
                 */
                list_del(&card->node);
                mmc_remove_card(card);
                printk("drop test remove card\n");
		host->sdhc_err = 3;

        }

        /* for the application layers sufficient time
         * to respond to the hotplug event.
         */
        mpm_handle_ioi();
	
        printk("drop test remove card out\n");
	if( host->sdhc_err == 2)
		host->sdhc_err = 4;
}

static int mxcmci_cmd_done(struct mxcmci_host *host, unsigned int stat);

static void mxc_sdhc_err(struct mxcmci_host * host)
{
        printk("%s: FORCED CMD TIMEOUT for drop test error2.\n", DRIVER_NAME);
        mxcmci_cmd_done(host, (unsigned int)(STATUS_TIME_OUT_RESP|STATUS_END_CMD_RESP));

        card_selected[host->id] = 0;

        if(host->sdhc_err == 0 || host->sdhc_err == 4)
        {
        	host->sdhc_err = 1;
        	INIT_WORK( &del_dead, mxc_remove_dead_card, (void *)host); 
		schedule_delayed_work(&del_dead, 20);
	}

        host->mmc->ios.clock = 0;
        host->mmc->ios.power_mode = MMC_POWER_OFF;

}



void mxcsdhc_clockpll2_disable(struct mmc_host *host)
{
	struct platform_device *pdev = to_platform_device(host->dev);

	if (pdev->id == 0)
		mxc_clks_disable(SDHC1_CLK);
	else
		mxc_clks_disable(SDHC2_CLK);
#ifdef CONFIG_PM
	mxc_pll_release_pll(USBPLL);
#endif
}

void mxcsdhc_clockpll2_enable(struct mmc_host *host)
{
	struct platform_device *pdev = to_platform_device(host->dev);

#ifdef CONFIG_PM
	mxc_pll_request_pll(USBPLL);
#endif
	if (pdev->id == 0)
		mxc_clks_enable(SDHC1_CLK);
	else
		mxc_clks_enable(SDHC2_CLK);
}

#ifndef CONFIG_MOT_FEAT_GPIO_API_SDHC
extern void gpio_sdhc_active(int module);
extern void gpio_sdhc_inactive(int module);
extern int sdhc_intr_setup(void *host,
			   irqreturn_t(*handler) (int, void *,
						  struct pt_regs *));
extern void sdhc_intr_destroy(void *host);
extern void sdhc_intr_clear(int flag);
extern unsigned int sdhc_get_min_clock(enum mxc_clocks clk);
extern unsigned int sdhc_get_max_clock(enum mxc_clocks clk);
extern int sdhc_find_card(int id);
#endif /* not CONFIG_MOT_FEAT_GPIO_API_SDHC */

#ifdef MXC_MMC_DMA_ENABLE
static void mxcmci_dma_irq(void *devid);
#endif
static int mxcmci_data_done(struct mxcmci_host *host, unsigned int stat);

#ifdef CONFIG_MOT_FEAT_MEGASIM
static struct mxcmci_host *megasim_host = NULL;

#if 0
struct timer_list megasim_timer; 
static void megasim_timer_handler(unsigned long arg)
{
	printk("megasim rescan start\n");
	megasim_restart_mmc(1);
}
#endif
#endif

static inline void mxcmci_sg_to_dma(struct mxcmci_host* host, struct mmc_data* data)
{
        unsigned int len, i, size;
        struct scatterlist* sg;
        char* dmabuf = host->dma_buffer;
        char* sgbuf;

        size = host->dma_size;

        sg = data->sg;
        len = data->sg_len;

        /*
         * Just loop through all entries. Size might not
         * be the entire list though so make sure that
         * we do not transfer too much.
         */
        for (i = 0;i < len;i++)
        {
                sgbuf = kmap_atomic(sg[i].page, KM_BIO_SRC_IRQ) + sg[i].offset;
                if (size < sg[i].length)
                        memcpy(dmabuf, sgbuf, size);
                else
                        memcpy(dmabuf, sgbuf, sg[i].length);
                kunmap_atomic(sg[i].page, KM_BIO_SRC_IRQ);
                dmabuf += sg[i].length;

                if (size < sg[i].length)
                        size = 0;
                else
                        size -= sg[i].length;

                if (size == 0)
                        break;
        }

        /*
         * Check that we didn't get a request to transfer
         * more data than can fit into the SG list.
         */

        BUG_ON(size != 0);

        host->dma_size -= size;
}


static inline void mxcmci_dma_to_sg(struct mxcmci_host* host, struct mmc_data* data)
{
        unsigned int len, i, size;
        struct scatterlist* sg;
        char* dmabuf = host->dma_buffer;
        char* sgbuf;

        size = host->dma_size;

        sg = data->sg;
        len = data->sg_len;

        /*
         * Just loop through all entries. Size might not
         * be the entire list though so make sure that
         * we do not transfer too much.
         */
        for (i = 0;i < len;i++)
        {
                sgbuf = kmap_atomic(sg[i].page, KM_BIO_SRC_IRQ) + sg[i].offset;
                if (size < sg[i].length)
                        memcpy(sgbuf, dmabuf, size);
                else
                        memcpy(sgbuf, dmabuf, sg[i].length);
                kunmap_atomic(sg[i].page, KM_BIO_SRC_IRQ);
                dmabuf += sg[i].length;

                if (size < sg[i].length)
                        size = 0;
                else
                        size -= sg[i].length;

                if (size == 0)
                        break;
        }

        /*
         * Check that we didn't get a request to transfer
         * more data than can fit into the SG list.
         */

        BUG_ON(size != 0);

        host->dma_size -= size;
}

/*
 * This function init the MMC/SD power supply
 */
static void mxcmci_power_init(int id)
{
	switch(id){
		case 0:
#ifndef CONFIG_MACH_MXC91131EVB
			printk("mxclay: slot 0 power up \n");
			power_ic_set_transflash_voltage(3200);
#endif
			break;
		case 1:

			break;
		default:
			;
	}
	
}
static void mxcmci_power_exit(int id)
{
	switch(id){
		case 0:
#ifndef CONFIG_MACH_MXC91131EVB
			printk("mxclay: slot 0 power down \n");
			power_ic_set_transflash_voltage(0);
#endif
			break;
		case 1:
			break;
		default:
			;
	}
}

/*!
 * This function sets the SDHC register to stop the clock and waits for the
 * clock stop indication.
 */
static int mxcmci_stop_clock(struct mxcmci_host *host)
{
	int wait_cnt,ret = 0;
	unsigned long start = jiffies;

	while (1) {
		__raw_writel(0, host->base + MMC_STR_STP_CLK);
		__raw_writel(STR_STP_CLK_STOP_CLK, host->base + MMC_STR_STP_CLK);
		wait_cnt = 0;
		while ((__raw_readl(host->base + MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN) && (wait_cnt++ < 100)) ;

		if (!(__raw_readl(host->base + MMC_STATUS) &
		     STATUS_CARD_BUS_CLK_RUN))
			break;
		if( jiffies - start > 100 ){
		/* drop test error, the previod command is timeout */
			printk("mxclay: time out when stop clock %x\n", (__raw_readl(host->base + MMC_STATUS)));
			ret = -1;
#ifdef CONFIG_MMC_DEBUG
			dump_sdhc(host);
#endif
			break;
		}
	}
	return ret;
}

/* Wait count to start the clock */
#define CMD_WAIT_CNT 100
/*!
 * This function sets the SDHC register to start the clock and waits for the
 * clock start indication. When the clock starts SDHC module starts processing
 * the command in CMD Register with arguments in ARG Register.
 *
 * @param host Pointer to MMC/SD host structure
 */
static int mxcmci_start_clock(struct mxcmci_host *host)
{
	int wait_cnt, ret = 0;
	unsigned long start = jiffies;

#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, __raw_readl(host->base + MMC_STATUS));
#endif
	while (1) {
		__raw_writel(STR_STP_CLK_START_CLK, host->base + MMC_STR_STP_CLK);
		wait_cnt = 0;
		while (!(__raw_readl(host->base + MMC_STATUS) &
			 STATUS_CARD_BUS_CLK_RUN) &&
		       (wait_cnt++ < CMD_WAIT_CNT)) {
			/* Do Nothing */
		}

		if (__raw_readl(host->base + MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN) {
			break;
		}
                if( jiffies - start > 100 ){
                /* drop test error, the previod command is timeout */
                        printk("mxclay:time out when start clock%x\n", (__raw_readl(host->base + MMC_STATUS)));
                        ret = -1;
#ifdef CONFIG_MMC_DEBUG
			dump_sdhc(host);
#endif
                        break;
                }

	}
#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, __raw_readl(host->base + MMC_STATUS));
#endif
	DBG(3,"%s:CLK_RATE: 0x%08x\n", DRIVER_NAME,
	    __raw_readl(host->base + MMC_CLK_RATE));
	return ret;
}

/*!
 * This function resets the SDHC host.
 *
 * @param host  Pointer to MMC/SD  host structure
 */
static void mxcmci_softreset(struct mxcmci_host *host)
{
	/* reset sequence */
	__raw_writel(0x8, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x9, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x3f, host->base + MMC_CLK_RATE);

	__raw_writel(0xff, host->base + MMC_RES_TO);
	__raw_writel(512, host->base + MMC_BLK_LEN);
	__raw_writel(1, host->base + MMC_NOB);
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
static void mxcmci_setup_data(struct mxcmci_host *host, struct mmc_data *data)
{
	unsigned int nob = data->blocks;

#ifdef MXC_MMC_DMA_ENABLE
	dma_request_t sdma_request;
	dma_channel_params params;
#endif
	if (data->flags & MMC_DATA_STREAM) {
		nob = 0xffff;
	}

	host->data = data;

	__raw_writel(nob, host->base + MMC_NOB);
	__raw_writel(1 << data->blksz_bits, host->base + MMC_BLK_LEN);

	host->dma_size = data->blocks << data->blksz_bits;
	DBG(2,"%s:Request bytes to transfer:%d\n", DRIVER_NAME, host->dma_size);
#ifdef MXC_MMC_DMA_ENABLE
	if (data->flags & MMC_DATA_READ) {
		host->dma_dir = DMA_FROM_DEVICE;
	} else {
		mxcmci_sg_to_dma(host, data);
		host->dma_dir = DMA_TO_DEVICE;
	}
	host->dma_addr = dma_map_single(mmc_dev(host->mmc), host->dma_buffer, 
				host->dma_len, host->dma_dir);
	
	if (data->flags & MMC_DATA_READ) {
		sdma_request.destAddr = (__u8 *)host->dma_addr;
		params.transfer_type = per_2_emi;
	} else {
		sdma_request.sourceAddr = (__u8 *)host->dma_addr;
		params.transfer_type = emi_2_per;
	}

	/*
	 * Setup DMA Channel parameters
	 */
	/* 16 bytes in 1-bit mode and 64 bytes in 4-bit mode */
	params.watermark_level = (host->mmc->ios.bus_width ? SDHC_SD_WML : SDHC_MMC_WML);
	params.peripheral_type = MMC;
	params.per_address = host->res->start + MMC_BUFFER_ACCESS;
	params.event_id = host->event_id;
	params.bd_number = 1 /* host->dma_size / 256 */ ;
	params.word_size = TRANSFER_32BIT;
	params.callback = mxcmci_dma_irq;
	params.arg = host;
	mxc_dma_setup_channel(host->dma, &params);

	sdma_request.count = data->blocks << data->blksz_bits;
	mxc_dma_set_config(host->dma, &sdma_request, 0);

	/* mxc_dma_start(host->dma); */
#endif
}

/*!
 * This function is called by \b mxcmci_request() function to setup the SDHC
 * register to issue command. This function disables the card insertion and
 * removal detection interrupt.
 *
 * @param host  Pointer to MMC/SD host structure
 * @param cmd   Pointer to MMC/SD command structure
 * @param cmdat Value to store in Command and Data Control Register
 */
static void mxcmci_start_cmd(struct mxcmci_host *host, struct mmc_command *cmd,
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
		cmdat |= CMD_DAT_CONT_INIT;	/* This command needs init */
	}
	
	if (host->mmc->ios.bus_width) {
		cmdat |= CMD_DAT_CONT_BUS_WIDTH_4; /* support 4bit width */
	} 
	
	__raw_writel(cmd->opcode, host->base + MMC_CMD);
	__raw_writel(cmd->arg, host->base + MMC_ARG);

	__raw_writel(cmdat, host->base + MMC_CMD_DAT_CONT);

	if( host->sdhc_err || mxcmci_start_clock(host) != 0){
		mxc_sdhc_err(host);
                return;
        }
	else 
		mod_timer(&host->cmd_timeout_timer, jiffies + 20);
}




static void cmd_timeout_timer_func(unsigned long p)
{
        struct mxcmci_host *host;

        host = (struct mxcmci_host *)p;

	mxcmci_cmd_done(host, (unsigned int)(STATUS_TIME_OUT_RESP|STATUS_END_CMD_RESP));

}

/*!
 * This function is called to complete the command request.
 * This function enables insertion or removal interrupt.
 *
 * @param host Pointer to MMC/SD host structure
 * @param req  Pointer to MMC/SD command request structure
 */
static void mxcmci_finish_request(struct mxcmci_host *host,
				  struct mmc_request *req)
{

	mmc_turn_off_clock(host->mmc);
	
	host->req = NULL;
	host->cmd = NULL;
	host->data = NULL;
	mmc_request_done(host->mmc, req);
}

/*!
 * This function is called when the requested command is completed.
 * This function reads the response from the card and data if the command is for
 * data transfer. This function checks for CRC error in response FIFO or
 * data FIFO.
 *
 * @param host  Pointer to MMC/SD host structure
 * @param stat  Content of SDHC Status Register
 *
 * @return This function returns 0 if there is no pending command, otherwise 1
 * always.
 */
static int mxcmci_cmd_done(struct mxcmci_host *host, unsigned int stat)
{
	struct mmc_command *cmd = host->cmd;
#ifndef MXC_MMC_DMA_ENABLE
	struct mmc_data *data = host->data;
#endif
	int i;
	u32 a, b, c;

	if (!cmd) {
		/* There is no command for completion */
		return 0;
	}

	/* As this function finishes the command, initialize cmd to NULL */
	host->cmd = NULL;

	/* check for Time out errors */
	if (stat & STATUS_TIME_OUT_RESP) {
		printk("%s: cmd 0x%x timeout\n", DRIVER_NAME, cmd->opcode);
		cmd->error = MMC_ERR_TIMEOUT;
	} else if (stat & STATUS_RESP_CRC_ERR && cmd->flags & MMC_RSP_CRC) {
		printk("%s: cmd 0x%x crcerror\n", DRIVER_NAME, cmd->opcode);
		cmd->error = MMC_ERR_BADCRC;
	}

	if (cmd->opcode == MMC_SELECT_CARD) {
		if (cmd->error == MMC_ERR_NONE) {
			DBG(2,"%s: Card selected\n", DRIVER_NAME);
			card_selected[host->id] = 1;
			if (sdhc_find_card(host->id) != 0) {
				printk("%s: Card Un-selected\n", DRIVER_NAME);
				card_selected[host->id] = 0;
			}
		} else {
			printk("%s: Card Un-selected\n", DRIVER_NAME);
			card_selected[host->id] = 0;
		}
	}

	/* Read response from the card */
	switch (cmd->flags & (MMC_RSP_MASK | MMC_RSP_CRC)) {
	case MMC_RSP_SHORT | MMC_RSP_CRC:
		a = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		b = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		c = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		cmd->resp[0] = a << 24 | b << 8 | c >> 8;
		break;
	case MMC_RSP_SHORT:
		a = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		b = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		c = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		cmd->resp[0] = a << 24 | b << 8 | c >> 8;
		break;
	case MMC_RSP_LONG | MMC_RSP_CRC:
		for (i = 0; i < 4; i++) {
			a = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
			b = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
			cmd->resp[i] = a << 16 | b;
		}
		break;
	default:
		break;
	}

	DBG(3,"%s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n", DRIVER_NAME, cmd->resp[0],
	    cmd->resp[1], cmd->resp[2], cmd->resp[3]);

	if (host->data && cmd->error == MMC_ERR_NONE) {
		/* The command is associated with data transfer */
#ifdef MXC_MMC_DMA_ENABLE
		unsigned int icntrval;

		DBG(2," mxc_dma_start \n");

		icntrval = __raw_readl(host->base + MMC_INT_CNTR);
		icntrval |= 0x3;
		__raw_writel(icntrval, host->base + MMC_INT_CNTR);

		mxc_dma_start(host->dma);
#endif
#ifndef MXC_MMC_DMA_ENABLE	/* Manual tranfer of data */
		unsigned int i = 0, status;
		unsigned long *buf = (unsigned long *)data->req->buffer;

		/* calculate the number of bytes requested for transfer */
		int no_of_bytes = data->blocks << data->blksz_bits;
		int no_of_words = no_of_bytes / 4;
		DBG(3,"no_of_words=%d\n", no_of_words);

		if (data->flags & MMC_DATA_READ) {
			for (i = 0; i < no_of_words; i++) {
				/* wait for buffers to be ready for read */
				do {
					status =
					    __raw_readl(host->base + MMC_STATUS);
				} while (!(status & STATUS_BUF_READ_RDY));

				udelay(2);

				/* read 32 bit data */
				*buf++ = __raw_readl(host->base + MMC_BUFFER_ACCESS);
				if (__raw_readl(host->base + MMC_STATUS) &
				    STATUS_READ_OP_DONE) {
					DBG(3,"i=%d\n", i);
					/*
					 * Check whether you completely read
					 * no_of_words?
					 *
					 */
					status = __raw_readl(host->base + MMC_STATUS);
					if (status & (STATUS_BUF_READ_RDY |
					     STATUS_XBUF_FULL |
					     STATUS_YBUF_FULL)) {
						continue;
					} else {
						break;
					}
				}
			}

			/* wait for read operation completion bit */
			do {
				status = __raw_readl(host->base + MMC_STATUS);
			} while (!(status & STATUS_READ_OP_DONE));

			/* check for time out and CRC errors */
			status = __raw_readl(host->base + MMC_STATUS);
			if (status & STATUS_READ_OP_DONE) {
				if (status & STATUS_TIME_OUT_READ) {
					DBG(1,"%s: Read time out occurred\n",
					    DRIVER_NAME);
					data->error = MMC_ERR_TIMEOUT;
					__raw_writel(STATUS_TIME_OUT_READ,
						     host->base + MMC_STATUS);
				} else if (status & STATUS_READ_CRC_ERR) {
					DBG(1,"%s: Read CRC error occurred\n",
					    DRIVER_NAME);
					data->error = MMC_ERR_BADCRC;
					__raw_writel(STATUS_READ_CRC_ERR,
						     host->base + MMC_STATUS);
				}
				__raw_writel(STATUS_READ_OP_DONE,
					     host->base + MMC_STATUS);
			}
			DBG(3,"%s: Read %u words\n", DRIVER_NAME, i);
		} else {
			for (i = 0; i < no_of_words; i++) {

				/* wait for buffers to be ready for write */
				do {
					status =  __raw_readl(host->base + MMC_STATUS);
				} while (!(status & STATUS_BUF_WRITE_RDY));

				/* write 32 bit data */
				__raw_writel(*buf++, host->base + MMC_BUFFER_ACCESS);
				if (__raw_readl(host->base + MMC_STATUS) &
				    STATUS_WRITE_OP_DONE) {
					break;
				}
			}

			/* wait for write operation completion bit */
			do {
				status = __raw_readl(host->base + MMC_STATUS);
			} while (!(status & STATUS_WRITE_OP_DONE));

			/* check for CRC errors */
			if (status & STATUS_WRITE_OP_DONE) {
				if (status & STATUS_WRITE_CRC_ERR) {
					DBG(3,"%s: Write CRC error occurred\n",
					    DRIVER_NAME);
					data->error = MMC_ERR_BADCRC;
					__raw_writel(STATUS_WRITE_CRC_ERR,
						     host->base + MMC_STATUS);
				}
				__raw_writel(STATUS_WRITE_OP_DONE, host->base +
					     MMC_STATUS);
			}
			DBG(3,"%s: Written %u words\n", DRIVER_NAME, i);
		}

		/* complete the data transfer request */
		mxcmci_data_done(host, status);
#endif
	} else {
		/* complete the command */
		mxcmci_finish_request(host, host->req);
	}

	return 1;
}

/*!
 * This function is called when the data transfer is completed either by SDMA
 * or by core. This function is called to clean up the DMA buffer and to send
 * STOP transmission command for commands to transfer data. This function
 * completes request issued by the MMC/SD core driver.
 *
 * @param host   pointer to MMC/SD host structure.
 * @param stat   content of SDHC Status Register
 *
 * @return This function returns 0 if no data transfer otherwise return 1
 * always.
 */
static int mxcmci_data_done(struct mxcmci_host *host, unsigned int stat)
{
	struct mmc_data *data = host->data;

	if (!data) {
		return 0;
	}
#ifdef MXC_MMC_DMA_ENABLE
	dma_unmap_single(mmc_dev(host->mmc), host->dma_addr,
                                host->dma_len, host->dma_dir);
#endif
	if (__raw_readl(host->base + MMC_STATUS) & STATUS_ERR_MASK) {
		DBG(3,"%s: request failed. status: 0x%08x\n",
		    DRIVER_NAME, __raw_readl(host->base + MMC_STATUS));
	}

	if(data->flags & MMC_DATA_READ)
		mxcmci_dma_to_sg(host, data);

	host->data = NULL;
	data->bytes_xfered = host->dma_size;

	if (host->req->stop && data->error == MMC_ERR_NONE) {
		mxcmci_start_cmd(host, host->req->stop, 0);
	} else {
		mxcmci_finish_request(host, host->req);
	}

	return 1;
}

static void mxcmci_debounce_timer(unsigned long arg)
{
	struct mxcmci_host *host = (struct mxcmci_host *)arg;
	int card_gpio_status = sdhc_find_card(host->id);

	DBG(2,"In.. %s\n", __FUNCTION__);
	if (card_gpio_status == 0) {
		host->remove_debounce_counter = 0;
		if (mmc_blk_get_refcount(host->id)) {
			printk(KERN_INFO "Card has NOT been unmounted, waiting...");
			mod_timer(&host->debounce_timer,
				jiffies + (MXC_MMC_DEBOUNCE_POLL_DELAY_MSEC * HZ / 1000));
			return;
		}
		host->insert_debounce_counter++;

	} else {
		host->insert_debounce_counter = 0;
		host->remove_debounce_counter++;
	}

	/*
	 * Take care of 2 scenarios:
	 * Insert - wait for us to see that card status gpio is sampled at
	 * a stable level 10 times before we assume card can be accessed, in
	 * case someone is plugging card in slowly.
	 * Remove - we tried to do mmc_detect_change as soon as we saw
	 * the gpio indicate the card not present, but do it again to be sure
	 * that a bounce didn't happen that make the card look present when
	 * the mmc stack was detecting.
	 */
	if ((host->insert_debounce_counter >= MXC_MMC_DEBOUNCE_HAPPY_COUNT) ||
	    (host->remove_debounce_counter >= MXC_MMC_DEBOUNCE_HAPPY_COUNT) ||
		host->id == 1) {
		if( host->id == 0 )
		{
			if (card_gpio_status) {
				printk(KERN_INFO "Card is removed\n");
/*#ifndef CONFIG_MACH_MXC91131EVB
				power_ic_set_transflash_voltage(0);
#endif*/
			} else {
				printk(KERN_INFO "Card is inserted\n");
/*#ifndef CONFIG_MACH_MXC91131EVB
				power_ic_set_transflash_voltage(3200);
#endif*/
			}
		}
		schedule_work(&host->switch_work);
	} else {
		/*
		 * Reschedule the debounce timer to fire again.
		 */
		mod_timer(&host->debounce_timer,
			  jiffies + (MXC_MMC_DEBOUNCE_POLL_DELAY_MSEC *
				     HZ / 1000));
	}
	DBG(2,"Out.. %s\n", __FUNCTION__);
}

/*!
 * Bottom half of GPIO interrupt service routine for card insertion and
 * card removal interrupts.  Implemented as a work queue instead of
 * a tasklet only because we want to sleep.
 *
 * @param   devid  driver private data
 *
 * @return  void
 */
void mxcmci_wq_handler(void *devid)
{
	struct mxcmci_host *host = (struct mxcmci_host *)devid;

	DBG(2,"In.. %s\n", __FUNCTION__);
	/* If we were suspended, resume automatically so we can read card */
	if ((host != 0) && host->mxc_mmc_suspend_flag) {
		mxc_clks_enable(host->clock_id);
		gpio_sdhc_active(host->id);
		host->mxc_mmc_suspend_flag = 0;
	}

	mmc_detect_change(host->mmc, 0);
	DBG(2,"Out.. %s\n", __FUNCTION__);
}

/*!
 * Bottom half of GPIO interrupt service routine for deleting MMC card
 * device node from system immediatly if a MMC card GPIO interrupt is detected.
 *
 * @param   devid  driver private data
 *
 * @return  void
 */
void mxcmci_remove_card(void *devid)
{
	struct mxcmci_host *host = (struct mxcmci_host *)devid;
	struct mmc_host *mhost;
	struct list_head *l, *n;

	mhost = host->mmc;
	list_for_each_safe(l, n, &mhost->cards) {
		struct mmc_card *card = mmc_list_to_card(l);

		mmc_card_set_dead(card);

		/*
		 * If this card is dead, destroy it.
		 */
		list_del(&card->node);
		if (mmc_card_present(card))
			device_del(&card->dev);
		put_device(&card->dev);
	}
}

static unsigned int mmc_busy_flag = 0;

void mmc_clear_busy_flag(void)
{
	mmc_busy_flag = 0;
}
EXPORT_SYMBOL(mmc_clear_busy_flag);

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
static irqreturn_t mxcmci_gpio_irq(int irq, void *devid, struct pt_regs *regs)
{
	struct mxcmci_host *host = devid;
	int card_gpio_status = sdhc_find_card(host->id);

	DBG(2,"In.. %s\n", __FUNCTION__);
	DBG(2,"%s: MMC%d status=%d %s\n", DRIVER_NAME, host->id, card_gpio_status,
	    card_gpio_status ? "removed" : "inserted");

	/*
	 * If interrupt is because card has been inserted, set
	 * level detect to irq if card is removed.  And vice versa.
	 */
	sdhc_intr_clear( host->id, card_gpio_status);

	/* set busy flag to prevent the system from entering DSM, 
	 * it will be cleared after finish rescaning mmc bus in mmc.c.
	 */
	mmc_busy_flag = 1;
	card_selected[host->id] = 0;
	DBG(2,"%s: Card Un-selected\n", DRIVER_NAME);

	if (card_gpio_status) {
		DBG(2, "gpio_irq: card_gpio_status = 0x%x\n", card_gpio_status);
		schedule_work(&host->card_del_work);
	}

	/*
	 * If the card is being pulled out, schedule to call mmc_detect_change
	 * to detect card removal.
	 */
	if (host->insert_debounce_counter >= MXC_MMC_DEBOUNCE_HAPPY_COUNT) {
		schedule_work(&host->switch_work);
	}

	/*
	 * We just got an interrupt, so clear debounce counter because
	 * it's not done bouncing.
	 */
	if (card_gpio_status == 0) {
		host->remove_debounce_counter = 0;
	} else {
		host->insert_debounce_counter = 0;
	}

	/*
	 * Schedule or reschedule the debounce timer to fire off after
	 * a short delay.
	 */
	mod_timer(&host->debounce_timer,
		  jiffies + (MXC_MMC_DEBOUNCE_POLL_DELAY_MSEC * HZ / 1000));

	/* tell mpm to delay sleep */
	mpm_handle_ioi();

	DBG(2,"Out.. %s\n", __FUNCTION__);
	return IRQ_RETVAL(1);
}

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
static irqreturn_t mxcmci_irq(int irq, void *devid, struct pt_regs *regs)
{
	struct mxcmci_host *host = devid;
	unsigned int status = 0;
	ulong flags;

	spin_lock_irqsave(&host->lock, flags);

	DBG(3,"In.. %s\n", __FUNCTION__);
	status = __raw_readl(host->base + MMC_STATUS);
#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, status);
#endif
	__raw_writel(status, host->base + MMC_STATUS);
	if (status & STATUS_END_CMD_RESP) {
		del_timer(&host->cmd_timeout_timer);
		mxcmci_cmd_done(host, status);
	}

        if (status & (STATUS_READ_OP_DONE | STATUS_WRITE_OP_DONE)) {
                unsigned int icntrval;
		struct mmc_data *data = host->data;

                icntrval = __raw_readl(host->base + MMC_INT_CNTR);
                icntrval &= ~0x3;
                __raw_writel(icntrval, host->base + MMC_INT_CNTR);

                DBG(2,"%s:READ/WRITE OPERATION DONE\n", DRIVER_NAME);
                /* check for time out and CRC errors */
                status = __raw_readl(host->base + MMC_STATUS);
                if (status & STATUS_READ_OP_DONE) {
                        if (status & STATUS_TIME_OUT_READ) {
                                printk("%s: Read time out occurred\n",
                                    DRIVER_NAME);
                                data->error = MMC_ERR_TIMEOUT;
                                __raw_writel(STATUS_TIME_OUT_READ,
                                             host->base + MMC_STATUS);
                        } else if (status & STATUS_READ_CRC_ERR) {
                                printk("%s: Read CRC error occurred\n",
                                    DRIVER_NAME);
                                data->error = MMC_ERR_BADCRC;
                                __raw_writel(STATUS_READ_CRC_ERR,
                                             host->base + MMC_STATUS);
                        }
                        __raw_writel(STATUS_READ_OP_DONE, host->base + MMC_STATUS);
                }

                /* check for CRC errors */
                if (status & STATUS_WRITE_OP_DONE) {
                        if (status & STATUS_WRITE_CRC_ERR) {
                                printk("%s: Write CRC error occurred\n",
                                    DRIVER_NAME);
                                data->error = MMC_ERR_BADCRC;
                                __raw_writel(STATUS_WRITE_CRC_ERR,
                                             host->base + MMC_STATUS);
                        }
                        __raw_writel(STATUS_WRITE_OP_DONE,
                                     host->base + MMC_STATUS);
                }
        
		mxcmci_data_done(host, status);
        }

	DBG(3,"Out.. %s\n", __FUNCTION__);

	spin_unlock_irqrestore(&host->lock, flags);
	return IRQ_RETVAL(1);
}


/*!
 * This function is called by MMC/SD Bus Protocol driver to issue a MMC
 * and SD commands to the SDHC.
 *
 * @param  mmc  Pointer to MMC/SD host structure
 * @param  req  Pointer to MMC/SD command request structure
 */
static void mxcmci_request(struct mmc_host *mmc, struct mmc_request *req)
{
	struct mxcmci_host *host = mmc_priv(mmc);
	/* Holds the value of Command and Data Control Register */
	unsigned long cmdat;

	mmc_turn_on_clock(mmc);
	if( host->sdhc_err || mxcmci_stop_clock(host)!= 0){
		mxc_sdhc_err( host);
	        return;
	}

	WARN_ON(host->req != NULL);

	host->req = req;
#ifdef CONFIG_MMC_DEBUG
	dump_cmd(req->cmd);
	dump_status(__FUNCTION__, __raw_readl(host->base + MMC_STATUS));
#endif

	if (req->cmd->opcode == MMC_WRITE_BLOCK ||
	    req->cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ||
	    req->cmd->opcode == MMC_READ_SINGLE_BLOCK ||
	    req->cmd->opcode == MMC_READ_MULTIPLE_BLOCK) {
		if (card_selected[host->id] == 0) {
			printk("%s: FORCED CMD TIMEOUT\n", DRIVER_NAME);
			host->req->cmd->error = MMC_ERR_TIMEOUT;
			mxcmci_finish_request(host, host->req);
			return;
		}
	}
	cmdat = 0;
	if (req->data) {
		mxcmci_setup_data(host, req->data);

		cmdat |= CMD_DAT_CONT_DATA_ENABLE;

		if (req->data->flags & MMC_DATA_WRITE) {
			cmdat |= CMD_DAT_CONT_WRITE;
		}
		if (req->data->flags & MMC_DATA_STREAM) {
			printk("MXC MMC does not support stream mode\n");
		}
	}
	mxcmci_start_cmd(host, req->cmd, cmdat);
}

/*!
 * This function is called by MMC/SD Bus Protocol driver to change the clock
 * speed of MMC or SD card
 *
 * @param mmc Pointer to MMC/SD host structure
 * @param ios Pointer to MMC/SD I/O type structure
 */
static void mxcmci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct mxcmci_host *host = mmc_priv(mmc);
	/*This variable holds the value of clock prescaler */
	int prescaler;
	int clk_rate;

	DBG(1,"%s: slot %d clock %u power %u vdd %u.%02u\n", DRIVER_NAME,host->id,
	    ios->clock, ios->power_mode, ios->vdd / 100, ios->vdd % 100);

	if( ios->power_mode == MMC_POWER_UP ){
		printk("mxclay: ios MMC_POWER_ON. id %d mode %d \n", host->id, ios->power_mode);
		mxcmci_power_init(host->id);
	}else if( ios->power_mode == MMC_POWER_OFF ){
		printk("mxclay: ios MMC_POWER_OFF. id %d mode %d \n", host->id, ios->power_mode);
		mxcmci_power_exit(host->id);
	}

	mmc_turn_on_clock(mmc);
	
	clk_rate = mxc_get_clocks(host->clock_id);
	
	/*
	 *  Keep always, prescaler = 0
	 *  vary clk_dev, start with 1 to F
	 */
	if (ios->clock) {
		unsigned int clk_dev;

		/*
		 * when prescaler = 16, CLK_20M = CLK_DIV / 2
		 */
		if (ios->clock == mmc->f_min)
			prescaler = 16;
		else
			prescaler = 0;

		/* clk_dev =1, CLK_DIV = ipg_perclk/2 */

		if ( machine_is_mxc91131evb() ) {
			clk_dev = 0;
		} else {
			clk_dev = 3;	/* we can only use 16MHz or lower freq to avoid SDHC fifo overflow */
		}

		for (; clk_dev <= 0xF; clk_dev++) {
			int x;
			if (ios->clock == mmc->f_min)
				x = (clk_rate / (clk_dev + 1)) / (prescaler * 2);
			else
				x = clk_rate / (clk_dev + 1);

			DBG(3,"x=%d, clock=%d %d\n", x, ios->clock, clk_dev);
			if (x <= ios->clock) {
				DBG(1, "%s: clock=%d %d %d, actual clock=%d\n", 
						DRIVER_NAME, ios->clock, clk_rate, clk_dev, x);
				break;
			}
		}

		mxcmci_stop_clock(host);
		__raw_writel((prescaler << 4) | clk_dev, host->base + MMC_CLK_RATE);
		mxcmci_start_clock(host);
	} else {
		mxcmci_stop_clock(host);
	}

	mmc_turn_off_clock(mmc);
}

/*!
 * MMC/SD host operations structure.
 * These functions are registered with MMC/SD Bus protocol driver.
 */
static struct mmc_host_ops mxcmci_ops = {
	.request = mxcmci_request,
	.set_ios = mxcmci_set_ios
};

#ifdef MXC_MMC_DMA_ENABLE
/*!
 * This function is called by SDMA Interrupt Service Routine to indicate
 * requested DMA transfer is completed.
 *
 * @param   devid  pointer to device specific structure
 */
static void mxcmci_dma_irq(void *devid)
{
	struct mxcmci_host *host = devid;
	u32 status;
#ifdef CONFIG_MMC_DEBUG
	ulong nob, blk_size, blk_len;
#endif
	/* This variable holds DMA configuration parameters */
	dma_request_t sdma_request;

	DBG(2,"In.. %s\n", __FUNCTION__);
	mxc_dma_stop(host->dma);

	mxc_dma_get_config(host->dma, &sdma_request, 0);
	if (sdma_request.bd_error) {
		printk("Error in SDMA transfer\n");
		status = __raw_readl(host->base + MMC_STATUS);
#ifdef CONFIG_MMC_DEBUG
		dump_status(__FUNCTION__, status);
#endif
		mxcmci_data_done(host, status);
		return;
	}

	DBG(2,"%s: Transfered bytes:%d\n", DRIVER_NAME, sdma_request.count);
#ifdef CONFIG_MMC_DEBUG
	nob = __raw_readl(host->base + MMC_REM_NOB);
	blk_size = __raw_readl(host->base + MMC_REM_BLK_SIZE);
	blk_len = __raw_readl(host->base + MMC_BLK_LEN);
#endif
	DBG(2,"%s: REM_NOB:%lu REM_BLK_SIZE:%lu\n", DRIVER_NAME, nob, blk_size);
	DBG(2,"Out.. %s\n", __FUNCTION__);
}
#endif

static int mxcmci_init_dma(struct mxcmci_host* host)
{
	int ret = 0; 
        host->dma = 0;
	host->dma_len = DMA_BUF_LEN;

	ret = mxc_request_dma(&host->dma, host->mmc->host_name);
        if (ret)
                goto err;

        /*
         * We need to allocate a special buffer in
         * order for ISA to be able to DMA to it.
         */
        host->dma_buffer = kmalloc(host->dma_len,
                GFP_NOIO | GFP_DMA | __GFP_REPEAT | __GFP_NOWARN);
        if (!host->dma_buffer) {
		ret = -ENOMEM;
                goto free;
	}

        /*
         * ISA DMA must be aligned on a 64k basis.
         */
        if ((host->dma_addr & 0xffff) != 0) {
                ret = -EFAULT;
		goto kfree;
	}
        /*
         * ISA cannot access memory above 16 MB.
         */
        else if (host->dma_addr >= 0x1000000) {
                ret = -EFAULT;
                goto kfree;
	}

        return ret;

kfree:
        /*
         * If we've gotten here then there is some kind of alignment bug
         */
        BUG_ON(1);

        kfree(host->dma_buffer);
        host->dma_buffer = NULL;

free:
        mxc_free_dma(host->dma);

err:
	return ret;
}

/*!
 * This function is called during the driver binding process. Based on the SDHC
 * module that is being probed this function adds the appropriate SDHC module
 * structure in the core driver.
 *
 * @param   dev   the device structure used to store device specific
 *                information that is used by the suspend, resume and remove
 *                functions.
 *
 * @return  The function returns 0 on successful registration and initialization
 *          of SDHC module. Otherwise returns specific error code.
 */
static int mxcmci_probe(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc;
	struct mxcmci_host *host = NULL;
	int ret = 0;
DBG(2,"mxcmci_probe: begin. id = 0x%x; slot_id = 0x%x  \n", pdev->id, slot_id);
	if (pdev->id != 0 && pdev->id != 1) {	//There is 2 SDHC
		printk(KERN_DEBUG
		       "mmxmci_probe: invalid controller selected\n");
DBG(2,"mxcmci_probe: invalid controller selected \n");
		return -ENODEV;
	}

	if (pdev->resource[0].flags != IORESOURCE_MEM
	    || pdev->resource[1].flags != IORESOURCE_IRQ) {
		printk(KERN_ERR "mmxmci_probe: invalid resource type\n");
		return -ENODEV;
	}

	if (!request_mem_region(pdev->resource[0].start,
				pdev->resource[0].end -
				pdev->resource[0].start + 1, pdev->name)) {
		printk(KERN_ERR "request_mem_region failed\n");
		return -EBUSY;
	}

	mmc = mmc_alloc_host(sizeof(struct mxcmci_host), dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto out;
	}

	mmc->ops = &mxcmci_ops;
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->max_phys_segs = NR_SG;
 	mmc->max_hw_segs = NR_SG; 
	mmc->max_sectors = 127;
	mmc->max_seg_size = 128 * 512; 

#ifdef CONFIG_MOT_FEAT_MEGASIM
	if (pdev->id == 0) 
		mmc->caps = MMC_CAP_4_BIT_DATA;
#else
	mmc->caps = MMC_CAP_4_BIT_DATA;
#endif
	host = mmc_priv(mmc);
	host->sdhc_err = 0;
	host->mmc = mmc;
	host->dma = -1;
	host->id = pdev->id;
	host->mxc_mmc_suspend_flag = 0;

	/* Default is card not inserted. */
	host->insert_debounce_counter = 0;
	host->remove_debounce_counter = 0;

	if (pdev->id == 0) {
		host->event_id = DMA_REQ_SDHC1;
		host->clock_id = SDHC1_CLK;
	} else {
		host->event_id = DMA_REQ_SDHC2;
		host->clock_id = SDHC2_CLK;
	}

	mmc->f_min = sdhc_get_min_clock(host->clock_id);
	mmc->f_max = sdhc_get_max_clock(host->clock_id);
	DBG(1,"SDHC:%d clock:%lu\n", pdev->id, mxc_get_clocks(host->clock_id));

	spin_lock_init(&host->lock);
	host->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->irq = platform_get_irq(pdev, 0);
	host->base = (void *)IO_ADDRESS(pdev->resource[0].start);

	if (!host->base) {
		ret = -ENOMEM;
		goto out;
	}

	dev_set_drvdata(dev, mmc);

	/* Initialize work queues for MMC detection and extraction */
	INIT_WORK(&host->switch_work, mxcmci_wq_handler, (void *)host);
	INIT_WORK(&host->card_del_work, mxcmci_remove_card, (void *)host);

	/* Initialize debounce timer */
	init_timer(&host->debounce_timer);
	host->debounce_timer.function = mxcmci_debounce_timer;
	host->debounce_timer.data = (unsigned long)host;

	init_timer(&host->cmd_timeout_timer);
	host->cmd_timeout_timer.function = cmd_timeout_timer_func;
	host->cmd_timeout_timer.data =(unsigned long )host;

	gpio_sdhc_active(pdev->id);
	
	mmc_turn_on_clock(mmc);

	if( pdev->id == 0 ) { //do this only on SDHC0
		ret = sdhc_intr_setup(pdev->id, host, mxcmci_gpio_irq);
		if ( ret ) 
			goto out1;
	}
	 
	mxcmci_softreset(host);
	
	mmc_turn_off_clock(mmc);
	
	if (__raw_readl(host->base + MMC_REV_NO) != SDHC_REV_NO) {
		printk("%s: wrong rev.no. 0x%08x. aborting.\n",
		       pdev->name, MMC_REV_NO);
		goto out2;
	}
	__raw_writel(READ_TO_VALUE, host->base + MMC_READ_TO);

	host->icntr = INT_CNTR_END_CMD_RES;

	__raw_writel(host->icntr, host->base + MMC_INT_CNTR);

	memcpy(mmc->host_name, pdev->name, 8);
#ifdef MXC_MMC_DMA_ENABLE
	host->dma = 0;
	if ((ret = mxcmci_init_dma(host)) < 0) {
		printk("%s: %s:err = %d\n", __FUNCTION__, host->mmc->host_name, ret);
		goto out2;
	}
#endif
	printk("%s: request irq no = %d\n", __FUNCTION__, host->irq);
	ret = request_irq(host->irq, mxcmci_irq, 0, pdev->name, host);
	if (ret) {
		goto out3;
	}

/*	if( pdev->id == 0 ){
#ifndef CONFIG_MACH_MXC91131EVB
		power_ic_set_transflash_voltage(3200);	
#endif
	}*/

	mmc_add_host(mmc);

#ifdef CONFIG_MOT_FEAT_MEGASIM
	if (pdev->id == 1) {
		/* for megasim interface */
		megasim_host = host;
#if 0 
		init_timer(&megasim_timer);
		megasim_timer.function = megasim_timer_handler;
		megasim_timer.data = (unsigned long)host;
		mod_timer(&megasim_timer, jiffies + (MXC_MMC_DEBOUNCE_POLL_DELAY_MSEC * HZ));
#endif
	}
#endif

	/* check whether cards be insert */
/*	if ( pdev->id == 0 && sdhc_find_card(host->id) != 0) {
		printk("card(%d) is not inserted, so cut power\n", host->id);
#ifndef CONFIG_MACH_MXC91131EVB
		power_ic_set_transflash_voltage(0);
#endif
	}*/

	printk(KERN_INFO "%s-%d found\n", pdev->name, pdev->id);
	
	return 0;

      out3:
	if (host) {
#ifdef MXC_MMC_DMA_ENABLE
		if (host->dma >= 0) {
			mxc_free_dma(host->dma);
		}
#endif
	}
      out2:
	sdhc_intr_destroy(pdev->id, host);
      out1:
	gpio_sdhc_inactive(pdev->id);
      out:
	printk("%s: Error in initializing....", pdev->name);
#ifdef CONFIG_MOT_FEAT_MEGASIM
	if (pdev->id == 1) {
		megasim_host = NULL;
	}
#endif
	if (mmc) {
		mmc_free_host(mmc);
	}
	release_mem_region(pdev->resource[0].start,
			   pdev->resource[0].end - pdev->resource[0].start + 1);
	return ret;
}

/*!
 * Dissociates the driver from the SDHC device. Removes the appropriate SDHC
 * module structure from the core driver.
 *
 * @param   dev   the device structure used to give information on which SDHC
 *                to remove
 *
 * @return  The function always returns 0.
 */
static int mxcmci_remove(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = dev_get_drvdata(dev);
	dev_set_drvdata(dev, NULL);

	if (mmc) {
		struct mxcmci_host *host = mmc_priv(mmc);

		mmc_remove_host(mmc);
		free_irq(host->irq, host);
		mxc_free_dma(host->dma);
		release_mem_region(pdev->resource[0].start,
				   pdev->resource[0].end -
				   pdev->resource[0].start + 1);
		mmc_free_host(mmc);
		gpio_sdhc_inactive(pdev->id);
		sdhc_intr_destroy(pdev->id, host);
	}

/*	if( pdev->id == 0 ){
#ifndef CONFIG_MACH_MXC91131EVB
		power_ic_set_transflash_voltage(0);
#endif
	}*/

#ifdef CONFIG_MOT_FEAT_MEGASIM
	megasim_host = NULL;
#endif
	return 0;
}

#ifdef CONFIG_PM

/*!
 * This function is called to put the SDHC in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device structure used to give information on which SDHC
 *                to suspend
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxcmci_suspend(struct device *dev, u32 state, u32 level)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = dev_get_drvdata(dev);
	struct mxcmci_host *host = mmc_priv(mmc);
	int ret = 0;
	DBG(2,"In.. %s\n", __FUNCTION__);

	if (mmc_busy_flag) {
		printk("mmcbus is busying now!\n");
		return -EBUSY;
	}

	if (mmc && level == SUSPEND_DISABLE) {
		host->mxc_mmc_suspend_flag = 1;
		//ret = mmc_suspend_host(mmc, state);
	} else if (level == SUSPEND_POWER_DOWN) {
		//mxc_clks_disable(host->clock_id);
		gpio_sdhc_inactive(pdev->id);
	}
	DBG(2,"Out.. %s\n", __FUNCTION__);
	return ret;
}

/*!
 * This function is called to bring the SDHC back from a low power state. Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device structure used to give information on which SDHC
 *                to resume
 * @param   level the stage in device resumption process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxcmci_resume(struct device *dev, u32 level)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = dev_get_drvdata(dev);
	struct mxcmci_host *host = mmc_priv(mmc);
	int ret = 0;
	DBG(2,"In.. %s\n", __FUNCTION__);
	/*
	 * Note that a card insertion interrupt will cause this
	 * driver to resume automatically.  In that case we won't
	 * actually have to do any work here.  Return success.
	 */
	if (!host->mxc_mmc_suspend_flag) {
		return 0;
	}

	if (mmc && level == RESUME_ENABLE) {
		/* 
		 * Resume will enter schedule() function and may block system. 
		 * According analysis, we consider the mmc_resume_host() is
		 * not necessary for mmc/sd wake up. Just remarked it.
		 */ 
		// ret = mmc_resume_host(mmc);
		host->mxc_mmc_suspend_flag = 0;
	} else if (level == RESUME_POWER_ON) {
		//mxc_clks_enable(host->clock_id);
		gpio_sdhc_active(pdev->id);
	}
	DBG(2,"Out.. %s\n", __FUNCTION__);
	return ret;
}
#else
#define mxcmci_suspend  NULL
#define mxcmci_resume   NULL
#endif				/* CONFIG_PM */

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxcmci_driver = {
	.name = "mxcmci",
	.bus = &platform_bus_type,
	.probe = mxcmci_probe,
	.remove = mxcmci_remove,
	.suspend = mxcmci_suspend,
	.resume = mxcmci_resume,
};

#ifdef CONFIG_MOT_FEAT_MEGASIM
DECLARE_MUTEX(megasim_restart_mutex);

int megasim_restart_mmc(unsigned int flag)
{
	int ret= 0;

	printk("Megasim request to %s mmc\n", flag ? "boot" : "halt");
	if (!megasim_host) {
		printk(KERN_ERR "Can not detect megasim device\n");
		return -ENODEV;
	}
        if(down_interruptible(&megasim_restart_mutex))
                return -EINTR;

	if (flag) {
		mod_timer(&megasim_host->debounce_timer,
                                jiffies + (MXC_MMC_DEBOUNCE_POLL_DELAY_MSEC * HZ / 1000));
	} else {
		/* Application requests to halt mmc */
		card_selected[megasim_host->id] = 0;
		mxcmci_remove_card((void *)megasim_host);
		megasim_host->mmc->ios.clock = 0;
		megasim_host->mmc->ios.power_mode = MMC_POWER_OFF;
	}

	up(&megasim_restart_mutex);

	return ret;
}

EXPORT_SYMBOL(megasim_restart_mmc);

#endif

/*!
 * This function is used to initialize the MMC/SD driver module. The function
 * registers the power management callback functions with the kernel and also
 * registers the MMC/SD callback functions with the core MMC/SD driver.
 *
 * @return  The function returns 0 on success and a non-zero value on failure.
 */
static int __init mxcmci_init(void)
{
	int ret = 0;
	printk(KERN_INFO "MXC MMC/SD driver\n");

	mmc_turn_on_clock = mxcsdhc_clockpll2_enable;
	mmc_turn_off_clock = mxcsdhc_clockpll2_disable;
#ifndef CONFIG_MACH_MXC91131EVB


#ifdef CONFIG_MOT_FEAT_MEGASIM
	init_MUTEX(&megasim_restart_mutex);
#endif

#ifdef  CONFIG_MACH_ELBA
        /* Sierra team have not support movinand, direct set power*/
        /* LIBkk54542 tracking for this issue */
        power_ic_set_reg_value(POWER_IC_REG_ATLAS_REG_SET_0, 6, 6, 3);

        /* Confirm regulator is enabled */
        power_ic_set_reg_value(POWER_IC_REG_ATLAS_REG_MODE_0, 12, 7, 3);
        power_ic_set_reg_value(POWER_IC_REG_ATLAS_REG_MODE_1, 18, 7, 3);

        udelay (1000);
#endif /* CONFIG_MACH_ELBA */

#else   /* CONFIG_MACH_MXC91131EVB is set*/
	
	/* Zeus Platform based power ic set voltage function should be invoked here */

#endif  /* End CONFIG_MACH_MXC91131EVB */

	ret = driver_register(&mxcmci_driver);
DBG(2,"MXC MMC/SD driver. ret = 0x%x \n",ret);
	return ret;
}

/*!
 * This function is used to cleanup all resources before the driver exits.
 */
static void __exit mxcmci_exit(void)
{
	driver_unregister(&mxcmci_driver);

/*#ifndef CONFIG_MACH_MXC91131EVB
	power_ic_set_transflash_voltage(0);
#else
	// Zeus Platform based power ic set voltage function should be invoked here
#endif
*/

}

module_init(mxcmci_init);
module_exit(mxcmci_exit);

MODULE_DESCRIPTION("MXC Multimedia Card Interface Driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
