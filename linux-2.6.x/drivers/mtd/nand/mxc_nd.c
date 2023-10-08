/*
 *
 * Copyright (C) 2005-2008 Motorola, Inc.
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * ChangeLog:
 *(mm-dd-yyyy)  Author    Comment
 * 06-10-2005   Motorola  implemented CONFIG_MOT_FEAT_MTD_FS feature.
 *			  added support to use string name for both mtd 
 *			  char and block device.
 *
 * 11-15-2005	Motorola  implemented CONFIG_MOT_FEAT_NAND_AUTO_DETECT feature.
 *			  added support for detecting on-board nand chip type,
 *			  setup NFC clock and NAND page_size & data_width config 
 *			  in CRM-COM register base upon NAND device manufacturer 
 *			  and cip Id.
 *
 * 04-25-2006	Motorola  implemented CONFIG_MOT_FEAT_NANDECC_TEST feature.
 *			  added NAND ECC 1-bit error injection capability to board driver.
 *
 * 06-07-2006	Motorola  implemented CONFIG_MOT_FEAT_LPNAND_SUPPORT feature.
 *			  added NAND large page support functionalities to board driver.
 *
 * 08-20-2006	Motorola  implemented CONFIG_MOT_FEAT_STM90NM feature.
 *			  added support for specail STM 90nm nand device.
 * 
 * 11-27-2006   Motorola  added interrupt driven functionalities to NAND driver.
 * 11-28-2006  Motorola disables NFC clock when the chip is not used.
 * 12-15-2006  Motorola doesn't check ecc for the main area ecc if the upper layer
 *             asked to read the OOB area only.
 * 01-11-2007  Motorola  Correct the NFC_CLK divisors (added 1 to each)
 * 04-25-2007  Motorola  Prevent disabling the nfc clock for the first 
 *                       60 seconds of uptime.
 * 05-21-2007  Motorola  update NFC_CLK dividsors to be 4 for Toshiba and STM parts.
 * 10-10-2007  Motorola  Add mpm_hanle_ioi to delay phone goto DSM when read from nand.
 * 10-18-2007  Motorola  Add mpm_hanle_ioi to delay phone goto DSM when read from nand.
 * 02-26-2008  Motorola  Remove mpm_handle_ioi.
 *                       Add busy flag for suspend rejection and mpm advise interface.
 * 02-29-2008  Motorola  Remove mpm_handle_ioi.
 *                       Add mpm advice mechanism for dynamic PM.
 * 11-13-2008  Motorola  update NFC_CLK divisors to be 4 for Hynix part.
 */


/*
 * Notes on Interrupt support:
 * 
 * The original implementation of the MXC NAND driver polled for completion of 
 * all NAND operations. Most NAND operations take on the order 10-20usec to 
 * complete so it's a bit of a toss-up as to whether or not it makes sense to 
 * just poll for the operation to complete OR allow the system to get some 
 * other work done while waiting for a completion interrupt. However, some 
 * operations (example: erase) take significantly longer than 10-20usecs and 
 * in these cases it certainly makes sense to get other work done while 
 * waiting for a completion interrupt. 
 *
 * Interrupt support was added by creating a third parameter to wait_op_done() 
 * which indicates whether to perform the operation synchronously (by
 * polling) or asynchronously (by using a processor interrupt). This way we can
 * selectively choose which NAND operations are synchronous vs asynchronous.
 * 
 * Below is the list of NAND commands/operations and the method used for each:
 * READ_ID - poll
 * STATUS - poll
 * RESET - interrupt
 * READ0 - poll
 * READOOB - poll
 * SEQIN - poll
 * PAGEPROG - interrupt
 * ERASE1 - poll
 * ERASE2 - interrupt
 * address cycles - poll
 * READSTART (lp only) - interrupt
 * read data transfer from flash to NFC - interrupt
 * write data transfer from NFC to flash - poll(lp), interrupt(sp)
 *
 * Interrupt considerations on large page:
 * - Since large page reads are done as four separate 512 byte reads, special
 * handling is required. To support this in interrupt mode, the first read
 * is issued from send_read_page_lp() but the next three page reads are 
 * initiated from the interrupt routine. 
 * - Write data transfer from the NFC to the nand is polled so a similar 
 * procedure is not required. There's no particular reason why write's 
 * couldn't be done the same way as reads. 
 * 12-15-2006  Motorola doesn't check ecc for the main area ecc if the upper layer
 *             asked to read the OOB area only.
 *             
 */

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/mtd/partitions.h>
#include <linux/mpm.h>
#include <asm/mach/flash.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
#include "mxc_nd.h"

#ifdef CONFIG_MOT_FEAT_STM90NM
#include <asm/boardrev.h>
#endif

#ifdef CONFIG_MOT_FEAT_KPANIC
extern int kpanic_in_progress;
#endif

#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
#define IS_LPNAND(mtd) ((mtd)->oobblock > 512)
#endif

#ifdef CONFIG_MOT_FEAT_NAND_AUTO_DETECT
static int nand_maf_id;
static int nand_dev_id;
#endif

#ifdef CONFIG_MOT_FEAT_NANDECC_TEST
int nand_ecc_flag = MTD_NANDECC_AUTOPLACE;
#endif

/*!
 * Number of static partitions on NAND Flash.
 */
#define NUM_PARTITIONS    (sizeof(partition_info)/sizeof(struct mtd_partition))

#define DVR_VER "2.0"

struct mxc_mtd_s {
	struct mtd_info mtd;
	struct nand_chip nand;
	struct mtd_partition *parts;
	struct device *dev;
};

static struct mxc_mtd_s *mxc_nand_data = NULL;

/*
 * Define delays in microsec for NAND device operations
 */
#define TROP_US_DELAY   20000
/*
 * Macros to get byte and bit positions of ECC
 */
#define COLPOS(x)  ((x) >> 3)
#define BITPOS(x) ((x)& 0xf)

/* Define single bit Error positions in Main & Spare area */
#define MAIN_SINGLEBIT_ERROR 0x4
#define SPARE_SINGLEBIT_ERROR 0x1

struct nand_info {
        volatile bool bTransComplete;
	bool bSpareOnly;
	bool bStatusRequest;
	u16 colAddr;
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
	u16 eccStatus[4];
	u16 subpage;
#endif
};

static struct nand_info g_nandfc_info;

#if CONFIG_MTD_NAND_MXC_SWECC
static int hardware_ecc = 0;
#else
static int hardware_ecc = 1;
#endif

#ifndef CONFIG_MTD_NAND_MXC_ECC_CORRECTION_OPTION2
static int Ecc_disabled;
#endif

static bool nand_debug = false;

/*
 * This variable indicates if the NFC is active
 */
static int nfc_active = -1;

#ifdef CONFIG_MOT_FEAT_PM
static int mxc_nand_mpm_advice_id = -1;
#endif        


/*
 * OOB placement block for use with hardware ecc generation in smallpage
 */
static struct nand_oobinfo nand_hw_eccoob = {
    .useecc = MTD_NANDECC_AUTOPLACE,
    .eccbytes = 5,
    .eccpos = { 6, 7, 8, 9, 10 },
    /* The OOB free bytes provided by hardware:
     * - 8BIT : {0, 5} , {11, 5}
     * - 16BIT: {0, 6} , {12, 4}
     * currently, the upper layer FS and the flashing tool expect
     * a common OOB layout for both x8 and x16 NAND chip. Therefor,
     * board driver should provide the common free bytes for "oobfree" 
     * to handle both cases.
     */
    .oobfree = { {0, 5} , {12, 4} } 
};

#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
/*
 * OOB placement block for use with hardware ecc generation in lagepage
 */
static struct nand_oobinfo nand_hw_eccoob_lp = {
    .useecc = MTD_NANDECC_AUTOPLACE,
    .eccbytes = 20,
    .eccpos = {6,7,8,9,10,22,23,24,25,26,38,39,40,41,42,54,55,56,57,58},
    .oobfree = {{0,5},{12,9},{28,9},{44,9},{60,4}}
};
#endif

/*!
 * @defgroup NAND_MTD NAND Flash MTD Driver for MXC processors
 */

