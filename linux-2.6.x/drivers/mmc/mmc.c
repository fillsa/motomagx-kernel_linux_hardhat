/*
 *  linux/drivers/mmc/mmc.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *  SD support Copyright (C) 2004 Ian Molton, All Rights Reserved.
 *  SD support Copyright (C) 2005 Pierre Ossman, All Rights Reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 *  Cambridge, MA 02139, USA
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
/*  Copyright (C) 2006-2008 Motorola
 * Date		Author			Comment
 * 11/17/2006	Motorola		Support SDA 2.0
 * 11/28/2006   Motorola                Support MMCA 4.1 movinand 4-bit mode	
 * 03/08/2007   Motorola                Turn off sdhc clock when card is not present 	
 * 07/04/2007   Motorola                Add support for MC/MR
 * 09/28/2007   Motorola		Add timeout for command
 * 12/04/2007	Motorola		Cut power when it is a bad card
 * 03/25/2008	Motorola		Support 4bit for SDA2.0 cards
 * 07/10/2008	Motorola		Modify one typo
 * 09/23/2008   Motorola        	Support shredding card
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/err.h>
#include <asm/scatterlist.h>
#include <linux/scatterlist.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/protocol.h>
#include <linux/mpm.h>

#include "mmc.h"

#define CMD_RETRIES	3

/*
 * OCR Bit positions to 10s of Vdd mV.
 */
static const unsigned short mmc_ocr_bit_to_vdd[] = {
	150,	155,	160,	165,	170,	180,	190,	200,
	210,	220,	230,	240,	250,	260,	270,	280,
	290,	300,	310,	320,	330,	340,	350,	360
};

static const unsigned int tran_exp[] = {
	10000,		100000,		1000000,	10000000,
	0,		0,		0,		0
};

static const unsigned char tran_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int tacc_exp[] = {
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};


/**
 *	mmc_request_done - finish processing an MMC command
 *	@host: MMC host which completed command
 *	@mrq: MMC request which completed
 *
 *	MMC drivers should call this function when they have completed
 *	their processing of a command.  This should be called before the
 *	data part of the command has completed.
 */
void mmc_request_done(struct mmc_host *host, struct mmc_request *mrq)
{
	struct mmc_command *cmd = mrq->cmd;
	int err = mrq->cmd->error;
	DBG(3,"MMC: req done (%02x): %d: %08x %08x %08x %08x\n", cmd->opcode,
	    err, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);

	if (err && cmd->retries) {
		cmd->retries--;
		cmd->error = 0;
		host->ops->request(host, mrq);
	} else if (mrq->done) {
		mrq->done(mrq);
	}
}

EXPORT_SYMBOL(mmc_request_done);

/**
 *	mmc_start_request - start a command on a host
 *	@host: MMC host to start command on
 *	@mrq: MMC request to start
 *
 *	Queue a command on the specified host.  We expect the
 *	caller to be holding the host lock with interrupts disabled.
 */
void
mmc_start_request(struct mmc_host *host, struct mmc_request *mrq)
{
	DBG(3,"MMC: starting cmd %02x arg %08x flags %08x\n",
	    mrq->cmd->opcode, mrq->cmd->arg, mrq->cmd->flags);

	WARN_ON(host->card_busy == NULL);

	mrq->cmd->error = 0;
	mrq->cmd->mrq = mrq;
	if (mrq->data) {
		mrq->cmd->data = mrq->data;
		mrq->data->error = 0;
		mrq->data->mrq = mrq;
		if (mrq->stop) {
			mrq->data->stop = mrq->stop;
			mrq->stop->error = 0;
			mrq->stop->mrq = mrq;
		}
	}
	host->ops->request(host, mrq);
}

EXPORT_SYMBOL(mmc_start_request);

static void mmc_wait_done(struct mmc_request *mrq)
{
	complete(mrq->done_data);
}

int mmc_wait_for_req(struct mmc_host *host, struct mmc_request *mrq)
{
	struct mmc_card *card = host->card_selected;
	u32 opcode = mrq->cmd->opcode;
	DECLARE_COMPLETION(complete);

	if (card) {
		if (mmc_card_locked(card)) {
			/* don't send unacceptable command to locked card */
			if ( !(opcode <= MMC_SET_BLOCKLEN || opcode == MMC_APP_CMD
					|| opcode == SD_APP_OP_COND 
					|| opcode == MMC_LOCK_UNLOCK) ) {
				 mrq->cmd->error = MMC_ERR_TIMEOUT;
				 return 0;
			}
		}
	}

	mrq->done_data = &complete;
	mrq->done = mmc_wait_done;

	mmc_start_request(host, mrq);

	wait_for_completion(&complete);

	return 0;
}

EXPORT_SYMBOL(mmc_wait_for_req);

/**
 *	mmc_wait_for_cmd - start a command and wait for completion
 *	@host: MMC host to start command
 *	@cmd: MMC command to start
 *	@retries: maximum number of retries
 *
 *	Start a new MMC command for a host, and wait for the command
 *	to complete.  Return any error that occurred while the command
 *	was executing.  Do not attempt to parse the response.
 */
int mmc_wait_for_cmd(struct mmc_host *host, struct mmc_command *cmd, int retries)
{
	struct mmc_request mrq;

	BUG_ON(host->card_busy == NULL);

	memset(&mrq, 0, sizeof(struct mmc_request));

	memset(cmd->resp, 0, sizeof(cmd->resp));
	cmd->retries = retries;

	mrq.cmd = cmd;
	cmd->data = NULL;

	mmc_wait_for_req(host, &mrq);

	return cmd->error;
}

EXPORT_SYMBOL(mmc_wait_for_cmd);

/**
 *	mmc_wait_for_app_cmd - start an application command and wait for
 			       completion
 *	@host: MMC host to start command
 *	@rca: RCA to send MMC_APP_CMD to
 *	@cmd: MMC command to start
 *	@retries: maximum number of retries
 *
 *	Sends a MMC_APP_CMD, checks the card response, sends the command
 *	in the parameter and waits for it to complete. Return any error
 *	that occurred while the command was executing.  Do not attempt to
 *	parse the response.
 */