/*!
 * @file mxc_nd.c
 *
 * @brief This file contains the hardware specific layer for NAND Flash on
 * MXC processor
 *
 * @ingroup NAND_MTD
 */

/*!
 * MTD structure for NAND MTD.
 */
static struct mtd_info *mxc_mtd = NULL;

#ifdef CONFIG_MOT_FEAT_MTD_FS
/*!
 * This array is used by parse_mtd_partitions() for dynamic partition parsing.
 * The proper partition parsing functions will be called in the order as they
 * appear in this array. For the NOR+NAND configuration, the Redboot partition
 * parsing function is called first and then the command line parsing function.
 * For NAND flash only, no Redboot partition parsing, only the command line
 * parsing function is called. If a valid partition table is found by any of
 * these dynamic partition parsing the static partition table won't be used.
 */
#if defined(CONFIG_MTD_SCMA11REF) || defined(CONFIG_MTD_BUTEREF)
extern const char *probes[];
extern struct mtd_partition *parsed_parts;
#else
static const char *probes[] = {"cmdlinepart", NULL};
static struct mtd_partition *parsed_parts = NULL;
#endif
#endif  /* CONFIG_MOT_FEAT_MTD_FS */

/*!
 * This table contains the static partition information for the NAND flash. 
 * It is only used by add_mtd_partitions() for static partition registration to 
 * the upper MTD driver. See include/linux/mtd/partitions.h for definition of 
 * the mtd_partition structure.
 */
static struct mtd_partition partition_info[] = {
        { 
                .name = "IPL-SPL",
                .offset = 0,
                .size = 0x4000 
        },
        { 
                .name = "Kernel",
                .offset = MTDPART_OFS_APPEND,
                .size = 4*1024*1024 
        },
        { 
                .name = "ROOTFS.CRAMFS",
                .offset = MTDPART_OFS_APPEND,
                .size = 20*1024*1024 
        },
        { 
                .name = "JFFS2",
                .offset =  MTDPART_OFS_APPEND,
#ifdef CONFIG_MACH_SCMA11REF
		.size = 16*1024*1024
	},
	{
		.name = "YAFFS",
		.offset = MTDPART_OFS_APPEND,
		.size = 0x1000000
	},
	{
		.name = "Other",
		.offset = MTDPART_OFS_APPEND,
#endif		
		.size =    MTDPART_SIZ_FULL
        }
};


static wait_queue_head_t irq_waitq;

/* This is the interrupt handler responsible for disabling interrupts and 
 * waking up whoever is waiting on the waitq.
 * 
 * @param       irq                     IRQ being sent    
 * @param       dev_id                  Device ID 
 * @param       regs                    Registers
 */

static irqreturn_t mxc_nfc_irq(int irq, void* dev_id, struct pt_regs* regs)
{
	NFC_CONFIG1 |= NFC_INT_MSK;	     /* Disable interrupt */
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT

	if (g_nandfc_info.subpage)
	{
                /* Issues the remaining 512 byte reads for LP nand.  The first one is 
                * issued in send_read_page_lp() */

		/* get the ECC status from ECC registers for the completed page*/
		g_nandfc_info.eccStatus[g_nandfc_info.subpage-1 ] &= NFC_ECC_STATUS_RESULT;
		if (g_nandfc_info.subpage < 4) {
		        /* fill in ADDR register with proper addr */
		        NFC_BUF_ADDR = g_nandfc_info.subpage;
		        /* always enabling both main and spare data */
		        NFC_CONFIG1 &= (~(NFC_SP_EN));
		        /* set config2 register for data output */
		        NFC_CONFIG2 = NFC_OUTPUT;
		        g_nandfc_info.subpage++; 

		        NFC_CONFIG1 &= ~NFC_INT_MSK; /* Enable interrupt */
		        return IRQ_RETVAL(1);
                } else 
			g_nandfc_info.subpage = 0;
	}
#endif
        g_nandfc_info.bTransComplete = 1;
        wake_up(&irq_waitq);

	return IRQ_RETVAL(1);
}

/*!
 * This function polls the NANDFC to wait for the basic operation to complete by
 * checking the INT bit of config2 register.
 * 
 * @param       maxRetries     number of retry attempts (separated by 1 us)
 * @param       param          parameter for debug
 * @param       useirq         True if IRQ should be used rather than polling
 */
static void wait_op_done(int maxRetries, u16 param, bool useirq)
{
        nfc_active = 1;
#ifdef CONFIG_MOT_FEAT_PM
        mpm_driver_advise(mxc_nand_mpm_advice_id, MPM_ADVICE_DRIVER_IS_BUSY);
#endif        
        
#ifdef CONFIG_MOT_FEAT_KPANIC
	if (useirq && !kpanic_in_progress) {
#else
	if (useirq) {
#endif
                    g_nandfc_info.bTransComplete = 0;
                    wmb();
		    NFC_CONFIG1 &= ~NFC_INT_MSK; /* Enable interrupt */
		    wait_event(irq_waitq, g_nandfc_info.bTransComplete);
		    NFC_CONFIG2 &= ~NFC_INT;
	} else {
		while (maxRetries-- > 0) {
			if (NFC_CONFIG2 & NFC_INT) {
				NFC_CONFIG2 &= ~NFC_INT;
				break;
			}

			udelay(1);
		}
		if (maxRetries <= 0) {
#ifdef CONFIG_MOT_FEAT_MTD_FS
        		printk (KERN_NOTICE "%s(%d): INT not set\n", __FUNCTION__,param);
#else
			DEBUG(MTD_DEBUG_LEVEL0, "%s(%d): INT not set\n",
			      __FUNCTION__, param);
#endif
		}
	}
        
        nfc_active = -1;
#ifdef CONFIG_MOT_FEAT_PM
        mpm_driver_advise(mxc_nand_mpm_advice_id, MPM_ADVICE_DRIVER_IS_NOT_BUSY);
#endif        
        
}

/*!
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @param       cmd     command for NAND Flash
 * @param       useirq  True if IRQ should be used rather than polling
 */
static void send_cmd(u16 cmd, bool useirq)
{
	if (nand_debug)
		DEBUG(MTD_DEBUG_LEVEL3,
		      "send_cmd(0x%x,%d)\n", cmd, useirq);

	NFC_FLASH_CMD = (u16) cmd;
	NFC_CONFIG2 = NFC_CMD;
	if (NAND_CMD_RESET == cmd) {
#if defined CONFIG_ARCH_MXC91331
		udelay(10);
#elif defined CONFIG_ARCH_MXC91231
		if (system_rev == CHIP_REV_1_0) {
			udelay(10);
		}
#endif
	}
	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, cmd, useirq);
}

/*!
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @param       addr    address to be written to NFC.
 */
static void send_addr(u16 addr)
{
	if (nand_debug)
		DEBUG(MTD_DEBUG_LEVEL3, "send_addr(0x%x)\n", addr);

	NFC_FLASH_ADDR = addr;
	NFC_CONFIG2 = NFC_ADDR;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, addr, false);
}

#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
/*!
 * This function requests the NANDFC to initate the transfer
 * of data currently in the NANDFC RAM buffer to the NAND device.
 *
 * @param       bSpareOnly    set true if only the spare area is transferred
 */
static void send_prog_page_lp(bool bSpareOnly)
{
	u16 i;
	
	if (nand_debug)
		DEBUG (MTD_DEBUG_LEVEL3, "send_prog_page_lp (%d)\n", bSpareOnly);

	/* always Configure for page+spare access */
	for (i = 0x0; i<=0x3; i++) {
		/* fill in ADDR register with proper addr */
		NFC_BUF_ADDR = i;
		/* always enabling both main and spare data */
		NFC_CONFIG1 &= (~(NFC_SP_EN));
		/* set config2 register for data input */
		NFC_CONFIG2 = NFC_INPUT;
		/* wait for INT to rise */
		wait_op_done(TROP_US_DELAY, bSpareOnly, false);
	}
}

/*!
 * This function requests the NANDFC to initated the transfer
 * of data from the NAND device into in the NANDFC ram buffer.
 *
 * @param       bSpareOnly    set true if only the spare area is transferred
 */
static void send_read_page_lp(bool bSpareOnly)
{
	u16 i;
	if (nand_debug)
		DEBUG (MTD_DEBUG_LEVEL3, "send_read_page_lp (%d)\n", bSpareOnly);

#ifdef MENSHE_E8_71_01_8BR
#ifdef CONFIG_MOT_FEAT_PM
        /* Send IOI to MPM in order to not sleep during NAND access */
        mpm_handle_ioi();
#endif
#endif
	
		
	/* Only issues the first 512 byte read.  The rest are issued in
         * mxc_nfc_irq().  The LP flash is not readable when
         * kpanic_in_progress since kpanic_in_progress only polls and
         * reading from LP nand will not work in polled mode.
         */

        if (kpanic_in_progress) {
                /* always configure for page+spare access */
                for (i = 0x0; i<=0x3; i++) {
                        /* fill in ADDR register with proper addr */
                        NFC_BUF_ADDR = i;
                        /* always enabling both main and spare data */
                        NFC_CONFIG1 &= (~(NFC_SP_EN));
                        /* set config2 register for data output */
                        NFC_CONFIG2 = NFC_OUTPUT;
                        /* wait for INT to rise */
                        wait_op_done(TROP_US_DELAY, bSpareOnly, false);
                        /* get the ECC status from ECC registers */
                        g_nandfc_info.eccStatus[i] = NFC_ECC_STATUS_RESULT;
                }
        } else {

            for (i=0; i<=3; i++) {

			    /* Pre-fill the eccStatus with the mask of the bits we care about. */
				g_nandfc_info.eccStatus[i] = (bSpareOnly?0x0C:0x0F);
			}

	        g_nandfc_info.subpage = 0;
	        /* always Configure for page+spare access */

      	        /* fill in ADDR register with proper addr */
	        NFC_BUF_ADDR = g_nandfc_info.subpage;
	        /* always enabling both main and spare data */
	        NFC_CONFIG1 &= (~(NFC_SP_EN));
	        /* set config2 register for data output */
	        NFC_CONFIG2 = NFC_OUTPUT;
	        g_nandfc_info.subpage++; 
	        /* wait for INT to rise */
	        wait_op_done(TROP_US_DELAY, bSpareOnly, true);
        }
}
#endif

/*!
 * This function requests the NANDFC to initate the transfer
 * of data currently in the NANDFC RAM buffer to the NAND device.
 *
 * @param       bSpareOnly    set true if only the spare area is transferred
 */
static void send_prog_page(bool bSpareOnly)
{
	if (nand_debug)
		DEBUG(MTD_DEBUG_LEVEL3, "send_prog_page (%d)\n", bSpareOnly);

	/* NANDFC buffer 0 is used for page read/write */
	NFC_BUF_ADDR = 0x0;
	
	/* Configure spare or page+spare access */
	if (bSpareOnly) {
		NFC_CONFIG1 |= NFC_SP_EN;
	} else {
		NFC_CONFIG1 &= (~(NFC_SP_EN));
	}
	NFC_CONFIG2 = NFC_INPUT;
	
	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, bSpareOnly, true);
}

/*!
 * This function will correct the single bit ECC error
 *
 * @param  eccpos  Ecc byte and bit position
 * @param  bSpareOnly  set to true if only spare area needs correction
 */

static void mxc_nd_correct_error(u16 eccpos, bool bSpareOnly)
{
	u16 col;
	u8 pos;
	volatile u16 *buf;

	/* Get col & bit position of error */
	col = COLPOS(eccpos);	/* Get half-word position */
	pos = BITPOS(eccpos);	/* Get bit position */

	if (nand_debug)
		DEBUG(MTD_DEBUG_LEVEL3,
		      "mxc_nd_correct_error (col=%d pos=%d)\n", col, pos);

	/* Set the pointer for main / spare area */
	if (!bSpareOnly) {
		buf = MAIN_AREA0 + (col >> 1);
	} else {
		buf = SPARE_AREA0 + (col >> 1);
	}

	/* Fix the data */
	*buf ^= (1 << pos);
}

/*!
 * This function will maintains state of single bit Error
 * in Main & spare  area
 *
 * @param  spare  set to true if only spare area needs correction
 */
static void mxc_nd_correct_ecc(bool spare)
{
#ifdef CONFIG_MTD_NAND_MXC_ECC_CORRECTION_OPTION2
	static int lastErrMain = 0, lastErrSpare = 0;	/* To maintain single bit
							   error in previous page */
#endif
	u16 value, ecc_status;
	/* Read the ECC result */
	ecc_status = NFC_ECC_STATUS_RESULT;
	if (nand_debug)
		DEBUG(MTD_DEBUG_LEVEL3,
		      "mxc_nd_correct_ecc (Ecc status=%x)\n", ecc_status);

#ifdef CONFIG_MTD_NAND_MXC_ECC_CORRECTION_OPTION2
	/* Check for Error in Mainarea */
	if (ecc_status & MAIN_SINGLEBIT_ERROR) {
		/* Check for error in previous page */
		if (lastErrMain && !spare) {
			value = NFC_RSLTMAIN_AREA;
			/* Correct single bit error in Mainarea
			   NFC will not correct the error in
			   current page */
			mxc_nd_correct_error(value, false);
		} else {
			/* Set if single bit error in current page */
			lastErrMain = 1;
		}
	} else {
		/* Reset if no single bit error in current page */
		lastErrMain = 0;
	}

	/* Check for Error in Sparearea */
	if (ecc_status & SPARE_SINGLEBIT_ERROR) {
		/* Check for error in previous page */
		if (lastErrSpare) {
			value = NFC_RSLTSPARE_AREA;
			/* Correct single bit error in Mainarea
			   NFC will not correct the error in
			   current page */
			mxc_nd_correct_error(value, true);
		} else {
			/* Set if single bit error in current page */
			lastErrSpare = 1;
		}
	} else {
		/* Reset if no single bit error in current page */
		lastErrSpare = 0;
	}
#else
	if ((ecc_status & MAIN_SINGLEBIT_ERROR)
	    || (ecc_status & SPARE_SINGLEBIT_ERROR)) {
		if (Ecc_disabled) {
			if (ecc_status & MAIN_SINGLEBIT_ERROR) {
				value = NFC_RSLTMAIN_AREA;
				/* Correct single bit error in Mainarea
				   NFC will not correct the error in
				   current page */
				mxc_nd_correct_error(value, false);
			}
			if (ecc_status & SPARE_SINGLEBIT_ERROR) {
				value = NFC_RSLTSPARE_AREA;
				/* Correct single bit error in Mainarea
				   NFC will not correct the error in
				   current page */
				mxc_nd_correct_error(value, true);
			}

		} else {
			/* Disable ECC  */
			NFC_CONFIG1 &= ~(NFC_ECC_EN);
			Ecc_disabled = 1;
		}
	} else if (ecc_status == 0) {
		if (Ecc_disabled) {
			/* Enable ECC */
			NFC_CONFIG1 |= NFC_ECC_EN;
			Ecc_disabled = 0;
		}
	} else {
		/* 2-bit Error  Do nothing */
	}
#endif				/* CONFIG_MTD_NAND_MXC_ECC_CORRECTION_OPTION2 */

}

/*!
 * This function requests the NANDFC to initated the transfer
 * of data from the NAND device into in the NANDFC ram buffer.
 *
 * @param       bSpareOnly    set true if only the spare area is transferred
 */
static void send_read_page(bool bSpareOnly)
{
	if (nand_debug)
		DEBUG(MTD_DEBUG_LEVEL3, "send_read_page (%d)\n", bSpareOnly);

	/* NANDFC buffer 0 is used for page read/write */
	NFC_BUF_ADDR = 0x0;

	/* Configure spare or page+spare access */
	if (bSpareOnly) {
		NFC_CONFIG1 |= NFC_SP_EN;
	} else {
		NFC_CONFIG1 &= (~(NFC_SP_EN));
	}

	NFC_CONFIG2 = NFC_OUTPUT;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, bSpareOnly, true);

	/* If there are single bit errors in
	   two consecutive page reads then
	   the error is not  corrected by the
	   NFC for the second page.
	   Correct single bit error in driver */
#if defined CONFIG_ARCH_MXC91231
	/* Removed NFC workaround in MXC91231-P2.1 */
	if (system_rev != CHIP_REV_2_1) {
		mxc_nd_correct_ecc(bSpareOnly);
	}