int mmc_wait_for_app_cmd(struct mmc_host *host, unsigned int rca,
	struct mmc_command *cmd, int retries)
{
	struct mmc_request mrq;
	struct mmc_command appcmd;

	int i, err;

	BUG_ON(host->card_busy == NULL);
	BUG_ON(retries < 0);

	err = MMC_ERR_INVALID;

	/*
	 * We have to resend MMC_APP_CMD for each attempt so
	 * we cannot use the retries field in mmc_command.
	 */
	for (i = 0;i <= retries;i++) {
		memset(&mrq, 0, sizeof(struct mmc_request));

		appcmd.opcode = MMC_APP_CMD;
		appcmd.arg = rca << 16;
		appcmd.flags = MMC_RSP_R1;
		appcmd.retries = 0;
		memset(appcmd.resp, 0, sizeof(appcmd.resp));
		appcmd.data = NULL;

		mrq.cmd = &appcmd;
		appcmd.data = NULL;

		mmc_wait_for_req(host, &mrq);

		if (appcmd.error) {
			err = appcmd.error;
			continue;
		}

		/* Check that card supported application commands */
		if (!(appcmd.resp[0] & R1_APP_CMD))
			return MMC_ERR_FAILED;

		memset(&mrq, 0, sizeof(struct mmc_request));

		memset(cmd->resp, 0, sizeof(cmd->resp));
		cmd->retries = 0;

		mrq.cmd = cmd;
		cmd->data = NULL;

		mmc_wait_for_req(host, &mrq);

		err = cmd->error;
		if (cmd->error == MMC_ERR_NONE)
			break;
	}

	return err;
}

EXPORT_SYMBOL(mmc_wait_for_app_cmd);

static int mmc_select_card(struct mmc_host *host, struct mmc_card *card);

/**
 *	__mmc_claim_host - exclusively claim a host
 *	@host: mmc host to claim
 *	@card: mmc card to claim host for
 *
 *	Claim a host for a set of operations.  If a valid card
 *	is passed and this wasn't the last card selected, select
 *	the card before returning.
 *
 *	Note: you should use mmc_card_claim_host or mmc_claim_host.
 */
int __mmc_claim_host(struct mmc_host *host, struct mmc_card *card)
{
	DECLARE_WAITQUEUE(wait, current);
	unsigned long flags;
	int err = 0;

	add_wait_queue(&host->wq, &wait);
	spin_lock_irqsave(&host->lock, flags);
	while (1) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (host->card_busy == NULL)
			break;
		spin_unlock_irqrestore(&host->lock, flags);
		schedule();
		spin_lock_irqsave(&host->lock, flags);
	}
	set_current_state(TASK_RUNNING);
	host->card_busy = card;
	spin_unlock_irqrestore(&host->lock, flags);
	remove_wait_queue(&host->wq, &wait);

	if (card != (void *)-1) {
		err = mmc_select_card(host, card);
		if (err != MMC_ERR_NONE)
			return err;
	}

	return err;
}

EXPORT_SYMBOL(__mmc_claim_host);

/**
 *	mmc_release_host - release a host
 *	@host: mmc host to release
 *
 *	Release a MMC host, allowing others to claim the host
 *	for their operations.
 */
void mmc_release_host(struct mmc_host *host)
{
	unsigned long flags;

	BUG_ON(host->card_busy == NULL);

	spin_lock_irqsave(&host->lock, flags);
	host->card_busy = NULL;
	spin_unlock_irqrestore(&host->lock, flags);

	wake_up(&host->wq);
}

EXPORT_SYMBOL(mmc_release_host);

static int mmc_select_card(struct mmc_host *host, struct mmc_card *card)
{
	int err;
	struct mmc_command cmd;
DBG(2,"%s \n", __FUNCTION__);
	BUG_ON(host->card_busy == NULL);

	if (host->card_selected == card)
		return MMC_ERR_NONE;

	host->card_selected = card;

	cmd.opcode = MMC_SELECT_CARD;
	cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_R1;

	err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
	if (err != MMC_ERR_NONE)
		return err;

	/*
	 * Default bus width is 1 bit.
	 */
	host->ios.bus_width = MMC_BUS_WIDTH_1;

	/*
	 * We can only change the bus width of the selected
	 * card so therefore we have to put the handling
	 * here.
	 */
	if (host->caps & MMC_CAP_4_BIT_DATA) {
		/*
		 * The card is in 1 bit mode by default so
		 * we only need to change if it supports the
		 * wider version.
		 */
		if ((mmc_card_sd(card) || mmc_card_hcsd(card))){	//SD
			if (card->scr.bus_widths & SD_SCR_BUS_WIDTH_4) {
				struct mmc_command cmd;

				cmd.opcode = SD_APP_SET_BUS_WIDTH;
				cmd.arg = SD_BUS_WIDTH_4;
				cmd.flags = MMC_RSP_R1;

				err = mmc_wait_for_app_cmd(host, card->rca, &cmd,
					CMD_RETRIES);
				if (err != MMC_ERR_NONE)
					return err;

				host->ios.bus_width = MMC_BUS_WIDTH_4;
				DBG(1,"SD: MMC_BUS_WIDTH_4 \n");
			}
		}else{ 			/* MMC v4.1 Set MMC bus width to 4 bit. */
			struct mmc_command cmd;
	                cmd.opcode = MMC_SWITCH;
        	        cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                	          (EXT_CSD_BUS_WIDTH << 16) |
                        	  (1 << 8) | /* 0:1bit, 1:4bit, 2:8bit */
	                          EXT_CSD_CMD_SET_NORMAL;
        	        cmd.flags = MMC_RSP_R1B;

                	err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
	                if (err != MMC_ERR_NONE) {
        	                printk("%s: failed to switch card to mmc 4 bit mode.\n",
                        	       mmc_hostname(card->host));
				return err;	
	                } 
                	mmc_card_set_4bitmode(card);
			host->ios.bus_width = MMC_BUS_WIDTH_4;
			DBG(1,"MMC: MMC_BUS_WIDTH_4 \n");
		}
	}

	host->ops->set_ios(host, &host->ios);

	return MMC_ERR_NONE;
}