#else
	mxc_nd_correct_ecc(bSpareOnly);
#endif

}

/*!
 * This function requests the NANDFC to perform a read of the
 * NAND device ID.
 */
static void send_read_id(void)
{
	struct nand_chip *this = &mxc_nand_data->nand;

	/* NANDFC buffer 0 is used for device ID output */
	NFC_BUF_ADDR = 0x0;

	/* Read ID into main buffer */
	NFC_CONFIG1 &= (~(NFC_SP_EN));
	NFC_CONFIG2 = NFC_ID;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, 0, false);

	if (this->options & NAND_BUSWIDTH_16) {
		volatile u16 *mainBuf = MAIN_AREA0;

		/*
		 * Pack the every-other-byte result for 16-bit ID reads
		 * into every-byte as the generic code expects and various
		 * chips implement.
		 */

		mainBuf[0] = (mainBuf[0] & 0xff) | ((mainBuf[1] & 0xff) << 8);
		mainBuf[1] = (mainBuf[2] & 0xff) | ((mainBuf[3] & 0xff) << 8);
		mainBuf[2] = (mainBuf[4] & 0xff) | ((mainBuf[5] & 0xff) << 8);
	}
}

/*!
 * This function requests the NANDFC to perform a read of the
 * NAND device status and returns the current status.
 *
 * @return  device status
 */
static u16 get_dev_status(void)
{
	volatile u16 *mainBuf = MAIN_AREA1;

	/* Issue status request to NAND device */

	/*
	 * NANDFC buffer 1 is used for device status to prevent
	 * corruption of read/write buffer on status requests.
	 */
	NFC_BUF_ADDR = 1;

#ifndef CONFIG_MOT_WFN472
	/* Send the Read status command before reading status */
	send_cmd(NAND_CMD_STATUS, true);
#endif

	/* Read status into main buffer */
	NFC_CONFIG1 &= (~(NFC_SP_EN));
	NFC_CONFIG2 = NFC_STATUS;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY, 0, false);

	/* Status is placed in first word of main buffer */
	return mainBuf[0];
}

/*!
 * This functions is used by upper layer to checks if device is ready
 *
 * @param       mtd     MTD structure for the NAND Flash
 *
 * @return  0 if device is busy else 1
 */
static int mxc_nand_dev_ready(struct mtd_info *mtd)
{
	/* Check if NAND device is ready */
#ifdef CONFIG_MOT_WFN381
	/* After mxc_nand_command(), the device is in ready state */
	return 1;
#else
	if (get_dev_status() & NAND_STATUS_READY) {
		return 1;
	}
	return 0;
#endif
}

static void mxc_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	/*
	 * If HW ECC is enabled, we turn it on during init.  There is
	 * no need to enable again here.
	 */
}

#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
static int mxc_nand_correct_data_lp(struct mtd_info * mtd, u_char * dat, u_char * read_ecc, u_char * calc_ecc)
{
	/* In large page (2k) NAND flash, the HW ECC detection and correction
	 * handle them as 4 sections of small page (512+16). For each section
	 * 1-Bit error is automatically corrected, and it can correct up to 4
	 * 1-Bit errors if they are in four different section; 2-Bit error in any
	 * section cannot be corrected by HW ECC, so we need to return failure.
	 */
	int i, ret = 0;
	for (i=0; i<4; i++) {
		if (((g_nandfc_info.eccStatus[i]&0x3) == 2)||
		   ((g_nandfc_info.eccStatus[i]>>2) == 2)) {
			printk(KERN_NOTICE
				"MXC_NAND: HWECC uncorrectable 2-bit ECC error on section %d\n",i);
			return -1; /* return with 2-BIT ECC error */
		}
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
		else if (((g_nandfc_info.eccStatus[i]&0x3) == 1)||
			((g_nandfc_info.eccStatus[i]>>2) == 1)) {
			printk(KERN_NOTICE
				"MXC_NAND: HWECC detected/corrected 1-bit ECC error on section %d\n",i);
			ret = 1;
		}
#endif /* CONFIG_MOT_FEAT_NAND_RDDIST */
	}
	return ret; /* return with no ECC or 1bit error */
}
#endif

static int mxc_nand_correct_data(struct mtd_info * mtd, u_char * dat, u_char * read_ecc, u_char * calc_ecc)
{
	/*
	 * 1-Bit errors are automatically corrected in HW.  No need for
	 * additional correction.  2-Bit errors cannot be corrected by
	 * HW ECC, so we need to return failure
	 */
	u16 ecc_status = NFC_ECC_STATUS_RESULT;

	if (((ecc_status & 0x3) == 2) || ((ecc_status >> 2) == 2)) {
		DEBUG(MTD_DEBUG_LEVEL0,
		      "MXC_NAND: HWECC uncorrectable 2-bit ECC error\n");
		return -1;
	}
#ifdef CONFIG_MOT_FEAT_NAND_RDDIST
	else /* HWECC detect and correct 1-bit error */
	if (((ecc_status & 0x3) == 1)||((ecc_status >> 2) == 1)) {
		printk(KERN_NOTICE "MXC_NAND: HWECC detected/corrected 1-bit ECC error\n");
		return 1;
	}
#endif /* CONFIG_MOT_FEAT_NAND_RDDIST */
	return 0;
}

static int mxc_nand_calculate_ecc(struct mtd_info *mtd, const u_char * dat,
				  u_char * ecc_code)
{
	/*
	 * Just return success.  HW ECC does not read/write the NFC spare
	 * buffer.  Only the FLASH spare area contains the calcuated ECC.
	 */
	return 0;
}

/*!
 * This function reads byte from the NAND Flash
 *
 * @param       mtd     MTD structure for the NAND Flash
 *
 * @return    data read from the NAND Flash
 */
static u_char mxc_nand_read_byte(struct mtd_info *mtd)
{
	u_char retVal = 0;
	u16 col, rdWord;
	volatile u16 *mainBuf = MAIN_AREA0;
	volatile u16 *spareBuf = SPARE_AREA0;

	/* Check for status request */
	if (g_nandfc_info.bStatusRequest) {
		return (get_dev_status() & 0xFF);
	}

	/* Get column for 16-bit access */
	col = g_nandfc_info.colAddr >> 1;

	/* If we are accessing the spare region */
	if (g_nandfc_info.bSpareOnly) {
		rdWord = spareBuf[col];
	} else {
		rdWord = mainBuf[col];
	}

	/* Pick upper/lower byte of word from RAM buffer */
	if (g_nandfc_info.colAddr & 0x1) {
		retVal = (rdWord >> 8) & 0xFF;
	} else {
		retVal = rdWord & 0xFF;
	}

	/* Update saved column address */
	g_nandfc_info.colAddr++;

	return retVal;
}

/*!
 * This function writes the data byte to the NAND Flash
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       byte    data to be written to NAND Flash
 */
static void mxc_nand_write_byte(struct mtd_info *mtd, u_char byte)
{
	DEBUG(MTD_DEBUG_LEVEL0, "ERROR:  mxc_nand_write_byte not supported.\n");
}

/*!
  * This function reads word from the NAND Flash
  *
  * @param       mtd     MTD structure for the NAND Flash
  *
  * @return    data read from the NAND Flash
  */
static u16 mxc_nand_read_word(struct mtd_info *mtd)
{
	u16 col;
	u16 rdWord, retVal;
	volatile u16 *p;

	if (nand_debug) {
		DEBUG(MTD_DEBUG_LEVEL3,
		      "mxc_nand_read_word(col = %d)\n", g_nandfc_info.colAddr);
	}

	col = g_nandfc_info.colAddr;
	/* Adjust saved column address */
	if (col < mtd->oobblock && g_nandfc_info.bSpareOnly)
		col += mtd->oobblock;

	if (col < mtd->oobblock)
		p = (MAIN_AREA0) + (col >> 1);
	else
		p = (SPARE_AREA0) + ((col - mtd->oobblock) >> 1);

	if (col & 1) {
		rdWord = *p;
		retVal = (rdWord >> 8) & 0xff;
		rdWord = *(p + 1);
		retVal |= (rdWord << 8) & 0xff00;

	} else {
		retVal = *p;

	}

	/* Update saved column address */
	g_nandfc_info.colAddr = col + 2;

	return retVal;
}

 /*!
  * This function writes the data word to the NAND Flash
  *
  * @param       mtd     MTD structure for the NAND Flash
  * @param       word    data to be written to NAND Flash
  */
static void mxc_nand_write_word(struct mtd_info *mtd, u16 word)
{
	DEBUG(MTD_DEBUG_LEVEL0, "ERROR:  mxc_nand_write_word not supported.\n");
}

/*!
 * This function writes data of length \b len to buffer \b buf. The data to be
 * written on NAND Flash is first copied to RAMbuffer. After the Data Input
 * Operation by the NFC, the data is written to NAND Flash
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be written to NAND Flash
 * @param       len     number of bytes to be written
 */
static void mxc_nand_write_buf(struct mtd_info *mtd,
			       const u_char * buf, int len)
{
	int n;
	int col;
	int i = 0;

	if (nand_debug)
		DEBUG(MTD_DEBUG_LEVEL3,
		      "mxc_nand_write_buf(col = %d, len = %d)\n",
		      g_nandfc_info.colAddr, len);

	col = g_nandfc_info.colAddr;

	/* Adjust saved column address */
	if (col < mtd->oobblock && g_nandfc_info.bSpareOnly)
	{
		col += mtd->oobblock;
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
		if (IS_LPNAND(mtd))
			/* preset main areas with 0xff, and
			 * make sure no overwrite on main area */
			memset((volatile u32 *)((ulong)(MAIN_AREA0)), 0xff, mtd->oobblock);
#endif
	}

	n = mtd->oobblock + mtd->oobsize - col;
	n = min(len, n);

	if (nand_debug)
		DEBUG(MTD_DEBUG_LEVEL3,
		      "%s:%d: col = %d, n = %d\n",
		      __FUNCTION__, __LINE__, col, n);

	while (n) {
		volatile u32 *p;
		if (col < mtd->oobblock)
			p = (volatile u32 *)((ulong) (MAIN_AREA0) + (col & ~3));
		else
			p = (volatile u32 *)((ulong) (SPARE_AREA0) -
					     mtd->oobblock + (col & ~3));

		if (nand_debug)
			DEBUG(MTD_DEBUG_LEVEL3,
			      "%s:%d: p = %p\n", __FUNCTION__, __LINE__, p);

		if (((col | (int)&buf[i]) & 3) || n < 16) {
			u32 data = 0;

			if (col & 3 || n < 4)
				data = *p;

			switch (col & 3) {
			case 0:
				if (n) {
					data = (data & 0xffffff00) |
					    (buf[i++] << 0);
					n--;
					col++;
				}
			case 1:
				if (n) {
					data = (data & 0xffff00ff) |
					    (buf[i++] << 8);
					n--;
					col++;
				}
			case 2:
				if (n) {
					data = (data & 0xff00ffff) |
					    (buf[i++] << 16);
					n--;
					col++;
				}
			case 3:
				if (n) {
					data = (data & 0x00ffffff) |
					    (buf[i++] << 24);
					n--;
					col++;
				}
			}

			*p = data;
		} else {
			int m = mtd->oobblock - col;

			if (col >= mtd->oobblock)
				m += mtd->oobsize;

			m = min(n, m) & ~3;

			if (nand_debug)
				DEBUG(MTD_DEBUG_LEVEL3,
				      "%s:%d: n = %d, m = %d, i = %d, col = %d\n",
				      __FUNCTION__, __LINE__, n, m, i, col);

			memcpy((void *)(p), &buf[i], m);
			col += m;
			i += m;
			n -= m;
		}
	}
#if 0
	/* dumpping write buffer */
	if (nand_debug) {
		int j;
		for (j=0; j<len; j++) {
			if (g_nandfc_info.bSpareOnly)
				printk("w[%03d]=%02x ",j+col-len,buf[j]);
			else
				printk("w[%03d]=%02x ",j,buf[j]);
			if ((j%8)==7) printk("\n");
		}
	}
#endif
	/* Update saved column address */
	g_nandfc_info.colAddr = col;
}

/*!
 * This function id is used to read the data buffer from the NAND Flash. To
 * read the data from NAND Flash first the data output cycle is initiated by
 * the NFC, which copies the data to RAMbuffer. This data of length \b len is
 * then copied to buffer \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be read from NAND Flash
 * @param       len     number of bytes to be read
 */
static void mxc_nand_read_buf(struct mtd_info *mtd, u_char * buf, int len)
{

	int n;
	int col;
	int i = 0;

	if (nand_debug) {
		DEBUG(MTD_DEBUG_LEVEL3,
		      "mxc_nand_read_buf(col = %d, len = %d)\n",
		      g_nandfc_info.colAddr, len);
	}

	col = g_nandfc_info.colAddr;
	/* Adjust saved column address */
	if (col < mtd->oobblock && g_nandfc_info.bSpareOnly)
		col += mtd->oobblock;

	n = mtd->oobblock + mtd->oobsize - col;
	n = min(len, n);

	while (n) {
		volatile u32 *p;

		if (col < mtd->oobblock)
			p = (volatile u32 *)((ulong) (MAIN_AREA0) + (col & ~3));
		else
			p = (volatile u32 *)((ulong) (SPARE_AREA0) -
					     mtd->oobblock + (col & ~3));

		if (((col | (int)&buf[i]) & 3) || n < 16) {
			u32 data;

			data = *p;
			switch (col & 3) {
			case 0:
				if (n) {
					buf[i++] = (u8) (data);
					n--;
					col++;
				}
			case 1:
				if (n) {
					buf[i++] = (u8) (data >> 8);
					n--;
					col++;
				}
			case 2:
				if (n) {
					buf[i++] = (u8) (data >> 16);
					n--;
					col++;
				}
			case 3:
				if (n) {
					buf[i++] = (u8) (data >> 24);
					n--;
					col++;
				}
			}
		} else {
			int m = mtd->oobblock - col;

			if (col >= mtd->oobblock)
				m += mtd->oobsize;

			m = min(n, m) & ~3;
			memcpy(&buf[i], (void *)(p), m);
			col += m;
			i += m;
			n -= m;
		}
	}
#if 0
	/* debug only - dumpping read buffer */
	if (nand_debug) {
		int j;
		for (j=0; j<len; j++) {
			if (g_nandfc_info.bSpareOnly)
				printk("r[%03d]=%02x ",j+col-len,buf[j]);
			else
				printk("r[%03d]=%02x ",j,buf[j]);
			if ((j%8)==7) printk("\n");
		}
	}
#endif
	/* Update saved column address */
	g_nandfc_info.colAddr = col;
}

/*!
 * This function is used by the upper layer to verify the data in NAND Flash
 * with the data in the \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be verified
 * @param       len     length of the data to be verified
 *
 * @return      -EFAULT if error else 0
 *
 */
static int
mxc_nand_verify_buf(struct mtd_info *mtd, const u_char * buf, int len)
{
	return -EFAULT;
}

/*!
 * This function is used by upper layer for select and deselect of the NAND
 * chip
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       chip    val indicating select or deselect
 */
static void mxc_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if (chip > 0) {
		DEBUG(MTD_DEBUG_LEVEL0,
		      "ERROR:  Illegal chip select (chip = %d)\n", chip);
		return;
	}

	/* Motorola decided not to use the CONFIG_MTD_NAND_MXC_FORCE_CE flag in this 
	 * function since this flag needs to be turned on to support the enable/disable
	 * NFC clock feature.
	 */

	if (chip == -1) {
                if(likely(jiffies > 6000)) {
                        /* Disable the NFC clock */
                        mxc_clks_disable(NFC_CLK);
                }
                else {
                        return;
                }
	} else {
		/* Enable the NFC clock */
		mxc_clks_enable(NFC_CLK);
	}
	return;
}

/*!
 * This function is used by the upper layer to write command to NAND Flash for
 * different operations to be carried out on NAND Flash
 *
 * @param       mtd             MTD structure for the NAND Flash
 * @param       command         command for NAND Flash
 * @param       column          column offset for the page read
 * @param       page_addr       page to be read from NAND Flash
 *
 * NOTE:       	when mxc_nand_command function succeed, it is garrantee that
 *		the nand device is in ready state. (added with WFN381)
 */