/*
 * Ensure that no card is selected.
 */
static void mmc_deselect_cards(struct mmc_host *host)
{
	struct mmc_command cmd;

	if (host->card_selected) {
		host->card_selected = NULL;

		cmd.opcode = MMC_SELECT_CARD;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_NONE;

		mmc_wait_for_cmd(host, &cmd, 0);
	}
}


static inline void mmc_delay(unsigned int ms)
{
	if (ms < HZ / 1000) {
		yield();
		mdelay(ms);
	} else {
		msleep_interruptible (ms);
	}
}

/*
 * Mask off any voltages we don't support and select
 * the lowest voltage
 */
static u32 mmc_select_voltage(struct mmc_host *host, u32 ocr)
{
	int bit;

	ocr &= host->ocr_avail;

	bit = ffs(ocr);
	if (bit) {
		bit -= 1;

		ocr = 3 << bit;

		host->ios.vdd = bit;
		host->ops->set_ios(host, &host->ios);
	} else {
		ocr = 0;
	}

	return ocr;
}

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
static void mmc_decode_cid(struct mmc_card *card)
{
	u32 *resp = card->raw_cid;

	memset(&card->cid, 0, sizeof(struct mmc_cid));

	if (mmc_card_sd(card) || mmc_card_hcsd(card)) {
		/*
		 * SD doesn't currently have a version field so we will
		 * have to assume we can parse this.
		 */
		card->cid.manfid		= UNSTUFF_BITS(resp, 120, 8);
		card->cid.oemid			= UNSTUFF_BITS(resp, 104, 16);
		card->cid.prod_name[0]		= UNSTUFF_BITS(resp, 96, 8);
		card->cid.prod_name[1]		= UNSTUFF_BITS(resp, 88, 8);
		card->cid.prod_name[2]		= UNSTUFF_BITS(resp, 80, 8);
		card->cid.prod_name[3]		= UNSTUFF_BITS(resp, 72, 8);
		card->cid.prod_name[4]		= UNSTUFF_BITS(resp, 64, 8);
		card->cid.hwrev			= UNSTUFF_BITS(resp, 60, 4);
		card->cid.fwrev			= UNSTUFF_BITS(resp, 56, 4);
		card->cid.serial		= UNSTUFF_BITS(resp, 24, 32);
		card->cid.year			= UNSTUFF_BITS(resp, 12, 8);
		card->cid.month			= UNSTUFF_BITS(resp, 8, 4);

		card->cid.year += 2000; /* SD cards year offset */
	} else {
		/*
		 * The selection of the format here is based upon published
		 * specs from sandisk and from what people have reported.
		 */
		switch (card->csd.mmca_vsn) {
		case 0: /* MMC v1.0 - v1.2 */
		case 1: /* MMC v1.4 */
			card->cid.manfid	= UNSTUFF_BITS(resp, 104, 24);
			card->cid.prod_name[0]	= UNSTUFF_BITS(resp, 96, 8);
			card->cid.prod_name[1]	= UNSTUFF_BITS(resp, 88, 8);
			card->cid.prod_name[2]	= UNSTUFF_BITS(resp, 80, 8);
			card->cid.prod_name[3]	= UNSTUFF_BITS(resp, 72, 8);
			card->cid.prod_name[4]	= UNSTUFF_BITS(resp, 64, 8);
			card->cid.prod_name[5]	= UNSTUFF_BITS(resp, 56, 8);
			card->cid.prod_name[6]	= UNSTUFF_BITS(resp, 48, 8);
			card->cid.hwrev		= UNSTUFF_BITS(resp, 44, 4);
			card->cid.fwrev		= UNSTUFF_BITS(resp, 40, 4);
			card->cid.serial	= UNSTUFF_BITS(resp, 16, 24);
			card->cid.month		= UNSTUFF_BITS(resp, 12, 4);
			card->cid.year		= UNSTUFF_BITS(resp, 8, 4) + 1997;
			break;

		case 2: /* MMC v2.0 - v2.2 */
		case 3: /* MMC v3.1 - v3.3 */
		case 4: /* MMC v4.x ? */
			card->cid.manfid	= UNSTUFF_BITS(resp, 120, 8);
			card->cid.oemid		= UNSTUFF_BITS(resp, 104, 16);
			card->cid.prod_name[0]	= UNSTUFF_BITS(resp, 96, 8);
			card->cid.prod_name[1]	= UNSTUFF_BITS(resp, 88, 8);
			card->cid.prod_name[2]	= UNSTUFF_BITS(resp, 80, 8);
			card->cid.prod_name[3]	= UNSTUFF_BITS(resp, 72, 8);
			card->cid.prod_name[4]	= UNSTUFF_BITS(resp, 64, 8);
			card->cid.prod_name[5]	= UNSTUFF_BITS(resp, 56, 8);
			card->cid.serial	= UNSTUFF_BITS(resp, 16, 32);
			card->cid.month		= UNSTUFF_BITS(resp, 12, 4);
			card->cid.year		= UNSTUFF_BITS(resp, 8, 4) + 1997;
			break;

		default:
			printk("%s: card has unknown MMCA version %d\n",
				mmc_hostname(card->host), card->csd.mmca_vsn);
			mmc_card_set_bad(card);
			break;
		}
	}
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static void mmc_decode_csd(struct mmc_card *card)
{
	struct mmc_csd *csd = &card->csd;
	unsigned int e, m, num=0, csd_struct;
	u32 *resp = card->raw_csd;

	if (mmc_card_sd(card) || mmc_card_hcsd(card)) {
		csd_struct = UNSTUFF_BITS(resp, 126, 2);
#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
		if (csd_struct != 0 && csd_struct != 1) {
#else
		if (csd_struct != 0) {
#endif
			printk("%s: unrecognised CSD structure version %d\n",
				mmc_hostname(card->host), csd_struct);
			mmc_card_set_bad(card);
			return;
		}

		m = UNSTUFF_BITS(resp, 115, 4);
		e = UNSTUFF_BITS(resp, 112, 3);
		csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
		csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
		csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
		if (csd_struct == 0) {
			e = UNSTUFF_BITS(resp, 47, 3);
			m = UNSTUFF_BITS(resp, 62, 12);

			if( csd->read_blkbits > 9 ){
				num = csd->read_blkbits - 9;
				csd->read_blkbits = 9;
				if(num )
					m = m<<num;
			}
		
			csd->capacity	  = (1 + m) << (e + 2);
		} else {
			m = UNSTUFF_BITS(resp, 48, 22);
			csd->capacity = (m+1)*1024;
		}
#else
		e = UNSTUFF_BITS(resp, 47, 3);
		m = UNSTUFF_BITS(resp, 62, 12);
		csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);

		if( csd->read_blkbits > 9 ){
			num = csd->read_blkbits - 9;
			csd->read_blkbits = 9;
			if(num )
				m = m<<num;
		}

		csd->capacity     = ((1 + m) << (e + 2));
#endif
	} else {
		/*
		 * We only understand CSD structure v1.1 and v1.2.
		 * v1.2 has extra information in bits 15, 11 and 10.
		 */
		csd_struct = UNSTUFF_BITS(resp, 126, 2);
		if (csd_struct != 1 && csd_struct != 2) {
			printk("%s: unrecognised CSD structure version %d\n",
				mmc_hostname(card->host), csd_struct);
			mmc_card_set_bad(card);
			return;
		}

		csd->mmca_vsn	 = UNSTUFF_BITS(resp, 122, 4);
		m = UNSTUFF_BITS(resp, 115, 4);
		e = UNSTUFF_BITS(resp, 112, 3);
		csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
		csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

		e = UNSTUFF_BITS(resp, 47, 3);
		m = UNSTUFF_BITS(resp, 62, 12);
		csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);

		if( csd->read_blkbits > 9 ){
			num = csd->read_blkbits - 9;
			csd->read_blkbits = 9;
			if(num )
				m = m<<num;
		}

		csd->capacity	  = (1 + m) << (e + 2);
	}
DBG(1,"%s: raw_csd = 0x%x, max_dtr = %d, C_SIZE = %d, CS_MULT = %d, CAP = %u, BL_LEN = %d \n", __FUNCTION__, card->raw_csd[0], csd->max_dtr, m, e, csd->capacity, csd->read_blkbits );
}