static void mxc_nand_command(struct mtd_info *mtd, unsigned command,
			     int column, int page_addr)
{
        bool useirq = true;

	if (nand_debug)
		DEBUG(MTD_DEBUG_LEVEL3,
		      "mxc_nand_command (cmd = 0x%x, col = 0x%x, page = 0x%x)\n",
		      command, column, page_addr);

#ifdef CONFIG_MOT_FEAT_PM
        mpm_driver_advise(mxc_nand_mpm_advice_id, MPM_ADVICE_DRIVER_IS_BUSY);
#endif
	/*
	 * Reset command state information
	 */
	g_nandfc_info.bStatusRequest = false;

	/*
	 * Command pre-processing step
	 */
	switch (command) {

	case NAND_CMD_STATUS:
		g_nandfc_info.colAddr = 0;
		g_nandfc_info.bStatusRequest = true;
                useirq = false;
		break;

	case NAND_CMD_READ0:
		g_nandfc_info.colAddr = column;
		g_nandfc_info.bSpareOnly = false;
		useirq = false;
		break;

	case NAND_CMD_READOOB:
		g_nandfc_info.colAddr = column;
		g_nandfc_info.bSpareOnly = true;
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
		if (IS_LPNAND(mtd))
			command = NAND_CMD_READ0;
#endif
		useirq = false;
		break;

	case NAND_CMD_SEQIN:
		if (column >= mtd->oobblock) {
			g_nandfc_info.colAddr = column - mtd->oobblock;
			g_nandfc_info.bSpareOnly = true;
			/* Set program pointer to spare region */
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
			if (!(IS_LPNAND(mtd)))
#endif
				send_cmd(NAND_CMD_READOOB, false);
		} else {
			g_nandfc_info.bSpareOnly = false;
			g_nandfc_info.colAddr = column;
			/* Set program pointer to page start */
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
			if (!(IS_LPNAND(mtd)))
#endif
				send_cmd(NAND_CMD_READ0, false);
		}
		useirq = false;
		break;

	case NAND_CMD_PAGEPROG:
#ifndef CONFIG_MTD_NAND_MXC_ECC_CORRECTION_OPTION2
		if (Ecc_disabled) {
			/* Enable Ecc for page writes */
			NFC_CONFIG1 |= NFC_ECC_EN;
		}
#endif
#ifdef CONFIG_MOT_FEAT_NANDECC_TEST
	    	if (nand_ecc_flag == MTD_NANDECC_OFF) {
	    		/* turn off HW ECC to inject 1bit or 2bit error */
			NFC_CONFIG1 &= ~NFC_ECC_EN;
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
			if (IS_LPNAND(mtd))
				send_prog_page_lp(g_nandfc_info.bSpareOnly);
			else
#endif /*CONFIG_MOT_FEAT_LPNAND_SUPPORT*/
				send_prog_page(g_nandfc_info.bSpareOnly);
			NFC_CONFIG1 |= NFC_ECC_EN;
			nand_ecc_flag = MTD_NANDECC_AUTOPLACE;
	    	} else
#endif /*CONFIG_MOT_FEAT_NANDECC_TEST*/
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
		if (IS_LPNAND(mtd))
			send_prog_page_lp(g_nandfc_info.bSpareOnly);
		else
#endif
			send_prog_page(g_nandfc_info.bSpareOnly);
		break;

	case NAND_CMD_ERASE1:
		useirq = false;
		break;
	}

        send_cmd(command, useirq);

	/*
	 * Write out column address, if necessary
	 */
	if (column != -1) {
		/*
		 * MXC NANDFC can only perform full page+spare or
		 * spare-only read/write.  When the upper layers
		 * layers perform a read/write buf operation,
		 * we will used the saved column adress to index into
		 * the full page.
		 */
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
		if (IS_LPNAND(mtd)) {
			/* 1st column address cycle for large_page nand */
			send_addr(column);  /* coladdr_0 - coladdr_7 */
			/* 2nd column address cycle for large_page nand */
			send_addr((column>>8)&0x0f);  /* coladd_8 - coladdr_11 */
		}
		else
#endif			/* only one column address cycle for small_page nand */
			send_addr(0);
	}

	/*
	 * Write out page address, if necessary
	 */
	if (page_addr != -1) {
		send_addr(page_addr & 0xff); /* paddr_0 - paddr_7 */
                send_addr((page_addr >> 8) & 0xff); /* paddr_8 - paddr_15 */

		/* One more address cycle for higher density devices */
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
		/*  - LargePage: 256MB (0x10000000)
		 *  - SmallPage: 64MB (0x4000000)
		 */
		if ((mtd->size >= 0x10000000) ||
		    ((mtd->size >= 0x4000000) && !(IS_LPNAND(mtd))))
#else
		if (mtd->size >= 0x4000000)

#endif
		   	send_addr((page_addr >> 16) & 0xff);
	}

	/*
	 * Command post-processing step
	 */
	switch (command) {

	case NAND_CMD_RESET:
#ifdef CONFIG_MOT_FEAT_NAND_AUTO_DETECT
		g_nandfc_info.colAddr = 0;
#endif
		break;

	case NAND_CMD_READOOB:
	case NAND_CMD_READ0:
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
		if (IS_LPNAND(mtd)) {
			send_cmd(NAND_CMD_READSTART, true);
			send_read_page_lp(g_nandfc_info.bSpareOnly);
		} else
#endif
			send_read_page(g_nandfc_info.bSpareOnly);
		break;

	case NAND_CMD_READID:
		send_read_id();
		break;

	case NAND_CMD_PAGEPROG:
#ifndef CONFIG_MTD_NAND_MXC_ECC_CORRECTION_OPTION2
		if (Ecc_disabled) {
			/* Disble Ecc after page writes */
			NFC_CONFIG1 &= ~(NFC_ECC_EN);
		}
#endif
		break;

	case NAND_CMD_STATUS:
		break;

	case NAND_CMD_ERASE2:
		break;
	}
#ifdef CONFIG_MOT_FEAT_PM
        mpm_driver_advise(mxc_nand_mpm_advice_id, MPM_ADVICE_DRIVER_IS_NOT_BUSY);
#endif
}

#ifdef CONFIG_MXC_NAND_LOW_LEVEL_ERASE
static void mxc_low_erase(struct mtd_info *mtd)
{

	struct nand_chip *this;
	this = mtd->priv;
	unsigned int page_addr, addr;
	u_char status;

	DEBUG(MTD_DEBUG_LEVEL0, "MXC_ND : mxc_low_erase:Erasing NAND\n");
	for (addr = 0; addr < this->chipsize; addr += mtd->erasesize) {
		page_addr = addr / mtd->oobblock;
		mxc_nand_command(mtd, NAND_CMD_ERASE1, -1, page_addr);
		mxc_nand_command(mtd, NAND_CMD_ERASE2, -1, -1);
		mxc_nand_command(mtd, NAND_CMD_STATUS, -1, -1);
		status = mxc_nand_read_byte(mtd);
		if (status & NAND_STATUS_FAIL) {
			printk("ERASE FAILED(block = %d,status = 0x%x)\n",
			       addr / mtd->erasesize, status);
		}
	}

}
#endif
/*!
 * This function is called during the driver binding process.
 *
 * @param   dev   the device structure used to store device specific
 *                information that is used by the suspend, resume and
 *                remove functions
 *
 * @return  The function always returns 0.
 */