/*
 * Given a 64-bit response, decode to our card SCR structure.
 */
static void mmc_decode_scr(struct mmc_card *card)
{
	struct sd_scr *scr = &card->scr;
	unsigned int scr_struct;
	u32 resp[4];

	BUG_ON(!(mmc_card_sd(card) || mmc_card_hcsd(card)));

	resp[3] = card->raw_scr[1];
	resp[2] = card->raw_scr[0];

	scr_struct = UNSTUFF_BITS(resp, 60, 4);
	if (scr_struct != 0) {
		printk("%s: unrecognised SCR structure version %d\n",
			mmc_hostname(card->host), scr_struct);
		mmc_card_set_bad(card);
		return;
	}

	scr->sda_vsn = UNSTUFF_BITS(resp, 56, 4);
	scr->bus_widths = UNSTUFF_BITS(resp, 48, 4);
}

/*
 * Locate a MMC card on this MMC host given a raw CID.
 */
static struct mmc_card *mmc_find_card(struct mmc_host *host, u32 *raw_cid)
{
	struct mmc_card *card;

	list_for_each_entry(card, &host->cards, node) {
		if (memcmp(card->raw_cid, raw_cid, sizeof(card->raw_cid)) == 0)
			return card;
	}
	return NULL;
}

/*
 * Allocate a new MMC card, and assign a unique RCA.
 */
static struct mmc_card *
mmc_alloc_card(struct mmc_host *host, u32 *raw_cid, unsigned int *frca)
{
	struct mmc_card *card, *c;
	unsigned int rca = *frca;

	card = kmalloc(sizeof(struct mmc_card), GFP_KERNEL);
	if (!card)
		return ERR_PTR(-ENOMEM);

	mmc_init_card(card, host);
	memcpy(card->raw_cid, raw_cid, sizeof(card->raw_cid));

 again:
	list_for_each_entry(c, &host->cards, node)
		if (c->rca == rca) {
			rca++;
			goto again;
		}

	card->rca = rca;

	*frca = rca;

	return card;
}

/*
 * Tell attached cards to go to IDLE state
 */
static void mmc_idle_cards(struct mmc_host *host)
{
	struct mmc_command cmd;

	host->ios.chip_select = MMC_CS_HIGH;
	host->ops->set_ios(host, &host->ios);

	mmc_delay(1);

	cmd.opcode = MMC_GO_IDLE_STATE;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_NONE;

	mmc_wait_for_cmd(host, &cmd, 0);

	mmc_delay(1);

	host->ios.chip_select = MMC_CS_DONTCARE;
	host->ops->set_ios(host, &host->ios);

	mmc_delay(1);
}

/*
 * Apply power to the MMC stack.
 */
static void mmc_power_up(struct mmc_host *host)
{
	int bit = fls(host->ocr_avail) - 1;

	host->ios.vdd = bit;
	host->ios.clock = host->f_min;
	host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
	host->ios.chip_select = MMC_CS_DONTCARE;
	host->ios.power_mode = MMC_POWER_UP;
	host->ios.bus_width = MMC_BUS_WIDTH_1;
	host->ops->set_ios(host, &host->ios);

	mmc_delay(1);

	host->ios.power_mode = MMC_POWER_ON;
	host->ops->set_ios(host, &host->ios);

	mmc_delay(2);
}

static void mmc_power_off(struct mmc_host *host)
{
	host->ios.clock = 0;
	host->ios.vdd = 0;
	host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
	host->ios.chip_select = MMC_CS_DONTCARE;
	host->ios.power_mode = MMC_POWER_OFF;
	host->ios.bus_width = MMC_BUS_WIDTH_1;
	host->ops->set_ios(host, &host->ios);
}

static int mmc_send_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr)
{
	struct mmc_command cmd;
	int i, err = 0;

	cmd.opcode = MMC_SEND_OP_COND;
	cmd.arg = ocr;
	cmd.flags = MMC_RSP_R3;

	for (i = 100; i; i--) {
		err = mmc_wait_for_cmd(host, &cmd, 0);
		if (err != MMC_ERR_NONE)
			break;

		if (cmd.resp[0] & MMC_CARD_BUSY || ocr == 0)
			break;
		mmc_delay(1);
		err = MMC_ERR_TIMEOUT;

		mmc_delay(10);
	}

	if (rocr)
		*rocr = cmd.resp[0];

	return err;
}

static int mmc_send_app_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr)
{
	struct mmc_command cmd;
	int i, err = 0;

	cmd.opcode = SD_APP_OP_COND;
	cmd.arg = ocr;
	cmd.flags = MMC_RSP_R3;

	for (i = 100; i; i--) {
		err = mmc_wait_for_app_cmd(host, 0, &cmd, CMD_RETRIES);
		if (err != MMC_ERR_NONE)
			break;

		if (cmd.resp[0] & MMC_CARD_BUSY || ocr == 0)
			break;

		err = MMC_ERR_TIMEOUT;

		mmc_delay(10);
	}

	if (rocr)
		*rocr = cmd.resp[0];
	
	return err;
}

#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
static int mmc_send_if_cond(struct mmc_host *host, u32 vhs)
{
	struct mmc_command cmd;
	int err = MMC_ERR_NONE;

	cmd.opcode = SD_SEND_IF_COND;
	cmd.arg = ((vhs & 0xF) << 8) | 0x000000AA;
	cmd.flags = MMC_RSP_R7;

	err = mmc_wait_for_cmd(host, &cmd, 0);
	if (err != MMC_ERR_NONE)
		return err;
	if (cmd.resp[0] != cmd.arg)
		err = MMC_ERR_INVALID;

	return err;
}
#endif

/*
 * Discover cards by requesting their CID.  If this command
 * times out, it is not an error; there are no further cards
 * to be discovered.  Add new cards to the list.
 *
 * Create a mmc_card entry for each discovered card, assigning
 * it an RCA, and save the raw CID for decoding later.
 */