static int __init mxcnd_probe(struct device *dev)
{
	struct nand_chip *this;
	struct mtd_info *mtd;
	struct platform_device *pdev = to_platform_device(dev);
	struct flash_platform_data *flash = pdev->dev.platform_data;
	int nr_parts = 0;

	int err = 0;
	/* Allocate memory for MTD device structure and private data */
	mxc_nand_data = kmalloc(sizeof(struct mxc_mtd_s), GFP_KERNEL);
	if (!mxc_nand_data) {
		printk(KERN_ERR "%s: failed to allocate mtd_info\n",
		       __FUNCTION__);
		err = -ENOMEM;
		goto out;
	}
	memset(mxc_nand_data, 0, sizeof(struct mxc_mtd_s));

	mxc_nand_data->dev = dev;
	/* structures must be linked */
	this = &mxc_nand_data->nand;
	mtd = &mxc_nand_data->mtd;
	mtd->priv = this;
	this->priv = mxc_nand_data;

	memset((char *)&g_nandfc_info, 0, sizeof(g_nandfc_info));

	/* 5 us command delay time */
	this->chip_delay = 5;

	this->dev_ready = mxc_nand_dev_ready;
	this->cmdfunc = mxc_nand_command;
	this->select_chip = mxc_nand_select_chip;
	this->write_byte = mxc_nand_write_byte;
	this->read_byte = mxc_nand_read_byte;
	this->write_word = mxc_nand_write_word;
	this->read_word = mxc_nand_read_word;

	this->write_buf = mxc_nand_write_buf;
	this->read_buf = mxc_nand_read_buf;
	this->verify_buf = mxc_nand_verify_buf;

	NFC_CONFIG1 = NFC_INT_MSK;
	init_waitqueue_head(&irq_waitq);
	err = request_irq(INT_NANDFC, mxc_nfc_irq, 0, "mxc_nd", NULL);
	if (err) {
		goto out_1;
	}

	if (hardware_ecc) {
		this->correct_data = mxc_nand_correct_data;
		this->enable_hwecc = mxc_nand_enable_hwecc;
		this->calculate_ecc = mxc_nand_calculate_ecc;
		this->eccmode = NAND_ECC_HW3_512;
		this->autooob = &nand_hw_eccoob;
		NFC_CONFIG1 |= NFC_ECC_EN;
	} else {
		this->eccmode = NAND_ECC_SOFT;
	}

	/* Reset NAND */
	this->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* preset operation */
	/* Unlock the internal RAM Buffer */
	NFC_CONFIG = 0x2;

	/* Blocks to be unlocked */
	NFC_UNLOCKSTART_BLKADDR = 0x0;
	NFC_UNLOCKEND_BLKADDR = 0x4000;

#ifdef CONFIG_MOT_FEAT_MTD_FS
	this->options |= NAND_USE_FLASH_BBT;
#endif

	/* Unlock Block Command for given address range */
	NFC_WRPROT = 0x4;

#ifdef CONFIG_MOT_FEAT_NAND_AUTO_DETECT
	/* NFC bit register in CRM_COM registers @5004400C
	 * NF_PG_SZ:(bit 20) 0 - 512 bytes (default); 1 - 2048 bytes
	 * NF_DWITH:(bit 19) 0 - x8BIT (default); 1 - x16BIT
	 * It is assumed that the MBM bootload has already set the CRM_COM
	 * with the correct NF_PG_SZ and NF_DWITH during startup.
	 */
	int i, busw = 0;

	if (NF_PGSZ_DWITH & (1 << 19))
		busw = NAND_BUSWIDTH_16;

	/* read chip's manufacturer and device IDs */
	this->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
	nand_maf_id = mxc_nand_read_byte(mtd);
	if (busw) mxc_nand_read_byte(mtd);
	nand_dev_id = mxc_nand_read_byte(mtd);

	DEBUG (MTD_DEBUG_LEVEL0, "maf_id:0x%x, dev_id:0x%x\n", nand_maf_id, nand_dev_id);
	for (i = 0; nand_flash_ids[i].name != NULL; i++) {
		/* find the matched chip device id from nand_flash id table */
		if (nand_dev_id != nand_flash_ids[i].id)
			continue;

		/*
		* found chip device id from nand_flash id table! Then,
		* setup NFC clock based on chip manufacturer ID: nand_maf_id
		* (The MXC NFC support nand chip with power voltage: 1.70~1.95v)
		*/
		switch(nand_maf_id) {
		case NAND_MFR_STMICRO:
		#ifdef CONFIG_MOT_FEAT_STM90NM
			if (boardrev() == BOARDREV_P1A) {
				/* P1A with STMICRO nand 90nm chip R/W Access time at 120ns  
				 * NFC clock rate is set to 16.6MHz (with DIV:8 - 0x0111) */
				mxc_set_clocks_div(NFC_CLK, 8);
				break;
			}
		#endif
		case NAND_MFR_TOSHIBA:
			/* Read/Write Access time at 60ns has been validated, and
			 * NFC clock rate will be set to 33.3Mhz (with DIV:4 - 0x0011)
			 */
			 mxc_set_clocks_div(NFC_CLK, 4);
			 break;
		case NAND_MFR_MICRON:
		case NAND_MFR_SAMSUNG:
			/* Read/Write Access time at 90ns has been validated, and
			 * NFC clock rate will be set to 22.2Mhz (with DIV:6 - 0x0101)
			 */
			mxc_set_clocks_div(NFC_CLK, 6);
			break;
		case NAND_MFR_HYNIX:
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA)
			/* Read/Write Access time at 90ns has NOT been validated yet,
			 * NFC clock rate is set to 19 MHz (with DIV:7 - 0x0110)
			 */
			mxc_set_clocks_div(NFC_CLK, 7);
#else
                        /* Read/Write Access time at 60ns has been validated, and
                         * NFC clock rate will be set to 33.3Mhz (with DIV:4 - 0x0011)
                         */			
			mxc_set_clocks_div(NFC_CLK, 4);
#endif
			break;
		default:
			/* default, NFC clock at 16.7MHz */
			break;
		}

		busw = nand_flash_ids[i].options & NAND_BUSWIDTH_16;
		if (busw)
			NF_PGSZ_DWITH |= (1 << 19);

		/* Setup pagesize for the onboard nand chip */
#ifdef CONFIG_MOT_FEAT_LPNAND_SUPPORT
		if (!nand_flash_ids[i].pagesize) /* for large page */
		{
			/* set the pagesize as 2k */
			NF_PGSZ_DWITH |= 1 << 20;
			if (hardware_ecc) {
				/* overwrite with largepage settings */
				this->autooob = &nand_hw_eccoob_lp;
				this->correct_data  = mxc_nand_correct_data_lp;
				this->eccmode = NAND_ECC_HW12_2048;
			}
		}
#endif
		break;
	}

	/* reset the NAND NFC and g_nandfc_info.colAddr */
	this->cmdfunc(mxc_mtd, NAND_CMD_RESET, -1, -1);
	this->options |= busw;
#else 
	/* NAND bus width determines access funtions used by upper layer */
	if (flash->width == 2) {
		this->options |= NAND_BUSWIDTH_16;
	} else {
		this->options |= 0;
	}
#endif /* CONFIG_MOT_FEAT_NAND_AUTO_DETECT */

	/* Scan to find existence of the device */
	if (nand_scan(mtd, 1)) {
		DEBUG(MTD_DEBUG_LEVEL0,
		      "MXC_ND: Unable to find any NAND device.\n");
		err = -ENXIO;
		goto out_1;
	}
	/*
	 * Enable nand_debug only after nand_scan(), otherwise you might be
	 * flooded with debug messages.
	 */
	/* nand_debug = true; */

	/* Register the partitions */
#if defined(CONFIG_MTD_PARTITIONS) && defined(CONFIG_MOT_FEAT_MTD_FS)
	strcpy(mtd->name, "nand0");
	nr_parts =
	    parse_mtd_partitions(mtd, probes, &mxc_nand_data->parts, 0);
	if (nr_parts > 0)
		add_mtd_partitions(mtd, mxc_nand_data->parts, nr_parts);
	else if (flash->parts)
		add_mtd_partitions(mtd, flash->parts, flash->nr_parts);
	else
#endif
	{
		printk("Registering %s as whole device\n", mtd->name);
		add_mtd_device(mtd);
	}
#ifdef CONFIG_MXC_NAND_LOW_LEVEL_ERASE
	/* Erase all the blocks of a NAND */
	mxc_low_erase(mtd);
#endif

	dev_set_drvdata(dev, mtd);
	return 0;

      out_1:
	kfree(mxc_nand_data);
      out:
	return err;

}

/*!
 * Dissociates the driver from the device.
 *
 * @param   dev   the device structure used to give information on which
 *
 * @return  The function always returns 0.
 */
static int __exit mxcnd_remove(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mtd_info *mtd = dev_get_drvdata(&pdev->dev);

	dev_set_drvdata(dev, NULL);

	if (mxc_nand_data) {
		nand_release(mtd);
		free_irq(INT_NANDFC, NULL);
		kfree(mxc_nand_data);
	}

	return 0;
}