#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
static void mmc_discover_cards(struct mmc_host *host, int high_capacity)
#else
static void mmc_discover_cards(struct mmc_host *host)
#endif
{
	struct mmc_card *card;
	unsigned int first_rca = 1, err;

	while (1) {
		struct mmc_command cmd;

		cmd.opcode = MMC_ALL_SEND_CID;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R2;

		err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
		if (err == MMC_ERR_TIMEOUT) {
			err = MMC_ERR_NONE;
			break;
		}
		if (err != MMC_ERR_NONE) {
			printk(KERN_ERR "%s: error requesting CID: %d\n",
				mmc_hostname(host), err);
			break;
		}

		card = mmc_find_card(host, cmd.resp);
		if (!card) {
			card = mmc_alloc_card(host, cmd.resp, &first_rca);
			if (IS_ERR(card)) {
				err = PTR_ERR(card);
				break;
			}
			list_add(&card->node, &host->cards);
		}

		card->state &= ~MMC_STATE_DEAD;

#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
		if (host->mode == MMC_MODE_SD || host->mode == MMC_MODE_SD2) {
			if (high_capacity) {
				mmc_card_set_hcsd(card);
			} else {
				mmc_card_set_sd(card);
			}
#else
		if (host->mode == MMC_MODE_SD) {
			mmc_card_set_sd(card);
#endif
			cmd.opcode = SD_SEND_RELATIVE_ADDR;
			cmd.arg = 0;
			cmd.flags = MMC_RSP_R1;

			err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
			if (err != MMC_ERR_NONE)
				mmc_card_set_dead(card);
			else {
				card->rca = cmd.resp[0] >> 16;

				if (!host->ops->get_ro) {
					printk(KERN_WARNING "%s: host does not "
						"support reading read-only "
						"switch. assuming write-enable.\n",
						mmc_hostname(host));
				} else {
					if (host->ops->get_ro(host))
						mmc_card_set_readonly(card);
				}
			}
		} else {
			cmd.opcode = MMC_SET_RELATIVE_ADDR;
			cmd.arg = card->rca << 16;
			cmd.flags = MMC_RSP_R1;

			err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
			if (err != MMC_ERR_NONE)
				mmc_card_set_dead(card);
		}

		/* check whether card is locked by passwd */
		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1;

		err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
		if (err == MMC_ERR_NONE) {
			if (cmd.resp[0] & R1_CARD_IS_LOCKED) {
				printk(KERN_INFO "Card is locked(0x%x)\n", cmd.resp[0]);
				mmc_card_set_locked(card);	
			}
		}
	}
}

static void mmc_read_csds(struct mmc_host *host)
{
	struct mmc_card *card;

	list_for_each_entry(card, &host->cards, node) {
		struct mmc_command cmd;
		int err;

		if (card->state & (MMC_STATE_DEAD|MMC_STATE_PRESENT))
			continue;

		cmd.opcode = MMC_SEND_CSD;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R2;

		err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
		if (err != MMC_ERR_NONE) {
			mmc_card_set_dead(card);
			continue;
		}

		memcpy(card->raw_csd, cmd.resp, sizeof(card->raw_csd));

		mmc_decode_csd(card);
		mmc_decode_cid(card);
	}
}

static void mmc_read_scrs(struct mmc_host *host)
{
	int err;
	struct mmc_card *card;
	
	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_data data;

	struct scatterlist sg;

	list_for_each_entry(card, &host->cards, node) {
		if (card->state & (MMC_STATE_DEAD|MMC_STATE_PRESENT|MMC_STATE_LOCKED))
			continue;
		if (!(mmc_card_sd(card) || mmc_card_hcsd(card)))
			continue;

		err = mmc_select_card(host, card);
		if (err != MMC_ERR_NONE) {
			mmc_card_set_dead(card);
			continue;
		}

		memset(&cmd, 0, sizeof(struct mmc_command));

		cmd.opcode = MMC_APP_CMD;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1;

		err = mmc_wait_for_cmd(host, &cmd, 0);
		if ((err != MMC_ERR_NONE) || !(cmd.resp[0] & R1_APP_CMD)) {
			mmc_card_set_dead(card);
			continue;
		}

		memset(&cmd, 0, sizeof(struct mmc_command));

		cmd.opcode = SD_APP_SEND_SCR;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1;

		memset(&data, 0, sizeof(struct mmc_data));

		data.timeout_ns = card->csd.tacc_ns * 10;
		data.timeout_clks = card->csd.tacc_clks * 10;
		data.blksz_bits = 3;
		data.blocks = 1;
		data.flags = MMC_DATA_READ;
		data.sg = &sg;
		data.sg_len = 1;

		memset(&mrq, 0, sizeof(struct mmc_request));

		mrq.cmd = &cmd;
		mrq.data = &data;

		sg_init_one(&sg, (u8*)card->raw_scr, 8);

		err = mmc_wait_for_req(host, &mrq);
		if (cmd.error != MMC_ERR_NONE) {
			mmc_card_set_dead(card);
			continue;
		}

		card->raw_scr[0] = ntohl(card->raw_scr[0]);
		card->raw_scr[1] = ntohl(card->raw_scr[1]);

		mmc_decode_scr(card);
	}

	mmc_deselect_cards(host);
}
/*
* read and process mmc 4.1 EXT_CSD infomation
*/
static void mmc_process_ext_csds(struct mmc_host *host)
{
	int err;
	struct mmc_card *card;

	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_data data;

	struct scatterlist sg;

	/*
	 * As the ext_csd is so large and mostly unused, we don't store the
	 * raw block in mmc_card.
	 */
	u8 ext_csd[512];

	list_for_each_entry(card, &host->cards, node) {
		if (card->state & (MMC_STATE_DEAD|MMC_STATE_PRESENT))
			continue;
		if (mmc_card_sd(card))
			continue;
		if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
			continue;

		err = mmc_select_card(host, card);
		if (err != MMC_ERR_NONE) {
			mmc_card_set_dead(card);
			continue;
		}

DBG(1,"process ext csd HS BIT4 \n");
		memset(&cmd, 0, sizeof(struct mmc_command));

		cmd.opcode = MMC_SEND_EXT_CSD;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1;

		memset(&data, 0, sizeof(struct mmc_data));

		data.timeout_ns = card->csd.tacc_ns * 10;
		data.timeout_clks = card->csd.tacc_clks * 10;
		data.blksz_bits = 9;
		data.blocks = 1;
		data.flags = MMC_DATA_READ;
		data.sg = &sg;
		data.sg_len = 1;

		memset(&mrq, 0, sizeof(struct mmc_request));

		mrq.cmd = &cmd;
		mrq.data = &data;

		sg_init_one(&sg, ext_csd, 512);

		mmc_wait_for_req(host, &mrq);

		if (cmd.error != MMC_ERR_NONE || data.error != MMC_ERR_NONE) {
			mmc_card_set_dead(card);
			continue;
		}

		/*
		 * Motorola - cause scma11 sdhc clock only support upto 24Hz , default mmc clock is 20Hz 
		 * movinand highspeed mode support upto 26Hz, by test movinand default mode can work with
		 * 24Hz sdhc, and have a better speed. so we choose this method
		 */
		switch (ext_csd[EXT_CSD_CARD_TYPE]) {
		case EXT_CSD_CARD_TYPE_52 | EXT_CSD_CARD_TYPE_26:
			card->ext_csd.hs_max_dtr = 52000000;
			mmc_card_set_highspeed(card);
			break;
		case EXT_CSD_CARD_TYPE_26:
			card->ext_csd.hs_max_dtr = 26000000;
			mmc_card_set_highspeed(card); 
			break;
		default:
			/* MMC v4 spec says this cannot happen */
			printk("%s: card is mmc v4 but doesn't support "
			       "any high-speed modes.\n",
				mmc_hostname(card->host));
			mmc_card_set_bad(card);
			/* printk a warning */
			continue;
		}
	}

	mmc_deselect_cards(host);
}

static unsigned int mmc_calculate_clock(struct mmc_host *host)
{
	struct mmc_card *card;
	unsigned int max_dtr = host->f_max;

	list_for_each_entry(card, &host->cards, node){
		if( !mmc_card_dead(card)){
			DBG(1,"f_max %d, csd.max_dtr %d\n", max_dtr, card->csd.max_dtr);
			if ( mmc_card_highspeed(card)&& mmc_card_sd(card) ){
				DBG(1,"high speed && sd card \n");				
			}else if ( mmc_card_highspeed(card) && !mmc_card_sd(card) ){
				DBG(1,"high speed && mmc card \n");				
				if (max_dtr > card->ext_csd.hs_max_dtr)
					max_dtr = card->ext_csd.hs_max_dtr;
			}else{
				DBG(1,"low speed sd card or mmc card\n");				
				if ( max_dtr > card->csd.max_dtr)
					max_dtr = card->csd.max_dtr;
			}
		}
	}
	DBG(1,"MMC: selected %d.%03dMHz transfer rate\n",
	    max_dtr / 1000000, (max_dtr / 1000) % 1000);

	return max_dtr;
}

/*
 * Check whether cards we already know about are still present.
 * We do this by requesting status, and checking whether a card
 * responds.
 *
 * A request for status does not cause a state change in data
 * transfer mode.
 */
static void mmc_check_cards(struct mmc_host *host)
{
	struct list_head *l, *n;

	mmc_deselect_cards(host);

	list_for_each_safe(l, n, &host->cards) {
		struct mmc_card *card = mmc_list_to_card(l);
		struct mmc_command cmd;
		int err;

		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1;

		err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
		if (err == MMC_ERR_NONE)
			continue;

		mmc_card_set_dead(card);
	}
}

static void mmc_setup(struct mmc_host *host)
{
#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
	int err;
	u32 ocr;
DBG(2,"mmc_setup...");
	if (host->ios.power_mode != MMC_POWER_ON) {
DBG(2,"...POWER_NOT_ON \n");
		host->mode = MMC_MODE_SD2;

		mmc_power_up(host);
		mmc_idle_cards(host);

		err = mmc_send_if_cond(host, 0x1);

		/* If the card doesn't respond to CMD8 (SEND_IF_COND),
		 * it is NOT Ver2.0 or later SD Memory Card
		 */
		if (err == MMC_ERR_INVALID) {
			printk("Invalid response to CMD8, unusable card.\n");
			return;
		} else if (err != MMC_ERR_NONE) {
			printk("No response to CMD8, not Ver2.00 or later card\n");
			host->mode = MMC_MODE_SD;
		}
#else
DBG(2,"mmc_setup...");
	if (host->ios.power_mode != MMC_POWER_ON) {
		int err;
		u32 ocr;
DBG(2,"...POWER_NOT_ON \n");
		host->mode = MMC_MODE_SD;

		mmc_power_up(host);
		mmc_idle_cards(host);
#endif
		host->card_selected = NULL;

		err = mmc_send_app_op_cond(host, 0, &ocr);

		/*
		 * If we fail to detect any SD cards then try
		 * searching for MMC cards.
		 */
		if (err != MMC_ERR_NONE) {
			host->mode = MMC_MODE_MMC;

			err = mmc_send_op_cond(host, 0, &ocr);
			if (err != MMC_ERR_NONE)
				return;
		}

		host->last_ocr = ocr;
		host->ocr = mmc_select_voltage(host, ocr);

		/*
		 * Since we're changing the OCR value, we seem to
		 * need to tell some cards to go back to the idle
		 * state.  We wait 1ms to give cards time to
		 * respond.
		 * Ver 2.0 SD card will not respond to the
		 * succeeding Operation Condition Validation
		 * command if the IDLE command is sent once more.
		 */
#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
		if (host->ocr && (host->mode != MMC_MODE_SD2))
#else
		if (host->ocr)
#endif
			mmc_idle_cards(host);
	} else {
DBG(2,"...POWER_ON \n");
		host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
		host->ios.clock = host->f_min;
		host->ops->set_ios(host, &host->ios);

		/*
		 * We should remember the OCR mask from the existing
		 * cards, and detect the new cards OCR mask, combine
		 * the two and re-select the VDD.  However, if we do
		 * change VDD, we should do an idle, and then do a
		 * full re-initialisation.  We would need to notify
		 * drivers so that they can re-setup the cards as
		 * well, while keeping their queues at bay.
		 *
		 * For the moment, we take the easy way out - if the
		 * new cards don't like our currently selected VDD,
		 * they drop off the bus.
		 */
	}

	if (host->ocr == 0)
		return;

	/*
	 * Send the selected OCR multiple times... until the cards
	 * all get the idea that they should be ready for CMD2.
	 * (My SanDisk card seems to need this.)
	 */

#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
	if (host->mode == MMC_MODE_SD)
		err = mmc_send_app_op_cond(host, host->ocr, &ocr);
	else if (host->mode == MMC_MODE_SD2) {
		host->ocr |= SD_HOST_HCS;
		err = mmc_send_app_op_cond(host, host->ocr, &ocr);
	} else
		err = mmc_send_op_cond(host, host->ocr, &ocr);

	if (err != MMC_ERR_NONE) {
		printk(KERN_ERR "%s: error validating operating condition: %d\n",
		      mmc_hostname(host), err);
		return;
	}

	mmc_discover_cards(host, ocr&SD_CARD_CAP_HIGH ? 1 : 0);
#else
	if (host->mode == MMC_MODE_SD)
		mmc_send_app_op_cond(host, host->ocr, NULL);
	else
		mmc_send_op_cond(host, host->ocr, NULL);

	mmc_discover_cards(host);
#endif

	/*
	 * Ok, now switch to push-pull mode.
	 */
	host->ios.bus_mode = MMC_BUSMODE_PUSHPULL;
	host->ops->set_ios(host, &host->ios);

	mmc_read_csds(host);
#ifdef CONFIG_MOT_FEAT_MMCSD_HCSD
	if (host->mode == MMC_MODE_SD || host->mode == MMC_MODE_SD2)
#else
	if (host->mode == MMC_MODE_SD)
#endif
		mmc_read_scrs(host);
	else 
		mmc_process_ext_csds(host);
}

/**
 *	mmc_detect_change - process change of state on a MMC socket
 *	@host: host which changed state.
 *	@delay: optional delay to wait before detection (jiffies)
 *
 *	All we know is that card(s) have been inserted or removed
 *	from the socket(s).  We don't know which socket or cards.
 */
void mmc_detect_change(struct mmc_host *host, unsigned long delay)
{
	if (delay)
		schedule_delayed_work(&host->detect, delay);
	else
		schedule_work(&host->detect);
}

EXPORT_SYMBOL(mmc_detect_change);

extern void mmc_clear_busy_flag(void);

static void mmc_rescan(void *data)
{
	struct mmc_host *host = data;
	struct list_head *l, *n;
DBG(2,"mmc_rescan: before claim_host~~~~ \n");
	mmc_claim_host(host);

	if (host->ios.power_mode == MMC_POWER_ON)
		mmc_check_cards(host);

	mmc_setup(host);

	if (!list_empty(&host->cards)) {
		/*
		 * (Re-)calculate the fastest clock rate which the
		 * attached cards and the host support.
		 */
		host->ios.clock = mmc_calculate_clock(host);
		host->ops->set_ios(host, &host->ios);
	}

	mmc_release_host(host);

	list_for_each_safe(l, n, &host->cards) {
		struct mmc_card *card = mmc_list_to_card(l);

		/*
		 * If this is a new and good card, register it.
		 */
		if (!mmc_card_present(card) && !mmc_card_dead(card) && !mmc_card_bad(card)) {
			if (mmc_register_card(card))
				mmc_card_set_dead(card);
			else
				mmc_card_set_present(card);
		}

		/*
		 * If this card is dead, destroy it.
		 */
		if (mmc_card_dead(card) || mmc_card_bad(card) ) {
			printk("mxclay: card bad list del \n");
			list_del(&card->node);
			mmc_remove_card(card);
		}
	}

	/*
	 * If we discover that there are no cards on the
	 * bus, turn off the clock and power down.
	 */
	if (list_empty(&host->cards))
		mmc_power_off(host);

	/* for the application layers sufficient time 
	 * to respond to the hotplug event.
	 */
	mpm_handle_ioi();
	mmc_clear_busy_flag();
	DBG(2,"rescan out...\n");
}


int mmc_erase_allcard(struct mmc_card * card)
{
	struct mmc_host *host = card->host;
	struct mmc_command cmd;
	int err = 0, i=0, j=0;
	printk("mxclay: begin to erase all card \n");
	mmc_claim_host(host);

	cmd.opcode = MMC_ERASE_BLK_START;
	cmd.arg = 0; 
	cmd.flags = MMC_RSP_R1; 

	err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
	if (err != MMC_ERR_NONE){
		printk("mxclay: do erase blk start 0 err is %d\n", err);
		return err;
	}

	cmd.opcode = MMC_ERASE_BLK_END;
	cmd.arg = card->csd.capacity * 512; 
	cmd.flags = MMC_RSP_R1;

	err = mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
	if (err != MMC_ERR_NONE){
		printk("mxclay: do erase blk end %d err is %d\n", card->csd.capacity, err);
		return err;
	}
	printk("mxclay: erase capacity is %d \n", cmd.arg);

	cmd.opcode = MMC_ERASE;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_R1B;

	err =  mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
	if (err != MMC_ERR_NONE){
		printk("mxclay: do erase err %d \n", err);
		return err;
	}

	printk("done erase\n");

	for( i=0; i< 130; i++)
	{
		for( j=0; j<10; j++)
			mmc_delay(100);
		printk("mxclay: do sleep %d\n", i);
	}

	mmc_release_host(host);
	printk("mxclay: end of erase all card \n");
	return err;
}
EXPORT_SYMBOL(mmc_erase_allcard);

/**
 *	mmc_alloc_host - initialise the per-host structure.
 *	@extra: sizeof private data structure
 *	@dev: pointer to host device model structure
 *
 *	Initialise the per-host structure.
 */
struct mmc_host *mmc_alloc_host(int extra, struct device *dev)
{
	struct mmc_host *host;

	host = mmc_alloc_host_sysfs(extra, dev);
	if (host) {
		spin_lock_init(&host->lock);
		init_waitqueue_head(&host->wq);
		INIT_LIST_HEAD(&host->cards);
		INIT_WORK(&host->detect, mmc_rescan, host);

		/*
		 * By default, hosts do not support SGIO or large requests.
		 * They have to set these according to their abilities.
		 */
		host->max_hw_segs = 1;
		host->max_phys_segs = 1;
		host->max_sectors = 1 << (PAGE_CACHE_SHIFT - 9);
		host->max_seg_size = PAGE_CACHE_SIZE;
	}

	return host;
}

EXPORT_SYMBOL(mmc_alloc_host);

/**
 *	mmc_add_host - initialise host hardware
 *	@host: mmc host
 */
int mmc_add_host(struct mmc_host *host)
{
	int ret;
DBG(2,"mmc_add_host: begin\n");
	ret = mmc_add_host_sysfs(host);
	if (ret == 0) {
DBG(2,"mmc_add_host: ret=0\n");
		mmc_power_off(host);
		mmc_detect_change(host, 0);
	}

	return ret;
}

EXPORT_SYMBOL(mmc_add_host);

/**
 *	mmc_remove_host - remove host hardware
 *	@host: mmc host
 *
 *	Unregister and remove all cards associated with this host,
 *	and power down the MMC bus.
 */
void mmc_remove_host(struct mmc_host *host)
{
	struct list_head *l, *n;

	list_for_each_safe(l, n, &host->cards) {
		struct mmc_card *card = mmc_list_to_card(l);

		mmc_remove_card(card);
	}

	mmc_power_off(host);
	mmc_remove_host_sysfs(host);
}

EXPORT_SYMBOL(mmc_remove_host);

/**
 *	mmc_free_host - free the host structure
 *	@host: mmc host
 *
 *	Free the host once all references to it have been dropped.
 */
void mmc_free_host(struct mmc_host *host)
{
	flush_scheduled_work();
	mmc_free_host_sysfs(host);
}

EXPORT_SYMBOL(mmc_free_host);

#ifdef CONFIG_PM

/**
 *	mmc_suspend_host - suspend a host
 *	@host: mmc host
 *	@state: suspend mode (PM_SUSPEND_xxx)
 */
int mmc_suspend_host(struct mmc_host *host, u32 state)
{
	mmc_claim_host(host);
	mmc_deselect_cards(host);
	mmc_power_off(host);
	mmc_release_host(host);

	return 0;
}

EXPORT_SYMBOL(mmc_suspend_host);

/**
 *	mmc_resume_host - resume a previously suspended host
 *	@host: mmc host
 */
int mmc_resume_host(struct mmc_host *host)
{
	mmc_rescan(host);

	return 0;
}

EXPORT_SYMBOL(mmc_resume_host);

#endif

MODULE_LICENSE("GPL");