#ifdef CONFIG_PM
/*!
 * This function is called to put the NAND in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device information structure
 *
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function returns 0 on success and -1 on failure
 */

static int mxcnd_suspend(struct device *dev, u32 state, u32 level)
{
	struct mtd_info *info = dev_get_drvdata(dev);
	int ret = 0;
	DEBUG(MTD_DEBUG_LEVEL0, "MXC_ND : NAND suspend (Level = %d)\n", level);

	switch (level) {
	case SUSPEND_DISABLE:
		if (nfc_active == 1) {
			DEBUG(MTD_DEBUG_LEVEL0, "%s, %d: NFC is busy. sending -EBUSY\n",
			      __FUNCTION__, __LINE__);
			return -EBUSY;
		}
		break;

	case SUSPEND_SAVE_STATE:
		if (info)
			ret = info->suspend(info);
		break;

	case SUSPEND_POWER_DOWN:
		/* Disable the NFC clock */
		mxc_clks_disable(NFC_CLK);
		break;

	default:
		/* Error */
		return -1;
	}
	
	return ret;
}

/*!
 * This function is called to bring the NAND back from a low power state. Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device information structure
 * @param   level the stage in device resumption process that we want the
 *                device to be put in
 *
 * @return  The function returns 0 on success and -1 on failure
 */
static int mxcnd_resume(struct device *dev, u32 level)
{

	struct mtd_info *info = dev_get_drvdata(dev);
	int ret = 0;
	DEBUG(MTD_DEBUG_LEVEL0, "MXC_ND : NAND resume (Level = %d)\n", level);

	switch (level) {
	case RESUME_POWER_ON:
		/* Enable the NFC clock */
		mxc_clks_enable(NFC_CLK);
		break;

	case RESUME_RESTORE_STATE:
		if (info)
			info->resume(info);
		break;

	case RESUME_ENABLE:
		/* Do nothing */
		break;

	default:
		return -1;
	}
	return ret;
}

#else
#define mxcnd_suspend   NULL
#define mxcnd_resume    NULL
#endif				/* CONFIG_PM */

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxcnd_driver = {

	.name = "mxc_nand_flash",
	.bus = &platform_bus_type,
	.probe = mxcnd_probe,
	.remove = __exit_p(mxcnd_remove),
	.suspend = mxcnd_suspend,
	.resume = mxcnd_resume,
};

/*!
 * Device Definition for MXC NAND
 */
static struct platform_device  mxcnd_device = {
	.name	= "mxc_nand_flash",
	.id	= 0,
};

#if 0
#if defined(CONFIG_MOT_FEAT_MTD_FS)
/* TEST code */
#ifdef CONFIG_MOT_FEAT_LARGEPG_NAND
u8 buf[2112];
#else
u8 buf[528];
#endif
void test_erase_block(struct mtd_info *mtd, u32 block)
{
        u_char status;
        
        mxc_nand_command(mtd, NAND_CMD_ERASE1, -1, block*(mtd->erasesize/mtd->oobblock));
        mxc_nand_command(mtd, NAND_CMD_ERASE2, -1, -1);
        mxc_nand_command(mtd, NAND_CMD_STATUS, -1, -1);

        status = mxc_nand_read_byte(mtd);
        if (status & NAND_STATUS_FAIL)
        {
            printk("ERASE FAILED (block = %d, status = 0x%x)\n", block, status);
        }
}


void test_read_page(struct mtd_info *mtd, u32 page, u8 val)
{
        int i;
        u_char status;
        
        printk("Reading page %d\n", page);

        for (i = 0; i < sizeof(buf); i++) buf[i] = 0x0;
        
        mxc_nand_command(mtd, NAND_CMD_READ0, 0, page);
        mxc_nand_read_buf(mtd, buf, sizeof(buf));
        mxc_nand_command(mtd, NAND_CMD_STATUS, -1, -1);

        status = mxc_nand_read_byte(mtd);
        if (status & NAND_STATUS_FAIL)
        {
            printk("NAND READ FAILED (status = 0x%x)\n", status);
        }

        for (i = 0; i < sizeof(buf); i++) {
            if (buf[i] != val) {                
                printk("READ ERROR:  buf[%d] = 0x%x\n", i, buf[i]);
                break;
            }
        }
}

void test_write_page(struct mtd_info *mtd, u32 page, u8 val)
{
        int i;
        u_char status;
    
        for (i = 0; i < sizeof(buf); i++) buf[i] = val;

        printk("Writing page %d\n", page);
        mxc_nand_select_chip(mtd, 0);
        mxc_nand_command(mtd, NAND_CMD_SEQIN, 0, page);
        mxc_nand_write_buf(mtd, buf, sizeof(buf));
        mxc_nand_command(mtd, NAND_CMD_PAGEPROG, -1, -1);
        mxc_nand_command(mtd, NAND_CMD_STATUS, -1, -1);

        while (!(mxc_nand_dev_ready(mtd)));

        status = mxc_nand_read_byte(mtd);
        if (status & NAND_STATUS_FAIL)
        {
            printk("WRITE FAILED (status = 0x%x)\n", status);
        }
        mxc_nand_select_chip(mtd, -1);
        
}

static void mxc_test_write(struct mtd_info *mtd)
{
        int i, page, block, numpg = mtd->erasesize/mtd->oobblock;

        block = 100;

        test_erase_block(mtd, block);

        for (i = 0, page = block*numpg; i < numpg; i++, page++)
        {   
            test_read_page(mtd, page, 0xff);
        }

        page = block * numpg;
        test_write_page(mtd, page, 0x55);
        page++;
        test_write_page(mtd, page, 0xAA);

        page = block * numpg;
        test_read_page(mtd, page, 0x55);
        page++;
        test_read_page(mtd, page, 0xAA);

        for (i = 2, page = block * 32 + 2; i < 32; i++, page++)
        {   
            test_read_page(mtd, page, 0xff);
        }
        
        test_erase_block(mtd, block);
            
}

static void mxc_test_erase_all(struct mtd_info *mtd)
{
        int block;

        printk("Erasing:");
        for (block = 0; block < mtd->size/mtd->erasesize; block++)
	{
                printk(".");
                test_erase_block(mtd, block);
        }

        printk("\nErasing completed on total of %d blocks\n", block);
}
#endif /* CONFIG_MOT_FEAT_MTD_FS */
#endif /* End test code */

/*
 * Main initialization routine
 */
int __init mxc_nd_init (void)
{
#ifdef CONFIG_MOT_FEAT_PM
        /* register to MPM for busy/non-busy advice */
        mxc_nand_mpm_advice_id = mpm_register_with_mpm("mxc_nand_flash");
        if ( mxc_nand_mpm_advice_id < 0 ) {
                    /* we needn't unregister here */
                printk(KERN_ERR "Can't register mxc_nand to mpm ( %d )\n", mxc_nand_mpm_advice_id);
        }
#endif        
	/* Register the device driver structure. */
	pr_info("MXC MTD nand Driver %s\n", DVR_VER);
	if (driver_register(&mxcnd_driver) != 0) {
		printk(KERN_ERR "Driver register failed for mxcnd_driver\n");
		return -ENODEV;
	}
	if (platform_device_register(&mxcnd_device) != 0) {
		printk(KERN_ERR "Platform Device register failed for mxcnd_driver\n");
		driver_unregister(&mxcnd_driver);
		return -ENODEV;
	}
	return 0;
}

/*
 * Clean up routine
 */
static void __exit mxc_nd_cleanup (void)
{
	/* Unregister the device structure */
	platform_device_unregister(&mxcnd_device);
	driver_unregister(&mxcnd_driver);
#ifdef CONFIG_MOT_FEAT_PM
        if (mxc_nand_mpm_advice_id >= 0)
          mpm_unregister_with_mpm(mxc_nand_mpm_advice_id);
#endif        
}

module_init(mxc_nd_init);
module_exit(mxc_nd_cleanup);

MODULE_AUTHOR("Freescale");
MODULE_DESCRIPTION("MXC NAND MTD driver");
MODULE_LICENSE("GPL");
